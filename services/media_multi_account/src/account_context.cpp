/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define MLOG_TAG "AccountContext"

#include "account_context.h"

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

#include "preferences.h"
#include "preferences_helper.h"
#include "media_pure_file_utils.h"
#include "media_path_utils.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"

#include "media_log.h"
#include "nlohmann/json.hpp"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace OHOS {
namespace Media {

constexpr const char* ACCOUNT_PREFS_PATH = "/data/storage/el2/base/preferences/medialibrary_multi_account_config.xml";
constexpr const char* ACCOUNTS_CONFIG_KEY = "accounts_config";
constexpr const char* ACCOUNTS_LIST_KEY = "accounts";
constexpr const char* FOREGROUND_ACCOUNT_ID_KEY = "foregroundAccountId";
constexpr const char* DELETED_ACCOUNTS_KEY = "deleted_accounts";
constexpr int32_t MIN_ACCOUNT_ID = 100000;
constexpr int32_t MAX_ACCOUNT_ID = 999999;
const std::string FOREGROUND_ACCOUNT_ID_XML = std::string(CONST_MEDIA_DB_DIR) +
    "/rdb/medialibrary_foreground_account_id.xml";
constexpr mode_t SHARED_XML_PERMISSION = 0771;

static std::shared_ptr<NativePreferences::Preferences> GetConfigPrefs()
{
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(ACCOUNT_PREFS_PATH, errCode);
    if (prefs == nullptr) {
        MEDIA_WARN_LOG("failed to get preferences, err=%{public}d", errCode);
    }
    return prefs;
}

static bool JsonContainsAccountId(const nlohmann::json &arr, int32_t accountId)
{
    for (const auto &item : arr) {
        if (item.is_number() && item.get<int32_t>() == accountId) {
            return true;
        }
    }
    return false;
}

bool AccountContext::IsValidAccountId(int32_t accountId)
{
    return accountId >= MIN_ACCOUNT_ID && accountId <= MAX_ACCOUNT_ID;
}

int32_t AccountContext::GetForegroundAccountId() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return foregroundAccountId_;
}

bool AccountContext::IsForegroundAccount(int32_t accountId) const
{
    if (!IsValidAccountId(accountId)) {
        MEDIA_ERR_LOG("invalid accountId=%{public}d", accountId);
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    return foregroundAccountId_ == accountId;
}

int32_t AccountContext::GetDefaultAccountId()
{
    return DEFAULT_ACCOUNT_ID;
}

size_t AccountContext::GetAccountCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return accounts_.size();
}

bool AccountContext::HasAccount(int32_t accountId) const
{
    if (!IsValidAccountId(accountId)) {
        MEDIA_ERR_LOG("invalid accountId=%{public}d", accountId);
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    return accounts_.count(accountId) > 0;
}

std::unordered_set<int32_t> AccountContext::GetAllAccounts() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return accounts_;
}

nlohmann::json AccountContext::LoadJson(std::shared_ptr<NativePreferences::Preferences>& prefs)
{
    std::string jsonStr = prefs->GetString(ACCOUNTS_CONFIG_KEY, "");
    if (jsonStr.empty()) {
        return nlohmann::json::object();
    }
    auto result = nlohmann::json::parse(jsonStr, nullptr, false);
    if (result.is_discarded()) {
        MEDIA_ERR_LOG("parse failed");
        return nlohmann::json::object();
    }
    return result;
}

bool AccountContext::AddAccount(int32_t accountId)
{
    if (!IsValidAccountId(accountId)) {
        MEDIA_ERR_LOG("invalid accountId=%{public}d", accountId);
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (accounts_.count(accountId) > 0) {
        MEDIA_INFO_LOG("account already exists, accountId=%{public}d", accountId);
        return false;
    }

    SaveAccountToPref(accountId);
    accounts_.insert(accountId);
    MEDIA_INFO_LOG("added accountId=%{public}d, mapSize=%{public}zu", accountId, accounts_.size());
    return true;
}

void AccountContext::SetForegroundAccount(int32_t accountId)
{
    if (!IsValidAccountId(accountId)) {
        MEDIA_ERR_LOG("invalid accountId=%{public}d", accountId);
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    foregroundAccountId_ = accountId;
    SaveForegroundAccountIdToPref();
    MEDIA_INFO_LOG("accountId: %{public}d", accountId);
}

int32_t AccountContext::RemoveAccount(int32_t accountId)
{
    if (!IsValidAccountId(accountId)) {
        MEDIA_ERR_LOG("invalid accountId=%{public}d", accountId);
        return E_INVALID_VALUES;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (accounts_.erase(accountId) == 0) {
        MEDIA_WARN_LOG("accountId=%{public}d not found in set", accountId);
        return E_ACCOUNT_NOT_FOUND;
    }

    RemoveAccountFromPref(accountId);
    MEDIA_INFO_LOG("removed accountId=%{public}d, mapSize=%{public}zu",
        accountId, accounts_.size());
    return E_OK;
}

void AccountContext::Init()
{
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(ACCOUNT_PREFS_PATH, errCode);
    if (prefs == nullptr) {
        MEDIA_ERR_LOG("failed to get preferences, err=%{public}d", errCode);
        return;
    }
    nlohmann::json accountConfig = LoadJson(prefs);

    std::lock_guard<std::mutex> lock(mutex_);
    if (accountConfig.contains(ACCOUNTS_LIST_KEY) && accountConfig[ACCOUNTS_LIST_KEY].is_array()) {
        for (const auto& item : accountConfig[ACCOUNTS_LIST_KEY]) {
            if (item.is_number()) {
                int32_t accountId = item.get<int32_t>();
                accounts_.insert(accountId);
            }
        }
    }

    if (accountConfig.contains(FOREGROUND_ACCOUNT_ID_KEY) && accountConfig[FOREGROUND_ACCOUNT_ID_KEY].is_number()) {
        foregroundAccountId_ = accountConfig[FOREGROUND_ACCOUNT_ID_KEY].get<int32_t>();
        EnsureSharedXmlReady();
        SaveForegroundAccountIdToPref();
    }

    MEDIA_INFO_LOG("loaded %{public}zu accounts, foregroundAccountId=%{public}d",
        accounts_.size(), foregroundAccountId_);
}

void AccountContext::SaveAccountToPref(int32_t accountId)
{
    if (!IsValidAccountId(accountId)) {
        MEDIA_ERR_LOG("invalid accountId=%{public}d", accountId);
        return;
    }
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(ACCOUNT_PREFS_PATH, errCode);
    if (prefs == nullptr) {
        MEDIA_WARN_LOG("failed to get preferences, err=%{public}d", errCode);
        return;
    }

    nlohmann::json accountConfig = LoadJson(prefs);
    if (!accountConfig.contains(ACCOUNTS_LIST_KEY) || !accountConfig[ACCOUNTS_LIST_KEY].is_array()) {
        accountConfig[ACCOUNTS_LIST_KEY] = nlohmann::json::array();
    }
    // 防止 accountId 在 pref 中重复
    auto& arr = accountConfig[ACCOUNTS_LIST_KEY];
    for (const auto& item : arr) {
        if (item.is_number() && item.get<int32_t>() == accountId) {
            MEDIA_INFO_LOG("accountId=%{public}d already exists, skip", accountId);
            return;
        }
    }
    arr.push_back(accountId);
    prefs->PutString(ACCOUNTS_CONFIG_KEY, accountConfig.dump());
    prefs->FlushSync();
}

void AccountContext::RemoveAccountFromPref(int32_t accountId)
{
    if (!IsValidAccountId(accountId)) {
        MEDIA_ERR_LOG("invalid accountId=%{public}d", accountId);
        return;
    }
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(ACCOUNT_PREFS_PATH, errCode);
    if (prefs == nullptr) {
        MEDIA_WARN_LOG("failed to get preferences, err=%{public}d", errCode);
        return;
    }

    nlohmann::json accountConfig = LoadJson(prefs);
    if (accountConfig.contains(ACCOUNTS_LIST_KEY) && accountConfig[ACCOUNTS_LIST_KEY].is_array()) {
        auto& arr = accountConfig[ACCOUNTS_LIST_KEY];
        for (auto it = arr.begin(); it != arr.end(); ++it) {
            if (it->is_number() && it->get<int32_t>() == accountId) {
                arr.erase(it);
                break;
            }
        }
    }
    prefs->PutString(ACCOUNTS_CONFIG_KEY, accountConfig.dump());
    prefs->FlushSync();
}

void AccountContext::EnsureSharedXmlReady()
{
    std::string parentDir = MediaPathUtils::GetParentPath(FOREGROUND_ACCOUNT_ID_XML);
    if (!MediaPureFileUtils::CreateDirectory(parentDir)) {
        MEDIA_WARN_LOG("failed to create parent dir: %{public}s", parentDir.c_str());
    }
    if (access(FOREGROUND_ACCOUNT_ID_XML.c_str(), F_OK) != 0) {
        int32_t fd = open(FOREGROUND_ACCOUNT_ID_XML.c_str(), O_CREAT | O_WRONLY, SHARED_XML_PERMISSION);
        if (fd >= 0) {
            close(fd);
            chmod(FOREGROUND_ACCOUNT_ID_XML.c_str(), SHARED_XML_PERMISSION);
            MEDIA_INFO_LOG("created shared xml: %{public}s", FOREGROUND_ACCOUNT_ID_XML.c_str());
        } else {
            MEDIA_WARN_LOG("failed to create shared xml, errno=%{public}d", errno);
        }
    }
}

void AccountContext::SaveForegroundAccountIdToPref()
{
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(ACCOUNT_PREFS_PATH, errCode);
    if (prefs == nullptr) {
        MEDIA_WARN_LOG("failed to get preferences, err=%{public}d", errCode);
        return;
    }

    // 1.保存
    nlohmann::json accountConfig = LoadJson(prefs);
    accountConfig[FOREGROUND_ACCOUNT_ID_KEY] = foregroundAccountId_;
    prefs->PutString(ACCOUNTS_CONFIG_KEY, accountConfig.dump());
    prefs->FlushSync();

    // Also write foregroundAccountId to shared file for cross-process (client) access.
    int32_t xmlErrCode = 0;
    auto xmlPrefs = NativePreferences::PreferencesHelper::GetPreferences(FOREGROUND_ACCOUNT_ID_XML, xmlErrCode);
    if (xmlPrefs == nullptr) {
        MEDIA_ERR_LOG("failed to get shared prefs, err=%{public}d", xmlErrCode);
        return;
    }
    int32_t ret = xmlPrefs->PutInt(FOREGROUND_ACCOUNT_ID_KEY, foregroundAccountId_);
    if (ret != NativePreferences::E_OK) {
        MEDIA_ERR_LOG("PutInt failed, ret=%{public}d", ret);
        return;
    }
    ret = xmlPrefs->FlushSync();
    if (ret != NativePreferences::E_OK) {
        MEDIA_ERR_LOG("FlushSync failed, ret=%{public}d", ret);
        return;
    }
    MEDIA_INFO_LOG("saved to shared file, foregroundAccountId=%{public}d", foregroundAccountId_);
}

int32_t AccountContext::MarkAccountDeleted(int32_t accountId)
{
    if (accountId <= 0) {
        MEDIA_ERR_LOG("invalid accountId=%{public}d", accountId);
        return E_INVALID_VALUES;
    }
    std::shared_ptr<NativePreferences::Preferences> prefs = GetConfigPrefs();
    if (prefs == nullptr) {
        return E_FAIL;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    std::string jsonStr = prefs->GetString(DELETED_ACCOUNTS_KEY, "[]");
    nlohmann::json json = nlohmann::json::parse(jsonStr, nullptr, false);
    if (json.is_discarded()) {
        json = nlohmann::json::array();
    }
    if (JsonContainsAccountId(json, accountId)) {
        return E_OK; // already marked - idempotent
    }
    json.push_back(accountId);
    prefs->PutString(DELETED_ACCOUNTS_KEY, json.dump());
    prefs->FlushSync();
    MEDIA_INFO_LOG("MarkAccountDeleted accountId=%{public}d", accountId);
    return E_OK;
}

int32_t AccountContext::UnmarkAccountDeleted(int32_t accountId)
{
    if (accountId <= 0) {
        MEDIA_ERR_LOG("invalid accountId=%{public}d", accountId);
        return E_INVALID_VALUES;
    }
    std::shared_ptr<NativePreferences::Preferences> prefs = GetConfigPrefs();
    if (prefs == nullptr) {
        return E_FAIL;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    std::string jsonStr = prefs->GetString(DELETED_ACCOUNTS_KEY, "[]");
    nlohmann::json json = nlohmann::json::parse(jsonStr, nullptr, false);
    if (json.is_discarded()) {
        // Corrupted content: reset to empty (marker gone = cleanup done)
        prefs->PutString(DELETED_ACCOUNTS_KEY, "[]");
        prefs->FlushSync();
        return E_OK;
    }
    nlohmann::json newArray = nlohmann::json::array();
    for (const auto &item : json) {
        if (item.is_number() && item.get<int32_t>() != accountId) {
            newArray.push_back(item);
        }
    }
    prefs->PutString(DELETED_ACCOUNTS_KEY, newArray.dump());
    prefs->FlushSync();
    MEDIA_INFO_LOG("UnmarkAccountDeleted accountId=%{public}d", accountId);
    return E_OK;
}

std::vector<int32_t> AccountContext::GetDeletedAccountIds()
{
    std::vector<int32_t> result;
    std::shared_ptr<NativePreferences::Preferences> prefs = GetConfigPrefs();
    if (prefs == nullptr) {
        return result;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    std::string jsonStr = prefs->GetString(DELETED_ACCOUNTS_KEY, "[]");
    nlohmann::json json = nlohmann::json::parse(jsonStr, nullptr, false);
    if (json.is_discarded() || !json.is_array()) {
        return result;
    }
    for (const auto &item : json) {
        if (item.is_number()) {
            result.push_back(item.get<int32_t>());
        }
    }
    return result;
}
}  // namespace Media
}  // namespace OHOS
#endif  // MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

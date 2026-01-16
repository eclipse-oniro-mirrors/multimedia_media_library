/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "HeifTranscodingCheckUtils"

#include "heif_transcoding_check_utils.h"

#include <fstream>
#include <mutex>
#include <sstream>
#include <thread>
#include <sys/stat.h>

#include "heif_bundle_info_cache.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_tracer.h"
#include "bundle_mgr_proxy.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "bundle_constants.h"

using std::string;
using std::unordered_map;
using std::mutex;
using namespace nlohmann;
namespace OHOS {
namespace Media {
const std::string HEIF_TRANSCODING_CHECKLIST_NAME = "heif_transcoding_checklist.json";
const std::string HEIF_TRANSCODING_CLECK_LIST_JSON_LOCAL_PATH =
    "/system/etc/com.ohos.medialibrary.medialibrarydata/heif_transcoding/" + HEIF_TRANSCODING_CHECKLIST_NAME;
const string DUE_INSTALL_DIR =
    "/data/service/el1/public/update/param_service/install/system/etc/"
    "com.ohos.medialibrary.medialibrarydata/heif_transcoding/";
const string COTA_UPDATE_EVENT = "usual.event.DUE_HAP_CFG_UPDATED";
const string RECEIVE_UPDATE_MESSAGE = "ohos.permission.RECEIVE_UPDATE_MESSAGE";
const string COTA_EVENT_INFO_TYPE = "type";
const string COTA_EVENT_INFO_SUBTYPE = "subtype";
const string COTA_EVENT_INFO_SUBTYPE_VALUE = "heif_transcoding";
const string LIST_STRATEGY = "listStrategy";
const string LIST_STRATEGY_WHITELIST = "whiteList";
const string LIST_STRATEGY_DENYLIST = "denyList";

const int CONFIG_EVENT_SUBSCRIBE_DELAY_TIME = 300;

using WhiteList = std::unordered_map<std::string, std::string>;
using DenyList = std::unordered_set<std::string>;

WhiteList HeifTranscodingCheckUtils::whiteList_;
DenyList HeifTranscodingCheckUtils::denyList_;
bool HeifTranscodingCheckUtils::isUseWhiteList_ = false;

sptr<AppExecFwk::IBundleMgr> HeifTranscodingCheckUtils::bundleMgr_ = nullptr;
mutex HeifTranscodingCheckUtils::bundleMgrMutex_;
std::shared_ptr<EventFwk::CommonEventSubscriber> HeifTranscodingCheckUtils::cotaUpdateSubscriber_{};
class HeifTranscodingCheckUtils::CotaUpdateReceiver : public EventFwk::CommonEventSubscriber {
public:
    explicit CotaUpdateReceiver(const EventFwk::CommonEventSubscribeInfo &subscribeInfo);
    ~CotaUpdateReceiver() {}
    void OnReceiveEvent(const EventFwk::CommonEventData &data) override;
};

HeifTranscodingCheckUtils::CotaUpdateReceiver::CotaUpdateReceiver(
    const EventFwk::CommonEventSubscribeInfo &subscribeInfo)
    : EventFwk::CommonEventSubscriber(subscribeInfo)
{
}

void HeifTranscodingCheckUtils::CotaUpdateReceiver::OnReceiveEvent(const EventFwk::CommonEventData &data)
{
    std::string action = data.GetWant().GetAction();
    std::string type = data.GetWant().GetStringParam(COTA_EVENT_INFO_TYPE);
    std::string subtype = data.GetWant().GetStringParam(COTA_EVENT_INFO_SUBTYPE);
    MEDIA_INFO_LOG("CotaUpdateReceiver: action[%{public}s], type[%{public}s], subType[%{public}s]", action.c_str(),
        type.c_str(), subtype.c_str());

    if (action != COTA_UPDATE_EVENT || type != COTA_EVENT_INFO_SUBTYPE_VALUE) {
        MEDIA_ERR_LOG("other action, ignore.");
        return;
    }

    HeifTranscodingCheckUtils::ReadCheckList();
}

bool IsFileExists(const std::string &fileName)
{
    struct stat statInfo {};
    return ((stat(fileName.c_str(), &statInfo)) == 0);
}

sptr<AppExecFwk::IBundleMgr> HeifTranscodingCheckUtils::GetSysBundleManager()
{
    if (bundleMgr_ != nullptr) {
        return bundleMgr_;
    }

    std::lock_guard<mutex> lock(bundleMgrMutex_);
    if (bundleMgr_ != nullptr) {
        return bundleMgr_;
    }

    auto systemAbilityMgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityMgr == nullptr) {
        MEDIA_ERR_LOG("Failed to get SystemAbilityManager.");
        return nullptr;
    }

    auto bundleObj = systemAbilityMgr->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (bundleObj == nullptr) {
        MEDIA_ERR_LOG("Remote object is nullptr.");
        return nullptr;
    }

    auto bundleMgr = iface_cast<AppExecFwk::IBundleMgr>(bundleObj);
    if (bundleMgr == nullptr) {
        MEDIA_ERR_LOG("Failed to iface_cast");
        return nullptr;
    }
    bundleMgr_ = bundleMgr;

    return bundleMgr_;
}

int32_t HeifTranscodingCheckUtils::ReadCheckList()
{
    std::ifstream jFile;
    if (IsFileExists(DUE_INSTALL_DIR + HEIF_TRANSCODING_CHECKLIST_NAME)) {
        jFile.open(DUE_INSTALL_DIR + HEIF_TRANSCODING_CHECKLIST_NAME);
        if (!jFile.is_open()) {
            MEDIA_WARN_LOG("Failed to open file in DUE install directory, falling back to local path");
            jFile.open(HEIF_TRANSCODING_CLECK_LIST_JSON_LOCAL_PATH);
        }
    } else {
        jFile.open(HEIF_TRANSCODING_CLECK_LIST_JSON_LOCAL_PATH);
    }
    if (!jFile.is_open()) {
        MEDIA_ERR_LOG("Failed to open file, Error: %{public}s", std::strerror(errno));
        return E_FAIL;
    }

    std::stringstream buffer;
    buffer << jFile.rdbuf();
    std::string jStr = buffer.str();
    nlohmann::json checkListJson;
    jFile.close();
    if (!jStr.empty() && nlohmann::json::accept(jStr)) {
        checkListJson = nlohmann::json::parse(jStr, nullptr, false);
        if (checkListJson.is_discarded()) {
            MEDIA_ERR_LOG("JSON parse error");
            return E_FAIL;
        }
    } else {
        MEDIA_ERR_LOG("Failed to get json, Error: %{public}s", std::strerror(errno));
        return E_FAIL;
    }

    ClearCheckList();
    ClearBundleInfoInCache();

    if (!checkListJson.contains(LIST_STRATEGY) || !checkListJson[LIST_STRATEGY].is_string()) {
        MEDIA_ERR_LOG("Invalid or missing 'listStrategy' in json");
        return E_FAIL;
    }
    string listStrategy = checkListJson[LIST_STRATEGY].get<std::string>();
    isUseWhiteList_ = (listStrategy == LIST_STRATEGY_WHITELIST);
    if (isUseWhiteList_) {
        return ParseWhiteList(checkListJson);
    }
    return ParseDenyList(checkListJson);
}

void HeifTranscodingCheckUtils::ClearCheckList()
{
    whiteList_.clear();
    denyList_.clear();
    isUseWhiteList_ = false;
}

int32_t HeifTranscodingCheckUtils::SubscribeCotaUpdatedEvent()
{
    CHECK_AND_RETURN_RET_WARN_LOG(cotaUpdateSubscriber_ == nullptr, E_OK, "cota update event already subscribed.");

    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(COTA_UPDATE_EVENT);
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    subscribeInfo.SetPermission(RECEIVE_UPDATE_MESSAGE);
    cotaUpdateSubscriber_ = std::make_shared<CotaUpdateReceiver>(subscribeInfo);
    CHECK_AND_RETURN_RET_LOG(cotaUpdateSubscriber_ != nullptr, E_FAIL, "cota update subscriber nullptr.");

    if (EventFwk::CommonEventManager::SubscribeCommonEvent(cotaUpdateSubscriber_)) {
        MEDIA_INFO_LOG("Subscribe cota update event successed");
        return E_OK;
    }
    MEDIA_ERR_LOG("Subscribe cota update event fail");
    return E_FAIL;
}

void HeifTranscodingCheckUtils::UnsubscribeCotaUpdatedEvent()
{
    CHECK_AND_RETURN_WARN_LOG(cotaUpdateSubscriber_ != nullptr, "cota update event not subscribed.");

    bool subscribeResult = EventFwk::CommonEventManager::UnSubscribeCommonEvent(cotaUpdateSubscriber_);
    MEDIA_INFO_LOG("subscribeResult = %{public}d", subscribeResult);
    cotaUpdateSubscriber_ = nullptr;
}

int32_t HeifTranscodingCheckUtils::InitCheckList()
{
    MediaLibraryTracer tracer;
    tracer.Start("InitCheckList Excute");
    CHECK_AND_RETURN_RET_INFO_LOG(ReadCheckList() == E_OK, E_FAIL, "ReadCheckList failed");
    CHECK_AND_RETURN_RET_INFO_LOG(SubscribeCotaUpdatedEvent() == E_OK, E_FAIL, "SubscribeCotaUpdatedEvent failed");
    ClearBundleInfoInCache();
    return E_OK;
}

std::vector<int> SplitVersion(const std::string& version)
{
    std::vector<int> parts;
    std::stringstream ss(version);
    std::string item;
    while (std::getline(ss, item, '.')) {
        if (MediaFileUtils::IsValidInteger(item)) {
            parts.push_back(std::stoi(item));
        }
    }
    return parts;
}

bool CompareVersion(const std::string& v1, const std::string& v2)
{
    std::vector<int> parts1 = SplitVersion(v1);
    std::vector<int> parts2 = SplitVersion(v2);

    size_t n = std::max(parts1.size(), parts2.size());
    parts1.resize(n, 0);
    parts2.resize(n, 0);

    for (size_t i = 0; i < n; ++i) {
        if (parts1[i] > parts2[i]) {
            return true;
        }
        if (parts1[i] < parts2[i]) {
            return false;
        }
    }
    return true;
}

int32_t HeifTranscodingCheckUtils::ParseWhiteList(const nlohmann::json &checkListJson)
{
    if (!checkListJson.contains(LIST_STRATEGY_WHITELIST) || !checkListJson[LIST_STRATEGY_WHITELIST].is_array()) {
        MEDIA_ERR_LOG("Invalid or missing 'whiteList' in json");
        return E_FAIL;
    }
    for (const auto &item : checkListJson[LIST_STRATEGY_WHITELIST]) {
        if (item.is_string()) {
            std::string str = item.get<std::string>();
            size_t atPos = str.find('@');
            if (atPos == std::string::npos || atPos == 0 || atPos == str.size() - 1) {
                MEDIA_ERR_LOG("Invalid format in whiteList item: %{public}s", str.c_str());
                continue;
            }
            std::string bundleName = str.substr(0, atPos);
            std::string version = str.substr(atPos + 1);
            MEDIA_DEBUG_LOG("Bundle %{public}s with version %{public}s added to whiteList",
                bundleName.c_str(), version.c_str());
            auto it = whiteList_.find(bundleName);
            if (it == whiteList_.end()) {
                whiteList_[bundleName] = version;
                continue;
            }
            // If the bundle already exists, keep the higher version
            if (CompareVersion(version, it->second)) {
                MEDIA_DEBUG_LOG("Bundle %{public}s version updated from %{public}s to %{public}s",
                    bundleName.c_str(), it->second.c_str(), version.c_str());
                it->second = version;
            }
        }
    }
    return E_OK;
}

int32_t HeifTranscodingCheckUtils::ParseDenyList(const nlohmann::json &checkListJson)
{
    if (!checkListJson.contains(LIST_STRATEGY_DENYLIST) || !checkListJson[LIST_STRATEGY_DENYLIST].is_array()) {
        MEDIA_ERR_LOG("Invalid or missing 'denyList' in json");
        return E_FAIL;
    }
    for (const auto &item : checkListJson[LIST_STRATEGY_DENYLIST]) {
        if (item.is_string()) {
            std::string str = item.get<std::string>();
            MEDIA_DEBUG_LOG("Bundle %{public}s added to denyList", str.c_str());
            denyList_.insert(str);
        }
    }
    return E_OK;
}

void HeifTranscodingCheckUtils::ClearBundleInfoInCache()
{
    HeifBundleInfoCache::ClearBundleInfoInCache();
}

bool HeifTranscodingCheckUtils::CanSupportedCompatibleDuplicate(const std::string &bundleName)
{
    if (isUseWhiteList_) {
        auto it = whiteList_.find(bundleName);
        if (it == whiteList_.end()) {
            MEDIA_INFO_LOG("Bundle %{public}s is not in white list", bundleName.c_str());
            return true;
        }
        bool isSupport = false;
        if (HeifBundleInfoCache::GetBundleCacheInfo(bundleName, isSupport)) {
            MEDIA_INFO_LOG("[cache] %{public}s is use jpg [%{public}d]", bundleName.c_str(), isSupport);
            return isSupport;
        }
        AppExecFwk::BundleInfo bundleInfo;
        ErrCode state = GetSysBundleManager()->GetBundleInfoV9(
            bundleName, static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_APPLICATION),
            bundleInfo, OHOS::AppExecFwk::Constants::START_USERID);
        if (state != 0) {
            MEDIA_ERR_LOG("Failed to get bundle info for %{public}s, Error: %{public}d", bundleName.c_str(), state);
            return false;
        }
        if (!CompareVersion(bundleInfo.versionName, it->second)) {
            HeifBundleInfoCache::InsertBundleCacheInfo(bundleName, true);
            MEDIA_INFO_LOG("Bundle %{public}s version %{public}s is less than white list version %{public}s",
                bundleName.c_str(), bundleInfo.versionName.c_str(), it->second.c_str());
            return true;
        }
        HeifBundleInfoCache::InsertBundleCacheInfo(bundleName, false);
        MEDIA_INFO_LOG("Bundle %{public}s version %{public}s is in white list and meets the version requirement",
            bundleName.c_str(), bundleInfo.versionName.c_str());
        return false;
    } else {
        if (denyList_.find(bundleName) != denyList_.end()) {
            MEDIA_INFO_LOG("Bundle %{public}s is in deny list", bundleName.c_str());
            return true;
        }
        MEDIA_INFO_LOG("Bundle %{public}s is not in deny list", bundleName.c_str());
        return false;
    }
}
}
}
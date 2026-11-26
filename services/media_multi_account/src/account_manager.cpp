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

#define MLOG_TAG "AccountManager"

#include "account_manager.h"

#include <unistd.h>

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

#include "account_context.h"
#include "account_enable_checker.h"
#include "account_rdb_upgrade_manager.h"
#include "media_account_dir_utils.h"
#include "media_log.h"
#include "media_path_utils.h"
#include "medialibrary_errno.h"
#include "ohos_account_kits.h"

namespace OHOS {
namespace Media {
constexpr uint32_t BASE_USER_RANGE = 200000;

AccountManager& AccountManager::GetInstance()
{
    static AccountManager instance;
    return instance;
}

static int32_t GetSubProfileId()
{
    int32_t userId = getuid() / BASE_USER_RANGE;
    int32_t subProfileId = -1;
    ErrCode errCode =
        AccountSA::OhosAccountKits::GetInstance().GetOsAccountForegroundSubProfileId(userId, subProfileId);
    if (errCode != ERR_OK) {
        MEDIA_WARN_LOG("failed for userId=%{public}d, err=%{public}d", userId, errCode);
        return E_ERR;
    }
    return subProfileId;
}

static void EnsureForegroundAccount(AccountContext &context)
{
    int32_t subProfileId = GetSubProfileId();
    if (subProfileId <= 0) {
        MEDIA_WARN_LOG("invalid subProfileId=%{public}d, skip", subProfileId);
        return;
    }
    if (context.IsForegroundAccount(subProfileId)) {
        MEDIA_INFO_LOG("current account id is foreground.");
        return;
    }
    context.SetForegroundAccount(subProfileId);
}

bool AccountManager::Init()
{
    MEDIA_INFO_LOG("Init: start");
    // Publish disabled mode too: callers must not duplicate the screen policy.
    MediaPathUtils::RegisterGetAccountId([](int32_t& id) -> bool {
        return AccountManager::GetInstance().GetForegroundAccountId(id);
    });
    if (!AccountEnableChecker::IsMultiAccountEnabled()) {
        MEDIA_INFO_LOG("multi-account not enabled.");
        return false;
    }

    // 1. Initialize context from xml
    accountContext_.Init();

    // 2. Check if default account exists
    if (!accountContext_.HasAccount(AccountContext::GetDefaultAccountId())) {
        MEDIA_INFO_LOG("DEFAULT_ACCOUNT_ID not found, added manually.");
        accountContext_.AddAccount(AccountContext::GetDefaultAccountId());
    }

    // 3. Check if current account is set as foreground
    EnsureForegroundAccount(accountContext_);

    MEDIA_INFO_LOG("initialized: accountCount=%{public}zu.", accountContext_.GetAccountCount());
    return true;
}

int32_t AccountManager::Login()
{
    if (!AccountEnableChecker::IsMultiAccountEnabled()) {
        MEDIA_WARN_LOG("IsMultiAccountEnabled: false.");
        return E_OK;
    }

    int32_t subProfileId = GetSubProfileId();
    if (subProfileId <= 0) {
        MEDIA_WARN_LOG("invalid subProfileId=%{public}d, skip", subProfileId);
        return E_INVALID_VALUES;
    }

    MEDIA_INFO_LOG("accountId=%{public}d", subProfileId);
    bool isNewAccount = accountContext_.AddAccount(subProfileId);
    accountContext_.SetForegroundAccount(subProfileId);

    // 标记当前登录账号为优先升级，使正在进行的升级循环优先处理该账号
    AccountRdbUpgradeManager::GetInstance().SetPriorityAccount(subProfileId);

    // Account login
    HwidLoginInternal(accountContext_.GetForegroundAccountId());

    MEDIA_INFO_LOG("completed, accountId=%{public}d, accountCount=%{public}zu",
        subProfileId, accountContext_.GetAccountCount());
    return E_OK;
}

int32_t AccountManager::Logout()
{
    if (!AccountEnableChecker::IsMultiAccountEnabled()) {
        MEDIA_WARN_LOG("IsMultiAccountEnabled: false.");
        return E_OK;
    }

    int32_t subProfileId = GetSubProfileId();
    if (subProfileId <= 0) {
        MEDIA_WARN_LOG("invalid subProfileId=%{public}d, skip", subProfileId);
        return E_INVALID_VALUES;
    }

    MEDIA_INFO_LOG("accountId=%{public}d", subProfileId);

    if (!accountContext_.HasAccount(subProfileId)) {
        MEDIA_INFO_LOG("accountId=%{public}d not in cache, skip (idempotent), ret=E_OK", subProfileId);
        return E_OK;
    }

    // Set to BACKGROUND — keep cache/data/directory
    // DB behavior: foreground switches to visitor (accountId=1000).
    // rdbStoreMap_ has no entry for visitor, so GetRaw()/GetRawChecked() return nullptr.
    // This is expected: media library is unavailable after Logout until a new account Login.
    // The previous account's DB remains in rdbStoreMap_ and can be reused if the same account logs in again.
    int32_t defaultAccountId = accountContext_.GetDefaultAccountId();
    accountContext_.SetForegroundAccount(defaultAccountId);

    // Notify MediaLibrary to cleanup via callback
    HwidLogoutInternal(subProfileId);

    MEDIA_INFO_LOG("accountId=%{public}d set to BACKGROUND", subProfileId);
    return E_OK;
}

int32_t AccountManager::Delete(int32_t accountId)
{
    MEDIA_INFO_LOG("accountId=%{public}d", accountId);

    if (accountId <= 0) {
        MEDIA_ERR_LOG("invalid accountId=%{public}d", accountId);
        return E_INVALID_VALUES;
    }

    // 1. Account management
    if (!accountContext_.HasAccount(accountId)) {
        MEDIA_ERR_LOG("accountId=%{public}d not in cache, ret=E_ACCOUNT_NOT_FOUND", accountId);
        return E_ACCOUNT_NOT_FOUND;
    }

    if (accountContext_.IsForegroundAccount(accountId)) {
        accountContext_.SetForegroundAccount(AccountContext::GetDefaultAccountId());
        MEDIA_INFO_LOG("accountId=%{public}d set to BACKGROUND and wait to delete", accountId);
    }

    accountContext_.RemoveAccount(accountId);

    MEDIA_INFO_LOG("accountId=%{public}d removed, accountCount=%{public}zu",
        accountId, accountContext_.GetAccountCount());
    return E_OK;
}

int32_t AccountManager::MarkAccountDeleted(int32_t accountId)
{
    if (!AccountEnableChecker::IsMultiAccountEnabled()) {
        return E_FAIL;
    }
    return accountContext_.MarkAccountDeleted(accountId);
}

int32_t AccountManager::UnmarkAccountDeleted(int32_t accountId)
{
    if (!AccountEnableChecker::IsMultiAccountEnabled()) {
        return E_FAIL;
    }
    return accountContext_.UnmarkAccountDeleted(accountId);
}

std::vector<int32_t> AccountManager::GetDeletedAccountIds()
{
    if (!AccountEnableChecker::IsMultiAccountEnabled()) {
        return {};
    }
    return accountContext_.GetDeletedAccountIds();
}

// --- Callback setters ---

void AccountManager::SetHwidOnStartHandler(HwidOnStartHandler handler)
{
    if (!AccountEnableChecker::IsMultiAccountEnabled()) {
        return;
    }
    std::lock_guard<std::mutex> lock(handlerMutex_);
    hwidOnStartHandler_ = std::move(handler);
}

void AccountManager::SetHwidOnStopHandler(HwidOnStopHandler handler)
{
    if (!AccountEnableChecker::IsMultiAccountEnabled()) {
        return;
    }
    std::lock_guard<std::mutex> lock(handlerMutex_);
    hwidOnStopHandler_ = std::move(handler);
}

void AccountManager::RegisterObserver(std::shared_ptr<AccountObserver> observer)
{
    if (!AccountEnableChecker::IsMultiAccountEnabled() || !observer) {
        return;
    }
    std::lock_guard<std::mutex> lock(observerMutex_);
    for (const auto& existing : observers_) {
        if (existing == observer) {
            MEDIA_INFO_LOG("observer already registered, skip");
            return;
        }
    }
    observers_.push_back(observer);
    MEDIA_INFO_LOG("observer registered, total=%{public}zu", observers_.size());
}

void AccountManager::UnregisterObserver(std::shared_ptr<AccountObserver> observer)
{
    if (!observer) {
        return;
    }
    std::lock_guard<std::mutex> lock(observerMutex_);
    auto it = std::remove(observers_.begin(), observers_.end(), observer);
    if (it != observers_.end()) {
        observers_.erase(it, observers_.end());
        MEDIA_INFO_LOG("observer unregistered, total=%{public}zu", observers_.size());
    }
}

void AccountManager::HwidLoginInternal(int32_t accountId)
{
    if (!AccountEnableChecker::IsMultiAccountEnabled()) {
        return;
    }
    HwidOnStartHandler handler;
    {
        std::lock_guard<std::mutex> lock(handlerMutex_);
        handler = hwidOnStartHandler_;
    }
    if (handler) {
        MEDIA_INFO_LOG("HwidLoginInternal: triggering callback for accountId=%{public}d", accountId);
        handler(accountId);
    }
}

void AccountManager::HwidLogoutInternal(int32_t accountId)
{
    // Notify all registered observers
    std::vector<std::shared_ptr<AccountObserver>> snapshot;
    {
        std::lock_guard<std::mutex> lock(observerMutex_);
        snapshot = observers_;
    }
    for (const auto& observer : snapshot) {
        if (observer) {
            observer->OnAccountLogout(accountId);
        }
    }
    MEDIA_INFO_LOG("HwidLogoutInternal: notified %{public}zu observers for accountId=%{public}d",
        snapshot.size(), accountId);

    if (!AccountEnableChecker::IsMultiAccountEnabled()) {
        return;
    }
    HwidOnStopHandler handler;
    {
        std::lock_guard<std::mutex> lock(handlerMutex_);
        handler = hwidOnStopHandler_;
    }
    if (handler) {
        MEDIA_INFO_LOG("HwidLogoutInternal: triggering callback for accountId=%{public}d", accountId);
        handler(accountId);
    }
}

// --- AccountContext query proxies ---

bool AccountManager::GetForegroundAccountId(int32_t& accountId) const
{
    if (!AccountEnableChecker::IsMultiAccountEnabled()) {
        MEDIA_INFO_LOG("multi-account not enabled, return false");
        return false;
    }
    accountId = accountContext_.GetForegroundAccountId();
    return true;
}

std::unordered_set<int32_t> AccountManager::GetAllAccounts() const
{
    if (!AccountEnableChecker::IsMultiAccountEnabled()) {
        MEDIA_INFO_LOG("multi-account not enabled, return empty");
        return {};
    }
    return accountContext_.GetAllAccounts();
}

void AccountManager::EnsureAccountRootDirsReady()
{
    if (!AccountEnableChecker::IsMultiAccountEnabled()) {
        return;
    }

    const auto& accountIds = GetInstance().GetAllAccounts();
    for (int32_t accountId : accountIds) {
        if (!MediaAccountDirUtils::CheckRootDirsReady(accountId)) {
            MEDIA_ERR_LOG("failed for accountId=%{public}d", accountId);
        }
    }

    if (accountIds.size() == 0) {
        int32_t defaultAccountId = AccountContext::GetDefaultAccountId();
        if (!MediaAccountDirUtils::CheckRootDirsReady(defaultAccountId)) {
            MEDIA_ERR_LOG("failed for default accountId=%{public}d", defaultAccountId);
        }
    }

    MEDIA_INFO_LOG("completed, total=%{public}zu", accountIds.size());
}

}  // namespace Media
}  // namespace OHOS

#endif  // MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

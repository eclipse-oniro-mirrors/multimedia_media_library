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

#define MLOG_TAG "AccountRdbUpgradeMgr"

#include "account_rdb_upgrade_manager.h"

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

#include "account_manager.h"
#include "media_log.h"
#include "medialibrary_errno.h"

namespace OHOS {
namespace Media {

AccountRdbUpgradeManager &AccountRdbUpgradeManager::GetInstance()
{
    static AccountRdbUpgradeManager instance;
    return instance;
}

void AccountRdbUpgradeManager::SetUpgradeCallback(UpgradeCallback callback)
{
    std::lock_guard<std::mutex> lock(rdbUpgradeMutex_);
    upgradeCallback_ = std::move(callback);
}

void AccountRdbUpgradeManager::SetBackupCallback(BackupCallback callback)
{
    std::lock_guard<std::mutex> lock(rdbUpgradeMutex_);
    backupCallback_ = std::move(callback);
}

void AccountRdbUpgradeManager::SetPriorityAccount(int32_t accountId)
{
    priorityAccountId_.store(accountId);
    MEDIA_INFO_LOG("Priority account set: accountId=%{public}d", accountId);
}

void AccountRdbUpgradeManager::HandleUpgrade(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore)
{
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is nullptr!");

    // 1. Upgrade foreground account first (if not already done)
    int32_t foregroundAccountId = 0;
    bool hasForeground = AccountManager::GetInstance().GetForegroundAccountId(foregroundAccountId);
    {
        std::lock_guard<std::mutex> lock(rdbUpgradeMutex_);
        if (hasForeground && TryClaimAccountForUpgrade(foregroundAccountId) && upgradeCallback_) {
            MEDIA_INFO_LOG("Upgrading foreground account: accountId=%{public}d", foregroundAccountId);
            upgradeCallback_(rdbStore);
            MEDIA_INFO_LOG("Foreground account upgrade done, accountId=%{public}d", foregroundAccountId);
            if (backupCallback_) {
                MEDIA_INFO_LOG("Creating slave DB for foreground account: accountId=%{public}d", foregroundAccountId);
                backupCallback_(rdbStore);
            }
        }
    }

    // 2. Upgrade all remaining accounts with priority support
    UpgradeAllAccounts(rdbStore);
}

void AccountRdbUpgradeManager::HandleUpgradeForCurrentAccount(
    const std::shared_ptr<MediaLibraryRdbStore> &rdbStore)
{
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is nullptr!");

    int32_t foregroundAccountId = 0;
    bool hasForeground = AccountManager::GetInstance().GetForegroundAccountId(foregroundAccountId);
    {
        std::lock_guard<std::mutex> lock(rdbUpgradeMutex_);
        if (hasForeground && TryClaimAccountForUpgrade(foregroundAccountId) && upgradeCallback_) {
            MEDIA_INFO_LOG("Login upgrade foreground account: accountId=%{public}d", foregroundAccountId);
            upgradeCallback_(rdbStore);
            MEDIA_INFO_LOG("Login upgrade done, accountId=%{public}d", foregroundAccountId);
            if (backupCallback_) {
                MEDIA_INFO_LOG("Login creating slave DB, accountId=%{public}d", foregroundAccountId);
                backupCallback_(rdbStore);
            }
        }
    }
}

void AccountRdbUpgradeManager::Reset()
{
    // Wait for any in-flight upgrade to complete before resetting
    std::lock_guard<std::mutex> lock(rdbUpgradeMutex_);
    {
        std::lock_guard<std::mutex> upgradedLock(upgradedAccountsMutex_);
        upgradedAccounts_.clear();
    }
    priorityAccountId_.store(INVALID_ACCOUNT_ID);
    upgradeCallback_ = nullptr;
    backupCallback_ = nullptr;
    MEDIA_INFO_LOG("Reset completed");
}

bool AccountRdbUpgradeManager::TryClaimAccountForUpgrade(int32_t accountId)
{
    std::lock_guard<std::mutex> lock(upgradedAccountsMutex_);
    if (upgradedAccounts_.count(accountId) > 0) {
        return false;
    }
    upgradedAccounts_.insert(accountId);
    return true;
}

bool AccountRdbUpgradeManager::IsAccountUpgraded(int32_t accountId)
{
    std::lock_guard<std::mutex> lock(upgradedAccountsMutex_);
    return upgradedAccounts_.count(accountId) > 0;
}

void AccountRdbUpgradeManager::UpgradeSingleAccount(
    const std::shared_ptr<MediaLibraryRdbStore> &rdbStore, int32_t accountId)
{
    AccountScope scope(accountId);
    int32_t ret = rdbStore->Init();
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Init failed, accountId=%{public}d, ret=%{public}d", accountId, ret);
        return;
    }
    if (upgradeCallback_) {
        upgradeCallback_(rdbStore);
    }
    if (backupCallback_) {
        MEDIA_INFO_LOG("Creating slave DB, accountId=%{public}d", accountId);
        backupCallback_(rdbStore);
    }
    // Skip closing the foreground account's database — it must stay open for queries.
    // This prevents a race where Login changes the foreground during the upgrade loop:
    // without this check, Close() would erase the foreground account's store from
    // rdbStoreMap_ and release its db handles, causing all subsequent queries to fail.
    int32_t foregroundAccountId = 0;
    if (AccountManager::GetInstance().GetForegroundAccountId(foregroundAccountId) &&
        accountId == foregroundAccountId) {
        MEDIA_INFO_LOG("Skip close for foreground account: accountId=%{public}d", accountId);
        return;
    }
    rdbStore->Close(1000); // 1s timeout
    MEDIA_INFO_LOG("Upgrade done for accountId=%{public}d", accountId);
}

void AccountRdbUpgradeManager::UpgradeAccountWithMutex(
    const std::shared_ptr<MediaLibraryRdbStore> &rdbStore, int32_t accountId)
{
    if (IsAccountUpgraded(accountId)) {
        return;
    }
    std::lock_guard<std::mutex> lock(rdbUpgradeMutex_);
    if (!TryClaimAccountForUpgrade(accountId) || !upgradeCallback_) {
        return; // Another thread already upgraded, or callback was reset
    }
    MEDIA_INFO_LOG("Upgrading account: accountId=%{public}d", accountId);
    UpgradeSingleAccount(rdbStore, accountId);
}

void AccountRdbUpgradeManager::UpgradeAllAccounts(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore)
{
    auto accountIds = AccountManager::GetInstance().GetAllAccounts();
    MEDIA_INFO_LOG("UpgradeAllAccounts: accountCount=%{public}zu", accountIds.size());
    if (accountIds.empty()) {
        return;
    }

    for (int32_t accountId : accountIds) {
        // Before processing this account, check if a priority account should go first
        int32_t priorityId = priorityAccountId_.load();
        if (priorityId > 0 && priorityId != accountId &&
            accountIds.count(priorityId) > 0 && !IsAccountUpgraded(priorityId)) {
            MEDIA_INFO_LOG("Priority account detected, upgrading first: accountId=%{public}d", priorityId);
            UpgradeAccountWithMutex(rdbStore, priorityId);
        }
        // Upgrade current account
        UpgradeAccountWithMutex(rdbStore, accountId);
    }
    MEDIA_INFO_LOG("UpgradeAllAccounts completed");
}

}  // namespace Media
}  // namespace OHOS

#endif  // MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

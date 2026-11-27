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

#ifndef OHOS_MEDIA_ACCOUNT_RDB_UPGRADE_MANAGER_H
#define OHOS_MEDIA_ACCOUNT_RDB_UPGRADE_MANAGER_H

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_set>

#include "account_scope.h"

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

#include "medialibrary_rdbstore.h"

namespace OHOS {
namespace Media {

/**
 * @brief Manages multi-account RDB database upgrades with priority support.
 *
 * When the process starts, all accounts' databases need upgrading. If the user
 * switches accounts during this process, the newly logged-in account's database
 * should be upgraded first (priority), ahead of accounts still waiting in the queue.
 *
 * Key features:
 * - Deduplication: tracks upgraded accounts to prevent double upgrades from
 *   concurrent startup and login threads.
 * - Serialization: a mutex ensures only one account upgrades at a time, avoiding
 *   I/O contention.
 * - Priority: SetPriorityAccount() marks an account for immediate upgrade; the
 *   upgrade loop checks this before each iteration and processes it first.
 */
class AccountRdbUpgradeManager {
public:
    /**
     * @brief Callback invoked to upgrade a single account's RDB schema.
     * The caller (MediaLibraryDataManager) provides UpgradeRdbInternal as this
     * callback. AccountScope is set by the manager before invoking the callback,
     * so the callback operates on the correct account's store.
     */
    using UpgradeCallback = std::function<void(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore)>;

    /**
     * @brief Callback invoked to create the slave/backup DB for a single account.
     * Called after the upgrade callback completes, while the account's DB is still
     * open (before Close). AccountScope is set by the manager.
     */
    using BackupCallback = std::function<void(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore)>;

    EXPORT static AccountRdbUpgradeManager &GetInstance();

    /**
     * @brief Set the upgrade callback (typically UpgradeRdbInternal).
     */
    void SetUpgradeCallback(UpgradeCallback callback);

    /**
     * @brief Set the backup callback (typically a slave-DB creation function).
     */
    void SetBackupCallback(BackupCallback callback);

    /**
     * @brief Mark an account as priority for upgrade.
     * Called on account login so the upgrade loop processes this account first.
     * @param accountId The account to prioritize
     */
    void SetPriorityAccount(int32_t accountId);

    /**
     * @brief Upgrade the foreground account, then all remaining accounts.
     * The foreground account is upgraded first (if not already done), then
     * all other accounts are upgraded with priority-aware ordering.
     * @param rdbStore The shared RDB store instance
     */
    void HandleUpgrade(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore);

    /**
     * @brief Upgrade and create slave DB only for the current foreground account.
     * Used on account login — does NOT iterate other accounts.
     * @param rdbStore The shared RDB store instance
     */
    void HandleUpgradeForCurrentAccount(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore);

    /**
     * @brief Reset all upgrade state (upgraded accounts, priority, callback).
     * Called during cleanup when all extensions are destroyed.
     * Waits for any in-flight upgrade to complete before resetting.
     */
    void Reset();

private:
    AccountRdbUpgradeManager() = default;
    ~AccountRdbUpgradeManager() = default;
    AccountRdbUpgradeManager(const AccountRdbUpgradeManager &) = delete;
    AccountRdbUpgradeManager &operator=(const AccountRdbUpgradeManager &) = delete;

    bool TryClaimAccountForUpgrade(int32_t accountId);
    bool IsAccountUpgraded(int32_t accountId);
    void UpgradeSingleAccount(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore, int32_t accountId);
    void UpgradeAccountWithMutex(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore, int32_t accountId);
    void UpgradeAllAccounts(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore);

    std::mutex rdbUpgradeMutex_;
    std::mutex upgradedAccountsMutex_;
    std::unordered_set<int32_t> upgradedAccounts_;
    std::atomic<int32_t> priorityAccountId_ {INVALID_ACCOUNT_ID};
    UpgradeCallback upgradeCallback_;
    BackupCallback backupCallback_;
};

}  // namespace Media
}  // namespace OHOS

#endif  // MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

#endif  // OHOS_MEDIA_ACCOUNT_RDB_UPGRADE_MANAGER_H

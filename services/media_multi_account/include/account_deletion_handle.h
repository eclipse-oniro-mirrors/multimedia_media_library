/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_ACCOUNT_DELETION_HANDLE_H
#define OHOS_ACCOUNT_DELETION_HANDLE_H

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "medialibrary_async_worker.h"
#include "os_account_sub_profile_subscribe_callback.h"

namespace OHOS {
namespace Media {

// Sub-profile deletion event callback; declared here, implemented (with logs) in cpp
class AccountDeletionCallback : public AccountSA::OsAccountSubProfileSubscribeCallback {
public:
    AccountDeletionCallback() = default;
    ~AccountDeletionCallback() override = default;
    void OnSubProfileChanged(const AccountSA::SubProfileEventData &eventData) override;
};

// Account deletion handler (singleton):
//   - OnStart: subscribe DELETED events only (SubscribeAccountDeletion)
//   - event callback: enqueue the account into a pending queue, no thread spawned
//   - end of InitMediaLibraryMgr: two-defense-line recovery (RecoverAccountDeletions)
//   - every enqueue submits a drain task to MediaLibraryAsyncWorker; the task
//     claims drain ownership via drainRunning_ (active drain -> no-op re-arm),
//     so at most ONE drain loop runs at any time and a task dropped by
//     AsyncWorker Stop()/Interrupt() (bg queue clear) self-heals on next enqueue
class AccountDeletionHandle {
public:
    enum class AccountDeletionStatus {
        IDLE,
        DELETING,
        COMPLETED,
        FAILED
    };

    static AccountDeletionHandle& GetInstance();
    ~AccountDeletionHandle() = default;
    AccountDeletionHandle(const AccountDeletionHandle&) = delete;
    AccountDeletionHandle& operator=(const AccountDeletionHandle&) = delete;

    // OnStart entry: subscribe DELETED events only, no cleanup is dispatched
    int32_t SubscribeAccountDeletion();

    // End-of-init entry: two-defense-line recovery + start the single drain thread
    int32_t RecoverAccountDeletions();

    // Idempotent enqueue; event callback and recovery both funnel here
    int32_t EnqueueAccountDeletion(int32_t accountId, int32_t userId = -1);

    struct RecoveryResult {
        std::set<int32_t> toClean;
        std::vector<int32_t> toMark;
        std::vector<int32_t> toUnmark;
    };
    static RecoveryResult ComputeRecovery(const std::vector<int32_t> &knownIds,
        const std::vector<int32_t> &deletedIds, const std::vector<int32_t> &existingIds);

private:
    AccountDeletionHandle() = default;

    struct CleanContext {
        CleanContext(int32_t accountId, int32_t userId) : accountId(accountId), userId(userId) {}
        int32_t accountId = -1;
        int32_t userId = -1;
        AccountDeletionStatus status = AccountDeletionStatus::IDLE;
        std::vector<std::string> failedFiles;
        std::vector<std::pair<std::string, int32_t>> failedSteps;
        std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::time_point();
    };

    struct DeletionTask {
        int32_t accountId = -1;
        int32_t userId = -1;
    };

    static int32_t Prepare(CleanContext &ctx);
    static int32_t CleanLocalFiles(CleanContext &ctx);
    static int32_t CleanKvData(CleanContext &ctx);
    static int32_t CleanXmlFiles(CleanContext &ctx);
    static int32_t DeleteXmlFilesWithSuffix(CleanContext &ctx, const std::string &suffix);
    static int32_t CleanRdbData(CleanContext &ctx);
    static int32_t CleanAccountPreferences(CleanContext &ctx);
    static void ReportDeletionMetrics(CleanContext &ctx);

    static int32_t ExecuteDeletionSteps(CleanContext &ctx);
    static int32_t CleanAccountData(CleanContext &ctx);

    int32_t RecoverPendingDeletions();
    static int32_t GetDriverScreenUserId(int32_t &userId);
    void ApplyRecoveryResult(const RecoveryResult &recovery, int32_t userId);

    static void DrainDeletionQueue(AsyncTaskData *data);
    void RunDeletionDrain();
    void ProcessDeletionTask(const DeletionTask &task);

    std::queue<DeletionTask> deletionQueue_;
    std::set<int32_t> deletingAccounts_;
    bool drainRunning_ = false;
    std::mutex queueMutex_;
    std::shared_ptr<AccountDeletionCallback> deletionCallback_;
    std::atomic<bool> processingStarted_ { false };
};

} // namespace Media
} // namespace OHOS

#endif // OHOS_ACCOUNT_DELETION_HANDLE_H

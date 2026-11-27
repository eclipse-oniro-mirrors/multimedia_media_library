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

#define MLOG_TAG "AccountDeletionHandle"

#include "account_deletion_handle.h"

#include <algorithm>
#include <array>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <unordered_set>
#include <utility>
#include <cerrno>
#include <dirent.h>

#include "account_enable_checker.h"
#include "account_manager.h"
#include "dfx_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_path_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_kvstore_manager.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_tracer.h"
#include "os_account_manager.h"
#include "os_account_subprofile_client.h"
#include "preferences.h"
#include "preferences_helper.h"

namespace OHOS {
namespace Media {

void AccountDeletionCallback::OnSubProfileChanged(const AccountSA::SubProfileEventData &eventData)
{
    if (eventData.type_ != AccountSA::OsAccountSubProfileEventType::DELETED) {
        return;
    }
    MEDIA_INFO_LOG("OnSubProfileChanged: DELETED event, osAccountId=%{public}d, subProfileId=%{public}d",
        eventData.osAccountId_, eventData.subProfileId_);
    AccountDeletionHandle::GetInstance().EnqueueAccountDeletion(eventData.subProfileId_, eventData.osAccountId_);
}

namespace {
// RDB base directory - per-account dirs are /data/storage/el2/database/{accountId}/
constexpr const char *RDB_BASE_DIR = "/data/storage/el2/database/";

// Driver-screen user is 100 (product contract)
constexpr int32_t DRIVER_SCREEN_USER_ID = 100;
} // namespace

AccountDeletionHandle& AccountDeletionHandle::GetInstance()
{
    static AccountDeletionHandle instance;
    return instance;
}

// --- Table-driven deletion steps: fatal steps exit early, others aggregate ---
int32_t AccountDeletionHandle::ExecuteDeletionSteps(CleanContext &ctx)
{
    struct StepEntry {
        const char *stepName;
        int32_t (*stepFn)(CleanContext &);
        bool fatal;
    };
    static const std::array<StepEntry, 6> steps = {{
        {"Step 0: Prepare",                 &AccountDeletionHandle::Prepare,                 true},
        {"Step 1: CleanLocalFiles",         &AccountDeletionHandle::CleanLocalFiles,         false},
        {"Step 2: CleanKvData",             &AccountDeletionHandle::CleanKvData,             false},
        {"Step 3: CleanXmlFiles",           &AccountDeletionHandle::CleanXmlFiles,           false},
        {"Step 4: CleanRdbData",           &AccountDeletionHandle::CleanRdbData,           true},
        {"Step 5: CleanAccountPreferences", &AccountDeletionHandle::CleanAccountPreferences, false},
    }};

    bool hasFailure = false;
    for (const auto &step : steps) {
        int32_t err = (*step.stepFn)(ctx);
        MEDIA_INFO_LOG("%{public}s done, accountId=%{public}d, ret=%{public}d",
            step.stepName, ctx.accountId, err);
        if (err != E_OK) {
            ctx.failedSteps.emplace_back(step.stepName, err);
            if (step.fatal) {
                return err;
            }
            hasFailure = true;
        }
    }
    return hasFailure ? E_FAIL : E_OK;
}

// --- CleanAccountData: outer shell (status derive + single-exit reporting) ---
int32_t AccountDeletionHandle::CleanAccountData(CleanContext &ctx)
{
    MediaLibraryTracer tracer;
    tracer.Start("AccountDeletion_Total");
    MEDIA_INFO_LOG("CleanAccountData start, accountId=%{public}d", ctx.accountId);
    ctx.status = AccountDeletionStatus::DELETING;

    int32_t err = ExecuteDeletionSteps(ctx);

    ctx.status = (err == E_OK) ? AccountDeletionStatus::COMPLETED : AccountDeletionStatus::FAILED;
    ReportDeletionMetrics(ctx);
    return err;
}

int32_t AccountDeletionHandle::Prepare(CleanContext &ctx)
{
    MediaLibraryTracer tracer;
    tracer.Start("Step 0: Prepare");
    // Driver-screen user is fixed at 100 (product contract)
    if (ctx.userId != DRIVER_SCREEN_USER_ID) {
        ErrCode ret = OHOS::AccountSA::OsAccountManager::GetForegroundOsAccountLocalId(ctx.userId);
        if (ret != ERR_OK || ctx.userId != DRIVER_SCREEN_USER_ID) {
            MEDIA_ERR_LOG("unexpected user for cleanup, ret=%{public}d, userId=%{public}d (expect %{public}d)",
                ret, ctx.userId, DRIVER_SCREEN_USER_ID);
            return E_FAIL;
        }
    }
    return E_OK;
}

int32_t AccountDeletionHandle::CleanLocalFiles(CleanContext &ctx)
{
    MediaLibraryTracer tracer;
    tracer.Start("Step 1: CleanLocalFiles");
    // Local assets: /storage/media/local/files/<accountId>/
    // Cloud cache:  /storage/cloud/files/<accountId>/
    // Delete the whole per-account subdir (the dir itself plus its contents);
    // the parent files/ (shared bucket) is preserved. Idempotent:
    // DeleteFileOrFolder returns true for non-existent paths.
    const std::vector<std::string> dirs = {
        MediaPathUtils::GetDir(MediaPathType::LOCAL_FILES) + std::to_string(ctx.accountId) + "/",
        MediaPathUtils::GetDir(MediaPathType::CLOUD_FILES) + std::to_string(ctx.accountId) + "/",
    };

    int32_t ret = E_OK;
    for (const auto &dir : dirs) {
        if (!MediaFileUtils::DeleteFileOrFolder(dir, false)) {
            MEDIA_ERR_LOG("DeleteFileOrFolder failed: %{public}s", DfxUtils::GetSafePath(dir).c_str());
            ctx.failedFiles.push_back(dir);
            ret = E_FAIL;
        }
    }
    return ret;
}

int32_t AccountDeletionHandle::CleanKvData(CleanContext &ctx)
{
    MediaLibraryTracer tracer;
    tracer.Start("Step 2: CleanKvData");
    // KV multi-account layout: per-account stores medialibrary_{month|year}_astc_data_<accountId>
    MediaLibraryKvStoreRoute route { true, ctx.accountId };
    MediaLibraryKvStoreManager &kvManager = MediaLibraryKvStoreManager::GetInstance();

    if (kvManager.DeleteAccountKvStores(route) != E_OK) {
        MEDIA_ERR_LOG("DeleteAccountKvStores failed, accountId=%{public}d", ctx.accountId);
        ctx.failedFiles.push_back("astc_stores:" + std::to_string(ctx.accountId));
        return E_FAIL;
    }
    return E_OK;
}

int32_t AccountDeletionHandle::CleanXmlFiles(CleanContext &ctx)
{
    MediaLibraryTracer tracer;
    tracer.Start("Step 3: CleanXmlFiles");
    if (ctx.accountId <= 0) {
        MEDIA_WARN_LOG("invalid accountId=%{public}d, skip CleanXmlFiles", ctx.accountId);
        return E_OK;
    }
    const std::string suffix = "_" + std::to_string(ctx.accountId) + ".xml";
    return DeleteXmlFilesWithSuffix(ctx, suffix);
}

// Delete the per-account preference xml files ({name}_{accountId}.xml) under the prefs dir
int32_t AccountDeletionHandle::DeleteXmlFilesWithSuffix(CleanContext &ctx, const std::string &suffix)
{
    const std::string prefsDir = MediaPathUtils::GetPreferencesDir();
    DIR *dir = opendir(prefsDir.c_str());
    if (dir == nullptr) {
        if (errno == ENOENT || errno == ENOTDIR) {
            MEDIA_WARN_LOG("prefs dir not exists, treat as E_OK, errno=%{public}d", errno);
            return E_OK;
        }
        MEDIA_ERR_LOG("opendir prefs dir: %{public}s failed, errno=%{public}d",
            DfxUtils::GetSafePath(prefsDir).c_str(), errno);
        return E_FAIL;
    }
    int32_t ret = E_OK;
    int32_t deletedCount = 0;
    struct dirent *entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_REG || entry->d_name[0] == '.') {
            continue;
        }
        const std::string fileName(entry->d_name);
        if (fileName.size() <= suffix.size() ||
            fileName.compare(fileName.size() - suffix.size(), suffix.size(), suffix) != 0) {
            continue;
        }
        const std::string filePath = prefsDir + fileName;
        NativePreferences::PreferencesHelper::RemovePreferencesFromCache(filePath);
        int32_t errCode = NativePreferences::PreferencesHelper::DeletePreferences(filePath);
        if (errCode != NativePreferences::E_OK) {
            MEDIA_ERR_LOG("DeletePreferences failed: %{public}s, err=%{public}d",
                DfxUtils::GetSafePath(filePath).c_str(), errCode);
            ctx.failedFiles.push_back(filePath);
            ret = E_FAIL;
        } else {
            deletedCount++;
        }
    }
    closedir(dir);
    return ret;
}

int32_t AccountDeletionHandle::CleanRdbData(CleanContext &ctx)
{
    MediaLibraryTracer tracer;
    tracer.Start("Step 4: CleanRdbData");
    // Multi-db architecture: MediaLibraryRdbStore::rdbStoreMap_ manages db connections per accountId.
    int32_t ret = MediaLibraryRdbStore::CloseAccountDb(ctx.accountId);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CloseAccountDb failed, accountId=%{public}d, ret=%{public}d", ctx.accountId, ret);
        return ret;
    }

    std::string dbDir = std::string(RDB_BASE_DIR) + std::to_string(ctx.accountId);
    if (!MediaFileUtils::DeleteFileOrFolder(dbDir, false)) {
        MEDIA_ERR_LOG("Delete db dir failed: %{public}s", DfxUtils::GetSafePath(dbDir).c_str());
        ctx.failedFiles.push_back(dbDir);
        return E_FAIL;
    }
    return E_OK;
}

int32_t AccountDeletionHandle::CleanAccountPreferences(CleanContext &ctx)
{
    MediaLibraryTracer tracer;
    tracer.Start("Step 5: CleanAccountPreferences");
    // Cleanup targets (AccountManager caches): account info / token / credential
    int32_t ret = AccountManager::GetInstance().Delete(ctx.accountId);
    if (ret == E_ACCOUNT_NOT_FOUND) {
        MEDIA_INFO_LOG("account:%{public}d not in AccountManager, treat as E_OK", ctx.accountId);
        return E_OK;
    }
    return ret;
}

void AccountDeletionHandle::ReportDeletionMetrics(CleanContext &ctx)
{
    int64_t totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - ctx.startTime).count();
    MEDIA_INFO_LOG("Step 6: ReportDeletionMetrics accountId=%{public}d status=%{public}d "
        "failedFiles=%{public}zu failedSteps=%{public}zu totalTimeMs=%{public}lld",
        ctx.accountId, static_cast<int32_t>(ctx.status),
        ctx.failedFiles.size(), ctx.failedSteps.size(), static_cast<long long>(totalMs));
    for (const auto &step : ctx.failedSteps) {
        MEDIA_ERR_LOG("failedStep: %{public}s err=%{public}d", step.first.c_str(), step.second);
    }
}

// --- Queue + single drain task on MediaLibraryAsyncWorker ---

// Idempotent enqueue: set dedup, persist marker, then push + always submit a
// drain task. The drain task claims ownership via drainRunning_ (under
// queueMutex_): if a drain loop is already active the submitted task returns
// immediately, so at most ONE drain loop runs at any time (single-thread
// semantics). Submitting on every enqueue also self-heals a drain task that
// was dropped by MediaLibraryAsyncWorker Stop()/Interrupt() (bg queue clear).
int32_t AccountDeletionHandle::EnqueueAccountDeletion(int32_t accountId, int32_t userId)
{
    MEDIA_INFO_LOG("EnqueueAccountDeletion, accountId=%{public}d, userId=%{public}d", accountId, userId);
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (deletingAccounts_.count(accountId) > 0) {
            MEDIA_INFO_LOG("accountId=%{public}d is already being deleted, idempotent return", accountId);
            return E_OK;
        }
        deletingAccounts_.insert(accountId);
    }
    // best-effort: on failure defense line 2 rediscovers it next start
    AccountManager::GetInstance().MarkAccountDeleted(accountId);

    std::shared_ptr<MediaLibraryAsyncWorker> asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    if (asyncWorker == nullptr) {
        MEDIA_ERR_LOG("AsyncWorker is nullptr, cannot schedule deletion drain");
        std::lock_guard<std::mutex> lock(queueMutex_);
        deletingAccounts_.erase(accountId);
        return E_FAIL;
    }

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        deletionQueue_.push(DeletionTask { accountId, userId });
    }
    std::shared_ptr<MediaLibraryAsyncTask> drainTask = std::make_shared<MediaLibraryAsyncTask>(
        &AccountDeletionHandle::DrainDeletionQueue, nullptr);
    asyncWorker->AddTask(drainTask, false);
    return E_OK;
}

// AsyncWorker task entry: claim the drain ownership or no-op (re-arm)
void AccountDeletionHandle::DrainDeletionQueue(AsyncTaskData *data)
{
    (void)data;
    AccountDeletionHandle &handler = AccountDeletionHandle::GetInstance();
    {
        std::lock_guard<std::mutex> lock(handler.queueMutex_);
        if (handler.drainRunning_) {
            return; // a drain loop is already active - this task is a no-op re-arm
        }
        handler.drainRunning_ = true;
    }
    handler.RunDeletionDrain();
}

// Consumes the queue serially until it is empty. drainRunning_ was set by the
// claiming task and is cleared only here, under the lock, with the queue
// empty - an enqueued task is never lost.
void AccountDeletionHandle::RunDeletionDrain()
{
    MEDIA_INFO_LOG("deletion drain task started");
    while (true) {
        DeletionTask task;
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            if (deletionQueue_.empty()) {
                drainRunning_ = false;
                MEDIA_INFO_LOG("deletion drain done, queue empty");
                return;
            }
            task = deletionQueue_.front();
            deletionQueue_.pop();
        }
        ProcessDeletionTask(task);
    }
}

void AccountDeletionHandle::ProcessDeletionTask(const DeletionTask &task)
{
    CleanContext ctx(task.accountId, task.userId);
    ctx.startTime = std::chrono::steady_clock::now();
    int32_t err = CleanAccountData(ctx);
    if (err == E_OK) {
        AccountManager::GetInstance().UnmarkAccountDeleted(task.accountId);
    } else {
        MEDIA_ERR_LOG("CleanAccountData failed code: %{public}d, accountId: %{public}d",
            err, task.accountId);
    }
    std::lock_guard<std::mutex> lock(queueMutex_);
    deletingAccounts_.erase(task.accountId);
}

// --- End-of-init entry: recovery + drain ---

int32_t AccountDeletionHandle::RecoverAccountDeletions()
{
    // Driver-screen gate: the whole multi-account deletion feature is driver screen only
    if (!AccountEnableChecker::IsMultiAccountEnabled()) {
        MEDIA_INFO_LOG("not on driver screen, multi-account deletion disabled, skip");
        return E_OK;
    }
    bool expected = false;
    if (!processingStarted_.compare_exchange_strong(expected, true)) {
        MEDIA_INFO_LOG("RecoverAccountDeletions already done, skip");
        return E_OK;
    }
    MEDIA_INFO_LOG("RecoverAccountDeletions");
    return RecoverPendingDeletions();
}

// Two defense lines, run synchronously at the end of init:
//   line 2: knownIds - existingIds = deleted sub-profiles (event lost -> toMark + toClean)
//   line 1: deletedIds scan (not existing -> toClean; existing again -> toUnmark)
int32_t AccountDeletionHandle::RecoverPendingDeletions()
{
    int32_t userId = -1;
    int32_t ret = GetDriverScreenUserId(userId);
    if (ret != E_OK) {
        return ret;
    }

    const std::unordered_set<int32_t> accounts = AccountManager::GetInstance().GetAllAccounts();
    const std::vector<int32_t> knownIds(accounts.begin(), accounts.end());
    const std::vector<int32_t> deletedIds = AccountManager::GetInstance().GetDeletedAccountIds();

    // Probe each known sub-profile individually instead of pulling the whole
    // sub-profile list (GetOsAccountSubProfileIds has permission restrictions).
    // E_OK -> still exists; ERR_OS_ACCOUNT_SUBPROFILE_NOT_FOUND -> deleted;
    // any other error -> unknown, abort this round (retry next start).
    std::vector<int32_t> existingIds;
    for (const int32_t accountId : knownIds) {
        int32_t osAccountId = -1;
        ErrCode probeRet = AccountSA::OsAccountSubProfileClient::GetInstance().
            GetOsAccountLocalIdForSubProfile(accountId, osAccountId);
        if (probeRet == ERR_OK) {
            existingIds.push_back(accountId);
        } else if (probeRet != ERR_OS_ACCOUNT_SUBPROFILE_NOT_FOUND) {
            MEDIA_WARN_LOG("probe accountId=%{public}d failed, ret=%{public}d, skip recovery",
                accountId, probeRet);
            return E_FAIL;
        }
    }
    // Fail-safe: an empty existing list with a non-empty known list would mark
    // every known account as deleted and wipe live data if the query misbehaved.
    if (existingIds.empty() && !knownIds.empty()) {
        MEDIA_WARN_LOG("existingIds empty while knownIds=%{public}zu, skip recovery this round",
            knownIds.size());
        return E_FAIL;
    }

    const RecoveryResult recovery = ComputeRecovery(knownIds, deletedIds, existingIds);
    ApplyRecoveryResult(recovery, userId);
    MEDIA_INFO_LOG("recovery completed: known=%{public}zu, existing=%{public}zu, toClean=%{public}zu",
        knownIds.size(), existingIds.size(), recovery.toClean.size());
    return E_OK;
}

int32_t AccountDeletionHandle::GetDriverScreenUserId(int32_t &userId)
{
    userId = -1;
    ErrCode ret = OHOS::AccountSA::OsAccountManager::GetForegroundOsAccountLocalId(userId);
    if (ret != ERR_OK || userId != DRIVER_SCREEN_USER_ID) {
        MEDIA_ERR_LOG("unexpected foreground user, ret=%{public}d, userId=%{public}d (expect %{public}d)",
            ret, userId, DRIVER_SCREEN_USER_ID);
        return E_FAIL;
    }
    return E_OK;
}

void AccountDeletionHandle::ApplyRecoveryResult(const RecoveryResult &recovery, int32_t userId)
{
    for (const int32_t accountId : recovery.toMark) {
        MEDIA_INFO_LOG("account %{public}d deleted but not tracked, marking", accountId);
        // best-effort: on failure line 2 rediscovers it next start
        AccountManager::GetInstance().MarkAccountDeleted(accountId);
    }
    for (const int32_t accountId : recovery.toUnmark) {
        MEDIA_INFO_LOG("account %{public}d exists again, removing stale marker", accountId);
        // best-effort: a stale marker is re-evaluated (and retried) next start
        AccountManager::GetInstance().UnmarkAccountDeleted(accountId);
    }
    for (const int32_t accountId : recovery.toClean) {
        EnqueueAccountDeletion(accountId, userId);
    }
}

// --- ComputeRecovery (class static, unit-testable) ---
AccountDeletionHandle::RecoveryResult AccountDeletionHandle::ComputeRecovery(
    const std::vector<int32_t> &knownIds,
    const std::vector<int32_t> &deletedIds,
    const std::vector<int32_t> &existingIds)
{
    MEDIA_INFO_LOG("ComputeRecovery started");
    RecoveryResult result;

    // Defense line 2: knownIds - existingIds = deleted sub-profiles
    for (const int32_t accountId : knownIds) {
        if (std::find(existingIds.begin(), existingIds.end(), accountId) != existingIds.end()) {
            continue;
        }
        if (std::find(deletedIds.begin(), deletedIds.end(), accountId) == deletedIds.end()) {
            result.toMark.push_back(accountId);
        }
        result.toClean.insert(accountId);
    }

    // Defense line 1: scan deletedIds
    for (const int32_t accountId : deletedIds) {
        if (std::find(existingIds.begin(), existingIds.end(), accountId) != existingIds.end()) {
            result.toUnmark.push_back(accountId);
        } else {
            result.toClean.insert(accountId);
        }
    }

    return result;
}

// --- OnStart entry: subscribe DELETED events only ---
int32_t AccountDeletionHandle::SubscribeAccountDeletion()
{
    if (!AccountEnableChecker::IsMultiAccountEnabled()) {
        MEDIA_INFO_LOG("not on driver screen, multi-account deletion disabled, skip subscribe");
        return E_OK;
    }

    if (deletionCallback_ == nullptr) {
        deletionCallback_ = std::make_shared<AccountDeletionCallback>();
    }

    std::set<AccountSA::OsAccountSubProfileEventType> eventTypes = {
        AccountSA::OsAccountSubProfileEventType::DELETED
    };
    ErrCode ret = AccountSA::OsAccountSubProfileClient::GetInstance().
        SubscribeOsAccountSubProfileEvents(eventTypes, deletionCallback_);
    if (ret != ERR_OK) {
        MEDIA_ERR_LOG("SubscribeOsAccountSubProfileEvents failed, ret=%{public}d", ret);
        return E_FAIL;
    }

    MEDIA_INFO_LOG("SubscribeOsAccountSubProfileEvents succeeded for DELETED event");
    return E_OK;
}

} // namespace Media
} // namespace OHOS

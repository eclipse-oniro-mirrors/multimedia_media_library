/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MediaLibraryRestore"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <utility>
#include "acl.h"
#include "medialibrary_restore.h"
#include "dfx_utils.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_tracer.h"
#include "medialibrary_unistore_manager.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "parameter.h"
#include "parameters.h"
#include "post_event_utils.h"
#include "medialibrary_notify_new.h"
#include "medialibrary_rdbstore.h"
#include "photo_album_column.h"
#include "recovery_summary.h"
#include "recovery_window.h"
#include "replica_recovery_policy.h"
#ifdef META_RECOVERY_SUPPORT
#include "medialibrary_meta_recovery.h"
#include "medialibrary_subscriber.h"
#endif
#ifdef CLOUD_SYNC_MANAGER
#include "cloud_sync_manager.h"
#endif

namespace OHOS {
namespace Media {
namespace {
    const std::string SWITCH_STATUS_KEY = "persist.multimedia.medialibrary.rdb_switch_status";
    const std::string CLOUD_STOP_FLAG_K = "persist.kernel.medialibrarydata.stopflag";
    const std::string CLOUD_STOP_FLAG_V = "1";
    constexpr int PARAMETER_E_OK        = 0;
    constexpr int WAIT_SECONDS          = 30;
    constexpr int SLEEP_SECONDS         = 60;
    enum HA_SWITCH_STATUS : uint32_t {
        HA_SWITCH_READY = 0,
        HA_SWITCHING = 1
    };
} // namespace

MediaLibraryRestore &MediaLibraryRestore::GetInstance()
{
    static MediaLibraryRestore instance;
    return instance;
}

void MediaLibraryRestore::SaveHAModeSwitchStatusToPara(const int64_t &status)
{
    int ret = SetParameter(SWITCH_STATUS_KEY.c_str(), std::to_string(status).c_str());
    CHECK_AND_RETURN_LOG((ret == PARAMETER_E_OK), "MediaLibraryRestore SetParameter switch error");
}

static int64_t RecoveryElapsedMs(const std::chrono::steady_clock::time_point &start)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
}

static int64_t RecoveryTableCount(const NativeRdb::DbInfo &info, const std::string &name)
{
    const auto item = info.tables.find(name);
    return item == info.tables.end() ? -1 : item->second.count;
}

static void CaptureDiagnosisInfo(const NativeRdb::PrimaryAndReplicaInfo &info)
{
    MEDIA_INFO_LOG("Recovery diagnosis sizes=%{public}lld/%{public}lld versions=%{public}lld/%{public}lld",
        static_cast<long long>(info.primaryInfo.dbInfo.size), static_cast<long long>(info.replicaInfo.dbInfo.size),
        static_cast<long long>(info.primaryInfo.userVersion), static_cast<long long>(info.replicaInfo.userVersion));
    MEDIA_INFO_LOG("Recovery diagnosis Photos=%{public}lld/%{public}lld PhotoAlbum=%{public}lld/%{public}lld",
        static_cast<long long>(RecoveryTableCount(info.primaryInfo, PhotoColumn::PHOTOS_TABLE)),
        static_cast<long long>(RecoveryTableCount(info.replicaInfo, PhotoColumn::PHOTOS_TABLE)),
        static_cast<long long>(RecoveryTableCount(info.primaryInfo, PhotoAlbumColumns::TABLE)),
        static_cast<long long>(RecoveryTableCount(info.replicaInfo, PhotoAlbumColumns::TABLE)));
    MEDIA_INFO_LOG("Recovery diagnosis primaryTime=%{public}lld replicaTime=%{public}lld",
        static_cast<long long>(info.GetPrimaryLastModifyTime()),
        static_cast<long long>(info.GetReplicaLastModifyTime()));
    MEDIA_INFO_LOG("Recovery diagnosis primaryDbMtime=%{public}lld primaryWalMtime=%{public}lld "
        "replicaDbMtime=%{public}lld binlogTime=%{public}lld binlogExists=%{public}d",
        static_cast<long long>(info.primaryInfo.dbInfo.mtime), static_cast<long long>(info.primaryInfo.walInfo.mtime),
        static_cast<long long>(info.replicaInfo.dbInfo.mtime), static_cast<long long>(info.binlogInfo.lastModifyTime),
        info.binlogInfo.exist ? 1 : 0);
}

static ReplicaRecoveryDecision RecordDiagnosis(int32_t error, const NativeRdb::PrimaryAndReplicaInfo &info,
    const std::chrono::steady_clock::time_point &start, DatabaseRecoveryReport &report)
{
    const auto decision = CheckReplicaRecovery(error, info, MEDIA_RDB_VERSION);
    report.error = error;
    report.decision = static_cast<int32_t>(decision);
    report.result = error != NativeRdb::E_OK ? DatabaseRecoveryResult::FAILED :
        (decision == ReplicaRecoveryDecision::ALLOW ? DatabaseRecoveryResult::SUCCESS :
        DatabaseRecoveryResult::REJECTED);
    const int64_t duration = RecoveryElapsedMs(start);
    if (error == NativeRdb::E_OK) {
        CaptureDiagnosisInfo(info);
    }
    MEDIA_INFO_LOG("Recovery diagnosis result=%{public}d decision=%{public}d duration=%{public}lld",
        error, report.decision, static_cast<long long>(duration));
    return decision;
}

static RecoveryWindowResult RegisterRecoveryTask(DatabaseRecoveryReport &report,
    int64_t replicaTime, std::string &taskPath)
{
    report.stage = DatabaseRecoveryStage::WINDOW_REGISTRATION;
    const int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    CHECK_AND_PRINT_LOG(MediaFileUtils::CreateDirectory(RECOVERY_WINDOW_DIRECTORY),
        "Could not prepare recovery task directory");
    auto window = RegisterRecoveryWindowFile(RECOVERY_WINDOW_DIRECTORY, replicaTime, endTime);
    MEDIA_INFO_LOG("Recovery window start=%{public}lld end=%{public}lld errno=%{public}d merged=%{public}d "
        "expired=%{public}d", static_cast<long long>(window.startMs > 0 ? window.startMs : replicaTime),
        static_cast<long long>(endTime), window.error, window.merged, window.expired);
    if (window.error != 0) {
        report.error = window.error;
        report.result = DatabaseRecoveryResult::FAILED;
        return window;
    }
    taskPath = window.path;
    DatabaseRecoveryReport previous;
    if (LoadRecoverySummary(taskPath, previous) == 0) {
        const std::string date = report.date;
        report = std::move(previous);
        if (report.date.empty()) {
            report.date = date;
        }
    }
    report.stage = DatabaseRecoveryStage::WINDOW_REGISTRATION;
    report.result = DatabaseRecoveryResult::SUCCESS;
    report.error = 0;
    report.decision = static_cast<int32_t>(ReplicaRecoveryDecision::ALLOW);
    report.windowStart = window.startMs;
    report.windowEnd = endTime;
    // The Force result is not known until the call returns.
    report.waitForRestore = false;
    report.jsonBucketsDone.clear();
    report.fileBucketsDone.clear();
    int saveError = SaveRecoverySummary(taskPath, report);
    if (saveError != 0) {
        MEDIA_ERR_LOG("Could not save recovery progress before ForceRestore, errno=%{public}d", saveError);
        (void)ForgetRecoverySummary(taskPath);
    }
    return window;
}

static int32_t ExecuteForceRestore(const std::shared_ptr<NativeRdb::RdbStore> &store,
    DatabaseRecoveryReport &report, const std::string &taskPath)
{
    report.stage = DatabaseRecoveryStage::FORCE_RESTORE;
    int32_t ret = NativeRdb::E_CANCEL;
    if (MediaLibraryRdbStore::GetRaw() == store) {
        auto begin = std::chrono::steady_clock::now();
        ret = store->ForceRestore("");
        RecordForceAttempt(report, ret, RecoveryElapsedMs(begin));
    } else {
        report.waitForRestore = true;
    }
    report.error = ret;
    report.result = ret == NativeRdb::E_OK ? DatabaseRecoveryResult::SUCCESS : DatabaseRecoveryResult::FAILED;
    MEDIA_INFO_LOG("Force attempt=%{public}d failures=%{public}d result=%{public}d elapsed=%{public}lld",
        report.forceAttempts, report.forceFailures, ret, static_cast<long long>(report.forceMs));
    if (report.forceAttempts > 1) {
        MEDIA_WARN_LOG("Recovery required multiple Force attempts, last error=%{public}d", report.lastForceError);
    }
    int saveError = SaveRecoverySummary(taskPath, report);
    if (saveError != 0) {
        MEDIA_ERR_LOG("Could not save ForceRestore result, errno=%{public}d", saveError);
        (void)ForgetRecoverySummary(taskPath);
    }
    return ret;
}

static int32_t RunForceRestore(const std::shared_ptr<NativeRdb::RdbStore> &store, DatabaseRecoveryReport &report,
    std::string &taskPath)
{
    report.stage = DatabaseRecoveryStage::DIAGNOSIS;
    auto start = std::chrono::steady_clock::now();
    auto [error, info] = store->GetPrimaryReplicaInfo({PhotoColumn::PHOTOS_TABLE, PhotoAlbumColumns::TABLE});
    const auto decision = RecordDiagnosis(error, info, start, report);
    const int64_t replicaTime = decision == ReplicaRecoveryDecision::ALLOW ? info.GetReplicaLastModifyTime() : 0;
    const auto registerWindow = [&report, &taskPath, replicaTime]() {
        return RegisterRecoveryTask(report, replicaTime, taskPath);
    };
    const auto forceRestore = [&store, &report, &taskPath]() { return ExecuteForceRestore(store, report, taskPath); };
    auto result = RunForceRecovery(decision == ReplicaRecoveryDecision::ALLOW, registerWindow, forceRestore);
    if (!result.forceCalled) {
        MEDIA_ERR_LOG("ForceRestore skipped: diagnosis=%{public}d, window errno=%{public}d",
            static_cast<int32_t>(decision), result.window.error);
        return NativeRdb::E_SQLITE_CORRUPT;
    }
    MEDIA_INFO_LOG("ForceRestore result=%{public}d", result.forceError);
    return result.forceError;
}

static int32_t RunOrdinaryRestore(const std::shared_ptr<NativeRdb::RdbStore> &store, DatabaseRecoveryReport &report)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRestore::Restore");
    Notification::MediaLibraryNotifyNew::AddDbAvailabilityItem("unavailable", "Database corrupted");
    // Bypass the generic SQL retry wrapper: recovery is attempted only once.
    const int32_t errCode = store->Restore("");
    report.stage = DatabaseRecoveryStage::ORDINARY_RESTORE;
    report.error = errCode;
    report.result = errCode == NativeRdb::E_OK ? DatabaseRecoveryResult::SUCCESS : DatabaseRecoveryResult::FAILED;
    MEDIA_INFO_LOG("MediaLibraryRestore::Restore errCode = %{public}d", errCode);
    return errCode;
}

static int32_t DiagnoseAfterRestore(int32_t error, const std::shared_ptr<NativeRdb::RdbStore> &store,
    DatabaseRecoveryReport &report, std::string &taskPath)
{
    if (error != NativeRdb::E_SQLITE_CORRUPT) {
        return error;
    }
    if (MediaLibraryRdbStore::GetRaw() == store) {
        return RunForceRestore(store, report, taskPath);
    }
    MEDIA_ERR_LOG("ForceRestore skipped: database store missing or changed");
    report.stage = DatabaseRecoveryStage::DIAGNOSIS;
    report.error = NativeRdb::E_CANCEL;
    report.result = DatabaseRecoveryResult::FAILED;
    report.decision = static_cast<int32_t>(ReplicaRecoveryDecision::DIAGNOSIS_UNAVAILABLE);
    return NativeRdb::E_CANCEL;
}

static void FinishDatabaseRecovery(int32_t error, const std::shared_ptr<NativeRdb::RdbStore> &store,
    const std::string &taskPath, DatabaseRecoveryReport &report, std::atomic<bool> &restoring)
{
    if (error == NativeRdb::E_OK) {
        report.stage = DatabaseRecoveryStage::DATABASE_USABLE;
        if (MediaLibraryRdbStore::GetRaw() == store) {
            Notification::MediaLibraryNotifyNew::AddDbAvailabilityItem("available", "");
            report.error = NativeRdb::E_OK;
            report.result = DatabaseRecoveryResult::SUCCESS;
        } else {
            MEDIA_ERR_LOG("Recovery succeeded, but database store is missing or changed");
            report.error = NativeRdb::E_CANCEL;
            report.result = DatabaseRecoveryResult::FAILED;
        }
    }
    int updated = 0;
    if (error == NativeRdb::E_OK && report.error == NativeRdb::E_OK) {
        int markError = PrepareRecoveryWindowsAfterRestore(RECOVERY_WINDOW_DIRECTORY, updated);
        CHECK_AND_PRINT_LOG(markError == 0 || markError == ENOENT,
            "Could not prepare pending recovery after restore, errno=%{public}d", markError);
    }
    const bool ordinaryRestoreFailed =
        report.stage == DatabaseRecoveryStage::ORDINARY_RESTORE && report.error != NativeRdb::E_OK;
    if (!ordinaryRestoreFailed && taskPath.empty() && updated == 0 &&
        (report.result != DatabaseRecoveryResult::SUCCESS ||
        !HasPendingRecoveryWindow(RECOVERY_WINDOW_DIRECTORY))) {
        PostEventUtils::GetInstance().PostDatabaseRecovery(report);
    }
    MEDIA_INFO_LOG("MediaLibraryRestore::Restore [end]. errCode = %{public}d", error);
    restoring = false;
#ifdef META_RECOVERY_SUPPORT
    if (error == NativeRdb::E_OK && MediaLibraryRdbStore::GetRaw() == store &&
        MedialibrarySubscriber::IsCurrentStatusOn() &&
        HasPendingRecoveryWindow(RECOVERY_WINDOW_DIRECTORY)) {
        MediaLibraryMetaRecovery::GetInstance().CheckRecoveryState();
    }
#endif
}

static void RunRecoveryThread(const std::string &date, std::atomic<bool> &restoring)
{
    MEDIA_INFO_LOG("MediaLibraryRestore::Restore [start]");
    DatabaseRecoveryReport report;
    report.date = date;
    std::string taskPath;
    auto rdb = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    auto rawStore = MediaLibraryRdbStore::GetRaw();
    if (rdb == nullptr || rawStore == nullptr) {
        report.stage = DatabaseRecoveryStage::ORDINARY_RESTORE;
        report.error = NativeRdb::E_ERROR;
        report.result = DatabaseRecoveryResult::FAILED;
        PostEventUtils::GetInstance().PostDatabaseRecovery(report);
        restoring = false;
        MEDIA_ERR_LOG("Restore rdbStore or rawStore is nullptr");
        return;
    }
    int32_t error = RunOrdinaryRestore(rawStore, report);
    error = DiagnoseAfterRestore(error, rawStore, report, taskPath);
    FinishDatabaseRecovery(error, rawStore, taskPath, report, restoring);
}

void MediaLibraryRestore::CheckRestore(const int32_t &errCode)
{
    MEDIA_DEBUG_LOG("CheckRestore is called, errcode=%{public}d", errCode);
    if (errCode != NativeRdb::E_SQLITE_CORRUPT) {
        return;
    }
    MEDIA_INFO_LOG("Restore is called");
    CHECK_AND_RETURN_LOG(TryBeginDatabaseRecovery(isRestoring_), "RdbStore is restoring");
#ifdef META_RECOVERY_SUPPORT
    MediaLibraryMetaRecovery::GetInstance().InterruptIncrementalRecovery();
#endif
    std::string date = DfxUtils::GetCurrentDateMillisecond();
    VariantMap map = {{KEY_DB_CORRUPT, date}};
    PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_CORRUPT_ERR, map);
    std::thread([this, date] { RunRecoveryThread(date, isRestoring_); }).detach();
}

#ifdef CLOUD_SYNC_MANAGER
void MediaLibraryRestore::StopCloudSync()
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRestore::StopCloudSync");
    int32_t ret = FileManagement::CloudSync::CloudSyncManager::GetInstance().StopSync(CONST_BUNDLE_NAME, true);
    CHECK_AND_RETURN_LOG(ret == PARAMETER_E_OK, "StopCloudSync fail, errcode = %{public}d", ret);
    std::this_thread::sleep_for(std::chrono::seconds(WAIT_SECONDS));
    MEDIA_INFO_LOG("StopCloudSync wait done, start backup");
}
#endif

void MediaLibraryRestore::CheckBackup()
{
    MEDIA_INFO_LOG("CheckBackup is called");
    CHECK_AND_RETURN_LOG((!isRestoring_), "CheckBackup: is restoring, return");
    auto rdb = MediaLibraryDataManager::GetInstance()->rdbStore_;
    CHECK_AND_RETURN_LOG((rdb != nullptr), "CheckBackup: rdbStore is nullptr");
    if (!rdb->IsSlaveDiffFromMaster()) {
        MEDIA_INFO_LOG("CheckBackup: isSlaveDiffFromMaster [false], return");
        return;
    }
    MEDIA_INFO_LOG("CheckBackup: isSlaveDiffFromMaster [true]");
    DoRdbBackup();
}

void MediaLibraryRestore::ResetHAModeSwitchStatus()
{
    auto switchStatus = HA_SWITCH_READY;
    SaveHAModeSwitchStatusToPara(std::move(switchStatus));
#ifdef CLOUD_SYNC_MANAGER
    auto ret = FileManagement::CloudSync::CloudSyncManager::GetInstance().StartSync(CONST_BUNDLE_NAME);
    MEDIA_INFO_LOG("ResetHAModeSwitchStatus::StartSync [%{public}d]", ret);
    ret = FileManagement::CloudSync::CloudSyncManager::GetInstance().DownloadThumb();
    MEDIA_INFO_LOG("ResetHAModeSwitchStatus::DownloadThumb [%{public}d]", ret);
#endif
}

void MediaLibraryRestore::DoRdbBackup()
{
    CHECK_AND_RETURN_LOG((!isBackuping_.load()), "DoRdbBackup: is backuping, return");
    isBackuping_ = true;

    std::thread([] {
        MEDIA_INFO_LOG("DoRdbBackup: Backup [start]");
        {
            MEDIA_INFO_LOG("DoRdbBackup: wait_for [start]");
            std::unique_lock<std::mutex> lock(MediaLibraryRestore::GetInstance().mutex_);
            MediaLibraryRestore::GetInstance().cv_.wait_for(lock, std::chrono::seconds(SLEEP_SECONDS),
                [] { return !MediaLibraryRestore::GetInstance().IsBackuping(); });
            MEDIA_INFO_LOG("DoRdbBackup: wait_for [end]");
        }
        CHECK_AND_RETURN_LOG((MediaLibraryRestore::GetInstance().IsBackuping()),
            "DoRdbBackup: after sleep isbackuping fasle, return");
        auto currentTime = MediaFileUtils::UTCTimeSeconds();
        MediaLibraryRestore::GetInstance().SaveHAModeSwitchStatusToPara(currentTime);
#ifdef CLOUD_SYNC_MANAGER
        MediaLibraryRestore::GetInstance().StopCloudSync();
#endif
        auto rdb = MediaLibraryDataManager::GetInstance()->rdbStore_;
        if (rdb == nullptr || !MediaLibraryRestore::GetInstance().IsBackuping()) {
            MediaLibraryRestore::GetInstance().ResetHAModeSwitchStatus();
            MediaLibraryRestore::GetInstance().isBackuping_ = false;
            MEDIA_ERR_LOG("DoRdbBackup: rdb is nullptr or interrupt or isbackuping false");
            return;
        }
        MediaLibraryTracer tracer;
        tracer.Start("MediaLibraryRestore::DoRdbBackup Backup");
        MediaLibraryRestore::GetInstance().isDoingBackup_ = true;
        if (!MediaLibraryRdbUtils::ExecuteDatabaseQuickCheck(rdb)
            || !MediaLibraryRestore::GetInstance().IsBackuping()) {
            MediaLibraryRestore::GetInstance().ResetHAModeSwitchStatus();
            MediaLibraryRestore::GetInstance().isBackuping_ = false;
            MEDIA_ERR_LOG("DoRdbBackup: QuickCheck fail");
            return;
        }
        MEDIA_INFO_LOG("DoRdbBackup: Backup [start]");
        int errCode = rdb->Backup("", false);
        MediaLibraryRestore::GetInstance().isDoingBackup_ = false;
        if (errCode == NativeRdb::E_OK) {
            Acl::AclSetSlaveDatabase();
        }
        MEDIA_INFO_LOG("DoRdbBackup: Backup [end]. errCode = %{public}d", errCode);
        MediaLibraryRestore::GetInstance().ResetHAModeSwitchStatus();
        if (errCode != NativeRdb::E_CANCEL) {
            MediaLibraryRestore::GetInstance().isBackuping_ = false;
        }
    }).detach();
}

void MediaLibraryRestore::InterruptBackup()
{
    MEDIA_INFO_LOG("InterruptBackup [start]");
    if (!isBackuping_.load()) {
        MEDIA_INFO_LOG("rdb is not backuping, no need to interrupt");
        return;
    }
    auto rdb = MediaLibraryDataManager::GetInstance()->rdbStore_;
    CHECK_AND_RETURN_LOG((rdb != nullptr), "[InterruptBackup] rdbStore is nullptr");
    int errCode = rdb->InterruptBackup();
    isBackuping_ = false;
    ResetHAModeSwitchStatus();
    cv_.notify_all();
    MEDIA_INFO_LOG("InterruptBackup [end]. errCode = %{public}d", errCode);
}

void MediaLibraryRestore::CheckResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    MediaLibraryTracer tracer;
    tracer.Start("CheckResultSet");
    if (resultSet == nullptr) {
        return;
    }
    int count;
    int errCode = resultSet->GetRowCount(count);
    CheckRestore(errCode);
}

bool MediaLibraryRestore::IsRestoring() const
{
    return isRestoring_;
}

bool MediaLibraryRestore::IsBackuping() const
{
    return isBackuping_.load();
}

bool MediaLibraryRestore::IsRealBackuping() const
{
    return isDoingBackup_;
}
} // namespace Media
} // namespace OHOS

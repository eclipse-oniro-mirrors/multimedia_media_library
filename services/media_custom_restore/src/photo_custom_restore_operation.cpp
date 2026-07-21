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
#define MLOG_TAG "PhotoCustomRestoreOperation"

#include "photo_custom_restore_operation.h"

#include <dirent.h>
#include <dlfcn.h>
#include <thread>

#include "asset_compress_version_manager.h"
#include "custom_restore_const.h"
#include "dfx_reporter.h"
#include "dfx_utils.h"
#include "directory_ex.h"
#include "ffrt.h"
#include "ffrt_inner.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_file_uri.h"
#include "media_log.h"
#include "media_unique_number_column.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_json_operation.h"
#include "medialibrary_notify.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_type_const.h"
#include "medialibrary_db_const.h"
#include "metadata_extractor.h"
#include "mimetype_utils.h"
#include "moving_photo_file_utils.h"
#include "photo_album_column.h"
#include "photo_file_utils.h"
#include "result_set_utils.h"
#include "scanner_utils.h"
#include "shooting_mode_column.h"
#include "userfile_manager_types.h"
#include "refresh_business_name.h"
#include "media_edit_utils.h"
#include "media_share_dirty_data_cleaner.h"
#include "media_time_utils.h"
#include "batch_restore_utils.h"
#include "scan_config.h"
#include "scan_config_builder.h"
#include "media_scanner_manager.h"
// LCOV_EXCL_START
using namespace std;
namespace OHOS::Media {
std::shared_ptr<PhotoCustomRestoreOperation> PhotoCustomRestoreOperation::instance_ = nullptr;
std::mutex PhotoCustomRestoreOperation::objMutex_;
static const int32_t INVALID_DURATION = -1;
const int EARLY_NOTIFY_DELAY_MS = 50;

PhotoCustomRestoreOperation &PhotoCustomRestoreOperation::GetInstance()
{
    std::lock_guard<std::mutex> lock(PhotoCustomRestoreOperation::objMutex_);
    if (PhotoCustomRestoreOperation::instance_ == nullptr) {
        PhotoCustomRestoreOperation::instance_ = std::make_shared<PhotoCustomRestoreOperation>();
    }
    return *PhotoCustomRestoreOperation::instance_;
}

PhotoCustomRestoreOperation::~PhotoCustomRestoreOperation()
{
    MEDIA_INFO_LOG("[ASYNC] Destructor called. stopping worker");

    stopWorker_.store(true);
    queueCv_.notify_one();

    if (workerThread_.joinable()) {
        MEDIA_INFO_LOG("[ASYNC] Destructor: joining worker thread");
        workerThread_.join();
    }

    MEDIA_INFO_LOG("[ASYNC] Destructor: worker thread joined. cleanup complete");
}

PhotoCustomRestoreOperation &PhotoCustomRestoreOperation::Start()
{
    if (this->isRunning_.exchange(true)) {
        MEDIA_WARN_LOG("custom restore operation is running. skip start");
        return *this;
    }
    CleanTimeoutCustomRestoreTaskDir();
    while (!this->taskQueue_.empty()) {
        RestoreTaskInfo restoreTaskInfo = this->taskQueue_.front();
        if (IsCancelTask(restoreTaskInfo)) {
            CancelTaskFinish(restoreTaskInfo);
            this->taskQueue_.pop();
            continue;
        }
        DoCustomRestore(restoreTaskInfo);
        this->taskQueue_.pop();
    }
    {
        std::unique_lock<std::shared_mutex> lockGuard(cancelOprationLock_);
        cancelKeySet_.clear();
    }
    this->isRunning_.store(false);
    return *this;
}

PhotoCustomRestoreOperation &PhotoCustomRestoreOperation::StartAsync()
{
    MEDIA_INFO_LOG("[ASYNC] - StartAsync - workerRunning: %{public}d", workerRunning_.load());
    std::lock_guard<std::mutex> lock(queueMutex_);
    if (workerRunning_.load()) {
        MEDIA_INFO_LOG("[ASYNC] - StartAsync - worker already running, skip. queueSize: %{public}zu",
            this->taskQueue_.size());
        return *this;
    }

    workerRunning_.store(true);
    stopWorker_.store(false);

    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    workerThread_ = std::thread([this]() {WorkerLoop(); });
    MEDIA_INFO_LOG("[ASYNC] - StartAsync - worker thread started. queueSize: %{public}zu",
        this->taskQueue_.size());
    return *this;
}

void PhotoCustomRestoreOperation::WorkerLoop()
{
    MEDIA_INFO_LOG("[ASYNC] WorkerLoop started. thread entering main loop");
    CleanTimeoutCustomRestoreTaskDir();
    int32_t taskCount = 0;
    bool shouldContinue = true;

    while (shouldContinue) {
        std::unique_lock<std::mutex> lock(queueMutex_);
        MEDIA_INFO_LOG("[ASYNC] WorkerLoop: waiting for task. queueSize: %{public}zu, "
            "tasksProcessed: %{public}d", this->taskQueue_.size(), taskCount);

        queueCv_.wait(lock, [this]() {
            return !this->taskQueue_.empty() || stopWorker_.load();
        });

        if (stopWorker_.load() && this->taskQueue_.empty()) {
            MEDIA_INFO_LOG("[ASYNC] WorkerLoop: stop signal received, exiting. "
                "tasksProcessed: %{public}d", taskCount);
            shouldContinue = false;
            break;
        }

        RestoreTaskInfo restoreTaskInfo = this->taskQueue_.front();
        this->taskQueue_.pop();
        size_t remainingTasks = this->taskQueue_.size();
        isProcessingTask_.store(true);
        lock.unlock();

        taskCount++;
        MEDIA_INFO_LOG("[ASYNC] WorkerLoop: dequeued task #%{public}d. "
            "keyPath: %{public}s, remainingInQueue: %{public}zu",
            taskCount, restoreTaskInfo.keyPath.c_str(), remainingTasks);

        if (IsCancelTask(restoreTaskInfo)) {
            MEDIA_INFO_LOG("[ASYNC] WorkerLoop: task cancelled. keyPath: %{public}s",
                restoreTaskInfo.keyPath.c_str());
            CancelTaskFinish(restoreTaskInfo);
            isProcessingTask_.store(false);
            continue;
        }

        MEDIA_INFO_LOG("[ASYNC] WorkerLoop: DoCustomRestore starting. keyPath: %{public}s",
            restoreTaskInfo.keyPath.c_str());

        DoCustomRestore(restoreTaskInfo);
        isProcessingTask_.store(false);

        MEDIA_INFO_LOG("[ASYNC] WorkerLoop: DoCustomRestore finished. keyPath: %{public}s",
            restoreTaskInfo.keyPath.c_str());
    }

    {
        std::unique_lock<std::shared_mutex> lockGuard(cancelOprationLock_);
        cancelKeySet_.clear();
    }

    workerRunning_.store(false);
    MEDIA_INFO_LOG("[ASYNC] WorkerLoop finished. totalTasksProcessed: %{public}d", taskCount);
}

void PhotoCustomRestoreOperation::CancelTask(RestoreTaskInfo restoreTaskInfo)
{
    MEDIA_INFO_LOG("cancel custom restore task. keyPath: %{public}s", restoreTaskInfo.keyPath.c_str());
    std::unique_lock<std::shared_mutex> lockGuard(cancelOprationLock_);
    cancelKeySet_.insert(restoreTaskInfo.keyPath);
}

bool PhotoCustomRestoreOperation::IsCancelTask(RestoreTaskInfo &restoreTaskInfo)
{
    std::shared_lock<std::shared_mutex> lockGuard(cancelOprationLock_);
    return cancelKeySet_.count(restoreTaskInfo.keyPath) > 0;
}

void PhotoCustomRestoreOperation::CancelTaskFinish(RestoreTaskInfo &restoreTaskInfo)
{
    UniqueNumber uniqueNumber;
    SendNotifyMessage(restoreTaskInfo, NOTIFY_CANCEL, E_OK, 0, uniqueNumber);
    std::unique_lock<std::shared_mutex> lockGuard(cancelOprationLock_);
    cancelKeySet_.erase(restoreTaskInfo.keyPath);
}

void PhotoCustomRestoreOperation::ApplyEfficiencyQuota(int32_t fileNum)
{
#ifdef EFFICIENCY_MANAGER_ENABLE
    int64_t quota = fileNum * BASE_EFFICIENCY_QUOTA / BASE_FILE_NUM;
    if (quota == 0) {
        MEDIA_WARN_LOG("quota is zero, skip apply efficiency quota.");
        return;
    }
    string module = CONST_BUNDLE_NAME;
    string reason = "Custom Restore";
    MEDIA_DEBUG_LOG("ApplyEfficiencyQuota. quota: %{public}" PRId64, quota);
    void *resourceQuotaMgrHandle = dlopen(ABNORMAL_MANAGER_LIB.c_str(), RTLD_NOW);
    CHECK_AND_RETURN_LOG(resourceQuotaMgrHandle, "Not find resource_quota_manager lib.");

    using HandleQuotaFunc = bool (*)(const std::string &, uint32_t, int64_t, const std::string &);
    auto handleQuotaFunc =
        reinterpret_cast<HandleQuotaFunc>(dlsym(resourceQuotaMgrHandle, "ApplyAbnormalControlQuota"));
    if (!handleQuotaFunc) {
        MEDIA_ERR_LOG("Not find ApplyAbnormalControlQuota func.");
        dlclose(resourceQuotaMgrHandle);
        return;
    }
    CHECK_AND_PRINT_LOG(handleQuotaFunc(module, MODULE_POWER_OVERUSED, quota, reason), "Do handleQuotaFunc failed.");
    dlclose(resourceQuotaMgrHandle);
#endif
}

PhotoCustomRestoreOperation &PhotoCustomRestoreOperation::AddTask(RestoreTaskInfo restoreTaskInfo)
{
    MEDIA_INFO_LOG("add custom restore task. keyPath: %{public}s", restoreTaskInfo.keyPath.c_str());
    restoreTaskInfo.uriType = RESTORE_URI_TYPE_PHOTO;
    this->taskQueue_.push(restoreTaskInfo);
    return *this;
}

PhotoCustomRestoreOperation &PhotoCustomRestoreOperation::AddTaskAsync(RestoreTaskInfo restoreTaskInfo)
{
    MEDIA_INFO_LOG("add async restore task. keyPath: %{public}s", restoreTaskInfo.keyPath.c_str());
    std::unique_lock<std::mutex> lock(queueMutex_);
    
    if (this->taskQueue_.empty() && !isProcessingTask_.load()) {
        restoreTaskInfo.uriType = RESTORE_URI_TYPE_PHOTO;
    } else {
        restoreTaskInfo.uriType = RESTORE_URI_TYPE_EMPTY;
        restoreTaskInfo.earlyNotifySent = true;
    }

    MEDIA_INFO_LOG("AddTaskAsync uriType: %{public}d", restoreTaskInfo.uriType);

    this->taskQueue_.push(restoreTaskInfo);
    lock.unlock();
    queueCv_.notify_one();

    if (restoreTaskInfo.uriType == RESTORE_URI_TYPE_EMPTY) {
        std::string keyPath = restoreTaskInfo.keyPath;
        ffrt::submit([keyPath]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(EARLY_NOTIFY_DELAY_MS));
            InnerRestoreResult result;
            result.errCode = 0;
            result.stage = "onRestore";
            result.uriType = RESTORE_URI_TYPE_EMPTY;
            result.uri = "";
            result.totalNum = 0;
            result.successNum = 0;
            result.failedNum = 0;
            result.sameNum = 0;
            result.cancelNum = 0;
            result.progress = 0;
            MEDIA_INFO_LOG("[ASYNC] - AddTaskAsync - early notify, keyPath: %{public}s", keyPath.c_str());
            CustomRestoreNotify customRestoreNotify;
            customRestoreNotify.Notify(keyPath, result);
        });
    }
    return *this;
}

unordered_map<string, TimeInfo> PhotoCustomRestoreOperation::QueryMediaInfo(
    const std::shared_ptr<NativeRdb::RdbStore> &rdbStore)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, {}, "rdbstore is nullptr");

    const std::vector<NativeRdb::ValueObject> bindArgs = {BASE_FILE_NUM};
    const std::string sql = "SELECT"
                            "  file_name,"
                            "  date_taken,"
                            "  detail_time "
                            "FROM"
                            "  media_info "
                            "ORDER BY"
                            "  date_added ASC "
                            "  LIMIT ?;";

    auto resultSet = rdbStore->QuerySql(sql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, {}, "resultSet is null");

    unordered_map<string, TimeInfo> timeInfoMap;
    int64_t nowTime = MediaTimeUtils::UTCTimeMilliSeconds();
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string fileName = GetStringVal(SHARE_RESTORE_MEDIA_INFO_FILE_NAME, resultSet);
        int64_t dateTaken = GetInt64Val(SHARE_RESTORE_MEDIA_INFO_DATE_TAKEN, resultSet);
        std::string detailTime = GetStringVal(SHARE_RESTORE_MEDIA_INFO_DETAIL_TIME, resultSet);
        timeInfoMap.insert({fileName, {nowTime, dateTaken, detailTime}});
        nowTime++;
    }
    resultSet->Close();
    return timeInfoMap;
}

unordered_map<string, TimeInfo> PhotoCustomRestoreOperation::GetTimeInfoMap(RestoreTaskInfo &restoreTaskInfo)
{
    if (restoreTaskInfo.dbPath.empty()) {
        MEDIA_ERR_LOG("dbPath not exists, dbPath: %{public}s", restoreTaskInfo.dbPath.c_str());
        return {};
    }
    const std::string dbPath = CUSTOM_RESTORE_DIR + "/" + restoreTaskInfo.dbPath;
    const std::string dbName = MediaFileUtils::GetFileName(dbPath);
    if (dbName.empty() || access(dbPath.c_str(), F_OK) != 0) {
        MEDIA_ERR_LOG("db not exists, dbPath: %{public}s", restoreTaskInfo.dbPath.c_str());
        return {};
    }
    NativeRdb::RdbStoreConfig config(dbName);
    config.SetPath(dbPath);
    config.SetBundleName(restoreTaskInfo.bundleName);
    config.SetSecurityLevel(NativeRdb::SecurityLevel::S3);
    config.SetWalLimitSize(SHARE_RESTORE_DB_WAL_LIMIT_SIZE);

    int32_t err;
    ShareRestoreRdbCallback cb;
    std::shared_ptr<NativeRdb::RdbStore> rdbStore =
        NativeRdb::RdbHelper::GetRdbStore(config, SHARE_RESTORE_DB_VERSION, cb, err);
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("GetRdbStore failed, dbPath: %{public}s, err: %{public}d", restoreTaskInfo.dbPath.c_str(), err);
        return {};
    }
    return QueryMediaInfo(rdbStore);
}

void PhotoCustomRestoreOperation::DoCustomRestore(RestoreTaskInfo &restoreTaskInfo)
{
    ffrt_set_cpu_worker_max_num(ffrt::qos_utility, MAX_RESTORE_THREAD_NUM);
    vector<string> files;
    GetDirFiles(restoreTaskInfo.sourceDir, files);
    InitRestoreTask(restoreTaskInfo, files.size());
    MediaShareDirtyDataCleaner::CheckDirtyData();
    MediaShareDirtyDataCleaner::UpdateShareTime(true);
    const unordered_map<string, TimeInfo> timeInfoMap = GetTimeInfoMap(restoreTaskInfo);
    bool isFirstRestoreSuccess = false;
    int32_t firstRestoreIndex = 0;
    int32_t total = static_cast<int32_t>(files.size());
    MEDIA_INFO_LOG("DoCustomRestore files count: %{public}d", total);
    int32_t lastIndex = total - 1;
    for (int32_t index = 0; index < total; index++) {
        if (IsCancelTask(restoreTaskInfo)) {
            break;
        }
        if (index == 0 || !isFirstRestoreSuccess) {
            isFirstRestoreSuccess =
                HandleFirstRestoreFile(timeInfoMap, restoreTaskInfo, files, index, firstRestoreIndex);
            if (!isFirstRestoreSuccess && index == lastIndex) {
                break;
            }
            continue;
        }
        // Remainder and multiple of MAX_RESTORE_FILE_NUM
        int32_t remainder = (index + 1) % MAX_RESTORE_FILE_NUM;
        int32_t multiples = (index + 1) / MAX_RESTORE_FILE_NUM;
        if (remainder == 0 || index == lastIndex) {
            int32_t fileNum = remainder == 0 && multiples > 0 ? MAX_RESTORE_FILE_NUM : remainder;
            int32_t beginOffset = index - fileNum + 1;
            if (beginOffset <= firstRestoreIndex) {
                // not contain first restore file
                beginOffset = firstRestoreIndex + 1;
                fileNum = index - beginOffset + 1;
            }
            int32_t notifyType = index == lastIndex ? NOTIFY_LAST : NOTIFY_PROGRESS;
            vector<string> subFiles(files.begin() + beginOffset, files.begin() + index + 1);
            ffrt::submit(
                [this, &timeInfoMap, &restoreTaskInfo, notifyType, subFiles = std::move(subFiles)]() {
                    HandleBatchCustomRestore(timeInfoMap, restoreTaskInfo, notifyType, std::move(subFiles));
                },
                {},
                {},
                ffrt::task_attr().qos(static_cast<int32_t>(ffrt::qos_utility)));
        }
    }
    ffrt::wait();
    MediaShareDirtyDataCleaner::UpdateShareTime(false);
    MediaShareDirtyDataCleaner::SetSharingState(false);
    ReleaseCustomRestoreTask(restoreTaskInfo);
}

void PhotoCustomRestoreOperation::PhotoCustomRestoreOperation::ReleaseCustomRestoreTask(
    RestoreTaskInfo &restoreTaskInfo)
{
    photoCache_.clear();
    CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteDir(restoreTaskInfo.sourceDir), "delete dir failed.");

    if (!restoreTaskInfo.dbPath.empty()) {
        std::string dbPath = CUSTOM_RESTORE_DIR + "/" + restoreTaskInfo.dbPath;
        std::string dbDir = MediaFileUtils::GetParentPath(dbPath);
        CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteDir(dbDir), "delete db dir failed.");
    }
    ReportCustomRestoreTask(restoreTaskInfo);
    if (IsCancelTask(restoreTaskInfo)) {
        CancelTaskFinish(restoreTaskInfo);
    }
}

void PhotoCustomRestoreOperation::ReportCustomRestoreTask(RestoreTaskInfo &restoreTaskInfo)
{
    restoreTaskInfo.endTime = MediaTimeUtils::UTCTimeSeconds();
    CustomRestoreDfxDataPoint point;
    point.customRestorePackageName = restoreTaskInfo.packageName;
    point.albumLPath = restoreTaskInfo.albumLpath;
    point.keyPath = restoreTaskInfo.keyPath;
    point.totalNum = restoreTaskInfo.totalNum;
    point.successNum = successNum_;
    point.failedNum = failNum_;
    point.sameNum = sameNum_;
    if (IsCancelTask(restoreTaskInfo)) {
        point.cancelNum = point.totalNum - point.successNum - point.sameNum - point.failedNum;
    } else {
        point.cancelNum = 0;
        point.failedNum = point.totalNum - point.successNum - point.sameNum;
    }
    point.totalTime = static_cast<uint64_t>(restoreTaskInfo.endTime - restoreTaskInfo.beginTime);
    MEDIA_INFO_LOG("report custom restore finished. cost:%{public}" PRId64, point.totalTime);
    DfxReporter::ReportCustomRestoreFusion(point);
}

bool PhotoCustomRestoreOperation::HandleFirstRestoreFile(const unordered_map<string, TimeInfo> &timeInfoMap,
    RestoreTaskInfo &restoreTaskInfo, const vector<string> &files, int32_t index, int32_t &firstRestoreIndex)
{
    vector<string> subFiles(files.begin() + index, files.begin() + index + 1);
    UniqueNumber uniqueNumber;
    int32_t errCode = E_OK;
    if (!subFiles.empty() && TlvUtil::ValidateTlvFile(subFiles[0]) == E_OK) {
        errCode = HandleTlvRestore(timeInfoMap, restoreTaskInfo, subFiles, true, uniqueNumber);
    } else {
        errCode = HandleCustomRestore(timeInfoMap, restoreTaskInfo, subFiles, true, uniqueNumber);
    }
    bool isFirstRestoreSuccess = errCode == E_OK;
    int32_t lastIndex = static_cast<int32_t>(files.size() - 1);
    if (!isFirstRestoreSuccess && index == lastIndex) {
        MEDIA_ERR_LOG("first file restore failed. stop restore task.");
        SendNotifyMessage(restoreTaskInfo, NOTIFY_FIRST, errCode, 1, uniqueNumber);
    }
    if (isFirstRestoreSuccess) {
        MEDIA_ERR_LOG("first file restore success.");
        firstRestoreIndex = index;
        int notifyType = index == lastIndex ? NOTIFY_LAST : NOTIFY_FIRST;
        SendNotifyMessage(restoreTaskInfo, notifyType, errCode, 1, uniqueNumber);
    }
    return isFirstRestoreSuccess;
}

void PhotoCustomRestoreOperation::HandleBatchCustomRestore(const unordered_map<string, TimeInfo> &timeInfoMap,
    RestoreTaskInfo &restoreTaskInfo, int32_t notifyType, const vector<string> &subFiles)
{
    if (IsCancelTask(restoreTaskInfo)) {
        return;
    }
    int32_t fileNum = static_cast<int32_t>(subFiles.size());
    UniqueNumber uniqueNumber;
    int32_t errCode = E_OK;
    if (!subFiles.empty() && HasTlvFiles(subFiles)) {
        errCode = HandleTlvRestore(timeInfoMap, restoreTaskInfo, subFiles, false, uniqueNumber);
    } else {
        errCode = HandleCustomRestore(timeInfoMap, restoreTaskInfo, subFiles, false, uniqueNumber);
    }
    SendNotifyMessage(restoreTaskInfo, notifyType, errCode, fileNum, uniqueNumber);
}

void PhotoCustomRestoreOperation::InitRestoreTask(RestoreTaskInfo &restoreTaskInfo, int32_t fileNum)
{
    successNum_.store(0);
    failNum_.store(0);
    sameNum_.store(0);
    photoCache_.clear();
    restoreTaskInfo.totalNum = fileNum;
    restoreTaskInfo.beginTime = MediaTimeUtils::UTCTimeSeconds();
    std::thread applyEfficiencyQuotaThread([this, fileNum] { ApplyEfficiencyQuota(fileNum); });
    applyEfficiencyQuotaThread.detach();
    QueryAlbumId(restoreTaskInfo);
    GetAlbumInfoBySubType(PhotoAlbumSubType::IMAGE, restoreTaskInfo.imageAlbumUri, restoreTaskInfo.imageAlbumId);
    GetAlbumInfoBySubType(PhotoAlbumSubType::VIDEO, restoreTaskInfo.videoAlbumUri, restoreTaskInfo.videoAlbumId);
}

static void NotifyAnalysisAlbum(const vector<string>& changedAlbumIds)
{
    if (changedAlbumIds.size() <= 0) {
        return;
    }
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
    for (const string& albumId : changedAlbumIds) {
        watch->Notify(MediaFileUtils::GetUriByExtrConditions(
            PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX, albumId), NotifyType::NOTIFY_UPDATE);
    }
}

static void UpdateAndNotifyShootingModeAlbumIfNeeded(const vector<RestoreFileInfo>& fileInfos)
{
    set<ShootingModeAlbumType> albumTypesSet;
    for (const auto &fileInfo : fileInfos) {
        vector<ShootingModeAlbumType> albumTypes = ShootingModeAlbum::GetShootingModeAlbumOfAsset(
            fileInfo.subtype, fileInfo.mimeType, fileInfo.movingPhotoEffectMode,
            fileInfo.frontCamera, fileInfo.shootingMode);
        albumTypesSet.insert(albumTypes.begin(), albumTypes.end());
    }

    vector<string> albumIdsToUpdate;
    for (const auto& type : albumTypesSet) {
        int32_t albumId;
        if (MediaLibraryRdbUtils::QueryShootingModeAlbumIdByType(type, albumId)) {
            albumIdsToUpdate.push_back(to_string(albumId));
        }
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbstore is nullptr");

    if (albumIdsToUpdate.size() > 0) {
        MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(rdbStore, albumIdsToUpdate);
        NotifyAnalysisAlbum(albumIdsToUpdate);
    }
}

vector<RestoreFileInfo> PhotoCustomRestoreOperation::scanfile(const unordered_map<string, TimeInfo> &timeInfoMap,
    RestoreTaskInfo &restoreTaskInfo, const vector<string> &filePathVector,
    vector<RestoreFileInfo> &restoreFiles, int32_t &sameFileNum, bool isFirst)
{
    auto scanConfig = ScanConfigBuilder().UseCustomRestorePreset(
        filePathVector,
        restoreFiles,
        timeInfoMap,
        restoreTaskInfo.albumId,
        restoreTaskInfo.isDeduplication,
        restoreTaskInfo.hasPhotoCache,
        photoCache_,
        restoreTaskInfo.packageName,
        restoreTaskInfo.bundleName,
        restoreTaskInfo.appId,
        isFirst).Build();
    int32_t errCode = MediaScannerManager::GetInstance()->ScanSync(scanConfig);
    sameFileNum = scanConfig.GetOutSameFileNum();
    return scanConfig.GetOutFileInfos();
}

int32_t PhotoCustomRestoreOperation::HandleCustomRestore(const unordered_map<string, TimeInfo> &timeInfoMap,
    RestoreTaskInfo &restoreTaskInfo, const vector<string> &filePathVector, bool isFirst, UniqueNumber &uniqueNumber)
{
    MEDIA_INFO_LOG("HandleCustomRestore begin. size: %{public}d, isFirst: %{public}d",
        static_cast<int32_t>(filePathVector.size()),
        isFirst ? 1 : 0);
    vector<RestoreFileInfo> restoreFiles = GetFileInfos(filePathVector, uniqueNumber);
    int32_t errCode = UpdateUniqueNumber(uniqueNumber);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "UpdateUniqueNumber failed. errCode: %{public}d", errCode);

    vector<RestoreFileInfo> destRestoreFiles = SetDestinationPath(restoreFiles, uniqueNumber);
    if (destRestoreFiles.size() == 0) {
        MEDIA_ERR_LOG("restore file number is zero.");
        return E_ERR;
    }
    int32_t sameFileNum = 0;
    MediaShareDirtyDataCleaner::UpdateCleanFlag(true);
    vector<RestoreFileInfo> insertRestoreFiles =
        scanfile(timeInfoMap, restoreTaskInfo, filePathVector, destRestoreFiles, sameFileNum, isFirst);
    MediaShareDirtyDataCleaner::SetSharingState(true);
    int32_t successFileNum = RenameFiles(insertRestoreFiles);
    AccurateRefresh::AssetAccurateRefresh assetRefresh(AccurateRefresh::CUSTOM_RESTORE_BUSSINESS_NAME);
    errCode = BatchUpdateTimePending(insertRestoreFiles, assetRefresh);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "BatchUpdateTimePending failed. errCode: %{public}d", errCode);
    MediaShareDirtyDataCleaner::SetSharingState(false);
    MediaShareDirtyDataCleaner::UpdateCleanFlag(false);
    assetRefresh.RefreshAlbum();
    assetRefresh.Notify();
    UpdateAndNotifyShootingModeAlbumIfNeeded(insertRestoreFiles);
    if (isFirst) {
        CHECK_AND_RETURN_RET(successFileNum != 0, E_ERR);
        CHECK_AND_RETURN_RET_LOG(UpdatePhotoAlbum(restoreTaskInfo, insertRestoreFiles[0]) == E_OK,
            errCode, "UpdatePhotoAlbum failed.");
    }
    int32_t totalFileNum = static_cast<int32_t>(filePathVector.size());
    successNum_.fetch_add(successFileNum);
    failNum_.fetch_add(totalFileNum - successFileNum - sameFileNum);
    MEDIA_DEBUG_LOG("HandleCustomRestore success.");
    return E_OK;
}

int32_t PhotoCustomRestoreOperation::ProcessExtractTlvFile(const std::string &tlvFilePath, std::string &destDir,
    std::unordered_map<TlvTag, std::string> &extractedFiles)
{
    extractedFiles.clear();
    destDir = "";
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(tlvFilePath), E_ERR, "tlvFilePath is empty");
    MEDIA_DEBUG_LOG("tlv file path: %{public}s", DfxUtils::GetSafePath(tlvFilePath).c_str());
    destDir = PhotoCustomRestoreOperation::GetUniqueTempDir(tlvFilePath);
    CHECK_AND_RETURN_RET_LOG(!destDir.empty(), E_ERR, "destDir is empty");
    MEDIA_DEBUG_LOG("Decode tlv dir: %{public}s", DfxUtils::GetSafePath(destDir).c_str());
    string originFileName = MediaFileUtils::GetFileName(tlvFilePath);
    CHECK_AND_RETURN_RET_LOG(!originFileName.empty(), E_ERR, "origin file name is empty");
    int32_t ret = TlvUtil::ExtractTlv(tlvFilePath, destDir, extractedFiles, originFileName);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ExtractTlv failed, ret: %{public}d", ret);
        auto it = extractedFiles.find(TlvTag::TLV_TAG_ORIGIN);
        CHECK_AND_RETURN_RET_LOG(it != extractedFiles.end(), E_ERR, "origin file not in tlv");
        std::string originPath = it->second;
        CHECK_AND_RETURN_RET_LOG(!originPath.empty(), E_ERR, "origin file path is empty");
        extractedFiles.clear();
        extractedFiles[TlvTag::TLV_TAG_ORIGIN] = originPath;
    }
    return E_OK;
}

bool PhotoCustomRestoreOperation::DeleteDirectoryIfExists(const std::string &dirPath)
{
    if (MediaFileUtils::IsDirExists(dirPath)) {
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::DeleteDir(dirPath), false, "delete dir failed: %{public}s",
            DfxUtils::GetSafePath(dirPath).c_str());
    }
    return true;
}

int32_t PhotoCustomRestoreOperation::HandleTlvRestore(const unordered_map<string, TimeInfo> &timeInfoMap,
    RestoreTaskInfo &restoreTaskInfo, const vector<string> &filePathVector, bool isFirst, UniqueNumber &uniqueNumber)
{
    MEDIA_INFO_LOG("HandleTlvRestore begin. size: %{public}d, isFirst: %{public}d",
        static_cast<int32_t>(filePathVector.size()), isFirst ? 1 : 0);
    CHECK_AND_RETURN_RET_LOG(!filePathVector.empty(), E_ERR, "filePathVector is empty");
    int32_t totalSuccess = 0;
    int32_t sameFileNum = 0;
    int32_t ret = E_OK;
    std::string decodeTlvDir = "";
    std::unordered_map<TlvTag, std::string> decodeTlvPathMap;
    vector<string> fallbackFiles;
    UniqueNumber tempUniqueNumber;
    for (const auto &tlvPath : filePathVector) {
        CHECK_AND_CONTINUE_ERR_LOG(MediaFileUtils::IsFileExists(tlvPath), "tlvPath is not exist: %{public}s",
            DfxUtils::GetSafePath(tlvPath).c_str());
        if (TlvUtil::ValidateTlvFile(tlvPath) != E_OK) {
            MEDIA_WARN_LOG("not tlv file, path: %{public}s", DfxUtils::GetSafePath(tlvPath).c_str());
            fallbackFiles.push_back(tlvPath);
            continue;
        }
        ret = ProcessExtractTlvFile(tlvPath, decodeTlvDir, decodeTlvPathMap);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("ProcessExtractTlvFile failed, ret: %{public}d", ret);
            CHECK_AND_PRINT_LOG(DeleteDirectoryIfExists(decodeTlvDir), "delete decode tlv dir failed.");
            fallbackFiles.push_back(tlvPath);
            continue;
        }
        CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteFile(tlvPath), "delete tlv file failed, path: %{public}s",
            DfxUtils::GetSafePath(tlvPath).c_str());
        tempUniqueNumber.clear();
        ret = HandleTlvSingleRestore(decodeTlvPathMap, timeInfoMap, restoreTaskInfo, isFirst, tempUniqueNumber);
        uniqueNumber = uniqueNumber + tempUniqueNumber;
        CHECK_AND_PRINT_LOG(DeleteDirectoryIfExists(decodeTlvDir), "delete decode tlv dir failed.");
        CHECK_AND_PRINT_LOG(ret == E_FILE_EXIST || ret == E_OK, "HandleTlvSingleRestore failed, ret: %{public}d", ret);
        CHECK_AND_EXECUTE(ret != E_FILE_EXIST, sameFileNum++);
        CHECK_AND_EXECUTE(ret != E_OK, totalSuccess++);
    }
    int32_t totalFileNum = static_cast<int32_t>(filePathVector.size());
    successNum_.fetch_add(totalSuccess);
    failNum_.fetch_add(totalFileNum - totalSuccess - sameFileNum);
    if (!fallbackFiles.empty()) {
        tempUniqueNumber.clear();
        ret = HandleCustomRestore(timeInfoMap, restoreTaskInfo, fallbackFiles, isFirst, tempUniqueNumber);
        uniqueNumber = uniqueNumber + tempUniqueNumber;
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "HandleCustomRestore failed in fallback, ret: %{public}d", ret);
    }
    MEDIA_INFO_LOG("HandleTlvRestore end, totalNum: %{public}d, totalSuccess: %{public}d, fallbackNum: %{public}d",
        totalFileNum, successNum_.load(), static_cast<int32_t>(fallbackFiles.size()));
    return E_OK;
}

std::string PhotoCustomRestoreOperation::GetUniqueTempDir(const std::string &tlvPath)
{
    std::string fileName = MediaFileUtils::GetFileName(tlvPath);
    std::string baseTempDir = MediaFileUtils::GetParentPath(tlvPath) + "/temp_" + ScannerUtils::GetFileTitle(fileName);
    std::string uniqueTempDir = baseTempDir;
    int32_t attempt = 0;
    const int32_t maxAttempts = 100;
    while (MediaFileUtils::IsDirExists(uniqueTempDir) && attempt < maxAttempts) {
        uniqueTempDir = baseTempDir + "_" + std::to_string(attempt);
        attempt++;
    }
    CHECK_AND_RETURN_RET_LOG(!MediaFileUtils::IsDirExists(uniqueTempDir) &&
        MediaFileUtils::CreateDirectory(uniqueTempDir), "", "Create unique temp dir failed: %{private}s",
        uniqueTempDir.c_str());
    return uniqueTempDir;
}

int32_t PhotoCustomRestoreOperation::UpdateTlvEditDataSize(const std::string &assetPath)
{
    MEDIA_DEBUG_LOG("UpdateTlvEditDataSize begin");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "rdbStore is null");
    vector<string> queryColumns = { MediaColumn::MEDIA_ID };
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_FILE_PATH, assetPath);
    auto resultSet = rdbStore->Query(predicates, queryColumns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "Query failed for assetPath: %{public}s",
        DfxUtils::GetSafePath(assetPath).c_str());
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        MEDIA_ERR_LOG("No record found for assetPath: %{public}s", DfxUtils::GetSafePath(assetPath).c_str());
        return E_ERR;
    }
    int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    resultSet->Close();
    std::string editDir = MediaEditUtils::GetEditDataDir(assetPath);
    CHECK_AND_RETURN_RET_LOG(!editDir.empty(), E_ERR, "Edit data dir is empty for assetPath: %{public}s",
        DfxUtils::GetSafePath(assetPath).c_str());
    int32_t ret = MediaLibraryRdbStore::UpdateEditDataSize(rdbStore, to_string(fileId), editDir);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "UpdateEditDataSize failed for fileId: %{public}d", fileId);
    MEDIA_INFO_LOG("UpdateTlvEditDataSize success, fileId: %{public}d", fileId);
    return E_OK;
}

int32_t PhotoCustomRestoreOperation::HandleTlvSingleRestore(const std::unordered_map<TlvTag, std::string> &editFileMap,
    const unordered_map<string, TimeInfo> &timeInfoMap, RestoreTaskInfo &restoreTaskInfo, bool isFirst,
    UniqueNumber &uniqueNumber)
{
    MEDIA_INFO_LOG("HandleTlvSingleRestore begin.");

    CHECK_AND_RETURN_RET_LOG(editFileMap.count(TlvTag::TLV_TAG_ORIGIN) > 0, E_ERR, "Not find TLV_TAG_ORIGIN in tlv");
    vector<string> effectPaths = {editFileMap.at(TlvTag::TLV_TAG_ORIGIN)};
    vector<RestoreFileInfo> restoreFiles = GetFileInfos(effectPaths, uniqueNumber);
    int32_t ret = UpdateUniqueNumber(uniqueNumber);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "UpdateUniqueNumber failed. ret: %{public}d", ret);

    vector<RestoreFileInfo> destRestoreFiles = SetDestinationPath(restoreFiles, uniqueNumber);
    CHECK_AND_RETURN_RET_LOG(!destRestoreFiles.empty(), E_ERR, "restore file number is zero.");

    int32_t sameFileNum = 0;
    MediaShareDirtyDataCleaner::UpdateCleanFlag(true);
    vector<RestoreFileInfo> insertRestoreFiles =
        scanfile(timeInfoMap, restoreTaskInfo, effectPaths, destRestoreFiles, sameFileNum, isFirst);
    CHECK_AND_RETURN_RET_INFO_LOG(sameFileNum == 0, E_FILE_EXIST, "tlv single restore has same file.");
    MediaShareDirtyDataCleaner::SetSharingState(true);
    int32_t successFileNum = RenameFiles(insertRestoreFiles);
    CHECK_AND_RETURN_RET_LOG(successFileNum > 0, E_ERR, "RenameFiles failed.");

    std::string assetPath = insertRestoreFiles[0].filePath;
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(assetPath), E_ERR, "Restored asset file not exists");
    ret = HandleAllEditData(editFileMap, assetPath);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("HandleAllEditData failed, rollback tlv restore. ret: %{public}d", ret);
        RestoreTlvRollback(assetPath);
        return ret;
    }
    ret = UpdateTlvEditDataSize(assetPath);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "UpdateTlvEditDataSize failed");
    AccurateRefresh::AssetAccurateRefresh assetRefresh(AccurateRefresh::CUSTOM_RESTORE_BUSSINESS_NAME);
    ret = BatchUpdateTimePending(insertRestoreFiles, assetRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "BatchUpdateTimePending failed. ret: %{public}d", ret);
    MediaShareDirtyDataCleaner::SetSharingState(false);
    MediaShareDirtyDataCleaner::UpdateCleanFlag(false);
    assetRefresh.RefreshAlbum();
    assetRefresh.Notify();
    UpdateAndNotifyShootingModeAlbumIfNeeded(insertRestoreFiles);
    if (isFirst && !insertRestoreFiles.empty()) {
        ret = UpdatePhotoAlbum(restoreTaskInfo, insertRestoreFiles[0]);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "UpdatePhotoAlbum failed.");
    }
    return E_OK;
}

void PhotoCustomRestoreOperation::RestoreTlvRollback(const std::string &assetPath)
{
    MEDIA_INFO_LOG("RestoreTlvRollback begin");

    string sourcePhotoPath = MediaEditUtils::GetEditDataSourcePath(assetPath);
    CHECK_AND_EXECUTE(!MediaFileUtils::IsFileExists(sourcePhotoPath), MediaFileUtils::DeleteFile(sourcePhotoPath));

    string sourceBackPhotoPath = MediaEditUtils::GetEditDataSourceBackPath(assetPath);
    CHECK_AND_EXECUTE(!MediaFileUtils::IsFileExists(sourceBackPhotoPath),
        MediaFileUtils::DeleteFile(sourceBackPhotoPath));
 
    string editDataPath = MediaEditUtils::GetEditDataPath(assetPath);
    CHECK_AND_EXECUTE(!MediaFileUtils::IsFileExists(editDataPath),
        MediaFileUtils::DeleteFile(editDataPath));

    string editDataCameraPath = MediaEditUtils::GetEditDataCameraPath(assetPath);
    CHECK_AND_EXECUTE(!MediaFileUtils::IsFileExists(editDataCameraPath),
        MediaFileUtils::DeleteFile(editDataCameraPath));

    string movingPhotoVideoSourcePath = MovingPhotoFileUtils::GetSourceMovingPhotoVideoPath(assetPath);
    CHECK_AND_EXECUTE(!MediaFileUtils::IsFileExists(movingPhotoVideoSourcePath),
        MediaFileUtils::DeleteFile(movingPhotoVideoSourcePath));

    string movingPhotoVideoSourceBackPath = MovingPhotoFileUtils::GetSourceBackMovingPhotoVideoPath(assetPath);
    CHECK_AND_EXECUTE(!MediaFileUtils::IsFileExists(movingPhotoVideoSourceBackPath),
        MediaFileUtils::DeleteFile(movingPhotoVideoSourceBackPath));
}

int32_t PhotoCustomRestoreOperation::HandleMovingPhotoVideoRestore(const string &originalSrcPath,
    const std::string &assetPath)
{
    MEDIA_INFO_LOG("HandleMovingPhotoVideoRestore start");
    string targetPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(assetPath);
    int32_t ret = MoveFile(originalSrcPath, targetPath);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Move moving photo video file failed");
    return E_OK;
}

int32_t PhotoCustomRestoreOperation::HandlePhotoSourceRestore(const string &originalSrcPath,
    const std::string &assetPath)
{
    MEDIA_INFO_LOG("HandlePhotoSourceRestore start");
    string targetPath = MediaEditUtils::GetEditDataSourcePath(assetPath);
    int32_t ret = MoveFile(originalSrcPath, targetPath);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Move photo source file failed to %{public}s",
        DfxUtils::GetSafePath(targetPath).c_str());
    return E_OK;
}

int32_t PhotoCustomRestoreOperation::HandleMovingPhotoVideoSourceRestore(const string &srcPath,
    const std::string &assetPath)
{
    MEDIA_INFO_LOG("HandleMovingPhotoVideoSourceRestore start");
    string targetPath = MovingPhotoFileUtils::GetSourceMovingPhotoVideoPath(assetPath);
    int32_t ret = MoveFile(srcPath, targetPath);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Move moving photo video source file failed to %{public}s",
        DfxUtils::GetSafePath(targetPath).c_str());
    return E_OK;
}

int32_t PhotoCustomRestoreOperation::HandleMovingPhotoVideoSourceBackRestore(const string &srcPath,
    const std::string &assetPath)
{
    MEDIA_INFO_LOG("HandleMovingPhotoVideoSourceBackRestore start");
    string targetPath = MovingPhotoFileUtils::GetSourceBackMovingPhotoVideoPath(assetPath);
    int32_t ret = MoveFile(srcPath, targetPath);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Move moving photo video source back file failed to %{public}s",
        DfxUtils::GetSafePath(targetPath).c_str());
    return E_OK;
}

int32_t PhotoCustomRestoreOperation::MoveFile(const std::string &srcPath, const std::string &destPath)
{
    CHECK_AND_RETURN_RET_LOG(!destPath.empty(), E_INVALID_PATH, "MoveFile destPath is empty");
    std::string dir = MediaFileUtils::GetParentPath(destPath);
    if (!MediaFileUtils::IsDirExists(dir)) {
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(dir), E_HAS_FS_ERROR,
            "Create dir failed: %{public}s", DfxUtils::GetSafePath(dir).c_str());
    }
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::MoveFile(srcPath, destPath, true), E_HAS_FS_ERROR,
        "MoveFile failed to %{public}s",  DfxUtils::GetSafePath(destPath).c_str());
    return E_OK;
}

int32_t PhotoCustomRestoreOperation::HandlePhotoSourceBackRestore(const std::string &sourceBackSrcPath,
    const std::string &assetPath)
{
    MEDIA_INFO_LOG("HandlePhotoSourceBackRestore start");
    std::string targetPath = MediaEditUtils::GetEditDataSourceBackPath(assetPath);
    int32_t ret = MoveFile(sourceBackSrcPath, targetPath);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Move source back file failed to %{public}s",
        DfxUtils::GetSafePath(targetPath).c_str());
    return E_OK;
}

int32_t PhotoCustomRestoreOperation::HandleEditDataRestore(const string &editDataSrcPath, const std::string &assetPath)
{
    MEDIA_INFO_LOG("HandleEditDataRestore start");
    string targetPath = MediaEditUtils::GetEditDataPath(assetPath);
    int32_t ret = MoveFile(editDataSrcPath, targetPath);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Move edit data file failed to %{public}s",
        DfxUtils::GetSafePath(targetPath).c_str());
    return E_OK;
}

int32_t PhotoCustomRestoreOperation::HandleExtraDataRestore(const string &editDataCameraSrcPath,
    const std::string &assetPath)
{
    MEDIA_INFO_LOG("HandleExtraDataRestore start");
    string targetPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(assetPath);
    int32_t ret = MoveFile(editDataCameraSrcPath, targetPath);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Move moving photo extra data failed");
    return E_OK;
}

int32_t PhotoCustomRestoreOperation::HandleEditDataCameraRestore(const string &editDataCameraSrcPath,
    const std::string &assetPath)
{
    MEDIA_INFO_LOG("HandleEditDataCameraRestore start");
    string targetPath = MediaEditUtils::GetEditDataCameraPath(assetPath);
    int32_t ret = MoveFile(editDataCameraSrcPath, targetPath);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Move edit camera file failed to %{public}s",
        DfxUtils::GetSafePath(targetPath).c_str());
    return E_OK;
}

bool PhotoCustomRestoreOperation::CheckNeedProcessMovingPhotoSize(const NativeRdb::ValuesBucket &values)
{
    MEDIA_DEBUG_LOG("CheckNeedProcessMovingPhotoSize start");
    CHECK_AND_RETURN_RET_LOG(!values.IsEmpty(), false, "ValuesBucket is empty");
    CHECK_AND_RETURN_RET_LOG(values.HasColumn(PhotoColumn::PHOTO_SUBTYPE), false, "subtype not exist in values");
    NativeRdb::ValueObject valueObj;
    CHECK_AND_RETURN_RET_LOG(values.GetObject(PhotoColumn::PHOTO_SUBTYPE, valueObj), false,
        "Get subtype failed");
    CHECK_AND_RETURN_RET_LOG(valueObj.GetType() == NativeRdb::ValueObject::TypeId::TYPE_INT, false, "type not match");
    int32_t subtype = 0;
    CHECK_AND_RETURN_RET_LOG(valueObj.GetInt(subtype) == E_OK, false, "Get subtype int failed");
    CHECK_AND_RETURN_RET_LOG(values.GetObject(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, valueObj), false,
        "Get moving photo mode failed");
    CHECK_AND_RETURN_RET_LOG(valueObj.GetType() == NativeRdb::ValueObject::TypeId::TYPE_INT, false, "type not match");
    int32_t movingPhotoMode = 0;
    CHECK_AND_RETURN_RET_LOG(valueObj.GetInt(movingPhotoMode) == E_OK, false, "Get moving photo mode int failed");
    if (subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ||
        movingPhotoMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY)) {
        MEDIA_INFO_LOG("Need process moving photo size");
        return true;
    }
    return false;
}

int32_t PhotoCustomRestoreOperation::HandleDbFieldsFromJsonRestore(const std::string &jsonPath,
    const std::string &assetPath)
{
    MEDIA_INFO_LOG("HandleDbFieldsFromJsonRestore start");
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(jsonPath), E_INVALID_VALUES, "json file not exists");
    int32_t version = AssetCompressVersionManager::GetAssetCompressVersion();
    AssetCompressSpec assetCompressSpec = AssetCompressVersionManager::GetAssetCompressSpec(version);
    NativeRdb::ValuesBucket values = MediaJsonOperation::ReadJsonToValuesBucket(jsonPath,
        assetCompressSpec.editedDataColumns);
    if (CheckNeedProcessMovingPhotoSize(values)) {
        size_t movingPhotoSize = MovingPhotoFileUtils::GetMovingPhotoSize(assetPath);
        CHECK_AND_RETURN_RET_LOG(movingPhotoSize <= INT64_MAX, E_ERR, "movingPhotoSize overflow");
        values.Put(MediaColumn::MEDIA_SIZE, static_cast<int64_t>(movingPhotoSize));
    }
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore is nullptr.");

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_FILE_PATH, assetPath);

    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, values, predicates);

    CHECK_AND_RETURN_RET_LOG(ret == E_OK && changedRows > 0, E_ERR, "Failed to update edit data for file");

    MEDIA_INFO_LOG("Successfully updated edit data.");
    return E_OK;
}

int32_t PhotoCustomRestoreOperation::HandleAllEditData(const std::unordered_map<TlvTag, std::string> &decodeTlvPathMap,
    const std::string &assetPath)
{
    MEDIA_INFO_LOG("HandleAllEditData begin.");
    CHECK_AND_RETURN_RET_LOG(!decodeTlvPathMap.empty(), E_ERR, "decodeTlvPathMap is empty.");
    int32_t ret = E_OK;
    for (const auto& [tag, path] : decodeTlvPathMap) {
        CHECK_AND_CONTINUE(tag != TlvTag::TLV_TAG_ORIGIN && tag != TlvTag::TLV_TAG_JSON);
        CHECK_AND_CONTINUE_ERR_LOG(MediaFileUtils::IsFileExists(path), "File not exist for tag %{public}d: ",
            static_cast<int32_t>(tag));
        switch (tag) {
            case TlvTag::TLV_TAG_MOVING_PHOTO_VIDEO:
                ret = HandleMovingPhotoVideoRestore(path, assetPath);
                break;
            case TlvTag::TLV_TAG_EDITDATA:
                ret = HandleEditDataRestore(path, assetPath);
                break;
            case TlvTag::TLV_TAG_CAMERA:
                ret = HandleEditDataCameraRestore(path, assetPath);
                break;
            case TlvTag::TLV_TAG_EXTRA_DATA:
                ret = HandleExtraDataRestore(path, assetPath);
                break;
            case TlvTag::TLV_TAG_SOURCE:
                ret = HandlePhotoSourceRestore(path, assetPath);
                break;
            case TlvTag::TLV_TAG_SOURCE_BACK:
                ret = HandlePhotoSourceBackRestore(path, assetPath);
                break;
            case TlvTag::TLV_TAG_MOVING_PHOTO_VIDEO_SOURCE:
                ret = HandleMovingPhotoVideoSourceRestore(path, assetPath);
                break;
            case TlvTag::TLV_TAG_MOVING_PHOTO_VIDEO_SOURCE_BACK:
                ret = HandleMovingPhotoVideoSourceBackRestore(path, assetPath);
                break;
            default:
                MEDIA_WARN_LOG("Unknown TLV tag ignored: %{public}d", static_cast<int32_t>(tag));
                break;
        }
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Handle file failed for tag %{public}d", static_cast<int32_t>(tag));
    }
    CHECK_AND_RETURN_RET_LOG(decodeTlvPathMap.count(TlvTag::TLV_TAG_JSON) > 0, E_ERR, "json tag not exist");
    ret = HandleDbFieldsFromJsonRestore(decodeTlvPathMap.at(TlvTag::TLV_TAG_JSON), assetPath);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "HandleDbFieldsFromJsonRestore failed");
    return ret;
}

int32_t PhotoCustomRestoreOperation::BatchUpdateTimePending(const vector<RestoreFileInfo> &restoreFiles,
    AccurateRefresh::AssetAccurateRefresh &assetRefresh)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "BatchUpdateTimePending: get rdb store fail!");

    // 构建包含所有文件路径的参数列表
    std::vector<std::string> filePahts;
    for (const auto& file : restoreFiles) {
        filePahts.push_back(file.filePath);
    }

    NativeRdb::ValuesBucket values;
    values.Put(MediaColumn::MEDIA_TIME_PENDING, 0);
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(PhotoColumn::MEDIA_FILE_PATH, filePahts);
    int32_t changeRows = -1;

    int32_t errCode = assetRefresh.Update(changeRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG((errCode == E_OK && changeRows > 0), E_HAS_DB_ERROR,
        "BatchUpdateTimePending: update time_pending failed. errCode: %{public}d, updateRows: %{public}d", errCode,
        changeRows);
    return E_OK;
}

int32_t PhotoCustomRestoreOperation::UpdatePhotoAlbum(RestoreTaskInfo &restoreTaskInfo, RestoreFileInfo fileInfo)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET(rdbStore != nullptr, E_HAS_DB_ERROR);
    const string querySql = "SELECT " + MediaColumn::MEDIA_ID + "," + PhotoColumn::PHOTO_OWNER_ALBUM_ID + " FROM " +
                            PhotoColumn::PHOTOS_TABLE + " WHERE data = ?;";
    std::vector<std::string> sqlArgs;
    sqlArgs.push_back(fileInfo.filePath);
    auto resultSet = rdbStore->QuerySql(querySql, sqlArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "execute select unique number failed.");

    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("execute GoToFirstRow err.");
        resultSet->Close();
        return E_HAS_DB_ERROR;
    }
    int32_t fileId = GetInt32Val(PhotoColumn::MEDIA_ID, resultSet);
    int32_t albumId = GetInt32Val(PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSet);
    resultSet->Close();
    std::string updateQuerySql = "UPDATE PhotoAlbum SET unique_id = '" + MediaFileUtils::GenerateUUID() + "' "
        + " WHERE album_id = " + std::to_string(albumId) + " AND (unique_id IS NULL OR unique_id = '')" +
        " AND (" + PhotoAlbumColumns::ALBUM_SUBTYPE + " = " + std::to_string(PhotoAlbumSubType::SOURCE_GENERIC) +
        " AND " + PhotoAlbumColumns::ALBUM_NAME + " != '.hiddenAlbum'" + " )";
    rdbStore->ExecuteSql(updateQuerySql);
    restoreTaskInfo.firstFileId = fileId;
    string extrUri = MediaFileUtils::GetExtraUri(fileInfo.displayName, fileInfo.filePath);
    restoreTaskInfo.firstFileUri = MediaFileUtils::GetUriByExtrConditions(
        CONST_ML_FILE_URI_PREFIX + MediaFileUri::GetMediaTypeUri(fileInfo.mediaType, MEDIA_API_VERSION_V10) + "/",
        to_string(fileId),
        extrUri);
    if (restoreTaskInfo.uriType == RESTORE_URI_TYPE_PHOTO) {
        restoreTaskInfo.uri = restoreTaskInfo.firstFileUri;
    } else if (restoreTaskInfo.uriType == RESTORE_URI_TYPE_EMPTY) {
        restoreTaskInfo.uri = "";
    } else if (restoreTaskInfo.uriType == RESTORE_URI_TYPE_ALBUM) {
        restoreTaskInfo.uri = PhotoAlbumColumns::ALBUM_URI_PREFIX + to_string(albumId);
    } else {
        MEDIA_WARN_LOG("[ASYNC] - UpdatePhotoAlbum - undefined uriType: %{public}d", restoreTaskInfo.uriType);
    }
 
    MEDIA_INFO_LOG("[ASYNC] - UpdatePhotoAlbum - uriType: %{public}d, uri: %{public}s, keyPath: %{public}s",
        restoreTaskInfo.uriType, restoreTaskInfo.uri.c_str(), restoreTaskInfo.keyPath.c_str());
    restoreTaskInfo.albumId = albumId;
    return E_OK;
}

void PhotoCustomRestoreOperation::SendPhotoAlbumNotify(RestoreTaskInfo &restoreTaskInfo, int32_t notifyType,
    const UniqueNumber &uniqueNumber)
{
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
    if (notifyType == NOTIFY_FIRST) {
        watch->Notify(restoreTaskInfo.firstFileUri, NOTIFY_ADD);
    } else {
        watch->Notify(PhotoColumn::PHOTO_URI_PREFIX, NOTIFY_ADD);
    }
    std::string albumUri =
        MediaFileUtils::GetUriByExtrConditions(PhotoAlbumColumns::ALBUM_URI_PREFIX, to_string(restoreTaskInfo.albumId));
    watch->Notify(albumUri, NotifyType::NOTIFY_UPDATE);
    watch->Notify(PhotoColumn::PHOTO_URI_PREFIX, NotifyType::NOTIFY_ALBUM_ADD_ASSET, restoreTaskInfo.albumId);
    if (uniqueNumber.imageTotalNumber > 0) {
        watch->Notify(restoreTaskInfo.imageAlbumUri, NotifyType::NOTIFY_UPDATE);
        watch->Notify(PhotoColumn::PHOTO_URI_PREFIX, NotifyType::NOTIFY_ALBUM_ADD_ASSET, restoreTaskInfo.imageAlbumId);
    }
    if (uniqueNumber.videoTotalNumber > 0) {
        watch->Notify(restoreTaskInfo.videoAlbumUri, NotifyType::NOTIFY_UPDATE);
        watch->Notify(PhotoColumn::PHOTO_URI_PREFIX, NotifyType::NOTIFY_ALBUM_ADD_ASSET, restoreTaskInfo.videoAlbumId);
    }
    MEDIA_DEBUG_LOG("PhotoAlbumNotify finished.");
}

InnerRestoreResult PhotoCustomRestoreOperation::GenerateCustomRestoreNotify(
    RestoreTaskInfo &restoreTaskInfo, int32_t notifyType)
{
    InnerRestoreResult restoreResult;
    restoreResult.errCode = 0;
    restoreResult.stage = (notifyType == NOTIFY_LAST || notifyType == NOTIFY_CANCEL) ? "finished" : "onRestore";
    restoreResult.uriType = restoreTaskInfo.uriType;
    restoreResult.uri = restoreTaskInfo.uri;
    restoreResult.totalNum = restoreTaskInfo.totalNum;
    if (restoreTaskInfo.totalNum == 0) {
        restoreResult.successNum = 0;
        restoreResult.failedNum = 0;
        restoreResult.sameNum = 0;
        restoreResult.progress = 0;
        return restoreResult;
    }
    restoreResult.successNum = successNum_;
    restoreResult.sameNum = sameNum_;
    if (notifyType == NOTIFY_LAST) {
        restoreResult.failedNum = restoreTaskInfo.totalNum - restoreResult.successNum - restoreResult.sameNum;
    } else {
        restoreResult.failedNum = failNum_;
    }
    restoreResult.cancelNum = 0;
    if (notifyType == NOTIFY_CANCEL) {
        restoreResult.cancelNum =
            restoreResult.totalNum - restoreResult.successNum - restoreResult.sameNum - restoreResult.failedNum;
    }
    restoreResult.progress = PROGRESS_MULTI_NUM *
                             (restoreResult.successNum + restoreResult.failedNum + restoreResult.sameNum) /
                             restoreTaskInfo.totalNum;
    return restoreResult;
}

void PhotoCustomRestoreOperation::SendNotifyMessage(
    RestoreTaskInfo &restoreTaskInfo, int32_t notifyType, int32_t errCode, int32_t fileNum,
    const UniqueNumber &uniqueNumber)
{
    // notify album
    if (errCode == E_OK && successNum_ > 0) {
        SendPhotoAlbumNotify(restoreTaskInfo, notifyType, uniqueNumber);
    }
    if (errCode != E_OK) {
        failNum_.fetch_add(fileNum);
    }
    InnerRestoreResult restoreResult = GenerateCustomRestoreNotify(restoreTaskInfo, notifyType);
    if (notifyType == NOTIFY_FIRST && errCode != E_OK) {
        restoreResult.stage = "finished";
        restoreResult.errCode = 1;
        restoreResult.progress = 0;
        restoreResult.uri = "";
    }
    MEDIA_DEBUG_LOG("CustomRestoreNotify stage:%{public}s errCode:%{public}d progress:%{public}d",
        restoreResult.stage.c_str(), restoreResult.errCode, restoreResult.progress);
    MEDIA_DEBUG_LOG(
        "CustomRestoreNotify totalNum:%{public}d successNum:%{public}d failedNum:%{public}d sameNum:%{public}d",
        restoreResult.totalNum, restoreResult.successNum, restoreResult.failedNum, restoreResult.sameNum);
    
    if (notifyType == NOTIFY_FIRST && restoreTaskInfo.earlyNotifySent && errCode == E_OK) {
        MEDIA_INFO_LOG("[ASYNC] - SendNotifyMessage - skip NOTIFY_FIRST (early sent), keyPath: %{public}s",
            restoreTaskInfo.keyPath.c_str());
        return;
    }
    CustomRestoreNotify customRestoreNotify;
    customRestoreNotify.Notify(restoreTaskInfo.keyPath, restoreResult);
}

vector<RestoreFileInfo> PhotoCustomRestoreOperation::SetDestinationPath(
    vector<RestoreFileInfo> &restoreFiles, UniqueNumber &uniqueNumber)
{
    vector<RestoreFileInfo> newRestoreFiles;
    for (auto &fileInfo : restoreFiles) {
        string mediaDirPath;
        int32_t mediaType = fileInfo.mediaType;
        GetAssetRootDir(mediaType, mediaDirPath);
        CHECK_AND_CONTINUE_ERR_LOG(!mediaDirPath.empty(),
            "get asset root dir failed. mediaType: %{public}d", mediaType);

        int32_t fileId = 0;
        if (fileInfo.mediaType == MediaType::MEDIA_TYPE_IMAGE) {
            fileId = uniqueNumber.imageCurrentNumber++;
        } else if (fileInfo.mediaType == MediaType::MEDIA_TYPE_VIDEO) {
            fileId = uniqueNumber.videoCurrentNumber++;
        }
        int32_t bucketNum = 0;
        int32_t retCode = MediaFileUri::CreateAssetBucket(fileId, bucketNum);
        CHECK_AND_CONTINUE_ERR_LOG(retCode == E_OK, "CreateAssetBucket failed. bucketNum: %{public}d", bucketNum);

        string realName;
        retCode = MediaFileUtils::CreateAssetRealName(fileId, fileInfo.mediaType, fileInfo.extension, realName);
        CHECK_AND_CONTINUE_ERR_LOG(retCode == E_OK, "CreateAssetRealName failed. retCode: %{public}d", retCode);

        string dirPath = ROOT_MEDIA_DIR + mediaDirPath + to_string(bucketNum);
        if (!MediaFileUtils::IsFileExists(dirPath)) {
            CHECK_AND_CONTINUE_ERR_LOG(MediaFileUtils::CreateDirectory(dirPath),
                "CreateDirectory failed. retCode: %{public}s", dirPath.c_str());
        }
        fileInfo.filePath = dirPath + "/" + realName;
        fileInfo.fileId = fileId;
        newRestoreFiles.push_back(fileInfo);
    }
    return newRestoreFiles;
}

void PhotoCustomRestoreOperation::GetAssetRootDir(int32_t mediaType, string &rootDirPath)
{
    map<int, string> rootDir = {
        {MEDIA_TYPE_FILE, DOCUMENT_BUCKET + SLASH_CHAR},
        {MEDIA_TYPE_VIDEO, PHOTO_BUCKET + SLASH_CHAR},
        {MEDIA_TYPE_IMAGE, PHOTO_BUCKET + SLASH_CHAR},
        {MEDIA_TYPE_AUDIO, AUDIO_BUCKET + SLASH_CHAR},
    };
    if (rootDir.count(mediaType) == 0) {
        rootDirPath = rootDir[MEDIA_TYPE_FILE];
    } else {
        rootDirPath = rootDir[mediaType];
    }
}

void PhotoCustomRestoreOperation::QueryAlbumId(RestoreTaskInfo &restoreTaskInfo)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "QueryAlbumId: get rdb store fail!");

    const string queryAlbumPathSql =
        "SELECT lpath from album_plugin WHERE (bundle_name = ? OR album_name = ?) AND priority = '1';";
    std::vector<NativeRdb::ValueObject> albumPathParams = {restoreTaskInfo.bundleName, restoreTaskInfo.packageName};
    auto albumPathResult = rdbStore->QuerySql(queryAlbumPathSql, albumPathParams);
    string lpath;
    if (albumPathResult == nullptr) {
        MEDIA_ERR_LOG("QueryAlbumId: query album_plugin failed!");
    } else if (albumPathResult->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_WARN_LOG("QueryAlbumId: album_plugin have no record!");
        albumPathResult->Close();
    } else {
        lpath = GetStringVal("lpath", albumPathResult);
        albumPathResult->Close();
    }
    if (lpath.empty()) {
        lpath = ALBUM_PATH_PREFIX + restoreTaskInfo.packageName;
    }
 
    const string querySql = "SELECT " + PhotoAlbumColumns::ALBUM_ID + " FROM " + PhotoAlbumColumns::TABLE + " WHERE " +
                            PhotoAlbumColumns::ALBUM_LPATH + " = ? ;";
    std::vector<NativeRdb::ValueObject> params = {lpath};
    auto resultSet = rdbStore->QuerySql(querySql, params);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "QueryAlbumId: query PhotoAlbum failed!");

    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_WARN_LOG("album is not exits, skip check duplication.");
        restoreTaskInfo.isDeduplication = false;
        resultSet->Close();
        return;
    }
    int32_t albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
    resultSet->Close();
    if (albumId > 0) {
        restoreTaskInfo.albumId = albumId;
        MEDIA_ERR_LOG("QueryAlbumId albumId:%{public}d", albumId);
        InitPhotoCache(restoreTaskInfo);
    }
}

int32_t PhotoCustomRestoreOperation::GetAlbumInfoBySubType(int32_t subType, string &albumUri, int32_t &albumId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "GetAlbumInfoBySubType: get rdb store fail!");

    const string querySql = "SELECT " + PhotoAlbumColumns::ALBUM_ID + " FROM " + PhotoAlbumColumns::TABLE + " WHERE " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + " = ? ;";
    std::vector<NativeRdb::ValueObject> params = {subType};
    auto resultSet = rdbStore->QuerySql(querySql, params);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR,
        "GetAlbumInfoBySubType: query PhotoAlbum failed! subType=%{public}d", subType);

    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("GetAlbumInfoBySubType first row empty.");
        resultSet->Close();
        return E_HAS_DB_ERROR;
    }
    albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
    resultSet->Close();
    albumUri = MediaFileUtils::GetUriByExtrConditions(PhotoAlbumColumns::ALBUM_URI_PREFIX, to_string(albumId));
    return E_OK;
}

int32_t PhotoCustomRestoreOperation::InitPhotoCache(RestoreTaskInfo &restoreTaskInfo)
{
    bool cond = (!restoreTaskInfo.isDeduplication || restoreTaskInfo.albumId == 0);
    CHECK_AND_RETURN_RET(!cond, E_OK);
    MEDIA_INFO_LOG("InitPhotoCache begin, albumId: %{public}d", restoreTaskInfo.albumId);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "InitPhotoCache: get rdb store fail!");

    const string queryCountSql = "SELECT COUNT(1) as count FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " +
                                 PhotoColumn::PHOTO_OWNER_ALBUM_ID + "=" + to_string(restoreTaskInfo.albumId) + ";";
    auto resultSet = rdbStore->QuerySql(queryCountSql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "InitPhotoCache: get resultSet fail!");
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("InitPhotoCache first row empty.");
        resultSet->Close();
        return E_HAS_DB_ERROR;
    }

    int32_t count = GetInt32Val("count", resultSet);
    resultSet->Close();
    if (count > MAX_PHOTO_CACHE_NUM) {
        MEDIA_WARN_LOG("album has more than %{public}d, skip create cache.", MAX_PHOTO_CACHE_NUM);
        return E_OK;
    }
    const string queryCacheSql = "SELECT " + MediaColumn::MEDIA_NAME + "," + MediaColumn::MEDIA_SIZE + "," +
                                 MediaColumn::MEDIA_TYPE + "," + PhotoColumn::PHOTO_ORIENTATION + " FROM " +
                                 PhotoColumn::PHOTOS_TABLE + " WHERE " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + "=" +
                                 to_string(restoreTaskInfo.albumId) + ";";
    auto resultCacheSet = rdbStore->QuerySql(queryCacheSql);
    CHECK_AND_RETURN_RET(resultCacheSet != nullptr, E_HAS_DB_ERROR);

    while (resultCacheSet->GoToNextRow() == NativeRdb::E_OK) {
        string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultCacheSet);
        int64_t mediaSize = GetInt64Val(MediaColumn::MEDIA_SIZE, resultCacheSet);
        int32_t mediaType = GetInt32Val(MediaColumn::MEDIA_TYPE, resultCacheSet);
        int32_t orientation = GetInt32Val(PhotoColumn::PHOTO_ORIENTATION, resultCacheSet);
        photoCache_.insert(
            displayName + "_" + to_string(mediaSize) + "_" + to_string(mediaType) + "_" + to_string(orientation));
    }
    resultCacheSet->Close();
    restoreTaskInfo.hasPhotoCache = true;
    MEDIA_INFO_LOG("InitPhotoCache success, num:%{public}d", static_cast<int32_t>(photoCache_.size()));
    return E_OK;
}

int32_t PhotoCustomRestoreOperation::UpdateUniqueNumber(UniqueNumber &uniqueNumber)
{
    int32_t errCode = MediaLibraryAssetOperations::CreateAssetUniqueIds(
        MediaType::MEDIA_TYPE_IMAGE, uniqueNumber.imageTotalNumber, uniqueNumber.imageCurrentNumber);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "CreateAssetUniqueNumber IMAGE fail!, ret:%{public}d", errCode);

    MEDIA_DEBUG_LOG("imageTotalNumber: %{public}d imageCurrentNumber: %{public}d",
        uniqueNumber.imageTotalNumber, uniqueNumber.imageCurrentNumber);
    errCode = MediaLibraryAssetOperations::CreateAssetUniqueIds(
        MediaType::MEDIA_TYPE_VIDEO, uniqueNumber.videoTotalNumber, uniqueNumber.videoCurrentNumber);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "CreateAssetUniqueNumber VIDEO fail!, ret:%{public}d", errCode);
    MEDIA_DEBUG_LOG("videoTotalNumber: %{public}d videoCurrentNumber: %{public}d",
        uniqueNumber.videoTotalNumber, uniqueNumber.videoCurrentNumber);
    return E_OK;
}

vector<RestoreFileInfo> PhotoCustomRestoreOperation::GetFileInfos(
    const vector<string> &filePathVector, UniqueNumber &uniqueNumber)
{
    vector<RestoreFileInfo> restoreFiles;
    for (const auto &filePath : filePathVector) {
        RestoreFileInfo fileInfo;
        fileInfo.fileName = MediaFileUtils::GetFileName(filePath);
        fileInfo.displayName = fileInfo.fileName;
        fileInfo.originFilePath = filePath;
        fileInfo.extension = ScannerUtils::GetFileExtension(fileInfo.fileName);
        fileInfo.title = ScannerUtils::GetFileTitle(fileInfo.fileName);
        fileInfo.mediaType = MediaFileUtils::GetMediaType(fileInfo.fileName);
        if (MovingPhotoFileUtils::IsLivePhoto(filePath)) {
            fileInfo.isLivePhoto = true;
            string videoPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(filePath);
            string extraDataPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(filePath);
            string extraPathDir = MovingPhotoFileUtils::GetMovingPhotoExtraDataDir(filePath);
            if (!MediaFileUtils::IsFileExists(extraPathDir) && !MediaFileUtils::CreateDirectory(extraPathDir)) {
                MEDIA_WARN_LOG("Failed to create local extra data dir");
            }
            int32_t ret = MovingPhotoFileUtils::ConvertToMovingPhoto(filePath, filePath, videoPath, extraDataPath);
            if (ret != E_OK) {
                MEDIA_ERR_LOG("Failed to convert live photo, ret:%{public}d", ret);
                (void)MediaFileUtils::DeleteFile(filePath);
                (void)MediaFileUtils::DeleteFile(videoPath);
                (void)MediaFileUtils::DeleteDir(extraPathDir);
            }
        }

        if (fileInfo.mediaType == MediaType::MEDIA_TYPE_FILE) {
            fileInfo.mediaType = MediaFileUtils::GetMediaTypeNotSupported(fileInfo.fileName);
            bool cond = (fileInfo.mediaType == MediaType::MEDIA_TYPE_IMAGE ||
                fileInfo.mediaType == MediaType::MEDIA_TYPE_VIDEO);
            CHECK_AND_WARN_LOG(!cond, "media type not support, fileName is %{private}s", fileInfo.fileName.c_str());
        }
        if (fileInfo.mediaType == MediaType::MEDIA_TYPE_IMAGE) {
            uniqueNumber.imageTotalNumber++;
        } else if (fileInfo.mediaType == MediaType::MEDIA_TYPE_VIDEO) {
            uniqueNumber.videoTotalNumber++;
        } else {
            MEDIA_ERR_LOG(
                "This media type[%{public}s] is not image or video, skip restore.", fileInfo.extension.c_str());
            continue;
        }
        restoreFiles.push_back(fileInfo);
    }
    return restoreFiles;
}

static void UpdateCoverPosition(const string &filePath, int64_t coverPosition)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "UpdateCoverPosition: get rdb store fail!");
    string updateSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_COVER_POSITION + " = ?, " +
        PhotoColumn::PHOTO_IS_RECTIFICATION_COVER + " = 1 WHERE " + PhotoColumn::MEDIA_FILE_PATH + " = ?;";
    std::vector<NativeRdb::ValueObject> params = {coverPosition, filePath};
    int32_t errCode = rdbStore->ExecuteSql(updateSql, params);
    CHECK_AND_PRINT_LOG(errCode >= 0, "UpdateCoverPosition: execute update cover_position failed,"
        " ret = %{public}d", errCode);
}

int32_t PhotoCustomRestoreOperation::RenameFiles(const vector<RestoreFileInfo> &restoreFiles)
{
    int32_t renameNum = 0;
    for (const auto &fileInfo : restoreFiles) {
        if (fileInfo.isLivePhoto) {
            if (MoveLivePhoto(fileInfo.originFilePath, fileInfo.filePath) != E_OK) {
                MEDIA_ERR_LOG("MoveFile failed. srcFile:%{public}s, destFile:%{public}s",
                    fileInfo.originFilePath.c_str(),
                    fileInfo.filePath.c_str());
                DeleteDatabaseRecord(fileInfo.filePath);
            } else {
                renameNum++;
            }
            continue;
        }
        if (!MediaFileUtils::MoveFile(fileInfo.originFilePath, fileInfo.filePath, true)) {
            MEDIA_ERR_LOG("MoveFile failed. errno:%{public}d, srcFile:%{public}s, destFile:%{public}s", errno,
                fileInfo.originFilePath.c_str(),
                fileInfo.filePath.c_str());
            DeleteDatabaseRecord(fileInfo.filePath);
        } else {
            renameNum++;
        }
    }
    return renameNum;
}

int32_t PhotoCustomRestoreOperation::MoveLivePhoto(const string &originFilePath, const string &filePath)
{
    string originVideoPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(originFilePath);
    string originExtraPathDir = MovingPhotoFileUtils::GetMovingPhotoExtraDataDir(originFilePath);
    string originExtraDataPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(originFilePath);

    string videoPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(filePath);
    string extraDataPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(filePath);
    string extraPathDir = MovingPhotoFileUtils::GetMovingPhotoExtraDataDir(filePath);
    if (!MediaFileUtils::IsFileExists(extraPathDir) && !MediaFileUtils::CreateDirectory(extraPathDir)) {
        MEDIA_WARN_LOG("Failed to create local extra data dir");
        return E_HAS_FS_ERROR;
    }

    int32_t ret = MoveFile(originFilePath, filePath);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("MoveLivePhoto: move image failed, ret:%{public}d", ret);
        MediaFileUtils::DeleteFile(filePath);
        MediaFileUtils::DeleteFile(videoPath);
        MediaFileUtils::DeleteFile(extraDataPath);
        return ret;
    }
    ret = MoveFile(originVideoPath, videoPath);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("MoveLivePhoto: move video failed, ret:%{public}d", ret);
        MediaFileUtils::DeleteFile(filePath);
        MediaFileUtils::DeleteFile(videoPath);
        MediaFileUtils::DeleteFile(extraDataPath);
        return ret;
    }
    ret = MoveFile(originExtraDataPath, extraDataPath);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("MoveLivePhoto: move extra data failed, ret:%{public}d", ret);
        MediaFileUtils::DeleteFile(filePath);
        MediaFileUtils::DeleteFile(videoPath);
        MediaFileUtils::DeleteFile(extraDataPath);
        return ret;
    }
    MediaFileUtils::DeleteDir(originExtraPathDir);

    return E_OK;
}

void PhotoCustomRestoreOperation::DeleteDatabaseRecord(const string &filePath)
{
    string deleteSql =
        "DELETE FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " + PhotoColumn::MEDIA_FILE_PATH + " = ?;";
    std::vector<NativeRdb::ValueObject> params = {filePath};
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return;
    }

    int32_t ret = rdbStore->ExecuteSql(deleteSql, params);
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK, "delete data from database failed, ret:%{public}d", ret);
}

void PhotoCustomRestoreOperation::CleanTimeoutCustomRestoreTaskDir()
{
    MEDIA_INFO_LOG("CleanTimeoutCustomRestoreTaskDir");
    auto timestampNow = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    DIR* dir = opendir(CUSTOM_RESTORE_DIR.c_str());
    if (dir == nullptr) {
        MEDIA_ERR_LOG("failed open dir, errno: %{public}d.", errno);
        return;
    }
    struct dirent *ptr = nullptr;
    while ((ptr = readdir(dir)) != nullptr) {
        if (ptr->d_type != DT_DIR || strcmp(".", ptr->d_name) == 0
            || strcmp("..", ptr->d_name) == 0) {
            continue;
        }
        std::string fileStr = CUSTOM_RESTORE_DIR + "/" + ptr->d_name;
        struct stat statInfo {};
        if (stat(fileStr.c_str(), &statInfo) != 0) {
            MEDIA_ERR_LOG("stat syscall err");
            continue;
        }
        auto dateCreated = static_cast<int64_t>(MediaFileUtils::Timespec2Millisecond(statInfo.st_ctim));
        if ((timestampNow - dateCreated) < TIMEOUT_TASK_DIR_CLEAN_INTERVAL) {
            MEDIA_ERR_LOG("no timeout file");
            continue;
        }
        CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteDir(fileStr), "clean timeout task dir failed");
    }
    closedir(dir);
}

bool PhotoCustomRestoreOperation::HasTlvFiles(const vector<string> &files)
{
    CHECK_AND_RETURN_RET_LOG(!files.empty(), false, "files is empty");
    for (const auto &file : files) {
        if (TlvUtil::ValidateTlvFile(file) == E_OK) {
            return true;
        }
    }
    return false;
}
}  // namespace OHOS::Media
// LCOV_EXCL_STOP

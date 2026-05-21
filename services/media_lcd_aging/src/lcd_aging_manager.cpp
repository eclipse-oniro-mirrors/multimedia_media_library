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
#define MLOG_TAG "Lcd_Aging"

#include "lcd_aging_manager.h"

#include <sys/stat.h>

#include "cloud_sync_manager.h"
#include "lcd_aging_utils.h"
#include "lcd_aging_worker.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_astc_stat.h"
#include "medialibrary_errno.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_tracer.h"
#include "parameters.h"
#include "photo_file_utils.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "thumbnail_service.h"
#include "thumbnail_generate_worker.h"
#include "ithumbnail_helper.h"
#include "thumbnail_source_loading.h"
#include "medialibrary_unistore_manager.h"

namespace OHOS::Media {
const std::string LCD_AGING_XML = "/data/storage/el2/base/preferences/lcd_aging.xml";
const std::string LAST_LCD_AGING_END_TIME = "last_lcd_aging_end_time";
const std::string IS_ACTIVE_LCD_AGING = "is_active_lcd_aging";
constexpr int64_t BATCH_TASK_SIZE = 10000;
constexpr size_t BATCH_AGING_SIZE = 500;
constexpr int32_t CYCLE_NUMBER = 100;
constexpr int32_t E_FINISH = 1;
constexpr int32_t E_AGING_STOP = 2;
constexpr int32_t E_AGING_INTERRUPT = 3;
constexpr uint32_t MAX_PROGRESS = 100;
const std::string MEDIA_RESTORE_FLAG = "multimedia.medialibrary.restoreFlag";
const std::string MEDIA_BACKUP_FLAG = "multimedia.medialibrary.backupFlag";
const std::string CLOUDSYNC_SWITCH_STATUS_KEY = "persist.kernel.cloudsync.switch_status";

constexpr uint32_t DELETE_LCD_FILES_WAIT_SECONDS = 2;

LcdAgingManager& LcdAgingManager::GetInstance()
{
    static LcdAgingManager instance;
    return instance;
}

int32_t LcdAgingManager::ReadyAgingLcd()
{
    MediaLibraryTracer tracer;
    tracer.Start("ReadyAgingLcd");
    this->hasAgingLcdNumber_ = 0;
    this->totalAgingLcdNumber_ = 0;
    this->lastAgingProgress_ = 0;
    this->notAgingFileIds_.clear();
    return E_OK;
}

int32_t LcdAgingManager::BatchAgingLcdFileTask(const std::atomic<bool> &shouldStop)
{
    MEDIA_INFO_LOG("BatchAgingLcdFileTask begin");
    std::lock_guard<std::mutex> lock(lcdOperationMutex_);

    int64_t taskSize = -1;
    int32_t ret = InitAgingTask(taskSize);
    if (ret != E_OK) {
        HandleAfterAgingProgress(ret);
        FinishAgingTask();
        return ret;
    }

    ret = ExecuteAgingLoop(taskSize, shouldStop);

    HandleAfterAgingProgress(ret);
    FinishAgingTask();
    MEDIA_INFO_LOG("BatchAgingLcdFileTask end");
    return E_OK;
}

int32_t LcdAgingManager::InitAgingTask(int64_t &taskSize)
{
    this->ReadyAgingLcd();

    int32_t ret = this->GetNeedAgingLcdSize(taskSize);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to GetNeedAgingLcdSize, ret: %{public}d", ret);
        return ret;
    }
    this->totalAgingLcdNumber_ = taskSize;
    // 查询taskSize耗时，提前给应用上报一次进度
    HandleAgingProgress();
    return E_OK;
}

int32_t LcdAgingManager::ExecuteAgingLoop(int64_t taskSize, const std::atomic<bool> &shouldStop)
{
    int32_t ret = E_ERR;
    int32_t cycleNumber = 0;
    bool hasTrashedData = true;

    while (taskSize > 0) {
        CHECK_AND_BREAK_ERR_LOG(cycleNumber++ <= CYCLE_NUMBER, "cycleNumber exceeds the limit");
        ret = CheckLcdAgingStatus(shouldStop);
        CHECK_AND_BREAK(ret == E_OK);

        int32_t currentBatchSize = static_cast<int32_t>(taskSize > BATCH_TASK_SIZE ? BATCH_TASK_SIZE : taskSize);
        ret = ExecuteSingleBatch(currentBatchSize, hasTrashedData, shouldStop);
        CHECK_AND_PRINT_LOG(ret == E_OK, "failed to BatchAgingLcdFile, ret: %{public}d", ret);

        bool shouldBreak = (ret == E_NO_QUERY_DATA || ret == E_AGING_STOP || ret == E_AGING_INTERRUPT);
        CHECK_AND_BREAK_INFO_LOG(!shouldBreak, "break aging lcd task, ret: %{public}d", ret);

        taskSize = this->totalAgingLcdNumber_ - this->hasAgingLcdNumber_;
        MEDIA_INFO_LOG("batch aging lcdFile, currentBatchSize: %{public}d, hasAgingLcdNumber: %{public}" PRId64
            ", taskSize: %{public}" PRId64 ", cycleNumber: %{public}d",
            currentBatchSize, this->hasAgingLcdNumber_.load(), taskSize, cycleNumber);
    }
    return ret;
}

int32_t LcdAgingManager::ExecuteSingleBatch(const int32_t batchSize, bool &hasTrashedData,
    const std::atomic<bool> &shouldStop)
{
    int32_t ret = E_NO_QUERY_DATA;
    const std::vector<BatchAgingLcdFileFunc> batchAgingFuncs = {
        &LcdAgingManager::BatchAgingLcdFileTrashed,
        &LcdAgingManager::BatchAgingLcdFileNotTrashed
    };

    for (size_t i = 0; i < batchAgingFuncs.size(); ++i) {
        bool isContinue = (i == 0 && !hasTrashedData);
        CHECK_AND_CONTINUE(!isContinue);

        ret = (this->*batchAgingFuncs[i])(batchSize, shouldStop);
        CHECK_AND_BREAK(ret == E_NO_QUERY_DATA);
        // when query no data with trashed, hasTrashedData = false
        if (i == 0) {
            hasTrashedData = false;
            MEDIA_INFO_LOG("No more trashed data to age, will try not trashed data next");
        }
    }
    return ret;
}

int32_t LcdAgingManager::BatchAgingLcdFileTrashed(const int32_t size, const std::atomic<bool> &shouldStop)
{
    MediaLibraryTracer tracer;
    tracer.Start("BatchAgingLcdFileTrashed");
    std::vector<LcdAgingFileInfo> lcdAgingFileInfoList;
    int32_t ret = this->lcdAgingDao_.QueryAgingLcdDataTrashed(size, this->notAgingFileIds_, lcdAgingFileInfoList);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "Failed to QueryAgingLcdDataTrashed, ret: %{public}d", ret);
    CHECK_AND_RETURN_RET_LOG(!lcdAgingFileInfoList.empty(), E_NO_QUERY_DATA, "no aging lcd data (trashed)");
    ret = this->DoBatchAgingLcdFile(lcdAgingFileInfoList, shouldStop);
    CHECK_AND_PRINT_LOG(ret == E_OK, "Failed to DoBatchAgingLcdFile, ret: %{public}d", ret);
    return ret;
}

int32_t LcdAgingManager::BatchAgingLcdFileNotTrashed(const int32_t size, const std::atomic<bool> &shouldStop)
{
    MediaLibraryTracer tracer;
    tracer.Start("BatchAgingLcdFileNotTrashed");
    std::vector<LcdAgingFileInfo> lcdAgingFileInfoList;
    int32_t ret = this->lcdAgingDao_.QueryAgingLcdDataNotTrashed(size, this->notAgingFileIds_, lcdAgingFileInfoList);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "Failed to QueryAgingLcdDataNotTrashed, ret: %{public}d", ret);
    CHECK_AND_RETURN_RET_LOG(!lcdAgingFileInfoList.empty(), E_NO_QUERY_DATA, "no aging lcd data (not trashed)");
    ret = this->DoBatchAgingLcdFile(lcdAgingFileInfoList, shouldStop);
    CHECK_AND_PRINT_LOG(ret == E_OK, "Failed to DoBatchAgingLcdFile, ret: %{public}d", ret);
    return ret;
}

int32_t LcdAgingManager::DoBatchAgingLcdFile(const std::vector<LcdAgingFileInfo> &lcdAgingFileInfoList,
    const std::atomic<bool> &shouldStop)
{
    MediaLibraryTracer tracer;
    tracer.Start("DoBatchAgingLcdFile");
    CHECK_AND_RETURN_RET_LOG(!lcdAgingFileInfoList.empty(), E_ERR, "lcdAgingFileInfoList is empty");

    size_t totalSize = lcdAgingFileInfoList.size();
    int32_t ret = E_OK;

    for (size_t offset = 0; offset < totalSize; offset += BATCH_AGING_SIZE) {
        ret = CheckLcdAgingStatus(shouldStop);
        CHECK_AND_RETURN_RET(ret == E_OK, ret);

        size_t batchSize = std::min(BATCH_AGING_SIZE, totalSize - offset);

        std::vector<LcdAgingFileInfo> batch(
            lcdAgingFileInfoList.begin() + offset,
            lcdAgingFileInfoList.begin() + offset + batchSize
        );

        int64_t batchSuccessSize = 0;
        std::vector<std::string> failFileIds;
        ret = this->DoBatchAgingLcdFileInternal(batch, batchSuccessSize, failFileIds);
        if (!failFileIds.empty()) {
            this->notAgingFileIds_.insert(this->notAgingFileIds_.end(), failFileIds.begin(), failFileIds.end());
        }
        CHECK_AND_CONTINUE_ERR_LOG(ret == E_OK, "Failed to DoBatchAgingLcdFileInternal, ret: %{public}d", ret);

        this->hasAgingLcdNumber_ += batchSuccessSize;
        HandleAgingProgress();

        MEDIA_INFO_LOG("DoBatchAgingLcdFile progress: %{public}zu/%{public}zu, batchSuccessSize: %{public}" PRId64,
            offset + batchSize, totalSize, batchSuccessSize);
    }

    return E_OK;
}

int32_t LcdAgingManager::DoBatchAgingLcdFileInternal(const std::vector<LcdAgingFileInfo> &lcdAgingFileInfoList,
    int64_t &agingSuccessSize, std::vector<std::string> &failFileIds)
{
    MediaLibraryTracer tracer;
    tracer.Start("DoBatchAgingLcdFileInternal");
    agingSuccessSize = 0;
    CHECK_AND_RETURN_RET_LOG(!lcdAgingFileInfoList.empty(), E_ERR, "lcdAgingFileInfoList is empty");

    std::vector<std::string> fileIds = this->GetFileIdFromAgingFiles(lcdAgingFileInfoList);
    int32_t ret = this->lcdAgingDao_.SetLcdNotDownloadStatus(fileIds);
    CHECK_AND_PRINT_LOG(ret == E_OK, "Failed to SetLcdNotDownloadStatus, ret: %{public}d", ret);
    ret = this->lcdAgingDao_.UpdateLcdFileSize(lcdAgingFileInfoList);
    CHECK_AND_PRINT_LOG(ret == E_OK, "Failed to UpdateLcdFileSize, ret: %{public}d", ret);

    std::vector<DentryFileInfo> dentryFileInfos = LcdAgingUtils::ConvertAgingFileToDentryFile(lcdAgingFileInfoList);
    std::vector<std::string> failCloudIds;
    MEDIA_INFO_LOG("Begin to BatchDentryFileInsert");
    ret = CloudSyncManager::GetInstance().BatchDentryFileInsert(dentryFileInfos, failCloudIds);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to insert dentry file, ret: %{public}d", ret);
        failFileIds = std::move(fileIds);
        ret = this->lcdAgingDao_.RevertToLcdDownloadStatus(failFileIds);
        CHECK_AND_PRINT_LOG(ret == E_OK, "Failed to RevertToLcdDownloadStatus, ret: %{public}d", ret);
        return E_ERR;
    }
    MEDIA_INFO_LOG("End to BatchDentryFileInsert");

    failFileIds = this->GetFailedFileIds(lcdAgingFileInfoList, failCloudIds);
    if (!failFileIds.empty()) {
        this->lcdAgingDao_.RevertToLcdDownloadStatus(failFileIds);
    }
    agingSuccessSize = static_cast<int64_t>(dentryFileInfos.size()) - static_cast<int64_t>(failCloudIds.size());
    CHECK_AND_EXECUTE(agingSuccessSize >= 0, agingSuccessSize = 0);

    // 异步删除本地LCD文件
    this->AsyncDeleteLcdFiles(lcdAgingFileInfoList, failFileIds);
    return E_OK;
}

std::vector<std::string> LcdAgingManager::GetFailedFileIds(const std::vector<LcdAgingFileInfo> &agingFileInfos,
    const std::vector<std::string> &failCloudIds)
{
    std::vector<std::string> failFileIds;
    for (auto &agingFileInfo : agingFileInfos) {
        if (std::find(failCloudIds.begin(), failCloudIds.end(), agingFileInfo.cloudId) != failCloudIds.end()) {
            failFileIds.emplace_back(std::to_string(agingFileInfo.fileId));
            MEDIA_ERR_LOG("Failed to insert dentry, cloudId: %{public}s, fileId: %{public}d",
                agingFileInfo.cloudId.c_str(), agingFileInfo.fileId);
        }
    }
    return failFileIds;
}

std::vector<std::string> LcdAgingManager::GetFileIdFromAgingFiles(const std::vector<LcdAgingFileInfo> &agingFileInfos)
{
    std::vector<std::string> fileIds;
    for (auto &agingFileInfo : agingFileInfos) {
        fileIds.emplace_back(std::to_string(agingFileInfo.fileId));
    }
    return fileIds;
}

int32_t LcdAgingManager::DeleteLocalFile(const std::string &localPath)
{
    CHECK_AND_RETURN_RET_LOG(!localPath.empty(), E_ERR, "path is empty");
    if (!MediaFileUtils::IsFileExists(localPath)) {
        MEDIA_WARN_LOG("localPath not exist, path: %{public}s", MediaFileUtils::DesensitizePath(localPath).c_str());
        return E_ERR;
    }
    if (!MediaFileUtils::DeleteFile(localPath)) {
        // 删除文件失败，兜底重试一次
        CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteFile(localPath),
            "Failed to delete localPath, path: %{public}s", MediaFileUtils::DesensitizePath(localPath).c_str());
        return E_ERR;
    }
    return E_OK;
}

void LcdAgingManager::UpdateLastLcdAgingEndTime(const int64_t lastLcdAgingEndTime)
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(LCD_AGING_XML, errCode);
    CHECK_AND_RETURN_LOG(prefs, "Get preferences error: %{public}d", errCode);
    prefs->PutLong(LAST_LCD_AGING_END_TIME, lastLcdAgingEndTime);
    prefs->FlushSync();
}

int32_t LcdAgingManager::GetNeedAgingLcdSize(int64_t &taskSize)
{
    int64_t lcdCurrentNumber = -1;
    int32_t ret = lcdAgingDao_.GetCurrentNumberOfLcd(lcdCurrentNumber);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "Failed to GetCurrentNumberOfLcd, ret: %{public}d", ret);
    CHECK_AND_RETURN_RET_LOG(lcdCurrentNumber >= LcdAgingUtils::GetMaxThresholdOfLcd(), E_NO_QUERY_DATA,
        "lcdCurrentNumber: %{public}" PRId64, lcdCurrentNumber);

    int64_t expectAgingSize = lcdCurrentNumber - LcdAgingUtils::GetScaleThresholdOfLcd();
    CHECK_AND_RETURN_RET_LOG(expectAgingSize > 0, E_NO_QUERY_DATA,
        "expectAgingSize: %{public}" PRId64, expectAgingSize);

    int64_t actualAgingSize = lcdAgingDao_.GetAgingLcdCount();
    CHECK_AND_RETURN_RET_LOG(actualAgingSize > 0, E_NO_QUERY_DATA,
        "actualAgingSize: %{public}" PRId64, actualAgingSize);

    taskSize = std::min(expectAgingSize, actualAgingSize);
    MEDIA_INFO_LOG("lcdCurrentNumber: %{public}" PRId64 ", actualAgingSize: %{public}" PRId64
        ", taskSize: %{public}" PRId64, lcdCurrentNumber, actualAgingSize, taskSize);
    return E_OK;
}

int32_t LcdAgingManager::FinishAgingTask()
{
    MEDIA_INFO_LOG("start FinishAgingTask");
    // 打点上报

    // 清理缓存
    this->hasAgingLcdNumber_ = 0;
    this->totalAgingLcdNumber_ = 0;
    this->lastAgingProgress_ = 0;
    this->notAgingFileIds_.clear();
    MEDIA_INFO_LOG("end FinishAgingTask");
    return E_FINISH;
}

void LcdAgingManager::DelayLcdAgingTime()
{
    int64_t currentTime = MediaFileUtils::UTCTimeSeconds();
    this->UpdateLastLcdAgingEndTime(currentTime);
}

int32_t LcdAgingManager::GenerateLcdWithLocal(const LcdAgingFileInfo &agingFileInfo)
{
    ThumbnailData thumbnailData = ConvertLcdAgingFileInfoToThumbnailData(agingFileInfo);
    auto createLcdBackgroundTask = [](std::shared_ptr<ThumbnailTaskData> &data) {
        CHECK_AND_RETURN_LOG(data != nullptr, "CreateLcd failed, data is null");
        auto &thumbnailData = data->thumbnailData_;
        thumbnailData.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
        IThumbnailHelper::CreateLcd(data);
    };
    thumbnailData.genThumbScene = GenThumbScene::NO_LCD_AND_GEN_IT_BACKGROUND;
    ThumbRdbOpt opts;
    opts.table = PhotoColumn::PHOTOS_TABLE;
    opts.store = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    opts.row = thumbnailData.id;
    IThumbnailHelper::AddThumbnailGenerateTask(createLcdBackgroundTask,
        opts, thumbnailData, ThumbnailTaskType::BACKGROUND, ThumbnailTaskPriority::LOW);
    return E_OK;
}

ThumbnailData LcdAgingManager::ConvertLcdAgingFileInfoToThumbnailData
    (const LcdAgingFileInfo &agingFileInfo)
{
    ThumbnailData thumbnailData;
    thumbnailData.id = std::to_string(agingFileInfo.fileId);
    thumbnailData.path = agingFileInfo.path;
    thumbnailData.mediaType = agingFileInfo.mediaType;
    thumbnailData.orientation = agingFileInfo.orientation;
    thumbnailData.exifRotate = agingFileInfo.exifRotate;
    thumbnailData.thumbnailReady = agingFileInfo.thumbnailReady;
    thumbnailData.dateModified = std::to_string(agingFileInfo.dateModified);
    thumbnailData.isLocalFile = true;
    return thumbnailData;
}

std::mutex& LcdAgingManager::GetLcdOperationMutex()
{
    return lcdOperationMutex_;
}

void LcdAgingManager::AsyncDeleteLcdFiles(const std::vector<LcdAgingFileInfo> &lcdAgingFileInfoList,
    const std::vector<std::string> &failFileIds)
{
    bool needStartWorker = false;
    {
        std::lock_guard<std::mutex> lock(deleteLcdFilesTaskMutex_);
        deleteLcdFilesTaskQueue_.push({lcdAgingFileInfoList, failFileIds});
        if (!isDeleteLcdFilesWorkerRunning_) {
            isDeleteLcdFilesWorkerRunning_ = true;
            needStartWorker = true;
        }
    }
    if (needStartWorker) {
        std::thread([this]() { ProcessDeleteLcdFilesTasks(); }).detach();
    } else {
        deleteLcdFilesTaskCv_.notify_one();
    }
}

void LcdAgingManager::ProcessDeleteLcdFilesTasks()
{
    MEDIA_INFO_LOG("Start ProcessDeleteLcdFilesTasks thread");
    while (isDeleteLcdFilesWorkerRunning_) {
        DeleteLcdFilesTask task;
        {
            std::unique_lock<std::mutex> lock(deleteLcdFilesTaskMutex_);
            if (deleteLcdFilesTaskQueue_.empty()) {
                deleteLcdFilesTaskCv_.wait_for(lock, std::chrono::seconds(DELETE_LCD_FILES_WAIT_SECONDS));
                if (deleteLcdFilesTaskQueue_.empty()) {
                    isDeleteLcdFilesWorkerRunning_ = false;
                    MEDIA_INFO_LOG("DeleteLcdFiles worker exiting, queue empty for %{public}u seconds",
                        DELETE_LCD_FILES_WAIT_SECONDS);
                    return;
                }
            }
            task = std::move(deleteLcdFilesTaskQueue_.front());
            deleteLcdFilesTaskQueue_.pop();
        }
        ExecuteDeleteLcdFiles(task);
    }
    MEDIA_INFO_LOG("End ProcessDeleteLcdFilesTasks thread");
}

void LcdAgingManager::ExecuteDeleteLcdFiles(const DeleteLcdFilesTask &task)
{
    MediaLibraryTracer tracer;
    tracer.Start("AsyncDeleteLocalLcdFiles");
    std::vector<std::pair<std::string, std::string>> thumbnailSizeUpdateList;
    for (const auto &agingFileInfo : task.agingFileInfos) {
        bool isValid = std::find(task.failFileIds.begin(), task.failFileIds.end(),
            std::to_string(agingFileInfo.fileId)) == task.failFileIds.end();
        CHECK_AND_CONTINUE(isValid);

        CHECK_AND_EXECUTE(!agingFileInfo.hasExThumbnail,
            DeleteLocalFile(agingFileInfo.localLcdExPath));
        DeleteLocalFile(agingFileInfo.localLcdPath);
        thumbnailSizeUpdateList.emplace_back(std::to_string(agingFileInfo.fileId), agingFileInfo.path);
    }
    CHECK_AND_EXECUTE(thumbnailSizeUpdateList.empty(),
        MediaLibraryPhotoOperations::BatchStoreThumbnailSize(thumbnailSizeUpdateList));
    MEDIA_INFO_LOG("AsyncDeleteLocalLcdFiles completed, count: %{public}zu", task.agingFileInfos.size());
}

int32_t LcdAgingManager::StartDeepOptimizeSpace(const sptr<IRemoteObject> &clientRemote,
    const sptr<IRemoteObject> &callbackRemote)
{
    CHECK_AND_RETURN_RET_LOG(clientRemote != nullptr, E_ERR, "Client remote is null");
    int32_t ret = CheckLcdAgingStatus(false);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_OPERATION_NOT_SUPPORT, "CheckLcdAgingStatus, ret: %{public}d", ret);

    // 持久化主动老化LCD标记
    SetIsActiveLcdAging(true);
    
    ret = LcdAgingWorker::GetInstance().StartDeepOptimizeSpace(clientRemote, callbackRemote);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to start worker, ret: %{public}d", ret);
        return ret;
    }

    MEDIA_INFO_LOG("Deep optimize space task started successfully");
    return E_OK;
}

int32_t LcdAgingManager::StopDeepOptimizeSpace()
{
    MEDIA_INFO_LOG("Stopping deep optimize space task");
    int32_t ret = LcdAgingWorker::GetInstance().StopDeepOptimizeSpace();
    MEDIA_INFO_LOG("Task stopped, ret: %{public}d", ret);
    return ret;
}

int32_t LcdAgingManager::CheckLcdAgingStatus(const std::atomic<bool> &shouldStop)
{
    if (shouldStop.load()) {
        MEDIA_INFO_LOG("shouldStop");
        return E_AGING_STOP;
    }
    bool isClone = !MedialibrarySubscriber::IsBackgroundTaskAllowed();
    bool isBackUp = system::GetIntParameter(MEDIA_BACKUP_FLAG, 0) != 0;
    bool isRestore = system::GetIntParameter(MEDIA_RESTORE_FLAG, 0) != 0;
    bool isCloudCleaning = system::GetIntParameter(CLOUDSYNC_SWITCH_STATUS_KEY, 0) != 0;
    // wyftodo 正在标记时刻

    bool shouldInterrupt = isClone || isBackUp || isRestore || isCloudCleaning;

    if (shouldInterrupt) {
        MEDIA_INFO_LOG("shouldInterrupt, status: %{public}d, %{public}d, %{public}d, %{public}d,",
            isClone, isBackUp, isRestore, isCloudCleaning);
        return E_AGING_INTERRUPT;
    }
    return E_OK;
}

void LcdAgingManager::HandleAgingProgress()
{
    CHECK_AND_RETURN_LOG(this->totalAgingLcdNumber_ > 0,
        "invalid totalAgingLcdNumber: %{public}" PRId64, this->totalAgingLcdNumber_.load());
    
    uint32_t progress = static_cast<uint32_t>(
        (static_cast<double>(this->hasAgingLcdNumber_) / static_cast<double>(this->totalAgingLcdNumber_)) * 100.0);
    CHECK_AND_EXECUTE(progress <= MAX_PROGRESS, progress = MAX_PROGRESS);
    CHECK_AND_EXECUTE(progress >= this->lastAgingProgress_, progress = this->lastAgingProgress_);
    this->lastAgingProgress_ = progress;

    auto state = (progress == MAX_PROGRESS) ? DeepOptimizeSpaceState::COMPLETED : DeepOptimizeSpaceState::RUNNING;
    LcdAgingWorker::GetInstance().NotifyProgress(state, progress);
}

void LcdAgingManager::HandleAfterAgingProgress(const int32_t errorCode)
{
    switch (errorCode) {
        case E_NO_QUERY_DATA:
            if (this->lastAgingProgress_ < MAX_PROGRESS) {
                LcdAgingWorker::GetInstance().NotifyProgress(DeepOptimizeSpaceState::COMPLETED, MAX_PROGRESS);
            }
            break;
        case E_AGING_STOP:
            LcdAgingWorker::GetInstance().NotifyProgress(DeepOptimizeSpaceState::STOPPED, this->lastAgingProgress_);
            break;
        case E_AGING_INTERRUPT:
            LcdAgingWorker::GetInstance().NotifyProgress(DeepOptimizeSpaceState::INTERRUPTED, this->lastAgingProgress_);
            break;
        default:
            if (this->lastAgingProgress_ < MAX_PROGRESS) {
                LcdAgingWorker::GetInstance().NotifyProgress(DeepOptimizeSpaceState::FAILED, this->lastAgingProgress_);
            }
            break;
    }
}

int32_t LcdAgingManager::AnalysisRemoveCloudLcd(const std::vector<int64_t> &fileIds)
{
    MediaLibraryTracer tracer;
    tracer.Start("AnalysisRemoveCloudLcd");
    CHECK_AND_RETURN_RET_INFO_LOG(this->GetIsActiveLcdAging(), E_OK, "isActiveLcdAging is false");
    std::vector<LcdAgingFileInfo> lcdAgingFileInfoList;
    int32_t ret = this->lcdAgingDao_.QueryAgingLcdDataByFileIds(fileIds, lcdAgingFileInfoList);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "Failed to query aging LCD data, ret: %{public}d", ret);
    CHECK_AND_RETURN_RET_INFO_LOG(!lcdAgingFileInfoList.empty(), E_OK, "no Remove Cloud LCD data found");
    int64_t agingSuccessSize = 0;
    std::vector<std::string> failFileIds;
    ret = this->DoBatchAgingLcdFileInternal(lcdAgingFileInfoList, agingSuccessSize, failFileIds);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "Failed to DoBatchAgingLcdFileInternal, ret: %{public}d", ret);
    return E_OK;
}

bool LcdAgingManager::LoadIsActiveLcdAgingFromPrefs()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(LCD_AGING_XML, errCode);
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, false, "Get preferences error: %{public}d", errCode);
    return prefs->GetBool(IS_ACTIVE_LCD_AGING, false);
}

void LcdAgingManager::SaveIsActiveLcdAgingToPrefs(bool isActive)
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(LCD_AGING_XML, errCode);
    CHECK_AND_RETURN_LOG(prefs != nullptr, "Get preferences error: %{public}d", errCode);
    prefs->PutBool(IS_ACTIVE_LCD_AGING, isActive);
    prefs->FlushSync();
    MEDIA_INFO_LOG("SaveIsActiveLcdAgingToPrefs: %{public}d", isActive);
}

bool LcdAgingManager::GetIsActiveLcdAging()
{
    int32_t value = isActiveLcdAging_.load();
    if (value == -1) {
        value = LoadIsActiveLcdAgingFromPrefs() ? 1 : 0;
        isActiveLcdAging_.store(value);
        MEDIA_INFO_LOG("GetIsActiveLcdAging first load: %{public}d", value);
    }
    MEDIA_DEBUG_LOG("get isActiveLcdAging_: %{public}d", value);
    return value == 1;
}

int32_t LcdAgingManager::SetIsActiveLcdAging(bool isActive)
{
    SaveIsActiveLcdAgingToPrefs(isActive);
    isActiveLcdAging_.store(isActive ? 1 : 0);
    MEDIA_INFO_LOG("SetIsActiveLcdAging: %{public}d", isActive ? 1 : 0);
    return E_OK;
}
}  // namespace OHOS::Media
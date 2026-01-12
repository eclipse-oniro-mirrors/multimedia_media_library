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

#include "thumbnail_ready_manager.h"

#include <thread>

#include "datashare_abs_result_set.h"
#include "datashare_helper.h"

#include "dfx_utils.h"
#include "ithumbnail_helper.h"
#include "media_log.h"
#include "media_file_uri.h"
#include "medialibrary_errno.h"
#include "medialibrary_notify.h"
#include "medialibrary_tracer.h"
#include "medialibrary_unistore_manager.h"
#include "thumbnail_const.h"
#include "thumbnail_file_utils.h"
#include "thumbnail_generate_worker.h"
#include "thumbnail_generate_worker_manager.h"
#include "thumbnail_generation_post_process.h"
#include "thumbnail_rdb_utils.h"
#include "thumbnail_source_loading.h"
#include "thumbnail_utils.h"

using namespace std;
using namespace OHOS::FileManagement::CloudSync;
using namespace OHOS::NativeRdb;
namespace OHOS {
namespace Media {
const int TIME_MILLI_SECONDS = 5000;
constexpr int32_t THUMB_BATCH_WAIT_TIME = 15 * 1000;
constexpr int32_t RETRY_DOWNLOAD_THUMB_LIMIT = 3;
constexpr int64_t THUMB_TEN_MINUTES_WAIT_TIME = 10 * 60 * 1000;
constexpr int32_t THUMB_INVALID_VALUE = -1;
constexpr int32_t THUMB_PROCESS_WAIT_TIME = 30;
const int THREADNUM = 1;

void ThumbnailCloudDownloadCallback::OnDownloadProcess(const DownloadProgressObj &progress)
{
    auto thumbReadyTaskData = ThumbnailReadyManager::GetInstance().GetAstcBatchTaskInfo(pid_);
    CHECK_AND_RETURN_LOG(thumbReadyTaskData != nullptr, "GetAstcBatchTaskInfo failed");
    CHECK_AND_RETURN_LOG(thumbReadyTaskData->downloadId == progress.downloadId, "downloadId is not equal");
    if (progress.state == DownloadProgressObj::Status::COMPLETED) {
        ThumbnailReadyManager::GetInstance().CreateAstcAfterDownloadThumbOnDemand(progress.path, thumbReadyTaskData);
    } else if (progress.state == DownloadProgressObj::Status::FAILED) {
        MEDIA_ERR_LOG("download thumbnail file failed, file path is %{public}s", progress.path.c_str());
        if (progress.downloadErrorType == DownloadProgressObj::DownloadErrorType::CONTENT_NOT_FOUND) {
            ThumbnailReadyManager::GetInstance().RecordNotFoundThumbnail(progress.path, thumbReadyTaskData);
        }
    }
    if (progress.batchState == DownloadProgressObj::Status::COMPLETED ||
        progress.batchState == DownloadProgressObj::Status::STOPPED ||
        progress.batchState == DownloadProgressObj::Status::FAILED) {
        MEDIA_INFO_LOG("download thumbnail file, progress.batchState is %{public}d", progress.batchState);
        ThumbnailReadyManager::GetInstance().SetDownloadEnd(pid_);
        std::lock_guard<std::mutex> cvLock(thumbReadyTaskData->cvMutex);
        thumbReadyTaskData->pendingTasks--;
        if (thumbReadyTaskData->pendingTasks <= 0) {
            thumbReadyTaskData->cv.notify_all();
        }
    }
}

ThumbnailReadyManager::~ThumbnailReadyManager()
{
    processRequestMap_.Clear();
}

ThumbnailReadyManager& ThumbnailReadyManager::GetInstance()
{
    static ThumbnailReadyManager instance;
    return instance;
}

int32_t ThumbnailReadyManager::GetCurrentTemperatureLevel()
{
    return currentTemperatureLevel_;
}

std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo> ThumbnailReadyManager::GetAstcBatchTaskInfo(const pid_t pid)
{
    std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo> taskInfo;
    processRequestMap_.Find(pid, taskInfo);
    return taskInfo;
}

void ThumbnailReadyManager::SetDownloadEnd(pid_t pid)
{
    auto taskInfo = GetAstcBatchTaskInfo(pid);
    if (taskInfo != nullptr) {
        taskInfo->isDownloadEnd = true;
    }
}

bool ThumbnailReadyManager::IsNeedExecuteTask(int32_t requestId, pid_t pid)
{
    auto taskInfo = make_shared<ThumbnailReadyManager::AstcBatchTaskInfo>();
    CHECK_AND_RETURN_RET_LOG(processRequestMap_.Find(pid, taskInfo), false,
        "pid:%{public}d not found", pid);
    CHECK_AND_RETURN_RET_LOG(taskInfo != nullptr, false, "pid:%{public}d taskInfo is null", pid);
    CHECK_AND_RETURN_RET_LOG(taskInfo->requestId == requestId, false,
        "pid:%{public}d requestId:%{public}d not match", pid, requestId);
    return true;
}

bool ThumbnailReadyManager::IsNeedExecuteTask(int32_t requestId, pid_t pid,
    std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo> &taskInfo)
{
    CHECK_AND_RETURN_RET_LOG(processRequestMap_.Find(pid, taskInfo), false,
        "pid:%{public}d not found", pid);
    CHECK_AND_RETURN_RET_LOG(taskInfo != nullptr, false, "pid:%{public}d taskInfo is null", pid);
    CHECK_AND_RETURN_RET_LOG(taskInfo->requestId == requestId, false,
        "pid:%{public}d requestId:%{public}d not match", pid, requestId);
    return true;
}

void ThumbnailReadyManager::ExecuteCreateThumbnailTask(std::shared_ptr<ThumbnailTaskData> &taskData,
    int32_t requestId, pid_t pid)
{
    CHECK_AND_RETURN_LOG(taskData != nullptr, "data is null");
    std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo> taskInfo;
    CHECK_AND_RETURN(IsNeedExecuteTask(requestId, pid, taskInfo));
    CHECK_AND_RETURN_WARN_LOG(!taskInfo->isCancel, "download thumbnail is canceled");
    taskData->thumbnailData_.genThumbScene = GenThumbScene::NEED_MORE_THUMB_READY;
    if (taskData->thumbnailData_.isLocalFile) {
        ThumbnailUtils::RecordStartGenerateStats(taskData->thumbnailData_.stats, GenerateScene::FOREGROUND,
            LoadSourceType::LOCAL_PHOTO);
        taskData->thumbnailData_.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
        IThumbnailHelper::CreateThumbnail(taskData);
    } else {
        taskData->thumbnailData_.needGenerateExThumbnail = false;
        taskData->thumbnailData_.loaderOpts.loadingStates =
            ThumbnailUtils::IsExCloudThumbnail(taskData->thumbnailData_) ?
            SourceLoader::CLOUD_LCD_SOURCE_LOADING_STATES : SourceLoader::CLOUD_SOURCE_LOADING_STATES;
        if (ThumbnailUtils::IsExCloudThumbnail(taskData->thumbnailData_)) {
            IThumbnailHelper::CreateAstcEx(taskData);
        } else {
            IThumbnailHelper::CreateAstc(taskData);
        }
    }
    std::lock_guard<std::mutex> cvLock(taskInfo->cvMutex);
    taskInfo->pendingTasks--;
    if (taskInfo->pendingTasks <= 0) {
        taskInfo->cv.notify_all();
    }
}

void ThumbnailReadyManager::AddQueryNoAstcRulesOnlyLocal(NativeRdb::RdbPredicates &rdbPredicate)
{
    rdbPredicate.BeginWrap();
    rdbPredicate.EqualTo(PhotoColumn::PHOTO_POSITION, "1");
    rdbPredicate.Or();
    rdbPredicate.EqualTo(PhotoColumn::PHOTO_POSITION, "3");
    rdbPredicate.Or();
    rdbPredicate.BeginWrap();
    rdbPredicate.EqualTo(PhotoColumn::PHOTO_POSITION, "2");
    rdbPredicate.And();
    rdbPredicate.BeginWrap();
    rdbPredicate.EqualTo(PhotoColumn::PHOTO_THUMB_STATUS, "0");
    rdbPredicate.Or();
    rdbPredicate.EqualTo(PhotoColumn::PHOTO_THUMB_STATUS, "2");
    rdbPredicate.EndWrap();
    rdbPredicate.EndWrap();
    rdbPredicate.EndWrap();
}

bool ThumbnailReadyManager::QueryNoAstcInfosOnDemand(ThumbRdbOpt &opts,
    std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo> taskInfo,
    NativeRdb::RdbPredicates rdbPredicate, int &err)
{
    MediaLibraryTracer tracer;
    tracer.Start("QueryNoAstcInfosOnDemand");
    vector<string> column = {
        MEDIA_DATA_DB_ID, MEDIA_DATA_DB_FILE_PATH, MEDIA_DATA_DB_HEIGHT, MEDIA_DATA_DB_WIDTH,
        MEDIA_DATA_DB_POSITION, MEDIA_DATA_DB_MEDIA_TYPE, MEDIA_DATA_DB_DATE_ADDED, MEDIA_DATA_DB_NAME,
        MEDIA_DATA_DB_ORIENTATION, PhotoColumn::PHOTO_EXIF_ROTATE, MEDIA_DATA_DB_DATE_TAKEN,
        MEDIA_DATA_DB_DATE_MODIFIED, PhotoColumn::PHOTO_THUMB_STATUS,
    };

    rdbPredicate.EqualTo(PhotoColumn::PHOTO_THUMBNAIL_READY, "0");
    if (!ThumbnailUtils::IsMobileNetworkEnabled() || timeoutCount_ >= RETRY_DOWNLOAD_THUMB_LIMIT) {
        CHECK_AND_PRINT_LOG(timeoutCount_ < RETRY_DOWNLOAD_THUMB_LIMIT,
            "timeoutCount_:%{public}d is over three, just query local", timeoutCount_.load());
        auto currentMilliSecond = MediaFileUtils::UTCTimeMilliSeconds();
        if (currentMilliSecond - lastTimeoutTime_ > THUMB_TEN_MINUTES_WAIT_TIME) {
            MEDIA_INFO_LOG("Download thumbnail timeout 10 minutes, reset timeout count.");
            timeoutCount_.store(0);
            lastTimeoutTime_ = currentMilliSecond;
        } else {
            AddQueryNoAstcRulesOnlyLocal(rdbPredicate);
        }
    }
    rdbPredicate.EqualTo(MEDIA_DATA_DB_TIME_PENDING, "0");
    rdbPredicate.EqualTo(PhotoColumn::PHOTO_CLEAN_FLAG, "0");
    rdbPredicate.EqualTo(MEDIA_DATA_DB_DATE_TRASHED, "0");
    rdbPredicate.EqualTo(COMPAT_HIDDEN, "0");
    rdbPredicate.Limit(THUMBNAIL_GENERATE_BATCH_COUNT);

    vector<ThumbnailData> infos;
    CHECK_AND_RETURN_RET_LOG(ThumbnailRdbUtils::QueryThumbnailDataInfos(opts.store, rdbPredicate, column, infos, err),
        false, "QueryThumbnailDataInfos failed, err:%{public}d", err);
    taskInfo->localInfos.clear();
    taskInfo->cloudPaths.clear();
    for (auto &info : infos) {
        if (info.isLocalFile || (info.position == static_cast<int32_t>(PhotoPositionType::CLOUD) &&
            (info.thumbnailStatus == static_cast<int32_t>(PhotoThumbStatusType::DOWNLOADED) ||
            info.thumbnailStatus == static_cast<int32_t>(PhotoThumbStatusType::ONLY_LCD_DOWNLOADED)))) {
            taskInfo->localInfos.emplace_back(info);
        } else {
            std::string uri = "";
            uri = MediaFileUri::GetPhotoUri(info.id, info.path, info.displayName);
            taskInfo->cloudPaths.emplace_back(uri);
            taskInfo->downloadThumbMap[uri] = info;
        }
    }
    return true;
}

void ThumbnailReadyManager::CreateAstcAfterDownloadThumbOnDemand(const std::string &path,
    std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo> thumbReadyTaskData)
{
    MediaLibraryTracer tracer;
    tracer.Start("CreateAstcAfterDownloadThumbOnDemand");
    CHECK_AND_RETURN_LOG(thumbReadyTaskData != nullptr, "taskInfo is null");
    CHECK_AND_RETURN_LOG(thumbReadyTaskData->downloadThumbMap.find(path) != thumbReadyTaskData->downloadThumbMap.end(),
        "downloaded thumbnail path not found");
    auto data = thumbReadyTaskData->downloadThumbMap[path];
    ThumbRdbOpt opts = thumbReadyTaskData->opts;
    opts.row = data.id;
    ThumbnailUtils::RecordStartGenerateStats(data.stats, GenerateScene::FOREGROUND, LoadSourceType::LOCAL_PHOTO);
    ValuesBucket values;
    Size lcdSize;
    if (data.mediaType == MEDIA_TYPE_VIDEO && ThumbnailUtils::GetLocalThumbSize(data, ThumbnailType::LCD, lcdSize)) {
        ThumbnailUtils::SetThumbnailSizeValue(values, lcdSize, PhotoColumn::PHOTO_LCD_SIZE);
        int changedRows;
        CHECK_AND_RETURN_LOG(opts.store != nullptr, "store is null");
        int32_t err = opts.store->Update(changedRows, opts.table, values, MEDIA_DATA_DB_ID + " = ?",
        vector<string> { data.id });
        CHECK_AND_PRINT_LOG(err == NativeRdb::E_OK, "RdbStore lcd size failed! %{public}d", err);
    }
    thumbReadyTaskData->pendingTasks++;
    auto executor = [this, requestId = thumbReadyTaskData->requestId,
        pid = thumbReadyTaskData->pid](std::shared_ptr<ThumbnailTaskData> &taskData) {
            this->ExecuteCreateThumbnailTask(taskData, requestId, pid);
    };
    IThumbnailHelper::AddThumbnailGenerateTask(executor, opts, data,
        ThumbnailTaskType::FOREGROUND, ThumbnailTaskPriority::LOW);
}

void ThumbnailReadyManager::RecordNotFoundThumbnail(const std::string &path,
    std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo> thumbReadyTaskData)
{
    MediaLibraryTracer tracer;
    tracer.Start("RecordNotFoundThumbnail");
    CHECK_AND_RETURN_LOG(thumbReadyTaskData != nullptr, "thumbReadyTaskData is null");
    CHECK_AND_RETURN_LOG(thumbReadyTaskData->downloadThumbMap.find(path) != thumbReadyTaskData->downloadThumbMap.end(),
        "downloaded thumbnail path not found");
    auto data = thumbReadyTaskData->downloadThumbMap[path];
    ThumbRdbOpt& opts = thumbReadyTaskData->opts;
    IThumbnailHelper::CacheThumbnailState(opts, data, false);
    int32_t err = ThumbnailGenerationPostProcess::PostProcess(data, opts);
    CHECK_AND_PRINT_LOG(err == E_OK, "PostProcess failed, err %{public}d", err);
}

void ThumbnailReadyManager::HandleDownloadBatch(int32_t requestId, pid_t pid)
{
    MediaLibraryTracer tracer;
    tracer.Start("HandleDownloadBatch");
    std::lock_guard<std::mutex> lock(downloadIdMutex_);
    std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo> thumbReadyTaskData;
    CHECK_AND_RETURN(IsNeedExecuteTask(requestId, pid, thumbReadyTaskData));
    auto downloadCallback = std::make_shared<ThumbnailCloudDownloadCallback>(requestId, pid);
    int32_t ret = CloudSyncManager::GetInstance().StartFileCache(thumbReadyTaskData->cloudPaths,
        thumbReadyTaskData->downloadId, FieldKey::FIELDKEY_LCD, downloadCallback, TIME_MILLI_SECONDS);
    if (ret == CloudSync::E_OK) {
        RegisterDownloadTimer(requestId, pid);
    } else {
        if (ret == CloudSync::E_TIMEOUT) {
            timeoutCount_++;
            MEDIA_INFO_LOG("Download thumbnail timeout, count:%{public}d", timeoutCount_.load());
            auto currentMilliSecond = MediaFileUtils::UTCTimeMilliSeconds();
            lastTimeoutTime_ = timeoutCount_ > 1 ? lastTimeoutTime_ : currentMilliSecond;
            if (currentMilliSecond - lastTimeoutTime_ > THUMB_TEN_MINUTES_WAIT_TIME) {
                MEDIA_INFO_LOG("Download thumbnail timeout 10 minutes, reset timeout count.");
                timeoutCount_.store(0);
            }
            lastTimeoutTime_ = currentMilliSecond;
        }
        std::lock_guard<std::mutex> cvLock(thumbReadyTaskData->cvMutex);
        thumbReadyTaskData->pendingTasks--;
        if (thumbReadyTaskData->pendingTasks.load() <= 0) {
            thumbReadyTaskData->cv.notify_all();
        }
    }
}

void ThumbnailReadyManager::CreateAstcBatchOnDemandTaskFinish(std::shared_ptr<ThumbnailReadyManager::
    AstcBatchTaskInfo>& thumbReadyTaskData)
{
    UnRegisterDownloadTimer();
    CHECK_AND_RETURN_LOG(thumbReadyTaskData != nullptr, "GetAstcBatchTaskInfo failed");
    int32_t requestId = thumbReadyTaskData->requestId;
    pid_t pid = thumbReadyTaskData->pid;
    MEDIA_INFO_LOG("CreateAstcBatchOnDemand cloud and local task all finish, pid: %{public}d, requestId: %{public}d",
        pid, requestId);
    if (thumbReadyTaskData->isCancel.load()) {
        MEDIA_INFO_LOG("Need cancel task, StopFileCache in");
        std::lock_guard<std::mutex> lock(downloadIdMutex_);
        CHECK_AND_RETURN_LOG(thumbReadyTaskData->downloadId != -1, "downloadId is invalid");
        int res = CloudSyncManager::GetInstance().StopFileCache(thumbReadyTaskData->downloadId, true);
        thumbReadyTaskData->downloadId = THUMB_INVALID_VALUE;
    }
    CHECK_AND_RETURN(IsNeedExecuteTask(requestId, pid));
    ThumbGenBatchTaskFinishNotify(requestId, pid);
}

void ThumbnailReadyManager::ThumbGenBatchTaskFinishNotify(int32_t requestId, pid_t pid)
{
    {
        std::lock_guard<std::mutex> lock(processMutex_);
        processRequestMap_.Erase(pid);
    }
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "watch is null");
    std::string notifyUri = PhotoColumn::PHOTO_URI_PREFIX + std::to_string(requestId) + "," +
        std::to_string(pid);
    MEDIA_INFO_LOG("ThumbGenBatchTaskFinishNotify notifyUri is : %{public}s", notifyUri.c_str());
    watch->Notify(notifyUri, NotifyType::NOTIFY_THUMB_UPDATE);
}

void ThumbnailReadyManager::ProcessAstcBatchTask(ThumbRdbOpt opts, NativeRdb::RdbPredicates predicate,
    const int32_t requestId, const pid_t pid)
{
    MEDIA_INFO_LOG("ProcessAstcBatchTask start!");
    std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo> latestInfo;
    CHECK_AND_RETURN(IsNeedExecuteTask(requestId, pid, latestInfo));
    CHECK_AND_RETURN_LOG(latestInfo != nullptr, "GetAstcBatchTaskInfo failed");
    int32_t taskErr = 0;
    latestInfo->opts = opts;

    CHECK_AND_RETURN_LOG(QueryNoAstcInfosOnDemand(opts, latestInfo, predicate, taskErr),
        "Failed to QueryNoAstcInfos %{public}d", taskErr);

    latestInfo->isLocalTaskFinish = latestInfo->localInfos.empty();
    latestInfo->isCloudTaskFinish = latestInfo->cloudPaths.empty();
    latestInfo->pendingTasks.store(0);
    for (auto& info : latestInfo->localInfos) {
        opts.row = info.id;
        latestInfo->pendingTasks++;
        auto executor = [this, requestId = requestId,
            pid = pid](std::shared_ptr<ThumbnailTaskData> &taskData) {
            this->ExecuteCreateThumbnailTask(taskData, requestId, pid);
        };
        IThumbnailHelper::AddThumbnailGenerateTask(executor, opts, info,
            ThumbnailTaskType::FOREGROUND, ThumbnailTaskPriority::LOW);
    }
    latestInfo->isLocalTaskFinish = true;
    if (!latestInfo->cloudPaths.empty()) {
        latestInfo->pendingTasks++;
        HandleDownloadBatch(requestId, pid);
    }
    {
        std::unique_lock<std::mutex> cvLock(latestInfo->cvMutex);
        bool waitResult = false;
        do {
            waitResult = latestInfo->cv.wait_for(cvLock, std::chrono::seconds(THUMB_PROCESS_WAIT_TIME),
                [latestInfo] { return latestInfo->pendingTasks == 0 || latestInfo->isCancel; });
            CHECK_AND_WARN_LOG(waitResult, "ProcessAstcBatchTask wait timeout 30s!");
        } while (!waitResult);
    }
    CreateAstcBatchOnDemandTaskFinish(latestInfo);
    MEDIA_INFO_LOG("ProcessAstcBatchTask end!");
}

int32_t ThumbnailReadyManager::CreateAstcBatchOnDemand(
    ThumbRdbOpt &opts, NativeRdb::RdbPredicates predicate, int32_t requestId, pid_t pid)
{
    MEDIA_INFO_LOG("CreateAstcBatchOnDemand start! pid:%{public}d, requestId:%{public}d", pid, requestId);
    CHECK_AND_RETURN_RET_LOG(opts.store != nullptr, E_ERR, "rdbStore is not init");
    std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo> taskInfo =
        make_shared<ThumbnailReadyManager::AstcBatchTaskInfo>();
    int32_t err = 0;
    CHECK_AND_RETURN_RET_LOG(QueryNoAstcInfosOnDemand(opts, taskInfo, predicate, err),
        err, "Failed to QueryNoAstcInfos %{public}d", err);
    if (taskInfo->localInfos.empty() && taskInfo->cloudPaths.empty()) {
        if (timeoutCount_ >= RETRY_DOWNLOAD_THUMB_LIMIT) {
            timeoutCount_.store(0);
        } else {
            timeoutCount_.store(0);
            MEDIA_INFO_LOG("No need create Astc.");
            return E_THUMBNAIL_ASTC_ALL_EXIST;
        }
    }
    MEDIA_INFO_LOG("CreateAstcBatchOnDemand pid:%{public}d, requestId:%{public}d,"
        "localInfos size:%{public}zu, cloudPaths size:%{public}zu", pid, requestId,
        taskInfo->localInfos.size(), taskInfo->cloudPaths.size());

    auto executor = [this, opts = opts, predicate = predicate, requestId = requestId,
        pid = pid](std::shared_ptr<ThumbnailTaskData> &taskData) {
        this->ProcessAstcBatchTask(opts, predicate, requestId, pid);
    };
    ThumbnailData data;
    IThumbnailHelper::AddThumbnailGenerateTask(executor, opts, data,
        ThumbnailTaskType::THUMB_READY, ThumbnailTaskPriority::LOW);
    MEDIA_INFO_LOG("CreateAstcBatchOnDemand end!");
    return E_OK;
}

void ThumbnailReadyManager::DownloadTimeOut(int32_t requestId, pid_t pid)
{
    MediaLibraryTracer tracer;
    tracer.Start("DownloadTimeOut");
    MEDIA_INFO_LOG("DownloadTimeOut pid:%{public}d, requestId:%{public}d", pid, requestId);
    std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo> timerInfo;
    CHECK_AND_RETURN(IsNeedExecuteTask(requestId, pid, timerInfo));
    if (!timerInfo->isDownloadEnd) {
        timeoutCount_++;
        MEDIA_INFO_LOG("Download thumbnail timeout, count:%{public}d", timeoutCount_.load());
        auto currentMilliSecond = MediaFileUtils::UTCTimeMilliSeconds();
        lastTimeoutTime_ = timeoutCount_ > 1 ? lastTimeoutTime_ : currentMilliSecond;
        if (currentMilliSecond - lastTimeoutTime_ > THUMB_TEN_MINUTES_WAIT_TIME) {
            MEDIA_INFO_LOG("Download thumbnail timeout 10 minutes, reset timeout count.");
            timeoutCount_.store(0);
        }
        lastTimeoutTime_ = currentMilliSecond;
        std::lock_guard<std::mutex> cvLock(timerInfo->cvMutex);
        timerInfo->isCancel.store(true);
        timerInfo->cv.notify_all();
    } else {
        MEDIA_INFO_LOG("Download thumbnail timeout 15s, but download is end, reset timeout count");
        timeoutCount_.store(0);
    }
}

void ThumbnailReadyManager::RegisterDownloadTimer(int32_t requestId, pid_t pid)
{
    MediaLibraryTracer tracer;
    tracer.Start("RegisterDownloadTimer");
    auto timerInfo = GetAstcBatchTaskInfo(pid);
    CHECK_AND_RETURN_LOG(timerInfo != nullptr, "GetAstcBatchTaskInfo failed");
    Utils::Timer::TimerCallback timerCallback = [this, requestId, pid]() {
        this->DownloadTimeOut(requestId, pid);
    };
    std::lock_guard<std::mutex> lock(timerMutex_);
    if (timerId > 0) {
        timer_.Unregister(timerId);
    }
    timer_.Setup();
    timerId = timer_.Register(timerCallback, THUMB_BATCH_WAIT_TIME, true);
    timerInfo->isCancel.store(false);
    MEDIA_INFO_LOG("15s download timer Restart, timeId:%{public}u", timerId);
}

void ThumbnailReadyManager::UnRegisterDownloadTimer()
{
    std::lock_guard<std::mutex> lock(timerMutex_);
    if (timerId <= 0) {
        return;
    }
    timer_.Unregister(timerId);
    timer_.Shutdown(false);
    timerId = 0;
}

void ThumbnailReadyManager::InsertHighTemperatureTask(NativeRdb::RdbPredicates &rdbPredicate,
    int32_t requestId, pid_t pid)
{
    auto temperatureStatus = make_shared<ThumbnailReadyManager::AstcBatchTaskInfo>();
    if (temperatureStatusMap_.Find(pid, temperatureStatus)) {
        if (temperatureStatus != nullptr) {
            int32_t oldRequestId = temperatureStatus->requestId;
            CHECK_AND_RETURN(oldRequestId < requestId);
        }
        temperatureStatusMap_.Erase(pid);
    }
    temperatureStatus = make_shared<ThumbnailReadyManager::AstcBatchTaskInfo>();
    temperatureStatus->requestId = requestId;
    temperatureStatus->pid = pid;
    temperatureStatus->rdbPredicatePtr = make_shared<NativeRdb::RdbPredicates>(rdbPredicate);
    temperatureStatus->isTemperatureHighForReady = true;
    temperatureStatusMap_.Insert(pid, temperatureStatus);
}

int32_t ThumbnailReadyManager::CreateAstcBatchOnDemand(NativeRdb::RdbPredicates &rdbPredicate,
    int32_t requestId, pid_t pid)
{
    std::lock_guard<std::mutex> lock(processMutex_);
    CHECK_AND_RETURN_RET_LOG(requestId > 0, E_INVALID_VALUES,
        "create astc batch failed, invalid request id:%{public}d", requestId);
    if (GetCurrentTemperatureLevel() >= READY_TEMPERATURE_LEVEL) {
        InsertHighTemperatureTask(rdbPredicate, requestId, pid);
        MEDIA_INFO_LOG("temperature is too high, the operation is suspended");
        return E_OK;
    }

    std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo> taskInfo;
    if (processRequestMap_.Find(pid, taskInfo)) {
        if (taskInfo != nullptr) {
            int32_t oldRequestId = taskInfo->requestId;
            MEDIA_INFO_LOG("create astc batch task, pid:%{public}d, oldRequestId:%{public}d", pid, oldRequestId);
            CHECK_AND_RETURN_RET(oldRequestId < requestId, E_INVALID_VALUES);
            std::lock_guard<std::mutex> cvLock(taskInfo->cvMutex);
            taskInfo->isCancel.store(true);
            taskInfo->cv.notify_all();
        }
    }
    processRequestMap_.Erase(pid);
    taskInfo = make_shared<ThumbnailReadyManager::AstcBatchTaskInfo>();
    taskInfo->requestId = requestId;
    taskInfo->pid = pid;
    MEDIA_INFO_LOG("create astc batch task, pid:%{public}d, requestId:%{public}d", pid, requestId);
    processRequestMap_.Insert(pid, taskInfo);

    ThumbRdbOpt opts = {
        .store = MediaLibraryUnistoreManager::GetInstance().GetRdbStore(),
        .table = PhotoColumn::PHOTOS_TABLE
    };
    return CreateAstcBatchOnDemand(opts, rdbPredicate, requestId, pid);
}

void ThumbnailReadyManager::CancelAstcBatchTask(int32_t requestId, pid_t pid)
{
    MEDIA_INFO_LOG("CancelAstcBatchTask pid:%{public}d, requestId:%{public}d", pid, requestId);
    std::lock_guard<std::mutex> lock(processMutex_);
    CHECK_AND_RETURN_LOG(requestId > 0, "cancel astc batch failed, invalid request id:%{public}d", requestId);

    auto temperatureStatus = make_shared<ThumbnailReadyManager::AstcBatchTaskInfo>();
    if (temperatureStatusMap_.Find(pid, temperatureStatus)) {
        if (temperatureStatus->requestId == requestId) {
            temperatureStatusMap_.Erase(pid);
        }
    }
    auto taskInfo = make_shared<ThumbnailReadyManager::AstcBatchTaskInfo>();
    CHECK_AND_RETURN_LOG(processRequestMap_.Find(pid, taskInfo), "cancel astc batch failed, no task found");
    CHECK_AND_RETURN_LOG(taskInfo != nullptr, "cancel astc batch failed, task info is null");
    CHECK_AND_RETURN(taskInfo->requestId == requestId);
    {
        std::lock_guard<std::mutex> cvLock(taskInfo->cvMutex);
        taskInfo->isCancel.store(true);
        taskInfo->cv.notify_all();
    }
    processRequestMap_.Erase(pid);
    MEDIA_INFO_LOG("CancelAstcBatchTask end");
}

void ThumbnailReadyManager::NotifyTempStatusForReady(const int32_t &currentTemperatureLevel)
{
    currentTemperatureLevel_ = currentTemperatureLevel;
    CHECK_AND_RETURN(currentTemperatureLevel_ < READY_TEMPERATURE_LEVEL);
    temperatureStatusMap_.Iterate([this](const pid_t pid,
        std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo> temperatureStatus) {
        if (temperatureStatus != nullptr && temperatureStatus->isTemperatureHighForReady &&
            temperatureStatus->requestId > 0) {
            auto predicate = temperatureStatus->rdbPredicatePtr;
            if (predicate != nullptr) {
                this->CreateAstcBatchOnDemand(*predicate.get(), temperatureStatus->requestId,
                    temperatureStatus->pid);
            }
        }
    });
    temperatureStatusMap_.Clear();
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS
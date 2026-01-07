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

#include <thread>

#include "thumbnail_ready_manager.h"

#include "datashare_helper.h"
#include "datashare_abs_result_set.h"
#include "dfx_utils.h"
#include "ithumbnail_helper.h"
#include "media_log.h"
#include "media_file_uri.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_errno.h"
#include "thumbnail_const.h"
#include "thumbnail_file_utils.h"
#include "thumbnail_generate_worker.h"
#include "thumbnail_generate_worker_manager.h"
#include "thumbnail_rdb_utils.h"
#include "thumbnail_source_loading.h"
#include "thumbnail_utils.h"

using namespace std;
using namespace OHOS::FileManagement::CloudSync;
using namespace OHOS::NativeRdb;
namespace OHOS {
namespace Media {
// LCOV_EXCL_START
const int TIME_MILLI_SECONDS = 5000;
constexpr int32_t THUMB_BATCH_WAIT_TIME = 15 * 1000;
constexpr int32_t RETRY_DOWNLOAD_THUMB_LIMIT = 3;
constexpr int64_t THUMB_TEN_MINUTES_WAIT_TIME = 10 * 60 * 1000;
constexpr int32_t THUMB_THREAD_WAIT_TIME = 5 * 60 * 1000;
const int THREADNUM = 1;

void ThumbnailCloudDownloadCallback::OnDownloadProcess(const DownloadProgressObj &progress)
{
    MEDIA_INFO_LOG("OnDownloadProcess, pid is %{public}d, requestId is %{public}d", pid_, requestId_);
    auto thumbReadyTaskData = ThumbnailReadyManager::GetInstance().GetAstcBatchTaskInfo(pid_);
    CHECK_AND_RETURN_LOG(thumbReadyTaskData != nullptr, "GetAstcBatchTaskInfo failed");
    MEDIA_INFO_LOG("OnDownloadProcess, thumbReadyTaskData downloadId is %{public}" PRId64
        ", progress downloadId is %{public}" PRId64,
        thumbReadyTaskData->downloadId, progress.downloadId);
    CHECK_AND_RETURN_LOG(thumbReadyTaskData->downloadId == progress.downloadId, "downloadId is not equal");
    if (progress.state == DownloadProgressObj::Status::COMPLETED) {
        ThumbnailReadyManager::GetInstance().CreateAstcAfterDownloadThumbOnDemand(progress.path, thumbReadyTaskData);
    } else if (progress.state == DownloadProgressObj::Status::FAILED) {
        MEDIA_ERR_LOG("download thumbnail file failed, file path is %{public}s", progress.path.c_str());
    }
    if (progress.batchState == DownloadProgressObj::Status::COMPLETED ||
        progress.batchState == DownloadProgressObj::Status::STOPPED ||
        progress.batchState == DownloadProgressObj::Status::FAILED) {
        ThumbnailReadyManager::GetInstance().SetDownloadEnd(pid_);
        ThumbnailReadyManager::GetInstance().SetCloudFinish(pid_);
        ThumbnailReadyManager::GetInstance().CreateAstcBatchOnDemandTaskFinish(pid_);
    }
    MEDIA_INFO_LOG("OnDownloadProcess end");
}

ReadyThreadPool::ReadyThreadPool(int32_t threadNum) : stop_(false)
{
    for (auto i = 0; i < threadNum; ++i) {
        workers_.emplace_back(&ReadyThreadPool::ReadyThreadWorker, this);
    }
}

ReadyThreadPool::~ReadyThreadPool()
{
    stop_.store(true);
    readyCv_.notify_all();
    for (auto& worker : workers_) {
        CHECK_AND_EXECUTE(!worker.joinable(), worker.join());
    }
}

void ReadyThreadPool::ReadyThreadWorker()
{
    while (!stop_.load()) {
        ReadyThreadPool::ReadyTask task;
        bool hasTask = false;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            readyCv_.wait_for(lock, std::chrono::milliseconds(THUMB_THREAD_WAIT_TIME),
                [this]() { return stop_.load() || !taskQueue_.empty() || !highTaskQueue_.empty(); });
            if (!stop_ && taskQueue_.empty() && highTaskQueue_.empty()) {
                stop_.store(true);
                MEDIA_INFO_LOG("After 5 minutes, all threads are cleared");
                break;
            }

            if (stop_.load() && taskQueue_.empty() && highTaskQueue_.empty()) {
                break;
            }

            if (!highTaskQueue_.empty()) {
                task = std::move(highTaskQueue_.front());
                highTaskQueue_.pop();
                hasTask = true;
            } else if (!taskQueue_.empty()) {
                task = std::move(taskQueue_.front());
                taskQueue_.pop();
                hasTask = true;
            }
        }
        if (hasTask) {
            task.func();
        }
    }
}

void ReadyThreadPool::Reinitialize()
{
    if (!stop_.load()) {
        return;
    }
    readyCv_.notify_all();
    for (auto& worker : workers_) {
        CHECK_AND_EXECUTE(!worker.joinable(), worker.join());
    }
    workers_.clear();
    stop_.store(false);
    for (auto i = 0; i < THREADNUM; ++i) {
        workers_.emplace_back(&ReadyThreadPool::ReadyThreadWorker, this);
    }
}

void ReadyThreadPool::SubmitReadyTask(int32_t requestId, pid_t pid, std::function<void()> func)
{
    ReadyThreadPool::ReadyTask task{pid, requestId, func};
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (stop_.load()) {
            Reinitialize();
        }
        taskQueue_.push(std::move(task));
    }
    readyCv_.notify_all();
    MEDIA_INFO_LOG("Task submitted - PID: %d, RequestId: %d", pid, requestId);
}

void ReadyThreadPool::SubmitHighPriorityReadyTask(int32_t requestId, pid_t pid, std::function<void()> func)
{
    ReadyThreadPool::ReadyTask task{pid, requestId, func};
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (stop_.load()) {
            Reinitialize();
        }
        highTaskQueue_.push(std::move(task));
    }
    readyCv_.notify_all();
    MEDIA_INFO_LOG("High priority task submitted - PID: %d, RequestId: %d", pid, requestId);
}

bool ReadyThreadPool::IsTaskMapEmpty()
{
    return taskQueue_.empty() && highTaskQueue_.empty();
}

ThumbnailReadyManager::ThumbnailReadyManager()
{
    threadPool_ = std::make_shared<ReadyThreadPool>();
    currentTemperatureLevel_ = 0;
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

void ThumbnailReadyManager::SetCloudFinish(const pid_t pid)
{
    auto taskInfo = GetAstcBatchTaskInfo(pid);
    if (taskInfo != nullptr) {
        taskInfo->isCloudTaskFinish = true;
    }
}

std::shared_ptr<ReadyThreadPool> ThumbnailReadyManager::GetThreadPool()
{
    return threadPool_;
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

void ThumbnailReadyManager::ExecuteCreateThumbnailTask(std::shared_ptr<ThumbnailTaskData> &taskData)
{
    MEDIA_INFO_LOG("ExecuteCreateThumbnailTask start!!!");
    CHECK_AND_RETURN_LOG(taskData != nullptr, "data is null");
    CHECK_AND_RETURN(IsNeedExecuteTask(taskData->requestId_, taskData->pid_));

    if (taskData->thumbnailData_.loaderOpts.loadingStates == SourceLoader::LOCAL_SOURCE_LOADING_STATES) {
        IThumbnailHelper::CreateThumbnail(taskData);
        return;
    }
    taskData->thumbnailData_.needGenerateExThumbnail = false;
    taskData->thumbnailData_.loaderOpts.loadingStates = ThumbnailUtils::IsExCloudThumbnail(taskData->thumbnailData_) ?
        SourceLoader::CLOUD_LCD_SOURCE_LOADING_STATES : SourceLoader::CLOUD_SOURCE_LOADING_STATES;
    taskData->thumbnailData_.genThumbScene = GenThumbScene::CLOUD_DOWNLOAD_THUMB;
    if (ThumbnailUtils::IsExCloudThumbnail(taskData->thumbnailData_)) {
        IThumbnailHelper::CreateAstcEx(taskData);
    } else {
        IThumbnailHelper::CreateAstc(taskData);
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
    rdbPredicate.EqualTo(PhotoColumn::PHOTO_THUMB_STATUS, "0");
    rdbPredicate.EndWrap();
    rdbPredicate.EndWrap();
}

bool ThumbnailReadyManager::QueryNoAstcInfosOnDemand(ThumbRdbOpt &opts,
    std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo> taskInfo,
    NativeRdb::RdbPredicates rdbPredicate, int &err)
{
    MEDIA_INFO_LOG("QueryNoAstcInfosOnDemand start!");
    vector<string> column = {
        MEDIA_DATA_DB_ID, MEDIA_DATA_DB_FILE_PATH, MEDIA_DATA_DB_HEIGHT, MEDIA_DATA_DB_WIDTH,
        MEDIA_DATA_DB_POSITION, MEDIA_DATA_DB_MEDIA_TYPE, MEDIA_DATA_DB_DATE_ADDED, MEDIA_DATA_DB_NAME,
        MEDIA_DATA_DB_ORIENTATION, PhotoColumn::PHOTO_EXIF_ROTATE, MEDIA_DATA_DB_DATE_TAKEN,
        MEDIA_DATA_DB_DATE_MODIFIED, PhotoColumn::PHOTO_THUMB_STATUS,
    };

    rdbPredicate.EqualTo(PhotoColumn::PHOTO_THUMBNAIL_READY, "0");
    if (!ThumbnailUtils::IsMobileNetworkEnabled() ||
        (timeoutCount_ > RETRY_DOWNLOAD_THUMB_LIMIT && ! isLocalAllFinished)) {
        CHECK_AND_PRINT_LOG(timeoutCount_ <= RETRY_DOWNLOAD_THUMB_LIMIT,
            "timeoutCount_:%{public}d is over three, just query local", timeoutCount_.load());
        AddQueryNoAstcRulesOnlyLocal(rdbPredicate);
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
            info.thumbnailStatus == 0)) {
            taskInfo->localInfos.emplace_back(info);
        } else {
            std::string uri = "";
            uri = MediaFileUri::GetPhotoUri(info.id, info.path, info.displayName);
            taskInfo->cloudPaths.emplace_back(uri);
            taskInfo->downloadThumbMap[uri] = info;
        }
    }
    MEDIA_INFO_LOG("QueryNoAstcInfosOnDemand end!");
    return true;
}

void ThumbnailReadyManager::CreateAstcAfterDownloadThumbOnDemand(const std::string &path,
    std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo> thumbReadyTaskData)
{
    MEDIA_INFO_LOG("CreateAstcAfterDownloadThumbOnDemand start!");
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

    IThumbnailHelper::AddThumbnailGenBatchTask(std::bind(&ThumbnailReadyManager::ExecuteCreateThumbnailTask,
        this, std::placeholders::_1), opts,
        data, thumbReadyTaskData->requestId, thumbReadyTaskData->pid);
    MEDIA_INFO_LOG("CreateAstcAfterDownloadThumbOnDemand end!");
}

void ThumbnailReadyManager::HandleDownloadBatch(int32_t requestId, pid_t pid)
{
    MEDIA_INFO_LOG("HandleDownloadBatch start!");
    CHECK_AND_RETURN(IsNeedExecuteTask(requestId, pid));

    std::lock_guard<std::mutex> lock(downloadIdMutex_);
    auto thumbReadyTaskData = GetAstcBatchTaskInfo(pid);
    CHECK_AND_RETURN_LOG(thumbReadyTaskData != nullptr, "thumbReadyTaskData is null");
    if (!thumbReadyTaskData->cloudPaths.empty()) {
        auto downloadCallback = std::make_shared<ThumbnailCloudDownloadCallback>(requestId, pid);
        int32_t ret = CloudSyncManager::GetInstance().StartFileCache(thumbReadyTaskData->cloudPaths,
            thumbReadyTaskData->downloadId, FieldKey::FIELDKEY_LCD, downloadCallback, TIME_MILLI_SECONDS);
        if (ret == CloudSync::E_OK) {
            RegisterDownloadTimer(pid);
        }
        if (ret == CloudSync::E_TIMEOUT) {
            timeoutCount_++;
            MEDIA_INFO_LOG("Download thumbnail timeout, count:%{public}d", timeoutCount_.load());
            auto currentMilliSecond = MediaFileUtils::UTCTimeMilliSeconds();
            lastTimeoutTime_ = timeoutCount_ > 1 ? lastTimeoutTime_ : currentMilliSecond;
            if (currentMilliSecond - lastTimeoutTime_ > THUMB_TEN_MINUTES_WAIT_TIME) {
                MEDIA_INFO_LOG("Download thumbnail timeout 10 minutes, reset timeout count.");
                timeoutCount_.store(0);
                isLocalAllFinished.store(false);
            }
            lastTimeoutTime_ = currentMilliSecond;
        }
    }
    MEDIA_INFO_LOG("HandleDownloadBatch end!");
}

void ThumbnailReadyManager::CreateAstcBatchOnDemandTaskFinish(const pid_t pid)
{
    MEDIA_INFO_LOG("CreateAstcBatchOnDemandTaskFinish start!");
    auto thumbReadyTaskData = GetAstcBatchTaskInfo(pid);
    CHECK_AND_RETURN_LOG(thumbReadyTaskData != nullptr, "GetAstcBatchTaskInfo failed");
    if (!thumbReadyTaskData->isLocalTaskFinish || !thumbReadyTaskData->isCloudTaskFinish) {
        return;
    }
    MEDIA_INFO_LOG("CreateAstcBatchOnDemand cloud and local task all finish, pid: %{public}d, requestId: %{public}d",
        pid, thumbReadyTaskData->requestId);
    IThumbnailHelper::AddThumbnailNotifyTask(IThumbnailHelper::ThumbGenBatchTaskFinishNotify,
        thumbReadyTaskData->requestId, pid);
}

void ThumbnailReadyManager::ProcessAstcBatchTask(ThumbRdbOpt opts, NativeRdb::RdbPredicates predicate,
    const int32_t requestId, const pid_t pid)
{
    MEDIA_INFO_LOG("ProcessAstcBatchTask start!");
    CHECK_AND_RETURN(IsNeedExecuteTask(requestId, pid));
    auto latestInfo = GetAstcBatchTaskInfo(pid);
    CHECK_AND_RETURN_LOG(latestInfo != nullptr, "GetAstcBatchTaskInfo failed");
    int32_t taskErr = 0;
    latestInfo->pid = pid;
    latestInfo->opts = opts;

    CHECK_AND_RETURN_LOG(QueryNoAstcInfosOnDemand(opts, latestInfo, predicate, taskErr),
        "Failed to QueryNoAstcInfos %{public}d", taskErr);
    if (latestInfo->localInfos.empty() && latestInfo->cloudPaths.empty()) {
        MEDIA_INFO_LOG("No need create Astc.");
        return;
    }
    if (latestInfo->localInfos.empty()) isLocalAllFinished = true;
    latestInfo->isLocalTaskFinish = latestInfo->localInfos.empty();
    latestInfo->isCloudTaskFinish = latestInfo->cloudPaths.empty();
    for (auto& info : latestInfo->localInfos) {
        info.genThumbScene = GenThumbScene::NEED_MORE_THUMB_READY;
        opts.row = info.id;
        ThumbnailUtils::RecordStartGenerateStats(info.stats, GenerateScene::FOREGROUND, LoadSourceType::LOCAL_PHOTO);
        info.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
        IThumbnailHelper::AddThumbnailGenBatchTask(std::bind(&ThumbnailReadyManager::ExecuteCreateThumbnailTask,
            this, std::placeholders::_1), opts, info, requestId, pid);
    }
    latestInfo->isLocalTaskFinish = true;
    if (!latestInfo->cloudPaths.empty()) {
        HandleDownloadBatch(requestId, pid);
    }
    CreateAstcBatchOnDemandTaskFinish(pid);
    MEDIA_INFO_LOG("ProcessAstcBatchTask end!");
    return;
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
        timeoutCount_.store(0);
        isLocalAllFinished.store(false);
        MEDIA_INFO_LOG("No need create Astc.");
        return E_THUMBNAIL_ASTC_ALL_EXIST;
    }
    MEDIA_INFO_LOG("CreateAstcBatchOnDemand pid:%{public}d, requestId:%{public}d,"
        "localInfos size:%{public}zu, cloudPaths size:%{public}zu", pid, requestId,
        taskInfo->localInfos.size(), taskInfo->cloudPaths.size());
    CHECK_AND_RETURN_RET_LOG(ThumbnailReadyManager::GetInstance().threadPool_ != nullptr,
        E_ERR, "ThreadPool is not init");
    ThumbnailReadyManager::GetInstance().threadPool_->SubmitReadyTask(requestId, pid,
        [this, opts, predicate, requestId, pid] {
        ProcessAstcBatchTask(opts, predicate, requestId, pid);
    });
    MEDIA_INFO_LOG("CreateAstcBatchOnDemand end!");
    return E_OK;
}

void ThumbnailReadyManager::DownloadTimeOut(pid_t pid)
{
    MEDIA_INFO_LOG("DownloadTimeOut pid:%{public}d", pid);
    auto timerInfo = GetAstcBatchTaskInfo(pid);
    CHECK_AND_RETURN_LOG(timerInfo != nullptr, "GetAstcBatchTaskInfo failed");
    if (!timerInfo->isDownloadEnd) {
        auto thumbReadyTaskData = GetAstcBatchTaskInfo(pid);
        CHECK_AND_RETURN_LOG(ThumbnailReadyManager::GetInstance().threadPool_ != nullptr, "ThreadPool is not init");
        ThumbnailReadyManager::GetInstance().threadPool_->SubmitHighPriorityReadyTask(thumbReadyTaskData->requestId,
            pid, [this, pid] {
            MEDIA_INFO_LOG("DownloadTimeOut 15s, StopFileCache in");
            std::lock_guard<std::mutex> lock(downloadIdMutex_);
            auto thumbReadyTaskData = GetAstcBatchTaskInfo(pid);
            CHECK_AND_RETURN(thumbReadyTaskData != nullptr);
            CHECK_AND_RETURN(thumbReadyTaskData->downloadId != -1);
            int res = CloudSyncManager::GetInstance().StopFileCache(thumbReadyTaskData->downloadId, true);
        });
        timeoutCount_++;
        MEDIA_INFO_LOG("Download thumbnail timeout, count:%{public}d", timeoutCount_.load());
        auto currentMilliSecond = MediaFileUtils::UTCTimeMilliSeconds();
        lastTimeoutTime_ = timeoutCount_ > 1 ? lastTimeoutTime_ : currentMilliSecond;
        if (currentMilliSecond - lastTimeoutTime_ > THUMB_TEN_MINUTES_WAIT_TIME) {
            MEDIA_INFO_LOG("Download thumbnail timeout 10 minutes, reset timeout count.");
            timeoutCount_.store(0);
            isLocalAllFinished.store(false);
        }
        lastTimeoutTime_ = currentMilliSecond;
    } else {
        MEDIA_INFO_LOG("Download thumbnail timeout 15s, but download is end, reset timeout count");
        timeoutCount_.store(0);
        isLocalAllFinished.store(false);
    }
    std::lock_guard<std::mutex> lock(timerMutex_);
    timer_.Unregister(timerInfo->timerId);
    timerInfo->timerId = 0;
}

void ThumbnailReadyManager::RegisterDownloadTimer(pid_t pid)
{
    auto timerInfo = GetAstcBatchTaskInfo(pid);
    CHECK_AND_RETURN_LOG(timerInfo != nullptr, "GetAstcBatchTaskInfo failed");
    Utils::Timer::TimerCallback timerCallback = [this, pid]() {
        this->DownloadTimeOut(pid);
    };
    std::lock_guard<std::mutex> lock(timerMutex_);
    timerInfo->timerId = timer_.Register(timerCallback, THUMB_BATCH_WAIT_TIME, true);
    MEDIA_INFO_LOG("15s download timer Restart, timeId:%{public}u", timerInfo->timerId);
}

void ThumbnailReadyManager::UnRegisterDownloadTimer(pid_t pid)
{
    auto timerInfo = GetAstcBatchTaskInfo(pid);
    CHECK_AND_RETURN_LOG(timerInfo != nullptr, "GetAstcBatchTaskInfo failed");
    std::lock_guard<std::mutex> lock(timerMutex_);
    timer_.Unregister(timerInfo->timerId);
    timerInfo->timerId = 0;
}

void ThumbnailReadyManager::CancelTask(int32_t requestId, pid_t pid)
{
    MEDIA_INFO_LOG("CancelTask pid:%{public}d, requestId:%{public}d", pid, requestId);
    std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo> taskInfo;
    CHECK_AND_RETURN(processRequestMap_.Find(pid, taskInfo));
    CHECK_AND_RETURN(taskInfo != nullptr);
    if (taskInfo->requestId == requestId) {
        processRequestMap_.Erase(pid);
    }
    CHECK_AND_RETURN(ThumbnailReadyManager::GetInstance().threadPool_ != nullptr);
    ThumbnailReadyManager::GetInstance().threadPool_->SubmitHighPriorityReadyTask(requestId,
        pid, [this, pid] {
        MEDIA_INFO_LOG("Need cancel task, StopFileCache in");
        UnRegisterDownloadTimer(pid);
        std::lock_guard<std::mutex> lock(downloadIdMutex_);
        auto thumbReadyTaskData = GetAstcBatchTaskInfo(pid);
        CHECK_AND_RETURN(thumbReadyTaskData != nullptr);
        CHECK_AND_RETURN_LOG(thumbReadyTaskData->downloadId != -1, "downloadId is invalid");
        int res = CloudSyncManager::GetInstance().StopFileCache(thumbReadyTaskData->downloadId, true);
    });
}

int32_t ThumbnailReadyManager::CreateAstcBatchOnDemand(NativeRdb::RdbPredicates &rdbPredicate,
    int32_t requestId, pid_t pid)
{
    std::lock_guard<std::mutex> lock(processMutex_);
    CHECK_AND_RETURN_RET_LOG(requestId > 0, E_INVALID_VALUES,
        "create astc batch failed, invalid request id:%{public}d", requestId);

    std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo> taskInfo;
    if (!processRequestMap_.Find(pid, taskInfo)) {
        taskInfo = make_shared<ThumbnailReadyManager::AstcBatchTaskInfo>();
        taskInfo->requestId = requestId;
        MEDIA_INFO_LOG("create astc batch task, pid:%{public}d, requestId:%{public}d", pid, requestId);
        processRequestMap_.Insert(pid, taskInfo);
    } else {
        CHECK_AND_RETURN_RET(taskInfo != nullptr, E_INVALID_VALUES);
        int32_t oldRequestId = taskInfo->requestId;
        CHECK_AND_RETURN_RET(oldRequestId < requestId, E_INVALID_VALUES);
        taskInfo->requestId = requestId;
        MEDIA_INFO_LOG("update astc batch task, pid:%{public}d, oldRequestId:%{public}d, requestId:%{public}d",
            pid, oldRequestId, requestId);
        CancelTask(oldRequestId, pid);
    }

    if (GetCurrentTemperatureLevel() >= READY_TEMPERATURE_LEVEL) {
        auto temperatureStatus = make_shared<ThumbnailReadyManager::AstcBatchTaskInfo>();
        temperatureStatus->requestId = requestId;
        temperatureStatus->rdbPredicatePtr = make_shared<NativeRdb::RdbPredicates>(rdbPredicate);
        temperatureStatus->isTemperatureHighForReady = true;
        temperatureStatusMap_.Insert(pid, temperatureStatus);
        MEDIA_INFO_LOG("temperature is too high, the operation is suspended");
        return E_OK;
    }

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
    auto taskInfo = make_shared<ThumbnailReadyManager::AstcBatchTaskInfo>();
    CHECK_AND_RETURN_LOG(processRequestMap_.Find(pid, taskInfo), "cancel astc batch failed, no task found");
    CHECK_AND_RETURN_LOG(taskInfo != nullptr, "cancel astc batch failed, task info is null");
    CHECK_AND_RETURN(taskInfo->requestId == requestId);

    auto temperatureStatus = make_shared<ThumbnailReadyManager::AstcBatchTaskInfo>();
    if (temperatureStatusMap_.Find(pid, temperatureStatus)) {
        temperatureStatusMap_.Erase(pid);
    }
    CancelTask(requestId, pid);
    MEDIA_INFO_LOG("CancelAstcBatchTask end");
}

void ThumbnailReadyManager::NotifyTempStatusForReady(const int32_t &currentTemperatureLevel)
{
    static std::mutex notifyTempStatusForReadyLock;
    std::lock_guard<std::mutex> lock(notifyTempStatusForReadyLock);
    currentTemperatureLevel_ = currentTemperatureLevel;
    CHECK_AND_RETURN(currentTemperatureLevel_ < READY_TEMPERATURE_LEVEL);
    temperatureStatusMap_.Iterate([this](const pid_t pid,
        std::shared_ptr<ThumbnailReadyManager::AstcBatchTaskInfo> temperatureStatus) {
        if (temperatureStatus->isTemperatureHighForReady && temperatureStatus->requestId > 0 &&
            temperatureStatus->rdbPredicatePtr != nullptr) {
            this->CreateAstcBatchOnDemand(*temperatureStatus->rdbPredicatePtr, temperatureStatus->requestId,
                temperatureStatus->isTemperatureHighForReady);
        }
    });
    temperatureStatusMap_.Clear();
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS
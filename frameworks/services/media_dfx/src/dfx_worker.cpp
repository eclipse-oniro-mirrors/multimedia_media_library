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
#define MLOG_TAG "DfxWorker"

#include "dfx_worker.h"

#include <pthread.h>

#include "cloud_media_asset_manager.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "dfx_manager.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "parameters.h"
#include "madvise_utils.h"

using namespace std;
namespace OHOS {
namespace Media {
shared_ptr<DfxWorker> DfxWorker::dfxWorkerInstance_{nullptr};

shared_ptr<DfxWorker> DfxWorker::GetInstance()
{
    if (dfxWorkerInstance_ == nullptr) {
        dfxWorkerInstance_ = make_shared<DfxWorker>();
    }
    return dfxWorkerInstance_;
}

DfxWorker::DfxWorker() : isThreadRunning_(false)
{
}

DfxWorker::~DfxWorker()
{
    MEDIA_INFO_LOG("DfxWorker deconstructor");
    isThreadRunning_ = false;
    workCv_.notify_all();
    if (delayThread_.joinable()) {
        delayThread_.join();
    }
    dfxWorkerInstance_ = nullptr;
}

void DfxWorker::Init()
{
    MEDIA_INFO_LOG("init");
    isThreadRunning_ = true;
    delayThread_ = thread([this] { this->InitDelayThread(); });
}

static void MadviseMemoryCheck()
{
    int64_t MAX_IPC_DURATION_TIME = 60 * 1000;
    int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t lastIPCTime = DfxManager::GetInstance()->GetLastIPCTime();
    if (!DfxManager::GetInstance()->GetMemoryRelease() && currentTime - lastIPCTime > MAX_IPC_DURATION_TIME) {
        const vector<string> MADVISE_LIST = {
            "libmedialibrary_data_extension.z.so",
            "libmedia_library.z.so",
            "libmedia_cloud_sync_data.z.so",
            "libarkdata_db_core.z.so",
            "libarkweb_engine.so",
        };
        MadviseUtils::MadviseMultipleLibraries(MADVISE_LIST);
        MEDIA_INFO_LOG("Madvise done, currentTime:%{public}lld, lastIPCTime:%{public}lld", currentTime,
            lastIPCTime);
        DfxManager::GetInstance()->SetMemoryRelease(true);
    }
}

static void HandleLoopTask(DfxData *data)
{
    MEDIA_DEBUG_LOG("HandleLoopTask");
    MadviseMemoryCheck();

    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
       NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    int64_t lastReportTime = prefs->GetLong(LAST_REPORT_TIME, 0);
    int64_t lastMiddleReportTime = prefs->GetLong(LAST_MIDDLE_REPORT_TIME, 0);
    DfxManager::GetInstance()->HandleFiveMinuteTask();
    if (MediaFileUtils::UTCTimeSeconds() - lastMiddleReportTime >= SIX_HOUR) {
        MEDIA_INFO_LOG("Report Middle Xml");
        lastMiddleReportTime = DfxManager::GetInstance()->HandleMiddleReport();
        prefs->PutLong(LAST_MIDDLE_REPORT_TIME, lastMiddleReportTime);
        prefs->FlushSync();
    }
    if (MediaFileUtils::UTCTimeSeconds() - lastReportTime >= ONE_DAY) {
        MEDIA_INFO_LOG("Report one day Xml");
        lastReportTime = DfxManager::GetInstance()->HandleOneDayReport();
        prefs->PutLong(LAST_REPORT_TIME, lastReportTime);
        prefs->FlushSync();
    }
    CloudMediaAssetManager::GetInstance().CheckStorageAndRecoverDownloadTask();
}

void DfxWorker::Prepare()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    thumbnailVersion_ = prefs->GetInt(THUMBNAIL_ERROR_VERSION, 0);
    deleteStatisticVersion_ = prefs->GetInt(DELETE_STATISTIC_VERSION, 0);
    if (IsThumbnailUpdate()) {
        thumbnailVersion_ = LATEST_THUMBNAIL_ERROR_VERSION;
        prefs->PutInt(THUMBNAIL_ERROR_VERSION, LATEST_THUMBNAIL_ERROR_VERSION);
    }
    if (IsDeleteStatisticUpdate()) {
        deleteStatisticVersion_ = LATEST_DELETE_STATISTIC_VERSION;
        prefs->PutInt(DELETE_STATISTIC_VERSION, LATEST_DELETE_STATISTIC_VERSION);
    }
    prefs->FlushSync();
}

bool DfxWorker::IsThumbnailUpdate()
{
    if (thumbnailVersion_ < LATEST_THUMBNAIL_ERROR_VERSION) {
        MEDIA_INFO_LOG("update thumbnail version from %{public}d to %{public}d", thumbnailVersion_,
            LATEST_THUMBNAIL_ERROR_VERSION);
        int32_t errCode;
        shared_ptr<NativePreferences::Preferences> prefs =
            NativePreferences::PreferencesHelper::GetPreferences(THUMBNAIL_ERROR_XML, errCode);
        if (!prefs) {
            MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
            return false;
        }
        prefs->Clear();
        prefs->FlushSync();
        return true;
    }
    return false;
}

bool DfxWorker::IsDeleteStatisticUpdate()
{
    if (deleteStatisticVersion_ < LATEST_DELETE_STATISTIC_VERSION) {
        MEDIA_INFO_LOG("update delete statistic version from %{public}d to %{public}d", deleteStatisticVersion_,
            LATEST_DELETE_STATISTIC_VERSION);
        int32_t errCode;
        shared_ptr<NativePreferences::Preferences> prefs =
            NativePreferences::PreferencesHelper::GetPreferences(DELETE_BEHAVIOR_XML, errCode);
        if (!prefs) {
            MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
            return false;
        }
        prefs->Clear();
        prefs->FlushSync();
        return true;
    }
    return false;
}

void DfxWorker::InitDelayThread()
{
    Prepare();
    bool isStartLoopTask = true;
    MEDIA_INFO_LOG("InitDelayThread");
    string name("DfxDelayThread");
    pthread_setname_np(pthread_self(), name.c_str());
    while (isThreadRunning_) {
        if (isStartLoopTask) {
            HandleLoopTask(nullptr);
            StartLoopTaskDelay();
            isStartLoopTask = false;
        }
        WaitForTask();
        if (!isThreadRunning_) {
            break;
        }
        if (IsTaskQueueEmpty()) {
            continue;
        }
        auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
        auto executeTime = std::chrono::time_point_cast<std::chrono::milliseconds>(GetWaitTime());
        auto delay = now.time_since_epoch().count() - executeTime.time_since_epoch().count();
        if (delay < 0) {
            continue;
        }
        shared_ptr<DfxTask> task = GetTask();
        if (task == nullptr) {
            continue;
        }
        task->executor_(task->data_);
        if (task->isDelayTask_) {
            StartLoopTaskDelay();
        }
        task = nullptr;
    }
}

void DfxWorker::StartLoopTaskDelay()
{
    auto loopTask = make_shared<DfxTask>(HandleLoopTask, nullptr);
    AddTask(loopTask, FIVE_MINUTE);
}

static bool compare(const shared_ptr<DfxTask> &taskOne, const shared_ptr<DfxTask> &taskTwo)
{
    auto firstTime = std::chrono::time_point_cast<std::chrono::milliseconds>(taskOne->executeTime_);
    auto secondTime = std::chrono::time_point_cast<std::chrono::milliseconds>(taskTwo->executeTime_);
    return firstTime.time_since_epoch().count() > secondTime.time_since_epoch().count();
}

void DfxWorker::AddTask(const shared_ptr<DfxTask> &task, int64_t delayTime)
{
    lock_guard<mutex> lockGuard(taskLock_);
    if (delayTime > 0) {
        task->executeTime_ = std::chrono::system_clock::now() + std::chrono::milliseconds(delayTime);
        task->isDelayTask_ = true;
    }
    taskList_.push_back(task);
    sort(taskList_.begin(), taskList_.end(), compare);
    workCv_.notify_one();
}

bool DfxWorker::IsTaskQueueEmpty()
{
    lock_guard<mutex> lock_Guard(taskLock_);
    return taskList_.empty();
}

void DfxWorker::WaitForTask()
{
    std::unique_lock<std::mutex> lock(workLock_);
    if (IsTaskQueueEmpty()) {
        workCv_.wait(lock,
            [this]() { return !isThreadRunning_ || !IsTaskQueueEmpty(); });
    } else {
        workCv_.wait_until(lock, GetWaitTime(),
            [this]() { return !isThreadRunning_ || !IsDelayTask(); });
    }
}

shared_ptr<DfxTask> DfxWorker::GetTask()
{
    lock_guard<mutex> lockGuard(taskLock_);
    if (taskList_.empty()) {
        return nullptr;
    }
    shared_ptr<DfxTask> task = taskList_.back();
    taskList_.pop_back();
    return task;
}


bool DfxWorker::IsDelayTask()
{
    lock_guard<mutex> lockGuard(taskLock_);
    if (taskList_.empty()) {
        return false;
    }
    shared_ptr<DfxTask> task = taskList_.back();
    return task->isDelayTask_;
}

std::chrono::system_clock::time_point DfxWorker::GetWaitTime()
{
    lock_guard<mutex> lockGuard(taskLock_);
    if (taskList_.empty()) {
        return std::chrono::system_clock::time_point();
    }
    shared_ptr<DfxTask> task = taskList_.back();
    return task->executeTime_;
}

void DfxWorker::End()
{
    isEnd_ = true;
}
} // namespace Media
} // namespace OHOS
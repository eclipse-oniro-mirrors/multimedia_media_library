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
#define MLOG_TAG "MediaLibraryPeriodWorker"

#include "medialibrary_period_worker.h"

#include "media_log.h"
#include "power_efficiency_manager.h"

using namespace std;

namespace OHOS {
namespace Media {

shared_ptr<MediaLibraryPeriodWorker> MediaLibraryPeriodWorker::periodWorkerInstance_{nullptr};
mutex MediaLibraryPeriodWorker::instanceMtx_;
mutex MediaLibraryPeriodWorker::mtx_;
atomic<bool> MediaLibraryPeriodWorker::stop_ = true;
std::map<int32_t, std::shared_ptr<MedialibraryPeriodTask>>MediaLibraryPeriodWorker::tasks_;
static constexpr int32_t NOTIFY_TIME = 100;
static constexpr int32_t UPDATE_ANALYSIS_TIME = 2000;
static constexpr int32_t WAIT_FOR_DETACH_TIME = 100;

shared_ptr<MediaLibraryPeriodWorker> MediaLibraryPeriodWorker::GetInstance()
{
    if (periodWorkerInstance_ == nullptr) {
        lock_guard<mutex> lockGuard(instanceMtx_);
        if (periodWorkerInstance_ == nullptr) {
            periodWorkerInstance_ = shared_ptr<MediaLibraryPeriodWorker>(new MediaLibraryPeriodWorker());
            periodWorkerInstance_->Init();
        }
    }
    return periodWorkerInstance_;
}

MediaLibraryPeriodWorker::MediaLibraryPeriodWorker() {}

MediaLibraryPeriodWorker::~MediaLibraryPeriodWorker()
{
    lock_guard<mutex> lockGuard(instanceMtx_);
    stop_.store(true);
    this_thread::sleep_for(chrono::milliseconds(WAIT_FOR_DETACH_TIME));
    periodWorkerInstance_ = nullptr;
}

void MediaLibraryPeriodWorker::Init()
{
    auto notifyTask = make_shared<MedialibraryPeriodTask>(PeriodTaskType::COMMON_NOTIFY, NOTIFY_TIME, "notify worker");
    auto cloudAnalysisAlbumTask = make_shared<MedialibraryPeriodTask>(PeriodTaskType::CLOUD_ANALYSIS_ALBUM,
        UPDATE_ANALYSIS_TIME, "analysis worker");
    tasks_[PeriodTaskType::COMMON_NOTIFY] = notifyTask;
    tasks_[PeriodTaskType::CLOUD_ANALYSIS_ALBUM] = cloudAnalysisAlbumTask;
}

bool MediaLibraryPeriodWorker::StartTask(PeriodTaskType periodTaskType, PeriodExecute periodExecute,
    PeriodTaskData *data)
{
    lock_guard<mutex> lockGuard(mtx_);
    auto task = tasks_.find(periodTaskType);
    if (task == tasks_.end() || !task->second) {
        return false;
    }
    task->second->isThreadRunning_.store(true);
    task->second->isStop_.store(false);
    task->second->executor_ = periodExecute;
    task->second->data_ = data;
    thread([periodTaskType]() { MediaLibraryPeriodWorker::HandleTask(periodTaskType); }).detach();
    return true;
}


void MediaLibraryPeriodWorker::StopThread(PeriodTaskType periodTaskType)
{
    lock_guard<mutex> lockGuard(mtx_);
    auto task = tasks_.find(periodTaskType);
    if (task == tasks_.end() || !task->second) {
        return;
    }
    task->second->isStop_.store(true);
}

bool MediaLibraryPeriodWorker::IsThreadRunning(PeriodTaskType periodTaskType)
{
    lock_guard<mutex> lockGuard(mtx_);
    auto task = tasks_.find(periodTaskType);
    if (task == tasks_.end() || !task->second) {
        return false;
    }
    return task->second->isThreadRunning_.load();
}

void MediaLibraryPeriodWorker::HandleTask(PeriodTaskType periodTaskType)
{
    std::shared_ptr<MedialibraryPeriodTask> task = nullptr;
    {
        lock_guard<mutex> lockGuard(mtx_);
        auto iterator = tasks_.find(periodTaskType);
        if (iterator == tasks_.end() || iterator->second == nullptr) {
            return;
        }
        task = iterator->second;
    }
    string name = task->threadName_;
    pthread_setname_np(pthread_self(), name.c_str());
    MEDIA_INFO_LOG("start %{public}s", name.c_str());
    stop_.store(false);
    while (!stop_.load() && task->isThreadRunning_.load()) {
        task->executor_(task->data_);
        if (task->isStop_.load()) {
            task->isThreadRunning_.store(false);
            break;
        }
        this_thread::sleep_for(chrono::milliseconds(task->period_));
    }
    MEDIA_INFO_LOG("end %{public}s", name.c_str());
}
} // namespace Media
} // namespace OHOS
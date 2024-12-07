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
static constexpr int32_t NOTIFY_TIME = 100;
static constexpr int32_t UPDATE_ANALYSIS_TIME = 2000;

shared_ptr<MediaLibraryPeriodWorker> MediaLibraryPeriodWorker::GetInstance()
{
    if (periodWorkerInstance_ == nullptr) {
        lock_guard<mutex> lockGuard(instanceMtx_);
        periodWorkerInstance_ = shared_ptr<MediaLibraryPeriodWorker>(new MediaLibraryPeriodWorker());
        if (periodWorkerInstance_ != nullptr) {
            periodWorkerInstance_->Init();
        }
    }
    return periodWorkerInstance_;
}

MediaLibraryPeriodWorker::MediaLibraryPeriodWorker() {}

MediaLibraryPeriodWorker::~MediaLibraryPeriodWorker()
{
    for (auto &task : tasks_) {
        task.second->isStop_.store(true);
    }
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
    if (task == tasks_.end()) {
        return false;
    }
    task->second->isThreadRunning_.store(true);
    task->second->isStop_.store(false);
    task->second->executor_ = periodExecute;
    task->second->data_ = data;
    thread([this, periodTaskType]() { this->HandleTask(periodTaskType); }).detach();
    return true;
}


void MediaLibraryPeriodWorker::StopThread(PeriodTaskType periodTaskType)
{
    lock_guard<mutex> lockGuard(mtx_);
    auto task = tasks_.find(periodTaskType);
    if (task == tasks_.end()) {
        return;
    }
    task->second->isStop_.store(true);
}

bool MediaLibraryPeriodWorker::IsThreadRunning(PeriodTaskType periodTaskType)
{
    lock_guard<mutex> lockGuard(mtx_);
    auto task = tasks_.find(periodTaskType);
    if (task == tasks_.end()) {
        return false;
    }
    return task->second->isThreadRunning_.load();
}

void MediaLibraryPeriodWorker::HandleTask(PeriodTaskType periodTaskType)
{
    auto task = tasks_.find(periodTaskType);
    if (task == tasks_.end()) {
        return;
    }
    string name = task->second->threadName_;
    pthread_setname_np(pthread_self(), name.c_str());
    MEDIA_INFO_LOG("start %{public}s", name.c_str());
    while (task->second->isThreadRunning_.load()) {
        task->second->executor_(task->second->data_);
        if (task->second->isStop_.load()) {
            task->second->isThreadRunning_.store(false);
            break;
        }
        this_thread::sleep_for(chrono::milliseconds(task->second->period_));
    }
    MEDIA_INFO_LOG("end %{public}s", name.c_str());
}
} // namespace Media
} // namespace OHOS
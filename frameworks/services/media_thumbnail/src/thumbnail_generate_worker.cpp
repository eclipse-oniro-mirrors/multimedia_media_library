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

#include "thumbnail_generate_worker.h"

#include <pthread.h>

#include "medialibrary_errno.h"
#include "medialibrary_notify.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
static constexpr int32_t THREAD_NUM_FOREGROUND = 4;
static constexpr int32_t THREAD_NUM_BACKGROUND = 2;
constexpr size_t TASK_INSERT_COUNT = 15;
constexpr size_t CLOSE_THUMBNAIL_WORKER_TIME_INTERVAL = 270000;

ThumbnailGenerateWorker::~ThumbnailGenerateWorker()
{
    ClearWorkerThreads();

    if (timerId_ != 0) {
        timer_.Unregister(timerId_);
        timer_.Shutdown();
        timerId_ = 0;
    }
}

int32_t ThumbnailGenerateWorker::Init(const ThumbnailTaskType &taskType)
{
    std::unique_lock<std::mutex> lock(taskMutex_);
    if (!threads_.empty()) {
        return E_OK;
    }

    MEDIA_INFO_LOG("threads empty, need to init, taskType:%{public}d", taskType_);
    int32_t threadNum;
    std::string threadName;
    taskType_ = taskType;
    if (taskType == ThumbnailTaskType::FOREGROUND) {
        threadNum = THREAD_NUM_FOREGROUND;
        threadName = THREAD_NAME_FOREGROUND;
    } else if (taskType == ThumbnailTaskType::BACKGROUND) {
        threadNum = THREAD_NUM_BACKGROUND;
        threadName = THREAD_NAME_BACKGROUND;
    } else {
        MEDIA_ERR_LOG("invalid task type");
        return E_ERR;
    }

    isThreadRunning_ = true;
    for (auto i = 0; i < threadNum; i++) {
        std::shared_ptr<ThumbnailGenerateThreadStatus> threadStatus =
            std::make_shared<ThumbnailGenerateThreadStatus>(i);
        std::thread thread([this, threadStatus] { this->StartWorker(threadStatus); });
        pthread_setname_np(thread.native_handle(), threadName.c_str());
        threads_.emplace_back(std::move(thread));
        threadsStatus_.emplace_back(threadStatus);
    }
    lock.unlock();
    RegisterWorkerTimer();
    return E_OK;
}

int32_t ThumbnailGenerateWorker::ReleaseTaskQueue(const ThumbnailTaskPriority &taskPriority)
{
    if (taskPriority == ThumbnailTaskPriority::HIGH) {
        highPriorityTaskQueue_.Clear();
    } else if (taskPriority == ThumbnailTaskPriority::MID) {
        midPriorityTaskQueue_.Clear();
    } else if (taskPriority == ThumbnailTaskPriority::LOW) {
        lowPriorityTaskQueue_.Clear();
    } else {
        MEDIA_ERR_LOG("invalid task priority");
        return E_ERR;
    }
    return E_OK;
}

int32_t ThumbnailGenerateWorker::AddTask(
    const std::shared_ptr<ThumbnailGenerateTask> &task, const ThumbnailTaskPriority &taskPriority)
{
    if (taskPriority == ThumbnailTaskPriority::HIGH) {
        highPriorityTaskQueue_.Push(task);
    } else if (taskPriority == ThumbnailTaskPriority::MID) {
        midPriorityTaskQueue_.Push(task);
    } else if (taskPriority == ThumbnailTaskPriority::LOW) {
        lowPriorityTaskQueue_.Push(task);
    } else {
        MEDIA_ERR_LOG("invalid task priority");
        return E_ERR;
    }

    Init(taskType_);
    workerCv_.notify_one();
    return E_OK;
}

void ThumbnailGenerateWorker::IgnoreTaskByRequestId(int32_t requestId)
{
    ignoreRequestId_.store(requestId);

    std::unique_lock<std::mutex> lock(requestIdMapLock_);
    requestIdTaskMap_.erase(requestId);
    MEDIA_INFO_LOG("IgnoreTaskByRequestId, requestId: %{public}d", requestId);
}

bool ThumbnailGenerateWorker::WaitForTask(std::shared_ptr<ThumbnailGenerateThreadStatus> threadStatus)
{
    std::unique_lock<std::mutex> lock(workerLock_);
    if (highPriorityTaskQueue_.Empty() && midPriorityTaskQueue_.Empty() && lowPriorityTaskQueue_.Empty() &&
        isThreadRunning_) {
        ignoreRequestId_ = 0;
        threadStatus->isThreadWaiting_ = true;
        bool ret = workerCv_.wait_for(lock, std::chrono::milliseconds(CLOSE_THUMBNAIL_WORKER_TIME_INTERVAL), [this]() {
            return !isThreadRunning_ || !highPriorityTaskQueue_.Empty() || !midPriorityTaskQueue_.Empty() ||
                   !lowPriorityTaskQueue_.Empty();
        });
        if (!ret) {
            MEDIA_INFO_LOG("Wait for task timeout");
            return false;
        }
    }
    threadStatus->isThreadWaiting_ = false;
    return isThreadRunning_;
}

void ThumbnailGenerateWorker::StartWorker(std::shared_ptr<ThumbnailGenerateThreadStatus> threadStatus)
{
    std::string name("ThumbnailGenerateWorker");
    pthread_setname_np(pthread_self(), name.c_str());
    MEDIA_INFO_LOG("ThumbnailGenerateWorker thread start, taskType:%{public}d, id:%{public}d",
        taskType_, threadStatus->threadId_);
    while (isThreadRunning_) {
        if (!WaitForTask(threadStatus)) {
            continue;
        }
        std::shared_ptr<ThumbnailGenerateTask> task;
        if (!highPriorityTaskQueue_.Empty() && highPriorityTaskQueue_.Pop(task) && task != nullptr) {
            if (NeedIgnoreTask(task->data_->requestId_)) {
                continue;
            }
            task->executor_(task->data_);
            ++(threadStatus->taskNum_);
            continue;
        }

        if (!midPriorityTaskQueue_.Empty() && midPriorityTaskQueue_.Pop(task) && task != nullptr) {
            if (NeedIgnoreTask(task->data_->requestId_)) {
                continue;
            }
            task->executor_(task->data_);
            ++(threadStatus->taskNum_);
            continue;
        }

        if (!lowPriorityTaskQueue_.Empty() && lowPriorityTaskQueue_.Pop(task) && task != nullptr) {
            if (NeedIgnoreTask(task->data_->requestId_)) {
                continue;
            }
            task->executor_(task->data_);
            ++(threadStatus->taskNum_);
        }
    }
    MEDIA_INFO_LOG("ThumbnailGenerateWorker thread finish, taskType:%{public}d, id:%{public}d",
        taskType_, threadStatus->threadId_);
}

bool ThumbnailGenerateWorker::NeedIgnoreTask(int32_t requestId)
{
    return ignoreRequestId_ != 0 && ignoreRequestId_ == requestId;
}

void ThumbnailGenerateWorker::ClearWorkerThreads()
{
    {
        std::unique_lock<std::mutex> lock(workerLock_);
        isThreadRunning_ = false;
    }
    ignoreRequestId_ = 0;
    workerCv_.notify_all();
    for (auto &thread : threads_) {
        if (!thread.joinable()) {
            continue;
        }
        thread.join();
    }
    threadsStatus_.clear();
    threads_.clear();
    MEDIA_INFO_LOG("Clear ThumbnailGenerateWorker threads successfully, taskType:%{public}d", taskType_);
}

bool ThumbnailGenerateWorker::IsAllThreadWaiting()
{
    std::unique_lock<std::mutex> lock(workerLock_);
    if (!highPriorityTaskQueue_.Empty() || !midPriorityTaskQueue_.Empty() || !lowPriorityTaskQueue_.Empty()) {
        MEDIA_INFO_LOG("task queue is not empty, no need to clear worker threads, taskType:%{public}d", taskType_);
        return false;
    }

    for (auto &threadStatus : threadsStatus_) {
        if (!threadStatus->isThreadWaiting_) {
            MEDIA_INFO_LOG("thread is running, taskType:%{public}d, id:%{public}d, taskNum:%{public}d",
                taskType_, threadStatus->threadId_, threadStatus->taskNum_);
            return false;
        }
    }
    isThreadRunning_ = false;
    return true;
}

void ThumbnailGenerateWorker::TryClearWorkerThreads()
{
    std::unique_lock<std::mutex> lock(taskMutex_);
    if (!IsAllThreadWaiting()) {
        return;
    }
    
    ClearWorkerThreads();
}

void ThumbnailGenerateWorker::RegisterWorkerTimer()
{
    Utils::Timer::TimerCallback timerCallback = [this]() {
        MEDIA_INFO_LOG("ThumbnailGenerateWorker timerCallback, ClearWorkerThreads, taskType:%{public}d", taskType_);
        TryClearWorkerThreads();
    };

    std::lock_guard<std::mutex> lock(timerMutex_);
    if (timerId_ == 0) {
        MEDIA_INFO_LOG("ThumbnailGenerateWorker timer Setup, taskType:%{public}d", taskType_);
        timer_.Setup();
    } else {
        timer_.Unregister(timerId_);
    }
    
    timerId_ = timer_.Register(timerCallback, CLOSE_THUMBNAIL_WORKER_TIME_INTERVAL, false);
    MEDIA_INFO_LOG("ThumbnailGenerateWorker timer Restart, taskType:%{public}d, timeId:%{public}u",
        taskType_, timerId_);
}

void ThumbnailGenerateWorker::TryCloseTimer()
{
    std::lock_guard<std::mutex> lock(timerMutex_);
    if (threads_.empty() && timerId_ != 0) {
        timer_.Unregister(timerId_);
        timer_.Shutdown();
        timerId_ = 0;
        MEDIA_INFO_LOG("ThumbnailGenerateWorker timer Shutdown, taskType:%{public}d", taskType_);
    }
}
} // namespace Media
} // namespace OHOS
/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "medialibrary_async_worker.h"

#include <pthread.h>
#include "media_log.h"

using namespace std;

namespace OHOS {
namespace Media {
static const int32_t SUCCESS = 0;
static const int32_t BG_SLEEP_COUNT = 500;
static const int32_t FG_SLEEP_COUNT = 50;
static const int32_t REST_FOR_MILLISECOND = 20;
static const int32_t REST_FOR_LONG_SECOND = 2;
static const int32_t THREAD_NUM = 2;
shared_ptr<MediaLibraryAsyncWorker> MediaLibraryAsyncWorker::asyncWorkerInstance_{nullptr};
mutex MediaLibraryAsyncWorker::instanceLock_;

shared_ptr<MediaLibraryAsyncWorker> MediaLibraryAsyncWorker::GetInstance()
{
    if (asyncWorkerInstance_ == nullptr) {
        lock_guard<mutex> lockGuard(instanceLock_);
        asyncWorkerInstance_ = shared_ptr<MediaLibraryAsyncWorker>(new MediaLibraryAsyncWorker());
        if (asyncWorkerInstance_ != nullptr) {
            asyncWorkerInstance_->Init();
        }
    }
    return asyncWorkerInstance_;
}

MediaLibraryAsyncWorker::MediaLibraryAsyncWorker() : isThreadRunning_(false), doneTotal_(0)
{}

MediaLibraryAsyncWorker::~MediaLibraryAsyncWorker()
{
    isThreadRunning_ = false;
    bgWorkCv_.notify_all();
    for (auto &thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    asyncWorkerInstance_ = nullptr;
}

void MediaLibraryAsyncWorker::Init()
{
    isThreadRunning_ = true;
    doneTotal_ = 0;
    for (auto i = 0; i < THREAD_NUM; i++) {
        threads_.emplace_back(
            std::thread([this, num = i]() { this->StartWorker(num); })
        );
    }
}

void MediaLibraryAsyncWorker::Interrupt()
{
    ReleaseBgTask();
}

void MediaLibraryAsyncWorker::Stop()
{
    ReleaseBgTask();
    ReleaseFgTask();
}

int32_t MediaLibraryAsyncWorker::AddTask(const shared_ptr<MediaLibraryAsyncTask> &task, bool isFg)
{
    if (isFg) {
        lock_guard<mutex> lockGuard(fgTaskLock_);
        fgTaskQueue_.push(task);
    } else {
        lock_guard<mutex> lockGuard(bgTaskLock_);
        bgTaskQueue_.push(task);
    }

    bgWorkCv_.notify_one();
    return SUCCESS;
}

shared_ptr<MediaLibraryAsyncTask> MediaLibraryAsyncWorker::GetFgTask()
{
    lock_guard<mutex> lockGuard(fgTaskLock_);
    if (fgTaskQueue_.empty()) {
        return nullptr;
    }
    shared_ptr<MediaLibraryAsyncTask> task = fgTaskQueue_.front();
    fgTaskQueue_.pop();
    return task;
}

void MediaLibraryAsyncWorker::ReleaseFgTask()
{
    lock_guard<mutex> lockGuard(fgTaskLock_);
    std::queue<std::shared_ptr<MediaLibraryAsyncTask>> tmp;
    fgTaskQueue_.swap(tmp);
}

shared_ptr<MediaLibraryAsyncTask> MediaLibraryAsyncWorker::GetBgTask()
{
    lock_guard<mutex> lockGuard(bgTaskLock_);
    if (bgTaskQueue_.empty()) {
        return nullptr;
    }
    shared_ptr<MediaLibraryAsyncTask> task = bgTaskQueue_.front();
    bgTaskQueue_.pop();
    return task;
}

void MediaLibraryAsyncWorker::ReleaseBgTask()
{
    lock_guard<mutex> lockGuard(bgTaskLock_);
    std::queue<std::shared_ptr<MediaLibraryAsyncTask>> tmp;
    bgTaskQueue_.swap(tmp);
}

bool MediaLibraryAsyncWorker::IsFgQueueEmpty()
{
    lock_guard<mutex> lock_Guard(fgTaskLock_);
    return fgTaskQueue_.empty();
}

bool MediaLibraryAsyncWorker::IsBgQueueEmpty()
{
    lock_guard<mutex> lock_Guard(bgTaskLock_);
    return bgTaskQueue_.empty();
}

void MediaLibraryAsyncWorker::ClearRefreshTaskQueue()
{
    lock_guard<mutex> lockGuardFg(fgTaskLock_);
    std::queue<std::shared_ptr<MediaLibraryAsyncTask>> tmp;
    while (!fgTaskQueue_.empty()) {
        auto task = fgTaskQueue_.front();
        fgTaskQueue_.pop();
        if (task->taskType_ != TaskType::REFRESH_ALBUM) {
            tmp.push(task);
        }
    }
    fgTaskQueue_.swap(tmp);

    std::queue<std::shared_ptr<MediaLibraryAsyncTask>> tail;
    tmp.swap(tail);

    lock_guard<mutex> lockGuardBg(bgTaskLock_);
    while (!bgTaskQueue_.empty()) {
        auto task = bgTaskQueue_.front();
        bgTaskQueue_.pop();
        if (task->taskType_ != TaskType::REFRESH_ALBUM) {
            tmp.push(task);
        }
    }
    bgTaskQueue_.swap(tmp);
}

void MediaLibraryAsyncWorker::WaitForTask()
{
    std::unique_lock<std::mutex> lock(bgWorkLock_);
    bgWorkCv_.wait(lock,
        [this]() { return !isThreadRunning_ || !IsFgQueueEmpty() || !IsBgQueueEmpty(); });
}

void MediaLibraryAsyncWorker::SleepFgWork()
{
    if ((doneTotal_.load() % FG_SLEEP_COUNT) == 0) {
        this_thread::sleep_for(chrono::milliseconds(REST_FOR_MILLISECOND));
    }
}

void MediaLibraryAsyncWorker::SleepBgWork()
{
    this_thread::sleep_for(chrono::milliseconds(REST_FOR_MILLISECOND));
    if ((doneTotal_.load() % BG_SLEEP_COUNT) == 0) {
        this_thread::sleep_for(chrono::seconds(REST_FOR_LONG_SECOND));
    }
}

void MediaLibraryAsyncWorker::StartWorker(int num)
{
    string name("MediaLibraryAsyncWorker");
    name.append(to_string(num));
    pthread_setname_np(pthread_self(), name.c_str());
    while (true) {
        WaitForTask();
        if (!isThreadRunning_) {
            return;
        }
        if (!IsFgQueueEmpty()) {
            shared_ptr<MediaLibraryAsyncTask> fgTask = GetFgTask();
            if (fgTask != nullptr) {
                fgTask->executor_(fgTask->data_);
                fgTask = nullptr;
                doneTotal_++;
            }
        } else if (!IsBgQueueEmpty()) {
            shared_ptr<MediaLibraryAsyncTask> bgTask = GetBgTask();
            if (bgTask != nullptr) {
                bgTask->executor_(bgTask->data_);
                bgTask = nullptr;
                doneTotal_++;
                SleepBgWork();
            }
        }
    }
}
} // namespace Media
} // namespace OHOS

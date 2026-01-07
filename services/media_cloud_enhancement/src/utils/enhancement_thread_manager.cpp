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

#define MLOG_TAG "EnhancementThreadManager"

#include "enhancement_thread_manager.h"

#include "enhancement_service_callback.h"
#include "media_log.h"

using namespace std;

namespace OHOS {
namespace Media {
static constexpr int32_t WAIT_TIME = 30;
static constexpr int32_t WAIT_RELEASE = 50;

EnhancementThreadManager::EnhancementThreadManager()
{
    stop = false;
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    isThreadAlive = true;
    thread(&EnhancementThreadManager::DealWithTasks, this).detach();
#endif
}

EnhancementThreadManager::~EnhancementThreadManager()
{
    stop = true;
    condVar_.notify_all();
    unique_lock<mutex> lock(releaseMutex_);
    releaseVar_.wait_for(lock, chrono::milliseconds(WAIT_RELEASE), [this]() {
        return isThreadAlive == false;
    });
}

void EnhancementThreadManager::StartConsumerThread()
{
    if (!isThreadAlive) {
        isThreadAlive = true;
        thread(&EnhancementThreadManager::DealWithTasks, this).detach();
    }
}

void EnhancementThreadManager::OnProducerCallback(CloudEnhancementThreadTask& task)
{
    {
        lock_guard<mutex> lock(queueMutex_);
        taskQueue_.push(task);
        StartConsumerThread();
    }
    condVar_.notify_one();
}

void EnhancementThreadManager::DealWithTasks()
{
    MEDIA_INFO_LOG("cloud enhancement consumer thread start");
    bool loopCondition = true;
    while (loopCondition) {
        bool needExtraWork = false;
        CloudEnhancementThreadTask task("", 0, nullptr, 0, false, nullptr, 0);
        {
            unique_lock<mutex> lock(queueMutex_);
            if (condVar_.wait_for(lock, chrono::seconds(WAIT_TIME), [this]() {
                return !taskQueue_.empty() || stop;
            })) {
                if (stop && taskQueue_.empty()) {
                    loopCondition = false;
                    break;
                }
                task = taskQueue_.front();
                taskQueue_.pop();

                if (taskQueue_.empty()) {
                    needExtraWork = true;
                }
            } else {
                loopCondition = false;
                break;
            }
        }
        if (task.taskId.empty()) {
            continue;
        }
        task.isSuccessed ? ExecSuccessedTask(task) : ExecFailedTask(task);
    }
    MEDIA_INFO_LOG("cloud enhancement thread task queue is empty for %{public}d seconds", WAIT_TIME);
    isThreadAlive = false;
}

void EnhancementThreadManager::ExecSuccessedTask(CloudEnhancementThreadTask& task)
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    EnhancementServiceCallback::DealWithSuccessedTask(task);
#endif
}

void EnhancementThreadManager::ExecFailedTask(CloudEnhancementThreadTask& task)
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    EnhancementServiceCallback::DealWithFailedTask(task);
#endif
}

void EnhancementThreadManager::ExecExtraWork()
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    EnhancementServiceCallback::UpdateAlbumsForCloudEnhancement();
#endif
}
} // namespace Media
} // namespace OHOS
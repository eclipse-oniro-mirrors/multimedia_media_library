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

#ifndef FRAMEWORKS_SERVICES_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_ENHANCEMENT_THREAD_MANAGER_H
#define FRAMEWORKS_SERVICES_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_ENHANCEMENT_THREAD_MANAGER_H

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>

namespace OHOS {
namespace Media {
struct CloudEnhancementThreadTask {
    std::string taskId;
    int32_t statusCode;
    uint8_t *addr;
    uint32_t bytes;
    bool isSuccessed;
    uint8_t *videoAddr;
    uint32_t videoBytes;
    CloudEnhancementThreadTask(const std::string& taskId, int32_t statusCode, uint8_t *addr, uint32_t bytes,
        bool isSuccessed, uint8_t *videoAddr, uint32_t videoBytes)
        : taskId(taskId), statusCode(statusCode), addr(addr), bytes(bytes),
        isSuccessed(isSuccessed), videoAddr(videoAddr), videoBytes(videoBytes) {}
};

class EnhancementThreadManager {
public:
    EnhancementThreadManager();
    ~EnhancementThreadManager();
    void StartConsumerThread();
    void OnProducerCallback(CloudEnhancementThreadTask& task);

private:
    std::atomic<bool> stop;
    std::atomic<bool> isThreadAlive;
    std::mutex queueMutex_;
    std::condition_variable condVar_;
    std::mutex releaseMutex_;
    std::condition_variable releaseVar_;
    std::queue<CloudEnhancementThreadTask> taskQueue_;

    void DealWithTasks();
    void ExecSuccessedTask(CloudEnhancementThreadTask& task);
    void ExecFailedTask(CloudEnhancementThreadTask& task);
    void ExecExtraWork();
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_ENHANCEMENT_THREAD_MANAGER_H
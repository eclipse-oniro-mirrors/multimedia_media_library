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

#ifndef FRAMEWORKS_SERVICES_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_ENHANCEMENT_TASK_MANAGER_H
#define FRAMEWORKS_SERVICES_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_ENHANCEMENT_TASK_MANAGER_H

#include <mutex>
#include <string>
#include <unordered_map>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

constexpr int32_t TYPE_MANUAL_ENHANCEMENT = 0;
constexpr int32_t TYPE_AUTO_ENHANCEMENT_FOREGROUND = 1;
constexpr int32_t TYPE_AUTO_ENHANCEMENT_BACKGROUND = 2;

struct EnhancementTaskInfo {
    std::string taskId;
    int32_t fileId;
    int32_t requestCount;
    int32_t taskType;
    EnhancementTaskInfo() : taskId(""), fileId(0), requestCount(0), taskType(0) {}
    EnhancementTaskInfo(std::string taskId, int32_t fileId, int32_t count, int32_t taskType)
        : taskId(taskId), fileId(fileId), requestCount(count), taskType(taskType) {}
};

class EnhancementTaskManager {
public:
    EXPORT static void AddEnhancementTask(int32_t fileId, const std::string &photoId,
        int32_t taskType);
    EXPORT static void RemoveEnhancementTask(const std::string &photoId);
    EXPORT static void RemoveAllEnhancementTask(std::vector<std::string> &taskIds);
    EXPORT static bool InProcessingTask(const std::string &photoId);
    EXPORT static std::string QueryPhotoIdByFileId(int32_t fileId);
    EXPORT static int32_t QueryTaskTypeByPhotoId(const std::string &photoId);
    EXPORT static void SetTaskRequestCount(const std::string &photoId, int32_t count);
    EXPORT static int32_t GetTaskRequestCount(const std::string &photoId);

private:
    // key: photo_id
    EXPORT static std::unordered_map<std::string, std::shared_ptr<EnhancementTaskInfo>> taskInProcess_;
    EXPORT static std::unordered_map<int32_t, std::string> fileId2PhotoId_;

    static std::mutex mutex_;
};
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_SERVICES_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_ENHANCEMENT_TASK_MANAGER_H
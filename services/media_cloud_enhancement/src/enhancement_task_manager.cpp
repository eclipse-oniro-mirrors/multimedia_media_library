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

#define MLOG_TAG "EnhancementTaskManager"

#include "media_log.h"
#include "enhancement_task_manager.h"

using namespace std;

namespace OHOS {
namespace Media {
unordered_map<string, shared_ptr<EnhancementTaskInfo>>
    EnhancementTaskManager::taskInProcess_ = {};
unordered_map<int32_t, string> EnhancementTaskManager::fileId2PhotoId_ = {};
mutex EnhancementTaskManager::mutex_;

void EnhancementTaskManager::AddEnhancementTask(int32_t fileId, const string &photoId,
    int32_t taskType)
{
    unique_lock<mutex> lock(mutex_);
    fileId2PhotoId_.emplace(fileId, photoId);
    taskInProcess_.emplace(photoId, make_shared<EnhancementTaskInfo>(photoId, fileId, 0, taskType));
    taskInProcess_[photoId]->taskType = taskType;
}

void EnhancementTaskManager::RemoveEnhancementTask(const std::string &photoId)
{
    unique_lock<mutex> lock(mutex_);
    if (taskInProcess_.find(photoId) == taskInProcess_.end()) {
        return;
    }
    int32_t fileId = taskInProcess_[photoId]->fileId;
    fileId2PhotoId_.erase(fileId);
    taskInProcess_.erase(photoId);
}

void EnhancementTaskManager::RemoveAllEnhancementTask(vector<string> &taskIds)
{
    unique_lock<mutex> lock(mutex_);
    transform(taskInProcess_.begin(), taskInProcess_.end(), back_inserter(taskIds),
        [](const auto& pair) { return pair.first; });
    fileId2PhotoId_.clear();
    taskInProcess_.clear();
}

bool EnhancementTaskManager::InProcessingTask(const string &photoId)
{
    unique_lock<mutex> lock(mutex_);
    if (photoId.empty() || taskInProcess_.find(photoId) == taskInProcess_.end()) {
        return false;
    }
    return true;
}

string EnhancementTaskManager::QueryPhotoIdByFileId(int32_t fileId)
{
    unique_lock<mutex> lock(mutex_);
    if (fileId2PhotoId_.find(fileId) != fileId2PhotoId_.end()) {
        return fileId2PhotoId_[fileId];
    }
    return "";
}

int32_t EnhancementTaskManager::QueryTaskTypeByPhotoId(const std::string& photoId)
{
    unique_lock<mutex> lock(mutex_);
    if (taskInProcess_.find(photoId) != taskInProcess_.end()) {
        return taskInProcess_[photoId]->taskType;
    }
    return -1;
}

void EnhancementTaskManager::SetTaskRequestCount(const string &photoId, int32_t count)
{
    unique_lock<mutex> lock(mutex_);
    if (taskInProcess_.find(photoId) != taskInProcess_.end()) {
        taskInProcess_[photoId]->requestCount = count;
    }
}

int32_t EnhancementTaskManager::GetTaskRequestCount(const string &photoId)
{
    unique_lock<mutex> lock(mutex_);
    if (taskInProcess_.find(photoId) != taskInProcess_.end()) {
        return taskInProcess_[photoId]->requestCount;
    }
    return -1;
}
} // namespace Media
} // namespace OHOS
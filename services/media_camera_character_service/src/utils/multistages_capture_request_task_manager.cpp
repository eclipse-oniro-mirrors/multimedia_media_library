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

#define MLOG_TAG "MultiStagesCaptureRequestTaskManager"

#include "media_log.h"
#include "multistages_capture_request_task_manager.h"
#include "medialibrary_errno.h"

using namespace std;

namespace OHOS {
namespace Media {
std::unordered_map<int32_t, std::string> MultiStagesCaptureRequestTaskManager::fileId2PhotoId_ = {};
std::unordered_map<std::string, std::shared_ptr<LowQualityPhotoInfo>>
    MultiStagesCaptureRequestTaskManager::photoIdInProcess_ = {};
std::mutex MultiStagesCaptureRequestTaskManager::mutex_;

void MultiStagesCaptureRequestTaskManager::AddPhotoInProgress(int32_t fileId, const string &photoId, bool isTrashed)
{
    unique_lock<mutex> lock(mutex_);
    fileId2PhotoId_.emplace(fileId, photoId);
    PhotoState state = PhotoState::NORMAL;
    if (isTrashed) {
        state = PhotoState::TRASHED;
    }
    photoIdInProcess_.emplace(photoId, make_shared<LowQualityPhotoInfo>(fileId, state, 0));
}

// 1. RestoreImage,从回收站恢复,isTrashed=false, state TRASHED => NORMAL
// 2. 删除到回收站,isTrashed=false, state NORMAL => TRASHED
void MultiStagesCaptureRequestTaskManager::UpdatePhotoInProgress(const string &photoId)
{
    unique_lock<mutex> lock(mutex_);
    if (photoIdInProcess_.count(photoId) == 0) {
        MEDIA_INFO_LOG("photo id (%{public}s) not in progress", photoId.c_str());
        return;
    }
    shared_ptr<LowQualityPhotoInfo> photo = photoIdInProcess_.at(photoId);
    CHECK_AND_RETURN_LOG((photo != nullptr), "UpdatePhotoInProgress failed, photo is nullptr");
    photo->state = (photo->state == PhotoState::NORMAL) ? PhotoState::TRASHED : PhotoState::NORMAL;
    photoIdInProcess_[photoId] = photo;
}

void MultiStagesCaptureRequestTaskManager::RemovePhotoInProgress(const string &photoId, bool isRestorable)
{
    if (!isRestorable) {
        unique_lock<mutex> lock(mutex_);
        if (photoIdInProcess_.count(photoId) == 0) {
            MEDIA_INFO_LOG("photo id (%{public}s) not in progress.", photoId.c_str());
            return;
        }
        int32_t fileId = photoIdInProcess_.at(photoId)->fileId;
        fileId2PhotoId_.erase(fileId);
        photoIdInProcess_.erase(photoId);
        return;
    }

    UpdatePhotoInProgress(photoId);
}

int32_t MultiStagesCaptureRequestTaskManager::UpdatePhotoInProcessRequestCount(const std::string &photoId,
    RequestType requestType)
{
    unique_lock<mutex> lock(mutex_);
    if (photoIdInProcess_.count(photoId) == 0) {
        MEDIA_INFO_LOG("photo id (%{public}s) not in progress.", photoId.c_str());
        return 0;
    }

    shared_ptr<LowQualityPhotoInfo> photo = photoIdInProcess_.at(photoId);
    CHECK_AND_RETURN_RET_LOG((photo != nullptr), 0, "UpdatePhotoInProcessRequestCount failed, photo is nullptr");
    photo->requestCount += (int32_t) requestType;
    photoIdInProcess_[photoId] = photo;
    return photo->requestCount;
}

bool MultiStagesCaptureRequestTaskManager::ClearPhotoInProcessRequestCount(const string &photoId)
{
    unique_lock<mutex> lock(mutex_);
    if (photoId.empty() || photoIdInProcess_.find(photoId) == photoIdInProcess_.end()) {
        return false;
    }
    shared_ptr<LowQualityPhotoInfo> photo = photoIdInProcess_.at(photoId);
    CHECK_AND_RETURN_RET_LOG((photo != nullptr), false, "ClearPhotoInProcessRequestCount failed, photo is nullptr");
    photo->requestCount = 0;
    photoIdInProcess_[photoId] = photo;
    return true;
}

bool MultiStagesCaptureRequestTaskManager::IsPhotoInProcess(const string &photoId)
{
    unique_lock<mutex> lock(mutex_);
    if (photoId.empty() || photoIdInProcess_.find(photoId) == photoIdInProcess_.end()) {
        return false;
    }
    return true;
}

std::string MultiStagesCaptureRequestTaskManager::GetProcessingPhotoId(int32_t fileId)
{
    unique_lock<mutex> lock(mutex_);
    if (fileId2PhotoId_.find(fileId) == fileId2PhotoId_.end()) {
        MEDIA_ERR_LOG("photo not in process, id=%{public}d", fileId);
        return "";
    }
    return fileId2PhotoId_[fileId];
}

int32_t MultiStagesCaptureRequestTaskManager::GetProcessingFileId(const std::string &photoId, int32_t &fileId)
{
    unique_lock<mutex> lock(mutex_);
    for (auto iter = fileId2PhotoId_.begin(); iter != fileId2PhotoId_.end(); ++iter) {
        if (iter->second == photoId) {
            fileId = iter->first;
            return E_OK;
        }
    }
    MEDIA_INFO_LOG("photo id (%{public}s) not in progress.", photoId.c_str());
    return E_ERR;
}

} // Media
} // OHOS
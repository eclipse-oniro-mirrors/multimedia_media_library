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

#ifndef MULTISTAGES_CAPTURE_REQUEST_TASK_MANAGER_H
#define MULTISTAGES_CAPTURE_REQUEST_TASK_MANAGER_H

#include <mutex>
#include <string>
#include <unordered_set>
#include <unordered_map>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
enum class PhotoState : int32_t {
    NORMAL = 0, // state of photo in normal album
    TRASHED,    // state of photo in trashed album
    DELETED,    // state of photo delete from trashed album
};

enum class RequestType : int32_t {
    CANCEL_REQUEST = -1,
    REQUEST = 1,
};

struct LowQualityPhotoInfo {
    int32_t fileId;
    PhotoState state;
    int32_t requestCount;
    LowQualityPhotoInfo() : fileId(0), state(PhotoState::NORMAL), requestCount(0) {}
    LowQualityPhotoInfo(int32_t fileId, PhotoState state, int32_t count)
        : fileId(fileId), state(state), requestCount(count) {}
};

class MultiStagesCaptureRequestTaskManager {
public:
    EXPORT static void AddPhotoInProgress(int32_t fileId, const std::string &photoId, bool isTrashed);
    EXPORT static void RemovePhotoInProgress(const std::string &photoId, bool isRestorable);
    static void UpdatePhotoInProgress(const std::string &photoId);
    static bool ClearPhotoInProcessRequestCount(const std::string &photoId);
    static bool IsPhotoInProcess(const std::string &photoId);
    static int32_t UpdatePhotoInProcessRequestCount(const std::string &photoId, RequestType requestType);
    static std::string GetProcessingPhotoId(int32_t fileId);
    static int32_t GetProcessingFileId(const std::string &photoId, int32_t &fileId);

private:
    // key: file_id, value: photo_id
    EXPORT static std::unordered_map<int32_t, std::string> fileId2PhotoId_;

    // key: photo_id
    EXPORT static std::unordered_map<std::string, std::shared_ptr<LowQualityPhotoInfo>> photoIdInProcess_;

    static std::mutex mutex_;
};
} // Media
} // OHOS
#endif // MULTISTAGES_CAPTURE_REQUEST_TASK_MANAGER_H
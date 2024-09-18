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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTI_STAGES_CAPTURE_MANAGER_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTI_STAGES_CAPTURE_MANAGER_H

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <mutex>

#include "deferred_processing_adapter.h"
#include "medialibrary_type_const.h"
#include "medialibrary_command.h"
#include "result_set.h"
#include "picture_manager_thread.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

struct MultiStagesPhotoInfo {
    std::string photoId;
    int32_t photoQuality;
    int32_t fileId;
    MultiStagesPhotoInfo(int32_t fileId, const std::string &id, int32_t quality);
    MultiStagesPhotoInfo();
};

class MultiStagesCaptureManager {
public:
    EXPORT static MultiStagesCaptureManager& GetInstance();
    bool Init();

    EXPORT int32_t UpdateDbInfo(MediaLibraryCommand &cmd);
    void UpdateLocation(int32_t fileId, const std::string &path, double longitude, double latitude);

    std::shared_ptr<OHOS::NativeRdb::ResultSet> HandleMultiStagesOperation(MediaLibraryCommand &cmd,
        const std::vector<std::string> &columns);

    EXPORT bool CancelProcessRequest(const std::string &photoId);
    void RemoveImage(const std::string &photoId, bool isRestorable = true);
    void RemoveImages(const NativeRdb::AbsRdbPredicates &predicates, bool isRestorable = true);
    void RestoreImages(const NativeRdb::AbsRdbPredicates &predicates);
    EXPORT void AddImage(int32_t fileId, const std::string &photoId, int32_t deferredProcType);
    void ProcessImage(int fileId, int deliveryMode);

    void AddImageInternal(int32_t fileId, const std::string &photoId, int32_t deferredProcType,
        bool discardable = false);
    bool IsHighQualityPhotoExist(const std::string &uri);
    void DealHighQualityPicture(const std::string &imageId, std::shared_ptr<Media::Picture> picture,
        bool isEdited = false);
    void DealLowQualityPicture(const std::string &imageId, std::shared_ptr<Media::Picture> picture,
        bool isEdited = false);
    void SaveLowQualityImageInfo(MediaLibraryCommand &cmd);

    EXPORT bool IsPhotoDeleted(const std::string &photoId);

    EXPORT void SyncWithDeferredProcSession();
    EXPORT void SyncWithDeferredProcSessionInternal();

private:
    MultiStagesCaptureManager();
    ~MultiStagesCaptureManager();
    MultiStagesCaptureManager(const MultiStagesCaptureManager &manager) = delete;
    const MultiStagesCaptureManager &operator=(const MultiStagesCaptureManager &manager) = delete;

    EXPORT std::vector<std::shared_ptr<MultiStagesPhotoInfo>> GetPhotosInfo(
        const NativeRdb::AbsRdbPredicates &predicates);
    void CancelRequestAndRemoveImage(const std::vector<std::string> &columns);
    void AddImage(MediaLibraryCommand &cmd);
    int32_t UpdatePictureQuality(const std::string &photoId);

    std::unordered_set<int32_t> setOfDeleted_;

    std::shared_ptr<DeferredProcessingAdapter> deferredProcSession_;

    std::mutex deferredProcMutex_;
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTI_STAGES_CAPTURE_MANAGER_H
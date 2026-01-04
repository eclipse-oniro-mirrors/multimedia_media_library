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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTI_STAGES_PHOTO_CAPTURE_MANAGER_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTI_STAGES_PHOTO_CAPTURE_MANAGER_H

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <mutex>

#include "deferred_photo_proc_adapter.h"
#include "medialibrary_type_const.h"
#include "medialibrary_command.h"
#include "result_set.h"
#include "picture_manager_thread.h"
#include "add_image_dto.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

class MultiStagesPhotoCaptureManager {
public:
    EXPORT static MultiStagesPhotoCaptureManager& GetInstance();
    bool Init();

    EXPORT int32_t UpdateDbInfo(MediaLibraryCommand &cmd);
    static void UpdateLocation(const NativeRdb::ValuesBucket &values, bool isWriteGpsAdvanced,
        const std::string &path = "", const int32_t &id = 0);

    std::shared_ptr<OHOS::NativeRdb::ResultSet> HandleMultiStagesOperation(MediaLibraryCommand &cmd,
        const std::vector<std::string> &columns);

    EXPORT bool CancelProcessRequest(const std::string &photoId);
    void RemoveImage(const std::string &photoId, bool isRestorable = true);
    void RestoreImage(const std::string &photoId);
    EXPORT void AddImage(int32_t fileId, const std::string &photoId, int32_t deferredProcType,
        const std::string &packageName = "");
    void AddImage(AddImageDto &dto);
    void AddImage(MediaLibraryCommand &cmd);
    void ProcessImage(int fileId, int deliveryMode);

    void AddImageInternal(int32_t fileId, const std::string &photoId, int32_t deferredProcType,
        bool discardable = false, const std::string &packageName = "");
    bool IsHighQualityPhotoExist(const std::string &uri);
    void DealHighQualityPicture(const std::string &imageId, std::shared_ptr<Media::Picture> picture,
        bool isEdited = false, bool isTakeEffect = false);
    void DealLowQualityPicture(const std::string &imageId, std::shared_ptr<Media::Picture> picture,
        bool isEdited = false);
    void SaveLowQualityImageInfo(MediaLibraryCommand &cmd);
    void SaveLowQualityPicture(const std::string &imageId);

    EXPORT bool IsPhotoDeleted(const std::string &photoId);

    EXPORT void SyncWithDeferredProcSession();
    EXPORT void SyncWithDeferredProcSessionInternal();
    void NotifyProcessImage();
#ifdef ABILITY_CAMERA_SUPPORT
    void SetProcessImageDoneCallback(const OHOS::Media::ProcessDoneHandler &func);
#endif

private:
    MultiStagesPhotoCaptureManager();
    ~MultiStagesPhotoCaptureManager();
    MultiStagesPhotoCaptureManager(const MultiStagesPhotoCaptureManager &manager) = delete;
    const MultiStagesPhotoCaptureManager &operator=(const MultiStagesPhotoCaptureManager &manager) = delete;

    void CancelRequestAndRemoveImage(const std::vector<std::string> &columns);
    int32_t UpdatePictureQuality(const std::string &photoId);
    int32_t UpdatePictureQualityByFileId(int32_t fileId);

    std::unordered_set<int32_t> setOfDeleted_;

    std::shared_ptr<DeferredPhotoProcessingAdapter> deferredProcSession_;
    
    std::mutex deferredProcMutex_;
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTI_STAGES_PHOTO_CAPTURE_MANAGER_H

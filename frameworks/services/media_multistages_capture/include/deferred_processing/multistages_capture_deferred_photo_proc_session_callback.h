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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_MULTISTAGES_CAPTURE_DEFERRED_PHOTO_PROC_SESSION_CALLBACK_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_MULTISTAGES_CAPTURE_DEFERRED_PHOTO_PROC_SESSION_CALLBACK_H

#ifdef ABILITY_CAMERA_SUPPORT
#include <memory>
#include <string>

#include "deferred_photo_proc_session.h"
#include "result_set_utils.h"
#include "medialibrary_command.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class MultiStagesCaptureDeferredPhotoProcSessionCallback : public CameraStandard::IDeferredPhotoProcSessionCallback {
public:
    EXPORT MultiStagesCaptureDeferredPhotoProcSessionCallback();
    EXPORT ~MultiStagesCaptureDeferredPhotoProcSessionCallback();

    void OnProcessImageDone(const std::string &imageId, const uint8_t *addr, const long bytes,
        uint32_t cloudImageEnhanceFlag) override;
    void OnProcessImageDone(const std::string &imageId, std::shared_ptr<Media::Picture> picture,
        uint32_t cloudImageEnhanceFlag) override;
    void OnDeliveryLowQualityImage(const std::string &imageId, std::shared_ptr<Media::Picture> picture) override;
    EXPORT void OnError(const std::string &imageId, const CameraStandard::DpsErrorCode error) override;
    void OnStateChanged(const CameraStandard::DpsStatusCode state) override;

private:
    EXPORT int32_t UpdatePhotoQuality(const std::string &photoId);
    EXPORT void UpdateCEAvailable(const std::string &photoId, uint32_t cloudImageEnhanceFlag, int32_t modifyType = 0);
    void GetCommandByImageId(const std::string &imageId, MediaLibraryCommand &cmd);
    void UpdateHighQualityPictureInfo(const std::string &imageId, uint32_t cloudImageEnhanceFlag,
        int32_t modifyType = 0);
    void NotifyIfTempFile(std::shared_ptr<NativeRdb::ResultSet> resultSet);
    void ProcessAndSaveHighQualityImage(const std::string& imageId, std::shared_ptr<Media::Picture> picture,
        std::shared_ptr<NativeRdb::ResultSet> resultSet, uint32_t cloudImageEnhanceFlag, int32_t modifyType = 0);
};
} // namespace Media
} // namespace OHOS
#endif
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_MULTISTAGES_CAPTURE_DEFERRED_PHOTO_PROC_SESSION_CALLBACK_H

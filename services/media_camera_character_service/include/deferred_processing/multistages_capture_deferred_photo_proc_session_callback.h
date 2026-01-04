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

#ifndef MULTISTAGES_CAPTURE_DEFERRED_PHOTO_PROC_SESSION_CALLBACK_H
#define MULTISTAGES_CAPTURE_DEFERRED_PHOTO_PROC_SESSION_CALLBACK_H

#ifdef ABILITY_CAMERA_SUPPORT
#include <memory>
#include <string>
#include <sstream>

#include "camera_character_types.h"
#include "deferred_photo_proc_session.h"
#include "result_set_utils.h"
#include "medialibrary_command.h"
#include "user_define_notify_info.h"

namespace OHOS {
namespace Media {
using namespace Notification;
class Picture;
#define EXPORT __attribute__ ((visibility ("default")))
using ProcessDoneHandler = std::function<void(bool, const std::string &)>;
class MultiStagesCaptureDeferredPhotoProcSessionCallback : public CameraStandard::IDeferredPhotoProcSessionCallback {
public:
    EXPORT MultiStagesCaptureDeferredPhotoProcSessionCallback();
    EXPORT ~MultiStagesCaptureDeferredPhotoProcSessionCallback();

    void OnProcessImageDone(
        const std::string& imageId, std::shared_ptr<PictureIntf> picture, const DpsMetadata& metadata) override {};
    void OnProcessImageDone(const std::string &imageId, const uint8_t *addr, const long bytes,
        uint32_t cloudImageEnhanceFlag) override;
    void OnProcessImageDone(const std::string &imageId, std::shared_ptr<CameraStandard::PictureIntf> picture,
        uint32_t cloudImageEnhanceFlag);
    void OnDeliveryLowQualityImage(const std::string &imageId,
        std::shared_ptr<CameraStandard::PictureIntf> picture) override;
    EXPORT void OnError(const std::string &imageId, const CameraStandard::DpsErrorCode error) override;
    void OnStateChanged(const CameraStandard::DpsStatusCode state) override;

    void SetProcessImageDoneCallback(const ProcessDoneHandler &func);

private:
    void HandleForNullData(const std::string &imageId, std::shared_ptr<Media::Picture> picture);
    void HandleForIsTemp(const std::shared_ptr<FileAsset> &fileAsset, std::shared_ptr<Media::Picture> &picture,
        uint32_t cloudImageEnhanceFlag);
    void HandleOnError(const string &imageId, const CameraStandard::DpsErrorCode error);
    void HandleOnProcessImageDone(const string &imageId, const uint8_t *addr,
        const long bytes, uint32_t cloudImageEnhanceFlag);
    void HandleOnProcessImageDone(
        const std::string &imageId, std::shared_ptr<CameraStandard::PictureIntf> pictureIntf,
        uint32_t cloudImageEnhanceFlag);
    EXPORT int32_t UpdatePhotoQuality(const int32_t &fileId);
    EXPORT void UpdatePhotoQuality(const int32_t &fileId, NativeRdb::ValuesBucket &updateValues);
    EXPORT void UpdateCEAvailable(const int32_t &fileId, uint32_t cloudImageEnhanceFlag,
        NativeRdb::ValuesBucket &updateValues, int32_t modifyType = 0);
    EXPORT void GetCommandByImageId(const std::string &imageId, MediaLibraryCommand &cmd);
    EXPORT void UpdateHighQualityPictureInfo(const int32_t &fileId, uint32_t cloudImageEnhanceFlag,
         int32_t modifyType = 0);
    EXPORT void NotifyIfTempFile(const std::shared_ptr<FileAsset> &fileAsset, bool isError = false);
    EXPORT void ProcessAndSaveHighQualityImage(const std::shared_ptr<FileAsset> &fileAsset,
        std::shared_ptr<Media::Picture> picture, uint32_t cloudImageEnhanceFlag);
    void CallProcessImageDone(bool success, const std::string &photoId);

private:
    ProcessDoneHandler processDoneCallback_;
};
} // namespace Media
} // namespace OHOS
#endif
#endif  // MULTISTAGES_CAPTURE_DEFERRED_PHOTO_PROC_SESSION_CALLBACK_H
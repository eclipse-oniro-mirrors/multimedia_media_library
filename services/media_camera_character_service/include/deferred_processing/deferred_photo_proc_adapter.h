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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_DEFERRED_PHOTO_PROC_ADAPTER_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_DEFERRED_PHOTO_PROC_ADAPTER_H

#include <string>

#ifdef ABILITY_CAMERA_SUPPORT
#include "deferred_proc_session/deferred_photo_proc_session.h"
#include "multistages_capture_deferred_photo_proc_session_callback.h"
#endif
namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
// 延时子服务适配器
#ifdef ABILITY_CAMERA_SUPPORT
class DeferredPhotoProcessingAdapter : public RefBase {
#else
class DeferredPhotoProcessingAdapter {
#endif
public:
    EXPORT DeferredPhotoProcessingAdapter();
    EXPORT virtual ~DeferredPhotoProcessingAdapter();

    EXPORT virtual void BeginSynchronize();
    EXPORT virtual void EndSynchronize();
#ifdef ABILITY_CAMERA_SUPPORT
    void AddImage(const std::string &imageId, CameraStandard::DpsMetadata &metadata, const bool isTrashed = false,
        const std::string &packageName = "");
    void SetProcessImageDoneCallback(const OHOS::Media::ProcessDoneHandler &func);
#endif
    EXPORT virtual void RemoveImage(const std::string &imageId, const bool isRestorable = true);
    EXPORT void RestoreImage(const std::string &imageId);
    EXPORT void ProcessImage(const std::string &appName, const std::string &imageId);
    EXPORT bool CancelProcessImage(const std::string &imageId);
    EXPORT void NotifyProcessImage();
private:
#ifdef ABILITY_CAMERA_SUPPORT
    sptr<CameraStandard::DeferredPhotoProcSession> deferredPhotoProcSession_;
    std::shared_ptr<MultiStagesCaptureDeferredPhotoProcSessionCallback> photoProcCallback_{nullptr};
#endif
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_DEFERRED_PHOTO_PROC_ADAPTER_H
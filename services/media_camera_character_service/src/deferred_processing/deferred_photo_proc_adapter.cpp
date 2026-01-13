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

#define MLOG_TAG "DeferredPhotoProcessingAdapter"

#include "deferred_photo_proc_adapter.h"

#ifdef ABILITY_CAMERA_SUPPORT
#include "camera_manager.h"
#endif
#include "ipc_skeleton.h"
#include "media_log.h"

using namespace std;
#ifdef ABILITY_CAMERA_SUPPORT
using namespace OHOS::CameraStandard;
#endif

namespace OHOS {
namespace Media {

DeferredPhotoProcessingAdapter::DeferredPhotoProcessingAdapter()
{
#ifdef ABILITY_CAMERA_SUPPORT
    const static int32_t INVALID_UID = -1;
    const static int32_t BASE_USER_RANGE = 200000;

    int uid = IPCSkeleton::GetCallingUid();
    CHECK_AND_RETURN_LOG(uid > INVALID_UID, "DeferredPhotoProcessingAdapter invalid uid: %{public}d", uid);

    int32_t userId = uid / BASE_USER_RANGE;
    photoProcCallback_ = make_shared<MultiStagesCaptureDeferredPhotoProcSessionCallback>();
    CHECK_AND_RETURN_LOG(photoProcCallback_ != nullptr, "make PhotoProcSessionCallback fail");
    deferredPhotoProcSession_ = CameraManager::CreateDeferredPhotoProcessingSession(userId, photoProcCallback_);
    CHECK_AND_PRINT_LOG(deferredPhotoProcSession_ != nullptr, "CreateDeferredPhotoProcessingSession err");
#endif
    MEDIA_INFO_LOG("DeferredPhotoProcessingAdapter init succ");
}

DeferredPhotoProcessingAdapter::~DeferredPhotoProcessingAdapter() {}

void DeferredPhotoProcessingAdapter::BeginSynchronize()
{
    MEDIA_INFO_LOG("DeferredPhotoProcessingAdapter::BeginSynchronize");
#ifdef ABILITY_CAMERA_SUPPORT
    if (deferredPhotoProcSession_ == nullptr) {
        MEDIA_ERR_LOG("BeginSynchronize deferredPhotoProcSession_ is nullptr");
        return;
    }
    deferredPhotoProcSession_->BeginSynchronize();
#endif
}

void DeferredPhotoProcessingAdapter::EndSynchronize()
{
    MEDIA_INFO_LOG("DeferredPhotoProcessingAdapter::EndSynchronize");
#ifdef ABILITY_CAMERA_SUPPORT
    if (deferredPhotoProcSession_ == nullptr) {
        MEDIA_ERR_LOG("EndSynchronize deferredPhotoProcSession_ is nullptr");
        return;
    }
    deferredPhotoProcSession_->EndSynchronize();
#endif
}

#ifdef ABILITY_CAMERA_SUPPORT
void DeferredPhotoProcessingAdapter::AddImage(const std::string &imageId, DpsMetadata &metadata, const bool isTrashed,
    const std::string &packageName)
{
    MEDIA_INFO_LOG("enter photoid: %{public}s, isTrashed: %{public}d", imageId.c_str(), isTrashed);
    CHECK_AND_RETURN_LOG(deferredPhotoProcSession_ != nullptr, "AddImage deferredPhotoProcSession_ is nullptr");
    deferredPhotoProcSession_->AddImage(imageId, metadata, isTrashed, packageName);
}
#endif

void DeferredPhotoProcessingAdapter::RemoveImage(const std::string &imageId, bool isRestorable)
{
    MEDIA_INFO_LOG("enter photoid: %{public}s, isRestorable: %{public}d", imageId.c_str(), isRestorable);
#ifdef ABILITY_CAMERA_SUPPORT
    CHECK_AND_RETURN_LOG(deferredPhotoProcSession_ != nullptr, "RemoveImage deferredPhotoProcSession_ is nullptr");
    deferredPhotoProcSession_->RemoveImage(imageId, isRestorable);
#endif
}

void DeferredPhotoProcessingAdapter::RestoreImage(const std::string &imageId)
{
    MEDIA_INFO_LOG("enter photoid: %{public}s", imageId.c_str());
#ifdef ABILITY_CAMERA_SUPPORT
    if (deferredPhotoProcSession_ == nullptr) {
        MEDIA_ERR_LOG("RestoreImage deferredPhotoProcSession_ is nullptr");
        return;
    }
    deferredPhotoProcSession_->RestoreImage(imageId);
#endif
}

void DeferredPhotoProcessingAdapter::ProcessImage(const std::string &appName, const std::string &imageId)
{
    MEDIA_INFO_LOG("enter appName: %{public}s, photoid: %{public}s", appName.c_str(), imageId.c_str());
#ifdef ABILITY_CAMERA_SUPPORT
    if (deferredPhotoProcSession_ == nullptr) {
        MEDIA_ERR_LOG("ProcessImage deferredPhotoProcSession_ is nullptr");
        return;
    }
    deferredPhotoProcSession_->ProcessImage(appName, imageId);
#endif
}

bool DeferredPhotoProcessingAdapter::CancelProcessImage(const std::string &imageId)
{
    MEDIA_INFO_LOG("DeferredPhotoProcessingAdapter::CancelProcessImage photoid: %{public}s", imageId.c_str());
#ifdef ABILITY_CAMERA_SUPPORT
    if (deferredPhotoProcSession_ == nullptr) {
        MEDIA_ERR_LOG("CancelProcessImage deferredPhotoProcSession_ is nullptr");
        return false;
    }
    return deferredPhotoProcSession_->CancelProcessImage(imageId);
#else
    return false;
#endif
}

#ifdef ABILITY_CAMERA_SUPPORT
void DeferredPhotoProcessingAdapter::SetProcessImageDoneCallback(const ProcessDoneHandler &func)
{
    photoProcCallback_->SetProcessImageDoneCallback(func);
}
#endif

void DeferredPhotoProcessingAdapter::NotifyProcessImage()
{
    MEDIA_INFO_LOG("DeferredPhotoProcessingAdapter::NotifyProcessImage");
#ifdef ABILITY_CAMERA_SUPPORT
    if (deferredPhotoProcSession_ == nullptr) {
        MEDIA_ERR_LOG("NotifyProcessImage deferredPhotoProcSession_ is nullptr");
        return;
    }
    deferredPhotoProcSession_->NotifyProcessImage();
#endif
}
} // namespace Media
} // namespace OHOS
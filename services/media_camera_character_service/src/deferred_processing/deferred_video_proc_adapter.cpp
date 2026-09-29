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

#define MLOG_TAG "DeferredVideoProcessingAdapter"

#include "deferred_video_proc_adapter.h"

#ifdef ABILITY_CAMERA_SUPPORT
#include "camera_manager.h"
#endif
#include "ipc_skeleton.h"
#include "media_log.h"
#ifdef ABILITY_CAMERA_SUPPORT
#include "multistages_capture_deferred_video_proc_session_callback.h"
#endif
using namespace std;
#ifdef ABILITY_CAMERA_SUPPORT
using namespace OHOS::CameraStandard;
#endif

namespace OHOS {
namespace Media {

enum FdIndex {
    LOW_SRC_FD_INDEX = 0,  // 低源文件描述符索引
    DST_FD_INDEX = 1,      // 目标文件描述符索引
    SRC_FD_INDEX = 2,      // 源文件描述符索引
    SRC_FD_COPY_INDEX = 3  // 源文件描述符副本索引
};

DeferredVideoProcessingAdapter::DeferredVideoProcessingAdapter()
{
#ifdef ABILITY_CAMERA_SUPPORT
    const static int32_t INVALID_UID = -1;
    const static int32_t BASE_USER_RANGE = 200000;

    int uid = IPCSkeleton::GetCallingUid();
    CHECK_AND_RETURN_LOG(uid > INVALID_UID, "CreateDeferredVideoProcessingSession invalid uid: %{public}d", uid);
    int32_t userId = uid / BASE_USER_RANGE;
    deferredVideoProcSession_ = CameraManager::CreateDeferredVideoProcessingSession(userId,
        make_shared<MultiStagesCaptureDeferredVideoProcSessionCallback>());
    CHECK_AND_RETURN_LOG(deferredVideoProcSession_ != nullptr, "CreateDeferredVideoProcessingSession err");
#endif
    MEDIA_INFO_LOG("CreateDeferredVideoProcessingSession succ");
}

DeferredVideoProcessingAdapter::~DeferredVideoProcessingAdapter() {}

void DeferredVideoProcessingAdapter::BeginSynchronize()
{
    MEDIA_INFO_LOG("DeferredVideoProcessingAdapter::BeginSynchronize");
#ifdef ABILITY_CAMERA_SUPPORT
    if (deferredVideoProcSession_ == nullptr) {
        MEDIA_ERR_LOG("BeginSynchronize deferredVideoProcSession_ is nullptr");
        return;
    }
    deferredVideoProcSession_->BeginSynchronize();
#endif
}

void DeferredVideoProcessingAdapter::EndSynchronize()
{
    MEDIA_INFO_LOG("DeferredVideoProcessingAdapter::EndSynchronize");
#ifdef ABILITY_CAMERA_SUPPORT
    if (deferredVideoProcSession_ == nullptr) {
        MEDIA_ERR_LOG("EndSynchronize deferredVideoProcSession_ is nullptr");
        return;
    }
    deferredVideoProcSession_->EndSynchronize();
#endif
}

void DeferredVideoProcessingAdapter::AddVideo(const std::string &videoId, const std::vector<std::string> &srcPath,
    const std::string &sharedTemp1Path, const std::string &sharedTemp2Path)
{
#ifdef ABILITY_CAMERA_SUPPORT
    MEDIA_INFO_LOG("SingleSrcFd, videoId: %{public}s", videoId.c_str());
    CHECK_AND_RETURN_LOG(deferredVideoProcSession_ != nullptr,
        "AddVideo deferredVideoProcSession_ is nullptr");
    CHECK_AND_RETURN_LOG(srcPath.size() >= 2, "srcPath size is invalid"); // 2 edit and photo org path
    deferredVideoProcSession_->AddVideo(videoId, srcPath, sharedTemp1Path, sharedTemp2Path);
#endif
}

void DeferredVideoProcessingAdapter::AddVideo(const std::string &videoId, const std::vector<std::string> &srcPath,
    const std::string &sharedTemp1Path, const std::string &sharedTemp2Path, const std::string &moviePath)
{
#ifdef ABILITY_CAMERA_SUPPORT
    MEDIA_INFO_LOG("DoubleSrcFd, videoId: %{public}s", videoId.c_str());
    CHECK_AND_RETURN_LOG(deferredVideoProcSession_ != nullptr,
        "AddVideo deferredVideoProcSession_ is nullptr");
    CHECK_AND_RETURN_LOG(srcPath.size() >= 2, "srcPath size is invalid"); // 2 edit and photo org path
    deferredVideoProcSession_->AddVideo(videoId, srcPath, sharedTemp1Path, sharedTemp2Path, moviePath);
#endif
}

void DeferredVideoProcessingAdapter::RemoveVideo(const std::string &videoId, const bool isRestorable)
{
    MEDIA_INFO_LOG("DeferredVideoProcessingAdapter::RemoveVideo videoId: %{public}s", videoId.c_str());
#ifdef ABILITY_CAMERA_SUPPORT
    if (deferredVideoProcSession_ == nullptr) {
        MEDIA_ERR_LOG("RemoveVideo deferredVideoProcSession_ is nullptr");
        return;
    }
    deferredVideoProcSession_->RemoveVideo(videoId, isRestorable);
#endif
}

void DeferredVideoProcessingAdapter::RestoreVideo(const std::string &videoId)
{
    MEDIA_INFO_LOG("DeferredVideoProcessingAdapter::RestoreVideo videoId: %{public}s", videoId.c_str());
#ifdef ABILITY_CAMERA_SUPPORT
    if (deferredVideoProcSession_ == nullptr) {
        MEDIA_ERR_LOG("RestoreVideo deferredVideoProcSession_ is nullptr");
        return;
    }
    deferredVideoProcSession_->RestoreVideo(videoId);
#endif
}

void DeferredVideoProcessingAdapter::ProcessVideo(const std::string &appName, const std::string &videoId)
{
    MEDIA_INFO_LOG("DeferredVideoProcessingAdapter::ProcessVideo appName: %{public}s, videoId: %{public}s",
        appName.c_str(), videoId.c_str());
#ifdef ABILITY_CAMERA_SUPPORT
    if (deferredVideoProcSession_ == nullptr) {
        MEDIA_ERR_LOG("ProcessVideo deferredVideoProcSession_ is nullptr");
        return;
    }
    deferredVideoProcSession_->ProcessVideo(appName, videoId);
#endif
}

void DeferredVideoProcessingAdapter::CancelProcessVideo(const std::string &videoId)
{
    MEDIA_INFO_LOG("DeferredVideoProcessingAdapter::CancelProcessVideo videoId: %{public}s", videoId.c_str());
#ifdef ABILITY_CAMERA_SUPPORT
    if (deferredVideoProcSession_ == nullptr) {
        MEDIA_ERR_LOG("CancelProcessVideo deferredVideoProcSession_ is nullptr");
        return;
    }
    deferredVideoProcSession_->CancelProcessVideo(videoId);
    return;
#else
    return;
#endif
}

} // namespace Media
} // namespace OHOS
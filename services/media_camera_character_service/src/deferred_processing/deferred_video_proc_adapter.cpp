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
const int32_t MIN_FDS_LENGTH = 4;

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

void DeferredVideoProcessingAdapter::AddVideo(const std::string &videoId,
    int32_t srcFd, int32_t dstFd)
{
#ifdef ABILITY_CAMERA_SUPPORT
    MEDIA_INFO_LOG("SingleSrcFd, videoId: %{public}s", videoId.c_str());
    CHECK_AND_RETURN_LOG(deferredVideoProcSession_ != nullptr,
        "AddVideo deferredVideoProcSession_ is nullptr");
    CHECK_AND_RETURN_LOG(srcFd >= 0, "AddVideo srcFd is %{public}d", srcFd);
    CHECK_AND_RETURN_LOG(dstFd >= 0, "AddVideo dstFd is %{public}d", dstFd);
    auto src = sptr<IPCFileDescriptor>::MakeSptr(srcFd);
    auto dst = sptr<IPCFileDescriptor>::MakeSptr(dstFd);
    deferredVideoProcSession_->AddVideo(videoId, src, dst);
#endif
}

void DeferredVideoProcessingAdapter::AddVideo(const std::string &videoId, const std::vector<int32_t> &fds)
{
#ifdef ABILITY_CAMERA_SUPPORT
    MEDIA_INFO_LOG("DoubleSrcFd, videoId: %{public}s", videoId.c_str());
    CHECK_AND_RETURN_LOG(deferredVideoProcSession_ != nullptr,
        "AddVideo deferredVideoProcSession_ is nullptr");
    CHECK_AND_RETURN_LOG(fds.size() >= MIN_FDS_LENGTH, "AddVideo fds size < MIN_FDS_LENGTH");
    CHECK_AND_RETURN_LOG(fds[LOW_SRC_FD_INDEX] >= 0, "AddVideo lowSrcFd is %{public}d", fds[LOW_SRC_FD_INDEX]);
    CHECK_AND_RETURN_LOG(fds[DST_FD_INDEX] >= 0, "AddVideo dstFd is %{public}d", fds[DST_FD_INDEX]);
    CHECK_AND_RETURN_LOG(fds[SRC_FD_INDEX] >= 0, "AddVideo srcFd is %{public}d", fds[SRC_FD_INDEX]);
    CHECK_AND_RETURN_LOG(fds[SRC_FD_COPY_INDEX] >= 0, "AddVideo srcFdCopy is %{public}d", fds[SRC_FD_COPY_INDEX]);

    std::vector<sptr<IPCFileDescriptor>> ipcFds;
    for (int32_t fd : fds) {
        auto ipcFd = sptr<IPCFileDescriptor>::MakeSptr(fd);
        ipcFds.emplace_back(ipcFd);
    }

    deferredVideoProcSession_->AddVideo(videoId, ipcFds);
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
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

#ifndef MULTISTAGES_CAPTURE_DEFERRED_VIDEO_PROC_SESSION_CALLBACK_H
#define MULTISTAGES_CAPTURE_DEFERRED_VIDEO_PROC_SESSION_CALLBACK_H

#ifdef ABILITY_CAMERA_SUPPORT
#include <memory>
#include <string>

#include "deferred_video_proc_session.h"
#include "medialibrary_async_worker.h"
#include "result_set_utils.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class MultiStagesCaptureDeferredVideoProcSessionCallback : public CameraStandard::IDeferredVideoProcSessionCallback {
public:
    EXPORT MultiStagesCaptureDeferredVideoProcSessionCallback();
    EXPORT ~MultiStagesCaptureDeferredVideoProcSessionCallback();

    void OnProcessVideoDone(const std::string& videoId) override;
    EXPORT void OnError(const std::string& videoId, const CameraStandard::DpsErrorCode errorCode) override;
    void OnStateChanged(const CameraStandard::DpsStatusCode state) override;
    void OnProcessingProgress(const std::string& videoId, float progress) override;

private:
    static void NotifyIfTempFile(const std::shared_ptr<FileAsset> &fileAsset, bool isError = false);
    static int32_t UpdateVideoQuality(const int32_t &fileId, const std::shared_ptr<FileAsset> &fileAsset,
        bool isSuccess);
    static void AsyncOnErrorProc(const std::string& videoId, const CameraStandard::DpsErrorCode errorCode);
    static void VideoFaileProcAsync(AsyncTaskData *data);
    static void HandleVideoProcFailedAndInvalid(const std::string& videoId,
        const CameraStandard::DpsErrorCode errorCode, const std::shared_ptr<FileAsset> &fileAsset);
    static void HandleVideoProcInterrupted(const std::string& videoId, const CameraStandard::DpsErrorCode errorCode,
        const std::shared_ptr<FileAsset> &fileAsset);
    class VideoFaileProcTaskData : public AsyncTaskData {
    public:
        VideoFaileProcTaskData(const std::string& videoId, const CameraStandard::DpsErrorCode errorCode)
            : videoId_(std::move(videoId)), errorCode_(errorCode) {}
        virtual ~VideoFaileProcTaskData() override = default;
        std::string videoId_;
        CameraStandard::DpsErrorCode errorCode_;
    };
};
} // namespace Media
} // namespace OHOS
#endif
#endif  // MULTISTAGES_CAPTURE_DEFERRED_VIDEO_PROC_SESSION_CALLBACK_H

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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTI_STAGES_VIDEO_CAPTURE_MANAGER_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTI_STAGES_VIDEO_CAPTURE_MANAGER_H

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <mutex>

#include "deferred_video_proc_adapter.h"
#include "medialibrary_type_const.h"
#include "medialibrary_command.h"
#include "result_set.h"
#include "multistages_capture_request_task_manager.h"
#include "add_process_video_dto.h"
#include "get_progress_callback_vo.h"
#include "process_video_dto.h"
#include "save_camera_photo_dto.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

enum class VideoCount : int32_t {
    SINGLE = 1, // 单路流
    DOUBLE = 2, // 双路流
};
 
struct VideoInfo {
    int32_t fileId = -1;
    VideoCount videoCount = VideoCount::SINGLE;
    std::string filePath = "";
    std::string absSrcFilePath = "";
    std::string videoPath = "";
};

class MultiStagesVideoCaptureManager {
public:
    EXPORT static MultiStagesVideoCaptureManager& GetInstance();
    bool Init();

    EXPORT void SyncWithDeferredVideoProcSession();
    EXPORT void SyncWithDeferredVideoProcSessionInternal();
    EXPORT void AddVideoInfo(const std::string &videoId, VideoInfo &videoInfo);
    EXPORT void RemoveVideoInfo(const std::string &videoId);
    EXPORT void GetVideoInfo(const std::string &videoId, VideoInfo &videoInfo);
    EXPORT std::shared_ptr<OHOS::NativeRdb::ResultSet> HandleMultiStagesOperation(
        MediaLibraryCommand &cmd, const std::vector<std::string> &columns);
    EXPORT void AddSingleVideo(const std::string &videoId, VideoInfo &videoInfo, bool isMovingPhoto);
    EXPORT void AddDoubleVideo(const std::string &videoId, VideoInfo &videoInfo, bool isMovingPhoto);
    EXPORT void AddVideoInternal(const std::string &videoId, VideoInfo &videoInfo,
        bool isTrashed, bool isMovingPhoto = false);
    EXPORT void AddVideo(const std::string &videoId, const std::string &fileId, VideoInfo &videoInfo);
    EXPORT void AddVideo(const AddProcessVideoDto &dto);
    EXPORT void RemoveVideo(const std::string &videoId, const bool restorable);
    EXPORT void RemoveVideo(const std::string &videoId, const std::string &mediaFilePath, const int32_t &photoSubType,
        const bool restorable);
    EXPORT void RestoreVideo(const std::string &videoId);
    EXPORT void ProcessVideo(const ProcessVideoDto &dto);
    EXPORT void CancelProcessRequest(const std::string &videoId);
    EXPORT static int32_t QuerySubType(const std::string &photoId);
    void InsertCinematicProgress(const std::string &videoId, const std::string &requestId, double progress);
    void InsertCinematicProgress(const std::string &videoId, double progress);
    int32_t ClearCinematicProgressMap(const std::string &videoId);
    EXPORT int32_t GetProgressCallback(GetProgressCallbackRespBody &respbody);
    EXPORT static bool Openfd4AddDoubleVideo(const std::string &effectVideoPath, VideoInfo &videoInfo,
        int32_t &lowSrcFd, int32_t &srcFd, int32_t &srcFdCopy);
    int32_t SaveCameraVideo(const SaveCameraPhotoDto &dto);

private:
    MultiStagesVideoCaptureManager();
    ~MultiStagesVideoCaptureManager();
    EXPORT static int32_t ToInt32(const std::string &str);

    std::shared_ptr<DeferredVideoProcessingAdapter> deferredProcSession_;
    EXPORT static std::map<std::string, VideoInfo> videoInfoMap_;  // <videoId, VideoInfo>
    std::mutex mutex_;

    std::map<std::string, std::pair<std::string, double>> cinematicProgressMap_;     // <videoId, <requestId, progress>>
    std::mutex progressMutex_;
};
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTI_STAGES_VIDEO_CAPTURE_MANAGER_H
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

#ifndef FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_INCLUDE_MOVING_PHOTO_FILE_UTILS_H
#define FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_INCLUDE_MOVING_PHOTO_FILE_UTILS_H

#include <string>

#include "photo_file_utils.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
EXPORT const std::string LIVE_TAG = "LIVE_";

EXPORT constexpr int32_t LIVE_TAG_LEN = 20;
EXPORT constexpr int32_t PLAY_INFO_LEN = 20;
EXPORT constexpr int32_t VERSION_TAG_LEN = 20;
EXPORT constexpr int32_t CINEMAGRAPH_INFO_SIZE_LEN = 4;
EXPORT constexpr int32_t MIN_STANDARD_SIZE = LIVE_TAG_LEN + PLAY_INFO_LEN + VERSION_TAG_LEN;

class MovingPhotoFileUtils : public PhotoFileUtils {
public:
    EXPORT static int32_t ConvertToMovingPhoto(const std::string &livePhotoPath,
        const std::string &movingPhotoImagePath, const std::string &movingPhotoVideoPath,
        const std::string &extraDataPath);
    EXPORT static int32_t GetMovingPhotoDetailedSize(const int32_t fd,
        int64_t &imageSize, int64_t &videoSize, int64_t &extraDataSize);
    EXPORT static int32_t ConvertToMovingPhoto(const int32_t fd,
        const std::string &movingPhotoImagePath, const std::string &movingPhotoVideoPath,
        const std::string &extraDataPath);
    EXPORT static int32_t ConvertToMovingPhoto(const int32_t fd,
        void *imageArrayBuffer, void *videoArrayBuffer, void *extraDataArrayBuffer);
    EXPORT static int32_t ConvertToLivePhoto(const std::string &movingPhotoImagepath, int64_t coverPosition,
        std::string &livePhotoPath, int32_t userId = -1);
    EXPORT static int32_t ConvertToSourceLivePhoto(const std::string &movingPhotoImagepath,
        std::string &sourceLivePhotoPath, int32_t userId = -1);
    EXPORT static int32_t GetCoverPosition(const std::string &videoPath, const uint32_t frameIndex,
        uint64_t &coverPosition, int32_t scene = 0);
    EXPORT static int32_t GetVersionAndFrameNum(const std::string &tag,
        uint32_t &version, uint32_t &frameIndex, bool &hasCinemagraphInfo);
    EXPORT static int32_t GetVersionAndFrameNum(int32_t fd, uint32_t &version, uint32_t &frameIndex,
        bool &hasCinemagraphInfo);
    EXPORT static std::string GetMovingPhotoVideoPath(const std::string &imagePath, int32_t userId = -1);
    EXPORT static std::string GetMovingPhotoExtraDataDir(const std::string &imagePath, int32_t userId = -1);
    EXPORT static std::string GetMovingPhotoExtraDataPath(const std::string &imagePath, int32_t userId = -1);
    EXPORT static std::string GetSourceMovingPhotoImagePath(const std::string &imagePath, int32_t userId = -1);
    EXPORT static std::string GetSourceMovingPhotoVideoPath(const std::string &imagePath, int32_t userId = -1);
    EXPORT static std::string GetSourceBackMovingPhotoImagePath(const std::string &imagePath, int32_t userId = -1);
    EXPORT static std::string GetSourceBackMovingPhotoVideoPath(const std::string &imagePath, int32_t userId = -1);
    EXPORT static std::string GetLivePhotoCacheDir(const std::string &imagePath, int32_t userId = -1);
    EXPORT static std::string GetLivePhotoCachePath(const std::string &imagePath, int32_t userId = -1);
    EXPORT static std::string GetSourceLivePhotoCachePath(const std::string &imagePath, int32_t userId = -1);
    EXPORT static bool IsMovingPhoto(int32_t subtype, int32_t effectMode, int32_t originalSubtype);
    EXPORT static bool IsGraffiti(int32_t subtype, int32_t originalSubtype);
    EXPORT static bool IsLivePhoto(const std::string &path);
    EXPORT static int32_t GetLivePhotoSize(int32_t fd, int64_t &liveSize);
    EXPORT static int32_t GetExtraDataLen(const std::string &imagePath, const std::string &videoPath,
        uint32_t frameIndex, int64_t coverPosition, off_t &fileSize, bool isCameraShotMovingPhoto = false);
    EXPORT static uint32_t GetFrameIndex(int64_t time, const int32_t fd);
    EXPORT static size_t GetMovingPhotoSize(const std::string &imagePath, int32_t userId = -1);
    EXPORT static bool IsExistsLivePhotoFiles(const std::string &imagePath);
    EXPORT static int32_t FindMovingPhotoAttachments(const std::string &cloudPath,
        std::vector<std::string> &cloudPathList);
};
} // namespace OHOS::Media

#endif // FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_INCLUDE_MOVING_PHOTO_FILE_UTILS_H

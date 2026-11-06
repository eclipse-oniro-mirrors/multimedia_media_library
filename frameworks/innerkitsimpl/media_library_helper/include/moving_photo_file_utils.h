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

#include "unique_fd.h"
#include "photo_file_utils.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
EXPORT const std::string LIVE_TAG = "LIVE_";

EXPORT constexpr int32_t LIVE_TAG_LEN = 20;
EXPORT constexpr int32_t PLAY_INFO_LEN = 20;
EXPORT constexpr int32_t VERSION_TAG_LEN = 20;
EXPORT constexpr int32_t CINEMAGRAPH_INFO_SIZE_LEN = 4;
EXPORT constexpr int32_t MIN_STANDARD_SIZE = LIVE_TAG_LEN + PLAY_INFO_LEN + VERSION_TAG_LEN;
EXPORT constexpr uint32_t LIVE_PHOTO_4D_VERSION = 8;

enum class MOVING_PHOTO_VERSION : uint32_t {
    MOVING_PHOTO_VERSION_8 = 8,        // 不允许外部写入（LIVE_PHOTO_4D_VERSION）
    MOVING_PHOTO_VERSION_9 = 9,        // 允许外部写入
};

struct MovingPhotoExtraDataParams {
    std::string imagePath;
    std::string videoPath;
    uint32_t frameIndex = 0;
    uint32_t version = 0;
    int64_t coverPosition = 0;
    bool isCameraShotMovingPhoto = false;
    // 源合一文件末尾整段 extraData 原样内容，长度只能是 40B（无版本段）、60B（无 CinemagraphInfo）
    // 或 60B+N（含 CinemagraphInfo）；非空时优先生效，不再按帧号与版本号重拼
    std::string extraData;
};

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
    EXPORT static int32_t ConvertToLivePhoto(const std::string &movingPhotoImagePath,
        const std::string &movingPhotoVideoPath, const std::string &movingPhotoExtraDataPath,
        int64_t coverPosition, std::string &livePhotoPath, int32_t userId = -1);
    EXPORT static int32_t ConvertToLivePhoto(const int32_t imageFd, const int32_t videoFd,
        const std::string &livePhotoPath, int64_t coverPosition = 0);
    EXPORT static int32_t GetCoverPosition(const std::string &videoPath, const uint32_t frameIndex,
        uint64_t &coverPosition, int32_t scene = 0);
    EXPORT static int32_t GetLivePhotoCoverPosition(const std::string &videoPath,
        const std::string &livePhotoPath, int64_t &coverPosition);
    EXPORT static int32_t GetLivePhotoCoverPosition(const std::string livePhotoPath, int64_t &coverPosition);
    EXPORT static int32_t GetLivePhotoCoverPositionAndDuration(const std::string livePhotoPath,
        int64_t &coverPosition, int32_t &duration);
    EXPORT static int32_t GetCoverPositionFromExtraData(const std::string &videoPath,
        const std::string &extraDataPath, int64_t &coverPosition);
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
    EXPORT static bool IsLivePhoto(const int32_t fd);
    EXPORT static int32_t GetLivePhotoSize(const int32_t fd, int64_t &liveSize);
    EXPORT static bool IsLivePhotoAsset(const std::string &realPath);
    EXPORT static int32_t GetExtraDataLen(const std::string &imagePath, const std::string &videoPath,
        uint32_t frameIndex, int64_t coverPosition, off_t &fileSize, bool isCameraShotMovingPhoto = false);
    /**
     * @brief 按 params 中的帧号与版本号创建或复用 extraData；已存在时直接返回其长度，不做修改。
     *        params.extraData 非空时以整段 extraData 为准，一次写出后 fileSize 为实际写入长度；
     *        否则按帧号、封面时间与视频大小重新拼装 60B。
     */
    EXPORT static int32_t CreateExtraData(const MovingPhotoExtraDataParams &params, off_t &fileSize);
    /**
     * @brief 读取合一文件尾部的整段 extraData（40B / 60B / 60B+N），不接管 fd 所有权。
     *        版本段带 _c 但 CinemagraphInfo 长度字段损坏时，整段已回退为 60B，
     *        此时剔除版本段的 _c 后缀，避免消费侧按 _c 语义多算 4B 长度字段。
     * @return 成功返回 E_OK；文件太小或不是有效容器返回 E_INVALID_LIVE_PHOTO；IO 异常返回 E_HAS_FS_ERROR。
     */
    EXPORT static int32_t GetWholeExtraData(int32_t fd, std::string &extraData);
    /**
     * @brief 校验整段 extraData：长度只能是 40B 或 60B 及以上；末尾 20B 必须是 LIVE_ 段；
     *        60B 及以上时末尾 60B 起始处的版本段必须是标准 v<版本号>_f<帧号> 格式（允许 _c 后缀）。
     *        40B 不含版本段，无法校验版本号，只校验 LIVE_ 段。
     * @return 合法返回 E_OK，其余返回 E_INVALID_VALUES。
     */
    EXPORT static int32_t CheckExtraData(const std::string &extraData);
    EXPORT static uint32_t GetFrameIndex(int64_t time, const int32_t fd);
    EXPORT static size_t GetMovingPhotoSize(const std::string &imagePath, int32_t userId = -1);
    EXPORT static bool IsExistsLivePhotoFiles(const std::string &imagePath);
    EXPORT static int32_t FindMovingPhotoAttachments(const std::string &cloudPath,
        std::vector<std::string> &cloudPathList);
    EXPORT static int32_t ModifyExtraDataVersion(const std::string& extraPath, uint32_t newVersion);
    EXPORT static int32_t GetExtraDataVersion(const std::string& extraPath, uint32_t& version);
    EXPORT static bool CheckMovingPhotoVideo(const std::string &path, bool checkDuration = true);
    EXPORT static bool CheckMovingPhotoVideo(const UniqueFd &uniqueFd);
    /**
     * @brief 校验动态照片视频是否有效，但**不限制时长上限**。
     *
     * 供合一动态照片链路使用：容器内视频允许超过 10s，但仍需扩展名合规且能解析出
     * 有效时长，以拒绍空文件与损坏文件。
     */
    EXPORT static bool CheckMovingPhotoVideoFormat(const std::string &path);
    EXPORT static int32_t GetMovingPhotoVideoDuration(const std::string &path);
    EXPORT static int32_t GetMovingPhotoVideoDuration(const UniqueFd &uniqueFd);
    EXPORT static bool CheckMovingPhotoVideoDuration(int32_t duration);
    EXPORT static int32_t GetExtraDataVersionByFilePath(const std::string &filePath, uint32_t &version);
    EXPORT static void GetLocalAssetSize(const int32_t movingPhotoEffectMode, const std::string& filePath,
        const int64_t size, int64_t& localAssetSize);
    EXPORT static bool CheckMovingPhotoDetailedSize(const int32_t fd);
    EXPORT static bool DecomposeLivePhoto(const std::string &filePath);
};
} // namespace OHOS::Media

#endif // FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_INCLUDE_MOVING_PHOTO_FILE_UTILS_H

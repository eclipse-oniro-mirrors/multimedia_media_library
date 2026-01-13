/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_CLOUD_MEDIA_SYNC_UTILS_H
#define OHOS_CLOUD_MEDIA_SYNC_UTILS_H

#include <string>
#include <vector>

#include "photo_album_po.h"
#include "photos_dto.h"
#include "photos_po.h"
#include "cloud_media_dao_const.h"
#include "cloud_media_pull_data_dto.h"

namespace OHOS::Media::CloudSync {
#define EXPORT __attribute__ ((visibility ("default")))
using namespace OHOS::Media::ORM;
class EXPORT CloudMediaSyncUtils {
public:
    CloudMediaSyncUtils();
    ~CloudMediaSyncUtils();

    static std::string GetLocalPath(const std::string &filePath);
    static std::string RestoreCloudPath(const std::string &filepath);
    static bool IsLocalDirty(int32_t dirty, bool isDelete);
    static bool FileIsLocal(const int32_t position);
    static std::string GetCloudPath(const std::string &path, const std::string &prefixCloud);
    static std::string GetThumbParentPath(const std::string &path, const std::string &prefixCloud);
    static void RemoveThmParentPath(const std::string &path, const std::string &prefixCloud);
    static void RemoveEditDataParentPath(const std::string &path);
    static void RemoveMetaDataPath(const std::string &path);
    static void InvalidVideoCache(const std::string &localPath);
    static void RemoveMovingPhoto(const CloudMediaPullDataDto &pullData);
    static void BackUpEditDataSourcePath(const std::string &localPath);
    static void RemoveEditDataSourcePath(const std::string &localPath);
    static void RemoveEditDataPath(const std::string &localPath);
    static void RemoveTransCodePath(const std::string &localPath);
    static uint32_t GenerateCloudIdWithHash(PhotoAlbumPo &record);
    static int32_t FillPhotosDto(PhotosDto &photosDto, const std::string &path, const int32_t &orientation,
        const int32_t exifRotate, const int32_t &thumbState);
    static int32_t FillPhotosDto(PhotosDto &photosDto, const CloudMediaPullDataDto &data);
    static std::string GetLpathFromSourcePath(const std::string &sourcePath);
    static std::string GetLpath(const CloudMediaPullDataDto &pullData);
    static std::string GetMovingPhotoExtraDataDir(const std::string &localPath);
    static std::string GetMovingPhotoExtraDataPath(const std::string &localPath);
    static std::string GetEditDataDir(const std::string &localPath);
    static std::string GetEditDataPath(const std::string &localPath);
    static std::string GetTransCodePath(const std::string &localPath);
    static std::string GetMovingPhotoVideoPath(const std::string &localPath);
    static std::string GetMovingPhotoTmpPath(const std::string &localPath);
    static std::string GetEditDataSourcePath(const std::string& photoPath);
    static std::string GetSourceMovingPhotoImagePath(const std::string& photoPath);
    static std::string GetSourceMovingPhotoVideoPath(const std::string& photoPath);
    static bool IsMovingPhoto(const PhotosPo &photosPo);
    static bool IsMovingPhoto(const CloudMediaPullDataDto &pullData);
    static bool IsGraffiti(const PhotosPo &photosPo);
    static bool IsLivePhoto(const PhotosPo &photosPo);
    static int32_t UpdateModifyTime(const std::string &localPath, int64_t localMtime);
    static bool IsUserAlbumPath(const std::string &lpath);
    static bool CanUpdateExifRotateOnly(int32_t mediaType, int32_t oldExifRotate, int32_t newExifRotate);
    static bool IsCloudEnhancementSupported();
    static void SyncDealWithCompositePhoto(const std::string &assetDataPath, int32_t photoId);
};
} // namespace OHOS::Media::CloudSync
#endif // OHOS_CLOUD_MEDIA_SYNC_UTILS_H

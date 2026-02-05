/*
 * Copyright (C) 2024-2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_PHOTO_FILE_OPERATIOIN_H
#define OHOS_MEDIA_PHOTO_FILE_OPERATIOIN_H

#include <string>
#include <vector>

#include "rdb_store.h"
#include "photos_po.h"
#include "cloud_media_define.h"

namespace OHOS::Media {
using namespace OHOS::Media::ORM;
class EXPORT PhotoFileOperation {
private:
    struct PhotoAssetInfo {
        std::string displayName;
        std::string filePath;
        int64_t dateModified{0};
        int32_t subtype{0};
        bool isMovingPhoto{false};
        std::string videoFilePath;
        std::string editDataFolder;
        std::string thumbnailFolder;
    };

public:
    int32_t CopyPhoto(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, const std::string &targetPath);
    int32_t MovePhoto(const PhotosPo &sourcePhotosPo, const PhotosPo &targetPhotosPo);
    int32_t CopyPhoto(const PhotosPo &sourcePhotosPo, const PhotosPo &targetPhotosPo);
    int32_t CopyThumbnail(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, const std::string &targetPath,
        int64_t &newAssetId);
    int32_t CopyThumbnail(const PhotosPo &sourcePhotosPo, const PhotosPo &targetPhotosPo, bool withAstcData = true);
    int32_t ConvertFormatPhoto(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, const std::string &targetPath,
        const std::string &extension);
    int32_t CreateTmpCompatibleDup(const std::string &srcPath, size_t &size);
    int32_t DeletePhoto(const PhotosPo &photoInfo);
    int32_t DeleteThumbnail(const PhotosPo &photoInfo);
    std::string GetAuditLog() const;
    int32_t CopyLocalThumbnail(const PhotosPo &sourcePhotosPo, const PhotosPo &targetPhotosPo);

private:
    std::string GetVideoFilePath(const PhotoAssetInfo &photoInfo);
    std::string FindVideoFilePath(const PhotoAssetInfo &photoInfo);
    std::string FindRelativePath(const std::string &filePath);
    std::string FindPrefixOfEditDataFolder(const std::string &filePath);
    std::string FindPrefixOfThumbnailFolder(const std::string &filePath);
    std::string FindPrefixDataFolder(const std::string &filePath, const std::string &dataName);
    std::string BuildEditDataFolder(const PhotoFileOperation::PhotoAssetInfo &photoInfo);
    std::string BuildThumbnailFolder(const PhotoFileOperation::PhotoAssetInfo &photoInfo);
    std::string FindEditDataFolder(const PhotoAssetInfo &photoInfo);
    std::string FindThumbnailFolder(const PhotoAssetInfo &photoInfo);
    int32_t CopyPhotoFile(const PhotoAssetInfo &sourcePhotoInfo, const PhotoAssetInfo &targetPhotoInfo);
    int32_t CopyPhotoRelatedVideoFile(const PhotoAssetInfo &sourcePhotoInfo, const PhotoAssetInfo &targetPhotoInfo);
    int32_t CopyPhotoRelatedExtraData(const PhotoAssetInfo &sourcePhotoInfo, const PhotoAssetInfo &targetPhotoInfo);
    int32_t CopyPhotoRelatedThumbnail(const PhotoAssetInfo &sourcePhotoInfo, const PhotoAssetInfo &targetPhotoInfo);
    int32_t CopyPhotoRelatedData(const PhotoAssetInfo &sourcePhotoInfo, const PhotoAssetInfo &targetPhotoInfo,
        const std::string &srcFolder, const std::string &targetFolder);
    int32_t CopyPhoto(const PhotoAssetInfo &sourcePhotoInfo, const PhotoAssetInfo &targetPhotoInfo);
    int32_t CopyFile(const std::string &srcPath, std::string &targetPath);
    std::string ToString(const PhotoAssetInfo &photoInfo);
    int32_t HandleThumbnailAstcData(const std::string &dateTaken, const std::string &oldAssetId,
        const std::string &newAssetId);
    int32_t ConvertFormatPhoto(const PhotoAssetInfo &sourcePhotoInfo, const PhotoAssetInfo &targetPhotoInfo,
        const std::string &extension);
    int32_t ConvertFormatFile(const std::string &srcFilePath, const std::string &dstFilePath,
        const int64_t dateModified, const std::string &extension);
    int32_t ConvertFormatPhotoExtraData(const std::string &srcPath, const std::string &dstPath,
        const std::string &extension);
    int32_t MovePhoto(const PhotoAssetInfo &sourcePhotoInfo, const PhotoAssetInfo &targetPhotoInfo);
    int32_t MovePhotoFile(const PhotoAssetInfo &sourcePhotoInfo, const PhotoAssetInfo &targetPhotoInfo);
    int32_t MovePhotoRelatedVideoFile(const PhotoAssetInfo &sourcePhotoInfo, const PhotoAssetInfo &targetPhotoInfo);
    int32_t MovePhotoRelatedExtraData(const PhotoAssetInfo &sourcePhotoInfo, const PhotoAssetInfo &targetPhotoInfo);
    int32_t MovePhotoRelatedData(const PhotoAssetInfo &sourcePhotoInfo, const PhotoAssetInfo &targetPhotoInfo,
        const std::string &srcFolder, const std::string &targetFolder);
    int32_t MoveFile(const std::string &srcPath, std::string &targetPath);
    int32_t DeletePhoto(const PhotoAssetInfo &sourcePhotoInfo);
    int32_t DeletePhotoFile(const PhotoAssetInfo &sourcePhotoInfo);
    int32_t DeletePhotoRelatedVideoFile(const PhotoAssetInfo &sourcePhotoInfo);
    int32_t DeletePhotoRelatedData(const PhotoAssetInfo &sourcePhotoInfo, const std::string &srcFolder);
    int32_t DeletePhotoRelatedExtraData(const PhotoAssetInfo &sourcePhotoInfo);
    void AuditLog(const std::string &path, const std::string &action, const int32_t result);

private:
    const std::string TAG_MOVE_IN = "MOVE_IN";
    const std::string TAG_MOVE_OUT = "MOVE_OUT";
    const std::string TAG_COPY_SOURCE = "COPY_SOURCE";
    const std::string TAG_COPY_TARGET = "COPY_TARGET";
    const std::string TAG_DELETE = "DELETE";
    std::vector<std::string> opStats_;
};
}  // namespace OHOS::Media
#endif  // FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_INCLUDE_PHOTO_FILE_H_
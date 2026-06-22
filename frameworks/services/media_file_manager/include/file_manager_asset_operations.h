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
#ifndef MEDIALIBRARY_FILE_MANAGER_ASSET_OPERATIONS_H
#define MEDIALIBRARY_FILE_MANAGER_ASSET_OPERATIONS_H
#include <string>

#include "asset_accurate_refresh.h"
#include "photos_po.h"

namespace OHOS::Media {
using namespace OHOS::Media::ORM;
struct MoveAssetsToFileManagerUpdateData {
    int32_t mediaId;
    std::string title;
    std::string displayName;
    std::string storagePath;
    std::string sourcePath;
};
 
class FileManagerAssetOperations {
public:
    // 图库到文管，用于隐藏、删除恢复场景
    static int32_t MoveAssetsToFileManager(
        AccurateRefresh::AssetAccurateRefresh &refresh, const std::vector<std::string> &ids);
    // 文管到图库，用于隐藏、删除场景
    static int32_t MoveAssetsFromFileManager(AccurateRefresh::AssetAccurateRefresh &refresh,
        const std::vector<std::string> &ids, bool needRefresh);
    static int32_t MoveFileManagerAsset(const PhotosPo &photoInfo);
    static int32_t MoveFileManagerAsset(const PhotosPo &photoInfo, const PhotosPo &targetPhotoInfo);
    static int32_t MoveFileManagerAsset(const std::string &srcPath,
        const std::string &destPath, bool isMovingPhoto = false, bool isLocalRename = true);
    // 重命名文管资产
    EXPORT static int32_t CheckAndRenameFileManagerAsset(AccurateRefresh::AccurateRefreshBase &refresh,
        MediaLibraryCommand &cmd, const std::shared_ptr<FileAsset> &fileAsset);
};
} // namespace OHOS::Media
#endif // MEDIALIBRARY_FILE_MANAGER_ASSET_OPERATIONS_H
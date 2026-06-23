/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_INCLUDE_MEDIA_FILE_ACCESS_UTILS_H_
#define FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_INCLUDE_MEDIA_FILE_ACCESS_UTILS_H_

#include <string>

#include "asset_operation_info.h"
#include "file_asset.h"
#include "medialibrary_errno.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

struct MoveResult {
    int32_t errCode {E_ERR};
    bool isExecuteRename {false};
    std::string newPath;
    std::string newTitle;
    std::string newDisplayName;
};

enum RenameMode : int32_t {
    RENAME = 0,
    NOT_RENAME = 1,
};

class MediaFileAccessUtils {
public:
    EXPORT static int32_t OpenAssetFile(const std::string &path, const std::string &mode);

    EXPORT static std::string GetAssetRealPath(const AssetOperationInfo &obj);
    EXPORT static std::string GetAssetRealPath(const std::string &path);
    EXPORT static std::string GetAssetRealPathById(const std::string &fileId);

    /**
     * @brief Move asset, support cross policy move and same name rename.
     *
     * @param srcObj asset operation info before move, must contain valid fileId or data
     * @param destPath the expected file path after move
     * @param destSourceType source type of the asset after move
     * @param deleteSrc whether to delete the src asset after move when cross policy move
     * @return Return move result, including error code and new path for same name rename
     */
    EXPORT static MoveResult MoveAsset(const AssetOperationInfo &srcObj, const std::string &destPath,
        FileSourceType destSourceType, bool deleteSrc = false, RenameMode needRename = RenameMode::RENAME);
    EXPORT static int32_t MoveFileInEditScene(const std::string &oldPath, const std::string &newPath,
        RenameMode needRename = RenameMode::RENAME);

    EXPORT static int32_t CopyFile(const std::string &srcPath, std::string &destPath,
        std::function<void(uint64_t)> progressCallback = nullptr, const std::string &requestId = "");

    EXPORT static bool DeleteAsset(const AssetOperationInfo &obj);

    EXPORT static int32_t HandleSameNameRename(const std::string &sameNamePath,
        std::string &renamePath, std::string &renameTitle, std::string &renameDisplayName);
    EXPORT static int32_t HandleSameNameRename(const std::string &sameNamePath,
        std::string &renamePath, std::string &renameTitle, std::string &renameDisplayName,
        const std::function<bool(const std::string &)> &isPathOccupiedChecker);
    EXPORT static int32_t HandleBurstSameNameRename(const std::string &sameNamePath,
        std::string &renamePath, std::string &renameTitle, std::string &renameDisplayName,
        const std::function<bool(const std::string &)> &isPathOccupiedChecker);

    EXPORT static bool IsDirectoryEmpty(const std::string& dirPath);
    EXPORT static void UpdateModifyTime(const std::string &path, int64_t localMtime);

    EXPORT static std::shared_ptr<FileAsset> GetFileAssetFromDb(const std::string &column, const std::string &value);

    EXPORT static MoveResult ProcessLivePhotoToMovingPhoto(const std::string &srcPath,
        const std::string &destPath, bool deleteSrc);
    EXPORT static MoveResult ProcessMovingPhotoToLivePhoto(const std::string &srcPath,
        const std::string &destPath, FileSourceType destSourceType, bool deleteSrc);

private:
    struct AssetPathConvertInfo {
        std::string assetPath;
        std::string storagePath;
        FileSourceType sourceType {FileSourceType::MEDIA};
        PhotoSubType subType {PhotoSubType::DEFAULT};
        BurstCoverLevelType burstCoverLevel {BurstCoverLevelType::DEFAULT};
        int32_t position {static_cast<int32_t>(PhotoPositionType::LOCAL)};
    };
#ifdef MEDIALIBRARY_LAKE_SUPPORT
    static bool IsZeroBucketPath(const std::string &path);
#endif
    static bool NeedConvertPath(const std::string& path);

    static bool NeedCheckSameNameRename(FileSourceType destSourceType);

    static int32_t MoveFileCrossPolicy(const std::string &srcPath, const std::string &destPath,
        bool deleteSrc = false);

    static MoveResult MoveNormalAsset(const AssetOperationInfo &srcObj, const std::string &destPath,
        FileSourceType destSourceType, bool deleteSrc = false, RenameMode needRename = RenameMode::RENAME);
    static MoveResult MoveMovingPhotoAsset(const AssetOperationInfo &srcObj, const std::string &destPath,
        FileSourceType destSourceType, bool deleteSrc = false, RenameMode needRename = RenameMode::RENAME);
    static MoveResult MoveBurstAsset(const AssetOperationInfo &srcObj, const std::string &destPath,
        FileSourceType destSourceType, bool deleteSrc = false, RenameMode needRename = RenameMode::RENAME);

    static std::string GetAssetRealPath(const AssetPathConvertInfo &info);
    static bool CheckBurstMemberDataExist(PhotoSubType subType, BurstCoverLevelType burstCoverLevel,
        const std::string &assetPath);
    static int32_t UpateMetaDataForRename(const AssetOperationInfo &srcObj, const MoveResult &result);
    static int32_t HandleSameNameRenameImpl(const std::string &sameNamePath,
        std::string &renamePath, std::string &renameTitle, std::string &renameDisplayName,
        const std::function<bool(const std::string &)> &isPathOccupiedChecker);
};
} // namespace OHOS::Media
#endif // FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_INCLUDE_MEDIA_FILE_ACCESS_UTILS_H_

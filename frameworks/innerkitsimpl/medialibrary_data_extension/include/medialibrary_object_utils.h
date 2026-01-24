/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIALIBRARY_OBJECT_UTILS_H
#define OHOS_MEDIALIBRARY_OBJECT_UTILS_H

#include <string>
#include <vector>

#include "datashare_abs_result_set.h"
#include "datashare_values_bucket.h"
#include "dir_asset.h"
#include "file_asset.h"
#include "medialibrary_command.h"
#include "medialibrary_db_const.h"
#include "medialibrary_unistore_manager.h"
#include "native_album_asset.h"
#include "imedia_scanner_callback.h"
#include "picture.h"
#include "rdb_utils.h"
#include "uri.h"
#include "value_object.h"
#include "values_bucket.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
EXPORT const std::string MEDIA_NO_FILE = ".nofile";

class MediaLibraryObjectUtils {
public:
    static int32_t BuildFileAsset(MediaLibraryCommand &cmd, FileAsset &fileAsset, NativeAlbumAsset &dirAsset,
        std::string &path, int32_t &mediaType);
    EXPORT static int32_t CreateFileObj(MediaLibraryCommand &cmd);
    EXPORT static int32_t CreateDirWithPath(const std::string &path);
    EXPORT static int32_t CreateDirObj(MediaLibraryCommand &cmd, int64_t &rowId);
    EXPORT static int32_t DeleteFileObj(const std::shared_ptr<FileAsset> &fileAsset);
    static int32_t DeleteDirObj(const std::shared_ptr<FileAsset> &dirAsset);
    EXPORT static int32_t DeleteMisc(const int32_t fileId, const std::string &filePath, const int32_t parentId);
    EXPORT static int32_t RenameFileObj(MediaLibraryCommand &cmd, const std::string &srcFilePath,
        const std::string &dstFilePath);
    EXPORT static int32_t RenameDirObj(MediaLibraryCommand &cmd, const std::string &srcDirPath,
        const std::string &dstDirPath);
    EXPORT static int32_t OpenFile(MediaLibraryCommand &cmd, const std::string &mode);
    EXPORT static int32_t CloseFile(MediaLibraryCommand &cmd);
    EXPORT static void ScanFileAsync(const std::string &path, const std::string &id, MediaLibraryApi api,
        bool isCameraShotMovingPhoto = false, std::shared_ptr<Media::Picture> resultPicture = nullptr,
        std::shared_ptr<IMediaScannerCallback> callback = nullptr);
    static void ScanFileSyncWithoutAlbumUpdate(const std::string &path, const std::string &id, MediaLibraryApi api,
        std::shared_ptr<Media::Picture> picture = nullptr);
    EXPORT static void ScanMovingPhotoVideoAsync(
        const std::string &path, bool isCameraShotMovingPhoto, bool isScanCreateThumb = false);
    EXPORT static int32_t GetIdByPathFromDb(const std::string &path);
    static std::string GetPathByIdFromDb(const std::string &id, const bool isDelete = false);
    static std::unique_ptr<FileAsset> GetFileAssetByPredicates(const NativeRdb::AbsRdbPredicates &predicates,
        const std::vector<std::string> &columns);
    static std::shared_ptr<FileAsset> GetFileAssetFromId(const std::string &id, const std::string &networkId = "");
    EXPORT static std::shared_ptr<FileAsset> GetFileAssetFromUri(const std::string &uriStr);

    EXPORT static int32_t InsertInDb(MediaLibraryCommand &cmd);
    EXPORT static int32_t ModifyInfoByIdInDb(MediaLibraryCommand &cmd, const std::string &fileId = "");
    EXPORT static int32_t DeleteInfoByIdInDb(MediaLibraryCommand &cmd, const std::string &fileId = "");
    EXPORT static std::shared_ptr<NativeRdb::ResultSet> QueryWithCondition(MediaLibraryCommand &cmd,
        const std::vector<std::string> &columns, const std::string &conditionColumn = "");

    EXPORT static bool IsColumnValueExist(const std::string &value, const std::string &column);
    EXPORT static bool IsAssetExistInDb(const int32_t id, const bool isIncludeTrash = false);
    EXPORT static bool IsFileExistInDb(const std::string &path);
    static bool CheckUriPending(const std::string &uri);
    EXPORT static int32_t CopyAsset(const std::shared_ptr<FileAsset> &srcFileAsset, const std::string &relativePath);
    EXPORT static int32_t CopyDir(const std::shared_ptr<FileAsset> &srcDirAsset, const std::string &relativePath);
    EXPORT static NativeAlbumAsset GetDirAsset(const std::string &relativePath);
    EXPORT static bool IsSmartAlbumExistInDb(const int32_t id);
    EXPORT static bool IsParentSmartAlbum(const int32_t id, const bool includeEmptyAlbum = false);
    EXPORT static int32_t CheckDirExtension(const std::string &destFilePath);
    EXPORT static int32_t CheckDirExtension(const std::string &relativePath, const std::string &displayName);
    EXPORT static int32_t UpdateDateModified(const std::string &dirPath);
    EXPORT static void GetDefaultRelativePath(const int32_t mediaType, string &relativePath);
    static void InvalidateThumbnail(const string &id, const string &tableName = CONST_MEDIALIBRARY_TABLE,
        const string &path = "");
    static void TryUpdateAnalysisProp(const std::string str);

private:
    static int32_t DeleteEmptyDirsRecursively(int32_t dirId);
    static void ScanFile(const std::string &srcPath, MediaLibraryApi api = MediaLibraryApi::API_OLD);
    EXPORT static int32_t ModifyInfoByPathInDb(MediaLibraryCommand &cmd, const std::string &path);
    EXPORT static int32_t DeleteInfoByPathInDb(MediaLibraryCommand &cmd, const std::string &path);
    EXPORT static std::string GetStringColumnByIdFromDb(const std::string &id,
        const std::string &column, const bool isDelete = false);
    EXPORT static int32_t InsertFileInDb(MediaLibraryCommand &cmd, const FileAsset &fileAsset,
        const NativeAlbumAsset &dirAsset);
    EXPORT static int32_t DeleteInvalidRowInDb(const std::string &path);
    EXPORT static NativeAlbumAsset GetLastDirExistInDb(const std::string &dirPath);
    EXPORT static int32_t DeleteRows(const std::vector<int64_t> &rowIds);
    EXPORT static int32_t InsertDirToDbRecursively(const std::string &dirPath, int64_t &rowId);
    EXPORT static int32_t ProcessNoMediaFile(const std::string &dstFileName, const std::string &dstAlbumPath);
    EXPORT static int32_t ProcessHiddenFile(const std::string &dstFileName, const std::string &srcPath);
    EXPORT static int32_t ProcessHiddenDir(const std::string &dstDirName, const std::string &srcDirPath);
    EXPORT static int32_t UpdateFileInfoInDb(MediaLibraryCommand &cmd, const std::string &dstPath,
        const int &bucketId, const std::string &bucketName);
    EXPORT static int32_t CopyAssetByFd(int32_t srcFd, int32_t srcId, int32_t destFd, int32_t destId);
    EXPORT static void CloseFileById(int32_t fileId);
    EXPORT static int32_t GetFileResult(std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        int count, const string &relativePath, const string &displayName);
    static std::shared_ptr<NativeRdb::ResultSet> QuerySmartAlbum(MediaLibraryCommand &cmd);
    static int32_t DeleteInfoRecursively(const shared_ptr<FileAsset> &fileAsset);
};
} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIALIBRARY_OBJECT_UTILS_H

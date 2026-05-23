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
#define MLOG_TAG "MediaFileAccessUtils"

#include <dirent.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "media_file_access_utils.h"

#include "directory_ex.h"
#include "dfx_utils.h"
#ifdef MEDIALIBRARY_LAKE_SUPPORT
#include "file_scan_utils.h"
#endif
#include "medialibrary_asset_operations.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_path_utils.h"
#include "media_string_utils.h"
#include "photo_file_utils.h"
#include "moving_photo_file_utils.h"
#include "multistages_video_capture_manager.h"
#include "display_name_info.h"

namespace OHOS::Media {
constexpr int32_t CROSS_POLICY_ERR = 18;
constexpr uint32_t RENAME_MAX_RETRY_COUNT = 10000;
const std::string FILE_SEPARATOR = "/";
const std::string PHOTO_DIR_VALUES = "Photo/";
const std::string MEDIA_PHOTO_DIR = ROOT_MEDIA_DIR + PHOTO_DIR_VALUES;
const mode_t CHOWN_RO_USR_GRP = 0644;

const std::string MEDIALIBRARY_ZERO_BUCKET_PATH = MEDIA_PHOTO_DIR + "0";
const std::vector<std::string> FILE_INFO_COLUMNS = {
    MediaColumn::MEDIA_FILE_PATH,
    PhotoColumn::PHOTO_STORAGE_PATH,
    PhotoColumn::PHOTO_FILE_SOURCE_TYPE,
    MediaColumn::MEDIA_ID,
    PhotoColumn::PHOTO_SUBTYPE,
    PhotoColumn::PHOTO_ORIGINAL_SUBTYPE,
    PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
    PhotoColumn::PHOTO_OWNER_ALBUM_ID,
    PhotoColumn::PHOTO_BURST_COVER_LEVEL,
    PhotoColumn::PHOTO_BURST_KEY,
    PhotoColumn::PHOTO_POSITION,
    MediaColumn::MEDIA_NAME,
};
#ifdef MEDIALIBRARY_LAKE_SUPPORT
bool MediaFileAccessUtils::IsZeroBucketPath(const std::string &path)
{
    return path.find(MEDIALIBRARY_ZERO_BUCKET_PATH) != std::string::npos;
}
#endif

bool MediaFileAccessUtils::NeedConvertPath(const std::string& path)
{
    return path.length() >= ROOT_MEDIA_DIR.length() && MediaStringUtils::StartsWith(path, MEDIA_PHOTO_DIR);
}

std::string MediaFileAccessUtils::GetAssetRealPath(const std::string &path)
{
    MEDIA_DEBUG_LOG("GetAssetRealPath from path: %{public}s", MediaFileUtils::DesensitizePath(path).c_str());
    CHECK_AND_RETURN_RET_INFO_LOG(NeedConvertPath(path), path, "no need to convert, path: %{public}s",
        MediaFileUtils::DesensitizePath(path).c_str());
#ifdef MEDIALIBRARY_LAKE_SUPPORT
    CHECK_AND_RETURN_RET(!IsZeroBucketPath(path), FileScanUtils::GetAssetRealPath(path));
#endif
    auto fileAsset = GetFileAssetFromDb(MediaColumn::MEDIA_FILE_PATH, path);
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, path, "get file asset from db failed");
    CHECK_AND_RETURN_RET(fileAsset->GetFileSourceType() == static_cast<int32_t>(FILE_MANAGER), path);
    CHECK_AND_RETURN_RET(!CheckBurstMemberDataExist(static_cast<PhotoSubType>(fileAsset->GetPhotoSubType()),
        static_cast<BurstCoverLevelType>(fileAsset->GetBurstCoverLevel()), path), path);
    std::string realPath = fileAsset->GetStoragePath();
    MEDIA_DEBUG_LOG("file belongs to file manager, get real path %{public}s",
        MediaFileUtils::DesensitizePath(realPath).c_str());
    return realPath;
}

std::string MediaFileAccessUtils::GetAssetRealPathById(const std::string &fileId)
{
    MEDIA_DEBUG_LOG("GetAssetRealPathById id: %{public}s", fileId.c_str());
    CHECK_AND_RETURN_RET_LOG(!fileId.empty(), "", "fileId is empty.");
    auto fileAsset = GetFileAssetFromDb(MediaColumn::MEDIA_ID, fileId);
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, "", "get file asset from db failed");
    AssetPathConvertInfo info;
    info.assetPath = fileAsset->GetFilePath();
    info.storagePath = fileAsset->GetStoragePath();
    info.sourceType = static_cast<FileSourceType>(fileAsset->GetFileSourceType());
    info.subType = static_cast<PhotoSubType>(fileAsset->GetPhotoSubType());
    info.burstCoverLevel = static_cast<BurstCoverLevelType>(fileAsset->GetBurstCoverLevel());
    return GetAssetRealPath(info);
}

std::string MediaFileAccessUtils::GetAssetRealPath(const AssetPathConvertInfo &info)
{
#ifdef MEDIALIBRARY_LAKE_SUPPORT
    if (IsZeroBucketPath(info.assetPath)) {
        CHECK_AND_RETURN_RET(info.sourceType != FileSourceType::MEDIA, info.assetPath);
        MEDIA_DEBUG_LOG("lake path %{public}s", MediaFileUtils::DesensitizePath(info.storagePath).c_str());
        return info.storagePath;
    }
#endif
    CHECK_AND_RETURN_RET(info.sourceType == FileSourceType::FILE_MANAGER, info.assetPath);
    CHECK_AND_RETURN_RET(!CheckBurstMemberDataExist(info.subType, info.burstCoverLevel, info.assetPath),
        info.assetPath);
    MEDIA_DEBUG_LOG("file manager path %{public}s", MediaFileUtils::DesensitizePath(info.storagePath).c_str());
    return info.storagePath;
}

bool MediaFileAccessUtils::CheckBurstMemberDataExist(PhotoSubType subType, BurstCoverLevelType burstCoverLevel,
    const std::string &assetPath)
{
    if (subType == PhotoSubType::BURST && burstCoverLevel == BurstCoverLevelType::MEMBER &&
        MediaFileUtils::IsFileExists(assetPath)) {
        MEDIA_DEBUG_LOG("burst member keeps asset path %{public}s", MediaFileUtils::DesensitizePath(assetPath).c_str());
        return true;
    }
    return false;
}

std::shared_ptr<FileAsset> MediaFileAccessUtils::GetFileAssetFromDb(const std::string &column, const std::string &value)
{
    MEDIA_DEBUG_LOG("GetFileAssetFromDb enter");
    return MediaLibraryAssetOperations::GetFileAssetFromDb(column, value, OperationObject::FILESYSTEM_PHOTO,
        FILE_INFO_COLUMNS);
}

int32_t MediaFileAccessUtils::OpenAssetFile(const std::string &path, const std::string &mode)
{
    MEDIA_DEBUG_LOG("OpenAssetFile enter");
    std::string realPath = GetAssetRealPath(path);
    CHECK_AND_RETURN_RET(!realPath.empty(), E_ERR);
    return MediaFileUtils::OpenAsset(realPath, mode);
}

int32_t MediaFileAccessUtils::MoveFileInEditScene(const std::string &oldPath, const std::string &newPath,
    RenameMode needRename)
{
    CHECK_AND_RETURN_RET_LOG(!newPath.empty(), E_ERR, "empty destPath");
    std::string srcPath = GetAssetRealPath(oldPath);
    std::string destPath = GetAssetRealPath(newPath);
#ifdef MEDIALIBRARY_LAKE_SUPPORT
    bool hasLakePath = IsZeroBucketPath(oldPath) || IsZeroBucketPath(newPath);
    CHECK_AND_RETURN_RET(!hasLakePath, FileScanUtils::MoveFileInEditScene(srcPath, destPath));
#endif
    MEDIA_INFO_LOG("move file src: %{public}s, dest: %{public}s", MediaFileUtils::DesensitizePath(srcPath).c_str(),
        MediaFileUtils::DesensitizePath(destPath).c_str());

    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(srcPath), E_ERR, "source file does not exist");

    bool destExists = MediaFileUtils::IsFileExists(destPath);
    bool isSuccess = false;

    CHECK_AND_RETURN_RET(rename(srcPath.c_str(), destPath.c_str()) != 0, E_OK);
    MEDIA_WARN_LOG("rename failed, errno: %{public}d", errno);
    CHECK_AND_RETURN_RET(errno == CROSS_POLICY_ERR, isSuccess);
    if (PhotoFileUtils::CheckFileManagerRealPath(destPath)) {
        isSuccess = MediaFileUtils::CopyFileUtil(srcPath, destPath);
        if (isSuccess) {
            CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteFile(srcPath), "delete failed, errno: %{public}d", errno);
        }
    } else {
        CHECK_AND_RETURN_RET(!destExists, E_ERR);
        isSuccess = MediaFileUtils::CopyFileUtil(srcPath, destPath);
    }

    return isSuccess ? E_OK : E_ERR;
}

bool MediaFileAccessUtils::IsDirectoryEmpty(const std::string& dirPath)
{
    DIR* dir = opendir(dirPath.c_str());
    if (!dir) return true;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name != "." && name != "..") {
            closedir(dir);
            return false;
        }
    }
    closedir(dir);
    return true;
}

void MediaFileAccessUtils::UpdateModifyTime(const std::string &path, int64_t localMtime)
{
    CHECK_AND_RETURN_LOG(!path.empty(), "empty path");
    CHECK_AND_RETURN_LOG(PhotoFileUtils::CheckFileManagerRealPath(path), "not file manager path");

    struct timeval times[2];
    // set atime
    times[0].tv_sec = static_cast<time_t>(localMtime / MSEC_TO_SEC);
    times[0].tv_usec = static_cast<suseconds_t>((localMtime % MSEC_TO_SEC) * MSEC_TO_SEC);
    // set mtime
    times[1] = times[0];

    std::string pathToModifyTime = path;
    if (utimes(pathToModifyTime.c_str(), times) < 0) {
        MEDIA_ERR_LOG("utimes failed %{public}d, path: %{public}s", errno,
            DfxUtils::GetSafePath(pathToModifyTime).c_str());
    }
}

std::string MediaFileAccessUtils::GetAssetRealPath(const AssetOperationInfo &obj)
{
    MEDIA_INFO_LOG("GetAssetRealPath from obj, fileId: %{public}s, path: %{public}s", obj.GetFileId().c_str(),
        MediaFileUtils::DesensitizePath(obj.GetAssetPath()).c_str());
    CHECK_AND_RETURN_RET_LOG(obj.IsValid(), "", "obj is not valid");
    if (!obj.IsInfoAvailable()) {
        CHECK_AND_RETURN_RET_LOG(!obj.GetAssetPath().empty(), "", "asset path is empty");
        return obj.GetAssetPath();
    }
    AssetPathConvertInfo info;
    info.assetPath = obj.GetAssetPath();
    info.storagePath = obj.GetStoragePath();
    info.sourceType = obj.GetFileSourceType();
    info.subType = obj.GetSubType();
    info.burstCoverLevel = obj.GetBurstCoverLevel();
    return GetAssetRealPath(info);
}

MoveResult MediaFileAccessUtils::MoveAsset(const AssetOperationInfo &srcObj, const std::string &destPath,
    FileSourceType destSourceType, bool deleteSrc, RenameMode needRename)
{
    MoveResult result;
    CHECK_AND_RETURN_RET_LOG(!destPath.empty(), result, "empty destPath");
    CHECK_AND_RETURN_RET_LOG(srcObj.IsValid(), result, "srcObj is not valid");
    bool isAvailable = srcObj.IsInfoAvailable();
    PhotoSubType subType = srcObj.GetSubType();
    if (!isAvailable && subType != PhotoSubType::MOVING_PHOTO) {
        MEDIA_DEBUG_LOG("asset info is not available, srcObj path: %{public}s, fileId: %{public}s",
            MediaFileUtils::DesensitizePath(srcObj.GetAssetPath()).c_str(), srcObj.GetFileId().c_str());
        CHECK_AND_RETURN_RET_LOG(!srcObj.GetAssetPath().empty(), result, "asset path is empty");
        return MoveNormalAsset(srcObj, destPath, destSourceType, deleteSrc, needRename);
    }
    if (subType == PhotoSubType::MOVING_PHOTO) {
        return MoveMovingPhotoAsset(srcObj, destPath, destSourceType, deleteSrc, needRename);
    } else if (subType == PhotoSubType::BURST) {
        return MoveBurstAsset(srcObj, destPath, destSourceType, deleteSrc, needRename);
    } else {
        return MoveNormalAsset(srcObj, destPath, destSourceType, deleteSrc, needRename);
    }
}

bool MediaFileAccessUtils::IsAlbumHasSameNameAsset(const AssetOperationInfo &srcObj, const std::string &displayName)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "get rdbstore error");
    std::string sql = "SELECT COUNT(1) AS count FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " +
                      MediaColumn::MEDIA_NAME + " = ? AND " + MediaColumn::MEDIA_ID + "<>? AND " +
                      PhotoColumn::PHOTO_OWNER_ALBUM_ID + "= ?;";
    std::vector<std::string> args = {displayName, srcObj.GetFileId(), srcObj.GetOwnerAlbumId()};
    auto resultSet = rdbStore->QuerySql(sql, args);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "query database error");
    if (resultSet->GoToFirstRow() != E_OK) {
        MEDIA_ERR_LOG("query resultset error");
        resultSet->Close();
        return E_ERR;
    }
    int32_t count = MediaLibraryRdbStore::GetInt(resultSet, "count");
    resultSet->Close();
    MEDIA_INFO_LOG("IsAlbumHasSameNameAsset fileId: %{public}s, count: %{public}d", srcObj.GetFileId().c_str(), count);
    return count > 0;
}

int32_t MediaFileAccessUtils::HandleSameNameRename(const AssetOperationInfo &srcObj, const std::string &sameNamePath,
    std::string &renamePath, std::string &renameTitle, std::string &renameDisplayName)
{
    MEDIA_DEBUG_LOG("HandleSameNameRename sameNamePath: %{public}s",
        MediaFileUtils::DesensitizePath(sameNamePath).c_str());
    CHECK_AND_RETURN_RET_LOG(!sameNamePath.empty(), E_ERR, "empty sameNamePath");
    std::string tempPath = sameNamePath;
    std::string tempDisplayName = MediaFileUtils::GetFileName(sameNamePath);
    size_t dotPos = tempDisplayName.rfind('.');
    std::string fileExtension = (dotPos != std::string::npos) ? tempDisplayName.substr(dotPos) : "";
    std::string tempTitle = dotPos != std::string::npos ? tempDisplayName.substr(0, dotPos) : tempDisplayName;
    CHECK_AND_RETURN_RET_LOG(!tempDisplayName.empty(), E_ERR, "empty display name");
    CHECK_AND_RETURN_RET_LOG(!tempTitle.empty(), E_ERR, "empty title");
    std::string parentPath = MediaFileUtils::GetParentPath(sameNamePath);
    CHECK_AND_RETURN_RET_LOG(!parentPath.empty(), E_ERR, "empty parent path");
    std::string baseName = tempTitle;

    uint32_t retryCount = 0;
    while (retryCount < RENAME_MAX_RETRY_COUNT &&
            (MediaFileUtils::IsFileExists(tempPath) || IsAlbumHasSameNameAsset(srcObj, tempDisplayName))) {
        ++retryCount;
        tempTitle = baseName + "(" + std::to_string(retryCount) + ")";
        tempDisplayName = tempTitle + fileExtension;
        tempPath = parentPath + FILE_SEPARATOR + tempDisplayName;
        MEDIA_DEBUG_LOG("path conflict, try new path: %{public}s", MediaFileUtils::DesensitizePath(tempPath).c_str());
    }
    if (retryCount == RENAME_MAX_RETRY_COUNT) {
        CHECK_AND_RETURN_RET_LOG(!MediaFileUtils::IsFileExists(tempPath) &&
            !IsAlbumHasSameNameAsset(srcObj, tempDisplayName), E_ERR, "path still conflict after retry");
    }
    renamePath = tempPath;
    renameTitle = tempTitle;
    renameDisplayName = tempDisplayName;
    return E_OK;
}

int32_t MediaFileAccessUtils::HandleBurstSameNameRename(const AssetOperationInfo &srcObj,
    const std::string &sameNamePath, std::string &renamePath, std::string &renameTitle, std::string &renameDisplayName)
{
    CHECK_AND_RETURN_RET_LOG(!sameNamePath.empty(), E_ERR, "empty sameNamePath");
    std::string displayName = MediaFileUtils::GetFileName(sameNamePath);
    std::string parentPath = MediaFileUtils::GetParentPath(sameNamePath);
    CHECK_AND_RETURN_RET_LOG(!parentPath.empty(), E_ERR, "empty parent path");

    std::string tempPath = sameNamePath;
    size_t dotPos = displayName.rfind('.');
    std::string tempTitle = dotPos != std::string::npos ? displayName.substr(0, dotPos) : displayName;
    uint32_t retryCount = 0;
    while (retryCount < RENAME_MAX_RETRY_COUNT &&
            (MediaFileUtils::IsFileExists(tempPath) || IsAlbumHasSameNameAsset(srcObj, displayName))) {
        PhotoAssetInfo photoAssetInfo;
        photoAssetInfo.displayName = displayName;
        photoAssetInfo.subtype = static_cast<int32_t>(PhotoSubType::BURST);
        DisplayNameInfo displayNameInfo(photoAssetInfo);
        std::string newDisplayName = displayNameInfo.Next();
        CHECK_AND_RETURN_RET_LOG(!newDisplayName.empty(), E_ERR, "Failed to generate new display name");
        CHECK_AND_RETURN_RET_LOG(newDisplayName != displayName, E_ERR, "DisplayNameInfo::Next() returns same value");
        displayName = newDisplayName;
        tempPath = parentPath + FILE_SEPARATOR + displayName;
        dotPos = displayName.rfind('.');
        tempTitle = dotPos != std::string::npos ? displayName.substr(0, dotPos) : displayName;
        ++retryCount;
    }
    if (retryCount == RENAME_MAX_RETRY_COUNT) {
        CHECK_AND_RETURN_RET_LOG(!MediaFileUtils::IsFileExists(tempPath) &&
            !IsAlbumHasSameNameAsset(srcObj, displayName), E_ERR, "path still conflict after retry");
    }

    renamePath = tempPath;
    renameTitle = tempTitle;
    renameDisplayName = displayName;
    MEDIA_INFO_LOG("displayname: %{public}s, renamePath:%{public}s",
        displayName.c_str(), MediaFileUtils::DesensitizePath(renamePath).c_str());
    return E_OK;
}

bool MediaFileAccessUtils::NeedCheckSameNameRename(FileSourceType destSourceType)
{
    return destSourceType == FileSourceType::MEDIA_HO_LAKE || destSourceType == FileSourceType::FILE_MANAGER;
}

int32_t MediaFileAccessUtils::MoveFileCrossPolicy(const std::string &srcPath, const std::string &destPath,
    bool deleteSrc)
{
    CHECK_AND_RETURN_RET_LOG(!destPath.empty(), E_ERR, "Empty destPath");
    MEDIA_INFO_LOG("MoveFile src: %{public}s, dest: %{public}s", MediaFileUtils::DesensitizePath(srcPath).c_str(),
        MediaFileUtils::DesensitizePath(destPath).c_str());
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(srcPath), E_ERR, "Source file does not exist");
    int64_t originalDataModified = 0;
    bool isDateModifiedValid = MediaFileUtils::GetDateModified(srcPath, originalDataModified);
    std::string parentDir = MediaFileUtils::GetParentPath(destPath);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsDirExists(parentDir) || MediaFileUtils::CreateDirectory(parentDir),
        E_ERR, "Failed to create parent directory");
    CHECK_AND_RETURN_RET(rename(srcPath.c_str(), destPath.c_str()) != 0, E_OK);
    MEDIA_WARN_LOG("rename failed when cross policy, try copy and delete, errno: %{public}d", errno);

    bool success = false;
    if (errno == CROSS_POLICY_ERR) {
        success = deleteSrc ? MediaFileUtils::CopyFileAndDelSrc(srcPath, destPath) :
            MediaFileUtils::CopyFileSafe(srcPath, destPath);
    }
    CHECK_AND_EXECUTE(!(success && isDateModifiedValid), UpdateModifyTime(destPath, originalDataModified));
    if (!success && MediaFileUtils::IsFileExists(destPath)) {
        MEDIA_WARN_LOG("Failed to move file and dest file exists, try to delete dest file, errno: %{public}d", errno);
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::DeleteFile(destPath), E_ERR,
            "Failed to delete dest file, errno: %{public}d", errno);
    }
    return success ? E_OK : E_ERR;
}

int32_t MediaFileAccessUtils::UpateMetaDataForRename(const AssetOperationInfo &srcObj, const MoveResult &result)
{
    std::string fileId = srcObj.GetFileId();
    CHECK_AND_RETURN_RET_LOG(fileId != "", E_ERR, "srcObj's fileId in not initialized");
    auto refresh = srcObj.GetAssetRefresh();
    CHECK_AND_RETURN_RET_LOG(refresh != nullptr, E_ERR, "get rdbstore error");
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutString(MediaColumn::MEDIA_TITLE, result.newTitle);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, result.newDisplayName);
    valuesBucket.PutString(PhotoColumn::PHOTO_STORAGE_PATH, result.newPath);
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    int32_t changedRows = -1;
    refresh->Update(changedRows, valuesBucket, predicates);
    CHECK_AND_RETURN_RET_LOG(changedRows > 0, E_ERR, "fail to update");
    return E_OK;
}

MoveResult MediaFileAccessUtils::MoveNormalAsset(const AssetOperationInfo &srcObj, const std::string &destPath,
    FileSourceType destSourceType, bool deleteSrc, RenameMode needRename)
{
    MEDIA_INFO_LOG("MoveNormalAsset destPath: %{public}s, deleteSrc: %{public}d, destSourceType: %{public}d",
        MediaFileUtils::DesensitizePath(destPath).c_str(), deleteSrc, static_cast<int32_t>(destSourceType));
    MoveResult result;
    result.newPath = destPath;
    result.newDisplayName = MediaPathUtils::GetFileName(destPath);
    result.newTitle = MediaFileUtils::GetTitleFromDisplayName(result.newDisplayName);
    if (NeedCheckSameNameRename(destSourceType)) {
        result.errCode = MediaFileAccessUtils::HandleSameNameRename(srcObj, destPath, result.newPath, result.newTitle,
            result.newDisplayName);
        CHECK_AND_RETURN_RET_LOG(result.errCode == E_OK, result, "handle same name failed");
        result.isExecuteRename = destPath != result.newPath;
    }
    if (result.isExecuteRename && needRename == RenameMode::NOT_RENAME) {
        result.errCode = E_RENAME;
    } else if (result.isExecuteRename && needRename == RenameMode::RENAME) {
        int32_t ret = UpateMetaDataForRename(srcObj, result);
        result.errCode = (ret == E_OK) ? E_OK : E_RDB;
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, result, "fail to update metadata for rename");
    }
    std::string srcPath = GetAssetRealPath(srcObj);
    result.errCode = MoveFileCrossPolicy(srcPath, result.newPath, deleteSrc);
    return result;
}

MoveResult MediaFileAccessUtils::ProcessLivePhotoToMovingPhoto(const std::string &srcPath,
    const std::string &destPath, bool deleteSrc)
{
    MoveResult result;
    std::string videoPath = MediaFileUtils::GetMovingPhotoVideoPath(destPath);
    std::string extraDataPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(destPath);
    std::string extraPathDir = MovingPhotoFileUtils::GetMovingPhotoExtraDataDir(destPath);
    if (!MediaFileUtils::IsFileExists(extraPathDir) && !MediaFileUtils::CreateDirectory(extraPathDir)) {
        MEDIA_WARN_LOG("Failed to create local extra data dir");
        result.errCode = E_HAS_FS_ERROR;
        return result;
    }
    int32_t ret = MovingPhotoFileUtils::ConvertToMovingPhoto(srcPath, destPath, videoPath, extraDataPath);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to convert live photo, ret:%{public}d", ret);
        (void)MediaFileUtils::DeleteFile(destPath);
        (void)MediaFileUtils::DeleteFile(videoPath);
        (void)MediaFileUtils::DeleteDir(extraPathDir);
        result.errCode = ret;
        return result;
    }
    (void)MediaFileUtils::DeleteFile(srcPath);
    result.errCode = E_OK;
    return result;
}


static std::string GetCloudPath(const std::string &path)
{
    std::string PHOTO_CLOUD_PATH_PREFIX = "/storage/cloud/files/";
    std::string PHOTO_MEDIA_PATH_PREFIX = "/storage/media/local/files/";
    std::string cloudPath = path;
    size_t pos = cloudPath.find(PHOTO_MEDIA_PATH_PREFIX);
    if (pos != std::string::npos) {
        cloudPath.replace(pos, PHOTO_MEDIA_PATH_PREFIX.length(), PHOTO_CLOUD_PATH_PREFIX);
    }
    return cloudPath;
}

static MoveResult HandleRemoveVideo(const std::string &srcPath)
{
    MoveResult result;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Failed to get rdbstore");
        result.errCode = E_HAS_DB_ERROR;
        return result;
    }
    std::string sql = "SELECT " + PhotoColumn::PHOTO_ID + ", " + PhotoColumn::STAGE_VIDEO_TASK_STATUS +
        " FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " + MediaColumn::MEDIA_FILE_PATH + " = ?;";
    std::vector<std::string> args = { GetCloudPath(srcPath) };
    auto resultSet = rdbStore->QuerySql(sql, args);
    if (resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        std::string photoId = MediaLibraryRdbStore::GetString(resultSet, PhotoColumn::PHOTO_ID);
        int32_t stageVideoTaskStatus = MediaLibraryRdbStore::GetInt(resultSet,
            PhotoColumn::STAGE_VIDEO_TASK_STATUS);
        resultSet->Close();
        if (!photoId.empty() && stageVideoTaskStatus ==
            static_cast<int32_t>(StageVideoTaskStatus::STAGE_TASK_DELIVERED)) {
            MultiStagesVideoCaptureManager::GetInstance().RemoveVideo(photoId, false);
        }
    } else if (resultSet != nullptr) {
        resultSet->Close();
    }
    result.errCode = E_OK;
    return result;
}

MoveResult MediaFileAccessUtils::ProcessMovingPhotoToLivePhoto(const std::string &srcPath,
    const std::string &destPath, FileSourceType destSourceType, bool deleteSrc)
{
    MoveResult result;
    std::string videoPath = MediaFileUtils::GetMovingPhotoVideoPath(srcPath);
    std::string extraDataPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(srcPath);
    int64_t coverPosition = 0;
    int32_t ret = MovingPhotoFileUtils::GetLivePhotoCoverPosition(videoPath, extraDataPath, coverPosition);
    if (ret != E_OK) {
        MEDIA_WARN_LOG("Failed to get cover position, use default 0, ret:%{public}d", ret);
        coverPosition = 0;
    }

    string livePhotoPath;
    ret = MovingPhotoFileUtils::ConvertToLivePhoto(srcPath, videoPath, extraDataPath,
        coverPosition, livePhotoPath);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to convert to livePhoto");
        result.errCode = E_HAS_FS_ERROR;
        return result;
    }

    AssetOperationInfo srcpath = AssetOperationInfo::CreateFromPath(livePhotoPath);
    result = MediaFileAccessUtils::MoveNormalAsset(srcpath, destPath, destSourceType, deleteSrc);
    if (result.errCode != E_OK) {
        MEDIA_ERR_LOG("failed to move live photo file, ret: %{public}d", ret);
        CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteFile(livePhotoPath),
            "failed to delete cache file, errno: %{public}d", errno);
        result.errCode = E_HAS_FS_ERROR;
        return result;
    }
    (void)MediaFileUtils::DeleteFile(srcPath);
    (void)MediaFileUtils::DeleteFile(videoPath);
    (void)MediaFileUtils::DeleteFile(extraDataPath);

    return HandleRemoveVideo(srcPath);
}

MoveResult MediaFileAccessUtils::MoveMovingPhotoAsset(const AssetOperationInfo &srcObj, const std::string &destPath,
    FileSourceType destSourceType, bool deleteSrc, RenameMode needRename)
{
    MEDIA_INFO_LOG("MoveMovingPhotoAsset destPath: %{public}s, deleteSrc: %{public}d, destSourceType: %{public}d",
        MediaFileUtils::DesensitizePath(destPath).c_str(), deleteSrc, static_cast<int32_t>(destSourceType));
    FileSourceType srcSourceType = srcObj.GetFileSourceType();
    bool isSrcLivePhoto = srcSourceType == FileSourceType::MEDIA_HO_LAKE ||
        srcSourceType == FileSourceType::FILE_MANAGER;
    bool isDestLivePhoto = destSourceType == FileSourceType::MEDIA_HO_LAKE ||
        destSourceType == FileSourceType::FILE_MANAGER;
    if (isSrcLivePhoto == isDestLivePhoto) {
        return MoveNormalAsset(srcObj, destPath, destSourceType, deleteSrc);
    }

    MoveResult result;
    std::string srcPath = GetAssetRealPath(srcObj);
    if (isSrcLivePhoto) {
        return ProcessLivePhotoToMovingPhoto(srcPath, destPath, deleteSrc);
    } else {
        return ProcessMovingPhotoToLivePhoto(srcPath, destPath, destSourceType, deleteSrc);
    }

    result.errCode = E_OK;
    return result;
}

MoveResult MediaFileAccessUtils::MoveBurstAsset(const AssetOperationInfo &srcObj, const std::string &destPath,
    FileSourceType destSourceType, bool deleteSrc, RenameMode needRename)
{
    MoveResult result;
    if (srcObj.GetFileSourceType() == FileSourceType::MEDIA && destSourceType == FileSourceType::FILE_MANAGER) {
        CHECK_AND_RETURN_RET_LOG(srcObj.GetBurstCoverLevel() == BurstCoverLevelType::COVER, result,
            "only support move burst cover asset from media to file manager");
    }
    return MoveNormalAsset(srcObj, destPath, destSourceType, deleteSrc, needRename);
}

int32_t MediaFileAccessUtils::CopyFile(const std::string &srcPath, std::string &destPath,
    std::function<void(uint64_t)> progressCallback, const std::string &requestId)
{
    std::string tmpPath = GetAssetRealPath(srcPath);
    if (srcPath.empty() || !MediaFileUtils::IsFileExists((tmpPath)) || !MediaFileUtils::IsFileValid(tmpPath)) {
        MEDIA_ERR_LOG("source file invalid! srcPath: %{public}s", DfxUtils::GetSafePath(tmpPath).c_str());
        return E_INNER_FAIL;
    }
    CHECK_AND_RETURN_RET_LOG(!destPath.empty(), E_INNER_FAIL, "empty destPath");
    if (progressCallback) {
        return MediaFileUtils::SegmentedCopyFileUtile(tmpPath, destPath, progressCallback, requestId);
    }
    return MediaFileUtils::CopyFileUtil(tmpPath, destPath) ? E_OK : E_INNER_FAIL;
}

bool MediaFileAccessUtils::DeleteAsset(const AssetOperationInfo &obj)
{
    CHECK_AND_RETURN_RET_LOG(obj.IsValid(), false, "obj is not valid");
    std::string assetPath = GetAssetRealPath(obj);
    return (remove(assetPath.c_str()) == E_SUCCESS);
}
} // namespace OHOS::Media
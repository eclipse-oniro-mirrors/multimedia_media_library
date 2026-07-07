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
#define MLOG_TAG "FileManagerAssetOperations"

#include "file_manager_asset_operations.h"

#include <sys/time.h>

#include "dfx_utils.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_unistore_manager.h"
#include "photo_album_column.h"
#include "result_set_utils.h"
#include "media_string_utils.h"
#include "media_file_access_utils.h"
#include "photo_file_utils.h"
#include "media_duplicate_checker_utils.h"
#include "moving_photo_file_utils.h"
#include "file_management_utils.h"
#include "photo_file_operation.h"

using namespace OHOS::NativeRdb;
using namespace OHOS::Media::AccurateRefresh;
namespace OHOS::Media {

static void UpdateModifyTime(const string &path, int64_t localMtime)
{
    struct timeval times[2];
    // set atime
    times[0].tv_sec = static_cast<time_t>(localMtime / MSEC_TO_SEC);
    times[0].tv_usec = static_cast<suseconds_t>((localMtime % MSEC_TO_SEC) * MSEC_TO_SEC);
    // set mtime
    times[1] = times[0];

    string pathToModifyTime = path;
    if (MediaStringUtils::StartsWith(path, PhotoColumn::FILES_CLOUD_DIR)) {
        pathToModifyTime.replace(0, PhotoColumn::FILES_CLOUD_DIR.length(), PhotoColumn::FILES_LOCAL_DIR);
    }

    if (utimes(pathToModifyTime.c_str(), times) < 0) {
        MEDIA_ERR_LOG("utimes failed %{public}d, path: %{public}s", errno,
            DfxUtils::GetSafePath(pathToModifyTime).c_str());
    }
}

static bool MoveAsset(const std::string &srcPath, const std::string &destPath, bool isLocalRename, int64_t dateModified)
{
    bool ret = false;
    if (isLocalRename) {
        std::string localPath = FileManagementUtils::GetLocalPath(destPath);
        bool opRet = MediaFileUtils::MoveFile(srcPath, localPath);
        if (!opRet) {
            MEDIA_WARN_LOG("MoveFile failed, try CrossPolicy mode.");
            opRet = MediaFileUtils::MoveFile(srcPath, localPath, true);
            if (opRet) {
                MediaFileUtils::ModifyFile(localPath, dateModified / MSEC_TO_SEC);
            }
        }
        if (!opRet) {
            MEDIA_ERR_LOG("MoveFile failed, srcPath: %{public}s, localPath: %{public}s",
                DfxUtils::GetSafePath(srcPath).c_str(), DfxUtils::GetSafePath(localPath).c_str());
            return false;
        }
    } else {
        ret = MediaFileUtils::CopyFileAndDelSrc(srcPath, destPath);
        if (!ret) {
            MEDIA_ERR_LOG("CopyFileUtil failed, srcPath: %{public}s, destPath: %{public}s",
                DfxUtils::GetSafePath(srcPath).c_str(), DfxUtils::GetSafePath(destPath).c_str());
            return false;
        }
    }
    return true;
}

int32_t FileManagerAssetOperations::MoveFileManagerAsset(
    const std::string &srcPath, const std::string &destPath, bool isMovingPhoto, bool isLocalRename)
{
    MEDIA_INFO_LOG("MoveFileManagerAsset from %{public}s to %{public}s", DfxUtils::GetSafePath(srcPath).c_str(),
        DfxUtils::GetSafePath(destPath).c_str());
    int64_t originalDataModified = 0;
    bool isDateModifiedValid = MediaFileUtils::GetDateModified(srcPath, originalDataModified);
    std::string parentDir = destPath.substr(0, destPath.find_last_of('/'));
    if (!MediaFileUtils::IsFileExists(parentDir) && !MediaFileUtils::CreateDirectory(parentDir)) {
        MEDIA_ERR_LOG("create dir %{private}s error, the file path is %{public}s",
            DfxUtils::GetSafePath(parentDir).c_str(), DfxUtils::GetSafePath(destPath).c_str());
    }
    int32_t ret = E_ERR;
    if (isMovingPhoto) {
        MoveResult result = MediaFileAccessUtils::ProcessMovingPhotoToLivePhoto(
            srcPath, destPath, FileSourceType::FILE_MANAGER, true);
        ret = result.errCode;
    } else {
        ret = MoveAsset(srcPath, destPath, isLocalRename, originalDataModified) ? E_OK : E_ERR;
    }
    if (isDateModifiedValid) {
        UpdateModifyTime(destPath, originalDataModified);
    }
    return ret;
}

int32_t FileManagerAssetOperations::MoveFileManagerAsset(const PhotosPo &photoInfo, const PhotosPo &targetPhotoInfo)
{
    if (photoInfo.fileSourceType != FileSourceType::FILE_MANAGER) {
        MEDIA_ERR_LOG("fileSourceType is not file manager");
        return E_ERR;
    }
    string filePath = photoInfo.data.value_or("");
    std::string parentDir = filePath.substr(0, filePath.find_last_of('/'));
    if (!MediaFileUtils::IsFileExists(parentDir) && !MediaFileUtils::CreateDirectory(parentDir)) {
        MEDIA_ERR_LOG("create dir %{private}s error, the file path is %{public}s",
            DfxUtils::GetSafePath(parentDir).c_str(), DfxUtils::GetSafePath(filePath).c_str());
    }
    int32_t ret = E_ERR;
    auto subtype = photoInfo.subtype.value_or(0);
    auto effectMode = photoInfo.movingPhotoEffectMode.value_or(0);
    auto originalSubtype = photoInfo.originalSubtype.value_or(0);
    int64_t dateModified = photoInfo.dateModified.value_or(0);
    auto isMovingPhoto = MovingPhotoFileUtils::IsMovingPhoto(subtype, effectMode, originalSubtype);
    if (isMovingPhoto) {
        MoveResult result = MediaFileAccessUtils::ProcessMovingPhotoToLivePhoto(
            photoInfo.data.value_or(""), targetPhotoInfo.storagePath.value_or(""),
            FileSourceType::FILE_MANAGER, true);
        ret = result.errCode;
    } else {
        ret = MoveAsset(photoInfo.data.value_or(""),
            targetPhotoInfo.storagePath.value_or(""), false, dateModified) ? E_OK : E_ERR;
        UpdateModifyTime(targetPhotoInfo.storagePath.value_or(""), dateModified);
    }
    return ret;
}

int32_t FileManagerAssetOperations::MoveFileManagerAsset(const PhotosPo &photoInfo)
{
    if (photoInfo.fileSourceType != FileSourceType::FILE_MANAGER) {
        MEDIA_ERR_LOG("fileSourceType is not file manager");
        return E_ERR;
    }
    string filePath = photoInfo.data.value_or("");
    std::string parentDir = filePath.substr(0, filePath.find_last_of('/'));
    if (!MediaFileUtils::IsFileExists(parentDir) && !MediaFileUtils::CreateDirectory(parentDir)) {
        MEDIA_ERR_LOG("create dir %{private}s error, the file path is %{public}s",
            DfxUtils::GetSafePath(parentDir).c_str(), DfxUtils::GetSafePath(filePath).c_str());
    }
    int32_t ret = E_ERR;
    auto subtype = photoInfo.subtype.value_or(0);
    auto effectMode = photoInfo.movingPhotoEffectMode.value_or(0);
    auto originalSubtype = photoInfo.originalSubtype.value_or(0);
    int64_t dateModified = photoInfo.dateModified.value_or(0);
    auto isMovingPhoto = MovingPhotoFileUtils::IsMovingPhoto(subtype, effectMode, originalSubtype);
    if (isMovingPhoto) {
        MoveResult result = MediaFileAccessUtils::ProcessLivePhotoToMovingPhoto(photoInfo.storagePath.value_or(""),
            photoInfo.data.value_or(""), true);
        ret = result.errCode;
    } else {
        ret = MoveAsset(photoInfo.storagePath.value_or(""),
            photoInfo.data.value_or(""), true, dateModified) ? E_OK : E_ERR;
    }
    return ret;
}

static int32_t UpdateFileManagerAsset(AccurateRefreshBase &refresh, const MoveAssetsToFileManagerUpdateData &data,
    bool needRefresh)
{
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, data.mediaId);
    ValuesBucket values;
    if (data.storagePath.empty()) {
        values.PutNull(PhotoColumn::PHOTO_STORAGE_PATH);
    } else {
        values.PutString(PhotoColumn::PHOTO_STORAGE_PATH, data.storagePath);
    }
    values.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, static_cast<int32_t>(FileSourceType::MEDIA));
    int32_t changedRows = -1;
    int32_t ret = E_ERR;
    if (needRefresh) {
        ret = refresh.Update(changedRows, values, predicates);
    } else {
        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB, "get rdb store error");
        ret = rdbStore->Update(changedRows, values, predicates);
    }
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "Failed to update file manager asset");
    return E_OK;
}

static int32_t ReNameAssetsFromFileManager(AccurateRefreshBase &refresh,
    shared_ptr<NativeRdb::ResultSet> &resultSet, bool needRefresh)
{
    int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    std::string fileManagerPath = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
    std::string mediaPath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    if (mediaPath.empty() || fileManagerPath.empty()) {
        MEDIA_ERR_LOG("fileId: %{public}d, mediaPath or fileManagerPath is empty", fileId);
        return E_ERR;
    }
    int32_t subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    int32_t effectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
    int32_t originalSubtype = GetInt32Val(PhotoColumn::PHOTO_ORIGINAL_SUBTYPE, resultSet);
    int32_t position = GetInt32Val(PhotoColumn::PHOTO_POSITION, resultSet);
    // 刷新数据库字段
    MoveAssetsToFileManagerUpdateData data;
    data.mediaId = fileId;
    data.storagePath = "";
    int32_t ret = UpdateFileManagerAsset(refresh, data, needRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "Failed to update file manager asset");
    // 纯云资产字段同步刷新，但无需移动原文件
    if (position == static_cast<int32_t>(PhotoPositionType::CLOUD)) {
        return E_OK;
    }
    // 先判断是否是动图
    bool isMovingPhoto = MovingPhotoFileUtils::IsMovingPhoto(subtype, effectMode, originalSubtype);
    if (isMovingPhoto) {
        MoveResult result = MediaFileAccessUtils::ProcessLivePhotoToMovingPhoto(fileManagerPath, mediaPath, true);
        ret = result.errCode;
    } else {
        ret = FileManagerAssetOperations::MoveFileManagerAsset(fileManagerPath, mediaPath, false, true);
    }
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "move asset from %{public}s to %{public}s error, errno: %{public}d",
        DfxUtils::GetSafePath(fileManagerPath).c_str(), DfxUtils::GetSafePath(mediaPath).c_str(), errno);
    return E_OK;
}

int32_t FileManagerAssetOperations::MoveAssetsFromFileManager(AssetAccurateRefresh &refresh,
    const std::vector<std::string> &ids, bool needRefresh)
{
    MEDIA_INFO_LOG("MoveAssetsFromFileManager enter %{public}zu", ids.size());
    CHECK_AND_RETURN_RET_LOG(!ids.empty(), E_INVALID_ARGUMENTS, "move asset param error");
    vector<string> columns = {MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH, PhotoColumn::PHOTO_STORAGE_PATH,
        PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::MOVING_PHOTO_EFFECT_MODE, PhotoColumn::PHOTO_ORIGINAL_SUBTYPE,
        PhotoColumn::PHOTO_POSITION };
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(MediaColumn::MEDIA_ID, ids);
    predicates.EqualTo(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, static_cast<int32_t>(FileSourceType::FILE_MANAGER));
    auto resultSet = MediaLibraryRdbStore::Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RDB, "query storage path error");
    int32_t rowCount = -1;
    int32_t ret = resultSet->GetRowCount(rowCount);
    if (ret != E_OK || rowCount < 0) {
        MEDIA_ERR_LOG("move asset get row count error");
        resultSet->Close();
        return E_ERR;
    }
    if (rowCount == 0) {
        MEDIA_WARN_LOG("move asset row count is 0");
        resultSet->Close();
        return E_OK;
    }
    ret = E_OK;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        if (ReNameAssetsFromFileManager(refresh, resultSet, needRefresh) != E_OK) {
            MEDIA_ERR_LOG("move asset from file manager error, errno: %{public}d", errno);
            ret = E_ERR;
        }
    }
    resultSet->Close();
    return ret;
}

static int32_t UpdateFileManageAssetInfo(AccurateRefreshBase &refresh,
    const int32_t &fileId, const std::string &sourcePath, const std::string &displayName, const std::string &title)
{
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    ValuesBucket values;
    if (sourcePath.empty()) {
        values.PutNull(PhotoColumn::PHOTO_STORAGE_PATH);
    } else {
        values.PutString(PhotoColumn::PHOTO_STORAGE_PATH, sourcePath);
    }
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutString(MediaColumn::MEDIA_TITLE, title);
    values.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, static_cast<int32_t>(FileSourceType::FILE_MANAGER));
    int32_t changedRows = -1;
    int32_t ret = refresh.Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK && changedRows > 0,
        E_ERR,
        "update path error ret: %{public}d, change rows: %{public}d",
        ret,
        changedRows);
    return E_OK;
}

static std::string ConvertPath(const std::string &originalPath)
{
    const std::string oldPrefix = "/storage/emulated/0/FromDocs/";
    const std::string newPrefix = "/storage/media/local/files/Docs/";
    if (originalPath.compare(0, oldPrefix.size(), oldPrefix) == 0) {
        return newPrefix + originalPath.substr(oldPrefix.size());
    }
    return originalPath;
}

static bool IsMovingPhoto(shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    int32_t subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    int32_t effectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
    int32_t originalSubtype = GetInt32Val(PhotoColumn::PHOTO_ORIGINAL_SUBTYPE, resultSet);
    return MovingPhotoFileUtils::IsMovingPhoto(subtype, effectMode, originalSubtype);
}

static bool CheckBurstToFileManage(AccurateRefreshBase &refresh,
    shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    int32_t subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    int32_t originalSubtype = GetInt32Val(PhotoColumn::PHOTO_ORIGINAL_SUBTYPE, resultSet);
    bool isBurst = (subtype == static_cast<int32_t>(PhotoSubType::BURST) ||
        originalSubtype == static_cast<int32_t>(PhotoSubType::BURST));
    if (!isBurst) {
        return false;
    }
    int32_t burstCoverLevel = GetInt32Val(PhotoColumn::PHOTO_BURST_COVER_LEVEL, resultSet);
    if (burstCoverLevel == static_cast<int32_t>(BurstCoverLevelType::COVER)) {
        return false;
    }
    MEDIA_INFO_LOG("burst member to file manager");
    return true;
}

static int32_t MoveAssetToFileManage(AccurateRefreshBase &refresh,
    shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    int32_t mediaId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    std::string sourcePath = GetStringVal(PhotoColumn::PHOTO_SOURCE_PATH, resultSet);
    std::string outerFilePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    std::string tmpTitle = GetStringVal(MediaColumn::MEDIA_TITLE, resultSet);
    std::string tmpDisplayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    bool cond = outerFilePath.empty() || sourcePath.empty();
    CHECK_AND_RETURN_RET(!cond, E_ERR);
    std::string innerFileSourcePath = ConvertPath(sourcePath);
    // 非文管路径不处理
    if (!PhotoFileUtils::CheckFileManagerRealPath(innerFileSourcePath)) {
        MEDIA_INFO_LOG("file is not file manager, path: %{public}s",
            DfxUtils::GetSafePath(innerFileSourcePath).c_str());
        return E_OK;
    }
    // 连拍成员仅更新数据库
    if (CheckBurstToFileManage(refresh, resultSet)) {
        return E_OK;
    }
    // 纯云资产仅更新数据库
    if (GetInt32Val(PhotoColumn::PHOTO_POSITION, resultSet) == static_cast<int32_t>(PhotoPositionType::CLOUD)) {
        MEDIA_INFO_LOG("CLOUD file manager");
        UpdateFileManageAssetInfo(refresh, mediaId, innerFileSourcePath, tmpDisplayName, tmpTitle);
        return E_OK;
    }
    if (!MediaFileUtils::IsFileExists(outerFilePath)) {
        MEDIA_ERR_LOG("file not exist %{public}s", DfxUtils::GetSafePath(outerFilePath).c_str());
        return E_ERR;
    }
    std::string tmpInnerFilePath = innerFileSourcePath;
    // 同路径存在同名文件
    if (MediaFileUtils::IsFileExists(tmpInnerFilePath)) {
        if (MediaFileAccessUtils::HandleSameNameRename(tmpInnerFilePath, tmpInnerFilePath,
            tmpTitle, tmpDisplayName) != E_OK) {
            MEDIA_ERR_LOG("can not move file to %{public}s", DfxUtils::GetSafePath(tmpInnerFilePath).c_str());
            return E_ERR;
        }
    }
    //移动源文件前需要刷新数据库字段
    UpdateFileManageAssetInfo(refresh, mediaId, tmpInnerFilePath, tmpDisplayName, tmpTitle);
    int32_t ret = FileManagerAssetOperations::MoveFileManagerAsset(outerFilePath, tmpInnerFilePath,
        IsMovingPhoto(resultSet), false);
    CHECK_AND_PRINT_LOG(ret == E_OK, "move asset %{public}s to %{public}s error, errno: %{public}d.",
        DfxUtils::GetSafePath(outerFilePath).c_str(), DfxUtils::GetSafePath(tmpInnerFilePath).c_str(), errno);
    if (ret == E_OK) {
        if (tmpInnerFilePath.compare(innerFileSourcePath) != E_OK) {
            UpdateFileManageAssetInfo(refresh, mediaId, tmpInnerFilePath, tmpDisplayName, tmpTitle);
        }
    }
    return E_OK;
}

int32_t FileManagerAssetOperations::MoveAssetsToFileManager(AccurateRefresh::AssetAccurateRefresh &refresh,
    const std::vector<std::string> &ids)
{
    MEDIA_INFO_LOG("MoveAssetsToFileManage enter %{public}zu", ids.size());
    CHECK_AND_RETURN_RET_LOG(!ids.empty(), E_INVALID_ARGUMENTS, "move asset param error");
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(MediaColumn::MEDIA_ID, ids);
    predicates.EqualTo(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, static_cast<int32_t>(FileSourceType::MEDIA));

    vector<string> columns = {MediaColumn::MEDIA_ID, MediaColumn::MEDIA_TITLE, MediaColumn::MEDIA_NAME,
        MediaColumn::MEDIA_FILE_PATH, PhotoColumn::PHOTO_STORAGE_PATH, PhotoColumn::PHOTO_SOURCE_PATH,
        PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::MOVING_PHOTO_EFFECT_MODE, PhotoColumn::PHOTO_ORIGINAL_SUBTYPE,
        PhotoColumn::PHOTO_POSITION, PhotoColumn::PHOTO_BURST_COVER_LEVEL };
    auto resultSet = MediaLibraryRdbStore::Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RDB, "query storage path error");
    int32_t rowCount = -1;
    int32_t ret = resultSet->GetRowCount(rowCount);
    if (ret != E_OK || rowCount < 0) {
        MEDIA_ERR_LOG("move asset get row count error");
        resultSet->Close();
        return E_ERR;
    }
    if (rowCount == 0) {
        MEDIA_WARN_LOG("move asset row count is 0");
        resultSet->Close();
        return E_OK;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        MoveAssetToFileManage(refresh, resultSet);
    }
    resultSet->Close();
    return E_OK;
}

int32_t FileManagerAssetOperations::CheckAndRenameFileManagerAsset(AccurateRefresh::AccurateRefreshBase &refresh,
    MediaLibraryCommand &cmd, const std::shared_ptr<FileAsset> &fileAsset)
{
    string prefix = "/storage/media/local/files/Docs/";
    int32_t fileSourceType = fileAsset->GetFileSourceType();
    string storagePath = fileAsset->GetStoragePath();
    MEDIA_INFO_LOG("modify file source type:%{public}d", fileSourceType);
    if (storagePath.empty() || storagePath.find(prefix) == std::string::npos ||
        fileSourceType != FileSourceType::FILE_MANAGER) {
        MEDIA_INFO_LOG("not file manager asset, no need to modify");
        return E_OK;
    }
    int32_t fileId = fileAsset->GetId();
    CHECK_AND_RETURN_RET_LOG(fileId > 0, E_ERR, "fileId is empty");
    MEDIA_INFO_LOG("enter CheckAndRenameFileManagerAsset fileId:%{public}d", fileId);
    ValueObject valueDisplayName;
    string newDisplayName;
    if (cmd.GetValueBucket().GetObject(MediaColumn::MEDIA_NAME, valueDisplayName)) {
        valueDisplayName.GetString(newDisplayName);
    }
    // 非重命名资产场景，newDisplayName可能为空
    CHECK_AND_RETURN_RET_LOG(!newDisplayName.empty(), E_OK, "newDisplayName is empty");
    size_t lastDot = newDisplayName.rfind('.');
    if (lastDot == std::string::npos) {
        lastDot = newDisplayName.length();
    }
    std::string newTitle = newDisplayName.substr(0, lastDot);

    CHECK_AND_RETURN_RET_LOG(MediaDuplicateCheckerUtils::checkPhotoNameDuplicate(to_string(fileId), newTitle) == E_OK,
        E_ERR, "Invalid title");

    std::string newPhotoPath = MediaDuplicateCheckerUtils::replaceFilename(storagePath, newTitle);
    if (rename(storagePath.c_str(), newPhotoPath.c_str()) == 0) {
        AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
        ValuesBucket values;
        values.PutString(PhotoColumn::PHOTO_STORAGE_PATH, newPhotoPath);
        int32_t changedRows = -1;
        int32_t ret = refresh.Update(changedRows, values, predicates);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK && changedRows > 0, E_ERR,
            "update asset error ret: %{public}d, change rows: %{public}d", ret, changedRows);
    } else {
        MEDIA_ERR_LOG("rename asset failed! oldPath:%{public}s, newPath:%{public}s, errno:%{public}d, error:%{public}s",
            storagePath.c_str(), newPhotoPath.c_str(), errno, strerror(errno));
        return E_ERR;
    }
    return E_OK;
}
}  // namespace OHOS::Media
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

int32_t FileManagerAssetOperations::MoveFileManagerAsset(
    const std::string &srcPath, const std::string &destPath, bool isMovingPhoto)
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
        ret = MediaFileUtils::CopyFileAndDelSrc(srcPath, destPath) ? E_OK : E_ERR;
    }
    if (isDateModifiedValid) {
        UpdateModifyTime(destPath, originalDataModified);
    }
    return ret;
}

static int32_t UpdateFileManagerAsset(const std::vector<std::string> &updateIds)
{
    CHECK_AND_RETURN_RET_LOG(!updateIds.empty(), E_INVALID_ARGUMENTS, "update param error");
    std::string inClause;
    for (size_t i = 0; i < updateIds.size(); i++) {
        if (i > 0) {
            inClause += ",";
        }
        inClause += updateIds[i];
    }
    const std::string updateSql = "UPDATE Photos SET storage_path = data,"
        " file_source_type = 0 WHERE file_id IN (" + inClause + ");";
    MEDIA_INFO_LOG("UpdateFileManagerAsset updateSql = %{public}s", updateSql.c_str());
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    int32_t ret = rdbStore->ExecuteSql(updateSql);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "Failed to update file manager asset");
    return E_OK;
}

int32_t FileManagerAssetOperations::MoveAssetsFromFileManager(const std::vector<std::string> &ids)
{
    MEDIA_INFO_LOG("MoveAssetsFromFileManager enter %{public}zu", ids.size());
    CHECK_AND_RETURN_RET_LOG(!ids.empty(), E_INVALID_ARGUMENTS, "move asset param error");
    vector<string> columns = {MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH, PhotoColumn::PHOTO_STORAGE_PATH,
        PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::MOVING_PHOTO_EFFECT_MODE, PhotoColumn::PHOTO_ORIGINAL_SUBTYPE};
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
    std::vector<std::string> updateIds;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        std::string fileManagerPath = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
        std::string mediaPath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        if (mediaPath.empty() || fileManagerPath.empty()) {
            continue;
        }
        int32_t subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
        int32_t effectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
        int32_t originalSubtype = GetInt32Val(PhotoColumn::PHOTO_ORIGINAL_SUBTYPE, resultSet);
        // 先判断是否是动图
        bool isMovingPhoto = MovingPhotoFileUtils::IsMovingPhoto(subtype, effectMode, originalSubtype);
        if (isMovingPhoto) {
            MoveResult result = MediaFileAccessUtils::ProcessLivePhotoToMovingPhoto(fileManagerPath, mediaPath, true);
            ret = result.errCode;
        } else {
            ret = MoveFileManagerAsset(fileManagerPath, mediaPath);
        }
        CHECK_AND_PRINT_LOG(ret == E_OK, "move asset from %{public}s to %{public}s error, errno: %{public}d",
            DfxUtils::GetSafePath(fileManagerPath).c_str(), DfxUtils::GetSafePath(mediaPath).c_str(), errno);
        if (ret == E_OK) {
            updateIds.emplace_back(to_string(fileId));
        }
    }
    resultSet->Close();
    return UpdateFileManagerAsset(updateIds);
}

static int32_t UpdateFileManageAssetInfo(AccurateRefreshBase &refresh,
    const int32_t &fileId, const std::string &sourcePath, const std::string &displayName, const std::string &title)
{
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    ValuesBucket values;
    values.PutString(PhotoColumn::PHOTO_SOURCE_PATH, sourcePath);
    values.PutString(PhotoColumn::PHOTO_STORAGE_PATH, sourcePath);
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutString(MediaColumn::MEDIA_TITLE, title);
    int32_t changedRows = -1;
    int32_t ret = refresh.Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK && changedRows > 0,
        E_ERR,
        "update path error ret: %{public}d, change rows: %{public}d",
        ret,
        changedRows);
    return E_OK;
}

static int32_t UpdateAfterMoveAssetsToFileManage(AccurateRefreshBase &refresh,
    const std::vector<MoveAssetsToFileManagerUpdateData> &updateDatas, const FileSourceType &type)
{
    CHECK_AND_RETURN_RET_LOG(!updateDatas.empty(), E_INVALID_ARGUMENTS, "update file source type param error");
    for (auto &updateData : updateDatas) {
        AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(MediaColumn::MEDIA_ID, updateData.mediaId);
        ValuesBucket values;
        values.PutString(MediaColumn::MEDIA_NAME, updateData.displayName);
        values.PutString(MediaColumn::MEDIA_TITLE, updateData.title);
        values.PutString(PhotoColumn::PHOTO_STORAGE_PATH, updateData.storagePath);
        values.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, static_cast<int32_t>(type));
        int32_t changedRows = -1;
        int32_t ret = refresh.Update(changedRows, values, predicates);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "update move assets from lake error");
    }
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

static int32_t MoveAssetToFileManage(AccurateRefreshBase &refresh,
    shared_ptr<NativeRdb::ResultSet> &resultSet, std::vector<MoveAssetsToFileManagerUpdateData> &updateDatas)
{
    int32_t mediaId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    std::string sourcePath = GetStringVal(PhotoColumn::PHOTO_SOURCE_PATH, resultSet);
    std::string outerFilePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    std::string title = GetStringVal(MediaColumn::MEDIA_TITLE, resultSet);
    std::string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    bool cond = outerFilePath.empty() || sourcePath.empty();
    CHECK_AND_RETURN_RET(!cond, E_ERR);
    std::string innerFileSourcePath = ConvertPath(sourcePath);
    // 非文管路径不处理
    if (!PhotoFileUtils::CheckFileManagerRealPath(innerFileSourcePath)) {
        MEDIA_INFO_LOG("file is not file manager, path: %{public}s",
            DfxUtils::GetSafePath(innerFileSourcePath).c_str());
        return E_OK;
    }
    if (!MediaFileUtils::IsFileExists(outerFilePath)) {
        MEDIA_ERR_LOG("file not exist %{public}s", DfxUtils::GetSafePath(outerFilePath).c_str());
        return E_ERR;
    }
    std::string tmpInnerFilePath = innerFileSourcePath;
    std::string tmpTitle = title;
    std::string tmpDisplayName = displayName;
    // 同路径存在同名文件
    if (MediaFileUtils::IsFileExists(tmpInnerFilePath)) {
        AssetOperationInfo srcObj = AssetOperationInfo::CreateFromFileId(to_string(mediaId));
        if (MediaFileAccessUtils::HandleSameNameRename(srcObj, tmpInnerFilePath, tmpInnerFilePath,
            tmpTitle, tmpDisplayName) != E_OK) {
            MEDIA_ERR_LOG("can not move file to %{public}s", DfxUtils::GetSafePath(tmpInnerFilePath).c_str());
            return E_ERR;
        }
    }
    //移动源文件前需要刷新数据库字段
    UpdateFileManageAssetInfo(refresh, mediaId, tmpInnerFilePath, tmpDisplayName, tmpTitle);
    MoveAssetsToFileManagerUpdateData updateData;
    updateData.mediaId = mediaId;
    updateData.title = tmpTitle;
    updateData.displayName = tmpDisplayName;
    updateData.sourcePath = "";
    updateData.storagePath = tmpInnerFilePath;
    bool isMovingPhoto = IsMovingPhoto(resultSet);
    int32_t ret = FileManagerAssetOperations::MoveFileManagerAsset(outerFilePath, tmpInnerFilePath, isMovingPhoto);
    CHECK_AND_PRINT_LOG(ret == E_OK, "move asset %{public}s to %{public}s error, errno: %{public}d.",
        DfxUtils::GetSafePath(outerFilePath).c_str(), DfxUtils::GetSafePath(tmpInnerFilePath).c_str(), errno);
    if (ret == E_OK) {
        updateDatas.emplace_back(updateData);
        if (tmpInnerFilePath.compare(innerFileSourcePath) != E_OK) {
            UpdateFileManageAssetInfo(refresh, mediaId, tmpInnerFilePath, tmpDisplayName, tmpTitle);
        }
    }
    return E_OK;
}

int32_t FileManagerAssetOperations::MoveAssetsToFileManager(AccurateRefresh::AccurateRefreshBase &refresh,
    const std::vector<std::string> &ids)
{
    MEDIA_INFO_LOG("MoveAssetsToFileManage enter %{public}zu", ids.size());
    CHECK_AND_RETURN_RET_LOG(!ids.empty(), E_INVALID_ARGUMENTS, "move asset param error");
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(MediaColumn::MEDIA_ID, ids);
    predicates.EqualTo(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, static_cast<int32_t>(FileSourceType::MEDIA));
    predicates.And()
        ->BeginWrap()
        ->IsNotNull(PhotoColumn::PHOTO_STORAGE_PATH)
        ->And()
        ->NotEqualTo(PhotoColumn::PHOTO_STORAGE_PATH, "")
        ->EndWrap();
    predicates.And()
        ->BeginWrap()
        ->EqualTo(PhotoColumn::PHOTO_POSITION, to_string(static_cast<int32_t>(PhotoPositionType::LOCAL)))
        ->Or()
        ->EqualTo(PhotoColumn::PHOTO_POSITION, to_string(static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD)))
        ->EndWrap();
    predicates.EqualTo(MediaColumn::MEDIA_HIDDEN, 0);

    vector<string> columns = {MediaColumn::MEDIA_ID, MediaColumn::MEDIA_TITLE, MediaColumn::MEDIA_NAME,
        MediaColumn::MEDIA_FILE_PATH, PhotoColumn::PHOTO_STORAGE_PATH, PhotoColumn::PHOTO_SOURCE_PATH,
        PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::MOVING_PHOTO_EFFECT_MODE, PhotoColumn::PHOTO_ORIGINAL_SUBTYPE};
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
    std::vector<MoveAssetsToFileManagerUpdateData> updateDatas;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        MoveAssetToFileManage(refresh, resultSet, updateDatas);
    }
    resultSet->Close();
    return UpdateAfterMoveAssetsToFileManage(refresh, updateDatas, FileSourceType::FILE_MANAGER);
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
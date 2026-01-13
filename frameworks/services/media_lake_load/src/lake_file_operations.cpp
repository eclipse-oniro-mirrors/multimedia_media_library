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
#define MLOG_TAG "LakeFileOperations"

#include "lake_file_operations.h"

#include <sys/stat.h>
#include <sys/time.h>

#include "nlohmann/json.hpp"

#include "dfx_utils.h"
#include "lake_file_utils.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "photo_album_column.h"
#include "result_set_utils.h"
#include "photo_file_utils.h"

using namespace OHOS::NativeRdb;
using namespace OHOS::Media::AccurateRefresh;
namespace OHOS::Media {

#define IN_LAKE_PATH_LOGIC_PREFIX "/data/service/el2/100/hmdfs/account/files/Docs/HO_DATA_EXT_MISC/"
#define IN_LAKE_MOUNT_OUTLAKE_PATH_PREFIEX "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/"
constexpr size_t INLAKE_PATH_LOGIC_LEN = sizeof(IN_LAKE_PATH_LOGIC_PREFIX) - 1;
constexpr uint32_t RENAME_MAX_RETRY_COUNT = 10000;

static void UpdateModifyTime(const string &path, int64_t localMtime)
{
    struct timeval times[2];
    // set atime
    times[0].tv_sec = static_cast<time_t>(localMtime / MSEC_TO_SEC);
    times[0].tv_usec = static_cast<suseconds_t>((localMtime % MSEC_TO_SEC) * MSEC_TO_SEC);
    // set mtime
    times[1] = times[0];

    string pathToModifyTime = path;
    if (MediaFileUtils::StartsWith(path, PhotoColumn::FILES_CLOUD_DIR)) {
        pathToModifyTime.replace(0, PhotoColumn::FILES_CLOUD_DIR.length(), PhotoColumn::FILES_LOCAL_DIR);
    }

    if (utimes(pathToModifyTime.c_str(), times) < 0) {
        MEDIA_ERR_LOG("utimes failed %{public}d, path: %{public}s", errno,
            DfxUtils::GetSafePath(pathToModifyTime).c_str());
    }
}

int32_t LakeFileOperations::MoveLakeFile(const std::string &srcPath, const std::string &destPath)
{
    MEDIA_INFO_LOG("MoveLakeFile from %{public}s to %{public}s", DfxUtils::GetSafePath(srcPath).c_str(),
        DfxUtils::GetSafePath(destPath).c_str());
    int64_t originalDataModified = 0;
    bool isDateModifiedValid = MediaFileUtils::GetDateModified(srcPath, originalDataModified);
    std::string parentDir = destPath.substr(0, destPath.find_last_of('/'));
    if (!MediaFileUtils::IsFileExists(parentDir) && !MediaFileUtils::CreateDirectory(parentDir)) {
        MEDIA_ERR_LOG("create dir %{private}s error, the file path is %{public}s",
            DfxUtils::GetSafePath(parentDir).c_str(), DfxUtils::GetSafePath(destPath).c_str());
    }
    int32_t ret = MediaFileUtils::CopyFileAndDelSrc(srcPath, destPath) ? E_OK : E_ERR;
    if (isDateModifiedValid) {
        UpdateModifyTime(destPath, originalDataModified);
    }
    return ret;
}

static bool IsAlbumHasSameNameAsset(const int32_t &fileId, const std::string &displayName)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "get rdbstore error");
    std::string sql = "SELECT COUNT(1) AS count FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " +
                      MediaColumn::MEDIA_NAME + " LIKE ? AND " + MediaColumn::MEDIA_ID + "<>? AND " +
                      PhotoColumn::PHOTO_OWNER_ALBUM_ID + "=(SELECT " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + " FROM " +
                      PhotoColumn::PHOTOS_TABLE + " WHERE " + MediaColumn::MEDIA_ID + "=?);";
    std::vector<std::string> args = {displayName, std::to_string(fileId), std::to_string(fileId)};
    auto resultSet = rdbStore->QuerySql(sql, args);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "query database error");
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == E_OK, false, "query resultset error");
    int32_t count = MediaLibraryRdbStore::GetInt(resultSet, "count");
    MEDIA_INFO_LOG("IsAlbumHasSameNameAsset fileId: %{public}d, count: %{public}d", fileId, count);
    return count > 0;
}

static int32_t HandleSameNameRename(const int32_t fileId, const std::string targetPath, std::string &newTargetPath,
    std::string &newTitle, std::string &newDisplayName)
{
    MEDIA_INFO_LOG("HandleSameNameRename target: %{public}s", DfxUtils::GetSafePath(targetPath).c_str());
    // 例如targetPath = /home/aaa/bb/test.mp4
    CHECK_AND_RETURN_RET_LOG(!targetPath.empty(), E_ERR, "targetPath is empty!");
    // 提取文件路径，dirPath = /home/aaa/bb/
    size_t slashPos = targetPath.rfind('/');
    CHECK_AND_RETURN_RET_LOG(slashPos != std::string::npos, E_ERR, "targetPath error");
    string dirPath = targetPath.substr(0, slashPos + 1);
    // 提取文件名和扩展名之间.的索引位置
    size_t dotPos = targetPath.rfind('.');
    if (dotPos == std::string::npos) {
        // 说明文件没有扩展名
        MEDIA_INFO_LOG("fileName have no suffix");
        dotPos = targetPath.length();
    }
    CHECK_AND_RETURN_RET_LOG(dotPos > slashPos,
        E_ERR,
        "targetPath format error %{public}s, slash: %{public}u, dot: %{public}u",
        DfxUtils::GetSafePath(targetPath).c_str(),
        dotPos,
        slashPos);
    // 提取文件名test
    std::string fileName = targetPath.substr(slashPos + 1, dotPos - slashPos - 1);
    CHECK_AND_RETURN_RET_LOG(!fileName.empty(), E_ERR, "file name is too short");
    // 提取扩展名.mp4
    std::string fileExtension = targetPath.substr(dotPos);
    std::string tmpName;
    size_t leftParentheses = fileName.rfind('(');
    size_t rightParentheses = fileName.rfind(')');
    int32_t num = 1;
    if (leftParentheses != std::string::npos && rightParentheses != std::string::npos &&
        leftParentheses < rightParentheses) {
        std::string numStr = fileName.substr(leftParentheses + 1, rightParentheses);
        if (all_of(numStr.begin(), numStr.end(), ::isdigit)) {
            StrToInt(numStr, num);
            tmpName = fileName.substr(0, leftParentheses + 1) + to_string(num + 1) + ")";
        }
    }
    if (tmpName.empty()) {
        fileName.append("(1)");
        tmpName = fileName;
        leftParentheses = fileName.rfind('(');
    }
    uint32_t retryCount = 1;
    while ((MediaFileUtils::IsFileExists(dirPath + tmpName + fileExtension) ||
        IsAlbumHasSameNameAsset(fileId, tmpName + fileExtension)) &&
        retryCount <= RENAME_MAX_RETRY_COUNT) {
        // 存在同名文件，重命名
        tmpName = fileName.substr(0, leftParentheses + 1) + to_string(num + retryCount) + ")";
        retryCount++;
    }
    CHECK_AND_RETURN_RET_LOG(retryCount <= RENAME_MAX_RETRY_COUNT, E_ERR, "rename file reach max count");
    newTitle = tmpName;
    newDisplayName = tmpName + fileExtension;
    newTargetPath = dirPath + newDisplayName;
    return E_OK;
}

static int32_t CreateParentDir(int32_t &targetAlbumId, std::string &parentDir)
{
    // 查询目标相册lpath
    AbsRdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, targetAlbumId);
    auto resultSet = MediaLibraryRdbStore::Query(predicates, { PhotoAlbumColumns::ALBUM_LPATH });
    if (resultSet->GoToNextRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("get target album lpath error");
        resultSet->Close();
        return E_ERR;
    }
    std::string innerLakeLpath = GetStringVal(PhotoAlbumColumns::ALBUM_LPATH, resultSet);
    resultSet->Close();
    std::string pathPreFix(IN_LAKE_MOUNT_OUTLAKE_PATH_PREFIEX);
    // 若lpath不存在，新建目录
    const std::string prefix = "/";
    if (innerLakeLpath.find(prefix) == 0) {
        innerLakeLpath = innerLakeLpath.substr(1);
    }
    if (static_cast<int32_t>(innerLakeLpath.rfind(prefix)) !=
        (static_cast<int32_t>(innerLakeLpath.length()) - static_cast<int32_t>(prefix.length()))) {
        innerLakeLpath = innerLakeLpath + prefix;
    }
    parentDir = pathPreFix + innerLakeLpath;
    if (!MediaFileUtils::IsDirectory(parentDir)) {
        MEDIA_INFO_LOG("CreateDirectory START");
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(parentDir), E_HAS_FS_ERROR,
            "create dir %{private}s error", parentDir.c_str());
    }
    return E_OK;
}

static int32_t UpdateAfterMoveAssetsToLake(AccurateRefreshBase &refresh,
    const std::vector<MoveAssetsToLakeUpdateData> &updateDatas, const FileSourceType &type)
{
    CHECK_AND_RETURN_RET_LOG(!updateDatas.empty(), E_INVALID_ARGUMENTS, "update file source type param error");
    for (auto &updateData : updateDatas) {
        AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(MediaColumn::MEDIA_ID, updateData.mediaId);
        ValuesBucket values;
        values.PutString(PhotoColumn::PHOTO_STORAGE_PATH, updateData.storagePath);
        values.PutString(MediaColumn::MEDIA_NAME, updateData.displayName);
        values.PutString(MediaColumn::MEDIA_TITLE, updateData.title);
        values.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, static_cast<int32_t>(type));
        int32_t changedRows = -1;
        int32_t ret = refresh.Update(changedRows, values, predicates);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "update move assets from lake error");
    }
    return E_OK;
}

static int32_t UpdateLakeAssetInfo(AccurateRefreshBase &refresh,
    const int32_t &fileId, const std::string &storagePath, const std::string &displayName, const std::string &title)
{
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    ValuesBucket values;
    values.PutString(PhotoColumn::PHOTO_STORAGE_PATH, storagePath);
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

static int32_t UpdateOuterLakeAssetInfo(AccurateRefreshBase &refresh,
    const int32_t &fileId, const std::string &storagePath, const std::string &displayName, const std::string &title)
{
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    predicates.BeginWrap();
    predicates.EqualTo(MediaColumn::MEDIA_HIDDEN, 1)
        ->Or()->GreaterThan(MediaColumn::MEDIA_DATE_TRASHED, to_string(0))
        ->Or()->EqualTo(PhotoColumn::PHOTO_POSITION, to_string(static_cast<int32_t>(PhotoPositionType::CLOUD)));
    predicates.EndWrap();
    ValuesBucket values;
    values.PutString(PhotoColumn::PHOTO_STORAGE_PATH, storagePath);
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

static int32_t MoveAssetToLake(AccurateRefreshBase &refresh,
    shared_ptr<NativeRdb::ResultSet> &resultSet, std::vector<MoveAssetsToLakeUpdateData> &updateDatas)
{
    int32_t mediaId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    std::string innerLakePath = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
    std::string outerLakePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    std::string title = GetStringVal(MediaColumn::MEDIA_TITLE, resultSet);
    std::string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    if (outerLakePath.empty() || innerLakePath.empty()) {
        return E_ERR;
    }
    std::string tmpInnerLakePath = innerLakePath;
    std::string tmpTitle = title;
    std::string tmpDisplayName = displayName;
    // 湖外同路径存在同名文件
    if (MediaFileUtils::IsFileExists(tmpInnerLakePath)) {
        if (HandleSameNameRename(mediaId, tmpInnerLakePath, tmpInnerLakePath, tmpTitle, tmpDisplayName) != E_OK) {
            MEDIA_ERR_LOG("can not move file to %{public}s", DfxUtils::GetSafePath(tmpInnerLakePath).c_str());
            return E_ERR;
        } else { // 重命名成功后, 需要先修改storage_path, 后续文件通知判断变更
            // 后续移动失败要回退storage_path
            UpdateLakeAssetInfo(refresh, mediaId, tmpInnerLakePath, tmpDisplayName, tmpTitle);
        }
    }
    MoveAssetsToLakeUpdateData updateData;
    updateData.mediaId = mediaId;
    updateData.title = tmpTitle;
    updateData.displayName = tmpDisplayName;
    updateData.storagePath = tmpInnerLakePath;
    int32_t ret = LakeFileOperations::MoveLakeFile(outerLakePath, tmpInnerLakePath);
    CHECK_AND_PRINT_LOG(ret == E_OK,
        "move asset %{public}s to %{public}s error, errno: %{public}d.",
        DfxUtils::GetSafePath(outerLakePath).c_str(),
        DfxUtils::GetSafePath(tmpInnerLakePath).c_str(),
        errno);
    if (ret == E_OK) {
        updateDatas.emplace_back(updateData);
        if (tmpInnerLakePath.compare(innerLakePath) != E_OK) {
            UpdateLakeAssetInfo(refresh, mediaId, tmpInnerLakePath, tmpDisplayName, tmpTitle);
        }
    }
    return E_OK;
}

std::vector<MoveAssetsToLakeUpdateData> LakeFileOperations::GetInnerLakeAssets(
    const std::vector<std::string> &ids)
{
    std::vector<MoveAssetsToLakeUpdateData> innerLakeAssets;
    MEDIA_INFO_LOG("GetInnerLakeAssetsIds START %{public}u", ids.size());
    CHECK_AND_RETURN_RET_LOG(!ids.empty(), innerLakeAssets, "move lake assets ids is empty");
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(MediaColumn::MEDIA_ID, ids);
    // 筛选湖内，即file_source_type为MEDIA_HO_LAKE的资源
    predicates.EqualTo(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE));
    auto resultSet = MediaLibraryRdbStore::Query(predicates,
        { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_TITLE, MediaColumn::MEDIA_NAME, PhotoColumn::PHOTO_STORAGE_PATH });
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, innerLakeAssets, "query lake assets error");
    int32_t rowCount = -1;
    int32_t ret = resultSet->GetRowCount(rowCount);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("move asset get row count error");
        resultSet->Close();
        return innerLakeAssets;
    }
    if (rowCount < 0) {
        MEDIA_ERR_LOG("move asset row count error");
        resultSet->Close();
        return innerLakeAssets;
    }
    if (rowCount == 0) {
        MEDIA_WARN_LOG("move asset row count is 0");
        resultSet->Close();
        return innerLakeAssets;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t mediaId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        std::string title = GetStringVal(MediaColumn::MEDIA_TITLE, resultSet);
        std::string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        std::string storagePath = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
        MoveAssetsToLakeUpdateData innerLakeAsset;
        innerLakeAsset.mediaId = mediaId;
        innerLakeAsset.title = title;
        innerLakeAsset.displayName = displayName;
        innerLakeAsset.storagePath = storagePath;
        MEDIA_INFO_LOG("dbinfo %{private}s, %{private}s", displayName.c_str(), storagePath.c_str());
        innerLakeAssets.emplace_back(innerLakeAsset);
    }
    resultSet->Close();
    MEDIA_INFO_LOG("GetInnerLakeAssetsIds END");
    return innerLakeAssets;
}

int32_t LakeFileOperations::MoveInnerLakeAssetsToNewAlbum(
    AccurateRefreshBase &refresh, const std::vector<std::string> &ids, int32_t targetAlbumId)
{
    CHECK_AND_RETURN_RET_LOG(!ids.empty() && targetAlbumId, E_INVALID_ARGUMENTS, "move asset param error");
    MEDIA_INFO_LOG("MoveInnerLakeAssetsToNewAlbum START");
    // 筛选湖内文件资产
    std::vector<MoveAssetsToLakeUpdateData> innerLakeAssets = GetInnerLakeAssets(ids);
    CHECK_AND_RETURN_RET_LOG(!innerLakeAssets.empty(), E_ERR, "inner lake assets ids is empty");
    std::string checkStoragePath;
    CHECK_AND_RETURN_RET_LOG(CreateParentDir(targetAlbumId, checkStoragePath) == E_OK, E_ERR,
        "create parent dir error");
    // 获取新的storage_path、title、displayname，移动文件到湖内新目录
    for (MoveAssetsToLakeUpdateData innerLakeAsset : innerLakeAssets) {
        std::string orgInnerLakePath = innerLakeAsset.storagePath;
        std::string orgDisplayName = innerLakeAsset.displayName;
        std::string orgTitle = innerLakeAsset.title;
        // 湖内文件的 storagePath 与 displayname可能不一致 （重命名函数对于两者不是同时修改）, 用displayname拼接一个新storagePath
        std::string originInnerLakePath = "";
        size_t lastSlashPos = innerLakeAsset.storagePath.rfind("/");
        if (lastSlashPos != string::npos) {
            originInnerLakePath = innerLakeAsset.storagePath.substr(0, lastSlashPos + 1) + innerLakeAsset.displayName;
        }
        std::string innerLakePath = checkStoragePath + innerLakeAsset.displayName;
        std::string tmpInnerLakePath = innerLakePath;
        std::string tmpTitle = innerLakeAsset.title;
        std::string tmpDisplayName = innerLakeAsset.displayName;
        // 如果目标lpath存在同名文件，重命名
        if (MediaFileUtils::IsFileExists(innerLakePath)) {
            if (HandleSameNameRename(
                innerLakeAsset.mediaId, innerLakePath, tmpInnerLakePath, tmpTitle, tmpDisplayName) != E_OK) {
                break;
            }
            innerLakePath = tmpInnerLakePath;
        }
        innerLakeAsset.displayName = tmpDisplayName;
        innerLakeAsset.storagePath = tmpInnerLakePath;
        innerLakeAsset.title = tmpTitle;
        int32_t ret =
            UpdateLakeAssetInfo(refresh, innerLakeAsset.mediaId, tmpInnerLakePath, tmpDisplayName, tmpTitle);
        CHECK_AND_CONTINUE_ERR_LOG(ret == E_OK, "update inner lake asset info error");
        ret = rename(originInnerLakePath.c_str(), innerLakeAsset.storagePath.c_str());
        MEDIA_INFO_LOG("%{public}s, %{public}s", LakeFileUtils::GarbleFilePath(originInnerLakePath).c_str(),
            LakeFileUtils::GarbleFilePath(innerLakeAsset.storagePath).c_str());
        if (ret < 0) {
            UpdateLakeAssetInfo(refresh, innerLakeAsset.mediaId, orgInnerLakePath, orgDisplayName, orgTitle);
            MEDIA_ERR_LOG("Failed to move in rename file, ret: %{public}d, errno: %{public}d", ret, errno);
        }
    }
    return E_OK;
}

int32_t LakeFileOperations::MoveAssetsToLake(AccurateRefreshBase &refresh, const std::vector<std::string> &ids)
{
    MEDIA_INFO_LOG("MoveAssetsToLake enter %{public}u", ids.size());
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
    auto resultSet = MediaLibraryRdbStore::Query(predicates,
        { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_TITLE, MediaColumn::MEDIA_NAME,
            MediaColumn::MEDIA_FILE_PATH, PhotoColumn::PHOTO_STORAGE_PATH });
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
    std::vector<MoveAssetsToLakeUpdateData> updateDatas;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        MoveAssetToLake(refresh, resultSet, updateDatas);
    }
    resultSet->Close();
    return UpdateAfterMoveAssetsToLake(refresh, updateDatas, FileSourceType::MEDIA_HO_LAKE);
}

static int32_t UpdateFileSourceType(const std::vector<std::string> &updateIds, const FileSourceType &type)
{
    CHECK_AND_RETURN_RET_LOG(!updateIds.empty(), E_INVALID_ARGUMENTS, "update file source type param error");
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(MediaColumn::MEDIA_ID, updateIds);
    ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, static_cast<int32_t>(type));
    int32_t changedRows = -1;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB, "update file source type rdbstore error");
    int32_t ret = rdbStore->Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK && changedRows > 0,
        E_ERR,
        "update file source type error ret: %{public}d, change rows: %{public}d",
        ret,
        changedRows);
    return E_OK;
}

int32_t LakeFileOperations::MoveAssetsFromLake(const std::vector<std::string> &ids)
{
    MEDIA_INFO_LOG("MoveAssetsFromLake enter %{public}u", ids.size());
    CHECK_AND_RETURN_RET_LOG(!ids.empty(), E_INVALID_ARGUMENTS, "move asset param error");
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(MediaColumn::MEDIA_ID, ids);
    // 只处理湖内且file_source_type为MEDIA_HO_LAKE，且position=1和position=3的类型资源
    predicates.EqualTo(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE));
    auto resultSet = MediaLibraryRdbStore::Query(predicates,
        { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH, PhotoColumn::PHOTO_STORAGE_PATH });
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RDB, "query storage path error");
    int32_t rowCount = -1;
    int32_t ret = resultSet->GetRowCount(rowCount);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("move asset get row count error");
        resultSet->Close();
        return E_ERR;
    }
    if (rowCount < 0) {
        MEDIA_ERR_LOG("move asset row count error");
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
        int32_t mediaId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        std::string innerLakePath = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
        std::string outerLakePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        if (outerLakePath.empty() || innerLakePath.empty()) {
            continue;
        }
        // 移动湖内文件到湖外
        ret = MoveLakeFile(innerLakePath, outerLakePath);
        CHECK_AND_PRINT_LOG(ret == E_OK, "move asset %{public}s to %{public}s error, errno: %{public}d",
            DfxUtils::GetSafePath(innerLakePath).c_str(), DfxUtils::GetSafePath(outerLakePath).c_str(), errno);
        if (ret == E_OK) {
            updateIds.emplace_back(to_string(mediaId));
        }
    }
    resultSet->Close();
    return UpdateFileSourceType(updateIds, FileSourceType::MEDIA);
}

static bool IsLakeModify(const std::string& editPath, int64_t editTime)
{
    int64_t realEditTime = 0;
    if (!MediaFileUtils::GetDateModified(editPath, realEditTime)) {
        return false;
    }

    // 1000ms 5000ms
    MEDIA_INFO_LOG("RealEditTime:%{public}lld, editTime:%{public}lld", realEditTime, editTime);
    const int MILLISECONDS_PER_SECOND = 1000;
    const int FIVE_SECONDS_IN_MILLISECONDS = 5000;
    if (realEditTime - editTime * MILLISECONDS_PER_SECOND >= FIVE_SECONDS_IN_MILLISECONDS) {
        MEDIA_INFO_LOG("Lake modify");
        return true;
    }
    return false;
}

static bool IsLakeFile(const std::string& fileUri)
{
    return fileUri.find("HO_DATA_EXT_MISC") != std::string::npos;
}

static int32_t GetEditPathAndEditTime(const std::string& fileUri, std::string& editPath, int64_t& editTime)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("RdbStore is nullptr");
        return -1;
    }

    std::string storagePath;
    std::string pathPreFix(IN_LAKE_MOUNT_OUTLAKE_PATH_PREFIEX);
    if (fileUri.size() > INLAKE_PATH_LOGIC_LEN &&
        fileUri.compare(0, INLAKE_PATH_LOGIC_LEN, IN_LAKE_PATH_LOGIC_PREFIX) == 0) {
        storagePath = pathPreFix + fileUri.substr(INLAKE_PATH_LOGIC_LEN);
    }

    std::string sql = "SELECT " +
        PhotoColumn::PHOTO_EDIT_TIME + ", " +
        PhotoColumn::MEDIA_FILE_PATH +
        " FROM " + PhotoColumn::PHOTOS_TABLE +
        " WHERE " + PhotoColumn::PHOTO_STORAGE_PATH + " = '" + storagePath + "'";
    auto resultSet = rdbStore->QuerySql(sql);
    if (!resultSet || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to get result set");
        return -1;
    }

    int64_t editTimeLocal;
    std::string cloudPath;
    resultSet->GetLong(0, editTimeLocal);
    resultSet->GetString(1, cloudPath);

    editTime = editTimeLocal;
    editPath = PhotoFileUtils::GetEditDataPath(cloudPath);
    if (!MediaFileUtils::IsFileExists(editPath)) {
        MEDIA_ERR_LOG("Path does not exist, editPath:%{public}s", DfxUtils::GetSafePath(editPath).c_str());
        return -1;
    }
    return E_OK;
}

int32_t LakeFileOperations::UpdateMediaAssetEditData(string& fileUri)
{
    if (fileUri.empty()) {
        MEDIA_ERR_LOG("FileUri is null");
        return E_ERR;
    }

    // 湖外文件不处理
    if (!IsLakeFile(fileUri)) {
        MEDIA_DEBUG_LOG("File is not in lake");
        return E_OK;
    }

    std::string editDataPath;
    int64_t editTime;
    if (GetEditPathAndEditTime(fileUri, editDataPath, editTime) != E_OK) {
        MEDIA_ERR_LOG("Failed to get edit path and time");
        return E_ERR;
    }

    // 湖外修改不处理
    if (!IsLakeModify(editDataPath, editTime)) {
        MEDIA_DEBUG_LOG("File not modified");
        return E_OK;
    }

    std::string editDataStr;
    if (!MediaFileUtils::ReadStrFromFile(editDataPath, editDataStr)) {
        MEDIA_ERR_LOG("Failed to read file");
        return E_ERR;
    }

    nlohmann::json editDataJson;
    if (editDataStr.empty() || !nlohmann::json::accept(editDataStr.c_str(), editDataStr.size())) {
        editDataJson[COMPATIBLE_FORMAT] = "public";
        editDataJson[FORMAT_VERSION] = "";
        editDataJson[EDIT_DATA] = "";
        editDataJson[APP_ID] = "";
        MEDIA_ERR_LOG("JSON format is invalid");
    } else {
        auto j = nlohmann::json::parse(editDataStr);
        editDataJson[COMPATIBLE_FORMAT] = "public";
        editDataJson[FORMAT_VERSION] = j.value(FORMAT_VERSION, "");
        editDataJson[EDIT_DATA] = j.value(EDIT_DATA, "");
        editDataJson[APP_ID] = j.value(APP_ID, "");
    }

    std::string editData = editDataJson.dump();
    if (!MediaFileUtils::WriteStrToFile(editDataPath, editData)) {
        MEDIA_ERR_LOG("Failed to write file");
        return E_ERR;
    }

    return E_OK;
}

int32_t LakeFileOperations::RenamePhoto(AccurateRefreshBase &refresh, const int32_t &fileId,
    const std::string &displayName, const std::string &storagePath, const std::string &data)
{
    MEDIA_INFO_LOG("RenamePhoto fileId: %{public}d, displayName: %{public}s, path: %{public}s",
        fileId,
        DfxUtils::GetSafePath(displayName).c_str(),
        DfxUtils::GetSafePath(storagePath).c_str());
    CHECK_AND_RETURN_RET_LOG(!displayName.empty() && !storagePath.empty(), E_ERR, "param error");
    size_t lastSlash = storagePath.rfind('/');
    size_t lastDot = displayName.rfind('.');
    if (lastDot == std::string::npos) {
        lastDot = displayName.length();
    }
    CHECK_AND_RETURN_RET_LOG(lastSlash != std::string::npos, E_ERR, "slash not found in storage path");
    std::string dir = storagePath.substr(0, lastSlash + 1);
    std::string newPath = dir + displayName;
    std::string newDisplayName = displayName;
    std::string newTitle = displayName.substr(0, lastDot);
    if (MediaFileUtils::IsFileExists(newPath) || IsAlbumHasSameNameAsset(fileId, displayName)) {
        // 存在同名文件，重命名
        if (HandleSameNameRename(fileId, newPath, newPath, newTitle, newDisplayName) != E_OK) {
            MEDIA_ERR_LOG("can not rename file to %{public}s", DfxUtils::GetSafePath(newPath).c_str());
            return E_ERR;
        }
    }
    MEDIA_INFO_LOG("RenamePhoto: %{public}s to: %{public}s",
        DfxUtils::GetSafePath(storagePath).c_str(),
        DfxUtils::GetSafePath(newPath).c_str());
    int32_t ret = LakeFileUtils::RenameFileCrossPolicy(storagePath, newPath);
    CHECK_AND_PRINT_LOG(ret == E_OK, "move file error %{public}d", errno);
    int errorPathNotExist = 2;
    if (ret == E_OK) {
        UpdateLakeAssetInfo(refresh, fileId, newPath, newDisplayName, newTitle);
    } else if (errno == errorPathNotExist) { // storagePath not exist: trashed, hidden, cloud
        UpdateOuterLakeAssetInfo(refresh, fileId, newPath, newDisplayName, newTitle);
    }
    return ret;
}
}  // namespace OHOS::Media
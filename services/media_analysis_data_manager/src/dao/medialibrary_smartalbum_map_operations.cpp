/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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
#define MLOG_TAG "SmartAlbum"

#include "medialibrary_smartalbum_map_operations.h"

#include "datashare_predicates.h"
#include "datashare_result_set.h"
#include "fetch_result.h"
#include "medialibrary_common_utils.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_dir_operations.h"
#include "medialibrary_errno.h"
#include "medialibrary_file_operations.h"
#include "medialibrary_notify.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_smartalbum_operations.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_smart_album_column.h"
#include "media_smart_map_column.h"
#include "rdb_utils.h"
#include "result_set_utils.h"

using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using namespace OHOS::RdbDataShareAdapter;

namespace OHOS {
namespace Media {
using ChangeType = AAFwk::ChangeInfo::ChangeType;
static const std::string HASH_COLLISION_SUFFIX = "(1)";
static const std::string ASSET_RECYCLE_SUFFIX = "-copy";
static const std::string DIR_RECYCLE_SUFFIX = "_recycle";
const std::string RECYCLE_DIR = ".recycle/";
constexpr int64_t ONEDAY_TO_SEC = 60 * 60 * 24;
constexpr int32_t HASH_COLLISION_MAX_TRY = 10;
std::atomic<bool> MediaLibrarySmartAlbumMapOperations::isInterrupt_ = false;
mutex MediaLibrarySmartAlbumMapOperations::g_opMutex;

static string MakeSuffixPathName(const string &assetPath)
{
    string outSuffixPath;
    size_t extensionIndex = assetPath.rfind(".");
    if (extensionIndex != string::npos) {
        string extension = assetPath.substr(extensionIndex);
        string noExtensionPath = assetPath.substr(0, extensionIndex);
        outSuffixPath = noExtensionPath + ASSET_RECYCLE_SUFFIX + extension;
    } else {
        outSuffixPath = assetPath + ASSET_RECYCLE_SUFFIX;
    }
    MEDIA_DEBUG_LOG("outSuffixPath = %{private}s", outSuffixPath.c_str());
    return outSuffixPath;
}

static shared_ptr<NativeRdb::ResultSet> QueryAgeingTrashFiles()
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, nullptr, "UniStore is nullptr");

    MediaLibraryCommand querySmartAlbumCmd(OperationObject::SMART_ALBUM, OperationType::QUERY);
    querySmartAlbumCmd.GetAbsRdbPredicates()->EqualTo(SMARTALBUM_DB_ID, to_string(TRASH_ALBUM_ID_VALUES));
    auto resultSet = uniStore->Query(querySmartAlbumCmd, { SMARTALBUM_DB_EXPIRED_TIME });
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, nullptr, "Failed to query rdb");

    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK, nullptr, "Failed to query rdb");
    auto recycleDays = GetInt32Val(SMARTALBUM_DB_EXPIRED_TIME, resultSet);
    resultSet.reset();

    int64_t dateAgeing = MediaFileUtils::UTCTimeMilliSeconds();
    string strAgeingQueryCondition = MEDIA_DATA_DB_DATE_TRASHED + "> 0" + " AND " + to_string(dateAgeing) + " - " +
        MEDIA_DATA_DB_DATE_TRASHED + " > " + to_string(recycleDays * ONEDAY_TO_SEC * MSEC_TO_SEC);
    MEDIA_INFO_LOG("StrAgeingQueryCondition = %{private}s", strAgeingQueryCondition.c_str());

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::QUERY);
    cmd.GetAbsRdbPredicates()->SetWhereClause(strAgeingQueryCondition);
    return MediaLibraryObjectUtils::QueryWithCondition(cmd, {});
}

int32_t MediaLibrarySmartAlbumMapOperations::HandleAgingOperation(shared_ptr<int> countPtr)
{
    auto resultSet = QueryAgeingTrashFiles();
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "Failed to query ageing trash files");

    int32_t count = 0;
    CHECK_AND_RETURN_RET_LOG(resultSet->GetRowCount(count) == NativeRdb::E_OK, E_HAS_DB_ERROR,
        "Get query result count failed");
    if (count == 0) {
        MEDIA_INFO_LOG("Have no ageing trash file");
        return E_SUCCESS;
    }

    auto fetchFileResult = make_shared<FetchResult<FileAsset>>();
    for (int32_t row = 0; row < count; row++) {
        if (MediaLibrarySmartAlbumMapOperations::GetInterrupt()) {
            MEDIA_INFO_LOG("recieve interrupt, stop aging");
            return E_ERR;
        }

        unique_ptr<FileAsset> fileAsset = fetchFileResult->GetObjectFromRdb(resultSet, row);
        CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_HAS_DB_ERROR, "Get fileAsset failed");
        int32_t errCode;
        if (fileAsset->GetIsTrash() == TRASHED_ASSET) {
            errCode = MediaLibraryObjectUtils::DeleteFileObj(move(fileAsset));
        } else if (fileAsset->GetIsTrash() == TRASHED_DIR) {
            errCode = MediaLibraryObjectUtils::DeleteDirObj(move(fileAsset));
        } else {
            MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::DELETE);
            errCode = MediaLibraryObjectUtils::DeleteInfoByIdInDb(cmd, to_string(fileAsset->GetId()));
        }
        CHECK_AND_RETURN_RET_LOG(errCode >= 0, errCode, "Failed to delete during trash aging: %{public}d", errCode);
    }
    if (countPtr != nullptr) {
        *countPtr = count;
    }
    return E_SUCCESS;
}

static string GetAssetRecycle(const int32_t assetId, string &filePath, string &outTrashDirPath)
{
    auto dirQuerySetMap = MediaLibraryDataManager::GetDirQuerySetMap();
    for (pair<string, DirAsset> dirPair : dirQuerySetMap) {
        DirAsset dirAsset = dirPair.second;
        string rootPath = ROOT_MEDIA_DIR + dirAsset.GetDirectory();
        if (filePath.find(rootPath) == 0) {
            string trashDirPath = rootPath + RECYCLE_DIR;
            return trashDirPath;
        }
    }
    return "";
}

static int32_t MakeRecycleDisplayName(const int32_t assetId, const string &trashDirPath, string &outRecyclePath)
{
    shared_ptr<FileAsset> fileAsset = MediaLibraryObjectUtils::GetFileAssetFromId(to_string(assetId));
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("Failed to get asset, assetId = %{public}d", assetId);
        return E_GET_ASSET_FAIL;
    }
    string hashDisplayName;
    string name = to_string(fileAsset->GetId()) + fileAsset->GetRelativePath() + fileAsset->GetDisplayName();
    int32_t errorCode = MediaLibraryCommonUtils::GenKeySHA256(name, hashDisplayName);
    if (errorCode != E_OK) {
        MEDIA_ERR_LOG("Failed to genKey, err: %{public}d", errorCode);
        return E_MAKE_HASHNAME_FAIL;
    }
    string extension;
    outRecyclePath = trashDirPath + hashDisplayName;
    if (fileAsset->GetMediaType() != MEDIA_TYPE_ALBUM) {
        size_t extensionIndex = fileAsset->GetDisplayName().rfind(".");
        if (extensionIndex != string::npos) {
            extension = fileAsset->GetDisplayName().substr(extensionIndex);
        }
        outRecyclePath += extension;
        MEDIA_INFO_LOG("Asset outRecyclePath = %{private}s", outRecyclePath.c_str());
    }
    int32_t suffixTime = 0;
    while (MediaLibraryObjectUtils::IsColumnValueExist(outRecyclePath, MEDIA_DATA_DB_RECYCLE_PATH)) {
        if (suffixTime > HASH_COLLISION_MAX_TRY) {
            MEDIA_ERR_LOG("Reach max hash collision times");
            errorCode = E_MAKE_HASHNAME_FAIL;
            break;
        }
        name += HASH_COLLISION_SUFFIX;
        errorCode = MediaLibraryCommonUtils::GenKeySHA256(name, hashDisplayName);
        if (errorCode != E_OK) {
            MEDIA_ERR_LOG("Failed to genKey, err: %{public}d", errorCode);
            return E_MAKE_HASHNAME_FAIL;
        }
        outRecyclePath = trashDirPath + hashDisplayName;
        if (!extension.empty()) {
            outRecyclePath += extension;
        }
        suffixTime++;
    }
    return errorCode;
}

static int32_t UpdateChildRecycleInfoInDb(const int32_t assetId)
{
    ValuesBucket values;
    values.PutLong(MEDIA_DATA_DB_DATE_TRASHED, 0);
    values.PutInt(MEDIA_DATA_DB_IS_TRASH, 0);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::UPDATE, values);
    return MediaLibraryObjectUtils::ModifyInfoByIdInDb(cmd, to_string(assetId));
}

static int32_t RecycleChildAssetsInfoUtil(const int32_t parentId)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_HAS_DB_ERROR, "UniStore is nullptr");

    MediaLibraryCommand queryCmd(OperationObject::FILESYSTEM_ASSET, OperationType::QUERY);
    queryCmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_PARENT_ID, to_string(parentId))->And()->
        EqualTo(MEDIA_DATA_DB_IS_TRASH, to_string(TRASHED_DIR_CHILD));
    auto resultSet = uniStore->Query(queryCmd, { MEDIA_DATA_DB_ID, MEDIA_DATA_DB_MEDIA_TYPE });
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "QueryResultSet is nullptr");

    int32_t errCode = E_SUCCESS;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = GetInt32Val(MEDIA_DATA_DB_ID, resultSet);
        errCode = UpdateChildRecycleInfoInDb(fileId);
        CHECK_AND_RETURN_RET_LOG(errCode > 0, errCode, "Failed to UpdateChildRecycleInfoInDb");

        if (GetInt32Val(MEDIA_DATA_DB_MEDIA_TYPE, resultSet) == MEDIA_TYPE_ALBUM) {
            errCode = RecycleChildAssetsInfoUtil(fileId);
            CHECK_AND_RETURN_RET_LOG(errCode >= 0, errCode, "Failed to RecycleChildAssetsInfoUtil");
        }
    }
    return errCode;
}

static string GetNewPath(const string &path, const string &srcRelPath, const string &newRelPath)
{
    if ((path.find(srcRelPath) != 0) && (path.find(ROOT_MEDIA_DIR + srcRelPath) != 0)) {
        return "";
    }
    string newPath = newRelPath;
    if (path.find(ROOT_MEDIA_DIR) != string::npos) {
        newPath = ROOT_MEDIA_DIR + newPath;
    }
    newPath += path.substr(path.find(srcRelPath) + srcRelPath.length());
    return newPath;
}

static int32_t RecycleChildSameNameInfoUtil(const int32_t parentId, const string &srcRelPath, const string &newRelPath,
    bool isFirstLevel)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_HAS_DB_ERROR, "UniStore is nullptr");

    MediaLibraryCommand queryCmd(OperationObject::FILESYSTEM_ASSET, OperationType::QUERY);
    queryCmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_PARENT_ID, to_string(parentId));
    vector<string> columns = { MEDIA_DATA_DB_ID, MEDIA_DATA_DB_FILE_PATH, MEDIA_DATA_DB_MEDIA_TYPE,
        MEDIA_DATA_DB_IS_TRASH, MEDIA_DATA_DB_RECYCLE_PATH, MEDIA_DATA_DB_RELATIVE_PATH };
    auto result = uniStore->Query(queryCmd, columns);
    CHECK_AND_RETURN_RET_LOG(result != nullptr, E_HAS_DB_ERROR, "Failed to obtain value from database");

    int32_t count = -1;
    CHECK_AND_RETURN_RET(result->GetRowCount(count) == NativeRdb::E_OK, E_HAS_DB_ERROR);
    for (int32_t i = 0; i < count; i++) {
        CHECK_AND_RETURN_RET(result->GoToNextRow() == NativeRdb::E_OK, E_HAS_DB_ERROR);
        ValuesBucket values;
        string relativePath = GetStringVal(MEDIA_DATA_DB_RELATIVE_PATH, result);
        values.PutString(MEDIA_DATA_DB_RELATIVE_PATH, GetNewPath(relativePath, srcRelPath, newRelPath));

        int32_t mediaType = GetInt32Val(MEDIA_DATA_DB_MEDIA_TYPE, result);
        if (isFirstLevel && (mediaType != MEDIA_TYPE_ALBUM)) {
            string newParentName = newRelPath;
            // delete the final '/' in relative_path
            newParentName.pop_back();
            newParentName = MediaLibraryDataManagerUtils::GetDisPlayNameFromPath(newParentName);
            values.PutString(MEDIA_DATA_DB_BUCKET_NAME, newParentName);
        }

        // Update recycle_path for TRASHED_ASSET and TRASHED_DIR, update data for TRASHED_DIR_CHILD.
        int32_t isTrash = GetInt32Val(MEDIA_DATA_DB_IS_TRASH, result);
        if (isTrash == TRASHED_DIR_CHILD) {
            string path = GetStringVal(MEDIA_DATA_DB_FILE_PATH, result);
            values.PutString(MEDIA_DATA_DB_FILE_PATH, GetNewPath(path, srcRelPath, newRelPath));
        } else {
            string recyclePath = GetStringVal(MEDIA_DATA_DB_RECYCLE_PATH, result);
            values.PutString(MEDIA_DATA_DB_RECYCLE_PATH, GetNewPath(recyclePath, srcRelPath, newRelPath));
        }

        int32_t fileId = GetInt32Val(MEDIA_DATA_DB_ID, result);
        MediaLibraryCommand updateCmd(OperationObject::FILESYSTEM_ASSET, OperationType::UPDATE, values);
        updateCmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_ID, to_string(fileId));
        int32_t changedRows = -1;
        auto ret = uniStore->Update(updateCmd, changedRows);
        if ((ret != NativeRdb::E_OK) || (changedRows <= 0)) {
            MEDIA_ERR_LOG("Update DB failed. Ret: %{public}d. Update fileId: %{public}d", ret, fileId);
            return E_HAS_DB_ERROR;
        }

        if (mediaType == MEDIA_TYPE_ALBUM) {
            ret = RecycleChildSameNameInfoUtil(fileId, srcRelPath, newRelPath, false);
            CHECK_AND_RETURN_RET_LOG(ret == E_SUCCESS, E_HAS_DB_ERROR, "Update DB failed. fileId: %{public}d", fileId);
        }
    }
    return E_SUCCESS;
}

static int32_t RecycleDir(const shared_ptr<FileAsset> &fileAsset)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_HAS_DB_ERROR, "UniStore is nullptr");

    ValuesBucket values;
    int32_t parentId = MediaLibraryObjectUtils::CreateDirWithPath(ROOT_MEDIA_DIR +
        MediaFileUtils::AddDocsToRelativePath(fileAsset->GetRelativePath()));
    if (parentId != fileAsset->GetParent()) {
        values.PutInt(MEDIA_DATA_DB_PARENT_ID, parentId);
    }

    bool hasSameName = false;
    string assetPath = fileAsset->GetRecyclePath();
    while (MediaLibraryObjectUtils::IsFileExistInDb(assetPath)) {
        hasSameName = true;
        assetPath = assetPath + DIR_RECYCLE_SUFFIX;
    }

    if (!MediaFileUtils::RenameDir(fileAsset->GetPath(), assetPath)) {
        if (errno == ENAMETOOLONG) {
            return -ENAMETOOLONG;
        }
        return E_RENAME_DIR_FAIL;
    }

    if (hasSameName) {
        fileAsset->SetRecyclePath(assetPath);
        // update dir info
        string newName = MediaLibraryDataManagerUtils::GetDisPlayNameFromPath(assetPath);
        values.PutString(MEDIA_DATA_DB_NAME, newName);
        values.PutString(MEDIA_DATA_DB_TITLE, newName);
        values.PutString(MEDIA_DATA_DB_RECYCLE_PATH, assetPath);

        // update child info
        string srcRelPath = fileAsset->GetRelativePath() + fileAsset->GetDisplayName() + "/";
        string newRelPath = fileAsset->GetRelativePath() + newName + "/";
        RecycleChildSameNameInfoUtil(fileAsset->GetId(), srcRelPath, newRelPath, true);
    }

    if (!values.IsEmpty()) {
        MediaLibraryCommand updateCmd(OperationObject::FILESYSTEM_ASSET, OperationType::UPDATE, values);
        updateCmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_ID, to_string(fileAsset->GetId()));
        int32_t changedRows = -1;
        return uniStore->Update(updateCmd, changedRows);
    }

    return E_SUCCESS;
}

static int32_t UpdateRecycleInfoInDb(const int32_t assetId, const string &realPath)
{
    ValuesBucket values;
    values.PutString(MEDIA_DATA_DB_FILE_PATH, realPath);
    values.PutString(MEDIA_DATA_DB_RECYCLE_PATH, "");
    values.PutLong(MEDIA_DATA_DB_DATE_TRASHED, 0);
    values.PutInt(MEDIA_DATA_DB_IS_TRASH, 0);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::UPDATE, values);
    return MediaLibraryObjectUtils::ModifyInfoByIdInDb(cmd, to_string(assetId));
}

static int32_t RecycleDirAssetsInfoUtil(const shared_ptr<FileAsset> &fileAsset)
{
    if (!MediaFileUtils::IsDirectory(fileAsset->GetPath())) {
        MEDIA_ERR_LOG("recycle dir is not exists");
        return E_RECYCLE_FILE_IS_NULL;
    }

    auto errorCode = RecycleDir(fileAsset);
    CHECK_AND_RETURN_RET_LOG(errorCode == E_SUCCESS, errorCode, "Failed to RecycleDir");

    errorCode = RecycleChildAssetsInfoUtil(fileAsset->GetId());
    CHECK_AND_RETURN_RET_LOG(errorCode >= 0, errorCode, "Failed to recycleChildAssetsInfoUtil");

    string dirRealPath = fileAsset->GetRecyclePath();
    return UpdateRecycleInfoInDb(fileAsset->GetId(), dirRealPath);
}

static int32_t RecycleFile(const shared_ptr<FileAsset> &fileAsset)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_HAS_DB_ERROR, "UniStore is nullptr");

    ValuesBucket values;
    int32_t parentId = MediaLibraryObjectUtils::CreateDirWithPath(ROOT_MEDIA_DIR +
        MediaFileUtils::AddDocsToRelativePath(fileAsset->GetRelativePath()));
    if (parentId != fileAsset->GetParent()) {
        values.PutInt(MEDIA_DATA_DB_PARENT_ID, parentId);
        values.PutInt(MEDIA_DATA_DB_BUCKET_ID, parentId);
    }

    bool hasSameName = false;
    string assetPath = fileAsset->GetRecyclePath();
    while (MediaLibraryObjectUtils::IsFileExistInDb(assetPath)) {
        hasSameName = true;
        assetPath = MakeSuffixPathName(assetPath);
    }

    if (!MediaFileUtils::MoveFile(fileAsset->GetPath(), assetPath)) {
        if (errno == ENAMETOOLONG) {
            return -ENAMETOOLONG;
        }
        return E_RENAME_FILE_FAIL;
    }
    MediaLibraryObjectUtils::InvalidateThumbnail(to_string(fileAsset->GetId()));

    if (hasSameName) {
        fileAsset->SetRecyclePath(assetPath);
        string newName = MediaLibraryDataManagerUtils::GetDisPlayNameFromPath(assetPath);
        values.PutString(MEDIA_DATA_DB_NAME, newName);
        values.PutString(MEDIA_DATA_DB_RECYCLE_PATH, assetPath);
        MediaType mediaType = fileAsset->GetMediaType();
        if ((mediaType != MEDIA_TYPE_VIDEO) && (mediaType != MEDIA_TYPE_AUDIO)) {
            values.PutString(MEDIA_DATA_DB_TITLE, newName);
        }
    }

    if (!values.IsEmpty()) {
        MediaLibraryCommand updateCmd(OperationObject::FILESYSTEM_ASSET, OperationType::UPDATE, values);
        updateCmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_ID, to_string(fileAsset->GetId()));
        int32_t changedRows = -1;
        return uniStore->Update(updateCmd, changedRows);
    }

    return E_SUCCESS;
}

static int32_t RecycleFileAssetsInfoUtil(const shared_ptr<FileAsset> &fileAsset)
{
    string recyclePath = fileAsset->GetPath();
    if (!MediaFileUtils::IsFileExists(recyclePath)) {
        MEDIA_ERR_LOG("RecycleFile is null, fileid = %{private}d", fileAsset->GetId());
        return E_RECYCLE_FILE_IS_NULL;
    }

    int32_t errorCode = RecycleFile(fileAsset);
    CHECK_AND_RETURN_RET_LOG(errorCode >= 0, errorCode, "Failed to recycleFile");

    string fileRealPath = fileAsset->GetRecyclePath();
    return UpdateRecycleInfoInDb(fileAsset->GetId(), fileRealPath);
}

static int32_t RemoveTrashAssetsInfoUtil(const int32_t fileAssetId)
{
    shared_ptr<FileAsset> fileAsset = MediaLibraryObjectUtils::GetFileAssetFromId(to_string(fileAssetId));
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_GET_ASSET_FAIL, "fileAsset is nullptr, assetId: %{public}d",
        fileAssetId);

    int32_t ret = 0;
    if (fileAsset->GetMediaType() != MEDIA_TYPE_ALBUM) {
        ret = RecycleFileAssetsInfoUtil(fileAsset);
    } else {
        ret = RecycleDirAssetsInfoUtil(fileAsset);
    }
    return ret;
}

static int32_t UpdateFavoriteAssetsInfoUtil(const int32_t fileAssetId, const bool isFavorites)
{
    ValuesBucket values;
    values.PutInt(MEDIA_DATA_DB_IS_FAV, isFavorites ? 1 : 0);
    MEDIA_INFO_LOG("Update asset %{public}d favorite to %{public}d", fileAssetId, isFavorites ? 1 : 0);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::UPDATE, values);
    return MediaLibraryObjectUtils::ModifyInfoByIdInDb(cmd, to_string(fileAssetId));
}

int32_t MediaLibrarySmartAlbumMapOperations::HandleRemoveAssetOperation(const int32_t albumId,
    const int32_t childFileAssetId, MediaLibraryCommand &cmd)
{
    lock_guard<mutex> guard(g_opMutex);
    int32_t errorCode = E_SUCCESS;
    if (albumId == TRASH_ALBUM_ID_VALUES) {
        errorCode = RemoveTrashAssetsInfoUtil(childFileAssetId);
    } else if (albumId == FAVOURITE_ALBUM_ID_VALUES) {
        errorCode = UpdateFavoriteAssetsInfoUtil(childFileAssetId, false);
    }
    CHECK_AND_RETURN_RET_LOG(errorCode >= 0, errorCode, "Failed to handleRemoveAssetOperations");
    cmd.GetAbsRdbPredicates()->EqualTo(SMARTALBUMMAP_DB_ALBUM_ID, to_string(albumId))->And()->
        EqualTo(SMARTALBUMMAP_DB_CHILD_ASSET_ID, to_string(childFileAssetId));
    return MediaLibraryObjectUtils::DeleteInfoByIdInDb(cmd);
}

static int32_t UpdateChildTrashInfoInDb(const int32_t assetId, const int64_t trashDate)
{
    ValuesBucket values;
    values.PutLong(MEDIA_DATA_DB_DATE_TRASHED, trashDate);
    values.PutInt(MEDIA_DATA_DB_IS_TRASH, TRASHED_DIR_CHILD);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::UPDATE, values);
    return MediaLibraryObjectUtils::ModifyInfoByIdInDb(cmd, to_string(assetId));
}

static int32_t TrashChildAssetsInfoUtil(const int32_t parentId, const int64_t &trashDate)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_HAS_DB_ERROR, "UniStore is nullptr");

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::QUERY);
    string parentPath = MediaLibraryObjectUtils::GetPathByIdFromDb(to_string(parentId));
    cmd.GetAbsRdbPredicates()->Like(MEDIA_DATA_DB_FILE_PATH, parentPath + "/%")->And()->
        EqualTo(MEDIA_DATA_DB_IS_TRASH, to_string(NOT_TRASHED));

    auto result = uniStore->Query(cmd, { MEDIA_DATA_DB_ID });
    CHECK_AND_RETURN_RET_LOG(result != nullptr, E_HAS_DB_ERROR, "QueryResultSet is nullptr");

    auto count = 0;
    CHECK_AND_RETURN_RET_LOG(result->GetRowCount(count) == NativeRdb::E_OK, E_HAS_DB_ERROR,
        "Get query result count failed");
    if (count == 0) {
        MEDIA_INFO_LOG("Have no child assets");
        return E_SUCCESS;
    }

    while (result->GoToNextRow() == NativeRdb::E_OK) {
        auto ret = UpdateChildTrashInfoInDb(GetInt32Val(MEDIA_DATA_DB_ID, result), trashDate);
        CHECK_AND_RETURN_RET_LOG(ret >= 0, ret, "Failed to updateChildTrashInfoInDb");
    }
    return E_SUCCESS;
}

static int32_t UpdateTrashInfoInDb(const int32_t assetId, const int64_t trashDate, string &recyclePath,
    const string &oldPath, const uint32_t trashType)
{
    ValuesBucket values;
    values.PutString(MEDIA_DATA_DB_FILE_PATH, recyclePath);
    values.PutString(MEDIA_DATA_DB_RECYCLE_PATH, oldPath);
    values.PutLong(MEDIA_DATA_DB_DATE_TRASHED, trashDate);
    values.PutInt(MEDIA_DATA_DB_IS_TRASH, trashType);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::UPDATE, values);
    return MediaLibraryObjectUtils::ModifyInfoByIdInDb(cmd, to_string(assetId));
}

static int32_t TrashDirAssetsInfoUtil(const int32_t assetId)
{
    string oldPath = MediaLibraryObjectUtils::GetPathByIdFromDb(to_string(assetId));
    string trashDirPath = GetAssetRecycle(assetId, oldPath, trashDirPath);
    CHECK_AND_RETURN_RET_LOG(!trashDirPath.empty(), E_CHECK_ROOT_DIR_FAIL, "Failed to get recycle dir");
    if (!MediaFileUtils::IsDirectory(trashDirPath)) {
        if (!MediaFileUtils::CreateDirectory(trashDirPath)) {
            MEDIA_ERR_LOG("TrashDirPath create failed");
            return E_CREATE_TRASHDIR_FAIL;
        }
    }
    string recyclePath;
    auto errorCode = MakeRecycleDisplayName(assetId, trashDirPath, recyclePath);
    CHECK_AND_RETURN_RET_LOG(errorCode == E_SUCCESS, errorCode, "Failed to makeRecycleDisplayName");
    if (!MediaFileUtils::RenameDir(oldPath, recyclePath)) {
        return E_MODIFY_DATA_FAIL;
    }
    int64_t trashDate = MediaFileUtils::UTCTimeMilliSeconds();
    errorCode = TrashChildAssetsInfoUtil(assetId, trashDate);
    CHECK_AND_RETURN_RET_LOG(errorCode == E_SUCCESS, errorCode, "Failed to trashChildAssetsInfoUtil");
    return UpdateTrashInfoInDb(assetId, trashDate, recyclePath, oldPath, TRASHED_DIR);
}

static int32_t TrashFileAssetsInfoUtil(const int32_t assetId)
{
    string oldPath = MediaLibraryObjectUtils::GetPathByIdFromDb(to_string(assetId));
    string trashDirPath = GetAssetRecycle(assetId, oldPath, trashDirPath);
    CHECK_AND_RETURN_RET_LOG(!trashDirPath.empty(), E_CHECK_ROOT_DIR_FAIL, "Failed to get recycle dir");
    if (!MediaFileUtils::IsDirectory(trashDirPath)) {
        if (!MediaFileUtils::CreateDirectory(trashDirPath)) {
            MEDIA_ERR_LOG("TrashDirPath create failed");
            return E_CREATE_TRASHDIR_FAIL;
        }
    }
    string recyclePath;
    auto errorCode = MakeRecycleDisplayName(assetId, trashDirPath, recyclePath);
    CHECK_AND_RETURN_RET_LOG(errorCode == E_SUCCESS, errorCode, "Failed to make recycle display name");
    errorCode = MediaFileUtils::ModifyAsset(oldPath, recyclePath);
    CHECK_AND_RETURN_RET_LOG(errorCode == E_SUCCESS, errorCode, "Failed to modifyAsset");
    MediaLibraryObjectUtils::InvalidateThumbnail(to_string(assetId));
    int64_t trashDate = MediaFileUtils::UTCTimeSeconds();
    return UpdateTrashInfoInDb(assetId, trashDate, recyclePath, oldPath, TRASHED_ASSET);
}

static int32_t InsertTrashAssetsInfoUtil(const int32_t fileAssetId)
{
    shared_ptr<FileAsset> fileAsset = MediaLibraryObjectUtils::GetFileAssetFromId(to_string(fileAssetId));
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("FileAsset is nullptr");
        return E_GET_ASSET_FAIL;
    }
    if (fileAsset->GetDateTrashed() != 0) {
        return E_IS_RECYCLED;
    }
    if (fileAsset->GetMediaType() != MEDIA_TYPE_ALBUM) {
        return TrashFileAssetsInfoUtil(fileAssetId);
    } else {
        return TrashDirAssetsInfoUtil(fileAssetId);
    }
}

int32_t MediaLibrarySmartAlbumMapOperations::HandleAddAssetOperation(const int32_t albumId,
    const int32_t childFileAssetId, const int32_t childAlbumId, MediaLibraryCommand &cmd)
{
    MEDIA_DEBUG_LOG("HandleAddAssetOperations albumId = %{public}d, childFileAssetId = %{public}d",
        albumId, childFileAssetId);
    lock_guard<mutex> guard(g_opMutex);
    int32_t errorCode = E_SUCCESS;
    if (albumId == TRASH_ALBUM_ID_VALUES) {
        errorCode = InsertTrashAssetsInfoUtil(childFileAssetId);
    } else if (albumId == FAVOURITE_ALBUM_ID_VALUES) {
        errorCode = UpdateFavoriteAssetsInfoUtil(childFileAssetId, true);
    }
    CHECK_AND_RETURN_RET_LOG(errorCode >= 0, errorCode, "Failed to handleAddAssetOperations");
    if (childAlbumId != DEFAULT_ALBUMID) {
        if (!MediaLibraryObjectUtils::IsParentSmartAlbum(albumId, true)) {
            MEDIA_ERR_LOG("Child can not add smart album");
            return E_CHILD_CAN_NOT_ADD_SMARTALBUM;
        }
    }
    if (childFileAssetId != DEFAULT_ASSETID) {
        if (MediaLibraryObjectUtils::IsParentSmartAlbum(albumId)) {
            MEDIA_ERR_LOG("Parent can not add assets");
            return E_PARENT_CAN_NOT_ADDASSETS;
        }
    }
    return MediaLibraryObjectUtils::InsertInDb(cmd);
}

int32_t MediaLibrarySmartAlbumMapOperations::HandleSmartAlbumMapOperation(MediaLibraryCommand &cmd)
{
    ValueObject valueObject;
    ValuesBucket values = cmd.GetValueBucket();
    int32_t albumId = DEFAULT_ALBUMID;
    if (values.GetObject(SMARTALBUMMAP_DB_ALBUM_ID, valueObject)) {
        valueObject.GetInt(albumId);
    }
    CHECK_AND_RETURN_RET_LOG(MediaLibraryObjectUtils::IsSmartAlbumExistInDb(albumId),
        E_SMARTALBUM_IS_NOT_EXISTED, "Failed to get smartAlbumId");
    int32_t childAssetId = DEFAULT_ASSETID;
    int32_t childAlbumId = DEFAULT_ALBUMID;
    if (values.GetObject(SMARTALBUMMAP_DB_CHILD_ASSET_ID, valueObject)) {
        valueObject.GetInt(childAssetId);
    }
    if (values.GetObject(SMARTALBUMMAP_DB_CHILD_ALBUM_ID, valueObject)) {
        valueObject.GetInt(childAlbumId);
    }
    // childAssetId and childAlbumId only one has value
    if ((childAlbumId == DEFAULT_ALBUMID) && (childAssetId == DEFAULT_ASSETID)) {
        MEDIA_ERR_LOG("GetObject failed");
        return E_GET_VALUEBUCKET_FAIL;
    }
    if (childAlbumId != DEFAULT_ALBUMID) {
        CHECK_AND_RETURN_RET_LOG(MediaLibraryObjectUtils::IsSmartAlbumExistInDb(childAlbumId),
            E_SMARTALBUM_IS_NOT_EXISTED, "Failed to get childAlbumId");
    }
    if (childAssetId != DEFAULT_ASSETID) {
        CHECK_AND_RETURN_RET_LOG(MediaLibraryObjectUtils::IsAssetExistInDb(childAssetId,
            true), E_GET_ASSET_FAIL, "Failed to get childAssetId");
    }
    MEDIA_INFO_LOG("childAssetId = %{public}d , childAlbumId = %{public}d", childAssetId, childAlbumId);
    int32_t errCode = E_ERR;
    switch (cmd.GetOprnType()) {
        case OperationType::CREATE:
            errCode = HandleAddAssetOperation(albumId, childAssetId, childAlbumId, cmd);
            break;
        case OperationType::DELETE:
            errCode = HandleRemoveAssetOperation(albumId, childAssetId, cmd);
            break;
        case OperationType::AGING:
            errCode = HandleAgingOperation();
            break;
        default:
            MEDIA_WARN_LOG("Unknown operation type %{private}d", cmd.GetOprnType());
            break;
        }
    return errCode;
}

void MediaLibrarySmartAlbumMapOperations::SetInterrupt(bool interrupt)
{
    isInterrupt_ = interrupt;
}

bool MediaLibrarySmartAlbumMapOperations::GetInterrupt()
{
    return isInterrupt_.load();
}
} // namespace Media
} // namespace OHOS

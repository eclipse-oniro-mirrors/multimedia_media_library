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
#define MLOG_TAG "ObjectUtils"

#include "medialibrary_object_utils.h"
#include <sys/sendfile.h>
#include "dfx_manager.h"
#include "directory_ex.h"
#include "media_file_utils.h"
#include "media_container_types.h"
#include "media_directory_type_column.h"
#include "media_scanner_manager.h"
#include "media_smart_album_column.h"
#include "media_smart_map_column.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_db_const.h"
#include "medialibrary_inotify.h"
#include "media_privacy_manager.h"
#include "mimetype_utils.h"
#include "parameter.h"
#include "permission_utils.h"
#include "photo_file_utils.h"
#include "result_set_utils.h"
#include "sandbox_helper.h"
#include "medialibrary_tracer.h"
#include "picture_handle_service.h"
#include "post_event_utils.h"
#include "userfilemgr_uri.h"
#include "dfx_utils.h"
#include "medialibrary_transcode_data_aging_operation.h"
#include "media_audio_column.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
static const string ASSET_RECYCLE_SUFFIX = "-copy";
static const string NO_MEDIA_TAG = ".nomedia";
const char* HAS_DATA = "persist.multimedia.media_analysis_service.hasdata";
constexpr int32_t OFFSET = 5;
constexpr int32_t ZERO_ASCII = '0';
int32_t MediaLibraryObjectUtils::CreateDirWithPath(const string &dirPath)
{
    if (dirPath.empty()) {
        return E_INVALID_PATH;
    }

    int64_t rowId = -1;
    ValuesBucket values;
    values.PutString(MEDIA_DATA_DB_FILE_PATH, dirPath);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ALBUM, OperationType::CREATE, values);
    int32_t ret = CreateDirObj(cmd, rowId);
    if (ret == E_FILE_EXIST || ret == E_SUCCESS) {
        return rowId;
    }
    return ret;
}

NativeAlbumAsset MediaLibraryObjectUtils::GetDirAsset(const string &path)
{
    NativeAlbumAsset dirAsset;
    if (path.empty()) {
        MEDIA_ERR_LOG("Path is empty, create failed!");
        dirAsset.SetAlbumId(E_INVALID_PATH);
        return dirAsset;
    }

    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr!");
        dirAsset.SetAlbumId(E_HAS_DB_ERROR);
        return dirAsset;
    }

    int32_t dirId = CreateDirWithPath(path);
    MEDIA_DEBUG_LOG("After CreateDirWithPath, get dirId = %{private}d!", dirId);
    dirAsset.SetAlbumId(dirId);
    if (dirId < 0) {
        return dirAsset;
    }

    string nameVal = GetStringColumnByIdFromDb(to_string(dirId), MEDIA_DATA_DB_NAME);
    if (nameVal.empty()) {
        MEDIA_ERR_LOG("Get dir name failed!");
        return dirAsset;
    }
    dirAsset.SetAlbumName(nameVal);
    MEDIA_DEBUG_LOG("bucketId = %{private}d bucketName = %{private}s", dirId, nameVal.c_str());
    return dirAsset;
}

int32_t MediaLibraryObjectUtils::DeleteInvalidRowInDb(const string &path)
{
    if (GetIdByPathFromDb(path) < 0) {
        // path is not in database, no need to delete
        return E_SUCCESS;
    }

    if (MediaFileUtils::IsFileExists(path)) {
        // File aready exist, not need to create again
        return E_SUCCESS;
    }

    MediaLibraryCommand deleteCmd(OperationObject::FILESYSTEM_ASSET, OperationType::DELETE);
    if (DeleteInfoByPathInDb(deleteCmd, path) != E_SUCCESS) {
        // Delete the record in database if file is not in filesystem any more
        MEDIA_ERR_LOG("CreateFileAsset: delete info in db failed");
        return E_DELETE_DIR_FAIL;
    }
    return E_SUCCESS;
}

int32_t MediaLibraryObjectUtils::InsertFileInDb(MediaLibraryCommand &cmd,
    const FileAsset &fileAsset, const NativeAlbumAsset &dirAsset)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr!");
        return E_HAS_DB_ERROR;
    }

    // Fill basic file information into DB
    string displayName = fileAsset.GetDisplayName();
    ValuesBucket assetInfo;
    assetInfo.PutInt(MEDIA_DATA_DB_MEDIA_TYPE, fileAsset.GetMediaType());
    assetInfo.PutString(MEDIA_DATA_DB_URI, MediaFileUtils::GetMediaTypeUri(fileAsset.GetMediaType()));
    string extension = ScannerUtils::GetFileExtension(displayName);
    assetInfo.PutString(MEDIA_DATA_DB_MIME_TYPE, MimeTypeUtils::GetMimeTypeFromExtension(extension));
    assetInfo.PutString(MEDIA_DATA_DB_RELATIVE_PATH, fileAsset.GetRelativePath());
    assetInfo.PutString(MEDIA_DATA_DB_NAME, displayName);
    assetInfo.PutString(MEDIA_DATA_DB_TITLE, MediaFileUtils::GetTitleFromDisplayName(displayName));
    struct stat statInfo {};
    if (stat(fileAsset.GetPath().c_str(), &statInfo) == 0) {
        assetInfo.PutLong(MEDIA_DATA_DB_SIZE, statInfo.st_size);
        assetInfo.PutLong(MEDIA_DATA_DB_DATE_ADDED, MediaFileUtils::UTCTimeMilliSeconds());
        assetInfo.PutLong(MEDIA_DATA_DB_DATE_MODIFIED, MediaFileUtils::Timespec2Millisecond(statInfo.st_mtim));
    }
    assetInfo.PutString(MEDIA_DATA_DB_FILE_PATH, fileAsset.GetPath());
    assetInfo.PutInt(MEDIA_DATA_DB_BUCKET_ID, dirAsset.GetAlbumId());
    assetInfo.PutInt(MEDIA_DATA_DB_PARENT_ID, dirAsset.GetAlbumId());
    assetInfo.PutString(MEDIA_DATA_DB_BUCKET_NAME, dirAsset.GetAlbumName());
    assetInfo.PutString(MEDIA_DATA_DB_OWNER_PACKAGE, cmd.GetBundleName());
    assetInfo.PutString(MEDIA_DATA_DB_OWNER_APPID, PermissionUtils::GetAppIdByBundleName(cmd.GetBundleName()));
    if (!cmd.GetBundleName().empty()) {
        assetInfo.PutString(MEDIA_DATA_DB_PACKAGE_NAME,
            PermissionUtils::GetPackageNameByBundleName(cmd.GetBundleName()));
    }

    assetInfo.PutString(MEDIA_DATA_DB_DEVICE_NAME, cmd.GetDeviceName());
    assetInfo.PutLong(MEDIA_DATA_DB_TIME_PENDING, fileAsset.GetTimePending());
    cmd.SetValueBucket(assetInfo);
    int64_t outRowId = -1;
    int32_t errCode = uniStore->Insert(cmd, outRowId);
    return (errCode == NativeRdb::E_OK) ? outRowId : errCode;
}

void GetRelativePathFromValues(ValuesBucket &values, string &relativePath, int32_t mediaType)
{
    ValueObject valueObject;
    if (values.GetObject(MEDIA_DATA_DB_RELATIVE_PATH, valueObject)) {
        valueObject.GetString(relativePath);
        return;
    }
    if (values.GetObject(MEDIA_DATA_DB_URI, valueObject)) {
        string albumUri;
        valueObject.GetString(albumUri);
        auto albumAsset = MediaLibraryObjectUtils::GetFileAssetFromUri(albumUri);
        if (albumAsset != nullptr) {
            relativePath = albumAsset->GetRelativePath() + albumAsset->GetDisplayName() + SLASH_CHAR;
        }
    } else {
        MediaLibraryObjectUtils::GetDefaultRelativePath(mediaType, relativePath);
    }
}

int32_t MediaLibraryObjectUtils::BuildFileAsset(MediaLibraryCommand &cmd, FileAsset &fileAsset,
    NativeAlbumAsset &dirAsset, string &path, int32_t &mediaType)
{
    string relativePath;
    string displayName;

    ValueObject valueObject;
    ValuesBucket &values = cmd.GetValueBucket();
    if (!values.GetObject(MEDIA_DATA_DB_NAME, valueObject)) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::CREATE}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return E_HAS_DB_ERROR;
    }
    valueObject.GetString(displayName);
    fileAsset.SetDisplayName(displayName);
    if (!values.GetObject(MEDIA_DATA_DB_MEDIA_TYPE, valueObject)) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::CREATE}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return E_HAS_DB_ERROR;
    }
    valueObject.GetInt(mediaType);
    fileAsset.SetMediaType(static_cast<MediaType>(mediaType));
    GetRelativePathFromValues(values, relativePath, mediaType);
    if (!relativePath.empty()) {
        values.PutString(MEDIA_DATA_DB_RELATIVE_PATH, relativePath);
        path = ROOT_MEDIA_DIR + relativePath + displayName;
        fileAsset.SetRelativePath(relativePath);
        fileAsset.SetPath(path);
    }

    // check dir and extension
    int32_t errCode = CheckDirExtension(relativePath, displayName);
    if (errCode != E_SUCCESS) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, errCode},
            {KEY_OPT_FILE, path}, {KEY_OPT_TYPE, OptType::CREATE}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
    }
    CHECK_AND_RETURN_RET_LOG(errCode == E_SUCCESS, errCode, "CreateFileAsset: check file asset failed");
    dirAsset = GetDirAsset(ROOT_MEDIA_DIR + relativePath);
    if (dirAsset.GetAlbumId() < 0) {
        return dirAsset.GetAlbumId();
    }
    fileAsset.SetTimePending(0);
    return E_SUCCESS;
}

// create
int32_t MediaLibraryObjectUtils::CreateFileObj(MediaLibraryCommand &cmd)
{
    FileAsset fileAsset;
    NativeAlbumAsset dirAsset;
    string path;
    int32_t mediaType = static_cast<int32_t>(MEDIA_TYPE_FILE);
    int errCode = BuildFileAsset(cmd, fileAsset, dirAsset, path, mediaType);
    if (errCode != E_SUCCESS) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, errCode},
            {KEY_OPT_TYPE, OptType::CREATE}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        MEDIA_ERR_LOG("Build file asset error");
        return errCode;
    }
    // delete rows in database but not in real filesystem
    errCode = DeleteInvalidRowInDb(path);
    if (errCode != E_SUCCESS) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, errCode},
            {KEY_OPT_TYPE, OptType::CREATE}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        MEDIA_ERR_LOG("Delete invalid row in database failed");
        return errCode;
    }

    errCode = MediaFileUtils::CreateAsset(path);
    if (errCode != E_SUCCESS) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, errCode},
            {KEY_OPT_FILE, path}, {KEY_OPT_TYPE, OptType::CREATE}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        MEDIA_ERR_LOG("CreateFileAsset: create file asset failed");
        return errCode;
    }

    if (mediaType == MEDIA_TYPE_NOFILE) {
        errCode = UpdateDateModified(MediaFileUtils::GetParentPath(MediaFileUtils::GetParentPath(fileAsset.GetPath())));
        if (errCode == E_HAS_DB_ERROR) {
            VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, errCode},
                {KEY_OPT_TYPE, OptType::CREATE}};
            PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        }
        return dirAsset.GetAlbumId();
    }
    auto ret = InsertFileInDb(cmd, fileAsset, dirAsset);
    if (ret > 0) {
        UpdateDateModified(MediaFileUtils::GetParentPath(fileAsset.GetPath()));
    } else if (ret < 0) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, ret},
            {KEY_OPT_TYPE, OptType::CREATE}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
    }
    return ret;
}

NativeAlbumAsset MediaLibraryObjectUtils::GetLastDirExistInDb(const string &dirPath)
{
    NativeAlbumAsset dirAsset;
    string lastPath = dirPath;
    if (lastPath.back() == '/') {
        lastPath.pop_back();
    }
    int32_t dirId = 0;
    int32_t lastPathId = -1;
    do {
        size_t slashIndex = lastPath.rfind(SLASH_CHAR);
        if (slashIndex == string::npos || lastPath.length() <= ROOT_MEDIA_DIR.length()) {
            break;
        }
        lastPath = lastPath.substr(0, slashIndex);
        lastPathId = GetIdByPathFromDb(lastPath);
        if (lastPathId >= 0) {
            dirId = lastPathId;
        }
    } while (lastPathId < 0);
    MEDIA_DEBUG_LOG("GetLastAlbumExistInDb lastPath = %{private}s", lastPath.c_str());
    dirAsset.SetAlbumId(dirId);
    dirAsset.SetAlbumPath(lastPath);
    return dirAsset;
}

int32_t MediaLibraryObjectUtils::DeleteRows(const vector<int64_t> &rowIds)
{
    int32_t errCode = 0;

    for (auto id : rowIds) {
        MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::DELETE);
        errCode = DeleteInfoByIdInDb(cmd, to_string(id));
    }
    return errCode;
}

int32_t SetDirValuesByPath(ValuesBucket &values, const string &path, int32_t parentId)
{
    string title = MediaFileUtils::GetFileName(path);
    if (MediaFileUtils::CheckDentryName(title) < 0) {
        MEDIA_ERR_LOG("Check display name failed!");
        return E_INVALID_DISPLAY_NAME;
    }

    string relativePath;
    string parentPath = MediaFileUtils::GetParentPath(path);
    if (parentPath.length() > ROOT_MEDIA_DIR.length()) {
        relativePath = parentPath.substr(ROOT_MEDIA_DIR.length()) + "/";
    }

    values.PutString(MEDIA_DATA_DB_FILE_PATH, path);
    values.PutString(MEDIA_DATA_DB_RELATIVE_PATH, relativePath);
    values.PutString(MEDIA_DATA_DB_TITLE, title);
    values.PutString(MEDIA_DATA_DB_NAME, title);
    values.PutInt(MEDIA_DATA_DB_MEDIA_TYPE, MediaType::MEDIA_TYPE_ALBUM);
    values.PutInt(MEDIA_DATA_DB_PARENT_ID, parentId);
    values.PutLong(MEDIA_DATA_DB_DATE_ADDED, MediaFileUtils::UTCTimeMilliSeconds());

    struct stat statInfo {};
    if (stat(path.c_str(), &statInfo) == 0) {
        values.PutLong(MEDIA_DATA_DB_SIZE, statInfo.st_size);
        if (statInfo.st_mtime == 0) {
            values.PutLong(MEDIA_DATA_DB_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
        } else {
            values.PutLong(MEDIA_DATA_DB_DATE_MODIFIED, MediaFileUtils::Timespec2Millisecond(statInfo.st_mtim));
        }
    }
    return E_SUCCESS;
}

int32_t MediaLibraryObjectUtils::InsertDirToDbRecursively(const string &dirPath, int64_t &rowId)
{
    CHECK_AND_RETURN_RET_LOG(!dirPath.empty(), E_VIOLATION_PARAMETERS, "Input parameter dirPath is empty!");

    NativeAlbumAsset dirAsset = GetLastDirExistInDb(dirPath);
    string parentPath = dirAsset.GetAlbumPath();
    int32_t parentId = dirAsset.GetAlbumId();
    if ((parentId == 0) && ((parentPath + "/") != ROOT_MEDIA_DIR)) {
        return E_INVALID_PATH;
    }
    vector<int64_t> outIds;
    rowId = parentId;
    string path = dirPath;
    if (path.back() != '/') {
        path.append("/");
    }
    while (parentPath.length() < (path.length() - 1)) {
        size_t index = path.find("/", parentPath.length() + 1);
        string currentPath = path.substr(0, index);
        ValuesBucket values;
        auto ret = SetDirValuesByPath(values, currentPath, parentId);
        if (ret == E_INVALID_DISPLAY_NAME) {
            DeleteRows(outIds);
        }
        if (ret != E_SUCCESS) {
            return ret;
        }

        MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ALBUM, OperationType::CREATE, values);
        rowId = InsertInDb(cmd);
        if (rowId <= 0) {
            rowId = parentId;
            return E_HAS_DB_ERROR;
        }
        parentId = rowId;
        parentPath = currentPath;
        outIds.push_back(rowId);
    }
    return E_SUCCESS;
}

int32_t MediaLibraryObjectUtils::CreateDirObj(MediaLibraryCommand &cmd, int64_t &rowId)
{
    string dirPath;
    ValueObject valueObject;
    const ValuesBucket &values = cmd.GetValueBucket();
    if (values.GetObject(MEDIA_DATA_DB_FILE_PATH, valueObject)) {
        valueObject.GetString(dirPath);
    }
    if (dirPath.empty()) {
        MEDIA_ERR_LOG("Dir path is empty!");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_INVALID_PATH},
            {KEY_OPT_TYPE, OptType::CREATE}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return E_INVALID_PATH;
    }

    rowId = GetIdByPathFromDb(dirPath);
    MEDIA_DEBUG_LOG("dirPath %{private}s id in database is %{private}d", dirPath.c_str(), static_cast<int>(rowId));
    if ((rowId < 0) || (!MediaFileUtils::IsDirectory(dirPath))) {
        if ((!MediaFileUtils::CreateDirectory(dirPath)) && (errno != EEXIST)) {
            VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, -errno},
                {KEY_OPT_FILE, dirPath}, {KEY_OPT_TYPE, OptType::CREATE}};
            PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
            return E_FAIL;
        }
        auto ret = InsertDirToDbRecursively(dirPath, rowId);
        if (ret != E_SUCCESS) {
            VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, ret},
                {KEY_OPT_TYPE, OptType::CREATE}};
            PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        }
        return ret;
    }
    return E_FILE_EXIST;
}

int32_t InitQueryParentResultSet(int32_t dirId, int32_t &parentIdVal, string &dirVal,
    shared_ptr<MediaLibraryRdbStore> uniStore)
{
    if (MediaLibraryObjectUtils::IsColumnValueExist(to_string(dirId), MEDIA_DATA_DB_PARENT_ID)) {
        return E_SUCCESS;
    }

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::QUERY);
    cmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_ID, to_string(dirId));
    shared_ptr<NativeRdb::ResultSet> queryParentResultSet = uniStore->Query(cmd, {});
    if (queryParentResultSet == nullptr || queryParentResultSet->GoToNextRow() != NativeRdb::E_OK) {
        return E_SUCCESS;
    }
    int32_t colIndex = 0;
    queryParentResultSet->GetColumnIndex(MEDIA_DATA_DB_PARENT_ID, colIndex);
    queryParentResultSet->GetInt(colIndex, parentIdVal);
    queryParentResultSet->GetColumnIndex(MEDIA_DATA_DB_FILE_PATH, colIndex);
    queryParentResultSet->GetString(colIndex, dirVal);
    queryParentResultSet.reset();
    if (parentIdVal == 0) {
        return E_SUCCESS;
    }
    MEDIA_DEBUG_LOG("dirVal = %{private}s, parentIdVal = %{public}d", dirVal.c_str(), parentIdVal);
    return E_ERR;
}

int32_t MediaLibraryObjectUtils::DeleteEmptyDirsRecursively(int32_t dirId)
{
    if (dirId <= 0) {
        return E_INVALID_FILEID;
    }
    shared_ptr<MediaLibraryRdbStore> uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr!");
        return E_HAS_DB_ERROR;
    }

    int err = E_ERR;
    const int32_t MAX_DIR_DEPTH = 15;
    int depth = 0;
    while ((depth++ < MAX_DIR_DEPTH) && (dirId > 0)) {
        int32_t parentIdVal = 0;
        string dirVal;
        int32_t state = InitQueryParentResultSet(dirId, parentIdVal, dirVal, uniStore);
        if (state == E_SUCCESS) {
            return state;
        }
        // Do not delete user created dir
        if (MediaFileUtils::IsFileExists(dirVal + "/" + ".nofile")) {
            return E_SUCCESS;
        }
        if (!MediaFileUtils::IsDirEmpty(dirVal)) {
            return E_SUCCESS;
        }

        if (!MediaFileUtils::DeleteDir(dirVal)) {
            MEDIA_ERR_LOG("Delete dir in filesystem failed, errno = %{public}d", errno);
            err = E_HAS_FS_ERROR;
            break;
        }
        MediaLibraryCommand deleteDirCmd(OperationObject::FILESYSTEM_DIR, OperationType::DELETE);
        int32_t deletedRows = DeleteInfoByIdInDb(deleteDirCmd, to_string(dirId));
        if (deletedRows < 0) {
            MEDIA_ERR_LOG("Delete dir info failed, err: %{public}d", deletedRows);
            err = deletedRows;
            break;
        } else if (deletedRows == 0) {
            MEDIA_ERR_LOG("Failed to delete dir in db!");
            return E_HAS_DB_ERROR;
        }
        dirId = parentIdVal;
    }
    return err;
}

void MediaLibraryObjectUtils::InvalidateThumbnail(const string &id, const string &tableName, const string &path)
{
    ThumbnailService::GetInstance()->HasInvalidateThumbnail(id, tableName, path);
}

int32_t MediaLibraryObjectUtils::DeleteMisc(const int32_t fileId, const string &filePath, const int32_t parentId)
{
    // 1) update parent's modify time
    string parentPath = MediaFileUtils::GetParentPath(filePath);
    auto updatedRows = UpdateDateModified(parentPath);
    if (updatedRows <= 0) {
        MEDIA_ERR_LOG("Update album date_modified failed, path: %{private}s", parentPath.c_str());
    }
    // 2) recursively delete empty parent dirs
    if (DeleteEmptyDirsRecursively(parentId) != E_SUCCESS) {
        return E_DELETE_DIR_FAIL;
    }
    // 3) delete relative records in smart album
    MediaLibraryCommand deleteSmartMapCmd(OperationObject::SMART_ALBUM_MAP, OperationType::DELETE);
    deleteSmartMapCmd.GetAbsRdbPredicates()->EqualTo(SMARTALBUMMAP_DB_CHILD_ASSET_ID, to_string(fileId));
    return DeleteInfoByIdInDb(deleteSmartMapCmd);
}

// Restriction: input param cmd MUST have file id in either uri or valuebucket
int32_t MediaLibraryObjectUtils::DeleteFileObj(const shared_ptr<FileAsset> &fileAsset)
{
    // delete file in filesystem
    string filePath = fileAsset->GetPath();
    if (!MediaFileUtils::DeleteFile(filePath)) {
        MEDIA_ERR_LOG("Delete file asset failed, errno: %{public}d", errno);
        return E_HAS_FS_ERROR;
    }

    // delete thumbnail
    int32_t fileId = fileAsset->GetId();
    InvalidateThumbnail(to_string(fileId));

    // delete file in db
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::DELETE);
    cmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_ID, to_string(fileId));
    int32_t deleteRows = DeleteInfoByIdInDb(cmd);
    if (deleteRows <= 0) {
        MEDIA_ERR_LOG("Delete file info in database failed!");
        return deleteRows;
    }

    return DeleteMisc(fileId, filePath, fileAsset->GetParent());
}

int32_t MediaLibraryObjectUtils::DeleteInfoRecursively(const shared_ptr<FileAsset> &fileAsset)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_HAS_DB_ERROR, "UniStore is nullptr");

    string fileId = to_string(fileAsset->GetId());
    if (fileAsset->GetMediaType() == MEDIA_TYPE_ALBUM) {
        MediaLibraryCommand queryCmd(OperationObject::FILESYSTEM_ASSET, OperationType::QUERY);
        queryCmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_PARENT_ID, fileId);
        if (fileAsset->GetIsTrash() == NOT_TRASHED) {
            queryCmd.GetAbsRdbPredicates()->And()->EqualTo(MEDIA_DATA_DB_IS_TRASH, to_string(NOT_TRASHED));
        } else {
            queryCmd.GetAbsRdbPredicates()->And()->EqualTo(MEDIA_DATA_DB_IS_TRASH, to_string(TRASHED_DIR_CHILD));
        }
        vector<string> columns = { MEDIA_DATA_DB_ID, MEDIA_DATA_DB_MEDIA_TYPE, MEDIA_DATA_DB_IS_TRASH };
        auto result = uniStore->Query(queryCmd, columns);
        CHECK_AND_RETURN_RET_LOG(result != nullptr, E_DB_FAIL, "Get child assets failed.");

        while (result->GoToNextRow() == NativeRdb::E_OK) {
            auto childAsset = make_shared<FileAsset>();
            childAsset->SetId(GetInt32Val(MEDIA_DATA_DB_ID, result));
            childAsset->SetMediaType(static_cast<MediaType>(GetInt32Val(MEDIA_DATA_DB_MEDIA_TYPE, result)));
            childAsset->SetIsTrash(GetInt32Val(MEDIA_DATA_DB_IS_TRASH, result));
            auto ret = DeleteInfoRecursively(childAsset);
            CHECK_AND_RETURN_RET_LOG(ret == E_SUCCESS, ret,
                "Delete file info in database failed, file_id: %{public}d, ret: %{public}d", childAsset->GetId(), ret);
        }
    }

    InvalidateThumbnail(fileId);
    MediaLibraryCommand deleteCmd(Uri(MEDIALIBRARY_DATA_URI), OperationType::DELETE);
    int32_t deleteRows = MediaLibraryObjectUtils::DeleteInfoByIdInDb(deleteCmd, fileId);
    if (deleteRows <= 0) {
        MEDIA_ERR_LOG("Delete file info in database failed, file_id: %{private}s", fileId.c_str());
        return E_DB_FAIL;
    }
    return E_SUCCESS;
}

int32_t MediaLibraryObjectUtils::DeleteDirObj(const shared_ptr<FileAsset> &dirAsset)
{
    // delete dir in filesystem
    string dirPath = dirAsset->GetPath();
    if (!MediaFileUtils::DeleteDir(dirPath)) {
        MEDIA_ERR_LOG("Delete album asset failed, errno: %{public}d", errno);
        return E_HAS_FS_ERROR;
    }

    // delete dir and children in db
    auto ret = DeleteInfoRecursively(dirAsset);
    CHECK_AND_RETURN_RET_LOG(ret == E_SUCCESS, ret,
        "Delete file info in database failed, file_id: %{public}d, ret: %{public}d", dirAsset->GetId(), ret);

    return DeleteMisc(dirAsset->GetId(), dirPath, dirAsset->GetParent());
}

// Restriction: input param cmd MUST have id in uri
int32_t MediaLibraryObjectUtils::RenameFileObj(MediaLibraryCommand &cmd,
    const string &srcFilePath, const string &dstFilePath)
{
    if (srcFilePath.empty() || dstFilePath.empty()) {
        MEDIA_ERR_LOG("srcFilePath or dstFilePath is empty, rename failed!");
        return E_INVALID_PATH;
    }
    if (srcFilePath.compare(dstFilePath) == 0) {
        MEDIA_DEBUG_LOG("Skip modify the file, the path of new file is the same as old");
        return E_SUCCESS;
    }
    auto errCode = CheckDirExtension(dstFilePath);
    CHECK_AND_RETURN_RET_LOG(errCode == E_SUCCESS, errCode, "CreateFileAsset: check file asset failed");
    if (errCode != E_SUCCESS) {
        return errCode;
    }

    string dstAlbumPath = MediaFileUtils::GetParentPath(dstFilePath);
    NativeAlbumAsset dirAsset = GetDirAsset(dstAlbumPath);
    if (dirAsset.GetAlbumId() <= 0) {
        MEDIA_ERR_LOG("Failed to get or create directory");
        return dirAsset.GetAlbumId();
    }

    errCode = MediaFileUtils::ModifyAsset(srcFilePath, dstFilePath);
    if (errCode != E_SUCCESS) {
        if (errCode != E_FILE_EXIST) {
            MEDIA_ERR_LOG("Failed to modify the file in the device, errCode = %{public}d", errCode);
        }
        return errCode;
    }
    InvalidateThumbnail(cmd.GetOprnFileId());
    string dstFileName = MediaFileUtils::GetFileName(dstFilePath);
    if ((ProcessNoMediaFile(dstFileName, dstAlbumPath) == E_SUCCESS) ||
        (ProcessHiddenFile(dstFileName, srcFilePath) == E_SUCCESS)) {
        MEDIA_ERR_LOG("New file is a .nomedia file or hidden file.");
        // why: return fail insteal of success
        return E_FAIL;
    }

    auto ret = UpdateFileInfoInDb(cmd, dstFilePath, dirAsset.GetAlbumId(), dirAsset.GetAlbumName());
    if (ret > 0) {
        UpdateDateModified(dstAlbumPath);
        string srcAlbumPath = MediaFileUtils::GetParentPath(srcFilePath);
        UpdateDateModified(srcAlbumPath);
    }
    return ret;
}

// Restriction: input param cmd MUST have id in uri
int32_t MediaLibraryObjectUtils::RenameDirObj(MediaLibraryCommand &cmd,
    const string &srcDirPath, const string &dstDirPath)
{
    MEDIA_DEBUG_LOG("enter, srcDirPath = %{private}s, dstDirPath = %{private}s",
        srcDirPath.c_str(), dstDirPath.c_str());
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr!");
        return E_HAS_DB_ERROR;
    }

    if (srcDirPath.empty() || dstDirPath.empty()) {
        MEDIA_ERR_LOG("srcDirPath or dstDirPath is empty, rename failed!");
        return E_INVALID_PATH;
    }
    if (!MediaFileUtils::RenameDir(srcDirPath, dstDirPath)) {
        MEDIA_ERR_LOG("Rename directory failed!");
        return E_HAS_FS_ERROR;
    }
    string dstDirName = MediaFileUtils::GetFileName(dstDirPath);
    if (ProcessHiddenDir(dstDirName, srcDirPath) == E_SUCCESS) {
        MEDIA_ERR_LOG("New album is a hidden album.");
        return E_SUCCESS;
    }

    ValuesBucket &values = cmd.GetValueBucket();
    values.PutString(Media::MEDIA_DATA_DB_RELATIVE_PATH, MediaFileUtils::GetParentPath(dstDirPath));
    values.PutString(Media::MEDIA_DATA_DB_FILE_PATH, dstDirPath);
    values.PutLong(MEDIA_DATA_DB_DATE_MODIFIED, MediaFileUtils::GetAlbumDateModified(dstDirPath));
    int32_t retVal = ModifyInfoByIdInDb(cmd);
    if (retVal <= 0) {
        return retVal;
    }

    // Update the path, relative path and album Name for internal files
    const string modifyAlbumInternalsStmt =
        "UPDATE " + MEDIALIBRARY_TABLE + " SET " + MEDIA_DATA_DB_FILE_PATH + " = replace(" +
        MEDIA_DATA_DB_FILE_PATH + ", '" + srcDirPath + "/' , '" + dstDirPath + "/'), " +
        MEDIA_DATA_DB_RELATIVE_PATH + " = replace(" + MEDIA_DATA_DB_RELATIVE_PATH + ", '" + srcDirPath + "', '" +
        dstDirPath + "'), " + MEDIA_DATA_DB_ALBUM_NAME + " = replace(" + MEDIA_DATA_DB_ALBUM_NAME + ", '" +
        MediaFileUtils::GetFileName(srcDirPath) + "', '" + dstDirName + "')" + "where " +
        MEDIA_DATA_DB_FILE_PATH + " LIKE '" + srcDirPath + "/%'";
    if (uniStore->ExecuteSql(modifyAlbumInternalsStmt) != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Album update sql failed");
        return E_HAS_DB_ERROR;
    }
    return E_SUCCESS;
}

static int32_t OpenAsset(const string &filePath, const string &mode, const string &fileId, int32_t type = -1)
{
    MediaLibraryTracer tracer;
    tracer.Start("OpenAsset");

    string absFilePath;
    if (!PathToRealPath(filePath, absFilePath)) {
        MEDIA_ERR_LOG("Failed to get real path: %{private}s", filePath.c_str());
        return E_ERR;
    }
    MEDIA_DEBUG_LOG("File absFilePath is %{private}s", absFilePath.c_str());
    MEDIA_DEBUG_LOG("object util type:%{public}d", type);

    return MediaPrivacyManager(absFilePath, mode, fileId, type).Open();
}

static bool CheckIsOwner(const string &bundleName)
{
    string clientBundle = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    if (strcmp(bundleName.c_str(), clientBundle.c_str()) == 0) {
        return true;
    }
    return false;
}

static int32_t OpenDocument(const string &uri, const string &mode)
{
    static constexpr uint32_t BASE_USER_RANGE = 200000;
    uid_t uid = getuid() / BASE_USER_RANGE;
    string realPath;
    int32_t ret = AppFileService::SandboxHelper::GetPhysicalPath(uri, to_string(uid), realPath);
    if (ret != E_OK || !AppFileService::SandboxHelper::CheckValidPath(realPath)) {
        MEDIA_ERR_LOG("file not exist, uri=%{private}s, realPath=%{private}s",
                      uri.c_str(), realPath.c_str());
        return E_INVALID_URI;
    }
    return MediaFileUtils::OpenFile(realPath, mode);
}

static bool IsNumber(const string &betaIssueId)
{
    CHECK_AND_RETURN_RET_LOG(!betaIssueId.empty(), false, "betaIssueId is empty");
    for (const char &c : betaIssueId) {
        CHECK_AND_RETURN_RET(isdigit(c) != 0, false);
    }
    return true;
}

static int32_t OpenDebugDatabase(const string &uri, const string &mode)
{
    CHECK_AND_RETURN_RET_LOG(PermissionUtils::IsSystemApp(), E_CHECK_SYSTEMAPP_FAIL, "Caller not systemapp");
    CHECK_AND_RETURN_RET_LOG(PermissionUtils::IsBetaVersion(), E_BETA_VERSION_FAIL, "Caller not beta version");
    size_t pos = uri.find_last_of("/");
    CHECK_AND_RETURN_RET_LOG(pos != string::npos, E_OPR_DEBUG_DB_FAIL, "uri not contain '/' ");
    string betaIssueId = uri.substr(pos + 1);
    CHECK_AND_RETURN_RET_LOG(IsNumber(betaIssueId), E_ACQ_BETA_TASK_FAIL, "betaIssueId is invalid");
    string realPath = "/data/storage/el2/log/logpack/media_library_" + betaIssueId + ".db.zip";
    int32_t fileFd = MediaFileUtils::OpenFile(realPath, mode);
    CHECK_AND_RETURN_RET_LOG(fileFd >= 0, E_OPR_DEBUG_DB_FAIL, "Failed to open debug db, errno %{public}d", errno);
    return fileFd;
}

static bool IsDocumentUri(const std::string &uriString)
{
    Uri uri(uriString);
    return uri.GetAuthority() == DOCUMENT_URI_AUTHORITY;
}

static void GetType(string &uri, int32_t &type)
{
    size_t pos = uri.find("type=");
    if (pos != uri.npos) {
        type = uri[pos + OFFSET] - ZERO_ASCII;
    }
}

int32_t HandleRequestPicture(MediaLibraryCommand &cmd)
{
    std::string fileId = cmd.GetQuerySetParam(MediaColumn::MEDIA_ID);
    int32_t fd;
    PictureHandlerService::OpenPicture(fileId, fd);
    return fd;
}

int32_t HandlePhotoRequestPictureBuffer(MediaLibraryCommand &cmd)
{
    std::string fd = cmd.GetQuerySetParam("fd");
    return PictureHandlerService::RequestBufferHandlerFd(fd);
}

int32_t MediaLibraryObjectUtils::OpenFile(MediaLibraryCommand &cmd, const string &mode)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryObjectUtils::OpenFile");
    string uriString = cmd.GetUri().ToString();
    int32_t type = -1;
    GetType(uriString, type);
    if (cmd.GetOprnObject() == OperationObject::THUMBNAIL) {
        return ThumbnailService::GetInstance()->GetThumbnailFd(uriString);
    } else if (cmd.GetOprnObject() == OperationObject::THUMBNAIL_ASTC) {
        return ThumbnailService::GetInstance()->GetThumbnailFd(uriString, true);
    } else if (cmd.GetOprnObject() == OperationObject::REQUEST_PICTURE) {
        return HandleRequestPicture(cmd);
    } else if (cmd.GetOprnObject() == OperationObject::PHOTO_REQUEST_PICTURE_BUFFER) {
        return HandlePhotoRequestPictureBuffer(cmd);
    } else if (cmd.GetOprnObject() == OperationObject::KEY_FRAME) {
        return ThumbnailService::GetInstance()->GetKeyFrameThumbnailFd(uriString, true);
    } else if (IsDocumentUri(uriString)) {
        return OpenDocument(uriString, mode);
    } else if (cmd.GetOprnObject() == OperationObject::FILESYSTEM_DEBUG_DB) {
        return OpenDebugDatabase(uriString, mode);
    }
    shared_ptr<FileAsset> fileAsset = GetFileAssetFromUri(uriString);
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_URI, "Failed to obtain path from Database");
    if (fileAsset->GetTimePending() != 0 && !CheckIsOwner(fileAsset->GetOwnerPackage().c_str())) {
        MEDIA_ERR_LOG("Failed to open fileId:%{public}d, it is not owner", fileAsset->GetId());
        return E_IS_PENDING_ERROR;
    }
    bool isHeif = cmd.GetQuerySetParam(PHOTO_TRANSCODE_OPERATION) == OPRN_TRANSCODE_HEIF;
    int32_t err = MediaLibraryTranscodeDataAgingOperation::SetTranscodeUriToFileAsset(fileAsset, mode, isHeif);
    string path = MediaFileUtils::UpdatePath(fileAsset->GetPath(), fileAsset->GetUri());
    string fileId = MediaFileUtils::GetIdFromUri(fileAsset->GetUri());
    int32_t fd = OpenAsset(path, mode, fileId, type);
    CHECK_AND_RETURN_RET_LOG(fd >= 0, E_HAS_FS_ERROR, "open file fd %{private}d, errno %{private}d", fd, errno);
    if (err == 0) {
        MediaLibraryTranscodeDataAgingOperation::DoTranscodeDfx(ACCESS_MEDIALIB);
    }
    if (mode.find(MEDIA_FILEMODE_WRITEONLY) != string::npos) {
        auto watch = MediaLibraryInotify::GetInstance();
        if (watch != nullptr) {
            MEDIA_INFO_LOG("enter inotify, path = %{public}s, fileId = %{public}d",
                DfxUtils::GetSafePath(path).c_str(), fileAsset->GetId());
            watch->AddWatchList(path, fileAsset->GetUri());
        }
    }
    MEDIA_DEBUG_LOG("MediaLibraryDataManager OpenFile: Success");
    return fd;
}

void MediaLibraryObjectUtils::ScanFileAsync(const string &path, const string &id, MediaLibraryApi api,
    bool isCameraShotMovingPhoto, std::shared_ptr<Media::Picture> resultPicture,
    std::shared_ptr<IMediaScannerCallback> callback)
{
    string tableName;
    if (MediaFileUtils::IsFileTablePath(path)) {
        tableName = MEDIALIBRARY_TABLE;
    } else if (MediaFileUtils::IsPhotoTablePath(path)) {
        tableName = PhotoColumn::PHOTOS_TABLE;
    } else {
        tableName = AudioColumn::AUDIOS_TABLE;
    }

    if (PhotoFileUtils::IsThumbnailExists(path) && !PhotoFileUtils::IsThumbnailLatest(path)) {
        InvalidateThumbnail(id, tableName);
    }

    shared_ptr<ScanFileCallback> scanFileCb = make_shared<ScanFileCallback>();
    if (scanFileCb == nullptr) {
        MEDIA_ERR_LOG("Failed to create scan file callback object");
        return ;
    }
    scanFileCb->SetOriginalPhotoPicture(resultPicture);
    scanFileCb->SetCallback(callback);
    int ret = MediaScannerManager::GetInstance()->ScanFile(path, scanFileCb, api, isCameraShotMovingPhoto);
    if (ret != 0) {
        MEDIA_ERR_LOG("Scan file failed!");
    }
}

void MediaLibraryObjectUtils::ScanFileSyncWithoutAlbumUpdate(const string &path, const string &id, MediaLibraryApi api,
    std::shared_ptr<Media::Picture> picture)
{
    string tableName;
    if (MediaFileUtils::IsFileTablePath(path)) {
        tableName = MEDIALIBRARY_TABLE;
    } else if (MediaFileUtils::IsPhotoTablePath(path)) {
        tableName = PhotoColumn::PHOTOS_TABLE;
    } else {
        tableName = AudioColumn::AUDIOS_TABLE;
    }

    if (PhotoFileUtils::IsThumbnailExists(path) && !PhotoFileUtils::IsThumbnailLatest(path)) {
        InvalidateThumbnail(id, tableName);
    }

    shared_ptr<ScanFileCallback> scanFileCb = make_shared<ScanFileCallback>();
    if (scanFileCb == nullptr) {
        MEDIA_ERR_LOG("Failed to create scan file callback object");
        return ;
    }
    if (picture != nullptr) {
        scanFileCb->SetOriginalPhotoPicture(picture);
    }
    int ret = MediaScannerManager::GetInstance()->ScanFileSyncWithoutAlbumUpdate(path, scanFileCb, api, true);
    if (ret != 0) {
        MEDIA_ERR_LOG("Scan file failed!");
    }
}

void MediaLibraryObjectUtils::ScanMovingPhotoVideoAsync(
    const std::string &path, bool isCameraShotMovingPhoto, bool isScanCreateThumb)
{
    int ret = E_ERR;
    if (isScanCreateThumb) {
        shared_ptr<ScanFileCallback> scanFileCb = make_shared<ScanFileCallback>();
        if (scanFileCb == nullptr) {
            MEDIA_ERR_LOG("Failed to create scan file callback object");
            return;
        }
        scanFileCb->SetOriginalPhotoPicture(nullptr);
        ret = MediaScannerManager::GetInstance()->ScanFile(
            path, scanFileCb, MediaLibraryApi::API_10, isCameraShotMovingPhoto);
    } else {
        ret = MediaScannerManager::GetInstance()->ScanFile(
            path, nullptr, MediaLibraryApi::API_10, isCameraShotMovingPhoto);
    }
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Scan file failed!");
    }
}

int32_t MediaLibraryObjectUtils::CloseFile(MediaLibraryCommand &cmd)
{
    string strFileId = cmd.GetOprnFileId();
    if (strFileId.empty()) {
        MEDIA_ERR_LOG("Get id from uri or valuesBucket failed!");
        return E_INVALID_FILEID;
    }

    auto fileAsset = GetFileAssetFromId(strFileId);
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("Get fileAsset from database fail!");
        return E_INVALID_FILEID;
    }
    string srcPath = fileAsset->GetPath();
    string fileName = MediaFileUtils::GetFileName(srcPath);
    if ((fileName.length() != 0) && (fileName.at(0) != '.')) {
        string dirPath = MediaFileUtils::GetParentPath(srcPath);
        UpdateDateModified(dirPath);
    }

    // remove inotify event since there is close cmd
    auto watch = MediaLibraryInotify::GetInstance();
    if (watch != nullptr) {
        watch->RemoveByFileUri(fileAsset->GetUri());
    }
    if (PhotoFileUtils::IsThumbnailExists(srcPath) && !PhotoFileUtils::IsThumbnailLatest(srcPath)) {
        InvalidateThumbnail(strFileId);
    }
    ScanFile(srcPath);
    return E_SUCCESS;
}

void MediaLibraryObjectUtils::ScanFile(const string &path, MediaLibraryApi api)
{
    MEDIA_DEBUG_LOG("enter, path = %{private}s, api = %{public}d", path.c_str(), static_cast<int>(api));
    shared_ptr<ScanFileCallback> scanFileCb = make_shared<ScanFileCallback>();
    if (scanFileCb == nullptr) {
        MEDIA_ERR_LOG("Failed to create scan file callback object");
        return ;
    }
    int ret = MediaScannerManager::GetInstance()->ScanFileSync(path, scanFileCb, api);
    if (ret != 0) {
        MEDIA_ERR_LOG("Scan file failed!");
    }
}

int32_t MediaLibraryObjectUtils::ProcessNoMediaFile(const string &dstFileName, const string &dstAlbumPath)
{
    MEDIA_DEBUG_LOG("enter, dstFileName = %{private}s, dstAlbumPath = %{private}s",
        dstFileName.c_str(), dstAlbumPath.c_str());
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr!");
        return E_HAS_DB_ERROR;
    }

    if (dstFileName.empty() || dstAlbumPath.empty() || dstFileName != NO_MEDIA_TAG) {
        MEDIA_INFO_LOG("Not a .nomedia file, no need to do anything.");
        return E_INVALID_ARGUMENTS;
    }

    // the whole folder containing .nomedia file is invisible in database
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::DELETE);
    string dstAlbumPathPrefix = dstAlbumPath.back() != '/' ? (dstAlbumPath + "/") : dstAlbumPath;
    cmd.GetAbsRdbPredicates()->BeginsWith(MEDIA_DATA_DB_FILE_PATH, dstAlbumPathPrefix);
    cmd.GetAbsRdbPredicates()->Or()->EqualTo(MEDIA_DATA_DB_FILE_PATH, dstAlbumPath);

    int32_t deletedRows = -1;
    if (uniStore->Delete(cmd, deletedRows) != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Delete rows for the hidden album failed");
    }
    return E_SUCCESS;
}

int32_t MediaLibraryObjectUtils::ProcessHiddenFile(const string &dstFileName, const string &srcPath)
{
    MEDIA_DEBUG_LOG("enter, dstFileName = %{private}s, srcPath = %{private}s",
        dstFileName.c_str(), srcPath.c_str());
    if (dstFileName.empty() || srcPath.empty() || dstFileName.at(0) != '.') {
        MEDIA_INFO_LOG("Not a hidden file (file name begin with \'.\'), no need to do anything.");
        return E_INVALID_ARGUMENTS;
    }
    MediaLibraryCommand deleteCmd(OperationObject::FILESYSTEM_ASSET, OperationType::DELETE);
    if (DeleteInfoByPathInDb(deleteCmd, srcPath) != E_SUCCESS) {
        MEDIA_ERR_LOG("Delete rows for the old path failed");
    }
    return E_SUCCESS;
}

int32_t MediaLibraryObjectUtils::ProcessHiddenDir(const string &dstDirName, const string &srcDirPath)
{
    if (dstDirName.empty() || srcDirPath.empty() || dstDirName.at(0) != '.') {
        MEDIA_INFO_LOG("Not a hidden dir(name begin with \'.\'), no need to do anything.");
        return E_INVALID_PATH;
    }
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr!");
        return E_HAS_DB_ERROR;
    }

    MediaLibraryCommand deleteCmd(OperationObject::FILESYSTEM_ASSET, OperationType::DELETE);
    string dstAlbumPathPrefix = srcDirPath.back() != '/' ? (srcDirPath + "/") : srcDirPath;
    deleteCmd.GetAbsRdbPredicates()->BeginsWith(MEDIA_DATA_DB_FILE_PATH, dstAlbumPathPrefix);
    deleteCmd.GetAbsRdbPredicates()->Or()->EqualTo(MEDIA_DATA_DB_FILE_PATH, srcDirPath);

    int32_t deletedRows = -1;
    if (uniStore->Delete(deleteCmd, deletedRows) != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Delete src dir in database failed!");
        return E_HAS_DB_ERROR;
    }
    return E_SUCCESS;
}

int32_t MediaLibraryObjectUtils::UpdateDateModified(const string &dirPath)
{
    if (dirPath.empty()) {
        MEDIA_ERR_LOG("Path is empty, update failed!");
        return E_INVALID_PATH;
    }

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::UPDATE);
    ValuesBucket valuesBucket;
    int64_t dateModified = MediaFileUtils::GetAlbumDateModified(dirPath);
#ifdef MEDIALIBRARY_COMPATIBILITY
    if (dateModified == 0) {
        return 0;
    }
#endif
    valuesBucket.PutLong(MEDIA_DATA_DB_DATE_MODIFIED, dateModified);

    cmd.SetValueBucket(valuesBucket);

    return ModifyInfoByPathInDb(cmd, dirPath);
}

unique_ptr<FileAsset> MediaLibraryObjectUtils::GetFileAssetByPredicates(const NativeRdb::AbsRdbPredicates &predicates,
    const vector<string> &columns)
{
    auto absResultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    if (absResultSet == nullptr) {
        return nullptr;
    }
    auto resultSetBridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(absResultSet);
    auto resultSet = make_shared<DataShare::DataShareResultSet>(resultSetBridge);
    auto fetchResult = make_unique<FetchResult<FileAsset>>(move(resultSet));
    return fetchResult->GetFirstObject();
}

shared_ptr<FileAsset> MediaLibraryObjectUtils::GetFileAssetFromId(const string &id, const string &networkId)
{
    if ((id.empty()) || (!MediaLibraryDataManagerUtils::IsNumber(id)) || (stoi(id) == -1)) {
        MEDIA_ERR_LOG("Id for the path is incorrect: %{private}s", id.c_str());
        return nullptr;
    }

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::QUERY, networkId);
    cmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_ID, id);

    auto resultSet = QueryWithCondition(cmd, {});
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to obtain file asset from database");
        return nullptr;
    }

    shared_ptr<FetchResult<FileAsset>> fetchFileResult = make_shared<FetchResult<FileAsset>>();
    if (fetchFileResult == nullptr) {
        MEDIA_ERR_LOG("Failed to obtain fetch file result");
        return nullptr;
    }
    fetchFileResult->SetNetworkId(networkId);
    return fetchFileResult->GetObjectFromRdb(resultSet, 0);
}

shared_ptr<FileAsset> MediaLibraryObjectUtils::GetFileAssetFromUri(const string &uriStr)
{
    string id = MediaFileUtils::GetIdFromUri(uriStr);
    string networkId = MediaFileUtils::GetNetworkIdFromUri(uriStr);

    return GetFileAssetFromId(id, networkId);
}

void MediaLibraryObjectUtils::GetDefaultRelativePath(const int32_t mediaType, string &relativePath)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_DIR, OperationType::QUERY);
    cmd.GetAbsRdbPredicates()->EqualTo(DIRECTORY_DB_MEDIA_TYPE, to_string(mediaType));

    auto resultSet = QueryWithCondition(cmd, {});
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to obtain file asset from database, mediaType: %{public}d", static_cast<int>(mediaType));
        return;
    }

    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        relativePath = get<string>(ResultSetUtils::GetValFromColumn(DIRECTORY_DB_DIRECTORY, resultSet, TYPE_STRING));
    }
}

string GetRelativePathFromFilePath(const string &path)
{
    string relativePath;
    if (path.length() > ROOT_MEDIA_DIR.length()) {
        relativePath = path.substr(ROOT_MEDIA_DIR.length());
    }
    size_t pos = relativePath.rfind('/');
    if (pos != string::npos) {
        relativePath = relativePath.substr(0, pos + 1);
    }
    return relativePath;
}

int32_t MediaLibraryObjectUtils::UpdateFileInfoInDb(MediaLibraryCommand &cmd, const string &dstPath,
    const int32_t &bucketId, const string &bucketName)
{
    MEDIA_DEBUG_LOG("enter, dstPath: %{private}s,", dstPath.c_str());
    if (dstPath.empty()) {
        MEDIA_ERR_LOG("Input argument is empty.");
        return E_INVALID_PATH;
    }

    // dispName doesn't be used, maybe forget
    size_t found = dstPath.rfind("/");
    string dispName;
    if (found != string::npos) {
        dispName = dstPath.substr(found + 1);
    }

    struct stat statInfo;
    if (stat(dstPath.c_str(), &statInfo) != 0) {
        MEDIA_ERR_LOG("dstPath %{private}s is invalid. Modify failed!", dstPath.c_str());
        return E_HAS_FS_ERROR;
    }
    string fileId = cmd.GetOprnFileId();
    string mimeType = MimeTypeUtils::GetMimeTypeFromExtension(ScannerUtils::GetFileExtension(dstPath));
    MediaType mediaType = MimeTypeUtils::GetMediaTypeFromMimeType(mimeType);
    string displayName = MediaLibraryDataManagerUtils::GetDisPlayNameFromPath(dstPath);
    ValuesBucket values;
    values.PutString(MEDIA_DATA_DB_NAME, displayName);
    values.PutString(MEDIA_DATA_DB_TITLE, MediaFileUtils::GetTitleFromDisplayName(displayName));
    values.PutString(MEDIA_DATA_DB_FILE_PATH, dstPath);
    values.PutString(MEDIA_DATA_DB_RELATIVE_PATH, GetRelativePathFromFilePath(dstPath));
    values.PutString(MEDIA_DATA_DB_BUCKET_NAME, bucketName);
    values.PutString(MEDIA_DATA_DB_MIME_TYPE, mimeType);
    values.PutString(MEDIA_DATA_DB_URI, MediaFileUtils::GetMediaTypeUri(mediaType));
    values.PutInt(MEDIA_DATA_DB_MEDIA_TYPE, mediaType);
    values.PutInt(MEDIA_DATA_DB_BUCKET_ID, bucketId);
    values.PutInt(MEDIA_DATA_DB_PARENT_ID, bucketId);
    cmd.SetValueBucket(values);

    return ModifyInfoByIdInDb(cmd, fileId);
}

string MediaLibraryObjectUtils::GetPathByIdFromDb(const string &id, const bool isDelete)
{
    return GetStringColumnByIdFromDb(id, MEDIA_DATA_DB_FILE_PATH, isDelete);
}

string MediaLibraryObjectUtils::GetStringColumnByIdFromDb(const string &id, const string &column, const bool isDelete)
{
    string value;
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr!");
        return value;
    }

    if ((id.empty()) || (!MediaLibraryDataManagerUtils::IsNumber(id)) || (stoi(id) == -1)) {
        MEDIA_ERR_LOG("Id for the path is incorrect or rdbStore is null");
        return value;
    }

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::QUERY);
    if (isDelete) {
        cmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_ID, id);
    } else {
        cmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_ID, id)->And()->
            EqualTo(MEDIA_DATA_DB_IS_TRASH, to_string(NOT_TRASHED));
    }
    vector<string> columns;
    columns.push_back(column);

    auto queryResultSet = uniStore->Query(cmd, columns);
    CHECK_AND_RETURN_RET_LOG(queryResultSet != nullptr, value, "Failed to obtain value from database");

    auto ret = queryResultSet->GoToFirstRow();
    CHECK_AND_RETURN_RET_LOG(ret == 0, value, "Failed to shift at first row");
    int32_t columnIndex = 0;
    ret = queryResultSet->GetColumnIndex(column, columnIndex);
    CHECK_AND_RETURN_RET_LOG(ret == 0, value, "Failed to obtain column index");

    ret = queryResultSet->GetString(columnIndex, value);
    CHECK_AND_RETURN_RET_LOG(ret == 0, value, "Failed to obtain value");

    return value;
}

int32_t MediaLibraryObjectUtils::GetIdByPathFromDb(const string &path)
{
    MEDIA_DEBUG_LOG("enter, path = %{private}s", path.c_str());
    if (path.empty()) {
        return E_INVALID_PATH;
    }
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr!");
        return E_HAS_DB_ERROR;
    }

    int32_t columnIndex = 0;
    string newPath = path;
    if (newPath.back() == '/') {
        newPath.pop_back();
    }
    int32_t fileId = E_INVALID_FILEID;

    vector<string> columns;
    columns.push_back(MEDIA_DATA_DB_ID);

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::QUERY);
    cmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_FILE_PATH, newPath)->And()->
        EqualTo(MEDIA_DATA_DB_IS_TRASH, to_string(NOT_TRASHED));
    auto queryResultSet = uniStore->Query(cmd, columns);
    CHECK_AND_RETURN_RET_LOG(queryResultSet != nullptr, fileId, "Failed to obtain path from database");

    auto ret = queryResultSet->GoToFirstRow();
    CHECK_AND_RETURN_RET(ret == 0, fileId);

    ret = queryResultSet->GetColumnIndex(MEDIA_DATA_DB_ID, columnIndex);
    CHECK_AND_RETURN_RET_LOG(ret == 0, fileId, "Failed to obtain column index");

    ret = queryResultSet->GetInt(columnIndex, fileId);
    CHECK_AND_RETURN_RET_LOG(ret == 0, fileId, "Failed to obtain file id");

    return fileId;
}

int32_t MediaLibraryObjectUtils::InsertInDb(MediaLibraryCommand &cmd)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr!");
        return E_HAS_DB_ERROR;
    }

    int64_t outRowId = E_HAS_DB_ERROR;
    int32_t result = uniStore->Insert(cmd, outRowId);
    if (result != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Insert operation failed. Result %{public}d. Deleted %{public}d",
            result, static_cast<int32_t>(outRowId));
        return E_HAS_DB_ERROR;
    }
    return static_cast<int32_t>(outRowId);
}

int32_t MediaLibraryObjectUtils::DeleteInfoByPathInDb(MediaLibraryCommand &cmd, const string &path)
{
    MEDIA_DEBUG_LOG("enter, path = %{private}s", path.c_str());
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr!");
        return E_HAS_DB_ERROR;
    }

    int32_t deletedRows = E_HAS_DB_ERROR;
    cmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_FILE_PATH, path);
    int32_t result = uniStore->Delete(cmd, deletedRows);
    if (result != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Delete operation failed. Result %{public}d. Deleted %{public}d", result, deletedRows);
        return E_HAS_DB_ERROR;
    }

    return E_SUCCESS;
}

int32_t MediaLibraryObjectUtils::DeleteInfoByIdInDb(MediaLibraryCommand &cmd, const string &fileId)
{
    MEDIA_DEBUG_LOG("enter, fileId = %{private}s", fileId.c_str());
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr!");
        return E_HAS_DB_ERROR;
    }

    string strDeleteCondition = cmd.GetAbsRdbPredicates()->GetWhereClause();
    if (strDeleteCondition.empty()) {
        string strRow = fileId.empty() ? cmd.GetOprnFileId() : fileId;
        if (strRow.empty() || !MediaLibraryDataManagerUtils::IsNumber(strRow)) {
            MEDIA_ERR_LOG("MediaLibraryObjectUtils DeleteFile: Index not digit");
            return E_INVALID_FILEID;
        }
        cmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_ID, strRow);
    }

    int32_t deletedRows = E_HAS_DB_ERROR;
    int32_t result = uniStore->Delete(cmd, deletedRows);
    if (result != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Delete operation failed. Result %{public}d. Deleted %{public}d", result, deletedRows);
    }

    return deletedRows;
}

int32_t MediaLibraryObjectUtils::ModifyInfoByPathInDb(MediaLibraryCommand &cmd, const string &path)
{
    MEDIA_DEBUG_LOG("enter, path = %{private}s", path.c_str());
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr!");
        return E_HAS_DB_ERROR;
    }

    cmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_FILE_PATH, path);
    int32_t updatedRows = E_HAS_DB_ERROR;
    int32_t result = uniStore->Update(cmd, updatedRows);
    if (result != NativeRdb::E_OK || updatedRows <= 0) {
        MEDIA_ERR_LOG("Update operation failed. Result %{public}d. Updated %{public}d", result, updatedRows);
    }

    return updatedRows;
}

int32_t MediaLibraryObjectUtils::ModifyInfoByIdInDb(MediaLibraryCommand &cmd, const string &fileId)
{
    MEDIA_DEBUG_LOG("enter, fileId = %{private}s", fileId.c_str());
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr!");
        return E_HAS_DB_ERROR;
    }

    string strDeleteCondition = cmd.GetAbsRdbPredicates()->GetWhereClause();
    if (strDeleteCondition.empty()) {
        string strRow = fileId.empty() ? cmd.GetOprnFileId() : fileId;
        if (strRow.empty() || !MediaLibraryDataManagerUtils::IsNumber(strRow) || (stoi(strRow) == -1)) {
            MEDIA_ERR_LOG("DeleteFile: Index not digit");
            return E_INVALID_FILEID;
        }
        cmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_ID, strRow);
    }

    int32_t updatedRows = E_HAS_DB_ERROR;
    int32_t result = uniStore->Update(cmd, updatedRows);
    if (result != NativeRdb::E_OK || updatedRows <= 0) {
        MEDIA_ERR_LOG("Update operation failed. Result %{public}d. Updated %{public}d", result, updatedRows);
    }

    return updatedRows;
}

shared_ptr<ResultSet> MediaLibraryObjectUtils::QueryWithCondition(MediaLibraryCommand &cmd,
    const vector<string> &columns, const string &conditionColumn)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr!");
        return nullptr;
    }

    string strQueryCondition = cmd.GetAbsRdbPredicates()->GetWhereClause();
    if (strQueryCondition.empty()) {
        if (conditionColumn.empty()) {
            return uniStore->QuerySql("SELECT * FROM " + cmd.GetTableName());
        }
        string strFileId = cmd.GetOprnFileId();
        if (strFileId.empty()) {
            MEDIA_ERR_LOG("Get file id from uri or valuebucket failed!");
            return nullptr;
        }
        cmd.GetAbsRdbPredicates()->EqualTo(conditionColumn, strFileId);
    }

    return uniStore->Query(cmd, columns);
}

bool MediaLibraryObjectUtils::IsColumnValueExist(const string &value, const string &column)
{
    if (column.empty()) {
        MEDIA_ERR_LOG("Empty column param");
        return false;
    }
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr!");
        return false;
    }

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::QUERY);
    cmd.GetAbsRdbPredicates()->EqualTo(column, value);
    vector<string> columns;
    columns.push_back(column);
    auto queryResultSet = uniStore->Query(cmd, columns);
    if (queryResultSet != nullptr) {
        int32_t count = 0;
        queryResultSet->GetRowCount(count);
        if (count > 0) {
            return true;
        }
    }
    return false;
}

bool MediaLibraryObjectUtils::IsAssetExistInDb(const int32_t id, const bool isIncludeTrash)
{
    if (id <= 0) {
        MEDIA_ERR_LOG("Invalid id param");
        return false;
    }
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::QUERY);
    if (isIncludeTrash) {
        cmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_ID, to_string(id));
    } else {
        cmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_ID, to_string(id))->And()->
            EqualTo(MEDIA_DATA_DB_IS_TRASH, to_string(NOT_TRASHED));
    }
    vector<string> columns;
    auto queryResultSet = QueryWithCondition(cmd, columns);
    if (queryResultSet != nullptr && queryResultSet->GoToNextRow() == NativeRdb::E_OK) {
        return true;
    }
    return false;
}

bool MediaLibraryObjectUtils::IsFileExistInDb(const string &path)
{
    if (path.empty()) {
        MEDIA_ERR_LOG("path is incorrect");
        return false;
    }

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::QUERY);
    cmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_FILE_PATH, path)
        ->And()->EqualTo(MEDIA_DATA_DB_IS_TRASH, to_string(NOT_TRASHED));

    vector<string> columns;
    columns.push_back(MEDIA_DATA_DB_FILE_PATH);
    auto queryResultSet = QueryWithCondition(cmd, columns);
    if (queryResultSet != nullptr && queryResultSet->GoToNextRow() == NativeRdb::E_OK) {
        return true;
    }

    return false;
}

bool MediaLibraryObjectUtils::CheckUriPending(const std::string &uri)
{
    if (!uri.empty()) {
        size_t positon = uri.find_first_of('?');
        string tempUri = uri.substr(0, positon);
        auto fileAsset = GetFileAssetFromUri(tempUri);
        if ((fileAsset != nullptr) && (fileAsset->GetTimePending() != 0)) {
            return true;
        }
    }
    return false;
}

int32_t MediaLibraryObjectUtils::CopyAsset(const shared_ptr<FileAsset> &srcFileAsset,
    const string &relativePath)
{
    if (srcFileAsset == nullptr) {
        MEDIA_ERR_LOG("Failed to obtain path from Database");
        return E_INVALID_URI;
    }
    string srcPath = MediaFileUtils::UpdatePath(srcFileAsset->GetPath(), srcFileAsset->GetUri());
    string fileId = MediaFileUtils::GetIdFromUri(srcFileAsset->GetUri());
    int32_t srcFd = OpenAsset(srcPath, MEDIA_FILEMODE_READWRITE, fileId);
    // dest asset
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    ValuesBucket values;
    values.PutString(MEDIA_DATA_DB_RELATIVE_PATH, relativePath);
    string displayName = srcFileAsset->GetDisplayName();
    values.PutString(MEDIA_DATA_DB_NAME, displayName);
    values.PutInt(MEDIA_DATA_DB_MEDIA_TYPE, srcFileAsset->GetMediaType());
    cmd.SetValueBucket(values);
    int32_t outRow = CreateFileObj(cmd);
    while (outRow == E_FILE_EXIST) {
        displayName = displayName + ASSET_RECYCLE_SUFFIX;
        values.PutString(MEDIA_DATA_DB_NAME, displayName);
        outRow = CreateFileObj(cmd);
    }
    if (outRow < 0) {
        MEDIA_ERR_LOG("Failed to obtain CreateFileObj");
        CloseFileById(srcFileAsset->GetId());
        return outRow;
    }
    shared_ptr<FileAsset> destFileAsset = GetFileAssetFromId(to_string(outRow));
    if (destFileAsset == nullptr) {
        MEDIA_ERR_LOG("Failed to obtain path from Database");
        CloseFileById(srcFileAsset->GetId());
        return E_INVALID_URI;
    }
    string destPath = MediaFileUtils::UpdatePath(destFileAsset->GetPath(), destFileAsset->GetUri());
    string destFileId = MediaFileUtils::GetIdFromUri(destFileAsset->GetUri());
    int32_t destFd = OpenAsset(destPath, MEDIA_FILEMODE_READWRITE, destFileId);
    return CopyAssetByFd(srcFd, srcFileAsset->GetId(), destFd, outRow);
}

int32_t MediaLibraryObjectUtils::CopyAssetByFd(int32_t srcFd, int32_t srcId, int32_t destFd, int32_t destId)
{
    struct stat statSrc;
    if (fstat(srcFd, &statSrc) == -1) {
        CloseFileById(srcId);
        CloseFileById(destId);
        MEDIA_ERR_LOG("File get stat failed, %{public}d", errno);
        return E_FILE_OPER_FAIL;
    }

    if (sendfile(destFd, srcFd, nullptr, statSrc.st_size) == -1) {
        CloseFileById(srcId);
        CloseFileById(destId);
        MEDIA_ERR_LOG("copy file fail %{public}d ", errno);
        return E_FILE_OPER_FAIL;
    }
    CloseFileById(srcId);
    CloseFileById(destId);
    return destId;
}

void MediaLibraryObjectUtils::CloseFileById(int32_t fileId)
{
    ValuesBucket values;
    values.PutInt(MEDIA_DATA_DB_ID, fileId);
    MediaLibraryCommand closeCmd(OperationObject::FILESYSTEM_ASSET, OperationType::CLOSE, values);
    CloseFile(closeCmd);
}

int32_t MediaLibraryObjectUtils::GetFileResult(shared_ptr<NativeRdb::ResultSet> &resultSet,
    int count, const string &relativePath, const string &displayName)
{
    shared_ptr<FetchResult<FileAsset>> fetchFileResult = make_shared<FetchResult<FileAsset>>();
    int errCode = E_SUCCESS;
    for (int32_t row = 0; row < count; row++) {
        unique_ptr<FileAsset> fileAsset = fetchFileResult->GetObjectFromRdb(resultSet, row);
        if (fileAsset == nullptr) {
            MEDIA_ERR_LOG("get fileAsset failed");
            continue;
        }
        if (fileAsset->GetMediaType() == MEDIA_TYPE_ALBUM) {
            errCode = CopyDir(move(fileAsset), relativePath + displayName + "/");
            CHECK_AND_RETURN_RET_LOG(errCode > 0, errCode, "failed to copy dir");
        } else {
            errCode = CopyAsset(move(fileAsset), relativePath + displayName + "/");
            CHECK_AND_RETURN_RET_LOG(errCode > 0, errCode, "failed to copy asset");
        }
    }
    return errCode;
}

int32_t MediaLibraryObjectUtils::CopyDir(const shared_ptr<FileAsset> &srcDirAsset,
    const string &relativePath)
{
    if (srcDirAsset == nullptr) {
        MEDIA_ERR_LOG("Failed to obtain path from Database");
        return E_INVALID_URI;
    }
    ValuesBucket values;
    values.PutString(MEDIA_DATA_DB_NAME, ".nofile");
    values.PutInt(MEDIA_DATA_DB_MEDIA_TYPE, MEDIA_TYPE_NOFILE);
    string displayName = srcDirAsset->GetDisplayName();
    values.PutString(MEDIA_DATA_DB_RELATIVE_PATH, relativePath + displayName + "/");
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    cmd.SetValueBucket(values);
    int32_t outRow = CreateFileObj(cmd);
    while (outRow == E_FILE_EXIST) {
        displayName = displayName + ASSET_RECYCLE_SUFFIX;
        values.PutString(MEDIA_DATA_DB_RELATIVE_PATH, relativePath + displayName + "/");
        outRow = CreateFileObj(cmd);
    }
    if (outRow < 0) {
        MEDIA_ERR_LOG("Failed to obtain CreateFileObj");
        return outRow;
    }
    MediaLibraryCommand queryCmd(OperationObject::FILESYSTEM_ASSET, OperationType::QUERY);
    queryCmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_PARENT_ID, to_string(srcDirAsset->GetId()))->And()->
        EqualTo(MEDIA_DATA_DB_IS_TRASH, "0")->And()->NotEqualTo(MEDIA_DATA_DB_MEDIA_TYPE, to_string(MEDIA_TYPE_NOFILE));
    auto resultSet = QueryWithCondition(queryCmd, {});
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "query rdbstore failed");
    auto count = 0;
    auto ret = resultSet->GetRowCount(count);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_HAS_DB_ERROR, "get rdbstore failed");
    if (count == 0) {
        MEDIA_ERR_LOG("have no copy file");
        return E_SUCCESS;
    }

    int err = GetFileResult(resultSet, count, relativePath, displayName);
    resultSet.reset();
    if (err <= 0) {
        return err;
    }
    Uri srcUri(MEDIALIBRARY_DATA_URI + "/" + to_string(outRow));
    string srcUriString = srcUri.ToString();
    shared_ptr<FileAsset> srcAsset = MediaLibraryObjectUtils::GetFileAssetFromUri(srcUriString);
    if (srcAsset == nullptr) {
        MEDIA_ERR_LOG("Failed to obtain parentAsset from Database");
        return E_INVALID_URI;
    }
    return srcAsset->GetParent();
}

static string GetRelativePathFromPath(const string &path)
{
    string relativePath;
    if (path.find(ROOT_MEDIA_DIR) == 0) {
        relativePath = path.substr(ROOT_MEDIA_DIR.length());
    }
    auto pos = relativePath.rfind('/');
    if (pos == string::npos) {
        return "";
    }
    return relativePath.substr(0, pos + 1);
}

int32_t MediaLibraryObjectUtils::CheckDirExtension(const string &destFilePath)
{
    string relativePath = GetRelativePathFromPath(destFilePath);
    string displayName = MediaLibraryDataManagerUtils::GetDisPlayNameFromPath(destFilePath);
    return CheckDirExtension(relativePath, displayName);
}

static int32_t GetRootDirAssetByRelativePath(const string &relativePath, DirAsset &dirAsset)
{
    auto pos = relativePath.find('/');
    if (pos == string::npos) {
        return E_CHECK_EXTENSION_FAIL;
    }
    string rootDir = relativePath.substr(0, pos + 1);
    auto dirMap = MediaLibraryDataManager::GetDirQuerySetMap();
    if (dirMap.find(rootDir) == dirMap.end()) {
        return E_CHECK_EXTENSION_FAIL;
    }
    dirAsset = dirMap.at(rootDir);
    return E_SUCCESS;
}

void MediaLibraryObjectUtils::TryUpdateAnalysisProp(const std::string value)
{
    char buffer[16];
    GetParameter(HAS_DATA, NULL, buffer, sizeof(buffer));
    if (buffer == value) {
        return;
    }

    int ret = SetParameter(HAS_DATA, value.c_str());
    if (ret != 0) {
        MEDIA_ERR_LOG("Failed to UpdateAnalysisProp, result:%{public}d", ret);
    }
}

int32_t MediaLibraryObjectUtils::CheckDirExtension(const string &relativePath, const string &displayName)
{
    if (relativePath.empty() || displayName.empty()) {
        return E_INVALID_ARGUMENTS;
    }
    if (displayName.compare(MEDIA_NO_FILE) == 0) {
        return E_SUCCESS;
    }
    if (MediaFileUtils::CheckFileDisplayName(displayName) < 0) {
        MEDIA_ERR_LOG("Check File DisplayName failed, displayName: %{private}s", displayName.c_str());
        return E_FILE_NAME_INVALID;
    }
    DirAsset rootDirAsset;
    auto ret = GetRootDirAssetByRelativePath(relativePath, rootDirAsset);
    CHECK_AND_RETURN_RET_LOG(ret == E_SUCCESS, ret, "get root dir from relativePath failed, path: %{private}s",
        relativePath.c_str());
    string dirMediaTypes = rootDirAsset.GetMediaTypes();
    if (dirMediaTypes == DIR_ALL_TYPE_VALUES) {
        return E_SUCCESS;
    }
    string fileExtension = MediaFileUtils::GetExtensionFromPath(displayName);
    string fileMimeType = MimeTypeUtils::GetMimeTypeFromExtension(fileExtension);
    string fileMediaType = to_string(static_cast<int32_t>(MimeTypeUtils::GetMediaTypeFromMimeType(fileMimeType)));
    if (dirMediaTypes.find(fileMediaType) == string::npos) {
        MEDIA_ERR_LOG("CheckDirExtension failed, file extension: %{private}s, root dir media_type: %{private}s",
            fileExtension.c_str(), dirMediaTypes.c_str());
        return E_CHECK_EXTENSION_FAIL;
    }
    return E_SUCCESS;
}

bool MediaLibraryObjectUtils::IsSmartAlbumExistInDb(const int32_t id)
{
    MediaLibraryCommand querySmartAlbumCmd(OperationObject::SMART_ALBUM, OperationType::QUERY);
    querySmartAlbumCmd.GetAbsRdbPredicates()->EqualTo(SMARTALBUM_DB_ID, to_string(id));
    auto queryResultSet = QuerySmartAlbum(querySmartAlbumCmd);
    if (queryResultSet != nullptr) {
        if (queryResultSet->GoToFirstRow() == NativeRdb::E_OK) {
            return true;
        }
    } else {
        MEDIA_ERR_LOG("QuerySmartAlbum failed");
    }
    return false;
}

bool MediaLibraryObjectUtils::IsParentSmartAlbum(const int32_t id, const bool isInclude)
{
    MediaLibraryCommand querySmartAlbumCmd(OperationObject::SMART_ALBUM_MAP, OperationType::QUERY);
    querySmartAlbumCmd.GetAbsRdbPredicates()->EqualTo(SMARTALBUMMAP_DB_ALBUM_ID, to_string(id));
    if (isInclude) {
        auto queryResultSet = QuerySmartAlbum(querySmartAlbumCmd);
        if (queryResultSet != nullptr) {
            if (queryResultSet->GoToFirstRow() != NativeRdb::E_OK) {
                return true;
            }
            queryResultSet.reset();
        } else {
            MEDIA_ERR_LOG("QuerySmartAlbum failed");
            return false;
        }
    }
    querySmartAlbumCmd.GetAbsRdbPredicates()->IsNotNull(SMARTALBUMMAP_DB_CHILD_ALBUM_ID);
    auto queryResultSet = QuerySmartAlbum(querySmartAlbumCmd);
    if (queryResultSet != nullptr) {
        if (queryResultSet->GoToFirstRow() == NativeRdb::E_OK) {
            return true;
        }
    }
    return false;
}

shared_ptr<ResultSet> MediaLibraryObjectUtils::QuerySmartAlbum(MediaLibraryCommand &cmd)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("UniStore is nullptr");
        return nullptr;
    }
    vector<string> columns;
    return uniStore->Query(cmd, columns);
}
} // namespace Media
} // namespace OHOS

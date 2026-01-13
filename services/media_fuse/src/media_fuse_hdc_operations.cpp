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

#include "media_fuse_hdc_operations.h"
#include <fcntl.h>
#define FUSE_USE_VERSION FUSE_MAKE_VERSION(3, 17)
#include <fuse.h>
#include <sys/utsname.h>
#include "media_log.h"
#include "medialibrary_errno.h"
#include "media_column.h"
#include "rdb_utils.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_ptp_operations.h"
#include "medialibrary_photo_operations.h"
#include "result_set_utils.h"

namespace OHOS {
namespace Media {
const std::string FUSE_ROOT_MEDIA_DIR = "/storage/cloud/files/Photo/";
const std::string FUSE_OPEN_PHOTO_PRE = "/Photo";
const std::string FUSE_LOCAL_MEDIA_DIR = "/storage/media/local/files/Photo/";
const std::string FUSE_URI_PREFIX = "file://media";
const std::string HIDDEN_ALBUM = ".hiddenAlbum";
const std::string VIDEO_EXTENSION = "mp4";
const std::string FIXED_PHOTO_ALBUM = "DeveloperAlbum";

static constexpr int32_t HDC_FIRST_ARGS = 0;
static constexpr int32_t HDC_SECOND_ARGS = 1;
static constexpr uid_t CUSTOM_UID = 1008;
static constexpr mode_t DIR_PERMISSION = 0777;
static constexpr mode_t FILE_PERMISSION = 0664;
static constexpr off_t DIR_DEFAULT_SIZE = 3440;
static constexpr int64_t MILLISECONDS_THRESHOLD = 1000000000000LL;
static constexpr int64_t MILLISECONDS_PER_SECOND = 1000LL;
static constexpr int32_t FUSE_MAX_PAGE_NUM = 22;

time_t MediaFuseHdcOperations::GetAlbumMTime(const std::shared_ptr<NativeRdb::ResultSet>& resultSet)
{
    int64_t dateModified = GetInt64Val(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, resultSet);
    int64_t dateAdded = GetInt64Val(PhotoAlbumColumns::ALBUM_DATE_ADDED, resultSet);
    int64_t mtimeRaw = (dateModified > 0) ? dateModified : dateAdded;
    if (mtimeRaw > MILLISECONDS_THRESHOLD) {
        mtimeRaw /= MILLISECONDS_PER_SECOND;
    }
    return (mtimeRaw > 0) ? static_cast<time_t>(mtimeRaw) : time(nullptr);
}

void MediaFuseHdcOperations::FillDirStat(struct stat *stbuf, time_t mtime, const std::string& uniqueKey)
{
    if (!stbuf) {
        return;
    }
    *stbuf = (struct stat) {
        .st_mode = S_IFDIR | DIR_PERMISSION,
        .st_nlink = 2,
        .st_uid = CUSTOM_UID,
        .st_gid = CUSTOM_UID,
        .st_size = DIR_DEFAULT_SIZE,
    };
    stbuf->st_mtime = mtime ? mtime : time(nullptr);
    stbuf->st_ctime = stbuf->st_mtime;
    stbuf->st_atime = stbuf->st_mtime;
    stbuf->st_ino = std::hash<std::string>{}(uniqueKey);
}

int32_t MediaFuseHdcOperations::GetArgs(const std::string &path, std::vector<std::string> &parts)
{
    std::stringstream ss(path);
    std::string part;
    while (getline(ss, part, '/')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    return E_SUCCESS;
}

bool MediaFuseHdcOperations::IsImageOrVideoFile(const std::string &fileName)
{
    auto mediaType = MediaFileUtils::GetMediaType(fileName);
    return (mediaType == Media::MediaType::MEDIA_TYPE_IMAGE) ||
        (mediaType == Media::MediaType::MEDIA_TYPE_VIDEO);
}

int32_t MediaFuseHdcOperations::GetPathFromDisplayname(
    const std::string &displayName, int albumId, std::string &filePath)
{
    if (displayName.empty()) {
        MEDIA_ERR_LOG("Displayname is empty.");
        return E_ERR;
    }
    NativeRdb::RdbPredicates rdbPredicate(PhotoColumn::PHOTOS_TABLE);
    rdbPredicate.EqualTo(MediaColumn::MEDIA_NAME, displayName);
    rdbPredicate.And()->EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, albumId);
    rdbPredicate.And()->EqualTo(MediaColumn::MEDIA_DATE_TRASHED, to_string(0));
    rdbPredicate.And()->EqualTo(MediaColumn::MEDIA_HIDDEN, to_string(0));
    std::vector<std::string> positions = {to_string(static_cast<int32_t>(PhotoPositionType::LOCAL)),
        to_string(static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD))};
    rdbPredicate.And()->In(PhotoColumn::PHOTO_POSITION, positions);
    std::vector<std::string> columns;
    columns.push_back(MediaColumn::MEDIA_FILE_PATH);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to get filePath from db");
        return E_ERR;
    }
    filePath = "";
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        filePath = MediaLibraryRdbStore::GetString(resultSet, MediaColumn::MEDIA_FILE_PATH);
    }
    resultSet->Close();
    MEDIA_INFO_LOG("get filePath from db, filePath = %{private}s", filePath.c_str());
    return E_SUCCESS;
}

int32_t MediaFuseHdcOperations::GetAlbumIdFromAlbumName(const std::string &name, int32_t &albumId)
{
    if (name.empty()) {
        MEDIA_ERR_LOG("AlbumName is empty.");
        return E_ERR;
    }
    NativeRdb::RdbPredicates rdbPredicate(PhotoAlbumColumns::TABLE);
    rdbPredicate.EqualTo(PhotoAlbumColumns::ALBUM_NAME, name);
    rdbPredicate.And()->IsNotNull(MEDIA_DATA_DB_ALBUM_NAME);
    rdbPredicate.And()->NotEqualTo(MEDIA_DATA_DB_ALBUM_NAME, HIDDEN_ALBUM);
    std::vector<std::string> columns;
    columns.push_back(PhotoAlbumColumns::ALBUM_ID);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query albumName and id from db");
        return E_ERR;
    }
    albumId = -1;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        albumId = MediaLibraryRdbStore::GetInt(resultSet, PhotoAlbumColumns::ALBUM_ID);
        resultSet->Close();
        return E_SUCCESS;
    }
    MEDIA_INFO_LOG("get albumId from db, albumId = %{private}d", albumId);
    return E_SUCCESS;
}

int32_t MediaFuseHdcOperations::Parse(
    const std::string &path, int32_t &albumId, std::string &filePath, std::string &displayName)
{
    std::vector<std::string> args;
    int32_t res = GetArgs(path, args);
    CHECK_AND_RETURN_RET_LOG(res == E_SUCCESS, E_ERR, "GetArgs fail.");
    if (args.size() < HDC_SECOND_ARGS) {
        MEDIA_ERR_LOG("invalid path.");
        return E_ERR;
    }
    displayName = args[args.size() - 1];
    if (args.size() == HDC_SECOND_ARGS) {
        res = GetAlbumIdFromAlbumName(FIXED_PHOTO_ALBUM, albumId);
        CHECK_AND_RETURN_RET_LOG(res == E_SUCCESS, E_ERR,
            "GetAlbumIdFromAlbumName fail, use FIXED_PHOTO_ALBUM: %{private}s", FIXED_PHOTO_ALBUM.c_str());
    } else {
        res = GetAlbumIdFromAlbumName(args[HDC_FIRST_ARGS], albumId);
        CHECK_AND_RETURN_RET_LOG(res == E_SUCCESS, E_ERR,
            "GetAlbumIdFromAlbumName fail, albumName: %{private}s", args[HDC_FIRST_ARGS].c_str());
    }
    res = GetPathFromDisplayname(displayName, albumId, filePath);
    CHECK_AND_RETURN_RET_LOG(res == E_SUCCESS, E_ERR, "GetPathFromDisplayname fail");
    return res;
}

int32_t MediaFuseHdcOperations::ExtractFileNameAndExtension(
    const std::string &input, std::string &outName, std::string &outExt)
{
    if (input.empty()) {
        return E_ERR;
    }
    std::string fileName;
    size_t lastSlashPos = input.find_last_of('/');
    if (lastSlashPos == std::string::npos) {
        fileName = input;
    } else {
        fileName = input.substr(lastSlashPos + 1);
        if (fileName.empty()) {
            return E_ERR;
        }
    }
    size_t lastDotPos = fileName.find_last_of('.');
    if (lastDotPos == std::string::npos || lastDotPos == 0 || lastDotPos == fileName.length() - 1) {
        outName = fileName;
        outExt = "";
        return E_SUCCESS;
    }
    outName = fileName.substr(0, lastDotPos);
    outExt = fileName.substr(lastDotPos + 1);
    return E_SUCCESS;
}

bool MediaFuseHdcOperations::IsMovingPhoto(int32_t subtype, int32_t effectMode)
{
    return (subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ||
        effectMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY));
}

int32_t MediaFuseHdcOperations::HandleMovingPhoto(std::string &filePath, std::string &displayName, int32_t albumId)
{
    std::string title;
    std::string ext;
    int32_t res = ExtractFileNameAndExtension(displayName, title, ext);
    CHECK_AND_RETURN_RET_LOG(res == E_SUCCESS, E_ERR, "ExtractFileNameAndExtension fail");
    NativeRdb::RdbPredicates rdbPredicate(PhotoColumn::PHOTOS_TABLE);
    rdbPredicate.EqualTo(MediaColumn::MEDIA_TITLE, title);
    rdbPredicate.And()->EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, albumId);
    rdbPredicate.And()->EqualTo(MediaColumn::MEDIA_DATE_TRASHED, to_string(0));
    rdbPredicate.And()->EqualTo(MediaColumn::MEDIA_HIDDEN, to_string(0));
    std::vector<std::string> positions = {to_string(static_cast<int32_t>(PhotoPositionType::LOCAL)),
        to_string(static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD))};
    rdbPredicate.And()->In(PhotoColumn::PHOTO_POSITION, positions);
    std::vector<std::string> columns = {
        PhotoColumn::MEDIA_NAME,
        PhotoColumn::PHOTO_SUBTYPE,
        PhotoColumn::MOVING_PHOTO_EFFECT_MODE
    };
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query date from db");
        return E_ERR;
    }
    if (resultSet->GoToNextRow() != NativeRdb::E_OK) {
        resultSet->Close();
        MEDIA_ERR_LOG("Failed to query subtype from db");
        return E_NO_SUCH_FILE;
    }
    int32_t subtype = MediaLibraryRdbStore::GetInt(resultSet, PhotoColumn::PHOTO_SUBTYPE);
    int32_t effectMode = MediaLibraryRdbStore::GetInt(resultSet, PhotoColumn::MOVING_PHOTO_EFFECT_MODE);
    displayName = MediaLibraryRdbStore::GetString(resultSet, PhotoColumn::MEDIA_NAME);
    if (!IsMovingPhoto(subtype, effectMode)) {
        resultSet->Close();
        return E_ERR;
    }
    resultSet->Close();
    return E_SUCCESS;
}

int32_t MediaFuseHdcOperations::HandleFstat(const struct fuse_file_info *fi, struct stat *stbuf)
{
    int32_t res = fstat(static_cast<int32_t>(fi->fh), stbuf);
    if (res < 0) {
        MEDIA_ERR_LOG("fstat failed, res = %{public}d", res);
        return -errno;
    }
    return E_SUCCESS;
}

int32_t MediaFuseHdcOperations::HandleRootOrPhoto(const char *path, struct stat *stbuf)
{
    if (strcmp(path, "/") == 0) {
        FillDirStat(stbuf);
        return E_SUCCESS;
    }
    return E_ERR;
}

int32_t MediaFuseHdcOperations::HandleDirStat(const int32_t &albumId, struct stat *stbuf)
{
    NativeRdb::RdbPredicates rdbPredicate(PhotoAlbumColumns::TABLE);
    rdbPredicate.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    rdbPredicate.Limit(1);
    std::vector<std::string> columns = {
        PhotoAlbumColumns::ALBUM_NAME,
        PhotoAlbumColumns::ALBUM_DATE_MODIFIED,
        PhotoAlbumColumns::ALBUM_DATE_ADDED
    };
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query date from db");
        return E_ERR;
    }
    if (resultSet->GoToNextRow() != NativeRdb::E_OK) {
        resultSet->Close();
        MEDIA_ERR_LOG("Failed to query albumId from db");
        return E_ERR;
    }
    std::string albumName = MediaLibraryRdbStore::GetString(resultSet, PhotoAlbumColumns::ALBUM_NAME);
    time_t mtime = GetAlbumMTime(resultSet);
    FillDirStat(stbuf, mtime, albumName);
    resultSet->Close();
    return E_SUCCESS;
}

int32_t MediaFuseHdcOperations::HandleLstat(const std::string &localPath, struct stat *stbuf)
{
    int32_t res = lstat(localPath.c_str(), stbuf);
    CHECK_AND_RETURN_RET_LOG(res == E_SUCCESS, E_ERR,
        "lstat fail localPath = %{private}s, errno = %{public}d.", localPath.c_str(), errno);
    stbuf->st_mode = stbuf->st_mode | 0x6;
    return E_SUCCESS;
}

int32_t MediaFuseHdcOperations::HandlePhotoPath(
    const std::string &inputPath, int32_t &albumId, std::string &localPath, struct stat *stbuf)
{
    int32_t res = -1;
    if (IsImageOrVideoFile(inputPath)) {
        res = GetAlbumIdFromAlbumName(FIXED_PHOTO_ALBUM, albumId);
        CHECK_AND_RETURN_RET_LOG(res == E_SUCCESS, E_ERR, "GetAlbumIdFromAlbumName fail");
        res = GetPathFromDisplayname(inputPath, albumId, localPath);
        CHECK_AND_RETURN_RET_LOG(res == E_SUCCESS, E_ERR, "GetPathFromDisplayname fail");
        if (localPath.empty()) {
            MEDIA_ERR_LOG("LocalPath is empty.");
            return -ENOENT;
        }
        res = HandleLstat(localPath, stbuf);
        CHECK_AND_RETURN_RET_LOG(res == E_SUCCESS, E_ERR, "HandleLstat fail");
        return E_SUCCESS;
    }
    res = GetAlbumIdFromAlbumName(inputPath, albumId);
    CHECK_AND_RETURN_RET_LOG(res == E_SUCCESS, E_ERR, "GetAlbumIdFromAlbumName fail");
    if (albumId <= 0) {
        MEDIA_ERR_LOG("not exist album %{private}s.", inputPath.c_str());
        return -ENOENT;
    }
    res = HandleDirStat(albumId, stbuf);
    if (res != E_SUCCESS) {
        MEDIA_ERR_LOG("HandleDirStat fail");
        return res;
    }
    return E_SUCCESS;
}

int32_t MediaFuseHdcOperations::HandleFilePath(
    const std::vector<std::string> &args, int32_t &albumId, std::string &localPath)
{
    int32_t res = GetAlbumIdFromAlbumName(args[HDC_FIRST_ARGS], albumId);
    CHECK_AND_RETURN_RET_LOG(res == E_SUCCESS, E_ERR, "GetAlbumIdFromAlbumName fail");
    res = GetPathFromDisplayname(args[HDC_SECOND_ARGS], albumId, localPath);
    CHECK_AND_RETURN_RET_LOG(res == E_SUCCESS, E_ERR, "GetPathFromDisplayname fail");
    std::string displayName = args[HDC_SECOND_ARGS];
    if (localPath.empty()) {
        res = HandleMovingPhoto(localPath, displayName, albumId);
        if (res != E_SUCCESS) {
            MEDIA_ERR_LOG("HandleMovingPhoto fail");
            return -ENOENT;
        }
        res = GetPathFromDisplayname(displayName, albumId, localPath);
        CHECK_AND_RETURN_RET_LOG(res == E_SUCCESS, E_ERR, "GetPathFromDisplayname fail");
        localPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(localPath);
    }
    return E_SUCCESS;
}

int32_t MediaFuseHdcOperations::ConvertToLocalPhotoPath(const std::string &inputPath, std::string &output)
{
    if (inputPath.empty() || inputPath.find(FUSE_ROOT_MEDIA_DIR) != 0) {
        MEDIA_ERR_LOG("ConvertToLocalPhotoPath inputPath err, filePath:%{private}s", inputPath.c_str());
        return E_ERR;
    }
    output = FUSE_LOCAL_MEDIA_DIR + inputPath.substr(FUSE_ROOT_MEDIA_DIR.length());
    return E_SUCCESS;
}

int32_t MediaFuseHdcOperations::CreateFd(const std::string &displayName, const int32_t &albumId, int32_t &fd)
{
    std::string title;
    std::string extension;
    int ret = ExtractFileNameAndExtension(displayName, title, extension);
    CHECK_AND_RETURN_RET_LOG(ret == E_SUCCESS, E_ERR, "ExtractFileNameAndExtension failed.");
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    auto mediaType = MediaFileUtils::GetMediaType(displayName);
    NativeRdb::ValuesBucket assetInfo;
    assetInfo.PutString(ASSET_EXTENTION, extension);
    if (mediaType == Media::MediaType::MEDIA_TYPE_IMAGE) {
        assetInfo.PutInt(MediaColumn::MEDIA_TYPE, MEDIA_TYPE_IMAGE);
    } else if (mediaType == Media::MediaType::MEDIA_TYPE_VIDEO) {
        assetInfo.PutInt(MediaColumn::MEDIA_TYPE, MEDIA_TYPE_VIDEO);
    } else {
        MEDIA_ERR_LOG("Unsupported format!");
        return E_ERR;
    }
    assetInfo.PutString(MediaColumn::MEDIA_TITLE, title);
    assetInfo.PutString(MediaColumn::MEDIA_NAME, displayName);
    assetInfo.PutInt(MediaColumn::MEDIA_TIME_PENDING, 0);
    if (albumId <= 0) {
        assetInfo.Put(MediaColumn::MEDIA_PACKAGE_NAME, FIXED_PHOTO_ALBUM);
    } else {
        assetInfo.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, albumId);
    }
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE, MediaLibraryApi::API_10);
    cmd.SetValueBucket(assetInfo);
    ret = MediaLibraryPhotoOperations::Create(cmd);
    if (ret < 0) {
        MEDIA_ERR_LOG("MediaLibraryPhotoOperations::Create failed, ret = %{public}d", ret);
        return E_ERR;
    }
    std::string fileUriStr = cmd.GetResult();
    Uri uri(fileUriStr);
    MediaLibraryCommand openLivePhotoCmd(uri, Media::OperationType::OPEN);
    fd = MediaLibraryPhotoOperations::Open(openLivePhotoCmd, "w");
    if (fd <= 0) {
        string fileId = MediaFileUtils::GetIdFromUri(uri.ToString());
        NativeRdb::RdbPredicates rdbPredicate(PhotoColumn::PHOTOS_TABLE);
        rdbPredicate.EqualTo(MediaColumn::MEDIA_ID, fileId);
        int ret = MediaLibraryPtpOperations::DeletePtpPhoto(rdbPredicate);
        CHECK_AND_RETURN_RET_LOG(ret == E_SUCCESS, E_ERR, "DeletePtpPhoto failed.");
        return E_ERR;
    }
    MEDIA_INFO_LOG("CreateFd success, fd = %{private}d", fd);
    return E_SUCCESS;
}

int32_t MediaFuseHdcOperations::GetFileIdFromPath(const std::string &filePath, std::string &fileId)
{
    NativeRdb::RdbPredicates rdbPredicate(PhotoColumn::PHOTOS_TABLE);
    rdbPredicate.EqualTo(MediaColumn::MEDIA_FILE_PATH, filePath);
    std::vector<std::string> columns;
    columns.push_back(MediaColumn::MEDIA_ID);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query date from db");
        return E_ERR;
    }
    if (resultSet->GoToNextRow() != NativeRdb::E_OK) {
        resultSet->Close();
        MEDIA_ERR_LOG("Failed to query filePath from db");
        return E_ERR;
    }
    fileId = MediaLibraryRdbStore::GetString(resultSet, MediaColumn::MEDIA_ID);
    resultSet->Close();
    MEDIA_INFO_LOG("get fileId from db, fileId = %{private}s", fileId.c_str());
    return E_SUCCESS;
}

int32_t MediaFuseHdcOperations::UpdatePhotoRdb(const std::string &displayName, const std::string &filePath)
{
    if (displayName.find('/') != std::string::npos) {
        MEDIA_ERR_LOG("Invalid displayName: contains '/'. displayName = %{private}s", displayName.c_str());
        return E_ERR;
    }
    std::string title;
    std::string ext;
    std::string fileId;
    int32_t res = ExtractFileNameAndExtension(filePath, title, ext);
    CHECK_AND_RETURN_RET_LOG(res == E_SUCCESS, E_ERR, "ExtractFileNameAndExtension fail");
    res = GetFileIdFromPath(filePath, fileId);
    CHECK_AND_RETURN_RET_LOG(res == E_SUCCESS, E_ERR, "GetFileIdFromPath fail");
    std::string uri = FUSE_URI_PREFIX + FUSE_OPEN_PHOTO_PRE + "/" + fileId + "/" + title + "/" + displayName;
    MEDIA_INFO_LOG("UpdatePhotoRdb uri = %{private}s", uri.c_str());
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Failed to get rdbStore instance.");
        return E_HAS_DB_ERROR;
    }
    MediaLibraryCommand updatePendingCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    updatePendingCmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, fileId);
    NativeRdb::ValuesBucket values;
    int64_t pendingTime = UNCLOSE_FILE_TIMEPENDING;
    values.PutLong(MediaColumn::MEDIA_TIME_PENDING, pendingTime);
    updatePendingCmd.SetValueBucket(values);
    int32_t rowId = 0;
    int32_t result = rdbStore->Update(updatePendingCmd, rowId);
    if (result != NativeRdb::E_OK || rowId <= 0) {
        MEDIA_ERR_LOG("Update File pending failed. Result %{public}d.", result);
        return E_HAS_DB_ERROR;
    }
    MediaLibraryCommand closeCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CLOSE);
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutString(MEDIA_DATA_DB_URI, uri);
    closeCmd.SetValueBucket(valuesBucket);
    closeCmd.SetTableName(PhotoColumn::PHOTOS_TABLE);
    int32_t ret = MediaLibraryPhotoOperations::Close(closeCmd);
    if (ret < 0) {
        MEDIA_ERR_LOG("MediaLibraryPhotoOperations::Close failed ret = %{public}d.", ret);
        return E_HAS_DB_ERROR;
    }
    return E_SUCCESS;
}

int32_t MediaFuseHdcOperations::ScanFileByPath(const std::string &path)
{
    MEDIA_INFO_LOG("hdc write start, path=%{private}s.", path.c_str());
    if (path.empty()) {
        MEDIA_ERR_LOG("Invalid path");
        return -EINVAL;
    }
    int32_t albumId = -1;
    std::string filePath;
    std::string displayName;
    int32_t res = Parse(path, albumId, filePath, displayName);
    CHECK_AND_RETURN_RET_LOG(res == E_SUCCESS, E_ERR, "Parse fail");
    if (filePath.empty()) {
        res = HandleMovingPhoto(filePath, displayName, albumId);
        if (res != E_SUCCESS) {
            MEDIA_ERR_LOG("HandleMovingPhoto fail");
            return -ENOENT;
        }
        res = GetPathFromDisplayname(displayName, albumId, filePath);
        CHECK_AND_RETURN_RET_LOG(res == E_SUCCESS, E_ERR, "GetPathFromDisplayname fail");
    }
    res = UpdatePhotoRdb(displayName, filePath);
    CHECK_AND_RETURN_RET_LOG(res == E_SUCCESS, E_ERR, "UpdatePhotoRdb fail");
    return E_SUCCESS;
}

int32_t MediaFuseHdcOperations::ReadPhotoRootDir(void *buf, fuse_fill_dir_t filler, off_t offset)
{
    NativeRdb::RdbPredicates rdbPredicate(PhotoAlbumColumns::TABLE);
    rdbPredicate.IsNotNull(MEDIA_DATA_DB_ALBUM_NAME);
    rdbPredicate.NotEqualTo(MEDIA_DATA_DB_ALBUM_NAME, HIDDEN_ALBUM);
    rdbPredicate.GroupBy({ PhotoAlbumColumns::ALBUM_NAME });
    std::vector<std::string> columns = {
        PhotoAlbumColumns::ALBUM_NAME,
        PhotoAlbumColumns::ALBUM_DATE_ADDED,
        PhotoAlbumColumns::ALBUM_DATE_MODIFIED
    };
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query albumName and date from db");
        return E_ERR;
    }
    off_t curr_off = 0;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        if (curr_off < offset) {
            curr_off++;
            continue;
        }
        std::string albumName = MediaLibraryRdbStore::GetString(resultSet, PhotoAlbumColumns::ALBUM_NAME);
        time_t mtime = GetAlbumMTime(resultSet);
        struct stat st;
        FillDirStat(&st, mtime, albumName);
        off_t nextoff = curr_off + 1;
        if (filler(buf, albumName.c_str(), &st, nextoff, FUSE_FILL_DIR_PLUS)) {
            break;
        }
        curr_off++;
    }
    resultSet->Close();
    return E_SUCCESS;
}

std::string MediaFuseHdcOperations::JpgToMp4(const std::string& displayName)
{
    if (displayName.empty()) {
        MEDIA_ERR_LOG("Invalid displayName");
        return "";
    }
    size_t dotPos = displayName.find_last_of('.');
    std::string videoName = (dotPos != std::string::npos)
        ? displayName.substr(0, dotPos) + "." + VIDEO_EXTENSION
        : displayName + "." + VIDEO_EXTENSION;
    return videoName;
}

bool MediaFuseHdcOperations::FillDirectoryEntry(
    void *buf, fuse_fill_dir_t filler, const std::string &name, const std::string &fullPath, off_t nextoff)
{
    struct stat st;
    if (lstat(fullPath.c_str(), &st) == -1) {
        st.st_mode = S_IFREG | FILE_PERMISSION;
        st.st_nlink = 1;
        st.st_uid = CUSTOM_UID;
        st.st_gid = CUSTOM_UID;
        st.st_size = 0;
    }
    return filler(buf, name.c_str(), &st, nextoff, FUSE_FILL_DIR_PLUS);
}

std::shared_ptr<NativeRdb::ResultSet> MediaFuseHdcOperations::QueryAlbumPhotos(const int32_t &albumId)
{
    NativeRdb::RdbPredicates photoPred(PhotoColumn::PHOTOS_TABLE);
    photoPred.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, albumId);
    photoPred.And()->EqualTo(MediaColumn::MEDIA_HIDDEN, to_string(0));
    photoPred.And()->EqualTo(MediaColumn::MEDIA_DATE_TRASHED, to_string(0));
    photoPred.And()->EqualTo(MediaColumn::MEDIA_TIME_PENDING, to_string(0));
    std::vector<std::string> positions = {to_string(static_cast<int32_t>(PhotoPositionType::LOCAL)),
        to_string(static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD))};
    photoPred.And()->In(PhotoColumn::PHOTO_POSITION, positions);
    photoPred.GroupBy({ MediaColumn::MEDIA_NAME });
    std::vector<std::string> columns = {
        MediaColumn::MEDIA_NAME,
        MediaColumn::MEDIA_FILE_PATH,
        PhotoColumn::PHOTO_SUBTYPE,
        PhotoColumn::MOVING_PHOTO_EFFECT_MODE
    };
    return MediaLibraryRdbStore::Query(photoPred, columns);
}

int32_t MediaFuseHdcOperations::ReadAlbumDir(
    const std::string &inputPath, void* buf, fuse_fill_dir_t filler, off_t offset)
{
    std::string albumName = inputPath.substr(1);
    int32_t albumId;
    if (GetAlbumIdFromAlbumName(albumName, albumId) != E_SUCCESS) {
        MEDIA_ERR_LOG("Failed to get album ID for: %{private}s", albumName.c_str());
        return E_ERR;
    }
    auto resultSet = QueryAlbumPhotos(albumId);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "Failed to query photos in album");
    int32_t count = 0;
    off_t curr_off = 0;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        if (curr_off < offset) {
            curr_off++;
            continue;
        }
        std::string displayName = MediaLibraryRdbStore::GetString(resultSet, MediaColumn::MEDIA_NAME);
        int32_t subtype = MediaLibraryRdbStore::GetInt(resultSet, PhotoColumn::PHOTO_SUBTYPE);
        int32_t effectMode = MediaLibraryRdbStore::GetInt(resultSet, PhotoColumn::MOVING_PHOTO_EFFECT_MODE);
        std::string filePath = MediaLibraryRdbStore::GetString(resultSet, MediaColumn::MEDIA_FILE_PATH);
        std::string localPath;
        if (ConvertToLocalPhotoPath(filePath, localPath) != E_SUCCESS) {
            MEDIA_ERR_LOG("Failed to convert to local path: %{private}s", filePath.c_str());
            continue;
        }
        std::set<std::string> fileNames;
        std::string videoPath = "";
        fileNames.insert(displayName);
        if (IsMovingPhoto(subtype, effectMode)) {
            videoPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(localPath);
            std::string videoName = JpgToMp4(displayName);
            fileNames.insert(videoName);
        }
        if (++count >= FUSE_MAX_PAGE_NUM) {
            return E_SUCCESS;
        }
        for (const auto& name : fileNames) {
            std::string fullPath = (name == displayName) ? localPath : videoPath;
            off_t nextoff = (name == displayName) ? (++curr_off) : curr_off;
            if (FillDirectoryEntry(buf, filler, name, fullPath, nextoff)) {
                //filler is full
                return E_SUCCESS;
            }
        }
    }
    resultSet->Close();
    return E_SUCCESS;
}

int32_t MediaFuseHdcOperations::DeletePhotoByFilePath(const std::string &filePath)
{
    std::string fileId;
    int32_t res = GetFileIdFromPath(filePath, fileId);
    CHECK_AND_RETURN_RET_LOG(res == E_SUCCESS, E_ERR, "GetFileIdFromPath fail");

    NativeRdb::RdbPredicates rdbPredicate(PhotoColumn::PHOTOS_TABLE);
    rdbPredicate.EqualTo(MediaColumn::MEDIA_ID, fileId);
    rdbPredicate.And()->EqualTo(MediaColumn::MEDIA_DATE_TRASHED, to_string(0));
    int ret = MediaLibraryPtpOperations::DeletePtpPhoto(rdbPredicate);
    if (ret != 0) {
        MEDIA_ERR_LOG("Unlink failed");
        return E_ERR;
    }
    return E_SUCCESS;
}
} // namespace Media
} // namespace OHOS
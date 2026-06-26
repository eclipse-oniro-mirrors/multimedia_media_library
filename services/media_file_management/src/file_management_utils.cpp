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

#define MLOG_TAG "FileManagementUtils"
#include "file_management_utils.h"

#include <string>
#include <map>
#include <sys/stat.h>

#include "rdb_predicates.h"
#include "media_column.h"
#include "photo_album_column.h"
#include "medialibrary_rdbstore.h"
#include "result_set_utils.h"
#include "medialibrary_type_const.h"
#include "photo_album_upload_status_operation.h"
#include "album_accurate_refresh.h"
#include "asset_accurate_refresh.h"
#include "photo_file_utils.h"
#include "media_uri_utils.h"
#include "userfile_manager_types.h"
#include "settings_data_manager.h"

namespace OHOS::Media {
using namespace AccurateRefresh;
const std::string PHOTO_CLOUD_PATH_URI = "/storage/cloud/files/";
const std::string PHOTO_MEDIA_PATH_URI = "/storage/media/local/files/";
const std::vector<string> PHOTO_QUERY_COLUMN = {MediaColumn::MEDIA_ID, MediaColumn::MEDIA_TYPE,
    MediaColumn::MEDIA_TITLE, PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::PHOTO_POSITION, MediaColumn::MEDIA_FILE_PATH,
    PhotoColumn::PHOTO_STORAGE_PATH, PhotoColumn::PHOTO_FILE_SOURCE_TYPE, PhotoColumn::MEDIA_NAME,
    PhotoColumn::PHOTO_BURST_KEY, MediaColumn::MEDIA_SIZE, MediaColumn::MEDIA_DATE_TRASHED, MediaColumn::MEDIA_HIDDEN,
    PhotoColumn::PHOTO_DIRTY, PhotoColumn::PHOTO_IS_TEMP, MediaColumn::MEDIA_TIME_PENDING,
};

int32_t FileManagementUtils::GetRelativeDir(std::string& target, std::string& relativePath)
{
    if (target.size() < FILE_MANAGEMENT_PREFIX.size() ||
        target.substr(0, FILE_MANAGEMENT_PREFIX.size()) != FILE_MANAGEMENT_PREFIX) {
        MEDIA_ERR_LOG("target does not start with required prefix");
        return E_ERR;
    }

    for (const auto& forbiddenPrefix : FORBIDDEN_PREFIXES) {
        if (target.size() >= forbiddenPrefix.size() &&
            target.substr(0, forbiddenPrefix.size()) == forbiddenPrefix) {
            MEDIA_ERR_LOG("ProcessPath: target starts with forbidden prefix: %{public}s", forbiddenPrefix.c_str());
            return E_ERR;
        }
    }
    relativePath = target.substr(FILE_MANAGEMENT_PREFIX.size());
    if (!relativePath.empty() && relativePath.back() == '/') {
        relativePath.pop_back();
    }
    if (!target.empty() && target.back() == '/') {
        target.pop_back();
    }
    return E_OK;
}

std::string FileManagementUtils::GetLocalPath(const std::string &path)
{
    std::string localPath = path;
    size_t pos = localPath.find(PHOTO_CLOUD_PATH_URI);
    if (pos != std::string::npos) {
        localPath.replace(pos, PHOTO_CLOUD_PATH_URI.length(), PHOTO_MEDIA_PATH_URI);
    }
    return localPath;
}

std::string FileManagementUtils::ReplaceLastSegment(const std::string& data, const std::string& displayname)
{
    size_t lastSlashPos = data.find_last_of('/');
    if (lastSlashPos == std::string::npos) {
        return "";
    }

    std::string pathPrefix = data.substr(0, lastSlashPos + 1);
    std::string result = pathPrefix + displayname;
    return result;
}

std::string FileManagementUtils::GetLastDirName(const std::string& path)
{
    if (path.empty()) {
        return "";
    }
    std::string lastDirName = "";
    size_t lastSlashPos = path.find_last_of('/');
    if (lastSlashPos == std::string::npos) {
        lastDirName = path;
    } else if (lastSlashPos == (path.size() - 1)) {
        lastSlashPos = path.find_last_of('/', lastSlashPos - 1);
        lastDirName = (lastSlashPos == std::string::npos) ? path.substr(0, path.length() - 1) :
            path.substr(lastSlashPos + 1);
    } else {
        lastDirName = path.substr(lastSlashPos + 1);
    }
    return lastDirName;
}

int64_t FileManagementUtils::CalculateTotalSizeByPath(const std::vector<std::string> &assetpaths)
{
    int64_t totalSize = 0;
    for (const auto &filePath : assetpaths) {
        if (filePath.empty()) {
            MEDIA_ERR_LOG("Failed to get file path from uri: %{public}s", filePath.c_str());
            continue;
        }
        size_t fileSize = 0;
        if (!MediaFileUtils::GetFileSize(filePath, fileSize)) {
            MEDIA_ERR_LOG("Failed to get file size: %{public}s", filePath.c_str());
            continue;
        }
        totalSize += static_cast<int64_t>(fileSize);
    }
    return totalSize;
}

int64_t FileManagementUtils::CalculateTotalSize(const std::vector<std::string> &assets)
{
    int64_t totalSize = 0;
    for (const auto &asset : assets) {
        std::string filePath = MediaUriUtils::GetPathFromUri(asset);
        if (filePath.empty()) {
            MEDIA_ERR_LOG("Failed to get file path from uri: %{public}s",
                MediaFileUtils::DesensitizePath(filePath).c_str());
            continue;
        }
        size_t fileSize = 0;
        if (!MediaFileUtils::GetFileSize(filePath, fileSize)) {
            MEDIA_ERR_LOG("Failed to get file size: %{public}s",
                MediaFileUtils::DesensitizePath(filePath).c_str());
            continue;
        }
        totalSize += static_cast<int64_t>(fileSize);
    }
    return totalSize;
}

int32_t FileManagementUtils::QueryMoveAssetInfos(const NativeRdb::RdbPredicates& predicate,
    std::map<int32_t, FileAssetsInfo> &moveAssetMap)
{
    shared_ptr<NativeRdb::ResultSet> resultSet = MediaLibraryRdbStore::QueryWithFilter(predicate, PHOTO_QUERY_COLUMN);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RDB_QUERY_NO_RES, "fail to query asset");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        FileAssetsInfo info;
        info.fileId = get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_ID, resultSet, TYPE_INT32));
        info.mediaType =
            get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_TYPE, resultSet, TYPE_INT32));
        info.title =
            get<std::string>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_TITLE, resultSet, TYPE_STRING));
        info.photoSubtype =
            get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_SUBTYPE, resultSet, TYPE_INT32));
        info.position =
            get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_POSITION, resultSet, TYPE_INT32));
        info.data =
            get<std::string>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_FILE_PATH, resultSet, TYPE_STRING));
        info.storagePath =
            get<std::string>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_STORAGE_PATH, resultSet, TYPE_STRING));
        info.fileSourceType =
            get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, resultSet, TYPE_INT32));
        info.displayName =
            get<std::string>(ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_NAME, resultSet, TYPE_STRING));
        info.burstKey =
            get<std::string>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_BURST_KEY, resultSet, TYPE_STRING));
        info.size = get<int64_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_SIZE, resultSet, TYPE_INT64));
        info.dateTrashed =
            get<int64_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_DATE_TRASHED, resultSet, TYPE_INT64));
        info.hidden =
            get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_HIDDEN, resultSet, TYPE_INT32));
        info.dirty =
            get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_DIRTY, resultSet, TYPE_INT32));
        info.isTemp =
            get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_IS_TEMP, resultSet, TYPE_INT32));
        info.timePending =
            get<int64_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_TIME_PENDING, resultSet, TYPE_INT64));
        if (info.fileId == 0) {
            MEDIA_WARN_LOG("skip corrupted data");
            continue;
        }
        moveAssetMap.emplace(info.fileId, info);
    }
    resultSet->Close();
    return E_OK;
}

int32_t FileManagementUtils::QueryTargetAlbumInfo(const std::string relativePath, int32_t &albumId)
{
    std::string fileAlbumPath = "/FromDocs/" + relativePath;
    NativeRdb::RdbPredicates predicate(PhotoAlbumColumns::TABLE);
    predicate.EqualTo(PhotoAlbumColumns::ALBUM_LPATH, fileAlbumPath);
    vector<string> fetchColumn = {PhotoAlbumColumns::ALBUM_ID};
    shared_ptr<NativeRdb::ResultSet> resultSet = MediaLibraryRdbStore::QueryWithFilter(predicate, fetchColumn);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RDB_QUERY_NO_RES, "fail to query asset");
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Go To First Row Fails");
        resultSet->Close();
        return E_ERR;
    }
    albumId = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_ID, resultSet, TYPE_INT32));
    resultSet->Close();
    return E_OK;
}

int64_t FileManagementUtils::InsertFileAlbum(const FileAlbumInfo &fileAlbumInfo)
{
    MEDIA_INFO_LOG("begin insert PhotoAlbum.");
    NativeRdb::ValuesBucket value;
    int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
    value.PutInt(PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumType::SOURCE);
    value.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER);
    value.PutLong(PhotoAlbumColumns::ALBUM_DATE_ADDED, currentTime);
    value.PutLong(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, currentTime);
    value.PutString(PhotoAlbumColumns::ALBUM_NAME, fileAlbumInfo.albumName);
    value.PutString(PhotoAlbumColumns::ALBUM_LPATH, fileAlbumInfo.lpath);
    value.PutInt(PhotoAlbumColumns::UPLOAD_STATUS, PhotoAlbumUploadStatusOperation::GetAlbumUploadStatus());

    int64_t albumId = 0;
    AlbumAccurateRefresh albumRefresh;
    int32_t ret = albumRefresh.Insert(albumId, PhotoAlbumColumns::TABLE, value);
    SettingsDataManager::ComfirmUploadStatus();
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK && albumId > 0, E_ERR,
        "Insert photo albums failed, failed lpath is %{public}s", fileAlbumInfo.lpath.c_str());
    MEDIA_INFO_LOG("FolderParser: end insert PhotoAlbum.");
    return albumId;
}

int32_t FileManagementUtils::UpdateBurstNumber(std::shared_ptr<AssetAccurateRefresh> &refresh,
    const FileAssetsInfo &info)
{
    CHECK_AND_RETURN_RET_LOG(info.burstKey != "", E_ERR, "fail to get burstKey");
    NativeRdb::ValuesBucket value;
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_BURST_KEY, info.burstKey);
    predicates.And()->EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL, static_cast<int32_t>(BurstCoverLevelType::MEMBER));
    predicates.And()->EqualTo(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::BURST));
    value.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, info.ownerAlbumId);
    int32_t changedRows = refresh->UpdateWithDateTime(value, predicates);
    CHECK_AND_RETURN_RET_LOG(changedRows > 0, E_HAS_DB_ERROR,
        "Failed to update burst number, changeRows = %{public}d", changedRows);
    return E_OK;
}

int32_t FileManagementUtils::UpdateMoveAsset(std::shared_ptr<AssetAccurateRefresh> refresh, const FileAssetsInfo &info)
{
    NativeRdb::ValuesBucket value;
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, info.fileId);
    value.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, info.fileSourceType);
    value.PutString(PhotoColumn::PHOTO_STORAGE_PATH, info.storagePath);
    value.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, info.ownerAlbumId);
    int32_t changedRows = refresh->UpdateWithDateTime(value, predicates);
    CHECK_AND_RETURN_RET_LOG(changedRows > 0, E_HAS_DB_ERROR,
        "Failed to update file size, changeRows = %{public}d", changedRows);
    return E_OK;
}

int32_t FileManagementUtils::GetFileMtime(const string &filePath, time_t &mtime)
{
    struct stat statInfo {};
    if (stat(filePath.c_str(), &statInfo) != 0) {
        MEDIA_ERR_LOG("Get file mtime failed, path = %{public}s",
            MediaFileUtils::DesensitizePath(filePath).c_str());
        return E_ERR;
    }
    mtime = statInfo.st_mtime;
    return E_OK;
}
} // OHOS::Media
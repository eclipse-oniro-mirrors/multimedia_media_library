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
#define MLOG_TAG "PhotoDao"

#include "photo_dao.h"

#include <sstream>

#include "media_column.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_unistore_manager.h"
#include "rdb_predicates.h"
#include "result_set_utils.h"
#include "media_log_utils.h"
#include "userfile_manager_types.h"

using namespace OHOS::NativeRdb;

namespace {
const std::string SQL_PHOTOS_FIND_SAME_FILE_FOR_CLONE_RESTORE = "\
        SELECT file_id \
        FROM Photos \
        WHERE owner_album_id = ? AND display_name = ? AND size = ? \
        AND (1 <> ? OR orientation = ?) \
        LIMIT 1;";
const std::string SQL_PHOTOS_FIND_SAME_FILE_BY_STORAGE_PATH = "\
        SELECT file_id, size, date_modified, mime_type, media_type, inode, storage_path, file_source_type, \
        owner_album_id, owner_package, package_name, date_taken, data, sync_status, edit_time, subtype, position, \
        date_year, date_month, date_day, detail_time \
        FROM Photos \
        WHERE LOWER(storage_path) = LOWER(?) AND \
        (file_source_type = ? OR (file_source_type = ? AND position IN (?, ?) AND date_trashed = ? AND hidden = ?)) \
        LIMIT 1;";
} // namespace

namespace OHOS::Media {
PhotoDao::PhotoDao()
{
    rdbStore_ = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
}

PhotoDao::~PhotoDao() = default;

bool PhotoDao::PhotosRowData::IsExist()
{
    return fileId > 0;
}

std::string PhotoDao::PhotosRowData::ToString() const
{
    std::stringstream ss;
    ss << "PhotosRowData["
        << "fileId: " << fileId << ", "
        << "mediaType: " << mediaType << ", "
        << "fileSourceType: " << fileSourceType << ", "
        << "size: " << size << ", "
        << "dateModified: " << dateModified << ", "
        << "dateTaken: " << dateTaken << ", "
        << "inode: " << inode << ", "
        << "mimeType: " << mimeType << ", "
        << "storagePath: " << MediaLogUtils::GarbleFilePath(storagePath) << ", "
        << "ownerAlbumId: " << ownerAlbumId << ", "
        << "ownerPackage: " << MediaLogUtils::GarbleFile(ownerPackage) << ", "
        << "packageName: " << MediaLogUtils::GarbleFile(packageName) << ", "
        << "data: " << MediaLogUtils::GarbleFilePath(data) << "]";
    return ss.str();
}

// 公共查询逻辑：查询数据库获取 ThumbnailInfo 列表
int32_t PhotoDao::QueryThumbnailInfos(const std::vector<std::string> &inodes,
    std::vector<ThumbnailInfo> &infos, std::vector<int32_t> &thumbnailVisibleList)
{
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(PhotoColumn::PHOTO_FILE_INODE, inodes);
    for (const auto &inode : inodes) {
        MEDIA_INFO_LOG("generate thumbnail inode: %{public}s", inode.c_str());
    }
    auto rdbStore = rdbStore_;
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "rdbStore is null.");

    auto resultSet = rdbStore->Query(predicates, {PhotoColumn::PHOTO_FILE_INODE, MediaColumn::MEDIA_ID,
        MediaColumn::MEDIA_DATE_MODIFIED, MediaColumn::MEDIA_NAME, MediaColumn::MEDIA_DATE_TAKEN,
        PhotoColumn::MEDIA_FILE_PATH, PhotoColumn::PHOTO_THUMBNAIL_VISIBLE});
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "resultSet is nullptr!");

    while (resultSet->GoToNextRow() == E_OK) {
        ThumbnailInfo info;
        std::string inode = GetStringVal(PhotoColumn::PHOTO_FILE_INODE, resultSet);
        info.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        info.displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        info.path = GetStringVal(PhotoColumn::MEDIA_FILE_PATH, resultSet);
        info.dateTaken = GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet);
        info.dateModified = GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet);
        int32_t thumbnailVisible = GetInt32Val(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, resultSet);

        infos.push_back(info);
        thumbnailVisibleList.push_back(thumbnailVisible);
    }
    resultSet->Close();
    return E_OK;
}

int32_t PhotoDao::IsExistSameFileForCloneRestore(const std::vector<NativeRdb::ValueObject> &params)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, E_ERR, "rdbStore_ is null.");
    auto resultSet = rdbStore_->QuerySql(SQL_PHOTOS_FIND_SAME_FILE_FOR_CLONE_RESTORE, params);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK,
        E_ERR, "Query failed, not exist same file");
    resultSet->Close();
    return E_OK;
}

PhotoDao::PhotosRowData PhotoDao::FindSameFileByStoragePath(const std::string &storagePath,
    FileSourceType sourceType)
{
    const int32_t NOT_TRASHED = 0;
    const int32_t NOT_HIDDEN = 0;
    PhotoDao::PhotosRowData rowData;
    CHECK_AND_RETURN_RET_LOG(!storagePath.empty(), rowData, "storagePath is empty");
    std::vector<NativeRdb::ValueObject> params = { storagePath, sourceType, FileSourceType::MEDIA,
        static_cast<int32_t>(PhotoPositionType::LOCAL), static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD),
        NOT_TRASHED, NOT_HIDDEN };
    return FindSameFileInDatabase(SQL_PHOTOS_FIND_SAME_FILE_BY_STORAGE_PATH, params);
}

PhotoDao::PhotosRowData PhotoDao::FindSameFileInDatabase(const std::string &querySql,
    const std::vector<NativeRdb::ValueObject> &params)
{
    PhotoDao::PhotosRowData rowData;
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, rowData, "rdbStore_ is null.");
    auto resultSet = rdbStore_->QuerySql(querySql, params);
    CHECK_AND_RETURN_RET(resultSet != nullptr, rowData);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return rowData;
    }
    rowData.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    rowData.mediaType = GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet);
    rowData.fileSourceType = GetInt32Val(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, resultSet);
    rowData.syncStatus = GetInt32Val(PhotoColumn::PHOTO_SYNC_STATUS, resultSet);
    rowData.size = GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet);
    rowData.dateModified = GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet);
    rowData.editTime = GetInt64Val(PhotoColumn::PHOTO_EDIT_TIME, resultSet);
    rowData.inode = GetStringVal(PhotoColumn::PHOTO_FILE_INODE, resultSet);
    rowData.mimeType = GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet);
    rowData.storagePath = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
    rowData.ownerAlbumId = GetInt32Val(PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSet);
    rowData.ownerPackage = GetStringVal(PhotoColumn::MEDIA_OWNER_PACKAGE, resultSet);
    rowData.packageName = GetStringVal(PhotoColumn::MEDIA_PACKAGE_NAME, resultSet);
    rowData.dateTaken = GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet);
    rowData.data = GetStringVal(PhotoColumn::MEDIA_FILE_PATH, resultSet);
    rowData.subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    rowData.position = GetInt32Val(PhotoColumn::PHOTO_POSITION, resultSet);
    rowData.dateYear = GetInt32Val(PhotoColumn::PHOTO_DATE_YEAR, resultSet);
    rowData.dateMonth = GetInt32Val(PhotoColumn::PHOTO_DATE_MONTH, resultSet);
    rowData.detailTime = GetStringVal(PhotoColumn::PHOTO_DETAIL_TIME, resultSet);
    rowData.dateDay = GetInt32Val(PhotoColumn::PHOTO_DATE_DAY, resultSet);
    resultSet->Close();
    MEDIA_INFO_LOG("PhotoDao: rowData: %{public}s", rowData.ToString().c_str());
    return rowData;
}

int32_t PhotoDao::QuerySubtypeAndEffectMode(int32_t fileId, int32_t &subtype, int32_t &effectMode, bool &found)
{
    found = false;
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, E_ERR, "Failed to get rdbStore when query owner_album_id");
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    std::vector<std::string> columns = {PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::MOVING_PHOTO_EFFECT_MODE};
    auto resultSet = rdbStore_->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "failed to acquire result from visitor query.");
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
        effectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
        found = true;
    }
    resultSet->Close();
    return E_OK;
}

int32_t PhotoDao::QuerySubtype(int32_t fileId, int32_t &subtype, bool &found)
{
    found = false;
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, E_ERR, "Failed to get rdbStore when query subtype");
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    std::vector<std::string> columns = { PhotoColumn::PHOTO_SUBTYPE };
    auto resultSet = rdbStore_->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "Failed to query subtype of file: %{public}d", fileId);
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
        found = true;
    }
    resultSet->Close();
    return E_OK;
}

std::vector<std::string> PhotoDao::QueryPhotoPathsByStoragePaths(const std::vector<std::string> &files)
{
    std::vector<std::string> existingPaths;
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, existingPaths, "rdbStore_ is null.");
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(PhotoColumn::PHOTO_STORAGE_PATH, files);
    auto resultSet = rdbStore_->Query(predicates, {PhotoColumn::PHOTO_STORAGE_PATH});
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, existingPaths, "query failed");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string storagePath;
        int32_t result = resultSet->GetString(0, storagePath);
        if (result == E_OK) {
            existingPaths.push_back(storagePath);
        } else {
            MEDIA_WARN_LOG("Get storagePath fail: %{public}d", result);
        }
    }
    resultSet->Close();
    return existingPaths;
}
} // namespace OHOS::Media

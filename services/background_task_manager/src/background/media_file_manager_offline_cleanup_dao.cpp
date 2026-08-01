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

#define MLOG_TAG "FileManagerCleanupDao"

#include "media_file_manager_offline_cleanup_dao.h"

#include <sstream>

#include "dfx_utils.h"
#include "media_column.h"
#include "media_log.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "photo_album_column.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"

using namespace OHOS::NativeRdb;

namespace OHOS::Media::Background {
namespace {
using AssetRefresh = AccurateRefresh::AssetAccurateRefresh;
using AlbumRefresh = AccurateRefresh::AlbumAccurateRefresh;

constexpr int32_t LEGACY_ALBUM_SUBTYPE = static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER);
constexpr int32_t FILE_MANAGER_SOURCE_TYPE = static_cast<int32_t>(FileSourceType::FILE_MANAGER);
constexpr int32_t MEDIA_SOURCE_TYPE = static_cast<int32_t>(FileSourceType::MEDIA);
constexpr int32_t POSITION_LOCAL = static_cast<int32_t>(PhotoPositionType::LOCAL);
constexpr int32_t POSITION_CLOUD = static_cast<int32_t>(PhotoPositionType::CLOUD);
constexpr int32_t POSITION_LOCAL_AND_CLOUD = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
constexpr int32_t SUBTYPE_BURST = static_cast<int32_t>(PhotoSubType::BURST);
constexpr int32_t BURST_COVER = static_cast<int32_t>(BurstCoverLevelType::COVER);
constexpr int32_t BURST_MEMBER = static_cast<int32_t>(BurstCoverLevelType::MEMBER);
constexpr int32_t ALBUM_DIRTY_DELETED = static_cast<int32_t>(DirtyType::TYPE_DELETED);
const std::string LEGACY_SOURCE_PREFIX_PATTERN = "/storage/emulated/0/FromDocs/%";

template<typename FillFunc>
std::vector<OfflineCleanupPhotoRecord> QueryPhotoRecords(const std::string &sql,
    const std::vector<ValueObject> &args, FillFunc fillFunc)
{
    std::vector<OfflineCleanupPhotoRecord> records;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, records, "rdbStore is nullptr");
    auto resultSet = rdbStore->QuerySql(sql, args);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, records, "QuerySql failed");
    while (resultSet->GoToNextRow() == E_OK) {
        OfflineCleanupPhotoRecord record;
        fillFunc(record, resultSet);
        records.emplace_back(record);
    }
    resultSet->Close();
    return records;
}

template<typename FillFunc>
std::vector<OfflineCleanupAlbumRecord> QueryAlbumRecords(const std::string &sql,
    const std::vector<ValueObject> &args, FillFunc fillFunc)
{
    std::vector<OfflineCleanupAlbumRecord> records;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, records, "rdbStore is nullptr");
    auto resultSet = rdbStore->QuerySql(sql, args);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, records, "QuerySql failed");
    while (resultSet->GoToNextRow() == E_OK) {
        OfflineCleanupAlbumRecord record;
        fillFunc(record, resultSet);
        records.emplace_back(record);
    }
    resultSet->Close();
    return records;
}
}  // namespace

std::string OfflineCleanupPhotoRecord::ToString() const
{
    std::stringstream ss;
    ss << "Photo["
        << fileId << ", "
        << DfxUtils::GetSafePath(storagePath) << "]";
    return ss.str();
}

std::string OfflineCleanupAlbumRecord::ToString() const
{
    std::stringstream ss;
    ss << "Album["
        << albumId << ", "
        << albumSubtype << ", "
        << "lpath: " << DfxUtils::GetSafeUri(lpath) << "]";
    return ss.str();
}

std::vector<OfflineCleanupPhotoRecord> MediaFileManagerOfflineCleanupDao::QueryLocalDeleteCandidates(
    int32_t lastFileId, int32_t limit)
{
    const std::string sql =
        "SELECT file_id, media_type, data, source_path, storage_path, position, subtype, burst_cover_level, "
        "burst_key FROM Photos WHERE file_id > ? AND file_source_type = ? AND position = ? AND sync_status = 0 "
        "AND clean_flag = 0 AND time_pending = 0 AND is_temp = 0 ORDER BY file_id LIMIT ?";
    return QueryPhotoRecords(sql, {lastFileId, FILE_MANAGER_SOURCE_TYPE, POSITION_LOCAL, limit},
        [](OfflineCleanupPhotoRecord &record, const std::shared_ptr<ResultSet> &resultSet) {
            record.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
            record.mediaType = GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet);
            record.data = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
            record.sourcePath = GetStringVal(PhotoColumn::PHOTO_SOURCE_PATH, resultSet);
            record.storagePath = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
            record.position = GetInt32Val(PhotoColumn::PHOTO_POSITION, resultSet);
            record.subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
            record.burstCoverLevel = GetInt32Val(PhotoColumn::PHOTO_BURST_COVER_LEVEL, resultSet);
            record.burstKey = GetStringVal(PhotoColumn::PHOTO_BURST_KEY, resultSet);
        });
}

std::vector<OfflineCleanupPhotoRecord> MediaFileManagerOfflineCleanupDao::QueryPendingDeletedPhotos(
    int32_t lastFileId, int32_t limit)
{
    const std::string sql =
        "SELECT file_id, media_type, data, storage_path, display_name, subtype, moving_photo_effect_mode, "
        "date_taken, size "
        "FROM Photos WHERE file_id > ? AND file_source_type = ? AND position = ? "
        "AND time_pending = ? AND clean_flag = ? ORDER BY file_id LIMIT ?";
    return QueryPhotoRecords(sql, {lastFileId, FILE_MANAGER_SOURCE_TYPE, POSITION_LOCAL, TIME_PENDING_OFFLINE_CLEANUP,
            static_cast<int32_t>(CleanType::TYPE_OFFLINE_CLEAN), limit},
        [](OfflineCleanupPhotoRecord &record, const std::shared_ptr<ResultSet> &resultSet) {
            record.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
            record.mediaType = GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet);
            record.data = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
            record.storagePath = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
            record.displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
            record.subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
            record.effectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
            record.dateTaken = GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet);
            record.size = GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet);
        });
}

std::vector<OfflineCleanupPhotoRecord> MediaFileManagerOfflineCleanupDao::QueryBurstCoverPhotos(
    int32_t lastFileId, int32_t limit)
{
    const std::string sql =
        "SELECT file_id, media_type, data, storage_path, display_name, subtype, burst_cover_level, burst_key, "
        "date_modified FROM Photos WHERE file_id > ? AND file_source_type = ? AND position = ? AND sync_status = 0 "
        "AND clean_flag = 0 AND time_pending = 0 AND is_temp = 0 AND subtype = ? AND burst_cover_level = ? "
        "ORDER BY file_id LIMIT ?";
    return QueryPhotoRecords(sql, {lastFileId, FILE_MANAGER_SOURCE_TYPE, POSITION_LOCAL, SUBTYPE_BURST, BURST_COVER,
            limit},
        [](OfflineCleanupPhotoRecord &record, const std::shared_ptr<ResultSet> &resultSet) {
            record.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
            record.mediaType = GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet);
            record.data = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
            record.storagePath = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
            record.displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
            record.subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
            record.burstCoverLevel = GetInt32Val(PhotoColumn::PHOTO_BURST_COVER_LEVEL, resultSet);
            record.burstKey = GetStringVal(PhotoColumn::PHOTO_BURST_KEY, resultSet);
            record.dateModified = GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet);
        });
}

std::vector<OfflineCleanupPhotoRecord> MediaFileManagerOfflineCleanupDao::QueryLocalCloudPhotos(
    int32_t lastFileId, int32_t limit)
{
    const std::string sql =
        "SELECT file_id, file_source_type, position, media_type, data, storage_path, display_name, cloud_id, "
        "date_modified, size FROM Photos WHERE file_id > ? AND file_source_type = ? AND position = ? "
        "AND sync_status = 0 AND clean_flag = 0 AND time_pending = 0 AND is_temp = 0 "
        "ORDER BY file_id LIMIT ?";
    return QueryPhotoRecords(sql, {lastFileId, FILE_MANAGER_SOURCE_TYPE, POSITION_LOCAL_AND_CLOUD, limit},
        [](OfflineCleanupPhotoRecord &record, const std::shared_ptr<ResultSet> &resultSet) {
            record.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
            record.fileSourceType = GetInt32Val(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, resultSet);
            record.position = GetInt32Val(PhotoColumn::PHOTO_POSITION, resultSet);
            record.mediaType = GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet);
            record.data = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
            record.storagePath = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
            record.displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
            record.cloudId = GetStringVal(PhotoColumn::PHOTO_CLOUD_ID, resultSet);
            record.dateModified = GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet);
            record.size = GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet);
        });
}

std::vector<OfflineCleanupPhotoRecord> MediaFileManagerOfflineCleanupDao::QueryCloudOnlyPhotos(
    int32_t lastFileId, int32_t limit)
{
    const std::string sql =
        "SELECT file_id FROM Photos WHERE file_id > ? AND file_source_type = ? AND position = ? "
        "AND sync_status = 0 AND clean_flag = 0 AND time_pending = 0 AND is_temp = 0 "
        "ORDER BY file_id LIMIT ?";
    return QueryPhotoRecords(sql, {lastFileId, FILE_MANAGER_SOURCE_TYPE, POSITION_CLOUD, limit},
        [](OfflineCleanupPhotoRecord &record, const std::shared_ptr<ResultSet> &resultSet) {
            record.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        });
}

std::vector<OfflineCleanupPhotoRecord> MediaFileManagerOfflineCleanupDao::QueryLegacyAlbumPhotos(
    int32_t lastFileId, int32_t limit)
{
    const std::string sql =
        "SELECT p.file_id, p.owner_album_id, p.source_path, a.album_name, a.album_subtype, a.lpath "
        "FROM Photos p LEFT JOIN PhotoAlbum a ON p.owner_album_id = a.album_id WHERE p.file_id > ? "
        "AND (a.album_subtype = ? OR p.source_path LIKE ?) AND p.time_pending = 0 AND p.clean_flag = 0 "
        "ORDER BY p.file_id LIMIT ?";
    return QueryPhotoRecords(sql, {lastFileId, LEGACY_ALBUM_SUBTYPE, LEGACY_SOURCE_PREFIX_PATTERN, limit},
        [](OfflineCleanupPhotoRecord &record, const std::shared_ptr<ResultSet> &resultSet) {
            record.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
            record.ownerAlbumId = GetInt32Val(PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSet);
            record.sourcePath = GetStringVal(PhotoColumn::PHOTO_SOURCE_PATH, resultSet);
            record.albumName = GetStringVal(PhotoAlbumColumns::ALBUM_NAME, resultSet);
            record.albumSubtype = GetInt32Val(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet);
            record.albumLpath = GetStringVal(PhotoAlbumColumns::ALBUM_LPATH, resultSet);
        });
}

std::vector<OfflineCleanupAlbumRecord> MediaFileManagerOfflineCleanupDao::QueryEmptyLegacyAlbums(
    int32_t lastAlbumId, int32_t limit)
{
    const std::string sql =
        "SELECT a.album_id, a.album_subtype, a.dirty, a.album_name, a.lpath FROM PhotoAlbum a "
        "WHERE a.album_id > ? AND a.album_subtype = ? AND a.dirty != ? AND NOT EXISTS "
        "(SELECT 1 FROM Photos p WHERE p.owner_album_id = a.album_id) ORDER BY a.album_id LIMIT ?";
    return QueryAlbumRecords(sql, {lastAlbumId, LEGACY_ALBUM_SUBTYPE, ALBUM_DIRTY_DELETED, limit},
        [](OfflineCleanupAlbumRecord &record, const std::shared_ptr<ResultSet> &resultSet) {
            record.albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
            record.albumSubtype = GetInt32Val(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet);
            record.dirty = GetInt32Val(PhotoAlbumColumns::ALBUM_DIRTY, resultSet);
            record.albumName = GetStringVal(PhotoAlbumColumns::ALBUM_NAME, resultSet);
            record.lpath = GetStringVal(PhotoAlbumColumns::ALBUM_LPATH, resultSet);
        });
}

bool MediaFileManagerOfflineCleanupDao::MarkPhotosForOfflineCleanup(const std::vector<int32_t> &fileIds,
    AssetRefresh &assetRefresh, int32_t &changedRows)
{
    CHECK_AND_RETURN_RET_LOG(!fileIds.empty(), true, "fileIds is empty");
    ValuesBucket values;
    values.PutInt(MediaColumn::MEDIA_TIME_PENDING, TIME_PENDING_OFFLINE_CLEANUP);
    values.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(CleanType::TYPE_OFFLINE_CLEAN));
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    std::vector<std::string> idArgs;
    for (int32_t fileId : fileIds) {
        idArgs.emplace_back(std::to_string(fileId));
    }
    predicates.In(MediaColumn::MEDIA_ID, idArgs);
    predicates.And()->EqualTo(MediaColumn::MEDIA_TIME_PENDING, 0);
    predicates.And()->EqualTo(PhotoColumn::PHOTO_CLEAN_FLAG, 0);
    int32_t ret = assetRefresh.Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, false,
        "MarkPhotosForOfflineCleanup failed, ret: %{public}d, changedRows: %{public}d", ret, changedRows);
    return true;
}

bool MediaFileManagerOfflineCleanupDao::DeleteOfflineCleanupPhotos(const std::vector<std::string> &fileIds,
    AssetRefresh &assetRefresh, int32_t &deletedRows)
{
    CHECK_AND_RETURN_RET_LOG(!fileIds.empty(), true, "fileIds is empty");
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(MediaColumn::MEDIA_ID, fileIds);
    predicates.And()->EqualTo(MediaColumn::MEDIA_TIME_PENDING, TIME_PENDING_OFFLINE_CLEANUP);
    predicates.And()->EqualTo(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(CleanType::TYPE_OFFLINE_CLEAN));
    int32_t ret = assetRefresh.Delete(deletedRows, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, false,
        "DeleteOfflineCleanupPhotos failed, ret: %{public}d, deletedRows: %{public}d", ret, deletedRows);
    return true;
}

bool MediaFileManagerOfflineCleanupDao::UpdateBurstCoverPhoto(const OfflineCleanupPhotoRecord &photo,
    AssetRefresh &assetRefresh)
{
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, photo.fileId);
    predicates.And()->EqualTo(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, FILE_MANAGER_SOURCE_TYPE);
    ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, MEDIA_SOURCE_TYPE);
    values.PutNull(PhotoColumn::PHOTO_STORAGE_PATH);
    values.PutNull(PhotoColumn::PHOTO_FILE_INODE);
    int32_t changedRows = 0;
    int32_t ret = assetRefresh.Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK && changedRows > 0, false,
        "Update failed, ret: %{public}d, changedRows: %{public}d", ret, changedRows);
    return true;
}

bool MediaFileManagerOfflineCleanupDao::UpdateLocalCloudPhoto(const OfflineCleanupPhotoRecord &photo,
    AssetRefresh &assetRefresh)
{
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, photo.fileId);
    predicates.And()->EqualTo(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, FILE_MANAGER_SOURCE_TYPE);
    predicates.And()->EqualTo(PhotoColumn::PHOTO_POSITION, POSITION_LOCAL_AND_CLOUD);
    ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, MEDIA_SOURCE_TYPE);
    values.PutInt(PhotoColumn::PHOTO_POSITION, POSITION_CLOUD);
    values.PutInt(PhotoColumn::LOCAL_ASSET_SIZE, 0);
    values.PutNull(PhotoColumn::PHOTO_STORAGE_PATH);
    values.PutNull(PhotoColumn::PHOTO_FILE_INODE);
    int32_t changedRows = 0;
    int32_t ret = assetRefresh.Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK && changedRows > 0, false,
        "Update failed, ret: %{public}d, changedRows: %{public}d", ret, changedRows);
    return true;
}

bool MediaFileManagerOfflineCleanupDao::UpdateCloudOnlyPhotos(const std::vector<int32_t> &fileIds,
    AssetRefresh &assetRefresh)
{
    CHECK_AND_RETURN_RET_LOG(!fileIds.empty(), true, "fileIds is empty");
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    std::vector<std::string> idArgs;
    for (int32_t fileId : fileIds) {
        idArgs.emplace_back(std::to_string(fileId));
    }
    predicates.In(MediaColumn::MEDIA_ID, idArgs);
    predicates.And()->EqualTo(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, FILE_MANAGER_SOURCE_TYPE);
    ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, MEDIA_SOURCE_TYPE);
    values.PutNull(PhotoColumn::PHOTO_STORAGE_PATH);
    values.PutNull(PhotoColumn::PHOTO_FILE_INODE);
    int32_t changedRows = 0;
    int32_t ret = assetRefresh.Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, false,
        "UpdateCloudOnlyPhotos failed, ret: %{public}d, changedRows: %{public}d", ret, changedRows);
    return true;
}

bool MediaFileManagerOfflineCleanupDao::ExistMediaBurstMember(const OfflineCleanupPhotoRecord &photo)
{
    CHECK_AND_RETURN_RET(photo.subtype == SUBTYPE_BURST && photo.burstCoverLevel == BURST_COVER &&
        !photo.burstKey.empty(), false);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "rdbStore is nullptr");
    const std::string sql =
        "SELECT file_id FROM Photos WHERE file_id != ? AND burst_key = ? AND file_source_type = ? "
        "AND burst_cover_level = ? AND time_pending = 0 AND clean_flag = 0 LIMIT 1";
    const std::vector<ValueObject> args = {photo.fileId, photo.burstKey, MEDIA_SOURCE_TYPE, BURST_MEMBER};
    auto resultSet = rdbStore->QuerySql(sql, args);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "QuerySql failed");
    const bool found = resultSet->GoToFirstRow() == E_OK;
    resultSet->Close();
    return found;
}

bool MediaFileManagerOfflineCleanupDao::QueryAlbumByLpath(const std::string &lpath, OfflineCleanupAlbumRecord &album)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "rdbStore is nullptr");
    const std::string sql =
        "SELECT album_id, album_subtype, dirty, album_name, lpath FROM PhotoAlbum "
        "WHERE LOWER(lpath) = LOWER(?) LIMIT 1";
    const std::vector<ValueObject> args = {lpath};
    auto resultSet = rdbStore->QuerySql(sql, args);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "QuerySql failed");
    if (resultSet->GoToFirstRow() != E_OK) {
        resultSet->Close();
        return false;
    }
    album.albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
    album.albumSubtype = GetInt32Val(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet);
    album.dirty = GetInt32Val(PhotoAlbumColumns::ALBUM_DIRTY, resultSet);
    album.albumName = GetStringVal(PhotoAlbumColumns::ALBUM_NAME, resultSet);
    album.lpath = GetStringVal(PhotoAlbumColumns::ALBUM_LPATH, resultSet);
    resultSet->Close();
    return true;
}

bool MediaFileManagerOfflineCleanupDao::IsAlbumNameOccupied(const std::string &albumName)
{
    CHECK_AND_RETURN_RET_LOG(!albumName.empty(), false, "albumName is empty");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "rdbStore is nullptr");
    const std::string sql =
        "SELECT COUNT(1) AS count FROM PhotoAlbum WHERE album_name = ? AND dirty != ? AND album_subtype != ?";
    const std::vector<ValueObject> args = {albumName, ALBUM_DIRTY_DELETED, LEGACY_ALBUM_SUBTYPE};
    auto resultSet = rdbStore->QuerySql(sql, args);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "QuerySql failed");
    int32_t count = 0;
    CHECK_AND_EXECUTE(resultSet->GoToFirstRow() != E_OK, count = GetInt32Val("count", resultSet));
    resultSet->Close();
    return count > 0;
}

bool MediaFileManagerOfflineCleanupDao::RenewDeletedAlbum(int32_t albumId, AlbumRefresh &albumRefresh)
{
    AbsRdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    predicates.And()->EqualTo(PhotoAlbumColumns::ALBUM_DIRTY, ALBUM_DIRTY_DELETED);
    ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_DIRTY, static_cast<int32_t>(DirtyType::TYPE_MDIRTY));
    int32_t changedRows = 0;
    return albumRefresh.Update(changedRows, values, predicates) == E_OK;
}

bool MediaFileManagerOfflineCleanupDao::UpdatePhotoAlbumRelation(int32_t fileId, int32_t oldAlbumId,
    int32_t targetAlbumId, const std::string &targetSourcePath, AssetRefresh &assetRefresh)
{
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    predicates.And()->EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, oldAlbumId);
    ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, targetAlbumId);
    values.PutString(PhotoColumn::PHOTO_SOURCE_PATH, targetSourcePath);
    int32_t changedRows = 0;
    return assetRefresh.Update(changedRows, values, predicates) == E_OK && changedRows > 0;
}

bool MediaFileManagerOfflineCleanupDao::LogicalDeleteEmptyLegacyAlbums(const std::vector<int32_t> &albumIds,
    AlbumRefresh &albumRefresh, int32_t &deletedCount)
{
    deletedCount = 0;
    std::vector<std::string> idArgs;
    for (int32_t albumId : albumIds) {
        idArgs.emplace_back(std::to_string(albumId));
    }
    AbsRdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.In(PhotoAlbumColumns::ALBUM_ID, idArgs);
    predicates.And()->EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, LEGACY_ALBUM_SUBTYPE);
    int32_t deletedRows = 0;
    const int32_t ret = albumRefresh.LogicalDeleteReplaceByUpdate(predicates, deletedRows);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, false,
        "LogicalDeleteEmptyLegacyAlbums failed, ret: %{public}d, deletedRows: %{public}d", ret, deletedRows);
    deletedCount += deletedRows;
    return true;
}

int64_t MediaFileManagerOfflineCleanupDao::CountLegacyPhotos()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, 0, "rdbStore is nullptr");
    const std::string sql =
        "SELECT COUNT(1) FROM Photos p LEFT JOIN PhotoAlbum a ON p.owner_album_id = a.album_id "
        "WHERE p.file_source_type = ? OR a.album_subtype = ? OR p.source_path LIKE ?";
    const std::vector<ValueObject> args = {FILE_MANAGER_SOURCE_TYPE, LEGACY_ALBUM_SUBTYPE,
        LEGACY_SOURCE_PREFIX_PATTERN};
    auto resultSet = rdbStore->QuerySql(sql, args);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, 0, "QuerySql failed");
    int64_t count = 0;
    CHECK_AND_EXECUTE(resultSet->GoToFirstRow() != E_OK, resultSet->GetLong(0, count));
    resultSet->Close();
    return count;
}

int64_t MediaFileManagerOfflineCleanupDao::CountPendingDeletedPhotos()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, 0, "rdbStore is nullptr");
    const std::string sql = "SELECT COUNT(1) FROM Photos WHERE time_pending = ? AND clean_flag = ?";
    const std::vector<ValueObject> args = {TIME_PENDING_OFFLINE_CLEANUP,
        static_cast<int32_t>(CleanType::TYPE_OFFLINE_CLEAN)};
    auto resultSet = rdbStore->QuerySql(sql, args);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, 0, "QuerySql failed");
    int64_t count = 0;
    CHECK_AND_EXECUTE(resultSet->GoToFirstRow() != E_OK, resultSet->GetLong(0, count));
    resultSet->Close();
    return count;
}

int64_t MediaFileManagerOfflineCleanupDao::CountLegacyAlbums()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, 0, "rdbStore is nullptr");
    const std::string sql = "SELECT COUNT(1) FROM PhotoAlbum WHERE album_subtype = ?";
    const std::vector<ValueObject> args = {LEGACY_ALBUM_SUBTYPE};
    auto resultSet = rdbStore->QuerySql(sql, args);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, 0, "QuerySql failed");
    int64_t count = 0;
    CHECK_AND_EXECUTE(resultSet->GoToFirstRow() != E_OK, resultSet->GetLong(0, count));
    resultSet->Close();
    return count;
}
}  // namespace OHOS::Media::Background

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

#include "shooting_mode_album_clone.h"

#include <algorithm>

#include "backup_database_utils.h"
#include "media_log.h"
#include "media_time_utils.h"
#include "result_set_utils.h"

namespace OHOS {
namespace Media {

ShootingModeAlbumClone::ShootingModeAlbumClone(
    std::shared_ptr<NativeRdb::RdbStore> sourceRdb,
    std::shared_ptr<NativeRdb::RdbStore> destRdb)
    : sourceRdb_(std::move(sourceRdb)), destRdb_(std::move(destRdb))
{
}

bool ShootingModeAlbumClone::Execute()
{
    int64_t startTime = MediaTimeUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("ShootingModeAlbumClone::Execute start");

    auto sourceAlbums = QuerySourceAlbums();
    if (!sourceAlbums) {
        MEDIA_ERR_LOG("Failed to query source albums");
        return false;
    }

    auto destAlbums = QueryDestAlbums();
    if (!destAlbums) {
        MEDIA_ERR_LOG("Failed to query dest albums");
        return false;
    }

    AlbumNameIndex destIndex(destAlbums->GetAlbums());
    auto operations = BuildOperations(sourceAlbums->GetAlbums(), destIndex);

    MEDIA_INFO_LOG("ShootingModeAlbumClone: built %{public}zu operations (update: %{public}zu, insert: %{public}zu)",
        operations.size(), destIndex.Size(), operations.size() - destIndex.Size());

    bool result = ExecuteOperations(operations);

    int64_t endTime = MediaTimeUtils::UTCTimeMilliSeconds();
    int64_t duration = endTime - startTime;
    MEDIA_INFO_LOG("ShootingModeAlbumClone::Execute completed, total time: %{public}" PRId64 " ms, "
        "source albums: %{public}zu, dest albums: %{public}zu, operations: %{public}zu",
        duration, sourceAlbums->GetAlbums().size(), destAlbums->GetAlbums().size(), operations.size());

    return result;
}

std::optional<ShootingModeAlbumClone::AlbumQueryResult> ShootingModeAlbumClone::QuerySourceAlbums()
{
    return QueryAlbums(sourceRdb_, "source");
}

std::optional<ShootingModeAlbumClone::AlbumQueryResult> ShootingModeAlbumClone::QueryDestAlbums()
{
    return QueryAlbums(destRdb_, "dest");
}

std::optional<ShootingModeAlbumClone::AlbumQueryResult> ShootingModeAlbumClone::QueryAlbums(
    std::shared_ptr<NativeRdb::RdbStore> rdb, const std::string& logPrefix)
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::string querySql = "SELECT album_id, album_name, album_type, album_subtype "
                           "FROM AnalysisAlbum WHERE album_subtype = 4101";

    auto resultSet = rdb->QuerySql(querySql);
    if (!resultSet) {
        MEDIA_ERR_LOG("Failed to query %{public}s AnalysisAlbum", logPrefix.c_str());
        return std::nullopt;
    }

    AlbumQueryResult result;
    while (resultSet->GoToNextRow() == 0) {
        AlbumInfo album;
        album.albumId = GetInt32Val("album_id", resultSet);
        album.albumName = GetStringVal("album_name", resultSet);
        album.albumType = GetInt32Val("album_type", resultSet);
        album.albumSubtype = GetInt32Val("album_subtype", resultSet);
        result.albums_.push_back(album);
    }

    MEDIA_INFO_LOG("ShootingModeAlbumClone: queried %{public}zu %{public}s albums",
        result.albums_.size(), logPrefix.c_str());

    return result;
}

ShootingModeAlbumClone::AlbumNameIndex::AlbumNameIndex(const std::vector<AlbumInfo>& albums)
{
    for (const auto& album : albums) {
        nameToIdMap_[album.albumName] = album.albumId;
    }
}

std::optional<int32_t> ShootingModeAlbumClone::AlbumNameIndex::FindAlbumId(
    const std::string& albumName) const
{
    auto it = nameToIdMap_.find(albumName);
    if (it != nameToIdMap_.end()) {
        return std::optional<int32_t>(it->second);
    }
    return std::nullopt;
}

std::vector<std::unique_ptr<ShootingModeAlbumClone::AlbumOperation>> ShootingModeAlbumClone::BuildOperations(
    const std::vector<AlbumInfo>& sourceAlbums, const AlbumNameIndex& destIndex)
{
    std::vector<std::unique_ptr<AlbumOperation>> operations;

    for (const auto& album : sourceAlbums) {
        auto destAlbumId = destIndex.FindAlbumId(album.albumName);
        if (destAlbumId) {
            operations.push_back(std::make_unique<AlbumUpdateOperation>(
                *destAlbumId, album.albumId));
        } else {
            operations.push_back(std::make_unique<AlbumInsertOperation>(
                album));
        }
    }

    return operations;
}

bool ShootingModeAlbumClone::ExecuteOperations(
    const std::vector<std::unique_ptr<AlbumOperation>>& operations)
{
    std::lock_guard<std::mutex> lock(mutex_);

    int32_t successCount = 0;
    int32_t failCount = 0;

    for (const auto& operation : operations) {
        if (operation->Execute(destRdb_)) {
            successCount++;
        } else {
            failCount++;
            MEDIA_WARN_LOG("Failed to execute album operation");
        }
    }

    MEDIA_INFO_LOG("ShootingModeAlbumClone: executed operations, success: %{public}d, fail: %{public}d",
        successCount, failCount);
    return true;
}

ShootingModeAlbumClone::AlbumUpdateOperation::AlbumUpdateOperation(
    int32_t oldAlbumId, int32_t newAlbumId)
    : oldAlbumId_(oldAlbumId), newAlbumId_(newAlbumId)
{
}

bool ShootingModeAlbumClone::AlbumUpdateOperation::Execute(
    std::shared_ptr<NativeRdb::RdbStore> rdb)
{
    int32_t ANALYSIS_ALBUM = 4096;
    int32_t SHOOT_MODE_TYPE = 4101;

    NativeRdb::ValuesBucket updateValues;
    updateValues.PutInt("album_id", newAlbumId_);
    auto updatePredicates = std::make_unique<NativeRdb::AbsRdbPredicates>(ANALYSIS_ALBUM_TABLE);
    updatePredicates->EqualTo("album_id", oldAlbumId_);

    int32_t updatedRows = 0;
    int32_t ret = BackupDatabaseUtils::Update(rdb, updatedRows,
        updateValues, updatePredicates);
    if (ret != E_OK) {
        MEDIA_WARN_LOG("Failed to update album_id from %{public}d to %{public}d, "
            "ret=%{public}d", oldAlbumId_, newAlbumId_, ret);
        return false;
    }

    NativeRdb::ValuesBucket tabOldAlbumsValues;
    tabOldAlbumsValues.PutInt("album_id", newAlbumId_);
    auto tabOldAlbumsPredicates = std::make_unique<NativeRdb::AbsRdbPredicates>("tab_old_albums");
    tabOldAlbumsPredicates->EqualTo("album_id", oldAlbumId_)
                          ->And()
                          ->EqualTo("album_type", ANALYSIS_ALBUM)
                          ->And()
                          ->EqualTo("album_subtype", SHOOT_MODE_TYPE);

    int32_t tabUpdatedRows = 0;
    int32_t tabRet = BackupDatabaseUtils::Update(rdb, tabUpdatedRows,
        tabOldAlbumsValues, tabOldAlbumsPredicates);
    if (tabRet == E_OK) {
        return true;
    }

    MEDIA_WARN_LOG("Failed to update tab_old_albums album_id from %{public}d to %{public}d, "
        "ret=%{public}d", oldAlbumId_, newAlbumId_, tabRet);
    return true;
}

ShootingModeAlbumClone::AlbumInsertOperation::AlbumInsertOperation(
    const AlbumInfo& album)
    : album_(album)
{
}

bool ShootingModeAlbumClone::AlbumInsertOperation::Execute(
    std::shared_ptr<NativeRdb::RdbStore> rdb)
{
    NativeRdb::ValuesBucket values;
    values.PutInt("album_id", album_.albumId);
    values.PutInt("album_type", album_.albumType);
    values.PutInt("album_subtype", album_.albumSubtype);
    values.PutString("album_name", album_.albumName);

    int64_t insertedRowId = -1;
    int32_t ret = rdb->Insert(insertedRowId, ANALYSIS_ALBUM_TABLE, values);
    if (ret != E_OK) {
        MEDIA_WARN_LOG("Failed to insert album '%{public}s', ret=%{public}d",
            album_.albumName.c_str(), ret);
        return false;
    }

    return true;
}

} // namespace Media
} // namespace OHOS
/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "tab_old_albums_clone.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "backup_const_column.h"
#include "backup_database_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "photo_album_column.h"
#include "rdb_errno.h"
#include "rdb_store.h"
#include "rdb_predicates.h"
#include "values_bucket.h"

namespace OHOS::Media {
namespace {
    constexpr int32_t INITIAL_CLONE_SEQUENCE = 1;
    constexpr int32_t INVALID_ID = -1;
}

TabOldAlbumsClone::TabOldAlbumsClone(
    const std::shared_ptr<NativeRdb::RdbStore>& sourceRdb,
    const std::shared_ptr<NativeRdb::RdbStore>& destRdb,
    const std::unordered_map<std::string, std::unordered_map<int32_t, int32_t>>& tableAlbumIdMap)
    : sourceRdb_(sourceRdb), destRdb_(destRdb), tableAlbumIdMap_(tableAlbumIdMap)
{
    MEDIA_INFO_LOG("TabOldAlbumsClone constructor");
}

int32_t TabOldAlbumsClone::CloneAlbums(const std::vector<std::string> &sourceTables)
{
    if (sourceRdb_ == nullptr || destRdb_ == nullptr) {
        MEDIA_ERR_LOG("Source or destination RdbStore is null");
        return E_INVALID_ARGUMENTS;
    }

    if (sourceTables.empty()) {
        MEDIA_ERR_LOG("Source tables vector is empty");
        return E_INVALID_ARGUMENTS;
    }

    MEDIA_INFO_LOG("Starting album cloning from %{public}zu source tables", sourceTables.size());

    // Get the clone sequence once for this entire operation
    int32_t cloneSequence = GetNextCloneSequence();
    MEDIA_INFO_LOG("Using clone sequence %{public}d for all albums in this clone operation", cloneSequence);

    // Clean up the tab_old_albums table before cloning
    int32_t cleanupResult = CleanupTable();
    if (cleanupResult != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to clean up tab_old_albums table: %{public}d", cleanupResult);
    }

    int32_t finalResult = NativeRdb::E_OK;
    for (const auto &sourceTable : sourceTables) {
        int32_t result = CloneAlbumsFromTable(sourceTable, cloneSequence);
        if (result != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Failed to clone albums from table: %{public}s, error: %{public}d", sourceTable.c_str(),
                result);
            finalResult = result;
            continue;
        }
    }
    
    MEDIA_INFO_LOG("Album cloning completed successfully");
    return finalResult;
}

int32_t TabOldAlbumsClone::CloneAlbumsFromTable(const std::string &sourceTable, int32_t cloneSequence)
{
    if (sourceTable.empty()) {
        MEDIA_ERR_LOG("Source table name is empty");
        return E_INVALID_ARGUMENTS;
    }

    MEDIA_INFO_LOG("Cloning albums from source table: %{public}s with clone sequence: %{public}d",
                   sourceTable.c_str(), cloneSequence);

    const int32_t pageSize = 100;
    // Add a maximum iteration limit to prevent infinite loops
    const int32_t maxIterations = 10000;
    int32_t offset = 0;
    size_t totalCloned = 0;
    int32_t iterationCount = 0;

    while (iterationCount < maxIterations) {
        std::vector<AlbumMapTbl> albumMapTbls = QueryAlbumsFromTable(sourceTable, offset, pageSize);
        if (albumMapTbls.empty()) {
            break;
        }

        std::vector<AlbumMapTbl> processedAlbums = ProcessAlbumMapTbls(albumMapTbls, sourceTable, cloneSequence);
        if (processedAlbums.empty()) {
            MEDIA_WARN_LOG("No albums to process after mapping for table %{public}s", sourceTable.c_str());
            // If no albums were processed but we got data, we've reached the end
            if (albumMapTbls.size() < pageSize) {
                break;
            }
            offset += pageSize;
            iterationCount++;
            continue;
        }

        int32_t insertResult = InsertAlbumData(processedAlbums);
        if (insertResult != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Failed to batch insert album data for table %{public}s", sourceTable.c_str());
            return insertResult;
        }

        totalCloned += processedAlbums.size();
        offset += pageSize;
        iterationCount++;

        if (albumMapTbls.size() < pageSize) {
            break;
        }
    }

    if (iterationCount >= maxIterations) {
        MEDIA_ERR_LOG("Maximum iteration limit reached for table %{public}s, possible infinite loop detected",
            sourceTable.c_str());
        return E_INVALID_ARGUMENTS;
    }

    MEDIA_INFO_LOG("Successfully cloned %{public}d albums from table: %{public}s",
        static_cast<int32_t>(totalCloned), sourceTable.c_str());
    return NativeRdb::E_OK;
}

int32_t TabOldAlbumsClone::GetNextCloneSequence()
{
    if (destRdb_ == nullptr) {
        MEDIA_ERR_LOG("Destination RdbStore is null");
        return INITIAL_CLONE_SEQUENCE;
    }

    std::string query = "SELECT MAX(" + ALBUM_CLONE_SEQUENCE_COL + ") AS "
    + ALBUM_CLONE_SEQUENCE_COL + " FROM " + TAB_OLD_ALBUMS;

    auto resultSet = destRdb_->QuerySql(query);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query for existing clone sequence");
        return INITIAL_CLONE_SEQUENCE;
    }

    int32_t result = resultSet->GoToFirstRow();
    if (result != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to go to first row in clone sequence query");
        return INITIAL_CLONE_SEQUENCE;
    }

    int32_t maxSequence = GetInt32Val(ALBUM_CLONE_SEQUENCE_COL, resultSet);
    return maxSequence + INITIAL_CLONE_SEQUENCE;
}

int32_t TabOldAlbumsClone::CleanupTable()
{
    if (destRdb_ == nullptr) {
        MEDIA_ERR_LOG("Destination RdbStore is null");
        return E_INVALID_ARGUMENTS;
    }

    std::string deleteQuery = "DELETE FROM " + TAB_OLD_ALBUMS;
    int32_t result = destRdb_->ExecuteSql(deleteQuery);
    if (result != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to clean up tab_old_albums table: %{public}d", result);
        return result;
    }

    MEDIA_INFO_LOG("Successfully cleaned up tab_old_albums table");
    return NativeRdb::E_OK;
}

bool TabOldAlbumsClone::ValidateAlbumMapTbl(const AlbumMapTbl& albumMapTbl)
{
    return albumMapTbl.oldAlbumId.has_value() &&
           albumMapTbl.albumId.has_value() &&
           albumMapTbl.albumType.has_value() &&
           albumMapTbl.albumSubtype.has_value() &&
           albumMapTbl.cloneSequence.has_value();
}

int32_t TabOldAlbumsClone::InsertAlbumData(const std::vector<AlbumMapTbl>& albumMapTbls)
{
    if (destRdb_ == nullptr) {
        MEDIA_ERR_LOG("Destination RdbStore is null");
        return E_INVALID_ARGUMENTS;
    }

    if (albumMapTbls.empty()) {
        MEDIA_INFO_LOG("Album map table list is empty, no need to insert.");
        return NativeRdb::E_OK;
    }

    std::vector<NativeRdb::ValuesBucket> valuesList;
    for (const auto& albumMapTbl : albumMapTbls) {
        if (!ValidateAlbumMapTbl(albumMapTbl)) {
            MEDIA_ERR_LOG("AlbumMapTbl validation failed for oldAlbumId: %{public}d",
                albumMapTbl.oldAlbumId.value_or(INVALID_ID));
            return E_INVALID_ARGUMENTS;
        }
        valuesList.push_back(CreateValuesBucketFromAlbumMapTbl(albumMapTbl));
    }

    int64_t insertedCount = 0;
    int32_t result = destRdb_->BatchInsert(insertedCount, TAB_OLD_ALBUMS, valuesList);
    if (result != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to batch insert album data into tab_old_albums: %{public}d", result);
        return result;
    }

    MEDIA_INFO_LOG("Successfully batch inserted %{public}lld albums.", static_cast<long long>(insertedCount));

    return NativeRdb::E_OK;
}

std::vector<AlbumMapTbl> TabOldAlbumsClone::QueryAlbumsFromTable(const std::string &sourceTable,
    int32_t offset, int32_t limit)
{
    std::vector<AlbumMapTbl> result;

    if (sourceTable.empty()) {
        MEDIA_ERR_LOG("Source table name is empty");
        return result;
    }

    std::string query = "SELECT " + PhotoAlbumColumns::ALBUM_ID + ", " +
                       PhotoAlbumColumns::ALBUM_TYPE + ", " +
                       PhotoAlbumColumns::ALBUM_SUBTYPE + " FROM " + sourceTable +
                       " LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset);

    auto resultSet = sourceRdb_->QuerySql(query);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query source table: %{public}s", sourceTable.c_str());
        return result;
    }

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        AlbumMapTbl albumMapTbl;
        ParseAlbumResultSet(resultSet, albumMapTbl);
        result.emplace_back(albumMapTbl);
    }

    resultSet->Close();
    return result;
}

void TabOldAlbumsClone::ParseAlbumResultSet(
    const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
    AlbumMapTbl& albumMapTbl)
{
    int32_t albumId = 0;
    int32_t albumType = 0;
    int32_t albumSubtype = 0;

    albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
    albumType = GetInt32Val(PhotoAlbumColumns::ALBUM_TYPE, resultSet);
    albumSubtype = GetInt32Val(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet);

    albumMapTbl.albumId = albumId;
    albumMapTbl.albumType = albumType;
    albumMapTbl.albumSubtype = albumSubtype;
    albumMapTbl.oldAlbumId = albumId;
}

std::vector<AlbumMapTbl> TabOldAlbumsClone::ProcessAlbumMapTbls(
    const std::vector<AlbumMapTbl>& albumMapTbls,
    const std::string& sourceTable,
    int32_t cloneSequence)
{
    std::vector<AlbumMapTbl> processedAlbums;
    processedAlbums.reserve(albumMapTbls.size());

    MEDIA_INFO_LOG("Using clone sequence %{public}d for all albums in table %{public}s",
                   cloneSequence, sourceTable.c_str());

    for (const auto& albumMapTbl : albumMapTbls) {
        AlbumMapTbl processedAlbum = albumMapTbl;
        
        // Map old album ID to new album ID using tableAlbumIdMap_
        if (processedAlbum.albumId.has_value()) {
            int32_t oldAlbumId = processedAlbum.albumId.value();
            std::optional<int32_t> newAlbumId = GetNewAlbumId(sourceTable, oldAlbumId);
            
            if (newAlbumId.has_value()) {
                processedAlbum.albumId = newAlbumId.value();
                processedAlbum.oldAlbumId = oldAlbumId;
                
                // Set the same clone sequence for all albums in this operation
                processedAlbum.cloneSequence = cloneSequence;
            } else {
                MEDIA_WARN_LOG("No mapping found for old album ID %{public}d in table %{public}s", oldAlbumId,
                    sourceTable.c_str());
                continue;
            }
        }
        
        processedAlbums.push_back(std::move(processedAlbum));
    }

    return processedAlbums;
}

NativeRdb::ValuesBucket TabOldAlbumsClone::CreateValuesBucketFromAlbumMapTbl(const AlbumMapTbl& albumMapTbl)
{
    NativeRdb::ValuesBucket values;

    if (albumMapTbl.oldAlbumId.has_value()) {
        values.PutInt(OLD_ALBUM_ID_COL, albumMapTbl.oldAlbumId.value());
    }
    if (albumMapTbl.albumId.has_value()) {
        values.PutInt(ALBUM_ID_COL, albumMapTbl.albumId.value());
    }
    if (albumMapTbl.albumType.has_value()) {
        values.PutInt(ALBUM_TYPE_COL, albumMapTbl.albumType.value());
    }
    if (albumMapTbl.albumSubtype.has_value()) {
        values.PutInt(ALBUM_SUBTYPE_COL, albumMapTbl.albumSubtype.value());
    }
    if (albumMapTbl.cloneSequence.has_value()) {
        values.PutInt(ALBUM_CLONE_SEQUENCE_COL, albumMapTbl.cloneSequence.value());
    }

    return values;
}

std::optional<int32_t> TabOldAlbumsClone::GetNewAlbumId(const std::string& sourceTable, int32_t oldAlbumId)
{
    // Look up the table in tableAlbumIdMap_
    auto tableIt = tableAlbumIdMap_.find(sourceTable);
    if (tableIt == tableAlbumIdMap_.end()) {
        MEDIA_WARN_LOG("Source table %{public}s not found in tableAlbumIdMap_", sourceTable.c_str());
        return std::nullopt;
    }

    // Look up the old album ID in the inner map
    const auto& albumIdMap = tableIt->second;
    auto albumIt = albumIdMap.find(oldAlbumId);
    if (albumIt == albumIdMap.end()) {
        MEDIA_WARN_LOG("Old album ID %{public}d not found in table %{public}s mapping", oldAlbumId,
            sourceTable.c_str());
        return std::nullopt;
    }

    return albumIt->second; // Return the new album ID
}
} // namespace Media

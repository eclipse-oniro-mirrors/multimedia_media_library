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

#ifndef FOUNDATION_MULTIMEDIA_MEDIA_LIBRARY_SERVICES_MEDIA_BACKUP_EXTENSION_INCLUDE_RESTORE_TAB_OLD_ALBUMS_CLONE_H
#define FOUNDATION_MULTIMEDIA_MEDIA_LIBRARY_SERVICES_MEDIA_BACKUP_EXTENSION_INCLUDE_RESTORE_TAB_OLD_ALBUMS_CLONE_H

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <unordered_map>

#include "backup_const_column.h"
#include "rdb_store.h"

namespace OHOS {
namespace Media {

struct AlbumMapTbl {
    std::optional<int32_t> albumId;
    std::optional<int32_t> albumType;
    std::optional<int32_t> albumSubtype;
    std::optional<int32_t> oldAlbumId;
    std::optional<int32_t> cloneSequence;
};

class TabOldAlbumsClone {
public:
    TabOldAlbumsClone(
        const std::shared_ptr<NativeRdb::RdbStore>& sourceRdb,
        const std::shared_ptr<NativeRdb::RdbStore>& destRdb,
        const std::unordered_map<std::string, std::unordered_map<int32_t, int32_t>>& tableAlbumIdMap);
    ~TabOldAlbumsClone() = default;

    /**
     * @brief Clone albums from source tables to tab_old_albums
     * @param sourceTables Vector of source table names to clone from
     * @return int32_t Returns 0 on success, error code on failure
     */
    int32_t CloneAlbums(const std::vector<std::string> &sourceTables);

private:
    /**
     * @brief Clone albums from a specific source table
     * @param sourceTable Source table name
     * @param cloneSequence Clone sequence for this operation
     * @return int32_t Returns 0 on success, error code on failure
     */
    int32_t CloneAlbumsFromTable(const std::string &sourceTable, int32_t cloneSequence);

    /**
     * @brief Get the next global clone sequence for the current clone operation
     * @return int32_t Next clone sequence number
     */
    int32_t GetNextCloneSequence();

    /**
     * @brief Clean up the tab_old_albums table by deleting all rows
     * @return int32_t Returns 0 on success, error code on failure
     */
    int32_t CleanupTable();

    /**
     * @brief Insert album data into tab_old_albums table
     * @param albumMapTbl Album data struct containing all required fields
     * @return int32_t Returns 0 on success, error code on failure
     */
    int32_t InsertAlbumData(const std::vector<AlbumMapTbl>& albumMapTbls);

    /**
     * @brief Validate that AlbumMapTbl contains all required data
     * @param albumMapTbl Album data struct to validate
     * @return bool Returns true if all required fields are present, false otherwise
     */
    bool ValidateAlbumMapTbl(const AlbumMapTbl& albumMapTbl);

    /**
     * @brief Query albums from source table and return as AlbumMapTbl vector
     * @param sourceTable Source table name
     * @return std::vector<AlbumMapTbl> Vector of album data
     */
    std::vector<AlbumMapTbl> QueryAlbumsFromTable(const std::string &sourceTable, int32_t offset, int32_t limit);

    /**
     * @brief Parse result set into AlbumMapTbl struct
     * @param resultSet Database result set
     * @param albumMapTbl Reference to AlbumMapTbl to populate
     */
    void ParseAlbumResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet, AlbumMapTbl& albumMapTbl);

    /**
     * @brief Process album data and set clone sequence
     * @param albumMapTbls Vector of album data to process
     * @param sourceTable Source table name for album ID mapping
     * @param cloneSequence Clone sequence for this operation
     * @return std::vector<AlbumMapTbl> Processed album data
     */
    std::vector<AlbumMapTbl> ProcessAlbumMapTbls(
        const std::vector<AlbumMapTbl>& albumMapTbls,
        const std::string& sourceTable,
        int32_t cloneSequence);

    /**
     * @brief Create ValuesBucket from AlbumMapTbl
     * @param albumMapTbl Album data struct
     * @return NativeRdb::ValuesBucket Values bucket for insertion
     */
    NativeRdb::ValuesBucket CreateValuesBucketFromAlbumMapTbl(const AlbumMapTbl& albumMapTbl);

    /**
     * @brief Get new album ID from old album ID using tableAlbumIdMap_
     * @param sourceTable Source table name
     * @param oldAlbumId Old album ID
     * @return std::optional<int32_t> New album ID if found, nullopt otherwise
     */
    std::optional<int32_t> GetNewAlbumId(const std::string& sourceTable, int32_t oldAlbumId);

    std::shared_ptr<NativeRdb::RdbStore> sourceRdb_;
    std::shared_ptr<NativeRdb::RdbStore> destRdb_;
    const std::unordered_map<std::string, std::unordered_map<int32_t, int32_t>>& tableAlbumIdMap_;
};
} // namespace Media
} // namespace OHOS

#endif // FOUNDATION_MULTIMEDIA_MEDIA_LIBRARY_SERVICES_MEDIA_BACKUP_EXTENSION_INCLUDE_RESTORE_TAB_OLD_ALBUMS_CLONE_H
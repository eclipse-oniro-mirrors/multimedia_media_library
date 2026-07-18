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

#ifndef OHOS_MEDIA_CLOUD_DATA_CLEANER_H
#define OHOS_MEDIA_CLOUD_DATA_CLEANER_H

#include <string>
#include <memory>
#include "rdb_store.h"
#include "media_log.h"

namespace OHOS {
namespace Media {

/**
 * @brief Cloud data cleaner for FastRestore scenario.
 *
 * This class is used to clean cloud-only data and convert local-and-cloud data
 * to local-only data when the Huawei account is invalid (isAccountValid_ == false)
 * during FastRestore process.
 *
 * Reference implementation: CloudMediaAssetManager::ForceRetainDownloadCloudMediaEx
 * Key differences:
 * - Uses destRdb_ (swapped/old database) as the operation handle, not global GetRdbStore()
 * - No notification logic (no notifyFileIds collection, no NotifyPhotosChanged)
 * - Optimized SQL for batch updates (merge queries and updates, batch execution)
 */
class CloudDataCleaner {
public:
    /**
     * @brief Construct a new Cloud Data Cleaner object
     * @param rdbStore The database store handle to operate on (destRdb_ after swap)
     */
    explicit CloudDataCleaner(std::shared_ptr<NativeRdb::RdbStore> rdbStore);
    ~CloudDataCleaner() = default;

    /**
     * @brief Execute all cloud data cleanup steps
     * @return true if all steps succeeded, false if at least one step failed
     */
    bool CleanCloudData();

    /**
    * @brief Clean cloud data with real_lcd_visit_time = -3
    * Updates display_name to DELETE_DISPLAY_NAME for records where real_lcd_visit_time = -3
    * @return true if success, false if failed
    */
    bool CleanCloudDataWithNegativeVisitTime();

private:
    /**
     * @brief Update cloud media assets - mark pure cloud data for cleanup
     * Clears cloud-related fields and marks records as needing cleanup.
     * @return true if success, false if failed
     */
    bool UpdateCloudMediaAssets();

    /**
     * @brief Check if there are cloud assets to update.
     * @return true if there are assets to update, false otherwise
     */
    bool HasDataForUpdate();

    /**
     * @brief Delete analysis backup albums - remove albums from backup table
     * that are not associated with cleaned photos.
     */
    void DeleteAnalysisBackupAlbums();

    /**
     * @brief Delete empty cloud albums - remove cloud albums that contain no valid photos.
     */
    int32_t DeleteEmptyCloudAlbums();

    /**
     * @brief Clear deleted database records - remove records marked as TYPE_DELETED.
     */
    int32_t ClearDeletedDbData();

    /**
     * @brief Update both local and cloud assets - convert local-and-cloud data to local-only.
     * @return true if success, false if failed
     */
    bool UpdateBothLocalAndCloudAssets();

    /**
     * @brief Check if there are local-and-cloud assets to update.
     * @return true if there are assets to update, false otherwise
     */
    bool HasLocalAndCloudAssets();

    /**
     * @brief Check if there are cloud data with real_lcd_visit_time = -3 to update.
     * @return true if there are assets to update, false otherwise
     */
    bool HasNegativeVisitTimeData();

    /**
     * @brief Update local albums - clear cloud-related fields from local albums.
     */
    int32_t UpdateLocalAlbums();

    /**
     * @brief Execute SQL statement with bind parameters.
     * @param sql The SQL statement to execute
     * @param args Optional parameters for the SQL statement
     * @return NativeRdb::E_OK on success, error code otherwise
     */
    int32_t ExecuteSql(const std::string& sql, const std::vector<NativeRdb::ValueObject>& args = {});

    std::shared_ptr<NativeRdb::RdbStore> rdbStore_;

    // Constants
    static constexpr int BATCH_SIZE = 500;
    static constexpr int MAX_CYCLES = 5000;
    static constexpr int32_t POSITION_LOCAL = 1;
    static constexpr int32_t POSITION_CLOUD = 2;
    static constexpr int32_t POSITION_LOCAL_AND_CLOUD = 3;
    static constexpr int32_t DIRTY_TYPE_NEW = 1;
    static constexpr int32_t DIRTY_TYPE_DELETED = -1;
    static constexpr int32_t CLEAN_FLAG_NOT_CLEAN = 0;
    static constexpr int32_t CLEAN_FLAG_NEED_CLEAN = 1;
    static constexpr const char* DELETE_DISPLAY_NAME = "cloud_media_asset_deleted";
    static constexpr int32_t ALBUM_FROM_CLOUD = 2;   // 对应 PhotoAlbum 表的 album_is_local 云相册值
    static constexpr int32_t REAL_LCD_VISIT_TIME_INVALID = -3;

    // SQL statements for UpdateCloudMediaAssets
    const std::string SQL_UPDATE_CLOUD_MEDIA_ASSETS_BATCH =
        "UPDATE Photos SET clean_flag = 1, dirty = ?, cloud_version = ?, cloud_id = NULL, display_name = ?, "
        "real_lcd_visit_time = ? "
        "WHERE file_id IN ("
        "  SELECT file_id FROM Photos "
        "  WHERE position = ? AND display_name != ? "
        "  LIMIT ?"
        ");";

    const std::string SQL_CHECK_HAS_DATA_FOR_UPDATE =
        "SELECT 1 FROM Photos WHERE position = ? AND display_name != ? LIMIT 1;";

    // SQL statements for UpdateBothLocalAndCloudAssets
    const std::string SQL_UPDATE_LOCAL_AND_CLOUD_ASSETS_BATCH =
        "UPDATE Photos "
        "SET dirty = ?, "
        "    cloud_version = ?, "
        "    cloud_id = NULL, "
        "    position = ?, "
        "    south_device_type = ? "
        "WHERE file_id IN ("
        "  SELECT file_id FROM Photos "
        "  WHERE (position = ? "
        "      OR (position = ? AND (cloud_id IS NOT NULL OR cloud_version != ? "
        "                            OR (dirty != ? AND dirty != ?) "
        "                            OR south_device_type != ?))) "
        "  LIMIT ?"
        ");";

    const std::string SQL_CHECK_HAS_LOCAL_AND_CLOUD_ASSETS =
        "SELECT 1 FROM Photos "
        "WHERE (position = ? "
        "    OR (position = ? AND (cloud_id IS NOT NULL OR cloud_version != ? "
        "                          OR (dirty != ? AND dirty != ?) "
        "                          OR south_device_type != ?))) "
        "LIMIT 1;";
    
    // SQL statements for CleanCloudDataWithNegativeVisitTime
    const std::string SQL_UPDATE_NEGATIVE_VISIT_TIME_BATCH =
        "UPDATE Photos SET display_name = ? "
        "WHERE file_id IN ("
        "  SELECT file_id FROM Photos "
        "  WHERE real_lcd_visit_time = ? AND display_name != ? "
        "  LIMIT ?"
        ");";

    const std::string SQL_CHECK_HAS_NEGATIVE_VISIT_TIME_DATA =
        "SELECT 1 FROM Photos WHERE real_lcd_visit_time = ? AND display_name != ? LIMIT 1;";

    // SQL statements for DeleteAnalysisBackupAlbums
    const std::string SQL_DROP_ANALYSIS_BACKUP_TABLE =
        "DROP TABLE IF EXISTS PhotosAlbumBackupForSaveAnalysisData;";

    // SQL statements for DeleteEmptyCloudAlbums
    const std::string SQL_DELETE_EMPTY_CLOUD_ALBUMS =
        "DELETE FROM PhotoAlbum "
        "WHERE (is_local = ? AND album_id NOT IN ("
        "  SELECT DISTINCT owner_album_id FROM Photos WHERE clean_flag = ?"
        ")) OR dirty = ?;";

    // SQL statements for ClearDeletedDbData
    const std::string SQL_CLEAR_DELETED_DB_DATA =
        "DELETE FROM Photos WHERE dirty = ?;";

    // SQL statements for UpdateLocalAlbums
    const std::string SQL_UPDATE_LOCAL_ALBUMS_CLEAR_CLOUD_ID =
        "UPDATE PhotoAlbum SET dirty = 1, cloud_id = NULL;";
};

} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_CLOUD_DATA_CLEANER_H

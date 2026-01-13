/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_DATATRANSFER_MEDIA_LIBRARY_DB_UPGRADE_H
#define OHOS_MEDIA_DATATRANSFER_MEDIA_LIBRARY_DB_UPGRADE_H

#include <string>

#include "rdb_store.h"
#include "db_upgrade_utils.h"

namespace OHOS::Media {
namespace DataTransfer {
class MediaLibraryDbUpgrade {
public:
    int32_t OnUpgrade(NativeRdb::RdbStore &store);
    int32_t CreateClassifyAlbum(const std::string &newAlbumName, NativeRdb::RdbStore &store);
    bool CheckClassifyAlbumExist(const std::string &newAlbumName, NativeRdb::RdbStore &store,
        bool &isSetAggregateBit);
    void SetAggregateBit(const int32_t &bitPosition);

private:
    int32_t UpgradeAlbumPlugin(NativeRdb::RdbStore &store);
    int32_t UpgradePhotoAlbum(NativeRdb::RdbStore &store);
    int32_t UpgradePhotos(NativeRdb::RdbStore &store);
    int32_t UpgradePhotoMap(NativeRdb::RdbStore &store);
    int32_t MergeAlbumFromOldBundleNameToNewBundleName(NativeRdb::RdbStore &store);
    int32_t UpgradePhotosBelongsToAlbum(NativeRdb::RdbStore &store);
    void AggregateClassifyAlbum(NativeRdb::RdbStore &store);
    int32_t UpgradePhotoAlbumAddUploadStatusColumn(NativeRdb::RdbStore &store);

private:
    int32_t AddPhotosChangeTime(NativeRdb::RdbStore &store);
    int32_t AddPhotoAlbumChangeTime(NativeRdb::RdbStore &store);
    int32_t AddPhotosEffectModeColumn(NativeRdb::RdbStore &store);
    int32_t AddOwnerAlbumIdColumn(NativeRdb::RdbStore &store);
    int32_t AddFileSourceTypeColumn(NativeRdb::RdbStore &store);
    int32_t AddlPathColumn(NativeRdb::RdbStore &store);
    int32_t MoveSingleRelationshipToPhotos(NativeRdb::RdbStore &store);
    int32_t UpdatelPathColumn(NativeRdb::RdbStore &store);
    int32_t ExecSqlWithRetry(std::function<int32_t()> execSql);
    void ProcessClassifyAlbum(const std::string &newAlbumName, const std::vector<std::string> &oriAlbumNames,
        NativeRdb::RdbStore &store, bool &isSetAggregateBitSecond);
    void ProcessOcrClassifyAlbum(const std::string &newAlbumName, const std::vector<std::string> &ocrText,
        NativeRdb::RdbStore &store);

private:
    DbUpgradeUtils dbUpgradeUtils_;

private:
    const std::string SQL_PHOTO_ALBUM_TABLE_ADD_LPATH_COLUMN = "ALTER TABLE PhotoAlbum ADD COLUMN lpath TEXT;";
    const std::string SQL_PHOTO_ALBUM_TABLE_UPDATE_LPATH_COLUMN = "\
        UPDATE PhotoAlbum \
        SET lpath = \
        ( \
            SELECT \
                CASE \
                    WHEN COALESCE(album_plugin.lpath, '') <> '' THEN album_plugin.lpath \
                    WHEN COALESCE(plugin_v2.lpath, '') <> '' THEN plugin_v2.lpath \
                    WHEN album_type = 2048 THEN '/Pictures/'||PA.album_name \
                    WHEN album_type = 0 THEN '/Pictures/Users/'||PA.album_name \
                    ELSE '/Pictures/其它' \
                END AS s_lpath \
            FROM PhotoAlbum AS PA \
                LEFT JOIN album_plugin \
                ON COALESCE(PA.bundle_name, '') <> '' AND PA.bundle_name = album_plugin.bundle_name \
                LEFT JOIN album_plugin AS plugin_v2 \
                ON PA.album_type = 2048 AND PA.album_name = plugin_v2.album_name \
            WHERE PhotoAlbum.album_id = PA.album_id \
            ORDER BY \
                ( \
                CASE \
                    WHEN COALESCE(album_plugin.lpath, '') <> '' THEN album_plugin.priority \
                    WHEN COALESCE(plugin_v2.lpath, '') <> '' THEN plugin_v2.priority \
                    ELSE 1 \
                END \
                ) DESC, s_lpath ASC \
            LIMIT 1 \
        ) \
        WHERE COALESCE(PhotoAlbum.album_name, '') <> '' AND \
            PhotoAlbum.album_subtype != 1024 AND \
            COALESCE(PhotoAlbum.lpath, '') = '';";
    const std::string SQL_PHOTOS_TABLE_ADD_OWNER_ALBUM_ID = "\
        ALTER TABLE Photos ADD COLUMN owner_album_id INT DEFAULT 0;";
    const std::string SQL_PHOTOS_TABLE_ADD_FILE_SOURCE_TYPE = "\
        ALTER TABLE Photos ADD COLUMN file_source_type INT DEFAULT 0;";
    const std::string SQL_TEMP_PHOTO_MAP_TABLE_DROP = "DROP TABLE IF EXISTS temp_photo_map;";
    // 使用 group 分组查找 Photos 的一条关联关系
    const std::string SQL_TEMP_PHOTO_MAP_TABLE_CREATE = "\
        CREATE TABLE IF NOT EXISTS temp_photo_map \
        AS \
        SELECT MIN(map_album) AS map_album, \
            map_asset \
        FROM PhotoMap \
            INNER JOIN Photos \
            ON PhotoMap.map_asset=Photos.file_id \
        WHERE position IN (1, 3) AND \
            COALESCE(owner_album_id, 0) = 0 \
        GROUP BY map_asset;";
    const std::vector<std::string> SQL_TEMP_PHOTO_MAP_TABLE_CREATE_INDEX_ARRAY = {
        "CREATE INDEX IF NOT EXISTS unique_index_temp_photo_map_map_asset ON temp_photo_map ( \
            map_asset \
        );",
        "CREATE UNIQUE INDEX IF NOT EXISTS unique_index_temp_photo_map_all ON temp_photo_map ( \
            map_album, map_asset \
        );"};
    // owner_album_id 默认值是 0，使用 UNION 避免无查询结果时，被更新为 NULL
    const std::string SQL_PHOTOS_TABLE_UPDATE_ALBUM_ID = "\
        UPDATE Photos SET owner_album_id = \
        ( \
            SELECT map_album \
            FROM temp_photo_map \
            WHERE file_id=map_asset \
            UNION \
            SELECT 0 AS map_album \
            ORDER BY map_album DESC \
            LIMIT 1 \
        ) \
        WHERE COALESCE(owner_album_id, 0)=0;";
    const std::string SQL_PHOTO_MAP_TABLE_DELETE_SINGLE_RELATIONSHIP = "\
        DELETE FROM PhotoMap \
        WHERE EXISTS \
        ( \
            SELECT 1 \
            FROM temp_photo_map \
            WHERE PhotoMap.map_album=temp_photo_map.map_album AND \
                PhotoMap.map_asset=temp_photo_map.map_asset \
        );";
    /* Clear the cache table. */
    const std::string SQL_TEMP_ALBUM_BUNDLE_NAME_DELETE = "\
        DROP TABLE IF EXISTS temp_album_bundle_name;";

    /* Cache the mapping of old to new bundle names */
    const std::string SQL_TEMP_ALBUM_BUNDLE_NAME_CREATE =
        "CREATE TABLE IF NOT EXISTS temp_album_bundle_name ("
        "bundle_name_old TEXT,"
        "bundle_name_new TEXT"
        ");";

    const std::string SQL_TEMP_ALBUM_BUNDLE_NAME_INSERT =
        "INSERT INTO temp_album_bundle_name (bundle_name_old, bundle_name_new) VALUES "
        "('com.huawei.ohos.screenrecorder', 'com.huawei.hmos.screenrecorder'),"
        "('com.huawei.ohos.screenshot', 'com.huawei.hmos.screenshot');";

    /* Create the Album if it doesn't exist */
    const std::string SQL_PHOTO_ALBUM_INSERT_NEW_ALBUM = "\
        INSERT INTO PhotoAlbum( \
            album_type, \
            album_subtype, \
            album_name, \
            bundle_name, \
            lpath, \
            date_modified, \
            date_added \
        ) \
        SELECT \
            PA1.album_type, \
            PA1.album_subtype, \
            PA1.album_name, \
            M.bundle_name_new AS bundle_name, \
            CASE \
                WHEN COALESCE(album_plugin.lpath,'') <> '' THEN album_plugin.lpath \
                ELSE '/Pictures/'||PA1.album_name \
            END AS lpath, \
            strftime('%s000', 'now') AS date_modified, \
            strftime('%s000', 'now') AS date_added \
        FROM PhotoAlbum AS PA1 \
            INNER JOIN temp_album_bundle_name AS M \
            ON PA1.bundle_name=M.bundle_name_old \
            LEFT JOIN PhotoAlbum AS PA2 \
            ON M.bundle_name_new=PA2.bundle_name \
            LEFT JOIN album_plugin \
            ON M.bundle_name_new=album_plugin.bundle_name \
        WHERE PA2.bundle_name IS NULL;";
    /* Add the relationship in PhotoMap for new Album and Photo */
    const std::string SQL_PHOTO_MAP_INSERT_NEW_ALBUM = "\
        INSERT INTO PhotoMap( \
            map_album, \
            map_asset \
        ) \
        SELECT \
            PA2.album_id AS album_id_new, \
            PM.map_asset \
        FROM PhotoAlbum AS PA1 \
            INNER JOIN temp_album_bundle_name AS M \
            ON PA1.bundle_name=M.bundle_name_old \
            INNER JOIN PhotoAlbum AS PA2 \
            ON M.bundle_name_new=PA2.bundle_name \
            INNER JOIN PhotoMap AS PM \
            ON PA1.album_id=PM.map_album \
        EXCEPT \
        SELECT PA.album_id, \
            PM.map_asset \
        FROM PhotoAlbum AS PA \
            INNER JOIN temp_album_bundle_name AS M \
            ON PA.bundle_name=M.bundle_name_new \
            INNER JOIN PhotoMap AS PM \
            ON PA.album_id=PM.map_album \
        ; ";
    /* Remove the relationship in PhotoMap for old Album and Photo */
    const std::string SQL_PHOTO_MAP_DELETE_OLD_ALBUM = "\
        DELETE FROM PhotoMap \
        WHERE map_album IN \
        ( \
            SELECT \
                PA1.album_id \
            FROM PhotoAlbum AS PA1 \
                INNER JOIN temp_album_bundle_name AS M \
                ON PA1.bundle_name=M.bundle_name_old \
        );";
    /* Replace the relationship in Photos for new Album and Photo */
    const std::string SQL_PHOTOS_UPDATE_NEW_ALBUM = "\
        UPDATE Photos \
        SET owner_album_id=( \
            SELECT PA2.album_id \
            FROM PhotoAlbum AS PA1 \
                INNER JOIN temp_album_bundle_name AS M \
                ON PA1.bundle_name=M.bundle_name_old \
                INNER JOIN PhotoAlbum AS PA2 \
                ON M.bundle_name_new=PA2.bundle_name \
            WHERE Photos.owner_album_id=PA1.album_id \
        ) \
        WHERE owner_album_id IN \
        ( \
            SELECT \
                PA1.album_id \
            FROM PhotoAlbum AS PA1 \
                INNER JOIN temp_album_bundle_name AS M \
                ON PA1.bundle_name=M.bundle_name_old \
        ); ";
    /* Remove the relationship in Photos for old Album and Photo */
    const std::string SQL_PHOTO_ALBUM_DELETE_OLD_ALBUM = "\
        DELETE FROM PhotoAlbum \
        WHERE album_id IN \
        ( \
            SELECT \
                PA.album_id \
            FROM PhotoAlbum AS PA \
                INNER JOIN temp_album_bundle_name AS M \
                ON PA.bundle_name=M.bundle_name_old \
        ); ";
    const std::vector<std::string> SQL_MERGE_ALBUM_FROM_OLD_BUNDLE_NAME_TO_NEW_BUNDLE_NAME = {
        SQL_TEMP_ALBUM_BUNDLE_NAME_DELETE,
        SQL_TEMP_ALBUM_BUNDLE_NAME_CREATE,
        SQL_TEMP_ALBUM_BUNDLE_NAME_INSERT,
        SQL_PHOTO_ALBUM_INSERT_NEW_ALBUM,
        SQL_PHOTO_MAP_INSERT_NEW_ALBUM,
        SQL_PHOTO_MAP_DELETE_OLD_ALBUM,
        SQL_PHOTOS_UPDATE_NEW_ALBUM,
        SQL_PHOTO_ALBUM_DELETE_OLD_ALBUM,
        SQL_TEMP_ALBUM_BUNDLE_NAME_DELETE,
    };
    /* Create the Album if it doesn't exist */
    const std::string SQL_PHOTO_ALBUM_INSERT_OTHER_ALBUM = "\
        INSERT INTO PhotoAlbum( \
            album_type, \
            album_subtype, \
            album_name, \
            bundle_name, \
            lpath \
            ) \
        SELECT \
            2048 AS album_type, \
            2049 AS album_subtype, \
            '其它' AS album_name, \
            'com.other.album' AS bundle_name, \
            '/Pictures/其它' AS lpath \
        EXCEPT \
        SELECT \
            album_type, \
            album_subtype, \
            album_name, \
            bundle_name, \
            lpath \
        FROM PhotoAlbum ;";
    /* The Photo, doesn't belong to any album, should belongs to '其它' album */
    const std::string SQL_PHOTOS_UPDATE_OTHER_ALBUM = "\
        UPDATE Photos \
        SET owner_album_id=( \
            SELECT album_id \
            FROM PhotoAlbum \
            WHERE lpath='/Pictures/其它' \
            LIMIT 1 \
        ) \
        WHERE file_id IN \
        ( \
            SELECT file_id \
            FROM Photos \
                LEFT JOIN PhotoMap \
                ON Photos.file_id = PhotoMap.map_asset \
            WHERE PhotoMap.map_asset IS NULL AND \
                COALESCE(Photos.owner_album_id, 0) = 0 AND \
                Photos.position IN (1, 3) \
        );";
    const std::vector<std::string> SQL_PHOTOS_NEED_TO_BELONGS_TO_ALBUM = {
        SQL_PHOTO_ALBUM_INSERT_OTHER_ALBUM,
        SQL_PHOTOS_UPDATE_OTHER_ALBUM,
    };
    const std::string SQL_QUERY_CLASSIFY_ALBUM_EXIST = " \
        SELECT \
            count(1) AS count \
        FROM AnalysisAlbum \
        WHERE \
            album_type = ? \
        AND \
            album_subtype = ? \
        AND \
            album_name = ?;";
    const std::string SQL_CREATE_CLASSIFY_ALBUM = " \
        INSERT INTO AnalysisAlbum( \
            album_type, \
            album_subtype, \
            album_name, \
            is_local, \
            date_modified, \
            count \
            ) \
        VALUES( \
            ?, \
            ?, \
            ?, \
            1, \
            0, \
            0 \
        );";
    const std::string SQL_INSERT_MAPPING_RESULT = " \
        INSERT INTO AnalysisPhotoMap( \
            map_album, \
            map_asset \
            ) \
        SELECT DISTINCT ( \
            SELECT \
                album_id \
            FROM AnalysisAlbum \
            WHERE album_name = ? \
            ) AS album_id, \
            AnalysisPhotoMap.map_asset \
        FROM AnalysisAlbum \
        INNER JOIN AnalysisPhotoMap \
            ON AnalysisAlbum.album_id = AnalysisPhotoMap.map_album \
            AND AnalysisAlbum.album_type = ? \
            AND AnalysisAlbum.album_subtype = ? \
            AND AnalysisAlbum.album_name IN ";
    const std::string SQL_SELECT_CLASSIFY_OCR = " \
        WITH TempResult AS ( \
            SELECT DISTINCT ( \
                SELECT \
                    album_id \
                FROM AnalysisAlbum \
                WHERE album_name = ? \
                ) AS album_id, \
                AnalysisPhotoMap.map_asset as map_asset \
            FROM AnalysisAlbum \
            INNER JOIN AnalysisPhotoMap \
            ON AnalysisAlbum.album_id = AnalysisPhotoMap.map_album \
            AND AnalysisAlbum.album_type = ? \
            AND AnalysisAlbum.album_subtype = ? \
            AND AnalysisAlbum.album_name = ? \
            INNER JOIN tab_analysis_ocr \
            ON AnalysisPhotoMap.map_asset = tab_analysis_ocr.file_id \
            AND (";
    const std::string SQL_PHOTO_ALBUM_TABLE_ADD_UPDATE_STATUS_COLUMN =
        "ALTER TABLE PhotoAlbum ADD COLUMN upload_status INT DEFAULT 1 NOT NULL;";
    const std::vector<std::string> SQL_PHOTOS_CHANGE_TIME = {
        "ALTER TABLE Photos ADD COLUMN change_time BIGINT DEFAULT 0 NOT NULL;",
        "UPDATE Photos SET change_time = date_modified;"
    };
    const std::vector<std::string> SQL_PHOTO_ALBUM_CHANGE_TIME = {
        "ALTER TABLE PhotoAlbum ADD COLUMN change_time BIGINT DEFAULT 0 NOT NULL;",
        "UPDATE PhotoAlbum SET change_time = date_modified;"
    };
    const std::string SQL_PHOTOS_TABLE_ADD_EFFECT_MODE =
        "ALTER TABLE Photos ADD COLUMN moving_photo_effect_mode INT DEFAULT 0 NOT NULL;";
};
}  // namespace DataTransfer
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_DATATRANSFER_MEDIA_LIBRARY_DB_UPGRADE_H
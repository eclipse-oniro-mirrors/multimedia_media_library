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
#ifndef OHOS_MEDIA_DATATRANSFER_GALLERY_DB_UPGRADE_H
#define OHOS_MEDIA_DATATRANSFER_GALLERY_DB_UPGRADE_H

#include "rdb_store.h"
#include "db_upgrade_utils.h"

namespace OHOS::Media {
namespace DataTransfer {
class GalleryDbUpgrade {
public:
    int32_t OnUpgrade(std::shared_ptr<NativeRdb::RdbStore> galleryRdbPtr);

private:
    int32_t OnUpgrade(NativeRdb::RdbStore &store);
    int32_t AddPhotoQualityOfGalleryMedia(NativeRdb::RdbStore &store);
    int32_t AddResolutionOfGalleryMedia(NativeRdb::RdbStore &store);
    int32_t AddRelativeBucketIdOfGalleryAlbum(NativeRdb::RdbStore &store);
    int32_t GarbageAlbumUpgrade(NativeRdb::RdbStore &store);
    int32_t GarbageAlbumCheckOrAddRelativeBucketId(NativeRdb::RdbStore &store);
    int32_t GarbageAlbumCheckOrAddType(NativeRdb::RdbStore &store);
    int32_t AddIndexOfGalleryAlbum(NativeRdb::RdbStore &store);
    int32_t AddIndexAlbumIdOfGalleryAlbum(NativeRdb::RdbStore &store);
    int32_t AddIndexOfAlbumPlugin(NativeRdb::RdbStore &store);
    int32_t AddStoryChosenOfGalleryMedia(NativeRdb::RdbStore &store);
    int32_t CreateRelativeAlbumOfGalleryAlbum(NativeRdb::RdbStore &store);
    int32_t AddRelativeBucketIdColumn(NativeRdb::RdbStore &store);
    int32_t AddUserDisplayLevelIntoMergeTag(NativeRdb::RdbStore &store);
    int32_t AddHdcUniqueIdIntoGalleryMedia(NativeRdb::RdbStore &store);
    int32_t AddColumnsOfTOcrResult(NativeRdb::RdbStore &store);
    int32_t AddUploadStatusIntoGalleryAlbum(NativeRdb::RdbStore &store);
    int32_t AddHdcUploadStatusIntoGalleryAlbum(NativeRdb::RdbStore &store);

private:
    // Note: The column photo_quality's default value is 0.
    // But we should set it to 1 for the existing data.
    const std::string SQL_GALLERY_MEDIA_TABLE_ADD_PHOTO_QUALITY = "\
        ALTER TABLE gallery_media ADD COLUMN photo_quality INTEGER DEFAULT 1;";
    const std::string SQL_GALLERY_MEDIA_TABLE_ADD_RESOLUTION = "\
        ALTER TABLE gallery_media ADD COLUMN resolution TEXT;";
    const std::string SQL_GALLERY_ALBUM_TABLE_ADD_RELATIVE_BUCKET_ID = "\
        ALTER TABLE gallery_album ADD COLUMN relativeBucketId TEXT;";
    const std::string SQL_GARBAGE_ALBUM_TABLE_ADD_RELATIVE_BUCKET_ID = "\
        ALTER TABLE garbage_album ADD COLUMN relative_bucket_id TEXT;";
    const std::string SQL_GARBAGE_ALBUM_TABLE_ADD_TYPE = "\
        ALTER TABLE garbage_album ADD COLUMN type INTEGER DEFAULT 0;";
    const std::string SQL_GALLERY_ALBUM_INDEX_RELATIVE_BUCKET_ID = "\
        CREATE INDEX IF NOT EXISTS gallery_album_index_relativeBucketId ON gallery_album \
        ( \
            relativeBucketId \
        );";
    const std::string SQL_GALLERY_ALBUM_INDEX_ALBUM_ID = "\
        CREATE INDEX IF NOT EXISTS gallery_album_index_albumId ON gallery_album \
        ( \
            albumId \
        );";
    const std::string SQL_ALBUM_PLUGIN_INDEX_ALBUM_NAME = "\
        CREATE INDEX IF NOT EXISTS album_plugin_index_album_name ON album_plugin \
        ( \
            album_name \
        );";
    const std::string SQL_GALLERY_MEDIA_TABLE_ADD_STORY_CHOSEN = "\
        ALTER TABLE gallery_media ADD COLUMN story_chosen INTEGER DEFAULT 1;";
    const std::string CREATE_RELATE_ALBUM_TBL_SQL = "CREATE TABLE IF NOT EXISTS relative_album ("
        "relativeBucketId TEXT PRIMARY KEY, "
        "lPath TEXT NOT NULL);";

    const std::string INSERT_RELATE_ALBUM_TBL_SQL = "INSERT OR REPLACE INTO relative_album "
        "SELECT relativeBucketId, lPath FROM gallery_album "
        "WHERE COALESCE(relativeBucketId, '') <> '' GROUP BY relativeBucketId;";
    const std::string SQL_GALLERY_ALBUM_TABLE_ADD_UPLOAD_STATUS_COLUMN = "\
        ALTER TABLE gallery_album ADD COLUMN uploadStatus INT DEFAULT 1;";
    const std::string SQL_GALLERY_ALBUM_TABLE_ADD_HDC_UPLOAD_STATUS_COLUMN = "\
        ALTER TABLE gallery_album ADD COLUMN hdcUploadStatus INT DEFAULT 1;";

    const std::string UPDATE_USER_DISPLAY_LEVEL_SQL = R"(
        UPDATE merge_tag
        SET user_display_level = 1
        WHERE group_tag IN (
            SELECT DISTINCT t.group_tag
            FROM (
                SELECT
                    merge_face.hash,
                    merge_tag.tag_id,
                    merge_tag.group_tag,
                    merge_tag.user_operation,
                    merge_tag.is_hidden,
                    merge_tag.tag_name
                FROM
                    merge_face
                JOIN merge_tag ON
                    (merge_face.tag_id = merge_tag.tag_id
                        and merge_tag.album_type = 0)
            ) t
            GROUP BY
                t.group_tag
            HAVING
                (((t.tag_name is NOT NULL
                    AND t.tag_name != '')
                OR t.user_operation = 1
                OR t.user_operation = 2
                OR (t.user_operation = 0
                    AND count(DISTINCT t.hash) >= 5))
                AND count(DISTINCT t.hash) > 0)
                AND (t.is_hidden != -3)
        )
    )";
    const std::string SQL_GALLERY_MEDIA_TABLE_ADD_HDC_UNIQUE_ID_COLUMN = "\
        ALTER TABLE gallery_media ADD COLUMN hdc_unique_id INT DEFAULT 0;";
    const std::string SQL_T_OCR_RESULT_TABLE_ADD_VERSION_OCR = "\
        ALTER TABLE t_ocr_result ADD COLUMN version_ocr INTEGER NOT NULL DEFAULT 0;";
    const std::string SQL_T_OCR_RESULT_TABLE_ADD_WIDTH = "\
        ALTER TABLE t_ocr_result ADD COLUMN width INTEGER NOT NULL DEFAULT 0;";
    const std::string SQL_T_OCR_RESULT_TABLE_ADD_HEIGHT = "\
        ALTER TABLE t_ocr_result ADD COLUMN height INTEGER NOT NULL DEFAULT 0;";
    const std::string SQL_T_OCR_RESULT_TABLE_COPY_WIDTH_HEIGHT = "\
        UPDATE t_ocr_result SET \
        width = (SELECT width FROM gallery_media WHERE gallery_media.hash = t_ocr_result.hash), \
        height = (SELECT height FROM gallery_media WHERE gallery_media.hash = t_ocr_result.hash) \
        WHERE EXISTS (SELECT 1 FROM gallery_media WHERE gallery_media.hash = t_ocr_result.hash);";

private:
    DbUpgradeUtils dbUpgradeUtils_;
};
}  // namespace DataTransfer
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_DATATRANSFER_GALLERY_DB_UPGRADE_H
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
#ifndef OHOS_MEDIA_BACKUP_GALLERY_MEDIA_DAO_H
#define OHOS_MEDIA_BACKUP_GALLERY_MEDIA_DAO_H

#include <string>
#include <vector>

#include "rdb_store.h"

namespace OHOS::Media {
class GalleryMediaDao {
public:
    GalleryMediaDao() = default;
    GalleryMediaDao(std::shared_ptr<NativeRdb::RdbStore> galleryRdb) : galleryRdb_(galleryRdb)
    {}
    void SetGalleryRdb(std::shared_ptr<NativeRdb::RdbStore> galleryRdb);
    std::shared_ptr<NativeRdb::ResultSet> GetGalleryMedia(
        int32_t offset, int pageSize, bool shouldIncludeSd, bool hasLowQualityImage);
    std::shared_ptr<NativeRdb::ResultSet> GetCloudGalleryMedia(
        int32_t offset, int pageSize, bool shouldIncludeSd, bool hasLowQualityImage);
    std::shared_ptr<NativeRdb::ResultSet> GetHdcGalleryMedia(
        int32_t offset, int pageSize, bool shouldIncludeSd, bool hasLowQualityImage);
    int32_t GetGalleryMediaCount(bool shouldIncludeSd, bool hasLowQualityImage);
    int32_t GetCloudMetaCount(bool shouldIncludeSd, bool hasLowQualityImage);
    int32_t GetHdcMetaCount(bool shouldIncludeSd, bool hasLowQualityImage);
    int32_t GetNoNeedMigrateCount(bool shouldIncludeSd);

private:
    std::shared_ptr<NativeRdb::RdbStore> galleryRdb_;

private:
    const std::string SQL_GALLERY_MEDIA_QUERY_COUNT = "\
        SELECT COUNT(1) AS count \
        FROM gallery_media \
            LEFT JOIN gallery_album \
            ON gallery_media.albumId=gallery_album.albumId \
            LEFT JOIN relative_album \
            ON gallery_media.relative_bucket_id = relative_album.relativeBucketId \
        WHERE (local_media_id != -1) AND \
            (relative_bucket_id IS NULL OR \
                relative_bucket_id NOT IN ( \
                    SELECT DISTINCT relative_bucket_id \
                    FROM garbage_album \
                    WHERE type = 1 \
                ) \
            ) AND \
            (_size > 0 OR (1 = ? AND _size = 0 AND photo_quality = 0)) AND \
            _data NOT LIKE '/storage/emulated/0/Pictures/cloud/Imports%' AND \
            COALESCE(_data, '') <> '' AND \
            (1 = ? OR COALESCE(storage_id, 0) IN (0, 65537) ) \
        ORDER BY _id ASC ;";
    const std::string SQL_CLOUD_META_QUERY_COUNT = "\
        SELECT COUNT(1) AS count \
        FROM gallery_media \
            LEFT JOIN gallery_album \
            ON gallery_media.albumId=gallery_album.albumId \
            LEFT JOIN relative_album \
            ON gallery_media.relative_bucket_id = relative_album.relativeBucketId \
        WHERE (local_media_id == -1) AND COALESCE(uniqueId,'') <> '' AND \
            (relative_bucket_id IS NULL OR \
                relative_bucket_id NOT IN ( \
                    SELECT DISTINCT relative_bucket_id \
                    FROM garbage_album \
                    WHERE type = 1 \
                ) \
            ) AND \
            (_size > 0 OR (1 = ? AND _size = 0 AND photo_quality = 0)) AND \
            _data NOT LIKE '/storage/emulated/0/Pictures/cloud/Imports%' AND \
            COALESCE(_data, '') <> '' AND \
            (1 = ? OR COALESCE(storage_id, 0) IN (0, 65537) ) \
        ORDER BY _id ASC ;";
    const std::string SQL_HDC_META_QUERY_COUNT = "\
        SELECT COUNT(1) AS count \
        FROM gallery_media \
            LEFT JOIN gallery_album \
            ON gallery_media.albumId=gallery_album.albumId \
            LEFT JOIN relative_album \
            ON gallery_media.relative_bucket_id = relative_album.relativeBucketId \
        WHERE (local_media_id == -1) AND COALESCE(uniqueId,'') = '' AND COALESCE(hdc_unique_id,'') <> '' AND \
            (relative_bucket_id IS NULL OR \
                relative_bucket_id NOT IN ( \
                    SELECT DISTINCT relative_bucket_id \
                    FROM garbage_album \
                    WHERE type = 1 \
                ) \
            ) AND \
            (_size > 0 OR (1 = ? AND _size = 0 AND photo_quality = 0)) AND \
            _data NOT LIKE '/storage/emulated/0/Pictures/cloud/Imports%' AND \
            COALESCE(_data, '') <> '' AND \
            (1 = ? OR COALESCE(storage_id, 0) IN (0, 65537) ) \
        ORDER BY _id ASC ;";
    const std::string SQL_GALLERY_MEDIA_QUERY_FOR_RESTORE = "\
        SELECT \
            _id, \
            local_media_id, \
            localThumbPath, \
            localBigThumbPath, \
            _data, \
            _display_name, \
            description, \
            is_hw_favorite, \
            recycledTime, \
            _size, \
            duration, \
            media_type, \
            showDateToken, \
            height, \
            width, \
            title, \
            orientation, \
            date_modified, \
            date_added, \
            relative_bucket_id, \
            sourcePath, \
            is_hw_burst, \
            recycleFlag, \
            hash, \
            special_file_type, \
            first_update_time, \
            datetaken, \
            detail_time, \
            photo_quality, \
            thumbType, \
            gallery_media.albumId, \
            local_media_id, \
            uniqueId, \
            hdc_unique_id, \
            resolution, \
            CASE WHEN COALESCE(gallery_album.lPath, '') <> '' \
                THEN gallery_album.lPath \
                ELSE relative_album.lPath \
            END AS lPath, \
            latitude, \
            longitude, \
            story_id, \
            portrait_id, \
            story_chosen \
        FROM gallery_media \
            LEFT JOIN gallery_album \
            ON gallery_media.albumId=gallery_album.albumId \
            LEFT JOIN relative_album \
            ON gallery_media.relative_bucket_id = relative_album.relativeBucketId \
        WHERE _id > ? AND (local_media_id != -1) AND \
            (relative_bucket_id IS NULL OR \
                relative_bucket_id NOT IN ( \
                    SELECT DISTINCT relative_bucket_id \
                    FROM garbage_album \
                    WHERE type = 1 \
                ) \
            ) AND \
            (_size > 0 OR (1 = ? AND _size = 0 AND photo_quality = 0)) AND \
            _data NOT LIKE '/storage/emulated/0/Pictures/cloud/Imports%' AND \
            COALESCE(_data, '') <> '' AND \
            (1 = ? OR COALESCE(storage_id, 0) IN (0, 65537) ) \
        ORDER BY _id ASC \
        LIMIT ?;";
    const std::string SQL_GALLERY_CLOUD_QUERY_FOR_RESTORE = "\
        SELECT \
            _id, \
            local_media_id, \
            localThumbPath, \
            localBigThumbPath, \
            _data, \
            _display_name, \
            description, \
            is_hw_favorite, \
            recycledTime, \
            _size, \
            duration, \
            media_type, \
            showDateToken, \
            height, \
            width, \
            title, \
            orientation, \
            date_modified, \
            date_added, \
            relative_bucket_id, \
            sourcePath, \
            is_hw_burst, \
            recycleFlag, \
            hash, \
            special_file_type, \
            first_update_time, \
            datetaken, \
            detail_time, \
            photo_quality, \
            thumbType, \
            gallery_media.albumId, \
            local_media_id, \
            uniqueId, \
            hdc_unique_id, \
            resolution, \
            CASE WHEN COALESCE(gallery_album.lPath, '') <> '' \
                THEN gallery_album.lPath \
                ELSE relative_album.lPath \
            END AS lPath, \
            latitude, \
            longitude, \
            story_id, \
            portrait_id, \
            story_chosen \
        FROM gallery_media \
            LEFT JOIN gallery_album \
            ON gallery_media.albumId=gallery_album.albumId \
            LEFT JOIN relative_album \
            ON gallery_media.relative_bucket_id = relative_album.relativeBucketId \
        WHERE _id > ? AND (local_media_id == -1) AND COALESCE(uniqueId,'') <> '' AND \
            (relative_bucket_id IS NULL OR \
                relative_bucket_id NOT IN ( \
                    SELECT DISTINCT relative_bucket_id \
                    FROM garbage_album \
                    WHERE type = 1 \
                ) \
            ) AND \
            (_size > 0 OR (1 = ? AND _size = 0 AND photo_quality = 0)) AND \
            _data NOT LIKE '/storage/emulated/0/Pictures/cloud/Imports%' AND \
            COALESCE(_data, '') <> '' AND \
            (1 = ? OR COALESCE(storage_id, 0) IN (0, 65537) ) \
        ORDER BY _id ASC \
        LIMIT ?;";
    const std::string SQL_GALLERY_HDC_QUERY_FOR_RESTORE = "\
        SELECT \
            _id, \
            local_media_id, \
            localThumbPath, \
            localBigThumbPath, \
            _data, \
            _display_name, \
            description, \
            is_hw_favorite, \
            recycledTime, \
            _size, \
            duration, \
            media_type, \
            showDateToken, \
            height, \
            width, \
            title, \
            orientation, \
            date_modified, \
            date_added, \
            relative_bucket_id, \
            sourcePath, \
            is_hw_burst, \
            recycleFlag, \
            hash, \
            special_file_type, \
            first_update_time, \
            datetaken, \
            detail_time, \
            photo_quality, \
            thumbType, \
            gallery_media.albumId, \
            local_media_id, \
            uniqueId, \
            hdc_unique_id, \
            resolution, \
            CASE WHEN COALESCE(gallery_album.lPath, '') <> '' \
                THEN gallery_album.lPath \
                ELSE relative_album.lPath \
            END AS lPath, \
            latitude, \
            longitude, \
            story_id, \
            portrait_id, \
            story_chosen \
        FROM gallery_media \
            LEFT JOIN gallery_album \
            ON gallery_media.albumId=gallery_album.albumId \
            LEFT JOIN relative_album \
            ON gallery_media.relative_bucket_id = relative_album.relativeBucketId \
        WHERE _id >= ? AND (local_media_id == -1) AND \
            COALESCE(uniqueId,'') = '' AND COALESCE(hdc_unique_id,'') <> '' AND \
            (relative_bucket_id IS NULL OR \
                relative_bucket_id NOT IN ( \
                    SELECT DISTINCT relative_bucket_id \
                    FROM garbage_album \
                    WHERE type = 1 \
                ) \
            ) AND \
            (_size > 0 OR (1 = ? AND _size = 0 AND photo_quality = 0)) AND \
            _data NOT LIKE '/storage/emulated/0/Pictures/cloud/Imports%' AND \
            COALESCE(_data, '') <> '' AND \
            (1 = ? OR COALESCE(storage_id, 0) IN (0, 65537) ) \
        ORDER BY _id ASC \
        LIMIT ?;";
    const std::string SQL_GALLERY_MEDIA_QUERY_NO_NEED_MIGRATE_COUNT = "\
        SELECT COUNT(1) AS count \
        FROM gallery_media \
        WHERE (local_media_id = -1) OR \
            _data LIKE '/storage/emulated/0/Pictures/cloud/Imports%' OR \
            (0 = ? AND COALESCE(storage_id, 0) NOT IN (0, 65537));";
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_PHOTO_ALBUM_DAO_H
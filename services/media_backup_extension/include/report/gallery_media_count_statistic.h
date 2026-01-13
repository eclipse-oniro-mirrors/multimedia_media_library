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
#ifndef OHOS_MEDIA_BACKUP_GALLERY_MEDIA_COUNT_STATISTIC_H
#define OHOS_MEDIA_BACKUP_GALLERY_MEDIA_COUNT_STATISTIC_H

#include <string>
#include <vector>

#include "rdb_store.h"
#include "result_set_utils.h"
#include "media_backup_report_data_type.h"

namespace OHOS::Media {
class GalleryMediaCountStatistic {
public:
    GalleryMediaCountStatistic &SetGalleryRdb(std::shared_ptr<NativeRdb::RdbStore> galleryRdb)
    {
        this->galleryRdb_ = galleryRdb;
        return *this;
    }
    GalleryMediaCountStatistic &SetSceneCode(int32_t sceneCode)
    {
        this->sceneCode_ = sceneCode;
        return *this;
    }
    GalleryMediaCountStatistic &SetShouldIncludeSd(bool shouldIncludeSd)
    {
        this->shouldIncludeSd_ = shouldIncludeSd;
        return *this;
    }
    GalleryMediaCountStatistic &SetTaskId(const std::string &taskId)
    {
        this->taskId_ = taskId;
        return *this;
    }
    std::vector<AlbumMediaStatisticInfo> Load();

private:
    int32_t GetCount(const std::string &query, const std::vector<NativeRdb::ValueObject> &args = {});
    int32_t QueryGalleryAllCount(SearchCondition searchCondition);
    int32_t QueryAlbumGalleryCount(SearchCondition searchCondition);
    int32_t GetGalleryMediaAllRestoreCount(SearchCondition searchCondition);
    int32_t QueryGalleryCloneCount();
    int32_t QueryGallerySdCardCount(SearchCondition searchCondition);
    bool HasLowQualityImage();
    int32_t QueryGalleryAppTwinDataCount();
    int32_t QueryGalleryOnlyHDCDataCount(SearchCondition searchCondition);
    int32_t QueryGallerySizeUnnormalDataCount();
    int32_t QueryAlbumAllVideoCount(SearchCondition searchCondition);
    std::vector<AlbumStatisticInfo> QueryAlbumCountByName(
        const std::string &albumName, SearchCondition searchCondition);
    std::vector<AlbumStatisticInfo> QueryAlbumCountByLPath(const std::string &lPath, SearchCondition searchCondition);
    int32_t QueryLiveCount(int32_t searchType, int32_t mediaType);
    int32_t QueryTempCount(int32_t searchType, int32_t mediaType);
    AlbumMediaStatisticInfo GetAllStatInfo();
    AlbumMediaStatisticInfo GetAllImageStatInfo();
    AlbumMediaStatisticInfo GetAllVideoStatInfo();
    AlbumMediaStatisticInfo GetAllRestoreStatInfo();
    AlbumMediaStatisticInfo GetAllRestoreImageStatInfo();
    AlbumMediaStatisticInfo GetAllRestoreVideoStatInfo();
    AlbumMediaStatisticInfo GetSdCardStatInfo();
    AlbumMediaStatisticInfo GetDuplicateStatInfo();
    AlbumMediaStatisticInfo GetAppTwinStatInfo();
    AlbumMediaStatisticInfo GetOnlyHDCInfo();
    AlbumMediaStatisticInfo GetSizeUnnormalInfo();
    AlbumMediaStatisticInfo GetImageAlbumInfo();
    AlbumMediaStatisticInfo GetFavoriteAlbumStatInfo();
    AlbumMediaStatisticInfo GetTrashedAlbumStatInfo();
    std::vector<AlbumMediaStatisticInfo> GetAlbumInfoByName(const std::string &albumName);
    std::vector<AlbumMediaStatisticInfo> GetAlbumInfoByLPath(const std::string &lPath);
    AlbumMediaStatisticInfo GetGalleryAlbumCountInfo();
    AlbumMediaStatisticInfo GetVideoAlbumInfo();
    AlbumMediaStatisticInfo GetLiveStatInfo();
    AlbumMediaStatisticInfo GetTempInfo();
    AlbumMediaStatisticInfo GetNotSyncInfo();

private:
    std::shared_ptr<NativeRdb::RdbStore> galleryRdb_;
    int32_t sceneCode_;
    bool shouldIncludeSd_{false};
    std::string taskId_;

private:
    const std::string SQL_QUERY_ALL_GALLERY_COUNT = "\
        SELECT COUNT(1) AS count \
        FROM gallery_media \
        WHERE (0 = ? OR media_type = ?) AND \
            (-1 = ? OR 0 = ? AND COALESCE(local_media_id, 0) <> -4 OR 1 = ? AND COALESCE(local_media_id, 0) = -4) AND \
            (-1 = ? OR 0 = ? AND COALESCE(recycleFlag, 0) = 0 OR 1 = ? AND COALESCE(recycleFlag, 0) <> 0) AND \
            (-1 = ? OR 0 = ? AND local_media_id <> -1 OR 1 = ? AND local_media_id = -1) AND \
            (-1 = ? OR 0 = ? AND COALESCE(is_hw_favorite, 0) <> 1 OR 1 = ? AND COALESCE(is_hw_favorite, 0) = 1) AND \
            (-1 = ? OR 0 = ? AND is_hw_burst IN (1, 2) OR 1 = ? AND is_hw_burst = 1) \
        ;";
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
            (1 = ? OR COALESCE(storage_id, 0) IN (0, 65537) ) AND \
            (0 = ? OR media_type = ?) AND \
            (-1 = ? OR 0 = ? AND COALESCE(local_media_id, 0) <> -4 OR 1 = ? AND COALESCE(local_media_id, 0) = -4) AND \
            (-1 = ? OR 0 = ? AND COALESCE(recycleFlag, 0) = 0 OR 1 = ? AND COALESCE(recycleFlag, 0) <> 0) AND \
            (-1 = ? OR 0 = ? AND local_media_id <> -1 OR 1 = ? AND local_media_id = -1) AND \
            (-1 = ? OR 0 = ? AND COALESCE(is_hw_favorite, 0) <> 1 OR 1 = ? AND COALESCE(is_hw_favorite, 0) = 1) AND \
            (-1 = ? OR 0 = ? AND is_hw_burst IN (1, 2) OR 1 = ? AND is_hw_burst = 1) \
        ;";
    const std::string SQL_QUERY_GALLERY_SD_CARD_COUNT = "\
        SELECT COUNT(1) AS count \
        FROM gallery_media \
        WHERE COALESCE(storage_id, 0) NOT IN (0, 65537) AND \
            _size > 0 AND \
            (0 = ? OR media_type = ?) AND \
            (-1 = ? OR 0 = ? AND COALESCE(local_media_id, 0) <> -4 OR 1 = ? AND COALESCE(local_media_id, 0) = -4) AND \
            (-1 = ? OR 0 = ? AND COALESCE(recycleFlag, 0) = 0 OR 1 = ? AND COALESCE(recycleFlag, 0) <> 0) AND \
            (-1 = ? OR 0 = ? AND local_media_id <> -1 OR 1 = ? AND local_media_id = -1) AND \
            (-1 = ? OR 0 = ? AND COALESCE(is_hw_favorite, 0) <> 1 OR 1 = ? AND COALESCE(is_hw_favorite, 0) = 1) AND \
            (-1 = ? OR 0 = ? AND is_hw_burst IN (1, 2) OR 1 = ? AND is_hw_burst = 1) \
        ;";
    const std::string SQL_QUERY_GALLERY_MEDIA_ALL_VIDEO_COUNT = "\
        SELECT COUNT(1) AS count \
        FROM gallery_media \
            INNER JOIN gallery_album \
            ON gallery_media.albumId = gallery_album.albumId OR \
                gallery_media.relative_bucket_id = gallery_album.relativeBucketId \
        WHERE LOWER(gallery_album.lPath) <> LOWER('/Pictures/Screenshots') AND \
            (COALESCE(is_hw_burst, 1) <> 2) AND \
            (0 = ? OR media_type = ?) AND \
            (-1 = ? OR 0 = ? AND COALESCE(local_media_id, 0) <> -4 OR 1 = ? AND COALESCE(local_media_id, 0) = -4) AND \
            (-1 = ? OR 0 = ? AND COALESCE(recycleFlag, 0) = 0 OR 1 = ? AND COALESCE(recycleFlag, 0) <> 0) AND \
            (-1 = ? OR 0 = ? AND local_media_id <> -1 OR 1 = ? AND local_media_id = -1) AND \
            (-1 = ? OR 0 = ? AND COALESCE(is_hw_favorite, 0) <> 1 OR 1 = ? AND COALESCE(is_hw_favorite, 0) = 1) AND \
            (-1 = ? OR 0 = ? AND is_hw_burst IN (1, 2) OR 1 = ? AND is_hw_burst = 1) \
        ;";
    const std::string SQL_QUERY_ALBUM_GALLERY_COUNT = "\
        SELECT COUNT(1) AS count \
        FROM gallery_media \
        WHERE (COALESCE(is_hw_burst, 1) <> 2) AND \
            (0 = ? OR media_type = ?) AND \
            (-1 = ? OR 0 = ? AND COALESCE(local_media_id, 0) <> -4 OR 1 = ? AND COALESCE(local_media_id, 0) = -4) AND \
            (-1 = ? OR 0 = ? AND COALESCE(recycleFlag, 0) = 0 OR 1 = ? AND COALESCE(recycleFlag, 0) <> 0) AND \
            (-1 = ? OR 0 = ? AND local_media_id <> -1 OR 1 = ? AND local_media_id = -1) AND \
            (-1 = ? OR 0 = ? AND COALESCE(is_hw_favorite, 0) <> 1 OR 1 = ? AND COALESCE(is_hw_favorite, 0) = 1) AND \
            (-1 = ? OR 0 = ? AND is_hw_burst IN (1, 2) OR 1 = ? AND is_hw_burst = 1) \
        ;";
    const std::string SQL_QUERY_ALBUM_COUNT_BY_NAME = "\
        SELECT album_plugin.lpath AS lPath, \
            album_plugin.album_name AS albumName, \
            COUNT(1) AS count \
        FROM gallery_media \
            INNER JOIN gallery_album \
            ON gallery_media.albumId = gallery_album.albumId OR \
                gallery_media.relative_bucket_id = gallery_album.relativeBucketId \
            INNER JOIN album_plugin \
            ON LOWER(gallery_album.lPath) = LOWER(album_plugin.lpath) \
        WHERE album_plugin.album_name = ? AND \
            (COALESCE(is_hw_burst, 1) <> 2) AND \
            (0 = ? OR media_type = ?) AND \
            (-1 = ? OR 0 = ? AND COALESCE(local_media_id, 0) <> -4 OR 1 = ? AND COALESCE(local_media_id, 0) = -4) AND \
            (-1 = ? OR 0 = ? AND COALESCE(recycleFlag, 0) = 0 OR 1 = ? AND COALESCE(recycleFlag, 0) <> 0) AND \
            (-1 = ? OR 0 = ? AND local_media_id <> -1 OR 1 = ? AND local_media_id = -1) AND \
            (-1 = ? OR 0 = ? AND COALESCE(is_hw_favorite, 0) <> 1 OR 1 = ? AND COALESCE(is_hw_favorite, 0) = 1) AND \
            (-1 = ? OR 0 = ? AND is_hw_burst IN (1, 2) OR 1 = ? AND is_hw_burst = 1) \
        GROUP BY album_plugin.lpath, album_plugin.album_name ;";
    const std::string SQL_QUERY_ALBUM_COUNT_BY_LPATH = "\
        SELECT gallery_album.lPath, \
            CASE WHEN COALESCE(album_plugin.album_name, '') <> '' THEN album_plugin.album_name \
                ELSE gallery_album.albumName \
            END AS albumName, \
            COUNT(1) AS count \
        FROM gallery_media \
            INNER JOIN gallery_album \
            ON gallery_media.albumId = gallery_album.albumId OR \
                gallery_media.relative_bucket_id = gallery_album.relativeBucketId \
            LEFT JOIN album_plugin \
            ON LOWER(gallery_album.lPath) = LOWER(album_plugin.lpath) \
        WHERE LOWER(gallery_album.lPath) = LOWER(?) AND \
            (COALESCE(is_hw_burst, 1) <> 2) AND \
            (0 = ? OR media_type = ?) AND \
            (-1 = ? OR 0 = ? AND COALESCE(local_media_id, 0) <> -4 OR 1 = ? AND COALESCE(local_media_id, 0) = -4) AND \
            (-1 = ? OR 0 = ? AND COALESCE(recycleFlag, 0) = 0 OR 1 = ? AND COALESCE(recycleFlag, 0) <> 0) AND \
            (-1 = ? OR 0 = ? AND local_media_id <> -1 OR 1 = ? AND local_media_id = -1) AND \
            (-1 = ? OR 0 = ? AND COALESCE(is_hw_favorite, 0) <> 1 OR 1 = ? AND COALESCE(is_hw_favorite, 0) = 1) AND \
            (-1 = ? OR 0 = ? AND is_hw_burst IN (1, 2) OR 1 = ? AND is_hw_burst = 1) \
        GROUP BY gallery_album.lPath, gallery_album.albumName ;";
    const std::string SQL_QUERY_GALLERY_ALBUM_COUNT = "\
        SELECT COUNT(1) AS count \
        FROM gallery_album \
        WHERE albumId IN \
            ( \
                SELECT DISTINCT albumId \
                FROM gallery_media \
            ) OR \
            relativeBucketId IN \
            ( \
                SELECT DISTINCT relative_bucket_id \
                FROM gallery_media \
            ) \
        ;";
    const std::string SQL_QUERY_LIVE_COUNT = "\
        SELECT COUNT(1) AS count \
        FROM gallery_media \
        WHERE special_file_type IN (50, 1050) AND \
            local_media_id <> -4 AND \
            COALESCE(recycleFlag, 0) = 0 AND \
            (0 = ? OR local_media_id = -1) AND \
            (0 = ? OR media_type = ?);";
    const std::string SQL_QUERY_TEMP_COUNT = "\
        SELECT COUNT(1) AS count \
        FROM gallery_media \
        WHERE relative_bucket_id IN \
            ( \
                SELECT DISTINCT relative_bucket_id \
                FROM garbage_album \
                WHERE type = 1 \
            ) AND \
            (0 = ? OR local_media_id = -1) AND \
            (0 = ? OR media_type = ?);";
    const std::string SQL_QUERY_NOT_SYNC_COUNT = "\
        SELECT COUNT(1) AS count \
        FROM gallery_media \
        WHERE local_media_id = -3;";
    const std::string SQL_ONLY_HDC_META_QUERY_COUNT = "\
        SELECT COUNT(1) AS count \
        FROM gallery_media \
            LEFT JOIN gallery_album \
            ON gallery_media.albumId=gallery_album.albumId \
            LEFT JOIN relative_album \
            ON gallery_media.relative_bucket_id = relative_album.relativeBucketId \
        WHERE (0 = ? OR media_type = ?) AND \
            (local_media_id == -1) AND COALESCE(uniqueId,'') = '' AND COALESCE(hdc_unique_id,'') <> '' AND \
            (relative_bucket_id IS NULL OR \
                relative_bucket_id NOT IN ( \
                    SELECT DISTINCT relative_bucket_id \
                    FROM garbage_album \
                    WHERE type = 1 \
                ) \
            ) AND \
            (_size > 0 OR (_size = 0 AND photo_quality = 0)) AND \
            _data NOT LIKE '/storage/emulated/0/Pictures/cloud/Imports%' AND \
            COALESCE(_data, '') <> '' AND \
            (COALESCE(storage_id, 0) IN (0, 65537) ) \
        ORDER BY _id ASC ;";
    const std::string SQL_SIZE_UNNORMAL_META_QUERY_COUNT = "\
        SELECT COUNT(1) AS count \
        FROM gallery_media \
            LEFT JOIN gallery_album \
            ON gallery_media.albumId=gallery_album.albumId \
            LEFT JOIN relative_album \
            ON gallery_media.relative_bucket_id = relative_album.relativeBucketId \
        WHERE (relative_bucket_id IS NULL OR \
                relative_bucket_id NOT IN ( \
                    SELECT DISTINCT relative_bucket_id \
                    FROM garbage_album \
                    WHERE type = 1 \
                ) \
            ) AND \
            (COALESCE(_size, -1) < 0 OR (_size = 0 AND photo_quality <> 0)) AND \
            _data NOT LIKE '/storage/emulated/0/Pictures/cloud/Imports%' AND \
            COALESCE(_data, '') <> '' AND \
            (COALESCE(storage_id, 0) IN (0, 65537) ) \
        ORDER BY _id ASC ;";
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_BACKUP_GALLERY_MEDIA_COUNT_STATISTIC_H
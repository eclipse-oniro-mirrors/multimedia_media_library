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
#ifndef OHOS_MEDIA_BACKUP_PHOTOS_COUNT_STATISTIC_H
#define OHOS_MEDIA_BACKUP_PHOTOS_COUNT_STATISTIC_H

#include <string>
#include <vector>

#include "rdb_store.h"
#include "result_set_utils.h"
#include "media_backup_report_data_type.h"

namespace OHOS::Media {
class PhotosCountStatistic {
public:
    PhotosCountStatistic &SetMediaLibraryRdb(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb)
    {
        this->mediaLibraryRdb_ = mediaLibraryRdb;
        return *this;
    }
    PhotosCountStatistic &SetSceneCode(int32_t sceneCode)
    {
        this->sceneCode_ = sceneCode;
        return *this;
    }
    PhotosCountStatistic &SetTaskId(const std::string &taskId)
    {
        this->taskId_ = taskId;
        return *this;
    }
    PhotosCountStatistic &SetPeriod(const int32_t &period)
    {
        this->period_ = period;
        return *this;
    }
    std::vector<AlbumMediaStatisticInfo> Load();

private:
    int32_t GetCount(const std::string &query, const std::vector<NativeRdb::ValueObject> &args = {});
    int32_t QueryTotalCount(SearchCondition searchCondition);
    int32_t QueryAllRestoreCount(SearchCondition searchCondition);
    int32_t QueryPicturesTotalCount(SearchCondition searchCondition);
    std::vector<AlbumStatisticInfo> QueryAlbumCountByName(
        const std::string &albumName, SearchCondition searchCondition);
    int32_t QueryLiveCount(int32_t searchType);
    AlbumMediaStatisticInfo GetAllStatInfo();
    AlbumMediaStatisticInfo GetAllImageStatInfo();
    AlbumMediaStatisticInfo GetAllVideoStatInfo();
    AlbumMediaStatisticInfo GetAllRestoreStatInfo();
    AlbumMediaStatisticInfo GetAllRestoreImageStatInfo();
    AlbumMediaStatisticInfo GetAllRestoreVideoStatInfo();
    AlbumMediaStatisticInfo GetImageAlbumInfo();
    AlbumMediaStatisticInfo GetVideoAlbumInfo();
    std::vector<AlbumMediaStatisticInfo> GetAlbumInfoByName(const std::string &albumName);
    AlbumMediaStatisticInfo GetFavoriteAlbumStatInfo();
    AlbumMediaStatisticInfo GetTrashedAlbumStatInfo();
    AlbumMediaStatisticInfo GetHiddenAlbumStatInfo();
    AlbumMediaStatisticInfo GetGalleryAlbumCountInfo();
    AlbumMediaStatisticInfo GetGalleryDeletedAlbumCountInfo();
    AlbumMediaStatisticInfo GetLiveStatInfo();

private:
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;
    int32_t sceneCode_;
    std::string taskId_;
    int32_t period_ = 0;  // 0 - BEFORE, 1 - AFTER

private:
    const std::string SQL_PHOTOS_ALL_TOTAL_COUNT = "\
        SELECT COUNT(1) AS count \
        FROM Photos \
        WHERE (0 = ? OR media_type = ?) AND \
            (-1 = ? OR 0 = ? AND COALESCE(hidden, 0) = 0 OR 1 = ? AND COALESCE(hidden, 0) = 1) AND \
            (-1 = ? OR 0 = ? AND COALESCE(date_trashed, 0) = 0 OR 1 = ? AND COALESCE(date_trashed, 0) <> 0) AND \
            (-1 = ? OR 0 = ? AND position IN (1, 3) OR 1 = ? AND position = 2) AND \
            (-1 = ? OR 0 = ? AND COALESCE(is_favorite, 1) = 0 OR 1 = ? AND COALESCE(is_favorite, 1) = 1) AND \
            (-1 = ? OR 0 = ? AND subtype = 4 OR 1 = ? AND subtype = 4 AND COALESCE(burst_cover_level, 1) = 1) AND \
            (-1 = ? OR file_source_type = ?) \
        ;";
    const std::string SQL_PHOTOS_ALL_RESTORE_COUNT = "\
        SELECT COUNT(1) AS count \
        FROM Photos \
            LEFT JOIN PhotoAlbum \
            ON Photos.owner_album_id = PhotoAlbum.album_id \
        WHERE position IN (1, 3) AND \
            (COALESCE(PhotoAlbum.album_type, 0) != 2048 OR COALESCE(PhotoAlbum.album_name, '') != '.hiddenAlbum') AND \
            (0 = ? OR media_type = ?) AND \
            (-1 = ? OR 0 = ? AND COALESCE(Photos.hidden, 0) = 0 OR 1 = ? AND COALESCE(Photos.hidden, 0) = 1) AND \
            (-1 = ? OR 0 = ? AND COALESCE(date_trashed, 0) = 0 OR 1 = ? AND COALESCE(date_trashed, 0) <> 0) AND \
            (-1 = ? OR 0 = ? AND position IN (1, 3) OR 1 = ? AND position = 2) AND \
            (-1 = ? OR 0 = ? AND COALESCE(is_favorite, 1) = 0 OR 1 = ? AND COALESCE(is_favorite, 1) = 1) AND \
            (-1 = ? OR 0 = ? AND subtype = 4 OR 1 = ? AND subtype = 4 AND COALESCE(burst_cover_level, 1) = 1) AND \
            (-1 = ? OR file_source_type = ?) \
        ;";
    const std::string SQL_PHOTOS_PICTURES_TOTAL_COUNT = "\
        SELECT COUNT(1) AS count \
        FROM Photos \
        WHERE COALESCE(burst_cover_level, 1) = 1 AND \
            (0 = ? OR media_type = ?) AND \
            (-1 = ? OR 0 = ? AND COALESCE(hidden, 0) = 0 OR 1 = ? AND COALESCE(hidden, 0) = 1) AND \
            (-1 = ? OR 0 = ? AND COALESCE(date_trashed, 0) = 0 OR 1 = ? AND COALESCE(date_trashed, 0) <> 0) AND \
            (-1 = ? OR 0 = ? AND position IN (1, 3) OR 1 = ? AND position = 2) AND \
            (-1 = ? OR 0 = ? AND COALESCE(is_favorite, 1) = 0 OR 1 = ? AND COALESCE(is_favorite, 1) = 1) AND \
            (-1 = ? OR 0 = ? AND subtype = 4 OR 1 = ? AND subtype = 4 AND COALESCE(burst_cover_level, 1) = 1) AND \
            (-1 = ? OR file_source_type = ?) \
        ;";
    const std::string SQL_PHOTOS_COUNT_BY_ALBUM_NAME = "\
        SELECT PhotoAlbum.lpath, \
            album_plugin.album_name AS albumName, \
            COUNT(1) AS count \
        FROM Photos \
            INNER JOIN PhotoAlbum \
            ON Photos.owner_album_id = PhotoAlbum.album_id \
            INNER JOIN album_plugin \
            ON LOWER(PhotoAlbum.lpath) = LOWER(album_plugin.lpath) \
        WHERE album_plugin.album_name = ? AND \
            COALESCE(burst_cover_level, 1) = 1 AND \
            COALESCE(date_trashed, 0) = 0 AND \
            COALESCE(Photos.hidden, 0) = 0 AND \
            (0 = ? OR media_type = ?) AND \
            (-1 = ? OR 0 = ? AND COALESCE(Photos.hidden, 0) = 0 OR 1 = ? AND COALESCE(Photos.hidden, 0) = 1) AND \
            (-1 = ? OR 0 = ? AND COALESCE(date_trashed, 0) = 0 OR 1 = ? AND COALESCE(date_trashed, 0) <> 0) AND \
            (-1 = ? OR 0 = ? AND position IN (1, 3) OR 1 = ? AND position = 2) AND \
            (-1 = ? OR 0 = ? AND COALESCE(is_favorite, 1) = 0 OR 1 = ? AND COALESCE(is_favorite, 1) = 1) AND \
            (-1 = ? OR 0 = ? AND subtype = 4 OR 1 = ? AND subtype = 4 AND COALESCE(burst_cover_level, 1) = 1) AND \
            (-1 = ? OR file_source_type = ?) \
        GROUP BY PhotoAlbum.lpath, album_plugin.album_name;";
    const std::string SQL_PHOTO_ALBUM_COUNT = "\
        SELECT COUNT(1) AS count \
        FROM PhotoAlbum \
        WHERE album_id IN \
            ( \
                SELECT DISTINCT owner_album_id \
                FROM Photos \
            ) OR \
            album_id IN \
            ( \
                SELECT DISTINCT map_album \
                FROM PhotoMap \
            );";
    const std::string SQL_PHOTO_DELETED_ALBUM_COUNT = "\
        SELECT COUNT(1) AS count \
        FROM PhotoAlbum \
        WHERE dirty = ?;";
    const std::string SQL_PHOTOS_LIVE_COUNT = "\
        SELECT COUNT(1) AS count \
        FROM Photos \
        WHERE subtype = 3 AND \
            (0 = ? OR position = 2) AND \
            COALESCE(burst_cover_level, 1) = 1 AND \
            COALESCE(date_trashed, 0) = 0 AND \
            COALESCE(hidden, 0) = 0;";
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_BACKUP_PHOTOS_COUNT_STATISTIC_H
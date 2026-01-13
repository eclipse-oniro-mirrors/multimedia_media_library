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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_ASSET_RETAIN_COMPARE_DAO_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_ASSET_RETAIN_COMPARE_DAO_H

#include <string>
#include <vector>
#include <memory>

#include "rdb_store.h"
#include "cloud_media_pull_data_dto.h"
#include "result_set.h"

#include "abs_rdb_predicates.h"
#include "media_column.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_rdb_utils.h"

namespace OHOS::Media::CloudSync {

struct DuplicatePhotoInfo {
    int32_t fileId{0};
    std::string data;
    int32_t cleanFlag{0};
    int32_t position{0};
    bool isValid{false};
    int64_t real_lcd_visit_time {0};
};

class CloudMediaAssetCompareDao {
public:
    CloudMediaAssetCompareDao() : maxFileId_(0) {};
    ~CloudMediaAssetCompareDao() = default;

    void SetRdbStore(std::shared_ptr<MediaLibraryRdbStore> rdbStore)
    {
        this->rdbStore_ = rdbStore;
        this->maxFileId_ = GetPhotosMaxFileId();
    }

    int32_t GetMaxFileIdBeforeCompare() const
    {
        return maxFileId_ ;
    }

    DuplicatePhotoInfo FindDuplicatePhoto(const CloudMediaPullDataDto &pullData, int32_t maxFileId);

private:

    DuplicatePhotoInfo FindSamePhotoInAlbum(const CloudMediaPullDataDto &pullData, int32_t maxFileId);
    DuplicatePhotoInfo FindSamePhotoWithoutAlbum(const CloudMediaPullDataDto &pullData, int32_t maxFileId);
    DuplicatePhotoInfo FindSamePhotoBySourcePath(const CloudMediaPullDataDto &pullData, int32_t maxFileId);

    DuplicatePhotoInfo ExecuteDuplicateQuery(const std::string &querySql,
        const std::vector<NativeRdb::ValueObject> &params);

    int32_t GetPhotosMaxFileId();
    int32_t GetMediaTypeFromPullData(const CloudMediaPullDataDto &pullData);

private:
    int32_t maxFileId_;
    std::shared_ptr<MediaLibraryRdbStore> rdbStore_;

    const std::string SQL_PHOTOS_MAX_FILE_ID = "\
        SELECT \
            MAX(file_id) AS max_file_id\
        FROM Photos; \
        ";

    const std::string SQL_PHOTOS_FIND_SAME_FILE_IN_ALBUM = "\
        SELECT \
            p.file_id, \
            p.data, \
            p.clean_flag, \
            p.position, \
            p.real_lcd_visit_time \
        FROM \
            Photos AS p \
        WHERE \
            p.owner_album_id IN ( \
                SELECT album_id \
                FROM PhotosAlbumBackupForSaveAnalysisData \
                WHERE LOWER(lpath) = LOWER(?) \
            ) \
            AND p.file_id <= ? \
            AND p.display_name = ? \
            AND p.size = ? \
            AND (1 <> ? OR p.orientation = ?) \
        ORDER BY \
            p.clean_flag ASC \
        LIMIT 1;";

    const std::string SQL_PHOTOS_FIND_SAME_FILE_WITHOUT_ALBUM = "\
        SELECT \
            P.file_id, \
            P.data, \
            P.clean_flag, \
            P.position, \
            P.real_lcd_visit_time \
        FROM Photos AS P \
        WHERE file_id <= ? AND \
            display_name = ? AND \
            size = ? AND \
            (owner_album_id IS NULL OR owner_album_id = 0) AND \
            (1 <> ? OR orientation = ?) \
            ORDER BY p.clean_flag ASC \
        LIMIT 1;";

    const std::string SQL_PHOTOS_FIND_SAME_FILE_BY_SOURCE_PATH = "\
        SELECT \
            file_id, \
            data, \
            clean_flag, \
            position, \
            real_lcd_visit_time \
        FROM \
        ( \
            SELECT file_id, \
                data, \
                clean_flag, \
                position, \
                display_name, \
                size, \
                orientation, \
                hidden, \
                date_trashed, \
                source_path, \
                real_lcd_visit_time \
            FROM Photos \
                LEFT JOIN PhotosAlbumBackupForSaveAnalysisData \
                ON Photos.owner_album_id = PhotosAlbumBackupForSaveAnalysisData.album_id \
            WHERE PhotosAlbumBackupForSaveAnalysisData.album_id IS NULL AND \
                COALESCE(Photos.source_path, '') <> '' AND \
                ( \
                    COALESCE(Photos.hidden, 0) = 1 OR \
                    COALESCE(Photos.date_trashed, 0) <> 0 \
                ) \
        ) AS MISS \
        LEFT JOIN \
        ( \
            SELECT \
                ? AS source_path, \
                ? AS max_file_id, \
                ? AS display_name, \
                ? AS size, \
                ? AS picture_flag, \
                ? AS orientation \
        ) AS INPUT \
        ON 1 = 1 \
        WHERE MISS.file_id <= INPUT.max_file_id AND \
            MISS.display_name = INPUT.display_name AND \
            MISS.size = INPUT.size AND \
            ( 1 <> INPUT.picture_flag OR MISS.orientation = INPUT.orientation ) AND \
            LOWER(MISS.source_path) = LOWER(INPUT.source_path) \
        ORDER BY MISS.clean_flag ASC \
        LIMIT 1;";
};

}  // namespace OHOS::Media::CloudSync

#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_ASSET_RETAIN_COMPARE_DAO_H
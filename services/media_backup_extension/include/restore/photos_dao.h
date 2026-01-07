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
#ifndef OHOS_MEDIA_PHOTOS_DAO
#define OHOS_MEDIA_PHOTOS_DAO

#include <string>

#include "backup_const.h"
#include "rdb_store.h"
#include "media_log.h"

namespace OHOS::Media {
class PhotosDao {
public:
    struct PhotosRowData {
        int32_t fileId {0};
        std::string data;
        int32_t ownerAlbumId {0};
        std::string burstKey;
        int32_t cleanFlag {0};
        int32_t position {0};
        int32_t subtype {0};
        int32_t fileSourceType {0};
        bool IsValid()
        {
            return !cleanFlag && !data.empty() && fileId != 0;
        }
    };

    struct PhotosBasicInfo {
        int32_t maxFileId;
        int32_t count;
    };

public:
    void SetMediaLibraryRdb(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb)
    {
        this->mediaLibraryRdb_ = mediaLibraryRdb;
    }
    /**
     * @brief Find same file info by lPath, displayName, size, orientation.
     * lPath - if original fileInfo's lPath is empty, it will be ignored.
     * orientation - if original fileInfo's fileType is not Video(2), it will be ignored.
     */
    PhotosRowData FindSameFile(const FileInfo &fileInfo, int32_t maxFileId);
    PhotosBasicInfo GetBasicInfo();
    int32_t GetDirtyFilesCount();
    std::vector<PhotosRowData> GetDirtyFiles(int32_t offset);

private:
    PhotosRowData FindSameFileWithoutAlbum(const FileInfo &fileInfo, int32_t maxFileId);
    PhotosRowData FindSameFileInAlbum(const FileInfo &fileInfo, int32_t maxFileId);
    PhotosRowData FindSameFileBySourcePath(const FileInfo &fileInfo, int32_t maxFileId);
    PhotosRowData FindSameFileWithCloudId(const FileInfo &fileInfo, int32_t maxFileId);
    std::string ToString(const FileInfo &fileInfo);
    std::string ToString(const PhotosRowData &rowData);
    std::string ToLower(const std::string &str);

private:
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;

private:
    const std::string SOURCE_PATH_PREFIX = "/storage/emulated/0";
    const std::string SQL_PHOTOS_BASIC_INFO = "\
        SELECT \
            MAX(file_id) AS max_file_id, \
            COUNT(1) AS count \
        FROM Photos; \
    ";
    const std::string SQL_PHOTOS_FIND_SAME_FILE_IN_ALBUM = "\
        SELECT \
            p.file_id, \
            p.data, \
            p.clean_flag, \
            p.position \
        FROM \
        ( \
            SELECT album_id \
            FROM PhotoAlbum \
            WHERE LOWER(lpath) = LOWER(?) \
        ) \
        AS a \
        INNER JOIN \
        ( \
            SELECT \
                file_id, \
                data, \
                clean_flag, \
                position, \
                size, \
                orientation, \
                owner_album_id \
            FROM Photos \
            WHERE file_id <= ? AND \
                display_name = ? AND \
                size = ? AND \
                ( 1 <> ? OR orientation= ? ) \
        ) \
        AS p \
        ON a.album_id = p.owner_album_id \
        ORDER BY p.clean_flag ASC \
        LIMIT 1; ";
    const std::string SQL_PHOTOS_FIND_SAME_FILE_WITHOUT_ALBUM = "\
        SELECT \
            P.file_id, \
            P.data, \
            P.clean_flag, \
            P.position \
        FROM Photos AS P \
        WHERE file_id <= ? AND \
            display_name = ? AND \
            size = ? AND \
            (owner_album_id IS NULL OR owner_album_id = 0) AND \
            (1 <> ? OR orientation = ?) \
            ORDER BY p.clean_flag ASC \
        LIMIT 1;";
    const std::string SQL_PHOTOS_FIND_SAME_FILE_WITH_CLOUD_ID = "\
        SELECT \
            P.file_id, \
            P.data, \
            P.clean_flag, \
            P.position \
        FROM Photos AS P \
        WHERE file_id <= ? AND \
            cloud_id = ? \
        LIMIT 1;";
    const std::string SQL_PHOTOS_FIND_SAME_FILE_BY_SOURCE_PATH = "\
        SELECT \
            file_id, \
            data, \
            clean_flag, \
            position \
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
                source_path \
            FROM Photos \
                LEFT JOIN PhotoAlbum \
                ON Photos.owner_album_id = PhotoAlbum.album_id \
            WHERE PhotoAlbum.album_id IS NULL AND \
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
    const std::string SQL_PHOTOS_GET_DIRTY_FILES_COUNT =
        "SELECT count(1) as count FROM Photos WHERE sync_status = ?";
    const std::string SQL_PHOTOS_GET_DIRTY_FILES =
        "SELECT file_id, data, position, subtype FROM Photos WHERE sync_status = ? LIMIT ?, ?";
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_PHOTOS_DAO
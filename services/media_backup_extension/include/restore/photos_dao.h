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
        std::string displayName;
        int64_t fileSize {0};
        int32_t orientation {0};
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
    int32_t GetBackupMediaCount(const std::vector<int32_t> &mediaTypes, const std::vector<int32_t> &fileSourceTypes,
        const std::vector<int32_t> &positionTypes);
    int32_t GetBackupAudioCount(const std::vector<int32_t> &mediaTypes);
    int64_t GetAssetTotalSizeByFileSourceType(int32_t fileSourceType);
    std::unordered_set<std::string> GetExistingStoragePaths(const std::vector<std::string> &storagePaths,
        int32_t maxFileId);
    std::unordered_set<std::string> GetExistingData(const std::vector<std::string> &data, int32_t maxFileId);
    std::shared_ptr<NativeRdb::ResultSet> QueryCloneFileInfo(const std::vector<int32_t> &fileSourceTypes);
    int32_t InsertCloneFileInfo(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
        std::shared_ptr<NativeRdb::ResultSet> resultSet, AncoFileListClone ancoFileListClone,
        FileManagerFileListClone fileManagerFileListClone);
    int32_t GetCloneFileInfo(std::shared_ptr<NativeRdb::RdbStore> rdbStore, AncoFileListClone ancoFileListClone,
        FileManagerFileListClone fileManagerFileListClone);

private:
    PhotosRowData FindSameFileWithoutAlbum(const FileInfo &fileInfo, int32_t maxFileId);
    PhotosRowData FindSameFileInAlbum(const FileInfo &fileInfo, int32_t maxFileId);
    PhotosRowData FindSameFileBySourcePath(const FileInfo &fileInfo, int32_t maxFileId);
    PhotosRowData FindSameFileWithCloudId(const FileInfo &fileInfo, int32_t maxFileId);
    void ParseResultSetOfSameFile(PhotosDao::PhotosRowData &rowData, std::shared_ptr<NativeRdb::ResultSet> resultSet);
    std::string ToString(const FileInfo &fileInfo);
    std::string ToString(const PhotosRowData &rowData);
    std::string ToLower(const std::string &str);
    std::string GetPhotosSizeSqlByFileSourceType(int32_t fileSourceType);
    std::string GetThumbSizeSqlByFileSourceType(int32_t fileSourceType);
    std::string GetAudiosSizeSqlByFileSourceType(int32_t fileSourceType);

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
            p.position, \
            p.file_source_type \
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
                file_source_type, \
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
            P.position, \
            P.file_source_type \
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
            P.display_name, \
            P.size, \
            P.orientation, \
            P.clean_flag, \
            P.position, \
            P.file_source_type \
        FROM Photos AS P \
        WHERE file_id <= ? AND \
            cloud_id = ? \
        LIMIT 1;";
    const std::string SQL_PHOTOS_FIND_SAME_FILE_BY_SOURCE_PATH = "\
        SELECT \
            file_id, \
            data, \
            clean_flag, \
            position, \
            file_source_type \
        FROM \
        ( \
            SELECT file_id, \
                data, \
                clean_flag, \
                position, \
                file_source_type, \
                display_name, \
                size, \
                orientation, \
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
    const std::string SQL_PHOTOS_GET_MEDIA_COUNT =
        "SELECT count(1) as count FROM Photos "
        "WHERE position IN ({2}) AND sync_status = 0 AND clean_flag = 0 AND time_pending = 0 AND is_temp = 0 AND "
        "media_type IN ({0}) AND file_source_type IN ({1})";
    const std::string SQL_AUDIOS_GET_AUDIO_COUNT =
        "SELECT count(1) as count FROM Audios WHERE media_type IN ({0})";
    const std::string SQL_PHOTOS_GET_EXISTING_STORAGE_PATHS =
        "SELECT storage_path FROM Photos WHERE file_id <= ? AND file_source_type IN (?, ?) AND storage_path IN ({0})";
    const std::string SQL_PHOTOS_GET_EXISTING_DATA = "SELECT data FROM Photos WHERE file_id <= ? AND data IN ({0})";
    const std::string SQL_GET_CLONE_FILE_INFO = "\
        SELECT file_id, storage_path, display_name, media_type, size, date_modified, file_source_type \
        FROM Photos \
        WHERE file_source_type IN ({0})";
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_PHOTOS_DAO
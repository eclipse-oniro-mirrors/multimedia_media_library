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
#ifndef OHOS_MEDIA_PHOTO_ALBUM_DAO_H
#define OHOS_MEDIA_PHOTO_ALBUM_DAO_H

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <mutex>

#include "rdb_store.h"
#include "safe_map.h"

namespace OHOS::Media {
class PhotoAlbumDao {
public:
    struct PhotoAlbumRowData {
        int32_t albumId;
        int32_t albumType;
        int32_t albumSubType;
        std::string albumName;
        std::string lPath;
        std::string bundleName;
        int32_t priority;
        int32_t uploadStatus;
        PhotoAlbumRowData();
        bool IsValidSourceAlbum();
    };

public:
    void SetMediaLibraryRdb(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb)
    {
        this->mediaLibraryRdb_ = mediaLibraryRdb;
    }
    std::vector<PhotoAlbumRowData> GetPhotoAlbums();
    PhotoAlbumRowData GetPhotoAlbum(const std::string &lPath);
    PhotoAlbumRowData GetOrCreatePhotoAlbum(const PhotoAlbumRowData &album);
    PhotoAlbumRowData GetOrCreatePhotoAlbumForClone(const PhotoAlbumRowData &album);
    int32_t RestoreAlbums(std::vector<PhotoAlbumRowData> &photoAlbums);
    PhotoAlbumRowData BuildAlbumInfoOfRecorders();
    std::string ParseSourcePathToLPath(const std::string &sourcePath);
    PhotoAlbumRowData BuildAlbumInfoByLPath(const std::string &lPath);
    std::string ToString(const PhotoAlbumRowData &albumInfo)
    {
        return "albumId: " + std::to_string(albumInfo.albumId) + ", albumType: " + std::to_string(albumInfo.albumType) +
               ", albumSubType: " + std::to_string(albumInfo.albumSubType) + ", albumName: " + albumInfo.albumName +
               ", lPath: " + albumInfo.lPath + ", bundleName: " + albumInfo.bundleName;
    }
    void LoadPhotoAlbums();
    std::vector<int32_t> GetAlbumIdsFromCache();
    int32_t UpdateUploadStatus(const std::string &lPath, const int32_t uploadStatus);
    bool HasSameAlbumName(std::string albumName);

private:
    std::string FindUniqueAlbumName(const PhotoAlbumRowData &photoAlbum);
    bool CheckAlbumNameUnique(const std::string &albumName, const std::string &lPath);
    std::string ToString(const std::vector<NativeRdb::ValueObject> &bindArgs);
    PhotoAlbumRowData BuildAlbumInfoByLPath(
        const std::string &lPath, const int32_t albumType, const int32_t albumSubType);
    size_t FindRootPos(const std::string &path);
    PhotoAlbumRowData GetOrInsertIntoCache(const PhotoAlbumRowData &album);

private:
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;
    OHOS::SafeMap<std::string, PhotoAlbumRowData> photoAlbumCache_;
    std::vector<std::string> albumNameCache_;
    std::mutex cacheLock_;
    std::mutex photoAlbumCreateLock_;

private:
    const int32_t MAX_ALBUM_NAME_SEQUENCE = 1000;
    const std::string FIELD_NAME_ALBUM_ID = "album_id";
    const std::string FIELD_NAME_ALBUM_TYPE = "album_type";
    const std::string FIELD_NAME_ALBUM_SUBTYPE = "album_subtype";
    const std::string FIELD_NAME_ALBUM_NAME = "album_name";
    const std::string FIELD_NAME_BUNDLE_NAME = "bundle_name";
    const std::string FIELD_NAME_LPATH = "lpath";
    const std::string FIELD_NAME_PRIORITY = "priority";
    const std::string FIELD_UPLOAD_STATUS = "upload_status";
    const std::string SQL_PHOTO_ALBUM_SELECT = "\
        SELECT album_id, \
            album_type, \
            album_subtype, \
            album_name, \
            bundle_name, \
            lpath, \
            cloud_id, \
            relative_path, \
            priority, \
            upload_status \
        FROM PhotoAlbum \
        WHERE album_type != 1024 \
        ORDER BY album_id \
        LIMIT ?, ? ;";
    const std::string SQL_PHOTO_ALBUM_SELECT_BY_LPATH = "\
        SELECT album_id, \
            album_type, \
            album_subtype, \
            album_name, \
            bundle_name, \
            lpath, \
            cloud_id, \
            relative_path, \
            priority, \
            upload_status \
        FROM PhotoAlbum \
        WHERE LOWER(lpath) = LOWER(?) \
        ORDER BY album_id DESC \
        LIMIT 1 ;";
    // The albumName of PhotoAlbum, which is not in album_plugin, should be unique.
    const std::string SQL_PHOTO_ALBUM_CHECK_ALBUM_NAME_UNIQUE = "\
        SELECT COUNT(1) AS count \
        FROM PhotoAlbum \
            LEFT JOIN album_plugin \
            ON LOWER(PhotoAlbum.lpath) = (album_plugin.lpath) \
        WHERE LOWER(PhotoAlbum.album_name) = LOWER(?) AND \
            LOWER(PhotoAlbum.lpath) != LOWER(?) AND \
            album_plugin.lpath IS NULL ;";
    // create PhotoAlbum from albumInfo and album_plugin.
    // If the lPath is in album_plugin, use the album_plugin info to create.
    const std::string SQL_PHOTO_ALBUM_INSERT = "\
        INSERT INTO PhotoAlbum ( \
            album_type, \
            album_subtype, \
            album_name, \
            bundle_name, \
            lpath, \
            priority, \
            date_modified, \
            date_added, \
            upload_status \
        ) \
        SELECT \
            INPUT.album_type, \
            INPUT.album_subtype, \
            CASE \
                WHEN COALESCE(album_plugin.album_name, '') <> '' THEN album_plugin.album_name \
                WHEN NAME.count > 0 THEN INPUT.album_name || ' ' || NAME.count \
                ELSE INPUT.album_name \
            END AS album_name, \
            CASE \
                WHEN COALESCE(album_plugin.bundle_name, '') = '' THEN INPUT.bundle_name \
                ELSE album_plugin.bundle_name \
            END AS bundle_name, \
            CASE \
                WHEN COALESCE(album_plugin.lpath, '') = '' THEN INPUT.lpath \
                ELSE album_plugin.lpath \
            END AS lpath, \
            CASE \
                WHEN album_plugin.priority IS NULL THEN INPUT.priority \
                ELSE album_plugin.priority \
            END AS priority, \
            strftime('%s000', 'now') AS date_modified, \
            strftime('%s000', 'now') AS date_added, \
            INPUT.upload_status \
        FROM \
        ( \
            SELECT \
                ? AS album_type, \
                ? AS album_subtype, \
                ? AS album_name, \
                ? AS bundle_name, \
                ? AS lpath, \
                ? AS priority, \
                ? AS upload_status \
        ) AS INPUT \
        LEFT JOIN album_plugin \
            ON LOWER(INPUT.lpath)=LOWER(album_plugin.lpath) \
        LEFT JOIN PhotoAlbum \
            ON LOWER(INPUT.lpath)=LOWER(PhotoAlbum.lpath) \
        LEFT JOIN  \
        ( \
            SELECT album_name, COUNT(1) AS count \
            FROM PhotoAlbum \
            GROUP BY album_name \
        ) AS NAME \
            ON INPUT.album_name = NAME.album_name \
        WHERE PhotoAlbum.lpath IS NULL \
        LIMIT 1;";
    const std::string NESTED_ROOT_PATTERN =
        R"(^(/storage/emulated/[^/]+/storage/emulated/[^/]+/|/storage/[^/]+/storage/emulated/[^/]+/))";
    const std::string NON_NESTED_ROOT_PATTERN =
        R"(^(/storage/emulated/[^/]+/|/storage/[^/]+/))";
    const std::string SQL_PHOTO_ALBUM_UPDATE_UPLOAD_STATUS =
        "UPDATE PhotoAlbum SET upload_status = ? WHERE LOWER(lpath) = LOWER(?)";
};

class StringUtils {
    StringUtils() = delete;
    ~StringUtils() = delete;

public:
    static std::string ToLower(const std::string &str);
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_PHOTO_ALBUM_DAO_H
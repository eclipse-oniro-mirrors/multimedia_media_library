/*
 * Copyright (C) 2025-2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_PHOTO_OWNER_ALBUM_ID_OPERATION_H
#define OHOS_MEDIA_PHOTO_OWNER_ALBUM_ID_OPERATION_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "medialibrary_rdbstore.h"

namespace OHOS::Media {
class PhotoOwnerAlbumIdOperation {
public:
    struct MediaData {
        // PhotoAlbum fields
        std::string albumId;
        int32_t albumType;
        int32_t albumSubType;
        std::string albumName;
        std::string lPath;
        std::string bundleName;
        int32_t priority = 1;
        // Photos fields
        std::string fileId;
    };

private:
    std::vector<std::string> fileIds_;
    std::shared_ptr<MediaLibraryRdbStore> rdbStore_;

public:  // getter & setter
    PhotoOwnerAlbumIdOperation &SetFileIds(const std::vector<std::string> &fileIds)
    {
        this->fileIds_ = fileIds;
        return *this;
    }
    PhotoOwnerAlbumIdOperation &SetRdbStore(std::shared_ptr<MediaLibraryRdbStore> &rdbStore)
    {
        this->rdbStore_ = rdbStore;
        return *this;
    }

public:
    int32_t FixPhotoRelation();
    int32_t BuildAlbumBySourcePath(const std::string &sourcePath);

private:
    std::string ToStringWithComma(const std::vector<NativeRdb::ValueObject> &bindArgs) const;
    std::string ToStringWithComma(const std::vector<std::string> &fileIds) const;
    std::string ToStringWithCommaAndQuote(const std::vector<std::string> &fileIds) const;
    std::string ToString(const MediaData &albumInfo) const;
    std::string FillParams(const std::string &sql, const std::vector<std::string> &bindArgs);
    std::vector<std::string> GetFileIdsWithoutAlbum(const std::string &fileIdWithComma, bool &containsScreenVideo);
    std::vector<std::string> GetFileIdsWithAlbum(const std::string &fileIdWithComma, bool &containsScreenVideo);
    int32_t FixPhotoRelationForNoAlbum(const std::vector<std::string> &fileIds);
    int32_t FixPhotoRelationForHasAlbum(const std::vector<std::string> &fileIds);
    std::vector<std::string> GetOwnerAlbumIds(const std::vector<std::string> &fileIds);
    int32_t ResetAlbumDirty(const std::vector<std::string> &ownerAlbumIds);
    std::unordered_map<std::string, std::vector<MediaData>> GetPhotolPath(
        const std::vector<std::string> &fileIds, std::unordered_set<std::string> &lPathSet);
    MediaData GetPhotoAlbum(const std::string &lPath);
    std::unordered_map<std::string, MediaData> GetPhotoAlbums(const std::unordered_set<std::string> &lPathSet);
    std::string ParseSourcePathToLPath(const std::string &sourcePath);
    MediaData BuildAlbumInfoByLPath(const std::string &lPath);
    MediaData BuildAlbumInfoByLPath(const std::string &lPath, const int32_t albumType, const int32_t albumSubType);
    int32_t CreateAlbums(const std::unordered_set<std::string> &lPathSet);
    int32_t CreateAlbum(const MediaData &albumInfo);
    // std::unordered_map<std::string, std::vector<MediaData>> GetAssetExpectedAlbums(
    // const std::vector<std::string> &fileIds, const std::vector<std::string> lPaths);
    std::vector<std::string> GetFileIds(const std::vector<MediaData> &fileIds);
    int32_t UpdatePhotoOwnerAlbumId(const std::vector<std::string> &fileIds, const std::string &ownerAlbumId);
    int32_t BatchUpdatePhotoOwnerAlbumId(
        const std::unordered_map<std::string, std::vector<MediaData>> &photoTargetlPaths,
        const std::unordered_map<std::string, MediaData> &albumInfos);
    std::vector<std::string> GetScreenVideoFileIds();
    int32_t FixScreenVideoRelation();
    int32_t GetPhotoAlbumId(const std::string &lPath);
    int32_t CheckAndUpdateAlbumName(std::string &albumName, const int32_t albumType);

private:  // sqls
    const int32_t MEDIA_TYPE_VIDEO = 2;
    const std::string LPATH_SCREEN_RECORDS = "/Pictures/Screenrecords";
    const std::string LPATH_SCREEN_SHOTS = "/Pictures/Screenshots";
    const std::string GALLERT_ROOT_PATH = "/storage/emulated/";
    const std::string FILE_SEPARATOR = "/";
    const std::string SQL_NO_ALBUM_FILE_IDS = "\
        SELECT file_id, \
            media_type, \
            source_path \
        FROM Photos \
            LEFT JOIN PhotoAlbum \
            ON Photos.owner_album_id = PhotoAlbum.album_id \
        WHERE file_id IN ({0}) AND \
        album_id IS NULL;";
    const std::string SQL_HAS_ALBUM_FILE_IDS = "\
        SELECT file_id, \
            media_type, \
            lpath \
        FROM Photos \
            INNER JOIN PhotoAlbum \
            ON Photos.owner_album_id = PhotoAlbum.album_id \
        WHERE file_id IN ({0});";
    const std::string SQL_PHOTO_OWNER_ALBUM_ID_QUERY = "\
        SELECT owner_album_id \
        FROM Photos \
            INNER JOIN PhotoAlbum \
            ON Photos.owner_album_id = PhotoAlbum.album_id \
        WHERE file_id IN ({0}) \
        GROUP BY owner_album_id;";
    const std::string SQL_PHOTO_ALBUM_DIRTY_UPDATE = "\
        UPDATE PhotoAlbum \
        SET dirty = 1 \
        WHERE album_id IN ({0}) AND \
            dirty = 4;";
    const std::string SQL_PHOTOS_SOURCE_PATH_QUERY = "\
        SELECT file_id, \
            source_path \
        FROM Photos \
            LEFT JOIN PhotoAlbum \
            ON Photos.owner_album_id = PhotoAlbum.album_id \
        WHERE file_id IN ({0}) AND \
            album_id IS NULL;";
    const std::string SQL_PHOTO_ALBUM_QUERY = "\
        SELECT album_id, \
            lpath \
        FROM PhotoAlbum \
        WHERE album_type IN (0, 2048) AND \
            LOWER(lpath) = LOWER(?) \
        ORDER BY album_id DESC \
        LIMIT 1;";
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
            CASE \
                WHEN LOWER(INPUT.lpath) IN ( \
                    LOWER('/DCIM/Camera'), \
                    LOWER('/Pictures/Screenshots'), \
                    LOWER('/Pictures/Screenrecords') \
                    ) THEN 1 \
                WHEN NOT EXISTS (SELECT 1 FROM PhotoAlbum WHERE album_type IN (0, 2048) AND dirty <> 4) THEN 0 \
                WHEN EXISTS \
                (SELECT 1 FROM PhotoAlbum WHERE album_type IN (0, 2048) AND dirty <> 4 AND upload_status = 0) THEN 0 \
                ELSE 1 \
            END AS upload_status \
        FROM \
        ( \
            SELECT \
                ? AS album_type, \
                ? AS album_subtype, \
                ? AS album_name, \
                ? AS bundle_name, \
                ? AS lpath, \
                ? AS priority \
        ) AS INPUT \
        LEFT JOIN album_plugin \
            ON LOWER(INPUT.lpath)=LOWER(album_plugin.lpath) \
        LEFT JOIN PhotoAlbum \
            ON LOWER(INPUT.lpath)=LOWER(PhotoAlbum.lpath) \
        WHERE PhotoAlbum.lpath IS NULL \
        LIMIT 1";
    const std::string SQL_PHOTOS_OWNER_ALBUM_ID_UPDATE = "\
        UPDATE Photos \
        SET owner_album_id = ? \
        WHERE file_id IN ({0});";
    const std::string SQL_PHOTOS_SCREEN_VIDEO_QUERY = "\
        WITH SCREEN AS \
        ( \
            SELECT album_id \
            FROM PhotoAlbum \
            WHERE LOWER(lpath) = LOWER('/Pictures/Screenshots') \
        ) \
        SELECT file_id \
        FROM Photos \
            INNER JOIN SCREEN \
            ON owner_album_id = album_id \
        WHERE media_type = 2;";
    const std::string SQL_QUERY_ALBUM_NAME_UNIQUE = "\
        SELECT album_name \
        FROM PhotoAlbum \
        WHERE LOWER(album_name) = LOWER(?) AND dirty != 4;";
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_PHOTO_OWNER_ALBUM_ID_OPERATION_H
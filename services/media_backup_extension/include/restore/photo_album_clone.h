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
#ifndef OHOS_MEDIA_PHOTO_ALBUM_CLONE
#define OHOS_MEDIA_PHOTO_ALBUM_CLONE

#include <string>

#include "backup_const.h"
#include "photo_album_dao.h"
#include "rdb_store.h"

namespace OHOS::Media {
class PhotoAlbumClone {
public:
    /**
     * @brief Restore Start Event Handler.
     */
    void OnStart(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryOriginalRdb,
        std::shared_ptr<NativeRdb::RdbStore> mediaLibraryTargetRdb, bool isCloudRestoreSatisfied)
    {
        this->mediaLibraryOriginalRdb_ = mediaLibraryOriginalRdb;
        this->mediaLibraryTargetRdb_ = mediaLibraryTargetRdb;
        this->photoAlbumDao_.SetMediaLibraryRdb(mediaLibraryTargetRdb);
        isCloudRestoreSatisfied_ = isCloudRestoreSatisfied;
    }

    int32_t GetPhotoAlbumCountInOriginalDb();
    std::shared_ptr<NativeRdb::ResultSet> GetPhotoAlbumInOriginalDb(int32_t offset, int32_t pageSize);

    bool HasSameAlbum(const std::string &lPath, int32_t &albumId)
    {
        // Do not allow albums with empty lPath to be created.
        if (lPath.empty()) {
            return true;
        }
        PhotoAlbumDao::PhotoAlbumRowData albumInfo = this->photoAlbumDao_.GetPhotoAlbum(lPath);
        if (albumInfo.lPath.empty()) {
            return false;
        }
        albumId = albumInfo.albumId;
        return true;
    }

    bool HasSameAlbumName(const std::string &albumName)
    {
        return this->photoAlbumDao_.HasSameAlbumName(albumName);
    }
    void TRACE_LOG(const std::string &tableName, std::vector<AlbumInfo> &albumInfos);
    void TRACE_LOG(std::vector<PhotoAlbumDao::PhotoAlbumRowData> &albumInfos);
    int32_t FindUploadStatus(const AlbumInfo &albumInfo) const;
    int32_t UpdatePhotoAlbum(const AlbumInfo &albumInfo);

private:
    std::string ToString(const std::vector<NativeRdb::ValueObject> &bindArgs);
    std::string GetPhotoAlbumCountQuerySql();
    std::string GetPhotoAlbumSelectQuerySql();

private:
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryTargetRdb_;
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryOriginalRdb_;
    PhotoAlbumDao photoAlbumDao_;
    bool isCloudRestoreSatisfied_ {false};

private:
    const std::string SQL_PHOTO_ALBUM_COUNT_FOR_CLONE_LOCAL = "\
        SELECT COUNT(DISTINCT PhotoAlbum.album_id) AS count \
        FROM PhotoAlbum \
            LEFT JOIN PhotoMap \
            ON PhotoAlbum.album_id = PhotoMap.map_album \
            LEFT JOIN Photos AS P1 \
            ON PhotoMap.map_asset=P1.file_id \
            LEFT JOIN Photos AS P2 \
            ON PhotoAlbum.album_id=P2.owner_album_id \
        WHERE PhotoAlbum.dirty <> ? AND \
            (P1.file_id IS NOT NULL AND P1.position IN (1, 3) AND P1.file_source_type = 0 OR \
            P2.file_id IS NOT NULL AND P2.position IN (1, 3) AND P2.file_source_type = 0) ;";
    const std::string SQL_PHOTO_ALBUM_SELECT_FOR_CLONE_LOCAL = "\
        SELECT DISTINCT PhotoAlbum.* \
        FROM PhotoAlbum \
            LEFT JOIN PhotoMap \
            ON PhotoAlbum.album_id = PhotoMap.map_album \
            LEFT JOIN Photos AS P1 \
            ON PhotoMap.map_asset=P1.file_id \
            LEFT JOIN Photos AS P2 \
            ON PhotoAlbum.album_id=P2.owner_album_id \
        WHERE PhotoAlbum.dirty <> ? AND \
            (P1.file_id IS NOT NULL AND P1.position IN (1, 3) AND P1.file_source_type = 0 OR \
            P2.file_id IS NOT NULL AND P2.position IN (1, 3) AND P2.file_source_type = 0) \
        ORDER BY PhotoAlbum.album_id \
        LIMIT ?, ? ;";
    const std::string SQL_PHOTO_ALBUM_COUNT_FOR_CLONE_LOCAL_AND_CLOUD = "\
        SELECT COUNT(DISTINCT PhotoAlbum.album_id) AS count \
        FROM PhotoAlbum \
            LEFT JOIN PhotoMap \
            ON PhotoAlbum.album_id = PhotoMap.map_album \
            LEFT JOIN Photos AS P1 \
            ON PhotoMap.map_asset=P1.file_id \
            LEFT JOIN Photos AS P2 \
            ON PhotoAlbum.album_id=P2.owner_album_id \
        WHERE PhotoAlbum.dirty <> ? AND \
            (P1.file_id IS NOT NULL AND P1.position IN (1, 2, 3) AND P1.file_source_type = 0 OR \
            P2.file_id IS NOT NULL AND P2.position IN (1, 2, 3) AND P2.file_source_type = 0) ;";
    const std::string SQL_PHOTO_ALBUM_SELECT_FOR_CLONE_LOCAL_AND_CLOUD = "\
        SELECT DISTINCT PhotoAlbum.* \
        FROM PhotoAlbum \
            LEFT JOIN PhotoMap \
            ON PhotoAlbum.album_id = PhotoMap.map_album \
            LEFT JOIN Photos AS P1 \
            ON PhotoMap.map_asset=P1.file_id \
            LEFT JOIN Photos AS P2 \
            ON PhotoAlbum.album_id=P2.owner_album_id \
        WHERE PhotoAlbum.dirty <> ? AND \
            (P1.file_id IS NOT NULL AND P1.position IN (1, 2, 3) AND P1.file_source_type = 0 OR \
            P2.file_id IS NOT NULL AND P2.position IN (1, 2, 3) AND P2.file_source_type = 0) \
        ORDER BY PhotoAlbum.album_id \
        LIMIT ?, ? ;";
};
}  // namespace OHOS::Media

#endif  // OHOS_MEDIA_PHOTO_ALBUM_CLONE
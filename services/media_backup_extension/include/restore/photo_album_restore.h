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
#ifndef OHOS_MEDIA_PHOTO_ALBUM_RESTORE
#define OHOS_MEDIA_PHOTO_ALBUM_RESTORE

#include <string>
#include <vector>

#include "rdb_store.h"
#include "photo_album_dao.h"
#include "media_log.h"

namespace OHOS::Media {
class PhotoAlbumRestore {
public:
    struct GalleryAlbumRowData {
        std::string albumId;
        std::string relativeBucketId;
        std::string albumName;
        std::string bundleName;
        std::string lPath;
        int32_t priority = 1;
        int32_t uploadStatus = 0;
    };

    /**
     * @brief Restore Start Event Handler.
     */
    void OnStart(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, std::shared_ptr<NativeRdb::RdbStore> galleryRdb,
        bool isAccountValidAndSwitchOn = false)
    {
        this->mediaLibraryRdb_ = mediaLibraryRdb;
        this->galleryRdb_ = galleryRdb;
        this->photoAlbumDao_.SetMediaLibraryRdb(mediaLibraryRdb);
        this->isAccountValidAndSwitchOn_ = isAccountValidAndSwitchOn;
    }

    void TRACE_LOG(std::vector<GalleryAlbumRowData> &galleryAlbumInfos)
    {
        MEDIA_INFO_LOG(
            "Media_Restore: galleryAlbumInfos size : %{public}d", static_cast<int32_t>(galleryAlbumInfos.size()));
        for (auto &info : galleryAlbumInfos) {
            MEDIA_INFO_LOG("Media_Restore: restore album info: albumId = %{public}s, \
                relativeBucketId = %{public}s, \
                albumName = %{public}s, \
                bundleName = %{public}s, \
                lPath = %{public}s, \
                priority = %{public}d, \
                uploadStatus = %{public}d",
                info.albumId.c_str(),
                info.relativeBucketId.c_str(),
                info.albumName.c_str(),
                info.bundleName.c_str(),
                info.lPath.c_str(),
                info.priority,
                info.uploadStatus);
        }
    }

    void TRACE_LOG(std::vector<PhotoAlbumDao::PhotoAlbumRowData> &albumInfos)
    {
        MEDIA_INFO_LOG("Media_Restore: albumInfos size : %{public}d", static_cast<int32_t>(albumInfos.size()));
        for (auto &info : albumInfos) {
            MEDIA_INFO_LOG("Media_Restore: restore album info: albumId = %{public}d, \
                albumName = %{public}s, \
                albumType = %{public}d, \
                albumSubType = %{public}d, \
                lPath = %{public}s, \
                bundleName = %{public}s, \
                priority = %{public}d, \
                uploadStatus = %{public}d",
                info.albumId,
                info.albumName.c_str(),
                info.albumType,
                info.albumSubType,
                info.lPath.c_str(),
                info.bundleName.c_str(),
                info.priority,
                info.uploadStatus);
        }
    }

    int32_t Restore()
    {
        this->maxAlbumId_ = this->QueryMaxAlbumId("PhotoAlbum", "album_id");
        // fetch all albums from galleryRdb
        std::vector<GalleryAlbumRowData> galleryAlbumInfos = this->GetGalleryAlbums();
        TRACE_LOG(galleryAlbumInfos);
        // fetch all albums from mediaLibraryRdb
        std::vector<PhotoAlbumDao::PhotoAlbumRowData> albumInfos = this->photoAlbumDao_.GetPhotoAlbums();
        TRACE_LOG(albumInfos);
        // compare albums to find the album that need to be updated
        std::vector<PhotoAlbumDao::PhotoAlbumRowData> albumInfoToUpdate =
            this->GetAlbumInfoToUpdate(albumInfos, galleryAlbumInfos);
        this->UpdateAlbums(albumInfoToUpdate);
        // compare albums to find the album that need to be restored
        std::vector<PhotoAlbumDao::PhotoAlbumRowData> albumInfosToRestore =
            this->GetAlbumsToRestore(albumInfos, galleryAlbumInfos);
        TRACE_LOG(albumInfosToRestore);
        // restore albums
        return this->photoAlbumDao_.RestoreAlbums(albumInfosToRestore);
    }

    std::vector<PhotoAlbumDao::PhotoAlbumRowData> GetAlbumsToRestore(
        const std::vector<PhotoAlbumDao::PhotoAlbumRowData> &photoAlbums,
        const std::vector<GalleryAlbumRowData> &galleryAlbums);
    int32_t GetMaxAlbumId();

private:
    std::vector<GalleryAlbumRowData> GetGalleryAlbums();
    int32_t QueryMaxAlbumId(const std::string &tableName, const std::string &idName);
    std::vector<PhotoAlbumDao::PhotoAlbumRowData> GetAlbumInfoToUpdate(
        const std::vector<PhotoAlbumDao::PhotoAlbumRowData> &photoAlbums,
        const std::vector<GalleryAlbumRowData> &galleryAlbums);
    int32_t UpdateAlbums(const std::vector<PhotoAlbumDao::PhotoAlbumRowData> &albumInfos);
    GalleryAlbumRowData BuildAlbumInfoOfGalleryRecorders(const int32_t uploadStatus);
    int32_t ReadResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, GalleryAlbumRowData &albumInfo);
    void DetermineAlbumTypeByLPath(PhotoAlbumDao::PhotoAlbumRowData &albumRowData, const std::string &lPath);

private:
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;
    std::shared_ptr<NativeRdb::RdbStore> galleryRdb_;
    PhotoAlbumDao photoAlbumDao_;
    int32_t maxAlbumId_ = 0;
    bool isAccountValidAndSwitchOn_ = false;

private:
    const std::string GALLERY_ALBUM_ID = "albumId";
    const std::string GALLERY_ALBUM_NAME = "albumName";
    const std::string GALLERY_ALBUM_BUCKET_ID = "relativeBucketId";
    const std::string GALLERY_ALBUM_lPATH = "lPath";
    const std::string GALLERY_BUNDLE_NAME = "bundleName";
    const std::string GALLERY_PRIORITY = "priority";
    const std::string GALLERY_UPLOAD_STATUS = "uploadStatus";
    const std::string GALLERY_HDC_UPLOAD_STATUS = "hdcUploadStatus";

    const std::string SQL_GALLERY_ALBUM_SELECT = "\
        SELECT \
            albumId, \
            CASE \
                WHEN album_plugin.album_name IS NOT NULL OR album_plugin.album_name <> '' \
                    THEN album_plugin.album_name \
                WHEN album_plugin.album_name_en IS NOT NULL OR album_plugin.album_name_en <> '' \
                    THEN album_plugin.album_name_en \
                WHEN garbage_album.nick_name IS NOT NULL OR garbage_album.nick_name <> '' \
                    THEN garbage_album.nick_name \
                ELSE albumName \
            END AS albumName, \
            relativeBucketId, \
            gallery_album.lPath AS lPath, \
            COALESCE(album_plugin.bundle_name, '') AS bundleName, \
            COALESCE(priority, 1) AS priority, \
            COALESCE(uploadStatus, 0) AS uploadStatus, \
            COALESCE(hdcUploadStatus, 0) AS hdcUploadStatus \
        FROM gallery_album \
        LEFT JOIN garbage_album \
            ON gallery_album.lPath = garbage_album.nick_dir \
        LEFT JOIN album_plugin \
            ON gallery_album.lPath = album_plugin.lpath \
        WHERE gallery_album.lPath !='/Pictures/cloud/Imports' \
            AND gallery_album.lPath !='/Pictures/hiddenAlbum' \
        ORDER BY gallery_album.albumId \
        LIMIT ?, ? ;";
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_PHOTO_ALBUM_RESTORE
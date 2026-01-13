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
#ifndef OHOS_MEDIA_PHOTOS_RESTORE
#define OHOS_MEDIA_PHOTOS_RESTORE

#include <mutex>
#include <string>
#include <sstream>
#include <unordered_set>

#include "ffrt.h"
#include "ffrt_inner.h"
#include "rdb_store.h"
#include "photo_album_restore.h"
#include "photos_dao.h"
#include "photo_album_dao.h"
#include "gallery_media_dao.h"

namespace OHOS::Media {
class PhotosRestore {
public:
    /**
     * @brief Restore Start Event Handler.
     */
    void OnStart(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, std::shared_ptr<NativeRdb::RdbStore> galleryRdb)
    {
        this->SetMediaLibraryRdb(mediaLibraryRdb).SetGalleryRdb(galleryRdb).LoadBasicInfo();
    }

    /**
     * @brief Load the PhotoAlbum cache of target media_library.db for quick access.
     */
    void LoadPhotoAlbums()
    {
        this->photoAlbumDao_.LoadPhotoAlbums();
    }

    PhotosDao::PhotosRowData FindSameFile(const FileInfo &fileInfo)
    {
        int32_t maxFileId = this->photosBasicInfo_.maxFileId;
        return this->photosDao_.FindSameFile(fileInfo, maxFileId);
    }

    std::shared_ptr<NativeRdb::ResultSet> GetGalleryMedia(
        int32_t offset, int pageSize, bool shouldIncludeSd, bool hasLowQualityImage);
    std::shared_ptr<NativeRdb::ResultSet> GetCloudGalleryMedia(
        int32_t offset, int pageSize, bool shouldIncludeSd, bool hasLowQualityImage);
    std::shared_ptr<NativeRdb::ResultSet> GetHdcGalleryMedia(
        int32_t offset, int pageSize, bool shouldIncludeSd, bool hasLowQualityImage);
    int32_t GetGalleryMediaCount(bool shouldIncludeSd, bool hasLowQualityImage);
    int32_t GetCloudMetaCount(bool shouldIncludeSd, bool hasLowQualityImage);
    int32_t GetHdcMetaCount(bool shouldIncludeSd, bool hasLowQualityImage);
    void GetDuplicateData(int32_t duplicateDataCount);
    bool IsDuplicateData(const std::string &data);

public:
    std::string FindlPath(const FileInfo &fileInfo);
    std::string FindPackageName(const FileInfo &fileInfo);
    std::string FindBundleName(const FileInfo &fileInfo);
    int32_t FindAlbumId(const FileInfo &fileInfo);
    int32_t FindSubtype(const FileInfo &fileInfo);
    int32_t FindDirty(const FileInfo &fileInfo);
    std::string FindBurstKey(const FileInfo &fileInfo);
    int32_t FindBurstCoverLevel(const FileInfo &fileInfo);
    int64_t FindDateTrashed(const FileInfo &fileInfo);
    int32_t FindPhotoQuality(const FileInfo &fileInfo);
    int32_t FindMediaType(const FileInfo &fileInfo);
    std::string FindSourcePath(const FileInfo &fileInfo);
    int32_t FindStrongAssociation(const FileInfo &fileInfo);
    int32_t FindStrongAssociationByDisplayName(const FileInfo &fileInfo);
    int32_t FindCeAvailable(const FileInfo &fileInfo);
    int32_t FindCeAvailableByDisplayName(const FileInfo &fileInfo);
    bool FindIsLivePhoto(const FileInfo &fileInfo);
    PhotoAlbumDao::PhotoAlbumRowData GetPhotoAlbumDaoRowData(std::string& lPath);
    std::vector<int32_t> GetAlbumIdsFromPhotoAlbumCache();
    bool IsTrashedByRecycleFlag(const FileInfo &fileInfo);

    std::string ToLower(const std::string &str)
    {
        std::string lowerStr;
        std::transform(
            str.begin(), str.end(), std::back_inserter(lowerStr), [](unsigned char c) { return std::tolower(c); });
        return lowerStr;
    }
 
private:
    PhotosRestore &SetMediaLibraryRdb(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb)
    {
        this->mediaLibraryRdb_ = mediaLibraryRdb;
        this->photosDao_.SetMediaLibraryRdb(mediaLibraryRdb);
        this->photoAlbumDao_.SetMediaLibraryRdb(mediaLibraryRdb);
        return *this;
    }
    PhotosRestore &SetGalleryRdb(std::shared_ptr<NativeRdb::RdbStore> galleryRdb)
    {
        this->galleryRdb_ = galleryRdb;
        this->galleryMediaDao_.SetGalleryRdb(galleryRdb);
        return *this;
    }
    void LoadBasicInfo()
    {
        this->photosBasicInfo_ = this->photosDao_.GetBasicInfo();
    }
    PhotoAlbumDao::PhotoAlbumRowData FindAlbumInfo(const FileInfo &fileInfo);
    std::string ToString(const FileInfo &fileInfo)
    {
        std::stringstream ss;
        ss << "FileInfo[ fileId: " << fileInfo.fileIdOld << ", displayName: " << fileInfo.displayName
           << ", bundleName: " << fileInfo.bundleName << ", lPath: " << fileInfo.lPath
           << ", size: " << fileInfo.fileSize << ", fileType: " << fileInfo.fileType
           << ", oldPath: " << fileInfo.oldPath << ", sourcePath: " << fileInfo.sourcePath << " ]";
        return ss.str();
    }
    std::string GetSuffix(const std::string &displayName);
    bool IsEndWithEnhanced(const std::string &displayName);

private:
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;
    std::shared_ptr<NativeRdb::RdbStore> galleryRdb_;
    PhotosDao::PhotosBasicInfo photosBasicInfo_;
    PhotosDao photosDao_;
    PhotoAlbumDao photoAlbumDao_;
    ffrt::mutex duplicateDataUsedCountMutex_;
    std::unordered_map<std::string, int32_t> duplicateDataUsedCountMap_;
    GalleryMediaDao galleryMediaDao_;

private:
    const std::string SQL_GALLERY_MEDIA_QUERY_DUPLICATE_DATA = "\
        SELECT _data, count(1) as count \
        FROM gallery_media \
        GROUP BY _data \
        HAVING count(1) > 1 \
        LIMIT ?, ?;";
    const std::string SOURCE_PATH_PREFIX = "/storage/emulated/0";
};
}  // namespace OHOS::Media

#endif  // OHOS_MEDIA_PHOTOS_RESTORE
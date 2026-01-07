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
#include <regex>
#include <vector>

#include "photos_restore.h"
#include "backup_const.h"
#include "backup_file_utils.h"
#include "userfile_manager_types.h"
#include "rdb_store.h"
#include "result_set_utils.h"
#include "media_log.h"
#include "moving_photo_file_utils.h"
#include "photo_album_dao.h"
#include "album_plugin_config.h"
#include "backup_file_utils.h"
#include "mimetype_utils.h"

namespace OHOS::Media {
static const int32_t CLOUD_ENHANCEMENT_ALBUM = 1;
static const int32_t DUAL_ENHANCEMENT_PHOTO_QUALITY = 120;
static const int32_t SINGLE_CLOUD_ENHANCEMENT_PHOTO = 120;

/**
 * @brief Get the gallery_media to restore to Photos.
 */
std::shared_ptr<NativeRdb::ResultSet> PhotosRestore::GetGalleryMedia(
    int32_t minId, int pageSize, bool shouldIncludeSd, bool hasLowQualityImage)
{
    return this->galleryMediaDao_.GetGalleryMedia(minId, pageSize, shouldIncludeSd, hasLowQualityImage);
}

/**
 * @brief Get the gallery_media for cloud to restore to Photos.
 */
std::shared_ptr<NativeRdb::ResultSet> PhotosRestore::GetCloudGalleryMedia(
    int32_t minId, int pageSize, bool shouldIncludeSd, bool hasLowQualityImage)
{
    return this->galleryMediaDao_.GetCloudGalleryMedia(minId, pageSize, shouldIncludeSd, hasLowQualityImage);
}

/**
 * @brief Get the gallery_media for hdc to restore to Photos.
 */
std::shared_ptr<NativeRdb::ResultSet> PhotosRestore::GetHdcGalleryMedia(
    int32_t minId, int pageSize, bool shouldIncludeSd, bool hasLowQualityImage)
{
    return this->galleryMediaDao_.GetHdcGalleryMedia(minId, pageSize, shouldIncludeSd, hasLowQualityImage);
}

/**
 * @brief Get the row count of gallery_media.
 */
int32_t PhotosRestore::GetGalleryMediaCount(bool shouldIncludeSd, bool hasLowQualityImage)
{
    return this->galleryMediaDao_.GetGalleryMediaCount(shouldIncludeSd, hasLowQualityImage);
}

/**
 * @brief Get the row count of cloud_meta.
 */
int32_t PhotosRestore::GetCloudMetaCount(bool shouldIncludeSd, bool hasLowQualityImage)
{
    return this->galleryMediaDao_.GetCloudMetaCount(shouldIncludeSd, hasLowQualityImage);
}

/**
 * @brief Get the row count of hdc_meta.
 */
int32_t PhotosRestore::GetHdcMetaCount(bool shouldIncludeSd, bool hasLowQualityImage)
{
    return this->galleryMediaDao_.GetHdcMetaCount(shouldIncludeSd, hasLowQualityImage);
}

/**
 * @brief Get the PhotoAlbum basic info.
 */
PhotoAlbumDao::PhotoAlbumRowData PhotosRestore::FindAlbumInfo(const FileInfo &fileInfo)
{
    std::string lPath = fileInfo.lPath;
    // Scenario 2, WHEN FileInfo is in hidden album, THEN override lPath to the folder in sourcePath.
    // Scenario 3, WHEN FileInfo is not belongs to any album, THEN override lPath to the folder in sourcePath.
    if (fileInfo.lPath.empty() || this->ToLower(fileInfo.lPath) == this->ToLower(GALLERT_HIDDEN_ALBUM)) {
        lPath = this->photoAlbumDao_.ParseSourcePathToLPath(fileInfo.sourcePath);
        MEDIA_INFO_LOG("Media_Restore: fix lPath of album.fileInfo.lPath: %{public}s, "
                       "lPathFromSourcePath: %{public}s, lowercase: %{public}s, FileInfo Object: %{public}s",
            fileInfo.lPath.c_str(),
            lPath.c_str(),
            this->ToLower(lPath).c_str(),
            this->ToString(fileInfo).c_str());
    }
    PhotoAlbumDao::PhotoAlbumRowData albumInfo;
    // Scenario 1, WHEN FileInfo is in /Pictures/Screenshots and Video type, THEN redirect to /Pictures/Screenrecords
    if (this->ToLower(lPath) == this->ToLower(AlbumPlugin::LPATH_SCREEN_SHOTS) &&
        fileInfo.fileType == MediaType::MEDIA_TYPE_VIDEO) {
        albumInfo = this->photoAlbumDao_.BuildAlbumInfoOfRecorders();
        albumInfo = this->photoAlbumDao_.GetOrCreatePhotoAlbum(albumInfo);
        MEDIA_INFO_LOG("Media_Restore: screenshots redirect to screenrecords, fileInfo.lPath: %{public}s, "
                       "lPathForScreenshot: %{public}s, Object: %{public}s, albumInfo: %{public}s",
            fileInfo.lPath.c_str(),
            lPath.c_str(),
            this->ToString(fileInfo).c_str(),
            this->photoAlbumDao_.ToString(albumInfo).c_str());
        return albumInfo;
    }
    albumInfo = this->photoAlbumDao_.BuildAlbumInfoByLPath(lPath);
    return this->photoAlbumDao_.GetOrCreatePhotoAlbum(albumInfo);
}

/**
 * @brief Get the PhotoAlbum basic info.
 */
int32_t PhotosRestore::FindAlbumId(const FileInfo &fileInfo)
{
    PhotoAlbumDao::PhotoAlbumRowData albumInfo = this->FindAlbumInfo(fileInfo);
    CHECK_AND_PRINT_LOG(!albumInfo.lPath.empty(), "Can not find PhotoAlbum. fileInfo.lPath= %{public}s,"
        " fileInfo.sourcePath= %{public}s", fileInfo.lPath.c_str(), fileInfo.sourcePath.c_str());
    return albumInfo.albumId;
}

/**
 * @brief find lPath by FileInfo.
 */
std::string PhotosRestore::FindlPath(const FileInfo &fileInfo)
{
    PhotoAlbumDao::PhotoAlbumRowData albumInfo = this->FindAlbumInfo(fileInfo);
    CHECK_AND_PRINT_LOG(!albumInfo.lPath.empty(), "Can not find PhotoAlbum. fileInfo.lPath= %{public}s,"
        " fileInfo.sourcePath= %{public}s", fileInfo.lPath.c_str(), fileInfo.sourcePath.c_str());
    return albumInfo.lPath;
}

/**
 * @brief Find PackageName by FileInfo.
 */
std::string PhotosRestore::FindPackageName(const FileInfo &fileInfo)
{
    PhotoAlbumDao::PhotoAlbumRowData albumInfo = this->FindAlbumInfo(fileInfo);
    CHECK_AND_PRINT_LOG(!albumInfo.lPath.empty(), "Can not find PhotoAlbum. fileInfo.lPath= %{public}s,"
        " fileInfo.sourcePath= %{public}s", fileInfo.lPath.c_str(), fileInfo.sourcePath.c_str());
    return albumInfo.albumName;
}

/**
 * @brief Find BundleName by FileInfo.
 */
std::string PhotosRestore::FindBundleName(const FileInfo &fileInfo)
{
    PhotoAlbumDao::PhotoAlbumRowData albumInfo = this->FindAlbumInfo(fileInfo);
    CHECK_AND_PRINT_LOG(!albumInfo.lPath.empty(), "Can not find PhotoAlbum. fileInfo.lPath= %{public}s,"
        "fileInfo.sourcePath= %{public}s", fileInfo.lPath.c_str(), fileInfo.sourcePath.c_str());
    return albumInfo.bundleName;
}

/**
 * @brief Find BurstKey by FileInfo.
 */
std::string PhotosRestore::FindBurstKey(const FileInfo &fileInfo)
{
    CHECK_AND_RETURN_RET(fileInfo.burstKey.size() <= 0, fileInfo.burstKey);
    return "";
}

/**
 * @brief Find Dirty by FileInfo.
 */
int32_t PhotosRestore::FindDirty(const FileInfo &fileInfo)
{
    return static_cast<int32_t>(DirtyTypes::TYPE_NEW);
}

/**
 * @brief Find burst_cover_level by FileInfo.
 */
int32_t PhotosRestore::FindBurstCoverLevel(const FileInfo &fileInfo)
{
    // identify burst photo
    bool cond = (fileInfo.isBurst == static_cast<int32_t>(BurstCoverLevelType::COVER) ||
        fileInfo.isBurst == static_cast<int32_t>(BurstCoverLevelType::MEMBER));
    CHECK_AND_RETURN_RET(!cond, fileInfo.isBurst);
    return static_cast<int32_t>(BurstCoverLevelType::COVER);
}

/**
 * @brief Find subtype by FileInfo.
 */
int32_t PhotosRestore::FindSubtype(const FileInfo &fileInfo)
{
    CHECK_AND_RETURN_RET(fileInfo.burstKey.size() <= 0, static_cast<int32_t>(PhotoSubType::BURST));
    CHECK_AND_RETURN_RET(!BackupFileUtils::IsLivePhoto(fileInfo) &&
        !(fileInfo.subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)),
        static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    return static_cast<int32_t>(PhotoSubType::DEFAULT);
}

/**
 * @brief Find date_trashed by FileInfo.
 */
int64_t PhotosRestore::FindDateTrashed(const FileInfo &fileInfo)
{
    // prevent Photos marked as deleted when it's in use.
    CHECK_AND_RETURN_RET(fileInfo.recycleFlag != 0, 0);
    // LOG INFO for analyser.
    if (fileInfo.recycledTime != 0) {
        string fileName = fileInfo.displayName;
        MEDIA_WARN_LOG("the file :%{public}s is trash.", BackupFileUtils::GarbleFileName(fileName).c_str());
    }
    return fileInfo.recycledTime;
}

/**
 * @brief Get duplicate data in gallery db.
 */
void PhotosRestore::GetDuplicateData(int32_t duplicateDataCount)
{
    CHECK_AND_RETURN(duplicateDataCount > 0);
    std::string querySql = this->SQL_GALLERY_MEDIA_QUERY_DUPLICATE_DATA;
    int rowCount = 0;
    int offset = 0;
    int pageSize = 200;
    do {
        std::vector<NativeRdb::ValueObject> params = {offset, pageSize};
        CHECK_AND_BREAK_ERR_LOG(this->galleryRdb_ != nullptr, "Media_Restore: galleryRdb_ is null.");
        auto resultSet = this->galleryRdb_->QuerySql(querySql, params);
        CHECK_AND_BREAK_ERR_LOG(resultSet != nullptr, "Query resultSql is null.");
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            std::string data = GetStringVal(GALLERY_FILE_DATA, resultSet);
            int32_t count = GetInt32Val(CUSTOM_COUNT, resultSet);
            this->duplicateDataUsedCountMap_[ToLower(data)] = 0;
            MEDIA_INFO_LOG("Get duplicate data: %{public}s, count: %{public}d",
                BackupFileUtils::GarbleFilePath(data, DEFAULT_RESTORE_ID).c_str(),
                count);
        }
        // Check if there are more rows to fetch.
        resultSet->GetRowCount(rowCount);
        offset += pageSize;
        resultSet->Close();
    } while (rowCount > 0);
}

/**
 * @brief Check if it is duplicate data in gallery db.
 */
bool PhotosRestore::IsDuplicateData(const std::string &data)
{
    std::lock_guard<ffrt::mutex> lock(this->duplicateDataUsedCountMutex_);
    CHECK_AND_RETURN_RET(this->duplicateDataUsedCountMap_.count(ToLower(data)) != 0, false);
    this->duplicateDataUsedCountMap_[ToLower(data)]++;
    return this->duplicateDataUsedCountMap_.at(ToLower(data)) > 1;
}

/**
 * @brief Find PhotoQuality by FileInfo.
 */
int32_t PhotosRestore::FindPhotoQuality(const FileInfo &fileInfo)
{
    bool cond = (fileInfo.photoQuality == 1);
    CHECK_AND_RETURN_RET(!cond, 0);
    return fileInfo.photoQuality;
}

/**
 * @brief Find suffix of displayName. include dot, e.g. ".jpg"
 * @return return the suffix of displayName. "" if not found.
 */
std::string PhotosRestore::GetSuffix(const std::string &displayName)
{
    size_t dotPos = displayName.rfind('.');
    /* without dot, e.g. "jpg" */
    CHECK_AND_RETURN_RET(dotPos == std::string::npos, this->ToLower(displayName.substr(dotPos + 1)));
    return "";
}

/**
 * @brief Find media_type by FileInfo.
 */
int32_t PhotosRestore::FindMediaType(const FileInfo &fileInfo)
{
    int32_t dualMediaType = fileInfo.fileType;
    bool cond = (dualMediaType == DUAL_MEDIA_TYPE::IMAGE_TYPE || dualMediaType == DUAL_MEDIA_TYPE::VIDEO_TYPE);
    CHECK_AND_RETURN_RET(!cond,
        (dualMediaType == DUAL_MEDIA_TYPE::VIDEO_TYPE ? MediaType::MEDIA_TYPE_VIDEO : MediaType::MEDIA_TYPE_IMAGE));
    std::string suffix = this->GetSuffix(fileInfo.displayName);
    MediaType mediaType = MimeTypeUtils::GetMediaTypeFromMimeType(MimeTypeUtils::GetMimeTypeFromExtension(suffix));
    MEDIA_INFO_LOG("Media_Restore: correct mediaType from %{public}d to %{public}d, displayName: %{public}s",
        fileInfo.fileType,
        mediaType,
        fileInfo.displayName.c_str());
    return mediaType;
}

/**
 * @brief Find source_path for the target device by FileInfo.
 */
std::string PhotosRestore::FindSourcePath(const FileInfo &fileInfo)
{
    CHECK_AND_RETURN_RET(fileInfo.sourcePath.empty(), fileInfo.sourcePath);
    std::string lPath = "/Pictures/其它";
    if (!fileInfo.lPath.empty()) {
        lPath = fileInfo.lPath;
    } else {
        MEDIA_ERR_LOG("Media_Restore: fileInfo.lPath is empty. Use default lPath: %{public}s", lPath.c_str());
    }
    return this->SOURCE_PATH_PREFIX + lPath + "/" + fileInfo.displayName;
}

/**
 * @brief Find enhancement photo quality for the target device by FileInfo.
 */
int32_t PhotosRestore::FindStrongAssociation(const FileInfo &fileInfo)
{
    CHECK_AND_RETURN_RET(fileInfo.photoQuality != DUAL_ENHANCEMENT_PHOTO_QUALITY, CLOUD_ENHANCEMENT_ALBUM);
    return 0;
}

/**
 * @brief Find enhancement photo quality for the target device by FileInfo displayName, only for cloud backup restore
 */
int32_t PhotosRestore::FindStrongAssociationByDisplayName(const FileInfo &fileInfo)
{
    return IsEndWithEnhanced(fileInfo.displayName) ? CLOUD_ENHANCEMENT_ALBUM : 0;
}

/**
 * @brief Find cloud enhancement available for the target device by FileInfo.
 */
int32_t PhotosRestore::FindCeAvailable(const FileInfo &fileInfo)
{
    CHECK_AND_RETURN_RET(fileInfo.photoQuality != DUAL_ENHANCEMENT_PHOTO_QUALITY, SINGLE_CLOUD_ENHANCEMENT_PHOTO);
    return 0;
}

/**
 * @brief Find cloud enhancement available for the target device by FileInfo displayName, only for cloud backup restore
 */
int32_t PhotosRestore::FindCeAvailableByDisplayName(const FileInfo &fileInfo)
{
    return IsEndWithEnhanced(fileInfo.displayName) ? SINGLE_CLOUD_ENHANCEMENT_PHOTO : 0;
}

/**
 * @brief Find if it is live photo by file format.
 */
bool PhotosRestore::FindIsLivePhoto(const FileInfo &fileInfo)
{
    return MovingPhotoFileUtils::IsLivePhoto(fileInfo.filePath);
}

bool PhotosRestore::IsEndWithEnhanced(const std::string &displayName)
{
    std::regex pattern(R"(.*_enhanced(\.[^.]+)$)");
    return std::regex_match(displayName, pattern);
}

PhotoAlbumDao::PhotoAlbumRowData PhotosRestore::GetPhotoAlbumDaoRowData(string& lPath)
{
    return this->photoAlbumDao_.GetPhotoAlbum(lPath);
}

std::vector<int32_t> PhotosRestore::GetAlbumIdsFromPhotoAlbumCache()
{
    return this->photoAlbumDao_.GetAlbumIdsFromCache();
}

bool PhotosRestore::IsTrashedByRecycleFlag(const FileInfo &fileInfo)
{
    const std::unordered_set<int32_t> RECYCLE_FLAG_SET = { RECYCLE_FLAG_LOCAL, RECYCLE_FLAG_UNSYNCED,
        RECYCLE_FLAG_SYNCED, RECYCLE_FLAG_DELETE_UNSYNCED, RECYCLE_FLAG_HARD_DELETE_UNSYNCED };
    return RECYCLE_FLAG_SET.count(fileInfo.recycleFlag) > 0;
}
}  // namespace OHOS::Media
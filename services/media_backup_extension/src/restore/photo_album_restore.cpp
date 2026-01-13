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
#define MLOG_TAG "Media_Restore"

#include <vector>
#include <string>
#include <unordered_set>

#include "photo_album_restore.h"
#include "media_log.h"
#include "rdb_store.h"
#include "rdb_errno.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"
#include "backup_const.h"
#include "photo_album_dao.h"
#include "backup_database_utils.h"
#include "photo_album_upload_status_operation.h"
#include "album_plugin_config.h"
#include "settings_data_manager.h"

namespace OHOS::Media {
int32_t PhotoAlbumRestore::ReadResultSet(
    const std::shared_ptr<NativeRdb::ResultSet> &resultSet, PhotoAlbumRestore::GalleryAlbumRowData &albumInfo)
{
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "Query resultSql is null.");
    albumInfo.albumId = GetStringVal(this->GALLERY_ALBUM_ID, resultSet);
    albumInfo.albumName = GetStringVal(this->GALLERY_ALBUM_NAME, resultSet);
    albumInfo.relativeBucketId = GetStringVal(this->GALLERY_ALBUM_BUCKET_ID, resultSet);
    albumInfo.lPath = GetStringVal(this->GALLERY_ALBUM_lPATH, resultSet);
    albumInfo.bundleName = GetStringVal(this->GALLERY_BUNDLE_NAME, resultSet);
    albumInfo.priority = GetInt32Val(this->GALLERY_PRIORITY, resultSet);
    int32_t uploadStatus = GetInt32Val(this->GALLERY_UPLOAD_STATUS, resultSet);
    int32_t hdcUploadStatus = GetInt32Val(this->GALLERY_HDC_UPLOAD_STATUS, resultSet);
    auto switchStatus = SettingsDataManager::GetPhotosSyncSwitchStatus();
    if (switchStatus == SwitchStatus::HDC) {
        MEDIA_INFO_LOG("get photoSyncSwitchStatus: HDC");
        uploadStatus = hdcUploadStatus;
    }
    // case 1. Camera fixed to 1, avoid restore to 0
    if (StringUtils::ToLower(albumInfo.lPath) == StringUtils::ToLower(AlbumPlugin::LPATH_CAMERA) && uploadStatus != 1) {
        MEDIA_WARN_LOG("Album of Camera need to update to 1, original uploadStatus is ERROR.");
        uploadStatus = 1;
    }
    albumInfo.uploadStatus = uploadStatus;
    return E_OK;
}

/**
 * @brief get the data of gallery_album table, to restore the PhotoAlbum table
 */
std::vector<PhotoAlbumRestore::GalleryAlbumRowData> PhotoAlbumRestore::GetGalleryAlbums()
{
    std::vector<PhotoAlbumRestore::GalleryAlbumRowData> result;
    std::optional<PhotoAlbumRestore::GalleryAlbumRowData> screenShotAlbumInfoOp;
    std::string querySql = this->SQL_GALLERY_ALBUM_SELECT;
    int rowCount = 0;
    int offset = 0;
    int pageSize = 200;
    do {
        std::vector<NativeRdb::ValueObject> params = {offset, pageSize};
        CHECK_AND_BREAK_ERR_LOG(this->galleryRdb_ != nullptr, "Media_Restore: galleryRdb_ is null.");
        auto resultSet = this->galleryRdb_->QuerySql(querySql, params);
        CHECK_AND_BREAK_ERR_LOG(resultSet != nullptr, "Query resultSql is null.");
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            PhotoAlbumRestore::GalleryAlbumRowData albumInfo;
            CHECK_AND_BREAK_ERR_LOG(this->ReadResultSet(resultSet, albumInfo) == E_OK, "ReadResultSet failed.");
            result.emplace_back(albumInfo);
            // cache the albumInfo of ScreenShots
            if (StringUtils::ToLower(albumInfo.lPath) == StringUtils::ToLower(AlbumPlugin::LPATH_SCREEN_SHOTS)) {
                screenShotAlbumInfoOp = albumInfo;
            }
        }
        // Check if there are more rows to fetch.
        resultSet->GetRowCount(rowCount);
        offset += pageSize;
        resultSet->Close();
    } while (rowCount > 0);
    if (screenShotAlbumInfoOp.has_value()) {
        int32_t uploadStatus = screenShotAlbumInfoOp.value().uploadStatus;
        MEDIA_INFO_LOG(
            "ScreenShotAlbumInfo has value, auto-create ScreenRecorderAlbumInfo with uploadStatus: %{public}d",
            uploadStatus);
        result.emplace_back(this->BuildAlbumInfoOfGalleryRecorders(uploadStatus));
    }
    return result;
}

/**
 * @brief Find the albums that need to be restored.
 */
std::vector<PhotoAlbumDao::PhotoAlbumRowData> PhotoAlbumRestore::GetAlbumsToRestore(
    const std::vector<PhotoAlbumDao::PhotoAlbumRowData> &photoAlbums,
    const std::vector<PhotoAlbumRestore::GalleryAlbumRowData> &galleryAlbums)
{
    // cache the lPath of the albums in the gallery_media table
    std::unordered_set<std::string> uniquePaths;
    for (const auto &album : photoAlbums) {
        uniquePaths.insert(album.lPath);
    }
    // filter gallery_media by lPath, find the albums that need to be restored
    // Note 1, the album with the same lPath is considered to be the same album.
    // Note 2, the lPath is case-sensitively matched here,
    // for case-insensitive matching is performed in the SQL excution at the end.
    std::vector<PhotoAlbumRestore::GalleryAlbumRowData> filteredAlbums;
    std::copy_if(galleryAlbums.begin(),
        galleryAlbums.end(),
        std::back_inserter(filteredAlbums),
        [&uniquePaths](const PhotoAlbumRestore::GalleryAlbumRowData &album) {
            return uniquePaths.find(album.lPath) == uniquePaths.end();
        });
    // build the result
    std::vector<PhotoAlbumDao::PhotoAlbumRowData> result;
    for (const PhotoAlbumRestore::GalleryAlbumRowData &galleryAlbum : filteredAlbums) {
        PhotoAlbumDao::PhotoAlbumRowData albumRowData;
        DetermineAlbumTypeByLPath(albumRowData, galleryAlbum.lPath);
        albumRowData.albumName = galleryAlbum.albumName;
        albumRowData.bundleName = galleryAlbum.bundleName;
        albumRowData.lPath = galleryAlbum.lPath;
        albumRowData.priority = galleryAlbum.priority;
        albumRowData.uploadStatus = isAccountValidAndSwitchOn_ ? galleryAlbum.uploadStatus :
            PhotoAlbumUploadStatusOperation::GetAlbumUploadStatusWithLpath(albumRowData.lPath);
        result.emplace_back(albumRowData);
    }
    return result;
}

int32_t PhotoAlbumRestore::QueryMaxAlbumId(const std::string &tableName, const std::string &idName)
{
    int32_t maxAlbumId = -1;
    const std::string QUERY_SQL = "SELECT MAX(" + idName + ") " + idName + " FROM " + tableName;
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaLibraryRdb_, QUERY_SQL);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, -1, "query resultSql is null.");
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        auto albumId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, idName);
        CHECK_AND_EXECUTE(!albumId.has_value(), maxAlbumId = albumId.value());
    }
    resultSet->Close();
    return maxAlbumId;
}

int32_t PhotoAlbumRestore::GetMaxAlbumId()
{
    return this->maxAlbumId_;
}

std::vector<PhotoAlbumDao::PhotoAlbumRowData> PhotoAlbumRestore::GetAlbumInfoToUpdate(
    const std::vector<PhotoAlbumDao::PhotoAlbumRowData> &photoAlbums,
    const std::vector<GalleryAlbumRowData> &galleryAlbums)
{
    // cache the lPath of the albums in the gallery_media table
    std::unordered_set<std::string> uniquePaths;
    for (const auto &album : photoAlbums) {
        uniquePaths.insert(album.lPath);
    }
    // filter gallery_media by lPath, find the albums that need to be update.
    // Note 1, the album with the same lPath is considered to be the same album.
    // Note 2, the lPath is case-sensitively matched here,
    // for case-insensitive matching is performed in the SQL excution at the end.
    std::vector<PhotoAlbumRestore::GalleryAlbumRowData> filteredAlbums;
    std::copy_if(galleryAlbums.begin(),
        galleryAlbums.end(),
        std::back_inserter(filteredAlbums),
        [&uniquePaths](const PhotoAlbumRestore::GalleryAlbumRowData &album) {
            return uniquePaths.find(album.lPath) != uniquePaths.end();
        });
    // build the result
    std::vector<PhotoAlbumDao::PhotoAlbumRowData> result;
    for (const PhotoAlbumRestore::GalleryAlbumRowData &galleryAlbum : filteredAlbums) {
        PhotoAlbumDao::PhotoAlbumRowData albumRowData;
        albumRowData.albumType = static_cast<int32_t>(PhotoAlbumType::SOURCE);
        albumRowData.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC);
        albumRowData.albumName = galleryAlbum.albumName;
        albumRowData.bundleName = galleryAlbum.bundleName;
        albumRowData.lPath = galleryAlbum.lPath;
        albumRowData.priority = galleryAlbum.priority;
        albumRowData.uploadStatus = galleryAlbum.uploadStatus;
        result.emplace_back(albumRowData);
    }
    return result;
}

int32_t PhotoAlbumRestore::UpdateAlbums(const std::vector<PhotoAlbumDao::PhotoAlbumRowData> &albumInfos)
{
    // update the upload status of the album
    CHECK_AND_RETURN_RET_INFO_LOG(this->isAccountValidAndSwitchOn_,
        NativeRdb::E_OK,
        "UpdateAlbums skipped, account is invalid or switch is off.");
    for (auto &albumInfo : albumInfos) {
        this->photoAlbumDao_.UpdateUploadStatus(albumInfo.lPath, albumInfo.uploadStatus);
    }
    return NativeRdb::E_OK;
}

/**
 * @brief Build GalleryAlbumRowData for ScreenRecorder.
 */
PhotoAlbumRestore::GalleryAlbumRowData PhotoAlbumRestore::BuildAlbumInfoOfGalleryRecorders(const int32_t uploadStatus)
{
    PhotoAlbumRestore::GalleryAlbumRowData albumInfo;
    // bind albumName and bundleName by lPath.
    albumInfo.albumName = AlbumPlugin::ALBUM_NAME_SCREEN_RECORDS;
    albumInfo.bundleName = AlbumPlugin::BUNDLE_NAME_SCREEN_RECORDS;
    albumInfo.lPath = AlbumPlugin::LPATH_SCREEN_RECORDS;
    albumInfo.priority = 1;
    albumInfo.uploadStatus = uploadStatus;
    return albumInfo;
}

void PhotoAlbumRestore::DetermineAlbumTypeByLPath(
    PhotoAlbumDao::PhotoAlbumRowData &albumRowData, const std::string &lPath)
{
    albumRowData.albumType = static_cast<int32_t>(PhotoAlbumType::SOURCE);
    albumRowData.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC);

    std::string target = "/Pictures/Users/";
    std::transform(target.begin(), target.end(), target.begin(), ::tolower);
    std::string lPathLower = lPath;
    std::transform(lPathLower.begin(), lPathLower.end(), lPathLower.begin(), ::tolower);

    CHECK_AND_RETURN_RET_LOG(
        lPathLower.find(target) == 0, , "lPath does not start with target path: %{public}s", lPath.c_str());
    albumRowData.albumType = static_cast<int32_t>(PhotoAlbumType::USER);
    albumRowData.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC);
}
}  // namespace OHOS::Media
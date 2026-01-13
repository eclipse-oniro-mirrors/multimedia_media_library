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
#include "photos_clone.h"

#include <uuid.h>
#include <numeric>

#include "rdb_store.h"
#include "result_set_utils.h"
#include "photo_album_dao.h"
#include "backup_const.h"
#include "media_log.h"
#include "album_plugin_config.h"
#include "userfile_manager_types.h"

namespace OHOS::Media {
std::string PhotosClone::ToString(const FileInfo &fileInfo)
{
    return "FileInfo[ fileId: " + std::to_string(fileInfo.fileIdOld) + ", displayName: " + fileInfo.displayName +
           ", bundleName: " + fileInfo.bundleName + ", lPath: " + fileInfo.lPath +
           ", size: " + std::to_string(fileInfo.fileSize) + ", fileType: " + std::to_string(fileInfo.fileType) + " ]";
}

std::string PhotosClone::ToLower(const std::string &str)
{
    std::string lowerStr;
    std::transform(
        str.begin(), str.end(), std::back_inserter(lowerStr), [](unsigned char c) { return std::tolower(c); });
    return lowerStr;
}

/**
 * @brief Get Row Count of Photos in Album.
 */
int32_t PhotosClone::GetPhotosRowCountInPhotoMap()
{
    std::string querySql = this->SQL_PHOTOS_TABLE_COUNT_IN_PHOTO_MAP;
    CHECK_AND_RETURN_RET_LOG(this->mediaLibraryOriginalRdb_ != nullptr, 0,
        "Media_Restore: mediaLibraryOriginalRdb_ is null.");
    auto resultSet = this->mediaLibraryOriginalRdb_->QuerySql(querySql);
    CHECK_AND_RETURN_RET(resultSet != nullptr, 0);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return 0;
    }
    int32_t count = GetInt32Val("count", resultSet);
    return count;
}

/**
 * @brief Get Row Count of Cloud Photos in Album.
 */
int32_t PhotosClone::GetCloudPhotosRowCountInPhotoMap()
{
    std::string querySql = this->SQL_CLOUD_PHOTOS_TABLE_COUNT_IN_PHOTO_MAP;
    CHECK_AND_RETURN_RET_LOG(this->mediaLibraryOriginalRdb_ != nullptr, 0,
        "singleCloud Media_Restore: mediaLibraryOriginalRdb_ is null.");
    auto resultSet = this->mediaLibraryOriginalRdb_->QuerySql(querySql);
    CHECK_AND_RETURN_RET(resultSet != nullptr, 0);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return 0;
    }
    int32_t count = GetInt32Val("count", resultSet);
    return count;
}

/**
 * @brief Get Row Count of Photos not in Album.
 */
int32_t PhotosClone::GetPhotosRowCountNotInPhotoMap()
{
    std::string querySql = this->SQL_PHOTOS_TABLE_COUNT_NOT_IN_PHOTO_MAP;
    CHECK_AND_RETURN_RET_LOG(this->mediaLibraryOriginalRdb_ != nullptr, 0,
        "Media_Restore: mediaLibraryOriginalRdb_ is null.");
    auto resultSet = this->mediaLibraryOriginalRdb_->QuerySql(querySql);
    CHECK_AND_RETURN_RET(resultSet != nullptr, 0);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return 0;
    }
    int32_t count = GetInt32Val("count", resultSet);
    return count;
}

/**
 * @brief Get Row Count of Cloud Photos not in Album.
 */
int32_t PhotosClone::GetCloudPhotosRowCountNotInPhotoMap()
{
    std::string querySql = this->SQL_CLOUD_PHOTOS_TABLE_COUNT_NOT_IN_PHOTO_MAP;
    CHECK_AND_RETURN_RET_LOG(this->mediaLibraryOriginalRdb_ != nullptr, 0,
        "singleCloud Media_Restore: mediaLibraryOriginalRdb_ is null.");
    auto resultSet = this->mediaLibraryOriginalRdb_->QuerySql(querySql);
    CHECK_AND_RETURN_RET(resultSet != nullptr, 0);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return 0;
    }
    int32_t count = GetInt32Val("count", resultSet);
    return count;
}

/**
 * @brief Query the Photos Info, which is in PhotoAlbum, from the Original MediaLibrary Database.
 */
std::shared_ptr<NativeRdb::ResultSet> PhotosClone::GetPhotosInPhotoMap(int32_t offset, int32_t pageSize)
{
    std::vector<NativeRdb::ValueObject> bindArgs = {offset, pageSize};
    CHECK_AND_RETURN_RET_LOG(this->mediaLibraryOriginalRdb_ != nullptr, nullptr,
        "Media_Restore: mediaLibraryOriginalRdb_ is null.");
    return this->mediaLibraryOriginalRdb_->QuerySql(this->SQL_PHOTOS_TABLE_QUERY_IN_PHOTO_MAP, bindArgs);
}

/**
 * @brief Query the Cloud Photos Info, which is in PhotoAlbum, from the Original MediaLibrary Database.
 */
std::shared_ptr<NativeRdb::ResultSet> PhotosClone::GetCloudPhotosInPhotoMap(int32_t offset, int32_t pageSize)
{
    std::vector<NativeRdb::ValueObject> bindArgs = {offset, pageSize};
    CHECK_AND_RETURN_RET_LOG(this->mediaLibraryOriginalRdb_ != nullptr, nullptr,
        "singleCloud Media_Restore: mediaLibraryOriginalRdb_ is null.");
    return this->mediaLibraryOriginalRdb_->QuerySql(this->SQL_CLOUD_PHOTOS_TABLE_QUERY_IN_PHOTO_MAP, bindArgs);
}

/**
 * @brief Query the Photos Info, which is not in PhotoAlbum, from the Original MediaLibrary Database.
 */
std::shared_ptr<NativeRdb::ResultSet> PhotosClone::GetPhotosNotInPhotoMap(int32_t offset, int32_t pageSize)
{
    std::vector<NativeRdb::ValueObject> bindArgs = {offset, pageSize};
    CHECK_AND_RETURN_RET_LOG(this->mediaLibraryOriginalRdb_ != nullptr, nullptr,
        "Media_Restore: mediaLibraryOriginalRdb_ is null.");
    return this->mediaLibraryOriginalRdb_->QuerySql(this->SQL_PHOTOS_TABLE_QUERY_NOT_IN_PHOTO_MAP, bindArgs);
}

/**
 * @brief Query the Cloud Photos Info, which is not in PhotoAlbum, from the Original MediaLibrary Database.
 */
std::shared_ptr<NativeRdb::ResultSet> PhotosClone::GetCloudPhotosNotInPhotoMap(int32_t offset, int32_t pageSize)
{
    std::vector<NativeRdb::ValueObject> bindArgs = {offset, pageSize};
    CHECK_AND_RETURN_RET_LOG(this->mediaLibraryOriginalRdb_ != nullptr, nullptr,
        "singleCloud Media_Restore: mediaLibraryOriginalRdb_ is null.");
    return this->mediaLibraryOriginalRdb_->QuerySql(this->SQL_CLOUD_PHOTOS_TABLE_QUERY_NOT_IN_PHOTO_MAP, bindArgs);
}

/**
 * @note If the lPath is empty, return '/Pictures/其它' string.
 *      If the lPath is '/Pictures/ScreenShots', return '/Pictures/ScreenShots' string.
 *      Otherwise, return the lPath of the FileInfo.
 */
PhotoAlbumDao::PhotoAlbumRowData PhotosClone::FindAlbumInfo(const FileInfo &fileInfo)
{
    PhotoAlbumDao::PhotoAlbumRowData albumInfo;
    std::string lPath = fileInfo.lPath;
    // Scenario 2, WHEN FileInfo is in hidden album, THEN override lPath to the folder in sourcePath.
    // Scenario 3, WHEN FileInfo is not belongs to any album, THEN override lPath to the folder in sourcePath.
    // Note, sourcePath is a sign of the possible scenaio that the file is not in any album.
    bool islPathMiss = !fileInfo.sourcePath.empty() && (fileInfo.hidden == 1 || fileInfo.recycledTime != 0);
    islPathMiss = islPathMiss || fileInfo.lPath.empty();
    if (islPathMiss) {
        lPath = this->photoAlbumDao_.ParseSourcePathToLPath(fileInfo.sourcePath);
        MEDIA_INFO_LOG("Media_Restore: fix lPath of album.fileInfo.lPath: %{public}s, "
                       "lPathFromSourcePath: %{public}s, lowercase: %{public}s, FileInfo Object: %{public}s",
            fileInfo.lPath.c_str(),
            lPath.c_str(),
            this->ToLower(lPath).c_str(),
            this->ToString(fileInfo).c_str());
    }
    // Scenario 1, WHEN FileInfo is in /Pictures/Screenshots and Video type, THEN redirect to /Pictures/Screenrecords
    if (this->ToLower(lPath) == this->ToLower(AlbumPlugin::LPATH_SCREEN_SHOTS) &&
        fileInfo.fileType == MediaType::MEDIA_TYPE_VIDEO) {
        albumInfo = this->photoAlbumDao_.BuildAlbumInfoOfRecorders();
        albumInfo = this->photoAlbumDao_.GetOrCreatePhotoAlbum(albumInfo);
        MEDIA_INFO_LOG("Media_Restore: screenshots redirect to screenrecords, fileInfo.lPath: %{public}s, "
                       "lPath: %{public}s, Object: %{public}s, albumInfo: %{public}s",
            fileInfo.lPath.c_str(),
            lPath.c_str(),
            this->ToString(fileInfo).c_str(),
            this->photoAlbumDao_.ToString(albumInfo).c_str());
        return albumInfo;
    }
    return BuildAlbumInfoByCondition(fileInfo, lPath);
}

PhotoAlbumDao::PhotoAlbumRowData PhotosClone::BuildAlbumInfoByCondition(const FileInfo &fileInfo,
    const std::string &lPath)
{
    PhotoAlbumDao::PhotoAlbumRowData albumInfo = this->photoAlbumDao_.BuildAlbumInfoByLPath(lPath);
    if (fileInfo.hidden == 0 && fileInfo.recycledTime == 0) {
        return this->photoAlbumDao_.GetOrCreatePhotoAlbum(albumInfo);
    }
    return this->photoAlbumDao_.GetOrCreatePhotoAlbumForClone(albumInfo);
}

/**
 * @brief Find the lPath of the PhotoAlbum related to Photos from target database.
 */
std::string PhotosClone::FindlPath(const FileInfo &fileInfo)
{
    PhotoAlbumDao::PhotoAlbumRowData albumInfo = this->FindAlbumInfo(fileInfo);
    return albumInfo.lPath;
}

/**
 * @brief Find the albumId of the PhotoAlbum related to Photos from target database.
 */
int32_t PhotosClone::FindAlbumId(const FileInfo &fileInfo)
{
    PhotoAlbumDao::PhotoAlbumRowData albumInfo = this->FindAlbumInfo(fileInfo);
    bool cond = albumInfo.albumId > static_cast<int32_t>(PhotoAlbumId::DEFAULT) ||
        (fileInfo.hidden == 0 && fileInfo.recycledTime == 0);
    CHECK_AND_RETURN_RET(!cond, albumInfo.albumId);
    return fileInfo.recycledTime != 0 ? static_cast<int32_t>(PhotoAlbumId::TRASH) :
        static_cast<int32_t>(PhotoAlbumId::HIDDEN);
}

/**
 * @brief Find the packageName of the PhotoAlbum related to Photos from target database.
 */
std::string PhotosClone::FindPackageName(const FileInfo &fileInfo)
{
    PhotoAlbumDao::PhotoAlbumRowData albumInfo = this->FindAlbumInfo(fileInfo);
    // Only provide the package name of the existing SOURCE album.
    CHECK_AND_RETURN_RET(albumInfo.IsValidSourceAlbum(), "");
    return albumInfo.albumName;
}

/**
 * @brief Find the bundleName of the PhotoAlbum related to Photos from target database.
 */
std::string PhotosClone::FindBundleName(const FileInfo &fileInfo)
{
    PhotoAlbumDao::PhotoAlbumRowData albumInfo = this->FindAlbumInfo(fileInfo);
    // Only provide the bundle name of the existing SOURCE album.
    CHECK_AND_RETURN_RET(albumInfo.IsValidSourceAlbum(), "");
    return albumInfo.bundleName;
}

std::vector<PhotosDao::PhotosRowData> PhotosClone::FindDuplicateBurstKey()
{
    std::vector<PhotosDao::PhotosRowData> result;
    std::string querySql = this->SQL_PHOTOS_TABLE_BURST_KEY_DUPLICATE_QUERY;
    int rowCount = 0;
    int offset = 0;
    int pageSize = 200;
    do {
        std::vector<NativeRdb::ValueObject> bindArgs = {offset, pageSize};
        CHECK_AND_BREAK_ERR_LOG(this->mediaLibraryOriginalRdb_ != nullptr,
            "Media_Restore: mediaLibraryOriginalRdb_ is null.");
        auto resultSet = this->mediaLibraryTargetRdb_->QuerySql(querySql, bindArgs);
        CHECK_AND_BREAK_ERR_LOG(resultSet != nullptr, "Query resultSql is null.");
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            PhotosDao::PhotosRowData info;
            info.burstKey = GetStringVal("burst_key", resultSet);
            info.ownerAlbumId = GetInt32Val("owner_album_id", resultSet);
            result.emplace_back(info);
        }
        // Check if there are more rows to fetch.
        resultSet->GetRowCount(rowCount);
        offset += pageSize;
        resultSet->Close();
    } while (rowCount > 0);
    return result;
}

int32_t PhotosClone::FindPhotoQuality(const FileInfo &fileInfo)
{
    CHECK_AND_RETURN_RET(fileInfo.photoQuality != 1, 0);
    return fileInfo.photoQuality;
}

std::string PhotosClone::ToString(const std::vector<NativeRdb::ValueObject> &values)
{
    std::vector<std::string> result;
    for (auto &value : values) {
        std::string str;
        value.GetString(str);
        result.emplace_back(str + ", ");
    }
    return std::accumulate(result.begin(), result.end(), std::string());
}

/**
 * @brief generate a uuid
 *
 * @return std::string uuid with 32 characters
 */
std::string PhotosClone::GenerateUuid()
{
    uuid_t uuid;
    uuid_generate(uuid);
    char str[UUID_STR_LENGTH] = {};
    uuid_unparse(uuid, str);
    return str;
}

/**
 * @brief Fix Duplicate burst_key in Photos, which is used in different PhotoAlbum.
 */
int32_t PhotosClone::FixDuplicateBurstKeyInDifferentAlbum(std::atomic<uint64_t> &totalNumber)
{
    std::vector<PhotosDao::PhotosRowData> duplicateBurstKeyList = this->FindDuplicateBurstKey();
    totalNumber += static_cast<uint64_t>(duplicateBurstKeyList.size());
    MEDIA_INFO_LOG("Media_Restore: onProcess Update otherTotalNumber_: %{public}lld", (long long)totalNumber);
    std::string executeSql = this->SQL_PHOTOS_TABLE_BURST_KEY_UPDATE;
    for (auto &info : duplicateBurstKeyList) {
        CHECK_AND_CONTINUE(!info.burstKey.empty());
        std::string burstKeyNew = this->GenerateUuid();
        std::vector<NativeRdb::ValueObject> bindArgs = {burstKeyNew, info.ownerAlbumId, info.burstKey};
        MEDIA_INFO_LOG("Media_Restore: executeSql = %{public}s, bindArgs=%{public}s",
            executeSql.c_str(),
            this->ToString(bindArgs).c_str());
        CHECK_AND_BREAK_ERR_LOG(this->mediaLibraryOriginalRdb_ != nullptr,
            "Media_Restore: mediaLibraryOriginalRdb_ is null.");
        int32_t ret = this->mediaLibraryTargetRdb_->ExecuteSql(executeSql, bindArgs);
        CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK,
            "Media_Restore: FixDuplicateBurstKeyInDifferentAlbum failed,"
            " ret=%{public}d, sql=%{public}s, bindArgs=%{public}s",
            ret, executeSql.c_str(), this->ToString(bindArgs).c_str());
    }
    return 0;
}

std::string PhotosClone::FindSourcePath(const FileInfo &fileInfo)
{
    CHECK_AND_RETURN_RET(!fileInfo.lPath.empty(), fileInfo.sourcePath);
    CHECK_AND_RETURN_RET(fileInfo.sourcePath.empty(), fileInfo.sourcePath);
    bool cond = (fileInfo.hidden == 0 && fileInfo.recycledTime == 0);
    CHECK_AND_RETURN_RET(!cond, fileInfo.sourcePath);
    return this->SOURCE_PATH_PREFIX + fileInfo.lPath + "/" + fileInfo.displayName;
}

/**
 * @brief Get Row Count of Photos No Need Migrate.
 */
int32_t PhotosClone::GetNoNeedMigrateCount()
{
    std::string querySql = this->SQL_PHOTOS_TABLE_COUNT_NO_NEED_MIGRATE;
    CHECK_AND_RETURN_RET_LOG(this->mediaLibraryOriginalRdb_ != nullptr, 0,
        "Media_Restore: mediaLibraryOriginalRdb_ is null.");
    auto resultSet = this->mediaLibraryOriginalRdb_->QuerySql(querySql);
    CHECK_AND_RETURN_RET(resultSet != nullptr, 0);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return 0;
    }
    int32_t count = GetInt32Val("count", resultSet);
    return count;
}
}  // namespace OHOS::Media
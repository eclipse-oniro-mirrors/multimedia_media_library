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
#include "photo_album_dao.h"

#include <regex>

#include "album_plugin_config.h"
#include "backup_database_utils.h"
#include "backup_file_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdb_transaction.h"
#include "media_log.h"
#include "rdb_store.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"

namespace OHOS::Media {
PhotoAlbumDao::PhotoAlbumRowData::PhotoAlbumRowData()
{
    albumId = static_cast<int32_t>(PhotoAlbumId::DEFAULT);
    albumType = -1;
    albumSubType = -1;
    priority = 1;
}

bool PhotoAlbumDao::PhotoAlbumRowData::IsValidSourceAlbum()
{
    return albumType == static_cast<int32_t>(PhotoAlbumType::SOURCE) &&
        albumSubType == static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC) &&
        albumId > static_cast<int32_t>(PhotoAlbumId::DEFAULT);
}

std::string StringUtils::ToLower(const std::string &str)
{
    std::string lowerStr;
    std::transform(
        str.begin(), str.end(), std::back_inserter(lowerStr), [](unsigned char c) { return std::tolower(c); });
    return lowerStr;
}

/**
 * @brief Check the AlbumName unique or not. true - unique, false - not unique.
 */
bool PhotoAlbumDao::CheckAlbumNameUnique(const std::string &albumName, const std::string &lPath)
{
    std::vector<NativeRdb::ValueObject> bindArgs = {albumName, lPath};
    std::string querySql = this->SQL_PHOTO_ALBUM_CHECK_ALBUM_NAME_UNIQUE;
    CHECK_AND_RETURN_RET_LOG(this->mediaLibraryRdb_ != nullptr, true,
        "Media_Restore: mediaLibraryRdb_ is null.");
    auto resultSet = this->mediaLibraryRdb_->QuerySql(querySql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, true, "Media_Restore: resultSet is nullptr");
    if (resultSet->GoToNextRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return true;
    }
    int32_t count = GetInt32Val("count", resultSet);
    resultSet->Close();
    return count == 0;
}

/**
 * @brief Find the Unique Album Name.
 */
std::string PhotoAlbumDao::FindUniqueAlbumName(const PhotoAlbumDao::PhotoAlbumRowData &photoAlbum)
{
    bool cond = (photoAlbum.lPath.empty() || photoAlbum.albumName.empty());
    CHECK_AND_RETURN_RET_LOG(!cond, "", "Media_Restore: Invalid album data");
    const std::string lPath = photoAlbum.lPath;
    // The PhotoAlbum is cached.
    PhotoAlbumDao::PhotoAlbumRowData albumDataInCache;
    CHECK_AND_RETURN_RET(!this->photoAlbumCache_.Find(StringUtils::ToLower(lPath), albumDataInCache),
        albumDataInCache.albumName);
    // Check if the album name is unique.
    std::string albumName = photoAlbum.albumName;
    int32_t sequence = 1;
    bool isUnique = this->CheckAlbumNameUnique(albumName, photoAlbum.lPath);
    while (!isUnique && sequence < this->MAX_ALBUM_NAME_SEQUENCE) {
        albumName = photoAlbum.albumName + " " + std::to_string(sequence);
        sequence++;
        isUnique = this->CheckAlbumNameUnique(albumName, photoAlbum.lPath);
    }
    MEDIA_INFO_LOG("Media_Restore: FindUniqueAlbumName, old name: %{public}s, new name: %{public}s, lPath: %{public}s",
        photoAlbum.albumName.c_str(),
        albumName.c_str(),
        photoAlbum.lPath.c_str());
    return albumName;
}

/**
 * @brief get the data of PhotoAlbum table, to find the differenct between the PhotoAlbum and the gallery_album
 */
std::vector<PhotoAlbumDao::PhotoAlbumRowData> PhotoAlbumDao::GetPhotoAlbums()
{
    std::vector<PhotoAlbumDao::PhotoAlbumRowData> result;
    std::string querySql = this->SQL_PHOTO_ALBUM_SELECT;
    int rowCount = 0;
    int offset = 0;
    int pageSize = 200;
    do {
        std::vector<NativeRdb::ValueObject> bindArgs = {offset, pageSize};
        CHECK_AND_BREAK_ERR_LOG(this->mediaLibraryRdb_ != nullptr, "Media_Restore: mediaLibraryRdb_ is null.");
        auto resultSet = this->mediaLibraryRdb_->QuerySql(querySql, bindArgs);
        CHECK_AND_BREAK_ERR_LOG(resultSet != nullptr, "Media_Restore: Query resultSql is null.");
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            PhotoAlbumDao::PhotoAlbumRowData albumRowData;
            albumRowData.albumId = GetInt32Val(this->FIELD_NAME_ALBUM_ID, resultSet);
            albumRowData.albumName = GetStringVal(this->FIELD_NAME_ALBUM_NAME, resultSet);
            albumRowData.bundleName = GetStringVal(this->FIELD_NAME_BUNDLE_NAME, resultSet);
            albumRowData.albumType = GetInt32Val(this->FIELD_NAME_ALBUM_TYPE, resultSet);
            albumRowData.albumSubType = GetInt32Val(this->FIELD_NAME_ALBUM_SUBTYPE, resultSet);
            albumRowData.lPath = GetStringVal(this->FIELD_NAME_LPATH, resultSet);
            albumRowData.priority = GetInt32Val(this->FIELD_NAME_PRIORITY, resultSet);
            albumRowData.uploadStatus = GetInt32Val(this->FIELD_UPLOAD_STATUS, resultSet);
            result.emplace_back(albumRowData);
        }
        // Check if there are more rows to fetch.
        resultSet->GetRowCount(rowCount);
        resultSet->Close();
        offset += pageSize;
    } while (rowCount > 0);
    return result;
}

/**
 * @brief Get and cache PhotoAlbum info by lPath from PhotoAlbum table.
 */
PhotoAlbumDao::PhotoAlbumRowData PhotoAlbumDao::GetPhotoAlbum(const std::string &lPath)
{
    std::unique_lock<std::mutex> lock(this->cacheLock_);
    // find the PhotoAlbum info by lPath in cache
    PhotoAlbumDao::PhotoAlbumRowData albumRowData;
    CHECK_AND_RETURN_RET(!this->photoAlbumCache_.Find(StringUtils::ToLower(lPath), albumRowData), albumRowData);
    MEDIA_INFO_LOG("Media_Restore: can not find the PhotoAlbum info by lPath in cache."
                   " lPath=%{public}s, lPath in cache=%{public}s",
        lPath.c_str(),
        StringUtils::ToLower(lPath).c_str());
    // query the PhotoAlbum info by lPath from PhotoAlbum table
    std::vector<NativeRdb::ValueObject> bindArgs = {lPath};
    std::string querySql = this->SQL_PHOTO_ALBUM_SELECT_BY_LPATH;
    CHECK_AND_RETURN_RET_LOG(this->mediaLibraryRdb_ != nullptr, albumRowData,
        "Media_Restore: mediaLibraryRdb_ is null.");
    auto resultSet = this->mediaLibraryRdb_->QuerySql(querySql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, albumRowData, "Media_Restore: can not find the PhotoAlbum info by"
        " lPath [%{public}s] in PhotoAlbum table.", lPath.c_str());
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Media_Restore: can not find the PhotoAlbum info by"
            " lPath [%{public}s] in PhotoAlbum table.", lPath.c_str());
        return albumRowData;
    }

    albumRowData.albumId = GetInt32Val(this->FIELD_NAME_ALBUM_ID, resultSet);
    albumRowData.albumName = GetStringVal(this->FIELD_NAME_ALBUM_NAME, resultSet);
    albumRowData.bundleName = GetStringVal(this->FIELD_NAME_BUNDLE_NAME, resultSet);
    albumRowData.albumType = GetInt32Val(this->FIELD_NAME_ALBUM_TYPE, resultSet);
    albumRowData.albumSubType = GetInt32Val(this->FIELD_NAME_ALBUM_SUBTYPE, resultSet);
    albumRowData.lPath = GetStringVal(this->FIELD_NAME_LPATH, resultSet);
    albumRowData.priority = GetInt32Val(this->FIELD_NAME_PRIORITY, resultSet);
    albumRowData.uploadStatus = GetInt32Val(this->FIELD_UPLOAD_STATUS, resultSet);
    resultSet->Close();
    // cache the PhotoAlbum info by lPath
    this->photoAlbumCache_.Insert(StringUtils::ToLower(lPath), albumRowData);
    MEDIA_INFO_LOG("Media_Restore: add the PhotoAlbum info by lPath into cache."
                   " lPath=%{public}s, lPath in cache=%{public}s",
        lPath.c_str(),
        StringUtils::ToLower(albumRowData.lPath).c_str());
    return albumRowData;
}

bool PhotoAlbumDao::HasSameAlbumName(std::string albumName)
{
    std::unique_lock<std::mutex> lock(this->cacheLock_);
    CHECK_AND_RETURN_RET(std::count(this->albumNameCache_.begin(), this->albumNameCache_.end(),
        StringUtils::ToLower(albumName)) == 0, true);
    std::string sql = "SELECT album_name FROM PhotoAlbum WHERE LOWER(album_name) == LOWER(?) AND dirty != 4";
    CHECK_AND_RETURN_RET_LOG(this->mediaLibraryRdb_ != nullptr, false,
        "Media_Restore: mediaLibraryRdb_ is null.");
    std::vector<NativeRdb::ValueObject> bindArgs = {albumName};
    auto resultSet = this->mediaLibraryRdb_->QuerySql(sql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "Media_Restore: can not find the PhotoAlbum info");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        string albumNameVal = GetStringVal(this->FIELD_NAME_ALBUM_NAME, resultSet);
        this->albumNameCache_.push_back(StringUtils::ToLower(albumNameVal));
    }
    resultSet->Close();
    return std::count(this->albumNameCache_.begin(), this->albumNameCache_.end(), StringUtils::ToLower(albumName)) != 0;
}

/**
 * @brief Get and cache PhotoAlbum info by lPath, if PhotoAlbum not exists, create it.
 */
PhotoAlbumDao::PhotoAlbumRowData PhotoAlbumDao::GetOrCreatePhotoAlbum(const PhotoAlbumRowData &album)
{
    // validate inputs
    CHECK_AND_RETURN_RET_LOG(!album.lPath.empty(),
        album,
        "Media_Restore: Invalid album data, lPath is empty."
        " Object: %{public}s",
        this->ToString(album).c_str());
    std::unique_lock<std::mutex> lock(this->photoAlbumCreateLock_);
    // try to get from cache
    PhotoAlbumDao::PhotoAlbumRowData albumRowData = this->GetPhotoAlbum(album.lPath);
    CHECK_AND_RETURN_RET(albumRowData.lPath.empty(), albumRowData);
    std::string uniqueAlbumName = this->FindUniqueAlbumName(album);
    std::vector<NativeRdb::ValueObject> bindArgs = {
        album.albumType,
        album.albumSubType,
        uniqueAlbumName,
        album.bundleName,
        album.lPath,
        album.priority,
        album.uploadStatus,
    };
    CHECK_AND_RETURN_RET_LOG(this->mediaLibraryRdb_ != nullptr, album, "Media_Restore: mediaLibraryRdb_ is null.");
    auto err = BackupDatabaseUtils::ExecuteSQL(this->mediaLibraryRdb_, this->SQL_PHOTO_ALBUM_INSERT, bindArgs);
    CHECK_AND_RETURN_RET_LOG(err == NativeRdb::E_OK,
        album,
        "Media_Restore: INSERT INTO PhotoAlbum failed,"
        "err = %{public}d, executeSql = %{public}s, bindArgs = %{public}s",
        err,
        this->SQL_PHOTO_ALBUM_INSERT.c_str(),
        this->ToString(bindArgs).c_str());
    MEDIA_INFO_LOG("Media_Restore: INSERT INTO PhotoAlbum success, Object: %{public}s", this->ToString(album).c_str());
    return this->GetPhotoAlbum(album.lPath);
}

/**
 * @brief Get and cache PhotoAlbum info by lPath, if not found, insert into cache instead of inserting into database
 */
PhotoAlbumDao::PhotoAlbumRowData PhotoAlbumDao::GetOrCreatePhotoAlbumForClone(const PhotoAlbumRowData &album)
{
    // validate inputs
    CHECK_AND_RETURN_RET_LOG(!album.lPath.empty(), album, "Media_Restore: Invalid album data, lPath is empty."
        " Object: %{public}s", this->ToString(album).c_str());
    std::unique_lock<std::mutex> lock(this->photoAlbumCreateLock_);
    // try to get from cache
    PhotoAlbumDao::PhotoAlbumRowData albumRowData = this->GetPhotoAlbum(album.lPath);
    CHECK_AND_RETURN_RET(albumRowData.lPath.empty(), albumRowData);
    return GetOrInsertIntoCache(album);
}

PhotoAlbumDao::PhotoAlbumRowData PhotoAlbumDao::GetOrInsertIntoCache(const PhotoAlbumRowData &album)
{
    std::unique_lock<std::mutex> lock(this->cacheLock_);
    this->photoAlbumCache_.Insert(StringUtils::ToLower(album.lPath), album);
    return album;
}

std::string PhotoAlbumDao::ToString(const std::vector<NativeRdb::ValueObject> &bindArgs)
{
    std::string args;
    for (auto &arg : bindArgs) {
        std::string tempStr;
        arg.GetString(tempStr);
        args += tempStr + ", ";
    }
    return args;
}

/**
 * @brief restore the PhotoAlbum table
 */
int32_t PhotoAlbumDao::RestoreAlbums(std::vector<PhotoAlbumDao::PhotoAlbumRowData> &photoAlbums)
{
    CHECK_AND_RETURN_RET_INFO_LOG(!photoAlbums.empty(), NativeRdb::E_OK, "Media_Restore: albumInfos are empty");
    int32_t err = NativeRdb::E_OK;
    CHECK_AND_RETURN_RET_LOG(this->mediaLibraryRdb_ != nullptr, E_FAIL, "Media_Restore: mediaLibraryRdb_ is null.");
    int32_t count = 0;
    for (const PhotoAlbumDao::PhotoAlbumRowData &data : photoAlbums) {
        bool cond = (data.lPath.empty() || data.albumName.empty());
        CHECK_AND_CONTINUE_ERR_LOG(!cond,
            "Media_Restore: restore albums failed, lPath or albumName is empty. Object: %{public}s",
            this->ToString(data).c_str());
        std::vector<NativeRdb::ValueObject> bindArgs = {
            data.albumType,
            data.albumSubType,
            data.albumName,
            data.bundleName,
            data.lPath,
            data.priority,
            data.uploadStatus,
        };
        err = BackupDatabaseUtils::ExecuteSQL(this->mediaLibraryRdb_, this->SQL_PHOTO_ALBUM_INSERT, bindArgs);
        CHECK_AND_CONTINUE_ERR_LOG(err == NativeRdb::E_OK,
            "Media_Restore: restore albums failed, "
            "err = %{public}d, executeSql = %{public}s, bindArgs = %{public}s",
            err,
            this->SQL_PHOTO_ALBUM_INSERT.c_str(),
            this->ToString(bindArgs).c_str());
        count++;
    }
    MEDIA_INFO_LOG("Media_Restore: restore albums success, total %{public}d, restored %{public}d",
        static_cast<int32_t>(photoAlbums.size()),
        count);
    return NativeRdb::E_OK;
}

/**
 * @brief Build PhotoAlbumRowData for ScreenRecorder.
 */
PhotoAlbumDao::PhotoAlbumRowData PhotoAlbumDao::BuildAlbumInfoOfRecorders()
{
    PhotoAlbumDao::PhotoAlbumRowData albumInfo;
    // bind albumName and bundleName by lPath.
    albumInfo.albumName = AlbumPlugin::ALBUM_NAME_SCREEN_RECORDS;
    albumInfo.bundleName = AlbumPlugin::BUNDLE_NAME_SCREEN_RECORDS;
    albumInfo.lPath = AlbumPlugin::LPATH_SCREEN_RECORDS;
    albumInfo.albumType = static_cast<int32_t>(PhotoAlbumType::SOURCE);
    albumInfo.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC);
    albumInfo.priority = 1;
    return albumInfo;
}

void PhotoAlbumDao::LoadPhotoAlbums()
{
    std::vector<PhotoAlbumDao::PhotoAlbumRowData> photoAlbums = this->GetPhotoAlbums();
    for (const auto &album : photoAlbums) {
        CHECK_AND_CONTINUE(!album.lPath.empty());
        this->photoAlbumCache_.Insert(StringUtils::ToLower(album.lPath), album);
    }
    MEDIA_INFO_LOG(
        "Media_Restore: LoadPhotoAlbums success, %{public}d albums", static_cast<int32_t>(photoAlbums.size()));
}

/**
 * @brief Parse the sourcePath to lPath.
 * example, sourcePath=/storage/emulated/0/DCIM/Camera/IMG_20240829_072213.jpg, lPath=/DCIM/Camera
 * if the sourcePath can not be parsed, return /Pictures/其它.
 */
std::string PhotoAlbumDao::ParseSourcePathToLPath(const std::string &sourcePath)
{
    std::string result = "/Pictures/其它";

    size_t startPos = FindRootPos(sourcePath);
    size_t endPos = sourcePath.find_last_of("/");
    CHECK_AND_RETURN_RET(startPos != std::string::npos && endPos != std::string::npos, result);
    CHECK_AND_RETURN_RET(startPos != endPos, "/");
    CHECK_AND_RETURN_RET(endPos - startPos > 1, result);
    result = sourcePath.substr(startPos, endPos - startPos);

    result = result == AlbumPlugin::LPATH_HIDDEN_ALBUM ? AlbumPlugin::LPATH_RECOVER : result;
    return result;
}

/**
 * @brief Find the end pos of root, if corner cases happen, return std::string::npos
 */
size_t PhotoAlbumDao::FindRootPos(const std::string &path)
{
    std::string rootPath;
    size_t lastSlashPos = path.find_last_of("/");
    // corner case 1: no slash
    if (lastSlashPos == std::string::npos) {
        MEDIA_ERR_LOG("No slash: %{public}s", BackupFileUtils::GarbleFilePath(path, DEFAULT_RESTORE_ID).c_str());
        return std::string::npos;
    }

    std::smatch matchResult;
    std::regex nestedRootPattern(NESTED_ROOT_PATTERN);
    std::regex nonNestedRootPattern(NON_NESTED_ROOT_PATTERN);
    if (std::regex_search(path, matchResult, nestedRootPattern)) {
        rootPath = matchResult.str(0);
        // corner case 2: matched with nested pattern, but no more slash
        if (rootPath.size() > lastSlashPos) {
            MEDIA_ERR_LOG("No more slash after matched string: %{public}s",
                BackupFileUtils::GarbleFilePath(path, DEFAULT_RESTORE_ID).c_str());
            return std::string::npos;
        }
    } else if (std::regex_search(path, matchResult, nonNestedRootPattern)) {
        rootPath = matchResult.str(0);
    }
    
    if (rootPath.empty()) {
        MEDIA_ERR_LOG("Not matched: %{public}s", BackupFileUtils::GarbleFilePath(path, DEFAULT_RESTORE_ID).c_str());
        return std::string::npos;
    }
    return rootPath.length() - 1; // truncate last slash
}

/**
 * @brief Build PhotoAlbumRowData from lPath.
 */
PhotoAlbumDao::PhotoAlbumRowData PhotoAlbumDao::BuildAlbumInfoByLPath(
    const std::string &lPath, const int32_t albumType, const int32_t albumSubType)
{
    PhotoAlbumDao::PhotoAlbumRowData albumInfo;
    // find albumName from lPath
    std::string albumName = "其它";
    std::string albumlPath = lPath;
    int32_t albumTypeTmp = albumType;
    int32_t albumSubTypeTmp = albumSubType;
    size_t fileIndex = albumlPath.find_last_of(FILE_SEPARATOR);
    if (fileIndex != string::npos) {
        albumName = albumlPath.substr(fileIndex + 1);
    } else {
        albumlPath = "/Pictures/其它";
        albumTypeTmp = static_cast<int32_t>(PhotoAlbumType::SOURCE);
        albumSubTypeTmp = static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC);
    }
    albumInfo.albumName = albumName;
    albumInfo.lPath = albumlPath;
    albumInfo.albumType = albumTypeTmp;
    albumInfo.albumSubType = albumSubTypeTmp;
    albumInfo.priority = 1;
    return albumInfo;
}

/**
 * @brief Build PhotoAlbumRowData from lPath.
 */
PhotoAlbumDao::PhotoAlbumRowData PhotoAlbumDao::BuildAlbumInfoByLPath(const std::string &lPath)
{
    int32_t albumType = static_cast<int32_t>(PhotoAlbumType::SOURCE);
    int32_t albumSubType = static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC);

    std::string target = "/Pictures/Users/";
    std::transform(target.begin(), target.end(), target.begin(), ::tolower);
    std::string lPathLower = lPath;
    std::transform(lPathLower.begin(), lPathLower.end(), lPathLower.begin(), ::tolower);
    if (lPathLower.find(target) == 0) {
        albumType = static_cast<int32_t>(PhotoAlbumType::USER);
        albumSubType = static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC);
    }
    return this->BuildAlbumInfoByLPath(lPath, albumType, albumSubType);
}

/**
 * @brief Get the Album Id Set From Photo Album Cache.
 */
std::vector<int32_t> PhotoAlbumDao::GetAlbumIdsFromCache()
{
    vector<int32_t> result;
    SafeMap<std::string, PhotoAlbumDao::PhotoAlbumRowData>::SafeMapCallBack callback =
        [&](std::string key, PhotoAlbumDao::PhotoAlbumRowData &value) {
            result.push_back(value.albumId);
        };
    photoAlbumCache_.Iterate(callback);
    return result;
}

int32_t PhotoAlbumDao::UpdateUploadStatus(const std::string &lPath, const int32_t uploadStatus)
{
    std::vector<NativeRdb::ValueObject> bindArgs = {
        uploadStatus,
        lPath,
    };
    int32_t ret =
        BackupDatabaseUtils::ExecuteSQL(this->mediaLibraryRdb_, this->SQL_PHOTO_ALBUM_UPDATE_UPLOAD_STATUS, bindArgs);
    MEDIA_INFO_LOG("UpdateUploadStatus lPath: %{public}s, uploadStatus: %{public}d, ret: %{public}d",
        lPath.c_str(),
        uploadStatus,
        ret);
    return ret;
}
}  // namespace OHOS::Media
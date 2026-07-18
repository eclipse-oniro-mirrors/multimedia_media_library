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
#include "backup_database_utils.h"
#include "result_set_utils.h"
#include "photo_album_dao.h"
#include "backup_const.h"
#include "media_log.h"
#include "album_plugin_base.h"
#include "userfile_manager_types.h"

#include <sys/stat.h>
#include "backup_adapters.h"
#include "backup_file_utils.h"
#include "upgrade_restore_task_report.h"
#include "backup_log_utils.h"
#include "medialibrary_errno.h"
#include "media_file_utils.h"
#include "media_string_utils.h"

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
    // Scenario 3, WHEN FileInfo is not belongs to any album, THEN override lPath to the folder in sourcePath.
    // Note, sourcePath is a sign of the possible scenaio that the file is not in any album.
    const bool islPathMiss = fileInfo.lPath.empty() && !fileInfo.sourcePath.empty();
    if (islPathMiss) {
        lPath = this->photoAlbumDao_.ParseSourcePathToLPath(fileInfo.sourcePath);
        MEDIA_INFO_LOG("Media_Restore: fix lPath of album.fileInfo.lPath: %{public}s, "
                       "lPathFromSourcePath: %{public}s, lowercase: %{public}s, FileInfo Object: %{public}s",
            MediaFileUtils::DesensitizePath(fileInfo.lPath).c_str(),
            MediaFileUtils::DesensitizePath(lPath).c_str(),
            MediaFileUtils::DesensitizePath(this->ToLower(lPath)).c_str(),
            this->ToString(fileInfo).c_str());
    }
    // Scenario 1, WHEN FileInfo is in /Pictures/Screenshots and Video type, THEN redirect to /Pictures/Screenrecords
    if (this->ToLower(lPath) == this->ToLower(AlbumPlugin::LPATH_SCREEN_SHOTS) &&
        fileInfo.fileType == MediaType::MEDIA_TYPE_VIDEO) {
        albumInfo = this->photoAlbumDao_.BuildAlbumInfoOfRecorders();
        albumInfo = this->photoAlbumDao_.GetOrCreatePhotoAlbum(albumInfo);
        MEDIA_INFO_LOG("Media_Restore: screenshots redirect to screenrecords, fileInfo.lPath: %{public}s, "
                       "lPath: %{public}s, Object: %{public}s, albumInfo: %{public}s",
            MediaFileUtils::DesensitizePath(fileInfo.lPath).c_str(),
            MediaFileUtils::DesensitizePath(lPath).c_str(),
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
    CHECK_AND_RETURN_RET(!fileInfo.originalPackageName.empty(), "");
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

static void ClearStoragePathForClone(std::vector<FileInfo> &fileInfos)
{
    for (auto &fileInfo : fileInfos) {
        if (!FileAdapter::IsLakeFile(fileInfo) && !FileAdapter::IsFileManagerFile(fileInfo)) {
            fileInfo.inode.clear();
            fileInfo.storagePath.clear();
        }
    }
}

void PhotosClone::SetFilePathBase(std::vector<FileInfo> &fileInfos, AncoFileTransfer ancoFileTransfer)
{
    ClearStoragePathForClone(fileInfos);
    SetIsStoragePathExistInDb(fileInfos);
    SetIsCloudPathExistInDb(fileInfos);
}

void PhotosClone::SetFilePath(std::vector<FileInfo> &fileInfos, AncoFileTransfer ancoFileTransfer)
{
    UpdateFileInfoFromCloneRestoreDb(fileInfos, ancoFileTransfer);
    SetFilePathBase(fileInfos, ancoFileTransfer);
}

void PhotosClone::SetFilePathForReverseClone(std::vector<FileInfo> &fileInfos, AncoFileTransfer ancoFileTransfer)
{
    SetFilePathBase(fileInfos, ancoFileTransfer);
}

void PhotosClone::SetIsStoragePathExistInDb(std::vector<FileInfo> &fileInfos)
{
    int32_t existCount = 0;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    std::vector<std::string> storagePaths;
    for (const auto &fileInfo : fileInfos) {
        CHECK_AND_CONTINUE(FileAdapter::IsLakeFile(fileInfo));
        storagePaths.emplace_back(fileInfo.storagePath);
    }
    std::unordered_set<std::string> existingStoragePaths =
        photosDao_.GetExistingStoragePaths(storagePaths, photosBasicInfo_.maxFileId);
    for (auto &fileInfo : fileInfos) {
        CHECK_AND_CONTINUE(FileAdapter::IsLakeFile(fileInfo));
        fileInfo.isStoragePathExistInDb = existingStoragePaths.count(fileInfo.storagePath) > 0;
        existCount += fileInfo.isStoragePathExistInDb;
    }
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("LakeClone: TimeCost: SetIsStoragePathExistInDb cost %{public}" PRId64 "ms, existCount: %{public}d",
        endTime - startTime, existCount);
}

void PhotosClone::SetIsCloudPathExistInDb(std::vector<FileInfo> &fileInfos)
{
    int32_t existCount = 0;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    std::vector<std::string> cloudPaths;
    for (auto &fileInfo : fileInfos) {
        // Source file exists. Data update required.
        if (MediaFileUtils::IsFileExists(fileInfo.oldPath)) {
            fileInfo.isCloudPathExistInDb = true;
            existCount++;
            continue;
        }
        cloudPaths.emplace_back(fileInfo.oldPath);
    }
    std::unordered_set<std::string> existingCloudPaths =
        photosDao_.GetExistingData(cloudPaths, photosBasicInfo_.maxFileId);
    for (auto &fileInfo : fileInfos) {
        CHECK_AND_CONTINUE(!fileInfo.isCloudPathExistInDb);
        fileInfo.isCloudPathExistInDb = existingCloudPaths.count(fileInfo.oldPath) > 0;
        existCount += fileInfo.isCloudPathExistInDb;
    }
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("LakeClone: TimeCost: SetIsCloudPathExistInDb cost %{public}" PRId64 "ms, existCount: %{public}d",
        endTime - startTime, existCount);
}

RestoreError PhotosClone::IsFileSizeMatched(const FileInfo &fileInfo, const std::string &storagePath,
    int64_t &actualSize)
{
    struct stat statInfo {};
    CHECK_AND_RETURN_RET_LOG(stat(storagePath.c_str(), &statInfo) == E_SUCCESS, RestoreError::PATH_INVALID,
        "LakeClone: IsFileSizeMatched stat fail path: %{private}s", storagePath.c_str());
    actualSize = static_cast<int64_t>(statInfo.st_size);
    CHECK_AND_RETURN_RET_LOG(actualSize == fileInfo.fileSize, RestoreError::DEDUPLICATION_FILE_SIZE_MISMATCH,
        "LakeClone: IsFileSizeMatched size mismatch, actualSize: %{public}" PRId64 ", expectedSize: %{public}" PRId64,
        actualSize, fileInfo.fileSize);
    return RestoreError::SUCCESS;
}

std::string PhotosClone::GetNumberedStoragePath(const std::string &storagePath, uint32_t number)
{
    CHECK_AND_RETURN_RET(number > 0, storagePath);
    size_t pos = storagePath.find_last_of(".");
    CHECK_AND_RETURN_RET(pos != std::string::npos, storagePath + "(" + std::to_string(number) + ")");
    std::string title = storagePath.substr(0, pos);
    std::string extension = storagePath.substr(pos + 1);
    return title + "(" + std::to_string(number) + ")." + extension;
}

void PhotosClone::SetLakeFileInfo(FileInfo &fileInfo, const std::string &storagePath)
{
    struct stat statInfo {};
    CHECK_AND_RETURN(stat(storagePath.c_str(), &statInfo) == E_SUCCESS);
    fileInfo.inode = std::to_string(statInfo.st_ino);
    fileInfo.storagePath = storagePath;
    fileInfo.filePath = storagePath;
    fileInfo.displayName = MediaFileUtils::GetFileName(fileInfo.filePath);
    fileInfo.title = MediaFileUtils::GetTitleFromDisplayName(fileInfo.displayName);
    MEDIA_INFO_LOG("LakeClone: fileInfo inode: %{public}s, storagePath: %{public}s, displayName: %{private}s",
        fileInfo.inode.c_str(), BackupFileUtils::GarbleFilePath(storagePath, DEFAULT_RESTORE_ID).c_str(),
        BackupFileUtils::GarbleFilePath(fileInfo.displayName, DEFAULT_RESTORE_ID).c_str());
}

int32_t PhotosClone::GetLakeFileFailInfoCount()
{
    CHECK_AND_RETURN_RET(cloneRestoreRdbStore_ != nullptr, 0);
    
    bool isTableExist = false;
    CHECK_AND_RETURN_RET(BackupDatabaseUtils::isTableExist(cloneRestoreRdbStore_,
        LAKE_FILE_INFO_FAIL_TABLE, isTableExist), 0);
    CHECK_AND_RETURN_RET(isTableExist, 0);
    
    auto resultSet = cloneRestoreRdbStore_->QuerySql(SQL_QUERY_LAKE_FILE_FAIL_INFO_COUNT);
    CHECK_AND_RETURN_RET(resultSet != nullptr, 0);
    
    int32_t count = 0;
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        count = GetInt32Val("count", resultSet);
    }
    resultSet->Close();
    
    MEDIA_INFO_LOG("LakeClone: GetLakeFileFailInfoCount count: %{public}d", count);
    return count;
}

std::vector<std::string> PhotosClone::QueryLakeFileFailPathsBatch(int32_t offset)
{
    std::vector<std::string> failPaths;
    CHECK_AND_RETURN_RET(cloneRestoreRdbStore_ != nullptr, failPaths);
    
    std::vector<NativeRdb::ValueObject> params = { offset, QUERY_COUNT };
    auto resultSet = cloneRestoreRdbStore_->QuerySql(SQL_QUERY_LAKE_FILE_FAIL_INFO_BATCH, params);
    CHECK_AND_RETURN_RET(resultSet != nullptr, failPaths);
    
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string tmpFailPath = GetStringVal(PhotoColumn::CLONE_FILE_INFO_PATH, resultSet);
        std::string failPath = BackupFileUtils::ConvertToStoragePath(tmpFailPath);
        if (!failPath.empty()) {
            failPaths.emplace_back(failPath);
        }
    }
    resultSet->Close();
    
    MEDIA_INFO_LOG("LakeClone: QueryLakeFileFailPathsBatch offset: %{public}d, count: %{public}zu",
        offset, failPaths.size());
    return failPaths;
}

std::unordered_map<std::string, std::pair<int32_t, std::string>> PhotosClone::QueryLakeFileByFailPaths(
    const std::vector<std::string> &failPaths)
{
    std::unordered_map<std::string, std::pair<int32_t, std::string>> failPathInfoMap;
    CHECK_AND_RETURN_RET(!failPaths.empty(), failPathInfoMap);
    CHECK_AND_RETURN_RET(mediaLibraryOriginalRdb_ != nullptr, failPathInfoMap);
    
    std::string selection;
    for (const auto &failPath : failPaths) {
        BackupDatabaseUtils::UpdateSelection(selection, failPath, true);
    }
    
    std::string querySql = MediaStringUtils::FillParams(SQL_QUERY_LAKE_FILE_BY_FAIL_PATHS, { selection });
    auto resultSet = mediaLibraryOriginalRdb_->QuerySql(querySql);
    CHECK_AND_RETURN_RET(resultSet != nullptr, failPathInfoMap);
    
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string storagePath = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
        int32_t mediaType = GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet);
        std::string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        failPathInfoMap[storagePath] = {mediaType, displayName};
    }
    resultSet->Close();
    
    MEDIA_INFO_LOG("LakeClone: QueryLakeFileByFailPaths failPaths: %{public}zu, found: %{public}zu",
        failPaths.size(), failPathInfoMap.size());
    return failPathInfoMap;
}

void PhotosClone::ProcessLakeFileFailInfoBatch(int32_t offset,
    std::unordered_map<std::string, FailedFileInfo> &lakePhotoFailedFiles,
    std::unordered_map<std::string, FailedFileInfo> &lakeVideoFailedFiles)
{
    std::vector<std::string> failPaths = QueryLakeFileFailPathsBatch(offset);
    CHECK_AND_RETURN(!failPaths.empty());
    
    auto failPathInfoMap = QueryLakeFileByFailPaths(failPaths);
    CHECK_AND_RETURN(!failPathInfoMap.empty());
    
    for (const auto &pair : failPathInfoMap) {
        const std::string &failPath = pair.first;
        int32_t mediaType = pair.second.first;
        const std::string &displayName = pair.second.second;
        
        FileInfo fileInfo;
        fileInfo.displayName = displayName;
        fileInfo.storagePath = failPath;
        
        FailedFileInfo failedFileInfo(sceneCode_, fileInfo, RestoreError::ANCO_TRANSFER_FAILED);
        
        if (mediaType == static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE)) {
            lakePhotoFailedFiles.emplace(failPath, failedFileInfo);
        } else if (mediaType == static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO)) {
            lakeVideoFailedFiles.emplace(failPath, failedFileInfo);
        }
    }
    
    MEDIA_INFO_LOG("LakeClone: ProcessLakeFileFailInfoBatch offset: %{public}d, "
        "photoFail: %{public}zu, videoFail: %{public}zu",
        offset, lakePhotoFailedFiles.size(), lakeVideoFailedFiles.size());
}

void PhotosClone::QueryLakeFileFailInfo(
    std::unordered_map<std::string, FailedFileInfo> &lakePhotoFailedFiles,
    std::unordered_map<std::string, FailedFileInfo> &lakeVideoFailedFiles)
{
    lakePhotoFailedFiles.clear();
    lakeVideoFailedFiles.clear();
    
    CHECK_AND_RETURN_LOG(cloneRestoreRdbStore_ != nullptr, "cloneRestoreRdbStore is nullptr");
    
    int32_t totalCount = GetLakeFileFailInfoCount();
    MEDIA_INFO_LOG("LakeClone: QueryLakeFileFailInfo totalCount: %{public}d", totalCount);
    CHECK_AND_RETURN(totalCount != 0);
    
    for (int32_t offset = 0; offset < totalCount; offset += QUERY_COUNT) {
        ProcessLakeFileFailInfoBatch(offset, lakePhotoFailedFiles, lakeVideoFailedFiles);
    }
    
    MEDIA_INFO_LOG("LakeClone: QueryLakeFileFailInfo finished, totalCount: %{public}d, "
        "photoFail: %{public}zu, videoFail: %{public}zu",
        totalCount, lakePhotoFailedFiles.size(), lakeVideoFailedFiles.size());
}

void PhotosClone::InitDeduplicationInfo()
{
    int32_t ret = InitCloneRestoreRdbStore();
    CHECK_AND_RETURN_LOG(ret == E_OK, "LakeClone: failed to init CloneRestoreRdbStore");
    CHECK_AND_RETURN_LOG(cloneRestoreRdbStore_ != nullptr, "LakeClone: cloneRestoreRdbStore is nullptr");
    QueryDeduplicationFileInfo(deduplicationMap_);
    MEDIA_INFO_LOG("LakeClone: InitDeduplicationInfo completed, deduplicationMap size: %{public}zu",
        deduplicationMap_.size());
}

void PhotosClone::UpdateFileInfoFromCloneRestoreDb(std::vector<FileInfo> &fileInfos, AncoFileTransfer ancoFileTransfer)
{
    int32_t totalCount = static_cast<int32_t>(deduplicationMap_.size());
    int32_t matchedCount = 0;
    int32_t successCount = 0;
    int32_t failCount = 0;

    for (auto it = fileInfos.begin(); it != fileInfos.end();) {
        if (!FileAdapter::IsLakeFile(*it)) {
            ++it;
            continue;
        }
        if (ancoFileTransfer == AncoFileTransfer::ANCO_FILE_TRANSFER_NONE) {
            it = fileInfos.erase(it);
            continue;
        }
        auto dedupIt = deduplicationMap_.find(it->storagePath);
        if (dedupIt != deduplicationMap_.end()) {
            matchedCount++;
            if (ApplyDeduplicationFileInfo(*it, dedupIt->second)) {
                successCount++;
                ++it;
            } else {
                failCount++;
                it = fileInfos.erase(it);
            }
        } else {
            it->filePath = it->storagePath;
            ++it;
        }
    }

    MEDIA_INFO_LOG("LakeClone: UpdateFileInfoFromCloneRestoreDb isLakeTransfer: %{public}d, "
        "deduplicationMapSize: %{public}d, matched: %{public}d, success: %{public}d, fail: %{public}d",
        static_cast<int32_t>(ancoFileTransfer), totalCount, matchedCount, successCount, failCount);
}

int32_t PhotosClone::InitCloneRestoreRdbStore()
{
    std::string CLONE_RESTORE_DB_PATH = OTHER_CLONE_PATH + CLONE_FILE_INFO_RESTORE_DB;
    MEDIA_INFO_LOG("LakeClone: InitCloneRestoreRdbStore start, dbPath: %{private}s", CLONE_RESTORE_DB_PATH.c_str());

    if (!MediaFileUtils::IsFileExists(CLONE_RESTORE_DB_PATH)) {
        MEDIA_ERR_LOG("LakeClone: InitCloneRestoreRdbStore db file not exist, dbPath: %{private}s",
            CLONE_RESTORE_DB_PATH.c_str());
        return E_ERR;
    }

    CloneFileInfoRestoreDbCallback rdbDataCallcack;
    NativeRdb::RdbStoreConfig config("");
    config.SetName(CLONE_FILE_INFO_RESTORE_DB);
    config.SetPath(CLONE_RESTORE_DB_PATH);
    int32_t err = 0;
    cloneRestoreRdbStore_ = NativeRdb::RdbHelper::GetRdbStore(config, 1, rdbDataCallcack, err);
    if (cloneRestoreRdbStore_ == nullptr) {
        MEDIA_ERR_LOG("LakeClone: InitCloneRestoreRdbStore failed, GetRdbStore return nullptr, err: %{public}d", err);
        ErrorInfo errorInfo(RestoreError::OPEN_CLONE_RESTORE_DATABASE_FAILED, 1, std::to_string(err),
            "InitCloneRestoreRdbStore failed for " + CLONE_RESTORE_DB_PATH);
        UpgradeRestoreTaskReport(sceneCode_, taskId_).ReportErrorInAudit(errorInfo);
        return E_ERR;
    }

    return E_OK;
}

void PhotosClone::QueryDeduplicationFileInfo(std::unordered_map<std::string, DeduplicationInfo> &deduplicationMap)
{
    deduplicationMap.clear();

    bool isTableExist = false;
    CHECK_AND_RETURN_LOG(BackupDatabaseUtils::isTableExist(cloneRestoreRdbStore_,
        LAKE_FILE_INFO_DEDUPLICATION_TABLE, isTableExist), "LakeClone: fail to check table exist");
    CHECK_AND_RETURN_WARN_LOG(isTableExist, "LakeClone: LAKE_FILE_INFO_DEDUPLICATION_TABLE not exist");

    auto resultSet = cloneRestoreRdbStore_->QuerySql(SQL_QUERY_DEDUPLICATION_FILE_INFO);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("LakeClone: QueryDeduplicationFileInfo failed, QuerySql return nullptr");
        ErrorInfo errorInfo(RestoreError::CLONE_RESTORE_DATABASE_CORRUPTION, 1, "",
            "QueryDeduplicationFileInfo failed for table: " + LAKE_FILE_INFO_DEDUPLICATION_TABLE);
        UpgradeRestoreTaskReport(sceneCode_, taskId_).ReportErrorInAudit(errorInfo);
        return;
    }

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        DeduplicationInfo deduplicationInfo;
        std::string tmpPath = GetStringVal(PhotoColumn::CLONE_FILE_INFO_PATH, resultSet);
        std::string tmpNewPath = GetStringVal(PhotoColumn::CLONE_FILE_INFO_NEW_PATH, resultSet);
        deduplicationInfo.path = BackupFileUtils::ConvertToStoragePath(tmpPath);
        deduplicationInfo.newPath = BackupFileUtils::ConvertToStoragePath(tmpNewPath);
        deduplicationMap[deduplicationInfo.path] = deduplicationInfo;
    }
    resultSet->Close();
    MEDIA_INFO_LOG("LakeClone: QueryDeduplicationFileInfo deduplicationMap size: %{public}zu",
        deduplicationMap.size());
}

bool PhotosClone::ApplyDeduplicationFileInfo(FileInfo &fileInfo, const DeduplicationInfo &deduplicationInfo)
{
    if (deduplicationInfo.newPath.empty()) {
        MEDIA_ERR_LOG("LakeClone: ApplyDeduplicationFileInfo failed, newPath is empty, path: %{private}s",
            deduplicationInfo.path.c_str());
        return false;
    }

    int64_t actualSize = 0;
    RestoreError errCode = IsFileSizeMatched(fileInfo, deduplicationInfo.newPath, actualSize);
    if (errCode != RestoreError::SUCCESS) {
        ErrorInfo errorInfo(errCode, 1, "", "oldPath=" + deduplicationInfo.path + ", newPath="
            + deduplicationInfo.newPath + ", oldSize=" + std::to_string(fileInfo.fileSize) +
            ", newSize=" + std::to_string(actualSize));
        UpgradeRestoreTaskReport(sceneCode_, taskId_).ReportErrorInAudit(errorInfo);
        return false;
    }

    struct stat statInfo {};
    CHECK_AND_EXECUTE(stat(deduplicationInfo.newPath.c_str(), &statInfo) != E_SUCCESS,
        fileInfo.inode = std::to_string(statInfo.st_ino));
    fileInfo.storagePath = deduplicationInfo.newPath;
    fileInfo.filePath = deduplicationInfo.newPath;
    fileInfo.displayName = MediaFileUtils::GetFileName(fileInfo.filePath);
    fileInfo.title = MediaFileUtils::GetTitleFromDisplayName(fileInfo.displayName);

    MEDIA_INFO_LOG("LakeClone: Update fileInfo from CloneRestore db, fileIdOld: %{public}d, "
        "newPath: %{private}s, newDisplayName: %{private}s", fileInfo.fileIdOld,
        BackupFileUtils::GarbleFilePath(deduplicationInfo.newPath, DEFAULT_RESTORE_ID).c_str(),
        BackupFileUtils::GarbleFileName(fileInfo.displayName).c_str());
    return true;
}

bool PhotosClone::ShouldDeleteDuplicateLakeFile(const FileInfo &fileInfo)
{
    CHECK_AND_RETURN_RET(FileAdapter::IsLakeFile(fileInfo), true);
    // keep source only when target is pure-cloud(position=2) and container original already exists;
    // otherwise always delete.
    bool keepSource = (fileInfo.needMove|| fileInfo.isStoragePathExistInDb);
    return !keepSource;
}

bool PhotosClone::IsCloudPathExist(const FileInfo &fileInfo)
{
    return fileInfo.isCloudPathExistInDb;
}

int32_t PhotosClone::CreateCloudPath(int32_t uniqueId, FileInfo &fileInfo)
{
    int32_t fixedBucketNum = FileAdapter::IsLakeFile(fileInfo) ? 0 : -1;
    return BackupFileUtils::CreateAssetPathById(uniqueId, fileInfo.fileType,
        MediaFileUtils::GetExtensionFromPath(fileInfo.displayName), fileInfo.cloudPath, fixedBucketNum);
}
}  // namespace OHOS::Media
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
#include "photos_dao.h"

#include <string>
#include <vector>
#include <sstream>
#include <unordered_set>

#include "backup_database_utils.h"
#include "rdb_store.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"
#include "backup_const.h"
#include "media_string_utils.h"

namespace OHOS::Media {
/**
 * @brief Find FileInfo related PhotoAlbu, by lPath, displayName, fileSize and orientation.
 */
PhotosDao::PhotosRowData PhotosDao::FindSameFileInAlbum(const FileInfo &fileInfo, int32_t maxFileId)
{
    PhotosDao::PhotosRowData rowData;
    CHECK_AND_RETURN_RET(maxFileId > 0, rowData);
    // pictureFlag: 0 for video, 1 for photo; Only search for photo in this case.
    int pictureFlag = fileInfo.fileType == MEDIA_TYPE_VIDEO ? 0 : 1;
    const std::vector<NativeRdb::ValueObject> params = {
        fileInfo.lPath, maxFileId, fileInfo.displayName, fileInfo.fileSize, pictureFlag, fileInfo.orientation};
    std::string querySql = this->SQL_PHOTOS_FIND_SAME_FILE_IN_ALBUM;
    CHECK_AND_RETURN_RET_LOG(this->mediaLibraryRdb_ != nullptr, rowData, "Media_Restore: mediaLibraryRdb_ is null.");
    auto resultSet = this->mediaLibraryRdb_->QuerySql(querySql, params);
    CHECK_AND_RETURN_RET(resultSet != nullptr, rowData);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return rowData;
    }
    ParseResultSetOfSameFile(rowData, resultSet);
    resultSet->Close();
    return rowData;
}

/**
 * @brief Find FileInfo, which is not related PhotoAlbum, by displayName, fileSize and orientation.
 */
PhotosDao::PhotosRowData PhotosDao::FindSameFileWithoutAlbum(const FileInfo &fileInfo, int32_t maxFileId)
{
    PhotosDao::PhotosRowData rowData;
    CHECK_AND_RETURN_RET(maxFileId > 0, rowData);
    // pictureFlag: 0 for video, 1 for photo; Only search for photo in this case.
    int pictureFlag = fileInfo.fileType == MEDIA_TYPE_VIDEO ? 0 : 1;
    const std::vector<NativeRdb::ValueObject> params = {
        maxFileId, fileInfo.displayName, fileInfo.fileSize, pictureFlag, fileInfo.orientation};
    std::string querySql = this->SQL_PHOTOS_FIND_SAME_FILE_WITHOUT_ALBUM;
    CHECK_AND_RETURN_RET_LOG(this->mediaLibraryRdb_ != nullptr, rowData, "Media_Restore: mediaLibraryRdb_ is null.");

    auto resultSet = this->mediaLibraryRdb_->QuerySql(querySql, params);
    CHECK_AND_RETURN_RET(resultSet != nullptr, rowData);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return rowData;
    }
    ParseResultSetOfSameFile(rowData, resultSet);
    resultSet->Close();
    return rowData;
}

/**
 * @brief Find FileInfo, which is cloud photo, by cloud_id.
 */
PhotosDao::PhotosRowData PhotosDao::FindSameFileWithCloudId(const FileInfo &fileInfo, int32_t maxFileId)
{
    PhotosDao::PhotosRowData rowData;
    CHECK_AND_RETURN_RET(maxFileId > 0, rowData);
    const std::vector<NativeRdb::ValueObject> params = {
        maxFileId, fileInfo.uniqueId};
    std::string querySql = this->SQL_PHOTOS_FIND_SAME_FILE_WITH_CLOUD_ID;
    CHECK_AND_RETURN_RET_LOG(this->mediaLibraryRdb_ != nullptr, rowData, "Media_Restore: mediaLibraryRdb_ is null.");

    auto resultSet = this->mediaLibraryRdb_->QuerySql(querySql, params);
    CHECK_AND_RETURN_RET(resultSet != nullptr, rowData);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return rowData;
    }
    ParseResultSetOfSameFile(rowData, resultSet);
    resultSet->Close();
    return rowData;
}

void PhotosDao::ParseResultSetOfSameFile(PhotosDao::PhotosRowData &rowData,
    std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
    rowData.fileId = GetInt32Val("file_id", resultSet);
    rowData.data = GetStringVal("data", resultSet);
    rowData.displayName = GetStringVal("display_name", resultSet);
    rowData.fileSize = GetInt64Val("size", resultSet);
    rowData.orientation = GetInt32Val("orientation", resultSet);
    rowData.cleanFlag = GetInt32Val("clean_flag", resultSet);
    rowData.position = GetInt32Val("position", resultSet);
    rowData.fileSourceType = GetInt32Val("file_source_type", resultSet);
}

/**
 * @brief Get basic information of the Photos.
 */
PhotosDao::PhotosBasicInfo PhotosDao::GetBasicInfo()
{
    PhotosDao::PhotosBasicInfo basicInfo = {0, 0};
    std::string querySql = this->SQL_PHOTOS_BASIC_INFO;
    CHECK_AND_RETURN_RET_LOG(this->mediaLibraryRdb_ != nullptr, basicInfo,
        "Media_Restore: mediaLibraryRdb_ is null.");
    auto resultSet = this->mediaLibraryRdb_->QuerySql(querySql);
    CHECK_AND_RETURN_RET_WARN_LOG(resultSet != nullptr, basicInfo, "Media_Restore: resultSet is null.");
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_WARN_LOG("Media_Restore: GetBasicInfo resultSet is null. querySql: %{public}s", querySql.c_str());
        resultSet->Close();
        return basicInfo;
    }
    basicInfo.maxFileId = GetInt32Val("max_file_id", resultSet);
    basicInfo.count = GetInt32Val("count", resultSet);
    resultSet->Close();
    MEDIA_INFO_LOG("Media_Restore: max_file_id: %{public}d, count: %{public}d", basicInfo.maxFileId, basicInfo.count);
    return basicInfo;
}

/**
 * @brief Find same file info by lPath, displayName, size, orientation.
 * lPath - if original fileInfo's lPath is empty, it will be ignored.
 * orientation - if original fileInfo's fileType is not Video(2), it will be ignored.
 */
PhotosDao::PhotosRowData PhotosDao::FindSameFile(const FileInfo &fileInfo, const int32_t maxFileId)
{
    PhotosDao::PhotosRowData rowData;
    CHECK_AND_RETURN_RET(maxFileId > 0, rowData);
    if (!fileInfo.uniqueId.empty()) {
        rowData = this->FindSameFileWithCloudId(fileInfo, maxFileId);
        CHECK_AND_RETURN_RET(!rowData.IsValid(), rowData);
    }
    if (fileInfo.lPath.empty()) {
        rowData = this->FindSameFileWithoutAlbum(fileInfo, maxFileId);
        MEDIA_ERR_LOG("Media_Restore: FindSameFile - lPath is empty, DB Info: %{public}s, Object: %{public}s",
            this->ToString(rowData).c_str(), this->ToString(fileInfo).c_str());
        return rowData;
    }
    rowData = this->FindSameFileInAlbum(fileInfo, maxFileId);
    CHECK_AND_RETURN_RET(!rowData.IsValid(), rowData);

    rowData = this->FindSameFileBySourcePath(fileInfo, maxFileId);
    CHECK_AND_RETURN_RET_WARN_LOG(!rowData.IsValid(), rowData,
        "Media_Restore: FindSameFile - find Photos by sourcePath, DB Info: %{public}s, Object: %{public}s",
        this->ToString(rowData).c_str(), this->ToString(fileInfo).c_str());
    return rowData;
}

/**
 * @brief Find FileInfo not related PhotoAlbum, by sourcePath, displayName, fileSize and orientation.
 */
PhotosDao::PhotosRowData PhotosDao::FindSameFileBySourcePath(const FileInfo &fileInfo, const int32_t maxFileId)
{
    PhotosDao::PhotosRowData rowData;
    CHECK_AND_RETURN_RET(maxFileId > 0, rowData);
    // pictureFlag: 0 for video, 1 for photo; Only search for photo in this case.
    int pictureFlag = fileInfo.fileType == MEDIA_TYPE_VIDEO ? 0 : 1;
    std::string sourcePath = this->SOURCE_PATH_PREFIX + fileInfo.lPath + "/" + fileInfo.displayName;
    const std::vector<NativeRdb::ValueObject> params = {
        sourcePath, maxFileId, fileInfo.displayName, fileInfo.fileSize, pictureFlag, fileInfo.orientation};
    std::string querySql = this->SQL_PHOTOS_FIND_SAME_FILE_BY_SOURCE_PATH;
    CHECK_AND_RETURN_RET_LOG(this->mediaLibraryRdb_ != nullptr, rowData, "Media_Restore: mediaLibraryRdb_ is null.");

    auto resultSet = this->mediaLibraryRdb_->QuerySql(querySql, params);
    CHECK_AND_RETURN_RET(resultSet != nullptr, rowData);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return rowData;
    }
    ParseResultSetOfSameFile(rowData, resultSet);
    resultSet->Close();
    return rowData;
}

std::string PhotosDao::ToString(const FileInfo &fileInfo)
{
    std::stringstream ss;
    ss << "FileInfo[ fileId: " << fileInfo.fileIdOld << ", displayName: " << fileInfo.displayName
       << ", bundleName: " << fileInfo.bundleName << ", lPath: " << fileInfo.lPath << ", size: " << fileInfo.fileSize
       << ", fileType: " << fileInfo.fileType << ", oldPath: " << fileInfo.oldPath
       << ", sourcePath: " << fileInfo.sourcePath << " ]";
    return ss.str();
}

std::string PhotosDao::ToString(const PhotosDao::PhotosRowData &rowData)
{
    std::stringstream ss;
    ss << "PhotosRowData[ fileId: " << rowData.fileId << ", data: " << rowData.data
       << ", displayName: " << rowData.displayName << ", size: " << rowData.fileSize
       << ", orientation: " << rowData.orientation << " ]";
    return ss.str();
}

std::string PhotosDao::ToLower(const std::string &str)
{
    std::string lowerStr;
    std::transform(
        str.begin(), str.end(), std::back_inserter(lowerStr), [](unsigned char c) { return std::tolower(c); });
    return lowerStr;
}

int32_t PhotosDao::GetDirtyFilesCount()
{
    std::vector<NativeRdb::ValueObject> params = { static_cast<int32_t>(SyncStatusType::TYPE_BACKUP) };
    return BackupDatabaseUtils::QueryInt(mediaLibraryRdb_, SQL_PHOTOS_GET_DIRTY_FILES_COUNT, "count", params);
}

std::vector<PhotosDao::PhotosRowData> PhotosDao::GetDirtyFiles(int32_t offset)
{
    std::vector<PhotosDao::PhotosRowData> rowDataList;
    std::vector<NativeRdb::ValueObject> params = { static_cast<int32_t>(SyncStatusType::TYPE_BACKUP),
        offset, QUERY_COUNT };
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, SQL_PHOTOS_GET_DIRTY_FILES, params);
    CHECK_AND_RETURN_RET(resultSet != nullptr, rowDataList);
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        PhotosDao::PhotosRowData rowData;
        rowData.data = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        rowData.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        rowData.position = GetInt32Val(PhotoColumn::PHOTO_POSITION, resultSet);
        rowData.subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
        rowDataList.emplace_back(rowData);
    }
    resultSet->Close();
    return rowDataList;
}

int32_t PhotosDao::GetBackupMediaCount(const std::vector<int32_t> &mediaTypes,
    const std::vector<int32_t> &fileSourceTypes, const std::vector<int32_t> &positionTypes)
{
    std::vector<std::string> bindArgs = { BackupDatabaseUtils::JoinValues(mediaTypes, ","),
        BackupDatabaseUtils::JoinValues(fileSourceTypes, ","), BackupDatabaseUtils::JoinValues(positionTypes, ",") };
    std::string querySql = MediaStringUtils::FillParams(SQL_PHOTOS_GET_MEDIA_COUNT, bindArgs);
    return BackupDatabaseUtils::QueryInt(mediaLibraryRdb_, querySql, "count");
}

int32_t PhotosDao::GetBackupAudioCount(const std::vector<int32_t> &mediaTypes)
{
    std::vector<std::string> bindArgs = { BackupDatabaseUtils::JoinValues(mediaTypes, ",") };
    std::string querySql = MediaStringUtils::FillParams(SQL_AUDIOS_GET_AUDIO_COUNT, bindArgs);
    return BackupDatabaseUtils::QueryInt(mediaLibraryRdb_, querySql, "count");
}

int64_t PhotosDao::GetAssetTotalSizeByFileSourceType(int32_t fileSourceType)
{
    std::string mediaVolumeQuery = GetPhotosSizeSqlByFileSourceType(fileSourceType);
    std::string thumbSizeSql = GetThumbSizeSqlByFileSourceType(fileSourceType);
    CHECK_AND_EXECUTE(thumbSizeSql.empty(), mediaVolumeQuery += " UNION ALL " + thumbSizeSql);
    std::string audiosSizeSql = GetAudiosSizeSqlByFileSourceType(fileSourceType);
    CHECK_AND_EXECUTE(audiosSizeSql.empty(), mediaVolumeQuery += " UNION ALL " + audiosSizeSql);

    auto resultSet = BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, mediaVolumeQuery);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, 0, "Failed to execute media volume query");

    int64_t totalVolume = 0;
    MEDIA_INFO_LOG("Initial totalVolume: %{public}" PRId64, totalVolume);

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int64_t mediaSize = GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet);
        int32_t mediatype = GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet);
        MEDIA_INFO_LOG("mediatype is %{public}d, current media asset size is: %{public}" PRId64, mediatype, mediaSize);
        CHECK_AND_PRINT_LOG(mediaSize >= 0,
            "ill mediaSize: %{public}" PRId64 " for mediatype: %{public}d", mediaSize, mediatype);
        totalVolume += mediaSize;
    }
    resultSet->Close();
    MEDIA_INFO_LOG("media db media asset size is: %{public}" PRId64, totalVolume);

    CHECK_AND_RETURN_RET_LOG(totalVolume >= 0, totalVolume,
        "totalVolume is negative: %{public}" PRId64 ". Return 0.", totalVolume);
    return totalVolume;
}

std::string PhotosDao::GetPhotosSizeSqlByFileSourceType(int32_t fileSourceType)
{
    return "SELECT sum(" + MediaColumn::MEDIA_SIZE + ") AS " + MediaColumn::MEDIA_SIZE + "," + MediaColumn::MEDIA_TYPE +
        " FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " + "(" +
        MediaColumn::MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_IMAGE) + " OR " +
        MediaColumn::MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_VIDEO) + ") AND " +
        PhotoColumn::PHOTO_POSITION + " != 2 AND " +
        PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " = " + std::to_string(fileSourceType) +
        " GROUP BY " + MediaColumn::MEDIA_TYPE;
}

std::string PhotosDao::GetThumbSizeSqlByFileSourceType(int32_t fileSourceType)
{
    CHECK_AND_RETURN_RET(fileSourceType == FileSourceType::MEDIA, "");
    return "SELECT SUM(CAST(" + PhotoExtColumn::THUMBNAIL_SIZE + " AS BIGINT)) AS " + CONST_MEDIA_DATA_DB_SIZE +
        ", -1 AS " + MediaColumn::MEDIA_TYPE + " FROM " + PhotoExtColumn::PHOTOS_EXT_TABLE;
}

std::string PhotosDao::GetAudiosSizeSqlByFileSourceType(int32_t fileSourceType)
{
    CHECK_AND_RETURN_RET(fileSourceType == FileSourceType::MEDIA, "");
    return AudioColumn::QUERY_MEDIA_VOLUME;
}

std::unordered_set<std::string> PhotosDao::GetExistingStoragePaths(const std::vector<std::string> &storagePaths,
    int32_t maxFileId)
{
    std::unordered_set<std::string> existingStoragePaths;
    CHECK_AND_RETURN_RET(maxFileId > 0, existingStoragePaths);

    std::string selection;
    for (const auto &storagePath : storagePaths) {
        BackupDatabaseUtils::UpdateSelection(selection, storagePath, true);
    }
    std::string querySql = MediaStringUtils::FillParams(SQL_PHOTOS_GET_EXISTING_STORAGE_PATHS, { selection });
    std::vector<NativeRdb::ValueObject> params = { maxFileId,
        FileSourceType::MEDIA_HO_LAKE, FileSourceType::FILE_MANAGER };
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, querySql, params);
    CHECK_AND_RETURN_RET(resultSet != nullptr, existingStoragePaths);
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string storagePath = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
        existingStoragePaths.insert(storagePath);
    }
    resultSet->Close();
    return existingStoragePaths;
}

std::unordered_set<std::string> PhotosDao::GetExistingData(const std::vector<std::string> &data, int32_t maxFileId)
{
    std::unordered_set<std::string> existingData;
    CHECK_AND_RETURN_RET(maxFileId > 0, existingData);

    std::string selection;
    for (const auto &datum : data) {
        BackupDatabaseUtils::UpdateSelection(selection, datum, true);
    }
    std::string querySql = MediaStringUtils::FillParams(SQL_PHOTOS_GET_EXISTING_DATA, { selection });
    std::vector<NativeRdb::ValueObject> params = { maxFileId };
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, querySql, params);
    CHECK_AND_RETURN_RET(resultSet != nullptr, existingData);
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string data = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        existingData.insert(data);
    }
    resultSet->Close();
    return existingData;
}

std::shared_ptr<NativeRdb::ResultSet> PhotosDao::QueryCloneFileInfo(const std::vector<int32_t> &fileSourceTypes)
{
    CHECK_AND_RETURN_RET(mediaLibraryRdb_ != nullptr, nullptr);

    std::string selection;
    for (const auto &fileSourceType : fileSourceTypes) {
        BackupDatabaseUtils::UpdateSelection(selection, std::to_string(fileSourceType), true);
    }
    std::string querySql =  MediaStringUtils::FillParams(SQL_GET_CLONE_FILE_INFO, { selection });
    return BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, querySql);
}

static void BatchInsertCloneFileInfo(std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string tableName,
    std::vector<NativeRdb::ValuesBucket> &values, int64_t &totalFailCount)
{
    CHECK_AND_RETURN_LOG(!values.empty(), "input ValuesBucket is empty");
    int64_t rowNum = 0;
    int32_t ret = BackupDatabaseUtils::BatchInsert(rdbStore, tableName, values, rowNum);
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK, "LakeClone: BatchInsert file info fail, ret = %{public}d", ret);
    if (rowNum != static_cast<int64_t>(values.size())) {
        totalFailCount += static_cast<int64_t>(values.size()) - rowNum;
    }
}
    
int32_t PhotosDao::InsertCloneFileInfo(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
    std::shared_ptr<NativeRdb::ResultSet> resultSet, AncoFileListClone ancoFileListClone,
    FileManagerFileListClone fileManagerFileListClone)
{
    CHECK_AND_RETURN_RET(rdbStore != nullptr, NativeRdb::E_ERROR);
    CHECK_AND_RETURN_RET(resultSet != nullptr, NativeRdb::E_ERROR);

    std::vector<NativeRdb::ValuesBucket> lakeValues;
    std::vector<NativeRdb::ValuesBucket> fileManagerValues;
    int32_t unexpectedCount = 0;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        NativeRdb::ValuesBucket value;
        value.PutInt(MediaColumn::MEDIA_ID, GetInt32Val(MediaColumn::MEDIA_ID, resultSet));
        value.PutString(PhotoColumn::CLONE_FILE_INFO_PATH, GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet));
        value.PutString(MediaColumn::MEDIA_NAME, GetStringVal(MediaColumn::MEDIA_NAME, resultSet));
        value.PutInt(MediaColumn::MEDIA_TYPE, GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet));
        value.PutLong(MediaColumn::MEDIA_SIZE, GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet));
        value.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet));
        int32_t fileSourceType = GetInt32Val(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, resultSet);
        if (fileSourceType == FileSourceType::MEDIA_HO_LAKE &&
                ancoFileListClone == AncoFileListClone::ANCO_FILE_LIST_CLONE_SUPPORTED) {
            lakeValues.push_back(value);
        } else if (fileSourceType == FileSourceType::FILE_MANAGER &&
            fileManagerFileListClone == FileManagerFileListClone::FILE_MANAGER_FILE_LIST_CLONE_SUPPORTED) {
            fileManagerValues.push_back(value);
        } else {
            MEDIA_WARN_LOG("Unexpected file source: %{public}d", fileSourceType);
            unexpectedCount++;
        }
    }
    resultSet->Close();
    int64_t totalFailCount = 0;
    BatchInsertCloneFileInfo(rdbStore, LAKE_FILE_INFO_TABLE, lakeValues, totalFailCount);
    BatchInsertCloneFileInfo(rdbStore, FILE_MANAGER_INFO_TABLE, fileManagerValues, totalFailCount);
    MEDIA_INFO_LOG("LakeClone: InsertCloneFileInfo finished, lake: %{public}zu, fileManager: %{public}zu, "
        "unexpectedCount: %{public}d, failed: %{public}" PRId64, lakeValues.size(), fileManagerValues.size(),
        unexpectedCount, totalFailCount);
    return NativeRdb::E_OK;
}

int32_t PhotosDao::GetCloneFileInfo(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
    AncoFileListClone ancoFileListClone, FileManagerFileListClone fileManagerFileListClone)
{
    CHECK_AND_RETURN_RET(rdbStore != nullptr, NativeRdb::E_ERROR);

    std::vector<int32_t> fileSourceTypes;
    if (ancoFileListClone == AncoFileListClone::ANCO_FILE_LIST_CLONE_SUPPORTED) {
        fileSourceTypes.push_back(FileSourceType::MEDIA_HO_LAKE);
    }
    if (fileManagerFileListClone == FileManagerFileListClone::FILE_MANAGER_FILE_LIST_CLONE_SUPPORTED) {
        fileSourceTypes.push_back(FileSourceType::FILE_MANAGER);
    }
    CHECK_AND_RETURN_RET(!fileSourceTypes.empty(), NativeRdb::E_OK);

    auto resultSet = QueryCloneFileInfo(fileSourceTypes);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, NativeRdb::E_ERROR, "PhotosDao::GetCloneFileInfo fail");

    return InsertCloneFileInfo(rdbStore, resultSet, ancoFileListClone, fileManagerFileListClone);
}
}  // namespace OHOS::Media
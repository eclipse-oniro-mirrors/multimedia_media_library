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

#include "backup_database_utils.h"
#include "rdb_store.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"
#include "backup_const.h"

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
    rowData.fileId = GetInt32Val("file_id", resultSet);
    rowData.data = GetStringVal("data", resultSet);
    rowData.cleanFlag = GetInt32Val("clean_flag", resultSet);
    rowData.position = GetInt32Val("position", resultSet);
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
    rowData.fileId = GetInt32Val("file_id", resultSet);
    rowData.data = GetStringVal("data", resultSet);
    rowData.cleanFlag = GetInt32Val("clean_flag", resultSet);
    rowData.position = GetInt32Val("position", resultSet);
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
    rowData.fileId = GetInt32Val("file_id", resultSet);
    rowData.data = GetStringVal("data", resultSet);
    rowData.cleanFlag = GetInt32Val("clean_flag", resultSet);
    rowData.position = GetInt32Val("position", resultSet);
    resultSet->Close();
    return rowData;
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
    rowData.fileId = GetInt32Val("file_id", resultSet);
    rowData.data = GetStringVal("data", resultSet);
    rowData.cleanFlag = GetInt32Val("clean_flag", resultSet);
    rowData.position = GetInt32Val("position", resultSet);
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
    ss << "PhotosRowData[ fileId: " << rowData.fileId << ", data: " << rowData.data << " ]";
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
}  // namespace OHOS::Media
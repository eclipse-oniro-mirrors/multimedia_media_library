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
#define MLOG_TAG "MediaLibraryBackupUtils"

#include "backup_database_helper.h"
#include "backup_database_utils.h"
#include "backup_file_utils.h"
#include "media_file_utils.h"
#include "media_log.h"

namespace OHOS::Media {
void BackupDatabaseHelper::Init(int32_t sceneCode, bool shouldIncludeSd, const std::string &prefix)
{
    CHECK_AND_RETURN_INFO_LOG(sceneCode == DUAL_FRAME_CLONE_RESTORE_ID,
        "sceneCode is dual clone restore");
    std::vector<int32_t> dbTypeList;
    if (shouldIncludeSd) {
        dbTypeList = { DbType::PHOTO_CACHE, DbType::VIDEO_CACHE, DbType::PHOTO_SD_CACHE, DbType::VIDEO_SD_CACHE };
    } else {
        dbTypeList = { DbType::PHOTO_CACHE, DbType::VIDEO_CACHE };
    }
    InitDb(dbTypeList, prefix);
}

void BackupDatabaseHelper::InitDb(int32_t dbType, const std::string &prefix)
{
    CHECK_AND_RETURN_LOG(!HasDb(dbType), "Db %{public}d already exists", dbType);
    CHECK_AND_RETURN_LOG(DB_INFO_MAP.count(dbType) != 0, "No such db type: %{public}d", dbType);
    DbInfo dbInfo = DB_INFO_MAP.at(dbType);
    std::string dbFullPath = prefix + dbInfo.path;
    CHECK_AND_RETURN_LOG(MediaFileUtils::IsFileExists(dbFullPath),
        "Db not exist, type: %{public}d, path: %{public}s", dbType,
        BackupFileUtils::GarbleFilePath(dbFullPath, DEFAULT_RESTORE_ID).c_str());

    int32_t errCode = BackupDatabaseUtils::InitDb(dbInfo.rdbStore, dbInfo.name, dbFullPath, BUNDLE_NAME, false);
    CHECK_AND_RETURN_LOG(dbInfo.rdbStore != nullptr,
        "Init db failed, type: %{public}d, errCode: %{public}d", dbType, errCode);
    dbInfoMap_[dbType] = dbInfo;
    MEDIA_INFO_LOG("Init db succeeded, type: %{public}d, current size: %{public}zu", dbType, dbInfoMap_.size());
}

void BackupDatabaseHelper::InitDb(const std::vector<int32_t> &dbTypeList, const std::string &prefix)
{
    for (auto dbType : dbTypeList) {
        InitDb(dbType, prefix);
    }
}

void BackupDatabaseHelper::AddDb(int32_t dbType, std::shared_ptr<NativeRdb::RdbStore> rdbStore)
{
    if (HasDb(dbType)) {
        MEDIA_WARN_LOG("Db %{public}d already exists", dbType);
        return;
    }
    dbInfoMap_[dbType] = DbInfo(rdbStore);
    MEDIA_INFO_LOG("Add db succeeded, type: %{public}d, current size: %{public}zu", dbType, dbInfoMap_.size());
}

void BackupDatabaseHelper::IsFileExist(int32_t sceneCode, const FileInfo &fileInfo, int32_t &dbType, int32_t &dbStatus,
    int32_t &fileStatus)
{
    FileQueryInfo fileQueryInfo;
    GetFileQueryInfo(sceneCode, fileInfo, fileQueryInfo);
    dbType = fileQueryInfo.dbType;
    if (!HasDb(fileQueryInfo.dbType)) {
        dbStatus = E_DB_FAIL;
        return;
    }
    DbInfo dbInfo = dbInfoMap_.at(fileQueryInfo.dbType);
    std::string querySql = "SELECT count(1) as count FROM " + fileQueryInfo.tableName + " WHERE " +
        fileQueryInfo.columnName + " = ?";
    std::vector<std::string> queryArgs = { fileQueryInfo.path };
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(dbInfo.rdbStore, querySql, queryArgs);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK) {
        dbStatus = E_HAS_DB_ERROR;
        return;
    }
    dbStatus = E_OK;
    fileStatus = GetInt32Val(CUSTOM_COUNT, resultSet) > 0 ? E_OK : E_NO_SUCH_FILE;
    resultSet->Close();
}

bool BackupDatabaseHelper::HasDb(int32_t dbType)
{
    return dbInfoMap_.count(dbType) > 0;
}

void BackupDatabaseHelper::GetFileQueryInfo(int32_t sceneCode, const FileInfo &fileInfo, FileQueryInfo &fileQueryInfo)
{
    if (sceneCode == UPGRADE_RESTORE_ID) {
        fileQueryInfo = FileQueryInfo(DbType::EXTERNAL, "files", "_data", fileInfo.oldPath);
        return;
    }
    int32_t dbType;
    if (fileInfo.isInternal) {
        dbType = fileInfo.fileType == DUAL_MEDIA_TYPE::IMAGE_TYPE ? DbType::PHOTO_CACHE : DbType::VIDEO_CACHE;
    } else {
        dbType = fileInfo.fileType == DUAL_MEDIA_TYPE::IMAGE_TYPE ? DbType::PHOTO_SD_CACHE : DbType::VIDEO_SD_CACHE;
    }
    std::string tableName = fileInfo.fileSize >= TAR_FILE_LIMIT ? "normal_file" : "small_file";
    fileQueryInfo = FileQueryInfo(dbType, tableName, "filepath", fileInfo.oldPath);
}
} // namespace OHOS::Media
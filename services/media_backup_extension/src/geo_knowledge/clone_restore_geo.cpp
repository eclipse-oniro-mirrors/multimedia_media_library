/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#define MLOG_TAG "CloneRestoreGeo"

#include "clone_restore_geo.h"

#include "backup_database_utils.h"
#include "locale_config.h"
#include "medialibrary_unistore_manager.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "result_set_utils.h"
#include "upgrade_restore_task_report.h"
#include "location_column.h"
#include "vision_column_comm.h"

namespace OHOS::Media {
const std::string INTEGER = "INTEGER";

void CloneRestoreGeo::Init(int32_t sceneCode, const std::string &taskId,
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, std::shared_ptr<NativeRdb::RdbStore> mediaRdb)
{
    sceneCode_ = sceneCode;
    taskId_ = taskId;
    mediaLibraryRdb_ = mediaLibraryRdb;
    mediaRdb_ = mediaRdb;
    systemLanguage_ = Global::I18n::LocaleConfig::GetSystemLanguage();
    analysisType_ = "geo";
}

void CloneRestoreGeo::Restore(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    CHECK_AND_RETURN_LOG(mediaRdb_ != nullptr && mediaLibraryRdb_ != nullptr,
        "Restore failed, rdbStore is nullptr");

    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    GetMaxIds();
    cloneRestoreAnalysisTotal_.Init(analysisType_, PAGE_SIZE, mediaRdb_, mediaLibraryRdb_);
    int32_t totalNumber = cloneRestoreAnalysisTotal_.GetTotalNumber();
    for (int32_t offset = 0; offset < totalNumber; offset += PAGE_SIZE) {
        RestoreBatch(photoInfoMap);
    }
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    restoreTimeCost_ = end - start;
    ReportRestoreTask();
    MEDIA_INFO_LOG("TimeCost: Restore: %{public}" PRId64, end - start);
}

void CloneRestoreGeo::GetMaxIds()
{
    maxId_ = BackupDatabaseUtils::QueryMaxId(mediaLibraryRdb_, GEO_KNOWLEDGE_TABLE, "rowid");
}

void CloneRestoreGeo::RestoreBatch(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    int64_t startGet = MediaFileUtils::UTCTimeMilliSeconds();
    cloneRestoreAnalysisTotal_.GetInfos(photoInfoMap);
    int64_t startRestoreMaps = MediaFileUtils::UTCTimeMilliSeconds();
    RestoreMaps();
    int64_t startUpdate = MediaFileUtils::UTCTimeMilliSeconds();
    cloneRestoreAnalysisTotal_.UpdateDatabase();
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("TimeCost: GetAnalysisTotalInfos: %{public}" PRId64 ", RestoreMaps: %{public}" PRId64
        ", UpdateDatabase: %{public}" PRId64,
        startRestoreMaps - startGet, startUpdate - startRestoreMaps, end - startUpdate);
}

void CloneRestoreGeo::RestoreMaps()
{
    bool cond = (mediaRdb_ == nullptr || mediaLibraryRdb_ == nullptr);
    CHECK_AND_RETURN_LOG(!cond, "rdbStore is nullptr");

    MEDIA_INFO_LOG("restore geo knowledge start.");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    std::vector<GeoCloneInfo> infos;
    GetInfos(infos);
    int64_t startInsert = MediaFileUtils::UTCTimeMilliSeconds();
    InsertIntoTable(infos);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("TimeCost: GetInfos: %{public}" PRId64 ", InsertIntoTable: %{public}" PRId64,
        startInsert - start, end - startInsert);
    MEDIA_INFO_LOG("restore geo knowledge end.");
}

void CloneRestoreGeo::GetInfos(std::vector<GeoCloneInfo> &infos)
{
    std::unordered_map<std::string, std::string> columns;
    columns[FILE_ID] = INTEGER;
    bool hasRequiredColumns = CheckTableColumns(GEO_KNOWLEDGE_TABLE, columns);
    if (!hasRequiredColumns) {
        MEDIA_ERR_LOG("The tab_analysis_geo_knowledge does not contain the required columns.");
        ErrorInfo errorInfo(RestoreError::TABLE_LACK_OF_COLUMN, static_cast<int32_t>(columns.size()),
            "", "The tab_analysis_geo_knowledge does not contain file_id");
        UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
        return;
    }

    std::stringstream querySql;
    std::string placeHolders;
    std::vector<NativeRdb::ValueObject> params;
    cloneRestoreAnalysisTotal_.SetPlaceHoldersAndParamsByFileIdOld(placeHolders, params);
    querySql << "SELECT * FROM " + GEO_KNOWLEDGE_TABLE + " WHERE " + FILE_ID + " IN (" << placeHolders << ")";

    auto resultSet = BackupDatabaseUtils::QuerySql(mediaRdb_, querySql.str(), params);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Query resultSql is null.");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        GeoCloneInfo info;
        GetInfo(info, resultSet);
        infos.emplace_back(info);
    }
    resultSet->Close();
    MEDIA_INFO_LOG("query tab_analysis_geo_knowledge nums: %{public}zu", infos.size());
}

void CloneRestoreGeo::DeduplicateInfos(std::vector<GeoCloneInfo> &infos)
{
    CHECK_AND_RETURN(!infos.empty());
    std::unordered_set<int32_t> existingFileIds = GetExistingFileIds(GEO_KNOWLEDGE_TABLE);
    RemoveDuplicateInfos(infos, existingFileIds);
    MEDIA_INFO_LOG("existing: %{public}zu, after deduplicate: %{public}zu", existingFileIds.size(), infos.size());
}

std::unordered_set<int32_t> CloneRestoreGeo::GetExistingFileIds(const std::string &tableName)
{
    std::unordered_set<int32_t> existingFileIds;
    std::stringstream querySql;
    std::string placeHolders;
    std::vector<NativeRdb::ValueObject> params;
    cloneRestoreAnalysisTotal_.SetPlaceHoldersAndParamsByFileIdNew(placeHolders, params);
    querySql << "SELECT file_id FROM " + tableName + " WHERE " + FILE_ID + " IN (" << placeHolders << ")";

    auto resultSet = BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, querySql.str(), params);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, existingFileIds, "Query resultSql is null.");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = GetInt32Val("file_id", resultSet);
        existingFileIds.insert(fileId);
    }
    resultSet->Close();
    return existingFileIds;
}

void CloneRestoreGeo::RemoveDuplicateInfos(std::vector<GeoCloneInfo> &infos,
    const std::unordered_set<int32_t> &existingFileIds)
{
    infos.erase(std::remove_if(infos.begin(), infos.end(), [&](GeoCloneInfo &info) {
        if (!info.fileIdOld.has_value()) {
            return true;
        }

        size_t index = cloneRestoreAnalysisTotal_.FindIndexByFileIdOld(info.fileIdOld.value());
        if (index == std::string::npos) {
            return true;
        }

        int32_t fileIdNew = cloneRestoreAnalysisTotal_.GetFileIdNewByIndex(index);
        info.fileIdNew = fileIdNew;
        if (existingFileIds.count(fileIdNew) == 0) {
            return false;
        }
        cloneRestoreAnalysisTotal_.UpdateRestoreStatusAsDuplicateByIndex(index);
        duplicateCnt_++;
        return true;
    }),
        infos.end());
}

void CloneRestoreGeo::InsertIntoTable(std::vector<GeoCloneInfo> &infos)
{
    DeduplicateInfos(infos);
    CHECK_AND_RETURN(!infos.empty());

    std::unordered_set<std::string> intersection = GetCommonColumns(GEO_KNOWLEDGE_TABLE);
    int64_t startInsert = MediaFileUtils::UTCTimeMilliSeconds();
    size_t offset = 0;
    do {
        std::vector<NativeRdb::ValuesBucket> values;
        for (size_t index = 0; index < PAGE_SIZE && index + offset < infos.size(); index++) {
            CHECK_AND_CONTINUE(infos[index + offset].fileIdNew.has_value());
            NativeRdb::ValuesBucket value;
            GetMapInsertValue(value, infos[index + offset], intersection);
            values.emplace_back(value);
        }
        MEDIA_INFO_LOG("Insert into geo_knowledge values size: %{public}zu", values.size());
        int64_t rowNum = 0;
        int32_t errCode = BatchInsertWithRetry(GEO_KNOWLEDGE_TABLE, values, rowNum);
        if (errCode != E_OK || rowNum != static_cast<int64_t>(values.size())) {
            int64_t failNums = static_cast<int64_t>(values.size()) - rowNum;
            MEDIA_ERR_LOG("Insert into geo_knowledge fail, num: %{public}" PRId64, failNums);
            ErrorInfo errorInfo(RestoreError::INSERT_FAILED, static_cast<int32_t>(values.size()),
                "errCode: " + std::to_string(errCode), "Insert into geo_knowledge fail");
            UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
            cloneRestoreAnalysisTotal_.UpdateRestoreStatusAsFailed();
            failedCnt_ += failNums;
        }
        offset += PAGE_SIZE;
        successCnt_ += rowNum;
    } while (offset < infos.size());
}

void CloneRestoreGeo::GetInfo(GeoCloneInfo &info, std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
    GetGeoInfoFromResultSet(info, resultSet);
}

void CloneRestoreGeo::GetMapInsertValue(NativeRdb::ValuesBucket &value, GeoCloneInfo info,
    const std::unordered_set<std::string> &intersection)
{
    GetGeoInsertValue(value, info, intersection);
}

bool CloneRestoreGeo::CheckTableColumns(const std::string& tableName,
    std::unordered_map<std::string, std::string>& columns)
{
    return CloneRestoreGeoBase::CheckTableColumns(tableName, columns, mediaRdb_);
}

std::unordered_set<std::string> CloneRestoreGeo::GetCommonColumns(const string &tableName)
{
    return CloneRestoreGeoBase::GetCommonColumns(tableName, mediaRdb_, mediaLibraryRdb_);
}

int32_t CloneRestoreGeo::BatchInsertWithRetry(const std::string &tableName,
    std::vector<NativeRdb::ValuesBucket> &values, int64_t &rowNum)
{
    return CloneRestoreGeoBase::BatchInsertWithRetry(tableName, values, rowNum, mediaLibraryRdb_);
}

void CloneRestoreGeo::ReportRestoreTask()
{
    ReportRestoreTaskOfTotal();
    ReportRestoreTaskofData();
}

void CloneRestoreGeo::ReportRestoreTaskOfTotal()
{
    RestoreTaskInfo info;
    cloneRestoreAnalysisTotal_.SetRestoreTaskInfo(info);
    info.type = "CLONE_RESTORE_GEO_TOTAL";
    info.errorCode = std::to_string(GEO_STATUS_SUCCESS);
    info.errorInfo = "timeCost: " + std::to_string(restoreTimeCost_);
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).Report(info);
}

void CloneRestoreGeo::ReportRestoreTaskofData()
{
    RestoreTaskInfo info;
    info.type = "CLONE_RESTORE_GEO_DATA";
    info.errorCode = std::to_string(GEO_STATUS_SUCCESS);
    info.errorInfo = "max_id: " + std::to_string(maxId_);
    info.successCount = successCnt_;
    info.failedCount = failedCnt_;
    info.duplicateCount = duplicateCnt_;
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).Report(info);
}
} // namespace OHOS::Media
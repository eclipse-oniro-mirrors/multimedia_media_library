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
#include "clone_restore_analysis_data.h"

#include "backup_database_utils.h"
#include "media_backup_report_data_type.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "result_set_utils.h"
#include "upgrade_restore_task_report.h"

namespace OHOS::Media {
const int32_t PAGE_SIZE = 200;
const std::string FILE_ID = "file_id";
const int32_t ANALYSIS_STATUS_SUCCESS = 1;
static const uint8_t BINARY_FEATURE_END_FLAG = 0x01;
const unordered_map<string, ResultSetDataType> COLUMN_TYPE_MAP = {
    { "INT", ResultSetDataType::TYPE_INT32 },
    { "INTEGER", ResultSetDataType::TYPE_INT32 },
    { "BIGINT", ResultSetDataType::TYPE_INT64 },
    { "DOUBLE", ResultSetDataType::TYPE_DOUBLE },
    { "REAL", ResultSetDataType::TYPE_DOUBLE },
    { "TEXT", ResultSetDataType::TYPE_STRING },
    { "BLOB", ResultSetDataType::TYPE_BLOB },
};

void CloneRestoreAnalysisData::Init(int32_t sceneCode, const std::string &taskId,
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb,
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, const std::string &totalTableName)
{
    sceneCode_ = sceneCode;
    taskId_ = taskId;
    mediaRdb_ = mediaRdb;
    mediaLibraryRdb_ = mediaLibraryRdb;
    totalTableName_ = totalTableName;
}

std::unordered_map<std::string, std::string> CloneRestoreAnalysisData::GetTableCommonColumns(
    const std::unordered_set<std::string> &excludedColumns)
{
    std::unordered_map<std::string, std::string> srcColumnInfoMap =
        BackupDatabaseUtils::GetColumnInfoMap(mediaRdb_, table_);
    std::unordered_map<std::string, std::string> dstColumnInfoMap =
        BackupDatabaseUtils::GetColumnInfoMap(mediaLibraryRdb_, table_);
    std::unordered_map<std::string, std::string> result;
    for (auto it = dstColumnInfoMap.begin(); it != dstColumnInfoMap.end(); ++it) {
        bool cond = (srcColumnInfoMap.find(it->first) != srcColumnInfoMap.end() &&
            excludedColumns.find(it->first) == excludedColumns.end());
        CHECK_AND_EXECUTE(!cond, result[it->first] = it->second);
    }
    return result;
}


template<typename Key, typename Value>
Value GetValueFromMap(const unordered_map<Key, Value> &map, const Key &key, const Value &defaultValue = Value())
{
    auto it = map.find(key);
    CHECK_AND_RETURN_RET(it != map.end(), defaultValue);
    return it->second;
}

void CloneRestoreAnalysisData::GetValFromResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    AnalysisDataInfo &info, const std::string &columnName, const std::string &columnType)
{
    int32_t columnIndex = 0;
    int32_t errCode = resultSet->GetColumnIndex(columnName, columnIndex);
    CHECK_AND_RETURN_LOG(errCode == 0, "Get column index errCode: %{public}d", errCode);
    bool isNull = false;
    errCode = resultSet->IsColumnNull(columnIndex, isNull);
    if (errCode || isNull) {
        return;
    }
    ResultSetDataType dataType = GetValueFromMap(COLUMN_TYPE_MAP, columnType, ResultSetDataType::TYPE_NULL);
    switch (dataType) {
        case ResultSetDataType::TYPE_INT32: {
            int32_t int32Val;
            if (resultSet->GetInt(columnIndex, int32Val) == E_OK) {
                info.columnValMap[columnName] = int32Val;
            }
            break;
        }
        case ResultSetDataType::TYPE_INT64: {
            int64_t int64Val;
            if (resultSet->GetLong(columnIndex, int64Val) == E_OK) {
                info.columnValMap[columnName] = int64Val;
            }
            break;
        }
        case ResultSetDataType::TYPE_DOUBLE: {
            double doubleVal;
            if (resultSet->GetDouble(columnIndex, doubleVal) == E_OK) {
                info.columnValMap[columnName] = doubleVal;
            }
            break;
        }
        case ResultSetDataType::TYPE_BLOB: {
            std::vector<uint8_t> blobVal;
            if (resultSet->GetBlob(columnIndex, blobVal) == E_OK) {
                info.columnValMap[columnName] = blobVal;
            }
            break;
        }
        case ResultSetDataType::TYPE_STRING: {
            std::vector<uint8_t> blobVal;
            if (resultSet->GetBlob(columnIndex, blobVal) == E_OK) {
                if (!blobVal.empty() && blobVal.back() == BINARY_FEATURE_END_FLAG) {
                    info.columnValMap[columnName] = blobVal;
                } else {
                    std::string strVal;
                    resultSet->GetString(columnIndex, strVal);
                    info.columnValMap[columnName] = strVal;
                }
            }
            break;
        }
        default:
            MEDIA_ERR_LOG("No such column type: %{public}s", columnType.c_str());
    }
}

void CloneRestoreAnalysisData::GetAnalysisDataRowInfo(AnalysisDataInfo &info,
    const std::shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    info.fileId = GetInt32Val(FILE_ID, resultSet);
    for (auto it = tableCommonColumns_.begin(); it != tableCommonColumns_.end(); ++it) {
        std::string columnName = it->first;
        std::string columnType = it->second;
        GetValFromResultSet(resultSet, info, columnName, columnType);
    }
}

void CloneRestoreAnalysisData::GetAnalysisDataInfo()
{
    CHECK_AND_RETURN_LOG(mediaRdb_ != nullptr, "rdbStore is nullptr");
    std::stringstream querySql;
    std::string placeHolders;
    std::vector<NativeRdb::ValueObject> params;
    cloneRestoreAnalysisTotal_.SetPlaceHoldersAndParamsByFileIdOld(placeHolders, params);
    querySql << "SELECT * FROM " + table_ + " WHERE " + FILE_ID + " IN (" << placeHolders << ")";
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaRdb_, querySql.str(), params);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Query resultSql is null.");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        AnalysisDataInfo info;
        GetAnalysisDataRowInfo(info, resultSet);
        analysisDataInfos_.emplace_back(info);
    }
    resultSet->Close();
    MEDIA_INFO_LOG("query %{public}s nums: %{public}zu", table_.c_str(), analysisDataInfos_.size());
}

void CloneRestoreAnalysisData::PrepareCommonColumnVal(NativeRdb::ValuesBucket &value, const std::string &columnName,
    const std::string &columnType,
    const std::variant<int32_t, int64_t, double, std::string, std::vector<uint8_t>> &columnVal)
{
    ResultSetDataType dataType = GetValueFromMap(COLUMN_TYPE_MAP, columnType, ResultSetDataType::TYPE_NULL);
    switch (dataType) {
        case ResultSetDataType::TYPE_INT32: {
            value.PutInt(columnName, get<int32_t>(columnVal));
            break;
        }
        case ResultSetDataType::TYPE_INT64: {
            value.PutLong(columnName, get<int64_t>(columnVal));
            break;
        }
        case ResultSetDataType::TYPE_DOUBLE: {
            value.PutDouble(columnName, get<double>(columnVal));
            break;
        }
        case ResultSetDataType::TYPE_BLOB: {
            value.PutBlob(columnName, get<std::vector<uint8_t>>(columnVal));
            break;
        }
        case ResultSetDataType::TYPE_STRING: {
            if (std::holds_alternative<std::string>(columnVal)) {
                value.PutString(columnName, get<std::string>(columnVal));
            } else {
                value.PutBlob(columnName, get<std::vector<uint8_t>>(columnVal));
            }
            break;
        }
        default:
            MEDIA_ERR_LOG("No such column type: %{public}s", columnType.c_str());
    }
}

void CloneRestoreAnalysisData::GetAnalysisDataInsertValue(NativeRdb::ValuesBucket &value,
    const AnalysisDataInfo &info)
{
    for (auto it = tableCommonColumns_.begin(); it != tableCommonColumns_.end(); ++it) {
        std::string columnName = it->first;
        std::string columnType = it->second;
        auto columnIndex = info.columnValMap.find(columnName);
        if (columnIndex != info.columnValMap.end()) {
            PrepareCommonColumnVal(value, columnName, columnType, columnIndex->second);
        }
    }
    value.PutInt(MEDIA_DATA_DB_ID, info.fileId);
}

int32_t CloneRestoreAnalysisData::BatchInsertWithRetry(const std::string &tableName,
    std::vector<NativeRdb::ValuesBucket> &values, int64_t &rowNum)
{
    if (values.empty()) {
        return 0;
    }

    int32_t errCode = E_ERR;
    TransactionOperations trans{ __func__ };
    trans.SetBackupRdbStore(mediaLibraryRdb_);
    std::function<int(void)> func = [&]()->int {
        errCode = trans.BatchInsert(rowNum, tableName, values);
        CHECK_AND_PRINT_LOG(errCode == E_OK,
            "InsertSql failed, errCode: %{public}d, rowNum: %{public}ld.", errCode, (long)rowNum);
        return errCode;
    };
    errCode = trans.RetryTrans(func, true);
    CHECK_AND_PRINT_LOG(errCode == E_OK, "BatchInsertWithRetry: tans finish fail!, ret:%{public}d", errCode);
    return errCode;
}

void CloneRestoreAnalysisData::RemoveDuplicateInfos(const std::unordered_set<int32_t> &existingFileIds)
{
    analysisDataInfos_.erase(std::remove_if(analysisDataInfos_.begin(), analysisDataInfos_.end(),
        [&](AnalysisDataInfo &info) {
        size_t index = cloneRestoreAnalysisTotal_.FindIndexByFileIdOld(info.fileId);
        if (index == std::string::npos) {
            return true;
        }

        int32_t fileIdNew = cloneRestoreAnalysisTotal_.GetFileIdNewByIndex(index);
        info.fileId = fileIdNew;
        if (existingFileIds.count(fileIdNew) == 0) {
            return false;
        }
        cloneRestoreAnalysisTotal_.UpdateRestoreStatusAsDuplicateByIndex(index);
        duplicateCnt_++;
        return true;
    }),
        analysisDataInfos_.end());
}

std::unordered_set<int32_t> CloneRestoreAnalysisData::GetExistingFileIds()
{
    std::unordered_set<int32_t> existingFileIds;
    std::stringstream querySql;
    std::string placeHolders;
    std::vector<NativeRdb::ValueObject> params;
    cloneRestoreAnalysisTotal_.SetPlaceHoldersAndParamsByFileIdNew(placeHolders, params);
    querySql << "SELECT file_id FROM " + table_ + " WHERE " + FILE_ID + " IN (" << placeHolders << ")";
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, querySql.str(), params);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, existingFileIds, "Query resultSql is null.");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = GetInt32Val(FILE_ID, resultSet);
        existingFileIds.insert(fileId);
    }
    resultSet->Close();
    return existingFileIds;
}

void CloneRestoreAnalysisData::DeleteDuplicateInfos()
{
    CHECK_AND_RETURN(!analysisDataInfos_.empty());
    std::unordered_set<int32_t> existingFileIds = GetExistingFileIds();
    RemoveDuplicateInfos(existingFileIds);
}

void CloneRestoreAnalysisData::InsertIntoAnalysisTable()
{
    DeleteDuplicateInfos();
    CHECK_AND_RETURN(!analysisDataInfos_.empty());
    size_t offset = 0;
    do {
        std::vector<NativeRdb::ValuesBucket> values;
        for (size_t index = 0; index < PAGE_SIZE && index + offset < analysisDataInfos_.size(); index++) {
            NativeRdb::ValuesBucket value;
            GetAnalysisDataInsertValue(value, analysisDataInfos_[index + offset]);
            values.emplace_back(value);
        }
        int64_t rowNum = 0;
        int32_t errCode = BatchInsertWithRetry(table_, values, rowNum);
        if (errCode != E_OK || rowNum != static_cast<int64_t>(values.size())) {
            int64_t failNums = static_cast<int64_t>(values.size()) - rowNum;
            MEDIA_ERR_LOG("insert into %{public}s fail, num: %{public}" PRId64, table_.c_str(), failNums);
            ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(errCode),
                "insert into " + table_ + "fail, num:" + std::to_string(failNums));
            failCnt_ += failNums;
            cloneRestoreAnalysisTotal_.UpdateRestoreStatusAsFailed();
            UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
        }
        offset += PAGE_SIZE;
        successCnt_ += rowNum;
    } while (offset < analysisDataInfos_.size());
}

void CloneRestoreAnalysisData::RestoreAnalysisDataMaps()
{
    bool cond = (mediaRdb_ == nullptr || mediaLibraryRdb_ == nullptr);
    CHECK_AND_RETURN_LOG(!cond, "rdbStore is nullptr");

    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    analysisDataInfos_.clear();
    GetAnalysisDataInfo();
    int64_t startInsert = MediaFileUtils::UTCTimeMilliSeconds();
    InsertIntoAnalysisTable();
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("TimeCost: Clone analysis table: %{public}s,"
        "GetInfos: %{public}" PRId64 ", InsertIntoTable: %{public}" PRId64,
        table_.c_str(), startInsert - start, end - startInsert);
}

void CloneRestoreAnalysisData::AnalysisDataRestoreBatch()
{
    int64_t startGet = MediaFileUtils::UTCTimeMilliSeconds();
    cloneRestoreAnalysisTotal_.GetInfos(photoInfoMap_);
    int64_t startRestoreMaps = MediaFileUtils::UTCTimeMilliSeconds();
    RestoreAnalysisDataMaps();
    int64_t startUpdate = MediaFileUtils::UTCTimeMilliSeconds();
    cloneRestoreAnalysisTotal_.UpdateDatabase();
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("TimeCost: Clone analysis table: %{public}s, GetAnalysisTotalInfos: %{public}" PRId64
    ", RestoreMaps: %{public}" PRId64", UpdateDatabase: %{public}" PRId64,
    table_.c_str(), startRestoreMaps - startGet, startUpdate - startRestoreMaps, end - startUpdate);
}

void CloneRestoreAnalysisData::ReportRestoreTaskOfTotal()
{
    RestoreTaskInfo info;
    cloneRestoreAnalysisTotal_.SetRestoreTaskInfo(info);
    info.type = "CLONE_RESTORE_" + ToUpper(analysisType_) +"_TOTAL";
    info.errorCode = std::to_string(ANALYSIS_STATUS_SUCCESS);
    info.errorInfo = "timeCost: " + std::to_string(restoreTimeCost_);
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).Report(info);
}

void CloneRestoreAnalysisData::ReportRestoreTaskofData()
{
    RestoreTaskInfo info;
    info.type = "CLONE_RESTORE_" + ToUpper(analysisType_) +"_DATA";
    info.errorCode = std::to_string(ANALYSIS_STATUS_SUCCESS);
    info.errorInfo = "max_id: " + std::to_string(maxId_);
    info.successCount = successCnt_;
    info.failedCount = failCnt_;
    info.duplicateCount = duplicateCnt_;
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).Report(info);
}

void CloneRestoreAnalysisData::ReportAnalysisTableRestoreTask()
{
    ReportRestoreTaskOfTotal();
    ReportRestoreTaskofData();
}

void CloneRestoreAnalysisData::GetMaxIds()
{
    maxId_ = BackupDatabaseUtils::QueryMaxId(mediaLibraryRdb_, table_, "rowid");
}

void CloneRestoreAnalysisData::CloneAnalysisData(const std::string &table, const std::string &type,
    const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap, const std::unordered_set<std::string> &excludedColumns)
{
    totalCnt_ = 0;
    successCnt_ = 0;
    failCnt_ = 0;
    duplicateCnt_ = 0;
    table_ = table;
    analysisType_ = type;
    photoInfoMap_ = photoInfoMap;

    CloneRestoreAnalysisTotal cloneRestoreAnalysisTotal;
    cloneRestoreAnalysisTotal.Init(analysisType_, PAGE_SIZE, mediaRdb_, mediaLibraryRdb_, totalTableName_);
    cloneRestoreAnalysisTotal_ = cloneRestoreAnalysisTotal;
    
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    tableCommonColumns_ = GetTableCommonColumns(excludedColumns);
    int32_t totalNumber = cloneRestoreAnalysisTotal_.GetTotalNumber();
    for (int32_t offset = 0; offset < totalNumber; offset += PAGE_SIZE) {
        AnalysisDataRestoreBatch();
    }
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    restoreTimeCost_ = end - start;
    MEDIA_INFO_LOG("TimeCost: clone analysis table :%{public}s, number:%{public}d, cost time: %{public}" PRId64,
       table_.c_str(), totalNumber, end - start);
    ReportAnalysisTableRestoreTask();
}
}
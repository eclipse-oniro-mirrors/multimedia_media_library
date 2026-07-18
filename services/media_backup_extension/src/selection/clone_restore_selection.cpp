/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaLibraryCloneRestoreSelection"

#include "media_log.h"
#include "clone_restore_selection.h"
#include "backup_database_utils.h"
#include "backup_dfx_utils.h"
#include "media_file_utils.h"
#include "upgrade_restore_task_report.h"
#include "clone_restore_analysis_total.h"
#include "media_column.h"
#include "vision_column.h"
#include "medialibrary_type_const.h"
#include "backup_const_column.h"
#include "medialibrary_data_manager_utils.h"

using namespace std;
namespace OHOS::Media {
const int32_t PAGE_SIZE = 200;

// 超时控制配置
const int64_t THRESHOLD_DATA_SIZE = 30000;   // 3万条数据阈值
const int64_t THRESHOLD_DATA_TIME = 600000;  // 10分钟 (不超过3万条时的基线)
const int64_t DEFAULT_FAULT_TIME = 0;
const int64_t BASIC_NUMBER = 10000;                      // 1万条
const int64_t SUPPORT_NUMBER = 9999;                     // 余量
const int64_t SINGLE_OVER_THRESHOLD_DATA_TIME = 216000;  // 216秒 (每1万条数据)
const std::string SELECTION_TABLE = "tab_analysis_selection";
const std::string ATOM_EVENT_TABLE = "tab_analysis_atom_event";

void CloneRestoreSelection::Init(int32_t sceneCode, const std::string &taskId,
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, std::shared_ptr<NativeRdb::RdbStore> mediaRdb,
    const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap, bool isCloudRestoreSatisfied)
{
    MEDIA_INFO_LOG("CloneRestoreSelection Init");
    this->sceneCode_ = sceneCode;
    this->taskId_ = taskId;
    this->mediaRdb_ = mediaRdb;
    this->mediaLibraryRdb_ = mediaLibraryRdb;
    this->photoInfoMap_ = photoInfoMap;
    this->isCloudRestoreSatisfied_ = isCloudRestoreSatisfied;
}

bool CloneRestoreSelection::RefreshTotalNumber()
{
    CHECK_AND_RETURN_RET_LOG(mediaRdb_ != nullptr && mediaLibraryRdb_ != nullptr, true, "rdbStore is nullptr.");
    std::string querySql = "SELECT count(1) AS count FROM " + SELECTION_TABLE;
    std::string whereClause;
    AppendExtraWhereClause(whereClause);
    if (!whereClause.empty()) {
        querySql += " WHERE " + whereClause;
    }
    totalSelectionNumber_ = BackupDatabaseUtils::QueryInt(mediaRdb_, querySql, "count");
    MEDIA_INFO_LOG("QuerySelection totalNumber = %{public}d", totalSelectionNumber_);
    std::string queryEventSql = "SELECT count(1) AS count FROM " + ATOM_EVENT_TABLE;

    totalAtomEventNumber_ = BackupDatabaseUtils::QueryInt(mediaRdb_, queryEventSql, "count");
    MEDIA_INFO_LOG("QueryAtomEvent totalNumber = %{public}d", totalAtomEventNumber_);
    return totalSelectionNumber_ <= 0 && totalAtomEventNumber_ <= 0;
}

bool CloneRestoreSelection::Preprocess()
{
    MEDIA_INFO_LOG("Preprocess");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    RefreshTotalNumber();
    CHECK_AND_EXECUTE(!(totalSelectionNumber_ > 0 || totalAtomEventNumber_ > 0), DeleteExistingSelectionInfos());

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    migrateSelectionTotalTimeCost_ += end - start;
    return totalSelectionNumber_ <= 0 && totalAtomEventNumber_ <= 0;
}

void CloneRestoreSelection::DeleteExistingSelectionInfos()
{
    MEDIA_INFO_LOG("DeleteExistingSelectionInfos");
    ClearTotalTableSelectionFields();
    DeleteExistingSelectionTable();
    DeleteExistingAtomEventTable();
}

void CloneRestoreSelection::ClearTotalTableSelectionFields()
{
    MEDIA_INFO_LOG("ClearTotalTableSelectionFields");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    // 精选视图是所有照片为范围的，清理 total 表中所有照片的 selection 字段
    std::string photoQueryWhereClause;
    if (isCloudRestoreSatisfied_) {
        photoQueryWhereClause = PhotoColumn::PHOTO_POSITION + " IN (1, 2, 3) AND ";
    } else {
        photoQueryWhereClause = PhotoColumn::PHOTO_POSITION + " IN (1, 3) AND ";
    }
    photoQueryWhereClause += PhotoColumn::PHOTO_SYNC_STATUS + " = " +
                             std::to_string(static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE)) + " AND " +
                             PhotoColumn::PHOTO_CLEAN_FLAG + " = " +
                             std::to_string(static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN)) + " AND " +
                             MediaColumn::MEDIA_TIME_PENDING + " = 0" + " AND " + PhotoColumn::PHOTO_IS_TEMP + " = 0";
    std::string updateSql = "UPDATE " + VISION_TOTAL_TABLE +
                            " SET selection = 0 WHERE file_id IN (SELECT file_id FROM " + PhotoColumn::PHOTOS_TABLE +
                            " WHERE " + photoQueryWhereClause + ")";
    int32_t totalRet = BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, updateSql);
    MEDIA_INFO_LOG("Update TableAnalysisTotal selection, ret: %{public}d", totalRet);

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("ClearTotalTableSelectionFields cost %{public}lld", (long long)(end - start));
}

void CloneRestoreSelection::DeleteExistingSelectionTable()
{
    MEDIA_INFO_LOG("DeleteExistingSelectionTable");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    std::string deleteSql = "DELETE FROM " + SELECTION_TABLE;
    BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, deleteSql);

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("DeleteExistingSelectionTable cost %{public}lld", (long long)(end - start));
}

void CloneRestoreSelection::DeleteExistingAtomEventTable()
{
    MEDIA_INFO_LOG("DeleteExistingAtomEventTable");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    std::string deleteSql = "DELETE FROM " + ATOM_EVENT_TABLE;
    BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, deleteSql);

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("DeleteExistingAtomEventTable cost %{public}lld", (long long)(end - start));
}

void CloneRestoreSelection::AppendExtraWhereClause(std::string &whereClause)
{
    std::string photoQueryWhereClause;
    if (isCloudRestoreSatisfied_) {
        photoQueryWhereClause = PhotoColumn::PHOTO_POSITION + " IN (1, 2, 3) AND ";
    } else {
        photoQueryWhereClause = PhotoColumn::PHOTO_POSITION + " IN (1, 3) AND ";
    }
    photoQueryWhereClause += PhotoColumn::PHOTO_SYNC_STATUS + " = " +
                             std::to_string(static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE)) + " AND " +
                             PhotoColumn::PHOTO_CLEAN_FLAG + " = " +
                             std::to_string(static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN)) + " AND " +
                             MediaColumn::MEDIA_TIME_PENDING + " = 0" + " AND " + PhotoColumn::PHOTO_IS_TEMP + " = 0";
    std::string selectionQueryWhereClause = "EXISTS (SELECT " + MediaColumn::MEDIA_ID + " FROM " +
                                            PhotoColumn::PHOTOS_TABLE + " WHERE " + MediaColumn::MEDIA_ID + " = " +
                                            SELECTION_TABLE + ".file_id";
    selectionQueryWhereClause += " AND " + photoQueryWhereClause + ")";
    whereClause += whereClause.empty() ? "" : " AND ";
    whereClause += selectionQueryWhereClause;
}

int64_t CloneRestoreSelection::GetShouldEndTime()
{
    CHECK_AND_RETURN_RET_LOG(!taskId_.empty() && MediaLibraryDataManagerUtils::IsNumber(taskId_),
        DEFAULT_FAULT_TIME,
        "taskId: %{public}s invalid",
        taskId_.c_str());
    int64_t backupStartTime = std::stoll(taskId_) * 1000;
    int64_t dataSize = static_cast<int64_t>(photoInfoMap_.size());
    MEDIA_INFO_LOG("dataSize: %{public}" PRId64 ", backupStartTime: %{public}" PRId64, dataSize, backupStartTime);
    CHECK_AND_RETURN_RET(dataSize > THRESHOLD_DATA_SIZE, backupStartTime + THRESHOLD_DATA_TIME);
    return backupStartTime + (dataSize + SUPPORT_NUMBER) / BASIC_NUMBER * SINGLE_OVER_THRESHOLD_DATA_TIME;
}

void CloneRestoreSelection::Restore()
{
    MEDIA_INFO_LOG("Start Restore");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t shouldEndTime = GetShouldEndTime();
    CHECK_AND_RETURN_LOG(start <= shouldEndTime, "over shouldEndTime, skip Restore");

    CHECK_AND_RETURN_LOG(mediaRdb_ != nullptr && mediaLibraryRdb_ != nullptr, "rdbStore is nullptr.");
    RestoreSelectionData();
    RestoreAtomEventData();
    RestoreAnalysisTotalSelectionStatus();

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    migrateSelectionTotalTimeCost_ += end - start;
}

void CloneRestoreSelection::RestoreAnalysisTotalSelectionStatus()
{
    MEDIA_INFO_LOG("RestoreAnalysisTotalSelectionStatus");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    CloneRestoreAnalysisTotal cloneRestoreAnalysisTotal;
    cloneRestoreAnalysisTotal.Init("selection", PAGE_SIZE, mediaRdb_, mediaLibraryRdb_);
    int32_t totalNumber = cloneRestoreAnalysisTotal.GetTotalNumber();
    for (int32_t offset = 0; offset < totalNumber; offset += PAGE_SIZE) {
        cloneRestoreAnalysisTotal.GetInfos(photoInfoMap_);
        cloneRestoreAnalysisTotal.UpdateDatabase();
    }

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("TimeCost: UpdateDatabase: %{public}" PRId64, end - start);
}

void CloneRestoreSelection::RestoreSelectionData()
{
    MEDIA_INFO_LOG("RestoreSelectionData");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t shouldEndTime = GetShouldEndTime();

    if (totalSelectionNumber_ == 0) {
        MEDIA_INFO_LOG("No selection data to restore");
        return;
    }
    MEDIA_INFO_LOG("RestoreSelectionData: total records to restore = %{public}d", totalSelectionNumber_);

    int32_t lastFileId = 0;
    int32_t batchCount = 0;
    while (true) {
        int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
        CHECK_AND_RETURN_LOG(currentTime <= shouldEndTime,
            "over shouldEndTime, batch count: %{public}d, RestoreSelectionData cost: %{public}lld",
            batchCount,
            (long long)(currentTime - start));

        int64_t batchQueryStart = MediaFileUtils::UTCTimeMilliSeconds();
        std::vector<SelectionInfo> selectionInfos = QuerySelectionTbl(lastFileId);
        if (selectionInfos.empty()) {
            MEDIA_INFO_LOG("RestoreSelectionData: no more records at batch = %{public}d", batchCount);
            break;
        }
        int64_t batchInsertStart = MediaFileUtils::UTCTimeMilliSeconds();

        BatchInsertSelectionData(selectionInfos);
        int64_t batchEnd = MediaFileUtils::UTCTimeMilliSeconds();
        MEDIA_INFO_LOG("RestoreSelectionData: batch count = %{public}d, inserted = %{public}zu, batch query cost = "
                       "%{public}lld ms, batch insert cost = %{public}lld ms",
            batchCount,
            selectionInfos.size(),
            (long long)(batchInsertStart - batchQueryStart),
            (long long)(batchEnd - batchInsertStart));

        // 更新游标为最后一条记录的file_id
        if (selectionInfos.back().fileId.has_value()) {
            lastFileId = selectionInfos.back().fileId.value();
        }
        batchCount++;

        // 如果返回的记录数少于QUERY_COUNT，说明已经查询完所有数据
        if (selectionInfos.size() < static_cast<size_t>(QUERY_COUNT)) {
            MEDIA_INFO_LOG("RestoreSelectionData: reached end of data at batch = %{public}d", batchCount);
            break;
        }
    }

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG(
        "RestoreSelectionData completed, migrateSelectionNum %{public}llu records, Total time: %{public}lld ms",
        (unsigned long long)migrateSelectionNumber_.load(),
        (long long)(end - start));
}

std::vector<SelectionInfo> CloneRestoreSelection::QuerySelectionTbl(int32_t lastFileId)
{
    std::vector<SelectionInfo> result;
    result.reserve(QUERY_COUNT);
    std::string querySql = "SELECT file_id, month_flag, year_flag, selection_version, event_id "
                           " FROM " +
                           SELECTION_TABLE;
    std::string whereClause;
    AppendExtraWhereClause(whereClause);
    if (!whereClause.empty()) {
        querySql += " WHERE " + whereClause;
        if (lastFileId > 0) {
            querySql += " AND file_id > " + std::to_string(lastFileId);
        }
    } else if (lastFileId > 0) {
        querySql += " WHERE file_id > " + std::to_string(lastFileId);
    }
    querySql += " ORDER BY file_id LIMIT " + std::to_string(QUERY_COUNT);
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSql is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        SelectionInfo info;
        info.fileId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "file_id");
        info.monthFlag = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "month_flag");
        info.yearFlag = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "year_flag");
        info.selectionVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "selection_version");
        info.eventId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "event_id");
        result.emplace_back(info);
    }
    resultSet->Close();
    return result;
}

void CloneRestoreSelection::BatchInsertSelectionData(const std::vector<SelectionInfo> &selectionInfos)
{
    CHECK_AND_RETURN_LOG(mediaLibraryRdb_ != nullptr, "mediaLibraryRdb_ is null");
    CHECK_AND_RETURN_LOG(!selectionInfos.empty(), "selectionInfos are empty");

    std::vector<NativeRdb::ValuesBucket> valuesBuckets;
    for (const auto &info : selectionInfos) {
        SelectionInfo mappedInfo = info;
        if (info.fileId.has_value()) {
            auto it = photoInfoMap_.find(info.fileId.value());
            if (it != photoInfoMap_.end() && it->second.fileIdNew != -1) {
                mappedInfo.fileId = it->second.fileIdNew;
            } else {
                MEDIA_ERR_LOG("Cannot find new file id for old file id: %{public}d", info.fileId.value());
                continue;
            }
        }
        valuesBuckets.push_back(CreateValuesBucketFromSelectionInfo(mappedInfo));
    }
    int64_t rowNum = 0;
    int32_t ret = BatchInsertWithRetry(SELECTION_TABLE, valuesBuckets, rowNum);
    CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to batch insert selection data");
    migrateSelectionNumber_ += static_cast<uint64_t>(rowNum);
}

NativeRdb::ValuesBucket CloneRestoreSelection::CreateValuesBucketFromSelectionInfo(const SelectionInfo &info)
{
    NativeRdb::ValuesBucket values;

    BackupDatabaseUtils::PutIfPresent(values, "file_id", info.fileId);
    BackupDatabaseUtils::PutIfPresent(values, "month_flag", info.monthFlag);
    BackupDatabaseUtils::PutIfPresent(values, "year_flag", info.yearFlag);
    BackupDatabaseUtils::PutIfPresent(values, "selection_version", info.selectionVersion);
    BackupDatabaseUtils::PutIfPresent(values, "event_id", info.eventId);

    return values;
}

void CloneRestoreSelection::RestoreAtomEventData()
{
    MEDIA_INFO_LOG("RestoreAtomEventData");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t shouldEndTime = GetShouldEndTime();

    if (totalAtomEventNumber_ == 0) {
        MEDIA_INFO_LOG("No atom event data to restore");
        return;
    }
    MEDIA_INFO_LOG("RestoreAtomEventData: total records to restore = %{public}d", totalAtomEventNumber_);

    int32_t lastEventId = 0;
    int32_t batchCount = 0;
    while (true) {
        int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
        CHECK_AND_RETURN_LOG(currentTime <= shouldEndTime,
            "over shouldEndTime, batch count: %{public}d, RestoreAtomEventData cost: %{public}lld",
            batchCount,
            (long long)(currentTime - start));

        int64_t batchQueryStart = MediaFileUtils::UTCTimeMilliSeconds();
        std::vector<AtomEventInfo> atomEventInfos = QueryAtomEventTbl(lastEventId);
        if (atomEventInfos.empty()) {
            MEDIA_INFO_LOG("RestoreAtomEventData: no more records at batch = %{public}d", batchCount);
            break;
        }
        int64_t batchInsertStart = MediaFileUtils::UTCTimeMilliSeconds();
        BatchInsertAtomEventData(atomEventInfos);
        int64_t batchEnd = MediaFileUtils::UTCTimeMilliSeconds();
        MEDIA_INFO_LOG("RestoreAtomEventData: batch count = %{public}d, inserted = %{public}zu, batch query cost = "
                       "%{public}lld ms, batch insert cost = %{public}lld ms",
            batchCount,
            atomEventInfos.size(),
            (long long)(batchInsertStart - batchQueryStart),
            (long long)(batchEnd - batchInsertStart));

        // 更新游标为最后一条记录的event_id
        if (atomEventInfos.back().eventId.has_value()) {
            lastEventId = atomEventInfos.back().eventId.value();
        }
        batchCount++;

        // 如果返回的记录数少于QUERY_COUNT，说明已经查询完所有数据
        if (atomEventInfos.size() < static_cast<size_t>(QUERY_COUNT)) {
            MEDIA_INFO_LOG("RestoreAtomEventData: reached end of data at batch = %{public}d", batchCount);
            break;
        }
    }
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG(
        "RestoreAtomEventData completed, migrateAtomEventNum %{public}llu records, Total time: %{public}lld ms",
        (unsigned long long)migrateAtomEventNumber_.load(),
        (long long)(end - start));
}

std::vector<AtomEventInfo> CloneRestoreSelection::QueryAtomEventTbl(int32_t lastEventId)
{
    std::vector<AtomEventInfo> result;
    result.reserve(QUERY_COUNT);
    std::string querySql = "SELECT event_id, min_date, max_date, count, date_day, date_month, "
                           "event_type, event_score, event_version, event_status "
                           " FROM " +
                           ATOM_EVENT_TABLE;
    if (lastEventId > 0) {
        querySql += " WHERE event_id > " + std::to_string(lastEventId);
    }
    querySql += " ORDER BY event_id LIMIT " + std::to_string(QUERY_COUNT);

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSql is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        AtomEventInfo info;
        info.eventId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "event_id");
        info.minDate = BackupDatabaseUtils::GetOptionalValue<int64_t>(resultSet, "min_date");
        info.maxDate = BackupDatabaseUtils::GetOptionalValue<int64_t>(resultSet, "max_date");
        info.count = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "count");
        info.dateDay = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "date_day");
        info.dateMonth = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "date_month");
        info.dateType = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "event_type");
        info.eventScore = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "event_score");
        info.eventVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "event_version");
        info.eventStatus = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "event_status");
        result.emplace_back(info);
    }
    resultSet->Close();
    return result;
}

void CloneRestoreSelection::BatchInsertAtomEventData(const std::vector<AtomEventInfo> &atomEventInfos)
{
    CHECK_AND_RETURN_LOG(mediaLibraryRdb_ != nullptr, "mediaLibraryRdb_ is null");
    CHECK_AND_RETURN_LOG(!atomEventInfos.empty(), "atomEventInfos are empty");

    std::vector<NativeRdb::ValuesBucket> valuesBuckets;
    for (const auto &info : atomEventInfos) {
        valuesBuckets.push_back(CreateValuesBucketFromAtomEventInfo(info));
    }
    int64_t rowNum = 0;
    int32_t ret = BatchInsertWithRetry(ATOM_EVENT_TABLE, valuesBuckets, rowNum);
    CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to batch insert atom event data");
    migrateAtomEventNumber_ += static_cast<uint64_t>(rowNum);
}

NativeRdb::ValuesBucket CloneRestoreSelection::CreateValuesBucketFromAtomEventInfo(const AtomEventInfo &info)
{
    NativeRdb::ValuesBucket values;

    BackupDatabaseUtils::PutIfPresent(values, "event_id", info.eventId);
    BackupDatabaseUtils::PutIfPresent(values, "min_date", info.minDate);
    BackupDatabaseUtils::PutIfPresent(values, "max_date", info.maxDate);
    BackupDatabaseUtils::PutIfPresent(values, "count", info.count);
    BackupDatabaseUtils::PutIfPresent(values, "date_day", info.dateDay);
    BackupDatabaseUtils::PutIfPresent(values, "date_month", info.dateMonth);
    BackupDatabaseUtils::PutIfPresent(values, "event_type", info.dateType);
    BackupDatabaseUtils::PutIfPresent(values, "event_score", info.eventScore);
    BackupDatabaseUtils::PutIfPresent(values, "event_version", info.eventVersion);
    BackupDatabaseUtils::PutIfPresent(values, "event_status", info.eventStatus);

    return values;
}

int32_t CloneRestoreSelection::BatchInsertWithRetry(
    const std::string &tableName, std::vector<NativeRdb::ValuesBucket> &values, int64_t &rowNum)
{
    int32_t ret = BackupDatabaseUtils::BatchInsert(mediaLibraryRdb_, tableName, values, rowNum);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("BatchInsert failed, tableName: %{public}s, ret: %{public}d", tableName.c_str(), ret);
        ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(ret), "insert into " + tableName + " fail");
        UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
    }
    return ret;
}
}  // namespace OHOS::Media
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

#include "ai_retouch_clone.h"

#include "backup_database_utils.h"
#include "backup_const.h"
#include "backup_const_column.h"
#include "backup_dfx_utils.h"
#include "backup_file_utils.h"
#include "backup_log_utils.h"
#include "database_report.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_library_db_upgrade.h"
#include "media_log.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "rdb_store.h"
#include "result_set_utils.h"

namespace OHOS::Media {
const int32_t CLONE_QUERY_COUNT = 200;
const int64_t THRESHOLD_DATA_SIZE = 30000;
const int64_t THRESHOLD_DATA_TIME = 600000; // 不超过THRESHOLD_DATA_SIZE，基线为10min
const int64_t DEFAULT_FAULT_TIME = 0;

// 超出THRESHOLD_DATA_SIZE，每1w数据基线为216s
const int64_t BASIC_NUMBER = 10000;
const int64_t SUPPORT_NUMBER = 9999;
const int64_t SINGLE_OVER_THRESHOLD_DATA_TIME = 216000;

AiRetouchClone::AiRetouchClone(
    const std::shared_ptr<NativeRdb::RdbStore>& sourceRdb,
    const std::shared_ptr<NativeRdb::RdbStore>& destRdb,
    const std::unordered_map<int32_t, PhotoInfo>& photoInfoMap,
    const int64_t& maxTotalFileId,
    const std::string& taskId,
    bool isReverse)
    : sourceRdb_(sourceRdb),
      destRdb_(destRdb),
      photoInfoMap_(photoInfoMap),
      maxTotalFileId_(maxTotalFileId),
      taskId_(taskId),
      isReverse_(isReverse)
{
}

void AiRetouchClone::ParseAiRetouchFromResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
    AiRetouchTbl& aiRetouchTbl)
{
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "resultSql is nullptr");
    aiRetouchTbl.fileId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, AI_RETOUCH_COL_FILE_ID);
    aiRetouchTbl.portraitRefine = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        AI_RETOUCH_COL_PORTRAIT_REFINE);
    aiRetouchTbl.passersRemove = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        AI_RETOUCH_COL_PASSERS_REMOVE);
    aiRetouchTbl.reflectiveRemove = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        AI_RETOUCH_COL_REFLECTIVE_REMOVE);
    aiRetouchTbl.moireRemove = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        AI_RETOUCH_COL_MOIRE_REMOVE);
    aiRetouchTbl.magicEmoji = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        AI_RETOUCH_COL_MAGIC_EMOJI);
    aiRetouchTbl.aiRetouchVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        AI_RETOUCH_COL_AI_RETOUCH_VERSION);
    aiRetouchTbl.magicEmojiVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        AI_RETOUCH_COL_MAGIC_EMOJI_VERSION);
    aiRetouchTbl.analysisVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        AI_RETOUCH_COL_ANALYSIS_VERSION);
}

int64_t AiRetouchClone::GetShouldEndTime()
{
    CHECK_AND_RETURN_RET_LOG(!taskId_.empty() && MediaLibraryDataManagerUtils::IsNumber(taskId_),
        DEFAULT_FAULT_TIME, "taskId: %{public}s invalid", taskId_.c_str());
    int64_t backupStartTime = std::stoll(taskId_) * 1000;
    int64_t dataSize = static_cast<int64_t>(photoInfoMap_.size());
    MEDIA_INFO_LOG("dataSize: %{public}" PRId64 ", backupStartTime: %{public}" PRId64, dataSize, backupStartTime);
    CHECK_AND_RETURN_RET(dataSize > THRESHOLD_DATA_SIZE, backupStartTime + THRESHOLD_DATA_TIME);
    // entire clone task should end time
    int64_t taskShouldEndTime = backupStartTime + (dataSize + SUPPORT_NUMBER) / BASIC_NUMBER *
        SINGLE_OVER_THRESHOLD_DATA_TIME;
    return taskShouldEndTime;
}

void AiRetouchClone::CloneAiRetouchInfo()
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t shouldEndTime = GetShouldEndTime();
    CHECK_AND_RETURN_LOG(startTime <= shouldEndTime, "over shouldEndTime, skip CloneAiRetouchInfo");
    std::vector<int32_t> oldFileIds;
    oldFileIds.reserve(photoInfoMap_.size());

    for (const auto& pair : photoInfoMap_) {
        CHECK_AND_CONTINUE(pair.second.fileIdNew > maxTotalFileId_);
        oldFileIds.push_back(pair.first);
    }
    CHECK_AND_RETURN_LOG(!oldFileIds.empty(), "no ai retouch data to clone");
    oldFileIds.shrink_to_fit();

    // check is need to insert tab_analysis_ai_retouch
    std::vector<std::string> aiRetouchCommonColumn = BackupDatabaseUtils::GetCommonColumnInfos(sourceRdb_, destRdb_,
        VISION_AI_RETOUCH_TABLE);
    bool isNeedInsertAiRetouch = aiRetouchCommonColumn.size() > 1;
    std::string aiRetouchInClause = BackupDatabaseUtils::JoinValues<std::string>(aiRetouchCommonColumn, ", ");

    // check is need to update tab_analysis_total
    std::vector<std::string> totalCommonColumn = BackupDatabaseUtils::GetCommonColumnInfos(sourceRdb_, destRdb_,
        VISION_TOTAL_TABLE);
    const std::vector<std::string> INCLUDED_AI_RETOUCH_COLUMNS = {TOTAL_COL_FILE_ID, TOTAL_COL_AI_RETOUCH,
        TOTAL_COL_MAGIC_EMOJI};
    std::vector<std::string> commonAiRetouchColumn = BackupDatabaseUtils::FilterIncludedColumns(totalCommonColumn,
        INCLUDED_AI_RETOUCH_COLUMNS);
    bool isNeedUpdateTotal = commonAiRetouchColumn.size() > 1;
    std::string totalInClause = BackupDatabaseUtils::JoinValues<std::string>(commonAiRetouchColumn, ", ");

    for (size_t i = 0; i < oldFileIds.size(); i += SQL_BATCH_SIZE) {
        int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
        CHECK_AND_RETURN_LOG(currentTime <= shouldEndTime, "over shouldEndTime, last file_id: %{public}zu, "
            "aiRetouch insert: %{public}" PRId64 ", total insert: %{public}u, Total time: %{public}" PRId64 " ms", i,
            aiRetouchNum_, totalAiRetouchNum_, currentTime - startTime);
        auto batch_begin = oldFileIds.begin() + i;
        auto batch_end = ((i + SQL_BATCH_SIZE < oldFileIds.size()) ?
            (oldFileIds.begin() + i + SQL_BATCH_SIZE) : oldFileIds.end());
        std::vector<int32_t> batchOldFileIds(batch_begin, batch_end);
        CHECK_AND_CONTINUE(!batchOldFileIds.empty());
        std::string fileIdOldInClause = "(" + BackupDatabaseUtils::JoinValues<int>(batchOldFileIds, ", ") + ")";
        CHECK_AND_EXECUTE(!isNeedInsertAiRetouch, InsertAiRetouchInBatch(fileIdOldInClause, aiRetouchInClause));
        CHECK_AND_EXECUTE(!isNeedUpdateTotal, UpdateTotalInBatch(fileIdOldInClause, totalInClause));
    }

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("CloneAiRetouchInfo completed, aiRetouchNum %{public}" PRId64 " records, "
        "totalAiRetouchNum %{public}u records, Total time: %{public}" PRId64 " ms", aiRetouchNum_, totalAiRetouchNum_,
        end - startTime);
}

void AiRetouchClone::ReverseCloneAiRetouchInfo()
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();

    if (!QueryAndInsertSourceAiRetouch()) {
        MEDIA_ERR_LOG("ReverseCloneAiRetouchInfo: Failed to query and insert source ai retouch");
        return;
    }

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("ReverseCloneAiRetouchInfo completed, aiRetouchNum %{public}" PRId64 " records, "
        "Total time: %{public}" PRId64 " ms", aiRetouchNum_, end - startTime);
}

void AiRetouchClone::InsertAiRetouchInBatch(const std::string& fileIdClause, const std::string& inClause)
{
    std::vector<AiRetouchTbl> aiRetouchTbls = QueryAiRetouchTbl(fileIdClause, inClause);
    CHECK_AND_RETURN_LOG(!aiRetouchTbls.empty(), "QueryAiRetouchTbl result empty in: %{public}s",
        fileIdClause.c_str());
    std::vector<AiRetouchTbl> processAiRetouches = ProcessAiRetouchTbls(aiRetouchTbls);
    BatchInsertAiRetouch(processAiRetouches);
}

std::vector<AiRetouchTbl> AiRetouchClone::QueryAiRetouchTbl(const std::string& fileIdClause,
    const std::string& inClause)
{
    std::vector<AiRetouchTbl> result;
    std::string querySql =
        "SELECT " + inClause +
        " FROM " + VISION_AI_RETOUCH_TABLE;
    querySql += " WHERE " + AI_RETOUCH_COL_FILE_ID + " IN " + fileIdClause;

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(sourceRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSet is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        AiRetouchTbl aiRetouchTbl;
        ParseAiRetouchFromResultSet(resultSet, aiRetouchTbl);
        result.emplace_back(aiRetouchTbl);
    }

    resultSet->Close();
    return result;
}

std::vector<AiRetouchTbl> AiRetouchClone::ProcessAiRetouchTbls(std::vector<AiRetouchTbl>& aiRetouchTbls)
{
    CHECK_AND_RETURN_RET_LOG(!aiRetouchTbls.empty(), {}, "ai retouch tbl empty");

    std::vector<AiRetouchTbl> aiRetouchNewTbls;
    aiRetouchNewTbls.reserve(aiRetouchTbls.size());

    for (const auto& aiRetouchTbl : aiRetouchTbls) {
        if (aiRetouchTbl.fileId.has_value()) {
            int32_t oldFileId = aiRetouchTbl.fileId.value();
            const auto it = photoInfoMap_.find(oldFileId);
            if (it != photoInfoMap_.end()) {
                AiRetouchTbl updatedTbl = aiRetouchTbl;
                updatedTbl.fileId = it->second.fileIdNew;
                aiRetouchNewTbls.push_back(std::move(updatedTbl));
            } else {
                MEDIA_WARN_LOG("Original fileId %{public}d not found in photoInfoMap_, skipping.", oldFileId);
            }
        }
    }
    return aiRetouchNewTbls;
}

void AiRetouchClone::BatchInsertAiRetouch(const std::vector<AiRetouchTbl>& aiRetouchTbls)
{
    std::vector<NativeRdb::ValuesBucket> valuesBucketsToInsert;
    std::unordered_set<int32_t> fileIdsNewlyInserted;
    valuesBucketsToInsert.reserve(aiRetouchTbls.size());
    for (const auto& aiRetouchTbl : aiRetouchTbls) {
        if (!aiRetouchTbl.fileId.has_value()) {
            MEDIA_WARN_LOG("aiRetouchTbl has no fileId, skipping.");
            continue;
        }
        int32_t currentFileId = aiRetouchTbl.fileId.value();
        valuesBucketsToInsert.push_back(CreateValuesBucketFromAiRetouchTbl(aiRetouchTbl));
        fileIdsNewlyInserted.insert(currentFileId);
    }

    if (valuesBucketsToInsert.empty()) {
        MEDIA_ERR_LOG("No new ai retouch entries to insert after filtering by maxAiRetouchFileId_");
        return;
    }

    int64_t rowNum = 0;
    int32_t ret = BatchInsertWithRetry(VISION_AI_RETOUCH_TABLE, valuesBucketsToInsert, rowNum);
    CHECK_AND_RETURN_LOG(ret == E_OK, "failed to batch insert ai retouch");
    aiRetouchNum_ += rowNum;
}

NativeRdb::ValuesBucket AiRetouchClone::CreateValuesBucketFromAiRetouchTbl(
    const AiRetouchTbl& aiRetouchTbl)
{
    NativeRdb::ValuesBucket values;
    BackupDatabaseUtils::PutIfPresent(values, AI_RETOUCH_COL_FILE_ID, aiRetouchTbl.fileId);
    BackupDatabaseUtils::PutIfPresent(values, AI_RETOUCH_COL_PORTRAIT_REFINE, aiRetouchTbl.portraitRefine);
    BackupDatabaseUtils::PutIfPresent(values, AI_RETOUCH_COL_PASSERS_REMOVE, aiRetouchTbl.passersRemove);
    BackupDatabaseUtils::PutIfPresent(values, AI_RETOUCH_COL_REFLECTIVE_REMOVE, aiRetouchTbl.reflectiveRemove);
    BackupDatabaseUtils::PutIfPresent(values, AI_RETOUCH_COL_MOIRE_REMOVE, aiRetouchTbl.moireRemove);
    BackupDatabaseUtils::PutIfPresent(values, AI_RETOUCH_COL_MAGIC_EMOJI, aiRetouchTbl.magicEmoji);
    BackupDatabaseUtils::PutIfPresent(values, AI_RETOUCH_COL_AI_RETOUCH_VERSION, aiRetouchTbl.aiRetouchVersion);
    BackupDatabaseUtils::PutIfPresent(values, AI_RETOUCH_COL_MAGIC_EMOJI_VERSION, aiRetouchTbl.magicEmojiVersion);
    BackupDatabaseUtils::PutIfPresent(values, AI_RETOUCH_COL_ANALYSIS_VERSION, aiRetouchTbl.analysisVersion);
    return values;
}

int32_t AiRetouchClone::BatchInsertWithRetry(const std::string &tableName,
    std::vector<NativeRdb::ValuesBucket> &values, int64_t &rowNum)
{
    CHECK_AND_RETURN_RET(!values.empty(), E_OK);
    int32_t errCode = E_ERR;
    TransactionOperations trans{ __func__ };
    trans.SetBackupRdbStore(destRdb_);
    std::function<int(void)> func = [&]()->int {
        errCode = trans.BatchInsert(rowNum, tableName, values);
        CHECK_AND_PRINT_LOG(errCode == E_OK, "InsertSql failed, errCode: %{public}d, rowNum: %{public}ld.",
            errCode, (long)rowNum);
        return errCode;
    };
    errCode = trans.RetryTrans(func, true);
    CHECK_AND_PRINT_LOG(errCode == E_OK, "BatchInsertWithRetry: tans finish fail!, ret:%{public}d", errCode);
    return errCode;
}

void AiRetouchClone::UpdateTotalInBatch(const std::string& fileIdClause, const std::string& inClause)
{
    aiRetouchFileIdMap_.clear();
    magicEmojiFileIdMap_.clear();
    std::vector<TotalAiRetouchTbl> totalAiRetouchTbls = QueryTotalAiRetouchTbl(fileIdClause, inClause);
    CHECK_AND_RETURN_LOG(!totalAiRetouchTbls.empty(), "QueryTotalAiRetouchTbl result empty in: %{public}s",
        fileIdClause.c_str());
    ProcessTotalAiRetouchTbls(totalAiRetouchTbls);
    UpdateTotalTbl();
}

std::vector<TotalAiRetouchTbl> AiRetouchClone::QueryTotalAiRetouchTbl(const std::string& fileIdClause,
    const std::string& inClause)
{
    std::vector<TotalAiRetouchTbl> result;
    std::string querySql =
        "SELECT " + inClause +
        " FROM " + VISION_TOTAL_TABLE;
    querySql += " WHERE file_id IN " + fileIdClause;

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(sourceRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSet is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        TotalAiRetouchTbl totalAiRetouchTbl;
        ParseTotalAiRetouchFromResultSet(resultSet, totalAiRetouchTbl);
        result.emplace_back(totalAiRetouchTbl);
    }

    resultSet->Close();
    return result;
}

void AiRetouchClone::ParseTotalAiRetouchFromResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
    TotalAiRetouchTbl& totalAiRetouchTbl)
{
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "resultSql is nullptr");
    totalAiRetouchTbl.fileId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, TOTAL_COL_FILE_ID);
    totalAiRetouchTbl.aiRetouch = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, TOTAL_COL_AI_RETOUCH);
    totalAiRetouchTbl.magicEmoji = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, TOTAL_COL_MAGIC_EMOJI);
}

void AiRetouchClone::ProcessTotalAiRetouchTbls(std::vector<TotalAiRetouchTbl>& totalAiRetouchTbls)
{
    CHECK_AND_RETURN_LOG(!totalAiRetouchTbls.empty(), "ai retouch tbl empty");
    for (const auto& totalAiRetouchTbl : totalAiRetouchTbls) {
        CHECK_AND_CONTINUE(totalAiRetouchTbl.fileId.has_value());
        int32_t oldFileId = totalAiRetouchTbl.fileId.value();
        const auto it = photoInfoMap_.find(oldFileId);
        if (it != photoInfoMap_.end()) {
            int32_t newFileId = it->second.fileIdNew;
            int32_t aiRetouch = totalAiRetouchTbl.aiRetouch.has_value() ?
                totalAiRetouchTbl.aiRetouch.value() : 0;
            int32_t magicEmoji = totalAiRetouchTbl.magicEmoji.has_value() ?
                totalAiRetouchTbl.magicEmoji.value() : 0;
            CHECK_AND_EXECUTE(aiRetouch == 0, aiRetouchFileIdMap_[aiRetouch].push_back(newFileId));
            CHECK_AND_EXECUTE(magicEmoji == 0, magicEmojiFileIdMap_[magicEmoji].push_back(newFileId));
        } else {
            MEDIA_WARN_LOG("Original fileId %{public}d not found in photoInfoMap_, skipping.", oldFileId);
        }
    }
}

void AiRetouchClone::UpdateTotalTbl()
{
    for (const auto& aiRetouch : aiRetouchFileIdMap_) {
        BatchUpdateTotal(TOTAL_COL_AI_RETOUCH, to_string(aiRetouch.first), aiRetouch.second);
    }
    for (const auto& magicEmoji : magicEmojiFileIdMap_) {
        BatchUpdateTotal(TOTAL_COL_MAGIC_EMOJI, to_string(magicEmoji.first), magicEmoji.second);
    }
}

void AiRetouchClone::BatchUpdateTotal(const std::string& column, const std::string& value,
    const std::vector<int32_t>& fileId)
{
    for (size_t i = 0; i < fileId.size(); i += SQL_BATCH_SIZE) {
        auto batch_begin = fileId.begin() + i;
        auto batch_end = ((i + SQL_BATCH_SIZE < fileId.size()) ?
            (fileId.begin() + i + SQL_BATCH_SIZE) : fileId.end());
        std::vector<int32_t> batchFileIds(batch_begin, batch_end);
        if (batchFileIds.empty()) {
            continue;
        }

        std::string fileIdInClause = "(" + BackupDatabaseUtils::JoinValues<int>(batchFileIds, ", ") + ")";
        std::string updateSql = "UPDATE tab_analysis_total "
            "SET " + column + " = " + value + " "
            "WHERE tab_analysis_total.file_id IN " + fileIdInClause;
        int32_t errCode = BackupDatabaseUtils::ExecuteSQL(destRdb_, updateSql);
        CHECK_AND_PRINT_LOG(errCode >= 0, "execute update analysis total failed for batch, ret=%{public}d", errCode);
        totalAiRetouchNum_ += batchFileIds.size();
    }
}

bool AiRetouchClone::QueryAndInsertSourceAiRetouch()
{
    if (isReverse_) {
        if (!HandleDuplicateAssetReplacement()) {
            MEDIA_ERR_LOG("QueryAndInsertSourceAiRetouch: Failed to handle duplicate asset replacement");
            return false;
        }
    }

    std::vector<int32_t> sourceFileIds = QuerySourceFileIds();
    if (sourceFileIds.empty()) {
        MEDIA_INFO_LOG("ReverseCloneAiRetouchInfo: No source file IDs found to clone");
        return true;
    }

    std::vector<std::string> aiRetouchCommonColumn = BackupDatabaseUtils::GetCommonColumnInfos(sourceRdb_, destRdb_,
        VISION_AI_RETOUCH_TABLE);
    bool isNeedInsertAiRetouch = aiRetouchCommonColumn.size() > 1;
    std::string aiRetouchInClause = BackupDatabaseUtils::JoinValues<std::string>(aiRetouchCommonColumn, ", ");

    if (!isNeedInsertAiRetouch) {
        MEDIA_INFO_LOG("ReverseCloneAiRetouchInfo: No need to insert ai retouch");
        return true;
    }

    std::vector<AiRetouchTbl> sourceAiRetouch = QuerySourceAiRetouch(sourceFileIds, aiRetouchInClause);
    if (sourceAiRetouch.empty()) {
        MEDIA_INFO_LOG("ReverseCloneAiRetouchInfo: No source ai retouch records found");
        return true;
    }

    return InsertOrUpdateDestAiRetouch(sourceAiRetouch);
}

std::vector<int32_t> AiRetouchClone::QuerySourceFileIds()
{
    std::vector<int32_t> fileIds;
    fileIds.reserve(photoInfoMap_.size());

    for (const auto& [sourceFileId, photoInfo] : photoInfoMap_) {
        fileIds.push_back(sourceFileId);
    }

    return fileIds;
}

std::vector<AiRetouchTbl> AiRetouchClone::QuerySourceAiRetouch(const std::vector<int32_t>& fileIds,
    const std::string& commonColumns)
{
    std::vector<AiRetouchTbl> result;
    if (fileIds.empty()) {
        return result;
    }

    for (size_t i = 0; i < fileIds.size(); i += SQL_BATCH_SIZE) {
        auto batch_begin = fileIds.begin() + i;
        auto batch_end = (i + SQL_BATCH_SIZE < fileIds.size()) ?
            (fileIds.begin() + i + SQL_BATCH_SIZE) : fileIds.end();
        std::vector<int32_t> batchFileIds(batch_begin, batch_end);

        if (batchFileIds.empty()) {
            continue;
        }

        std::string fileIdClause = "(" + BackupDatabaseUtils::JoinValues<int>(batchFileIds, ", ") + ")";
        std::vector<AiRetouchTbl> aiRetouchTbls = QueryAiRetouchTbl(fileIdClause, commonColumns);

        if (aiRetouchTbls.empty()) {
            MEDIA_WARN_LOG("Query returned empty result for batch starting at index %{public}zu", i);
            continue;
        }

        result.insert(result.end(), aiRetouchTbls.begin(), aiRetouchTbls.end());
    }

    return result;
}

bool AiRetouchClone::InsertOrUpdateDestAiRetouch(const std::vector<AiRetouchTbl>& aiRetouchTbls)
{
    if (aiRetouchTbls.empty()) {
        return true;
    }

    std::vector<int32_t> fileIds;
    fileIds.reserve(aiRetouchTbls.size());
    for (const auto& tbl : aiRetouchTbls) {
        if (tbl.fileId.has_value()) {
            fileIds.push_back(tbl.fileId.value());
        }
    }

    if (fileIds.empty()) {
        return true;
    }

    std::unordered_set<int32_t> existingFileIds = QueryExistingDestFileIds(fileIds);

    std::vector<AiRetouchTbl> toUpdate;
    std::vector<AiRetouchTbl> toInsert;
    toUpdate.reserve(aiRetouchTbls.size());
    toInsert.reserve(aiRetouchTbls.size());

    for (const auto& tbl : aiRetouchTbls) {
        if (!tbl.fileId.has_value()) {
            continue;
        }
        const int32_t fileId = tbl.fileId.value();
        if (existingFileIds.count(fileId) > 0) {
            toUpdate.push_back(tbl);
        } else {
            toInsert.push_back(tbl);
        }
    }

    bool updateSuccess = true;
    if (!toUpdate.empty()) {
        updateSuccess = UpdateDestAiRetouch(toUpdate);
    }

    bool insertSuccess = true;
    if (!toInsert.empty()) {
        insertSuccess = InsertNewDestAiRetouch(toInsert);
    }

    return updateSuccess && insertSuccess;
}

std::unordered_set<int32_t> AiRetouchClone::QueryExistingDestFileIds(const std::vector<int32_t>& fileIds)
{
    std::unordered_set<int32_t> existingIds;

    for (size_t i = 0; i < fileIds.size(); i += SQL_BATCH_SIZE) {
        auto batch_begin = fileIds.begin() + i;
        auto batch_end = (i + SQL_BATCH_SIZE < fileIds.size()) ?
            (fileIds.begin() + i + SQL_BATCH_SIZE) : fileIds.end();
        std::vector<int32_t> batchFileIds(batch_begin, batch_end);

        if (batchFileIds.empty()) {
            continue;
        }

        const std::string fileIdList = BackupDatabaseUtils::JoinValues(batchFileIds, ", ");
        const std::string querySql = "SELECT " + AI_RETOUCH_COL_FILE_ID +
            " FROM " + VISION_AI_RETOUCH_TABLE +
            " WHERE " + AI_RETOUCH_COL_FILE_ID + " IN (" + fileIdList + ")" +
            " AND " + AI_RETOUCH_COL_FILE_ID + " <= " + std::to_string(maxTotalFileId_);

        auto resultSet = BackupDatabaseUtils::GetQueryResultSet(destRdb_, querySql);
        CHECK_AND_RETURN_RET(resultSet != nullptr, existingIds);

        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            int32_t fileId = 0;
            if (resultSet->GetInt(0, fileId) == NativeRdb::E_OK) {
                existingIds.insert(fileId);
            }
        }
        resultSet->Close();
    }

    return existingIds;
}

bool AiRetouchClone::UpdateDestAiRetouch(const std::vector<AiRetouchTbl>& aiRetouchTbls)
{
    if (aiRetouchTbls.empty()) {
        return true;
    }

    int32_t updatedCount = 0;
    std::vector<std::pair<NativeRdb::ValuesBucket, std::string>> updates;
    updates.reserve(aiRetouchTbls.size());

    for (const auto& tbl : aiRetouchTbls) {
        if (!tbl.fileId.has_value()) {
            continue;
        }

        NativeRdb::ValuesBucket values = CreateValuesBucketFromAiRetouchTbl(tbl);
        const std::string whereClause = AI_RETOUCH_COL_FILE_ID + " = " +
            std::to_string(tbl.fileId.value()) +
            " AND " + AI_RETOUCH_COL_FILE_ID + " <= " + std::to_string(maxTotalFileId_);

        updates.emplace_back(std::move(values), whereClause);
    }

    if (updates.empty()) {
        return true;
    }

    int32_t errCode = BatchUpdateWithRetry(VISION_AI_RETOUCH_TABLE, updates);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, false, "Failed to update ai retouch");

    updatedCount = static_cast<int32_t>(updates.size());
    aiRetouchNum_ += updatedCount;
    MEDIA_INFO_LOG("ReverseCloneAiRetouchInfo: Updated %{public}d ai retouch records", updatedCount);
    return true;
}

bool AiRetouchClone::InsertNewDestAiRetouch(const std::vector<AiRetouchTbl>& aiRetouchTbls)
{
    if (aiRetouchTbls.empty()) {
        return true;
    }

    std::vector<NativeRdb::ValuesBucket> valuesBucketsToInsert;
    valuesBucketsToInsert.reserve(aiRetouchTbls.size());

    for (const auto& aiRetouchTbl : aiRetouchTbls) {
        if (!aiRetouchTbl.fileId.has_value()) {
            MEDIA_WARN_LOG("aiRetouchTbl has no fileId, skipping.");
            continue;
        }

        valuesBucketsToInsert.push_back(CreateValuesBucketFromAiRetouchTbl(aiRetouchTbl));
    }

    if (valuesBucketsToInsert.empty()) {
        return true;
    }

    int64_t rowNum = 0;
    int32_t ret = BatchInsertWithRetry(VISION_AI_RETOUCH_TABLE, valuesBucketsToInsert, rowNum);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, false, "Failed to batch insert ai retouch");

    aiRetouchNum_ += rowNum;
    MEDIA_INFO_LOG("ReverseCloneAiRetouchInfo: Inserted %{public}lld new ai retouch records",
        (long long)rowNum);
    return true;
}

int32_t AiRetouchClone::BatchUpdateWithRetry(const std::string& tableName,
    const std::vector<std::pair<NativeRdb::ValuesBucket, std::string>>& updates)
{
    CHECK_AND_RETURN_RET(!updates.empty(), E_OK);
    int32_t errCode = E_ERR;
    TransactionOperations trans{ __func__ };
    trans.SetBackupRdbStore(destRdb_);

    std::function<int(void)> func = [&]()->int {
        for (const auto& [values, whereClause] : updates) {
            int32_t changedRows = 0;
            errCode = destRdb_->Update(changedRows, tableName, values, whereClause);
            if (errCode != E_OK) {
                MEDIA_ERR_LOG("Update failed for whereClause: %{public}s, errCode: %{public}d",
                    whereClause.c_str(), errCode);
                return errCode;
            }
        }
        return E_OK;
    };

    errCode = trans.RetryTrans(func, true);
    CHECK_AND_PRINT_LOG(errCode == E_OK, "BatchUpdateWithRetry: transaction finish fail!, ret:%{public}d", errCode);
    return errCode;
}

bool AiRetouchClone::HandleDuplicateAssetReplacement()
{
    std::vector<int32_t> sourceFileIds;
    std::vector<int32_t> destFileIds;
    std::unordered_map<int32_t, int32_t> fileIdMapping;

    sourceFileIds.reserve(photoInfoMap_.size());
    destFileIds.reserve(photoInfoMap_.size());

    for (const auto& [sourceFileId, photoInfo] : photoInfoMap_) {
        int32_t destFileId = photoInfo.fileIdNew;
        if (sourceFileId == destFileId) {
            continue;
        }

        sourceFileIds.push_back(sourceFileId);
        destFileIds.push_back(destFileId);
        fileIdMapping[sourceFileId] = destFileId;
    }

    if (sourceFileIds.empty()) {
        return true;
    }

    if (!BatchDeleteDestAiRetouch(destFileIds)) {
        MEDIA_ERR_LOG("HandleDuplicateAssetReplacement: Failed to delete dest ai retouch");
        return false;
    }

    if (!BatchQueryAndInsertSourceAiRetouch(sourceFileIds, fileIdMapping)) {
        MEDIA_ERR_LOG("HandleDuplicateAssetReplacement: Failed to insert source ai retouch");
        return false;
    }

    return true;
}

bool AiRetouchClone::BatchDeleteDestAiRetouch(const std::vector<int32_t>& destFileIds)
{
    if (destFileIds.empty()) {
        return true;
    }

    int32_t errCode = E_ERR;
    TransactionOperations trans{ __func__ };
    trans.SetBackupRdbStore(destRdb_);

    std::function<int(void)> func = [&]()->int {
        for (size_t i = 0; i < destFileIds.size(); i += SQL_BATCH_SIZE) {
            auto batch_begin = destFileIds.begin() + i;
            auto batch_end = (i + SQL_BATCH_SIZE < destFileIds.size()) ?
                (destFileIds.begin() + i + SQL_BATCH_SIZE) : destFileIds.end();
            std::vector<int32_t> batchFileIds(batch_begin, batch_end);

            if (batchFileIds.empty()) {
                continue;
            }

            std::string fileIdList = BackupDatabaseUtils::JoinValues(batchFileIds, ", ");
            std::string deleteSql = "DELETE FROM " + VISION_AI_RETOUCH_TABLE +
                " WHERE " + AI_RETOUCH_COL_FILE_ID + " IN (" + fileIdList + ")";

            errCode = BackupDatabaseUtils::ExecuteSQL(destRdb_, deleteSql);
            if (errCode < 0) {
                MEDIA_ERR_LOG("BatchDeleteDestAiRetouch: Failed to delete, errCode: %{public}d", errCode);
                return errCode;
            }
        }
        return E_OK;
    };

    errCode = trans.RetryTrans(func, true);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, false,
        "BatchDeleteDestAiRetouch: transaction failed, errCode: %{public}d", errCode);

    return true;
}

bool AiRetouchClone::BatchQueryAndInsertSourceAiRetouch(const std::vector<int32_t>& sourceFileIds,
    const std::unordered_map<int32_t, int32_t>& fileIdMapping)
{
    if (sourceFileIds.empty()) {
        return true;
    }

    std::vector<std::string> aiRetouchCommonColumn = BackupDatabaseUtils::GetCommonColumnInfos(sourceRdb_, destRdb_,
        VISION_AI_RETOUCH_TABLE);
    bool isNeedInsertAiRetouch = aiRetouchCommonColumn.size() > 1;
    std::string aiRetouchInClause = BackupDatabaseUtils::JoinValues<std::string>(aiRetouchCommonColumn, ", ");

    if (!isNeedInsertAiRetouch) {
        return true;
    }

    std::vector<AiRetouchTbl> allAiRetouchTbls;
    allAiRetouchTbls.reserve(sourceFileIds.size());

    for (size_t i = 0; i < sourceFileIds.size(); i += SQL_BATCH_SIZE) {
        auto batch_begin = sourceFileIds.begin() + i;
        auto batch_end = (i + SQL_BATCH_SIZE < sourceFileIds.size()) ?
            (sourceFileIds.begin() + i + SQL_BATCH_SIZE) : sourceFileIds.end();
        std::vector<int32_t> batchFileIds(batch_begin, batch_end);

        if (batchFileIds.empty()) {
            continue;
        }

        std::string fileIdClause = "(" + BackupDatabaseUtils::JoinValues<int>(batchFileIds, ", ") + ")";
        std::vector<AiRetouchTbl> aiRetouchTbls = QueryAiRetouchTbl(fileIdClause, aiRetouchInClause);

        if (aiRetouchTbls.empty()) {
            continue;
        }

        allAiRetouchTbls.insert(allAiRetouchTbls.end(), aiRetouchTbls.begin(), aiRetouchTbls.end());
    }

    if (allAiRetouchTbls.empty()) {
        return true;
    }

    for (auto& tbl : allAiRetouchTbls) {
        if (tbl.fileId.has_value()) {
            int32_t sourceFileId = tbl.fileId.value();
            const auto it = fileIdMapping.find(sourceFileId);
            if (it != fileIdMapping.end()) {
                tbl.fileId = it->second;
            }
        }
    }

    return InsertNewDestAiRetouch(allAiRetouchTbls);
}
} // namespace OHOS::Media
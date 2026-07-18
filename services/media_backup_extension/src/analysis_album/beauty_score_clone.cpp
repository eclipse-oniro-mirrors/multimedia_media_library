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

#include "beauty_score_clone.h"

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
// Score mask bits
const uint32_t BIT0 = 1u << 0;  // 美学基础分
const uint32_t BIT20 = 1u << 20;  // 刷新状态标记

BeautyScoreClone::BeautyScoreClone(
    const std::shared_ptr<NativeRdb::RdbStore>& sourceRdb,
    const std::shared_ptr<NativeRdb::RdbStore>& destRdb,
    const std::unordered_map<int32_t, PhotoInfo>& photoInfoMap,
    const int64_t maxBeautyFileId,
    std::unordered_map<int32_t, uint32_t>* scoreMaskMap)
    : sourceRdb_(sourceRdb),
      destRdb_(destRdb),
      photoInfoMap_(photoInfoMap),
      maxBeautyFileId_(maxBeautyFileId),
      externalScoreMaskMap_(scoreMaskMap)
{
}

std::unordered_set<int32_t> BeautyScoreClone::CloneBeautyScoreInBatches(const std::vector<int32_t>& oldFileIds,
    const std::vector<std::string>& commonColumns)
{
    std::unordered_set<int32_t> allInsertedFileIds;
    for (size_t i = 0; i < oldFileIds.size(); i += SQL_BATCH_SIZE) {
        auto batch_begin = oldFileIds.begin() + i;
        auto batch_end = ((i + SQL_BATCH_SIZE < oldFileIds.size()) ?
            (oldFileIds.begin() + i + SQL_BATCH_SIZE) : oldFileIds.end());
        std::vector<int32_t> batchOldFileIds(batch_begin, batch_end);

        if (batchOldFileIds.empty()) {
            continue;
        }

        std::string fileIdOldInClause = "(" + BackupDatabaseUtils::JoinValues<int>(batchOldFileIds, ", ") + ")";
        std::vector<BeautyScoreTbl> beautyScoreTbls = QueryBeautyScoreTbl(fileIdOldInClause, commonColumns);
        if (beautyScoreTbls.empty()) {
            MEDIA_WARN_LOG("Query returned empty result for batch starting at index %{public}zu", i);
            continue;
        }

        std::vector<BeautyScoreTbl> processBeautyScores = ProcessBeautyScoreTbls(beautyScoreTbls);
        std::unordered_set<int32_t> insertedFileIds = BatchInsertBeautyScores(processBeautyScores);
        allInsertedFileIds.insert(insertedFileIds.begin(), insertedFileIds.end());
    }
    return allInsertedFileIds;
}

bool BeautyScoreClone::CloneBeautyScoreInfo()
{
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    std::vector<int32_t> oldFileIds;
    oldFileIds.reserve(photoInfoMap_.size());
    std::vector<int32_t> newFileIds;
    newFileIds.reserve(photoInfoMap_.size());

    for (const auto& pair : photoInfoMap_) {
        oldFileIds.push_back(pair.first);
        newFileIds.push_back(pair.second.fileIdNew);
    }

    if (oldFileIds.empty()) {
        MEDIA_INFO_LOG("photoInfoMap_ is empty, no aesthetics score entries to clone.");
        migrateScoreTotalTimeCost_ = MediaFileUtils::UTCTimeMilliSeconds() - start;
        return true;
    }

    std::vector<std::string> commonColumn = BackupDatabaseUtils::GetCommonColumnInfos(sourceRdb_, destRdb_,
        VISION_AESTHETICS_TABLE);
    std::vector<std::string> commonColumns = BackupDatabaseUtils::FilterExcludedColumns(commonColumn,
        EXCLUDED_BEAUTY_SCORE_COLUMNS);
    CHECK_AND_RETURN_RET_LOG(!commonColumns.empty(),
        false, "No common columns found for aesthetics score table after exclusion.");

    std::unordered_set<int32_t> insertedFileIds = CloneBeautyScoreInBatches(oldFileIds, commonColumns);

    UpdateTotalTblBeautyScoreStatus(destRdb_, newFileIds);
    UpdateAnalysisTotalTblBeautyScore(destRdb_, sourceRdb_, newFileIds, oldFileIds);
    UpdateTotalTblBeautyScoreAllStatus(destRdb_, newFileIds);
    UpdateAnalysisTotalTblBeautyScoreAll(destRdb_, sourceRdb_, newFileIds, oldFileIds);

    // 记录需要刷新的分数类型的 mask 值（只记录实际克隆的）
    MEDIA_INFO_LOG("record bit0 of mask");
    for (int32_t fileId : insertedFileIds) {
        UpdateScoreMask(fileId, BIT0 | BIT20);  // 美学基础分
    }

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    migrateScoreTotalTimeCost_ += end - start;
    MEDIA_INFO_LOG("CloneBeautyScoreInfo completed. Migrated %{public}lld records. "
        "Total time: %{public}lld ms",
        (long long)migrateScoreNum_, (long long)migrateScoreTotalTimeCost_);
    return true;
}

bool BeautyScoreClone::ReverseCloneBeautyScoreInfo()
{
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    if (!QueryAndInsertSourceBeautyScores()) {
        MEDIA_ERR_LOG("ReverseCloneBeautyScoreInfo: Failed to query and insert source beauty scores");
        migrateScoreTotalTimeCost_ = MediaFileUtils::UTCTimeMilliSeconds() - start;
        return false;
    }

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    migrateScoreTotalTimeCost_ += end - start;
    MEDIA_INFO_LOG("ReverseCloneBeautyScoreInfo completed. Migrated %{public}lld records. "
        "Total time: %{public}lld ms",
        (long long)migrateScoreNum_, (long long)migrateScoreTotalTimeCost_);
    return true;
}

std::vector<BeautyScoreTbl> BeautyScoreClone::QueryBeautyScoreTbl(const std::string &fileIdClause,
    const std::vector<std::string> &commonColumns)
{
    std::vector<BeautyScoreTbl> result;

    std::string inClause = BackupDatabaseUtils::JoinValues<std::string>(commonColumns, ", ");
    std::string querySql =
        "SELECT " + inClause +
        " FROM " + VISION_AESTHETICS_TABLE;
    querySql += " WHERE " + BEAUTY_SCORE_COL_FILE_ID + " IN " + fileIdClause;

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(sourceRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSet is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        BeautyScoreTbl beautyScoreTbl;
        ParseBeautyScoreResultSet(resultSet, beautyScoreTbl);
        result.emplace_back(beautyScoreTbl);
    }

    resultSet->Close();
    return result;
}

void BeautyScoreClone::ParseBeautyScoreResultSet(
    const std::shared_ptr<NativeRdb::ResultSet>& resultSet, BeautyScoreTbl& beautyScoreTbl)
{
    beautyScoreTbl.fileId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, BEAUTY_SCORE_COL_FILE_ID);
    beautyScoreTbl.aestheticsScore = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        BEAUTY_SCORE_COL_AESTHETICS_SCORE);
    beautyScoreTbl.aestheticsVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        BEAUTY_SCORE_COL_AESTHETICS_VERSION);
    beautyScoreTbl.prob = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, BEAUTY_SCORE_COL_PROB);
    beautyScoreTbl.analysisVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        BEAUTY_SCORE_COL_ANALYSIS_VERSION);
    beautyScoreTbl.selectedFlag = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        BEAUTY_SCORE_COL_SELECTED_FLAG);
    beautyScoreTbl.selectedAlgoVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        BEAUTY_SCORE_COL_SELECTED_ALGO_VERSION);
    beautyScoreTbl.selectedStatus = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        BEAUTY_SCORE_COL_SELECTED_STATUS);
    beautyScoreTbl.negativeFlag = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        BEAUTY_SCORE_COL_NEGATIVE_FLAG);
    beautyScoreTbl.negativeAlgoVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        BEAUTY_SCORE_COL_NEGATIVE_ALGO_VERSION);
    beautyScoreTbl.aestheticsAllVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        BEAUTY_SCORE_COL_AESTHETICS_ALL_VERSION);
    beautyScoreTbl.aestheticsScoreAll = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        BEAUTY_SCORE_COL_AESTHETICS_SCORE_ALL);
    beautyScoreTbl.isFilteredHard = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        BEAUTY_SCORE_COL_IS_FILTERED_HARD);
    beautyScoreTbl.clarityScoreAll = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        BEAUTY_SCORE_COL_CLARITY_SCORE_ALL);
    beautyScoreTbl.saturationScoreAll = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        BEAUTY_SCORE_COL_SATURATION_SCORE_ALL);
    beautyScoreTbl.luminanceScoreAll = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        BEAUTY_SCORE_COL_LUMINANCE_SCORE_ALL);
    beautyScoreTbl.semanticsScore = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        BEAUTY_SCORE_COL_SEMANTICS_SCORE);
    beautyScoreTbl.isBlackWhiteStripe = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        BEAUTY_SCORE_COL_IS_BLACK_WHITE_STRIPE);
    beautyScoreTbl.isBlurry = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        BEAUTY_SCORE_COL_IS_BLURRY);
    beautyScoreTbl.isMosaic = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        BEAUTY_SCORE_COL_IS_MOSAIC);
}

std::vector<BeautyScoreTbl> BeautyScoreClone::ProcessBeautyScoreTbls(
    const std::vector<BeautyScoreTbl>& beautyScoreTbls)
{
    CHECK_AND_RETURN_RET_LOG(!beautyScoreTbls.empty(), {}, "aesthetics scores tbl empty");

    std::vector<BeautyScoreTbl> beautyScoreNewTbls;
    beautyScoreNewTbls.reserve(beautyScoreTbls.size());

    for (const auto& beautyScoreTbl : beautyScoreTbls) {
        if (beautyScoreTbl.fileId.has_value()) {
            int32_t oldFileId = beautyScoreTbl.fileId.value();
            const auto it = photoInfoMap_.find(oldFileId);
            if (it != photoInfoMap_.end()) {
                BeautyScoreTbl updatedScore = beautyScoreTbl;
                updatedScore.fileId = it->second.fileIdNew;
                beautyScoreNewTbls.push_back(std::move(updatedScore));
            } else {
                MEDIA_WARN_LOG("Original fileId %{public}d not found in photoInfoMap_, skipping.", oldFileId);
            }
        }
    }
    return beautyScoreNewTbls;
}

std::unordered_set<int32_t> BeautyScoreClone::BatchInsertBeautyScores(
    const std::vector<BeautyScoreTbl> &beautyScoreTbls)
{
    std::vector<NativeRdb::ValuesBucket> valuesBucketsToInsert;
    std::unordered_set<int32_t> fileIdsNewlyInserted;
    valuesBucketsToInsert.reserve(beautyScoreTbls.size());

    for (const auto& beautyScoreTbl : beautyScoreTbls) {
        if (!beautyScoreTbl.fileId.has_value()) {
            MEDIA_WARN_LOG("BeautyScoreTbl has no fileId, skipping.");
            continue;
        }

        int32_t currentFileId = beautyScoreTbl.fileId.value();
        if (currentFileId <= maxBeautyFileId_) {
            continue;
        }

        valuesBucketsToInsert.push_back(CreateValuesBucketFromBeautyScoreTbl(beautyScoreTbl));
        fileIdsNewlyInserted.insert(currentFileId);
    }

    if (valuesBucketsToInsert.empty()) {
        MEDIA_ERR_LOG("No new aesthetics score entries to insert after filtering by maxBeautyFileId_.");
        return fileIdsNewlyInserted;
    }

    int64_t rowNum = 0;
    int32_t ret = BatchInsertWithRetry(VISION_AESTHETICS_TABLE, valuesBucketsToInsert, rowNum);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, fileIdsNewlyInserted, "Failed to batch insert aesthetics scores");

    migrateScoreNum_ += rowNum;
    migrateScoreFileNumber_ += static_cast<int64_t>(fileIdsNewlyInserted.size());
    return fileIdsNewlyInserted;
}

NativeRdb::ValuesBucket BeautyScoreClone::CreateValuesBucketFromBeautyScoreTbl(
    const BeautyScoreTbl& beautyScoreTbl)
{
    NativeRdb::ValuesBucket values;

    PutIfPresent(values, BEAUTY_SCORE_COL_ID, beautyScoreTbl.id);
    PutIfPresent(values, BEAUTY_SCORE_COL_FILE_ID, beautyScoreTbl.fileId);
    PutIfPresent(values, BEAUTY_SCORE_COL_AESTHETICS_SCORE, beautyScoreTbl.aestheticsScore);
    PutIfPresent(values, BEAUTY_SCORE_COL_AESTHETICS_VERSION, beautyScoreTbl.aestheticsVersion);
    PutIfPresent(values, BEAUTY_SCORE_COL_PROB, beautyScoreTbl.prob);
    PutIfPresent(values, BEAUTY_SCORE_COL_ANALYSIS_VERSION, beautyScoreTbl.analysisVersion);
    PutIfPresent(values, BEAUTY_SCORE_COL_SELECTED_FLAG, beautyScoreTbl.selectedFlag);
    PutIfPresent(values, BEAUTY_SCORE_COL_SELECTED_ALGO_VERSION, beautyScoreTbl.selectedAlgoVersion);
    PutIfPresent(values, BEAUTY_SCORE_COL_SELECTED_STATUS, beautyScoreTbl.selectedStatus);
    PutIfPresent(values, BEAUTY_SCORE_COL_NEGATIVE_FLAG, beautyScoreTbl.negativeFlag);
    PutIfPresent(values, BEAUTY_SCORE_COL_NEGATIVE_ALGO_VERSION, beautyScoreTbl.negativeAlgoVersion);
    PutIfPresent(values, BEAUTY_SCORE_COL_AESTHETICS_ALL_VERSION, beautyScoreTbl.aestheticsAllVersion);
    PutWithDefault<int32_t>(values, BEAUTY_SCORE_COL_AESTHETICS_SCORE_ALL, beautyScoreTbl.aestheticsScoreAll, 0);
    PutWithDefault<int32_t>(values, BEAUTY_SCORE_COL_IS_FILTERED_HARD, beautyScoreTbl.isFilteredHard, 0);
    PutWithDefault<double>(values, BEAUTY_SCORE_COL_CLARITY_SCORE_ALL, beautyScoreTbl.clarityScoreAll, 0);
    PutWithDefault<double>(values, BEAUTY_SCORE_COL_SATURATION_SCORE_ALL, beautyScoreTbl.saturationScoreAll, 0);
    PutWithDefault<double>(values, BEAUTY_SCORE_COL_LUMINANCE_SCORE_ALL, beautyScoreTbl.luminanceScoreAll, 0);
    PutWithDefault<double>(values, BEAUTY_SCORE_COL_SEMANTICS_SCORE, beautyScoreTbl.semanticsScore, 0);
    PutWithDefault<int32_t>(values, BEAUTY_SCORE_COL_IS_BLACK_WHITE_STRIPE, beautyScoreTbl.isBlackWhiteStripe, 0);
    PutWithDefault<int32_t>(values, BEAUTY_SCORE_COL_IS_BLURRY, beautyScoreTbl.isBlurry, 0);
    PutWithDefault<int32_t>(values, BEAUTY_SCORE_COL_IS_MOSAIC, beautyScoreTbl.isMosaic, 0);

    return values;
}

int32_t BeautyScoreClone::BatchInsertWithRetry(const std::string &tableName,
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

void BeautyScoreClone::UpdateTotalTblBeautyScoreStatus(
    std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::vector<int32_t>& newFileIds)
{
    if (newFileIds.empty()) {
        return;
    }

    for (size_t i = 0; i < newFileIds.size(); i += SQL_BATCH_SIZE) {
        auto batch_begin = newFileIds.begin() + i;
        auto batch_end = ((i + SQL_BATCH_SIZE < newFileIds.size()) ?
            (newFileIds.begin() + i + SQL_BATCH_SIZE) : newFileIds.end());
        std::vector<int32_t> batchNewFileIds(batch_begin, batch_end);

        if (batchNewFileIds.empty()) {
            continue;
        }

        std::string fileIdNewFilterClause = "(" + BackupDatabaseUtils::JoinValues<int>(batchNewFileIds, ", ") + ")";
        std::string updateSql = "UPDATE tab_analysis_total "
            "SET aesthetics_score = 1 "
            "WHERE EXISTS (SELECT 1 FROM tab_analysis_aesthetics_score "
            "WHERE tab_analysis_aesthetics_score.file_id = tab_analysis_total.file_id) "
            "AND tab_analysis_total.file_id IN " + fileIdNewFilterClause;

        int32_t errCode = BackupDatabaseUtils::ExecuteSQL(rdbStore, updateSql);
        CHECK_AND_PRINT_LOG(errCode >= 0, "execute update analysis total failed for batch, ret=%{public}d", errCode);
    }
}

void BeautyScoreClone::UpdateTotalTblBeautyScoreAllStatus(
    std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::vector<int32_t>& newFileIds)
{
    if (newFileIds.empty()) {
        return;
    }

    for (size_t i = 0; i < newFileIds.size(); i += SQL_BATCH_SIZE) {
        auto batch_begin = newFileIds.begin() + i;
        auto batch_end = ((i + SQL_BATCH_SIZE < newFileIds.size()) ?
            (newFileIds.begin() + i + SQL_BATCH_SIZE) : newFileIds.end());
        std::vector<int32_t> batchNewFileIds(batch_begin, batch_end);

        if (batchNewFileIds.empty()) {
            continue;
        }

        std::string fileIdNewFilterClause = "(" + BackupDatabaseUtils::JoinValues<int>(batchNewFileIds, ", ") + ")";
        std::string updateSql = "UPDATE tab_analysis_total "
            "SET aesthetics_score_all = 1 "
            "WHERE EXISTS (SELECT 1 FROM tab_analysis_aesthetics_score "
            "WHERE tab_analysis_aesthetics_score.file_id = tab_analysis_total.file_id "
            "AND tab_analysis_aesthetics_score.aesthetics_all_version IS NOT NULL "
            "AND tab_analysis_aesthetics_score.aesthetics_score_all != 0) "
            "AND tab_analysis_total.file_id IN " + fileIdNewFilterClause;

        int32_t errCode = BackupDatabaseUtils::ExecuteSQL(rdbStore, updateSql);
        CHECK_AND_PRINT_LOG(errCode >= 0, "execute update analysis total failed for batch, ret=%{public}d", errCode);
    }
}

std::unordered_map<int32_t, int32_t> BeautyScoreClone::QueryBeautyScoreMap(shared_ptr<NativeRdb::RdbStore> rdbStore,
    const std::string& sql, const std::string& keyColumnName, const std::string& valueColumnName)
{
    std::unordered_map<int32_t, int32_t> results;
    if (rdbStore == nullptr) {
        return results;
    }

    auto resultSet = rdbStore->QuerySql(sql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query SQL or resultSet is null for QueryIntIntMap");
        return results;
    }

    int32_t keyColumnIndex = -1;
    int32_t valueColumnIndex = -1;

    if (resultSet->GetColumnIndex(keyColumnName, keyColumnIndex) != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to get column index for keyColumnName: %{public}s", keyColumnName.c_str());
        resultSet->Close();
        return results;
    }
    if (resultSet->GetColumnIndex(valueColumnName, valueColumnIndex) != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to get column index for valueColumnName: %{public}s", valueColumnName.c_str());
        resultSet->Close();
        return results;
    }

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t key;
        int32_t value;
        if (resultSet->GetInt(keyColumnIndex, key) == NativeRdb::E_OK &&
            resultSet->GetInt(valueColumnIndex, value) == NativeRdb::E_OK) {
            results[key] = value;
        } else {
            MEDIA_ERR_LOG("Failed to get int values from resultSet for key or value column.");
        }
    }

    resultSet->Close();
    return results;
}

std::unordered_map<int32_t, int32_t> BeautyScoreClone::QueryScoresForColumnInBatches(
    std::shared_ptr<NativeRdb::RdbStore> oldRdbStore, const std::vector<int32_t>& fileIdOld,
    const std::string& scoreColumnName)
{
    std::unordered_map<int32_t, int32_t> oldFileIdToScoreMap;
    oldFileIdToScoreMap.reserve(fileIdOld.size());

    for (size_t i = 0; i < fileIdOld.size(); i += SQL_BATCH_SIZE) {
        auto batch_begin = fileIdOld.begin() + i;
        auto batch_end = ((i + SQL_BATCH_SIZE < fileIdOld.size()) ?
            (fileIdOld.begin() + i + SQL_BATCH_SIZE) : fileIdOld.end());
        std::vector<int32_t> batchOldFileIds(batch_begin, batch_end);

        if (batchOldFileIds.empty()) {
            continue;
        }

        std::string fileIdOldInClause = "(" + BackupDatabaseUtils::JoinValues<int>(batchOldFileIds, ", ") + ")";
        std::string querySql = "SELECT file_id, " + scoreColumnName + " FROM tab_analysis_total "
            "WHERE " + scoreColumnName + " < 0 AND file_id IN " + fileIdOldInClause;

        std::unordered_map<int32_t, int32_t> batchResultMap =
            QueryBeautyScoreMap(oldRdbStore, querySql, "file_id", scoreColumnName);

        if (!batchResultMap.empty()) {
            oldFileIdToScoreMap.insert(batchResultMap.begin(), batchResultMap.end());
        }
    }
    return oldFileIdToScoreMap;
}

void BeautyScoreClone::ApplyScoreUpdatesToNewDb(std::shared_ptr<NativeRdb::RdbStore> newRdbStore,
    const std::unordered_map<int32_t, int32_t>& oldFileIdToScoreMap, const std::string& scoreColumnName)
{
    for (const auto& [oldId, score] : oldFileIdToScoreMap) {
        auto it = photoInfoMap_.find(oldId);
        if (it != photoInfoMap_.end()) {
            int32_t newId = it->second.fileIdNew;
            std::string updateSql = "UPDATE tab_analysis_total "
                "SET " + scoreColumnName + " = " + std::to_string(score) + " "
                "WHERE file_id = " + std::to_string(newId);

            int32_t errCode = BackupDatabaseUtils::ExecuteSQL(newRdbStore, updateSql);
            CHECK_AND_PRINT_LOG(errCode >= 0,
                "execute update analysis total for %{public}s failed for newId=%{public}d, ret=%{public}d",
                scoreColumnName.c_str(), newId, errCode);
        }
    }
}

void BeautyScoreClone::UpdateAnalysisTotalTblForScoreColumn(std::shared_ptr<NativeRdb::RdbStore> newRdbStore,
    std::shared_ptr<NativeRdb::RdbStore> oldRdbStore, const std::vector<int32_t>& fileIdOld,
    const std::string& scoreColumnName)
{
    if (fileIdOld.empty()) {
        MEDIA_ERR_LOG("No old file IDs to process for %{public}s update.", scoreColumnName.c_str());
        return;
    }

    std::unordered_map<int32_t, int32_t> oldFileIdToScoreMap =
        QueryScoresForColumnInBatches(oldRdbStore, fileIdOld, scoreColumnName);

    if (oldFileIdToScoreMap.empty()) {
        MEDIA_ERR_LOG("No old files found with %{public}s < 0 status to migrate.", scoreColumnName.c_str());
        return;
    }

    ApplyScoreUpdatesToNewDb(newRdbStore, oldFileIdToScoreMap, scoreColumnName);
}

void BeautyScoreClone::UpdateAnalysisTotalTblBeautyScore(std::shared_ptr<NativeRdb::RdbStore> newRdbStore,
    std::shared_ptr<NativeRdb::RdbStore> oldRdbStore, const std::vector<int32_t>& fileIdNew,
    const std::vector<int32_t>& fileIdOld)
{
    UpdateAnalysisTotalTblForScoreColumn(newRdbStore, oldRdbStore, fileIdOld, "aesthetics_score");
}

void BeautyScoreClone::UpdateAnalysisTotalTblBeautyScoreAll(std::shared_ptr<NativeRdb::RdbStore> newRdbStore,
    std::shared_ptr<NativeRdb::RdbStore> oldRdbStore, const std::vector<int32_t>& fileIdNew,
    const std::vector<int32_t>& fileIdOld)
{
    UpdateAnalysisTotalTblForScoreColumn(newRdbStore, oldRdbStore, fileIdOld, "aesthetics_score_all");
}

void BeautyScoreClone::UpdateScoreMask(int32_t fileId, uint32_t mask)
{
    if (externalScoreMaskMap_ != nullptr) {
        (*externalScoreMaskMap_)[fileId] |= mask;
    } else {
        scoreMaskMap_[fileId] |= mask;
    }
}

bool BeautyScoreClone::QueryAndInsertSourceBeautyScores()
{
    std::vector<int32_t> sourceFileIds = QuerySourceFileIds();
    if (sourceFileIds.empty()) {
        MEDIA_INFO_LOG("ReverseCloneBeautyScoreInfo: No source file IDs found to clone");
        return true;
    }

    std::vector<std::string> commonColumn = BackupDatabaseUtils::GetCommonColumnInfos(sourceRdb_, destRdb_,
        VISION_AESTHETICS_TABLE);
    std::vector<std::string> commonColumns = BackupDatabaseUtils::FilterExcludedColumns(commonColumn,
        EXCLUDED_BEAUTY_SCORE_COLUMNS);
    CHECK_AND_RETURN_RET_LOG(!commonColumns.empty(), false,
        "No common columns found for aesthetics score table after exclusion.");

    std::vector<BeautyScoreTbl> sourceBeautyScores = QuerySourceBeautyScores(sourceFileIds, commonColumns);
    if (sourceBeautyScores.empty()) {
        MEDIA_INFO_LOG("ReverseCloneBeautyScoreInfo: No source beauty score records found");
        return true;
    }

    return InsertOrUpdateDestBeautyScores(sourceBeautyScores);
}

std::vector<int32_t> BeautyScoreClone::QuerySourceFileIds()
{
    std::vector<int32_t> fileIds;
    fileIds.reserve(photoInfoMap_.size());

    for (const auto& [sourceFileId, photoInfo] : photoInfoMap_) {
        fileIds.push_back(sourceFileId);
    }

    return fileIds;
}

std::vector<BeautyScoreTbl> BeautyScoreClone::QuerySourceBeautyScores(const std::vector<int32_t>& fileIds,
    const std::vector<std::string>& commonColumns)
{
    std::vector<BeautyScoreTbl> result;
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
        std::vector<BeautyScoreTbl> beautyScoreTbls = QueryBeautyScoreTbl(fileIdClause, commonColumns);

        if (beautyScoreTbls.empty()) {
            MEDIA_WARN_LOG("Query returned empty result for batch starting at index %{public}zu", i);
            continue;
        }

        result.insert(result.end(), beautyScoreTbls.begin(), beautyScoreTbls.end());
    }

    return result;
}

bool BeautyScoreClone::InsertOrUpdateDestBeautyScores(const std::vector<BeautyScoreTbl>& beautyScoreTbls)
{
    if (beautyScoreTbls.empty()) {
        return true;
    }

    std::vector<int32_t> fileIds;
    fileIds.reserve(beautyScoreTbls.size());
    for (const auto& tbl : beautyScoreTbls) {
        if (tbl.fileId.has_value()) {
            fileIds.push_back(tbl.fileId.value());
        }
    }

    if (fileIds.empty()) {
        return true;
    }

    std::unordered_set<int32_t> existingFileIds = QueryExistingDestFileIds(fileIds);

    std::vector<BeautyScoreTbl> toUpdate;
    std::vector<BeautyScoreTbl> toInsert;
    toUpdate.reserve(beautyScoreTbls.size());
    toInsert.reserve(beautyScoreTbls.size());

    for (const auto& tbl : beautyScoreTbls) {
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
        updateSuccess = UpdateDestBeautyScores(toUpdate);
    }

    bool insertSuccess = true;
    if (!toInsert.empty()) {
        insertSuccess = InsertNewDestBeautyScores(toInsert);
    }

    return updateSuccess && insertSuccess;
}

std::unordered_set<int32_t> BeautyScoreClone::QueryExistingDestFileIds(const std::vector<int32_t>& fileIds)
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
        const std::string querySql = "SELECT " + BEAUTY_SCORE_COL_FILE_ID +
            " FROM " + VISION_AESTHETICS_TABLE +
            " WHERE " + BEAUTY_SCORE_COL_FILE_ID + " IN (" + fileIdList + ")" +
            " AND " + BEAUTY_SCORE_COL_FILE_ID + " <= " + std::to_string(maxBeautyFileId_);

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

bool BeautyScoreClone::UpdateDestBeautyScores(const std::vector<BeautyScoreTbl>& beautyScoreTbls)
{
    if (beautyScoreTbls.empty()) {
        return true;
    }

    int32_t updatedCount = 0;
    std::vector<std::pair<NativeRdb::ValuesBucket, std::string>> updates;
    updates.reserve(beautyScoreTbls.size());

    for (const auto& tbl : beautyScoreTbls) {
        if (!tbl.fileId.has_value()) {
            continue;
        }

        NativeRdb::ValuesBucket values = CreateValuesBucketFromBeautyScoreTbl(tbl);
        const std::string whereClause = BEAUTY_SCORE_COL_FILE_ID + " = " +
            std::to_string(tbl.fileId.value()) +
            " AND " + BEAUTY_SCORE_COL_FILE_ID + " <= " + std::to_string(maxBeautyFileId_);

        updates.emplace_back(std::move(values), whereClause);
    }

    if (updates.empty()) {
        return true;
    }

    int32_t errCode = BatchUpdateWithRetry(VISION_AESTHETICS_TABLE, updates);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, false, "Failed to update beauty scores");

    updatedCount = static_cast<int32_t>(updates.size());
    migrateScoreNum_ += updatedCount;
    migrateScoreFileNumber_ += static_cast<int64_t>(updates.size());
    MEDIA_INFO_LOG("ReverseCloneBeautyScoreInfo: Updated %{public}d beauty score records", updatedCount);
    return true;
}

bool BeautyScoreClone::InsertNewDestBeautyScores(const std::vector<BeautyScoreTbl>& beautyScoreTbls)
{
    if (beautyScoreTbls.empty()) {
        return true;
    }

    std::vector<NativeRdb::ValuesBucket> valuesBucketsToInsert;
    valuesBucketsToInsert.reserve(beautyScoreTbls.size());

    for (const auto& beautyScoreTbl : beautyScoreTbls) {
        if (!beautyScoreTbl.fileId.has_value()) {
            MEDIA_WARN_LOG("BeautyScoreTbl has no fileId, skipping.");
            continue;
        }

        valuesBucketsToInsert.push_back(CreateValuesBucketFromBeautyScoreTbl(beautyScoreTbl));
    }

    if (valuesBucketsToInsert.empty()) {
        return true;
    }

    int64_t rowNum = 0;
    int32_t ret = BatchInsertWithRetry(VISION_AESTHETICS_TABLE, valuesBucketsToInsert, rowNum);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, false, "Failed to batch insert beauty scores");

    migrateScoreNum_ += rowNum;
    migrateScoreFileNumber_ += static_cast<int64_t>(beautyScoreTbls.size());
    MEDIA_INFO_LOG("ReverseCloneBeautyScoreInfo: Inserted %{public}lld new beauty score records",
        (long long)rowNum);
    return true;
}

int32_t BeautyScoreClone::BatchUpdateWithRetry(const std::string& tableName,
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
} // namespace OHOS::Media
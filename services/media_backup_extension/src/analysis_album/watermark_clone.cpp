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

#include <cstdint>
#define MLOG_TAG "MediaLibraryCloneRestoreWatermarkTbl"

#include "watermark_clone.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <optional>
#include <chrono>

#include "backup_database_utils.h"
#include "backup_const_column.h"
#include "backup_dfx_utils.h"
#include "media_file_utils.h"
#include "media_log.h"

namespace OHOS::Media {
// LCOV_EXCL_START
WaterMarkClone::WaterMarkClone(
    const std::shared_ptr<NativeRdb::RdbStore>& sourceRdb,
    const std::shared_ptr<NativeRdb::RdbStore>& destRdb,
    const std::unordered_map<int32_t, PhotoInfo>& photoInfoMap,
    const std::unordered_map<int32_t, int32_t>* reverseDupMap)
    : sourceRdb_(sourceRdb), destRdb_(destRdb), photoInfoMap_(photoInfoMap), reverseDupMap_(reverseDupMap)
{
}

bool WaterMarkClone::Clone()
{
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    if (photoInfoMap_.empty()) {
        migrateWaterMarkTotalTimeCost_ = MediaFileUtils::UTCTimeMilliSeconds() - start;
        MEDIA_INFO_LOG("photoInfoMap_ is empty, no watermark entries to clone Total time: %{public}lld ms",
            (long long)migrateWaterMarkTotalTimeCost_);
        return true;
    }

    std::vector<int32_t> oldFileIds;
    oldFileIds.reserve(photoInfoMap_.size());

    for (const auto& pair : photoInfoMap_) {
        oldFileIds.push_back(pair.first);
    }

    std::string fileIdOldInClause = "(" + BackupDatabaseUtils::JoinValues<int32_t>(oldFileIds, ", ") + ")";
    CHECK_AND_RETURN_RET_LOG(ShouldClone(fileIdOldInClause, start), true,
        "sourceRdb does not need to be cloned, timeCost: %{public}lld ms", (long long)migrateWaterMarkTotalTimeCost_);

    std::vector<std::string> commonColumns = BackupDatabaseUtils::GetCommonColumnInfos(sourceRdb_, destRdb_,
        ANALYSIS_WATERMARK_TABLE);
    migrateWaterMarkTotalTimeCost_ = MediaFileUtils::UTCTimeMilliSeconds() - start;
    CHECK_AND_RETURN_RET_LOG(!commonColumns.empty(),
        false, "No common columns found for watermark table, timeCost: %{public}lld ms",
        (long long)migrateWaterMarkTotalTimeCost_);

    for (size_t i = 0; i < oldFileIds.size(); i += QUERY_COUNT) {
        auto batch_begin = oldFileIds.begin() + i;
        auto batch_end = ((i + QUERY_COUNT < oldFileIds.size()) ?
            (oldFileIds.begin() + i + QUERY_COUNT) : oldFileIds.end());
        std::vector<int32_t> batchOldFileIds(batch_begin, batch_end);

        CHECK_AND_CONTINUE(!batchOldFileIds.empty());

        std::string fileIdOldInClause = "(" + BackupDatabaseUtils::JoinValues<int32_t>(batchOldFileIds, ", ") + ")";
        std::vector<AnalysisWaterMarkTbl> analysisWaterMarkTbls = QueryAnalysisWaterMarkTbl(
            fileIdOldInClause, commonColumns);

        CHECK_AND_CONTINUE_ERR_LOG(!analysisWaterMarkTbls.empty(),
            "Query returned empty result for batch starting at index %{public}zu", i);
        ProcessWaterMarkTbls(analysisWaterMarkTbls);
        BatchInsertWaterMark(analysisWaterMarkTbls);
    }

    migrateWaterMarkTotalTimeCost_ = MediaFileUtils::UTCTimeMilliSeconds() - start;
    MEDIA_INFO_LOG("WaterMarkClone::Clone completed. Migrated %{public}lld records."
        "Total time: %{public}lld ms", (long long)migrateWaterMarkNum_, (long long)migrateWaterMarkTotalTimeCost_);
    return true;
}

bool WaterMarkClone::ShouldClone(const std::string& fileIdOldInClause, int64_t start)
{
    std::string querySql = "SELECT count(1) AS count FROM tab_analysis_watermark WHERE file_id IN " + fileIdOldInClause;

    int32_t totalNumber = BackupDatabaseUtils::QueryInt(sourceRdb_, querySql, CUSTOM_COUNT);
    MEDIA_INFO_LOG("QueryWaterMarkNumber, totalNumber = %{public}d", totalNumber);

    if (totalNumber <= 0) {
        int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
        migrateWaterMarkTotalTimeCost_ += end - start;
        return false;
    }
    return true;
}

void WaterMarkClone::ProcessWaterMarkTbls(std::vector<AnalysisWaterMarkTbl>& waterMarkTbls)
{
    CHECK_AND_RETURN_LOG(!waterMarkTbls.empty(), "watermark tbls empty");

    auto newEnd = std::remove_if(waterMarkTbls.begin(), waterMarkTbls.end(),
        [this](const AnalysisWaterMarkTbl& tbl) {
            if (!tbl.fileId.has_value() || tbl.fileId.value() <= 0) {
                return true;
            }
            return photoInfoMap_.find(tbl.fileId.value())  == photoInfoMap_.end();
        });

    for (auto it = waterMarkTbls.begin(); it != newEnd; it++) {
            int32_t oldId = it->fileId.value();
            int32_t newId = photoInfoMap_.at(oldId).fileIdNew;
            if (newId <= 0) {
                it->fileId.reset();
                continue;
            }
            it->fileId = photoInfoMap_.at(oldId).fileIdNew;
        }

    waterMarkTbls.erase(newEnd, waterMarkTbls.end());
}

std::vector<AnalysisWaterMarkTbl> WaterMarkClone::QueryAnalysisWaterMarkTbl(
    const std::string& fileIdClause, const std::vector<std::string>& commonColumns)
{
    std::vector<AnalysisWaterMarkTbl> result;

    std::string inClause = BackupDatabaseUtils::JoinValues<std::string>(commonColumns, ", ");
    std::string querySql = "SELECT " + inClause + " FROM tab_analysis_watermark WHERE file_id IN " + fileIdClause;

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(sourceRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSql for watermark is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        AnalysisWaterMarkTbl analysisWaterMarkTbl;
        ParseAnalysisWaterMarkResultSet(resultSet, analysisWaterMarkTbl);
        result.emplace_back(analysisWaterMarkTbl);
    }

    resultSet->Close();
    return result;
}

void WaterMarkClone::ParseAnalysisWaterMarkResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
    AnalysisWaterMarkTbl& analysisWaterMarkTbl)
{
    analysisWaterMarkTbl.fileId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, WATERMARK_COL_FILE_ID);
    analysisWaterMarkTbl.status = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, WATERMARK_COL_STATUS);
    analysisWaterMarkTbl.type = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, WATERMARK_COL_TYPE);
    analysisWaterMarkTbl.valid_region_x = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        WATERMARK_COL_VALID_REGION_X);
    analysisWaterMarkTbl.valid_region_y = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        WATERMARK_COL_VALID_REGION_Y);
    analysisWaterMarkTbl.valid_region_width = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        WATERMARK_COL_VALID_REGION_WIDTH);
    analysisWaterMarkTbl.valid_region_height = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        WATERMARK_COL_VALID_REGION_HEIGHT);
    analysisWaterMarkTbl.algo_version = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        WATERMARK_COL_ALGO_VERSION);
}

void WaterMarkClone::BatchInsertWaterMark(const std::vector<AnalysisWaterMarkTbl>& waterMarkTbls)
{
    std::vector<NativeRdb::ValuesBucket> valuesBuckets;
    std::unordered_set<int32_t> fileIdSet;
    valuesBuckets.reserve(waterMarkTbls.size());
    for (const auto& waterMarkTbl : waterMarkTbls) {
        valuesBuckets.push_back(CreateValuesBucketFromWaterMarkTbl(waterMarkTbl));
    }

    CHECK_AND_RETURN_LOG(!valuesBuckets.empty(), "No valid watermark data to insert skip batch insert");
    int64_t rowNum = 0;
    int32_t ret = BatchInsertWithRetry(ANALYSIS_WATERMARK_TABLE, valuesBuckets, rowNum);
    CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to batch insert watermark records");

    for (const auto& waterMarkTbl : waterMarkTbls) {
        if (waterMarkTbl.fileId.has_value()) {
            fileIdSet.insert(waterMarkTbl.fileId.value());
        }
    }

    migrateWaterMarkNum_ += rowNum;
}

NativeRdb::ValuesBucket WaterMarkClone::CreateValuesBucketFromWaterMarkTbl(const AnalysisWaterMarkTbl& waterMarkTbl)
{
    NativeRdb::ValuesBucket values;

    PutIfPresent(values, WATERMARK_COL_FILE_ID, waterMarkTbl.fileId);
    PutIfPresent(values, WATERMARK_COL_STATUS, waterMarkTbl.status);
    PutIfPresent(values, WATERMARK_COL_TYPE, waterMarkTbl.type);
    PutIfPresent(values, WATERMARK_COL_VALID_REGION_X, waterMarkTbl.valid_region_x);
    PutIfPresent(values, WATERMARK_COL_VALID_REGION_Y, waterMarkTbl.valid_region_y);
    PutIfPresent(values, WATERMARK_COL_VALID_REGION_WIDTH, waterMarkTbl.valid_region_width);
    PutIfPresent(values, WATERMARK_COL_VALID_REGION_HEIGHT, waterMarkTbl.valid_region_height);
    PutIfPresent(values, WATERMARK_COL_ALGO_VERSION, waterMarkTbl.algo_version);

    return values;
}

int32_t WaterMarkClone::BatchInsertWithRetry(const std::string& tableName,
    std::vector<NativeRdb::ValuesBucket>& values, int64_t& rowNum)
{
    CHECK_AND_RETURN_RET(!values.empty(), E_OK);
    int32_t errCode = E_ERR;
    TransactionOperations trans{ __func__ };
    trans.SetBackupRdbStore(destRdb_);
    std::function<int(void)> func = [&]()->int {
        errCode = trans.BatchInsert(rowNum, tableName, values, NativeRdb::ConflictResolution::ON_CONFLICT_REPLACE);
        CHECK_AND_PRINT_LOG(errCode == E_OK, "InsertSql failed, errCode: %{public}d, rowNum: %{public}ld.",
            errCode, (long)rowNum);
        return errCode;
    };
    errCode = trans.RetryTrans(func, true);
    CHECK_AND_PRINT_LOG(errCode == E_OK, "BatchInsertWithRetry: tans finish fail!, ret:%{public}d", errCode);
    return errCode;
}

bool WaterMarkClone::ReverseClone()
{
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    if (photoInfoMap_.empty()) {
        migrateWaterMarkTotalTimeCost_ = MediaFileUtils::UTCTimeMilliSeconds() - start;
        MEDIA_INFO_LOG("photoInfoMap_ is empty, no watermark entries to reverse clone Total time: %{public}lld ms",
            (long long)migrateWaterMarkTotalTimeCost_);
        return true;
    }

    migrateWaterMarkNum_ = 0;
    bool success = QueryAndInsertSourceRecords();

    migrateWaterMarkTotalTimeCost_ = MediaFileUtils::UTCTimeMilliSeconds() - start;
    MEDIA_INFO_LOG("WaterMarkClone::ReverseClone completed. Migrated %{public}lld records."
        "Total time: %{public}lld ms", (long long)migrateWaterMarkNum_, (long long)migrateWaterMarkTotalTimeCost_);
    return success;
}

bool WaterMarkClone::QueryAndInsertSourceRecords()
{
    std::vector<int32_t> sourceFileIds = QuerySourceFileIds();
    CHECK_AND_RETURN_RET_LOG(!sourceFileIds.empty(), true, "No source watermark records found");

    std::vector<std::string> commonColumns = BackupDatabaseUtils::GetCommonColumnInfos(sourceRdb_, destRdb_,
        ANALYSIS_WATERMARK_TABLE);
    CHECK_AND_RETURN_RET_LOG(!commonColumns.empty(), false, "No common columns found for watermark table");

    std::vector<AnalysisWaterMarkTbl> allWaterMarkTbls;
    const size_t batchSize = QUERY_COUNT;

    for (size_t i = 0; i < sourceFileIds.size(); i += batchSize) {
        auto batch_begin = sourceFileIds.begin() + i;
        auto batch_end = ((i + batchSize < sourceFileIds.size()) ?
            (sourceFileIds.begin() + i + batchSize) : sourceFileIds.end());
        std::vector<int32_t> batchFileIds(batch_begin, batch_end);

        CHECK_AND_CONTINUE(!batchFileIds.empty());

        std::vector<AnalysisWaterMarkTbl> batchWaterMarkTbls = QuerySourceWaterMark(batchFileIds, commonColumns);
        if (!batchWaterMarkTbls.empty()) {
            allWaterMarkTbls.insert(allWaterMarkTbls.end(), batchWaterMarkTbls.begin(), batchWaterMarkTbls.end());
        }
    }

    CHECK_AND_RETURN_RET_LOG(!allWaterMarkTbls.empty(), true, "No watermark records to process");

    ProcessWaterMarkTblsForReverse(allWaterMarkTbls);
    return InsertOrUpdateDestWaterMark(allWaterMarkTbls);
}

std::vector<int32_t> WaterMarkClone::QuerySourceFileIds()
{
    std::vector<int32_t> sourceFileIds;
    sourceFileIds.reserve(photoInfoMap_.size());

    for (const auto& [sourceFileId, photoInfo] : photoInfoMap_) {
        sourceFileIds.push_back(sourceFileId);
    }

    return sourceFileIds;
}

std::vector<AnalysisWaterMarkTbl> WaterMarkClone::QuerySourceWaterMark(const std::vector<int32_t>& sourceFileIds,
    const std::vector<std::string>& commonColumns)
{
    std::vector<AnalysisWaterMarkTbl> result;

    std::string fileIdClause = "(" + BackupDatabaseUtils::JoinValues<int32_t>(sourceFileIds, ", ") + ")";
    std::string inClause = BackupDatabaseUtils::JoinValues<std::string>(commonColumns, ", ");
    std::string querySql = "SELECT " + inClause + " FROM tab_analysis_watermark WHERE file_id IN " + fileIdClause;

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(sourceRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSql for watermark is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        AnalysisWaterMarkTbl analysisWaterMarkTbl;
        ParseAnalysisWaterMarkResultSet(resultSet, analysisWaterMarkTbl);
        result.emplace_back(analysisWaterMarkTbl);
    }

    resultSet->Close();
    return result;
}

void WaterMarkClone::ProcessWaterMarkTblsForReverse(std::vector<AnalysisWaterMarkTbl>& waterMarkTbls)
{
    CHECK_AND_RETURN_LOG(!waterMarkTbls.empty(), "watermark tbls empty");

    auto newEnd = std::remove_if(waterMarkTbls.begin(), waterMarkTbls.end(),
        [this](const AnalysisWaterMarkTbl& tbl) {
            if (!tbl.fileId.has_value() || tbl.fileId.value() <= 0) {
                return true;
            }
            return photoInfoMap_.find(tbl.fileId.value()) == photoInfoMap_.end();
        });

    for (auto it = waterMarkTbls.begin(); it != newEnd; it++) {
        int32_t oldId = it->fileId.value();
        int32_t newId = photoInfoMap_.at(oldId).fileIdNew;
        if (newId <= 0) {
            it->fileId.reset();
            continue;
        }
        it->fileId = photoInfoMap_.at(oldId).fileIdNew;
    }

    waterMarkTbls.erase(newEnd, waterMarkTbls.end());
}

bool WaterMarkClone::InsertOrUpdateDestWaterMark(const std::vector<AnalysisWaterMarkTbl>& waterMarkTbls)
{
    std::vector<int32_t> destFileIds;
    destFileIds.reserve(waterMarkTbls.size());

    for (const auto& tbl : waterMarkTbls) {
        if (tbl.fileId.has_value()) {
            destFileIds.push_back(tbl.fileId.value());
        }
    }

    CHECK_AND_RETURN_RET_LOG(!destFileIds.empty(), true, "No valid fileIds to process");

    std::unordered_set<int32_t> existingDestFileIds = QueryExistingDestFileIds(destFileIds);
    std::vector<AnalysisWaterMarkTbl> toInsert;
    toInsert.reserve(waterMarkTbls.size());

    for (const auto& tbl : waterMarkTbls) {
        if (!tbl.fileId.has_value()) {
            continue;
        }

        int32_t fileIdNew = tbl.fileId.value();
        bool shouldInsert = ProcessWaterMarkRecord(tbl, fileIdNew, existingDestFileIds);
        if (shouldInsert) {
            toInsert.push_back(tbl);
        }
    }

    if (!toInsert.empty()) {
        return InsertNewDestWaterMark(toInsert);
    }

    return true;
}

bool WaterMarkClone::ProcessWaterMarkRecord(const AnalysisWaterMarkTbl& tbl, int32_t fileIdNew,
    const std::unordered_set<int32_t>& existingDestFileIds)
{
    if (reverseDupMap_ == nullptr) {
        return true;
    }

    auto it = reverseDupMap_->find(fileIdNew);
    if (it == reverseDupMap_->end()) {
        return true;
    }

    int32_t dupFileId = it->second;
    if (existingDestFileIds.find(dupFileId) == existingDestFileIds.end()) {
        return true;
    }

    if (!UpdateDestWaterMarkFileId(dupFileId, fileIdNew)) {
        MEDIA_ERR_LOG("Failed to update watermark file_id from %{public}d to %{public}d",
            dupFileId, fileIdNew);
    } else {
        migrateWaterMarkNum_++;
    }

    return false;
}

std::unordered_set<int32_t> WaterMarkClone::QueryExistingDestFileIds(const std::vector<int32_t>& destFileIds)
{
    std::unordered_set<int32_t> existingFileIds;

    std::string fileIdClause = "(" + BackupDatabaseUtils::JoinValues<int32_t>(destFileIds, ", ") + ")";
    std::string querySql = "SELECT DISTINCT file_id FROM tab_analysis_watermark WHERE file_id IN " + fileIdClause;

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(destRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, existingFileIds, "Query resultSql for existing fileIds is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = 0;
        if (resultSet->GetInt(0, fileId) == NativeRdb::E_OK && fileId > 0) {
            existingFileIds.insert(fileId);
        }
    }

    resultSet->Close();
    return existingFileIds;
}

bool WaterMarkClone::UpdateDestWaterMarkFileId(int32_t oldFileId, int32_t newFileId)
{
    int32_t errCode = E_ERR;
    TransactionOperations trans{ __func__ };
    trans.SetBackupRdbStore(destRdb_);
    std::function<int(void)> func = [&]()->int {
        std::string updateSql = "UPDATE tab_analysis_watermark SET file_id = " + std::to_string(newFileId) +
            " WHERE file_id = " + std::to_string(oldFileId);
        errCode = destRdb_->ExecuteSql(updateSql);
        CHECK_AND_PRINT_LOG(errCode == E_OK, "Update watermark file_id failed, errCode: %{public}d", errCode);
        return errCode;
    };
    errCode = trans.RetryTrans(func, true);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, false,
        "UpdateDestWaterMarkFileId: transaction failed!, ret:%{public}d", errCode);
    return true;
}

bool WaterMarkClone::InsertNewDestWaterMark(const std::vector<AnalysisWaterMarkTbl>& waterMarkTbls)
{
    std::vector<NativeRdb::ValuesBucket> valuesBuckets;
    valuesBuckets.reserve(waterMarkTbls.size());

    for (const auto& waterMarkTbl : waterMarkTbls) {
        valuesBuckets.push_back(CreateValuesBucketFromWaterMarkTbl(waterMarkTbl));
    }

    CHECK_AND_RETURN_RET_LOG(!valuesBuckets.empty(), true, "No valid watermark data to insert");

    int64_t rowNum = 0;
    int32_t ret = BatchInsertWithRetry(ANALYSIS_WATERMARK_TABLE, valuesBuckets, rowNum);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, false, "Failed to batch insert watermark records");

    migrateWaterMarkNum_ += rowNum;
    return true;
}
}
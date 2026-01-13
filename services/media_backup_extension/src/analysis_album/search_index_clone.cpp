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

#define MLOG_TAG "MediaLibraryCloneRestoreSearchTbl"

#include "search_index_clone.h"

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
// LCOV_EXCL_START
SearchIndexClone::SearchIndexClone(
    const std::shared_ptr<NativeRdb::RdbStore>& sourceRdb,
    const std::shared_ptr<NativeRdb::RdbStore>& destRdb,
    const std::unordered_map<int32_t, PhotoInfo>& photoInfoMap,
    int64_t maxSearchId)
    : sourceRdb_(sourceRdb), destRdb_(destRdb), photoInfoMap_(photoInfoMap), maxSearchId_(maxSearchId)
{
}

bool SearchIndexClone::Clone()
{
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    std::vector<int32_t> oldFileIds;
    oldFileIds.reserve(photoInfoMap_.size());
    for (const auto& pair : photoInfoMap_) {
        oldFileIds.push_back(pair.first);
    }

    if (oldFileIds.empty()) {
        MEDIA_INFO_LOG("photoInfoMap_ is empty, no search index entries to clone.");
        totalTimeCost_ = MediaFileUtils::UTCTimeMilliSeconds() - start;
        return true;
    }

    std::vector<std::string> exclusions(EXCLUDED_ANALYSIS_SEARCH_IDX_COLS.begin(),
        EXCLUDED_ANALYSIS_SEARCH_IDX_COLS.end());
    std::vector<std::string> commonColumn = BackupDatabaseUtils::GetCommonColumnInfos(sourceRdb_, destRdb_,
        ANALYSIS_SEARCH_INDEX_TABLE);
    std::vector<std::string> commonColumns = BackupDatabaseUtils::filterColumns(commonColumn, exclusions);
    CHECK_AND_RETURN_RET_LOG(!commonColumns.empty(),
        false, "No common columns found for search index table after exclusion.");

    for (size_t i = 0; i < oldFileIds.size(); i += SQL_BATCH_SIZE) {
        auto batch_begin = oldFileIds.begin() + i;
        auto batch_end = ((i + SQL_BATCH_SIZE < oldFileIds.size()) ?
            (oldFileIds.begin() + i + SQL_BATCH_SIZE) : oldFileIds.end());
        std::vector<int32_t> batchOldFileIds(batch_begin, batch_end);

        if (batchOldFileIds.empty()) {
            continue;
        }

        std::string fileIdOldInClause = "(" + BackupDatabaseUtils::JoinValues<int>(batchOldFileIds, ", ") + ")";
        std::vector<AnalysisSearchIndexTbl> analysisSearchIndexTbl = QueryAnalysisSearchIndexTbl(
            fileIdOldInClause, commonColumns);

        if (analysisSearchIndexTbl.empty()) {
            MEDIA_WARN_LOG("Query returned empty result for batch starting at index %{public}zu", i);
            continue;
        }
        auto searchIdxTbl = ProcessSearchIndexTbls(analysisSearchIndexTbl);
        InsertAnalysisSearchIndex(searchIdxTbl);
    }

    totalTimeCost_ = MediaFileUtils::UTCTimeMilliSeconds() - start;
    MEDIA_INFO_LOG("SearchIndexClone::Clone completed. Migrated %{public}lld records. "
        "Total time: %{public}lld ms", (long long)migratedCount_, (long long)totalTimeCost_);
    return true;
}

std::vector<AnalysisSearchIndexTbl> SearchIndexClone::ProcessSearchIndexTbls(
    const std::vector<AnalysisSearchIndexTbl>& searchIndexTbls)
{
    CHECK_AND_RETURN_RET_LOG(!searchIndexTbls.empty(), {}, "search index tbls empty");

    std::vector<AnalysisSearchIndexTbl> newSearchIndexTbls;
    newSearchIndexTbls.reserve(searchIndexTbls.size());

    for (const auto& tbl : searchIndexTbls) {
        if (tbl.fileId.has_value()) {
            const int32_t oldFileId = tbl.fileId.value();
            const auto it = photoInfoMap_.find(oldFileId);
            if (it != photoInfoMap_.end()) {
                AnalysisSearchIndexTbl updatedTbl = tbl;
                updatedTbl.fileId = it->second.fileIdNew;
                newSearchIndexTbls.push_back(std::move(updatedTbl));
            }
        }
    }

    return newSearchIndexTbls;
}

std::vector<AnalysisSearchIndexTbl> SearchIndexClone::QueryAnalysisSearchIndexTbl(
    const std::string &fileIdClause, const std::vector<std::string>& commonColumns)
{
    std::vector<AnalysisSearchIndexTbl> result;

    std::string inClause = BackupDatabaseUtils::JoinValues<std::string>(commonColumns, ", ");
    std::string querySql = "SELECT " + inClause + " FROM " + ANALYSIS_SEARCH_INDEX_TABLE;
    querySql += " WHERE " + SEARCH_IDX_COL_FILE_ID + " IN " + fileIdClause;

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(sourceRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSql for search index is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        AnalysisSearchIndexTbl analysisSearchIndexTbl;
        ParseAnalysisSearchIndexResultSet(resultSet, analysisSearchIndexTbl);
        result.emplace_back(analysisSearchIndexTbl);
    }

    resultSet->Close();
    return result;
}

void SearchIndexClone::ParseAnalysisSearchIndexResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
    AnalysisSearchIndexTbl& analysisSearchIndexTbl)
{
    analysisSearchIndexTbl.id = BackupDatabaseUtils::GetOptionalValue<int64_t>(resultSet, SEARCH_IDX_COL_ID);
    analysisSearchIndexTbl.fileId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, SEARCH_IDX_COL_FILE_ID);
    analysisSearchIndexTbl.data = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, SEARCH_IDX_COL_DATA);
    analysisSearchIndexTbl.displayName = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        SEARCH_IDX_COL_DISPLAY_NAME);
    analysisSearchIndexTbl.latitude = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, SEARCH_IDX_COL_LATITUDE);
    analysisSearchIndexTbl.longitude = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        SEARCH_IDX_COL_LONGITUDE);
    analysisSearchIndexTbl.dateModified = BackupDatabaseUtils::GetOptionalValue<int64_t>(resultSet,
        SEARCH_IDX_COL_DATE_MODIFIED);
    analysisSearchIndexTbl.version = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, SEARCH_IDX_COL_VERSION);
    analysisSearchIndexTbl.systemLanguage = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        SEARCH_IDX_COL_SYSTEM_LANGUAGE);
}

void SearchIndexClone::InsertAnalysisSearchIndex(std::vector<AnalysisSearchIndexTbl>& analysisSearchIndexTbl)
{
    CHECK_AND_RETURN_LOG(destRdb_ != nullptr, "destRdb_ is null for search index insert");
    CHECK_AND_RETURN_LOG(!analysisSearchIndexTbl.empty(), "analysisSearchIndexTbl vector is empty");

    std::vector<int32_t> fileIdsToProcess;
    fileIdsToProcess.reserve(analysisSearchIndexTbl.size());
    for (const auto& entry : analysisSearchIndexTbl) {
        if (entry.fileId.has_value()) {
            fileIdsToProcess.push_back(entry.fileId.value());
        }
    }
    if (fileIdsToProcess.empty()) {
        return;
    }

    auto [protectedIds, overrideIds] = QueryExistingIdsWithStrategy(fileIdsToProcess);
    std::vector<int32_t> overrideDeleteIds;
    std::vector<AnalysisSearchIndexTbl> finalInsertEntries;
    for (const auto& entry : analysisSearchIndexTbl) {
        if (!entry.fileId.has_value()) continue;
        const int32_t fileId = entry.fileId.value();
        if (protectedIds.count(fileId)) {
            continue;
        }
        if (overrideIds.count(fileId)) {
            overrideDeleteIds.push_back(fileId);
        }
        finalInsertEntries.push_back(entry);
    }
    if (!overrideDeleteIds.empty()) {
        DeleteOverrideRecords(overrideDeleteIds);
    }
    if (!finalInsertEntries.empty()) {
        int32_t insertedRowNum = InsertSearchIndexByTable(finalInsertEntries);
        CHECK_AND_PRINT_LOG(insertedRowNum != E_ERR, "Failed to insert search index batch");
        migratedCount_ += insertedRowNum;
    }
}

IdSetPair SearchIndexClone::QueryExistingIdsWithStrategy(const std::vector<int32_t>& fileIds)
{
    std::unordered_set<int32_t> protectedIds;
    std::unordered_set<int32_t> overrideIds;

    for (size_t i = 0; i < fileIds.size(); i += SQL_BATCH_SIZE) {
        auto batch_begin = fileIds.begin() + i;
        auto batch_end = (i + SQL_BATCH_SIZE < fileIds.size()) ? (fileIds.begin() + i + SQL_BATCH_SIZE) : fileIds.end();
        std::vector<int32_t> batchFileIds(batch_begin, batch_end);

        if (batchFileIds.empty()) {
            continue;
        }

        const std::string fileIdList = BackupDatabaseUtils::JoinValues(batchFileIds, ", ");

        const std::string protectedQuery = "SELECT " + SEARCH_IDX_COL_FILE_ID +
            " FROM " + ANALYSIS_SEARCH_INDEX_TABLE +
            " WHERE " + SEARCH_IDX_COL_FILE_ID + " IN (" + fileIdList + ")" +
            " AND " + SEARCH_IDX_COL_ID + " <= " + std::to_string(maxSearchId_);
        auto protectedSetBatch = ExecuteIdQuery(protectedQuery);
        if (!protectedSetBatch.empty()) {
            protectedIds.insert(protectedSetBatch.begin(), protectedSetBatch.end());
        }

        const std::string overrideQuery = "SELECT " + SEARCH_IDX_COL_FILE_ID +
            " FROM " + ANALYSIS_SEARCH_INDEX_TABLE +
            " WHERE " + SEARCH_IDX_COL_FILE_ID + " IN (" + fileIdList + ")" +
            " AND " + SEARCH_IDX_COL_ID + " > " + std::to_string(maxSearchId_);
        auto overrideSetBatch = ExecuteIdQuery(overrideQuery);
        if (!overrideSetBatch.empty()) {
            overrideIds.insert(overrideSetBatch.begin(), overrideSetBatch.end());
        }
    }

    return {protectedIds, overrideIds};
}

std::unordered_set<int32_t> SearchIndexClone::ExecuteIdQuery(const std::string& querySql)
{
    std::unordered_set<int32_t> result;
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(destRdb_, querySql);
    CHECK_AND_RETURN_RET(resultSet != nullptr, result);

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = 0;
        if (resultSet->GetInt(0, fileId) == NativeRdb::E_OK) {
            result.insert(fileId);
        }
    }
    resultSet->Close();
    return result;
}

void SearchIndexClone::DeleteOverrideRecords(const std::vector<int32_t>& fileIds)
{
    CHECK_AND_RETURN_LOG(!fileIds.empty(), "fileIds vector is empty for deletion");

    for (size_t i = 0; i < fileIds.size(); i += SQL_BATCH_SIZE) {
        auto batch_begin = fileIds.begin() + i;
        auto batch_end = (i + SQL_BATCH_SIZE < fileIds.size()) ? (fileIds.begin() + i + SQL_BATCH_SIZE) : fileIds.end();
        std::vector<int32_t> batchFileIds(batch_begin, batch_end);

        if (batchFileIds.empty()) {
            continue;
        }

        const std::string fileIdList = BackupDatabaseUtils::JoinValues(batchFileIds, ", ");
        const std::string deleteSql =
            "DELETE FROM " + ANALYSIS_SEARCH_INDEX_TABLE +
            " WHERE " + SEARCH_IDX_COL_FILE_ID + " IN (" + fileIdList + ")" +
            " AND " + SEARCH_IDX_COL_ID + " > " + std::to_string(maxSearchId_);

        BackupDatabaseUtils::ExecuteSQL(destRdb_, deleteSql);
    }
}

int32_t SearchIndexClone::InsertSearchIndexByTable(std::vector<AnalysisSearchIndexTbl>& analysisSearchIndexTbl)
{
    std::vector<NativeRdb::ValuesBucket> valuesBuckets = GetInsertSearchIndexValues(analysisSearchIndexTbl);
    CHECK_AND_RETURN_RET(destRdb_ != nullptr, E_ERR);
    CHECK_AND_RETURN_RET(!valuesBuckets.empty(), 0);

    int64_t rowNum = 0;
    int32_t ret {0};
    ret = BatchInsertWithRetry(ANALYSIS_SEARCH_INDEX_TABLE, valuesBuckets, rowNum);
    CHECK_AND_RETURN_RET(ret == E_OK, E_ERR);
    return static_cast<int32_t>(rowNum);
}

std::vector<NativeRdb::ValuesBucket> SearchIndexClone::GetInsertSearchIndexValues(std::vector<AnalysisSearchIndexTbl>&
    analysisSearchIndexTbl)
{
    std::vector<NativeRdb::ValuesBucket> values;
    values.reserve(analysisSearchIndexTbl.size());
    for (auto& searchIndexInfo : analysisSearchIndexTbl) {
        NativeRdb::ValuesBucket value = GetInsertSearchIndexValue(searchIndexInfo);
        values.emplace_back(value);
    }

    return values;
}

NativeRdb::ValuesBucket SearchIndexClone::GetInsertSearchIndexValue(const AnalysisSearchIndexTbl& searchIndexInfo)
{
    NativeRdb::ValuesBucket values;

    PutIfPresent(values, SEARCH_IDX_COL_FILE_ID, searchIndexInfo.fileId);
    PutIfPresent(values, SEARCH_IDX_COL_DATA, searchIndexInfo.data);
    PutIfPresent(values, SEARCH_IDX_COL_DISPLAY_NAME, searchIndexInfo.displayName);
    PutIfPresent(values, SEARCH_IDX_COL_LATITUDE, searchIndexInfo.latitude);
    PutIfPresent(values, SEARCH_IDX_COL_LONGITUDE, searchIndexInfo.longitude);
    PutWithDefault<long>(values, SEARCH_IDX_COL_DATE_MODIFIED, searchIndexInfo.dateModified, 0L);
    PutWithDefault<int>(values, SEARCH_IDX_COL_PHOTO_STATUS, searchIndexInfo.photoStatus, 0);
    PutWithDefault<int>(values, SEARCH_IDX_COL_CV_STATUS, searchIndexInfo.cvStatus, 0);
    PutWithDefault<int>(values, SEARCH_IDX_COL_GEO_STATUS, searchIndexInfo.geoStatus, 0);
    PutWithDefault<int>(values, SEARCH_IDX_COL_VERSION, searchIndexInfo.version, 0);
    PutIfPresent(values, SEARCH_IDX_COL_SYSTEM_LANGUAGE, searchIndexInfo.systemLanguage);

    return values;
}

int32_t SearchIndexClone::BatchInsertWithRetry(const std::string &tableName,
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
// LCOV_EXCL_STOP
} // namespace OHOS::Media
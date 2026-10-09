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

#define MLOG_TAG "BeautyScoreCloneBase"

#include "beauty_score_clone_base.h"

#include "backup_database_utils.h"
#include "backup_const_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdb_transaction.h"

namespace OHOS::Media {
// LCOV_EXCL_START
bool BeautyScoreCloneBase::ExecuteClone()
{
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    std::vector<int32_t> fileIds = GetFileIdsForQuery();
    if (fileIds.empty()) {
        migrateScoreTotalTimeCost_ = MediaFileUtils::UTCTimeMilliSeconds() - start;
        MEDIA_INFO_LOG("fileIds is empty, no beauty score entries to clone, time: %{public}lld ms",
            (long long)migrateScoreTotalTimeCost_);
        return true;
    }

    std::vector<std::string> commonColumn = BackupDatabaseUtils::GetCommonColumnInfos(
        GetSourceRdb(), GetTargetRdb(), VISION_AESTHETICS_TABLE);
    std::vector<std::string> commonColumns = BackupDatabaseUtils::FilterExcludedColumns(commonColumn,
        EXCLUDED_BEAUTY_SCORE_COLUMNS);
    CHECK_AND_RETURN_RET_LOG(!commonColumns.empty(), false,
        "No common columns found for aesthetics score table after exclusion.");

    std::unordered_set<int32_t> allInsertedFileIds;
    for (size_t i = 0; i < fileIds.size(); i += SQL_BATCH_SIZE) {
        auto batchFileIds = GetBatchFileIds(fileIds, i);
        std::string fileIdInClause = BuildFileIdInClause(batchFileIds);

        if (ShouldSkipClone(fileIdInClause)) {
            continue;
        }

        std::vector<BeautyScoreTbl> beautyScoreTbls = QueryBeautyScoreTbl(batchFileIds, commonColumns);
        CHECK_AND_CONTINUE(!beautyScoreTbls.empty());

        ProcessBeautyScoreTbls(beautyScoreTbls);
        std::vector<BeautyScoreTbl> filteredScores = FilterByMaxId(beautyScoreTbls);
        CHECK_AND_CONTINUE(!filteredScores.empty());

        std::unordered_set<int32_t> existingFileIds = QueryExistingFileIds(batchFileIds);
        std::vector<int32_t> fileIdsToDelete;
        for (const auto& tbl : filteredScores) {
            if (tbl.fileId.has_value() && existingFileIds.count(tbl.fileId.value())) {
                fileIdsToDelete.push_back(tbl.fileId.value());
            }
        }
        if (!fileIdsToDelete.empty()) {
            DeleteExistingRecords(fileIdsToDelete);
        }

        std::unordered_set<int32_t> insertedFileIds = BatchInsertBeautyScores(filteredScores);
        allInsertedFileIds.insert(insertedFileIds.begin(), insertedFileIds.end());
    }

    migrateScoreTotalTimeCost_ = MediaFileUtils::UTCTimeMilliSeconds() - start;
    MEDIA_INFO_LOG("BeautyScoreClone completed. Migrated %{public}lld records, %{public}lld files, "
        "time: %{public}lld ms", (long long)migrateScoreNum_, (long long)migrateScoreFileNumber_,
        (long long)migrateScoreTotalTimeCost_);
    return true;
}

std::vector<BeautyScoreTbl> BeautyScoreCloneBase::QueryBeautyScoreTbl(
    const std::vector<int32_t>& fileIds, const std::vector<std::string>& commonColumns)
{
    std::vector<BeautyScoreTbl> result;
    std::string inClause = BackupDatabaseUtils::JoinValues<std::string>(commonColumns, ", ");
    std::string fileIdInClause = BuildFileIdInClause(fileIds);
    std::string querySql = "SELECT " + inClause + " FROM " + VISION_AESTHETICS_TABLE +
        " WHERE " + BEAUTY_SCORE_COL_FILE_ID + " IN " + fileIdInClause;

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(GetSourceRdb(), querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSet is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        BeautyScoreTbl tbl;
        ParseBeautyScoreResultSet(resultSet, tbl);
        result.emplace_back(tbl);
    }
    resultSet->Close();
    return result;
}

void BeautyScoreCloneBase::ParseBeautyScoreResultSet(
    const std::shared_ptr<NativeRdb::ResultSet>& resultSet, BeautyScoreTbl& tbl)
{
    tbl.id = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, BEAUTY_SCORE_COL_ID);
    tbl.fileId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, BEAUTY_SCORE_COL_FILE_ID);
    tbl.aestheticsScore = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, BEAUTY_SCORE_COL_AESTHETICS_SCORE);
    tbl.aestheticsVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        BEAUTY_SCORE_COL_AESTHETICS_VERSION);
    tbl.prob = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, BEAUTY_SCORE_COL_PROB);
    tbl.analysisVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        BEAUTY_SCORE_COL_ANALYSIS_VERSION);
    tbl.selectedFlag = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, BEAUTY_SCORE_COL_SELECTED_FLAG);
    tbl.selectedAlgoVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        BEAUTY_SCORE_COL_SELECTED_ALGO_VERSION);
    tbl.selectedStatus = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, BEAUTY_SCORE_COL_SELECTED_STATUS);
    tbl.negativeFlag = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, BEAUTY_SCORE_COL_NEGATIVE_FLAG);
    tbl.negativeAlgoVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        BEAUTY_SCORE_COL_NEGATIVE_ALGO_VERSION);
    tbl.aestheticsAllVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        BEAUTY_SCORE_COL_AESTHETICS_ALL_VERSION);
    tbl.aestheticsScoreAll = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        BEAUTY_SCORE_COL_AESTHETICS_SCORE_ALL);
    tbl.isFilteredHard = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, BEAUTY_SCORE_COL_IS_FILTERED_HARD);
    tbl.clarityScoreAll = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, BEAUTY_SCORE_COL_CLARITY_SCORE_ALL);
    tbl.saturationScoreAll = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        BEAUTY_SCORE_COL_SATURATION_SCORE_ALL);
    tbl.luminanceScoreAll = BackupDatabaseUtils::GetOptionalValue<double>(resultSet,
        BEAUTY_SCORE_COL_LUMINANCE_SCORE_ALL);
    tbl.semanticsScore = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, BEAUTY_SCORE_COL_SEMANTICS_SCORE);
    tbl.isBlackWhiteStripe = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        BEAUTY_SCORE_COL_IS_BLACK_WHITE_STRIPE);
    tbl.isBlurry = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, BEAUTY_SCORE_COL_IS_BLURRY);
    tbl.isMosaic = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, BEAUTY_SCORE_COL_IS_MOSAIC);
}

std::unordered_set<int32_t> BeautyScoreCloneBase::BatchInsertBeautyScores(std::vector<BeautyScoreTbl>& beautyScoreTbls)
{
    std::vector<NativeRdb::ValuesBucket> valuesBucketsToInsert;
    std::unordered_set<int32_t> fileIdsNewlyInserted;
    valuesBucketsToInsert.reserve(beautyScoreTbls.size());

    for (const auto& tbl : beautyScoreTbls) {
        if (!tbl.fileId.has_value() || tbl.fileId.value() <= 0) {
            continue;
        }
        valuesBucketsToInsert.push_back(CreateValuesBucketFromBeautyScoreTbl(tbl));
        fileIdsNewlyInserted.insert(tbl.fileId.value());
    }

    if (valuesBucketsToInsert.empty()) {
        return fileIdsNewlyInserted;
    }

    int64_t rowNum = 0;
    int32_t ret = BatchInsertWithRetry(VISION_AESTHETICS_TABLE, valuesBucketsToInsert, rowNum);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, fileIdsNewlyInserted, "Failed to batch insert aesthetics scores");

    migrateScoreNum_ += rowNum;
    migrateScoreFileNumber_ += static_cast<int64_t>(fileIdsNewlyInserted.size());
    return fileIdsNewlyInserted;
}

NativeRdb::ValuesBucket BeautyScoreCloneBase::CreateValuesBucketFromBeautyScoreTbl(const BeautyScoreTbl& tbl)
{
    NativeRdb::ValuesBucket values;
    PutIfPresent(values, BEAUTY_SCORE_COL_ID, tbl.id);
    PutIfPresent(values, BEAUTY_SCORE_COL_FILE_ID, tbl.fileId);
    PutIfPresent(values, BEAUTY_SCORE_COL_AESTHETICS_SCORE, tbl.aestheticsScore);
    PutIfPresent(values, BEAUTY_SCORE_COL_AESTHETICS_VERSION, tbl.aestheticsVersion);
    PutIfPresent(values, BEAUTY_SCORE_COL_PROB, tbl.prob);
    PutIfPresent(values, BEAUTY_SCORE_COL_ANALYSIS_VERSION, tbl.analysisVersion);
    PutIfPresent(values, BEAUTY_SCORE_COL_SELECTED_FLAG, tbl.selectedFlag);
    PutIfPresent(values, BEAUTY_SCORE_COL_SELECTED_ALGO_VERSION, tbl.selectedAlgoVersion);
    PutIfPresent(values, BEAUTY_SCORE_COL_SELECTED_STATUS, tbl.selectedStatus);
    PutIfPresent(values, BEAUTY_SCORE_COL_NEGATIVE_FLAG, tbl.negativeFlag);
    PutIfPresent(values, BEAUTY_SCORE_COL_NEGATIVE_ALGO_VERSION, tbl.negativeAlgoVersion);
    PutIfPresent(values, BEAUTY_SCORE_COL_AESTHETICS_ALL_VERSION, tbl.aestheticsAllVersion);
    PutWithDefault<int32_t>(values, BEAUTY_SCORE_COL_AESTHETICS_SCORE_ALL, tbl.aestheticsScoreAll, 0);
    PutWithDefault<int32_t>(values, BEAUTY_SCORE_COL_IS_FILTERED_HARD, tbl.isFilteredHard, 0);
    PutWithDefault<double>(values, BEAUTY_SCORE_COL_CLARITY_SCORE_ALL, tbl.clarityScoreAll, 0);
    PutWithDefault<double>(values, BEAUTY_SCORE_COL_SATURATION_SCORE_ALL, tbl.saturationScoreAll, 0);
    PutWithDefault<double>(values, BEAUTY_SCORE_COL_LUMINANCE_SCORE_ALL, tbl.luminanceScoreAll, 0);
    PutWithDefault<double>(values, BEAUTY_SCORE_COL_SEMANTICS_SCORE, tbl.semanticsScore, 0);
    PutWithDefault<int32_t>(values, BEAUTY_SCORE_COL_IS_BLACK_WHITE_STRIPE, tbl.isBlackWhiteStripe, 0);
    PutWithDefault<int32_t>(values, BEAUTY_SCORE_COL_IS_BLURRY, tbl.isBlurry, 0);
    PutWithDefault<int32_t>(values, BEAUTY_SCORE_COL_IS_MOSAIC, tbl.isMosaic, 0);
    return values;
}

int32_t BeautyScoreCloneBase::BatchInsertWithRetry(const std::string& tableName,
    std::vector<NativeRdb::ValuesBucket>& values, int64_t& rowNum)
{
    CHECK_AND_RETURN_RET(!values.empty(), E_OK);
    int32_t errCode = E_ERR;
    TransactionOperations trans{ __func__ };
    trans.SetBackupRdbStore(GetTargetRdb());
    std::function<int(void)> func = [&]()->int {
        errCode = trans.BatchInsert(rowNum, tableName, values);
        CHECK_AND_PRINT_LOG(errCode == E_OK, "InsertSql failed, errCode: %{public}d, rowNum: %{public}ld.",
            errCode, (long)rowNum);
        return errCode;
    };
    errCode = trans.RetryTrans(func, true);
    return errCode;
}

std::string BeautyScoreCloneBase::BuildFileIdInClause(const std::vector<int32_t>& fileIds)
{
    return "(" + BackupDatabaseUtils::JoinValues<int32_t>(fileIds, ", ") + ")";
}

std::vector<int32_t> BeautyScoreCloneBase::GetBatchFileIds(const std::vector<int32_t>& fileIds, size_t offset)
{
    auto batch_begin = fileIds.begin() + offset;
    auto batch_end = ((offset + SQL_BATCH_SIZE < fileIds.size()) ?
        (fileIds.begin() + offset + SQL_BATCH_SIZE) : fileIds.end());
    return std::vector<int32_t>(batch_begin, batch_end);
}
// LCOV_EXCL_STOP
} // namespace OHOS::Media
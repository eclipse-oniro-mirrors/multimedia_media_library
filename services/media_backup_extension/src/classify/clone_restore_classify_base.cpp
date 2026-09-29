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

#define MLOG_TAG "CloneRestoreClassifyBase"

#include "clone_restore_classify_base.h"

#include "backup_database_utils.h"
#include "media_log.h"
#include "result_set_utils.h"
#include "medialibrary_rdb_transaction.h"

namespace OHOS::Media {
// LCOV_EXCL_START
void CloneRestoreClassifyBase::ParseClassifyAlbumResultSet(
    ClassifyAlbumInfo &info,
    std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
    info.albumId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "album_id");
    info.albumName = BackupDatabaseUtils::GetOptionalValue<string>(resultSet, "album_name");
    info.albumType = BackupDatabaseUtils::GetOptionalValue<string>(resultSet, "album_type");
    info.albumSubType = BackupDatabaseUtils::GetOptionalValue<string>(resultSet, "album_subtype");
}

void CloneRestoreClassifyBase::GetAnalysisAlbumInsertValue(NativeRdb::ValuesBucket &value,
    const ClassifyAlbumInfo &info)
{
    value.Put("album_id", info.albumId.value());
    value.Put("album_name", info.albumName.value());
    value.Put("album_type", info.albumType.value());
    value.Put("album_subtype", info.albumSubType.value());
}

void CloneRestoreClassifyBase::GetClassifyInfoFromResultSet(
    ClassifyCloneInfo &info,
    std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
    CHECK_AND_RETURN(resultSet != nullptr);
    info.id = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, ID);
    info.fileIdOld = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, FILE_ID);
    info.categoryId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, CATEGORY_ID);
    info.subLabel = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, SUB_LABEL);
    info.prob = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, PROB);
    info.feature = BackupDatabaseUtils::GetOptionalValue<std::vector<uint8_t>>(resultSet, FEATURE);
    info.simResult = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, SIM_RESULT);
    info.labelVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, LABEL_VERSION);
    info.saliencySubProb = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, SALIENCY_SUB_PROB);
    info.analysisVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ANALYSIS_VERSION);
    info.captionResult = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, CAPTION_RESULT);
    info.captionVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, CAPTION_VERSION);
    info.significanceScore = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, SIGNIFICANCE_SCORE);
    info.significanceScoreVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet,
        SIGNIFICANCE_SCORE_VERSION);
}

void CloneRestoreClassifyBase::GetClassifyVideoInfoFromResultSet(
    ClassifyVideoCloneInfo &info,
    std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
    CHECK_AND_RETURN(resultSet != nullptr);
    info.id = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, ID);
    info.fileIdOld = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, FILE_ID);
    info.categoryId = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, CATEGORY_ID);
    info.confidenceProbability = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, CONFIDENCE_PROBABILITY);
    info.subCategory = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, SUB_CATEGORY);
    info.subConfidenceProb = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, SUB_CONFIDENCE_PROB);
    info.subLabel = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, SUB_LABEL);
    info.subLabelProb = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, SUB_LABEL_PROB);
    info.subLabelType = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, SUB_LABEL_TYPE);
    info.tracks = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, TRACKS);
    info.videoPartFeature = BackupDatabaseUtils::GetOptionalValue<std::vector<uint8_t>>(resultSet, VIDEO_PART_FEATURE);
    info.filterTag = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, FILTER_TAG);
    info.algoVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ALGO_VERSION);
    info.analysisVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ANALYSIS_VERSION);
    info.triggerGenerateThumbnail = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        TRIGGER_GENERATE_THUMBNAIL);
}

void CloneRestoreClassifyBase::UpdateScoreMask(int32_t fileId, uint32_t mask)
{
    if (externalScoreMaskMap_ == nullptr) {
        return;
    }
    (*externalScoreMaskMap_)[fileId] |= mask;
}

std::unordered_set<std::string> CloneRestoreClassifyBase::GetCommonColumns(
    const std::string &tableName)
{
    auto srcColumnInfoMap = BackupDatabaseUtils::GetColumnInfoMap(mediaLibraryRdb_, tableName);
    auto dstColumnInfoMap = BackupDatabaseUtils::GetColumnInfoMap(mediaRdb_, tableName);
    std::unordered_set<std::string> result;

    for (auto it = dstColumnInfoMap.begin(); it != dstColumnInfoMap.end(); ++it) {
        if (srcColumnInfoMap.find(it->first) != srcColumnInfoMap.end()) {
            result.insert(it->first);
        }
    }
    return result;
}

void CloneRestoreClassifyBase::GetMapInsertValue(
    NativeRdb::ValuesBucket &value,
    ClassifyCloneInfo &info,
    const std::unordered_set<std::string> &intersection)
{
    PutIfInIntersection(value, FILE_ID, info.fileIdOld, intersection);
    PutIfInIntersection(value, CATEGORY_ID, info.categoryId, intersection);
    PutIfInIntersection(value, SUB_LABEL, info.subLabel, intersection);
    PutIfInIntersection(value, PROB, info.prob, intersection);
    PutIfInIntersection(value, FEATURE, info.feature, intersection);
    PutIfInIntersection(value, SIM_RESULT, info.simResult, intersection);
    PutIfInIntersection(value, LABEL_VERSION, info.labelVersion, intersection);
    PutIfInIntersection(value, SALIENCY_SUB_PROB, info.saliencySubProb, intersection);
    PutIfInIntersection(value, ANALYSIS_VERSION, info.analysisVersion, intersection);
    PutIfInIntersection(value, CAPTION_RESULT, info.captionResult, intersection);
    PutIfInIntersection(value, CAPTION_VERSION, info.captionVersion, intersection);
    PutIfInIntersection(value, SIGNIFICANCE_SCORE, info.significanceScore, intersection);
    PutIfInIntersection(value, SIGNIFICANCE_SCORE_VERSION, info.significanceScoreVersion, intersection);
}

void CloneRestoreClassifyBase::GetVideoMapInsertValue(
    NativeRdb::ValuesBucket &value,
    const ClassifyVideoCloneInfo &info,
    const std::unordered_set<std::string> &intersection)
{
    PutIfInIntersection(value, FILE_ID, info.fileIdOld, intersection);
    PutIfInIntersection(value, CATEGORY_ID, info.categoryId, intersection);
    PutIfInIntersection(value, CONFIDENCE_PROBABILITY, info.confidenceProbability, intersection);
    PutIfInIntersection(value, SUB_CATEGORY, info.subCategory, intersection);
    PutIfInIntersection(value, SUB_CONFIDENCE_PROB, info.subConfidenceProb, intersection);
    PutIfInIntersection(value, SUB_LABEL, info.subLabel, intersection);
    PutIfInIntersection(value, SUB_LABEL_PROB, info.subLabelProb, intersection);
    PutIfInIntersection(value, SUB_LABEL_TYPE, info.subLabelType, intersection);
    PutIfInIntersection(value, TRACKS, info.tracks, intersection);
    PutIfInIntersection(value, VIDEO_PART_FEATURE, info.videoPartFeature, intersection);
    PutIfInIntersection(value, FILTER_TAG, info.filterTag, intersection);
    PutIfInIntersection(value, ALGO_VERSION, info.algoVersion, intersection);
    PutIfInIntersection(value, ANALYSIS_VERSION, info.analysisVersion, intersection);
    PutIfInIntersection(value, TRIGGER_GENERATE_THUMBNAIL, info.triggerGenerateThumbnail, intersection);
}

bool CloneRestoreClassifyBase::CheckTableColumns(const std::string &tableName,
    std::unordered_map<std::string, std::string> &columns)
{
    columns = BackupDatabaseUtils::GetColumnInfoMap(mediaRdb_, tableName);
    return !columns.empty();
}

int32_t CloneRestoreClassifyBase::BatchInsertWithRetry(const std::string &tableName,
    std::vector<NativeRdb::ValuesBucket> &values,
    int64_t &rowNum,
    std::shared_ptr<NativeRdb::RdbStore> rdbStore)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore is nullptr");
    TransactionOperations trans{ __func__ };
    trans.SetBackupRdbStore(rdbStore);
    std::function<int(void)> func = [&]()->int {
        return trans.BatchInsert(rowNum, tableName, values);
    };
    return trans.RetryTrans(func, true);
}
// LCOV_EXCL_STOP
} // namespace OHOS::Media
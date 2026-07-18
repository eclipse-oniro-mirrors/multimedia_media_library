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
    // 新旧机file_id一致
    if (intersection.count(FILE_ID) > 0 && info.fileIdOld.has_value()) {
        value.PutInt(FILE_ID, info.fileIdOld.value());
    }
    if (intersection.count(CATEGORY_ID) > 0 && info.categoryId.has_value()) {
        value.PutInt(CATEGORY_ID, info.categoryId.value());
    }
    if (intersection.count(SUB_LABEL) > 0 && info.subLabel.has_value()) {
        value.PutString(SUB_LABEL, info.subLabel.value());
    }
    if (intersection.count(PROB) > 0 && info.prob.has_value()) {
        value.PutDouble(PROB, info.prob.value());
    }
    if (intersection.count(FEATURE) > 0 && info.feature.has_value()) {
        value.PutBlob(FEATURE, info.feature.value());
    }
    if (intersection.count(SIM_RESULT) > 0 && info.simResult.has_value()) {
        value.PutString(SIM_RESULT, info.simResult.value());
    }
    if (intersection.count(LABEL_VERSION) > 0 && info.labelVersion.has_value()) {
        value.PutString(LABEL_VERSION, info.labelVersion.value());
    }
    if (intersection.count(SALIENCY_SUB_PROB) > 0 && info.saliencySubProb.has_value()) {
        value.PutString(SALIENCY_SUB_PROB, info.saliencySubProb.value());
    }
    if (intersection.count(ANALYSIS_VERSION) > 0 && info.analysisVersion.has_value()) {
        value.PutString(ANALYSIS_VERSION, info.analysisVersion.value());
    }
    if (intersection.count(CAPTION_RESULT) > 0 && info.captionResult.has_value()) {
        value.PutString(CAPTION_RESULT, info.captionResult.value());
    }
    if (intersection.count(CAPTION_VERSION) > 0 && info.captionVersion.has_value()) {
        value.PutString(CAPTION_VERSION, info.captionVersion.value());
    }
    if (intersection.count(SIGNIFICANCE_SCORE) > 0 && info.significanceScore.has_value()) {
        value.PutInt(SIGNIFICANCE_SCORE, info.significanceScore.value());
    }
    if (intersection.count(SIGNIFICANCE_SCORE_VERSION) > 0 &&
        info.significanceScoreVersion.has_value()) {
        value.PutString(SIGNIFICANCE_SCORE_VERSION, info.significanceScoreVersion.value());
    }
}

void CloneRestoreClassifyBase::GetVideoMapInsertValue(
    NativeRdb::ValuesBucket &value,
    const ClassifyVideoCloneInfo &info,
    const std::unordered_set<std::string> &intersection)
{
    if (intersection.count(FILE_ID) > 0 && info.fileIdOld.has_value()) {
        value.PutInt(FILE_ID, info.fileIdOld.value());
    }
    if (intersection.count(CATEGORY_ID) > 0 && info.categoryId.has_value()) {
        value.PutString(CATEGORY_ID, info.categoryId.value());
    }
    if (intersection.count(CONFIDENCE_PROBABILITY) > 0 && info.confidenceProbability.has_value()) {
        value.PutString(CONFIDENCE_PROBABILITY, info.confidenceProbability.value());
    }
    if (intersection.count(SUB_CATEGORY) > 0 && info.subCategory.has_value()) {
        value.PutString(SUB_CATEGORY, info.subCategory.value());
    }
    if (intersection.count(SUB_CONFIDENCE_PROB) > 0 && info.subConfidenceProb.has_value()) {
        value.PutString(SUB_CONFIDENCE_PROB, info.subConfidenceProb.value());
    }
    if (intersection.count(SUB_LABEL) > 0 && info.subLabel.has_value()) {
        value.PutString(SUB_LABEL, info.subLabel.value());
    }
    if (intersection.count(SUB_LABEL_PROB) > 0 && info.subLabelProb.has_value()) {
        value.PutString(SUB_LABEL_PROB, info.subLabelProb.value());
    }
    if (intersection.count(SUB_LABEL_TYPE) > 0 && info.subLabelType.has_value()) {
        value.PutString(SUB_LABEL_TYPE, info.subLabelType.value());
    }
    if (intersection.count(TRACKS) > 0 && info.tracks.has_value()) {
        value.PutString(TRACKS, info.tracks.value());
    }
    if (intersection.count(VIDEO_PART_FEATURE) > 0 && info.videoPartFeature.has_value()) {
        value.PutBlob(VIDEO_PART_FEATURE, info.videoPartFeature.value());
    }
    if (intersection.count(FILTER_TAG) > 0 && info.filterTag.has_value()) {
        value.PutString(FILTER_TAG, info.filterTag.value());
    }
    if (intersection.count(ALGO_VERSION) > 0 && info.algoVersion.has_value()) {
        value.PutString(ALGO_VERSION, info.algoVersion.value());
    }
    if (intersection.count(ANALYSIS_VERSION) > 0 && info.analysisVersion.has_value()) {
        value.PutString(ANALYSIS_VERSION, info.analysisVersion.value());
    }
    if (intersection.count(TRIGGER_GENERATE_THUMBNAIL) > 0 &&
        info.triggerGenerateThumbnail.has_value()) {
        value.PutInt(TRIGGER_GENERATE_THUMBNAIL, info.triggerGenerateThumbnail.value());
    }
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
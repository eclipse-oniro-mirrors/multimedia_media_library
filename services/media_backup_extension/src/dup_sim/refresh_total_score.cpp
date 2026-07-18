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

#define MLOG_TAG "MediaLibraryRefreshTotalScore"

#include "media_log.h"
#include "refresh_total_score.h"
#include "backup_database_utils.h"
#include "media_file_utils.h"
#include "vision_column.h"

using namespace std;
namespace OHOS::Media {
const std::string LABEL_TABLE = "tab_analysis_label";
const std::string IMAGE_FACE_TABLE = "tab_analysis_image_face";
const std::string AESTHETICS_SCORE_TABLE = "tab_analysis_aesthetics_score";
const std::string AFFECTIVE_TABLE = "tab_analysis_affective";
const std::string PROFILE_TABLE = "tab_analysis_profile";
const std::string DEDUP_TABLE = "tab_analysis_dedup";
const uint32_t BIT20 = 1u << 20;  // 刷新状态标记

void RefreshTotalScore::Init(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb, const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap,
    const std::unordered_map<int32_t, uint32_t> &scoreMaskMap, int64_t shouldEndTime)
{
    MEDIA_INFO_LOG("RefreshTotalScore Init");
    this->mediaLibraryRdb_ = mediaLibraryRdb;
    this->mediaRdb_ = mediaRdb;
    this->photoInfoMap_ = photoInfoMap;
    this->scoreMaskMap_ = scoreMaskMap;
    this->shouldEndTime_ = shouldEndTime;
}

void RefreshTotalScore::Refresh()
{
    MEDIA_INFO_LOG("Start Refresh");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    CHECK_AND_RETURN_LOG(mediaRdb_ != nullptr && mediaLibraryRdb_ != nullptr, "rdbStore is nullptr.");
    CHECK_AND_RETURN_LOG(shouldEndTime_ == 0 || start <= shouldEndTime_, "over shouldEndTime, skip Refresh");

    int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
    CHECK_AND_RETURN_LOG(shouldEndTime_ == 0 || currentTime <= shouldEndTime_,
        "over shouldEndTime, Refresh cost: %{public}lld",
        (long long)(currentTime - start));

    CalculateCloneTotalScore();

    currentTime = MediaFileUtils::UTCTimeMilliSeconds();
    CHECK_AND_RETURN_LOG(shouldEndTime_ == 0 || currentTime <= shouldEndTime_,
        "over shouldEndTime, after CalculateCloneTotalScore cost: %{public}lld",
        (long long)(currentTime - start));

    MergeWithDestinationTotalScore();

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("Refresh total cost %{public}lld ms", (long long)(end - start));
}

void RefreshTotalScore::CalculateCloneTotalScore()
{
    MEDIA_INFO_LOG("CalculateCloneTotalScore");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    CHECK_AND_RETURN_LOG(mediaRdb_ != nullptr, "rdbStore is nullptr.");

    std::string queryTotalSql = "SELECT file_id, total_score FROM " + VISION_TOTAL_TABLE;
    auto totalResultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, queryTotalSql);
    CHECK_AND_RETURN_LOG(totalResultSet != nullptr, "Query total table failed.");

    cloneTotalScores_.clear();
    while (totalResultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t oldFileId = BackupDatabaseUtils::GetOptionalValue<int32_t>(totalResultSet, "file_id").value_or(0);
        int32_t totalScore = BackupDatabaseUtils::GetOptionalValue<int32_t>(totalResultSet, "total_score").value_or(0);

        CHECK_AND_CONTINUE(oldFileId > 0);

        // 将旧file_id映射到新file_id
        auto it = photoInfoMap_.find(oldFileId);
        CHECK_AND_CONTINUE(it != photoInfoMap_.end() && it->second.fileIdNew != -1);

        int32_t newFileId = it->second.fileIdNew;
        // 用新file_id去匹配scoreMaskMap_
        auto maskIt = scoreMaskMap_.find(newFileId);
        CHECK_AND_CONTINUE(maskIt != scoreMaskMap_.end());

        uint32_t mask = maskIt->second;
        uint32_t cloneScore = static_cast<uint32_t>(totalScore) & mask;

        // 对于标记位（如BIT20），如果mask中有，直接设置，不进行&运算
        if (mask & BIT20) {
            cloneScore |= BIT20;
        }

        cloneTotalScores_[newFileId] = static_cast<int32_t>(cloneScore);
    }
    totalResultSet->Close();

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("CalculateCloneTotalScore cost %{public}lld, processed: %{public}zu",
        (long long)(end - start),
        cloneTotalScores_.size());
}

void RefreshTotalScore::MergeWithDestinationTotalScore()
{
    MEDIA_INFO_LOG("MergeWithDestinationTotalScore");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    CHECK_AND_RETURN_LOG(mediaLibraryRdb_ != nullptr, "mediaLibraryRdb_ is nullptr.");

    std::string queryTotalSql = "SELECT file_id, total_score FROM " + VISION_TOTAL_TABLE;
    auto totalResultSet = BackupDatabaseUtils::GetQueryResultSet(mediaLibraryRdb_, queryTotalSql);
    CHECK_AND_RETURN_LOG(totalResultSet != nullptr, "Query total table failed.");

    std::unordered_map<int32_t, int32_t> destTotalScores;
    while (totalResultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = BackupDatabaseUtils::GetOptionalValue<int32_t>(totalResultSet, "file_id").value_or(0);
        int32_t totalScore = BackupDatabaseUtils::GetOptionalValue<int32_t>(totalResultSet, "total_score").value_or(0);

        CHECK_AND_CONTINUE(fileId > 0);
        destTotalScores[fileId] = totalScore;
    }
    totalResultSet->Close();

    std::string updateSql = "UPDATE " + VISION_TOTAL_TABLE + " SET total_score = ? WHERE file_id = ?";
    for (const auto &[newFileId, cloneScore] : cloneTotalScores_) {
        auto destIt = destTotalScores.find(newFileId);
        if (destIt != destTotalScores.end()) {
            // 合并克隆分数和目标库分数
            uint32_t mergedScore = static_cast<uint32_t>(cloneScore) | static_cast<uint32_t>(destIt->second);
            std::vector<NativeRdb::ValueObject> bindArgs = {static_cast<int32_t>(mergedScore), newFileId};
            mediaLibraryRdb_->ExecuteSql(updateSql, bindArgs);
        }
    }

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("MergeWithDestinationTotalScore cost %{public}lld", (long long)(end - start));
}
}  // namespace OHOS::Media
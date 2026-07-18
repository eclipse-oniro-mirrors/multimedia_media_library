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

#ifndef CLONE_RESTORE_DUP_SIM_H
#define CLONE_RESTORE_DUP_SIM_H

#include <string>
#include <optional>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <atomic>
#include <cstdint>

#include "backup_const.h"
#include "backup_file_utils.h"
#include "rdb_store.h"
#include "refresh_total_score.h"

namespace OHOS::Media {
struct ProfileInfo {
    std::optional<int32_t> fileId;
    std::optional<std::vector<uint8_t>> fingerprint;
    std::optional<std::string> fingerprintVersion;
    std::optional<std::vector<uint8_t>> neuralhashValue;
    std::optional<std::string> neuralhashVersion;
    std::optional<int32_t> totalScore;
    std::optional<std::string> totalScoreVersion;
    std::optional<int32_t> faceScore;
    std::optional<std::string> faceScoreVersion;
    std::optional<int32_t> isDocument;
    std::optional<int32_t> isNegative;
    std::optional<int32_t> personalizationScore;
    std::optional<std::string> personalizationScoreVersion;
};

struct DedupInfo {
    std::optional<int32_t> fileId;
    std::optional<int32_t> groupIdRep;
    std::optional<std::string> dedupGroupVersion;
    std::optional<int32_t> groupIdSim;
    std::optional<std::string> simGroupVersion;
};

struct AffectiveInfo {
    std::optional<int32_t> id;
    std::optional<int32_t> fileId;
    std::optional<std::string> emotionCategory;
    std::optional<int32_t> valence;
    std::optional<int32_t> arousal;
    std::optional<std::string> modelVersion;
    std::optional<std::string> modelName;
    std::optional<std::string> extra;
    std::optional<int64_t> timestamp;
    std::optional<std::string> analysisVersion;
    std::optional<int32_t> affectiveScore;
    std::optional<std::string> affectiveScoreVersion;
};

class CloneRestoreDupSim {
public:
    void Init(int32_t sceneCode, const std::string &taskId,
        std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
        std::shared_ptr<NativeRdb::RdbStore> mediaRdb,
        const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap, bool isCloudRestoreSatisfied,
        std::unordered_map<int32_t, uint32_t>* scoreMaskMap = nullptr);
    void Restore();
    void RefreshTotalScore();

private:
    void RestoreProfileData();
    void RestoreDedupData();
    void RestoreAffectiveData();
    void UpdateScoreMask(int32_t fileId, uint32_t mask);
    void PreprocessSourceTotalTable();
    void UpdateTotalTableForProfile();
    void UpdateTotalTableForDedup();
    void UpdateTotalTableForAffective();
    void UpdateSimilarityAndDuplicateFields();
    std::vector<int32_t> ConvertNewFileIdsToOldFileIds(const std::vector<int32_t> &newFileIds);
    void UpdateTotalTableField(const std::string &field, const std::vector<int32_t> &insertedFileIds);
    std::vector<std::pair<int32_t, int32_t>> QueryFieldValuesFromSource(
        const std::string &field, const std::vector<int32_t> &oldFileIds);
    void BatchUpdateTotalTableField(const std::string &field,
        const std::vector<std::pair<int32_t, int32_t>> &fieldValues);
    std::vector<ProfileInfo> QueryProfileTblByFileIds(const std::string &fileIdClause);
    void BatchInsertProfileData(const std::vector<ProfileInfo> &profileInfos,
        const std::unordered_set<int32_t> &existingFileIds);
    NativeRdb::ValuesBucket CreateValuesBucketFromProfileInfo(const ProfileInfo &info);
    std::vector<DedupInfo> QueryDedupTblByFileIds(const std::string &fileIdClause);
    void BatchInsertDedupData(const std::vector<DedupInfo> &dedupInfos,
        const std::unordered_set<int32_t> &existingFileIds);
    NativeRdb::ValuesBucket CreateValuesBucketFromDedupInfo(const DedupInfo &info);
    std::vector<AffectiveInfo> QueryAffectiveTblByFileIds(const std::string &fileIdClause);
    void BatchInsertAffectiveData(const std::vector<AffectiveInfo> &affectiveInfos,
        const std::unordered_set<int32_t> &existingFileIds);
    NativeRdb::ValuesBucket CreateValuesBucketFromAffectiveInfo(const AffectiveInfo &info);
    int32_t BatchInsertWithRetry(
        const std::string &tableName, std::vector<NativeRdb::ValuesBucket> &values, int64_t &rowNum);
    int64_t GetShouldEndTime();

private:
    int32_t sceneCode_;
    std::string taskId_;
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb_;
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;
    std::unordered_map<int32_t, PhotoInfo> photoInfoMap_;
    bool isCloudRestoreSatisfied_;
    std::unordered_map<int32_t, uint32_t> scoreMaskMap_;
    std::unordered_map<int32_t, uint32_t>* externalScoreMaskMap_ = nullptr;
    std::unordered_map<int32_t, int32_t> newToOldMap_;  // 新file_id到旧file_id的缓存映射
    std::atomic<uint64_t> migrateProfileNumber_{0};
    std::atomic<uint64_t> migrateDedupNumber_{0};
    std::atomic<uint64_t> migrateAffectiveNumber_{0};
    std::vector<int32_t> insertedProfileFileIds_;   // 记录 Profile 表实际插入的 file_id
    std::vector<int32_t> insertedAffectiveFileIds_; // 记录 Affective 表实际插入的 file_id
};
}  // namespace OHOS::Media
#endif  // CLONE_RESTORE_DUP_SIM_H
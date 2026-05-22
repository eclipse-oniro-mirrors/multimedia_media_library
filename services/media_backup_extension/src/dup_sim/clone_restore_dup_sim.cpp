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

#define MLOG_TAG "MediaLibraryCloneRestoreDupSim"

#include <unordered_set>
#include "media_log.h"
#include "clone_restore_dup_sim.h"
#include "backup_database_utils.h"
#include "backup_dfx_utils.h"
#include "media_file_utils.h"
#include "upgrade_restore_task_report.h"
#include "media_column.h"
#include "medialibrary_type_const.h"
#include "backup_const_column.h"
#include "vision_column.h"
#include "vision_total_column.h"
#include "medialibrary_data_manager_utils.h"
#include "backup_const.h"
#include "clone_restore_analysis_total.h"

using namespace std;
namespace OHOS::Media {
const std::string PROFILE_TABLE = "tab_analysis_profile";
const std::string DEDUP_TABLE = "tab_analysis_dedup";
const std::string AFFECTIVE_TABLE = "tab_analysis_affective";

// Score mask bit definitions
const uint32_t BIT3 = 1u << 3;    // 情感分
const uint32_t BIT20 = 1u << 20;  // 刷新状态标记

// 超时控制配置
const int64_t THRESHOLD_DATA_SIZE = 30000;   // 3万条数据阈值
const int64_t THRESHOLD_DATA_TIME = 600000;  // 10分钟 (不超过3万条时的基线)
const int64_t DEFAULT_FAULT_TIME = 0;
const int64_t BASIC_NUMBER = 10000;                      // 1万条
const int64_t SUPPORT_NUMBER = 9999;                     // 余量
const int64_t SINGLE_OVER_THRESHOLD_DATA_TIME = 216000;  // 216秒 (每1万条数据)

static int32_t GetNewFileId(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap, int32_t oldFileId);
static bool UpdateGroupId(DedupInfo &mappedInfo, const std::optional<int32_t> &groupId,
    const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap);

void CloneRestoreDupSim::Init(int32_t sceneCode, const std::string &taskId,
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, std::shared_ptr<NativeRdb::RdbStore> mediaRdb,
    const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap, bool isCloudRestoreSatisfied,
    std::unordered_map<int32_t, uint32_t> *scoreMaskMap)
{
    MEDIA_INFO_LOG("CloneRestoreDupSim Init");
    this->sceneCode_ = sceneCode;
    this->taskId_ = taskId;
    this->mediaRdb_ = mediaRdb;
    this->mediaLibraryRdb_ = mediaLibraryRdb;
    this->photoInfoMap_ = photoInfoMap;
    this->isCloudRestoreSatisfied_ = isCloudRestoreSatisfied;
    this->externalScoreMaskMap_ = scoreMaskMap;

    // 构建新file_id到旧file_id的哈希映射表缓存
    newToOldMap_.reserve(photoInfoMap_.size());
    for (const auto &pair : photoInfoMap_) {
        if (pair.second.fileIdNew != -1) {
            newToOldMap_[pair.second.fileIdNew] = pair.first;
        }
    }
}

int64_t CloneRestoreDupSim::GetShouldEndTime()
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

void CloneRestoreDupSim::UpdateScoreMask(int32_t fileId, uint32_t mask)
{
    if (externalScoreMaskMap_ != nullptr) {
        (*externalScoreMaskMap_)[fileId] |= mask;
    } else {
        scoreMaskMap_[fileId] |= mask;
    }
}

void CloneRestoreDupSim::Restore()
{
    MEDIA_INFO_LOG("Start Restore");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t shouldEndTime = GetShouldEndTime();
    CHECK_AND_RETURN_LOG(start <= shouldEndTime, "over shouldEndTime, skip Restore");

    CHECK_AND_RETURN_LOG(mediaRdb_ != nullptr && mediaLibraryRdb_ != nullptr, "rdbStore is nullptr.");

    // Preprocess source total table: set similarity and duplicate to 1 if > 0
    PreprocessSourceTotalTable();
    RestoreProfileData();
    RestoreDedupData();
    RestoreAffectiveData();
    RefreshTotalScore();

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("Restore total cost %{public}lld ms", (long long)(end - start));
}
void CloneRestoreDupSim::RefreshTotalScore()
{
    MEDIA_INFO_LOG("RefreshTotalScore");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t shouldEndTime = GetShouldEndTime();

    CHECK_AND_RETURN_LOG(mediaRdb_ != nullptr && mediaLibraryRdb_ != nullptr, "rdbStore is nullptr.");
    class RefreshTotalScore refreshTotalScore;
    auto &scoreMaskMapToUse = (externalScoreMaskMap_ != nullptr) ? *externalScoreMaskMap_ : scoreMaskMap_;
    refreshTotalScore.Init(mediaLibraryRdb_, mediaRdb_, photoInfoMap_, scoreMaskMapToUse, shouldEndTime);
    refreshTotalScore.Refresh();

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("RefreshTotalScore cost %{public}lld ms", (long long)(end - start));
}

std::vector<ProfileInfo> CloneRestoreDupSim::QueryProfileTblByFileIds(const std::string &fileIdClause)
{
    std::vector<ProfileInfo> result;
    std::string querySql = "SELECT file_id, fingerprint, fingerprint_version, neuralhash_value, "
                           "neuralhash_version, total_score, total_score_version, face_score, face_score_version, "
                           "is_document, is_negative, personalization_score, personalization_score_version "
                           " FROM " +
                           PROFILE_TABLE;
    querySql += " WHERE file_id IN " + fileIdClause;

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSet is null.");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        ProfileInfo info;
        info.fileId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "file_id");
        info.fingerprint = BackupDatabaseUtils::GetOptionalValue<std::vector<uint8_t>>(resultSet, "fingerprint");
        info.fingerprintVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "fingerprint_version");
        info.neuralhashValue =
            BackupDatabaseUtils::GetOptionalValue<std::vector<uint8_t>>(resultSet, "neuralhash_value");
        info.neuralhashVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "neuralhash_version");
        info.totalScore = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "total_score");
        info.totalScoreVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "total_score_version");
        info.faceScore = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "face_score");
        info.faceScoreVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "face_score_version");
        info.isDocument = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "is_document");
        info.isNegative = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "is_negative");
        info.personalizationScore = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "personalization_score");
        info.personalizationScoreVersion =
            BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "personalization_score_version");
        result.emplace_back(info);
    }
    resultSet->Close();
    return result;
}

static std::unordered_set<int32_t> GetExistingFileIds(
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, const std::string &tableName)
{
    std::unordered_set<int32_t> existingFileIds;
    std::string queryExistingSql = "SELECT file_id FROM " + tableName;
    auto existingResultSet = BackupDatabaseUtils::GetQueryResultSet(mediaLibraryRdb, queryExistingSql);
    if (existingResultSet != nullptr) {
        while (existingResultSet->GoToNextRow() == NativeRdb::E_OK) {
            int32_t fileId = BackupDatabaseUtils::GetOptionalValue<int32_t>(existingResultSet, "file_id").value_or(0);
            if (fileId > 0) {
                existingFileIds.insert(fileId);
            }
        }
        existingResultSet->Close();
    }
    return existingFileIds;
}

void CloneRestoreDupSim::BatchInsertProfileData(
    const std::vector<ProfileInfo> &profileInfos, const std::unordered_set<int32_t> &existingFileIds)
{
    CHECK_AND_RETURN_LOG(mediaLibraryRdb_ != nullptr, "mediaLibraryRdb_ is null");
    CHECK_AND_RETURN_LOG(!profileInfos.empty(), "profileInfos are empty");

    int32_t skipNoFileId = 0;
    int32_t skipNoMapping = 0;
    int32_t skipExisting = 0;
    int32_t toInsert = 0;
    std::vector<NativeRdb::ValuesBucket> valuesBuckets;
    for (const auto &info : profileInfos) {
        if (!info.fileId.has_value()) {
            skipNoFileId++;
            continue;
        }
        int32_t newFileId = GetNewFileId(photoInfoMap_, info.fileId.value());
        if (newFileId == -1) {
            skipNoMapping++;
            continue;
        }
        if (existingFileIds.find(newFileId) != existingFileIds.end()) {
            skipExisting++;
            continue;
        }
        toInsert++;
        ProfileInfo mappedInfo = info;
        mappedInfo.fileId = newFileId;
        valuesBuckets.push_back(CreateValuesBucketFromProfileInfo(mappedInfo));
        insertedProfileFileIds_.push_back(newFileId);
    }

    if (!valuesBuckets.empty()) {
        int64_t rowNum = 0;
        int32_t ret = BatchInsertWithRetry(PROFILE_TABLE, valuesBuckets, rowNum);
        CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to batch insert profile data");
        migrateProfileNumber_ += static_cast<uint64_t>(rowNum);
    }
}

NativeRdb::ValuesBucket CloneRestoreDupSim::CreateValuesBucketFromProfileInfo(const ProfileInfo &info)
{
    NativeRdb::ValuesBucket values;

    BackupDatabaseUtils::PutIfPresent(values, "file_id", info.fileId);
    BackupDatabaseUtils::PutIfPresent(values, "fingerprint", info.fingerprint);
    BackupDatabaseUtils::PutIfPresent(values, "fingerprint_version", info.fingerprintVersion);
    BackupDatabaseUtils::PutIfPresent(values, "neuralhash_value", info.neuralhashValue);
    BackupDatabaseUtils::PutIfPresent(values, "neuralhash_version", info.neuralhashVersion);
    BackupDatabaseUtils::PutIfPresent(values, "total_score", info.totalScore);
    BackupDatabaseUtils::PutIfPresent(values, "total_score_version", info.totalScoreVersion);
    BackupDatabaseUtils::PutIfPresent(values, "face_score", info.faceScore);
    BackupDatabaseUtils::PutIfPresent(values, "face_score_version", info.faceScoreVersion);
    BackupDatabaseUtils::PutIfPresent(values, "is_document", info.isDocument);
    BackupDatabaseUtils::PutIfPresent(values, "is_negative", info.isNegative);
    BackupDatabaseUtils::PutIfPresent(values, "personalization_score", info.personalizationScore);
    BackupDatabaseUtils::PutIfPresent(values, "personalization_score_version", info.personalizationScoreVersion);

    return values;
}

void CloneRestoreDupSim::RestoreDedupData()
{
    MEDIA_INFO_LOG("RestoreDedupData");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t shouldEndTime = GetShouldEndTime();

    if (photoInfoMap_.empty()) {
        MEDIA_INFO_LOG("photoInfoMap_ is empty, no dedup data to restore.");
        return;
    }

    // 在循环外只查询一次existingFileIds，避免每批都查询全表
    std::unordered_set<int32_t> existingFileIds = GetExistingFileIds(mediaLibraryRdb_, DEDUP_TABLE);

    // 收集所有 oldFileIds
    std::vector<int32_t> oldFileIds;
    oldFileIds.reserve(photoInfoMap_.size());
    for (const auto &pair : photoInfoMap_) {
        oldFileIds.push_back(pair.first);
    }
    // 分批查询和插入
    for (size_t i = 0; i < oldFileIds.size(); i += QUERY_COUNT) {
        auto batch_begin = oldFileIds.begin() + i;
        auto batch_end =
            ((i + QUERY_COUNT < oldFileIds.size()) ? (oldFileIds.begin() + i + QUERY_COUNT) : oldFileIds.end());
        std::vector<int32_t> batchOldFileIds(batch_begin, batch_end);
        CHECK_AND_CONTINUE(!batchOldFileIds.empty());
        int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
        CHECK_AND_RETURN_LOG(currentTime <= shouldEndTime,
            "over shouldEndTime, RestoreDedupData cost: %{public}lld",
            (long long)(currentTime - start));

        std::string fileIdOldInClause = "(" + BackupDatabaseUtils::JoinValues<int>(batchOldFileIds, ", ") + ")";
        std::vector<DedupInfo> dedupInfos = QueryDedupTblByFileIds(fileIdOldInClause);
        if (dedupInfos.empty()) {
            MEDIA_WARN_LOG("Query returned empty result for batch starting at index %{public}zu", i);
            continue;
        }
        MEDIA_INFO_LOG("RestoreDedupData: batch index = %{public}zu, queried = %{public}zu", i, dedupInfos.size());
        BatchInsertDedupData(dedupInfos, existingFileIds);
    }

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("RestoreDedupData completed, migrateDedupNum %{public}llu records, Total time: %{public}lld ms",
        (unsigned long long)migrateDedupNumber_.load(),
        (long long)(end - start));
    UpdateTotalTableForDedup();
}

void CloneRestoreDupSim::RestoreAffectiveData()
{
    MEDIA_INFO_LOG("RestoreAffectiveData");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t shouldEndTime = GetShouldEndTime();

    if (photoInfoMap_.empty()) {
        MEDIA_INFO_LOG("photoInfoMap_ is empty, no affective data to restore.");
        return;
    }
    // 在循环外只查询一次existingFileIds
    std::unordered_set<int32_t> existingFileIds = GetExistingFileIds(mediaLibraryRdb_, AFFECTIVE_TABLE);

    // 收集所有 oldFileIds
    std::vector<int32_t> oldFileIds;
    oldFileIds.reserve(photoInfoMap_.size());
    for (const auto &pair : photoInfoMap_) {
        oldFileIds.push_back(pair.first);
    }
    // 分批查询和插入
    for (size_t i = 0; i < oldFileIds.size(); i += QUERY_COUNT) {
        auto batch_begin = oldFileIds.begin() + i;
        auto batch_end =
            ((i + QUERY_COUNT < oldFileIds.size()) ? (oldFileIds.begin() + i + QUERY_COUNT) : oldFileIds.end());
        std::vector<int32_t> batchOldFileIds(batch_begin, batch_end);
        CHECK_AND_CONTINUE(!batchOldFileIds.empty());
        int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
        CHECK_AND_RETURN_LOG(currentTime <= shouldEndTime,
            "over shouldEndTime, RestoreAffectiveData cost: %{public}lld",
            (long long)(currentTime - start));

        std::string fileIdOldInClause = "(" + BackupDatabaseUtils::JoinValues<int>(batchOldFileIds, ", ") + ")";
        std::vector<AffectiveInfo> affectiveInfos = QueryAffectiveTblByFileIds(fileIdOldInClause);
        if (affectiveInfos.empty()) {
            MEDIA_WARN_LOG("Query returned empty result for batch starting at index %{public}zu", i);
            continue;
        }

        MEDIA_INFO_LOG(
            "RestoreAffectiveData: batch index = %{public}zu, queried = %{public}zu", i, affectiveInfos.size());
        BatchInsertAffectiveData(affectiveInfos, existingFileIds);
    }

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG(
        "RestoreAffectiveData completed, migrateAffectiveNum %{public}llu records, Total time: %{public}lld ms",
        (unsigned long long)migrateAffectiveNumber_.load(),
        (long long)(end - start));
    UpdateTotalTableForAffective();
}

void CloneRestoreDupSim::RestoreProfileData()
{
    MEDIA_INFO_LOG("RestoreProfileData");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t shouldEndTime = GetShouldEndTime();

    if (photoInfoMap_.empty()) {
        MEDIA_INFO_LOG("photoInfoMap_ is empty, no profile data to restore.");
        return;
    }
    // 在循环外只查询一次existingFileIds，避免每批都查询全表
    std::unordered_set<int32_t> existingFileIds = GetExistingFileIds(mediaLibraryRdb_, PROFILE_TABLE);

    // 收集所有 oldFileIds
    std::vector<int32_t> oldFileIds;
    oldFileIds.reserve(photoInfoMap_.size());
    for (const auto &pair : photoInfoMap_) {
        oldFileIds.push_back(pair.first);
    }
    // 分批查询和插入
    for (size_t i = 0; i < oldFileIds.size(); i += QUERY_COUNT) {
        auto batch_begin = oldFileIds.begin() + i;
        auto batch_end =
            ((i + QUERY_COUNT < oldFileIds.size()) ? (oldFileIds.begin() + i + QUERY_COUNT) : oldFileIds.end());
        std::vector<int32_t> batchOldFileIds(batch_begin, batch_end);
        CHECK_AND_CONTINUE(!batchOldFileIds.empty());
        int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
        CHECK_AND_RETURN_LOG(currentTime <= shouldEndTime,
            "over shouldEndTime, RestoreProfileData cost: %{public}lld",
            (long long)(currentTime - start));

        std::string fileIdOldInClause = "(" + BackupDatabaseUtils::JoinValues<int>(batchOldFileIds, ", ") + ")";
        std::vector<ProfileInfo> profileInfos = QueryProfileTblByFileIds(fileIdOldInClause);
        if (profileInfos.empty()) {
            MEDIA_WARN_LOG("Query returned empty result for batch starting at index %{public}zu", i);
            continue;
        }

        MEDIA_INFO_LOG("RestoreProfileData: batch index = %{public}zu, queried = %{public}zu", i, profileInfos.size());
        BatchInsertProfileData(profileInfos, existingFileIds);
    }

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("RestoreProfileData completed, migrateProfileNum %{public}llu records, Total time: %{public}lld ms",
        (unsigned long long)migrateProfileNumber_.load(),
        (long long)(end - start));

    // 记录需要刷新的分数类型的 mask 值
    // Profile 表需要全量记录所有源端 file_id（包含重复不克隆的部分）的 BIT20
    MEDIA_INFO_LOG("record bit20 of mask for profile");
    for (const auto &pair : photoInfoMap_) {
        if (pair.second.fileIdNew != -1) {
            UpdateScoreMask(pair.second.fileIdNew, BIT20);
        }
    }
    UpdateTotalTableForProfile();
}

std::vector<DedupInfo> CloneRestoreDupSim::QueryDedupTblByFileIds(const std::string &fileIdClause)
{
    std::vector<DedupInfo> result;
    std::string querySql = "SELECT file_id, group_id_rep, dedup_group_version, group_id_sim, sim_group_version "
                           " FROM " +
                           DEDUP_TABLE;
    querySql += " WHERE file_id IN " + fileIdClause;

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSet is null.");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        DedupInfo info;
        info.fileId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "file_id");
        info.groupIdRep = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "group_id_rep");
        info.dedupGroupVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "dedup_group_version");
        info.groupIdSim = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "group_id_sim");
        info.simGroupVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "sim_group_version");
        result.emplace_back(info);
    }
    resultSet->Close();
    return result;
}

static int32_t GetNewFileId(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap, int32_t oldFileId)
{
    auto it = photoInfoMap.find(oldFileId);
    if (it != photoInfoMap.end() && it->second.fileIdNew != -1) {
        return it->second.fileIdNew;
    }
    return -1;
}

static bool UpdateGroupId(DedupInfo &mappedInfo, const std::optional<int32_t> &groupId,
    const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    if (!groupId.has_value()) {
        return true;  // groupId为空，视为成功
    }
    int32_t newGroupId = GetNewFileId(photoInfoMap, groupId.value());
    if (newGroupId != -1) {
        if (groupId == mappedInfo.groupIdRep) {
            mappedInfo.groupIdRep = newGroupId;
        } else if (groupId == mappedInfo.groupIdSim) {
            mappedInfo.groupIdSim = newGroupId;
        }
        return true;  // 映射成功
    }
    return false;  // 映射失败
}

void CloneRestoreDupSim::BatchInsertDedupData(
    const std::vector<DedupInfo> &dedupInfos, const std::unordered_set<int32_t> &existingFileIds)
{
    CHECK_AND_RETURN_LOG(mediaLibraryRdb_ != nullptr, "mediaLibraryRdb_ is null");
    CHECK_AND_RETURN_LOG(!dedupInfos.empty(), "dedupInfos are empty");

    int32_t skipNoFileId = 0;
    int32_t skipNoMapping = 0;
    int32_t skipGroupMapping = 0;
    int32_t skipExisting = 0;
    int32_t toInsert = 0;
    std::vector<NativeRdb::ValuesBucket> valuesBuckets;
    for (const auto &info : dedupInfos) {
        if (!info.fileId.has_value()) {
            skipNoFileId++;
            continue;
        }
        int32_t newFileId = GetNewFileId(photoInfoMap_, info.fileId.value());
        if (newFileId == -1) {
            skipNoMapping++;
            continue;
        }
        if (existingFileIds.find(newFileId) != existingFileIds.end()) {
            skipExisting++;
            continue;
        }

        // 检查groupId映射
        DedupInfo mappedInfo = info;
        mappedInfo.fileId = newFileId;
        bool groupRepSuccess = UpdateGroupId(mappedInfo, info.groupIdRep, photoInfoMap_);
        bool groupSimSuccess = UpdateGroupId(mappedInfo, info.groupIdSim, photoInfoMap_);
        // 如果任何一个groupId映射失败，跳过这条数据
        if (!groupRepSuccess || !groupSimSuccess) {
            skipGroupMapping++;
            continue;
        }

        toInsert++;
        valuesBuckets.push_back(CreateValuesBucketFromDedupInfo(mappedInfo));
    }
    if (!valuesBuckets.empty()) {
        int64_t rowNum = 0;
        int32_t ret = BatchInsertWithRetry(DEDUP_TABLE, valuesBuckets, rowNum);
        CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to batch insert dedup data");
        migrateDedupNumber_ += static_cast<uint64_t>(rowNum);
    }
    MEDIA_INFO_LOG("BatchInsertDedupData: skipNoFileId=%{public}d, skipNoMapping=%{public}d, "
                   "skipGroupMapping=%{public}d, skipExisting=%{public}d, toInsert=%{public}d",
        skipNoFileId,
        skipNoMapping,
        skipGroupMapping,
        skipExisting,
        toInsert);
}

NativeRdb::ValuesBucket CloneRestoreDupSim::CreateValuesBucketFromDedupInfo(const DedupInfo &info)
{
    NativeRdb::ValuesBucket values;

    BackupDatabaseUtils::PutIfPresent(values, "file_id", info.fileId);
    BackupDatabaseUtils::PutIfPresent(values, "group_id_rep", info.groupIdRep);
    BackupDatabaseUtils::PutIfPresent(values, "dedup_group_version", info.dedupGroupVersion);
    BackupDatabaseUtils::PutIfPresent(values, "group_id_sim", info.groupIdSim);
    BackupDatabaseUtils::PutIfPresent(values, "sim_group_version", info.simGroupVersion);

    return values;
}

std::vector<AffectiveInfo> CloneRestoreDupSim::QueryAffectiveTblByFileIds(const std::string &fileIdClause)
{
    std::vector<AffectiveInfo> result;
    std::string querySql = "SELECT id, file_id, category, valence, arousal, model_version, model_name, "
                           "extra, timestamp, analysis_version, affective_score, affective_score_version "
                           " FROM " +
                           AFFECTIVE_TABLE;
    querySql += " WHERE file_id IN " + fileIdClause;

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSet is null.");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        AffectiveInfo info;
        info.id = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "id");
        info.fileId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "file_id");
        info.emotionCategory = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "category");
        info.valence = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "valence");
        info.arousal = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "arousal");
        info.modelVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "model_version");
        info.modelName = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "model_name");
        info.extra = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "extra");
        info.timestamp = BackupDatabaseUtils::GetOptionalValue<int64_t>(resultSet, "timestamp");
        info.analysisVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "analysis_version");
        info.affectiveScore = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "affective_score");
        info.affectiveScoreVersion =
            BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "affective_score_version");
        result.emplace_back(info);
    }
    resultSet->Close();
    return result;
}

void CloneRestoreDupSim::BatchInsertAffectiveData(
    const std::vector<AffectiveInfo> &affectiveInfos, const std::unordered_set<int32_t> &existingFileIds)
{
    CHECK_AND_RETURN_LOG(mediaLibraryRdb_ != nullptr, "mediaLibraryRdb_ is null");
    CHECK_AND_RETURN_LOG(!affectiveInfos.empty(), "affectiveInfos are empty");

    std::vector<NativeRdb::ValuesBucket> valuesBuckets;
    std::vector<int32_t> insertedFileIds;  // 记录实际插入的 file_id
    for (const auto &info : affectiveInfos) {
        CHECK_AND_CONTINUE(info.fileId.has_value());
        int32_t newFileId = GetNewFileId(photoInfoMap_, info.fileId.value());
        CHECK_AND_CONTINUE_ERR_LOG(
            newFileId != -1, "Cannot find new file id for old file id: %{public}d", info.fileId.value());
        // 融合模式：如果目标设备已有该file_id的记录，则跳过（保留目标设备的）
        CHECK_AND_CONTINUE(existingFileIds.find(newFileId) == existingFileIds.end());
        AffectiveInfo mappedInfo = info;
        mappedInfo.fileId = newFileId;
        valuesBuckets.push_back(CreateValuesBucketFromAffectiveInfo(mappedInfo));
        insertedFileIds.push_back(newFileId);
    }
    if (!valuesBuckets.empty()) {
        int64_t rowNum = 0;
        int32_t ret = BatchInsertWithRetry(AFFECTIVE_TABLE, valuesBuckets, rowNum);
        CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to batch insert affective data");
        migrateAffectiveNumber_ += static_cast<uint64_t>(rowNum);

        // 插入成功后，记录实际克隆的 file_id 的 BIT3（情感分）和 BIT20
        MEDIA_INFO_LOG("record bit3 and bit20 of mask for affective");
        for (int32_t fileId : insertedFileIds) {
            UpdateScoreMask(fileId, BIT3 | BIT20);
            insertedAffectiveFileIds_.push_back(fileId);
        }
    }
}

NativeRdb::ValuesBucket CloneRestoreDupSim::CreateValuesBucketFromAffectiveInfo(const AffectiveInfo &info)
{
    NativeRdb::ValuesBucket values;

    BackupDatabaseUtils::PutIfPresent(values, "file_id", info.fileId);
    BackupDatabaseUtils::PutIfPresent(values, "category", info.emotionCategory);
    BackupDatabaseUtils::PutIfPresent(values, "valence", info.valence);
    BackupDatabaseUtils::PutIfPresent(values, "arousal", info.arousal);
    BackupDatabaseUtils::PutIfPresent(values, "model_version", info.modelVersion);
    BackupDatabaseUtils::PutIfPresent(values, "model_name", info.modelName);
    BackupDatabaseUtils::PutIfPresent(values, "extra", info.extra);
    BackupDatabaseUtils::PutIfPresent(values, "timestamp", info.timestamp);
    BackupDatabaseUtils::PutIfPresent(values, "analysis_version", info.analysisVersion);
    BackupDatabaseUtils::PutIfPresent(values, "affective_score", info.affectiveScore);
    BackupDatabaseUtils::PutIfPresent(values, "affective_score_version", info.affectiveScoreVersion);

    return values;
}

int32_t CloneRestoreDupSim::BatchInsertWithRetry(
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

void CloneRestoreDupSim::PreprocessSourceTotalTable()
{
    MEDIA_INFO_LOG("PreprocessSourceTotalTable: Set similarity and duplicate to 1 if > 0 in source database");

    // Update similarity field: set to 1 if > 0
    std::string updateSimilaritySql =
        "UPDATE " + VISION_TOTAL_TABLE + " SET " + SIMILARITY + " = 1 WHERE " + SIMILARITY + " > 0";
    int32_t ret1 = BackupDatabaseUtils::ExecuteSQL(mediaRdb_, updateSimilaritySql);
    MEDIA_INFO_LOG("PreprocessSourceTotalTable: similarity updated, ret=%{public}d", ret1);

    // Update duplicate field: set to 1 if > 0
    std::string updateDuplicateSql =
        "UPDATE " + VISION_TOTAL_TABLE + " SET " + DUPLICATE + " = 1 WHERE " + DUPLICATE + " > 0";
    int32_t ret2 = BackupDatabaseUtils::ExecuteSQL(mediaRdb_, updateDuplicateSql);
    MEDIA_INFO_LOG("PreprocessSourceTotalTable: duplicate updated, ret=%{public}d", ret2);
}

void CloneRestoreDupSim::UpdateSimilarityAndDuplicateFields()
{
    std::vector<std::string> fields = {DUPLICATE, SIMILARITY};
    for (const auto &field : fields) {
        CloneRestoreAnalysisTotal cloneRestoreAnalysisTotal;
        cloneRestoreAnalysisTotal.Init(field, QUERY_COUNT, mediaRdb_, mediaLibraryRdb_, VISION_TOTAL_TABLE);
        int32_t totalNumber = cloneRestoreAnalysisTotal.GetTotalNumber();
        for (int32_t offset = 0; offset < totalNumber; offset += QUERY_COUNT) {
            cloneRestoreAnalysisTotal.GetInfos(photoInfoMap_);
            cloneRestoreAnalysisTotal.UpdateDatabase();
        }
        MEDIA_INFO_LOG("UpdateSimilarityAndDuplicateFields: field=%{public}s, totalNumber=%{public}d",
            field.c_str(), totalNumber);
    }
}

std::vector<int32_t> CloneRestoreDupSim::ConvertNewFileIdsToOldFileIds(const std::vector<int32_t> &newFileIds)
{
    std::vector<int32_t> oldFileIds;
    oldFileIds.reserve(newFileIds.size());

    // 使用缓存的映射表直接查找
    for (int32_t newFileId : newFileIds) {
        auto it = newToOldMap_.find(newFileId);
        if (it != newToOldMap_.end()) {
            oldFileIds.push_back(it->second);
        }
    }
    return oldFileIds;
}

void CloneRestoreDupSim::UpdateTotalTableField(const std::string &field, const std::vector<int32_t> &insertedFileIds)
{
    if (insertedFileIds.empty()) {
        MEDIA_INFO_LOG("UpdateTotalTableField: no inserted files, skip %{public}s update", field.c_str());
        return;
    }

    std::vector<int32_t> oldFileIds = ConvertNewFileIdsToOldFileIds(insertedFileIds);
    if (oldFileIds.empty()) {
        return;
    }

    // 从源端查询字段值
    std::vector<std::pair<int32_t, int32_t>> fieldValues = QueryFieldValuesFromSource(field, oldFileIds);
    if (fieldValues.empty()) {
        return;
    }

    // 批量更新到目标端
    BatchUpdateTotalTableField(field, fieldValues);
}

std::vector<std::pair<int32_t, int32_t>> CloneRestoreDupSim::QueryFieldValuesFromSource(
    const std::string &field, const std::vector<int32_t> &oldFileIds)
{
    std::string oldFileIdsStr = "(" + BackupDatabaseUtils::JoinValues(oldFileIds, ",") + ")";
    std::string querySourceSql =
        "SELECT file_id, " + field + " FROM " + std::string(VISION_TOTAL_TABLE) +
        " WHERE file_id IN " + oldFileIdsStr;

    auto sourceResultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, querySourceSql);
    if (sourceResultSet == nullptr) {
        return {};
    }

    std::vector<std::pair<int32_t, int32_t>> fieldValues;
    while (sourceResultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t oldFileId = BackupDatabaseUtils::GetOptionalValue<int32_t>(sourceResultSet, "file_id").value_or(0);
        int32_t fieldValue = BackupDatabaseUtils::GetOptionalValue<int32_t>(sourceResultSet, field).value_or(0);
        if (oldFileId <= 0 || fieldValue <= 0) {
            continue;
        }

        auto photoIt = photoInfoMap_.find(oldFileId);
        if (photoIt == photoInfoMap_.end() || photoIt->second.fileIdNew <= 0) {
            continue;
        }

        fieldValues.push_back({photoIt->second.fileIdNew, fieldValue});
    }
    sourceResultSet->Close();
    return fieldValues;
}

void CloneRestoreDupSim::BatchUpdateTotalTableField(const std::string &field,
    const std::vector<std::pair<int32_t, int32_t>> &fieldValues)
{
    // 批量更新：使用 CASE WHEN 语句，每批处理1000条记录
    const int32_t BATCH_SIZE = 1000;
    for (size_t i = 0; i < fieldValues.size(); i += BATCH_SIZE) {
        auto batch_begin = fieldValues.begin() + i;
        auto batch_end = (i + BATCH_SIZE < fieldValues.size()) ?
                         (fieldValues.begin() + i + BATCH_SIZE) : fieldValues.end();

        // 构建 CASE WHEN 批量更新SQL
        std::string updateSql = "UPDATE " + std::string(VISION_TOTAL_TABLE) +
                                " SET " + field + " = CASE file_id ";
        std::string fileIdsStr;
        for (auto it = batch_begin; it != batch_end; ++it) {
            updateSql += "WHEN " + std::to_string(it->first) + " THEN " + std::to_string(it->second) + " ";
            if (it != batch_begin) {
                fileIdsStr += ",";
            }
            fileIdsStr += std::to_string(it->first);
        }
        updateSql += "END WHERE file_id IN (" + fileIdsStr + ")";

        BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, updateSql);
    }
    MEDIA_INFO_LOG("UpdateTotalTableField: %{public}s field updated, count=%{public}zu",
        field.c_str(), fieldValues.size());
}

void CloneRestoreDupSim::UpdateTotalTableForProfile()
{
    MEDIA_INFO_LOG("start UpdateTotalTableForProfile");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    UpdateSimilarityAndDuplicateFields();
    UpdateTotalTableField(NEGATIVE, insertedProfileFileIds_);

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("UpdateTotalTableForProfile cost %{public}lld ms", (long long)(end - start));
}

void CloneRestoreDupSim::UpdateTotalTableForDedup()
{
    MEDIA_INFO_LOG("start UpdateTotalTableForDedup");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    UpdateSimilarityAndDuplicateFields();

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("UpdateTotalTableForDedup cost %{public}lld ms", (long long)(end - start));
}

void CloneRestoreDupSim::UpdateTotalTableForAffective()
{
    MEDIA_INFO_LOG("start UpdateTotalTableForAffective");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();

    UpdateTotalTableField(AFFECTIVE, insertedAffectiveFileIds_);

    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("UpdateTotalTableForAffective cost %{public}lld ms", (long long)(end - start));
}
}  // namespace OHOS::Media
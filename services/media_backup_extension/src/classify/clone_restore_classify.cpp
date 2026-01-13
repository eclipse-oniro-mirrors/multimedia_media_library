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
#define MLOG_TAG "CloneRestoreClassify"

#include "clone_restore_classify.h"

#include "classify_aggregate_types.h"
#include "backup_database_utils.h"
#include "media_library_db_upgrade.h"
#include "medialibrary_unistore_manager.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "result_set_utils.h"
#include "upgrade_restore_task_report.h"

namespace OHOS::Media {
const int32_t PAGE_SIZE = 200;

const string ID = "id";
const string FILE_ID = "file_id";
const string CATEGORY_ID = "category_id";
const string SUB_LABEL = "sub_label";
const string PROB = "prob";
const string FEATURE = "feature";
const string SIM_RESULT = "sim_result";
const string LABEL_VERSION = "label_version";
const string SALIENCY_SUB_PROB = "saliency_sub_prob";
const string ANALYSIS_VERSION = "analysis_version";
const string CAPTION_RESULT = "caption_result";
const string CAPTION_VERSION = "caption_version";

const string CONFIDENCE_PROBABILITY = "confidence_probability";
const string SUB_CATEGORY = "sub_category";
const string SUB_CONFIDENCE_PROB = "sub_confidence_prob";
const string SUB_LABEL_PROB = "sub_label_prob";
const string SUB_LABEL_TYPE = "sub_label_type";
const string TRACKS = "tracks";
const string VIDEO_PART_FEATURE = "video_part_feature";
const string FILTER_TAG = "filter_tag";
const string ALGO_VERSION = "algo_version";
const string TRIGGER_GENERATE_THUMBNAIL = "trigger_generate_thumbnail";

const string ANALYSIS_LABEL_TABLE = "tab_analysis_label";
const string ANALYSIS_VIDEO_TABLE = "tab_analysis_video_label";
const string FIELD_TYPE_INT = "INT";

const int32_t CLASSIFY_STATUS_SUCCESS = 1;
const int32_t CLASSIFY_TYPE = 4097;
const int32_t FRONT_CAMERA = 1;

const unordered_map<string, unordered_set<string>> COMPARED_COLUMNS_MAP = {
    { "tab_analysis_label",
        {
            "id",
            "file_id",
            "category_id",
            "sub_label",
            "prob",
            "feature",
            "sim_result",
            "label_version",
            "saliency_sub_prob",
            "analysis_version",
            "caption_result",
            "caption_version",
        }
    },
    { "tab_analysis_video_label",
        {
            "id",
            "file_id",
            "category_id",
            "confidence_probability",
            "sub_category",
            "sub_confidence_prob",
            "sub_label",
            "sub_label_prob",
            "sub_label_type",
            "tracks",
            "video_part_feature",
            "filter_tag",
            "algo_version",
            "analysis_version",
            "trigger_generate_thumbnail",
        }
    }
};

template<typename Key, typename Value>
Value GetValueFromMap(const unordered_map<Key, Value> &map, const Key &key, const Value &defaultValue = Value())
{
    auto it = map.find(key);
    CHECK_AND_RETURN_RET(it == map.end(), it->second);
    return defaultValue;
}

void CloneRestoreClassify::Init(int32_t sceneCode, const std::string &taskId,
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, std::shared_ptr<NativeRdb::RdbStore> mediaRdb)
{
    sceneCode_ = sceneCode;
    taskId_ = taskId;
    mediaLibraryRdb_ = mediaLibraryRdb;
    mediaRdb_ = mediaRdb;
    analysisType_ = "label";
}

void CloneRestoreClassify::Restore(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    CHECK_AND_RETURN_LOG(mediaRdb_ != nullptr && mediaLibraryRdb_ != nullptr,
        "ClassifyRestore failed, rdbStore is nullptr");

    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    GetMaxIds();
    RestoreByVersion(photoInfoMap);
    AddSpecialAlbum();
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    restoreTimeCost_ = end - start;
    ReportRestoreTask();
    MEDIA_INFO_LOG("TimeCost: ClassifyRestore: %{public}" PRId64, end - start);
}

void CloneRestoreClassify::RestoreByVersion(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    // Note: if tab_analysis_video_total exists in old database, it is a new version
    BackupDatabaseUtils::isTableExist(mediaRdb_, "tab_analysis_video_total", isRestoreFromNewVersion_);
    isRestoreFromNewVersion_ ? RestoreFromNewVersion(photoInfoMap) : RestoreFromOldVersion(photoInfoMap);
}

void CloneRestoreClassify::RestoreFromOldVersion(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    cloneRestoreAnalysisTotal_.Init(analysisType_, PAGE_SIZE, mediaRdb_, mediaLibraryRdb_);
    int32_t totalNumber = cloneRestoreAnalysisTotal_.GetTotalNumber();
    for (int32_t offset = 0; offset < totalNumber; offset += PAGE_SIZE) {
        RestoreBatch(photoInfoMap);
    }
    HandleOldVersionData();
}

void CloneRestoreClassify::RestoreFromNewVersion(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    // Note: first restore tab_analysis_total + tab_analysis_label
    infoType_ = InfoType::IMAGE;
    cloneRestoreAnalysisTotal_.Init(analysisType_, PAGE_SIZE, mediaRdb_, mediaLibraryRdb_);
    int32_t totalNumber = cloneRestoreAnalysisTotal_.GetTotalNumber();
    for (int32_t offset = 0; offset < totalNumber; offset += PAGE_SIZE) {
        RestoreBatchByType(photoInfoMap);
    }
    // Note: second restore tab_analysis_video_total + tab_analysis_video_label
    infoType_ = InfoType::VIDEO;
    cloneRestoreAnalysisVideoTotal_.Init(analysisType_, PAGE_SIZE, mediaRdb_, mediaLibraryRdb_,
        "tab_analysis_video_total");
    totalNumber = cloneRestoreAnalysisVideoTotal_.GetTotalNumber();
    for (int32_t offset = 0; offset < totalNumber; offset += PAGE_SIZE) {
        RestoreBatchByType(photoInfoMap);
    }
}

void CloneRestoreClassify::GetMaxIds()
{
    maxIdOfLabel_ = BackupDatabaseUtils::QueryMaxId(mediaLibraryRdb_, ANALYSIS_LABEL_TABLE, ID);
    maxIdOfVideoLabel_ = BackupDatabaseUtils::QueryMaxId(mediaLibraryRdb_, ANALYSIS_VIDEO_TABLE, ID);
}

void CloneRestoreClassify::RestoreBatch(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    int64_t startGet = MediaFileUtils::UTCTimeMilliSeconds();
    cloneRestoreAnalysisTotal_.GetInfos(photoInfoMap);
    int64_t startRestoreMaps = MediaFileUtils::UTCTimeMilliSeconds();
    RestoreMaps();
    int64_t startRestoreVideoMaps = MediaFileUtils::UTCTimeMilliSeconds();
    RestoreVideoMaps();
    int64_t startUpdate = MediaFileUtils::UTCTimeMilliSeconds();
    cloneRestoreAnalysisTotal_.UpdateDatabase();
    int64_t startAdd = MediaFileUtils::UTCTimeMilliSeconds();
    cloneRestoreAnalysisTotal_.AddSuccessVideoFileIds(); // Note: keep track of all the success video fileIds
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("TimeCost: GetAnalysisTotalInfos: %{public}" PRId64 ", RestoreMaps: %{public}" PRId64
        ", RestoreVideoMaps: %{public}" PRId64 ", UpdateDatabase: %{public}" PRId64
        ",AddSuccessVideoFileIds: %{public}" PRId64,
        startRestoreMaps - startGet, startRestoreVideoMaps - startRestoreMaps, startUpdate - startRestoreVideoMaps,
        startAdd - startUpdate, end - startAdd);
}

void CloneRestoreClassify::RestoreBatchByType(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    CloneRestoreAnalysisTotal &analysisTotal = GetCloneRestoreAnalysisTotal();
    int64_t startGet = MediaFileUtils::UTCTimeMilliSeconds();
    analysisTotal.GetInfos(photoInfoMap);
    int64_t startRestoreMapsByType = MediaFileUtils::UTCTimeMilliSeconds();
    RestoreMapsByType();
    int64_t startUpdate = MediaFileUtils::UTCTimeMilliSeconds();
    analysisTotal.UpdateDatabase();
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("TimeCost: GetAnalysisTotalInfos: %{public}" PRId64 ", RestoreMapsByType: %{public}" PRId64
        ", UpdateDatabase: %{public}" PRId64,
        startRestoreMapsByType - startGet, startUpdate - startRestoreMapsByType, end - startUpdate);
}

void CloneRestoreClassify::RestoreMapsByType()
{
    infoType_ == InfoType::VIDEO ? RestoreVideoMaps() : RestoreMaps();
}

void CloneRestoreClassify::RestoreMaps()
{
    bool cond = (mediaRdb_ == nullptr || mediaLibraryRdb_ == nullptr);
    CHECK_AND_RETURN_LOG(!cond, "rdbStore is nullptr");

    MEDIA_INFO_LOG("restore classify albums start.");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    std::vector<ClassifyCloneInfo> classifyInfos;
    GetClassifyInfos(classifyInfos);
    int64_t startInsert = MediaFileUtils::UTCTimeMilliSeconds();
    InsertClassifyAlbums(classifyInfos);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("TimeCost: GetClassifyInfos: %{public}" PRId64 ", InsertClassifyAlbums: %{public}" PRId64,
        startInsert - start, end - startInsert);
    restoreLabelTimeCost_ += end - start;
    MEDIA_INFO_LOG("restore classify albums end.");
}

void CloneRestoreClassify::RestoreVideoMaps()
{
    bool cond = (mediaRdb_ == nullptr || mediaLibraryRdb_ == nullptr);
    CHECK_AND_RETURN_LOG(!cond, "rdbStore is nullptr");

    MEDIA_INFO_LOG("restore classify video albums start.");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    std::vector<ClassifyVideoCloneInfo> classifyVideoInfos;
    GetClassifyVideoInfos(classifyVideoInfos);
    int64_t startInsert = MediaFileUtils::UTCTimeMilliSeconds();
    InsertClassifyVideoAlbums(classifyVideoInfos);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("TimeCost: GetClassifyVideoInfos: %{public}" PRId64 ", InsertClassifyVideoAlbums: %{public}" PRId64,
        startInsert - start, end - startInsert);
    restoreVideoLabelTimeCost_ += end - start;
    MEDIA_INFO_LOG("restore classify video albums end.");
}

void CloneRestoreClassify::GetClassifyInfos(std::vector<ClassifyCloneInfo> &classifyInfos)
{
    std::unordered_map<std::string, std::string> columns;
    columns[FILE_ID] = FIELD_TYPE_INT;
    bool hasRequiredColumns = CheckTableColumns(ANALYSIS_LABEL_TABLE, columns);
    if (!hasRequiredColumns) {
        MEDIA_ERR_LOG("The tab_analysis_label does not contain the required columns.");
        ErrorInfo errorInfo(RestoreError::TABLE_LACK_OF_COLUMN, static_cast<int32_t>(columns.size()),
            "", "The tab_analysis_label does not contain id or file_id");
        UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
        return;
    }

    std::stringstream querySql;
    std::string placeHolders;
    std::vector<NativeRdb::ValueObject> params;
    cloneRestoreAnalysisTotal_.SetPlaceHoldersAndParamsByFileIdOld(placeHolders, params);
    querySql << "SELECT * FROM " + ANALYSIS_LABEL_TABLE + " WHERE " + FILE_ID + " IN (" << placeHolders << ")";

    auto resultSet = BackupDatabaseUtils::QuerySql(mediaRdb_, querySql.str(), params);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Query resultSql is null.");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        ClassifyCloneInfo info;
        GetClassifyInfo(info, resultSet);
        classifyInfos.emplace_back(info);
    }
    resultSet->Close();
    MEDIA_INFO_LOG("query tab_analysis_label nums: %{public}zu", classifyInfos.size());
}

void CloneRestoreClassify::GetClassifyVideoInfos(std::vector<ClassifyVideoCloneInfo> &classifyVideoInfos)
{
    CloneRestoreAnalysisTotal &analysisTotal = GetCloneRestoreAnalysisTotal();
    std::unordered_map<std::string, std::string> columns;
    columns[FILE_ID] = FIELD_TYPE_INT;
    bool hasRequiredColumns = CheckTableColumns(ANALYSIS_VIDEO_TABLE, columns);
    if (!hasRequiredColumns) {
        MEDIA_ERR_LOG("The tab_analysis_video_label does not contain the required columns.");
        ErrorInfo errorInfo(RestoreError::TABLE_LACK_OF_COLUMN, static_cast<int32_t>(columns.size()),
            "", "The tab_analysis_video_label does not contain file_id");
        UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
        return;
    }

    std::stringstream querySql;
    std::string placeHolders;
    std::vector<NativeRdb::ValueObject> params;
    analysisTotal.SetPlaceHoldersAndParamsByFileIdOld(placeHolders, params);
    querySql << "SELECT * FROM " + ANALYSIS_VIDEO_TABLE + " WHERE " + FILE_ID + " IN (" << placeHolders << ")";

    auto resultSet = BackupDatabaseUtils::QuerySql(mediaRdb_, querySql.str(), params);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Query resultSql is null.");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        ClassifyVideoCloneInfo info;
        GetClassifyVideoInfo(info, resultSet);
        classifyVideoInfos.emplace_back(info);
    }
    resultSet->Close();
    MEDIA_INFO_LOG("query tab_analysis_video_label nums: %{public}zu", classifyVideoInfos.size());
}

void CloneRestoreClassify::DeduplicateClassifyInfos(std::vector<ClassifyCloneInfo> &infos)
{
    CHECK_AND_RETURN(!infos.empty());
    std::unordered_set<int32_t> existingFileIds = GetExistingFileIds(ANALYSIS_LABEL_TABLE);
    RemoveDuplicateClassifyInfos(infos, existingFileIds);
    MEDIA_INFO_LOG("existing: %{public}zu, after deduplicate: %{public}zu", existingFileIds.size(), infos.size());
}

void CloneRestoreClassify::DeduplicateClassifyVideoInfos(std::vector<ClassifyVideoCloneInfo> &infos)
{
    CHECK_AND_RETURN(!infos.empty());
    std::unordered_set<int32_t> existingFileIds = GetExistingFileIds(ANALYSIS_VIDEO_TABLE);
    RemoveDuplicateClassifyVideoInfos(infos, existingFileIds);
    MEDIA_INFO_LOG("existing: %{public}zu, after deduplicate: %{public}zu", existingFileIds.size(), infos.size());
}

std::unordered_set<int32_t> CloneRestoreClassify::GetExistingFileIds(const std::string &tableName)
{
    std::unordered_set<int32_t> existingFileIds;
    std::stringstream querySql;
    std::string placeHolders;
    std::vector<NativeRdb::ValueObject> params;
    cloneRestoreAnalysisTotal_.SetPlaceHoldersAndParamsByFileIdNew(placeHolders, params);
    querySql << "SELECT file_id FROM " + tableName + " WHERE " + FILE_ID + " IN (" << placeHolders << ")";

    auto resultSet = BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, querySql.str(), params);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, existingFileIds, "Query resultSql is null.");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = GetInt32Val("file_id", resultSet);
        existingFileIds.insert(fileId);
    }
    resultSet->Close();
    return existingFileIds;
}

void CloneRestoreClassify::RemoveDuplicateClassifyInfos(std::vector<ClassifyCloneInfo> &infos,
    const std::unordered_set<int32_t> &existingFileIds)
{
    infos.erase(std::remove_if(infos.begin(), infos.end(), [&](ClassifyCloneInfo &info) {
        if (!info.fileIdOld.has_value()) {
            return true;
        }

        size_t index = cloneRestoreAnalysisTotal_.FindIndexByFileIdOld(info.fileIdOld.value());
        if (index == std::string::npos) {
            return true;
        }

        int32_t fileIdNew = cloneRestoreAnalysisTotal_.GetFileIdNewByIndex(index);
        info.fileIdNew = fileIdNew;
        if (existingFileIds.count(fileIdNew) == 0) {
            return false;
        }
        cloneRestoreAnalysisTotal_.UpdateRestoreStatusAsDuplicateByIndex(index);
        duplicateLabelCnt_++;
        return true;
    }),
        infos.end());
}

void CloneRestoreClassify::RemoveDuplicateClassifyVideoInfos(std::vector<ClassifyVideoCloneInfo> &infos,
    const std::unordered_set<int32_t> &existingFileIds)
{
    CloneRestoreAnalysisTotal &analysisTotal = GetCloneRestoreAnalysisTotal();
    infos.erase(std::remove_if(infos.begin(), infos.end(), [&](ClassifyVideoCloneInfo &info) {
        if (!info.fileIdOld.has_value()) {
            return true;
        }

        size_t index = analysisTotal.FindIndexByFileIdOld(info.fileIdOld.value());
        if (index == std::string::npos) {
            return true;
        }

        int32_t fileIdNew = analysisTotal.GetFileIdNewByIndex(index);
        info.fileIdNew = fileIdNew;
        if (existingFileIds.count(fileIdNew) == 0) {
            return false;
        }
        analysisTotal.UpdateRestoreStatusAsDuplicateByIndex(index);
        duplicateVideoLabelCnt_++;
        return true;
    }),
        infos.end());
}

void CloneRestoreClassify::InsertClassifyAlbums(std::vector<ClassifyCloneInfo> &classifyInfos)
{
    DeduplicateClassifyInfos(classifyInfos);
    CHECK_AND_RETURN(!classifyInfos.empty());

    std::unordered_set<std::string> intersection = GetCommonColumns(ANALYSIS_LABEL_TABLE);
    size_t offset = 0;
    do {
        std::vector<NativeRdb::ValuesBucket> values;
        for (size_t index = 0; index < PAGE_SIZE && index + offset < classifyInfos.size(); index++) {
            CHECK_AND_CONTINUE(classifyInfos[index + offset].fileIdNew.has_value());
            NativeRdb::ValuesBucket value;
            GetMapInsertValue(value, classifyInfos[index + offset], intersection);
            values.emplace_back(value);
        }
        MEDIA_INFO_LOG("Insert classify albums, values size: %{public}zu", values.size());
        int64_t rowNum = 0;
        int32_t errCode = BatchInsertWithRetry(ANALYSIS_LABEL_TABLE, values, rowNum);
        if (errCode != E_OK || rowNum != static_cast<int64_t>(values.size())) {
            int64_t failNums = static_cast<int64_t>(values.size()) - rowNum;
            MEDIA_ERR_LOG("Insert classify albums fail, num: %{public}" PRId64, failNums);
            ErrorInfo errorInfo(RestoreError::INSERT_FAILED, static_cast<int32_t>(values.size()),
                "errCode: " + std::to_string(errCode), "Insert classify albums fail");
            UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
            cloneRestoreAnalysisTotal_.UpdateRestoreStatusAsFailed();
            failInsertLabelCnt_ += failNums;
        }
        offset += PAGE_SIZE;
        successInsertLabelCnt_ += rowNum;
    } while (offset < classifyInfos.size());
}

void CloneRestoreClassify::InsertClassifyVideoAlbums(std::vector<ClassifyVideoCloneInfo> &classifyVideoInfos)
{
    CloneRestoreAnalysisTotal &analysisTotal = GetCloneRestoreAnalysisTotal();
    DeduplicateClassifyVideoInfos(classifyVideoInfos);
    CHECK_AND_RETURN(!classifyVideoInfos.empty());

    std::unordered_set<std::string> intersection = GetCommonColumns(ANALYSIS_VIDEO_TABLE);
    size_t offset = 0;
    do {
        std::vector<NativeRdb::ValuesBucket> values;
        for (size_t index = 0; index < PAGE_SIZE && index + offset < classifyVideoInfos.size(); index++) {
            CHECK_AND_CONTINUE(classifyVideoInfos[index + offset].fileIdNew.has_value());
            NativeRdb::ValuesBucket value;
            GetVideoMapInsertValue(value, classifyVideoInfos[index + offset], intersection);
            values.emplace_back(value);
        }
        MEDIA_INFO_LOG("Insert classify video albums, values size: %{public}zu", values.size());
        int64_t rowNum = 0;
        int32_t errCode = BatchInsertWithRetry(ANALYSIS_VIDEO_TABLE, values, rowNum);
        if (errCode != E_OK || rowNum != static_cast<int64_t>(values.size())) {
            int64_t failNums = static_cast<int64_t>(values.size()) - rowNum;
            MEDIA_ERR_LOG("Insert classify video albums fail, num: %{public}" PRId64, failNums);
            ErrorInfo errorInfo(RestoreError::INSERT_FAILED, static_cast<int32_t>(values.size()),
                "errCode: " + std::to_string(errCode), "Insert classify video albums fail");
            UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
            analysisTotal.UpdateRestoreStatusAsFailed();
            failInsertVideoLabelCnt_ += failNums;
        }
        offset += PAGE_SIZE;
        successInsertVideoLabelCnt_ += rowNum;
    } while (offset < classifyVideoInfos.size());
}

void CloneRestoreClassify::GetClassifyInfo(ClassifyCloneInfo &info,
    std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
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
}

void CloneRestoreClassify::GetMapInsertValue(NativeRdb::ValuesBucket &value, ClassifyCloneInfo info,
    const std::unordered_set<std::string> &intersection)
{
    PutIfInIntersection(value, FILE_ID, info.fileIdNew, intersection);
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
}

void CloneRestoreClassify::GetClassifyVideoInfo(ClassifyVideoCloneInfo &info,
    std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
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

void CloneRestoreClassify::GetVideoMapInsertValue(NativeRdb::ValuesBucket &value, ClassifyVideoCloneInfo info,
    const std::unordered_set<std::string> &intersection)
{
    PutIfInIntersection(value, FILE_ID, info.fileIdNew, intersection);
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

bool CloneRestoreClassify::CheckTableColumns(const std::string& tableName,
    std::unordered_map<std::string, std::string>& columns)
{
    std::unordered_map<std::string, std::string> srcColumnInfoMap =
        BackupDatabaseUtils::GetColumnInfoMap(mediaRdb_, tableName);
    for (auto it = columns.begin(); it != columns.end(); ++it) {
        CHECK_AND_CONTINUE(srcColumnInfoMap.find(it->first) == srcColumnInfoMap.end());
        return false;
    }
    return true;
}

std::unordered_set<std::string> CloneRestoreClassify::GetCommonColumns(const string &tableName)
{
    std::unordered_map<std::string, std::string> srcColumnInfoMap =
        BackupDatabaseUtils::GetColumnInfoMap(mediaRdb_, tableName);
    std::unordered_map<std::string, std::string> dstColumnInfoMap =
        BackupDatabaseUtils::GetColumnInfoMap(mediaLibraryRdb_, tableName);
    std::unordered_set<std::string> result;
    auto comparedColumns = GetValueFromMap(COMPARED_COLUMNS_MAP, tableName);
    for (auto it = dstColumnInfoMap.begin(); it != dstColumnInfoMap.end(); ++it) {
        bool cond = srcColumnInfoMap.find(it->first) != srcColumnInfoMap.end() && comparedColumns.count(it->first) > 0;
        CHECK_AND_EXECUTE(!cond, result.insert(it->first));
    }
    return result;
}

int32_t CloneRestoreClassify::BatchInsertWithRetry(const std::string &tableName,
    std::vector<NativeRdb::ValuesBucket> &values, int64_t &rowNum)
{
    CHECK_AND_RETURN_RET(!values.empty(), E_OK);
    int32_t errCode = E_ERR;
    TransactionOperations trans{ __func__ };
    trans.SetBackupRdbStore(mediaLibraryRdb_);
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

void CloneRestoreClassify::ReportRestoreTask()
{
    ReportRestoreTaskOfTotal();
    ReportRestoreTaskofImage();
    ReportRestoreTaskofVideo();
}

void CloneRestoreClassify::ReportRestoreTaskOfTotal()
{
    RestoreTaskInfo info;
    cloneRestoreAnalysisTotal_.SetRestoreTaskInfo(info);
    info.type = "CLONE_RESTORE_CLASSIFY_TOTAL";
    info.errorCode = std::to_string(CLASSIFY_STATUS_SUCCESS);
    info.errorInfo = "timeCost: " + std::to_string(restoreTimeCost_) +
        ", isRestoreFromNewVersion: " + std::to_string(isRestoreFromNewVersion_);
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).Report(info);
}

void CloneRestoreClassify::ReportRestoreTaskofImage()
{
    RestoreTaskInfo info;
    info.type = "CLONE_RESTORE_CLASSIFY_IMAGE";
    info.errorCode = std::to_string(CLASSIFY_STATUS_SUCCESS);
    info.errorInfo =
        "max_id: " + std::to_string(maxIdOfLabel_) +
        ", timeCost: " + std::to_string(restoreLabelTimeCost_);
    info.successCount = successInsertLabelCnt_;
    info.failedCount = failInsertLabelCnt_;
    info.duplicateCount = duplicateLabelCnt_;
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).Report(info);
}

void CloneRestoreClassify::ReportRestoreTaskofVideo()
{
    RestoreTaskInfo info;
    info.type = "CLONE_RESTORE_CLASSIFY_VIDEO";
    info.errorCode = std::to_string(CLASSIFY_STATUS_SUCCESS);
    info.errorInfo =
        "max_id: " + std::to_string(maxIdOfVideoLabel_) +
        ", timeCost: " + std::to_string(restoreVideoLabelTimeCost_);
    info.successCount = successInsertVideoLabelCnt_;
    info.failedCount = failInsertVideoLabelCnt_;
    info.duplicateCount = duplicateVideoLabelCnt_;
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).Report(info);
}

void CloneRestoreClassify::AddSpecialAlbum()
{
    MEDIA_INFO_LOG("AddSpecialAlbum start");
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    AddSelfieAlbum();
    AddUserCommentAlbum();
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("AddSpecialAlbum end, cost: %{public}" PRId64,
        endTime - startTime);
}

void CloneRestoreClassify::AddSelfieAlbum()
{
    CHECK_AND_RETURN_LOG(mediaLibraryRdb_ != nullptr, "AddSelfieAlbum failed, rdbStore is nullptr");
    std::string selfieAlbum = std::to_string(static_cast<int32_t>(AggregateType::SELFIE_ALBUM));
    DataTransfer::MediaLibraryDbUpgrade medialibraryDbUpgrade;
    bool isSetAggregateBit;
    bool isExist =  medialibraryDbUpgrade.CheckClassifyAlbumExist(selfieAlbum,
        *this->mediaLibraryRdb_, isSetAggregateBit);
    CHECK_AND_RETURN_INFO_LOG(!isExist, "SelfieAlbum already exist.");
    std::string querySql = "SELECT count(1) AS count FROM Photos WHERE front_camera = ?;";
    std::vector<NativeRdb::ValueObject> params;
    params.push_back(FRONT_CAMERA);
    auto resultSet = mediaLibraryRdb_->QuerySql(querySql, params);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "resultSet is nullptr");
    if (resultSet->GoToNextRow() == NativeRdb::E_OK && GetInt32Val("count", resultSet) > 0) {
        resultSet->Close();
        int32_t ret = medialibraryDbUpgrade.CreateClassifyAlbum(selfieAlbum, *this->mediaLibraryRdb_);
        CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK, "AddSelfieAlbum failed");
        MEDIA_INFO_LOG("AddSelfieAlbum success");
        return;
    }
    resultSet->Close();
    MEDIA_INFO_LOG("not meet the criteria for AddSelfieAlbum");
}

void CloneRestoreClassify::AddUserCommentAlbum()
{
    CHECK_AND_RETURN_LOG(mediaLibraryRdb_ != nullptr, "AddUserCommentAlbum failed, rdbStore is nullptr");
    std::string userCommentAlbum = std::to_string(static_cast<int32_t>(AggregateType::USER_COMMENT_ALBUM));
    DataTransfer::MediaLibraryDbUpgrade medialibraryDbUpgrade;
    bool isSetAggregateBit;
    bool isExist =  medialibraryDbUpgrade.CheckClassifyAlbumExist(userCommentAlbum,
        *this->mediaLibraryRdb_, isSetAggregateBit);
    CHECK_AND_RETURN_INFO_LOG(!isExist, "UserCommentAlbum already exist.");
    std::string querySql = "SELECT count(1) AS count FROM Photos "
        "WHERE user_comment IS NOT NULL AND user_comment != '';";
    auto resultSet = mediaLibraryRdb_->QuerySql(querySql);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "resultSet is nullptr");
    if (resultSet->GoToNextRow() == NativeRdb::E_OK && GetInt32Val("count", resultSet) > 0) {
        resultSet->Close();
        int32_t ret = medialibraryDbUpgrade.CreateClassifyAlbum(userCommentAlbum, *this->mediaLibraryRdb_);
        CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK, "AddUserCommentAlbum failed");
        MEDIA_INFO_LOG("AddUserCommentAlbum success");
        return;
    }
    resultSet->Close();
    MEDIA_INFO_LOG("not meet the criteria for AddUserCommentAlbum");
}

CloneRestoreAnalysisTotal& CloneRestoreClassify::GetCloneRestoreAnalysisTotal()
{
    return infoType_ == InfoType::VIDEO ? cloneRestoreAnalysisVideoTotal_ : cloneRestoreAnalysisTotal_;
}

void CloneRestoreClassify::HandleOldVersionData()
{
    // Move the values of column label in tab_analysis_total to tab_analysis_video_total, and clear tab_analysis_total
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    std::vector<int32_t> videoFileIds = cloneRestoreAnalysisTotal_.GetSuccessVideoFileIds();
    CHECK_AND_RETURN(!videoFileIds.empty());

    for (size_t index = 0; index < videoFileIds.size(); index += static_cast<size_t>(PAGE_SIZE)) {
        std::vector<int32_t> batchFileIds(videoFileIds.begin() + index,
            videoFileIds.begin() + std::min(index + static_cast<size_t>(PAGE_SIZE), videoFileIds.size()));
        std::string fileIdInClause = BackupDatabaseUtils::JoinValues(batchFileIds, ",");

        std::string updateVideoTotalSql =
            "UPDATE tab_analysis_video_total SET label = (SELECT t.label FROM tab_analysis_total AS t "
            "WHERE t.file_id = tab_analysis_video_total.file_id) WHERE file_id IN (" + fileIdInClause + ")";
        int32_t ret = BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, updateVideoTotalSql);
        CHECK_AND_PRINT_INFO_LOG(ret == NativeRdb::E_OK, "Update video total sql failed");

        std::string clearTotalSql =
            "UPDATE tab_analysis_total SET label = 0, status = CASE WHEN status = 1 THEN 0 ELSE status END "
            "WHERE file_id IN (" + fileIdInClause + ")";
        ret = BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, clearTotalSql);
        CHECK_AND_PRINT_INFO_LOG(ret == NativeRdb::E_OK, "clear total sql failed");
    }
    CheckLabelToAnalysisVideoTotalTable();
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("TimeCost: HandleOldVersionData: %{public}" PRId64, end - start);
}

void CloneRestoreClassify::CheckLabelToAnalysisVideoTotalTable()
{
    std::string checkLabel_sql =
        "UPDATE tab_analysis_video_total SET label = 0, "
        "status = CASE WHEN status = 1 THEN 0 ELSE status END "
        "WHERE NOT EXISTS ( "
            "SELECT 1 FROM tab_analysis_video_label "
            "WHERE file_id = tab_analysis_video_total.file_id);";
    MEDIA_DEBUG_LOG("CheckFaceToAnalysisVideoTotalTable \
        checkLabel_sql sql = %{public}s", checkLabel_sql.c_str());
    int32_t ret = BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, checkLabel_sql);
    CHECK_AND_PRINT_INFO_LOG(ret == NativeRdb::E_OK, "check video total failed");
}
}
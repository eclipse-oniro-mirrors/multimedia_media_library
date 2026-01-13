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

#include "classify_restore.h"
#include <algorithm>
#include <cctype>
#include "media_library_db_upgrade.h"
#include "userfile_manager_types.h"
#include "backup_database_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "upgrade_restore_task_report.h"
#include "medialibrary_data_manager_utils.h"
// LCOV_EXCL_START
namespace OHOS::Media {
const int32_t PAGE_SIZE = 200;
const int32_t INVALID_LABEL = -2;
const int32_t CLASSIFY_RESTORE_STATUS_SUCCESS = 1;
const int32_t FRONT_CAMERA = 1;
const std::string VERSION_PREFIX = "backup";
const std::string ANALYSIS_LABEL_TABLE = "tab_analysis_label";
const std::string ID = "id";
const int32_t ADD_ITEMS = 10000;

const int64_t THRESHOLD_DATA_SIZE = 30000;
const int64_t THRESHOLD_DATA_TIME = 600000; // 不超过THRESHOLD_DATA_SIZE，基线为10min
const int64_t DEFAULT_FAULT_TIME = 0;

// 超出THRESHOLD_DATA_SIZE，每1w数据基线为216s
const int64_t BASIC_NUMBER = 10000;
const int64_t SUPPORT_NUMBER = 9999;
const int64_t SINGLE_OVER_THRESHOLD_DATA_TIME = 216000;

const int32_t NORMAL_EXIT_CODE = 0; // 正常恢复
const int32_t MIDDLE_EXIT_CODE = 1; // 异常恢复，恢复中超时退出
const int32_t BEGIN_EXIT_CODE = 2; // 异常恢复，一开始超时退出

void ClassifyRestore::Init(int32_t sceneCode, const std::string &taskId,
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, std::shared_ptr<NativeRdb::RdbStore> galleryRdb)
{
    sceneCode_ = sceneCode;
    taskId_ = taskId;
    mediaLibraryRdb_ = mediaLibraryRdb;
    galleryRdb_ = galleryRdb;
}

void ClassifyRestore::RestoreClassify(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    MEDIA_INFO_LOG("RestoreClassify start");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    CHECK_AND_RETURN_LOG(galleryRdb_ != nullptr && mediaLibraryRdb_ != nullptr, "rdbStore is nullptr");
    albumAssetMap_.clear();
    albumIdCache_.clear();
    GetMaxIds();
    RestoreLabel(photoInfoMap);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    restoreTimeCost_ = end - start;
    ReportRestoreTask();
    MEDIA_INFO_LOG("RestoreClassify Time cost: %{public}" PRId64, end - start);
}

void ClassifyRestore::TransferLabelInfo(GalleryLabelInfo &info)
{
    info.version = VERSION_PREFIX + info.version;
    CHECK_AND_EXECUTE(info.categoryId != INVALID_LABEL, info.subLabel = "");
    CHECK_AND_EXECUTE(info.subLabel.empty(), info.subLabel.pop_back()); // remove last ','
    info.subLabel = "[" + info.subLabel + "]";
}

void ClassifyRestore::UpdateLabelInsertValues(std::vector<NativeRdb::ValuesBucket> &values,
    const GalleryLabelInfo &info)
{
    CHECK_AND_RETURN(info.photoInfo.fileIdNew > 0);
    NativeRdb::ValuesBucket value;
    value.PutInt("file_id", info.photoInfo.fileIdNew);
    value.PutInt("category_id", info.categoryId);
    value.PutString("sub_label", info.subLabel);
    value.PutDouble("prob", info.prob);
    value.PutString("label_version", info.version);
    values.push_back(value);
}

void ClassifyRestore::UpdateStatus(std::vector<int32_t> &fileIds)
{
    CHECK_AND_RETURN_WARN_LOG(!fileIds.empty(), "fileIds is empty");
    std::string fileIdClause = "(" + BackupDatabaseUtils::JoinValues<int>(fileIds, ", ") + ");";
    std::string updateSql =
        "UPDATE tab_analysis_total "
        "SET label = 1 "
        "WHERE EXISTS (SELECT 1 FROM tab_analysis_label "
                        "WHERE tab_analysis_label.file_id = tab_analysis_total.file_id) "
        "AND file_id IN " + fileIdClause;
    int32_t ret = BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, updateSql);
    CHECK_AND_RETURN_LOG(ret >= 0, "execute update analysis total failed, ret = %{public}d", ret);
}

void ClassifyRestore::DeleteExistMapping(std::vector<int32_t> &fileIds)
{
    CHECK_AND_RETURN_WARN_LOG(!fileIds.empty(), "fileIds is empty");
    std::string fileIdClause = "(" + BackupDatabaseUtils::JoinValues<int>(fileIds, ", ") + ");";
    std::string deleteSql =
        "DELETE FROM AnalysisPhotoMap "
        "WHERE map_album IN (SELECT album_id FROM AnalysisAlbum WHERE album_subtype = 4097) "
        "AND map_asset IN " + fileIdClause;
    int32_t ret = BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, deleteSql);
    CHECK_AND_RETURN_LOG(ret >= 0, "execute delete exist mapping failed, ret = %{public}d", ret);
}

void ClassifyRestore::ProcessCategoryAlbums()
{
    CreateOrUpdateCategoryAlbums();
    EnsureSpecialAlbums();
}

std::vector<int32_t> ClassifyRestore::ParseSubLabel(const std::string &subLabel) const
{
    std::vector<int32_t> labels;
    CHECK_AND_RETURN_RET(!subLabel.empty(), labels);

    std::string content = subLabel;
    if (!content.empty() && content.front() == '[') {
        content.erase(content.begin());
    }
    if (!content.empty() && content.back() == ']') {
        content.pop_back();
    }
    std::vector<std::string> parts = BackupDatabaseUtils::SplitString(content, ',');
    for (auto &part : parts) {
        while (!part.empty() && std::isspace(static_cast<unsigned char>(part.front()))) {
            part.erase(part.begin());
        }
        while (!part.empty() && std::isspace(static_cast<unsigned char>(part.back()))) {
            part.pop_back();
        }
        if (part.empty()) {
            continue;
        }
        if (MediaLibraryDataManagerUtils::IsNumber(part)) {
            labels.emplace_back(std::stoi(part));
        } else {
            MEDIA_WARN_LOG("Label is not a valid number: %{public}s", part.c_str());
        }
    }
    return labels;
}

std::unordered_set<int32_t> ClassifyRestore::GetAggregateTypes(const std::vector<int32_t> &labels) const
{
    std::unordered_set<int32_t> aggregates;
    for (auto label : labels) {
        auto it = AGGREGATE_MAPPING_TABLE.find(static_cast<PhotoLabel>(label));
        if (it != AGGREGATE_MAPPING_TABLE.end()) {
            aggregates.insert(static_cast<int32_t>(it->second));
        }
    }
    return aggregates;
}

void ClassifyRestore::CollectAlbumInfo(int32_t fileIdNew, int32_t categoryId, const std::vector<int32_t> &labels)
{
    CHECK_AND_RETURN(fileIdNew > 0);
    if (categoryId != INVALID_LABEL) {
        std::string categoryAlbum = std::to_string(ADD_ITEMS + categoryId);
        albumAssetMap_[categoryAlbum].insert(fileIdNew);
    }
    std::unordered_set<int32_t> uniqueLabels(labels.begin(), labels.end());
    for (int32_t label : uniqueLabels) {
        albumAssetMap_[std::to_string(label)].insert(fileIdNew);
    }
    auto aggregates = GetAggregateTypes(labels);
    aggregates.erase(static_cast<int32_t>(AggregateType::SELFIE_ALBUM));
    aggregates.erase(static_cast<int32_t>(AggregateType::USER_COMMENT_ALBUM));
    for (int32_t aggregate : aggregates) {
        albumAssetMap_[std::to_string(aggregate)].insert(fileIdNew);
    }
}

int32_t ClassifyRestore::EnsureClassifyAlbumId(const std::string &albumName)
{
    auto cached = albumIdCache_.find(albumName);
    if (cached != albumIdCache_.end()) {
        return cached->second;
    }
    CHECK_AND_RETURN_RET(mediaLibraryRdb_ != nullptr, -1);
    DataTransfer::MediaLibraryDbUpgrade dbUpgrade;
    bool isSetAggregateBit = false;
    bool exists = dbUpgrade.CheckClassifyAlbumExist(albumName, *mediaLibraryRdb_, isSetAggregateBit);
    if (!exists) {
        int32_t ret = dbUpgrade.CreateClassifyAlbum(albumName, *mediaLibraryRdb_);
        CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, -1,
            "Create classify album failed, album:%{public}s", albumName.c_str());
    }
    std::string querySql = "SELECT album_id FROM AnalysisAlbum WHERE album_type = ? "
        "AND album_subtype = ? AND album_name = ?";
    std::vector<NativeRdb::ValueObject> params = {
        static_cast<int32_t>(PhotoAlbumType::SMART),
        static_cast<int32_t>(PhotoAlbumSubType::CLASSIFY),
        albumName
    };
    auto resultSet = mediaLibraryRdb_->QuerySql(querySql, params);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, -1, "Query classify album id failed");
    int32_t albumId = -1;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        albumId = GetInt32Val("album_id", resultSet);
        albumIdCache_[albumName] = albumId;
    }
    resultSet->Close();
    return albumId;
}

void ClassifyRestore::InsertAlbumMappings(std::vector<NativeRdb::ValuesBucket> &values)
{
    CHECK_AND_RETURN_INFO_LOG(!values.empty(), "values is empty");
    int64_t rowNum = 0;
    int32_t errCode = BatchInsertWithRetry("AnalysisPhotoMap", values, rowNum);
    if (errCode != E_OK || rowNum != static_cast<int64_t>(values.size())) {
        int64_t failNums = static_cast<int64_t>(values.size()) - rowNum;
        MEDIA_ERR_LOG("InsertAlbumMappings failed, errCode:%{public}d, fail:%{public}" PRId64, errCode, failNums);
    }
    values.clear();
}

void ClassifyRestore::CreateOrUpdateCategoryAlbums()
{
    CHECK_AND_RETURN_INFO_LOG(!albumAssetMap_.empty(), "albumAssetMap is empty");
    std::vector<NativeRdb::ValuesBucket> mapValues;
    mapValues.reserve(PAGE_SIZE);
    for (auto &entry : albumAssetMap_) {
        CHECK_AND_CONTINUE(!entry.second.empty());
        int32_t albumId = EnsureClassifyAlbumId(entry.first);
        CHECK_AND_CONTINUE(albumId > 0);
        std::vector<int32_t> assetList(entry.second.begin(), entry.second.end());
        std::sort(assetList.begin(), assetList.end());
        for (int32_t assetId : assetList) {
            NativeRdb::ValuesBucket value;
            value.PutInt("map_album", albumId);
            value.PutInt("map_asset", assetId);
            mapValues.emplace_back(value);
            if (mapValues.size() >= static_cast<size_t>(PAGE_SIZE)) {
                InsertAlbumMappings(mapValues);
            }
        }
    }
    CHECK_AND_EXECUTE(mapValues.empty(), InsertAlbumMappings(mapValues));
    albumAssetMap_.clear();
}

void ClassifyRestore::EnsureSpecialAlbums()
{
    EnsureSelfieAlbum();
    EnsureUserCommentAlbum();
}

void ClassifyRestore::EnsureSelfieAlbum()
{
    CHECK_AND_RETURN_LOG(mediaLibraryRdb_ != nullptr, "EnsureSelfieAlbum failed, rdbStore is nullptr");
    std::string selfieAlbum = std::to_string(static_cast<int32_t>(AggregateType::SELFIE_ALBUM));
    DataTransfer::MediaLibraryDbUpgrade medialibraryDbUpgrade;
    bool isSetAggregateBit = false;
    bool exists = medialibraryDbUpgrade.CheckClassifyAlbumExist(selfieAlbum, *mediaLibraryRdb_, isSetAggregateBit);
    if (exists) {
        return;
    }
    std::string querySql = "SELECT count(1) AS count FROM Photos WHERE front_camera = ?;";
    std::vector<NativeRdb::ValueObject> params { FRONT_CAMERA };
    auto resultSet = mediaLibraryRdb_->QuerySql(querySql, params);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "EnsureSelfieAlbum query failed");
    bool shouldCreate = false;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK && GetInt32Val("count", resultSet) > 0) {
        shouldCreate = true;
    }
    resultSet->Close();
    if (!shouldCreate) {
        return;
    }
    int32_t ret = medialibraryDbUpgrade.CreateClassifyAlbum(selfieAlbum, *mediaLibraryRdb_);
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK, "EnsureSelfieAlbum create failed");
}

void ClassifyRestore::EnsureUserCommentAlbum()
{
    CHECK_AND_RETURN_LOG(mediaLibraryRdb_ != nullptr, "EnsureUserCommentAlbum failed, rdbStore is nullptr");
    std::string userCommentAlbum = std::to_string(static_cast<int32_t>(AggregateType::USER_COMMENT_ALBUM));
    DataTransfer::MediaLibraryDbUpgrade medialibraryDbUpgrade;
    bool isSetAggregateBit = false;
    bool exists = medialibraryDbUpgrade.CheckClassifyAlbumExist(userCommentAlbum, *mediaLibraryRdb_, isSetAggregateBit);
    if (exists) {
        return;
    }
    std::string querySql = "SELECT count(1) AS count FROM Photos "
        "WHERE user_comment IS NOT NULL AND user_comment != '';";
    auto resultSet = mediaLibraryRdb_->QuerySql(querySql);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "EnsureUserCommentAlbum query failed");
    bool shouldCreate = false;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK && GetInt32Val("count", resultSet) > 0) {
        shouldCreate = true;
    }
    resultSet->Close();
    if (!shouldCreate) {
        return;
    }
    int32_t ret = medialibraryDbUpgrade.CreateClassifyAlbum(userCommentAlbum, *mediaLibraryRdb_);
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK, "EnsureUserCommentAlbum create failed");
}

int64_t ClassifyRestore::GetShouldEndTime(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    CHECK_AND_RETURN_RET_LOG(!taskId_.empty() && MediaLibraryDataManagerUtils::IsNumber(taskId_),
        DEFAULT_FAULT_TIME, "taskId: %{public}s invalid", taskId_.c_str());
    int64_t backupStartTime = std::stoll(taskId_) * 1000;
    int64_t dataSize = static_cast<int64_t>(photoInfoMap.size());
    MEDIA_INFO_LOG("dataSize: %{public}" PRId64 ", backupStartTime: %{public}" PRId64, dataSize, backupStartTime);
    CHECK_AND_RETURN_RET(dataSize > THRESHOLD_DATA_SIZE, backupStartTime + THRESHOLD_DATA_TIME);
    return backupStartTime + (dataSize + SUPPORT_NUMBER) / BASIC_NUMBER * SINGLE_OVER_THRESHOLD_DATA_TIME;
}

void ClassifyRestore::ProcessImageCollectionInfo(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    std::unordered_map<std::string, GalleryLabelInfo> &galleryLabelInfoMap, std::string &hash)
{
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "resultSet is nullptr");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        hash = GetStringVal("hash", resultSet);
        GalleryLabelInfo labelInfo;
        labelInfo.categoryId = GetInt32Val("category_id", resultSet);
        labelInfo.subLabel = GetStringVal("sub_label", resultSet);
        labelInfo.prob = GetDoubleVal("prob", resultSet);
        labelInfo.version = GetStringVal("version", resultSet);
        galleryLabelInfoMap[hash] = labelInfo;
    }
}

void ClassifyRestore::ProcessGalleryMediaInfo(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    std::unordered_map<std::string, GalleryLabelInfo> &galleryLabelInfoMap)
{
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "resultSet is nullptr");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string hash = GetStringVal("hash", resultSet);
        int32_t id = GetInt32Val("_id", resultSet);
        CHECK_AND_CONTINUE(galleryLabelInfoMap.find(hash) != galleryLabelInfoMap.end());
        galleryLabelInfoMap[hash].fileIdOld = id;
    }
}

void ClassifyRestore::ProcessLabelInfo(std::unordered_map<std::string, GalleryLabelInfo> &galleryLabelInfoMap,
    const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    std::vector<int32_t> labelFileIds;
    std::vector<NativeRdb::ValuesBucket> values;
    std::unordered_map<int32_t, std::vector<int32_t>> subLabelMap;
    for (const auto &pair : galleryLabelInfoMap) {
        GalleryLabelInfo galleryLabelInfo = pair.second;
        CHECK_AND_CONTINUE_INFO_LOG(photoInfoMap.find(galleryLabelInfo.fileIdOld) != photoInfoMap.end(),
            "photoInfoMap not find _id: %{public}d", galleryLabelInfo.fileIdOld);
        galleryLabelInfo.photoInfo = photoInfoMap.at(galleryLabelInfo.fileIdOld);
        labelFileIds.push_back(galleryLabelInfo.photoInfo.fileIdNew);
        TransferLabelInfo(galleryLabelInfo);
        std::vector<int32_t> labels = ParseSubLabel(galleryLabelInfo.subLabel);
        subLabelMap[galleryLabelInfo.photoInfo.fileIdNew] = labels;
        CollectAlbumInfo(galleryLabelInfo.photoInfo.fileIdNew, galleryLabelInfo.categoryId, labels);
        UpdateLabelInsertValues(values, galleryLabelInfo);
    }

    int64_t updateRows = 0;
    int errCode = BatchInsertWithRetry(ANALYSIS_LABEL_TABLE, values, updateRows);
    if (errCode != E_OK || updateRows != static_cast<int64_t>(values.size())) {
        int64_t failNums = static_cast<int64_t>(values.size()) - updateRows;
        MEDIA_ERR_LOG("RestoreLabelInfos fail, num: %{public}" PRId64, failNums);
        ErrorInfo errorInfo(RestoreError::INSERT_FAILED, static_cast<int32_t>(values.size()),
            "errCode: " + std::to_string(errCode), "RestoreLabelInfos fail.");
        UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
        failInsertLabelCnt_ += failNums;
    }
    successInsertLabelCnt_ += updateRows;
    DeleteExistMapping(labelFileIds);
    HandleOcr(subLabelMap);
    UpdateStatus(labelFileIds);
    MEDIA_INFO_LOG("RestoreLabelInfos one batch end, values count: %{public}d, updateRows: %{public}d",
        static_cast<int>(values.size()), static_cast<int>(updateRows));
}

void ClassifyRestore::RestoreLabel(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    MEDIA_INFO_LOG("RestoreLabel start");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    CHECK_AND_RETURN_LOG(galleryRdb_ != nullptr, "rdbStore is nullptr");
    int64_t shouldEndTime = GetShouldEndTime(photoInfoMap);
    std::string hash = "";
    int rowCount = 0;
    bool firstBatch = true;
    do {
        int64_t partOne = MediaFileUtils::UTCTimeMilliSeconds();
        CHECK_AND_EXECUTE(partOne <= shouldEndTime || !firstBatch, exitCode_ = BEGIN_EXIT_CODE);
        CHECK_AND_EXECUTE(partOne <= shouldEndTime || firstBatch, exitCode_ = MIDDLE_EXIT_CODE);
        CHECK_AND_RETURN_INFO_LOG(partOne <= shouldEndTime, "current time: %{public}" PRId64
            ", over shouldEndTime: %{public}" PRId64 ", RestoreLabel cost: %{public}" PRId64,
            partOne, shouldEndTime, partOne - start);
        std::vector<NativeRdb::ValueObject> params = {};
        params.push_back(NativeRdb::ValueObject(hash));
        params.push_back(NativeRdb::ValueObject(std::to_string(PAGE_SIZE)));
        std::unordered_map<std::string, GalleryLabelInfo> galleryLabelInfoMap;

        auto labelResultSet = BackupDatabaseUtils::QuerySql(galleryRdb_, QUERY_LABEL_SQL_PART_ONE, params);
        ProcessImageCollectionInfo(labelResultSet, galleryLabelInfoMap, hash);
        labelResultSet->GetRowCount(rowCount);
        labelResultSet->Close();
        int64_t partTwo = MediaFileUtils::UTCTimeMilliSeconds();

        auto idResultSet = BackupDatabaseUtils::QuerySql(galleryRdb_, QUERY_LABEL_SQL_PART_TWO, params);
        ProcessGalleryMediaInfo(idResultSet, galleryLabelInfoMap);
        idResultSet->Close();
        int64_t partThree = MediaFileUtils::UTCTimeMilliSeconds();

        ProcessLabelInfo(galleryLabelInfoMap, photoInfoMap);
        int64_t partFour = MediaFileUtils::UTCTimeMilliSeconds();

        ProcessCategoryAlbums();
        int64_t partFive = MediaFileUtils::UTCTimeMilliSeconds();

        MEDIA_INFO_LOG("One Batch ProcessImageCollectionInfo cost: %{public}" PRId64
            ", ProcessGalleryMediaInfo cost: %{public}" PRId64 ", ProcessLabelInfo cost: %{public}" PRId64
            ", ProcessCategoryAlbums cost: %{public}" PRId64 ", total cost: %{public}" PRId64,
            partTwo - partOne, partThree - partTwo, partFour - partThree, partFive - partFour, partFive - partOne);
        galleryLabelInfoMap.clear();
        firstBatch = false;
    } while (rowCount == PAGE_SIZE);
    exitCode_ = NORMAL_EXIT_CODE;
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("RestoreLabel Time cost: %{public}" PRId64, end - start);
}

void ClassifyRestore::HandleOcr(const std::unordered_map<int32_t, std::vector<int32_t>> &subLabelMap)
{
    std::vector<int32_t> fileIdsToProcess;
    for (const auto &[fileId, subLabels] : subLabelMap) {
        auto it = std::find_if(subLabels.begin(), subLabels.end(),
            [](int32_t id) { return id == static_cast<int32_t>(PhotoLabel::ID_CARD); });
        if (it != subLabels.end()) {
            fileIdsToProcess.emplace_back(fileId);
        }
    }
    HandleOcrHelper(fileIdsToProcess);
}

void ClassifyRestore::HandleOcrHelper(const std::vector<int32_t> &fileIds)
{
    for (const auto &[aggregateType, ocrTexts] : OCR_AGGREGATE_MAPPING_TABLE) {
        std::string subOcrSql = "";
        for (size_t i = 0; i < ocrTexts.size(); i++) {
            subOcrSql += "tab_analysis_ocr.ocr_text LIKE '%" + ocrTexts[i] + "%'";
            if (i != ocrTexts.size() - 1) {
                subOcrSql += " AND ";
            }
        }

        subOcrSql += "AND tab_analysis_ocr.file_id IN (" + BackupDatabaseUtils::JoinValues<int>(fileIds, ", ") + ");";
        std::string querySql = QUERY_OCR_TEXT_SQL + subOcrSql;
        auto ocrResultSet = mediaLibraryRdb_->QuerySql(querySql);
        std::unordered_set<int32_t> fileIdsToUpdateSet;
        CHECK_AND_RETURN_LOG(ocrResultSet != nullptr, "ocrResultSet is nullptr");

        while (ocrResultSet->GoToNextRow() == NativeRdb::E_OK) {
            int32_t fileId = GetInt32Val("file_id", ocrResultSet);
            fileIdsToUpdateSet.insert(fileId);
        }
        ocrResultSet->Close();
        std::vector<int32_t> fileIdsToUpdate(fileIdsToUpdateSet.begin(), fileIdsToUpdateSet.end());
        CHECK_AND_RETURN_INFO_LOG(!fileIdsToUpdate.empty(), "fileIdsToUpdate is empty");
        std::string albumName = std::to_string(static_cast<int32_t>(aggregateType));
        const std::string UPDATE_SUB_LABEL_SQL =
                    "UPDATE tab_analysis_label SET sub_label = "
                    "CASE WHEN sub_label = '[]' THEN '[" + albumName + "]' "
                    "ELSE SUBSTR(sub_label,1,LENGTH(sub_label)-1)||'," + albumName + "]' END "
                    "WHERE file_id IN (" + BackupDatabaseUtils::JoinValues<int>(fileIdsToUpdate, ", ") + ");";
        
        int32_t ret = BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, UPDATE_SUB_LABEL_SQL);
        CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK, "UPDATE_SUB_LABEL_SQL failed");
        AddIdCardAlbum(aggregateType, fileIdsToUpdateSet);
    }
}

int32_t ClassifyRestore::BatchInsertWithRetry(const std::string &tableName,
    std::vector<NativeRdb::ValuesBucket> &values, int64_t &rowNum)
{
    CHECK_AND_RETURN_RET(!values.empty(), E_OK);
    int32_t errCode = E_ERR;
    TransactionOperations trans{ __func__ };
    trans.SetBackupRdbStore(mediaLibraryRdb_);
    std::function<int(void)> func = [&]() -> int {
        errCode = trans.BatchInsert(rowNum, tableName, values);
        CHECK_AND_PRINT_LOG(errCode == E_OK,
            "InsertSql failed, errCode: %{public}d, rowNum: %{public}ld.", errCode, (long)rowNum);
        return errCode;
    };
    errCode = trans.RetryTrans(func, true);
    CHECK_AND_PRINT_LOG(errCode == E_OK, "BatchInsertWithRetry: trans finish fail!, ret: %{public}d", errCode);
    return errCode;
}

void ClassifyRestore::GetMaxIds()
{
    maxIdOfLabel_ = BackupDatabaseUtils::QueryMaxId(mediaLibraryRdb_, ANALYSIS_LABEL_TABLE, ID);
}

void ClassifyRestore::ReportRestoreTask()
{
    RestoreTaskInfo info;
    info.type = "CLASSIFY_RESTORE_IMAGE";
    info.errorCode = std::to_string(CLASSIFY_RESTORE_STATUS_SUCCESS);
    info.errorInfo =
        "max_id: " + std::to_string(maxIdOfLabel_) +
        ", timeCost: " + std::to_string(restoreTimeCost_) +
        ", exitCode: " + std::to_string(exitCode_);
    info.successCount = successInsertLabelCnt_;
    info.failedCount = failInsertLabelCnt_;
    info.duplicateCount = duplicateLabelCnt_;
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).Report(info);
}

void ClassifyRestore::AddIdCardAlbum(OcrAggregateType type, std::unordered_set<int32_t> &fileIdsToUpdateSet)
{
    std::string idCardAlbum = std::to_string(static_cast<int32_t>(type));
    albumAssetMap_[idCardAlbum] = fileIdsToUpdateSet;
    CHECK_AND_RETURN_LOG(mediaLibraryRdb_ != nullptr, "AddIdCardAlbum failed, rdbStore is nullptr");
    DataTransfer::MediaLibraryDbUpgrade medialibraryDbUpgrade;
    bool isSetAggregateBit;
    bool isExist = medialibraryDbUpgrade.CheckClassifyAlbumExist(idCardAlbum,
        *this->mediaLibraryRdb_, isSetAggregateBit);
    CHECK_AND_RETURN_INFO_LOG(!isExist, "IdCardAlbum already exist.");
    int32_t ret = medialibraryDbUpgrade.CreateClassifyAlbum(idCardAlbum, *this->mediaLibraryRdb_);
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK, "IdCardAlbum failed");
    MEDIA_INFO_LOG("IdCardAlbum success");
}
} // namespace OHOS::Media
// LCOV_EXCL_STOP
/*
 * Copyright (C) 2025-2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaLibraryCloneRestoreOcr"

#include "ocr_restore.h"

#include <algorithm>

#include "backup_database_utils.h"
#include "media_backup_report_data_type.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_rdb_transaction.h"
#include "upgrade_restore_task_report.h"

namespace OHOS::Media {
const int32_t PAGE_SIZE = 200;

// 数据量不超过THRESHOLD_DATA_SIZE时，恢复时间基线为10min
const int64_t THRESHOLD_DATA_SIZE = 30000;
const int64_t THRESHOLD_DATA_TIME = 600000;
const int64_t DEFAULT_FAULT_TIME = 0;

// 超出THRESHOLD_DATA_SIZE，每1w数据基线为216s
const int64_t BASIC_NUMBER = 10000;
const int64_t SUPPORT_NUMBER = 9999;
const int64_t SINGLE_OVER_THRESHOLD_DATA_TIME = 216000;

const int32_t OCR_RESTORE_STATUS_SUCCESS = 1;
const int32_t NORMAL_EXIT_CODE = 0; // 正常恢复
const int32_t MIDDLE_EXIT_CODE = 1; // 异常恢复，恢复过程中超时退出
const int32_t BEGIN_EXIT_CODE = 2;  // 异常恢复，恢复开始时超时退出

const std::string VERSION_PREFIX = "backup";

void OCRRestore::Init(int32_t sceneCode, const std::string &taskId,
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, std::shared_ptr<NativeRdb::RdbStore> galleryRdb)
{
    sceneCode_ = sceneCode;
    taskId_ = taskId;
    mediaLibraryRdb_ = mediaLibraryRdb;
    galleryRdb_ = galleryRdb;
}

void OCRRestore::RestoreOCR(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap, bool isCloudRestoreSatisfied)
{
    MEDIA_INFO_LOG("RestoreOCR start");
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    CHECK_AND_RETURN_LOG(galleryRdb_ != nullptr && mediaLibraryRdb_ != nullptr, "rdbStore is nullptr");
    GetMaxId();
    totalGalleryOcrRecords_ = GetTotalGalleryOcrRecords();
    CollectOcrDataProfile();
    successCnt_ = 0;
    failCnt_ = 0;
    exitCode_ = -1; // 重置退出码，避免实例复用时残留上次状态
    RestoreOCRInfos(photoInfoMap, isCloudRestoreSatisfied);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    restoreTimeCost_ = end - start;
    ReportRestoreTask();
    MEDIA_INFO_LOG("RestoreOCR Time cost: %{public}" PRId64 ", successCount: %{public}" PRId64
        ", failedCount: %{public}" PRId64, restoreTimeCost_.load(), successCnt_.load(), failCnt_.load());
}

void OCRRestore::UpdateOcrInsertValues(std::vector<NativeRdb::ValuesBucket> &values, const GalleryOCRInfo &ocrInfo)
{
    NativeRdb::ValuesBucket value;
    value.PutInt("file_id", ocrInfo.photoInfo.fileIdNew);
    value.PutString("ocr_text", ocrInfo.ocrText);
    value.PutString("ocr_version", VERSION_PREFIX + std::to_string(ocrInfo.ocrVersion));
    value.PutInt("width", ocrInfo.width);
    value.PutInt("height", ocrInfo.height);
    values.push_back(value);
}

void OCRRestore::RestoreOCRTotal(const std::vector<int32_t> &fileIds)
{
    CHECK_AND_RETURN_WARN_LOG(!fileIds.empty(), "fileIds is empty");
    stringstream ss;
    ss << "file_id IN ( ";
    bool isFirst = true;
    for (int32_t fileId : fileIds) {
        if (!isFirst) {
            ss << ",";
        }
        ss << fileId;
        isFirst = false;
    }
    ss << ") AND EXISTS (select 1 from tab_analysis_ocr where tab_analysis_ocr.file_id = tab_analysis_total.file_id)";

    std::unique_ptr<NativeRdb::AbsRdbPredicates> updatePredicates =
        std::make_unique<NativeRdb::AbsRdbPredicates>("tab_analysis_total");
    updatePredicates->SetWhereClause(ss.str());
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt("ocr", 1);
    int32_t updatedRows = 0;
    int32_t errCode = BackupDatabaseUtils::Update(mediaLibraryRdb_, updatedRows, valuesBucket, updatePredicates);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("UpdateDatabaseyStatus failed, errCode = %{public}d", errCode);
        ErrorInfo errorInfo(RestoreError::UPDATE_FAILED, 0, std::to_string(errCode), "RestoreOCRTotal fail.");
        UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
    }
    MEDIA_INFO_LOG("RestoreOCRTotal one batch end, fileId count: %{public}d, updatedRows: %{public}d",
        static_cast<int>(fileIds.size()),
        updatedRows);
}

bool OCRRestore::CheckBatchTimeout(int64_t currentTime, int64_t shouldEndTime,
    int64_t startTime, bool firstBatch)
{
    CHECK_AND_EXECUTE(currentTime <= shouldEndTime || !firstBatch, exitCode_ = BEGIN_EXIT_CODE);
    CHECK_AND_EXECUTE(currentTime <= shouldEndTime || firstBatch, exitCode_ = MIDDLE_EXIT_CODE);
    if (currentTime > shouldEndTime) {
        MEDIA_INFO_LOG("current time: %{public}s, over shouldEndTime: %{public}s , RestoreOCRInfos cost: %{public}s",
            std::to_string(currentTime).c_str(), std::to_string(shouldEndTime).c_str(),
            std::to_string(currentTime - startTime).c_str());
        return false;
    }
    return true;
}

void OCRRestore::BatchInsertAndReport(const std::vector<NativeRdb::ValuesBucket> &values)
{
    int64_t updatedRows = 0;
    int64_t valuesCount = static_cast<int64_t>(values.size());
    std::vector<NativeRdb::ValuesBucket> mutableValues(values);
    int32_t errCode = BatchInsertWithRetry("tab_analysis_ocr", mutableValues, updatedRows);
    successCnt_ += updatedRows;
    if (errCode != E_OK || updatedRows != valuesCount) {
        int64_t failNums = valuesCount - updatedRows;
        failCnt_ += failNums;
        MEDIA_ERR_LOG("RestoreOCRInfos fail, num: %{public}" PRId64, failNums);
        ErrorInfo errorInfo(RestoreError::INSERT_FAILED, static_cast<int32_t>(valuesCount),
            std::to_string(errCode), "RestoreOCRInfos fail.");
        UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
    }
    MEDIA_INFO_LOG("RestoreOCRInfos one batch end, values count: %{public}" PRId64 ", updatedRows: %{public}" PRId64,
        valuesCount, updatedRows);
}

void OCRRestore::RestoreOCRInfos(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap,
    bool isCloudRestoreSatisfied)
{
    MEDIA_INFO_LOG("Start to restore ocr info. isCloudRestoreSatisfied: %{public}d",
        static_cast<int>(isCloudRestoreSatisfied));
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t shouldEndTime = GetShouldEndTime(photoInfoMap);
    std::string querySql = "select gallery_media._id, gallery_media.hash, ocr_text, version_ocr, t_ocr_result.width, "
                           "t_ocr_result.height from t_ocr_result INNER JOIN gallery_media on t_ocr_result.hash = "
                           "gallery_media.hash and " +
                           (isCloudRestoreSatisfied ? ALL_PHOTOS_WHERE_CLAUSE : LOCAL_PHOTOS_WHERE_CLAUSE) +
                           " AND gallery_media._id > ? ORDER BY gallery_media._id ASC LIMIT ?";
    int32_t rowCount = 0;
    int32_t offset = 0;
    bool firstBatch = true;
    exitCode_ = NORMAL_EXIT_CODE;
    do {
        int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
        if (!CheckBatchTimeout(currentTime, shouldEndTime, start, firstBatch)) {
            return;
        }
        std::vector<int32_t> ocrFileIds;
        std::vector<NativeRdb::ValuesBucket> values;
        std::vector<NativeRdb::ValueObject> params = {offset, PAGE_SIZE};
        auto resultSet = BackupDatabaseUtils::QuerySql(galleryRdb_, querySql, params);
        CHECK_AND_BREAK_ERR_LOG(resultSet != nullptr, "resultSet is nullptr");
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            GalleryOCRInfo ocrInfo;
            ocrInfo.fileIdOld = GetInt32Val("_id", resultSet);
            offset = ocrInfo.fileIdOld;
            ocrInfo.hash = GetStringVal("hash", resultSet);
            CHECK_AND_CONTINUE(photoInfoMap.find(ocrInfo.fileIdOld) != photoInfoMap.end());
            ocrInfo.photoInfo = photoInfoMap.at(ocrInfo.fileIdOld);
            ocrInfo.ocrText = GetStringVal("ocr_text", resultSet);
            ocrInfo.ocrVersion = GetInt32Val("version_ocr", resultSet);
            ocrInfo.width = GetInt32Val("width", resultSet);
            ocrInfo.height = GetInt32Val("height", resultSet);
            ocrFileIds.push_back(ocrInfo.photoInfo.fileIdNew);
            UpdateOcrInsertValues(values, ocrInfo);
        }
        resultSet->GetRowCount(rowCount);
        resultSet->Close();
        BatchInsertAndReport(values);
        RestoreOCRTotal(ocrFileIds);
        firstBatch = false;
    } while (rowCount == PAGE_SIZE);
}

int32_t OCRRestore::BatchInsertWithRetry(
    const std::string &tableName, std::vector<NativeRdb::ValuesBucket> &values, int64_t &rowNum)
{
    CHECK_AND_RETURN_RET(!values.empty(), 0);
    int32_t errCode = E_ERR;
    TransactionOperations trans{__func__};
    trans.SetBackupRdbStore(mediaLibraryRdb_);
    std::function<int(void)> func = [&]() -> int {
        errCode = trans.BatchInsert(rowNum, tableName, values);
        CHECK_AND_PRINT_LOG(
            errCode == E_OK, "InsertSql failed, errCode: %{public}d, rowNum: %{public}ld.", errCode, (long)rowNum);
        return errCode;
    };
    errCode = trans.RetryTrans(func, true);
    CHECK_AND_PRINT_LOG(errCode == E_OK, "BatchInsertWithRetry: trans finish fail!, ret:%{public}d", errCode);
    return errCode;
}

int64_t OCRRestore::GetShouldEndTime(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    CHECK_AND_RETURN_RET_LOG(!taskId_.empty() && MediaLibraryDataManagerUtils::IsNumber(taskId_),
        DEFAULT_FAULT_TIME, "taskId: %{public}s invalid", taskId_.c_str());
    int64_t backupStartTime = std::stoll(taskId_) * 1000;
    int64_t dataSize = static_cast<int64_t>(photoInfoMap.size());
    MEDIA_INFO_LOG("dataSize: %{public}" PRId64 ", backupStartTime: %{public}" PRId64,
        dataSize, backupStartTime);
    // 数据量不超过阈值时，使用固定基线时间；超过阈值后按每1w数据叠加时间预算
    CHECK_AND_RETURN_RET(dataSize > THRESHOLD_DATA_SIZE, backupStartTime + THRESHOLD_DATA_TIME);
    return backupStartTime + (dataSize + SUPPORT_NUMBER) / BASIC_NUMBER
        * SINGLE_OVER_THRESHOLD_DATA_TIME;
}

int64_t OCRRestore::GetTotalGalleryOcrRecords()
{
    CHECK_AND_RETURN_RET_LOG(galleryRdb_ != nullptr, 0, "rdbStore is nullptr");
    std::string querySql = "SELECT count(1) as count FROM t_ocr_result;";
    auto resultSet = galleryRdb_->QuerySql(querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, 0, "resultSet is nullptr");
    int64_t count = 0;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        count = GetInt64Val("count", resultSet);
    }
    resultSet->Close();
    return count;
}

void OCRRestore::GetMaxId()
{
    maxId_ = BackupDatabaseUtils::QueryMaxId(mediaLibraryRdb_, "tab_analysis_ocr", "rowid");
}

void OCRRestore::CollectOcrDataProfile()
{
    CHECK_AND_RETURN_LOG(galleryRdb_ != nullptr, "rdbStore is nullptr");
    ocrDataProfile_ = OcrDataProfile {};
    QueryOcrTableAnalysis();
    QueryOcrAnomalyAnalysis();
}

void OCRRestore::QueryOcrTableAnalysis()
{
    std::vector<NativeRdb::ValueObject> params = {};
    auto resultSet = BackupDatabaseUtils::QuerySql(galleryRdb_, SQL_OCR_TABLE_ANALYSIS, params);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "QueryOcrTableAnalysis resultSet is nullptr");
    if (resultSet->GoToNextRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("QueryOcrTableAnalysis no result row");
        resultSet->Close();
        return;
    }
    ocrDataProfile_.ocrTotal = GetInt64Val("ocr_total", resultSet);
    ocrDataProfile_.ocrValidHash = GetInt64Val("ocr_valid_hash", resultSet);
    ocrDataProfile_.distinctOcrValidHash = GetInt64Val("distinct_ocr_valid_hash", resultSet);
    ocrDataProfile_.mediaTotal = GetInt64Val("media_total", resultSet);
    ocrDataProfile_.mediaValidHash = GetInt64Val("media_valid_hash", resultSet);
    ocrDataProfile_.distinctMediaValidHash = GetInt64Val("distinct_media_valid_hash", resultSet);
    ocrDataProfile_.matchedHash = GetInt64Val("matched_hash", resultSet);
    resultSet->Close();
}

void OCRRestore::QueryOcrAnomalyAnalysis()
{
    std::vector<NativeRdb::ValueObject> params = {};
    auto resultSet = BackupDatabaseUtils::QuerySql(galleryRdb_, SQL_OCR_ANOMALY_ANALYSIS, params);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "QueryOcrAnomalyAnalysis resultSet is nullptr");
    if (resultSet->GoToNextRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("QueryOcrAnomalyAnalysis no result row");
        resultSet->Close();
        return;
    }
    ocrDataProfile_.hashEmpty = GetInt64Val("hash_empty", resultSet);
    ocrDataProfile_.textEmpty = GetInt64Val("text_empty", resultSet);
    ocrDataProfile_.duplicateHashCount = GetInt64Val("duplicate_hash_count", resultSet);
    ocrDataProfile_.maxDuplicateTimes = GetInt64Val("max_duplicate_times", resultSet);
    ocrDataProfile_.hashNotExist = GetInt64Val("hash_not_exist", resultSet);
    resultSet->Close();
}

void OCRRestore::ReportRestoreTask()
{
    RestoreTaskInfo info;
    info.type = "OCR_RESTORE";
    info.errorCode = std::to_string(OCR_RESTORE_STATUS_SUCCESS);
    info.errorInfo =
        "max_id: " + std::to_string(maxId_) +
        ", timeCost: " + std::to_string(restoreTimeCost_) +
        ", exitCode: " + std::to_string(exitCode_) +
        ", totalOcrRecords: " + std::to_string(totalGalleryOcrRecords_) +
        ", ocrTotal: " + std::to_string(ocrDataProfile_.ocrTotal) +
        ", ocrValidHash: " + std::to_string(ocrDataProfile_.ocrValidHash) +
        ", distinctOcrValidHash: " + std::to_string(ocrDataProfile_.distinctOcrValidHash) +
        ", mediaTotal: " + std::to_string(ocrDataProfile_.mediaTotal) +
        ", mediaValidHash: " + std::to_string(ocrDataProfile_.mediaValidHash) +
        ", distinctMediaValidHash: " + std::to_string(ocrDataProfile_.distinctMediaValidHash) +
        ", matchedHash: " + std::to_string(ocrDataProfile_.matchedHash) +
        ", hashEmpty: " + std::to_string(ocrDataProfile_.hashEmpty) +
        ", textEmpty: " + std::to_string(ocrDataProfile_.textEmpty) +
        ", duplicateHashCount: " + std::to_string(ocrDataProfile_.duplicateHashCount) +
        ", maxDuplicateTimes: " + std::to_string(ocrDataProfile_.maxDuplicateTimes) +
        ", hashNotExist: " + std::to_string(ocrDataProfile_.hashNotExist);
    info.duplicateCount = ocrDataProfile_.duplicateHashCount;
    info.successCount = successCnt_;
    info.failedCount = failCnt_;
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).Report(info);
}
}  // namespace OHOS::Media
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

#include "ocr_restore.h"

#include "backup_database_utils.h"
#include "media_backup_report_data_type.h"
#include "media_log.h"
#include "medialibrary_rdb_transaction.h"
#include "upgrade_restore_task_report.h"

namespace OHOS::Media {
const int32_t PAGE_SIZE = 200;

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
    CHECK_AND_RETURN_LOG(galleryRdb_ != nullptr && mediaLibraryRdb_ != nullptr, "rdbStore is nullptr");
    RestoreOCRInfos(photoInfoMap, isCloudRestoreSatisfied);
}

void OCRRestore::UpdateOcrInsertValues(std::vector<NativeRdb::ValuesBucket> &values, const GalleryOCRInfo &ocrInfo)
{
    const std::string versionPrefix = "backup";
    NativeRdb::ValuesBucket value;
    value.PutInt("file_id", ocrInfo.photoInfo.fileIdNew);
    value.PutString("ocr_text", ocrInfo.ocrText);
    value.PutString("ocr_version", versionPrefix + std::to_string(ocrInfo.ocrVersion));
    value.PutInt("width", ocrInfo.width);
    value.PutInt("height", ocrInfo.height);
    values.push_back(value);
}

void OCRRestore::RestoreOCRTotal(const vector<int32_t> &fileIds)
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

void OCRRestore::RestoreOCRInfos(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap,
    bool isCloudRestoreSatisfied)
{
    MEDIA_INFO_LOG("Start to restore ocr info. isCloudRestoreSatisfied: %{public}d",
        static_cast<int>(isCloudRestoreSatisfied));
    std::string querySql = "select gallery_media._id, gallery_media.hash, ocr_text, version_ocr, t_ocr_result.width, "
                           "t_ocr_result.height from t_ocr_result INNER JOIN gallery_media on t_ocr_result.hash = "
                           "gallery_media.hash and " +
                           (isCloudRestoreSatisfied ? ALL_PHOTOS_WHERE_CLAUSE : LOCAL_PHOTOS_WHERE_CLAUSE) +
                           " AND gallery_media._id > ? ORDER BY gallery_media._id ASC LIMIT ?";
    int rowCount = 0;
    int offset = 0;
    do {
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
        int64_t updatedRows = 0;
        int errCode = BatchInsertWithRetry("tab_analysis_ocr", values, updatedRows);
        if (errCode != E_OK) {
            MEDIA_ERR_LOG("RestoreOCRInfos fail.");
            ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(errCode), "RestoreOCRInfos fail.");
            UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
        }
        MEDIA_INFO_LOG("RestoreOCRInfos one batch end, values count: %{public}d, updatedRows: %{public}d",
            static_cast<int>(values.size()),
            static_cast<int>(updatedRows));
        RestoreOCRTotal(ocrFileIds);
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
}  // namespace OHOS::Media
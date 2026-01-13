/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Media_Background"

#include "media_file_manager_temp_file_aging_task.h"

#include "rdb_predicates.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "dfx_utils.h"
#include "media_file_utils.h"
#include "photo_file_utils.h"
#include "medialibrary_subscriber.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "thumbnail_service.h"

using namespace OHOS::NativeRdb;

namespace OHOS::Media::Background {
static const int32_t batchSize = 100;
static const int32_t defaultValueZero = 0;
static const int32_t prefsNullErrCode = -1;

bool MediaFileManagerTempFileAgingTask::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}

void MediaFileManagerTempFileAgingTask::Execute()
{
    this->HandleMediaFileManagerTempFileAging();
    return;
}

void MediaFileManagerTempFileAgingTask::SetBatchStatus(int32_t startFileId)
{
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(FILE_MANAGER_TEMP_FILE_AGING_EVENT, errCode);
    MEDIA_INFO_LOG("file_manager_temp_file_aging_events prefs errCode: %{public}d", errCode);
    CHECK_AND_RETURN_LOG(prefs != nullptr, "prefs is nullptr");
    prefs->PutInt("startFileId", startFileId);
    prefs->FlushSync();
    MEDIA_INFO_LOG("startFileId set to: %{public}d", startFileId);
}

int32_t MediaFileManagerTempFileAgingTask::GetBatchStatus()
{
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(FILE_MANAGER_TEMP_FILE_AGING_EVENT, errCode);
    MEDIA_INFO_LOG("file_manager_temp_file_aging_events prefs errCode: %{public}d", errCode);
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, prefsNullErrCode, "prefs is nullptr");
    int32_t defaultVal = 0;
    int32_t currStartFileId = prefs->GetInt("startFileId", defaultVal);
    MEDIA_INFO_LOG("currStartFileId is %{public}d", currStartFileId);
    return currStartFileId;
}

AgingFilesInfo MediaFileManagerTempFileAgingTask::QueryAgingFiles(std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
    int32_t startFileId, bool checkOmitted)
{
    AgingFilesInfo agingFilesInfo;
    int64_t current = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t timeBefore24Hours = current - 24 * 60 * 60 * 1000;
    std::string QUERY_FILE_MANAGER_TEMP_FILE_24H_BEFORE_INFO =
        "SELECT file_id, data, date_taken FROM Photos WHERE " + PhotoColumn::PHOTO_FILE_SOURCE_TYPE +
        " = " + std::to_string(static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)) + " AND " +
        PhotoColumn::MEDIA_DATE_ADDED + " <= " + std::to_string(timeBefore24Hours);
    if (checkOmitted) {
        QUERY_FILE_MANAGER_TEMP_FILE_24H_BEFORE_INFO += " AND " + MediaColumn::MEDIA_ID + " < " +
            std::to_string(startFileId);
    } else {
        QUERY_FILE_MANAGER_TEMP_FILE_24H_BEFORE_INFO += " AND " + MediaColumn::MEDIA_ID + " IN (";
        for (int32_t fileId = startFileId; fileId < startFileId + batchSize; fileId++) {
            QUERY_FILE_MANAGER_TEMP_FILE_24H_BEFORE_INFO += std::to_string(fileId);
            if (fileId != startFileId + batchSize - 1) {
                QUERY_FILE_MANAGER_TEMP_FILE_24H_BEFORE_INFO += ",";
            }
        }
        QUERY_FILE_MANAGER_TEMP_FILE_24H_BEFORE_INFO += ")";
    }
    std::shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(
        QUERY_FILE_MANAGER_TEMP_FILE_24H_BEFORE_INFO);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, agingFilesInfo, "Query not match data fails");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int columnIndex = 0;
        int32_t fileId = -1;
        if (resultSet->GetColumnIndex(MediaColumn::MEDIA_ID, columnIndex) == NativeRdb::E_OK) {
            resultSet->GetInt(columnIndex, fileId);
            agingFilesInfo.fileIds.emplace_back(std::to_string(fileId));
        }
        std::string filePath = "";
        if (resultSet->GetColumnIndex(MediaColumn::MEDIA_FILE_PATH, columnIndex) == NativeRdb::E_OK) {
            resultSet->GetString(columnIndex, filePath);
            agingFilesInfo.filePaths.emplace_back(filePath);
        }
        int64_t dateTaken = 0;
        if (resultSet->GetColumnIndex(MediaColumn::MEDIA_DATE_TAKEN, columnIndex) == NativeRdb::E_OK) {
            resultSet->GetLong(columnIndex, dateTaken);
            agingFilesInfo.dateTakens.emplace_back(std::to_string(dateTaken));
        }
        MEDIA_DEBUG_LOG("Handle file id %{public}d", fileId);
    }
    resultSet->Close();
    return agingFilesInfo;
}

void MediaFileManagerTempFileAgingTask::DeleteTempFiles(std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
    const AgingFilesInfo &agingFilesInfo)
{
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(MediaColumn::MEDIA_ID, agingFilesInfo.fileIds);
    int deleteRow = -1;
    auto ret = rdbStore->Delete(deleteRow, predicates);
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK,
        "Failed to delete temp files in db, ret = %{public}d, deleteRow is %{public}d", ret, deleteRow);
    MEDIA_INFO_LOG("Delete temp files in db, deleteRow is %{public}d", deleteRow);
    for (const std::string &path : agingFilesInfo.filePaths) {
        CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteFile(path),
            "Failed to delete temp file path: %{public}s, errno: %{public}d",
            DfxUtils::GetSafePath(path).c_str(), errno);
        std::string editDataPath = PhotoFileUtils::GetEditDataPath(path);
        if (MediaFileUtils::IsFileExists(editDataPath)) {
            CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteFile(editDataPath),
                "Failed to delete edit data path: %{public}s, errno: %{public}d",
                DfxUtils::GetSafePath(editDataPath).c_str(), errno);
        }
    }
    CHECK_AND_PRINT_LOG(ThumbnailService::GetInstance()->BatchDeleteThumbnailDirAndAstc(
        PhotoColumn::PHOTOS_TABLE, agingFilesInfo.fileIds, agingFilesInfo.filePaths, agingFilesInfo.dateTakens),
        "Failed to delete temp file thumbnail dir and astc");
}

void MediaFileManagerTempFileAgingTask::HandleMediaFileManagerTempFileAging()
{
    MEDIA_INFO_LOG("FileManagerTempFileAging Start");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is nullptr");

    std::string QUERY_MAX_FILE_ID = "SELECT MAX(file_id) as last_id FROM Photos";
    std::shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(QUERY_MAX_FILE_ID);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Query not match data fails");

    int columnIndex = 0;
    int32_t maxFileId = -1;
    CHECK_AND_RETURN_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK, "ResultSet go to first row fails");
    CHECK_AND_RETURN_LOG(resultSet->GetColumnIndex("last_id", columnIndex) == NativeRdb::E_OK,
        "ResultSet get column index fails");
    resultSet->GetInt(columnIndex, maxFileId);

    CHECK_AND_EXECUTE(MediaFileUtils::IsFileExists(FILE_MANAGER_TEMP_FILE_AGING_EVENT),
        SetBatchStatus(defaultValueZero));
    int32_t currStartFileId = GetBatchStatus();
    CHECK_AND_RETURN_LOG(currStartFileId != prefsNullErrCode, "prefs is nullptr");
    int32_t startFileId = currStartFileId == defaultValueZero ? 1 : currStartFileId;

    AgingFilesInfo omittedAgingFilesInfo = QueryAgingFiles(rdbStore, startFileId, true);
    CHECK_AND_PRINT_INFO_LOG(omittedAgingFilesInfo.fileIds.size() == 0, "Find omitted temp files num: %{public}d",
        omittedAgingFilesInfo.fileIds.size());
    CHECK_AND_EXECUTE(omittedAgingFilesInfo.fileIds.size() == 0, DeleteTempFiles(rdbStore, omittedAgingFilesInfo));
    while (startFileId <= maxFileId) {
        if (!this->Accept()) {
            MEDIA_ERR_LOG("FileManagerTempFileAging check condition failed End");
            SetBatchStatus(startFileId);
            return;
        }
        AgingFilesInfo agingFilesInfo = QueryAgingFiles(rdbStore, startFileId, false);
        CHECK_AND_EXECUTE(agingFilesInfo.fileIds.size() == 0, DeleteTempFiles(rdbStore, agingFilesInfo));
        startFileId += batchSize;
    }
    SetBatchStatus(maxFileId + 1);
    MEDIA_INFO_LOG("FileManagerTempFileAging End");
}
}  // namespace OHOS::Media::Background
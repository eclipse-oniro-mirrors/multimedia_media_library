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

#define MLOG_TAG "BatchDownloadResourcesTaskDao"

#include "batch_download_resources_task_dao.h"

#include "cloud_media_asset_types.h"

#include "photo_map_column.h"
#include "media_log.h"
#include "medialibrary_rdbstore.h"
#include "cloud_media_operation_code.h"
#include "medialibrary_unistore_manager.h"
#include "result_set.h"
#include "result_set_utils.h"
#include "photos_po_writer.h"
#include "result_set_reader.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "unordered_set"
#include "medialibrary_errno.h"
#include "rdb_predicates.h"
#include "cloud_media_common.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
#ifdef MEDIALIBRARY_CLOUD_SYNC_SERVICE_SUPPORT
using namespace OHOS::Media::CloudSync;
#endif

int32_t BatchDownloadResourcesTaskDao::AddOtherBurstIdsToFileIds(std::vector<std::string> &fileIds)
{
    CHECK_AND_RETURN_RET_LOG(!fileIds.empty(), E_ERR, "AddOtherBurstIdsToFileIds No uris");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "QueryDownloadResources Failed to get rdbStore.");
    std::string sqlBefore = "SELECT p1." + PhotoColumn::MEDIA_ID + ", p1." + PhotoColumn::PHOTO_BURST_KEY +
        ", p2." + PhotoColumn::MEDIA_ID + " AS related_file_id FROM " +
        PhotoColumn::PHOTOS_TABLE +" p1 JOIN "+ PhotoColumn::PHOTOS_TABLE +
        " p2 ON p1." + PhotoColumn::PHOTO_BURST_KEY + " = p2." + PhotoColumn::PHOTO_BURST_KEY +
        " WHERE p1." + PhotoColumn::MEDIA_ID + " IN ({0}) AND p1." + PhotoColumn::PHOTO_BURST_COVER_LEVEL +
        " = 1 AND p1." + PhotoColumn::PHOTO_SUBTYPE + " = " + to_string(static_cast<int32_t>(PhotoSubType::BURST)) +
        " AND p1." + PhotoColumn::PHOTO_BURST_KEY + " IS NOT NULL AND p1." + PhotoColumn::MEDIA_ID +
        " != p2." + PhotoColumn::MEDIA_ID;
    std::string inClause = CloudMediaCommon::ToStringWithComma(fileIds);
    std::string sql = CloudMediaCommon::FillParams(sqlBefore, {inClause});
    std::shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(sql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "Failed to query batch selected files!");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t relatedFileId = GetInt32Val("related_file_id", resultSet);
        MEDIA_INFO_LOG("BatchSelectFileDownload Add burst relatedFileId %{public}d", relatedFileId);
        fileIds.emplace_back(std::to_string(relatedFileId));
    }
    resultSet->Close();
    return NativeRdb::E_OK;
}

int32_t BatchDownloadResourcesTaskDao::QueryValidBatchDownloadPoFromPhotos(std::vector<std::string> &fileIds,
    std::vector<DownloadResourcesTaskPo> &downloadResourcesTasks)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "QueryDownloadResources Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(PhotoColumn::MEDIA_ID, fileIds);
    predicates.EqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(CloudFilePosition::POSITION_CLOUD));
    auto resultSet = rdbStore->Query(predicates, PULL_QUERY_DOWNLOAD_COLUMNS);
    // Resultset 转换成 DownloadResourcesTaskPo
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "QueryValidBatchDownloadPoFromPhotos rs is null");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        DownloadResourcesTaskPo taskPo;
        int32_t fileId = GetInt32Val(PhotoColumn::MEDIA_ID, resultSet);
        MEDIA_DEBUG_LOG("BatchSelectFileDownload Add After fileId %{public}d", fileId);
        string fileName = GetStringVal(PhotoColumn::MEDIA_NAME, resultSet);
        string filePath = GetStringVal(PhotoColumn::MEDIA_FILE_PATH, resultSet);
        taskPo.fileId = fileId;
        taskPo.fileName = fileName;
        taskPo.fileSize = GetInt64Val(PhotoColumn::MEDIA_SIZE, resultSet);
        taskPo.fileUri = MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, to_string(fileId),
            MediaFileUtils::GetExtraUri(fileName, filePath));
        taskPo.dateAdded = MediaFileUtils::UTCTimeSeconds();
        taskPo.dateFinish = 0; // default
        taskPo.downloadStatus = 0; // default
        taskPo.percent = -1; // default
        taskPo.autoPauseReason = 0; // default
        taskPo.coverLevel = GetInt32Val(PhotoColumn::PHOTO_BURST_COVER_LEVEL, resultSet); // default
        downloadResourcesTasks.emplace_back(taskPo);
    }
    resultSet->Close();
    MEDIA_INFO_LOG("BatchDownloadAction Will Add size %{public}zu", downloadResourcesTasks.size());
    return NativeRdb::E_OK;
}

int32_t BatchDownloadResourcesTaskDao::BatchInsert(int64_t &insertCount, const std::string &table,
    std::vector<NativeRdb::ValuesBucket> &initialBatchValues)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "BatchInsert Failed to get rdbStore.");
    const uint32_t TRY_TIMES = 5;
    std::vector<NativeRdb::ValuesBucket> succeedValues;
    int32_t ret = NativeRdb::E_OK;
    insertCount = 0;
    uint32_t tryCount = 0;
    if (initialBatchValues.size() == 0) {
        return ret;
    }
    while (tryCount < TRY_TIMES) {
        for (auto val = initialBatchValues.begin(); val != initialBatchValues.end();) {
            {
                int64_t rowId = 0;
                ret = rdbStore->Insert(rowId, table, *val);
                MEDIA_DEBUG_LOG("batch insert RowId %{public}" PRId64, rowId);
            }
            if (ret == NativeRdb::E_OK) {
                succeedValues.push_back(*val);
                val = initialBatchValues.erase(val);
                insertCount++;
            } else {
                val++;
            }
        }
        if (initialBatchValues.empty()) {
            break;
        } else {
            MEDIA_INFO_LOG("batch insert fail try next time, retry time is tryCount %{public}d", tryCount);
            tryCount++;
        }
    }
    if (!initialBatchValues.empty()) {
        MEDIA_ERR_LOG("batch insert fail, try too many times, %{public}zu is not inserted", initialBatchValues.size());
    }
    if (!succeedValues.empty()) {
        ret = NativeRdb::E_OK;
    }
    initialBatchValues.swap(succeedValues);
    return ret;
}

int32_t BatchDownloadResourcesTaskDao::QueryPauseDownloadingStatusResources(std::vector<std::string> &fileIds,
    std::vector<std::string> &fileIdsDownloading, std::vector<std::string> &fileIdsNotInDownloading)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "QueryPauseDownloading Failed to get rdbStore.");
    CHECK_AND_RETURN_RET_LOG(!fileIds.empty(), E_ERR, "QueryPauseDownloading No uris");
    // 处于待下载（0）、待下载暂停的（2或者5）、下载失败的（3），下载成功的（4），删除任务列表返回
    std::string inClause = CloudMediaCommon::ToStringWithComma(fileIds);
    string sql = "SELECT " + DownloadResourcesColumn::MEDIA_ID + " FROM " + DownloadResourcesColumn::TABLE
        + " WHERE " + DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS + " = "
        + std::to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_DOWNLOADING)) + " AND "
        + DownloadResourcesColumn::MEDIA_ID + " IN (" + inClause + ")";
    // SELECT file_id FROM download_resources_task_records WHERE download_status = 1 AND file_id in (1,2,3)
    std::shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(sql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "QueryPauseDownloading resultSet is null!");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        fileIdsDownloading.push_back(to_string(GetInt32Val(DownloadResourcesColumn::MEDIA_ID, resultSet)));
    }
    resultSet->Close();
    std::sort(fileIds.begin(), fileIds.end());
    std::sort(fileIdsDownloading.begin(), fileIdsDownloading.end());
    std::set_difference(fileIds.begin(), fileIds.end(), fileIdsDownloading.begin(), fileIdsDownloading.end(),
                        std::back_inserter(fileIdsNotInDownloading));
    MEDIA_INFO_LOG("QueryPauseDownloadingStatusResources After Query fileIdsDownloading Size: %{public}zu",
        fileIdsDownloading.size());
    return NativeRdb::E_OK;
}

int32_t BatchDownloadResourcesTaskDao::UpdatePauseDownloadResourcesInfo(const std::vector<std::string> &fileIds)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "UpdatePauseDownload Failed to get rdbStore.");
    CHECK_AND_RETURN_RET_INFO_LOG(!fileIds.empty(), NativeRdb::E_OK, "UpdatePauseDownload empty");
    // update download_resources_task_records set download_status = 2
    // where file_id in() AND download_status != 4 AND download_status != 3 AND download_status != 2;
    std::string inClause = CloudMediaCommon::ToStringWithComma(fileIds);
    std::string whereClauseBefore = DownloadResourcesColumn::MEDIA_ID +  " IN ({0}) AND " +
        DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS+ " != ? AND " +
        DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS+ " != ? AND " +
        DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS+ " != ?";
    std::string whereClause = CloudMediaCommon::FillParams(whereClauseBefore, {inClause});
    NativeRdb::ValuesBucket valuesBucket;

    valuesBucket.PutInt(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_PAUSE));
    valuesBucket.PutInt(DownloadResourcesColumn::MEDIA_AUTO_PAUSE_REASON,
        static_cast<int32_t>(BatchDownloadAutoPauseReasonType::TYPE_DEFAULT));
    std::vector<std::string> whereArgs = {
        to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_SUCCESS)),
        to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_FAIL)),
        to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_PAUSE))};
    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, DownloadResourcesColumn::TABLE, valuesBucket, whereClause, whereArgs);
    MEDIA_INFO_LOG("BatchSelectFileDownload Pause After ret: %{public}d, changedRows %{public}d",
        ret, changedRows);
    return ret;
}

int32_t BatchDownloadResourcesTaskDao::UpdateAllPauseDownloadResourcesInfo()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "UpdatePauseDownload Failed to get rdbStore.");
    MEDIA_INFO_LOG("BatchSelectFileDownload ALL Pause In fileid");
    // update download_resources_task_records set download_status = 2 where download_status != 4 AND download_status !=3
    NativeRdb::AbsRdbPredicates predicates(DownloadResourcesColumn::TABLE);
    NativeRdb::ValuesBucket value;
    predicates.And()->NotEqualTo(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_SUCCESS));
    predicates.And()->NotEqualTo(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_FAIL));
    predicates.And()->NotEqualTo(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_PAUSE));
    value.PutInt(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_PAUSE));
    value.PutInt(DownloadResourcesColumn::MEDIA_AUTO_PAUSE_REASON,
        static_cast<int32_t>(BatchDownloadAutoPauseReasonType::TYPE_DEFAULT));
    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, value, predicates);
    MEDIA_INFO_LOG("Pause After ret: %{public}d, changedRows %{public}d", ret, changedRows);
    return ret;
}

int32_t BatchDownloadResourcesTaskDao::UpdateStatusFailedToWaiting(const std::vector<std::string> &fileIds)
{
    CHECK_AND_RETURN_RET_LOG(!fileIds.empty(), NativeRdb::E_OK, "UpdateStatusFailedToWaiting empty");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "FailedToWaiting Failed to get rdbStore.");
    // set download_status = waiting where file_id in (1,2,3) AND download_status = fail
    std::string inClause = CloudMediaCommon::ToStringWithComma(fileIds);
    std::string whereClauseBefore = DownloadResourcesColumn::MEDIA_ID +  " IN ({0}) AND " +
        DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS+ " = ?";
    std::string whereClause = CloudMediaCommon::FillParams(whereClauseBefore, {inClause});
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_WAITING));
    valuesBucket.PutInt(DownloadResourcesColumn::MEDIA_PERCENT, -1);
    std::vector<std::string> whereArgs = {
        to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_FAIL))};
    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, DownloadResourcesColumn::TABLE, valuesBucket, whereClause, whereArgs);
    MEDIA_INFO_LOG("BatchSelectFileDownload Resume FailedToWaiting ret: %{public}d, changedRows %{public}d",
        ret, changedRows);
    return ret;
}

int32_t BatchDownloadResourcesTaskDao::UpdateStatusPauseToWaiting(const std::vector<std::string> &fileIds)
{
    CHECK_AND_RETURN_RET_LOG(!fileIds.empty(), NativeRdb::E_OK, "UpdateStatusPauseToWaiting empty");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "PauseToWaiting Failed to get rdbStore.");
     // set download_status = waiting where file_id in (1,2,3) AND
     // (download_status = 2 or download_status = 5) AND percent = -1
    std::string inClause = CloudMediaCommon::ToStringWithComma(fileIds);
    std::string whereClauseBefore = DownloadResourcesColumn::MEDIA_ID +  " IN ({0}) AND (" +
        DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS + " = ? OR " +
        DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS + " = ?) AND " +
        DownloadResourcesColumn::MEDIA_PERCENT + " = -1";
    std::string whereClause = CloudMediaCommon::FillParams(whereClauseBefore, {inClause});
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_WAITING));
    std::vector<std::string> whereArgs = {
        to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_PAUSE)),
        to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_AUTO_PAUSE))};
    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, DownloadResourcesColumn::TABLE, valuesBucket, whereClause, whereArgs);
    MEDIA_INFO_LOG("BatchSelectFileDownload Resume PauseToWaiting ret: %{public}d, changedRows %{public}d",
        ret, changedRows);
    return ret;
}

int32_t BatchDownloadResourcesTaskDao::UpdateStatusPauseToDownloading(const std::vector<std::string> &fileIds)
{
    CHECK_AND_RETURN_RET_LOG(!fileIds.empty(), NativeRdb::E_OK, "UpdateStatusPauseToDownloading empty");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "PauseToDownloading Failed to get rdbStore.");
    // set download_status = downloading where file_id in (1,2,3) AND ((download_status = 2 AND percent != -1)
    // or (download_status = 5 AND percent = -1))
    std::string inClause = CloudMediaCommon::ToStringWithComma(fileIds);
    std::string whereClauseBefore = DownloadResourcesColumn::MEDIA_ID +  " IN ({0}) AND ((" +
        DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS + " = ? AND " +
        DownloadResourcesColumn::MEDIA_PERCENT + " != -1) OR (" +
        DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS + " = ? AND " +
        DownloadResourcesColumn::MEDIA_PERCENT + " != -1))";
    std::string whereClause = CloudMediaCommon::FillParams(whereClauseBefore, {inClause});
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_DOWNLOADING));
    std::vector<std::string> whereArgs = {
        to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_PAUSE)),
        to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_AUTO_PAUSE))};
    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, DownloadResourcesColumn::TABLE, valuesBucket, whereClause, whereArgs);
    MEDIA_INFO_LOG("BatchSelectFileDownload Resume PauseToDownloading ret: %{public}d, changedRows %{public}d",
        ret, changedRows);
    return ret;
}

int32_t BatchDownloadResourcesTaskDao::UpdateResumeDownloadResourcesInfo(const std::vector<std::string> &fileIds)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    MEDIA_INFO_LOG("BatchSelectFileDownload UpdateResumeDownload Resume In fileid size %{public}zu", fileIds.size());
    int32_t ret = UpdateStatusFailedToWaiting(fileIds);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "BatchSelectFileDownload UpdateStatusFailedToWaiting Failed.");

    ret = UpdateStatusPauseToWaiting(fileIds);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "BatchSelectFileDownload UpdateStatusPauseToWaiting Failed.");

    ret = UpdateStatusPauseToDownloading(fileIds);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "BatchSelectFileDownload UpdateStatusPauseToDownloading Failed.");
    return NativeRdb::E_OK;
}

int32_t BatchDownloadResourcesTaskDao::UpdateAllStatusFailedToWaiting()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "All FailedToWaiting Failed to get rdbStore.");
    // set download_status = waiting where download_status = fail
    std::string whereClause = DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS+ " = ?";
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_WAITING));
    std::vector<std::string> whereArgs = {
        to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_FAIL))};
    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, DownloadResourcesColumn::TABLE, valuesBucket, whereClause, whereArgs);
    MEDIA_INFO_LOG("BatchSelectFileDownload Resume FailedToWaiting All ret: %{public}d, changedRows %{public}d",
        ret, changedRows);
    return ret;
}

int32_t BatchDownloadResourcesTaskDao::UpdateAllStatusPauseToWaiting()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "All PauseToWaiting Failed to get rdbStore.");
     // set download_status = waiting where download_status = 2 or download_status = 5 AND percent = -1
    std::string whereClause = "(" + DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS + " = ? OR " +
        DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS + " = ?) AND " +
        DownloadResourcesColumn::MEDIA_PERCENT + " = -1";
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_WAITING));
    std::vector<std::string> whereArgs = {
        to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_PAUSE)),
        to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_AUTO_PAUSE))};
    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, DownloadResourcesColumn::TABLE, valuesBucket, whereClause, whereArgs);
    MEDIA_INFO_LOG("BatchSelectFileDownload Resume PauseToWaiting All ret: %{public}d, changedRows %{public}d",
        ret, changedRows);
    return ret;
}

int32_t BatchDownloadResourcesTaskDao::UpdateAllStatusPauseToDownloading()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "All PauseToDownloading Failed to get rdbStore.");
    // set download_status = downloading where (download_status = 2 AND percent != -1)
    // or (download_status = 5 AND percent != -1)
    std::string whereClause = "(" + DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS + " = ? AND " +
        DownloadResourcesColumn::MEDIA_PERCENT + " != -1) OR (" +
        DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS + " = ? AND " +
        DownloadResourcesColumn::MEDIA_PERCENT + " != -1)";
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_DOWNLOADING));
    std::vector<std::string> whereArgs = {
        to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_PAUSE)),
        to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_AUTO_PAUSE))};
    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, DownloadResourcesColumn::TABLE, valuesBucket, whereClause, whereArgs);
    MEDIA_INFO_LOG("BatchSelectFileDownload Resume PauseToDownloading All ret: %{public}d, changeds %{public}d",
        ret, changedRows);
    return ret;
}

int32_t BatchDownloadResourcesTaskDao::UpdateResumeAllDownloadResourcesInfo()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    MEDIA_INFO_LOG("BatchSelectFileDownload UpdateResumeDownload Resume All");
    int32_t ret = UpdateAllStatusFailedToWaiting();
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "BatchSelectFileDownload UpdateAllStatusFailedToWaiting Failed.");

    ret = UpdateAllStatusPauseToWaiting();
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "BatchSelectFileDownload UpdateAllStatusPauseToWaiting Failed.");

    ret = UpdateAllStatusPauseToDownloading();
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "BatchSelectFileDownload UpdateAllStatusPauseToDownloading Failed.");
    return NativeRdb::E_OK;
}

int32_t BatchDownloadResourcesTaskDao::QueryCancelDownloadingStatusResources(std::vector<std::string> &fileIds,
    std::vector<std::string> &fileIdsDownloading, std::vector<std::string> &fileIdsNotInDownloading)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "QueryCancelDownloading Failed to get rdbStore.");
    CHECK_AND_RETURN_RET_LOG(!fileIds.empty(), E_ERR, "QueryCancelDownloading No uris");
    // 处于待下载（0）、待下载暂停的（2或者5）、下载失败的（3），下载成功的（4），删除任务列表返回
    std::string inClause = CloudMediaCommon::ToStringWithComma(fileIds);
    string sql = "SELECT " + DownloadResourcesColumn::MEDIA_ID + " FROM " + DownloadResourcesColumn::TABLE
        + " WHERE (" + DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS + " = "
        + std::to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_DOWNLOADING))
        + " OR (" + DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS + " = "
        + std::to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_PAUSE)) + " AND "
        + DownloadResourcesColumn::MEDIA_PERCENT + " != -1" + ") OR ("
        + DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS + " = "
        + std::to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_AUTO_PAUSE)) + " AND "
        + DownloadResourcesColumn::MEDIA_PERCENT + " != -1" + ")) AND "
        + DownloadResourcesColumn::MEDIA_ID + " IN (" + inClause + ") ORDER BY " + DownloadResourcesColumn::MEDIA_ID;
    /**
        SELECT file_id FROM download_resources_task_records WHERE (download_status = 1 or
        (download_status = 2 AND percent != -1) or (download_status = 5 AND percent != -1)) AND file_id in (1,2,3)
    */
    std::shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(sql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "QueryCancelDownloading resultSet is null!");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        fileIdsDownloading.push_back(to_string(GetInt32Val(DownloadResourcesColumn::MEDIA_ID, resultSet)));
    }
    if (resultSet != nullptr) {
        resultSet->Close();
    }
    std::sort(fileIds.begin(), fileIds.end());
    std::sort(fileIdsDownloading.begin(), fileIdsDownloading.end());
    std::set_difference(fileIds.begin(), fileIds.end(), fileIdsDownloading.begin(), fileIdsDownloading.end(),
                        std::back_inserter(fileIdsNotInDownloading));
    MEDIA_INFO_LOG("QueryCancelDownloadingStatusResources After Query fileIdsDownloading Size: %{public}zu",
        fileIdsDownloading.size());
    return NativeRdb::E_OK;
}

int32_t BatchDownloadResourcesTaskDao::DeleteCancelStateDownloadResources(const std::vector<std::string> &fileIds)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "UpdateCancelDownload Failed to get rdbStore.");
    MEDIA_INFO_LOG("BatchSelectFileDownload Cancel In fileid size %{public}zu", fileIds.size());
    CHECK_AND_RETURN_RET_LOG(!fileIds.empty(), E_ERR, "UpdateCancelDownloadResourcesInfo No uris");
    NativeRdb::AbsRdbPredicates deletePredicates = NativeRdb::AbsRdbPredicates(DownloadResourcesColumn::TABLE);
    deletePredicates.In(DownloadResourcesColumn::MEDIA_ID, fileIds);
    int32_t deletedRows = -1;
    int32_t deleteRet = rdbStore->Delete(deletedRows, deletePredicates);
    CHECK_AND_RETURN_RET_LOG(deleteRet == NativeRdb::E_OK, OHOS::Media::E_RDB, "DeleteDownloadResources Failed.");
    MEDIA_INFO_LOG("DeleteDownloadResources after ret: %{public}d, changedRows %{public}d", deleteRet, deletedRows);
    return NativeRdb::E_OK;
}

int32_t BatchDownloadResourcesTaskDao::DeleteAllDownloadResourcesInfo()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "DeleteDownloadResources Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(DownloadResourcesColumn::TABLE);
    int32_t deletedRows = -1;
    int32_t ret = rdbStore->Delete(deletedRows, predicates);
    MEDIA_INFO_LOG("BatchSelectFileDownload Cancel All ret: %{public}d, changedRows %{public}d",
        ret, deletedRows);
    return ret;
}

int32_t BatchDownloadResourcesTaskDao::UpdateExistedTasksStatus(
    std::vector<std::string> &fileIds, const int32_t status, const bool isUpdateTimeStamp)
{
    MEDIA_INFO_LOG("UpdateExistedTasksStatus In tasks size %{public}zu", fileIds.size());
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL,
        "FilterExistedDownloadResources Failed to get rdbStore.");
    CHECK_AND_RETURN_RET_INFO_LOG(!fileIds.empty(), E_ERR, "UpdateExistedTasksStatus Empty Ids");
    // update download_resources_task_records set download_status = 2 where file_id IN ('1' ,'2');
    std::string inClause = CloudMediaCommon::ToStringWithComma(fileIds);
    std::string whereClauseBefore = DownloadResourcesColumn::MEDIA_ID +  " IN ({0})";
    std::string whereClause = CloudMediaCommon::FillParams(whereClauseBefore, {inClause});
    MEDIA_INFO_LOG("UpdateExistedTasksStatus query whereClause: %{public}s", whereClause.c_str());
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS, status);
    if (status == static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_WAITING)) {
        valuesBucket.PutInt(DownloadResourcesColumn::MEDIA_PERCENT, -1);
    }
    if (isUpdateTimeStamp) {
        valuesBucket.PutLong(DownloadResourcesColumn::MEDIA_DATE_ADDED, MediaFileUtils::UTCTimeSeconds());
        valuesBucket.PutLong(DownloadResourcesColumn::MEDIA_DATE_FINISH, 0);
    }
    std::vector<std::string> whereArgs = {};
    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, DownloadResourcesColumn::TABLE, valuesBucket, whereClause, whereArgs);
    MEDIA_INFO_LOG("UpdateExistedTasksStatus update status ret: %{public}d, changedRows %{public}d",
        ret, changedRows);
    return ret;
}

int32_t BatchDownloadResourcesTaskDao::HandleDuplicateAddTask(std::vector<DownloadResourcesTaskPo> &taskPos)
{
    MEDIA_INFO_LOG("HandleDuplicateAddTask update download status");
    std::vector<std::string> toWaiting;
    std::vector<std::string> toWaitingWithTime;
    std::vector<std::string> toDownloading;

    for (auto &task : taskPos) {
        std::string fileId_ = std::to_string(task.fileId.value());
        switch (task.downloadStatus.value()) {
            case static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_WAITING):
                break;
            case static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_DOWNLOADING):
                break;
            case static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_PAUSE):
            case static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_AUTO_PAUSE):
                if (task.percent.value() == -1) {
                    toWaiting.push_back(fileId_);
                } else {
                    toDownloading.push_back(fileId_);
                }
                break;
            case static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_SUCCESS):
                toWaitingWithTime.push_back(fileId_);
                break;
            case static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_FAIL):
                toWaitingWithTime.push_back(fileId_);
                break;
            default:
                MEDIA_INFO_LOG("HandleDuplicateAddTask no process for unexcepted status");
                break;
        }
    }

    UpdateExistedTasksStatus(toWaiting, static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_WAITING), false);
    UpdateExistedTasksStatus(toWaitingWithTime,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_WAITING), true);
    UpdateExistedTasksStatus(toDownloading,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_DOWNLOADING), false);
    return NativeRdb::E_OK;
}

int32_t BatchDownloadResourcesTaskDao::ClassifyExistedDownloadTasks(std::vector<std::string> &allFileIds,
    std::vector<std::string> &newIds, std::vector<std::string> &existedIds)
{
    CHECK_AND_RETURN_RET_INFO_LOG(!allFileIds.empty(), E_ERR, "ClassifyDownloadTasks No uris");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL,
        "ClassifyExistedDownloadTasks Failed to get rdbStore.");
    std::vector<std::string> columns = {DownloadResourcesColumn::MEDIA_ID};
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(DownloadResourcesColumn::TABLE);
    predicates.In(DownloadResourcesColumn::MEDIA_ID, allFileIds);
    predicates.OrderByAsc(DownloadResourcesColumn::MEDIA_ID);
    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "Get resultSet is null");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        existedIds.push_back(to_string(GetInt32Val(DownloadResourcesColumn::MEDIA_ID, resultSet)));
    }
    resultSet->Close();
    std::sort(allFileIds.begin(), allFileIds.end());
    std::sort(existedIds.begin(), existedIds.end());
    std::set_difference(allFileIds.begin(), allFileIds.end(), existedIds.begin(), existedIds.end(),
                        std::back_inserter(newIds));
    MEDIA_INFO_LOG("ClassifyExistedDownloadTasks After Query NewIds Size: %{public}zu", newIds.size());
    return NativeRdb::E_OK;
}

int32_t BatchDownloadResourcesTaskDao::ClassifyInvalidDownloadTasks(std::vector<std::string> &newIds,
    std::vector<std::string> &invalidIds)
{
    CHECK_AND_RETURN_RET_LOG(!newIds.empty(), E_ERR, "ClassifyDownloadTasks No uris");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL,
        "ClassifyInvalidDownloadTasks Failed to get rdbStore.");

    std::vector<std::string> columns = {PhotoColumn::MEDIA_ID};
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.And()->In(PhotoColumn::MEDIA_ID, newIds);
    predicates.And()->EqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(CloudFilePosition::POSITION_CLOUD));
    predicates.And()->EqualTo(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN));
    predicates.And()->NotEqualTo(PhotoColumn::MEDIA_FILE_PATH, "");
    predicates.And()->IsNotNull(PhotoColumn::MEDIA_FILE_PATH);
    predicates.And()->GreaterThan(MediaColumn::MEDIA_SIZE, to_string(0));
    predicates.OrderByAsc(PhotoColumn::MEDIA_ID);
    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "Get resultSet is null");
    std::vector<std::string> validIds;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        validIds.push_back(to_string(GetInt32Val(PhotoColumn::MEDIA_ID, resultSet)));
    }
    resultSet->Close();
    std::sort(newIds.begin(), newIds.end());
    std::sort(validIds.begin(), validIds.end());
    std::set_difference(newIds.begin(), newIds.end(), validIds.begin(), validIds.end(),
        std::back_inserter(invalidIds));
    MEDIA_INFO_LOG("ClassifyInvalidDownloadTasks After Query InvalidIds Size: %{public}zu", invalidIds.size());
    newIds.swap(validIds);
    return NativeRdb::E_OK;
}

int32_t BatchDownloadResourcesTaskDao::HandleAddExistedDownloadTasks(std::vector<std::string> &fileIds)
{
    CHECK_AND_RETURN_RET_INFO_LOG(!fileIds.empty(), E_ERR, "HandelExistedDownloadTasks No uris");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "QueryLocalByCloudId Failed to get rdbStore.");

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(DownloadResourcesColumn::TABLE);
    predicates.In(DownloadResourcesColumn::MEDIA_ID, fileIds);
    predicates.OrderByAsc(DownloadResourcesColumn::MEDIA_ID);
    auto resultSet = rdbStore->Query(predicates, PULL_QUERY_DOWNLOAD_STATUS_COLUMNS);
    MEDIA_INFO_LOG("FilterExistedDownloadResources after Query");
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "Get resultSet is null");
    std::vector<DownloadResourcesTaskPo> taskPos;
    CloudMediaBatchDownloadResourcesStatusToTaskPo(resultSet, taskPos);
    HandleDuplicateAddTask(taskPos);
    return NativeRdb::E_OK;
}

int32_t BatchDownloadResourcesTaskDao::FromUriToAllFileIds(
    const std::vector<std::string> &uris, std::vector<std::string> &fileIds)
{
    CHECK_AND_RETURN_RET_INFO_LOG(!uris.empty(), E_ERR, "FromUriToAllFileIds No uris");
    for (const auto& uri : uris) {
        string fileId = MediaFileUri::GetPhotoId(uri);
        if (!fileId.empty() && all_of(fileId.begin(), fileId.end(), ::isdigit)) {
            fileIds.push_back(fileId);
        }
    }
    AddOtherBurstIdsToFileIds(fileIds); // 补齐fileIds包含连拍的封面id 的其他图片id
    MEDIA_INFO_LOG("BatchSelectFileDownload Get AddOtherBurstIds fileid size %{public}zu", fileIds.size());
    return NativeRdb::E_OK;
}

int32_t BatchDownloadResourcesTaskDao::QueryCloudMediaBatchDownloadResourcesStatus(
    NativeRdb::RdbPredicates &predicates, std::vector<DownloadResourcesTaskPo> &downloadResourcesTasks)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Query Status Failed to get rdbStore.");
    auto resultSet = rdbStore->Query(predicates, PULL_QUERY_DOWNLOAD_STATUS_COLUMNS);
    MEDIA_INFO_LOG("QueryCloudMediaBatchDownloadResourcesStatus after Query");
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "Get status resultSet is null");
    // Resultset  转换成 DownloadResourcesTaskPo
    CloudMediaBatchDownloadResourcesStatusToTaskPo(resultSet, downloadResourcesTasks);
    return NativeRdb::E_OK;
}

int32_t BatchDownloadResourcesTaskDao::QueryCloudMediaBatchDownloadResourcesCount(
    NativeRdb::RdbPredicates &predicates, int32_t &count)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Query Count Failed to get rdbStore.");
    auto resultSet = rdbStore->Query(predicates, {"COUNT(*) AS totalCount"});
    MEDIA_INFO_LOG("QueryCloudMediaBatchDownloadResourcesCount after Query");
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "Get Count resultSet is null");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        count = GetInt32Val("totalCount", resultSet);
    }
    resultSet->Close();
    return NativeRdb::E_OK;
}

void BatchDownloadResourcesTaskDao::CloudMediaBatchDownloadResourcesStatusToTaskPo(
    std::shared_ptr<NativeRdb::ResultSet> resultSet, std::vector<DownloadResourcesTaskPo> &downloadResourcesTasks)
{
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        DownloadResourcesTaskPo taskPo;
        taskPo.fileId = GetInt32Val(DownloadResourcesColumn::MEDIA_ID, resultSet);
        taskPo.fileName = GetStringVal(DownloadResourcesColumn::MEDIA_NAME, resultSet);
        taskPo.fileSize = GetInt64Val(DownloadResourcesColumn::MEDIA_SIZE, resultSet);
        taskPo.fileUri = GetStringVal(DownloadResourcesColumn::MEDIA_URI, resultSet);
        taskPo.dateAdded = GetInt64Val(DownloadResourcesColumn::MEDIA_DATE_ADDED, resultSet);
        taskPo.dateFinish = GetInt64Val(DownloadResourcesColumn::MEDIA_DATE_FINISH, resultSet);
        taskPo.downloadStatus = GetInt32Val(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS, resultSet);
        taskPo.percent = GetInt32Val(DownloadResourcesColumn::MEDIA_PERCENT, resultSet);
        taskPo.autoPauseReason = GetInt32Val(DownloadResourcesColumn::MEDIA_AUTO_PAUSE_REASON, resultSet);
        taskPo.coverLevel = GetInt32Val(DownloadResourcesColumn::MEDIA_COVER_LEVEL, resultSet);
        downloadResourcesTasks.emplace_back(taskPo);
    }
    resultSet->Close();
    MEDIA_INFO_LOG("BatchSelectFileDownload Get After size %{public}zu", downloadResourcesTasks.size());
}
} // namespace Media
} // namespace OHOS
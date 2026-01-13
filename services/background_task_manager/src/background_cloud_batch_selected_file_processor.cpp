/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define MLOG_TAG "BackgroundCloudBatchSelectedFileProcessor"

#include "background_cloud_batch_selected_file_processor.h"
#include <sys/statvfs.h>

#include "abs_rdb_predicates.h"
#include "cloud_sync_manager.h"
#include "common_timer_errors.h"
#include "media_column.h"
#include "download_resources_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_db_const.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "metadata_extractor.h"
#include "mimetype_utils.h"
#include "rdb_store.h"
#include "rdb_utils.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"
#include "values_bucket.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "cloud_sync_utils.h"
#include "notification_merging.h"
#include "medialibrary_data_manager_utils.h"

#ifdef HAS_BATTERY_MANAGER_PART
#include "battery_srv_client.h"
#endif
#ifdef HAS_POWER_MANAGER_PART
#include "power_mgr_client.h"
#endif
#ifdef HAS_THERMAL_MANAGER_PART
#include "thermal_mgr_client.h"
#endif
#include "power_efficiency_manager.h"
#ifdef HAS_WIFI_MANAGER_PART
#include "wifi_device.h"
#endif
#include "net_conn_client.h"
#include "power_efficiency_manager.h"

namespace OHOS {
namespace Media {
using namespace FileManagement::CloudSync;
using namespace Notification;
// The task can be performed only when the ratio of available storage capacity reaches this value
static const double ABLE_STOP_DOWNLOAD_STORAGE_FREE_RATIO = 0.05;
static const double ABLE_RESTORE_DOWNLOAD_STORAGE_FREE_RATIO = 0.10;

#ifdef HAS_BATTERY_MANAGER_PART
static const int32_t ABLE_STOP_DOWNLOAD_POWER = 20;
static const int32_t ABLE_RESTORE_DOWNLOAD_POWER = 30;
#endif

static const int64_t DOWNLOAD_ID_DEFAULT = -1;

int32_t BackgroundCloudBatchSelectedFileProcessor::downloadInterval_ = DOWNLOAD_INTERVAL;  // 1 minute
int32_t BackgroundCloudBatchSelectedFileProcessor::downloadSelectedInterval_ = DOWNLOAD_SELECTED_INTERVAL;
int32_t BackgroundCloudBatchSelectedFileProcessor::downloadDuration_ = DOWNLOAD_DURATION; // 10 seconds
recursive_mutex BackgroundCloudBatchSelectedFileProcessor::mutex_;

Utils::Timer BackgroundCloudBatchSelectedFileProcessor::batchDownloadResourceTimer_(
    "background_batch_download_processor");
uint32_t BackgroundCloudBatchSelectedFileProcessor::batchDownloadResourcesStartTimerId_ = 0;
std::unordered_map<int64_t, BackgroundCloudBatchSelectedFileProcessor::InDownloadingFileInfo>
   BackgroundCloudBatchSelectedFileProcessor::currentDownloadIdFileInfoMap_;
std::mutex BackgroundCloudBatchSelectedFileProcessor::downloadResultMutex_;
std::mutex BackgroundCloudBatchSelectedFileProcessor::mutexRunningStatus_;
std::mutex BackgroundCloudBatchSelectedFileProcessor::autoActionMutex_;
std::unordered_map<std::string, BackgroundCloudBatchSelectedFileProcessor::BatchDownloadStatus>
    BackgroundCloudBatchSelectedFileProcessor::downloadResult_;
std::unordered_map<std::string, int32_t> BackgroundCloudBatchSelectedFileProcessor::downloadFileIdAndCount_;
std::atomic<bool> BackgroundCloudBatchSelectedFileProcessor::batchDownloadTaskAdded_ = true; // 初值true被杀后能自动恢复
std::atomic<bool> BackgroundCloudBatchSelectedFileProcessor::downloadLatestFinished_ = false; // 通知栏判断
std::atomic<bool> BackgroundCloudBatchSelectedFileProcessor::batchDownloadProcessRunningStatus_{false};
int32_t BackgroundCloudBatchSelectedFileProcessor::batchDownloadQueueLimitNum_ = batchDownloadQueueLimitNumHigh;
// LCOV_EXCL_START

bool BackgroundCloudBatchSelectedFileProcessor::GetCurrentRoundInDownloadingFileIdList(std::string &fileIdsStr)
{
    unique_lock<mutex> downloadLock(downloadResultMutex_);
    CHECK_AND_RETURN_RET_INFO_LOG(!currentDownloadIdFileInfoMap_.empty(), false,
        "Not file downloading, skip get round file!");
    std::ostringstream oss;
    for (const auto& entry : currentDownloadIdFileInfoMap_) {
        oss << entry.second.fileId << ",";
    }
    downloadLock.unlock();
    if (!oss.str().empty()) {
        fileIdsStr = oss.str().substr(0, oss.str().length() - 1); // 去除最后逗号
    } else {
        fileIdsStr.clear();
    }
    return !fileIdsStr.empty();
}

bool BackgroundCloudBatchSelectedFileProcessor::GetCurrentRoundExcludeList(std::string &fileIdsStr)
{
    unique_lock<mutex> downloadLock(downloadResultMutex_);
    std::ostringstream oss;
    if (downloadResult_.size() > 0) { // 排除非下载中状态
        for (const auto& entry : downloadResult_) {
            oss << entry.first << ",";
        }
    }
    downloadLock.unlock();
    if (!oss.str().empty()) {
        fileIdsStr = oss.str().substr(0, oss.str().length() - 1); // 去除最后逗号
    } else {
        fileIdsStr.clear();
    }
    return !fileIdsStr.empty();
}

int32_t BackgroundCloudBatchSelectedFileProcessor::GetDownloadQueueSizeWithLock()
{
    unique_lock<mutex> downloadLock(downloadResultMutex_);
    int32_t size =  static_cast<int32_t>(currentDownloadIdFileInfoMap_.size());
    return size;
}

bool BackgroundCloudBatchSelectedFileProcessor::IsFileIdInCurrentRoundWithoutLock(const std::string &fileId)
{
    for (const auto& pair : currentDownloadIdFileInfoMap_) {
        if (pair.second.fileId == fileId) {
            return true;
        }
    }
    return false;
}

int64_t BackgroundCloudBatchSelectedFileProcessor::GetDownloadIdByFileIdInCurrentRound(const std::string &fileId)
{
    unique_lock<mutex> downloadLock(downloadResultMutex_);
    for (const auto& pair : currentDownloadIdFileInfoMap_) {
        if (pair.second.fileId == fileId) {
            return pair.first;
        }
    }
    return DOWNLOAD_ID_DEFAULT;
}

void BackgroundCloudBatchSelectedFileProcessor::ClassifyCurrentRoundFileIdInList(std::vector<std::string> &fileIdList,
    std::vector<int64_t> &needStopDownloadIds)
{
    for (const auto& fileId : fileIdList) {
        int64_t downloadId = GetDownloadIdByFileIdInCurrentRound(fileId);
        if (downloadId != DOWNLOAD_ID_DEFAULT) {
            needStopDownloadIds.emplace_back(downloadId);
        }
    }
}

void BackgroundCloudBatchSelectedFileProcessor::StopAllDownloadingTask(bool needClean)
{
    vector<int64_t> downloadIdList;
    unique_lock<mutex> downloadLock(downloadResultMutex_);
    for (auto &entry : currentDownloadIdFileInfoMap_) {
        downloadIdList.push_back(entry.first);
    }
    downloadLock.unlock();
    for (auto downloadId : downloadIdList) {
        StopDownloadFiles(downloadId, needClean);
    }
    ClearRoundMapInfos();
}

bool BackgroundCloudBatchSelectedFileProcessor::GetStorageFreeRatio(double &freeRatio)
{
    struct statvfs diskInfo;
    int ret = statvfs("/data/storage/el2/database", &diskInfo);
    CHECK_AND_RETURN_RET_LOG(ret == 0, false, "Get file system status information failed, err: %{public}d", ret);

    double totalSize = static_cast<double>(diskInfo.f_bsize) * static_cast<double>(diskInfo.f_blocks);
    CHECK_AND_RETURN_RET_LOG(totalSize >= 1e-9, false,
        "Get file system total size failed, totalSize=%{public}f", totalSize);

    double freeSize = static_cast<double>(diskInfo.f_bsize) * static_cast<double>(diskInfo.f_bfree);
    freeRatio = freeSize / totalSize;
    return true;
}

std::shared_ptr<NativeRdb::ResultSet> BackgroundCloudBatchSelectedFileProcessor::QueryBatchSelectedResourceFiles()
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, nullptr, "uniStore is nullptr!");
    string sql = "SELECT P." + PhotoColumn::MEDIA_FILE_PATH + ", P." + PhotoColumn::PHOTO_POSITION + ", P." +
        PhotoColumn::MEDIA_ID + ", P." + PhotoColumn::MEDIA_NAME + ", P." + MediaColumn::MEDIA_TYPE +
        " FROM " + PhotoColumn::PHOTOS_TABLE + " AS P JOIN " + DownloadResourcesColumn::TABLE + " AS D ON P." +
        PhotoColumn::MEDIA_ID + " = D." + DownloadResourcesColumn::MEDIA_ID + " WHERE " +
        PhotoColumn::PHOTO_CLEAN_FLAG + " = " + std::to_string(static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN)) +
        " AND P." + MediaColumn::MEDIA_SIZE + " > 0" +
        " AND D." + DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS + " IN (" +
        std::to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_WAITING)) + "," +
        std::to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_DOWNLOADING)) + ")";
    std::string fileIdsStr;
    if (GetCurrentRoundExcludeList(fileIdsStr)) {
        sql = sql + " AND D." + DownloadResourcesColumn::MEDIA_ID + " NOT IN (" +
            fileIdsStr + ")";
    }
    sql += " ORDER BY D." + DownloadResourcesColumn::MEDIA_PERCENT + " DESC, D." +
        DownloadResourcesColumn::MEDIA_DATE_ADDED + " ASC, P." +
        PhotoColumn::MEDIA_ID + " DESC LIMIT " + std::to_string(batchQueryLimitNum);
    /**
        SELECT P.data, P.position, P.file_id, P.display_name
        FROM Photos AS P JOIN download_resources_task_records AS D ON P.file_id = D.file_id
        WHERE clean_flag  = 0 AND P.size > 0 AND D.download_status IN (0,1)
        AND D.file_id NOT IN (xxx)
        ORDER BY D.percent DESC, D.add_time ASC, P.file_id DESC LIMIT 10;
        时间升序 保证第一批下载完下载第二批,查10个 取前五个设置为下载任务，后续第二批继续
    */
    return uniStore->QuerySql(sql);
}

void BackgroundCloudBatchSelectedFileProcessor::UpdateDBStatusInfoForSingleDownloadCompletely(int32_t fileId)
{
    std::string fileIdStr = std::to_string(fileId);
    int32_t ret = UpdateDBProgressInfoForFileId(fileIdStr, 100, MediaFileUtils::UTCTimeSeconds(),
        static_cast<int32_t>(OHOS::Media::BatchDownloadStatusType::TYPE_SUCCESS));
    MEDIA_INFO_LOG("BatchSelectFileDownload Single Download UpdateDBProgressInfo, fileId: %{public}d,"
        " ret: %{public}d", fileId, ret);
    ret = NotificationMerging::ProcessNotifyDownloadProgressInfo(DownloadAssetsNotifyType::DOWNLOAD_FINISH,
        fileId, 100); // 100 finish
    MEDIA_INFO_LOG("BatchSelectFileDownload Single Success NotifyDownloadProgressInfo, ret: %{public}d", ret);
}

void BackgroundCloudBatchSelectedFileProcessor::UpdateDBProgressStatusInfoForBatch(vector<int32_t> fileIds,
    int32_t status)
{
    if (status == static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_SUCCESS)) {
        for (int32_t fileId : fileIds) {
            std::string fileIdStr = std::to_string(fileId);
            int32_t ret = UpdateDBProgressInfoForFileId(fileIdStr, 100, MediaFileUtils::UTCTimeSeconds(), status);
            MEDIA_INFO_LOG("BatchSelectFileDownload already download UpdateDBProgressInfo, fileId: %{public}d,"
                " ret: %{public}d", fileId, ret);
            ret = NotificationMerging::ProcessNotifyDownloadProgressInfo(DownloadAssetsNotifyType::DOWNLOAD_FINISH,
                fileId, 100); // 100 finish
            MEDIA_INFO_LOG("BatchSelectFileDownload Already Success NotifyDownloadProgressInfo, ret: %{public}d", ret);
        }
    } else if (status == static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_FAIL)) {
        for (int32_t fileId : fileIds) {
            std::string fileIdStr = std::to_string(fileId);
            int32_t ret = UpdateDBProgressInfoForFileId(fileIdStr, -1, -1, status);
            MEDIA_INFO_LOG("BatchSelectFileDownload exception download UpdateDBProgressInfo, fileId: %{public}d,"
                " ret: %{public}d", fileId, ret);
            int32_t percentDB = 0;
            QueryPercentOnTaskStart(fileIdStr, percentDB);
            ret = NotificationMerging::ProcessNotifyDownloadProgressInfo(DownloadAssetsNotifyType::DOWNLOAD_FAILED,
                fileId, percentDB);
            MEDIA_INFO_LOG("BatchSelectFileDownload Already Failed NotifyDownloadProgressInfo, ret: %{public}d", ret);
        }
    }
}

int32_t BackgroundCloudBatchSelectedFileProcessor::QueryBatchDownloadFinishStatusCountFromDB(int32_t &totalValue,
    int32_t &completedValue, int32_t &failedValue)
{
    totalValue = 0;
    completedValue = 0;
    failedValue = 0;
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_RDB_STORE_NULL, "uniStore is nullptr!");
    string sql = "SELECT COUNT(*) AS total_records, SUM(CASE WHEN " +
    DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS + " = " +
    std::to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_SUCCESS)) +
    " THEN 1 ELSE 0 END) AS completed_orders, SUM(CASE WHEN " +
    DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS + " = " +
    std::to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_FAIL)) +
    " THEN 1 ELSE 0 END) AS failed_orders FROM "+ DownloadResourcesColumn::TABLE;
    std::shared_ptr<NativeRdb::ResultSet> resultSet = uniStore->QuerySql(sql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "Failed to query batch selected files!");
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        totalValue = GetInt32Val("total_records", resultSet);
        completedValue = GetInt32Val("completed_orders", resultSet);
        failedValue = GetInt32Val("failed_orders", resultSet);
    }
    resultSet->Close();
    return E_OK;
}

void BackgroundCloudBatchSelectedFileProcessor::ExitDownloadSelectedBatchResources()
{
    int32_t totalValue = 0;
    int32_t completedValue = 0;
    int32_t failedValue = 0;  // 查失败不更新通知
    CHECK_AND_RETURN(QueryBatchDownloadFinishStatusCountFromDB(totalValue, completedValue, failedValue) == E_OK);
    MEDIA_INFO_LOG("BatchDownloadProgress Exit change total:%{public}d, cur:%{public}d", totalValue, completedValue);
    if (totalValue >= 0 && totalValue == completedValue + failedValue) { // 进度已完成
        // 唯一结束出口
        MEDIA_INFO_LOG("BatchSelectFileDownload Exit");
        downloadLatestFinished_.store(true);
        DownloadLatestBatchSelectedFinished();
        return;
    }
}

void BackgroundCloudBatchSelectedFileProcessor::DownloadSelectedBatchResources()
{
    MEDIA_INFO_LOG("-- BatchSelectFileDownload Start downloading batch Round START --");
    if (BackgroundCloudBatchSelectedFileProcessor::StopProcessConditionCheck()) {
        MEDIA_INFO_LOG("-- BatchSelectFileDownload AutoStop Stop process --");
        return;
    }
    RemoveFinishedResult();
    ControlDownloadLimit();
    std::string resultStr;
    GetCurrentRoundInDownloadingFileIdList(resultStr);
    MEDIA_INFO_LOG("BatchSelectFileDownload last round fileids: %{public}s", resultStr.c_str());
    CHECK_AND_RETURN_LOG(GetDownloadQueueSizeWithLock() < batchDownloadQueueLimitNum_,
        "BatchSelectFileDownload Queue is full wait next round"); // 队列满 暂不添加
    auto resultSet = QueryBatchSelectedResourceFiles();
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Failed to query batch selected files");
    vector<std::string> pendingURIs;
    vector<int32_t> localFileIds;
    vector<int32_t> exceptionFileIds;
    ParseBatchSelectedToDoFiles(resultSet, pendingURIs, localFileIds, exceptionFileIds);
    if (resultSet != nullptr) {
        resultSet->Close();
    }
    // 当前round 过程中已下载的文件任务表状态
    UpdateDBProgressStatusInfoForBatch(localFileIds,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_SUCCESS));
    UpdateDBProgressStatusInfoForBatch(exceptionFileIds,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_FAIL));
    if (pendingURIs.empty() && localFileIds.empty() && exceptionFileIds.empty()) {
        // 全部添加 下载任务完成 最后一轮次下载中
        MEDIA_INFO_LOG("BatchSelectFileDownload Last Round downloading downloadLatestFinished_ change to true");
        if (GetDownloadQueueSizeWithLock() == 0) {
            MEDIA_INFO_LOG("BatchSelectFileDownload All Task Finish, Exit");
            downloadLatestFinished_.store(true);
            ExitDownloadSelectedBatchResources();
        }
    }
    CHECK_AND_RETURN_INFO_LOG(!pendingURIs.empty(), "No cloud files need to be downloaded this batch");
    MEDIA_INFO_LOG("BatchSelectFileDownload current round pending add size: %{public}zu queue size: %{public}d",
        pendingURIs.size(), GetDownloadQueueSizeWithLock());
    // 组装5个任务并开始
    AddTasksAndStarted(pendingURIs);
    // 滑动后处理
    MEDIA_INFO_LOG("-- BatchSelectFileDownload Start downloading batch Round END --");
}

int32_t BackgroundCloudBatchSelectedFileProcessor::AddTasksAndStarted(vector<std::string> &pendingURIs)
{
    CHECK_AND_RETURN_RET_LOG(GetDownloadQueueSizeWithLock() < batchDownloadQueueLimitNum_, E_OK,
        "BatchSelectFileDownload Queue is full tasks still running"); // 队列满 暂不添加
    int32_t canAddCount = batchDownloadQueueLimitNum_ - GetDownloadQueueSizeWithLock();
    MEDIA_INFO_LOG("BatchSelectFileDownload AddTasksAndStarted not full can add task %{public}d", canAddCount);
    for (std::string &uri : pendingURIs) { // 队列空 加到队列满或者 uri都加完
        if (canAddCount <= 0) { // 队列满
            MEDIA_INFO_LOG("BatchSelectFileDownload AddTasksAndStarted Queue Is Full");
            return E_OK;
        }
        int32_t ret = AddSelectedBatchDownloadTask(uri);
        CHECK_AND_PRINT_LOG(ret == E_OK, "Failed to add download task! err: %{public}d", ret);
        canAddCount--;
        downloadLatestFinished_.store(false); // 开始下载
    }
    return E_OK;
}

void BackgroundCloudBatchSelectedFileProcessor::ClearRoundMapInfos()
{
    MEDIA_INFO_LOG("BatchSelectFileDownload ClearRoundMapInfos IN");
    unique_lock<mutex> downloadLock(downloadResultMutex_);
    downloadResult_.clear();
    downloadFileIdAndCount_.clear();
    currentDownloadIdFileInfoMap_.clear();
}

void BackgroundCloudBatchSelectedFileProcessor::DownloadLatestBatchSelectedFinished()
{
    MEDIA_INFO_LOG("BatchSelectFileDownload DownloadLatestBatchSelectedFinished IN");
    ClearRoundMapInfos();
    MEDIA_INFO_LOG("BatchSelectFileDownload DownloadLatestBatchSelectedFinished IN after downloadLock");
    {
        lock_guard<recursive_mutex> lock(mutex_);
        MEDIA_INFO_LOG("BatchSelectFileDownload DownloadLatestBatchSelectedFinished IN after Timer Lock");
        if (batchDownloadResourcesStartTimerId_ > 0) {
            MEDIA_INFO_LOG("BatchSelectFileDownload batchDownloadResourcesStartTimerId IN");
            batchDownloadResourceTimer_.Unregister(batchDownloadResourcesStartTimerId_);
            batchDownloadResourcesStartTimerId_ = 0;
            batchDownloadResourceTimer_.Shutdown(false);
        }
    }
    MEDIA_INFO_LOG("BatchSelectFileDownload Timer Shutdown");
    if (!HaveBatchDownloadForAutoResumeTask()) {
        SetBatchDownloadAddedFlag(false);
    }
    SetBatchDownloadProcessRunningStatus(false);
    MEDIA_INFO_LOG("BatchSelectFileDownload Exit Done %{public}d", batchDownloadProcessRunningStatus_.load());
}

void BackgroundCloudBatchSelectedFileProcessor::ParseBatchSelectedToDoFiles(
    std::shared_ptr<NativeRdb::ResultSet> &resultSet, vector<std::string> &pendingURIs, vector<int32_t> &localFileIds,
    vector<int32_t> &exceptionFileIds)
{
    pendingURIs.clear();
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = GetInt32Val(PhotoColumn::MEDIA_ID, resultSet);
        std::string path = GetStringVal(PhotoColumn::MEDIA_FILE_PATH, resultSet);
        std::string displayName = GetStringVal(PhotoColumn::MEDIA_NAME, resultSet);
        if (path.empty() || displayName.empty()) {
            MEDIA_WARN_LOG("BatchSelectFileDownload Failed to get cloud file path or displayName!");
            exceptionFileIds.push_back(fileId); // 清理任务表记录 直接更新为失败状态
            continue;
        }
        int32_t position = GetInt32Val(PhotoColumn::PHOTO_POSITION, resultSet);
        // 1表示本地，2表示纯云，3表示本地和云都有
        if (position != static_cast<int32_t>(POSITION_CLOUD)) {
            MEDIA_INFO_LOG("BatchSelectFileDownload cloud file invalid position: %{public}d", position);
            localFileIds.push_back(fileId); // 清理任务表记录 已下载的 直接更新为完成状态
            continue;
        }
        MEDIA_INFO_LOG("BatchSelectFileDownload ParseBatchSelectedDownloadFiles fileId: %{public}d", fileId);
        std::string uri = "";
        uri = MediaFileUri::GetPhotoUri(to_string(fileId), path, displayName);
        MEDIA_INFO_LOG("BatchSelectFileDownload ParseBatchSelectedDownloadFiles DesensitizePath uri: %{public}s",
            MediaFileUtils::DesensitizePath(uri).c_str());
        int32_t mediaType = GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet);
        MEDIA_INFO_LOG("BatchSelectFileDownload ParseBatchSelectedDownloadFiles mediaType: %{public}d", mediaType);
        int32_t cnt = GetDownloadFileIdCnt(std::to_string(fileId));
        if (cnt < DOWNLOAD_FAIL_MAX_TIMES) {
            pendingURIs.push_back(uri);
        } else {
            // skip fileId set failed
            exceptionFileIds.push_back(fileId);
        }
        if (static_cast<int32_t>(pendingURIs.size()) >= batchDownloadQueueLimitNum_) {  // 大于5 分批
            break;
        }
    }
}

int32_t BackgroundCloudBatchSelectedFileProcessor::GetDownloadFileIdCnt(std::string fileId)
{
    unique_lock<mutex> downloadLock(downloadResultMutex_);
    if (downloadFileIdAndCount_.find(fileId) == downloadFileIdAndCount_.end()) {
        downloadFileIdAndCount_[fileId] = 0;
    }
    return downloadFileIdAndCount_[fileId];
}

void BackgroundCloudBatchSelectedFileProcessor::CheckAndUpdateDownloadFileIdCnt(std::string fileId, int32_t cnt)
{
    unique_lock<mutex> downloadLock(downloadResultMutex_);
    if (downloadFileIdAndCount_.find(fileId) == downloadFileIdAndCount_.end()) {
        downloadFileIdAndCount_[fileId] = 0;
    }
    downloadFileIdAndCount_[fileId] = cnt + 1;
}

void BackgroundCloudBatchSelectedFileProcessor::RemoveFinishedResult()
{
    MEDIA_INFO_LOG("BatchSelectFileDownload round RemoveFinishedResult");
    unique_lock<mutex> downloadLock(downloadResultMutex_); // 清理非本轮状态downloadResult键值
    for (auto it = downloadResult_.begin(); it != downloadResult_.end();) {
        bool exist = BackgroundCloudBatchSelectedFileProcessor::IsFileIdInCurrentRoundWithoutLock(it->first);
        if (!exist) {
            it = downloadResult_.erase(it);
        } else {
            exist = false;
            it++;
        }
    }
}

int32_t BackgroundCloudBatchSelectedFileProcessor::AddSelectedBatchDownloadTask(std::string &downloadFilesUri)
{
    MEDIA_INFO_LOG("BatchSelectFileDownload AddTask In fileId: %{public}s",
        MediaFileUri::GetPhotoId(downloadFilesUri).c_str());
    auto asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    CHECK_AND_RETURN_RET_LOG(asyncWorker != nullptr, E_FAIL, "Failed to get async worker instance!");
    SingleDownloadFiles downloadFile;
    downloadFile.uri = downloadFilesUri; // 单个下载
    // 双框架图片1、视频3 单框架是图片1、视频2
    downloadFile.mediaType = MEDIA_TYPE_IMAGE;
    auto *taskData = new (std::nothrow) BatchDownloadCloudFilesData(downloadFile);
    CHECK_AND_RETURN_RET_LOG(taskData != nullptr, E_NO_MEMORY,
        "Failed to alloc async data for downloading cloud files!");
    auto asyncTask = std::make_shared<MediaLibraryAsyncTask>(DownloadSelectedBatchFilesExecutor, taskData);
    asyncWorker->AddTask(asyncTask, false);
    MEDIA_INFO_LOG("BatchSelectFileDownload AddTask End fileId: %{public}s",
        MediaFileUri::GetPhotoId(downloadFilesUri).c_str());
    return E_OK;
}

int32_t BackgroundCloudBatchSelectedFileProcessor::QueryPercentOnTaskStart(std::string &fileId, int32_t &percent)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "QueryPercentOnTaskStart Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(DownloadResourcesColumn::TABLE);
    predicates.EqualTo(DownloadResourcesColumn::MEDIA_ID, fileId);
    auto resultSet = rdbStore->Query(predicates, {DownloadResourcesColumn::MEDIA_PERCENT});
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "QueryPercentOnTaskStart rs is null");
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        percent = GetInt32Val(DownloadResourcesColumn::MEDIA_PERCENT, resultSet);
        MEDIA_INFO_LOG("BatchSelectFileDownload percent resume fileId %{public}s, percent %{public}d",
            fileId.c_str(), percent);
    }
    resultSet->Close();
    percent = (percent == -1) ? 0 : percent; // -1 not start
    return NativeRdb::E_OK;
}

void BackgroundCloudBatchSelectedFileProcessor::DownloadSelectedBatchFilesExecutor(AsyncTaskData *data)
{
    auto *taskData = static_cast<BatchDownloadCloudFilesData *>(data);
    auto downloadFile = taskData->downloadFile_;
    std::string fileId = MediaFileUri::GetPhotoId(downloadFile.uri);
    CHECK_AND_RETURN_LOG(MediaLibraryDataManagerUtils::IsNumber(fileId), "Error fileId: %{public}s", fileId.c_str());
    int64_t downloadId = GetDownloadIdByFileIdInCurrentRound(fileId);
    CHECK_AND_RETURN_LOG(downloadId == DOWNLOAD_ID_DEFAULT, "Already add fileId: %{public}s", fileId.c_str());
    MEDIA_INFO_LOG("BatchSelectFileDownload Start to download cloud file, fileId: %{public}s", fileId.c_str());
    std::shared_ptr<BackgroundBatchSelectedFileDownloadCallback> downloadCallback =
        std::make_shared<BackgroundBatchSelectedFileDownloadCallback>();
    CHECK_AND_RETURN_LOG(downloadCallback != nullptr, "downloadCallback is null.");
    MEDIA_DEBUG_LOG("BatchSelectFileDownload StartFileCache Before");
    int32_t ret = CloudSyncManager::GetInstance().StartFileCache({downloadFile.uri}, downloadId,
        FieldKey::FIELDKEY_CONTENT, downloadCallback);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("BatchSelectFileDownload failed to StartFileCache, ret: %{public}d, downloadId: %{public}s.", ret,
            to_string(downloadId).c_str());
        return;
    }
    MEDIA_INFO_LOG("BatchSelectFileDownload StartFileCache downloadId: %{public}s.", to_string(downloadId).c_str());
    int32_t percentDB = 0;
    QueryPercentOnTaskStart(fileId, percentDB);
    int32_t cnt = GetDownloadFileIdCnt(fileId);
    CheckAndUpdateDownloadFileIdCnt(fileId, cnt);
    InDownloadingFileInfo currentDownloadFileInfo;
    currentDownloadFileInfo.fileId = fileId;
    currentDownloadFileInfo.percent = percentDB;
    currentDownloadFileInfo.status = BatchDownloadStatus::INIT;
    unique_lock<mutex> downloadLock(downloadResultMutex_);
    downloadResult_[fileId] = BatchDownloadStatus::INIT;
    currentDownloadIdFileInfoMap_[downloadId] = currentDownloadFileInfo;
    downloadLock.unlock();
    MEDIA_DEBUG_LOG("BatchSelectFileDownload StartFileCache END");
    // 更新任务表
    ret = UpdateDBProgressInfoForFileId(fileId, percentDB, -1,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_DOWNLOADING));
    CHECK_AND_PRINT_LOG(ret == E_OK,
        "BatchSelectFileDownload Failed to start Executor UpdateDBProgress, ret: %{public}d", ret);
    // 检查点 批量下载 通知应用 notify type 0 开始进度
    int32_t retProgress = NotificationMerging::ProcessNotifyDownloadProgressInfo(
        DownloadAssetsNotifyType::DOWNLOAD_PROGRESS, std::stoi(fileId), percentDB);
    MEDIA_INFO_LOG("BatchSelectFileDownload StartNotify DOWNLOAD_PROGRESS downloadId: %{public}" PRId64
        ", ret: %{public}d", downloadId, retProgress);
}

void BackgroundCloudBatchSelectedFileProcessor::StopDownloadFiles(int64_t downloadId, bool needClean)
{
    if (downloadId != DOWNLOAD_ID_DEFAULT) {
        int32_t ret = CloudSyncManager::GetInstance().StopFileCache(downloadId, needClean, -1);
        MEDIA_INFO_LOG("Stop downloading cloud file, err: %{public}d, downloadId: %{public}s",
            ret, to_string(downloadId).c_str());
        int32_t percent = -1;
        std::string fileId;
        unique_lock<mutex> downloadLock(downloadResultMutex_);
        bool cond = currentDownloadIdFileInfoMap_.find(downloadId) == currentDownloadIdFileInfoMap_.end();
        CHECK_AND_RETURN_WARN_LOG(!cond, "downloadId progress not update, downloadId: %{public}s,",
            to_string(downloadId).c_str());
        fileId = currentDownloadIdFileInfoMap_[downloadId].fileId;
        percent = currentDownloadIdFileInfoMap_[downloadId].percent;
        downloadLock.unlock();
        CHECK_AND_RETURN_LOG(!fileId.empty(), "fileId invalid, skip update progress");
        ret = UpdateDBProgressInfoForFileId(fileId, percent, -1, -1);
        MEDIA_INFO_LOG("BatchSelectFileDownload stop update downloadId: %{public}s, percent: %{public}d,"
            "ret: %{public}d", fileId.c_str(), percent, ret);
    }
}

int32_t BackgroundCloudBatchSelectedFileProcessor::UpdateDBProgressInfoForFileId(std::string &fileIdStr,
    int32_t percent, int64_t finishTime, int32_t status)
{
    CHECK_AND_RETURN_RET_LOG(!fileIdStr.empty(), E_ERR, "UpdateDBProgressInfoForFileId invalid uris empty");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL,
        "UpdateDBProgressInfoForFileId Failed to get rdbStore.");
    // update download_resources_task_records set percent = xx,date_finish = xx,download_status = x
    // where file_id = 1 and download_status != 4;
    NativeRdb::ValuesBucket valuesBucket;
    if (finishTime != -1) {
        valuesBucket.PutLong(DownloadResourcesColumn::MEDIA_DATE_FINISH, finishTime);
    }
    if (status != -1) {
        valuesBucket.PutInt(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS, status);
    }
    if (percent != -1) {
        valuesBucket.PutInt(DownloadResourcesColumn::MEDIA_PERCENT, percent);
    }
    if (status == static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_SUCCESS)) {
        valuesBucket.PutInt(DownloadResourcesColumn::MEDIA_AUTO_PAUSE_REASON,
            static_cast<int32_t>(BatchDownloadAutoPauseReasonType::TYPE_DEFAULT));
    }
    CHECK_AND_RETURN_RET_INFO_LOG(valuesBucket.Size() != 0, E_OK, "nothing to update!");
    std::string whereClause = DownloadResourcesColumn::MEDIA_ID +  " = ? AND " +
        DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS+ " != ?";
    std::vector<std::string> whereArgs = {fileIdStr,
        to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_SUCCESS))};
    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, DownloadResourcesColumn::TABLE, valuesBucket, whereClause, whereArgs);
    MEDIA_INFO_LOG("UpdateDBProgressInfoForFileId after update ret: %{public}d, changedRows %{public}d",
        ret, changedRows);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_RDB, "UpdateDBProgressInfoForFileId Failed");
    if (status == static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_SUCCESS)) {
        int64_t downloadId = GetDownloadIdByFileIdInCurrentRound(fileIdStr);
        unique_lock<mutex> downloadLock(downloadResultMutex_);
        currentDownloadIdFileInfoMap_.erase(downloadId);
        downloadFileIdAndCount_.erase(fileIdStr);
        downloadResult_.erase(fileIdStr);
        downloadLock.unlock();
    }
    return E_OK;
}

void BackgroundCloudBatchSelectedFileProcessor::HandleBatchSelectedRunningCallback(const DownloadProgressObj& progress)
{
    std::string fileId = MediaFileUri::GetPhotoId(progress.path);
    MEDIA_INFO_LOG("BatchSelectFileDownload RunningCallback, downloadId: %{public}" PRId64
        ", fid: %{public}s , state: %{public}d, downloadErrorType: %{public}d"
        ", downloadedSize: %{public}" PRId64 ", totalSize: %{public}" PRId64
        ", batchDownloadSize: %{public}" PRId64 ", batchTotalSize: %{public}" PRId64
        ", batchSuccNum: %{public}" PRId64 ", batchFailNum: %{public}" PRId64
        ", batchTotalNum: %{public}" PRId64 ", batchState: %{public}d",
        progress.downloadId, MediaFileUri::GetPhotoId(progress.path).c_str(),
        static_cast<int32_t>(progress.state), progress.downloadErrorType,
        progress.downloadedSize, progress.totalSize,
        progress.batchDownloadSize, progress.batchTotalSize,
        progress.batchSuccNum, progress.batchFailNum,
        progress.batchTotalNum, static_cast<int32_t>(progress.batchState));
    unique_lock<mutex> downloadLock(downloadResultMutex_);
    bool cond = (currentDownloadIdFileInfoMap_.find(progress.downloadId) == currentDownloadIdFileInfoMap_.end() ||
        downloadResult_.find(fileId) == downloadResult_.end());
    downloadLock.unlock();
    CHECK_AND_RETURN_WARN_LOG(!cond, "downloadId or uri is err, fileId: %{public}s, downloadId: %{public}s,",
        fileId.c_str(), to_string(progress.downloadId).c_str());
    if (progress.totalSize != 0) {
        int32_t percent = (100 * progress.downloadedSize) / progress.totalSize;
        int32_t percentDB = 0;
        QueryPercentOnTaskStart(fileId, percentDB);
        MEDIA_INFO_LOG("BatchSelectFileDownload RunningCallback, fileId: %{public}s, percent: %{public}d,"
            "percentDB: %{public}d", fileId.c_str(), percent, percentDB);
        CHECK_AND_RETURN_LOG(percentDB <= percent, "skip write percent fileId: %{public}s", fileId.c_str());
        downloadLock.lock();
        currentDownloadIdFileInfoMap_[progress.downloadId].percent = percent;
        downloadLock.unlock();
        int32_t retDB = UpdateDBProgressInfoForFileId(fileId, percent, -1,
            static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_DOWNLOADING));
        MEDIA_INFO_LOG("BatchSelectFileDownload RunningCallback UpdateDBProgress, ret: %{public}d", retDB);
        // 检查点 批量下载 通知应用 notify type 0 进度
        if (MediaLibraryDataManagerUtils::IsNumber(fileId)) {
            int32_t ret = NotificationMerging::ProcessNotifyDownloadProgressInfo(
                DownloadAssetsNotifyType::DOWNLOAD_PROGRESS, std::stoi(fileId), percent);
            MEDIA_INFO_LOG("BatchSelectFileDownload RunningCallback NotifyDownloadProgressInfo, ret: %{public}d", ret);
        }
    }
}

void BackgroundCloudBatchSelectedFileProcessor::HandleBatchSelectedSuccessCallback(const DownloadProgressObj& progress)
{
    std::string fileId = MediaFileUri::GetPhotoId(progress.path);
    MEDIA_INFO_LOG("BatchSelectFileDownload SuccessCallback, downloadId: %{public}" PRId64
        ", fid: %{public}s, state: %{public}d, downloadErrorType: %{public}d"
        ", downloadedSize: %{public}" PRId64 ", totalSize: %{public}" PRId64
        ", batchDownloadSize: %{public}" PRId64 ", batchTotalSize: %{public}" PRId64
        ", batchSuccNum: %{public}" PRId64 ", batchFailNum: %{public}" PRId64
        ", batchTotalNum: %{public}" PRId64 ", batchState: %{public}d",
        progress.downloadId, fileId.c_str(),
        static_cast<int32_t>(progress.state), progress.downloadErrorType,
        progress.downloadedSize, progress.totalSize,
        progress.batchDownloadSize, progress.batchTotalSize,
        progress.batchSuccNum, progress.batchFailNum,
        progress.batchTotalNum, static_cast<int32_t>(progress.batchState));
    MEDIA_INFO_LOG("BatchSelectFileDownload SuccessCallback download success, fileId: %{public}s.", fileId.c_str());
    // 更新任务表
    int32_t ret = UpdateDBProgressInfoForFileId(fileId, 100, MediaFileUtils::UTCTimeSeconds(), // 100 finish
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_SUCCESS));
    MEDIA_INFO_LOG("BatchSelectFileDownload SuccessCallback UpdateDBProgress, ret: %{public}d", ret);
    // 检查点 批量下载 通知应用 notify type 1 完成
    if (MediaLibraryDataManagerUtils::IsNumber(fileId)) {
        ret = NotificationMerging::ProcessNotifyDownloadProgressInfo(DownloadAssetsNotifyType::DOWNLOAD_FINISH,
            std::stoi(fileId), 100); // 100 finish
        MEDIA_INFO_LOG("BatchSelectFileDownload SuccessCallback NotifyDownloadProgressInfo, ret: %{public}d", ret);
    }
    unique_lock<mutex> downloadLock(downloadResultMutex_);
    bool cond = (currentDownloadIdFileInfoMap_.find(progress.downloadId) == currentDownloadIdFileInfoMap_.end() ||
        downloadResult_.find(fileId) == downloadResult_.end());
    CHECK_AND_PRINT_LOG(!cond, "downloadId or uri is err, fileId: %{public}s, downloadId: %{public}s,",
        fileId.c_str(), to_string(progress.downloadId).c_str());
    downloadResult_[fileId] = BatchDownloadStatus::SUCCESS;
    downloadFileIdAndCount_.erase(fileId);
    currentDownloadIdFileInfoMap_.erase(progress.downloadId);
    downloadLock.unlock();
}

void BackgroundCloudBatchSelectedFileProcessor::HandleBatchSelectedFailedCallback(const DownloadProgressObj& progress)
{
    std::string fileId = MediaFileUri::GetPhotoId(progress.path);
    MEDIA_INFO_LOG("BatchSelectFileDownload FailedCallback, downloadId: %{public}" PRId64
        ", fid: %{public}s , state: %{public}d, downloadErrorType: %{public}d"
        ", downloadedSize: %{public}" PRId64 ", totalSize: %{public}" PRId64
        ", batchDownloadSize: %{public}" PRId64 ", batchTotalSize: %{public}" PRId64
        ", batchSuccNum: %{public}" PRId64 ", batchFailNum: %{public}" PRId64
        ", batchTotalNum: %{public}" PRId64 ", batchState: %{public}d",
        progress.downloadId, fileId.c_str(),
        static_cast<int32_t>(progress.state), progress.downloadErrorType,
        progress.downloadedSize, progress.totalSize,
        progress.batchDownloadSize, progress.batchTotalSize,
        progress.batchSuccNum, progress.batchFailNum,
        progress.batchTotalNum, static_cast<int32_t>(progress.batchState)
        );
    MEDIA_ERR_LOG("download failed, error type: %{public}d, uri: %{public}s.", progress.downloadErrorType,
        MediaFileUtils::DesensitizePath(progress.path).c_str());
    if (GetDownloadFileIdCnt(fileId) > DOWNLOAD_FAIL_MAX_TIMES && progress.totalSize != 0) {
        int32_t percent = (100 * progress.downloadedSize) / progress.totalSize;
        MEDIA_INFO_LOG("BatchSelectFileDownload FailedCallback, percent: %{public}d", percent);
        // 更新任务表
        int32_t ret = UpdateDBProgressInfoForFileId(fileId, percent, -1,
            static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_FAIL));
        MEDIA_INFO_LOG("BatchSelectFileDownload FailedCallback UpdateDBProgress, ret: %{public}d", ret);
        unique_lock<mutex> downloadLockInside(downloadResultMutex_);
        downloadFileIdAndCount_.erase(fileId);
        downloadLockInside.unlock();
        // 检查点 批量下载 通知应用 notify type 2 失败
        if (MediaLibraryDataManagerUtils::IsNumber(fileId)) {
            ret = NotificationMerging::ProcessNotifyDownloadProgressInfo(DownloadAssetsNotifyType::DOWNLOAD_FAILED,
                std::stoi(fileId), percent);
            MEDIA_INFO_LOG("BatchSelectFileDownload FailedCallback NotifyDownloadProgressInfo, ret: %{public}d", ret);
        }
    }
    unique_lock<mutex> downloadLock(downloadResultMutex_);
    bool cond = (currentDownloadIdFileInfoMap_.find(progress.downloadId) == currentDownloadIdFileInfoMap_.end() ||
        downloadResult_.find(fileId) == downloadResult_.end());
    CHECK_AND_PRINT_LOG(!cond, "downloadId or uri is err, fileId: %{public}s, downloadId: %{public}s,",
        fileId.c_str(), to_string(progress.downloadId).c_str());
    downloadResult_.erase(fileId);
    currentDownloadIdFileInfoMap_.erase(progress.downloadId);
    downloadLock.unlock();
}

void BackgroundCloudBatchSelectedFileProcessor::HandleBatchSelectedStoppedCallback(const DownloadProgressObj& progress)
{
    std::string fileId = MediaFileUri::GetPhotoId(progress.path);
    MEDIA_INFO_LOG("BatchSelectFileDownload StoppedCallback, downloadId: %{public}" PRId64
        ", fid: %{public}s , state: %{public}d, downloadErrorType: %{public}d"
        ", downloadedSize: %{public}" PRId64 ", totalSize: %{public}" PRId64
        ", batchDownloadSize: %{public}" PRId64 ", batchTotalSize: %{public}" PRId64
        ", batchSuccNum: %{public}" PRId64 ", batchFailNum: %{public}" PRId64
        ", batchTotalNum: %{public}" PRId64 ", batchState: %{public}d",
        progress.downloadId, fileId.c_str(),
        static_cast<int32_t>(progress.state), progress.downloadErrorType,
        progress.downloadedSize, progress.totalSize,
        progress.batchDownloadSize, progress.batchTotalSize,
        progress.batchSuccNum, progress.batchFailNum,
        progress.batchTotalNum, static_cast<int32_t>(progress.batchState));
    MEDIA_ERR_LOG("download stopped, uri: %{public}s.", MediaFileUtils::DesensitizePath(progress.path).c_str());
    // 更新任务表
    if (progress.totalSize != 0) {
        int32_t percent = (100 * progress.downloadedSize) / progress.totalSize;
        MEDIA_INFO_LOG("BatchSelectFileDownload StoppedCallback, percent: %{public}d", percent);
        int32_t percentDB = 0;
        QueryPercentOnTaskStart(fileId, percentDB);
        MEDIA_INFO_LOG("BatchSelectFileDownload StoppedCallback, fileId: %{public}s, percent: %{public}d,"
            "percentDB: %{public}d", fileId.c_str(), percent, percentDB);
        CHECK_AND_RETURN_LOG(percentDB <= percent, "skip write percent fileId: %{public}s", fileId.c_str());
        int32_t ret = UpdateDBProgressInfoForFileId(fileId, percent, -1, -1);
        MEDIA_INFO_LOG("BatchSelectFileDownload StoppedCallback UpdateDBProgress, ret: %{public}d", ret);
    }
    unique_lock<mutex> downloadLock(downloadResultMutex_);
    bool cond = (currentDownloadIdFileInfoMap_.find(progress.downloadId) == currentDownloadIdFileInfoMap_.end() ||
        downloadResult_.find(fileId) == downloadResult_.end());
    CHECK_AND_PRINT_LOG(!cond, "downloadId or uri is err, fileId: %{public}s, downloadId: %{public}s,",
        fileId.c_str(), to_string(progress.downloadId).c_str());
    downloadResult_[fileId] = BatchDownloadStatus::STOPPED;
    currentDownloadIdFileInfoMap_.erase(progress.downloadId);
    downloadFileIdAndCount_.erase(fileId);
    downloadResult_.erase(fileId);
    downloadLock.unlock();
}

int32_t BackgroundCloudBatchSelectedFileProcessor::QueryBatchSelectedResourceFilesNum()
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, 0, "uniStore is nullptr!");
    string sql = "SELECT COUNT(*) AS count FROM " + DownloadResourcesColumn::TABLE
        + " WHERE "
        + DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS + " IN ("
        + std::to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_WAITING)) + ","
        + std::to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_DOWNLOADING))
        + ")";
    // SELECT COUNT(*) FROM download_resources_task_records WHERE download_status IN (0, 1, 2)
    std::shared_ptr<NativeRdb::ResultSet> resultSet = uniStore->QuerySql(sql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, 0, "Failed to query batch selected files!");
    int num = 0;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        num = GetInt32Val("count", resultSet);
    }
    if (resultSet != nullptr) {
        resultSet->Close();
    }
    return num;
}

int32_t BackgroundCloudBatchSelectedFileProcessor::QueryBatchSelectedFilesNumForAutoResume()
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, 0, "uniStore is nullptr!");
    string sql = "SELECT COUNT(*) AS count FROM " + DownloadResourcesColumn::TABLE
        + " WHERE "
        + DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS + " IN ("
        + std::to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_WAITING)) + ","
        + std::to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_AUTO_PAUSE)) + ","
        + std::to_string(static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_DOWNLOADING))
        + ")";
    // SELECT COUNT(*) FROM download_resources_task_records WHERE download_status IN (0, 1, 2)
    std::shared_ptr<NativeRdb::ResultSet> resultSet = uniStore->QuerySql(sql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, 0, "Failed to query batch selected files!");
    int num = 0;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        num = GetInt32Val("count", resultSet);
    }
    if (resultSet != nullptr) {
        resultSet->Close();
    }
    return num;
}

void BackgroundCloudBatchSelectedFileProcessor::SetBatchDownloadAddedFlag(bool status)
{
    batchDownloadTaskAdded_.store(status);
    MEDIA_INFO_LOG("BatchSelectFileDownload SetBatchDownloadAddedFlag status: %{public}d", status);
}

bool BackgroundCloudBatchSelectedFileProcessor::GetBatchDownloadAddedFlag()
{
    return batchDownloadTaskAdded_.load();
}


bool BackgroundCloudBatchSelectedFileProcessor::HaveBatchDownloadResourcesTask()
{
    MEDIA_DEBUG_LOG("BatchSelectFileDownload HaveBatchDownloadResourcesTask START");
    if (!CloudSyncUtils::IsCloudSyncSwitchOn()) {
        MEDIA_INFO_LOG("Cloud sync switch off, skip BatchSelectFileDownload");
        SetBatchDownloadAddedFlag(false);
        return false;
    }
    CHECK_AND_RETURN_RET_INFO_LOG(batchDownloadTaskAdded_, false, "no batch download start trigger");
    int32_t num = QueryBatchSelectedResourceFilesNum(); // 查询是否有需要下载 或处理的任务
    if (num == 0) {
        downloadLatestFinished_.store(true); // 之前下载已完成
        MEDIA_DEBUG_LOG("BatchDownloadProgress downloadLatestFinished_ HaveBatchDownloadResourcesTask change to true");
    } else {
        MEDIA_INFO_LOG("BatchSelectFileDownload HaveBatchDownloadResourcesTask END count num: %{public}d", num);
    }
    return (num > 0);
}

bool BackgroundCloudBatchSelectedFileProcessor::HaveBatchDownloadForAutoResumeTask()
{
    MEDIA_DEBUG_LOG("BatchSelectFileDownload HaveBatchDownloadForAutoResumeTask START");
    if (!CloudSyncUtils::IsCloudSyncSwitchOn()) {
        MEDIA_INFO_LOG("Cloud sync switch off, skip BatchSelectFileDownload");
        SetBatchDownloadAddedFlag(false);
        return false;
    }
    CHECK_AND_RETURN_RET_INFO_LOG(batchDownloadTaskAdded_, false, "no batch download start trigger");
    int32_t num = QueryBatchSelectedFilesNumForAutoResume(); // 查询是否有需要下载 或处理的任务
    if (num == 0) {
        downloadLatestFinished_.store(true); // 之前下载已完成
        MEDIA_DEBUG_LOG("BatchDownloadProgress downloadLatestFinished_ HaveBatchDownloadResourcesTask change to true");
    } else {
        MEDIA_INFO_LOG("BatchSelectFileDownload HaveBatchDownloadResourcesTask END count num: %{public}d", num);
    }
    return (num > 0);
}

bool BackgroundCloudBatchSelectedFileProcessor::IsStartTimerRunning()
{
    MEDIA_DEBUG_LOG("BatchSelectFileDownload IsStartTimerRunning IN");
    return batchDownloadResourcesStartTimerId_ > 0;
}

void BackgroundCloudBatchSelectedFileProcessor::StartBatchDownloadResourcesTimer()
{
    lock_guard<recursive_mutex> lock(mutex_);
    MEDIA_INFO_LOG("BatchSelectFileDownload StartBatchDownloadResourcesTimer START");
    CHECK_AND_EXECUTE(batchDownloadResourcesStartTimerId_ <= 0,
        batchDownloadResourceTimer_.Unregister(batchDownloadResourcesStartTimerId_));
    uint32_t ret = batchDownloadResourceTimer_.Setup();
    CHECK_AND_PRINT_LOG(ret == Utils::TIMER_ERR_OK,
        "Failed to start BatchDownloadResources cloud files timer, err: %{public}d", ret);
    batchDownloadResourcesStartTimerId_ = batchDownloadResourceTimer_.Register(DownloadSelectedBatchResources,
        downloadSelectedInterval_); // 2s 定时轮询任务
    MEDIA_INFO_LOG("BatchSelectFileDownload StartBatchDownloadResourcesTimer END");
}

void BackgroundCloudBatchSelectedFileProcessor::StopBatchDownloadResourcesTimer(bool needClean)
{
    SetBatchDownloadProcessRunningStatus(false); // 无任务 且timer 停止重新设置状态 先设置保证不重复进
    StopAllDownloadingTask(needClean);
    lock_guard<recursive_mutex> lockRec(mutex_);
    MEDIA_INFO_LOG("BatchSelectFileDownload StopBatchDownloadResourcesTimer START");
    CHECK_AND_EXECUTE(batchDownloadResourcesStartTimerId_ <= 0,
        batchDownloadResourceTimer_.Unregister(batchDownloadResourcesStartTimerId_));
    batchDownloadResourcesStartTimerId_ = 0;
    batchDownloadResourceTimer_.Shutdown(false);
    MEDIA_INFO_LOG("BatchSelectFileDownload StopBatchDownloadResourcesTimer END");
}

bool BackgroundCloudBatchSelectedFileProcessor::IsBatchDownloadProcessRunningStatus()
{
    unique_lock<std::mutex> lock(mutexRunningStatus_);
    MEDIA_DEBUG_LOG("BatchSelectFileDownload BatchDownloadProcessRunningStatus: %{public}d",
        batchDownloadProcessRunningStatus_.load());
    return batchDownloadProcessRunningStatus_.load();
}

void BackgroundCloudBatchSelectedFileProcessor::SetBatchDownloadProcessRunningStatus(bool running)
{
    unique_lock<std::mutex> lock(mutexRunningStatus_);
    batchDownloadProcessRunningStatus_.store(running);
}

bool BackgroundCloudBatchSelectedFileProcessor::StopProcessConditionCheck()
{
    int32_t num = QueryBatchSelectedResourceFilesNum();
    if (num == 0) {
        MEDIA_INFO_LOG("BatchSelectFileDownload no task to stop");
        return false;
    }
    
    BatchDownloadAutoPauseReasonType autoPauseReason = BatchDownloadAutoPauseReasonType::TYPE_DEFAULT;
    if (!BackgroundCloudBatchSelectedFileProcessor::CanAutoStopCondition(autoPauseReason)) {
        MEDIA_INFO_LOG("BatchSelectFileDownload check result: keep downloading");
        return false;
    }
    AutoStopAction(autoPauseReason);
    return true;
}

// 全量设置自动暂停
int32_t BackgroundCloudBatchSelectedFileProcessor::UpdateAllAutoPauseDownloadResourcesInfo(
    BatchDownloadAutoPauseReasonType &autoPauseReason)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "UpdatePauseDownload Failed to get rdbStore.");
    MEDIA_INFO_LOG("BatchSelectFileDownload bg ALL Pause In fileid");
    // update download_resources_task_records set download_status = 2 where download_status != 4 AND download_status !=3
    NativeRdb::AbsRdbPredicates predicates(DownloadResourcesColumn::TABLE);
    NativeRdb::ValuesBucket value;
    predicates.And()->NotEqualTo(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_SUCCESS));
    predicates.And()->NotEqualTo(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_FAIL));
    predicates.And()->NotEqualTo(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_PAUSE));
    predicates.And()->NotEqualTo(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_AUTO_PAUSE));
    value.PutInt(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_AUTO_PAUSE));
    value.PutInt(DownloadResourcesColumn::MEDIA_AUTO_PAUSE_REASON,
        static_cast<int32_t>(autoPauseReason));
    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, value, predicates);
    MEDIA_INFO_LOG("AllAutoPause After ret: %{public}d, changedRows %{public}d", ret, changedRows);
    return ret;
}

// 全量设置自动恢复
int32_t BackgroundCloudBatchSelectedFileProcessor::UpdateAllAutoResumeDownloadResourcesInfo()
{
    int32_t ret = UpdateAllStatusAutoPauseToDownloading();
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "UpdateAllStatusAutoPauseToDownloading fail");
    ret = UpdateAllStatusAutoPauseToWaiting();
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "UpdateAllStatusAutoPauseToWating fail");
    return ret;
}

int32_t BackgroundCloudBatchSelectedFileProcessor::UpdateAllStatusAutoPauseToDownloading()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "UpdatePauseDownload Failed to get rdbStore.");
    MEDIA_INFO_LOG("BatchSelectFileDownload bg ALL Pause To Downloading");
    // update download_resources_task_records set download_status = 1 where (download_status = 5 AND percent > -1)
    NativeRdb::AbsRdbPredicates predicates(DownloadResourcesColumn::TABLE);
    NativeRdb::ValuesBucket value;
    predicates.And()->EqualTo(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_AUTO_PAUSE));
    predicates.And()->GreaterThan(DownloadResourcesColumn::MEDIA_PERCENT, -1);
    value.PutInt(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_DOWNLOADING));
    value.PutInt(DownloadResourcesColumn::MEDIA_AUTO_PAUSE_REASON,
        static_cast<int32_t>(BatchDownloadAutoPauseReasonType::TYPE_DEFAULT));
    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, value, predicates);
    MEDIA_INFO_LOG("AutoResume ToDownloading ret: %{public}d, changedRows %{public}d", ret, changedRows);
    return ret;
}

int32_t BackgroundCloudBatchSelectedFileProcessor::UpdateAllStatusAutoPauseToWaiting()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "UpdatePauseDownload Failed to get rdbStore.");
    MEDIA_INFO_LOG("BatchSelectFileDownload bg ALL Pause To Waiting");
    // update download_resources_task_records set download_status = 0 where (download_status = 5 AND percent == -1)
    NativeRdb::AbsRdbPredicates predicates(DownloadResourcesColumn::TABLE);
    NativeRdb::ValuesBucket value;
    predicates.And()->EqualTo(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_AUTO_PAUSE));
    predicates.And()->EqualTo(DownloadResourcesColumn::MEDIA_PERCENT, -1);
    value.PutInt(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_WAITING));
    value.PutInt(DownloadResourcesColumn::MEDIA_AUTO_PAUSE_REASON,
        static_cast<int32_t>(BatchDownloadAutoPauseReasonType::TYPE_DEFAULT));
    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, value, predicates);
    MEDIA_INFO_LOG("AutoResume ToWaiting ret: %{public}d, changedRows %{public}d", ret, changedRows);
    return ret;
}

int32_t BackgroundCloudBatchSelectedFileProcessor::ClassifyFileIdsInDownloadResourcesTable(
    const std::vector<std::string> &fileIds, std::vector<std::string> &existedIds)
{
    CHECK_AND_RETURN_RET_INFO_LOG(!fileIds.empty(), NativeRdb::E_OK, "IsFileIdsInDownloadResourcesTable No uris");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL,
        "IsFileIdsInDownloadResourcesTable Failed to get rdbStore.");
    std::vector<std::string> columns = {DownloadResourcesColumn::MEDIA_ID};
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(DownloadResourcesColumn::TABLE);
    predicates.In(DownloadResourcesColumn::MEDIA_ID, fileIds);
    predicates.OrderByAsc(DownloadResourcesColumn::MEDIA_ID);
    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "Get resultSet is null");
    MEDIA_INFO_LOG("IsFileIdsInDownloadResourcesTable after Query Task");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        existedIds.push_back(to_string(GetInt32Val(DownloadResourcesColumn::MEDIA_ID, resultSet)));
    }
    resultSet->Close();
    return NativeRdb::E_OK;
}

int32_t BackgroundCloudBatchSelectedFileProcessor::DeleteCancelStateDownloadResources(
    const std::vector<std::string> &fileIds)
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

void BackgroundCloudBatchSelectedFileProcessor::AutoStopAction(BatchDownloadAutoPauseReasonType &autoPauseReason)
{
    unique_lock<std::mutex> lock(autoActionMutex_);
    MEDIA_INFO_LOG("BatchSelectFileDownload AutoStopAction cause: %{public}d", static_cast<int32_t>(autoPauseReason));
    // 检查点 批量下载 通知应用 notify type 4 自动暂停
    MEDIA_INFO_LOG("BatchSelectFileDownload autoPause START");
    StopAllDownloadingTask(false);
    // updateDB
    UpdateAllAutoPauseDownloadResourcesInfo(autoPauseReason);
    MEDIA_INFO_LOG("BatchSelectFileDownload autoPause END");
    TriggerStopBatchDownloadProcessor(false);
    int32_t ret = NotificationMerging::ProcessNotifyDownloadProgressInfo(
        DownloadAssetsNotifyType::DOWNLOAD_AUTO_PAUSE, -1, -1,
        static_cast<int32_t>(autoPauseReason));
    MEDIA_INFO_LOG("BatchSelectFileDownload StartNotify DOWNLOAD_AUTO_PAUSE ret: %{public}d", ret);
}

void BackgroundCloudBatchSelectedFileProcessor::AutoResumeAction()
{
    unique_lock<std::mutex> lock(autoActionMutex_);
    MEDIA_INFO_LOG("BatchSelectFileDownload AutoResumeAction");
    // updateDB
    UpdateAllAutoResumeDownloadResourcesInfo();
    // 检查点 批量下载 通知应用 notify type 5 自动恢复
    int32_t ret = NotificationMerging::ProcessNotifyDownloadProgressInfo(
        DownloadAssetsNotifyType::DOWNLOAD_AUTO_RESUME, -1, -1);
    MEDIA_INFO_LOG("BatchSelectFileDownload StartNotify DOWNLOAD_AUTO_RESUME ret: %{public}d", ret);
}

void BackgroundCloudBatchSelectedFileProcessor::TriggerAutoStopBatchDownloadResourceCheck()
{
    MEDIA_DEBUG_LOG("BatchSelectFileDownload Inmediately AutoStop check downloading: %{public}d",
        BackgroundCloudBatchSelectedFileProcessor::IsBatchDownloadProcessRunningStatus());
    if (BackgroundCloudBatchSelectedFileProcessor::IsBatchDownloadProcessRunningStatus()
        && BackgroundCloudBatchSelectedFileProcessor::GetBatchDownloadAddedFlag()
        && BackgroundCloudBatchSelectedFileProcessor::IsStartTimerRunning()) { // 在运行停止且有添加任务
        MEDIA_INFO_LOG("BatchSelectFileDownload Inmediately AutoStop Processor");
        CHECK_AND_RETURN_LOG(!StopProcessConditionCheck(),
            "BatchSelectFileDownload AutoStop satisfy, skip start download process"); // 立即自动停止
    }
}

void BackgroundCloudBatchSelectedFileProcessor::TriggerAutoResumeBatchDownloadResourceCheck()
{
    MEDIA_DEBUG_LOG("BatchSelectFileDownload Timely check downloading: %{public}d",
        BackgroundCloudBatchSelectedFileProcessor::IsBatchDownloadProcessRunningStatus());
    if (!BackgroundCloudBatchSelectedFileProcessor::IsBatchDownloadProcessRunningStatus()
        && BackgroundCloudBatchSelectedFileProcessor::GetBatchDownloadAddedFlag()) { // 停止且有添加任务且可恢复状态
        MEDIA_DEBUG_LOG("BatchSelectFileDownload Timely Check AutoResume Processor");
        BackgroundCloudBatchSelectedFileProcessor::LaunchAutoResumeBatchDownloadProcessor(); // 自动恢复
    }
}

// 自动恢复使用
void BackgroundCloudBatchSelectedFileProcessor::LaunchAutoResumeBatchDownloadProcessor()
{
    bool isProcessRunning = IsBatchDownloadProcessRunningStatus();
    if (!isProcessRunning) { // 未运行状态
        if (BackgroundCloudBatchSelectedFileProcessor::HaveBatchDownloadForAutoResumeTask() &&
            !BackgroundCloudBatchSelectedFileProcessor::IsStartTimerRunning() &&
            BackgroundCloudBatchSelectedFileProcessor::CanAutoRestoreCondition()) { // 有任务 无timer在运行 启动
            MEDIA_INFO_LOG("LaunchAutoResumeBatchDownloadProcessor Start Timer");
            AutoResumeAction();
            BackgroundCloudBatchSelectedFileProcessor::StartBatchDownloadResourcesTimer();
            SetBatchDownloadProcessRunningStatus(true); // 恢复任务
        }
    } else {
        if (BackgroundCloudBatchSelectedFileProcessor::HaveBatchDownloadForAutoResumeTask()
            && !BackgroundCloudBatchSelectedFileProcessor::IsStartTimerRunning()) { // 运行 有任务 无timer的异常恢复
            MEDIA_WARN_LOG("LaunchAutoResumeBatchDownloadProcessor exception restore");
            SetBatchDownloadProcessRunningStatus(false);
        }
    }
}

void BackgroundCloudBatchSelectedFileProcessor::LaunchBatchDownloadProcessor()
{
    CHECK_AND_RETURN_LOG(!StopProcessConditionCheck(),
        "BatchSelectFileDownload AutoStop satisfy, skip start download process");
    bool isProcessRunning = IsBatchDownloadProcessRunningStatus();
    if (!isProcessRunning) { // 未运行状态
        if (BackgroundCloudBatchSelectedFileProcessor::HaveBatchDownloadResourcesTask() &&
            !BackgroundCloudBatchSelectedFileProcessor::IsStartTimerRunning()) { // 有任务 无timer在运行 启动
            MEDIA_INFO_LOG("LaunchBatchDownloadProcessor condition satisfy Start Timer");
            SetBatchDownloadProcessRunningStatus(true);
            BackgroundCloudBatchSelectedFileProcessor::StartBatchDownloadResourcesTimer();
        }
    } else { // 在运行状态
        if (!BackgroundCloudBatchSelectedFileProcessor::HaveBatchDownloadResourcesTask()) { // 无任务
            if (BackgroundCloudBatchSelectedFileProcessor::IsStartTimerRunning()) {  // 还在run
                MEDIA_INFO_LOG("LaunchBatchDownloadProcessor BatchDownloadResources End");
                BackgroundCloudBatchSelectedFileProcessor::StopBatchDownloadResourcesTimer(false);
            }
            MEDIA_INFO_LOG("LaunchBatchDownloadProcessor no task ProcessRunningStatus switch to false");
        }
        if (BackgroundCloudBatchSelectedFileProcessor::HaveBatchDownloadResourcesTask()
            && !BackgroundCloudBatchSelectedFileProcessor::IsStartTimerRunning()) { // ?? 运行 有任务 无timer的异常恢复
            MEDIA_WARN_LOG("LaunchBatchDownloadProcessor exception restore");
            SetBatchDownloadProcessRunningStatus(false);
        }
    }
}

// 手动/自动触发/云同步删除 部分取消
void BackgroundCloudBatchSelectedFileProcessor::TriggerCancelBatchDownloadProcessor(std::vector<std::string>
    &fileIds, bool sendNotify)
{
    CHECK_AND_RETURN_INFO_LOG(!fileIds.empty(), "UpdateCancelDownload empty");
    std::vector<std::string> existedTaskFileId;
    //检查 fileIds 在任务表 有任务
    ClassifyFileIdsInDownloadResourcesTable(fileIds, existedTaskFileId);
    CHECK_AND_RETURN_INFO_LOG(!existedTaskFileId.empty(), "UpdateCancelDownload tasks empty");
    CHECK_AND_PRINT_LOG(existedTaskFileId.empty(), "UpdateCancelDownload size: %{public}zu", existedTaskFileId.size());

    if (BackgroundCloudBatchSelectedFileProcessor::IsStartTimerRunning()) {
        MEDIA_INFO_LOG("LaunchBatchDownloadProcessor TriggerCancelBatchDownloadProcessor End");
        if (GetDownloadQueueSizeWithLock() != 0) {
            std::vector<int64_t> needStopDownloadIds;
            ClassifyCurrentRoundFileIdInList(existedTaskFileId, needStopDownloadIds);
            for (auto downloadId : needStopDownloadIds) {
                StopDownloadFiles(downloadId, true);
            }
        }
    }
    int32_t ret = DeleteCancelStateDownloadResources(existedTaskFileId);
    MEDIA_INFO_LOG("BatchSelectFileDownload AutoCancel delete failed, ret %{public}d", ret);
    if (sendNotify) {
        for (auto &fileId : existedTaskFileId) {
            // 检查点 批量下载 通知应用 notify type 3 删除
            CHECK_AND_RETURN_LOG(MediaLibraryDataManagerUtils::IsNumber(fileId), "Error fileId: %{public}s",
                fileId.c_str());
            int32_t ret = NotificationMerging::ProcessNotifyDownloadProgressInfo(
                DownloadAssetsNotifyType::DOWNLOAD_ASSET_DELETE, std::stoi(fileId), -1);
            MEDIA_INFO_LOG("BatchSelectFileDownload StartNotify DOWNLOAD_ASSET_DELETE fileId: %{public}s,"
                " ret: %{public}d", fileId.c_str(), ret);
        }
    }
}

// 手动触发 部分暂停 只停止任务
void BackgroundCloudBatchSelectedFileProcessor::TriggerPauseBatchDownloadProcessor(std::vector<std::string>
    &fileIdsDownloading)
{
    CHECK_AND_RETURN_INFO_LOG(!fileIdsDownloading.empty(), "UpdatePauseDownload empty");
    if (BackgroundCloudBatchSelectedFileProcessor::IsStartTimerRunning()) {
        MEDIA_INFO_LOG("BatchSelectFileDownload TriggerPauseBatchDownloadProcessor IN");
        CHECK_AND_RETURN_INFO_LOG(GetDownloadQueueSizeWithLock() != 0,
            "Not downloading, skip StopDownloadFiles");
        std::vector<int64_t> needStopDownloadIds;
        ClassifyCurrentRoundFileIdInList(fileIdsDownloading, needStopDownloadIds);
        for (auto downloadId : needStopDownloadIds) {
            StopDownloadFiles(downloadId, false);
        }
    }
}

// 检查点 批量下载 通知应用 notify type 6 刷新
void BackgroundCloudBatchSelectedFileProcessor::NotifyRefreshProgressInfo()
{
    int32_t ret = NotificationMerging::ProcessNotifyDownloadProgressInfo(
        DownloadAssetsNotifyType::DOWNLOAD_REFRESH);
    MEDIA_INFO_LOG("BatchSelectFileDownload StartNotify DOWNLOAD_REFRESH ret: %{public}d", ret);
}

// 手动触发 全量停止和取消入口
void BackgroundCloudBatchSelectedFileProcessor::TriggerStopBatchDownloadProcessor(bool cleanCache)
{
    if (BackgroundCloudBatchSelectedFileProcessor::IsStartTimerRunning()) {
        MEDIA_INFO_LOG("LaunchBatchDownloadProcessor BatchDownloadResources End");
        BackgroundCloudBatchSelectedFileProcessor::StopBatchDownloadResourcesTimer(cleanCache);
    }
}

int32_t BackgroundCloudBatchSelectedFileProcessor::QueryAutoPauseReason(int32_t &autoStopReason)
{
    autoStopReason = -1;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "QueryAutoPauseReason Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(DownloadResourcesColumn::TABLE);
    predicates.EqualTo(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_AUTO_PAUSE));
    predicates.Limit(1);
    auto resultSet = rdbStore->Query(predicates, {DownloadResourcesColumn::MEDIA_AUTO_PAUSE_REASON});
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "QueryAutoPauseReason rs is null");
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        autoStopReason = GetInt32Val(DownloadResourcesColumn::MEDIA_AUTO_PAUSE_REASON, resultSet);
        MEDIA_INFO_LOG("BatchSelectFileDownload autostop reason  %{public}d", autoStopReason);
    }
    resultSet->Close();
    return NativeRdb::E_OK;
}

int32_t BackgroundCloudBatchSelectedFileProcessor::UpdateAllAutoPauseReason(int32_t autoStopReason)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "UpdatePauseDownload Failed to get rdbStore.");
    MEDIA_INFO_LOG("BatchSelectFileDownload bg ALL Pause To Downloading");
    NativeRdb::AbsRdbPredicates predicates(DownloadResourcesColumn::TABLE);
    NativeRdb::ValuesBucket value;
    predicates.EqualTo(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_AUTO_PAUSE));
    value.PutInt(DownloadResourcesColumn::MEDIA_AUTO_PAUSE_REASON, autoStopReason);
    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, value, predicates);
    MEDIA_INFO_LOG("UpdateAllAutoPauseReason ret: %{public}d, changedRows %{public}d", ret, changedRows);
    return ret;
}

void BackgroundCloudBatchSelectedFileProcessor::RefreshNotRestoreReason(vector<int32_t>
    &notRestoreReasons)
{
    CHECK_AND_RETURN_INFO_LOG(!notRestoreReasons.empty(), "NotRestoreReasons empty");
    int32_t autoStopReason = -1;
    QueryAutoPauseReason(autoStopReason);
    CHECK_AND_RETURN_INFO_LOG(autoStopReason != -1, "No AutoStop Task"); // 非自动停止状态 跳过 无需刷新
    if (std::find(notRestoreReasons.begin(), notRestoreReasons.end(), autoStopReason) == notRestoreReasons.end()) {
        int32_t reason = notRestoreReasons.front();  // 刷新为新原因 并通知
        UpdateAllAutoPauseReason(reason);
        int32_t ret = NotificationMerging::ProcessNotifyDownloadProgressInfo(
            DownloadAssetsNotifyType::DOWNLOAD_AUTO_PAUSE, -1, -1, reason);
        MEDIA_INFO_LOG("BatchSelectFileDownload StartNotify NotRestore DOWNLOAD_AUTO_PAUSE ret: %{public}d", ret);
    }
}

// 自动停止 网络不满足 电量20- rom 可用10以下 任意满足
bool BackgroundCloudBatchSelectedFileProcessor::CanAutoStopCondition(BatchDownloadAutoPauseReasonType &autoPauseReason)
{
    bool netValidated = MedialibraryRelatedSystemStateManager::GetInstance()->IsNetValidatedAtRealTime();
    bool isNetworkAvailable =
        MedialibraryRelatedSystemStateManager::GetInstance()->IsNetAvailableWithUnlimitCondition();
    if (!isNetworkAvailable) {
        autoPauseReason = netValidated ?
            BatchDownloadAutoPauseReasonType::TYPE_CELLNET_LIMIT :
            BatchDownloadAutoPauseReasonType::TYPE_NETWORK_DISCONNECT;
        return true;
    }
    bool isPowerSufficient = true;
    #ifdef HAS_BATTERY_MANAGER_PART
        int32_t batteryCapacity = PowerMgr::BatterySrvClient::GetInstance().GetCapacity();
        isPowerSufficient = batteryCapacity > ABLE_STOP_DOWNLOAD_POWER;
        if (!isPowerSufficient) {
            autoPauseReason = BatchDownloadAutoPauseReasonType::TYPE_POWER_LOW;
            return true;
        }
    #endif
    double freeRatio = 0.0;
    BackgroundCloudBatchSelectedFileProcessor::GetStorageFreeRatio(freeRatio);
    bool isDiskEnough = freeRatio > ABLE_STOP_DOWNLOAD_STORAGE_FREE_RATIO;
    if (!isDiskEnough) {
        autoPauseReason = BatchDownloadAutoPauseReasonType::TYPE_ROM_LOW;
        return true;
    }
    bool isCloudSyncOn = CloudSyncUtils::IsCloudSyncSwitchOn();
    bool ableAutoStopDownload = !(isCloudSyncOn && isNetworkAvailable && isPowerSufficient && isDiskEnough);
    MEDIA_DEBUG_LOG("BatchSelectFileDownloadAuto AutoStopCondition ableAutoStopDownload: %{public}d, "
        "isNetworkAvailable: %{public}d, power: %{public}d, disk: %{public}d, cloudsync: %{public}d",
        ableAutoStopDownload, isNetworkAvailable, isPowerSufficient, isDiskEnough, isCloudSyncOn);
    if (!ableAutoStopDownload && HaveBatchDownloadResourcesTask()) { // 如果有自动暂停的任务，新增任务都保持同样的自动暂停状态
        int32_t autoStopReason = -1;
        QueryAutoPauseReason(autoStopReason);
        if (autoStopReason == static_cast<int32_t>(BatchDownloadAutoPauseReasonType::TYPE_POWER_LOW) ||
            autoStopReason == static_cast<int32_t>(BatchDownloadAutoPauseReasonType::TYPE_ROM_LOW)) {
            autoPauseReason = static_cast<BatchDownloadAutoPauseReasonType>(autoStopReason);
            MEDIA_DEBUG_LOG("BatchSelectFileDownloadAuto AutoStopCondition Keep Reason: %{public}d", autoStopReason);
            return true;
        }
    }
    return ableAutoStopDownload;
}

bool BackgroundCloudBatchSelectedFileProcessor::CanAutoRestoreCondition()
{
    // 自动恢复 网络 电量50+ rom 可用20以上 全满足
    bool netValidated = MedialibraryRelatedSystemStateManager::GetInstance()->IsNetValidatedAtRealTime();
    vector<int32_t> currentNotRestoreReasons;
    bool isNetworkAvailable =
        MedialibraryRelatedSystemStateManager::GetInstance()->IsNetAvailableWithUnlimitCondition();
    if (!isNetworkAvailable) {
        BatchDownloadAutoPauseReasonType reason = netValidated ?
            BatchDownloadAutoPauseReasonType::TYPE_CELLNET_LIMIT :
            BatchDownloadAutoPauseReasonType::TYPE_NETWORK_DISCONNECT;
        currentNotRestoreReasons.push_back(static_cast<int32_t>(reason));
    }
    bool isCloudSyncOn = CloudSyncUtils::IsCloudSyncSwitchOn();
    bool isPowerSufficient = true;
    #ifdef HAS_BATTERY_MANAGER_PART
        int32_t batteryCapacity = PowerMgr::BatterySrvClient::GetInstance().GetCapacity();
        isPowerSufficient = batteryCapacity > ABLE_RESTORE_DOWNLOAD_POWER;
        if (!isPowerSufficient) {
            currentNotRestoreReasons.push_back(static_cast<int32_t>(BatchDownloadAutoPauseReasonType::TYPE_POWER_LOW));
        }
    #endif

    double freeRatio = 0.0;
    BackgroundCloudBatchSelectedFileProcessor::GetStorageFreeRatio(freeRatio);
    bool isDiskEnough =  freeRatio > ABLE_RESTORE_DOWNLOAD_STORAGE_FREE_RATIO;
    if (!isDiskEnough) {
        currentNotRestoreReasons.push_back(static_cast<int32_t>(BatchDownloadAutoPauseReasonType::TYPE_ROM_LOW));
    }
    bool ableAutoResotreDownload = isCloudSyncOn && isNetworkAvailable && isPowerSufficient && isDiskEnough;
    MEDIA_DEBUG_LOG("BatchSelectFileDownloadAuto AutoRestoreCondition ableAutoResotreDownload: %{public}d, "
        "isNetworkAvailable: %{public}d, power: %{public}d, disk: %{public}d, cloudsync: %{public}d",
        ableAutoResotreDownload, isNetworkAvailable, isPowerSufficient,
        isDiskEnough, isCloudSyncOn);
    if (!ableAutoResotreDownload) {
        RefreshNotRestoreReason(currentNotRestoreReasons);
    }
    return ableAutoResotreDownload;
}

int32_t BackgroundCloudBatchSelectedFileProcessor::GetDeviceTemperature()
{
    int32_t temperatureLevel = 1; // Normal
    #ifdef HAS_THERMAL_MANAGER_PART
        auto& thermalMgrClient = PowerMgr::ThermalMgrClient::GetInstance();
        temperatureLevel = static_cast<int32_t>(thermalMgrClient.GetThermalLevel());
        MEDIA_INFO_LOG("BatchSelectFileDownload AutoStop temperatureLevel: %{public}d", temperatureLevel);
    #endif
    return temperatureLevel;
}

// 温度43上 温度39下 流控
void BackgroundCloudBatchSelectedFileProcessor::ControlDownloadLimit()
{
    int32_t temperatureLevel = GetDeviceTemperature();
    if (temperatureLevel < 3) { // 3 HOT
        batchDownloadQueueLimitNum_ = batchDownloadQueueLimitNumHigh;
        return;
    }
    if (temperatureLevel > 4) { // 4 Overheated
        batchDownloadQueueLimitNum_ = batchDownloadQueueLimitNumLow;
        return;
    }
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS

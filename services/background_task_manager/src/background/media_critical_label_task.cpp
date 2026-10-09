/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <string>
 
#include "media_critical_label_task.h"
 
#include "media_column.h"
#include "result_set_utils.h"
#include "media_file_utils.h"
#include "medialibrary_subscriber.h"
#include "media_log.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "critical_label_task_queue.h"
#ifdef MEDIALIBRARY_SECURE_ALBUM_ENABLE
#include "watch_lite/cloud_audit_impl.h"
#include "watch_system_handler.h"
#endif
#include "parameters.h"
#include "userfile_manager_types.h"
 
using namespace OHOS::NativeRdb;
 
namespace OHOS::Media::Background {
 
static const int32_t BATCH_SIZE = 50;
static const std::string CONST_MEDIA_SECURE_ALBUM = "const.media.secure_album";
std::mutex MediaCriticalLabelTask::mtx;
 
bool MediaCriticalLabelTask::Accept()
{
    return MedialibrarySubscriber::IsCriticalTypeStatusOn();
}
 
void MediaCriticalLabelTask::Execute()
{
    HandleCriticalLabelProcessing();
}
 
PhotoBatchInfo MediaCriticalLabelTask::QueryPhotosBatch(std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
    int32_t limit)
{
    PhotoBatchInfo batchInfo;
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, batchInfo, "rdbStore is nullptr");
#ifdef MEDIALIBRARY_SECURE_ALBUM_ENABLE
    auto criticalLabelTaskQueue = TTLPriorityQueue::GetInstance();
    CHECK_AND_RETURN_RET_LOG(criticalLabelTaskQueue != nullptr, batchInfo, "criticalLabelTaskQueue is nullptr");

    std::string sql = "SELECT " + MediaColumn::MEDIA_ID + ", " +
                    MediaColumn::MEDIA_NAME + ", " +
                    MediaColumn::MEDIA_FILE_PATH + ", " +
                    MediaColumn::MEDIA_TYPE + ", " +
                    MediaColumn::MEDIA_DATE_ADDED +
                    " FROM " + PhotoColumn::PHOTOS_TABLE +
                    " WHERE " + PhotoColumn::PHOTO_RISK_STATUS + "= " +
                    std::to_string(static_cast<int32_t>(PhotoRiskStatus::UNIDENTIFIED)) + " AND " +
                    PhotoColumn::PHOTO_POSITION + " != " +
                        std::to_string(static_cast<int32_t>(PhotoPositionType::CLOUD)) + " AND " +
                    MediaColumn::MEDIA_SIZE + " != 0";

    std::string filterName = "";
    auto displayNames = criticalLabelTaskQueue->GetElementsTruncatedPaths();
    for (auto &displayName : displayNames) {
        filterName += "'" + displayName + "'" + ",";
    }

    if (!filterName.empty()) {
        filterName.pop_back();
        sql += " AND " + MediaColumn::MEDIA_FILE_PATH + " NOT IN " + "(" + filterName + ")";
    }

    sql += " ORDER BY " + MediaColumn::MEDIA_ID + " ASC";
    sql += " LIMIT " + std::to_string(limit);
 
    std::shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(sql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, batchInfo, "Query photos batch fails");
 
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        PhotoInfo photoInfo;
        photoInfo.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        photoInfo.displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        photoInfo.filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        photoInfo.mediaType = GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet);
        photoInfo.addedTime = GetInt32Val(MediaColumn::MEDIA_DATE_ADDED, resultSet);
 
        batchInfo.emplace_back(photoInfo);
    }
    resultSet->Close();
#endif
    return batchInfo;
}
 
std::string MediaCriticalLabelTask::ConstructPhotoUri(const std::string &fileAssetData,
    const std::string &displayName, int32_t fileId)
{
    return MediaFileUtils::GetFileAssetUri(fileAssetData, displayName, fileId);
}

void MediaCriticalLabelTask::SendToAnlyze(AsyncTaskData *data)
{
    if (data == nullptr) {
        MEDIA_INFO_LOG("SendToAnlyze: SendToAnlyze failed to start, data is NULL");
        return;
    }
    CriticalLabelAsyncTaskData* notifyData = static_cast<CriticalLabelAsyncTaskData*>(data);
    CHECK_AND_RETURN_LOG(notifyData != nullptr, "notifyData is empty");

    for (const auto &photo : notifyData->batchInfo) {
        std::string uri = ConstructPhotoUri(photo.filePath, photo.displayName, photo.fileId);
        MEDIA_DEBUG_LOG("file_id: %{public}d, truncated_path: %{public}s, type: %{public}d, uri: %{public}s",
            photo.fileId,
            MediaFileUtils::DesensitizeName(photo.displayName).c_str(),
            photo.mediaType,
            MediaFileUtils::DesensitizeUri(uri).c_str());
        // Sepreate this part make for unit test work in phone
#ifdef MEDIALIBRARY_SECURE_ALBUM_ENABLE
        if (OHOS::system::GetParameter(CONST_MEDIA_SECURE_ALBUM, "") == "true") {
            // parse display name here
            std::string truncatedPath = "";
            WatchSystemHandler::ParseAssetName(photo.filePath, truncatedPath);
            TTLPriorityQueue::AssetParams params;
            params.truncated_path = truncatedPath;
            params.original_path = photo.filePath;
            params.id = 0;
            params.priority = 1;
            params.type = photo.mediaType;
            params.uri = uri;
            params.added_time = photo.addedTime;
            auto criticalLabelTaskQueue = TTLPriorityQueue::GetInstance();
            CHECK_AND_RETURN_LOG(criticalLabelTaskQueue != nullptr, "criticalLabelTaskQueue is nullptr");
            CHECK_AND_CONTINUE_ERR_LOG(MedialibrarySubscriber::IsCriticalTypeStatusOn(),
                "MediaCriticalLabelTask check condition failed, skiping adding queue.");
            auto ret = criticalLabelTaskQueue->AddElement(params);
            if (ret) {
                MEDIA_DEBUG_LOG("non-realtime addElement, added to queue with displayName: %{public}s",
                    MediaFileUtils::DesensitizeName(params.truncated_path).c_str());
            } else {
                MEDIA_DEBUG_LOG("non-realtime addElement skipped adding queue, displayName: %{public}s",
                    MediaFileUtils::DesensitizeName(params.truncated_path).c_str());
            }
        }
#endif
    }
}

void MediaCriticalLabelTask::HandleCriticalLabelProcessing()
{
    std::lock_guard<std::mutex> lock(mtx);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is nullptr");
    std::shared_ptr<MediaLibraryAsyncWorker> asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    CHECK_AND_RETURN_LOG(asyncWorker != nullptr, "asyncWorker is nullptr");
#ifdef MEDIALIBRARY_SECURE_ALBUM_ENABLE
    MEDIA_INFO_LOG("MediaCriticalLabelTask Start");
    // Check if task should continue
    if (!MedialibrarySubscriber::IsCriticalTypeStatusOn()) {
        MEDIA_INFO_LOG("MediaCriticalLabelTask check condition failed.");
        return;
    }

    auto criticalLabelTaskQueue = TTLPriorityQueue::GetInstance();
    CHECK_AND_RETURN_LOG(criticalLabelTaskQueue != nullptr, "criticalLabelTaskQueue is nullptr");

    auto limit = criticalLabelTaskQueue->GetRemainingQueueSize();
    CHECK_AND_RETURN_LOG(limit != 0, "Queue is full, no need to query more assets.");

    // Query batch of photos
    PhotoBatchInfo batchInfo = QueryPhotosBatch(rdbStore, limit);
    // If batch is empty, we're done
    if (batchInfo.empty()) {
        MEDIA_DEBUG_LOG("No more photos to process, iteration complete");
        return;
    }

    CriticalLabelAsyncTaskData* taskData = new (std::nothrow) CriticalLabelAsyncTaskData();
    if (taskData == nullptr) {
        MEDIA_ERR_LOG("Failed to create new taskData");
        return;
    }

    taskData->batchInfo = batchInfo;
    std::shared_ptr<MediaLibraryAsyncTask> criticalLabelAsyncTask = std::make_shared<MediaLibraryAsyncTask>(
        SendToAnlyze, taskData);

    if (criticalLabelAsyncTask != nullptr) {
        // Process and print each photo in the batch
        asyncWorker->AddTask(criticalLabelAsyncTask, true);
    } else {
        MEDIA_ERR_LOG("Start SendToAnlyze failed");
    }
    MEDIA_INFO_LOG("MediaCriticalLabelTask End");
#endif
}
}  // namespace OHOS::Media::Background
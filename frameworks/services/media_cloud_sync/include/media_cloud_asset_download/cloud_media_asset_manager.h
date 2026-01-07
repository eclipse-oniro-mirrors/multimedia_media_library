/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_CLOUD_MEDIA_ASSET_MANAGER_H
#define OHOS_CLOUD_MEDIA_ASSET_MANAGER_H

#include <iostream>
#include <memory>
#include <chrono>
#include <mutex>

#include "cloud_media_asset_download_operation.h"
#include "cloud_media_asset_types.h"
#include "medialibrary_command.h"
#include "rdb_store.h"
#include "download_resources_column.h"
#include "start_batch_download_cloud_resources_vo.h"
#include "resume_batch_download_cloud_resources_vo.h"
#include "pause_batch_download_cloud_resources_vo.h"
#include "cancel_batch_download_cloud_resources_vo.h"
#include "get_batch_download_cloud_resources_status_vo.h"
#include "get_batch_download_cloud_resources_count_vo.h"
#include "batch_download_resources_task_dao.h"
#include "cloud_media_retain_smart_data.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

enum class TaskDeleteState : int32_t {
    IDLE = 0,
    ACTIVE_DELETE = 1,
    BACKGROUND_DELETE = 2
};

class CloudMediaAssetManager {
public:
    EXPORT static CloudMediaAssetManager& GetInstance();
    EXPORT int32_t HandleCloudMediaAssetUpdateOperations(MediaLibraryCommand &cmd);
    EXPORT std::string HandleCloudMediaAssetGetTypeOperations(MediaLibraryCommand &cmd);
    EXPORT int32_t StartDownloadCloudAsset(const CloudMediaDownloadType &type);
    EXPORT int32_t RecoverDownloadCloudAsset(const CloudMediaTaskRecoverCause &cause);
    EXPORT int32_t PauseDownloadCloudAsset(const CloudMediaTaskPauseCause &pauseCause);
    EXPORT int32_t CancelDownloadCloudAsset();
    EXPORT int32_t ForceRetainDownloadCloudMedia(
        CloudMediaRetainType retainType = CloudMediaRetainType::RETAIN_FORCE,
        bool needAvoidRepeatedDoing = true);
    EXPORT std::string GetCloudMediaAssetTaskStatus();
    EXPORT bool SetIsThumbnailUpdate();
    EXPORT int32_t GetTaskStatus();
    EXPORT int32_t GetDownloadType();
    EXPORT bool SetBgDownloadPermission(const bool &flag);
    EXPORT void CheckStorageAndRecoverDownloadTask();
    EXPORT static void DeleteAllCloudMediaAssetsAsync();
    EXPORT static void StartDeleteCloudMediaAssets();
    EXPORT static void StopDeleteCloudMediaAssets();
    EXPORT void RestartForceRetainCloudAssets();
    EXPORT int32_t UpdateAddTaskStatus(const std::vector<std::string> &fileIds,
        const CloudMediaTaskDownloadCloudAssetCode &status, std::map<std::string, int32_t> &uriStatusMap);
    EXPORT int32_t BuildTaskValuesAndBatchInsert(
        int64_t &insertCount, std::vector<DownloadResourcesTaskPo> &newTaskPos);
    EXPORT int32_t StartBatchDownloadCloudResources(StartBatchDownloadCloudResourcesReqBody &reqBody,
        StartBatchDownloadCloudResourcesRespBody &respBody);
    EXPORT int32_t ResumeBatchDownloadCloudResources(ResumeBatchDownloadCloudResourcesReqBody &reqBody);
    EXPORT int32_t PauseBatchDownloadCloudResources(PauseBatchDownloadCloudResourcesReqBody &reqBody);
    EXPORT int32_t CancelBatchDownloadCloudResources(CancelBatchDownloadCloudResourcesReqBody &reqBody);
    EXPORT int32_t GetCloudMediaBatchDownloadResourcesStatus(
    GetBatchDownloadCloudResourcesStatusReqBody &reqBody, GetBatchDownloadCloudResourcesStatusRespBody &respBody);
    EXPORT int32_t GetCloudMediaBatchDownloadResourcesCount(
        GetBatchDownloadCloudResourcesCountReqBody &reqBody, GetBatchDownloadCloudResourcesCountRespBody &respBody);
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    EXPORT void CleanDownloadTasksTable();
#endif
    EXPORT static int32_t DeleteEditdata(const std::string &path);

private:
    CloudMediaAssetManager() {}
    ~CloudMediaAssetManager() {}
    CloudMediaAssetManager(const CloudMediaAssetManager &manager) = delete;
    const CloudMediaAssetManager &operator=(const CloudMediaAssetManager &manager) = delete;

    EXPORT int32_t CheckDownloadTypeOfTask(const CloudMediaDownloadType &type);
    EXPORT static int32_t DeleteBatchCloudFile(const std::vector<std::string> &fileIds);
    EXPORT static int32_t ReadyDataForDelete(std::vector<std::string> &fileIds, std::vector<std::string> &paths,
        std::vector<std::string> &dateTakens, std::vector<int64_t>& lcdVisitTimes, std::vector<int32_t> &subTypes);
    static bool ProcessDeleteBatch(const std::vector<std::string> &fileIds,
        const std::vector<std::string> &paths, const std::vector<std::string> &dateTakens,
        const std::vector<int64_t> &lcdVisitTimes, std::vector<int32_t> &subTypes);
    static void DeleteAllCloudMediaAssetsOperation(AsyncTaskData *data);
    EXPORT int32_t UpdateCloudMediaAssets(CloudMediaRetainType retainType = CloudMediaRetainType::RETAIN_FORCE,
        SmartDataProcessingMode mode = SmartDataProcessingMode::NONE);
    EXPORT int32_t DeleteEmptyCloudAlbums();
    EXPORT int32_t UpdateLocalAlbums();
    EXPORT int32_t UpdateBothLocalAndCloudAssets(
        CloudMediaRetainType retainType = CloudMediaRetainType::RETAIN_FORCE);
    EXPORT static std::string GetEditDataDirPath(const std::string &path);
    EXPORT static void CleanUpFileData(const std::string& path);
    EXPORT bool HasDataForUpdate(CloudMediaRetainType retainType, std::vector<std::string> &updateFileIds,
        const std::string &lastFileId, SmartDataProcessingMode mode);
    EXPORT int32_t UpdateCloudAssets(const std::vector<std::string> &updateFileIds, SmartDataProcessingMode mode);
    EXPORT void NotifyUpdateAssetsChange(const std::vector<std::string> &notifyFileIds);
    EXPORT bool HasLocalAndCloudAssets(CloudMediaRetainType retainType, std::vector<std::string> &updateFileIds,
        const std::string &lastFileId);
    EXPORT int32_t UpdateLocalAndCloudAssets(const std::vector<std::string> &updateFileIds);
    EXPORT void SetCloudsyncStatusKey(const int32_t statusKey);
    EXPORT void TryToStartSync();
    EXPORT int32_t ClearDeletedDbData();
    EXPORT int32_t ForceRetainDownloadCloudMediaEx(CloudMediaRetainType retainType, SmartDataProcessingMode mode);
    EXPORT std::vector<int32_t> QueryEmptyAlbumsAndBackup();
    EXPORT std::string BuildEmptyAlbumsWhereClause(const std::vector<int32_t>& albumIds);
private:
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation_{nullptr};
    inline static std::atomic<TaskDeleteState> doDeleteTask_{TaskDeleteState::IDLE};
    inline static std::mutex deleteMutex_;
    std::mutex updateMutex_;
    std::mutex batchDownloadMutex_;
    BatchDownloadResourcesTaskDao batchDownloadResourcesTaskDao_;

private:
    const std::string SQL_CLEAE_CLOUD_INFO_OF_ALBUM = "UPDATE PhotoAlbum SET dirty = 1, cloud_id = NULL;";
    const std::string SQL_RESET_UPLOAD_STATUS = "\
        UPDATE PhotoAlbum \
        SET upload_status = ( \
                CASE \
                    WHEN LOWER(lpath) IN ( \
                        LOWER('/DCIM/Camera'), \
                        LOWER('/Pictures/Screenrecords'), \
                        LOWER('/Pictures/Screenshots') \
                    ) \
                    THEN 1 \
                ELSE 0 END \
                ) \
        WHERE album_type IN (0, 2048);";
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_CLOUD_MEDIA_ASSET_MANAGER_H
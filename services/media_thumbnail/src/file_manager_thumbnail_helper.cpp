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

#define MLOG_TAG "FileManagerThumbnailHelper"

#include "file_manager_thumbnail_helper.h"

#include "dfx_utils.h"
#include "media_log.h"
#include "ithumbnail_helper.h"
#include "thumbnail_const.h"
#include "thumbnail_utils.h"
#include "thumbnail_generation_post_process.h"
#include "thumbnail_generate_worker_manager.h"
#include "thumbnail_source_loading.h"
#include "media_column.h"
#include "photo_album_column.h"
#include "result_set_utils.h"
#include "display_manager.h"

#ifdef HAS_THERMAL_MANAGER_PART
#include "thermal_mgr_client.h"
#endif

#ifdef HAS_BATTERY_MANAGER_PART
#include "battery_srv_client.h"
#endif

using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {

// ========== FileManager任务完成通知机制 ==========
// 使用计数器追踪FileManager任务，不依赖队列是否完全空
static std::condition_variable g_taskCompleteCv;
static std::mutex g_taskCompleteMutex;
static std::atomic<int32_t> g_activeTaskCount{0};
static std::atomic<bool> g_isRestoring{false};
static std::atomic<bool> g_cancelRestore{false};

// ========== 温度和电量检查实现 ==========
// LCOV_EXCL_START
bool FileManagerThumbnailHelper::CheckTemperatureBatteryConditionForRealtime()
{
#ifdef HAS_THERMAL_MANAGER_PART
    auto &thermalMgrClient = PowerMgr::ThermalMgrClient::GetInstance();
    int32_t temperatureLevel = static_cast<int32_t>(thermalMgrClient.GetThermalLevel());
    if (temperatureLevel > PROPER_DEVICE_TEMPERATURE_LEVEL_43) {
        MEDIA_INFO_LOG("Temperature level %{public}d exceeds 43, stop thumbnail generation", temperatureLevel);
        return false;
    }
#endif

#ifdef HAS_BATTERY_MANAGER_PART
    auto &batteryClient = PowerMgr::BatterySrvClient::GetInstance();
    int32_t batteryCapacity = batteryClient.GetCapacity();
    if (batteryCapacity <= PROPER_DEVICE_BATTERY_CAPACITY_THUMBNAIL) {
        MEDIA_INFO_LOG("Battery capacity %{public}d below 20, stop thumbnail generation", batteryCapacity);
        return false;
    }
#endif

    return true;
}

bool FileManagerThumbnailHelper::CheckTemperatureBatteryRestoreCondition()
{
#ifdef HAS_THERMAL_MANAGER_PART
    auto &thermalMgrClient = PowerMgr::ThermalMgrClient::GetInstance();
    int32_t temperatureLevel = static_cast<int32_t>(thermalMgrClient.GetThermalLevel());
    if (temperatureLevel > PROPER_DEVICE_TEMPERATURE_LEVEL_37) {
        MEDIA_INFO_LOG("Temperature level %{public}d exceeds 37, not ready to restore", temperatureLevel);
        return false;
    }
#endif

#ifdef HAS_BATTERY_MANAGER_PART
    auto &batteryClient = PowerMgr::BatterySrvClient::GetInstance();
    int32_t batteryCapacity = batteryClient.GetCapacity();
    if (batteryCapacity <= PROPER_DEVICE_BATTERY_CAPACITY_RESTORE) {
        MEDIA_INFO_LOG("Battery capacity %{public}d%% not above 30%%, not ready to restore", batteryCapacity);
        return false;
    }
#endif

    return true;
}

// ========== SP存储操作实现 ==========

void FileManagerThumbnailHelper::SaveThumbnailTaskToSP(const std::string &fileId)
{
    int32_t errCode = 0;
    auto prefs = NativePreferences::PreferencesHelper::GetPreferences(FILE_MANAGER_THUMB_TASK_SP, errCode);
    if (prefs == nullptr) {
        MEDIA_ERR_LOG("Failed to get preferences for thumbnail task, errCode: %{public}d", errCode);
        return;
    }

    prefs->PutString(fileId, fileId);
    prefs->Flush();
    MEDIA_DEBUG_LOG("Saved thumbnail task to SP, fileId: %{public}s", fileId.c_str());
}

void FileManagerThumbnailHelper::RemoveThumbnailTaskFromSP(const std::string &fileId)
{
    int32_t errCode = 0;
    auto prefs = NativePreferences::PreferencesHelper::GetPreferences(FILE_MANAGER_THUMB_TASK_SP, errCode);
    if (prefs == nullptr) {
        MEDIA_ERR_LOG("Failed to get preferences for thumbnail task, errCode: %{public}d", errCode);
        return;
    }

    prefs->Delete(fileId);
    prefs->Flush();
    MEDIA_DEBUG_LOG("Removed thumbnail task from SP, fileId: %{public}s", fileId.c_str());
}

// ========== 任务执行实现 ==========

int32_t FileManagerThumbnailHelper::CreateThumbnailForFileManager(const ThumbnailInfo &fileInfo,
    std::shared_ptr<MediaLibraryRdbStore> rdbStorePtr)
{
    MEDIA_INFO_LOG("CreateThumbnailForFileManager called, fileId: %{public}d", fileInfo.fileId);

    CHECK_AND_RETURN_RET_LOG(rdbStorePtr != nullptr, E_HAS_DB_ERROR,
        "rdbStorePtr is nullptr, fileId: %{public}d", fileInfo.fileId);

    if (!CheckTemperatureBatteryConditionForRealtime()) {
        MEDIA_INFO_LOG("Temperature or battery condition not met, save task to SP, fileId: %{public}d",
            fileInfo.fileId);
        SaveThumbnailTaskToSP(std::to_string(fileInfo.fileId));
        return E_OK;
    }

    g_activeTaskCount.fetch_add(1);

    std::string uri = PhotoColumn::PHOTO_URI_PREFIX + std::to_string(fileInfo.fileId);
    ThumbRdbOpt opts = {
        .store = rdbStorePtr,
        .path = fileInfo.path,
        .table = PhotoColumn::PHOTOS_TABLE,
        .row = std::to_string(fileInfo.fileId),
        .dateTaken = std::to_string(fileInfo.dateTaken),
        .dateModified = std::to_string(fileInfo.dateModified),
        .fileUri = uri
    };

    ThumbnailData thumbData;
    ThumbnailUtils::GetThumbnailInfo(opts, thumbData);
    thumbData.needResizeLcd = false;
    thumbData.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
    thumbData.genThumbScene = GenThumbScene::FILE_MANAGER_THUMB;

    IThumbnailHelper::AddThumbnailGenerateTask(
        FileManagerThumbnailTaskExecutor,
        opts, thumbData, ThumbnailTaskType::FOREGROUND, ThumbnailTaskPriority::MID);

    MEDIA_INFO_LOG("Successfully added thumbnail task to thumbnail worker, fileId: %{public}d",
        fileInfo.fileId);
    return E_OK;
}

void FileManagerThumbnailHelper::FileManagerThumbnailTaskExecutor(std::shared_ptr<ThumbnailTaskData> &data)
{
    if (data == nullptr) {
        MEDIA_ERR_LOG("Thumbnail task data is nullptr");
        return;
    }

    if (data->opts_.store == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr, fileId: %{public}s", data->opts_.row.c_str());
        NotifyTaskComplete();
        return;
    }

    MEDIA_DEBUG_LOG("Start executing FileManager thumbnail generate task, fileId: %{public}s",
        data->opts_.row.c_str());

    if (!CheckTemperatureBatteryConditionForRealtime()) {
        MEDIA_INFO_LOG("Temperature or battery condition not met, skip execution, fileId: %{public}s",
            data->opts_.row.c_str());
        SaveThumbnailTaskToSP(data->opts_.row);
        NotifyTaskComplete();
        return;
    }

    data->thumbnailData_.needResizeLcd = false;
    data->thumbnailData_.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;

    ThumbnailUtils::RecordStartGenerateStats(
        data->thumbnailData_.stats, GenerateScene::LOCAL, LoadSourceType::LOCAL_PHOTO);

    IThumbnailHelper::DoCreateThumbnail(data->opts_, data->thumbnailData_);
    int32_t ret = ThumbnailGenerationPostProcess::PostProcess(data->thumbnailData_, data->opts_);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to create thumbnail for fileId: %{public}s, ret: %{public}d",
            data->opts_.row.c_str(), ret);
    } else {
        MEDIA_DEBUG_LOG("Successfully created thumbnail for fileId: %{public}s", data->opts_.row.c_str());
    }

    ThumbnailUtils::RecordCostTimeAndReport(data->thumbnailData_.stats);
    
    // 任务完成后，减少计数器，到0时通知等待的恢复线程
    NotifyTaskComplete();
}

// ========== 任务恢复辅助函数实现 ==========

void FileManagerThumbnailHelper::StartAsyncRestoreTasks(std::shared_ptr<MediaLibraryRdbStore> rdbStorePtr)
{
    bool conditionsMet = CheckTemperatureBatteryRestoreCondition();
    if (!conditionsMet) {
        if (g_isRestoring.load()) {
            MEDIA_INFO_LOG("Restore in progress but conditions no longer met, signaling cancel");
            g_cancelRestore.store(true);
        }
        return;
    }

    if (g_isRestoring.load()) {
        MEDIA_DEBUG_LOG("Restore already in progress, skip this request");
        return;
    }

    int32_t errCode = 0;
    auto prefs = NativePreferences::PreferencesHelper::GetPreferences(FILE_MANAGER_THUMB_TASK_SP, errCode);
    CHECK_AND_RETURN_LOG(prefs != nullptr, "Failed to get preferences, errCode: %{public}d", errCode);

    auto prefsMap = prefs->GetAll();
    CHECK_AND_RETURN(!prefsMap.empty());

    g_isRestoring.store(true);
    g_cancelRestore.store(false);

    std::thread restoreThread([prefs, prefsMap, rdbStorePtr]() {
        int32_t restoreCount = ProcessThumbnailRestoreTasks(prefs, prefsMap, rdbStorePtr);
        MEDIA_INFO_LOG("Async restore completed, restored %{public}d thumbnail tasks from SP", restoreCount);
        g_isRestoring.store(false);
        g_cancelRestore.store(false);
    });
    restoreThread.detach();
}

int32_t FileManagerThumbnailHelper::ProcessThumbnailRestoreTasks(
    std::shared_ptr<NativePreferences::Preferences> prefs,
    const std::map<std::string, NativePreferences::PreferencesValue> &prefsMap,
    std::shared_ptr<MediaLibraryRdbStore> rdbStorePtr)
{
    int32_t restoreCount = 0;
    constexpr int32_t BATCH_SIZE = 200;
    constexpr int32_t MAX_WAIT_MS = 3000;

    for (const auto &entry : prefsMap) {
        if (g_cancelRestore.load()) {
            MEDIA_INFO_LOG("Restore canceled, stopping at %{public}d tasks", restoreCount);
            break;
        }

        if (!CheckTemperatureBatteryRestoreCondition()) {
            MEDIA_INFO_LOG("Conditions no longer met, stopping at %{public}d tasks", restoreCount);
            break;
        }

        auto result = RestoreSingleThumbnailTask(prefs, entry.first, rdbStorePtr);
        if (result == RestoreResult::SUCCESS) {
            restoreCount++;

            if (restoreCount % BATCH_SIZE == 0) {
                MEDIA_INFO_LOG("Added batch %{public}d tasks, waiting for completion", BATCH_SIZE);
                WaitUntilAllTasksComplete(MAX_WAIT_MS);
            }
        }
    }

    return restoreCount;
}

void FileManagerThumbnailHelper::WaitUntilAllTasksComplete(int32_t maxWaitMs)
{
    std::unique_lock<std::mutex> lock(g_taskCompleteMutex);

    bool completed = g_taskCompleteCv.wait_for(lock, std::chrono::milliseconds(maxWaitMs), [&]() {
        return g_activeTaskCount.load() == 0 || g_cancelRestore.load();
    });
    if (completed && g_activeTaskCount.load() == 0) {
        MEDIA_DEBUG_LOG("All tasks completed, continue adding next batch");
    } else if (g_cancelRestore.load()) {
        MEDIA_ERR_LOG("Restore canceled while waiting for tasks");
    } else {
        int32_t remaining = g_activeTaskCount.load();
        MEDIA_WARN_LOG("Tasks still pending %{public}d after %{public}d ms, continue adding", remaining, maxWaitMs);
    }
}

void FileManagerThumbnailHelper::NotifyTaskComplete()
{
    int32_t prevCount = g_activeTaskCount.fetch_sub(1);
    if (prevCount == 1) {
        std::lock_guard<std::mutex> lock(g_taskCompleteMutex);
        g_taskCompleteCv.notify_all();
        MEDIA_DEBUG_LOG("All FileManager tasks completed, notified waiting threads");
    }
}

FileManagerThumbnailHelper::RestoreResult FileManagerThumbnailHelper::RestoreSingleThumbnailTask(
    std::shared_ptr<NativePreferences::Preferences> prefs,
    const std::string &fileId,
    std::shared_ptr<MediaLibraryRdbStore> rdbStorePtr)
{
    RemoveThumbnailTaskFromSP(fileId);

    ThumbRdbOpt opts = {
        .store = rdbStorePtr,
        .table = PhotoColumn::PHOTOS_TABLE,
        .row = fileId
    };
    int err = 0;
    ThumbnailData data;
    ThumbnailUtils::QueryThumbnailDataFromFileId(opts, fileId, data, err);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, FileManagerThumbnailHelper::RestoreResult::FILE_DELETED,
        "QueryThumbnailDataFromFileId failed, path: %{public}s", DfxUtils::GetSafePath(data.path).c_str());

    AddFileManagerThumbnailTask(opts, data, rdbStorePtr);
    MEDIA_DEBUG_LOG("Restored thumbnail task, fileId: %{public}s", fileId.c_str());
    return FileManagerThumbnailHelper::RestoreResult::SUCCESS;
}

void FileManagerThumbnailHelper::AddFileManagerThumbnailTask(ThumbRdbOpt &opts, ThumbnailData &thumbData,
    std::shared_ptr<MediaLibraryRdbStore> rdbStorePtr)
{
    g_activeTaskCount.fetch_add(1);

    thumbData.needResizeLcd = false;
    thumbData.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
    thumbData.genThumbScene = GenThumbScene::FILE_MANAGER_THUMB_RESTORE;

    IThumbnailHelper::AddThumbnailGenerateTask(
        FileManagerThumbnailTaskExecutor,
        opts, thumbData, ThumbnailTaskType::FOREGROUND, ThumbnailTaskPriority::MID);
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS
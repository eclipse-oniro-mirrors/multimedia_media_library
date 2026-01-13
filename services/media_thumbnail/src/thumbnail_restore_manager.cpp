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

#include "thumbnail_restore_manager.h"

#include "res_sched_client.h"
#ifdef HAS_POWER_MANAGER_PART
#include "power_mgr_client.h"
#endif

#include "ithumbnail_helper.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_errno.h"
#include "thumbnail_const.h"
#include "thumbnail_data.h"
#include "thumbnail_source_loading.h"
#include "thumbnail_utils.h"

using namespace OHOS::ResourceSchedule;

namespace OHOS {
namespace Media {
// LCOV_EXCL_START
const int FFRT_MAX_RESTORE_ASTC_THREADS = 4;
const std::string MEDIALIBRARYBUNDLENAME = "com.ohos.medialibrary.medialibrarydata";
const std::string BUNDLENAME = "bundleName";
const std::string RELEASETIME = "releaseTime";
const std::string PROGRESS = "progress";

ThumbnailRestoreManager& ThumbnailRestoreManager::GetInstance()
{
    static ThumbnailRestoreManager instance;
    return instance;
}

ThumbnailRestoreManager::~ThumbnailRestoreManager()
{
    Reset();
    MEDIA_INFO_LOG("ThumbnailRestoreManager destroyed");
}

void ThumbnailRestoreManager::InitializeRestore(int64_t totalTasks)
{
    std::lock_guard<std::mutex> lock(progressMutex_);
    
    totalTasks_.store(totalTasks);
    completedTasks_.store(0);
    isRestoreActive_.store(true);
    
    MEDIA_INFO_LOG("Initialized restore progress: totalTasks=%{public}lld", totalTasks);

    bool isScreenOn = false;
#ifdef HAS_POWER_MANAGER_PART
    isScreenOn = PowerMgr::PowerMgrClient::GetInstance().IsScreenOn();
#endif
    if (isScreenOn) {
        ReportProgressBegin();
    }
}

void ThumbnailRestoreManager::AddCompletedTasks(int64_t count)
{
    CHECK_AND_RETURN(count > 0);
    int64_t newCompleted = completedTasks_.fetch_add(count) + count;
    int64_t total = totalTasks_.load();
    if (newCompleted >= total && total > 0) {
        isRestoreActive_.store(false);
        std::unordered_map<std::string, std::string> payload = {
            {BUNDLENAME, MEDIALIBRARYBUNDLENAME},
            {RELEASETIME, "0"},
            {PROGRESS, "100%"}
        };
        bool isScreenOn = false;
#ifdef HAS_POWER_MANAGER_PART
        isScreenOn = PowerMgr::PowerMgrClient::GetInstance().IsScreenOn();
#endif
        if (isScreenOn) {
            MEDIA_INFO_LOG(
                "Report data bundle name:com.ohos.medialibrary.medialibrarydata, releaseTime:0, progress:100");
            ResourceSchedule::ResSchedClient::GetInstance().ReportData(
                ResourceSchedule::ResType::RES_TYPE_BACKGROUND_STATUS, REPORT_OPEN, payload);
        }
        ResourceSchedule::ResSchedClient::GetInstance().ReportData(
            ResourceSchedule::ResType::RES_TYPE_BACKGROUND_STATUS, REPORT_END, payload);

        StopProgressReporting();
    }
}

void ThumbnailRestoreManager::StartProgressReporting(uint32_t reportIntervalMs)
{
    std::lock_guard<std::mutex> lock(progressMutex_);

    if (isReporting_.load()) {
        MEDIA_WARN_LOG("Progress reporting already started");
        return;
    }

    progressTimer_.Setup();

    Utils::Timer::TimerCallback callback = [this]() {
        bool isScreenOn = false;
#ifdef HAS_POWER_MANAGER_PART
        isScreenOn = PowerMgr::PowerMgrClient::GetInstance().IsScreenOn();
#endif
        MEDIA_INFO_LOG("Start progress reporting");
        this->ReportProgress(isScreenOn);
    };

    progressTimerId_ = progressTimer_.Register(callback, reportIntervalMs, true);
    if (progressTimerId_ == 0) {
        MEDIA_ERR_LOG("Failed to register progress report timer");
        return;
    }
    
    isReporting_.store(true);
    MEDIA_INFO_LOG("Started progress reporting with interval %{public}ums", reportIntervalMs);
}

void ThumbnailRestoreManager::StopProgressReporting()
{
    std::lock_guard<std::mutex> lock(progressMutex_);
    if (!isReporting_.load()) {
        return;
    }

    if (progressTimerId_ != 0) {
        progressTimer_.Unregister(progressTimerId_);
        progressTimer_.Shutdown();
        progressTimerId_ = 0;
    }

    isReporting_.store(false);
    MEDIA_INFO_LOG("Stopped progress reporting");
}

void ThumbnailRestoreManager::OnScreenStateChanged(bool isScreenOn)
{
    bool previousScreenState = lastScreenState_.exchange(isScreenOn);
    if (!previousScreenState && isScreenOn && isRestoreActive_.load()) {
        MEDIA_INFO_LOG("Screen state changed from OFF to ON, reporting progress immediately");
        ReportProgress(isScreenOn);
    }
}

void ThumbnailRestoreManager::ReportProgressBegin()
{
    int64_t total = totalTasks_.load();
    int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();

    startTime_.store(currentTime);
    if (total == 0) {
        MEDIA_ERR_LOG("Error thumbnail number");
        return;
    }
    float totalTime = total * static_cast<float>(SINGLE_THREAD_RUNTIME_MS) /
        static_cast<float>(FFRT_MAX_RESTORE_ASTC_THREADS) / MILLIS_PER_MINUTE;
    std::unordered_map<std::string, std::string> payload = {
        {BUNDLENAME, MEDIALIBRARYBUNDLENAME},
        {RELEASETIME, std::to_string(totalTime)},
        {PROGRESS, "0%"}
    };
    MEDIA_INFO_LOG(
        "Report data bundle name:com.ohos.medialibrary.medialibrarydata, releaseTime:%{public}f, progress:0",
        totalTime);
    ResourceSchedule::ResSchedClient::GetInstance().ReportData(
        ResourceSchedule::ResType::RES_TYPE_BACKGROUND_STATUS, REPORT_OPEN, payload);
}

void ThumbnailRestoreManager::ReportProgress(bool isScreenOn)
{
    if (!isScreenOn) {
        MEDIA_INFO_LOG("Screen is OFF, skipping progress report");
        return;
    }
    int64_t completed = completedTasks_.load();
    int64_t total = totalTasks_.load();
    int64_t startTime = startTime_.load();
    int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t delta = currentTime - startTime;

    int64_t deltaReadyAstc = completed - readyAstc_.load();
    if (deltaReadyAstc == 0) {
        MEDIA_ERR_LOG("Error no progress");
        return;
    }
    startTime_.store(currentTime);
    readyAstc_.store(completed);
    float releaseTime = (total - completed) * (static_cast<float>(delta) / deltaReadyAstc) /
        MILLIS_PER_MINUTE;

    if (total == 0) {
        MEDIA_ERR_LOG("Error thumbnail number");
        return;
    }
    float progress = (static_cast<float>(completed) / total) * PROGRESS_TO_PERCENT;

    std::unordered_map<std::string, std::string> payload = {
        {BUNDLENAME, MEDIALIBRARYBUNDLENAME},
        {RELEASETIME, std::to_string(releaseTime)},
        {PROGRESS, std::to_string(progress) + "%"}
    };
    MEDIA_INFO_LOG(
        "Report data bundle name:com.ohos.medialibrary.medialibrarydata, releaseTime:%{public}f, progress:%{public}f",
        releaseTime, progress);
    ResourceSchedule::ResSchedClient::GetInstance().ReportData(
        ResourceSchedule::ResType::RES_TYPE_BACKGROUND_STATUS, REPORT_OPEN, payload);
}

void ThumbnailRestoreManager::RestoreAstcDualFrameTask(std::shared_ptr<ThumbnailTaskData> &data)
{
    if (data == nullptr) {
        MEDIA_ERR_LOG("RestoreAstcDualFrameTask failed, data is null");
        return;
    }

    IThumbnailHelper::CreateThumbnail(data);
    ThumbnailRestoreManager::GetInstance().AddCompletedTasks();
}

int32_t ThumbnailRestoreManager::RestoreAstcDualFrame(ThumbRdbOpt &opts, const int32_t &restoreAstcCount)
{
    CHECK_AND_RETURN_RET_LOG(restoreAstcCount > 0, E_ERR, "RestoreAstcCount:%{public}d is invalid", restoreAstcCount);
    CHECK_AND_RETURN_RET_LOG(opts.store != nullptr, E_ERR, "RdbStore is not init");
    
    std::vector<ThumbnailData> infos;
    int32_t err = 0;
    if (!ThumbnailUtils::QueryNoAstcInfosRestored(opts, infos, err, restoreAstcCount)) {
        MEDIA_ERR_LOG("Failed to QueryNoAstcInfosRestored %{public}d", err);
        return err;
    }
    
    if (infos.empty()) {
        MEDIA_INFO_LOG("No photos need restore astc.");
        return E_OK;
    }
    InitializeRestore(infos.size());
    StartProgressReporting(RESTORE_THUMBNAIL_REPORT_INTERVAL_MS);

    MEDIA_INFO_LOG("create astc for restored dual frame photos count:%{public}zu, restoreAstcCount:%{public}d",
        infos.size(), restoreAstcCount);

    for (auto &info : infos) {
        info.genThumbScene = GenThumbScene::CLONE_OR_DUAL_FRAME_UPGRADE;
        opts.row = info.id;
        info.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
        ThumbnailUtils::RecordStartGenerateStats(info.stats, GenerateScene::RESTORE, LoadSourceType::LOCAL_PHOTO);
        IThumbnailHelper::AddThumbnailGenerateTask(RestoreAstcDualFrameTask, opts, info,
            ThumbnailTaskType::FOREGROUND, ThumbnailTaskPriority::MID);
    }

    MEDIA_INFO_LOG("create astc for restored dual frame photos finished");
    return E_OK;
}

void ThumbnailRestoreManager::Reset()
{
    StopProgressReporting();
    {
        std::lock_guard<std::mutex> lock(progressMutex_);
        readyAstc_.store(0);
        completedTasks_.store(0);
        totalTasks_.store(0);
        startTime_.store(0);
        lastScreenState_.store(false);
        isRestoreActive_.store(false);
    }
    MEDIA_INFO_LOG("ThumbnailRestoreManager reset completed");
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS

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
#define MLOG_TAG "DfxManager"

#include "dfx_manager.h"

#include "dfx_cloud_manager.h"
#include "dfx_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "userfile_manager_types.h"
#include "medialibrary_bundle_manager.h"
#include "dfx_database_utils.h"
#include "vision_aesthetics_score_column.h"
#include "parameters.h"
#include "preferences.h"
#include "preferences_helper.h"

using namespace std;

namespace OHOS {
namespace Media {

shared_ptr<DfxManager> DfxManager::dfxManagerInstance_{nullptr};
mutex DfxManager::instanceLock_;

shared_ptr<DfxManager> DfxManager::GetInstance()
{
    lock_guard<mutex> lockGuard(instanceLock_);
    if (dfxManagerInstance_ == nullptr) {
        dfxManagerInstance_ = make_shared<DfxManager>();
        if (dfxManagerInstance_ != nullptr) {
            dfxManagerInstance_->Init();
        }
    }
    return dfxManagerInstance_;
}

DfxManager::DfxManager() : isInitSuccess_(false)
{
}

DfxManager::~DfxManager()
{
}

void DfxManager::Init()
{
    MEDIA_INFO_LOG("Init DfxManager");
    dfxCollector_ = make_shared<DfxCollector>();
    dfxAnalyzer_ = make_shared<DfxAnalyzer>();
    dfxReporter_ = make_shared<DfxReporter>();
    dfxWorker_ = DfxWorker::GetInstance();
    dfxWorker_->Init();
    isInitSuccess_ = true;
}

void DfxManager::HandleTimeOutOperation(std::string &bundleName, int32_t type, int32_t object, int32_t time)
{
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return;
    }
    dfxReporter_->ReportTimeOutOperation(bundleName, type, object, time);
}

int32_t DfxManager::HandleHighMemoryThumbnail(std::string &path, int32_t mediaType, int32_t width,
    int32_t height)
{
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return NOT_INIT;
    }
    string suffix = MediaFileUtils::GetExtensionFromPath(path);
    if (mediaType == MEDIA_TYPE_IMAGE) {
        return dfxReporter_->ReportHighMemoryImageThumbnail(path, suffix, width, height);
    } else {
        return dfxReporter_->ReportHighMemoryVideoThumbnail(path, suffix, width, height);
    }
}

void DfxManager::HandleThumbnailError(const std::string &path, int32_t method, int32_t errorCode)
{
    string safePath = DfxUtils::GetSafePath(path);
    MEDIA_ERR_LOG("Failed to %{public}d, path: %{public}s, err: %{public}d", method, safePath.c_str(), errorCode);
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return;
    }
    dfxCollector_->CollectThumbnailError(safePath, method, errorCode);
}

void DfxManager::HandleThumbnailGeneration(const ThumbnailData::GenerateStats &stats)
{
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return;
    }
    dfxReporter_->ReportThumbnailGeneration(stats);
}

void DfxManager::HandleCommonBehavior(string bundleName, int32_t type)
{
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return;
    }
    dfxCollector_->AddCommonBahavior(bundleName, type);
}

static void LogDelete(DfxData *data)
{
    if (data == nullptr) {
        return;
    }
    auto *taskData = static_cast<DeleteBehaviorTask *>(data);
    string id = taskData->id_;
    int32_t type = taskData->type_;
    int32_t size = taskData->size_;
    std::shared_ptr<DfxReporter> dfxReporter = taskData->dfxReporter_;
    MEDIA_INFO_LOG("id: %{public}s, type: %{public}d, size: %{public}d", id.c_str(), type, size);
    std::vector<std::string> uris = taskData->uris_;
    if (!uris.empty()) {
        for (auto& uri: uris) {
            string::size_type pos = uri.find_last_of('/');
            if (pos == string::npos) {
                continue;
            }
            string halfUri = uri.substr(0, pos);
            string::size_type pathPos = halfUri.find_last_of('/');
            if (pathPos == string::npos) {
                continue;
            }
            dfxReporter->ReportDeleteBehavior(id, type, halfUri.substr(pathPos + 1));
        }
    }
}

void DfxManager::HandleNoPermmison(int32_t type, int32_t object, int32_t error)
{
    MEDIA_INFO_LOG("permission deny: {%{public}d, %{public}d, %{public}d}", type, object, error);
}

void DfxManager::HandleDeleteBehavior(int32_t type, int32_t size, std::vector<std::string> &uris, string bundleName)
{
    if (bundleName == "") {
        bundleName = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    }
    dfxCollector_->CollectDeleteBehavior(bundleName, type, size);
    if (dfxWorker_ == nullptr) {
        MEDIA_ERR_LOG("Can not get dfxWork_");
        return;
    }
    string id = bundleName == "" ? to_string(IPCSkeleton::GetCallingUid()) : bundleName;
    auto *taskData = new (nothrow) DeleteBehaviorTask(id, type, size, uris, dfxReporter_);
    if (taskData == nullptr) {
        MEDIA_ERR_LOG("Failed to new taskData");
        return;
    }
    auto deleteBehaviorTask = make_shared<DfxTask>(LogDelete, taskData);
    if (deleteBehaviorTask == nullptr) {
        MEDIA_ERR_LOG("Failed to create async task for deleteBehaviorTask.");
        return;
    }
    dfxWorker_->AddTask(deleteBehaviorTask);
}

static void HandlePhotoInfo(std::shared_ptr<DfxReporter> &dfxReporter)
{
    int32_t localImageCount = DfxDatabaseUtils::QueryFromPhotos(MediaType::MEDIA_TYPE_IMAGE, true);
    int32_t localVideoCount = DfxDatabaseUtils::QueryFromPhotos(MediaType::MEDIA_TYPE_VIDEO, true);
    int32_t cloudImageCount = DfxDatabaseUtils::QueryFromPhotos(MediaType::MEDIA_TYPE_IMAGE, false);
    int32_t cloudVideoCount = DfxDatabaseUtils::QueryFromPhotos(MediaType::MEDIA_TYPE_VIDEO, false);
    MEDIA_INFO_LOG("localImageCount: %{public}d, localVideoCount: %{public}d, cloudImageCount: %{public}d, \
        cloudVideoCount: %{public}d", localImageCount, localVideoCount, cloudImageCount, cloudVideoCount);
    dfxReporter->ReportPhotoInfo(localImageCount, localVideoCount, cloudImageCount, cloudVideoCount);
}

static void HandleAlbumInfoBySubtype(std::shared_ptr<DfxReporter> &dfxReporter, int32_t albumSubType)
{
    AlbumInfo albumInfo = DfxDatabaseUtils::QueryAlbumInfoBySubtype(albumSubType);
    string albumName = ALBUM_MAP.at(albumSubType);
    MEDIA_INFO_LOG("album %{public}s: {count:%{public}d, imageCount:%{public}d, videoCount:%{public}d, \
        isLocal:%{public}d}", albumName.c_str(), albumInfo.count, albumInfo.imageCount, albumInfo.videoCount,
        albumInfo.isLocal);
    dfxReporter->ReportAlbumInfo(albumName.c_str(), albumInfo.imageCount, albumInfo.videoCount, albumInfo.isLocal);
}

static void HandleAlbumInfo(std::shared_ptr<DfxReporter> &dfxReporter)
{
    HandleAlbumInfoBySubtype(dfxReporter, static_cast<int32_t>(PhotoAlbumSubType::IMAGE));
    HandleAlbumInfoBySubtype(dfxReporter, static_cast<int32_t>(PhotoAlbumSubType::VIDEO));
    HandleAlbumInfoBySubtype(dfxReporter, static_cast<int32_t>(PhotoAlbumSubType::FAVORITE));
    HandleAlbumInfoBySubtype(dfxReporter, static_cast<int32_t>(PhotoAlbumSubType::HIDDEN));
    HandleAlbumInfoBySubtype(dfxReporter, static_cast<int32_t>(PhotoAlbumSubType::TRASH));
}

static void HandleDirtyCloudPhoto(std::shared_ptr<DfxReporter> &dfxReporter)
{
    vector<PhotoInfo> photoInfoList = DfxDatabaseUtils::QueryDirtyCloudPhoto();
    if (photoInfoList.empty()) {
        return;
    }
    for (auto& photoInfo: photoInfoList) {
        dfxReporter->ReportDirtyCloudPhoto(photoInfo.data, photoInfo.dirty, photoInfo.cloudVersion);
    }
}

static void HandleLocalVersion(std::shared_ptr<DfxReporter> &dfxReporter)
{
    int32_t dbVersion = DfxDatabaseUtils::QueryDbVersion();
    dfxReporter->ReportCommonVersion(dbVersion);
    int32_t aestheticsVersion = DfxDatabaseUtils::QueryAnalysisVersion("tab_analysis_aesthetics_score",
        AESTHETICS_VERSION);
    dfxReporter->ReportAnalysisVersion("tab_analysis_aesthetics_score", aestheticsVersion);
}

static void HandleStatistic(DfxData *data)
{
    if (data == nullptr) {
        return;
    }
    auto *taskData = static_cast<StatisticData *>(data);
    std::shared_ptr<DfxReporter> dfxReporter = taskData->dfxReporter_;
    HandlePhotoInfo(dfxReporter);
    HandleAlbumInfo(dfxReporter);
    HandleDirtyCloudPhoto(dfxReporter);
    HandleLocalVersion(dfxReporter);
}

void DfxManager::HandleHalfDayMissions()
{
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return;
    }
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    int64_t lastReportTime = prefs->GetLong(LAST_HALF_DAY_REPORT_TIME, 0);
    if (MediaFileUtils::UTCTimeSeconds() - lastReportTime > HALF_DAY && dfxWorker_ != nullptr) {
        MEDIA_INFO_LOG("start handle statistic behavior");
        auto *taskData = new (nothrow) StatisticData(dfxReporter_);
        if (taskData == nullptr) {
        MEDIA_ERR_LOG("Failed to alloc async data for Handle Half Day Missions!");
        return;
        }
        auto statisticTask = make_shared<DfxTask>(HandleStatistic, taskData);
        if (statisticTask == nullptr) {
            MEDIA_ERR_LOG("Failed to create statistic task.");
            return;
        }
        dfxWorker_->AddTask(statisticTask);
        int64_t time = MediaFileUtils::UTCTimeSeconds();
        prefs->PutLong(LAST_HALF_DAY_REPORT_TIME, time);
        prefs->FlushSync();
    }
}

void DfxManager::HandleFiveMinuteTask()
{
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return;
    }
    std::unordered_map<string, CommonBehavior> commonBehavior = dfxCollector_->GetCommonBehavior();
    dfxAnalyzer_->FlushCommonBehavior(commonBehavior);
    HandleDeleteBehaviors();
    std::unordered_map<std::string, ThumbnailErrorInfo> result = dfxCollector_->GetThumbnailError();
    dfxAnalyzer_->FlushThumbnail(result);
    AdaptationToMovingPhotoInfo adaptationInfo = dfxCollector_->GetAdaptationToMovingPhotoInfo();
    dfxAnalyzer_->FlushAdaptationToMovingPhoto(adaptationInfo);
}

void DfxManager::HandleDeleteBehaviors()
{
    std::unordered_map<string, int32_t> deleteAssetToTrash =
        dfxCollector_->GetDeleteBehavior(DfxType::TRASH_PHOTO);
    dfxAnalyzer_->FlushDeleteBehavior(deleteAssetToTrash, DfxType::TRASH_PHOTO);
    std::unordered_map<string, int32_t> deleteAssetFromDisk =
        dfxCollector_->GetDeleteBehavior(DfxType::ALBUM_DELETE_ASSETS);
    dfxAnalyzer_->FlushDeleteBehavior(deleteAssetToTrash, DfxType::ALBUM_DELETE_ASSETS);
    std::unordered_map<string, int32_t> removeAssets =
        dfxCollector_->GetDeleteBehavior(DfxType::ALBUM_REMOVE_PHOTOS);
    dfxAnalyzer_->FlushDeleteBehavior(removeAssets, DfxType::ALBUM_REMOVE_PHOTOS);
}

int64_t DfxManager::HandleMiddleReport()
{
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return MediaFileUtils::UTCTimeSeconds();
    }
    dfxReporter_->ReportCommonBehavior();
    dfxReporter_->ReportDeleteStatistic();
    return MediaFileUtils::UTCTimeSeconds();
}

int64_t DfxManager::HandleOneDayReport()
{
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return MediaFileUtils::UTCTimeSeconds();
    }
    dfxReporter_->ReportThumbnailError();
    dfxReporter_->ReportAdaptationToMovingPhoto();
    dfxReporter_->ReportPhotoRecordInfo();
    return MediaFileUtils::UTCTimeSeconds();
}

void DfxManager::HandleAdaptationToMovingPhoto(const string &appName, bool adapted)
{
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return;
    }
    dfxCollector_->CollectAdaptationToMovingPhotoInfo(appName, adapted);
}

bool IsReported()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get dfx common preferences error: %{public}d", errCode);
        return false;
    }
    return prefs->GetBool(IS_REPORTED, false);
}

void SetReported(bool isReported)
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get dfx common preferences error: %{public}d", errCode);
        return;
    }
    prefs->PutBool(IS_REPORTED, isReported);
}

CloudSyncDfxManager::~CloudSyncDfxManager()
{
    ShutDownTimer();
}

CloudSyncDfxManager& CloudSyncDfxManager::GetInstance()
{
    static CloudSyncDfxManager cloudSyncDfxManager;
    return cloudSyncDfxManager;
}

CloudSyncStatus GetCloudSyncStatus()
{
    return static_cast<CloudSyncStatus>(system::GetParameter(CLOUDSYNC_STATUS_KEY, "0").at(0) - '0');
}

CloudSyncDfxManager::CloudSyncDfxManager()
{
    InitSyncState();
    uint16_t newState = static_cast<uint16_t>(syncState_);
    stateProcessFuncs_[newState].Process(*this);
}

void CloudSyncDfxManager::InitSyncState()
{
    CloudSyncStatus cloudSyncStatus = GetCloudSyncStatus();
    switch (cloudSyncStatus) {
        case CloudSyncStatus::BEGIN:
        case CloudSyncStatus::SYNC_SWITCHED_OFF:
            syncState_ = SyncState::INIT_STATE;
            return;
        case CloudSyncStatus::FIRST_FIVE_HUNDRED:
        case CloudSyncStatus::TOTAL_DOWNLOAD:
            syncState_ = SyncState::START_STATE;
            return;
        case CloudSyncStatus::TOTAL_DOWNLOAD_FINISH:
            syncState_ = SyncState::END_STATE;
            return;
        default:
            return;
    }
}

bool InitState::StateSwitch(CloudSyncDfxManager& manager)
{
    CloudSyncStatus cloudSyncStatus = GetCloudSyncStatus();
    switch (cloudSyncStatus) {
        case CloudSyncStatus::FIRST_FIVE_HUNDRED:
        case CloudSyncStatus::TOTAL_DOWNLOAD:
            manager.syncState_ = SyncState::START_STATE;
            return true;
        case CloudSyncStatus::TOTAL_DOWNLOAD_FINISH:
            manager.syncState_ = SyncState::END_STATE;
            MEDIA_INFO_LOG("CloudSyncDfxManager new status:%{public}hu", manager.syncState_);
            return true;
        default:
            return false;
    }
}

void InitState::Process(CloudSyncDfxManager& manager)
{
    MEDIA_INFO_LOG("CloudSyncDfxManager new status:%{public}hu", manager.syncState_);
    manager.ResetStartTime();
    manager.ShutDownTimer();
    SetReported(false);
}

void CloudSyncDfxManager::RunDfx()
{
    uint16_t oldState = static_cast<uint16_t>(syncState_);
    if (stateProcessFuncs_[oldState].StateSwitch(*this)) {
        uint16_t newState = static_cast<uint16_t>(syncState_);
        stateProcessFuncs_[newState].Process(*this);
    }
}

bool StartState::StateSwitch(CloudSyncDfxManager& manager)
{
    CloudSyncStatus cloudSyncStatus = GetCloudSyncStatus();
    switch (cloudSyncStatus) {
        case CloudSyncStatus::BEGIN:
        case CloudSyncStatus::SYNC_SWITCHED_OFF:
            manager.syncState_ = SyncState::INIT_STATE;
            return true;
        case CloudSyncStatus::TOTAL_DOWNLOAD_FINISH:
            manager.syncState_ = SyncState::END_STATE;
            MEDIA_INFO_LOG("CloudSyncDfxManager new status:%{public}hu", manager.syncState_);
            return true;
        default:
            return false;
    }
}

void StartState::Process(CloudSyncDfxManager& manager)
{
    MEDIA_INFO_LOG("CloudSyncDfxManager new status:%{public}hu", manager.syncState_);
    manager.SetStartTime();
    manager.StartTimer();
    SetReported(false);
}

bool EndState::StateSwitch(CloudSyncDfxManager& manager)
{
    CloudSyncStatus cloudSyncStatus = GetCloudSyncStatus();
    switch (cloudSyncStatus) {
        case CloudSyncStatus::BEGIN:
        case CloudSyncStatus::SYNC_SWITCHED_OFF:
            manager.syncState_ = SyncState::INIT_STATE;
            return true;
        case CloudSyncStatus::TOTAL_DOWNLOAD_FINISH:
            return true;
        default:
            return false;
    }
}

void EndState::Process(CloudSyncDfxManager& manager)
{
    if (IsReported()) {
        manager.ShutDownTimer();
        return;
    }
    manager.SetStartTime();
    manager.StartTimer();
    int32_t downloadedThumb = 0;
    int32_t generatedThumb = 0;
    if (!DfxDatabaseUtils::QueryDownloadedAndGeneratedThumb(downloadedThumb, generatedThumb)) {
        if (downloadedThumb != generatedThumb) {
            return;
        }
        int32_t totalDownload = 0;
        DfxDatabaseUtils::QueryTotalCloudThumb(totalDownload);
        if (totalDownload != downloadedThumb) {
            return;
        }
        SetReported(true);
        manager.ShutDownTimer();
        DfxReporter::ReportCloudSyncThumbGenerationStatus(downloadedThumb, generatedThumb, totalDownload);
    }
}

void CloudSyncDfxManager::StartTimer()
{
    if (timerId_ != 0) {
        return;
    }
    if (timer_.Setup() != ERR_OK) {
        MEDIA_INFO_LOG("CloudSync Dfx Set Timer Failed");
        return;
    }
    Utils::Timer::TimerCallback timerCallback = [this]() {
        if (IsReported()) {
            return;
        }
        int32_t generatedThumb = 0;
        int32_t downloadedThumb = 0;
        if (!DfxDatabaseUtils::QueryDownloadedAndGeneratedThumb(downloadedThumb, generatedThumb)) {
            int32_t totalDownload = 0;
            DfxDatabaseUtils::QueryTotalCloudThumb(totalDownload);
            if (downloadedThumb == generatedThumb && totalDownload == generatedThumb) {
                MEDIA_INFO_LOG("CloudSyncDfxManager Dfx report Thumb generation status, "
                    "download: %{public}d, generate: %{public}d", downloadedThumb, generatedThumb);
                SetReported(true);
            }
            DfxReporter::ReportCloudSyncThumbGenerationStatus(downloadedThumb, generatedThumb, totalDownload);
        }
    };
    timerId_ = timer_.Register(timerCallback, SIX_HOUR * TO_MILLION, false);
}

void CloudSyncDfxManager::ShutDownTimer()
{
    if (timerId_ == 0) {
        return;
    }
    MEDIA_INFO_LOG("CloudSyncDfxManager ShutDownTimer");
    timer_.Unregister(timerId_);
    timerId_ = 0;
    timer_.Shutdown();
}

void CloudSyncDfxManager::ResetStartTime()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get dfx common preferences error: %{public}d", errCode);
        return;
    }
    prefs->PutLong(CLOUD_SYNC_START_TIME, 0);
    prefs->FlushSync();
}

void CloudSyncDfxManager::SetStartTime()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get dfx common preferences error: %{public}d", errCode);
        return;
    }
    int64_t time = prefs->GetLong(CLOUD_SYNC_START_TIME, 0);
    // if startTime exists, no need to reset startTime
    if (time != 0) {
        return;
    }
    time = MediaFileUtils::UTCTimeSeconds();
    prefs->PutLong(CLOUD_SYNC_START_TIME, time);
    prefs->FlushSync();
}

} // namespace Media
} // namespace OHOS
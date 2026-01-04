/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MedialibrarySubscribe"

#include "medialibrary_subscriber.h"

#include <chrono>

#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
#include "background_cloud_file_processor.h"
#include "background_cloud_batch_selected_file_processor.h"
#endif
#include "background_task_mgr_helper.h"
#ifdef HAS_BATTERY_MANAGER_PART
#include "battery_srv_client.h"
#endif
#include "cloud_media_asset_manager.h"
#include "cloud_media_asset_types.h"
#include "cloud_upload_checker.h"
#include "common_event_utils.h"
#include "dfx_cloud_manager.h"
#include "dfx_moving_photo.h"

#include "post_event_utils.h"
#ifdef HAS_POWER_MANAGER_PART
#include "power_mgr_client.h"
#endif
#ifdef HAS_THERMAL_MANAGER_PART
#include "thermal_mgr_client.h"
#endif

#include "medialibrary_album_fusion_utils.h"
#include "medialibrary_all_album_refresh_processor.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_facard_operations.h"
#include "medialibrary_inotify.h"
#include "medialibrary_kvstore_manager.h"
#ifdef META_RECOVERY_SUPPORT
#include "medialibrary_meta_recovery.h"
#endif
#include "medialibrary_restore.h"
#include "medialibrary_subscriber_database_utils.h"
#include "media_lake_check_manager.h"
#include "ability_manager_client.h"
#include "resource_type.h"
#include "dfx_manager.h"
#include "medialibrary_update_dirty_data_task_data.h"
#include "moving_photo_processor.h"
#include "permission_utils.h"
#include "thumbnail_generate_worker_manager.h"
#include "shooting_mode_album_operation.h"
#include "parameters.h"
#include "height_width_correct_operation.h"
#ifdef HAS_WIFI_MANAGER_PART
#include "wifi_device.h"
#endif
#include "net_conn_client.h"
#include "power_efficiency_manager.h"
#include "photo_album_lpath_operation.h"
#include "photo_day_month_year_operation.h"
#include "photo_mimetype_operation.h"
#include "preferences_helper.h"
#include "medialibrary_astc_stat.h"
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
#include "enhancement_manager.h"
#include "cloud_enhancement_checker.h"
#endif
#include "map_code_upload_checker.h"
#include "medialibrary_transcode_data_aging_operation.h"
#include "medialibrary_aspect_ratio_operation.h"
#include "database_adapter.h"
#include "product_info.h"
#include "permission_whitelist_utils.h"
#include "cloud_media_retain_smart_data.h"
#include "power_mgr_client.h"
#include "power_mode_info.h"

using namespace OHOS::AAFwk;

namespace OHOS {
namespace Media {

shared_ptr<MedialibrarySubscriber> MedialibrarySubscriber::subscriber_ = nullptr;
std::future<bool> MedialibrarySubscriber::subscribeAsyncTask_;
std::mutex MedialibrarySubscriber::subscribeLock_;
std::mutex MedialibrarySubscriber::subscribeAsyncTaskLock_;

// The task can be performed when the battery level reaches the value
const int32_t PROPER_DEVICE_BATTERY_CAPACITY = 50;
const int32_t PROPER_DEVICE_BATTERY_CAPACITY_THUMBNAIL = 20;

const int TIME_START_RELEASE_TEMPERATURE_LIMIT = 1;
const int TIME_STOP_RELEASE_TEMPERATURE_LIMIT = 6;

// The task can be performed only when the temperature of the device is lower than the value
const int32_t PROPER_DEVICE_TEMPERATURE_LEVEL_37 = 1;
const int32_t PROPER_DEVICE_TEMPERATURE_LEVEL_40 = 2;
const int32_t PROPER_DEVICE_TEMPERATURE_LEVEL_43 = 3;

// WIFI should be available in this state
const int32_t WIFI_STATE_CONNECTED = 4;
const int32_t DELAY_TASK_TIME = 30000;
const int32_t COMMON_EVENT_KEY_GET_DEFAULT_PARAM = -1;
const int32_t MegaByte = 1024 * 1024;
const int32_t MAX_FILE_SIZE_MB = 10240;
const int32_t UPDATE_DIRTY_CLOUD_CLONE_V1 = 1;
const int32_t UPDATE_DIRTY_CLOUD_CLONE_V2 = 2;
const std::string COMMON_EVENT_KEY_BATTERY_CAPACITY = "soc";
const std::string COMMON_EVENT_KEY_DEVICE_TEMPERATURE = "0";
static const std::string TASK_PROGRESS_XML = "/data/storage/el2/base/preferences/task_progress.xml";
static const std::string NO_UPDATE_DIRTY = "no_update_dirty";
static const std::string NO_UPDATE_DIRTY_CLOUD_CLONE_V2 = "no_update_dirty_cloud_clone_v2";
static const std::string NO_DELETE_DIRTY_HDC_DATA = "no_delete_dirty_hdc_data";
static const std::string NO_UPDATE_EDITDATA_SIZE = "no_update_editdata_size";
static const std::string UPDATE_EDITDATA_SIZE_COUNT = "update_editdata_size_count";

// The network should be available in this state
const int32_t NET_CONN_STATE_CONNECTED = 3;
// The net bearer type is in net_all_capabilities.h
const int32_t BEARER_CELLULAR = 0;
const int32_t THUMB_ASTC_ENOUGH = 20000;
bool MedialibrarySubscriber::isCellularNetConnected_ = false;
bool MedialibrarySubscriber::isWifiConnected_ = false;
bool MedialibrarySubscriber::currentStatus_ = false;
// BetaVersion will upload the DB
const std::string KEY_HIVIEW_VERSION_TYPE = "const.logsystem.versiontype";
std::mutex uploadDBMutex;
int64_t g_lastTime = MediaFileUtils::UTCTimeMilliSeconds();
const int64_t TWELVE_HOUR_MS = static_cast<int64_t>(12 * 3600 * 1000);
constexpr int32_t SUBSCRIBE_TASK_TIMEOUT_SECOND = 2;
const string CLOUD_UPDATE_EVENT = "usual.event.DUE_HAP_CFG_UPDATED";
const string CLOUD_EVENT_INFO_TYPE = "type";
const string CLOUD_EVENT_INFO_TYPE_VALUE = "medialibrary_kit_whitelist";
const std::string CLONE_FLAG = "multimedia.medialibrary.cloneFlag";

const std::vector<std::string> MedialibrarySubscriber::events_ = {
    EventFwk::CommonEventSupport::COMMON_EVENT_CHARGING,
    EventFwk::CommonEventSupport::COMMON_EVENT_DISCHARGING,
    EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_OFF,
    EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_ON,
    EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED,
    EventFwk::CommonEventSupport::COMMON_EVENT_BATTERY_CHANGED,
    EventFwk::CommonEventSupport::COMMON_EVENT_THERMAL_LEVEL_CHANGED,
    EventFwk::CommonEventSupport::COMMON_EVENT_WIFI_CONN_STATE,
    EventFwk::CommonEventSupport::COMMON_EVENT_CONNECTIVITY_CHANGE,
    EventFwk::CommonEventSupport::COMMON_EVENT_TIME_TICK,
    EventFwk::CommonEventSupport::COMMON_EVENT_HWID_LOGOUT,
    EventFwk::CommonEventSupport::COMMON_EVENT_DATA_SHARE_READY,
    EventFwk::CommonEventSupport::COMMON_EVENT_POWER_CONNECTED,
    EventFwk::CommonEventSupport::COMMON_EVENT_POWER_DISCONNECTED,
    CLOUD_UPDATE_EVENT
};

const std::map<std::string, StatusEventType> BACKGROUND_OPERATION_STATUS_MAP = {
    {EventFwk::CommonEventSupport::COMMON_EVENT_CHARGING, StatusEventType::CHARGING},
    {EventFwk::CommonEventSupport::COMMON_EVENT_DISCHARGING, StatusEventType::DISCHARGING},
    {EventFwk::CommonEventSupport::COMMON_EVENT_POWER_CONNECTED, StatusEventType::POWER_CONNECTED},
    {EventFwk::CommonEventSupport::COMMON_EVENT_POWER_DISCONNECTED, StatusEventType::POWER_DISCONNECTED},
    {EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_OFF, StatusEventType::SCREEN_OFF},
    {EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_ON, StatusEventType::SCREEN_ON},
    {EventFwk::CommonEventSupport::COMMON_EVENT_BATTERY_CHANGED, StatusEventType::BATTERY_CHANGED},
    {EventFwk::CommonEventSupport::COMMON_EVENT_THERMAL_LEVEL_CHANGED, StatusEventType::THERMAL_LEVEL_CHANGED},
    {EventFwk::CommonEventSupport::COMMON_EVENT_TIME_TICK, StatusEventType::TIME_TICK},
};

bool GetNowLocalTime(std::tm &nowLocalTime)
{
    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    return localtime_r(&nowTime, &nowLocalTime) != nullptr;
}

void MedialibrarySubscriber::RefreshCellularNetStatus()
{
    NetManagerStandard::NetHandle handle;
    int32_t ret = NetManagerStandard::NetConnClient::GetInstance().GetDefaultNet(handle);
    CHECK_AND_RETURN_LOG(ret == 0, "GetDefaultNet failed, err:%{public}d", ret);
    NetManagerStandard::NetAllCapabilities netAllCap;
    ret = NetManagerStandard::NetConnClient::GetInstance().GetNetCapabilities(handle, netAllCap);
    CHECK_AND_RETURN_LOG(ret == 0, "GetNetCapabilities failed, err:%{public}d", ret);
    const std::set<NetManagerStandard::NetBearType>& types = netAllCap.bearerTypes_;
    if (types.count(NetManagerStandard::BEARER_CELLULAR)) {
        MEDIA_INFO_LOG("init cellular status success: %{public}d", isCellularNetConnected_);
        isCellularNetConnected_ = true;
    }
    return;
}

MedialibrarySubscriber::MedialibrarySubscriber(const EventFwk::CommonEventSubscribeInfo &subscriberInfo)
    : EventFwk::CommonEventSubscriber(subscriberInfo)
{
#ifdef HAS_POWER_MANAGER_PART
    auto& powerMgrClient = PowerMgr::PowerMgrClient::GetInstance();
    isScreenOff_ = !powerMgrClient.IsScreenOn();
#endif
#ifdef HAS_BATTERY_MANAGER_PART
    auto& batteryClient = PowerMgr::BatterySrvClient::GetInstance();
    auto chargeState = batteryClient.GetChargingStatus();
    isCharging_ = (chargeState == PowerMgr::BatteryChargeState::CHARGE_STATE_ENABLE) ||
        (chargeState == PowerMgr::BatteryChargeState::CHARGE_STATE_FULL);
    batteryCapacity_ = batteryClient.GetCapacity();
#endif
#ifdef HAS_THERMAL_MANAGER_PART
    auto& thermalMgrClient = PowerMgr::ThermalMgrClient::GetInstance();
    newTemperatureLevel_ = static_cast<int32_t>(thermalMgrClient.GetThermalLevel());
    isDeviceTemperatureProper_ = newTemperatureLevel_ <= PROPER_DEVICE_TEMPERATURE_LEVEL_37;
#endif
#ifdef HAS_WIFI_MANAGER_PART
    auto wifiDevicePtr = Wifi::WifiDevice::GetInstance(WIFI_DEVICE_ABILITY_ID);
    if (wifiDevicePtr == nullptr) {
        MEDIA_ERR_LOG("MedialibrarySubscriber wifiDevicePtr is null");
    } else {
        ErrCode ret = wifiDevicePtr->IsConnected(isWifiConnected_);
        if (ret != Wifi::WIFI_OPT_SUCCESS) {
            MEDIA_ERR_LOG("MedialibrarySubscriber Get-IsConnected-fail: -%{public}d", ret);
        }
    }
#endif
    MedialibrarySubscriber::RefreshCellularNetStatus();
    MediaLibraryAllAlbumRefreshProcessor::GetInstance()->OnCurrentStatusChanged(
        isScreenOff_ && isCharging_ && batteryCapacity_ >= PROPER_DEVICE_BATTERY_CAPACITY
        && isDeviceTemperatureProper_);
    MEDIA_DEBUG_LOG("MedialibrarySubscriber current status:%{public}d, %{public}d, %{public}d, %{public}d, %{public}d",
        isScreenOff_, isCharging_, batteryCapacity_, newTemperatureLevel_, isWifiConnected_);
}

MedialibrarySubscriber::~MedialibrarySubscriber()
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    if (cloudHelper_ != nullptr && CloudMediaAssetUnlimitObserver_ != nullptr) {
        cloudHelper_->UnregisterObserverExt(Uri(CLOUD_URI), CloudMediaAssetUnlimitObserver_);
        cloudHelper_ = nullptr;
    }
#endif
}

#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
void CloudMediaAssetUnlimitObserver::OnChange(const ChangeInfo &changeInfo)
{
    auto subscriber = subscriber_.lock();
    CHECK_AND_RETURN(subscriber != nullptr);

    std::list<Uri> uris = changeInfo.uris_;
    for (auto &uri : uris) {
        bool cond = (uri.ToString() != CLOUD_URI || changeInfo.changeType_ != DataShareObserver::ChangeType::OTHER);
        CHECK_AND_RETURN_INFO_LOG(!cond, "Current uri is not suitable for task.");

        bool isUnlimitedTrafficStatusOn = CloudSyncUtils::IsUnlimitedTrafficStatusOn();
        MEDIA_INFO_LOG("CloudMediaAssetUnlimitObserver OnChange, isUnlimitedTrafficStatusOn: %{public}d.",
            isUnlimitedTrafficStatusOn);
        if (isUnlimitedTrafficStatusOn) {
            BackgroundCloudBatchSelectedFileProcessor::TriggerAutoResumeBatchDownloadResourceCheck();
        }
        if (!CommonEventUtils::IsWifiConnected() && !isUnlimitedTrafficStatusOn) {
            BackgroundCloudBatchSelectedFileProcessor::TriggerAutoStopBatchDownloadResourceCheck(); // 批量下载立即停止
        }
    }
}
#endif

bool MedialibrarySubscriber::Subscribe(void)
{
    std::lock_guard<std::mutex> lock(subscribeLock_);
    EventFwk::MatchingSkills matchingSkills;
    for (auto event : events_) {
        matchingSkills.AddEvent(event);
    }

    MEDIA_INFO_LOG("Subscribe: add event.");
    CHECK_AND_RETURN_RET_LOG(subscriber_ == nullptr, false,
        "Subscriber_ is not null, must unsubscribe last subscriber_");
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);

    subscriber_ = std::make_shared<MedialibrarySubscriber>(subscribeInfo);
    CHECK_AND_RETURN_RET_LOG(subscriber_ != nullptr, false, "Subscriber_ is null!");
    bool ret = EventFwk::CommonEventManager::SubscribeCommonEvent(subscriber_);
    CHECK_AND_EXECUTE(ret, {
        MEDIA_ERR_LOG("EventFwk::CommonEventManager::SubscribeCommonEvent failed");
        subscriber_ = nullptr;
        return false;
    });

#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    CreateOptions options;
    options.enabled_ = true;
    subscriber_->cloudHelper_ = DataShare::DataShareHelper::Creator(CLOUD_DATASHARE_URI, options);
    CHECK_AND_RETURN_RET_LOG(subscriber_->cloudHelper_ != nullptr, E_ERR, "cloudHelper_ is null.");
    std::weak_ptr<MedialibrarySubscriber> subscriberWeakPtr(subscriber_);
    subscriber_->CloudMediaAssetUnlimitObserver_ = std::make_shared<CloudMediaAssetUnlimitObserver>(subscriberWeakPtr);
    CHECK_AND_RETURN_RET_LOG(subscriber_->CloudMediaAssetUnlimitObserver_ != nullptr, ret,
        "CloudMediaAssetUnlimitObserver_ is null.");
    // observer more than 50, failed to register
    subscriber_->cloudHelper_->RegisterObserverExt(Uri(CLOUD_URI), subscriber_->CloudMediaAssetUnlimitObserver_, true);
#endif
    return ret;
}

void MedialibrarySubscriber::SubscribeAsync()
{
    std::lock_guard<std::mutex> lockSubscribeAsyncTask(subscribeAsyncTaskLock_);
    CHECK_AND_RETURN_INFO_LOG(!subscribeAsyncTask_.valid(), "There is already a task to do it");
    subscribeAsyncTask_ = std::async(std::launch::async, [&] {
        return Media::MedialibrarySubscriber::Subscribe();
    });
}

bool MedialibrarySubscriber::UnSubscribe()
{
    std::lock_guard<std::mutex> lockSubscribeAsyncTask(subscribeAsyncTaskLock_);
    CHECK_AND_RETURN_RET_INFO_LOG(subscribeAsyncTask_.valid(), true, "No running task, no need unsubscribe");
    MEDIA_INFO_LOG("Waiting subscribeAsyncTask done.");
    auto status = subscribeAsyncTask_.wait_for(std::chrono::seconds(SUBSCRIBE_TASK_TIMEOUT_SECOND));
    if (status == std::future_status::ready) {
        bool ret = subscribeAsyncTask_.get();
        MEDIA_INFO_LOG("SubscribeAsyncTask execute res: %{public}d", ret);
    } else if (status == std::future_status::timeout) {
        MEDIA_ERR_LOG("Wait subscribeAsyncTask timeout, wait time: %{public}d second", SUBSCRIBE_TASK_TIMEOUT_SECOND);
        return false;
    } else {
        MEDIA_ERR_LOG("Invalid status: %{public}d", static_cast<int>(status));
        return false;
    }
    std::lock_guard<std::mutex> lockSubscribe(subscribeLock_);
    CHECK_AND_RETURN_RET_LOG(subscriber_ != nullptr, false, "Subscriber_ is nullptr!");
    CHECK_AND_RETURN_RET_LOG(EventFwk::CommonEventManager::UnSubscribeCommonEvent(subscriber_),
        false, "UnSubscribeCommonEvent failed");
    subscriber_ = nullptr;
    return true;
}

static bool IsBetaVersion()
{
    static const string versionType = system::GetParameter(KEY_HIVIEW_VERSION_TYPE, "unknown");
    static bool isBetaVersion = versionType.find("beta") != std::string::npos;
    return isBetaVersion;
}

static bool IsTwelveHoursAgo()
{
    std::lock_guard<mutex> lock(uploadDBMutex);
    int64_t curTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("IsTwelveHoursAgo enter, interval:%{public}s ms", to_string(curTime - g_lastTime).c_str());
    if (curTime - g_lastTime >= TWELVE_HOUR_MS) {
        g_lastTime = curTime;
        return true;
    }
    return false;
}

static void UploadDBFile()
{
    std::lock_guard<mutex> lock(uploadDBMutex);
    int64_t begin = MediaFileUtils::UTCTimeMilliSeconds();
    static const std::string databaseDir = MEDIA_DB_DIR + "/rdb";
    static const std::vector<std::string> dbFileName = { "/media_library.db",
                                                         "/media_library.db-shm",
                                                         "/media_library.db-wal" };
    static const std::string destPath = "/data/storage/el2/log/logpack";
    int64_t totalFileSize = 0;
    for (auto &dbName : dbFileName) {
        string dbPath = databaseDir + dbName;
        struct stat statInfo {};
        if (stat(dbPath.c_str(), &statInfo) != 0) {
            continue;
        }
        totalFileSize += statInfo.st_size;
    }
    totalFileSize /= MegaByte; // Convert bytes to MB
    if (totalFileSize > MAX_FILE_SIZE_MB) {
        MEDIA_WARN_LOG("DB file over 10GB are not uploaded, totalFileSize is %{public}ld MB",
            static_cast<long>(totalFileSize));
        return ;
    }
    if (!MediaFileUtils::IsFileExists(destPath) && !MediaFileUtils::CreateDirectory(destPath)) {
        MEDIA_ERR_LOG("Create dir failed, dir=%{private}s", destPath.c_str());
        return ;
    }
    auto dataManager = MediaLibraryDataManager::GetInstance();
    if (dataManager == nullptr) {
        MEDIA_ERR_LOG("dataManager is nullptr");
        return;
    }
    dataManager->UploadDBFileInner(totalFileSize);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("Handle %{public}s MB DBFile success, cost %{public}s ms", std::to_string(totalFileSize).c_str(),
        std::to_string(end - begin).c_str());
}

void MedialibrarySubscriber::CheckHalfDayMissions()
{
    if (isScreenOff_ && isCharging_) {
        DfxManager::GetInstance()->HandleHalfDayMissions();
        MediaLibraryRestore::GetInstance().CheckBackup();
    }
    if (!isScreenOff_ || !isCharging_) {
        MediaLibraryRestore::GetInstance().InterruptBackup();
    }
#ifdef META_RECOVERY_SUPPORT
    MediaLibraryMetaRecovery::GetInstance().StatisticSave();
#endif
}

void MedialibrarySubscriber::UpdateCurrentStatus()
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::tm nowLocalTime;
    bool newStatus = false;
    bool isPowerSufficient = batteryCapacity_ >= PROPER_DEVICE_BATTERY_CAPACITY;
    if (GetNowLocalTime(nowLocalTime) && nowLocalTime.tm_hour >= TIME_START_RELEASE_TEMPERATURE_LIMIT &&
        nowLocalTime.tm_hour < TIME_STOP_RELEASE_TEMPERATURE_LIMIT) {
        newStatus = isScreenOff_ && isCharging_ && isPowerSufficient &&
            newTemperatureLevel_ <= PROPER_DEVICE_TEMPERATURE_LEVEL_43;
    } else {
        newStatus = isScreenOff_ && isCharging_ && isPowerSufficient &&
            newTemperatureLevel_ <= PROPER_DEVICE_TEMPERATURE_LEVEL_37;
    }

    if (currentStatus_ == newStatus) {
        return;
    }

    MEDIA_INFO_LOG("update status current:%{public}d, new:%{public}d, %{public}d, %{public}d, %{public}d, %{public}d",
        currentStatus_, newStatus, isScreenOff_, isCharging_, isPowerSufficient, newTemperatureLevel_);
    currentStatus_ = newStatus;
    backgroundDelayTask_.EndBackgroundOperationThread();
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    BackgroundCloudFileProcessor::RepairMimeType();
#endif
    if (currentStatus_) {
        backgroundDelayTask_.SetOperationThread([this] { this->DoBackgroundOperation(); });
    } else {
        StopBackgroundOperation();
    }
    MediaLibraryAllAlbumRefreshProcessor::GetInstance()->OnCurrentStatusChanged(currentStatus_);
}

void MedialibrarySubscriber::WalCheckPointAsync()
{
    if (!isScreenOff_ || !isCharging_) {
        return;
    }
    std::thread(MediaLibraryRdbStore::WalCheckPoint).detach();
}

bool MedialibrarySubscriber::GetPowerConnected()
{
    auto& service = OHOS::PowerMgr::BatterySrvClient::GetInstance();
    PowerMgr::BatteryChargeState chargeState = service.GetChargingStatus();
    bool isPowerConnected = (chargeState == PowerMgr::BatteryChargeState::CHARGE_STATE_ENABLE||
            chargeState == PowerMgr::BatteryChargeState::CHARGE_STATE_FULL);
    return isPowerConnected;
}

void MedialibrarySubscriber::UpdateBackgroundOperationStatus(
    const AAFwk::Want &want, const StatusEventType statusEventType)
{
    switch (statusEventType) {
        case StatusEventType::SCREEN_OFF:
            isScreenOff_ = true;
            break;
        case StatusEventType::SCREEN_ON:
            isScreenOff_ = false;
            break;
        case StatusEventType::CHARGING:
            isCharging_ = true;
            break;
        case StatusEventType::DISCHARGING:
            isCharging_ = false;
            break;
        case StatusEventType::POWER_CONNECTED:
            isCharging_ = true;
            break;
        case StatusEventType::POWER_DISCONNECTED:
            isCharging_ = false;
            break;
        case StatusEventType::BATTERY_CHANGED:
            batteryCapacity_ = want.GetIntParam(COMMON_EVENT_KEY_BATTERY_CAPACITY,
                COMMON_EVENT_KEY_GET_DEFAULT_PARAM);
            break;
        case StatusEventType::THERMAL_LEVEL_CHANGED: {
            newTemperatureLevel_ = want.GetIntParam(COMMON_EVENT_KEY_DEVICE_TEMPERATURE,
                COMMON_EVENT_KEY_GET_DEFAULT_PARAM);
            isDeviceTemperatureProper_ = newTemperatureLevel_ <= PROPER_DEVICE_TEMPERATURE_LEVEL_37;
            break;
        }
        case StatusEventType::TIME_TICK:
            break;
        default:
            MEDIA_WARN_LOG("StatusEventType:%{public}d is not invalid", statusEventType);
            return;
    }

    UpdateCurrentStatus();
    UpdateThumbnailBgGenerationStatus();
    UpdateMediaInLakeCheckStatus();
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    UpdateBackgroundTimer();
#endif
    DealWithEventsAfterUpdateStatus(statusEventType);
}

void MedialibrarySubscriber::UpdateCloudMediaAssetDownloadStatus(const AAFwk::Want &want,
    const StatusEventType statusEventType)
{
    if (statusEventType != StatusEventType::THERMAL_LEVEL_CHANGED) {
        return;
    }
    int32_t taskStatus = CloudMediaAssetManager::GetInstance().GetTaskStatus();
    int32_t downloadType = CloudMediaAssetManager::GetInstance().GetDownloadType();
    bool foregroundTemperature = want.GetIntParam(COMMON_EVENT_KEY_DEVICE_TEMPERATURE,
        COMMON_EVENT_KEY_GET_DEFAULT_PARAM) <= PROPER_DEVICE_TEMPERATURE_LEVEL_43;
    if (!foregroundTemperature && downloadType == static_cast<int32_t>(CloudMediaDownloadType::DOWNLOAD_FORCE)) {
        CloudMediaAssetManager::GetInstance().PauseDownloadCloudAsset(CloudMediaTaskPauseCause::TEMPERATURE_LIMIT);
        return;
    }
    if (foregroundTemperature && taskStatus == static_cast<int32_t>(CloudMediaAssetTaskStatus::PAUSED)) {
        CloudMediaAssetManager::GetInstance().RecoverDownloadCloudAsset(
            CloudMediaTaskRecoverCause::FOREGROUND_TEMPERATURE_PROPER);
    }
}

bool MedialibrarySubscriber::IsCellularNetConnected()
{
    return isCellularNetConnected_;
}

bool MedialibrarySubscriber::IsWifiConnected()
{
    return isWifiConnected_;
}

bool MedialibrarySubscriber::IsCurrentStatusOn()
{
    return currentStatus_;
}

void MedialibrarySubscriber::OnReceiveEvent(const EventFwk::CommonEventData &eventData)
{
    const AAFwk::Want &want = eventData.GetWant();
    std::string action = want.GetAction();
    CHECK_AND_EXECUTE(action != EventFwk::CommonEventSupport::COMMON_EVENT_DATA_SHARE_READY,
        MediaLibraryFaCardOperations::InitFaCard());
    bool cond = action != EventFwk::CommonEventSupport::COMMON_EVENT_BATTERY_CHANGED &&
                action != EventFwk::CommonEventSupport::COMMON_EVENT_TIME_TICK;
    CHECK_AND_PRINT_INFO_LOG(!cond, "OnReceiveEvent action:%{public}s.", action.c_str());
    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_WIFI_CONN_STATE) {
        isWifiConnected_ = eventData.GetCode() == WIFI_STATE_CONNECTED;
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
        HandleBatchDownloadWhenNetChange();
        UpdateBackgroundTimer();
#endif
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_CONNECTIVITY_CHANGE) {
        int netType = want.GetIntParam("NetType", -1);
        bool isNetConnected = eventData.GetCode() == NET_CONN_STATE_CONNECTED;
        MEDIA_INFO_LOG("netType: %{public}d, isConnected: %{public}d.", netType, static_cast<int32_t>(isNetConnected));
        isCellularNetConnected_ = netType == BEARER_CELLULAR ? isNetConnected : isCellularNetConnected_;
    } else if (BACKGROUND_OPERATION_STATUS_MAP.count(action) != 0) {
        UpdateBackgroundOperationStatus(want, BACKGROUND_OPERATION_STATUS_MAP.at(action));
        UpdateCloudMediaAssetDownloadStatus(want, BACKGROUND_OPERATION_STATUS_MAP.at(action));
    } else if (action.compare(EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED) == 0) {
        string packageName = want.GetElement().GetBundleName();
        RevertPendingByPackage(packageName);
        MediaLibraryBundleManager::GetInstance()->Clear();
        PermissionUtils::ClearBundleInfoInCache();
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_HWID_LOGOUT) {
        // when turn off gallery switch or quit account, clear the download lastest finished flag,
        // so we can download lastest images for the subsequent login new account
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
        BackgroundCloudFileProcessor::SetDownloadLatestFinished(false);
#endif
    }
    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_BATTERY_CHANGED &&
        isScreenOff_ && isCharging_ && IsBetaVersion() && batteryCapacity_ >= PROPER_DEVICE_BATTERY_CAPACITY) {
        UploadDB();
    }
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_WIFI_CONN_STATE ||
        action == EventFwk::CommonEventSupport::COMMON_EVENT_CONNECTIVITY_CHANGE) {
        EnhancementManager::GetInstance().HandleNetChange(isWifiConnected_, isCellularNetConnected_);
    }
#endif

    std::string type = want.GetStringParam(CLOUD_EVENT_INFO_TYPE);
    if (action == CLOUD_UPDATE_EVENT && type == CLOUD_EVENT_INFO_TYPE_VALUE) {
        PermissionWhitelistUtils::OnReceiveEvent();
    }
}

#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
void MedialibrarySubscriber::HandleBatchDownloadWhenNetChange()
{
    if (!isWifiConnected_ && BackgroundCloudBatchSelectedFileProcessor::IsBatchDownloadProcessRunningStatus()) {
        MEDIA_INFO_LOG("BatchSelectFileDownload COMMON_EVENT_WIFI_CONN_STATE Change");
        BackgroundCloudBatchSelectedFileProcessor::StopProcessConditionCheck();
    }
}
#endif

int64_t MedialibrarySubscriber::GetNowTime()
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return t.tv_sec * SEC_TO_MSEC + t.tv_nsec / MSEC_TO_NSEC;
}

void MedialibrarySubscriber::Init()
{
    lockTime_ = GetNowTime();
    agingCount_ = 0;
}

void DeleteTemporaryPhotos()
{
    auto dataManager = MediaLibraryDataManager::GetInstance();
    if (dataManager == nullptr) {
        return;
    }

    string UriString = PAH_DISCARD_CAMERA_PHOTO;
    MediaFileUtils::UriAppendKeyValue(UriString, URI_PARAM_API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri uri(UriString);
    MediaLibraryCommand cmd(uri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::PHOTO_IS_TEMP, true);
    DataShare::DataSharePredicates predicates;

    // 24H之前的数据
    int64_t current = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t timeBefore24Hours = current - 24 * 60 * 60 * 1000;
    string where = PhotoColumn::PHOTO_IS_TEMP + " = 1 AND " + PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " <> 4 AND (" +
        PhotoColumn::MEDIA_DATE_ADDED + " <= " + to_string(timeBefore24Hours) + " OR " + MediaColumn::MEDIA_ID +
        " NOT IN (SELECT " + MediaColumn::MEDIA_ID + " FROM (SELECT " + MediaColumn::MEDIA_ID + " FROM " +
        PhotoColumn::PHOTOS_TABLE + " WHERE " + PhotoColumn::PHOTO_IS_TEMP + " = 1 AND " +
        PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " <> 4 ORDER BY " + MediaColumn::MEDIA_ID +
        " DESC LIMIT 50)) AND (select COUNT(1) from " + PhotoColumn::PHOTOS_TABLE + " where " +
        PhotoColumn::PHOTO_IS_TEMP + " = 1 AND " + PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " <> 4 ) > 100) ";
    predicates.SetWhereClause(where);

    auto changedRows = dataManager->Update(cmd, valuesBucket, predicates);
    CHECK_AND_RETURN_LOG(changedRows >= 0, "Failed to update property of asset, err: %{public}d", changedRows);
    MEDIA_INFO_LOG("delete %{public}d temp files exceeding 24 hous or exceed maximum quantity.", changedRows);
}

void MedialibrarySubscriber::DoAgingOperation()
{
    auto dataManager = MediaLibraryDataManager::GetInstance();
    CHECK_AND_RETURN_LOG(dataManager != nullptr, "dataManager is nullptr");

    int32_t result = dataManager->DoAging();
    CHECK_AND_PRINT_LOG(result == E_OK, "DoAging faild");

    shared_ptr<int> trashCountPtr = make_shared<int>();
    result = dataManager->DoTrashAging(trashCountPtr);
    CHECK_AND_PRINT_LOG(result == E_OK, "DoTrashAging faild");

    VariantMap map = {{KEY_COUNT, *trashCountPtr}};
    PostEventUtils::GetInstance().PostStatProcess(StatType::AGING_STAT, map);
}

static void QueryBurstNeedUpdate(AsyncTaskData *data)
{
    auto dataManager = MediaLibraryDataManager::GetInstance();
    CHECK_AND_RETURN_LOG(dataManager != nullptr,  "dataManager is nullptr");

    int32_t result = dataManager->UpdateBurstFromGallery();
    CHECK_AND_PRINT_LOG(result == E_OK, "UpdateBurstFromGallery faild");
}

static int32_t DoUpdateBurstFromGallery()
{
    MEDIA_INFO_LOG("Begin DoUpdateBurstFromGallery");
    auto asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    CHECK_AND_RETURN_RET_LOG(asyncWorker != nullptr, E_FAIL, "Failed to get async worker instance!");

    shared_ptr<MediaLibraryAsyncTask> updateBurstTask =
        make_shared<MediaLibraryAsyncTask>(QueryBurstNeedUpdate, nullptr);
    CHECK_AND_RETURN_RET_LOG(updateBurstTask != nullptr, E_FAIL,
        "Failed to create async task for updateBurstTask!");
    asyncWorker->AddTask(updateBurstTask, false);
    return E_SUCCESS;
}

static void QueryUpdateSize(AsyncTaskData *data)
{
    auto dataManager = MediaLibraryDataManager::GetInstance();
    CHECK_AND_RETURN_LOG(dataManager != nullptr,  "dataManager is nullptr");

    int32_t result = dataManager->UpdateMediaSizeFromStorage();
    CHECK_AND_PRINT_LOG(result == E_OK, "UpdateMediaSizeFromStorage failed");
}

static int32_t UpdateAllEditDataSize()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(TASK_PROGRESS_XML, errCode);
    if (prefs == nullptr) {
        MEDIA_ERR_LOG("Get preferences error: %{public}d", errCode);
        return E_ERR;
    }
    if (prefs->GetInt(NO_UPDATE_EDITDATA_SIZE, 0) == 1) {
        return E_SUCCESS;
    }

    MEDIA_INFO_LOG("Begin DoUpdateAllEditDataSize");
    auto asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    CHECK_AND_RETURN_RET_LOG(asyncWorker != nullptr, E_FAIL, "Failed to get async worker instance!");

    shared_ptr<MediaLibraryAsyncTask> updateSizeTask =
        make_shared<MediaLibraryAsyncTask>(QueryUpdateSize, nullptr);
    CHECK_AND_RETURN_RET_LOG(updateSizeTask != nullptr, E_FAIL,
        "Failed to create async task for updateBurstTask!");
    asyncWorker->AddTask(updateSizeTask, false);
    return E_SUCCESS;
}

static void UpdateDirtyForCloudClone(AsyncTaskData *data)
{
    auto *taskData = static_cast<UpdateDirtyDataAsyncTaskData *>(data);
    CHECK_AND_RETURN_LOG(taskData != nullptr, "taskData is nullptr!");
    int32_t taskVersion = taskData->taskVersion_;
    auto dataManager = MediaLibraryDataManager::GetInstance();
    CHECK_AND_RETURN_LOG(dataManager != nullptr, "Failed to MediaLibraryDataManager instance!");

    int32_t result = dataManager->UpdateDirtyForCloudClone(taskVersion);
    CHECK_AND_PRINT_LOG(result == E_OK, "UpdateDirtyForCloudClone faild, result = %{public}d", result);
}

static void ClearDirtyHdcData(AsyncTaskData *data)
{
    auto dataManager = MediaLibraryDataManager::GetInstance();
    CHECK_AND_RETURN_LOG(dataManager != nullptr, "Failed to MediaLibraryDataManager instance!");

    int32_t result = dataManager->ClearDirtyHdcData();
    CHECK_AND_PRINT_LOG(result == E_OK, "ClearDirtyHdcData faild, result = %{public}d", result);
}


static int32_t DoUpdateDirtyForCloudClone(int32_t taskVersion)
{
    auto asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    CHECK_AND_RETURN_RET_LOG(asyncWorker != nullptr, E_FAIL,
        "Failed to get async worker instance!");
    auto *taskData = new (std::nothrow) UpdateDirtyDataAsyncTaskData(taskVersion);
    CHECK_AND_RETURN_RET_LOG(taskData != nullptr, E_FAIL, "Failed to alloc async data for update dirty data!");
    shared_ptr<MediaLibraryAsyncTask> updateDirtyForCloudTask =
        make_shared<MediaLibraryAsyncTask>(UpdateDirtyForCloudClone, taskData);
    CHECK_AND_RETURN_RET_LOG(updateDirtyForCloudTask != nullptr, E_FAIL,
        "Failed to create async task for updateDirtyForCloudTask !");
    asyncWorker->AddTask(updateDirtyForCloudTask, false);
    return E_SUCCESS;
}

static int32_t DoClearDirtyHdcData()
{
    auto asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    CHECK_AND_RETURN_RET_LOG(asyncWorker != nullptr, E_FAIL,
        "Failed to get async worker instance!");
    shared_ptr<MediaLibraryAsyncTask> clearDirtyHdcDataTask =
        make_shared<MediaLibraryAsyncTask>(ClearDirtyHdcData, nullptr);
    CHECK_AND_RETURN_RET_LOG(clearDirtyHdcDataTask != nullptr, E_FAIL,
        "Failed to create async task for clearDirtyHdcDataTask !");
    asyncWorker->AddTask(clearDirtyHdcDataTask, false);
    return E_SUCCESS;
}

static void RecoverBackgroundDownloadCloudMediaAsset()
{
    if (!CloudMediaAssetManager::GetInstance().SetBgDownloadPermission(true)) {
        return;
    }
    int32_t ret = CloudMediaAssetManager::GetInstance().RecoverDownloadCloudAsset(
        CloudMediaTaskRecoverCause::BACKGROUND_TASK_AVAILABLE);
    CHECK_AND_PRINT_LOG(ret == E_OK, "RecoverDownloadCloudAsset faild");
}

static void UpdateBurstCoverLevelFromGallery(AsyncTaskData *data)
{
    auto dataManager = MediaLibraryDataManager::GetInstance();
    CHECK_AND_RETURN_LOG(dataManager != nullptr, "dataManager is nullptr");

    int32_t result = dataManager->UpdateBurstCoverLevelFromGallery();
    CHECK_AND_PRINT_LOG(result == E_OK, "UpdateBurstCoverLevelFromGallery faild");
}

static int32_t DoUpdateBurstCoverLevelFromGallery()
{
    MEDIA_INFO_LOG("Begin DoUpdateBurstCoverLevelFromGallery");
    auto asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    CHECK_AND_RETURN_RET_LOG(asyncWorker != nullptr, E_FAIL,
        "Failed to get async worker instance!");

    shared_ptr<MediaLibraryAsyncTask> updateBurstCoverLevelTask =
        make_shared<MediaLibraryAsyncTask>(UpdateBurstCoverLevelFromGallery, nullptr);
    CHECK_AND_RETURN_RET_LOG(updateBurstCoverLevelTask != nullptr, E_FAIL,
        "Failed to create async task for updateBurstCoverLevelTask!");
    asyncWorker->AddTask(updateBurstCoverLevelTask, false);
    return E_SUCCESS;
}

static void UpdatePhotoHdrModeTask(AsyncTaskData *data)
{
    auto dataManager = MediaLibraryDataManager::GetInstance();
    CHECK_AND_RETURN_LOG(dataManager != nullptr, "dataManager is nullptr");

    int32_t result = dataManager->UpdatePhotoHdrMode();
    CHECK_AND_PRINT_LOG(result == E_OK, "UpdatePhotoHdrMode faild");
}

static int32_t DoUpdatePhotoHdrMode()
{
    MEDIA_INFO_LOG("Begin DoUpdatePhotoHdrMode");
    auto asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    CHECK_AND_RETURN_RET_LOG(asyncWorker != nullptr, E_FAIL,
        "Failed to get async worker instance!");

    shared_ptr<MediaLibraryAsyncTask> updateHdrModeTask =
        make_shared<MediaLibraryAsyncTask>(UpdatePhotoHdrModeTask, nullptr);
    CHECK_AND_RETURN_RET_LOG(updateHdrModeTask != nullptr, E_FAIL,
        "Failed to create async task for updateHdrModeTask!");
    asyncWorker->AddTask(updateHdrModeTask, false);
    return E_SUCCESS;
}

static void UpdateDirtyForBeta(const shared_ptr<NativePreferences::Preferences>& prefs)
{
    CHECK_AND_RETURN_LOG((IsBetaVersion() && prefs != nullptr), "not need UpdateDirtyForBeta");
    if (prefs->GetInt(NO_UPDATE_DIRTY, 0) != 1) {
        int32_t ret = DoUpdateDirtyForCloudClone(UPDATE_DIRTY_CLOUD_CLONE_V1);
        CHECK_AND_PRINT_LOG(ret == E_OK, "DoUpdateDirtyForCloudClone failed");
    }
    if (prefs->GetInt(NO_UPDATE_DIRTY_CLOUD_CLONE_V2, 0) != 1) {
        int32_t ret = DoUpdateDirtyForCloudClone(UPDATE_DIRTY_CLOUD_CLONE_V2);
        CHECK_AND_PRINT_LOG(ret == E_OK, "DoUpdateDirtyForCloudClone failed");
    }
    return;
}

static void ClearDirtyHdcData(const shared_ptr<NativePreferences::Preferences>& prefs)
{
    if (prefs != nullptr && (prefs->GetInt(NO_DELETE_DIRTY_HDC_DATA, 0) != 1)) {
        int32_t ret = DoClearDirtyHdcData();
        CHECK_AND_PRINT_LOG(ret == E_OK, "DoUpdateDirtyForCloudClone failed");
    }
    return;
}

void MedialibrarySubscriber::ClearDirtyData()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(TASK_PROGRESS_XML, errCode);
    CHECK_AND_RETURN_LOG(prefs, "Get preferences error: %{public}d", errCode);
    UpdateDirtyForBeta(prefs);
    ClearDirtyHdcData(prefs);
    TryClearContinueCloneData();
    return;
}

static void PeriodicAnalyzePhotosData()
{
    constexpr int64_t analyzeIntervalMillisecond = 24 * 60 * 60 * 1000;
    static int64_t lastAnalyzeTime {0};
    if (MediaFileUtils::UTCTimeMilliSeconds() - lastAnalyzeTime >= analyzeIntervalMillisecond) {
        MediaLibraryRdbUtils::AnalyzePhotosData();
        lastAnalyzeTime = MediaFileUtils::UTCTimeMilliSeconds();
        return;
    }
}

void MedialibrarySubscriber::AgingTmpCompatibleDuplicates(bool isAge)
{
    auto dataAging = MediaLibraryTranscodeDataAgingOperation::GetInstance();
    CHECK_AND_RETURN_LOG(dataAging != nullptr, "dataAging is nullptr");
    if (isAge) {
        dataAging->AgingTmpCompatibleDuplicates();
    } else {
        dataAging->InterruptAgingTmpCompatibleDuplicates();
    }
}

void UploadHiddenAssetsToCloud(const shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    MEDIA_INFO_LOG("Begin InsertOthersAndHiddenSourceAlbum");
    const int32_t hiddenStatus = 1;
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_HIDDEN, hiddenStatus);
    predicates.EqualTo(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_SYNCED));

    NativeRdb::ValuesBucket value;
    value.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_MDIRTY));

    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, value, predicates);
    CHECK_AND_WARN_LOG(ret == E_OK,
        "Update hidden asset dirty failed, ret = %{public}d, changedRows = %{public}d", ret, changedRows);
    return;
}

void InsertOthersSourceAlbum(const shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    MEDIA_INFO_LOG("Begin InsertOthersSourceAlbum");
    const std::string CREATE_OTHERS_ALBUM =
        "INSERT OR REPLACE INTO " + PhotoAlbumColumns::TABLE +
            "(album_type, album_subtype, album_name,bundle_name, dirty, is_local, date_modified, " +
            "date_added, lpath, priority) Values ('2048', '2049', '其它', 'com.other.album', '1', '1', " +
            "strftime('%s000', 'now'), strftime('%s000', 'now'), '/Pictures/其它', '1')";
    int32_t ret = rdbStore->ExecuteSql(CREATE_OTHERS_ALBUM);
    CHECK_AND_RETURN_WARN_LOG(ret == E_OK,
        "Can not insert othersAlbum into PhotoAlbum Table, ret = %{public}d", ret);
    return;
}

void InsertAndUpdateHiddenSourceAlbum(const shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    MEDIA_INFO_LOG("Begin InsertAndUpdateHiddenSourceAlbum");
    const std::string createHiddenAlbum =
        "INSERT OR REPLACE INTO " + PhotoAlbumColumns::TABLE +
            "(album_type, album_subtype, album_name, bundle_name, dirty, is_local, date_added, lpath, priority)"
            " Values ('2048', '2049', '.hiddenAlbum', 'com.hidden.album', '1', "
            "'1', strftime('%s000', 'now'), '/Pictures/hiddenAlbum', '1')";
    int32_t ret = rdbStore->ExecuteSql(createHiddenAlbum);
    CHECK_AND_RETURN_WARN_LOG(ret == E_OK,
        "Can not insert hiddenAlbum into PhotoAlbum Table, ret = %{public}d", ret);
    UploadHiddenAssetsToCloud(rdbStore);
    return;
}

int32_t DoRecoverCloudHiddenAssets()
{
    MEDIA_INFO_LOG("Begin DoRecoverCloudHiddenAssets");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "Failed to get rdbStore");

    const std::string hiddenAlbumLpath = "/Pictures/hiddenAlbum";
    const std::string othersAlbumLpath = "/Pictures/其它";
    std::vector<std::string> lpaths = {hiddenAlbumLpath, othersAlbumLpath};

    NativeRdb::RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.In(PhotoAlbumColumns::ALBUM_LPATH, lpaths);
    predicates.NotEqualTo(PhotoAlbumColumns::ALBUM_DIRTY, to_string(static_cast<int32_t>(DirtyTypes::TYPE_DELETED)));

    std::vector<std::string> columns = {PhotoAlbumColumns::ALBUM_LPATH};
    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "Failed to get resultSet");

    int32_t numRows = -1;
    int32_t ret = resultSet->GetRowCount(numRows);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK && numRows >= 0, E_ERR, "Failed to get resultSet rowCount");

    CHECK_AND_RETURN_RET_LOG(static_cast<size_t>(numRows) != lpaths.size(), E_SUCCESS, "Not need to recover albums");

    bool isInsertHiddenAlbum = true;
    bool isInsertOthersAlbum = true;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string lpath = get<std::string>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_LPATH,
            resultSet, TYPE_STRING));
        CHECK_AND_EXECUTE(lpath != hiddenAlbumLpath, isInsertHiddenAlbum = false);
        CHECK_AND_EXECUTE(lpath != othersAlbumLpath, isInsertOthersAlbum = false);
    }
    CHECK_AND_EXECUTE(!isInsertHiddenAlbum, InsertAndUpdateHiddenSourceAlbum(rdbStore));
    CHECK_AND_EXECUTE(!isInsertOthersAlbum, InsertOthersSourceAlbum(rdbStore));
    return E_SUCCESS;
}

void MedialibrarySubscriber::UpdateMediaInLakeCheckStatus()
{
    std::lock_guard<std::mutex> lock(mutex_);
    bool isPowerSufficientForInLakeCheck = batteryCapacity_ >= PROPER_DEVICE_BATTERY_CAPACITY;
    bool newStatus = isCharging_ && isScreenOff_ && isPowerSufficientForInLakeCheck &&
        newTemperatureLevel_ <= PROPER_DEVICE_TEMPERATURE_LEVEL_37;

    if (checkInLakeStatus_ == newStatus) {
        return;
    }
    checkInLakeStatus_ = newStatus;
    MEDIA_INFO_LOG("lake update status new:%{public}d, s:%{public}d, c:%{public}d, b:%{public}d, t:%{public}d",
        newStatus, isScreenOff_, isCharging_, batteryCapacity_, newTemperatureLevel_);

    if (newStatus) {
        MediaInLakeCheckManager::GetInstance()->Start();
    } else {
        MediaInLakeCheckManager::GetInstance()->Stop();
    }
}

void MedialibrarySubscriber::DoBackgroundOperation()
{
    bool cond = (!backgroundDelayTask_.IsDelayTaskTimeOut() || !currentStatus_);
    CHECK_AND_RETURN_LOG(!cond, "The conditions for DoBackgroundOperation are not met, will return.");
#ifdef META_RECOVERY_SUPPORT
    // check metadata recovery state
    MediaLibraryMetaRecovery::GetInstance().CheckRecoveryState();
#endif
    MediaLibraryDataManager::GetInstance()->RestoreInvalidPosData();
    AgingTmpCompatibleDuplicates(true);
    // delete temporary photos
    DeleteTemporaryPhotos();
    // clear dirty data
    ClearDirtyData();
    BackgroundTaskMgr::EfficiencyResourceInfo resourceInfo = BackgroundTaskMgr::EfficiencyResourceInfo(
        BackgroundTaskMgr::ResourceType::CPU, true, 0, "apply", true, true);
    BackgroundTaskMgr::BackgroundTaskMgrHelper::ApplyEfficiencyResources(resourceInfo);
    Init();
    DoAgingOperation();
    PeriodicAnalyzePhotosData();
    // update burst from gallery
    int32_t ret = DoUpdateBurstFromGallery();
    CHECK_AND_PRINT_LOG(ret == E_OK, "DoUpdateBurstFromGallery faild");
    // update all editdata size
    ret = UpdateAllEditDataSize();
    CHECK_AND_PRINT_LOG(ret == E_OK, "DoUpdateAllEditDataSize faild");
    CloudUploadChecker::RepairNoOriginPhoto();

    // recover cloud hidden assets
    ret = DoRecoverCloudHiddenAssets();
    CHECK_AND_PRINT_LOG(ret == E_OK, "DoRecoverCloudHiddenAssets faild");

#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
    // add permission for cloud enhancement photo
    CloudEnhancementChecker::AddPermissionForCloudEnhancement();
#endif
    // migration highlight info to new path
    if (MediaFileUtils::IsFileExists(ROOT_MEDIA_DIR + HIGHLIGHT_INFO_OLD)) {
        MEDIA_INFO_LOG("Migration highlight info to new path");
        bool retMigration = MediaFileUtils::CopyDirAndDelSrc(ROOT_MEDIA_DIR + HIGHLIGHT_INFO_OLD,
            ROOT_MEDIA_DIR + HIGHLIGHT_INFO_NEW);
        if (retMigration) {
            bool retDelete = MediaFileUtils::DeleteDir(ROOT_MEDIA_DIR + HIGHLIGHT_INFO_OLD);
            if (!retDelete) {
                MEDIA_ERR_LOG("Delete old highlight path fail");
            }
        }
    }
    ret = DoUpdateBurstCoverLevelFromGallery();
    CHECK_AND_PRINT_LOG(ret == E_OK, "DoUpdateBurstCoverLevelFromGallery faild");
    DoUpdatePhotoHdrMode();
    RecoverBackgroundDownloadCloudMediaAsset();
    CloudMediaAssetManager::GetInstance().StartDeleteCloudMediaAssets();
    // compat old-version moving photo
    MovingPhotoProcessor::StartProcess();
    MediaLibraryAlbumFusionUtils::CleanInvalidCloudAlbumAndData(true);
    auto watch = MediaLibraryInotify::GetInstance();
    if (watch != nullptr) {
        watch->DoAging();
    }
    DoBackgroundOperationStepTwo();
}

void MedialibrarySubscriber::DoBackgroundOperationStepTwo()
{
    DfxMovingPhoto::AbnormalMovingPhotoStatistics();
    PhotoMimetypeOperation::UpdateInvalidMimeType();
    HeightWidthCorrectOperation::UpdateHeightAndWidth();
    ShootingModeAlbumOperation::UpdateShootingModeAlbum();
    DfxManager::GetInstance()->HandleTwoDayMissions();
    DfxManager::GetInstance()->HandleOneWeekMissions();
    PhotoDayMonthYearOperation::RepairDateTime();
    MapCodeUploadChecker::RepairNoMapCodePhoto();
    MediaLibraryAspectRatioOperation::UpdateAspectRatioValue();
    backgroundTaskFactory_.Execute();
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
    CloudEnhancementChecker::RecognizeCloudEnhancementPhotosByDisplayName();
#endif
    int32_t ret = DoCloudMediaRetainCleanup();
    CHECK_AND_PRINT_LOG(ret == E_OK, "Failed to schedule DoCleanPhotosTableCloudData task");
    ThumbnailService::GetInstance()->DfxReportThumbnailDirAcl();
    ResetCloneFlagAfterOneDay();
}

static void PauseBackgroundDownloadCloudMedia()
{
    if (!CloudMediaAssetManager::GetInstance().SetBgDownloadPermission(false)) {
        return;
    }
    int32_t taskStatus = static_cast<int32_t>(CloudMediaAssetTaskStatus::DOWNLOADING);
    int32_t downloadType = static_cast<int32_t>(CloudMediaDownloadType::DOWNLOAD_GENTLE);
    if (CloudMediaAssetManager::GetInstance().GetTaskStatus() == taskStatus &&
        CloudMediaAssetManager::GetInstance().GetDownloadType() == downloadType) {
        CloudMediaAssetManager::GetInstance().PauseDownloadCloudAsset(
            CloudMediaTaskPauseCause::BACKGROUND_TASK_UNAVAILABLE);
    }
}

void MedialibrarySubscriber::StopBackgroundOperation()
{
#ifdef META_RECOVERY_SUPPORT
    MediaLibraryMetaRecovery::GetInstance().InterruptRecovery();
#endif
    MovingPhotoProcessor::StopProcess();
    MediaLibraryDataManager::GetInstance()->InterruptBgworker();
    PauseBackgroundDownloadCloudMedia();
    PhotoAlbumLPathOperation::GetInstance().Stop();
    CloudMediaAssetManager::GetInstance().StopDeleteCloudMediaAssets();
    HeightWidthCorrectOperation::Stop();
    MediaLibraryAspectRatioOperation::Stop();
    ShootingModeAlbumOperation::Stop();
    AgingTmpCompatibleDuplicates(false);
}

void MedialibrarySubscriber::UpdateThumbnailBgGenerationStatus()
{
    std::lock_guard<std::mutex> lock(mutex_);
    bool isPowerSufficientForThumbnail = batteryCapacity_ >= PROPER_DEVICE_BATTERY_CAPACITY_THUMBNAIL;
    bool newStatus = false;
    if (system::GetParameter(CLONE_FLAG, "0") != "0") {
        newStatus = false;
    } else if (isCharging_) {
        newStatus = isScreenOff_ && isPowerSufficientForThumbnail &&
            newTemperatureLevel_ <= PROPER_DEVICE_TEMPERATURE_LEVEL_43;
    } else if (isScreenOff_ && newTemperatureLevel_ <= PROPER_DEVICE_TEMPERATURE_LEVEL_40 &&
        batteryCapacity_ >= PROPER_DEVICE_BATTERY_CAPACITY) {
        int32_t thumbAstcCount = 0;
        int32_t thumbTotalCount = 0;
        MedialibrarySubscriberDatabaseUtils::QueryThumbAstc(thumbAstcCount);
        MedialibrarySubscriberDatabaseUtils::QueryThumbTotal(thumbTotalCount);
        bool isThumbAstcEnough = thumbAstcCount > THUMB_ASTC_ENOUGH || thumbAstcCount == thumbTotalCount;
        newStatus = !isThumbAstcEnough;
        CHECK_AND_PRINT_LOG(isThumbAstcEnough,
            "ThumbnailBg generate status: isThumbAstcEnough:%{public}d,"
            " thumbAstcCount:%{public}d, thumbTotalCount:%{public}d",
            isThumbAstcEnough, thumbAstcCount, thumbTotalCount);
    }

    if (thumbnailBgGenerationStatus_ == newStatus) {
        return;
    }
    MEDIA_INFO_LOG("update status thumbnailBg:%{public}d, new:%{public}d, "
        "%{public}d, %{public}d, %{public}d, %{public}d",
        thumbnailBgGenerationStatus_, newStatus, isScreenOff_,
        isCharging_, batteryCapacity_, newTemperatureLevel_);
    thumbnailBgGenerationStatus_ = newStatus;

    thumbnailBgDelayTask_.EndBackgroundOperationThread();
    if (thumbnailBgGenerationStatus_) {
        thumbnailBgDelayTask_.SetOperationThread([this] { this->DoThumbnailBgOperation(); });
    } else {
        MediaLibraryAstcStat::GetInstance().GetInterruptInfo(isScreenOff_, isCharging_,
            isPowerSufficientForThumbnail,
            newTemperatureLevel_ <= PROPER_DEVICE_TEMPERATURE_LEVEL_40);
        StopThumbnailBgOperation();
    }
}

void MedialibrarySubscriber::DoThumbnailBgOperation()
{
    bool cond = (!thumbnailBgDelayTask_.IsDelayTaskTimeOut() || !thumbnailBgGenerationStatus_);
    CHECK_AND_RETURN_LOG(!cond, "The conditions for DoThumbnailBgOperation are not met, will return.");

    auto dataManager = MediaLibraryDataManager::GetInstance();
    CHECK_AND_RETURN_LOG(dataManager != nullptr, "dataManager is nullptr");

    auto result = dataManager->GenerateThumbnailBackground();
    CHECK_AND_PRINT_LOG(result == E_OK, "GenerateThumbnailBackground faild");

    result = dataManager->UpgradeThumbnailBackground(isWifiConnected_);
    CHECK_AND_PRINT_LOG(result == E_OK, "UpgradeThumbnailBackground faild");

    result = dataManager->GenerateHighlightThumbnailBackground();
    CHECK_AND_PRINT_LOG(result == E_OK, "GenerateHighlightThumbnailBackground failed %{public}d", result);
}

void MedialibrarySubscriber::StopThumbnailBgOperation()
{
    MediaLibraryDataManager::GetInstance()->InterruptThumbnailBgWorker();
}

#ifdef MEDIALIBRARY_MTP_ENABLE
void MedialibrarySubscriber::DoStartMtpService()
{
    AAFwk::Want want;
    want.SetElementName("com.ohos.medialibrary.medialibrarydata", "MtpService");
    auto abilityContext = AbilityRuntime::Context::GetApplicationContext();
    ErrCode err = AAFwk::AbilityManagerClient::GetInstance()->StartAbility(want, abilityContext->GetToken(),
        OHOS::AAFwk::DEFAULT_INVAL_VALUE);
    MEDIA_INFO_LOG("MedialibrarySubscriber::DoStartMtpService. End calling StartAbility. ret=%{public}d", err);
}
#endif

void MedialibrarySubscriber::RevertPendingByPackage(const std::string &bundleName)
{
    MediaLibraryDataManager::GetInstance()->RevertPendingByPackage(bundleName);
}

#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
void MedialibrarySubscriber::UpdateBackgroundTimer()
{
    std::lock_guard<std::mutex> lock(mutex_);
    BackgroundCloudBatchSelectedFileProcessor::TriggerAutoResumeBatchDownloadResourceCheck();
    MEDIA_DEBUG_LOG("UpdateBackgroundTimer TriggerBatchDownloadResource after");
    if (BackgroundCloudBatchSelectedFileProcessor::IsBatchDownloadProcessRunningStatus()) {
        // 触发了批量下载 后台下载可以暂不触发
        MEDIA_INFO_LOG("UpdateBackgroundTimer no allow download 30 day pic");
        return;
    }
    MEDIA_DEBUG_LOG("UpdateBackgroundTimer Allow Download 30 day Pic");
    bool isPowerSufficient = batteryCapacity_ >= PROPER_DEVICE_BATTERY_CAPACITY;
    bool newStatus = isScreenOff_ && isCharging_ && isPowerSufficient &&
        isDeviceTemperatureProper_ && isWifiConnected_;
    if (timerStatus_ == newStatus) {
        return;
    }

    MEDIA_INFO_LOG("update timer status current:%{public}d, new:%{public}d, %{public}d, %{public}d, %{public}d, "
        "%{public}d, %{public}d",
        timerStatus_, newStatus, isScreenOff_, isCharging_, isPowerSufficient, isDeviceTemperatureProper_,
        isWifiConnected_);

    timerStatus_ = newStatus;
    if (timerStatus_) {
        BackgroundCloudFileProcessor::StartTimer();
    } else {
        BackgroundCloudFileProcessor::StopTimer();
    }
}
#endif

void MedialibrarySubscriber::DealWithEventsAfterUpdateStatus(const StatusEventType statusEventType)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (deviceTemperatureLevel_ != newTemperatureLevel_) {
        deviceTemperatureLevel_ = newTemperatureLevel_;
        ThumbnailService::GetInstance()->NotifyTempStatusForReady(deviceTemperatureLevel_);
    }

    CloudSyncDfxManager::GetInstance().RunDfx();
    ThumbnailService::GetInstance()->UpdateCurrentStatusForTask(thumbnailBgGenerationStatus_);
    ThumbnailGenerateWorkerManager::GetInstance().TryCloseThumbnailWorkerTimer();
    MediaLibraryKvStoreManager::GetInstance().TryCloseAllKvStore();
    PowerEfficiencyManager::SetSubscriberStatus(isCharging_, isScreenOff_);

    if (statusEventType == StatusEventType::SCREEN_ON || statusEventType == StatusEventType::SCREEN_OFF) {
        ThumbnailService::GetInstance()->RestoreThumbnailOnScreenStateChanged(!isScreenOff_);
    }

    if (statusEventType == StatusEventType::SCREEN_OFF || statusEventType == StatusEventType::CHARGING) {
        WalCheckPointAsync();
        CheckHalfDayMissions();
        return;
    }

    if (statusEventType == StatusEventType::SCREEN_ON || statusEventType == StatusEventType::DISCHARGING) {
        CheckHalfDayMissions();
        return;
    }

    if (statusEventType == StatusEventType::THERMAL_LEVEL_CHANGED) {
        MEDIA_INFO_LOG("Current temperature level is %{public}d", newTemperatureLevel_);
        PowerEfficiencyManager::UpdateAlbumUpdateInterval(isDeviceTemperatureProper_);
    }
}

void MedialibrarySubscriber::UploadDB()
{
    std::tm nowLocalTime;
    const int32_t BACKUP_START_TIME = 23;
    const int32_t BACKUP_END_TIME = 5;
    if (GetNowLocalTime(nowLocalTime) &&
        (nowLocalTime.tm_hour >= BACKUP_START_TIME || nowLocalTime.tm_hour < BACKUP_END_TIME) &&
        IsTwelveHoursAgo()) {
        MEDIA_INFO_LOG("Version is BetaVersion, UploadDBFile, now:%{public}d, batteryCapacity:%{public}d",
            nowLocalTime.tm_hour, batteryCapacity_);
        UploadDBFile();
    }
}

bool MedialibrarySubscriber::DelayTask::IsDelayTaskTimeOut()
{
    std::unique_lock<std::mutex> lock(this->lock);
    return !cv.wait_for(lock, std::chrono::milliseconds(DELAY_TASK_TIME), [this]() {
        return !isTaskWaiting;
    });
}

void MedialibrarySubscriber::DelayTask::EndBackgroundOperationThread()
{
    {
        std::unique_lock<std::mutex> lock(this->lock);
        isTaskWaiting = false;
        MEDIA_INFO_LOG("DelayTask %{public}s EndBackgroundOperationThread", taskName.c_str());
    }
#ifdef META_RECOVERY_SUPPORT
    MediaLibraryMetaRecovery::GetInstance().InterruptRecovery();
#endif
    MovingPhotoProcessor::StopProcess();
    cv.notify_all();
    if (!operationThread.joinable()) {
        return;
    }
    operationThread.join();
}

void MedialibrarySubscriber::DelayTask::SetOperationThread(std::function<void()> operationTask)
{
    if (operationThread.joinable()) {
        operationThread.join();
    }
    {
        std::unique_lock<std::mutex> lock(this->lock);
        isTaskWaiting = true;
    }
    this->operationThread = std::thread(operationTask);
}

int32_t MedialibrarySubscriberDatabaseUtils::QueryThumbAstc(int32_t& thumbAstcCount)
{
    std::vector<std::string> columns = { "count(1) AS count" };
    std::string queryColumn = "count";
    NativeRdb::RdbPredicates astcPredicates(PhotoColumn::PHOTOS_TABLE);
    astcPredicates.GreaterThanOrEqualTo(PhotoColumn::PHOTO_THUMBNAIL_READY,
        static_cast<int32_t>(ThumbnailReady::GENERATE_THUMB_RETRY));
    int32_t errCode = QueryInt(astcPredicates, columns, queryColumn, thumbAstcCount);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Query thumbAstcCount fail: %{public}d", errCode);
    return E_OK;
}

int32_t MedialibrarySubscriberDatabaseUtils::QueryThumbTotal(int32_t& thumbTotalCount)
{
    std::vector<std::string> columns = { "count(1) AS count" };
    std::string queryColumn = "count";
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    int32_t errCode = QueryInt(predicates, columns, queryColumn, thumbTotalCount);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Query thumbTotalCount fail: %{public}d", errCode);
    return E_OK;
}

int32_t MedialibrarySubscriberDatabaseUtils::QueryInt(const NativeRdb::AbsRdbPredicates &predicates,
    const std::vector<std::string> &columns, const std::string &queryColumn, int32_t &value)
{
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    bool cond = (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK);
    CHECK_AND_RETURN_RET(!cond, E_DB_FAIL);
    value = GetInt32Val(queryColumn, resultSet);
    return E_OK;
}

void MedialibrarySubscriber::ResetCloneFlagAfterOneDay()
{
    std::string cloneFlagStr = system::GetParameter(CLONE_FLAG, "0");
    if (cloneFlagStr == "0") {
        MEDIA_INFO_LOG("Clone flag is 0, no need to reset");
        return;
    }
    int64_t cloneStartTime = 0;
    if (!cloneFlagStr.empty()) {
        cloneStartTime = static_cast<int64_t>(MediaFileUtils::StrToInt64(cloneFlagStr));
    }
    auto currentTime = MediaFileUtils::UTCTimeSeconds();
    int64_t timeCost = currentTime - cloneStartTime;
    const int64_t oneDayInSeconds = 86400;
    if (timeCost >= oneDayInSeconds) {
        bool retFlag = system::SetParameter(CLONE_FLAG, "0");
        CHECK_AND_PRINT_LOG(retFlag, "Failed to set stop parameter cloneFlag, retFlag:%{public}d", retFlag);
    }
}
}  // namespace Media
}  // namespace OHOS

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
#define MLOG_TAG "Subscribe"

#include "medialibrary_subscriber.h"

#include <memory>
#include "appexecfwk_errors.h"
#include "background_task_mgr_helper.h"
#ifdef HAS_BATTERY_MANAGER_PART
#include "battery_srv_client.h"
#endif
#include "bundle_info.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "want.h"
#include "post_event_utils.h"
#ifdef HAS_POWER_MANAGER_PART
#include "power_mgr_client.h"
#endif
#ifdef HAS_THERMAL_MANAGER_PART
#include "thermal_mgr_client.h"
#endif

#include "media_actively_calling_analyse.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_inotify.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_scanner_manager.h"
#include "application_context.h"
#include "ability_manager_client.h"
#include "resource_type.h"
#include "dfx_manager.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_rdb_utils.h"

using namespace OHOS::AAFwk;

namespace OHOS {
namespace Media {
// The task can be performed when the battery level reaches the value
const int32_t PROPER_DEVICE_BATTERY_CAPACITY = 50;

// The task can be performed only when the temperature of the device is lower than the value
// Level 0: The device temperature is lower than 35℃
// Level 1: The device temperature ranges from 35℃ to 37℃
const int32_t PROPER_DEVICE_TEMPERATURE_LEVEL = 1;
const int32_t COMMON_EVENT_KEY_GET_DEFAULT_PARAM = -1;
const std::string COMMON_EVENT_KEY_BATTERY_CAPACITY = "soc";
const std::string COMMON_EVENT_KEY_DEVICE_TEMPERATURE = "0";
const std::vector<std::string> MedialibrarySubscriber::events_ = {
    EventFwk::CommonEventSupport::COMMON_EVENT_CHARGING,
    EventFwk::CommonEventSupport::COMMON_EVENT_DISCHARGING,
    EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_OFF,
    EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_ON,
    EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED,
    EventFwk::CommonEventSupport::COMMON_EVENT_BATTERY_CHANGED,
    EventFwk::CommonEventSupport::COMMON_EVENT_THERMAL_LEVEL_CHANGED
};

const std::map<std::string, StatusEventType> BACKGROUND_OPERATION_STATUS_MAP = {
    {EventFwk::CommonEventSupport::COMMON_EVENT_CHARGING, StatusEventType::CHARGING},
    {EventFwk::CommonEventSupport::COMMON_EVENT_DISCHARGING, StatusEventType::DISCHARGING},
    {EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_OFF, StatusEventType::SCREEN_OFF},
    {EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_ON, StatusEventType::SCREEN_ON},
    {EventFwk::CommonEventSupport::COMMON_EVENT_BATTERY_CHANGED, StatusEventType::BATTERY_CHANGED},
    {EventFwk::CommonEventSupport::COMMON_EVENT_THERMAL_LEVEL_CHANGED, StatusEventType::THERMAL_LEVEL_CHANGED},
};

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
    isPowerSufficient_ = batteryClient.GetCapacity() >= PROPER_DEVICE_BATTERY_CAPACITY;
#endif
#ifdef HAS_THERMAL_MANAGER_PART
    auto& thermalMgrClient = PowerMgr::ThermalMgrClient::GetInstance();
    isDeviceTemperatureProper_ = static_cast<int32_t>(
        thermalMgrClient.GetThermalLevel()) <= PROPER_DEVICE_TEMPERATURE_LEVEL;
#endif
    MEDIA_INFO_LOG("MedialibrarySubscriber current status:%{public}d, %{public}d, %{public}d, %{public}d",
        isScreenOff_, isCharging_, isPowerSufficient_, isDeviceTemperatureProper_);
}

bool MedialibrarySubscriber::Subscribe(void)
{
    EventFwk::MatchingSkills matchingSkills;
    for (auto event : events_) {
        matchingSkills.AddEvent(event);
    }
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);

    std::shared_ptr<MedialibrarySubscriber> subscriber = std::make_shared<MedialibrarySubscriber>(subscribeInfo);
    return EventFwk::CommonEventManager::SubscribeCommonEvent(subscriber);
}

void MedialibrarySubscriber::CheckHalfDayMissions()
{
    if (isScreenOff_ && isCharging_) {
        DfxManager::GetInstance()->HandleHalfDayMissions();
    }
}

void MedialibrarySubscriber::UpdateCurrentStatus()
{
    bool newStatus = isScreenOff_ && isCharging_ && isPowerSufficient_ && isDeviceTemperatureProper_;
    MEDIA_INFO_LOG("update status current:%{public}d, new:%{public}d, %{public}d, %{public}d, %{public}d, %{public}d",
        currentStatus_, newStatus, isScreenOff_, isCharging_, isPowerSufficient_, isDeviceTemperatureProper_);
    if (currentStatus_ == newStatus) {
        return;
    }
    currentStatus_ = newStatus;
    if (currentStatus_) {
        DoBackgroundOperation();
    } else {
        StopBackgroundOperation();
    }
}

void MedialibrarySubscriber::StartAnalysisService()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw()->GetRaw();
    bool hasData = MediaLibraryRdbUtils::HasDataToAnalysis(rdbStore);\
    if (!hasData) {
        MEDIA_INFO_LOG("No data to analysis");
        return;
    }
    int32_t code = MediaActivelyCallingAnalyse::ActivateServiceType::START_BACKGROUND_TASK;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    MediaActivelyCallingAnalyse mediaActivelyCallingAnalyse(nullptr);
    if (!mediaActivelyCallingAnalyse.SendTransactCmd(code, data, reply, option)) {
        MEDIA_ERR_LOG("StartAnalysisService Fail");
    }
}

void MedialibrarySubscriber::UpdateBackgroundOperationStatus(
    const AAFwk::Want &want, const StatusEventType statusEventType)
{
    switch (statusEventType) {
        case StatusEventType::SCREEN_OFF:
            isScreenOff_ = true;
            CheckHalfDayMissions();
            break;
        case StatusEventType::SCREEN_ON:
            isScreenOff_ = false;
            break;
        case StatusEventType::CHARGING:
            isCharging_ = true;
            CheckHalfDayMissions();
            break;
        case StatusEventType::DISCHARGING:
            isCharging_ = false;
            break;
        case StatusEventType::BATTERY_CHANGED:
            isPowerSufficient_ = want.GetIntParam(COMMON_EVENT_KEY_BATTERY_CAPACITY,
                COMMON_EVENT_KEY_GET_DEFAULT_PARAM) >= PROPER_DEVICE_BATTERY_CAPACITY;
            break;
        case StatusEventType::THERMAL_LEVEL_CHANGED:
            isDeviceTemperatureProper_ = want.GetIntParam(COMMON_EVENT_KEY_DEVICE_TEMPERATURE,
                COMMON_EVENT_KEY_GET_DEFAULT_PARAM) <= PROPER_DEVICE_TEMPERATURE_LEVEL;
            break;
        default:
            MEDIA_WARN_LOG("StatusEventType:%{public}d is not invalid", statusEventType);
            return;
    }

    UpdateCurrentStatus();
}

void MedialibrarySubscriber::OnReceiveEvent(const EventFwk::CommonEventData &eventData)
{
    const AAFwk::Want& want = eventData.GetWant();
    std::string action = want.GetAction();
    MEDIA_INFO_LOG("OnReceiveEvent action:%{public}s.", action.c_str());
    if (BACKGROUND_OPERATION_STATUS_MAP.count(action) != 0) {
        UpdateBackgroundOperationStatus(want, BACKGROUND_OPERATION_STATUS_MAP.at(action));
    } else if (action.compare(EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED) == 0) {
        string packageName = want.GetElement().GetBundleName();
        RevertPendingByPackage(packageName);
        MediaLibraryBundleManager::GetInstance()->Clear();
    }
}

int64_t MedialibrarySubscriber::GetNowTime()
{
    struct timespec t;
    constexpr int64_t SEC_TO_MSEC = 1e3;
    constexpr int64_t MSEC_TO_NSEC = 1e6;
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
    string where = PhotoColumn::PHOTO_IS_TEMP + " = 1 AND (" + PhotoColumn::MEDIA_DATE_ADDED + " <= " +
        to_string(timeBefore24Hours) + " OR " + MediaColumn::MEDIA_ID + " NOT IN (SELECT " + MediaColumn::MEDIA_ID +
        " FROM (SELECT " + MediaColumn::MEDIA_ID + " FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " +
        PhotoColumn::PHOTO_IS_TEMP + " = 1 " + "ORDER BY " + MediaColumn::MEDIA_ID +
        " DESC LIMIT 50)) AND (select COUNT(1) from " + PhotoColumn::PHOTOS_TABLE +
        " where " + PhotoColumn::PHOTO_IS_TEMP + " = 1) > 100) ";
    predicates.SetWhereClause(where);

    auto changedRows = dataManager->Update(cmd, valuesBucket, predicates);
    if (changedRows < 0) {
        MEDIA_INFO_LOG("Failed to update property of asset, err: %{public}d", changedRows);
        return;
    }
    MEDIA_INFO_LOG("delete %{public}d temp files exceeding 24 hous or exceed maximum quantity.", changedRows);
}

void MedialibrarySubscriber::DoBackgroundOperation()
{
    if (!currentStatus_) {
        MEDIA_INFO_LOG("The conditions for DoBackgroundOperation are not met, will return.");
        return;
    }

    // delete temporary photos
    DeleteTemporaryPhotos();

    BackgroundTaskMgr::EfficiencyResourceInfo resourceInfo = BackgroundTaskMgr::EfficiencyResourceInfo(
        BackgroundTaskMgr::ResourceType::CPU, true, 0, "apply", true, true);
    BackgroundTaskMgr::BackgroundTaskMgrHelper::ApplyEfficiencyResources(resourceInfo);
    Init();
    auto dataManager = MediaLibraryDataManager::GetInstance();
    if (dataManager == nullptr) {
        return;
    }

    auto result = dataManager->GenerateThumbnailBackground();
    if (result != E_OK) {
        MEDIA_ERR_LOG("GenerateThumbnailBackground faild");
    }

    result = dataManager->UpgradeThumbnailBackground();
    if (result != E_OK) {
        MEDIA_ERR_LOG("UpgradeThumbnailBackground faild");
    }

    result = dataManager->DoAging();
    if (result != E_OK) {
        MEDIA_ERR_LOG("DoAging faild");
    }

    shared_ptr<int> trashCountPtr = make_shared<int>();
    result = dataManager->DoTrashAging(trashCountPtr);
    if (result != E_OK) {
        MEDIA_ERR_LOG("DoTrashAging faild");
    }

    VariantMap map = {{KEY_COUNT, *trashCountPtr}};
    PostEventUtils::GetInstance().PostStatProcess(StatType::AGING_STAT, map);

    auto watch = MediaLibraryInotify::GetInstance();
    if (watch != nullptr) {
        watch->DoAging();
    }
    auto scannerManager = MediaScannerManager::GetInstance();
    if (scannerManager == nullptr) {
        return;
    }
    scannerManager->ScanError();

    MEDIA_INFO_LOG("Do success, current status:%{public}d, %{public}d, %{public}d, %{public}d, %{public}d",
        currentStatus_, isScreenOff_, isCharging_, isPowerSufficient_, isDeviceTemperatureProper_);
}

void MedialibrarySubscriber::StopBackgroundOperation()
{
    MediaLibraryDataManager::GetInstance()->InterruptBgworker();
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
}  // namespace Media
}  // namespace OHOS

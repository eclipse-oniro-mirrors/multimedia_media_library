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

#define MLOG_TAG "ThumbnailDoubleUpgradeConfigManager"

#include "thumbnail_double_upgrade_config_manager.h"

#include <chrono>
#include <cinttypes>
#include <thread>

#include "common_timer_errors.h"
#include "parameter.h"
#include "parameters.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "preferences_errno.h"
#include "os_account_info.h"
#include "os_account_manager.h"

#include "dfx_database_utils.h"
#include "dfx_reporter.h"
#include "medialibrary_astc_stat.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "thumbnail_const.h"

namespace OHOS {
namespace Media {
namespace {
const char* DOUBLE_UPGRADE_SP_FILE = "/data/storage/el2/base/preferences/double_upgrade_config_sp.xml";

const char* KEY_DOUBLE_UPGRADE_STATUS = "double_update_single_flag";
const char* KEY_DOUBLE_UPGRADE_START_TIME = "double_upgrade_start_time";
const char* KEY_DOUBLE_UPGRADE_END_TIME = "double_upgrade_end_time";
const char* KEY_DOUBLE_UPGRADE_DATA_REPORT_FLAG = "double_upgrade_data_report_flag";

const std::string DOUBLE_UPGRADE_IN_PROGRESS = "1";
const std::string DOUBLE_UPGRADE_COMPLETED = "0";

const char* MAIN_SPACE_DOUBLE_UPGRADE_FLAG = "persist.update.hmos_to_next_flag";
const char* PRIVATE_SPACE_DOUBLE_UPGRADE_FLAG = "persist.update.private_hmos_to_next_flag";

constexpr uint32_t DOUBLE_UPGRADE_TIMEOUT_MS = 12 * 60 * 60 * 1000;

constexpr int32_t STORAGE_SIZE_256G = 256;
constexpr int32_t STORAGE_SIZE_512G = 512;
constexpr int32_t STORAGE_SIZE_1T = 1024;

constexpr int32_t STORAGE_SIZE_LEFT_THRESHOLD_256G = 30;
constexpr int32_t STORAGE_SIZE_LEFT_THRESHOLD_512G = 20;
constexpr int32_t STORAGE_SIZE_LEFT_THRESHOLD_1T = 15;

constexpr int64_t ONE_GB_CAST_BYTE = 1024 * 1024 * 1024;
} // namespace

ThumbnailDoubleUpgradeConfigManager& ThumbnailDoubleUpgradeConfigManager::GetInstance()
{
    static ThumbnailDoubleUpgradeConfigManager instance;
    return instance;
}

ThumbnailDoubleUpgradeConfigManager::ThumbnailDoubleUpgradeConfigManager()
{
    MEDIA_INFO_LOG("ThumbnailDoubleUpgradeConfigManager");
    LoadInfoFromSp();
    accountType_ = GetOsAccountType();
    updateStorageSizeThreshold_ = GetDoubleUpgradeThresholdByStorageSize();
}

ThumbnailDoubleUpgradeConfigManager::~ThumbnailDoubleUpgradeConfigManager()
{
    MEDIA_INFO_LOG("~ThumbnailDoubleUpgradeConfigManager");
    UnregisterParameterWatcher();
    StopCallbackTimer();
}

void ThumbnailDoubleUpgradeConfigManager::Init()
{
    MEDIA_INFO_LOG("Init");
    if (IsCompleted()) {
        DataReport();
        MEDIA_INFO_LOG("Init, double upgrade is completed, userType:%{public}d", accountType_);
        return;
    }

    const auto currentTime = MediaFileUtils::UTCTimeMilliSeconds();
    if (IsTimeout(currentTime)) {
        MEDIA_INFO_LOG("Init, double upgrade is timeout, userType:%{public}d", accountType_);
        SaveCompleteInfoToSp(currentTime);
        DataReport();
        UnregisterParameterWatcher();
        StopCallbackTimer();
        return;
    }

    HandleProcess(currentTime, false);

    RegisterParameterWatcher();
}

void ThumbnailDoubleUpgradeConfigManager::GetDoubleUpgradeFlag(std::string& doubleUpgradeFlag)
{
    if (IsMainUser()) {
        doubleUpgradeFlag = OHOS::system::GetParameter(MAIN_SPACE_DOUBLE_UPGRADE_FLAG, "");
    } else if (IsPrivateUser()) {
        doubleUpgradeFlag = OHOS::system::GetParameter(PRIVATE_SPACE_DOUBLE_UPGRADE_FLAG, "");
    } else {
        doubleUpgradeFlag = "";
    }
}

void ThumbnailDoubleUpgradeConfigManager::DataReport()
{
    if (isDataReport_) {
        MEDIA_INFO_LOG("DataReport, reported do nothing");
        return;
    }

    const int64_t updateDuration = GetUpdateDuration();
    const std::string uidType = IsMainUser() ? "main_hmos" : (IsPrivateUser() ? "private_hmos" : "");
    MediaLibraryAstcStat::GetInstance().SetDoubleUpgradeInfo(updateDuration, uidType);

    LcdAndAstcCount count = {};
    count.localAstcCount = DfxDatabaseUtils::QueryASTCThumb(true);
    count.cloudAstcCount = DfxDatabaseUtils::QueryASTCThumb(false);
    count.localLcdCount = DfxDatabaseUtils::QueryLCDThumb(true);
    count.cloudLcdCount = DfxDatabaseUtils::QueryLCDThumb(false);
    DfxReporter().ReportAstcInfo(count);

    MEDIA_INFO_LOG("DataReport, startTime: %{public}" PRId64 ", endTime: %{public}" PRId64
        ", updateDuration: %{public}" PRId64 ", uidType: %{public}s",
        startTimeMs_, endTimeMs_, updateDuration, uidType.c_str());

    SaveDataReportFlagToSp();
}

int32_t ThumbnailDoubleUpgradeConfigManager::GetCurrentSpaceThreshold(bool isCloudSyncOn)
{
    int32_t threshold = 0;
    if (IsInProgress()) {
        threshold = updateStorageSizeThreshold_;
    } else {
        threshold = isCloudSyncOn ? THUMBNAIL_FREE_SIZE_LIMIT_5 : THUMBNAIL_FREE_SIZE_LIMIT_10;
    }
    MEDIA_DEBUG_LOG("GetCurrentSpaceThreshold, threshold: %{public}d, isCloudSyncOn: %{public}d",
        threshold, isCloudSyncOn);
    return threshold;
}

bool ThumbnailDoubleUpgradeConfigManager::IsCompleted() const
{
    return status_ == DoubleUpgradeStatus::COMPLETED;
}

bool ThumbnailDoubleUpgradeConfigManager::IsInProgress() const
{
    return status_ == DoubleUpgradeStatus::IN_PROGRESS;
}

int64_t ThumbnailDoubleUpgradeConfigManager::GetUpdateDuration() const
{
    if (endTimeMs_ > 0 && startTimeMs_ > 0) {
        return std::abs(endTimeMs_ - startTimeMs_);
    }
    return 0;
}

int32_t ThumbnailDoubleUpgradeConfigManager::GetDoubleUpgradeThresholdByStorageSize() const
{
    const int64_t totalSize = MediaFileUtils::GetTotalSize();
    const auto totalGB = static_cast<int32_t>(totalSize / ONE_GB_CAST_BYTE);

    int32_t threshold;
    if (totalGB <= STORAGE_SIZE_256G) {
        threshold = STORAGE_SIZE_LEFT_THRESHOLD_256G;
    } else if (totalGB <= STORAGE_SIZE_512G) {
        threshold = STORAGE_SIZE_LEFT_THRESHOLD_512G;
    } else if (totalGB <= STORAGE_SIZE_1T) {
        threshold = STORAGE_SIZE_LEFT_THRESHOLD_1T;
    } else {
        threshold = STORAGE_SIZE_LEFT_THRESHOLD_1T;
    }
    MEDIA_INFO_LOG("GetDoubleUpgradeThresholdByStorageSize, totalGB:%{public}d, totalSize:%{public}" PRId64
        ", threshold:%{public}d", totalGB, totalSize, threshold);
    return threshold;
}

void ThumbnailDoubleUpgradeConfigManager::OnTimerTimeout()
{
    MEDIA_INFO_LOG("OnTimerTimeout, double upgrade timed out after 12 hours");
    UnregisterParameterWatcher();
    SaveCompleteInfoToSp(MediaFileUtils::UTCTimeMilliSeconds());
    DataReport();
}

void ThumbnailDoubleUpgradeConfigManager::OnParameterChanged(const char *key, const char *value, void *context)
{
    if (key == nullptr || value == nullptr) {
        MEDIA_ERR_LOG("OnParameterChanged, key or value invalid");
        return;
    }

    MEDIA_INFO_LOG("OnParameterChanged, key:%{public}s, value:%{public}s", key, value);
    const std::string keyStr = key;
    const std::string valueStr = value;
    if (keyStr == MAIN_SPACE_DOUBLE_UPGRADE_FLAG || keyStr == PRIVATE_SPACE_DOUBLE_UPGRADE_FLAG) {
        HandleProcess(MediaFileUtils::UTCTimeMilliSeconds(), true, valueStr);
    }
}

void ThumbnailDoubleUpgradeConfigManager::OnDoubleUpgradeParameterChanged(const char *key,
    const char *value, void *context)
{
    ThumbnailDoubleUpgradeConfigManager::GetInstance().OnParameterChanged(key, value, context);
}

void ThumbnailDoubleUpgradeConfigManager::RegisterParameterWatcher()
{
    MEDIA_INFO_LOG("RegisterParameterWatcher");
    if (IsCompleted()) {
        MEDIA_INFO_LOG("RegisterParameterWatcher, double upgrade single is completed, no need to watcher parameter");
        return;
    }

    if (IsMainUser()) {
        int32_t ret = WatchParameter(MAIN_SPACE_DOUBLE_UPGRADE_FLAG, OnDoubleUpgradeParameterChanged, nullptr);
        CHECK_AND_RETURN_LOG(ret == 0, "RegisterParameterWatcher, failed to register watch for "
            "MAIN_SPACE_DOUBLE_UPGRADE_FLAG, ret:%{public}d", ret);
        MEDIA_INFO_LOG("RegisterParameterWatcher, successfully registered watch for MAIN_SPACE_DOUBLE_UPGRADE_FLAG");
        return;
    }

    if (IsPrivateUser()) {
        int32_t ret = WatchParameter(PRIVATE_SPACE_DOUBLE_UPGRADE_FLAG, OnDoubleUpgradeParameterChanged, nullptr);
        CHECK_AND_RETURN_LOG(ret == 0, "RegisterParameterWatcher, failed to register watch for "
            "PRIVATE_SPACE_DOUBLE_UPGRADE_FLAG, ret:%{public}d", ret);
        MEDIA_INFO_LOG("RegisterParameterWatcher, successfully registered watch for PRIVATE_SPACE_DOUBLE_UPGRADE_FLAG");
    }
}

void ThumbnailDoubleUpgradeConfigManager::UnregisterParameterWatcher()
{
    MEDIA_INFO_LOG("UnregisterParameterWatcher");
    if (IsMainUser()) {
        MEDIA_INFO_LOG("UnregisterParameterWatcher mainUser");
        int32_t ret = WatchParameter(MAIN_SPACE_DOUBLE_UPGRADE_FLAG, nullptr, nullptr);
        CHECK_AND_RETURN_LOG(ret == 0, "UnregisterParameterWatcher, failed to unregister watch for "
            "MAIN_SPACE_DOUBLE_UPGRADE_FLAG, ret:%{public}d", ret);
        MEDIA_INFO_LOG("UnregisterParameterWatcher, successfully unregister watch for MAIN_SPACE_DOUBLE_UPGRADE_FLAG");
        return;
    }

    if (IsPrivateUser()) {
        MEDIA_INFO_LOG("UnregisterParameterWatcher privateUser");
        int32_t ret = WatchParameter(PRIVATE_SPACE_DOUBLE_UPGRADE_FLAG, nullptr, nullptr);
        CHECK_AND_RETURN_LOG(ret == 0, "UnregisterParameterWatcher, failed to unregister watch for "
            "PRIVATE_SPACE_DOUBLE_UPGRADE_FLAG, ret:%{public}d", ret);
        MEDIA_INFO_LOG("UnregisterParameterWatcher, successfully unregister watch for "
            "PRIVATE_SPACE_DOUBLE_UPGRADE_FLAG");
    }
}

void ThumbnailDoubleUpgradeConfigManager::AsyncUnregisterParameterWatcher()
{
    std::thread([this]() {
        MEDIA_INFO_LOG("AsyncUnregisterParameterWatcher, start");
        this->UnregisterParameterWatcher();
        MEDIA_INFO_LOG("AsyncUnregisterParameterWatcher, end");
    }).detach();
}

void ThumbnailDoubleUpgradeConfigManager::SaveDataReportFlagToSp()
{
    int32_t errCode = 0;
    auto preferences = OHOS::NativePreferences::PreferencesHelper::GetPreferences(DOUBLE_UPGRADE_SP_FILE, errCode);
    if (preferences == nullptr || errCode != 0) {
        MEDIA_ERR_LOG("SaveDataReportFlagToSp, failed to get preferences, errCode:%{public}d", errCode);
        return;
    }
  
    int32_t ret = preferences->PutBool(KEY_DOUBLE_UPGRADE_DATA_REPORT_FLAG, true);
    CHECK_AND_RETURN_LOG(ret == NativePreferences::E_OK, "SaveDataReportFlagToSp, failed to put data report flag, "
        "ret=%{public}d", ret);
    
    ret = preferences->FlushSync();
    CHECK_AND_RETURN_LOG(ret == NativePreferences::E_OK, "SaveDataReportFlagToSp, failed to flush preferences, "
        "ret=%{public}d", ret);

    isDataReport_ = true;
    MEDIA_INFO_LOG("SaveDataReportFlagToSp, KEY_DOUBLE_UPGRADE_DATA_REPORT_FLAG:true");
}

void ThumbnailDoubleUpgradeConfigManager::LoadInfoFromSp()
{
    int32_t errCode = 0;
    auto preferences = OHOS::NativePreferences::PreferencesHelper::GetPreferences(DOUBLE_UPGRADE_SP_FILE, errCode);
    if (preferences == nullptr || errCode != 0) {
        MEDIA_ERR_LOG("LoadInfoFromSp, failed to get preferences, errCode:%{public}d", errCode);
        return;
    }

    status_ = static_cast<DoubleUpgradeStatus>(preferences->GetInt(KEY_DOUBLE_UPGRADE_STATUS,
        static_cast<int32_t>(DoubleUpgradeStatus::NOT_STARTED)));
    startTimeMs_ = preferences->GetLong(KEY_DOUBLE_UPGRADE_START_TIME, 0);
    endTimeMs_ = preferences->GetLong(KEY_DOUBLE_UPGRADE_END_TIME, 0);
    isDataReport_ = preferences->GetBool(KEY_DOUBLE_UPGRADE_DATA_REPORT_FLAG, false);
    MEDIA_INFO_LOG("LoadInfoFromSp, status:%{public}d, startTime:%{public}" PRId64 ", endTime:%{public}" PRId64
        ", isDataReport:%{public}d", static_cast<int32_t>(status_), startTimeMs_, endTimeMs_, isDataReport_);
}

AccountSA::OsAccountType ThumbnailDoubleUpgradeConfigManager::GetOsAccountType() const
{
    AccountSA::OsAccountType accountType = AccountSA::OsAccountType::END;
    ErrCode ret = AccountSA::OsAccountManager::GetOsAccountTypeFromProcess(accountType);
    if (ret != ERR_OK) {
        MEDIA_ERR_LOG("GetOsAccountType, get osAccountType failed, ret: %{public}d", ret);
    }
    MEDIA_INFO_LOG("GetOsAccountType, accountType:%{public}d", static_cast<int32_t>(accountType));
    return accountType;
}

bool ThumbnailDoubleUpgradeConfigManager::IsMainUser() const
{
    return accountType_ == AccountSA::OsAccountType::ADMIN;
}

bool ThumbnailDoubleUpgradeConfigManager::IsPrivateUser() const
{
    return accountType_ == AccountSA::OsAccountType::PRIVATE;
}

void ThumbnailDoubleUpgradeConfigManager::StartCallbackTimer(int64_t currentTime)
{
    if (startTimeMs_ <= 0) {
        MEDIA_INFO_LOG("StartCallbackTimer, invalid time, startTimeMs:%{public}" PRId64
            ", currentTime:%{public}" PRId64, startTimeMs_, currentTime);
        return;
    }
    if (IsTimeout(currentTime)) {
        OnTimerTimeout();
        return;
    }

    if (callbackTimerId_ != 0) {
        MEDIA_INFO_LOG("StartCallbackTimer, Callback timer already running, unregister and restart");
        callbackTimer_.Unregister(callbackTimerId_);
        callbackTimer_.Shutdown();
        callbackTimerId_ = 0;
    }
    uint32_t ret = callbackTimer_.Setup();
    CHECK_AND_RETURN_LOG(ret == Utils::TIMER_ERR_OK, "StartCallbackTimer, Callback timer setup failed,"
        "ret: %{public}u", ret);

    Utils::Timer::TimerCallback callback = [this]() {
        this->OnTimerTimeout();
    };
    const auto interval = static_cast<uint32_t>(
        std::abs(DOUBLE_UPGRADE_TIMEOUT_MS - std::abs(currentTime - startTimeMs_)));
    MEDIA_INFO_LOG("StartCallbackTimer, interval:%{public}u, startTime:%{public}" PRId64
        ", currentTime:%{public}" PRId64, interval, startTimeMs_, currentTime);
    callbackTimerId_ = callbackTimer_.Register(callback, interval, true);
    if (callbackTimerId_ == 0) {
        MEDIA_ERR_LOG("StartCallbackTimer, Callback timer register failed");
        callbackTimer_.Shutdown();
    }
    MEDIA_INFO_LOG("StartCallbackTimer, success callbackTimerId:%{public}u", callbackTimerId_);
}

void ThumbnailDoubleUpgradeConfigManager::StopCallbackTimer()
{
    MEDIA_INFO_LOG("StopCallbackTimer");
    CHECK_AND_RETURN_LOG(callbackTimerId_ != 0, "callbackTimerId_ is 0");
    callbackTimer_.Unregister(callbackTimerId_);
    callbackTimer_.Shutdown();
    callbackTimerId_ = 0;
    MEDIA_INFO_LOG("StopCallbackTimer success");
}

void ThumbnailDoubleUpgradeConfigManager::HandleProcess(int64_t currentTime, bool isNewFlag,
    const std::string& doubleUpgradeFlag)
{
    std::string updateFlag;
    if (isNewFlag) {
        updateFlag = doubleUpgradeFlag;
    } else {
        GetDoubleUpgradeFlag(updateFlag);
    }
    MEDIA_INFO_LOG("HandleProcess, updateFlag:%{public}s, isNewFlag:%{public}d", updateFlag.c_str(), isNewFlag);

    if (updateFlag == DOUBLE_UPGRADE_IN_PROGRESS) {
        SaveInProcessInfoToSp(currentTime);
        StartCallbackTimer(currentTime);
        MEDIA_INFO_LOG("HandleProcess, double upgrade started, startTime:%{public}" PRId64
            ", currentTime:%{public}" PRId64, startTimeMs_, currentTime);
    } else if (updateFlag == DOUBLE_UPGRADE_COMPLETED) {
        SaveCompleteInfoToSp(currentTime);
        DataReport();
        AsyncUnregisterParameterWatcher();
        StopCallbackTimer();
    } else {
        MEDIA_INFO_LOG("HandleProcess, invalid value, do nothing");
    }
}

 void ThumbnailDoubleUpgradeConfigManager::SaveInProcessInfoToSp(int64_t currentTime)
 {
    int32_t errCode = 0;
    auto preferences = OHOS::NativePreferences::PreferencesHelper::GetPreferences(DOUBLE_UPGRADE_SP_FILE, errCode);
    if (preferences == nullptr || errCode != 0) {
        MEDIA_ERR_LOG("SaveInProcessInfoToSp, failed to get preferences, errCode:%{public}d", errCode);
        return;
    }
  
    int32_t ret = preferences->PutInt(KEY_DOUBLE_UPGRADE_STATUS,
        static_cast<int32_t>(DoubleUpgradeStatus::IN_PROGRESS));
    CHECK_AND_RETURN_LOG(ret == NativePreferences::E_OK, "SaveInProcessInfoToSp, failed to put status, "
        "ret=%{public}d", ret);

    if (startTimeMs_ <= 0) {
        ret = preferences->PutLong(KEY_DOUBLE_UPGRADE_START_TIME, currentTime);
        CHECK_AND_RETURN_LOG(ret == NativePreferences::E_OK, "SaveInProcessInfoToSp, failed to put start time, "
            "ret=%{public}d", ret);
    }
    
    ret = preferences->FlushSync();
    CHECK_AND_RETURN_LOG(ret == NativePreferences::E_OK, "SaveInProcessInfoToSp, failed to flush preferences, "
        "ret=%{public}d", ret);

    status_ = DoubleUpgradeStatus::IN_PROGRESS;
    if (startTimeMs_ <= 0) {
        startTimeMs_ = currentTime;
    }
    MEDIA_INFO_LOG("SaveInProcessInfoToSp, status:%{public}d, startTime:%{public}" PRId64
        ", currentTime:%{public}" PRId64, static_cast<int32_t>(status_), startTimeMs_, currentTime);
}

void ThumbnailDoubleUpgradeConfigManager::SaveCompleteInfoToSp(int64_t currentTime)
{
    int32_t errCode = 0;
    auto preferences = OHOS::NativePreferences::PreferencesHelper::GetPreferences(DOUBLE_UPGRADE_SP_FILE, errCode);
    if (preferences == nullptr || errCode != 0) {
        MEDIA_ERR_LOG("SaveCompleteInfoToSp, failed to get preferences, errCode:%{public}d", errCode);
        return;
    }

    int32_t ret = preferences->PutInt(KEY_DOUBLE_UPGRADE_STATUS, static_cast<int32_t>(DoubleUpgradeStatus::COMPLETED));
    CHECK_AND_RETURN_LOG(ret == NativePreferences::E_OK, "SaveCompleteInfoToSp, failed to put status, "
        "ret=%{public}d", ret);

    if (endTimeMs_ <= 0) {
        ret = preferences->PutLong(KEY_DOUBLE_UPGRADE_END_TIME, currentTime);
        CHECK_AND_RETURN_LOG(ret == NativePreferences::E_OK, "SaveCompleteInfoToSp, failed to put end time, "
            "ret=%{public}d", ret);
    }

    ret = preferences->FlushSync();
    CHECK_AND_RETURN_LOG(ret == NativePreferences::E_OK, "SaveCompleteInfoToSp, failed to flush preferences, "
        "ret=%{public}d", ret);

    status_ = DoubleUpgradeStatus::COMPLETED;
    if (endTimeMs_ <= 0) {
        endTimeMs_ = currentTime;
    }
    MEDIA_INFO_LOG("SaveCompleteInfoToSp, status:%{public}d, endTime:%{public}" PRId64
        ", currentTime:%{public}" PRId64, static_cast<int32_t>(status_), endTimeMs_, currentTime);
}

bool ThumbnailDoubleUpgradeConfigManager::IsTimeout(int64_t currentTime)
{
    if (startTimeMs_ <= 0) {
        return false;
    }
    const bool isTimeout = std::abs(currentTime - startTimeMs_) >= DOUBLE_UPGRADE_TIMEOUT_MS;
    MEDIA_INFO_LOG("IsTimeout, isTimeout:%{public}d, startTime:%{public}" PRId64
        ", currentTime:%{public}" PRId64, isTimeout, startTimeMs_, currentTime);
    return isTimeout;
}
} // namespace Media
} // namespace OHOS
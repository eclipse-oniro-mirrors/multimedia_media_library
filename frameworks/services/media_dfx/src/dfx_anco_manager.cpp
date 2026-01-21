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
#define MLOG_TAG "AncoDfxManager"

#include "dfx_anco_manager.h"

#include "dfx_reporter.h"
#include "dfx_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "dfx_database_utils.h"
#include "parameters.h"
#include "preferences.h"
#include "preferences_helper.h"

using namespace std;

namespace OHOS {
namespace Media {

AncoDfxManager::~AncoDfxManager()
{
    ShutDownTimer();
}

AncoDfxManager& AncoDfxManager::GetInstance()
{
    static AncoDfxManager ancoDfxManager;
    return ancoDfxManager;
}

AncoDfxManager::AncoDfxManager()
{
}

void AncoDfxManager::RunDfx()
{
    if (alreadyRunDfx_) {
        return;
    }
    MEDIA_INFO_LOG("AncoDfxManager RunDfx");
    SetStartTime();
    StartTimer();
    alreadyRunDfx_ = true;
}

void AncoDfxManager::ReportFirstLoadInfo(uint64_t loadStartTime, uint64_t loadEndTime)
{
    MEDIA_INFO_LOG("anco ReportFirstLoadInfo");
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    bool alreadyFirstLoad = prefs->GetBool(ANCO_FIRST_LOAD_FLAG, false);
    if (alreadyFirstLoad) {
        MEDIA_INFO_LOG("anco already first load report");
        return;
    }
    InnerReportAncoCountFormatInfo(loadStartTime, loadEndTime, true);
    prefs->PutBool(ANCO_FIRST_LOAD_FLAG, true);
    prefs->FlushSync();
    MEDIA_INFO_LOG("anco first load report");
}

void AncoDfxManager::ReportAncoCheckInfo(const AncoCheckInfo& reportData)
{
    DfxReporter::ReportAncoCheckInfo(reportData);
}

void AncoDfxManager::ReportAncoOperationChangeInfo(const AncoOperationChangeInfo& reportData)
{
    DfxReporter::ReportAncoOperationChangeInfo(reportData);
}

void AncoDfxManager::NotifyOperationChange(const int32_t objType, const int32_t optType)
{
    MEDIA_INFO_LOG("anco NotifyOperationChange objType: %{public}d, optType: %{public}d",
        objType, optType);
    std::unique_lock<std::mutex> lock(ancoOptChangeInfoMutex_);
    if (objType == NOTIFY_FILE_TYPE) {
        if (optType == NOTIFY_ADD_OPT_TYPE) {
            ancoOptChangeInfo_.photoOptAddCount += 1;
        } else if (optType == NOTIFY_MOD_OPT_TYPE) {
            ancoOptChangeInfo_.photoOptUpdateCount += 1;
        } else if (optType == NOTIFY_DEL_OPT_TYPE) {
            ancoOptChangeInfo_.photoOptDeleteCount += 1;
        }
    } else if (objType == NOTIFY_DIR_TYPE) {
        if (optType == NOTIFY_ADD_OPT_TYPE) {
            ancoOptChangeInfo_.albumOptAddCount += 1;
        } else if (optType == NOTIFY_MOD_OPT_TYPE) {
            ancoOptChangeInfo_.albumOptUpdateCount += 1;
        } else if (optType == NOTIFY_DEL_OPT_TYPE) {
            ancoOptChangeInfo_.albumOptDeleteCount += 1;
        }
    }
    ancoOptChangeInfo_.totalOptCount += 1;
}

void AncoDfxManager::InnerReportAncoCountFormatInfo(uint64_t loadStartTime, uint64_t loadEndTime, bool firstLoad)
{
    MEDIA_INFO_LOG("anco InnerReportAncoCountFormatInfo");
    AncoCountFormatInfo reportData;
    reportData.loadStartTime = loadStartTime;
    reportData.loadEndTime = loadEndTime;
    int32_t queryRet = DfxDatabaseUtils::QueryAncoPhotosFormatAndCount(reportData);
    if (queryRet != E_OK) {
        MEDIA_ERR_LOG("QueryAncoPhotosFormatAndCount error: %{public}d", queryRet);
        return;
    }
    int32_t dfxRet = DfxReporter::ReportAncoCountFormatInfo(reportData, firstLoad);
    if (dfxRet != E_OK) {
        MEDIA_ERR_LOG("DfxReporter ReportAncoCountFormatInfo error: %{public}d", dfxRet);
        return;
    }
    // 刷新打点时间
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    prefs->PutLong(ANCO_FORMAT_COUNT_LAST_REPORT_TIME, MediaFileUtils::UTCTimeSeconds());
    prefs->FlushSync();
}

void AncoDfxManager::InnerReportAndResetOptChangeInfo()
{
    MEDIA_INFO_LOG("anco InnerReportAndResetOptChangeInfo");
    std::unique_lock<std::mutex> lock(ancoOptChangeInfoMutex_);
    int32_t dfxRet = DfxReporter::ReportAncoOperationChangeInfo(ancoOptChangeInfo_);
    if (dfxRet != E_OK) {
        MEDIA_ERR_LOG("DfxReporter InnerReportAndResetOptChangeInfo error: %{public}d", dfxRet);
        return;
    }
    // 刷新打点时间
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    prefs->PutLong(ANCO_OPT_CHANGE_LAST_REPORT_TIME, MediaFileUtils::UTCTimeSeconds());
    prefs->FlushSync();
    ancoOptChangeInfo_.photoOptAddCount = 0;
    ancoOptChangeInfo_.photoOptUpdateCount = 0;
    ancoOptChangeInfo_.photoOptDeleteCount = 0;
    ancoOptChangeInfo_.albumOptAddCount = 0;
    ancoOptChangeInfo_.albumOptUpdateCount = 0;
    ancoOptChangeInfo_.albumOptDeleteCount = 0;
    ancoOptChangeInfo_.totalOptCount = 0;
}

void AncoDfxManager::RegisterFormatCountTimer()
{
    MEDIA_INFO_LOG("anco RegisterFormatCountTimer");
    Utils::Timer::TimerCallback ancoCountFormatTimerCallback = [this]() {
        (void)this;
        MEDIA_INFO_LOG("Timer anco statics resource format and count");
        InnerReportAncoCountFormatInfo();
    };
    timerId_ = timer_.Register(ancoCountFormatTimerCallback, REPORT_FORMAT_COUNT_INFO_INTERVAL, false);
    alreadyRegisterFormatCountTimer_ = true;
}

void AncoDfxManager::StartFormatCountTimer()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    // 查询上次上报时间
    int64_t lastReportTime = prefs->GetLong(ANCO_FORMAT_COUNT_LAST_REPORT_TIME, 0);
    // 如果为0，说明第一次上报，
    if (lastReportTime == 0) {
        // 直接第一次上报
        InnerReportAncoCountFormatInfo();
        // 注册6小时周期上报
        RegisterFormatCountTimer();
        return;
    }
    int64_t reportTimeInterval = (MediaFileUtils::UTCTimeSeconds() - lastReportTime);
    // 或者上一次上报时间不为0，且距离当前时间超过6小时，例如，中间出现关机的情况
    if ((MediaFileUtils::UTCTimeSeconds() - lastReportTime) >= REPORT_FORMAT_COUNT_INFO_INTERVAL) {
        // 直接上报一次
        InnerReportAncoCountFormatInfo();
        // 注册6小时周期上报
        RegisterFormatCountTimer();
        return;
    }
    std::thread([this, reportTimeInterval]() {
        this_thread::sleep_for(chrono::seconds(reportTimeInterval));
        // 上报一次
        InnerReportAncoCountFormatInfo();
        // 注册6小时周期上报
        RegisterFormatCountTimer();
    }).detach();
}

void AncoDfxManager::RegisterOptChangeInfoTimer()
{
    MEDIA_INFO_LOG("anco RegisterOptChangeInfoTimer");
    Utils::Timer::TimerCallback ancoOptChangeInfoTimerCallback = [this]() {
        (void)this;
        MEDIA_INFO_LOG("Timer anco statics opt change info");
        InnerReportAndResetOptChangeInfo();
    };
    timerId_ = timer_.Register(ancoOptChangeInfoTimerCallback, REPORT_OPT_CHANGE_INFO_INTERVAL, false);
    alreadyRegisterOptChangeInfoTimer_ = true;
}

void AncoDfxManager::StartOptChangeInfoTimer()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    // 查询上次上报时间
    int64_t lastReportTime = prefs->GetLong(ANCO_OPT_CHANGE_LAST_REPORT_TIME, 0);
    // 如果为0，说明第一次上报，
    if (lastReportTime == 0) {
        // 直接第一次上报
        InnerReportAndResetOptChangeInfo();
        // 注册6小时周期上报
        RegisterOptChangeInfoTimer();
        return;
    }
    int64_t reportTimeInterval = (MediaFileUtils::UTCTimeSeconds() - lastReportTime);
    // 或者上一次上报时间不为0，且距离当前时间超过6小时，例如，中间出现关机的情况
    if ((MediaFileUtils::UTCTimeSeconds() - lastReportTime) >= REPORT_OPT_CHANGE_INFO_INTERVAL) {
        // 直接上报一次
        InnerReportAndResetOptChangeInfo();
        // 注册6小时周期上报
        RegisterOptChangeInfoTimer();
        return;
    }
    std::thread([this, reportTimeInterval]() {
        this_thread::sleep_for(chrono::seconds(reportTimeInterval));
        // 上报一次
        InnerReportAndResetOptChangeInfo();
        // 注册6小时周期上报
        RegisterOptChangeInfoTimer();
    }).detach();
}

void AncoDfxManager::StartTimer()
{
    MEDIA_INFO_LOG("AncoDfxManager StartTimer start");
    std::unique_lock<std::mutex> lock(timerMutex_);
    if (timerId_ != 0) {
        return;
    }
    if (timer_.Setup() != ERR_OK) {
        MEDIA_INFO_LOG("anco Dfx Set Timer Failed");
        return;
    }
    StartFormatCountTimer();
    StartOptChangeInfoTimer();
    MEDIA_INFO_LOG("AncoDfxManager StartTimer id:%{public}d end", timerId_);
}

void AncoDfxManager::ShutDownTimer()
{
    MEDIA_INFO_LOG("AncoDfxManager ShutDownTimer start");
    std::unique_lock<std::mutex> lock(timerMutex_);
    if (timerId_ == 0) {
        return;
    }
    MEDIA_INFO_LOG("AncoDfxManager ShutDownTimer id:%{public}d", timerId_);
    timer_.Unregister(timerId_);
    timerId_ = 0;
    timer_.Shutdown();
}

void AncoDfxManager::ResetStartTime()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    prefs->PutLong(ANCO_DFX_START_TIME, 0);
    prefs->FlushSync();
}

void AncoDfxManager::SetStartTime()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    int64_t time = prefs->GetLong(ANCO_DFX_START_TIME, 0);
    // if startTime exists, no need to reset startTime
    if (time != 0) {
        return;
    }
    time = MediaFileUtils::UTCTimeSeconds();
    prefs->PutLong(ANCO_DFX_START_TIME, time);
    prefs->FlushSync();
}

} // namespace Media
} // namespace OHOS

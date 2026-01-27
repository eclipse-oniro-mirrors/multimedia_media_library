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

#define MLOG_TAG "MediaLakeCloneEventManager"

#include "media_lake_clone_event_manager.h"

#include "common_event_support.h"
#include "global_scanner.h"
#include "iservice_registry.h"
#include "media_file_change_manager.h"
#include "media_lake_check_manager.h"
#include "media_log.h"
#include "media_thread.h"
#include "parameters.h"

namespace OHOS::Media {
const int32_t BACKUP_SA_ID = 5203;
const int32_t MIN_TIME_OUT = 4;
const std::string GLOBAL_SCAN_ROOT_DIR = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC";
const std::unordered_map<std::string, uint8_t> BUNDLE_NAME_BIT_MAP = {
    { "com.ohos.medialibrary.medialibrarydata", 0 },
    { "com.huawei.hmos.filemanager", 1 },
};

bool MediaLakeCloneEventManager::IsRestoreEvent(const AAFwk::Want &want)
{
    return IsSubscribedAction(want) && IsSubscribedBundle(want);
}

bool MediaLakeCloneEventManager::IsRestoring()
{
    std::lock_guard<std::mutex> lock(statusMutex_);
    return currentRestoreStatusBitMap_.load() > 0;
}

void MediaLakeCloneEventManager::HandleRestoreEvent(const AAFwk::Want &want)
{
    std::string action = want.GetAction();
    SetDeathRecipient();
    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_RESTORE_START) {
        HandleRestoreStartEvent(want);
    } else {
        HandleRestoreEndEvent(want);
    }
}

bool MediaLakeCloneEventManager::IsSubscribedAction(const AAFwk::Want &want)
{
    std::string action = want.GetAction();
    return action == EventFwk::CommonEventSupport::COMMON_EVENT_RESTORE_START ||
        action == EventFwk::CommonEventSupport::COMMON_EVENT_RESTORE_END;
}

bool MediaLakeCloneEventManager::IsSubscribedBundle(const AAFwk::Want &want)
{
    std::string bundleName = GetEventBundleName(want);
    return BUNDLE_NAME_BIT_MAP.count(bundleName) > 0;
}

std::string MediaLakeCloneEventManager::GetEventBundleName(const AAFwk::Want &want)
{
    return want.GetStringParam("bundleName");
}

void MediaLakeCloneEventManager::HandleRestoreStartEvent(const AAFwk::Want &want)
{
    std::string bundleName = GetEventBundleName(want);
    MEDIA_INFO_LOG("LakeClone: Get COMMON_EVENT_RESTORE_START of %{public}s", bundleName.c_str());
    SetRestoreStatusBitMapForStart(bundleName);
    CHECK_AND_RETURN(ShouldUnregisterLakeFileMonitor());
    UnregisterLakeFileMonitor();
}

void MediaLakeCloneEventManager::HandleRestoreEndEvent(const AAFwk::Want &want)
{
    std::string bundleName = GetEventBundleName(want);
    MEDIA_INFO_LOG("LakeClone: Get COMMON_EVENT_RESTORE_END of %{public}s", bundleName.c_str());
    SetRestoreStatusBitMapForEnd(bundleName);
    CHECK_AND_RETURN(ShouldRegisterLakeFileMonitor());
    RegisterLakeFileMonitor();
}

bool MediaLakeCloneEventManager::GetBitByBundleName(const std::string &bundleName, uint8_t &bit)
{
    auto iter = BUNDLE_NAME_BIT_MAP.find(bundleName);
    CHECK_AND_RETURN_RET(iter != BUNDLE_NAME_BIT_MAP.end(), false);
    bit = iter->second;
    return true;
}

void MediaLakeCloneEventManager::CheckIsExecuteGlobalScan(uint8_t bit)
{
    // 默认触发全盘扫描，若选择媒体库，则不触发全盘扫描
    CHECK_AND_RETURN(bit == 0);
    isExecuteGlobalScan_.store(false);
}

void MediaLakeCloneEventManager::SetRestoreStatusBitMapForStart(const std::string &bundleName)
{
    std::lock_guard<std::mutex> lock(statusMutex_);
    uint8_t bit = 0;
    CHECK_AND_RETURN(GetBitByBundleName(bundleName, bit));
    initRestoreStatusBitMap_ = currentRestoreStatusBitMap_.load();
    currentRestoreStatusBitMap_.fetch_or(1 << bit);
    CheckIsExecuteGlobalScan(bit);
    MEDIA_INFO_LOG("LakeClone: Set restoreStatusBitMap init: %{public}d, current: %{public}d",
        initRestoreStatusBitMap_.load(), currentRestoreStatusBitMap_.load());
}

void MediaLakeCloneEventManager::SetRestoreStatusBitMapForEnd(const std::string &bundleName)
{
    std::lock_guard<std::mutex> lock(statusMutex_);
    uint8_t bit = 0;
    CHECK_AND_RETURN(GetBitByBundleName(bundleName, bit));
    initRestoreStatusBitMap_ = currentRestoreStatusBitMap_.load();
    currentRestoreStatusBitMap_.fetch_and(~(1 << bit));
    MEDIA_INFO_LOG("LakeClone: Set restoreStatusBitMap init: %{public}d, current: %{public}d",
        initRestoreStatusBitMap_.load(), currentRestoreStatusBitMap_.load());
}

void MediaLakeCloneEventManager::UnregisterLakeFileMonitor()
{
    MEDIA_INFO_LOG("LakeClone: Start UnregisterLakeFileMonitor");
    MediaFileChangeManager::GetInstance()->StopProcessChangeData();
    MediaInLakeCheckManager::GetInstance()->Stop();
    MEDIA_INFO_LOG("LakeClone: End UnregisterLakeFileMonitor");
}

void MediaLakeCloneEventManager::RegisterLakeFileMonitor()
{
    MEDIA_INFO_LOG("LakeClone: Start RegisterLakeFileMonitor");
    RunGlobalScanner();
    isExecuteGlobalScan_.store(true); // 重置isExecuteGlobalScan_为默认全盘扫描状态
    MediaFileChangeManager::GetInstance()->StartProcessChangeData();
    MEDIA_INFO_LOG("LakeClone: End RegisterLakeFileMonitor");
}

void MediaLakeCloneEventManager::RunGlobalScanner()
{
    CHECK_AND_RETURN(isExecuteGlobalScan_.load());
    ScannerStatus scannerStatus = GlobalScanner::GetInstance().GetScannerStatus();
    CHECK_AND_RETURN_WARN_LOG(scannerStatus == ScannerStatus::IDLE,
        "LakeClone: Scanner is running, status: %{public}d", static_cast<int32_t>(scannerStatus));

    MEDIA_INFO_LOG("LakeClone: Start RunGlobalScanner");
    auto scannerFunc = []() { GlobalScanner::GetInstance().Run(GLOBAL_SCAN_ROOT_DIR, false); };
    Media::thread myThread("LakeClone", scannerFunc);
    if (myThread.is_invalid()) {
        MEDIA_ERR_LOG("LakeClone: Start RunGlobalScanner thread failed.");
    } else {
        myThread.detach();
    }
}

bool MediaLakeCloneEventManager::ShouldUnregisterLakeFileMonitor()
{
    std::lock_guard<std::mutex> lock(statusMutex_);
    return initRestoreStatusBitMap_.load() == 0 && currentRestoreStatusBitMap_.load() != 0;
}

bool MediaLakeCloneEventManager::ShouldRegisterLakeFileMonitor()
{
    std::lock_guard<std::mutex> lock(statusMutex_);
    return initRestoreStatusBitMap_.load() != 0 && currentRestoreStatusBitMap_.load() == 0;
}

void MediaLakeCloneEventManager::SetDeathRecipient()
{
    std::lock_guard<std::mutex> lock(deathRecipientMutex_);
    CHECK_AND_RETURN_LOG(backupSaRemoteObject_ == nullptr, "LakeClone: No need to set death recipient");
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHECK_AND_RETURN_LOG(saManager != nullptr, "Failed to get SystemAbilityManagerClient.");

    backupSaRemoteObject_ = saManager->CheckSystemAbility(BACKUP_SA_ID);
    if (backupSaRemoteObject_ == nullptr) {
        MEDIA_INFO_LOG("LakeClone: Try to load Backup SystemAbility");
        backupSaRemoteObject_ = saManager->LoadSystemAbility(BACKUP_SA_ID, MIN_TIME_OUT);
        CHECK_AND_RETURN_LOG(backupSaRemoteObject_ != nullptr, "LakeClone: backupSaRemoteObject_ is null.");
    }

    CHECK_AND_PRINT_LOG(backupSaRemoteObject_->AddDeathRecipient(sptr(new MediaLakeCloneDeathRecipient())),
        "LakeClone: Failed to add death recipient.");
}

void MediaLakeCloneEventManager::HandleDeathRecipient()
{
    {
        std::lock_guard<std::mutex> lock(deathRecipientMutex_);
        backupSaRemoteObject_ = nullptr;
    }
    {
        std::lock_guard<std::mutex> lock(statusMutex_);
        CHECK_AND_RETURN_INFO_LOG(currentRestoreStatusBitMap_.load() != 0,
            "LakeClone: No need to handle death recipient");
        MEDIA_INFO_LOG("LakeClone: Start handle death recipient, currentRestoreStatusBitMap_: %{public}d",
            currentRestoreStatusBitMap_.load());
    }
    RegisterLakeFileMonitor();
    ResetRestoreStatusBitMap();
    MEDIA_INFO_LOG("LakeClone: End handle death recipient");
}

void MediaLakeCloneEventManager::ResetRestoreStatusBitMap()
{
    std::lock_guard<std::mutex> lock(statusMutex_);
    initRestoreStatusBitMap_.store(0);
    currentRestoreStatusBitMap_.store(0);
    isExecuteGlobalScan_.store(true); // 重置isExecuteGlobalScan_为默认全盘扫描状态
}

void MediaLakeCloneDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    MEDIA_INFO_LOG("LakeClone: OnRemoteDied");
    CHECK_AND_RETURN_LOG(object != nullptr, "remote object is nullptr");
    sptr<IRemoteObject> objectPtr = object.promote();
    CHECK_AND_RETURN_LOG(objectPtr != nullptr, "remote object sptr is nullptr");
    objectPtr->RemoveDeathRecipient(this);
    MediaLakeCloneEventManager::GetInstance().HandleDeathRecipient();
}
}

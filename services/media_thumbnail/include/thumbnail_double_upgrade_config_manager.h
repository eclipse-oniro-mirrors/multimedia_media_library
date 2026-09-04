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

#ifndef FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_DOUBLE_UPGRADE_CONFIG_MANAGER_H
#define FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_DOUBLE_UPGRADE_CONFIG_MANAGER_H

#include <string>
#include <cstdint>
#include <timer.h>

#include "os_account_info.h"

#include "thumbnail_const.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

class ThumbnailDoubleUpgradeConfigManager {
public:
    EXPORT static void OnDoubleUpgradeParameterChanged(const char *key, const char *value, void *context);
    EXPORT void Init();
    EXPORT int32_t GetCurrentSpaceThreshold(bool isCloudSyscOn);
    EXPORT static ThumbnailDoubleUpgradeConfigManager& GetInstance();

private:
    ThumbnailDoubleUpgradeConfigManager();
    ~ThumbnailDoubleUpgradeConfigManager();

    ThumbnailDoubleUpgradeConfigManager(const ThumbnailDoubleUpgradeConfigManager&) = delete;
    ThumbnailDoubleUpgradeConfigManager& operator=(const ThumbnailDoubleUpgradeConfigManager&) = delete;
    ThumbnailDoubleUpgradeConfigManager(ThumbnailDoubleUpgradeConfigManager&&) = delete;
    ThumbnailDoubleUpgradeConfigManager& operator=(ThumbnailDoubleUpgradeConfigManager&&) = delete;

    [[nodiscard]] EXPORT bool IsCompleted() const;
    [[nodiscard]] EXPORT bool IsInProgress() const;
    [[nodiscard]] EXPORT bool IsMainUser() const;
    [[nodiscard]] EXPORT bool IsPrivateUser() const;
    [[nodiscard]] EXPORT int32_t GetDoubleUpgradeThresholdByStorageSize() const;
    [[nodiscard]] EXPORT int64_t GetUpdateDuration() const;
    [[nodiscard]] EXPORT AccountSA::OsAccountType GetOsAccountType() const;

    EXPORT void GetDoubleUpgradeFlag(std::string& doubleUpgradeFlag);
    EXPORT void RegisterParameterWatcher();
    EXPORT void UnregisterParameterWatcher();
    EXPORT void AsyncUnregisterParameterWatcher();
    EXPORT void OnParameterChanged(const char *key, const char *value, void *context);
    EXPORT void LoadInfoFromSp();
    EXPORT void SaveInProcessInfoToSp(int64_t currentTime);
    EXPORT void SaveCompleteInfoToSp(int64_t currentTime);    
    EXPORT void SaveDataReportFlagToSp();
    EXPORT void StartCallbackTimer(int64_t currentTime);    
    EXPORT void StopCallbackTimer();
    EXPORT void OnTimerTimeout();
    EXPORT void DataReport();
    EXPORT void HandleProcess(int64_t currentTime, bool isNewFlag, const std::string& doubleUpgradeFlag = "");
    EXPORT void IsTimeout(int64_t currentTime);    

    enum class DoubleUpgradeStatus : int32_t {
        NOT_STARTED = 0,
        IN_PROGRESS = 1,
        COMPLETED = 2,
    }

    bool isDataReport_{false};
    DoubleUpgradeStatus status_{DoubleUpgradeStatus::NOT_STARTED};
    AccountSA::OsAccountType accountType_ {AccountSA::OsAccountType::END}
    int32_t updateStorageSizeThreshold_{THUMBNAIL_FREE_SIZE_LIMIT_10};
    int64_t startTimeMs_{0};
    int64_t endTimeMs_{0};

    uint32_t callbackTimerId_{0};
    Utils::Timer callbackTimer_{"DoubleUpdateSingleCallback"};
};
} // namespace Media
} // namespace OHOS

#endif // FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_DOUBLE_UPGRADE_CONFIG_MANAGER_H
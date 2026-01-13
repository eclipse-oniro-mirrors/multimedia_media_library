/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License"){return 0;}
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

#ifndef OHOS_CLOUD_MEDIA_RETAIN_SMART_DATA_H
#define OHOS_CLOUD_MEDIA_RETAIN_SMART_DATA_H

#include <cstdint>

#include "cloud_media_operation_code.h"
#include "cloud_media_asset_types.h"
#include "settings_data_manager.h"
#include "cloud_media_sync_mutex.h"

namespace OHOS::Media {

enum class SmartDataProcessingMode {
    NONE = 0,
    RETAIN = 1,
    RECOVER = 2,
};

enum class CleanTaskState : int64_t {
    IDLE = 0,
    CLEANING = 1
};

enum class UpdateSmartDataState : int64_t {
    IDLE = 0,
    UPDATE_SMART_DATA = 1
};

const int64_t REAL_LCD_VISIT_TIME_INVALID = -2;
const int64_t REAL_LCD_VISIT_TIME_DELETED = -3;
constexpr int64_t TIMESTAMP_UP_TO_LAST_RETAIN_OF_HDC = 180LL * 24 * 60 * 60 * 1000;
constexpr int64_t TIMESTAMP_UP_TO_LAST_RETAIN_OF_CLOUD = 30LL * 24 * 60 * 60 * 1000;

int64_t GetSmartDataRetainTime();
SmartDataProcessingMode GetSmartDataProcessingMode(CloudMediaRetainType retainType, SwitchStatus switchStatus);
void SetSouthDeviceNextStatus(CloudMediaRetainType retainType, SwitchStatus switchStatus);
SwitchStatus GetSouthDeviceNextStatus(CloudMediaRetainType retainType);
void SetSmartDataProcessingMode(SmartDataProcessingMode mode);
SmartDataProcessingMode GetSmartDataProcessingMode();
void InitBackupPhotosAlbumTable();
void BackupBackupPhotosAlbumTable();
void DeleteBackupPhotosAlbumForSmartData();
bool IsNeedRecoverSmartData();
int32_t DoCleanPhotosTableCloudData();
int32_t DoCloudMediaRetainCleanup();
int32_t UpdatePhotosLcdVisitTime(const std::vector<std::string> &fileIds);
void SetSmartDataRetainTime();
void UpdateInvalidCloudHighlightInfo();
int32_t DoUpdateSmartDataAlbum();
void SetSmartDataCleanState(CleanTaskState currentState);
int64_t GetSmartDataCleanState();
void SetSmartDataUpdateState(UpdateSmartDataState currentState);
int64_t GetSmartDataUpdateState();
}

#endif
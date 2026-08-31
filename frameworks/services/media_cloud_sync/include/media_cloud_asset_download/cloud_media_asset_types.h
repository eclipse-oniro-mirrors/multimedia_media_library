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

#ifndef OHOS_CLOUD_MEDIA_ASSET_TYPES_H
#define OHOS_CLOUD_MEDIA_ASSET_TYPES_H

#include <iostream>
#include <atomic>
#include <chrono>
#include <string>
#include <map>
// LCOV_EXCL_START
namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
enum class CloudMediaDownloadType : int32_t {
    DOWNLOAD_FORCE = 0,
    DOWNLOAD_GENTLE,
};

enum class CloudMediaAssetTaskStatus : int32_t {
    DOWNLOADING = 0,
    PAUSED,
    IDLE,
};

enum class CloudMediaRetainType : int32_t {
    RETAIN_FORCE = 0, // 删除原文件在云端的本地元数据和缩略图
    HDC_RETAIN_FORCE = 1, // 删除原文件在家庭存储端的本地元数据和缩略图
    SHARE_RETAIN_FORCE = 2 // 删除共享资产在本地的元数据和缩略图
};

enum class CloudMediaTaskPauseCause : int32_t {
    NO_PAUSE = 0,
    TEMPERATURE_LIMIT,
    ROM_LIMIT,
    NETWORK_FLOW_LIMIT,
    WIFI_UNAVAILABLE,
    POWER_LIMIT,
    BACKGROUND_TASK_UNAVAILABLE,
    FREQUENT_USER_REQUESTS,
    CLOUD_ERROR,
    USER_PAUSED,
};

enum class CloudMediaTaskRecoverCause : int32_t {
    FOREGROUND_TEMPERATURE_PROPER = 1,
    STORAGE_NORMAL,
    NETWORK_FLOW_UNLIMIT,
    NETWORK_NORMAL,
    BACKGROUND_TASK_AVAILABLE,
    RETRY_FOR_FREQUENT_REQUESTS,
    RETRY_FOR_CLOUD_ERROR,
};

enum class CloudMediaTaskDownloadCloudAssetCode : int32_t {
    ADD_DOWNLOAD_TASK_SUCC = 0,
    ADD_DOWNLOAD_ASSET_NOT_EXIST = 1,
};
} // namespace Media
} // namespace OHOS
// LCOV_EXCL_STOP
#endif // OHOS_CLOUD_MEDIA_ASSET_TYPES_H
/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_MEDIA_SHARE_ASSETS_UTILS_H
#define OHOS_MEDIA_MEDIA_SHARE_ASSETS_UTILS_H

#include <cstdint>

namespace OHOS::Media {
// 云同步状态枚举，完整定义见 cloud_sync_notify_handler.h
enum CloudSyncStatus : int32_t;

class MediaShareAssetsCloudExitUtils {
public:
    // 记录共享资产清理状态: CLOUD_CLEANING 时写入当前时间戳, 其他状态置 0
    static void SetShareAssetCleanStatus(CloudSyncStatus status);
    // 查询共享资产是否处于清理中(带超时判断)
    static bool IsShareAssetCleaning();
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_MEDIA_SHARE_ASSETS_UTILS_H

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

#ifndef OHOS_MEDIA_CLOUD_SHARE_SYNC_FOUNDATION_SERVICE_H
#define OHOS_MEDIA_CLOUD_SHARE_SYNC_FOUNDATION_SERVICE_H

#include <stdint.h>

#include "media_log.h"

// LCOV_EXCL_START
namespace OHOS::Media {

// 共享资产退出云空间场景下的端云同步基础服务
class CloudShareSyncFoundationService {
public:
    CloudShareSyncFoundationService() {}
    ~CloudShareSyncFoundationService() {}

    // 停止端云同步
    int32_t StopSync();
    // 尝试启动端云同步
    int32_t TryToStartSync();
    int32_t ResetCursor();
};
} // namespace OHOS::Media
// LCOV_EXCL_STOP
#endif // OHOS_MEDIA_CLOUD_SHARE_SYNC_FOUNDATION_SERVICE_H

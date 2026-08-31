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

#define MLOG_TAG "Media_Service"

#include "cloud_share_sync_foundation_service.h"

#include "medialibrary_errno.h"

namespace OHOS::Media {

int32_t CloudShareSyncFoundationService::StopSync()
{
    MEDIA_ERR_LOG("refer to CloudSyncManager::GetInstance().StopSync");
    return E_OK;
}

int32_t CloudShareSyncFoundationService::TryToStartSync()
{
    MEDIA_ERR_LOG("refer to CloudMediaAssetManager::TryToStartSync()");
    return E_OK;
}

int32_t CloudShareSyncFoundationService::ResetCursor()
{
    MEDIA_ERR_LOG("refer to CloudSyncManager::GetInstance().ResetCursor(true)");
    return E_OK;
}
} // namespace OHOS::Media

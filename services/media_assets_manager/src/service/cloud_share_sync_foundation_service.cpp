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
#include "cloud_sync_manager.h"
#include "cloud_sync_utils.h"
#include "medialibrary_db_const.h"

namespace OHOS::Media {

const std::string SHARED_ALBUM_BUNDLE_NAME = "com.ohos.photos.shared";

int32_t CloudShareSyncFoundationService::StopSync()
{
    FileManagement::CloudSync::CloudSyncManager::GetInstance().StopSync(SHARED_ALBUM_BUNDLE_NAME);
    return E_OK;
}

int32_t CloudShareSyncFoundationService::TryToStartSync()
{
    if (!CloudSyncUtils::IsSharedAlbumCloudSyncSwitchOn()) {
        MEDIA_INFO_LOG("syncSwitch is not open");
        return E_OK;
    }
    MEDIA_INFO_LOG("cloud sync manager start share album sync");
    int32_t ret = FileManagement::CloudSync::CloudSyncManager::GetInstance().StartSync(SHARED_ALBUM_BUNDLE_NAME);
    CHECK_AND_PRINT_LOG(ret == E_OK, "cloud sync manager start share album sync err %{public}d", ret);
    MEDIA_INFO_LOG("cloud sync manager end share album sync");
    return E_OK;
}

int32_t CloudShareSyncFoundationService::ResetCursor()
{
    if (CloudSyncUtils::IsSharedAlbumCloudSyncSwitchOn()) {
        MEDIA_INFO_LOG("cloud sync manager start reset share album cursor");
        FileManagement::CloudSync::CloudSyncManager::GetInstance().ResetCursor(true, SHARED_ALBUM_BUNDLE_NAME);
        MEDIA_INFO_LOG("cloud sync manager end reset share album cursor");
    }
    return E_OK;
}
} // namespace OHOS::Media

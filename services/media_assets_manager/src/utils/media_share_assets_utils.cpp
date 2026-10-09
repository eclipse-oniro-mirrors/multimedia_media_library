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

#define MLOG_TAG "Media_Utils"

#include "media_share_assets_utils.h"

#include <cinttypes>
#include <memory>
#include <string>

#include "cloud_sync_notify_handler.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "preferences.h"
#include "preferences_helper.h"

namespace OHOS::Media {
// 超时时间
constexpr int64_t SOUTH_DEVICE_CLEAN_DATA_TIMEOUT_MILLISECOND = 12 * 60 * 60 * 1000;
const std::string SHARE_RETAIN_STATUS_INFO = "/data/storage/el2/base/preferences/share_retain_status_info.xml";
const std::string SHARE_RETAIN_STATUS_KEY = "persist.multimedia.medialibrary.retain.share.status";

void MediaShareAssetsCloudExitUtils::SetShareAssetCleanStatus(CloudSyncStatus status)
{
    // 防止一直无法恢复, 使用时间戳代替开关
    int64_t timeStamp = 0;
    if (status == CloudSyncStatus::CLOUD_CLEANING) {
        timeStamp = MediaFileUtils::UTCTimeMilliSeconds();
    }

    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(SHARE_RETAIN_STATUS_INFO, errCode);

    if (errCode != E_OK || prefs == nullptr) {
        MEDIA_ERR_LOG("Failed to get preferences, errCode = %{public}d", errCode);
        return;
    }
    bool retFlag = prefs->PutLong(SHARE_RETAIN_STATUS_KEY, timeStamp);
    prefs->FlushSync();
    MEDIA_INFO_LOG("SetShareAssetCleanStatus set status: %{public}d, result: %{public}d, timeStamp: %{public}" PRId64,
        static_cast<int32_t>(status), retFlag, timeStamp);
}

bool MediaShareAssetsCloudExitUtils::IsShareAssetCleaning()
{
    int64_t timeStamp = 0;
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(SHARE_RETAIN_STATUS_INFO, errCode);

    if (errCode != E_OK || prefs == nullptr) {
        MEDIA_ERR_LOG("Failed to get preferences, errCode = %{public}d", errCode);
        return false;
    }
    timeStamp = prefs->GetLong(SHARE_RETAIN_STATUS_KEY, timeStamp);
    if (timeStamp == 0) {
        return false;
    }

    auto nowTime = MediaFileUtils::UTCTimeMilliSeconds();
    return ((nowTime - timeStamp) < SOUTH_DEVICE_CLEAN_DATA_TIMEOUT_MILLISECOND);
}
}  // namespace OHOS::Media

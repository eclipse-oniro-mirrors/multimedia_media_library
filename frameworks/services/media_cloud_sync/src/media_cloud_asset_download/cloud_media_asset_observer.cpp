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

#define MLOG_TAG "CloudMediaAssetObserver"

#include "cloud_media_asset_observer.h"

#include <list>

#include "cloud_media_asset_download_operation.h"
#include "cloud_sync_utils.h"
#include "common_event_utils.h"
#include "media_log.h"
#include "medialibrary_subscriber.h"
#include "uri.h"
#include "medialibrary_related_system_state_manager.h"

using namespace std;
using Uri = OHOS::Uri;

namespace OHOS {
namespace Media {

void CloudMediaAssetObserver::OnChange(const ChangeInfo &changeInfo)
{
    bool cond = (operation_ == nullptr || operation_->GetTaskStatus() == CloudMediaAssetTaskStatus::IDLE);
    CHECK_AND_RETURN(!cond);
    CHECK_AND_RETURN_INFO_LOG(!MedialibraryRelatedSystemStateManager::GetInstance()->IsWifiConnectedAtRealTime(),
        "wifi not connection.");

    std::list<Uri> uris = changeInfo.uris_;
    for (auto &uri : uris) {
        bool cond = (uri.ToString() != CLOUD_URI || changeInfo.changeType_ != DataShareObserver::ChangeType::OTHER);
        CHECK_AND_RETURN_INFO_LOG(!cond, "Current uri is not suitable for task.");

        bool isUnlimitedTrafficStatusOn = CloudSyncUtils::IsUnlimitedTrafficStatusOn();
        if (operation_->GetTaskStatus() == CloudMediaAssetTaskStatus::DOWNLOADING) {
            CHECK_AND_RETURN(!isUnlimitedTrafficStatusOn);
            CloudMediaTaskPauseCause pauseCause =
                MedialibraryRelatedSystemStateManager::GetInstance()->IsCellularNetConnectedAtRealTime() ?
                CloudMediaTaskPauseCause::WIFI_UNAVAILABLE : CloudMediaTaskPauseCause::NETWORK_FLOW_LIMIT;
            operation_->PauseDownloadTask(pauseCause);
            MEDIA_INFO_LOG("Cloud media asset download paused, pauseCause: %{public}d.",
                static_cast<int32_t>(pauseCause));
            return;
        }

        if (operation_->GetTaskStatus() == CloudMediaAssetTaskStatus::PAUSED &&
            MedialibraryRelatedSystemStateManager::GetInstance()->IsCellularNetConnectedAtRealTime() &&
            isUnlimitedTrafficStatusOn) {
            operation_->PassiveStatusRecoverTask(CloudMediaTaskRecoverCause::NETWORK_NORMAL);
            MEDIA_INFO_LOG("Cloud media asset download recovered, recoverCause: %{public}d.",
                static_cast<int32_t>(CloudMediaTaskRecoverCause::NETWORK_NORMAL));
            return;
        }
    }
}
} // namespace Media
} // namespace OHOS
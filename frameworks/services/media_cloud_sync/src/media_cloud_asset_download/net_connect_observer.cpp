/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "NetConnectObserver"

#include "net_connect_observer.h"

#include "cloud_media_asset_manager.h"
#include "cloud_sync_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"

using namespace std;
using namespace OHOS::NetManagerStandard;

namespace OHOS {
namespace Media {
void NetConnectObserver::SetNetConnStatus(const NetConnStatus status)
{
    netStatus_ = status;
}

int32_t NetConnectObserver::NetCapabilitiesChange(sptr<NetHandle> &netHandle, const sptr<NetAllCapabilities> &netAllCap)
{
    CHECK_AND_RETURN_RET_LOG(netAllCap != nullptr, E_ERR, "netAllCap is nullptr");
    if (netAllCap->netCaps_.count(NetCap::NET_CAPABILITY_INTERNET) == 0) {
        return E_OK;
    }
    if (netAllCap->bearerTypes_.count(BEARER_WIFI)) {
        MEDIA_INFO_LOG("wifi connected");
        SetNetConnStatus(NetConnStatus::WIFI_CONNECTED);
        CloudMediaAssetManager::GetInstance().RecoverDownloadCloudAsset(CloudMediaTaskRecoverCause::NETWORK_NORMAL);
    } else if (netAllCap->bearerTypes_.count(BEARER_CELLULAR)) {
        MEDIA_INFO_LOG("cellular connected");
        SetNetConnStatus(NetConnStatus::CELLULAR_CONNECTED);
        if (CloudSyncUtils::IsUnlimitedTrafficStatusOn()) {
            CloudMediaAssetManager::GetInstance().RecoverDownloadCloudAsset(CloudMediaTaskRecoverCause::NETWORK_NORMAL);
        } else {
            CloudMediaAssetManager::GetInstance().PauseDownloadCloudAsset(CloudMediaTaskPauseCause::WIFI_UNAVAILABLE);
        }
    } else {
        MEDIA_INFO_LOG("other net connected");
        SetNetConnStatus(NetConnStatus::NO_NETWORK);
        CloudMediaAssetManager::GetInstance().PauseDownloadCloudAsset(CloudMediaTaskPauseCause::NETWORK_FLOW_LIMIT);
    }
    return E_OK;
}

int32_t NetConnectObserver::NetLost(sptr<NetHandle> &netHandle)
{
    MEDIA_INFO_LOG("net lost");
    SetNetConnStatus(NetConnStatus::NO_NETWORK);
    CloudMediaAssetManager::GetInstance().PauseDownloadCloudAsset(CloudMediaTaskPauseCause::NETWORK_FLOW_LIMIT);
    return E_OK;
}
} // namespace Media
} // namespace OHOS
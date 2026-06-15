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

#define MLOG_TAG "DefaultNetConnectObserver"

#include "background_cloud_batch_selected_file_processor.h"
#include "default_net_connect_observer.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#ifdef MEDIALIBRARY_SECURE_ALBUM_ENABLE
#include "critical_label_task_queue.h"
#endif

using namespace std;
using namespace OHOS::NetManagerStandard;

namespace OHOS {
namespace Media {
// LCOV_EXCL_START
void DefaultNetConnectObserver::SetNetConnStatus(const DefaultNetConnStatus status)
{
    netStatus_ = status;
}

int32_t DefaultNetConnectObserver::NetCapabilitiesChange(sptr<NetHandle> &netHandle,
    const sptr<NetAllCapabilities> &netAllCap)
{
    CHECK_AND_RETURN_RET_LOG(netAllCap != nullptr, E_ERR, "netAllCap is nullptr");
    if (netAllCap->netCaps_.count(NetCap::NET_CAPABILITY_INTERNET) == 0) {
        return E_OK;
    }
    if (netAllCap->bearerTypes_.count(BEARER_WIFI)) {
        MEDIA_INFO_LOG("DefaultNetConnectObserver wifi connected");
        SetNetConnStatus(DefaultNetConnStatus::WIFI_CONNECTED);
        MedialibraryRelatedSystemStateManager::GetInstance()->SetWifiConnected(true);
        BackgroundCloudBatchSelectedFileProcessor::TriggerAutoResumeBatchDownloadResourceCheck();
    } else if (netAllCap->bearerTypes_.count(BEARER_CELLULAR)) {
        MEDIA_INFO_LOG("DefaultNetConnectObserver cellular connected");
        MedialibraryRelatedSystemStateManager::GetInstance()->SetCellularNetConnected(true);
        SetNetConnStatus(DefaultNetConnStatus::CELLULAR_CONNECTED);
    } else {
        MEDIA_INFO_LOG("DefaultNetConnectObserver other net connected");
        SetNetConnStatus(DefaultNetConnStatus::NO_NETWORK);
    }
#ifdef MEDIALIBRARY_SECURE_ALBUM_ENABLE
        auto criticalLabelTaskQueue = TTLPriorityQueue::GetInstance();
        if (criticalLabelTaskQueue != nullptr) {
            criticalLabelTaskQueue->NotifyThread();
        }
#endif
    return E_OK;
}

int32_t DefaultNetConnectObserver::NetLost(sptr<NetHandle> &netHandle)
{
    MEDIA_INFO_LOG("net lost");
    SetNetConnStatus(DefaultNetConnStatus::NO_NETWORK);
    return E_OK;
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS
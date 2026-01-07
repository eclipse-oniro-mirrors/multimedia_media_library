/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define MLOG_TAG "MedialibraryRelatedSystemStateManager"

#include "medialibrary_related_system_state_manager.h"
#include <sys/statvfs.h>

#include "cloud_sync_utils.h"
#ifdef HAS_WIFI_MANAGER_PART
#include "wifi_device.h"
#endif
#include "net_conn_client.h"
#include "medialibrary_tracer.h"
#include "media_log.h"
#include "medialibrary_errno.h"

namespace OHOS {
namespace Media {
std::mutex MedialibraryRelatedSystemStateManager::mutex_;
std::shared_ptr<MedialibraryRelatedSystemStateManager> medialibraryRelatedSystemStateManager = nullptr;
// LCOV_EXCL_START
 
std::shared_ptr<MedialibraryRelatedSystemStateManager> MedialibraryRelatedSystemStateManager::GetInstance()
{
    if (medialibraryRelatedSystemStateManager == nullptr) { // 双重检查锁定 (DCLP)
        std::lock_guard<std::mutex> lock(mutex_);
        if (medialibraryRelatedSystemStateManager == nullptr) {
            medialibraryRelatedSystemStateManager = std::make_shared<MedialibraryRelatedSystemStateManager>();
            int32_t ret = medialibraryRelatedSystemStateManager->Init();
            CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, nullptr, "failed to init medialibraryRelatedSystemStateManager");
        }
    }
    return medialibraryRelatedSystemStateManager;
}

int32_t MedialibraryRelatedSystemStateManager::Init()
{
    MediaLibraryTracer tracer;
    tracer.Start("MedialibraryRelatedSystemStateManager::Init");
    return ERR_OK;
}
 
MedialibraryRelatedSystemStateManager::MedialibraryRelatedSystemStateManager()
{
    MEDIA_DEBUG_LOG("Instances create");
}

MedialibraryRelatedSystemStateManager::~MedialibraryRelatedSystemStateManager()
{
    MEDIA_DEBUG_LOG("Instances Destroy");
}

void MedialibraryRelatedSystemStateManager::SetCellularNetConnected(bool isCellularNetConnected)
{
    isCellularNetConnected_ = isCellularNetConnected;
}

bool MedialibraryRelatedSystemStateManager::IsCellularNetConnected()
{
    return isCellularNetConnected_;
}

void MedialibraryRelatedSystemStateManager::SetWifiConnected(bool isWifiConnected)
{
    isWifiConnected_ = isWifiConnected;
}

bool MedialibraryRelatedSystemStateManager::IsWifiConnected()
{
    return isWifiConnected_;
}

bool MedialibraryRelatedSystemStateManager::IsWifiConnectedAtRealTime()
{
    bool isWifiConnected = false;
#ifdef HAS_WIFI_MANAGER_PART
    auto wifiDevicePtr = Wifi::WifiDevice::GetInstance(WIFI_DEVICE_ABILITY_ID);
    if (wifiDevicePtr == nullptr) {
        MEDIA_ERR_LOG("wifiDevicePtr is null");
    } else {
        ErrCode ret = wifiDevicePtr->IsConnected(isWifiConnected);
        if (ret != Wifi::WIFI_OPT_SUCCESS) {
            MEDIA_ERR_LOG("Get-IsConnected-fail: -%{public}d", ret);
        }
    }
#endif
    SetWifiConnected(isWifiConnected);
    return isWifiConnected;
}

bool MedialibraryRelatedSystemStateManager::IsCellularNetConnectedAtRealTime()
{
    bool isCellularNetConnected = false;
    NetManagerStandard::NetHandle handle;
    NetManagerStandard::NetAllCapabilities netAllCap;
    int32_t ret = NetManagerStandard::NetConnClient::GetInstance().GetDefaultNet(handle);
    CHECK_AND_RETURN_RET_LOG(ret == 0, isCellularNetConnected, "GetDefaultNet failed, err:%{public}d", ret);
    ret = NetManagerStandard::NetConnClient::GetInstance().GetNetCapabilities(handle, netAllCap);
    CHECK_AND_RETURN_RET_LOG(ret == 0, isCellularNetConnected, "GetNetCapabilities failed, err:%{public}d",
        ret);
    const std::set<NetManagerStandard::NetBearType>& types = netAllCap.bearerTypes_;
    if (types.count(NetManagerStandard::BEARER_CELLULAR)) {
        isCellularNetConnected = true;
        MEDIA_DEBUG_LOG("init cellular status success: %{public}d", isCellularNetConnected);
    }
    SetCellularNetConnected(isCellularNetConnected);
    return isCellularNetConnected;
}

bool MedialibraryRelatedSystemStateManager::IsNetValidatedAtRealTime()
{
    bool isNetValidated = false;
    NetManagerStandard::NetHandle handle;
    NetManagerStandard::NetAllCapabilities netAllCap;
    int32_t ret = NetManagerStandard::NetConnClient::GetInstance().GetDefaultNet(handle);
    CHECK_AND_RETURN_RET_LOG(ret == 0, isNetValidated, "GetDefaultNet failed, err:%{public}d", ret);
    NetManagerStandard::NetConnClient::GetInstance().GetNetCapabilities(handle, netAllCap);
    CHECK_AND_RETURN_RET_LOG(ret == 0, isNetValidated, "GetNetCapabilities failed, err:%{public}d",
        ret);
    const std::set<NetManagerStandard::NetCap>& types = netAllCap.netCaps_;
    if (types.count(NetManagerStandard::NET_CAPABILITY_INTERNET) &&
        types.count(NetManagerStandard::NET_CAPABILITY_VALIDATED)) {
        isNetValidated = true;
    }
    MEDIA_DEBUG_LOG("BatchSelectFileDownload net validate : %{public}d", isNetValidated);
    return isNetValidated;
}

// wifi和 无限流量场景组合 wifi连接+未使用蜂窝+网络连通 受无限流量开关控制 不偷跑流量
bool MedialibraryRelatedSystemStateManager::IsNetAvailableWithUnlimitCondition()
{
    return IsNetValidatedAtRealTime() && ((IsWifiConnected() && !IsCellularNetConnected()) ||
        (IsCellularNetConnected() && CloudSyncUtils::IsUnlimitedTrafficStatusOn()));
}

// 仅wifi下可用
bool MedialibraryRelatedSystemStateManager::IsNetAvailableInOnlyWifiCondition()
{
    return IsNetValidatedAtRealTime() && (IsWifiConnected() && !IsCellularNetConnected());
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS
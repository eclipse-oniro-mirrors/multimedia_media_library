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

#ifndef OHOS_NET_CONNECT_OBSERVER_H
#define OHOS_NET_CONNECT_OBSERVER_H

#include "net_all_capabilities.h"
#include "net_conn_callback_stub.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

enum class NetConnStatus : int32_t {
    NO_NETWORK = 0,
    WIFI_CONNECTED,
    CELLULAR_CONNECTED,
};

class EXPORT NetConnectObserver : public NetManagerStandard::NetConnCallbackStub {
public:
    NetConnectObserver() {}
    int32_t NetCapabilitiesChange(sptr<NetManagerStandard::NetHandle> &netHandle,
        const sptr<NetManagerStandard::NetAllCapabilities> &netAllCap) override;
    int32_t NetLost(sptr<NetManagerStandard::NetHandle> &netHandle) override;

private:
    void SetNetConnStatus(const NetConnStatus status);

private:
    NetConnStatus netStatus_ = NetConnStatus::NO_NETWORK;
};
} // namespace OHOS::Media
#endif  // OHOS_NET_CONNECT_OBSERVER_H
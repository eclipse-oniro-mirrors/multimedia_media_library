/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORKS_SERVICES_CLOUD_SYNC_NOTIFY_HANDLE_INCLUDE_WATCH_SYSTEM_HANDLER_H
#define FRAMEWORKS_SERVICES_CLOUD_SYNC_NOTIFY_HANDLE_INCLUDE_WATCH_SYSTEM_HANDLER_H

#ifdef MEDIALIBRARY_SECURE_ALBUM_ENABLE

#include <mutex>

#include "datashare_helper.h"
#include "medialibrary_async_worker.h"
#include "userfile_manager_types.h"
#include "watch_lite/cloud_audit_callback.h"

namespace OHOS {

namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

class EXPORT WatchSystemHandler : public WatchSystemService::CloudAuditCallback {
public:
    WatchSystemHandler() = default;
    ~WatchSystemHandler() = default;

    void OnReceiveData(const std::string &msg, int32_t type) override;
    void OnReceiveConfigData(const std::string &key, const std::string &values) override;

    static bool GetAllowNetworkSwitch();
    static void UpdateAllowNetworkSwitch();
    static void ParseAssetName(const std::string &fileAssetData, std::string &displayName);

private:
    // -1 error 0, 1 valid
    inline static int32_t allowNetworkSwitch = -1;
    inline static const std::string ALBUM_TRAFFIC_MONITOR_SWITCH  = "albumTrafficMonitorSwitch";
    
    void SetAllowNetworkSwitch(int32_t val);
    void ParsePushInfo(const std::string &msg, std::string &displayName, int32_t &isCritical,
        int32_t &criticalType);
};
}
}
#endif // FRAMEWORKS_SERVICES_CLOUD_SYNC_NOTIFY_HANDLE_INCLUDE_WATCH_SYSTEM_HANDLER_H
#endif
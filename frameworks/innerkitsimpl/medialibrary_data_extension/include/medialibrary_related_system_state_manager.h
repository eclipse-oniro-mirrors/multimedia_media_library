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
 
#ifndef OHOS_MEDIALIBRARY_RELATED_SYSTEM_STATE_MANAGER_H
#define OHOS_MEDIALIBRARY_RELATED_SYSTEM_STATE_MANAGER_H
 
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
 
namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
 
 
class MedialibraryRelatedSystemStateManager {
public:
    static std::shared_ptr<MedialibraryRelatedSystemStateManager> GetInstance();
 
    EXPORT void SetCellularNetConnected(bool isCellularNetConnected);
    EXPORT void SetWifiConnected(bool isWifiConnected);
 
    EXPORT bool IsWifiConnectedAtRealTime();
    EXPORT bool IsCellularNetConnectedAtRealTime();
    EXPORT bool IsNetValidatedAtRealTime();
    EXPORT bool IsCellularNetConnected();
    EXPORT bool IsWifiConnected();
    EXPORT bool IsNetAvailableWithUnlimitCondition();
    EXPORT bool IsNetAvailableInOnlyWifiCondition();

    int32_t Init();
    MedialibraryRelatedSystemStateManager();
    ~MedialibraryRelatedSystemStateManager();
private:
 
    std::atomic<bool> isCellularNetConnected_{false};
    std::atomic<bool> isWifiConnected_{false};
    static std::mutex mutex_;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_RELATED_SYSTEM_STATE_MANAGER_H
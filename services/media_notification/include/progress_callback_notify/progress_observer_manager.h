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

#ifndef PROGRESS_OBSERVER_MANAGER_H
#define PROGRESS_OBSERVER_MANAGER_H

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "data_ability_observer_stub.h"
#include "iremote_object.h"
#include "parcel.h"
#include "media_progress_change_info.h"

namespace OHOS {
namespace AAFwk {
class DataAbilityObserverStub;
} // namespace AAFwk
} // namespace OHOS

namespace OHOS {
namespace Media {
namespace Notification {
#define EXPORT __attribute__ ((visibility ("default")))
class EXPORT ProgressObserverManager {
public:
    static ProgressObserverManager &GetInstance();
    
    // 添加观察者
    int32_t AddObserver(const int32_t &requestId, const sptr<AAFwk::IDataAbilityObserver> &observer);
    
    // 移除观察者
    int32_t RemoveObserver(const int32_t &requestId);
    
    // 查找观察者
    sptr<AAFwk::IDataAbilityObserver> GetObserver(const int32_t &requestId);
    
    // 触发观察者回调
    bool NotifyProgress(const std::shared_ptr<MediaProgressChangeInfo> &progressInfo);
    
private:
    ProgressObserverManager() = default;
    ~ProgressObserverManager() = default;
    
    std::mutex mutex_;
    std::unordered_map<int32_t, sptr<AAFwk::IDataAbilityObserver>> observers_;
};

} // namespace Notification
} // namespace Media
} // namespace OHOS

#endif // PROGRESS_OBSERVER_MANAGER_H
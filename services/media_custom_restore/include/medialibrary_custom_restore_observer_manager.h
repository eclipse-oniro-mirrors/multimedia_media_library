/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#ifndef OHOS_MEDIALIBRARY_CUSTOM_RESTORE_OBSERVER_MANAGER_H
#define OHOS_MEDIALIBRARY_CUSTOM_RESTORE_OBSERVER_MANAGER_H

#include <safe_map.h>
#include <string>

#include "datashare_helper.h"
#include "media_library_custom_restore.h"

namespace OHOS {
namespace Media {

class CustomRestoreNotifyObserver : public DataShare::DataShareObserver {
public:
    explicit CustomRestoreNotifyObserver(std::shared_ptr<CustomRestoreCallback> customRestoreCallback)
        : customRestoreCallback_(customRestoreCallback) {}
    CustomRestoreNotifyObserver() = default;
    ~CustomRestoreNotifyObserver() = default;
    void OnChange(const ChangeInfo &changeInfo) override;
private:
    std::shared_ptr<CustomRestoreCallback> customRestoreCallback_;
};

class CustomRestoreObserverManager {
public:
    static CustomRestoreObserverManager &GetInstance();
    std::shared_ptr<CustomRestoreNotifyObserver> QueryObserver(std::shared_ptr<CustomRestoreCallback> callback);
    bool AttachObserver(std::shared_ptr<CustomRestoreCallback> callback,
        std::shared_ptr<CustomRestoreNotifyObserver> notifyObserver);
    bool DetachObserver(std::shared_ptr<CustomRestoreCallback> callback);
private:
    CustomRestoreObserverManager() = default;
    ~CustomRestoreObserverManager() = default;
    SafeMap<std::shared_ptr<CustomRestoreCallback>,
        std::shared_ptr<CustomRestoreNotifyObserver>> callbackMap_;
};

} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_CUSTOM_RESTORE_OBSERVER_MANAGER_H
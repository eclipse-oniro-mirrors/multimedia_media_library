/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef CLONE_STATUS_LISTENER_H
#define CLONE_STATUS_LISTENER_H
 
#include <atomic>
#include <string>
#include <mutex>

#include "singleton.h"
#include "iremote_object.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
class CloneStatusListener : public DelayedSingleton<CloneStatusListener> {
    DECLARE_DELAYED_SINGLETON(CloneStatusListener);

public:
    void RegisterCloneStatusChangeListener();
    void UnRegisterCloneStatusChangeListener();
    void HandleDeathRecipient();
    bool InitAsync();

private:
    void HandleCloneStatusChanged();
    void SetDeathRecipient();
    static void OnParameterChange(const char *key, const char *value, void *context);

private:
    bool isCloneStatusChangedListenerRegistered_ = false;
    std::mutex registerMutex_;
    std::mutex deathRecipientMutex_;
    sptr<OHOS::IRemoteObject> backupSaRemoteObject_;
};

class EXPORT CloneStatusListenerDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    CloneStatusListenerDeathRecipient() {}
    ~CloneStatusListenerDeathRecipient() {}
    void OnRemoteDied(const wptr<IRemoteObject> &object);
};
} // namespace OHOS::Media
#endif  //CLONE_STATUS_LISTENER_H

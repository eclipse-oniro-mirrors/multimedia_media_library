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
#ifndef MEDIA_LIBRARY_MEDIA_LAKE_CLONE_EVENT_MANAGER_H
#define MEDIA_LIBRARY_MEDIA_LAKE_CLONE_EVENT_MANAGER_H

#include <string>

#include "iremote_object.h"
#include "want.h"

namespace OHOS {
namespace Media {
class MediaLakeCloneEventManager {
public:
    static MediaLakeCloneEventManager &GetInstance()
    {
        static MediaLakeCloneEventManager instance;
        return instance;
    }
    static bool IsRestoreEvent(const AAFwk::Want &want);
    bool IsRestoring();
    void HandleRestoreEvent(const AAFwk::Want &want);
    void HandleDeathRecipient();

private:
    static bool IsSubscribedAction(const AAFwk::Want &want);
    static bool IsSubscribedBundle(const AAFwk::Want &want);
    static std::string GetEventBundleName(const AAFwk::Want &want);
    void HandleRestoreStartEvent(const AAFwk::Want &want);
    void HandleRestoreEndEvent(const AAFwk::Want &want);
    bool GetBitByBundleName(const std::string &bundleName, uint8_t &bit);
    void SetRestoreStatusBitMapForStart(const std::string &bundleName);
    void SetRestoreStatusBitMapForEnd(const std::string &bundleName);
    void UnregisterLakeFileMonitor();
    void RegisterLakeFileMonitor();
    void CheckIsExecuteGlobalScan(uint8_t bit);
    void RunGlobalScanner();
    bool ShouldUnregisterLakeFileMonitor();
    bool ShouldRegisterLakeFileMonitor();
    void SetDeathRecipient();
    void ResetRestoreStatusBitMap();

    std::atomic<uint32_t> initRestoreStatusBitMap_ = 0;
    std::atomic<uint32_t> currentRestoreStatusBitMap_ = 0;
    std::atomic<bool> isExecuteGlobalScan_ = true;
    std::mutex deathRecipientMutex_;
    std::mutex statusMutex_;
    sptr<OHOS::IRemoteObject> backupSaRemoteObject_;
};

class MediaLakeCloneDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    MediaLakeCloneDeathRecipient() {}
    ~MediaLakeCloneDeathRecipient() {}
    void OnRemoteDied(const wptr<IRemoteObject> &object);
};
}
}
#endif // MEDIA_LIBRARY_MEDIA_LAKE_CLONE_EVENT_MANAGER_H
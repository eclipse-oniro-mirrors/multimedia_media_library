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

#ifndef OHOS_MEDIA_LCD_AGING_WORKER_H
#define OHOS_MEDIA_LCD_AGING_WORKER_H

#include <atomic>
#include <thread>

#include "deep_optimize_space_callback.h"
#include "iremote_object.h"

namespace OHOS::Media {

class LcdAgingWorker {
public:
    static LcdAgingWorker& GetInstance();
    
    int32_t StartDeepOptimizeSpace(const sptr<IRemoteObject> &clientRemote,
                                    const sptr<IRemoteObject> &callbackRemote);
    int32_t StopDeepOptimizeSpace();
    bool IsRunning();
    void OnClientDied();
    void NotifyProgress(DeepOptimizeSpaceState state, int32_t progress);

private:
    LcdAgingWorker() {}
    ~LcdAgingWorker();
    LcdAgingWorker(const LcdAgingWorker &worker) = delete;
    const LcdAgingWorker &operator=(const LcdAgingWorker &worker) = delete;

    class ClientDeathRecipient;
    
    void HandleDeepOptimizeTask();
    void Cleanup();
    void CleanupInternal();

    std::atomic<bool> shouldStop_{false};
    std::atomic<bool> isThreadRunning_{false};
    std::thread workerThread_;
    std::mutex mutex_;
    
    sptr<IRemoteObject> clientRemote_;
    sptr<IDepOptimizeSpaceCallback> callbackProxy_;
    sptr<ClientDeathRecipient> deathRecipient_;
};

class LcdAgingWorker::ClientDeathRecipient final : public IRemoteObject::DeathRecipient {
public:
    explicit ClientDeathRecipient(LcdAgingWorker *worker) : worker_(worker) {}
    
    void OnRemoteDied(const wptr<IRemoteObject> &remote) override
    {
        if (worker_ != nullptr) {
            worker_->OnClientDied();
        }
    }

private:
    LcdAgingWorker *worker_;
};

}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_LCD_AGING_WORKER_H
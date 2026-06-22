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
#define MLOG_TAG "Lcd_Aging"

#include "lcd_aging_worker.h"

#include <chrono>
#include <thread>

#include "lcd_aging_manager.h"
#include "media_log.h"
#include "medialibrary_errno.h"

namespace OHOS::Media {

LcdAgingWorker& LcdAgingWorker::GetInstance()
{
    static LcdAgingWorker instance;
    return instance;
}

LcdAgingWorker::~LcdAgingWorker()
{
    shouldStop_.store(true);
    if (workerThread_.joinable()) {
        MEDIA_INFO_LOG("Waiting for worker thread to finish");
        workerThread_.join();
        MEDIA_INFO_LOG("Worker thread finished");
    }
    isThreadRunning_.store(false);
}

void LcdAgingWorker::CleanupInternal()
{
    if (clientRemote_ != nullptr && deathRecipient_ != nullptr) {
        clientRemote_->RemoveDeathRecipient(deathRecipient_);
    }
    clientRemote_ = nullptr;
    deathRecipient_ = nullptr;
    callbackProxy_ = nullptr;
    isThreadRunning_.store(false);
    shouldStop_.store(false);
}

void LcdAgingWorker::Cleanup()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CleanupInternal();
}

int32_t LcdAgingWorker::StartDeepOptimizeSpace(const sptr<IRemoteObject> &clientRemote,
    const sptr<IRemoteObject> &callbackRemote)
{
    CHECK_AND_RETURN_RET_LOG(clientRemote != nullptr, E_ERR, "Client remote is null");
    
    bool expected = false;
    CHECK_AND_RETURN_RET_LOG(isThreadRunning_.compare_exchange_strong(expected, true), E_OPERATION_NOT_SUPPORT,
        "Task already running");
    CHECK_AND_EXECUTE(!workerThread_.joinable(), workerThread_.join());
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        clientRemote_ = clientRemote;
        deathRecipient_ = sptr<ClientDeathRecipient>(new ClientDeathRecipient(this));
        if (!clientRemote_->AddDeathRecipient(deathRecipient_)) {
            MEDIA_ERR_LOG("Failed to add death recipient");
            CleanupInternal();
            return E_ERR;
        }
        
        if (callbackRemote != nullptr) {
            callbackProxy_ = iface_cast<IDepOptimizeSpaceCallback>(callbackRemote);
            if (callbackProxy_ == nullptr) {
                MEDIA_ERR_LOG("Failed to cast callback proxy");
                CleanupInternal();
                return E_ERR;
            }
        }
    }
    
    shouldStop_.store(false);
    workerThread_ = std::thread([this]() { HandleDeepOptimizeTask(); });
    MEDIA_INFO_LOG("Deep optimize space task started");
    return E_OK;
}

int32_t LcdAgingWorker::StopDeepOptimizeSpace()
{
    CHECK_AND_RETURN_RET_WARN_LOG(isThreadRunning_.load(), E_OK, "no task running");
    shouldStop_.store(true);
    MEDIA_DEBUG_LOG("Stop deep optimize space task");
    return E_OK;
}

bool LcdAgingWorker::IsRunning()
{
    return isThreadRunning_.load();
}

void LcdAgingWorker::OnClientDied()
{
    MEDIA_WARN_LOG("Client died, cleaning up callback");
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callbackProxy_ = nullptr;
    }
    shouldStop_.store(true);
}

void LcdAgingWorker::NotifyProgress(DeepOptimizeSpaceState state, int32_t progress)
{
    sptr<IDepOptimizeSpaceCallback> callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callback = callbackProxy_;
    }
    
    if (callback != nullptr && isThreadRunning_.load()) {
        DeepOptimizeSpaceProgress data;
        data.state = state;
        data.progress = progress;
        callback->OnProgressUpdate(data);
    }
}

void LcdAgingWorker::HandleDeepOptimizeTask()
{
    MEDIA_INFO_LOG("Start HandleDeepOptimizeTask thread");
    pthread_setname_np(pthread_self(), "DeepOptimizeTask");
    
    int32_t ret = LcdAgingManager::GetInstance().BatchAgingLcdFileTask(shouldStop_);
    MEDIA_INFO_LOG("Execute finished, ret: %{public}d", ret);
    
    Cleanup();
    MEDIA_INFO_LOG("End HandleDeepOptimizeTask thread");
}

}  // namespace OHOS::Media
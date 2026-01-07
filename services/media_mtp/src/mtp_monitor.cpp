/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MtpMonitor"
#include "mtp_monitor.h"
#include "media_log.h"
#include "mtp_file_observer.h"
#include "mtp_medialibrary_manager.h"
#include "mtp_store_observer.h"
using namespace std;
namespace OHOS {
namespace Media {
constexpr int32_t IPC_SERVER_RELEASED = -204;
constexpr int32_t IPC_OBJECT_INVALID = -4;
constexpr int32_t ERROR_SLEEP_TIME = 10;

void MtpMonitor::Start()
{
    MEDIA_INFO_LOG("MtpMonitor::Start begin threadRunning_:%{public}d", threadRunning_.load());
    if (!threadRunning_.load()) {
        threadRunning_.store(true);
        if (thread_ == nullptr) {
            thread_ = std::make_unique<std::thread>(&MtpMonitor::Run, this);
        }
    }
    MEDIA_INFO_LOG("MtpMonitor::Start end threadRunning_:%{public}d", threadRunning_.load());
}

void MtpMonitor::Stop()
{
    MEDIA_INFO_LOG("MtpMonitor::Stop begin threadRunning_:%{public}d", threadRunning_.load());
    threadRunning_.store(false);
    if (thread_ != nullptr) {
        if (thread_->joinable()) {
            thread_->join();
        }
        thread_ = nullptr;
    }
    MEDIA_INFO_LOG("MtpMonitor::Stop end threadRunning_:%{public}d", threadRunning_.load());
}

void MtpMonitor::Run()
{
    MEDIA_INFO_LOG("MtpMonitor::Run start");
    pthread_setname_np(pthread_self(), "MtpMonitor::Run");
    while (threadRunning_.load()) {
        if (operationPtr_ == nullptr) {
            operationPtr_ = make_shared<MtpOperation>();
        }
        if (operationPtr_ != nullptr) {
            int32_t errorCode = operationPtr_->Execute();
            if (errorCode == IPC_SERVER_RELEASED || errorCode == IPC_OBJECT_INVALID) {
                break;
            }
            if (errorCode != 0 && threadRunning_.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(ERROR_SLEEP_TIME));
            }
        }
    }
    MEDIA_INFO_LOG("MtpMonitor::Run break");
    {
        MtpFileObserver::GetInstance().StopFileInotify();
        MtpStoreObserver::StopObserver();
        MtpMedialibraryManager::GetInstance()->Clear();
    }
    if (operationPtr_ != nullptr) {
        operationPtr_->Stop();
        operationPtr_.reset();
    }
    threadRunning_.store(false);
    MEDIA_INFO_LOG("MtpMonitor::Run end");
}
} // namespace Media
} // namespace OHOS

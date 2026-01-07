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

#include <string>
#include <dlfcn.h>
#include <thread>
#include <cstdint>
#define MLOG_TAG "MediaMtpServiceManager"

#include "media_mtp_service_manager.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
namespace {
constexpr int64_t WAIT_TIMEOUT = 60000; // 60s
const std::string PLUGIN_SO_PATH = "libmedia_mtp.z.so";
const std::string STOP_MTP_SERVICE = "StopMtpService";
const std::string START_MTP_SERVICE = "StartMtpService";
}

void MediaMtpServiceManager::StopMtpService()
{
    MEDIA_INFO_LOG("mtp MediaMtpServiceManager StopMtpService");
    CHECK_AND_RETURN_LOG(handler_, "Dynamic library libmedia_mtp.z.so not loaded");

    using StopMtpServiceFunc = void(*)();
    StopMtpServiceFunc stopMtpService = (StopMtpServiceFunc)dlsym(handler_, STOP_MTP_SERVICE.c_str());
    if (stopMtpService == nullptr) {
        MEDIA_ERR_LOG("Not find stopMtpService func.");
        CloseLibrary();
        return;
    }

    stopMtpService();
    isNeedClose_.store(true);
    NotifyRefreshStopWaitTime();
}

void MediaMtpServiceManager::StartMtpService(const MtpMode mode)
{
    MEDIA_INFO_LOG("mtp MediaMtpServiceManager StartMtpService");
    if (handler_ == nullptr) {
        handler_ = dlopen(PLUGIN_SO_PATH.c_str(), RTLD_NOW);
        CHECK_AND_RETURN_LOG(handler_, "Not find libmedia_mtp.z.so");
    }

    using StartMtpServiceFunc = void(*)(uint32_t mtpMode);
    StartMtpServiceFunc startMtpServiceFunc = (StartMtpServiceFunc) dlsym(handler_, START_MTP_SERVICE.c_str());
    if (startMtpServiceFunc == nullptr) {
        MEDIA_ERR_LOG("mtp dlsym failed: %{public}s", dlerror());
        CloseLibrary();
        return;
    }
    startMtpServiceFunc(static_cast<uint32_t>(mode));
    isNeedClose_.store(false);
    isTimerRefresh_.store(true);
    cv_.notify_all();
}

void MediaMtpServiceManager::NotifyRefreshStopWaitTime()
{
    MEDIA_INFO_LOG("mtp NotifyRefreshStopWaitTime");
    isTimerRefresh_.store(true);
    if (isThreadRunning_.load()) {
        cv_.notify_all();
    } else {
        isThreadRunning_.store(true);
        std::thread([&] { CloseDlopenByStop(); }).detach();
    }
}

void MediaMtpServiceManager::CloseDlopenByStop()
{
    while (isThreadRunning_.load()) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            isTimerRefresh_.store(false);
            cv_.wait_for(lock, std::chrono::milliseconds(WAIT_TIMEOUT), [] {
                return isTimerRefresh_.load() || !isThreadRunning_.load();
            });
            CHECK_AND_CONTINUE(!isTimerRefresh_.load());
        }
        if (!isThreadRunning_.load()) {
            CloseLibrary();
            return;
        }
        MEDIA_INFO_LOG("mtp CloseDlopenByStop dlclose start isNeedClose_: %{public}d", isNeedClose_.load());
        if (isNeedClose_.load()) {
            CloseLibrary();
        }
    }
}

void MediaMtpServiceManager::CloseLibrary()
{
    isThreadRunning_.store(false);
    isNeedClose_.store(false);
    isTimerRefresh_.store(false);
    cv_.notify_all();
    if (handler_ != nullptr) {
        dlclose(handler_);
        handler_ = nullptr;
        MEDIA_DEBUG_LOG("Dynamic library libmedia_mtp.z.so closed");
    }
}
} // namespace Media
} // namespace OHOS
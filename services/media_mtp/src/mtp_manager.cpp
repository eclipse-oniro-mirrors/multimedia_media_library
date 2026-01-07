/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MtpManager"

#include "mtp_manager.h"
#include "media_log.h"
#include "mtp_file_observer.h"
#include "mtp_service.h"
#include "mtp_store_observer.h"
#include "mtp_medialibrary_manager.h"
#include "os_account_manager.h"
#include "parameter.h"
#include "parameters.h"
#include "usb_srv_client.h"
#include "usb_srv_support.h"

#include <thread>

namespace OHOS {
namespace Media {
namespace {
    static std::mutex mutex_;
    std::shared_ptr<MtpService> mtpServicePtr = nullptr;
    std::atomic<bool> isMtpServiceRunning = false;
} // namespace
// LCOV_EXCL_START
MtpManager &MtpManager::GetInstance()
{
    static MtpManager instance;
    return instance;
}

std::shared_ptr<MtpService> GetMtpService()
{
    static std::once_flag oc;
    std::call_once(oc, []() {
        mtpServicePtr = std::make_shared<MtpService>();
    });
    return mtpServicePtr;
}

void StartMtpService(const uint32_t mode)
{
    MEDIA_INFO_LOG("MtpManager::StartMtpService is called");
    bool isForeground = true;
    OHOS::ErrCode errCode = OHOS::AccountSA::OsAccountManager::IsOsAccountForeground(isForeground);
    // not current user foreground, return
    bool cond = (errCode == ERR_OK && !isForeground);
    CHECK_AND_RETURN_LOG(!cond,
        "StartMtpService errCode = %{public}d isForeground %{public}d", errCode, isForeground);
    {
        std::unique_lock lock(mutex_);
        CHECK_AND_RETURN_INFO_LOG(!isMtpServiceRunning.load(),
            "MtpManager::StartMtpService -- service is already running");
        auto service = GetMtpService();
        CHECK_AND_RETURN_LOG(service != nullptr, "MtpManager mtpServicePtr is nullptr");
        if (MtpManager::GetInstance().mtpMode_ != MtpManager::MtpMode::NONE_MODE) {
            MtpDfxReporter::GetInstance().NotifyDoDfXReporter(static_cast<int32_t>(MtpManager::GetInstance().mtpMode_));
            service->StopService();
        }
        MtpManager::GetInstance().mtpMode_ = static_cast<MtpManager::MtpMode>(mode);
        if (static_cast<MtpManager::MtpMode>(mode) == MtpManager::MtpMode::MTP_MODE) {
            MtpFileObserver::GetInstance().StartFileInotify();
            MtpStoreObserver::StartObserver();
        }
        service->StartService();
        isMtpServiceRunning = true;
    }
}

void StopMtpService()
{
    MEDIA_INFO_LOG("MtpManager::StopMtpService is called");
    {
        std::unique_lock lock(mutex_);
        CHECK_AND_RETURN_INFO_LOG(isMtpServiceRunning.load(),
            "MtpManager::StopMtpService -- service is already stopped");
        auto service = GetMtpService();
        CHECK_AND_RETURN_LOG(service != nullptr, "MtpManager mtpServicePtr is nullptr");
        MtpDfxReporter::GetInstance().NotifyDoDfXReporter(static_cast<int32_t>(MtpManager::GetInstance().mtpMode_));
        MtpManager::GetInstance().mtpMode_ = MtpManager::MtpMode::NONE_MODE;
        service->StopService();
        isMtpServiceRunning = false;
    }
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS

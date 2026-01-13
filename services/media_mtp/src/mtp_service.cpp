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
#define MLOG_TAG "MtpService"
#include "mtp_service.h"
#include "media_log.h"
#include "mtp_dfx_reporter.h"
#include "mtp_media_library.h"

using namespace std;
namespace OHOS {
namespace Media {
MtpService::MtpService(void) : monitorPtr_(nullptr), isMonitorRun_(false)
{
}

void MtpService::Init()
{
    if (monitorPtr_ == nullptr) {
        monitorPtr_ = make_shared<MtpMonitor>();
    }
}

void MtpService::StartService()
{
    MEDIA_INFO_LOG("MtpService::StartService");
    {
        std::unique_lock lock(mutex_);
        Init();
        CHECK_AND_RETURN_LOG(!isMonitorRun_, "MtpService::StartService -- monitor is already running, return");
        CHECK_AND_RETURN_LOG(monitorPtr_ != nullptr, "MtpService::StartService monitorPtr_ is nullptr");
        MtpDfxReporter::GetInstance().Init();
        monitorPtr_->Start();
        isMonitorRun_ = true;
    }
}

void MtpService::StopService()
{
    MEDIA_INFO_LOG("MtpService::StopService");
    {
        std::unique_lock lock(mutex_);
        CHECK_AND_RETURN_LOG(isMonitorRun_, "MtpService::StopService -- monitor is not running, return");
        CHECK_AND_RETURN_LOG(monitorPtr_ != nullptr, "MtpService::StopService monitorPtr_ is nullptr");
        monitorPtr_->Stop();
        isMonitorRun_ = false;
        // after stop mtp service, clear the unordered_map memory of the MtpMediaLibrary
        MtpMediaLibrary::GetInstance()->Clear();
    }
}
} // namespace Media
} // namespace OHOS
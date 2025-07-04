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
#include "mtp_subscriber.h"
#include "mtp_medialibrary_manager.h"
#include "os_account_manager.h"
#include "parameter.h"
#include "parameters.h"
#include "usb_srv_client.h"
#include "usb_srv_support.h"

#include <thread>

#define USB_FUNTION_HDC     (1 << 2)
#define USB_FUNTION_MTP     (1 << 3)
#define USB_FUNTION_PTP     (1 << 4)
#define USB_FUNTION_STORAGE     (1 << 9)

namespace OHOS {
namespace Media {
namespace {
    static std::mutex mutex_;
    std::shared_ptr<MtpService> mtpServicePtr = nullptr;
    std::atomic<bool> isMtpServiceRunning = false;
    const std::string KEY_CUST = "const.cust.custPath";
    const std::string CUST_DEFAULT = "phone";
    const std::string CUST_TOBBASIC = "tobbasic";
    const std::string CUST_HWIT = "hwit";
    const char *MTP_SERVER_DISABLE = "persist.edm.mtp_server_disable";
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

void MtpManager::Init()
{
    std::thread([]() {
        MEDIA_INFO_LOG("MtpManager Init");
        // IT管控 PC - tobasic/hwit 不启动MTP服务 start
        std::string cust = OHOS::system::GetParameter(KEY_CUST, CUST_DEFAULT);
        bool cond = (cust.find(CUST_TOBBASIC) != std::string::npos || cust.find(CUST_HWIT) != std::string::npos);
        CHECK_AND_RETURN_INFO_LOG(!cond, "MtpManager Init Return cust = [%{public}s]", cust.c_str());
        // IT管控 PC - tobasic/hwit 不启动MTP服务 end
        bool result = MtpSubscriber::Subscribe();
        MEDIA_INFO_LOG("MtpManager Subscribe result = %{public}d", result);
        // param 监听注册
        MtpManager::GetInstance().RegisterMtpParamListener();

        int32_t funcs = 0;
        int ret = OHOS::USB::UsbSrvClient::GetInstance().GetCurrentFunctions(funcs);
        MEDIA_INFO_LOG("MtpManager Init GetCurrentFunctions = %{public}d ret = %{public}d", funcs, ret);
        CHECK_AND_RETURN_LOG(ret == 0, "GetCurrentFunctions failed");
        uint32_t unsignedfuncs = static_cast<uint32_t>(funcs);
        if (unsignedfuncs & USB::UsbSrvSupport::Function::FUNCTION_MTP) {
            std::string param(MTP_SERVER_DISABLE);
            bool mtpDisable = system::GetBoolParameter(param, false);
            if (mtpDisable) {
                MEDIA_INFO_LOG("MtpManager Init MTP Manager persist.edm.mtp_server_disable = true");
            } else {
                MEDIA_INFO_LOG("MtpManager Init USB MTP connected");
                MtpManager::GetInstance().StartMtpService(MtpMode::MTP_MODE);
            }
            return;
        }
        if (unsignedfuncs & USB::UsbSrvSupport::Function::FUNCTION_PTP) {
            MtpManager::GetInstance().StartMtpService(MtpMode::PTP_MODE);
            return;
        }
        if (unsignedfuncs & USB::UsbSrvSupport::Function::FUNCTION_HDC) {
            MtpManager::GetInstance().StopMtpService();
        }
        MEDIA_INFO_LOG("MtpManager Init success end");
    }).detach();
    MtpManager::GetInstance().RemoveMtpParamListener();
}

void MtpManager::StartMtpService(const MtpMode mode)
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
        if (mtpMode_ != MtpMode::NONE_MODE) {
            MtpDfxReporter::GetInstance().NotifyDoDfXReporter(static_cast<int32_t>(mtpMode_));
            service->StopService();
        }
        mtpMode_ = mode;
        if (mode == MtpMode::MTP_MODE) {
            MtpFileObserver::GetInstance().StartFileInotify();
            MtpStoreObserver::StartObserver();
        }
        service->StartService();
        isMtpServiceRunning = true;
    }
}

void MtpManager::StopMtpService()
{
    MEDIA_INFO_LOG("MtpManager::StopMtpService is called");
    {
        std::unique_lock lock(mutex_);
        CHECK_AND_RETURN_INFO_LOG(isMtpServiceRunning.load(),
            "MtpManager::StopMtpService -- service is already stopped");
        auto service = GetMtpService();
        CHECK_AND_RETURN_LOG(service != nullptr, "MtpManager mtpServicePtr is nullptr");
        MtpDfxReporter::GetInstance().NotifyDoDfXReporter(static_cast<int32_t>(mtpMode_));
        mtpMode_ = MtpMode::NONE_MODE;
        service->StopService();
        isMtpServiceRunning = false;
    }
}

void MtpManager::RegisterMtpParamListener()
{
    MEDIA_INFO_LOG("RegisterMTPParamListener");
    WatchParameter(MTP_SERVER_DISABLE, OnMtpParamDisableChanged, this);
}

void MtpManager::RemoveMtpParamListener()
{
    MEDIA_INFO_LOG("RemoveMtpParamListener");
    RemoveParameterWatcher(MTP_SERVER_DISABLE, OnMtpParamDisableChanged, this);
}

void MtpManager::OnMtpParamDisableChanged(const char *key, const char *value, void *context)
{
    bool cond = (key == nullptr || value == nullptr);
    CHECK_AND_RETURN_LOG(!cond, "OnMtpParamDisableChanged return invalid value");
    MEDIA_INFO_LOG("OnMTPParamDisable, key = %{public}s, value = %{public}s", key, value);
    CHECK_AND_RETURN_INFO_LOG(strcmp(key, MTP_SERVER_DISABLE) == 0, "event key mismatch");
    MtpManager *instance = reinterpret_cast<MtpManager*>(context);
    std::string param(MTP_SERVER_DISABLE);
    bool mtpDisable = system::GetBoolParameter(param, false);
    if (!mtpDisable) {
        int32_t funcs = 0;
        int ret = OHOS::USB::UsbSrvClient::GetInstance().GetCurrentFunctions(funcs);
        MEDIA_INFO_LOG("OnMtpparamDisableChanged GetCurrentFunction = %{public}d ret = %{public}d", funcs, ret);
        CHECK_AND_RETURN_INFO_LOG(ret != 0, "OnMtpparamDisableChanged GetCurrentFunction failed");
        uint32_t unsignedFuncs = static_cast<uint32_t>(funcs);
        if (unsignedFuncs && USB::UsbSrvSupport::Function::FUNCTION_MTP) {
            instance->StartMtpService(MtpMode::MTP_MODE);
            return;
        }
    } else {
        MEDIA_INFO_LOG("MTP Manager not init");
        int32_t currentFunctions_ = USB_FUNTION_STORAGE;
        int ret = OHOS::USB::UsbSrvClient::GetInstance().GetCurrentFunctions(currentFunctions_);
        if (ret == 0) {
            currentFunctions_ = static_cast<uint32_t>(currentFunctions_) & (~USB_FUNTION_MTP) & (~USB_FUNTION_PTP);
            currentFunctions_ = currentFunctions_ == 0 ? USB_FUNTION_STORAGE : currentFunctions_;
            MEDIA_INFO_LOG("start to execute disconnect task");
            // 调用内核接口，将MTP或PTP端口切换为HDC接口
            OHOS::USB::UsbSrvClient::GetInstance().SetCurrentFunctions(currentFunctions_);
        }
        instance->StopMtpService();
    }
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS

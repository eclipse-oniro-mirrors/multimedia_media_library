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

#include "file_monitor_wrapper.h"

#include <dlfcn.h>
#include "media_log.h"

namespace OHOS::Media {
    FileMonitorWrapper::FileMonitorWrapper() : handler_(nullptr), createFileMonitorProxyFunc_(nullptr),
        realseFileMonitorProxyFunc_(nullptr)
    {
        handler_ = dlopen("libfile_monitor_ipc_interface.z.so", RTLD_NOW);
        CHECK_AND_RETURN_LOG(handler_ != nullptr, "Not find file_monitor_ipc_interfacelib.");

        createFileMonitorProxyFunc_  =
            reinterpret_cast<CreateFileMonitorProxyFunc>(dlsym(handler_, "CreateFileMonitorProxy"));
        realseFileMonitorProxyFunc_ =
            reinterpret_cast<RealseFileMonitorProxyFunc>(dlsym(handler_, "RealseFileMonitorProxy"));
        if (createFileMonitorProxyFunc_ == nullptr || realseFileMonitorProxyFunc_ == nullptr) {
            MEDIA_ERR_LOG("Not find CreateFileMonitorProxy or RealseFileMonitoProxy func: %{public}s", dlerror());
            createFileMonitorProxyFunc_ = nullptr;
            realseFileMonitorProxyFunc_ = nullptr;
            dlclose(handler_);
            handler_ = nullptr;
        }
    }

    FileMonitorWrapper::~FileMonitorWrapper()
    {
        if (handler_ != nullptr) {
            MEDIA_INFO_LOG("Close file_monitor_ipc_interface lib.");
            createFileMonitorProxyFunc_ = nullptr;
            realseFileMonitorProxyFunc_ = nullptr;
            dlclose(handler_);
            handler_ = nullptr;
        }
    }

    FileMonitorWrapper& FileMonitorWrapper::GetInstance()
    {
        static FileMonitorWrapper instance;
        return instance;
    }

    FileMonitorService::FileMonitorProxy* FileMonitorWrapper::CreateFileMonitorProxy(int32_t tableID)
    {
        CHECK_AND_RETURN_RET_LOG(createFileMonitorProxyFunc_ != nullptr, nullptr,
            "CreateFileMonitorProxy failed, func_ is nullptr");
        return createFileMonitorProxyFunc_(tableID);
    }

    void FileMonitorWrapper::RealseFileMonitorProxy(FileMonitorService::FileMonitorProxy* fileMonitorProxy)
    {
        CHECK_AND_RETURN_LOG(realseFileMonitorProxyFunc_ != nullptr, " RealseFileMonitorProxy faild, func is nullptr");
        realseFileMonitorProxyFunc_(fileMonitorProxy);
    }
} // OHOS::Media
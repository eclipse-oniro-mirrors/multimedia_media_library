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
#ifndef MEDIA_FILE_MONITOR_PROXY_WRAPPER_H
#define MEDIA_FILE_MONITOR_PROXY_WRAPPER_H

#include <cstdint>
#include <memory>
#include "file_monitor_wrapper.h"

#include "medialibrary_errno.h"
#include "media_log.h"

namespace OHOS::Media {

class MediaFileMonitorProxyWrapper {
public:
    explicit MediaFileMonitorProxyWrapper(uint64_t type)
    {
        MEDIA_INFO_LOG("enter file monitor proxy wrapper, care type: 0x%{public}llu", type);
        constexpr int32_t tableId = 100000; // media library proxy
        type_ = type;
        fileMonitorProxy_ = FileMonitorWrapper::GetInstance().CreateFileMonitorProxy(tableId);
        if (fileMonitorProxy_ == nullptr) {
            MEDIA_ERR_LOG("create file monitor proxy failed");
            return;
        }
    }

    ~MediaFileMonitorProxyWrapper()
    {
        if (fileMonitorProxy_) {
            fileMonitorProxy_->UnregisteRequest();
            FileMonitorWrapper::GetInstance().RealseFileMonitorProxy(fileMonitorProxy_);
            fileMonitorProxy_ = nullptr;
            MEDIA_INFO_LOG("file monitor proxy unregister");
        }
        MEDIA_INFO_LOG("exit file monitor proxy wrapper");
    }

    int32_t RegisterRequest(const std::shared_ptr<FileMonitorService::FileChangeCallback>& callback)
    {
        if (fileMonitorProxy_) {
            MEDIA_INFO_LOG("file monitor proxy register, type: 0x%{public}llu", type_);
            return fileMonitorProxy_->RegisteRequest(type_, callback);
        }

        MEDIA_ERR_LOG("file monitor proxy unregister");
        return E_ERR;
    }

    int32_t SearchMonitorData(std::vector<FileMonitorService::FileMsgModel> &msgs)
    {
        if (fileMonitorProxy_) {
            return fileMonitorProxy_->SearchRequest(1, type_, msgs);
        }
        MEDIA_ERR_LOG("file monitor proxy is null");
        return E_ERR;
    }

    int32_t UpdateRequest(const std::vector<int32_t>& ids)
    {
        if (fileMonitorProxy_) {
            return fileMonitorProxy_->UpdateRequest(ids);
        }
        return E_OK;
    }

private:
    FileMonitorService::FileMonitorProxy *fileMonitorProxy_{nullptr};
    uint64_t type_{0};
};

}
#endif // MEDIA_FILE_MONITOR_PROXY_WRAPPER_H

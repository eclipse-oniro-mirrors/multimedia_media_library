/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DOWNLOAD_CONTROLLER_SERVICE_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DOWNLOAD_CONTROLLER_SERVICE_H

#include <string>
#include <vector>
#include <map>

#include "message_parcel.h"
#include "datashare_stub.h"
#include "rdb_store.h"
#include "sys_utils.h"
#include "user_define_ipc.h"
#include "i_media_controller_service.h"
#include "cloud_media_operation_code.h"
#include "medialibrary_errno.h"
#include "cloud_media_download_controller_processor.h"
#include "cloud_media_download_service.h"
#include "cloud_media_define.h"

namespace OHOS::Media::CloudSync {
class EXPORT CloudMediaDownloadControllerService : public IPC::IMediaControllerService {
private:
    int32_t GetDownloadThms(MessageParcel &data, MessageParcel &reply);
    int32_t GetDownloadThmNum(MessageParcel &data, MessageParcel &reply);
    int32_t GetDownloadThmsByUri(MessageParcel &data, MessageParcel &reply);
    int32_t OnDownloadThms(MessageParcel &data, MessageParcel &reply);
    int32_t GetDownloadAsset(MessageParcel &data, MessageParcel &reply);
    int32_t OnDownloadAsset(MessageParcel &data, MessageParcel &reply);
    int32_t OnDownloadLakeAsset(MessageParcel &data, MessageParcel &reply);

private:
    using RequestHandle = int32_t (CloudMediaDownloadControllerService::*)(MessageParcel &, MessageParcel &);
    static const std::map<uint32_t, RequestHandle> HANDLERS = {
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_DOWNLOAD_THM),
            &CloudMediaDownloadControllerService::GetDownloadThms},
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_DOWNLOAD_THM_NUM),
            &CloudMediaDownloadControllerService::GetDownloadThmNum},
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_DOWNLOAD_THM_BY_URI),
            &CloudMediaDownloadControllerService::GetDownloadThmsByUri},
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_ON_DOWNLOAD_THMS),
            &CloudMediaDownloadControllerService::OnDownloadThms},
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_DOWNLOAD_ASSET),
            &CloudMediaDownloadControllerService::GetDownloadAsset},
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_ON_DOWNLOAD_ASSET),
            &CloudMediaDownloadControllerService::OnDownloadAsset},
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_ON_DOWNLOAD_LAKE_ASSET),
            &CloudMediaDownloadControllerService::OnDownloadLakeAsset},
    };

public:
    virtual ~CloudMediaDownloadControllerService() = default;
    bool Accept(uint32_t code) override
    {
        return this->HANDLERS.find(code) != this->HANDLERS.end();
    }
    int32_t OnRemoteRequest(
        uint32_t code, MessageParcel &data, MessageParcel &reply, OHOS::Media::IPC::IPCContext &context) override
    {
        auto it = this->HANDLERS.find(code);
        CHECK_AND_RETURN_RET(it != this->HANDLERS.end(), IPC::UserDefineIPC().WriteResponseBody(reply, E_IPC_SEVICE_NOT_FOUND));
        SysUtils::SlowDown();
        return (this->*(it->second))(data, reply);
    }
    int32_t GetPermissionPolicy(
        uint32_t code, std::vector<std::vector<PermissionType>> &permissionPolicy, bool &isBypass) override
    {
        permissionPolicy = {{CLOUD_READ, CLOUD_WRITE}};
        return E_SUCCESS;
    }

private:
    CloudMediaDownloadControllerProcessor processor_;
    CloudMediaDownloadService service_;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DOWNLOAD_CONTROLLER_SERVICE_H
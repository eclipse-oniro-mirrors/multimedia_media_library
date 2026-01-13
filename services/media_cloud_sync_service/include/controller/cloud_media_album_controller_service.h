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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_ALBUM_CONTROLLER_SERVICE_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_ALBUM_CONTROLLER_SERVICE_H

#include <string>
#include <vector>
#include <map>

#include "message_parcel.h"
#include "datashare_stub.h"
#include "sys_utils.h"
#include "i_media_controller_service.h"
#include "cloud_media_operation_code.h"
#include "medialibrary_errno.h"
#include "media_column.h"
#include "media_resp_vo.h"
#include "media_empty_obj_vo.h"
#include "user_define_ipc.h"
#include "cloud_media_album_service.h"
#include "cloud_media_album_controller_processor.h"
#include "cloud_media_define.h"
#include "cloud_media_context.h"

namespace OHOS::Media::CloudSync {
class EXPORT CloudMediaAlbumControllerService : public IPC::IMediaControllerService {
private:
    int32_t OnFetchRecords(MessageParcel &data, MessageParcel &reply);
    int32_t OnDentryFileInsert(MessageParcel &data, MessageParcel &reply);
    int32_t GetCreatedRecords(MessageParcel &data, MessageParcel &reply);
    int32_t GetMetaModifiedRecords(MessageParcel &data, MessageParcel &reply);
    int32_t GetDeletedRecords(MessageParcel &data, MessageParcel &reply);
    int32_t OnCreateRecords(MessageParcel &data, MessageParcel &reply);
    int32_t OnMdirtyRecords(MessageParcel &data, MessageParcel &reply);
    int32_t OnFdirtyRecords(MessageParcel &data, MessageParcel &reply);
    int32_t OnDeleteRecords(MessageParcel &data, MessageParcel &reply);
    int32_t OnCopyRecords(MessageParcel &data, MessageParcel &reply);
    int32_t OnStartSync(MessageParcel &data, MessageParcel &reply);
    int32_t OnCompleteSync(MessageParcel &data, MessageParcel &reply);
    int32_t OnCompletePull(MessageParcel &data, MessageParcel &reply);
    int32_t OnCompletePush(MessageParcel &data, MessageParcel &reply);
    int32_t OnCompleteCheck(MessageParcel &data, MessageParcel &reply);

private:
    using RequestHandle = int32_t (CloudMediaAlbumControllerService::*)(MessageParcel &, MessageParcel &);
    const std::map<uint32_t, RequestHandle> HANDLERS = {
        {static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_FETCH_RECORDS),
            &CloudMediaAlbumControllerService::OnFetchRecords},
        {static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_DENTRY_FILE_INSERT),
            &CloudMediaAlbumControllerService::OnDentryFileInsert},
        {static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_GET_CREATED_RECORDS),
            &CloudMediaAlbumControllerService::GetCreatedRecords},
        {static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_GET_META_MODIFIED_RECORDS),
            &CloudMediaAlbumControllerService::GetMetaModifiedRecords},
        {static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_GET_DELETED_RECORDS),
            &CloudMediaAlbumControllerService::GetDeletedRecords},
        {static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_CREATE_RECORDS),
            &CloudMediaAlbumControllerService::OnCreateRecords},
        {static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_MDIRTY_RECORDS),
            &CloudMediaAlbumControllerService::OnMdirtyRecords},
        {static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_FDIRTY_RECORDS),
            &CloudMediaAlbumControllerService::OnFdirtyRecords},
        {static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_DELETE_RECORDS),
            &CloudMediaAlbumControllerService::OnDeleteRecords},
        {static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_COPY_RECORDS),
            &CloudMediaAlbumControllerService::OnCopyRecords},
        {static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_START_SYNC),
            &CloudMediaAlbumControllerService::OnStartSync},
        {static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_COMPLETE_SYNC),
            &CloudMediaAlbumControllerService::OnCompleteSync},
        {static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_COMPLETE_PULL),
            &CloudMediaAlbumControllerService::OnCompletePull},
        {static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_COMPLETE_PUSH),
            &CloudMediaAlbumControllerService::OnCompletePush},
        {static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_COMPLETE_CHECK),
            &CloudMediaAlbumControllerService::OnCompleteCheck},
    };

public:
    virtual ~CloudMediaAlbumControllerService() = default;
    bool Accept(uint32_t code) override
    {
        return this->HANDLERS.find(code) != this->HANDLERS.end();
    }
    int32_t OnRemoteRequest(
        uint32_t code, MessageParcel &data, MessageParcel &reply, OHOS::Media::IPC::IPCContext &context) override
    {
        auto headerMap = context.GetHeader();
        auto headerIt = headerMap.find(PhotoColumn::CLOUD_TYPE);
        if (headerIt != headerMap.end()) {
            int32_t cloudType = std::atoi(headerIt->second.c_str());
            CloudMediaContext::GetInstance().SetCloudType(cloudType);
        }
        auto it = this->HANDLERS.find(code);
        if (!this->Accept(code) || it == this->HANDLERS.end()) {
            return IPC::UserDefineIPC().WriteResponseBody(reply, E_IPC_SEVICE_NOT_FOUND);
        }
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
    CloudMediaAlbumService albumService_;
    CloudMediaAlbumControllerProcessor processor_;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_ALBUM_CONTROLLER_SERVICE_H
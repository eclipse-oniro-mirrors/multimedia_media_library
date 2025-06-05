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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_PHOTO_CONTROLLER_SERVICE_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_PHOTO_CONTROLLER_SERVICE_H

#include <string>
#include <vector>
#include <map>

#include "message_parcel.h"
#include "datashare_stub.h"
#include "sys_utils.h"
#include "i_media_controller_service.h"
#include "cloud_media_operation_code.h"
#include "medialibrary_errno.h"
#include "media_resp_vo.h"
#include "media_empty_obj_vo.h"
#include "user_define_ipc.h"
#include "cloud_media_photos_service.h"
#include "cloud_media_photo_controller_processor.h"
#include "cloud_media_define.h"

namespace OHOS::Media::CloudSync {
class EXPORT CloudMediaPhotoControllerService : public IPC::IMediaControllerService {
private:
    void OnFetchRecords(MessageParcel &data, MessageParcel &reply);
    void OnDentryFileInsert(MessageParcel &data, MessageParcel &reply);
    void GetCreatedRecords(MessageParcel &data, MessageParcel &reply);
    void GetMetaModifiedRecords(MessageParcel &data, MessageParcel &reply);
    void GetFileModifiedRecords(MessageParcel &data, MessageParcel &reply);
    void GetDeletedRecords(MessageParcel &data, MessageParcel &reply);
    void GetCopyRecords(MessageParcel &data, MessageParcel &reply);
    void GetCheckRecords(MessageParcel &data, MessageParcel &reply);
    void OnCreateRecords(MessageParcel &data, MessageParcel &reply);
    void OnMdirtyRecords(MessageParcel &data, MessageParcel &reply);
    void OnFdirtyRecords(MessageParcel &data, MessageParcel &reply);
    void OnDeleteRecords(MessageParcel &data, MessageParcel &reply);
    void OnCopyRecords(MessageParcel &data, MessageParcel &reply);
    void GetRetryRecords(MessageParcel &data, MessageParcel &reply);
    void OnStartSync(MessageParcel &data, MessageParcel &reply);
    void OnCompleteSync(MessageParcel &data, MessageParcel &reply);
    void OnCompletePull(MessageParcel &data, MessageParcel &reply);
    void OnCompletePush(MessageParcel &data, MessageParcel &reply);
    void OnCompleteCheck(MessageParcel &data, MessageParcel &reply);
    void ReportFailure(MessageParcel &data, MessageParcel &reply);

private:
    using RequestHandle = void (CloudMediaPhotoControllerService::*)(MessageParcel &, MessageParcel &);
    const std::map<uint32_t, RequestHandle> HANDLERS = {
        {static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_FETCH_RECORDS),
            &CloudMediaPhotoControllerService::OnFetchRecords},
        {static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_DENTRY_FILE_INSERT),
            &CloudMediaPhotoControllerService::OnDentryFileInsert},
        {static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_CREATED_RECORDS),
            &CloudMediaPhotoControllerService::GetCreatedRecords},
        {static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_META_MODIFIED_RECORDS),
            &CloudMediaPhotoControllerService::GetMetaModifiedRecords},
        {static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_FILE_MODIFIED_RECORDS),
            &CloudMediaPhotoControllerService::GetFileModifiedRecords},
        {static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_DELETED_RECORDS),
            &CloudMediaPhotoControllerService::GetDeletedRecords},
        {static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_COPY_RECORDS),
            &CloudMediaPhotoControllerService::GetCopyRecords},
        {static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_CHECK_RECORDS),
            &CloudMediaPhotoControllerService::GetCheckRecords},
        {static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_CREATE_RECORDS),
            &CloudMediaPhotoControllerService::OnCreateRecords},
        {static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_MDIRTY_RECORDS),
            &CloudMediaPhotoControllerService::OnMdirtyRecords},
        {static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_FDIRTY_RECORDS),
            &CloudMediaPhotoControllerService::OnFdirtyRecords},
        {static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_DELETE_RECORDS),
            &CloudMediaPhotoControllerService::OnDeleteRecords},
        {static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_COPY_RECORDS),
            &CloudMediaPhotoControllerService::OnCopyRecords},
        {static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_RETRY_RECORDS),
            &CloudMediaPhotoControllerService::GetRetryRecords},
        {static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_START_SYNC),
            &CloudMediaPhotoControllerService::OnStartSync},
        {static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_COMPLETE_SYNC),
            &CloudMediaPhotoControllerService::OnCompleteSync},
        {static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_COMPLETE_PULL),
            &CloudMediaPhotoControllerService::OnCompletePull},
        {static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_COMPLETE_PUSH),
            &CloudMediaPhotoControllerService::OnCompletePush},
        {static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_COMPLETE_CHECK),
            &CloudMediaPhotoControllerService::OnCompleteCheck},
        {static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_REPORT_FAILURE),
            &CloudMediaPhotoControllerService::ReportFailure},
    };

public:
    virtual ~CloudMediaPhotoControllerService() = default;
    bool Accept(uint32_t code) override
    {
        return this->HANDLERS.find(code) != this->HANDLERS.end();
    }
    void OnRemoteRequest(
        uint32_t code, MessageParcel &data, MessageParcel &reply, OHOS::Media::IPC::IPCContext &context) override
    {
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
        return E_SUCCESS;
    }

private:
    CloudMediaPhotoControllerProcessor processor_;
    CloudMediaPhotosService photosService_;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_PHOTO_CONTROLLER_SERVICE_H
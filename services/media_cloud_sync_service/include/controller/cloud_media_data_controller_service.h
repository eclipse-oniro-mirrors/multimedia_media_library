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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DATA_CONTROLLER_SERVICE_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DATA_CONTROLLER_SERVICE_H

#include <string>
#include <vector>
#include <map>

#include "message_parcel.h"
#include "datashare_stub.h"
#include "sys_utils.h"
#include "photos_dto.h"
#include "photos_vo.h"
#include "rdb_store.h"
#include "i_media_controller_service.h"
#include "cloud_media_operation_code.h"
#include "medialibrary_errno.h"
#include "media_resp_vo.h"
#include "media_empty_obj_vo.h"
#include "user_define_ipc.h"
#include "cloud_media_data_controller_processor.h"
#include "cloud_media_data_service.h"
#include "cloud_media_define.h"
#include "cloud_media_enhance_service.h"

namespace OHOS::Media::CloudSync {
class EXPORT CloudMediaDataControllerService : public IPC::IMediaControllerService {
private:
    int32_t UpdateDirty(MessageParcel &data, MessageParcel &reply);
    int32_t UpdatePosition(MessageParcel &data, MessageParcel &reply);
    int32_t UpdatePosWithType(MessageParcel &data, MessageParcel &reply);
    int32_t UpdateFileSourceType(MessageParcel &data, MessageParcel &reply);
    int32_t UpdateThmStatus(MessageParcel &data, MessageParcel &reply);
    int32_t GetAgingFile(MessageParcel &data, MessageParcel &reply);
    int32_t GetActiveAgingFile(MessageParcel &data, MessageParcel &reply);
    int32_t GetVideoToCache(MessageParcel &data, MessageParcel &reply);
    int32_t GetFilePosStat(MessageParcel &data, MessageParcel &reply);
    int32_t GetCloudThmStat(MessageParcel &data, MessageParcel &reply);
    int32_t GetDirtyTypeStat(MessageParcel &data, MessageParcel &reply);
    int32_t UpdateLocalFileDirty(MessageParcel &data, MessageParcel &reply);
    int32_t UpdateSyncStatus(MessageParcel &data, MessageParcel &reply);
    int32_t GetCloudSyncUnPreparedData(MessageParcel &data, MessageParcel &reply);
    int32_t SubmitCloudSyncPreparedDataTask(MessageParcel &data, MessageParcel &reply);
    int32_t CheckAndFixAlbum(MessageParcel &data, MessageParcel &reply);
    int32_t QueryData(MessageParcel &data, MessageParcel &reply);
    int32_t UpdateData(MessageParcel &data, MessageParcel &reply);

private:
    using RequestHandle = int32_t (CloudMediaDataControllerService::*)(MessageParcel &, MessageParcel &);
    static const std::map<uint32_t, RequestHandle> HANDLERS = {
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_DIRTY_FOR_CLOUD_CHECK),
            &CloudMediaDataControllerService::UpdateDirty},
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_POSITION_FOR_CLOUD_CHECK),
            &CloudMediaDataControllerService::UpdatePosition},
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_POSITION_WITH_TYPE_FOR_CLOUD_CHECK),
            &CloudMediaDataControllerService::UpdateFileSourceType},
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_FILE_SOURCE_TYPE),
            &CloudMediaDataControllerService::UpdatePosWithType},
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_THM_STATUS_FOR_CLOUD_CHECK),
            &CloudMediaDataControllerService::UpdateThmStatus},
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_VIDEO_TO_CACHE),
            &CloudMediaDataControllerService::GetVideoToCache},
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_FILE_POS_STAT),
            &CloudMediaDataControllerService::GetFilePosStat},
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_CLOUD_THM_STAT),
            &CloudMediaDataControllerService::GetCloudThmStat},
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_DIRTY_TYPE_STAT),
            &CloudMediaDataControllerService::GetDirtyTypeStat},
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_AGING_ASSET),
            &CloudMediaDataControllerService::GetAgingFile},
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_ACTIVE_AGING_ASSET),
            &CloudMediaDataControllerService::GetActiveAgingFile},
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_LOCAL_FILE_DIRTY),
            &CloudMediaDataControllerService::UpdateLocalFileDirty},
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_SYNC_STATUS),
            &CloudMediaDataControllerService::UpdateSyncStatus},
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_CLOUD_SYNC_UNPREPARED_DATA),
            &CloudMediaDataControllerService::GetCloudSyncUnPreparedData},
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_SUBMIT_CLOUD_SYNC_UNPREPARED_DATA_TASK),
            &CloudMediaDataControllerService::SubmitCloudSyncPreparedDataTask},
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_CHECK_AND_FIX_ALBUM),
            &CloudMediaDataControllerService::CheckAndFixAlbum},
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_QUERY_DATA),
            &CloudMediaDataControllerService::QueryData},
        {static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_DATA),
            &CloudMediaDataControllerService::UpdateData},
    };

public:
    virtual ~CloudMediaDataControllerService() = default;
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
    CloudMediaDataControllerProcessor processor_;
    CloudMediaDataService dataService_;
    CloudMediaEnhanceService enhanceService_;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DATA_CONTROLLER_SERVICE_H
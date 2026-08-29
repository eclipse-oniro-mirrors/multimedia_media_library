/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_SHARE_PHOTO_DATA_CONTROLLER_SERVICE_H
#define OHOS_MEDIA_SHARE_PHOTO_DATA_CONTROLLER_SERVICE_H

#include <map>

#include "message_parcel.h"
#include "i_media_controller_service.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "user_define_ipc.h"

#include "cloud_media_operation_code.h"
#include "sys_utils.h"
#include "media_share_photo_data_service.h"

namespace OHOS::Media::ShareAlbum {

class MediaSharePhotoDataControllerService : public IPC::IMediaControllerService {
public:
    virtual ~MediaSharePhotoDataControllerService() = default;

    bool Accept(uint32_t code) override
    {
        return this->HANDLERS.find(code) != this->HANDLERS.end();
    }

    int32_t OnRemoteRequest(
        uint32_t code, MessageParcel &data, MessageParcel &reply, OHOS::Media::IPC::IPCContext &context) override
    {
        auto it = this->HANDLERS.find(code);
        CHECK_AND_RETURN_RET(
            it != this->HANDLERS.end(), IPC::UserDefineIPC().WriteResponseBody(reply, E_IPC_SEVICE_NOT_FOUND));
        CloudSync::SysUtils::SlowDown();
        return (this->*(it->second))(data, reply);
    }

    int32_t GetPermissionPolicy(
        uint32_t code, std::vector<std::vector<PermissionType>> &permissionPolicy, bool &isBypass) override
    {
        permissionPolicy = {{CLOUD_READ}};
        return E_SUCCESS;
    }

private:
    int32_t GetShareAlbumOwnerId(MessageParcel &data, MessageParcel &reply);

    using RequestHandle = int32_t (MediaSharePhotoDataControllerService::*)(MessageParcel &, MessageParcel &);
    const std::map<uint32_t, RequestHandle> HANDLERS = {
        {static_cast<uint32_t>(CloudMediaSharePhotoOperationCode::CMD_GET_SHARE_ALBUM_OWNER_ID),
            &MediaSharePhotoDataControllerService::GetShareAlbumOwnerId},
    };

private:
    MediaSharePhotoDataService dataService_;
};

}  // namespace OHOS::Media::ShareAlbum

#endif  // OHOS_MEDIA_SHARE_PHOTO_DATA_CONTROLLER_SERVICE_H

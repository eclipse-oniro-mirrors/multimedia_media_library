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

#ifndef OHOS_MEDIA_CAMERA_CHARACTER_CONTROLLER_SERVICE_H
#define OHOS_MEDIA_CAMERA_CHARACTER_CONTROLLER_SERVICE_H

#include "message_parcel.h"
#include "medialibrary_business_code.h"
#include "i_media_controller_service.h"
#include "user_define_ipc.h"
#include "medialibrary_errno.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

class EXPORT MediaCameraCharacterControllerService : public IPC::IMediaControllerService {
public:
    EXPORT int32_t AddProcessVideo(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t CancelRequest(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t ProcessVideo(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetProgressCallback(MessageParcel &data, MessageParcel &reply);

public:
    virtual ~MediaCameraCharacterControllerService() = default;
    bool Accept(uint32_t code) override;
    int32_t OnRemoteRequest(
        uint32_t code, MessageParcel &data, MessageParcel &reply, OHOS::Media::IPC::IPCContext &context) override;
    int32_t GetPermissionPolicy(
        uint32_t code, std::vector<std::vector<PermissionType>> &permissionPolicy, bool &isBypass) override;
};
} // namespace OHOS::Media
#endif  // OHOS_MEDIA_ASSETS_CONTROLLER_SERVICE_H
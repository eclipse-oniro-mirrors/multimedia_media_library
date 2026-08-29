/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Media_Controller"

#include "media_share_photo_data_controller_service.h"

#include "get_share_album_owner_vo.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "user_define_ipc.h"

namespace OHOS::Media::ShareAlbum {

int32_t MediaSharePhotoDataControllerService::GetShareAlbumOwnerId(MessageParcel &data, MessageParcel &reply)
{
    GetShareAlbumOwnerReqBody req;
    GetShareAlbumOwnerRespBody resp;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, req);
    CHECK_AND_RETURN_RET_LOG(
        ret == E_OK,
        IPC::UserDefineIPC().WriteResponseBody(reply, resp, ret),
        "GetShareAlbumOwnerId Read Req Error");
    std::string shareAlbumOwner;
    ret = this->dataService_.GetShareAlbumOwnerId(req.data, shareAlbumOwner);
    resp.shareAlbumOwner = shareAlbumOwner;
    MEDIA_INFO_LOG("GetShareAlbumOwnerId Resp, shareAlbumOwner:%{public}s", shareAlbumOwner.c_str());
    return IPC::UserDefineIPC().WriteResponseBody(reply, resp, ret);
}

}  // namespace OHOS::Media::ShareAlbum

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
#define MLOG_TAG "Media_Client"

#include "media_share_photo_data_client_handler.h"

#include <string>

#include "cloud_media_operation_code.h"
#include "get_share_album_owner_vo.h"
#include "media_column.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "user_define_ipc_client.h"

namespace OHOS::Media::ShareAlbum {

void MediaSharePhotoDataClientHandler::SetUserId(const int32_t &userId)
{
    this->userId_ = userId;
}

void MediaSharePhotoDataClientHandler::SetTraceId(const std::string &traceId)
{
    this->traceId_ = traceId;
}

void MediaSharePhotoDataClientHandler::SetCloudType(const int32_t cloudType)
{
    this->cloudType_ = cloudType;
    this->header_[PhotoColumn::CLOUD_TYPE] = std::to_string(cloudType);
}

std::unordered_map<std::string, std::string> &MediaSharePhotoDataClientHandler::GetHeader()
{
    return header_;
}

int32_t MediaSharePhotoDataClientHandler::GetShareAlbumOwnerId(std::string data, std::string &ownerId)
{
    GetShareAlbumOwnerReqBody reqBody;
    GetShareAlbumOwnerRespBody respBody;
    reqBody.data = data;
    uint32_t operationCode =
        static_cast<uint32_t>(CloudMediaSharePhotoOperationCode::CMD_GET_SHARE_ALBUM_OWNER_ID);
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId_).SetTraceId(traceId_)
            .SetHeader(GetHeader()).Post(operationCode, reqBody, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to GetShareAlbumOwnerId, ret:%{public}d, data:%{public}s", ret, data.c_str());
        return ret;
    }
    ownerId = respBody.shareAlbumOwner;
    return E_OK;
}

} // namespace OHOS::Media::ShareAlbum

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

#include "media_share_photo_data_client.h"

#include <string>

#include "medialibrary_errno.h"
#include "media_log.h"
#include "media_share_photo_data_client_handler.h"

namespace OHOS::Media::ShareAlbum {

MediaSharePhotoDataClient::MediaSharePhotoDataClient(const int32_t cloudType, const int32_t userId)
    : cloudType_(cloudType), userId_(userId)
{
    this->dataHandler_ = std::make_shared<MediaSharePhotoDataClientHandler>();
    this->dataHandler_->SetCloudType(cloudType);
    this->dataHandler_->SetUserId(userId);
}

void MediaSharePhotoDataClient::SetUserId(const int32_t &userId)
{
    this->userId_ = userId;
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return;
    }
    this->dataHandler_->SetUserId(userId);
}
void MediaSharePhotoDataClient::SetCloudType(const int32_t cloudType)
{
    this->cloudType_ = cloudType;
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return;
    }
    this->dataHandler_->SetCloudType(cloudType);
}

void MediaSharePhotoDataClient::SetTraceId(const std::string &traceId)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return;
    }
    this->dataHandler_->SetTraceId(traceId);
}

int32_t MediaSharePhotoDataClient::GetShareAlbumOwnerId(std::string data, std::string &ownerId)
{
    if (this->dataHandler_ == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return E_IPC_ERR;
    }
    return this->dataHandler_->GetShareAlbumOwnerId(data, ownerId);
}

} // namespace OHOS::Media::ShareAlbum

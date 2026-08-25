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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DATA_HANDLER_FACTORY_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DATA_HANDLER_FACTORY_H

#include <map>
#include <vector>

#include "cloud_share_album_define.h"
#include "cloud_media_client_utils.h"
#include "i_cloud_media_data_handler.h"
#include "cloud_media_album_handler.h"
#include "cloud_media_photo_handler.h"

namespace OHOS::Media::CloudSync {
class CloudMediaDataHandlerFactory {
private:
    const std::map<std::string, std::shared_ptr<ICloudMediaDataHandler>> DATA_HANDLER = {
        {"PhotoAlbum", std::make_shared<CloudMediaAlbumHandler>()},
        {"Photos", std::make_shared<CloudMediaPhotoHandler>()}};

public:  // constructor
    CloudMediaDataHandlerFactory() = default;
    virtual ~CloudMediaDataHandlerFactory() = default;

public:
    std::shared_ptr<ICloudMediaDataHandler> GetDataHandler(const std::string &tableName,
        const int32_t cloudType, const int32_t userId, int32_t sceneType = 0)
    {
        std::shared_ptr<ICloudMediaDataHandler> handler = nullptr;
        auto iter = this->DATA_HANDLER.find(tableName);
        if (iter != this->DATA_HANDLER.end()) {
            handler = iter->second;
        }
        CHECK_AND_RETURN_RET_LOG(handler != nullptr, handler, "handler is null");
        handler->SetCloudType(cloudType);
        handler->SetUserId(userId);
        handler->SetSceneType(sceneType);
        return handler;
    }

    static bool IsPhotoAlbum(const std::string &tableName)
    {
        return tableName == "PhotoAlbum";
    }

    static bool IsPhoto(const std::string &tableName)
    {
        return tableName == "Photos";
    }
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DATA_HANDLER_FACTORY_H
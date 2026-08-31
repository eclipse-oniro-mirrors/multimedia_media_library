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

#ifndef OHOS_MEDIA_SHARE_ALBUM_MEDIA_SHARE_PHOTO_DATA_CLIENT_H
#define OHOS_MEDIA_SHARE_ALBUM_MEDIA_SHARE_PHOTO_DATA_CLIENT_H

#include <cstdint>
#include <memory>
#include <string>

#include "i_media_share_photo_data_client.h"

namespace OHOS::Media::ShareAlbum {

class EXPORT MediaSharePhotoDataClient : public IMediaSharePhotoDataClient {
public:  // constructors
    MediaSharePhotoDataClient(const int32_t cloudType, const int32_t userId);
    virtual ~MediaSharePhotoDataClient() = default;

public:  // getter & setter
    void SetUserId(const int32_t &userId) override;
    void SetTraceId(const std::string &traceId) override;
    void SetCloudType(const int32_t cloudType) override;

public:
    int32_t GetShareAlbumOwnerId(std::string data, std::string &ownerId) override;

private:
    std::shared_ptr<IMediaSharePhotoDataClient> dataHandler_;
    int32_t cloudType_;
    int32_t userId_;
};

} // namespace OHOS::Media::ShareAlbum

#endif // OHOS_MEDIA_SHARE_ALBUM_MEDIA_SHARE_PHOTO_DATA_CLIENT_H

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

#ifndef OHOS_MEDIA_SHARE_ALBUM_I_MEDIA_SHARE_PHOTO_DATA_CLIENT_H
#define OHOS_MEDIA_SHARE_ALBUM_I_MEDIA_SHARE_PHOTO_DATA_CLIENT_H

#include <cstdint>
#include <string>

#define EXPORT __attribute__ ((visibility ("default")))

namespace OHOS::Media::ShareAlbum {
class EXPORT IMediaSharePhotoDataClient {
public:  // constructors & destructors
    virtual ~IMediaSharePhotoDataClient() = default;

public:  // getter & setter
    virtual void SetUserId(const int32_t &userId) = 0;
    virtual void SetTraceId(const std::string &traceId) = 0;
    virtual void SetCloudType(const int32_t cloudType) = 0;

public:
    virtual int32_t GetShareAlbumOwnerId(std::string data, std::string &ownerId) = 0;
};
}  // namespace OHOS::Media::ShareAlbum
#endif  // OHOS_MEDIA_SHARE_ALBUM_I_MEDIA_SHARE_PHOTO_DATA_CLIENT_H

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

#ifndef OHOS_MEDIA_SHARE_ALBUM_GET_SHARE_ALBUM_OWNER_VO_H
#define OHOS_MEDIA_SHARE_ALBUM_GET_SHARE_ALBUM_OWNER_VO_H

#include <string>

#include "i_media_parcelable.h"

#ifndef EXPORT
#define EXPORT __attribute__ ((visibility ("default")))
#endif

namespace OHOS::Media::ShareAlbum {

class EXPORT GetShareAlbumOwnerReqBody : public IPC::IMediaParcelable {
public:
    std::string data;

public:
    virtual ~GetShareAlbumOwnerReqBody() = default;
    bool Marshalling(MessageParcel &parcel) const override;
    bool Unmarshalling(MessageParcel &parcel) override;

public:
    std::string ToString() const;
};

class EXPORT GetShareAlbumOwnerRespBody : public IPC::IMediaParcelable {
public:
    std::string shareAlbumOwner;

public:
    virtual ~GetShareAlbumOwnerRespBody() = default;
    bool Marshalling(MessageParcel &parcel) const override;
    bool Unmarshalling(MessageParcel &parcel) override;

public:
    std::string ToString() const;
};

} // namespace OHOS::Media::ShareAlbum

#endif // OHOS_MEDIA_SHARE_ALBUM_GET_SHARE_ALBUM_OWNER_VO_H
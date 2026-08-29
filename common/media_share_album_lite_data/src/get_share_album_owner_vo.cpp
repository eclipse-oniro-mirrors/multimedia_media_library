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

#include "get_share_album_owner_vo.h"

#include <sstream>

namespace OHOS::Media::ShareAlbum {

bool GetShareAlbumOwnerReqBody::Marshalling(MessageParcel &parcel) const
{
    return parcel.WriteString(data);
}

bool GetShareAlbumOwnerReqBody::Unmarshalling(MessageParcel &parcel)
{
    return parcel.ReadString(data);
}

bool GetShareAlbumOwnerRespBody::Marshalling(MessageParcel &parcel) const
{
    return parcel.WriteString(shareAlbumOwner);
}

bool GetShareAlbumOwnerRespBody::Unmarshalling(MessageParcel &parcel)
{
    return parcel.ReadString(shareAlbumOwner);
}

std::string GetShareAlbumOwnerReqBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"data\": \"" << this->data << "\""
       << "}";
    return ss.str();
}

std::string GetShareAlbumOwnerRespBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"shareAlbumOwner\": \"" << this->shareAlbumOwner << "\""
       << "}";
    return ss.str();
}

} // namespace OHOS::Media::ShareAlbum

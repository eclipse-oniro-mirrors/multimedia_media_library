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

#include "set_share_album_name_vo.h"
#include "media_log.h"

namespace OHOS::Media {

bool SetShareAlbumNameReqBody::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->albumId), false, "read albumId failed");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->owner), false, "read owner failed");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->albumName), false, "read albumName failed");
    return true;
}

bool SetShareAlbumNameReqBody::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->albumId), false, "write albumId failed");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->owner), false, "write owner failed");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->albumName), false, "write albumName failed");
    return true;
}
} // namespace OHOS::Media

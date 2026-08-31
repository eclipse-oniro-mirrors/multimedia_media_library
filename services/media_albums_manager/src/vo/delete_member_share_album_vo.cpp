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

#include "delete_member_share_album_vo.h"
#include "media_log.h"

namespace OHOS::Media {

static const int32_t MAX_ALBUM_IDS_SIZE = 10000;

bool DeleteMemberShareAlbumReqBody::Unmarshalling(MessageParcel &parcel)
{
    if (!parcel.ReadString(this->owner)) {
        return false;
    }
    int32_t size = 0;
    if (!parcel.ReadInt32(size)) {
        return false;
    }
    if (size < 0 || size > MAX_ALBUM_IDS_SIZE) {
        MEDIA_ERR_LOG("invalid albumIds size=%{public}d", size);
        return false;
    }
    this->albumIds.reserve(size);
    for (int32_t i = 0; i < size; i++) {
        int32_t albumId = 0;
        if (!parcel.ReadInt32(albumId)) {
            return false;
        }
        this->albumIds.push_back(albumId);
    }
    return true;
}

bool DeleteMemberShareAlbumReqBody::Marshalling(MessageParcel &parcel) const
{
    if (!parcel.WriteString(this->owner)) {
        return false;
    }
    if (!parcel.WriteInt32(static_cast<int32_t>(this->albumIds.size()))) {
        return false;
    }
    for (const auto &albumId : this->albumIds) {
        if (!parcel.WriteInt32(albumId)) {
            return false;
        }
    }
    return true;
}

} // namespace OHOS::Media

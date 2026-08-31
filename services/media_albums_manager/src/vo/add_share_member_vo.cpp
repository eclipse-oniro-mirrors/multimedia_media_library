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

#include "add_share_member_vo.h"

namespace OHOS::Media {

bool AddShareMemberReqBody::Unmarshalling(MessageParcel &parcel)
{
    return parcel.ReadInt32(this->albumId) &&
        parcel.ReadString(this->owner) &&
        parcel.ReadString(this->member) &&
        parcel.ReadInt32(this->status);
}

bool AddShareMemberReqBody::Marshalling(MessageParcel &parcel) const
{
    return parcel.WriteInt32(this->albumId) &&
        parcel.WriteString(this->owner) &&
        parcel.WriteString(this->member) &&
        parcel.WriteInt32(this->status);
}

bool UpdateShareMemberStatusReqBody::Unmarshalling(MessageParcel &parcel)
{
    return parcel.ReadInt32(this->albumId) &&
        parcel.ReadString(this->owner) &&
        parcel.ReadString(this->member) &&
        parcel.ReadInt32(this->status);
}

bool UpdateShareMemberStatusReqBody::Marshalling(MessageParcel &parcel) const
{
    return parcel.WriteInt32(this->albumId) &&
        parcel.WriteString(this->owner) &&
        parcel.WriteString(this->member) &&
        parcel.WriteInt32(this->status);
}

} // namespace OHOS::Media

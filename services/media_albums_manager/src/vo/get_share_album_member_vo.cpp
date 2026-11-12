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

#include "get_share_album_member_vo.h"

#include "media_itypes_utils.h"
#include "media_log.h"

namespace OHOS::Media {

bool MemberInfoInner::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteString(this->member);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteInt32(this->status);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool MemberInfoInner::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadString(this->member);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadInt32(this->status);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool GetShareAlbumMemberReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteString(this->owner);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteInt32(this->albumId);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool GetShareAlbumMemberReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadString(this->owner);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadInt32(this->albumId);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool GetShareAlbumMemberRespBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteString(this->owner);
    CHECK_AND_RETURN_RET(status, status);
    status = IPC::ITypeMediaUtil::MarshallingParcelable<MemberInfoInner>(this->members, parcel);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool GetShareAlbumMemberRespBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadString(this->owner);
    CHECK_AND_RETURN_RET(status, status);
    status = IPC::ITypeMediaUtil::UnmarshallingParcelable<MemberInfoInner>(this->members, parcel);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

} // namespace OHOS::Media

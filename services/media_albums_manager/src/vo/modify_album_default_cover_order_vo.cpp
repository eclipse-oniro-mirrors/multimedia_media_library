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

#include "modify_album_default_cover_order_vo.h"
#include "message_parcel.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
static const size_t MAX_SIZE = 1024 * 1024;

bool ModifyAlbumDefaultCoverOrderReqBody::Marshalling(MessageParcel &parcel) const
{
    size_t size = coverOrderInfos.size();
    CHECK_AND_RETURN_RET(size >= 1 && size <= MAX_SIZE, false);
    bool status = parcel.WriteInt32(static_cast<int32_t>(size));
    CHECK_AND_RETURN_RET(status, status);
    for (const auto &info : coverOrderInfos) {
        status = parcel.WriteInt32(info.albumType);
        CHECK_AND_RETURN_RET(status, status);
        status = parcel.WriteInt32(info.albumSubType);
        CHECK_AND_RETURN_RET(status, status);
        status = parcel.WriteString(info.lpath);
        CHECK_AND_RETURN_RET(status, status);
        status = parcel.WriteString(info.orderKey);
        CHECK_AND_RETURN_RET(status, status);
        status = parcel.WriteString(info.orderSubKey);
        CHECK_AND_RETURN_RET(status, status);
        status = parcel.WriteInt32(info.orderType);
        CHECK_AND_RETURN_RET(status, status);
    }
    status = parcel.WriteBool(disable);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteBool(isAsyncRefreshAlbum);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool ModifyAlbumDefaultCoverOrderReqBody::Unmarshalling(MessageParcel &parcel)
{
    int32_t size = 0;
    bool status = parcel.ReadInt32(size);
    CHECK_AND_RETURN_RET(status && size >= 1 && size <= MAX_SIZE, false);
    coverOrderInfos.resize(static_cast<size_t>(size));
    for (auto &info : coverOrderInfos) {
        status = parcel.ReadInt32(info.albumType);
        CHECK_AND_RETURN_RET(status, status);
        status = parcel.ReadInt32(info.albumSubType);
        CHECK_AND_RETURN_RET(status, status);
        status = parcel.ReadString(info.lpath);
        CHECK_AND_RETURN_RET(status, status);
        status = parcel.ReadString(info.orderKey);
        CHECK_AND_RETURN_RET(status, status);
        status = parcel.ReadString(info.orderSubKey);
        CHECK_AND_RETURN_RET(status, status);
        status = parcel.ReadInt32(info.orderType);
        CHECK_AND_RETURN_RET(status, status);
    }
    status = parcel.ReadBool(disable);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadBool(isAsyncRefreshAlbum);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

} // namespace Media
} // namespace OHOS
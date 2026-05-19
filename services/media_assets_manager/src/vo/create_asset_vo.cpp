/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "create_asset_vo.h"

#include "media_log.h"

namespace OHOS::Media {

bool CreateAssetReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadInt32(this->mediaType);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadInt32(this->photoSubtype);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadString(this->title);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadString(this->extension);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadString(this->displayName);
    CHECK_AND_RETURN_RET(status, status);
    return parcel.ReadString(this->cameraShotKey);
}

bool CreateAssetReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteInt32(this->mediaType);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteInt32(this->photoSubtype);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteString(this->title);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteString(this->extension);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteString(this->displayName);
    CHECK_AND_RETURN_RET(status, status);
    return parcel.WriteString(this->cameraShotKey);
}

bool CreateAssetRespBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadInt32(this->fileId);
    CHECK_AND_RETURN_RET(status, status);
    return parcel.ReadString(this->outUri);
}

bool CreateAssetRespBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteInt32(this->fileId);
    CHECK_AND_RETURN_RET(status, status);
    return parcel.WriteString(this->outUri);
}

bool CreateAssetForAppReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadInt64(this->tokenId);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadInt32(this->mediaType);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadInt32(this->photoSubtype);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadString(this->title);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadString(this->extension);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadString(this->displayName);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadString(this->bundleName);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadString(this->packageName);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadString(this->appId);
    CHECK_AND_RETURN_RET(status, status);
    return parcel.ReadString(this->ownerAlbumId);
}

bool CreateAssetForAppReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteInt64(this->tokenId);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteInt32(this->mediaType);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteInt32(this->photoSubtype);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteString(this->title);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteString(this->extension);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteString(this->displayName);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteString(this->bundleName);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteString(this->packageName);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteString(this->appId);
    CHECK_AND_RETURN_RET(status, status);
    return parcel.WriteString(this->ownerAlbumId);
}

bool CreateFileMgrAssetReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadString(this->displayName);
    CHECK_AND_RETURN_RET(status, status);
    return parcel.ReadString(this->ownerAlbumId);
}

bool CreateFileMgrAssetReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteString(this->displayName);
    CHECK_AND_RETURN_RET(status, status);
    return parcel.WriteString(this->ownerAlbumId);
}

bool CreateAssetsWithAlbumReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadInt32(this->mediaType);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadString(this->title);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadString(this->extension);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadString(this->ownerAlbumId);
    CHECK_AND_RETURN_RET(status, status);
    return parcel.ReadBool(this->isRealTimeThumb);
}

bool CreateAssetsWithAlbumReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteInt32(this->mediaType);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteString(this->title);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteString(this->extension);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteString(this->ownerAlbumId);
    CHECK_AND_RETURN_RET(status, status);
    return parcel.WriteBool(this->isRealTimeThumb);
}
} // namespace OHOS::Media
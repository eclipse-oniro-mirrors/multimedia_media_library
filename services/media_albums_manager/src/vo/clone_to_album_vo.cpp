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

#include "clone_to_album_vo.h"

#include "message_parcel.h"
#include "media_log.h"

namespace OHOS::Media {

bool CloneToAlbumReqBody::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteStringVector(assetsArray);
    parcel.WriteInt32(albumId);
    parcel.WriteInt32(albumType);
    parcel.WriteInt32(albumSubType);
    parcel.WriteInt32(mode);
    parcel.WriteInt32(requestId);
    parcel.WriteString(albumLpath);
    parcel.WriteString(targetDir);
    parcel.WriteRemoteObject(progressCallback);
    parcel.WriteString(owner);
    parcel.WriteInt64(shareGroup);
    parcel.WriteString(shareAlbumOwner);
    return true;
}

bool CloneToAlbumReqBody::Unmarshalling(MessageParcel &parcel)
{
    parcel.ReadStringVector(&assetsArray);
    albumId = parcel.ReadInt32();
    albumType = parcel.ReadInt32();
    albumSubType = parcel.ReadInt32();
    mode = parcel.ReadInt32();
    requestId = parcel.ReadInt32();
    albumLpath = parcel.ReadString();
    targetDir = parcel.ReadString();
    progressCallback = parcel.ReadRemoteObject();
    owner = parcel.ReadString();
    shareGroup = parcel.ReadInt64();
    shareAlbumOwner = parcel.ReadString();
    return true;
}
} // namespace OHOS::Media

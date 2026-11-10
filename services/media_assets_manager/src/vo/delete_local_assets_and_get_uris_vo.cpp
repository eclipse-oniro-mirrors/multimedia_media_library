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

#include "delete_local_assets_and_get_uris_vo.h"

#include "media_itypes_utils.h"

namespace OHOS::Media {

bool DeleteLocalAssetsAndGetUrisReqBody::Unmarshalling(MessageParcel &parcel)
{
    return IPC::ITypeMediaUtil::UnmarshalStrVec(this->fileIds, parcel);
}

bool DeleteLocalAssetsAndGetUrisReqBody::Marshalling(MessageParcel &parcel) const
{
    return IPC::ITypeMediaUtil::MarshalStrVec(this->fileIds, parcel);
}

bool DeleteLocalAssetsAndGetUrisRespBody::Unmarshalling(MessageParcel &parcel)
{
    return IPC::ITypeMediaUtil::UnmarshallingParcelable<AssetUriResult>(this->results, parcel);
}

bool DeleteLocalAssetsAndGetUrisRespBody::Marshalling(MessageParcel &parcel) const
{
    return IPC::ITypeMediaUtil::MarshallingParcelable<AssetUriResult>(this->results, parcel);
}
}  // namespace OHOS::Media

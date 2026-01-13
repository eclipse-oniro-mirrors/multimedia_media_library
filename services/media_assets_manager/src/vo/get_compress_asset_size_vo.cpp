/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaGetCompressAssetSizeVo"
#include "get_compress_asset_size_vo.h"
#include <cinttypes>
#include <sstream>

#include "itypes_util.h"
#include "media_log.h"

namespace OHOS::Media {
bool GetCompressAssetSizeReqBody::Unmarshalling(MessageParcel &parcel)
{
    return parcel.ReadStringVector(&this->uris);
}
bool GetCompressAssetSizeReqBody::Marshalling(MessageParcel &parcel) const
{
    return parcel.WriteStringVector(this->uris);
}

bool GetCompressAssetSizeRespBody::Unmarshalling(MessageParcel &parcel)
{
    this->totalSize = parcel.ReadInt64();
    MEDIA_DEBUG_LOG("GetCompressAssetSize read totalSize: %{public}" PRId64, this->totalSize);
    return true;
}
bool GetCompressAssetSizeRespBody::Marshalling(MessageParcel &parcel) const
{
    MEDIA_DEBUG_LOG("GetCompressAssetSize write totalSize: %{public}" PRId64, this->totalSize);
    bool status = parcel.WriteInt64(this->totalSize);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}
} // namespace OHOS::Media
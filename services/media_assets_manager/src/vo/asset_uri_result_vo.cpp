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

#include "asset_uri_result_vo.h"

#include "media_log.h"

namespace OHOS::Media {

bool AssetUriResult::Unmarshalling(MessageParcel &parcel)
{
    if (!parcel.ReadString(this->originalFileId) || !parcel.ReadString(this->resultUri) ||
        !parcel.ReadBool(this->isSuccess)) {
        MEDIA_ERR_LOG("Failed to read the asset uri result.");
        return false;
    }
    return true;
}

bool AssetUriResult::Marshalling(MessageParcel &parcel) const
{
    if (!parcel.WriteString(this->originalFileId) || !parcel.WriteString(this->resultUri) ||
        !parcel.WriteBool(this->isSuccess)) {
        MEDIA_ERR_LOG("Failed to write the asset uri result, originalFileId: %{public}s",
            this->originalFileId.c_str());
        return false;
    }
    return true;
}
}  // namespace OHOS::Media

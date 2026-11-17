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

#ifndef OHOS_MEDIA_ASSETS_MANAGER_ASSET_URI_RESULT_VO_H
#define OHOS_MEDIA_ASSETS_MANAGER_ASSET_URI_RESULT_VO_H

#include <cstdint>
#include <string>

#include "i_media_parcelable.h"

namespace OHOS::Media {
class AssetUriResult : public IPC::IMediaParcelable {
public:
    std::string originalFileId;  // fileId of the input asset, used to align with the input uris.
    std::string resultUri;       // uri of the asset after the operation, empty string when it failed.
    bool isSuccess {false};      // true: the asset is handled successfully, false: skipped or failed.

public:  // functions of Parcelable.
    virtual ~AssetUriResult() = default;
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_ASSETS_MANAGER_ASSET_URI_RESULT_VO_H

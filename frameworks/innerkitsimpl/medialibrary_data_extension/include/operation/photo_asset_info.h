/*
 * Copyright (C) 2024-2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_PHOTO_ASSET_INFO_H
#define OHOS_MEDIA_PHOTO_ASSET_INFO_H

#include <string>
#include <sstream>

namespace OHOS::Media {
class PhotoAssetInfo {
public:
    std::string displayName;
    int32_t subtype;
    int32_t ownerAlbumId;
    std::string burstGroupName;

public:
    std::string ToString() const
    {
        std::stringstream ss;
        ss << "PhotoAssetInfo[displayName: " << displayName << ", subtype: " << subtype
           << ", ownerAlbumId: " << ownerAlbumId << ", burstGroupName: " << burstGroupName << "]";
        return ss.str();
    }
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_PHOTO_ASSET_INFO_H
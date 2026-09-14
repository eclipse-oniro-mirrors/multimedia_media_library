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

#include "live_photo_4d_asset_helper.h"

#include "medialibrary_type_const.h"

namespace OHOS {
namespace Media::AccurateRefresh {

bool LivePhoto4DAssetHelper::IsAsset(const PhotoAssetChangeInfo &assetInfo)
{
    return IsTypeStatusLivePhoto4D(assetInfo) && AlbumAssetHelper::IsCommonSystemAsset(assetInfo, false);
}

bool LivePhoto4DAssetHelper::IsVideoAsset(const PhotoAssetChangeInfo &assetInfo)
{
    return IsAsset(assetInfo) && AlbumAssetHelper::IsVideoAsset(assetInfo);
}

bool LivePhoto4DAssetHelper::IsHiddenAsset(const PhotoAssetChangeInfo &assetInfo)
{
    return IsTypeStatusLivePhoto4D(assetInfo) && AlbumAssetHelper::IsCommonSystemAsset(assetInfo, true);
}
bool LivePhoto4DAssetHelper::IsNewerAsset(const PhotoAssetChangeInfo &compareAssetInfo,
    const PhotoAssetChangeInfo &currentAssetInfo)
{
    return IsAsset(compareAssetInfo) && IsAsset(currentAssetInfo) &&
        AlbumAssetHelper::IsNewerByDateTaken(compareAssetInfo, currentAssetInfo, false);
}
bool LivePhoto4DAssetHelper::IsNewerHiddenAsset(const PhotoAssetChangeInfo &compareAssetInfo,
    const PhotoAssetChangeInfo &currentAssetInfo)
{
    return IsHiddenAsset(compareAssetInfo) && IsHiddenAsset(currentAssetInfo) &&
        AlbumAssetHelper::IsNewerByHiddenTime(compareAssetInfo, currentAssetInfo);
}

bool LivePhoto4DAssetHelper::IsTypeStatusLivePhoto4D(const PhotoAssetChangeInfo &assetInfo)
{
    return assetInfo.livephoto4dStatus_ == static_cast<int32_t>(LivePhoto4dStatusType::TYPE_LIVEPHOTO_4D);
}
} // namespace Media
} // namespace OHOS
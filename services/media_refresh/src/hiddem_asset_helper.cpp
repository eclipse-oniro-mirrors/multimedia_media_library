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

#include "hidden_asset_helper.h"

namespace OHOS {
namespace Media::AccurateRefresh {

bool HiddenAssetHelper::IsAsset(const PhotoAssetChangeInfo &assetInfo)
{
    return AlbumAssetHelper::IsCommonSystemAsset(assetInfo, true, 0);
}

bool HiddenAssetHelper::IsVideoAsset(const PhotoAssetChangeInfo &assetInfo)
{
    return IsAsset(assetInfo) && AlbumAssetHelper::IsVideoAsset(assetInfo);
}

bool HiddenAssetHelper::IsHiddenAsset(const PhotoAssetChangeInfo &assetInfo)
{
    return IsAsset(assetInfo);
}
bool HiddenAssetHelper::IsNewerAsset(const PhotoAssetChangeInfo &compareAssetInfo,
    const PhotoAssetChangeInfo &currentAssetInfo)
{
    return IsNewerHiddenAsset(compareAssetInfo, currentAssetInfo);
}
bool HiddenAssetHelper::IsNewerHiddenAsset(const PhotoAssetChangeInfo &compareAssetInfo,
    const PhotoAssetChangeInfo &currentAssetInfo)
{
    return IsHiddenAsset(compareAssetInfo) && IsHiddenAsset(currentAssetInfo) &&
        AlbumAssetHelper::IsNewerByHiddenTime(compareAssetInfo, currentAssetInfo);
}

} // namespace Media
} // namespace OHOS
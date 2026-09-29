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

#include "trash_asset_helper.h"
#include "userfile_manager_types.h"
#include "medialibrary_type_const.h"

namespace OHOS {
namespace Media::AccurateRefresh {

bool TrashAssetHelper::IsAsset(const PhotoAssetChangeInfo &assetInfo)
{
    return assetInfo.syncStatus_ == static_cast<int32_t> (SyncStatusType::TYPE_VISIBLE) &&
        assetInfo.cleanFlag_ == static_cast<int32_t> (CleanType::TYPE_NOT_CLEAN) &&
        assetInfo.dateTrashedMs_ > 0 &&
        assetInfo.burstCoverLevel_ == static_cast<int32_t> (BurstCoverLevelType::COVER) &&
        assetInfo.timePending_ == 0 &&
        !assetInfo.isTemp_ &&
        assetInfo.isShared_ == 0;
}

bool TrashAssetHelper::IsVideoAsset(const PhotoAssetChangeInfo &assetInfo)
{
    return IsAsset(assetInfo) && AlbumAssetHelper::IsVideoAsset(assetInfo);
}

bool TrashAssetHelper::IsHiddenAsset(const PhotoAssetChangeInfo &assetInfo)
{
    return assetInfo.syncStatus_ == static_cast<int32_t> (SyncStatusType::TYPE_VISIBLE) &&
        assetInfo.cleanFlag_ == static_cast<int32_t> (CleanType::TYPE_NOT_CLEAN) &&
        assetInfo.dateTrashedMs_ > 0 &&
        assetInfo.burstCoverLevel_ == static_cast<int32_t> (BurstCoverLevelType::COVER) &&
        assetInfo.timePending_ == 0 &&
        !assetInfo.isTemp_ &&
        assetInfo.isHidden_ &&
        assetInfo.isShared_ == 0;
}

bool TrashAssetHelper::IsNewerAsset(const PhotoAssetChangeInfo &compareAssetInfo,
    const PhotoAssetChangeInfo &currentAssetInfo)
{
    return IsAsset(compareAssetInfo) && IsAsset(currentAssetInfo) &&
        AlbumAssetHelper::IsNewerByDateTaken(compareAssetInfo, currentAssetInfo);
}
bool TrashAssetHelper::IsNewerHiddenAsset(const PhotoAssetChangeInfo &compareAssetInfo,
    const PhotoAssetChangeInfo &currentAssetInfo)
{
    return IsHiddenAsset(compareAssetInfo) && IsHiddenAsset(currentAssetInfo) &&
        AlbumAssetHelper::IsNewerByHiddenTime(compareAssetInfo, currentAssetInfo);
}

} // namespace Media
} // namespace OHOS
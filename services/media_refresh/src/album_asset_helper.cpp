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
#define MLOG_TAG "AccurateRefresh::AlbumAssetHelper"

#include "album_asset_helper.h"
#include "userfile_manager_types.h"
#include "medialibrary_type_const.h"
#include "accurate_debug_log.h"

namespace OHOS {
namespace Media::AccurateRefresh {

bool AlbumAssetHelper::IsCommonSystemAsset(const PhotoAssetChangeInfo &assetInfo, bool isHiddenAsset)
{
    return assetInfo.syncStatus_ == static_cast<int32_t> (SyncStatusType::TYPE_VISIBLE) &&
        assetInfo.cleanFlag_ == static_cast<int32_t> (CleanType::TYPE_NOT_CLEAN) &&
        assetInfo.dateTrashedMs_ == 0 &&
        assetInfo.isHidden_ == isHiddenAsset &&
        assetInfo.timePending_ == 0 &&
        !assetInfo.isTemp_ &&
        assetInfo.burstCoverLevel_ == static_cast<int32_t> (BurstCoverLevelType::COVER);
}

bool AlbumAssetHelper::IsVideoAsset(const PhotoAssetChangeInfo &assetInfo)
{
    return assetInfo.mediaType_ == static_cast<int32_t> (MediaType::MEDIA_TYPE_VIDEO);
}

bool AlbumAssetHelper::IsImageAsset(const PhotoAssetChangeInfo &assetInfo)
{
    return assetInfo.mediaType_ == static_cast<int32_t> (MediaType::MEDIA_TYPE_IMAGE);
}

bool AlbumAssetHelper::IsNewerByDateAdded(const PhotoAssetChangeInfo &compareAssetInfo,
    const PhotoAssetChangeInfo &currentAssetInfo, bool isAsc)
{
    return compareAssetInfo.dateAddedMs_ > currentAssetInfo.dateAddedMs_ ||
        (compareAssetInfo.dateAddedMs_ == currentAssetInfo.dateAddedMs_ &&
        (isAsc ? compareAssetInfo.fileId_ < currentAssetInfo.fileId_ :
        compareAssetInfo.fileId_ > currentAssetInfo.fileId_));
}

bool AlbumAssetHelper::IsNewerByDateTaken(const PhotoAssetChangeInfo &compareAssetInfo,
    const PhotoAssetChangeInfo &currentAssetInfo, bool isAsc)
{
    return compareAssetInfo.dateTakenMs_ > currentAssetInfo.dateTakenMs_ ||
        (compareAssetInfo.dateTakenMs_ == currentAssetInfo.dateTakenMs_ &&
        (isAsc ? compareAssetInfo.fileId_ < currentAssetInfo.fileId_ :
        compareAssetInfo.fileId_ > currentAssetInfo.fileId_));
}

bool AlbumAssetHelper::IsNewerByHiddenTime(const PhotoAssetChangeInfo &compareAssetInfo,
    const PhotoAssetChangeInfo &currentAssetInfo, bool isAsc)
{
    return compareAssetInfo.hiddenTime_ > currentAssetInfo.hiddenTime_ ||
        (compareAssetInfo.hiddenTime_ == currentAssetInfo.hiddenTime_ &&
        (isAsc ? compareAssetInfo.fileId_ < currentAssetInfo.fileId_ :
        compareAssetInfo.fileId_ > currentAssetInfo.fileId_));
}

bool AlbumAssetHelper::IsInvalidAsset(const PhotoAssetChangeInfo &assetInfo)
{
    return assetInfo.fileId_ == INVALID_INT32_VALUE;
}

bool AlbumAssetHelper::IsNameChange(const PhotoAssetChangeData &assetChangeData)
{
    return assetChangeData.infoBeforeChange_.displayName_ != assetChangeData.infoAfterChange_.displayName_;
}

bool AlbumAssetHelper::IsSizeChange(const PhotoAssetChangeData &assetChangeData)
{
    return assetChangeData.infoBeforeChange_.size_ != assetChangeData.infoAfterChange_.size_;
}

bool AlbumAssetHelper::UpdateCover(const PhotoAssetChangeData &assetChangeData,
    std::function<bool(const PhotoAssetChangeInfo&)> isAlbumAsset,
    std::function<bool(const PhotoAssetChangeInfo&, const PhotoAssetChangeInfo&)> isNewerAsset,
    PhotoAssetChangeInfo &addCover, std::unordered_set<int32_t> &removeFileIds)
{
    bool before = isAlbumAsset(assetChangeData.infoBeforeChange_);
    bool after = isAlbumAsset(assetChangeData.infoAfterChange_);
    bool ret = false;
    if (!before && after) {
        if (AlbumAssetHelper::IsInvalidAsset(addCover) ||
            isNewerAsset(assetChangeData.infoAfterChange_, addCover)) {
            addCover = assetChangeData.infoAfterChange_;
            ret = true;
        }
    } else if (before && !after) {
        removeFileIds.insert(assetChangeData.infoBeforeChange_.fileId_);
    }

    return ret;
}

bool AlbumAssetHelper::UpdateCount(const PhotoAssetChangeData &assetChangeData,
    std::function<bool(PhotoAssetChangeInfo)> isAlbumAsset, int32_t &count)
{
    bool before = isAlbumAsset(assetChangeData.infoBeforeChange_);
    bool after = isAlbumAsset(assetChangeData.infoAfterChange_);
    bool ret = false;
    if (!before && after) {
        count++;
        ret = true;
    }
    if (before && !after) {
        count--;
        ret = true;
    }
    
    return ret;
}

bool AlbumAssetHelper::CalAlbumRefreshInfo(std::function<bool(AlbumRefreshInfo &refreshInfo)> calFunc,
    AlbumRefreshInfo &refreshInfo, int32_t albumId, bool isHidden, AlbumRefreshTimestamp &assetTimestamp)
{
    auto &isForceRefresh = isHidden ? refreshInfo.isHiddenForceRefresh_ : refreshInfo.isForceRefresh_;
    if (isForceRefresh) {
        ACCURATE_DEBUG("already force refresh[%{public}d, %{public}d]", albumId, isHidden);
        return true;
    }
    // refreshInfo记录计算时的album全量刷新时间戳
    auto &albumRefreshTimestamp = isHidden ? refreshInfo.albumHiddenRefreshTimestamp_ :
        refreshInfo.albumRefreshTimestamp_;
    if (albumRefreshTimestamp.start_ == INVALID_INT64_VALUE || albumRefreshTimestamp.end_ == INVALID_INT64_VALUE) {
        albumRefreshTimestamp = AlbumAccurateRefreshManager::GetInstance().GetRefreshTimestamp(albumId, isHidden);
    }
    auto action = AlbumAccurateRefreshManager::GetInstance().GetRefreshAction(albumRefreshTimestamp, assetTimestamp);
    MEDIA_DEBUG_LOG("set force refresh[%{public}d, %{public}d] albumRefreshTimestamp: %{public}s, "
        "assetTimestamp: %{public}s, action: %{public}d", albumId, isHidden, albumRefreshTimestamp.ToString().c_str(),
        assetTimestamp.ToString().c_str(), action);
    if (action == AssetRefreshAlbumAction::ACCURATE_REFRESH) {
        return calFunc(refreshInfo);
    } else if (action == AssetRefreshAlbumAction::IGNORE) {
        return false;
    }

    // AssetRefreshAlbumAction::FORCE_REFRESH
    isForceRefresh = true;
    ACCURATE_DEBUG("set force refresh[%{public}d, %{public}d] ", albumId, isHidden);
    return true;
}

} // namespace Media
} // namespace OHOS
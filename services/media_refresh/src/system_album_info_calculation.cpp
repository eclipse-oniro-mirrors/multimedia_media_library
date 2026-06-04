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

#define MLOG_TAG "AccurateRefresh::SystemAlbumInfoCalculation"

#include "system_album_info_calculation.h"
#include "album_asset_helper.h"
#include "album_accurate_refresh_manager.h"

#include "accurate_debug_log.h"

using namespace std;

namespace OHOS {
namespace Media::AccurateRefresh {

bool SystemAlbumInfoCalculation::CalAlbumRefreshInfo(const PhotoAssetChangeData &assetChangeData,
    AlbumRefreshInfo &refreshInfo, int32_t albumId)
{
    bool ret = false;
    AlbumRefreshTimestamp assetTimestamp(assetChangeData.infoBeforeChange_.timestamp_,
        assetChangeData.infoAfterChange_.timestamp_);
    // 普通信息变化
    if (IsSystemAlbumInfoChange(assetChangeData, isSystemAsset_)) {
        std::function<bool(AlbumRefreshInfo &)> calRefreshInfoFunc = [this, &assetChangeData]
            (AlbumRefreshInfo &refreshInfo) -> bool {
            return this->UpdateRefreshNormalInfo(assetChangeData, refreshInfo);
        };
        ret = AlbumAssetHelper::CalAlbumRefreshInfo(calRefreshInfoFunc, refreshInfo, albumId, false, assetTimestamp);
    }
    
    // 隐藏相册不需要计算
    if (subType_ == PhotoAlbumSubType::HIDDEN) {
        return ret;
    }

    // 隐藏信息变化
    if (IsSystemAlbumInfoChange(assetChangeData, isHiddenSystemAsset_)) {
        std::function<bool(AlbumRefreshInfo &)> calHiddenRefreshInfoFunc = [this, &assetChangeData]
            (AlbumRefreshInfo &refreshInfo) -> bool {
            return this->UpdateRefreshHiddenInfo(assetChangeData, refreshInfo);
        };
        ret = AlbumAssetHelper::CalAlbumRefreshInfo(calHiddenRefreshInfoFunc, refreshInfo, albumId, true,
            assetTimestamp) || ret;
    }
    
    return ret;
}

bool SystemAlbumInfoCalculation::IsNameChange(const PhotoAssetChangeData &assetChangeData,
    std::function<bool(PhotoAssetChangeInfo)> isSystemAsset)
{
    return isSystemAsset(assetChangeData.infoBeforeChange_) && isSystemAsset(assetChangeData.infoAfterChange_) &&
        AlbumAssetHelper::IsNameChange(assetChangeData);
}

bool SystemAlbumInfoCalculation::IsSizeChange(const PhotoAssetChangeData &assetChangeData,
    std::function<bool(PhotoAssetChangeInfo)> isSystemAsset)
{
    return isSystemAsset(assetChangeData.infoBeforeChange_) && isSystemAsset(assetChangeData.infoAfterChange_) &&
        AlbumAssetHelper::IsSizeChange(assetChangeData);
}

bool SystemAlbumInfoCalculation::UpdateCover(const PhotoAssetChangeData &assetChangeData,
    std::function<bool(const PhotoAssetChangeInfo&)> isSystemAsset,
    std::function<bool(const PhotoAssetChangeInfo&, const PhotoAssetChangeInfo&)> isNewerAsset,
    PhotoAssetChangeInfo &addCover, unordered_set<int32_t> &removeFileIds)
{
    function<bool(const std::string &orderKey)> setOrderKey = [&] (const std::string &orderKey) -> bool {
        addCover.orderKey_ = orderKey;
        return true;
    };
    return AlbumAssetHelper::UpdateCover(assetChangeData, isSystemAsset, isNewerAsset, addCover, removeFileIds) ||
        (IsNameChange(assetChangeData, isSystemAsset) && setOrderKey("diaplay_name")) ||
        (IsSizeChange(assetChangeData, isSystemAsset) && setOrderKey("size"));
}

bool SystemAlbumInfoCalculation::UpdateCount(const PhotoAssetChangeData &assetChangeData,
    std::function<bool(PhotoAssetChangeInfo)> isSystemAsset, int32_t &count)
{
    return AlbumAssetHelper::UpdateCount(assetChangeData, isSystemAsset, count);
}

bool SystemAlbumInfoCalculation::UpdateRefreshNormalInfo(const PhotoAssetChangeData &assetChangeData,
    AlbumRefreshInfo& refreshInfo)
{
    bool ret = false;
    // count更新
    if (UpdateCount(assetChangeData, isSystemAsset_, refreshInfo.deltaCount_)) {
        refreshInfo.assetModifiedCnt_++;
        ret = true;
    }
    
    // video countvideo count更新
    if (UpdateCount(assetChangeData, isVideoAsset_, refreshInfo.deltaVideoCount_)) {
        ret = true;
    }

    // cover更新
    if (UpdateCover(assetChangeData, isSystemAsset_, isNewerAsset_, refreshInfo.deltaAddCover_,
        refreshInfo.removeFileIds)) {
        ret = true;
    }
    return ret;
}

bool SystemAlbumInfoCalculation::UpdateRefreshHiddenInfo(const PhotoAssetChangeData &assetChangeData,
    AlbumRefreshInfo& refreshInfo)
{
    bool ret = false;
    // hidden count
    if (this->UpdateCount(assetChangeData, this->isHiddenSystemAsset_, refreshInfo.deltaHiddenCount_)) {
        refreshInfo.hiddenAssetModifiedCnt_++;
        ret = true;
    }
    // hidden cover更新
    if (this->UpdateCover(assetChangeData, this->isHiddenSystemAsset_, this->isNewerHiddenAsset_,
        refreshInfo.deltaAddHiddenCover_, refreshInfo.removeHiddenFileIds)) {
        ret = true;
    }
    return ret;
}

bool SystemAlbumInfoCalculation::IsSystemAlbumInfoChange(const PhotoAssetChangeData &assetChangeData,
    std::function<bool(PhotoAssetChangeInfo)> isAlbumAsset)
{
    return isAlbumAsset(assetChangeData.infoBeforeChange_) != isAlbumAsset(assetChangeData.infoAfterChange_) ||
        IsNameChange(assetChangeData, isAlbumAsset) || IsSizeChange(assetChangeData, isAlbumAsset);
}
} // namespace Media
} // namespace OHOS
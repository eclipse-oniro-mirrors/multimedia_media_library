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

/*
 * 共享相册精准刷新 — 增量刷新路径
 * 参照 OwnerAlbumInfoCalculation 实现共享相册的增量刷新信息计算。
 * 与普通相册的核心差异：
 *   - 资产归属判断：需 is_shared=1
 *   - 封面排序：share_group DESC + date_taken DESC
 *   - 封面候选资产：额外过滤 photo_visibility=0
 */
#define MLOG_TAG "AccurateRefresh::ShareAlbumInfoCalculation"

#include "share_album_info_calculation.h"
#include "album_asset_helper.h"
#include "accurate_debug_log.h"

using namespace std;

namespace OHOS {
namespace Media::AccurateRefresh {

unordered_map<int32_t, AlbumRefreshInfo> ShareAlbumInfoCalculation::CalShareAlbumRefreshInfo(
    const std::vector<PhotoAssetChangeData> &assetChangeDatas)
{
    unordered_map<int32_t, AlbumRefreshInfo> shareAlbumInfos;
    for (auto &assetChangeData : assetChangeDatas) {
        auto initAlbumId = assetChangeData.infoBeforeChange_.ownerAlbumId_;
        auto modifiedAlbumId = assetChangeData.infoAfterChange_.ownerAlbumId_;
        MEDIA_DEBUG_LOG("initAlbumId: %{public}d, modifiedAlbumId: %{public}d", initAlbumId, modifiedAlbumId);

        // 无效数据：变更前后都不属于任何相册
        if (initAlbumId == INVALID_INT32_VALUE && modifiedAlbumId == INVALID_INT32_VALUE) {
            MEDIA_WARN_LOG("asset change data invalid albumId.");
            continue;
        }

        // 更新变更前所属的共享相册
        if (initAlbumId != INVALID_INT32_VALUE) {
            UpdateShareRefreshInfo(initAlbumId, assetChangeData, shareAlbumInfos);
        }

        // 更新变更后所属的共享相册（如果与变更前不同）
        if (modifiedAlbumId != INVALID_INT32_VALUE && initAlbumId != modifiedAlbumId) {
            UpdateShareRefreshInfo(modifiedAlbumId, assetChangeData, shareAlbumInfos);
        }
    }
    return shareAlbumInfos;
}

void ShareAlbumInfoCalculation::UpdateShareRefreshInfo(int32_t albumId,
    const PhotoAssetChangeData &assetChangeData,
    std::unordered_map<int32_t, AlbumRefreshInfo> &shareAlbumInfos)
{
    auto refreshInfoIter = shareAlbumInfos.find(albumId);
    if (refreshInfoIter != shareAlbumInfos.end()) {
        CalShareAlbumRefreshInfo(assetChangeData, albumId, refreshInfoIter->second);
    } else {
        AlbumRefreshInfo refreshInfo;
        if (CalShareAlbumRefreshInfo(assetChangeData, albumId, refreshInfo)) {
            shareAlbumInfos.emplace(albumId, refreshInfo);
        }
    }
}

/*
 * 判断资产是否为共享相册的普通可见资产（count 用）
 * 条件：owner_album_id 匹配 + is_shared=1 + IsCommonSystemAsset（不含 photo_visibility 过滤）
 * 规格：共享相册 count 需要包含封禁图片，所以不检查 photo_visibility
 */
bool ShareAlbumInfoCalculation::IsShareAlbumAsset(const PhotoAssetChangeInfo &assetInfo, int32_t albumId)
{
    return assetInfo.ownerAlbumId_ == albumId
        && assetInfo.isShared_ == 1
        && AlbumAssetHelper::IsCommonSystemAsset(assetInfo, false, 1);
}

bool ShareAlbumInfoCalculation::IsShareAlbumCoverAsset(const PhotoAssetChangeInfo &assetInfo, int32_t albumId)
{
    return IsShareAlbumAsset(assetInfo, albumId)
        && assetInfo.photoVisibility_ == 0;
}

bool ShareAlbumInfoCalculation::IsShareAlbumHiddenAsset(const PhotoAssetChangeInfo &assetInfo, int32_t albumId)
{
    return assetInfo.ownerAlbumId_ == albumId
        && assetInfo.isShared_ == 1
        && AlbumAssetHelper::IsCommonSystemAsset(assetInfo, true, 1);
}

bool ShareAlbumInfoCalculation::IsShareAlbumVideoAsset(const PhotoAssetChangeInfo &assetInfo, int32_t albumId)
{
    return IsShareAlbumAsset(assetInfo, albumId) && AlbumAssetHelper::IsVideoAsset(assetInfo);
}

bool ShareAlbumInfoCalculation::IsNewerAsset(const PhotoAssetChangeInfo &compareAssetInfo,
    const PhotoAssetChangeInfo &currentAssetInfo, int32_t albumId)
{
    // 只有同为共享相册的资产才比较（通过 IsShareAlbumCoverAsset 保证）
    // share_group 降序：值大的优先（INVALID_INT64_VALUE 视为 0，与 SQL COALESCE 一致）
    int64_t compareGroup = (compareAssetInfo.shareGroup_ != INVALID_INT64_VALUE) ? compareAssetInfo.shareGroup_ : 0;
    int64_t currentGroup = (currentAssetInfo.shareGroup_ != INVALID_INT64_VALUE) ? currentAssetInfo.shareGroup_ : 0;
    if (compareGroup != currentGroup) {
        return compareGroup > currentGroup;
    }
    // date_taken 降序：时间晚的优先
    if (compareAssetInfo.dateTakenMs_ != currentAssetInfo.dateTakenMs_) {
        return compareAssetInfo.dateTakenMs_ > currentAssetInfo.dateTakenMs_;
    }
    // display_name 降序：名称大的优先
    if (compareAssetInfo.displayName_ != currentAssetInfo.displayName_) {
        return compareAssetInfo.displayName_ > currentAssetInfo.displayName_;
    }
    // fileId 降序兜底
    return compareAssetInfo.fileId_ > currentAssetInfo.fileId_;
}

bool ShareAlbumInfoCalculation::IsNewerHiddenAsset(const PhotoAssetChangeInfo &compareAssetInfo,
    const PhotoAssetChangeInfo &currentAssetInfo, int32_t albumId)
{
    return IsShareAlbumAsset(compareAssetInfo, albumId)
        && IsShareAlbumAsset(currentAssetInfo, albumId)
        && AlbumAssetHelper::IsNewerByHiddenTime(compareAssetInfo, currentAssetInfo);
}

bool ShareAlbumInfoCalculation::IsNameChange(const PhotoAssetChangeData &assetChangeData, int32_t albumId,
    std::function<bool(const PhotoAssetChangeInfo&, int32_t)> isAlbumAsset)
{
    return isAlbumAsset(assetChangeData.infoBeforeChange_, albumId) &&
        isAlbumAsset(assetChangeData.infoAfterChange_, albumId) &&
        AlbumAssetHelper::IsNameChange(assetChangeData);
}

bool ShareAlbumInfoCalculation::IsSizeChange(const PhotoAssetChangeData &assetChangeData, int32_t albumId,
    std::function<bool(const PhotoAssetChangeInfo&, int32_t)> isAlbumAsset)
{
    return isAlbumAsset(assetChangeData.infoBeforeChange_, albumId) &&
        isAlbumAsset(assetChangeData.infoAfterChange_, albumId) &&
        AlbumAssetHelper::IsSizeChange(assetChangeData);
}

/*
 * 更新封面信息
 * 与普通相册的 UpdateCover 逻辑相同，但使用共享相册专用的 isNewerAsset（share_group 排序）
 */
bool ShareAlbumInfoCalculation::UpdateCover(const PhotoAssetChangeData &assetChangeData,
    function<bool(const PhotoAssetChangeInfo&, int32_t)> isAsset, int32_t albumId,
    function<bool(const PhotoAssetChangeInfo&, const PhotoAssetChangeInfo&, int32_t)> isNewerAsset,
    PhotoAssetChangeInfo &addCover, unordered_set<int32_t> &removeFileIds)
{
    function<bool(const PhotoAssetChangeInfo&)> isAlbumAsset =
        [&] (const PhotoAssetChangeInfo& assetChangeData) -> bool {
            return isAsset(assetChangeData, albumId);
    };
    function<bool(const PhotoAssetChangeInfo&, const PhotoAssetChangeInfo&)> isAlbumNewerAsset =
        [&] (const PhotoAssetChangeInfo &compare, const PhotoAssetChangeInfo &current) -> bool {
            return isNewerAsset(compare, current, albumId);
    };
    return AlbumAssetHelper::UpdateCover(assetChangeData, isAlbumAsset, isAlbumNewerAsset, addCover, removeFileIds) ||
        IsNameChange(assetChangeData, albumId, isAsset) || IsSizeChange(assetChangeData, albumId, isAsset);
}

bool ShareAlbumInfoCalculation::UpdateCount(const PhotoAssetChangeData &assetChangeData,
    std::function<bool(const PhotoAssetChangeInfo&, int32_t)> isAsset, int32_t albumId, int32_t &count)
{
    function<bool(const PhotoAssetChangeInfo&)> isAlbumAsset =
        [&] (const PhotoAssetChangeInfo& assetChangeData) -> bool {
            return isAsset(assetChangeData, albumId);
    };
    return AlbumAssetHelper::UpdateCount(assetChangeData, isAlbumAsset, count);
}

/*
 * 计算单个资产的增量刷新信息
 * 分别计算普通信息和隐藏信息的增量变化
 */
bool ShareAlbumInfoCalculation::CalShareAlbumRefreshInfo(const PhotoAssetChangeData &assetChangeData,
    int32_t albumId, AlbumRefreshInfo &refreshInfo)
{
    bool ret = false;
    AlbumRefreshTimestamp assetTimestamp(assetChangeData.infoBeforeChange_.timestamp_,
        assetChangeData.infoAfterChange_.timestamp_);

    // 共享相册普通信息变化：入口使用 IsShareAlbumAsset（不含 photo_visibility 过滤，count 需包含封禁图片）
    // cover_uri 在 UpdateRefreshNormalInfo 内部通过 IsShareAlbumCoverAsset 单独过滤
    if (IsShareAlbumInfoChange(assetChangeData, IsShareAlbumAsset, albumId)) {
        function<bool(AlbumRefreshInfo&)> calRefreshInfoFunc = [&assetChangeData, albumId]
            (AlbumRefreshInfo &refreshInfo) -> bool {
                return ShareAlbumInfoCalculation::UpdateRefreshNormalInfo(assetChangeData, albumId, refreshInfo);
        };
        ret = AlbumAssetHelper::CalAlbumRefreshInfo(calRefreshInfoFunc, refreshInfo, albumId, false, assetTimestamp);
    }

    // 共享相册隐藏信息变化
    if (IsShareAlbumInfoChange(assetChangeData, IsShareAlbumHiddenAsset, albumId)) {
        function<bool(AlbumRefreshInfo&)> calHiddenRefreshInfoFunc = [&assetChangeData, albumId]
            (AlbumRefreshInfo &refreshInfo) -> bool {
                return ShareAlbumInfoCalculation::UpdateRefreshHiddenInfo(assetChangeData, albumId, refreshInfo);
        };
        ret = AlbumAssetHelper::CalAlbumRefreshInfo(calHiddenRefreshInfoFunc, refreshInfo, albumId, true,
            assetTimestamp) || ret;
    }
    return ret;
}

/*
 * 更新普通信息增量
 * count 使用 IsShareAlbumAsset（不含 photo_visibility 过滤），
 * cover 使用 IsShareAlbumCoverAsset（含 photo_visibility=0 过滤），通过 IsNewerAsset 排序
 */
bool ShareAlbumInfoCalculation::UpdateRefreshNormalInfo(const PhotoAssetChangeData &assetChangeData,
    int32_t albumId, AlbumRefreshInfo& refreshInfo)
{
    bool ret = false;
    // count 增量：使用 IsShareAlbumAsset（不含 photo_visibility，count 包含封禁图片）
    if (UpdateCount(assetChangeData, IsShareAlbumAsset, albumId, refreshInfo.deltaCount_)) {
        refreshInfo.assetModifiedCnt_++;
        ret = true;
    }
    // video_count 增量
    if (UpdateCount(assetChangeData, IsShareAlbumVideoAsset, albumId,
        refreshInfo.deltaVideoCount_)) {
        ret = true;
    }
    // cover 增量：使用 IsShareAlbumCoverAsset（含 photo_visibility=0 过滤）+ share_group 排序
    if (UpdateCover(assetChangeData, IsShareAlbumCoverAsset, albumId, IsNewerAsset,
        refreshInfo.deltaAddCover_, refreshInfo.removeFileIds)) {
        ret = true;
    }
    return ret;
}

bool ShareAlbumInfoCalculation::UpdateRefreshHiddenInfo(const PhotoAssetChangeData &assetChangeData,
    int32_t albumId, AlbumRefreshInfo& refreshInfo)
{
    bool ret = false;
    if (UpdateCount(assetChangeData, IsShareAlbumHiddenAsset, albumId,
        refreshInfo.deltaHiddenCount_)) {
        refreshInfo.hiddenAssetModifiedCnt_++;
        ret = true;
    }
    if (UpdateCover(assetChangeData, IsShareAlbumHiddenAsset, albumId, IsNewerHiddenAsset,
        refreshInfo.deltaAddHiddenCover_, refreshInfo.removeHiddenFileIds)) {
        ret = true;
    }
    return ret;
}

/*
 * 判断资产是否对共享相册产生实质性变化
 */
bool ShareAlbumInfoCalculation::IsShareAlbumInfoChange(const PhotoAssetChangeData &assetChangeData,
    std::function<bool(const PhotoAssetChangeInfo&, int32_t)> isAlbumAsset, int32_t albumId)
{
    return isAlbumAsset(assetChangeData.infoBeforeChange_, albumId) !=
        isAlbumAsset(assetChangeData.infoAfterChange_, albumId) ||
        IsNameChange(assetChangeData, albumId, isAlbumAsset) ||
        IsSizeChange(assetChangeData, albumId, isAlbumAsset);
}

} // namespace Media
} // namespace OHOS

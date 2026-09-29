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

#ifndef OHOS_MEDIALIBRARY_SHARE_ALBUM_INFO_CALCULATION_H
#define OHOS_MEDIALIBRARY_SHARE_ALBUM_INFO_CALCULATION_H

#include <unordered_map>
#include "album_change_info.h"
#include "photo_asset_change_info.h"

namespace OHOS {
namespace Media::AccurateRefresh {

class ShareAlbumInfoCalculation {
public:
    static std::unordered_map<int32_t, AlbumRefreshInfo> CalShareAlbumRefreshInfo(
        const std::vector<PhotoAssetChangeData> &assetChangeDatas);

private:
    // 判断资产是否为共享相册的普通可见资产（count 用，不需要 photo_visibility 条件）
    static bool IsShareAlbumAsset(const PhotoAssetChangeInfo &assetInfo, int32_t albumId);

    // 判断资产是否为共享相册的隐藏资产（hidden_count 用）
    static bool IsShareAlbumHiddenAsset(const PhotoAssetChangeInfo &assetInfo, int32_t albumId);

    // 判断资产是否为共享相册的视频资产（video_count 用）
    static bool IsShareAlbumVideoAsset(const PhotoAssetChangeInfo &assetInfo, int32_t albumId);

    // 判断是否为更新的封面候选资产（cover_uri 用，需要 photo_visibility=0）
    static bool IsShareAlbumCoverAsset(const PhotoAssetChangeInfo &assetInfo, int32_t albumId);

    // 按 share_group DESC + date_taken DESC 比较新旧资产
    static bool IsNewerAsset(const PhotoAssetChangeInfo &compareAssetInfo,
        const PhotoAssetChangeInfo &currentAssetInfo, int32_t albumId);

    // 按 hidden_time 比较隐藏资产
    static bool IsNewerHiddenAsset(const PhotoAssetChangeInfo &compareAssetInfo,
        const PhotoAssetChangeInfo &currentAssetInfo, int32_t albumId);

    // 计算单个资产的增量刷新信息
    static bool CalShareAlbumRefreshInfo(const PhotoAssetChangeData &assetChangeData, int32_t albumId,
        AlbumRefreshInfo &refreshInfo);

    // 更新封面信息（使用 share_group 排序）
    static bool UpdateCover(const PhotoAssetChangeData &assetChangeData,
        std::function<bool(const PhotoAssetChangeInfo&, int32_t)> isAsset, int32_t albumId,
        std::function<bool(const PhotoAssetChangeInfo&, const PhotoAssetChangeInfo&, int32_t)> isNewerAsset,
        PhotoAssetChangeInfo &addCover, std::unordered_set<int32_t> &removeFileIds);

    // 更新 count
    static bool UpdateCount(const PhotoAssetChangeData &assetChangeData,
        std::function<bool(const PhotoAssetChangeInfo&, int32_t)> isAsset, int32_t albumId,
        int32_t &count);

    // 更新普通信息增量（deltaCount_, deltaVideoCount_, deltaAddCover_, removeFileIds）
    static bool UpdateRefreshNormalInfo(const PhotoAssetChangeData &assetChangeData, int32_t albumId,
        AlbumRefreshInfo& refreshInfo);

    // 更新隐藏信息增量（deltaHiddenCount_, deltaAddHiddenCover_, removeHiddenFileIds）
    static bool UpdateRefreshHiddenInfo(const PhotoAssetChangeData &assetChangeData, int32_t albumId,
        AlbumRefreshInfo& refreshInfo);

    // 判断资产是否对共享相册产生实质性变化
    static bool IsShareAlbumInfoChange(const PhotoAssetChangeData &assetChangeData,
        std::function<bool(const PhotoAssetChangeInfo&, int32_t)> isAlbumAsset, int32_t albumId);

    // 更新或创建指定 albumId 的刷新信息
    static void UpdateShareRefreshInfo(int32_t albumId, const PhotoAssetChangeData &assetChangeData,
        std::unordered_map<int32_t, AlbumRefreshInfo> &shareAlbumInfos);

    static bool IsNameChange(const PhotoAssetChangeData &assetChangeData, int32_t albumId,
        std::function<bool(const PhotoAssetChangeInfo&, int32_t)> isAlbumAsset);

    static bool IsSizeChange(const PhotoAssetChangeData &assetChangeData, int32_t albumId,
        std::function<bool(const PhotoAssetChangeInfo&, int32_t)> isAlbumAsset);
};

} // namespace Media
} // namespace OHOS

#endif

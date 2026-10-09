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

#ifndef OHOS_MEDIALIBRARY_OWNER_ALBUM_INFO_CALCULATION_H
#define OHOS_MEDIALIBRARY_OWNER_ALBUM_INFO_CALCULATION_H

#include <unordered_map>
#include "album_change_info.h"
#include "photo_asset_change_info.h"

namespace OHOS {
namespace Media::AccurateRefresh {

class OwnerAlbumInfoCalculation {
public:
    static std::unordered_map<int32_t, AlbumRefreshInfo> CalOwnerAlbumRefreshInfo(
        const std::vector<PhotoAssetChangeData> &assetChangeDatas);

private:
    static bool IsOwnerAlbumAsset(const PhotoAssetChangeInfo &assetInfo, int32_t albumId);
    static bool IsOwnerAlbumHiddenAsset(const PhotoAssetChangeInfo &assetInfo, int32_t albumId);
    static bool IsOwnerAlbumVideoAsset(const PhotoAssetChangeInfo &assetInfo, int32_t albumId);
    static bool IsNewerAsset(const PhotoAssetChangeInfo &compareAssetInfo,
        const PhotoAssetChangeInfo &currentAssetInfo, int32_t albumId);
    static bool IsNewerHiddenAsset(const PhotoAssetChangeInfo &compareAssetInfo,
        const PhotoAssetChangeInfo &currentAssetInfo, int32_t albumId);
    static bool CalOwnerAlbumRefreshInfo(const PhotoAssetChangeData &assetChangeData, int32_t albumId,
        AlbumRefreshInfo &refreshInfo);
    static bool UpdateCover(const PhotoAssetChangeData &assetChangeData,
        std::function<bool(const PhotoAssetChangeInfo&, int32_t)> isAsset, int32_t albumId,
        std::function<bool(const PhotoAssetChangeInfo&, const PhotoAssetChangeInfo&, int32_t)> isNewerAsset,
        PhotoAssetChangeInfo &addCover, std::unordered_set<int32_t> &removeFileIds);
    static bool UpdateCount(const PhotoAssetChangeData &assetChangeData,
        std::function<bool(const PhotoAssetChangeInfo&, int32_t)> isAsset, int32_t albumId,
        int32_t &count);
    static bool UpdateRefreshNormalInfo(const PhotoAssetChangeData &assetChangeData, int32_t albumId,
        AlbumRefreshInfo& refreshInfo);
    static bool UpdateRefreshHiddenInfo(const PhotoAssetChangeData &assetChangeData, int32_t albumId,
        AlbumRefreshInfo& refreshInfo);
    static bool IsOwnerAlbumInfoChange(const PhotoAssetChangeData &assetChangeData,
        std::function<bool(PhotoAssetChangeInfo, int32_t)> isAlbumAsset, int32_t albumId);
    static void UpdateOwnerRefreshInfo(int32_t albumId, const PhotoAssetChangeData &assetChangeData,
        std::unordered_map<int32_t, AlbumRefreshInfo> &ownerAlbumInfos);
    static bool IsNameChange(const PhotoAssetChangeData &assetChangeData, int32_t albumId,
        std::function<bool(PhotoAssetChangeInfo, int32_t)> isAlbumAsset);
    static bool IsSizeChange(const PhotoAssetChangeData &assetChangeData, int32_t albumId,
        std::function<bool(PhotoAssetChangeInfo, int32_t)> isAlbumAsset);
};

} // namespace Media
} // namespace OHOS

#endif
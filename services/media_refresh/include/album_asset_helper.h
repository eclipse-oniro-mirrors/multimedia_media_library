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

#ifndef OHOS_MEDIALIBRARY_ALBUM_ASSET_HELPER
#define OHOS_MEDIALIBRARY_ALBUM_ASSET_HELPER

#include <functional>

#include "photo_asset_change_info.h"
#include "album_change_info.h"
#include "album_accurate_refresh_manager.h"

namespace OHOS {
namespace Media::AccurateRefresh {

// 计算指定系统相册类型资产的基类，包括收藏相册、视频相册、隐藏相册、回收站相册、图片相册、云增强相册
class AlbumAssetHelper {
public:
    // 有效资产
    static bool IsCommonSystemAsset(const PhotoAssetChangeInfo &assetInfo, bool isHiddenAsset = false);
    
    // 视频资产
    static bool IsVideoAsset(const PhotoAssetChangeInfo &assetInfo);
    
    // 图片资产
    static bool IsImageAsset(const PhotoAssetChangeInfo &assetInfo);

    // 根据data_added字段判断compareAssetInfo比currentAssetInfo新
    static bool IsNewerByDateAdded(const PhotoAssetChangeInfo &compareAssetInfo,
        const PhotoAssetChangeInfo &currentAssetInfo, bool isAsc = true);

    // 根据date_taken字段判断compareAssetInfo比currentAssetInfo新
    static bool IsNewerByDateTaken(const PhotoAssetChangeInfo &compareAssetInfo,
        const PhotoAssetChangeInfo &currentAssetInfo, bool isAsc = true);

    // 根据hidden_time字段判断compareAssetInfo比currentAssetInfo新
    static bool IsNewerByHiddenTime(const PhotoAssetChangeInfo &compareAssetInfo,
        const PhotoAssetChangeInfo &currentAssetInfo, bool isAsc = true);

    // 根据display_name字段判断是否变化
    static bool IsNameChange(const PhotoAssetChangeData &assetChangeData);

    // 根据size字段判断是否变化
    static bool IsSizeChange(const PhotoAssetChangeData &assetChangeData);

    // 无效资产
    static bool IsInvalidAsset(const PhotoAssetChangeInfo &assetInfo);
    static bool UpdateCover(const PhotoAssetChangeData &assetChangeData,
        std::function<bool(const PhotoAssetChangeInfo&)> isAlbumAsset,
        std::function<bool(const PhotoAssetChangeInfo&, const PhotoAssetChangeInfo&)> isNewerAsset,
        PhotoAssetChangeInfo &addCover, std::unordered_set<int32_t> &removeFileIds);
    static bool UpdateCount(const PhotoAssetChangeData &assetChangeData,
        std::function<bool(PhotoAssetChangeInfo)> isAlbumAsset, int32_t &count);
    static bool CalAlbumRefreshInfo(std::function<bool(AlbumRefreshInfo &)> calFunc, AlbumRefreshInfo &refreshInfo,
        int32_t albumId, bool isHidden, AlbumRefreshTimestamp &assetTimestamp);
};

} // namespace Media
} // namespace OHOS

#endif
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

#ifndef OHOS_MEDIALIBRARY_SYSTEM_ALBUM_INFO_CALCULATION_H
#define OHOS_MEDIALIBRARY_SYSTEM_ALBUM_INFO_CALCULATION_H

#include <functional>
#include <map>
#include <vector>

#include "photo_asset_change_info.h"
#include "album_change_info.h"

namespace OHOS {
namespace Media::AccurateRefresh {

class SystemAlbumInfoCalculation {
public:
    SystemAlbumInfoCalculation(std::function<bool(const PhotoAssetChangeInfo&)> isSystemAsset,
        std::function<bool(const PhotoAssetChangeInfo&)> isVideoAsset,
        std::function<bool(const PhotoAssetChangeInfo&)> isHiddenSystemAsset,
        std::function<bool(const PhotoAssetChangeInfo&, const PhotoAssetChangeInfo&)> isNewerAsset,
        std::function<bool(const PhotoAssetChangeInfo&, const PhotoAssetChangeInfo&)> isNewerHiddenAsset_,
        int32_t subType) :isSystemAsset_(isSystemAsset), isVideoAsset_(isVideoAsset),
        isHiddenSystemAsset_(isHiddenSystemAsset), isNewerAsset_(isNewerAsset),
        isNewerHiddenAsset_(isNewerHiddenAsset_), subType_(subType) { }
    bool CalAlbumRefreshInfo(const PhotoAssetChangeData &assetChangeData, AlbumRefreshInfo &refreshInfo,
        int32_t albumId);

private:
    bool UpdateCover(const PhotoAssetChangeData &assetChangeData,
        std::function<bool(const PhotoAssetChangeInfo&)> isSystemAsset,
        std::function<bool(const PhotoAssetChangeInfo&, const PhotoAssetChangeInfo&)> isNewerAsset,
        PhotoAssetChangeInfo &addCover, std::unordered_set<int32_t> &removeFileIds);
    bool UpdateCount(const PhotoAssetChangeData &assetChangeData,
        std::function<bool(PhotoAssetChangeInfo)> isSystemAsset, int32_t &count);
    bool UpdateRefreshNormalInfo(const PhotoAssetChangeData &assetChangeData, AlbumRefreshInfo& refreshInfo);
    bool UpdateRefreshHiddenInfo(const PhotoAssetChangeData &assetChangeData, AlbumRefreshInfo& refreshInfo);
    bool IsSystemAlbumInfoChange(const PhotoAssetChangeData &assetChangeData,
        std::function<bool(PhotoAssetChangeInfo)> isAlbumAsset);
    static bool IsNameChange(const PhotoAssetChangeData &assetChangeData,
        std::function<bool(PhotoAssetChangeInfo)> isSystemAsset);
    static bool IsSizeChange(const PhotoAssetChangeData &assetChangeData,
        std::function<bool(PhotoAssetChangeInfo)> isSystemAsset);

public:
    std::function<bool(const PhotoAssetChangeInfo&)> isSystemAsset_;
    std::function<bool(const PhotoAssetChangeInfo&)> isVideoAsset_;
    std::function<bool(const PhotoAssetChangeInfo&)> isHiddenSystemAsset_;
    std::function<bool(const PhotoAssetChangeInfo&, const PhotoAssetChangeInfo&)> isNewerAsset_;
    std::function<bool(const PhotoAssetChangeInfo&, const PhotoAssetChangeInfo&)> isNewerHiddenAsset_;
    int32_t subType_;
};

} // namespace Media
} // namespace OHOS

#endif
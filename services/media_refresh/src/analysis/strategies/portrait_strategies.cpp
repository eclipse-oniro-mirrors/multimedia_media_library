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

#include "portrait_strategies.h"

#include "analysis_strategy_registry.h"
#include "media_log.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_rdb_utils.h"
#include "result_set_utils.h"

namespace OHOS {
namespace Media {
namespace AccurateRefresh {

bool PortraitCoverStrategy::ShouldRefreshCover(const UpdateAlbumData &oldAlbum,
    const AnalysisAlbumRefreshInfo &info)
{
    if (info.isForceRefresh_) {
        MEDIA_INFO_LOG("Force refresh portrait album cover, id: %{public}d", oldAlbum.albumId);
        return true;
    }

    CHECK_AND_RETURN_RET_LOG(!isCurrentCoverDeleted(oldAlbum, info), true,
        "Invalid cover due to delete, id: %{public}d", oldAlbum.albumId);

    const int32_t newCount = oldAlbum.albumCount + info.deltaCount_;
    if (newCount > 0 && oldAlbum.albumCoverUri.empty()) {
        MEDIA_INFO_LOG("Refresh empty portrait album cover, id: %{public}d", oldAlbum.albumId);
        return true;
    }

    if (oldAlbum.albumCount == 0 && info.deltaCount_ > 0) {
        MEDIA_INFO_LOG("Recover from an empty portrait album, id: %{public}d", oldAlbum.albumId);
        return true;
    }
    return false;
}

/**
 * 人像封面挑选策略
 * 直接从数据库选当前最佳的人像封面
 */
bool PortraitCoverPickerStrategy::PickCover(const UpdateAlbumData &baseInfo, AnalysisAlbumRefreshInfo &info)
{
    MEDIA_INFO_LOG("PortraitCoverPickerStrategy::PickCover albumId=%{public}d", baseInfo.albumId);

    auto rdb = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdb != nullptr, false, "rdbStore null");

    auto resultSet = MediaLibraryRdbUtils::QueryPortraitAlbumCover(rdb, std::to_string(baseInfo.albumId));

    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "portrait resultSet null");

    std::string cover = MediaLibraryRdbUtils::GetCover(resultSet);

    // 写入新的封面结果
    MEDIA_INFO_LOG("PortraitCoverPickerStrategy oldCover: %{public}s, newCover: %{public}s",
        baseInfo.albumCoverUri.c_str(), cover.c_str());
    info.refreshCover_ = cover;

    return true;
}

static const auto PortraitStrategyReg =
    AnalysisStrategyRegistry::Register(PhotoAlbumSubType::PORTRAIT)
        .Effective<PortraitAlbumEffectiveStrategy>()
        .Count<PortraitCountStrategy>()
        .Cover<PortraitCoverStrategy>()
        .Picker<PortraitCoverPickerStrategy>()
        .UseDefaultPipeline()
        .Build();

static const auto GroupPhotoStrategyReg =
    AnalysisStrategyRegistry::Register(PhotoAlbumSubType::GROUP_PHOTO)
        .Effective<PortraitAlbumEffectiveStrategy>()
        .Count<PortraitCountStrategy>()
        .Cover<PortraitCoverStrategy>()
        .Picker<PortraitCoverPickerStrategy>()
        .UseDefaultPipeline()
        .Build();

} // namespace AccurateRefresh
} // namespace Media
} // namespace OHOS

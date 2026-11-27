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

#include "shooting_mode_strategies.h"

#include "analysis_strategy_registry.h"
#include "media_string_utils.h"
namespace OHOS {
namespace Media {
namespace AccurateRefresh {

int ShootingModeCountStrategy::HandleUpdateOperation(const PhotoAssetChangeData &data,
    const UpdateAlbumData &baseInfo) const
{
    CHECK_AND_RETURN_RET_LOG(MediaStringUtils::IsValidInteger(baseInfo.albumName), DELTA_NO_CHANGE,
        "Invalid shooting mode album, id: %{public}d. name: %{public}s", baseInfo.albumId, baseInfo.albumName.c_str());
    ShootingModeAlbumType currentType = static_cast<ShootingModeAlbumType>(std::stoi(baseInfo.albumName));

    auto infoBefore =  data.infoBeforeChange_;
    auto infoAfter =  data.infoAfterChange_;
    bool isTypeContainBefore = IsTypeExist(infoBefore, currentType);
    bool isTypeContainAfter = IsTypeExist(infoAfter, currentType);

    int delta = CountStrategyBase::HandleUpdateOperation(data, baseInfo);
    return HandleDeltaCountResult(delta, isTypeContainBefore, isTypeContainAfter, baseInfo);
}

bool ShootingModeCountStrategy::IsTypeExist(const PhotoAssetChangeInfo &info,
    ShootingModeAlbumType currentType) const
{
    auto types = ShootingModeAlbum::GetShootingModeAlbumOfAsset(
        info.subType_, info.mimeType_, info.movingPhotoEffectMode_,
        info.frontCamera_, info.shootingMode_);

    return std::find(types.begin(), types.end(), currentType) != types.end();
}

int ShootingModeCountStrategy::HandleDeltaCountResult(int delta, bool isTypeContainBefore,
    bool isTypeContainAfter, const UpdateAlbumData &baseInfo) const
{
    switch (delta) {
        case DELTA_ADD:
            CHECK_AND_RETURN_RET_LOG(isTypeContainAfter, DELTA_NO_CHANGE,
                "Type not in infoAfter, do not refresh count, id: %{public}d", baseInfo.albumId);
            return DELTA_ADD;

        case DELTA_REMOVE:
            CHECK_AND_RETURN_RET_LOG(isTypeContainBefore, DELTA_NO_CHANGE,
                "Type not in infoBefore, do not refresh count, id: %{public}d", baseInfo.albumId);
            return DELTA_REMOVE;

        case DELTA_NO_CHANGE:
            // 拍摄模式相册更新时，Photos 表刷新可能造成相册间迁移
            if (!isTypeContainBefore && isTypeContainAfter) {
                return DELTA_ADD;
            }
            if (isTypeContainBefore && !isTypeContainAfter) {
                return DELTA_REMOVE;
            }
            return DELTA_NO_CHANGE;

        default:
            MEDIA_ERR_LOG("Undefined delta=%{public}d", delta);
            return DELTA_NO_CHANGE;
    }
}

static const auto ShootingModeStrategyReg =
    AnalysisStrategyRegistry::Register(PhotoAlbumSubType::SHOOTING_MODE)
        .Effective<ShootingModeAlbumEffectiveStrategy>()
        .Count<ShootingModeCountStrategy>()
        .Cover<ShootingModeCoverStrategy>()
        .Picker<ShootingModeCoverPickerStrategy>()
        .UseDefaultPipeline()
        .Build();

} // namespace AccurateRefresh
} // namespace Media
} // namespace OHOS

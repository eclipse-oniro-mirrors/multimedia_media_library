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

#include "count_strategy.h"

#include "album_asset_helper.h"

namespace OHOS {
namespace Media {
namespace AccurateRefresh {

int32_t CountStrategyBase::CalcCountDelta(const PhotoAssetChangeData &data,
    const UpdateAlbumData &baseInfo, AnalysisAlbumRefreshInfo &info)
{
    int32_t delta = ComputeDelta(data, baseInfo);
    info.deltaCount_ += delta;
    return delta;
}

int CountStrategyBase::ComputeDelta(const PhotoAssetChangeData &data, const UpdateAlbumData &baseInfo) const
{
    RdbOperation oper = data.operation_;
    switch (oper) {
        case RDB_OPERATION_ADD:
            return HandleAddOperation(data, baseInfo);
        case RDB_OPERATION_REMOVE:
            return HandleRemoveOperation(data, baseInfo);
        case RDB_OPERATION_UPDATE:
            return HandleUpdateOperation(data, baseInfo);
        default:
            MEDIA_ERR_LOG("Invalid rdb operation, return with no change");
            return DELTA_NO_CHANGE;
    }
}

bool CountStrategyBase::IsVisibleSystemAsset(const PhotoAssetChangeInfo &assetInfo) const
{
    return AlbumAssetHelper::IsCommonSystemAsset(assetInfo, false, 0);
}

int CountStrategyBase::HandleAddOperation(const PhotoAssetChangeData &data,
    const UpdateAlbumData &baseInfo) const
{
    return IsVisibleSystemAsset(data.infoAfterChange_) ? DELTA_ADD : DELTA_NO_CHANGE;
}

int CountStrategyBase::HandleRemoveOperation(const PhotoAssetChangeData &data,
    const UpdateAlbumData &baseInfo) const
{
    return IsVisibleSystemAsset(data.infoBeforeChange_) ? DELTA_REMOVE : DELTA_NO_CHANGE;
}

int CountStrategyBase::HandleUpdateOperation(const PhotoAssetChangeData &data,
    const UpdateAlbumData &baseInfo) const
{
    bool isVisibleBefore = IsVisibleSystemAsset(data.infoBeforeChange_);
    bool isVisibleAfter = IsVisibleSystemAsset(data.infoAfterChange_);
    if (isVisibleBefore && !isVisibleAfter) {
        return DELTA_REMOVE;
    }
    if (!isVisibleBefore && isVisibleAfter) {
        return DELTA_ADD;
    }
    return DELTA_NO_CHANGE;
}

} // namespace AccurateRefresh
} // namespace Media
} // namespace OHOS

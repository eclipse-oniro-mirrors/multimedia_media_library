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
#ifndef OHOS_MEDIALIBRARY_EFFECTIVE_STRATEGY_REGISTER_H
#define OHOS_MEDIALIBRARY_EFFECTIVE_STRATEGY_REGISTER_H

#include "photo_asset_change_info.h"
#include "medialibrary_rdb_utils.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
namespace AccurateRefresh {

// 接口：判断该变更是否对相册有效
class IAlbumEffectiveStrategy {
public:
    virtual ~IAlbumEffectiveStrategy() = default;
    virtual bool IsEffective(const PhotoAssetChangeData &data, const UpdateAlbumData &baseInfo) = 0;
};

// 抽象基类：提供通用逻辑，可被不同相册类型复用
class AlbumEffectiveStrategyBase : public IAlbumEffectiveStrategy {
public:
    bool IsEffective(const PhotoAssetChangeData &data, const UpdateAlbumData &baseInfo) override
    {
        CHECK_AND_RETURN_RET_LOG(PreCheckDataInput(data), false, "Invalid data input, operation: %{public}d",
            static_cast<int32_t>(data.operation_));

        CHECK_AND_RETURN_RET_DEBUG_LOG(IsValidSystemAsset(data.infoBeforeChange_) ||
            IsValidSystemAsset(data.infoAfterChange_), false, "No info in data is valid");

        return IsEffectiveForCurrentStrategy(data, baseInfo);
    }

protected:
    // 通用预检查逻辑，先检查入参有效性，可被子类实现覆盖
    virtual bool PreCheckDataInput(const PhotoAssetChangeData &data) const
    {
        return data.GetFileId() != INVALID_INT32_VALUE && data.operation_ != RDB_OPERATION_UNDEFINED;
    }

    virtual bool IsValidSystemAsset(const PhotoAssetChangeInfo &assetInfo)
    {
        return (assetInfo.syncStatus_ == static_cast<int32_t> (SyncStatusType::TYPE_VISIBLE) &&
            assetInfo.cleanFlag_ == static_cast<int32_t> (CleanType::TYPE_NOT_CLEAN) &&
            assetInfo.timePending_ == 0 &&
            !assetInfo.isTemp_ &&
            assetInfo.burstCoverLevel_ == static_cast<int32_t> (BurstCoverLevelType::COVER)) ||
            assetInfo.fileId_ == INVALID_INT32_VALUE;
    }

    // 留给子类扩展的具体有效性判断
    virtual bool IsEffectiveForCurrentStrategy(const PhotoAssetChangeData &data, const UpdateAlbumData &baseInfo)
    {
        return true;
    };
};

} // namespace AccurateRefresh
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_EFFECTIVE_STRATEGY_REGISTER_H

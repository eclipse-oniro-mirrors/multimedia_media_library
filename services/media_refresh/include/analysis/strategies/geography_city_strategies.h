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
#ifndef OHOS_MEDIALIBRARY_GEOGRAPHY_CITY_STRATEGIES_H
#define OHOS_MEDIALIBRARY_GEOGRAPHY_CITY_STRATEGIES_H

#include "analysis_strategy_registry.h"
#include "count_strategy.h"
#include "cover_strategy.h"
#include "cover_picker_strategy.h"
#include "default_strategies.h"
#include "effective_strategy.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_rdb_utils.h"

namespace OHOS {
namespace Media {
namespace AccurateRefresh {

// 城市相册默认有效策略
using GeographyCityAlbumEffectiveStrategy = AlbumEffectiveStrategyBase;

// 城市相册计数策略：复用默认实现
using GeographyCityCountStrategy = DefaultCountStrategy;

// 城市相册封面变更策略：复用默认实现 Decision 逻辑
using GeographyCityCoverStrategy = CoverStrategyBase;

class GeographyCityCoverPickerStrategy : public CoverPickerStrategyBase {
protected:
    std::string QueryCover(const std::shared_ptr<MediaLibraryRdbStore> &rdb,
        const UpdateAlbumData& baseInfo) override;
}

} // namespace AccurateRefresh
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_GEOGRAPHY_CITY_STRATEGIES_H
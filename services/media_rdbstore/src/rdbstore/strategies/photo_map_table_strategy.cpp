/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "PhotoMapTableStrategy"

#include "photo_map_table_strategy.h"

#include "media_column.h"
#include "medialibrary_type_const.h"

using namespace OHOS::NativeRdb;

namespace OHOS::Media {
TableStrategyErrno PhotoMapTableStrategy::ExtendDeleteValues(NativeRdb::ValuesBucket& values,
    const TableStrategyConfig &config)
{
    if (config.enableDefault) {
        values.PutInt(PhotoMap::DIRTY, static_cast<int32_t>(DirtyType::TYPE_DELETED));
    }
    return TableStrategyErrno::STRATEGY_OK;
}

std::string PhotoMapTableStrategy::GetQueryFilter(const TableStrategyConfig &config) const
{
    return PhotoMap::TABLE + "." + PhotoMap::DIRTY + " != " + std::to_string(static_cast<int32_t>(
        DirtyTypes::TYPE_DELETED));
}
} // namespace OHOS::Media
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

#ifndef OHOS_RDB_TABLE_STRATEGY_MANAGER_H
#define OHOS_RDB_TABLE_STRATEGY_MANAGER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "medialibrary_errno.h"
#include "rdb_predicates.h"
#include "rdb_store.h"
#include "rdb_table_strategy.h"
#include "values_bucket.h"

namespace OHOS {
namespace Media {
class RdbTableStrategyManager {
#define EXPORT __attribute__ ((visibility ("default")))
public:
    EXPORT static RdbTableStrategyManager& GetInstance();
    EXPORT void RegisterStrategy(const std::string& tableName, std::shared_ptr<RdbTableStrategy> strategy);

    EXPORT int32_t ExtendInsertValues(const std::string& tableName, NativeRdb::ValuesBucket& values,
        NativeRdb::RdbStore& store, const TableStrategyConfig &config);
    EXPORT int32_t ExtendBatchInsertValues(const std::string& tableName, std::vector<NativeRdb::ValuesBucket>& values,
        NativeRdb::RdbStore& store, const TableStrategyConfig &config);
    EXPORT TableStrategyErrno ExtendDeleteValues(const std::string& tableName, NativeRdb::ValuesBucket& values,
        const TableStrategyConfig &config);
    EXPORT int32_t ExtendUpdateValues(const std::string& tableName, NativeRdb::ValuesBucket& values,
        const TableStrategyConfig &config);
    EXPORT int32_t ExtendQueryFilters(NativeRdb::AbsRdbPredicates &predicates, const TableStrategyConfig &config);

private:
    RdbTableStrategyManager();
    ~RdbTableStrategyManager() = default;
    RdbTableStrategyManager(const RdbTableStrategyManager&) = delete;
    RdbTableStrategyManager& operator=(const RdbTableStrategyManager&) = delete;

    std::shared_ptr<RdbTableStrategy> GetStrategy(const std::string& tableName);

    std::unordered_map<std::string, std::shared_ptr<RdbTableStrategy>> strategies_;
};
} // namespace Media
} // namespace OHOS

#endif // OHOS_RDB_TABLE_STRATEGY_MANAGER_H
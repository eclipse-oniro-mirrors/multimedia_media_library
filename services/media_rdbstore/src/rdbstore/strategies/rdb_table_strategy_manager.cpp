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

#define MLOG_TAG "RdbTableStrategyManager"

#include "rdb_table_strategy_manager.h"

#include "files_table_strategy.h"
#include "media_log.h"
#include "photo_album_table_strategy.h"
#include "photo_map_table_strategy.h"
#include "photos_table_strategy.h"

namespace OHOS::Media {
RdbTableStrategyManager& RdbTableStrategyManager::GetInstance()
{
    static RdbTableStrategyManager instance;
    return instance;
}

RdbTableStrategyManager::RdbTableStrategyManager()
{
    RegisterStrategy(PhotoColumn::PHOTOS_TABLE, std::make_shared<PhotosTableStrategy>());
    RegisterStrategy(PhotoAlbumColumns::TABLE, std::make_shared<PhotoAlbumTableStrategy>());

    RegisterStrategy(CONST_MEDIALIBRARY_TABLE, std::make_shared<FilesTableStrategy>());
    RegisterStrategy(PhotoMap::TABLE, std::make_shared<PhotoMapTableStrategy>());
}

void RdbTableStrategyManager::RegisterStrategy(const std::string& tableName, std::shared_ptr<RdbTableStrategy> strategy)
{
    strategies_[tableName] = strategy;
}

std::shared_ptr<RdbTableStrategy> RdbTableStrategyManager::GetStrategy(const std::string& tableName)
{
    auto it = strategies_.find(tableName);
    if (it == strategies_.end()) {
        MEDIA_DEBUG_LOG("strategy not exist, tableName: %{public}s.", tableName.c_str());
        return nullptr;
    }
    return it->second;
}

int32_t RdbTableStrategyManager::ExtendInsertValues(const std::string& tableName,
    NativeRdb::ValuesBucket& values, NativeRdb::RdbStore& store, const TableStrategyConfig &config)
{
    auto strategy = GetStrategy(tableName);
    if (strategy == nullptr) {
        return E_OK;
    }
    return strategy->ExtendInsertValues(values, store, config);
}

int32_t RdbTableStrategyManager::ExtendBatchInsertValues(const std::string& tableName,
    std::vector<NativeRdb::ValuesBucket>& values, NativeRdb::RdbStore& store, const TableStrategyConfig &config)
{
    auto strategy = GetStrategy(tableName);
    if (strategy == nullptr) {
        return E_OK;
    }
    return strategy->ExtendBatchInsertValues(values, store, config);
}

TableStrategyErrno RdbTableStrategyManager::ExtendDeleteValues(const std::string& tableName,
    NativeRdb::ValuesBucket& values, const TableStrategyConfig &config)
{
    auto strategy = GetStrategy(tableName);
    if (strategy == nullptr) {
        return TableStrategyErrno::NO_SUCH_STRATEGY;
    }
    return strategy->ExtendDeleteValues(values, config);
}

int32_t RdbTableStrategyManager::ExtendUpdateValues(const std::string& tableName, NativeRdb::ValuesBucket& values,
    const TableStrategyConfig &config)
{
    auto strategy = GetStrategy(tableName);
    if (strategy == nullptr) {
        return E_OK;
    }
    return strategy->ExtendUpdateValues(values, config);
}

int32_t RdbTableStrategyManager::ExtendQueryFilters(NativeRdb::AbsRdbPredicates &predicates,
    const TableStrategyConfig &config)
{
    /* build all-table vector */
    std::string tableName = predicates.GetTableName();
    std::vector<std::string> joinTables = predicates.GetJoinTableNames();
    joinTables.push_back(tableName);

    /* add filters */
    std::string filters;
    for (auto &table : joinTables) {
        auto strategy = GetStrategy(table);
        if (strategy == nullptr) {
            continue;
        }

        std::string filter = strategy->GetQueryFilter(config);
        if (filter.empty()) {
            continue;
        }

        if (filters.empty()) {
            filters += filter;
        } else {
            filters += " AND " + filter;
        }
    }

    if (filters.empty()) {
        return E_OK;
    }

    std::string queryCondition = predicates.GetWhereClause();
    queryCondition = queryCondition.empty() ? filters : filters + " AND " + queryCondition;
    predicates.SetWhereClause(queryCondition);
    return E_OK;
}
} // namespace OHOS::Media
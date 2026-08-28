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

#define MLOG_TAG "Media_Upgrade"

#include "media_library_upgrade_manager.h"

#include "media_log.h"
#include "medialibrary_rdbstore.h"
#include "media_library_upgrade_task_registry.h"
#include "media_library_upgrade_helper.h"
#include "result_set_utils.h"
#include <chrono>

namespace OHOS {
namespace Media {

UpgradeManager& UpgradeManager::GetInstance()
{
    static UpgradeManager instance;
    return instance;
}

int32_t UpgradeManager::Initialize(const UpgradeManagerConfig& config)
{
    observer_ = std::make_shared<DefaultUpgradeObserver>();
    executor_.SetObserver(observer_);
    executor_.SetRdbConfigPath(config.rdbConfigPath);
    executor_.SetUpgradeEventPath(config.upgradeEventPath);
    currentVersion_ = config.currentVersion;
    targetVersion_ = config.targetVersion;
    isCloned_ = config.isCloned;
    MEDIA_INFO_LOG("UpgradeManager initialized");
    return NativeRdb::E_OK;
}

void UpgradeManager::SetObserver(std::shared_ptr<IUpgradeObserver> observer)
{
    observer_ = observer;
    executor_.SetObserver(observer);
}

int32_t UpgradeManager::DoUpgrade(NativeRdb::RdbStore& store, bool isSync)
{
    int64_t startTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    if (currentVersion_ >= targetVersion_) {
        MEDIA_INFO_LOG("No upgrade needed, current version: %{public}d", currentVersion_);
        return NativeRdb::E_OK;
    }

    MEDIA_INFO_LOG("Start %{public}s upgrade from version %{public}d to %{public}d",
        isSync ? "sync" : "async", currentVersion_, targetVersion_);

    std::vector<std::shared_ptr<IUpgradeTask>> tasks =
        UpgradeTaskRegistry::GetInstance().GetTasksAfterVersion(currentVersion_, isSync);

    if (tasks.empty()) {
        MEDIA_INFO_LOG("No upgrade tasks to execute");
        return NativeRdb::E_OK;
    }

    // 执行升级任务
    int32_t ret = executor_.ExecuteTasks(tasks, store, currentVersion_, isSync);
    if (!isCloned_) {
        RdbUpgradeUtils::ReportUpgradeDfxMessages(startTime, currentVersion_, targetVersion_, isSync);
    }
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("ExecuteTasks failed");
        return ret;
    }

    MEDIA_INFO_LOG("Upgrade completed successfully");
    return NativeRdb::E_OK;
}

int32_t UpgradeManager::UpgradeSync(NativeRdb::RdbStore& store)
{
    return DoUpgrade(store, true);
}

int32_t UpgradeManager::UpgradeAsync(NativeRdb::RdbStore& store)
{
    return DoUpgrade(store, false);
}

bool UpgradeManager::IsSchemaSubsetByAttach(NativeRdb::RdbStore& mainStore,
    NativeRdb::RdbStore& subsetStore)
{
    // 获取主数据库的所有表名
    std::set<std::string> mainTables = GetTablesFromStore(mainStore);
    if (mainTables.empty()) {
        MEDIA_ERR_LOG("Failed to get tables from main store");
        return false;
    }

    // 获取子数据库的所有表名
    std::set<std::string> subsetTables = GetTablesFromStore(subsetStore);
    if (subsetTables.empty()) {
        MEDIA_ERR_LOG("Failed to get tables from subset store");
        return false;
    }

    const std::string DDMS_DATA = "ddms_data_search_aux_config";
    const std::string NATURALBASE = "naturalbase_rdb_aux_metadata";
    // 检查子数据库的表是否都在主数据库中
    for (const auto &tableName : subsetTables) {
        if (mainTables.find(tableName) == mainTables.end() && tableName != DDMS_DATA && tableName != NATURALBASE) {
            MEDIA_INFO_LOG("Table %{private}s not found in main store", tableName.c_str());
            return false;
        }
    }

    // 检查每个表的列是否都在主数据库中
    for (const auto& tableName : subsetTables) {
        if (tableName == DDMS_DATA || tableName == NATURALBASE) {
            continue;
        }
        std::set<std::string> mainColumns = GetColumnsFromStore(mainStore, tableName);
        std::set<std::string> subsetColumns = GetColumnsFromStore(subsetStore, tableName);

        if (mainColumns.empty() || subsetColumns.empty()) {
            MEDIA_ERR_LOG("Failed to get columns for table %{private}s", tableName.c_str());
            return false;
        }

        for (const auto& columnName : subsetColumns) {
            if (mainColumns.find(columnName) == mainColumns.end()) {
                MEDIA_INFO_LOG("Column %{private}s not found in table %{private}s of main store",
                    columnName.c_str(), tableName.c_str());
                return false;
            }
        }
    }

    return true;
}

std::set<std::string> UpgradeManager::GetTablesFromStore(NativeRdb::RdbStore& store)
{
    std::set<std::string> tables;
    std::string sql = "SELECT name FROM sqlite_master WHERE type=\'table\' AND name NOT LIKE \'sqlite_%\'";

    auto resultSet = store.QuerySql(sql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query tables from store");
        return tables;
    }

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string tableName;
        resultSet->GetString(0, tableName);
        tables.insert(tableName);
    }

    return tables;
}

std::set<std::string> UpgradeManager::GetColumnsFromStore(NativeRdb::RdbStore& store,
    const std::string& tableName)
{
    std::set<std::string> columns;
    std::string sql = "SELECT name FROM pragma_table_info(\'" + tableName + "\')";

    auto resultSet = store.QuerySql(sql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query columns for table %{private}s", tableName.c_str());
        return columns;
    }

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string columnName;
        resultSet->GetString(0, columnName);
        columns.insert(columnName);
    }

    return columns;
}

} // namespace Media
} // namespace OHOS
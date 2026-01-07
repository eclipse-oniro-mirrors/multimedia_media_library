/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "db_upgrade_utils.h"

#include <string>
#include <vector>

#include "rdb_store.h"
#include "media_log.h"
#include "result_set_utils.h"

namespace OHOS::Media {
namespace DataTransfer {
/**
 * @brief Check table exists or not.
 */
bool DbUpgradeUtils::IsTableExists(NativeRdb::RdbStore &store, const std::string &tableName)
{
    std::string querySql = this->SQL_SQLITE_MASTER_QUERY;
    std::vector<NativeRdb::ValueObject> bindArgs = {tableName};
    auto resultSet = store.QuerySql(querySql, bindArgs);
    CHECK_AND_RETURN_RET_WARN_LOG(resultSet != nullptr, false,
        "Query resultSql is null. tableName: %{public}s is not exists.", tableName.c_str());
    int rowCount = 0;
    bool isExists = !resultSet->GetRowCount(rowCount) && rowCount > 0;
    MEDIA_INFO_LOG("Media_Restore: tableName=%{public}s, isExists:%{public}d", tableName.c_str(), isExists);
    resultSet->Close();
    return isExists;
}

/**
 * @brief Check table column exists or not.
 */
bool DbUpgradeUtils::IsColumnExists(
    NativeRdb::RdbStore &store, const std::string &tableName, const std::string &columnName)
{
    std::string querySql = this->SQL_PRAGMA_TABLE_INFO_QUERY;
    std::vector<NativeRdb::ValueObject> bindArgs = {tableName, columnName};
    auto resultSet = store.QuerySql(querySql, bindArgs);
    CHECK_AND_RETURN_RET_WARN_LOG(resultSet != nullptr, false,
        "Query resultSql is null. tableName: %{public}s, columnName: %{public}s is not exists.",
        tableName.c_str(), columnName.c_str());
    int rowCount = 0;
    bool isExists = !resultSet->GetRowCount(rowCount) && rowCount > 0;
    MEDIA_INFO_LOG("Media_Restore: tableName=%{public}s, columnName=%{public}s, isExists:%{public}d",
        tableName.c_str(),
        columnName.c_str(),
        isExists);
    resultSet->Close();
    return isExists;
}

std::vector<std::string> DbUpgradeUtils::GetAllTriggers(NativeRdb::RdbStore &store, const std::string &tableName)
{
    std::string querySql = this->SQL_SQLITE_MASTER_QUERY_TRIGGER;
    std::vector<NativeRdb::ValueObject> bindArgs = {tableName};
    auto resultSet = store.QuerySql(querySql, bindArgs);
    std::vector<std::string> result;
    CHECK_AND_RETURN_RET_WARN_LOG(resultSet != nullptr, result,
        "Query resultSql is null. tableName: %{public}s does not have any triggers.", tableName.c_str());
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string triggerName = GetStringVal("name", resultSet);
        result.emplace_back(triggerName);
    }
    resultSet->Close();
    return result;
}

int32_t DbUpgradeUtils::DropAllTriggers(NativeRdb::RdbStore &store, const std::string &tableName)
{
    const std::vector<std::string> triggerNames = this->GetAllTriggers(store, tableName);
    std::string prefix = "DROP TRIGGER IF EXISTS ";
    for (auto &triggerName : triggerNames) {
        std::string deleteTriggerSql = prefix + triggerName + ";";
        int32_t ret = store.ExecuteSql(deleteTriggerSql);
        CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK,
            "Media_Restore: Drop trigger failed, triggerName=%{public}s", triggerName.c_str());
    }
    return NativeRdb::E_OK;
}
std::vector<std::string> DbUpgradeUtils::GetAllUniqueIndex(NativeRdb::RdbStore &store, const std::string &tableName)
{
    std::string querySql = this->SQL_SQLITE_MASTER_QUERY_UNIQUE_INDEX;
    std::vector<NativeRdb::ValueObject> bindArgs = {tableName};
    auto resultSet = store.QuerySql(querySql, bindArgs);
    std::vector<std::string> result;
    CHECK_AND_RETURN_RET_WARN_LOG(resultSet != nullptr, result,
        "resultSet is null. tableName: %{public}s does not have any unique index.", tableName.c_str());
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string uniqueIndexName = GetStringVal("name", resultSet);
        result.emplace_back(uniqueIndexName);
    }
    resultSet->Close();
    return result;
}

int32_t DbUpgradeUtils::DropAllUniqueIndex(NativeRdb::RdbStore &store, const std::string &tableName)
{
    const std::vector<std::string> uniqueIndexNames = this->GetAllUniqueIndex(store, tableName);
    std::string prefix = "DROP INDEX IF EXISTS ";
    for (auto &indexName : uniqueIndexNames) {
        std::string deleteIndexSql = prefix + indexName + ";";
        int32_t ret = store.ExecuteSql(deleteIndexSql);
        CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK,
            "Media_Restore: Drop trigger failed, indexName=%{public}s", indexName.c_str());
    }
    return NativeRdb::E_OK;
}
}  // namespace DataTransfer
}  // namespace OHOS::Media
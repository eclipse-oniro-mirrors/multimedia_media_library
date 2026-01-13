/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include "database_utils.h"

#include <string>
#include <vector>

#include "media_log.h"
#include "result_set_utils.h"
#include "media_file_utils.h"

namespace OHOS::Media {
/**
 * @brief Check table column exists or not.
 */
bool DatabaseUtils::IsColumnExists(
    std::shared_ptr<MediaLibraryRdbStore> store, const std::string &tableName, const std::string &columnName)
{
    CHECK_AND_RETURN_RET_LOG(store != nullptr, false, "store is null");
    std::string querySql = this->SQL_PRAGMA_TABLE_INFO_QUERY;
    std::vector<NativeRdb::ValueObject> bindArgs = {tableName, columnName};
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    auto resultSet = store->QuerySql(querySql, bindArgs);
    CHECK_AND_RETURN_RET_WARN_LOG(resultSet != nullptr,
        false,
        "Query resultSql is null. tableName: %{public}s, columnName: %{public}s is not exists.",
        tableName.c_str(),
        columnName.c_str());
    int rowCount = 0;
    bool isExists = !resultSet->GetRowCount(rowCount) && rowCount > 0;
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    MEDIA_INFO_LOG("tableName=%{public}s, columnName=%{public}s, isExists:%{public}d, costTime:%{public}" PRId64,
        tableName.c_str(),
        columnName.c_str(),
        isExists,
        costTime);
    return isExists;
}

std::vector<std::string> DatabaseUtils::GetAllTriggers(
    std::shared_ptr<MediaLibraryRdbStore> store, const std::string &tableName)
{
    std::string querySql = this->SQL_SQLITE_MASTER_QUERY_TRIGGER;
    std::vector<NativeRdb::ValueObject> bindArgs = {tableName};
    auto resultSet = store->QuerySql(querySql, bindArgs);
    std::vector<std::string> result;
    CHECK_AND_RETURN_RET_WARN_LOG(resultSet != nullptr,
        result,
        "Query resultSql is null. tableName: %{public}s does not have any triggers.",
        tableName.c_str());
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string triggerName = GetStringVal("name", resultSet);
        result.emplace_back(triggerName);
    }
    return result;
}

bool DatabaseUtils::IsTriggerExists(
    std::shared_ptr<MediaLibraryRdbStore> store, const std::string &tableName, const std::string &triggerName)
{
    std::vector<std::string> triggers = this->GetAllTriggers(store, tableName);
    return std::find(triggers.begin(), triggers.end(), triggerName) != triggers.end();
}
}  // namespace OHOS::Media
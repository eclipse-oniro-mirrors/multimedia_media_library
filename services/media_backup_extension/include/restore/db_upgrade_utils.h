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
#ifndef OHOS_MEDIA_DATATRANSFER_DB_UPGRADE_UTILS_H
#define OHOS_MEDIA_DATATRANSFER_DB_UPGRADE_UTILS_H

#include <string>

#include "rdb_store.h"

namespace OHOS::Media {
namespace DataTransfer {
class DbUpgradeUtils {
public:
    bool IsTableExists(NativeRdb::RdbStore &store, const std::string &tableName);
    bool IsColumnExists(NativeRdb::RdbStore &store, const std::string &tableName, const std::string &columnName);
    int32_t DropAllTriggers(NativeRdb::RdbStore &store, const std::string &tableName);
    int32_t DropAllUniqueIndex(NativeRdb::RdbStore &store, const std::string &tableName);

private:
    std::vector<std::string> GetAllTriggers(NativeRdb::RdbStore &store, const std::string &tableName);
    std::vector<std::string> GetAllUniqueIndex(NativeRdb::RdbStore &store, const std::string &tableName);

private:
    const std::string SQL_PRAGMA_TABLE_INFO_QUERY = "\
        SELECT \
            name, \
            type, \
            [notnull], \
            dflt_value, \
            pk \
        FROM \
            pragma_table_info(?) \
        WHERE name = ?;";
    const std::string SQL_SQLITE_MASTER_QUERY = "\
        SELECT \
            type, \
            name, \
            tbl_name \
        FROM sqlite_master \
        WHERE type='table' AND \
            tbl_name = ?;";
    const std::string SQL_SQLITE_MASTER_QUERY_TRIGGER = "\
        SELECT \
            type, \
            name, \
            tbl_name \
        FROM sqlite_master \
        WHERE \
            type='trigger' AND \
            tbl_name = ?;";
    const std::string SQL_SQLITE_MASTER_QUERY_UNIQUE_INDEX = "\
        SELECT \
            type, \
            name, \
            tbl_name \
        FROM sqlite_master \
        WHERE \
            tbl_name = ? AND \
            type = 'index' AND \
            sql LIKE 'CREATE UNIQUE INDEX %';";
};
}  // namespace DataTransfer
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_DATATRANSFER_DB_UPGRADE_UTILS_H
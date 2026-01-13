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
#ifndef OHOS_MEDIA_EVENT_HANDLER_DATABASE_UTILS_H
#define OHOS_MEDIA_EVENT_HANDLER_DATABASE_UTILS_H

#include <string>

#include "medialibrary_rdbstore.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
class EXPORT DatabaseUtils {
public:
    bool IsColumnExists(
        std::shared_ptr<MediaLibraryRdbStore> store, const std::string &tableName, const std::string &columnName);
    bool IsTriggerExists(
        std::shared_ptr<MediaLibraryRdbStore> store, const std::string &tableName, const std::string &triggerName);

private:
    std::vector<std::string> GetAllTriggers(std::shared_ptr<MediaLibraryRdbStore> store, const std::string &tableName);

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
    const std::string SQL_SQLITE_MASTER_QUERY_TRIGGER = "\
        SELECT \
            type, \
            name, \
            tbl_name \
        FROM sqlite_master \
        WHERE \
            type='trigger' AND \
            tbl_name = ?;";
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_EVENT_HANDLER_DB_UPGRADE_UTILS_H
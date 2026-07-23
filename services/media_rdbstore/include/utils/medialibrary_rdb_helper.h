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

#ifndef OHOS_MEDIALIBRARY_RDB_HELPER_H
#define OHOS_MEDIALIBRARY_RDB_HELPER_H

#include <string>
#include <vector>

#include "abs_rdb_predicates.h"
#include "rdb_store.h"
#include "values_bucket.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class MediaLibraryRdbHelper {
public:
    EXPORT static int32_t ExecSqlWithRetry(std::function<int32_t()> execSql);
    EXPORT static void BuildValuesSql(const NativeRdb::ValuesBucket &values,
        std::vector<NativeRdb::ValueObject> &bindArgs, std::string &sql);
    EXPORT static void BuildQuerySql(const NativeRdb::AbsRdbPredicates &predicates,
        const std::vector<std::string> &columns, std::vector<NativeRdb::ValueObject> &bindArgs, std::string &sql);
    EXPORT static void ReplacePredicatesUriToId(NativeRdb::AbsRdbPredicates &predicates);

    // 支持raw db操作
    EXPORT static bool HasColumnInTable(NativeRdb::RdbStore &store, const std::string &columnName,
        const std::string &tableName);
    EXPORT static void AddColumnIfNotExists(NativeRdb::RdbStore &store, const std::string &columnName,
        const std::string &columnType, const std::string &tableName);
    static int32_t QueryExistingShootingModeAlbumNames(NativeRdb::RdbStore& store,
        std::vector<std::string>& existingAlbumNames);
    static int32_t InsertShootingModeAlbumValues(const std::string& albumName, NativeRdb::RdbStore &store);
    EXPORT static int32_t PrepareShootingModeAlbum(NativeRdb::RdbStore &store);
};
} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_LIBRARY_RDB_HELPER_H
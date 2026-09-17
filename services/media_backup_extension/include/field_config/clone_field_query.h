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

#ifndef OHOS_MEDIA_CLONE_FIELD_QUERY_H
#define OHOS_MEDIA_CLONE_FIELD_QUERY_H

#include <string>
#include <unordered_map>

#include "field_config/clone_field_meta.h"
#include "rdb_helper.h"
#include "result_set.h"

namespace OHOS {
namespace Media {

class CloneFieldQuery {
public:
    static CloneFieldType GetType(const std::string &table, const std::string &col);
    static bool HasDefault(const std::string &table, const std::string &col);
    static bool ApplyDefault(NativeRdb::ValuesBucket &bucket, const std::string &table, const std::string &col);

    static FieldPolicy GetPolicy(const std::string &table, const std::string &col,
        CloneDirection dir, RecordPath path);
    static bool ShouldTakeSource(const std::string &table, const std::string &col,
        CloneDirection dir, RecordPath path);

    static bool IsNeeded(const std::string &table, const std::string &col);
    static std::string GetWhereClause(const std::string &table, const std::string &col, bool withCloud);

    static std::unordered_map<std::string, std::string> GetCommonColumns(const std::string &table,
        const std::shared_ptr<NativeRdb::RdbStore> &src,
        const std::shared_ptr<NativeRdb::RdbStore> &dst,
        CloneDirection dir, RecordPath path = RecordPath::INSERT);
    static std::unordered_map<std::string, std::string> GetCommonColumns(const std::string &table,
        const std::unordered_map<std::string, std::string> &srcColumns,
        const std::unordered_map<std::string, std::string> &dstColumns,
        CloneDirection dir, RecordPath path = RecordPath::INSERT);

    static const std::vector<std::string> &GetCloneAlbums();
    static std::string GetMapTable(const std::string &albumTable);

private:
    static bool HasSameColumn(const std::unordered_map<std::string, std::string> &columnInfoMap,
        const std::string &columnName, const std::string &columnType);
};

} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_CLONE_FIELD_QUERY_H

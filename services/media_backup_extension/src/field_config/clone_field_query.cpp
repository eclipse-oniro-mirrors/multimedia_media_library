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

#include "field_config/clone_field_query.h"

#include "backup_database_utils.h"
#include "field_config/clone_field_registry.h"

namespace OHOS {
namespace Media {

bool CloneFieldQuery::HasSameColumn(const std::unordered_map<std::string, std::string> &columnInfoMap,
    const std::string &columnName, const std::string &columnType)
{
    auto it = columnInfoMap.find(columnName);
    return it != columnInfoMap.end() && it->second == columnType;
}

CloneFieldType CloneFieldQuery::GetType(const std::string &table, const std::string &col)
{
    const CloneFieldMeta *m = CloneFieldRegistry::Instance().GetField(table, col);
    return m != nullptr ? m->type : CloneFieldType::UNKNOWN;
}

bool CloneFieldQuery::HasDefault(const std::string &table, const std::string &col)
{
    const CloneFieldMeta *m = CloneFieldRegistry::Instance().GetField(table, col);
    return m != nullptr && m->defaultValue.hasDefault;
}

bool CloneFieldQuery::ApplyDefault(NativeRdb::ValuesBucket &bucket, const std::string &table, const std::string &col)
{
    const CloneFieldMeta *m = CloneFieldRegistry::Instance().GetField(table, col);
    if (m == nullptr || !m->defaultValue.hasDefault || !m->defaultValue.generator) {
        return false;
    }
    m->defaultValue.generator(bucket, col);
    return true;
}

FieldPolicy CloneFieldQuery::GetPolicy(const std::string &table, const std::string &col,
    CloneDirection dir, RecordPath path)
{
    const CloneFieldMeta *m = CloneFieldRegistry::Instance().GetField(table, col);
    if (m == nullptr) {
        if (path == RecordPath::INSERT) {
            return FieldPolicy::INHERIT_SOURCE;
        }
        return FieldPolicy::KEEP_TARGET;
    }
    if (path == RecordPath::INSERT) {
        return dir == CloneDirection::FORWARD ? m->insertForward : m->insertReverse;
    }
    return dir == CloneDirection::FORWARD ? m->mergeForward : m->mergeReverse;
}

bool CloneFieldQuery::ShouldTakeSource(const std::string &table, const std::string &col,
    CloneDirection dir, RecordPath path)
{
    return GetPolicy(table, col, dir, path) == FieldPolicy::INHERIT_SOURCE;
}

bool CloneFieldQuery::IsNeeded(const std::string &table, const std::string &col)
{
    const CloneFieldMeta *m = CloneFieldRegistry::Instance().GetField(table, col);
    return m != nullptr && m->isNeeded;
}

std::string CloneFieldQuery::GetWhereClause(const std::string &table, const std::string &col, bool withCloud)
{
    const CloneFieldMeta *m = CloneFieldRegistry::Instance().GetField(table, col);
    if (m == nullptr) {
        return "";
    }
    if (withCloud && !m->whereClauseCloud.empty()) {
        return m->whereClauseCloud;
    }
    return m->whereClause;
}

std::unordered_map<std::string, std::string> CloneFieldQuery::GetCommonColumns(const std::string &table,
    const std::unordered_map<std::string, std::string> &srcColumns,
    const std::unordered_map<std::string, std::string> &dstColumns, CloneDirection dir, RecordPath path)
{
    std::unordered_map<std::string, std::string> commonColumns;
    for (auto it = dstColumns.begin(); it != dstColumns.end(); ++it) {
        if (!HasSameColumn(srcColumns, it->first, it->second)) {
            continue;
        }
        if (GetPolicy(table, it->first, dir, path) == FieldPolicy::SKIP) {
            continue;
        }
        if (dir == CloneDirection::FORWARD && path == RecordPath::INSERT && IsNeeded(table, it->first)) {
            const CloneFieldMeta *m = CloneFieldRegistry::Instance().GetField(table, it->first);
            if (m == nullptr || !m->neededException) {
                continue;
            }
        }
        commonColumns[it->first] = it->second;
    }
    return commonColumns;
}

std::unordered_map<std::string, std::string> CloneFieldQuery::GetCommonColumns(const std::string &table,
    const std::shared_ptr<NativeRdb::RdbStore> &src, const std::shared_ptr<NativeRdb::RdbStore> &dst,
    CloneDirection dir, RecordPath path)
{
    if (src == nullptr || dst == nullptr) {
        return {};
    }
    auto srcColumns = BackupDatabaseUtils::GetColumnInfoMap(src, table);
    auto dstColumns = BackupDatabaseUtils::GetColumnInfoMap(dst, table);
    return GetCommonColumns(table, srcColumns, dstColumns, dir, path);
}

const std::vector<std::string> &CloneFieldQuery::GetCloneAlbums()
{
    static const std::vector<std::string> albums = []() {
        CloneFieldRegistry::Instance().Init();
        std::vector<std::string> result;
        for (const auto &table : CloneFieldRegistry::Instance().GetRegisteredTables()) {
            const CloneTableMeta *meta = CloneFieldRegistry::Instance().GetTable(table);
            if (meta != nullptr && !meta->mapTable.empty()) {
                result.push_back(table);
            }
        }
        return result;
    }();
    return albums;
}

std::string CloneFieldQuery::GetMapTable(const std::string &albumTable)
{
    const CloneTableMeta *meta = CloneFieldRegistry::Instance().GetTable(albumTable);
    return (meta != nullptr) ? meta->mapTable : "";
}

} // namespace Media
} // namespace OHOS

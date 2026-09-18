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

#include "field_config/clone_field_writer.h"

#include <memory>

#include "backup_database_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"

namespace OHOS {
namespace Media {

namespace {
const std::unordered_map<std::string, ResultSetDataType> COLUMN_TYPE_MAP = {
    { "INT", ResultSetDataType::TYPE_INT32 },
    { "INTEGER", ResultSetDataType::TYPE_INT32 },
    { "BIGINT", ResultSetDataType::TYPE_INT64 },
    { "DOUBLE", ResultSetDataType::TYPE_DOUBLE },
    { "REAL", ResultSetDataType::TYPE_DOUBLE },
    { "TEXT", ResultSetDataType::TYPE_STRING },
    { "BLOB", ResultSetDataType::TYPE_BLOB },
};

ResultSetDataType SqlTypeToDataType(const std::string &sqlType)
{
    auto it = COLUMN_TYPE_MAP.find(sqlType);
    return it != COLUMN_TYPE_MAP.end() ? it->second : ResultSetDataType::TYPE_NULL;
}
} // namespace

void CloneFieldWriter::ReadAndPutByType(NativeRdb::ValuesBucket &bucket, const std::string &col,
    const std::string &sqlType, const std::shared_ptr<NativeRdb::ResultSet> &rs)
{
    if (rs == nullptr) {
        return;
    }
    int32_t columnIndex = 0;
    if (rs->GetColumnIndex(col, columnIndex) != 0) {
        return;
    }
    bool isNull = false;
    if (rs->IsColumnNull(columnIndex, isNull) != 0 || isNull) {
        return;
    }
    switch (SqlTypeToDataType(sqlType)) {
        case ResultSetDataType::TYPE_INT32: {
            int32_t v = 0;
            if (rs->GetInt(columnIndex, v) == E_OK) {
                bucket.PutInt(col, v);
            }
            break;
        }
        case ResultSetDataType::TYPE_INT64: {
            int64_t v = 0;
            if (rs->GetLong(columnIndex, v) == E_OK) {
                bucket.PutLong(col, v);
            }
            break;
        }
        case ResultSetDataType::TYPE_DOUBLE: {
            double v = 0;
            if (rs->GetDouble(columnIndex, v) == E_OK) {
                bucket.PutDouble(col, v);
            }
            break;
        }
        case ResultSetDataType::TYPE_STRING: {
            std::string v;
            if (rs->GetString(columnIndex, v) == E_OK) {
                bucket.PutString(col, v);
            }
            break;
        }
        case ResultSetDataType::TYPE_BLOB: {
            std::vector<uint8_t> v;
            if (rs->GetBlob(columnIndex, v) == E_OK) {
                bucket.PutBlob(col, v);
            }
            break;
        }
        default:
            break;
    }
}

void CloneFieldWriter::CopyCommonColumnsForInsert(NativeRdb::ValuesBucket &bucket, const std::string &table,
    const std::shared_ptr<NativeRdb::ResultSet> &rs,
    const std::unordered_map<std::string, std::string> &commonColumns, CloneDirection dir)
{
    for (const auto &it : commonColumns) {
        if (CloneFieldQuery::ShouldTakeSource(table, it.first, dir, RecordPath::INSERT)) {
            ReadAndPutByType(bucket, it.first, it.second, rs);
            continue;
        }
        CloneFieldQuery::ApplyDefault(bucket, table, it.first);
    }
}

bool CloneFieldWriter::BuildMergeUpdateBucket(NativeRdb::ValuesBucket &bucket, const std::string &table,
    const std::shared_ptr<NativeRdb::ResultSet> &srcRs,
    const std::unordered_map<std::string, std::string> &commonColumns, CloneDirection dir)
{
    bool hasSet = false;
    for (const auto &it : commonColumns) {
        FieldPolicy policy = CloneFieldQuery::GetPolicy(table, it.first, dir, RecordPath::MERGE);
        if (policy == FieldPolicy::KEEP_TARGET) {
            continue;
        }
        if (policy == FieldPolicy::INHERIT_SOURCE) {
            ReadAndPutByType(bucket, it.first, it.second, srcRs);
            hasSet = true;
        } else {
            if (CloneFieldQuery::ApplyDefault(bucket, table, it.first)) {
                hasSet = true;
            }
        }
    }
    return hasSet;
}

int32_t CloneFieldWriter::ApplyMergeUpdate(std::shared_ptr<NativeRdb::RdbStore> &targetDb, const std::string &table,
    const std::string &idColumn, int32_t targetId, NativeRdb::ValuesBucket &bucket)
{
    if (targetDb == nullptr) {
        return E_ERR;
    }
    auto predicates = std::make_unique<NativeRdb::AbsRdbPredicates>(table);
    predicates->EqualTo(idColumn, std::to_string(targetId));
    int32_t changeRows = 0;
    int32_t ret = BackupDatabaseUtils::Update(targetDb, changeRows, bucket, predicates);
    if (ret != E_OK || changeRows <= 0) {
        MEDIA_WARN_LOG("ApplyMergeUpdate failed, table=%{public}s, ret=%{public}d, rows=%{public}d",
            table.c_str(), ret, changeRows);
        return ret == E_OK ? E_ERR : ret;
    }
    return E_OK;
}

} // namespace Media
} // namespace OHOS

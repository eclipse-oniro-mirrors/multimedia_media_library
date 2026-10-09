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

#ifndef OHOS_MEDIA_CLONE_FIELD_WRITER_H
#define OHOS_MEDIA_CLONE_FIELD_WRITER_H

#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "field_config/clone_field_meta.h"
#include "field_config/clone_field_query.h"
#include "rdb_helper.h"
#include "result_set.h"

namespace OHOS {
namespace Media {

class CloneFieldWriter {
public:
    template<typename Variant>
    static void PutFromVariant(NativeRdb::ValuesBucket &bucket, const std::string &col, const Variant &val);

    template<typename T>
    static void PutIfPresent(NativeRdb::ValuesBucket &bucket, const std::string &col,
        const std::optional<T> &val);

    template<typename T, typename U>
    static void PutWithDefault(NativeRdb::ValuesBucket &bucket, const std::string &col,
        const std::optional<T> &val, const U &explicitDefault);
    template<typename T>
    static void PutWithDefault(NativeRdb::ValuesBucket &bucket, const std::string &col,
        const std::optional<T> &val, const std::string &table);

    template<typename T>
    static void PutIfInIntersection(NativeRdb::ValuesBucket &bucket, const std::string &col,
        const std::optional<T> &val, const std::unordered_set<std::string> &intersection);

    static void CopyCommonColumnsForInsert(NativeRdb::ValuesBucket &bucket, const std::string &table,
        const std::shared_ptr<NativeRdb::ResultSet> &rs,
        const std::unordered_map<std::string, std::string> &commonColumns, CloneDirection dir);

    static bool BuildMergeUpdateBucket(NativeRdb::ValuesBucket &bucket, const std::string &table,
        const std::shared_ptr<NativeRdb::ResultSet> &srcRs,
        const std::unordered_map<std::string, std::string> &commonColumns, CloneDirection dir);

    static int32_t ApplyMergeUpdate(std::shared_ptr<NativeRdb::RdbStore> &targetDb, const std::string &table,
        const std::string &idColumn, int32_t targetId, NativeRdb::ValuesBucket &bucket);

    template<typename ValMap>
    static bool BuildMergeUpdateBucketFromValMap(NativeRdb::ValuesBucket &bucket, const std::string &table,
        const ValMap &valMap, const std::unordered_map<std::string, std::string> &commonColumns,
        CloneDirection dir);

private:
    static void ReadAndPutByType(NativeRdb::ValuesBucket &bucket, const std::string &col,
        const std::string &sqlType, const std::shared_ptr<NativeRdb::ResultSet> &rs);
};

template<typename Variant>
void CloneFieldWriter::PutFromVariant(NativeRdb::ValuesBucket &bucket, const std::string &col, const Variant &val)
{
    std::visit([&](auto &&v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int32_t>) {
            bucket.PutInt(col, v);
        } else if constexpr (std::is_same_v<T, int64_t>) {
            bucket.PutLong(col, v);
        } else if constexpr (std::is_same_v<T, double>) {
            bucket.PutDouble(col, v);
        } else if constexpr (std::is_same_v<T, std::string>) {
            bucket.PutString(col, v);
        } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
            bucket.PutBlob(col, v);
        }
        }, val);
}

template<typename T>
void CloneFieldWriter::PutIfPresent(NativeRdb::ValuesBucket &bucket, const std::string &col,
    const std::optional<T> &val)
{
    if (!val.has_value()) {
        return;
    }
    if constexpr (std::is_same_v<std::decay_t<T>, int32_t>) {
        bucket.PutInt(col, *val);
    } else if constexpr (std::is_same_v<std::decay_t<T>, int64_t>) {
        bucket.PutLong(col, *val);
    } else if constexpr (std::is_same_v<std::decay_t<T>, double>) {
        bucket.PutDouble(col, *val);
    } else if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
        bucket.PutString(col, *val);
    } else if constexpr (std::is_same_v<std::decay_t<T>, std::vector<uint8_t>>) {
        bucket.PutBlob(col, *val);
    }
}

template<typename T, typename U>
void CloneFieldWriter::PutWithDefault(NativeRdb::ValuesBucket &bucket, const std::string &col,
    const std::optional<T> &val, const U &explicitDefault)
{
    if (val.has_value()) {
        PutIfPresent<T>(bucket, col, val);
        return;
    }
    PutIfPresent<T>(bucket, col, std::optional<T>(static_cast<T>(explicitDefault)));
}

template<typename T>
void CloneFieldWriter::PutWithDefault(NativeRdb::ValuesBucket &bucket, const std::string &col,
    const std::optional<T> &val, const std::string &table)
{
    if (val.has_value()) {
        PutIfPresent<T>(bucket, col, val);
        return;
    }
    CloneFieldQuery::ApplyDefault(bucket, table, col);
}

template<typename T>
void CloneFieldWriter::PutIfInIntersection(NativeRdb::ValuesBucket &bucket, const std::string &col,
    const std::optional<T> &val, const std::unordered_set<std::string> &intersection)
{
    if (intersection.count(col) > 0) {
        PutIfPresent<T>(bucket, col, val);
    }
}

template<typename ValMap>
bool CloneFieldWriter::BuildMergeUpdateBucketFromValMap(NativeRdb::ValuesBucket &bucket, const std::string &table,
    const ValMap &valMap, const std::unordered_map<std::string, std::string> &commonColumns, CloneDirection dir)
{
    bool hasSet = false;
    for (const auto &it : commonColumns) {
        FieldPolicy policy = CloneFieldQuery::GetPolicy(table, it.first, dir, RecordPath::MERGE);
        if (policy == FieldPolicy::KEEP_TARGET) {
            continue;
        }
        auto valIt = valMap.find(it.first);
        if (valIt == valMap.end()) {
            continue;
        }
        if (policy == FieldPolicy::INHERIT_SOURCE) {
            PutFromVariant(bucket, it.first, valIt->second);
            hasSet = true;
        } else {
            if (CloneFieldQuery::ApplyDefault(bucket, table, it.first)) {
                hasSet = true;
            }
        }
    }
    return hasSet;
}

} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_CLONE_FIELD_WRITER_H

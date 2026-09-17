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

#ifndef OHOS_MEDIA_CLONE_FIELD_META_H
#define OHOS_MEDIA_CLONE_FIELD_META_H

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "rdb_helper.h"
#include "result_set.h"

namespace OHOS {
namespace Media {

enum class CloneFieldType {
    INT32,
    INT64,
    DOUBLE,
    STRING,
    BLOB,
    UNKNOWN,
};

enum class CloneDirection {
    FORWARD,
    REVERSE,
};

enum class RecordPath {
    INSERT,
    MERGE,
};

enum class FieldPolicy {
    INHERIT_SOURCE,
    KEEP_TARGET,
    SKIP,
};

using DefaultValueGenerator = std::function<void(NativeRdb::ValuesBucket &, const std::string &)>;

struct CloneFieldDefault {
    bool hasDefault = false;
    DefaultValueGenerator generator;
};

struct CloneFieldMeta {
    std::string column;
    CloneFieldType type = CloneFieldType::UNKNOWN;
    CloneFieldDefault defaultValue;

    FieldPolicy insertForward = FieldPolicy::INHERIT_SOURCE;
    FieldPolicy insertReverse = FieldPolicy::INHERIT_SOURCE;
    FieldPolicy mergeForward = FieldPolicy::KEEP_TARGET;
    FieldPolicy mergeReverse = FieldPolicy::KEEP_TARGET;

    bool isNeeded = false;
    bool neededException = false;
    std::string whereClause;
    std::string whereClauseCloud;
};

struct CloneTableMeta {
    std::string table;
    std::string mapTable;
    std::vector<CloneFieldMeta> fields;
};

} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_CLONE_FIELD_META_H

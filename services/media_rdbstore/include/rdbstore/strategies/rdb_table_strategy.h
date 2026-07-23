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

#ifndef OHOS_RDB_TABLE_STRATEGY_H
#define OHOS_RDB_TABLE_STRATEGY_H

#include <stdint.h>

#include "medialibrary_errno.h"
#include "rdb_store.h"
#include "values_bucket.h"

namespace OHOS {
namespace Media {
class RdbTableStrategyManager;

enum class TableStrategyErrno : int32_t {
    NO_SUCH_STRATEGY = -1,
    STRATEGY_OK = 0,
    STRATEGY_FAILED = 1,
};

struct TableStrategyConfig {
    // 公共策略调用
    bool enableDefault = true;
    bool enableAccountId = false;

    // 仅query策略使用
    bool isAlbumRefresh = false;
};

class RdbTableStrategy {
    friend class RdbTableStrategyManager;

public:
    virtual ~RdbTableStrategy() = default;

    virtual std::string GetTableName() const = 0;

protected:
    virtual int32_t ExtendInsertValues(NativeRdb::ValuesBucket& values, NativeRdb::RdbStore &store,
        const TableStrategyConfig &config)
    {
        return E_OK;
    }

    virtual int32_t ExtendBatchInsertValues(std::vector<NativeRdb::ValuesBucket>& values, NativeRdb::RdbStore &store,
        const TableStrategyConfig &config)
    {
        return E_OK;
    }

    virtual TableStrategyErrno ExtendDeleteValues(NativeRdb::ValuesBucket& values, const TableStrategyConfig &config)
    {
        return TableStrategyErrno::STRATEGY_OK;
    }

    virtual int32_t ExtendUpdateValues(NativeRdb::ValuesBucket& values, const TableStrategyConfig &config)
    {
        return E_OK;
    }

    virtual std::string GetQueryFilter(const TableStrategyConfig &config) const
    {
        return "";
    }
};
} // namespace Media
} // namespace OHOS

#endif // OHOS_RDB_TABLE_STRATEGY_H
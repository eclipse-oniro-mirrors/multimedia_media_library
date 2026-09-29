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

#ifndef OHOS_PHOTO_ALBUM_TABLE_STRATEGY_H
#define OHOS_PHOTO_ALBUM_TABLE_STRATEGY_H

#include "rdb_table_strategy.h"

#include "photo_album_column.h"

namespace OHOS {
namespace Media {
class PhotoAlbumTableStrategy final : public RdbTableStrategy {
public:
    PhotoAlbumTableStrategy() = default;
    ~PhotoAlbumTableStrategy() override = default;

    virtual std::string GetTableName() const override
    {
        return PhotoAlbumColumns::TABLE;
    }

protected:
    virtual int32_t ExtendInsertValues(NativeRdb::ValuesBucket& values, NativeRdb::RdbStore &store,
        const TableStrategyConfig &config) override;
    virtual int32_t ExtendBatchInsertValues(std::vector<NativeRdb::ValuesBucket>& values, NativeRdb::RdbStore &store,
        const TableStrategyConfig &config) override;
    virtual TableStrategyErrno ExtendDeleteValues(NativeRdb::ValuesBucket& values,
        const TableStrategyConfig &config) override;
    virtual std::string GetQueryFilter(const TableStrategyConfig &config) const override;

private:
    int32_t AddCoverOrderValuesFromRecord(NativeRdb::ValuesBucket& values, NativeRdb::RdbStore &store);
};
} // namespace Media
} // namespace OHOS

#endif // OHOS_PHOTO_ALBUM_TABLE_STRATEGY_H
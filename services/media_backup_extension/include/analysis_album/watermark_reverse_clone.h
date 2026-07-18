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

#ifndef WATERMARK_REVERSE_CLONE_H
#define WATERMARK_REVERSE_CLONE_H

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include "backup_const.h"
#include "rdb_store.h"
#include "backup_database_utils.h"

namespace OHOS {
namespace Media {

struct WatermarkTbl {
    std::optional<int32_t> fileId;
    std::optional<int32_t> status;
    std::optional<int32_t> type;
    std::optional<double> validRegionX;
    std::optional<double> validRegionY;
    std::optional<double> validRegionWidth;
    std::optional<double> validRegionHeight;
    std::optional<std::string> algoVersion;
};

class WatermarkReverseClone {
public:
    WatermarkReverseClone(
        const std::shared_ptr<NativeRdb::RdbStore>& sourceRdb,
        const std::shared_ptr<NativeRdb::RdbStore>& destRdb);

    void ReverseClone();

private:
    bool ShouldSkipClone();
    std::vector<WatermarkTbl> QueryWatermarkTbl(int32_t offset,
        const std::vector<std::string>& commonColumns);
    std::shared_ptr<NativeRdb::RdbStore> GetSourceRdb() { return sourceRdb_; }
    std::shared_ptr<NativeRdb::RdbStore> GetTargetRdb() { return destRdb_; }
    void ParseWatermarkResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
        WatermarkTbl& tbl);
    int32_t InsertWatermarkByTable(std::vector<WatermarkTbl>& watermarkTbl);
    NativeRdb::ValuesBucket CreateValuesBucketFromWatermarkTbl(const WatermarkTbl& tbl);
    int32_t BatchInsertWithRetry(const std::string& tableName,
        std::vector<NativeRdb::ValuesBucket>& values, int64_t& rowNum);
    std::vector<int32_t> QueryExistingFileIds(const std::vector<int32_t>& fileIds);
    void FilterExistingWatermarks(std::vector<WatermarkTbl>& watermarkTbls);

    std::shared_ptr<NativeRdb::RdbStore> sourceRdb_;
    std::shared_ptr<NativeRdb::RdbStore> destRdb_;
    static constexpr int32_t QUERY_COUNT = 200;
    static constexpr int32_t SQL_BATCH_SIZE = 200;
};

} // namespace Media
} // namespace OHOS

#endif // WATERMARK_REVERSE_CLONE_H
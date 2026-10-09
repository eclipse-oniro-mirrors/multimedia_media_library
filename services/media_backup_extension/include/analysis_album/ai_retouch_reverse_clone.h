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

#ifndef AI_RETOUCH_REVERSE_CLONE_H
#define AI_RETOUCH_REVERSE_CLONE_H

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include "backup_const.h"
#include "rdb_store.h"
#include "backup_database_utils.h"

namespace OHOS {
namespace Media {

struct AiRetouchTbl {
    std::optional<int32_t> fileId;
    std::optional<int32_t> portraitRefine;
    std::optional<int32_t> passersRemove;
    std::optional<int32_t> reflectiveRemove;
    std::optional<int32_t> moireRemove;
    std::optional<int32_t> magicEmoji;
    std::optional<std::string> aiRetouchVersion;
    std::optional<std::string> magicEmojiVersion;
    std::optional<std::string> analysisVersion;
};

struct TotalAiRetouchTbl {
    std::optional<int32_t> fileId;
    std::optional<int32_t> aiRetouch;
    std::optional<int32_t> magicEmoji;
};

class AiRetouchReverseClone {
public:
    AiRetouchReverseClone(
        const std::shared_ptr<NativeRdb::RdbStore>& sourceRdb,
        const std::shared_ptr<NativeRdb::RdbStore>& destRdb);

    void ReverseClone();

private:
    bool ShouldSkipClone();
    std::vector<AiRetouchTbl> QueryAiRetouchTbl(int32_t offset,
        const std::vector<std::string>& commonColumns);
    std::shared_ptr<NativeRdb::RdbStore> GetSourceRdb() { return sourceRdb_; }
    std::shared_ptr<NativeRdb::RdbStore> GetTargetRdb() { return destRdb_; }
    void ParseAiRetouchResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
        AiRetouchTbl& tbl);
    int32_t InsertAiRetouchByTable(std::vector<AiRetouchTbl>& aiRetouchTbl);
    NativeRdb::ValuesBucket CreateValuesBucketFromAiRetouchTbl(const AiRetouchTbl& tbl);
    int32_t BatchInsertWithRetry(const std::string& tableName,
        std::vector<NativeRdb::ValuesBucket>& values, int64_t& rowNum);
    void UpdateTotalTableAiRetouchStatus();
    std::vector<TotalAiRetouchTbl> QueryTotalAiRetouchTbl(int32_t offset,
        const std::vector<std::string>& commonColumns);
    void ParseTotalAiRetouchResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
        TotalAiRetouchTbl& tbl);
    void UpdateTotalTableBatch(const std::vector<TotalAiRetouchTbl>& totalTbls);

    std::shared_ptr<NativeRdb::RdbStore> sourceRdb_;
    std::shared_ptr<NativeRdb::RdbStore> destRdb_;
    static constexpr int32_t QUERY_COUNT = 200;
    static constexpr int32_t SQL_BATCH_SIZE = 200;
};

} // namespace Media
} // namespace OHOS

#endif // AI_RETOUCH_REVERSE_CLONE_H
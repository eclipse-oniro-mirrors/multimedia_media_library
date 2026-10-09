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

#ifndef AI_RETOUCH_CLONE_H
#define AI_RETOUCH_CLONE_H

#include <string>
#include <vector>
#include <optional>
#include <type_traits>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "backup_const.h"
#include "rdb_store.h"
#include "backup_database_utils.h"

namespace OHOS {
namespace Media {
struct TotalAiRetouchTbl {
    std::optional<int32_t> fileId;
    std::optional<int32_t> aiRetouch;
    std::optional<int32_t> magicEmoji;
};

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

class AiRetouchClone {
public:
    AiRetouchClone(
        const std::shared_ptr<NativeRdb::RdbStore>& sourceRdb,
        const std::shared_ptr<NativeRdb::RdbStore>& destRdb,
        const std::unordered_map<int32_t, PhotoInfo>& photoInfoMap,
        const int64_t& maxTotalFileId,
        const std::string& taskId,
        bool isReverse = false);
    void CloneAiRetouchInfo();
    void ReverseCloneAiRetouchInfo();
private:
    void ParseAiRetouchFromResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
        AiRetouchTbl& aiRetouchTbl);
    void InsertAiRetouchInBatch(const std::string& fileIdClause, const std::string& inClause);
    std::vector<AiRetouchTbl> QueryAiRetouchTbl(const std::string& fileIds,
        const std::string& commonColumns);
    std::vector<AiRetouchTbl> ProcessAiRetouchTbls(std::vector<AiRetouchTbl>& aiRetouchTbls);
    void UpdateTotalInBatch(const std::string& fileIdClause, const std::string& inClause);
    void UpdateTotalTbl();
    void BatchUpdateTotal(const std::string& column, const std::string& value,
        const std::vector<int32_t>& fileId);
    void BatchInsertAiRetouch(const std::vector<AiRetouchTbl>& aiRetouchTbls);
    void ProcessTotalAiRetouchTbls(std::vector<TotalAiRetouchTbl>& totalAiRetouchTbls);
    std::vector<TotalAiRetouchTbl> QueryTotalAiRetouchTbl(const std::string& fileIdClause,
        const std::string& inClause);
    NativeRdb::ValuesBucket CreateValuesBucketFromAiRetouchTbl(const AiRetouchTbl& aiRetouchTbl);
    int32_t BatchInsertWithRetry(const std::string &tableName, std::vector<NativeRdb::ValuesBucket> &values,
        int64_t &rowNum);
    void ParseTotalAiRetouchFromResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
        TotalAiRetouchTbl& totalAiRetouchTbl);
    int64_t GetShouldEndTime();

    bool QueryAndInsertSourceAiRetouch();
    std::vector<int32_t> QuerySourceFileIds();
    std::vector<AiRetouchTbl> QuerySourceAiRetouch(const std::vector<int32_t>& fileIds,
        const std::string& commonColumns);
    bool InsertOrUpdateDestAiRetouch(const std::vector<AiRetouchTbl>& aiRetouchTbls);
    std::unordered_set<int32_t> QueryExistingDestFileIds(const std::vector<int32_t>& fileIds);
    bool UpdateDestAiRetouch(const std::vector<AiRetouchTbl>& aiRetouchTbls);
    bool InsertNewDestAiRetouch(const std::vector<AiRetouchTbl>& aiRetouchTbls);
    int32_t BatchUpdateWithRetry(const std::string& tableName,
        const std::vector<std::pair<NativeRdb::ValuesBucket, std::string>>& updates);
    bool HandleDuplicateAssetReplacement();
    bool BatchDeleteDestAiRetouch(const std::vector<int32_t>& destFileIds);
    bool BatchQueryAndInsertSourceAiRetouch(const std::vector<int32_t>& sourceFileIds,
        const std::unordered_map<int32_t, int32_t>& fileIdMapping);

private:
    std::shared_ptr<NativeRdb::RdbStore> sourceRdb_;
    std::shared_ptr<NativeRdb::RdbStore> destRdb_;
    const std::unordered_map<int32_t, PhotoInfo>& photoInfoMap_;
    int64_t maxTotalFileId_ = 0;
    std::string taskId_;
    int64_t aiRetouchNum_ = 0;
    uint32_t totalAiRetouchNum_ = 0;
    static constexpr size_t SQL_BATCH_SIZE = 1000;
    std::unordered_map<int32_t, std::vector<int32_t>> aiRetouchFileIdMap_;
    std::unordered_map<int32_t, std::vector<int32_t>> magicEmojiFileIdMap_;
    bool isReverse_ = false;
};
} // namespace Media
} // namespace OHOS

#endif // AI_RETOUCH_CLONE_H
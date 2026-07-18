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

#ifndef BEAUTY_SCORE_CLONE_BASE_H
#define BEAUTY_SCORE_CLONE_BASE_H

#include <optional>
#include <type_traits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "backup_const.h"
#include "backup_const_column.h"
#include "rdb_store.h"

namespace OHOS::Media {

struct BeautyScoreTbl {
    std::optional<int32_t> id;
    std::optional<int32_t> fileId;
    std::optional<int32_t> aestheticsScore;
    std::optional<std::string> aestheticsVersion;
    std::optional<double> prob;
    std::optional<std::string> analysisVersion;
    std::optional<int32_t> selectedFlag;
    std::optional<std::string> selectedAlgoVersion;
    std::optional<int32_t> selectedStatus;
    std::optional<int32_t> negativeFlag;
    std::optional<std::string> negativeAlgoVersion;
    std::optional<std::string> aestheticsAllVersion;
    std::optional<int32_t> aestheticsScoreAll;
    std::optional<int32_t> isFilteredHard;
    std::optional<double> clarityScoreAll;
    std::optional<double> saturationScoreAll;
    std::optional<double> luminanceScoreAll;
    std::optional<double> semanticsScore;
    std::optional<int32_t> isBlackWhiteStripe;
    std::optional<int32_t> isBlurry;
    std::optional<int32_t> isMosaic;
};

class BeautyScoreCloneBase {
public:
    BeautyScoreCloneBase() = default;
    virtual ~BeautyScoreCloneBase() = default;

    bool ExecuteClone();

    int64_t GetMigratedScoreCount() const { return migrateScoreNum_; }
    int64_t GetMigratedFileCount() const { return migrateScoreFileNumber_; }
    int64_t GetTotalTimeCost() const { return migrateScoreTotalTimeCost_; }

protected:
    virtual bool ShouldSkipClone(const std::string& fileIdInClause) = 0;
    virtual std::shared_ptr<NativeRdb::RdbStore> GetSourceRdb() = 0;
    virtual std::shared_ptr<NativeRdb::RdbStore> GetTargetRdb() = 0;
    virtual void ProcessBeautyScoreTbls(std::vector<BeautyScoreTbl>& beautyScoreTbls) = 0;
    virtual std::vector<int32_t> GetFileIdsForQuery() = 0;
    virtual std::vector<BeautyScoreTbl> FilterByMaxId(std::vector<BeautyScoreTbl>& beautyScoreTbls) = 0;
    virtual std::unordered_set<int32_t> QueryExistingFileIds(const std::vector<int32_t>& fileIds) = 0;
    virtual void DeleteExistingRecords(const std::vector<int32_t>& fileIds) = 0;

    std::vector<BeautyScoreTbl> QueryBeautyScoreTbl(
        const std::vector<int32_t>& fileIds, const std::vector<std::string>& commonColumns);
    void ParseBeautyScoreResultSet(
        const std::shared_ptr<NativeRdb::ResultSet>& resultSet, BeautyScoreTbl& tbl);
    std::unordered_set<int32_t> BatchInsertBeautyScores(std::vector<BeautyScoreTbl>& beautyScoreTbls);
    NativeRdb::ValuesBucket CreateValuesBucketFromBeautyScoreTbl(const BeautyScoreTbl& tbl);
    int32_t BatchInsertWithRetry(const std::string& tableName,
        std::vector<NativeRdb::ValuesBucket>& values, int64_t& rowNum);
    std::string BuildFileIdInClause(const std::vector<int32_t>& fileIds);
    std::vector<int32_t> GetBatchFileIds(const std::vector<int32_t>& fileIds, size_t offset);

    template<typename T>
    void PutIfPresent(NativeRdb::ValuesBucket& values, const std::string& columnName,
        const std::optional<T>& optionalValue)
    {
        if (optionalValue.has_value()) {
            if constexpr (std::is_same_v<std::decay_t<T>, int32_t>) {
                values.PutInt(columnName, optionalValue.value());
            } else if constexpr (std::is_same_v<std::decay_t<T>, int64_t>) {
                values.PutLong(columnName, optionalValue.value());
            } else if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
                values.PutString(columnName, optionalValue.value());
            } else if constexpr (std::is_same_v<std::decay_t<T>, double>) {
                values.PutDouble(columnName, optionalValue.value());
            }
        }
    }

    template<typename T, typename U>
    void PutWithDefault(NativeRdb::ValuesBucket& values, const std::string& columnName,
        const std::optional<T>& optionalValue, const U& defaultValue)
    {
        if (optionalValue.has_value()) {
            PutIfPresent(values, columnName, optionalValue);
        } else {
            PutIfPresent(values, columnName, std::optional<T>(static_cast<T>(defaultValue)));
        }
    }

protected:
    int64_t migrateScoreNum_ = 0;
    int64_t migrateScoreFileNumber_ = 0;
    int64_t migrateScoreTotalTimeCost_ = 0;
    static constexpr int32_t SQL_BATCH_SIZE = 200;
};

} // namespace OHOS::Media
#endif // BEAUTY_SCORE_CLONE_BASE_H
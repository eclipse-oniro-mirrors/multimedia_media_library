/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef SEARCH_INDEX_CLONE_H
#define SEARCH_INDEX_CLONE_H

#include <optional>
#include <type_traits>
#include <string>

#include "backup_const.h"
#include "rdb_store.h"

namespace OHOS::Media {
using IdSetPair = std::pair<std::unordered_set<int32_t>, std::unordered_set<int32_t>>;
struct AnalysisSearchIndexTbl {
    std::optional<int64_t> id;
    std::optional<int32_t> fileId;
    std::optional<std::string> data;
    std::optional<std::string> displayName;
    std::optional<double> latitude;
    std::optional<double> longitude;
    std::optional<int64_t> dateModified;
    std::optional<int32_t> photoStatus;
    std::optional<int32_t> cvStatus;
    std::optional<int32_t> geoStatus;
    std::optional<int32_t> version;
    std::optional<std::string> systemLanguage;
};

class SearchIndexClone {
public:
    SearchIndexClone(
        const std::shared_ptr<NativeRdb::RdbStore>& sourceRdb,
        const std::shared_ptr<NativeRdb::RdbStore>& destRdb,
        const std::unordered_map<int32_t, PhotoInfo>& photoInfoMap,
        int64_t maxSearchId);

    bool Clone();

    int64_t GetMigratedCount() const { return migratedCount_; }
    int64_t GetTotalTimeCost() const { return totalTimeCost_; }

private:
    std::vector<AnalysisSearchIndexTbl> QueryAnalysisSearchIndexTbl(const std::string &fileIdClause,
        const std::vector<std::string>& commonColumns);

    void ParseAnalysisSearchIndexResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
        AnalysisSearchIndexTbl& analysisSearchIndexTbl);

    std::vector<AnalysisSearchIndexTbl> ProcessSearchIndexTbls(
        const std::vector<AnalysisSearchIndexTbl>& searchIndexTbls);
    void InsertAnalysisSearchIndex(std::vector<AnalysisSearchIndexTbl>& analysisSearchIndexTbl);

    IdSetPair QueryExistingIdsWithStrategy(const std::vector<int32_t>& fileIds);

    std::unordered_set<int32_t> ExecuteIdQuery(const std::string& querySql);
    void DeleteOverrideRecords(const std::vector<int32_t>& fileIds);

    int32_t InsertSearchIndexByTable(std::vector<AnalysisSearchIndexTbl>& analysisSearchIndexTbl);

    std::vector<NativeRdb::ValuesBucket> GetInsertSearchIndexValues(std::vector<AnalysisSearchIndexTbl>&
        analysisSearchIndexTbl);

    NativeRdb::ValuesBucket GetInsertSearchIndexValue(const AnalysisSearchIndexTbl& searchIndexInfo);

    int32_t BatchInsertWithRetry(const std::string &tableName,
        std::vector<NativeRdb::ValuesBucket> &values, int64_t &rowNum);

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

private:
    std::shared_ptr<NativeRdb::RdbStore> sourceRdb_;
    std::shared_ptr<NativeRdb::RdbStore> destRdb_;
    const std::unordered_map<int32_t, PhotoInfo>& photoInfoMap_;
    int64_t maxSearchId_;
    int64_t migratedCount_ = 0;
    int64_t totalTimeCost_ = 0;
};
} // namespace OHOS::Media
#endif // SEARCH_INDEX_CLONE_H
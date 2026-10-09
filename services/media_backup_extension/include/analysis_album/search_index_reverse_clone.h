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

#ifndef SEARCH_INDEX_REVERSE_CLONE_H
#define SEARCH_INDEX_REVERSE_CLONE_H

#include "backup_const.h"
#include "rdb_store.h"

namespace OHOS::Media {

class SearchIndexReverseClone {
public:
    SearchIndexReverseClone(
        const std::shared_ptr<NativeRdb::RdbStore>& sourceRdb,
        const std::shared_ptr<NativeRdb::RdbStore>& destRdb);

    void ReverseClone();

protected:
    bool ShouldSkipClone(const std::string& fileIdInClause = "");
    std::vector<AnalysisSearchIndexTbl> QueryAnalysisSearchIndexTbl(const std::vector<std::string>& commonColumns);
    std::shared_ptr<NativeRdb::RdbStore> GetSourceRdb() { return sourceRdb_; }
    std::shared_ptr<NativeRdb::RdbStore> GetTargetRdb() { return destRdb_; }
    void ParseAnalysisSearchIndexResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
        AnalysisSearchIndexTbl& tbl);
    int32_t InsertSearchIndexByTable(std::vector<AnalysisSearchIndexTbl>& searchIndexTbl);
    NativeRdb::ValuesBucket CreateValuesBucketFromSearchIndexTbl(const AnalysisSearchIndexTbl& tbl);
    int32_t BatchInsertWithRetry(const std::string& tableName, std::vector<NativeRdb::ValuesBucket>& values,
        int64_t& rowNum);

private:
    std::shared_ptr<NativeRdb::RdbStore> sourceRdb_;      // 新机数据库
    std::shared_ptr<NativeRdb::RdbStore> destRdb_;        // 旧机数据库
};

} // namespace OHOS::Media
#endif // SEARCH_INDEX_REVERSE_CLONE_H
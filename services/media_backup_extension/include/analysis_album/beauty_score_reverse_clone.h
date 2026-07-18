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

#ifndef BEAUTY_SCORE_REVERSE_CLONE_H
#define BEAUTY_SCORE_REVERSE_CLONE_H

#include "beauty_score_clone_base.h"
#include "rdb_store.h"

namespace OHOS::Media {

class BeautyScoreReverseClone {
public:
    BeautyScoreReverseClone(
        const std::shared_ptr<NativeRdb::RdbStore>& sourceRdb,
        const std::shared_ptr<NativeRdb::RdbStore>& destRdb);

    void ReverseClone();

private:
    bool ShouldSkipClone();
    std::vector<BeautyScoreTbl> QueryBeautyScoreTbl(const std::vector<std::string>& commonColumns);
    std::shared_ptr<NativeRdb::RdbStore> GetSourceRdb() { return sourceRdb_; }
    std::shared_ptr<NativeRdb::RdbStore> GetTargetRdb() { return destRdb_; }
    void ParseBeautyScoreResultSet(
        const std::shared_ptr<NativeRdb::ResultSet>& resultSet, BeautyScoreTbl& tbl);
    int32_t InsertBeautyScoreByTable(std::vector<BeautyScoreTbl>& beautyScoreTbl);
    NativeRdb::ValuesBucket CreateValuesBucketFromBeautyScoreTbl(const BeautyScoreTbl& tbl);
    int32_t BatchInsertWithRetry(const std::string& tableName, std::vector<NativeRdb::ValuesBucket>& values,
        int64_t& rowNum);
    void UpdateTotalTableAestheticsStatus();

    std::shared_ptr<NativeRdb::RdbStore> sourceRdb_;      // 新机数据库
    std::shared_ptr<NativeRdb::RdbStore> destRdb_;        // 旧机数据库
    static constexpr int32_t SQL_BATCH_SIZE = 200;
};

} // namespace OHOS::Media
#endif // BEAUTY_SCORE_REVERSE_CLONE_H
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

#ifndef CLONE_RESTORE_ANALYSIS_DATA_H
#define CLONE_RESTORE_ANALYSIS_DATA_H

#include <string>

#include "backup_const.h"
#include "clone_restore_analysis_total.h"
#include "media_backup_report_data_type.h"
#include "rdb_store.h"

namespace OHOS::Media {
class CloneRestoreAnalysisData {
public:
    struct AnalysisDataInfo {
        int32_t fileId;
        std::unordered_map<std::string, std::variant<int32_t, int64_t, double,
            std::string, std::vector<uint8_t>>> columnValMap;
    };

    void Init(int32_t sceneCode, const std::string &taskId, std::shared_ptr<NativeRdb::RdbStore> mediaRdb,
        std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, const std::string &totalTableName = "tab_analysis_total");
    std::unordered_map<std::string, std::string> GetTableCommonColumns(
        const std::unordered_set<std::string> &excludedColumns);
    void GetValFromResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, AnalysisDataInfo &info,
        const std::string &columnName, const std::string &columnType);
    void PrepareCommonColumnVal(NativeRdb::ValuesBucket &value, const std::string &columnName,
        const std::string &columnType,
        const std::variant<int32_t, int64_t, double, std::string, std::vector<uint8_t>> &columnVal);
    void GetAnalysisDataRowInfo(AnalysisDataInfo &info, const std::shared_ptr<NativeRdb::ResultSet> &resultSet);
    void GetAnalysisDataInfo();
    void GetAnalysisDataInsertValue(NativeRdb::ValuesBucket &value, const AnalysisDataInfo &info);
    int32_t BatchInsertWithRetry(const std::string &tableName, std::vector<NativeRdb::ValuesBucket> &values,
        int64_t &rowNum);
    void RemoveDuplicateInfos(const std::unordered_set<int32_t> &existingFileIds);
    std::unordered_set<int32_t> GetExistingFileIds();
    void DeleteDuplicateInfos();
    void InsertIntoAnalysisTable();
    void RestoreAnalysisDataMaps();
    void AnalysisDataRestoreBatch();
    void ReportRestoreTaskOfTotal();
    void ReportRestoreTaskofData();
    void ReportAnalysisTableRestoreTask();
    void GetMaxIds();
    void CloneAnalysisData(const std::string &table, const std::string &type,
        const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap,
        const std::unordered_set<std::string> &excludedColumns);

private:
    std::string ToUpper(const std::string &str)
    {
        std::string upperStr;
        std::transform(
            str.begin(), str.end(), std::back_inserter(upperStr), [](unsigned char c) { return std::toupper(c); });
        return upperStr;
    }

private:
    std::string table_;
    std::string analysisType_;
    int32_t sceneCode_{-1};
    std::string taskId_;
    int32_t maxId_{0};
    std::unordered_map<std::string, std::string> tableCommonColumns_;
    std::vector<AnalysisDataInfo> analysisDataInfos_;
    CloneRestoreAnalysisTotal cloneRestoreAnalysisTotal_;
    std::unordered_map<int32_t, PhotoInfo> photoInfoMap_;
    std::atomic<int64_t> restoreTimeCost_{0};
    int32_t totalCnt_{0};
    int32_t successCnt_{0};
    int32_t failCnt_{0};
    int32_t duplicateCnt_{0};
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb_;
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;
    std::string totalTableName_;
};
} // namespace OHOS::Media
#endif // CLONE_RESTORE_ANALYSIS_DATA_H
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

#ifndef CLONE_RESTORE_ANALYSIS_TOTAL_H
#define CLONE_RESTORE_ANALYSIS_TOTAL_H

#include <string>

#include "backup_const.h"
#include "media_backup_report_data_type.h"
#include "rdb_store.h"

namespace OHOS::Media {
class CloneRestoreAnalysisTotal {
public:
    void Init(const std::string &type, int32_t pageSize, std::shared_ptr<NativeRdb::RdbStore> mediaRdb,
        std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, const std::string &totalTableName = "tab_analysis_total");
    int32_t GetTotalNumber();
    void GetInfos(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap);
    void SetPlaceHoldersAndParamsByFileIdOld(std::string &placeHolders, std::vector<NativeRdb::ValueObject> &params);
    void SetPlaceHoldersAndParamsByFileIdNew(std::string &placeHolders, std::vector<NativeRdb::ValueObject> &params);
    size_t FindIndexByFileIdOld(int32_t fileIdOld);
    int32_t GetFileIdNewByIndex(size_t index);
    void UpdateRestoreStatusAsDuplicateByIndex(size_t index);
    void UpdateRestoreStatusAsFailed();
    void UpdateDatabase();
    void SetRestoreTaskInfo(RestoreTaskInfo &info);
    void AddSuccessVideoFileIds();
    std::vector<int32_t> GetSuccessVideoFileIds();
private:
    enum AnalysisStatus : int32_t {
        UNANALYZED = 0
    };
    enum RestoreStatus : int32_t {
        SUCCESS = 0,
        DUPLICATE,
        FAILED
    };
    struct AnalysisTotalInfo {
        int32_t fileIdOld {-1};
        int32_t fileIdNew {-1};
        int32_t status {AnalysisStatus::UNANALYZED};
        int32_t restoreStatus {RestoreStatus::SUCCESS};
    };

    std::unordered_map<int32_t, std::vector<std::string>> GetStatusFileIdsMap();
    int32_t UpdateDatabaseByStatus(int32_t status, const std::vector<std::string> &fileIds);

private:
    int32_t lastId_ {0};
    int32_t pageSize_ {0};
    int32_t totalCnt_{0};
    int32_t successCnt_{0};
    int32_t failedCnt_{0};
    int32_t duplicateCnt_{0};
    std::string type_;
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb_;
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;
    std::vector<AnalysisTotalInfo> analysisTotalInfos_;
    std::string totalTableName_;
    std::vector<int32_t> successVideoFileIds_;
};
} // namespace OHOS::Media
#endif // CLONE_RESTORE_ANALYSIS_TOTAL_H
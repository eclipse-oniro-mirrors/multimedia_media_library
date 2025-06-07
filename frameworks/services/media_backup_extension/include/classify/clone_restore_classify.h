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

#ifndef CLONE_RESTORE_CLASSIFY_H
#define CLONE_RESTORE_CLASSIFY_H

#include <string>

#include "backup_const.h"
#include "rdb_store.h"

namespace OHOS::Media {
class CloneRestoreClassify {
public:
    void Init(int32_t sceneCode, const std::string &taskId,
        std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, std::shared_ptr<NativeRdb::RdbStore> mediaRdb);
    void Restore(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap);

    void RestoreMaps();
    void RestoreVideoMaps();
    void ReportClassifyRestoreTask();

    template<typename T>
    static void PutIfPresent(NativeRdb::ValuesBucket& values, const std::string& columnName,
        const std::optional<T>& optionalValue);

    template<typename T>
    static void PutIfInIntersection(NativeRdb::ValuesBucket& values, const std::string& columnName,
        const std::optional<T>& optionalValue, const std::unordered_set<std::string> &intersection);

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
    struct ClassifyCloneInfo {
        std::optional<int32_t> id;
        std::optional<int32_t> fileIdOld;
        std::optional<int32_t> fileIdNew;
        std::optional<int32_t> categoryId;
        std::optional<std::string> subLabel;
        std::optional<double> prob;
        std::optional<std::string> feature;
        std::optional<std::string> simResult;
        std::optional<std::string> labelVersion;
        std::optional<std::string> saliencySubProb;
        std::optional<std::string> analysisVersion;
        std::optional<std::string> captionResult;
        std::optional<std::string> captionVersion;
    };
    struct ClassifyVideoCloneInfo {
        std::optional<int32_t> id;
        std::optional<int32_t> fileIdOld;
        std::optional<int32_t> fileIdNew;
        std::optional<std::string> categoryId;
        std::optional<double> confidenceProbability;
        std::optional<std::string> subCategory;
        std::optional<double> subConfidenceProb;
        std::optional<std::string> subLabel;
        std::optional<double> subLabelProb;
        std::optional<int32_t> subLabelType;
        std::optional<std::string> tracks;
        std::optional<std::vector<uint8_t>> videoPartFeature;
        std::optional<std::string> filterTag;
        std::optional<std::string> algoVersion;
        std::optional<std::string> analysisVersion;
        std::optional<int32_t> triggerGenerateThumbnail;
    };

    void GetClassifyInfos(std::vector<ClassifyCloneInfo> &classifyInfos);
    void GetClassifyVideoInfos(std::vector<ClassifyVideoCloneInfo> &classifyVideoInfos);
    void DeduplicateClassifyInfos(std::vector<ClassifyCloneInfo> &infos);
    void DeduplicateClassifyVideoInfos(std::vector<ClassifyVideoCloneInfo> &infos);
    std::unordered_set<int32_t> GetExistingFileIds(const std::string &tableName);
    void RemoveDuplicateClassifyInfos(std::vector<ClassifyCloneInfo> &infos,
        const std::unordered_set<int32_t> &existingFileIds);
    void RemoveDuplicateClassifyVideoInfos(std::vector<ClassifyVideoCloneInfo> &infos,
        const std::unordered_set<int32_t> &existingFileIds);
    void InsertClassifyAlbums(std::vector<ClassifyCloneInfo> &classifyInfos);
    void InsertClassifyVideoAlbums(std::vector<ClassifyVideoCloneInfo> &classifyVideoInfos);
    void UpdateAnalysisTotalInfosRestoreStatus(int32_t restoreStatus);

    void GetClassifyInfo(ClassifyCloneInfo &info, std::shared_ptr<NativeRdb::ResultSet> resultSet);
    void GetMapInsertValue(NativeRdb::ValuesBucket &value, ClassifyCloneInfo info,
        const std::unordered_set<std::string> &intersection);
    void GetClassifyVideoInfo(ClassifyVideoCloneInfo &info, std::shared_ptr<NativeRdb::ResultSet> resultSet);
    void GetVideoMapInsertValue(NativeRdb::ValuesBucket &value, ClassifyVideoCloneInfo info,
        const std::unordered_set<std::string> &intersection);

    bool CheckTableColumns(const std::string& tableName, std::unordered_map<std::string, std::string>& columns);
    std::unordered_set<std::string> GetCommonColumns(const std::string &tableName);
    int32_t BatchInsertWithRetry(const std::string &tableName, std::vector<NativeRdb::ValuesBucket> &values,
        int64_t &rowNum);
    
    void GetMaxIds();
    std::vector<int32_t> GetMinIdsOfAnalysisTotal();
    void RestoreBatch(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap, int32_t minId);
    void GetAnalysisTotalInfos(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap, int32_t minId);
    void UpdateAnalysisTotal();
    std::unordered_map<int32_t, std::vector<std::string>> GetAnalysisTotalStatusFileIdsMap();
    int32_t UpdateAnalysisTotalByStatus(int32_t status, const std::vector<std::string> &fileIds);

private:
    int32_t sceneCode_{-1};
    std::string taskId_;
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb_;
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;
    int32_t maxIdOfLabel_{0};
    int32_t maxIdOfVideoLabel_{0};
    std::atomic<int32_t> successInsertLabelCnt_{0};
    std::atomic<int32_t> successInsertVideoLabelCnt_{0};
    std::atomic<int32_t> failInsertLabelCnt_{0};
    std::atomic<int32_t> failInsertVideoLabelCnt_{0};
    std::atomic<int32_t> duplicateLabelCnt_{0};
    std::atomic<int32_t> duplicateVideoLabelCnt_{0};
    std::atomic<int64_t> restoreLabelTimeCost_{0};
    std::atomic<int64_t> restoreVideoLabelTimeCost_{0};
    std::vector<AnalysisTotalInfo> analysisTotalInfos_;
};

template<typename T>
void CloneRestoreClassify::PutIfPresent(NativeRdb::ValuesBucket& values, const std::string& columnName,
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
        } else if constexpr (std::is_same_v<std::decay_t<T>, std::vector<uint8_t>>) {
            values.PutBlob(columnName, optionalValue.value());
        }
    }
}

template<typename T>
void CloneRestoreClassify::PutIfInIntersection(NativeRdb::ValuesBucket& values, const std::string& columnName,
    const std::optional<T>& optionalValue, const std::unordered_set<std::string> &intersection)
{
    if (intersection.count(columnName) > 0) {
        PutIfPresent<T>(values, columnName, optionalValue);
    }
}
} // namespace OHOS::Media
#endif // CLONE_RESTORE_CLASSIFY_H
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

#ifndef CLONE_REVERSE_RESTORE_CLASSIFY_H
#define CLONE_REVERSE_RESTORE_CLASSIFY_H

#include "clone_restore_classify_base.h"

namespace OHOS::Media {

class CloneReverseRestoreClassify : public CloneRestoreClassifyBase {
public:
    void Init(const ClassifyCloneRestoreConfig& config);

    void Restore();

private:
    void RestoreReverseByVersion();
    void ReportReverseRestoreTask();
    void ReverseRestoreLabelAndTotalData();
    void GetImageClassifyInfos(std::vector<ClassifyCloneInfo> &classifyInfos);
    void InsertImageLabelData(std::vector<ClassifyCloneInfo> &insertInfos);
    void InsertClassifyInfosBatch(std::vector<ClassifyCloneInfo> &insertInfos,
        const std::unordered_set<std::string> &intersection);
    void InsertSingleBatch(std::vector<ClassifyCloneInfo> &insertInfos,
        size_t offset, const std::unordered_set<std::string> &intersection);
    void BuildInsertValue(ClassifyCloneInfo &info, const std::unordered_set<std::string> &intersection,
        std::vector<NativeRdb::ValuesBucket> &values);
    void GetVideoClassifyInfos(std::vector<ClassifyVideoCloneInfo> &classifyVideoInfos);
    void InsertVideoLabelData(std::vector<ClassifyVideoCloneInfo> &insertInfos);
    std::string GetFileIdsStr(const std::vector<ClassifyCloneInfo> &imageInfos);
    std::string GetFileIdsStr(const std::vector<ClassifyVideoCloneInfo> &videoInfos);
    void QueryAndUpdateTotal(const std::string& tableName, const std::string& fileIdClause);
    void AddReverseSpecialAlbum();
    void AddReverseSelfieAlbum();
    void AddReverseUserCommentAlbum();
    void RestoreAlbum();
    void InsertClassifyAlbumData();
    std::unordered_map<std::string, int32_t> QueryExistingAlbumNames();
    void RestoreMap();
    void QueryNewClassifyMaps();
    void InsertClassifyMapsToOldDb();
    void DeleteDuplicateAlbum(int32_t oldAlbumId, int32_t newAlbumId);

    void UpdateTabOldAlbumsId(int32_t oldAlbumId, int32_t newAlbumId);

private:
    bool isRestoreFromNewVersion_{false};
    int32_t maxIdOfLabel_{0};
    int32_t maxIdOfVideoLabel_{0};
    int64_t restoreTimeCost_{0};
    int64_t restoreLabelTimeCost_{0};
    int64_t restoreVideoLabelTimeCost_{0};
    int32_t successInsertLabelCnt_{0};
    int32_t successUpdateLabelCnt_{0};
    int32_t failInsertLabelCnt_{0};
    int32_t failUpdateLabelCnt_{0};
    int32_t maxAnalysisAlbumId_{0};
    std::vector<ClassifyAlbumInfo> classifyAlbumInfos_;
    std::vector<ClassifyMapInfo> classifyMapInfos_;
    std::unordered_map<int32_t, int32_t> albumIdMap_;
};

} // namespace OHOS::Media

#endif // CLONE_REVERSE_RESTORE_CLASSIFY_H
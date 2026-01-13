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

#ifndef CLONE_RESTORE_CV_ANALYSIS_H
#define CLONE_RESTORE_CV_ANALYSIS_H

#include <string>

#include "clone_restore_highlight.h"
#include "nlohmann/json.hpp"
#include "rdb_store.h"

namespace OHOS::Media {
class CloneRestoreCVAnalysis {
public:
    void Init(int32_t sceneCode, const std::string &taskId, std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
        std::shared_ptr<NativeRdb::RdbStore> mediaRdb, const std::string &backupRestoreDir);
    void RestoreAlbums(CloneRestoreHighlight &cloneHighlight);

private:
    void RestoreAssetSdMap(CloneRestoreHighlight &cloneHighlight);
    void RestoreAlbumAssetMap(CloneRestoreHighlight &cloneHighlight);
    void InsertIntoAssetSdMap(std::vector<NativeRdb::ValuesBucket> &values);
    void InsertIntoAlbumAssetMap(std::vector<NativeRdb::ValuesBucket> &values);
    void MoveAnalysisAssets(const std::string &srcPath, const std::string &dstPath);
    int32_t BatchInsertWithRetry(const std::string &tableName,
        const std::vector<NativeRdb::ValuesBucket> &values, int64_t &rowNum);
    std::string ParsePlayInfo(const std::string &oldPlayInfo, CloneRestoreHighlight &cloneHighlight);
    void ParseEffectline(nlohmann::json &newPlayInfo, CloneRestoreHighlight &cloneHighlight);
    void ProcessEffectVideoUri(nlohmann::json &effectlineInfoArray, size_t effectlineIndex,
        CloneRestoreHighlight &cloneHighlight);
    void ProcessTransitionVideoUri(nlohmann::json &effectlineInfoArray, size_t effectlineIndex,
        CloneRestoreHighlight &cloneHighlight);
    void ParseEffectlineFileData(nlohmann::json &effectlineInfoArray, size_t effectlineIndex,
        CloneRestoreHighlight &cloneHighlight);
    void ProcessIds(nlohmann::json &effectlineInfo, CloneRestoreHighlight &cloneHighlight);
    void ProcessUris(nlohmann::json &effectlineInfo, CloneRestoreHighlight &cloneHighlight);
    void ParseTimeline(nlohmann::json &newPlayInfo, CloneRestoreHighlight &cloneHighlight);
    void ProcessTimelineInfo(nlohmann::json &timelineInfo, CloneRestoreHighlight &cloneHighlight);
    void UpdateHighlightPlayInfos(CloneRestoreHighlight &cloneHighlight, std::vector<int32_t> &updateHighlightIds);
    void ReportCloneRestoreCVAnalysisTask();
    std::string GetNewEffectVideoUri(const std::string &oldVideoUri, CloneRestoreHighlight &cloneHighlight);
    std::string GetNewTransitionVideoUri(const std::string &oldVideoUri, CloneRestoreHighlight &cloneHighlight);
    std::string GetNewPhotoUriByUri(const std::string &oldUri, CloneRestoreHighlight &cloneHighlight);

    int32_t sceneCode_{-1};
    std::string taskId_;
    // old media_liabrary.db
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb_;
    // new media_liabrary.db
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;
    std::string assetPath_;
    std::string garblePath_;
    std::unordered_map<std::string, std::string> assetUriMap_;
    bool isHighlightVideoDirExist_{false};
    int64_t assetSdSuccessCnt_{0};
    int64_t assetSdFailedCnt_{0};
    int64_t albumAssetSuccessCnt_{0};
    int64_t albumAssetFailedCnt_{0};
    int64_t restoreTimeCost_{0};
};
} // namespace OHOS::Media
#endif // CLONE_RESTORE_CV_ANALYSIS_H
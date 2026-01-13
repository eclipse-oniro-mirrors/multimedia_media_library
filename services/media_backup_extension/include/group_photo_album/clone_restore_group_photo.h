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

#ifndef CLONE_RESTORE_GROUP_PHOTO_H
#define CLONE_RESTORE_GROUP_PHOTO_H

#include <string>
#include <sstream>

#include "backup_const.h"
#include "rdb_store.h"
#include "clone_restore_portrait_base.h"

namespace OHOS::Media {
class CloneRestoreGroupPhoto : public CloneRestorePortraitBase {
public:
    void Init(int32_t sceneCode, const std::string &taskId, std::string restoreInfo,
        std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
        std::shared_ptr<NativeRdb::RdbStore> mediaRdb, bool isCloudRestoreSatisfied);
    void Restore(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap);

private:
    int32_t RestoreGroupPhotoAlbumInfo();
    int32_t RestoreMaps();

    std::vector<GroupPhotoAlbumDfx> QueryGroupPhotoAlbumInOldDb(int32_t& offset, int32_t& rowCount);
    int32_t QueryGroupPhotoAlbumTbl(const std::vector<std::string>& commonColumns);
    std::unordered_set<std::string> QueryAllGroupTag();
    int32_t InsertGroupPhotoAlbum();
    void UpdateMapInsertValues(std::vector<NativeRdb::ValuesBucket> &values);
    void InsertAnalysisPhotoMap(std::vector<NativeRdb::ValuesBucket> &values);
    void RestoreMapsBatch();
    int32_t DeleteGroupPhotoAlbumInNewDb(const std::vector<std::string> &deletedAlbumIds);
    int32_t DeleteGroupPhotoMapInNewDb(const std::vector<std::string> &deletedAlbumIds);
    int32_t DeleteGroupPhotoAlbumInfoInNewDb();

    NativeRdb::ValuesBucket GetMapInsertValue(int32_t albumId, int32_t fileId,
        std::optional<int32_t> &order);
    void ParseMapInsertValues(std::vector<NativeRdb::ValuesBucket> &values,
        std::shared_ptr<NativeRdb::ResultSet> resultSet);
    bool IsExistPortraitDataInOldDb();

    void RecordOldGroupPhotoAlbumDfx();
    void LogGroupPhotoCloneDfx();
    void ReportRestoreTaskOfTotal();
    void ReportRestoreTaskOfAlbumStats();
    void ReportRestoreTaskOfAlbumInfo();
    void ReportCloneRestoreGroupPhotoTask();

private:
    struct CloneRestoreGroupPhotoInfo {
        std::optional<int32_t> id;
        std::optional<int32_t> fileIdOld;
        std::optional<int32_t> fileIdNew;
        std::optional<int32_t> categoryId;
        std::optional<std::string> subLabel;
        std::optional<double> prob;
        std::optional<std::vector<uint8_t>> feature;
        std::optional<std::string> simResult;
        std::optional<std::string> labelVersion;
        std::optional<std::string> saliencySubProb;
        std::optional<std::string> analysisVersion;
        std::optional<std::string> captionResult;
        std::optional<std::string> captionVersion;
    };

private:
    std::string taskId_;
    std::string analysisType_;
    std::vector<GroupPhotoAlbumDfx> groupPhotoAlbumDfx_;
    int32_t maxIdOfAlbum_{0};
    int32_t lastIdOfMap_{0};
    int64_t restoreTimeCost_{0};
    int64_t mapSuccessCnt_{0};
    int64_t mapFailedCnt_{0};
    int64_t albumFailedCnt_{0};
    int64_t albumSuccessCnt_{0};
    int64_t albumDeleteCnt_{0};
    bool isMapOrder_{false};
    std::mutex counterMutex_;
    std::vector<CoverUriInfo> coverUriInfo_;
    std::vector<AnalysisAlbumTbl> groupPhotoAlbumInfos_;
    int64_t migrateGroupPhotoTotalTimeCost_{0};
    std::unordered_map<std::string, int32_t> albumPhotoCounter_;
};
} // namespace OHOS::Media
#endif // CLONE_RESTORE_GROUP_PHOTO_H
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

#ifndef PORTRAIT_ALBUM_CLONE_H
#define PORTRAIT_ALBUM_CLONE_H

#include <string>

#include "backup_const.h"
#include "backup_file_utils.h"
#include "clone_restore_analysis_total.h"
#include "rdb_store.h"
#include "clone_restore_portrait_base.h"

namespace OHOS::Media {
constexpr int32_t BATCH_SIZE = 200;
class CloneRestorePortrait : public CloneRestorePortraitBase {
public:
    void Init(int32_t sceneCode, const std::string &taskId, std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
        std::shared_ptr<NativeRdb::RdbStore> mediaRdb,
        const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap, bool isCloudRestoreSatisfied);
    void Preprocess();
    void Restore();
    void RestoreFromGalleryPortraitAlbum();
    void RestorePortraitClusteringInfo();
    void RestoreImageFaceInfo();
    void UpdateAnalysisTotalTblNoFaceStatus();
    void UpdateAnalysisTotalTblStatus();
    int32_t RestoreMaps();

protected:
    std::atomic<uint64_t> migratePortraitAlbumNumber_{0};
    std::atomic<uint64_t> migratePortraitFaceNumber_{0};
    std::atomic<uint64_t> migratePortraitPhotoNumber_{0};
    std::atomic<uint64_t> migratePortraitFaceTagNumber_{0};
    std::atomic<uint64_t> migratePortraitAnalysisPhotoMapNumber_{0};
    std::atomic<uint64_t> migratePortraitTotalTimeCost_{0};

private:
    void RecordOldPortraitAlbumDfx();
    std::vector<PortraitAlbumDfx> QueryAllPortraitAlbum(int32_t& offset, int32_t& rowCount);
    void DeleteExistingPortraitInfos();
    void DeleteExistingPortraitAlbums();
    void DeleteExistingCluseringInfo();
    void DeleteExistingImageFaceInfos();
    void LogPortraitCloneDfx();
    vector<AnalysisAlbumTbl> QueryPortraitAlbumTbl(int32_t offset, const std::vector<std::string>& commonColumns);
    std::unordered_set<std::string> QueryAllPortraitAlbum();
    void InsertPortraitAlbum(std::vector<AnalysisAlbumTbl> &portraitAlbumTbl);
    int32_t InsertPortraitAlbumByTable(std::vector<AnalysisAlbumTbl> &portraitAlbumTbl);
    std::vector<NativeRdb::ValuesBucket> GetInsertValues(std::vector<AnalysisAlbumTbl> &portraitAlbumTbl);
    std::vector<FaceTagTbl> QueryFaceTagTbl(int32_t offset, const std::string &inClause);
    void ParseFaceTagResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet, FaceTagTbl& faceTagTbl);
    void BatchInsertFaceTags(const std::vector<FaceTagTbl>& faceTagTbls);
    NativeRdb::ValuesBucket CreateValuesBucketFromFaceTagTbl(const FaceTagTbl& faceTagTbl);
    template<typename T, typename U>
    void PutWithDefault(NativeRdb::ValuesBucket& values, const std::string& columnName,
        const std::optional<T>& optionalValue, const U& defaultValue);
    std::vector<ImageFaceTbl> ProcessImageFaceTbls(const std::vector<ImageFaceTbl>& imageFaceTbls,
        const std::vector<FileIdPair>& fileIdPairs);
    std::vector<ImageFaceTbl> QueryImageFaceTbl(int32_t offset, std::string &fileIdClause,
        const std::vector<std::string> &commonColumns);
    void ParseImageFaceResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
        ImageFaceTbl& imageFaceTbl);
    void ParseImageFaceResultSet1(const std::shared_ptr<NativeRdb::ResultSet>& resultSet, ImageFaceTbl& imageFaceTbl);
    NativeRdb::ValuesBucket CreateValuesBucketFromImageFaceTbl(const ImageFaceTbl& imageFaceTbl);
    void BatchInsertImageFaces(const std::vector<ImageFaceTbl>& imageFaceTbls);
    void RestoreMapsBatch();
    void UpdateMapInsertValues(std::vector<NativeRdb::ValuesBucket> &values);
    void UpdateMapInsertValuesByAlbumId(std::vector<NativeRdb::ValuesBucket> &values,
        std::shared_ptr<NativeRdb::ResultSet> resultSet);
    NativeRdb::ValuesBucket GetMapInsertValue(int32_t albumId, int32_t fileId, std::optional<int32_t> &order);
    void InsertAnalysisPhotoMap(std::vector<NativeRdb::ValuesBucket> &values);
    void ReportPortraitCloneStat(int32_t sceneCode);
    void RestoreAnalysisTotalFaceStatus();

public:
    std::vector<CoverUriInfo> coverUriInfo_;
    std::vector<AnalysisAlbumTbl> portraitAlbumInfoMap_;
    std::unordered_map<std::string, int32_t> albumPhotoCounter_;

private:
    std::string taskId_;
    std::string analysisAlbumExtraWhereClause_;
    std::vector<PortraitAlbumDfx> portraitAlbumDfx_;
    int32_t totalPortraitAlbumNumber_ = 0;
    int32_t lastIdOfMap_{0};
    bool isMapOrder_{false};
    int64_t mapSuccessCnt_{0};
    int64_t mapFailedCnt_{0};
    std::mutex counterMutex_;
};
}
#endif
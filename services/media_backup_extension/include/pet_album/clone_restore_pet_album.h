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

#ifndef PET_ALBUM_CLONE_H
#define PET_ALBUM_CLONE_H

#include <string>

#include "backup_const.h"
#include "backup_file_utils.h"
#include "clone_restore_analysis_total.h"
#include "rdb_store.h"
#include "clone_restore_pet_base.h"

namespace OHOS::Media {
const int32_t PET_BATCH_SIZE = 200;
class CloneRestorePet : public CloneRestorePetBase {
public:
    void Init(int32_t sceneCode, const std::string &taskId, std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
        std::shared_ptr<NativeRdb::RdbStore> mediaRdb,
        const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap, bool isCloudRestoreSatisfied);
    void Preprocess();
    void Restore();
    void RestoreFromGalleryPetAlbum();
    void RestorePetClusteringInfo();
    void RestorePetFaceInfo();
    int32_t RestoreMaps();

protected:
    std::atomic<uint64_t> migratePetAlbumNumber_{0};
    std::atomic<uint64_t> migratePetFaceNumber_{0};
    std::atomic<uint64_t> migratePetPhotoNumber_{0};
    std::atomic<uint64_t> migratePetFaceTagNumber_{0};
    std::atomic<uint64_t> migratePetAnalysisPhotoMapNumber_{0};
    std::atomic<uint64_t> migratePetTotalTimeCost_{0};

private:
    void RecordOldPetAlbumDfx();
    std::vector<PetAlbumDfx> QueryAllPetAlbum(int32_t& offset, int32_t& rowCount);
    std::unordered_set<std::string> QueryAllPetAlbum();
    void DeleteExistingPetInfos();
    void DeleteExistingPetAlbums();
    void DeleteExistingPetInfo();
    void LogPetCloneDfx();
    std::vector<AnalysisAlbumTbl> QueryPetAlbumTbl(int32_t offset, const std::vector<std::string>& commonColumns);
    void InsertPetAlbum(std::vector<AnalysisAlbumTbl> &PetAlbumTbl);
    int32_t InsertPetAlbumByTable(std::vector<AnalysisAlbumTbl> &PetAlbumTbl);
    std::vector<NativeRdb::ValuesBucket> GetInsertValues(std::vector<AnalysisAlbumTbl> &PetAlbumTbl);
    std::vector<PetTagTbl> QueryPetTagTbl(int32_t offset, const std::string &inClause);
    void ParsePetTagResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet, PetTagTbl& petTagTbl);
    void BatchInsertPetTags(const std::vector<PetTagTbl>& petTagTbls);
    NativeRdb::ValuesBucket CreateValuesBucketFromPetTagTbl(const PetTagTbl& petTagTbl);
    template<typename T, typename U>
    void PutWithDefault(NativeRdb::ValuesBucket& values, const std::string& columnName,
        const std::optional<T>& optionalValue, const U& defaultValue);
    std::vector<PetFaceTbl> ProcessPetFaceTbls(const std::vector<PetFaceTbl>& petFaceTbls,
        const std::vector<FileIdPair>& fileIdPairs);
    std::vector<PetFaceTbl> QueryPetFaceTbl(int32_t offset, std::string &fileIdClause,
        const std::vector<std::string> &commonColumns);
    void ParsePetFaceResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet, PetFaceTbl& petFaceTbl);
    void BatchInsertPetFaces(const std::vector<PetFaceTbl>& petFaceTbls);
    NativeRdb::ValuesBucket CreateValuesBucketFromPetFaceTbl(const PetFaceTbl& petFaceTbl);
    void RestoreMapsBatch();
    void UpdateMapInsertValues(std::vector<NativeRdb::ValuesBucket> &values);
    void UpdateMapInsertValuesByAlbumId(std::vector<NativeRdb::ValuesBucket> &values,
        std::shared_ptr<NativeRdb::ResultSet> resultSet);
    NativeRdb::ValuesBucket GetMapInsertValue(int32_t albumId, int32_t fileId, std::optional<int32_t> &order);
    void InsertAnalysisPhotoMap(std::vector<NativeRdb::ValuesBucket> &values);
    void ReportPetCloneStat(int32_t sceneCode);
    void RestoreAnalysisTotalFaceStatus();

public:
    std::vector<CoverUriInfo> coverUriInfo_;
    std::vector<AnalysisAlbumTbl> PetAlbumInfoMap_;
    std::unordered_map<std::string, int32_t> albumPhotoCounter_;

private:
    std::string taskId_;
    std::string analysisAlbumExtraWhereClause_;
    std::vector<PetAlbumDfx> PetAlbumDfx_;
    int32_t totalPetAlbumNumber_ = 0;
    int32_t lastIdOfMap_{0};
    bool isMapOrder_{false};
    int64_t mapSuccessCnt_{0};
    int64_t mapFailedCnt_{0};
    std::mutex counterMutex_;
};
}
#endif
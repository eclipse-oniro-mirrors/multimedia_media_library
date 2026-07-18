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

#ifndef CLONE_REVERSE_RESTORE_GEO_H
#define CLONE_REVERSE_RESTORE_GEO_H

#include "clone_restore_geo_base.h"
#include "backup_const_column.h"

namespace OHOS::Media {
class CloneReverseRestoreGeo : public CloneRestoreGeoBase {
public:
    void Init(int32_t sceneCode,
        const std::string &taskId,
        std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
        std::shared_ptr<NativeRdb::RdbStore> mediaRdb);

    void Restore();

private:
    void RestoreReverseGeoKnowledge();
    void RestoreReverseAlbums();
    void RestoreReverseMaps();
    void UpdateReverseGeoStatus();

    void QueryNewGeoKnowledge(std::vector<GeoCloneInfo> &infos);
    void ClassifyGeoKnowledge(std::vector<GeoCloneInfo> &infos,
        std::vector<GeoCloneInfo> &insertInfos,
        std::vector<GeoCloneInfo> &updateInfos);
    void InsertGeoKnowledgeToOld(std::vector<GeoCloneInfo> &insertInfos);
    void UpdateGeoKnowledgeToOld(std::vector<GeoCloneInfo> &updateInfos);

    void QueryNewCityAlbums(std::vector<AnalysisAlbumTbl> &albums);
    void InsertCityAlbumsToOld(std::vector<AnalysisAlbumTbl> &albums);
    void GenerateAlbumIdMap(const std::vector<AnalysisAlbumTbl> &albums);
    std::unordered_map<std::string, int32_t> QueryExistingCityAlbumNames();
    void DeleteDuplicateCityAlbum(int32_t oldAlbumId, int32_t newAlbumId);
    void UpdateTabOldAlbumsId(int32_t oldAlbumId, int32_t newAlbumId);

    void QueryNewCityMaps(std::vector<CityMapInfo> &maps);
    void InsertCityMapsToOld(std::vector<CityMapInfo> &maps);
    void UpdateCityMapsToOld(std::vector<CityMapInfo> &maps);

    std::unordered_set<int32_t> GetExistingFileIdsOld();
    std::unordered_set<int32_t> GetExistingMapAssetIdsOld();
    std::unordered_set<std::string> GetExistingCityIdsOld();
    int32_t GetMaxAlbumIdOld();

    void ReportReverseRestoreTask();

private:
    std::unordered_map<int32_t, int32_t> albumIdMap_;
    std::vector<AnalysisAlbumTbl> newAlbumInfos_;
    std::unordered_set<int32_t> existingFileIdsOld_;
    std::vector<int32_t> successGeoFileIds_;
    int32_t maxAlbumIdOld_{0};
    int64_t restoreTimeCost_{0};
    std::atomic<int32_t> successGeoKnowledgeCnt_{0};
    std::atomic<int32_t> successGeoDictionaryCnt_{0};
    std::atomic<int32_t> successAlbumCnt_{0};
    std::atomic<int32_t> successMapCnt_{0};
    std::atomic<int32_t> failedCnt_{0};
};

} // namespace OHOS::Media

#endif // CLONE_REVERSE_RESTORE_GEO_H
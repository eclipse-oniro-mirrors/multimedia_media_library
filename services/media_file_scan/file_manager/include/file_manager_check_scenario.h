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
#ifndef OHOS_MEDIA_FILE_MANAGER_CHECK_SCENARIO_H
#define OHOS_MEDIA_FILE_MANAGER_CHECK_SCENARIO_H

#include <set>

#include "asset_accurate_refresh.h"
#include "i_check_scenario.h"

namespace OHOS::Media {
class FileManagerCheckScenario final : public ICheckScenario {
public:
    bool IsConditionSatisfied(const ConsistencyCheck::DeviceStatus &deviceStatus) override;
    void Execute(std::atomic<bool> &isInterrupted) override;

private:
    struct PhotoCandidates {
        std::vector<ConsistencyCheck::PhotoRecord> photosToDelete;
        std::vector<ConsistencyCheck::PhotoRecord> photosToUpdatePosition;
        std::vector<ConsistencyCheck::PhotoRecord> photosToScan;
        std::set<std::string> affectedAnalysisAlbumIds;
    };

    struct AlbumCandidates {
        std::vector<ConsistencyCheck::AlbumRecord> albumsToDelete;
    };

    bool IsTemperatureSatisfied(int32_t temperature);
    bool IsCheckPeriodSatisfied(int64_t requiredPeriodInMs);

    int32_t RunForward(ScenarioContext &context);
    int32_t RunBackwardPhoto(ScenarioContext &context);
    int32_t RunBackwardAlbum(ScenarioContext &context);

    void LoadStatus(ConsistencyCheck::ScenarioProgress &progress, ConsistencyCheck::DfxStats &dfxStats);
    void SaveCurrentStatus(const ConsistencyCheck::ScenarioProgress &progress,
        const ConsistencyCheck::DfxStats &dfxStats);
    void SaveFinishedStatus(int64_t endTimeInMs);

    std::vector<ConsistencyCheck::PhotoRecord> GetPhotoRecords(ScenarioContext &context);
    PhotoCandidates SelectPhotoCandidates(ScenarioContext &context,
        const std::vector<ConsistencyCheck::PhotoRecord> &photoRecords);
    void HandleExistingPhoto(const ConsistencyCheck::PhotoRecord &photoRecord,
        PhotoCandidates &candidates);
    void HandleNonExistingPhoto(const ConsistencyCheck::PhotoRecord &photoRecord,
        PhotoCandidates &candidates);
    void ProcessPhotoCandidates(ScenarioContext &context, PhotoCandidates &candidates);
    void QueryAffectedAnalysisAlbumIds(PhotoCandidates &candidates);
    void DeletePhotos(ScenarioContext &context, PhotoCandidates &candidates);
    void DeletePhotoRecords(ScenarioContext &context, const std::vector<ConsistencyCheck::PhotoRecord> &photos);
    void DeletePhotoFiles(const std::vector<ConsistencyCheck::PhotoRecord> &photos);
    void UpdatePhotosPosition(ScenarioContext &context, const std::vector<ConsistencyCheck::PhotoRecord> &photos);
    void ScanPhotos(ScenarioContext &context, const std::vector<ConsistencyCheck::PhotoRecord> &photos);
    void ApplyPhotoChanges(const PhotoCandidates &candidates);
    void UpdateAnalysisAlbumsAndNotify(const PhotoCandidates &candidates);

    std::vector<ConsistencyCheck::AlbumRecord> GetAlbumRecords(ScenarioContext &context);
    AlbumCandidates SelectAlbumCandidates(ScenarioContext &context,
        const std::vector<ConsistencyCheck::AlbumRecord> &albumRecords);
    void ProcessAlbumCandidates(ScenarioContext &context, const AlbumCandidates &candidates);
    void DeleteAlbums(ScenarioContext &context, const std::vector<ConsistencyCheck::AlbumRecord> &albums);
    int32_t DeleteInPhotoAlbum(const std::vector<ConsistencyCheck::AlbumRecord> &albums);
    void ClearAlbumScanInfo(const std::vector<ConsistencyCheck::AlbumRecord> &albums);
    void ApplyAlbumChanges();

    std::string ConvertLpathToRealPath(const std::string &lpath);

    AccurateRefresh::AlbumAccurateRefresh albumRefresh_;
    AccurateRefresh::AssetAccurateRefresh assetRefresh_;

    static constexpr int32_t BATCH_SIZE = 100;
};
} // namespace OHOS::Media

#endif // OHOS_MEDIA_FILE_MANAGER_CHECK_SCENARIO_H

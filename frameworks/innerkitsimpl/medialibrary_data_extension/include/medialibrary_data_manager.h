/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIALIBRARY_DATA_MANAGER_H
#define OHOS_MEDIALIBRARY_DATA_MANAGER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <shared_mutex>

#include "fa_ability_context.h"
#include "cloud_sync_observer.h"
#include "context/context.h"
#include "dir_asset.h"
#include "datashare_predicates.h"
#include "datashare_values_bucket.h"
#include "distributed_kv_data_manager.h"
#include "imedia_scanner_callback.h"
#include "medialibrary_command.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_helper_container.h"
#include "medialibrary_db_const.h"
#include "medialibrary_rdbstore.h"
#include "rdb_predicates.h"
#include "rdb_store.h"
#include "result_set_bridge.h"
#include "uri.h"
#include "values_bucket.h"
#include "thumbnail_service.h"
#include "bundle_mgr_interface.h"

#define EXPORT __attribute__ ((visibility ("default")))

namespace OHOS {
namespace AbilityRuntime {
class MediaDataShareExtAbility;
}
namespace Media {
using OHOS::AbilityRuntime::MediaDataShareExtAbility;
class MediaLibraryDataManager {
public:
    static constexpr uint32_t URI_MIN_NUM = 3;
    EXPORT MediaLibraryDataManager();
    EXPORT ~MediaLibraryDataManager();
    EXPORT static MediaLibraryDataManager* GetInstance();

    EXPORT int32_t Insert(MediaLibraryCommand &cmd, const DataShare::DataShareValuesBucket &value);
    EXPORT int32_t InsertExt(MediaLibraryCommand &cmd, const DataShare::DataShareValuesBucket &value,
        std::string &result);
    EXPORT int32_t Delete(MediaLibraryCommand &cmd, const DataShare::DataSharePredicates &predicates);
    EXPORT int32_t BatchInsert(MediaLibraryCommand &cmd,
        const std::vector<DataShare::DataShareValuesBucket> &values);
    EXPORT int32_t Update(MediaLibraryCommand &cmd, const DataShare::DataShareValuesBucket &value,
        const DataShare::DataSharePredicates &predicates);
    EXPORT std::shared_ptr<DataShare::ResultSetBridge> Query(MediaLibraryCommand &cmd,
        const std::vector<std::string> &columns, const DataShare::DataSharePredicates &predicates, int &errCode);
    EXPORT std::shared_ptr<NativeRdb::ResultSet>
    EXPORT QueryRdb(MediaLibraryCommand &cmd, const std::vector<std::string> &columns,
        const DataShare::DataSharePredicates &predicates, int &errCode);
    EXPORT int32_t OpenFile(MediaLibraryCommand &cmd, const std::string &mode);
    EXPORT std::string GetType(const Uri &uri);
    EXPORT void NotifyChange(const Uri &uri);
    EXPORT int32_t GenerateThumbnailBackground();
    EXPORT int32_t RepairExifRotateBackground();
    EXPORT int32_t GenerateHighlightThumbnailBackground();
    EXPORT static std::shared_ptr<NativeRdb::ResultSet> QueryAnalysisAlbum(MediaLibraryCommand &cmd,
        const std::vector<std::string> &columns, const DataShare::DataSharePredicates &predicates);
    EXPORT static std::shared_ptr<NativeRdb::ResultSet> QueryGeo(const NativeRdb::RdbPredicates &rdbPredicates,
        const std::vector<std::string> &columns);
    // upgrade existed thumbnails to fix such as size, rotation and quality etc. problems
    EXPORT int32_t UpgradeThumbnailBackground(bool isWifiConnected);

    // restore thumbnail for date fronted 2000 photos from dual framework upgrade or clone
    EXPORT int32_t RestoreThumbnailDualFrame();
    void InterruptBgworker();
    void InterruptThumbnailBgWorker();
    EXPORT int32_t DoAging();
    EXPORT int32_t DoTrashAging(std::shared_ptr<int> countPtr = nullptr);
    /**
     * @brief Revert the pending state through the package name
     * @param bundleName packageName
     * @return revert result
     */
    EXPORT int32_t RevertPendingByPackage(const std::string &bundleName);

    // update burst photo from gallery
    EXPORT int32_t UpdateBurstFromGallery();
    // update burst_cover_level from gallery
    EXPORT int32_t UpdateBurstCoverLevelFromGallery();
    // update hdr mode
    EXPORT int32_t UpdatePhotoHdrMode();

    EXPORT std::shared_ptr<MediaLibraryRdbStore> rdbStore_;

    EXPORT int32_t InitMediaLibraryMgr(const std::shared_ptr<OHOS::AbilityRuntime::Context> &context,
        const std::shared_ptr<OHOS::AbilityRuntime::Context> &extensionContext,
        int32_t &sceneCode, bool isNeedCreateDir = true, bool isInMediaLibraryOnStart = false);
    EXPORT void ClearMediaLibraryMgr();
    EXPORT int32_t MakeDirQuerySetMap(std::unordered_map<std::string, DirAsset> &outDirQuerySetMap);
    EXPORT void CreateThumbnailAsync(const std::string &uri, const std::string &path,
        std::shared_ptr<Media::Picture> originalPhotoPicture = nullptr);
    EXPORT static std::unordered_map<std::string, DirAsset> GetDirQuerySetMap();
    EXPORT std::shared_ptr<MediaDataShareExtAbility> GetOwner();
    EXPORT void SetOwner(const std::shared_ptr<MediaDataShareExtAbility> &datashareExtension);
    EXPORT int GetThumbnail(const std::string &uri);
    EXPORT void SetStartupParameter();
    EXPORT void ReCreateMediaDir();
    EXPORT int32_t CheckCloudThumbnailDownloadFinish();
    EXPORT void UploadDBFileInner(int64_t totalFileSize);
    EXPORT int32_t UpdateDirtyForCloudClone(int32_t version);
    EXPORT int32_t ClearDirtyHdcData();
    EXPORT int32_t UpdateMediaSizeFromStorage();
    EXPORT int HandleAnalysisFaceUpdate(MediaLibraryCommand& cmd, NativeRdb::ValuesBucket &value,
                const DataShare::DataSharePredicates &predicates);
    EXPORT int32_t RestoreInvalidPosData();
    EXPORT int32_t RestoreInvalidHDCCloudDataPos();
    EXPORT int32_t AcquireDebugDatabase(const std::string &betaIssueId, const std::string &betaScenario,
        std::string &fileName, std::string &fileSzie);
    EXPORT int32_t ReleaseDebugDatabase(const std::string &betaIssueId);
    EXPORT void ScanLakeAsset();
    EXPORT std::shared_ptr<NativeRdb::ResultSet> ProcessBrokerChangeMsg(const std::string &operation);
    EXPORT static int32_t OpenAssetCompress(const std::string &uri, const int32_t type, const int32_t version,
        int32_t &fd);
    EXPORT static int32_t NotifyAssetSended(const std::string &uri);
    EXPORT static int32_t GetAssetCompressVersion();

private:
    int32_t InitMediaLibraryRdbStore();
    int32_t UpdateDirtyHdcDataStatus();
    int32_t UpdateDirtyForCloudClone();
    int32_t UpdateDirtyForCloudCloneV2();
    void InitResourceInfo();
    void DeleteDirtyFileAndDir(const std::vector<std::string>& deleteFilePaths);
    void HandleUpgradeRdbAsync(bool isInMediaLibraryOnStart);
    int32_t BatchInsertMediaAnalysisData(MediaLibraryCommand &cmd,
        const std::vector<DataShare::DataShareValuesBucket> &values);
    int32_t HandleThumbnailOperations(MediaLibraryCommand &cmd);

    EXPORT int32_t SolveInsertCmd(MediaLibraryCommand &cmd);
    EXPORT int32_t SetCmdBundleAndDevice(MediaLibraryCommand &outCmd);
    EXPORT int32_t InitialiseThumbnailService(const std::shared_ptr<OHOS::AbilityRuntime::Context> &extensionContext);
    std::shared_ptr<NativeRdb::ResultSet> QuerySet(MediaLibraryCommand &cmd, const std::vector<std::string> &columns,
        const DataShare::DataSharePredicates &predicates, int &errCode);
    void InitACLPermission();
    void InitDatabaseACLPermission();
    std::shared_ptr<NativeRdb::ResultSet> QueryInternal(MediaLibraryCommand &cmd,
        const std::vector<std::string> &columns, const DataShare::DataSharePredicates &predicates);
    EXPORT std::shared_ptr<ThumbnailService> thumbnailService_;
    int32_t RevertPendingByFileId(const std::string &fileId);
    int32_t DeleteInRdbPredicates(MediaLibraryCommand &cmd, NativeRdb::RdbPredicates &rdbPredicate);
    int32_t DeleteInRdbPredicatesMore(MediaLibraryCommand &cmd, NativeRdb::RdbPredicates &rdbPredicate);
    int32_t DeleteInRdbPredicatesAnalysis(MediaLibraryCommand &cmd, NativeRdb::RdbPredicates &rdbPredicate);
    int32_t UpdateInternal(MediaLibraryCommand &cmd, NativeRdb::ValuesBucket &value,
        const DataShare::DataSharePredicates &predicates);
    int32_t SolveInsertCmdSub(MediaLibraryCommand &cmd);
    void HandleOtherInitOperations();
    void InitRefreshAlbum();
    int32_t ProcessThumbnailBatchCmd(const MediaLibraryCommand &cmd,
        const NativeRdb::ValuesBucket &value, const DataShare::DataSharePredicates &predicates);
    void SubscriberPowerConsumptionDetection();
    int32_t AstcMthAndYearInsert(MediaLibraryCommand &cmd,
        const std::vector<DataShare::DataShareValuesBucket> &values);
    bool IsLakeAssetScanned();
    void CheckCleanLakeAsset();
    bool IsDirectoryEmpty();
    bool ReadSupportHmos();
    void SetIsLakeAssetScanned(bool isScanned);
    bool IsSupportHmos();

    std::shared_mutex mgrSharedMutex_;
    std::shared_ptr<OHOS::AbilityRuntime::Context> context_;
    std::string bundleName_ {BUNDLE_NAME};
    static std::mutex mutex_;
    static std::unique_ptr<MediaLibraryDataManager> instance_;
    static std::unordered_map<std::string, DirAsset> dirQuerySetMap_;
    std::atomic<int> refCnt_ {0};
    std::shared_ptr<MediaDataShareExtAbility> extension_;
    std::shared_ptr<CloudSyncObserver> cloudPhotoObserver_;
    std::shared_ptr<CloudSyncObserver> cloudPhotoAlbumObserver_;
    std::shared_ptr<CloudSyncObserver> galleryRebuildObserver_;
    std::shared_ptr<CloudSyncObserver> cloudGalleryPhotoObserver_;
    std::shared_ptr<CloudSyncObserver> cloudGalleryDownloadObserver_;
    std::atomic_bool isAgingDup_ {false};
    std::string supportHmos_ {""};
    std::string scanStatus_ {""};
};

// Scanner callback objects
class ScanFileCallback : public IMediaScannerCallback {
public:
    ScanFileCallback() = default;
    ~ScanFileCallback() = default;
    int32_t OnScanFinished(const int32_t status, const std::string &uri, const std::string &path) override;
    void SetOriginalPhotoPicture(std::shared_ptr<Media::Picture> resultPicture)
    {
        originalPhotoPicture = resultPicture;
    }
    void SetCallback(std::shared_ptr<IMediaScannerCallback> &callback);

private:
    std::shared_ptr<Media::Picture> originalPhotoPicture = nullptr;
    std::shared_ptr<IMediaScannerCallback> callback_;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_DATA_ABILITY_H

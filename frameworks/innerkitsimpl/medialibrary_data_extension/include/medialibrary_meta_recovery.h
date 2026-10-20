/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#ifndef OHOS_MEDIALIBRARY_METARECOVERY_H
#define OHOS_MEDIALIBRARY_METARECOVERY_H

#include <atomic>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <sys/stat.h>

#include "file_asset.h"
#include "post_event_utils.h"
#include "media_column.h"
#include "replica_recovery_policy.h"
#include "photo_album.h"
#include "values_bucket.h"
#include "rdb_store.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

enum class MediaLibraryMetaRecoveryState : int32_t {
    STATE_NONE = 0,
    STATE_RECOVERING,
    STATE_RECOVERING_ABORT,
    STATE_BACKING_UP,
    STATE_REPAIRING
};

class MediaLibraryMetaRecovery {
public:
    EXPORT static MediaLibraryMetaRecovery &GetInstance();
    EXPORT static int32_t DeleteMetaDataByPath(const std::string &filePath);

    EXPORT void CheckRecoveryState();
    EXPORT void InterruptRecovery();
    // Only the background thread owner resets cancellation after joining its previous worker.
    EXPORT void ResetRecoveryInterrupt();
    EXPORT int32_t WriteSingleMetaDataById(int32_t rowId);
    EXPORT int32_t StartAsyncRecovery();
    EXPORT int32_t SetRdbRebuiltStatus(bool status);
    EXPORT int32_t ResetAllMetaDirty();
    EXPORT void RecoveryStatistic();
    EXPORT void StatisticSave();
    EXPORT void PrepareIncrementalRecovery(const std::shared_ptr<NativeRdb::RdbStore> &store,
        const std::string &date);
    // One pass per eligible background window; unfinished work does not hold later maintenance.
    EXPORT void RecoverPendingRecords();

private:
    MediaLibraryMetaRecovery() = default;
    virtual ~MediaLibraryMetaRecovery() = default;
    EXPORT void StatisticRestore();
    EXPORT void StatisticReset();
    EXPORT bool IsIncrementalRecoveryAllowed() const;
    // Backup
    EXPORT void DoBackupMetadata();
    void AlbumBackup();
    int32_t QueryPhotoBackupBatch(int32_t lastId, int32_t upperId,
        std::vector<std::shared_ptr<FileAsset>> &photos);
    void PhotoBackupBatch();
    void PhotoBackup(const std::vector<std::shared_ptr<FileAsset>>&, int32_t&, int32_t&);

    // Recovery
    void DoDataBaseRecovery();
    EXPORT int32_t AlbumRecovery(const std::string &path);
    EXPORT int32_t PhotoRecovery(const std::string &path);
    EXPORT int32_t ScanMetaDir(const std::string &path, int32_t bucket_id);

    // Json
    bool WriteJsonFile(const std::string &filePath, const nlohmann::json &j);
    bool ReadJsonFile(const std::string &filePath, nlohmann::json &j);
    EXPORT int32_t WriteMetadataToFile(const std::string &filePath, const FileAsset &fileAsset);
    EXPORT int32_t ReadMetadataFromFile(const std::string &filePath, FileAsset &fileAsset);
    void AddMetadataToJson(nlohmann::json &j, const FileAsset &fileAsset);
    bool GetMetadataFromJson(const nlohmann::json &j, FileAsset &fileAsset);
    static bool DecodeMetadataMember(const std::string &name, const nlohmann::json &value,
        ResultSetDataType type, FileAsset &asset);
    int32_t WriteSingleMetaData(const FileAsset &asset);
    EXPORT int32_t WritePhotoAlbumToFile(const std::string &filePath,
                                  const std::vector<std::shared_ptr<PhotoAlbum>> &vecPhotoAlbum);
    EXPORT int32_t ReadPhotoAlbumFromFile(const std::string &filePath,
                                   std::vector<std::shared_ptr<PhotoAlbum>> &photoAlbumVector);
    void AddPhotoAlbumToJson(nlohmann::json &j, const PhotoAlbum &photoAlbum);
    bool GetPhotoAlbumFromJson(const nlohmann::json &j, PhotoAlbum &photoAlbum);
    void LoadAlbumMaps(const std::string &path);
    // Incremental recovery shares this class; see medialibrary_incremental_recovery.cpp.
    static int32_t CheckDatabaseUsability(NativeRdb::RdbStore &store,
        int64_t expectedVersion, bool checkIntegrity = true);
    int32_t ProcessPendingIncrementalRecovery(const std::shared_ptr<NativePreferences::Preferences> &prefs);
    int32_t RecordRecoveryScanResult(int32_t error, int64_t begin);
    int32_t RecoverMissingRecordsAfterRestore();
    int32_t RepairPhotoRecordsInDirectory(const std::string &path);
    int32_t CheckMetadataBackup(const FileAsset *asset = nullptr);
    static int32_t QueryPhotoRecoveryState(const std::string &path, int32_t &fileId, bool &hasAlbum);
    int32_t ScanAssetForRecoverySync(const std::string &path, int32_t fileId);
    static bool IsMetadataMarkedDeleted(const nlohmann::json &metadata);
    int32_t RepairPhotoRecord(const std::string &path);
    int32_t ReadIncrementalMetadata(const std::string &path, FileAsset &asset);
    bool DecodeIncrementalMetadata(const nlohmann::json &metadata, FileAsset &asset);
    static bool IsOrdinaryLocalMetadata(const FileAsset &asset);
    int32_t ValidateRecoveryMedia(FileAsset &asset);
    NativeRdb::ValuesBucket BuildRecoveryPhotoValues(FileAsset &asset);
    int32_t RepairRecoveryEntry(const std::string &path, const struct stat &info);
    EXPORT static bool IsMetadataMatchingAsset(const std::string &path, const FileAsset &asset);
    int32_t PrepareAlbumMappingsForRecovery();
    int32_t EnsureAlbumAndMapId(const std::shared_ptr<PhotoAlbum> &album);
    int32_t WriteRecoveredPhotoRecord(FileAsset &asset, int32_t fileId);
    EXPORT int32_t ReadMetaStatusFromFile(std::set<int32_t> &status);
    EXPORT int32_t WriteMetaStatusToFile(const std::string &keyPath, const int32_t status);
    EXPORT int32_t ReadMetaRecoveryCountFromFile();

    // DB
    bool UpdatePhotoOwnerAlbumId(NativeRdb::ValuesBucket&);
    static void SetValuesFromFileAsset(const FileAsset &fileAsset, NativeRdb::ValuesBucket &values,
        const std::unordered_map<std::string, ResultSetDataType> &columnInfoMap);
    static int32_t RefreshAlbumCount();
    int32_t InsertMetadataInDbRetry(const FileAsset &fileAsset);
    int32_t InsertMetadataInDb(const FileAsset &fileAsset);
    int32_t InsertMetadataInDb(const std::vector<std::shared_ptr<PhotoAlbum>> &vecPhotoAlbum);
    int32_t UpdateMetadataFlagInDb(const int32_t fieldId, const MetadataFlags &flag);

    // cloudsync
    EXPORT void StopCloudSync();
    EXPORT void RestartCloudSync();

    // RecoveryPhotosTableColumn
    EXPORT ResultSetDataType GetDataType(const std::string &name);
    EXPORT std::unordered_map<std::string, ResultSetDataType> QueryRecoveryPhotosTableColumnInfo();

private:
    static const std::string RDB_CONFIG;
    static const std::string META_RECOVERY_ALBUM_PATH;
    std::atomic<MediaLibraryMetaRecoveryState> recoveryState_{MediaLibraryMetaRecoveryState::STATE_NONE};
    bool rdbRebuilt_{false};
    std::mutex metadataMutex_;
    std::atomic<bool> cancelCurrentWork_ {false};
    DatabaseRecoveryRequest recoveryRequest_;
    std::shared_ptr<NativeRdb::RdbStore> incrementalStore_;
    DatabaseRecoveryReport recoveryReport_;
    std::set<int32_t> metaStatus;
    std::map<int32_t, std::string> oldAlbumIdToLpath;
    std::set<int32_t> deferredAlbumIds_;
    std::map<std::string, int32_t> lpathToNewAlbumId;

    int64_t backupSuccCnt_{0};   // succ in 12 hours
    int64_t backupCostTime_{0};  // costtime in 12 hours

    int64_t recoverySuccCnt_{0};  // file recovered successfully after the recovery process
    int64_t recoveryTotalBackupCnt_{0};   // total metafile in phone at the beginning of recovery

    int64_t reBuiltCount_{0};      // rebuilt count in 12 hours
    int64_t recoveryCostTime_{0};  // costtime in 12 hours
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_METARECOVERY_H

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

#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <set>

#include "file_asset.h"
#include "media_column.h"
#include "photo_album.h"
#include "values_bucket.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

enum class MediaLibraryMetaRecoveryState : int32_t {
    STATE_NONE = 0,
    STATE_RECOVERING,
    STATE_RECOVERING_ABORT,
    STATE_BACKING_UP
};

class MediaLibraryMetaRecovery {
public:
    EXPORT static MediaLibraryMetaRecovery &GetInstance();
    EXPORT static int32_t DeleteMetaDataByPath(const std::string &filePath);

    EXPORT void CheckRecoveryState();
    EXPORT void InterruptRecovery();
    EXPORT int32_t WriteSingleMetaDataById(int32_t rowId);
    EXPORT int32_t StartAsyncRecovery();
    EXPORT int32_t SetRdbRebuiltStatus(bool status);
    EXPORT int32_t ResetAllMetaDirty();
    EXPORT void RecoveryStatistic();
    EXPORT void StatisticSave();

private:
    MediaLibraryMetaRecovery() = default;
    virtual ~MediaLibraryMetaRecovery() = default;
    EXPORT void StatisticRestore();
    EXPORT void StatisticReset();
    // Backup
    EXPORT void DoBackupMetadata();
    void AlbumBackup();
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
    int32_t WriteSingleMetaData(const FileAsset &asset);
    EXPORT int32_t WritePhotoAlbumToFile(const std::string &filePath,
                                  const std::vector<std::shared_ptr<PhotoAlbum>> &vecPhotoAlbum);
    EXPORT int32_t ReadPhotoAlbumFromFile(const std::string &filePath,
                                   std::vector<std::shared_ptr<PhotoAlbum>> &photoAlbumVector);
    void AddPhotoAlbumToJson(nlohmann::json &j, const PhotoAlbum &photoAlbum);
    bool GetPhotoAlbumFromJson(const nlohmann::json &j, PhotoAlbum &photoAlbum);
    void LoadAlbumMaps(const std::string &path);
    EXPORT int32_t ReadMetaStatusFromFile(std::set<int32_t> &status);
    EXPORT int32_t WriteMetaStatusToFile(const std::string &keyPath, const int32_t status);
    EXPORT int32_t ReadMetaRecoveryCountFromFile();

    // DB
    bool UpdatePhotoOwnerAlbumId(NativeRdb::ValuesBucket&);
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
    std::atomic<MediaLibraryMetaRecoveryState> recoveryState_{MediaLibraryMetaRecoveryState::STATE_NONE};
    bool rdbRebuilt_{false};
    std::set<int32_t> metaStatus;
    std::map<int32_t, std::string> oldAlbumIdToLpath;
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
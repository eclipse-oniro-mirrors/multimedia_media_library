/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_CLONE_RESTORE_H
#define OHOS_MEDIA_CLONE_RESTORE_H

#include <optional>
#include <type_traits>
#include <set>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <vector>

#include "base_restore.h"
#include "backup_const.h"
#include "clone_restore_analysis_data.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_kvstore_manager.h"
#include "backup_database_utils.h"
#include "photo_album_clone.h"
#include "photos_clone.h"
#include "clone_restore_highlight.h"
#include "clone_restore_geo_dictionary.h"
#include "search_index_clone.h"
#include "video_face_clone.h"
#include "beauty_score_clone.h"
#include "asset_map_clone.h"
#include "tab_old_albums_clone.h"
#include "ffrt.h"
#include "ffrt_inner.h"
#include "settings_data_manager.h"

namespace OHOS {
namespace Media {
struct CloudPhotoFileExistFlag {
    bool isLcdExist {false};
    bool isThmExist {false};
    bool isDayAstcExist {false};
    bool isYearAstcExist {false};
    bool isExLcdExist {false};
    bool isExThmExist {false};
};

struct CloneRestoreConfigInfo {
    SwitchStatus switchStatus = SwitchStatus::NONE;
    std::string deviceId;
    bool isValid = false;

    bool operator==(const CloneRestoreConfigInfo& other)
    {
        return switchStatus == other.switchStatus &&
            deviceId == other.deviceId &&
            isValid == other.isValid;
    }

    std::string ToString()
    {
        return "[switchStatus: " + std::to_string(static_cast<int>(switchStatus)) + ", "
        "deviceId: " + deviceId + ", isValid:" + std::to_string(isValid) +"]";
    }
};

struct DstDeviceBackupInfo {
    bool hdcEnabled = false;
};

class CloneRestore : public BaseRestore {
public:
    CloneRestore();
    virtual ~CloneRestore() = default;
    // upgradePath is useless now
    void StartRestore(const std::string &backupRestorePath, const std::string &upgradePath) override;
    int32_t Init(const std::string &backupRestoreDir, const std::string &upgradeFilePath, bool isUpgrade) override;
    NativeRdb::ValuesBucket GetInsertValue(const FileInfo &fileInfo, const std::string &newPath,
        int32_t sourceType) override;
    NativeRdb::ValuesBucket GetCloudInsertValue(const FileInfo &fileInfo, const std::string &newPath,
        int32_t sourceType);
    std::string GetBackupInfo() override;
    void StartBackup() override;
    using CoverUriInfo = std::pair<std::string, std::pair<std::string, int32_t>>;
    void InheritManualCover();
    void UpdatePhotoAlbumCoverUri(vector<AlbumCoverInfo>& albumCoverInfos);
protected:
    void MoveMigrateCloudFile(std::vector<FileInfo> &fileInfos, int32_t &fileMoveCount, int32_t &videoFileMoveCount,
        int32_t sceneCode) override;
    void GetCloudPhotoFileExistFlag(const FileInfo &fileInfo, CloudPhotoFileExistFlag &resultExistFlag);
    void CloudPhotoFilesVerify(const std::vector<FileInfo> &fileInfos, std::vector<FileInfo> &LCDNotFound,
        std::vector<FileInfo> &THMNotFound, unordered_map<string, CloudPhotoFileExistFlag> &resultExistMap);
    bool HasExThumbnail(const FileInfo &info) override;
    void BackupRelease() override;
    bool IsCloudRestoreSatisfied() override;

private:
    void RestorePhoto(void) override;
    void RestorePhotoForCloud(void);
    void HandleRestData(void) override;
    std::vector<FileInfo> QueryFileInfos(int32_t offset, int32_t isRelatedToPhotoMap = 0);
    std::vector<FileInfo> QueryCloudFileInfos(int32_t offset, int32_t isRelatedToPhotoMap = 0);
    bool ParseResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &info,
        std::string dbName = "") override;
    bool ParseResultSetForAudio(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &info) override;
    void AnalyzeSource() override;
    void RestoreAlbum(void);
    void RestoreTabOldAlbumsData(void);
    void RestoreAudio(void) override;
    int InsertPhoto(std::vector<FileInfo> &fileInfos);
    std::vector<NativeRdb::ValuesBucket> GetInsertValues(int32_t sceneCode, std::vector<FileInfo> &fileInfos,
        int32_t sourceType);
    std::vector<NativeRdb::ValuesBucket> GetCloudInsertValues(int32_t sceneCode, std::vector<FileInfo> &fileInfos,
        int32_t sourceType) override;
    int InsertCloudPhoto(int32_t sceneCode, std::vector<FileInfo> &fileInfos, int32_t sourceType) override;
    int32_t MoveAsset(FileInfo &fileInfo);
    bool IsFilePathExist(const std::string &filePath) const;
    int32_t QueryTotalNumber(const std::string &tableName);
    std::vector<AlbumInfo> QueryAlbumInfos(const std::string &tableName, int32_t offset);
    bool ParseAlbumResultSet(const std::string &tableName, const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        AlbumInfo &albumInfo);
    bool PrepareCommonColumnInfoMap(const std::string &tableName,
        const std::unordered_map<std::string, std::string> &srcColumnInfoMap,
        const std::unordered_map<std::string, std::string> &dstColumnInfoMap);
    bool HasSameColumn(const std::unordered_map<std::string, std::string> &columnInfoMap, const std::string &columnName,
        const std::string &columnType);
    void GetValFromResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        std::unordered_map<std::string, std::variant<int32_t, int64_t, double, std::string>> &valMap,
        const std::string &columnName, const std::string &columnType);
    void PrepareCommonColumnVal(NativeRdb::ValuesBucket &values, const std::string &columnName,
        const std::variant<int32_t, int64_t, double, std::string> &columnVal,
        const std::unordered_map<std::string, std::string> &commonColumnInfoMap) const;
    void GetQueryWhereClause(const std::string &tableName,
        const std::unordered_map<std::string, std::string> &columnInfoMap);
    void BatchQueryPhoto(std::vector<FileInfo> &fileInfos);
    void UpdateAlbumOrderColumns(const AlbumInfo &albumInfo, const string &tableName);
    void UpdateSystemAlbumColumns(const string &tableName);
    void PopulateSystemAlbumIdMap();
    void InsertAlbum(std::vector<AlbumInfo> &albumInfos, const std::string &tableName);
    std::vector<NativeRdb::ValuesBucket> GetInsertValues(std::vector<AlbumInfo> &albumInfos,
        std::vector<std::string> &albumIds, const std::string &tableName);
    bool HasSameAlbum(AlbumInfo &albumInfo, const std::string &tableName);
    void BatchQueryAlbum(std::vector<AlbumInfo> &albumInfos, const std::string &tableName);
    void BatchInsertMap(const std::vector<FileInfo> &fileInfos, int64_t &totalRowNum);
    NativeRdb::ValuesBucket GetInsertValue(const MapInfo &mapInfo) const;
    NativeRdb::ValuesBucket GetInsertValue(const AlbumInfo &albumInfo, const std::string &tableName) const;
    void CheckTableColumnStatus(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
        const std::vector<std::vector<std::string>> &cloneTableList);
    bool HasColumns(const std::unordered_map<std::string, std::string> &columnInfoMap,
        const std::unordered_set<std::string> &columnSet);
    bool HasColumn(const std::unordered_map<std::string, std::string> &columnInfoMap, const std::string &columnName);
    void GetAlbumExtraQueryWhereClause(const std::string &tableName);
    bool IsReadyForRestore(const std::string &tableName);
    void PrepareEditTimeVal(NativeRdb::ValuesBucket &values, int64_t editTime, const FileInfo &fileInfo,
        const std::unordered_map<std::string, std::string> &commonColumnInfoMap) const;
    void RestoreGallery();
    bool PrepareCloudPath(const std::string &tableName, FileInfo &fileInfo);
    void RestoreMusic();
    std::vector<FileInfo> QueryFileInfos(const std::string &tableName, int32_t offset);
    bool ParseResultSet(const std::string &tableName, const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        FileInfo &fileInfo);
    void InsertAudio(std::vector<FileInfo> &fileInfos);
    int32_t QueryTotalNumberByMediaType(std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string &tableName,
        MediaType mediaType);
    size_t StatClonetotalSize(std::shared_ptr<NativeRdb::RdbStore> mediaRdb);
    std::string GetBackupInfoByCount(int32_t photoCount, int32_t videoCount, int32_t audioCount, size_t totalSize);
    void MoveMigrateFile(std::vector<FileInfo> &fileInfos, int64_t &fileMoveCount, int64_t &videoFileMoveCount);
    void RestorePhotoBatch(int32_t offset, int32_t isRelatedToPhotoMap = 0);
    void RestoreBatchForCloud(int32_t offset, int32_t isRelatedToPhotoMap = 0);
    void RestoreAudioBatch(int32_t offset);
    void InsertPhotoRelated(std::vector<FileInfo> &fileInfos, int32_t sourceType) override;
    void SetFileIdReference(const std::vector<FileInfo> &fileInfos, std::string &selection,
        std::unordered_map<int32_t, int32_t> &fileIdMap);
    int32_t QueryMapTotalNumber(const std::string &baseQuerySql);
    std::vector<MapInfo> QueryMapInfos(const std::string &tableName, const std::string &baseQuerySql, int32_t offset,
        const std::unordered_map<int32_t, int32_t> &fileIdMap, const std::unordered_map<int32_t, int32_t> &albumIdMap);
    int64_t InsertMapByTable(const std::string &tableName, const std::vector<MapInfo> &mapInfos,
        std::unordered_set<int32_t> &albumSet);
    std::vector<NativeRdb::ValuesBucket> GetInsertValues(const std::vector<MapInfo> &mapInfos);
    std::string GetQueryWhereClauseByTable(const std::string &tableName);
    void SetSpecialAttributes(const std::string &tableName, const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        FileInfo &fileInfo);
    bool IsSameFileForClone(const std::string &tableName, FileInfo &fileInfo);
    NativeRdb::ValuesBucket GetInsertValue(const AnalysisAlbumTbl &portraitAlbumInfo);
    int32_t MovePicture(FileInfo &fileInfo);
    int32_t MoveMovingPhotoVideo(FileInfo &fileInfo);
    int32_t MoveEditedData(FileInfo &fileInfo);
    int32_t MoveThumbnail(FileInfo &fileInfo);
    int32_t MoveThumbnailDir(FileInfo &fileInfo);
    int32_t MoveCloudThumbnailDir(FileInfo &fileInfo);
    int32_t MoveAstc(FileInfo &fileInfo);
    void InitThumbnailStatus();
    bool InitAllKvStore();
    void CloseAllKvStore();
    bool BackupKvStore();
    void GetThumbnailInsertValue(const FileInfo &fileInfo, NativeRdb::ValuesBucket &values);
    void GetCloudThumbnailInsertValue(const FileInfo &fileInfo, NativeRdb::ValuesBucket &values);
    void SetRestoreFailedAndErrorCount(uint64_t &failed, uint64_t &error) override;
    int32_t GetNoNeedMigrateCount() override;
    void GetAccountValid() override;
    int32_t GetHighlightCloudMediaCnt();
    void RestoreHighlightAlbums();
    void AddToPhotosFailedOffsets(int32_t offset);
    void ProcessPhotosBatchFailedOffsets(int32_t isRelatedToPhotoMap = 0);
    void ProcessCloudPhotosFailedOffsets(int32_t isRelatedToPhotoMap = 0);
    void RestoreAnalysisTablesData();
    void RestoreAnalysisData();
    void RestoreAssetMapData();
    void RestoreSearchIndexData();
    void RestoreAnalysisClassify();
    void RestoreAnalysisPortrait();
    void RestoreAnalysisPet();
    void RestoreAnalysisGeo();
    void RestoreGroupPhoto();
    void PopulateAnalysisAlbumIdMap();
    void StoreHighlightAlbumMappings(CloneRestoreHighlight& cloneRestoreHighlight);
    void RestoreBeautyScoreData();
    void RestoreVideoFaceData();
    void PrepareShootingModeVal(const FileInfo &fileInfo, NativeRdb::ValuesBucket &values);
    void SetTimeInfo(const FileInfo &info, NativeRdb::ValuesBucket &values);
    void GetInsertValueFromValMap(const FileInfo &fileInfo, NativeRdb::ValuesBucket &values);
    void SetAggregateBitThird();
    bool ShouldRestoreFromCloud();
    void UpdateRiskStatusForSamePhotos(vector<FileInfo> &fileInfos);
    bool CheckDestDbHasRiskStatusColumn();
    bool CheckSrcDbHasRiskStatusColumn();

    template<typename T>
    static void PutIfPresent(NativeRdb::ValuesBucket& values, const std::string& columnName,
        const std::optional<T>& optionalValue);

    template<typename T, typename U>
    void PutWithDefault(NativeRdb::ValuesBucket& values, const std::string& columnName,
        const std::optional<T>& optionalValue, const U& defaultValue);
    std::string GetThumbnailLocalPath(const string path);
    void BatchUpdateFileInfoData(std::vector<FileInfo> &fileInfos,
        unordered_map<string, CloudPhotoFileExistFlag> &resultExistMap);
    int32_t CheckThumbReady(const FileInfo &fileInfo,
        const CloudPhotoFileExistFlag &cloudPhotoFileExistFlag);
    int32_t CheckThumbStatus(const FileInfo &fileInfo,
        const CloudPhotoFileExistFlag &cloudPhotoFileExistFlag);
    int32_t CheckLcdVisitTime(const CloudPhotoFileExistFlag &cloudPhotoFileExistFlag);
    void AddToPhotoInfoMaps(std::vector<FileInfo> &fileInfos);
    void GetOrientationAndExifRotateValue(const shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &fileInfo);
    void UpdateExistNewAddColumnSet(const std::unordered_map<string, string> &srcColumnInfoMap);
    bool UpdateConfigInfo();
    void CheckSrcDstSwitchStatusMatch();
    bool BackupPreprocess();
    void ParseDstDeviceBackupInfo();
    bool InvalidateHdcCloudData(std::shared_ptr<NativeRdb::RdbStore> &rdbStore);
    CloneRestoreConfigInfo GetCurrentDeviceCloneConfigInfo();
    CloneRestoreConfigInfo GetCloneConfigInfoFromOriginDB();
    bool CheckSouthDeviceTypeMatchSwitchStatus(SwitchStatus switchStatus);
    bool CheckAlbumNameUnique(std::string albumName, const std::vector<string> &repetedAlbumName);
    void UpdateSourceAlbumName(bool &isUinque, vector<AlbumInfo> &albumInfos, vector<string> &repetedAlbumName,
        size_t index);

private:
    std::atomic<uint64_t> migrateDatabaseAlbumNumber_{0};
    std::atomic<uint64_t> migrateDatabaseMapNumber_{0};
    std::atomic<uint64_t> migrateLakeFileNumber_{0};
    std::atomic<uint64_t> migrateLakeVideoFileNumber_{0};
    std::atomic<uint64_t> migrateLakePhotoDuplicateNumber_{0};
    std::atomic<uint64_t> migrateLakeVideoDuplicateNumber_{0};
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb_;
    std::string filePath_;
    std::string dbPath_;
    std::string tmpDbPath_;
    std::unordered_map<std::string, bool> tableColumnStatusMap_;
    std::unordered_map<std::string, std::string> tableQueryWhereClauseMap_;
    std::unordered_map<std::string, std::string> tableExtraQueryWhereClauseMap_;
    std::unordered_map<std::string, std::unordered_map<int32_t, int32_t>> tableAlbumIdMap_;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> tableCommonColumnInfoMap_;
    std::string garbagePath_;
    PhotoAlbumClone photoAlbumClone_;
    PhotosClone photosClone_;
    static constexpr int32_t INVALID_COVER_SATISFIED_STATUS = 0;
    bool hasCloneThumbnailDir_{false};
    bool isInitKvstoreSuccess_{false};
    std::shared_ptr<MediaLibraryKvStore> oldMonthKvStorePtr_ = nullptr;
    std::shared_ptr<MediaLibraryKvStore> oldYearKvStorePtr_ = nullptr;
    std::shared_ptr<MediaLibraryKvStore> newMonthKvStorePtr_ = nullptr;
    std::shared_ptr<MediaLibraryKvStore> newYearKvStorePtr_ = nullptr;
    std::vector<int> photosFailedOffsets_;
    ffrt::mutex photosFailedMutex_;
    std::atomic<uint64_t> lcdMigrateFileNumber_{0};
    std::atomic<uint64_t> thumbMigrateFileNumber_{0};
    std::atomic<uint64_t> migrateCloudSuccessNumber_{0};
    CloneRestoreGeoDictionary cloneRestoreGeoDictionary_;
    CloneRestoreAnalysisData cloneRestoreAnalysisData_;
    int64_t maxSearchId_ {0};
    int64_t maxBeautyFileId_ {0};
    std::unordered_map<int32_t, PhotoInfo> photoInfoMap_;
    std::unordered_set<std::string> existNewAddColumnSet_;
    bool isSrcDstSwitchStatusMatch_ {false};
    CloneRestoreConfigInfo dstCloneRestoreConfigInfo_;
    CloneRestoreConfigInfo srcCloneRestoreConfigInfo_;
    DstDeviceBackupInfo dstDeviceBackupInfo_;

private:
    const std::string SQL_QUERY_CLASSIFY_ALBUM_EXIST = " \
        SELECT \
            count(1) AS count \
        FROM AnalysisAlbum \
        WHERE \
            album_type = ? \
        AND \
            album_subtype = ?;";
};

template<typename T>
void CloneRestore::PutIfPresent(NativeRdb::ValuesBucket& values, const std::string& columnName,
    const std::optional<T>& optionalValue)
{
    if (optionalValue.has_value()) {
        if constexpr (std::is_same_v<std::decay_t<T>, int32_t>) {
            values.PutInt(columnName, optionalValue.value());
        } else if constexpr (std::is_same_v<std::decay_t<T>, int64_t>) {
            values.PutLong(columnName, optionalValue.value());
        } else if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
            values.PutString(columnName, optionalValue.value());
        } else if constexpr (std::is_same_v<std::decay_t<T>, double>) {
            values.PutDouble(columnName, optionalValue.value());
        }  else if constexpr (std::is_same_v<std::decay_t<T>, std::vector<uint8_t>>) {
            values.PutBlob(columnName, optionalValue.value());
        }
    }
}

template<typename T, typename U>
void CloneRestore::PutWithDefault(NativeRdb::ValuesBucket& values, const std::string& columnName,
    const std::optional<T>& optionalValue, const U& defaultValue)
{
    if (optionalValue.has_value()) {
        PutIfPresent(values, columnName, optionalValue);
    } else {
        PutIfPresent(values, columnName, std::optional<T>(static_cast<T>(defaultValue)));
    }
}
} // namespace Media
} // namespace OHOS

#endif  // OHOS_MEDIA_CLONE_RESTORE_H

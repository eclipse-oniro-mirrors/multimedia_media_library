/*
 * Copyright (C) 2023-2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_UPGRADE_RESTORE_H
#define OHOS_MEDIA_UPGRADE_RESTORE_H

#include <libxml/tree.h>
#include <libxml/parser.h>

#include "backup_database_helper.h"
#include "base_restore.h"
#include "burst_key_generator.h"
#include "classify_restore.h"
#include "ffrt.h"
#include "ffrt_inner.h"
#include "ocr_restore.h"
#include "photos_restore.h"
// LCOV_EXCL_START
namespace OHOS {
namespace Media {
struct RestoreConfigInfo {
    SwitchStatus restoreSwitchType = SwitchStatus::NONE;
    SouthDeviceType southDeviceType = SouthDeviceType::SOUTH_DEVICE_NULL;
    PhotoPositionType photoPositionType = PhotoPositionType::INVALID;
};
class UpgradeRestore : public BaseRestore {
public:
    UpgradeRestore(const std::string &galleryAppName, const std::string &mediaAppName, int32_t sceneCode);
    UpgradeRestore(const std::string &galleryAppName, const std::string &mediaAppName, int32_t sceneCode,
        const std::string &dualDirName);
    virtual ~UpgradeRestore() = default;
    int32_t Init(const std::string &backupRestorePath, const std::string &upgradePath, bool isUpgrade) override;
    std::vector<FileInfo> QueryFileInfos(int32_t offset);
    std::vector<FileInfo> QueryCloudFileInfos(int32_t offset);
    NativeRdb::ValuesBucket GetInsertValue(const FileInfo &fileInfo, const std::string &newPath,
        int32_t sourceType) override;
    std::vector<FileInfo> QueryFileInfosFromExternal(int32_t offset, int32_t maxId, bool isCamera);
    std::vector<FileInfo> QueryAudioFileInfosFromAudio(int32_t offset);
    int32_t QueryNotSyncTotalNumber(int32_t offset, bool isCamera);
    void InitGarbageAlbum();
    void InheritManualCover();
    int32_t RecordAlbumCoverInfo(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        AlbumCoverInfo& albumCoverInfo);
    void UpdatePhotoAlbumCoverUri(vector<AlbumCoverInfo>& albumCoverInfos);
    void DeleteEmptyAlbums();
    RestoreConfigInfo GetCurrentDeviceRestoreConfigInfo();

protected:
    virtual void RestoreAnalysisAlbum();

    int32_t GetHighlightCloudMediaCnt();
    void RestoreSmartAlbums();
    void RestoreHighlightAlbums();
    void RestorePhoto(void) override;
    void RestoreAudio(void) override;
    void HandleRestData(void) override;
    bool ParseResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &info,
        std::string dbName = "") override;
    bool ParseResultSetForAudio(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &info) override;
    bool NeedBatchQueryPhotoForPortrait(const std::vector<FileInfo> &fileInfos, NeedQueryMap &needQueryMap) override;
    void InsertFaceAnalysisData(const std::vector<FileInfo> &fileInfos, const NeedQueryMap &needQueryMap,
        int64_t &faceRowNum, int64_t &mapRowNum, int64_t &photoNum) override;
    bool ParseResultSetFromExternal(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &info,
        int mediaType = DUAL_MEDIA_TYPE::IMAGE_TYPE);
    bool ParseResultSetFromAudioDb(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &info);
    virtual bool ParseResultSetFromGallery(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &info);
    void RestoreFromGallery();
    void RestoreCloudFromGallery();
    void RestoreFromExternal(bool isCamera);
    void RestoreAudioFromFile();
    bool IsValidDir(const std::string &path);
    void RestoreBatch(int32_t offset);
    void RestoreBatchForCloud(int32_t offset);
    void RestoreAudioBatch(int32_t offset);
    void RestoreExternalBatch(int32_t offset, int32_t maxId, bool isCamera, int32_t type);
    bool ConvertPathToRealPath(const std::string &srcPath, const std::string &prefix, std::string &newPath,
        std::string &relativePath) override;
    void AnalyzeSource() override;
    void AnalyzeGalleryErrorSource();
    void AnalyzeGalleryDuplicateData();
    void AnalyzeGallerySource();
    int32_t ParseXml(std::string path);
    int StringToInt(const std::string& str);
    int32_t InitDbAndXml(std::string xmlPath, bool isUpgrade);
    int32_t HandleXmlNode(xmlNodePtr cur);
    virtual bool ConvertPathToRealPath(const std::string &srcPath, const std::string &prefix, std::string &newPath,
        std::string &relativePath, FileInfo &fileInfo);
    bool HasSameFileForDualClone(FileInfo &fileInfo) override;
    void RestoreFromGalleryPortraitAlbum();
    int32_t QueryPortraitAlbumTotalNumber();
    std::vector<PortraitAlbumInfo> QueryPortraitAlbumInfos(int32_t offset,
        std::vector<std::string>& tagNameToDeleteSelection);
    bool ParsePortraitAlbumResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        PortraitAlbumInfo &portraitAlbumInfo);
    bool SetAttributes(PortraitAlbumInfo &portraitAlbumInfo);
    void InsertPortraitAlbum(std::vector<PortraitAlbumInfo> &portraitAlbumInfos);
    int32_t InsertPortraitAlbumByTable(std::vector<PortraitAlbumInfo> &portraitAlbumInfos, bool isAlbum);
    std::vector<NativeRdb::ValuesBucket> GetInsertValues(std::vector<PortraitAlbumInfo> &portraitAlbumInfos,
        bool isAlbum);
    NativeRdb::ValuesBucket GetInsertValue(const PortraitAlbumInfo &portraitAlbumInfo, bool isAlbum);
    void BatchQueryAlbum(std::vector<PortraitAlbumInfo> &portraitAlbumInfos);
    void SetHashReference(const std::vector<FileInfo> &fileInfos, const NeedQueryMap &needQueryMap,
        std::string &hashSelection, std::unordered_map<std::string, FileInfo> &fileInfoMap);
    int32_t QueryFaceTotalNumber(const std::string &hashSelection);
    std::vector<FaceInfo> QueryFaceInfos(const std::string &hashSelection,
        const std::unordered_map<std::string, FileInfo> &fileInfoMap, int32_t offset,
        std::unordered_set<std::string> &excludedFiles);
    bool ParseFaceResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, FaceInfo &faceInfo);
    bool SetAttributes(FaceInfo &faceInfo, const std::unordered_map<std::string, FileInfo> &fileInfoMap);
    int32_t InsertFaceAnalysisDataByTable(const std::vector<FaceInfo> &faceInfos, bool isMap,
        const std::unordered_set<std::string> &excludedFiles);
    std::vector<NativeRdb::ValuesBucket> GetInsertValues(const std::vector<FaceInfo> &faceInfos, bool isMap,
        const std::unordered_set<std::string> &excludedFiles);
    NativeRdb::ValuesBucket GetInsertValue(const FaceInfo &faceInfo, bool isMap);
    void UpdateFilesWithFace(std::unordered_set<std::string> &filesWithFace, const std::vector<FaceInfo> &faceInfos);
    void UpdateFaceAnalysisStatus();
    void UpdateDualCloneFaceAnalysisStatus();
    bool HasLowQualityImage();
    std::string CheckInvalidFile(const FileInfo &fileInfo, int32_t errCode) override;
    int32_t GetNoNeedMigrateCount() override;
    bool IsBasicInfoValid(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &info,
        const std::string &dbName);
    std::string CheckGalleryDbIntegrity();
    void RestorePhotoInner();
    void AddToGalleryFailedOffsets(int32_t offset);
    void AddToExternalFailedOffsets(int32_t offset);
    void ProcessGalleryFailedOffsets();
    void ProcessCloudGalleryFailedOffsets();
    void ProcessExternalFailedOffsets(int32_t maxId, bool isCamera, int32_t type);
    void SetCloneParameterAndStopSync();
    int32_t InitDb(bool isUpgrade);
    std::string GetCloudQuerySql();
    std::vector<int32_t> GetCloudPhotoMinIds();
    std::vector<int32_t> GetLocalPhotoMinIds();
    void SetOrientationAndExifRotate(FileInfo &info, NativeRdb::ValuesBucket &value,
        std::unique_ptr<Metadata> &data) override;

protected:
    std::shared_ptr<NativeRdb::RdbStore> galleryRdb_;
    std::shared_ptr<NativeRdb::RdbStore> externalRdb_;
    std::shared_ptr<NativeRdb::RdbStore> audioRdb_;
    BurstKeyGenerator burstKeyGenerator_;
    std::string galleryDbPath_;
    std::string filePath_;
    std::string externalDbPath_;
    std::string appDataPath_;
    std::string galleryAppName_;
    std::string mediaAppName_;
    std::string audioAppName_;
    std::set<std::string> cacheSet_;
    std::unordered_map<std::string, std::string> nickMap_;
    std::unordered_map<std::string, GalleryAlbumInfo> galleryAlbumMap_;
    std::vector<AlbumInfo> photoAlbumInfos_;
    std::string audioDbPath_;
    std::string hiddenAlbumBucketId_;
    int32_t mediaScreenreCorderAlbumId_{-1};
    bool shouldIncludeSd_{false};
    PhotoAlbumRestore photoAlbumRestore_;
    PhotosRestore photosRestore_;
    BackupDatabaseHelper backupDatabaseHelper_;
    ClassifyRestore classifyRestore_;
    std::vector<int> galleryFailedOffsets_;
    std::vector<int> externalFailedOffsets_;
    ffrt::mutex galleryFailedMutex_;
    ffrt::mutex externalFailedMutex_;
    int32_t maxId_{-1};
    int64_t maxAnalysisAlbumId_ {0};
    RestoreConfigInfo restoreConfig_;

private:
    void BatchDeleteEmptyAlbums(const std::vector<int32_t> &batchAlbumIds, int32_t &deleteRows);
};
} // namespace Media
} // namespace OHOS
// LCOV_EXCL_STOP
#endif  // OHOS_MEDIA_UPGRADE_RESTORE_H

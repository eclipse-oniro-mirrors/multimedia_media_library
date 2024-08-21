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

#include "base_restore.h"
#include "burst_key_generator.h"
#include <libxml/tree.h>
#include <libxml/parser.h>

namespace OHOS {
namespace Media {
class UpgradeRestore : public BaseRestore {
public:
    UpgradeRestore(const std::string &galleryAppName, const std::string &mediaAppName, int32_t sceneCode);
    UpgradeRestore(const std::string &galleryAppName, const std::string &mediaAppName, int32_t sceneCode,
        const std::string &dualDirName);
    virtual ~UpgradeRestore() = default;
    int32_t Init(const std::string &backupRestorePath, const std::string &upgradePath, bool isUpgrade) override;
    int32_t QueryTotalNumber(void) override;
    std::vector<FileInfo> QueryFileInfos(int32_t offset) override;
    NativeRdb::ValuesBucket GetInsertValue(const FileInfo &fileInfo, const std::string &newPath,
        int32_t sourceType) const override;
    std::vector<FileInfo> QueryFileInfosFromExternal(int32_t offset, int32_t maxId, bool isCamera);
    std::vector<FileInfo> QueryAudioFileInfosFromExternal(int32_t offset);
    std::vector<FileInfo> QueryAudioFileInfosFromAudio(int32_t offset);
    int32_t QueryNotSyncTotalNumber(int32_t offset, bool isCamera);
    void InitGarbageAlbum();
    void HandleClone();

private:
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
    bool ParseResultSetFromGallery(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &info);
    void RestoreFromGallery();
    void RestoreFromExternal(bool isCamera);
    void RestoreAudioFromExternal();
    void RestoreAudioFromFile();
    bool IsValidDir(const std::string &path);
    void RestoreBatch(int32_t offset);
    void RestoreAudioBatch(int32_t offset);
    void RestoreExternalBatch(int32_t offset, int32_t maxId, bool isCamera, int32_t type);
    bool ConvertPathToRealPath(const std::string &srcPath, const std::string &prefix, std::string &newPath,
        std::string &relativePath) override;
    void AnalyzeSource() override;
    void AnalyzeGallerySource();
    void AnalyzeExternalSource();
    void HandleCloneBatch(int32_t offset, int32_t maxId);
    void UpdateCloneWithRetry(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, int32_t &number);
    void RestoreFromGalleryAlbum();
    std::vector<GalleryAlbumInfo> QueryGalleryAlbumInfos();
    std::vector<AlbumInfo> QueryPhotoAlbumInfos();
    int32_t QueryAlbumTotalNumber(const std::string &tableName, bool bgallery);
    bool ParseAlbumResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, AlbumInfo &albumInfo);
    bool ParseGalleryAlbumResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        GalleryAlbumInfo &galleryAlbumInfos);
    void InsertAlbum(std::vector<GalleryAlbumInfo> &galleryAlbumInfos, bool bInsertScreenreCorderAlbum);
    std::vector<NativeRdb::ValuesBucket> GetInsertValues(std::vector<GalleryAlbumInfo> &galleryAlbumInfos,
        bool bInsertScreenreCorderAlbum);
    void BatchQueryAlbum(std::vector<GalleryAlbumInfo> &galleryAlbumInfos);
    void UpdateMediaScreenreCorderAlbumId();
    void UpdatehiddenAlbumBucketId();
    void UpdateHiddenAlbumToMediaAlbumId(const std::string &sourcePath, FileInfo &info);
    void InstertHiddenAlbum(const std::string &ablumIPath, FileInfo &info);
    void UpdateGalleryAlbumInfo(GalleryAlbumInfo &galleryAlbumInfo);
    void IntegratedAlbum(GalleryAlbumInfo &galleryAlbumInfo);
    void ParseResultSetForMap(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &info);
    void UpdateFileInfo(const GalleryAlbumInfo &galleryAlbumInfo, FileInfo &info);
    int32_t ParseXml(std::string path);
    int StringToInt(const std::string& str);
    int32_t InitDbAndXml(std::string xmlPath, bool isUpgrade);
    int32_t HandleXmlNode(xmlNodePtr cur);
    bool ConvertPathToRealPath(const std::string &srcPath, const std::string &prefix, std::string &newPath,
        std::string &relativePath, FileInfo &fileInfo);
    void RestoreFromGalleryPortraitAlbum();
    int32_t QueryPortraitAlbumTotalNumber();
    std::vector<PortraitAlbumInfo> QueryPortraitAlbumInfos(int32_t offset);
    bool ParsePortraitAlbumResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        PortraitAlbumInfo &portraitAlbumInfo);
    bool SetAttributes(PortraitAlbumInfo &portraitAlbumInfo);
    void UpdateGroupTagMap(const PortraitAlbumInfo &portraitAlbumInfo);
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

private:
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
};
} // namespace Media
} // namespace OHOS

#endif  // OHOS_MEDIA_UPGRADE_RESTORE_H

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

#ifndef OHOS_MEDIA_OTHERS_CLONE_RESTORE_H
#define OHOS_MEDIA_OTHERS_CLONE_RESTORE_H

#include <sys/stat.h>

#include "base_restore.h"
#include "photos_restore.h"

namespace OHOS {
namespace Media {

struct CloneDbInfo {
    std::string displayName;
    std::string data;
    double dateModified {0.0};
    double dateTaken {0.0};
    double latitude {0.0};
    double longitude {0.0};
    bool fileExists {false};
};

class OthersCloneRestore : public BaseRestore {
public:
    enum UserId {
        MAIN = 0,
        PRIVATE = 10
    };

    OthersCloneRestore(int32_t sceneCode, const std::string &mediaAppName, const std::string &bundleInfo = "");
    virtual ~OthersCloneRestore() = default;

    int32_t Init(const std::string &backupRetorePath, const std::string &upgradePath, bool isUpgrade);
    NativeRdb::ValuesBucket GetInsertValue(const FileInfo &fileInfo, const std::string &newPath,
        int32_t sourceType);

private:
    void RestorePhoto(void);
    void RestoreAudio(void);
    void HandleRestData(void);
    void ReportCloneBefore(void);
    bool ParseResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &info,
        std::string dbName = "");
    bool ParseResultSetForAudio(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &info);
    void AnalyzeSource();

    void InsertPhoto(std::vector<FileInfo> &fileInfos);
    void RestoreAlbum(std::vector<FileInfo> &fileInfos);
    void UpdateAlbumInfo(FileInfo &info);
    bool NeedBatchQueryPhotoForPortrait(const std::vector<FileInfo> &fileInfos, NeedQueryMap &needQueryMap);
    void AddAudioFile(FileInfo &tmpInfo);
    void SetFileInfosInCurrentDir(const std::string &file, struct stat &statInfo);
    int32_t GetAllfilesInCurrentDir(const std::string &path);
    bool CheckSamePathForSD(const std::string &dataPath, FileInfo &fileInfo, const std::string &filePath);
    std::string GenerateSearchKey(const FileInfo &fileInfo);
    CloneDbInfo* FindAudioDbInfo(const std::string &key, const std::string &displayName);
    CloneDbInfo* FindPhotoDbInfo(const std::string &key, const std::string &displayName);
    CloneDbInfo* FindDbInfoByFileType(const FileInfo &fileInfo, const std::string &key);
    double ConvertTimeToSeconds(double timeValue);
    void UpdateFileTimeInfo(FileInfo &fileInfo, CloneDbInfo *dbInfo);
    void UpDateFileModifiedTime(FileInfo &fileInfo);
    void UpdateFileGPS(FileInfo &fileInfo);
    void ReportMissingFilesFromDB(std::vector<CloneDbInfo> &mediaDbInfo, const std::string &dbType);
    void GetDbInfo(int32_t sceneCode, std::vector<CloneDbInfo> &mediaDbInfo,
        const std::shared_ptr<NativeRdb::ResultSet> &resultSet);
    std::string BuildDbPath(const std::string &dbName);
    bool CheckDbExists(const std::string &dbPath, const std::string &dbName);
    std::shared_ptr<NativeRdb::RdbStore> InitializeDatabase(const std::string &dbName, const std::string &dbPath);
    int32_t GetTotalRecordCount(std::shared_ptr<NativeRdb::RdbStore> mediaRdb,
        const std::string &dbName);
    void ProcessBatch(std::shared_ptr<NativeRdb::RdbStore> mediaRdb, int32_t offset,
        std::vector<CloneDbInfo> &mediaDbInfo);
    void ProcessChunk(std::shared_ptr<NativeRdb::RdbStore> mediaRdb, int32_t chunkStart,
        int32_t chunkEnd, std::vector<CloneDbInfo> &mediaDbInfo);
    void QueryDatabaseRecords(std::shared_ptr<NativeRdb::RdbStore> mediaRdb,
        int32_t totalNumber, std::vector<CloneDbInfo> &mediaDbInfo);
    bool IsPhotoVideoDatabase(const std::string &dbName);
    void BuildPhotoVideoDbMap(std::vector<CloneDbInfo> &mediaDbInfo);
    void BuildAudioDbMap(std::vector<CloneDbInfo> &mediaDbInfo);
    void BuildDbInfoMap(const std::string &dbName, std::vector<CloneDbInfo> &mediaDbInfo);
    void GetCloneDbInfos(const std::string &dbName, std::vector<CloneDbInfo> &mediaDbInfo);
    bool HasSameFileForDualClone(FileInfo &fileInfo);
    bool ConvertPathToRealPath(const std::string &srcPath, const std::string &prefix, std::string &newPath,
        std::string &relativePath, FileInfo &fileInfo);
    bool ConvertPathToRealPath(const std::string &srcPath, const std::string &prefix, std::string &newPath,
        std::string &relativePath);
    void HandleSelectBatch(std::shared_ptr<NativeRdb::RdbStore> mediaRdb, int32_t offset, int32_t sceneCode,
        std::vector<CloneDbInfo> &mediaDbInfo);
    void CloneInfoPushBack(std::vector<CloneDbInfo> &pushInfos, std::vector<CloneDbInfo> &popInfos);
    void HandleInsertBatch(int32_t offset);
    PhotoAlbumDao::PhotoAlbumRowData FindAlbumInfo(FileInfo &fileInfo);
    bool IsPrivateAccount();

    static std::string ParseSourcePathToLPath(int32_t sceneCode, const std::string &filePath, int32_t fileType);
    static std::string GetFileHeadPath(int32_t sceneCode, int32_t fileType);
    static void AddGalleryAlbum(std::vector<PhotoAlbumRestore::GalleryAlbumRowData> &galleryAlbumInfos,
        const std::string &lPath);
    static bool IsIosMovingPhotoVideo(FileInfo &fileInfo, int32_t sceneCode);

private:
    std::mutex cloneMutex_;
    std::string clonePhoneName_;
    std::string mediaAppName_;
    std::vector<FileInfo> photoInfos_;
    std::vector<FileInfo> audioInfos_;
    std::vector<CloneDbInfo> photoDbInfo_;
    std::vector<CloneDbInfo> audioDbInfo_;
    std::unordered_map<std::string, CloneDbInfo*> photoDbMap_;
    std::unordered_map<std::string, CloneDbInfo*> audioDbMap_;
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb_;
    PhotoAlbumDao photoAlbumDao_;
    PhotoAlbumRestore photoAlbumRestore_;
    PhotosRestore photosRestore_;
    int32_t photoFileNotFoundCount_ {0};
    int32_t audioFileNotFoundCount_ {0};
    int32_t userId_ {UserId::MAIN};
};
} // namespace Media
} // namespace OHOS

#endif  // OHOS_MEDIA_OTHERS_CLONE_RESTORE_H

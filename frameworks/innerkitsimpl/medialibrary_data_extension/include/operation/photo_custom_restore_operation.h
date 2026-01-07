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

#ifndef OHOS_MEDIA_PHOTO_CUSTOM_RESTORE_OPERATION_H
#define OHOS_MEDIA_PHOTO_CUSTOM_RESTORE_OPERATION_H

#include "asset_accurate_refresh.h"
#include "medialibrary_custom_restore_notify.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_rdb_utils.h"
#include "metadata.h"
#include "tlv_util.h"

namespace OHOS::Media {
struct RestoreTaskInfo {
    std::string dbPath;
    std::string albumLpath;
    std::string keyPath;
    bool isDeduplication;
    bool hasPhotoCache = false;
    std::string bundleName;
    std::string packageName;
    int32_t uriType;
    std::string uri;
    int32_t progress = 0;
    int32_t totalNum = 0;
    int32_t firstFileId;
    std::string firstFileUri;
    std::string appId;
    int32_t albumId;
    std::string imageAlbumUri;
    std::string videoAlbumUri;
    std::string sourceDir;
    int64_t beginTime;
    int64_t endTime;
    int32_t imageAlbumId;
    int32_t videoAlbumId;
};

struct FileInfo {
    std::string originFilePath;
    std::string filePath;
    std::string fileName;
    std::string displayName;
    std::string title;
    std::string extension;
    MediaType mediaType;
    int32_t size;
    int32_t orientation;
    bool isLivePhoto;
    int32_t fileId;
    std::string mimeType;
    int32_t subtype;
    int32_t movingPhotoEffectMode;
    std::string frontCamera;
    std::string shootingMode;
};

struct UniqueNumber {
    int32_t imageTotalNumber = 0;
    int32_t videoTotalNumber = 0;
    int32_t imageCurrentNumber = 0;
    int32_t videoCurrentNumber = 0;
};

struct TimeInfo {
    int64_t dateAdded{0};
    int64_t dateTaken{0};
    std::string detailTime;
};

class ShareRestoreRdbCallback : public NativeRdb::RdbOpenCallback {
public:
    virtual int32_t OnCreate(NativeRdb::RdbStore &rdb) override
    {
        return 0;
    }

    virtual int32_t OnUpgrade(NativeRdb::RdbStore &rdb, int32_t oldVersion, int32_t newVersion) override
    {
        return 0;
    }
};

const std::string CUSTOM_RESTORE_DIR = ROOT_MEDIA_DIR + CUSTOM_RESTORE_VALUES;
const int MAX_RESTORE_FILE_NUM = 200;
const int MAX_RESTORE_THREAD_NUM = 2;
const int RESTORE_URI_TYPE_PHOTO = 1;
const int RESTORE_URI_TYPE_ALBUM = 2;
const int PROGRESS_MULTI_NUM = 100;
const int NOTIFY_FIRST = 0;
const int NOTIFY_PROGRESS = 1;
const int NOTIFY_LAST = 2;
const int NOTIFY_CANCEL = 3;
const int BASE_EFFICIENCY_QUOTA = 712;
const int BASE_FILE_NUM = 25000;
const int MAX_PHOTO_CACHE_NUM = 5000;
const int MODULE_POWER_OVERUSED = 8;
const int64_t TIMEOUT_TASK_DIR_CLEAN_INTERVAL = 43200000;   //12h
const std::string ABNORMAL_MANAGER_LIB = "libabnormal_mgr.z.so";
const std::string ALBUM_PATH_PREFIX = "/Pictures/";

class PhotoCustomRestoreOperation {
public:
    static PhotoCustomRestoreOperation &GetInstance();
    PhotoCustomRestoreOperation &AddTask(RestoreTaskInfo restoreTaskInfo);
    PhotoCustomRestoreOperation &Start();
    void CancelTask(RestoreTaskInfo restoreTaskInfo);
    void CleanTimeoutCustomRestoreTaskDir();

private:
    void DoCustomRestore(RestoreTaskInfo &restoreTaskInfo);
    void InitRestoreTask(RestoreTaskInfo &restoreTaskInfo, int32_t fileNum);
    void ReleaseCustomRestoreTask(RestoreTaskInfo &restoreTaskInfo);
    int32_t HandleCustomRestore(const unordered_map<string, TimeInfo> &timeInfoMap, RestoreTaskInfo &restoreTaskInfo,
        const vector<string> &filePathVector, bool isFirst, UniqueNumber &uniqueNumber);
    bool HandleFirstRestoreFile(const unordered_map<string, TimeInfo> &timeInfoMap, RestoreTaskInfo &restoreTaskInfo,
        const vector<string> &files, int32_t index, int32_t &firstRestoreIndex);
    void HandleBatchCustomRestore(const unordered_map<string, TimeInfo> &timeInfoMap, RestoreTaskInfo &restoreTaskInfo,
        int32_t notifyType, const vector<string> &subFiles);
    int32_t HandleTlvRestore(const unordered_map<string, TimeInfo> &timeInfoMap, RestoreTaskInfo &restoreTaskInfo,
        const vector<string> &filePathVector, bool isFirst, UniqueNumber &uniqueNumber);
    void RestoreTlvRollback(const std::string &assetPath);
    vector<FileInfo> GetFileInfos(const vector<string> &filePathVector, UniqueNumber &uniqueNumber);
    int32_t UpdateUniqueNumber(UniqueNumber &uniqueNumber);
    int32_t CreateAssetUniqueNumber(int32_t type, UniqueNumber &uniqueNumber);
    vector<FileInfo> SetDestinationPath(vector<FileInfo> &restoreFiles, UniqueNumber &uniqueNumber);
    void GetAssetRootDir(int32_t mediaType, string &rootDirPath);
    vector<FileInfo> BatchInsert(const unordered_map<string, TimeInfo> &timeInfoMap, RestoreTaskInfo &restoreTaskInfo,
        vector<FileInfo> &restoreFiles, int32_t &sameFileNum, bool isFirst);
    NativeRdb::ValuesBucket GetInsertValue(
        const unordered_map<string, TimeInfo> &timeInfoMap, RestoreTaskInfo &restoreTaskInfo, FileInfo &fileInfo);
    int32_t FillMetadata(
        const unordered_map<string, TimeInfo> &timeInfoMap, const FileInfo &fileInfo, std::unique_ptr<Metadata> &data);
    int32_t GetFileMetadata(std::unique_ptr<Metadata> &data);
    int32_t RenameFiles(const vector<FileInfo> &restoreFiles);
    int32_t BatchUpdateTimePending(const vector<FileInfo> &restoreFiles,
        AccurateRefresh::AssetAccurateRefresh &assetRefresh);
    int32_t UpdatePhotoAlbum(RestoreTaskInfo &restoreTaskInfo, FileInfo fileInfo);
    void SendNotifyMessage(RestoreTaskInfo &restoreTaskInfo, int32_t notifyType, int32_t errCode, int32_t fileNum,
        const UniqueNumber &uniqueNumber);
    InnerRestoreResult GenerateCustomRestoreNotify(RestoreTaskInfo &restoreTaskInfo, int32_t notifyType);
    void SendPhotoAlbumNotify(RestoreTaskInfo &restoreTaskInfo, int32_t notifyType, const UniqueNumber &uniqueNumber);
    bool IsCancelTask(RestoreTaskInfo &restoreTaskInfo);
    void CancelTaskFinish(RestoreTaskInfo &restoreTaskInfo);
    void ApplyEfficiencyQuota(int32_t fileNum);
    bool IsDuplication(RestoreTaskInfo &restoreTaskInfo, FileInfo &fileInfo);
    int32_t InitPhotoCache(RestoreTaskInfo &restoreTaskInfo);
    void QueryAlbumId(RestoreTaskInfo &restoreTaskInfo);
    void ReportCustomRestoreTask(RestoreTaskInfo &restoreTaskInfo);
    int32_t MoveLivePhoto(const string &originFilePath, const string &filePath);
    void DeleteDatabaseRecord(const string &filePath);
    int32_t GetAlbumInfoBySubType(int32_t subType, string &albumUri, int32_t &albumId);
    unordered_map<string, TimeInfo> QueryMediaInfo(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore);
    unordered_map<string, TimeInfo> GetTimeInfoMap(RestoreTaskInfo &restoreTaskInfo);
    void SetTimeInfo(const std::unique_ptr<Metadata> &data, FileInfo &info, NativeRdb::ValuesBucket &value);

    int32_t HandleTlvSingleRestore(const std::unordered_map<TlvTag, std::string> &editFileMap,
        const unordered_map<string, TimeInfo> &timeInfoMap, RestoreTaskInfo &restoreTaskInfo, bool isFirst,
        UniqueNumber &uniqueNumber);
    static int32_t UpdateTlvEditDataSize(const std::string &assetPath);

    static std::string GetUniqueTempDir(const std::string &tlvPath);

    int32_t HandlePhotoSourceRestore(const std::string &sourceBackSrcPath, const std::string &assetPath);
    int32_t HandleEditDataRestore(const std::string &sourceBackSrcPath, const std::string &assetPath);
    int32_t HandleEditDataCameraRestore(const std::string &sourceBackSrcPath, const std::string &assetPath);
    int32_t HandleExtraDataRestore(const string &editDataCameraSrcPath, const std::string &assetPath);
    int32_t HandleMovingPhotoVideoRestore(const string &originalSrcPath, const std::string &assetPath);
    int32_t HandleDbFieldsFromJsonRestore(const std::string &jsonPath, const std::string &assetPath);
    int32_t HandleAllEditData(const std::unordered_map<TlvTag, std::string> &decodeTlvPathMap,
        const std::string &assetPath);
    int32_t HandlePhotoSourceBackRestore(const std::string &sourceBackSrcPath, const std::string &assetPath);
    int32_t HandleMovingPhotoVideoSourceRestore(const std::string &srcPath, const std::string &assetPath);
    int32_t HandleMovingPhotoVideoSourceBackRestore(const std::string &srcPath, const std::string &assetPath);
    static int32_t MoveFile(const std::string &srcPath, const std::string &destPath);
    static bool CheckNeedProcessMovingPhotoSize(const NativeRdb::ValuesBucket &values);

private:
    std::atomic<bool> isRunning_{false};
    static std::shared_ptr<PhotoCustomRestoreOperation> instance_;
    static std::mutex objMutex_;
    std::queue<RestoreTaskInfo> taskQueue_;
    std::unordered_set<std::string> cancelKeySet_;
    std::mutex uniqueNumberLock_;
    std::shared_mutex cancelOprationLock_;
    std::atomic<int32_t> successNum_{0};
    std::atomic<int32_t> failNum_{0};
    std::atomic<int32_t> sameNum_{0};
    std::unordered_set<std::string> photoCache_;
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_PHOTO_CUSTOM_RESTORE_OPERATION_H
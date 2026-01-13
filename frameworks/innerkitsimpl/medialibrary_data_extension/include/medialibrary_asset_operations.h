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

#ifndef MEDIALIBRARY_FILE_OPERATIONS
#define MEDIALIBRARY_FILE_OPERATIONS

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "abs_predicates.h"
#include "abs_shared_result_set.h"
#include "asset_compress_version_manager.h"
#include "datashare_predicates.h"
#include "datashare_values_bucket.h"
#include "file_asset.h"
#include "imedia_scanner_callback.h"
#include "media_column.h"
#include "medialibrary_async_worker.h"
#include "medialibrary_command.h"
#include "photo_album.h"
#include "picture.h"
#include "tlv_util.h"
#include "value_object.h"
#include "values_bucket.h"
#include "medialibrary_rdb_transaction.h"
#include "asset_accurate_refresh.h"
#include "cloud_sync_manager.h"
//LCOV_EXCL_START
namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
EXPORT extern const std::unordered_map<std::string, int>& GetFileAssetMemberMap();

typedef struct {
    int64_t sizeMp4;
    int64_t sizeExtra;
    int64_t size;
    int64_t dateModified;
    int64_t editTime;
    int32_t subType;
    int32_t effectMode;
    int32_t originalSubType;
    std::string videoPath;
    std::string extraPath;
    std::string editDataPath;
    std::string editDataCameraPath;
    std::string editDataSourcePath;
    std::string path;
    std::string cloudId;
    std::string displayName;
    std::string photoImagePath;
    std::string photoVideoPath;
    std::string cachePath;
} ExternalInfo;

struct TransCodeExifInfo {
    std::string orientation;
    std::string userComment;
    double latitude;
    double longitude;
    TransCodeExifInfo() : orientation("1"), userComment(""),
        latitude(0.0), longitude(0.0) {};
};

enum ExifType {
    EXIF_ORIENTATION,
    EXIF_USER_COMMENT,
    EXIF_GPS,
};

class MediaLibraryAssetOperations {
public:
    static int32_t HandleInsertOperation(MediaLibraryCommand &cmd);
    static int32_t HandleInsertOperationExt(MediaLibraryCommand& cmd);
    static int32_t DeleteOperation(MediaLibraryCommand &cmd);
    static std::shared_ptr<NativeRdb::ResultSet> QueryOperation(MediaLibraryCommand &cmd,
        const std::vector<std::string> &columns);
    EXPORT static int32_t UpdateOperation(MediaLibraryCommand &cmd);
    static int32_t OpenOperation(MediaLibraryCommand &cmd, const std::string &mode);
    static int32_t DeleteToolOperation(MediaLibraryCommand &cmd);

    EXPORT static int32_t CreateAssetBucket(int32_t fileId, int32_t &bucketNum);
    EXPORT static int32_t CreateAssetUniqueId(int32_t type,
        std::shared_ptr<TransactionOperations> trans = nullptr);
    EXPORT static int32_t CreateAssetUniqueIds(int32_t type, int32_t num, int32_t &startUniqueNumber);
    EXPORT static int32_t CreateAssetPathById(int32_t fileId, int32_t mediaType, const std::string &extension,
        std::string &filePath);
    EXPORT static int32_t DeleteFromDisk(NativeRdb::AbsRdbPredicates &predicates, const bool isAging,
        const bool compatible = false);
    EXPORT static int32_t DeletePermanently(NativeRdb::AbsRdbPredicates &predicates, const bool isAging,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh = nullptr);
    EXPORT static int32_t DeleteNormalPhotoPermanently(std::shared_ptr<FileAsset> &fileAsset,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh = nullptr);
    EXPORT static std::string GetEditDataSourcePath(const std::string &path);
    EXPORT static std::string GetEditDataSourceBackPath(const string& path);
    EXPORT static int32_t GetAlbumIdByPredicates(const std::string &whereClause,
        const std::vector<std::string> &whereArgs);
    EXPORT static int32_t CheckExist(const std::string &path);
    EXPORT static int32_t QueryTotalPhoto(std::vector<std::shared_ptr<FileAsset>> &fileAssetVector, int32_t batchSize);
    EXPORT static int32_t QueryTotalAlbum(std::vector<std::shared_ptr<PhotoAlbum>> &PhotoAlbumVector);
    EXPORT static std::shared_ptr<FileAsset> QuerySinglePhoto(int32_t rowId);
    EXPORT static std::vector<std::string> QueryPhotosTableColumnInfo();
    EXPORT static bool GetInt32FromValuesBucket(const NativeRdb::ValuesBucket &values, const std::string &column,
        int32_t &value);
    EXPORT static bool GetStringFromValuesBucket(const NativeRdb::ValuesBucket &values, const std::string &column,
        std::string &value);
    EXPORT static std::string GetEditDataDirPath(const std::string &path);
    static std::shared_ptr<FileAsset> GetAssetFromResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        const std::vector<std::string> &columns);
    EXPORT static std::shared_ptr<FileAsset> GetFileAssetFromDb(const std::string &column, const std::string &value,
        OperationObject oprnObject, const std::vector<std::string> &columns = {}, const std::string &networkId = "");
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    EXPORT static int32_t DealWithBatchDownloadingFiles(std::vector<shared_ptr<FileAsset>> &subFileAssetVector);
    EXPORT static int32_t DealWithBatchDownloadingFilesById(std::vector<std::string> &fileIds);
#endif
    static void ScanFile(const std::string &path, bool isCreateThumbSync, bool isInvalidateThumb,
        bool isForceScan = false, int32_t fileId = 0, std::shared_ptr<Media::Picture> resultPicture = nullptr);
    EXPORT static FileManagement::CloudSync::CleanFileInfo GetCleanFileInfo(shared_ptr<FileAsset> &fileAssetPtr);
    EXPORT static void ClearDeletedFile(const std::vector<std::string> &ids, const std::string &table);
    EXPORT static void SaveDeletedFile(const std::vector<std::string> &ids,
        const std::vector<std::string> &paths, const std::string &table, const std::vector<std::string> &dateTakens,
        std::vector<int32_t> &subTypes);
    EXPORT static void TaskDataFileProcess(const std::vector<std::string> &ids,
        const std::vector<std::string> &paths, const std::string &table, const std::vector<std::string> &dateTakens,
        std::vector<int32_t> &subTypes);
    EXPORT static std::string GetAssetCompressCachePath(const std::string &path);

protected:
    static std::shared_ptr<FileAsset> GetFileAssetFromDb(NativeRdb::AbsPredicates &predicates,
        OperationObject oprnObject, const std::vector<std::string> &columns = {}, const std::string &networkId = "");
    EXPORT static int32_t GetFileAssetVectorFromDb(NativeRdb::AbsPredicates &predicates, OperationObject oprnObject,
        std::vector<std::shared_ptr<FileAsset>> &fileAssetVector, const std::vector<std::string> &columns = {},
        const std::string &networkId = "");
    EXPORT static std::shared_ptr<FileAsset> GetFileAssetByUri(const std::string &fileUri, bool isPhoto,
        const std::vector<std::string> &columns, const std::string &pendingStatus = "");

    static int32_t CreateOperation(MediaLibraryCommand &cmd);
    static int32_t CloseOperation(MediaLibraryCommand &cmd);
    static int32_t InsertAssetInDb(std::shared_ptr<TransactionOperations> trans,
        MediaLibraryCommand &cmd, const FileAsset &fileAsset,
        shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh = nullptr);
    static int32_t CheckWithType(bool isContains, const std::string &displayName,
         const std::string &extention, int32_t mediaType);
    static int32_t CheckDisplayNameWithType(const std::string &displayName, int32_t mediaType);
    static int32_t CheckExtWithType(const std::string &extention, int32_t mediaType);
    static int32_t CheckRelativePathWithType(const std::string &relativePath, int32_t mediaType);
    static void GetAssetRootDir(int32_t mediaType, std::string &rootDirPath);
    EXPORT static int32_t SetAssetPathInCreate(FileAsset &fileAsset,
        std::shared_ptr<TransactionOperations> trans = nullptr);
    EXPORT static int32_t SetAssetPath(FileAsset &fileAsset, const std::string &extention,
        std::shared_ptr<TransactionOperations> trans = nullptr);
    EXPORT static int32_t DeleteAssetInDb(MediaLibraryCommand &cmd,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh = nullptr);

    EXPORT static int32_t UpdateFileName(MediaLibraryCommand &cmd, const std::shared_ptr<FileAsset> &fileAsset,
        bool &isNameChanged);
    EXPORT static int32_t SetUserComment(MediaLibraryCommand &cmd, const std::shared_ptr<FileAsset> &fileAsset);
    EXPORT static int32_t UpdateRelativePath(MediaLibraryCommand &cmd, const std::shared_ptr<FileAsset> &fileAsset,
        bool &isNameChanged);
    static void UpdateVirtualPath(MediaLibraryCommand &cmd, const std::shared_ptr<FileAsset> &fileAsset);
    static int32_t UpdateFileInDb(MediaLibraryCommand &cmd);
    EXPORT static int32_t OpenAsset(const std::shared_ptr<FileAsset> &fileAsset, const std::string &mode,
        MediaLibraryApi api, bool isMovingPhotoVideo = false, int32_t type = -1);
    static int32_t OpenHighlightCover(MediaLibraryCommand &cmd, const std::string &mode);
    static int32_t OpenHighlightVideo(MediaLibraryCommand &cmd, const std::string &mode);
    EXPORT static int32_t CloseAsset(const std::shared_ptr<FileAsset> &fileAsset, bool isCreateThumbSync = false);
    static void InvalidateThumbnail(const std::string &fileId, int32_t mediaType);
    static int32_t SendTrashNotify(MediaLibraryCommand &cmd, int32_t rowId, const std::string &extraUri = "",
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh = nullptr);
    static void SendFavoriteNotify(MediaLibraryCommand &cmd, std::shared_ptr<FileAsset> &fileAsset,
        const std::string &extraUri = "",
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh = nullptr);
    static void UpdateOwnerAlbumIdOnMove(MediaLibraryCommand &cmd, int32_t &targetAlbumId, int32_t &oriAlbumId);
    static int32_t SendModifyUserCommentNotify(MediaLibraryCommand &cmd, int32_t rowId,
        const std::string &extraUri = "");
    static int32_t SetPendingStatus(MediaLibraryCommand &cmd);
    EXPORT static int32_t GrantUriPermission(const std::string &uri, const std::string &bundleName,
        const std::string &path, bool isMovingPhoto = false);

    EXPORT static std::string CreateExtUriForV10Asset(FileAsset &fileAsset);
    EXPORT static int32_t OpenFileWithPrivacy(const std::string &filePath, const std::string &mode,
        const std::string &fileId, int32_t type = -1);
    static void ScanFileWithoutAlbumUpdate(const std::string &path, bool isCreateThumbSync, bool isInvalidateThumb,
        bool isForceScan = false, int32_t fileId = 0, std::shared_ptr<Media::Picture> resultPicture = nullptr);

    EXPORT static std::string GetEditDataPath(const std::string &path);
    EXPORT static std::string GetEditDataCameraPath(const std::string &path);
    static std::string GetAssetCacheDir();
    static std::string GetAssetCompressJsonPath(const std::string &path);
    static int32_t AddOtherBurstIdsToFileIds(std::vector<std::string> &fileIds);

private:
    static int32_t CreateAssetRealName(int32_t fileId, int32_t mediaType, const std::string &extension,
        std::string &name);
    static int32_t SetPendingTrue(const std::shared_ptr<FileAsset> &fileAsset);
    static int32_t SetPendingFalse(const std::shared_ptr<FileAsset> &fileAsset);

    static constexpr int ASSET_MAX_COMPLEMENT_ID = 999;

    class ScanAssetCallback : public IMediaScannerCallback {
    public:
        ScanAssetCallback() = default;
        ~ScanAssetCallback() = default;
        int32_t OnScanFinished(const int32_t status, const std::string &uri, const std::string &path) override;
        void SetSync(bool isSync)
        {
            isCreateThumbSync = isSync;
        }
        void SetIsInvalidateThumb(bool isInvalidate)
        {
            isInvalidateThumb = isInvalidate;
        }
        void SetOriginalPhotoPicture(std::shared_ptr<Media::Picture> resultPicture)
        {
            originalPhotoPicture = resultPicture;
        }
    private:
        bool isCreateThumbSync = false;
        bool isInvalidateThumb = true;
        std::shared_ptr<Media::Picture> originalPhotoPicture = nullptr;
    };
};

class DeleteFilesData : public AsyncTaskData {
public:
    DeleteFilesData(const std::vector<std::string> &ids, const std::vector<std::string> &paths,
        const std::vector<std::string> &dateTakens, const std::vector<int32_t> &subTypes, const std::string &table,
        int32_t deleteRows, std::string bundleName, bool containsHidden)
        : ids_(ids), paths_(paths), dateTakens_(dateTakens), subTypes_(subTypes),
        table_(table), deleteRows_(deleteRows), bundleName_(bundleName), containsHidden_(containsHidden) {}
    virtual ~DeleteFilesData() override = default;
    std::vector<std::string> ids_;
    std::vector<std::string> paths_;
    std::vector<std::string> notifyUris_;
    std::vector<std::string> dateTakens_;
    std::vector<int32_t> subTypes_;
    std::string table_;
    int32_t deleteRows_;
    std::string bundleName_;
    bool containsHidden_ = false;
};

class DeleteNotifyAsyncTaskData : public AsyncTaskData {
public:
    DeleteNotifyAsyncTaskData() = default;
    virtual ~DeleteNotifyAsyncTaskData() override = default;
    int32_t updateRows = 0;
    std::vector<std::string> notifyUris;
    std::string notifyUri;
    int64_t trashDate = 0;
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> refresh_ = nullptr;
};

using VerifyFunction = bool (*) (NativeRdb::ValueObject&, MediaLibraryCommand&);
class AssetInputParamVerification {
public:
    static bool CheckParamForUpdate(MediaLibraryCommand &cmd);

private:
    static bool Forbidden(NativeRdb::ValueObject &value, MediaLibraryCommand &cmd);
    static bool IsInt32(NativeRdb::ValueObject &value, MediaLibraryCommand &cmd);
    static bool IsInt64(NativeRdb::ValueObject &value, MediaLibraryCommand &cmd);
    static bool IsBool(NativeRdb::ValueObject &value, MediaLibraryCommand &cmd);
    static bool IsString(NativeRdb::ValueObject &value, MediaLibraryCommand &cmd);
    static bool IsDouble(NativeRdb::ValueObject &value, MediaLibraryCommand &cmd);
    static bool IsBelowApi9(NativeRdb::ValueObject &value, MediaLibraryCommand &cmd);
    static bool IsStringNotNull(NativeRdb::ValueObject &value, MediaLibraryCommand &cmd);
    static bool IsUniqueValue(NativeRdb::ValueObject &value, MediaLibraryCommand &cmd);

    static const std::unordered_map<std::string, std::vector<VerifyFunction>> UPDATE_VERIFY_PARAM_MAP;
};
} // namespace Media
} // namespace OHOS

#endif // MEDIALIBRARY_FILE_OPERATIONS
//LCOV_EXCL_STOP
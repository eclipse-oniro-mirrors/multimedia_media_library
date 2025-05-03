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

#ifndef MEDIALIBRARY_PHOTO_OPERATIONS_H
#define MEDIALIBRARY_PHOTO_OPERATIONS_H

#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>

#include "abs_shared_result_set.h"
#include "file_asset.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_command.h"
#include "picture.h"

namespace OHOS {
namespace Media {
struct PhotoExtInfo {
    std::string photoId;
    std::string format;
    std::string oldFilePath;
    std::string extension;
    std::shared_ptr<Media::Picture> picture;
};

class AlbumData {
public:
    AlbumData() = default;
    ~AlbumData() = default;
    std::vector<bool> isHidden = {};
};

#define EXPORT __attribute__ ((visibility ("default")))
class MediaLibraryPhotoOperations : public MediaLibraryAssetOperations {
public:
    EXPORT static int32_t Create(MediaLibraryCommand &cmd);
    EXPORT static std::shared_ptr<NativeRdb::ResultSet> Query(MediaLibraryCommand &cmd,
        const std::vector<std::string> &columns);
    EXPORT static int32_t Update(MediaLibraryCommand &cmd);
    EXPORT static int32_t Delete(MediaLibraryCommand &cmd);
    EXPORT static int32_t Open(MediaLibraryCommand &cmd, const std::string &mode);
    EXPORT static int32_t Close(MediaLibraryCommand &cmd);
    EXPORT static int32_t SubmitCache(MediaLibraryCommand &cmd);
    EXPORT static int32_t CommitEditInsert(MediaLibraryCommand &cmd);
    EXPORT static int32_t RevertToOrigin(MediaLibraryCommand &cmd);
    EXPORT static void DeleteRevertMessage(const std::string &path);
    EXPORT static std::shared_ptr<NativeRdb::ResultSet> ScanMovingPhoto(MediaLibraryCommand &cmd,
        const std::vector<std::string> &columns);
    EXPORT static int32_t AddFilters(MediaLibraryCommand &cmd);
    EXPORT static int32_t ProcessMultistagesPhoto(bool isEdited, const std::string &path,
        const uint8_t *addr, const long bytes, int32_t fileId);
    EXPORT static void StoreThumbnailSize(const std::string& photoId, const std::string& photoPath);
    EXPORT static bool HasDroppedThumbnailSize(const std::string& photoId);
    EXPORT static int32_t ScanFileWithoutAlbumUpdate(MediaLibraryCommand &cmd);
    EXPORT static int32_t ProcessMultistagesPhotoForPicture(bool isEdited, const std::string &path,
        std::shared_ptr<Media::Picture> &picture, int32_t fileId, const std::string &mime_type,
        std::shared_ptr<Media::Picture> &resultPicture, bool &isTakeEffect);
    EXPORT static int32_t Save(bool isEdited, const std::string &path,
        const uint8_t *addr, const long bytes, int32_t fileId);
    EXPORT static int32_t AddFiltersToPicture(std::shared_ptr<Media::Picture>& inPicture,
        const std::string &outputPath, std::string &editdata, const std::string &mime_type);
    EXPORT static int32_t SavePicture(const int32_t &fileType, const int32_t &fileId, const int32_t getPicRet,
        PhotoExtInfo &photoExtInfo, std::shared_ptr<Media::Picture> &resultPicture);
    EXPORT static int32_t GetPicture(const int32_t &fileId, std::shared_ptr<Media::Picture> &picture,
        bool isCleanImmediately, std::string &photoId, bool &isHighQualityPicture);
    EXPORT static int32_t FinishRequestPicture(MediaLibraryCommand &cmd);
    EXPORT static int64_t CloneSingleAsset(MediaLibraryCommand &cmd);
    EXPORT static int32_t AddFiltersForCloudEnhancementPhoto(int32_t fileId, const std::string& assetPath,
        const std::string& editDataCameraSourcePath, const std::string& mimeType);
    EXPORT static void UpdateSourcePath(const std::vector<std::string> &whereArgs,
        std::shared_ptr<AlbumData> AlbumData = nullptr);
    EXPORT static void TrashPhotosSendNotify(std::vector<std::string> &notifyUris,
        std::shared_ptr<AlbumData> AlbumData = nullptr);
    EXPORT static int32_t ProcessMultistagesVideo(bool isEdited, bool isMovingPhoto,
        bool isMovingPhotoEffectMode, const std::string &path);
    EXPORT static int32_t RemoveTempVideo(const std::string &path);
    EXPORT static int32_t SaveSourceVideoFile(const std::string &assetPath, const bool &isTemp);
    EXPORT static int32_t AddFiltersToVideoExecute(const std::string &assetPath,
        bool isSaveVideo, bool isNeedScan = false);
    EXPORT static int32_t DoRevertFilters(const std::shared_ptr<FileAsset> &fileAsset,
        std::string &path, std::string &sourcePath);
    EXPORT static int32_t CopyVideoFile(const std::string& assetPath, bool toSource);
    EXPORT static int32_t ProcessCustomRestore(MediaLibraryCommand &cmd);
    EXPORT static int32_t CancelCustomRestore(MediaLibraryCommand &cmd);
    EXPORT static int32_t UpdateSupportedWatermarkType(MediaLibraryCommand &cmd);
    static int32_t UpdateExtension(const int32_t &fileId, const int32_t &fileType, PhotoExtInfo &photoExtInfo,
        NativeRdb::ValuesBucket &updateValues);
private:
    static int32_t CreateV9(MediaLibraryCommand &cmd);
    static int32_t CreateV10(MediaLibraryCommand &cmd);
    static int32_t DeletePhoto(const std::shared_ptr<FileAsset> &fileAsset, MediaLibraryApi api);
    static int32_t UpdateV9(MediaLibraryCommand &cmd);
    static int32_t UpdateV10(MediaLibraryCommand &cmd);
    static int32_t TrashPhotos(MediaLibraryCommand &cmd);
    static void SolvePhotoAlbumInCreate(MediaLibraryCommand &cmd, FileAsset &fileAsset);
    static int32_t OpenCache(MediaLibraryCommand &cmd, const std::string &mode, bool &isCacheOperation);
    static int32_t DeleteCache(MediaLibraryCommand &cmd);
    static int32_t OpenEditOperation(MediaLibraryCommand &cmd, bool &isSkip);
    static int32_t RequestEditData(MediaLibraryCommand &cmd);
    static int32_t RequestEditSource(MediaLibraryCommand &cmd);
    static int32_t CommitEditOpen(MediaLibraryCommand &cmd);
    static int32_t CommitEditOpenExecute(const std::shared_ptr<FileAsset> &fileAsset);
    static int32_t CommitEditInsertExecute(const std::shared_ptr<FileAsset> &fileAsset,
        const std::string &editData);
    static int32_t DoRevertEdit(std::shared_ptr<FileAsset> &fileAsset);
    static std::string GetSourceFileFromEditPath(const std::string &path, const std::shared_ptr<FileAsset> &fileAsset);
    static int32_t UpdateDbByRevertToOrigin(
        std::shared_ptr<FileAsset> &fileAsset, std::string &path, const std::string &sourcePath);
    static int32_t RenameEditDataDirByRevert(const std::string &editDataDir, const std::string &path);
    static int32_t ParseMediaAssetEditData(MediaLibraryCommand &cmd, std::string &editData);
    static void ParseCloudEnhancementEditData(std::string& editData);
    static void CreateThumbnailFileScan(const std::shared_ptr<FileAsset> &fileAsset, std::string &extraUri,
        bool orientationUpdated, bool isNeedScan);
    static bool IsSetEffectMode(MediaLibraryCommand &cmd);
    static bool IsContainsData(MediaLibraryCommand &cm);
    static bool IsCameraEditData(MediaLibraryCommand &cmd);
    static int32_t ReadEditdataFromFile(const std::string &editDataPath, std::string &editData);
    static int32_t SaveEditDataCamera(MediaLibraryCommand &cmd, const std::string &assetPath,
        std::string &editData);
    static int32_t SaveSourceAndEditData(const std::shared_ptr<FileAsset> &fileAsset, const std::string &editData);
    static int32_t AddFiltersExecute(MediaLibraryCommand& cmd, std::shared_ptr<FileAsset>& fileAsset,
        const std::string &cachePath);
    static int32_t SubmitEditCacheExecute(MediaLibraryCommand &cmd,
        std::shared_ptr<FileAsset> &fileAsset, const std::string &cachePath, bool isWriteGpsAdvanced);
    static int32_t SubmitCacheExecute(MediaLibraryCommand &cmd,
        std::shared_ptr<FileAsset> &fileAsset, const std::string &cachePath);
    static int32_t ReNameVedioFilePath(
        const std::string &assetPath, const std::string &sourceImagePath, const std::string &assetVideoPath);
    static int32_t SubmitEffectModeExecute(MediaLibraryCommand &cmd);
    static int32_t SubmitEditMovingPhotoExecute(MediaLibraryCommand &cmd, const std::shared_ptr<FileAsset> &fileAsset);
    static int32_t GetMovingPhotoCachePath(MediaLibraryCommand &cmd, const std::shared_ptr<FileAsset> &fileAsset,
        std::string &imageCachePath, std::string &videoCachePath);
    static bool CheckCacheCmd(MediaLibraryCommand &cmd, int32_t subtype, const std::string &displayName);
    static int32_t MoveCacheFile(MediaLibraryCommand &cmd, int32_t subtype,
        const std::string &cachePath, const std::string &destPath);
    static int32_t UpdateMovingPhotoSubtype(int32_t fileId, int32_t currentPhotoSubType);
    static int32_t UpdateFileAsset(MediaLibraryCommand &cmd);
    static int32_t HandleNeedSetDisplayName(MediaLibraryCommand &cmd,
        std::shared_ptr<FileAsset> &fileAsset, bool &isNeedScan);
    static int32_t HandleNeedSetDisplayName(MediaLibraryCommand &cmd, std::shared_ptr<FileAsset> &fileAsset);
    static int32_t RenameEditDataDirBySetDisplayName(MediaLibraryCommand &cmd, std::shared_ptr<FileAsset> &fileAsset);
    static int32_t HandleCacheFile(MediaLibraryCommand& cmd, std::string cachePath);
    static int32_t HandleCacheFile(MediaLibraryCommand &cmd, int32_t id);
    static int32_t UpdateOrientationAllExif(MediaLibraryCommand &cmd, const std::shared_ptr<FileAsset> &fileAsset,
        std::string &currentOrientation);
    static int32_t UpdateOrientationExif(MediaLibraryCommand &cmd, const std::shared_ptr<FileAsset> &fileAsset,
        bool &orientationUpdated, std::string &currentOrientation);
    static int32_t BatchSetUserComment(MediaLibraryCommand &cmd);
    static int32_t BatchSetOwnerAlbumId(MediaLibraryCommand &cmd);
    static int32_t BatchSetRecentShow(MediaLibraryCommand &cmd);
    static int32_t AddFiltersToPhoto(const std::string &inputPath, const std::string &outputPath,
        const std::string &editdata, const std::string &photoStatus = "");
    static int32_t RevertToOriginalEffectMode(MediaLibraryCommand &cmd, const std::shared_ptr<FileAsset> &fileAsset,
        bool &isNeedScan);
    static bool IsNeedRevertEffectMode(MediaLibraryCommand& cmd, const std::shared_ptr<FileAsset>& fileAsset,
        int32_t& effectMode);
    static void ProcessEditedEffectMode(MediaLibraryCommand& cmd);
    static int32_t SaveCameraPhoto(MediaLibraryCommand &cmd);
    static std::shared_ptr<FileAsset> GetFileAsset(MediaLibraryCommand &cmd);
    static int32_t ForceSavePicture(MediaLibraryCommand& cmd);
    static int32_t SetVideoEnhancementAttr(MediaLibraryCommand &cmd);
    static int32_t DegenerateMovingPhoto(MediaLibraryCommand &cmd);
    static int32_t UpdateOwnerAlbumId(MediaLibraryCommand &cmd);
    static int32_t ProcessMovingPhotoOprnKey(MediaLibraryCommand &cmd, std::shared_ptr<FileAsset>& fileAsset,
        const std::string& id, bool& isMovingPhotoVideo);
    static int32_t GetTakeEffect(std::shared_ptr<Media::Picture> &picture, std::string &photoId);
    static int32_t DoRevertAfterAddFiltersFailed(const std::shared_ptr<FileAsset> &fileAsset,
        const std::string &path, const std::string &sourcePath);
private:
    static void UpdateEditDataPath(std::string filePath, const std::string &extension);
    static void DeleteAbnormalFile(std::string &assetPath, const int32_t &fileId, const std::string &oldFilePath);
    static std::mutex saveCameraPhotoMutex_;
    static std::condition_variable condition_;
    static std::string lastPhotoId_;
};

class PhotoEditingRecord {
public:
    EXPORT explicit PhotoEditingRecord();
    EXPORT static std::shared_ptr<PhotoEditingRecord> GetInstance();

    EXPORT bool StartCommitEdit(int32_t fileId);
    EXPORT void EndCommitEdit(int32_t fileId);
    EXPORT bool StartRevert(int32_t fileId);
    EXPORT void EndRevert(int32_t fileId);
    EXPORT bool IsInRevertOperation(int32_t fileId);
    EXPORT bool IsInEditOperation(int32_t fileId);

private:
    static std::mutex mutex_;
    static std::shared_ptr<PhotoEditingRecord> instance_;
    std::shared_mutex addMutex_;
    std::set<int32_t> editingPhotoSet_;
    std::set<int32_t> revertingPhotoSet_;
};
} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIALIBRARY_PHOTO_OPERATIONS_H
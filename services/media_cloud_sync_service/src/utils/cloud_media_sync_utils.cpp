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

#define MLOG_TAG "Media_Cloud_Utils"

#include "cloud_media_sync_utils.h"

#include <algorithm>
#include <string>
#include <sys/time.h>
#include <utime.h>

#include "cloud_media_file_utils.h"
#include "exif_rotate_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "cloud_media_sync_const.h"
#include "photo_file_utils.h"
#include "thumbnail_const.h"
#include "moving_photo_file_utils.h"
#include "cloud_media_uri_utils.h"
#include "media_edit_utils.h"
#include "media_path_utils.h"
#include "media_string_utils.h"
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
#include "enhancement_manager.h"
#endif

using namespace std;

namespace OHOS::Media::CloudSync {
static const string HMDFS_PATH_PREFIX = "/mnt/hmdfs/100";
static const string PHOTOS_PATH = "com.huawei.hmos.photos";

int32_t CloudMediaSyncUtils::FillPhotosDto(
    CloudSync::PhotosDto &photosDto, const std::string &path, const int32_t &orientation,
    const int32_t exifRotate, const int32_t &thumbState)
{
    MEDIA_DEBUG_LOG("FillPhotosDto enter %{public}s", path.c_str());
    bool isRotation = orientation != ROTATE_ANGLE_0 || exifRotate > static_cast<int32_t>(ExifRotateType::TOP_LEFT);
    std::string thumbSuffix = isRotation ? THUMBNAIL_THUMB_EX_SUFFIX : THUMBNAIL_THUMB_SUFFIX;
    std::string lcdSuffix = isRotation ? THUMBNAIL_LCD_EX_SUFFIX : THUMBNAIL_LCD_SUFFIX;

    std::string thumbLocalPath = GetThumbnailPath(path, thumbSuffix);
    std::string lcdLocalPath = GetThumbnailPath(path, lcdSuffix);

    /**
     * oritentaion = 0 fieldKey = thumbnail/lcd  attachmentPath = /storage/cloud/files/.thumbs/Photo/1/IMGxxx.jpg
     * oritentaion != 0 fieldKey = thumbnail  attachmentPath = /storage/cloud/files/.thumbs/Photo/1/IMGxxx.jpg/THM_EX
     * oritentaion != 0 fieldKey = lcd  attachmentPath = /storage/cloud/files/.thumbs/Photo/1/IMGxxx.jpg/LCD_EX
     */
    CloudSync::CloudFileDataDto dtoThm;
    CloudMediaFileUtils::GetParentPathAndFilename(thumbLocalPath, dtoThm.path, dtoThm.fileName);
    size_t thumbFileSize = 0;
    CloudMediaFileUtils::GetFileSizeV2(thumbLocalPath, thumbFileSize);
    dtoThm.size = static_cast<int64_t>(thumbFileSize);
    photosDto.attachment["thumbnail"] = dtoThm;

    CloudSync::CloudFileDataDto dtoLcd;
    CloudMediaFileUtils::GetParentPathAndFilename(lcdLocalPath, dtoLcd.path, dtoLcd.fileName);
    size_t lcdFileSize = 0;
    CloudMediaFileUtils::GetFileSizeV2(lcdLocalPath, lcdFileSize);
    dtoLcd.size = static_cast<int64_t>(lcdFileSize);
    photosDto.attachment["lcd"] = dtoLcd;
    MEDIA_DEBUG_LOG("FillPhotosDto end %{public}s", path.c_str());
    return E_OK;
}

int32_t CloudMediaSyncUtils::FillPhotosDto(
    CloudSync::PhotosDto &photosDto, const CloudSync::CloudMediaPullDataDto &data,
    const NativeRdb::ValuesBucket &values)
{
    constexpr uint64_t DEFAULT_SIZE = 2 * 1024 * 1024; // thumbnail and lcd default size is 2MB
    bool isRotation = data.propertiesRotate != ROTATE_ANGLE_0 ||
        data.exifRotate > static_cast<int32_t>(ExifRotateType::TOP_LEFT);
    std::string thumbSuffix = isRotation ? THUMBNAIL_THUMB_EX_SUFFIX : THUMBNAIL_THUMB_SUFFIX;
    std::string lcdSuffix = isRotation ? THUMBNAIL_LCD_EX_SUFFIX : THUMBNAIL_LCD_SUFFIX;

    std::string thumbLocalPath = GetThumbnailPath(data.localPath, thumbSuffix);
    std::string lcdLocalPath = GetThumbnailPath(data.localPath, lcdSuffix);

    CloudSync::CloudFileDataDto dtoThm;
    CloudMediaFileUtils::GetParentPathAndFilename(thumbLocalPath, dtoThm.path, dtoThm.fileName);
    bool isValid = data.thmSize != 0 && data.lcdSize != 0;
    CHECK_AND_PRINT_LOG(isValid, "invalid size, thmSize: %{public}" PRId64", lcdSize: %{public}" PRId64,
        data.thmSize, data.lcdSize);
    dtoThm.size = (data.thmSize <= 0) ? DEFAULT_SIZE : data.thmSize;
    photosDto.attachment["thumbnail"] = dtoThm;

    CloudSync::CloudFileDataDto dtoLcd;
    CloudMediaFileUtils::GetParentPathAndFilename(lcdLocalPath, dtoLcd.path, dtoLcd.fileName);
    dtoLcd.size = (data.lcdSize <= 0) ? DEFAULT_SIZE : data.lcdSize;
    photosDto.attachment["lcd"] = dtoLcd;

    NativeRdb::ValueObject valueObject;
    if (values.GetObject(PhotoColumn::UNIQUE_ID, valueObject)) {
        valueObject.GetString(photosDto.uniqueId);
    }
    if (values.GetObject(MediaColumn::MEDIA_PACKAGE_NAME, valueObject)) {
        valueObject.GetString(photosDto.packageName);
    }
    if (values.GetObject(PhotoColumn::PHOTO_RISK_STATUS, valueObject)) {
        valueObject.GetInt(photosDto.photoRiskStatus);
    }
    if (values.GetObject(PhotoColumn::PHOTO_IS_CRITICAL, valueObject)) {
        valueObject.GetInt(photosDto.isCritical);
    } else {
        MEDIA_WARN_LOG("Failed to get PHOTO_IS_CRITICAL from values");
        photosDto.isCritical = 0; // Set default value
    }

    return E_OK;
}

bool CloudMediaSyncUtils::IsLocalDirty(int32_t dirty, bool isDelete)
{
    bool localDirty = (dirty == static_cast<int32_t>(DirtyType::TYPE_MDIRTY)) ||
                      (dirty == static_cast<int32_t>(DirtyType::TYPE_FDIRTY));
    CHECK_AND_PRINT_INFO_LOG(!localDirty, "dirty: %{public}d, isDelete: %{public}d",
        dirty, static_cast<int32_t>(isDelete));
    if (isDelete) {
        return localDirty;
    } else {
        return localDirty || (dirty == static_cast<int32_t>(DirtyType::TYPE_DELETED));
    }
}

bool CloudMediaSyncUtils::FileIsLocal(const int32_t position)
{
    MEDIA_DEBUG_LOG("position: %{public}d.", position);
    return !!(static_cast<uint32_t>(position) & 1);
}

std::string CloudMediaSyncUtils::GetThumbParentPath(const std::string &path, const std::string &prefixCloud)
{
    size_t pos = path.find(prefixCloud);
    if (pos == std::string::npos) {
        MEDIA_ERR_LOG("invalid path %{public}s", MediaFileUtils::DesensitizePath(path).c_str());
        return "";
    }
    /* transform sandbox path to hmdfs local path */
    return "/storage/cloud/files/.thumbs/" + path.substr(pos + prefixCloud.size());
}

void CloudMediaSyncUtils::RemoveThmParentPath(const std::string &path, const std::string &prefixCloud)
{
    std::string thmPath = GetThumbParentPath(path, prefixCloud);
    MEDIA_INFO_LOG("filePath: %{public}s, thmbFileParentDir: %{public}s",
        MediaFileUtils::DesensitizePath(path).c_str(),
        MediaFileUtils::DesensitizePath(thmPath).c_str());
    MediaFileUtils::DeleteDir(thmPath);
}

void CloudMediaSyncUtils::RemoveEditDataParentPath(const std::string &path)
{
    std::string editParentPath = MediaEditUtils::GetEditDataDir(path);

    MEDIA_INFO_LOG("filePath: %{public}s, editDataParentDir: %{public}s",
        MediaFileUtils::DesensitizePath(path).c_str(),
        MediaFileUtils::DesensitizePath(editParentPath).c_str());
    MediaFileUtils::DeleteDir(editParentPath);
}

void CloudMediaSyncUtils::RemoveMetaDataPath(const std::string &path)
{
    std::string metaDataPath = PhotoFileUtils::GetMetaDataRealPath(path);

    MEDIA_INFO_LOG("filePath: %{public}s, metaDataPath: %{public}s",
        MediaFileUtils::DesensitizePath(path).c_str(),
        MediaFileUtils::DesensitizePath(metaDataPath).c_str());
    MediaFileUtils::DeleteFile(metaDataPath.c_str());
}

static std::string GetVideoCachePath(const std::string &filePath)
{
    std::string result = "";
    const std::string sandboxPrefix = "/storage/cloud";
    const std::string cachePathPrefix = "/account/device_view/local/data/";
    const std::string cachepathSuffix = "/.video_cache";
    size_t pos = filePath.find(sandboxPrefix);
    if (pos != 0 || pos == std::string::npos) {
        MEDIA_ERR_LOG(
            "GetVideoCachePath Invalid filePath, path: %{public}s", MediaFileUtils::DesensitizePath(filePath).c_str());
        return result;
    }
    std::string cachePath = HMDFS_PATH_PREFIX + cachePathPrefix + PHOTOS_PATH +
        cachepathSuffix + filePath.substr(sandboxPrefix.length());
    MEDIA_INFO_LOG("The cachePath is: %{public}s", cachePath.c_str());
    auto resolvedPath = realpath(cachePath.c_str(), nullptr);
    if (resolvedPath == nullptr) {
        if (errno != ENOENT) {
            MEDIA_ERR_LOG("GetVideoCachePath realpath failed, path: %{public}s",
                MediaFileUtils::DesensitizePath(filePath).c_str());
        }
        return result;
    }
    if (strncmp(resolvedPath, cachePath.c_str(), cachePath.size()) != 0) {
        MEDIA_ERR_LOG("GetVideoCachePath Invalid videoCachePath, path: %{public}s",
            MediaFileUtils::DesensitizePath(cachePath).c_str());
        free(resolvedPath);
        return result;
    }
    free(resolvedPath);
    return cachePath;
}

void CloudMediaSyncUtils::InvalidVideoCache(const std::string &localPath)
{
    MEDIA_INFO_LOG("InvalidVideoCache loca path: %{public}s", MediaFileUtils::DesensitizePath(localPath).c_str());
    const std::string sandboxPrefix = "/storage/cloud";
    size_t pos = localPath.find(sandboxPrefix);
    CHECK_AND_RETURN_LOG(pos == 0 && pos != std::string::npos,
        "InvalidVideoCache Invalid localPath, sandboxPrefix: %{public}s",
        sandboxPrefix.c_str());
    std::string videoCachePath = GetVideoCachePath(localPath);
    CHECK_AND_RETURN_LOG(!videoCachePath.empty(), "InvalidVideoCache Invalid videoCachePath");
    CHECK_AND_RETURN_LOG(unlink(videoCachePath.c_str()) >= 0,
        "InvalidVideoCache Failed to unlink video cache: %{public}s, errno: %{public}d",
        MediaFileUtils::DesensitizePath(videoCachePath).c_str(),
        errno);
    MEDIA_INFO_LOG(
        "InvalidVideoCache VideoCachePath: %{public}s", MediaFileUtils::DesensitizePath(videoCachePath).c_str());
}

std::string CloudMediaSyncUtils::GetMovingPhotoExtraDataDir(const std::string &localPath)
{
    if (localPath.length() < ROOT_MEDIA_DIR.length() || !MediaStringUtils::StartsWith(localPath, ROOT_MEDIA_DIR)) {
        return "";
    }
    return MEDIA_EDIT_DATA_DIR + localPath.substr(ROOT_MEDIA_DIR.length());
}

std::string CloudMediaSyncUtils::GetMovingPhotoExtraDataPath(const std::string &localPath)
{
    std::string parentPath = GetMovingPhotoExtraDataDir(localPath);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/extraData";
}

std::string CloudMediaSyncUtils::GetSourceMovingPhotoImagePath(const string& imagePath)
{
    return MediaEditUtils::GetEditDataSourcePath(imagePath);
}

std::string CloudMediaSyncUtils::GetSourceMovingPhotoVideoPath(const string& imagePath)
{
    return GetMovingPhotoVideoPath(GetSourceMovingPhotoImagePath(imagePath));
}

std::string CloudMediaSyncUtils::GetMovingPhotoVideoPath(const std::string &localPath)
{
    size_t splitIndex = localPath.find_last_of('.');
    size_t lastSlashIndex = localPath.find_last_of('/');
    if (splitIndex == std::string::npos || (lastSlashIndex != std::string::npos && lastSlashIndex > splitIndex)) {
        return "";
    }
    return localPath.substr(0, splitIndex) + ".mp4";
}

void CloudMediaSyncUtils::BackUpEditDataSourcePath(const std::string &localPath)
{
std::string editDataSourcePath = MediaEditUtils::GetEditDataSourcePath(localPath);
    std::string editDataTempPath = MediaEditUtils::GetEditDataTempPath(localPath);
    if (MediaFileUtils::IsFileExists(editDataSourcePath)) {
        if (!MediaFileUtils::MoveFile(editDataSourcePath, editDataTempPath)) {
            MEDIA_ERR_LOG("MoveFile failed. Fail to back source file of photo, errno:%{public}d", errno);
        }
    } else {
        if (!MediaFileUtils::CopyFileUtil(localPath, editDataTempPath)) {
            MEDIA_ERR_LOG("CopyFileUtil failed. Fail to back source file of photo, errno:%{public}d", errno);
        }
    }
}

void CloudMediaSyncUtils::RemoveEditDataSourcePath(const std::string &localPath)
{
    std::string editDataSourcePath = MediaEditUtils::GetEditDataSourcePath(localPath);
    MEDIA_INFO_LOG("RemoveEditDataSourcePath EditDataSourcePath: %{public}s", editDataSourcePath.c_str());
    if (unlink(editDataSourcePath.c_str()) != 0 && errno != ENOENT) {
        MEDIA_ERR_LOG("unlink editDataSource failed, errno %{public}d", errno);
    }
}

void CloudMediaSyncUtils::RemoveEditDataPath(const std::string &localPath)
{
    std::string editDataPath = MediaEditUtils::GetEditDataPath(localPath);
    MEDIA_INFO_LOG("RemoveEditDataPath EditDataPath: %{public}s", editDataPath.c_str());
    if (unlink(editDataPath.c_str()) != 0 && errno != ENOENT) {
        MEDIA_ERR_LOG("unlink editData failed, errno %{public}d", errno);
    }
}

void CloudMediaSyncUtils::RemoveTransCodePath(const std::string &localPath)
{
    std::string transCodePath = MediaEditUtils::GetTransCodePath(localPath);
    MEDIA_INFO_LOG("RemoveTransCodePath TransCodePath: %{public}s", transCodePath.c_str());
    if (unlink(transCodePath.c_str()) != 0 && errno != ENOENT) {
        MEDIA_ERR_LOG("[CloudMedia] unlink transCode failed, errno %{public}d", errno);
    } else {
        MEDIA_INFO_LOG("[CloudMedia] Delete transCode file Success!");
    }
}

void CloudMediaSyncUtils::RemoveMovingPhoto(const PhotosPo &photoInfo)
{
    CHECK_AND_RETURN_LOG(IsMovingPhoto(photoInfo), "RemoveMovingPhoto Is not MovingPhoto");
    const std::string cloudPath = photoInfo.data.value_or("");
    const std::string videoPath = GetMovingPhotoVideoPath(cloudPath);
    MEDIA_INFO_LOG("RemoveMovingPhoto MovingPhotoVideoPath: %{public}s", videoPath.c_str());
    if (unlink(videoPath.c_str()) != 0 && errno != ENOENT) {
        MEDIA_ERR_LOG("unlink moving photo's video failed, errno %{public}d, cloudPath: %{public}s",
            errno, videoPath.c_str());
    }
    const std::string extraDataPath = GetMovingPhotoExtraDataPath(cloudPath);
    MEDIA_INFO_LOG("RemoveMovingPhoto ExtraDataPath: %{public}s", extraDataPath.c_str());
    if (unlink(extraDataPath.c_str()) != 0 && errno != ENOENT) {
        MEDIA_ERR_LOG("unlink moving photo's video failed, errno %{public}d, cloudPath: %{public}s",
            errno, extraDataPath.c_str());
    }
}

int32_t HandleHashCode(const std::string &str) __attribute__((no_sanitize("signed-integer-overflow")))
{
    int32_t hash = 0;
    for (uint32_t i = 0; i < str.length(); i++) {
        char c = str.at(i);
        hash = hash * CloudSync::HASH_VLAUE + c;
    }
    return hash;
}

uint32_t CloudMediaSyncUtils::GenerateCloudIdWithHash(CloudSync::PhotoAlbumPo &record)
{
    std::string cloudId = record.cloudId.value_or("");
    if (!cloudId.empty()) {
        MEDIA_INFO_LOG("cloudId is not empty, it is %{public}s", cloudId.c_str());
        return E_CLOUDID_IS_NOT_NULL;
    }
    std::string lpath = record.lpath.value_or("");
    int64_t dateAdded = record.dateAdded.value_or(0);
    if (dateAdded == 0) {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        dateAdded = tv.tv_sec * CloudSync::MILLISECOND_TO_SECOND + tv.tv_usec / CloudSync::MILLISECOND_TO_SECOND;
    }
    int32_t hashValue = HandleHashCode(lpath);
    cloudId = "default-album-200-" + to_string(hashValue) + "-" + to_string(dateAdded);
    MEDIA_INFO_LOG("lpath is %{public}s, cloudid is %{public}s", lpath.c_str(), cloudId.c_str());
    record.cloudId = cloudId;
    return E_OK;
}

std::string CloudMediaSyncUtils::GetLpathFromSourcePath(const std::string &sourcePath)
{
    size_t pos = sourcePath.find(SOURCE_PATH_PERFIX);
    if (pos == std::string::npos) {
        MEDIA_ERR_LOG("invalid path %{private}s", MediaFileUtils::DesensitizePath(sourcePath).c_str());
        return "";
    }

    size_t lpathStart = pos + SOURCE_PATH_PERFIX.length();
    size_t lpathEnd = sourcePath.rfind('/');
    if (lpathEnd == std::string::npos || lpathEnd <= lpathStart) {
        MEDIA_ERR_LOG("invalid path %{private}s", MediaFileUtils::DesensitizePath(sourcePath).c_str());
        return "";
    }
    return sourcePath.substr(lpathStart, lpathEnd - lpathStart);
}

std::string CloudMediaSyncUtils::GetLpath(const CloudSync::CloudMediaPullDataDto &pullData)
{
    CHECK_AND_RETURN_RET_LOG(!pullData.propertiesSourcePath.empty(), "", "PullData cannot find attributes::sourcePath");
    std::string sourcePath = pullData.propertiesSourcePath;
    size_t pos = sourcePath.find(SCREENSHOT_ALBUM_PATH);
    if (pos != std::string::npos) {
        int32_t fileType = pullData.basicFileType;
        if (fileType == -1) {
            MEDIA_ERR_LOG("Cannot find basic::fileType");
        }

        std::string displayName = pullData.basicFileName;
        if (displayName.empty()) {
            MEDIA_ERR_LOG("Cannot find basic::fileName");
        }

        if (fileType == CloudSync::FILE_TYPE_VIDEO) {
            sourcePath = SCREENRECORD_ALBUM_PATH + displayName;
        }
    }
    return GetLpathFromSourcePath(sourcePath);
}

std::string CloudMediaSyncUtils::GetLocalPath(const std::string &path)
{
    std::string localPath = path;
    size_t pos = localPath.find(PHOTO_CLOUD_PATH_URI);
    if (pos != std::string::npos) {
        localPath.replace(pos, PHOTO_CLOUD_PATH_URI.length(), PHOTO_MEDIA_PATH_URI);
    }
    return localPath;
}

std::string CloudMediaSyncUtils::RestoreCloudPath(const std::string &path)
{
    std::string cloudPath = path;
    size_t pos = cloudPath.find(PHOTO_MEDIA_PATH_URI);
    if (pos != std::string::npos) {
        cloudPath.replace(pos, PHOTO_MEDIA_PATH_URI.length(), PHOTO_CLOUD_PATH_URI);
    }
    return cloudPath;
}

bool CloudMediaSyncUtils::IsMovingPhoto(const PhotosPo &photosPo)
{
    int32_t subtype = photosPo.subtype.value_or(0);
    int32_t movingPhotoEffectMode = photosPo.movingPhotoEffectMode.value_or(0);
    int32_t originalSubtype = photosPo.originalSubtype.value_or(0);
    return MovingPhotoFileUtils::IsMovingPhoto(subtype, movingPhotoEffectMode, originalSubtype);
}

bool CloudMediaSyncUtils::IsGraffiti(const PhotosPo &photosPo)
{
    int32_t subtype = photosPo.subtype.value_or(0);
    int32_t originalSubtype = photosPo.originalSubtype.value_or(0);
    return MovingPhotoFileUtils::IsGraffiti(subtype, originalSubtype);
}

bool CloudMediaSyncUtils::IsLivePhoto(const PhotosPo &photosPo)
{
    std::string path = photosPo.data.value_or("");
    std::string localPath = GetLocalPath(path);
    return MovingPhotoFileUtils::IsLivePhoto(localPath);
}

bool CloudMediaSyncUtils::IsUserAlbumPath(const std::string &lpath)
{
    std::string prefix = "/pictures/users/";
    if (lpath.size() < prefix.size()) {
        return false;
    }
    return lpath.substr(0, prefix.size()) == prefix;
}

bool CloudMediaSyncUtils::CanUpdateExifRotateOnly(int32_t mediaType, int32_t oldExifRotate, int32_t newExifRotate)
{
    if (mediaType == static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE)) {
        return oldExifRotate == 0 && newExifRotate == static_cast<int32_t>(ExifRotateType::TOP_LEFT);
    } else {
        return !ExifRotateUtils::IsExifRotateWithFlip(newExifRotate);
    }
}

bool CloudMediaSyncUtils::IsCloudEnhancementSupported()
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    return true;
#else
    return false;
#endif
}

void CloudMediaSyncUtils::SyncDealWithCompositePhoto(const std::string &assetDataPath, int32_t photoId)
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    string photoCloudPath = CloudMediaSyncUtils::RestoreCloudPath(assetDataPath);
    if (MediaEditUtils::IsEditDataSourceBackExists(photoCloudPath)) {
        bool exchange = EnhancementManager::GetInstance().SyncCleanCompositePhoto(photoCloudPath);
        auto [compositeDisplayStatus, ceAvailable] =
            EnhancementManager::GetInstance().SyncDealWithCompositeDisplayStatus(photoId, photoCloudPath, exchange);
        int32_t updateRet = EnhancementManager::GetInstance().UpdateCompositeDisplayStatus(
            photoId, compositeDisplayStatus, ceAvailable);
        CHECK_AND_PRINT_LOG(updateRet == E_OK, "fail to update composite display status of fileId: %{public}d",
            photoId);
    } else {
        CHECK_AND_PRINT_LOG(EnhancementManager::GetInstance().SyncClearNormalPhoto(photoId),
            "fail to clear normal photo, fileId: %{public}d", photoId);
    }
#endif
}

/**
 * 获取资产的文件存储路径
 * - 媒体文件路径：/storage/media/local/files/Photo/${bucketId}/${fileName}
 * - 湖内文件路径：/storage/media/local/files/Docs/HO_DATA_EXT_MISC/${lPath}/${displayName}
 * - 文管文件路径：/storage/media/local/files/Docs/${lPath}/${displayName}
 * @return 文件存储路径
 *
 * 数据场景：
 * | fileSourceType  | storagePath          | position | hidden | dateTrashed | fileStoragePath |
 * |-----------------|----------------------|----------|--------|-------------|-----------------|
 * | MEDIA(0)        | ./HO_DATA_EXT_MISC/. | 2        | 0      | 0           | 无本地文件，预期 storagePath |
 * | MEDIA(0)        | ./HO_DATA_EXT_MISC/. | 2        | 0      | 1           | 无本地文件，预期 data |
 * | MEDIA(0)        | ./HO_DATA_EXT_MISC/. | 2        | 1      | 0           | 无本地文件，预期 data |
 * | MEDIA(0)        | ./HO_DATA_EXT_MISC/. | 2        | 1      | 1           | 无本地文件，预期 data |
 * | LAKE(3)         | ./HO_DATA_EXT_MISC/. | 1 or 3   | 0      | 0           | storagePath     |
 * | MEDIA(0)        | ./HO_DATA_EXT_MISC/. | 1 or 3   | 0      | 1           | data |
 * | MEDIA(0)        | ./HO_DATA_EXT_MISC/. | 1 or 3   | 1      | 0           | data |
 * | MEDIA(0)        | ./HO_DATA_EXT_MISC/. | 1 or 3   | 1      | 1           | data |
 * | FILE_MANAGER(1) | ./Docs/.             | 2        | 0      | 0           | 无本地文件，预期 storagePath |
 * | MEDIA(0), 根据 sourcePath 识别文管 | ""  | 2        | 0      | 1           | 无本地文件，预期 data |
 * | MEDIA(0), 根据 sourcePath 识别文管 | ""  | 2        | 1      | 0           | 无本地文件，预期 data |
 * | MEDIA(0), 根据 sourcePath 识别文管 | ""  | 2        | 1      | 1           | 无本地文件，预期 data |
 * | FILE_MANAGER(1) | ./Docs/.             | 1 or 3   | 0      | 0           | storagePath     |
 * | MEDIA(0), 根据 sourcePath 识别文管 | ""  | 1 or 3   | 0      | 1           | data |
 * | MEDIA(0), 根据 sourcePath 识别文管 | ""  | 1 or 3   | 1      | 0           | data |
 * | MEDIA(0), 根据 sourcePath 识别文管 | ""  | 1 or 3   | 1      | 1           | data |
 * | MEDIA(0)        | ""                   | 任意     | 0      | 0           | data            |
 * | MEDIA(0)        | ""                   | 任意     | 1      | 0           | data            |
 * | MEDIA(0)        | ""                   | 任意     | 0      | 1           | data            |
 * | MEDIA(0)        | ""                   | 任意     | 1      | 1           | data            |
 *
 * 差异说明：
 * 1、纯云湖内资产必须是 MEDIA(0)，通过 storagePath 识别湖内资产；
 * 2、纯云文管资产必须是 FILE_MANAGER(1)，通过 fileSourceType 识别文管资产；
 * 3、隐藏和回收站资产不区分湖内和文管，通过 hidden 和 dateTrashed 识别，且必须是 MEDIA(0)；
 * 4、湖内隐藏或回收站资产，虽然 fileSourceType 是 MEDIA(0)，但通过 storagePath 识别为湖内资产；
 * 5、文管隐藏或回收站资产，虽然 fileSourceType 是 MEDIA(0)，但通过 sourcePath 识别为文管资产；
 */
std::string CloudMediaSyncUtils::FindFileStoragePath(const PhotosPo &photoInfo)
{
    const int32_t fileSourceType = photoInfo.fileSourceType.value_or(0);
    const bool isHidden = photoInfo.hidden.value_or(0) == 1;
    const bool isTrashed = photoInfo.dateTrashed.value_or(0) != 0;
    const std::string data = photoInfo.data.value_or("");
    if (isHidden || isTrashed) {
        return CloudMediaSyncUtils::GetLocalPath(data);
    }

    const std::string storagePath = photoInfo.storagePath.value_or("");
    bool isLakeAsset = fileSourceType == static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE);
    isLakeAsset = isLakeAsset || MediaStringUtils::StartsWith(storagePath, LAKE_STORAGE_PATH_PREFIX);
    const bool isFileManagerAsset = fileSourceType == static_cast<int32_t>(FileSourceType::FILE_MANAGER);
    const bool isStoragePathValid = isLakeAsset || isFileManagerAsset;
    CHECK_AND_RETURN_RET(!isStoragePathValid, storagePath);

    return CloudMediaSyncUtils::GetLocalPath(data);
}

bool CloudMediaSyncUtils::IsFileManagerAlbumPath(const std::string &lpath)
{
    std::string fromDocsTarget = FROM_DOCS_KEYWORD;
    fromDocsTarget = MediaStringUtils::ToLower(fromDocsTarget);
    std::string lPathLower = MediaStringUtils::ToLower(lpath);
    return MediaStringUtils::StartsWith(lPathLower, fromDocsTarget);
}

bool CloudMediaSyncUtils::IsMediaFile(const std::string &filePath)
{
    return MediaStringUtils::StartsWith(filePath, CLOUD_STORAGE_PATH_PREFIX);
}
}  // namespace OHOS::Media::CloudSync
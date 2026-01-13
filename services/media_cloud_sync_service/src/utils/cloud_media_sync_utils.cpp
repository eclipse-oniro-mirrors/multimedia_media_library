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
#include "media_image_framework_utils.h"
#include "media_log.h"
#include "media_player_framework_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "cloud_media_sync_const.h"
#include "photo_file_utils.h"
#include "thumbnail_const.h"
#include "moving_photo_file_utils.h"
#include "cloud_media_uri_utils.h"
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
    CloudSync::PhotosDto &photosDto, const CloudSync::CloudMediaPullDataDto &data)
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

    return E_OK;
}

bool CloudMediaSyncUtils::IsLocalDirty(int32_t dirty, bool isDelete)
{
    MEDIA_INFO_LOG("dirty: %{public}d, isDelete: %{public}d", dirty, static_cast<int32_t>(isDelete));
    bool localDirty = (dirty == static_cast<int32_t>(DirtyType::TYPE_MDIRTY)) ||
                      (dirty == static_cast<int32_t>(DirtyType::TYPE_FDIRTY));
    if (isDelete) {
        return localDirty;
    } else {
        return localDirty || (dirty == static_cast<int32_t>(DirtyType::TYPE_DELETED));
    }
}

bool CloudMediaSyncUtils::FileIsLocal(const int32_t position)
{
    MEDIA_INFO_LOG("position: %{public}d.", position);
    return !!(static_cast<uint32_t>(position) & 1);
}

std::string CloudMediaSyncUtils::GetCloudPath(const std::string &path, const std::string &prefixCloud)
{
    std::string cloudPath;
    if (path.find(prefixCloud) == 0) {
        cloudPath = prefixCloud + path.substr(prefixCloud.size());
    } else {
        MEDIA_ERR_LOG("Failed to get cloud path: %{public}s", MediaFileUtils::DesensitizePath(path).c_str());
        cloudPath = "";
    }
    return cloudPath;
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
    std::string editParentPath = PhotoFileUtils::GetEditDataDir(path);

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
    if (localPath.length() < ROOT_MEDIA_DIR.length() || !MediaFileUtils::StartsWith(localPath, ROOT_MEDIA_DIR)) {
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

static bool CheckPhotoPath(const std::string &localPath)
{
    return localPath.length() >= ROOT_MEDIA_DIR.length() && MediaFileUtils::StartsWith(localPath, ROOT_MEDIA_DIR);
}

std::string CloudMediaSyncUtils::GetEditDataSourcePath(const string& photoPath)
{
    string parentPath = GetEditDataDir(photoPath);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/source." + MediaFileUtils::GetExtensionFromPath(photoPath);
}

std::string CloudMediaSyncUtils::GetSourceMovingPhotoImagePath(const string& imagePath)
{
    return GetEditDataSourcePath(imagePath);
}

std::string CloudMediaSyncUtils::GetSourceMovingPhotoVideoPath(const string& imagePath)
{
    return GetMovingPhotoVideoPath(GetSourceMovingPhotoImagePath(imagePath));
}

std::string CloudMediaSyncUtils::GetEditDataDir(const std::string &localPath)
{
    if (!CheckPhotoPath(localPath)) {
        return "";
    }

    return MEDIA_EDIT_DATA_DIR + localPath.substr(ROOT_MEDIA_DIR.length());
}

std::string CloudMediaSyncUtils::GetEditDataPath(const std::string &localPath)
{
    std::string parentPath = GetEditDataDir(localPath);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/editdata";
}

std::string CloudMediaSyncUtils::GetTransCodePath(const std::string &localPath)
{
    std::string parentPath = GetEditDataDir(localPath);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/transcode.jpg";
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

std::string CloudMediaSyncUtils::GetMovingPhotoTmpPath(const std::string &localPath)
{
    std::string tempDownloadParent = "/mnt/hmdfs/account/device_view/local/files/.cloud_cache/download_cache/";
    if (!CheckPhotoPath(localPath)) {
        return "";
    }
    return tempDownloadParent + localPath.substr(ROOT_MEDIA_DIR.length());
}

void CloudMediaSyncUtils::BackUpEditDataSourcePath(const std::string &localPath)
{
    std::string editDataSourcePath = MovingPhotoFileUtils::GetEditDataSourcePath(localPath);
    std::string editDataTempPath = MovingPhotoFileUtils::GetEditDataTempPath(localPath);
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
    std::string editDataSourcePath = GetEditDataSourcePath(localPath);
    MEDIA_INFO_LOG("RemoveEditDataSourcePath EditDataSourcePath: %{public}s", editDataSourcePath.c_str());
    if (unlink(editDataSourcePath.c_str()) != 0 && errno != ENOENT) {
        MEDIA_ERR_LOG("unlink editDataSource failed, errno %{public}d", errno);
    }
}

void CloudMediaSyncUtils::RemoveEditDataPath(const std::string &localPath)
{
    std::string editDataPath = GetEditDataPath(localPath);
    MEDIA_INFO_LOG("RemoveEditDataPath EditDataPath: %{public}s", editDataPath.c_str());
    if (unlink(editDataPath.c_str()) != 0 && errno != ENOENT) {
        MEDIA_ERR_LOG("unlink editData failed, errno %{public}d", errno);
    }
}

void CloudMediaSyncUtils::RemoveTransCodePath(const std::string &localPath)
{
    std::string transCodePath = GetTransCodePath(localPath);
    MEDIA_INFO_LOG("RemoveTransCodePath TransCodePath: %{public}s", transCodePath.c_str());
    if (unlink(transCodePath.c_str()) != 0 && errno != ENOENT) {
        MEDIA_ERR_LOG("[CloudMedia] unlink transCode failed, errno %{public}d", errno);
    } else {
        MEDIA_INFO_LOG("[CloudMedia] Delete transCode file Success!");
    }
}

void CloudMediaSyncUtils::RemoveMovingPhoto(const CloudMediaPullDataDto &pullData)
{
    CHECK_AND_RETURN_LOG(IsMovingPhoto(pullData), "RemoveMovingPhoto Is not MovingPhoto");
    MEDIA_INFO_LOG(
        "RemoveMovingPhoto MovingPhotoVideoPath: %{public}s", GetMovingPhotoVideoPath(pullData.localPath).c_str());
    if (unlink(GetMovingPhotoVideoPath(pullData.localPath).c_str()) != 0 && errno != ENOENT) {
        MEDIA_ERR_LOG("unlink moving photo's video failed, errno %{public}d", errno);
    }
    MEDIA_INFO_LOG(
        "RemoveMovingPhoto ExtraDataPath: %{public}s", GetMovingPhotoExtraDataPath(pullData.localPath).c_str());
    if (unlink(GetMovingPhotoExtraDataPath(pullData.localPath).c_str()) != 0 && errno != ENOENT) {
        MEDIA_ERR_LOG("unlink moving photo's video failed, errno %{public}d", errno);
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

bool CloudMediaSyncUtils::IsMovingPhoto(const CloudMediaPullDataDto &pullData)
{
    int32_t subtype = pullData.attributesSubtype;
    int32_t movingPhotoEffectMode = pullData.attributesMovingPhotoEffectMode;
    int32_t originalSubtype = pullData.attributesOriginalSubtype;
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

int32_t CloudMediaSyncUtils::UpdateModifyTime(const std::string &localPath, int64_t localMtime)
{
    struct timeval times[2];
    times[0].tv_sec = static_cast<time_t>(localMtime / MILLISECOND_TO_SECOND);
    times[0].tv_usec = static_cast<suseconds_t>((localMtime % MILLISECOND_TO_SECOND) * MILLISECOND_TO_SECOND);
    times[1] = times[0];
    if (utimes(localPath.c_str(), times) < 0) {
        MEDIA_ERR_LOG(
            "utime failed %{public}d, lPath: %{public}s", errno, MediaFileUtils::DesensitizePath(localPath).c_str());
        return errno;
    }
    return E_OK;
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
    if (PhotoFileUtils::IsEditDataSourceBackExists(photoCloudPath)) {
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
}  // namespace OHOS::Media::CloudSync

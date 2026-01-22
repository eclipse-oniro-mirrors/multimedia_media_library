/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Media_Client"

#include "cloud_media_client_utils.h"

#include <string>

#include "media_log.h"
#include "cloud_lake_utils.h"
#include "media_file_utils.h"
#include "medialibrary_errno.h"

namespace OHOS::Media::CloudSync {
// Anco FileSourceType
const int32_t MEDIA_HO_LAKE_CONST = 3;
const std::string PREFIX = "/data/service/el2/";
const std::string SUFFIX = "/hmdfs/account/files";
const std::string SANDBOXPREFIX = "/storage/cloud/files";
static const std::string HMDFS_PATH_PREFIX = "/mnt/hmdfs/100";
static const std::string PHOTOS_PATH = "com.huawei.hmos.photos";
const std::string PHOTO_CLOUD_PATH_URI = "/storage/cloud/files/";
const std::string PHOTO_MEDIA_PATH_URI = "/storage/media/local/files/";
std::string CloudMediaClientUtils::GetLowerPath(const std::string &path, int32_t userId)
{
    size_t pos = path.find(SANDBOXPREFIX);
    if (pos == std::string::npos) {
        MEDIA_ERR_LOG("invalid path");
        return "";
    }
    return PREFIX + std::to_string(userId) + SUFFIX + path.substr(pos + SANDBOXPREFIX.size());
}

int32_t CloudMediaClientUtils::GetLocalPathByPhotosVo(const CloudMdkRecordPhotosVo &photosVo, std::string &localPath,
    int32_t userId)
{
    if (photosVo.fileSourceType != MEDIA_HO_LAKE_CONST) {
        localPath = GetLowerPath(photosVo.data, userId);
    } else {
        localPath = CloudLakeUtils::GetAbsoluteLakePath(photosVo.storagePath, userId);
    }
    return E_OK;
}

std::string CloudMediaClientUtils::GetLocalPath(const std::string &path)
{
    std::string localPath = path;
    size_t pos = localPath.find(PHOTO_CLOUD_PATH_URI);
    if (pos != std::string::npos) {
        localPath.replace(pos, PHOTO_CLOUD_PATH_URI.length(), PHOTO_MEDIA_PATH_URI);
    }
    return localPath;
}

std::string CloudMediaClientUtils::FindLocalPathFromCloudPath(const std::string &path, int32_t userId)
{
    std::string localPath = GetLocalPath(path);
    return AppendUserId(localPath, userId);
}

std::string CloudMediaClientUtils::AppendUserId(const std::string &path, int32_t userId)
{
    CHECK_AND_RETURN_RET(userId >= 0, path);
    const std::string ROOT_MEDIA_CLOUD_DIR = "/storage/cloud/files/";
    const std::string ROOT_MEDIA_CLOUD_PREFIX = "/storage/cloud/";
    const std::string ROOT_MEDIA_CLOUD_MIDFIX = "/files/";
    const std::string ROOT_MEDIA_LOCAL_DIR = "/storage/media/local/files/";
    const std::string ROOT_MEDIA_LOCAL_PREFIX = "/storage/media/";
    const std::string ROOT_MEDIA_LOCAL_MIDFIX = "/local/files/";
    // pattern(source): /storage/cloud/files/Photo/${bucketId}/${fileName}.${suffix}
    // pattern(target): /storage/cloud/${userId}/files/Photo/${bucketId}/${fileName}.${suffix}
    if (MediaFileUtils::StartWith(path, ROOT_MEDIA_CLOUD_DIR)) {
        return ROOT_MEDIA_CLOUD_PREFIX + std::to_string(userId) + ROOT_MEDIA_CLOUD_MIDFIX +
               path.substr(ROOT_MEDIA_CLOUD_DIR.length());
    }
    // pattern(source): /storage/media/local/files/Photo/${bucketId}/${fileName}.${suffix}
    // pattern(target): /storage/media/${userId}/local/files/Photo/${bucketId}/${fileName}.${suffix}
    if (MediaFileUtils::StartWith(path, ROOT_MEDIA_LOCAL_DIR)) {
        return ROOT_MEDIA_LOCAL_PREFIX + std::to_string(userId) + ROOT_MEDIA_LOCAL_MIDFIX +
               path.substr(ROOT_MEDIA_LOCAL_DIR.length());
    }
    return path;
}

std::string CloudMediaClientUtils::GetVideoCachePath(const std::string &filePath)
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
    char resolvedPathBuf[PATH_MAX] = {0};
    auto resolvedPath = realpath(cachePath.c_str(), resolvedPathBuf);
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

void CloudMediaClientUtils::InvalidVideoCache(const std::string &localPath)
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
}  // namespace OHOS::Media::CloudSync
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

#define MLOG_TAG "MediaEditUtils"

#include "media_edit_utils.h"

#include "media_log.h"
#include "media_string_utils.h"
#include "media_path_utils.h"
#include "media_pure_file_utils.h"
#include "media_file_utils.h"

using namespace std;

namespace OHOS::Media {
std::string MediaEditUtils::GetEditDataDirForCloud(const string& photoPath, int32_t userId)
{
    if (!MediaPathUtils::CheckFilesPath(MediaPathType::CLOUD_FILES, photoPath, AdaptAccount::YES)) {
        return "";
    }
    std::string rootCloudFilesDir = MediaPathUtils::GetDir(MediaPathType::CLOUD_FILES, AdaptAccount::YES);
    std::string cloudEditDataDir = MediaPathUtils::GetDir(MediaPathType::CLOUD_EDIT_DATA, AdaptAccount::YES);
    return MediaPathUtils::AppendUserId(cloudEditDataDir, userId) + photoPath.substr(rootCloudFilesDir.length());
}

std::string MediaEditUtils::GetEditDataDirForLocal(const string& photoPath, int32_t userId)
{
    if (!MediaPathUtils::CheckFilesPath(MediaPathType::LOCAL_FILES, photoPath, AdaptAccount::YES)) {
        return "";
    }
    std::string rootMediaFilesDir = MediaPathUtils::GetDir(MediaPathType::LOCAL_FILES, AdaptAccount::YES);
    std::string cloudEditDataDir = MediaPathUtils::GetDir(MediaPathType::CLOUD_EDIT_DATA, AdaptAccount::YES);
    return MediaPathUtils::AppendUserId(cloudEditDataDir, userId) + photoPath.substr(rootMediaFilesDir.length());
}

std::string MediaEditUtils::GetCacheDirForCloud(const string& photoPath, int32_t userId)
{
    if (!MediaPathUtils::CheckFilesPath(MediaPathType::CLOUD_FILES, photoPath, AdaptAccount::YES)) {
        return "";
    }
    std::string rootCloudFilesDir = MediaPathUtils::GetDir(MediaPathType::CLOUD_FILES, AdaptAccount::YES);
    std::string cloudCacheDir = MediaPathUtils::GetDir(MediaPathType::CLOUD_CACHE, AdaptAccount::YES);
    return MediaPathUtils::AppendUserId(cloudCacheDir, userId) + photoPath.substr(rootCloudFilesDir.length());
}

std::string MediaEditUtils::GetCacheDirForLocal(const string& photoPath, int32_t userId)
{
    if (!MediaPathUtils::CheckFilesPath(MediaPathType::LOCAL_FILES, photoPath, AdaptAccount::YES)) {
        return "";
    }
    std::string rootMediaFilesDir = MediaPathUtils::GetDir(MediaPathType::LOCAL_FILES, AdaptAccount::YES);
    std::string cloudCacheDir = MediaPathUtils::GetDir(MediaPathType::CLOUD_CACHE, AdaptAccount::YES);
    return MediaPathUtils::AppendUserId(cloudCacheDir, userId) + photoPath.substr(rootMediaFilesDir.length());
}

string MediaEditUtils::GetEditDataPath(const string& photoPath, int32_t userId)
{
    string parentPath = GetEditDataDirForCloud(photoPath, userId);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/editdata";
}

string MediaEditUtils::GetEditDataCameraPath(const string& photoPath, int32_t userId)
{
    string parentPath = GetEditDataDirForCloud(photoPath, userId);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/editdata_camera";
}

std::string MediaEditUtils::GetTransCodePath(const std::string &photoPath, int32_t userId)
{
    std::string parentPath = GetEditDataDirForCloud(photoPath, userId);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/transcode.jpg";
}

string MediaEditUtils::GetEditDataSourcePath(const string& photoPath, int32_t userId)
{
    string parentPath = GetEditDataDirForCloud(photoPath, userId);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/source." + MediaPathUtils::GetExtension(photoPath);
}

string MediaEditUtils::GetEditDataSourceBackPath(const string& photoPath, int32_t userId)
{
    string parentPath = GetEditDataDirForCloud(photoPath, userId);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/source_back." + MediaPathUtils::GetExtension(photoPath);
}

string MediaEditUtils::GetEditDataTempPath(const string &photoPath, int32_t userId)
{
    string parentPath = GetEditDataDirForCloud(photoPath, userId);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/photo_temp." + MediaPathUtils::GetExtension(photoPath);
}

string MediaEditUtils::GetEditDataSourceTempPath(const string& photoPath, int32_t userId)
{
    string parentPath = GetEditDataDirForCloud(photoPath, userId);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/source_temp." + MediaPathUtils::GetExtension(photoPath);
}

bool MediaEditUtils::IsEditDataSourceBackExists(const std::string &photoPath, int32_t userId)
{
    string editDataSourceBackPath = GetEditDataSourceBackPath(photoPath);
    return MediaPureFileUtils::IsFileExists(editDataSourceBackPath);
}

bool MediaEditUtils::HasEditData(int64_t editTime)
{
    return editTime > 0;
}

string MediaEditUtils::GetEnhancementTempMovingPhotoVideoPath(const string& photoPath, int32_t userId)
{
    string parentPath = GetCacheDirForCloud(photoPath, userId);
    if (parentPath.empty()) {
        return "";
    }
    if (!MediaFileUtils::IsDirExists(parentPath) && !MediaPureFileUtils::CreateDirectory(parentPath)) {
        MEDIA_ERR_LOG("Cannot create dir %{private}s, errno %{public}d", parentPath.c_str(), errno);
        return "";
    }
    return parentPath + "/enhancement_temp_video.mp4";
}

string MediaEditUtils::GetEnhancementTempLivePhotoImagePath(const string& photoPath, int32_t userId)
{
    string parentPath = GetCacheDirForCloud(photoPath, userId);
    if (parentPath.empty()) {
        return "";
    }
    if (!MediaFileUtils::IsDirExists(parentPath) && !MediaPureFileUtils::CreateDirectory(parentPath)) {
        MEDIA_ERR_LOG("Cannot create dir %{private}s, errno %{public}d", parentPath.c_str(), errno);
        return "";
    }
    return parentPath + "/enhancement_temp_image." + MediaPathUtils::GetExtension(photoPath);
}

bool MediaEditUtils::CheckAndCreateEditDataDir(const string& photoPath)
{
    string editDataDirPath = MediaEditUtils::GetEditDataDirForCloud(photoPath);
    if (!MediaFileUtils::IsDirExists(editDataDirPath)) {
        if (!MediaPureFileUtils::CreateDirectory(editDataDirPath)) {
            MEDIA_ERR_LOG("Create edit data directory %{public}s failed", editDataDirPath.c_str());
            return false;
        }
    }
    return true;
}
} // namespace OHOS::Media
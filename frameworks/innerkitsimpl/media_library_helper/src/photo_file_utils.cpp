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

#define MLOG_TAG "PhotoFileUtils"

#include "photo_file_utils.h"

#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"

using namespace std;

namespace OHOS::Media {
string PhotoFileUtils::AppendUserId(const string& path, int32_t userId)
{
    if (userId < 0 || !MediaFileUtils::StartsWith(path, ROOT_MEDIA_DIR)) {
        return path;
    }

    return "/storage/cloud/" + to_string(userId) + "/files/" + path.substr(ROOT_MEDIA_DIR.length());
}

static bool CheckPhotoPath(const string& photoPath)
{
    return photoPath.length() >= ROOT_MEDIA_DIR.length() && MediaFileUtils::StartsWith(photoPath, ROOT_MEDIA_DIR);
}

string PhotoFileUtils::GetEditDataDir(const string& photoPath, int32_t userId)
{
    if (!CheckPhotoPath(photoPath)) {
        return "";
    }

    return AppendUserId(MEDIA_EDIT_DATA_DIR, userId) + photoPath.substr(ROOT_MEDIA_DIR.length());
}

string PhotoFileUtils::GetEditDataPath(const string& photoPath, int32_t userId)
{
    string parentPath = GetEditDataDir(photoPath, userId);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/editdata";
}

string PhotoFileUtils::GetEditDataCameraPath(const string& photoPath, int32_t userId)
{
    string parentPath = GetEditDataDir(photoPath, userId);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/editdata_camera";
}

string PhotoFileUtils::GetEditDataSourcePath(const string& photoPath, int32_t userId)
{
    string parentPath = GetEditDataDir(photoPath, userId);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/source." + MediaFileUtils::GetExtensionFromPath(photoPath);
}

bool PhotoFileUtils::HasEditData(int64_t editTime)
{
    return editTime > 0;
}

bool PhotoFileUtils::HasSource(bool hasEditDataCamera, int64_t editTime, int32_t effectMode)
{
    return hasEditDataCamera || editTime > 0 ||
           (effectMode > static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT) &&
               effectMode != static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY));
}

int32_t PhotoFileUtils::GetMetaPathFromOrignalPath(const std::string &srcPath, std::string &metaPath)
{
    if (srcPath.empty()) {
        MEDIA_ERR_LOG("getMetaPathFromOrignalPath: source file invalid!");
        return E_INVALID_PATH;
    }

    size_t pos = srcPath.find(META_RECOVERY_PHOTO_RELATIVE_PATH);
    if (pos == string::npos) {
    MEDIA_ERR_LOG("getMetaPathFromOrignalPath: source path is not a photo path");
    return E_INVALID_PATH;
    }

    metaPath = srcPath;
    metaPath.replace(pos, META_RECOVERY_PHOTO_RELATIVE_PATH.length(), META_RECOVERY_META_RELATIVE_PATH);
    metaPath += META_RECOVERY_META_FILE_SUFFIX;

    return E_OK;
}

string PhotoFileUtils::GetMetaDataRealPath(const string &photoPath, int32_t userId)
{
    string metaPath;
    int ret = GetMetaPathFromOrignalPath(photoPath, metaPath);
    if (ret != E_OK) {
        return "";
    }
    return AppendUserId(ROOT_MEDIA_DIR, userId) + metaPath.substr(ROOT_MEDIA_DIR.length());
}

string PhotoFileUtils::GetThumbDir(const string &photoPath, int32_t userId)
{
    if (!CheckPhotoPath(photoPath)) {
        return "";
    }
    return AppendUserId(ROOT_MEDIA_DIR, userId) + ".thumbs/" + photoPath.substr(ROOT_MEDIA_DIR.length());
}

string PhotoFileUtils::GetLCDPath(const string &photoPath, int32_t userId)
{
    string thumbDir = GetThumbDir(photoPath, userId);
    if (thumbDir.empty()) {
        return "";
    }
    return thumbDir + "/LCD.jpg";
}

string PhotoFileUtils::GetTHMPath(const string &photoPath, int32_t userId)
{
    string thumbDir = GetThumbDir(photoPath, userId);
    if (thumbDir.empty()) {
        return "";
    }
    return thumbDir + "/THM.jpg";
}

static bool IsLaterThan(const string &currentPath, const string &targetPath)
{
    int64_t targetDateModified = 0;
    if (!MediaFileUtils::GetDateModified(targetPath, targetDateModified)) {
        return false;
    }
    int64_t currentDateModified = 0;
    if (!MediaFileUtils::GetDateModified(currentPath, currentDateModified)) {
        return false;
    }
    return currentDateModified > targetDateModified;
}

bool PhotoFileUtils::IsThumbnailExists(const string &photoPath)
{
    if (photoPath.empty()) {
        return false;
    }

    string lcdPath = GetLCDPath(photoPath);
    string thmPath = GetTHMPath(photoPath);
    return MediaFileUtils::IsFileExists(lcdPath) || MediaFileUtils::IsFileExists(thmPath);
}

bool PhotoFileUtils::IsThumbnailLatest(const string &photoPath)
{
    if (photoPath.empty()) {
        return false;
    }

    string lcdPath = GetLCDPath(photoPath);
    if (!lcdPath.empty() && IsLaterThan(lcdPath, photoPath)) {
        MEDIA_DEBUG_LOG("lcd %{private}s is latest", lcdPath.c_str());
        return true;
    }

    string thmPath = GetTHMPath(photoPath);
    if (!thmPath.empty() && IsLaterThan(thmPath, photoPath)) {
        MEDIA_DEBUG_LOG("thm %{private}s is latest", thmPath.c_str());
        return true;
    }
    return false;
}
} // namespace OHOS::Media
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

#include "media_path_utils.h"

#include "media_log.h"
#include "medialibrary_errno.h"
#include "media_string_utils.h"
#include <sys/xattr.h>

namespace OHOS::Media {
const char DOT = '.';
const std::string EMPTY_STRING = "";
const std::string ROOT_MEDIA_DIR = "/storage/cloud/files/";
const std::string ROOT_MEDIA_LOCAL_DIR = "/storage/media/local/files/";

std::string MediaPathUtils::AppendUserId(const std::string& path, int32_t userId)
{
    if (userId < 0 || !MediaStringUtils::StartsWith(path, ROOT_MEDIA_DIR)) {
        return path;
    }

    return "/storage/cloud/" + std::to_string(userId) + "/files/" + path.substr(ROOT_MEDIA_DIR.length());
}

bool MediaPathUtils::CheckPhotoPath(const std::string& photoPath)
{
    return photoPath.length() >= ROOT_MEDIA_DIR.length() && MediaStringUtils::StartsWith(photoPath, ROOT_MEDIA_DIR);
}

std::string MediaPathUtils::GetFileName(const std::string &filePath)
{
    std::string fileName;

    if (!filePath.empty()) {
        size_t lastSlash = filePath.rfind('/');
        if (lastSlash != std::string::npos) {
            if (filePath.size() > (lastSlash + 1)) {
                fileName = filePath.substr(lastSlash + 1);
            }
        }
    }

    return fileName;
}

std::string MediaPathUtils::GetExtension(const std::string &path)
{
    size_t splitIndex = path.find_last_of(DOT);
    if (splitIndex == std::string::npos || splitIndex == 0) {
        return EMPTY_STRING;
    }
    std::string extension = path.substr(splitIndex + 1);
    if (!extension.empty()) {
        transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    }
    return extension;
}

bool MediaPathUtils::CheckIsCloudFile(const std::string &sandboxPath)
{
    constexpr size_t MAX_ATTR_NAME = 64;
    constexpr const char* CLOUD_LOCATION_ATTR = "user.cloud.location";
    char cloudvalue[MAX_ATTR_NAME] = {'\0'};
    auto valueLen = getxattr(sandboxPath.c_str(), CLOUD_LOCATION_ATTR, cloudvalue, MAX_ATTR_NAME);
    CHECK_AND_RETURN_RET_LOG(valueLen > 0, false, "failed to getxattr, sandboxPath: %{public}s", sandboxPath.c_str());
    constexpr const char FILE_POSITION_CLOUD = '2';
    return cloudvalue[0] = FILE_POSITION_CLOUD;
}

std::string MediaPathUtils::ConvertCloudPathToLocalPath(const std::string &cloudPath)
{
    CHECK_AND_RETURN_RET_LOG(!cloudPath.empty(), EMPTY_STRING, "cloudPath is empty");
    CHECK_AND_RETURN_RET_LOG(MediaStringUtils::StartsWith(cloudPath, ROOT_MEDIA_DIR), EMPTY_STRING,
        "cloudPath %{public}s not start with %{public}s", cloudPath.c_str(), ROOT_MEDIA_DIR.c_str());
    std::string localPath = cloudPath;
    localPath.replace(0, ROOT_MEDIA_DIR.length(), ROOT_MEDIA_LOCAL_DIR);
    return localPath;
}
} // namespace OHOS::Media
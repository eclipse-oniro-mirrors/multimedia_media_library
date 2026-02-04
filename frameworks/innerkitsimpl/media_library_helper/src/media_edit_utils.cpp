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

using namespace std;

namespace OHOS::Media {
const std::string ROOT_MEDIA_DIR = "/storage/cloud/files/";
const std::string MEDIA_EDIT_DATA_DIR = ROOT_MEDIA_DIR + ".editData/";

string MediaEditUtils::GetEditDataDir(const string& photoPath, int32_t userId)
{
    if (!MediaPathUtils::CheckPhotoPath(photoPath)) {
        return "";
    }

    return MediaPathUtils::AppendUserId(MEDIA_EDIT_DATA_DIR, userId) + photoPath.substr(ROOT_MEDIA_DIR.length());
}

string MediaEditUtils::GetEditDataPath(const string& photoPath, int32_t userId)
{
    string parentPath = GetEditDataDir(photoPath, userId);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/editdata";
}

string MediaEditUtils::GetEditDataCameraPath(const string& photoPath, int32_t userId)
{
    string parentPath = GetEditDataDir(photoPath, userId);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/editdata_camera";
}

std::string MediaEditUtils::GetTransCodePath(const std::string &photoPath, int32_t userId)
{
    std::string parentPath = GetEditDataDir(photoPath, userId);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/transcode.jpg";
}

string MediaEditUtils::GetEditDataSourcePath(const string& photoPath, int32_t userId)
{
    string parentPath = GetEditDataDir(photoPath, userId);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/source." + MediaPathUtils::GetExtension(photoPath);
}

string MediaEditUtils::GetEditDataSourceBackPath(const string& photoPath, int32_t userId)
{
    string parentPath = GetEditDataDir(photoPath, userId);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/source_back." + MediaPathUtils::GetExtension(photoPath);
}

string MediaEditUtils::GetEditDataTempPath(const string &photoPath, int32_t userId)
{
    string parentPath = GetEditDataDir(photoPath, userId);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/photo_temp." + MediaPathUtils::GetExtension(photoPath);
}

string MediaEditUtils::GetEditDataSourceTempPath(const string& photoPath, int32_t userId)
{
    string parentPath = GetEditDataDir(photoPath, userId);
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
} // namespace OHOS::Media
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

#include <ctime>
#include <iomanip>
#include <sstream>

#include "media_column.h"
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

string PhotoFileUtils::GetTransCodePath(const string& photoPath, int32_t userId)
{
    string parentPath = GetEditDataDir(photoPath, userId);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/transcode.jpg";
}

string PhotoFileUtils::GetEditDataSourcePath(const string& photoPath, int32_t userId)
{
    string parentPath = GetEditDataDir(photoPath, userId);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/source." + MediaFileUtils::GetExtensionFromPath(photoPath);
}

string PhotoFileUtils::GetEditDataSourceBackPath(const string& photoPath, int32_t userId)
{
    string parentPath = GetEditDataDir(photoPath, userId);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/source_back." + MediaFileUtils::GetExtensionFromPath(photoPath);
}

string PhotoFileUtils::GetEditDataTempPath(const string &photoPath, int32_t userId)
{
    string parentPath = GetEditDataDir(photoPath, userId);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/photo_temp." + MediaFileUtils::GetExtensionFromPath(photoPath);
}

string PhotoFileUtils::GetEditDataSourceTempPath(const string& photoPath, int32_t userId)
{
    string parentPath = GetEditDataDir(photoPath, userId);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/source_temp." + MediaFileUtils::GetExtensionFromPath(photoPath);
}

bool PhotoFileUtils::IsEditDataSourceBackExists(const std::string &photoPath, int32_t userId)
{
    string editDataSourceBackPath = GetEditDataSourceBackPath(photoPath);
    return MediaFileUtils::IsFileExists(editDataSourceBackPath);
}

bool PhotoFileUtils::HasEditData(int64_t editTime)
{
    return editTime > 0;
}

bool PhotoFileUtils::HasSource(bool hasEditDataCamera, int64_t editTime, int32_t effectMode, int32_t subtype)
{
    return hasEditDataCamera || editTime > 0 ||
            (subtype == static_cast<int32_t>(PhotoSubType::CINEMATIC_VIDEO)) ||
            (effectMode > static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT) &&
                effectMode != static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY));
}

std::string PhotoFileUtils::GetAbsoluteLakeDir(int32_t userId)
{
    return "/mnt/data/" + to_string(userId) + "/HO_MEDIA/";
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

std::tuple<int64_t, std::string> PhotoFileUtils::ExtractTimeInfo(
    const std::string &timeStr, const std::string &format, bool isUTC)
{
    if (timeStr.empty()) {
        MEDIA_ERR_LOG("Failed to parse time, timeStr is empty");
        return {0, ""};
    }

    std::tm timeInfo{};
    std::istringstream iss(timeStr);
    iss >> std::get_time(&timeInfo, format.c_str());
    if (iss.fail()) {
        MEDIA_ERR_LOG("Convert failed, timeStr:%{public}s", timeStr.c_str());
        return {0, ""};
    }

    time_t timeStamp = 0;
    if (isUTC) {
        timeInfo.tm_isdst = 0;
        timeStamp = timegm(&timeInfo);
    } else {
        timeInfo.tm_isdst = -1;
        timeStamp = std::mktime(&timeInfo);
    }

    if (timeStamp <= 0) {
        MEDIA_ERR_LOG("Failed to convert time, timeStr: %{public}s", timeStr.c_str());
        return {0, ""};
    }

    std::ostringstream oss;
    oss << std::put_time(&timeInfo, PhotoColumn::PHOTO_DETAIL_TIME_FORMAT.c_str());
    return {static_cast<int64_t>(timeStamp) * SEC_TO_MSEC, oss.str()};
}

std::tuple<std::string, std::string, std::string> PhotoFileUtils::ExtractYearMonthDay(const std::string &detailTime)
{
    const size_t detailTimeLength = 19;
    if (detailTime.length() != detailTimeLength) {
        MEDIA_ERR_LOG("invalid length, detailTime: %{public}s", detailTime.c_str());
        return std::make_tuple("", "", "");
    }

    const size_t yearDelimiter = 4;
    const size_t monthDelimiter = 7;
    const size_t dateDelimiter = 10;
    if (detailTime[yearDelimiter] != ':' || detailTime[monthDelimiter] != ':' || detailTime[dateDelimiter] != ' ') {
        MEDIA_ERR_LOG("invalid delimiter, detailTime: %{public}s", detailTime.c_str());
        return std::make_tuple("", "", "");
    }
    const size_t yearIndex = 0;
    const size_t yearCount = 4;
    std::string year = detailTime.substr(yearIndex, yearCount);
    const size_t monthIndex = 5;
    const size_t monthCount = 2;
    std::string month = detailTime.substr(monthIndex, monthCount);
    const size_t dayIndex = 8;
    const size_t dayCount = 2;
    std::string day = detailTime.substr(dayIndex, dayCount);
    if (!all_of(year.begin(), year.end(), ::isdigit) || !all_of(month.begin(), month.end(), ::isdigit) ||
        !all_of(day.begin(), day.end(), ::isdigit)) {
        MEDIA_ERR_LOG("invalid year month day, detailTime: %{public}s", detailTime.c_str());
        return std::make_tuple("", "", "");
    }

    return std::make_tuple(year, year + month, year + month + day);
}

int64_t PhotoFileUtils::NormalizeTimestamp(int64_t timestamp, int64_t fallbackValue)
{
    if (timestamp > MAX_MILSEC_TIMESTAMP) {
        MEDIA_ERR_LOG("timestamp: %{public}lld, precision is incorrect", static_cast<long long>(timestamp));
        while (timestamp > MAX_MILSEC_TIMESTAMP) {
            timestamp /= TIMESTAMP_CONVERSION_FACTOR;
        }
    }
    if (timestamp < MIN_MILSEC_TIMESTAMP) {
        MEDIA_ERR_LOG("invalid timestamp: %{public}lld", static_cast<long long>(timestamp));
        return fallbackValue;
    }
    return timestamp;
}

// 提供给端云使用
string PhotoFileUtils::GetAbsoluteLakePath(const std::string &storagePath, int32_t userId)
{
    string lakeDir = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/";
    if (!MediaFileUtils::StartsWith(storagePath, lakeDir)) {
        MEDIA_INFO_LOG("Failed to check storagePath: %{public}s", storagePath.c_str());
        return "";
    }

    return GetAbsoluteLakeDir(userId) + storagePath.substr(lakeDir.length());
}
} // namespace OHOS::Media
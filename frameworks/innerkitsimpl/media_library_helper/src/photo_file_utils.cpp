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
#include <unordered_set>

#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "media_path_utils.h"
#include "media_string_utils.h"

using namespace std;

namespace OHOS::Media {
constexpr int32_t EDIT_DATA_EXIST = 1;
const std::string DOCS_DIR = "/storage/media/local/files/" + DOCS_PATH;
const std::string DOCS_LPATH_PREFIX = "/FromDocs/";
const std::unordered_set<std::string> FILE_MANAGER_EXCLUDED_DIR_NAMES = {
    "HO_DATA_EXT_MISC",
    ".thumbs",
    ".Recent",
    ".backup",
    ".Trash",
    ".VMDocs",
    ".ohpm",
    "PCEngine",
    "appdata",
};

bool PhotoFileUtils::HasSource(bool hasEditDataCamera,
    int64_t editTime, int32_t effectMode, int32_t subtype, int32_t editDataExist)
{
    return hasEditDataCamera || editTime > 0 ||
            (subtype == static_cast<int32_t>(PhotoSubType::CINEMATIC_VIDEO)) ||
            (subtype == static_cast<int32_t>(PhotoSubType::SLOW_MOTION_VIDEO) && editDataExist == EDIT_DATA_EXIST) ||
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
    return MediaPathUtils::AppendUserId(ROOT_MEDIA_DIR, userId) + metaPath.substr(ROOT_MEDIA_DIR.length());
}

string PhotoFileUtils::GetThumbDir(const string &photoPath, int32_t userId)
{
    if (!MediaPathUtils::CheckPhotoPath(photoPath)) {
        return "";
    }
    if (photoPath.find("..") != string::npos) {
        MEDIA_ERR_LOG("Invalid photoPath with path traversal: %{private}s", photoPath.c_str());
        return "";
    }
    return MediaPathUtils::AppendUserId(
        ROOT_MEDIA_DIR, userId) + ".thumbs/" + photoPath.substr(ROOT_MEDIA_DIR.length());
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

DateParts PhotoFileUtils::ConstructDateAddedDateParts(int64_t dateAdded)
{
    if (dateAdded <= 0) {
        dateAdded = MediaFileUtils::UTCTimeMilliSeconds();
    }
    string dateAddedDateInfo = MediaFileUtils::StrCreateTimeByMilliseconds(
        PhotoColumn::PHOTO_DETAIL_TIME_FORMAT, dateAdded);

    const auto [dateYear, dateMonth, dateDay] = PhotoFileUtils::ExtractYearMonthDay(dateAddedDateInfo);
    DateParts dateParts = {dateYear, dateMonth, dateDay};
    return dateParts;
}

std::string PhotoFileUtils::GetLocalLcdPath(const std::string &photoPath)
{
    if (!MediaPathUtils::CheckPhotoPath(photoPath)) {
        return "";
    }
    if (photoPath.find("..") != string::npos) {
        MEDIA_ERR_LOG("Invalid photoPath with path traversal: %{private}s", photoPath.c_str());
        return "";
    }
    return "/storage/media/local/files/.thumbs/" + photoPath.substr(ROOT_MEDIA_DIR.length()) + "/LCD.jpg";
}

std::string PhotoFileUtils::GetLocalLcdExPath(const std::string &photoPath)
{
    if (!MediaPathUtils::CheckPhotoPath(photoPath)) {
        return "";
    }
    if (photoPath.find("..") != string::npos) {
        MEDIA_ERR_LOG("Invalid photoPath with path traversal: %{private}s", photoPath.c_str());
        return "";
    }
    return "/storage/media/local/files/.thumbs/" + photoPath.substr(ROOT_MEDIA_DIR.length()) + "/THM_EX/LCD.jpg";
}

bool PhotoFileUtils::CheckSubDirForFileManager(const std::string &path, size_t startPos)
{
    size_t pos = path.find('/', startPos);
    std::string subDirName = (pos == std::string::npos) ? path.substr(startPos)
        : path.substr(startPos, pos - startPos);
    CHECK_AND_RETURN_RET(!subDirName.empty(), true);
    bool ret = FILE_MANAGER_EXCLUDED_DIR_NAMES.count(subDirName) == 0;
    CHECK_AND_PRINT_LOG(ret, "sub dir not belong to file manager path");
    return ret;
}

bool PhotoFileUtils::CheckFileManagerRealPath(const std::string &path)
{
    MEDIA_DEBUG_LOG("CheckFileManagerRealPath path: %{public}s", MediaFileUtils::DesensitizePath(path).c_str());
    CHECK_AND_RETURN_RET(MediaStringUtils::StartsWith(path, DOCS_DIR), false);
    size_t startPos = DOCS_DIR.length();
    return CheckSubDirForFileManager(path, startPos);
}

bool PhotoFileUtils::CheckFileManagerLPath(const std::string &lPath)
{
    MEDIA_DEBUG_LOG("CheckFileManagerLPath path: %{public}s", MediaFileUtils::DesensitizePath(lPath).c_str());
    CHECK_AND_RETURN_RET_LOG(MediaStringUtils::StartsWith(lPath, DOCS_LPATH_PREFIX), false, "invalid prefix");
    size_t startPos = DOCS_LPATH_PREFIX.size();
    return CheckSubDirForFileManager(lPath, startPos);
}

std::string PhotoFileUtils::GetFileManagerLPathFromRealPath(const std::string &path)
{
    CHECK_AND_RETURN_RET(CheckFileManagerRealPath(path), "");
    std::string lPath = DOCS_LPATH_PREFIX + path.substr(DOCS_DIR.length());
    size_t lastSlashPos = lPath.find_last_of('/');
    CHECK_AND_RETURN_RET(lastSlashPos != std::string::npos, "");
    if (MediaFileUtils::IsDirectory(path)) {
        lPath = lastSlashPos == lPath.length() - 1 ? lPath.substr(0, lastSlashPos) : lPath;
    } else {
        lPath = lPath.substr(0, lastSlashPos);
    }
    CHECK_AND_EXECUTE(lPath.size() >= DOCS_LPATH_PREFIX.size(), lPath = DOCS_LPATH_PREFIX);
    MEDIA_DEBUG_LOG("GetFileManagerLPathFromRealPath lPath: %{public}s",
        MediaFileUtils::DesensitizePath(lPath).c_str());
    return lPath;
}

std::string PhotoFileUtils::GetFileManagerDirFromLPath(const std::string &lPath)
{
    CHECK_AND_RETURN_RET(CheckFileManagerLPath(lPath), "");
    CHECK_AND_RETURN_RET(lPath != DOCS_LPATH_PREFIX, DOCS_DIR);
    std::string dir = DOCS_DIR + lPath.substr(DOCS_LPATH_PREFIX.size()) + SLASH_STR;
    MEDIA_DEBUG_LOG("GetFileManagerDirFromLPath dir: %{public}s", MediaFileUtils::DesensitizePath(dir).c_str());
    return dir;
}
} // namespace OHOS::Media
/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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
#define MLOG_TAG "Scanner"

#include "metadata_extractor.h"

#include <charconv>
#include <fcntl.h>
#include "directory_ex.h"
#include "hitrace_meter.h"
#include "media_exif.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_tracer.h"
#include "media_image_framework_utils.h"
#include "meta.h"
#include "meta_key.h"
#include "nlohmann/json.hpp"
#include "sandbox_helper.h"
#include "shooting_mode_column.h"
#include "moving_photo_file_utils.h"
#include "directory_ex.h"

namespace OHOS {
namespace Media {
using namespace std;

const double DEGREES2MINUTES = 60.0;
const double DEGREES2SECONDS = 3600.0;
constexpr int32_t OFFSET_NUM = 2;
constexpr int32_t HOURSTOSECOND = 60 * 60;
constexpr int32_t MINUTESTOSECOND = 60;
const string ZEROTIMESTRING = "0000:00:00 00:00:00";

template <class Type>
static Type stringToNum(const string &str)
{
    std::istringstream iss(str);
    Type num;
    iss >> num;
    return num;
}
// LCOV_EXCL_START
static bool IsMovingPhoto(unique_ptr<Metadata> &data)
{
    return data->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ||
        data->GetMovingPhotoEffectMode() == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY);
}

double GetLongitudeLatitude(string inputStr, const string& ref = "")
{
    auto pos = inputStr.find(',');
    CHECK_AND_RETURN_RET(pos != string::npos, 0);

    double ret = stringToNum<double>(inputStr.substr(0, pos));
    inputStr = inputStr.substr(pos + OFFSET_NUM);
    pos = inputStr.find(',');
    CHECK_AND_RETURN_RET(pos != string::npos, 0);

    ret += stringToNum<double>(inputStr.substr(0, pos)) / DEGREES2MINUTES;
    inputStr = inputStr.substr(pos + OFFSET_NUM);
    ret += stringToNum<double>(inputStr) / DEGREES2SECONDS;
    return (ref.compare("W") == 0 || ref.compare("S") == 0) ? -ret : ret;
}

/* used for video */
static time_t convertTimeStr2TimeStamp(string &timeStr)
{
    struct tm timeinfo;
    strptime(timeStr.c_str(), "%Y-%m-%d %H:%M:%S",  &timeinfo);
    time_t timeStamp = mktime(&timeinfo);
    return timeStamp;
}

static time_t convertUTCTimeInformat(const string &timeStr, const string &format)
{
    MEDIA_DEBUG_LOG("convertUTCTimeInformat time:%{public}s format:%{public}s", timeStr.c_str(), format.c_str());
    struct tm timeinfo;
    strptime(timeStr.c_str(), format.c_str(), &timeinfo);
    time_t convertOnceTime = mktime(&timeinfo);
    time_t convertTwiceTime = mktime(gmtime(&convertOnceTime));

    bool cond = (convertOnceTime == -1 || convertTwiceTime == -1);
    CHECK_AND_RETURN_RET(!cond, 0);

    time_t offset = convertOnceTime - convertTwiceTime;
    time_t utcTimeStamp = convertOnceTime + offset;
    MEDIA_DEBUG_LOG("convertUTCTimeInformat result utcTimeStamp:%{public}lld", static_cast<long long>(utcTimeStamp));
    return utcTimeStamp;
}

/* used for Image */
static time_t convertTimeStrToTimeStamp(string &timeStr)
{
    struct tm timeinfo;
    strptime(timeStr.c_str(), "%Y:%m:%d %H:%M:%S",  &timeinfo);
    time_t timeStamp = mktime(&timeinfo);
    return timeStamp;
}

static time_t convertUTCTimeStrToTimeStamp(string &timeStr)
{
    return convertUTCTimeInformat(timeStr, "%Y:%m:%d %H:%M:%S");
}

static int32_t offsetTimeToSeconds(const string& offsetStr, int32_t& offsetTime)
{
    char sign = offsetStr[0];
    const int32_t offsetTimeSize = 6;
    if (offsetStr.size() != offsetTimeSize || (sign != '+' && sign != '-')) {
        MEDIA_WARN_LOG("Invalid offset format, Offset string must be in format +HH:MM or -HH:MM");
        return E_ERR;
    }

    const int32_t colonPosition = 3;
    for (size_t i = 1; i < offsetStr.size(); i++) {
        if (i == colonPosition) {
            continue;
        }
        if (!isdigit(offsetStr[i])) {
            MEDIA_WARN_LOG("Invalid hour or minute format");
            return E_ERR;
        }
    }
    int32_t hours = stoi(offsetStr.substr(1, 2));
    int32_t minutes = stoi(offsetStr.substr(colonPosition + 1, 2));

    int totalSeconds = hours * HOURSTOSECOND + minutes * MINUTESTOSECOND;
    offsetTime = (sign == '-') ? totalSeconds : -totalSeconds;
    MEDIA_DEBUG_LOG("get offset success offsetTime=%{public}d", offsetTime);
    return E_OK;
}

static void SetSubSecondTime(const unique_ptr<ImageSource> &imageSource, const std::string &key, int64_t &timeStamp)
{
    string subTimeStr;
    uint32_t err = imageSource->GetImagePropertyString(0, key, subTimeStr);
    if (err == E_OK && !subTimeStr.empty()) {
        const size_t millisecondPrecision = 3;
        const size_t subTimeSize = std::min(millisecondPrecision, subTimeStr.size());
        int32_t subTime = 0;
        auto [ptr, ec] = std::from_chars(subTimeStr.data(), subTimeStr.data() + subTimeSize, subTime);
        if (ec == std::errc() && ptr == subTimeStr.data() + subTimeSize) {
            MEDIA_DEBUG_LOG("subTime:%{public}d from %{public}s in exif", subTime, key.c_str());
            timeStamp += subTime;
        } else {
            MEDIA_WARN_LOG("Invalid subTime format:%{public}s", subTimeStr.c_str());
        }
    }
}

static void ExtractDetailTimeMetadata(const unique_ptr<ImageSource> &imageSource, unique_ptr<Metadata> &data)
{
    string timeString;
    uint32_t err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_DATE_TIME_ORIGINAL, timeString);
    if (err == E_OK && !timeString.empty() && timeString.compare(ZEROTIMESTRING) != 0) {
        data->SetDetailTime(timeString);
        MEDIA_DEBUG_LOG("Set detail_time:%{public}s from DateTimeOriginal", timeString.c_str());
        return;
    }

    string dateString;
    err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_GPS_DATE_STAMP, dateString);
    if (err == E_OK && !dateString.empty()) {
        err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_GPS_TIME_STAMP, timeString);
        string fullTimeString = dateString + " " + timeString;
        if (err == E_OK && !timeString.empty() && fullTimeString != ZEROTIMESTRING) {
            string detailTime = PhotoFileUtils::ExtractDetailTimeFromGPS(dateString, timeString);
            if (!detailTime.empty()) {
                data->SetDetailTime(detailTime);
                MEDIA_DEBUG_LOG("Set detail_time:%{public}s from GPS", detailTime.c_str());
                return;
            }
        }
    }

    if (data->GetForAdd()) {
        err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_DATE_TIME, timeString);
        if (err == E_OK && !timeString.empty() && timeString.compare(ZEROTIMESTRING) != 0) {
            data->SetDetailTime(timeString);
            MEDIA_DEBUG_LOG("Set detail_time:%{public}s from DateTime", timeString.c_str());
            return;
        }
    }

    string detailTime = data->GetDetailTime();
    if (detailTime.empty()) {
        int64_t dateTaken = data->GetDateTaken() / MSEC_TO_SEC;
        detailTime = MediaFileUtils::StrCreateTime(PhotoColumn::PHOTO_DETAIL_TIME_FORMAT, dateTaken);
        data->SetDetailTime(detailTime);
        MEDIA_DEBUG_LOG("GetDetailTime is empty, get from dateTaken:%{public}lld", static_cast<long long>(dateTaken));
    }
    MEDIA_DEBUG_LOG("Set detail_time:%{public}s from dateTaken", detailTime.c_str());
}

static int64_t GetShootingTimeStampByExif(const unique_ptr<ImageSource> &imageSource)
{
    string timeString;
    int64_t timeStamp = 0;
    int32_t offsetTime = 0;
    string offsetString;
    uint32_t err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_DATE_TIME_ORIGINAL, timeString);
    if (err == E_OK && !timeString.empty() && timeString.compare(ZEROTIMESTRING) != 0) {
        err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_OFFSET_TIME_ORIGINAL, offsetString);
        if (err == E_OK && offsetTimeToSeconds(offsetString, offsetTime) == E_OK) {
            timeStamp = (convertUTCTimeStrToTimeStamp(timeString) + offsetTime) * MSEC_TO_SEC;
            MEDIA_DEBUG_LOG("Get timeStamp from DateTimeOriginal and OffsetTimeOriginal in exif");
        } else {
            timeStamp = (convertTimeStrToTimeStamp(timeString)) * MSEC_TO_SEC;
            MEDIA_DEBUG_LOG("Get timeStamp from DateTimeOriginal in exif");
        }
        if (timeStamp > 0) {
            SetSubSecondTime(imageSource, PHOTO_DATA_IMAGE_SUBSEC_TIME_ORIGINAL, timeStamp);
            MEDIA_DEBUG_LOG("OriginalTimeStamp:%{public}lld in exif", static_cast<long long>(timeStamp));
            return timeStamp;
        }
    }

    string dateString;
    err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_GPS_DATE_STAMP, dateString);
    if (err == E_OK && !dateString.empty()) {
        err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_GPS_TIME_STAMP, timeString);
        string fullTimeString = dateString + " " + timeString;
        if (err == E_OK && !timeString.empty() && fullTimeString.compare(ZEROTIMESTRING) != 0) {
            timeStamp = convertUTCTimeStrToTimeStamp(fullTimeString) * MSEC_TO_SEC;
            if (timeStamp > 0) {
                SetSubSecondTime(imageSource, PHOTO_DATA_IMAGE_SUBSEC_TIME_ORIGINAL, timeStamp);
                MEDIA_DEBUG_LOG("GPSTimeStamp:%{public}lld in exif", static_cast<long long>(timeStamp));
                return timeStamp;
            }
        }
    }
    return timeStamp;
}

static int64_t GetModifiedTimeStampByExif(const unique_ptr<ImageSource> &imageSource)
{
    string dateString;
    string timeString;
    int64_t timeStamp = 0;
    int32_t offsetTime = 0;
    string offsetString;
    uint32_t err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_DATE_TIME, timeString);
    if (err == E_OK && !timeString.empty() && timeString.compare(ZEROTIMESTRING) != 0) {
        err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_OFFSET_TIME, offsetString);
        if (err == E_OK && offsetTimeToSeconds(offsetString, offsetTime) == E_OK) {
            timeStamp = (convertUTCTimeStrToTimeStamp(timeString) + offsetTime) * MSEC_TO_SEC;
            MEDIA_DEBUG_LOG("Get timeStamp from DateTime and OffsetTime in exif");
        } else {
            timeStamp = (convertTimeStrToTimeStamp(timeString)) * MSEC_TO_SEC;
            MEDIA_DEBUG_LOG("Get timeStamp from DateTime in exif");
        }
        if (timeStamp > 0) {
            SetSubSecondTime(imageSource, PHOTO_DATA_IMAGE_SUBSEC_TIME, timeStamp);
            MEDIA_DEBUG_LOG("TimeStamp:%{public}lld in exif", static_cast<long long>(timeStamp));
            return timeStamp;
        }
    }
    return timeStamp;
}

static void ExtractDateTakenMetadata(const unique_ptr<ImageSource> &imageSource, unique_ptr<Metadata> &data)
{
    int64_t shootingTimeStamp = GetShootingTimeStampByExif(imageSource);
    if (shootingTimeStamp > 0) {
        data->SetDateTaken(shootingTimeStamp);
        MEDIA_INFO_LOG("forAdd:%{public}d, shootingTimeStamp:%{public}lld",
            data->GetForAdd(),
            static_cast<long long>(shootingTimeStamp));
        return;
    }
    int64_t dateTaken = data->GetDateTaken();
    if (dateTaken > 0 && !data->GetForAdd()) {
        MEDIA_WARN_LOG("Set date_taken use old date_taken: %{public}lld", static_cast<long long>(dateTaken));
        return;
    }
    int64_t dateAdded = data->GetFileDateAdded();
    if (dateAdded > 0) {
        dateTaken = dateTaken > 0 ? min(dateTaken, dateAdded) : dateAdded;
    }
    int64_t dateModified = data->GetFileDateModified();
    if (dateModified > 0) {
        dateTaken = dateTaken > 0 ? min(dateTaken, dateModified) : dateModified;
    }
    int64_t modifiedTimeStamp = GetModifiedTimeStampByExif(imageSource);
    if (modifiedTimeStamp > 0) {
        dateTaken = dateTaken > 0 ? min(dateTaken, modifiedTimeStamp) : modifiedTimeStamp;
    }
    if (dateTaken <= 0) {
        dateTaken = MediaFileUtils::UTCTimeMilliSeconds();
    }
    data->SetDateTaken(dateTaken);
    MEDIA_INFO_LOG("Set date_taken use dateAdded:%{public}lld or dateModified:%{public}lld or "
                   "modifiedTimeStamp:%{public}lld, dateTaken:%{public}lld",
        static_cast<long long>(dateAdded),
        static_cast<long long>(dateModified),
        static_cast<long long>(modifiedTimeStamp),
        static_cast<long long>(dateTaken));
}

static string GetCompatibleUserComment(const string& userComment)
{
    const string startFlag = "<mgzn-content>";
    const string endFlag = "<mgzn-worksdes>";
    size_t posStart = userComment.find(startFlag);
    size_t posEnd = userComment.find(endFlag);
    if (posStart == string::npos || posEnd == string::npos || posStart >= posEnd) {
        return userComment;
    }

    posStart += startFlag.length();
    return userComment.substr(posStart, posEnd - posStart);
}

int32_t MetadataExtractor::ExtractImageExif(std::unique_ptr<ImageSource> &imageSource, std::unique_ptr<Metadata> &data)
{
    if (imageSource == nullptr) {
        MEDIA_ERR_LOG("Failed to obtain image source");
        return E_OK;
    }

    int32_t intTempMeta = 0;
    string propertyStr;
    uint32_t err;

    nlohmann::json exifJson;
    err = imageSource->GetImagePropertyInt(0, PHOTO_DATA_IMAGE_ORIENTATION, intTempMeta);
    exifJson[PHOTO_DATA_IMAGE_ORIENTATION] = (err == 0) ? intTempMeta: 0;

    err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_GPS_LONGITUDE, propertyStr);
    exifJson[PHOTO_DATA_IMAGE_GPS_LONGITUDE] = (err == 0) ? GetLongitudeLatitude(propertyStr): 0;

    err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_GPS_LATITUDE, propertyStr);
    exifJson[PHOTO_DATA_IMAGE_GPS_LATITUDE] = (err == 0) ? GetLongitudeLatitude(propertyStr): 0;

    err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_FRONT_CAMERA, propertyStr);
    data->SetFrontCamera(err == 0 ? propertyStr : "0");

    for (auto &exifKey : exifInfoKeys) {
        err = imageSource->GetImagePropertyString(0, exifKey, propertyStr);
        exifJson[exifKey] = (err == 0) ? propertyStr: "";
    }
    exifJson[PHOTO_DATA_IMAGE_IMAGE_DESCRIPTION] =
        AppFileService::SandboxHelper::Encode(exifJson[PHOTO_DATA_IMAGE_IMAGE_DESCRIPTION]);
    data->SetAllExif(exifJson.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace));

    err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_USER_COMMENT, propertyStr);
    if (err == 0 && !propertyStr.empty()) {
        data->SetUserComment(GetCompatibleUserComment(propertyStr));
    }
    err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_PHOTO_MODE, propertyStr);
    if (err != 0 || propertyStr == "default_exif_value") {
        err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_ISO_SPEED_LATITUDE_ZZZ, propertyStr);
    }
    if (err == 0 && !propertyStr.empty()) {
        data->SetShootingModeTag(propertyStr);
        data->SetShootingMode(ShootingModeAlbum::MapShootingModeTagToShootingMode(propertyStr));
    }

    int64_t timeNow = MediaFileUtils::UTCTimeMilliSeconds();
    data->SetLastVisitTime(timeNow);

    return E_OK;
}

static void ExtractLocationMetadata(unique_ptr<ImageSource>& imageSource, unique_ptr<Metadata>& data)
{
    string propertyStr;
    string refStr;
    double tempLocation = -1;
    uint32_t err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_GPS_LONGITUDE, propertyStr);
    uint32_t refErr = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_GPS_LONGITUDE_REF, refStr);
    if (err == 0 && refErr == 0) {
        tempLocation = GetLongitudeLatitude(propertyStr, refStr);
        data->SetLongitude(tempLocation);
    }

    err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_GPS_LATITUDE, propertyStr);
    refErr = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_GPS_LATITUDE_REF, refStr);
    if (err == 0 && refErr == 0) {
        tempLocation = GetLongitudeLatitude(propertyStr, refStr);
        data->SetLatitude(tempLocation);
    }
}

static void ExtractImageExifRotate(
    std::unique_ptr<ImageSource> &imageSource, std::unique_ptr<Metadata> &data)
{
    int32_t exifRotate = static_cast<int32_t>(ExifRotateType::TOP_LEFT);
    MediaImageFrameWorkUtils::GetExifRotate(imageSource, exifRotate);
    data->SetExifRotate(exifRotate);
}

int32_t MetadataExtractor::ExtractImageMetadata(std::unique_ptr<Metadata> &data)
{
    uint32_t err = 0;

    SourceOptions opts;
    opts.formatHint = "image/" + data->GetFileExtension();
    std::unique_ptr<ImageSource> imageSource =
        ImageSource::CreateImageSource(data->GetFilePath(), opts, err);
    if (err != 0 || imageSource == nullptr) {
        MEDIA_ERR_LOG("Failed to obtain image source, err = %{public}d", err);
        return E_IMAGE;
    }

    ImageInfo imageInfo;
    err = imageSource->GetImageInfoFromExif(0, imageInfo);
    if (err == 0) {
        data->SetFileWidth(imageInfo.size.width);
        data->SetFileHeight(imageInfo.size.height);
    } else {
        MEDIA_ERR_LOG("Failed to get image info, err = %{public}d", err);
    }

    ExtractDateTakenMetadata(imageSource, data);
    ExtractDetailTimeMetadata(imageSource, data);
    auto const [dateYear, dateMonth, dateDay] = PhotoFileUtils::ExtractYearMonthDay(data->GetDetailTime());
    data->SetDateYear(dateYear);
    data->SetDateMonth(dateMonth);
    data->SetDateDay(dateDay);

    int32_t intTempMeta = 0;
    err = imageSource->GetImagePropertyInt(0, PHOTO_DATA_IMAGE_ORIENTATION, intTempMeta);
    if (err == 0) {
        data->SetOrientation(intTempMeta);
    }
    ExtractImageExifRotate(imageSource, data);

    if (imageSource->IsHdrImage()) {
        data->SetDynamicRangeType(static_cast<int32_t>(DynamicRangeType::HDR));
        HdrMode hdrMode = MediaImageFrameWorkUtils::ConvertImageHdrTypeToHdrMode(imageSource->CheckHdrType());
        data->SetHdrMode(static_cast<int32_t>(hdrMode));
    } else {
        data->SetDynamicRangeType(static_cast<int32_t>(DynamicRangeType::SDR));
    }

    ExtractLocationMetadata(imageSource, data);
    ExtractImageExif(imageSource, data);
    return E_OK;
}

static std::string ExtractVideoShootingMode(const std::string &genreJson)
{
    if (genreJson.empty()) {
        return "";
    }
    size_t pos = genreJson.find("param-use-tag");
    if (pos != std::string::npos) {
        size_t start = genreJson.find(":", pos);
        size_t end = genreJson.find(",", pos);
        if (end == std::string::npos) {
            end = genreJson.find("}", pos);
        }
        return genreJson.substr(start + 1, end - start - 1); // 1: length offset
    }
    return "";
}

void PopulateExtractedAVMetadataOne(const std::unordered_map<int32_t, std::string> &resultMap,
    std::unique_ptr<Metadata> &data)
{
    int32_t intTempMeta;

    string strTemp = resultMap.at(AV_KEY_ALBUM);
    if (strTemp != "") {
        data->SetAlbum(strTemp);
    }

    strTemp = resultMap.at(AV_KEY_ARTIST);
    if (strTemp != "") {
        data->SetFileArtist(strTemp);
    }

    strTemp = resultMap.at(AV_KEY_DURATION);
    if (strTemp != "") {
        intTempMeta = stringToNum<int32_t>(strTemp);
        data->SetFileDuration(intTempMeta);
    }

    strTemp = resultMap.at(AV_KEY_VIDEO_HEIGHT);
    if (strTemp != "") {
        intTempMeta = stringToNum<int32_t>(strTemp);
        data->SetFileHeight(intTempMeta);
    }

    strTemp = resultMap.at(AV_KEY_VIDEO_WIDTH);
    if (strTemp != "") {
        intTempMeta = stringToNum<int32_t>(strTemp);
        data->SetFileWidth(intTempMeta);
    }

    strTemp = resultMap.at(AV_KEY_MIME_TYPE);
    if (strTemp != "") {
        data->SetFileMimeType(strTemp);
    }
}

int32_t ExtractedAVMetadataExifRotate(const std::unordered_map<int32_t, std::string> &resultMap)
{
    int32_t exifRotate = static_cast<int32_t>(ExifRotateType::TOP_LEFT);
    auto it = resultMap.find(AV_KEY_VIDEO_ROTATE_ORIENTATION);
    CHECK_AND_RETURN_RET(it != resultMap.end(), exifRotate);

    std::string exifRotateStr = it->second;
    CHECK_AND_RETURN_RET(!exifRotateStr.empty(), exifRotate);

    exifRotate = stringToNum<int32_t>(exifRotateStr);
    return exifRotate;
}

std::string ParseDetailTime(const string &timeStr, const string &format)
{
    std::tm timeInfo{};
    std::istringstream iss(timeStr);
    iss >> std::get_time(&timeInfo, format.c_str());
    if (iss.fail()) {
        MEDIA_ERR_LOG(
            "Parse DetailTime failed, timeStr: %{public}s, format: %{public}s", timeStr.c_str(), format.c_str());
        return "";
    }

    std::ostringstream oss;
    oss << std::put_time(&timeInfo, PhotoColumn::PHOTO_DETAIL_TIME_FORMAT.c_str());
    return oss.str();
}

void PopulateAVMetadataDateTaken(
    const std::unordered_map<int32_t, std::string> &resultMap, std::unique_ptr<Metadata> &data)
{
    // first take utc time
    string timeStr = resultMap.at(AV_KEY_DATE_TIME_ISO8601);
    int64_t dateTaken = convertUTCTimeInformat(timeStr, "%Y-%m-%dT%H:%M:%S");
    if (dateTaken > 0) {
        data->SetDateTaken(dateTaken * MSEC_TO_SEC);
        return;
    }
    timeStr = resultMap.at(AV_KEY_DATE_TIME_FORMAT);
    dateTaken = convertTimeStr2TimeStamp(timeStr);
    if (dateTaken > 0) {
        data->SetDateTaken(dateTaken * MSEC_TO_SEC);
        MEDIA_WARN_LOG("Set date_taken use local time");
        return;
    }
    dateTaken = data->GetDateTaken();
    if (dateTaken > 0 && !data->GetForAdd()) {
        MEDIA_WARN_LOG("Set date_taken use old date_taken: %{public}lld", static_cast<long long>(dateTaken));
        return;
    }
    int64_t dateAdded = data->GetFileDateAdded();
    if (dateAdded > 0) {
        dateTaken = dateTaken > 0 ? min(dateTaken, dateAdded) : dateAdded;
    }
    int64_t dateModified = data->GetFileDateModified();
    if (dateModified > 0) {
        dateTaken = dateTaken > 0 ? min(dateTaken, dateModified) : dateModified;
    }
    if (dateTaken <= 0) {
        dateTaken = MediaFileUtils::UTCTimeMilliSeconds();
    }
    data->SetDateTaken(dateTaken);
    MEDIA_INFO_LOG("Set date_taken use dateAdded:%{public}lld or dateModified:%{public}lld, dateTaken:%{public}lld",
        static_cast<long long>(dateAdded),
        static_cast<long long>(dateModified),
        static_cast<long long>(dateTaken));
}

void PopulateAVMetadataDetailTime(
    const std::unordered_map<int32_t, std::string> &resultMap, std::unique_ptr<Metadata> &data)
{
    string timeStr = resultMap.at(AV_KEY_DATE_TIME_ISO8601);
    const string zeroTime = "1970-01-01T00:00:00.000000Z";
    if (timeStr != zeroTime) {
        timeStr = resultMap.at(AV_KEY_DATE_TIME_FORMAT);
        string detailTime = ParseDetailTime(timeStr, "%Y-%m-%d %H:%M:%S");
        if (!detailTime.empty()) {
            data->SetDetailTime(detailTime);
            MEDIA_DEBUG_LOG("Set detail_time:%{public}s from AV_KEY_DATE_TIME_FORMAT", detailTime.c_str());
            return;
        }
    } else {
        MEDIA_WARN_LOG("zeroTime:%{public}s", timeStr.c_str());
    }

    string detailTime = data->GetDetailTime();
    if (detailTime.empty() || data->GetForAdd()) {
        int64_t dateTaken = data->GetDateTaken() / MSEC_TO_SEC;
        detailTime = MediaFileUtils::StrCreateTime(PhotoColumn::PHOTO_DETAIL_TIME_FORMAT, dateTaken);
        MEDIA_DEBUG_LOG("detail_time:%{public}s, forAdd:%{public}d", detailTime.c_str(), data->GetForAdd());
    }
    data->SetDetailTime(detailTime);
    MEDIA_DEBUG_LOG("Set detail_time:%{public}s from dateTaken", detailTime.c_str());
}

void PopulateExtractedAVMetadataTwo(
    const std::unordered_map<int32_t, std::string> &resultMap, std::unique_ptr<Metadata> &data)
{
    int32_t intTempMeta{0};
    string strTemp = resultMap.at(AV_KEY_VIDEO_ORIENTATION);
    if (!strTemp.empty()) {
        intTempMeta = stringToNum<int32_t>(strTemp);
    }
    data->SetOrientation(intTempMeta);

    intTempMeta = ExtractedAVMetadataExifRotate(resultMap);
    data->SetExifRotate(intTempMeta);

    strTemp = resultMap.at(AV_KEY_GENRE);
    if (!strTemp.empty()) {
        std::string videoShootingMode = ExtractVideoShootingMode(strTemp);
        data->SetShootingModeTag(videoShootingMode);
        data->SetShootingMode(ShootingModeAlbum::MapShootingModeTagToShootingMode(videoShootingMode));
    }
    strTemp = resultMap.at(AV_KEY_VIDEO_IS_HDR_VIVID);
    const string isHdr = "yes";
    if (strcmp(strTemp.c_str(), isHdr.c_str()) == 0) {
        data->SetDynamicRangeType(static_cast<int32_t>(DynamicRangeType::HDR));
    } else {
        data->SetDynamicRangeType(static_cast<int32_t>(DynamicRangeType::SDR));
    }
}

void PopulateExtractedAVLocationMeta(std::shared_ptr<Meta> &meta, std::unique_ptr<Metadata> &data)
{
    float floatTempMeta;

    if (meta->GetData(Tag::MEDIA_LATITUDE, floatTempMeta)) {
        data->SetLatitude((double)floatTempMeta);
    }
    if (meta->GetData(Tag::MEDIA_LONGITUDE, floatTempMeta)) {
        data->SetLongitude((double)floatTempMeta);
    }
}

static void ParseLivePhotoCoverPosition(std::unique_ptr<Metadata> &data)
{
    string extraPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(data->GetMovingPhotoImagePath());
    string absExtraPath;
    if (!PathToRealPath(extraPath, absExtraPath)) {
        MEDIA_ERR_LOG("file is not real path: %{private}s, errno: %{public}d", extraPath.c_str(), errno);
        return;
    }
    UniqueFd fd(open(absExtraPath.c_str(), O_RDONLY));
    uint32_t version{0};
    uint32_t frameIndex{0};
    bool hasCinemagraphInfo{false};
    if (MovingPhotoFileUtils::GetVersionAndFrameNum(fd, version, frameIndex, hasCinemagraphInfo) != E_OK) {
        return;
    }
    uint64_t coverPosition;
    if (MovingPhotoFileUtils::GetCoverPosition(data->GetFilePath(), frameIndex, coverPosition) != E_OK) {
        return;
    }
    data->SetCoverPosition(static_cast<int64_t>(coverPosition));
}

static void ParseMovingPhotoCoverPosition(std::shared_ptr<Meta> &meta, std::unique_ptr<Metadata> &data)
{
    shared_ptr<Meta> customMeta = make_shared<Meta>();
    bool isValid = meta->GetData(PHOTO_DATA_VIDEO_CUSTOM_INFO, customMeta);
    if (!isValid) {
        MEDIA_INFO_LOG("Video of moving photo does not contain customInfo");
        return ParseLivePhotoCoverPosition(data);
    }

    float coverPosition = 0.0f;
    isValid = customMeta->GetData(PHOTO_DATA_VIDEO_COVER_TIME, coverPosition);
    if (!isValid) {
        MEDIA_INFO_LOG("Video of moving photo does not contain cover position");
        return ParseLivePhotoCoverPosition(data);
    }
    // convert cover position from ms(float) to us(int64_t)
    constexpr int32_t MS_TO_US = 1000;
    data->SetCoverPosition(static_cast<int64_t>(coverPosition * MS_TO_US));
}

void MetadataExtractor::FillExtractedMetadata(const std::unordered_map<int32_t, std::string> &resultMap,
    std::shared_ptr<Meta> &meta, std::unique_ptr<Metadata> &data)
{
    PopulateExtractedAVMetadataOne(resultMap, data);
    PopulateExtractedAVMetadataTwo(resultMap, data);
    PopulateAVMetadataDateTaken(resultMap, data);
    PopulateAVMetadataDetailTime(resultMap, data);
    auto const [dateYear, dateMonth, dateDay] = PhotoFileUtils::ExtractYearMonthDay(data->GetDetailTime());
    data->SetDateYear(dateYear);
    data->SetDateMonth(dateMonth);
    data->SetDateDay(dateDay);
    PopulateExtractedAVLocationMeta(meta, data);

    int64_t timeNow = MediaFileUtils::UTCTimeMilliSeconds();
    data->SetLastVisitTime(timeNow);

    if (IsMovingPhoto(data)) {
        ParseMovingPhotoCoverPosition(meta, data);
    }
}

static void FillFrameIndex(std::shared_ptr<AVMetadataHelper> &avMetadataHelper,
    std::unique_ptr<Metadata> &data)
{
    CHECK_AND_RETURN_WARN_LOG(IsMovingPhoto(data), "data is not moving photo");
    uint64_t coverPosition = static_cast<uint64_t>(data->GetCoverPosition());
    uint32_t frameIndex = 0;

    int32_t err = avMetadataHelper->GetFrameIndexByTime(coverPosition, frameIndex);
    CHECK_AND_RETURN_LOG(err == E_OK, "Failed to get frame index, err: %{public}d", err);
    data->SetFrameIndex(static_cast<int32_t>(frameIndex));
}

int32_t MetadataExtractor::ExtractAVMetadata(std::unique_ptr<Metadata> &data, int32_t scene)
{
    MediaLibraryTracer tracer;
    tracer.Start("ExtractAVMetadata");

    tracer.Start("CreateAVMetadataHelper");
    std::shared_ptr<AVMetadataHelper> avMetadataHelper = AVMetadataHelperFactory::CreateAVMetadataHelper();
    tracer.Finish();
    CHECK_AND_RETURN_RET_LOG(avMetadataHelper != nullptr, E_AVMETADATA, "AV metadata helper is null");

    // notify media_service clone event.
    if (scene == Scene::AV_META_SCENE_CLONE) {
        avMetadataHelper->SetScene(static_cast<Scene>(scene));
    }

    string filePath = data->GetFilePath();
    CHECK_AND_RETURN_RET_LOG(!filePath.empty(), E_AVMETADATA, "AV metadata file path is empty");
    std::string realFilePath;
    if (!PathToRealPath(filePath, realFilePath)) {
        MEDIA_ERR_LOG("file is not real path, file path: %{private}s", filePath.c_str());
        return E_AVMETADATA;
    }
    int32_t fd = open(realFilePath.c_str(), O_RDONLY);
    CHECK_AND_RETURN_RET_LOG(fd > 0, E_SYSCALL, "Open file descriptor failed, errno = %{public}d", errno);

    struct stat64 st;
    if (fstat64(fd, &st) != 0) {
        MEDIA_ERR_LOG("Get file state failed for the given fd");
        (void)close(fd);
        return E_SYSCALL;
    }
    data->SetFileSize(st.st_size);

    tracer.Start("avMetadataHelper->SetSource");
    int32_t err = avMetadataHelper->SetSource(fd, 0, static_cast<int64_t>(st.st_size), AV_META_USAGE_META_ONLY);
    tracer.Finish();
    if (err != 0) {
        MEDIA_ERR_LOG("SetSource failed for the given file descriptor, err = %{public}d", err);
        (void)close(fd);
        return E_AVMETADATA;
    }
    tracer.Start("avMetadataHelper->ResolveMetadata");
    std::shared_ptr<Meta> meta = avMetadataHelper->GetAVMetadata();
    std::unordered_map<int32_t, std::string> resultMap = avMetadataHelper->ResolveMetadata();
    tracer.Finish();
    if (!resultMap.empty()) {
        FillExtractedMetadata(resultMap, meta, data);
        if (IsMovingPhoto(data)) {
            FillFrameIndex(avMetadataHelper, data);
        }
    }

    (void)close(fd);

    return E_OK;
}

int32_t MetadataExtractor::CombineMovingPhotoMetadata(std::unique_ptr<Metadata> &data,
    bool isCameraShotMovingPhoto)
{
    // if video of moving photo does not exist, just return
    string videoPath = MediaFileUtils::GetMovingPhotoVideoPath(data->GetFilePath());
    if (!MediaFileUtils::IsFileExists(videoPath)) {
        MEDIA_INFO_LOG("Video of moving photo does not exist, path: %{private}s", videoPath.c_str());
        return E_OK;
    }

    size_t videoSize = 0;
    if (!MediaFileUtils::GetFileSize(videoPath, videoSize) || videoSize == 0) {
        MEDIA_INFO_LOG("Video of moving photo is empty now, path: %{private}s", videoPath.c_str());
        return E_OK;
    }

    unique_ptr<Metadata> videoData = make_unique<Metadata>();
    videoData->SetMovingPhotoImagePath(data->GetFilePath());
    videoData->SetFilePath(videoPath);
    videoData->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    int32_t err = ExtractAVMetadata(videoData);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err,
        "Failed to extract video metadata for moving photo: %{private}s", videoPath.c_str());

    data->SetCoverPosition(videoData->GetCoverPosition());

    uint32_t frameIndex = MovingPhotoFileUtils::GetFrameIndex(videoData->GetCoverPosition(),
        UniqueFd(open(videoPath.c_str(), O_RDONLY)));
    off_t extraDataSize{0};
    if (MovingPhotoFileUtils::GetExtraDataLen(data->GetFilePath(), videoPath,
        frameIndex, videoData->GetCoverPosition(), extraDataSize, isCameraShotMovingPhoto) != E_OK) {
        MEDIA_WARN_LOG("Failed to get extra data file size");
    }
    data->SetFileSize(data->GetFileSize() + videoData->GetFileSize() + extraDataSize);
    int64_t videoDateModified = videoData->GetFileDateModified();
    if (videoDateModified > data->GetFileDateModified()) {
        data->SetFileDateModified(videoDateModified);
    }

    int32_t duration = videoData->GetFileDuration();
    if (!MediaFileUtils::CheckMovingPhotoVideoDuration(duration)) {
        MEDIA_ERR_LOG("Failed to check video duration (%{public}d ms) of moving photo", duration);
        return E_MOVING_PHOTO;
    }
    return E_OK;
}

int32_t MetadataExtractor::Extract(std::unique_ptr<Metadata> &data, bool isCameraShotMovingPhoto)
{
    if (data->GetFileMediaType() == MEDIA_TYPE_IMAGE) {
        int32_t ret = ExtractImageMetadata(data);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to extract image metadata");
        if (IsMovingPhoto(data)) {
            return CombineMovingPhotoMetadata(data, isCameraShotMovingPhoto);
        }
        return ret;
    } else {
        return ExtractAVMetadata(data);
    }
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS

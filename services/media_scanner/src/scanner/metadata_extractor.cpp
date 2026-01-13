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
#define MLOG_TAG "MetadataExtractor"

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
#include "result_set_utils.h"
#include "medialibrary_unistore_manager.h"
#include "photo_video_mode_operation.h"
#include "userfile_manager_types.h"

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

static std::tuple<int64_t, std::string> TryGetFromDateTimeOriginal(const unique_ptr<ImageSource> &imageSource)
{
    string timeString;
    uint32_t err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_DATE_TIME_ORIGINAL, timeString);
    if (err != E_OK || timeString.empty() || timeString.compare(ZEROTIMESTRING) == 0) {
        return {0, ""};
    }

    string offsetString;
    err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_OFFSET_TIME_ORIGINAL, offsetString);
    int32_t offsetTime = 0;
    bool hasOffset = (err == E_OK && !offsetString.empty() && offsetTimeToSeconds(offsetString, offsetTime) == E_OK);

    auto [dateTaken, detailTime] =
        PhotoFileUtils::ExtractTimeInfo(timeString, PhotoColumn::PHOTO_DETAIL_TIME_FORMAT, hasOffset);
    if (dateTaken <= 0) {
        MEDIA_ERR_LOG("extract time info from DateTimeOriginal failed, timeString: %{public}s", timeString.c_str());
        return {0, ""};
    }

    if (hasOffset) {
        dateTaken += offsetTime * SEC_TO_MSEC;
        MEDIA_DEBUG_LOG("Get dateTaken from DateTimeOriginal and OffsetTimeOriginal in exif");
    } else {
        MEDIA_DEBUG_LOG("Get dateTaken from DateTimeOriginal in exif");
    }

    SetSubSecondTime(imageSource, PHOTO_DATA_IMAGE_SUBSEC_TIME_ORIGINAL, dateTaken);
    MEDIA_DEBUG_LOG("get dateTaken:%{public}lld, detailTime:%{public}s from DateTimeOriginal",
        static_cast<long long>(dateTaken),
        detailTime.c_str());
    return {dateTaken, detailTime};
}

static std::tuple<int64_t, std::string> TryGetFromGPSTime(const unique_ptr<ImageSource> &imageSource)
{
    string dateString;

    uint32_t err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_GPS_DATE_STAMP, dateString);
    if (err != E_OK || dateString.empty()) {
        return {0, ""};
    }

    string timeString;
    err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_GPS_TIME_STAMP, timeString);
    if (err != E_OK || timeString.empty()) {
        return {0, ""};
    }

    string fullTimeString = dateString + " " + timeString;
    if (fullTimeString.compare(ZEROTIMESTRING) == 0) {
        return {0, ""};
    }

    auto [dateTaken, detailTime] =
        PhotoFileUtils::ExtractTimeInfo(fullTimeString, PhotoColumn::PHOTO_DETAIL_TIME_FORMAT, true);
    if (dateTaken <= 0) {
        MEDIA_ERR_LOG("extract time info from GPS time failed, fullTimeString: %{public}s", fullTimeString.c_str());
        return {0, ""};
    }

    SetSubSecondTime(imageSource, PHOTO_DATA_IMAGE_SUBSEC_TIME_ORIGINAL, dateTaken);
    MEDIA_DEBUG_LOG("get dateTaken:%{public}lld, detailTime:%{public}s from GPS time",
        static_cast<long long>(dateTaken),
        detailTime.c_str());
    return {dateTaken, detailTime};
}

static std::tuple<int64_t, std::string> GetShootingTimeByExif(const unique_ptr<ImageSource> &imageSource)
{
    auto result = TryGetFromDateTimeOriginal(imageSource);
    if (std::get<0>(result) > 0) {
        return result;
    }

    return TryGetFromGPSTime(imageSource);
}

static std::tuple<int64_t, std::string> GetModifiedTimeByExif(const unique_ptr<ImageSource> &imageSource)
{
    string timeString;
    uint32_t err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_DATE_TIME, timeString);
    if (err == E_OK && !timeString.empty() && timeString.compare(ZEROTIMESTRING) != 0) {
        string offsetString;
        err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_OFFSET_TIME, offsetString);
        int32_t offsetTime = 0;
        int64_t dateTaken = 0;
        string detailTime;
        if (err == E_OK && offsetTimeToSeconds(offsetString, offsetTime) == E_OK) {
            std::tie(dateTaken, detailTime) =
                PhotoFileUtils::ExtractTimeInfo(timeString, PhotoColumn::PHOTO_DETAIL_TIME_FORMAT, true);
            dateTaken += offsetTime * SEC_TO_MSEC;
            MEDIA_DEBUG_LOG("Get dateTaken from DateTime and OffsetTime in exif");
        } else {
            std::tie(dateTaken, detailTime) =
                PhotoFileUtils::ExtractTimeInfo(timeString, PhotoColumn::PHOTO_DETAIL_TIME_FORMAT);
            MEDIA_DEBUG_LOG("Get dateTaken from DateTime in exif");
        }
        if (dateTaken > 0) {
            SetSubSecondTime(imageSource, PHOTO_DATA_IMAGE_SUBSEC_TIME, dateTaken);
            MEDIA_DEBUG_LOG("get dateTaken:%{public}lld, detailTime:%{public}s from DateTime",
                static_cast<long long>(dateTaken),
                detailTime.c_str());
            return {dateTaken, detailTime};
        }
    }
    return {0, ""};
}

static void SetTimeInfo(const int64_t dateTaken, const string &detailTime, std::unique_ptr<Metadata> &data)
{
    CHECK_AND_RETURN_LOG(data != nullptr, "Metadata is nullptr");

    data->SetDateTaken(dateTaken);
    data->SetDetailTime(detailTime);

    auto const [dateYear, dateMonth, dateDay] = PhotoFileUtils::ExtractYearMonthDay(detailTime);
    data->SetDateYear(dateYear);
    data->SetDateMonth(dateMonth);
    data->SetDateDay(dateDay);
}

void MetadataExtractor::ExtractImageTimeInfo(const unique_ptr<ImageSource> &imageSource, unique_ptr<Metadata> &data)
{
    auto [dateTaken, detailTime] = GetShootingTimeByExif(imageSource);
    bool forAdd = data->GetForAdd();
    if (dateTaken > 0) {
        SetTimeInfo(dateTaken, detailTime, data);
        MEDIA_INFO_LOG("forAdd:%{public}d, dateTaken:%{public}lld, detailTime:%{public}s",
            forAdd,
            static_cast<long long>(dateTaken),
            detailTime.c_str());
        return;
    }
    dateTaken = data->GetDateTaken();
    detailTime = data->GetDetailTime();
    if (dateTaken > 0 && !detailTime.empty() && !forAdd) {
        SetTimeInfo(dateTaken, detailTime, data);
        MEDIA_WARN_LOG("use old dateTaken:%{public}lld, detailTime:%{public}s",
            static_cast<long long>(dateTaken),
            detailTime.c_str());
        return;
    }
    if (dateTaken < MIN_MILSEC_TIMESTAMP) {
        dateTaken = data->GetFileDateAdded();
        int64_t dateModified = data->GetFileDateModified();
        if (dateModified >= MIN_MILSEC_TIMESTAMP) {
            dateTaken = dateTaken >= MIN_MILSEC_TIMESTAMP ? std::min(dateTaken, dateModified) : dateModified;
        }
        if (dateTaken < MIN_MILSEC_TIMESTAMP) {
            dateTaken = MediaFileUtils::UTCTimeMilliSeconds();
        }
    }
    const auto [modifiedTimeStamp, modifiedDetailTime] = GetModifiedTimeByExif(imageSource);
    dateTaken = modifiedTimeStamp >= MIN_MILSEC_TIMESTAMP ? std::min(dateTaken, modifiedTimeStamp) : dateTaken;
    const auto [parseDateTaken, parseDetailTime] =
        PhotoFileUtils::ExtractTimeInfo(detailTime, PhotoColumn::PHOTO_DETAIL_TIME_FORMAT);
    if (dateTaken == modifiedTimeStamp) {
        detailTime = modifiedDetailTime;
    } else if (parseDateTaken < MIN_MILSEC_TIMESTAMP || parseDateTaken > MAX_MILSEC_TIMESTAMP ||
               abs(dateTaken - parseDateTaken) > MAX_TIMESTAMP_DIFF) {
        detailTime = MediaFileUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DETAIL_TIME_FORMAT, dateTaken);
    } else {
        detailTime = parseDetailTime;
    }
    SetTimeInfo(dateTaken, detailTime, data);
    MEDIA_WARN_LOG("no shooting time, modifiedTimeStamp:%{public}lld, dateTaken:%{public}lld, detailTime:%{public}s",
        static_cast<long long>(modifiedTimeStamp),
        static_cast<long long>(dateTaken),
        detailTime.c_str());
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
        data->SetFileMimeType(imageInfo.encodedFormat);
        double aspectRatio =
            MediaFileUtils::CalculateAspectRatio(imageInfo.size.height, imageInfo.size.width);
        data->SetFileAspectRatio(aspectRatio);
    } else {
        MEDIA_ERR_LOG("Failed to get image info, err = %{public}d", err);
    }

    ExtractImageTimeInfo(imageSource, data);

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

    //Camera shooting mode parse after 6.1
    if (ShootingModeAlbum::VALID_TAGS.find(genreJson) != ShootingModeAlbum::VALID_TAGS.end()) {
        return genreJson;
    }

    //Compatible Camera shooting mode parse before 4.x
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
    MEDIA_INFO_LOG("PopulateExtractedAVMetadataOne start");
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

    double aspectRatio =
        MediaFileUtils::CalculateAspectRatio(data->GetFileHeight(), data->GetFileWidth());
    data->SetFileAspectRatio(aspectRatio);

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

bool TrySetTimeInfoFromVideoMetadata(const shared_ptr<Meta> &customMeta,
    const std::unordered_map<int32_t, std::string> &resultMap, std::unique_ptr<Metadata> &data)
{
    bool forAdd = data->GetForAdd();
    std::string localTime;
    if (customMeta != nullptr && customMeta->GetData(PHOTO_DATA_VIDEO_IOS_CREATION_DATE, localTime) &&
        !localTime.empty()) {
        auto [dateTaken, detailTime] = PhotoFileUtils::ExtractTimeInfo(localTime, "%Y-%m-%dT%H:%M:%S");
        if (dateTaken > 0) {
            SetTimeInfo(dateTaken, detailTime, data);
            MEDIA_INFO_LOG("forAdd:%{public}d, extract iOS video localTime:%{public}s", forAdd, localTime.c_str());
            return true;
        }
    }

    auto it = resultMap.find(AV_KEY_DATE_TIME_ISO8601);
    std::string utcTime = it != resultMap.end() ? it->second : "";
    auto [dateTaken, utcDetailTime] = PhotoFileUtils::ExtractTimeInfo(utcTime, "%Y-%m-%dT%H:%M:%S", true);

    it = resultMap.find(AV_KEY_DATE_TIME_FORMAT);
    localTime = it != resultMap.end() ? it->second : "";
    auto [localDateTaken, detailTime] = PhotoFileUtils::ExtractTimeInfo(localTime, "%Y-%m-%d %H:%M:%S");

    if (dateTaken > 0) {
        if (detailTime.empty()) {
            detailTime = utcDetailTime;
        }
        SetTimeInfo(dateTaken, detailTime, data);
        MEDIA_INFO_LOG("forAdd:%{public}d, use utcTime:%{public}s, localTime:%{public}s",
            forAdd,
            utcTime.c_str(),
            localTime.c_str());
        return true;
    }

    if (localDateTaken > 0) {
        SetTimeInfo(localDateTaken, detailTime, data);
        MEDIA_WARN_LOG("forAdd:%{public}d, utcTime:%{public}s, use localTime:%{public}s",
            forAdd,
            utcTime.c_str(),
            localTime.c_str());
        return true;
    }

    return false;
}

void MetadataExtractor::PopulateVideoTimeInfo(const shared_ptr<Meta> &customMeta,
    const std::unordered_map<int32_t, std::string> &resultMap, std::unique_ptr<Metadata> &data)
{
    if (TrySetTimeInfoFromVideoMetadata(customMeta, resultMap, data)) {
        return;
    }

    int64_t dateTaken = data->GetDateTaken();
    string detailTime = data->GetDetailTime();
    bool forAdd = data->GetForAdd();
    if (dateTaken > 0 && !detailTime.empty() && !forAdd) {
        SetTimeInfo(dateTaken, detailTime, data);
        MEDIA_WARN_LOG("use old dateTaken:%{public}lld, detailTime:%{public}s",
            static_cast<long long>(dateTaken),
            detailTime.c_str());
        return;
    }
    if (dateTaken < MIN_MILSEC_TIMESTAMP) {
        dateTaken = data->GetFileDateAdded();
        int64_t dateModified = data->GetFileDateModified();
        if (dateModified >= MIN_MILSEC_TIMESTAMP) {
            dateTaken = dateTaken >= MIN_MILSEC_TIMESTAMP ? std::min(dateTaken, dateModified) : dateModified;
        }
        if (dateTaken < MIN_MILSEC_TIMESTAMP) {
            dateTaken = MediaFileUtils::UTCTimeMilliSeconds();
        }
    }
    const auto [parseDateTaken, parseDetailTime] =
        PhotoFileUtils::ExtractTimeInfo(detailTime, PhotoColumn::PHOTO_DETAIL_TIME_FORMAT);
    if (parseDateTaken < MIN_MILSEC_TIMESTAMP || parseDateTaken > MAX_MILSEC_TIMESTAMP ||
        abs(dateTaken - parseDateTaken) > MAX_TIMESTAMP_DIFF) {
        detailTime = MediaFileUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DETAIL_TIME_FORMAT, dateTaken);
    } else {
        detailTime = parseDetailTime;
    }
    SetTimeInfo(dateTaken, detailTime, data);
    MEDIA_WARN_LOG(
        "forAdd:%{public}d, video metadata was invalid when setting dateTaken:%{public}lld, detailTime:%{public}s",
        forAdd,
        static_cast<long long>(dateTaken),
        detailTime.c_str());
}

static void FillSlowMotionMetadata(std::unique_ptr<Metadata> &data)
{
    if (data->GetShootingMode() == std::to_string(static_cast<int>(ShootingModeValue::SLOW_MOTION_SHOOTING_MODE))) {
        MEDIA_DEBUG_LOG("shoot mode type is SlowMotion");
        data->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::SLOW_MOTION_VIDEO));
    }
}

void PopulateExtractedAVMetadataTwo(
    const std::unordered_map<int32_t, std::string> &resultMap, std::unique_ptr<Metadata> &data)
{
    MEDIA_INFO_LOG("PopulateExtractedAVMetadataTwo start");
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
        FillSlowMotionMetadata(data);
    }
    strTemp = resultMap.at(AV_KEY_VIDEO_IS_HDR_VIVID);
    const string isHdr = "yes";
    if (strcmp(strTemp.c_str(), isHdr.c_str()) == 0) {
        data->SetDynamicRangeType(static_cast<int32_t>(DynamicRangeType::HDR));
    } else {
        data->SetDynamicRangeType(static_cast<int32_t>(DynamicRangeType::SDR));
    }

    strTemp = resultMap.at(AV_KEY_GLTF_OFFSET);
    if ((!strTemp.empty()) && (strTemp != "-1")) {
        MEDIA_DEBUG_LOG("PopulateExtractedAVMetadataTwo get AV_KEY_GLTF_OFFSET success");
        data->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::SPATIAL_3DGS));
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

static void ParseMovingPhotoCoverPosition(
    const shared_ptr<Meta> &customMeta, std::shared_ptr<Meta> &meta, std::unique_ptr<Metadata> &data)
{
    if (customMeta == nullptr) {
        MEDIA_INFO_LOG("Video of moving photo does not contain customInfo");
        return ParseLivePhotoCoverPosition(data);
    }

    float coverPosition = 0.0f;
    bool isValid = customMeta->GetData(PHOTO_DATA_VIDEO_COVER_TIME, coverPosition);
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
    shared_ptr<Meta> customMeta = make_shared<Meta>();
    bool isValid = meta->GetData(PHOTO_DATA_VIDEO_CUSTOM_INFO, customMeta);
    if (!isValid) {
        MEDIA_WARN_LOG("Unable to retrieve customInfo from Meta");
        customMeta = nullptr;
    }
    PopulateVideoTimeInfo(customMeta, resultMap, data);
    PopulateExtractedAVLocationMeta(meta, data);

    int64_t timeNow = MediaFileUtils::UTCTimeMilliSeconds();
    data->SetLastVisitTime(timeNow);

    if (IsMovingPhoto(data)) {
        ParseMovingPhotoCoverPosition(customMeta, meta, data);
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

static bool CanConvertToInt32(const std::string &str)
{
    std::istringstream iss(str);
    int32_t num = 0;
    iss >> num;
    return iss.eof() && !iss.fail();
}
 
static int32_t GetTransfertype(const std::string transfertypeStr)
{
    int32_t transfertype = 0;
    if (CanConvertToInt32(transfertypeStr)) {
        transfertype = static_cast<int32_t>(std::stoi(transfertypeStr));
    }
    return transfertype;
}

int32_t MetadataExtractor::ExtractAVLogMetadata(std::shared_ptr<Meta> &meta)
{
    int32_t videoMode = 0;
    static const int huaweiTransfertype = 2;
    CHECK_AND_RETURN_RET_LOG(meta != nullptr, E_ERR, "meta is nullptr");
    Meta logMeta = *meta;
    auto iter = logMeta.Find("transfer_characteristics");
    if (iter == logMeta.end()) {
        return videoMode;
    }
    string transfertypeStr;
    logMeta.GetData("transfer_characteristics", transfertypeStr);
    MEDIA_INFO_LOG("transfertype =%{public}s", transfertypeStr.c_str());
    int32_t transfertype = GetTransfertype(transfertypeStr);
    if (transfertype == huaweiTransfertype) {
        auto iterHw = logMeta.Find("customInfo");
        if (iterHw == logMeta.end()) {
            return videoMode;
        }
        shared_ptr<Meta> customInfoMeta;
        logMeta.GetData("customInfo", customInfoMeta);
        Meta customInfo = *customInfoMeta;
        auto iterCus = customInfo.Find("com.openharmony.video.sei.h_log");
        if (iterCus != customInfo.end()) {
            videoMode = static_cast<int32_t>(VideoMode::LOG_VIDEO);
        }
    }
    MEDIA_INFO_LOG("ExtractAVLogMetadata videoMode=%{public}d", videoMode);
    return videoMode;
}
 
void MetadataExtractor::ExtractVideoMode(int32_t fileId, std::unique_ptr<Metadata> &data, std::shared_ptr<Meta> &meta)
{
    const int32_t NOT_EXTRAC = -1;
    CHECK_AND_RETURN_LOG(fileId != 0, "AV metadata fileId is 0");
    int32_t videoMode = data->GetVideoMode();
    MEDIA_INFO_LOG("ExtractAVMetadata videoMode = %{public}d", videoMode);
    if (videoMode != static_cast<int32_t>(VideoMode::DEFAULT)) {
        MEDIA_INFO_LOG("video has scannered");
        return;
    }
    int32_t extVideoMode = ExtractAVLogMetadata(meta);
    data->SetVideoMode(extVideoMode);
    MEDIA_INFO_LOG("ExtractVideoMode extVideoMode = %{public}d", extVideoMode);
}

int32_t MetadataExtractor::BuildMetaData(
    std::shared_ptr<AVMetadataHelper> &avMetadataHelper, std::unique_ptr<Metadata> &data)
{
    MediaLibraryTracer tracer;
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
    } else {
        MEDIA_ERR_LOG("resultMap is empty, file path: %{private}s", MediaFileUtils::DesensitizePath(filePath).c_str());
    }
    int32_t fileId = data->GetFileId();
    if (fileId != FILE_ID_DEFAULT) {
        ExtractVideoMode(fileId, data, meta);
        (void)close(fd);
        return E_OK;
    }
    int32_t extVideoMode = ExtractAVLogMetadata(meta);
    data->SetVideoMode(extVideoMode);
    (void)close(fd);
    return E_OK;
}

int32_t MetadataExtractor::ExtractAVMetadata(std::unique_ptr<Metadata> &data, int32_t scene)
{
    MediaLibraryTracer tracer;

    tracer.Start("CreateAVMetadataHelper");
    std::shared_ptr<AVMetadataHelper> avMetadataHelper = AVMetadataHelperFactory::CreateAVMetadataHelper();
    tracer.Finish();
    CHECK_AND_RETURN_RET_LOG(avMetadataHelper != nullptr, E_AVMETADATA, "AV metadata helper is null");

    // notify media_service clone event.
    if (scene == Scene::AV_META_SCENE_CLONE) {
        avMetadataHelper->SetScene(static_cast<Scene>(scene));
    }
    return BuildMetaData(avMetadataHelper, data);
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
        data->SetVideoMode(0);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to extract image metadata");
        if (IsMovingPhoto(data)) {
            data->SetVideoMode(0);
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

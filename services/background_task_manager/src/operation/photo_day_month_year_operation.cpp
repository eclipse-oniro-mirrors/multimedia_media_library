/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define MLOG_TAG "PhotoDayMonthYearOperation"

#include "photo_day_month_year_operation.h"

#include <charconv>

#include "media_exif.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_unistore_manager.h"
#include "media_column.h"
#include "medialibrary_subscriber.h"
#include "photo_file_utils.h"
#include "preferences_helper.h"
#include "result_set_utils.h"
#include "thumbnail_service.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace NativeRdb;

struct UpdatePhoto {
    int64_t dateTaken{0};
    std::string detailTime;
    std::string dateYear;
    std::string dateMonth;
    std::string dateDay;
};

const std::string REPAIR_DATE_TIME_XML = "/data/storage/el2/base/preferences/repair_date_time.xml";
const std::string CURRENT_VERSION = "CURRENT_VERSION";
const int32_t RESCAN_VERSION = 3;

const std::string CURRENT_FILE_ID = "CURRENT_FILE_ID";
const std::string ZEROTIMESTRING = "0000:00:00 00:00:00";
const std::string DAY_19700101 = "19700101";

const std::int32_t BATCH_SIZE = 500;
const int32_t UPDATE_BATCH_SIZE = 200;

const int32_t HOURS_TO_SECOND = 3600;
const int32_t MINUTES_TO_SECOND = 60;
const size_t OFFSET_STR_SIZE = 6;  // ±HH:MM
const size_t COLON_POSITION = 3;

std::mutex PhotoDayMonthYearOperation::mutex_;

const std::string QUERY_NEED_UPDATE_FILE_IDS = ""
    "SELECT file_id FROM Photos "
    "WHERE"
    "  date_added = 0"
    "  OR date_taken = 0";

const std::string UPDATE_DAY_MONTH_YEAR = ""
    "UPDATE Photos "
    "SET date_added ="
    " CASE"
    "  WHEN date_added <> 0 THEN"
    "  date_added "
    "  WHEN date_taken <> 0 THEN"
    "  date_taken "
    "  WHEN date_modified <> 0 THEN"
    "  date_modified ELSE strftime( '%s', 'now' ) "
    " END, "
    "date_taken ="
    " CASE"
    "  WHEN date_taken <> 0 THEN"
    "  date_taken "
    "  WHEN date_added <> 0 THEN"
    "  date_added "
    "  WHEN date_modified <> 0 THEN"
    "  date_modified ELSE strftime( '%s', 'now' ) "
    " END, "
    "date_day ="
    " CASE"
    "  WHEN date_taken <> 0 THEN"
    "  strftime( '%Y%m%d', date_taken / 1000, 'unixepoch', 'localtime' ) "
    "  WHEN date_added <> 0 THEN"
    "  strftime( '%Y%m%d', date_added / 1000, 'unixepoch', 'localtime' ) "
    "  WHEN date_modified <> 0 THEN"
    "  strftime( '%Y%m%d', date_modified / 1000, 'unixepoch', 'localtime' ) "
    "  ELSE strftime( '%Y%m%d', strftime( '%s', 'now' ) / 1000, 'unixepoch', 'localtime' ) "
    " END, "
    "date_month ="
    " CASE"
    "  WHEN date_taken <> 0 THEN"
    "  strftime( '%Y%m', date_taken / 1000, 'unixepoch', 'localtime' ) "
    "  WHEN date_added <> 0 THEN"
    "  strftime( '%Y%m', date_added / 1000, 'unixepoch', 'localtime' ) "
    "  WHEN date_modified <> 0 THEN"
    "  strftime( '%Y%m', date_modified / 1000, 'unixepoch', 'localtime' ) "
    "  ELSE strftime( '%Y%m', strftime( '%s', 'now' ) / 1000, 'unixepoch', 'localtime' ) "
    " END, "
    "date_year ="
    " CASE"
    "  WHEN date_taken <> 0 THEN"
    "  strftime( '%Y', date_taken / 1000, 'unixepoch', 'localtime' ) "
    "  WHEN date_added <> 0 THEN"
    "  strftime( '%Y', date_added / 1000, 'unixepoch', 'localtime' ) "
    "  WHEN date_modified <> 0 THEN"
    "  strftime( '%Y', date_modified / 1000, 'unixepoch', 'localtime' ) "
    "  ELSE strftime( '%Y', strftime( '%s', 'now' ) / 1000, 'unixepoch', 'localtime' ) "
    " END, "
    "detail_time ="
    " CASE"
    "  WHEN date_taken <> 0 THEN"
    "  strftime( '%Y:%m:%d %H:%M:%S', date_taken / 1000, 'unixepoch', 'localtime' ) "
    "  WHEN date_added <> 0 THEN"
    "  strftime( '%Y:%m:%d %H:%M:%S', date_added / 1000, 'unixepoch', 'localtime' ) "
    "  WHEN date_modified <> 0 THEN"
    "  strftime( '%Y:%m:%d %H:%M:%S', date_modified / 1000, 'unixepoch', 'localtime' ) "
    "  ELSE strftime( '%Y:%m:%d %H:%M:%S', strftime( '%s', 'now' ) / 1000, 'unixepoch', 'localtime' ) "
    " END, "
    "dirty ="
    " CASE"
    "  WHEN dirty = 0 THEN"
    "  2 ELSE dirty "
    " END "
    "WHERE"
    "  file_id IN ( ";
// LCOV_EXCL_START
int32_t PhotoDayMonthYearOperation::UpdatePhotosDate(const std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    MEDIA_INFO_LOG("update photos date start");
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    bool cond = (rdbStore == nullptr || !rdbStore->CheckRdbStore());
    CHECK_AND_RETURN_RET_LOG(!cond, NativeRdb::E_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    auto resultSet = rdbStore->QueryByStep(QUERY_NEED_UPDATE_FILE_IDS);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, NativeRdb::E_ERROR, "query photos by step failed");

    std::vector<std::string> needUpdateFileIds;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        needUpdateFileIds.push_back(GetStringVal(PhotoColumn::MEDIA_ID, resultSet));
    }
    resultSet->Close();
    auto needChangedSize = needUpdateFileIds.size();
    CHECK_AND_RETURN_RET(needChangedSize > 0, NativeRdb::E_OK);

    int32_t ret = NativeRdb::E_OK;
    int64_t totalChanged = 0;
    for (size_t start = 0; start < needChangedSize; start += UPDATE_BATCH_SIZE) {
        size_t end = std::min(start + UPDATE_BATCH_SIZE, needChangedSize);
        std::stringstream updateSql;
        updateSql << UPDATE_DAY_MONTH_YEAR;
        for (size_t i = start; i < end; ++i) {
            if (i != start) {
                updateSql << ", ";
            }
            updateSql << needUpdateFileIds[i];
        }
        updateSql << " );";
        int64_t batchStart = MediaFileUtils::UTCTimeMilliSeconds();
        int64_t changedRowCount = 0;
        auto errCode = rdbStore->ExecuteForChangedRowCount(changedRowCount, updateSql.str());
        if (errCode != NativeRdb::E_OK) {
            ret = errCode;
            MEDIA_ERR_LOG("update photos date failed, errCode: %{public}d, batchStart: %{public}" PRId64
                ", cost: %{public}" PRId64,
                errCode, batchStart, MediaFileUtils::UTCTimeMilliSeconds() - batchStart);
        } else {
            totalChanged += changedRowCount;
            MEDIA_DEBUG_LOG("update photos date, batchStart: %{public}" PRId64 ", cost: %{public}" PRId64
                ", changedRowCount: %{public}" PRId64,
                batchStart, MediaFileUtils::UTCTimeMilliSeconds() - batchStart, changedRowCount);
        }
    }

    MEDIA_INFO_LOG("update photos date end, startTime: %{public}" PRId64 ", cost: %{public}" PRId64
        ", needChangedSize: %{public}zu, totalChanged: %{public}" PRId64,
        startTime, MediaFileUtils::UTCTimeMilliSeconds() - startTime, needChangedSize, totalChanged);
    return ret;
}

int32_t PhotoDayMonthYearOperation::UpdatePhotosDateAndIdx(const std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    MEDIA_INFO_LOG("update phots date start");
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    bool cond = (rdbStore == nullptr || !rdbStore->CheckRdbStore());
    CHECK_AND_RETURN_RET_LOG(!cond, NativeRdb::E_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");

    auto ret = UpdatePhotosDate(rdbStore);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret, "update day month year failed, ret=%{public}d", ret);
    MEDIA_INFO_LOG("update phots date end, startTime: %{public}" PRId64 ", cost: %{public}" PRId64, startTime,
        (MediaFileUtils::UTCTimeMilliSeconds() - startTime));
    return NativeRdb::E_OK;
}

int32_t PhotoDayMonthYearOperation::UpdatePhotosDate(NativeRdb::RdbStore &rdbStore)
{
    MEDIA_INFO_LOG("update photos date start");
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();

    auto resultSet = rdbStore.QueryByStep(QUERY_NEED_UPDATE_FILE_IDS);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, NativeRdb::E_ERROR, "query photos by step failed");

    std::vector<std::string> needUpdateFileIds;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        needUpdateFileIds.push_back(GetStringVal(PhotoColumn::MEDIA_ID, resultSet));
    }
    resultSet->Close();
    auto needChangedSize = needUpdateFileIds.size();
    CHECK_AND_RETURN_RET(needChangedSize > 0, NativeRdb::E_OK);

    std::stringstream updateSql;
    updateSql << UPDATE_DAY_MONTH_YEAR;
    for (size_t i = 0; i < needChangedSize; ++i) {
        if (i != 0) {
            updateSql << ", ";
        }
        updateSql << needUpdateFileIds[i];
    }
    updateSql << " );";
    int64_t changedRowCount = 0;
    auto errCode = rdbStore.ExecuteForChangedRowCount(changedRowCount, updateSql.str());
    CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK, errCode,
        "update photos date failed, errCode: %{public}d, startTime: %{public}" PRId64
        ", cost: %{public}" PRId64 ", needChangedSize: %{public}zu",
        errCode, startTime, MediaFileUtils::UTCTimeMilliSeconds() - startTime, needChangedSize);

    MEDIA_INFO_LOG("update photos date end, startTime: %{public}" PRId64 ", cost: %{public}" PRId64
        ", needChangedSize: %{public}zu, changedRowCount: %{public}" PRId64,
        startTime, MediaFileUtils::UTCTimeMilliSeconds() - startTime, needChangedSize, changedRowCount);
    return NativeRdb::E_OK;
}

int32_t PhotoDayMonthYearOperation::UpdatePhotosDateIdx(const std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    MEDIA_INFO_LOG("update photos date idx start");
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    bool cond = (rdbStore == nullptr || !rdbStore->CheckRdbStore());
    CHECK_AND_RETURN_RET_LOG(!cond, NativeRdb::E_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");

    auto ret = rdbStore->ExecuteSql(PhotoColumn::DROP_SCHPT_DAY_INDEX);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "drop idx date_day failed, ret=%{public}d", ret);

    ret = rdbStore->ExecuteSql(PhotoColumn::CREATE_SCHPT_DAY_INDEX);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "create idx date_day failed, ret=%{public}d", ret);

    ret = rdbStore->ExecuteSql(PhotoColumn::DROP_SCHPT_MONTH_COUNT_READY_INDEX);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "drop idx date_month failed, ret=%{public}d", ret);

    ret = rdbStore->ExecuteSql(PhotoColumn::CREATE_SCHPT_MONTH_COUNT_READY_INDEX);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "create idx date_month failed, ret=%{public}d", ret);

    ret = rdbStore->ExecuteSql(PhotoColumn::DROP_SCHPT_YEAR_COUNT_READY_INDEX);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "drop idx date_year failed, ret=%{public}d", ret);

    ret = rdbStore->ExecuteSql(PhotoColumn::CREATE_SCHPT_YEAR_COUNT_READY_INDEX);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret, "create idx date_year failed, ret=%{public}d", ret);

    MEDIA_INFO_LOG("update photos date idx end, startTime: %{public}" PRId64 ", cost: %{public}" PRId64, startTime,
        (MediaFileUtils::UTCTimeMilliSeconds() - startTime));
    return NativeRdb::E_OK;
}
// LCOV_EXCL_STOP

std::vector<DateAnomalyPhoto> PhotoDayMonthYearOperation::QueryDateAnomalyPhotos(const int32_t startFileId)
{
    std::vector<DateAnomalyPhoto> photos;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, photos, "Failed to get rdbstore!");

    const std::vector<NativeRdb::ValueObject> bindArgs = {startFileId, BATCH_SIZE};
    const std::string sql =
        "SELECT"
        "  file_id, dirty, date_taken, date_modified, date_added, "
        "  date_year, date_month, date_day, detail_time, all_exif "
        "FROM"
        "  Photos "
        "WHERE"
        "  file_id > ? AND sync_status = 0 AND clean_flag = 0 AND time_pending = 0 AND is_temp = 0 "
        "  AND ("
        "    date_taken < " +
        to_string(MIN_MILSEC_TIMESTAMP) + " OR date_taken > " + to_string(MAX_MILSEC_TIMESTAMP) +
        "    OR date_modified < " + to_string(MIN_MILSEC_TIMESTAMP) +
        "    OR date_modified > " + to_string(MAX_MILSEC_TIMESTAMP) +
        "    OR date_added < " + to_string(MIN_MILSEC_TIMESTAMP) +
        "    OR date_added > " + to_string(MAX_MILSEC_TIMESTAMP) +
        "    OR date_day IS NULL OR date_day = '' OR date_day = '19700101' "
        "    OR detail_time IS NULL OR detail_time = '' "
        "    OR date_day <> REPLACE ( SUBSTR( detail_time, 1, 10 ), ':', '' ) "
        "    OR detail_time <> strftime( '%Y:%m:%d %H:%M:%S', date_taken / 1000, 'unixepoch', 'localtime' ) "
        "  ) "
        "ORDER BY"
        "  file_id ASC "
        "  LIMIT ?;";
    auto resultSet = rdbStore->QuerySql(sql, bindArgs);
    bool cond = resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK;
    CHECK_AND_RETURN_RET_LOG(cond, photos, "resultSet is null or count is 0");

    do {
        DateAnomalyPhoto photo;
        photo.fileId = GetInt32Val(PhotoColumn::MEDIA_ID, resultSet);
        photo.dirty = GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet);
        photo.dateTaken = GetInt64Val(PhotoColumn::MEDIA_DATE_TAKEN, resultSet);
        photo.dateModified = GetInt64Val(PhotoColumn::MEDIA_DATE_MODIFIED, resultSet);
        photo.dateAdded = GetInt64Val(PhotoColumn::MEDIA_DATE_ADDED, resultSet);
        photo.dateYear = GetStringVal(PhotoColumn::PHOTO_DATE_YEAR, resultSet);
        photo.dateMonth = GetStringVal(PhotoColumn::PHOTO_DATE_MONTH, resultSet);
        photo.dateDay = GetStringVal(PhotoColumn::PHOTO_DATE_DAY, resultSet);
        photo.detailTime = GetStringVal(PhotoColumn::PHOTO_DETAIL_TIME, resultSet);
        photo.exif = GetStringVal(PhotoColumn::PHOTO_ALL_EXIF, resultSet);
        photos.push_back(photo);
    } while (MedialibrarySubscriber::IsCurrentStatusOn() && resultSet->GoToNextRow() == NativeRdb::E_OK);
    resultSet->Close();
    return photos;
}

static int32_t OffsetTimeToSeconds(const std::string &offsetStr, int32_t &offsetTime)
{
    if (offsetStr.size() != OFFSET_STR_SIZE) {
        MEDIA_WARN_LOG("Invalid offsetStr length: %{public}s", offsetStr.c_str());
        return E_ERR;
    }
    const char sign = offsetStr[0];
    if (sign != '+' && sign != '-') {
        MEDIA_WARN_LOG("Invalid sign character: %{public}s", offsetStr.c_str());
        return E_ERR;
    }
    if (offsetStr[COLON_POSITION] != ':') {
        MEDIA_WARN_LOG("Missing colon at position: %{public}s", offsetStr.c_str());
        return E_ERR;
    }
    int hours = 0;
    const size_t endHoursIndex = 3;
    std::from_chars_result result = std::from_chars(&offsetStr[1], &offsetStr[endHoursIndex], hours);
    if (result.ec != std::errc() || result.ptr != &offsetStr[endHoursIndex]) {
        MEDIA_WARN_LOG("Invalid offsetStr: %{public}s", offsetStr.c_str());
        return E_ERR;
    }
    int minutes = 0;
    const size_t startMinutesIndex = 4;
    const size_t endMinutesIndex = 6;
    result = std::from_chars(&offsetStr[startMinutesIndex], &offsetStr[endMinutesIndex], minutes);
    if (result.ec != std::errc() || result.ptr != &offsetStr[endMinutesIndex]) {
        MEDIA_WARN_LOG("Invalid offsetStr: %{public}s", offsetStr.c_str());
        return E_ERR;
    }
    const int maxHours = 23;
    if (hours < 0 || hours > maxHours) {
        MEDIA_WARN_LOG("Hours out of range: %{public}s", offsetStr.c_str());
        return E_ERR;
    }
    const int maxMinutes = 59;
    if (minutes < 0 || minutes > maxMinutes) {
        MEDIA_WARN_LOG("Minutes out of range: %{public}s", offsetStr.c_str());
        return E_ERR;
    }
    const int totalSeconds = hours * HOURS_TO_SECOND + minutes * MINUTES_TO_SECOND;
    offsetTime = (sign == '-') ? totalSeconds : -totalSeconds;
    MEDIA_DEBUG_LOG("Offset conversion successful: %{public}s -> %{public}d seconds", offsetStr.c_str(), offsetTime);
    return E_OK;
}

static std::string JsonSafeGetString(const nlohmann::json &json, const std::string &key)
{
    auto it = json.find(key);
    if (it == json.end()) {
        return "";
    }
    return it->is_string() ? it->get<std::string>() : "";
}

static void SetSubSecondTime(const nlohmann::json &exifJson, int64_t &timeStamp)
{
    string subTimeStr = JsonSafeGetString(exifJson, PHOTO_DATA_IMAGE_SUBSEC_TIME_ORIGINAL);
    if (subTimeStr.empty()) {
        return;
    }

    const size_t millisecondPrecision = 3;
    const size_t subTimeSize = std::min(millisecondPrecision, subTimeStr.size());
    int32_t subTime = 0;
    auto [ptr, ec] = std::from_chars(subTimeStr.data(), subTimeStr.data() + subTimeSize, subTime);
    if (ec == std::errc() && ptr == subTimeStr.data() + subTimeSize) {
        MEDIA_DEBUG_LOG("subTime:%{public}d from exif", subTime);
        timeStamp += subTime;
    } else {
        MEDIA_WARN_LOG("Invalid subTime format:%{public}s", subTimeStr.c_str());
    }
}

static std::tuple<int64_t, std::string> GetShootingTimeByOriginal(const nlohmann::json &exifJson)
{
    int64_t dateTaken = 0;
    string detailTime;
    string timeString = JsonSafeGetString(exifJson, PHOTO_DATA_IMAGE_DATE_TIME_ORIGINAL);
    if (timeString.empty() || timeString == ZEROTIMESTRING) {
        return {dateTaken, detailTime};
    }
    string offsetString = JsonSafeGetString(exifJson, PHOTO_DATA_IMAGE_OFFSET_TIME_ORIGINAL);
    int32_t offsetTime = 0;
    if (!offsetString.empty() && OffsetTimeToSeconds(offsetString, offsetTime) == E_OK) {
        std::tie(dateTaken, detailTime) =
            PhotoFileUtils::ExtractTimeInfo(timeString, PhotoColumn::PHOTO_DETAIL_TIME_FORMAT, true);
        dateTaken += offsetTime * SEC_TO_MSEC;
        MEDIA_DEBUG_LOG("Get dateTaken from DateTimeOriginal and OffsetTimeOriginal in exif");
    } else {
        std::tie(dateTaken, detailTime) =
            PhotoFileUtils::ExtractTimeInfo(timeString, PhotoColumn::PHOTO_DETAIL_TIME_FORMAT);
        MEDIA_DEBUG_LOG("Get dateTaken from DateTimeOriginal in exif");
    }
    if (dateTaken > 0) {
        SetSubSecondTime(exifJson, dateTaken);
        MEDIA_DEBUG_LOG("OriginalTimeStamp:%{public}lld in exif", static_cast<long long>(dateTaken));
    }
    return {dateTaken, detailTime};
}

static std::tuple<int64_t, std::string> GetShootingTimeByGPS(const nlohmann::json &exifJson)
{
    string dateStr = JsonSafeGetString(exifJson, PHOTO_DATA_IMAGE_GPS_DATE_STAMP);
    string timeStr = JsonSafeGetString(exifJson, PHOTO_DATA_IMAGE_GPS_TIME_STAMP);
    if (dateStr.empty() || timeStr.empty()) {
        return {0, ""};
    }
    string dateTime = dateStr + " " + timeStr;
    if (dateTime == ZEROTIMESTRING) {
        return {0, ""};
    }
    auto [dateTaken, detailTime] =
        PhotoFileUtils::ExtractTimeInfo(dateTime, PhotoColumn::PHOTO_DETAIL_TIME_FORMAT, true);
    if (dateTaken > 0) {
        SetSubSecondTime(exifJson, dateTaken);
        MEDIA_DEBUG_LOG("GPSTimeStamp:%{public}lld in exif", static_cast<long long>(dateTaken));
        return {dateTaken, detailTime};
    }
    return {0, ""};
}

static std::tuple<int64_t, std::string> ExtractShootingTime(const nlohmann::json &exifJson)
{
    auto [dateTaken, detailTime] = GetShootingTimeByOriginal(exifJson);
    if (dateTaken > 0) {
        return {dateTaken, detailTime};
    }
    std::tie(dateTaken, detailTime) = GetShootingTimeByGPS(exifJson);
    if (dateTaken > 0) {
        return {dateTaken, detailTime};
    }
    MEDIA_ERR_LOG("invalid dateTaken: %{public}lld", static_cast<long long>(dateTaken));
    return {0, ""};
}

static UpdatePhoto ExtractDateTime(const std::string &exif)
{
    if (exif.empty() || !nlohmann::json::accept(exif)) {
        return {};
    }
    nlohmann::json exifJson = nlohmann::json::parse(exif, nullptr, false);
    auto const [dateTaken, detailTime] = ExtractShootingTime(exifJson);
    if (dateTaken <= 0) {
        return {};
    }
    auto const [dateYear, dateMonth, dateDay] = PhotoFileUtils::ExtractYearMonthDay(detailTime);
    if (dateDay.empty() || dateDay == DAY_19700101) {
        return {};
    }
    return {dateTaken, detailTime, dateYear, dateMonth, dateDay};
}

template <typename T>
static void PutIfChanged(ValuesBucket &values, const std::string &column, const T &newValue, const T &oldValue)
{
    if (newValue != oldValue) {
        values.Put(column, newValue);
    }
}

static void UpdatePhotoDetails(
    const std::shared_ptr<MediaLibraryRdbStore> rdbStore, const DateAnomalyPhoto &photo, const UpdatePhoto &updatePhoto)
{
    ValuesBucket values;
    if (abs(photo.dateTaken - updatePhoto.dateTaken) >= MIN_TIMESTAMP_DIFF) {
        values.Put(PhotoColumn::MEDIA_DATE_TAKEN, updatePhoto.dateTaken);
    }
    PutIfChanged(values, PhotoColumn::PHOTO_DETAIL_TIME, updatePhoto.detailTime, photo.detailTime);
    PutIfChanged(values, PhotoColumn::PHOTO_DATE_YEAR, updatePhoto.dateYear, photo.dateYear);
    PutIfChanged(values, PhotoColumn::PHOTO_DATE_MONTH, updatePhoto.dateMonth, photo.dateMonth);
    PutIfChanged(values, PhotoColumn::PHOTO_DATE_DAY, updatePhoto.dateDay, photo.dateDay);

    const int64_t dateAdded =
        PhotoFileUtils::NormalizeTimestamp(photo.dateAdded, MediaFileUtils::UTCTimeMilliSeconds());
    PutIfChanged(values, PhotoColumn::MEDIA_DATE_ADDED, dateAdded, photo.dateAdded);

    const int64_t dateModified = PhotoFileUtils::NormalizeTimestamp(photo.dateModified, dateAdded);
    PutIfChanged(values, PhotoColumn::MEDIA_DATE_MODIFIED, dateModified, photo.dateModified);
    if (values.IsEmpty()) {
        MEDIA_DEBUG_LOG("no update required, file_id=%{public}d", photo.fileId);
        return;
    }
    if (photo.dirty == static_cast<int32_t>(DirtyTypes::TYPE_SYNCED)) {
        values.Put(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyTypes::TYPE_MDIRTY));
    }
    int32_t updateCount = 0;
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, photo.fileId);
    int32_t err = rdbStore->Update(updateCount, values, predicates);
    MEDIA_INFO_LOG("update succeed, fileId:%{public}d, Count:%{public}d, err:%{public}d, "
                   "detailTime:%{public}s->%{public}s, dateTaken:%{public}lld->%{public}lld, "
                   "dateModified:%{public}lld->%{public}lld, dateAdded:%{public}lld->%{public}lld",
        photo.fileId,
        updateCount,
        err,
        photo.detailTime.c_str(),
        updatePhoto.detailTime.c_str(),
        static_cast<long long>(photo.dateTaken),
        static_cast<long long>(updatePhoto.dateTaken),
        static_cast<long long>(photo.dateModified),
        static_cast<long long>(dateModified),
        static_cast<long long>(photo.dateAdded),
        static_cast<long long>(dateAdded));
    if (err == NativeRdb::E_OK && updatePhoto.dateTaken != photo.dateTaken) {
        ThumbnailService::GetInstance()->UpdateAstcWithNewDateTaken(
            to_string(photo.fileId), to_string(updatePhoto.dateTaken), to_string(photo.dateTaken));
    }
}

static void HandleDateAnomalyPhotos(const std::shared_ptr<MediaLibraryRdbStore> rdbStore, const DateAnomalyPhoto &photo)
{
    auto updatePhoto = ExtractDateTime(photo.exif);
    if (updatePhoto.dateTaken > 0) {
        UpdatePhotoDetails(rdbStore, photo, updatePhoto);
        return;
    }

    updatePhoto.dateTaken = photo.dateTaken;
    if (updatePhoto.dateTaken < MIN_MILSEC_TIMESTAMP) {
        int64_t dateAdded = photo.dateAdded;
        while (dateAdded > MAX_MILSEC_TIMESTAMP) {
            dateAdded /= TIMESTAMP_CONVERSION_FACTOR;
        }
        if (dateAdded >= MIN_MILSEC_TIMESTAMP) {
            updatePhoto.dateTaken = dateAdded;
        }

        int64_t dateModified = photo.dateModified;
        while (dateModified > MAX_MILSEC_TIMESTAMP) {
            dateModified /= TIMESTAMP_CONVERSION_FACTOR;
        }
        if (dateModified >= MIN_MILSEC_TIMESTAMP) {
            updatePhoto.dateTaken =
                updatePhoto.dateTaken >= MIN_MILSEC_TIMESTAMP ? min(updatePhoto.dateTaken, dateModified) : dateModified;
        }
    }

    updatePhoto.dateTaken =
        PhotoFileUtils::NormalizeTimestamp(updatePhoto.dateTaken, MediaFileUtils::UTCTimeMilliSeconds());
    const auto [dateTaken, detailTime] =
        PhotoFileUtils::ExtractTimeInfo(photo.detailTime, PhotoColumn::PHOTO_DETAIL_TIME_FORMAT);
    if (dateTaken < MIN_MILSEC_TIMESTAMP || dateTaken > MAX_MILSEC_TIMESTAMP ||
        abs(dateTaken - updatePhoto.dateTaken) > MAX_TIMESTAMP_DIFF) {
        updatePhoto.detailTime =
            MediaFileUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DETAIL_TIME_FORMAT, updatePhoto.dateTaken);
    } else {
        updatePhoto.detailTime = detailTime;
    }

    std::tie(updatePhoto.dateYear, updatePhoto.dateMonth, updatePhoto.dateDay) =
        PhotoFileUtils::ExtractYearMonthDay(updatePhoto.detailTime);
    UpdatePhotoDetails(rdbStore, photo, updatePhoto);
}

void PhotoDayMonthYearOperation::RepairDateAnomalyPhotos(
    const std::vector<DateAnomalyPhoto> &photos, int32_t &curFileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is nullptr");
    for (const DateAnomalyPhoto &photo : photos) {
        CHECK_AND_BREAK_INFO_LOG(MedialibrarySubscriber::IsCurrentStatusOn(), "current status is off, break");
        curFileId = photo.fileId;
        HandleDateAnomalyPhotos(rdbStore, photo);
    }
}

int32_t PhotoDayMonthYearOperation::RepairDateTime()
{
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
    CHECK_AND_RETURN_RET_WARN_LOG(lock.try_lock(), E_OK, "Repair date time has started, skipping this operation");

    int32_t errCode = E_OK;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(REPAIR_DATE_TIME_XML, errCode);
    CHECK_AND_RETURN_RET_LOG(prefs, E_ERR, "get preferences error: %{public}d", errCode);

    int32_t curFileId = 0;
    int32_t currentVersion = prefs->GetInt(CURRENT_VERSION, 0);
    if (currentVersion < RESCAN_VERSION) {
        prefs->PutInt(CURRENT_FILE_ID, curFileId);
        prefs->PutInt(CURRENT_VERSION, RESCAN_VERSION);
        prefs->FlushSync();
    } else {
        curFileId = prefs->GetInt(CURRENT_FILE_ID, 0);
    }
    MEDIA_INFO_LOG("currentVersion:%{public}d, RESCAN_VERSION:%{public}d, start file_id:%{public}d",
        currentVersion,
        RESCAN_VERSION,
        curFileId);

    do {
        MEDIA_INFO_LOG("Repair date time curFileId: %{public}d", curFileId);
        std::vector<DateAnomalyPhoto> photos = QueryDateAnomalyPhotos(curFileId);
        CHECK_AND_BREAK_INFO_LOG(!photos.empty(), "has no anomaly photo to repair");
        RepairDateAnomalyPhotos(photos, curFileId);
    } while (MedialibrarySubscriber::IsCurrentStatusOn());

    prefs->PutInt(CURRENT_FILE_ID, curFileId);
    prefs->FlushSync();
    MEDIA_INFO_LOG("Repair date time end file id: %{public}d", curFileId);
    return E_OK;
}
} // namespace Media
} // namespace OHOS

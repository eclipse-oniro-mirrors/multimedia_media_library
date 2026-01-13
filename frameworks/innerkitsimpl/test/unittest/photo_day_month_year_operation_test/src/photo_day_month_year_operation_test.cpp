/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#define MLOG_TAG "PhotoDayMonthYearOperationTest"

#include "photo_day_month_year_operation_test.h"

#include <chrono>

#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "photo_album_column.h"
#include "photo_day_month_year_operation.h"
#include "photo_file_utils.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"
#include "media_upgrade.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace testing::ext;

shared_ptr<MediaLibraryRdbStore> g_rdbStore;
std::atomic<int> g_num{0};

const int32_t MILSEC_TO_MICSEC = 1000;

int32_t ExecSqls(const vector<string> &sqls)
{
    EXPECT_NE(g_rdbStore, nullptr);
    int32_t err = E_OK;
    for (const auto &sql : sqls) {
        err = g_rdbStore->ExecuteSql(sql);
        MEDIA_INFO_LOG("exec sql: %{public}s result: %{public}d", sql.c_str(), err);
        EXPECT_EQ(err, E_OK);
    }
    return E_OK;
}

void ClearTables()
{
    vector<string> createTableSqlList = {
        "DELETE FROM " + PhotoColumn::PHOTOS_TABLE,
        "DELETE FROM " + PhotoAlbumColumns::TABLE,
    };
    MEDIA_INFO_LOG("start clear data");
    ExecSqls(createTableSqlList);
}

inline void IncrementNum()
{
    ++g_num;
}

int64_t GetTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    IncrementNum();
    return seconds.count() + g_num.load();
}

string GetTitle(int64_t &timestamp)
{
    IncrementNum();
    return "IMG_" + to_string(timestamp) + "_" + to_string(g_num.load());
}

int64_t InsertPhoto(const MediaType &mediaType, int32_t position)
{
    EXPECT_NE(g_rdbStore, nullptr);
    int64_t timestamp = GetTimestamp();
    int64_t timestampMilliSecond = timestamp * SEC_TO_MSEC;
    string title = GetTitle(timestamp);
    string displayName = mediaType == MEDIA_TYPE_VIDEO ? (title + ".mp4") : (title + ".jpg");
    string path = "/storage/cloud/files/photo/16/" + displayName;
    int64_t videoSize = 1 * 1000 * 1000 * 1000;
    int64_t imageSize = 10 * 1000 * 1000;
    int32_t videoDuration = 2560;
    int32_t imageDuration = 0;
    int32_t videoWidth = 3072;
    int32_t imageWidth = 1920;
    int32_t videoHeight = 4096;
    int32_t imageHeight = 1080;
    string videoMimeType = "video/mp4";
    string imageMimeType = "image/jpeg";
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(MediaColumn::MEDIA_TYPE, mediaType);
    valuesBucket.PutInt(PhotoColumn::PHOTO_POSITION, position);
    valuesBucket.PutLong(MediaColumn::MEDIA_SIZE, mediaType == MEDIA_TYPE_VIDEO ? videoSize : imageSize);
    valuesBucket.PutInt(MediaColumn::MEDIA_DURATION, mediaType == MEDIA_TYPE_VIDEO ? videoDuration : imageDuration);
    valuesBucket.PutInt(PhotoColumn::PHOTO_WIDTH, mediaType == MEDIA_TYPE_VIDEO ? videoWidth : imageWidth);
    valuesBucket.PutInt(PhotoColumn::PHOTO_HEIGHT, mediaType == MEDIA_TYPE_VIDEO ? videoHeight : imageHeight);
    valuesBucket.PutString(MediaColumn::MEDIA_MIME_TYPE, mediaType == MEDIA_TYPE_VIDEO ? videoMimeType : imageMimeType);
    valuesBucket.PutString(MediaColumn::MEDIA_FILE_PATH, path);
    valuesBucket.PutString(MediaColumn::MEDIA_TITLE, title);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, displayName);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_ADDED, timestampMilliSecond);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, timestampMilliSecond);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_TAKEN, timestampMilliSecond);
    valuesBucket.PutLong(MediaColumn::MEDIA_TIME_PENDING, 0);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_TRASHED, 0);
    valuesBucket.PutInt(MediaColumn::MEDIA_HIDDEN, 0);
    valuesBucket.PutInt(MediaColumn::MEDIA_TIME_PENDING, 0);
    string detailTime =
        MediaFileUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DETAIL_TIME_FORMAT, timestampMilliSecond);
    auto const [dateYear, dateMonth, dateDay] = PhotoFileUtils::ExtractYearMonthDay(detailTime);
    valuesBucket.Put(PhotoColumn::PHOTO_DETAIL_TIME, detailTime);
    valuesBucket.Put(PhotoColumn::PHOTO_DATE_YEAR, dateYear);
    valuesBucket.Put(PhotoColumn::PHOTO_DATE_MONTH, dateMonth);
    valuesBucket.Put(PhotoColumn::PHOTO_DATE_DAY, dateDay);
    string exif = "{\"DateTimeOriginal\":\"" + detailTime + "\"}";
    valuesBucket.Put(PhotoColumn::PHOTO_ALL_EXIF, exif);
    int64_t fileId = -1;
    int32_t ret = g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
    return fileId;
}

int64_t InsertPhotoWithDateTime(
    const int64_t dateTaken, const string &detailTime, const string &dateDay, const string &exif)
{
    EXPECT_NE(g_rdbStore, nullptr);

    int64_t timestamp = GetTimestamp();
    int64_t timestampMilliSecond = timestamp * SEC_TO_MSEC;
    string title = GetTitle(timestamp);
    string displayName = title + ".jpg";
    string path = "/storage/cloud/files/photo/16/" + displayName;
    int64_t imageSize = 10 * 1000 * 1000;
    int32_t imageWidth = 1920;
    int32_t imageHeight = 1080;
    string imageMimeType = "image/jpeg";
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(MediaColumn::MEDIA_TYPE, MEDIA_TYPE_IMAGE);
    valuesBucket.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    valuesBucket.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL));
    valuesBucket.PutLong(MediaColumn::MEDIA_SIZE, imageSize);
    valuesBucket.PutInt(PhotoColumn::PHOTO_WIDTH, imageWidth);
    valuesBucket.PutInt(PhotoColumn::PHOTO_HEIGHT, imageHeight);
    valuesBucket.PutString(MediaColumn::MEDIA_MIME_TYPE, imageMimeType);
    valuesBucket.PutString(MediaColumn::MEDIA_FILE_PATH, path);
    valuesBucket.PutString(MediaColumn::MEDIA_TITLE, title);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, displayName);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_ADDED, timestampMilliSecond);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, timestampMilliSecond);
    valuesBucket.PutLong(MediaColumn::MEDIA_TIME_PENDING, 0);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_TRASHED, 0);
    valuesBucket.PutInt(MediaColumn::MEDIA_HIDDEN, 0);
    valuesBucket.PutInt(MediaColumn::MEDIA_TIME_PENDING, 0);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_TAKEN, dateTaken);
    valuesBucket.Put(PhotoColumn::PHOTO_DETAIL_TIME, detailTime);
    auto const [detailYear, detailMonth, detailDay] = PhotoFileUtils::ExtractYearMonthDay(detailTime);
    valuesBucket.Put(PhotoColumn::PHOTO_DATE_YEAR, detailYear);
    valuesBucket.Put(PhotoColumn::PHOTO_DATE_MONTH, detailMonth);
    valuesBucket.Put(PhotoColumn::PHOTO_DATE_DAY, dateDay);
    valuesBucket.Put(PhotoColumn::PHOTO_ALL_EXIF, exif);
    int64_t fileId = -1;
    int32_t ret = g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
    return fileId;
}

void PreparePhotos()
{
    int64_t fileId = InsertPhoto(MEDIA_TYPE_VIDEO, static_cast<int32_t>(PhotoPositionType::LOCAL));
    EXPECT_GT(fileId, 0);

    fileId = InsertPhoto(MEDIA_TYPE_IMAGE, static_cast<int32_t>(PhotoPositionType::LOCAL));
    EXPECT_GT(fileId, 0);

    fileId = InsertPhoto(MEDIA_TYPE_VIDEO, static_cast<int32_t>(PhotoPositionType::CLOUD));
    EXPECT_GT(fileId, 0);

    fileId = InsertPhoto(MEDIA_TYPE_IMAGE, static_cast<int32_t>(PhotoPositionType::CLOUD));
    EXPECT_GT(fileId, 0);

    fileId = InsertPhoto(MEDIA_TYPE_VIDEO, static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
    EXPECT_GT(fileId, 0);

    fileId = InsertPhoto(MEDIA_TYPE_IMAGE, static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
    EXPECT_GT(fileId, 0);
}

void PrepareAbnormalPhotos()
{
    int64_t dateTaken = MediaFileUtils::UTCTimeMilliSeconds();
    auto detailTime = MediaFileUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DETAIL_TIME_FORMAT, dateTaken);
    string exif = "{\"DateTimeOriginal\":\"" + detailTime + "\"}";
    auto [dateYear, dateMonth, dateDay] = PhotoFileUtils::ExtractYearMonthDay(detailTime);

    int64_t fileId = InsertPhotoWithDateTime(0, detailTime, dateDay, exif);
    EXPECT_GT(fileId, 0);

    fileId = InsertPhotoWithDateTime(-1, detailTime, dateDay, exif);
    EXPECT_GT(fileId, 0);

    fileId = InsertPhotoWithDateTime(dateTaken * MILSEC_TO_MICSEC, detailTime, dateDay, exif);
    EXPECT_GT(fileId, 0);

    fileId = InsertPhotoWithDateTime(dateTaken, "1970:01:01 08:00:00", dateDay, exif);
    EXPECT_GT(fileId, 0);

    fileId = InsertPhotoWithDateTime(dateTaken, detailTime, "19700101", exif);
    EXPECT_GT(fileId, 0);

    exif = "{\"DateTimeOriginal\":\"2020:08:08 00:08:53\",\"OffsetTimeOriginal\":\"+08:00\","
           "\"SubsecTimeOriginal\":\"120000\",\"GPSDateStamp\":\"\",\"GPSTimeStamp\":\"\"}";
    fileId = InsertPhotoWithDateTime(dateTaken, detailTime, dateDay, exif);
    EXPECT_GT(fileId, 0);

    exif = "{\"DateTimeOriginal\":\"2020:08:08 00:08:53\",\"OffsetTimeOriginal\":\"\","
           "\"SubsecTimeOriginal\":\"\",\"GPSDateStamp\":\"\",\"GPSTimeStamp\":\"\"}";
    fileId = InsertPhotoWithDateTime(dateTaken, detailTime, dateDay, exif);
    EXPECT_GT(fileId, 0);

    exif = "{\"DateTimeOriginal\":\"\",\"OffsetTimeOriginal\":\"\",\"SubsecTimeOriginal\":\"\","
           "\"GPSDateStamp\":\"2020:08:08\",\"GPSTimeStamp\":\"00:08:53.12\"}";
    fileId = InsertPhotoWithDateTime(dateTaken, detailTime, dateDay, exif);
    EXPECT_GT(fileId, 0);

    exif = "{\"DateTimeOriginal\":\"\",\"OffsetTimeOriginal\":\"\",\"SubsecTimeOriginal\":\"\","
           "\"GPSDateStamp\":\"\",\"GPSTimeStamp\":\"\"}";
    fileId = InsertPhotoWithDateTime(dateTaken, detailTime, dateDay, exif);
    EXPECT_GT(fileId, 0);
}

int32_t QueryAbnormalPhotosCount()
{
    EXPECT_NE(g_rdbStore, nullptr);

    const string sql = "SELECT"
                       "  COUNT( * ) AS count "
                       "FROM"
                       "  Photos "
                       "WHERE"
                       "  date_taken <= 0 "
                       "  OR date_day IS NULL "
                       "  OR date_day = '' "
                       "  OR date_day = '19700101' "
                       "  OR detail_time IS NULL "
                       "  OR detail_time = '' "
                       "  OR date_day != REPLACE ( SUBSTR( detail_time, 1, 10 ), ':', '' );";
    shared_ptr<NativeRdb::ResultSet> resultSet = g_rdbStore->QuerySql(sql);
    EXPECT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToFirstRow(), E_OK);

    int32_t count = GetInt32Val("count", resultSet);
    MEDIA_INFO_LOG("Abnormal Photos Count is %{public}d", count);
    return count;
}

int64_t UpdatePhotosDateDay()
{
    EXPECT_NE(g_rdbStore, nullptr);

    const string sql = "UPDATE Photos "
                       "SET date_day = '19700101';";

    int64_t changeRows = 0;
    auto errCode = g_rdbStore->ExecuteForChangedRowCount(changeRows, sql);
    EXPECT_EQ(errCode, E_OK);

    return changeRows;
}

void SetTables()
{
    vector<string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        PhotoAlbumColumns::CREATE_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        ASSERT_NE(g_rdbStore, nullptr);

        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        ASSERT_EQ(ret, NativeRdb::E_OK);
        MEDIA_INFO_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
}

void PhotoDayMonthYearOperationTest::SetUpTestCase()
{
    MEDIA_INFO_LOG("PhotoDayMonthYearOperationTest SetUpTestCase");

    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    SetTables();
}

void PhotoDayMonthYearOperationTest::TearDownTestCase()
{
    MEDIA_INFO_LOG("PhotoDayMonthYearOperationTest TearDownTestCase");
    ClearTables();
}

void PhotoDayMonthYearOperationTest::SetUp()
{
    MEDIA_INFO_LOG("PhotoDayMonthYearOperationTest SetUp");
    ClearTables();
    g_num = 0;
}

void PhotoDayMonthYearOperationTest::TearDown()
{
    MEDIA_INFO_LOG("PhotoDayMonthYearOperationTest TearDown");
}

HWTEST_F(PhotoDayMonthYearOperationTest, photo_day_month_year_operation_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("photo_day_month_year_operation_test_001 Start");
    PreparePhotos();
    PrepareAbnormalPhotos();

    int32_t ret = PhotoDayMonthYearOperation::RepairDateTime();
    EXPECT_EQ(ret, E_OK);

    int32_t count = QueryAbnormalPhotosCount();
    EXPECT_EQ(count, 0);
    MEDIA_INFO_LOG("photo_day_month_year_operation_test_001 End");
}

HWTEST_F(PhotoDayMonthYearOperationTest, photo_day_month_year_operation_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("photo_day_month_year_operation_test_002 Start");
    PreparePhotos();
    PrepareAbnormalPhotos();

    int32_t count = QueryAbnormalPhotosCount();
    EXPECT_GT(count, 0);

    int32_t ret = PhotoDayMonthYearOperation::RepairDateTime();
    EXPECT_EQ(ret, E_OK);

    count = QueryAbnormalPhotosCount();
    EXPECT_EQ(count, 0);

    auto changeRows = UpdatePhotosDateDay();
    EXPECT_GT(changeRows, 0);

    ret = PhotoDayMonthYearOperation::RepairDateTime();
    EXPECT_EQ(ret, E_OK);

    count = QueryAbnormalPhotosCount();
    EXPECT_GT(count, 0);
    MEDIA_INFO_LOG("photo_day_month_year_operation_test_002 End");
}
}  // namespace Media
}  // namespace OHOS
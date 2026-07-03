/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "media_time_utils_test.h"

#include "media_time_utils.h"


namespace OHOS {
namespace Media {
using namespace testing::ext;

void MediaTimeUtilsUnitTest::SetUpTestCase(void) {}

void MediaTimeUtilsUnitTest::TearDownTestCase(void) {}

// SetUp:Execute before each test case
void MediaTimeUtilsUnitTest::SetUp() {}

void MediaTimeUtilsUnitTest::TearDown(void) {}

HWTEST_F(MediaTimeUtilsUnitTest, medialib_time_test_001, TestSize.Level1)
{
    EXPECT_EQ(MediaTimeUtils::UTCTimeSeconds() > 0, true);
    EXPECT_EQ(MediaTimeUtils::UTCTimeMilliSeconds() > 0, true);
    EXPECT_EQ(MediaTimeUtils::UTCTimeNanoSeconds() > 0, true);
}

HWTEST_F(MediaTimeUtilsUnitTest, medialib_create_time_test_001, TestSize.Level1)
{
    const std::string PHOTO_DATE_YEAR_FORMAT = "%Y";
    const std::string PHOTO_DATE_MONTH_FORMAT = "%Y%m";
    const std::string PHOTO_DATE_DAY_FORMAT = "%Y%m%d";
    const std::string PHOTO_DETAIL_TIME_FORMAT = "%Y:%m:%d %H:%M:%S";
    int64_t secondTime = 1766644004;
    EXPECT_EQ(MediaTimeUtils::StrCreateTime(PHOTO_DATE_YEAR_FORMAT, secondTime), "2025");
    EXPECT_EQ(MediaTimeUtils::StrCreateTime(PHOTO_DATE_MONTH_FORMAT, secondTime), "202512");
    EXPECT_EQ(MediaTimeUtils::StrCreateTime(PHOTO_DATE_DAY_FORMAT, secondTime), "20251225");
    EXPECT_EQ(MediaTimeUtils::StrCreateTime(PHOTO_DETAIL_TIME_FORMAT, secondTime), "2025:12:25 14:26:44");
    int64_t milliSecondTime = 1766644004132;
    EXPECT_EQ(MediaTimeUtils::StrCreateTimeByMilliseconds(PHOTO_DATE_YEAR_FORMAT, milliSecondTime), "2025");
    EXPECT_EQ(MediaTimeUtils::StrCreateTimeByMilliseconds(PHOTO_DATE_MONTH_FORMAT, milliSecondTime), "202512");
    EXPECT_EQ(MediaTimeUtils::StrCreateTimeByMilliseconds(PHOTO_DATE_DAY_FORMAT, milliSecondTime), "20251225");
    EXPECT_EQ(MediaTimeUtils::StrCreateTimeByMilliseconds(PHOTO_DETAIL_TIME_FORMAT, milliSecondTime),
        "2025:12:25 14:26:44");
}

HWTEST_F(MediaTimeUtilsUnitTest, Timespec2Millisecond_Zero_Test_001, TestSize.Level1)
{
    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec = 0;
    int64_t result = MediaTimeUtils::Timespec2Millisecond(time);
    EXPECT_EQ(result, 0);
}

HWTEST_F(MediaTimeUtilsUnitTest, Timespec2Millisecond_SecondOnly_Test_001, TestSize.Level1)
{
    struct timespec time;
    time.tv_sec = 1;
    time.tv_nsec = 0;
    int64_t result = MediaTimeUtils::Timespec2Millisecond(time);
    EXPECT_EQ(result, 1000);
}

HWTEST_F(MediaTimeUtilsUnitTest, Timespec2Millisecond_NanoOnly_Test_001, TestSize.Level1)
{
    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec = 500000000;
    int64_t result = MediaTimeUtils::Timespec2Millisecond(time);
    EXPECT_EQ(result, 500);
}

HWTEST_F(MediaTimeUtilsUnitTest, Timespec2Millisecond_Mixed_Test_001, TestSize.Level1)
{
    struct timespec time;
    time.tv_sec = 1;
    time.tv_nsec = 500000000;
    int64_t result = MediaTimeUtils::Timespec2Millisecond(time);
    EXPECT_EQ(result, 1500);
}

HWTEST_F(MediaTimeUtilsUnitTest, Timespec2Millisecond_LargeValue_Test_001, TestSize.Level1)
{
    struct timespec time;
    time.tv_sec = 1000;
    time.tv_nsec = 999999999;
    int64_t result = MediaTimeUtils::Timespec2Millisecond(time);
    EXPECT_EQ(result, 1000999);
}

HWTEST_F(MediaTimeUtilsUnitTest, Timespec2Millisecond_Negative_Test_001, TestSize.Level1)
{
    struct timespec time;
    time.tv_sec = -1;
    time.tv_nsec = 0;
    int64_t result = MediaTimeUtils::Timespec2Millisecond(time);
    EXPECT_EQ(result, -1000);
}

HWTEST_F(MediaTimeUtilsUnitTest, Timespec2Millisecond_NanoRounding_Test_001, TestSize.Level1)
{
    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec = 123456789;
    int64_t result = MediaTimeUtils::Timespec2Millisecond(time);
    EXPECT_EQ(result, 123);
}

HWTEST_F(MediaTimeUtilsUnitTest, Timespec2Millisecond_MaxNano_Test_001, TestSize.Level1)
{
    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec = 999999999;
    int64_t result = MediaTimeUtils::Timespec2Millisecond(time);
    EXPECT_EQ(result, 999);
}

HWTEST_F(MediaTimeUtilsUnitTest, Timespec2Millisecond_MinNano_Test_001, TestSize.Level1)
{
    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec = 1;
    int64_t result = MediaTimeUtils::Timespec2Millisecond(time);
    EXPECT_EQ(result, 0);
}

HWTEST_F(MediaTimeUtilsUnitTest, Timespec2Millisecond_HalfSecond_Test_001, TestSize.Level1)
{
    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec = 500000000;
    int64_t result = MediaTimeUtils::Timespec2Millisecond(time);
    EXPECT_EQ(result, 500);
}

HWTEST_F(MediaTimeUtilsUnitTest, Timespec2Millisecond_QuarterSecond_Test_001, TestSize.Level1)
{
    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec = 250000000;
    int64_t result = MediaTimeUtils::Timespec2Millisecond(time);
    EXPECT_EQ(result, 250);
}

HWTEST_F(MediaTimeUtilsUnitTest, Timespec2Millisecond_TenthSecond_Test_001, TestSize.Level1)
{
    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec = 100000000;
    int64_t result = MediaTimeUtils::Timespec2Millisecond(time);
    EXPECT_EQ(result, 100);
}

HWTEST_F(MediaTimeUtilsUnitTest, Timespec2Millisecond_HundredthSecond_Test_001, TestSize.Level1)
{
    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec = 10000000;
    int64_t result = MediaTimeUtils::Timespec2Millisecond(time);
    EXPECT_EQ(result, 10);
}

HWTEST_F(MediaTimeUtilsUnitTest, Timespec2Millisecond_ThousandthSecond_Test_001, TestSize.Level1)
{
    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec = 1000000;
    int64_t result = MediaTimeUtils::Timespec2Millisecond(time);
    EXPECT_EQ(result, 1);
}

HWTEST_F(MediaTimeUtilsUnitTest, UTCTimeSeconds_Positive_Test_001, TestSize.Level1)
{
    int64_t result = MediaTimeUtils::UTCTimeSeconds();
    EXPECT_GT(result, 0);
}

HWTEST_F(MediaTimeUtilsUnitTest, UTCTimeSeconds_Monotonic_Test_001, TestSize.Level1)
{
    int64_t result1 = MediaTimeUtils::UTCTimeSeconds();
    sleep(1);
    int64_t result2 = MediaTimeUtils::UTCTimeSeconds();
    EXPECT_GT(result2, result1);
    EXPECT_GE(result2 - result1, 1);
}

HWTEST_F(MediaTimeUtilsUnitTest, UTCTimeMilliSeconds_Positive_Test_001, TestSize.Level1)
{
    int64_t result = MediaTimeUtils::UTCTimeMilliSeconds();
    EXPECT_GT(result, 0);
}

HWTEST_F(MediaTimeUtilsUnitTest, UTCTimeMilliSeconds_Monotonic_Test_001, TestSize.Level1)
{
    int64_t result1 = MediaTimeUtils::UTCTimeMilliSeconds();
    usleep(100000);
    int64_t result2 = MediaTimeUtils::UTCTimeMilliSeconds();
    EXPECT_GT(result2, result1);
    EXPECT_GE(result2 - result1, 100);
}

HWTEST_F(MediaTimeUtilsUnitTest, UTCTimeMilliSeconds_Granularity_Test_001, TestSize.Level1)
{
    int64_t result1 = MediaTimeUtils::UTCTimeMilliSeconds();
    int64_t result2 = MediaTimeUtils::UTCTimeMilliSeconds();
    EXPECT_GE(result2 - result1, 0);
}

HWTEST_F(MediaTimeUtilsUnitTest, UTCTimeNanoSeconds_Positive_Test_001, TestSize.Level1)
{
    int64_t result = MediaTimeUtils::UTCTimeNanoSeconds();
    EXPECT_GT(result, 0);
}

HWTEST_F(MediaTimeUtilsUnitTest, UTCTimeNanoSeconds_Monotonic_Test_001, TestSize.Level1)
{
    int64_t result1 = MediaTimeUtils::UTCTimeNanoSeconds();
    usleep(1000);
    int64_t result2 = MediaTimeUtils::UTCTimeNanoSeconds();
    EXPECT_GT(result2, result1);
    EXPECT_GE(result2 - result1, 1000000);
}

HWTEST_F(MediaTimeUtilsUnitTest, UTCTimeNanoSeconds_Granularity_Test_001, TestSize.Level1)
{
    int64_t result1 = MediaTimeUtils::UTCTimeNanoSeconds();
    int64_t result2 = MediaTimeUtils::UTCTimeNanoSeconds();
    EXPECT_GE(result2 - result1, 0);
}

HWTEST_F(MediaTimeUtilsUnitTest, StrCreateTime_NegativeTime_Test_001, TestSize.Level1)
{
    const std::string format = "%Y";
    int64_t time = -1;
    std::string result = MediaTimeUtils::StrCreateTime(format, time);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaTimeUtilsUnitTest, StrCreateTime_DateFormat_Test_001, TestSize.Level1)
{
    const std::string format = "%Y-%m-%d";
    int64_t time = 1766644004;
    std::string result = MediaTimeUtils::StrCreateTime(format, time);
    EXPECT_EQ(result, "2025-12-25");
}

HWTEST_F(MediaTimeUtilsUnitTest, StrCreateTime_TimeFormat_Test_001, TestSize.Level1)
{
    const std::string format = "%H:%M:%S";
    int64_t time = 1766644004;
    std::string result = MediaTimeUtils::StrCreateTime(format, time);
    EXPECT_EQ(result, "14:26:44");
}

HWTEST_F(MediaTimeUtilsUnitTest, StrCreateTime_FullFormat_Test_001, TestSize.Level1)
{
    const std::string format = "%Y-%m-%d %H:%M:%S";
    int64_t time = 1766644004;
    std::string result = MediaTimeUtils::StrCreateTime(format, time);
    EXPECT_EQ(result, "2025-12-25 14:26:44");
}

HWTEST_F(MediaTimeUtilsUnitTest, StrCreateTime_SlashFormat_Test_001, TestSize.Level1)
{
    const std::string format = "%Y/%m/%d";
    int64_t time = 1766644004;
    std::string result = MediaTimeUtils::StrCreateTime(format, time);
    EXPECT_EQ(result, "2025/12/25");
}

HWTEST_F(MediaTimeUtilsUnitTest, StrCreateTime_EmptyFormat_Test_001, TestSize.Level1)
{
    const std::string format = "";
    int64_t time = 1766644004;
    std::string result = MediaTimeUtils::StrCreateTime(format, time);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaTimeUtilsUnitTest, StrCreateTimeByMilliseconds_TimeFormat_Test_001, TestSize.Level1)
{
    const std::string format = "%H:%M:%S";
    int64_t time = 1766644004132;
    std::string result = MediaTimeUtils::StrCreateTimeByMilliseconds(format, time);
    EXPECT_EQ(result, "14:26:44");
}

HWTEST_F(MediaTimeUtilsUnitTest, StrCreateTimeByMilliseconds_FullFormat_Test_001, TestSize.Level1)
{
    const std::string format = "%Y-%m-%d %H:%M:%S";
    int64_t time = 1766644004132;
    std::string result = MediaTimeUtils::StrCreateTimeByMilliseconds(format, time);
    EXPECT_EQ(result, "2025-12-25 14:26:44");
}

HWTEST_F(MediaTimeUtilsUnitTest, StrCreateTimeByMilliseconds_EmptyFormat_Test_001, TestSize.Level1)
{
    const std::string format = "";
    int64_t time = 1766644004132;
    std::string result = MediaTimeUtils::StrCreateTimeByMilliseconds(format, time);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaTimeUtilsUnitTest, StrCreateTimeByMilliseconds_LargeTime_Test_001, TestSize.Level1)
{
    const std::string format = "%Y";
    int64_t time = 4102444800000;
    std::string result = MediaTimeUtils::StrCreateTimeByMilliseconds(format, time);
    EXPECT_EQ(result, "2100");
}

HWTEST_F(MediaTimeUtilsUnitTest, StrCreateTimeByMilliseconds_SmallTime_Test_001, TestSize.Level1)
{
    const std::string format = "%Y";
    int64_t time = 0;
    std::string result = MediaTimeUtils::StrCreateTimeByMilliseconds(format, time);
    EXPECT_EQ(result, "1970");
}

HWTEST_F(MediaTimeUtilsUnitTest, StrCreateTimeByMilliseconds_MillisecondPrecision_Test_001, TestSize.Level1)
{
    const std::string format = "%Y-%m-%d %H:%M:%S";
    int64_t time1 = 1766644004000;
    int64_t time2 = 1766644004999;
    std::string result1 = MediaTimeUtils::StrCreateTimeByMilliseconds(format, time1);
    std::string result2 = MediaTimeUtils::StrCreateTimeByMilliseconds(format, time2);
    EXPECT_EQ(result1, result2);
}

HWTEST_F(MediaTimeUtilsUnitTest, TimeStampToUtcDate_Normal_Test_001, TestSize.Level1)
{
    int year{}, month{}, day{};
    EXPECT_EQ(MediaTimeUtils::TimeStampToUtcDate(1766644004, year, month, day), true);
    EXPECT_EQ(year, 2025);
    EXPECT_EQ(month, 12);
    EXPECT_EQ(day, 25);
}

HWTEST_F(MediaTimeUtilsUnitTest, TimeStampToUtcDate_Epoch_Test_001, TestSize.Level1)
{
    int year{}, month{}, day{};
    EXPECT_EQ(MediaTimeUtils::TimeStampToUtcDate(0, year, month, day), true);
    EXPECT_EQ(year, 1970);
    EXPECT_EQ(month, 1);
    EXPECT_EQ(day, 1);
}

HWTEST_F(MediaTimeUtilsUnitTest, IsPlausibleDateTime_WithinRange_Test_001, TestSize.Level1)
{
    EXPECT_EQ(MediaTimeUtils::IsPlausibleDateTime(2025, 12, 26, 1766661664000), true);
    EXPECT_EQ(MediaTimeUtils::IsPlausibleDateTime(2025, 12, 25, 1766661664000), true);
    EXPECT_EQ(MediaTimeUtils::IsPlausibleDateTime(2025, 12, 24, 1766661664000), true);
    EXPECT_EQ(MediaTimeUtils::IsPlausibleDateTime(2025, 12, 24, 1766644004000), true);
    EXPECT_EQ(MediaTimeUtils::IsPlausibleDateTime(2025, 12, 25, 1766644004000), true);
}

HWTEST_F(MediaTimeUtilsUnitTest, IsPlausibleDateTime_OutOfRange_Test_001, TestSize.Level1)
{
    EXPECT_EQ(MediaTimeUtils::IsPlausibleDateTime(2025, 12, 26, 1766644004000), false);
    EXPECT_EQ(MediaTimeUtils::IsPlausibleDateTime(2025, 12, 23, 1766644004000), false);
    EXPECT_EQ(MediaTimeUtils::IsPlausibleDateTime(2024, 12, 25, 1766644004000), false);
    EXPECT_EQ(MediaTimeUtils::IsPlausibleDateTime(2026, 1, 1, 1766644004000), false);
    EXPECT_EQ(MediaTimeUtils::IsPlausibleDateTime(2025, 12, 27, 1766661664000), false);
}
} // namespace Media
} // namespace OHOS
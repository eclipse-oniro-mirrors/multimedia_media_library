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
} // namespace Media
} // namespace OHOS
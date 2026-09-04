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

#include "media_string_utils_test.h"

#include "media_string_utils.h"


namespace OHOS {
namespace Media {
using namespace testing::ext;

void MediaStringUtilsUnitTest::SetUpTestCase(void) {}

void MediaStringUtilsUnitTest::TearDownTestCase(void) {}

// SetUp:Execute before each test case
void MediaStringUtilsUnitTest::SetUp() {}

void MediaStringUtilsUnitTest::TearDown(void) {}

HWTEST_F(MediaStringUtilsUnitTest, medialib_Conver_test_001, TestSize.Level1)
{
    std::string srt = "";
    int32_t value = -1;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
    EXPECT_EQ(value, -1);
    srt = "medialib_IsNumber_test_001";
    ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
    srt = "1";
    ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    value = 1;
}
HWTEST_F(MediaStringUtilsUnitTest, medialib_Convert_test_001, TestSize.Level1)
{
    std::string srt = "";
    int32_t value = -1;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
    EXPECT_EQ(value, -1);
    srt = "medialib_IsNumber_test_001";
    ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
    srt = "1";
    ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    value = 1;
}

HWTEST_F(MediaStringUtilsUnitTest, medialib_start_wtith_test_001, TestSize.Level1)
{
    std::string emptyString = "";
    EXPECT_EQ(MediaStringUtils::StartsWith(emptyString, ""), true);
    EXPECT_EQ(MediaStringUtils::StartsWith(emptyString, "1"), false);
    std::string testJpg = "1.jpg";
    EXPECT_EQ(MediaStringUtils::StartsWith(testJpg, ""), true);
    EXPECT_EQ(MediaStringUtils::StartsWith(testJpg, "1"), true);
    EXPECT_EQ(MediaStringUtils::StartsWith(testJpg, "2"), false);
}

HWTEST_F(MediaStringUtilsUnitTest, medialib_end_wtith_test_001, TestSize.Level1)
{
    std::string emptyString = "";
    EXPECT_EQ(MediaStringUtils::EndsWith(emptyString, ""), true);
    EXPECT_EQ(MediaStringUtils::EndsWith(emptyString, "jpg"), false);
    std::string testJpg = "1.jpg";
    EXPECT_EQ(MediaStringUtils::EndsWith(testJpg, ""), true);
    EXPECT_EQ(MediaStringUtils::EndsWith(testJpg, "jpg"), true);
    EXPECT_EQ(MediaStringUtils::EndsWith(testJpg, "png"), false);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToInt_Decimal_Test_001, TestSize.Level1)
{
    std::string srt = "0";
    int32_t value = -1;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, 0);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToInt_Decimal_Test_002, TestSize.Level1)
{
    std::string srt = "12345";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, 12345);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToInt_Decimal_Test_003, TestSize.Level1)
{
    std::string srt = "-12345";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, -12345);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToInt_Decimal_Test_004, TestSize.Level1)
{
    std::string srt = "2147483647";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, 2147483647);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntWithSpace_Test_001, TestSize.Level1)
{
    std::string srt = " 123";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntWithSpace_Test_002, TestSize.Level1)
{
    std::string srt = "123 ";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntWithSpace_Test_003, TestSize.Level1)
{
    std::string srt = "12 34";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntWithLetter_Test_001, TestSize.Level1)
{
    std::string srt = "123abc";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntWithLetter_Test_002, TestSize.Level1)
{
    std::string srt = "abc123";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntWithSpecialChar_Test_001, TestSize.Level1)
{
    std::string srt = "+123";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntOverflow_Test_001, TestSize.Level1)
{
    std::string srt = "2147483648";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntOverflow_Test_002, TestSize.Level1)
{
    std::string srt = "-2147483649";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntLargeNumber_Test_001, TestSize.Level1)
{
    std::string srt = "999999999";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, 999999999);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntLargeNumber_Test_002, TestSize.Level1)
{
    std::string srt = "-999999999";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, -999999999);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithBothEmpty_Test_001, TestSize.Level1)
{
    std::string str = "";
    std::string prefix = "";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithEmptyPrefix_Test_001, TestSize.Level1)
{
    std::string str = "hello";
    std::string prefix = "";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithEmptyString_Test_001, TestSize.Level1)
{
    std::string str = "";
    std::string prefix = "hello";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithMatch_Test_001, TestSize.Level1)
{
    std::string str = "hello world";
    std::string prefix = "hello";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithMatch_Test_002, TestSize.Level1)
{
    std::string str = "hello world";
    std::string prefix = "hello ";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithNoMatch_Test_001, TestSize.Level1)
{
    std::string str = "hello world";
    std::string prefix = "world";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithNoMatch_Test_002, TestSize.Level1)
{
    std::string str = "hello world";
    std::string prefix = "Hello";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithExactMatch_Test_001, TestSize.Level1)
{
    std::string str = "hello";
    std::string prefix = "hello";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithLongerPrefix_Test_001, TestSize.Level1)
{
    std::string str = "hello";
    std::string prefix = "hello world";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithSingleChar_Test_001, TestSize.Level1)
{
    std::string str = "hello";
    std::string prefix = "h";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithSingleChar_Test_002, TestSize.Level1)
{
    std::string str = "hello";
    std::string prefix = "x";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithSpecialChars_Test_001, TestSize.Level1)
{
    std::string str = "/storage/cloud/files";
    std::string prefix = "/storage/";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithSpecialChars_Test_002, TestSize.Level1)
{
    std::string str = "file://media/image/123";
    std::string prefix = "file://";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithNumbers_Test_001, TestSize.Level1)
{
    std::string str = "1234567890";
    std::string prefix = "123";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithUnicode_Test_001, TestSize.Level1)
{
    std::string str = "你好世界";
    std::string prefix = "你好";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithBothEmpty_Test_001, TestSize.Level1)
{
    std::string str = "";
    std::string suffix = "";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithEmptySuffix_Test_001, TestSize.Level1)
{
    std::string str = "hello";
    std::string suffix = "";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithEmptyString_Test_001, TestSize.Level1)
{
    std::string str = "";
    std::string suffix = "hello";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithMatch_Test_001, TestSize.Level1)
{
    std::string str = "hello world";
    std::string suffix = "world";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithMatch_Test_002, TestSize.Level1)
{
    std::string str = "hello world";
    std::string suffix = " world";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithNoMatch_Test_001, TestSize.Level1)
{
    std::string str = "hello world";
    std::string suffix = "hello";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithNoMatch_Test_002, TestSize.Level1)
{
    std::string str = "hello world";
    std::string suffix = "World";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithExactMatch_Test_001, TestSize.Level1)
{
    std::string str = "hello";
    std::string suffix = "hello";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithLongerSuffix_Test_001, TestSize.Level1)
{
    std::string str = "hello";
    std::string suffix = "hello world";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithSingleChar_Test_001, TestSize.Level1)
{
    std::string str = "hello";
    std::string suffix = "o";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithSingleChar_Test_002, TestSize.Level1)
{
    std::string str = "hello";
    std::string suffix = "x";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithFileExtensions_Test_001, TestSize.Level1)
{
    std::string str = "image.jpg";
    std::string suffix = ".jpg";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithFileExtensions_Test_002, TestSize.Level1)
{
    std::string str = "image.jpg";
    std::string suffix = ".png";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithFileExtensions_Test_003, TestSize.Level1)
{
    std::string str = "video.mp4";
    std::string suffix = ".mp4";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithFileExtensions_Test_004, TestSize.Level1)
{
    std::string str = "audio.mp3";
    std::string suffix = ".mp3";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithSpecialChars_Test_001, TestSize.Level1)
{
    std::string str = "/storage/cloud/files/";
    std::string suffix = "files/";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithNumbers_Test_001, TestSize.Level1)
{
    std::string str = "file123456";
    std::string suffix = "456";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithUnicode_Test_001, TestSize.Level1)
{
    std::string str = "你好世界";
    std::string suffix = "世界";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithCaseSensitive_Test_001, TestSize.Level1)
{
    std::string str = "hello";
    std::string suffix = "HELLO";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithCaseSensitive_Test_001, TestSize.Level1)
{
    std::string str = "hello";
    std::string prefix = "HELLO";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntZero_Test_001, TestSize.Level1)
{
    std::string srt = "0";
    int32_t value = 1;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, 0);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntLeadingZeros_Test_001, TestSize.Level1)
{
    std::string srt = "000123";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, 123);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntSingleDigit_Test_001, TestSize.Level1)
{
    std::string srt = "5";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, 5);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntMultipleDigits_Test_001, TestSize.Level1)
{
    std::string srt = "123456789";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, 123456789);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithPath_Test_001, TestSize.Level1)
{
    std::string str = "/storage/cloud/files/image.jpg";
    std::string prefix = "/storage/cloud/files/";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithUri_Test_001, TestSize.Level1)
{
    std::string str = "datashare:///media/image/123";
    std::string prefix = "datashare:///";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithPath_Test_001, TestSize.Level1)
{
    std::string str = "/storage/cloud/files/image.jpg";
    std::string suffix = "image.jpg";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithUri_Test_001, TestSize.Level1)
{
    std::string str = "datashare:///media/image/123";
    std::string suffix = "123";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntValueUnchanged_Test_001, TestSize.Level1)
{
    std::string srt = "invalid";
    int32_t value = 999;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
    EXPECT_EQ(value, 999);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithWhitespace_Test_001, TestSize.Level1)
{
    std::string str = " hello";
    std::string prefix = "hello";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithWhitespace_Test_001, TestSize.Level1)
{
    std::string str = "hello ";
    std::string suffix = "hello";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithRepeatedChars_Test_001, TestSize.Level1)
{
    std::string str = "aaaaa";
    std::string prefix = "aaa";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithRepeatedChars_Test_001, TestSize.Level1)
{
    std::string str = "aaaaa";
    std::string suffix = "aaa";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntMinInt_Test_001, TestSize.Level1)
{
    std::string srt = "-2147483648";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, -2147483648);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithMiddleMatch_Test_001, TestSize.Level1)
{
    std::string str = "hello world";
    std::string prefix = "ello";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithMiddleMatch_Test_001, TestSize.Level1)
{
    std::string str = "hello world";
    std::string suffix = "hello";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithPartialPrefix_Test_001, TestSize.Level1)
{
    std::string str = "he";
    std::string prefix = "hello";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithPartialSuffix_Test_001, TestSize.Level1)
{
    std::string str = "lo";
    std::string suffix = "hello";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntVeryLongNumber_Test_001, TestSize.Level1)
{
    std::string srt = "1000000000";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, 1000000000);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntVeryLongNumber_Test_002, TestSize.Level1)
{
    std::string srt = "-1000000000";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, -1000000000);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithFullString_Test_001, TestSize.Level1)
{
    std::string str = "hello";
    std::string prefix = "hello";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithFullString_Test_001, TestSize.Level1)
{
    std::string str = "hello";
    std::string suffix = "hello";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithDifferentCase_Test_001, TestSize.Level1)
{
    std::string str = "HelloWorld";
    std::string prefix = "helloworld";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithDifferentCase_Test_001, TestSize.Level1)
{
    std::string str = "HelloWorld";
    std::string suffix = "HELLOWORLD";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntWithPlusSign_Test_001, TestSize.Level1)
{
    std::string srt = "+123";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntWithMultipleMinusSign_Test_001, TestSize.Level1)
{
    std::string srt = "--123";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithWithSlash_Test_001, TestSize.Level1)
{
    std::string str = "/path/to/file";
    std::string prefix = "/";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithWithSlash_Test_001, TestSize.Level1)
{
    std::string str = "/path/to/file/";
    std::string suffix = "/";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntWithUnderscore_Test_001, TestSize.Level1)
{
    std::string srt = "12_34";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithWithDash_Test_001, TestSize.Level1)
{
    std::string str = "file-name.txt";
    std::string prefix = "file-";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithWithDash_Test_001, TestSize.Level1)
{
    std::string str = "file-name.txt";
    std::string suffix = ".txt";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntBoundary_Test_001, TestSize.Level1)
{
    std::string srt = "2147483646";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, 2147483646);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntBoundary_Test_002, TestSize.Level1)
{
    std::string srt = "-2147483647";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, -2147483647);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithMixedChars_Test_001, TestSize.Level1)
{
    std::string str = "a1b2c3";
    std::string prefix = "a1b";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithMixedChars_Test_001, TestSize.Level1)
{
    std::string str = "a1b2c3";
    std::string suffix = "2c3";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntWithComma_Test_001, TestSize.Level1)
{
    std::string srt = "1,234";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithUrl_Test_001, TestSize.Level1)
{
    std::string str = "https://example.com";
    std::string prefix = "https://";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithUrl_Test_001, TestSize.Level1)
{
    std::string str = "https://example.com";
    std::string suffix = ".com";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntScientificNotation_Test_001, TestSize.Level1)
{
    std::string srt = "1e5";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithProtocol_Test_001, TestSize.Level1)
{
    std::string str = "file:///storage/path";
    std::string prefix = "file://";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithDirectory_Test_001, TestSize.Level1)
{
    std::string str = "/storage/cloud/files/";
    std::string suffix = "/";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntWithHexPrefix_Test_001, TestSize.Level1)
{
    std::string srt = "0xFF";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithEmptyVsNonEmpty_Test_001, TestSize.Level1)
{
    std::string str = "";
    std::string prefix = "a";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithEmptyVsNonEmpty_Test_001, TestSize.Level1)
{
    std::string str = "";
    std::string suffix = "a";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntEmptyString_Test_001, TestSize.Level1)
{
    std::string srt = "";
    int32_t value = 123;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
    EXPECT_EQ(value, 123);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithNonEmptyVsEmpty_Test_001, TestSize.Level1)
{
    std::string str = "a";
    std::string prefix = "";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithNonEmptyVsEmpty_Test_001, TestSize.Level1)
{
    std::string str = "a";
    std::string suffix = "";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntWithOctalPrefix_Test_001, TestSize.Level1)
{
    std::string srt = "0123";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, 123);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithMultipleSlashes_Test_001, TestSize.Level1)
{
    std::string str = "///path";
    std::string prefix = "//";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithMultipleSlashes_Test_001, TestSize.Level1)
{
    std::string str = "path///";
    std::string suffix = "///";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntWithBinaryPrefix_Test_001, TestSize.Level1)
{
    std::string srt = "0b101";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithFilePath_Test_001, TestSize.Level1)
{
    std::string str = "C:\\Users\\file.txt";
    std::string prefix = "C:\\";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithFilePath_Test_001, TestSize.Level1)
{
    std::string str = "C:\\Users\\file.txt";
    std::string suffix = "file.txt";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntOneDigitNegative_Test_001, TestSize.Level1)
{
    std::string srt = "-5";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, -5);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithWithDot_Test_001, TestSize.Level1)
{
    std::string str = ".hiddenfile";
    std::string prefix = ".";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithWithDot_Test_001, TestSize.Level1)
{
    std::string str = "file.";
    std::string suffix = ".";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntAllZeros_Test_001, TestSize.Level1)
{
    std::string srt = "000000";
    int32_t value = 123;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, 0);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithWithUnderscore_Test_001, TestSize.Level1)
{
    std::string str = "_private_var";
    std::string prefix = "_";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithWithUnderscore_Test_001, TestSize.Level1)
{
    std::string str = "var_";
    std::string suffix = "_";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithWithAtSign_Test_001, TestSize.Level1)
{
    std::string str = "@username";
    std::string prefix = "@";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithWithAtSign_Test_001, TestSize.Level1)
{
    std::string str = "email@domain.com";
    std::string suffix = "@domain.com";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntOne_Test_001, TestSize.Level1)
{
    std::string srt = "1";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, 1);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntMinusOne_Test_001, TestSize.Level1)
{
    std::string srt = "-1";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, -1);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithWithHash_Test_001, TestSize.Level1)
{
    std::string str = "#include";
    std::string prefix = "#";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithWithHash_Test_001, TestSize.Level1)
{
    std::string str = "section#";
    std::string suffix = "#";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntWithTab_Test_001, TestSize.Level1)
{
    std::string srt = "123\t456";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithWithDollar_Test_001, TestSize.Level1)
{
    std::string str = "$variable";
    std::string prefix = "$";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithWithDollar_Test_001, TestSize.Level1)
{
    std::string str = "price$";
    std::string suffix = "$";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntWithNewline_Test_001, TestSize.Level1)
{
    std::string srt = "123\n456";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithWithPercent_Test_001, TestSize.Level1)
{
    std::string str = "%TEMP%";
    std::string prefix = "%";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithWithPercent_Test_001, TestSize.Level1)
{
    std::string str = "100%";
    std::string suffix = "%";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntWithCarriageReturn_Test_001, TestSize.Level1)
{
    std::string srt = "123\r456";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithWithPipeSeparator_Test_001, TestSize.Level1)
{
    std::string str = "part1|part2";
    std::string suffix = "|part2";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntMixedAlphaNum_Test_001, TestSize.Level1)
{
    std::string srt = "a1b2c3";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithWithColonSeparator_Test_001, TestSize.Level1)
{
    std::string str = "key:value";
    std::string prefix = "key:";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithWithColonSeparator_Test_001, TestSize.Level1)
{
    std::string str = "key:value";
    std::string suffix = ":value";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntOnlyLetters_Test_001, TestSize.Level1)
{
    std::string srt = "abc";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithWithDotSeparator_Test_001, TestSize.Level1)
{
    std::string str = "domain.com";
    std::string prefix = "domain.";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithWithDotSeparator_Test_001, TestSize.Level1)
{
    std::string str = "domain.com";
    std::string suffix = ".com";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntOnlySpecialChars_Test_001, TestSize.Level1)
{
    std::string srt = "!@#$%";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithWithHyphenSeparator_Test_001, TestSize.Level1)
{
    std::string str = "file-name.txt";
    std::string prefix = "file-";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithWithHyphenSeparator_Test_001, TestSize.Level1)
{
    std::string str = "file-name.txt";
    std::string suffix = "-name.txt";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntWithSpacesAround_Test_001, TestSize.Level1)
{
    std::string srt = " 123 ";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithWithUnderscoreSeparator_Test_001, TestSize.Level1)
{
    std::string str = "file_name.txt";
    std::string prefix = "file_";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithWithUnderscoreSeparator_Test_001, TestSize.Level1)
{
    std::string str = "file_name.txt";
    std::string suffix = "_name.txt";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntWithTabAround_Test_001, TestSize.Level1)
{
    std::string srt = "\t123\t";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithComplexPath_Test_001, TestSize.Level1)
{
    std::string str = "/storage/cloud/files/image.jpg";
    std::string prefix = "/storage/cloud/";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithComplexPath_Test_001, TestSize.Level1)
{
    std::string str = "/storage/cloud/files/image.jpg";
    std::string suffix = "files/image.jpg";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntWithNewlineAround_Test_001, TestSize.Level1)
{
    std::string srt = "\n123\n";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithComplexUri_Test_001, TestSize.Level1)
{
    std::string str = "datashare:///media/image/123?param=value";
    std::string prefix = "datashare:///media/";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithComplexUri_Test_001, TestSize.Level1)
{
    std::string str = "datashare:///media/image/123?param=value";
    std::string suffix = "?param=value";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntNullString_Test_001, TestSize.Level1)
{
    std::string srt;
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithVersionString_Test_001, TestSize.Level1)
{
    std::string str = "version 1.0.0";
    std::string prefix = "version ";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithVersionString_Test_001, TestSize.Level1)
{
    std::string str = "version 1.0.0";
    std::string suffix = "1.0.0";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntRepeatedDigit_Test_001, TestSize.Level1)
{
    std::string srt = "1111111111";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, 1111111111);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithProtocolHttp_Test_001, TestSize.Level1)
{
    std::string str = "http://example.com";
    std::string prefix = "http://";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithProtocolHttp_Test_001, TestSize.Level1)
{
    std::string str = "http://example.com";
    std::string suffix = "example.com";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntAlternatingDigits_Test_001, TestSize.Level1)
{
    std::string srt = "101010101";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, 101010101);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithProtocolHttps_Test_001, TestSize.Level1)
{
    std::string str = "https://example.com";
    std::string prefix = "https://";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithProtocolHttps_Test_001, TestSize.Level1)
{
    std::string str = "https://example.com";
    std::string suffix = "example.com";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntDescendingDigits_Test_001, TestSize.Level1)
{
    std::string srt = "987654321";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, 987654321);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithProtocolFtp_Test_001, TestSize.Level1)
{
    std::string str = "ftp://example.com";
    std::string prefix = "ftp://";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithProtocolFtp_Test_001, TestSize.Level1)
{
    std::string str = "ftp://example.com";
    std::string suffix = "example.com";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntAscendingDigits_Test_001, TestSize.Level1)
{
    std::string srt = "123456789";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, 123456789);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithProtocolFile_Test_001, TestSize.Level1)
{
    std::string str = "file:///path/to/file";
    std::string prefix = "file://";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithProtocolFile_Test_001, TestSize.Level1)
{
    std::string str = "file:///path/to/file";
    std::string suffix = "file";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntWithZeroPrefix_Test_001, TestSize.Level1)
{
    std::string srt = "012345";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, 12345);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithDataShare_Test_001, TestSize.Level1)
{
    std::string str = "datashare:///media";
    std::string prefix = "datashare://";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithDataShare_Test_001, TestSize.Level1)
{
    std::string str = "datashare:///media";
    std::string suffix = "media";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntNegativeLarge_Test_001, TestSize.Level1)
{
    std::string srt = "-1000000000";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, -1000000000);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithMediaLibrary_Test_001, TestSize.Level1)
{
    std::string str = "medialibrary://image/123";
    std::string prefix = "medialibrary://";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EndsWithMediaLibrary_Test_001, TestSize.Level1)
{
    std::string str = "medialibrary://image/123";
    std::string suffix = "123";
    EXPECT_EQ(MediaStringUtils::EndsWith(str, suffix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ConvertToIntPositiveLarge_Test_001, TestSize.Level1)
{
    std::string srt = "1000000000";
    int32_t value = 0;
    bool ret = MediaStringUtils::ConvertToInt(srt, value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, 1000000000);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithStoragePath_Test_001, TestSize.Level1)
{
    std::string str = "/storage/cloud/files";
    std::string prefix = "/storage/";
    EXPECT_EQ(MediaStringUtils::StartsWith(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, ToLower_EmptyString_Test_001, TestSize.Level1)
{
    std::string str = "";
    std::string result = MediaStringUtils::ToLower(str);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaStringUtilsUnitTest, ToLower_AllUppercase_Test_001, TestSize.Level1)
{
    std::string str = "HELLO 你好ABC123";
    std::string result = MediaStringUtils::ToLower(str);
    EXPECT_EQ(result, "hello 你好abc123");
}

HWTEST_F(MediaStringUtilsUnitTest, ToLower_MixedCase_Test_001, TestSize.Level1)
{
    std::string str = "HeLLoWoRLd测试@example.com";
    std::string result = MediaStringUtils::ToLower(str);
    EXPECT_EQ(result, "helloworld测试@example.com");
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithIgnoreCase_Match_Test_001, TestSize.Level1)
{
    std::string str = "Hello你好World";
    std::string prefix = "HELLO你好";
    EXPECT_EQ(MediaStringUtils::StartsWithIgnoreCase(str, prefix), true);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithIgnoreCase_EmptyString_Test_001, TestSize.Level1)
{
    std::string str = "";
    std::string prefix = "hello你好";
    EXPECT_EQ(MediaStringUtils::StartsWithIgnoreCase(str, prefix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, StartsWithIgnoreCase_LongerPrefix_Test_001, TestSize.Level1)
{
    std::string str = "hello你好";
    std::string prefix = "hello你好world";
    EXPECT_EQ(MediaStringUtils::StartsWithIgnoreCase(str, prefix), false);
}

HWTEST_F(MediaStringUtilsUnitTest, EqualToIgnoreCase_Match_Test_001, TestSize.Level1)
{
    std::string strA = "hello你好世界123";
    std::string strB = "HELLO你好世界123";
    EXPECT_EQ(MediaStringUtils::EqualToIgnoreCase(strA, strB), true);
}

HWTEST_F(MediaStringUtilsUnitTest, EqualToIgnoreCase_DifferentLength_Test_001, TestSize.Level1)
{
    std::string strA = "hello你好";
    std::string strB = "hello你好o";
    EXPECT_EQ(MediaStringUtils::EqualToIgnoreCase(strA, strB), false);
}
} // namespace Media
} // namespace OHOS
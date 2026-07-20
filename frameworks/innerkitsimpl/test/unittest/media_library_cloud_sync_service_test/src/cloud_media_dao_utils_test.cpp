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

#define MLOG_TAG "MediaCloudSync"

#include "cloud_media_dao_utils_test.h"

#include "media_log.h"
#include "cloud_media_dao_utils.h"
#include "cloud_media_photos_dao.h"
#include "cloud_mdkrecord_photos_vo.h"
#include "photos_po.h"
#include "cloud_media_define.h"
#include "media_string_utils.h"

namespace OHOS::Media::CloudSync {
using namespace testing::ext;

void CloudMediaDaoUtilsTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("start CloudMediaDaoUtilsTest::SetUpTestCase");
}

void CloudMediaDaoUtilsTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("start CloudMediaDaoUtilsTest::TearDownTestCase");
}

void CloudMediaDaoUtilsTest::SetUp()
{
    MEDIA_INFO_LOG("setup");
}

void CloudMediaDaoUtilsTest::TearDown(void) {}

HWTEST_F(CloudMediaDaoUtilsTest, ToStringWithCommaAndQuote_EmptyVector, TestSize.Level1)
{
    std::vector<std::string> values;
    std::string result = CloudMediaDaoUtils::ToStringWithCommaAndQuote(values);
    EXPECT_EQ(result, "");
}

HWTEST_F(CloudMediaDaoUtilsTest, ToStringWithCommaAndQuote_SingleElement, TestSize.Level1)
{
    std::vector<std::string> values;
    values.emplace_back("hello");
    std::string result = CloudMediaDaoUtils::ToStringWithCommaAndQuote(values);
    EXPECT_EQ(result, "'hello'");
}

HWTEST_F(CloudMediaDaoUtilsTest, ToStringWithCommaAndQuote_TwoElements, TestSize.Level1)
{
    std::vector<std::string> values;
    values.emplace_back("hello");
    values.emplace_back("world");
    std::string result = CloudMediaDaoUtils::ToStringWithCommaAndQuote(values);
    EXPECT_EQ(result, "'hello','world'");
}

HWTEST_F(CloudMediaDaoUtilsTest, ToStringWithCommaAndQuote_MultipleElements, TestSize.Level1)
{
    std::vector<std::string> values;
    values.emplace_back("a");
    values.emplace_back("b");
    values.emplace_back("c");
    values.emplace_back("d");
    values.emplace_back("e");
    std::string result = CloudMediaDaoUtils::ToStringWithCommaAndQuote(values);
    EXPECT_EQ(result, "'a','b','c','d','e'");
}

HWTEST_F(CloudMediaDaoUtilsTest, ToStringWithCommaAndQuote_EmptyString, TestSize.Level1)
{
    std::vector<std::string> values;
    values.emplace_back("");
    std::string result = CloudMediaDaoUtils::ToStringWithCommaAndQuote(values);
    EXPECT_EQ(result, "''");
}

HWTEST_F(CloudMediaDaoUtilsTest, ToStringWithCommaAndQuote_SpecialCharacters, TestSize.Level1)
{
    std::vector<std::string> values;
    values.emplace_back("hello'world");
    values.emplace_back("test\"data");
    std::string result = CloudMediaDaoUtils::ToStringWithCommaAndQuote(values);
    EXPECT_EQ(result, "'hello'world','test\"data'");
}

HWTEST_F(CloudMediaDaoUtilsTest, ToStringWithCommaAndQuote_NumericStrings, TestSize.Level1)
{
    std::vector<std::string> values;
    values.emplace_back("123");
    values.emplace_back("456");
    values.emplace_back("789");
    std::string result = CloudMediaDaoUtils::ToStringWithCommaAndQuote(values);
    EXPECT_EQ(result, "'123','456','789'");
}

HWTEST_F(CloudMediaDaoUtilsTest, ToStringWithComma_EmptyVector, TestSize.Level1)
{
    std::vector<std::string> fileIds;
    std::string result = CloudMediaDaoUtils::ToStringWithComma(fileIds);
    EXPECT_EQ(result, "");
}

HWTEST_F(CloudMediaDaoUtilsTest, ToStringWithComma_SingleElement, TestSize.Level1)
{
    std::vector<std::string> fileIds;
    fileIds.emplace_back("1");
    std::string result = CloudMediaDaoUtils::ToStringWithComma(fileIds);
    EXPECT_EQ(result, "1");
}

HWTEST_F(CloudMediaDaoUtilsTest, ToStringWithComma_TwoElements, TestSize.Level1)
{
    std::vector<std::string> fileIds;
    fileIds.emplace_back("1");
    fileIds.emplace_back("2");
    std::string result = CloudMediaDaoUtils::ToStringWithComma(fileIds);
    EXPECT_EQ(result, "1,2");
}

HWTEST_F(CloudMediaDaoUtilsTest, ToStringWithComma_MultipleElements, TestSize.Level1)
{
    std::vector<std::string> fileIds;
    fileIds.emplace_back("1");
    fileIds.emplace_back("2");
    fileIds.emplace_back("3");
    fileIds.emplace_back("4");
    fileIds.emplace_back("5");
    std::string result = CloudMediaDaoUtils::ToStringWithComma(fileIds);
    EXPECT_EQ(result, "1,2,3,4,5");
}

HWTEST_F(CloudMediaDaoUtilsTest, ToStringWithComma_StringIds, TestSize.Level1)
{
    std::vector<std::string> fileIds;
    fileIds.emplace_back("abc");
    fileIds.emplace_back("def");
    fileIds.emplace_back("ghi");
    std::string result = CloudMediaDaoUtils::ToStringWithComma(fileIds);
    EXPECT_EQ(result, "abc,def,ghi");
}

HWTEST_F(CloudMediaDaoUtilsTest, ToStringWithComma_LongStrings, TestSize.Level1)
{
    std::vector<std::string> fileIds;
    fileIds.emplace_back("very_long_string_id_12345");
    fileIds.emplace_back("another_long_string_id_67890");
    std::string result = CloudMediaDaoUtils::ToStringWithComma(fileIds);
    EXPECT_EQ(result, "very_long_string_id_12345,another_long_string_id_67890");
}

HWTEST_F(CloudMediaDaoUtilsTest, FillParams_NoPlaceholders, TestSize.Level1)
{
    std::string sql = "SELECT name, score FROM Stu WHERE age >= 18 AND age <= 45;";
    std::vector<std::string> bindArgs;
    std::string result = MediaStringUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, sql);
}

HWTEST_F(CloudMediaDaoUtilsTest, FillParams_SinglePlaceholder, TestSize.Level1)
{
    std::string sql = "SELECT name FROM Stu WHERE age >= {0};";
    std::vector<std::string> bindArgs = {"18"};
    std::string result = MediaStringUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, "SELECT name FROM Stu WHERE age >= 18;");
}

HWTEST_F(CloudMediaDaoUtilsTest, FillParams_MultiplePlaceholders, TestSize.Level1)
{
    std::string sql = "SELECT name, score FROM Stu WHERE age >= {0} AND age <= {1};";
    std::vector<std::string> bindArgs = {"18", "45"};
    std::string result = MediaStringUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, "SELECT name, score FROM Stu WHERE age >= 18 AND age <= 45;");
}

HWTEST_F(CloudMediaDaoUtilsTest, FillParams_RepeatedPlaceholder, TestSize.Level1)
{
    std::string sql = "SELECT name FROM Stu WHERE age = {0} OR score = {0};";
    std::vector<std::string> bindArgs = {"100"};
    std::string result = MediaStringUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, "SELECT name FROM Stu WHERE age = 100 OR score = 100;");
}

HWTEST_F(CloudMediaDaoUtilsTest, FillParams_MultipleRepeatedPlaceholders, TestSize.Level1)
{
    std::string sql = "SELECT name FROM Stu WHERE age = {0} OR age = {1} OR score = {0} OR score = {1};";
    std::vector<std::string> bindArgs = {"18", "45"};
    std::string result = MediaStringUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, "SELECT name FROM Stu WHERE age = 18 OR age = 45 OR score = 18 OR score = 45;");
}

HWTEST_F(CloudMediaDaoUtilsTest, FillParams_EmptyPlaceholderValue, TestSize.Level1)
{
    std::string sql = "SELECT name FROM Stu WHERE name = {0};";
    std::vector<std::string> bindArgs = {""};
    std::string result = MediaStringUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, "SELECT name FROM Stu WHERE name = ;");
}

HWTEST_F(CloudMediaDaoUtilsTest, FillParams_StringWithSpecialChars, TestSize.Level1)
{
    std::string sql = "SELECT name FROM Stu WHERE name = {0};";
    std::vector<std::string> bindArgs = {"John's Test"};
    std::string result = MediaStringUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, "SELECT name FROM Stu WHERE name = John's Test;");
}

HWTEST_F(CloudMediaDaoUtilsTest, FillParams_LargePlaceholderIndex, TestSize.Level1)
{
    std::string sql = "SELECT name FROM Stu WHERE id = {9};{0}";
    std::vector<std::string> bindArgs = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"};
    std::string result = MediaStringUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, "SELECT name FROM Stu WHERE id = j;a");
}

HWTEST_F(CloudMediaDaoUtilsTest, FillParams_NoBindArgs, TestSize.Level1)
{
    std::string sql = "SELECT name FROM Stu WHERE age >= {0} AND age <= {1};";
    std::vector<std::string> bindArgs;
    std::string result = MediaStringUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, sql);
}

HWTEST_F(CloudMediaDaoUtilsTest, FillParams_MoreBindArgsThanPlaceholders, TestSize.Level1)
{
    std::string sql = "SELECT name FROM Stu WHERE age = {0};";
    std::vector<std::string> bindArgs = {"18", "45", "90"};
    std::string result = MediaStringUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, "SELECT name FROM Stu WHERE age = 18;");
}

HWTEST_F(CloudMediaDaoUtilsTest, GetNumbers_EmptyVector, TestSize.Level1)
{
    std::vector<std::string> albumIds;
    auto result = CloudMediaDaoUtils::GetNumbers(albumIds);
    EXPECT_EQ(result.size(), 0);
}

HWTEST_F(CloudMediaDaoUtilsTest, GetNumbers_AllNumbers, TestSize.Level1)
{
    std::vector<std::string> albumIds;
    albumIds.emplace_back("1");
    albumIds.emplace_back("2");
    albumIds.emplace_back("3");
    auto result = CloudMediaDaoUtils::GetNumbers(albumIds);
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "1");
    EXPECT_EQ(result[1], "2");
    EXPECT_EQ(result[2], "3");
}

HWTEST_F(CloudMediaDaoUtilsTest, GetNumbers_AllStrings, TestSize.Level1)
{
    std::vector<std::string> albumIds;
    albumIds.emplace_back("hello");
    albumIds.emplace_back("world");
    albumIds.emplace_back("test");
    auto result = CloudMediaDaoUtils::GetNumbers(albumIds);
    EXPECT_EQ(result.size(), 0);
}

HWTEST_F(CloudMediaDaoUtilsTest, GetNumbers_Mixed, TestSize.Level1)
{
    std::vector<std::string> albumIds;
    albumIds.emplace_back("hello");
    albumIds.emplace_back("1");
    albumIds.emplace_back("world");
    albumIds.emplace_back("2");
    albumIds.emplace_back("test");
    auto result = CloudMediaDaoUtils::GetNumbers(albumIds);
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], "1");
    EXPECT_EQ(result[1], "2");
}

HWTEST_F(CloudMediaDaoUtilsTest, GetNumbers_NegativeNumbers, TestSize.Level1)
{
    std::vector<std::string> albumIds;
    albumIds.emplace_back("-1");
    albumIds.emplace_back("-2");
    albumIds.emplace_back("-3");
    auto result = CloudMediaDaoUtils::GetNumbers(albumIds);
    EXPECT_EQ(result.size(), 0);
}

HWTEST_F(CloudMediaDaoUtilsTest, GetNumbers_Zero, TestSize.Level1)
{
    std::vector<std::string> albumIds;
    albumIds.emplace_back("0");
    auto result = CloudMediaDaoUtils::GetNumbers(albumIds);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "0");
}

HWTEST_F(CloudMediaDaoUtilsTest, GetNumbers_LargeNumbers, TestSize.Level1)
{
    std::vector<std::string> albumIds;
    albumIds.emplace_back("999999");
    albumIds.emplace_back("1000000");
    auto result = CloudMediaDaoUtils::GetNumbers(albumIds);
    EXPECT_EQ(result.size(), 2);
}

HWTEST_F(CloudMediaDaoUtilsTest, GetNumbers_DecimalNumbers, TestSize.Level1)
{
    std::vector<std::string> albumIds;
    albumIds.emplace_back("1.5");
    albumIds.emplace_back("2.7");
    auto result = CloudMediaDaoUtils::GetNumbers(albumIds);
    EXPECT_EQ(result.size(), 0);
}

HWTEST_F(CloudMediaDaoUtilsTest, GetNumbers_Alphanumeric, TestSize.Level1)
{
    std::vector<std::string> albumIds;
    albumIds.emplace_back("abc123");
    albumIds.emplace_back("123abc");
    auto result = CloudMediaDaoUtils::GetNumbers(albumIds);
    EXPECT_EQ(result.size(), 0);
}

HWTEST_F(CloudMediaDaoUtilsTest, ToInt32_ValidPositiveNumber, TestSize.Level1)
{
    std::string str = "123";
    int32_t result = CloudMediaDaoUtils::ToInt32(str);
    EXPECT_EQ(result, 123);
}

HWTEST_F(CloudMediaDaoUtilsTest, ToInt32_ValidNegativeNumber, TestSize.Level1)
{
    std::string str = "-456";
    int32_t result = CloudMediaDaoUtils::ToInt32(str);
    EXPECT_EQ(result, -456);
}

HWTEST_F(CloudMediaDaoUtilsTest, ToInt32_Zero, TestSize.Level1)
{
    std::string str = "0";
    int32_t result = CloudMediaDaoUtils::ToInt32(str);
    EXPECT_EQ(result, 0);
}

HWTEST_F(CloudMediaDaoUtilsTest, ToInt32_MaxInt32, TestSize.Level1)
{
    std::string str = "2147483647";
    int32_t result = CloudMediaDaoUtils::ToInt32(str);
    EXPECT_EQ(result, INT_MAX);
}

HWTEST_F(CloudMediaDaoUtilsTest, ToInt32_MinInt32, TestSize.Level1)
{
    std::string str = "-2147483648";
    int32_t result = CloudMediaDaoUtils::ToInt32(str);
    EXPECT_EQ(result, INT_MIN);
}

HWTEST_F(CloudMediaDaoUtilsTest, ToInt32_InvalidString, TestSize.Level1)
{
    std::string str = "abc";
    int32_t result = CloudMediaDaoUtils::ToInt32(str);
    EXPECT_EQ(result, 0);
}

HWTEST_F(CloudMediaDaoUtilsTest, ToInt32_EmptyString, TestSize.Level1)
{
    std::string str = "";
    int32_t result = CloudMediaDaoUtils::ToInt32(str);
    EXPECT_EQ(result, 0);
}

HWTEST_F(CloudMediaDaoUtilsTest, ToInt32_PartialNumber, TestSize.Level1)
{
    std::string str = "123abc";
    int32_t result = CloudMediaDaoUtils::ToInt32(str);
    EXPECT_EQ(result, 0);
}

HWTEST_F(CloudMediaDaoUtilsTest, ToInt32_NumberWithSpace, TestSize.Level1)
{
    std::string str = "123 456";
    int32_t result = CloudMediaDaoUtils::ToInt32(str);
    EXPECT_EQ(result, 0);
}

HWTEST_F(CloudMediaDaoUtilsTest, ToInt32_Decimal, TestSize.Level1)
{
    std::string str = "123.456";
    int32_t result = CloudMediaDaoUtils::ToInt32(str);
    EXPECT_EQ(result, 0);
}

HWTEST_F(CloudMediaDaoUtilsTest, ToInt32_WithPlusSign, TestSize.Level1)
{
    std::string str = "+123";
    int32_t result = CloudMediaDaoUtils::ToInt32(str);
    EXPECT_EQ(result, 123);
}

HWTEST_F(CloudMediaDaoUtilsTest, GetStringVector_EmptyVector, TestSize.Level1)
{
    std::vector<int32_t> intVals;
    auto result = CloudMediaDaoUtils::GetStringVector(intVals);
    EXPECT_EQ(result.size(), 0);
}

HWTEST_F(CloudMediaDaoUtilsTest, GetStringVector_SingleElement, TestSize.Level1)
{
    std::vector<int32_t> intVals = {123};
    auto result = CloudMediaDaoUtils::GetStringVector(intVals);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "123");
}

HWTEST_F(CloudMediaDaoUtilsTest, GetStringVector_MultipleElements, TestSize.Level1)
{
    std::vector<int32_t> intVals = {1, 2, 3, 4, 5};
    auto result = CloudMediaDaoUtils::GetStringVector(intVals);
    EXPECT_EQ(result.size(), 5);
    EXPECT_EQ(result[0], "1");
    EXPECT_EQ(result[1], "2");
    EXPECT_EQ(result[2], "3");
    EXPECT_EQ(result[3], "4");
    EXPECT_EQ(result[4], "5");
}

HWTEST_F(CloudMediaDaoUtilsTest, GetStringVector_NegativeNumbers, TestSize.Level1)
{
    std::vector<int32_t> intVals = {-1, -2, -3};
    auto result = CloudMediaDaoUtils::GetStringVector(intVals);
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "-1");
    EXPECT_EQ(result[1], "-2");
    EXPECT_EQ(result[2], "-3");
}

HWTEST_F(CloudMediaDaoUtilsTest, GetStringVector_Zero, TestSize.Level1)
{
    std::vector<int32_t> intVals = {0};
    auto result = CloudMediaDaoUtils::GetStringVector(intVals);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "0");
}

HWTEST_F(CloudMediaDaoUtilsTest, GetStringVector_LargeNumbers, TestSize.Level1)
{
    std::vector<int32_t> intVals = {INT_MAX, INT_MIN};
    auto result = CloudMediaDaoUtils::GetStringVector(intVals);
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], "2147483647");
    EXPECT_EQ(result[1], "-2147483648");
}

HWTEST_F(CloudMediaDaoUtilsTest, VectorToString_EmptyVector, TestSize.Level1)
{
    std::vector<uint64_t> vec;
    std::string result = CloudMediaDaoUtils::VectorToString(vec);
    EXPECT_EQ(result, "[]");
}

HWTEST_F(CloudMediaDaoUtilsTest, VectorToString_SingleElement, TestSize.Level1)
{
    std::vector<uint64_t> vec = {1};
    std::string result = CloudMediaDaoUtils::VectorToString(vec);
    EXPECT_EQ(result, "[1]");
}

HWTEST_F(CloudMediaDaoUtilsTest, VectorToString_MultipleElements, TestSize.Level1)
{
    std::vector<uint64_t> vec = {1, 2, 3, 4, 5};
    std::string result = CloudMediaDaoUtils::VectorToString(vec);
    EXPECT_EQ(result, "[1, 2, 3, 4, 5]");
}

HWTEST_F(CloudMediaDaoUtilsTest, VectorToString_CustomSeparator, TestSize.Level1)
{
    std::vector<uint64_t> vec = {1, 2, 3};
    std::string result = CloudMediaDaoUtils::VectorToString(vec, "|");
    EXPECT_EQ(result, "[1|2|3]");
}

HWTEST_F(CloudMediaDaoUtilsTest, VectorToString_LargeNumbers, TestSize.Level1)
{
    std::vector<uint64_t> vec = {18446744073709551615ULL, 999999999999ULL};
    std::string result = CloudMediaDaoUtils::VectorToString(vec);
    EXPECT_EQ(result, "[18446744073709551615, 999999999999]");
}

HWTEST_F(CloudMediaDaoUtilsTest, VectorToString_Zero, TestSize.Level1)
{
    std::vector<uint64_t> vec = {0};
    std::string result = CloudMediaDaoUtils::VectorToString(vec);
    EXPECT_EQ(result, "[0]");
}

HWTEST_F(CloudMediaDaoUtilsTest, VectorToString_MultipleZeros, TestSize.Level1)
{
    std::vector<uint64_t> vec = {0, 0, 0};
    std::string result = CloudMediaDaoUtils::VectorToString(vec);
    EXPECT_EQ(result, "[0, 0, 0]");
}

HWTEST_F(CloudMediaDaoUtilsTest, VectorToString_EmptySeparator, TestSize.Level1)
{
    std::vector<uint64_t> vec = {1, 2, 3};
    std::string result = CloudMediaDaoUtils::VectorToString(vec, "");
    EXPECT_EQ(result, "[123]");
}

HWTEST_F(CloudMediaDaoUtilsTest, VectorToString_LongSeparator, TestSize.Level1)
{
    std::vector<uint64_t> vec = {1, 2, 3};
    std::string result = CloudMediaDaoUtils::VectorToString(vec, "SEPARATOR");
    EXPECT_EQ(result, "[1SEPARATOR2SEPARATOR3]");
}
HWTEST_F(CloudMediaDaoUtilsTest, ToStringWithCommaAndQuote_PathWithBraces, TestSize.Level1)
{
    std::vector<std::string> values;
    values.emplace_back("/test{path}/file{name}");
    std::string result = CloudMediaDaoUtils::ToStringWithCommaAndQuote(values);
    EXPECT_EQ(result, "'/test{path}/file{name}'");
}
}  // namespace OHOS::Media::CloudSync

/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#include <cstdint>

#include "medialibrary_common_utils_test.h"
#include "medialibrary_errno.h"
#include "thumbnail_utils.h"
#define private public
#include "medialibrary_common_utils.h"
#undef private

using namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {
void MediaLibraryCommonUtilsTest::SetUpTestCase(void) {}

void MediaLibraryCommonUtilsTest::TearDownTestCase(void) {}

// SetUp:Execute before each test case
void MediaLibraryCommonUtilsTest::SetUp() {}

void MediaLibraryCommonUtilsTest::TearDown(void) {}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_GenKeySHA256_test_001, TestSize.Level0)
{
    vector<uint8_t> input;
    string key = "";
    int32_t ret = MediaLibraryCommonUtils::GenKeySHA256(input, key);
    EXPECT_EQ(ret, -EINVAL);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_GenKeySHA256_test_002, TestSize.Level0)
{
    string input = "";
    string key = "";
    int32_t ret = MediaLibraryCommonUtils::GenKeySHA256(input, key);
    EXPECT_EQ(ret, -EINVAL);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_CheckWhereClause_test_001, TestSize.Level0)
{
    string whereClause = "";
    bool ret = MediaLibraryCommonUtils::CheckWhereClause(whereClause);
    EXPECT_EQ(ret, true);
    string whereClauseTest = "CheckWhereClause";
    ret = MediaLibraryCommonUtils::CheckWhereClause(whereClauseTest);
    EXPECT_EQ(ret, false);
    string where = "CheckWhereClause;";
    ret = MediaLibraryCommonUtils::CheckWhereClause(where);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_AppendSelections_test_001, TestSize.Level0)
{
    string selections = "";
    MediaLibraryCommonUtils::AppendSelections(selections);
    selections = "AppendSelections";
    MediaLibraryCommonUtils::AppendSelections(selections);
    EXPECT_EQ(selections, "(AppendSelections)");
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_Char2Hex_test_001, TestSize.Level0)
{
    unsigned char hash[64] = "";
    size_t len = 2;
    string hexStr = "";
    MediaLibraryCommonUtils::Char2Hex(hash, len, hexStr);
    EXPECT_NE(hexStr, "");
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_GenKey_test_001, TestSize.Level0)
{
    unsigned char hash[64] = "";
    size_t len = 0;
    string key = "";
    int32_t ret = MediaLibraryCommonUtils::GenKey(hash, len, key);
    EXPECT_EQ(ret, -EINVAL);
    size_t lenTest = 2;
    ret = MediaLibraryCommonUtils::GenKey(hash, lenTest, key);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_CheckIllegalCharacter_test_001, TestSize.Level0)
{
    string strCondition = "";
    bool ret = MediaLibraryCommonUtils::CheckIllegalCharacter(strCondition);
    EXPECT_EQ(ret, true);
    string strConditionTest = ";";
    ret = MediaLibraryCommonUtils::CheckIllegalCharacter(strConditionTest);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_CheckKeyWord_test_001, TestSize.Level0)
{
    string strCondition = "";
    bool ret = MediaLibraryCommonUtils::CheckKeyWord(strCondition);
    EXPECT_EQ(ret, true);
    string strConditionTest = "|\\s*insert\\s*|\\s*delete\\s*";
    ret = MediaLibraryCommonUtils::CheckKeyWord(strConditionTest);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_SeprateSelection_test_001, TestSize.Level0)
{
    string strCondition = "SeprateSelection";
    vector<string> sepratedStr;
    MediaLibraryCommonUtils::SeprateSelection(strCondition, sepratedStr);
    EXPECT_NE(sepratedStr.size(), 0);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_CheckExpressValidation_test_001, TestSize.Level0)
{
    vector<string> sepratedStr;
    bool ret = MediaLibraryCommonUtils::CheckExpressValidation(sepratedStr);
    EXPECT_EQ(ret, true);
    sepratedStr.push_back(">=");
    ret = MediaLibraryCommonUtils::CheckExpressValidation(sepratedStr);
    EXPECT_EQ(ret, true);
    sepratedStr.push_back("CheckExpressValidation");
    ret = MediaLibraryCommonUtils::CheckExpressValidation(sepratedStr);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_CheckExpressValidation_test_002, TestSize.Level0)
{
    vector<string> sepratedStr;
    sepratedStr.push_back("parent");
    bool ret = MediaLibraryCommonUtils::CheckExpressValidation(sepratedStr);
    EXPECT_EQ(ret, true);
    sepratedStr.push_back(">=date_added ");
    ret = MediaLibraryCommonUtils::CheckExpressValidation(sepratedStr);
    EXPECT_EQ(ret, true);
    sepratedStr.push_back(" date_added>=");
    ret = MediaLibraryCommonUtils::CheckExpressValidation(sepratedStr);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_CheckWhiteList_test_001, TestSize.Level0)
{
    string express = "";
    bool ret = MediaLibraryCommonUtils::CheckWhiteList(express);
    EXPECT_EQ(ret, false);
    string expressTest = "date_added";
    ret = MediaLibraryCommonUtils::CheckWhiteList(expressTest);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_ExtractKeyWord_test_001, TestSize.Level0)
{
    string str = "";
    MediaLibraryCommonUtils::ExtractKeyWord(str);
    str = ">=";
    MediaLibraryCommonUtils::ExtractKeyWord(str);
    EXPECT_EQ(str, "");
    str = ">=date_added";
    MediaLibraryCommonUtils::ExtractKeyWord(str);
    EXPECT_EQ(str, "date_added");
    str = "date_added>=";
    MediaLibraryCommonUtils::ExtractKeyWord(str);
    EXPECT_EQ(str, "date_added");
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_RemoveSpecialCondition_test_001, TestSize.Level0)
{
    string hacker = "";
    MediaLibraryCommonUtils::RemoveSpecialCondition(hacker);
    hacker = "RemoveSpecialConditionnot between ? and ?";
    MediaLibraryCommonUtils::RemoveSpecialCondition(hacker);
    EXPECT_EQ(hacker, "RemoveSpecialCondition ");
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_RemoveSpecialCondition_test_002, TestSize.Level0)
{
    string hacker = "";
    string pattern = "?";
    MediaLibraryCommonUtils::RemoveSpecialCondition(hacker, pattern);
    hacker = "RemoveSpecialCondition?";
    MediaLibraryCommonUtils::RemoveSpecialCondition(hacker, pattern);
    EXPECT_EQ(hacker, "RemoveSpecialCondition ");
}
} // namespace Media
} // namespace OHOS
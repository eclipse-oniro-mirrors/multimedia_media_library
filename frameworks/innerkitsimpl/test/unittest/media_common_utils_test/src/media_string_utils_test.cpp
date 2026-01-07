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

} // namespace Media
} // namespace OHOS
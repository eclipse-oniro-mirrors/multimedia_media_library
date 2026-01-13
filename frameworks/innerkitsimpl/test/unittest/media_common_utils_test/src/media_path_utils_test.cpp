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

#include "media_path_utils_test.h"

#include "media_path_utils.h"

#include "media_log.h"

namespace OHOS {
namespace Media {
using namespace testing::ext;

void MediaPathUtilsUnitTest::SetUpTestCase(void) {}

void MediaPathUtilsUnitTest::TearDownTestCase(void) {}

// SetUp:Execute before each test case
void MediaPathUtilsUnitTest::SetUp() {}

void MediaPathUtilsUnitTest::TearDown(void) {}

HWTEST_F(MediaPathUtilsUnitTest, medialib_get_filename_test_001, TestSize.Level1)
{
    std::string filePath1 = "";
    EXPECT_EQ(MediaPathUtils::GetFileName(filePath1), "");

    std::string filePath2 = "test";
    EXPECT_EQ(MediaPathUtils::GetFileName(filePath2), "");

    std::string filePath3 = "test/";
    EXPECT_EQ(MediaPathUtils::GetFileName(filePath3), "");

    std::string filePath4 = "test/test";
    EXPECT_EQ(MediaPathUtils::GetFileName(filePath4), "test");
}

HWTEST_F(MediaPathUtilsUnitTest, medialib_get_extension_test_001, TestSize.Level1)
{
    std::string filePath1 = "";
    std::string resutl1 = MediaPathUtils::GetExtension(filePath1);
    EXPECT_EQ(resutl1, "");
    std::string filePath2 = "test";
    std::string resutl2 = MediaPathUtils::GetExtension(filePath2);
    EXPECT_EQ(resutl2, "");
    std::string filePath3 = "test/";
    std::string resutl3 = MediaPathUtils::GetExtension(filePath3);
    EXPECT_EQ(resutl3, "");
    std::string filePath4 = "test/test";
    std::string resutl4 = MediaPathUtils::GetExtension(filePath4);
    EXPECT_EQ(resutl4, "");
    std::string filePath5 = "test/test.jpg";
    std::string resutl5 = MediaPathUtils::GetExtension(filePath5);
    EXPECT_EQ(resutl5, "jpg");
    std::string filePath6 = ".test";
    std::string resutl6 = MediaPathUtils::GetExtension(filePath6);
    EXPECT_EQ(resutl6, "");
}

} // namespace Media
} // namespace OHOS
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

#include "media_pure_file_utils_test.h"

#include "media_pure_file_utils.h"


namespace OHOS {
namespace Media {
using namespace testing::ext;

void MediaPureFileUtilsUnitTest::SetUpTestCase(void) {}

void MediaPureFileUtilsUnitTest::TearDownTestCase(void) {}

// SetUp:Execute before each test case
void MediaPureFileUtilsUnitTest::SetUp() {}

void MediaPureFileUtilsUnitTest::TearDown(void) {}

HWTEST_F(MediaPureFileUtilsUnitTest, medialib_is_file_exist_test_001, TestSize.Level1)
{
    string filePath1 = "/data/test/isfileexists_001";
    EXPECT_EQ(MediaPureFileUtils::IsFileExists(filePath1), false);
    string filePath2 = "";
    EXPECT_EQ(MediaPureFileUtils::IsDirectory(filePath2), false);
    string filePath3 = "";
    EXPECT_EQ(MediaPureFileUtils::DeleteDir(filePath3), false);
}
} // namespace Media
} // namespace OHOS
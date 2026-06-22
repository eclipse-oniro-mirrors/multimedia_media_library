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

#define MLOG_TAG "FolderScannerUtilsTest"

#include "folder_scanner_utils_test.h"

#include "folder_scanner_utils.h"
#include "scan_rule_config.h"
#include "file_const.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
using namespace testing::ext;

void FolderScannerUtilsTest::SetUpTestCase() {}
void FolderScannerUtilsTest::TearDownTestCase() {}
void FolderScannerUtilsTest::SetUp() {}
void FolderScannerUtilsTest::TearDown() {}

// IsCurrentOrParentDir Tests

/**
 * @tc.name: IsCurrentOrParentDir_Dot_001
 * @tc.desc: Test IsCurrentOrParentDir with "." returns true
 * @tc.type: FUNC
 */
HWTEST_F(FolderScannerUtilsTest, IsCurrentOrParentDir_Dot_001, TestSize.Level1)
{
    EXPECT_TRUE(FolderScannerUtils::IsCurrentOrParentDir("."));
}

/**
 * @tc.name: IsCurrentOrParentDir_DotDot_002
 * @tc.desc: Test IsCurrentOrParentDir with ".." returns true
 * @tc.type: FUNC
 */
HWTEST_F(FolderScannerUtilsTest, IsCurrentOrParentDir_DotDot_002, TestSize.Level1)
{
    EXPECT_TRUE(FolderScannerUtils::IsCurrentOrParentDir(".."));
}

/**
 * @tc.name: IsCurrentOrParentDir_NormalDir_003
 * @tc.desc: Test IsCurrentOrParentDir with normal directory name returns false
 * @tc.type: FUNC
 */
HWTEST_F(FolderScannerUtilsTest, IsCurrentOrParentDir_NormalDir_003, TestSize.Level1)
{
    EXPECT_FALSE(FolderScannerUtils::IsCurrentOrParentDir("Pictures"));
    EXPECT_FALSE(FolderScannerUtils::IsCurrentOrParentDir("DCIM"));
}

/**
 * @tc.name: IsCurrentOrParentDir_EmptyString_004
 * @tc.desc: Test IsCurrentOrParentDir with empty string returns false
 * @tc.type: FUNC
 */
HWTEST_F(FolderScannerUtilsTest, IsCurrentOrParentDir_EmptyString_004, TestSize.Level1)
{
    EXPECT_FALSE(FolderScannerUtils::IsCurrentOrParentDir(""));
}

// IsSkipCurrentFile Tests

/**
 * @tc.name: IsSkipCurrentFile_HiddenFile_SkipHiddenEnabled_001
 * @tc.desc: Test IsSkipCurrentFile with hidden file (starts with .) when skipHiddenFile=true
 * @tc.type: FUNC
 */
HWTEST_F(FolderScannerUtilsTest, IsSkipCurrentFile_HiddenFile_SkipHiddenEnabled_001, TestSize.Level1)
{
    ScanRuleConfig config;
    config.skipHiddenFile = true;
    bool result = FolderScannerUtils::IsSkipCurrentFile("/test/path/.hidden.jpg", config);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: IsSkipCurrentFile_HiddenFile_SkipHiddenDisabled_002
 * @tc.desc: Test IsSkipCurrentFile with hidden file when skipHiddenFile=false
 * @tc.type: FUNC
 */
HWTEST_F(FolderScannerUtilsTest, IsSkipCurrentFile_HiddenFile_SkipHiddenDisabled_002, TestSize.Level1)
{
    ScanRuleConfig config;
    config.skipHiddenFile = false;
    bool result = FolderScannerUtils::IsSkipCurrentFile("/test/path/.hidden.jpg", config);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: IsSkipCurrentFile_NormalFile_003
 * @tc.desc: Test IsSkipCurrentFile with normal file, should not be skipped
 * @tc.type: FUNC
 */
HWTEST_F(FolderScannerUtilsTest, IsSkipCurrentFile_NormalFile_003, TestSize.Level1)
{
    ScanRuleConfig config;
    config.skipHiddenFile = true;
    bool result = FolderScannerUtils::IsSkipCurrentFile("/test/path/photo.jpg", config);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: IsSkipCurrentFile_EmptyPath_004
 * @tc.desc: Test IsSkipCurrentFile with empty path, should be skipped
 * @tc.type: FUNC
 */
HWTEST_F(FolderScannerUtilsTest, IsSkipCurrentFile_EmptyPath_004, TestSize.Level1)
{
    ScanRuleConfig config;
    config.skipHiddenFile = true;
    bool result = FolderScannerUtils::IsSkipCurrentFile("", config);
    EXPECT_TRUE(result);
}

// IsSkipCurrentDirectory Tests

/**
 * @tc.name: IsSkipCurrentDirectory_EmptyPath_001
 * @tc.desc: Test IsSkipCurrentDirectory with empty path, should be skipped
 * @tc.type: FUNC
 */
HWTEST_F(FolderScannerUtilsTest, IsSkipCurrentDirectory_EmptyPath_001, TestSize.Level1)
{
    ScanRuleConfig config;
    bool result = FolderScannerUtils::IsSkipCurrentDirectory("", config);
    EXPECT_TRUE(result);
}

} // namespace Media
} // namespace OHOS

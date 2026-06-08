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

#define MLOG_TAG "ScanRuleConfigTest"

#include "scan_rule_config_test.h"

#include <regex>

#include "lake_scan_rule_config.h"
#include "file_manager_scan_rule_config.h"
#include "scan_rule_config.h"
#include "file_const.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
using namespace testing::ext;

void ScanRuleConfigTest::SetUpTestCase() {}
void ScanRuleConfigTest::TearDownTestCase() {}
void ScanRuleConfigTest::SetUp() {}
void ScanRuleConfigTest::TearDown() {}

// ==================== Lake Scan Rule Config Tests ====================

/**
 * @tc.name: LakeConfig_RootPath_001
 * @tc.desc: Test Lake scan rule config has correct root path
 * @tc.type: FUNC
 */
HWTEST_F(ScanRuleConfigTest, LakeConfig_RootPath_001, TestSize.Level1)
{
    const ScanRuleConfig &config = GetLakeScanRuleConfig();
    EXPECT_EQ(config.rootPath, std::string(LAKE_ROOT_PATH));
    EXPECT_EQ(config.rootPath, "/storage/media/local/files/Docs/HO_DATA_EXT_MISC");
}

/**
 * @tc.name: LakeConfig_DefaultFolderNames_004
 * @tc.desc: Test Lake scan rule config has expected default folder names
 * @tc.type: FUNC
 */
HWTEST_F(ScanRuleConfigTest, LakeConfig_DefaultFolderNames_004, TestSize.Level1)
{
    const ScanRuleConfig &config = GetLakeScanRuleConfig();
    EXPECT_FALSE(config.defaultFolderNames.empty());
    EXPECT_TRUE(config.defaultFolderNames.count("Music") > 0);
    EXPECT_TRUE(config.defaultFolderNames.count("Pictures") > 0);
    EXPECT_TRUE(config.defaultFolderNames.count("Movies") > 0);
    EXPECT_TRUE(config.defaultFolderNames.count("Download") > 0);
    EXPECT_TRUE(config.defaultFolderNames.count("DCIM") > 0);
    EXPECT_TRUE(config.defaultFolderNames.count("Documents") > 0);
}

/**
 * @tc.name: LakeConfig_TencentCachePattern_005
 * @tc.desc: Test Lake tencent cache regex matches expected pattern
 * @tc.type: FUNC
 */
HWTEST_F(ScanRuleConfigTest, LakeConfig_TencentCachePattern_005, TestSize.Level1)
{
    const ScanRuleConfig &config = GetLakeScanRuleConfig();
    std::string testPath =
        "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/Tencent/MicroMsg/abcdef0123456789abcdef0123456789";
    EXPECT_TRUE(std::regex_match(testPath, config.tencentCachePattern));
}

/**
 * @tc.name: LakeConfig_VisiblePattern_006
 * @tc.desc: Test Lake visible regex matches only the exact root path
 * @tc.type: FUNC
 */
HWTEST_F(ScanRuleConfigTest, LakeConfig_VisiblePattern_006, TestSize.Level1)
{
    const ScanRuleConfig &config = GetLakeScanRuleConfig();
    EXPECT_TRUE(std::regex_match(
        std::string("/storage/media/local/files/Docs/HO_DATA_EXT_MISC"), config.visiblePattern));
    EXPECT_FALSE(std::regex_match(
        std::string("/storage/media/local/files/Docs/HO_DATA_EXT_MISC/Pictures"), config.visiblePattern));
}

// ==================== File Manager Scan Rule Config Tests ====================

/**
 * @tc.name: FMConfig_RootPath_001
 * @tc.desc: Test File Manager scan rule config has correct root path
 * @tc.type: FUNC
 */
HWTEST_F(ScanRuleConfigTest, FMConfig_RootPath_001, TestSize.Level1)
{
    const ScanRuleConfig &config = GetFileManagerScanRuleConfig();
    EXPECT_EQ(config.rootPath, std::string(FILE_MANAGER_ROOT_PATH));
    EXPECT_EQ(config.rootPath, "/storage/media/local/files/Docs");
}

/**
 * @tc.name: FMConfig_DefaultFolderNames_Empty_004
 * @tc.desc: Test File Manager config has empty default folder names
 * @tc.type: FUNC
 */
HWTEST_F(ScanRuleConfigTest, FMConfig_DefaultFolderNames_Empty_004, TestSize.Level1)
{
    const ScanRuleConfig &config = GetFileManagerScanRuleConfig();
    EXPECT_TRUE(config.defaultFolderNames.empty());
}

/**
 * @tc.name: FMConfig_RelativePathPattern_005
 * @tc.desc: Test File Manager relative path regex matches Docs prefix
 * @tc.type: FUNC
 */
HWTEST_F(ScanRuleConfigTest, FMConfig_RelativePathPattern_005, TestSize.Level1)
{
    const ScanRuleConfig &config = GetFileManagerScanRuleConfig();
    EXPECT_TRUE(std::regex_search(
        std::string("/storage/media/local/files/Docs/Music"), config.relativePathPattern));
    EXPECT_FALSE(std::regex_search(
        std::string("/storage/media/local/files/Other"), config.relativePathPattern));
}

// ==================== Constant Tests ====================

/**
 * @tc.name: Constants_RootPathValues_001
 * @tc.desc: Test root path constant string_view values
 * @tc.type: FUNC
 */
HWTEST_F(ScanRuleConfigTest, Constants_RootPathValues_001, TestSize.Level1)
{
    EXPECT_EQ(std::string(FILE_MANAGER_ROOT_PATH), "/storage/media/local/files/Docs");
    EXPECT_EQ(std::string(LAKE_ROOT_PATH), "/storage/media/local/files/Docs/HO_DATA_EXT_MISC");
    // Lake root is a sub-path of file manager root
    EXPECT_TRUE(std::string(LAKE_ROOT_PATH).find(std::string(FILE_MANAGER_ROOT_PATH)) == 0);
}

} // namespace Media
} // namespace OHOS

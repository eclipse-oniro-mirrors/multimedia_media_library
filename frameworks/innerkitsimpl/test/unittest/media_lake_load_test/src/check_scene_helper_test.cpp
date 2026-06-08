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

#define MLOG_TAG "CheckSceneHelperTest"

#include "check_scene_helper_test.h"

#include "check_scene_helper.h"
#include "check_scene.h"
#include "file_const.h"
#include "lake_scan_rule_config.h"
#include "file_manager_scan_rule_config.h"
#include "scan_rule_config.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
using namespace testing::ext;

void CheckSceneHelperTest::SetUpTestCase() {}
void CheckSceneHelperTest::TearDownTestCase() {}
void CheckSceneHelperTest::SetUp() {}
void CheckSceneHelperTest::TearDown() {}

// ==================== ResolveSceneByPath Tests ====================

/**
 * @tc.name: ResolveSceneByPath_LakePath_001
 * @tc.desc: Test ResolveSceneByPath with lake root path prefix
 * @tc.type: FUNC
 */
HWTEST_F(CheckSceneHelperTest, ResolveSceneByPath_LakePath_001, TestSize.Level1)
{
    CheckScene scene = CheckSceneHelper::ResolveSceneByPath(
        "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/Pictures/test.jpg");
    EXPECT_EQ(scene, CheckScene::LAKE);
}

/**
 * @tc.name: ResolveSceneByPath_FileManagerPath_002
 * @tc.desc: Test ResolveSceneByPath with file manager root path prefix (not in lake)
 * @tc.type: FUNC
 */
HWTEST_F(CheckSceneHelperTest, ResolveSceneByPath_FileManagerPath_002, TestSize.Level1)
{
    // A path under Docs but NOT under HO_DATA_EXT_MISC should be FILE_MANAGER
    CheckScene scene = CheckSceneHelper::ResolveSceneByPath(
        "/storage/media/local/files/Docs/Music/song.mp3");
    EXPECT_EQ(scene, CheckScene::FILE_MANAGER);
}

/**
 * @tc.name: ResolveSceneByPath_UnknownPath_003
 * @tc.desc: Test ResolveSceneByPath with path not matching any known root
 * @tc.type: FUNC
 */
HWTEST_F(CheckSceneHelperTest, ResolveSceneByPath_UnknownPath_003, TestSize.Level1)
{
    CheckScene scene = CheckSceneHelper::ResolveSceneByPath(
        "/data/service/other/path");
    EXPECT_EQ(scene, CheckScene::UNKNOWN);
}

/**
 * @tc.name: ResolveSceneByPath_EmptyPath_004
 * @tc.desc: Test ResolveSceneByPath with empty path
 * @tc.type: FUNC
 */
HWTEST_F(CheckSceneHelperTest, ResolveSceneByPath_EmptyPath_004, TestSize.Level1)
{
    CheckScene scene = CheckSceneHelper::ResolveSceneByPath("");
    EXPECT_EQ(scene, CheckScene::UNKNOWN);
}

/**
 * @tc.name: ResolveSceneByPath_LakeExactRoot_005
 * @tc.desc: Test ResolveSceneByPath with exact lake root path
 * @tc.type: FUNC
 */
HWTEST_F(CheckSceneHelperTest, ResolveSceneByPath_LakeExactRoot_005, TestSize.Level1)
{
    CheckScene scene = CheckSceneHelper::ResolveSceneByPath(
        "/storage/media/local/files/Docs/HO_DATA_EXT_MISC");
    EXPECT_EQ(scene, CheckScene::LAKE);
}

// ==================== GetScanRuleConfig Tests ====================

/**
 * @tc.name: GetScanRuleConfig_LakeScene_001
 * @tc.desc: Test GetScanRuleConfig returns lake config for LAKE scene
 * @tc.type: FUNC
 */
HWTEST_F(CheckSceneHelperTest, GetScanRuleConfig_LakeScene_001, TestSize.Level1)
{
    const ScanRuleConfig &config = CheckSceneHelper::GetScanRuleConfig(CheckScene::LAKE);
    EXPECT_EQ(config.rootPath, std::string(LAKE_ROOT_PATH));
    EXPECT_TRUE(config.skipHiddenFile);
    EXPECT_TRUE(config.skipHiddenDirectory);
    EXPECT_TRUE(config.skipBlackList);
    EXPECT_TRUE(config.skipTencentCache);
}

/**
 * @tc.name: GetScanRuleConfig_FileManagerScene_002
 * @tc.desc: Test GetScanRuleConfig returns file manager config for FILE_MANAGER scene
 * @tc.type: FUNC
 */
HWTEST_F(CheckSceneHelperTest, GetScanRuleConfig_FileManagerScene_002, TestSize.Level1)
{
    const ScanRuleConfig &config = CheckSceneHelper::GetScanRuleConfig(CheckScene::FILE_MANAGER);
    EXPECT_EQ(config.rootPath, std::string(FILE_MANAGER_ROOT_PATH));
    EXPECT_FALSE(config.skipHiddenFile);
    EXPECT_FALSE(config.skipHiddenDirectory);
    EXPECT_TRUE(config.skipBlackList);
    EXPECT_FALSE(config.skipTencentCache);
}

/**
 * @tc.name: GetScanRuleConfig_UnknownScene_003
 * @tc.desc: Test GetScanRuleConfig returns default config for UNKNOWN scene
 * @tc.type: FUNC
 */
HWTEST_F(CheckSceneHelperTest, GetScanRuleConfig_UnknownScene_003, TestSize.Level1)
{
    const ScanRuleConfig &config = CheckSceneHelper::GetScanRuleConfig(CheckScene::UNKNOWN);
    // Default config: empty root path, default boolean values
    EXPECT_TRUE(config.rootPath.empty());
}

// ==================== GetFileSourceType Tests ====================

/**
 * @tc.name: GetFileSourceType_LakeScene_001
 * @tc.desc: Test GetFileSourceType returns MEDIA_HO_LAKE(3) for LAKE scene
 * @tc.type: FUNC
 */
HWTEST_F(CheckSceneHelperTest, GetFileSourceType_LakeScene_001, TestSize.Level1)
{
    int32_t type = CheckSceneHelper::GetFileSourceType(CheckScene::LAKE);
    EXPECT_EQ(type, 3); // MEDIA_HO_LAKE
}

/**
 * @tc.name: GetFileSourceType_FileManagerScene_002
 * @tc.desc: Test GetFileSourceType returns FILE_MANAGER(1) for FILE_MANAGER scene
 * @tc.type: FUNC
 */
HWTEST_F(CheckSceneHelperTest, GetFileSourceType_FileManagerScene_002, TestSize.Level1)
{
    int32_t type = CheckSceneHelper::GetFileSourceType(CheckScene::FILE_MANAGER);
    EXPECT_EQ(type, 1); // FILE_MANAGER
}

/**
 * @tc.name: GetFileSourceType_UnknownScene_003
 * @tc.desc: Test GetFileSourceType returns MEDIA(0) for UNKNOWN/default scene
 * @tc.type: FUNC
 */
HWTEST_F(CheckSceneHelperTest, GetFileSourceType_UnknownScene_003, TestSize.Level1)
{
    int32_t type = CheckSceneHelper::GetFileSourceType(CheckScene::UNKNOWN);
    EXPECT_EQ(type, 0); // MEDIA
}

/**
 * @tc.name: GetFileSourceType_IdleScene_004
 * @tc.desc: Test GetFileSourceType returns MEDIA(0) for IDLE scene
 * @tc.type: FUNC
 */
HWTEST_F(CheckSceneHelperTest, GetFileSourceType_IdleScene_004, TestSize.Level1)
{
    int32_t type = CheckSceneHelper::GetFileSourceType(CheckScene::IDLE);
    EXPECT_EQ(type, 0); // MEDIA (default)
}

// ==================== CheckScene Enum Tests ====================

/**
 * @tc.name: CheckScene_EnumValues_001
 * @tc.desc: Test CheckScene enum values
 * @tc.type: FUNC
 */
HWTEST_F(CheckSceneHelperTest, CheckScene_EnumValues_001, TestSize.Level1)
{
    EXPECT_EQ(static_cast<int>(CheckScene::UNKNOWN), 0);
    EXPECT_EQ(static_cast<int>(CheckScene::IDLE), 1);
    EXPECT_EQ(static_cast<int>(CheckScene::LAKE), 2);
    EXPECT_EQ(static_cast<int>(CheckScene::FILE_MANAGER), 3);
}

// ==================== CreateFileParser Tests ====================

/**
 * @tc.name: CreateFileParser_UnknownScene_ReturnsNull_001
 * @tc.desc: Test CreateFileParser with UNKNOWN scene returns nullptr
 * @tc.type: FUNC
 */
HWTEST_F(CheckSceneHelperTest, CreateFileParser_UnknownScene_ReturnsNull_001, TestSize.Level1)
{
    auto parser = CheckSceneHelper::CreateFileParser(
        CheckScene::UNKNOWN, "/test/path", ScanMode::INCREMENT);
    EXPECT_EQ(parser, nullptr);
}

/**
 * @tc.name: CreateFolderParser_UnknownScene_ReturnsNull_002
 * @tc.desc: Test CreateFolderParser with UNKNOWN scene returns nullptr
 * @tc.type: FUNC
 */
HWTEST_F(CheckSceneHelperTest, CreateFolderParser_UnknownScene_ReturnsNull_002, TestSize.Level1)
{
    auto parser = CheckSceneHelper::CreateFolderParser(
        CheckScene::UNKNOWN, "/test/path", ScanMode::INCREMENT);
    EXPECT_EQ(parser, nullptr);
}

} // namespace Media
} // namespace OHOS

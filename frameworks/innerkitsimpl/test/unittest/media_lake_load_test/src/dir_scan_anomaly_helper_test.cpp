/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "DirScanAnomalyHelperTest"

#include <filesystem>
#include <fstream>
#include <memory>

#include "dir_scan_anomaly_helper_test.h"

#include "dir_scan_anomaly_helper.h"
#include "media_log.h"
#include "medialibrary_errno.h"

namespace OHOS::Media {
using namespace testing::ext;

static const std::string TEST_DOWNLOAD_DIR = "/storage/media/local/files/Docs/Download";
static const std::string TEST_ROOT = TEST_DOWNLOAD_DIR + "/test_anomaly_dir";
static const std::string TEST_DIR_A = TEST_ROOT + "/A";
static const std::string TEST_DIR_B = TEST_ROOT + "/B";
static const std::string TEST_DIR_A_SUB = TEST_DIR_A + "/sub";
static const std::string TEST_NON_DOCS = "/data/test/non_docs_dir";

void DirScanAnomalyHelperTest::SetUpTestCase()
{
    std::error_code ec;
    std::filesystem::create_directories(TEST_DIR_A + "/sub", ec);
    std::filesystem::create_directories(TEST_DIR_B, ec);
}

void DirScanAnomalyHelperTest::TearDownTestCase()
{
    std::error_code ec;
    std::filesystem::remove_all(TEST_ROOT, ec);
}

void DirScanAnomalyHelperTest::SetUp()
{
    DirScanAnomalyHelper::ClearAll();
}

void DirScanAnomalyHelperTest::TearDown()
{
    DirScanAnomalyHelper::ClearAll();
}

HWTEST_F(DirScanAnomalyHelperTest, AddAnomalyDir_Basic, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start AddAnomalyDir_Basic");
    int32_t ret = DirScanAnomalyHelper::AddAnomalyDir(TEST_DIR_A);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(DirScanAnomalyHelper::MatchesAnomalyDir(TEST_DIR_A));
    MEDIA_INFO_LOG("End AddAnomalyDir_Basic");
}

HWTEST_F(DirScanAnomalyHelperTest, AddAnomalyDir_Dedup, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start AddAnomalyDir_Dedup");
    DirScanAnomalyHelper::AddAnomalyDir(TEST_DIR_A);
    DirScanAnomalyHelper::AddAnomalyDir(TEST_DIR_A);  // 重复写入
    DirScanAnomalyHelper::AddAnomalyDir(TEST_DIR_B);

    // 去重不影响存在性
    EXPECT_TRUE(DirScanAnomalyHelper::MatchesAnomalyDir(TEST_DIR_A));
    EXPECT_TRUE(DirScanAnomalyHelper::MatchesAnomalyDir(TEST_DIR_B));
    MEDIA_INFO_LOG("End AddAnomalyDir_Dedup");
}

HWTEST_F(DirScanAnomalyHelperTest, MatchesAnomalyDir, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start MatchesAnomalyDir");
    EXPECT_FALSE(DirScanAnomalyHelper::MatchesAnomalyDir(TEST_DIR_A));

    DirScanAnomalyHelper::AddAnomalyDir(TEST_DIR_A);
    DirScanAnomalyHelper::AddAnomalyDir(TEST_DIR_A_SUB);
    DirScanAnomalyHelper::AddAnomalyDir(TEST_DIR_B);

    // 精确匹配
    EXPECT_TRUE(DirScanAnomalyHelper::MatchesAnomalyDir(TEST_DIR_A));
    EXPECT_TRUE(DirScanAnomalyHelper::MatchesAnomalyDir(TEST_DIR_B));
    // 非存在路径不命中
    EXPECT_FALSE(DirScanAnomalyHelper::MatchesAnomalyDir(TEST_ROOT + "/other_path"));
    MEDIA_INFO_LOG("End MatchesAnomalyDir");
}

HWTEST_F(DirScanAnomalyHelperTest, MatchesAnomalyDir_Prefix, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start MatchesAnomalyDir_Prefix");
    DirScanAnomalyHelper::AddAnomalyDir(TEST_DIR_A_SUB);

    // 父目录匹配子目录异常
    EXPECT_TRUE(DirScanAnomalyHelper::MatchesAnomalyDir(TEST_DIR_A));
    // 不同前缀不匹配
    EXPECT_FALSE(DirScanAnomalyHelper::MatchesAnomalyDir(TEST_DIR_B));
    // 前缀边界：DirA 不应匹配 DirAB（要求 DirA/ 前缀）
    std::string base = TEST_ROOT + "/DirA";
    std::string other = TEST_ROOT + "/DirAB";
    std::filesystem::create_directories(other);
    DirScanAnomalyHelper::AddAnomalyDir(other);
    EXPECT_FALSE(DirScanAnomalyHelper::MatchesAnomalyDir(base));
    MEDIA_INFO_LOG("End MatchesAnomalyDir_Prefix");
}

HWTEST_F(DirScanAnomalyHelperTest, RemoveAnomalyDir, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start RemoveAnomalyDir");
    DirScanAnomalyHelper::AddAnomalyDir(TEST_DIR_A);
    DirScanAnomalyHelper::AddAnomalyDir(TEST_DIR_B);

    DirScanAnomalyHelper::RemoveAnomalyDir(TEST_DIR_A);
    EXPECT_FALSE(DirScanAnomalyHelper::MatchesAnomalyDir(TEST_DIR_A));
    EXPECT_TRUE(DirScanAnomalyHelper::MatchesAnomalyDir(TEST_DIR_B));
    // 删除不存在的记录不崩溃
    DirScanAnomalyHelper::RemoveAnomalyDir("/nonexistent/path");
    MEDIA_INFO_LOG("End RemoveAnomalyDir");
}

HWTEST_F(DirScanAnomalyHelperTest, RemoveAnomalyDir_Prefix, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start RemoveAnomalyDir_Prefix");
    // 场景：扫描 A/sub/1.jpg 丢失 -> 记录 A/sub；重命名 A -> C，RemoveAnomalyDir(A) 应连带清理 A/sub
    DirScanAnomalyHelper::AddAnomalyDir(TEST_DIR_A_SUB);
    DirScanAnomalyHelper::AddAnomalyDir(TEST_DIR_B);

    // 删除父目录 A，应连带清理子目录 A/sub（前缀匹配，与 MatchesAnomalyDir 对称）
    DirScanAnomalyHelper::RemoveAnomalyDir(TEST_DIR_A);
    EXPECT_FALSE(DirScanAnomalyHelper::MatchesAnomalyDir(TEST_DIR_A));
    EXPECT_FALSE(DirScanAnomalyHelper::MatchesAnomalyDir(TEST_DIR_A_SUB));
    // 兄弟目录 B 不受影响
    EXPECT_TRUE(DirScanAnomalyHelper::MatchesAnomalyDir(TEST_DIR_B));

    // 前缀边界：RemoveAnomalyDir(DirA) 不应误删 DirAB（要求 DirA/ 前缀）
    std::string base = TEST_ROOT + "/DirA";
    std::string other = TEST_ROOT + "/DirAB";
    std::filesystem::create_directories(other);
    DirScanAnomalyHelper::AddAnomalyDir(other);
    DirScanAnomalyHelper::RemoveAnomalyDir(base);
    EXPECT_TRUE(DirScanAnomalyHelper::MatchesAnomalyDir(other));
    MEDIA_INFO_LOG("End RemoveAnomalyDir_Prefix");
}

HWTEST_F(DirScanAnomalyHelperTest, ClearAll_Basic, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start ClearAll_Basic");
    DirScanAnomalyHelper::AddAnomalyDir(TEST_DIR_A);
    DirScanAnomalyHelper::AddAnomalyDir(TEST_DIR_B);

    DirScanAnomalyHelper::ClearAll();
    EXPECT_FALSE(DirScanAnomalyHelper::MatchesAnomalyDir(TEST_DIR_A));
    EXPECT_FALSE(DirScanAnomalyHelper::MatchesAnomalyDir(TEST_DIR_B));
    MEDIA_INFO_LOG("End ClearAll_Basic");
}

HWTEST_F(DirScanAnomalyHelperTest, HandleRenameCompensation_Lifecycle, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start HandleRenameCompensation_Lifecycle");
    // 无异常记录 → MatchesAnomalyDir false → 直接 return，已入库资产正常重命名
    EXPECT_FALSE(DirScanAnomalyHelper::MatchesAnomalyDir(TEST_DIR_A));

    // 命中异常 → 存量执行中写 tab_file_opt / 存量未执行 FolderScanner.Run()
    // 两个分支最后都 RemoveAnomalyDir → 清理干净
    DirScanAnomalyHelper::AddAnomalyDir(TEST_DIR_A);
    EXPECT_TRUE(DirScanAnomalyHelper::MatchesAnomalyDir(TEST_DIR_A));
    DirScanAnomalyHelper::RemoveAnomalyDir(TEST_DIR_A);
    EXPECT_FALSE(DirScanAnomalyHelper::MatchesAnomalyDir(TEST_DIR_A));

    // 删除路径同样清理异常（DeleteFileManagerDirByFileManagerPath 内部调用）
    DirScanAnomalyHelper::AddAnomalyDir(TEST_DIR_A);
    DirScanAnomalyHelper::RemoveAnomalyDir(TEST_DIR_A);
    EXPECT_FALSE(DirScanAnomalyHelper::MatchesAnomalyDir(TEST_DIR_A));
    MEDIA_INFO_LOG("End HandleRenameCompensation_Lifecycle");
}

HWTEST_F(DirScanAnomalyHelperTest, IsFileManagerDir_Guard, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start IsFileManagerDir_Guard");
    // 非 Docs/Download 路径拒绝
    EXPECT_NE(DirScanAnomalyHelper::AddAnomalyDir(TEST_NON_DOCS), E_OK);
    EXPECT_FALSE(DirScanAnomalyHelper::MatchesAnomalyDir(TEST_NON_DOCS));
    // DownloadAAA 不应匹配 Download 前缀
    std::string downloadAAA = TEST_DOWNLOAD_DIR + "AAA";
    EXPECT_NE(DirScanAnomalyHelper::AddAnomalyDir(downloadAAA), E_OK);
    EXPECT_FALSE(DirScanAnomalyHelper::MatchesAnomalyDir(downloadAAA));
    // 大小写变体：通知路径大小写与常量不一致（hmdfs 等场景）也应被接受
    std::string mixedCaseDir = "/storage/media/local/files/Docs/download/case";
    EXPECT_EQ(DirScanAnomalyHelper::AddAnomalyDir(mixedCaseDir), E_OK);
    EXPECT_TRUE(DirScanAnomalyHelper::MatchesAnomalyDir(mixedCaseDir));
    MEDIA_INFO_LOG("End IsFileManagerDir_Guard");
}

HWTEST_F(DirScanAnomalyHelperTest, PathNormalization, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start PathNormalization");
    // 带 ../ 的路径规范化后匹配
    std::string normalized = TEST_DIR_A;
    std::string unnormalized = TEST_DIR_A + "/sub/..";
    DirScanAnomalyHelper::AddAnomalyDir(unnormalized);
    EXPECT_TRUE(DirScanAnomalyHelper::MatchesAnomalyDir(normalized));
    MEDIA_INFO_LOG("End PathNormalization");
}

} // namespace OHOS::Media

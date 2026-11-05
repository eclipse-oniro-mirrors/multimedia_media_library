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

#include <cstdio>
#include <memory>

#include "media_pure_file_utils.h"
#include "medialibrary_errno.h"

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
    string filePath = "/data/test/deletefile_001";
    EXPECT_EQ(MediaPureFileUtils::DeleteFile(filePath), false);
}

HWTEST_F(MediaPureFileUtilsUnitTest, medialib_create_directory_test_001, TestSize.Level1)
{
    std::string rootDir = "/data/test/media_pure_file_utils/mkdir_001";
    std::string dirPath = rootDir + "/sub/deep";
    // errCodePtr 只在 stat 失败时被写入，成功后不应残留错误码
    auto errCodePtr = std::make_shared<int>(E_ERR);
    EXPECT_TRUE(MediaPureFileUtils::CreateDirectory(dirPath, errCodePtr));
    *errCodePtr = E_ERR;
    EXPECT_TRUE(MediaPureFileUtils::CreateDirectory(dirPath, errCodePtr));
    EXPECT_EQ(*errCodePtr, E_ERR);
    EXPECT_TRUE(MediaPureFileUtils::IsDirectory(dirPath));
    // 目录已存在时再次创建仍返回成功
    EXPECT_TRUE(MediaPureFileUtils::CreateDirectory(dirPath));
    EXPECT_TRUE(MediaPureFileUtils::DeleteDir(rootDir));
    EXPECT_FALSE(MediaPureFileUtils::IsDirectory(dirPath));
}

HWTEST_F(MediaPureFileUtilsUnitTest, medialib_copy_file_util_test_001, TestSize.Level1)
{
    std::string rootDir = "/data/test/media_pure_file_utils/copy_001";
    ASSERT_TRUE(MediaPureFileUtils::CreateDirectory(rootDir));
    std::string srcPath = rootDir + "/source.txt";
    std::string dstPath = rootDir + "/dest.txt";

    FILE *fp = fopen(srcPath.c_str(), "w");
    ASSERT_NE(fp, nullptr);
    (void)fprintf(fp, "media library");
    (void)fclose(fp);

    EXPECT_TRUE(MediaPureFileUtils::CopyFileUtil(srcPath, dstPath));
    EXPECT_TRUE(MediaPureFileUtils::IsFileExists(dstPath));
    EXPECT_TRUE(MediaPureFileUtils::CopyFileUtil(srcPath, dstPath, true));
    // 源文件不存在
    EXPECT_FALSE(MediaPureFileUtils::CopyFileUtil(rootDir + "/not_exist.txt", dstPath));
    // 目标路径为空
    EXPECT_FALSE(MediaPureFileUtils::CopyFileUtil(srcPath, ""));
    EXPECT_TRUE(MediaPureFileUtils::DeleteDir(rootDir));
}

HWTEST_F(MediaPureFileUtilsUnitTest, medialib_print_stat_information_test_001, TestSize.Level1)
{
    std::string rootDir = "/data/test/media_pure_file_utils/stat_001";
    ASSERT_TRUE(MediaPureFileUtils::CreateDirectory(rootDir));
    std::string filePath = rootDir + "/stat.txt";
    FILE *fp = fopen(filePath.c_str(), "w");
    ASSERT_NE(fp, nullptr);
    (void)fclose(fp);
    // 仅验证存在/不存在两条分支都不会崩溃，日志为副作用
    MediaPureFileUtils::PrintStatInformation(filePath);
    MediaPureFileUtils::PrintStatInformation(rootDir);
    MediaPureFileUtils::PrintStatInformation(rootDir + "/not_exist.txt");
    EXPECT_TRUE(MediaPureFileUtils::DeleteDir(rootDir));
}
} // namespace Media
} // namespace OHOS
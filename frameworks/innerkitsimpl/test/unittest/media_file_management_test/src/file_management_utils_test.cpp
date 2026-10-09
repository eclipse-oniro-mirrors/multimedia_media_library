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

#include "media_file_management_test.h"

#include <fstream>
#include <string>
#include <vector>
#include <sys/stat.h>

#include "file_management_utils.h"

using namespace testing::ext;

namespace OHOS {
namespace Media {

void MediaFileManagementUtilsTest::SetUpTestCase(void) {}
void MediaFileManagementUtilsTest::TearDownTestCase(void) {}
void MediaFileManagementUtilsTest::SetUp() {}
void MediaFileManagementUtilsTest::TearDown() {}

/*
 * Feature : FileManagementUtils
 * Function : GetRelativeDir
 * SubFunction : NA
 * FunctionPoints : 验证路径长度不足时返回错误
 * EnvContions : NA
 * CaseDescription : target长度小于前缀长度时返回E_ERR
 */
HWTEST_F(MediaFileManagementUtilsTest, GetRelativeDir_ShortPath_001, TestSize.Level1)
{
    std::string target = "/storage/media/";
    std::string relativePath;
    int32_t ret = FileManagementUtils::GetRelativeDir(target, relativePath);
    EXPECT_EQ(ret, E_ERR);
}

/*
 * Feature : FileManagementUtils
 * Function : GetRelativeDir
 * SubFunction : NA
 * FunctionPoints : 验证路径不以正确前缀开头时返回错误
 * EnvContions : NA
 * CaseDescription : target不以FILE_MANAGEMENT_PREFIX开头时返回E_ERR
 */
HWTEST_F(MediaFileManagementUtilsTest, GetRelativeDir_WrongPrefix_002, TestSize.Level1)
{
    std::string target = "/storage/media/local/files/Other/somefolder/subdir";
    std::string relativePath;
    int32_t ret = FileManagementUtils::GetRelativeDir(target, relativePath);
    EXPECT_EQ(ret, E_ERR);
}

/*
 * Feature : FileManagementUtils
 * Function : GetRelativeDir
 * SubFunction : NA
 * FunctionPoints : 验证禁止前缀匹配时返回错误
 * EnvContions : NA
 * CaseDescription : target以forbidden prefix开头时返回E_ERR
 */
HWTEST_F(MediaFileManagementUtilsTest, GetRelativeDir_ForbiddenPrefix_003, TestSize.Level1)
{
    std::string target = "/storage/media/local/files/Docs/.Trash/subdir";
    std::string relativePath;
    int32_t ret = FileManagementUtils::GetRelativeDir(target, relativePath);
    EXPECT_EQ(ret, E_ERR);
}

/*
 * Feature : FileManagementUtils
 * Function : GetRelativeDir
 * SubFunction : NA
 * FunctionPoints : 验证另一个禁止前缀HO_DATA_EXT_MISC
 * EnvContions : NA
 * CaseDescription : target以HO_DATA_EXT_MISC前缀开头时返回E_ERR
 */
HWTEST_F(MediaFileManagementUtilsTest, GetRelativeDir_ForbiddenPrefix_004, TestSize.Level1)
{
    std::string target = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/data";
    std::string relativePath;
    int32_t ret = FileManagementUtils::GetRelativeDir(target, relativePath);
    EXPECT_EQ(ret, E_ERR);
}

/*
 * Feature : FileManagementUtils
 * Function : GetRelativeDir
 * SubFunction : NA
 * FunctionPoints : 验证正常路径无尾部斜杠
 * EnvContions : NA
 * CaseDescription : 正常路径解析，无尾部斜杠，返回E_OK并正确设置relativePath
 */
HWTEST_F(MediaFileManagementUtilsTest, GetRelativeDir_NormalPath_005, TestSize.Level1)
{
    std::string target = "/storage/media/local/files/Docs/folder/subdir";
    std::string relativePath;
    int32_t ret = FileManagementUtils::GetRelativeDir(target, relativePath);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(relativePath, "folder/subdir");
}

/*
 * Feature : FileManagementUtils
 * Function : GetRelativeDir
 * SubFunction : NA
 * FunctionPoints : 验证有尾部斜杠的路径
 * EnvContions : NA
 * CaseDescription : 路径和target都有尾部斜杠时应去除
 */
HWTEST_F(MediaFileManagementUtilsTest, GetRelativeDir_TrailingSlash_006, TestSize.Level1)
{
    std::string target = "/storage/media/local/files/Docs/folder/subdir/";
    std::string relativePath;
    int32_t ret = FileManagementUtils::GetRelativeDir(target, relativePath);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(relativePath, "folder/subdir");
    EXPECT_EQ(target, "/storage/media/local/files/Docs/folder/subdir");
}

/*
 * Feature : FileManagementUtils
 * Function : GetRelativeDir
 * SubFunction : NA
 * FunctionPoints : 验证正好等于前缀的路径
 * EnvContions : NA
 * CaseDescription : target正好等于前缀时relativePath应为空
 */
HWTEST_F(MediaFileManagementUtilsTest, GetRelativeDir_ExactPrefix_007, TestSize.Level1)
{
    std::string target = "/storage/media/local/files/Docs/";
    std::string relativePath;
    int32_t ret = FileManagementUtils::GetRelativeDir(target, relativePath);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(relativePath, "");
}

/*
 * Feature : FileManagementUtils
 * Function : GetLocalPath
 * SubFunction : NA
 * FunctionPoints : 验证包含cloud路径时替换为media路径
 * EnvContions : NA
 * CaseDescription : 路径包含/storage/cloud/files/时替换为/storage/media/local/files/
 */
HWTEST_F(MediaFileManagementUtilsTest, GetLocalPath_CloudPath_001, TestSize.Level1)
{
    std::string path = "/storage/cloud/files/images/test.jpg";
    std::string result = FileManagementUtils::GetLocalPath(path);
    EXPECT_EQ(result, "/storage/media/local/files/images/test.jpg");
}

/*
 * Feature : FileManagementUtils
 * Function : GetLocalPath
 * SubFunction : NA
 * FunctionPoints : 验证不含cloud路径时原样返回
 * EnvContions : NA
 * CaseDescription : 路径不包含/storage/cloud/files/时原样返回
 */
HWTEST_F(MediaFileManagementUtilsTest, GetLocalPath_NonCloudPath_002, TestSize.Level1)
{
    std::string path = "/storage/media/local/files/videos/test.mp4";
    std::string result = FileManagementUtils::GetLocalPath(path);
    EXPECT_EQ(result, "/storage/media/local/files/videos/test.mp4");
}

/*
 * Feature : FileManagementUtils
 * Function : ReplaceLastSegment
 * SubFunction : NA
 * FunctionPoints : 验证无斜杠时返回空字符串
 * EnvContions : NA
 * CaseDescription : data中不包含'/'时返回空字符串
 */
HWTEST_F(MediaFileManagementUtilsTest, ReplaceLastSegment_NoSlash_001, TestSize.Level1)
{
    std::string result = FileManagementUtils::ReplaceLastSegment("nodir", "newname");
    EXPECT_EQ(result, "");
}

/*
 * Feature : FileManagementUtils
 * Function : ReplaceLastSegment
 * SubFunction : NA
 * FunctionPoints : 验证正常替换最后一个路径段
 * EnvContions : NA
 * CaseDescription : 正常路径替换最后一个段为新的displayname
 */
HWTEST_F(MediaFileManagementUtilsTest, ReplaceLastSegment_NormalPath_002, TestSize.Level1)
{
    std::string result = FileManagementUtils::ReplaceLastSegment("/storage/media/files/old.jpg", "new.jpg");
    EXPECT_EQ(result, "/storage/media/files/new.jpg");
}

/*
 * Feature : FileManagementUtils
 * Function : ReplaceLastSegment
 * SubFunction : NA
 * FunctionPoints : 验证空data输入
 * EnvContions : NA
 * CaseDescription : data为空字符串（无斜杠）时返回空
 */
HWTEST_F(MediaFileManagementUtilsTest, ReplaceLastSegment_EmptyData_003, TestSize.Level1)
{
    std::string result = FileManagementUtils::ReplaceLastSegment("", "new.jpg");
    EXPECT_EQ(result, "");
}

/*
 * Feature : FileManagementUtils
 * Function : GetLastDirName
 * SubFunction : NA
 * FunctionPoints : 验证空路径返回空字符串
 * EnvContions : NA
 * CaseDescription : path为空时返回空字符串
 */
HWTEST_F(MediaFileManagementUtilsTest, GetLastDirName_EmptyPath_001, TestSize.Level1)
{
    std::string result = FileManagementUtils::GetLastDirName("");
    EXPECT_EQ(result, "");
}

/*
 * Feature : FileManagementUtils
 * Function : GetLastDirName
 * SubFunction : NA
 * FunctionPoints : 验证无斜杠路径返回整个路径
 * EnvContions : NA
 * CaseDescription : path不包含'/'时返回path本身
 */
HWTEST_F(MediaFileManagementUtilsTest, GetLastDirName_NoSlash_002, TestSize.Level1)
{
    std::string result = FileManagementUtils::GetLastDirName("singledir");
    EXPECT_EQ(result, "singledir");
}

/*
 * Feature : FileManagementUtils
 * Function : GetLastDirName
 * SubFunction : NA
 * FunctionPoints : 验证尾部有斜杠的路径
 * EnvContions : NA
 * CaseDescription : path以'/'结尾时，取倒数第二个斜杠后的段
 */
HWTEST_F(MediaFileManagementUtilsTest, GetLastDirName_TrailingSlash_003, TestSize.Level1)
{
    std::string result = FileManagementUtils::GetLastDirName("/storage/media/mydir/");
    EXPECT_EQ(result, "mydir/");
}

/*
 * Feature : FileManagementUtils
 * Function : GetLastDirName
 * SubFunction : NA
 * FunctionPoints : 验证正常路径
 * EnvContions : NA
 * CaseDescription : 正常路径取最后一个斜杠后的段
 */
HWTEST_F(MediaFileManagementUtilsTest, GetLastDirName_NormalPath_004, TestSize.Level1)
{
    std::string result = FileManagementUtils::GetLastDirName("/storage/media/files/photo.jpg");
    EXPECT_EQ(result, "photo.jpg");
}

/*
 * Feature : FileManagementUtils
 * Function : GetLastDirName
 * SubFunction : NA
 * FunctionPoints : 验证仅有一个斜杠的尾部路径
 * EnvContions : NA
 * CaseDescription : 路径为"/dirname/"时返回"dirname"
 */
HWTEST_F(MediaFileManagementUtilsTest, GetLastDirName_SingleTrailingSlash_005, TestSize.Level1)
{
    std::string result = FileManagementUtils::GetLastDirName("/dirname/");
    EXPECT_EQ(result, "dirname/");
}

/*
 * Feature : FileManagementUtils
 * Function : CalculateTotalSizeByPath
 * SubFunction : NA
 * FunctionPoints : 验证空vector返回0
 * EnvContions : NA
 * CaseDescription : 空路径列表时totalSize为0
 */
HWTEST_F(MediaFileManagementUtilsTest, CalculateTotalSizeByPath_Empty_001, TestSize.Level1)
{
    std::vector<std::string> paths;
    int64_t result = FileManagementUtils::CalculateTotalSizeByPath(paths);
    EXPECT_EQ(result, 0);
}

/*
 * Feature : FileManagementUtils
 * Function : CalculateTotalSizeByPath
 * SubFunction : NA
 * FunctionPoints : 验证空字符串路径被跳过
 * EnvContions : NA
 * CaseDescription : 包含空字符串路径时跳过并继续
 */
HWTEST_F(MediaFileManagementUtilsTest, CalculateTotalSizeByPath_EmptyString_002, TestSize.Level1)
{
    std::vector<std::string> paths = {"", "/nonexistent/path/file.dat"};
    int64_t result = FileManagementUtils::CalculateTotalSizeByPath(paths);
    EXPECT_EQ(result, 0);
}

/*
 * Feature : FileManagementUtils
 * Function : CalculateTotalSizeByPath
 * SubFunction : NA
 * FunctionPoints : 验证不存在的文件路径被跳过
 * EnvContions : NA
 * CaseDescription : 文件不存在时GetFileSize返回false，跳过该路径
 */
HWTEST_F(MediaFileManagementUtilsTest, CalculateTotalSizeByPath_NonexistentFile_003, TestSize.Level1)
{
    std::vector<std::string> paths = {"/data/local/tmp/nonexistent_file_12345.dat"};
    int64_t result = FileManagementUtils::CalculateTotalSizeByPath(paths);
    EXPECT_EQ(result, 0);
}

/*
 * Feature : FileManagementUtils
 * Function : GetFileMtime
 * SubFunction : NA
 * FunctionPoints : 验证不存在的文件返回E_ERR
 * EnvContions : NA
 * CaseDescription : stat()对不存在文件失败时返回E_ERR
 */
HWTEST_F(MediaFileManagementUtilsTest, GetFileMtime_NonexistentFile_001, TestSize.Level1)
{
    time_t mtime = 0;
    int32_t ret = FileManagementUtils::GetFileMtime("/data/local/tmp/nonexistent_file_99999.dat", mtime);
    EXPECT_EQ(ret, E_ERR);
}

/*
 * Feature : FileManagementUtils
 * Function : GetFileMtime
 * SubFunction : NA
 * FunctionPoints : 验证存在的文件返回E_OK
 * EnvContions : 需要在可写目录创建临时文件
 * CaseDescription : stat()成功时返回E_OK且mtime非零
 */
HWTEST_F(MediaFileManagementUtilsTest, GetFileMtime_ExistingFile_002, TestSize.Level1)
{
    std::string tmpPath = "/data/local/tmp/test_file_mtime.tmp";
    std::ofstream ofs(tmpPath);
    ASSERT_TRUE(ofs.is_open());
    ofs << "test content for mtime";
    ofs.close();

    time_t mtime = 0;
    int32_t ret = FileManagementUtils::GetFileMtime(tmpPath, mtime);
    EXPECT_EQ(ret, E_OK);
    EXPECT_GT(mtime, 0);

    remove(tmpPath.c_str());
}

} // namespace Media
} // namespace OHOS

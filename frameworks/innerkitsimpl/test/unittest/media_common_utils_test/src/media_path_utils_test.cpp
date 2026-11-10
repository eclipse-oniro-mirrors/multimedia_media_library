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

#include <cstdio>

#include "media_log.h"
#include "media_pure_file_utils.h"
#include "medialibrary_errno.h"

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

HWTEST_F(MediaPathUtilsUnitTest, AppendUserId_ValidPath_Test_001, TestSize.Level1)
{
    std::string path = "/storage/cloud/files/image.jpg";
    int32_t userId = 100;
    std::string result = MediaPathUtils::AppendUserId(path, userId);
    EXPECT_EQ(result, "/storage/cloud/100/files/image.jpg");
}

HWTEST_F(MediaPathUtilsUnitTest, AppendUserId_ValidPath_Test_002, TestSize.Level1)
{
    std::string path = "/storage/cloud/files/Photo/image.jpg";
    int32_t userId = 100;
    std::string result = MediaPathUtils::AppendUserId(path, userId);
    EXPECT_EQ(result, "/storage/cloud/100/files/Photo/image.jpg");
}

HWTEST_F(MediaPathUtilsUnitTest, AppendUserId_ValidPath_Test_003, TestSize.Level1)
{
    std::string path = "/storage/cloud/files/Video/video.mp4";
    int32_t userId = 200;
    std::string result = MediaPathUtils::AppendUserId(path, userId);
    EXPECT_EQ(result, "/storage/cloud/200/files/Video/video.mp4");
}

HWTEST_F(MediaPathUtilsUnitTest, AppendUserId_ValidPath_Test_004, TestSize.Level1)
{
    std::string path = "/storage/cloud/files/Audio/audio.mp3";
    int32_t userId = 300;
    std::string result = MediaPathUtils::AppendUserId(path, userId);
    EXPECT_EQ(result, "/storage/cloud/300/files/Audio/audio.mp3");
}

HWTEST_F(MediaPathUtilsUnitTest, AppendUserId_InvalidUserId_Test_001, TestSize.Level1)
{
    std::string path = "/storage/cloud/files/image.jpg";
    int32_t userId = -1;
    std::string result = MediaPathUtils::AppendUserId(path, userId);
    EXPECT_EQ(result, path);
}

HWTEST_F(MediaPathUtilsUnitTest, AppendUserId_InvalidUserId_Test_002, TestSize.Level1)
{
    std::string path = "/storage/cloud/files/image.jpg";
    int32_t userId = -100;
    std::string result = MediaPathUtils::AppendUserId(path, userId);
    EXPECT_EQ(result, path);
}

HWTEST_F(MediaPathUtilsUnitTest, AppendUserId_InvalidPath_Test_001, TestSize.Level1)
{
    std::string path = "/local/files/image.jpg";
    int32_t userId = 100;
    std::string result = MediaPathUtils::AppendUserId(path, userId);
    EXPECT_EQ(result, path);
}

HWTEST_F(MediaPathUtilsUnitTest, AppendUserId_InvalidPath_Test_002, TestSize.Level1)
{
    std::string path = "/storage/files/image.jpg";
    int32_t userId = 100;
    std::string result = MediaPathUtils::AppendUserId(path, userId);
    EXPECT_EQ(result, path);
}

HWTEST_F(MediaPathUtilsUnitTest, AppendUserId_EmptyPath_Test_001, TestSize.Level1)
{
    std::string path = "";
    int32_t userId = 100;
    std::string result = MediaPathUtils::AppendUserId(path, userId);
    EXPECT_EQ(result, path);
}

HWTEST_F(MediaPathUtilsUnitTest, CheckPhotoPath_ValidPath_Test_001, TestSize.Level1)
{
    std::string photoPath = "/storage/cloud/files/image.jpg";
    bool result = MediaPathUtils::CheckPhotoPath(photoPath);
    EXPECT_EQ(result, true);
}

HWTEST_F(MediaPathUtilsUnitTest, CheckPhotoPath_ValidPath_Test_002, TestSize.Level1)
{
    std::string photoPath = "/storage/cloud/files/Photo/image.jpg";
    bool result = MediaPathUtils::CheckPhotoPath(photoPath);
    EXPECT_EQ(result, true);
}

HWTEST_F(MediaPathUtilsUnitTest, CheckPhotoPath_ValidPath_Test_003, TestSize.Level1)
{
    std::string photoPath = "/storage/cloud/files/Video/video.mp4";
    bool result = MediaPathUtils::CheckPhotoPath(photoPath);
    EXPECT_EQ(result, true);
}

HWTEST_F(MediaPathUtilsUnitTest, CheckPhotoPath_InvalidPath_Test_001, TestSize.Level1)
{
    std::string photoPath = "/local/files/image.jpg";
    bool result = MediaPathUtils::CheckPhotoPath(photoPath);
    EXPECT_EQ(result, false);
}

HWTEST_F(MediaPathUtilsUnitTest, CheckPhotoPath_InvalidPath_Test_002, TestSize.Level1)
{
    std::string photoPath = "/storage/files/image.jpg";
    bool result = MediaPathUtils::CheckPhotoPath(photoPath);
    EXPECT_EQ(result, false);
}

HWTEST_F(MediaPathUtilsUnitTest, CheckPhotoPath_EmptyPath_Test_001, TestSize.Level1)
{
    std::string photoPath = "";
    bool result = MediaPathUtils::CheckPhotoPath(photoPath);
    EXPECT_EQ(result, false);
}

HWTEST_F(MediaPathUtilsUnitTest, CheckPhotoPath_ShortPath_Test_001, TestSize.Level1)
{
    std::string photoPath = "/storage";
    bool result = MediaPathUtils::CheckPhotoPath(photoPath);
    EXPECT_EQ(result, false);
}

HWTEST_F(MediaPathUtilsUnitTest, CheckPhotoPath_ShortPath_Test_002, TestSize.Level1)
{
    std::string photoPath = "/storage/cloud";
    bool result = MediaPathUtils::CheckPhotoPath(photoPath);
    EXPECT_EQ(result, false);
}

HWTEST_F(MediaPathUtilsUnitTest, CheckPhotoPath_ShortPath_Test_003, TestSize.Level1)
{
    std::string photoPath = "/storage/cloud/";
    bool result = MediaPathUtils::CheckPhotoPath(photoPath);
    EXPECT_EQ(result, false);
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_EmptyPath_Test_001, TestSize.Level1)
{
    std::string filePath = "";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_NoSlash_Test_001, TestSize.Level1)
{
    std::string filePath = "filename.txt";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_EndsWithSlash_Test_001, TestSize.Level1)
{
    std::string filePath = "path/to/dir/";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_SingleSlash_Test_001, TestSize.Level1)
{
    std::string filePath = "/filename.txt";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "filename.txt");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_MultipleSlashes_Test_001, TestSize.Level1)
{
    std::string filePath = "/path/to/filename.txt";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "filename.txt");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_DirectoryPath_Test_001, TestSize.Level1)
{
    std::string filePath = "/storage/cloud/files/Photo/";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_ImageFile_Test_001, TestSize.Level1)
{
    std::string filePath = "/storage/cloud/files/Photo/image.jpg";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "image.jpg");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_VideoFile_Test_001, TestSize.Level1)
{
    std::string filePath = "/storage/cloud/files/Video/video.mp4";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "video.mp4");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_AudioFile_Test_001, TestSize.Level1)
{
    std::string filePath = "/storage/cloud/files/Audio/audio.mp3";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "audio.mp3");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_HiddenFile_Test_001, TestSize.Level1)
{
    std::string filePath = "/storage/cloud/files/.hidden";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, ".hidden");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_FileWithDots_Test_001, TestSize.Level1)
{
    std::string filePath = "/storage/cloud/files/file.name.with.dots.txt";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "file.name.with.dots.txt");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_UnixPath_Test_001, TestSize.Level1)
{
    std::string filePath = "/usr/local/bin/file";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "file");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_RelativePath_Test_001, TestSize.Level1)
{
    std::string filePath = "relative/path/to/file.txt";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "file.txt");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_DocumentFile_Test_001, TestSize.Level1)
{
    std::string filePath = "/storage/cloud/files/Docs/document.pdf";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "document.pdf");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_DownloadFile_Test_001, TestSize.Level1)
{
    std::string filePath = "/storage/cloud/files/Download/file.zip";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "file.zip");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_LongFileName_Test_001, TestSize.Level1)
{
    std::string filePath = "/storage/cloud/files/very_long_filename_with_many_characters.jpg";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "very_long_filename_with_many_characters.jpg");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_SpecialChars_Test_001, TestSize.Level1)
{
    std::string filePath = "/storage/cloud/files/file-name_123.txt";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "file-name_123.txt");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_UnicodeChars_Test_001, TestSize.Level1)
{
    std::string filePath = "/storage/cloud/files/文件名.jpg";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "文件名.jpg");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_DoubleSlash_Test_001, TestSize.Level1)
{
    std::string filePath = "/path//to//file.txt";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "file.txt");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_TrailingSlash_Test_001, TestSize.Level1)
{
    std::string filePath = "/path/to/file.txt/";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_RootPath_Test_001, TestSize.Level1)
{
    std::string filePath = "/";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_CurrentDir_Test_001, TestSize.Level1)
{
    std::string filePath = ".";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_ParentDir_Test_001, TestSize.Level1)
{
    std::string filePath = "..";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_EmptyPath_Test_001, TestSize.Level1)
{
    std::string filePath = "";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_NoExtension_Test_001, TestSize.Level1)
{
    std::string filePath = "filename";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_DotAtStart_Test_001, TestSize.Level1)
{
    std::string filePath = ".hiddenfile";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_SingleDot_Test_001, TestSize.Level1)
{
    std::string filePath = ".";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_DoubleDot_Test_001, TestSize.Level1)
{
    std::string filePath = "..";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_UpperCase_Test_001, TestSize.Level1)
{
    std::string filePath = "image.JPG";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "jpg");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_MixedCase_Test_001, TestSize.Level1)
{
    std::string filePath = "image.JpG";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "jpg");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_MultipleDots_Test_001, TestSize.Level1)
{
    std::string filePath = "file.name.with.dots.txt";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "txt");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_PathWithExtension_Test_001, TestSize.Level1)
{
    std::string filePath = "/path/to/image.jpg";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "jpg");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_PathWithExtension_Test_002, TestSize.Level1)
{
    std::string filePath = "/storage/cloud/files/Photo/image.jpg";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "jpg");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_PathWithExtension_Test_0033, TestSize.Level1)
{
    std::string filePath = "/storage/cloud/files/Video/video.mp4";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "mp4");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_PathWithExtension_Test_004, TestSize.Level1)
{
    std::string filePath = "/storage/cloud/files/Audio/audio.mp3";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "mp3");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_PathWithoutExtension_Test_001, TestSize.Level1)
{
    std::string filePath = "/path/to/filename";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_DirectoryPath_Test_001, TestSize.Level1)
{
    std::string filePath = "/storage/cloud/files/Photo/";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_SpecialChars_Test_001, TestSize.Level1)
{
    std::string filePath = "file-name_123.txt";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "txt");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_UnicodeChars_Test_001, TestSize.Level1)
{
    std::string filePath = "文件名.jpg";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "jpg");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_WindowsPath_Test_001, TestSize.Level1)
{
    std::string filePath = "C:\\Users\\file.txt";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "txt");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_RelativePath_Test_001, TestSize.Level1)
{
    std::string filePath = "relative/path/to/file.jpg";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "jpg");
}
HWTEST_F(MediaPathUtilsUnitTest, medialib_desensitize_path_test_001, TestSize.Level1)
{
    // 云路径前缀被替换为单个 *，其余部分保持
    EXPECT_EQ(MediaPathUtils::DesensitizePath("/storage/cloud/files/Photo/1/a.jpg"), "*/Photo/1/a.jpg");
    // 短于云路径前缀长度的路径原样返回
    EXPECT_EQ(MediaPathUtils::DesensitizePath("/data/test/a.jpg"), "/data/test/a.jpg");
    EXPECT_EQ(MediaPathUtils::DesensitizePath(""), "");
}

HWTEST_F(MediaPathUtilsUnitTest, medialib_desensitize_name_test_001, TestSize.Level1)
{
    // 脱敏规则：1~3 个字符簇 -> 1 个 *，4 个 -> 2 个 *，5 个及以上 -> 3 个 *；扩展名不参与脱敏
    EXPECT_EQ(MediaPathUtils::DesensitizeName("abc.jpg"), "a*c.jpg");
    EXPECT_EQ(MediaPathUtils::DesensitizeName("abcd.jpg"), "a**d.jpg");
    EXPECT_EQ(MediaPathUtils::DesensitizeName("abcde.jpg"), "a***e.jpg");
    EXPECT_EQ(MediaPathUtils::DesensitizeName(""), "");
}

HWTEST_F(MediaPathUtilsUnitTest, medialib_get_parent_path_test_001, TestSize.Level1)
{
    EXPECT_EQ(MediaPathUtils::GetParentPath("/data/test/abc/a.jpg"), "/data/test/abc");
    EXPECT_EQ(MediaPathUtils::GetParentPath("/data"), "");
    EXPECT_EQ(MediaPathUtils::GetParentPath("noslash"), "");
}

HWTEST_F(MediaPathUtilsUnitTest, medialib_copy_directory_test_001, TestSize.Level1)
{
    std::string srcDir = "/data/test/media_path_utils/copy_src";
    std::string dstDir = "/data/test/media_path_utils/copy_dst";
    EXPECT_EQ(MediaPathUtils::CopyDirectory("", dstDir), E_MODIFY_DATA_FAIL);
    EXPECT_EQ(MediaPathUtils::CopyDirectory("", ""), E_MODIFY_DATA_FAIL);
    ASSERT_TRUE(MediaPureFileUtils::CreateDirectory(srcDir));

    std::string srcFile = srcDir + "/a.txt";
    FILE *fp = fopen(srcFile.c_str(), "w");
    ASSERT_NE(fp, nullptr);
    (void)fprintf(fp, "media library");
    (void)fclose(fp);

    EXPECT_EQ(MediaPathUtils::CopyDirectory(srcDir, dstDir), E_OK);
    EXPECT_TRUE(MediaPureFileUtils::IsFileExists(dstDir + "/a.txt"));
    // 目标目录已存在时直接返回 E_FILE_EXIST，不覆盖
    EXPECT_EQ(MediaPathUtils::CopyDirectory(srcDir, dstDir), E_FILE_EXIST);
    // 源路径不是目录时返回 E_FAIL
    EXPECT_EQ(MediaPathUtils::CopyDirectory(srcFile, dstDir + "_2"), E_FAIL);

    EXPECT_TRUE(MediaPureFileUtils::DeleteDir(dstDir));
    EXPECT_TRUE(MediaPureFileUtils::DeleteDir(srcDir));
}
} // namespace Media
} // namespace OHOS
/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

// ==================== GetFileName Tests ====================

/**
 * @tc.name: GetFileName_EmptyPath_Test_001
 * @tc.desc: [Empty path returns empty string]
 *           [1] Input empty string ""
 *           [2] Expected to return empty string
 */
HWTEST_F(MediaPathUtilsUnitTest, GetFileName_EmptyPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFileName_EmptyPath_Test_001 start");
    std::string filePath = "";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_NoSlash_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFileName_NoSlash_Test_001 start");
    std::string filePath = "filename.txt";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_EndsWithSlash_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFileName_EndsWithSlash_Test_001 start");
    std::string filePath = "path/to/dir/";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_SingleSlash_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFileName_SingleSlash_Test_001 start");
    std::string filePath = "/filename.txt";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "filename.txt");
}

HWTEST_F(MediaPathUtilsUnitTest, medialib_check_path_traversal_strict_test_001, TestSize.Level1)
{
    // '/' 分隔的段级 ".."：开头 ../、中间 /../、结尾 /..、裸 ".." 均判为穿越
    EXPECT_TRUE(MediaPathUtils::CheckPathTraversalStrict("/storage/../../etc/passwd"));
    EXPECT_TRUE(MediaPathUtils::CheckPathTraversalStrict("/storage/cloud/files/../secret.jpg"));
    EXPECT_TRUE(MediaPathUtils::CheckPathTraversalStrict("../relative/escape.jpg"));
    EXPECT_TRUE(MediaPathUtils::CheckPathTraversalStrict("a/../b"));
    EXPECT_TRUE(MediaPathUtils::CheckPathTraversalStrict("/.."));
    EXPECT_TRUE(MediaPathUtils::CheckPathTraversalStrict(".."));
    EXPECT_TRUE(MediaPathUtils::CheckPathTraversalStrict("../"));
    EXPECT_TRUE(MediaPathUtils::CheckPathTraversalStrict("a/.."));
}

HWTEST_F(MediaPathUtilsUnitTest, medialib_check_path_traversal_strict_test_002, TestSize.Level1)
{
    // 仅含 ".." 子串但不是段级形态：合法文件名（displayName 允许中间 '.'）不判为穿越
    EXPECT_FALSE(MediaPathUtils::CheckPathTraversalStrict("a..b.jpg"));
    EXPECT_FALSE(MediaPathUtils::CheckPathTraversalStrict("photo..jpg"));
    EXPECT_FALSE(MediaPathUtils::CheckPathTraversalStrict("..a/b.jpg"));
    EXPECT_FALSE(MediaPathUtils::CheckPathTraversalStrict("a/.../b"));
    EXPECT_FALSE(MediaPathUtils::CheckPathTraversalStrict("/storage/cloud/files/Photos/a..b.jpg"));
}

HWTEST_F(MediaPathUtilsUnitTest, medialib_check_path_traversal_strict_test_003, TestSize.Level1)
{
    // '\' 在 OHOS 是合法文件名字符：Windows 风格形态不按路径分段处理
    EXPECT_FALSE(MediaPathUtils::CheckPathTraversalStrict("123\\123\\.."));
    EXPECT_FALSE(MediaPathUtils::CheckPathTraversalStrict("dir\\..\\escape"));
    EXPECT_FALSE(MediaPathUtils::CheckPathTraversalStrict("..\\win\\escape"));
}

HWTEST_F(MediaPathUtilsUnitTest, medialib_check_path_traversal_strict_test_004, TestSize.Level1)
{
    // 正常路径与空串
    EXPECT_FALSE(MediaPathUtils::CheckPathTraversalStrict("/storage/cloud/files/Photos/01.jpg"));
    EXPECT_FALSE(MediaPathUtils::CheckPathTraversalStrict("photo.jpg"));
    EXPECT_FALSE(MediaPathUtils::CheckPathTraversalStrict("/storage/./current.jpg"));
    EXPECT_FALSE(MediaPathUtils::CheckPathTraversalStrict(""));
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_MultipleSlashes_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFileName_MultipleSlashes_Test_001 start");
    std::string filePath = "/path/to/filename.txt";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "filename.txt");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_DirectoryPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFileName_DirectoryPath_Test_001 start");
    std::string filePath = "/storage/cloud/files/Photo/";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_ImageFile_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFileName_ImageFile_Test_001 start");
    std::string filePath = "/storage/cloud/files/Photo/image.jpg";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "image.jpg");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_VideoFile_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFileName_VideoFile_Test_001 start");
    std::string filePath = "/storage/cloud/files/Video/video.mp4";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "video.mp4");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_AudioFile_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFileName_AudioFile_Test_001 start");
    std::string filePath = "/storage/cloud/files/Audio/audio.mp3";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "audio.mp3");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_HiddenFile_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFileName_HiddenFile_Test_001 start");
    std::string filePath = "/storage/cloud/files/.hidden";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, ".hidden");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_FileWithDots_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFileName_FileWithDots_Test_001 start");
    std::string filePath = "/storage/cloud/files/file.name.with.dots.txt";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "file.name.with.dots.txt");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_UnixPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFileName_UnixPath_Test_001 start");
    std::string filePath = "/usr/local/bin/file";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "file");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_RelativePath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFileName_RelativePath_Test_001 start");
    std::string filePath = "relative/path/to/file.txt";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "file.txt");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_DocumentFile_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFileName_DocumentFile_Test_001 start");
    std::string filePath = "/storage/cloud/files/Docs/document.pdf";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "document.pdf");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_LongFileName_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFileName_LongFileName_Test_001 start");
    std::string filePath = "/storage/cloud/files/very_long_filename_with_many_characters.jpg";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "very_long_filename_with_many_characters.jpg");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_SpecialChars_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFileName_SpecialChars_Test_001 start");
    std::string filePath = "/storage/cloud/files/file-name_123.txt";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "file-name_123.txt");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_UnicodeChars_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFileName_UnicodeChars_Test_001 start");
    std::string filePath = "/storage/cloud/files/文件名.jpg";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "文件名.jpg");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_DoubleSlash_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFileName_DoubleSlash_Test_001 start");
    std::string filePath = "/path//to//file.txt";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "file.txt");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_TrailingSlash_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFileName_TrailingSlash_Test_001 start");
    std::string filePath = "/path/to/file.txt/";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_RootPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFileName_RootPath_Test_001 start");
    std::string filePath = "/";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_CurrentDir_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFileName_CurrentDir_Test_001 start");
    std::string filePath = ".";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetFileName_ParentDir_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetFileName_ParentDir_Test_001 start");
    std::string filePath = "..";
    std::string result = MediaPathUtils::GetFileName(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_EmptyPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetExtension_EmptyPath_Test_001 start");
    std::string filePath = "";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_NoExtension_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetExtension_NoExtension_Test_001 start");
    std::string filePath = "filename";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_DotAtStart_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetExtension_DotAtStart_Test_001 start");
    std::string filePath = ".hiddenfile";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_SingleDot_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetExtension_SingleDot_Test_001 start");
    std::string filePath = ".";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_DoubleDot_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetExtension_DoubleDot_Test_001 start");
    std::string filePath = "..";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_UpperCase_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetExtension_UpperCase_Test_001 start");
    std::string filePath = "image.JPG";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "jpg");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_MixedCase_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetExtension_MixedCase_Test_001 start");
    std::string filePath = "image.JpG";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "jpg");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_MultipleDots_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetExtension_MultipleDots_Test_001 start");
    std::string filePath = "file.name.with.dots.txt";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "txt");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_PathWithExtension_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetExtension_PathWithExtension_Test_001 start");
    std::string filePath = "/path/to/image.jpg";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "jpg");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_PathWithExtension_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetExtension_PathWithExtension_Test_002 start");
    std::string filePath = "/storage/cloud/files/Photo/image.jpg";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "jpg");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_PathWithExtension_Test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetExtension_PathWithExtension_Test_003 start");
    std::string filePath = "/storage/cloud/files/Video/video.mp4";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "mp4");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_PathWithExtension_Test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetExtension_PathWithExtension_Test_004 start");
    std::string filePath = "/storage/cloud/files/Audio/audio.mp3";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "mp3");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_PathWithoutExtension_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetExtension_PathWithoutExtension_Test_001 start");
    std::string filePath = "/path/to/filename";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_DirectoryPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetExtension_DirectoryPath_Test_001 start");
    std::string filePath = "/storage/cloud/files/Photo/";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_SpecialChars_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetExtension_SpecialChars_Test_001 start");
    std::string filePath = "file-name_123.txt";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "txt");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_UnicodeChars_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetExtension_UnicodeChars_Test_001 start");
    std::string filePath = "文件名.jpg";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "jpg");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_WindowsPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetExtension_WindowsPath_Test_001 start");
    std::string filePath = "C:\\Users\\file.txt";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "txt");
}

HWTEST_F(MediaPathUtilsUnitTest, GetExtension_RelativePath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetExtension_RelativePath_Test_001 start");
    std::string filePath = "relative/path/to/file.jpg";
    std::string result = MediaPathUtils::GetExtension(filePath);
    EXPECT_EQ(result, "jpg");
}

// ==================== GetParentPath Tests ====================

/**
 * @tc.name: GetParentPath_NormalPath_Test_001
 * @tc.desc: [Extract parent directory from standard path]
 *           [1] Input "/storage/cloud/files/Photo/image.jpg"
 *           [2] Expected to return "/storage/cloud/files/Photo"
 */
HWTEST_F(MediaPathUtilsUnitTest, GetParentPath_NormalPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetParentPath_NormalPath_Test_001 start");
    std::string path = "/storage/cloud/files/Photo/image.jpg";
    std::string result = MediaPathUtils::GetParentPath(path);
    EXPECT_EQ(result, "/storage/cloud/files/Photo");
}

/**
 * @tc.name: GetParentPath_RootPath_Test_001
 * @tc.desc: [Root path "/" returns empty string]
 *           [1] Input "/"
 *           [2] rfind('/') returns 0, substr(0,0) is empty, expected to return empty string
 */
HWTEST_F(MediaPathUtilsUnitTest, GetParentPath_RootPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetParentPath_RootPath_Test_001 start");
    std::string path = "/";
    std::string result = MediaPathUtils::GetParentPath(path);
    EXPECT_EQ(result, "");
}

/**
 * @tc.name: GetParentPath_EmptyPath_Test_001
 * @tc.desc: [Empty path returns empty string]
 *           [1] Input ""
 *           [2] Expected to return empty string
 */
HWTEST_F(MediaPathUtilsUnitTest, GetParentPath_EmptyPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetParentPath_EmptyPath_Test_001 start");
    std::string path = "";
    std::string result = MediaPathUtils::GetParentPath(path);
    EXPECT_EQ(result, "");
}

/**
 * @tc.name: GetParentPath_NoSlash_Test_001
 * @tc.desc: [Path without slash returns empty string]
 *           [1] Input "filename.txt"
 *           [2] rfind('/') returns npos, expected to return empty string
 */
HWTEST_F(MediaPathUtilsUnitTest, GetParentPath_NoSlash_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetParentPath_NoSlash_Test_001 start");
    std::string path = "filename.txt";
    std::string result = MediaPathUtils::GetParentPath(path);
    EXPECT_EQ(result, "");
}

/**
 * @tc.name: GetParentPath_TrailingSlash_Test_001
 * @tc.desc: [Path ending with slash correctly extracts parent directory]
 *           [1] Input "/storage/cloud/files/Photo/"
 *           [2] rfind('/') finds trailing slash, expected to return "/storage/cloud/files/Photo"
 */
HWTEST_F(MediaPathUtilsUnitTest, GetParentPath_TrailingSlash_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetParentPath_TrailingSlash_Test_001 start");
    std::string path = "/storage/cloud/files/Photo/";
    std::string result = MediaPathUtils::GetParentPath(path);
    EXPECT_EQ(result, "/storage/cloud/files/Photo");
}

/**
 * @tc.name: GetParentPath_SingleSegment_Test_001
 * @tc.desc: [Single segment path "/file" returns empty string]
 *           [1] Input "/file"
 *           [2] rfind('/') returns 0, substr(0,0) is empty, expected to return empty string
 */
HWTEST_F(MediaPathUtilsUnitTest, GetParentPath_SingleSegment_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetParentPath_SingleSegment_Test_001 start");
    std::string path = "/file";
    std::string result = MediaPathUtils::GetParentPath(path);
    EXPECT_EQ(result, "");
}

// ==================== GetDir Tests ====================

/**
 * @tc.name: GetDir_CloudRoot_Test_001
 * @tc.desc: [CLOUD_ROOT returns cloud root directory]
 *           [1] Call GetDir(MediaPathType::CLOUD_ROOT)
 *           [2] Expected to return "/storage/cloud/"
 */
HWTEST_F(MediaPathUtilsUnitTest, GetDir_CloudRoot_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetDir_CloudRoot_Test_001 start");
    std::string result = MediaPathUtils::GetDir(MediaPathType::CLOUD_ROOT);
    EXPECT_EQ(result, "/storage/cloud/");
}

/**
 * @tc.name: GetDir_CloudFiles_Test_001
 * @tc.desc: [CLOUD_FILES returns cloud files directory]
 *           [1] Call GetDir(MediaPathType::CLOUD_FILES)
 *           [2] Expected to return "/storage/cloud/files/"
 */
HWTEST_F(MediaPathUtilsUnitTest, GetDir_CloudFiles_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetDir_CloudFiles_Test_001 start");
    std::string result = MediaPathUtils::GetDir(MediaPathType::CLOUD_FILES);
    EXPECT_EQ(result, "/storage/cloud/files/");
}

/**
 * @tc.name: GetDir_CloudPhoto_Test_001
 * @tc.desc: [CLOUD_PHOTO returns cloud photo directory]
 *           [1] Call GetDir(MediaPathType::CLOUD_PHOTO)
 *           [2] Expected to return "/storage/cloud/files/Photo/"
 */
HWTEST_F(MediaPathUtilsUnitTest, GetDir_CloudPhoto_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetDir_CloudPhoto_Test_001 start");
    std::string result = MediaPathUtils::GetDir(MediaPathType::CLOUD_PHOTO);
    EXPECT_EQ(result, "/storage/cloud/files/Photo/");
}

/**
 * @tc.name: GetDir_CloudThumb_Test_001
 * @tc.desc: [CLOUD_THUMB returns cloud thumbnail directory]
 *           [1] Call GetDir(MediaPathType::CLOUD_THUMB)
 *           [2] Expected to return "/storage/cloud/files/.thumbs/"
 */
HWTEST_F(MediaPathUtilsUnitTest, GetDir_CloudThumb_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetDir_CloudThumb_Test_001 start");
    std::string result = MediaPathUtils::GetDir(MediaPathType::CLOUD_THUMB);
    EXPECT_EQ(result, "/storage/cloud/files/.thumbs/");
}

/**
 * @tc.name: GetDir_LocalRoot_Test_001
 * @tc.desc: [LOCAL_ROOT returns local root directory]
 *           [1] Call GetDir(MediaPathType::LOCAL_ROOT)
 *           [2] Expected to return "/storage/media/local/"
 */
HWTEST_F(MediaPathUtilsUnitTest, GetDir_LocalRoot_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetDir_LocalRoot_Test_001 start");
    std::string result = MediaPathUtils::GetDir(MediaPathType::LOCAL_ROOT);
    EXPECT_EQ(result, "/storage/media/local/");
}

/**
 * @tc.name: GetDir_LocalFiles_Test_001
 * @tc.desc: [LOCAL_FILES returns local files directory]
 *           [1] Call GetDir(MediaPathType::LOCAL_FILES)
 *           [2] Expected to return "/storage/media/local/files/"
 */
HWTEST_F(MediaPathUtilsUnitTest, GetDir_LocalFiles_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetDir_LocalFiles_Test_001 start");
    std::string result = MediaPathUtils::GetDir(MediaPathType::LOCAL_FILES);
    EXPECT_EQ(result, "/storage/media/local/files/");
}

/**
 * @tc.name: GetDir_LocalPhoto_Test_001
 * @tc.desc: [LOCAL_PHOTO returns local photo directory]
 *           [1] Call GetDir(MediaPathType::LOCAL_PHOTO)
 *           [2] Expected to return "/storage/media/local/files/Photo/"
 */
HWTEST_F(MediaPathUtilsUnitTest, GetDir_LocalPhoto_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetDir_LocalPhoto_Test_001 start");
    std::string result = MediaPathUtils::GetDir(MediaPathType::LOCAL_PHOTO);
    EXPECT_EQ(result, "/storage/media/local/files/Photo/");
}

/**
 * @tc.name: GetDir_CloudEditDataPhoto_Test_001
 * @tc.desc: [CLOUD_EDIT_DATA_PHOTO returns nested edit data directory]
 *           [1] Call GetDir(MediaPathType::CLOUD_EDIT_DATA_PHOTO)
 *           [2] Expected to return "/storage/cloud/files/.editData/Photo/"
 */
HWTEST_F(MediaPathUtilsUnitTest, GetDir_CloudEditDataPhoto_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetDir_CloudEditDataPhoto_Test_001 start");
    std::string result = MediaPathUtils::GetDir(MediaPathType::CLOUD_EDIT_DATA_PHOTO);
    EXPECT_EQ(result, "/storage/cloud/files/.editData/Photo/");
}

/**
 * @tc.name: GetDir_AppendSlashNo_Test_001
 * @tc.desc: [AppendSlash::NO removes trailing slash]
 *           [1] Call GetDir(CLOUD_FILES, AdaptAccount::NO, AppendSlash::NO)
 *           [2] Expected to return "/storage/cloud/files" (no trailing slash)
 */
HWTEST_F(MediaPathUtilsUnitTest, GetDir_AppendSlashNo_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetDir_AppendSlashNo_Test_001 start");
    std::string result = MediaPathUtils::GetDir(MediaPathType::CLOUD_FILES,
        AdaptAccount::NO, AppendSlash::NO);
    EXPECT_EQ(result, "/storage/cloud/files");
}

/**
 * @tc.name: GetDir_AppendSlashYes_Test_001
 * @tc.desc: [AppendSlash::YES keeps trailing slash]
 *           [1] Call GetDir(CLOUD_ROOT, AdaptAccount::NO, AppendSlash::YES)
 *           [2] Expected to return "/storage/cloud/" (with trailing slash)
 */
HWTEST_F(MediaPathUtilsUnitTest, GetDir_AppendSlashYes_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetDir_AppendSlashYes_Test_001 start");
    std::string result = MediaPathUtils::GetDir(MediaPathType::CLOUD_ROOT,
        AdaptAccount::NO, AppendSlash::YES);
    EXPECT_EQ(result, "/storage/cloud/");
}

/**
 * @tc.name: GetDir_InvalidPathType_Test_001
 * @tc.desc: [Invalid path type returns empty string]
 *           [1] Call GetDir(static_cast<MediaPathType>(999999))
 *           [2] No matching known type, expected to return empty string
 */
HWTEST_F(MediaPathUtilsUnitTest, GetDir_InvalidPathType_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetDir_InvalidPathType_Test_001 start");
    std::string result = MediaPathUtils::GetDir(static_cast<MediaPathType>(999999));
    EXPECT_EQ(result, "");
}

// ==================== GetBucketFix Tests ====================

/**
 * @tc.name: GetBucketFix_Photo_Test_001
 * @tc.desc: [PHOTO bucket returns "Photo/"]
 *           [1] Call GetBucketFix(FileBucketType::PHOTO)
 *           [2] Expected to return "Photo/"
 */
HWTEST_F(MediaPathUtilsUnitTest, GetBucketFix_Photo_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetBucketFix_Photo_Test_001 start");
    std::string result = MediaPathUtils::GetBucketFix(FileBucketType::PHOTO);
    EXPECT_EQ(result, "Photo/");
}

/**
 * @tc.name: GetBucketFix_Thumb_Test_001
 * @tc.desc: [THUMB bucket returns ".thumbs/"]
 *           [1] Call GetBucketFix(FileBucketType::THUMB)
 *           [2] Expected to return ".thumbs/"
 */
HWTEST_F(MediaPathUtilsUnitTest, GetBucketFix_Thumb_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetBucketFix_Thumb_Test_001 start");
    std::string result = MediaPathUtils::GetBucketFix(FileBucketType::THUMB);
    EXPECT_EQ(result, ".thumbs/");
}

/**
 * @tc.name: GetBucketFix_Camera_Test_001
 * @tc.desc: [CAMERA bucket returns "Camera/"]
 *           [1] Call GetBucketFix(FileBucketType::CAMERA)
 *           [2] Expected to return "Camera/"
 */
HWTEST_F(MediaPathUtilsUnitTest, GetBucketFix_Camera_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetBucketFix_Camera_Test_001 start");
    std::string result = MediaPathUtils::GetBucketFix(FileBucketType::CAMERA);
    EXPECT_EQ(result, "Camera/");
}

/**
 * @tc.name: GetBucketFix_BackUp_Test_001
 * @tc.desc: [BACK_UP bucket returns ".backup/"]
 *           [1] Call GetBucketFix(FileBucketType::BACK_UP)
 *           [2] Expected to return ".backup/"
 */
HWTEST_F(MediaPathUtilsUnitTest, GetBucketFix_BackUp_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetBucketFix_BackUp_Test_001 start");
    std::string result = MediaPathUtils::GetBucketFix(FileBucketType::BACK_UP);
    EXPECT_EQ(result, ".backup/");
}

/**
 * @tc.name: GetBucketFix_InvalidType_Test_001
 * @tc.desc: [Invalid bucket type returns empty string]
 *           [1] Call GetBucketFix(static_cast<FileBucketType>(999))
 *           [2] No matching known type, expected to return empty string
 */
HWTEST_F(MediaPathUtilsUnitTest, GetBucketFix_InvalidType_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetBucketFix_InvalidType_Test_001 start");
    std::string result = MediaPathUtils::GetBucketFix(static_cast<FileBucketType>(999));
    EXPECT_EQ(result, "");
}

// ==================== GetThumbnailFileFix Tests ====================

/**
 * @tc.name: GetThumbnailFileFix_Lcd_Test_001
 * @tc.desc: [LCD thumbnail type returns "/LCD.jpg"]
 *           [1] Call GetThumbnailFileFix(ThumbnailFileType::LCD)
 *           [2] Expected to return "/LCD.jpg"
 */
HWTEST_F(MediaPathUtilsUnitTest, GetThumbnailFileFix_Lcd_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetThumbnailFileFix_Lcd_Test_001 start");
    std::string result = MediaPathUtils::GetThumbnailFileFix(ThumbnailFileType::LCD);
    EXPECT_EQ(result, "/LCD.jpg");
}

/**
 * @tc.name: GetThumbnailFileFix_LcdEx_Test_001
 * @tc.desc: [LCD_EX thumbnail type returns "/THM_EX/LCD.jpg"]
 *           [1] Call GetThumbnailFileFix(ThumbnailFileType::LCD_EX)
 *           [2] Expected to return "/THM_EX/LCD.jpg"
 */
HWTEST_F(MediaPathUtilsUnitTest, GetThumbnailFileFix_LcdEx_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetThumbnailFileFix_LcdEx_Test_001 start");
    std::string result = MediaPathUtils::GetThumbnailFileFix(ThumbnailFileType::LCD_EX);
    EXPECT_EQ(result, "/THM_EX/LCD.jpg");
}

/**
 * @tc.name: GetThumbnailFileFix_InvalidType_Test_001
 * @tc.desc: [Invalid thumbnail type returns empty string]
 *           [1] Call GetThumbnailFileFix(static_cast<ThumbnailFileType>(999))
 *           [2] No matching known type, expected to return empty string
 */
HWTEST_F(MediaPathUtilsUnitTest, GetThumbnailFileFix_InvalidType_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetThumbnailFileFix_InvalidType_Test_001 start");
    std::string result = MediaPathUtils::GetThumbnailFileFix(static_cast<ThumbnailFileType>(999));
    EXPECT_EQ(result, "");
}

// ==================== CheckFilesPath Tests ====================

/**
 * @tc.name: CheckFilesPath_ValidPath_Test_001
 * @tc.desc: [cloud files path matches CLOUD_FILES type]
 *           [1] Input "/storage/cloud/files/image.jpg", type CLOUD_FILES
 *           [2] Prefix matches, expected to return true
 */
HWTEST_F(MediaPathUtilsUnitTest, CheckFilesPath_ValidPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckFilesPath_ValidPath_Test_001 start");
    std::string photoPath = "/storage/cloud/files/image.jpg";
    bool result = MediaPathUtils::CheckFilesPath(MediaPathType::CLOUD_FILES, photoPath);
    EXPECT_EQ(result, true);
}

/**
 * @tc.name: CheckFilesPath_ValidPath_Test_002
 * @tc.desc: [cloud photo path matches CLOUD_FILES type (prefix match)]
 *           [1] Input "/storage/cloud/files/Photo/image.jpg", type CLOUD_FILES
 *           [2] CLOUD_PHOTO path is a sub-path of CLOUD_FILES, expected to return true
 */
HWTEST_F(MediaPathUtilsUnitTest, CheckFilesPath_ValidPath_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckFilesPath_ValidPath_Test_002 start");
    std::string photoPath = "/storage/cloud/files/Photo/image.jpg";
    bool result = MediaPathUtils::CheckFilesPath(MediaPathType::CLOUD_FILES, photoPath);
    EXPECT_EQ(result, true);
}

/**
 * @tc.name: CheckFilesPath_ValidPath_Test_003
 * @tc.desc: [cloud video path matches CLOUD_FILES type]
 *           [1] Input "/storage/cloud/files/Video/video.mp4", type CLOUD_FILES
 *           [2] Prefix matches, expected to return true
 */
HWTEST_F(MediaPathUtilsUnitTest, CheckFilesPath_ValidPath_Test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckFilesPath_ValidPath_Test_003 start");
    std::string photoPath = "/storage/cloud/files/Video/video.mp4";
    bool result = MediaPathUtils::CheckFilesPath(MediaPathType::CLOUD_FILES, photoPath);
    EXPECT_EQ(result, true);
}

/**
 * @tc.name: CheckFilesPath_InvalidPath_Test_001
 * @tc.desc: [Non-cloud path does not match CLOUD_FILES]
 *           [1] Input "/local/files/image.jpg", type CLOUD_FILES
 *           [2] Prefix does not match, expected to return false
 */
HWTEST_F(MediaPathUtilsUnitTest, CheckFilesPath_InvalidPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckFilesPath_InvalidPath_Test_001 start");
    std::string photoPath = "/local/files/image.jpg";
    bool result = MediaPathUtils::CheckFilesPath(MediaPathType::CLOUD_FILES, photoPath);
    EXPECT_EQ(result, false);
}

/**
 * @tc.name: CheckFilesPath_InvalidPath_Test_002
 * @tc.desc: [Partial prefix does not match CLOUD_FILES]
 *           [1] Input "/storage/files/image.jpg", type CLOUD_FILES
 *           [2] Missing "cloud/" segment, expected to return false
 */
HWTEST_F(MediaPathUtilsUnitTest, CheckFilesPath_InvalidPath_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckFilesPath_InvalidPath_Test_002 start");
    std::string photoPath = "/storage/files/image.jpg";
    bool result = MediaPathUtils::CheckFilesPath(MediaPathType::CLOUD_FILES, photoPath);
    EXPECT_EQ(result, false);
}

/**
 * @tc.name: CheckFilesPath_EmptyPath_Test_001
 * @tc.desc: [Empty path does not match CLOUD_FILES]
 *           [1] Input "", type CLOUD_FILES
 *           [2] Empty string length insufficient, expected to return false
 */
HWTEST_F(MediaPathUtilsUnitTest, CheckFilesPath_EmptyPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckFilesPath_EmptyPath_Test_001 start");
    std::string photoPath = "";
    bool result = MediaPathUtils::CheckFilesPath(MediaPathType::CLOUD_FILES, photoPath);
    EXPECT_EQ(result, false);
}

/**
 * @tc.name: CheckFilesPath_ShortPath_Test_001
 * @tc.desc: [Path shorter than files directory does not match]
 *           [1] Input "/storage", type CLOUD_FILES
 *           [2] Length insufficient, expected to return false
 */
HWTEST_F(MediaPathUtilsUnitTest, CheckFilesPath_ShortPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckFilesPath_ShortPath_Test_001 start");
    std::string photoPath = "/storage";
    bool result = MediaPathUtils::CheckFilesPath(MediaPathType::CLOUD_FILES, photoPath);
    EXPECT_EQ(result, false);
}

/**
 * @tc.name: CheckFilesPath_ShortPath_Test_002
 * @tc.desc: [Path shorter than files directory does not match]
 *           [1] Input "/storage/cloud", type CLOUD_FILES
 *           [2] Length insufficient, expected to return false
 */
HWTEST_F(MediaPathUtilsUnitTest, CheckFilesPath_ShortPath_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckFilesPath_ShortPath_Test_002 start");
    std::string photoPath = "/storage/cloud";
    bool result = MediaPathUtils::CheckFilesPath(MediaPathType::CLOUD_FILES, photoPath);
    EXPECT_EQ(result, false);
}

/**
 * @tc.name: CheckFilesPath_ShortPath_Test_003
 * @tc.desc: [Path shorter than files directory does not match]
 *           [1] Input "/storage/cloud/", type CLOUD_FILES
 *           [2] Length insufficient, expected to return false
 */
HWTEST_F(MediaPathUtilsUnitTest, CheckFilesPath_ShortPath_Test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckFilesPath_ShortPath_Test_003 start");
    std::string photoPath = "/storage/cloud/";
    bool result = MediaPathUtils::CheckFilesPath(MediaPathType::CLOUD_FILES, photoPath);
    EXPECT_EQ(result, false);
}

// ==================== AppendUserId Tests ====================

/**
 * @tc.name: AppendUserId_ValidPath_Test_001
 * @tc.desc: [Insert userId into cloud path]
 *           [1] Input "/storage/cloud/files/image.jpg", userId=100
 *           [2] Expected to return "/storage/cloud/100/files/image.jpg"
 */
HWTEST_F(MediaPathUtilsUnitTest, AppendUserId_ValidPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AppendUserId_ValidPath_Test_001 start");
    std::string path = "/storage/cloud/files/image.jpg";
    int32_t userId = 100;
    std::string result = MediaPathUtils::AppendUserId(path, userId);
    EXPECT_EQ(result, "/storage/cloud/100/files/image.jpg");
}

/**
 * @tc.name: AppendUserId_ValidPath_Test_002
 * @tc.desc: [Insert userId into cloud photo path]
 *           [1] Input "/storage/cloud/files/Photo/image.jpg", userId=100
 *           [2] Expected to return "/storage/cloud/100/files/Photo/image.jpg"
 */
HWTEST_F(MediaPathUtilsUnitTest, AppendUserId_ValidPath_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("AppendUserId_ValidPath_Test_002 start");
    std::string path = "/storage/cloud/files/Photo/image.jpg";
    int32_t userId = 100;
    std::string result = MediaPathUtils::AppendUserId(path, userId);
    EXPECT_EQ(result, "/storage/cloud/100/files/Photo/image.jpg");
}

/**
 * @tc.name: AppendUserId_ValidPath_Test_003
 * @tc.desc: [Insert userId into cloud video path]
 *           [1] Input "/storage/cloud/files/Video/video.mp4", userId=200
 *           [2] Expected to return "/storage/cloud/200/files/Video/video.mp4"
 */
HWTEST_F(MediaPathUtilsUnitTest, AppendUserId_ValidPath_Test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("AppendUserId_ValidPath_Test_003 start");
    std::string path = "/storage/cloud/files/Video/video.mp4";
    int32_t userId = 200;
    std::string result = MediaPathUtils::AppendUserId(path, userId);
    EXPECT_EQ(result, "/storage/cloud/200/files/Video/video.mp4");
}

/**
 * @tc.name: AppendUserId_ValidPath_Test_004
 * @tc.desc: [Insert userId into cloud audio path]
 *           [1] Input "/storage/cloud/files/Audio/audio.mp3", userId=300
 *           [2] Expected to return "/storage/cloud/300/files/Audio/audio.mp3"
 */
HWTEST_F(MediaPathUtilsUnitTest, AppendUserId_ValidPath_Test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("AppendUserId_ValidPath_Test_004 start");
    std::string path = "/storage/cloud/files/Audio/audio.mp3";
    int32_t userId = 300;
    std::string result = MediaPathUtils::AppendUserId(path, userId);
    EXPECT_EQ(result, "/storage/cloud/300/files/Audio/audio.mp3");
}

/**
 * @tc.name: AppendUserId_InvalidUserId_Test_001
 * @tc.desc: [Negative userId returns original path]
 *           [1] Input "/storage/cloud/files/image.jpg", userId=-1
 *           [2] userId < 0 not inserted, expected to return original path
 */
HWTEST_F(MediaPathUtilsUnitTest, AppendUserId_InvalidUserId_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AppendUserId_InvalidUserId_Test_001 start");
    std::string path = "/storage/cloud/files/image.jpg";
    int32_t userId = -1;
    std::string result = MediaPathUtils::AppendUserId(path, userId);
    EXPECT_EQ(result, path);
}

/**
 * @tc.name: AppendUserId_InvalidUserId_Test_002
 * @tc.desc: [Large negative userId returns original path]
 *           [1] Input "/storage/cloud/files/image.jpg", userId=-100
 *           [2] userId < 0 not inserted, expected to return original path
 */
HWTEST_F(MediaPathUtilsUnitTest, AppendUserId_InvalidUserId_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("AppendUserId_InvalidUserId_Test_002 start");
    std::string path = "/storage/cloud/files/image.jpg";
    int32_t userId = -100;
    std::string result = MediaPathUtils::AppendUserId(path, userId);
    EXPECT_EQ(result, path);
}

/**
 * @tc.name: AppendUserId_InvalidPath_Test_001
 * @tc.desc: [Non-cloud path returns original path]
 *           [1] Input "/local/files/image.jpg", userId=100
 *           [2] Does not match cloud prefix, expected to return original path
 */
HWTEST_F(MediaPathUtilsUnitTest, AppendUserId_InvalidPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AppendUserId_InvalidPath_Test_001 start");
    std::string path = "/local/files/image.jpg";
    int32_t userId = 100;
    std::string result = MediaPathUtils::AppendUserId(path, userId);
    EXPECT_EQ(result, path);
}

/**
 * @tc.name: AppendUserId_InvalidPath_Test_002
 * @tc.desc: [Partial prefix path returns original path]
 *           [1] Input "/storage/files/image.jpg", userId=100
 *           [2] Does not match cloud prefix, expected to return original path
 */
HWTEST_F(MediaPathUtilsUnitTest, AppendUserId_InvalidPath_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("AppendUserId_InvalidPath_Test_002 start");
    std::string path = "/storage/files/image.jpg";
    int32_t userId = 100;
    std::string result = MediaPathUtils::AppendUserId(path, userId);
    EXPECT_EQ(result, path);
}

/**
 * @tc.name: AppendUserId_EmptyPath_Test_001
 * @tc.desc: [Empty path returns original path]
 *           [1] Input "", userId=100
 *           [2] Empty string does not match prefix, expected to return original path
 */
HWTEST_F(MediaPathUtilsUnitTest, AppendUserId_EmptyPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AppendUserId_EmptyPath_Test_001 start");
    std::string path = "";
    int32_t userId = 100;
    std::string result = MediaPathUtils::AppendUserId(path, userId);
    EXPECT_EQ(result, path);
}

// ==================== ConvertCloudPathToLocalPath Tests ====================

/**
 * @tc.name: ConvertCloudPathToLocalPath_ValidPath_Test_001
 * @tc.desc: [Convert cloud files path to local files path]
 *           [1] Input "/storage/cloud/files/Photo/image.jpg"
 *           [2] Expected to return "/storage/media/local/files/Photo/image.jpg"
 */
HWTEST_F(MediaPathUtilsUnitTest, ConvertCloudPathToLocalPath_ValidPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ConvertCloudPathToLocalPath_ValidPath_Test_001 start");
    std::string cloudPath = "/storage/cloud/files/Photo/image.jpg";
    std::string result = MediaPathUtils::ConvertCloudPathToLocalPath(cloudPath);
    EXPECT_EQ(result, "/storage/media/local/files/Photo/image.jpg");
}

/**
 * @tc.name: ConvertCloudPathToLocalPath_RootPath_Test_001
 * @tc.desc: [Convert cloud root path to local root path]
 *           [1] Input "/storage/cloud/"
 *           [2] Expected to return "/storage/media/local/"
 */
HWTEST_F(MediaPathUtilsUnitTest, ConvertCloudPathToLocalPath_RootPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ConvertCloudPathToLocalPath_RootPath_Test_001 start");
    std::string cloudPath = "/storage/cloud/";
    std::string result = MediaPathUtils::ConvertCloudPathToLocalPath(cloudPath);
    EXPECT_EQ(result, "/storage/media/local/");
}

/**
 * @tc.name: ConvertCloudPathToLocalPath_EmptyPath_Test_001
 * @tc.desc: [Empty path returns empty string]
 *           [1] Input ""
 *           [2] Does not match cloud prefix, expected to return empty string
 */
HWTEST_F(MediaPathUtilsUnitTest, ConvertCloudPathToLocalPath_EmptyPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ConvertCloudPathToLocalPath_EmptyPath_Test_001 start");
    std::string cloudPath = "";
    std::string result = MediaPathUtils::ConvertCloudPathToLocalPath(cloudPath);
    EXPECT_EQ(result, "");
}

/**
 * @tc.name: ConvertCloudPathToLocalPath_NonCloudPath_Test_001
 * @tc.desc: [Non-cloud path returned as-is]
 *           [1] Input "/storage/media/local/files/Photo/image.jpg"
 *           [2] Does not match cloud prefix, expected to return original path
 */
HWTEST_F(MediaPathUtilsUnitTest, ConvertCloudPathToLocalPath_NonCloudPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ConvertCloudPathToLocalPath_NonCloudPath_Test_001 start");
    std::string cloudPath = "/storage/media/local/files/Photo/image.jpg";
    std::string result = MediaPathUtils::ConvertCloudPathToLocalPath(cloudPath);
    EXPECT_EQ(result, cloudPath);
}

/**
 * @tc.name: ConvertCloudPathToLocalPath_PartialPrefix_Test_001
 * @tc.desc: [Partial prefix path returned as-is]
 *           [1] Input "/storage/cloud_backup/files/image.jpg"
 *           [2] Starts with "/storage/cloud" but not "/storage/cloud/", expected to return original path
 */
HWTEST_F(MediaPathUtilsUnitTest, ConvertCloudPathToLocalPath_PartialPrefix_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ConvertCloudPathToLocalPath_PartialPrefix_Test_001 start");
    std::string cloudPath = "/storage/cloud_backup/files/image.jpg";
    std::string result = MediaPathUtils::ConvertCloudPathToLocalPath(cloudPath);
    EXPECT_EQ(result, cloudPath);
}

// ==================== ConvertLocalPathToCloudPath Tests ====================

/**
 * @tc.name: ConvertLocalPathToCloudPath_ValidPath_Test_001
 * @tc.desc: [Convert local files path to cloud files path]
 *           [1] Input "/storage/media/local/files/Photo/image.jpg"
 *           [2] Expected to return "/storage/cloud/files/Photo/image.jpg"
 */
HWTEST_F(MediaPathUtilsUnitTest, ConvertLocalPathToCloudPath_ValidPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ConvertLocalPathToCloudPath_ValidPath_Test_001 start");
    std::string localPath = "/storage/media/local/files/Photo/image.jpg";
    std::string result = MediaPathUtils::ConvertLocalPathToCloudPath(localPath);
    EXPECT_EQ(result, "/storage/cloud/files/Photo/image.jpg");
}

/**
 * @tc.name: ConvertLocalPathToCloudPath_RootPath_Test_001
 * @tc.desc: [Convert local root path to cloud root path]
 *           [1] Input "/storage/media/local/"
 *           [2] Expected to return "/storage/cloud/"
 */
HWTEST_F(MediaPathUtilsUnitTest, ConvertLocalPathToCloudPath_RootPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ConvertLocalPathToCloudPath_RootPath_Test_001 start");
    std::string localPath = "/storage/media/local/";
    std::string result = MediaPathUtils::ConvertLocalPathToCloudPath(localPath);
    EXPECT_EQ(result, "/storage/cloud/");
}

/**
 * @tc.name: ConvertLocalPathToCloudPath_EmptyPath_Test_001
 * @tc.desc: [Empty path returns empty string]
 *           [1] Input ""
 *           [2] Does not match local prefix, expected to return empty string
 */
HWTEST_F(MediaPathUtilsUnitTest, ConvertLocalPathToCloudPath_EmptyPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ConvertLocalPathToCloudPath_EmptyPath_Test_001 start");
    std::string localPath = "";
    std::string result = MediaPathUtils::ConvertLocalPathToCloudPath(localPath);
    EXPECT_EQ(result, "");
}

/**
 * @tc.name: ConvertLocalPathToCloudPath_NonLocalPath_Test_001
 * @tc.desc: [Non-local path returned as-is]
 *           [1] Input "/storage/cloud/files/Photo/image.jpg"
 *           [2] Does not match local prefix, expected to return original path
 */
HWTEST_F(MediaPathUtilsUnitTest, ConvertLocalPathToCloudPath_NonLocalPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ConvertLocalPathToCloudPath_NonLocalPath_Test_001 start");
    std::string localPath = "/storage/cloud/files/Photo/image.jpg";
    std::string result = MediaPathUtils::ConvertLocalPathToCloudPath(localPath);
    EXPECT_EQ(result, localPath);
}

/**
 * @tc.name: ConvertLocalPathToCloudPath_PartialPrefix_Test_001
 * @tc.desc: [Partial prefix path returned as-is]
 *           [1] Input "/storage/media/local_backup/files/image.jpg"
 *           [2] Starts with "/storage/media/local" but not "/storage/media/local/", expected to return original path
 */
HWTEST_F(MediaPathUtilsUnitTest, ConvertLocalPathToCloudPath_PartialPrefix_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ConvertLocalPathToCloudPath_PartialPrefix_Test_001 start");
    std::string localPath = "/storage/media/local_backup/files/image.jpg";
    std::string result = MediaPathUtils::ConvertLocalPathToCloudPath(localPath);
    EXPECT_EQ(result, localPath);
}

// ==================== DesensitizePath Tests ====================

/**
 * @tc.name: DesensitizePath_CloudFilesPath_Test_001
 * @tc.desc: [Desensitize cloud files path to "*"+relative path]
 *           [1] Input "/storage/cloud/files/Photo/image.jpg"
 *           [2] Replace cloud files directory prefix with "*", expected to return "*Photo/image.jpg"
 */
HWTEST_F(MediaPathUtilsUnitTest, DesensitizePath_CloudFilesPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("DesensitizePath_CloudFilesPath_Test_001 start");
    std::string path = "/storage/cloud/files/Photo/image.jpg";
    std::string result = MediaPathUtils::DesensitizePath(path);
    EXPECT_EQ(result, "*Photo/image.jpg");
}

/**
 * @tc.name: DesensitizePath_NonCloudPath_Test_001
 * @tc.desc: [Non-cloud path returned as-is]
 *           [1] Input "/data/storage/el2/base/image.jpg"
 *           [2] Does not match cloud files prefix, expected to return original path
 */
HWTEST_F(MediaPathUtilsUnitTest, DesensitizePath_NonCloudPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("DesensitizePath_NonCloudPath_Test_001 start");
    std::string path = "/data/storage/el2/base/image.jpg";
    std::string result = MediaPathUtils::DesensitizePath(path);
    EXPECT_EQ(result, path);
}

/**
 * @tc.name: DesensitizePath_CloudRoot_Test_001
 * @tc.desc: [Cloud root path shorter than files directory returned as-is]
 *           [1] Input "/storage/cloud/"
 *           [2] Path length less than cloud files directory, expected to return original path
 */
HWTEST_F(MediaPathUtilsUnitTest, DesensitizePath_CloudRoot_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("DesensitizePath_CloudRoot_Test_001 start");
    std::string path = "/storage/cloud/";
    std::string result = MediaPathUtils::DesensitizePath(path);
    EXPECT_EQ(result, path);
}

// ==================== AdaptPreferencesPathForAllDevice Tests ====================

/**
 * @tc.name: AdaptPreferencesPathForAllDevice_ValidXmlPath_Test_001
 * @tc.desc: [Valid preferences xml path adapts accountId suffix]
 *           [1] Input "/data/storage/el2/base/preferences/test.xml"
 *           [2] When multi-account enabled, insert accountId suffix into filename; when disabled, return original path
 */
HWTEST_F(MediaPathUtilsUnitTest, AdaptPreferencesPathForAllDevice_ValidXmlPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AdaptPreferencesPathForAllDevice_ValidXmlPath_Test_001 start");
    std::string xmlPath = "/data/storage/el2/base/preferences/test.xml";
    std::string result = MediaPathUtils::AdaptPreferencesPathForAllDevice(xmlPath);
#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
    EXPECT_TRUE(result == "/data/storage/el2/base/preferences/test_100001.xml" || result == xmlPath);
#else
    EXPECT_EQ(result, xmlPath);
#endif
}

/**
 * @tc.name: AdaptPreferencesPathForAllDevice_NonXmlPath_Test_001
 * @tc.desc: [Non-xml file returns original path]
 *           [1] Input "/data/storage/el2/base/preferences/test.txt"
 *           [2] Non-.xml suffix, expected to return original path
 */
HWTEST_F(MediaPathUtilsUnitTest, AdaptPreferencesPathForAllDevice_NonXmlPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AdaptPreferencesPathForAllDevice_NonXmlPath_Test_001 start");
    std::string xmlPath = "/data/storage/el2/base/preferences/test.txt";
    std::string result = MediaPathUtils::AdaptPreferencesPathForAllDevice(xmlPath);
    EXPECT_EQ(result, xmlPath);
}

/**
 * @tc.name: AdaptPreferencesPathForAllDevice_NonPreferencesPath_Test_001
 * @tc.desc: [Path not under preferences directory returns original path]
 *           [1] Input "/data/storage/el2/base/test.xml"
 *           [2] Not under preferences directory, expected to return original path
 */
HWTEST_F(MediaPathUtilsUnitTest, AdaptPreferencesPathForAllDevice_NonPreferencesPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AdaptPreferencesPathForAllDevice_NonPreferencesPath_Test_001 start");
    std::string xmlPath = "/data/storage/el2/base/test.xml";
    std::string result = MediaPathUtils::AdaptPreferencesPathForAllDevice(xmlPath);
    EXPECT_EQ(result, xmlPath);
}

/**
 * @tc.name: AdaptPreferencesPathForAllDevice_EmptyPath_Test_001
 * @tc.desc: [Empty path returns original path]
 *           [1] Input ""
 *           [2] Expected to return empty string
 */
HWTEST_F(MediaPathUtilsUnitTest, AdaptPreferencesPathForAllDevice_EmptyPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AdaptPreferencesPathForAllDevice_EmptyPath_Test_001 start");
    std::string xmlPath = "";
    std::string result = MediaPathUtils::AdaptPreferencesPathForAllDevice(xmlPath);
    EXPECT_EQ(result, xmlPath);
}

/**
 * @tc.name: AdaptPreferencesPathForAllDevice_NoFileName_Test_001
 * @tc.desc: [Path ending with slash without filename returns original path]
 *           [1] Input "/data/storage/el2/base/preferences/"
 *           [2] No filename after rfind('/'), expected to return original path
 */
HWTEST_F(MediaPathUtilsUnitTest, AdaptPreferencesPathForAllDevice_NoFileName_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AdaptPreferencesPathForAllDevice_NoFileName_Test_001 start");
    std::string xmlPath = "/data/storage/el2/base/preferences/";
    std::string result = MediaPathUtils::AdaptPreferencesPathForAllDevice(xmlPath);
    EXPECT_EQ(result, xmlPath);
}

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
/**
 * @tc.name: GetForegroundAccountId_HandlerReturnsTrue_Test_001
 * @tc.desc: [Correctly get accountId when handler returns true]
 *           [1] Register handler returning true with accountId=100001
 *           [2] GetForegroundAccountId returns true, accountId is 100001
 */
HWTEST_F(MediaPathUtilsUnitTest, GetForegroundAccountId_HandlerReturnsTrue_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetForegroundAccountId_HandlerReturnsTrue_Test_001 start");
    MediaPathUtils::RegisterGetAccountId([](int32_t& id) -> bool {
        id = 100001;
        return true;
    });
    int32_t accountId = 0;
    bool ret = MediaPathUtils::GetForegroundAccountId(accountId);
    EXPECT_TRUE(ret);
    EXPECT_EQ(accountId, 100001);
}

/**
 * @tc.name: GetForegroundAccountId_HandlerReturnsTrue_Test_002
 * @tc.desc: [Correctly propagate different accountId from handler]
 *           [1] Register handler returning true with accountId=200003
 *           [2] GetForegroundAccountId returns true, accountId is 200003
 */
HWTEST_F(MediaPathUtilsUnitTest, GetForegroundAccountId_HandlerReturnsTrue_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetForegroundAccountId_HandlerReturnsTrue_Test_002 start");
    MediaPathUtils::RegisterGetAccountId([](int32_t& id) -> bool {
        id = 200003;
        return true;
    });
    int32_t accountId = 0;
    bool ret = MediaPathUtils::GetForegroundAccountId(accountId);
    EXPECT_TRUE(ret);
    EXPECT_EQ(accountId, 200003);
}

/**
 * @tc.name: GetForegroundAccountId_HandlerReturnsFalse_Test_001
 * @tc.desc: [GetForegroundAccountId returns false when handler returns false]
 *           [1] Register handler returning false
 *           [2] GetForegroundAccountId returns false, accountId not modified
 */
HWTEST_F(MediaPathUtilsUnitTest, GetForegroundAccountId_HandlerReturnsFalse_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetForegroundAccountId_HandlerReturnsFalse_Test_001 start");
    MediaPathUtils::RegisterGetAccountId([](int32_t& id) -> bool {
        id = 0;
        return false;
    });
    int32_t accountId = 999;
    bool ret = MediaPathUtils::GetForegroundAccountId(accountId);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: RegisterGetAccountId_NullptrIgnored_Test_001
 * @tc.desc: [Registering nullptr handler is ignored, existing handler retained]
 *           [1] First register valid handler (accountId=300005), then register nullptr
 *           [2] nullptr is ignored, GetForegroundAccountId still uses original handler returning 300005
 */
HWTEST_F(MediaPathUtilsUnitTest, RegisterGetAccountId_NullptrIgnored_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("RegisterGetAccountId_NullptrIgnored_Test_001 start");
    MediaPathUtils::RegisterGetAccountId([](int32_t& id) -> bool {
        id = 300005;
        return true;
    });
    MediaPathUtils::RegisterGetAccountId(nullptr);
    int32_t accountId = 0;
    bool ret = MediaPathUtils::GetForegroundAccountId(accountId);
    EXPECT_TRUE(ret);
    EXPECT_EQ(accountId, 300005);
}

// ---------------------------------------------------------------------------
// GetDir with AdaptAccount::YES (multi-account)
// ---------------------------------------------------------------------------

/**
 * @tc.name: GetDir_AdaptAccountYes_Test_001
 * @tc.desc: [AdaptAccount::YES inserts accountId into CLOUD_FILES path]
 *           [1] Register handler returning accountId=100001
 *           [2] GetDir(CLOUD_FILES, AdaptAccount::YES) expected to return "/storage/cloud/files/100001/"
 */
HWTEST_F(MediaPathUtilsUnitTest, GetDir_AdaptAccountYes_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetDir_AdaptAccountYes_Test_001 start");
    MediaPathUtils::RegisterGetAccountId([](int32_t& id) -> bool {
        id = 100001;
        return true;
    });
    std::string result = MediaPathUtils::GetDir(MediaPathType::CLOUD_FILES, AdaptAccount::YES);
    EXPECT_EQ(result, "/storage/cloud/files/100001/");
}

/**
 * @tc.name: GetDir_AdaptAccountYes_Test_002
 * @tc.desc: [AdaptAccount::YES inserts accountId into LOCAL_PHOTO path]
 *           [1] Register handler returning accountId=200003
 *           [2] GetDir(LOCAL_PHOTO, AdaptAccount::YES) expected to return "/storage/media/local/files/200003/Photo/"
 */
HWTEST_F(MediaPathUtilsUnitTest, GetDir_AdaptAccountYes_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetDir_AdaptAccountYes_Test_002 start");
    MediaPathUtils::RegisterGetAccountId([](int32_t& id) -> bool {
        id = 200003;
        return true;
    });
    std::string result = MediaPathUtils::GetDir(MediaPathType::LOCAL_PHOTO, AdaptAccount::YES);
    EXPECT_EQ(result, "/storage/media/local/files/200003/Photo/");
}

/**
 * @tc.name: GetDir_AdaptAccountYes_HandlerFalse_Test_001
 * @tc.desc: [AdaptAccount::YES does not insert accountId when handler returns false]
 *           [1] Register handler returning false
 *           [2] GetDir(CLOUD_FILES, AdaptAccount::YES)
 *               expected to return path without accountId "/storage/cloud/files/"
 */
HWTEST_F(MediaPathUtilsUnitTest, GetDir_AdaptAccountYes_HandlerFalse_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetDir_AdaptAccountYes_HandlerFalse_Test_001 start");
    MediaPathUtils::RegisterGetAccountId([](int32_t& id) -> bool {
        id = 0;
        return false;
    });
    std::string result = MediaPathUtils::GetDir(MediaPathType::CLOUD_FILES, AdaptAccount::YES);
    EXPECT_EQ(result, "/storage/cloud/files/");
}

// ---------------------------------------------------------------------------
// RemoveAccountId
// ---------------------------------------------------------------------------

/**
 * @tc.name: RemoveAccountId_CloudPath_Test_001
 * @tc.desc: [Remove valid accountId from cloud path (new format)]
 *           [1] Input "/storage/cloud/files/100001/Photo/image.jpg"
 *           [2] 6-digit segment after /files/ is accountId, expected to return "/storage/cloud/files/Photo/image.jpg"
 */
HWTEST_F(MediaPathUtilsUnitTest, RemoveAccountId_CloudPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("RemoveAccountId_CloudPath_Test_001 start");
    std::string path = "/storage/cloud/files/100001/Photo/image.jpg";
    std::string result = MediaPathUtils::RemoveAccountId(path);
    EXPECT_EQ(result, "/storage/cloud/files/Photo/image.jpg");
}

/**
 * @tc.name: RemoveAccountId_LocalPath_Test_001
 * @tc.desc: [Remove valid accountId from local path (new format)]
 *           [1] Input "/storage/media/local/files/200003/Video/movie.mp4"
 *           [2] 6-digit segment after /files/ is accountId,
 *               expected to return "/storage/media/local/files/Video/movie.mp4"
 */
HWTEST_F(MediaPathUtilsUnitTest, RemoveAccountId_LocalPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("RemoveAccountId_LocalPath_Test_001 start");
    std::string path = "/storage/media/local/files/200003/Video/movie.mp4";
    std::string result = MediaPathUtils::RemoveAccountId(path);
    EXPECT_EQ(result, "/storage/media/local/files/Video/movie.mp4");
}

/**
 * @tc.name: RemoveAccountId_NoAccountId_Test_001
 * @tc.desc: [Path without accountId returned as-is]
 *           [1] Input "/storage/cloud/files/Photo/image.jpg"
 *           [2] No 6-digit segment after /files/, expected to return original path
 */
HWTEST_F(MediaPathUtilsUnitTest, RemoveAccountId_NoAccountId_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("RemoveAccountId_NoAccountId_Test_001 start");
    std::string path = "/storage/cloud/files/Photo/image.jpg";
    std::string result = MediaPathUtils::RemoveAccountId(path);
    EXPECT_EQ(result, path);
}

/**
 * @tc.name: RemoveAccountId_NonAccountSegment_Test_001
 * @tc.desc: [Non-6-digit segment after /files/ is not removed]
 *           [1] Input "/storage/cloud/files/mydir/Photo/image.jpg"
 *           [2] "mydir" does not pass IsAccountId validation, expected to return original path
 */
HWTEST_F(MediaPathUtilsUnitTest, RemoveAccountId_NonAccountSegment_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("RemoveAccountId_NonAccountSegment_Test_001 start");
    std::string path = "/storage/cloud/files/mydir/Photo/image.jpg";
    std::string result = MediaPathUtils::RemoveAccountId(path);
    EXPECT_EQ(result, path);
}

/**
 * @tc.name: RemoveAccountId_NonMediaPath_Test_001
 * @tc.desc: [Non-media path returned as-is]
 *           [1] Input "/data/storage/el2/base/haps/entry/files/image.jpg"
 *           [2] Does not match cloud/local prefix, expected to return original path
 */
HWTEST_F(MediaPathUtilsUnitTest, RemoveAccountId_NonMediaPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("RemoveAccountId_NonMediaPath_Test_001 start");
    std::string path = "/data/storage/el2/base/haps/entry/files/image.jpg";
    std::string result = MediaPathUtils::RemoveAccountId(path);
    EXPECT_EQ(result, path);
}

/**
 * @tc.name: RemoveAccountId_EmptyPath_Test_001
 * @tc.desc: [Empty path returns empty string]
 *           [1] Input ""
 *           [2] Expected to return empty string
 */
HWTEST_F(MediaPathUtilsUnitTest, RemoveAccountId_EmptyPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("RemoveAccountId_EmptyPath_Test_001 start");
    std::string path = "";
    std::string result = MediaPathUtils::RemoveAccountId(path);
    EXPECT_EQ(result, "");
}

/**
 * @tc.name: RemoveAccountId_NoFilesMarker_Test_001
 * @tc.desc: [Path without /files/ anchor returned as-is]
 *           [1] Input "/storage/cloud/100001/Photo/image.jpg"
 *           [2] No /files/ anchor to locate accountId, expected to return original path
 */
HWTEST_F(MediaPathUtilsUnitTest, RemoveAccountId_NoFilesMarker_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("RemoveAccountId_NoFilesMarker_Test_001 start");
    std::string path = "/storage/cloud/100001/Photo/image.jpg";
    std::string result = MediaPathUtils::RemoveAccountId(path);
    EXPECT_EQ(result, path);
}

/**
 * @tc.name: RemoveAccountId_ShortAccountId_Test_001
 * @tc.desc: [5-digit number is not a valid accountId, returned as-is]
 *           [1] Input "/storage/cloud/files/10001/Photo/image.jpg"
 *           [2] "10001" is only 5 digits, does not pass IsAccountId validation, expected to return original path
 */
HWTEST_F(MediaPathUtilsUnitTest, RemoveAccountId_ShortAccountId_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("RemoveAccountId_ShortAccountId_Test_001 start");
    std::string path = "/storage/cloud/files/10001/Photo/image.jpg";
    std::string result = MediaPathUtils::RemoveAccountId(path);
    EXPECT_EQ(result, path);
}

/**
 * @tc.name: RemoveAccountId_LongAccountId_Test_001
 * @tc.desc: [7-digit number is not a valid accountId, returned as-is]
 *           [1] Input "/storage/cloud/files/1000001/Photo/image.jpg"
 *           [2] "1000001" is 7 digits, does not pass IsAccountId validation, expected to return original path
 */
HWTEST_F(MediaPathUtilsUnitTest, RemoveAccountId_LongAccountId_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("RemoveAccountId_LongAccountId_Test_001 start");
    std::string path = "/storage/cloud/files/1000001/Photo/image.jpg";
    std::string result = MediaPathUtils::RemoveAccountId(path);
    EXPECT_EQ(result, path);
}

/**
 * @tc.name: RemoveAccountId_LocalThumbPath_Test_001
 * @tc.desc: [Remove accountId from deep local thumbnail path (new format)]
 *           [1] Input "/storage/media/local/files/100001/.thumbs/Photo/abc/LCD.jpg"
 *           [2] Expected to return "/storage/media/local/files/.thumbs/Photo/abc/LCD.jpg"
 */
HWTEST_F(MediaPathUtilsUnitTest, RemoveAccountId_LocalThumbPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("RemoveAccountId_LocalThumbPath_Test_001 start");
    std::string path = "/storage/media/local/files/100001/.thumbs/Photo/abc/LCD.jpg";
    std::string result = MediaPathUtils::RemoveAccountId(path);
    EXPECT_EQ(result, "/storage/media/local/files/.thumbs/Photo/abc/LCD.jpg");
}

// ---------------------------------------------------------------------------
// AppendAccountId
// ---------------------------------------------------------------------------

/**
 * @tc.name: AppendAccountId_CloudPath_Test_001
 * @tc.desc: [Insert explicit accountId after /files/ in cloud path (new format)]
 *           [1] Input "/storage/cloud/files/Photo/image.jpg", accountId=100001
 *           [2] Expected to return "/storage/cloud/files/100001/Photo/image.jpg"
 */
HWTEST_F(MediaPathUtilsUnitTest, AppendAccountId_CloudPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AppendAccountId_CloudPath_Test_001 start");
    std::string path = "/storage/cloud/files/Photo/image.jpg";
    std::string result = MediaPathUtils::AppendAccountId(path, 100001);
    EXPECT_EQ(result, "/storage/cloud/files/100001/Photo/image.jpg");
}

/**
 * @tc.name: AppendAccountId_LocalPath_Test_001
 * @tc.desc: [Insert explicit accountId after /files/ in local path (new format)]
 *           [1] Input "/storage/media/local/files/Video/movie.mp4", accountId=200003
 *           [2] Expected to return "/storage/media/local/files/200003/Video/movie.mp4"
 */
HWTEST_F(MediaPathUtilsUnitTest, AppendAccountId_LocalPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AppendAccountId_LocalPath_Test_001 start");
    std::string path = "/storage/media/local/files/Video/movie.mp4";
    std::string result = MediaPathUtils::AppendAccountId(path, 200003);
    EXPECT_EQ(result, "/storage/media/local/files/200003/Video/movie.mp4");
}

/**
 * @tc.name: AppendAccountId_DefaultAccountId_Test_001
 * @tc.desc: [When accountId=-1, use foreground accountId returned by handler]
 *           [1] Register handler returning accountId=400005, call AppendAccountId(path) with default parameter
 *           [2] Expected to return "/storage/cloud/files/400005/Photo/image.jpg"
 */
HWTEST_F(MediaPathUtilsUnitTest, AppendAccountId_DefaultAccountId_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AppendAccountId_DefaultAccountId_Test_001 start");
    MediaPathUtils::RegisterGetAccountId([](int32_t& id) -> bool {
        id = 400005;
        return true;
    });
    std::string path = "/storage/cloud/files/Photo/image.jpg";
    std::string result = MediaPathUtils::AppendAccountId(path);
    EXPECT_EQ(result, "/storage/cloud/files/400005/Photo/image.jpg");
}

/**
 * @tc.name: AppendAccountId_AlreadyHasAccountId_Test_001
 * @tc.desc: [Path already contains valid accountId (new format), returned as-is]
 *           [1] Input "/storage/cloud/files/100001/Photo/image.jpg", accountId=200003
 *           [2] Path already has accountId after /files/, expected to return original path without duplicate insertion
 */
HWTEST_F(MediaPathUtilsUnitTest, AppendAccountId_AlreadyHasAccountId_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AppendAccountId_AlreadyHasAccountId_Test_001 start");
    std::string path = "/storage/cloud/files/100001/Photo/image.jpg";
    std::string result = MediaPathUtils::AppendAccountId(path, 200003);
    EXPECT_EQ(result, path);
}

/**
 * @tc.name: AppendAccountId_NonMediaPath_Test_001
 * @tc.desc: [Non-cloud/local path returned as-is]
 *           [1] Input "/data/storage/el2/base/haps/entry/files/image.jpg", accountId=100001
 *           [2] Does not match media prefix, expected to return original path
 */
HWTEST_F(MediaPathUtilsUnitTest, AppendAccountId_NonMediaPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AppendAccountId_NonMediaPath_Test_001 start");
    std::string path = "/data/storage/el2/base/haps/entry/files/image.jpg";
    std::string result = MediaPathUtils::AppendAccountId(path, 100001);
    EXPECT_EQ(result, path);
}

/**
 * @tc.name: AppendAccountId_EmptyPath_Test_001
 * @tc.desc: [Empty path returns empty string]
 *           [1] Input "", accountId=100001
 *           [2] Expected to return empty string
 */
HWTEST_F(MediaPathUtilsUnitTest, AppendAccountId_EmptyPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AppendAccountId_EmptyPath_Test_001 start");
    std::string path = "";
    std::string result = MediaPathUtils::AppendAccountId(path, 100001);
    EXPECT_EQ(result, "");
}

/**
 * @tc.name: AppendAccountId_NoFilesMarker_Test_001
 * @tc.desc: [Path without /files/ anchor returned as-is]
 *           [1] Input "/storage/cloud/Photo/image.jpg", accountId=100001
 *           [2] No /files/ anchor, expected to return original path
 */
HWTEST_F(MediaPathUtilsUnitTest, AppendAccountId_NoFilesMarker_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AppendAccountId_NoFilesMarker_Test_001 start");
    std::string path = "/storage/cloud/Photo/image.jpg";
    std::string result = MediaPathUtils::AppendAccountId(path, 100001);
    EXPECT_EQ(result, path);
}

/**
 * @tc.name: AppendAccountId_DefaultAccountIdHandlerFalse_Test_001
 * @tc.desc: [When accountId=-1 and handler returns false, returned as-is]
 *           [1] Register handler returning false, call AppendAccountId(path) with default parameter
 *           [2] Handler unavailable, expected to return original path
 */
HWTEST_F(MediaPathUtilsUnitTest, AppendAccountId_DefaultAccountIdHandlerFalse_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AppendAccountId_DefaultAccountIdHandlerFalse_Test_001 start");
    MediaPathUtils::RegisterGetAccountId([](int32_t& id) -> bool {
        id = 0;
        return false;
    });
    std::string path = "/storage/cloud/files/Photo/image.jpg";
    std::string result = MediaPathUtils::AppendAccountId(path);
    EXPECT_EQ(result, path);
}

/**
 * @tc.name: AppendAccountId_NonAccountSegment_Test_001
 * @tc.desc: [When segment after /files/ is not accountId, insert accountId after /files/]
 *           [1] Input "/storage/cloud/mydir/files/Photo/image.jpg", accountId=100001
 *           [2] "Photo" (segment after /files/) is not accountId, insert after /files/,
 *               expected to return "/storage/cloud/mydir/files/100001/Photo/image.jpg"
 */
HWTEST_F(MediaPathUtilsUnitTest, AppendAccountId_NonAccountSegment_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AppendAccountId_NonAccountSegment_Test_001 start");
    std::string path = "/storage/cloud/mydir/files/Photo/image.jpg";
    std::string result = MediaPathUtils::AppendAccountId(path, 100001);
    EXPECT_EQ(result, "/storage/cloud/mydir/files/100001/Photo/image.jpg");
}

// ---------------------------------------------------------------------------
// CheckAccountId
// ---------------------------------------------------------------------------

/**
 * @tc.name: CheckAccountId_CloudPathMatch_Test_001
 * @tc.desc: [Cloud path accountId matches explicit accountId (new format)]
 *           [1] Input "/storage/cloud/files/100001/Photo/image.jpg", accountId=100001
 *           [2] accountId segment after /files/ matches, expected to return true
 */
HWTEST_F(MediaPathUtilsUnitTest, CheckAccountId_CloudPathMatch_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckAccountId_CloudPathMatch_Test_001 start");
    std::string path = "/storage/cloud/files/100001/Photo/image.jpg";
    bool result = MediaPathUtils::CheckAccountId(path, 100001);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: CheckAccountId_LocalPathMatch_Test_001
 * @tc.desc: [Local path accountId matches explicit accountId (new format)]
 *           [1] Input "/storage/media/local/files/200003/Video/movie.mp4", accountId=200003
 *           [2] accountId segment after /files/ matches, expected to return true
 */
HWTEST_F(MediaPathUtilsUnitTest, CheckAccountId_LocalPathMatch_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckAccountId_LocalPathMatch_Test_001 start");
    std::string path = "/storage/media/local/files/200003/Video/movie.mp4";
    bool result = MediaPathUtils::CheckAccountId(path, 200003);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: CheckAccountId_CloudPathMismatch_Test_001
 * @tc.desc: [Cloud path accountId does not match explicit accountId (new format)]
 *           [1] Input "/storage/cloud/files/100001/Photo/image.jpg", accountId=200003
 *           [2] accountId segment after /files/ does not match, expected to return false
 */
HWTEST_F(MediaPathUtilsUnitTest, CheckAccountId_CloudPathMismatch_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckAccountId_CloudPathMismatch_Test_001 start");
    std::string path = "/storage/cloud/files/100001/Photo/image.jpg";
    bool result = MediaPathUtils::CheckAccountId(path, 200003);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: CheckAccountId_DefaultAccountIdMatch_Test_001
 * @tc.desc: [When accountId=-1, matches foreground accountId returned by handler (new format)]
 *           [1] Register handler returning accountId=500007, input path accountId=500007
 *           [2] Default parameter matches foreground accountId, expected to return true
 */
HWTEST_F(MediaPathUtilsUnitTest, CheckAccountId_DefaultAccountIdMatch_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckAccountId_DefaultAccountIdMatch_Test_001 start");
    MediaPathUtils::RegisterGetAccountId([](int32_t& id) -> bool {
        id = 500007;
        return true;
    });
    std::string path = "/storage/cloud/files/500007/Photo/image.jpg";
    bool result = MediaPathUtils::CheckAccountId(path);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: CheckAccountId_DefaultAccountIdMismatch_Test_001
 * @tc.desc: [When accountId=-1, path accountId does not match foreground accountId (new format)]
 *           [1] Register handler returning accountId=500007, input path accountId=100001
 *           [2] Default parameter does not match foreground accountId, expected to return false
 */
HWTEST_F(MediaPathUtilsUnitTest, CheckAccountId_DefaultAccountIdMismatch_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckAccountId_DefaultAccountIdMismatch_Test_001 start");
    MediaPathUtils::RegisterGetAccountId([](int32_t& id) -> bool {
        id = 500007;
        return true;
    });
    std::string path = "/storage/cloud/files/100001/Photo/image.jpg";
    bool result = MediaPathUtils::CheckAccountId(path);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: CheckAccountId_NoAccountIdInPath_Test_001
 * @tc.desc: [Path without accountId segment returns false (new format)]
 *           [1] Input "/storage/cloud/files/Photo/image.jpg", accountId=100001
 *           [2] No 6-digit segment after /files/ (Photo is not accountId), expected to return false
 */
HWTEST_F(MediaPathUtilsUnitTest, CheckAccountId_NoAccountIdInPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckAccountId_NoAccountIdInPath_Test_001 start");
    std::string path = "/storage/cloud/files/Photo/image.jpg";
    bool result = MediaPathUtils::CheckAccountId(path, 100001);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: CheckAccountId_NonAccountSegment_Test_001
 * @tc.desc: [Non-6-digit segment after /files/ returns false]
 *           [1] Input "/storage/cloud/mydir/files/Photo/image.jpg", accountId=100001
 *           [2] "Photo" (segment after /files/) does not pass IsAccountId validation, expected to return false
 */
HWTEST_F(MediaPathUtilsUnitTest, CheckAccountId_NonAccountSegment_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckAccountId_NonAccountSegment_Test_001 start");
    std::string path = "/storage/cloud/mydir/files/Photo/image.jpg";
    bool result = MediaPathUtils::CheckAccountId(path, 100001);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: CheckAccountId_NonMediaPath_Test_001
 * @tc.desc: [Non-cloud/local path returns false]
 *           [1] Input "/data/storage/el2/base/haps/entry/files/image.jpg", accountId=100001
 *           [2] Does not match media prefix, expected to return false
 */
HWTEST_F(MediaPathUtilsUnitTest, CheckAccountId_NonMediaPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckAccountId_NonMediaPath_Test_001 start");
    std::string path = "/data/storage/el2/base/haps/entry/files/image.jpg";
    bool result = MediaPathUtils::CheckAccountId(path, 100001);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: CheckAccountId_EmptyPath_Test_001
 * @tc.desc: [Empty path returns false]
 *           [1] Input "", accountId=100001
 *           [2] Expected to return false
 */
HWTEST_F(MediaPathUtilsUnitTest, CheckAccountId_EmptyPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckAccountId_EmptyPath_Test_001 start");
    std::string path = "";
    bool result = MediaPathUtils::CheckAccountId(path, 100001);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: CheckAccountId_NoFilesMarker_Test_001
 * @tc.desc: [Path without /files/ anchor returns false]
 *           [1] Input "/storage/cloud/100001/Photo/image.jpg", accountId=100001
 *           [2] No /files/ anchor, expected to return false
 */
HWTEST_F(MediaPathUtilsUnitTest, CheckAccountId_NoFilesMarker_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckAccountId_NoFilesMarker_Test_001 start");
    std::string path = "/storage/cloud/100001/Photo/image.jpg";
    bool result = MediaPathUtils::CheckAccountId(path, 100001);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: CheckAccountId_DefaultAccountIdHandlerFalse_Test_001
 * @tc.desc: [When accountId=-1 and handler returns false, returns false]
 *           [1] Register handler returning false, input path containing accountId
 *           [2] Handler unavailable, expected to return false
 */
HWTEST_F(MediaPathUtilsUnitTest, CheckAccountId_DefaultAccountIdHandlerFalse_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckAccountId_DefaultAccountIdHandlerFalse_Test_001 start");
    MediaPathUtils::RegisterGetAccountId([](int32_t& id) -> bool {
        id = 0;
        return false;
    });
    std::string path = "/storage/cloud/files/100001/Photo/image.jpg";
    bool result = MediaPathUtils::CheckAccountId(path);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: CheckAccountId_ShortAccountIdInPath_Test_001
 * @tc.desc: [5-digit number is not a valid accountId, returns false (new format)]
 *           [1] Input "/storage/cloud/files/10001/Photo/image.jpg", accountId=10001
 *           [2] "10001" is only 5 digits, does not pass IsAccountId validation, expected to return false
 */
HWTEST_F(MediaPathUtilsUnitTest, CheckAccountId_ShortAccountIdInPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckAccountId_ShortAccountIdInPath_Test_001 start");
    std::string path = "/storage/cloud/files/10001/Photo/image.jpg";
    bool result = MediaPathUtils::CheckAccountId(path, 10001);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: CheckAccountId_LocalThumbPathMatch_Test_001
 * @tc.desc: [Deep local thumbnail path accountId matches (new format)]
 *           [1] Input "/storage/media/local/files/600009/.thumbs/Photo/abc/LCD.jpg", accountId=600009
 *           [2] accountId segment after /files/ matches, expected to return true
 */
HWTEST_F(MediaPathUtilsUnitTest, CheckAccountId_LocalThumbPathMatch_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckAccountId_LocalThumbPathMatch_Test_001 start");
    std::string path = "/storage/media/local/files/600009/.thumbs/Photo/abc/LCD.jpg";
    bool result = MediaPathUtils::CheckAccountId(path, 600009);
    EXPECT_TRUE(result);
}

// ---------------------------------------------------------------------------
// ParseAccountIdFromPath (multi-account)
// ---------------------------------------------------------------------------

/**
 * @tc.name: ParseAccountIdFromPath_CloudPath_Test_001
 * @tc.desc: [Parse valid accountId from cloud path (new format)]
 *           [1] Input "/storage/cloud/files/100001/Photo/image.jpg"
 *           [2] accountId segment after /files/ is "100001", expected to return 100001
 */
HWTEST_F(MediaPathUtilsUnitTest, ParseAccountIdFromPath_CloudPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ParseAccountIdFromPath_CloudPath_Test_001 start");
    std::string path = "/storage/cloud/files/100001/Photo/image.jpg";
    int32_t result = MediaPathUtils::ParseAccountIdFromPath(path);
    EXPECT_EQ(result, 100001);
}

/**
 * @tc.name: ParseAccountIdFromPath_LocalPath_Test_001
 * @tc.desc: [Parse valid accountId from local path (new format)]
 *           [1] Input "/storage/media/local/files/200003/Video/movie.mp4"
 *           [2] accountId segment after /files/ is "200003", expected to return 200003
 */
HWTEST_F(MediaPathUtilsUnitTest, ParseAccountIdFromPath_LocalPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ParseAccountIdFromPath_LocalPath_Test_001 start");
    std::string path = "/storage/media/local/files/200003/Video/movie.mp4";
    int32_t result = MediaPathUtils::ParseAccountIdFromPath(path);
    EXPECT_EQ(result, 200003);
}

/**
 * @tc.name: ParseAccountIdFromPath_LocalThumbPath_Test_001
 * @tc.desc: [Parse valid accountId from deep local thumbnail path (new format)]
 *           [1] Input "/storage/media/local/files/600009/.thumbs/Photo/abc/LCD.jpg"
 *           [2] accountId segment after /files/ is "600009", expected to return 600009
 */
HWTEST_F(MediaPathUtilsUnitTest, ParseAccountIdFromPath_LocalThumbPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ParseAccountIdFromPath_LocalThumbPath_Test_001 start");
    std::string path = "/storage/media/local/files/600009/.thumbs/Photo/abc/LCD.jpg";
    int32_t result = MediaPathUtils::ParseAccountIdFromPath(path);
    EXPECT_EQ(result, 600009);
}

/**
 * @tc.name: ParseAccountIdFromPath_NoAccountId_Test_001
 * @tc.desc: [Path without accountId segment returns INVALID_ACCOUNT_ID]
 *           [1] Input "/storage/cloud/files/Photo/image.jpg"
 *           [2] "Photo" is not a valid accountId, expected to return -1
 */
HWTEST_F(MediaPathUtilsUnitTest, ParseAccountIdFromPath_NoAccountId_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ParseAccountIdFromPath_NoAccountId_Test_001 start");
    std::string path = "/storage/cloud/files/Photo/image.jpg";
    int32_t result = MediaPathUtils::ParseAccountIdFromPath(path);
    EXPECT_EQ(result, -1);
}

/**
 * @tc.name: ParseAccountIdFromPath_NonAccountSegment_Test_001
 * @tc.desc: [Non-6-digit segment after /files/ returns INVALID_ACCOUNT_ID]
 *           [1] Input "/storage/cloud/files/mydir/Photo/image.jpg"
 *           [2] "mydir" does not pass IsAccountId validation, expected to return -1
 */
HWTEST_F(MediaPathUtilsUnitTest, ParseAccountIdFromPath_NonAccountSegment_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ParseAccountIdFromPath_NonAccountSegment_Test_001 start");
    std::string path = "/storage/cloud/files/mydir/Photo/image.jpg";
    int32_t result = MediaPathUtils::ParseAccountIdFromPath(path);
    EXPECT_EQ(result, -1);
}

/**
 * @tc.name: ParseAccountIdFromPath_ShortAccountId_Test_001
 * @tc.desc: [5-digit number is not a valid accountId, returns INVALID_ACCOUNT_ID]
 *           [1] Input "/storage/cloud/files/10001/Photo/image.jpg"
 *           [2] "10001" is only 5 digits, does not pass IsAccountId validation, expected to return -1
 */
HWTEST_F(MediaPathUtilsUnitTest, ParseAccountIdFromPath_ShortAccountId_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ParseAccountIdFromPath_ShortAccountId_Test_001 start");
    std::string path = "/storage/cloud/files/10001/Photo/image.jpg";
    int32_t result = MediaPathUtils::ParseAccountIdFromPath(path);
    EXPECT_EQ(result, -1);
}

/**
 * @tc.name: ParseAccountIdFromPath_LongAccountId_Test_001
 * @tc.desc: [7-digit number is not a valid accountId, returns INVALID_ACCOUNT_ID]
 *           [1] Input "/storage/cloud/files/1000001/Photo/image.jpg"
 *           [2] "1000001" is 7 digits, does not pass IsAccountId validation, expected to return -1
 */
HWTEST_F(MediaPathUtilsUnitTest, ParseAccountIdFromPath_LongAccountId_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ParseAccountIdFromPath_LongAccountId_Test_001 start");
    std::string path = "/storage/cloud/files/1000001/Photo/image.jpg";
    int32_t result = MediaPathUtils::ParseAccountIdFromPath(path);
    EXPECT_EQ(result, -1);
}

/**
 * @tc.name: ParseAccountIdFromPath_NonMediaPath_Test_001
 * @tc.desc: [Non-cloud/local path returns INVALID_ACCOUNT_ID]
 *           [1] Input "/data/storage/el2/base/haps/entry/files/image.jpg"
 *           [2] Does not match cloud/local prefix, expected to return -1
 */
HWTEST_F(MediaPathUtilsUnitTest, ParseAccountIdFromPath_NonMediaPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ParseAccountIdFromPath_NonMediaPath_Test_001 start");
    std::string path = "/data/storage/el2/base/haps/entry/files/image.jpg";
    int32_t result = MediaPathUtils::ParseAccountIdFromPath(path);
    EXPECT_EQ(result, -1);
}

/**
 * @tc.name: ParseAccountIdFromPath_EmptyPath_Test_001
 * @tc.desc: [Empty path returns INVALID_ACCOUNT_ID]
 *           [1] Input ""
 *           [2] Expected to return -1
 */
HWTEST_F(MediaPathUtilsUnitTest, ParseAccountIdFromPath_EmptyPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ParseAccountIdFromPath_EmptyPath_Test_001 start");
    std::string path = "";
    int32_t result = MediaPathUtils::ParseAccountIdFromPath(path);
    EXPECT_EQ(result, -1);
}

/**
 * @tc.name: ParseAccountIdFromPath_NoFilesMarker_Test_001
 * @tc.desc: [Path without /files/ anchor returns INVALID_ACCOUNT_ID]
 *           [1] Input "/storage/cloud/100001/Photo/image.jpg"
 *           [2] No /files/ anchor to locate accountId, expected to return -1
 */
HWTEST_F(MediaPathUtilsUnitTest, ParseAccountIdFromPath_NoFilesMarker_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ParseAccountIdFromPath_NoFilesMarker_Test_001 start");
    std::string path = "/storage/cloud/100001/Photo/image.jpg";
    int32_t result = MediaPathUtils::ParseAccountIdFromPath(path);
    EXPECT_EQ(result, -1);
}

/**
 * @tc.name: ParseAccountIdFromPath_NoTrailingSlash_Test_001
 * @tc.desc: [Path where accountId is the last segment (no trailing slash) returns INVALID_ACCOUNT_ID]
 *           [1] Input "/storage/cloud/files/100001"
 *           [2] No '/' after accountId segment, expected to return -1
 */
HWTEST_F(MediaPathUtilsUnitTest, ParseAccountIdFromPath_NoTrailingSlash_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ParseAccountIdFromPath_NoTrailingSlash_Test_001 start");
    std::string path = "/storage/cloud/files/100001";
    int32_t result = MediaPathUtils::ParseAccountIdFromPath(path);
    EXPECT_EQ(result, -1);
}

// ---------------------------------------------------------------------------
// DesensitizePath with AdaptAccount (multi-account)
// ---------------------------------------------------------------------------

/**
 * @tc.name: DesensitizePath_CloudPathWithAccount_Test_001
 * @tc.desc: [Desensitize cloud path containing accountId (new format)]
 *           [1] Register handler returning accountId=100001, input "/storage/cloud/files/100001/Photo/image.jpg"
 *           [2] Desensitize replaces cloud files dir prefix with "*", expected to return "*Photo/image.jpg"
 */
HWTEST_F(MediaPathUtilsUnitTest, DesensitizePath_CloudPathWithAccount_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("DesensitizePath_CloudPathWithAccount_Test_001 start");
    MediaPathUtils::RegisterGetAccountId([](int32_t& id) -> bool {
        id = 100001;
        return true;
    });
    std::string path = "/storage/cloud/files/100001/Photo/image.jpg";
    std::string result = MediaPathUtils::DesensitizePath(path);
    EXPECT_EQ(result, "*Photo/image.jpg");
}

/**
 * @tc.name: DesensitizePath_CloudPathWithDifferentAccount_Test_001
 * @tc.desc: [Desensitize cloud path whose accountId differs from foreground (fallback to no-account prefix)]
 *           [1] Register handler returning accountId=100001, input "/storage/cloud/files/200003/Photo/image.jpg"
 *           [2] accountId prefix does not match foreground, fallback to no-account prefix,
 *               expected to return "*200003/Photo/image.jpg"
 */
HWTEST_F(MediaPathUtilsUnitTest, DesensitizePath_CloudPathWithDifferentAccount_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("DesensitizePath_CloudPathWithDifferentAccount_Test_001 start");
    MediaPathUtils::RegisterGetAccountId([](int32_t& id) -> bool {
        id = 100001;
        return true;
    });
    std::string path = "/storage/cloud/files/200003/Photo/image.jpg";
    std::string result = MediaPathUtils::DesensitizePath(path);
    EXPECT_EQ(result, "*200003/Photo/image.jpg");
}

/**
 * @tc.name: DesensitizePath_CloudPathNoAccount_Test_001
 * @tc.desc: [Desensitize cloud path without accountId segment (fallback to no-account prefix)]
 *           [1] Register handler returning accountId=100001, input "/storage/cloud/files/Photo/image.jpg"
 *           [2] accountId prefix does not match (no accountId in path), fallback to no-account prefix,
 *               expected to return "*Photo/image.jpg"
 */
HWTEST_F(MediaPathUtilsUnitTest, DesensitizePath_CloudPathNoAccount_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("DesensitizePath_CloudPathNoAccount_Test_001 start");
    MediaPathUtils::RegisterGetAccountId([](int32_t& id) -> bool {
        id = 100001;
        return true;
    });
    std::string path = "/storage/cloud/files/Photo/image.jpg";
    std::string result = MediaPathUtils::DesensitizePath(path);
    EXPECT_EQ(result, "*Photo/image.jpg");
}

// ---------------------------------------------------------------------------
// GetLocalThumbnailFilePath (multi-account)
// ---------------------------------------------------------------------------

/**
 * @tc.name: GetLocalThumbnailFilePath_ValidCloudPath_Test_001
 * @tc.desc: [Convert cloud photo path to local LCD thumbnail path (new format)]
 *           [1] Register handler returning accountId=100001, input cloud photo path, type LCD
 *           [2] Expected to return "/storage/media/local/files/100001/.thumbs/Photo/image.jpg/LCD.jpg"
 */
HWTEST_F(MediaPathUtilsUnitTest, GetLocalThumbnailFilePath_ValidCloudPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetLocalThumbnailFilePath_ValidCloudPath_Test_001 start");
    MediaPathUtils::RegisterGetAccountId([](int32_t& id) -> bool {
        id = 100001;
        return true;
    });
    std::string photoPath = "/storage/cloud/files/100001/Photo/image.jpg";
    std::string result = MediaPathUtils::GetLocalThumbnailFilePath(photoPath, ThumbnailFileType::LCD);
    EXPECT_EQ(result, "/storage/media/local/files/100001/.thumbs/Photo/image.jpg/LCD.jpg");
}

/**
 * @tc.name: GetLocalThumbnailFilePath_ValidCloudPath_Test_002
 * @tc.desc: [Convert cloud photo path to local LCD_EX thumbnail path (new format)]
 *           [1] Register handler returning accountId=200003, input cloud video path, type LCD_EX
 *           [2] Expected to return "/storage/media/local/files/200003/.thumbs/Photo/video.mp4/THM_EX/LCD.jpg"
 */
HWTEST_F(MediaPathUtilsUnitTest, GetLocalThumbnailFilePath_ValidCloudPath_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetLocalThumbnailFilePath_ValidCloudPath_Test_002 start");
    MediaPathUtils::RegisterGetAccountId([](int32_t& id) -> bool {
        id = 200003;
        return true;
    });
    std::string photoPath = "/storage/cloud/files/200003/Photo/video.mp4";
    std::string result = MediaPathUtils::GetLocalThumbnailFilePath(photoPath, ThumbnailFileType::LCD_EX);
    EXPECT_EQ(result, "/storage/media/local/files/200003/.thumbs/Photo/video.mp4/THM_EX/LCD.jpg");
}

/**
 * @tc.name: GetLocalThumbnailFilePath_PathTraversal_Test_001
 * @tc.desc: [Path containing ".." is rejected by path traversal protection]
 *           [1] Register handler returning accountId=100001, input cloud path containing ".."
 *           [2] Path traversal protection triggered, expected to return empty string
 */
HWTEST_F(MediaPathUtilsUnitTest, GetLocalThumbnailFilePath_PathTraversal_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetLocalThumbnailFilePath_PathTraversal_Test_001 start");
    MediaPathUtils::RegisterGetAccountId([](int32_t& id) -> bool {
        id = 100001;
        return true;
    });
    std::string photoPath = "/storage/cloud/files/100001/../Photo/image.jpg";
    std::string result = MediaPathUtils::GetLocalThumbnailFilePath(photoPath, ThumbnailFileType::LCD);
    EXPECT_EQ(result, "");
}

/**
 * @tc.name: GetLocalThumbnailFilePath_NonCloudPath_Test_001
 * @tc.desc: [Non-cloud path returns empty string]
 *           [1] Register handler returning accountId=100001, input non-cloud path "/data/storage/image.jpg"
 *           [2] Does not match cloud prefix, expected to return empty string
 */
HWTEST_F(MediaPathUtilsUnitTest, GetLocalThumbnailFilePath_NonCloudPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetLocalThumbnailFilePath_NonCloudPath_Test_001 start");
    MediaPathUtils::RegisterGetAccountId([](int32_t& id) -> bool {
        id = 100001;
        return true;
    });
    std::string photoPath = "/data/storage/image.jpg";
    std::string result = MediaPathUtils::GetLocalThumbnailFilePath(photoPath, ThumbnailFileType::LCD);
    EXPECT_EQ(result, "");
}

// ---------------------------------------------------------------------------
// Combined / Round-trip tests
// ---------------------------------------------------------------------------

/**
 * @tc.name: RoundTrip_RemoveThenAppend_CloudPath_Test_001
 * @tc.desc: [Cloud path Remove->Append round-trip consistency (new format)]
 *           [1] Original path "/storage/cloud/files/100001/Photo/image.jpg"
 *           [2] RemoveAccountId then AppendAccountId(100001), expected to restore original path
 */
HWTEST_F(MediaPathUtilsUnitTest, RoundTrip_RemoveThenAppend_CloudPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("RoundTrip_RemoveThenAppend_CloudPath_Test_001 start");
    std::string original = "/storage/cloud/files/100001/Photo/image.jpg";
    std::string removed = MediaPathUtils::RemoveAccountId(original);
    EXPECT_EQ(removed, "/storage/cloud/files/Photo/image.jpg");
    std::string appended = MediaPathUtils::AppendAccountId(removed, 100001);
    EXPECT_EQ(appended, original);
}

/**
 * @tc.name: RoundTrip_RemoveThenAppend_LocalPath_Test_001
 * @tc.desc: [Local path Remove->Append round-trip consistency (new format)]
 *           [1] Original path "/storage/media/local/files/200003/Video/movie.mp4"
 *           [2] RemoveAccountId then AppendAccountId(200003), expected to restore original path
 */
HWTEST_F(MediaPathUtilsUnitTest, RoundTrip_RemoveThenAppend_LocalPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("RoundTrip_RemoveThenAppend_LocalPath_Test_001 start");
    std::string original = "/storage/media/local/files/200003/Video/movie.mp4";
    std::string removed = MediaPathUtils::RemoveAccountId(original);
    EXPECT_EQ(removed, "/storage/media/local/files/Video/movie.mp4");
    std::string appended = MediaPathUtils::AppendAccountId(removed, 200003);
    EXPECT_EQ(appended, original);
}

/**
 * @tc.name: AppendThenCheck_CloudPath_Test_001
 * @tc.desc: [CheckAccountId match validation after Append]
 *           [1] Input "/storage/cloud/files/Photo/image.jpg", AppendAccountId(100001)
 *           [2] CheckAccountId matching 100001 returns true, not matching 200003 returns false
 */
HWTEST_F(MediaPathUtilsUnitTest, AppendThenCheck_CloudPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AppendThenCheck_CloudPath_Test_001 start");
    std::string path = "/storage/cloud/files/Photo/image.jpg";
    std::string appended = MediaPathUtils::AppendAccountId(path, 100001);
    bool match = MediaPathUtils::CheckAccountId(appended, 100001);
    EXPECT_TRUE(match);
    bool mismatch = MediaPathUtils::CheckAccountId(appended, 200003);
    EXPECT_FALSE(mismatch);
}

/**
 * @tc.name: CloudLocalConvert_RoundTrip_Test_001
 * @tc.desc: [cloud->local->cloud round-trip consistency]
 *           [1] Original path "/storage/cloud/files/Photo/image.jpg"
 *           [2] ConvertCloudPathToLocalPath then ConvertLocalPathToCloudPath, expected to restore original path
 */
HWTEST_F(MediaPathUtilsUnitTest, CloudLocalConvert_RoundTrip_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CloudLocalConvert_RoundTrip_Test_001 start");
    std::string original = "/storage/cloud/files/Photo/image.jpg";
    std::string local = MediaPathUtils::ConvertCloudPathToLocalPath(original);
    EXPECT_EQ(local, "/storage/media/local/files/Photo/image.jpg");
    std::string back = MediaPathUtils::ConvertLocalPathToCloudPath(local);
    EXPECT_EQ(back, original);
}

#endif // MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

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
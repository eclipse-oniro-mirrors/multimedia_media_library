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

#include "medialibrary_helper_test.h"

#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <unistd.h>
#include "medialibrary_errno.h"

#include "media_file_utils.h"
#include "moving_photo_file_utils.h"
#include "single_file_moving_photo_utils.h"
#include "unique_fd.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
static const string TEST_DIR = "/data/test/single_file_moving_photo";

class SingleFileMovingPhotoUtilsTest : public MediaLibraryHelperUnitTest {
public:
    void SetUp() override
    {
        MediaLibraryHelperUnitTest::SetUp();
        std::error_code errCode;
        std::filesystem::create_directories(TEST_DIR, errCode);
    }
    void TearDown() override
    {
        std::error_code errCode;
        std::filesystem::remove_all(TEST_DIR, errCode);
        MediaLibraryHelperUnitTest::TearDown();
    }
};

// ---- NormalizeExistingFile ----
HWTEST_F(SingleFileMovingPhotoUtilsTest, NormalizeExistingFile_Normal_001, TestSize.Level1)
{
    string path = TEST_DIR + "/normal.jpg";
    MediaFileUtils::CreateAsset(path);
    string outPath;
    EXPECT_EQ(SingleFileMovingPhotoUtils::NormalizeExistingFile(path, outPath), E_OK);
    EXPECT_FALSE(outPath.empty());
    // 用明确的后缀比对，避免硬编码长度在目录结构变化时误判
    const string suffix = "/normal.jpg";
    ASSERT_GE(outPath.size(), suffix.size());
    EXPECT_EQ(outPath.compare(outPath.size() - suffix.size(), suffix.size(), suffix), 0);
}

HWTEST_F(SingleFileMovingPhotoUtilsTest, NormalizeExistingFile_Empty_002, TestSize.Level1)
{
    string outPath;
    EXPECT_EQ(SingleFileMovingPhotoUtils::NormalizeExistingFile("", outPath), E_INVALID_VALUES);
    EXPECT_TRUE(outPath.empty());
}

HWTEST_F(SingleFileMovingPhotoUtilsTest, NormalizeExistingFile_Traversal_003, TestSize.Level1)
{
    string outPath;
    EXPECT_EQ(SingleFileMovingPhotoUtils::NormalizeExistingFile(TEST_DIR + "/../normal.jpg", outPath),
        E_INVALID_VALUES);
}

HWTEST_F(SingleFileMovingPhotoUtilsTest, NormalizeExistingFile_NotExist_004, TestSize.Level1)
{
    string outPath;
    EXPECT_EQ(SingleFileMovingPhotoUtils::NormalizeExistingFile(TEST_DIR + "/not_exist.jpg", outPath),
        E_INVALID_VALUES);
}

HWTEST_F(SingleFileMovingPhotoUtilsTest, NormalizeExistingFile_IsDir_005, TestSize.Level1)
{
    string outPath;
    EXPECT_EQ(SingleFileMovingPhotoUtils::NormalizeExistingFile(TEST_DIR, outPath), E_INVALID_VALUES);
}

// ---- NormalizeOutputDir ----
HWTEST_F(SingleFileMovingPhotoUtilsTest, NormalizeOutputDir_Normal_006, TestSize.Level1)
{
    string outDir;
    EXPECT_EQ(SingleFileMovingPhotoUtils::NormalizeOutputDir(TEST_DIR, outDir), E_OK);
    EXPECT_FALSE(outDir.empty());
}

HWTEST_F(SingleFileMovingPhotoUtilsTest, NormalizeOutputDir_Empty_007, TestSize.Level1)
{
    string outDir;
    EXPECT_EQ(SingleFileMovingPhotoUtils::NormalizeOutputDir("", outDir), E_INVALID_VALUES);
}

HWTEST_F(SingleFileMovingPhotoUtilsTest, NormalizeOutputDir_Traversal_008, TestSize.Level1)
{
    string outDir;
    EXPECT_EQ(SingleFileMovingPhotoUtils::NormalizeOutputDir(TEST_DIR + "/../", outDir), E_INVALID_VALUES);
}

HWTEST_F(SingleFileMovingPhotoUtilsTest, NormalizeOutputDir_NotExist_009, TestSize.Level1)
{
    string outDir;
    EXPECT_EQ(SingleFileMovingPhotoUtils::NormalizeOutputDir(TEST_DIR + "/not_exist", outDir), E_INVALID_VALUES);
}

HWTEST_F(SingleFileMovingPhotoUtilsTest, NormalizeOutputDir_IsFile_010, TestSize.Level1)
{
    string path = TEST_DIR + "/plain.jpg";
    MediaFileUtils::CreateAsset(path);
    string outDir;
    EXPECT_EQ(SingleFileMovingPhotoUtils::NormalizeOutputDir(path, outDir), E_INVALID_VALUES);
}

// ---- 后缀提取的四个分支，经 CheckImageSourceExtension 间接覆盖 ----
HWTEST_F(SingleFileMovingPhotoUtilsTest, CheckImageSourceExtension_Normal_011, TestSize.Level1)
{
    EXPECT_EQ(SingleFileMovingPhotoUtils::CheckImageSourceExtension(TEST_DIR + "/a.jpg"), E_OK);
    EXPECT_EQ(SingleFileMovingPhotoUtils::CheckImageSourceExtension(TEST_DIR + "/a.jpeg"), E_OK);
    EXPECT_EQ(SingleFileMovingPhotoUtils::CheckImageSourceExtension(TEST_DIR + "/a.heic"), E_OK);
    EXPECT_EQ(SingleFileMovingPhotoUtils::CheckImageSourceExtension(TEST_DIR + "/a.heif"), E_OK);
}

HWTEST_F(SingleFileMovingPhotoUtilsTest, CheckImageSourceExtension_NoDot_012, TestSize.Level1)
{
    EXPECT_EQ(SingleFileMovingPhotoUtils::CheckImageSourceExtension(TEST_DIR + "/noext"), E_INVALID_VALUES);
}

HWTEST_F(SingleFileMovingPhotoUtilsTest, CheckImageSourceExtension_DotInDir_013, TestSize.Level1)
{
    // 点号落在目录段时后缀为空，按不支持后缀拒绝
    EXPECT_EQ(SingleFileMovingPhotoUtils::CheckImageSourceExtension(TEST_DIR + "/a.b/c"), E_INVALID_VALUES);
}

HWTEST_F(SingleFileMovingPhotoUtilsTest, CheckImageSourceExtension_TrailingDot_014, TestSize.Level1)
{
    EXPECT_EQ(SingleFileMovingPhotoUtils::CheckImageSourceExtension(TEST_DIR + "/a."), E_INVALID_VALUES);
}

HWTEST_F(SingleFileMovingPhotoUtilsTest, CheckImageSourceExtension_Unsupported_015, TestSize.Level1)
{
    EXPECT_EQ(SingleFileMovingPhotoUtils::CheckImageSourceExtension(TEST_DIR + "/a.png"), E_INVALID_VALUES);
}

// ---- IsSupportedImageExtension ----
HWTEST_F(SingleFileMovingPhotoUtilsTest, IsSupportedImageExtension_016, TestSize.Level1)
{
    EXPECT_TRUE(SingleFileMovingPhotoUtils::IsSupportedImageExtension("jpg"));
    EXPECT_TRUE(SingleFileMovingPhotoUtils::IsSupportedImageExtension(".HEIC"));
    EXPECT_FALSE(SingleFileMovingPhotoUtils::IsSupportedImageExtension("png"));
    EXPECT_FALSE(SingleFileMovingPhotoUtils::IsSupportedImageExtension(""));
}

// ---- CheckPackingVideoMimetype ----
HWTEST_F(SingleFileMovingPhotoUtilsTest, CheckPackingVideoMimetype_NotMp4_017, TestSize.Level1)
{
    EXPECT_EQ(SingleFileMovingPhotoUtils::CheckPackingVideoMimetype(TEST_DIR + "/a.mov"), E_INVALID_VALUES);
}

HWTEST_F(SingleFileMovingPhotoUtilsTest, CheckPackingVideoMimetype_Empty_018, TestSize.Level1)
{
    EXPECT_EQ(SingleFileMovingPhotoUtils::CheckPackingVideoMimetype(""), E_INVALID_VALUES);
}

// ---- ResolveOutputImageExtension / ResolveOutputVideoExtension ----
HWTEST_F(SingleFileMovingPhotoUtilsTest, ResolveOutputImageExtension_InvalidFd_019, TestSize.Level1)
{
    // fd 非法时不降级为默认后缀，直接返回参数错误并清空出参
    std::string extension = "keep";
    EXPECT_EQ(SingleFileMovingPhotoUtils::ResolveOutputImageExtension(-1, extension), E_INVALID_VALUES);
    EXPECT_TRUE(extension.empty());
}

HWTEST_F(SingleFileMovingPhotoUtilsTest, ResolveOutputVideoExtension_InvalidRange_020, TestSize.Level1)
{
    // 区间非法时同样不降级，直接返回参数错误
    std::string extension = "keep";
    EXPECT_EQ(SingleFileMovingPhotoUtils::ResolveOutputVideoExtension(-1, 0, 0, extension), E_INVALID_VALUES);
    EXPECT_TRUE(extension.empty());
}

HWTEST_F(SingleFileMovingPhotoUtilsTest, ResolveOutputImageExtension_UnrecognizedContent_021, TestSize.Level1)
{
    // 内容无法识别 MIME 时不降级为默认后缀，直接失败并由上层判定为非法合一文件
    string path = TEST_DIR + "/unrecognized.jpg";
    MediaFileUtils::CreateAsset(path);
    UniqueFd fd(open(path.c_str(), O_RDONLY));
    ASSERT_GE(fd.Get(), 0);
    std::string extension = "keep";
    EXPECT_EQ(SingleFileMovingPhotoUtils::ResolveOutputImageExtension(fd.Get(), extension), E_INVALID_VALUES);
    EXPECT_TRUE(extension.empty());
    MediaFileUtils::DeleteFile(path);
}

HWTEST_F(SingleFileMovingPhotoUtilsTest, ResolveOutputVideoExtension_UnrecognizedContent_022, TestSize.Level1)
{
    // 视频段无法解析出 MIME 时直接失败，不再固定写成 mp4
    string path = TEST_DIR + "/unrecognized.mp4";
    MediaFileUtils::CreateAsset(path);
    string garbage(MIN_STANDARD_SIZE, '0');
    {
        ofstream ofs(path, ios::binary | ios::trunc);
        ASSERT_TRUE(ofs.is_open());
        ofs.write(garbage.c_str(), garbage.length());
    }
    UniqueFd fd(open(path.c_str(), O_RDONLY));
    ASSERT_GE(fd.Get(), 0);
    std::string extension = "keep";
    EXPECT_EQ(SingleFileMovingPhotoUtils::ResolveOutputVideoExtension(fd.Get(), 0, MIN_STANDARD_SIZE, extension),
        E_INVALID_VALUES);
    EXPECT_TRUE(extension.empty());
    MediaFileUtils::DeleteFile(path);
}

// ---- 合一文件识别与打包拒绝 ----
HWTEST_F(SingleFileMovingPhotoUtilsTest, IsSingleFileMovingPhoto_PlainImage_023, TestSize.Level1)
{
    string path = TEST_DIR + "/plain.jpg";
    MediaFileUtils::CreateAsset(path);
    EXPECT_FALSE(SingleFileMovingPhotoUtils::IsSingleFileMovingPhoto(path));
}

HWTEST_F(SingleFileMovingPhotoUtilsTest, ParseSingleFileMovingPhoto_PlainImage_024, TestSize.Level1)
{
    // 空文件在 GetFileSize 阶段返回 E_INVALID_VALUES，经归一化后固定为格式错误
    string path = TEST_DIR + "/plain2.jpg";
    MediaFileUtils::CreateAsset(path);
    SingleFileMovingPhotoInfo info;
    EXPECT_EQ(SingleFileMovingPhotoUtils::ParseSingleFileMovingPhoto(path, info), E_INVALID_LIVE_PHOTO);
    EXPECT_FALSE(info.IsSingleFileMovingPhoto());
    EXPECT_EQ(info.totalSize, 0);
}

HWTEST_F(SingleFileMovingPhotoUtilsTest, OpenReadOnly_NotExist_025, TestSize.Level1)
{
    // OpenReadOnly 经 ParseSingleFileMovingPhoto 间接覆盖，确认替换后仍返回文件系统错误
    SingleFileMovingPhotoInfo info;
    int32_t ret = SingleFileMovingPhotoUtils::ParseSingleFileMovingPhoto(TEST_DIR + "/not_exist.jpg", info);
    EXPECT_EQ(ret, E_HAS_FS_ERROR);
}

// ---- GetVideoOffset 对普通图片返回格式错误 ----
HWTEST_F(SingleFileMovingPhotoUtilsTest, GetVideoOffset_PlainImage_026, TestSize.Level1)
{
    // GetVideoOffset 直接透传 ParseSingleFileMovingPhoto 的格式错误，失败时不得写出参
    string path = TEST_DIR + "/plain3.jpg";
    MediaFileUtils::CreateAsset(path);
    int64_t offset = 0;
    EXPECT_EQ(SingleFileMovingPhotoUtils::GetVideoOffset(path, offset), E_INVALID_LIVE_PHOTO);
    EXPECT_EQ(offset, 0);
}

// ---- 拆分产物复核：两个产物都必须已落盘且非空 ----
HWTEST_F(SingleFileMovingPhotoUtilsTest, CheckDecodedResourceFiles_Normal_027, TestSize.Level1)
{
    string imagePath = TEST_DIR + "/decoded.jpg";
    string videoPath = TEST_DIR + "/decoded.mp4";
    ofstream(imagePath, ios::binary) << "image";
    ofstream(videoPath, ios::binary) << "video";
    EXPECT_TRUE(SingleFileMovingPhotoUtils::CheckDecodedResourceFiles(imagePath, videoPath));
}

HWTEST_F(SingleFileMovingPhotoUtilsTest, CheckDecodedResourceFiles_EmptyProduct_028, TestSize.Level1)
{
    // 写盘失败可能只留下空文件，不能当成功结果返回
    string imagePath = TEST_DIR + "/empty.jpg";
    string videoPath = TEST_DIR + "/empty.mp4";
    MediaFileUtils::CreateAsset(imagePath);
    ofstream(videoPath, ios::binary) << "video";
    EXPECT_FALSE(SingleFileMovingPhotoUtils::CheckDecodedResourceFiles(imagePath, videoPath));
    ofstream(imagePath, ios::binary) << "image";
    ofstream(videoPath, ios::binary | ios::trunc).flush();
    EXPECT_FALSE(SingleFileMovingPhotoUtils::CheckDecodedResourceFiles(imagePath, videoPath));
}

HWTEST_F(SingleFileMovingPhotoUtilsTest, CheckDecodedResourceFiles_MissingProduct_029, TestSize.Level1)
{
    string imagePath = TEST_DIR + "/missing.jpg";
    string videoPath = TEST_DIR + "/missing.mp4";
    ofstream(imagePath, ios::binary) << "image";
    EXPECT_FALSE(SingleFileMovingPhotoUtils::CheckDecodedResourceFiles(imagePath, videoPath));
    EXPECT_FALSE(SingleFileMovingPhotoUtils::CheckDecodedResourceFiles("", videoPath));
}
} // namespace Media
} // namespace OHOS

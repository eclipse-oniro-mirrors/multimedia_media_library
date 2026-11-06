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
#include <unistd.h>

#include <fstream>
#include <iterator>
#include <string>

#include "datashare_values_bucket.h"
#include "media_file_utils.h"
#include "media_values_bucket_utils.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "moving_photo_file_utils.h"
#include "rdb_utils.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
// ---- ConvertToLivePhoto (two overloads) ----
HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_ConvertToLivePhoto_EmptyPath_001, TestSize.Level1)
{
    int64_t coverPosition = 0;
    std::string livePhotoPath;
    int32_t ret = MovingPhotoFileUtils::ConvertToLivePhoto("", coverPosition, livePhotoPath);
    EXPECT_NE(ret, 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_ConvertToLivePhoto_InvalidPath_002, TestSize.Level1)
{
    int64_t coverPosition = 0;
    std::string livePhotoPath;
    int32_t ret = MovingPhotoFileUtils::ConvertToLivePhoto(
        "/non/existent/moving_photo.jpg", "/non/existent/cover.jpg",
        "/non/existent/extra.dat", coverPosition, livePhotoPath);
    EXPECT_NE(ret, 0);
}

// ---- IsLivePhoto (path / fd) ----
HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_IsLivePhoto_EmptyPath_003, TestSize.Level1)
{
    EXPECT_FALSE(MovingPhotoFileUtils::IsLivePhoto(""));
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_IsLivePhoto_InvalidPath_004, TestSize.Level1)
{
    EXPECT_FALSE(MovingPhotoFileUtils::IsLivePhoto("/non/existent/video.mp4"));
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_IsLivePhoto_InvalidFd_005, TestSize.Level1)
{
    EXPECT_FALSE(MovingPhotoFileUtils::IsLivePhoto(-1));
}

// ---- GetLivePhotoSize (fd) ----
HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_GetLivePhotoSize_InvalidFd_006, TestSize.Level1)
{
    int64_t liveSize = 0;
    int32_t ret = MovingPhotoFileUtils::GetLivePhotoSize(-1, liveSize);
    EXPECT_NE(ret, 0);
}

// ---- IsLivePhotoAsset ----
HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_IsLivePhotoAsset_Empty_007, TestSize.Level1)
{
    EXPECT_FALSE(MovingPhotoFileUtils::IsLivePhotoAsset(""));
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_IsLivePhotoAsset_Invalid_008, TestSize.Level1)
{
    EXPECT_FALSE(MovingPhotoFileUtils::IsLivePhotoAsset("/non/existent/asset.jpg"));
}

// ---- CheckMovingPhotoDetailedSize (fd) ----
HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_CheckDetailedSize_InvalidFd_009, TestSize.Level1)
{
    EXPECT_FALSE(MovingPhotoFileUtils::CheckMovingPhotoDetailedSize(-1));
}

// ---- GetCoverPositionFromExtraData (3-arg, public) ----
HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_GetCoverPosFromExtraData_Empty_010, TestSize.Level1)
{
    int64_t coverPosition = 0;
    int32_t ret = MovingPhotoFileUtils::GetCoverPositionFromExtraData("", "", coverPosition);
    EXPECT_NE(ret, 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_GetCoverPosFromExtraData_Invalid_011, TestSize.Level1)
{
    int64_t coverPosition = 0;
    int32_t ret = MovingPhotoFileUtils::GetCoverPositionFromExtraData(
        "/non/existent/video.mp4", "/non/existent/extra.dat", coverPosition);
    EXPECT_NE(ret, 0);
}

// ---- GetLivePhotoCoverPosition (two overloads, public) ----
HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_GetCoverPos_3Arg_Invalid_012, TestSize.Level1)
{
    int64_t coverPosition = 0;
    int32_t ret = MovingPhotoFileUtils::GetLivePhotoCoverPosition(
        "/non/existent/video.mp4", "/non/existent/extra.dat", coverPosition);
    EXPECT_NE(ret, 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_GetCoverPos_3Arg_Invalid_013, TestSize.Level1)
{
    int64_t coverPosition = 0;
    int32_t ret = MovingPhotoFileUtils::GetLivePhotoCoverPosition(
        "/non/existent/video.mp4", "/non/existent/livephoto.jpg", coverPosition);
    EXPECT_NE(ret, 0);
}

// ---- GetMovingPhotoExtraDataDir (path joining) ----
HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_GetExtraDataDir_Empty_014, TestSize.Level1)
{
    EXPECT_TRUE(MovingPhotoFileUtils::GetMovingPhotoExtraDataDir("").empty());
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_GetExtraDataDir_ValidRoot_015, TestSize.Level1)
{
    string dir = MovingPhotoFileUtils::GetMovingPhotoExtraDataDir(
        "/storage/media/local/files/Photos/test.jpg");
    EXPECT_FALSE(dir.empty());
    EXPECT_NE(dir.find("/test.jpg"), string::npos);
}

// ---- GetLivePhotoCacheDir (path joining) ----
HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_GetCacheDir_Empty_016, TestSize.Level1)
{
    EXPECT_TRUE(MovingPhotoFileUtils::GetLivePhotoCacheDir("").empty());
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_GetCacheDir_ValidRoot_017, TestSize.Level1)
{
    string dir = MovingPhotoFileUtils::GetLivePhotoCacheDir(
        "/storage/media/local/files/Photos/test.jpg");
    EXPECT_FALSE(dir.empty());
    EXPECT_NE(dir.find("/test.jpg"), string::npos);
}

// ---- GetExtraDataLen 带 coverPosition：固化合一路径的 extraData 创建与版本标签可读性 ----
HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_GetExtraDataLen_InvalidImage_001, TestSize.Level1)
{
    // 图片路径不可解析，CreateExtraData 在归一化图片路径处直接返回 E_HAS_FS_ERROR
    off_t fileSize = 0;
    int32_t ret = MovingPhotoFileUtils::GetExtraDataLen(
        "/non/existent/image.jpg", "/non/existent/video.mp4", 0, 0, fileSize);
    EXPECT_EQ(ret, E_HAS_FS_ERROR);
    EXPECT_EQ(fileSize, 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_GetExtraDataLen_ImageOutsideMediaRoot_002, TestSize.Level1)
{
    // 视频已存在而图片不在媒体库根目录下时，GetMovingPhotoExtraDataDir 返回空，
    // CreateExtraData 在创建 extraData 文件处返回 E_HAS_FS_ERROR
    const string imagePath = "/data/test/extra_data_image.jpg";
    const string videoPath = "/data/test/extra_data_video.mp4";
    MediaFileUtils::CreateAsset(imagePath);
    MediaFileUtils::CreateAsset(videoPath);
    off_t fileSize = 0;
    int32_t ret = MovingPhotoFileUtils::GetExtraDataLen(imagePath, videoPath, 0, 1000, fileSize);
    EXPECT_EQ(ret, E_HAS_FS_ERROR);
    EXPECT_EQ(fileSize, 0);
    MediaFileUtils::DeleteFile(imagePath);
    MediaFileUtils::DeleteFile(videoPath);
}

// ---- CreateExtraData：version 显式透传时写入源文件版本号，未透传时保持 v3 规则 ----
static bool PrepareExtraDataTestFile(const string &dir, const string &imagePath, const string &videoPath)
{
    if (!MediaFileUtils::CreateDirectory(dir)) {
        return false;
    }
    MediaFileUtils::CreateAsset(imagePath);
    MediaFileUtils::CreateAsset(videoPath);
    return true;
}

static string ReadExtraDataVersionTag(const string &extraPath)
{
    ifstream ifs(extraPath, ios::binary);
    if (!ifs.is_open()) {
        return "";
    }
    string content((istreambuf_iterator<char>(ifs)), istreambuf_iterator<char>());
    ifs.close();
    if (content.size() < static_cast<size_t>(MIN_STANDARD_SIZE)) {
        return "";
    }
    string tag = content.substr(0, VERSION_TAG_LEN);
    tag.erase(tag.find_last_not_of(' ') + 1);
    return tag;
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_CreateExtraData_WithVersion_004, TestSize.Level1)
{
    const string dir = "/storage/media/local/files/single_file_extra_test";
    const string imagePath = dir + "/img.jpg";
    const string videoPath = dir + "/video.mp4";
    ASSERT_TRUE(PrepareExtraDataTestFile(dir, imagePath, videoPath))
        << "Failed to prepare test files under " << dir;
    MovingPhotoExtraDataParams params;
    params.imagePath = imagePath;
    params.videoPath = videoPath;
    params.frameIndex = 12;
    params.version = 9;  // 模拟源合一文件自带的版本号
    off_t fileSize = 0;
    EXPECT_EQ(MovingPhotoFileUtils::CreateExtraData(params, fileSize), E_OK);
    EXPECT_EQ(fileSize, MIN_STANDARD_SIZE);
    string extraPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(imagePath);
    EXPECT_FALSE(extraPath.empty());
    EXPECT_EQ(ReadExtraDataVersionTag(extraPath), "v9_f12");
    MediaFileUtils::DeleteFile(extraPath);
    MediaFileUtils::DeleteFile(imagePath);
    MediaFileUtils::DeleteFile(videoPath);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_CreateExtraData_DefaultVersion_005, TestSize.Level1)
{
    const string dir = "/storage/media/local/files/single_file_extra_test_default";
    const string imagePath = dir + "/img.jpg";
    const string videoPath = dir + "/video.mp4";
    ASSERT_TRUE(PrepareExtraDataTestFile(dir, imagePath, videoPath))
        << "Failed to prepare test files under " << dir;
    MovingPhotoExtraDataParams params;
    params.imagePath = imagePath;
    params.videoPath = videoPath;
    params.frameIndex = 7;
    params.version = 0;  // 未透传版本号时沿用既有规则
    off_t fileSize = 0;
    EXPECT_EQ(MovingPhotoFileUtils::CreateExtraData(params, fileSize), E_OK);
    string extraPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(imagePath);
    EXPECT_FALSE(extraPath.empty());
    EXPECT_EQ(ReadExtraDataVersionTag(extraPath), "v3_f7");
    MediaFileUtils::DeleteFile(extraPath);
    MediaFileUtils::DeleteFile(imagePath);
    MediaFileUtils::DeleteFile(videoPath);
}

// ---- GetWholeExtraData / CheckExtraData：透传源合一文件末尾整段 extraData（40B / 60B / 60B+N）----
static string PadTag(const string &tag, int32_t len = VERSION_TAG_LEN)
{
    string padded = tag;
    if (static_cast<int32_t>(padded.length()) < len) {
        padded.append(len - padded.length(), ' ');
    }
    return padded;
}

// LIVE 段记录的是「视频段 + 20B LIVE 段自身」的十进制长度，与 CreateExtraData 的写法保持一致
static string BuildLiveTag(size_t videoSize)
{
    return PadTag("LIVE_" + to_string(videoSize + LIVE_TAG_LEN));
}

// CinemagraphInfo 段长度字段记录的是「段总长（含 4B 长度字段本身）」，消费侧把 4 个字节各自
// 展开成十六进制后整体解析，因此这里只填最低字节，其余补 0
static string BuildCinemagraphSegment(size_t payloadSize)
{
    const size_t segmentSize = payloadSize + CINEMAGRAPH_INFO_SIZE_LEN;
    string segment(payloadSize, 'c');
    segment.push_back('\0');
    segment.push_back('\0');
    segment.push_back('\0');
    segment.push_back(static_cast<char>(segmentSize));
    return segment;
}

// 按「图片段 + 视频段 + 整段 extraData」构造最小合法合一容器，extraData 的末尾 20B 必须是 LIVE 段
static void WriteSingleFileContainer(const string &path, size_t imageSize, size_t videoSize,
    const string &extraData)
{
    const string image = string(imageSize, 'i');
    const string video = string(videoSize, 'v');
    ofstream ofs(path, ios::binary | ios::trunc);
    ASSERT_TRUE(ofs.is_open());
    ofs.write(image.c_str(), static_cast<streamsize>(image.size()));
    ofs.write(video.c_str(), static_cast<streamsize>(video.size()));
    ofs.write(extraData.c_str(), static_cast<streamsize>(extraData.size()));
    ofs.close();
}

static string ReadWholeFile(const string &path)
{
    ifstream ifs(path, ios::binary);
    if (!ifs.is_open()) {
        return "";
    }
    string content((istreambuf_iterator<char>(ifs)), istreambuf_iterator<char>());
    ifs.close();
    return content;
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_GetWholeExtraData_Standard_006, TestSize.Level1)
{
    // 版本段标准且不带 _c：整段只有 60B，需原样返回
    const string path = "/data/test/whole_extra_standard.jpg";
    MediaFileUtils::CreateAsset(path);
    const size_t videoSize = 128;
    const string extraData = PadTag("v9_f12") + PadTag("0:5") + BuildLiveTag(videoSize);
    WriteSingleFileContainer(path, 64, videoSize, extraData);
    int32_t fd = open(path.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);
    string readExtraData;
    EXPECT_EQ(MovingPhotoFileUtils::GetWholeExtraData(fd, readExtraData), E_OK);
    EXPECT_EQ(readExtraData, extraData);
    close(fd);
    MediaFileUtils::DeleteFile(path);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_GetWholeExtraData_NoVersionTag_007, TestSize.Level1)
{
    // 版本段非标准时整段只剩 40B，不做补位，直接透传剩余内容
    const string path = "/data/test/whole_extra_no_version.jpg";
    MediaFileUtils::CreateAsset(path);
    const size_t videoSize = 128;
    const string extraData = PadTag("0:5") + BuildLiveTag(videoSize);
    WriteSingleFileContainer(path, 64, videoSize, extraData);
    int32_t fd = open(path.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);
    string readExtraData;
    EXPECT_EQ(MovingPhotoFileUtils::GetWholeExtraData(fd, readExtraData), E_OK);
    EXPECT_EQ(readExtraData.size(), static_cast<size_t>(LIVE_TAG_LEN + PLAY_INFO_LEN));
    EXPECT_EQ(readExtraData, extraData);
    close(fd);
    MediaFileUtils::DeleteFile(path);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_GetWholeExtraData_Cinemagraph_008, TestSize.Level1)
{
    // 带 CinemagraphInfo 时整段为 60B+N，CinemagraphInfo 段与 _c 后缀都必须完整保留
    const string path = "/data/test/whole_extra_cinemagraph.jpg";
    MediaFileUtils::CreateAsset(path);
    const size_t videoSize = 128;
    const string extraData = BuildCinemagraphSegment(12) + PadTag("v9_f3_c") + PadTag("0:5") +
        BuildLiveTag(videoSize);
    WriteSingleFileContainer(path, 64, videoSize, extraData);
    int32_t fd = open(path.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);
    string readExtraData;
    EXPECT_EQ(MovingPhotoFileUtils::GetWholeExtraData(fd, readExtraData), E_OK);
    EXPECT_EQ(readExtraData.size(), static_cast<size_t>(MIN_STANDARD_SIZE + 16));
    EXPECT_EQ(readExtraData, extraData);
    close(fd);
    MediaFileUtils::DeleteFile(path);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_GetWholeExtraData_CinemagraphFallback_009, TestSize.Level1)
{
    // 版本段带 _c 但长度字段不可用时整段回退成 60B，必须剔除 _c，否则消费侧会多算 4B 长度字段
    const string path = "/data/test/whole_extra_cinemagraph_fallback.jpg";
    MediaFileUtils::CreateAsset(path);
    const size_t videoSize = 128;
    const string tail = PadTag("0:5") + BuildLiveTag(videoSize);
    WriteSingleFileContainer(path, 64, videoSize, PadTag("v9_f3_c") + tail);
    int32_t fd = open(path.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);
    string readExtraData;
    EXPECT_EQ(MovingPhotoFileUtils::GetWholeExtraData(fd, readExtraData), E_OK);
    EXPECT_EQ(readExtraData, PadTag("v9_f3") + tail);
    close(fd);
    MediaFileUtils::DeleteFile(path);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_GetWholeExtraData_TooSmall_010, TestSize.Level1)
{
    // 不足 60B 的文件不是合一文件；判据是 totalSize > MIN_STANDARD_SIZE，恰好 60B 同样不满足
    const string path = "/data/test/whole_extra_too_small.jpg";
    MediaFileUtils::CreateAsset(path);
    const size_t tooSmallSizes[] = {
        static_cast<size_t>(MIN_STANDARD_SIZE) - 1,
        static_cast<size_t>(MIN_STANDARD_SIZE),
    };
    for (const auto size : tooSmallSizes) {
        {
            ofstream ofs(path, ios::binary | ios::trunc);
            ASSERT_TRUE(ofs.is_open());
            string content(size, 'v');
            ofs.write(content.c_str(), content.length());
        }
        int32_t fd = open(path.c_str(), O_RDONLY);
        ASSERT_GE(fd, 0);
        string extraData;
        EXPECT_EQ(MovingPhotoFileUtils::GetWholeExtraData(fd, extraData), E_INVALID_LIVE_PHOTO);
        EXPECT_TRUE(extraData.empty());
        close(fd);
    }
    MediaFileUtils::DeleteFile(path);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_GetWholeExtraData_InvalidFd_011, TestSize.Level1)
{
    string extraData;
    EXPECT_NE(MovingPhotoFileUtils::GetWholeExtraData(-1, extraData), E_OK);
    EXPECT_TRUE(extraData.empty());
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_CheckExtraData_Standard_012, TestSize.Level1)
{
    // 三种合法形态：40B 无版本段、60B 标准版本段、60B+N 带 CinemagraphInfo
    EXPECT_EQ(MovingPhotoFileUtils::CheckExtraData(PadTag("v3_f7") + PadTag("0:5") + PadTag("LIVE_2048")),
        E_OK);
    EXPECT_EQ(MovingPhotoFileUtils::CheckExtraData(PadTag("0:5") + PadTag("LIVE_2048")), E_OK);
    EXPECT_EQ(MovingPhotoFileUtils::CheckExtraData(BuildCinemagraphSegment(12) + PadTag("v9_f3_c") +
        PadTag("0:5") + PadTag("LIVE_4096")), E_OK);
    // PLAY_INFO 不校验格式，原样透传
    EXPECT_EQ(MovingPhotoFileUtils::CheckExtraData(PadTag("v3_f7") + string(PLAY_INFO_LEN, 'x') +
        PadTag("LIVE_2048")), E_OK);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_CheckExtraData_InvalidSize_013, TestSize.Level1)
{
    // 长度只能是 40B 或 60B 及以上，41~59B 属于畸形输入
    EXPECT_EQ(MovingPhotoFileUtils::CheckExtraData(""), E_INVALID_VALUES);
    EXPECT_EQ(MovingPhotoFileUtils::CheckExtraData(string(LIVE_TAG_LEN + PLAY_INFO_LEN + 1, 'v')),
        E_INVALID_VALUES);
    EXPECT_EQ(MovingPhotoFileUtils::CheckExtraData(string(MIN_STANDARD_SIZE - 1, 'v')), E_INVALID_VALUES);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_CheckExtraData_InvalidLiveTag_014, TestSize.Level1)
{
    // 末尾 20B 缺少 LIVE_ 前缀即非法，40B 形态同样要校验
    EXPECT_EQ(MovingPhotoFileUtils::CheckExtraData(PadTag("v3_f7") + PadTag("0:5") + PadTag("VIDEO_2048")),
        E_INVALID_VALUES);
    EXPECT_EQ(MovingPhotoFileUtils::CheckExtraData(PadTag("0:5") + PadTag("VIDEO_2048")), E_INVALID_VALUES);
    // 视频长度数值不再校验，交由拆分阶段按实际内容判定
    EXPECT_EQ(MovingPhotoFileUtils::CheckExtraData(PadTag("v3_f7") + PadTag("0:5") + PadTag("LIVE_0")), E_OK);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_CheckExtraData_InvalidVersionTag_015, TestSize.Level1)
{
    // 60B 及以上必须能从末尾 60B 起始处解析出版本段
    EXPECT_EQ(MovingPhotoFileUtils::CheckExtraData(string(VERSION_TAG_LEN, 'x') + PadTag("0:5") +
        PadTag("LIVE_2048")), E_INVALID_VALUES);
    EXPECT_EQ(MovingPhotoFileUtils::CheckExtraData(string(VERSION_TAG_LEN, ' ') + PadTag("0:5") +
        PadTag("LIVE_2048")), E_INVALID_VALUES);
    // 带 CinemagraphInfo 时版本段位置不变，同样要能解析
    EXPECT_EQ(MovingPhotoFileUtils::CheckExtraData(BuildCinemagraphSegment(12) + string(VERSION_TAG_LEN, 'x') +
        PadTag("0:5") + PadTag("LIVE_2048")), E_INVALID_VALUES);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_CreateExtraData_WithExtraData_016, TestSize.Level1)
{
    const string dir = "/storage/media/local/files/single_file_extra_metadata";
    const string imagePath = dir + "/img.jpg";
    const string videoPath = dir + "/video.mp4";
    ASSERT_TRUE(PrepareExtraDataTestFile(dir, imagePath, videoPath))
        << "Failed to prepare test files under " << dir;
    // 60B 形态需整段原样落盘，fileSize 为实际写入长度
    const string extraData = PadTag("v9_f4") + PadTag("1:25") + PadTag("LIVE_8192");
    MovingPhotoExtraDataParams params;
    params.imagePath = imagePath;
    params.videoPath = videoPath;
    params.extraData = extraData;
    off_t fileSize = 0;
    EXPECT_EQ(MovingPhotoFileUtils::CreateExtraData(params, fileSize), E_OK);
    EXPECT_EQ(fileSize, static_cast<off_t>(extraData.size()));
    string extraPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(imagePath);
    EXPECT_FALSE(extraPath.empty());
    EXPECT_EQ(ReadWholeFile(extraPath), extraData);
    MediaFileUtils::DeleteFile(extraPath);
    MediaFileUtils::DeleteFile(imagePath);
    MediaFileUtils::DeleteFile(videoPath);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_CreateExtraData_StripCinemagraphSuffix_018, TestSize.Level1)
{
    // 60B 带 _c 说明 CinemagraphInfo 段已丢失，落盘前必须剔除 _c，保证 extraData 自洽
    const string dir = "/storage/media/local/files/single_file_extra_strip";
    const string imagePath = dir + "/img.jpg";
    const string videoPath = dir + "/video.mp4";
    ASSERT_TRUE(PrepareExtraDataTestFile(dir, imagePath, videoPath))
        << "Failed to prepare test files under " << dir;
    const string tail = PadTag("1:25") + PadTag("LIVE_8192");
    MovingPhotoExtraDataParams params;
    params.imagePath = imagePath;
    params.videoPath = videoPath;
    params.extraData = PadTag("v9_f4_c") + tail;
    off_t fileSize = 0;
    EXPECT_EQ(MovingPhotoFileUtils::CreateExtraData(params, fileSize), E_OK);
    EXPECT_EQ(fileSize, MIN_STANDARD_SIZE);
    string extraPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(imagePath);
    EXPECT_EQ(ReadWholeFile(extraPath), PadTag("v9_f4") + tail);
    MediaFileUtils::DeleteFile(extraPath);
    MediaFileUtils::DeleteFile(imagePath);
    MediaFileUtils::DeleteFile(videoPath);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_CreateExtraData_InvalidExtraData_019, TestSize.Level1)
{
    // 透传内容非法时必须在落盘前拒绝，且不能留下半成品 extraData
    const string dir = "/storage/media/local/files/single_file_extra_metadata_invalid";
    const string imagePath = dir + "/img.jpg";
    const string videoPath = dir + "/video.mp4";
    ASSERT_TRUE(PrepareExtraDataTestFile(dir, imagePath, videoPath))
        << "Failed to prepare test files under " << dir;
    MovingPhotoExtraDataParams params;
    params.imagePath = imagePath;
    params.videoPath = videoPath;
    params.extraData = "invalid_extra_data";
    off_t fileSize = 0;
    EXPECT_EQ(MovingPhotoFileUtils::CreateExtraData(params, fileSize), E_INVALID_VALUES);
    string extraPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(imagePath);
    EXPECT_FALSE(MediaFileUtils::IsFileExists(extraPath));
    MediaFileUtils::DeleteFile(extraPath);
    MediaFileUtils::DeleteFile(imagePath);
    MediaFileUtils::DeleteFile(videoPath);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhoto_CreateExtraData_VideoNotExist_021, TestSize.Level1)
{
    // 重拼 extraData 需要视频段大小，视频缺失时必须在落盘前拒绝，避免把非法长度写进 LIVE_ 段
    const string dir = "/storage/media/local/files/single_file_extra_video_missing";
    const string imagePath = dir + "/img.jpg";
    const string videoPath = dir + "/not_exist.mp4";
    ASSERT_TRUE(MediaFileUtils::CreateDirectory(dir)) << "Failed to create dir " << dir;
    MediaFileUtils::CreateAsset(imagePath);
    MediaFileUtils::DeleteFile(videoPath);
    const string extraPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(imagePath);
    ASSERT_FALSE(extraPath.empty());
    MediaFileUtils::DeleteFile(extraPath);  // 清掉历史残留，确保校验的就是「本次没有落盘」
    MovingPhotoExtraDataParams params;
    params.imagePath = imagePath;
    params.videoPath = videoPath;
    off_t fileSize = 0;
    EXPECT_EQ(MovingPhotoFileUtils::CreateExtraData(params, fileSize), E_HAS_FS_ERROR);
    EXPECT_EQ(fileSize, 0);
    EXPECT_FALSE(MediaFileUtils::IsFileExists(extraPath));
    MediaFileUtils::DeleteFile(imagePath);
}
} // namespace Media
} // namespace OHOS

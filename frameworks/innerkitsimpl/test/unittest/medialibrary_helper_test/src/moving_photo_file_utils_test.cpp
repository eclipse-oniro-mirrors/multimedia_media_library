/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "medialibrary_helper_test.h"

#include <fcntl.h>
#include <fstream>

#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "moving_photo_file_utils.h"
#include "medialibrary_unittest_utils.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
static const unsigned char FILE_TEST_JPG[] = {
    0xFF, 0xD8, 0xFF, 0xE0, 0x01, 0x02, 0x03, 0x04, 0xFF, 0xD9
};

static const unsigned char FILE_TEST_MP4[] = {
    0x00, 0x00, 0x00, 0x18, 0x66, 0x74, 0x79, 0x70, 0x69, 0x73, 0x6F, 0x6D, 0x01, 0x02, 0x03, 0x04
};

static const unsigned char FILE_TEST_EXTRA_DATA[] = {
    0x76, 0x33, 0x5f, 0x66, 0x30, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x30, 0x3a,
    0x30, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x4c, 0x49, 0x56, 0x45,
    0x5f, 0x33, 0x36, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20 };

static const unsigned char FILE_TEST_LIVE_PHOTO[] = {
    0xFF, 0xD8, 0xFF, 0xE0, 0x01, 0x02, 0x03, 0x04, 0xFF, 0xD9, 0x00,
    0x00, 0x00, 0x18, 0x66, 0x74, 0x79, 0x70, 0x69, 0x73, 0x6F, 0x6D,
    0x01, 0x02, 0x03, 0x04, 0x76, 0x33, 0x5f, 0x66, 0x30, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x30, 0x3a, 0x30, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x4c, 0x49, 0x56, 0x45, 0x5f, 0x33, 0x36, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20
};

static bool WriteFileContent(const string& path, const unsigned char content[], int32_t size)
{
    int32_t fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
        MEDIA_ERR_LOG("Failed to open %{public}s", path.c_str());
        return false;
    }

    int32_t resWrite = write(fd, content, size);
    if (resWrite == -1) {
        MEDIA_ERR_LOG("Failed to write content");
        close(fd);
        return false;
    }

    close(fd);
    return true;
}

static bool CompareIfContentEquals(const unsigned char originArray[], const string& path, const int32_t size)
{
    int32_t fd = open(path.c_str(), O_RDONLY);
    int32_t len = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    if (len < 0 || len != size) {
        MEDIA_ERR_LOG("Failed to check size: %{public}d %{public}d", size, len);
        return false;
    }
    unsigned char* buf = static_cast<unsigned char*>(malloc(len));
    if (buf == nullptr) {
        return false;
    }
    read(fd, buf, len);
    close(fd);

    for (int i = 0; i < size - 1; i++) {
        if (originArray[i] != buf[i]) {
            free(buf);
            return false;
        }
    }

    free(buf);
    return true;
}

static bool CompareFileLenEquals(const string& path, const int32_t size)
{
    int32_t fd = open(path.c_str(), O_RDONLY);
    int32_t len = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    close(fd);
    return len == size;
}

static bool CompareIfArrayEquals(const unsigned char originArray[], const unsigned char buf[])
{
    int32_t size = sizeof(*buf);
    for (int i = 0; i < size - 1; i++) {
        if (originArray[i] != buf[i]) {
            return false;
        }
    }
    return true;
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_ConvertToMovingPhoto_001, TestSize.Level1)
{
    string dirPath = "/data/test/ConvertToMovingPhoto_001";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string livePhotoPath = dirPath + "/" + "livePhoto.jpg";
    EXPECT_EQ(WriteFileContent(livePhotoPath, FILE_TEST_LIVE_PHOTO, sizeof(FILE_TEST_LIVE_PHOTO)), true);
    string imagePath = dirPath + "/" + "image.jpg";
    string videoPath = dirPath + "/" + "video.mp4";
    string extraDataPath = dirPath + "/" + "extraData";
    EXPECT_EQ(MovingPhotoFileUtils::ConvertToMovingPhoto(livePhotoPath, imagePath, videoPath, extraDataPath), E_OK);
    EXPECT_EQ(CompareIfContentEquals(FILE_TEST_JPG, imagePath, sizeof(FILE_TEST_JPG)), true);
    EXPECT_EQ(CompareIfContentEquals(FILE_TEST_MP4, videoPath, sizeof(FILE_TEST_MP4)), true);
    EXPECT_EQ(CompareIfContentEquals(FILE_TEST_EXTRA_DATA, extraDataPath, sizeof(FILE_TEST_EXTRA_DATA)), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_ConvertToMovingPhoto_002, TestSize.Level1)
{
    string dirPath = "/data/test/ConvertToMovingPhoto_002";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string livePhotoPath = dirPath + "/" + "livePhotoSamePath.jpg";
    EXPECT_EQ(WriteFileContent(livePhotoPath, FILE_TEST_LIVE_PHOTO, sizeof(FILE_TEST_LIVE_PHOTO)), true);
    string imagePath = dirPath + "/" + "livePhotoSamePath.jpg";
    string videoPath = dirPath + "/" + "video.mp4";
    string extraDataPath = dirPath + "/" + "extraData";
    EXPECT_EQ(MovingPhotoFileUtils::ConvertToMovingPhoto(livePhotoPath, imagePath, videoPath, extraDataPath), E_OK);
    EXPECT_EQ(CompareIfContentEquals(FILE_TEST_JPG, imagePath, sizeof(FILE_TEST_JPG)), true);
    EXPECT_EQ(CompareIfContentEquals(FILE_TEST_MP4, videoPath, sizeof(FILE_TEST_MP4)), true);
    EXPECT_EQ(CompareIfContentEquals(FILE_TEST_EXTRA_DATA, extraDataPath, sizeof(FILE_TEST_EXTRA_DATA)), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetCoverPosition_001, TestSize.Level1)
{
    string dirPath = "/data/test/GetCoverPosition_001";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string videoPath = dirPath + "/" + "video.mp4";
    EXPECT_EQ(WriteFileContent(videoPath, FILE_TEST_MP4, sizeof(FILE_TEST_MP4)), true);
    uint64_t coverPosition;
    EXPECT_LT(MovingPhotoFileUtils::GetCoverPosition(videoPath, 0, coverPosition), E_OK);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetVersionAndFrameNum_001, TestSize.Level1)
{
    string tag = "v3_f31_c";
    uint32_t version;
    uint32_t frameIndex;
    bool hasCinemagraphInfo;
    int32_t ret = MovingPhotoFileUtils::GetVersionAndFrameNum(tag, version, frameIndex, hasCinemagraphInfo);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(version, 3);
    EXPECT_EQ(frameIndex, 31);
    EXPECT_EQ(hasCinemagraphInfo, true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetVersionAndFrameNum_002, TestSize.Level1)
{
    string tag = "v2_f30";
    uint32_t version;
    uint32_t frameIndex;
    bool hasCinemagraphInfo;
    int32_t ret = MovingPhotoFileUtils::GetVersionAndFrameNum(tag, version, frameIndex, hasCinemagraphInfo);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(version, 2);
    EXPECT_EQ(frameIndex, 30);
    EXPECT_EQ(hasCinemagraphInfo, false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetVersionAndFrameNum_003, TestSize.Level1)
{
    string tag = "V2_F29_C";
    uint32_t version;
    uint32_t frameIndex;
    bool hasCinemagraphInfo;
    int32_t ret = MovingPhotoFileUtils::GetVersionAndFrameNum(tag, version, frameIndex, hasCinemagraphInfo);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(version, 2);
    EXPECT_EQ(frameIndex, 29);
    EXPECT_EQ(hasCinemagraphInfo, true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetVersionAndFrameNum_004, TestSize.Level1)
{
    string tag = "V3_F33";
    uint32_t version;
    uint32_t frameIndex;
    bool hasCinemagraphInfo;
    int32_t ret = MovingPhotoFileUtils::GetVersionAndFrameNum(tag, version, frameIndex, hasCinemagraphInfo);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(version, 3);
    EXPECT_EQ(frameIndex, 33);
    EXPECT_EQ(hasCinemagraphInfo, false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetVersionAndFrameNum_005, TestSize.Level1)
{
    string tag = "";
    uint32_t version;
    uint32_t frameIndex;
    bool hasCinemagraphInfo;
    int32_t ret = MovingPhotoFileUtils::GetVersionAndFrameNum(tag, version, frameIndex, hasCinemagraphInfo);
    EXPECT_LT(ret, E_OK);

    tag = "invalid";
    ret = MovingPhotoFileUtils::GetVersionAndFrameNum(tag, version, frameIndex, hasCinemagraphInfo);
    EXPECT_LT(ret, E_OK);

    tag = "31";
    ret = MovingPhotoFileUtils::GetVersionAndFrameNum(tag, version, frameIndex, hasCinemagraphInfo);
    EXPECT_LT(ret, E_OK);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetVersionAndFrameNum_006, TestSize.Level1)
{
    string dirPath = "/data/test/GetVersionAndFrameNum_006";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string livePhotoPath = dirPath + "/" + "livePhoto.jpg";
    EXPECT_EQ(WriteFileContent(livePhotoPath, FILE_TEST_LIVE_PHOTO, sizeof(FILE_TEST_LIVE_PHOTO)), true);

    int32_t fd = open(livePhotoPath.c_str(), O_RDONLY);
    uint32_t version;
    uint32_t frameIndex;
    bool hasCinemagraphInfo;
    int32_t ret = MovingPhotoFileUtils::GetVersionAndFrameNum(fd, version, frameIndex, hasCinemagraphInfo);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(version, 3);
    EXPECT_EQ(frameIndex, 0);
    EXPECT_EQ(hasCinemagraphInfo, false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetVersionAndFrameNum_007, TestSize.Level1)
{
    string dirPath = "/data/test/GetVersionAndFrameNum_007";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string livePhotoPath = dirPath + "/" + "extraData";
    EXPECT_EQ(WriteFileContent(livePhotoPath, FILE_TEST_EXTRA_DATA, sizeof(FILE_TEST_EXTRA_DATA)), true);

    int32_t fd = open(livePhotoPath.c_str(), O_RDONLY);
    uint32_t version;
    uint32_t frameIndex;
    bool hasCinemagraphInfo;
    int32_t ret = MovingPhotoFileUtils::GetVersionAndFrameNum(fd, version, frameIndex, hasCinemagraphInfo);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(version, 3);
    EXPECT_EQ(frameIndex, 0);
    EXPECT_EQ(hasCinemagraphInfo, false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetMovingPhotoVideoPath_001, TestSize.Level1)
{
    string imagePath = "/storage/cloud/files/Photo/1/IMG_123435213_231.jpg";
    string videoPath = "/storage/cloud/files/Photo/1/IMG_123435213_231.mp4";
    EXPECT_EQ(MovingPhotoFileUtils::GetMovingPhotoVideoPath(imagePath), videoPath);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetMovingPhotoVideoPath_002, TestSize.Level1)
{
    string imagePath = "/storage/cloud/files/.hiddenTest/IMG_123435213_231";
    EXPECT_EQ(MovingPhotoFileUtils::GetMovingPhotoVideoPath(imagePath), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetMovingPhotoExtraDataDir_001, TestSize.Level1)
{
    string imagePath = "/storage/cloud/files/Photo/1/IMG_123435213_231.jpg";
    string extraDataDir = "/storage/cloud/files/.editData/Photo/1/IMG_123435213_231.jpg";
    EXPECT_EQ(MovingPhotoFileUtils::GetMovingPhotoExtraDataDir(imagePath), extraDataDir);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetMovingPhotoExtraDataPath_001, TestSize.Level1)
{
    string imagePath = "/storage/cloud/files/Photo/1/IMG_123435213_231.jpg";
    string extraDataPath = "/storage/cloud/files/.editData/Photo/1/IMG_123435213_231.jpg/extraData";
    EXPECT_EQ(MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(imagePath), extraDataPath);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetLivePhotoCacheDir_001, TestSize.Level1)
{
    string imagePath = "/storage/cloud/files/Photo/1/IMG_123435213_231.jpg";
    string extraDataDir = "/storage/cloud/files/.cache/Photo/1/IMG_123435213_231.jpg";
    EXPECT_EQ(MovingPhotoFileUtils::GetLivePhotoCacheDir(imagePath), extraDataDir);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetLivePhotoCachePath_001, TestSize.Level1)
{
    string imagePath = "/storage/cloud/files/Photo/1/IMG_123435213_231.jpg";
    string extraDataPath = "/storage/cloud/files/.cache/Photo/1/IMG_123435213_231.jpg/livePhoto.jpg";
    EXPECT_EQ(MovingPhotoFileUtils::GetLivePhotoCachePath(imagePath), extraDataPath);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetExtraDataLen_001, TestSize.Level1)
{
    string dirPath = "/storage/cloud/files/Photo/1";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string imagePath = dirPath + "/" + "livePhotoSamePath.jpg";
    EXPECT_EQ(WriteFileContent(imagePath, FILE_TEST_JPG, sizeof(FILE_TEST_JPG)), true);
    string videoPath = dirPath + "/" + "video.mp4";
    EXPECT_EQ(WriteFileContent(videoPath, FILE_TEST_MP4, sizeof(FILE_TEST_MP4)), true);
    off_t fileSize{0};
    EXPECT_EQ(MovingPhotoFileUtils::GetExtraDataLen(imagePath, videoPath, 0, 0, fileSize), E_OK);
    EXPECT_EQ(fileSize, MIN_STANDARD_SIZE);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetExtraDataLen_002, TestSize.Level1)
{
    string dirPath = "/storage/cloud/files/Photo/1";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string imagePath = dirPath + "/" + "livePhotoSamePath.jpg";
    EXPECT_EQ(WriteFileContent(imagePath, FILE_TEST_JPG, sizeof(FILE_TEST_JPG)), true);
    string videoPath = dirPath + "/" + "video.mp4";
    EXPECT_EQ(WriteFileContent(videoPath, FILE_TEST_MP4, sizeof(FILE_TEST_MP4)), true);

    string extraDir = MovingPhotoFileUtils::GetMovingPhotoExtraDataDir(imagePath);
    EXPECT_EQ(MediaFileUtils::CreateDirectory(extraDir), true);
    string extraPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(imagePath);
    MediaFileUtils::DeleteFile(extraPath);
    EXPECT_EQ(MediaFileUtils::CreateAsset(extraPath), E_SUCCESS);
    EXPECT_EQ(WriteFileContent(extraPath, FILE_TEST_EXTRA_DATA, sizeof(FILE_TEST_EXTRA_DATA)), true);

    off_t fileSize{0};
    EXPECT_EQ(MovingPhotoFileUtils::GetExtraDataLen(imagePath, videoPath, 0, 0, fileSize), E_OK);
    EXPECT_EQ(fileSize, sizeof(FILE_TEST_EXTRA_DATA));
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetFrameIndex_001, TestSize.Level1)
{
    string dirPath = "/storage/cloud/files/Photo/1";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string videoPath = dirPath + "/" + "video.mp4";
    EXPECT_EQ(WriteFileContent(videoPath, FILE_TEST_MP4, sizeof(FILE_TEST_MP4)), true);
    int32_t fd = open(videoPath.c_str(), O_RDONLY);
    EXPECT_GT(fd, 0);
    EXPECT_EQ(MovingPhotoFileUtils::GetFrameIndex(0, fd), 0);
    close(fd);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_IsLivePhoto_001, TestSize.Level1)
{
    string dirPath = "/storage/cloud/files/Photo/15";
    string livePhotDir = MovingPhotoFileUtils::GetLivePhotoCacheDir(dirPath);
    EXPECT_EQ(MediaFileUtils::CreateDirectory(livePhotDir), true);
    string livePhotoPath = livePhotDir + "/" + "livePhoto.jpg";
    EXPECT_EQ(MediaFileUtils::CreateAsset(livePhotoPath), E_SUCCESS);
    EXPECT_EQ(WriteFileContent(livePhotoPath, FILE_TEST_LIVE_PHOTO, sizeof(FILE_TEST_LIVE_PHOTO)), true);
    EXPECT_EQ(MovingPhotoFileUtils::IsLivePhoto(livePhotoPath), true);
    EXPECT_EQ(MediaFileUtils::DeleteFile(livePhotoPath), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_ConvertToSourceLivePhoto_001, TestSize.Level1)
{
    string movingPhotoImagepath = "/storage/cloud/files/Photo/10/IMG_123435213_987.jpg";
    string sourceLivePhotoPath;
    EXPECT_LT(MovingPhotoFileUtils::ConvertToSourceLivePhoto(movingPhotoImagepath, sourceLivePhotoPath), E_OK);
    EXPECT_EQ(sourceLivePhotoPath, "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_ConvertToSourceLivePhoto_002, TestSize.Level1)
{
    string movingPhotoImagepath = "/storage/cloud/files/Photo/50/IMG_123435213_1023.jpg";
    string sourceLivePhotoPath;
    string result = "/storage/cloud/files/.cache/Photo/50/IMG_123435213_1023.jpg/sourceLivePhoto.jpg";
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/.cache/Photo/50/IMG_123435213_1023.jpg"), true);
    EXPECT_EQ(MediaFileUtils::CreateAsset(result), E_SUCCESS);
    EXPECT_EQ(MovingPhotoFileUtils::ConvertToSourceLivePhoto(movingPhotoImagepath, sourceLivePhotoPath), E_OK);
    EXPECT_EQ(sourceLivePhotoPath, result);
    EXPECT_EQ(MediaFileUtils::DeleteFile(result), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetSourceMovingPhotoImagePath_001, TestSize.Level1)
{
    string movingPhotoImagepath = "/storage/cloud/files/Photo/1/IMG_123435213_231.jpg";
    EXPECT_EQ(MovingPhotoFileUtils::GetSourceMovingPhotoImagePath(movingPhotoImagepath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_231.jpg/source.jpg");
    EXPECT_EQ(MovingPhotoFileUtils::GetSourceMovingPhotoImagePath(movingPhotoImagepath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_231.jpg/source.jpg");
    EXPECT_EQ(MovingPhotoFileUtils::GetSourceMovingPhotoImagePath(movingPhotoImagepath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_231.jpg/source.jpg");
    EXPECT_EQ(MovingPhotoFileUtils::GetSourceMovingPhotoImagePath(movingPhotoImagepath, 102),
        "/storage/cloud/102/files/.editData/Photo/1/IMG_123435213_231.jpg/source.jpg");
    EXPECT_EQ(MovingPhotoFileUtils::GetSourceMovingPhotoImagePath("/storage/cloud/data/invalid.jpg"), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetSourceMovingPhotoVideoPath_001, TestSize.Level1)
{
    string movingPhotoImagepath = "/storage/cloud/files/Photo/1/IMG_123435213_231.jpg";
    EXPECT_EQ(MovingPhotoFileUtils::GetSourceMovingPhotoVideoPath(movingPhotoImagepath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_231.jpg/source.mp4");
    EXPECT_EQ(MovingPhotoFileUtils::GetSourceMovingPhotoVideoPath(movingPhotoImagepath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_231.jpg/source.mp4");
    EXPECT_EQ(MovingPhotoFileUtils::GetSourceMovingPhotoVideoPath(movingPhotoImagepath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_231.jpg/source.mp4");
    EXPECT_EQ(MovingPhotoFileUtils::GetSourceMovingPhotoVideoPath(movingPhotoImagepath, 102),
        "/storage/cloud/102/files/.editData/Photo/1/IMG_123435213_231.jpg/source.mp4");
    EXPECT_EQ(MovingPhotoFileUtils::GetSourceMovingPhotoVideoPath("/storage/cloud/test/invalid.jpg"), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetSourceLivePhotoCachePath_001, TestSize.Level1)
{
    string movingPhotoImagepath = "/storage/cloud/files/Photo/1/IMG_123435213_231.jpg";
    EXPECT_EQ(MovingPhotoFileUtils::GetSourceLivePhotoCachePath(movingPhotoImagepath),
        "/storage/cloud/files/.cache/Photo/1/IMG_123435213_231.jpg/sourceLivePhoto.jpg");
    EXPECT_EQ(MovingPhotoFileUtils::GetSourceLivePhotoCachePath(movingPhotoImagepath, -1),
        "/storage/cloud/files/.cache/Photo/1/IMG_123435213_231.jpg/sourceLivePhoto.jpg");
    EXPECT_EQ(MovingPhotoFileUtils::GetSourceLivePhotoCachePath(movingPhotoImagepath, 100),
        "/storage/cloud/100/files/.cache/Photo/1/IMG_123435213_231.jpg/sourceLivePhoto.jpg");
    EXPECT_EQ(MovingPhotoFileUtils::GetSourceLivePhotoCachePath(movingPhotoImagepath, 102),
        "/storage/cloud/102/files/.cache/Photo/1/IMG_123435213_231.jpg/sourceLivePhoto.jpg");
    EXPECT_EQ(MovingPhotoFileUtils::GetSourceLivePhotoCachePath("/storage/cloud/data/invalid.jpg"), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_IsMovingPhoto_001, TestSize.Level1)
{
    EXPECT_EQ(MovingPhotoFileUtils::IsMovingPhoto(0, 0, 0), false);
    EXPECT_EQ(MovingPhotoFileUtils::IsMovingPhoto(0, 0, 4), false);
    EXPECT_EQ(MovingPhotoFileUtils::IsMovingPhoto(0, 5, 1), false);
    EXPECT_EQ(MovingPhotoFileUtils::IsMovingPhoto(1, 0, 0), false);
    EXPECT_EQ(MovingPhotoFileUtils::IsMovingPhoto(2, 0, 0), false);
    EXPECT_EQ(MovingPhotoFileUtils::IsMovingPhoto(4, 0, 0), false);
    EXPECT_EQ(MovingPhotoFileUtils::IsMovingPhoto(3, 0, 0), true);
    EXPECT_EQ(MovingPhotoFileUtils::IsMovingPhoto(0, 10, 0), true);
    EXPECT_EQ(MovingPhotoFileUtils::IsMovingPhoto(0, 0, 3), true);
    EXPECT_EQ(MovingPhotoFileUtils::IsMovingPhoto(0, 2, 3), true);
    EXPECT_EQ(MovingPhotoFileUtils::IsMovingPhoto(3, 0, 3), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_IsGraffiti_001, TestSize.Level1)
{
    EXPECT_EQ(MovingPhotoFileUtils::IsGraffiti(0, 0), false);
    EXPECT_EQ(MovingPhotoFileUtils::IsGraffiti(0, 1), false);
    EXPECT_EQ(MovingPhotoFileUtils::IsGraffiti(0, 2), false);
    EXPECT_EQ(MovingPhotoFileUtils::IsGraffiti(3, 3), false);
    EXPECT_EQ(MovingPhotoFileUtils::IsGraffiti(0, 3), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetMovingPhotoSize_001, TestSize.Level1)
{
    size_t result = MovingPhotoFileUtils::GetMovingPhotoSize("");
    EXPECT_EQ(result, 0);
    result = MovingPhotoFileUtils::GetMovingPhotoSize("/storage/cloud/files/Photo/1/IMG_123435123_123.jpg");
    EXPECT_EQ(result, 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetExtraDataLen_003, TestSize.Level1)
{
    string dirPath = "/storage/cloud/files/Photo/1";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string imagePath = dirPath + "/" + "livePhotoSamePath.jpg";
    string videoPath = dirPath + "/" + "video.mp4";
    off_t fileSize{0};
    EXPECT_EQ(MovingPhotoFileUtils::GetExtraDataLen(imagePath, videoPath, 0, 0, fileSize), E_OK);
    EXPECT_EQ(WriteFileContent(imagePath, FILE_TEST_JPG, sizeof(FILE_TEST_JPG)), true);
    EXPECT_EQ(MovingPhotoFileUtils::GetExtraDataLen(imagePath, videoPath, 0, 0, fileSize), E_OK);
    EXPECT_EQ(WriteFileContent(videoPath, FILE_TEST_MP4, sizeof(FILE_TEST_MP4)), true);
    EXPECT_EQ(MovingPhotoFileUtils::GetExtraDataLen(imagePath, videoPath, 0, 0, fileSize), E_OK);
    EXPECT_EQ(fileSize, MIN_STANDARD_SIZE);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetExtraDataLen_004, TestSize.Level1)
{
    string dirPath = "/storage/cloud/files/Photo/1";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string imagePath = dirPath + "/livePhoto.jpg";
    string videoPath = dirPath + "/video.mp4";
    off_t fileSize = 0;

    EXPECT_EQ(MovingPhotoFileUtils::GetExtraDataLen("/invalid/path", videoPath, 0, 0, fileSize), E_HAS_FS_ERROR);

    string extraDir = MovingPhotoFileUtils::GetMovingPhotoExtraDataDir(imagePath);
    EXPECT_EQ(MediaFileUtils::CreateDirectory(extraDir + "/extra.dat"), true);
    EXPECT_EQ(MovingPhotoFileUtils::GetExtraDataLen(imagePath, videoPath, 0, 0, fileSize), E_HAS_FS_ERROR);

    EXPECT_EQ(chmod(dirPath.c_str(), 0555), 0);
    EXPECT_EQ(MovingPhotoFileUtils::GetExtraDataLen(imagePath, videoPath, 0, 0, fileSize), E_HAS_FS_ERROR);
    EXPECT_EQ(chmod(dirPath.c_str(), 0755), 0);

    string extraPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(imagePath);
    EXPECT_EQ(MediaFileUtils::CreateAsset(extraPath), 0);
    EXPECT_EQ(chmod(extraPath.c_str(), 0444), 0);
    EXPECT_EQ(MovingPhotoFileUtils::GetExtraDataLen(imagePath, videoPath, 0, 0, fileSize), E_HAS_FS_ERROR);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetFrameIndex_002, TestSize.Level1)
{
    string dirPath = "/storage/cloud/files/Photo/1";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string videoPath = dirPath + "/" + "video.mp4";
    EXPECT_EQ(WriteFileContent(videoPath, FILE_TEST_MP4, sizeof(FILE_TEST_MP4)), true);
    int32_t fd = open(videoPath.c_str(), O_RDONLY);
    EXPECT_GT(fd, 0);
    EXPECT_EQ(MovingPhotoFileUtils::GetFrameIndex(100, fd), 0);
    EXPECT_EQ(MovingPhotoFileUtils::GetFrameIndex(100, 0), 0);
    EXPECT_EQ(MovingPhotoFileUtils::GetFrameIndex(9223372036854775807, 0), 0);
    EXPECT_EQ(MovingPhotoFileUtils::GetFrameIndex(9223372036854775807, fd), 0);
    close(fd);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetFrameIndex_003, TestSize.Level1)
{
    string dirPath = "";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string videoPath = "";
    EXPECT_EQ(WriteFileContent(videoPath, FILE_TEST_MP4, sizeof(FILE_TEST_MP4)), false);
    int32_t fd = open(videoPath.c_str(), O_RDONLY);
    EXPECT_EQ(MovingPhotoFileUtils::GetFrameIndex(100, fd), 0);
    close(fd);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_ConvertToSourceLivePhoto_003, TestSize.Level1)
{
    string movingPhotoImagepath = "/123/456/789/IMG_123435213_987.jpg";
    string sourceLivePhotoPath;
    EXPECT_LT(MovingPhotoFileUtils::ConvertToSourceLivePhoto(movingPhotoImagepath, sourceLivePhotoPath), E_OK);
    EXPECT_EQ(sourceLivePhotoPath, "");

    movingPhotoImagepath = "";
    EXPECT_LT(MovingPhotoFileUtils::ConvertToSourceLivePhoto(movingPhotoImagepath, sourceLivePhotoPath), E_OK);
    EXPECT_EQ(sourceLivePhotoPath, "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_ConvertToMovingPhoto_003, TestSize.Level1)
{
    string dirPath = "/data/test/ConvertToMovingPhoto_002";
    string livePhotoPath = dirPath + "/" + "livePhotoSamePath.jpg";
    EXPECT_EQ(WriteFileContent(livePhotoPath, FILE_TEST_LIVE_PHOTO, sizeof(FILE_TEST_LIVE_PHOTO)), true);
    string imagePath = dirPath + "/" + "livePhotoSamePath.jpg";
    string videoPath = dirPath + "/" + "video.mp4";
    string extraDataPath = dirPath + "/" + "extraData";
    EXPECT_EQ(MovingPhotoFileUtils::ConvertToMovingPhoto(livePhotoPath, imagePath, videoPath, extraDataPath),
        E_OK);

    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    EXPECT_EQ(MovingPhotoFileUtils::ConvertToMovingPhoto("", imagePath, videoPath, extraDataPath), E_HAS_FS_ERROR);

    EXPECT_EQ(MovingPhotoFileUtils::ConvertToMovingPhoto(livePhotoPath, "", videoPath, extraDataPath),
        E_INVALID_LIVE_PHOTO);
    EXPECT_EQ(MovingPhotoFileUtils::ConvertToMovingPhoto(livePhotoPath, imagePath, "", extraDataPath),
        E_INVALID_LIVE_PHOTO);
    EXPECT_EQ(MovingPhotoFileUtils::ConvertToMovingPhoto(livePhotoPath, imagePath, videoPath, ""),
        E_INVALID_LIVE_PHOTO);

    EXPECT_EQ(MovingPhotoFileUtils::ConvertToMovingPhoto(livePhotoPath, imagePath, videoPath, extraDataPath),
        E_INVALID_LIVE_PHOTO);
    EXPECT_EQ(CompareIfContentEquals(FILE_TEST_JPG, imagePath, sizeof(FILE_TEST_JPG)), true);
    EXPECT_EQ(CompareIfContentEquals(FILE_TEST_MP4, videoPath, sizeof(FILE_TEST_MP4)), true);
    EXPECT_EQ(CompareIfContentEquals(FILE_TEST_EXTRA_DATA, extraDataPath, sizeof(FILE_TEST_EXTRA_DATA)), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetCoverPosition_002, TestSize.Level1)
{
    std::string videoPath = "";
    uint32_t frameIndex = 0;
    uint64_t coverPosition = 0;
    int32_t scene = 0;
    EXPECT_EQ(MovingPhotoFileUtils::GetCoverPosition(videoPath, frameIndex, coverPosition, scene), E_HAS_FS_ERROR);
    videoPath = "/data/test/ConvertToMovingPhoto_002/video.mp4";
    EXPECT_EQ(MovingPhotoFileUtils::GetCoverPosition(videoPath, frameIndex, coverPosition, scene), E_AVMETADATA);
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/data/test/ConvertToMovingPhoto_002/"), true);
    EXPECT_EQ(MediaFileUtils::CreateAsset(videoPath), E_FILE_EXIST);
    EXPECT_EQ(MovingPhotoFileUtils::GetCoverPosition(videoPath, frameIndex, coverPosition, scene), E_AVMETADATA);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetVersionAndFrameNum_008, TestSize.Level1)
{
    string tag = "";
    uint32_t version = 0;
    uint32_t frameIndex = 0;
    bool hasCinemagraphInfo;
    EXPECT_EQ(MovingPhotoFileUtils::GetVersionAndFrameNum(tag,
        version, frameIndex, hasCinemagraphInfo), E_INVALID_VALUES);
    tag = "v10_f2";
    EXPECT_EQ(MovingPhotoFileUtils::GetVersionAndFrameNum(tag,
        version, frameIndex, hasCinemagraphInfo), E_OK);
    tag = "v10_f2_c ";
    EXPECT_EQ(MovingPhotoFileUtils::GetVersionAndFrameNum(tag,
        version, frameIndex, hasCinemagraphInfo), E_OK);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetVersionAndFrameNum_009, TestSize.Level1)
{
    string testFile = "/data/test/version_tag.dat";
    unsigned char tagData[10] = "v1_f2_c";
    uint32_t version = 0;
    uint32_t frameIndex = 0;
    bool hasCinemagraphInfo;
    EXPECT_EQ(WriteFileContent(testFile, tagData, sizeof(tagData)), true);

    int32_t fd = -1;
    EXPECT_EQ(MovingPhotoFileUtils::GetVersionAndFrameNum(fd,
        version, frameIndex, hasCinemagraphInfo), E_HAS_FS_ERROR);

    fd = open(testFile.c_str(), O_RDONLY);
    EXPECT_GT(fd, 0);  // 确保文件打开成功

    EXPECT_EQ(MovingPhotoFileUtils::GetVersionAndFrameNum(fd,
        version, frameIndex, hasCinemagraphInfo), E_INVALID_LIVE_PHOTO);
    EXPECT_FALSE(hasCinemagraphInfo);

    close(fd);
    EXPECT_EQ(MediaFileUtils::DeleteFile(testFile), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetSourceLivePhotoCachePath_002, TestSize.Level1)
{
    string imagePath = "";
    int32_t userId = 1;
    EXPECT_EQ(MovingPhotoFileUtils::GetSourceLivePhotoCachePath(imagePath, userId), "");
    imagePath = "/storage/cloud/files/Photo/1/IMG_123435213_231.jpg";
    string extraDataPath = "/storage/cloud/1/files/.cache/Photo/1/IMG_123435213_231.jpg/sourceLivePhoto.jpg";
    EXPECT_EQ(MovingPhotoFileUtils::GetSourceLivePhotoCachePath(imagePath, userId), extraDataPath);
}

HWTEST_F(MediaLibraryHelperUnitTest, GetMovingPhotoDetailedSize_001, TestSize.Level1)
{
    string dirPath = "/data/test/GetMovingPhotoDetailedSize_001";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string livePhotoPath = dirPath + "/" + "livePhoto.jpg";
    EXPECT_EQ(WriteFileContent(livePhotoPath, FILE_TEST_LIVE_PHOTO, sizeof(FILE_TEST_LIVE_PHOTO)), true);
    int64_t imageSize = 0;
    int64_t videoSize = 0;
    int64_t extraDataSize = 0;
    int32_t fd = open(livePhotoPath.c_str(), O_RDONLY);
    int32_t ret = MovingPhotoFileUtils::GetMovingPhotoDetailedSize(fd, imageSize, videoSize, extraDataSize);
    EXPECT_EQ(ret, E_OK);
    close(fd);
    EXPECT_EQ(imageSize, sizeof(FILE_TEST_JPG));
    EXPECT_EQ(videoSize, sizeof(FILE_TEST_MP4));
    EXPECT_EQ(extraDataSize, sizeof(FILE_TEST_EXTRA_DATA));
    EXPECT_EQ(MediaFileUtils::DeleteDir(dirPath), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, ConvertToMovingPhoto_004, TestSize.Level1)
{
    string dirPath = "/data/test/ConvertToMovingPhoto_004";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string livePhotoPath = dirPath + "/" + "livePhoto.jpg";
    EXPECT_EQ(WriteFileContent(livePhotoPath, FILE_TEST_LIVE_PHOTO, sizeof(FILE_TEST_LIVE_PHOTO)), true);
    string imagePath = dirPath + "/" + "image.jpg";
    string videoPath = dirPath + "/" + "video.mp4";
    string extraDataPath = dirPath + "/" + "extraData";
    int32_t fd = open(livePhotoPath.c_str(), O_RDONLY);
    int32_t ret = MovingPhotoFileUtils::ConvertToMovingPhoto(fd, imagePath, videoPath, extraDataPath);
    EXPECT_EQ(ret, E_OK);
    close(fd);
    EXPECT_EQ(CompareFileLenEquals(imagePath, sizeof(FILE_TEST_JPG)), true);
    EXPECT_EQ(CompareFileLenEquals(videoPath, sizeof(FILE_TEST_MP4)), true);
    EXPECT_EQ(CompareFileLenEquals(extraDataPath, sizeof(FILE_TEST_EXTRA_DATA)), true);
    EXPECT_EQ(MediaFileUtils::DeleteDir(dirPath), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, ConvertToMovingPhoto_005, TestSize.Level1)
{
    string dirPath = "/data/test/ConvertToMovingPhoto_005";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string livePhotoPath = dirPath + "/" + "livePhoto.jpg";
    EXPECT_EQ(WriteFileContent(livePhotoPath, FILE_TEST_LIVE_PHOTO, sizeof(FILE_TEST_LIVE_PHOTO)), true);
    unsigned char imageArray[100];
    unsigned char videoArray[100];
    unsigned char extraDataArray[100];
    int32_t fd = open(livePhotoPath.c_str(), O_RDONLY);
    int32_t ret = MovingPhotoFileUtils::ConvertToMovingPhoto(fd, imageArray, videoArray, extraDataArray);
    EXPECT_EQ(ret, E_OK);
    close(fd);
    EXPECT_EQ(CompareIfArrayEquals(imageArray, FILE_TEST_JPG), true);
    EXPECT_EQ(CompareIfArrayEquals(videoArray, FILE_TEST_MP4), true);
    EXPECT_EQ(CompareIfArrayEquals(extraDataArray, FILE_TEST_EXTRA_DATA), true);
    EXPECT_EQ(MediaFileUtils::DeleteDir(dirPath), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetExtraDataVersion_Test_001, TestSize.Level1)
{
    // 用例说明：测试<获取额外数据版本功能>；覆盖<文件不存在场景>（触发条件：额外数据文件不存在）；验证<返回默认版本值>
    std::string filePath = "/storage/cloud/files/Photo/2/IMG_20260309_1830.jpg";
    uint32_t version = static_cast<uint32_t>(LivePhoto4dStatusType::TYPE_UNIDENTIFIED);
    int32_t ret = MovingPhotoFileUtils::GetExtraDataVersionByFilePath(filePath, version);
    EXPECT_NE(ret, E_OK);
    EXPECT_EQ(version, static_cast<uint32_t>(LivePhoto4dStatusType::TYPE_UNIDENTIFIED));
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetExtraDataVersion_Test_003, TestSize.Level1)
{
    // 用例说明：测试<获取额外数据版本功能>；覆盖<路径为空场景>（触发条件：传入空路径）；验证<返回默认版本值>
    std::string filePath = "";
    uint32_t version = static_cast<uint32_t>(LivePhoto4dStatusType::TYPE_UNIDENTIFIED);
    int32_t ret = MovingPhotoFileUtils::GetExtraDataVersionByFilePath(filePath, version);
    EXPECT_NE(ret, E_OK);
    EXPECT_EQ(version, static_cast<uint32_t>(LivePhoto4dStatusType::TYPE_UNIDENTIFIED));
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetLocalAssetSize_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("MovingPhotoFileUtils_GetLocalAssetSize_01 begin");

    std::string dirPath = "/data/test/GetLocalAssetSize";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    std::string filePath = dirPath + "/" + "photo.jpg";
    EXPECT_EQ(MediaFileUtils::CreateFile(filePath), true);

    int64_t localAssetSize = 100;
    int64_t inputSize = -100;
    MovingPhotoFileUtils::GetLocalAssetSize(static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY),
        filePath, inputSize, localAssetSize);

    // 获取文件的实际大小
    struct stat statInfo {};
    stat(filePath.c_str(), &statInfo);
    int64_t expectedSize = static_cast<int64_t>(statInfo.st_size);

    EXPECT_EQ(localAssetSize, expectedSize);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetLocalAssetSize_02, TestSize.Level1)
{
    MEDIA_INFO_LOG("MovingPhotoFileUtils_GetLocalAssetSize_02 begin");

    int64_t localAssetSize = 100;
    int64_t inputSize = -100;
    MovingPhotoFileUtils::GetLocalAssetSize(static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY),
        "invalid_path.jpg", inputSize, localAssetSize);

    EXPECT_EQ(localAssetSize, 100);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetLocalAssetSize_03, TestSize.Level1)
{
    MEDIA_INFO_LOG("MovingPhotoFileUtils_GetLocalAssetSize_03 begin");

    int64_t localAssetSize = 100;
    int64_t inputSize = -100;
    MovingPhotoFileUtils::GetLocalAssetSize(static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT),
        "invalid_path.jpg", inputSize, localAssetSize);

    EXPECT_EQ(localAssetSize, inputSize);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_ModifyExtraDataVersion_001, TestSize.Level1)
{
    string dirPath = "/data/test/ModifyExtraDataVersion_001";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string extraDataPath = dirPath + "/" + "extraData";
    EXPECT_EQ(WriteFileContent(extraDataPath, FILE_TEST_EXTRA_DATA, sizeof(FILE_TEST_EXTRA_DATA)), true);

    int32_t ret = MovingPhotoFileUtils::ModifyExtraDataVersion(extraDataPath,
        static_cast<uint32_t>(MOVING_PHOTO_VERSION::MOVING_PHOTO_VERSION_9));
    EXPECT_EQ(ret, E_OK);

    uint32_t version = 0;
    ret = MovingPhotoFileUtils::GetExtraDataVersion(extraDataPath, version);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(version, static_cast<uint32_t>(MOVING_PHOTO_VERSION::MOVING_PHOTO_VERSION_9));

    EXPECT_EQ(MediaFileUtils::DeleteFile(extraDataPath), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_ModifyExtraDataVersion_002, TestSize.Level1)
{
    string dirPath = "/data/test/ModifyExtraDataVersion_002";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string extraDataPath = dirPath + "/" + "extraData";
    EXPECT_EQ(WriteFileContent(extraDataPath, FILE_TEST_EXTRA_DATA, sizeof(FILE_TEST_EXTRA_DATA)), true);

    int32_t ret = MovingPhotoFileUtils::ModifyExtraDataVersion(extraDataPath,
        static_cast<uint32_t>(MOVING_PHOTO_VERSION::MOVING_PHOTO_VERSION_8));
    EXPECT_EQ(ret, E_OK);

    uint32_t version = 0;
    ret = MovingPhotoFileUtils::GetExtraDataVersion(extraDataPath, version);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(version, static_cast<uint32_t>(MOVING_PHOTO_VERSION::MOVING_PHOTO_VERSION_8));

    EXPECT_EQ(MediaFileUtils::DeleteFile(extraDataPath), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_ModifyExtraDataVersion_003, TestSize.Level1)
{
    string invalidPath = "";
    int32_t ret = MovingPhotoFileUtils::ModifyExtraDataVersion(invalidPath, 9);
    EXPECT_NE(ret, E_OK);

    ret = MovingPhotoFileUtils::ModifyExtraDataVersion(invalidPath, 8);
    EXPECT_NE(ret, E_OK);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetExtraDataVersion_001, TestSize.Level1)
{
    string dirPath = "/data/test/GetExtraDataVersion_001";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string extraDataPath = dirPath + "/" + "extraData";
    EXPECT_EQ(WriteFileContent(extraDataPath, FILE_TEST_EXTRA_DATA, sizeof(FILE_TEST_EXTRA_DATA)), true);

    uint32_t version = 0;
    int32_t ret = MovingPhotoFileUtils::GetExtraDataVersion(extraDataPath, version);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(version, 3);

    EXPECT_EQ(MediaFileUtils::DeleteFile(extraDataPath), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetExtraDataVersion_002, TestSize.Level1)
{
    string invalidPath = "";
    uint32_t version = 0;
    int32_t ret = MovingPhotoFileUtils::GetExtraDataVersion(invalidPath, version);
    EXPECT_NE(ret, E_OK);
}
} // namespace Media
} // namespace OHOS
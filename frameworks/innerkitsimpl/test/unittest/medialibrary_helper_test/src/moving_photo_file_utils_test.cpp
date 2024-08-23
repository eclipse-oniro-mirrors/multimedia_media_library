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

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_ConvertToMovingPhoto_001, TestSize.Level0)
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

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_ConvertToMovingPhoto_002, TestSize.Level0)
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

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetCoverPosition_001, TestSize.Level0)
{
    string dirPath = "/data/test/GetCoverPosition_001";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string videoPath = dirPath + "/" + "video.mp4";
    EXPECT_EQ(WriteFileContent(videoPath, FILE_TEST_MP4, sizeof(FILE_TEST_MP4)), true);
    uint64_t coverPosition;
    EXPECT_LT(MovingPhotoFileUtils::GetCoverPosition(videoPath, 0, coverPosition), E_OK);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetVersionAndFrameNum_001, TestSize.Level0)
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

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetVersionAndFrameNum_002, TestSize.Level0)
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

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetVersionAndFrameNum_003, TestSize.Level0)
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

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetVersionAndFrameNum_004, TestSize.Level0)
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

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetVersionAndFrameNum_005, TestSize.Level0)
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

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetVersionAndFrameNum_006, TestSize.Level0)
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

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetVersionAndFrameNum_007, TestSize.Level0)
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

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetMovingPhotoVideoPath_001, TestSize.Level0)
{
    string imagePath = "/storage/cloud/files/Photo/1/IMG_123435213_231.jpg";
    string videoPath = "/storage/cloud/files/Photo/1/IMG_123435213_231.mp4";
    EXPECT_EQ(MovingPhotoFileUtils::GetMovingPhotoVideoPath(imagePath), videoPath);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetMovingPhotoVideoPath_002, TestSize.Level0)
{
    string imagePath = "/storage/cloud/files/.hiddenTest/IMG_123435213_231";
    EXPECT_EQ(MovingPhotoFileUtils::GetMovingPhotoVideoPath(imagePath), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetMovingPhotoExtraDataDir_001, TestSize.Level0)
{
    string imagePath = "/storage/cloud/files/Photo/1/IMG_123435213_231.jpg";
    string extraDataDir = "/storage/cloud/files/.editData/Photo/1/IMG_123435213_231.jpg";
    EXPECT_EQ(MovingPhotoFileUtils::GetMovingPhotoExtraDataDir(imagePath), extraDataDir);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetMovingPhotoExtraDataPath_001, TestSize.Level0)
{
    string imagePath = "/storage/cloud/files/Photo/1/IMG_123435213_231.jpg";
    string extraDataPath = "/storage/cloud/files/.editData/Photo/1/IMG_123435213_231.jpg/extraData";
    EXPECT_EQ(MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(imagePath), extraDataPath);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetLivePhotoCacheDir_001, TestSize.Level0)
{
    string imagePath = "/storage/cloud/files/Photo/1/IMG_123435213_231.jpg";
    string extraDataDir = "/storage/cloud/files/.cache/Photo/1/IMG_123435213_231.jpg";
    EXPECT_EQ(MovingPhotoFileUtils::GetLivePhotoCacheDir(imagePath), extraDataDir);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetLivePhotoCachePath_001, TestSize.Level0)
{
    string imagePath = "/storage/cloud/files/Photo/1/IMG_123435213_231.jpg";
    string extraDataPath = "/storage/cloud/files/.cache/Photo/1/IMG_123435213_231.jpg/livePhoto.jpg";
    EXPECT_EQ(MovingPhotoFileUtils::GetLivePhotoCachePath(imagePath), extraDataPath);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetExtraDataLen_001, TestSize.Level0)
{
    string dirPath = "/data/test/GetExtraDataLen_001";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string imagePath = dirPath + "/" + "livePhotoSamePath.jpg";
    EXPECT_EQ(WriteFileContent(imagePath, FILE_TEST_JPG, sizeof(FILE_TEST_JPG)), true);
    string videoPath = dirPath + "/" + "video.mp4";
    EXPECT_EQ(WriteFileContent(videoPath, FILE_TEST_MP4, sizeof(FILE_TEST_MP4)), true);
    off_t fileSize{0};
    EXPECT_EQ(MovingPhotoFileUtils::GetExtraDataLen(imagePath, videoPath, 0, fileSize), E_OK);
    EXPECT_EQ(fileSize, MIN_STANDARD_SIZE);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetExtraDataLen_002, TestSize.Level0)
{
    string dirPath = "/data/test/GetExtraDataLen_002";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string imagePath = dirPath + "/" + "livePhotoSamePath.jpg";
    EXPECT_EQ(WriteFileContent(imagePath, FILE_TEST_JPG, sizeof(FILE_TEST_JPG)), true);
    string videoPath = dirPath + "/" + "video.mp4";
    EXPECT_EQ(WriteFileContent(videoPath, FILE_TEST_MP4, sizeof(FILE_TEST_MP4)), true);

    string extraDir = MovingPhotoFileUtils::GetMovingPhotoExtraDataDir(imagePath);
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string extraPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(imagePath);
    EXPECT_EQ(MediaFileUtils::CreateAsset(extraPath), E_SUCCESS);
    EXPECT_EQ(WriteFileContent(extraPath, FILE_TEST_EXTRA_DATA, sizeof(FILE_TEST_EXTRA_DATA)), true);

    off_t fileSize{0};
    EXPECT_EQ(MovingPhotoFileUtils::GetExtraDataLen(imagePath, videoPath, 0, fileSize), E_OK);
    EXPECT_EQ(fileSize, sizeof(FILE_TEST_EXTRA_DATA));
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_GetFrameIndex_001, TestSize.Level0)
{
    string dirPath = "/data/test/GetFrameIndex_001";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string videoPath = dirPath + "/" + "video.mp4";
    EXPECT_EQ(WriteFileContent(videoPath, FILE_TEST_MP4, sizeof(FILE_TEST_MP4)), true);
    int32_t fd = open(videoPath.c_str(), O_RDONLY);
    EXPECT_GT(fd, 0);
    EXPECT_GT(MovingPhotoFileUtils::GetFrameIndex(0, fd), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MovingPhotoFileUtils_IsLivePhoto_001, TestSize.Level0)
{
    string dirPath = "/data/test/IsLivePhoto_001/iamge.jpg";
    string livePhotDir = MovingPhotoFileUtils::GetLivePhotoCacheDir(dirPath);
    EXPECT_EQ(MediaFileUtils::CreateDirectory(livePhotDir), true);
    string livePhotoPath = livePhotDir + "/" + "livePhoto.jpg";
    EXPECT_EQ(MediaFileUtils::CreateAsset(livePhotoPath), E_SUCCESS);
    EXPECT_EQ(WriteFileContent(livePhotoPath, FILE_TEST_LIVE_PHOTO, sizeof(FILE_TEST_LIVE_PHOTO)), true);
    EXPECT_EQ(MovingPhotoFileUtils::IsLivePhoto(livePhotoPath), true);
}
} // namespace Media
} // namespace OHOS
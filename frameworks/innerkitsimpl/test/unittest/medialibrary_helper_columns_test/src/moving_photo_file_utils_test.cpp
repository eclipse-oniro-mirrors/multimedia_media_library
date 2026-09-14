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

#include "moving_photo_file_utils.h"

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
} // namespace Media
} // namespace OHOS

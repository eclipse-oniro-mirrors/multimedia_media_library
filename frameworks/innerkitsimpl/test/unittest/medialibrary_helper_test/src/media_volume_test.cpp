/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "media_log.h"
#include "medialibrary_errno.h"
#include "userfile_manager_types.h"
#define private public
#include "media_volume.h"
#undef private

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
const int FILE_DATA_SIZE = 4;

HWTEST_F(MediaLibraryHelperUnitTest, MediaVolum_GetAudiosSize_Test_001, TestSize.Level1)
{
    MediaVolume mediaVolume;
    int64_t ret = mediaVolume.GetAudiosSize();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaVolum_GetFilesSize_Test_001, TestSize.Level1)
{
    MediaVolume mediaVolume;
    int64_t ret = mediaVolume.GetFilesSize();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaVolum_GetImagesSize_Test_001, TestSize.Level1)
{
    MediaVolume mediaVolume;
    int64_t ret = mediaVolume.GetImagesSize();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaVolum_GetVideosSize_Test_001, TestSize.Level1)
{
    MediaVolume mediaVolume;
    int64_t ret = mediaVolume.GetVideosSize();
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.number    : MediaVolum_SetSize_Test_001
 * @tc.name      : MediaVolum_SetSize_Test_001
 * @tc.desc      : 1. test SetSize with MEDIA_TYPE_AUDIO
 *                 2. verify mediaVolumeMap_ size
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaVolum_SetSize_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaVolum_SetSize_Test_001 start");
    MediaVolume mediaVolume;
;
    int mediaType = MEDIA_TYPE_AUDIO;
    int size = 0;
    mediaVolume.SetSize(mediaType, size);
    EXPECT_EQ(mediaVolume.mediaVolumeMap_.size(), FILE_DATA_SIZE);
    MEDIA_INFO_LOG("MediaVolum_SetSize_Test_001 end");
}

/**
 * @tc.number    : MediaVolum_SetSize_Test_002
 * @tc.name      : MediaVolum_SetSize_Test_002
 * @tc.desc      : 1. test SetSize with MEDIA_TYPE_IMAGE
 *                 2. verify mediaVolumeMapSize
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaVolum_SetSize_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaVolum_SetSize_Test_002 start");
    MediaVolume mediaVolume;
    int mediaType = MEDIA_TYPE_IMAGE;
    int size = 1024;
    mediaVolume.SetSize(mediaType, size);
    EXPECT_EQ(mediaVolume.mediaVolumeMap_.size(), FILE_DATA_SIZE);
    MEDIA_INFO_LOG("MediaVolum_SetSize_Test_002 end");
}

/**
 * @tc.number    : MediaVolum_SetSize_Test_003
 * @tc.name      : MediaVolum_SetSize_Test_003
 * @tc.desc      : 1. test SetSize with MEDIA_TYPE_VIDEO
 *                 2. verify mediaVolumeMap_ size
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaVolum_SetSize_Test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaVolum_SetSize_Test_003 start");
    MediaVolume mediaVolume;
    int mediaType = MEDIA_TYPE_VIDEO;
    int size = 2048;
    mediaVolume.SetSize(mediaType, size);
    EXPECT_EQ(mediaVolume.mediaVolumeMap_.size(), FILE_DATA_SIZE);
    MEDIA_INFO_LOG("MediaVolum_SetSize_Test_003 end");
}

/**
 * @tc.number    : MediaVolum_SetSize_Test_004
 * @tc.name      : MediaVolum_SetSize_Test_004
 * @tc.desc           : 1. test SetSize with MEDIA_TYPE_FILE
 *                 2. verify mediaVolumeMap_ size
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaVolum_SetSize_Test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaVolum_SetSize_Test_004 start");
    MediaVolume mediaVolume;
    int mediaType = MEDIA_TYPE_FILE;
    int size = 4096;
    mediaVolume.SetSize(mediaType, size);
    EXPECT_EQ(mediaVolume.mediaVolumeMap_.size(), FILE_DATA_SIZE);
    MEDIA_INFO_LOG("MediaVolum_SetSize_Test_004 end");
}

/**
 * @tc.number    : MediaVolum_GetAudiosSize_Test_002
 * @tc.name      : MediaVolum_GetAudiosSize_Test_002
 * @tc.desc      : 1. test SetSize with MEDIA_TYPE_AUDIO
 *                 2. test GetAudiosSize returns correct size
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaVolum_GetAudiosSize_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaVolum_GetAudiosSize_Test_002 start");
    MediaVolume mediaVolume;
    int mediaType = MEDIA_TYPE_AUDIO;
    int size = 512;
    mediaVolume.SetSize(mediaType, size);
    int64_t ret = mediaVolume.GetAudiosSize();
    EXPECT_EQ(ret, size);
    MEDIA_INFO_LOG("MediaVolum_GetAudiosSize_Test_002 end");
}

/**
 * @tc.number    : MediaVolum_GetImagesSize_Test_002
 * @tc.name      : MediaVolum_GetImagesSize_Test_002
 * @tc.desc      : 1. test SetSize with MEDIA_TYPE_IMAGE
 *                 2. test GetImagesSize returns correct size
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaVolum_GetImagesSize_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaVolum_GetImagesSize_Test_002 start");
    MediaVolume mediaVolume;
    int mediaType = MEDIA_TYPE_IMAGE;
    int size = 1024;
    mediaVolume.SetSize(mediaType, size);
    int64_t ret = mediaVolume.GetImagesSize();
    EXPECT_EQ(ret, size);
    MEDIA_INFO_LOG("MediaVolum_GetImagesSize_Test_002 end");
}

/**
 * @tc.number    : MediaVolum_GetVideosSize_Test_002
 * @tc.name      : MediaVolum_GetVideosSize_Test_002
 * @tc.desc      : 1. test SetSize with MEDIA_TYPE_VIDEO
 *                 2. test GetVideosSize returns correct size
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaVolum_GetVideosSize_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaVolum_GetVideosSize_Test_002 start");
    MediaVolume mediaVolume;
    int mediaType = MEDIA_TYPE_VIDEO;
    int size = 2048;
    mediaVolume.SetSize(mediaType, size);
    int64_t ret = mediaVolume.GetVideosSize();
    EXPECT_EQ(ret, size);
    MEDIA_INFO_LOG("MediaVolum_GetVideosSize_Test_002 end");
}

/**
 * @tc.number    : MediaVolum_GetFilesSize_Test_002
 * @tc.name      : MediaVolum_GetFilesSize_Test_002
 * @tc.desc      : 1. test SetSize with MEDIA_TYPE_FILE
 *                 2. test GetFilesSize returns correct size
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaVolum_GetFilesSize_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaVolum_GetFilesSize_Test_002 start");
    MediaVolume mediaVolume;
    int mediaType = MEDIA_TYPE_FILE;
    int size = 4096;
    mediaVolume.SetSize(mediaType, size);
    int64_t ret = mediaVolume.GetFilesSize();
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("MediaVolum_GetFilesSize_Test_002 end");
}
}
}
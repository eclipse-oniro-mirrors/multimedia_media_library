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

#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "photo_file_utils.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataDir_001, TestSize.Level0)
{
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.jpg";
    EXPECT_EQ(PhotoFileUtils::GetEditDataDir(photoPath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataDir(photoPath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataDir(photoPath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_124.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataDir(photoPath, 101),
        "/storage/cloud/101/files/.editData/Photo/1/IMG_123435213_124.jpg");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataDir_002, TestSize.Level0)
{
    string photoPath = "/storage/data/test_invalid.jpg";
    EXPECT_EQ(PhotoFileUtils::GetEditDataDir(photoPath), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataDir(photoPath, -1), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataDir(photoPath, 100), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataDir(photoPath, 101), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataPath_001, TestSize.Level0)
{
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.jpg";
    EXPECT_EQ(PhotoFileUtils::GetEditDataPath(photoPath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/editdata");
    EXPECT_EQ(PhotoFileUtils::GetEditDataPath(photoPath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/editdata");
    EXPECT_EQ(PhotoFileUtils::GetEditDataPath(photoPath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_124.jpg/editdata");
    EXPECT_EQ(PhotoFileUtils::GetEditDataPath(photoPath, 101),
        "/storage/cloud/101/files/.editData/Photo/1/IMG_123435213_124.jpg/editdata");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataPath_002, TestSize.Level0)
{
    string photoPath = "/storage/data/test_invalid.jpg";
    EXPECT_EQ(PhotoFileUtils::GetEditDataPath(photoPath), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataPath(photoPath, -1), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataPath(photoPath, 100), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataPath(photoPath, 101), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataCameraPath_001, TestSize.Level0)
{
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.jpg";
    EXPECT_EQ(PhotoFileUtils::GetEditDataCameraPath(photoPath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/editdata_camera");
    EXPECT_EQ(PhotoFileUtils::GetEditDataCameraPath(photoPath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/editdata_camera");
    EXPECT_EQ(PhotoFileUtils::GetEditDataCameraPath(photoPath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_124.jpg/editdata_camera");
    EXPECT_EQ(PhotoFileUtils::GetEditDataCameraPath(photoPath, 101),
        "/storage/cloud/101/files/.editData/Photo/1/IMG_123435213_124.jpg/editdata_camera");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataCameraPath_002, TestSize.Level0)
{
    string photoPath = "/storage/cloud/data/test_invalid.jpg";
    EXPECT_EQ(PhotoFileUtils::GetEditDataCameraPath(photoPath), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataCameraPath(photoPath, -1), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataCameraPath(photoPath, 100), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataCameraPath(photoPath, 101), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataSourcePath_001, TestSize.Level0)
{
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.jpg";
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourcePath(photoPath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/source.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourcePath(photoPath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/source.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourcePath(photoPath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_124.jpg/source.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourcePath(photoPath, 101),
        "/storage/cloud/101/files/.editData/Photo/1/IMG_123435213_124.jpg/source.jpg");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataSourcePath_002, TestSize.Level0)
{
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.JPG";
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourcePath(photoPath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.JPG/source.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourcePath(photoPath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.JPG/source.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourcePath(photoPath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_124.JPG/source.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourcePath(photoPath, 101),
        "/storage/cloud/101/files/.editData/Photo/1/IMG_123435213_124.JPG/source.jpg");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataSourcePath_003, TestSize.Level0)
{
    string photoPath = "/storage/cloud/invalid/invalid.jpg";
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourcePath(photoPath), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourcePath(photoPath, -1), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourcePath(photoPath, 100), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourcePath(photoPath, 101), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_HasEditData_001, TestSize.Level0)
{
    EXPECT_EQ(PhotoFileUtils::HasEditData(0), false);
    EXPECT_EQ(PhotoFileUtils::HasEditData(-1), false);
    EXPECT_EQ(PhotoFileUtils::HasEditData(1732767140000), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_HasSource_001, TestSize.Level0)
{
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 0), false);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 10), false);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 1), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 5), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(true, 0, 0), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(true, 0, 10), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(true, 0, 2), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 1732767140111, 0), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(true, 1732767140222, 0), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 1732767140333, 2), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_IsThumbnailLatest_001, TestSize.Level0)
{
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/Photo/1/"), true);
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123456789_123.jpg";
    EXPECT_EQ(PhotoFileUtils::IsThumbnailLatest(photoPath), false);

    EXPECT_EQ(MediaFileUtils::CreateFile(photoPath), true);
    EXPECT_EQ(PhotoFileUtils::IsThumbnailLatest(photoPath), false);

    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/.thumbs/Photo/1/IMG_123456789_123.jpg"), true);
    EXPECT_EQ(MediaFileUtils::CreateFile("/storage/cloud/files/.thumbs/Photo/1/IMG_123456789_123.jpg/THM.jpg"), true);
    EXPECT_EQ(PhotoFileUtils::IsThumbnailLatest(photoPath), true);
    EXPECT_EQ(MediaFileUtils::CreateFile("/storage/cloud/files/.thumbs/Photo/1/IMG_123456789_123.jpg/LCD.jpg"), true);
    EXPECT_EQ(PhotoFileUtils::IsThumbnailLatest(photoPath), true);
}
} // namespace Media
} // namespace OHOS
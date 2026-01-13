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
HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataDir_001, TestSize.Level1)
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

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataDir_002, TestSize.Level1)
{
    string photoPath = "/storage/data/test_invalid.jpg";
    EXPECT_EQ(PhotoFileUtils::GetEditDataDir(photoPath), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataDir(photoPath, -1), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataDir(photoPath, 100), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataDir(photoPath, 101), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataPath_001, TestSize.Level1)
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

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataPath_002, TestSize.Level1)
{
    string photoPath = "/storage/data/test_invalid.jpg";
    EXPECT_EQ(PhotoFileUtils::GetEditDataPath(photoPath), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataPath(photoPath, -1), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataPath(photoPath, 100), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataPath(photoPath, 101), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataCameraPath_001, TestSize.Level1)
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

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataCameraPath_002, TestSize.Level1)
{
    string photoPath = "/storage/cloud/data/test_invalid.jpg";
    EXPECT_EQ(PhotoFileUtils::GetEditDataCameraPath(photoPath), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataCameraPath(photoPath, -1), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataCameraPath(photoPath, 100), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataCameraPath(photoPath, 101), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataSourcePath_001, TestSize.Level1)
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

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataSourcePath_002, TestSize.Level1)
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

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataSourcePath_003, TestSize.Level1)
{
    string photoPath = "/storage/cloud/invalid/invalid.jpg";
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourcePath(photoPath), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourcePath(photoPath, -1), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourcePath(photoPath, 100), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourcePath(photoPath, 101), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataSourceBackPath_001, TestSize.Level1)
{
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.jpg";
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourceBackPath(photoPath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/source_back.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourceBackPath(photoPath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/source_back.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourceBackPath(photoPath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_124.jpg/source_back.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourceBackPath(photoPath, 101),
        "/storage/cloud/101/files/.editData/Photo/1/IMG_123435213_124.jpg/source_back.jpg");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataSourceBackPath_002, TestSize.Level1)
{
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.JPG";
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourceBackPath(photoPath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.JPG/source_back.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourceBackPath(photoPath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.JPG/source_back.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourceBackPath(photoPath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_124.JPG/source_back.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourceBackPath(photoPath, 101),
        "/storage/cloud/101/files/.editData/Photo/1/IMG_123435213_124.JPG/source_back.jpg");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataSourceBackPath_003, TestSize.Level1)
{
    string photoPath = "/storage/cloud/invalid/invalid.jpg";
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourceBackPath(photoPath), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourceBackPath(photoPath, -1), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourceBackPath(photoPath, 100), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourceBackPath(photoPath, 101), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataSourceTempPath_001, TestSize.Level1)
{
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.jpg";
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourceTempPath(photoPath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/source_temp.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourceTempPath(photoPath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/source_temp.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourceTempPath(photoPath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_124.jpg/source_temp.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourceTempPath(photoPath, 101),
        "/storage/cloud/101/files/.editData/Photo/1/IMG_123435213_124.jpg/source_temp.jpg");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataSourceTempPath_002, TestSize.Level1)
{
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.JPG";
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourceTempPath(photoPath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.JPG/source_temp.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourceTempPath(photoPath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.JPG/source_temp.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourceTempPath(photoPath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_124.JPG/source_temp.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourceTempPath(photoPath, 101),
        "/storage/cloud/101/files/.editData/Photo/1/IMG_123435213_124.JPG/source_temp.jpg");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataSourceTempPath_003, TestSize.Level1)
{
    string photoPath = "/storage/cloud/invalid/invalid.jpg";
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourceTempPath(photoPath), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourceTempPath(photoPath, -1), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourceTempPath(photoPath, 100), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataSourceTempPath(photoPath, 101), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataTempPath_001, TestSize.Level1)
{
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.jpg";
    EXPECT_EQ(PhotoFileUtils::GetEditDataTempPath(photoPath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/photo_temp.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataTempPath(photoPath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/photo_temp.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataTempPath(photoPath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_124.jpg/photo_temp.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataTempPath(photoPath, 101),
        "/storage/cloud/101/files/.editData/Photo/1/IMG_123435213_124.jpg/photo_temp.jpg");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataTempPath_002, TestSize.Level1)
{
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.JPG";
    EXPECT_EQ(PhotoFileUtils::GetEditDataTempPath(photoPath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.JPG/photo_temp.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataTempPath(photoPath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.JPG/photo_temp.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataTempPath(photoPath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_124.JPG/photo_temp.jpg");
    EXPECT_EQ(PhotoFileUtils::GetEditDataTempPath(photoPath, 101),
        "/storage/cloud/101/files/.editData/Photo/1/IMG_123435213_124.JPG/photo_temp.jpg");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataTempPath_003, TestSize.Level1)
{
    string photoPath = "/storage/cloud/invalid/invalid.jpg";
    EXPECT_EQ(PhotoFileUtils::GetEditDataTempPath(photoPath), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataTempPath(photoPath, -1), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataTempPath(photoPath, 100), "");
    EXPECT_EQ(PhotoFileUtils::GetEditDataTempPath(photoPath, 101), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_IsEditDataSourceBackExists_001, TestSize.Level1)
{
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg"), true);
    EXPECT_EQ(
        MediaFileUtils::CreateFile("/storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/source_back.jpg"),
        true);
    string photoPath = "/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg";

    bool ret = PhotoFileUtils::IsEditDataSourceBackExists(photoPath);
    EXPECT_EQ(ret, true);

    EXPECT_EQ(
        MediaFileUtils::DeleteFile("/storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/source_back.jpg"),
        true);
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_IsEditDataSourceBackExists_002, TestSize.Level1)
{
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg"), true);
    string photoPath = "/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg";

    bool ret = PhotoFileUtils::IsEditDataSourceBackExists(photoPath);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_HasEditData_001, TestSize.Level1)
{
    EXPECT_EQ(PhotoFileUtils::HasEditData(0), false);
    EXPECT_EQ(PhotoFileUtils::HasEditData(-1), false);
    EXPECT_EQ(PhotoFileUtils::HasEditData(1732767140000), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_HasSource_001, TestSize.Level1)
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
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 0, 0), false);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 0, 5), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 10, 0), false);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 10, 5), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_IsThumbnailExists_001, TestSize.Level1)
{
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123456123_001.jpg";
    EXPECT_EQ(PhotoFileUtils::IsThumbnailExists(photoPath), false);

    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/.thumbs/Photo/1/IMG_123456123_001.jpg"), true);
    EXPECT_EQ(MediaFileUtils::CreateFile("/storage/cloud/files/.thumbs/Photo/1/IMG_123456123_001.jpg/LCD.jpg"), true);
    EXPECT_EQ(PhotoFileUtils::IsThumbnailExists(photoPath), true);
    EXPECT_EQ(MediaFileUtils::CreateFile("/storage/cloud/files/.thumbs/Photo/1/IMG_123456123_001.jpg/THM.jpg"), true);
    EXPECT_EQ(PhotoFileUtils::IsThumbnailExists(photoPath), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_IsThumbnailLatest_001, TestSize.Level1)
{
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/Photo/1/"), true);
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123456789_123.jpg";
    EXPECT_EQ(PhotoFileUtils::IsThumbnailLatest(photoPath), false);

    EXPECT_EQ(MediaFileUtils::CreateFile(photoPath), true);
    EXPECT_EQ(PhotoFileUtils::IsThumbnailLatest(photoPath), false);

    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/.thumbs/Photo/1/IMG_123456789_123.jpg"), true);
    EXPECT_EQ(MediaFileUtils::CreateFile("/storage/cloud/files/.thumbs/Photo/1/IMG_123456789_123.jpg/THM.jpg"), true);
    EXPECT_EQ(PhotoFileUtils::IsThumbnailLatest(photoPath), false);
    EXPECT_EQ(MediaFileUtils::CreateFile("/storage/cloud/files/.thumbs/Photo/1/IMG_123456789_123.jpg/LCD.jpg"), true);
    EXPECT_EQ(MediaFileUtils::DeleteDir("/storage/cloud/files/Photo/1/"), true);
    EXPECT_EQ(MediaFileUtils::DeleteDir("/storage/cloud/files/.thumbs/Photo/1/IMG_123456789_123.jpg"), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataPath_003, TestSize.Level1)
{
    string photoPath = "";
    EXPECT_EQ(PhotoFileUtils::GetEditDataPath(photoPath), "");

    photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.jpg";
    EXPECT_EQ(PhotoFileUtils::GetEditDataPath(photoPath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/editdata");
    EXPECT_EQ(PhotoFileUtils::GetEditDataPath(photoPath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/editdata");
    EXPECT_EQ(PhotoFileUtils::GetEditDataPath(photoPath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_124.jpg/editdata");
    EXPECT_EQ(PhotoFileUtils::GetEditDataPath(photoPath, 101),
        "/storage/cloud/101/files/.editData/Photo/1/IMG_123435213_124.jpg/editdata");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetEditDataCameraPath_003, TestSize.Level1)
{
    string photoPath = "";
    EXPECT_EQ(PhotoFileUtils::GetEditDataCameraPath(photoPath), "");

    photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.jpg";
    EXPECT_EQ(PhotoFileUtils::GetEditDataCameraPath(photoPath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/editdata_camera");
    EXPECT_EQ(PhotoFileUtils::GetEditDataCameraPath(photoPath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/editdata_camera");
    EXPECT_EQ(PhotoFileUtils::GetEditDataCameraPath(photoPath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_124.jpg/editdata_camera");
    EXPECT_EQ(PhotoFileUtils::GetEditDataCameraPath(photoPath, 101),
        "/storage/cloud/101/files/.editData/Photo/1/IMG_123435213_124.jpg/editdata_camera");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_HasSource_002, TestSize.Level1)
{
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 0), false);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 10), false);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 1), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 5), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(true, 0, 0), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(true, 0, 10), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(true, 0, 2), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 0), false);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 2), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 1), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 3), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 4), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 5), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 6), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 7), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 8), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 9), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 10), false);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 1, 0), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(true, 1, 0), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 1, 2), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetMetaPathFromOrignalPath_001, TestSize.Level1)
{
    string photoPath = "";
    string metaPath = "";
    EXPECT_EQ(PhotoFileUtils::GetMetaPathFromOrignalPath(photoPath, metaPath), -209);

    photoPath = "/123/456/789";
    EXPECT_EQ(PhotoFileUtils::GetMetaPathFromOrignalPath(photoPath, metaPath), -209);

    photoPath = "/data/media/Recovery/Photo/IMG_123.jpg";
    EXPECT_EQ(PhotoFileUtils::GetMetaPathFromOrignalPath(photoPath, metaPath), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetMetaDataRealPath_001, TestSize.Level1)
{
    string photoPath = "";
    int32_t userId = 0;
    EXPECT_EQ(PhotoFileUtils::GetMetaDataRealPath(photoPath, userId), "");

    photoPath = "/123/456/789";
    EXPECT_EQ(PhotoFileUtils::GetMetaDataRealPath(photoPath, userId), "");

    photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.jpg";
    EXPECT_EQ(PhotoFileUtils::GetMetaDataRealPath(photoPath, userId),
        "/storage/cloud/0/files/.meta/Photo/1/IMG_123435213_124.jpg.json");

    userId = -1;
    EXPECT_EQ(PhotoFileUtils::GetMetaDataRealPath(photoPath, userId),
        "/storage/cloud/files/.meta/Photo/1/IMG_123435213_124.jpg.json");
    userId = 1;
    EXPECT_EQ(PhotoFileUtils::GetMetaDataRealPath(photoPath, userId),
        "/storage/cloud/1/files/.meta/Photo/1/IMG_123435213_124.jpg.json");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_IsThumbnailLatest_002, TestSize.Level1)
{
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/Photo/1/"), true);
    string photoPath = "";
    EXPECT_EQ(PhotoFileUtils::IsThumbnailLatest(photoPath), false);
    photoPath = "/storage/cloud/files/Photo/1/IMG_123456789_123.jpg";
    EXPECT_EQ(PhotoFileUtils::IsThumbnailLatest(photoPath), false);

    EXPECT_EQ(MediaFileUtils::CreateFile(photoPath), true);
    EXPECT_EQ(PhotoFileUtils::IsThumbnailLatest(photoPath), false);

    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/.thumbs/Photo/1/IMG_123456789_123.jpg"), true);
    EXPECT_EQ(MediaFileUtils::CreateFile("/storage/cloud/files/.thumbs/Photo/1/IMG_123456789_123.jpg/THM.jpg"), true);
    EXPECT_EQ(PhotoFileUtils::IsThumbnailLatest(photoPath), false);
    EXPECT_EQ(MediaFileUtils::CreateFile("/storage/cloud/files/.thumbs/Photo/1/IMG_123456789_123.jpg/LCD.jpg"), true);
    EXPECT_EQ(PhotoFileUtils::IsThumbnailLatest(photoPath), false);
    EXPECT_EQ(MediaFileUtils::DeleteDir("/storage/cloud/files/Photo/1/"), true);
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/.thumbs/Photo/1/IMG_123456789_123.jpg"), true);
    EXPECT_EQ(PhotoFileUtils::IsThumbnailLatest(photoPath), false);
    EXPECT_EQ(MediaFileUtils::DeleteDir("/storage/cloud/files/.thumbs/Photo/1/IMG_123456789_123.jpg"), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetThumbDir_001, TestSize.Level1)
{
    std::string photoPath = "/Picture/";
    string res = PhotoFileUtils::GetThumbDir(photoPath);
    EXPECT_EQ(res, "");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetLCDPath_001, TestSize.Level1)
{
    std::string photoPath = "/Picture/";
    int32_t userId = 0;
    string res = PhotoFileUtils::GetLCDPath(photoPath, userId);
    EXPECT_EQ(res, "");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetTHMPath_001, TestSize.Level1)
{
    std::string photoPath = "/Picture/";
    int32_t userId = 0;
    string res = PhotoFileUtils::GetTHMPath(photoPath, userId);
    EXPECT_EQ(res, "");
}
} // namespace Media
} // namespace OHOS
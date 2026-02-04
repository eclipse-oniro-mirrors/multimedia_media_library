/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "media_edit_utils_test.h"

#include "media_edit_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "media_file_utils.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
void MediaEditUtilsUnitTest::SetUpTestCase(void) {}
void MediaEditUtilsUnitTest::TearDownTestCase(void) {}
void MediaEditUtilsUnitTest::SetUp(void) {}
void MediaEditUtilsUnitTest::TearDown(void) {}

HWTEST_F(MediaEditUtilsUnitTest, MediaEditUtils_GetEditDataDir_001, TestSize.Level1)
{
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.jpg";
    EXPECT_EQ(MediaEditUtils::GetEditDataDir(photoPath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataDir(photoPath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataDir(photoPath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_124.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataDir(photoPath, 101),
        "/storage/cloud/101/files/.editData/Photo/1/IMG_123435213_124.jpg");
}

HWTEST_F(MediaEditUtilsUnitTest, MediaEditUtils_GetEditDataDir_002, TestSize.Level1)
{
    string photoPath = "/storage/data/test_invalid.jpg";
    EXPECT_EQ(MediaEditUtils::GetEditDataDir(photoPath), "");
    EXPECT_EQ(MediaEditUtils::GetEditDataDir(photoPath, -1), "");
    EXPECT_EQ(MediaEditUtils::GetEditDataDir(photoPath, 100), "");
    EXPECT_EQ(MediaEditUtils::GetEditDataDir(photoPath, 101), "");
}

HWTEST_F(MediaEditUtilsUnitTest, MediaEditUtils_GetEditDataPath_001, TestSize.Level1)
{
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.jpg";
    EXPECT_EQ(MediaEditUtils::GetEditDataPath(photoPath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/editdata");
    EXPECT_EQ(MediaEditUtils::GetEditDataPath(photoPath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/editdata");
    EXPECT_EQ(MediaEditUtils::GetEditDataPath(photoPath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_124.jpg/editdata");
    EXPECT_EQ(MediaEditUtils::GetEditDataPath(photoPath, 101),
        "/storage/cloud/101/files/.editData/Photo/1/IMG_123435213_124.jpg/editdata");
}

HWTEST_F(MediaEditUtilsUnitTest, MediaEditUtils_GetEditDataPath_002, TestSize.Level1)
{
    string photoPath = "/storage/data/test_invalid.jpg";
    EXPECT_EQ(MediaEditUtils::GetEditDataPath(photoPath), "");
    EXPECT_EQ(MediaEditUtils::GetEditDataPath(photoPath, -1), "");
    EXPECT_EQ(MediaEditUtils::GetEditDataPath(photoPath, 100), "");
    EXPECT_EQ(MediaEditUtils::GetEditDataPath(photoPath, 101), "");
}

HWTEST_F(MediaEditUtilsUnitTest, MediaEditUtils_GetEditDataCameraPath_001, TestSize.Level1)
{
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.jpg";
    EXPECT_EQ(MediaEditUtils::GetEditDataCameraPath(photoPath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/editdata_camera");
    EXPECT_EQ(MediaEditUtils::GetEditDataCameraPath(photoPath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/editdata_camera");
    EXPECT_EQ(MediaEditUtils::GetEditDataCameraPath(photoPath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_124.jpg/editdata_camera");
    EXPECT_EQ(MediaEditUtils::GetEditDataCameraPath(photoPath, 101),
        "/storage/cloud/101/files/.editData/Photo/1/IMG_123435213_124.jpg/editdata_camera");
}

HWTEST_F(MediaEditUtilsUnitTest, MediaEditUtils_GetEditDataCameraPath_002, TestSize.Level1)
{
    string photoPath = "/storage/cloud/data/test_invalid.jpg";
    EXPECT_EQ(MediaEditUtils::GetEditDataCameraPath(photoPath), "");
    EXPECT_EQ(MediaEditUtils::GetEditDataCameraPath(photoPath, -1), "");
    EXPECT_EQ(MediaEditUtils::GetEditDataCameraPath(photoPath, 100), "");
    EXPECT_EQ(MediaEditUtils::GetEditDataCameraPath(photoPath, 101), "");
}

HWTEST_F(MediaEditUtilsUnitTest, MediaEditUtils_GetEditDataSourcePath_001, TestSize.Level1)
{
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.jpg";
    EXPECT_EQ(MediaEditUtils::GetEditDataSourcePath(photoPath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/source.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourcePath(photoPath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/source.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourcePath(photoPath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_124.jpg/source.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourcePath(photoPath, 101),
        "/storage/cloud/101/files/.editData/Photo/1/IMG_123435213_124.jpg/source.jpg");
}

HWTEST_F(MediaEditUtilsUnitTest, MediaEditUtils_GetEditDataSourcePath_002, TestSize.Level1)
{
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.JPG";
    EXPECT_EQ(MediaEditUtils::GetEditDataSourcePath(photoPath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.JPG/source.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourcePath(photoPath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.JPG/source.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourcePath(photoPath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_124.JPG/source.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourcePath(photoPath, 101),
        "/storage/cloud/101/files/.editData/Photo/1/IMG_123435213_124.JPG/source.jpg");
}

HWTEST_F(MediaEditUtilsUnitTest, MediaEditUtils_GetEditDataSourcePath_003, TestSize.Level1)
{
    string photoPath = "/storage/cloud/invalid/invalid.jpg";
    EXPECT_EQ(MediaEditUtils::GetEditDataSourcePath(photoPath), "");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourcePath(photoPath, -1), "");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourcePath(photoPath, 100), "");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourcePath(photoPath, 101), "");
}

HWTEST_F(MediaEditUtilsUnitTest, MediaEditUtils_GetEditDataSourceBackPath_001, TestSize.Level1)
{
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.jpg";
    EXPECT_EQ(MediaEditUtils::GetEditDataSourceBackPath(photoPath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/source_back.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourceBackPath(photoPath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/source_back.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourceBackPath(photoPath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_124.jpg/source_back.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourceBackPath(photoPath, 101),
        "/storage/cloud/101/files/.editData/Photo/1/IMG_123435213_124.jpg/source_back.jpg");
}

HWTEST_F(MediaEditUtilsUnitTest, MediaEditUtils_GetEditDataSourceBackPath_002, TestSize.Level1)
{
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.JPG";
    EXPECT_EQ(MediaEditUtils::GetEditDataSourceBackPath(photoPath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.JPG/source_back.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourceBackPath(photoPath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.JPG/source_back.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourceBackPath(photoPath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_124.JPG/source_back.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourceBackPath(photoPath, 101),
        "/storage/cloud/101/files/.editData/Photo/1/IMG_123435213_124.JPG/source_back.jpg");
}

HWTEST_F(MediaEditUtilsUnitTest, MediaEditUtils_GetEditDataSourceBackPath_003, TestSize.Level1)
{
    string photoPath = "/storage/cloud/invalid/invalid.jpg";
    EXPECT_EQ(MediaEditUtils::GetEditDataSourceBackPath(photoPath), "");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourceBackPath(photoPath, -1), "");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourceBackPath(photoPath, 100), "");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourceBackPath(photoPath, 101), "");
}

HWTEST_F(MediaEditUtilsUnitTest, MediaEditUtils_GetEditDataSourceTempPath_001, TestSize.Level1)
{
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.jpg";
    EXPECT_EQ(MediaEditUtils::GetEditDataSourceTempPath(photoPath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/source_temp.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourceTempPath(photoPath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/source_temp.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourceTempPath(photoPath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_124.jpg/source_temp.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourceTempPath(photoPath, 101),
        "/storage/cloud/101/files/.editData/Photo/1/IMG_123435213_124.jpg/source_temp.jpg");
}

HWTEST_F(MediaEditUtilsUnitTest, MediaEditUtils_GetEditDataSourceTempPath_002, TestSize.Level1)
{
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.JPG";
    EXPECT_EQ(MediaEditUtils::GetEditDataSourceTempPath(photoPath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.JPG/source_temp.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourceTempPath(photoPath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.JPG/source_temp.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourceTempPath(photoPath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_124.JPG/source_temp.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourceTempPath(photoPath, 101),
        "/storage/cloud/101/files/.editData/Photo/1/IMG_123435213_124.JPG/source_temp.jpg");
}

HWTEST_F(MediaEditUtilsUnitTest, MediaEditUtils_GetEditDataSourceTempPath_003, TestSize.Level1)
{
    string photoPath = "/storage/cloud/invalid/invalid.jpg";
    EXPECT_EQ(MediaEditUtils::GetEditDataSourceTempPath(photoPath), "");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourceTempPath(photoPath, -1), "");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourceTempPath(photoPath, 100), "");
    EXPECT_EQ(MediaEditUtils::GetEditDataSourceTempPath(photoPath, 101), "");
}

HWTEST_F(MediaEditUtilsUnitTest, MediaEditUtils_GetEditDataTempPath_001, TestSize.Level1)
{
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.jpg";
    EXPECT_EQ(MediaEditUtils::GetEditDataTempPath(photoPath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/photo_temp.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataTempPath(photoPath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.jpg/photo_temp.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataTempPath(photoPath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_124.jpg/photo_temp.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataTempPath(photoPath, 101),
        "/storage/cloud/101/files/.editData/Photo/1/IMG_123435213_124.jpg/photo_temp.jpg");
}

HWTEST_F(MediaEditUtilsUnitTest, MediaEditUtils_GetEditDataTempPath_002, TestSize.Level1)
{
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.JPG";
    EXPECT_EQ(MediaEditUtils::GetEditDataTempPath(photoPath),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.JPG/photo_temp.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataTempPath(photoPath, -1),
        "/storage/cloud/files/.editData/Photo/1/IMG_123435213_124.JPG/photo_temp.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataTempPath(photoPath, 100),
        "/storage/cloud/100/files/.editData/Photo/1/IMG_123435213_124.JPG/photo_temp.jpg");
    EXPECT_EQ(MediaEditUtils::GetEditDataTempPath(photoPath, 101),
        "/storage/cloud/101/files/.editData/Photo/1/IMG_123435213_124.JPG/photo_temp.jpg");
}

HWTEST_F(MediaEditUtilsUnitTest, MediaEditUtils_GetEditDataTempPath_003, TestSize.Level1)
{
    string photoPath = "/storage/cloud/invalid/invalid.jpg";
    EXPECT_EQ(MediaEditUtils::GetEditDataTempPath(photoPath), "");
    EXPECT_EQ(MediaEditUtils::GetEditDataTempPath(photoPath, -1), "");
    EXPECT_EQ(MediaEditUtils::GetEditDataTempPath(photoPath, 100), "");
    EXPECT_EQ(MediaEditUtils::GetEditDataTempPath(photoPath, 101), "");
}

HWTEST_F(MediaEditUtilsUnitTest, MediaEditUtils_IsEditDataSourceBackExists_001, TestSize.Level1)
{
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg"), true);
    EXPECT_EQ(
        MediaFileUtils::CreateFile("/storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/source_back.jpg"),
        true);
    string photoPath = "/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg";

    bool ret = MediaEditUtils::IsEditDataSourceBackExists(photoPath);
    EXPECT_EQ(ret, true);

    EXPECT_EQ(
        MediaFileUtils::DeleteFile("/storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/source_back.jpg"),
        true);
}

HWTEST_F(MediaEditUtilsUnitTest, MediaEditUtils_IsEditDataSourceBackExists_002, TestSize.Level1)
{
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg"), true);
    string photoPath = "/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg";

    bool ret = MediaEditUtils::IsEditDataSourceBackExists(photoPath);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaEditUtilsUnitTest, MediaEditUtils_HasEditData_001, TestSize.Level1)
{
    EXPECT_EQ(MediaEditUtils::HasEditData(0), false);
    EXPECT_EQ(MediaEditUtils::HasEditData(-1), false);
    EXPECT_EQ(MediaEditUtils::HasEditData(1732767140000), true);
}
} // namespace Media
} // namespace OHOS
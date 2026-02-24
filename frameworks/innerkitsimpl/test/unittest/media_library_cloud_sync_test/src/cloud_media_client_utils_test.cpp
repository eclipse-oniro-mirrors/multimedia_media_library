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

#define MLOG_TAG "CloudMediaClientUtilsTest"

#include "cloud_media_client_utils_test.h"

#include "cloud_mdkrecord_photos_vo.h"
#include "cloud_media_client_utils.h"
#include "medialibrary_errno.h"
#include "media_log.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media::CloudSync;

namespace OHOS {
namespace Media {

void CloudMediaClientUtilsTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("CloudMediaClientUtilsTest SetUpTestCase");
}

void CloudMediaClientUtilsTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("CloudMediaClientUtilsTest TearDownTestCase");
}

void CloudMediaClientUtilsTest::SetUp(void)
{
    MEDIA_INFO_LOG("CloudMediaClientUtilsTest SetUp");
}

void CloudMediaClientUtilsTest::TearDown(void)
{
    MEDIA_INFO_LOG("CloudMediaClientUtilsTest TearDown");
}

HWTEST_F(CloudMediaClientUtilsTest, GetLowerPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start GetLowerPath_Test_001");
    std::string path = "/storage/cloud/files/Photo/1/IMG_001.jpg";
    int32_t userId = 100;
    std::string result = CloudMediaClientUtils::GetLowerPath(path, userId);
    std::string expected = "/data/service/el2/100/hmdfs/account/files/Photo/1/IMG_001.jpg";
    EXPECT_EQ(result, expected);
    MEDIA_INFO_LOG("End GetLowerPath_Test_001");
}

HWTEST_F(CloudMediaClientUtilsTest, GetLowerPath_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start GetLowerPath_Test_002");
    std::string path = "/invalid/path/Photo/1/IMG_001.jpg";
    int32_t userId = 100;
    std::string result = CloudMediaClientUtils::GetLowerPath(path, userId);
    EXPECT_EQ(result, "");
    MEDIA_INFO_LOG("End GetLowerPath_Test_002");
}

HWTEST_F(CloudMediaClientUtilsTest, GetLowerPath_Test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start GetLowerPath_Test_003");
    std::string path = "/storage/cloud/files/Video/2/VID_001.mp4";
    int32_t userId = 200;
    std::string result = CloudMediaClientUtils::GetLowerPath(path, userId);
    std::string expected = "/data/service/el2/200/hmdfs/account/files/Video/2/VID_001.mp4";
    EXPECT_EQ(result, expected);
    MEDIA_INFO_LOG("End GetLowerPath_Test_003");
}

HWTEST_F(CloudMediaClientUtilsTest, GetLocalPathByPhotosVo_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start GetLocalPathByPhotosVo_Test_001");
    CloudMdkRecordPhotosVo photosVo;
    photosVo.fileSourceType = 2;
    photosVo.data = "/{cloudId}/Photo/1/IMG_001.jpg";
    std::string localPath;
    int32_t userId = 100;
    int32_t ret = CloudMediaClientUtils::GetLocalPathByPhotosVo(photosVo, localPath, userId);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("End GetLocalPathByPhotosVo_Test_001");
}

HWTEST_F(CloudMediaClientUtilsTest, GetLocalPathByPhotosVo_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start GetLocalPathByPhotosVo_Test_002");
    CloudMdkRecordPhotosVo photosVo;
    photosVo.fileSourceType = 3;
    photosVo.storagePath = "/storage/path/IMG_001.jpg";
    std::string localPath;
    int32_t userId = 100;
    int32_t ret = CloudMediaClientUtils::GetLocalPathByPhotosVo(photosVo, localPath, userId);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("End GetLocalPathByPhotosVo_Test_002");
}

HWTEST_F(CloudMediaClientUtilsTest, GetLocalPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start GetLocalPath_Test_001");
    std::string path = "/storage/cloud/files/Photo/1/IMG_001.jpg";
    std::string result = CloudMediaClientUtils::GetLocalPath(path);
    std::string expected = "/storage/media/local/files/Photo/1/IMG_001.jpg";
    EXPECT_EQ(result, expected);
    MEDIA_INFO_LOG("End GetLocalPath_Test_001");
}

HWTEST_F(CloudMediaClientUtilsTest, GetLocalPath_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start GetLocalPath_Test_002");
    std::string path = "/storage/media/local/files/Photo/1/IMG_001.jpg";
    std::string result = CloudMediaClientUtils::GetLocalPath(path);
    EXPECT_EQ(result, path);
    MEDIA_INFO_LOG("End GetLocalPath_Test_002");
}

HWTEST_F(CloudMediaClientUtilsTest, GetLocalPath_Test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start GetLocalPath_Test_003");
    std::string path = "/storage/cloud/files/Video/2/VID_001.mp4";
    std::string result = CloudMediaClientUtils::GetLocalPath(path);
    std::string expected = "/storage/media/local/files/Video/2/VID_001.mp4";
    EXPECT_EQ(result, expected);
    MEDIA_INFO_LOG("End GetLocalPath_Test_003");
}

HWTEST_F(CloudMediaClientUtilsTest, FindLocalPathFromCloudPath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start FindLocalPathFromCloudPath_Test_001");
    std::string path = "/storage/cloud/files/Photo/1/IMG_001.jpg";
    int32_t userId = 100;
    std::string result = CloudMediaClientUtils::FindLocalPathFromCloudPath(path, userId);
    std::string expected = "/storage/media/100/local/files/Photo/1/IMG_001.jpg";
    EXPECT_EQ(result, expected);
    MEDIA_INFO_LOG("End FindLocalPathFromCloudPath_Test_001");
}

HWTEST_F(CloudMediaClientUtilsTest, FindLocalPathFromCloudPath_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start FindLocalPathFromCloudPath_Test_002");
    std::string path = "/storage/media/local/files/Photo/1/IMG_001.jpg";
    int32_t userId = 100;
    std::string result = CloudMediaClientUtils::FindLocalPathFromCloudPath(path, userId);
    std::string expected = "/storage/media/100/local/files/Photo/1/IMG_001.jpg";
    EXPECT_EQ(result, expected);
    MEDIA_INFO_LOG("End FindLocalPathFromCloudPath_Test_002");
}

HWTEST_F(CloudMediaClientUtilsTest, FindLocalPathFromCloudPath_Test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start FindLocalPathFromCloudPath_Test_003");
    std::string path = "/storage/cloud/files/Video/2/VID_001.mp4";
    int32_t userId = 200;
    std::string result = CloudMediaClientUtils::FindLocalPathFromCloudPath(path, userId);
    std::string expected = "/storage/media/200/local/files/Video/2/VID_001.mp4";
    EXPECT_EQ(result, expected);
    MEDIA_INFO_LOG("End FindLocalPathFromCloudPath_Test_003");
}

HWTEST_F(CloudMediaClientUtilsTest, GetVideoCachePath_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start GetVideoCachePath_Test_001");
    std::string filePath = "/storage/cloud/files/Video/1/VID_001.mp4";
    std::string result = CloudMediaClientUtils::GetVideoCachePath(filePath);
    EXPECT_EQ(result, "");
    MEDIA_INFO_LOG("End GetVideoCachePath_Test_001");
}

HWTEST_F(CloudMediaClientUtilsTest, GetVideoCachePath_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start GetVideoCachePath_Test_002");
    std::string filePath = "/invalid/path/Video/1/VID_001.mp4";
    std::string result = CloudMediaClientUtils::GetVideoCachePath(filePath);
    EXPECT_TRUE(result.empty());
    MEDIA_INFO_LOG("End GetVideoCachePath_Test_002");
}

HWTEST_F(CloudMediaClientUtilsTest, GetVideoCachePath_Test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start GetVideoCachePath_Test_003");
    std::string filePath = "/storage/cloud/files/Photo/1/IMG_001.jpg";
    std::string result = CloudMediaClientUtils::GetVideoCachePath(filePath);
    EXPECT_EQ(result, "");
    MEDIA_INFO_LOG("End GetVideoCachePath_Test_003");
}

}  // namespace Media
}  // namespace OHOS

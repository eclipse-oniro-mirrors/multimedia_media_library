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

#include "media_cloud_dentry_batch_task_test.h"

#include "media_cloud_dentry_batch_task.h"
#include "media_log.h"

using namespace testing::ext;
using namespace std;
using namespace OHOS::Media::ORM;

namespace OHOS::Media::Background {

void MediaCloudDentryBatchTaskTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("MediaCloudDentryBatchTaskTest SetUpTestCase");
}

void MediaCloudDentryBatchTaskTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("MediaCloudDentryBatchTaskTest TearDownTestCase");
}

void MediaCloudDentryBatchTaskTest::SetUp() {}

void MediaCloudDentryBatchTaskTest::TearDown(void) {}

/**
 * 测试目的：验证NeedCreateDentryForPhoto方法处理空cloudId时返回false
 * 测试场景：PhotosPo的cloudId为空
 * 预期结果：返回false，不创建dentry
 */
HWTEST_F(MediaCloudDentryBatchTaskTest, NeedCreateDentryForPhoto_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Begin NeedCreateDentryForPhoto_test_001");
    auto task = std::make_shared<MediaCloudDentryBatchTask>();
    ASSERT_NE(task, nullptr);

    PhotosPo photosPo;
    photosPo.data = "/storage/cloud/files/test.jpg";
    photosPo.cloudId = "";
    photosPo.thumbStatus = 1;

    bool result = MediaCloudDentryBatchTask::NeedCreateDentryForPhoto(photosPo);
    EXPECT_EQ(result, false);
    MEDIA_INFO_LOG("End NeedCreateDentryForPhoto_test_001");
}

/**
 * 测试目的：验证NeedCreateDentryForPhoto方法处理空filePath时返回false
 * 测试场景：PhotosPo的filePath为空
 * 预期结果：返回false，不创建dentry
 */
HWTEST_F(MediaCloudDentryBatchTaskTest, NeedCreateDentryForPhoto_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Begin NeedCreateDentryForPhoto_test_002");
    auto task = std::make_shared<MediaCloudDentryBatchTask>();
    ASSERT_NE(task, nullptr);

    PhotosPo photosPo;
    photosPo.data = "";
    photosPo.cloudId = "test_cloud_id";
    photosPo.thumbStatus = 1;

    bool result = MediaCloudDentryBatchTask::NeedCreateDentryForPhoto(photosPo);
    EXPECT_EQ(result, false);
    MEDIA_INFO_LOG("End NeedCreateDentryForPhoto_test_002");
}

/**
 * 测试目的：验证NeedCreateDentryForPhoto方法为正常照片创建origin dentry
 * 测试场景：照片有有效的cloudId和filePath，无缩略图
 * 预期结果：返回true，创建origin dentry
 */
HWTEST_F(MediaCloudDentryBatchTaskTest, NeedCreateDentryForPhoto_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("Begin NeedCreateDentryForPhoto_test_003");
    auto task = std::make_shared<MediaCloudDentryBatchTask>();
    ASSERT_NE(task, nullptr);

    PhotosPo photosPo;
    photosPo.data = "/storage/cloud/files/album/photo.jpg";
    photosPo.cloudId = "test_cloud_id_003";
    photosPo.thumbStatus = 0;
    photosPo.displayName = "photo.jpg";
    photosPo.size = 1024;
    photosPo.dateModified = 1234567890;
    photosPo.mediaType = 1;

    bool result = MediaCloudDentryBatchTask::NeedCreateDentryForPhoto(photosPo);
    EXPECT_EQ(result, true);
    MEDIA_INFO_LOG("End NeedCreateDentryForPhoto_test_003");
}

/**
 * 测试目的：验证NeedCreateDentryForPhoto方法为有LCD缩略图的照片创建LCD dentry
 * 测试场景：照片的thumbStatus第0位为1，有LCD缩略图
 * 预期结果：返回true，创建origin和LCD dentry
 */
HWTEST_F(MediaCloudDentryBatchTaskTest, NeedCreateDentryForPhoto_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("Begin NeedCreateDentryForPhoto_test_004");
    auto task = std::make_shared<MediaCloudDentryBatchTask>();
    ASSERT_NE(task, nullptr);

    PhotosPo photosPo;
    photosPo.data = "/storage/cloud/files/album/photo.jpg";
    photosPo.cloudId = "test_cloud_id_004";
    photosPo.thumbStatus = 1;
    photosPo.displayName = "photo.jpg";
    photosPo.size = 1024;
    photosPo.dateModified = 1234567890;
    photosPo.mediaType = 1;
    photosPo.orientation = 0;
    photosPo.exifRotate = 0;

    bool result = MediaCloudDentryBatchTask::NeedCreateDentryForPhoto(photosPo);
    EXPECT_EQ(result, true);
    MEDIA_INFO_LOG("End NeedCreateDentryForPhoto_test_004");
}

/**
 * 测试目的：验证NeedCreateDentryForPhoto方法为有THM缩略图的照片创建THM dentry
 * 测试场景：照片的thumbStatus第1位为1，有THM缩略图
 * 预期结果：返回true，创建origin和THM dentry
 */
HWTEST_F(MediaCloudDentryBatchTaskTest, NeedCreateDentryForPhoto_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("Begin NeedCreateDentryForPhoto_test_005");
    auto task = std::make_shared<MediaCloudDentryBatchTask>();
    ASSERT_NE(task, nullptr);

    PhotosPo photosPo;
    photosPo.data = "/storage/cloud/files/album/photo.jpg";
    photosPo.cloudId = "test_cloud_id_005";
    photosPo.thumbStatus = 2;
    photosPo.displayName = "photo.jpg";
    photosPo.size = 1024;
    photosPo.dateModified = 1234567890;
    photosPo.mediaType = 1;
    photosPo.orientation = 0;
    photosPo.exifRotate = 0;

    bool result = MediaCloudDentryBatchTask::NeedCreateDentryForPhoto(photosPo);
    EXPECT_EQ(result, true);
    MEDIA_INFO_LOG("End NeedCreateDentryForPhoto_test_005");
}

/**
 * 测试目的：验证NeedCreateDentryForPhoto方法对有扩展缩略图的照片使用EX类型
 * 测试场景：照片有orientation或exifRotate，需要使用扩展缩略图类型
 * 预期结果：返回true，创建origin和扩展类型缩略图dentry
 */
HWTEST_F(MediaCloudDentryBatchTaskTest, NeedCreateDentryForPhoto_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("Begin NeedCreateDentryForPhoto_test_006");
    auto task = std::make_shared<MediaCloudDentryBatchTask>();
    ASSERT_NE(task, nullptr);

    PhotosPo photosPo;
    photosPo.data = "/storage/cloud/files/album/photo.jpg";
    photosPo.cloudId = "test_cloud_id_006";
    photosPo.thumbStatus = 3;
    photosPo.displayName = "photo.jpg";
    photosPo.size = 1024;
    photosPo.dateModified = 1234567890;
    photosPo.mediaType = 1;
    photosPo.orientation = 90;
    photosPo.exifRotate = 0;

    bool result = MediaCloudDentryBatchTask::NeedCreateDentryForPhoto(photosPo);
    EXPECT_EQ(result, true);
    MEDIA_INFO_LOG("End NeedCreateDentryForPhoto_test_006");
}

/**
 * 测试目的：验证BatchInsertDentry方法处理空列表时直接返回
 * 测试场景：传入空的dentryList
 * 预期结果：函数直接返回，不执行插入操作
 */
HWTEST_F(MediaCloudDentryBatchTaskTest, BatchInsertDentry_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Begin BatchInsertDentry_test_001");
    auto task = std::make_shared<MediaCloudDentryBatchTask>();
    ASSERT_NE(task, nullptr);

    std::vector<FileManagement::CloudSync::DentryFileInfo> emptyList;
    MediaCloudDentryBatchTask::BatchInsertDentry(emptyList, "origin");
    MEDIA_INFO_LOG("End BatchInsertDentry_test_001");
    EXPECT_TRUE(emptyList.empty());
}

}  // namespace OHOS::Media::Background
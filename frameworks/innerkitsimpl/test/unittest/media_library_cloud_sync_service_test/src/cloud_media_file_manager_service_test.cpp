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

#define MLOG_TAG "MediaCloudSync"

#include "cloud_media_file_manager_service_test.h"

#include "cloud_media_file_manager_service.h"
#include "cloud_media_pull_data_dto.h"
#include "photos_po.h"
#include "medialibrary_db_const.h"
#include "cloud_media_sync_const.h"
#include "cloud_media_dao_const.h"
#include "cloud_media_sync_utils.h"

using namespace testing::ext;

namespace OHOS::Media::CloudSync {

void CloudMediaFileManagerServiceTest::SetUpTestCase(void)
{}

void CloudMediaFileManagerServiceTest::TearDownTestCase(void)
{}

void CloudMediaFileManagerServiceTest::SetUp()
{}

void CloudMediaFileManagerServiceTest::TearDown()
{}

/**
 * @brief 测试目的：验证FixFileInfo方法正确调用内部服务
 * @brief 测试场景：调用FixFileInfo，验证fileSourceType和storagePath正确修正
 */
HWTEST_F(CloudMediaFileManagerServiceTest, FixFileInfo_Normal_Test_001, TestSize.Level1)
{
    CloudMediaFileManagerService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicDisplayName = "test.jpg";
    pullData.propertiesSourcePath = "/storage/emulated/0/FromDocs/test/test.jpg";
    pullData.basicRecycledTime = 0;

    service.FixFileInfo(pullData);

    EXPECT_EQ(pullData.attributesFileSourceType, static_cast<int32_t>(FileSourceType::FILE_MANAGER));
    EXPECT_FALSE(pullData.attributesStoragePath.empty());
}

/**
 * @brief 测试目的：验证HasLocalAsset对无本地资产的处理逻辑
 * @brief 测试场景：pullData无本地资产，验证返回false
 */
HWTEST_F(CloudMediaFileManagerServiceTest, HasLocalAsset_NoLocalAsset_Test_001, TestSize.Level1)
{
    CloudMediaFileManagerService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.localPhotosPoOp = std::nullopt;

    bool result = service.HasLocalAsset(pullData);

    EXPECT_FALSE(result);
}

/**
 * @brief 测试目的：验证HasLocalAsset对有本地资产的处理逻辑
 * @brief 测试场景：pullData有本地资产，验证返回true
 */
HWTEST_F(CloudMediaFileManagerServiceTest, HasLocalAsset_HasLocalAsset_Test_001, TestSize.Level1)
{
    CloudMediaFileManagerService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";

    PhotosPo photoInfo;
    photoInfo.displayName = "test.jpg";
    pullData.localPhotosPoOp = photoInfo;

    bool result = service.HasLocalAsset(pullData);

    EXPECT_TRUE(result);
}

/**
 * @brief 测试目的：验证IsLocalFile对纯云资产的处理逻辑
 * @brief 测试场景：本地资产position为纯云，验证返回false
 */
HWTEST_F(CloudMediaFileManagerServiceTest, IsLocalFile_CloudAsset_Test_001, TestSize.Level1)
{
    CloudMediaFileManagerService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";

    PhotosPo photoInfo;
    photoInfo.position = static_cast<int32_t>(PhotoPositionType::CLOUD);
    pullData.localPhotosPoOp = photoInfo;

    bool result = service.IsLocalFile(pullData);

    EXPECT_FALSE(result);
}

/**
 * @brief 测试目的：验证IsLocalFile对本地资产的处理逻辑
 * @brief 测试场景：本地资产position为本地，验证返回true
 */
HWTEST_F(CloudMediaFileManagerServiceTest, IsLocalFile_LocalAsset_Test_001, TestSize.Level1)
{
    CloudMediaFileManagerService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";

    PhotosPo photoInfo;
    photoInfo.position = static_cast<int32_t>(PhotoPositionType::LOCAL);
    pullData.localPhotosPoOp = photoInfo;

    bool result = service.IsLocalFile(pullData);

    EXPECT_TRUE(result);
}

/**
 * @brief 测试目的：验证IsNotBothMedia对两端都是媒体资产的处理逻辑
 * @brief 测试场景：本地和云端fileSourceType都是MEDIA，验证返回false
 */
HWTEST_F(CloudMediaFileManagerServiceTest, IsNotBothMedia_BothMedia_Test_001, TestSize.Level1)
{
    CloudMediaFileManagerService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.attributesFileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);

    PhotosPo photoInfo;
    photoInfo.fileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);
    pullData.localPhotosPoOp = photoInfo;

    bool result = service.IsNotBothMedia(pullData);

    EXPECT_FALSE(result);
}

/**
 * @brief 测试目的：验证IsNotBothMedia对一端不是媒体资产的处理逻辑
 * @brief 测试场景：本地fileSourceType为FILE_MANAGER，验证返回true
 */
HWTEST_F(CloudMediaFileManagerServiceTest, IsNotBothMedia_NotBothMedia_Test_001, TestSize.Level1)
{
    CloudMediaFileManagerService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.attributesFileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);

    PhotosPo photoInfo;
    photoInfo.fileSourceType = static_cast<int32_t>(FileSourceType::FILE_MANAGER);
    pullData.localPhotosPoOp = photoInfo;

    bool result = service.IsNotBothMedia(pullData);

    EXPECT_TRUE(result);
}

/**
 * @brief 测试目的：验证IsMediaFile对媒体文件路径的识别逻辑
 * @brief 测试场景：文件路径以媒体桶路径前缀开头，验证返回true
 */
HWTEST_F(CloudMediaFileManagerServiceTest, IsMediaFile_MediaPath_Test_001, TestSize.Level1)
{
    std::string filePath = "/storage/media/local/files/Photo/test.jpg";

    bool result = CloudMediaSyncUtils::IsMediaFile(filePath);

    EXPECT_TRUE(result);
}

/**
 * @brief 测试目的：验证IsMediaFile对非媒体文件路径的识别逻辑
 * @brief 测试场景：文件路径不以媒体桶路径前缀开头，验证返回false
 */
HWTEST_F(CloudMediaFileManagerServiceTest, IsMediaFile_NotMediaPath_Test_001, TestSize.Level1)
{
    std::string filePath = "/storage/media/local/files/Docs/test.jpg";

    bool result = CloudMediaSyncUtils::IsMediaFile(filePath);

    EXPECT_FALSE(result);
}

/**
 * @brief 测试目的：验证Accept方法对不满足移动条件的处理逻辑
 * @brief 测试场景：无本地资产，验证Accept返回false
 */
HWTEST_F(CloudMediaFileManagerServiceTest, Accept_NoLocalAsset_Test_001, TestSize.Level1)
{
    CloudMediaFileManagerService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.localPhotosPoOp = std::nullopt;

    bool result = service.Accept(pullData);

    EXPECT_FALSE(result);
}

/**
 * @brief 测试目的：验证Accept方法对满足部分条件的处理逻辑
 * @brief 测试场景：有本地资产但为纯云资产，验证Accept返回false
 */
HWTEST_F(CloudMediaFileManagerServiceTest, Accept_CloudAsset_Test_001, TestSize.Level1)
{
    CloudMediaFileManagerService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";

    PhotosPo photoInfo;
    photoInfo.position = static_cast<int32_t>(PhotoPositionType::CLOUD);
    pullData.localPhotosPoOp = photoInfo;

    bool result = service.Accept(pullData);

    EXPECT_FALSE(result);
}

/**
 * @brief 测试目的：验证RelocateFile对无本地资产的处理逻辑
 * @brief 测试场景：无本地资产，验证返回E_OK且不执行移动
 */
HWTEST_F(CloudMediaFileManagerServiceTest, RelocateFile_NoLocalAsset_Test_001, TestSize.Level1)
{
    CloudMediaFileManagerService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.localPhotosPoOp = std::nullopt;

    int32_t result = service.RelocateFile(pullData);

    EXPECT_EQ(result, E_OK);
}

/**
 * @brief 测试目的：验证RelocateFile对不满足Accept条件的处理逻辑
 * @brief 测试场景：两端都是MEDIA且路径相同，验证返回E_OK且不执行移动
 */
HWTEST_F(CloudMediaFileManagerServiceTest, RelocateFile_BothMediaSamePath_Test_001, TestSize.Level1)
{
    CloudMediaFileManagerService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.attributesFileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);
    pullData.attributesStoragePath = "/storage/cloud/files/Photo/test.jpg";

    PhotosPo photoInfo;
    photoInfo.fileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);
    photoInfo.position = static_cast<int32_t>(PhotoPositionType::LOCAL);
    photoInfo.storagePath = "/storage/cloud/files/Photo/test.jpg";
    pullData.localPhotosPoOp = photoInfo;

    int32_t result = service.RelocateFile(pullData);

    EXPECT_EQ(result, E_OK);
}

/**
 * @brief 测试目的：验证RelocateFile对纯云资产的处理逻辑
 * @brief 测试场景：本地position为CLOUD，验证返回E_OK且不执行移动
 */
HWTEST_F(CloudMediaFileManagerServiceTest, RelocateFile_CloudPosition_Test_001, TestSize.Level1)
{
    CloudMediaFileManagerService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.attributesFileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);

    PhotosPo photoInfo;
    photoInfo.fileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);
    photoInfo.position = static_cast<int32_t>(PhotoPositionType::CLOUD);
    pullData.localPhotosPoOp = photoInfo;

    int32_t result = service.RelocateFile(pullData);

    EXPECT_EQ(result, E_OK);
}

/**
 * @brief 测试目的：验证ResetPositionToCloudOnly对湖内资产的处理逻辑
 * @brief 测试场景：fileSourceType为MEDIA_HO_LAKE，验证重置为MEDIA
 */
HWTEST_F(CloudMediaFileManagerServiceTest, ResetPositionToCloudOnly_LakeAsset_Test_001, TestSize.Level1)
{
    CloudMediaFileManagerService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.attributesFileSourceType = static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE);

    int32_t result = service.ResetPositionToCloudOnly(pullData);

    EXPECT_EQ(result, E_OK);
}

/**
 * @brief 测试目的：验证ResetPositionToCloudOnly对普通资产的处理逻辑
 * @brief 测试场景：fileSourceType为FILE_MANAGER，验证保持不变
 */
HWTEST_F(CloudMediaFileManagerServiceTest, ResetPositionToCloudOnly_FileManager_Test_001, TestSize.Level1)
{
    CloudMediaFileManagerService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.attributesFileSourceType = static_cast<int32_t>(FileSourceType::FILE_MANAGER);

    int32_t result = service.ResetPositionToCloudOnly(pullData);

    EXPECT_EQ(result, E_OK);
}

/**
 * @brief 测试目的：验证IsLocalFile对无本地资产的边界处理逻辑
 * @brief 测试场景：pullData无本地资产，验证返回false
 */
HWTEST_F(CloudMediaFileManagerServiceTest, IsLocalFile_NoLocalAsset_Test_001, TestSize.Level1)
{
    CloudMediaFileManagerService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.localPhotosPoOp = std::nullopt;

    bool result = service.IsLocalFile(pullData);

    EXPECT_FALSE(result);
}

/**
 * @brief 测试目的：验证IsNotBothMedia对无本地资产的边界处理逻辑
 * @brief 测试场景：pullData无本地资产，验证返回false
 */
HWTEST_F(CloudMediaFileManagerServiceTest, IsNotBothMedia_NoLocalAsset_Test_001, TestSize.Level1)
{
    CloudMediaFileManagerService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.localPhotosPoOp = std::nullopt;

    bool result = service.IsNotBothMedia(pullData);

    EXPECT_FALSE(result);
}

/**
 * @brief 测试目的：验证IsPathChanged对无本地资产的边界处理逻辑
 * @brief 测试场景：pullData无本地资产，验证返回false
 */
HWTEST_F(CloudMediaFileManagerServiceTest, IsPathChanged_NoLocalAsset_Test_001, TestSize.Level1)
{
    CloudMediaFileManagerService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.localPhotosPoOp = std::nullopt;

    bool result = service.IsPathChanged(pullData);

    EXPECT_FALSE(result);
}

/**
 * @brief 测试目的：验证IsTargetMediaFileNotExists对无本地资产的边界处理逻辑
 * @brief 测试场景：pullData无本地资产，验证返回false
 */
HWTEST_F(CloudMediaFileManagerServiceTest, IsTargetMediaFileNotExists_NoLocalAsset_Test_001, TestSize.Level1)
{
    CloudMediaFileManagerService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.localPhotosPoOp = std::nullopt;

    bool result = service.IsTargetMediaFileNotExists(pullData);

    EXPECT_FALSE(result);
}

/**
 * @brief 测试目的：验证IsMediaFile对空路径的处理逻辑
 * @brief 测试场景：filePath为空，验证返回false
 */
HWTEST_F(CloudMediaFileManagerServiceTest, IsMediaFile_EmptyPath_Test_001, TestSize.Level1)
{
    std::string filePath = "";

    bool result = CloudMediaSyncUtils::IsMediaFile(filePath);

    EXPECT_FALSE(result);
}

/**
 * @brief 测试目的：验证IsNotBothMedia对云端为非MEDIA的处理逻辑
 * @brief 测试场景：云端fileSourceType为FILE_MANAGER，本地为MEDIA，验证返回true
 */
HWTEST_F(CloudMediaFileManagerServiceTest, IsNotBothMedia_CloudFileManager_Test_001, TestSize.Level1)
{
    CloudMediaFileManagerService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.attributesFileSourceType = static_cast<int32_t>(FileSourceType::FILE_MANAGER);

    PhotosPo photoInfo;
    photoInfo.fileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);
    pullData.localPhotosPoOp = photoInfo;

    bool result = service.IsNotBothMedia(pullData);

    EXPECT_TRUE(result);
}

/**
 * @brief 测试目的：验证IsNotBothMedia对本地为非MEDIA的处理逻辑
 * @brief 测试场景：云端fileSourceType为MEDIA，本地为LAKE，验证返回true
 */
HWTEST_F(CloudMediaFileManagerServiceTest, IsNotBothMedia_LocalLake_Test_001, TestSize.Level1)
{
    CloudMediaFileManagerService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.attributesFileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);

    PhotosPo photoInfo;
    photoInfo.fileSourceType = static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE);
    pullData.localPhotosPoOp = photoInfo;

    bool result = service.IsNotBothMedia(pullData);

    EXPECT_TRUE(result);
}

/**
 * @brief 测试目的：验证IsLocalFile对position为默认值的处理逻辑
 * @brief 测试场景：本地资产position为默认值0，验证返回false
 */
HWTEST_F(CloudMediaFileManagerServiceTest, IsLocalFile_DefaultPosition_Test_001, TestSize.Level1)
{
    CloudMediaFileManagerService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";

    PhotosPo photoInfo;
    photoInfo.position = 0;
    pullData.localPhotosPoOp = photoInfo;

    bool result = service.IsLocalFile(pullData);

    EXPECT_FALSE(result);
}

/**
 * @brief 测试目的：验证IsLocalFile对position为BOTH的处理逻辑
 * @brief 测试场景：本地资产position为BOTH，验证返回true
 */
HWTEST_F(CloudMediaFileManagerServiceTest, IsLocalFile_BothPosition_Test_001, TestSize.Level1)
{
    CloudMediaFileManagerService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";

    PhotosPo photoInfo;
    photoInfo.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    pullData.localPhotosPoOp = photoInfo;

    bool result = service.IsLocalFile(pullData);

    EXPECT_TRUE(result);
}

}  // namespace OHOS::Media::CloudSync
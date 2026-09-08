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

#include "cloud_media_file_info_service_test.h"

#include "cloud_media_file_info_service.h"
#include "cloud_media_pull_data_dto.h"
#include "photos_po.h"
#include "medialibrary_db_const.h"
#include "cloud_media_sync_const.h"
#include "cloud_media_dao_const.h"

using namespace testing::ext;

namespace OHOS::Media::CloudSync {

void CloudMediaFileInfoServiceTest::SetUpTestCase(void)
{}

void CloudMediaFileInfoServiceTest::TearDownTestCase(void)
{}

void CloudMediaFileInfoServiceTest::SetUp()
{}

void CloudMediaFileInfoServiceTest::TearDown()
{}

/**
 * @brief 测试目的：验证FixFileInfoWithCloudOnly对纯云端文管资产的修正逻辑
 * @brief 测试场景：sourcePath包含/FromDocs/，非隐藏非回收站，验证fileSourceType修正为FILE_MANAGER
 */
HWTEST_F(CloudMediaFileInfoServiceTest, FixFileInfoWithCloudOnly_FromDocsNormal_Test_001, TestSize.Level1)
{
    CloudMediaFileInfoService service;
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
 * @brief 测试目的：验证FixFileInfoWithCloudOnly对隐藏文管资产的修正逻辑
 * @brief 测试场景：sourcePath包含/FromDocs/，attributesSrcAlbumIds包含隐藏相册ID，验证fileSourceType修正为MEDIA
 */
HWTEST_F(CloudMediaFileInfoServiceTest, FixFileInfoWithCloudOnly_FromDocsHidden_Test_001, TestSize.Level1)
{
    CloudMediaFileInfoService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicDisplayName = "test.jpg";
    pullData.propertiesSourcePath = "/storage/emulated/0/FromDocs/test/test.jpg";
    pullData.basicRecycledTime = 0;
    pullData.attributesSrcAlbumIds.push_back(HIDDEN_ALBUM_CLOUD_ID);

    service.FixFileInfo(pullData);

    EXPECT_EQ(pullData.attributesFileSourceType, static_cast<int32_t>(FileSourceType::MEDIA));
    EXPECT_TRUE(pullData.attributesStoragePath.empty());
}

/**
 * @brief 测试目的：验证FixFileInfoWithCloudOnly对回收站文管资产的修正逻辑
 * @brief 测试场景：sourcePath包含/FromDocs/，trashed!=0，验证fileSourceType修正为MEDIA
 */
HWTEST_F(CloudMediaFileInfoServiceTest, FixFileInfoWithCloudOnly_FromDocsTrashed_Test_001, TestSize.Level1)
{
    CloudMediaFileInfoService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicDisplayName = "test.jpg";
    pullData.propertiesSourcePath = "/storage/emulated/0/FromDocs/test/test.jpg";
    pullData.basicRecycledTime = 1000;

    service.FixFileInfo(pullData);

    EXPECT_EQ(pullData.attributesFileSourceType, static_cast<int32_t>(FileSourceType::MEDIA));
    EXPECT_TRUE(pullData.attributesStoragePath.empty());
}

/**
 * @brief 测试目的：验证FixFileInfoWithCloudOnly对普通媒体资产的修正逻辑
 * @brief 测试场景：sourcePath不包含/FromDocs/，验证fileSourceType保持为MEDIA
 */
HWTEST_F(CloudMediaFileInfoServiceTest, FixFileInfoWithCloudOnly_NormalMedia_Test_001, TestSize.Level1)
{
    CloudMediaFileInfoService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicDisplayName = "test.jpg";
    pullData.propertiesSourcePath = "/storage/emulated/0/Pictures/test/test.jpg";
    pullData.basicRecycledTime = 0;

    service.FixFileInfo(pullData);

    EXPECT_EQ(pullData.attributesFileSourceType, static_cast<int32_t>(FileSourceType::MEDIA));
    EXPECT_TRUE(pullData.attributesStoragePath.empty());
}

/**
 * @brief 测试目的：验证FixFileInfoWithLocal对湖内资产的修正逻辑
 * @brief 测试场景：本地storagePath为湖内路径，非隐藏非回收站，验证fileSourceType修正为LAKE
 */
HWTEST_F(CloudMediaFileInfoServiceTest, FixFileInfoWithLocal_LakeNormal_Test_001, TestSize.Level1)
{
    CloudMediaFileInfoService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicDisplayName = "test.jpg";
    pullData.propertiesSourcePath = "/storage/emulated/0/Pictures/test/test.jpg";
    pullData.basicRecycledTime = 0;
    pullData.attributesFileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);

    PhotosPo photoInfo;
    photoInfo.storagePath = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/test.jpg";
    photoInfo.hidden = 0;
    photoInfo.dateTrashed = 0;
    pullData.localPhotosPoOp = photoInfo;

    service.FixFileInfo(pullData);

    EXPECT_EQ(pullData.attributesFileSourceType, static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE));
    EXPECT_FALSE(pullData.attributesStoragePath.empty());
}

/**
 * @brief 测试目的：验证FixFileInfoWithLocal对湖内隐藏资产的修正逻辑
 * @brief 测试场景：本地storagePath为湖内路径，attributesSrcAlbumIds包含隐藏相册ID，验证fileSourceType保持为MEDIA
 */
HWTEST_F(CloudMediaFileInfoServiceTest, FixFileInfoWithLocal_LakeHidden_Test_001, TestSize.Level1)
{
    CloudMediaFileInfoService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicDisplayName = "test.jpg";
    pullData.propertiesSourcePath = "/storage/emulated/0/Pictures/test/test.jpg";
    pullData.basicRecycledTime = 0;
    pullData.attributesFileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);
    pullData.attributesSrcAlbumIds.push_back(HIDDEN_ALBUM_CLOUD_ID);

    PhotosPo photoInfo;
    photoInfo.storagePath = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/test.jpg";
    photoInfo.hidden = 1;
    photoInfo.dateTrashed = 0;
    pullData.localPhotosPoOp = photoInfo;

    service.FixFileInfo(pullData);

    EXPECT_EQ(pullData.attributesFileSourceType, static_cast<int32_t>(FileSourceType::MEDIA));
}

/**
 * @brief 测试目的：验证FixFileInfoWithLocal对湖内回收站资产的修正逻辑
 * @brief 测试场景：本地storagePath为湖内路径，trashed!=0，验证fileSourceType保持为MEDIA
 */
HWTEST_F(CloudMediaFileInfoServiceTest, FixFileInfoWithLocal_LakeTrashed_Test_001, TestSize.Level1)
{
    CloudMediaFileInfoService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicDisplayName = "test.jpg";
    pullData.propertiesSourcePath = "/storage/emulated/0/Pictures/test/test.jpg";
    pullData.basicRecycledTime = 1000;
    pullData.attributesFileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);

    PhotosPo photoInfo;
    photoInfo.storagePath = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/test.jpg";
    photoInfo.hidden = 0;
    photoInfo.dateTrashed = 1000;
    pullData.localPhotosPoOp = photoInfo;

    service.FixFileInfo(pullData);

    EXPECT_EQ(pullData.attributesFileSourceType, static_cast<int32_t>(FileSourceType::MEDIA));
}

/**
 * @brief 测试目的：验证FixFileInfoWithLocal对非湖内资产的处理逻辑
 * @brief 测试场景：本地storagePath为普通路径，验证不修正fileSourceType
 */
HWTEST_F(CloudMediaFileInfoServiceTest, FixFileInfoWithLocal_NotLake_Test_001, TestSize.Level1)
{
    CloudMediaFileInfoService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicDisplayName = "test.jpg";
    pullData.propertiesSourcePath = "/storage/emulated/0/Pictures/test/test.jpg";
    pullData.basicRecycledTime = 0;
    pullData.attributesFileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);

    PhotosPo photoInfo;
    photoInfo.storagePath = "/storage/cloud/files/Photo/test.jpg";
    photoInfo.hidden = 0;
    photoInfo.dateTrashed = 0;
    pullData.localPhotosPoOp = photoInfo;

    service.FixFileInfo(pullData);

    EXPECT_EQ(pullData.attributesFileSourceType, static_cast<int32_t>(FileSourceType::MEDIA));
}

/**
 * @brief 测试目的：验证AdjustFileInfoWithLocal在属性一致时的调整逻辑
 * @brief 测试场景：云端和本地fileSourceType/storagePath一致，验证保持不变
 */
HWTEST_F(CloudMediaFileInfoServiceTest, AdjustFileInfoWithLocal_SameAttributes_Test_001, TestSize.Level1)
{
    CloudMediaFileInfoService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicDisplayName = "test.jpg";
    pullData.propertiesSourcePath = "/storage/emulated/0/Pictures/test/test.jpg";
    pullData.basicRecycledTime = 0;
    pullData.attributesFileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);
    pullData.attributesStoragePath = "";

    PhotosPo photoInfo;
    photoInfo.storagePath = "";
    photoInfo.fileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);
    photoInfo.displayName = "test.jpg";
    photoInfo.hidden = 0;
    photoInfo.dateTrashed = 0;
    pullData.localPhotosPoOp = photoInfo;

    service.FixFileInfo(pullData);

    EXPECT_EQ(pullData.attributesFileSourceType, static_cast<int32_t>(FileSourceType::MEDIA));
    EXPECT_TRUE(pullData.attributesStoragePath.empty());
}

/**
 * @brief 测试目的：验证AdjustFileInfoWithLocal在属性不一致但关键字段变更时的处理逻辑
 * @brief 测试场景：displayName变更，验证不调整fileSourceType/storagePath
 */
HWTEST_F(CloudMediaFileInfoServiceTest, AdjustFileInfoWithLocal_NameChanged_Test_001, TestSize.Level1)
{
    CloudMediaFileInfoService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicDisplayName = "new_test.jpg";
    pullData.propertiesSourcePath = "/storage/emulated/0/Pictures/test/test.jpg";
    pullData.basicRecycledTime = 0;
    pullData.attributesFileSourceType = static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE);
    pullData.attributesStoragePath = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/test.jpg";

    PhotosPo photoInfo;
    photoInfo.storagePath = "";
    photoInfo.fileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);
    photoInfo.displayName = "old_test.jpg";
    photoInfo.hidden = 0;
    photoInfo.dateTrashed = 0;
    pullData.localPhotosPoOp = photoInfo;

    service.FixFileInfo(pullData);

    EXPECT_EQ(pullData.attributesFileSourceType, static_cast<int32_t>(FileSourceType::MEDIA));
}

/**
 * @brief 测试目的：验证AdjustFileInfoWithLocal在hidden变更时的处理逻辑
 * @brief 测试场景：hidden状态变更（通过attributesSrcAlbumIds），验证不调整fileSourceType/storagePath
 */
HWTEST_F(CloudMediaFileInfoServiceTest, AdjustFileInfoWithLocal_HiddenChanged_Test_001, TestSize.Level1)
{
    CloudMediaFileInfoService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicDisplayName = "test.jpg";
    pullData.propertiesSourcePath = "/storage/emulated/0/Pictures/test/test.jpg";
    pullData.basicRecycledTime = 0;
    pullData.attributesFileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);
    pullData.attributesStoragePath = "";
    pullData.attributesSrcAlbumIds.push_back(HIDDEN_ALBUM_CLOUD_ID);

    PhotosPo photoInfo;
    photoInfo.storagePath = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/test.jpg";
    photoInfo.fileSourceType = static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE);
    photoInfo.displayName = "test.jpg";
    photoInfo.hidden = 0;
    photoInfo.dateTrashed = 0;
    pullData.localPhotosPoOp = photoInfo;

    service.FixFileInfo(pullData);

    EXPECT_EQ(pullData.attributesFileSourceType, static_cast<int32_t>(FileSourceType::MEDIA));
}

/**
 * @brief 测试目的：验证AdjustFileInfoWithLocal在trashed变更时的处理逻辑
 * @brief 测试场景：trashed状态变更，验证不调整fileSourceType/storagePath
 */
HWTEST_F(CloudMediaFileInfoServiceTest, AdjustFileInfoWithLocal_TrashedChanged_Test_001, TestSize.Level1)
{
    CloudMediaFileInfoService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicDisplayName = "test.jpg";
    pullData.propertiesSourcePath = "/storage/emulated/0/Pictures/test/test.jpg";
    pullData.basicRecycledTime = 1000;
    pullData.attributesFileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);
    pullData.attributesStoragePath = "";

    PhotosPo photoInfo;
    photoInfo.storagePath = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/test.jpg";
    photoInfo.fileSourceType = static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE);
    photoInfo.displayName = "test.jpg";
    photoInfo.hidden = 0;
    photoInfo.dateTrashed = 0;
    pullData.localPhotosPoOp = photoInfo;

    service.FixFileInfo(pullData);

    EXPECT_EQ(pullData.attributesFileSourceType, static_cast<int32_t>(FileSourceType::MEDIA));
}

/**
 * @brief 测试目的：验证无本地资产时的处理逻辑
 * @brief 测试场景：localPhotosPoOp无值，验证仅执行FixFileInfoWithCloudOnly
 */
HWTEST_F(CloudMediaFileInfoServiceTest, FixFileInfo_NoLocalAsset_Test_001, TestSize.Level1)
{
    CloudMediaFileInfoService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicDisplayName = "test.jpg";
    pullData.propertiesSourcePath = "/storage/emulated/0/Pictures/test/test.jpg";
    pullData.basicRecycledTime = 0;
    pullData.localPhotosPoOp = std::nullopt;

    service.FixFileInfo(pullData);

    EXPECT_EQ(pullData.attributesFileSourceType, static_cast<int32_t>(FileSourceType::MEDIA));
    EXPECT_TRUE(pullData.attributesStoragePath.empty());
}

/**
 * @brief 测试目的：验证文管路径storagePath的生成格式
 * @brief 测试场景：sourcePath包含/FromDocs/且有子目录，验证storagePath格式正确
 */
HWTEST_F(CloudMediaFileInfoServiceTest, FixFileInfoWithCloudOnly_DocsPathFormat_Test_001, TestSize.Level1)
{
    CloudMediaFileInfoService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicDisplayName = "test.jpg";
    pullData.propertiesSourcePath = "/storage/emulated/0/FromDocs/Documents/test.jpg";
    pullData.basicRecycledTime = 0;

    service.FixFileInfo(pullData);

    EXPECT_EQ(pullData.attributesFileSourceType, static_cast<int32_t>(FileSourceType::FILE_MANAGER));
    EXPECT_TRUE(pullData.attributesStoragePath.find("/storage/media/local/files/Docs/") != std::string::npos);
}

/**
 * @brief 测试目的：验证湖内路径storagePath的生成格式
 * @brief 测试场景：本地storagePath为湖内路径，验证storagePath格式正确
 */
HWTEST_F(CloudMediaFileInfoServiceTest, FixFileInfoWithLocal_LakePathFormat_Test_001, TestSize.Level1)
{
    CloudMediaFileInfoService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicDisplayName = "test.jpg";
    pullData.propertiesSourcePath = "/storage/emulated/0/Pictures/test.jpg";
    pullData.basicRecycledTime = 0;
    pullData.attributesFileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);

    PhotosPo photoInfo;
    photoInfo.storagePath = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/test.jpg";
    photoInfo.hidden = 0;
    photoInfo.dateTrashed = 0;
    pullData.localPhotosPoOp = photoInfo;

    service.FixFileInfo(pullData);

    EXPECT_TRUE(
        pullData.attributesStoragePath.find("/storage/media/local/files/Docs/HO_DATA_EXT_MISC/") != std::string::npos);
}

/**
 * @brief 测试目的：验证空sourcePath的处理逻辑
 * @brief 测试场景：sourcePath为空，验证fileSourceType保持为MEDIA
 */
HWTEST_F(CloudMediaFileInfoServiceTest, FixFileInfoWithCloudOnly_EmptySourcePath_Test_001, TestSize.Level1)
{
    CloudMediaFileInfoService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicDisplayName = "test.jpg";
    pullData.propertiesSourcePath = "";
    pullData.basicRecycledTime = 0;

    service.FixFileInfo(pullData);

    EXPECT_EQ(pullData.attributesFileSourceType, static_cast<int32_t>(FileSourceType::MEDIA));
    EXPECT_TRUE(pullData.attributesStoragePath.empty());
}

/**
 * @brief 测试目的：验证隐藏和回收站同时存在的处理逻辑
 * @brief 测试场景：attributesSrcAlbumIds包含隐藏相册ID且trashed!=0，验证fileSourceType修正为MEDIA
 */
HWTEST_F(CloudMediaFileInfoServiceTest, FixFileInfoWithCloudOnly_HiddenAndTrashed_Test_001, TestSize.Level1)
{
    CloudMediaFileInfoService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicDisplayName = "test.jpg";
    pullData.propertiesSourcePath = "/storage/emulated/0/FromDocs/test/test.jpg";
    pullData.basicRecycledTime = 1000;
    pullData.attributesSrcAlbumIds.push_back(HIDDEN_ALBUM_CLOUD_ID);

    service.FixFileInfo(pullData);

    EXPECT_EQ(pullData.attributesFileSourceType, static_cast<int32_t>(FileSourceType::MEDIA));
    EXPECT_TRUE(pullData.attributesStoragePath.empty());
}

/**
 * @brief 测试目的：验证fileSourceType为FILE_MANAGER时不修正的逻辑
 * @brief 测试场景：云端fileSourceType已为FILE_MANAGER，验证FixFileInfoWithLocal不修正
 */
HWTEST_F(CloudMediaFileInfoServiceTest, FixFileInfoWithLocal_ExistingFileManager_Test_001, TestSize.Level1)
{
    CloudMediaFileInfoService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicDisplayName = "test.jpg";
    pullData.propertiesSourcePath = "/storage/emulated/0/FromDocs/test/test.jpg";
    pullData.basicRecycledTime = 0;
    pullData.attributesFileSourceType = static_cast<int32_t>(FileSourceType::FILE_MANAGER);
    pullData.attributesStoragePath = "/storage/media/local/files/Docs/test.jpg";

    PhotosPo photoInfo;
    photoInfo.storagePath = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/test.jpg";
    photoInfo.fileSourceType = static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE);
    photoInfo.displayName = "test.jpg";
    photoInfo.hidden = 0;
    photoInfo.dateTrashed = 0;
    pullData.localPhotosPoOp = photoInfo;

    service.FixFileInfo(pullData);

    EXPECT_EQ(pullData.attributesFileSourceType, static_cast<int32_t>(FileSourceType::FILE_MANAGER));
}

/**
 * @brief 测试目的：验证本地storagePath为空时的处理逻辑
 * @brief 测试场景：本地storagePath为空，验证不修正为湖内类型
 */
HWTEST_F(CloudMediaFileInfoServiceTest, FixFileInfoWithLocal_EmptyLocalStoragePath_Test_001, TestSize.Level1)
{
    CloudMediaFileInfoService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicDisplayName = "test.jpg";
    pullData.propertiesSourcePath = "/storage/emulated/0/Pictures/test.jpg";
    pullData.basicRecycledTime = 0;
    pullData.attributesFileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);

    PhotosPo photoInfo;
    photoInfo.storagePath = "";
    photoInfo.hidden = 0;
    photoInfo.dateTrashed = 0;
    pullData.localPhotosPoOp = photoInfo;

    service.FixFileInfo(pullData);

    EXPECT_EQ(pullData.attributesFileSourceType, static_cast<int32_t>(FileSourceType::MEDIA));
}

/**
 * @brief 测试目的：验证特殊字符displayName的处理逻辑
 * @brief 测试场景：displayName包含特殊字符，验证storagePath正确生成
 */
HWTEST_F(CloudMediaFileInfoServiceTest, FixFileInfoWithCloudOnly_SpecialChars_Test_001, TestSize.Level1)
{
    CloudMediaFileInfoService service;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.basicDisplayName = "test file.name.jpg";
    pullData.propertiesSourcePath = "/storage/emulated/0/FromDocs/test/test file.name.jpg";
    pullData.basicRecycledTime = 0;

    service.FixFileInfo(pullData);

    EXPECT_EQ(pullData.attributesFileSourceType, static_cast<int32_t>(FileSourceType::FILE_MANAGER));
    EXPECT_FALSE(pullData.attributesStoragePath.empty());
}

}  // namespace OHOS::Media::CloudSync
/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "CloudMediaServiceTest"

#include "cloud_media_service_test.h"

#include "media_log.h"
#include "result_set_utils.h"
#include "media_file_utils.h"
#include "cloud_media_album_service.h"
#include "cloud_media_data_service.h"
#include "cloud_media_download_service.h"

#define protected public
#define private public
#include "cloud_media_photos_service.h"
#undef protected
#undef private
#include "medialibrary_unittest_utils.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unistore_manager.h"
#include "media_unique_number_column.h"
#include "medialibrary_db_const_sqls.h"
#include "media_cloud_sync_test_utils.h"

namespace OHOS::Media::CloudSync {
using namespace testing::ext;
using namespace OHOS::NativeRdb;

static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;

void CloudMediaServiceTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    SetTestTables(g_rdbStore);
}

void CloudMediaServiceTest::TearDownTestCase(void)
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    ClearAndRestart(g_rdbStore);
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("Clean is finish");
}

// SetUp:Execute before each test case
void CloudMediaServiceTest::SetUp()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    ClearAndRestart(g_rdbStore);
}

void CloudMediaServiceTest::TearDown() {}

HWTEST_F(CloudMediaServiceTest, GetAlbumRecordsIllegalSize_Test, TestSize.Level1)
{
    int32_t size = -1;
    auto services = std::make_shared<CloudMediaAlbumService>();
    ASSERT_TRUE(services);
    bool isCloudSpaceFull = false;
    std::vector<PhotoAlbumPo> photoAlbumList;
    auto ret = services->GetCreatedRecords(size, isCloudSpaceFull, photoAlbumList);
    services->GetCreatedRecords(1001, isCloudSpaceFull, photoAlbumList);
    EXPECT_EQ(photoAlbumList.empty(), true);

    isCloudSpaceFull = true;
    ret = services->GetCreatedRecords(size, isCloudSpaceFull, photoAlbumList);
    services->GetCreatedRecords(1001, isCloudSpaceFull, photoAlbumList);
    EXPECT_EQ(photoAlbumList.empty(), true);

    auto result = services->GetAlbumFileModifiedRecords(size);
    services->GetAlbumFileModifiedRecords(1001);
    EXPECT_EQ(result.empty(), true);

    result = services->GetAlbumDeletedRecords(size);
    services->GetAlbumDeletedRecords(1001);
    EXPECT_EQ(result.empty(), true);
}

HWTEST_F(CloudMediaServiceTest, GetAlbumRecordsNormalSize_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetAlbumCreatedRecords_Test");
    int32_t size = 1;
    bool isCloudSpaceFull = false;
    std::vector<PhotoAlbumPo> photoAlbumList;
    auto services = std::make_shared<CloudMediaAlbumService>();
    ASSERT_TRUE(services);
    auto ret = services->GetCreatedRecords(size, isCloudSpaceFull, photoAlbumList);
    EXPECT_EQ(photoAlbumList.empty(), true);
    EXPECT_EQ(ret, E_OK);

    isCloudSpaceFull = true;
    ret = services->GetCreatedRecords(size, isCloudSpaceFull, photoAlbumList);
    EXPECT_EQ(photoAlbumList.empty(), true);
    EXPECT_EQ(ret, E_OK);

    auto result = services->GetAlbumMetaModifiedRecords(size);
    EXPECT_EQ(result.empty(), true);

    result = services->GetAlbumFileModifiedRecords(size);
    EXPECT_EQ(result.empty(), true);

    result = services->GetAlbumDeletedRecords(size);
    EXPECT_EQ(result.empty(), true);
    MEDIA_INFO_LOG("end GetAlbumCreatedRecords_Test");
}

HWTEST_F(CloudMediaServiceTest, GetAlbumOnRecordsEmptyList_Test, TestSize.Level1)
{
    std::vector<PhotoAlbumDto> albumDtoList;
    int32_t failSize = 0;
    auto services = std::make_shared<CloudMediaAlbumService>();
    ASSERT_TRUE(services);

    int32_t ret = services->OnCreateRecords(albumDtoList, failSize);
    EXPECT_EQ(ret, E_OK);
    ret = services->OnMdirtyRecords(albumDtoList, failSize);
    EXPECT_EQ(ret, E_OK);
    ret = services->OnDeleteRecords(albumDtoList, failSize);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaServiceTest, GetAlbumOnRecordsEmptyPathList_Test, TestSize.Level1)
{
    int32_t failSize = 1;
    PhotoAlbumDto dto;
    std::vector<PhotoAlbumDto> albumDtoList;
    albumDtoList.emplace_back(dto);
    auto services = std::make_shared<CloudMediaAlbumService>();
    ASSERT_TRUE(services);

    int32_t ret = services->OnCreateRecords(albumDtoList, failSize);
    EXPECT_EQ(ret, E_OK);
    ret = services->OnMdirtyRecords(albumDtoList, failSize);
    EXPECT_EQ(ret, E_OK);
    ret = services->OnDeleteRecords(albumDtoList, failSize);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaServiceTest, GetAlbumOnRecordsNormalList_Test, TestSize.Level1)
{
    PhotoAlbumDto dto;
    std::vector<PhotoAlbumDto> albumDtoList;
    OnFetchRecordsAlbumRespBody resp;
    auto services = std::make_shared<CloudMediaAlbumService>();
    ASSERT_TRUE(services);
    int32_t ret = services->OnFetchRecords(albumDtoList, resp);
    EXPECT_EQ(ret, E_OK);

    albumDtoList.emplace_back(dto);
    ret = services->OnFetchRecords(albumDtoList, resp);
    EXPECT_EQ(ret, E_OK);

    albumDtoList.clear();
    dto.lPath = DEFAULT_SCREENSHOT_LPATH_EN;
    dto.cloudId = DEFAULT_SCREENSHOT_CLOUDID;
    albumDtoList.emplace_back(dto);
    ret = services->OnFetchRecords(albumDtoList, resp);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaServiceTest, GetAlbumCallback_Test, TestSize.Level1)
{
    auto services = std::make_shared<CloudMediaAlbumService>();
    ASSERT_TRUE(services);

    auto ret = services->OnStartSync();
    EXPECT_EQ(ret, E_OK);
    MediaOperateResult optRet = {"", 0, ""};
    ret = services->OnCompletePull(optRet);
    EXPECT_EQ(ret, E_OK);
    ret = services->OnCompletePush();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaServiceTest, GetPhotoRecords_Test, TestSize.Level1)
{
    auto services = std::make_shared<CloudMediaPhotosService>();
    ASSERT_TRUE(services);
}
}

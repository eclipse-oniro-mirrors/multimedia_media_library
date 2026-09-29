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

#define MLOG_TAG "MediaAlbumsControllerServiceTest"

#include "hidden_attribute_change_test.h"

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "media_albums_controller_service.h"

#include "album_change_set_hidden_attribute_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "test_data_builder.h"
#include "rdb_predicates.h"
#include "result_set_utils.h"
#include "media_upgrade.h"
#include "album_change_set_hidden_attribute_dto.h"
#include "album_change_set_hidden_attribute_vo.h"
#include "media_albums_service.h"
#include "media_permission_policy_type.h"
#include "medialibrary_business_code.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace IPC;

static shared_ptr<MediaLibraryRdbStore> g_rdbStoreChange;
static constexpr int32_t SLEEP_SECONDS = 1;

static int32_t g_userAlbumIdChange;
static int32_t g_sourceAlbumIdChange;
static int32_t g_fileManagerAlbumIdChange;
static int32_t g_userAlbumAssetIdChange;
static int32_t g_sourceAlbumAssetIdChange;
static int32_t g_fileManagerAlbumAssetIdChange;

static std::vector<std::string> createTableSqlLists = {
    PhotoAlbumColumns::CREATE_TABLE,
    PhotoUpgrade::CREATE_PHOTO_TABLE,
};

static std::vector<std::string> testTables = {
    PhotoAlbumColumns::TABLE,
    PhotoColumn::PHOTOS_TABLE,
};

void AlbumChangeSetHiddenAttributeChangeTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStoreChange = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStoreChange, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStoreChange, createTableSqlLists);

    MEDIA_INFO_LOG("AlbumChangeSetHiddenAttributeChangeTest SetUpTestCase done");
}

void AlbumChangeSetHiddenAttributeChangeTest::SetUp()
{
    auto& builder = TestDataBuilder::GetInstance();
    builder.Init(g_rdbStoreChange);
    builder.ClearAllTables();

    g_userAlbumIdChange = builder.CreateAlbum(TestAlbumType::USER_ALBUM, "TestUserAlbum");
    g_sourceAlbumIdChange = builder.CreateAlbum(TestAlbumType::SOURCE_ALBUM, "TestSourceAlbum");
    g_fileManagerAlbumIdChange = builder.CreateAlbum(TestAlbumType::FILE_MANAGER_ALBUM, "TestFileManagerAlbum");
    ASSERT_GT(g_userAlbumIdChange, 0);
    ASSERT_GT(g_sourceAlbumIdChange, 0);
    ASSERT_GT(g_fileManagerAlbumIdChange, 0);

    g_userAlbumAssetIdChange = builder.CreateAsset(g_userAlbumIdChange, "user_asset");
    g_sourceAlbumAssetIdChange = builder.CreateAsset(g_sourceAlbumIdChange, "source_asset");
    g_fileManagerAlbumAssetIdChange = builder.CreateAssetWithStoragePath(g_fileManagerAlbumIdChange,
        "filemanager_asset", "/storage/media/local/files/Docs/test/filemanager_asset.jpg");
    ASSERT_GT(g_userAlbumAssetIdChange, 0);
    ASSERT_GT(g_sourceAlbumAssetIdChange, 0);
    ASSERT_GT(g_fileManagerAlbumAssetIdChange, 0);

    MEDIA_INFO_LOG("AlbumChangeSetHiddenAttributeChangeTest SetUp");
}

void AlbumChangeSetHiddenAttributeChangeTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStoreChange, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("AlbumChangeSetHiddenAttributeChangeTest TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void AlbumChangeSetHiddenAttributeChangeTest::TearDown() {}

HWTEST_F(AlbumChangeSetHiddenAttributeChangeTest, Dto_FromVo_CopyAllFields, TestSize.Level0)
{
    AlbumChangeSetHiddenAttributeReqBody reqBody;
    reqBody.albumId = 123;
    reqBody.fileHidden = true;
    reqBody.inherited = false;
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SOURCE);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER);

    AlbumChangeSetHiddenAttributeDto dto;
    dto.FromVo(reqBody);
    EXPECT_EQ(dto.albumId, 123);
    EXPECT_EQ(dto.fileHidden, true);
    EXPECT_EQ(dto.inherited, false);
    EXPECT_EQ(dto.albumType, static_cast<int32_t>(PhotoAlbumType::SOURCE));
    EXPECT_EQ(dto.albumSubType, static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER));
}

HWTEST_F(AlbumChangeSetHiddenAttributeChangeTest, Vo_MarshallingUnmarshalling_RoundTrip, TestSize.Level0)
{
    AlbumChangeSetHiddenAttributeReqBody reqBody;
    reqBody.albumId = 88;
    reqBody.fileHidden = true;
    reqBody.inherited = false;
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::USER);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC);

    MessageParcel data;
    ASSERT_TRUE(reqBody.Marshalling(data));
    AlbumChangeSetHiddenAttributeReqBody out;
    ASSERT_TRUE(out.Unmarshalling(data));
    EXPECT_EQ(out.albumId, 88);
    EXPECT_EQ(out.fileHidden, true);
    EXPECT_EQ(out.inherited, false);
    EXPECT_EQ(out.albumType, static_cast<int32_t>(PhotoAlbumType::USER));
    EXPECT_EQ(out.albumSubType, static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
}

HWTEST_F(AlbumChangeSetHiddenAttributeChangeTest, Vo_Unmarshalling_DefaultsRoundTrip, TestSize.Level0)
{
    AlbumChangeSetHiddenAttributeReqBody reqBody;
    MessageParcel data;
    ASSERT_TRUE(reqBody.Marshalling(data));
    AlbumChangeSetHiddenAttributeReqBody out;
    ASSERT_TRUE(out.Unmarshalling(data));
    EXPECT_EQ(out.albumId, 0);
    EXPECT_EQ(out.fileHidden, false);
    EXPECT_EQ(out.inherited, false);
    EXPECT_EQ(out.albumType, -1);
    EXPECT_EQ(out.albumSubType, -1);
}

HWTEST_F(AlbumChangeSetHiddenAttributeChangeTest, Service_InvalidTypeCombo_ReturnsInvalidValues, TestSize.Level1)
{
    AlbumChangeSetHiddenAttributeReqBody reqBody;
    reqBody.albumId = g_userAlbumIdChange;
    reqBody.fileHidden = true;
    reqBody.inherited = false;
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::USER);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC);
    MessageParcel data;
    ASSERT_TRUE(reqBody.Marshalling(data));
    MessageParcel reply;
    auto controller = make_shared<MediaAlbumsControllerService>();
    EXPECT_EQ(controller->AlbumChangeSetHiddenAttribute(data, reply), E_INVALID_VALUES);
}

HWTEST_F(AlbumChangeSetHiddenAttributeChangeTest, Service_InvalidZeroType_ReturnsInvalidValues, TestSize.Level1)
{
    AlbumChangeSetHiddenAttributeReqBody reqBody;
    reqBody.albumId = g_userAlbumIdChange;
    reqBody.fileHidden = true;
    reqBody.inherited = false;
    reqBody.albumType = 0;
    reqBody.albumSubType = 0;
    MessageParcel data;
    ASSERT_TRUE(reqBody.Marshalling(data));
    MessageParcel reply;
    auto controller = make_shared<MediaAlbumsControllerService>();
    EXPECT_EQ(controller->AlbumChangeSetHiddenAttribute(data, reply), E_INVALID_VALUES);
}

HWTEST_F(AlbumChangeSetHiddenAttributeChangeTest, Service_ValidUserCombo_Delegates, TestSize.Level1)
{
    AlbumChangeSetHiddenAttributeReqBody reqBody;
    reqBody.albumId = g_userAlbumIdChange;
    reqBody.fileHidden = true;
    reqBody.inherited = false;
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::USER);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC);
    MessageParcel data;
    ASSERT_TRUE(reqBody.Marshalling(data));
    MessageParcel reply;
    auto controller = make_shared<MediaAlbumsControllerService>();
    EXPECT_NE(controller->AlbumChangeSetHiddenAttribute(data, reply), E_INVALID_VALUES);
}

HWTEST_F(AlbumChangeSetHiddenAttributeChangeTest, PermissionPolicy_HiddenAttribute, TestSize.Level0)
{
    auto service = make_shared<MediaAlbumsControllerService>();
    std::vector<std::vector<PermissionType>> policy;
    bool isBypass = false;
    int32_t ret = service->GetPermissionPolicy(
        static_cast<uint32_t>(MediaLibraryBusinessCode::ALBUM_CHANGE_SET_HIDDEN_ATTRIBUTE), policy, isBypass);
    EXPECT_EQ(ret, E_SUCCESS);
    ASSERT_FALSE(policy.empty());
    ASSERT_EQ(policy.size(), 1U);
    ASSERT_EQ(policy[0].size(), 1U);
    EXPECT_EQ(policy[0][0], SYSTEMAPI_PERM);
    EXPECT_FALSE(isBypass);
}

HWTEST_F(AlbumChangeSetHiddenAttributeChangeTest, PermissionPolicy_UnknownCode_ReturnsFail, TestSize.Level0)
{
    auto service = make_shared<MediaAlbumsControllerService>();
    std::vector<std::vector<PermissionType>> policy;
    bool isBypass = false;
    int32_t ret = service->GetPermissionPolicy(0xFFFFFFFF, policy, isBypass);
    EXPECT_EQ(ret, E_FAIL);
}

HWTEST_F(AlbumChangeSetHiddenAttributeChangeTest, PermissionPolicy_BypassCode_SetsBypass, TestSize.Level0)
{
    auto service = make_shared<MediaAlbumsControllerService>();
    std::vector<std::vector<PermissionType>> policy;
    bool isBypass = false;
    int32_t ret = service->GetPermissionPolicy(
        static_cast<uint32_t>(MediaLibraryBusinessCode::ALBUM_SYS_GET_ASSETS), policy, isBypass);
    EXPECT_EQ(ret, E_SUCCESS);
    EXPECT_TRUE(isBypass);
}
} // namespace OHOS::Media

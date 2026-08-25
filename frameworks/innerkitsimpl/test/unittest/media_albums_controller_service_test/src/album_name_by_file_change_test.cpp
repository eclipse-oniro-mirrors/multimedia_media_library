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

#include "album_name_by_file_change_test.h"

#include <memory>
#include <string>
#include <vector>

#include "media_albums_controller_service.h"

#include "album_change_set_album_name_by_file_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "test_data_builder.h"
#include "rdb_predicates.h"
#include "media_upgrade.h"
#include "album_change_set_album_name_by_file_dto.h"
#include "clone_to_album_vo.h"
#include "media_albums_service.h"
#include "media_permission_policy_type.h"
#include "medialibrary_business_code.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace IPC;

static shared_ptr<MediaLibraryRdbStore> g_rdbStoreChange;

static int32_t g_userAlbumIdChange;
static int32_t g_sourceAlbumIdChange;
static int32_t g_fileManagerAlbumIdChange;

static std::vector<std::string> createTableSqlLists = {
    PhotoAlbumColumns::CREATE_TABLE,
    PhotoUpgrade::CREATE_PHOTO_TABLE,
};

static std::vector<std::string> testTables = {
    PhotoAlbumColumns::TABLE,
    PhotoColumn::PHOTOS_TABLE,
};

void AlbumChangeSetAlbumNameByFileChangeTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStoreChange = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStoreChange, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStoreChange, createTableSqlLists);

    MEDIA_INFO_LOG("AlbumChangeSetAlbumNameByFileChangeTest SetUpTestCase done");
}

void AlbumChangeSetAlbumNameByFileChangeTest::SetUp()
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

    MEDIA_INFO_LOG("AlbumChangeSetAlbumNameByFileChangeTest SetUp");
}

void AlbumChangeSetAlbumNameByFileChangeTest::TearDownTestCase(void)
{
    std::system("rm -rf /storage/media/local/files/Docs/NewFileManagerAlbumName");
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStoreChange, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("AlbumChangeSetAlbumNameByFileChangeTest TearDownTestCase");
}

void AlbumChangeSetAlbumNameByFileChangeTest::TearDown() {}

HWTEST_F(AlbumChangeSetAlbumNameByFileChangeTest, Dto_FromVo_CopyAllFields, TestSize.Level0)
{
    AlbumChangeSetAlbumNameByFileReqBody reqBody;
    reqBody.albumId = 55;
    reqBody.albumName = "NameFromFile";
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SOURCE);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC);

    AlbumChangeSetAlbumNameByFileDto dto;
    dto.FromVo(reqBody);
    EXPECT_EQ(dto.albumId, 55);
    EXPECT_EQ(dto.albumName, "NameFromFile");
    EXPECT_EQ(dto.albumType, static_cast<int32_t>(PhotoAlbumType::SOURCE));
    EXPECT_EQ(dto.albumSubType, static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC));
}

HWTEST_F(AlbumChangeSetAlbumNameByFileChangeTest, Vo_MarshallingUnmarshalling_RoundTrip, TestSize.Level0)
{
    AlbumChangeSetAlbumNameByFileReqBody reqBody;
    reqBody.albumId = 42;
    reqBody.albumName = "MyAlbum";
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::USER);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC);

    MessageParcel data;
    ASSERT_TRUE(reqBody.Marshalling(data));
    AlbumChangeSetAlbumNameByFileReqBody out;
    ASSERT_TRUE(out.Unmarshalling(data));
    EXPECT_EQ(out.albumId, 42);
    EXPECT_EQ(out.albumName, "MyAlbum");
    EXPECT_EQ(out.albumType, static_cast<int32_t>(PhotoAlbumType::USER));
    EXPECT_EQ(out.albumSubType, static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
}

HWTEST_F(AlbumChangeSetAlbumNameByFileChangeTest, CloneToAlbumVo_MarshallingUnmarshalling_RoundTrip, TestSize.Level0)
{
    CloneToAlbumReqBody reqBody;
    reqBody.assetsArray = {"/a/1.jpg", "/a/2.jpg"};
    reqBody.albumId = 7;
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::USER);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC);
    reqBody.mode = 1;
    reqBody.requestId = 99;
    reqBody.albumLpath = "/storage/clone/lpath";
    reqBody.targetDir = "/data/clone/target";
    reqBody.progressCallback = nullptr;

    MessageParcel data;
    ASSERT_TRUE(reqBody.Marshalling(data));
    CloneToAlbumReqBody out;
    ASSERT_TRUE(out.Unmarshalling(data));
    ASSERT_EQ(out.assetsArray.size(), 2U);
    EXPECT_EQ(out.assetsArray[0], "/a/1.jpg");
    EXPECT_EQ(out.assetsArray[1], "/a/2.jpg");
    EXPECT_EQ(out.albumId, 7);
    EXPECT_EQ(out.albumType, static_cast<int32_t>(PhotoAlbumType::USER));
    EXPECT_EQ(out.albumSubType, static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    EXPECT_EQ(out.mode, 1);
    EXPECT_EQ(out.requestId, 99);
    EXPECT_EQ(out.albumLpath, "/storage/clone/lpath");
    EXPECT_EQ(out.targetDir, "/data/clone/target");
    EXPECT_EQ(out.progressCallback, nullptr);
}

HWTEST_F(AlbumChangeSetAlbumNameByFileChangeTest, Service_InvalidTypeCombo_ReturnsInvalidValues, TestSize.Level1)
{
    AlbumChangeSetAlbumNameByFileReqBody reqBody;
    reqBody.albumId = g_userAlbumIdChange;
    reqBody.albumName = "x";
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::USER);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC); // 非法组合
    MessageParcel data;
    ASSERT_TRUE(reqBody.Marshalling(data));
    MessageParcel reply;
    auto controller = make_shared<MediaAlbumsControllerService>();
    EXPECT_EQ(controller->AlbumChangeSetAlbumNameByFile(data, reply), E_INVALID_VALUES);
}

HWTEST_F(AlbumChangeSetAlbumNameByFileChangeTest, Service_ValidUserCombo_Delegates, TestSize.Level1)
{
    AlbumChangeSetAlbumNameByFileReqBody reqBody;
    reqBody.albumId = g_userAlbumIdChange;
    reqBody.albumName = "ServiceRenameByFile";
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::USER);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC);
    MessageParcel data;
    ASSERT_TRUE(reqBody.Marshalling(data));
    MessageParcel reply;
    auto controller = make_shared<MediaAlbumsControllerService>();
    EXPECT_NE(controller->AlbumChangeSetAlbumNameByFile(data, reply), E_INVALID_VALUES);
}

HWTEST_F(AlbumChangeSetAlbumNameByFileChangeTest, PermissionPolicy_AlbumNameByFile, TestSize.Level0)
{
    auto service = make_shared<MediaAlbumsControllerService>();
    std::vector<std::vector<PermissionType>> policy;
    bool isBypass = false;
    int32_t ret = service->GetPermissionPolicy(
        static_cast<uint32_t>(MediaLibraryBusinessCode::ALBUM_CHANGE_SET_ALBUM_NAME_BY_FILE), policy, isBypass);
    EXPECT_EQ(ret, E_SUCCESS);
    ASSERT_FALSE(policy.empty());
    ASSERT_EQ(policy[0].size(), 1U);
    EXPECT_EQ(policy[0][0], SYSTEMAPI_PERM);
    EXPECT_FALSE(isBypass);
}
} // namespace OHOS::Media

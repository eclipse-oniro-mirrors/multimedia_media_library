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

#include "album_change_set_hidden_attribute_test.h"

#include <memory>
#include <string>
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

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace IPC;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;

static int32_t g_userAlbumId;
static int32_t g_sourceAlbumId;
static int32_t g_fileManagerAlbumId;
static int32_t g_userAlbumAssetId;
static int32_t g_sourceAlbumAssetId;
static int32_t g_fileManagerAlbumAssetId;

static std::vector<std::string> createTableSqlLists = {
    PhotoAlbumColumns::CREATE_TABLE,
    PhotoUpgrade::CREATE_PHOTO_TABLE,
};

static std::vector<std::string> testTables = {
    PhotoAlbumColumns::TABLE,
    PhotoColumn::PHOTOS_TABLE,
};

void AlbumChangeSetHiddenAttributeTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);

    MEDIA_INFO_LOG("AlbumChangeSetHiddenAttributeTest SetUpTestCase done");
}

void AlbumChangeSetHiddenAttributeTest::SetUp()
{
    auto& builder = TestDataBuilder::GetInstance();
    builder.Init(g_rdbStore);
    builder.ClearAllTables();

    g_userAlbumId = builder.CreateAlbum(TestAlbumType::USER_ALBUM, "TestUserAlbum");
    g_sourceAlbumId = builder.CreateAlbum(TestAlbumType::SOURCE_ALBUM, "TestSourceAlbum");
    g_fileManagerAlbumId = builder.CreateAlbum(TestAlbumType::FILE_MANAGER_ALBUM, "TestFileManagerAlbum");
    ASSERT_GT(g_userAlbumId, 0);
    ASSERT_GT(g_sourceAlbumId, 0);
    ASSERT_GT(g_fileManagerAlbumId, 0);

    g_userAlbumAssetId = builder.CreateAsset(g_userAlbumId, "user_asset");
    g_sourceAlbumAssetId = builder.CreateAsset(g_sourceAlbumId, "source_asset");
    g_fileManagerAlbumAssetId = builder.CreateAssetWithStoragePath(g_fileManagerAlbumId, "filemanager_asset",
        "/storage/media/local/files/Docs/test/filemanager_asset.jpg");
    ASSERT_GT(g_userAlbumAssetId, 0);
    ASSERT_GT(g_sourceAlbumAssetId, 0);
    ASSERT_GT(g_fileManagerAlbumAssetId, 0);

    MEDIA_INFO_LOG("AlbumChangeSetHiddenAttributeTest SetUp");
}

void AlbumChangeSetHiddenAttributeTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("AlbumChangeSetHiddenAttributeTest TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void AlbumChangeSetHiddenAttributeTest::TearDown() {}

static int32_t SetAlbumHiddenAttribute(int32_t albumId, bool fileHidden, bool inherited,
    int32_t albumType, int32_t albumSubType)
{
    AlbumChangeSetHiddenAttributeReqBody reqBody;
    reqBody.albumId = albumId;
    reqBody.fileHidden = fileHidden;
    reqBody.inherited = inherited;
    reqBody.albumType = albumType;
    reqBody.albumSubType = albumSubType;

    MessageParcel data;
    MessageParcel reply;
    bool marshalRet = reqBody.Marshalling(data);
    if (!marshalRet) {
        return -1;
    }

    auto service = make_shared<MediaAlbumsControllerService>();
    service->AlbumChangeSetHiddenAttribute(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    if (!respVo.Unmarshalling(reply)) {
        return -1;
    }
    int32_t ret = respVo.GetErrCode();
    if (ret == E_OK) {
        return ret;
    }

    MessageParcel retryData;
    MessageParcel retryReply;
    if (!reqBody.Marshalling(retryData)) {
        return ret;
    }
    service->AlbumChangeSetHiddenAttribute(retryData, retryReply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> retryRespVo;
    if (!retryRespVo.Unmarshalling(retryReply)) {
        return ret;
    }
    return retryRespVo.GetErrCode();
}

static bool CheckAssetHiddenStatus(int32_t assetId, bool expectedHidden)
{
    vector<string> columns = { PhotoColumn::PHOTO_FILE_HIDDEN };
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(MediaColumn::MEDIA_ID, std::to_string(assetId));
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
        if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
            return false;
        }
    }
    int32_t fileHidden = GetInt32Val(PhotoColumn::PHOTO_FILE_HIDDEN, resultSet);
    return (fileHidden == (expectedHidden ? 1 : 0));
}

static bool CheckAlbumHiddenStatus(int32_t albumId, bool expectedHidden)
{
    vector<string> columns = { PhotoAlbumColumns::ALBUM_FILE_HIDDEN };
    RdbPredicates rdbPredicates(PhotoAlbumColumns::TABLE);
    rdbPredicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, std::to_string(albumId));
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
        if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
            return false;
        }
    }
    int32_t fileHidden = GetInt32Val(PhotoAlbumColumns::ALBUM_FILE_HIDDEN, resultSet);
    return (fileHidden == (expectedHidden ? 1 : 0));
}

HWTEST_F(AlbumChangeSetHiddenAttributeTest, SetHiddenAttribute_UserAlbum_HiddenTrue_InheritedTrue, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetHiddenAttribute_UserAlbum_HiddenTrue_InheritedTrue");
    
    int32_t ret = SetAlbumHiddenAttribute(g_userAlbumId, true, true,
        static_cast<int32_t>(PhotoAlbumType::USER),
        static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    EXPECT_EQ(ret, E_OK);
    
    EXPECT_TRUE(CheckAlbumHiddenStatus(g_userAlbumId, true));
    EXPECT_TRUE(CheckAssetHiddenStatus(g_userAlbumAssetId, true));

    MEDIA_INFO_LOG("end SetHiddenAttribute_UserAlbum_HiddenTrue_InheritedTrue");
}

HWTEST_F(AlbumChangeSetHiddenAttributeTest, SetHiddenAttribute_UserAlbum_HiddenTrue_InheritedFalse, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetHiddenAttribute_UserAlbum_HiddenTrue_InheritedFalse");
    
    int32_t ret = SetAlbumHiddenAttribute(g_userAlbumId, true, false,
        static_cast<int32_t>(PhotoAlbumType::USER),
        static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    EXPECT_EQ(ret, E_OK);
    
    EXPECT_TRUE(CheckAlbumHiddenStatus(g_userAlbumId, true));
    EXPECT_TRUE(CheckAssetHiddenStatus(g_userAlbumAssetId, false));

    MEDIA_INFO_LOG("end SetHiddenAttribute_UserAlbum_HiddenTrue_InheritedFalse");
}

HWTEST_F(AlbumChangeSetHiddenAttributeTest, SetHiddenAttribute_UserAlbum_HiddenFalse_InheritedTrue, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetHiddenAttribute_UserAlbum_HiddenFalse_InheritedTrue");
    
    int32_t ret = SetAlbumHiddenAttribute(g_userAlbumId, false, true,
        static_cast<int32_t>(PhotoAlbumType::USER),
        static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    EXPECT_EQ(ret, E_OK);
    
    EXPECT_TRUE(CheckAlbumHiddenStatus(g_userAlbumId, false));
    EXPECT_TRUE(CheckAssetHiddenStatus(g_userAlbumAssetId, false));

    MEDIA_INFO_LOG("end SetHiddenAttribute_UserAlbum_HiddenFalse_InheritedTrue");
}

HWTEST_F(AlbumChangeSetHiddenAttributeTest, SetHiddenAttribute_UserAlbum_HiddenFalse_InheritedFalse, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetHiddenAttribute_UserAlbum_HiddenFalse_InheritedFalse");
    
    int32_t ret = SetAlbumHiddenAttribute(g_userAlbumId, false, false,
        static_cast<int32_t>(PhotoAlbumType::USER),
        static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    EXPECT_EQ(ret, E_OK);
    
    EXPECT_TRUE(CheckAlbumHiddenStatus(g_userAlbumId, false));
    EXPECT_TRUE(CheckAssetHiddenStatus(g_userAlbumAssetId, false));

    MEDIA_INFO_LOG("end SetHiddenAttribute_UserAlbum_HiddenFalse_InheritedFalse");
}

HWTEST_F(AlbumChangeSetHiddenAttributeTest, SetHiddenAttribute_SourceAlbum_HiddenTrue_InheritedTrue, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetHiddenAttribute_SourceAlbum_HiddenTrue_InheritedTrue");
    
    int32_t ret = SetAlbumHiddenAttribute(g_sourceAlbumId, true, true,
        static_cast<int32_t>(PhotoAlbumType::SOURCE),
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC));
    EXPECT_EQ(ret, E_OK);
    
    EXPECT_TRUE(CheckAlbumHiddenStatus(g_sourceAlbumId, true));
    EXPECT_TRUE(CheckAssetHiddenStatus(g_sourceAlbumAssetId, true));

    MEDIA_INFO_LOG("end SetHiddenAttribute_SourceAlbum_HiddenTrue_InheritedTrue");
}

HWTEST_F(AlbumChangeSetHiddenAttributeTest, SetHiddenAttribute_FileManagerAlbum_HiddenTrue_InheritedTrue,
    TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetHiddenAttribute_FileManagerAlbum_HiddenTrue_InheritedTrue");
    
    int32_t ret = SetAlbumHiddenAttribute(g_fileManagerAlbumId, true, true,
        static_cast<int32_t>(PhotoAlbumType::SOURCE),
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER));
    EXPECT_EQ(ret, E_OK);
    
    EXPECT_TRUE(CheckAlbumHiddenStatus(g_fileManagerAlbumId, true));
    EXPECT_TRUE(CheckAssetHiddenStatus(g_fileManagerAlbumAssetId, true));

    MEDIA_INFO_LOG("end SetHiddenAttribute_FileManagerAlbum_HiddenTrue_InheritedTrue");
}

HWTEST_F(AlbumChangeSetHiddenAttributeTest, SetHiddenAttribute_InvalidAlbumId, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetHiddenAttribute_InvalidAlbumId");
    
    int32_t ret = SetAlbumHiddenAttribute(-1, true, false,
        static_cast<int32_t>(PhotoAlbumType::USER),
        static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    EXPECT_LT(ret, 0);

    MEDIA_INFO_LOG("end SetHiddenAttribute_InvalidAlbumId");
}

HWTEST_F(AlbumChangeSetHiddenAttributeTest, SetHiddenAttribute_EmptyParcel, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetHiddenAttribute_EmptyParcel");
    
    MessageParcel data;
    MessageParcel reply;
    auto service = make_shared<MediaAlbumsControllerService>();
    service->AlbumChangeSetHiddenAttribute(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);
    ASSERT_LT(respVo.GetErrCode(), 0);

    MEDIA_INFO_LOG("end SetHiddenAttribute_EmptyParcel");
}
} // namespace OHOS::Media

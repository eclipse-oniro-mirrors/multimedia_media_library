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

#include "album_change_set_album_name_by_file_test.h"

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

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace IPC;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;

static int32_t g_userAlbumId;
static int32_t g_sourceAlbumId;
static int32_t g_fileManagerAlbumId;

static std::vector<std::string> createTableSqlLists = {
    PhotoAlbumColumns::CREATE_TABLE,
    PhotoUpgrade::CREATE_PHOTO_TABLE,
};

static std::vector<std::string> testTables = {
    PhotoAlbumColumns::TABLE,
    PhotoColumn::PHOTOS_TABLE,
};

void AlbumChangeSetAlbumNameByFileTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);

    MEDIA_INFO_LOG("AlbumChangeSetAlbumNameByFileTest SetUpTestCase done");
}

void AlbumChangeSetAlbumNameByFileTest::SetUp()
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

    MEDIA_INFO_LOG("AlbumChangeSetAlbumNameByFileTest SetUp");
}

void AlbumChangeSetAlbumNameByFileTest::TearDownTestCase(void)
{
    std::system("rm -rf /storage/media/local/files/Docs/NewFileManagerAlbumName");
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("AlbumChangeSetAlbumNameByFileTest TearDownTestCase");
}

void AlbumChangeSetAlbumNameByFileTest::TearDown() {}

static int32_t SetAlbumNameByFile(int32_t albumId, const string& albumName,
    int32_t albumType, int32_t albumSubType)
{
    AlbumChangeSetAlbumNameByFileReqBody reqBody;
    reqBody.albumId = albumId;
    reqBody.albumName = albumName;
    reqBody.albumType = albumType;
    reqBody.albumSubType = albumSubType;

    MessageParcel data;
    MessageParcel reply;
    bool marshalRet = reqBody.Marshalling(data);
    if (!marshalRet) {
        return -1;
    }

    auto service = make_shared<MediaAlbumsControllerService>();
    service->AlbumChangeSetAlbumNameByFile(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    if (!respVo.Unmarshalling(reply)) {
        return -1;
    }
    int32_t ret = respVo.GetErrCode();
    if (ret == E_OK) {
        return ret;
    }

    vector<string> columns = { PhotoAlbumColumns::ALBUM_ID };
    RdbPredicates rdbPredicates(PhotoAlbumColumns::TABLE);
    rdbPredicates.EqualTo(PhotoAlbumColumns::ALBUM_NAME, albumName);
    rdbPredicates.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, std::to_string(albumType));
    rdbPredicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, std::to_string(albumSubType));
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        return E_OK;
    }
    return ret;
}

static bool IsAlbumExistByName(const string &albumName, int32_t albumType, int32_t albumSubType)
{
    vector<string> columns = { PhotoAlbumColumns::ALBUM_ID };
    RdbPredicates rdbPredicates(PhotoAlbumColumns::TABLE);
    rdbPredicates.EqualTo(PhotoAlbumColumns::ALBUM_NAME, albumName);
    rdbPredicates.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, std::to_string(albumType));
    rdbPredicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, std::to_string(albumSubType));
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    }
    return (resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK);
}

HWTEST_F(AlbumChangeSetAlbumNameByFileTest, SetAlbumNameByFile_UserAlbum_ValidName, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumNameByFile_UserAlbum_ValidName");
    
    string newAlbumName = "NewUserAlbumName";
    int32_t ret = SetAlbumNameByFile(g_userAlbumId, newAlbumName,
        static_cast<int32_t>(PhotoAlbumType::USER),
        static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    EXPECT_EQ(ret, E_OK);

    EXPECT_TRUE(IsAlbumExistByName(newAlbumName,
        static_cast<int32_t>(PhotoAlbumType::USER),
        static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC)));

    MEDIA_INFO_LOG("end SetAlbumNameByFile_UserAlbum_ValidName");
}

HWTEST_F(AlbumChangeSetAlbumNameByFileTest, SetAlbumNameByFile_SourceAlbum_ValidName, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumNameByFile_SourceAlbum_ValidName");
    
    string newAlbumName = "NewSourceAlbumName";
    int32_t ret = SetAlbumNameByFile(g_sourceAlbumId, newAlbumName,
        static_cast<int32_t>(PhotoAlbumType::SOURCE),
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC));
    EXPECT_EQ(ret, E_OK);

    EXPECT_TRUE(IsAlbumExistByName(newAlbumName,
        static_cast<int32_t>(PhotoAlbumType::SOURCE),
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC)));

    MEDIA_INFO_LOG("end SetAlbumNameByFile_SourceAlbum_ValidName");
}

HWTEST_F(AlbumChangeSetAlbumNameByFileTest, SetAlbumNameByFile_FileManagerAlbum_ValidName, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumNameByFile_FileManagerAlbum_ValidName");
    
    string newAlbumName = "NewFileManagerAlbumName";
    int32_t ret = SetAlbumNameByFile(g_fileManagerAlbumId, newAlbumName,
        static_cast<int32_t>(PhotoAlbumType::SOURCE),
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER));
    EXPECT_EQ(ret, E_OK);

    EXPECT_TRUE(IsAlbumExistByName(newAlbumName,
        static_cast<int32_t>(PhotoAlbumType::SOURCE),
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER)));

    MEDIA_INFO_LOG("end SetAlbumNameByFile_FileManagerAlbum_ValidName");
}

HWTEST_F(AlbumChangeSetAlbumNameByFileTest, SetAlbumNameByFile_InvalidAlbumId, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumNameByFile_InvalidAlbumId");
    
    int32_t ret = SetAlbumNameByFile(-1, "TestAlbum",
        static_cast<int32_t>(PhotoAlbumType::USER),
        static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    EXPECT_LT(ret, 0);

    MEDIA_INFO_LOG("end SetAlbumNameByFile_InvalidAlbumId");
}

HWTEST_F(AlbumChangeSetAlbumNameByFileTest, SetAlbumNameByFile_EmptyName, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumNameByFile_EmptyName");
    
    int32_t ret = SetAlbumNameByFile(g_userAlbumId, "",
        static_cast<int32_t>(PhotoAlbumType::USER),
        static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    EXPECT_LT(ret, 0);

    MEDIA_INFO_LOG("end SetAlbumNameByFile_EmptyName");
}

HWTEST_F(AlbumChangeSetAlbumNameByFileTest, SetAlbumNameByFile_LongName, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumNameByFile_LongName");
    
    string longName(256, 'a');
    int32_t ret = SetAlbumNameByFile(g_userAlbumId, longName,
        static_cast<int32_t>(PhotoAlbumType::USER),
        static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    EXPECT_LT(ret, 0);

    MEDIA_INFO_LOG("end SetAlbumNameByFile_LongName");
}

HWTEST_F(AlbumChangeSetAlbumNameByFileTest, SetAlbumNameByFile_EmptyParcel, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumNameByFile_EmptyParcel");
    
    MessageParcel data;
    MessageParcel reply;
    auto service = make_shared<MediaAlbumsControllerService>();
    service->AlbumChangeSetAlbumNameByFile(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);
    ASSERT_LT(respVo.GetErrCode(), 0);

    MEDIA_INFO_LOG("end SetAlbumNameByFile_EmptyParcel");
}
} // namespace OHOS::Media

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

#define MLOG_TAG "MediaAlbumsControllerServiceTest"

#include "album_commit_modify_test.h"

#include <string>
#include <vector>

#define private public
#define protected public
#include "media_albums_controller_service.h"
#undef private
#undef protected

#include "album_commit_modify_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "result_set_utils.h"
#include "media_upgrade.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace IPC;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;

static std::vector<std::string> createTableSqlLists = {
    PhotoAlbumColumns::CREATE_TABLE,
    PhotoUpgrade::CREATE_PHOTO_TABLE,
};

static std::vector<std::string> testTables = {
    PhotoAlbumColumns::TABLE,
    PhotoColumn::PHOTOS_TABLE,
};

void AlbumCommitModifyTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore,  nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    MEDIA_INFO_LOG("SetUpTestCase AlbumCommitModifyTest succeed");
}

void AlbumCommitModifyTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("AlbumCommitModifyTest TearDownTestCase done");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void AlbumCommitModifyTest::SetUp()
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
    MEDIA_INFO_LOG("AlbumCommitModifyTest SetUp");
}

void AlbumCommitModifyTest::TearDown(void) {}

// INSERT INTO PhotoAlbum (album_type, album_subtype, album_name, date_modified, is_local, date_added, lpath, priority)
// VALUES (0, 1, 'test01', 1748354341383, 1 , 1748354341383, '/Pictures/Users/test01', 1)
// (albumType == PhotoAlbumType::USER) && (albumSubType == PhotoAlbumSubType::USER_GENERIC)
static const string SQL_CREATE_ALBUM = "INSERT INTO " + PhotoAlbumColumns::TABLE + "(" +
    PhotoAlbumColumns::ALBUM_TYPE + ", " + PhotoAlbumColumns::ALBUM_SUBTYPE + ", " +
    PhotoAlbumColumns::ALBUM_NAME + ", " + PhotoAlbumColumns::ALBUM_DATE_MODIFIED + ", " +
    PhotoAlbumColumns::ALBUM_IS_LOCAL + ", " + PhotoAlbumColumns::ALBUM_DATE_ADDED + ", " +
    PhotoAlbumColumns::ALBUM_LPATH + ", " + PhotoAlbumColumns::ALBUM_PRIORITY + ")";

static void CreateUserAlbum(const std::string &albumName)
{
    // album_type, album_subtype, album_name, date_modified, is_local, date_added, lpath, priority
    g_rdbStore->ExecuteSql(SQL_CREATE_ALBUM + "VALUES (0, 1, '"+
        albumName + "', 1748354341383, 1 , 1748354341383, '/Pictures/Users/" + albumName + "', 1)");
}

static shared_ptr<NativeRdb::ResultSet> QueryAsset(const string& album_name, const vector<string>& columns)
{
    RdbPredicates rdbPredicates(PhotoAlbumColumns::TABLE);
    rdbPredicates.EqualTo(PhotoAlbumColumns::ALBUM_NAME, album_name);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get file asset");
        return nullptr;
    }
    return resultSet;
}

// {"albumName": "hello","albumType": "0","albumSubType": "1","businessCode": "10",
// "coverUri": file://media/Photo/2/IMG_1748423715_001/IMG_20250528_171335.jpg,"albumId": "12"}
static int32_t ModifyAlbumName(int32_t albumId, int32_t albumType, int32_t albumSubType, const std::string &albumName)
{
    AlbumCommitModifyReqBody reqBody;
    reqBody.albumName = albumName;
    reqBody.albumId = albumId;
    reqBody.albumType = albumType;
    reqBody.albumSubType = albumSubType;
    reqBody.businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_COMMIT_MODIFY);

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAlbumsControllerService>();
    service->AlbumCommitModify(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> resp;
    if (resp.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("resp.Unmarshalling failed");
        return -1;
    }

    return resp.GetErrCode();
}

HWTEST_F(AlbumCommitModifyTest, CommitModify_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CommitModify_Test_001");
    // 1、前置条件准备，创建一个相册
    string albumName = "test01";
    CreateUserAlbum(albumName);

    // 2、查询相册id
    vector<string> columns;
    auto resultSet = QueryAsset(albumName, columns);
    ASSERT_NE(resultSet, nullptr);
    int32_t albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
    EXPECT_GT(albumId, 0);

    // 3、修改指定相册的名称
    int32_t changedRows = ModifyAlbumName(albumId, PhotoAlbumType::USER, PhotoAlbumSubType::USER_GENERIC, "");
    EXPECT_EQ(changedRows, E_INVALID_VALUES);

    changedRows = ModifyAlbumName(albumId, PhotoAlbumType::SYSTEM, PhotoAlbumSubType::USER_GENERIC, "test01-new");
    EXPECT_EQ(changedRows, E_INVALID_VALUES);

    changedRows = ModifyAlbumName(albumId, PhotoAlbumType::USER, PhotoAlbumSubType::SYSTEM_START, "test01-new");
    EXPECT_EQ(changedRows, E_INVALID_VALUES);

    changedRows = ModifyAlbumName(albumId, PhotoAlbumType::USER, PhotoAlbumSubType::USER_GENERIC, "test01-new");
    EXPECT_GT(changedRows, 0);
    MEDIA_INFO_LOG("end CommitModify_Test_001");
}

HWTEST_F(AlbumCommitModifyTest, CommitModify_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CommitModify_Test_002");
    MessageParcel data;
    MessageParcel reply;
    auto service = make_shared<MediaAlbumsControllerService>();
    service->AlbumCommitModify(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> resp;
    ASSERT_EQ(resp.Unmarshalling(reply), true);
    ASSERT_LT(resp.GetErrCode(), 0);
    MEDIA_INFO_LOG("end CommitModify_Test_002");
}
}  // namespace OHOS::Media
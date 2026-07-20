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

#include "get_album_attribute_test.h"

#include <string>
#include <vector>

#define PRIVATE public
#define PROTECTED public
#include "media_albums_controller_service.h"
#include "media_analysis_data_controller_service.h"
#undef PRIVATE
#undef PROTECTED

#include "vision_db_sqls_more.h"
#include "analysis_album_get_attribute_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "result_set_utils.h"
#include "medialibrary_business_code.h"
#include "media_upgrade.h"
#include "medialibrary_client_errno.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace IPC;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;
static string g_albumName = "test01";

static std::vector<std::string> createTableSqlLists = {
    PhotoUpgrade::CREATE_PHOTO_TABLE,
    PhotoAlbumColumns::CREATE_TABLE,
    CREATE_ANALYSIS_ALBUM_FOR_ONCREATE,
};

static std::vector<std::string> testTables = {
    PhotoColumn::PHOTOS_TABLE,
    PhotoAlbumColumns::TABLE,
    ANALYSIS_ALBUM_TABLE,
};

void GetAlbumAttributeTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    MEDIA_INFO_LOG("GetAlbumAttributeTest SetUpTestCase");
}

void GetAlbumAttributeTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("GetAlbumAttributeTest TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void GetAlbumAttributeTest::SetUp()
{
    MEDIA_INFO_LOG("GetAlbumAttributeTest SetUp");
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
}

void GetAlbumAttributeTest::TearDown(void) {}

static const string SQL_CREATE_ALBUM = "INSERT INTO " + ANALYSIS_ALBUM_TABLE + "(" +
    PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
    PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
    PhotoAlbumColumns::ALBUM_COUNT + ", " + EXTRA_INFO + ")";

static void CreateAnalysisAlbum(const std::string &albumName)
{
    int32_t count = g_rdbStore->ExecuteSql("SELECT COUNT(*) FROM " + ANALYSIS_ALBUM_TABLE);
    int32_t albumId = count + 1;
    g_rdbStore->ExecuteSql(SQL_CREATE_ALBUM + "VALUES (" + to_string(albumId) +
        ", 4096, 4102, '"+ albumName + "', 2, 'keycontact:123456789')");
}

static shared_ptr<NativeRdb::ResultSet> QueryAsset(const string& table, const string& key, const string& value,
    const vector<string>& columns)
{
    RdbPredicates rdbPredicates(table);
    rdbPredicates.EqualTo(key, value);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get file asset");
        return nullptr;
    }
    return resultSet;
}

static void GetExtraInfoPrepare(int32_t& albumId)
{
    CreateAnalysisAlbum(g_albumName);
    vector<string> columns;
    auto resultSet = QueryAsset(ANALYSIS_ALBUM_TABLE, PhotoAlbumColumns::ALBUM_NAME, g_albumName, columns);
    if (resultSet == nullptr) {
        return;
    }

    albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
    if (albumId <= 0) {
        return;
    }
}

static int32_t GetAttribute(int32_t albumId, int32_t albumType, int32_t albumSubType,
    std::vector<std::string>& attributeArray)
{
    GetAttributeReqBody reqBody;
    reqBody.albumId = albumId;
    reqBody.albumType = albumType;
    reqBody.albumSubType = albumSubType;
    reqBody.attributeArray = attributeArray;

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAlbumsControllerService>();
    service->GetAnalysisAlbumAttribute(data, reply);

    IPC::MediaRespVo<GetAttributeRespBody> resp;
    if (resp.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("resp.Unmarshalling failed");
        return -1;
    }

    return resp.GetErrCode();
}

HWTEST_F(GetAlbumAttributeTest, GetExtraInfoTest_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetExtraInfoTest_Test_001");
    int32_t albumId = -1;
    GetExtraInfoPrepare(albumId);
    EXPECT_GT(albumId, 0);

    std::vector<std::string> attributeArray = { EXTRA_INFO };
    int32_t ret = GetAttribute(albumId, PhotoAlbumType::SMART, PhotoAlbumSubType::HIGHLIGHT, attributeArray);
    EXPECT_EQ(ret, E_PARAM_CONVERT_FORMAT);

    ret = GetAttribute(albumId, PhotoAlbumType::USER, PhotoAlbumSubType::PORTRAIT, attributeArray);
    EXPECT_EQ(ret, E_PARAM_CONVERT_FORMAT);

    ret = GetAttribute(albumId, PhotoAlbumType::SMART, PhotoAlbumSubType::PORTRAIT, attributeArray);
    EXPECT_EQ(ret, E_OK);

    MEDIA_INFO_LOG("end GetExtraInfoTest_Test_001");
}

HWTEST_F(GetAlbumAttributeTest, GetExtraInfoTest_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetExtraInfoTest_Test_002");
    MessageParcel data;
    MessageParcel reply;
    auto service = make_shared<MediaAlbumsControllerService>();
    service->GetAnalysisAlbumAttribute(data, reply);

    IPC::MediaRespVo<GetAttributeRespBody> resp;
    ASSERT_EQ(resp.Unmarshalling(reply), true);
    ASSERT_LT(resp.GetErrCode(), 0);
    MEDIA_INFO_LOG("end GetExtraInfoTest_Test_002");
}

static const string SQL_CREATE_ALBUM_WITH_FRIEND_ID = "INSERT INTO " + ANALYSIS_ALBUM_TABLE + "(" +
    PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
    PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
    PhotoAlbumColumns::ALBUM_COUNT + ", " + FRIEND_ID + ")";

static void CreateAnalysisAlbumWithFriendId(const std::string &albumName)
{
    int32_t count = g_rdbStore->ExecuteSql("SELECT COUNT(*) FROM " + ANALYSIS_ALBUM_TABLE);
    int32_t albumId = count + 1;
    g_rdbStore->ExecuteSql(SQL_CREATE_ALBUM_WITH_FRIEND_ID + "VALUES (" + to_string(albumId) +
        ", 4096, 4102, '"+ albumName + "', 2, 'friend_circle_001')");
}

static void GetFriendIdPrepare(int32_t& albumId)
{
    CreateAnalysisAlbumWithFriendId(g_albumName);
    vector<string> columns;
    auto resultSet = QueryAsset(ANALYSIS_ALBUM_TABLE, PhotoAlbumColumns::ALBUM_NAME, g_albumName, columns);
    if (resultSet == nullptr) {
        return;
    }

    albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
    if (albumId <= 0) {
        return;
    }
}

HWTEST_F(GetAlbumAttributeTest, GetFriendIdTest_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetFriendIdTest_Test_001");
    int32_t albumId = -1;
    GetFriendIdPrepare(albumId);
    EXPECT_GT(albumId, 0);

    std::vector<std::string> attributeArray = { FRIEND_ID };
    int32_t ret = GetAttribute(albumId, PhotoAlbumType::SMART, PhotoAlbumSubType::HIGHLIGHT, attributeArray);
    EXPECT_EQ(ret, E_PARAM_CONVERT_FORMAT);

    ret = GetAttribute(albumId, PhotoAlbumType::USER, PhotoAlbumSubType::PORTRAIT, attributeArray);
    EXPECT_EQ(ret, E_PARAM_CONVERT_FORMAT);

    ret = GetAttribute(albumId, PhotoAlbumType::SMART, PhotoAlbumSubType::PORTRAIT, attributeArray);
    EXPECT_EQ(ret, E_OK);

    MEDIA_INFO_LOG("end GetFriendIdTest_Test_001");
}

HWTEST_F(GetAlbumAttributeTest, GetFriendIdTest_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetFriendIdTest_Test_002");
    int32_t albumId = -1;
    GetFriendIdPrepare(albumId);
    EXPECT_GT(albumId, 0);

    std::vector<std::string> attributeArray = { FRIEND_ID, EXTRA_INFO };
    int32_t ret = GetAttribute(albumId, PhotoAlbumType::SMART, PhotoAlbumSubType::PORTRAIT, attributeArray);
    EXPECT_EQ(ret, E_OK);

    MEDIA_INFO_LOG("end GetFriendIdTest_Test_002");
}

HWTEST_F(GetAlbumAttributeTest, GetFriendIdTest_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetFriendIdTest_Test_003");
    int32_t albumId = -1;
    GetFriendIdPrepare(albumId);
    EXPECT_GT(albumId, 0);

    std::vector<std::string> attributeArray = { "invalid_attr" };
    int32_t ret = GetAttribute(albumId, PhotoAlbumType::SMART, PhotoAlbumSubType::PORTRAIT, attributeArray);
    EXPECT_LT(ret, 0);

    MEDIA_INFO_LOG("end GetFriendIdTest_Test_003");
}
}  // namespace OHOS::Media
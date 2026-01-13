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

#include "get_relationship_test.h"

#include <string>
#include <vector>

#define private public
#define protected public
#include "media_albums_controller_service.h"
#include "media_analysis_data_controller_service.h"
#undef private
#undef protected

#include "vision_db_sqls_more.h"
#include "get_relationship_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "result_set_utils.h"
#include "medialibrary_business_code.h"
#include "media_upgrade.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace IPC;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;
const std::string RELATIONSHIP = "relationship";
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

void GetRelationshipTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    MEDIA_INFO_LOG("GetRelationshipTest SetUpTestCase");
}

void GetRelationshipTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("GetRelationshipTest TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void GetRelationshipTest::SetUp()
{
    MEDIA_INFO_LOG("GetRelationshipTest SetUp");
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
}

void GetRelationshipTest::TearDown(void) {}

static const string SQL_CREATE_ALBUM = "INSERT INTO " + ANALYSIS_ALBUM_TABLE + "(" +
    PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
    PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
    PhotoAlbumColumns::ALBUM_COUNT + ", " + RELATIONSHIP + ")";

static void CreateAnalysisAlbum(const std::string &albumName)
{
    int32_t count = g_rdbStore->ExecuteSql("SELECT COUNT(*) FROM " + ANALYSIS_ALBUM_TABLE);
    int32_t albumId = count + 1;
    g_rdbStore->ExecuteSql(SQL_CREATE_ALBUM + "VALUES (" + to_string(albumId) +
        ", 4096, 4102, '"+ albumName + "', 2, 'mother')");
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

static void GetRelationshipPrepare(int32_t& albumId)
{
    // 1、创建AnalysisAlbum
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

static int32_t GetRelationship(int32_t albumId, int32_t albumType, int32_t albumSubType)
{
    GetRelationshipReqBody reqBody;
    reqBody.albumId = albumId;
    reqBody.albumType = albumType;
    reqBody.albumSubType = albumSubType;

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<AnalysisData::MediaAnalysisDataControllerService>();
    service->GetRelationship(data, reply);

    IPC::MediaRespVo<GetRelationshipRespBody> resp;
    if (resp.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("resp.Unmarshalling failed");
        return -1;
    }

    return resp.GetErrCode();
}

HWTEST_F(GetRelationshipTest, GetRelationshipTest_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetRelationshipTest_Test_001");
    // 1、前置条件准备
    int32_t albumId = -1;
    GetRelationshipPrepare(albumId);
    EXPECT_GT(albumId, 0);

    // 2、查询人物关系
    int32_t ret = GetRelationship(albumId, PhotoAlbumType::SMART, PhotoAlbumSubType::HIGHLIGHT);
    EXPECT_EQ(ret, E_INVALID_VALUES);

    ret = GetRelationship(albumId, PhotoAlbumType::USER, PhotoAlbumSubType::PORTRAIT);
    EXPECT_EQ(ret, E_INVALID_VALUES);

    ret = GetRelationship(albumId, PhotoAlbumType::SMART, PhotoAlbumSubType::PORTRAIT);
    EXPECT_EQ(ret, E_OK);

    MEDIA_INFO_LOG("end GetRelationshipTest_Test_001");
}

HWTEST_F(GetRelationshipTest, GetRelationshipTest_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetRelationshipTest_Test_002");
    MessageParcel data;
    MessageParcel reply;
    auto service = make_shared<AnalysisData::MediaAnalysisDataControllerService>();
    service->GetRelationship(data, reply);

    IPC::MediaRespVo<GetRelationshipRespBody> resp;
    ASSERT_EQ(resp.Unmarshalling(reply), true);
    ASSERT_LT(resp.GetErrCode(), 0);
    MEDIA_INFO_LOG("end GetRelationshipTest_Test_002");
}
}  // namespace OHOS::Media
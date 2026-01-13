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

#include "delete_highlight_albums_test.h"

#include <string>
#include <vector>

#define private public
#define protected public
#include "media_albums_controller_service.h"
#include "media_analysis_data_controller_service.h"
#undef private
#undef protected

#include "message_parcel.h"
#include "user_define_ipc_client.h"
#include "delete_highlight_albums_vo.h"
#include "vision_column.h"
#include "vision_column_comm.h"
#include "story_db_sqls.h"
#include "medialibrary_rdbstore.h"
#include "vision_db_sqls_more.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace IPC;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static const string INSERT_HIGHLIGHT_ALBUM_TABLE = "INSERT INTO " + HIGHLIGHT_ALBUM_TABLE + "(id, " + ALBUM_ID + ", " +
    AI_ALBUM_ID + ", " + SUB_TITLE + ", " + CLUSTER_TYPE + ", " + CLUSTER_SUB_TYPE + ", " + CLUSTER_CONDITION + ", " +
    MIN_DATE_ADDED + ", " + MAX_DATE_ADDED + ", " + GENERATE_TIME + ", " + HIGHLIGHT_VERSION + ", " +
    HIGHLIGHT_STATUS + ", " + HIGHLIGHT_INSERT_PIC_COUNT + ", " + HIGHLIGHT_REMOVE_PIC_COUNT + ", " +
    HIGHLIGHT_SHARE_SCREENSHOT_COUNT + ", " + HIGHLIGHT_SHARE_COVER_COUNT + ", " + HIGHLIGHT_RENAME_COUNT + ", " +
    HIGHLIGHT_CHANGE_COVER_COUNT + ", " + HIGHLIGHT_RENDER_VIEWED_TIMES + ", " +
    HIGHLIGHT_RENDER_VIEWED_DURATION + ", " + HIGHLIGHT_ART_LAYOUT_VIEWED_TIMES + ", " +
    HIGHLIGHT_ART_LAYOUT_VIEWED_DURATION + ", " + HIGHLIGHT_MUSIC_EDIT_COUNT + ", " +
    HIGHLIGHT_FILTER_EDIT_COUNT + ") ";
static const string VALUES_END = ") ";
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;

static std::vector<std::string> createTableSqlLists = {
    PhotoAlbumColumns::CREATE_TABLE,
    CREATE_ANALYSIS_ALBUM,
    CREATE_HIGHLIGHT_ALBUM_TABLE,
};

static std::vector<std::string> testTables = {
    PhotoAlbumColumns::TABLE,
    ANALYSIS_ALBUM_TABLE,
    HIGHLIGHT_ALBUM_TABLE,
};

void InsertHighlightAlbums()
{
    // highlight_status = 0
    g_rdbStore->ExecuteSql(INSERT_HIGHLIGHT_ALBUM_TABLE + " VALUES ( 1, 1, 2, '2024.05.22', 'TYPE_DBSCAN', "
        "'Old_AOI_0', '[]', 1716307200000, 1716392070000, 1738992083745, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0" +
        VALUES_END);
}

void DeleteHighlightAlbumsTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    MEDIA_INFO_LOG("DeleteHighlightAlbumsTest SetUpTestCase succeed");
}

void DeleteHighlightAlbumsTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("DeleteHighlightAlbumsTest TearDownTestCase");
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

void DeleteHighlightAlbumsTest::SetUp()
{
    MEDIA_INFO_LOG("DeleteHighlightAlbumsTest SetUp");
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
}

void DeleteHighlightAlbumsTest::TearDown(void) {}

bool QueryHighlightAlbumInfo()
{
    NativeRdb::RdbPredicates rdbPredicate(INSERT_HIGHLIGHT_ALBUM_TABLE);
    string id = "id";
    string idValue = "1";
    rdbPredicate.EqualTo(id, idValue);
    vector<string> columns;
    shared_ptr<NativeRdb::ResultSet> resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    if (resultSet != nullptr && resultSet->GoToFirstRow() != 0) {
        return true;
    }
    return false;
}

HWTEST_F(DeleteHighlightAlbumsTest, DeleteHighlightAlbums_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start DeleteHighlightAlbums_Test_001");
    InsertHighlightAlbums();
    bool ret = QueryHighlightAlbumInfo();
    ASSERT_EQ(ret, true);
    MessageParcel data;
    MessageParcel reply;
    DeleteHighLightAlbumsReqBody reqBody;
    vector<string> albumIds = { "1" };
    vector<int32_t> photoAlbumTypes = { static_cast<int32_t>(PhotoAlbumType::SMART) };
    vector<int32_t> photoAlbumSubtypes = { static_cast<int32_t>(PhotoAlbumSubType::HIGHLIGHT) };
    reqBody.albumIds = albumIds;
    reqBody.photoAlbumTypes = photoAlbumTypes;
    reqBody.photoAlbumSubtypes = photoAlbumSubtypes;
    bool errConn = !reqBody.Marshalling(data);
    ASSERT_EQ(errConn, false);
    auto service = make_shared<AnalysisData::MediaAnalysisDataControllerService>();
    service->DeleteHighlightAlbums(data, reply);
    MediaEmptyObjVo respVo;
    MediaRespVo<MediaEmptyObjVo> resp;
    bool isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    int32_t err = resp.GetErrCode();
    ASSERT_EQ(err, 1);
    MEDIA_INFO_LOG("End DeleteHighlightAlbums_Test_001");
}

HWTEST_F(DeleteHighlightAlbumsTest, DeleteHighlightAlbums_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start DeleteHighlightAlbums_Test_002");
    InsertHighlightAlbums();
    bool ret = QueryHighlightAlbumInfo();
    ASSERT_EQ(ret, true);
    MessageParcel data;
    MessageParcel reply;
    MediaReqVo<DeleteHighLightAlbumsReqBody> reqVo;
    DeleteHighLightAlbumsReqBody reqBody;
    reqBody.albumIds = { "1" };
    reqBody.photoAlbumTypes = { static_cast<int32_t>(PhotoAlbumType::SMART) };
    reqVo.SetBody(reqBody);
    bool errConn = !reqVo.Marshalling(data);
    ASSERT_EQ(errConn, false);
    auto service = make_shared<AnalysisData::MediaAnalysisDataControllerService>();
    service->DeleteHighlightAlbums(data, reply);
    MediaEmptyObjVo respVo;
    MediaRespVo<MediaEmptyObjVo> resp;
    bool isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    int32_t err = resp.GetErrCode();
    ASSERT_LT(err, 0);
    MEDIA_INFO_LOG("End DeleteHighlightAlbums_Test_002");
}
}  // namespace OHOS::Media
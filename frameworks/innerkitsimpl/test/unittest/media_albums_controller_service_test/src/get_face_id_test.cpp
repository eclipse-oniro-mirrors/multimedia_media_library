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

#include "get_face_id_test.h"

#include <string>
#include <vector>

#define private public
#define protected public
#include "media_albums_controller_service.h"
#include "media_analysis_data_controller_service.h"
#undef private
#undef protected

#include "get_face_id_vo.h"

#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "result_set_utils.h"
#include "media_file_uri.h"
#include "vision_db_sqls_more.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using OHOS::DataShare::DataSharePredicates;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;
static const string SQL_INSERT_PHOTO =
    "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "(" + MediaColumn::MEDIA_FILE_PATH + ", " + MediaColumn::MEDIA_SIZE +
    ", " + MediaColumn::MEDIA_TITLE + ", " + MediaColumn::MEDIA_NAME + ", " + MediaColumn::MEDIA_TYPE + ", " +
    MediaColumn::MEDIA_OWNER_PACKAGE + ", " + MediaColumn::MEDIA_PACKAGE_NAME + ", " + MediaColumn::MEDIA_DATE_ADDED +
    ", " + MediaColumn::MEDIA_DATE_MODIFIED + ", " + MediaColumn::MEDIA_DATE_TAKEN + ", " +
    MediaColumn::MEDIA_DURATION + ", " + MediaColumn::MEDIA_IS_FAV + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
    MediaColumn::MEDIA_HIDDEN + ", " + PhotoColumn::PHOTO_HEIGHT + ", " + PhotoColumn::PHOTO_WIDTH + ", " +
    PhotoColumn::PHOTO_EDIT_TIME + ", " + PhotoColumn::PHOTO_SHOOTING_MODE + ")";
static const string VALUES_END = ") ";

static std::vector<std::string> createTableSqlLists = {
    PhotoColumn::CREATE_PHOTO_TABLE,
    PhotoAlbumColumns::CREATE_TABLE,
    CREATE_ANALYSIS_ALBUM_FOR_ONCREATE,
};

static std::vector<std::string> testTables = {
    PhotoColumn::PHOTOS_TABLE,
    PhotoAlbumColumns::TABLE,
    ANALYSIS_ALBUM_TABLE,
};

void GetFaceIdTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    MEDIA_INFO_LOG("GetFaceIdTest SetUpTestCase");
}

void GetFaceIdTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("GetFaceIdTest TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void GetFaceIdTest::SetUp()
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
    MEDIA_INFO_LOG("GetFaceIdTest SetUp");
}

void GetFaceIdTest::TearDown(void) {}

static int32_t GetFaceId(int32_t albumId, int32_t albumSubType, string &groupTag)
{
    GetFaceIdReqBody reqBody;
    reqBody.albumId = albumId;
    reqBody.albumSubType = albumSubType;

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<AnalysisData::MediaAnalysisDataControllerService>();
    service->GetFaceId(data, reply);

    IPC::MediaRespVo<GetFaceIdRespBody> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    int32_t errCode = respVo.GetErrCode();
    if (errCode != 0) {
        MEDIA_ERR_LOG("respVo.GetErrCode: %{public}d", errCode);
        return errCode;
    }
    groupTag = respVo.GetBody().groupTag;
    return 0;
}

static void InsertAnalysisAlbum()
{
    std::string insertSql = "INSERT INTO AnalysisAlbum (album_name, album_subtype, group_tag) VALUES "
                            "('newalbum', 4103, 'success')";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute sql %{public}s failed", insertSql.c_str());
    }
}

static int32_t QueryAlbumIdByAlbumName(const string &albumName)
{
    vector<string> columns;
    RdbPredicates rdbPredicates("AnalysisAlbum");
    rdbPredicates.EqualTo("album_name", albumName);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get fileId");
        return -1;
    }
    int32_t albumId = GetInt32Val("album_id", resultSet);
    return albumId;
}

HWTEST_F(GetFaceIdTest, QueryCloudEnhancementTaskState_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("QueryCloudEnhancementTaskState_Test_001 Begin");
    // invalid albumId
    string groupTag;
    int32_t result = GetFaceId(100000, PhotoAlbumSubType::GROUP_PHOTO, groupTag);
    ASSERT_NE(result, 0);
}

HWTEST_F(GetFaceIdTest, QueryCloudEnhancementTaskState_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("QueryCloudEnhancementTaskState_Test_002 Begin");
    // invalid albumSubType
    InsertAnalysisAlbum();
    int32_t albumId = QueryAlbumIdByAlbumName("newalbum");
    string groupTag;
    int32_t result = GetFaceId(albumId, PhotoAlbumSubType::ANALYSIS_END, groupTag);
    ASSERT_NE(result, 0);
}

HWTEST_F(GetFaceIdTest, QueryCloudEnhancementTaskState_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("QueryCloudEnhancementTaskState_Test_003 Begin");
    InsertAnalysisAlbum();
    int32_t albumId = QueryAlbumIdByAlbumName("newalbum");
    MEDIA_INFO_LOG("albumId %{public}d", albumId);
    string groupTag;
    int32_t result = GetFaceId(albumId, PhotoAlbumSubType::GROUP_PHOTO, groupTag);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(groupTag, "success");
}
}  // namespace OHOS::Media
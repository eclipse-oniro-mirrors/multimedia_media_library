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
 
#include "get_data_analysis_process_test.h"
 
#include <string>
#include <vector>
 
#define private public
#define protected public
#include "datashare_result_set.h"
#include "media_albums_controller_service.h"
#include "rdb_utils.h"
#include "media_analysis_data_controller_service.h"
#undef private
#undef protected
 
#include "media_column.h"
#include "vision_db_sqls.h"
#include "story_db_sqls.h"
#include "media_file_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "result_set_utils.h"
#include "story_album_column.h"
#include "user_define_ipc_client.h"
#include "user_photography_info_column.h"
#include "vision_column.h"
#include "get_analysis_process_vo.h"
#include "query_result_vo.h"
#include "media_upgrade.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
 
static shared_ptr<MediaLibraryRdbStore> g_rdbStore = nullptr;
static constexpr int32_t SLEEP_SECONDS = 1;

static std::vector<std::string> createTableSqlLists = {
    PhotoUpgrade::CREATE_PHOTO_TABLE,
    CREATE_USER_PHOTOGRAPHY_INFO_TABLE,
    CREATE_HIGHLIGHT_ALBUM_TABLE,
    CREATE_TAB_ANALYSIS_TOTAL_FOR_ONCREATE,
};

static std::vector<std::string> testTables = {
    PhotoColumn::PHOTOS_TABLE,
    VISION_TOTAL_TABLE,
    USER_PHOTOGRAPHY_INFO_TABLE,
    HIGHLIGHT_ALBUM_TABLE,
};
 
void GetDataAnalysisProcessTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    MEDIA_INFO_LOG("GetDataAnalysisProcessTest SetUpTestCase");
}
 
void GetDataAnalysisProcessTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
    MEDIA_INFO_LOG("GetDataAnalysisProcessTest TearDownTestCase");
}
 
void GetDataAnalysisProcessTest::SetUp()
{
    MEDIA_INFO_LOG("GetDataAnalysisProcessTest SetUp");
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
}
 
void GetDataAnalysisProcessTest::TearDown(void) {}
 
static void InsertAssetIntoPhotosTable(const std::string &filePath)
{
    const std::string sqlInsertPhoto = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "("  +
        MediaColumn::MEDIA_FILE_PATH + ", " + MediaColumn::MEDIA_SIZE + ", " + MediaColumn::MEDIA_TITLE + ", " +
        MediaColumn::MEDIA_NAME + ", " + MediaColumn::MEDIA_TYPE + ", " + MediaColumn::MEDIA_OWNER_PACKAGE + ", " +
        MediaColumn::MEDIA_PACKAGE_NAME + ", " + MediaColumn::MEDIA_DATE_ADDED + ", "  +
        MediaColumn::MEDIA_DATE_MODIFIED + ", " + MediaColumn::MEDIA_DATE_TAKEN + ", " +
        MediaColumn::MEDIA_DURATION + ", " + MediaColumn::MEDIA_IS_FAV + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
        MediaColumn::MEDIA_HIDDEN + ", " + PhotoColumn::PHOTO_HEIGHT + ", " + PhotoColumn::PHOTO_WIDTH + ", " +
        PhotoColumn::PHOTO_EDIT_TIME + ", " + PhotoColumn::PHOTO_SHOOTING_MODE + ")";
    g_rdbStore->ExecuteSql(sqlInsertPhoto + "VALUES (" +
        "'" + filePath + "', 175258, 'cam_pic', 'cam_pic.jpg', 1, " +
        "'com.ohos.camera', '相机', 1501924205218, 0, 1501924205, 0, 0, 0, 0, " +
        "1280, 960, 0, '1' )");
}

static void InsertTabAnalysisTotal()
{
    int ret = g_rdbStore->ExecuteSql(
        "INSERT INTO tab_analysis_total "
        "(file_id, ocr, label, aesthetics_score, face, segmentation, saliency, head) "
        "VALUES (100001, 1, 1, 1, 1, 1, 1, 1)");
    std::cout << "InsertTabAnalysisTotal, ret = " << ret << std::endl;
}

static void InsertTabUserPhotographyInfo()
{
    int ret = g_rdbStore->ExecuteSql(
        "insert into tab_user_photography_info(highlight_analysis_progress) values"
        "(\"{'cvFinishedCount':11097,'geoFinishedCount':13164,'searchFinishedCount':6610,'totalCount':11631}\");");
    std::cout << "InsertTabUserPhotographyInfo, ret = " << ret << std::endl;
}

// HIGHLIGHT_ALBUM_TABLE tab_highlight_album
static void InsertHighLightAlbumTable()
{
    int ret = g_rdbStore->ExecuteSql(
        "insert into tab_highlight_album"
        "(highlight_status, cluster_type, cluster_sub_type, cluster_condition, highlight_version) "
        "values(1, 'TYPE_DBSCAN', 'DBScan_Default', '[{\"end\":\"1538816165000\",\"start\":\"1538814215000\"}]', 3);");
    std::cout << "InsertHighLightAlbumTable, ret = " << ret << std::endl;
}

HWTEST_F(GetDataAnalysisProcessTest, GetLabelAnalysisProgress_Test_001, TestSize.Level0)
{
    bool createDirRes = MediaFileUtils::CreateDirectory("/data/local/tmp");
    std::string filePath = "/data/local/tmp/IMG_1501924305_001.jpg";
    if (!MediaFileUtils::IsFileExists(filePath)) {
        EXPECT_EQ(MediaFileUtils::CreateFile(filePath), true);
    }
    InsertAssetIntoPhotosTable(filePath);

    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(MediaColumn::MEDIA_FILE_PATH, filePath);
    std::vector<std::string> columns = { MediaColumn::MEDIA_ID };
 
    shared_ptr<NativeRdb::ResultSet> resSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    EXPECT_NE(resSet, nullptr);
    EXPECT_EQ(resSet->GoToNextRow(), NativeRdb::E_OK);
    EXPECT_GT(GetInt32Val(MediaColumn::MEDIA_ID, resSet), 0);

    string clause = VISION_TOTAL_TABLE + "." + MediaColumn::MEDIA_ID + " = " + PhotoColumn::PHOTOS_TABLE+ "." +
        MediaColumn::MEDIA_ID;
    GetAnalysisProcessReqBody reqBody;
    reqBody.analysisType = AnalysisType::ANALYSIS_LABEL;
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    auto service = make_shared<AnalysisData::MediaAnalysisDataControllerService>();
    service->GetAnalysisProcess(data, reply);
    IPC::MediaRespVo<QueryResultRespBody> resp;
    bool isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    EXPECT_EQ(resp.GetErrCode(), E_SUCCESS);
    auto resultSet = resp.GetBody().resultSet;
    int rowsCount = 0;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        ++rowsCount;
    }
    EXPECT_EQ(rowsCount, 1);
}

HWTEST_F(GetDataAnalysisProcessTest, GetFaceAnalysisProgress_TEST_001, TestSize.Level0)
{
    InsertTabUserPhotographyInfo();
    RdbPredicates predicates(USER_PHOTOGRAPHY_INFO_TABLE);
    std::vector<std::string> columns = { HIGHLIGHT_ANALYSIS_PROGRESS };
    auto resSet = MediaLibraryRdbStore::Query(predicates, columns);
    EXPECT_NE(resSet, nullptr);
    int rowsCount = 0;
    while (resSet->GoToNextRow() == NativeRdb::E_OK) {
        for (int i = 0; i < columns.size(); ++i) {
            std::string val = "";
            resSet->GetString(i, val);
            std::cout << columns[i] << "=" << val << std::endl;
        }
        ++rowsCount;
    }
    EXPECT_EQ(rowsCount, 1);
}

HWTEST_F(GetDataAnalysisProcessTest, GetFaceAnalysisProgress_TEST_002, TestSize.Level0)
{
    InsertTabUserPhotographyInfo();
    std::vector<std::string> columns = { HIGHLIGHT_ANALYSIS_PROGRESS };
    GetAnalysisProcessReqBody reqBody;
    DataShare::DataSharePredicates dataSharePredicates;
    reqBody.analysisType = AnalysisType::ANALYSIS_FACE;
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    auto service = make_shared<AnalysisData::MediaAnalysisDataControllerService>();
    service->GetAnalysisProcess(data, reply);
    IPC::MediaRespVo<QueryResultRespBody> resp;
    bool isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    EXPECT_EQ(resp.GetErrCode(), E_SUCCESS);
    auto resultSet = resp.GetBody().resultSet;
    int rowsCount = 0;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        for (int i = 0; i < columns.size(); ++i) {
            std::string val = "";
            resultSet->GetString(i, val);
            std::cout << columns[i] << "=" << val << std::endl;
        }
        ++rowsCount;
    }
    EXPECT_EQ(rowsCount, 1);
}

HWTEST_F(GetDataAnalysisProcessTest, GetHighlightAnalysisProgressTest_TEST_001, TestSize.Level0)
{
    InsertHighLightAlbumTable();
    RdbPredicates predicates(HIGHLIGHT_ALBUM_TABLE);
    std::vector<std::string> columns = {
        "SUM(CASE WHEN highlight_status = -3 THEN 1 ELSE 0 END) AS ClearCount",
        "SUM(CASE WHEN highlight_status = -2 THEN 1 ELSE 0 END) AS DeleteCount",
        "SUM(CASE WHEN highlight_status = -1 THEN 1 ELSE 0 END) AS NotProduceCount",
        "SUM(CASE WHEN highlight_status > 0 THEN 1 ELSE 0 END) AS ProduceCount",
        "SUM(CASE WHEN highlight_status = 1 THEN 1 ELSE 0 END) AS PushCount",
    };
    auto resSet = MediaLibraryRdbStore::Query(predicates, columns);
    EXPECT_NE(resSet, nullptr);
    int rowsCount = 0;
    while (resSet->GoToNextRow() == NativeRdb::E_OK) {
        for (int i = 0; i < columns.size(); ++i) {
            std::string val = "";
            resSet->GetString(i, val);
            std::cout << columns[i] << "=" << val << std::endl;
        }
        ++rowsCount;
    }
    EXPECT_EQ(rowsCount, 1);
}

HWTEST_F(GetDataAnalysisProcessTest, GetHighlightAnalysisProgressTest_TEST_002, TestSize.Level0)
{
    InsertHighLightAlbumTable();
    DataShare::DataSharePredicates dataSharePredicates;
    GetAnalysisProcessReqBody reqBody;
    std::vector<std::string> columns = {
        "SUM(CASE WHEN highlight_status = -3 THEN 1 ELSE 0 END) AS ClearCount",
        "SUM(CASE WHEN highlight_status = -2 THEN 1 ELSE 0 END) AS DeleteCount",
        "SUM(CASE WHEN highlight_status = -1 THEN 1 ELSE 0 END) AS NotProduceCount",
        "SUM(CASE WHEN highlight_status > 0 THEN 1 ELSE 0 END) AS ProduceCount",
        "SUM(CASE WHEN highlight_status = 1 THEN 1 ELSE 0 END) AS PushCount",
    };
    reqBody.analysisType = AnalysisType::ANALYSIS_HIGHLIGHT;
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    auto service = make_shared<AnalysisData::MediaAnalysisDataControllerService>();
    service->GetAnalysisProcess(data, reply);
    IPC::MediaRespVo<QueryResultRespBody> resp;
    bool isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    EXPECT_EQ(resp.GetErrCode(), E_SUCCESS);
    auto resultSet = resp.GetBody().resultSet;
    int rowsCount = 0;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        for (int i = 0; i < columns.size(); ++i) {
            int val = 0;
            resultSet->GetInt(i, val);
            std::cout << columns[i] << "=" << val << std::endl;
            if (i == columns.size() - 1) {
                EXPECT_EQ(val, 1);
            }
        }
        ++rowsCount;
    }
    EXPECT_EQ(rowsCount, 1);
}
}  // namespace OHOS::Media
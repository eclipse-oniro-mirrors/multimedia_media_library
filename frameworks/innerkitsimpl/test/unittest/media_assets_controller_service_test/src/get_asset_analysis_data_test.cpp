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

#define MLOG_TAG "MediaAssetsControllerServiceTest"

#include "get_asset_analysis_data_test.h"

#include <memory>
#include <string>

#include "media_assets_controller_service.h"
#include "media_analysis_data_controller_service.h"

#include "get_asset_analysis_data_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_asset_operations.h"
#include "media_file_utils.h"
#include "mimetype_utils.h"
#include "parameter_utils.h"
#include "vision_total_column.h"
#include "vision_db_sqls.h"
#include "vision_aesthetics_score_column.h"
#include "media_upgrade.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 3;

static std::string Quote(const std::string &str)
{
    return "'" + str + "'";
}

static std::vector<std::string> createTableSqlLists = {
    CREATE_TAB_ANALYSIS_TOTAL_FOR_ONCREATE,
    PhotoUpgrade::CREATE_PHOTO_TABLE,
};

static std::vector<std::string> testTables = {
    VISION_TOTAL_TABLE,
    PhotoColumn::PHOTOS_TABLE,
};

static void ClearAssetsFile()
{
    std::string assetPath;
    vector<string> columns = {MediaColumn::MEDIA_FILE_PATH};
    NativeRdb::RdbPredicates rdbPredicate(PhotoColumn::PHOTOS_TABLE);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "MediaLibraryRdbStore::Query failed");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        assetPath = MediaLibraryRdbStore::GetString(resultSet, columns.front());
        MEDIA_INFO_LOG("DeleteFile assetPath:%{public}s", assetPath.c_str());
        MediaFileUtils::DeleteFile(assetPath);
    }
    resultSet->Close();
}

static void InsertAsset(const std::string &displayName, int32_t pending = 0)
{
    MEDIA_INFO_LOG("displayName:%{public}s pending:%{public}d", displayName.c_str(), pending);

    std::string ext;
    std::string title;
    int32_t errCode = ParameterUtils::GetTitleAndExtension(displayName, title, ext);
    CHECK_AND_RETURN_LOG(errCode == E_OK, "GetTitleAndExtension errCode:%{public}d", errCode);

    std::string assetPath;
    int64_t now = MediaFileUtils::UTCTimeMilliSeconds();
    string mimeType = MimeTypeUtils::GetMimeTypeFromExtension(ext);
    int32_t mediaType = MimeTypeUtils::GetMediaTypeFromMimeType(mimeType);
    int32_t id = (now > 0 && now <= INT32_MAX) ? static_cast<int32_t>(now) : 1;
    errCode = MediaLibraryAssetOperations::CreateAssetPathById(id, mediaType, ext, assetPath);
    CHECK_AND_RETURN_LOG(errCode == E_OK, "CreateAssetPathById errCode:%{public}d", errCode);

    std::vector<std::pair<std::string, std::string>> items = {
        {MediaColumn::MEDIA_FILE_PATH, Quote(assetPath)}, {MediaColumn::MEDIA_SIZE, "175258"},
        {MediaColumn::MEDIA_TITLE, Quote(title)}, {MediaColumn::MEDIA_NAME, Quote(displayName)},
        {MediaColumn::MEDIA_TYPE, to_string(mediaType)},
        {MediaColumn::MEDIA_OWNER_PACKAGE, Quote("com.ohos.camera")}, {MediaColumn::MEDIA_PACKAGE_NAME, Quote("相机")},
        {MediaColumn::MEDIA_DATE_ADDED, to_string(now)}, {MediaColumn::MEDIA_DATE_MODIFIED, "0"},
        {MediaColumn::MEDIA_DATE_TAKEN, to_string(now)}, {MediaColumn::MEDIA_DURATION, "0"},
        {MediaColumn::MEDIA_TIME_PENDING, to_string(pending)},
        {PhotoColumn::PHOTO_HEIGHT, "1280"}, {PhotoColumn::PHOTO_WIDTH, "960"},
        {PhotoColumn::PHOTO_SHOOTING_MODE, "'1'"},
    };

    std::string values;
    std::string columns;
    for (const auto &item : items) {
        if (!columns.empty()) {
            columns.append(",");
            values.append(",");
        }
        columns.append(item.first);
        values.append(item.second);
    }
    std::string sql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "(" + columns + ") VALUES (" + values + ")";
    errCode = g_rdbStore->ExecuteSql(sql);
    CHECK_AND_RETURN_LOG(errCode == E_OK, "ExecuteSql errCode:%{public}d", errCode);

    MEDIA_INFO_LOG("CreateFile assetPath:%{public}s", assetPath.c_str());
    MediaFileUtils::CreateFile(assetPath);
}

static void InsertAnalysisTotal(int32_t fileId)
{
    std::vector<std::pair<std::string, std::string>> items = {
        {MediaColumn::MEDIA_ID, to_string(fileId)},
        {STATUS, "0"}, {OCR, "0"}, {LABEL, "0"}, {AESTHETICS_SCORE, "0"}, {FACE, "0"},
        {OBJECT, "0"}, {RECOMMENDATION, "0"}, {SEGMENTATION, "0"}, {COMPOSITION, "0"},
        {SALIENCY, "0"}, {HEAD, "0"}, {POSE, "0"}, {GEO, "0"},
        {SELECTED, "0"}, {NEGATIVE, "0"}, {ABSTRACT_NODE_ANALYSIS, "0"},
    };

    std::string values;
    std::string columns;
    for (const auto &item : items) {
        if (!columns.empty()) {
            columns.append(",");
            values.append(",");
        }
        columns.append(item.first);
        values.append(item.second);
    }
    std::string sql = "INSERT INTO " + VISION_TOTAL_TABLE + "(" + columns + ") VALUES (" + values + ")";
    int32_t errCode = g_rdbStore->ExecuteSql(sql);
    CHECK_AND_RETURN_LOG(errCode == E_OK, "ExecuteSql errCode:%{public}d", errCode);
}

static int32_t GetAssetId(const std::string &displayName)
{
    NativeRdb::RdbPredicates rdbPredicate(PhotoColumn::PHOTOS_TABLE);
    rdbPredicate.EqualTo(MediaColumn::MEDIA_NAME, displayName);
    vector<string> columns = { MediaColumn::MEDIA_ID };
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query failed, displayName:%{public}s", displayName.c_str());
        return 0;
    }
    int32_t assetId = 0;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        assetId = MediaLibraryRdbStore::GetInt(resultSet, MediaColumn::MEDIA_ID);
        MEDIA_INFO_LOG("resultSet: assetId:%{public}d", assetId);
    }
    resultSet->Close();
    return assetId;
}

static void ShowResultSet(std::shared_ptr<DataShare::DataShareResultSet> resultSet)
{
    std::vector<std::string> columns;
    resultSet->GetAllColumnNames(columns);
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string tag;
        std::string rowData;
        for (size_t i = 0; i < columns.size(); i++) {
            std::string value;
            resultSet->GetString(i, value);
            rowData += tag + columns[i] + ":'" + value + "'";
            tag = ",";
        }
        MEDIA_INFO_LOG("rowData:[%{public}s]", rowData.c_str());
    }
}

void GetAssetAnalysisDataTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    // clear table  if existed
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    ClearAssetsFile();

    InsertAsset("GetAssetAnalysisData_Test.jpg");
    InsertAnalysisTotal(GetAssetId("GetAssetAnalysisData_Test.jpg"));
    MEDIA_INFO_LOG("SetUpTestCase");
}

void GetAssetAnalysisDataTest::TearDownTestCase(void)
{
    ClearAssetsFile();
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void GetAssetAnalysisDataTest::SetUp() {}

void GetAssetAnalysisDataTest::TearDown(void) {}

int32_t GetAssetAnalysisData(int32_t fileId, int32_t analysisType, bool analysisTotal)
{
    MessageParcel data;
    GetAssetAnalysisDataReqBody reqBody;
    reqBody.fileId = fileId;
    reqBody.language = "zh-Hans";
    reqBody.analysisType = analysisType;
    reqBody.analysisTotal = analysisTotal;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<AnalysisData::MediaAnalysisDataControllerService>();
    service->GetAssetAnalysisData(data, reply);

    IPC::MediaRespVo<GetAssetAnalysisDataRespBody> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }

    int32_t errCode = respVo.GetErrCode();
    if (errCode != 0) {
        MEDIA_ERR_LOG("respVo.GetErrCode: %{public}d", errCode);
        return errCode;
    }

    auto resultSet = respVo.GetBody().resultSet;
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultSet nullptr");
        return -1;
    }

    ShowResultSet(resultSet);

    int32_t rowCount = 0;
    resultSet->GetRowCount(rowCount);
    resultSet->Close();
    return rowCount;
}

HWTEST_F(GetAssetAnalysisDataTest, GetAssetAnalysisData_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetAssetAnalysisData_Test_001");
    int32_t assetId = GetAssetId("GetAssetAnalysisData_Test.jpg");
    ASSERT_GT(assetId, 0);

    ASSERT_LT(GetAssetAnalysisData(assetId, ANALYSIS_INVALID, 0), 0);
}

HWTEST_F(GetAssetAnalysisDataTest, GetAssetAnalysisData_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetAssetAnalysisData_Test_002");
    int32_t assetId = GetAssetId("GetAssetAnalysisData_Test.jpg");
    ASSERT_GT(assetId, 0);

    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_AESTHETICS_SCORE, 0), 0);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_LABEL, 0), 0);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_OCR, 0), 0);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_FACE, 0), 0);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_OBJECT, 0), 0);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_RECOMMENDATION, 0), 0);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_SEGMENTATION, 0), 0);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_COMPOSITION, 0), 0);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_SALIENCY, 0), 0);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_DETAIL_ADDRESS, 0), 1);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_HUMAN_FACE_TAG, 0), 0);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_HEAD_POSITION, 0), 0);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_BONE_POSE, 0), 0);
    ASSERT_GE(GetAssetAnalysisData(assetId, ANALYSIS_VIDEO_LABEL, 0), 0);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_MULTI_CROP, 0), 0);
}

HWTEST_F(GetAssetAnalysisDataTest, GetAssetAnalysisData_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetAssetAnalysisData_Test_003");
    int32_t assetId = GetAssetId("GetAssetAnalysisData_Test.jpg");
    ASSERT_GT(assetId, 0);

    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_AESTHETICS_SCORE, 1), 1);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_LABEL, 1), 1);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_OCR, 1), 1);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_FACE, 1), 1);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_OBJECT, 1), 1);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_RECOMMENDATION, 1), 1);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_SEGMENTATION, 1), 1);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_COMPOSITION, 1), 1);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_SALIENCY, 1), 1);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_DETAIL_ADDRESS, 1), 0);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_HUMAN_FACE_TAG, 1), 0);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_HEAD_POSITION, 1), 1);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_BONE_POSE, 1), 1);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_VIDEO_LABEL, 1), 0);
    ASSERT_EQ(GetAssetAnalysisData(assetId, ANALYSIS_MULTI_CROP, 1), 1);
}
}  // namespace OHOS::Media
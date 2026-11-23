/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "AnalysisMetadataQueryTest"

#include <algorithm>
#include <limits>
#include <gtest/gtest.h>
#include <gtest/hwext/gtest-ext.h>
#include <gtest/hwext/gtest-tag.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "analysis_metadata_query_test.h"
#include "media_analysis_data_controller_service.h"
#include "get_assets_analysis_metadata_vo.h"
#include "user_define_ipc_client.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "media_analysis_data_service.h"
#include "ipc_skeleton.h"
#include "result_set.h"
#include "analysis_metadata_query_test_adapter.h"
#include "userfile_manager_types.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t LABEL_FILE_ID = 2000001001;
static constexpr int32_t CAPTION_FILE_ID = 2000001002;
static constexpr int32_t NULL_CAPTION_FILE_ID = 2000001003;
static constexpr int32_t MISSING_CAPTION_FILE_ID = 2000001004;
static constexpr int32_t EMPTY_CAPTION_FILE_ID = 2000001005;
static const string HIDDEN_CAPTION_PHOTO_PATH =
    "/storage/cloud/files/Photo/analysis_metadata_caption_hidden.jpg";
static const string TRASHED_CAPTION_PHOTO_PATH =
    "/storage/cloud/files/Photo/analysis_metadata_caption_trashed.jpg";

static void ClearAnalysisMetadataFixtures()
{
    CHECK_AND_RETURN_LOG(g_rdbStore != nullptr, "Cannot clear analysis metadata fixtures without RDB store");
    (void)g_rdbStore->ExecuteSql("DELETE FROM AnalysisPhotoMap WHERE map_album IN (9001001,9001002,9001003)");
    (void)g_rdbStore->ExecuteSql("DELETE FROM AnalysisAlbum WHERE album_id IN (9001001,9001002,9001003)");
    (void)g_rdbStore->ExecuteSql(
        "DELETE FROM tab_analysis_caption WHERE file_id IN (2000001002,2000001003,2000001005)");
    const string photoPaths = "('" + HIDDEN_CAPTION_PHOTO_PATH + "','" + TRASHED_CAPTION_PHOTO_PATH + "')";
    (void)g_rdbStore->ExecuteSql("DELETE FROM tab_analysis_caption WHERE file_id IN "
        "(SELECT file_id FROM Photos WHERE data IN " + photoPaths + ")");
    (void)g_rdbStore->ExecuteSql("DELETE FROM Photos WHERE data IN " + photoPaths);
}

static bool ExecuteAnalysisMetadataFixtureSql(const string &sql)
{
    CHECK_AND_RETURN_RET_LOG(g_rdbStore != nullptr && g_rdbStore->ExecuteSql(sql) == NativeRdb::E_OK,
        false, "Failed to execute analysis metadata fixture SQL");
    return true;
}

static int32_t QueryAnalysisMetadataControllerLocally(bool queryLabels, const vector<int32_t> &fileIds,
    GetAssetsAnalysisMetadataRespBody &respBody)
{
    GetAssetsAnalysisMetadataReqBody reqBody;
    reqBody.fileIds = fileIds;
    MessageParcel data;
    MessageParcel reply;
    CHECK_AND_RETURN_RET_LOG(reqBody.Marshalling(data), E_FAIL, "Failed to marshal analysis metadata test request");
    AnalysisData::MediaAnalysisDataControllerService controller;
    uint32_t code = static_cast<uint32_t>(queryLabels ? MediaLibraryBusinessCode::INNER_GET_ASSETS_LABEL :
        MediaLibraryBusinessCode::INNER_GET_ASSETS_CAPTION);
    CHECK_AND_RETURN_RET_LOG(controller.Accept(code), E_FAIL, "Analysis metadata handler is not registered");
    MessageOption option;
    // Direct dispatch tests Parcel handling and Controller logic, without transport or the central permission chain.
    IPC::IPCContext context(option, E_OK);
    controller.OnRemoteRequest(code, data, reply, context);
    IPC::MediaRespVo<GetAssetsAnalysisMetadataRespBody> resp;
    CHECK_AND_RETURN_RET_LOG(resp.Unmarshalling(reply), E_FAIL, "Failed to unmarshal analysis metadata test response");
    respBody = resp.GetBody();
    return resp.GetErrCode();
}

void AnalysisMetadataQueryTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
}

void AnalysisMetadataQueryTest::TearDownTestCase()
{
    ClearAnalysisMetadataFixtures();
    g_rdbStore = nullptr;
}

void AnalysisMetadataQueryTest::SetUp()
{
    ClearAnalysisMetadataFixtures();
}

void AnalysisMetadataQueryTest::TearDown()
{
    ClearAnalysisMetadataFixtures();
}

HWTEST_F(AnalysisMetadataQueryTest, GetAssetsLabel_ReturnsMatchingClassificationAlbums, TestSize.Level1)
{
    ClearAnalysisMetadataFixtures();
    ASSERT_TRUE(ExecuteAnalysisMetadataFixtureSql(
        "INSERT INTO AnalysisAlbum (album_id, album_type, album_subtype, album_name) "
        "VALUES (9001001, 4096, 4097, 'landscape'), (9001002, 4096, 4097, 'portrait'), "
        "(9001003, 4096, 4096, 'excluded')"));
    ASSERT_TRUE(ExecuteAnalysisMetadataFixtureSql("INSERT INTO AnalysisPhotoMap (map_album, map_asset) VALUES "
        "(9001001, 2000001001), (9001002, 2000001001), (9001003, 2000001001)"));

    GetAssetsAnalysisMetadataRespBody respBody;
    ASSERT_EQ(QueryAnalysisMetadataControllerLocally(true,
        {LABEL_FILE_ID, LABEL_FILE_ID, MISSING_CAPTION_FILE_ID}, respBody), E_OK);
    ASSERT_TRUE(respBody.IsValid());
    ASSERT_EQ(respBody.fileIds.size(), 2U);
    ASSERT_EQ(respBody.values.size(), 2U);
    EXPECT_EQ(respBody.fileIds[0], LABEL_FILE_ID);
    EXPECT_EQ(respBody.fileIds[1], LABEL_FILE_ID);
    EXPECT_EQ(respBody.values[0], "landscape");
    EXPECT_EQ(respBody.values[1], "portrait");
}

HWTEST_F(AnalysisMetadataQueryTest, GetAssetsCaption_ServiceReturnsTextAndNullAsEmpty, TestSize.Level1)
{
    ClearAnalysisMetadataFixtures();
    ASSERT_TRUE(ExecuteAnalysisMetadataFixtureSql("INSERT INTO tab_analysis_caption (file_id, caption) "
        "VALUES (2000001002, 'coast at sunset'), (2000001003, NULL)"));

    GetAssetsAnalysisMetadataRespBody respBody;
    GetAssetsAnalysisMetadataReqBody reqBody;
    reqBody.fileIds = {NULL_CAPTION_FILE_ID, CAPTION_FILE_ID, CAPTION_FILE_ID, MISSING_CAPTION_FILE_ID};
    ASSERT_EQ(AnalysisData::MediaAnalysisDataService::GetInstance().GetAssetsCaption(reqBody, respBody), E_OK);
    ASSERT_EQ(respBody.fileIds.size(), 2U);
    ASSERT_EQ(respBody.values.size(), 2U);
    // Request order and duplicate IDs must not change the ascending response order.
    const vector<int32_t> expectedFileIds = {CAPTION_FILE_ID, NULL_CAPTION_FILE_ID};
    EXPECT_EQ(respBody.fileIds, expectedFileIds);
    for (size_t index = 0; index < respBody.fileIds.size(); ++index) {
        const int32_t fileId = respBody.fileIds[index];
        ASSERT_TRUE(fileId == CAPTION_FILE_ID || fileId == NULL_CAPTION_FILE_ID);
        EXPECT_EQ(respBody.values[index], fileId == CAPTION_FILE_ID ? "coast at sunset" : "");
    }
    EXPECT_EQ(std::count(respBody.fileIds.begin(), respBody.fileIds.end(), CAPTION_FILE_ID), 1);
    EXPECT_EQ(std::count(respBody.fileIds.begin(), respBody.fileIds.end(), NULL_CAPTION_FILE_ID), 1);
}

HWTEST_F(AnalysisMetadataQueryTest, GetAssetsMetadata_ServiceBatchLimit, TestSize.Level1)
{
    constexpr size_t batchMaxSize = 100;
    for (bool queryLabels : {true, false}) {
        SCOPED_TRACE(queryLabels ? "label" : "caption");
        vector<int32_t> fileIds;
        for (size_t index = 0; index < batchMaxSize; ++index) {
            fileIds.emplace_back(2000000000 + static_cast<int32_t>(index));
        }
        GetAssetsAnalysisMetadataRespBody respBody;
        auto query = [queryLabels, &respBody](const vector<int32_t> &ids) {
            GetAssetsAnalysisMetadataReqBody reqBody;
            reqBody.fileIds = ids;
            auto &service = AnalysisData::MediaAnalysisDataService::GetInstance();
            return queryLabels ? service.GetAssetsLabel(reqBody, respBody) :
                service.GetAssetsCaption(reqBody, respBody);
        };
        ASSERT_EQ(query(fileIds), E_OK);
        EXPECT_TRUE(respBody.fileIds.empty());
        EXPECT_TRUE(respBody.values.empty());

        fileIds.emplace_back(2000000000 + static_cast<int32_t>(batchMaxSize));
        EXPECT_EQ(query(fileIds), E_INVALID_ARGUMENTS);
        EXPECT_TRUE(respBody.fileIds.empty());
        EXPECT_TRUE(respBody.values.empty());

        // A valid batch of duplicate IDs is accepted and still returns no rows for a missing asset.
        fileIds.assign(batchMaxSize, 2000000000);
        EXPECT_EQ(query(fileIds), E_OK);
        EXPECT_TRUE(respBody.fileIds.empty());
        EXPECT_TRUE(respBody.values.empty());

        // Duplicate IDs must not bypass the service batch limit.
        fileIds.assign(batchMaxSize + 1, 2000000000);
        EXPECT_EQ(query(fileIds), E_INVALID_ARGUMENTS);
        EXPECT_TRUE(respBody.fileIds.empty());
        EXPECT_TRUE(respBody.values.empty());
    }
}

HWTEST_F(AnalysisMetadataQueryTest, GetAssetsCaption_ControllerUidCheckLocally, TestSize.Level1)
{
    // This covers the Controller UID guard only; it does not establish Native/SA authorization.
    const bool allowed = IPCSkeleton::GetCallingUid() == 3553;
    GetAssetsAnalysisMetadataRespBody respBody;
    const int32_t expectedRet = allowed ? E_OK : E_PERMISSION_DENIED;
    ASSERT_EQ(QueryAnalysisMetadataControllerLocally(false, {MISSING_CAPTION_FILE_ID}, respBody), expectedRet);
    if (allowed) {
        EXPECT_TRUE(respBody.IsValid());
    }
    EXPECT_TRUE(respBody.fileIds.empty());
    EXPECT_TRUE(respBody.values.empty());

    // A rejected caller must not reach the batch validation or database query.
    const vector<int32_t> oversizedFileIds(101, MISSING_CAPTION_FILE_ID);
    EXPECT_EQ(QueryAnalysisMetadataControllerLocally(false, oversizedFileIds, respBody),
        allowed ? E_INVALID_ARGUMENTS : E_PERMISSION_DENIED);
}

// F-21: different map rows can produce the same (file_id, album_name) projection.
HWTEST_F(AnalysisMetadataQueryTest, GetAssetsLabel_DistinctAlbumNames, TestSize.Level1)
{
    ASSERT_TRUE(ExecuteAnalysisMetadataFixtureSql(
        "INSERT INTO AnalysisAlbum (album_id, album_type, album_subtype, album_name) "
        "VALUES (9001001, 4096, 4097, 'same'), (9001002, 4096, 4097, 'same')"));
    ASSERT_TRUE(ExecuteAnalysisMetadataFixtureSql("INSERT INTO AnalysisPhotoMap (map_album, map_asset) VALUES "
        "(9001001, 2000001001), (9001002, 2000001001)"));
    GetAssetsAnalysisMetadataReqBody reqBody;
    reqBody.fileIds = {LABEL_FILE_ID};
    GetAssetsAnalysisMetadataRespBody respBody;
    ASSERT_EQ(AnalysisData::MediaAnalysisDataService::GetInstance().GetAssetsLabel(reqBody, respBody), E_OK);
    EXPECT_EQ(respBody.fileIds, vector<int32_t> {LABEL_FILE_ID});
    EXPECT_EQ(respBody.values, vector<string> {"same"});
}

// F-22 / F-23: NULL names are omitted; empty names remain actual labels.
HWTEST_F(AnalysisMetadataQueryTest, GetAssetsLabel_NullAndEmptyAlbumNames, TestSize.Level1)
{
    ASSERT_TRUE(ExecuteAnalysisMetadataFixtureSql(
        "INSERT INTO AnalysisAlbum (album_id, album_type, album_subtype, album_name) "
        "VALUES (9001001, 4096, 4097, NULL), (9001002, 4096, 4097, '')"));
    ASSERT_TRUE(ExecuteAnalysisMetadataFixtureSql("INSERT INTO AnalysisPhotoMap (map_album, map_asset) VALUES "
        "(9001001, 2000001001), (9001002, 2000001001)"));
    GetAssetsAnalysisMetadataReqBody reqBody;
    reqBody.fileIds = {LABEL_FILE_ID};
    GetAssetsAnalysisMetadataRespBody respBody;
    ASSERT_EQ(AnalysisData::MediaAnalysisDataService::GetInstance().GetAssetsLabel(reqBody, respBody), E_OK);
    EXPECT_EQ(respBody.fileIds, vector<int32_t> {LABEL_FILE_ID});
    EXPECT_EQ(respBody.values, vector<string> {""});
}

// F-25 / F-26: neither a non-classification map nor a missing map produces a label row.
HWTEST_F(AnalysisMetadataQueryTest, GetAssetsLabel_NoClassificationOrMap, TestSize.Level1)
{
    ASSERT_TRUE(ExecuteAnalysisMetadataFixtureSql(
        "INSERT INTO AnalysisAlbum (album_id, album_type, album_subtype, album_name) "
        "VALUES (9001001, 4096, 4096, 'excluded')"));
    ASSERT_TRUE(ExecuteAnalysisMetadataFixtureSql(
        "INSERT INTO AnalysisPhotoMap (map_album, map_asset) VALUES (9001001, 2000001001)"));
    GetAssetsAnalysisMetadataReqBody reqBody;
    reqBody.fileIds = {LABEL_FILE_ID, MISSING_CAPTION_FILE_ID};
    GetAssetsAnalysisMetadataRespBody respBody;
    ASSERT_EQ(AnalysisData::MediaAnalysisDataService::GetInstance().GetAssetsLabel(reqBody, respBody), E_OK);
    EXPECT_TRUE(respBody.fileIds.empty());
    EXPECT_TRUE(respBody.values.empty());
}

// C-12: both operations retain the Native inner API AND read-permission policy.
HWTEST_F(AnalysisMetadataQueryTest, GetAssetsMetadata_PermissionPolicy, TestSize.Level1)
{
    AnalysisData::MediaAnalysisDataControllerService controller;
    const vector<vector<PermissionType>> expected = {{SYSTEMINNERAPI_PERM, READ_PERM}};
    for (auto businessCode : {MediaLibraryBusinessCode::INNER_GET_ASSETS_LABEL,
        MediaLibraryBusinessCode::INNER_GET_ASSETS_CAPTION}) {
        vector<vector<PermissionType>> policy;
        bool isBypass = false;
        EXPECT_EQ(controller.GetPermissionPolicy(static_cast<uint32_t>(businessCode), policy, isBypass), E_OK);
        EXPECT_EQ(policy, expected);
        EXPECT_FALSE(isBypass);
    }
}

// F-32: an existing empty caption row remains an explicit response row.
HWTEST_F(AnalysisMetadataQueryTest, GetAssetsCaption_EmptyStringReturnsExistingRow, TestSize.Level1)
{
    ASSERT_TRUE(ExecuteAnalysisMetadataFixtureSql(
        "INSERT INTO tab_analysis_caption (file_id, caption) VALUES (2000001005, '')"));
    GetAssetsAnalysisMetadataReqBody reqBody;
    reqBody.fileIds = {EMPTY_CAPTION_FILE_ID};
    GetAssetsAnalysisMetadataRespBody respBody;
    ASSERT_EQ(AnalysisData::MediaAnalysisDataService::GetInstance().GetAssetsCaption(reqBody, respBody), E_OK);
    EXPECT_EQ(respBody.fileIds, vector<int32_t> {EMPTY_CAPTION_FILE_ID});
    EXPECT_EQ(respBody.values, vector<string> {""});
}

static bool InsertCaptionPhotoFixture(bool hidden, bool trashed, int32_t &fileId)
{
    CHECK_AND_RETURN_RET_LOG(g_rdbStore != nullptr, false, "Caption photo fixture has no RDB store");
    const string path = hidden ? HIDDEN_CAPTION_PHOTO_PATH : TRASHED_CAPTION_PHOTO_PATH;
    const string name = hidden ? "analysis_caption_hidden.jpg" : "analysis_caption_trashed.jpg";
    const int64_t trashedTime = trashed ? 1700000000000LL : 0;
    const string insertSql = "INSERT INTO Photos (data, display_name, media_type, hidden, date_trashed) VALUES ('" +
        path + "','" + name + "'," + to_string(MEDIA_TYPE_IMAGE) + "," + to_string(hidden ? 1 : 0) + "," +
        to_string(trashedTime) + ")";
    CHECK_AND_RETURN_RET_LOG(ExecuteAnalysisMetadataFixtureSql(insertSql), false,
        "Failed to insert caption photo fixture");
    auto state = g_rdbStore->QueryByStep(
        "SELECT file_id, hidden, date_trashed FROM Photos WHERE data = '" + path + "'");
    CHECK_AND_RETURN_RET_LOG(state != nullptr, false, "Cannot read caption photo fixture state");
    int64_t rowId = -1;
    int32_t actualHidden = -1;
    int64_t actualTrashed = -1;
    const bool read = state->GoToNextRow() == NativeRdb::E_OK && state->GetLong(0, rowId) == NativeRdb::E_OK &&
        state->GetInt(1, actualHidden) == NativeRdb::E_OK && state->GetLong(2, actualTrashed) == NativeRdb::E_OK;
    const bool singleRow = read && state->GoToNextRow() == NativeRdb::E_ROW_OUT_RANGE;
    const int32_t closeResult = state->Close();
    CHECK_AND_RETURN_RET_LOG(singleRow && closeResult == NativeRdb::E_OK && rowId > 0 &&
        rowId <= numeric_limits<int32_t>::max() && actualHidden == (hidden ? 1 : 0) &&
        actualTrashed == trashedTime, false, "Caption photo fixture state mismatch");
    fileId = static_cast<int32_t>(rowId);
    return true;
}

// F-34: a real hidden Photos row does not filter its existing caption.
HWTEST_F(AnalysisMetadataQueryTest, GetAssetsCaption_HiddenAssetReturnsCaption, TestSize.Level1)
{
    int32_t fileId = 0;
    ASSERT_TRUE(InsertCaptionPhotoFixture(true, false, fileId));
    ASSERT_TRUE(ExecuteAnalysisMetadataFixtureSql("INSERT INTO tab_analysis_caption (file_id, caption) VALUES (" +
        to_string(fileId) + ", 'hidden-caption')"));
    GetAssetsAnalysisMetadataReqBody reqBody;
    reqBody.fileIds = {fileId};
    GetAssetsAnalysisMetadataRespBody respBody;
    ASSERT_EQ(AnalysisData::MediaAnalysisDataService::GetInstance().GetAssetsCaption(reqBody, respBody), E_OK);
    EXPECT_EQ(respBody.fileIds, vector<int32_t> {fileId});
    EXPECT_EQ(respBody.values, vector<string> {"hidden-caption"});
}

// F-35: a real trashed Photos row does not filter its existing caption.
HWTEST_F(AnalysisMetadataQueryTest, GetAssetsCaption_TrashedAssetReturnsCaption, TestSize.Level1)
{
    int32_t fileId = 0;
    ASSERT_TRUE(InsertCaptionPhotoFixture(false, true, fileId));
    ASSERT_TRUE(ExecuteAnalysisMetadataFixtureSql("INSERT INTO tab_analysis_caption (file_id, caption) VALUES (" +
        to_string(fileId) + ", 'trashed-caption')"));
    GetAssetsAnalysisMetadataReqBody reqBody;
    reqBody.fileIds = {fileId};
    GetAssetsAnalysisMetadataRespBody respBody;
    ASSERT_EQ(AnalysisData::MediaAnalysisDataService::GetInstance().GetAssetsCaption(reqBody, respBody), E_OK);
    EXPECT_EQ(respBody.fileIds, vector<int32_t> {fileId});
    EXPECT_EQ(respBody.values, vector<string> {"trashed-caption"});
}

using Test::AnalysisMetadataFaultResultSet;
using Test::AnalysisMetadataReadFailure;

using Test::AnalysisMetadataReadFailureResult;
using Test::AnalysisMetadataReadFailureState;
using Test::ScopedAnalysisMetadataResultTransform;

static void QueryAnalysisMetadataWithReadFailure(bool queryLabels, AnalysisMetadataReadFailure failure,
    AnalysisMetadataReadFailureResult &outcome)
{
    if (queryLabels) {
        ASSERT_TRUE(ExecuteAnalysisMetadataFixtureSql(
            "INSERT INTO AnalysisAlbum (album_id, album_type, album_subtype, album_name) "
            "VALUES (9001001, 4096, 4097, 'read-failure-label')"));
        ASSERT_TRUE(ExecuteAnalysisMetadataFixtureSql(
            "INSERT INTO AnalysisPhotoMap (map_album, map_asset) VALUES (9001001, 2000001001)"));
    } else {
        ASSERT_TRUE(ExecuteAnalysisMetadataFixtureSql("INSERT INTO tab_analysis_caption (file_id, caption) "
            "VALUES (2000001002, 'read-failure-caption')"));
    }
    auto state = make_shared<AnalysisMetadataReadFailureState>();
    GetAssetsAnalysisMetadataReqBody reqBody;
    reqBody.fileIds = {queryLabels ? LABEL_FILE_ID : CAPTION_FILE_ID};
    GetAssetsAnalysisMetadataRespBody respBody;
    {
        ScopedAnalysisMetadataResultTransform transformScope(
            [state, failure](const shared_ptr<NativeRdb::ResultSet> &result) -> shared_ptr<NativeRdb::ResultSet> {
                CHECK_AND_RETURN_RET_LOG(result != nullptr, nullptr, "Read failure fixture has no result set");
                state->result = make_shared<AnalysisMetadataFaultResultSet>(result, failure);
                return state->result;
            });
        auto &service = AnalysisData::MediaAnalysisDataService::GetInstance();
        outcome.errorCode = queryLabels ? service.GetAssetsLabel(reqBody, respBody) :
            service.GetAssetsCaption(reqBody, respBody);
    }
    outcome.failureTriggered = state->result != nullptr && state->result->WasFailureTriggered();
    outcome.fileIds = std::move(respBody.fileIds);
    outcome.values = std::move(respBody.values);

    auto &service = AnalysisData::MediaAnalysisDataService::GetInstance();
    GetAssetsAnalysisMetadataRespBody recovered;
    outcome.recoveryCode = queryLabels ? service.GetAssetsLabel(reqBody, recovered) :
        service.GetAssetsCaption(reqBody, recovered);
    outcome.recoveredFileIds = std::move(recovered.fileIds);
    outcome.recoveredValues = std::move(recovered.values);
}

HWTEST_F(AnalysisMetadataQueryTest, GetAssetsLabel_FileIdReadFailureReturnsDbError, TestSize.Level1)
{
    AnalysisMetadataReadFailureResult outcome;
    QueryAnalysisMetadataWithReadFailure(true, AnalysisMetadataReadFailure::FILE_ID, outcome);
    ASSERT_TRUE(outcome.failureTriggered) << "The requested read failure was not reached";
    EXPECT_EQ(outcome.errorCode, E_HAS_DB_ERROR);
    EXPECT_TRUE(outcome.fileIds.empty());
    EXPECT_TRUE(outcome.values.empty());
    ASSERT_EQ(outcome.recoveryCode, E_OK);
    EXPECT_EQ(outcome.recoveredFileIds, vector<int32_t> {LABEL_FILE_ID});
    EXPECT_EQ(outcome.recoveredValues, vector<string> {"read-failure-label"});
}

HWTEST_F(AnalysisMetadataQueryTest, GetAssetsLabel_NullFlagReadFailureReturnsDbError, TestSize.Level1)
{
    AnalysisMetadataReadFailureResult outcome;
    QueryAnalysisMetadataWithReadFailure(true, AnalysisMetadataReadFailure::NULL_FLAG, outcome);
    ASSERT_TRUE(outcome.failureTriggered) << "The requested read failure was not reached";
    EXPECT_EQ(outcome.errorCode, E_HAS_DB_ERROR);
    EXPECT_TRUE(outcome.fileIds.empty());
    EXPECT_TRUE(outcome.values.empty());
    ASSERT_EQ(outcome.recoveryCode, E_OK);
    EXPECT_EQ(outcome.recoveredFileIds, vector<int32_t> {LABEL_FILE_ID});
    EXPECT_EQ(outcome.recoveredValues, vector<string> {"read-failure-label"});
}

HWTEST_F(AnalysisMetadataQueryTest, GetAssetsLabel_TextReadFailureReturnsDbError, TestSize.Level1)
{
    AnalysisMetadataReadFailureResult outcome;
    QueryAnalysisMetadataWithReadFailure(true, AnalysisMetadataReadFailure::TEXT, outcome);
    ASSERT_TRUE(outcome.failureTriggered) << "The requested read failure was not reached";
    EXPECT_EQ(outcome.errorCode, E_HAS_DB_ERROR);
    EXPECT_TRUE(outcome.fileIds.empty());
    EXPECT_TRUE(outcome.values.empty());
    ASSERT_EQ(outcome.recoveryCode, E_OK);
    EXPECT_EQ(outcome.recoveredFileIds, vector<int32_t> {LABEL_FILE_ID});
    EXPECT_EQ(outcome.recoveredValues, vector<string> {"read-failure-label"});
}

HWTEST_F(AnalysisMetadataQueryTest, GetAssetsCaption_FileIdReadFailureReturnsDbError, TestSize.Level1)
{
    AnalysisMetadataReadFailureResult outcome;
    QueryAnalysisMetadataWithReadFailure(false, AnalysisMetadataReadFailure::FILE_ID, outcome);
    ASSERT_TRUE(outcome.failureTriggered) << "The requested read failure was not reached";
    EXPECT_EQ(outcome.errorCode, E_HAS_DB_ERROR);
    EXPECT_TRUE(outcome.fileIds.empty());
    EXPECT_TRUE(outcome.values.empty());
    ASSERT_EQ(outcome.recoveryCode, E_OK);
    EXPECT_EQ(outcome.recoveredFileIds, vector<int32_t> {CAPTION_FILE_ID});
    EXPECT_EQ(outcome.recoveredValues, vector<string> {"read-failure-caption"});
}

HWTEST_F(AnalysisMetadataQueryTest, GetAssetsCaption_NullFlagReadFailureReturnsDbError, TestSize.Level1)
{
    AnalysisMetadataReadFailureResult outcome;
    QueryAnalysisMetadataWithReadFailure(false, AnalysisMetadataReadFailure::NULL_FLAG, outcome);
    ASSERT_TRUE(outcome.failureTriggered) << "The requested read failure was not reached";
    EXPECT_EQ(outcome.errorCode, E_HAS_DB_ERROR);
    EXPECT_TRUE(outcome.fileIds.empty());
    EXPECT_TRUE(outcome.values.empty());
    ASSERT_EQ(outcome.recoveryCode, E_OK);
    EXPECT_EQ(outcome.recoveredFileIds, vector<int32_t> {CAPTION_FILE_ID});
    EXPECT_EQ(outcome.recoveredValues, vector<string> {"read-failure-caption"});
}

HWTEST_F(AnalysisMetadataQueryTest, GetAssetsCaption_TextReadFailureReturnsDbError, TestSize.Level1)
{
    AnalysisMetadataReadFailureResult outcome;
    QueryAnalysisMetadataWithReadFailure(false, AnalysisMetadataReadFailure::TEXT, outcome);
    ASSERT_TRUE(outcome.failureTriggered) << "The requested read failure was not reached";
    EXPECT_EQ(outcome.errorCode, E_HAS_DB_ERROR);
    EXPECT_TRUE(outcome.fileIds.empty());
    EXPECT_TRUE(outcome.values.empty());
    ASSERT_EQ(outcome.recoveryCode, E_OK);
    EXPECT_EQ(outcome.recoveredFileIds, vector<int32_t> {CAPTION_FILE_ID});
    EXPECT_EQ(outcome.recoveredValues, vector<string> {"read-failure-caption"});
}

} // namespace OHOS::Media

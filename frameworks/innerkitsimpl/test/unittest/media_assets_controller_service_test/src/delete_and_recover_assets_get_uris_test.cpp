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

#define MLOG_TAG "MediaAssetsControllerServiceTest"

#include "delete_and_recover_assets_get_uris_test.h"

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

#include <message_parcel.h>

#include "media_assets_controller_service.h"
#include "asset_uri_result_vo.h"
#include "delete_local_assets_and_get_uris_vo.h"
#include "recover_assets_and_get_uris_vo.h"
#include "media_resp_vo.h"
#include "media_empty_obj_vo.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_errno.h"
#include "media_column.h"
#include "result_set_utils.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace IPC;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_ONE_SECOND = 1;

static const string SQL_INSERT_PHOTO = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "(" +
    MediaColumn::MEDIA_FILE_PATH + ", " + MediaColumn::MEDIA_SIZE + ", " + MediaColumn::MEDIA_TITLE + ", " +
    MediaColumn::MEDIA_NAME + ", " + MediaColumn::MEDIA_TYPE + ", " + MediaColumn::MEDIA_OWNER_PACKAGE + ", " +
    MediaColumn::MEDIA_PACKAGE_NAME + ", " + MediaColumn::MEDIA_DATE_ADDED + ", "  +
    MediaColumn::MEDIA_DATE_MODIFIED + ", " + MediaColumn::MEDIA_DATE_TAKEN + ", " +
    MediaColumn::MEDIA_DURATION + ", " + MediaColumn::MEDIA_IS_FAV + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
    MediaColumn::MEDIA_HIDDEN + ", " + PhotoColumn::PHOTO_HEIGHT + ", " + PhotoColumn::PHOTO_WIDTH + ", " +
    PhotoColumn::PHOTO_EDIT_TIME + ", " + PhotoColumn::PHOTO_SHOOTING_MODE + ")";

static int32_t ClearTable(const string &table)
{
    RdbPredicates predicates(table);

    int32_t rows = 0;
    int32_t err = g_rdbStore->Delete(rows, predicates);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to clear album table, err: %{public}d", err);
        return E_HAS_DB_ERROR;
    }
    system("rm -rf /storage/cloud/files/Photo/16/");
    return E_OK;
}

static int32_t GetAssetId(const std::string &title)
{
    NativeRdb::RdbPredicates rdbPredicate(PhotoColumn::PHOTOS_TABLE);
    rdbPredicate.EqualTo(MediaColumn::MEDIA_TITLE, title);
    vector<string> columns = { PhotoColumn::MEDIA_ID };
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query failed, title:%{public}s", title.c_str());
        return 0;
    }
    int32_t assetId = 0;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        assetId = GetInt32Val(PhotoColumn::MEDIA_ID, resultSet);
    }
    resultSet->Close();
    return assetId;
}

static void InsertAssetIntoPhotosTable(const std::string &title, int64_t dateTrashed)
{
    const std::string data = "/storage/cloud/files/Photo/16/IMG_" + title + ".jpg";
    g_rdbStore->ExecuteSql(SQL_INSERT_PHOTO + " VALUES('" + data + "', 175258, '" + title + "', '" + title +
        ".jpg', 1, 'com.ohos.camera', '相机', 1501924205218, 0, 1501924205, 0, 0, " +
        to_string(dateTrashed) + ", 0, 1280, 960, 0, '1' )");
    system(("mkdir -p /storage/cloud/files/Photo/16/ && touch " + data).c_str());
}

static std::string BuildExpectedUri(int32_t fileId, const std::string &title)
{
    return "file://media/Photo/" + to_string(fileId) + "/IMG_" + title + "/" + title + ".jpg";
}

static int64_t QueryDateTrashed(int32_t fileId)
{
    NativeRdb::RdbPredicates rdbPredicate(PhotoColumn::PHOTOS_TABLE);
    rdbPredicate.EqualTo(MediaColumn::MEDIA_ID, fileId);
    vector<string> columns = { MediaColumn::MEDIA_DATE_TRASHED };
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query date trashed failed, fileId:%{public}d", fileId);
        return -1;
    }
    int64_t dateTrashed = -1;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        dateTrashed = GetInt64Val(MediaColumn::MEDIA_DATE_TRASHED, resultSet);
    }
    resultSet->Close();
    return dateTrashed;
}

/*
 * The static NAPI/ANI methods cannot be invoked directly in this repository's unittest,
 * so the request/response body marshalling path of the controller is verified here,
 * which is exactly what the napi/ani layer calls through UserDefineIPCClient.
 */
static int32_t DeleteLocalAssetsAndGetUris(const std::vector<std::string> &fileIds,
    std::vector<AssetUriResult> &results)
{
    results.clear();
    DeleteLocalAssetsAndGetUrisReqBody reqBody;
    reqBody.fileIds = fileIds;
    MessageParcel data;
    if (!reqBody.Marshalling(data)) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }
    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->DeleteLocalAssetsAndGetUris(data, reply);

    MediaRespVo<DeleteLocalAssetsAndGetUrisRespBody> respVo;
    if (!respVo.Unmarshalling(reply)) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    results = respVo.GetBody().results;
    return respVo.GetErrCode();
}

static int32_t DeleteLocalAssetsAndGetUrisWithInvalidParcel()
{
    MessageParcel data;
    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->DeleteLocalAssetsAndGetUris(data, reply);

    MediaRespVo<MediaEmptyObjVo> respVo;
    if (!respVo.Unmarshalling(reply)) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    return respVo.GetErrCode();
}

static int32_t RecoverAssetsAndGetUris(const std::vector<std::string> &assets,
    std::vector<AssetUriResult> &results)
{
    results.clear();
    RecoverAssetsAndGetUrisReqBody reqBody;
    reqBody.assets = assets;
    MessageParcel data;
    if (!reqBody.Marshalling(data)) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }
    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->RecoverAssetsAndGetUris(data, reply);

    MediaRespVo<RecoverAssetsAndGetUrisRespBody> respVo;
    if (!respVo.Unmarshalling(reply)) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    results = respVo.GetBody().results;
    return respVo.GetErrCode();
}

static int32_t RecoverAssetsAndGetUrisWithInvalidParcel()
{
    MessageParcel data;
    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->RecoverAssetsAndGetUris(data, reply);

    MediaRespVo<MediaEmptyObjVo> respVo;
    if (!respVo.Unmarshalling(reply)) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    return respVo.GetErrCode();
}

void DeleteAndRecoverAssetsGetUrisTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start DeleteAndRecoverAssetsGetUrisTest failed, can not get g_rdbStore");
        exit(1);
    }
    ClearTable(PhotoColumn::PHOTOS_TABLE);
}

void DeleteAndRecoverAssetsGetUrisTest::TearDownTestCase(void)
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_ONE_SECOND));
}

void DeleteAndRecoverAssetsGetUrisTest::SetUp()
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
}

void DeleteAndRecoverAssetsGetUrisTest::TearDown(void) {}

/*
 * @tc.name: DeleteLocalAssetsAndGetUris_EmptyFileIds_001
 * @tc.desc: Empty fileIds are rejected by the argument check and return -EINVAL
 * @tc.type: FUNCTION
 */
HWTEST_F(DeleteAndRecoverAssetsGetUrisTest, DeleteLocalAssetsAndGetUris_EmptyFileIds_001, TestSize.Level0)
{
    std::vector<std::string> fileIds;
    std::vector<AssetUriResult> results;

    int32_t errCode = DeleteLocalAssetsAndGetUris(fileIds, results);
    EXPECT_EQ(errCode, -EINVAL);
    EXPECT_TRUE(results.empty());
}

/*
 * @tc.name: DeleteLocalAssetsAndGetUris_InvalidParcel_002
 * @tc.desc: Returns E_IPC_SEVICE_UNMARSHALLING_FAIL when the request body is invalid
 * @tc.type: FUNCTION
 */
HWTEST_F(DeleteAndRecoverAssetsGetUrisTest, DeleteLocalAssetsAndGetUris_InvalidParcel_002, TestSize.Level0)
{
    int32_t errCode = DeleteLocalAssetsAndGetUrisWithInvalidParcel();
    EXPECT_EQ(errCode, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

/*
 * @tc.name: DeleteLocalAssetsAndGetUris_NotExistAssets_003
 * @tc.desc: Returns E_OK with one failed result per input when the assets do not exist
 * @tc.type: FUNCTION
 */
HWTEST_F(DeleteAndRecoverAssetsGetUrisTest, DeleteLocalAssetsAndGetUris_NotExistAssets_003, TestSize.Level0)
{
    std::vector<std::string> fileIds = { "999999001", "999999002" };
    std::vector<AssetUriResult> results;

    int32_t errCode = DeleteLocalAssetsAndGetUris(fileIds, results);
    EXPECT_EQ(errCode, E_OK);
    ASSERT_EQ(results.size(), fileIds.size());
    for (size_t i = 0; i < fileIds.size(); i++) {
        EXPECT_EQ(results[i].originalFileId, fileIds[i]);
        EXPECT_FALSE(results[i].isSuccess);
        EXPECT_TRUE(results[i].resultUri.empty());
    }
}

/*
 * @tc.name: DeleteLocalAssetsAndGetUris_LocalAsset_004
 * @tc.desc: For a local-only asset the result uri is the current uri with the same fileId
 * @tc.type: FUNCTION
 */
HWTEST_F(DeleteAndRecoverAssetsGetUrisTest, DeleteLocalAssetsAndGetUris_LocalAsset_004, TestSize.Level0)
{
    InsertAssetIntoPhotosTable("del_local_pic", 0);
    int32_t assetId = GetAssetId("del_local_pic");
    ASSERT_GT(assetId, 0);

    std::vector<std::string> fileIds = { to_string(assetId) };
    std::vector<AssetUriResult> results;

    int32_t errCode = DeleteLocalAssetsAndGetUris(fileIds, results);
    EXPECT_EQ(errCode, E_OK);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].originalFileId, to_string(assetId));
    EXPECT_TRUE(results[0].isSuccess);
    EXPECT_EQ(results[0].resultUri, BuildExpectedUri(assetId, "del_local_pic"));
    EXPECT_GT(QueryDateTrashed(assetId), 0);
}

/*
 * @tc.name: RecoverAssetsAndGetUris_EmptyAssets_005
 * @tc.desc: Empty assets are rejected by the argument check and return -EINVAL
 * @tc.type: FUNCTION
 */
HWTEST_F(DeleteAndRecoverAssetsGetUrisTest, RecoverAssetsAndGetUris_EmptyAssets_005, TestSize.Level0)
{
    std::vector<std::string> assets;
    std::vector<AssetUriResult> results;

    int32_t errCode = RecoverAssetsAndGetUris(assets, results);
    EXPECT_EQ(errCode, -EINVAL);
    EXPECT_TRUE(results.empty());
}

/*
 * @tc.name: RecoverAssetsAndGetUris_InvalidParcel_006
 * @tc.desc: Returns E_IPC_SEVICE_UNMARSHALLING_FAIL when the recover request body is invalid
 * @tc.type: FUNCTION
 */
HWTEST_F(DeleteAndRecoverAssetsGetUrisTest, RecoverAssetsAndGetUris_InvalidParcel_006, TestSize.Level0)
{
    int32_t errCode = RecoverAssetsAndGetUrisWithInvalidParcel();
    EXPECT_EQ(errCode, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

/*
 * @tc.name: RecoverAssetsAndGetUris_TrashedAsset_007
 * @tc.desc: A trashed asset is recovered and the normal album uri is returned in input order
 * @tc.type: FUNCTION
 */
HWTEST_F(DeleteAndRecoverAssetsGetUrisTest, RecoverAssetsAndGetUris_TrashedAsset_007, TestSize.Level0)
{
    InsertAssetIntoPhotosTable("recover_pic", 1748488395008);
    int32_t assetId = GetAssetId("recover_pic");
    ASSERT_GT(assetId, 0);
    std::string trashedUri = BuildExpectedUri(assetId, "recover_pic");

    std::vector<std::string> assets = { trashedUri };
    std::vector<AssetUriResult> results;

    int32_t errCode = RecoverAssetsAndGetUris(assets, results);
    EXPECT_GE(errCode, 0);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].originalFileId, to_string(assetId));
    EXPECT_TRUE(results[0].isSuccess);
    EXPECT_EQ(results[0].resultUri, trashedUri);
}

/*
 * @tc.name: AssetUriResult_Unmarshalling_ReadOriginalFileIdFail_008
 * @tc.desc: Empty parcel makes the originalFileId read fail, then Unmarshalling returns false
 * @tc.type: FUNCTION
 */
HWTEST_F(DeleteAndRecoverAssetsGetUrisTest, AssetUriResult_Unmarshalling_ReadOriginalFileIdFail_008, TestSize.Level0)
{
    MessageParcel parcel;
    AssetUriResult result;
    EXPECT_FALSE(result.Unmarshalling(parcel));
}

/*
 * @tc.name: AssetUriResult_Unmarshalling_ReadResultUriFail_009
 * @tc.desc: Parcel with only originalFileId makes the resultUri read fail, then Unmarshalling returns false
 * @tc.type: FUNCTION
 */
HWTEST_F(DeleteAndRecoverAssetsGetUrisTest, AssetUriResult_Unmarshalling_ReadResultUriFail_009, TestSize.Level0)
{
    MessageParcel parcel;
    ASSERT_TRUE(parcel.WriteString("100"));
    parcel.RewindRead(0);

    AssetUriResult result;
    EXPECT_FALSE(result.Unmarshalling(parcel));
    EXPECT_EQ(result.originalFileId, "100");
}

/*
 * @tc.name: AssetUriResult_Unmarshalling_ReadIsSuccessFail_010
 * @tc.desc: Parcel without isSuccess makes the bool read fail, then Unmarshalling returns false
 * @tc.type: FUNCTION
 */
HWTEST_F(DeleteAndRecoverAssetsGetUrisTest, AssetUriResult_Unmarshalling_ReadIsSuccessFail_010, TestSize.Level0)
{
    MessageParcel parcel;
    ASSERT_TRUE(parcel.WriteString("100"));
    ASSERT_TRUE(parcel.WriteString("file://media/Photo/100"));
    parcel.RewindRead(0);

    AssetUriResult result;
    EXPECT_FALSE(result.Unmarshalling(parcel));
    EXPECT_EQ(result.resultUri, "file://media/Photo/100");
    EXPECT_FALSE(result.isSuccess);
}

/*
 * @tc.name: AssetUriResult_Marshalling_WriteOriginalFileIdFail_011
 * @tc.desc: originalFileId larger than the parcel max capacity makes WriteString fail, then Marshalling returns false
 * @tc.type: FUNCTION
 */
HWTEST_F(DeleteAndRecoverAssetsGetUrisTest, AssetUriResult_Marshalling_WriteOriginalFileIdFail_011, TestSize.Level0)
{
    MessageParcel parcel;
    const size_t capacity = parcel.GetMaxCapacity();
    ASSERT_GT(capacity, 0);

    AssetUriResult result;
    result.originalFileId = std::string(capacity, 'a');
    EXPECT_FALSE(result.Marshalling(parcel));
}

/*
 * @tc.name: AssetUriResult_Marshalling_WriteResultUriFail_012
 * @tc.desc: resultUri larger than the parcel remaining capacity makes WriteString fail, then Marshalling returns false
 * @tc.type: FUNCTION
 */
HWTEST_F(DeleteAndRecoverAssetsGetUrisTest, AssetUriResult_Marshalling_WriteResultUriFail_012, TestSize.Level0)
{
    MessageParcel parcel;
    const size_t capacity = parcel.GetMaxCapacity();
    ASSERT_GT(capacity, 0);

    AssetUriResult result;
    result.originalFileId = "100";
    result.resultUri = std::string(capacity, 'a');
    EXPECT_FALSE(result.Marshalling(parcel));
}

/*
 * @tc.name: AssetUriResult_Marshalling_WriteIsSuccessFail_013
 * @tc.desc: Two strings fill the parcel capacity exactly, then writing isSuccess fails and Marshalling returns false
 * @tc.type: FUNCTION
 */
HWTEST_F(DeleteAndRecoverAssetsGetUrisTest, AssetUriResult_Marshalling_WriteIsSuccessFail_013, TestSize.Level0)
{
    MessageParcel parcel;
    const size_t capacity = parcel.GetMaxCapacity();
    ASSERT_GT(capacity, sizeof(int32_t) * 2 + sizeof(int32_t) * 2);

    // Each string takes a 4-byte length prefix plus a 4-byte aligned payload,
    // so split the remaining capacity by the alignment unit to fill the parcel exactly.
    size_t payloadBudget = (capacity - sizeof(int32_t) * 2) / sizeof(int32_t) * sizeof(int32_t);
    size_t firstPayload = payloadBudget / 2 / sizeof(int32_t) * sizeof(int32_t);
    size_t secondPayload = payloadBudget - firstPayload;
    ASSERT_GE(firstPayload, sizeof(int32_t));
    ASSERT_GE(secondPayload, sizeof(int32_t));

    AssetUriResult result;
    result.originalFileId = std::string(firstPayload - 1, 'a');
    result.resultUri = std::string(secondPayload - 1, 'b');
    EXPECT_FALSE(result.Marshalling(parcel));
}

}  // namespace OHOS::Media

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

#define MLOG_TAG "CloneToAlbumAndConvertAssetTest"

#include "clone_to_album_test.h"

#include <chrono>
#include <thread>
#include <vector>

#include "clone_to_album_callback_proxy.h"
#include "convert_to_asset_vo.h"
#include "datashare_result_set.h"
#include "media_column.h"
#include "media_file_uri.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "media_upgrade.h"
#include "message_parcel.h"
#include "photo_album_column.h"
#include "rdb_utils.h"
#include "result_set_utils.h"

using namespace std;
using namespace OHOS::NativeRdb;
using namespace testing::ext;

namespace OHOS {
namespace Media {

std::shared_ptr<MediaLibraryRdbStore> g_testRdbStore = nullptr;

static constexpr int32_t SLEEP_SECONDS = 1;

static std::vector<std::string> g_createTableSqlLists = {
    PhotoAlbumColumns::CREATE_TABLE,
    PhotoUpgrade::CREATE_PHOTO_TABLE,
};

std::shared_ptr<DataShare::DataShareResultSet> BuildResultSet()
{
    vector<string> columns;
    NativeRdb::RdbPredicates pred(PhotoColumn::PHOTOS_TABLE);
    auto resultSet = g_testRdbStore->Query(pred, columns);
    if (resultSet == nullptr) {
        return nullptr;
    }
    auto bridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resultSet);
    return make_shared<DataShare::DataShareResultSet>(bridge);
}

void CloneToAlbumAndConvertAssetTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("CloneToAlbumTest SetUpTestCase");
    MediaLibraryUnitTestUtils::Init();
    g_testRdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_testRdbStore == nullptr) {
        MEDIA_ERR_LOG("CloneToAlbumTest: can not get rdbStore");
        exit(1);
    }
    MediaLibraryUnitTestUtils::CreateTestTables(g_testRdbStore, g_createTableSqlLists);
}

void CloneToAlbumAndConvertAssetTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("CloneToAlbumTest TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void CloneToAlbumAndConvertAssetTest::SetUp(void)
{
    MEDIA_INFO_LOG("CloneToAlbumTest SetUp");
}

void CloneToAlbumAndConvertAssetTest::TearDown(void)
{
    MEDIA_INFO_LOG("CloneToAlbumTest TearDown");
}

HWTEST_F(CloneToAlbumAndConvertAssetTest, OnProgress_NullRemote_ReturnsErr, TestSize.Level0)
{
    MEDIA_INFO_LOG("OnProgress_NullRemote_ReturnsErr enter");
    CloneToAlbumCallbackProxy proxy(sptr<IRemoteObject>(nullptr));
    EXPECT_LT(proxy.OnProgress(100, 200, 1, 2), 0);
    MEDIA_INFO_LOG("OnProgress_NullRemote_ReturnsErr end");
}

HWTEST_F(CloneToAlbumAndConvertAssetTest, OnProgress_SuccessRemote_ReturnsOk, TestSize.Level0)
{
    MEDIA_INFO_LOG("OnProgress_SuccessRemote_ReturnsOk enter");
    auto remote = sptr<IRemoteObject>(new MockRemoteObject());
    CloneToAlbumCallbackProxy proxy(remote);
    EXPECT_EQ(proxy.OnProgress(100, 200, 1, 2), E_OK);
    MEDIA_INFO_LOG("OnProgress_SuccessRemote_ReturnsOk end");
}

HWTEST_F(CloneToAlbumAndConvertAssetTest, OnComplete_NullRemote_ReturnsErr, TestSize.Level0)
{
    MEDIA_INFO_LOG("OnComplete_NullRemote_ReturnsErr enter");
    auto resultSet = BuildResultSet();
    ASSERT_NE(resultSet, nullptr);
    vector<string> successUris = { "uri_1" };
    CloneToAlbumCallbackProxy proxy(sptr<IRemoteObject>(nullptr));
    EXPECT_LT(proxy.OnComplete(0, successUris, resultSet), 0);
    MEDIA_INFO_LOG("OnComplete_NullRemote_ReturnsErr end");
}

HWTEST_F(CloneToAlbumAndConvertAssetTest, OnComplete_SuccessRemote_ReturnsOk, TestSize.Level0)
{
    MEDIA_INFO_LOG("OnComplete_SuccessRemote_ReturnsOk enter");
    auto remote = sptr<IRemoteObject>(new MockRemoteObject());
    auto resultSet = BuildResultSet();
    ASSERT_NE(resultSet, nullptr);
    vector<string> successUris = { "uri_1", "uri_2" };
    CloneToAlbumCallbackProxy proxy(remote);
    EXPECT_EQ(proxy.OnComplete(0, successUris, resultSet), E_OK);
    MEDIA_INFO_LOG("OnComplete_SuccessRemote_ReturnsOk end");
}

HWTEST_F(CloneToAlbumAndConvertAssetTest, OnComplete_EmptyUris_SuccessRemote_ReturnsOk, TestSize.Level0)
{
    MEDIA_INFO_LOG("OnComplete_EmptyUris_SuccessRemote_ReturnsOk enter");
    auto remote = sptr<IRemoteObject>(new MockRemoteObject());
    auto resultSet = BuildResultSet();
    ASSERT_NE(resultSet, nullptr);
    vector<string> successUris;
    CloneToAlbumCallbackProxy proxy(remote);
    EXPECT_EQ(proxy.OnComplete(-1, successUris, resultSet), E_OK);
    MEDIA_INFO_LOG("OnComplete_EmptyUris_SuccessRemote_ReturnsOk end");
}

HWTEST_F(CloneToAlbumAndConvertAssetTest, ConvertToAssetReqBody_MarshallingUnmarshalling_RoundTrip, TestSize.Level0)
{
    MEDIA_INFO_LOG("ConvertToAssetReqBody_MarshallingUnmarshalling_RoundTrip enter");
    ConvertToAssetReqBody req;
    req.path = "/storage/media/local/files/Docs/test/convert_asset.jpg";

    MessageParcel data;
    EXPECT_TRUE(req.Marshalling(data));

    ConvertToAssetReqBody req2;
    EXPECT_TRUE(req2.Unmarshalling(data));
    EXPECT_EQ(req2.path, req.path);
    MEDIA_INFO_LOG("ConvertToAssetReqBody_MarshallingUnmarshalling_RoundTrip end");
}

HWTEST_F(CloneToAlbumAndConvertAssetTest, ConvertToAssetReqBody_Unmarshalling_EmptyParcel_False, TestSize.Level0)
{
    MEDIA_INFO_LOG("ConvertToAssetReqBody_Unmarshalling_EmptyParcel_False enter");
    MessageParcel data; // 空包
    ConvertToAssetReqBody req;
    EXPECT_FALSE(req.Unmarshalling(data));
    MEDIA_INFO_LOG("ConvertToAssetReqBody_Unmarshalling_EmptyParcel_False end");
}

HWTEST_F(CloneToAlbumAndConvertAssetTest, ConvertToAssetRespBody_Marshalling_NullResultSet_False, TestSize.Level0)
{
    MEDIA_INFO_LOG("ConvertToAssetRespBody_Marshalling_NullResultSet_False enter");
    ConvertToAssetRespBody resp;
    resp.resultSet = nullptr;
    MessageParcel data;
    EXPECT_FALSE(resp.Marshalling(data));
    MEDIA_INFO_LOG("ConvertToAssetRespBody_Marshalling_NullResultSet_False end");
}

HWTEST_F(CloneToAlbumAndConvertAssetTest, ConvertToAssetRespBody_Unmarshalling_NullResultSet_False, TestSize.Level0)
{
    MEDIA_INFO_LOG("ConvertToAssetRespBody_Unmarshalling_NullResultSet_False enter");
    ConvertToAssetRespBody resp;
    resp.resultSet = nullptr;
    MessageParcel data;
    (void)resp.Marshalling(data);
    ConvertToAssetRespBody resp2;
    EXPECT_FALSE(resp2.Unmarshalling(data));
    MEDIA_INFO_LOG("ConvertToAssetRespBody_Unmarshalling_NullResultSet_False end");
}

HWTEST_F(CloneToAlbumAndConvertAssetTest, ConvertToAssetRespBody_MarshallingUnmarshalling_Success, TestSize.Level1)
{
    MEDIA_INFO_LOG("ConvertToAssetRespBody_MarshallingUnmarshalling_Success enter");
    auto resultSet = BuildResultSet();
    ASSERT_NE(resultSet, nullptr);

    ConvertToAssetRespBody resp;
    resp.resultSet = resultSet;
    MessageParcel data;
    EXPECT_TRUE(resp.Marshalling(data));

    ConvertToAssetRespBody resp2;
    EXPECT_TRUE(resp2.Unmarshalling(data));
    EXPECT_NE(resp2.resultSet, nullptr);
    MEDIA_INFO_LOG("ConvertToAssetRespBody_MarshallingUnmarshalling_Success end");
}

} // namespace Media
} // namespace OHOS

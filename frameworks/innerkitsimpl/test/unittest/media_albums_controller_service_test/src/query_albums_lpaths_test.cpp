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

#include "query_albums_lpaths_test.h"

#include <memory>
#include <string>

#include "media_albums_controller_service.h"

#include "query_albums_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "photo_album_column.h"
#include "rdb_predicates.h"
#include "photo_album_column.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 3;
static std::vector<std::string> ALBUM_FETCH_COLUMNS = {
    PhotoAlbumColumns::ALBUM_ID, PhotoAlbumColumns::ALBUM_LPATH
};

static void ClearTable(const string &table)
{
    int32_t rows = 0;
    RdbPredicates predicates(table);
    int32_t errCode = g_rdbStore->Delete(rows, predicates);
    CHECK_AND_RETURN_LOG(errCode == E_OK, "g_rdbStore->Delete errCode:%{public}d", errCode);
}

void QueryAlbumsLpathsTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start QueryAlbumsLpathsTest failed, can not get g_rdbStore");
        exit(1);
    }

    ClearTable(PhotoAlbumColumns::TABLE);
    MEDIA_INFO_LOG("QueryAlbumsLpathsTest::SetUpTestCase");
}

void QueryAlbumsLpathsTest::TearDownTestCase(void)
{
    ClearTable(PhotoAlbumColumns::TABLE);
    MEDIA_INFO_LOG("QueryAlbumsLpathsTest::TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void QueryAlbumsLpathsTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void QueryAlbumsLpathsTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

/**
 * @tc.name  : GetAlbumsLpaths_ShouldReturnError_WhenReadRequestBodyFails
 * @tc.number: GetAlbumsLpaths_Test_001
 * @tc.desc  : 测试当读取请求体失败时, QueryAlbumsLpaths 函数应返回错误
 */
HWTEST_F(QueryAlbumsLpathsTest, GetAlbumsLpaths_Test_001, TestSize.Level0) {
    MEDIA_INFO_LOG("GetAlbumsLpaths_Test_001 Start");
    MessageParcel data;
    MessageParcel reply;

    auto service = make_shared<MediaAlbumsControllerService>();
    service->QueryAlbumsLpaths(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);
    ASSERT_LT(respVo.GetErrCode(), 0);
}

/**
 * @tc.name  : QueryAlbumsLpaths_ShouldReturnSuccess_WhenQueryWithFilterSucceeds
 * @tc.number: QueryAlbumsLpaths_Test_002
 * @tc.desc  : 测试当数据库查询成功时, QueryAlbumsLpaths 函数应返回成功
 */
HWTEST_F(QueryAlbumsLpathsTest, QueryAlbumsLpaths_Test_002, TestSize.Level0) {
    MEDIA_INFO_LOG("Start QueryAlbumsLpaths_Test_002");
    MessageParcel data;
    MessageParcel reply;
    QueryAlbumsReqBody reqBody;
    reqBody.albumType = PhotoAlbumType::USER;
    reqBody.albumSubType = PhotoAlbumSubType::USER_GENERIC;
    reqBody.columns = ALBUM_FETCH_COLUMNS;

    auto ret = reqBody.Marshalling(data);
    EXPECT_EQ(ret, true);

    auto service = make_shared<MediaAlbumsControllerService>();
    service->QueryAlbumsLpaths(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ret = respVo.Unmarshalling(reply);

    EXPECT_EQ(ret, true);
    EXPECT_EQ(respVo.GetErrCode(), 0);
}

/**
 * @tc.name  : QueryAlbumsLpaths_ShouldReturnSuccess_WhenQueryWithFilterSucceeds
 * @tc.number: QueryAlbumsLpaths_Test_003
 * @tc.desc  : 测试当数据库查询成功时, QueryAlbumsLpaths 函数应返回成功
 */
HWTEST_F(QueryAlbumsLpathsTest, QueryAlbumsLpaths_Test_003, TestSize.Level0) {
    MEDIA_INFO_LOG("Start QueryAlbumsLpaths_Test_003");
    MessageParcel data;
    MessageParcel reply;
    QueryAlbumsReqBody reqBody;
    reqBody.albumType = PhotoAlbumType::SOURCE;
    reqBody.albumSubType = PhotoAlbumSubType::SOURCE_GENERIC;
    reqBody.columns = ALBUM_FETCH_COLUMNS;

    auto ret = reqBody.Marshalling(data);
    EXPECT_EQ(ret, true);

    auto service = make_shared<MediaAlbumsControllerService>();
    service->QueryAlbumsLpaths(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ret = respVo.Unmarshalling(reply);

    EXPECT_EQ(ret, true);
    EXPECT_EQ(respVo.GetErrCode(), 0);
}

/**
 * @tc.name  : QueryAlbumsLpaths_ShouldReturnError_WhenQueryWithFilterFails
 * @tc.number: QueryAlbumsLpaths_Test_004
 * @tc.desc  : 测试当数据库查询失败时, QueryAlbumsLpaths 函数应返回失败
 */
HWTEST_F(QueryAlbumsLpathsTest, QueryAlbumsLpaths_Test_004, TestSize.Level0) {
    MEDIA_INFO_LOG("Start QueryAlbumsLpaths_Test_004");
    MessageParcel data;
    MessageParcel reply;
    QueryAlbumsReqBody reqBody;
    reqBody.albumType = PhotoAlbumType::SYSTEM;
    reqBody.columns = ALBUM_FETCH_COLUMNS;

    auto ret = reqBody.Marshalling(data);
    EXPECT_EQ(ret, true);

    auto service = make_shared<MediaAlbumsControllerService>();
    service->QueryAlbumsLpaths(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ret = respVo.Unmarshalling(reply);

    EXPECT_EQ(ret, true);
    EXPECT_NE(respVo.GetErrCode(), 0);
}
} // namespace OHOS::Media
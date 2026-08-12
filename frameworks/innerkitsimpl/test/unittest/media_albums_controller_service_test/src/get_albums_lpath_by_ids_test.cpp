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

#include "get_albums_lpath_by_ids_test.h"

#include <memory>
#include <string>

#include "media_albums_controller_service.h"

#include "get_albums_lpath_by_ids_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "photo_album_column.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;

static int32_t ClearUserAlbums()
{
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(PhotoAlbumType::USER));

    int32_t rows = 0;
    int32_t err = g_rdbStore->Delete(rows, predicates);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to clear album table, err: %{public}d", err);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

void GetAlbumsLpathByIdsTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get g_rdbStore");
        exit(1);
    }
    ClearUserAlbums();
    MEDIA_INFO_LOG("SetUpTestCase");
}

void GetAlbumsLpathByIdsTest::TearDownTestCase(void)
{
    ClearUserAlbums();
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void GetAlbumsLpathByIdsTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void GetAlbumsLpathByIdsTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

/**
 * @tc.name  : GetAlbumsLpathByIds_ShouldReturnError_WhenReadRequestBodyFails
 * @tc.number: GetAlbumsLpathByIdsTest_001
 * @tc.desc  : 测试当读取请求体失败时,GetAlbumsLpathByIds 函数应返回错误
 */
HWTEST_F(GetAlbumsLpathByIdsTest, GetAlbumsLpathByIdsTest_001, TestSize.Level0) {
    MEDIA_INFO_LOG("GetAlbumsLpathByIdsTest_001 Start");
    MessageParcel data;
    MessageParcel reply;

    auto service = make_shared<MediaAlbumsControllerService>();
    service->GetAlbumsLpathByIds(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);
    ASSERT_LT(respVo.GetErrCode(), 0);
}

/**
 * @tc.name  : GetAlbumsLpathByIds_ShouldReturnSuccess_WhenQueryWithFilterSucceeds
 * @tc.number: GetAlbumsLpathByIdsTest_002
 * @tc.desc  : 测试当数据库查询成功时,GetAlbumsLpathByIds 函数应返回成功
 */
HWTEST_F(GetAlbumsLpathByIdsTest, GetAlbumsLpathByIdsTest_002, TestSize.Level0) {
    MEDIA_INFO_LOG("GetAlbumsLpathByIdsTest_002 Start");
    MessageParcel data;
    MessageParcel reply;
    GetAlbumsLpathByIdsReqBody reqBody;
    reqBody.albumId = 1;

    auto ret = reqBody.Marshalling(data);
    EXPECT_EQ(ret, true);

    auto service = make_shared<MediaAlbumsControllerService>();
    service->GetAlbumsLpathByIds(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ret = respVo.Unmarshalling(reply);

    EXPECT_EQ(ret, true);
    EXPECT_EQ(respVo.GetErrCode(), 0);
}
} // namespace OHOS::Media
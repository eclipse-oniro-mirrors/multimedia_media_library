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

#include "change_assets_test.h"

#include <memory>
#include <string>

#define private public
#define protected public
#include "media_albums_controller_service.h"
#include "media_analysis_data_controller_service.h"
#undef private
#undef protected

#include "change_request_add_assets_vo.h"
#include "change_request_remove_assets_vo.h"
#include "change_request_move_assets_vo.h"
#include "change_request_recover_assets_vo.h"
#include "change_request_delete_assets_vo.h"
#include "change_request_dismiss_assets_vo.h"
#include "change_request_merge_album_vo.h"
#include "change_request_place_before_vo.h"
#include "change_request_set_order_position_vo.h"

#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_ONE_SECOND = 1;
static constexpr int32_t TEST_ALBUM_ID = 11;

int32_t ChangeAssetsTest::ClearUserAlbums()
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

void ChangeAssetsTest::SetUpTestCase(void)
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

void ChangeAssetsTest::TearDownTestCase(void)
{
    ClearUserAlbums();
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_ONE_SECOND));
}

void ChangeAssetsTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void ChangeAssetsTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

int32_t ServiceAddAssets(int isHighlight, int isHiddenOnly)
{
    ChangeRequestAddAssetsReqBody reqBody;

    reqBody.albumId = TEST_ALBUM_ID;
    reqBody.isHighlight = isHighlight;
    reqBody.isHiddenOnly = isHiddenOnly;
    reqBody.assets = {"file://media/Photo/101", "file://media/Photo/102", "file://media/Photo/103"};
    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAlbumsControllerService>();
    service->AddAssets(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }

    return respVo.GetErrCode();
}

int32_t ServiceRemoveAssets(int isHiddenOnly)
{
    MessageParcel data;
    MessageParcel reply;
    ChangeRequestRemoveAssetsReqBody reqBody;

    reqBody.albumId = TEST_ALBUM_ID;
    reqBody.isHiddenOnly = isHiddenOnly;
    reqBody.assets = {"file://media/Photo/101", "file://media/Photo/102", "file://media/Photo/103"};
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    auto service = make_shared<MediaAlbumsControllerService>();
    service->RemoveAssets(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }

    return respVo.GetErrCode();
}
/**
 * @tc.name  : AddAssets_ShouldAddHighlightAssets_WhenIsHighlightIsTrue
 * @tc.number: AddAssets_Test_001
 * @tc.desc  : 测试当 isHighlight isHiddenOnly 能否正常添加资产
 */
HWTEST_F(ChangeAssetsTest, AddAssets_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start AddAssets_Test_001");
    int32_t result = ServiceAddAssets(0, 0);
    EXPECT_EQ(result, 3);
    result = ServiceAddAssets(0, 1);
    EXPECT_EQ(result, 3);
    result = ServiceAddAssets(1, 0);
    EXPECT_EQ(result, 0);
    result = ServiceAddAssets(1, 1);
    EXPECT_EQ(result, 0);
}

/**
 * @tc.name  : AddAssets_ShouldReturnError_WhenReadRequestBodyFails
 * @tc.number: AddAssets_Test_002
 * @tc.desc  : 测试当读取请求体失败时,AddAssets 函数应返回错误
 */
HWTEST_F(ChangeAssetsTest, AddAssets_Test_002, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;

    auto service = make_shared<MediaAlbumsControllerService>();
    service->AddAssets(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);
    ASSERT_LT(respVo.GetErrCode(), 0);
}

/**
 * @tc.name  : RemoveAssets_ShouldReturnError_WhenReadRequestBodyFails
 * @tc.number: RemoveAssets_Test_002
 * @tc.desc  : 测试当读取请求体失败时,RemoveAssets 函数应返回错误
 */
HWTEST_F(ChangeAssetsTest, RemoveAssets_Test_002, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;

    auto service = make_shared<MediaAlbumsControllerService>();
    service->RemoveAssets(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);
    ASSERT_LT(respVo.GetErrCode(), 0);
}

/**
 * @tc.name  : MoveAssets_ShouldReturnError_WhenReadRequestBodyFails
 * @tc.number: MoveAssetsTest_001
 * @tc.desc  : 测试当输入参数有效时,MoveAssets 函数应成功更新数据库。
 */
HWTEST_F(ChangeAssetsTest, MoveAssetsTest_001, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;
    ChangeRequestMoveAssetsReqBody reqBody;
    reqBody.albumId = 1;
    reqBody.targetAlbumId = 2;
    reqBody.assets = {"file://media/Photo/101", "file://media/Photo/102", "file://media/Photo/103"};
    auto ret = reqBody.Marshalling(data);
    EXPECT_EQ(ret, true);


    auto service = make_shared<MediaAlbumsControllerService>();
    service->MoveAssets(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ret = respVo.Unmarshalling(reply);
    EXPECT_EQ(true, ret);
}

/**
 * @tc.name  : MoveAssets_ShouldReturnError_WhenReadRequestBodyFails
 * @tc.number: MoveAssets_Test_002
 * @tc.desc  : 测试当读取请求体失败时,MoveAssets 函数应返回错误
 */
HWTEST_F(ChangeAssetsTest, MoveAssets_Test_002, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;

    auto service = make_shared<MediaAlbumsControllerService>();
    service->MoveAssets(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);
    ASSERT_LT(respVo.GetErrCode(), 0);
}

/**
 * @tc.name  : RecoverAssets_ShouldReturnError_WhenReadRequestBodyFails
 * @tc.number: RecoverAssetsTest_001
 * @tc.desc  : 测试 RecoverAssets 常规流程
 */
HWTEST_F(ChangeAssetsTest, RecoverAssetsTest_001, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;
    ChangeRequestRecoverAssetsReqBody reqBody;
    reqBody.assets = {"file://media/Photo/101", "file://media/Photo/102", "file://media/Photo/103"};
    auto ret = reqBody.Marshalling(data);
    EXPECT_EQ(ret, true);


    auto service = make_shared<MediaAlbumsControllerService>();
    service->RecoverAssets(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ret = respVo.Unmarshalling(reply);
    EXPECT_EQ(true, ret);
}

/**
 * @tc.name  : RecoverAssetsTest_ShouldReturnError_WhenReadRequestBodyFails
 * @tc.number: RecoverAssetsTest_Test_002
 * @tc.desc  : 测试当读取请求体失败时,RecoverAssets 函数应返回错误
 */
HWTEST_F(ChangeAssetsTest, RecoverAssets_Test_002, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;

    auto service = make_shared<MediaAlbumsControllerService>();
    service->RecoverAssets(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);
    ASSERT_LT(respVo.GetErrCode(), 0);
}

/**
 * @tc.name  : DeleteAssets_ShouldReturnError_WhenReadRequestBodyFails
 * @tc.number: DeleteAssetsTest_001
 * @tc.desc  : 测试  DeleteAssets 常规流程
 */
HWTEST_F(ChangeAssetsTest, DeleteAssetsTest_001, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;
    ChangeRequestDeleteAssetsReqBody reqBody;
    reqBody.assets = {"file://media/Photo/101", "file://media/Photo/102", "file://media/Photo/103"};
    auto ret = reqBody.Marshalling(data);
    EXPECT_EQ(ret, true);


    auto service = make_shared<MediaAlbumsControllerService>();
    service->DeleteAssets(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ret = respVo.Unmarshalling(reply);
    EXPECT_EQ(true, ret);
}

/**
 * @tc.name  : DeleteAssetsTest_ShouldReturnError_WhenReadRequestBodyFails
 * @tc.number: DeleteAssetsTest_Test_002
 * @tc.desc  : 测试当读取请求体失败时,DeleteAssets 函数应返回错误
 */
HWTEST_F(ChangeAssetsTest, DeleteAssetsTest_002, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;

    auto service = make_shared<MediaAlbumsControllerService>();
    service->DeleteAssets(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);
    ASSERT_LT(respVo.GetErrCode(), 0);
}

/**
 * @tc.name  : DismissAsset_ShouldReturnError_WhenRecoverPhotoAssetsFails
 * @tc.number: DismissAssetsTest_001
 * @tc.desc  : 测试当恢复资产失败时,RecoverAssets 函数应返回错误
 */
HWTEST_F(ChangeAssetsTest, DismissAssetsTest_001, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;

    // 模拟读取请求体成功
    ChangeRequestDismissAssetsReqBody reqBody;
    reqBody.assets = {"file://media/Photo/101", "file://media/Photo/102", "file://media/Photo/103"};
    reqBody.albumId = 1;

    auto ret = reqBody.Marshalling(data);
    EXPECT_EQ(ret, true);


    auto service = make_shared<MediaAlbumsControllerService>();
    service->DismissAssets(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ret = respVo.Unmarshalling(reply);
    EXPECT_EQ(true, ret);
}

/**
 * @tc.name  : DismissAssetsTest_ShouldReturnError_WhenReadRequestBodyFails
 * @tc.number: DismissAssetsTest_002
 * @tc.desc  : 测试当读取请求体失败时,DismissAssets 函数应返回错误
 */
HWTEST_F(ChangeAssetsTest, DismissAssetsTest_002, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;

    auto service = make_shared<MediaAlbumsControllerService>();
    service->DismissAssets(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);
    ASSERT_LT(respVo.GetErrCode(), 0);
}

HWTEST_F(ChangeAssetsTest, MergeAlbumTest_001, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;

    // 模拟读取请求体成功
    ChangeRequestMergeAlbumReqBody reqBody;
    reqBody.targetAlbumId = 2;
    reqBody.albumId = 1;

    auto ret = reqBody.Marshalling(data);
    EXPECT_EQ(ret, true);


    auto service = make_shared<MediaAlbumsControllerService>();
    service->MergeAlbum(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ret = respVo.Unmarshalling(reply);
    EXPECT_EQ(true, ret);
}

/**
 * @tc.name  : MergeAlbumTest_ShouldReturnError_WhenReadRequestBodyFails
 * @tc.number: MergeAlbumTest_002
 * @tc.desc  : 测试当读取请求体失败时,DismissAssets 函数应返回错误
 */
HWTEST_F(ChangeAssetsTest, MergeAlbumTest_002, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;

    auto service = make_shared<MediaAlbumsControllerService>();
    service->MergeAlbum(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);
    ASSERT_LT(respVo.GetErrCode(), 0);
}

HWTEST_F(ChangeAssetsTest, PlaceBeforeTest_001, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;

    // 模拟读取请求体成功
    ChangeRequestPlaceBeforeReqBody reqBody;
    reqBody.referenceAlbumId = 2;
    reqBody.albumId = 1;
    reqBody.albumType = 1;
    reqBody.albumSubType = 1;
    auto ret = reqBody.Marshalling(data);
    EXPECT_EQ(ret, true);


    auto service = make_shared<MediaAlbumsControllerService>();
    service->PlaceBefore(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ret = respVo.Unmarshalling(reply);
    EXPECT_EQ(true, ret);
}

/**
 * @tc.name  : PlaceBeforeTest_ShouldReturnError_WhenReadRequestBodyFails
 * @tc.number: PlaceBeforeTest_002
 * @tc.desc  : 测试当读取请求体失败时,PlaceBefore 函数应返回错误
 */
HWTEST_F(ChangeAssetsTest, PlaceBeforeTest_002, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;

    auto service = make_shared<MediaAlbumsControllerService>();
    service->PlaceBefore(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);
    ASSERT_LT(respVo.GetErrCode(), 0);
}

HWTEST_F(ChangeAssetsTest, SetOrderPositionTest_001, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;

    // 模拟读取请求体成功
    ChangeRequestSetOrderPositionReqBody reqBody;
    reqBody.albumId = 1;
    reqBody.orderString = "";
    reqBody.assetIds = {"101", "102", "103"};

    auto ret = reqBody.Marshalling(data);
    EXPECT_EQ(ret, true);


    auto service = make_shared<AnalysisData::MediaAnalysisDataControllerService>();
    service->SetOrderPosition(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ret = respVo.Unmarshalling(reply);
    EXPECT_EQ(true, ret);
}

/**
 * @tc.name  : SetOrderPositionTest_ShouldReturnError_WhenReadRequestBodyFails
 * @tc.number: SetOrderPositionTest_002
 * @tc.desc  : 测试当读取请求体失败时,PlaceBefore 函数应返回错误
 */
HWTEST_F(ChangeAssetsTest, SetOrderPositionTest_002, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;

    auto service = make_shared<AnalysisData::MediaAnalysisDataControllerService>();
    service->SetOrderPosition(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);
    ASSERT_LT(respVo.GetErrCode(), 0);
}
}  // namespace OHOS::Media
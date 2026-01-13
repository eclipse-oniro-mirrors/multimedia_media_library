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

#include "query_edit_asset_status_test.h"

#include <string>
#include <vector>

#define private public
#define protected public
#include "media_assets_controller_service.h"
#include "media_analysis_data_controller_service.h"
#undef private
#undef protected

#include "message_parcel.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "media_column.h"
#include "result_set_utils.h"
#include "medialibrary_errno.h"
#include "is_edited_vo.h"
#include "request_edit_data_vo.h"
#include "get_edit_data_vo.h"
#include "start_asset_analysis_vo.h"
#include "get_cloudmedia_asset_status_vo.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;

static int32_t ClearTable(const string &table)
{
    RdbPredicates predicates(table);

    int32_t rows = 0;
    int32_t err = g_rdbStore->Delete(rows, predicates);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to clear album table, err: %{public}d", err);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

void QueryEditAssetStatusTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get g_rdbStore");
        exit(1);
    }
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("SetUpTestCase");
}

void QueryEditAssetStatusTest::TearDownTestCase(void)
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void QueryEditAssetStatusTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void QueryEditAssetStatusTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}


/**
 * @tc.name  : IsEdited_ShouldReturnError_WhenReadRequestBodyFails
 * @tc.number: IsEditedTest_001
 * @tc.desc  : 测试当读取请求体失败时,IsEdited 函数应返回错误
 */
HWTEST_F(QueryEditAssetStatusTest, IsEditedTest_001, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;

    auto service = make_shared<MediaAssetsControllerService>();
    service->IsEdited(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);
    ASSERT_LT(respVo.GetErrCode(), 0);
}


/**
 * @tc.name  : IsEdited_ShouldQuerySuccessfully_WhenReadRequestBodySucceeds
 * @tc.number: IsEditedTest_002
 * @tc.desc  : 测试当读取请求体成功时,IsEdited 函数应成功执行查询操作
 */
HWTEST_F(QueryEditAssetStatusTest, IsEditedTest_002, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;
    IsEditedReqBody reqBody;
    reqBody.fileId = 1;
    auto ret = reqBody.Marshalling(data);
    EXPECT_EQ(ret, true);

    auto service = make_shared<MediaAssetsControllerService>();
    service->IsEdited(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ret = respVo.Unmarshalling(reply);

    EXPECT_EQ(ret, true);
    EXPECT_EQ(respVo.GetErrCode(), 0);
}

/**
 * @tc.name  : RequestEditData_ShouldReturnError_WhenReadRequestBodyFails
 * @tc.number: RequestEditDataTest_001
 * @tc.desc  : 测试当读取请求体失败时,RequestEditData 函数应返回错误
 */
HWTEST_F(QueryEditAssetStatusTest, RequestEditDataTest_001, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;

    auto service = make_shared<MediaAssetsControllerService>();
    service->RequestEditData(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);
    ASSERT_LT(respVo.GetErrCode(), 0);
}

/**
 * @tc.name  : RequestEditData_ShouldReturnResultSet_WhenQueryEditDataExistsSucceeds
 * @tc.number: RequestEditDataTest_002
 * @tc.desc  : 测试当查询编辑数据成功时,RequestEditData 函数应返回查询结果集
 */
HWTEST_F(QueryEditAssetStatusTest, RequestEditDataTest_002, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;
    RequestEditDataReqBody reqBody;
    reqBody.predicates.EqualTo("file_id", "1111111");

    auto ret = reqBody.Marshalling(data);
    EXPECT_EQ(ret, true);

    auto service = make_shared<MediaAssetsControllerService>();
    service->RequestEditData(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ret = respVo.Unmarshalling(reply);

    EXPECT_EQ(ret, true);
    EXPECT_EQ(respVo.GetErrCode(), 0);
}

/**
 * @tc.name  : GetEditData_ShouldReturnError_WhenReadRequestBodyFails
 * @tc.number: GetEditDataTest_001
 * @tc.desc  : 测试当读取请求体失败时,GetEditData 函数应返回错误
 */
HWTEST_F(QueryEditAssetStatusTest, GetEditDataTest_001, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;

    auto service = make_shared<MediaAssetsControllerService>();
    service->GetEditData(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);
    ASSERT_LT(respVo.GetErrCode(), 0);
}

/**
 * @tc.name  : GetEditData_ShouldReturnSuccess_WhenReadRequestBodySucceeds
 * @tc.number: GetEditDataTest_002
 * @tc.desc  : 测试当读取请求体成功时,GetEditData 函数应返回成功
 */
HWTEST_F(QueryEditAssetStatusTest, GetEditDataTest_002, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;
    GetEditDataReqBody reqBody;

    reqBody.predicates.EqualTo("file_id", "1111111");

    auto ret = reqBody.Marshalling(data);
    EXPECT_EQ(ret, true);

    auto service = make_shared<MediaAssetsControllerService>();
    service->GetEditData(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ret = respVo.Unmarshalling(reply);

    EXPECT_EQ(ret, true);
    EXPECT_EQ(respVo.GetErrCode(), 0);
}

/**
 * @tc.name  : GetCloudMediaAssetStatus_ShouldReturnError_WhenReadRequestBodyFails
 * @tc.number: GetCloudMediaAssetStatusTest_001
 * @tc.desc  : 测试当读取请求体失败时,函数应返回错误信息
 */
HWTEST_F(QueryEditAssetStatusTest, GetCloudMediaAssetStatusTest_001, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;

    auto service = make_shared<MediaAssetsControllerService>();
    service->GetCloudMediaAssetStatus(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);
    ASSERT_LT(respVo.GetErrCode(), 0);
}

/**
 * @tc.name  : GetCloudMediaAssetStatus_ShouldReturnStatus_WhenReadRequestBodySucceeds
 * @tc.number: GetCloudMediaAssetStatusTest_002
 * @tc.desc  : 测试当读取请求体成功时,函数应返回正确的任务状态
 */
HWTEST_F(QueryEditAssetStatusTest, GetCloudMediaAssetStatusTest_002, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;
    GetCloudMediaAssetStatusReqBody reqBody;

    auto ret = reqBody.Marshalling(data);
    EXPECT_EQ(ret, true);

    auto service = make_shared<MediaAssetsControllerService>();
    service->GetCloudMediaAssetStatus(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ret = respVo.Unmarshalling(reply);

    EXPECT_EQ(ret, true);
    EXPECT_EQ(respVo.GetErrCode(), 0);
}

/**
 * @tc.name  : StartAssetAnalysis_ShouldReturnError_WhenReadRequestBodyFails
 * @tc.number: StartAssetAnalysisTest_001
 * @tc.desc  : 测试当读取请求体失败时,StartAssetAnalysis 函数应返回错误响应
 */
HWTEST_F(QueryEditAssetStatusTest, StartAssetAnalysisTest_001, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;

    auto service = make_shared<AnalysisData::MediaAnalysisDataControllerService>();
    service->StartAssetAnalysis(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);
    ASSERT_LT(respVo.GetErrCode(), 0);
}

/**
 * @tc.name  : StartAssetAnalysis_ShouldReturnSuccess_WhenReadRequestBodySucceeds
 * @tc.number: StartAssetAnalysisTest_002
 * @tc.desc  : 测试当读取请求体成功时,StartAssetAnalysis 函数应返回成功响应
 */
HWTEST_F(QueryEditAssetStatusTest, StartAssetAnalysisTest_002, TestSize.Level0) {
    MessageParcel data;
    MessageParcel reply;
    StartAssetAnalysisReqBody reqBody;
    std::vector<std::string> fileIds{"111111", "222222"};

    reqBody.predicates.In("Photos.file_id", fileIds);
    auto ret = reqBody.Marshalling(data);
    EXPECT_EQ(ret, true);

    auto service = make_shared<AnalysisData::MediaAnalysisDataControllerService>();
    service->StartAssetAnalysis(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ret = respVo.Unmarshalling(reply);

    EXPECT_EQ(ret, true);
    EXPECT_EQ(respVo.GetErrCode(), 0);
}
}  // namespace OHOS::Media
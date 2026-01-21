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

#include "cloud_enhancement_test.h"

#include <string>
#include <vector>

#define private public
#define protected public
#include "media_assets_controller_service.h"
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
#include "cloud_enhancement_vo.h"
#include "enhancement_manager.h"
#include "create_asset_vo.h"
#include "cloud_enhancement_dto.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace IPC;

static constexpr int32_t SLEEP_SECONDS = 1;
static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
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

void CloudEnhancementTest::SetUpTestCase(void)
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

void CloudEnhancementTest::TearDownTestCase(void)
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void CloudEnhancementTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void CloudEnhancementTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

static int32_t ServicePublicCreateAsset(const std::string &ext, const std::string &title = "")
{
    CreateAssetReqBody reqBody;
    reqBody.mediaType = MEDIA_TYPE_IMAGE;
    reqBody.title = title;
    reqBody.extension = ext;

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->PublicCreateAsset(data, reply);

    IPC::MediaRespVo<CreateAssetRespBody> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }

    int32_t errCode = respVo.GetErrCode();
    if (errCode != 0) {
        MEDIA_ERR_LOG("respVo.GetErrCode: %{public}d", errCode);
        return errCode;
    }

    return respVo.GetBody().fileId;
}

static int32_t UpdateCEAvailable(int32_t fileId, int32_t ceAvailable)
{
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    int32_t changedRows = -1;
    ValuesBucket valueBucket;
    valueBucket.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE, ceAvailable);
    g_rdbStore->Update(changedRows, valueBucket, rdbPredicates);
    return changedRows;
}

HWTEST_F(CloudEnhancementTest, SubmitCloudEnhancementTasks_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SubmitCloudEnhancementTasks_Test_001");
    int32_t fileId = ServicePublicCreateAsset("jpg");
    ASSERT_GT(fileId, 0);
    int32_t changedRows = UpdateCEAvailable(fileId, 1);
    ASSERT_GT(changedRows, 0);

    CloudEnhancementReqBody reqBody;
    reqBody.hasCloudWatermark = true;
    reqBody.triggerMode = 1;
    reqBody.fileUris = { "file://media/Photo/" + to_string(fileId) };

    MessageParcel data;
    bool result = reqBody.Marshalling(data);
    ASSERT_NE(result, false);

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->SubmitCloudEnhancementTasks(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> respVo;
    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    int32_t errCode = respVo.GetErrCode();
    EXPECT_NE(errCode, E_OK);

    MEDIA_INFO_LOG("End SubmitCloudEnhancementTasks_Test_001");
}

HWTEST_F(CloudEnhancementTest, SubmitCloudEnhancementTasks_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SubmitCloudEnhancementTasks_Test_002");
    int32_t fileId = ServicePublicCreateAsset("jpg");
    ASSERT_GT(fileId, 0);
    int32_t changedRows = UpdateCEAvailable(fileId, 1);
    ASSERT_GT(changedRows, 0);

    CloudEnhancementReqBody reqBody;
    reqBody.hasCloudWatermark = true;
    reqBody.triggerMode = 1;
    reqBody.fileUris = { "datashare://media/Photo/" + to_string(fileId) };

    MessageParcel data;
    bool result = reqBody.Marshalling(data);
    ASSERT_NE(result, false);

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->SubmitCloudEnhancementTasks(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> respVo;
    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    int32_t errCode = respVo.GetErrCode();
    EXPECT_EQ(errCode, E_GET_PRAMS_FAIL);

    MEDIA_INFO_LOG("End SubmitCloudEnhancementTasks_Test_002");
}

HWTEST_F(CloudEnhancementTest, PrioritizeCloudEnhancementTask_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start PrioritizeCloudEnhancementTask_Test_001");
    int32_t fileId = ServicePublicCreateAsset("jpg");
    ASSERT_GT(fileId, 0);
    int32_t changedRows = UpdateCEAvailable(fileId, 2);
    ASSERT_GT(changedRows, 0);

    CloudEnhancementReqBody reqBody;
    reqBody.fileUris = { "file://media/Photo/" + to_string(fileId) };

    MessageParcel data;
    bool result = reqBody.Marshalling(data);
    ASSERT_NE(result, false);

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->PrioritizeCloudEnhancementTask(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> respVo;
    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    int32_t errCode = respVo.GetErrCode();
    EXPECT_NE(errCode, E_OK);
    MEDIA_INFO_LOG("End PrioritizeCloudEnhancementTask_Test_001");
}

HWTEST_F(CloudEnhancementTest, PrioritizeCloudEnhancementTask_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start PrioritizeCloudEnhancementTask_Test_002");
    int32_t fileId = ServicePublicCreateAsset("jpg");
    ASSERT_GT(fileId, 0);
    int32_t changedRows = UpdateCEAvailable(fileId, 2);
    ASSERT_GT(changedRows, 0);

    CloudEnhancementReqBody reqBody;
    reqBody.fileUris = { "datashare://media/Photo/" + to_string(fileId) };

    MessageParcel data;
    bool result = reqBody.Marshalling(data);
    ASSERT_NE(result, false);

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->PrioritizeCloudEnhancementTask(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> respVo;
    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    int32_t errCode = respVo.GetErrCode();
    EXPECT_EQ(errCode, E_GET_PRAMS_FAIL);
    MEDIA_INFO_LOG("End PrioritizeCloudEnhancementTask_Test_002");
}

HWTEST_F(CloudEnhancementTest, CancelCloudEnhancementTasks_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CancelCloudEnhancementTasks_Test_001");
    int32_t fileId = ServicePublicCreateAsset("jpg");
    ASSERT_GT(fileId, 0);
    int32_t changedRows = UpdateCEAvailable(fileId, 2);
    ASSERT_GT(changedRows, 0);

    CloudEnhancementReqBody reqBody;
    reqBody.fileUris = { "file://media/Photo/" + to_string(fileId) };

    MessageParcel data;
    bool result = reqBody.Marshalling(data);
    ASSERT_NE(result, false);

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->CancelCloudEnhancementTasks(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> respVo;
    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    int32_t errCode = respVo.GetErrCode();
    EXPECT_NE(errCode, E_OK);
    MEDIA_INFO_LOG("End CancelCloudEnhancementTasks_Test_001");
}

HWTEST_F(CloudEnhancementTest, CancelCloudEnhancementTasks_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CancelCloudEnhancementTasks_Test_002");
    int32_t fileId = ServicePublicCreateAsset("jpg");
    ASSERT_GT(fileId, 0);
    int32_t changedRows = UpdateCEAvailable(fileId, 2);
    ASSERT_GT(changedRows, 0);

    CloudEnhancementReqBody reqBody;
    reqBody.fileUris = { "datashare://media/Photo/" + to_string(fileId) };

    MessageParcel data;
    bool result = reqBody.Marshalling(data);
    ASSERT_NE(result, false);

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->CancelCloudEnhancementTasks(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> respVo;
    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    int32_t errCode = respVo.GetErrCode();
    EXPECT_EQ(errCode, E_GET_PRAMS_FAIL);
    MEDIA_INFO_LOG("End CancelCloudEnhancementTasks_Test_002");
}

HWTEST_F(CloudEnhancementTest, CancelAllCloudEnhancementTasks_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CancelAllCloudEnhancementTasks_Test_001");
    CloudEnhancementReqBody reqBody;

    MessageParcel data;
    bool result = reqBody.Marshalling(data);
    ASSERT_NE(result, false);

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->CancelAllCloudEnhancementTasks(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> respVo;
    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    int32_t errCode = respVo.GetErrCode();
    EXPECT_NE(errCode, E_OK);

    MEDIA_INFO_LOG("End CancelAllCloudEnhancementTasks_Test_001");
}

HWTEST_F(CloudEnhancementTest, CloudEnhancementDto_ToString, TestSize.Level0)
{
    CloudEnhancementDto dto;

    dto.fileUris.push_back("file1.txt");
    dto.fileUris.push_back("file2.txt");
    dto.fileUris.push_back("file3.txt");

    std::string expected = "{\"hasCloudWatermark\": \"0\",\"triggerMode\": \"-1\",\"fileUris\":"
    " \" [file1.txt, file2.txt, file3.txt]}";
    std::string str = dto.ToString();
    EXPECT_EQ(expected, str);
}
}  // namespace OHOS::Media
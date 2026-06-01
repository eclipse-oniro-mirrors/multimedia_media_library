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

#define MLOG_TAG "MultistagesCaptureDeferredPhotoProcSessionCallbackTest"

#include "multistages_capture_deferred_photo_proc_session_callback_test.h"

#include "camera_test_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_upgrade.h"
#include "media_uri_utils.h"
#include "multistages_capture_request_task_manager.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "mock_camera_pipeline.h"
#include "userfilemgr_uri.h"

#define private public
#define protected public
#include "multistages_camera_capture_manager.h"
#include "multistages_capture_deferred_photo_proc_session_callback.h"
#include "picture_adapter.h"
#undef private
#undef protected

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace testing;

namespace OHOS {
namespace Media {
static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
static const std::string HDR_PICTURE_PATH = "/data/local/tmp/test_jpeg.jpg";

static const int32_t FILE_ID_INPUT = 1;
static const std::string PHOTO_ID_INPUT = "1970_000000_0000_001";
static const std::string URI_INPUT = "file://media/Photo/1/IMG_000000_001/IMG_19700000_000001.jpg";

static void CleanTestTables()
{
    std::vector<std::string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
    };
    for (auto &dropTable : dropTableList) {
        string dropSql = "DROP TABLE " + dropTable + ";";
        int32_t ret = g_rdbStore->ExecuteSql(dropSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Drop %{public}s table failed", dropTable.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Drop %{public}s table success", dropTable.c_str());
    }
}

static void SetTables()
{
    std::vector<std::string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
}

static void ClearAndRestart()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    system("rm -rf /storage/cloud/files/.thumbs");
    system("rm -rf /storage/cloud/files/.editData");
    for (const auto &dir : TEST_ROOT_DIRS) {
        std::string ROOT_PATH = "/storage/cloud/100/files/";
        bool ret = MediaFileUtils::CreateDirectory(ROOT_PATH + dir + "/");
        CHECK_AND_PRINT_LOG(ret, "make %{public}s dir failed, ret=%{public}d", dir.c_str(), ret);
    }
    CleanTestTables();
    SetTables();
}

void MultiStagesCaptureDeferredPhotoProcSessionCallbackTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    SetTables();
}

void MultiStagesCaptureDeferredPhotoProcSessionCallbackTest::TearDownTestCase(void)
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    ClearAndRestart();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    this_thread::sleep_for(chrono::seconds(SLEEP_FIVE_SECONDS));
    MEDIA_INFO_LOG("Clean is finish");
}

// SetUp:Execute before each test case
void MultiStagesCaptureDeferredPhotoProcSessionCallbackTest::SetUp()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    ClearAndRestart();
}

void MultiStagesCaptureDeferredPhotoProcSessionCallbackTest::TearDown(void)
{
    MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.clear();
    MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.clear();
}

/**
 * @tc.name: OnDeliveryLowQualityLcd_NEW_IMAGE_test02
 * @tc.desc: pictureInf 如果为 null
 */
HWTEST_F(MultiStagesCaptureDeferredPhotoProcSessionCallbackTest, OnDeliveryLowQualityLcd_NEW_IMAGE_test02,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("enter OnDeliveryLowQualityLcd_NEW_IMAGE_test02");

    // 1.预置数据, MOCK 类型
    FileAssetInfo newImageAssetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    bool ret = CameraTestUtils::PrepareMockPipeline(CameraPipelineType::NEW_IMAGE, newImageAssetInfo);
    ASSERT_EQ(ret, true);

    // 2.执行
    MultiStagesCaptureDeferredPhotoProcSessionCallback* callback =
        new MultiStagesCaptureDeferredPhotoProcSessionCallback();
    ASSERT_NE(callback, nullptr);

    callback->OnDeliveryLowQualityLcd(URI_INPUT, nullptr);
    delete callback;

    // 3.value符合预期
    CameraPipelineType typeByFileId = CameraPipelineType::UNDEFINED;
    auto pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(FILE_ID_INPUT, typeByFileId);
    auto pipelineOutPut = std::static_pointer_cast<MockCameraPipeline>(pipeline);
    ASSERT_NE(pipelineOutPut, nullptr);
    ASSERT_EQ(pipelineOutPut->GetPipelineType(), CameraPipelineType::NEW_IMAGE);
}

/**
 * @tc.name: OnProcessImageDone_IMAGE_test01
 * @tc.desc: [1] 仅 IMAGE 类型的 pipeline 可以执行
 *           [2] IMAGE 正常流程中, 会执行到onProcessImageDone
 *           [3] 仅二阶段流程结束, 不会自动清理相关数据
 */
HWTEST_F(MultiStagesCaptureDeferredPhotoProcSessionCallbackTest, OnProcessImageDone_IMAGE_test01, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter OnProcessImageDone_IMAGE_test01");

    // 1.预置数据, MOCK 类型
    FileAssetInfo imageAssetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    bool ret = CameraTestUtils::PrepareMockPipeline(CameraPipelineType::IMAGE, imageAssetInfo);
    ASSERT_EQ(ret, true);

    CameraPipelineType typeByFileId = CameraPipelineType::UNDEFINED;
    auto pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(FILE_ID_INPUT, typeByFileId);
    auto pipelineOutPut = std::static_pointer_cast<MockCameraPipeline>(pipeline);
    ASSERT_NE(pipelineOutPut, nullptr);
    ASSERT_EQ(pipelineOutPut->GetPipelineType(), CameraPipelineType::IMAGE);
    pipelineOutPut->DoOnProcessImageDone_ = true;

    // 历史场景（待日落）
    MultiStagesCaptureRequestTaskManager::AddPhotoInProgress(FILE_ID_INPUT, PHOTO_ID_INPUT, false);

    // 2.准备数据
    int32_t bytes = 0;
    uint8_t* addr = CameraTestUtils::CreateFileAddr(bytes);
    ASSERT_NE(addr, nullptr);
    ASSERT_NE(bytes, 0);

    // 执行
    MultiStagesCaptureDeferredPhotoProcSessionCallback* callback =
        new MultiStagesCaptureDeferredPhotoProcSessionCallback();
    ASSERT_NE(callback, nullptr);
    callback->OnProcessImageDone(PHOTO_ID_INPUT, addr, bytes, 0);
    delete callback;

    // 3.value 符合预期
    pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(FILE_ID_INPUT, typeByFileId);
    ASSERT_NE(pipeline, nullptr);
}

/**
 * @tc.name: OnProcessImageDone_IMAGE_test02
 * @tc.desc: [1] 仅 IMAGE 类型的 pipeline 可以执行
 *           [2] 基于pipeline流程执行, 没有pipeline, 流程失败
 */
HWTEST_F(MultiStagesCaptureDeferredPhotoProcSessionCallbackTest, OnProcessImageDone_IMAGE_test02, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter OnProcessImageDone_IMAGE_test02");

    // 1.预置数据, MOCK 类型
    FileAssetInfo imageAssetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    bool ret = CameraTestUtils::PrepareMockPipeline(CameraPipelineType::IMAGE, imageAssetInfo);
    ASSERT_EQ(ret, true);

    CameraPipelineType typeByFileId = CameraPipelineType::UNDEFINED;
    auto pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(FILE_ID_INPUT, typeByFileId);
    auto pipelineOutPut = std::static_pointer_cast<MockCameraPipeline>(pipeline);
    ASSERT_NE(pipelineOutPut, nullptr);
    ASSERT_EQ(pipelineOutPut->GetPipelineType(), CameraPipelineType::IMAGE);
    pipelineOutPut->DoOnProcessImageDone_ = true;

    // 2.准备数据
    int32_t bytes = 0;
    uint8_t* addr = CameraTestUtils::CreateFileAddr(bytes);
    ASSERT_NE(addr, nullptr);
    ASSERT_NE(bytes, 0);

    // 执行
    MultiStagesCaptureDeferredPhotoProcSessionCallback* callback =
        new MultiStagesCaptureDeferredPhotoProcSessionCallback();
    ASSERT_NE(callback, nullptr);
    callback->OnProcessImageDone("new", addr, bytes, 0);
    delete callback;

    // 3.value 符合预期
    pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(FILE_ID_INPUT, typeByFileId);
    ASSERT_NE(pipeline, nullptr);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 1);
}

/**
 * @tc.name: OnProcessImageDone_IMAGE_test03
 * @tc.desc: 传入数据失败[addr = nullptr], 流程失败
 */
HWTEST_F(MultiStagesCaptureDeferredPhotoProcSessionCallbackTest, OnProcessImageDone_IMAGE_test03, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter OnProcessImageDone_IMAGE_test03");

    // 1.预置数据, MOCK 类型
    FileAssetInfo imageAssetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    bool ret = CameraTestUtils::PrepareMockPipeline(CameraPipelineType::IMAGE, imageAssetInfo);
    ASSERT_EQ(ret, true);

    // 2.准备数据
    int32_t bytes = 0;
    uint8_t* addr = CameraTestUtils::CreateFileAddr(bytes);
    ASSERT_NE(addr, nullptr);
    ASSERT_NE(bytes, 0);

    // 执行
    MultiStagesCaptureDeferredPhotoProcSessionCallback* callback =
        new MultiStagesCaptureDeferredPhotoProcSessionCallback();
    ASSERT_NE(callback, nullptr);
    callback->OnProcessImageDone(PHOTO_ID_INPUT, nullptr, bytes, 0);    // [addr = nullptr]
    delete callback;

    // 3.value 符合预期
    CameraPipelineType typeByFileId = CameraPipelineType::UNDEFINED;
    auto pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(FILE_ID_INPUT, typeByFileId);
    ASSERT_NE(pipeline, nullptr);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 1);
}

/**
 * @tc.name: OnProcessImageDone_IMAGE_test04
 * @tc.desc: 传入数据失败[bytes = 0], 流程失败
 */
HWTEST_F(MultiStagesCaptureDeferredPhotoProcSessionCallbackTest, OnProcessImageDone_IMAGE_test04, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter OnProcessImageDone_IMAGE_test04");

    // 1.预置数据, MOCK 类型
    FileAssetInfo imageAssetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    bool ret = CameraTestUtils::PrepareMockPipeline(CameraPipelineType::IMAGE, imageAssetInfo);
    ASSERT_EQ(ret, true);

    // 2.准备数据
    int32_t bytes = 0;
    uint8_t* addr = CameraTestUtils::CreateFileAddr(bytes);
    ASSERT_NE(addr, nullptr);
    ASSERT_NE(bytes, 0);

    // 执行
    MultiStagesCaptureDeferredPhotoProcSessionCallback* callback =
        new MultiStagesCaptureDeferredPhotoProcSessionCallback();
    ASSERT_NE(callback, nullptr);
    callback->OnProcessImageDone(PHOTO_ID_INPUT, addr, 0, 0);    // [bytes = 0]
    delete callback;

    // 3.value 符合预期
    CameraPipelineType typeByFileId = CameraPipelineType::UNDEFINED;
    auto pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(FILE_ID_INPUT, typeByFileId);
    ASSERT_NE(pipeline, nullptr);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 1);
}

/**
 * @tc.name: OnProcessImageDone_IMAGE_test05
 * @tc.desc: 数据正确, pipeline流程执行失败, 则不会清理数据
 */
HWTEST_F(MultiStagesCaptureDeferredPhotoProcSessionCallbackTest, OnProcessImageDone_IMAGE_test05, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter OnProcessImageDone_IMAGE_test05");

    // 1.预置数据, MOCK 类型
    FileAssetInfo imageAssetInfo = {
        .fileId = FILE_ID_INPUT,
        .photoId = PHOTO_ID_INPUT,
    };
    bool ret = CameraTestUtils::PrepareMockPipeline(CameraPipelineType::IMAGE, imageAssetInfo);
    ASSERT_EQ(ret, true);

    CameraPipelineType typeByFileId = CameraPipelineType::UNDEFINED;
    auto pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(FILE_ID_INPUT, typeByFileId);
    auto pipelineOutPut = std::static_pointer_cast<MockCameraPipeline>(pipeline);
    ASSERT_NE(pipelineOutPut, nullptr);
    ASSERT_EQ(pipelineOutPut->GetPipelineType(), CameraPipelineType::IMAGE);
    pipelineOutPut->DoOnProcessImageDone_ = false;

    // 2.准备数据
    int32_t bytes = 0;
    uint8_t* addr = CameraTestUtils::CreateFileAddr(bytes);
    ASSERT_NE(addr, nullptr);
    ASSERT_NE(bytes, 0);

    // 执行
    MultiStagesCaptureDeferredPhotoProcSessionCallback* callback =
        new MultiStagesCaptureDeferredPhotoProcSessionCallback();
    ASSERT_NE(callback, nullptr);
    callback->OnProcessImageDone(PHOTO_ID_INPUT, addr, bytes, 0);
    delete callback;

    // 3.value 符合预期
    pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(FILE_ID_INPUT, typeByFileId);
    ASSERT_NE(pipeline, nullptr);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.size(), 1);
    ASSERT_EQ(MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.size(), 1);
}


} // namespace Media
} // namespace OHOS
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

#define MLOG_TAG "MediaCloudSync"

#include "cloud_media_controller_service_test.h"

#include <memory>
#include "media_cloud_sync_test_utils.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unistore_manager.h"

#include "cloud_media_album_controller_service.h"
#include "cloud_media_operation_code.h"
#include "cloud_media_data_controller_service.h"
#include "cloud_media_download_controller_service.h"
#include "cloud_media_photo_controller_service.h"
#include "cloud_media_album_controller_processor.h"
#include "cloud_media_photo_controller_processor.h"

using namespace testing::ext;
using namespace OHOS::Media::IPC;
namespace OHOS::Media::CloudSync {

static constexpr int32_t DATA_OPRN_COUNT = 17;
static constexpr int32_t ALBUM_OPRN_COUNT = 16;
static constexpr int32_t PHOTO_OPRN_COUNT = 18;
static constexpr int32_t DOWNLOAD_OPRN_COUNT = 6;
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;

void CloudMediaContorllerServiceTest::SetUpTestCase()
{
    GTEST_LOG_(INFO) << "SetUpTestCase";
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        GTEST_LOG_(ERROR) << "init g_rdbStore failed";
        exit(1);
    }
    InitTestTables(g_rdbStore);
}

void CloudMediaContorllerServiceTest::TearDownTestCase()
{
    GTEST_LOG_(INFO) << "TearDownTestCase";
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
}

void CloudMediaContorllerServiceTest::SetUp() {}

void CloudMediaContorllerServiceTest::TearDown() {}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_Accept_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    EXPECT_EQ(service->Accept(0), false);
    EXPECT_EQ(service->Accept(static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_DENTRY_FILE_INSERT)), true);
    EXPECT_EQ(service->Accept(static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_CREATE_RECORDS)), true);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_OnRemoteRequest_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    service->OnRemoteRequest(0, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_IPC_SEVICE_NOT_FOUND);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_OnFetchRecords_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_FETCH_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_OnFetchRecords_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(1);
    data.WriteString("test1");
    data.WriteString("test2");
    data.WriteString("test3");
    data.WriteString("test4");
    data.WriteString("test5");
    data.WriteInt32(1);
    data.WriteInt32(2);
    data.WriteInt32(3);
    data.WriteInt32(4);
    data.WriteInt64(5);
    data.WriteInt64(6);
    data.WriteInt64(7);
    data.WriteBool(false);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_FETCH_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_OnDentryFileInsert_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_DENTRY_FILE_INSERT);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, 0);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_GetCreatedRecords_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_GET_CREATED_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_GetCreatedRecords_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(0);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_GET_CREATED_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_MEDIA_CLOUD_ARGS_INVAILD);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_GetCreatedRecords_Test_003, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_GET_CREATED_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_GetMetaModifiedRecords_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_GET_META_MODIFIED_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_GetMetaModifiedRecords_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(0);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_GET_META_MODIFIED_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_MEDIA_CLOUD_ARGS_INVAILD);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_GetMetaModifiedRecords_Test_003, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_GET_META_MODIFIED_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_GetDeletedRecords_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_GET_DELETED_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_GetDeletedRecords_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(0);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_GET_DELETED_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_MEDIA_CLOUD_ARGS_INVAILD);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_GetDeletedRecords_Test_003, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_GET_DELETED_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_OnCreateRecords_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_CREATE_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_OnCreateRecords_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(-1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_CREATE_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_OnCreateRecords_Test_003, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(1);
    data.WriteBool(false);
    data.WriteString("test1");
    data.WriteString("test2");
    data.WriteString("test3");
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_CREATE_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_OnMdirtyRecords_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_MDIRTY_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_OnMdirtyRecords_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(-1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_MDIRTY_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_OnMdirtyRecords_Test_003, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(1);
    data.WriteString("test1");
    data.WriteBool(false);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_MDIRTY_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_OnFdirtyRecords_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_FDIRTY_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_OnDeleteRecords_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_DELETE_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_OnDeleteRecords_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(-1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_DELETE_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_OnDeleteRecords_Test_003, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(1);
    data.WriteString("test1");
    data.WriteBool(false);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_DELETE_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_OnCopyRecords_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_COPY_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_OnStartSync_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_START_SYNC);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_OnCompleteSync_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_COMPLETE_SYNC);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_OnCompletePull_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_COMPLETE_PULL);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_OnCompletePush_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_COMPLETE_PUSH);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_OnCompleteCheck_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaAlbumControllerService> service = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaAlbumOperationCode::CMD_ON_COMPLETE_CHECK);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_Accept_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDataControllerService> service = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(service != nullptr);

    EXPECT_EQ(service->Accept(0), false);
    EXPECT_EQ(service->Accept(static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_DIRTY_FOR_CLOUD_CHECK)), true);
    EXPECT_EQ(service->Accept(static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_VIDEO_TO_CACHE)), true);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_OnRemoteRequest_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDataControllerService> service = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    service->OnRemoteRequest(0, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_IPC_SEVICE_NOT_FOUND);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_UpdateDirty_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDataControllerService> service = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_DIRTY_FOR_CLOUD_CHECK);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_UpdateDirty_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDataControllerService> service = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteString("test1");
    data.WriteInt32(0);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_DIRTY_FOR_CLOUD_CHECK);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_UpdatePosition_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDataControllerService> service = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_POSITION_FOR_CLOUD_CHECK);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_UpdatePosition_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDataControllerService> service = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(0);
    data.WriteInt32(-1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_POSITION_FOR_CLOUD_CHECK);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_UpdatePosition_Test_003, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDataControllerService> service = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(0);
    data.WriteInt32(1);
    data.WriteString("test1");
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_POSITION_FOR_CLOUD_CHECK);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_UpdateThmStatus_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDataControllerService> service = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_THM_STATUS_FOR_CLOUD_CHECK);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_UpdateThmStatus_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDataControllerService> service = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteString("test1");
    data.WriteInt32(1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_THM_STATUS_FOR_CLOUD_CHECK);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_GetVideoToCache_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDataControllerService> service = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_VIDEO_TO_CACHE);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_GetFilePosStat_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDataControllerService> service = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_FILE_POS_STAT);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_GetCloudThmStat_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDataControllerService> service = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_CLOUD_THM_STAT);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_GetDirtyTypeStat_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDataControllerService> service = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_DIRTY_TYPE_STAT);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_GetAgingFile_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDataControllerService> service = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_AGING_ASSET);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_MEDIA_CLOUD_ARGS_INVAILD);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_GetAgingFile_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDataControllerService> service = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt64(0);
    data.WriteInt32(1);
    data.WriteInt32(-1);
    data.WriteInt32(20);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_AGING_ASSET);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_MEDIA_CLOUD_ARGS_INVAILD);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_GetAgingFile_Test_003, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDataControllerService> service = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt64(0);
    data.WriteInt32(1);
    data.WriteInt32(1);
    data.WriteInt32(20);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_AGING_ASSET);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_GetActiveAgingFile_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDataControllerService> service = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_ACTIVE_AGING_ASSET);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_MEDIA_CLOUD_ARGS_INVAILD);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_GetActiveAgingFile_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDataControllerService> service = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt64(0);
    data.WriteInt32(1);
    data.WriteInt32(-1);
    data.WriteInt32(20);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_ACTIVE_AGING_ASSET);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_MEDIA_CLOUD_ARGS_INVAILD);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_GetActiveAgingFile_Test_003, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDataControllerService> service = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt64(0);
    data.WriteInt32(1);
    data.WriteInt32(1);
    data.WriteInt32(20);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_ACTIVE_AGING_ASSET);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_UpdateLocalFileDirty_Test_001,
         TestSize.Level1)
{
    std::shared_ptr<CloudMediaDataControllerService> service = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_LOCAL_FILE_DIRTY);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_UpdateLocalFileDirty_Test_002,
         TestSize.Level1)
{
    std::shared_ptr<CloudMediaDataControllerService> service = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(-1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_LOCAL_FILE_DIRTY);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_UpdateLocalFileDirty_Test_003,
         TestSize.Level1)
{
    std::shared_ptr<CloudMediaDataControllerService> service = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt64(0);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_LOCAL_FILE_DIRTY);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_UpdateSyncStatus_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDataControllerService> service = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_SYNC_STATUS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_UpdateSyncStatus_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDataControllerService> service = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteString("test1");
    data.WriteInt32(-1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_UPDATE_SYNC_STATUS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDownloadControllerService_Accept_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDownloadControllerService> service =
        std::make_shared<CloudMediaDownloadControllerService>();
    ASSERT_TRUE(service != nullptr);

    EXPECT_EQ(service->Accept(0), false);
    EXPECT_EQ(service->Accept(static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_DOWNLOAD_THM)), true);
    EXPECT_EQ(service->Accept(static_cast<uint32_t>(CloudMediaOperationCode::CMD_ON_DOWNLOAD_THMS)), true);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDownloadControllerService_OnRemoteRequest_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDownloadControllerService> service =
        std::make_shared<CloudMediaDownloadControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    service->OnRemoteRequest(0, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_IPC_SEVICE_NOT_FOUND);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDownloadControllerService_GetDownloadThms_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDownloadControllerService> service =
        std::make_shared<CloudMediaDownloadControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_DOWNLOAD_THM);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_MEDIA_CLOUD_ARGS_INVAILD);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDownloadControllerService_GetDownloadThms_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDownloadControllerService> service =
        std::make_shared<CloudMediaDownloadControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(-1);
    data.WriteInt32(0);
    data.WriteInt32(10);
    data.WriteBool(true);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_DOWNLOAD_THM);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_MEDIA_CLOUD_ARGS_INVAILD);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDownloadControllerService_GetDownloadThms_Test_003, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDownloadControllerService> service =
        std::make_shared<CloudMediaDownloadControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(1);
    data.WriteInt32(0);
    data.WriteInt32(10);
    data.WriteBool(true);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_DOWNLOAD_THM);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDownloadControllerService_GetDownloadThmNum_Test_001,
         TestSize.Level1)
{
    std::shared_ptr<CloudMediaDownloadControllerService> service =
        std::make_shared<CloudMediaDownloadControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_DOWNLOAD_THM_NUM);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDownloadControllerService_GetDownloadThmNum_Test_002,
         TestSize.Level1)
{
    std::shared_ptr<CloudMediaDownloadControllerService> service =
        std::make_shared<CloudMediaDownloadControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_DOWNLOAD_THM_NUM);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDownloadControllerService_GetDownloadThmsByUri_Test_001,
         TestSize.Level1)
{
    std::shared_ptr<CloudMediaDownloadControllerService> service =
        std::make_shared<CloudMediaDownloadControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_DOWNLOAD_THM_BY_URI);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDownloadControllerService_GetDownloadThmsByUri_Test_002,
         TestSize.Level1)
{
    std::shared_ptr<CloudMediaDownloadControllerService> service =
        std::make_shared<CloudMediaDownloadControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(1);
    data.WriteInt32(-1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_DOWNLOAD_THM_BY_URI);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDownloadControllerService_OnDownloadThms_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDownloadControllerService> service =
        std::make_shared<CloudMediaDownloadControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_ON_DOWNLOAD_THMS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDownloadControllerService_OnDownloadThms_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDownloadControllerService> service =
        std::make_shared<CloudMediaDownloadControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(-1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_ON_DOWNLOAD_THMS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_MEDIA_CLOUD_ARGS_INVAILD);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDownloadControllerService_OnDownloadThms_Test_003, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDownloadControllerService> service =
        std::make_shared<CloudMediaDownloadControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(1);
    data.WriteString("test1");
    data.WriteInt32(0);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_ON_DOWNLOAD_THMS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDownloadControllerService_GetDownloadAsset_Test_001,
         TestSize.Level1)
{
    std::shared_ptr<CloudMediaDownloadControllerService> service =
        std::make_shared<CloudMediaDownloadControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_DOWNLOAD_ASSET);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDownloadControllerService_GetDownloadAsset_Test_002,
         TestSize.Level1)
{
    std::shared_ptr<CloudMediaDownloadControllerService> service =
        std::make_shared<CloudMediaDownloadControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(-1);
    data.WriteInt32(-1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_GET_DOWNLOAD_ASSET);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDownloadControllerService_OnDownloadAsset_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDownloadControllerService> service =
        std::make_shared<CloudMediaDownloadControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_ON_DOWNLOAD_ASSET);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDownloadControllerService_OnDownloadAsset_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaDownloadControllerService> service =
        std::make_shared<CloudMediaDownloadControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(-1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaOperationCode::CMD_ON_DOWNLOAD_ASSET);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_Accept_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    EXPECT_EQ(service->Accept(0), false);
    EXPECT_EQ(service->Accept(static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_DENTRY_FILE_INSERT)), true);
    EXPECT_EQ(service->Accept(static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_CHECK_RECORDS)), true);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnRemoteRequest_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    service->OnRemoteRequest(0, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_IPC_SEVICE_NOT_FOUND);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnFetchRecords_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_FETCH_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnFetchRecords_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(-1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_FETCH_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnDentryFileInsert_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_DENTRY_FILE_INSERT);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnDentryFileInsert_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(-1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_DENTRY_FILE_INSERT);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_GetCreatedRecords_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_CREATED_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_NE(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_GetCreatedRecords_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(-1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_CREATED_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_MEDIA_CLOUD_ARGS_INVAILD);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_GetCreatedRecords_Test_003, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_CREATED_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_GetMetaModifiedRecords_Test_001,
         TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_META_MODIFIED_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_NE(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_GetMetaModifiedRecords_Test_002,
         TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(-1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_META_MODIFIED_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_MEDIA_CLOUD_ARGS_INVAILD);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_GetMetaModifiedRecords_Test_003,
         TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_META_MODIFIED_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_NE(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_GetFileModifiedRecords_Test_001,
         TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_FILE_MODIFIED_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_NE(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_GetFileModifiedRecords_Test_002,
         TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(-1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_FILE_MODIFIED_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_MEDIA_CLOUD_ARGS_INVAILD);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_GetFileModifiedRecords_Test_003,
         TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_FILE_MODIFIED_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_GetDeletedRecords_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_DELETED_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_NE(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_GetDeletedRecords_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(-1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_DELETED_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_MEDIA_CLOUD_ARGS_INVAILD);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_GetDeletedRecords_Test_003, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_DELETED_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_GetCopyRecords_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_COPY_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_NE(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_GetCopyRecords_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(-1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_COPY_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_MEDIA_CLOUD_ARGS_INVAILD);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_GetCopyRecords_Test_003, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_COPY_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_GetCheckRecords_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_CHECK_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_MEDIA_CLOUD_ARGS_INVAILD);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_GetCheckRecords_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(-1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_CHECK_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_GetCheckRecords_Test_003, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(1);
    data.WriteString("test1");
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_CHECK_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnCreateRecords_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_CREATE_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnCreateRecords_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(-1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_CREATE_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnCreateRecords_Test_003, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(1);
    data.WriteString("test1");
    data.WriteInt32(0);
    data.WriteInt32(1);
    data.WriteInt32(2);
    data.WriteInt32(3);
    data.WriteInt64(4);
    data.WriteInt64(5);
    data.WriteInt64(6);
    data.WriteInt64(7);
    data.WriteInt64(8);
    data.WriteString("test1");
    data.WriteString("test1");
    data.WriteString("test1");
    data.WriteString("test1");
    data.WriteInt64(9);
    data.WriteInt32(10);
    data.WriteBool(false);
    data.WriteInt32(0);
    data.WriteInt32(1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_CREATE_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnMdirtyRecords_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_MDIRTY_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnMdirtyRecords_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(-1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_MDIRTY_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnMdirtyRecords_Test_003, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(1);
    data.WriteString("test1");
    data.WriteString("test2");
    data.WriteInt32(0);
    data.WriteInt64(1);
    data.WriteInt64(2);
    data.WriteInt64(3);
    data.WriteBool(true);
    data.WriteInt32(0);
    data.WriteInt32(-1);
    data.WriteInt32(0);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_MDIRTY_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnFdirtyRecords_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_FDIRTY_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnFdirtyRecords_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(-1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_FDIRTY_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnFdirtyRecords_Test_003, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(1);
    data.WriteString("test");
    data.WriteInt32(1);
    data.WriteInt32(2);
    data.WriteInt32(3);
    data.WriteInt64(4);
    data.WriteInt64(5);
    data.WriteInt64(6);
    data.WriteInt64(7);
    data.WriteString("test");
    data.WriteString("test");
    data.WriteString("test");
    data.WriteInt64(8);
    data.WriteInt32(9);
    data.WriteBool(false);
    data.WriteInt32(0);
    data.WriteInt32(0);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_FDIRTY_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnDeleteRecords_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_DELETE_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnDeleteRecords_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(-1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_DELETE_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnDeleteRecords_Test_003, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(1);
    data.WriteString("test1");
    data.WriteString("test2");
    data.WriteBool(true);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_DELETE_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnCopyRecords_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_COPY_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnCopyRecords_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(-1);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_COPY_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnCopyRecords_Test_003, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(1);
    data.WriteString("test");
    data.WriteInt32(1);
    data.WriteInt32(2);
    data.WriteInt32(3);
    data.WriteInt64(4);
    data.WriteInt64(5);
    data.WriteString("test");
    data.WriteString("test");
    data.WriteString("test");
    data.WriteInt64(6);
    data.WriteInt32(7);
    data.WriteBool(false);
    data.WriteInt32(0);
    data.WriteInt32(0);
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_COPY_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_GetRetryRecords_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_GET_RETRY_RECORDS);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnStartSync_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_START_SYNC);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnCompleteSync_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_COMPLETE_SYNC);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnCompletePull_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_COMPLETE_PULL);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnCompletePush_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_COMPLETE_PUSH);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_OnCompleteCheck_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_ON_COMPLETE_CHECK);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_ReportFailure_Test_001, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_REPORT_FAILURE);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_ReportFailure_Test_002, TestSize.Level1)
{
    std::shared_ptr<CloudMediaPhotoControllerService> service = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(service != nullptr);

    MessageParcel data;
    data.WriteInt32(1);
    data.WriteInt32(2);
    data.WriteInt32(3);
    data.WriteString("test");
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    uint32_t code = static_cast<uint32_t>(CloudMediaPhotoOperationCode::CMD_REPORT_FAILURE);
    service->OnRemoteRequest(code, data, reply, context);

    int32_t ret = reply.ReadInt32();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDataControllerService_Test, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    auto services = std::make_shared<CloudMediaDataControllerService>();
    ASSERT_TRUE(services);

    std::array<CloudMediaOperationCode, DATA_OPRN_COUNT> operations = {{
        CloudMediaOperationCode::CMD_UPDATE_DIRTY_FOR_CLOUD_CHECK,
        CloudMediaOperationCode::CMD_UPDATE_POSITION_FOR_CLOUD_CHECK,
        CloudMediaOperationCode::CMD_UPDATE_THM_STATUS_FOR_CLOUD_CHECK,
        CloudMediaOperationCode::CMD_GET_DOWNLOAD_ASSET,
        CloudMediaOperationCode::CMD_GET_DOWNLOAD_THM,
        CloudMediaOperationCode::CMD_GET_VIDEO_TO_CACHE,
        CloudMediaOperationCode::CMD_GET_FILE_POS_STAT,
        CloudMediaOperationCode::CMD_GET_CLOUD_THM_STAT,
        CloudMediaOperationCode::CMD_GET_DIRTY_TYPE_STAT,
        CloudMediaOperationCode::CMD_GET_AGING_ASSET,
        CloudMediaOperationCode::CMD_GET_ACTIVE_AGING_ASSET,
        CloudMediaOperationCode::CMD_ON_DOWNLOAD_ASSET,
        CloudMediaOperationCode::CMD_ON_DOWNLOAD_THMS,
        CloudMediaOperationCode::CMD_GET_DOWNLOAD_THM_NUM,
        CloudMediaOperationCode::CMD_UPDATE_LOCAL_FILE_DIRTY,
        CloudMediaOperationCode::CMD_GET_DOWNLOAD_THM_BY_URI,
        CloudMediaOperationCode::CMD_UPDATE_SYNC_STATUS,
    }};
    for (const auto &operation : operations) {
        services->OnRemoteRequest(static_cast<uint32_t>(operation), data, reply, context);
    }
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumController_Test, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    auto services = std::make_shared<CloudMediaAlbumControllerService>();
    ASSERT_TRUE(services);

    std::array<CloudMediaAlbumOperationCode, ALBUM_OPRN_COUNT> operations = {{
        CloudMediaAlbumOperationCode::CMD_ON_FETCH_RECORDS,
        CloudMediaAlbumOperationCode::CMD_ON_DENTRY_FILE_INSERT,
        CloudMediaAlbumOperationCode::CMD_GET_CREATED_RECORDS,
        CloudMediaAlbumOperationCode::CMD_GET_META_MODIFIED_RECORDS,
        CloudMediaAlbumOperationCode::CMD_GET_DELETED_RECORDS,
        CloudMediaAlbumOperationCode::CMD_ON_CREATE_RECORDS,
        CloudMediaAlbumOperationCode::CMD_ON_MDIRTY_RECORDS,
        CloudMediaAlbumOperationCode::CMD_ON_FDIRTY_RECORDS,
        CloudMediaAlbumOperationCode::CMD_ON_DELETE_RECORDS,
        CloudMediaAlbumOperationCode::CMD_ON_COPY_RECORDS,
        CloudMediaAlbumOperationCode::CMD_ON_START_SYNC,
        CloudMediaAlbumOperationCode::CMD_ON_COMPLETE_SYNC,
        CloudMediaAlbumOperationCode::CMD_ON_COMPLETE_PULL,
        CloudMediaAlbumOperationCode::CMD_ON_COMPLETE_PUSH,
        CloudMediaAlbumOperationCode::CMD_ON_COMPLETE_CHECK,
    }};
    for (const auto &operation : operations) {
        services->OnRemoteRequest(static_cast<uint32_t>(operation), data, reply, context);
    }
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaPhotoControllerService_Test, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    auto services = std::make_shared<CloudMediaPhotoControllerService>();
    ASSERT_TRUE(services);

    std::array<CloudMediaPhotoOperationCode, PHOTO_OPRN_COUNT> operations = {{
        CloudMediaPhotoOperationCode::CMD_ON_FETCH_RECORDS,
        CloudMediaPhotoOperationCode::CMD_ON_DENTRY_FILE_INSERT,
        CloudMediaPhotoOperationCode::CMD_GET_CREATED_RECORDS,
        CloudMediaPhotoOperationCode::CMD_GET_META_MODIFIED_RECORDS,
        CloudMediaPhotoOperationCode::CMD_GET_FILE_MODIFIED_RECORDS,
        CloudMediaPhotoOperationCode::CMD_GET_DELETED_RECORDS,
        CloudMediaPhotoOperationCode::CMD_GET_COPY_RECORDS,
        CloudMediaPhotoOperationCode::CMD_GET_CHECK_RECORDS,
        CloudMediaPhotoOperationCode::CMD_ON_CREATE_RECORDS,
        CloudMediaPhotoOperationCode::CMD_ON_MDIRTY_RECORDS,
        CloudMediaPhotoOperationCode::CMD_ON_FDIRTY_RECORDS,
        CloudMediaPhotoOperationCode::CMD_ON_DELETE_RECORDS,
        CloudMediaPhotoOperationCode::CMD_ON_COPY_RECORDS,
        CloudMediaPhotoOperationCode::CMD_GET_RETRY_RECORDS,
        CloudMediaPhotoOperationCode::CMD_ON_COMPLETE_SYNC,
        CloudMediaPhotoOperationCode::CMD_ON_COMPLETE_PULL,
        CloudMediaPhotoOperationCode::CMD_ON_COMPLETE_PUSH,
        CloudMediaPhotoOperationCode::CMD_ON_COMPLETE_CHECK,
    }};

    for (const auto &operation : operations) {
        services->OnRemoteRequest(static_cast<uint32_t>(operation), data, reply, context);
    }
};

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDownloadControllerService_Test, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    IPCContext context(MessageOption(), 0);
    auto services = std::make_shared<CloudMediaDownloadControllerService>();
    ASSERT_TRUE(services);

    std::array<CloudMediaOperationCode, DOWNLOAD_OPRN_COUNT> operations = {{
        CloudMediaOperationCode::CMD_GET_DOWNLOAD_THM,
        CloudMediaOperationCode::CMD_GET_DOWNLOAD_THM_NUM,
        CloudMediaOperationCode::CMD_GET_DOWNLOAD_THM_BY_URI,
        CloudMediaOperationCode::CMD_ON_DOWNLOAD_THMS,
        CloudMediaOperationCode::CMD_GET_DOWNLOAD_ASSET,
        CloudMediaOperationCode::CMD_ON_DOWNLOAD_ASSET,
    }};

    for (const auto &operation : operations) {
        services->OnRemoteRequest(static_cast<uint32_t>(operation), data, reply, context);
    }
}

HWTEST_F(CloudMediaContorllerServiceTest, SetFdirtyDataVoFromDto_Test, TestSize.Level1)
{
    auto processor = std::make_shared<CloudMediaPhotoControllerProcessor>();
    ASSERT_TRUE(processor);
    std::vector<PhotosDto> fdirtyDataDtos;
    auto result = processor->SetFdirtyDataVoFromDto(fdirtyDataDtos);
    EXPECT_EQ(result.empty(), true);

    PhotosDto photosDto;
    CloudFileDataDto dataDto;
    photosDto.attachment["001"] = dataDto;
    fdirtyDataDtos.emplace_back(photosDto);
    result = processor->SetFdirtyDataVoFromDto(fdirtyDataDtos);
    EXPECT_EQ(result.empty(), false);
}

HWTEST_F(CloudMediaContorllerServiceTest, SetNewDataVoFromDto_Test, TestSize.Level1)
{
    auto processor = std::make_shared<CloudMediaPhotoControllerProcessor>();
    ASSERT_TRUE(processor);
    vector<PhotosDto> newDataDtos;
    auto result = processor->SetNewDataVoFromDto(newDataDtos);
    EXPECT_EQ(result.empty(), true);

    PhotosDto photosDto;
    CloudFileDataDto dataDto;
    photosDto.attachment["002"] = dataDto;
    newDataDtos.emplace_back(photosDto);
    result = processor->SetNewDataVoFromDto(newDataDtos);
    EXPECT_EQ(result.empty(), false);
}

HWTEST_F(CloudMediaContorllerServiceTest, GetCheckRecordsRespBody_Test, TestSize.Level1)
{
    auto processor = std::make_shared<CloudMediaPhotoControllerProcessor>();
    ASSERT_TRUE(processor);
    vector<PhotosDto> photosDtoVec;
    auto result = processor->GetCheckRecordsRespBody(photosDtoVec);
    EXPECT_EQ(result.empty(), true);

    PhotosDto photosDto;
    CloudFileDataDto dataDto;
    photosDto.attachment["002"] = dataDto;
    photosDtoVec.emplace_back(photosDto);
    result = processor->GetCheckRecordsRespBody(photosDtoVec);
    EXPECT_EQ(result.empty(), false);
}

HWTEST_F(CloudMediaContorllerServiceTest, ConvertRecordPoToVo_Test, TestSize.Level1)
{
    auto processor = std::make_shared<CloudMediaPhotoControllerProcessor>();
    ASSERT_TRUE(processor);
    PhotosPo record;
    processor->ConvertRecordPoToVo(record);
}

HWTEST_F(CloudMediaContorllerServiceTest, ConvertToCloudMediaPullData_Test, TestSize.Level1)
{
    auto processor = std::make_shared<CloudMediaPhotoControllerProcessor>();
    ASSERT_TRUE(processor);
    OnFetchPhotosVo photosVo;
    processor->ConvertToCloudMediaPullData(photosVo);
}

HWTEST_F(CloudMediaContorllerServiceTest, ConvertToPhotoDto_Test, TestSize.Level1)
{
    auto processor = std::make_shared<CloudMediaPhotoControllerProcessor>();
    ASSERT_TRUE(processor);
    OnCreateRecord recordVo;
    processor->ConvertToPhotoDto(recordVo);
}

HWTEST_F(CloudMediaContorllerServiceTest, PhotoConvertToPhotosDto_Test, TestSize.Level1)
{
    PhotosDto dto;
    OnFileDirtyRecord recordVo;
    recordVo.cloudId = "test";

    auto processor = std::make_shared<CloudMediaPhotoControllerProcessor>();
    ASSERT_TRUE(processor);
    processor->ConvertToPhotosDto(recordVo, dto);
    EXPECT_EQ(dto.cloudId, "test");

    OnModifyRecord modifyRe;
    modifyRe.cloudId = "test2";
    processor->ConvertToPhotosDto(modifyRe, dto);
    EXPECT_EQ(dto.cloudId, "test2");
}

HWTEST_F(CloudMediaContorllerServiceTest, AlbumConvertRecordPoToVo_Test, TestSize.Level1)
{
    auto processor = std::make_shared<CloudMediaAlbumControllerProcessor>();
    ASSERT_TRUE(processor);
    PhotoAlbumPo record;
    processor->ConvertRecordPoToVo(record);
}

HWTEST_F(CloudMediaContorllerServiceTest, DataControllerProcessorVoToDto_Test, TestSize.Level1)
{
    PhotosVo photosVo;
    CloudFileDataVo dataVo;
    photosVo.cloudId = "test";
    photosVo.attachment["001"] = dataVo;
    auto processor = std::make_shared<CloudMediaDataControllerProcessor>();
    ASSERT_TRUE(processor);

    PhotosDto dto = processor->ConvertPhotosVoToPhotosDto(photosVo);
    EXPECT_EQ(dto.cloudId, "test");
}

HWTEST_F(CloudMediaContorllerServiceTest, DataControllerProcessorDoToVo_Test, TestSize.Level1)
{
    PhotosDto dto;
    CloudFileDataDto dataDto;
    dto.cloudId = "test1";
    dto.attachment["001"] = dataDto;
    auto processor = std::make_shared<CloudMediaDataControllerProcessor>();
    ASSERT_TRUE(processor);

    PhotosVo photoVo = processor->ConvertPhotosDtoToPhotosVo(dto);
    EXPECT_EQ(photoVo.cloudId, "test1");
}

HWTEST_F(CloudMediaContorllerServiceTest, DataControllerProcessorGetRes_Test, TestSize.Level1)
{
    GetAgingFileReqBody reqBody;
    AgingFileQueryDto queryDto;
    MediaOperateResultDto dto1;
    MediaOperateResultDto dto2;
    std::vector<MediaOperateResultDto> resultDto;
    resultDto.emplace_back(dto1);
    resultDto.emplace_back(dto2);

    auto processor = std::make_shared<CloudMediaDataControllerProcessor>();
    ASSERT_TRUE(processor);
    auto result = processor->GetMediaOperateResult(resultDto);
    EXPECT_EQ(result.size(), 2);
    processor->GetAgingFileQueryDto(reqBody, queryDto);
}

HWTEST_F(CloudMediaContorllerServiceTest, CloudMediaDownloadControllerProcessor_Test, TestSize.Level1)
{
    PhotosDto photosDto;
    CloudFileDataDto dataDto;
    photosDto.cloudId = "test";
    photosDto.attachment["001"] = dataDto;
    auto processor = std::make_shared<CloudMediaDownloadControllerProcessor>();
    auto result = processor->ConvertPhotosDtoToPhotosVo(photosDto);
    EXPECT_EQ(result.cloudId, "test");

    GetDownloadThmReqBody reqBody;
    processor->GetDownloadThumbnailQueryDto(reqBody);

    MediaOperateResultDto dto1;
    MediaOperateResultDto dto2;
    std::vector<MediaOperateResultDto> mediaOperateResultDto;
    mediaOperateResultDto.emplace_back(dto1);
    mediaOperateResultDto.emplace_back(dto2);
    auto opResult = processor->GetMediaOperateResult(mediaOperateResultDto);
    EXPECT_EQ(opResult.size(), 2);
}
}
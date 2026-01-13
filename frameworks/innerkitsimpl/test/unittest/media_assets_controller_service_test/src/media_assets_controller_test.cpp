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

#define MLOG_TAG "MediaAssetsControllerTest"

#include "media_assets_controller_test.h"

#include <memory>
#include <string>

#define private public
#define protected public
#include "media_assets_controller_service.h"
#undef private
#undef protected

#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"
#include "media_file_uri.h"

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

void MediaAssetsControllerTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get g_rdbStore");
        exit(1);
    }
    ClearTable(PhotoAlbumColumns::TABLE);
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("SetUpTestCase");
}

void MediaAssetsControllerTest::TearDownTestCase(void)
{
    ClearTable(PhotoAlbumColumns::TABLE);
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void MediaAssetsControllerTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void MediaAssetsControllerTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(MediaAssetsControllerTest, OnRemoteRequest_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("OnRemoteRequest_Test_001 enter");

    auto controller = make_shared<MediaAssetsControllerService>();
    uint32_t start = static_cast<uint32_t>(MediaLibraryBusinessCode::ASSETS_BUSINESS_CODE_START);
    uint32_t end = static_cast<uint32_t>(MediaLibraryBusinessCode::ASSETS_BUSINESS_CODE_END);
    for (uint32_t code = start; code < end; ++code) {
        MessageParcel data;
        data.WriteInt32(INT32_MIN);
        MessageParcel reply;
        reply.WriteInt32(INT32_MIN);
        MessageOption option;
        IPC::IPCContext context(option, E_PERMISSION_DB_BYPASS);
        controller->OnRemoteRequest(code, data, reply, context);
        IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
        ASSERT_EQ(respVo.Unmarshalling(reply), true);
        MEDIA_INFO_LOG("OnRemoteRequest Unmarshalling ErrCode:%{public}d", respVo.GetErrCode());
        ASSERT_LE(respVo.GetErrCode(), 0);
    }

    MEDIA_INFO_LOG("OnRemoteRequest_Test_001 end");
}

HWTEST_F(MediaAssetsControllerTest, OnRemoteRequest_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("OnRemoteRequest_Test_002 enter");

    auto controller = make_shared<MediaAssetsControllerService>();
    uint32_t start = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_BUSINESS_CODE_START);
    uint32_t end = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_BUSINESS_CODE_END);
    for (uint32_t code = start; code < end; ++code) {
        MessageParcel data;
        data.WriteInt32(INT32_MIN);
        MessageParcel reply;
        reply.WriteInt32(INT32_MIN);
        MessageOption option;
        IPC::IPCContext context(option, E_PERMISSION_DB_BYPASS);
        controller->OnRemoteRequest(code, data, reply, context);
        IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
        ASSERT_EQ(respVo.Unmarshalling(reply), true);
        MEDIA_INFO_LOG("OnRemoteRequest Unmarshalling ErrCode:%{public}d", respVo.GetErrCode());
        ASSERT_LE(respVo.GetErrCode(), 0);
    }

    MEDIA_INFO_LOG("OnRemoteRequest_Test_002 end");
}

HWTEST_F(MediaAssetsControllerTest, OnRemoteRequest_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("OnRemoteRequest_Test_003 enter");

    auto controller = make_shared<MediaAssetsControllerService>();
    uint32_t start = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_BUSINESS_CODE_START);
    uint32_t end = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_BUSINESS_CODE_END);
    for (uint32_t code = start; code < end; ++code) {
        MessageParcel data;
        data.WriteInt32(INT32_MIN);
        MessageParcel reply;
        reply.WriteInt32(INT32_MIN);
        MessageOption option;
        IPC::IPCContext context(option, E_DOUBLE_CHECK);
        controller->OnRemoteRequest(code, data, reply, context);
        IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
        ASSERT_EQ(respVo.Unmarshalling(reply), true);
        MEDIA_INFO_LOG("OnRemoteRequest Unmarshalling ErrCode:%{public}d", respVo.GetErrCode());
        ASSERT_LE(respVo.GetErrCode(), 0);
    }

    MEDIA_INFO_LOG("OnRemoteRequest_Test_003 end");
}
}  // namespace OHOS::Media
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

#define MLOG_TAG "MediaAssetsControllerTest"

#include "media_assets_controller_ipc_test.h"

#include <memory>
#include <string>

#include "media_assets_controller_service.h"

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

void MediaAssetsControllerIpcTest::SetUpTestCase(void)
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

void MediaAssetsControllerIpcTest::TearDownTestCase(void)
{
    ClearTable(PhotoAlbumColumns::TABLE);
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void MediaAssetsControllerIpcTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void MediaAssetsControllerIpcTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(MediaAssetsControllerIpcTest, Accept_RegisteredCode_ReturnsTrue, TestSize.Level0)
{
    MEDIA_INFO_LOG("Accept_RegisteredCode_ReturnsTrue enter");
    auto controller = make_shared<MediaAssetsControllerService>();
    EXPECT_TRUE(controller->Accept(static_cast<uint32_t>(MediaLibraryBusinessCode::REMOVE_FORM_INFO)));
    EXPECT_TRUE(controller->Accept(static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_HIDDEN)));
    EXPECT_TRUE(controller->Accept(static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ASSETS)));
    MEDIA_INFO_LOG("Accept_RegisteredCode_ReturnsTrue end");
}

HWTEST_F(MediaAssetsControllerIpcTest, Accept_UnregisteredCode_ReturnsFalse, TestSize.Level0)
{
    MEDIA_INFO_LOG("Accept_UnregisteredCode_ReturnsFalse enter");
    auto controller = make_shared<MediaAssetsControllerService>();
    EXPECT_FALSE(controller->Accept(0xFFFFFFFF));
    MEDIA_INFO_LOG("Accept_UnregisteredCode_ReturnsFalse end");
}

HWTEST_F(MediaAssetsControllerIpcTest, OnRemoteRequest_UnknownCode_ReturnsError, TestSize.Level0)
{
    MEDIA_INFO_LOG("OnRemoteRequest_UnknownCode_ReturnsError enter");
    auto controller = make_shared<MediaAssetsControllerService>();
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    IPC::IPCContext context(option, 0);
    controller->OnRemoteRequest(0xFFFFFFFF, data, reply, context);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> resp;
    ASSERT_EQ(resp.Unmarshalling(reply), true);
    EXPECT_LT(resp.GetErrCode(), 0);
    MEDIA_INFO_LOG("OnRemoteRequest_UnknownCode_ReturnsError end");
}
}  // namespace OHOS::Media

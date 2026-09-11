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

#define MLOG_TAG "MediaAssetsControllerServiceTest"

#include "create_file_manager_asset_test.h"

#include <memory>
#include <string>

#include "message_parcel.h"
#include "medialibrary_errno.h"
#include "media_assets_controller_service.h"
#include "user_define_ipc.h"
#include "create_asset_vo.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace IPC;

void CreateFileManagerAssetTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("CreateFileManagerAssetTest SetUpTestCase");
}

void CreateFileManagerAssetTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("CreateFileManagerAssetTest TearDownTestCase");
}

void CreateFileManagerAssetTest::SetUp(void)
{
    MEDIA_INFO_LOG("SetUp");
}

void CreateFileManagerAssetTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(CreateFileManagerAssetTest, CreateFileManagerAsset_EmptyParcel_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("CreateFileManagerAsset_EmptyParcel_001 enter");
    MessageParcel data;
    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    int32_t ret = service->CreateFileManagerAsset(data, reply);
    EXPECT_LT(ret, 0);
    MEDIA_INFO_LOG("CreateFileManagerAsset_EmptyParcel_001 end");
}

HWTEST_F(CreateFileManagerAssetTest, CreateFileManagerAsset_InvalidParams_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("CreateFileManagerAsset_InvalidParams_001 enter");
    CreateFileMgrAssetReqBody reqBody;
    reqBody.displayName = "";
    reqBody.ownerAlbumId = "";

    MessageParcel data;
    MessageParcel reply;
    ASSERT_TRUE(reqBody.Marshalling(data));

    auto service = make_shared<MediaAssetsControllerService>();
    int32_t ret = service->CreateFileManagerAsset(data, reply);
    EXPECT_LT(ret, 0);
    MEDIA_INFO_LOG("CreateFileManagerAsset_InvalidParams_001 end");
}
} // namespace OHOS::Media

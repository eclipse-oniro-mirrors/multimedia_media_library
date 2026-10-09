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

#include "cancel_task_test.h"

#include <memory>
#include <string>

#include "message_parcel.h"
#include "medialibrary_errno.h"
#include "media_assets_controller_service.h"
#include "user_define_ipc.h"
#include "asset_cancel_task_vo.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace IPC;

void CancelTaskTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("CancelTaskTest SetUpTestCase");
}

void CancelTaskTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("CancelTaskTest TearDownTestCase");
}

void CancelTaskTest::SetUp(void)
{
    MEDIA_INFO_LOG("SetUp");
}

void CancelTaskTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(CancelTaskTest, CancelTask_EmptyParcel_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("CancelTask_EmptyParcel_001 enter");
    MessageParcel data;
    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    int32_t ret = service->CancelTask(data, reply);
    EXPECT_LT(ret, 0);
    MEDIA_INFO_LOG("CancelTask_EmptyParcel_001 end");
}
} // namespace OHOS::Media

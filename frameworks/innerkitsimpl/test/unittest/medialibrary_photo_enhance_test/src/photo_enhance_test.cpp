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

#include "photo_enhance_test.h"

#include <gmock/gmock.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>

#include "cloud_media_enhance_service.h"
#include "medialibrary_unittest_utils.h"

#include "media_log.h"
#include "medialibrary_errno.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Media::CloudSync {
void PhotoEnhanceTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void PhotoEnhanceTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(PhotoEnhanceTest, GetCloudSyncUnPreparedData_OK, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetCloudSyncUnPreparedData_OK Start");
    CloudMediaEnhanceService service;

    int32_t resultCount = 0;
    MediaLibraryUnitTestUtils::InitUnistore();
    int32_t ret = service.GetCloudSyncUnPreparedData(CloudSyncPrepareDataType::ASSET_MULTISTATE_INCOMPLETE,
        resultCount);

    EXPECT_GE(ret, E_OK);
    EXPECT_GE(resultCount, 0);
}

HWTEST_F(PhotoEnhanceTest, SubmitCloudSyncPreparedDataTask_OK, TestSize.Level1)
{
    MEDIA_INFO_LOG("SubmitCloudSyncPreparedDataTask_OK Start");
    CloudMediaEnhanceService service;

    MediaLibraryUnitTestUtils::InitUnistore();
    int32_t ret = service.SubmitCloudSyncPreparedDataTask(CloudSyncPrepareDataType::ASSET_MULTISTATE_INCOMPLETE);

    EXPECT_EQ(ret, E_OK);
}
}  // namespace OHOS::Media::CloudSync
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

#define MLOG_TAG "MediaShareAssetsUtilsTest"

#include "media_share_assets_test_utils.h"

#include "cloud_sync_notify_handler.h"
#include "media_log.h"
#include "media_share_assets_utils.h"
#include "medialibrary_errno.h"

using namespace std;
using namespace testing::ext;

namespace OHOS::Media {

/**
 * @tc.name: SetShareAssetCleanStatus_Cleaning_IsShareAssetCleaningTrue
 * @tc.desc: 清理中状态会写入时间戳, 重启续跑时可以查询到清理未完成(覆盖写入时间戳分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, SetShareAssetCleanStatus_Cleaning_IsShareAssetCleaningTrue, TestSize.Level1)
{
    MediaShareAssetsCloudExitUtils::SetShareAssetCleanStatus(CloudSyncStatus::CLOUD_CLEANING);
    EXPECT_TRUE(MediaShareAssetsCloudExitUtils::IsShareAssetCleaning());
}

/**
 * @tc.name: SetShareAssetCleanStatus_SwitchedOff_IsShareAssetCleaningFalse
 * @tc.desc: 清理完成后的状态时间戳为 0, 不会再触发续跑(覆盖时间戳置 0 分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, SetShareAssetCleanStatus_SwitchedOff_IsShareAssetCleaningFalse, TestSize.Level1)
{
    MediaShareAssetsCloudExitUtils::SetShareAssetCleanStatus(CloudSyncStatus::SYNC_SWITCHED_OFF);
    EXPECT_FALSE(MediaShareAssetsCloudExitUtils::IsShareAssetCleaning());
}

/**
 * @tc.name: IsShareAssetCleaning_AfterCleaningFinished_ReturnsFalse
 * @tc.desc: 先进入清理中再完成清理, 状态能够正确复位
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, IsShareAssetCleaning_AfterCleaningFinished_ReturnsFalse, TestSize.Level1)
{
    MediaShareAssetsCloudExitUtils::SetShareAssetCleanStatus(CloudSyncStatus::CLOUD_CLEANING);
    ASSERT_TRUE(MediaShareAssetsCloudExitUtils::IsShareAssetCleaning());

    MediaShareAssetsCloudExitUtils::SetShareAssetCleanStatus(CloudSyncStatus::SYNC_SWITCHED_OFF);
    EXPECT_FALSE(MediaShareAssetsCloudExitUtils::IsShareAssetCleaning());
}

} // namespace OHOS::Media
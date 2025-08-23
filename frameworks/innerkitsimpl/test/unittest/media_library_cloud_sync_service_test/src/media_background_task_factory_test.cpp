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

#include "media_background_task_factory_test.h"

#include "power_efficiency_manager.h"
#include "media_background_task_factory.h"

using namespace testing::ext;

namespace OHOS::Media::Background {
MediaBackgroundTaskFactory backgroundTaskFactory_;
void MediaBackgroundTaskFactoryTest::SetUpTestCase()
{
    GTEST_LOG_(INFO) << "SetUpTestCase";
}

void MediaBackgroundTaskFactoryTest::TearDownTestCase()
{
    GTEST_LOG_(INFO) << "TearDownTestCase";
}

void MediaBackgroundTaskFactoryTest::SetUp()
{}

void MediaBackgroundTaskFactoryTest::TearDown()
{}

HWTEST_F(MediaBackgroundTaskFactoryTest, MediaCloudSyncBackgroundTask_Test_001, TestSize.Level1)
{
    PowerEfficiencyManager::SetSubscriberStatus(true, true);
    backgroundTaskFactory_.Execute();
    EXPECT_EQ(backgroundTaskFactory_.Accept(), false);
}
}  // namespace OHOS::Media::CloudSync
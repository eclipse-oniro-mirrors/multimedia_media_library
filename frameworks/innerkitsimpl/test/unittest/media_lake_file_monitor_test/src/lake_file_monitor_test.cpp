/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#define MLOG_TAG "LakeFileMonitorTest"

#include "lake_file_monitor_test.h"

#include "media_file_change_processor.h"

#include "media_log.h"
#include "media_lake_notify_info.h"
#include "media_file_change_manager.h"
#include "file_monitor_interface.h"
#include "lake_file_utils.h"
 
namespace OHOS {
namespace Media {
using namespace testing::ext;

void LakeFileMonitorTest::SetUpTestCase() {}

void LakeFileMonitorTest::TearDownTestCase() {}

void LakeFileMonitorTest::SetUp() {}

void LakeFileMonitorTest::TearDown() {}

/**
 * @tc.number    : media_file_change_manager_test_001
 * @tc.name      : MediaFileChangeManager test
 * @tc.desc      : MediaFileChangeManager test
 */
HWTEST_F(LakeFileMonitorTest, media_file_change_manager_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_file_change_manager_test_001 start");

    auto manager = MediaFileChangeManager::GetInstance();
    auto processor = MediaFileChangeProcessor::GetInstance();
    EXPECT_NE(manager, nullptr);
    EXPECT_NE(processor, nullptr);

    MEDIA_INFO_LOG("media_file_change_manager_test_001 end");
}
} // namespace Media
} // namespace OHOS
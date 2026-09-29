/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaLibraryMonitorTest"

#include "media_library_monitor_test.h"
#include "media_library_monitor.h"
#include "media_log.h"

using namespace testing::ext;
using namespace OHOS::Media::Monitor;

namespace OHOS::Media::Monitor {

void MediaLibraryMonitorTest::SetUpTestCase() {}

void MediaLibraryMonitorTest::TearDownTestCase() {}

void MediaLibraryMonitorTest::SetUp()
{
    auto* info = testing::UnitTest::GetInstance()->current_test_info();
    MEDIA_INFO_LOG("%{public}s begin", info->name());
}

void MediaLibraryMonitorTest::TearDown()
{
    auto* info = testing::UnitTest::GetInstance()->current_test_info();
    MEDIA_INFO_LOG("%{public}s end", info->name());
}

HWTEST_F(MediaLibraryMonitorTest, GetInstance_ReturnsSameInstance_001, TestSize.Level1)
{
    auto& inst1 = MediaLibraryMonitor::GetInstance();
    auto& inst2 = MediaLibraryMonitor::GetInstance();
    EXPECT_EQ(&inst1, &inst2);
}

HWTEST_F(MediaLibraryMonitorTest, ReadSmapsRollup_ReturnsValidInfo_001, TestSize.Level1)
{
    auto& monitor = MediaLibraryMonitor::GetInstance();
    ProcessMemoryInfo info;
    bool ret = monitor.ReadSmapsRollup(info);
    EXPECT_TRUE(ret);
    EXPECT_GT(info.pssKb, static_cast<uint64_t>(0));
}
}  // namespace OHOS::Media::Monitor

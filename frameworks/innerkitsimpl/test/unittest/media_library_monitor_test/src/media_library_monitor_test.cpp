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

#include <chrono>
#include <thread>
#include <vector>

using namespace testing::ext;
using namespace OHOS::Media::Monitor;

namespace OHOS::Media::Monitor {

void MediaLibraryMonitorTest::SetUpTestCase() {}

void MediaLibraryMonitorTest::TearDownTestCase() {}

void MediaLibraryMonitorTest::SetUp()
{
    auto* info = testing::UnitTest::GetInstance()->current_test_info();
    MEDIA_INFO_LOG("%{public}s begin", info->name());
    auto& monitor = MediaLibraryMonitor::GetInstance();
    monitor.Stop();
    monitor.hasLastPrinted_ = false;
    monitor.lastPrintedPssKb_ = 0;
}

void MediaLibraryMonitorTest::TearDown()
{
    MediaLibraryMonitor::GetInstance().Stop();
    auto* info = testing::UnitTest::GetInstance()->current_test_info();
    MEDIA_INFO_LOG("%{public}s end", info->name());
}

HWTEST_F(MediaLibraryMonitorTest, GetInstance_ReturnsSameInstance_001, TestSize.Level1)
{
    auto& inst1 = MediaLibraryMonitor::GetInstance();
    auto& inst2 = MediaLibraryMonitor::GetInstance();
    EXPECT_EQ(&inst1, &inst2);
}

HWTEST_F(MediaLibraryMonitorTest, GetInstance_MultipleThreads_001, TestSize.Level1)
{
    const int threadCount = 10;
    std::vector<MediaLibraryMonitor*> instances(threadCount, nullptr);
    std::vector<std::thread> threads;
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([&instances, i]() {
            instances[i] = &MediaLibraryMonitor::GetInstance();
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    for (int i = 0; i < threadCount; ++i) {
        EXPECT_NE(instances[i], nullptr);
    }
    for (int i = 1; i < threadCount; ++i) {
        EXPECT_EQ(instances[0], instances[i]);
    }
}

HWTEST_F(MediaLibraryMonitorTest, Start_SetsRunningFlag_001, TestSize.Level1)
{
    auto& monitor = MediaLibraryMonitor::GetInstance();
    monitor.Start();
    EXPECT_TRUE(monitor.isRunning_);
    monitor.Stop();
}

HWTEST_F(MediaLibraryMonitorTest, Stop_ClearsRunningFlag_001, TestSize.Level1)
{
    auto& monitor = MediaLibraryMonitor::GetInstance();
    monitor.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    monitor.Stop();
    EXPECT_FALSE(monitor.isRunning_);
}

HWTEST_F(MediaLibraryMonitorTest, Start_Twice_NoDoubleSpawn_001, TestSize.Level1)
{
    auto& monitor = MediaLibraryMonitor::GetInstance();
    monitor.Start();
    monitor.Start();
    EXPECT_TRUE(monitor.isRunning_);
    monitor.Stop();
    EXPECT_FALSE(monitor.isRunning_);
}

HWTEST_F(MediaLibraryMonitorTest, Stop_WhenNotRunning_NoOp_001, TestSize.Level1)
{
    auto& monitor = MediaLibraryMonitor::GetInstance();
    monitor.Stop();
    EXPECT_FALSE(monitor.isRunning_);
    monitor.Stop();
    EXPECT_FALSE(monitor.isRunning_);
}

HWTEST_F(MediaLibraryMonitorTest, LogMemoryInfo_FirstCall_Prints_001, TestSize.Level1)
{
    auto& monitor = MediaLibraryMonitor::GetInstance();
    monitor.hasLastPrinted_ = false;
    monitor.lastPrintedPssKb_ = 0;
    ProcessMemoryInfo info;
    info.pssKb = 10 * 1024;
    monitor.LogMemoryInfo(info);
    EXPECT_TRUE(monitor.hasLastPrinted_);
    EXPECT_EQ(monitor.lastPrintedPssKb_, static_cast<uint64_t>(10 * 1024));
}

HWTEST_F(MediaLibraryMonitorTest, LogMemoryInfo_SmallDelta_Skips_001, TestSize.Level1)
{
    auto& monitor = MediaLibraryMonitor::GetInstance();
    monitor.hasLastPrinted_ = true;
    monitor.lastPrintedPssKb_ = 10 * 1024;
    ProcessMemoryInfo info;
    info.pssKb = 10 * 1024 + 500;
    monitor.LogMemoryInfo(info);
    EXPECT_EQ(monitor.lastPrintedPssKb_, static_cast<uint64_t>(10 * 1024));
    EXPECT_TRUE(monitor.hasLastPrinted_);
}

HWTEST_F(MediaLibraryMonitorTest, LogMemoryInfo_LargeDelta_Prints_001, TestSize.Level1)
{
    auto& monitor = MediaLibraryMonitor::GetInstance();
    monitor.hasLastPrinted_ = true;
    monitor.lastPrintedPssKb_ = 10 * 1024;
    ProcessMemoryInfo info;
    info.pssKb = 10 * 1024 + 2048;
    monitor.LogMemoryInfo(info);
    EXPECT_EQ(monitor.lastPrintedPssKb_, static_cast<uint64_t>(10 * 1024 + 2048));
    EXPECT_TRUE(monitor.hasLastPrinted_);
}

HWTEST_F(MediaLibraryMonitorTest, LogMemoryInfo_Decrease_LargeDelta_Prints_001, TestSize.Level1)
{
    auto& monitor = MediaLibraryMonitor::GetInstance();
    monitor.hasLastPrinted_ = true;
    monitor.lastPrintedPssKb_ = 10 * 1024;
    ProcessMemoryInfo info;
    info.pssKb = 8 * 1024;
    monitor.LogMemoryInfo(info);
    EXPECT_EQ(monitor.lastPrintedPssKb_, static_cast<uint64_t>(8 * 1024));
    EXPECT_TRUE(monitor.hasLastPrinted_);
}

HWTEST_F(MediaLibraryMonitorTest, ReadSmapsRollup_ReturnsValidInfo_001, TestSize.Level1)
{
    auto& monitor = MediaLibraryMonitor::GetInstance();
    ProcessMemoryInfo info;
    bool ret = monitor.ReadSmapsRollup(info);
    EXPECT_TRUE(ret);
    EXPECT_GT(info.pssKb, static_cast<uint64_t>(0));
}

HWTEST_F(MediaLibraryMonitorTest, Run_Lifecycle_001, TestSize.Level1)
{
    auto& monitor = MediaLibraryMonitor::GetInstance();
    monitor.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    EXPECT_TRUE(monitor.isRunning_);
    monitor.Stop();
    EXPECT_FALSE(monitor.isRunning_);
}
}  // namespace OHOS::Media::Monitor

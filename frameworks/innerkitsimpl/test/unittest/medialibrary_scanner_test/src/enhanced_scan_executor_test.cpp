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

#define private public
#include "enhanced_scan_executor_test.h"
#undef private

#include "media_log.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Media {

void EnhancedScanExecutorTest::SetUp()
{
    executor_ = std::make_shared<EnhancedScanExecutor>();
}

void EnhancedScanExecutorTest::TearDown()
{
    executor_->Stop();
    executor_ = nullptr;
}

std::shared_ptr<ScanTaskContext> EnhancedScanExecutorTest::CreateTask(int32_t fileId, ScanExecutionMode executionMode)
{
    auto config = ScanConfigBuilder()
        .SetFileId(fileId)
        .SetFilePath("/test/path")
        .SetExecutionMode(executionMode)
        .Build();
    return std::make_shared<ScanTaskContext>(config);
}

/**
 * @tc.name: EnhancedScanExecutor_Submit_test01
 * @tc.desc: Submit首个任务返回EXECUTING并标记为执行中
 */
HWTEST_F(EnhancedScanExecutorTest, EnhancedScanExecutor_Submit_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter EnhancedScanExecutor_Submit_test01");
    auto task = CreateTask(1, ScanExecutionMode::ASYNC);
    
    auto result = executor_->Submit(task);
    
    EXPECT_EQ(result, ScanSubmitResult::EXECUTING);
    EXPECT_EQ(executor_->globalTaskQueue_.size(), 1);
}

/**
 * @tc.name: EnhancedScanExecutor_Submit_test02
 * @tc.desc: Submit相同fileId任务返回WAITING不入队
 */
HWTEST_F(EnhancedScanExecutorTest, EnhancedScanExecutor_Submit_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter EnhancedScanExecutor_Submit_test02");
    auto task1 = CreateTask(1, ScanExecutionMode::ASYNC);
    auto task2 = CreateTask(1, ScanExecutionMode::ASYNC);
    
    executor_->Submit(task1);
    auto result = executor_->Submit(task2);
    
    EXPECT_EQ(result, ScanSubmitResult::WAITING);
    EXPECT_EQ(executor_->globalTaskQueue_.size(), 1);
}

/**
 * @tc.name: EnhancedScanExecutor_Submit_test03
 * @tc.desc: Submit不同fileId任务都返回EXECUTING
 */
HWTEST_F(EnhancedScanExecutorTest, EnhancedScanExecutor_Submit_test03, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter EnhancedScanExecutor_Submit_test03");
    auto task1 = CreateTask(1, ScanExecutionMode::ASYNC);
    auto task2 = CreateTask(2, ScanExecutionMode::ASYNC);
    
    auto result1 = executor_->Submit(task1);
    auto result2 = executor_->Submit(task2);
    
    EXPECT_EQ(result1, ScanSubmitResult::EXECUTING);
    EXPECT_EQ(result2, ScanSubmitResult::EXECUTING);
    EXPECT_EQ(executor_->globalTaskQueue_.size(), 2);
}

/**
 * @tc.name: EnhancedScanExecutor_Submit_test04
 * @tc.desc: Submit空任务返回REJECTED
 */
HWTEST_F(EnhancedScanExecutorTest, EnhancedScanExecutor_Submit_test04, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter EnhancedScanExecutor_Submit_test04");
    
    auto result = executor_->Submit(nullptr);
    
    EXPECT_EQ(result, ScanSubmitResult::REJECTED);
    EXPECT_EQ(executor_->globalTaskQueue_.size(), 0);
}

/**
 * @tc.name: EnhancedScanExecutor_Submit_test05
 * @tc.desc: Submit同步任务返回EXECUTING不入队
 */
HWTEST_F(EnhancedScanExecutorTest, EnhancedScanExecutor_Submit_test05, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter EnhancedScanExecutor_Submit_test05");
    auto task = CreateTask(1, ScanExecutionMode::SYNC);
    
    auto result = executor_->Submit(task);
    
    EXPECT_EQ(result, ScanSubmitResult::EXECUTING);
    EXPECT_EQ(executor_->globalTaskQueue_.size(), 0);
}

/**
 * @tc.name: EnhancedScanExecutor_Stop_test01
 * @tc.desc: Stop清空队列和等待队列
 */
HWTEST_F(EnhancedScanExecutorTest, EnhancedScanExecutor_Stop_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter EnhancedScanExecutor_Stop_test01");
    auto task = CreateTask(1, ScanExecutionMode::ASYNC);
    executor_->Submit(task);
    
    executor_->Stop();
    
    EXPECT_EQ(executor_->globalTaskQueue_.size(), 0);
}

/**
 * @tc.name: EnhancedScanExecutor_Stop_test02
 * @tc.desc: Stop多次调用安全
 */
HWTEST_F(EnhancedScanExecutorTest, EnhancedScanExecutor_Stop_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter EnhancedScanExecutor_Stop_test02");
    
    executor_->Stop();
    executor_->Stop();
    
    EXPECT_FALSE(executor_->running_);
}

/**
 * @tc.name: EnhancedScanExecutor_ClearAllTasks_test01
 * @tc.desc: ClearAllTasks清空队列
 */
HWTEST_F(EnhancedScanExecutorTest, EnhancedScanExecutor_ClearAllTasks_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter EnhancedScanExecutor_ClearAllTasks_test01");
    auto task = CreateTask(1, ScanExecutionMode::ASYNC);
    executor_->Submit(task);
    
    executor_->ClearAllTasks();
    
    EXPECT_EQ(executor_->globalTaskQueue_.size(), 0);
}

} // namespace Media
} // namespace OHOS
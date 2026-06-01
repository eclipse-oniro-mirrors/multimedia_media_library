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

#include "deduplication_handler_test.h"

#include <thread>

#include "media_log.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Media {

constexpr int WAIT_TIMEOUT_MS = 100;

void DeduplicationHandlerTest::SetUp()
{
    handler_ = std::make_shared<DeduplicationHandler>();
}

void DeduplicationHandlerTest::TearDown()
{
    handler_ = nullptr;
}

std::shared_ptr<ScanTaskContext> DeduplicationHandlerTest::CreateTask(int32_t fileId, ScanExecutionMode executionMode)
{
    auto config = ScanConfigBuilder()
        .SetFileId(fileId)
        .SetFilePath("/test/path/" + std::to_string(fileId))
        .SetExecutionMode(executionMode)
        .Build();
    return std::make_shared<ScanTaskContext>(config);
}

/**
 * @tc.name: DeduplicationHandler_Handle_test01
 * @tc.desc: Handle首个任务返回EXECUTING
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_Handle_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_Handle_test01");
    auto task = CreateTask(1, ScanExecutionMode::ASYNC);

    auto result = handler_->Handle(task);

    EXPECT_EQ(result, ScanSubmitResult::EXECUTING);
    EXPECT_TRUE(handler_->IsFileIdExecuting(1));
}

/**
 * @tc.name: DeduplicationHandler_Handle_test02
 * @tc.desc: Handle相同fileId的第二个任务返回WAITING
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_Handle_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_Handle_test02");
    auto task1 = CreateTask(1, ScanExecutionMode::ASYNC);
    auto task2 = CreateTask(1, ScanExecutionMode::ASYNC);

    handler_->Handle(task1);
    auto result = handler_->Handle(task2);

    EXPECT_EQ(result, ScanSubmitResult::WAITING);
    EXPECT_TRUE(handler_->IsFileIdExecuting(1));
    EXPECT_TRUE(handler_->HasWaitingTask(1));
}

/**
 * @tc.name: DeduplicationHandler_Handle_test03
 * @tc.desc: Handle不同fileId的任务返回EXECUTING
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_Handle_test03, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_Handle_test03");
    auto task1 = CreateTask(1, ScanExecutionMode::ASYNC);
    auto task2 = CreateTask(2, ScanExecutionMode::ASYNC);

    handler_->Handle(task1);
    auto result = handler_->Handle(task2);

    EXPECT_EQ(result, ScanSubmitResult::EXECUTING);
    EXPECT_TRUE(handler_->IsFileIdExecuting(1));
    EXPECT_TRUE(handler_->IsFileIdExecuting(2));
}

/**
 * @tc.name: DeduplicationHandler_Handle_test04
 * @tc.desc: Handle空任务返回REJECTED
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_Handle_test04, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_Handle_test04");
    auto result = handler_->Handle(nullptr);

    EXPECT_EQ(result, ScanSubmitResult::REJECTED);
}

/**
 * @tc.name: DeduplicationHandler_Handle_test05
 * @tc.desc: Handle无效fileId(-1)返回EXECUTING但不标记执行
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_Handle_test05, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_Handle_test05");
    auto task = CreateTask(-1, ScanExecutionMode::ASYNC);

    auto result = handler_->Handle(task);

    EXPECT_EQ(result, ScanSubmitResult::EXECUTING);
    EXPECT_FALSE(handler_->IsFileIdExecuting(-1));
}

/**
 * @tc.name: DeduplicationHandler_Handle_test06
 * @tc.desc: Handle零fileId返回EXECUTING但不标记执行
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_Handle_test06, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_Handle_test06");
    auto task = CreateTask(0, ScanExecutionMode::ASYNC);

    auto result = handler_->Handle(task);

    EXPECT_EQ(result, ScanSubmitResult::EXECUTING);
    EXPECT_FALSE(handler_->IsFileIdExecuting(0));
}

/**
 * @tc.name: DeduplicationHandler_Handle_test07
 * @tc.desc: Handle多个异步任务都返回WAITING
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_Handle_test07, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_Handle_test07");
    auto task1 = CreateTask(1, ScanExecutionMode::ASYNC);
    auto task2 = CreateTask(1, ScanExecutionMode::ASYNC);
    auto task3 = CreateTask(1, ScanExecutionMode::ASYNC);

    handler_->Handle(task1);
    auto result2 = handler_->Handle(task2);
    auto result3 = handler_->Handle(task3);

    EXPECT_EQ(result2, ScanSubmitResult::WAITING);
    EXPECT_EQ(result3, ScanSubmitResult::WAITING);
    EXPECT_TRUE(handler_->IsFileIdExecuting(1));
    EXPECT_TRUE(handler_->HasWaitingTask(1));
}

/**
 * @tc.name: DeduplicationHandler_Handle_test08
 * @tc.desc: 异步执行中同步任务到达返回WAITING并标记同步等待
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_Handle_test08, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_Handle_test08");
    auto asyncTask = CreateTask(1, ScanExecutionMode::ASYNC);
    auto syncTask = CreateTask(1, ScanExecutionMode::SYNC);

    handler_->Handle(asyncTask);
    auto result = handler_->Handle(syncTask);

    EXPECT_EQ(result, ScanSubmitResult::WAITING);
    EXPECT_TRUE(handler_->HasSyncWaiting(1));
    EXPECT_TRUE(handler_->HasWaitingTask(1));
}

/**
 * @tc.name: DeduplicationHandler_Handle_test09
 * @tc.desc: 同步执行中异步任务到达返回WAITING
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_Handle_test09, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_Handle_test09");
    auto syncTask = CreateTask(1, ScanExecutionMode::SYNC);
    auto asyncTask = CreateTask(1, ScanExecutionMode::ASYNC);

    handler_->Handle(syncTask);
    auto result = handler_->Handle(asyncTask);

    EXPECT_EQ(result, ScanSubmitResult::WAITING);
    EXPECT_TRUE(handler_->IsFileIdExecuting(1));
}

/**
 * @tc.name: DeduplicationHandler_Handle_test10
 * @tc.desc: 同步任务覆盖同步任务返回WAITING
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_Handle_test10, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_Handle_test10");
    auto sync1 = CreateTask(1, ScanExecutionMode::SYNC);
    auto sync2 = CreateTask(1, ScanExecutionMode::SYNC);

    handler_->Handle(sync1);
    auto result = handler_->Handle(sync2);

    EXPECT_EQ(result, ScanSubmitResult::WAITING);
    EXPECT_TRUE(handler_->HasSyncWaiting(1));
}

/**
 * @tc.name: DeduplicationHandler_UnmarkAsScanning_test01
 * @tc.desc: UnmarkAsScanning清除执行状态
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_UnmarkAsScanning_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_UnmarkAsScanning_test01");
    auto task = CreateTask(1, ScanExecutionMode::ASYNC);
    handler_->Handle(task);

    handler_->UnmarkAsScanning(task);

    EXPECT_FALSE(handler_->IsFileIdExecuting(1));
    EXPECT_FALSE(handler_->HasWaitingTask(1));
}

/**
 * @tc.name: DeduplicationHandler_UnmarkAsScanning_test02
 * @tc.desc: UnmarkAsScanning有同步等待时移动到SyncPending
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_UnmarkAsScanning_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_UnmarkAsScanning_test02");
    auto asyncTask = CreateTask(1, ScanExecutionMode::ASYNC);
    auto syncTask = CreateTask(1, ScanExecutionMode::SYNC);

    handler_->Handle(asyncTask);
    handler_->Handle(syncTask);
    handler_->UnmarkAsScanning(asyncTask);

    EXPECT_TRUE(handler_->HasSyncPending(1));
    EXPECT_TRUE(handler_->IsFileIdExecuting(1));
    EXPECT_FALSE(handler_->HasWaitingTask(1));
}

/**
 * @tc.name: DeduplicationHandler_UnmarkAsScanning_test03
 * @tc.desc: UnmarkAsScanning无等待任务仅清除执行状态
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_UnmarkAsScanning_test03, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_UnmarkAsScanning_test03");
    auto task = CreateTask(1, ScanExecutionMode::ASYNC);
    handler_->Handle(task);

    handler_->UnmarkAsScanning(task);

    EXPECT_FALSE(handler_->IsFileIdExecuting(1));
    EXPECT_FALSE(handler_->HasSyncPending(1));
}

/**
 * @tc.name: DeduplicationHandler_UnmarkAsScanning_test04
 * @tc.desc: UnmarkAsScanning有异步等待不产生SyncPending
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_UnmarkAsScanning_test04, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_UnmarkAsScanning_test04");
    auto task1 = CreateTask(1, ScanExecutionMode::ASYNC);
    auto task2 = CreateTask(1, ScanExecutionMode::ASYNC);

    handler_->Handle(task1);
    handler_->Handle(task2);
    handler_->UnmarkAsScanning(task1);

    EXPECT_FALSE(handler_->IsFileIdExecuting(1));
    EXPECT_FALSE(handler_->HasSyncPending(1));
    EXPECT_TRUE(handler_->HasWaitingTask(1));
}

/**
 * @tc.name: DeduplicationHandler_UnmarkAsScanning_test05
 * @tc.desc: UnmarkAsScanning空任务无效果
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_UnmarkAsScanning_test05, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_UnmarkAsScanning_test05");
    auto task = CreateTask(1, ScanExecutionMode::ASYNC);
    handler_->Handle(task);

    handler_->UnmarkAsScanning(nullptr);

    EXPECT_TRUE(handler_->IsFileIdExecuting(1));
}

/**
 * @tc.name: DeduplicationHandler_GetSyncPendingTask_test01
 * @tc.desc: GetSyncPendingTask无同步待处理返回null
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_GetSyncPendingTask_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_GetSyncPendingTask_test01");
    auto task = CreateTask(1, ScanExecutionMode::ASYNC);
    handler_->Handle(task);

    auto result = handler_->GetSyncPendingTask(1);

    EXPECT_EQ(result, nullptr);
}

/**
 * @tc.name: DeduplicationHandler_GetSyncPendingTask_test02
 * @tc.desc: GetSyncPendingTask有同步待处理返回任务
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_GetSyncPendingTask_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_GetSyncPendingTask_test02");
    auto asyncTask = CreateTask(1, ScanExecutionMode::ASYNC);
    auto syncTask = CreateTask(1, ScanExecutionMode::SYNC);

    handler_->Handle(asyncTask);
    handler_->Handle(syncTask);
    handler_->UnmarkAsScanning(asyncTask);

    auto result = handler_->GetSyncPendingTask(1);

    EXPECT_NE(result, nullptr);
    EXPECT_EQ(result->config.GetExecutionMode(), ScanExecutionMode::SYNC);
}

/**
 * @tc.name: DeduplicationHandler_GetSyncPendingTask_test03
 * @tc.desc: GetSyncPendingTask无效fileId返回null
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_GetSyncPendingTask_test03, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_GetSyncPendingTask_test03");
    auto result = handler_->GetSyncPendingTask(-1);

    EXPECT_EQ(result, nullptr);
}

/**
 * @tc.name: DeduplicationHandler_GetNextWaitingTask_test01
 * @tc.desc: GetNextWaitingTask无等待返回null
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_GetNextWaitingTask_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_GetNextWaitingTask_test01");
    auto task = CreateTask(1, ScanExecutionMode::ASYNC);
    handler_->Handle(task);

    auto result = handler_->GetNextWaitingTask(task);

    EXPECT_EQ(result, nullptr);
}

/**
 * @tc.name: DeduplicationHandler_GetNextWaitingTask_test02
 * @tc.desc: GetNextWaitingTask有异步等待返回任务
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_GetNextWaitingTask_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_GetNextWaitingTask_test02");
    auto task1 = CreateTask(1, ScanExecutionMode::ASYNC);
    auto task2 = CreateTask(1, ScanExecutionMode::ASYNC);

    handler_->Handle(task1);
    handler_->Handle(task2);
    handler_->UnmarkAsScanning(task1);

    auto result = handler_->GetNextWaitingTask(task1);

    EXPECT_NE(result, nullptr);
    EXPECT_TRUE(handler_->IsFileIdExecuting(1));
}

/**
 * @tc.name: DeduplicationHandler_GetNextWaitingTask_test03
 * @tc.desc: GetNextWaitingTask有同步等待返回null
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_GetNextWaitingTask_test03, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_GetNextWaitingTask_test03");
    auto asyncTask = CreateTask(1, ScanExecutionMode::ASYNC);
    auto syncTask = CreateTask(1, ScanExecutionMode::SYNC);

    handler_->Handle(asyncTask);
    handler_->Handle(syncTask);
    handler_->UnmarkAsScanning(asyncTask);

    auto result = handler_->GetNextWaitingTask(asyncTask);

    EXPECT_EQ(result, nullptr);
}

/**
 * @tc.name: DeduplicationHandler_GetNextWaitingTask_test04
 * @tc.desc: GetNextWaitingTask空上下文返回null
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_GetNextWaitingTask_test04, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_GetNextWaitingTask_test04");
    auto result = handler_->GetNextWaitingTask(nullptr);

    EXPECT_EQ(result, nullptr);
}

/**
 * @tc.name: DeduplicationHandler_NotifySyncTaskCompleted_test01
 * @tc.desc: NotifySyncTaskCompleted清除SyncPending
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_NotifySyncTaskCompleted_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_NotifySyncTaskCompleted_test01");
    auto asyncTask = CreateTask(1, ScanExecutionMode::ASYNC);
    auto syncTask = CreateTask(1, ScanExecutionMode::SYNC);

    handler_->Handle(asyncTask);
    handler_->Handle(syncTask);
    handler_->UnmarkAsScanning(asyncTask);
    handler_->GetSyncPendingTask(1);

    handler_->NotifySyncTaskCompleted(1);

    EXPECT_FALSE(handler_->HasSyncPending(1));
    EXPECT_FALSE(handler_->IsFileIdExecuting(1));
}

/**
 * @tc.name: DeduplicationHandler_NotifySyncTaskCompleted_test02
 * @tc.desc: NotifySyncTaskCompleted清除执行状态
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_NotifySyncTaskCompleted_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_NotifySyncTaskCompleted_test02");
    auto syncTask = CreateTask(1, ScanExecutionMode::SYNC);

    handler_->Handle(syncTask);
    handler_->NotifySyncTaskCompleted(1);

    auto newTask = CreateTask(1, ScanExecutionMode::ASYNC);
    auto result = handler_->Handle(newTask);
    EXPECT_EQ(result, ScanSubmitResult::EXECUTING);
}

/**
 * @tc.name: DeduplicationHandler_NotifySyncTaskCompleted_test03
 * @tc.desc: NotifySyncTaskCompleted无效fileId无效果
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_NotifySyncTaskCompleted_test03, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_NotifySyncTaskCompleted_test03");
    handler_->NotifySyncTaskCompleted(-1);

    auto task = CreateTask(1, ScanExecutionMode::ASYNC);
    auto result = handler_->Handle(task);
    EXPECT_EQ(result, ScanSubmitResult::EXECUTING);
}

/**
 * @tc.name: DeduplicationHandler_ClearWaitingTasks_test01
 * @tc.desc: ClearWaitingTasks空参数清除所有
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_ClearWaitingTasks_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_ClearWaitingTasks_test01");
    auto task1 = CreateTask(1, ScanExecutionMode::ASYNC);
    auto task2 = CreateTask(1, ScanExecutionMode::ASYNC);

    handler_->Handle(task1);
    handler_->Handle(task2);

    handler_->ClearWaitingTasks(nullptr);

    handler_->UnmarkAsScanning(task1);
    auto result = handler_->GetNextWaitingTask(task1);
    EXPECT_EQ(result, nullptr);
}

/**
 * @tc.name: DeduplicationHandler_ClearWaitingTasks_test02
 * @tc.desc: ClearWaitingTasks清除同步等待
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_ClearWaitingTasks_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_ClearWaitingTasks_test02");
    auto asyncTask = CreateTask(1, ScanExecutionMode::ASYNC);
    auto syncTask = CreateTask(1, ScanExecutionMode::SYNC);

    handler_->Handle(asyncTask);
    handler_->Handle(syncTask);

    handler_->ClearWaitingTasks(nullptr);

    EXPECT_FALSE(handler_->HasSyncWaiting(1));
}

/**
 * @tc.name: DeduplicationHandler_StateTransition_test01
 * @tc.desc: 异步任务完整状态转换周期
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_StateTransition_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_StateTransition_test01");
    auto task1 = CreateTask(1, ScanExecutionMode::ASYNC);
    auto result1 = handler_->Handle(task1);
    EXPECT_EQ(result1, ScanSubmitResult::EXECUTING);
    EXPECT_TRUE(handler_->IsFileIdExecuting(1));
    EXPECT_EQ(handler_->GetExecutingCount(), 1);

    auto task2 = CreateTask(1, ScanExecutionMode::ASYNC);
    auto result2 = handler_->Handle(task2);
    EXPECT_EQ(result2, ScanSubmitResult::WAITING);
    EXPECT_TRUE(handler_->HasWaitingTask(1));
    EXPECT_EQ(handler_->GetWaitingCount(), 1);

    handler_->UnmarkAsScanning(task1);
    auto nextTask = handler_->GetNextWaitingTask(task1);
    EXPECT_NE(nextTask, nullptr);
    EXPECT_TRUE(handler_->IsFileIdExecuting(1));

    handler_->UnmarkAsScanning(nextTask);
    auto task3 = CreateTask(1, ScanExecutionMode::ASYNC);
    auto result3 = handler_->Handle(task3);
    EXPECT_EQ(result3, ScanSubmitResult::EXECUTING);
    EXPECT_TRUE(handler_->IsFileIdExecuting(1));
}

/**
 * @tc.name: DeduplicationHandler_StateTransition_test02
 * @tc.desc: 同步任务完整状态转换周期
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_StateTransition_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_StateTransition_test02");
    auto asyncTask = CreateTask(1, ScanExecutionMode::ASYNC);
    auto syncTask = CreateTask(1, ScanExecutionMode::SYNC);

    handler_->Handle(asyncTask);
    auto result = handler_->Handle(syncTask);
    EXPECT_EQ(result, ScanSubmitResult::WAITING);
    EXPECT_TRUE(handler_->HasSyncWaiting(1));

    handler_->UnmarkAsScanning(asyncTask);
    auto syncPending = handler_->GetSyncPendingTask(1);
    EXPECT_NE(syncPending, nullptr);
    EXPECT_TRUE(handler_->HasSyncPending(1));
    EXPECT_TRUE(handler_->IsFileIdExecuting(1));

    handler_->NotifySyncTaskCompleted(1);
    EXPECT_FALSE(handler_->HasSyncPending(1));
    EXPECT_FALSE(handler_->IsFileIdExecuting(1));

    auto newTask = CreateTask(1, ScanExecutionMode::ASYNC);
    auto result2 = handler_->Handle(newTask);
    EXPECT_EQ(result2, ScanSubmitResult::EXECUTING);
}

/**
 * @tc.name: DeduplicationHandler_StateTransition_test03
 * @tc.desc: 多fileId状态转换
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_StateTransition_test03, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_StateTransition_test03");
    auto task1 = CreateTask(1, ScanExecutionMode::ASYNC);
    auto task2 = CreateTask(2, ScanExecutionMode::ASYNC);
    auto task3 = CreateTask(1, ScanExecutionMode::ASYNC);
    auto task4 = CreateTask(2, ScanExecutionMode::ASYNC);

    handler_->Handle(task1);
    handler_->Handle(task2);
    handler_->Handle(task3);
    handler_->Handle(task4);

    EXPECT_TRUE(handler_->IsFileIdExecuting(1));
    EXPECT_TRUE(handler_->IsFileIdExecuting(2));
    EXPECT_TRUE(handler_->HasWaitingTask(1));
    EXPECT_TRUE(handler_->HasWaitingTask(2));

    handler_->UnmarkAsScanning(task1);
    auto next1 = handler_->GetNextWaitingTask(task1);
    EXPECT_NE(next1, nullptr);

    handler_->UnmarkAsScanning(task2);
    auto next2 = handler_->GetNextWaitingTask(task2);
    EXPECT_NE(next2, nullptr);
}

/**
 * @tc.name: DeduplicationHandler_StateTransition_test04
 * @tc.desc: 混合优先级状态转换
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_StateTransition_test04, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_StateTransition_test04");
    auto asyncA = CreateTask(1, ScanExecutionMode::ASYNC);
    handler_->Handle(asyncA);
    EXPECT_TRUE(handler_->IsFileIdExecuting(1));

    auto syncB = CreateTask(1, ScanExecutionMode::SYNC);
    handler_->Handle(syncB);
    EXPECT_TRUE(handler_->HasSyncWaiting(1));

    auto asyncC = CreateTask(1, ScanExecutionMode::ASYNC);
    handler_->Handle(asyncC);
    EXPECT_TRUE(handler_->HasWaitingTask(1));

    handler_->UnmarkAsScanning(asyncA);
    auto syncPending = handler_->GetSyncPendingTask(1);
    EXPECT_NE(syncPending, nullptr);

    handler_->NotifySyncTaskCompleted(1);
    EXPECT_FALSE(handler_->IsFileIdExecuting(1));
}

/**
 * @tc.name: DeduplicationHandler_WaitForSyncScanCompletion_test01
 * @tc.desc: WaitForSyncScanCompletion同步扫描链等待完成
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_WaitForSyncScanCompletion_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_WaitForSyncScanCompletion_test01");
    auto asyncTask = CreateTask(1, ScanExecutionMode::ASYNC);
    auto syncTask = CreateTask(1, ScanExecutionMode::SYNC);

    handler_->Handle(asyncTask);
    handler_->Handle(syncTask);

    std::thread completer([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIMEOUT_MS));
        handler_->UnmarkAsScanning(asyncTask);
        handler_->GetSyncPendingTask(1);
        handler_->NotifySyncTaskCompleted(1);
    });
    completer.detach();

    handler_->WaitForSyncScanCompletion(1);

    EXPECT_FALSE(handler_->IsFileIdExecuting(1));
    EXPECT_FALSE(handler_->HasSyncPending(1));
}

/**
 * @tc.name: DeduplicationHandler_WaitForSyncScanCompletion_test02
 * @tc.desc: WaitForSyncScanCompletion未执行同步扫描时立即返回
 */
HWTEST_F(DeduplicationHandlerTest, DeduplicationHandler_WaitForSyncScanCompletion_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter DeduplicationHandler_WaitForSyncScanCompletion_test02");
    auto task = CreateTask(1, ScanExecutionMode::ASYNC);

    handler_->WaitForSyncScanCompletion(1);

    EXPECT_FALSE(handler_->IsFileIdExecuting(1));
}

} // namespace Media
} // namespace OHOS
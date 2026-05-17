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

#include "quality_conflict_resolver_test.h"

#include "media_log.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Media {

void QualityConflictResolverTest::SetUp()
{
    resolver_ = std::make_shared<QualityConflictResolver>();
}

void QualityConflictResolverTest::TearDown()
{
    resolver_ = nullptr;
}

std::shared_ptr<ScanTaskContext> QualityConflictResolverTest::CreateTask(ScanQuality quality)
{
    auto config = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test/path")
        .SetQuality(quality)
        .SetConflictPolicy(ConflictPolicy::QUALITY_PRIORITY)
        .Build();
    return std::make_shared<ScanTaskContext>(config);
}

/**
 * @tc.name: QualityConflictResolver_Resolve_test01
 * @tc.desc: Resolve测试各种质量组合的冲突解决
 */
HWTEST_F(QualityConflictResolverTest, QualityConflictResolver_Resolve_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter QualityConflictResolver_Resolve_test01");
    auto newTask = CreateTask(ScanQuality::LOW);
    auto executingTask = CreateTask(ScanQuality::FULL);
    auto result = resolver_->Resolve(newTask, executingTask);
    EXPECT_EQ(result, ScanSubmitResult::REJECTED);

    auto newTask2 = CreateTask(ScanQuality::FULL);
    auto executingTask2 = CreateTask(ScanQuality::LOW);
    result = resolver_->Resolve(newTask2, executingTask2);
    EXPECT_EQ(result, ScanSubmitResult::WAITING);

    auto newTask3 = CreateTask(ScanQuality::FULL);
    auto executingTask3 = CreateTask(ScanQuality::FULL);
    result = resolver_->Resolve(newTask3, executingTask3);
    EXPECT_EQ(result, ScanSubmitResult::WAITING);

    auto newTask4 = CreateTask(ScanQuality::LOW);
    auto executingTask4 = CreateTask(ScanQuality::LOW);
    result = resolver_->Resolve(newTask4, executingTask4);
    EXPECT_EQ(result, ScanSubmitResult::WAITING);

    auto newTask5 = CreateTask(ScanQuality::DEFAULT);
    auto executingTask5 = CreateTask(ScanQuality::FULL);
    result = resolver_->Resolve(newTask5, executingTask5);
    EXPECT_EQ(result, ScanSubmitResult::WAITING);

    MEDIA_INFO_LOG("end QualityConflictResolver_Resolve_test01");
}

/**
 * @tc.name: QualityConflictResolver_Resolve_test02
 * @tc.desc: Resolve空任务返回WAITING
 */
HWTEST_F(QualityConflictResolverTest, QualityConflictResolver_Resolve_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter QualityConflictResolver_Resolve_test02");
    auto executingTask = CreateTask(ScanQuality::FULL);
    auto result = resolver_->Resolve(nullptr, executingTask);
    EXPECT_EQ(result, ScanSubmitResult::WAITING);

    auto newTask = CreateTask(ScanQuality::FULL);
    result = resolver_->Resolve(newTask, nullptr);
    EXPECT_EQ(result, ScanSubmitResult::WAITING);

    result = resolver_->Resolve(nullptr, nullptr);
    EXPECT_EQ(result, ScanSubmitResult::WAITING);
    MEDIA_INFO_LOG("end QualityConflictResolver_Resolve_test02");
}

/**
 * @tc.name: QualityConflictResolver_IsStrategyEnabled_test01
 * @tc.desc: IsStrategyEnabled测试不同质量和策略条件
 */
HWTEST_F(QualityConflictResolverTest, QualityConflictResolver_IsStrategyEnabled_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter QualityConflictResolver_IsStrategyEnabled_test01");
    auto taskFull = CreateTask(ScanQuality::FULL);
    EXPECT_TRUE(resolver_->IsStrategyEnabled(taskFull));

    auto taskLow = CreateTask(ScanQuality::LOW);
    EXPECT_TRUE(resolver_->IsStrategyEnabled(taskLow));

    auto taskDefault = CreateTask(ScanQuality::DEFAULT);
    EXPECT_FALSE(resolver_->IsStrategyEnabled(taskDefault));

    EXPECT_FALSE(resolver_->IsStrategyEnabled(nullptr));

    auto config = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetConflictPolicy(ConflictPolicy::DEFAULT)
        .Build();
    auto taskOtherPolicy = std::make_shared<ScanTaskContext>(config);
    EXPECT_FALSE(resolver_->IsStrategyEnabled(taskOtherPolicy));
    MEDIA_INFO_LOG("end QualityConflictResolver_IsStrategyEnabled_test01");
}

} // namespace Media
} // namespace OHOS
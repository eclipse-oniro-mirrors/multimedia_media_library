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

#define MLOG_TAG "ConsistencyCheckManagerTest"

#include "consistency_check_manager_test.h"

#include "consistency_check_manager.h"
#include "global_scanner.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
using namespace testing::ext;

void Reset()
{
    MEDIA_INFO_LOG("Start Reset");
    ConsistencyCheckManager::GetInstance().EnableCheck();
    ConsistencyCheckManager::GetInstance().runningScene_ = CheckScene::IDLE;
    ConsistencyCheckManager::GetInstance().pendingScenes_.clear();
    ConsistencyCheckManager::GetInstance().isInterrupted_.store(false);
    GlobalScanner::GetInstance().scannerStatus_ = ScannerStatus::IDLE;
    GlobalScanner::GetInstance().isNotInterruptScanner_.store(true);
}

void ConsistencyCheckManagerTest::SetUpTestCase() {}

void ConsistencyCheckManagerTest::TearDownTestCase() {}

void ConsistencyCheckManagerTest::SetUp()
{
    Reset();
}

void ConsistencyCheckManagerTest::TearDown()
{
    Reset();
}

HWTEST_F(ConsistencyCheckManagerTest, DisableCheck_WhenRunningSceneIdle_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start DisableCheck_WhenRunningSceneIdle_001");
    GlobalScanner::GetInstance().scannerStatus_ = ScannerStatus::CHECK_SCAN;
    auto &manager = ConsistencyCheckManager::GetInstance();
    manager.runningScene_ = CheckScene::IDLE;
    
    manager.DisableCheck();
    
    EXPECT_FALSE(manager.checkEnabled_);
    EXPECT_TRUE(manager.isInterrupted_.load());
    EXPECT_TRUE(manager.pendingScenes_.empty());
    EXPECT_TRUE(GlobalScanner::GetInstance().isNotInterruptScanner_.load());
    MEDIA_INFO_LOG("End DisableCheck_WhenRunningSceneIdle_001");
}

HWTEST_F(ConsistencyCheckManagerTest, DisableCheck_WhenRunningSceneActive_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start DisableCheck_WhenRunningSceneActive_002");
    GlobalScanner::GetInstance().scannerStatus_ = ScannerStatus::CHECK_SCAN;
    auto &manager = ConsistencyCheckManager::GetInstance();
    manager.runningScene_ = CheckScene::LAKE;
    
    manager.DisableCheck();
    
    EXPECT_FALSE(manager.checkEnabled_);
    EXPECT_TRUE(manager.isInterrupted_.load());
    EXPECT_TRUE(manager.pendingScenes_.empty());
    EXPECT_FALSE(GlobalScanner::GetInstance().isNotInterruptScanner_.load());
    MEDIA_INFO_LOG("End DisableCheck_WhenRunningSceneActive_002");
}

HWTEST_F(ConsistencyCheckManagerTest, DisableCheck_WithPendingScenes_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start DisableCheck_WithPendingScenes_003");
    auto &manager = ConsistencyCheckManager::GetInstance();
    manager.runningScene_ = CheckScene::IDLE;
    manager.pendingScenes_.push_back(CheckScene::LAKE);
    manager.pendingScenes_.push_back(CheckScene::FILE_MANAGER);
    
    manager.DisableCheck();
    
    EXPECT_FALSE(manager.checkEnabled_);
    EXPECT_TRUE(manager.isInterrupted_.load());
    EXPECT_TRUE(manager.pendingScenes_.empty());
    MEDIA_INFO_LOG("End DisableCheck_WithPendingScenes_003");
}

HWTEST_F(ConsistencyCheckManagerTest, StopAll_WhenRunningSceneIdle_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start StopAll_WhenRunningSceneIdle_001");
    auto &manager = ConsistencyCheckManager::GetInstance();
    manager.runningScene_ = CheckScene::IDLE;
    manager.StopAll();
    
    EXPECT_TRUE(manager.checkEnabled_); // StopAll does not set checkEnabled_
    EXPECT_TRUE(manager.isInterrupted_.load());
    EXPECT_TRUE(manager.pendingScenes_.empty());
    MEDIA_INFO_LOG("End StopAll_WhenRunningSceneIdle_001");
}

HWTEST_F(ConsistencyCheckManagerTest, StopAll_WhenRunningSceneActive_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start StopAll_WhenRunningSceneActive_002");
    auto &manager = ConsistencyCheckManager::GetInstance();
    manager.runningScene_ = CheckScene::FILE_MANAGER;
    manager.StopAll();
    
    EXPECT_TRUE(manager.checkEnabled_); // StopAll does not set checkEnabled_
    EXPECT_TRUE(manager.isInterrupted_.load());
    EXPECT_TRUE(manager.pendingScenes_.empty());
    MEDIA_INFO_LOG("End StopAll_WhenRunningSceneActive_002");
}

HWTEST_F(ConsistencyCheckManagerTest, StopAll_WithPendingScenes_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start StopAll_WithPendingScenes_003");
    auto &manager = ConsistencyCheckManager::GetInstance();
    manager.runningScene_ = CheckScene::IDLE;
    manager.pendingScenes_.push_back(CheckScene::LAKE);
    manager.pendingScenes_.push_back(CheckScene::FILE_MANAGER);
    
    manager.StopAll();
    
    EXPECT_TRUE(manager.pendingScenes_.empty());
    MEDIA_INFO_LOG("End StopAll_WithPendingScenes_003");
}

HWTEST_F(ConsistencyCheckManagerTest, StopRunningScene_WhenRunningSceneIdle_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start StopRunningScene_WhenRunningSceneIdle_001");
    const size_t QUEUE_SIZE = 2;
    auto &manager = ConsistencyCheckManager::GetInstance();
    manager.runningScene_ = CheckScene::IDLE;
    manager.pendingScenes_.push_back(CheckScene::LAKE);
    manager.pendingScenes_.push_back(CheckScene::FILE_MANAGER);
    manager.StopRunningScene();
    
    EXPECT_TRUE(manager.checkEnabled_); // StopRunningScene does not set checkEnabled_
    EXPECT_TRUE(manager.isInterrupted_.load());
    EXPECT_EQ(manager.pendingScenes_.size(), QUEUE_SIZE); // StopRunningScene does not clear queue
    MEDIA_INFO_LOG("End StopRunningScene_WhenRunningSceneIdle_001");
}

HWTEST_F(ConsistencyCheckManagerTest, StopRunningScene_WhenRunningSceneActive_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start StopRunningScene_WhenRunningSceneActive_002");
    const size_t QUEUE_SIZE = 1;
    auto &manager = ConsistencyCheckManager::GetInstance();
    manager.runningScene_ = CheckScene::LAKE;
    manager.pendingScenes_.push_back(CheckScene::FILE_MANAGER);
    manager.StopRunningScene();
    
    EXPECT_TRUE(manager.checkEnabled_); // StopRunningScene does not set checkEnabled_
    EXPECT_TRUE(manager.isInterrupted_.load());
    EXPECT_EQ(manager.pendingScenes_.size(), QUEUE_SIZE); // StopRunningScene does not clear queue
    MEDIA_INFO_LOG("End StopRunningScene_WhenRunningSceneActive_002");
}

HWTEST_F(ConsistencyCheckManagerTest, StopScenesInternal_DisableCheckTrue_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start StopScenesInternal_DisableCheckTrue_001");
    auto &manager = ConsistencyCheckManager::GetInstance();
    manager.runningScene_ = CheckScene::IDLE;
    manager.checkEnabled_ = true;
    manager.pendingScenes_.push_back(CheckScene::LAKE);
    
    manager.StopScenesInternal(true, true);
    
    EXPECT_FALSE(manager.checkEnabled_);
    EXPECT_TRUE(manager.isInterrupted_.load());
    EXPECT_TRUE(manager.pendingScenes_.empty());
    MEDIA_INFO_LOG("End StopScenesInternal_DisableCheckTrue_001");
}

HWTEST_F(ConsistencyCheckManagerTest, StopScenesInternal_ClearQueueFalse_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start StopScenesInternal_ClearQueueFalse_002");
    auto &manager = ConsistencyCheckManager::GetInstance();
    manager.runningScene_ = CheckScene::IDLE;
    manager.pendingScenes_.push_back(CheckScene::LAKE);
    manager.pendingScenes_.push_back(CheckScene::FILE_MANAGER);
    
    manager.StopScenesInternal(false, false);
    
    EXPECT_EQ(manager.pendingScenes_.size(), 2);
    EXPECT_TRUE(manager.isInterrupted_.load());
    MEDIA_INFO_LOG("End StopScenesInternal_ClearQueueFalse_002");
}

HWTEST_F(ConsistencyCheckManagerTest, EnableCheck_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start EnableCheck_001");
    auto &manager = ConsistencyCheckManager::GetInstance();
    manager.checkEnabled_ = false;
    manager.EnableCheck();
    EXPECT_TRUE(manager.checkEnabled_);
    MEDIA_INFO_LOG("End EnableCheck_001");
}

HWTEST_F(ConsistencyCheckManagerTest, IsCheckAllowed_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start IsCheckAllowed_001");
    auto &manager = ConsistencyCheckManager::GetInstance();
    manager.checkEnabled_ = true;
    EXPECT_TRUE(manager.IsCheckAllowed());
    
    manager.checkEnabled_ = false;
    EXPECT_FALSE(manager.IsCheckAllowed());
    MEDIA_INFO_LOG("End IsCheckAllowed_001");
}

} // namespace Media
} // namespace OHOS
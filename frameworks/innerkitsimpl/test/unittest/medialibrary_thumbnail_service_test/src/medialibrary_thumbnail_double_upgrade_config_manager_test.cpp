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

#include "medialibrary_thumbnail_double_upgrade_config_manager_test.h"

#include <memory>

#define private public
#define protected public
#include "thumbnail_double_upgrade_config_manager.h"
#undef private
#undef protected

#include "media_log.h"

using namespace testing::ext;

namespace OHOS {
namespace Media {
constexpr int64_t TEST_TIME_1S_MS = 1000;
constexpr int64_t TEST_TIME_2S_MS = 2000;
constexpr int64_t TEST_TIME_3S_MS = 3000;
constexpr int64_t TEST_TIME_4S_MS = 4000;
constexpr int64_t TEST_TIME_5S_MS = 5000;

void MedialibraryThumbnailDoubleUpgradeConfigManagerTest::SetUpTestCase(void) {}

void MedialibraryThumbnailDoubleUpgradeConfigManagerTest::TearDownTestCase(void) {}

void MedialibraryThumbnailDoubleUpgradeConfigManagerTest::SetUp() {}

void MedialibraryThumbnailDoubleUpgradeConfigManagerTest::TearDown() {}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, GetInstance_test_001, TestSize.Level1)
{
    auto& instance1 = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    auto& instance2 = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    EXPECT_EQ(&instance1, &instance2);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, IsCompleted_test_001, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.status_ = ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::NOT_STARTED;
    EXPECT_FALSE(manager.IsCompleted());
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, IsCompleted_test_002, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.status_ = ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::IN_PROGRESS;
    EXPECT_FALSE(manager.IsCompleted());
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, IsCompleted_test_003, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.status_ = ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::COMPLETED;
    EXPECT_TRUE(manager.IsCompleted());
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, IsInProgress_test_001, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.status_ = ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::NOT_STARTED;
    EXPECT_FALSE(manager.IsInProgress());
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, IsInProgress_test_002, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.status_ = ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::IN_PROGRESS;
    EXPECT_TRUE(manager.IsInProgress());
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, IsInProgress_test_003, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.status_ = ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::COMPLETED;
    EXPECT_FALSE(manager.IsInProgress());
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, IsMainUser_test_001, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.accountType_ = AccountSA::OsAccountType::ADMIN;
    EXPECT_TRUE(manager.IsMainUser());
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, IsMainUser_test_002, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.accountType_ = AccountSA::OsAccountType::PRIVATE;
    EXPECT_FALSE(manager.IsMainUser());
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, IsMainUser_test_003, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.accountType_ = AccountSA::OsAccountType::END;
    EXPECT_FALSE(manager.IsMainUser());
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, IsPrivateUser_test_001, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.accountType_ = AccountSA::OsAccountType::PRIVATE;
    EXPECT_TRUE(manager.IsPrivateUser());
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, IsPrivateUser_test_002, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.accountType_ = AccountSA::OsAccountType::ADMIN;
    EXPECT_FALSE(manager.IsPrivateUser());
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, IsPrivateUser_test_003, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.accountType_ = AccountSA::OsAccountType::END;
    EXPECT_FALSE(manager.IsPrivateUser());
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, GetUpdateDuration_test_001, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.startTimeMs_ = TEST_TIME_1S_MS;
    manager.endTimeMs_ = TEST_TIME_5S_MS;
    EXPECT_EQ(manager.GetUpdateDuration(), TEST_TIME_5S_MS - TEST_TIME_1S_MS);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, GetUpdateDuration_test_002, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.startTimeMs_ = 0;
    manager.endTimeMs_ = TEST_TIME_5S_MS;
    EXPECT_EQ(manager.GetUpdateDuration(), 0);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, GetUpdateDuration_test_003, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.startTimeMs_ = TEST_TIME_1S_MS;
    manager.endTimeMs_ = 0;
    EXPECT_EQ(manager.GetUpdateDuration(), 0);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, GetUpdateDuration_test_004, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.startTimeMs_ = 0;
    manager.endTimeMs_ = 0;
    EXPECT_EQ(manager.GetUpdateDuration(), 0);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, IsTimeout_test_001, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.startTimeMs_ = 0;
    EXPECT_FALSE(manager.IsTimeout(TEST_TIME_1S_MS));
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, OnParameterChanged_test_001, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.OnParameterChanged(nullptr, "value", nullptr);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, OnParameterChanged_test_002, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.OnParameterChanged("key", nullptr, nullptr);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, OnDoubleUpgradeParameterChanged_test_001, TestSize.Level1)
{
    ThumbnailDoubleUpgradeConfigManager::OnDoubleUpgradeParameterChanged("key", "value", nullptr);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, OnDoubleUpgradeParameterChanged_test_002, TestSize.Level1)
{
    ThumbnailDoubleUpgradeConfigManager::OnDoubleUpgradeParameterChanged(nullptr, "value", nullptr);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, OnDoubleUpgradeParameterChanged_test_003, TestSize.Level1)
{
    ThumbnailDoubleUpgradeConfigManager::OnDoubleUpgradeParameterChanged("key", nullptr, nullptr);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, GetDoubleUpgradeFlag_test_001, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.accountType_ = AccountSA::OsAccountType::ADMIN;
    std::string flag;
    manager.GetDoubleUpgradeFlag(flag);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, GetDoubleUpgradeFlag_test_002, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.accountType_ = AccountSA::OsAccountType::PRIVATE;
    std::string flag;
    manager.GetDoubleUpgradeFlag(flag);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, GetDoubleUpgradeFlag_test_003, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.accountType_ = AccountSA::OsAccountType::END;
    std::string flag;
    manager.GetDoubleUpgradeFlag(flag);
    EXPECT_EQ(flag, "");
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, GetCurrentSpaceThreshold_test_001, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.status_ = ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::IN_PROGRESS;
    manager.updateStorageSizeThreshold_ = THUMBNAIL_FREE_SIZE_LIMIT_10;
    int32_t threshold = manager.GetCurrentSpaceThreshold(true);
    EXPECT_EQ(threshold, THUMBNAIL_FREE_SIZE_LIMIT_10);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, GetCurrentSpaceThreshold_test_002, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.status_ = ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::IN_PROGRESS;
    manager.updateStorageSizeThreshold_ = THUMBNAIL_FREE_SIZE_LIMIT_10;
    int32_t threshold = manager.GetCurrentSpaceThreshold(false);
    EXPECT_EQ(threshold, THUMBNAIL_FREE_SIZE_LIMIT_10);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, GetCurrentSpaceThreshold_test_003, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.status_ = ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::NOT_STARTED;
    int32_t threshold = manager.GetCurrentSpaceThreshold(true);
    EXPECT_EQ(threshold, THUMBNAIL_FREE_SIZE_LIMIT_5);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, GetCurrentSpaceThreshold_test_004, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.status_ = ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::NOT_STARTED;
    int32_t threshold = manager.GetCurrentSpaceThreshold(false);
    EXPECT_EQ(threshold, THUMBNAIL_FREE_SIZE_LIMIT_10);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, GetCurrentSpaceThreshold_test_005, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.status_ = ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::COMPLETED;
    int32_t threshold = manager.GetCurrentSpaceThreshold(false);
    EXPECT_EQ(threshold, THUMBNAIL_FREE_SIZE_LIMIT_10);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, DataReport_test_001, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.isDataReport_ = true;
    manager.DataReport();
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, SaveDataReportFlagToSp_test_001, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.isDataReport_ = false;
    manager.SaveDataReportFlagToSp();
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, HandleProcess_test_001, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.status_ = ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::NOT_STARTED;
    manager.HandleProcess(TEST_TIME_1S_MS, true, "1");
    EXPECT_EQ(manager.status_, ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::IN_PROGRESS);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, HandleProcess_test_002, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.status_ = ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::IN_PROGRESS;
    manager.startTimeMs_ = TEST_TIME_1S_MS;
    manager.HandleProcess(TEST_TIME_2S_MS, true, "0");
    EXPECT_EQ(manager.status_, ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::COMPLETED);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, HandleProcess_test_003, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.status_ = ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::NOT_STARTED;
    manager.HandleProcess(TEST_TIME_1S_MS, true, "invalid");
    EXPECT_EQ(manager.status_, ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::NOT_STARTED);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, HandleProcess_test_004, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.accountType_ = AccountSA::OsAccountType::END;
    manager.status_ = ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::NOT_STARTED;
    manager.HandleProcess(TEST_TIME_1S_MS, false, "");
    EXPECT_EQ(manager.status_, ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::NOT_STARTED);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, SaveInProcessInfoToSp_test_001, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.startTimeMs_ = TEST_TIME_1S_MS;
    manager.status_ = ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::NOT_STARTED;
    manager.SaveInProcessInfoToSp(TEST_TIME_2S_MS);
    EXPECT_EQ(manager.startTimeMs_, TEST_TIME_1S_MS);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, SaveInProcessInfoToSp_test_002, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.startTimeMs_ = 0;
    manager.status_ = ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::NOT_STARTED;
    manager.SaveInProcessInfoToSp(TEST_TIME_2S_MS);
    EXPECT_EQ(manager.startTimeMs_, TEST_TIME_2S_MS);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, SaveCompleteInfoToSp_test_001, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.endTimeMs_ = TEST_TIME_3S_MS;
    manager.status_ = ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::IN_PROGRESS;
    manager.SaveCompleteInfoToSp(TEST_TIME_4S_MS);
    EXPECT_EQ(manager.endTimeMs_, TEST_TIME_3S_MS);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, SaveCompleteInfoToSp_test_002, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.endTimeMs_ = 0;
    manager.status_ = ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::IN_PROGRESS;
    manager.SaveCompleteInfoToSp(TEST_TIME_4S_MS);
    EXPECT_EQ(manager.endTimeMs_, TEST_TIME_4S_MS);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, IsTimeout_test_002, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.startTimeMs_ = TEST_TIME_1S_MS;
    EXPECT_FALSE(manager.IsTimeout(TEST_TIME_2S_MS));
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, IsTimeout_test_003, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.startTimeMs_ = TEST_TIME_1S_MS;
    const int64_t timeoutMs = 12 * 60 * 60 * 1000;
    EXPECT_TRUE(manager.IsTimeout(TEST_TIME_1S_MS + timeoutMs));
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, StopCallbackTimer_test_001, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.callbackTimerId_ = 0;
    manager.StopCallbackTimer();
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, StartCallbackTimer_test_001, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.startTimeMs_ = 0;
    manager.StartCallbackTimer(TEST_TIME_1S_MS);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, OnTimerTimeout_test_001, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.status_ = ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::IN_PROGRESS;
    manager.OnTimerTimeout();
    EXPECT_EQ(manager.status_, ThumbnailDoubleUpgradeConfigManager::DoubleUpgradeStatus::COMPLETED);
}

HWTEST_F(MedialibraryThumbnailDoubleUpgradeConfigManagerTest, AsyncUnregisterParameterWatcher_test_001, TestSize.Level1)
{
    auto& manager = ThumbnailDoubleUpgradeConfigManager::GetInstance();
    manager.AsyncUnregisterParameterWatcher();
}
} // namespace Media
} // namespace OHOS
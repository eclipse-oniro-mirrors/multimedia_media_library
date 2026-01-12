/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "mtp_monitor.h"
#include "mtp_monitor_test.h"
#include "media_log.h"
#include "mtp_file_observer.h"
#include "mtp_medialibrary_manager.h"
#include "mtp_manager.h"
#include "mtp_store_observer.h"

using namespace std;
using namespace testing::ext;
 
namespace OHOS {
namespace Media {
 
void MtpMonitorTest::SetUpTestCase(void)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
}

void MtpMonitorTest::TearDownTestCase(void) {}
void MtpMonitorTest::SetUp() {}
void MtpMonitorTest::TearDown(void) {}

template<typename Predicate>
bool WaitFor(Predicate pred, std::chrono::milliseconds timeout = 300ms)
{
    auto start = std::chrono::steady_clock::now();
    while (!pred()) {
        if (std::chrono::steady_clock::now() - start > timeout) {
            return false;
        }
        std::this_thread::sleep_for(10ms);
    }
    return true;
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: mtpMonitor
 */
HWTEST_F(MtpMonitorTest, mtp_monitor_test_001, TestSize.Level1)
{
    std::shared_ptr<MtpMonitor> mtpMonitor = make_shared<MtpMonitor>();
    ASSERT_NE(mtpMonitor, nullptr);
    mtpMonitor->threadRunning_.store(true);
    mtpMonitor->operationPtr_ = make_shared<MtpOperation>();
    mtpMonitor->Start();
    mtpMonitor->Stop();
    mtpMonitor->threadRunning_.store(false);
    EXPECT_EQ(mtpMonitor->threadRunning_.load(), false);
    mtpMonitor = nullptr;
}

/*
 * Feature: MediaLibraryMTP
 * Function: Start
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Start creates a new thread
 */
HWTEST_F(MtpMonitorTest, mtp_monitor_test_002, TestSize.Level1)
{
    auto monitor = make_shared<MtpMonitor>();
    EXPECT_FALSE(monitor->threadRunning_.load());
    EXPECT_EQ(monitor->thread_, nullptr);

    monitor->Start();

    EXPECT_TRUE(monitor->threadRunning_.load());
    EXPECT_NE(monitor->thread_, nullptr);

    monitor->Stop(); // clean up
}

/*
 * Feature: MediaLibraryMTP
 * Function: Start
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Second Start does nothing
 */
HWTEST_F(MtpMonitorTest, mtp_monitor_test_003, TestSize.Level1)
{
    auto monitor = make_shared<MtpMonitor>();
    monitor->Start();

    auto threadPtr = monitor->thread_.get();
    bool wasRunning = monitor->threadRunning_.load();

    monitor->Start();

    EXPECT_EQ(monitor->thread_.get(), threadPtr);
    EXPECT_EQ(monitor->threadRunning_.load(), wasRunning);

    monitor->Stop();
}

/*
 * Feature: MediaLibraryMTP
 * Function: Stop
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Stop on fresh instance is safe
 */
HWTEST_F(MtpMonitorTest, mtp_monitor_test_004, TestSize.Level1)
{
    auto monitor = make_shared<MtpMonitor>();
    EXPECT_EQ(monitor->thread_, nullptr);

    monitor->Stop();

    EXPECT_EQ(monitor->thread_, nullptr);
    EXPECT_FALSE(monitor->threadRunning_.load());
}

/*
 * Feature: MediaLibraryMTP
 * Function: Stop
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Stop joins the thread
 */
HWTEST_F(MtpMonitorTest, mtp_monitor_test_005, TestSize.Level1)
{
    auto monitor = make_shared<MtpMonitor>();
    monitor->Start();

    EXPECT_NE(monitor->thread_, nullptr);
    EXPECT_TRUE(monitor->thread_->joinable());

    monitor->Stop();

    EXPECT_EQ(monitor->thread_, nullptr);
    EXPECT_FALSE(monitor->threadRunning_.load());
}

/*
 * Feature: MediaLibraryMTP
 * Function: Stop
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: moved-from or detached thread (simulate via default-constructed)
 */
HWTEST_F(MtpMonitorTest, mtp_monitor_test_006, TestSize.Level1)
{
    auto monitor = make_shared<MtpMonitor>();
    monitor->thread_ = make_unique<thread>();
    monitor->threadRunning_.store(true);

    EXPECT_FALSE(monitor->thread_->joinable());

    monitor->Stop();

    EXPECT_EQ(monitor->thread_, nullptr);
    EXPECT_FALSE(monitor->threadRunning_.load());
}

/*
 * Feature: MediaLibraryMTP
 * Function: Run
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Run creates operationPtr_ when it is initially null
 */
HWTEST_F(MtpMonitorTest, mtp_monitor_test_007, TestSize.Level1)
{
    auto monitor = std::make_shared<MtpMonitor>();
    EXPECT_EQ(monitor->operationPtr_, nullptr);
    EXPECT_FALSE(monitor->threadRunning_.load());

    monitor->Start();
    ASSERT_TRUE(WaitFor([monitor]() {
        return monitor->operationPtr_ != nullptr;
    }));

    EXPECT_NE(monitor->operationPtr_, nullptr);

    monitor->Stop();
}

/*
 * Feature: MediaLibraryMTP
 * Function: Run
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Run stays in loop as long as threadRunning_ is true
 */
HWTEST_F(MtpMonitorTest, mtp_monitor_test_008, TestSize.Level1)
{
    auto monitor = std::make_shared<MtpMonitor>();
    monitor->Start();

    EXPECT_TRUE(monitor->threadRunning_.load());
    EXPECT_NE(monitor->thread_, nullptr);

    std::this_thread::sleep_for(50ms);
    EXPECT_TRUE(monitor->threadRunning_.load());

    monitor->Stop();
}

/*
 * Feature: MediaLibraryMTP
 * Function: Run
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Stop() causes Run() to exit gracefully
 */
HWTEST_F(MtpMonitorTest, mtp_monitor_test_009, TestSize.Level1)
{
    auto monitor = std::make_shared<MtpMonitor>();
    monitor->Start();
    ASSERT_NE(monitor->thread_, nullptr);
    ASSERT_TRUE(monitor->threadRunning_.load());

    monitor->Stop();

    ASSERT_TRUE(WaitFor([monitor]() {
        return !monitor->threadRunning_.load();
    }));

    EXPECT_EQ(monitor->thread_, nullptr);
}

/*
 * Feature: MediaLibraryMTP
 * Function: Run
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: After Run exits, cleanup code is executed
 */
HWTEST_F(MtpMonitorTest, mtp_monitor_test_010, TestSize.Level1)
{
    auto monitor = std::make_shared<MtpMonitor>();
    monitor->Start();

    ASSERT_TRUE(WaitFor([monitor]() {
        return monitor->operationPtr_ != nullptr;
    }));

    monitor->Stop();

    ASSERT_TRUE(WaitFor([monitor]() {
        return !monitor->threadRunning_.load();
    }));

    EXPECT_EQ(monitor->operationPtr_, nullptr);
    EXPECT_EQ(monitor->thread_, nullptr);
    EXPECT_FALSE(monitor->threadRunning_.load());
}

/*
 * Feature: MediaLibraryMTP
 * Function: Run
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Run sleeps when non-fatal error occurs and thread is still running
 */
HWTEST_F(MtpMonitorTest, mtp_monitor_run_test_011, TestSize.Level1)
{
    auto monitor = make_shared<MtpMonitor>();
    auto op = make_shared<MtpOperation>();
    monitor->operationPtr_ = op;
    monitor->threadRunning_.store(true);

    std::thread runThread([monitor]() {
        monitor->Run();
    });
    usleep(15000);
    EXPECT_TRUE(monitor->threadRunning_.load());

    monitor->threadRunning_.store(false);
    if (runThread.joinable()) {
        runThread.join();
    }
    EXPECT_FALSE(monitor->threadRunning_.load());
}

/*
 * Feature: MediaLibraryMTP
 * Function: Run
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Run exits cleanly when threadRunning_ is set to false externally
 */
HWTEST_F(MtpMonitorTest, mtp_monitor_run_test_012, TestSize.Level1)
{
    auto monitor = make_shared<MtpMonitor>();
    monitor->operationPtr_ = make_shared<MtpOperation>();
    monitor->threadRunning_.store(true);

    std::thread runThread([monitor]() {
        monitor->Run();
    });
    usleep(5000);

    monitor->threadRunning_.store(false);

    if (runThread.joinable()) {
        runThread.join();
    }

    EXPECT_FALSE(monitor->threadRunning_.load());
    EXPECT_EQ(monitor->operationPtr_, nullptr);
}
} // namespace Media
} // namespace OHOS
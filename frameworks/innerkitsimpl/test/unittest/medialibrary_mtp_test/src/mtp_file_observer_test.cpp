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
#include "mtp_file_observer_test.h"
#include "mtp_file_observer.h"
#include <memory>
#include <securec.h>
#include <string>
#include <sys/inotify.h>
#include <unistd.h>
#include "media_log.h"
#include "mtp_manager.h"
#include "mtp_media_library.h"
#include <fcntl.h>
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {

void MtpFileObserverTest::SetUpTestCase(void)
{
    OHOS::Media::MtpManager::GetInstance().mtpMode_ = OHOS::Media::MtpManager::MtpMode::MTP_MODE;
}

void MtpFileObserverTest::TearDownTestCase(void) {}
void MtpFileObserverTest::SetUp() {}
void MtpFileObserverTest::TearDown(void) {}

const std::string PATH_SEPARATOR = "/";

struct MoveInfo {
    uint32_t cookie;
    std::string path;
} g_moveInfo;
const std::string REAL_DOCUMENT_FILE = "/storage/media/100/local/files/Docs/Document";
const std::string REAL_STORAGE_FILE = "/storage/media/100/local/files/Docs";

void WriteFakeInotifyEvent(int writeFd, int wd, uint32_t mask, const char* name = nullptr)
{
    struct inotify_event event = {0};
    event.wd = wd;
    event.mask = mask;
    if (name != nullptr) {
        size_t len = strlen(name);
        event.len = static_cast<uint32_t>(len + 1);
        ssize_t ret1 = write(writeFd, &event, sizeof(struct inotify_event));
        (void)ret1;
        ssize_t ret2 = write(writeFd, name, event.len);
        (void)ret2;
    } else {
        event.len = 0;
        write(writeFd, &event, sizeof(struct inotify_event));
    }
}
/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: EraseFromWatchMap
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_001, TestSize.Level1)
{
    MtpFileObserver::EraseFromWatchMap(REAL_DOCUMENT_FILE);
    MtpFileObserver::isRunning_ = false;
    ContextSptr context = make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    EXPECT_TRUE(MtpFileObserver::WatchPathThread(context));
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: UpdateWatchMap
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_002, TestSize.Level1)
{
    MtpFileObserver::UpdateWatchMap(REAL_DOCUMENT_FILE);
    MtpFileObserver::isRunning_ = false;
    ContextSptr context = make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    EXPECT_TRUE(MtpFileObserver::WatchPathThread(context));
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: DealWatchMap
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_003, TestSize.Level1)
{
    inotify_event event;
    event.mask = IN_DELETE;
    MtpFileObserver::DealWatchMap(event, REAL_DOCUMENT_FILE);
    MtpFileObserver::isRunning_ = false;
    ContextSptr context = make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    EXPECT_TRUE(MtpFileObserver::WatchPathThread(context));
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: DealWatchMap
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_004, TestSize.Level1)
{
    inotify_event event;
    event.mask = IN_MOVED_TO;
    event.cookie = 0x12345678;
    MtpFileObserver::DealWatchMap(event, REAL_DOCUMENT_FILE);
    MtpFileObserver::isRunning_ = false;
    ContextSptr context = make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    EXPECT_TRUE(MtpFileObserver::WatchPathThread(context));
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: DealWatchMap
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_005, TestSize.Level1)
{
    inotify_event event;
    event.mask = IN_MOVED_TO;
    event.cookie = 0x12345678;
    MtpFileObserver::DealWatchMap(event, REAL_DOCUMENT_FILE);
    MtpFileObserver::isRunning_ = false;
    ContextSptr context = make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    EXPECT_TRUE(MtpFileObserver::WatchPathThread(context));
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: SendEvent
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_006, TestSize.Level1)
{
    inotify_event event;
    event.mask = IN_MOVED_TO;
    event.cookie = 0x12345678;
    ContextSptr context = make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    MtpFileObserver::SendEvent(event, REAL_STORAGE_FILE, context);
    MtpFileObserver::isRunning_ = false;
    EXPECT_TRUE(MtpFileObserver::WatchPathThread(context));
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: SendEvent
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_007, TestSize.Level1)
{
    inotify_event event;
    event.mask = IN_MOVED_FROM;
    event.cookie = 0x12345678;
    ContextSptr context = make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    MtpFileObserver::SendEvent(event, REAL_STORAGE_FILE, context);
    MtpFileObserver::isRunning_ = false;
    EXPECT_TRUE(MtpFileObserver::WatchPathThread(context));
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: SendEvent
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_008, TestSize.Level1)
{
    inotify_event event;
    event.mask = IN_CLOSE_WRITE;
    event.cookie = 0x12345678;
    ContextSptr context = make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    MtpFileObserver::SendEvent(event, REAL_STORAGE_FILE, context);
    MtpFileObserver::isRunning_ = false;
    EXPECT_TRUE(MtpFileObserver::WatchPathThread(context));
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: SendEvent
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_009, TestSize.Level1)
{
    inotify_event event;
    event.mask = 0x400002C0;
    event.cookie = 0x12345678;
    ContextSptr context = make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    MtpFileObserver::SendEvent(event, REAL_STORAGE_FILE, context);
    MtpFileObserver::isRunning_ = false;
    EXPECT_TRUE(MtpFileObserver::WatchPathThread(context));
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: SendEvent
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0010, TestSize.Level1)
{
    inotify_event event;
    event.mask = 0x000003C8;
    event.cookie = 0x12345678;
    ContextSptr context = make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    MtpFileObserver::SendEvent(event, REAL_STORAGE_FILE, context);
    MtpFileObserver::isRunning_ = false;
    EXPECT_TRUE(MtpFileObserver::WatchPathThread(context));
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: SendBattery
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0012, TestSize.Level1)
{
    ContextSptr context = make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    MtpFileObserver::SendBattery(context);
    MtpFileObserver::isRunning_ = false;
    EXPECT_TRUE(MtpFileObserver::WatchPathThread(context));
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: StartFileInotify
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0014, TestSize.Level1)
{
    std::shared_ptr<MtpFileObserver> mtpFileObserver = make_shared<MtpFileObserver>();
    ASSERT_NE(mtpFileObserver, nullptr);
    EXPECT_TRUE(mtpFileObserver->StartFileInotify());
    mtpFileObserver->StopFileInotify();
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: WatchPathThread
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0015, TestSize.Level1)
{
    MtpFileObserver::isRunning_ = false;
    ContextSptr context = make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    EXPECT_TRUE(MtpFileObserver::WatchPathThread(context));
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: AddFileInotify
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0016, TestSize.Level1)
{
    std::shared_ptr<MtpFileObserver> mtpFileObserver = make_shared<MtpFileObserver>();
    ASSERT_NE(mtpFileObserver, nullptr);
    ContextSptr context = make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpFileObserver->StartFileInotify();
    mtpFileObserver->startThread_ = false;
    mtpFileObserver->inotifySuccess_ = true;
    mtpFileObserver->AddFileInotify(REAL_STORAGE_FILE, REAL_DOCUMENT_FILE, context);
    mtpFileObserver->StopFileInotify();
    EXPECT_TRUE(MtpFileObserver::WatchPathThread(context));
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: EraseFromWatchMap
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0017, TestSize.Level1)
{
    std::shared_ptr<MtpFileObserver> mtpFileObserver = make_shared<MtpFileObserver>();
    ASSERT_NE(mtpFileObserver, nullptr);
    ContextSptr context = make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpFileObserver->StartFileInotify();
    MtpFileObserver::EraseFromWatchMap(REAL_DOCUMENT_FILE);
    mtpFileObserver->StopFileInotify();
    EXPECT_TRUE(MtpFileObserver::WatchPathThread(context));
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: EraseFromWatchMap
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0018, TestSize.Level1)
{
    std::shared_ptr<MtpFileObserver> mtpFileObserver = std::make_shared<MtpFileObserver>();
    ASSERT_NE(mtpFileObserver, nullptr);

    std::string path = "/file/Docs";

    MtpFileObserver::watchMap_.insert(std::pair<int, std::string>(1, "/file/Docs/1.jpg"));
    MtpFileObserver::watchMap_.insert(std::pair<int, std::string>(1, "/file/Docs/"));

    mtpFileObserver->EraseFromWatchMap(path);
    EXPECT_TRUE(MtpFileObserver::watchMap_.empty());
}


/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: DealWatchMap
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0019, TestSize.Level1)
{
    inotify_event event;
    event.mask = 0x40000200;
    std::string path = "/file/Docs";

    MtpFileObserver::watchMap_.insert(std::pair<int, std::string>(1, "/file/Docs/1.jpg"));
    MtpFileObserver::watchMap_.insert(std::pair<int, std::string>(1, "/file/Docs/"));

    MtpFileObserver::DealWatchMap(event, path);
    EXPECT_TRUE(MtpFileObserver::watchMap_.empty());
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: DealWatchMap
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0020, TestSize.Level1)
{
    inotify_event event;
    event.mask = 0x40000040;
    std::string path = "/file/Docs";

    MtpFileObserver::DealWatchMap(event, path);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: DealWatchMap
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0021, TestSize.Level1)
{
    inotify_event event;
    event.mask = 0x40000080;
    std::string path = "/file/Docs";

    MtpFileObserver::DealWatchMap(event, path);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: DealWatchMap
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0022, TestSize.Level1)
{
    inotify_event event;
    event.mask = 0x40000000;
    std::string path = "/file/Docs";

    MtpFileObserver::DealWatchMap(event, path);
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: SendEvent
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0023, TestSize.Level1)
{
    inotify_event event;
    std::string path = "/file/Docs";
    ContextSptr context = std::make_shared<MtpOperationContext>();
    MtpFileObserver::isEventThreadRunning_.store(false);
    ASSERT_NE(context, nullptr);
    event.mask = 0x00000100;
    MtpFileObserver::SendEvent(event, path, context);

    ASSERT_NE(context, nullptr);
    event.mask = 0x00000080;
    MtpFileObserver::SendEvent(event, path, context);

    ASSERT_NE(context, nullptr);
    event.mask = 0x00000008;
    MtpFileObserver::SendEvent(event, path, context);

    ASSERT_NE(context, nullptr);
    event.mask = 0x00000200;
    MtpFileObserver::SendEvent(event, path, context);

    ASSERT_NE(context, nullptr);
    event.mask = 0x00000040;
    MtpFileObserver::SendEvent(event, path, context);
}

/*
 * Feature: MediaLibraryMTP
 * Function: StopFileInotify
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: StopFileInotify success
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0024, TestSize.Level1)
{
    std::shared_ptr<MtpFileObserver> mtpFileObserver = make_shared<MtpFileObserver>();
    ASSERT_NE(mtpFileObserver, nullptr);
    MtpFileObserver::isRunning_ = true;
    MtpFileObserver::watchMap_.clear();
    EXPECT_TRUE(mtpFileObserver->StopFileInotify());
}

/*
 * Feature: MediaLibraryMTP
 * Function: AddToQueue
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: AddToQueue success
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0025, TestSize.Level1)
{
    MtpFileObserver::StopSendEventThread();
    MtpFileObserver::isEventThreadRunning_.store(true);
    MtpFileObserver::AddToQueue(MTP_EVENT_OBJECT_ADDED_CODE, 0);
    EXPECT_EQ(MtpFileObserver::eventQueue_.size(), 1);
    MtpFileObserver::StopSendEventThread();
}

/*
 * Feature: MediaLibraryMTP
 * Function: SendEventThread
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: SendEventThread with isEventThreadRunning_ false
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0027, TestSize.Level1)
{
    MtpFileObserver::StopSendEventThread();
    MtpFileObserver::isEventThreadRunning_.store(true);
    MtpFileObserver::AddToQueue(MTP_EVENT_OBJECT_ADDED_CODE, 0);
    ContextSptr context = make_shared<MtpOperationContext>();
    MtpFileObserver::isEventThreadRunning_.store(false);
    ASSERT_NE(context, nullptr);
    MtpFileObserver::SendEventThread(context);
    EXPECT_EQ(MtpFileObserver::eventQueue_.size(), 1);
    MtpFileObserver::StopSendEventThread();
}

/*
 * Feature: MediaLibraryMTP
 * Function: EraseFromWatchMap
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: EraseFromWatchMap removes the entry with path exactly equal to input
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0028, TestSize.Level1)
{
    MtpFileObserver::inotifyFd_ = 0;
    const std::string testPath = "/storage/media/100/local/files/TestDir";

    int fakeWd = 12345;
    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::eventLock_);
        MtpFileObserver::watchMap_[fakeWd] = testPath;
    }

    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::eventLock_);
        EXPECT_EQ(MtpFileObserver::watchMap_.size(), 1u);
        EXPECT_EQ(MtpFileObserver::watchMap_.at(fakeWd), testPath);
    }

    MtpFileObserver::EraseFromWatchMap(testPath);

    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::eventLock_);
        EXPECT_TRUE(MtpFileObserver::watchMap_.empty());
    }
}

/*
 * Feature: MediaLibraryMTP
 * Function: EraseFromWatchMap
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: EraseFromWatchMap leaves watchMap_ unchanged when no match
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0029, TestSize.Level1)
{
    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::eventLock_);
        MtpFileObserver::watchMap_.clear();
    }

    const std::string unrelatedPath = "/storage/media/100/local/files/Unrelated";
    int fakeWd = 54321;
    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::eventLock_);
        MtpFileObserver::watchMap_[fakeWd] = unrelatedPath;
    }

    const std::string targetPath = "/storage/media/100/local/files/TestDir";
    MtpFileObserver::EraseFromWatchMap(targetPath);

    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::eventLock_);
        EXPECT_EQ(MtpFileObserver::watchMap_.size(), 1u);
        EXPECT_EQ(MtpFileObserver::watchMap_.at(fakeWd), unrelatedPath);
    }
}

/*
 * Feature: MediaLibraryMTP
 * Function: UpdateWatchMap
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: UpdateWatchMap does nothing on invalid inputs
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0030, TestSize.Level1)
{
    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::eventLock_);
        MtpFileObserver::watchMap_.clear();
    }

    int wd = 100;
    const std::string originalPath = "/dummy/path";
    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::eventLock_);
        MtpFileObserver::watchMap_[wd] = originalPath;
    }

    MtpFileObserver::UpdateWatchMap("");
    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::eventLock_);
        EXPECT_EQ(MtpFileObserver::watchMap_.at(wd), originalPath);
    }

    MtpFileObserver::UpdateWatchMap("/valid/new/path");
    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::eventLock_);
        EXPECT_EQ(MtpFileObserver::watchMap_.at(wd), originalPath);
    }
}

/*
 * Feature: MediaLibraryMTP
 * Function: AddInotifyEvents
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: AddInotifyEvents returns false on invalid/empty read
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0034, TestSize.Level1)
{
    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);

    close(pipefd[1]);
    MtpFileObserver::isRunning_ = true;

    bool result = MtpFileObserver::AddInotifyEvents(pipefd[0], nullptr);
    EXPECT_FALSE(result) << "Expected false when no event data";

    close(pipefd[0]);
    close(pipefd[1]);
}

/*
 * Feature: MediaLibraryMTP
 * Function: AddInotifyEvents
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Returns true but does not process any events
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0035, TestSize.Level1)
{
    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);

    WriteFakeInotifyEvent(pipefd[1], 123, IN_CREATE, "test.txt");
    close(pipefd[1]);

    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::mutex_);
        while (!MtpFileObserver::eventQueue_.empty()) {
            MtpFileObserver::eventQueue_.pop();
        }
    }

    MtpFileObserver::isRunning_ = false;

    bool result = MtpFileObserver::AddInotifyEvents(pipefd[0], nullptr);

    EXPECT_TRUE(result) << "Should return true even if not running";

    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::mutex_);
        EXPECT_EQ(MtpFileObserver::eventQueue_.size(), 0u);
    }

    close(pipefd[0]);
}

/*
 * Feature: MediaLibraryMTP
 * Function: AddInotifyEvents
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Does not call SendEvent for len=0 events
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0036, TestSize.Level1)
{
    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);

    const int wd = 999;
    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::eventLock_);
        MtpFileObserver::watchMap_[wd] = "/valid/path";
    }

    struct inotify_event ev = {0};
    ev.wd = wd;
    ev.mask = IN_DELETE_SELF;
    ev.len = 0;
    write(pipefd[1], &ev, sizeof(ev));
    close(pipefd[1]);

    size_t oldQueueSize;
    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::mutex_);
        oldQueueSize = MtpFileObserver::eventQueue_.size();
        while (!MtpFileObserver::eventQueue_.empty()) {
            MtpFileObserver::eventQueue_.pop();
        }
    }

    MtpFileObserver::isRunning_ = true;

    bool result = MtpFileObserver::AddInotifyEvents(pipefd[0], nullptr);
    EXPECT_TRUE(result);

    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::mutex_);
        EXPECT_EQ(MtpFileObserver::eventQueue_.size(), 0u);
    }

    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::eventLock_);
        MtpFileObserver::watchMap_.clear();
    }

    close(pipefd[0]);
}

/*
 * Feature: MediaLibraryMTP
 * Function: AddInotifyEvents
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Returns true, eventQueue unchanged
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0037, TestSize.Level1)
{
    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);

    const int unknownWd = 8888;
    WriteFakeInotifyEvent(pipefd[1], unknownWd, IN_MOVED_TO, "unknown.txt");
    close(pipefd[1]);

    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::eventLock_);
        MtpFileObserver::watchMap_.clear();
    }

    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::mutex_);
        while (!MtpFileObserver::eventQueue_.empty()) {
            MtpFileObserver::eventQueue_.pop();
        }
    }

    MtpFileObserver::isRunning_ = true;

    bool result = MtpFileObserver::AddInotifyEvents(pipefd[0], nullptr);
    EXPECT_TRUE(result);

    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::mutex_);
        EXPECT_EQ(MtpFileObserver::eventQueue_.size(), 0u);
    }

    close(pipefd[0]);
}

/*
 * Feature: MediaLibraryMTP
 * Function: WatchPathThread
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Thread joins within 100ms without blocking in AddInotifyEvents
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0039, TestSize.Level1)
{
    int inotifyFd = inotify_init();
    ASSERT_NE(inotifyFd, -1);
    MtpFileObserver::inotifyFd_ = inotifyFd;

    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::eventLock_);
        MtpFileObserver::watchMap_.clear();
    }

    MtpFileObserver::isRunning_ = true;
    auto context = static_cast<ContextSptr>(nullptr);
    std::thread worker(&MtpFileObserver::WatchPathThread, context);

    usleep(5000);

    MtpFileObserver::isRunning_ = false;
    auto start = std::chrono::steady_clock::now();
    if (worker.joinable()) {
        worker.join();
    }
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    ASSERT_LE(duration.count(), 100);
    close(inotifyFd);
}

/*
 * Feature: MediaLibraryMTP
 * Function: AddPathToWatchMap
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Path not added to watchMap due to inotify failure; returns early without crash
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0040, TestSize.Level1)
{
    if (MtpFileObserver::inotifyFd_ > 0) {
        close(MtpFileObserver::inotifyFd_);
    }
    MtpFileObserver::inotifyFd_ = -1;

    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::eventLock_);
        MtpFileObserver::watchMap_.clear();
    }

    std::string path = "/any/nonempty/path";
    std::shared_ptr<MtpFileObserver> mtpFileObserver = make_shared<MtpFileObserver>();
    mtpFileObserver->AddPathToWatchMap(path);

    std::lock_guard<std::mutex> lock(MtpFileObserver::eventLock_);
    bool found = false;
    for (const auto& item : MtpFileObserver::watchMap_) {
        if (item.second == path) {
            found = true;
            break;
        }
    }
    EXPECT_FALSE(found);
}

/*
 * Feature: MediaLibraryMTP
 * Function: AddPathToWatchMap
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Valid existing directory is registered via inotify and stored in watchMap
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0041, TestSize.Level1)
{
    if (MtpFileObserver::inotifyFd_ > 0) {
        close(MtpFileObserver::inotifyFd_);
    }
    MtpFileObserver::inotifyFd_ = 0;
    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::eventLock_);
        MtpFileObserver::watchMap_.clear();
    }

    char tempDir[] = "/data/local/tmp/mtp_addpath_XXXXXX";
    char* dirName = mkdtemp(tempDir);
    ASSERT_NE(dirName, nullptr);
    std::string validPath(dirName);

    int inotifyFd = inotify_init();
    ASSERT_NE(inotifyFd, -1);
    MtpFileObserver::inotifyFd_ = inotifyFd;

    std::shared_ptr<MtpFileObserver> mtpFileObserver = make_shared<MtpFileObserver>();
    mtpFileObserver->AddPathToWatchMap(validPath);

    bool found = false;
    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::eventLock_);
        for (const auto& item : MtpFileObserver::watchMap_) {
            if (item.second == validPath) {
                found = true;
                break;
            }
        }
    }
    EXPECT_TRUE(found);
    close(inotifyFd);
    rmdir(validPath.c_str());

    MtpFileObserver::inotifyFd_ = 0;
    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::eventLock_);
        MtpFileObserver::watchMap_.clear();
    }
}

/*
 * Feature: MediaLibraryMTP
 * Function: StartSendEventThread
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Enters 'if (isEventThreadRunning_)' branch and calls StopSendEventThread safely
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0042, TestSize.Level1)
{
    bool originalIsEventThreadRunning = MtpFileObserver::isEventThreadRunning_.load();
    std::queue<std::pair<uint16_t, uint32_t>> originalQueue;
    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::mutex_);
        originalQueue = MtpFileObserver::eventQueue_;
    }

    auto context = std::make_shared<MtpOperationContext>();
    context->transactionID = 12345;

    context->mtpDriver = std::make_shared<MtpDriver>();
    MtpFileObserver::isEventThreadRunning_.store(true);
    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::mutex_);
        MtpFileObserver::eventQueue_.push({0x1001, 100});
        EXPECT_EQ(MtpFileObserver::eventQueue_.size(), 1u);
    }

    MtpFileObserver::StartSendEventThread(context);
    EXPECT_TRUE(MtpFileObserver::isEventThreadRunning_.load());
    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::mutex_);
        EXPECT_EQ(MtpFileObserver::eventQueue_.size(), 0u);
    }
    MtpFileObserver::StopSendEventThread();
    usleep(5000);

    MtpFileObserver::isEventThreadRunning_.store(originalIsEventThreadRunning);
    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::mutex_);
        std::swap(MtpFileObserver::eventQueue_, originalQueue);
    }
}

/*
 * Feature: MediaLibraryMTP
 * Function: AddFileInotify
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Enters outer if but skips watch registration due to empty realPath
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0043, TestSize.Level1)
{
    auto mtpObserver = std::make_shared<MtpFileObserver>();

    bool origInotifySuccess = mtpObserver->inotifySuccess_;
    bool origStartThread = mtpObserver->startThread_;
    auto origWatchMap = MtpFileObserver::watchMap_;
    int origInotifyFd = MtpFileObserver::inotifyFd_;

    mtpObserver->inotifySuccess_ = true;
    mtpObserver->startThread_ = true;
    MtpFileObserver::inotifyFd_ = 1;

    std::string path = "/valid/path";
    std::string realPath = "";
    ContextSptr context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);

    mtpObserver->AddFileInotify(path, realPath, context);

    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::eventLock_);
        EXPECT_EQ(MtpFileObserver::watchMap_.size(), origWatchMap.size());
    }
    mtpObserver->inotifySuccess_ = origInotifySuccess;
    mtpObserver->startThread_ = origStartThread;
    MtpFileObserver::watchMap_ = origWatchMap;
    MtpFileObserver::inotifyFd_ = origInotifyFd;
}

/*
 * Feature: MediaLibraryMTP
 * Function: AddFileInotify
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Enters 'if (!startThread_)' branch and launches background threads
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0044, TestSize.Level1)
{
    auto mtpObserver = std::make_shared<MtpFileObserver>();
    bool origInotifySuccess = mtpObserver->inotifySuccess_;
    bool origStartThread = mtpObserver->startThread_;
    auto origWatchMap = MtpFileObserver::watchMap_;
    int origInotifyFd = MtpFileObserver::inotifyFd_;

    mtpObserver->inotifySuccess_ = true;
    mtpObserver->startThread_ = false;
    MtpFileObserver::inotifyFd_ = 1;

    std::string path = "/valid/path";
    std::string realPath = "/valid/real/path";
    ContextSptr context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->transactionID = 12345;
    mtpObserver->AddFileInotify(path, realPath, context);
    EXPECT_TRUE(mtpObserver->startThread_);

    {
        std::lock_guard<std::mutex> lock(MtpFileObserver::eventLock_);
        EXPECT_EQ(MtpFileObserver::watchMap_.size(), 1);
    }

    usleep(5000);
    MtpFileObserver::StopSendEventThread();
    mtpObserver->inotifySuccess_ = origInotifySuccess;
    mtpObserver->startThread_ = origStartThread;
    MtpFileObserver::watchMap_ = origWatchMap;
    MtpFileObserver::inotifyFd_ = origInotifyFd;
}

/*
 * Feature: MediaLibraryMTP
 * Function: SendEventThread
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Enters 'while (isEventThreadRunning_)' loop and processes one event from queue
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0045, TestSize.Level1)
{
    auto observer = std::make_shared<MtpFileObserver>();
    auto context = std::make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    context->transactionID = 1001;

    context->mtpDriver = std::make_shared<MtpDriver>();
    observer->isEventThreadRunning_.store(true);
    {
        std::lock_guard<std::mutex> lock(observer->mutex_);
        observer->eventQueue_.push({0x4002, 5678});
    }

    std::thread worker([observer, context]() {
        observer->SendEventThread(context);
    });
    usleep(10000);

    observer->isEventThreadRunning_.store(false);
    observer->cv_.notify_all();

    if (worker.joinable()) {
        worker.join();
    }
    {
        std::lock_guard<std::mutex> lock(observer->mutex_);
        EXPECT_TRUE(observer->eventQueue_.empty());
    }
}
} // namespace Media
} // namespace OHOS
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
#include "mtp_media_library.h"
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {

void MtpFileObserverTest::SetUpTestCase(void) {}
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

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: EraseFromWatchMap
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_001, TestSize.Level0)
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
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_002, TestSize.Level0)
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
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_003, TestSize.Level0)
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
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_004, TestSize.Level0)
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
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_005, TestSize.Level0)
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
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_006, TestSize.Level0)
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
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_007, TestSize.Level0)
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
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_008, TestSize.Level0)
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
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_009, TestSize.Level0)
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
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0010, TestSize.Level0)
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
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0012, TestSize.Level0)
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
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0014, TestSize.Level0)
{
    std::shared_ptr<MtpFileObserver> mtpFileObserver = make_shared<MtpFileObserver>();
    ASSERT_NE(mtpFileObserver, nullptr);
    EXPECT_TRUE(mtpFileObserver->StartFileInotify());
}

/*
 * Feature: MediaLibraryMTP
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: WatchPathThread
 */
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0015, TestSize.Level0)
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
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0016, TestSize.Level0)
{
    std::shared_ptr<MtpFileObserver> mtpFileObserver = make_shared<MtpFileObserver>();
    ASSERT_NE(mtpFileObserver, nullptr);
    ContextSptr context = make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpFileObserver->StartFileInotify();
    mtpFileObserver->startThread_ = false;
    mtpFileObserver->inotifySuccess_ = true;
    mtpFileObserver->AddFileInotify(REAL_STORAGE_FILE, REAL_DOCUMENT_FILE, context);
    MtpFileObserver::isRunning_ = false;
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
HWTEST_F(MtpFileObserverTest, mtp_file_observer_test_0017, TestSize.Level0)
{
    std::shared_ptr<MtpFileObserver> mtpFileObserver = make_shared<MtpFileObserver>();
    ASSERT_NE(mtpFileObserver, nullptr);
    ContextSptr context = make_shared<MtpOperationContext>();
    ASSERT_NE(context, nullptr);
    mtpFileObserver->StartFileInotify();
    MtpFileObserver::EraseFromWatchMap(REAL_DOCUMENT_FILE);
    MtpFileObserver::isRunning_ = false;
    EXPECT_TRUE(MtpFileObserver::WatchPathThread(context));
}
} // namespace Media
} // namespace OHOS
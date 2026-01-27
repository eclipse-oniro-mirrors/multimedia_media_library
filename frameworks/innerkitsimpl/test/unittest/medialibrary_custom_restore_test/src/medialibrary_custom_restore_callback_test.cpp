/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#define MLOG_TAG "CustomRestoreCallbackUnitTest"

#include <gtest/gtest.h>

#include "media_log.h"
#include "media_library_custom_restore.h"
#include "medialibrary_custom_restore_notify.h"
#include "medialibrary_custom_restore_observer_manager.h"
#include "medialibrary_errno.h"

using namespace testing;
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
class MediaLibraryCustomRestoreCallbackTest : public testing::Test {
public:
    // input testsuit setup step，setup invoked before all testcases
    static void SetUpTestCase(void);
    // input testsuit teardown step，teardown invoked after all testcases
    static void TearDownTestCase(void);
    // input testcase setup step，setup invoked before each testcases
    void SetUp();
    // input testcase teardown step，teardown invoked after each testcases
    void TearDown();
};

void MediaLibraryCustomRestoreCallbackTest::SetUpTestCase(void) {}

void MediaLibraryCustomRestoreCallbackTest::TearDownTestCase(void) {}

void MediaLibraryCustomRestoreCallbackTest::SetUp(void) {}

void MediaLibraryCustomRestoreCallbackTest::TearDown(void) {}

class MockCustomRestoreCallback : public CustomRestoreCallback {
    int32_t OnRestoreResult(RestoreResult restoreResult) override
    {
        MEDIA_INFO_LOG("MockCustomRestoreCallback OnRestoreResult stage: %{public}s, "
            "errCode: %{public}d, progress: %{public}d, "
            "uriType: %{public}d, uri: %{public}s, totalNum: %{public}d, "
            "successNum: %{public}d, failedNum: %{public}d, sameNum: %{public}d, "
            "cancelNum: %{public}d",
            (restoreResult.stage).c_str(),
            restoreResult.errCode, restoreResult.progress,
            restoreResult.uriType, (restoreResult.uri).c_str(),
            restoreResult.restoreInfo.totalNum, restoreResult.restoreInfo.successNum,
            restoreResult.restoreInfo.failedNum, restoreResult.restoreInfo.sameNum,
            restoreResult.restoreInfo.cancelNum);
        return E_OK;
    }
};

HWTEST_F(MediaLibraryCustomRestoreCallbackTest, Custom_Restore_Callback_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Custom_Restore_Callback_Test_001 Start");
    CustomRestore customRestore("customRestoreCallback", true);
    customRestore.Init("1", "2", "3", 4);
    MockCustomRestoreCallback customRestoreCallback;
    std::shared_ptr<MockCustomRestoreCallback> callback =
        std::make_shared<MockCustomRestoreCallback>(customRestoreCallback);
    EXPECT_EQ(customRestore.RegisterCustomRestoreCallback(callback), E_OK);
    CustomRestoreNotify customRestoreNotify;
    for (int32_t i = 1; i <= 100; ++i) {
        InnerRestoreResult restoreResult;
        restoreResult.stage = "stage";
        restoreResult.errCode = 0;
        restoreResult.progress = i;
        restoreResult.uriType = 2;
        restoreResult.uri = "customRestoreCallback";
        restoreResult.totalNum = i;
        restoreResult.successNum = i;
        restoreResult.failedNum = i;
        restoreResult.sameNum = i;
        restoreResult.cancelNum = i;
        customRestoreNotify.Notify("customRestoreCallback", restoreResult);
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));
    EXPECT_EQ(customRestore.UnregisterCustomRestoreCallback(callback), E_OK);
    MEDIA_INFO_LOG("Custom_Restore_Callback_Test_001 End");
}

HWTEST_F(MediaLibraryCustomRestoreCallbackTest, Custom_Restore_Callback_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Custom_Restore_Callback_Test_002 Start");
    CustomRestore customRestore("customRestoreCallback", true);
    EXPECT_NE(customRestore.RegisterCustomRestoreCallback(nullptr), E_OK);
    MEDIA_INFO_LOG("Custom_Restore_Callback_Test_002 End");
}

HWTEST_F(MediaLibraryCustomRestoreCallbackTest, Custom_Restore_Callback_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Custom_Restore_Callback_Test_003 Start");
    CustomRestore customRestore("customRestoreCallback", true);
    MockCustomRestoreCallback customRestoreCallback;
    std::shared_ptr<MockCustomRestoreCallback> callback =
        std::make_shared<MockCustomRestoreCallback>(customRestoreCallback);
    EXPECT_NE(customRestore.RegisterCustomRestoreCallback(callback), E_OK);
    MEDIA_INFO_LOG("Custom_Restore_Callback_Test_003 End");
}

HWTEST_F(MediaLibraryCustomRestoreCallbackTest, Custom_Restore_Callback_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Custom_Restore_Callback_Test_004 Start");
    CustomRestore customRestore("customRestoreCallback", true);
    EXPECT_NE(customRestore.UnregisterCustomRestoreCallback(nullptr), E_OK);
    MEDIA_INFO_LOG("Custom_Restore_Callback_Test_004 End");
}

HWTEST_F(MediaLibraryCustomRestoreCallbackTest, Custom_Restore_Callback_Test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Custom_Restore_Callback_Test_005 Start");
    CustomRestore customRestore("customRestoreCallback", true);
    MockCustomRestoreCallback customRestoreCallback;
    std::shared_ptr<MockCustomRestoreCallback> callback =
        std::make_shared<MockCustomRestoreCallback>(customRestoreCallback);
    CustomRestoreObserverManager::GetInstance().callbackMap_.Clear();
    EXPECT_NE(customRestore.UnregisterCustomRestoreCallback(callback), E_OK);
    MEDIA_INFO_LOG("Custom_Restore_Callback_Test_005 End");
}

}
}
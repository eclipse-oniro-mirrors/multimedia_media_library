/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define MLOG_TAG "MediaLibraryRelatedSystemStateTest"

#include "medialibrary_related_system_state_manager.h"
#include "media_library_related_system_state_test.h"
#include <chrono>
#include <thread>
#include "media_log.h"
#include "media_file_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_mock_tocken.h"
#include "medialibrary_unittest_utils.h"
#include "userfile_manager_types.h"


namespace OHOS {
namespace Media {
using namespace std;
using namespace testing::ext;

static uint64_t g_shellToken = 0;
static MediaLibraryMockHapToken* mockToken = nullptr;

void MediaLibraryRelatedSystemStateTest::SetUpTestCase()
{
    MEDIA_INFO_LOG("MediaLibraryRelatedSystemStateTest SetUpTestCase");
    MediaLibraryUnitTestUtils::Init();
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);
    vector<string> perms;
    perms.push_back("ohos.permission.GET_NETWORK_INFO");
    // mock  tokenID
    mockToken = new MediaLibraryMockHapToken("com.ohos.medialibrary.medialibrarydata", perms);
    for (auto &perm : perms) {
        MediaLibraryMockTokenUtils::GrantPermissionByTest(IPCSkeleton::GetSelfTokenID(), perm, 0);
    }
}

void MediaLibraryRelatedSystemStateTest::TearDownTestCase()
{
    MEDIA_INFO_LOG("MediaLibraryRelatedSystemStateTest TearDownTestCase");
    if (mockToken != nullptr) {
    delete mockToken;
    mockToken = nullptr;
    }
    MediaLibraryMockTokenUtils::ResetToken();
    SetSelfTokenID(g_shellToken);
    EXPECT_EQ(g_shellToken, IPCSkeleton::GetSelfTokenID());
}

void MediaLibraryRelatedSystemStateTest::SetUp()
{
    MEDIA_INFO_LOG("MediaLibraryRelatedSystemStateTest SetUp");
}

void MediaLibraryRelatedSystemStateTest::TearDown()
{
    MEDIA_INFO_LOG("MediaLibraryRelatedSystemStateTest TearDown");
}

HWTEST_F(MediaLibraryRelatedSystemStateTest, Mlrssm_Wifi_Normal_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mlrssm_Wifi_Normal_01 Start");
    MedialibraryRelatedSystemStateManager::GetInstance()->IsWifiConnectedAtRealTime();
    MedialibraryRelatedSystemStateManager::GetInstance()->IsCellularNetConnectedAtRealTime();
    MedialibraryRelatedSystemStateManager::GetInstance()->IsNetValidatedAtRealTime();
    MedialibraryRelatedSystemStateManager::GetInstance()->SetCellularNetConnected(false);
    MedialibraryRelatedSystemStateManager::GetInstance()->SetWifiConnected(true);
    MedialibraryRelatedSystemStateManager::GetInstance()->IsCellularNetConnected();
    MedialibraryRelatedSystemStateManager::GetInstance()->IsNetAvailableWithUnlimitCondition();
    MedialibraryRelatedSystemStateManager::GetInstance()->IsNetAvailableInOnlyWifiCondition();
    EXPECT_EQ(MedialibraryRelatedSystemStateManager::GetInstance()->IsWifiConnected(), true);
    MEDIA_INFO_LOG("Mlrssm_Wifi_Normal_01 End");
}
} // namespace Media
} // namespace OHOS
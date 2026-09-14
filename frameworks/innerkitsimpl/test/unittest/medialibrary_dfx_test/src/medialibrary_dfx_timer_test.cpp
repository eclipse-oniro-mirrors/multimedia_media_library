/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "medialibrary_dfx_timer_test.h"

#include <string>
#include <unordered_set>

#include "dfx_anco_manager.h"
#include "dfx_cloud_manager.h"
#include "dfx_collector.h"
#include "dfx_const.h"
#include "dfx_database_utils.h"
#include "dfx_manager.h"
#include "dfx_reporter.h"
#include "dfx_system_photo_keys.h"
#include "dfx_utils.h"
#include "dfx_timer.h"
#include "medialibrary_business_code.h"
#include "hisysevent.h"
#include "medialibrary_astc_stat.h"
#include "medialibrary_errno.h"
#include "medialibrary_inotify.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "media_file_utils.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "parameters.h"
#include "userfile_manager_types.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {

static constexpr int32_t SLEEP_THREE_SECONDS = 3;
static std::shared_ptr<MediaLibraryRdbStore> g_rdbStoreTimer;

void MediaLibraryDfxTimerTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("SetUpTestCase");
    MediaLibraryUnitTestUtils::Init();
    g_rdbStoreTimer = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    EXPECT_NE((g_rdbStoreTimer == nullptr), true);
}

void MediaLibraryDfxTimerTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_THREE_SECONDS));
}

void MediaLibraryDfxTimerTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void MediaLibraryDfxTimerTest::TearDown(void) {}

HWTEST_F(MediaLibraryDfxTimerTest, DfxTimer_OperationCodeTimeout_CloneConvertGroup, TestSize.Level1)
{
    MEDIA_INFO_LOG("DfxTimer_OperationCodeTimeout_CloneConvertGroup Start");
    std::vector<MediaLibraryBusinessCode> codes = {
        MediaLibraryBusinessCode::CLONE_TO_ALBUM,
        MediaLibraryBusinessCode::CLONE_TO_DIR,
        MediaLibraryBusinessCode::CLONE_ASSETS_BY_PATH,
        MediaLibraryBusinessCode::ALBUM_CANCEL_CLONE_TASK,
        MediaLibraryBusinessCode::CONVERT_FORMAT,
    };
    for (auto code : codes) {
        uint32_t key = static_cast<uint32_t>(code);
        auto it = DfxTimer::operationCodeTimeoutMap.find(key);
        EXPECT_NE(it, DfxTimer::operationCodeTimeoutMap.end());
        if (it != DfxTimer::operationCodeTimeoutMap.end()) {
            EXPECT_EQ(it->second, 200);
        }
        EXPECT_EQ(DfxTimer::GetOperationCodeTimeout(key), 200);
    }
    MEDIA_INFO_LOG("DfxTimer_OperationCodeTimeout_CloneConvertGroup End");
}

HWTEST_F(MediaLibraryDfxTimerTest, DfxTimer_OperationCodeTimeout_HiddenAttributeGroup, TestSize.Level1)
{
    MEDIA_INFO_LOG("DfxTimer_OperationCodeTimeout_HiddenAttributeGroup Start");
    std::vector<MediaLibraryBusinessCode> codes = {
        MediaLibraryBusinessCode::ASSET_CHANGE_SET_HIDDEN_ATTRIBUTE,
        MediaLibraryBusinessCode::ALBUM_CHANGE_SET_HIDDEN_ATTRIBUTE,
        MediaLibraryBusinessCode::ASSET_CHANGE_SET_DISPLAY_NAME_BY_FILE,
        MediaLibraryBusinessCode::ALBUM_CHANGE_SET_ALBUM_NAME_BY_FILE,
        MediaLibraryBusinessCode::INNER_CREATE_FILE_MANAGER_ASSET,
    };
    for (auto code : codes) {
        uint32_t key = static_cast<uint32_t>(code);
        auto it = DfxTimer::operationCodeTimeoutMap.find(key);
        EXPECT_NE(it, DfxTimer::operationCodeTimeoutMap.end());
        if (it != DfxTimer::operationCodeTimeoutMap.end()) {
            EXPECT_EQ(it->second, 200);
        }
        EXPECT_EQ(DfxTimer::GetOperationCodeTimeout(key), 200);
    }
    MEDIA_INFO_LOG("DfxTimer_OperationCodeTimeout_HiddenAttributeGroup End");
}
} // namespace Media
} // namespace OHOS

/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "medialibrary_rdb_upgrade_test.h"

#include "ability_context_impl.h"
#define private public
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_rdbstore.h"
#undef private
#include "medialibrary_errno.h"
#include "media_log.h"
using namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {

static constexpr int32_t SLEEP_FIVE_SECONDS = 5;

void MediaLibraryRdbUpgradeTest::SetUpTestCase(void) {}

void MediaLibraryRdbUpgradeTest::TearDownTestCase(void)
{
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
    MEDIA_INFO_LOG("MediaLibraryRdbUpgradeTest::TearDownTestCase done");
}

void MediaLibraryRdbUpgradeTest::SetUp(void) {}
void MediaLibraryRdbUpgradeTest::TearDown(void) {}

HWTEST_F(MediaLibraryRdbUpgradeTest, medialib_OnUpGrade_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("start medialib_OnUpGrade_test_001");
    MediaLibraryDataCallBack callback;
    auto stageContext = std::make_shared<AbilityRuntime::ContextImpl>();
    auto abilityContextImpl = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    abilityContextImpl->SetStageContext(stageContext);

    MediaLibraryUnistoreManager::GetInstance().Stop();
    ASSERT_EQ(MediaLibraryUnistoreManager::GetInstance().rdbStorePtr_, nullptr);
    int32_t ret = MediaLibraryUnistoreManager::GetInstance().Init(abilityContextImpl);
    ASSERT_EQ(ret, E_OK);

    std::shared_ptr<MediaLibraryRdbStore> rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(rdbStore, nullptr);
    ret = callback.OnUpgrade(*(rdbStore->GetRaw().get()), RdbUpgradeUtils::GetOldVersion(), 1);
    EXPECT_EQ(ret, E_OK);
    ret = callback.OnUpgrade(*(rdbStore->GetRaw().get()), RdbUpgradeUtils::GetOldVersion(), MEDIA_RDB_VERSION);
    EXPECT_EQ(ret, E_OK);
    MediaLibraryUnistoreManager::GetInstance().Stop();
    MEDIA_INFO_LOG("end medialib_OnUpGrade_test_001");
}
}  // namespace Media
}  // namespace OHOS
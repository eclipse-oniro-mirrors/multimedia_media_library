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

#include "medialibrary_kvstore_manager_test.h"
#include "medialibrary_kvstore_manager.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {

void MediaLibraryKvstoreManagerTest::SetUpTestCase(void) {}
void MediaLibraryKvstoreManagerTest::TearDownTestCase(void) {}
void MediaLibraryKvstoreManagerTest::SetUp(void) {}
void MediaLibraryKvstoreManagerTest::TearDown(void) {}

/*
 * Feature : MediaLibraryKvstoreManagerTest
 * Function : CloseAllKvStore
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryKvstoreManagerTest, MediaLibraryKvstoreManager_Test_001, TestSize.Level1)
{
    shared_ptr<MediaLibraryKvStore> mediaLibraryKvStore = make_shared<MediaLibraryKvStore>();
    ASSERT_NE(mediaLibraryKvStore, nullptr);
    KvStoreValueType valueType = KvStoreValueType::MONTH_ASTC;
    MediaLibraryKvStoreManager::GetInstance().kvStoreMap_.Insert(valueType, mediaLibraryKvStore);
    MediaLibraryKvStoreManager::GetInstance().CloseAllKvStore();
    MediaLibraryKvStoreManager::GetInstance().kvStoreMap_.Clear();
    MediaLibraryKvStoreManager::GetInstance().CloseAllKvStore();

    bool res = MediaLibraryKvStoreManager::GetInstance().CloseKvStore(valueType);
    EXPECT_FALSE(res);

    mediaLibraryKvStore->kvStorePtr_ = nullptr;
    res = MediaLibraryKvStoreManager::GetInstance().CloseKvStore(valueType);
    EXPECT_FALSE(res);
    
    mediaLibraryKvStore = nullptr;
    MediaLibraryKvStoreManager::GetInstance().kvStoreMap_.Insert(valueType, mediaLibraryKvStore);
    res = MediaLibraryKvStoreManager::GetInstance().CloseKvStore(valueType);
    EXPECT_FALSE(res);
}

/*
 * Feature : MediaLibraryKvstoreManagerTest
 * Function : InitMonthAndYearKvStore
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryKvstoreManagerTest, MediaLibraryKvstoreManager_Test_002, TestSize.Level1)
{
    KvStoreRoleType roleType = KvStoreRoleType::VISITOR;
    bool res = MediaLibraryKvStoreManager::GetInstance().InitMonthAndYearKvStore(roleType);
    EXPECT_FALSE(res);
}
} // namespace Media
} // namespace OHOS
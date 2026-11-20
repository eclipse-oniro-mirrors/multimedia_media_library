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
#include <algorithm>
#include "medialibrary_kvstore_test.h"
#include "medialibrary_mocksinglekvstore.h"
#include "medialibrary_errno.h"
#include "media_log.h"
#include "medialibrary_kvstore.h"
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {

const std::string TEST_PATH = "/data/test";
const std::string FIRST_KEY = "000001";
const std::string SECOND_KEY = "000002";
const std::string THIRD_KEY = "000003";
const std::string FORTH_KEY = "000004";
const std::string TEST_MONTH_STOREID = "test_month";

void MedialibraryKvstoreTest::SetUpTestCase(void) {}
void MedialibraryKvstoreTest::TearDownTestCase(void) {}
void MedialibraryKvstoreTest::SetUp() {}
void MedialibraryKvstoreTest::TearDown(void) {}

/*
 * Feature: MediaLibraryHelper
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Init
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_001, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    std::string baseDir = MEDIA_LIBRARY_DB_DIR;
    KvStoreRoleType invalidRole = static_cast<KvStoreRoleType>(KvStoreValueType::YEAR_ASTC_OLD_VERSION);
    int32_t ret = medialibraryKvstore->Init(invalidRole, KvStoreValueType::YEAR_ASTC_OLD_VERSION, baseDir);
    EXPECT_EQ(ret, E_ERR);
}

/*
 * Feature: MediaLibraryHelper
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Insert
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_002, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    medialibraryKvstore->kvStorePtr_ = std::make_shared<MockSingleKvStore>();
    std::string key = "";
    std::vector<uint8_t> value = {};
    int32_t ret = medialibraryKvstore->Insert(key, value);
    EXPECT_NE(ret, -1);
}

/*
 * Feature: MediaLibraryHelper
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Delete
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_003, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    medialibraryKvstore->kvStorePtr_ = nullptr;
    std::string key = "";
    int32_t ret = medialibraryKvstore->Delete(key);
    EXPECT_NE(ret, 1);
    medialibraryKvstore->kvStorePtr_ = std::make_shared<MockSingleKvStore>();
    key = "HANDSUP";
    ret = medialibraryKvstore->Delete(key);
    EXPECT_NE(ret, 1);
}

/*
 * Feature: MediaLibraryHelper
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetCount
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_004, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    medialibraryKvstore->kvStorePtr_ = nullptr;
    std::string key = "HANDSUP";
    int32_t count = 0;
    int32_t ret = medialibraryKvstore->GetCount(key, count);
    EXPECT_NE(ret, 1);
}

/*
 * Feature: MediaLibraryHelper
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Query
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_006, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    medialibraryKvstore->kvStorePtr_ = std::make_shared<MockSingleKvStore>();
    std::string key = "HANDSUP";
    std::vector<uint8_t> value = { 1, 2, 3, 4, 5 };
    medialibraryKvstore->Insert(key, value);
    int32_t ret = medialibraryKvstore->Query(key, value);
    EXPECT_EQ(ret, 0);
}

/*
 * Feature: MediaLibraryHelper
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: BatchQuery
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_007, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    medialibraryKvstore->kvStorePtr_ = std::make_shared<MockSingleKvStore>();
    std::vector<std::string> uriBatch;
    std::vector<std::vector<uint8_t>> dataBatch;
    uriBatch.push_back("000004");
    uriBatch.push_back("000003");
    uriBatch.push_back("000002");
    uriBatch.push_back("000001");
    int errCode = medialibraryKvstore->BatchQuery(uriBatch, dataBatch);
    EXPECT_EQ(errCode, E_OK);
    dataBatch.clear();
    errCode = medialibraryKvstore->BatchQuery(uriBatch, dataBatch);
    EXPECT_EQ(errCode, E_OK);
}

/*
 * Feature: MediaLibraryHelper
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: BatchQuery
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_008, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    medialibraryKvstore->kvStorePtr_ = nullptr;
    std::vector<std::string> uriBatch;
    std::vector<std::vector<uint8_t>> dataBatch;
    int errCode = medialibraryKvstore->BatchQuery(uriBatch, dataBatch);
    EXPECT_NE(errCode, E_OK);
    medialibraryKvstore->kvStorePtr_ = std::make_shared<MockSingleKvStore>();
    errCode = medialibraryKvstore->BatchQuery(uriBatch, dataBatch);
    EXPECT_NE(errCode, E_OK);
}

/*
 * Feature: MediaLibraryHelper
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: RebuildKvStore
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_009, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    medialibraryKvstore->kvStorePtr_ = std::make_shared<MockSingleKvStore>();
    int errCode = medialibraryKvstore->RebuildKvStore(KvStoreValueType::MONTH_ASTC, TEST_PATH);
    EXPECT_NE(errCode, E_OK);
    errCode = medialibraryKvstore->RebuildKvStore(KvStoreValueType::YEAR_ASTC, TEST_PATH);
    EXPECT_NE(errCode, E_OK);
}

/*
 * Feature: MediaLibraryHelper
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: GetKvStoreOption
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_012, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    bool errCode;
    medialibraryKvstore->kvStorePtr_ = std::make_shared<MockSingleKvStore>();
    KvStoreRoleType roletype_test = static_cast<KvStoreRoleType>(KvStoreValueType::YEAR_ASTC_OLD_VERSION);
    OHOS::DistributedKv::Options test_option;
    errCode = medialibraryKvstore->GetKvStoreOption(test_option, roletype_test, TEST_PATH);
    EXPECT_EQ(errCode, false);
}

/*
 * Feature: MediaLibraryHelper
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: BatchInsert
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_013, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    int32_t errCode;
    medialibraryKvstore->kvStorePtr_ = std::make_shared<MockSingleKvStore>();
    std::vector<DistributedKv::Entry> entries;
    errCode = medialibraryKvstore->BatchInsert(entries);
    EXPECT_EQ(errCode, E_ERR);
}

/*
 * Feature: MediaLibraryHelper
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: BatchInsert
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_014, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    int32_t errCode;
    medialibraryKvstore->kvStorePtr_ = nullptr;
    std::vector<DistributedKv::Entry> entries;
    errCode = medialibraryKvstore->BatchInsert(entries);
    EXPECT_EQ(errCode, E_HAS_DB_ERROR);
}

/*
 * Feature: MediaLibraryHelper
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: BatchInsert
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_015, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    int32_t errCode;
    medialibraryKvstore->kvStorePtr_ = std::make_shared<MockSingleKvStore>();
    std::vector<DistributedKv::Entry> entries;
    DistributedKv::Entry entry;
    entry.key = FIRST_KEY;
    std::vector<uint8_t> value;
    value.assign(FIRST_KEY.begin(), FIRST_KEY.end());
    entry.value = value;
    entries.emplace_back(std::move(entry));
    errCode = medialibraryKvstore->BatchInsert(entries);
    EXPECT_EQ(errCode, E_OK);
}

/*
 * Feature: MediaLibraryHelper
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: InitSingleKvstore
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_016, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    int32_t errCode;
    KvStoreRoleType roletype_test = static_cast<KvStoreRoleType>(KvStoreValueType::YEAR_ASTC_OLD_VERSION);
    errCode = medialibraryKvstore->InitSingleKvstore(roletype_test, TEST_MONTH_STOREID, TEST_PATH);
    EXPECT_NE(errCode, E_OK);
}

/*
 * Feature: MediaLibraryHelper
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: InitSingleKvstore
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_017, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    int32_t errCode;
    KvStoreRoleType roletype_test = KvStoreRoleType::VISITOR;
    errCode = medialibraryKvstore->InitSingleKvstore(roletype_test, " ", "/this/is/test");
    EXPECT_NE(errCode, E_OK);
}

/*
 * Feature: MediaLibraryHelper
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: PutAllValueToNewKvStore
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_018, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    std::shared_ptr<MediaLibraryKvStore> test_Kvstore = std::make_shared<MediaLibraryKvStore>();
    medialibraryKvstore->kvStorePtr_ = nullptr;
    int32_t errCode;
    errCode = medialibraryKvstore->PutAllValueToNewKvStore(test_Kvstore);
    EXPECT_NE(errCode, E_OK);
}

/*
 * Feature: MediaLibraryHelper
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: PutAllValueToNewKvStore
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_019, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    std::shared_ptr<MediaLibraryKvStore> test_Kvstore = std::make_shared<MediaLibraryKvStore>();
    medialibraryKvstore->kvStorePtr_ = std::make_shared<MockSingleKvStore>();
    int32_t errCode;
    errCode = medialibraryKvstore->PutAllValueToNewKvStore(test_Kvstore);
    EXPECT_EQ(errCode, E_OK);
}

/*
 * Feature: MediaLibraryHelper
 * Function:
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: PutAllValueToNewKvStore
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_020, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    std::shared_ptr<MediaLibraryKvStore> test_Kvstore = std::make_shared<MediaLibraryKvStore>();
    medialibraryKvstore->kvStorePtr_ = std::make_shared<MockSingleKvStore>();
    int32_t errCode = test_Kvstore->PutAllValueToNewKvStore(medialibraryKvstore);
    EXPECT_NE(errCode, E_OK);
    errCode = test_Kvstore->InitSingleKvstore(KvStoreRoleType::OWNER, TEST_MONTH_STOREID, TEST_PATH);
    EXPECT_EQ(errCode, E_OK);
    errCode = medialibraryKvstore->PutAllValueToNewKvStore(test_Kvstore);
    EXPECT_EQ(errCode, E_OK);
}

/*
 * Feature: MediaLibraryHelper
 * Function: Insert
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Insert with nullptr kvStorePtr_
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_021, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    medialibraryKvstore->kvStorePtr_ = nullptr;
    std::string key = "test_key";
    std::vector<uint8_t> value = {1, 2, 3};
    int32_t ret = medialibraryKvstore->Insert(key, value);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

/*
 * Feature: MediaLibraryHelper
 * Function: DeleteBatch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: DeleteBatch with nullptr kvStorePtr_
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_022, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    medialibraryKvstore->kvStorePtr_ = nullptr;
    std::vector<std::string> batchKeys = {FIRST_KEY, SECOND_KEY};
    int32_t ret = medialibraryKvstore->DeleteBatch(batchKeys);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

/*
 * Feature: MediaLibraryHelper
 * Function: DeleteBatch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: DeleteBatch with valid kvStorePtr_
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_023, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    medialibraryKvstore->kvStorePtr_ = std::make_shared<MockSingleKvStore>();
    std::vector<std::string> batchKeys = {FIRST_KEY, SECOND_KEY, THIRD_KEY};
    int32_t ret = medialibraryKvstore->DeleteBatch(batchKeys);
    EXPECT_EQ(ret, E_OK);
}

/*
 * Feature: MediaLibraryHelper
 * Function: BatchQuery
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: BatchQuery with nullptr kvStorePtr_
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_024, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    medialibraryKvstore->kvStorePtr_ = nullptr;
    std::vector<std::string> batchKeys = {FIRST_KEY, SECOND_KEY};
    std::vector<std::vector<uint8_t>> values;
    int32_t ret = medialibraryKvstore->BatchQuery(batchKeys, values);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

/*
 * Feature: MediaLibraryHelper
 * Function: Close
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Close with nullptr kvStorePtr_
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_025, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    medialibraryKvstore->kvStorePtr_ = nullptr;
    bool ret = medialibraryKvstore->Close();
    EXPECT_TRUE(ret);
}

/*
 * Feature: MediaLibraryHelper
 * Function: Close
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Close with valid kvStorePtr_
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_026, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    medialibraryKvstore->kvStorePtr_ = std::make_shared<MockSingleKvStore>();
    bool ret = medialibraryKvstore->Close();
    EXPECT_FALSE(ret);
}

/*
 * Feature: MediaLibraryHelper
 * Function: InitSingleKvstore
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: InitSingleKvstore with OWNER role
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_027, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    KvStoreRoleType roleType = KvStoreRoleType::OWNER;
    int32_t ret = medialibraryKvstore->InitSingleKvstore(roleType, TEST_MONTH_STOREID, TEST_PATH);
    EXPECT_EQ(ret, E_OK);
}

/*
 * Feature: MediaLibraryHelper
 * Function: InitSingleKvstore
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: InitSingleKvstore with VISITOR role
 */
HWTEST_F(MedialibraryKvstoreTest, medialibrary_kvstore_testlevel_028, TestSize.Level1)
{
    std::shared_ptr<MediaLibraryKvStore> medialibraryKvstore = std::make_shared<MediaLibraryKvStore>();
    ASSERT_NE(medialibraryKvstore, nullptr);
    KvStoreRoleType roleType = KvStoreRoleType::VISITOR;
    int32_t ret = medialibraryKvstore->InitSingleKvstore(roleType, TEST_MONTH_STOREID, TEST_PATH);
    EXPECT_EQ(ret, E_OK);
}
} // namespace Media
} // namespace OHOS
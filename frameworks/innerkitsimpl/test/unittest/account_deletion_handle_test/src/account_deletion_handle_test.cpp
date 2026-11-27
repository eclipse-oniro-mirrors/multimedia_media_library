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

#include "account_deletion_handle_test.h"

#include <chrono>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include "account_deletion_handle.h"
#include "account_deletion_mock.h"
#include "account_error_no.h"
#include "media_file_utils.h"
#include "media_pure_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "preferences.h"
#include "preferences_helper.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {

static constexpr int32_t TEST_ACCOUNT_ID_1 = 1001;
static constexpr int32_t TEST_ACCOUNT_ID_2 = 1002;
static constexpr int32_t TEST_ACCOUNT_ID_3 = 1003;
static constexpr int32_t TEST_USER_ID = 100;
static constexpr const char *TEST_PREF_DIR = "/data/storage/el2/base/preferences/";
static constexpr const char *TEST_ACCOUNT_PREFS_PATH =
    "/data/storage/el2/base/preferences/medialibrary_multi_account_config.xml";

// /storage/cloud/files/{accountId}/ - per-account cloud files root (sandbox
// view has no userId layer; matches CleanLocalFiles' GetDir(CLOUD_FILES)).
static std::string TestCloudDir(int32_t accountId)
{
    return "/storage/cloud/files/" + std::to_string(accountId) + "/";
}

// direct handle to the multi-account config preference file (JSON corruption cases)
static std::shared_ptr<NativePreferences::Preferences> GetTestAccountPrefs()
{
    int32_t errCode = 0;
    return NativePreferences::PreferencesHelper::GetPreferences(TEST_ACCOUNT_PREFS_PATH, errCode);
}

// whether the "deleted_accounts" preference marker currently contains accountId
static bool DeletedMarkerContains(int32_t accountId)
{
    auto deletedIds = AccountManager::GetInstance().GetDeletedAccountIds();
    for (auto id : deletedIds) {
        if (id == accountId) {
            return true;
        }
    }
    return false;
}

// clear all markers used by this suite
static void ClearDeletedMarkers()
{
    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_1);
    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_2);
    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_3);
}

// Wait until the async cleanup task for accountId finishes (it erases the
// deletingAccounts_ slot as its last step). Keeps dispatch-style cases
// self-contained: no leaked task can interleave with later cases.

// Production exposes no completion event for the slot erase, so this stays
// the state-query + bounded-timeout polling helper (review-accepted
// fallback); the subscribe drain uses the stub-side CV instead.
static void WaitDeletionTaskDone(int32_t accountId, int32_t timeoutMs = 3000)
{
    constexpr int32_t sleepMs = 10;
    for (int32_t waited = 0; waited < timeoutMs; waited += sleepMs) {
        bool done = false;
        {
            std::lock_guard<std::mutex> lock(AccountDeletionHandle::GetInstance().queueMutex_);
            done = AccountDeletionHandle::GetInstance().deletingAccounts_.count(accountId) == 0;
        }
        if (done) {
            return;
        }
        usleep(sleepMs * 1000); // 10ms
    }
}

// The recovery entry latches via CAS; suite cases reset it so each run starts fresh
static void ResetProcessingFlag()
{
    AccountDeletionHandle::GetInstance().processingStarted_.store(false);
}

void AccountDeletionHandleTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("SetUpTestCase");
    MediaLibraryUnitTestUtils::Init();
}

void AccountDeletionHandleTest::TearDownTestCase(void) {}

void AccountDeletionHandleTest::SetUp()
{
    // Reset link-time stubs: every case starts from the default success scenario
    AccountDeletionMock::Reset();
}

void AccountDeletionHandleTest::TearDown() {}

// ============================================================================
// ComputeRecovery tests (pure logic, no I/O)
// ============================================================================

/**
 * @tc.name: computeRecovery_crashInProgress_test_001
 * @tc.desc: mark done, cleanup unfinished
 * @tc.type: FUNC
 * @tc.step: known={1001,1002}, deleted={1002}, existing={1001}; call ComputeRecovery
 * @tc.expect: toClean={1002}; toMark/toUnmark empty
 */
HWTEST_F(AccountDeletionHandleTest, computeRecovery_crashInProgress_test_001, TestSize.Level0)
{
    std::vector<int32_t> knownIds = {TEST_ACCOUNT_ID_1, TEST_ACCOUNT_ID_2};
    std::vector<int32_t> deletedIds = {TEST_ACCOUNT_ID_2};
    std::vector<int32_t> existingIds = {TEST_ACCOUNT_ID_1};

    auto result = AccountDeletionHandle::ComputeRecovery(knownIds, deletedIds, existingIds);

    EXPECT_EQ(result.toClean.size(), 1u);
    EXPECT_EQ(result.toClean.count(TEST_ACCOUNT_ID_2), 1u);
    EXPECT_TRUE(result.toMark.empty());
    EXPECT_TRUE(result.toUnmark.empty());
}

/**
 * @tc.name: computeRecovery_eventLost_test_001
 * @tc.desc: crash before MarkAccountDeleted
 * @tc.type: FUNC
 * @tc.step: known={1001,1002}, deleted={}, existing={1001}
 * @tc.expect: toClean={1002}, toMark={1002}
 */
HWTEST_F(AccountDeletionHandleTest, computeRecovery_eventLost_test_001, TestSize.Level0)
{
    std::vector<int32_t> knownIds = {TEST_ACCOUNT_ID_1, TEST_ACCOUNT_ID_2};
    std::vector<int32_t> deletedIds = {};
    std::vector<int32_t> existingIds = {TEST_ACCOUNT_ID_1};

    auto result = AccountDeletionHandle::ComputeRecovery(knownIds, deletedIds, existingIds);

    EXPECT_EQ(result.toClean.size(), 1u);
    EXPECT_EQ(result.toClean.count(TEST_ACCOUNT_ID_2), 1u);
    ASSERT_EQ(result.toMark.size(), 1u);
    EXPECT_EQ(result.toMark[0], TEST_ACCOUNT_ID_2);
    EXPECT_TRUE(result.toUnmark.empty());
}

/**
 * @tc.name: computeRecovery_accountCameBack_test_001
 * @tc.desc: account re-login after crash
 * @tc.type: FUNC
 * @tc.step: known={1001,1002}, deleted={1002}, existing={1001,1002}
 * @tc.expect: toUnmark={1002} only
 */
HWTEST_F(AccountDeletionHandleTest, computeRecovery_accountCameBack_test_001, TestSize.Level0)
{
    std::vector<int32_t> knownIds = {TEST_ACCOUNT_ID_1, TEST_ACCOUNT_ID_2};
    std::vector<int32_t> deletedIds = {TEST_ACCOUNT_ID_2};
    std::vector<int32_t> existingIds = {TEST_ACCOUNT_ID_1, TEST_ACCOUNT_ID_2};

    auto result = AccountDeletionHandle::ComputeRecovery(knownIds, deletedIds, existingIds);

    EXPECT_TRUE(result.toClean.empty());
    EXPECT_TRUE(result.toMark.empty());
    ASSERT_EQ(result.toUnmark.size(), 1u);
    EXPECT_EQ(result.toUnmark[0], TEST_ACCOUNT_ID_2);
}

/**
 * @tc.name: computeRecovery_noDeletion_test_001
 * @tc.desc: nothing to delete
 * @tc.type: FUNC
 * @tc.step: known={1001,1002}, deleted={}, existing=all
 * @tc.expect: all empty
 */
HWTEST_F(AccountDeletionHandleTest, computeRecovery_noDeletion_test_001, TestSize.Level0)
{
    std::vector<int32_t> knownIds = {TEST_ACCOUNT_ID_1, TEST_ACCOUNT_ID_2};
    std::vector<int32_t> deletedIds = {};
    std::vector<int32_t> existingIds = {TEST_ACCOUNT_ID_1, TEST_ACCOUNT_ID_2};

    auto result = AccountDeletionHandle::ComputeRecovery(knownIds, deletedIds, existingIds);

    EXPECT_TRUE(result.toClean.empty());
    EXPECT_TRUE(result.toMark.empty());
    EXPECT_TRUE(result.toUnmark.empty());
}

/**
 * @tc.name: computeRecovery_deletionCompleted_test_001
 * @tc.desc: deletion already finished
 * @tc.type: FUNC
 * @tc.step: known={1001,1002,1003}, deleted={}, existing={1001,1003}
 * @tc.expect: toClean={1002}, toMark={1002}
 */
HWTEST_F(AccountDeletionHandleTest, computeRecovery_deletionCompleted_test_001, TestSize.Level0)
{
    std::vector<int32_t> knownIds = {TEST_ACCOUNT_ID_1, TEST_ACCOUNT_ID_2, TEST_ACCOUNT_ID_3};
    std::vector<int32_t> deletedIds = {};
    std::vector<int32_t> existingIds = {TEST_ACCOUNT_ID_1, TEST_ACCOUNT_ID_3};

    auto result = AccountDeletionHandle::ComputeRecovery(knownIds, deletedIds, existingIds);

    EXPECT_EQ(result.toClean.size(), 1u);
    EXPECT_EQ(result.toClean.count(TEST_ACCOUNT_ID_2), 1u);
    ASSERT_EQ(result.toMark.size(), 1u);
    EXPECT_EQ(result.toMark[0], TEST_ACCOUNT_ID_2);
}

/**
 * @tc.name: computeRecovery_emptyLists_test_001
 * @tc.desc: empty-list boundary
 * @tc.type: FUNC
 * @tc.step: all three lists empty
 * @tc.expect: all empty
 */
HWTEST_F(AccountDeletionHandleTest, computeRecovery_emptyLists_test_001, TestSize.Level0)
{
    std::vector<int32_t> knownIds = {};
    std::vector<int32_t> deletedIds = {};
    std::vector<int32_t> existingIds = {};

    auto result = AccountDeletionHandle::ComputeRecovery(knownIds, deletedIds, existingIds);

    EXPECT_TRUE(result.toClean.empty());
    EXPECT_TRUE(result.toMark.empty());
    EXPECT_TRUE(result.toUnmark.empty());
}

/**
 * @tc.name: computeRecovery_multiAccount_test_001
 * @tc.desc: multiple accounts deleted at once
 * @tc.type: FUNC
 * @tc.step: known=all, deleted={1002,1003}, existing={1001}
 * @tc.expect: toClean={1002,1003}
 */
HWTEST_F(AccountDeletionHandleTest, computeRecovery_multiAccount_test_001, TestSize.Level0)
{
    std::vector<int32_t> knownIds = {TEST_ACCOUNT_ID_1, TEST_ACCOUNT_ID_2, TEST_ACCOUNT_ID_3};
    std::vector<int32_t> deletedIds = {TEST_ACCOUNT_ID_2, TEST_ACCOUNT_ID_3};
    std::vector<int32_t> existingIds = {TEST_ACCOUNT_ID_1};

    auto result = AccountDeletionHandle::ComputeRecovery(knownIds, deletedIds, existingIds);

    EXPECT_EQ(result.toClean.size(), 2u);
    EXPECT_EQ(result.toClean.count(TEST_ACCOUNT_ID_2), 1u);
    EXPECT_EQ(result.toClean.count(TEST_ACCOUNT_ID_3), 1u);
    EXPECT_TRUE(result.toMark.empty());
    EXPECT_TRUE(result.toUnmark.empty());
}

/**
 * @tc.name: computeRecovery_partialCameBack_test_001
 * @tc.desc: partial comeback + partial unfinished
 * @tc.type: FUNC
 * @tc.step: known=all, deleted={1002,1003}, existing={1001,1002}
 * @tc.expect: toClean={1003}, toUnmark={1002}
 */
HWTEST_F(AccountDeletionHandleTest, computeRecovery_partialCameBack_test_001, TestSize.Level0)
{
    std::vector<int32_t> knownIds = {TEST_ACCOUNT_ID_1, TEST_ACCOUNT_ID_2, TEST_ACCOUNT_ID_3};
    std::vector<int32_t> deletedIds = {TEST_ACCOUNT_ID_2, TEST_ACCOUNT_ID_3};
    std::vector<int32_t> existingIds = {TEST_ACCOUNT_ID_1, TEST_ACCOUNT_ID_2};

    auto result = AccountDeletionHandle::ComputeRecovery(knownIds, deletedIds, existingIds);

    EXPECT_EQ(result.toClean.size(), 1u);
    EXPECT_EQ(result.toClean.count(TEST_ACCOUNT_ID_3), 1u);
    EXPECT_TRUE(result.toMark.empty());
    ASSERT_EQ(result.toUnmark.size(), 1u);
    EXPECT_EQ(result.toUnmark[0], TEST_ACCOUNT_ID_2);
}

// ============================================================================
// preference I/O tests (MarkAccountDeleted / UnmarkAccountDeleted / GetDeletedAccountIds)
// ============================================================================

/**
 * @tc.name: markAccountDeleted_twoAccounts_test_001
 * @tc.desc: mark two accounts and verify
 * @tc.type: FUNC
 * @tc.step: clear; MarkAccountDeleted 1001+1002
 * @tc.expect: list contains both
 */
HWTEST_F(AccountDeletionHandleTest, markAccountDeleted_twoAccounts_test_001, TestSize.Level0)
{
    ClearDeletedMarkers();

    AccountManager::GetInstance().MarkAccountDeleted(TEST_ACCOUNT_ID_1);
    AccountManager::GetInstance().MarkAccountDeleted(TEST_ACCOUNT_ID_2);

    EXPECT_EQ(AccountManager::GetInstance().GetDeletedAccountIds().size(), 2u);
    EXPECT_TRUE(DeletedMarkerContains(TEST_ACCOUNT_ID_1));
    EXPECT_TRUE(DeletedMarkerContains(TEST_ACCOUNT_ID_2));

    ClearDeletedMarkers();
}

/**
 * @tc.name: markAccountDeleted_idempotent_test_001
 * @tc.desc: mark same account twice
 * @tc.type: FUNC
 * @tc.step: MarkAccountDeleted 1001 twice
 * @tc.expect: single entry kept
 */
HWTEST_F(AccountDeletionHandleTest, markAccountDeleted_idempotent_test_001, TestSize.Level0)
{
    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_1);

    AccountManager::GetInstance().MarkAccountDeleted(TEST_ACCOUNT_ID_1);
    AccountManager::GetInstance().MarkAccountDeleted(TEST_ACCOUNT_ID_1);

    auto deletedIds = AccountManager::GetInstance().GetDeletedAccountIds();
    int32_t count = 0;
    for (auto id : deletedIds) {
        if (id == TEST_ACCOUNT_ID_1) {
            count++;
        }
    }
    EXPECT_EQ(count, 1);
}

/**
 * @tc.name: unmarkAccountDeleted_oneOfTwo_test_001
 * @tc.desc: unmark one of two
 * @tc.type: FUNC
 * @tc.step: mark both; unmark 1001
 * @tc.expect: only 1002 remains
 */
HWTEST_F(AccountDeletionHandleTest, unmarkAccountDeleted_oneOfTwo_test_001, TestSize.Level0)
{
    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_1);
    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_2);

    AccountManager::GetInstance().MarkAccountDeleted(TEST_ACCOUNT_ID_1);
    AccountManager::GetInstance().MarkAccountDeleted(TEST_ACCOUNT_ID_2);

    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_1);

    auto deletedIds = AccountManager::GetInstance().GetDeletedAccountIds();
    ASSERT_EQ(deletedIds.size(), 1u);
    EXPECT_EQ(deletedIds[0], TEST_ACCOUNT_ID_2);
}

/**
 * @tc.name: unmarkAccountDeleted_nonexistent_test_001
 * @tc.desc: unmark non-existent marker
 * @tc.type: FUNC
 * @tc.step: mark 1001; unmark 1003
 * @tc.expect: no crash; 1001 unchanged
 */
HWTEST_F(AccountDeletionHandleTest, unmarkAccountDeleted_nonexistent_test_001, TestSize.Level0)
{
    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_1);
    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_2);

    AccountManager::GetInstance().MarkAccountDeleted(TEST_ACCOUNT_ID_1);
    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_3);

    auto deletedIds = AccountManager::GetInstance().GetDeletedAccountIds();
    ASSERT_EQ(deletedIds.size(), 1u);
    EXPECT_EQ(deletedIds[0], TEST_ACCOUNT_ID_1);
}

/**
 * @tc.name: getDeletedAccountIds_empty_test_001
 * @tc.desc: no markers
 * @tc.type: FUNC
 * @tc.step: clear all
 * @tc.expect: empty list
 */
HWTEST_F(AccountDeletionHandleTest, getDeletedAccountIds_empty_test_001, TestSize.Level0)
{
    ClearDeletedMarkers();

    auto deletedIds = AccountManager::GetInstance().GetDeletedAccountIds();
    EXPECT_TRUE(deletedIds.empty());
}

/**
 * @tc.name: markAccountDeleted_thenUnmarkAll_test_001
 * @tc.desc: unmark all leaves empty
 * @tc.type: FUNC
 * @tc.step: mark all three; unmark one by one
 * @tc.expect: empty list
 */
HWTEST_F(AccountDeletionHandleTest, markAccountDeleted_thenUnmarkAll_test_001, TestSize.Level0)
{
    AccountManager::GetInstance().MarkAccountDeleted(TEST_ACCOUNT_ID_1);
    AccountManager::GetInstance().MarkAccountDeleted(TEST_ACCOUNT_ID_2);
    AccountManager::GetInstance().MarkAccountDeleted(TEST_ACCOUNT_ID_3);

    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_1);
    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_2);
    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_3);

    auto deletedIds = AccountManager::GetInstance().GetDeletedAccountIds();
    EXPECT_TRUE(deletedIds.empty());
}

// ============================================================================
// preference JSON boundary tests
// ============================================================================

/**
 * @tc.name: getDeletedAccountIds_nonNumberJson_test_001
 * @tc.desc: JSON with non-number elements
 * @tc.type: FUNC
 * @tc.step: write [1001,"abc",null,1002]
 * @tc.expect: only {1001,1002} kept
 */
HWTEST_F(AccountDeletionHandleTest, getDeletedAccountIds_nonNumberJson_test_001, TestSize.Level0)
{
    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_1);
    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_2);

    auto prefs = GetTestAccountPrefs();
    ASSERT_NE(prefs, nullptr);
    prefs->PutString("deleted_accounts", R"([1001, "abc", null, 1002])");
    prefs->FlushSync();

    auto deletedIds = AccountManager::GetInstance().GetDeletedAccountIds();
    EXPECT_EQ(deletedIds.size(), 2u);
    EXPECT_TRUE(DeletedMarkerContains(TEST_ACCOUNT_ID_1));
    EXPECT_TRUE(DeletedMarkerContains(TEST_ACCOUNT_ID_2));

    prefs->PutString("deleted_accounts", "[]");
    prefs->FlushSync();
}

/**
 * @tc.name: markAccountDeleted_corruptedJson_test_001
 * @tc.desc: corrupted JSON recovered by MarkAccountDeleted
 * @tc.type: FUNC
 * @tc.step: write "not_a_json"; MarkAccountDeleted 1001
 * @tc.expect: list recovered as [1001]
 */
HWTEST_F(AccountDeletionHandleTest, markAccountDeleted_corruptedJson_test_001, TestSize.Level0)
{
    auto prefs = GetTestAccountPrefs();
    ASSERT_NE(prefs, nullptr);
    prefs->PutString("deleted_accounts", "not_a_json");
    prefs->FlushSync();

    AccountManager::GetInstance().MarkAccountDeleted(TEST_ACCOUNT_ID_1);

    auto deletedIds = AccountManager::GetInstance().GetDeletedAccountIds();
    ASSERT_EQ(deletedIds.size(), 1u);
    EXPECT_EQ(deletedIds[0], TEST_ACCOUNT_ID_1);

    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_1);
}

/**
 * @tc.name: unmarkAccountDeleted_corruptedJson_test_001
 * @tc.desc: corrupted JSON reset by UnmarkAccountDeleted
 * @tc.type: FUNC
 * @tc.step: write "{{invalid"; UnmarkAccountDeleted
 * @tc.expect: E_OK; content reset to []
 */
HWTEST_F(AccountDeletionHandleTest, unmarkAccountDeleted_corruptedJson_test_001, TestSize.Level0)
{
    auto prefs = GetTestAccountPrefs();
    ASSERT_NE(prefs, nullptr);
    prefs->PutString("deleted_accounts", "{{invalid");
    prefs->FlushSync();

    int32_t ret = AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_1);
    EXPECT_EQ(ret, E_OK);

    auto deletedIds = AccountManager::GetInstance().GetDeletedAccountIds();
    EXPECT_TRUE(deletedIds.empty());

    prefs->PutString("deleted_accounts", "[]");
    prefs->FlushSync();
}

// ============================================================================
// EnqueueAccountDeletion idempotency tests
// ============================================================================

/**
 * @tc.name: enqueueAccountDeletion_idempotent_test_001
 * @tc.desc: repeated calls return idempotently
 * @tc.type: FUNC
 * @tc.step: call EnqueueAccountDeletion twice
 * @tc.expect: both E_OK; task drained
 */
HWTEST_F(AccountDeletionHandleTest, enqueueAccountDeletion_idempotent_test_001, TestSize.Level0)
{
    int32_t ret1 = AccountDeletionHandle::GetInstance().EnqueueAccountDeletion(TEST_ACCOUNT_ID_1, TEST_USER_ID);
    EXPECT_EQ(ret1, E_OK);

    int32_t ret2 = AccountDeletionHandle::GetInstance().EnqueueAccountDeletion(TEST_ACCOUNT_ID_1, TEST_USER_ID);
    EXPECT_EQ(ret2, E_OK);

    // drain the queued task so no cleanup work leaks into later cases
    WaitDeletionTaskDone(TEST_ACCOUNT_ID_1);
    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_1);
}

/**
 * @tc.name: enqueueAccountDeletion_checkSetState_test_001
 * @tc.desc: dedup slot taken after enqueue
 * @tc.type: FUNC
 * @tc.step: erase slot; call once
 * @tc.expect: slot contains accountId; drained
 */
HWTEST_F(AccountDeletionHandleTest, enqueueAccountDeletion_checkSetState_test_001, TestSize.Level0)
{
    {
        std::lock_guard<std::mutex> lock(AccountDeletionHandle::GetInstance().queueMutex_);
        AccountDeletionHandle::GetInstance().deletingAccounts_.erase(TEST_ACCOUNT_ID_3);
    }

    int32_t ret = AccountDeletionHandle::GetInstance().EnqueueAccountDeletion(TEST_ACCOUNT_ID_3, TEST_USER_ID);
    EXPECT_EQ(ret, E_OK);

    {
        std::lock_guard<std::mutex> lock(AccountDeletionHandle::GetInstance().queueMutex_);
        EXPECT_EQ(AccountDeletionHandle::GetInstance().deletingAccounts_.count(TEST_ACCOUNT_ID_3), 1u);
    }

    // drain the queued task (it erases the slot itself at the end)
    WaitDeletionTaskDone(TEST_ACCOUNT_ID_3);
    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_3);
}

// ============================================================================
// CleanRdbData / CleanLocalFiles / CleanAccountPreferences tests
// ============================================================================

/**
 * @tc.name: CleanRdbData_success_test_001
 * @tc.desc: absent db dir (idempotent)
 * @tc.type: FUNC
 * @tc.step: CleanRdbData on absent dir
 * @tc.expect: E_OK; no failedFiles
 */
HWTEST_F(AccountDeletionHandleTest, CleanRdbData_success_test_001, TestSize.Level0)
{
    AccountDeletionHandle::CleanContext ctx(TEST_ACCOUNT_ID_1, TEST_USER_ID);
    int32_t ret = AccountDeletionHandle::GetInstance().CleanRdbData(ctx);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(ctx.failedFiles.empty());
}

/**
 * @tc.name: cleanLocalFiles_success_test_001
 * @tc.desc: absent asset dirs (idempotent)
 * @tc.type: FUNC
 * @tc.step: CleanLocalFiles on absent dirs
 * @tc.expect: E_OK; no failedFiles
 */
HWTEST_F(AccountDeletionHandleTest, cleanLocalFiles_success_test_001, TestSize.Level0)
{
    AccountDeletionHandle::CleanContext ctx(TEST_ACCOUNT_ID_1, TEST_USER_ID);
    int32_t ret = AccountDeletionHandle::GetInstance().CleanLocalFiles(ctx);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(ctx.failedFiles.empty());
}

/**
 * @tc.name: cleanAccountPreferences_success_test_001
 * @tc.desc: Delete success passes through
 * @tc.type: FUNC
 * @tc.step: stub Delete=E_OK; call
 * @tc.expect: E_OK; Delete called once
 */
HWTEST_F(AccountDeletionHandleTest, cleanAccountPreferences_success_test_001, TestSize.Level0)
{
    AccountDeletionMock::SetDeleteResult(E_OK);
    AccountDeletionHandle::CleanContext ctx(TEST_ACCOUNT_ID_1, TEST_USER_ID);
    int32_t ret = AccountDeletionHandle::GetInstance().CleanAccountPreferences(ctx);
    EXPECT_EQ(ret, E_OK);
    EXPECT_GE(AccountDeletionMock::GetDeleteCallCount(), 1);
}

/**
 * @tc.name: cleanKvData_closesAndDeletesStores_test_001
 * @tc.desc: Step 2 deletes the account's stores outright (no rebuild); per-account
 * @tc.desc: CloseKvStore not landed yet, Close step skipped
 * @tc.type: FUNC
 * @tc.step: default stubs (E_OK); call CleanKvData
 * @tc.expect: E_OK; DeleteAccountKvStores once; no failedFiles
 */
HWTEST_F(AccountDeletionHandleTest, cleanKvData_closesAndDeletesStores_test_001, TestSize.Level0)
{
    AccountDeletionHandle::CleanContext ctx(TEST_ACCOUNT_ID_1, TEST_USER_ID);
    int32_t ret = AccountDeletionHandle::GetInstance().CleanKvData(ctx);

    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(AccountDeletionMock::GetKvDeleteStoresCount(), 1);  // both stores, one call
    EXPECT_TRUE(ctx.failedFiles.empty());
}

/**
 * @tc.name: cleanKvData_deleteFailAggregated_test_001
 * @tc.desc: physical delete failure aggregates (non-fatal, rerun-safe)
 * @tc.type: FUNC
 * @tc.step: stub delete=E_FAIL (in-flight holder); rdb dir absent;
     call ExecuteDeletionSteps
 * @tc.expect: E_FAIL; step2 recorded, step4 NOT (absent dir succeeds)
 */
HWTEST_F(AccountDeletionHandleTest, cleanKvData_deleteFailAggregated_test_001, TestSize.Level0)
{
    AccountDeletionMock::SetKvDeleteResult(E_FAIL);
    AccountDeletionHandle::CleanContext ctx(TEST_ACCOUNT_ID_1, TEST_USER_ID);

    int32_t ret = AccountDeletionHandle::ExecuteDeletionSteps(ctx);

    EXPECT_EQ(ret, E_FAIL);
    bool step2Recorded = false;
    bool step4Recorded = false;
    for (const auto &step : ctx.failedSteps) {
        if (step.first == "Step 2: CleanKvData") {
            step2Recorded = true;
        }
        if (step.first == "Step 4: CleanRdbData") {
            step4Recorded = true;
        }
    }
    EXPECT_TRUE(step2Recorded);
    EXPECT_FALSE(step4Recorded);
}

/**
 * @tc.name: cleanXmlFiles_reserved_test_001
 * @tc.desc: Step 3 reserved skeleton
 * @tc.type: FUNC
 * @tc.step: call CleanXmlFiles
 * @tc.expect: E_OK
 */
HWTEST_F(AccountDeletionHandleTest, cleanXmlFiles_reserved_test_001, TestSize.Level0)
{
    AccountDeletionHandle::CleanContext ctx(TEST_ACCOUNT_ID_1, TEST_USER_ID);
    int32_t ret = AccountDeletionHandle::GetInstance().CleanXmlFiles(ctx);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: cleanXmlFiles_deletesMatching_test_001
 * @tc.desc: deletes exactly the _<accountId>.xml files; others survive
 * @tc.type: FUNC
 * @tc.step: create prefs dir + 3 prefs files; CleanXmlFiles
 * @tc.expect: E_OK; account1 file gone; account2/config survive
 * @tc.cleanup: DeletePreferences(account2/config)
 */
HWTEST_F(AccountDeletionHandleTest, cleanXmlFiles_deletesMatching_test_001, TestSize.Level1)
{
    ASSERT_TRUE(MediaPureFileUtils::CreateDirectory(TEST_PREF_DIR));
    std::string account1 = std::string(TEST_PREF_DIR) + "fake_xml_1001.xml";
    std::string account2 = std::string(TEST_PREF_DIR) + "fake_xml_1002.xml";
    std::string config = std::string(TEST_PREF_DIR) + "medialibrary_multi_account_config.xml";
    int32_t errCode = 0;
    auto prefs1 = NativePreferences::PreferencesHelper::GetPreferences(account1, errCode);
    ASSERT_NE(prefs1, nullptr);
    prefs1->PutString("fake", "1001");
    (void)prefs1->FlushSync();
    auto prefs2 = NativePreferences::PreferencesHelper::GetPreferences(account2, errCode);
    ASSERT_NE(prefs2, nullptr);
    prefs2->PutString("fake", "1002");
    (void)prefs2->FlushSync();
    auto prefsConfig = NativePreferences::PreferencesHelper::GetPreferences(config, errCode);
    ASSERT_NE(prefsConfig, nullptr);
    prefsConfig->PutString("fake", "config");
    (void)prefsConfig->FlushSync();
    ASSERT_EQ(prefs1->GetString("fake", ""), "1001");
    ASSERT_EQ(prefs2->GetString("fake", ""), "1002");
    ASSERT_EQ(prefsConfig->GetString("fake", ""), "config");
    // on-disk pre-existence, otherwise the gone/survive checks below are vacuous
    ASSERT_EQ(access(account1.c_str(), F_OK), 0);
    ASSERT_EQ(access(account2.c_str(), F_OK), 0);
    ASSERT_EQ(access(config.c_str(), F_OK), 0);
    AccountDeletionHandle::CleanContext ctx(TEST_ACCOUNT_ID_1, TEST_USER_ID);
    int32_t ret = AccountDeletionHandle::GetInstance().CleanXmlFiles(ctx);
    EXPECT_EQ(ret, E_OK);
    EXPECT_NE(access(account1.c_str(), F_OK), 0);
    EXPECT_EQ(access(account2.c_str(), F_OK), 0);
    EXPECT_EQ(access(config.c_str(), F_OK), 0);
    NativePreferences::PreferencesHelper::DeletePreferences(account2);
    NativePreferences::PreferencesHelper::DeletePreferences(config);
}

// ============================================================================
// CleanAccountData integration tests
// ============================================================================

/**
 * @tc.name: cleanAccountData_fullFlow_test_001
 * @tc.desc: full Step 0-6 on absent dirs
 * @tc.type: FUNC
 * @tc.step: call CleanAccountData
 * @tc.expect: E_OK
 */
HWTEST_F(AccountDeletionHandleTest, cleanAccountData_fullFlow_test_001, TestSize.Level0)
{
    AccountDeletionHandle::CleanContext ctx(TEST_ACCOUNT_ID_2, TEST_USER_ID);
    ctx.startTime = std::chrono::steady_clock::now();
    int32_t ret = AccountDeletionHandle::CleanAccountData(ctx);
    EXPECT_EQ(ret, E_OK);
}

// ============================================================================
// ProcessDeletionTask tests (private, -fno-access-control)
// ============================================================================

/**
 * @tc.name: processDeletionTask_success_test_001
 * @tc.desc: success unmarks and erases
 * @tc.type: FUNC
 * @tc.step: pre-insert slot+marker; call
 * @tc.expect: slot erased; marker removed
 */
HWTEST_F(AccountDeletionHandleTest, processDeletionTask_success_test_001, TestSize.Level0)
{
    AccountDeletionHandle::DeletionTask task { TEST_ACCOUNT_ID_2, TEST_USER_ID };
    AccountDeletionHandle::GetInstance().deletingAccounts_.insert(TEST_ACCOUNT_ID_2);
    AccountManager::GetInstance().MarkAccountDeleted(TEST_ACCOUNT_ID_2);

    AccountDeletionHandle::GetInstance().ProcessDeletionTask(task);

    EXPECT_EQ(AccountDeletionHandle::GetInstance().deletingAccounts_.count(TEST_ACCOUNT_ID_2), 0u);
    EXPECT_FALSE(DeletedMarkerContains(TEST_ACCOUNT_ID_2));
}

/**
 * @tc.name: processDeletionTask_idempotentErase_test_001
 * @tc.desc: erase of absent element is safe
 * @tc.type: FUNC
 * @tc.step: call without pre-insert
 * @tc.expect: no crash; slot absent
 */
HWTEST_F(AccountDeletionHandleTest, processDeletionTask_idempotentErase_test_001, TestSize.Level0)
{
    AccountDeletionHandle::DeletionTask task { TEST_ACCOUNT_ID_3, TEST_USER_ID };
    AccountDeletionHandle::GetInstance().ProcessDeletionTask(task);
    EXPECT_EQ(AccountDeletionHandle::GetInstance().deletingAccounts_.count(TEST_ACCOUNT_ID_3), 0u);
}

// ============================================================================
// RecoverAccountDeletions (end-of-init recovery entry) tests
// ============================================================================

/**
 * @tc.name: recoverAccountDeletions_noop_test_001
 * @tc.desc: no-op when nothing to recover
 * @tc.type: FUNC
 * @tc.step: defaults; cleared markers; run entry
 * @tc.expect: E_OK; no marker applied
 */
HWTEST_F(AccountDeletionHandleTest, recoverAccountDeletions_noop_test_001, TestSize.Level0)
{
    ClearDeletedMarkers();
    ResetProcessingFlag();

    int32_t ret = AccountDeletionHandle::GetInstance().RecoverAccountDeletions();

    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(DeletedMarkerContains(TEST_ACCOUNT_ID_1));
    EXPECT_FALSE(DeletedMarkerContains(TEST_ACCOUNT_ID_2));
    EXPECT_FALSE(DeletedMarkerContains(TEST_ACCOUNT_ID_3));
}

// ============================================================================
// SubscribeAccountDeletion tests
// ============================================================================

/**
 * @tc.name: subscribeAccountDeletion_callTwice_test_001
 * @tc.desc: callback not rebuilt on 2nd call
 * @tc.type: FUNC
 * @tc.step: call twice
 * @tc.expect: same result; same callback pointer
 */
HWTEST_F(AccountDeletionHandleTest, subscribeAccountDeletion_callTwice_test_001, TestSize.Level0)
{
    int32_t ret1 = AccountDeletionHandle::GetInstance().SubscribeAccountDeletion();
    auto callback1 = AccountDeletionHandle::GetInstance().deletionCallback_;

    int32_t ret2 = AccountDeletionHandle::GetInstance().SubscribeAccountDeletion();
    auto callback2 = AccountDeletionHandle::GetInstance().deletionCallback_;

    // both calls return the same value (pass/fail depends on env; must not crash)
    EXPECT_EQ(ret1, ret2);
    // callback pointer unchanged (not rebuilt)
    EXPECT_EQ(callback1, callback2);
}

/**
 * @tc.name: subscribeAccountDeletion_nonDriverScreenSkip_test_001
 * @tc.desc: non-driver screen: subscribe entry returns E_OK without subscribing
 * @tc.type: FUNC
 * @tc.step: stub IsMultiAccountEnabled=false; call entry once
 * @tc.expect: E_OK; subscribe count unchanged
 */
HWTEST_F(AccountDeletionHandleTest, subscribeAccountDeletion_nonDriverScreenSkip_test_001, TestSize.Level0)
{
    AccountDeletionMock::SetMultiAccountEnabled(false);
    int32_t countBefore = AccountDeletionMock::GetSubscribeCallCount();

    int32_t ret = AccountDeletionHandle::GetInstance().SubscribeAccountDeletion();

    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(AccountDeletionMock::GetSubscribeCallCount(), countBefore);
}

// ============================================================================
// GetDeletedAccountIds JSON boundary extras
// ============================================================================

/**
 * @tc.name: getDeletedAccountIds_emptyString_test_001
 * @tc.desc: empty string value
 * @tc.type: FUNC
 * @tc.step: write ""
 * @tc.expect: empty list
 */
HWTEST_F(AccountDeletionHandleTest, getDeletedAccountIds_emptyString_test_001, TestSize.Level0)
{
    auto prefs = GetTestAccountPrefs();
    ASSERT_NE(prefs, nullptr);
    prefs->PutString("deleted_accounts", "");
    prefs->FlushSync();

    auto deletedIds = AccountManager::GetInstance().GetDeletedAccountIds();
    EXPECT_TRUE(deletedIds.empty());

    prefs->PutString("deleted_accounts", "[]");
    prefs->FlushSync();
}

/**
 * @tc.name: getDeletedAccountIds_nonArrayJson_test_001
 * @tc.desc: valid JSON but not array
 * @tc.type: FUNC
 * @tc.step: write {"key":"value"}
 * @tc.expect: empty list
 */
HWTEST_F(AccountDeletionHandleTest, getDeletedAccountIds_nonArrayJson_test_001, TestSize.Level0)
{
    auto prefs = GetTestAccountPrefs();
    ASSERT_NE(prefs, nullptr);
    prefs->PutString("deleted_accounts", R"({"key": "value"})");
    prefs->FlushSync();

    auto deletedIds = AccountManager::GetInstance().GetDeletedAccountIds();
    EXPECT_TRUE(deletedIds.empty());

    prefs->PutString("deleted_accounts", "[]");
    prefs->FlushSync();
}

// ============================================================================
// CleanAccountData failure-path tests
// ============================================================================

/**
 * @tc.name: cleanAccountData_step4Fail_test_001
 * @tc.desc: Step 4 failure propagates
 * @tc.type: FUNC
 * @tc.step: stub CloseAccountDb=E_ERR; CleanAccountData
 * @tc.expect: E_ERR; Step 5 skipped
 */
HWTEST_F(AccountDeletionHandleTest, cleanAccountData_step4Fail_test_001, TestSize.Level0)
{
    AccountDeletionMock::SetCloseAccountDbResult(E_ERR);

    AccountDeletionHandle::CleanContext ctx(TEST_ACCOUNT_ID_3, TEST_USER_ID);
    ctx.startTime = std::chrono::steady_clock::now();
    int32_t ret = AccountDeletionHandle::CleanAccountData(ctx);
    EXPECT_EQ(ret, E_ERR);
}

// NOTE: CloseAccountDb itself is stubbed in this exe (mock/src) because
// its real implementation lives in the flag-gated service lib; real-path
// coverage (rdbStoreMap_ erase / connection close) belongs to the service
// lib's own tests or integration tests. CleanRdbData's flag-on path (stubbed
// CloseAccountDb + real idempotent dir delete) is covered by
// CleanRdbData_success_test_001 below
// ============================================================================
// Prepare tests (Step 0)
// ============================================================================

/**
 * @tc.name: prepare_validUserId_test_001
 * @tc.desc: userId provided - no query
 * @tc.type: FUNC
 * @tc.step: ctx userId=100; call Prepare
 * @tc.expect: E_OK; userId unchanged
 */
HWTEST_F(AccountDeletionHandleTest, prepare_validUserId_test_001, TestSize.Level0)
{
    AccountDeletionHandle::CleanContext ctx(TEST_ACCOUNT_ID_1, TEST_USER_ID);
    int32_t ret = AccountDeletionHandle::GetInstance().Prepare(ctx);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(ctx.userId, TEST_USER_ID);
}

/**
 * @tc.name: prepare_userIdZero_test_001
 * @tc.desc: userId 0 is untrusted (driver screen is fixed at 100) - re-queried and corrected
 * @tc.type: FUNC
 * @tc.step: stub fg query=100; ctx userId=0; call Prepare
 * @tc.expect: E_OK; corrected to 100 (never kept as-is for asset paths)
 */
HWTEST_F(AccountDeletionHandleTest, prepare_userIdZero_test_001, TestSize.Level0)
{
    AccountDeletionMock::SetForegroundOsAccountLocalId(ERR_OK, TEST_USER_ID);

    AccountDeletionHandle::CleanContext ctx(TEST_ACCOUNT_ID_1, 0);
    int32_t ret = AccountDeletionHandle::GetInstance().Prepare(ctx);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(ctx.userId, TEST_USER_ID);
}

/**
 * @tc.name: prepare_invalidUserId_test_001
 * @tc.desc: negative userId triggers query
 * @tc.type: FUNC
 * @tc.step: stub fg query=100; ctx -1
 * @tc.expect: E_OK; userId filled 100
 */
HWTEST_F(AccountDeletionHandleTest, prepare_invalidUserId_test_001, TestSize.Level0)
{
    AccountDeletionMock::SetForegroundOsAccountLocalId(ERR_OK, TEST_USER_ID);

    AccountDeletionHandle::CleanContext ctx(TEST_ACCOUNT_ID_1, -1);
    int32_t ret = AccountDeletionHandle::GetInstance().Prepare(ctx);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(ctx.userId, TEST_USER_ID);
}

// ============================================================================
// ComputeRecovery additional branches (pure logic)
// ============================================================================

/**
 * @tc.name: computeRecovery_markerOrphan_test_001
 * @tc.desc: marker account absent from knownIds (line-1 only)
 * @tc.type: FUNC
 * @tc.step: known={1001}, deleted={1003}, existing={1001}
 * @tc.expect: toClean={1003} via defense line 1 only
 */
HWTEST_F(AccountDeletionHandleTest, computeRecovery_markerOrphan_test_001, TestSize.Level0)
{
    std::vector<int32_t> knownIds = {TEST_ACCOUNT_ID_1};
    std::vector<int32_t> deletedIds = {TEST_ACCOUNT_ID_3};
    std::vector<int32_t> existingIds = {TEST_ACCOUNT_ID_1};

    auto result = AccountDeletionHandle::ComputeRecovery(knownIds, deletedIds, existingIds);

    EXPECT_EQ(result.toClean.size(), 1u);
    EXPECT_EQ(result.toClean.count(TEST_ACCOUNT_ID_3), 1u);
    EXPECT_TRUE(result.toMark.empty());
    EXPECT_TRUE(result.toUnmark.empty());
}

/**
 * @tc.name: computeRecovery_newAccountAppeared_test_001
 * @tc.desc: brand-new account in existingIds only
 * @tc.type: FUNC
 * @tc.step: known={1001}, deleted={}, existing={1001,1002}
 * @tc.expect: all empty
 */
HWTEST_F(AccountDeletionHandleTest, computeRecovery_newAccountAppeared_test_001, TestSize.Level0)
{
    std::vector<int32_t> knownIds = {TEST_ACCOUNT_ID_1};
    std::vector<int32_t> deletedIds = {};
    std::vector<int32_t> existingIds = {TEST_ACCOUNT_ID_1, TEST_ACCOUNT_ID_2};

    auto result = AccountDeletionHandle::ComputeRecovery(knownIds, deletedIds, existingIds);

    EXPECT_TRUE(result.toClean.empty());
    EXPECT_TRUE(result.toMark.empty());
    EXPECT_TRUE(result.toUnmark.empty());
}

// ============================================================================
// MarkAccountDeleted / UnmarkAccountDeleted return-value contracts
// ============================================================================

/**
 * @tc.name: markAccountDeleted_returnValues_test_001
 * @tc.desc: return-value contracts incl. double-mark
 * @tc.type: FUNC
 * @tc.step: mark twice, verify, unmark
 * @tc.expect: all E_OK; single entry kept
 */
HWTEST_F(AccountDeletionHandleTest, markAccountDeleted_returnValues_test_001, TestSize.Level0)
{
    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_1);

    int32_t ret1 = AccountManager::GetInstance().MarkAccountDeleted(TEST_ACCOUNT_ID_1);
    EXPECT_EQ(ret1, E_OK);

    int32_t ret2 = AccountManager::GetInstance().MarkAccountDeleted(TEST_ACCOUNT_ID_1);
    EXPECT_EQ(ret2, E_OK); // already-marked branch still reports success

    auto deletedIds = AccountManager::GetInstance().GetDeletedAccountIds();
    int32_t count = 0;
    for (auto id : deletedIds) {
        if (id == TEST_ACCOUNT_ID_1) {
            count++;
        }
    }
    EXPECT_EQ(count, 1);

    int32_t ret3 = AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_1);
    EXPECT_EQ(ret3, E_OK);
    EXPECT_TRUE(AccountManager::GetInstance().GetDeletedAccountIds().empty());
}

// ============================================================================
// CleanLocalFiles real-directory tests (sandbox path: no userId layer)
// ============================================================================

/**
 * @tc.name: cleanLocalFiles_realDirs_test_001
 * @tc.desc: real per-account cloud dir deleted; sibling content survives
 * @tc.type: FUNC
 * @tc.step: create cloud files/{accountId}/Photo + a sibling dir under files/; CleanLocalFiles
 * @tc.expect: E_OK; account dir gone; sibling untouched
 * @tc.cleanup: remove leftovers
 */
HWTEST_F(AccountDeletionHandleTest, cleanLocalFiles_realDirs_test_001, TestSize.Level0)
{
    std::string filesRoot = "/storage/cloud/files/";
    std::string accountDir = TestCloudDir(TEST_ACCOUNT_ID_2);
    std::string siblingDir = filesRoot + "reserved/";
    ASSERT_TRUE(MediaPureFileUtils::CreateDirectory(accountDir + "Photo"));
    ASSERT_TRUE(MediaPureFileUtils::CreateDirectory(siblingDir));
    std::string filePath = accountDir + "Photo/test.jpg";
    FILE *fp = fopen(filePath.c_str(), "w");
    ASSERT_NE(fp, nullptr);
    fputs("test", fp);
    fclose(fp);

    AccountDeletionHandle::CleanContext ctx(TEST_ACCOUNT_ID_2, TEST_USER_ID);
    int32_t ret = AccountDeletionHandle::CleanLocalFiles(ctx);
    EXPECT_EQ(ret, E_OK);
    // deletion is scoped to the account dir under files/
    EXPECT_NE(access(accountDir.c_str(), F_OK), 0);
    EXPECT_EQ(access(siblingDir.c_str(), F_OK), 0);

    // remove only dirs this test created; never the shared files/ root
    if (access(accountDir.c_str(), F_OK) == 0) {
        MediaFileUtils::DeleteFileOrFolder(accountDir, false);
    }
    if (access(siblingDir.c_str(), F_OK) == 0) {
        MediaFileUtils::DeleteFileOrFolder(siblingDir, false);
    }
}

/**
 * @tc.name: processDeletionTask_failKeepsMarker_test_001
 * @tc.desc: failure keeps marker, releases slot
 * @tc.type: FUNC
 * @tc.step: stub E_ERR; pre-insert+mark; call
 * @tc.expect: slot erased; marker kept
 * @tc.cleanup: clear marker
 */
HWTEST_F(AccountDeletionHandleTest, processDeletionTask_failKeepsMarker_test_001, TestSize.Level0)
{
    AccountDeletionMock::SetCloseAccountDbResult(E_ERR);

    AccountDeletionHandle::DeletionTask task { TEST_ACCOUNT_ID_3, TEST_USER_ID };
    AccountDeletionHandle::GetInstance().deletingAccounts_.insert(TEST_ACCOUNT_ID_3);
    AccountManager::GetInstance().MarkAccountDeleted(TEST_ACCOUNT_ID_3);

    AccountDeletionHandle::GetInstance().ProcessDeletionTask(task);

    // set slot released on both success and failure paths
    EXPECT_EQ(AccountDeletionHandle::GetInstance().deletingAccounts_.count(TEST_ACCOUNT_ID_3), 0u);
    // marker kept: "deleted_accounts" still contains the accountId
    EXPECT_TRUE(DeletedMarkerContains(TEST_ACCOUNT_ID_3));

    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_3);
}

// ============================================================================
// RecoverAccountDeletions guards (driver-screen gate + CAS flag)
// ============================================================================

/**
 * @tc.name: recoverAccountDeletions_nonDriverScreenSkip_test_001
 * @tc.desc: non-driver screen: entry returns E_OK without scheduling
 * @tc.type: FUNC
 * @tc.step: stub IsMultiAccountEnabled=false; reset flag; call entry once
 * @tc.expect: E_OK; processingStarted_ stays false; nothing applied
 */
HWTEST_F(AccountDeletionHandleTest, recoverAccountDeletions_nonDriverScreenSkip_test_001, TestSize.Level0)
{
    AccountDeletionMock::SetMultiAccountEnabled(false);
    ResetProcessingFlag();
    ClearDeletedMarkers();

    int32_t ret = AccountDeletionHandle::GetInstance().RecoverAccountDeletions();

    EXPECT_EQ(ret, E_OK);
    // gate runs BEFORE the CAS guard: the flag must stay untouched, so a
    // later driver-screen invocation can still schedule normally
    EXPECT_FALSE(AccountDeletionHandle::GetInstance().processingStarted_.load());
}

/**
 * @tc.name: recoverAccountDeletions_reentryGuard_test_001
 * @tc.desc: CAS guard - second call skips
 * @tc.type: FUNC
 * @tc.step: call RecoverAccountDeletions twice
 * @tc.expect: both E_OK; flag set (recovery is synchronous now)
 */
HWTEST_F(AccountDeletionHandleTest, recoverAccountDeletions_reentryGuard_test_001, TestSize.Level0)
{
    ResetProcessingFlag();
    ClearDeletedMarkers();

    int32_t ret1 = AccountDeletionHandle::GetInstance().RecoverAccountDeletions();
    EXPECT_EQ(ret1, E_OK);
    EXPECT_TRUE(AccountDeletionHandle::GetInstance().processingStarted_.load());

    int32_t ret2 = AccountDeletionHandle::GetInstance().RecoverAccountDeletions();
    EXPECT_EQ(ret2, E_OK); // already-started skip branch
    EXPECT_TRUE(AccountDeletionHandle::GetInstance().processingStarted_.load());

    // restore for other tests / later init-like invocations
    ResetProcessingFlag();
}

// ============================================================================
// OnSubProfileChanged tests (event-callback entry, real event path)
// ============================================================================

/**
 * @tc.name: onSubProfileChanged_deletedEvent_test_001
 * @tc.desc: DELETED event dispatches cleanup
 * @tc.type: FUNC
 * @tc.step: fire DELETED(subProfileId=1002)
 * @tc.expect: slot+marker taken; drained
 * @tc.cleanup: drain, clear
 */
HWTEST_F(AccountDeletionHandleTest, onSubProfileChanged_deletedEvent_test_001, TestSize.Level0)
{
    {
        std::lock_guard<std::mutex> lock(AccountDeletionHandle::GetInstance().queueMutex_);
        AccountDeletionHandle::GetInstance().deletingAccounts_.erase(TEST_ACCOUNT_ID_2);
    }
    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_2);

    AccountDeletionCallback callback;
    AccountSA::SubProfileEventData eventData;
    eventData.type_ = AccountSA::OsAccountSubProfileEventType::DELETED;
    eventData.osAccountId_ = TEST_USER_ID;
    eventData.subProfileId_ = TEST_ACCOUNT_ID_2;
    callback.OnSubProfileChanged(eventData);

    // enqueue-side effects are deterministic (happen before the drain thread
    // processes the task): slot taken + marker persisted
    bool inSet = false;
    {
        std::lock_guard<std::mutex> lock(AccountDeletionHandle::GetInstance().queueMutex_);
        inSet = AccountDeletionHandle::GetInstance().deletingAccounts_.count(TEST_ACCOUNT_ID_2) > 0;
    }
    EXPECT_TRUE(inSet);
    EXPECT_TRUE(DeletedMarkerContains(TEST_ACCOUNT_ID_2));

    // cleanup: drain the queued task (it erases the slot itself), then
    // defensively clear the marker (already removed by the task on success)
    WaitDeletionTaskDone(TEST_ACCOUNT_ID_2);
    {
        std::lock_guard<std::mutex> lock(AccountDeletionHandle::GetInstance().queueMutex_);
        AccountDeletionHandle::GetInstance().deletingAccounts_.erase(TEST_ACCOUNT_ID_2);
    }
    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_2);
}

/**
 * @tc.name: onSubProfileChanged_nonDeletedIgnored_test_001
 * @tc.desc: non-DELETED event ignored
 * @tc.type: FUNC
 * @tc.step: fire CREATED(1003)
 * @tc.expect: no slot, no marker
 */
HWTEST_F(AccountDeletionHandleTest, onSubProfileChanged_nonDeletedIgnored_test_001, TestSize.Level0)
{
    {
        std::lock_guard<std::mutex> lock(AccountDeletionHandle::GetInstance().queueMutex_);
        AccountDeletionHandle::GetInstance().deletingAccounts_.erase(TEST_ACCOUNT_ID_3);
    }
    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_3);

    AccountDeletionCallback callback;
    AccountSA::SubProfileEventData eventData;
    eventData.type_ = AccountSA::OsAccountSubProfileEventType::CREATED;
    eventData.osAccountId_ = TEST_USER_ID;
    eventData.subProfileId_ = TEST_ACCOUNT_ID_3;
    callback.OnSubProfileChanged(eventData);

    bool inSet = false;
    {
        std::lock_guard<std::mutex> lock(AccountDeletionHandle::GetInstance().queueMutex_);
        inSet = AccountDeletionHandle::GetInstance().deletingAccounts_.count(TEST_ACCOUNT_ID_3) > 0;
    }
    EXPECT_FALSE(inSet);
    EXPECT_FALSE(DeletedMarkerContains(TEST_ACCOUNT_ID_3));
}

// ============================================================================
// Link-time stub tests (mock/include shadows os_account + AccountManager)
// ============================================================================

/**
 * @tc.name: subscribeAccountDeletion_successAndFail_test_001
 * @tc.desc: both subscribe branches
 * @tc.type: FUNC
 * @tc.step: stub OK then E_FAIL
 * @tc.expect: E_OK then E_FAIL; both counted
 */
HWTEST_F(AccountDeletionHandleTest, subscribeAccountDeletion_successAndFail_test_001, TestSize.Level0)
{
    AccountDeletionMock::SetSubscribeResult(ERR_OK);
    int32_t ret1 = AccountDeletionHandle::GetInstance().SubscribeAccountDeletion();
    EXPECT_EQ(ret1, E_OK);

    AccountDeletionMock::SetSubscribeResult(E_FAIL);
    int32_t ret2 = AccountDeletionHandle::GetInstance().SubscribeAccountDeletion();
    EXPECT_EQ(ret2, E_FAIL);

    EXPECT_GE(AccountDeletionMock::GetSubscribeCallCount(), 2);
}

/**
 * @tc.name: prepare_queryFails_test_001
 * @tc.desc: foreground query fails
 * @tc.type: FUNC
 * @tc.step: stub query fail; ctx -1
 * @tc.expect: E_FAIL
 */
HWTEST_F(AccountDeletionHandleTest, prepare_queryFails_test_001, TestSize.Level0)
{
    AccountDeletionMock::SetForegroundOsAccountLocalId(E_FAIL, -1);

    AccountDeletionHandle::CleanContext ctx(TEST_ACCOUNT_ID_1, -1);
    int32_t ret = AccountDeletionHandle::GetInstance().Prepare(ctx);
    EXPECT_EQ(ret, E_FAIL);
}

/**
 * @tc.name: CleanRdbData_closeAccountDbFail_test_001
 * @tc.desc: CloseAccountDb E_ERR fast-exits before dir delete
 * @tc.type: FUNC
 * @tc.step: stub CloseAccountDb=E_ERR; Internal
 * @tc.expect: E_ERR; step4 recorded; failedFiles empty
 */
HWTEST_F(AccountDeletionHandleTest, CleanRdbData_closeAccountDbFail_test_001, TestSize.Level0)
{
    AccountDeletionMock::SetCloseAccountDbResult(E_ERR);
    AccountDeletionHandle::CleanContext ctx(TEST_ACCOUNT_ID_1, TEST_USER_ID);

    int32_t ret = AccountDeletionHandle::ExecuteDeletionSteps(ctx);

    EXPECT_EQ(ret, E_ERR);
    ASSERT_EQ(ctx.failedSteps.size(), 1u);
    EXPECT_EQ(ctx.failedSteps[0].first, "Step 4: CleanRdbData");
    EXPECT_EQ(ctx.failedSteps[0].second, E_ERR);
    // fast exit: DeleteFileOrFolder never ran, so no file failures recorded
    EXPECT_TRUE(ctx.failedFiles.empty());
}

/**
 * @tc.name: cleanAccountData_step0FailEarlyExit_test_001
 * @tc.desc: Step 0 failure exits before Step 1
 * @tc.type: FUNC
 * @tc.step: stub query fail; real cloud dir; Internal
 * @tc.expect: E_FAIL; step0 only; dir untouched
 * @tc.cleanup: remove the dir
 */
HWTEST_F(AccountDeletionHandleTest, cleanAccountData_step0FailEarlyExit_test_001, TestSize.Level0)
{
    std::string cloudDir = TestCloudDir(TEST_ACCOUNT_ID_1);
    ASSERT_TRUE(MediaPureFileUtils::CreateDirectory(cloudDir));
    std::string filePath = cloudDir + "test.jpg";
    FILE *fp = fopen(filePath.c_str(), "w");
    ASSERT_NE(fp, nullptr);
    fputs("test", fp);
    fclose(fp);

    AccountDeletionMock::SetForegroundOsAccountLocalId(E_FAIL, -1);
    AccountDeletionHandle::CleanContext ctx(TEST_ACCOUNT_ID_1, -1);
    int32_t ret = AccountDeletionHandle::ExecuteDeletionSteps(ctx);

    EXPECT_EQ(ret, E_FAIL);
    ASSERT_EQ(ctx.failedSteps.size(), 1u);
    EXPECT_EQ(ctx.failedSteps[0].first, "Step 0: Prepare");
    // hard proof of the early exit: Step 1 would have deleted this dir
    EXPECT_EQ(access(cloudDir.c_str(), F_OK), 0);

    remove(filePath.c_str());
    rmdir(cloudDir.c_str());
}

/**
 * @tc.name: cleanAccountPreferences_notFoundMappedOk_test_001
 * @tc.desc: NOT_FOUND maps to E_OK
 * @tc.type: FUNC
 * @tc.step: stub Delete=E_ACCOUNT_NOT_FOUND
 * @tc.expect: E_OK; Delete called
 */
HWTEST_F(AccountDeletionHandleTest, cleanAccountPreferences_notFoundMappedOk_test_001, TestSize.Level0)
{
    AccountDeletionMock::SetDeleteResult(E_ACCOUNT_NOT_FOUND);

    AccountDeletionHandle::CleanContext ctx(TEST_ACCOUNT_ID_1, TEST_USER_ID);
    int32_t ret = AccountDeletionHandle::GetInstance().CleanAccountPreferences(ctx);
    EXPECT_EQ(ret, E_OK);
    EXPECT_GE(AccountDeletionMock::GetDeleteCallCount(), 1);
}

/**
 * @tc.name: cleanAccountPreferences_realFailAggregated_test_001
 * @tc.desc: real Step 5 failure aggregated
 * @tc.type: FUNC
 * @tc.step: stub Delete=E_FAIL; Internal
 * @tc.expect: E_FAIL; step5 recorded, step4 not
 */
HWTEST_F(AccountDeletionHandleTest, cleanAccountPreferences_realFailAggregated_test_001, TestSize.Level0)
{
    AccountDeletionMock::SetDeleteResult(E_FAIL);
    AccountDeletionHandle::CleanContext ctx(TEST_ACCOUNT_ID_2, TEST_USER_ID);
    int32_t ret = AccountDeletionHandle::ExecuteDeletionSteps(ctx);

    EXPECT_EQ(ret, E_FAIL);
    bool step5Recorded = false;
    bool step4Recorded = false;
    for (const auto &step : ctx.failedSteps) {
        if (step.first == "Step 5: CleanAccountPreferences") {
            step5Recorded = true;
        }
        if (step.first == "Step 4: CleanRdbData") {
            step4Recorded = true;
        }
    }
    EXPECT_TRUE(step5Recorded);
    EXPECT_FALSE(step4Recorded);
}

/**
 * @tc.name: recoverAccountDeletions_foregroundFail_test_001
 * @tc.desc: foreground query fail aborts recovery
 * @tc.type: FUNC
 * @tc.step: mark 1001; stub query fail; run entry
 * @tc.expect: E_FAIL; marker untouched
 */
HWTEST_F(AccountDeletionHandleTest, recoverAccountDeletions_foregroundFail_test_001, TestSize.Level0)
{
    ResetProcessingFlag();
    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_1);
    AccountManager::GetInstance().MarkAccountDeleted(TEST_ACCOUNT_ID_1);
    AccountDeletionMock::SetForegroundOsAccountLocalId(E_FAIL, -1);

    int32_t ret = AccountDeletionHandle::GetInstance().RecoverAccountDeletions();

    // early return - the marker is untouched (no unmark/mark/clean applied)
    EXPECT_EQ(ret, E_FAIL);
    EXPECT_TRUE(DeletedMarkerContains(TEST_ACCOUNT_ID_1));

    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_1);
}

/**
 * @tc.name: recoverAccountDeletions_rejectsUnexpectedUserId_test_001
 * @tc.desc: recovery rejects a foreground user other than the fixed 100
 * @tc.type: FUNC
 * @tc.step: stub fg=(ERR_OK, 0) - framework-valid but never the driver screen
 * @tc.expect: E_FAIL; recovery aborted at the user check - marker untouched
 */
HWTEST_F(AccountDeletionHandleTest, recoverAccountDeletions_rejectsUnexpectedUserId_test_001, TestSize.Level0)
{
    ResetProcessingFlag();
    ClearDeletedMarkers();
    AccountManager::GetInstance().MarkAccountDeleted(TEST_ACCOUNT_ID_1);
    AccountDeletionMock::SetForegroundOsAccountLocalId(ERR_OK, 0);
    AccountDeletionMock::SetLocalIdForSubProfile(ERR_OK, TEST_USER_ID);

    int32_t ret = AccountDeletionHandle::GetInstance().RecoverAccountDeletions();

    // aborted before apply - toUnmark not applied
    EXPECT_EQ(ret, E_FAIL);
    EXPECT_TRUE(DeletedMarkerContains(TEST_ACCOUNT_ID_1));
}

/**
 * @tc.name: recoverAccountDeletions_subProfileQueryFail_test_001
 * @tc.desc: sub-profile query fail skips recovery
 * @tc.type: FUNC
 * @tc.step: mark 1001; stub ids fail; run entry
 * @tc.expect: E_FAIL; marker untouched
 */
HWTEST_F(AccountDeletionHandleTest, recoverAccountDeletions_subProfileQueryFail_test_001, TestSize.Level0)
{
    ResetProcessingFlag();
    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_1);
    AccountManager::GetInstance().MarkAccountDeleted(TEST_ACCOUNT_ID_1);
    AccountDeletionMock::SetForegroundOsAccountLocalId(ERR_OK, TEST_USER_ID);
    AccountDeletionMock::SetAllAccounts({TEST_ACCOUNT_ID_1});  // knownIds non-empty so the probe loop runs
    AccountDeletionMock::SetLocalIdForSubProfile(E_FAIL, -1);

    int32_t ret = AccountDeletionHandle::GetInstance().RecoverAccountDeletions();

    // recovery skipped - the marker is untouched
    EXPECT_EQ(ret, E_FAIL);
    EXPECT_TRUE(DeletedMarkerContains(TEST_ACCOUNT_ID_1));

    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_1);
}

/**
 * @tc.name: recoverAccountDeletions_marksAndUnmarks_test_001
 * @tc.desc: toMark/toUnmark applied; slot keeps marker
 * @tc.type: FUNC
 * @tc.step: known={1,2}, existing={1}, stale=1; slot pre-taken for 2
 * @tc.expect: 1 unmarked; 2 marked and kept
 * @tc.cleanup: release slot, clear marker
 */
HWTEST_F(AccountDeletionHandleTest, recoverAccountDeletions_marksAndUnmarks_test_001, TestSize.Level0)
{
    ResetProcessingFlag();
    {
        std::lock_guard<std::mutex> lock(AccountDeletionHandle::GetInstance().queueMutex_);
        AccountDeletionHandle::GetInstance().deletingAccounts_.insert(TEST_ACCOUNT_ID_2);
    }
    ClearDeletedMarkers();
    AccountManager::GetInstance().MarkAccountDeleted(TEST_ACCOUNT_ID_1); // stale marker (came back)
    AccountDeletionMock::SetAllAccounts({TEST_ACCOUNT_ID_1, TEST_ACCOUNT_ID_2});
    // 1 exists, 2 deleted: per-account probe results
    AccountDeletionMock::SetLocalIdForSubProfileForAccount(TEST_ACCOUNT_ID_1, ERR_OK, TEST_USER_ID);
    AccountDeletionMock::SetLocalIdForSubProfileForAccount(TEST_ACCOUNT_ID_2, ERR_OS_ACCOUNT_SUBPROFILE_NOT_FOUND, -1);

    int32_t ret = AccountDeletionHandle::GetInstance().RecoverAccountDeletions();

    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(DeletedMarkerContains(TEST_ACCOUNT_ID_1)); // toUnmark applied: stale marker removed
    EXPECT_TRUE(DeletedMarkerContains(TEST_ACCOUNT_ID_2));  // toMark applied: event-lost account marked

    {
        std::lock_guard<std::mutex> lock(AccountDeletionHandle::GetInstance().queueMutex_);
        AccountDeletionHandle::GetInstance().deletingAccounts_.erase(TEST_ACCOUNT_ID_2);
    }
    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_2);
}

/**
 * @tc.name: recoverAccountDeletions_fullRecovery_test_001
 * @tc.desc: toClean dispatch observed via idempotent skip
 * @tc.type: FUNC
 * @tc.step: known={1,2}, existing={1}, deleted=2; slot pre-taken
 * @tc.expect: 2 stays marked
 * @tc.cleanup: release slot, clear marker
 */
HWTEST_F(AccountDeletionHandleTest, recoverAccountDeletions_fullRecovery_test_001, TestSize.Level0)
{
    ResetProcessingFlag();
    {
        std::lock_guard<std::mutex> lock(AccountDeletionHandle::GetInstance().queueMutex_);
        AccountDeletionHandle::GetInstance().deletingAccounts_.insert(TEST_ACCOUNT_ID_2);
    }
    ClearDeletedMarkers();
    AccountManager::GetInstance().MarkAccountDeleted(TEST_ACCOUNT_ID_2); // mid-deletion crash
    AccountDeletionMock::SetAllAccounts({TEST_ACCOUNT_ID_1, TEST_ACCOUNT_ID_2});
    // 1 exists, 2 deleted: per-account probe results
    AccountDeletionMock::SetLocalIdForSubProfileForAccount(TEST_ACCOUNT_ID_1, ERR_OK, TEST_USER_ID);
    AccountDeletionMock::SetLocalIdForSubProfileForAccount(TEST_ACCOUNT_ID_2, ERR_OS_ACCOUNT_SUBPROFILE_NOT_FOUND, -1);

    int32_t ret = AccountDeletionHandle::GetInstance().RecoverAccountDeletions();

    EXPECT_EQ(ret, E_OK);
    // toClean dispatch was requested (idempotent skip via pre-taken slot) and the marker stays
    EXPECT_TRUE(DeletedMarkerContains(TEST_ACCOUNT_ID_2));

    {
        std::lock_guard<std::mutex> lock(AccountDeletionHandle::GetInstance().queueMutex_);
        AccountDeletionHandle::GetInstance().deletingAccounts_.erase(TEST_ACCOUNT_ID_2);
    }
    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_2);
}

/**
 * @tc.name: drainDeletionQueue_claimsAndDrains_test_001
 * @tc.desc: task entry claims ownership and drains the queue synchronously
 * @tc.type: FUNC
 * @tc.step: pre-insert slot+marker+task, drainRunning_=false; call entry directly
 * @tc.expect: queue drained; slot erased; marker removed; flag released
 */
HWTEST_F(AccountDeletionHandleTest, drainDeletionQueue_claimsAndDrains_test_001, TestSize.Level0)
{
    AccountDeletionHandle::DeletionTask task { TEST_ACCOUNT_ID_2, TEST_USER_ID };
    AccountDeletionHandle::GetInstance().deletingAccounts_.insert(TEST_ACCOUNT_ID_2);
    AccountManager::GetInstance().MarkAccountDeleted(TEST_ACCOUNT_ID_2);
    {
        std::lock_guard<std::mutex> lock(AccountDeletionHandle::GetInstance().queueMutex_);
        AccountDeletionHandle::GetInstance().drainRunning_ = false;
        AccountDeletionHandle::GetInstance().deletionQueue_.push(task);
    }

    // direct call simulates the AsyncWorker picking the task; also the
    // self-heal path after a dropped drain task (flag unclaimed + queue loaded)
    AccountDeletionHandle::DrainDeletionQueue(nullptr);

    EXPECT_EQ(AccountDeletionHandle::GetInstance().deletingAccounts_.count(TEST_ACCOUNT_ID_2), 0u);
    EXPECT_FALSE(DeletedMarkerContains(TEST_ACCOUNT_ID_2));
    {
        std::lock_guard<std::mutex> lock(AccountDeletionHandle::GetInstance().queueMutex_);
        EXPECT_TRUE(AccountDeletionHandle::GetInstance().deletionQueue_.empty());
        EXPECT_FALSE(AccountDeletionHandle::GetInstance().drainRunning_);
    }
}

/**
 * @tc.name: drainDeletionQueue_noOpWhenActive_test_001
 * @tc.desc: entry no-ops (re-arm) when a drain loop is already active
 * @tc.type: FUNC
 * @tc.step: pre-insert slot+marker+task, drainRunning_=true; call entry directly
 * @tc.expect: returns without consuming; queue/slot/flag untouched
 */
HWTEST_F(AccountDeletionHandleTest, drainDeletionQueue_noOpWhenActive_test_001, TestSize.Level0)
{
    AccountDeletionHandle::DeletionTask task { TEST_ACCOUNT_ID_3, TEST_USER_ID };
    AccountDeletionHandle::GetInstance().deletingAccounts_.insert(TEST_ACCOUNT_ID_3);
    AccountManager::GetInstance().MarkAccountDeleted(TEST_ACCOUNT_ID_3);
    {
        std::lock_guard<std::mutex> lock(AccountDeletionHandle::GetInstance().queueMutex_);
        AccountDeletionHandle::GetInstance().drainRunning_ = true;
        AccountDeletionHandle::GetInstance().deletionQueue_.push(task);
    }

    AccountDeletionHandle::DrainDeletionQueue(nullptr);

    // no-op re-arm: nothing consumed while an (simulated) active drain owns the loop
    {
        std::lock_guard<std::mutex> lock(AccountDeletionHandle::GetInstance().queueMutex_);
        EXPECT_EQ(AccountDeletionHandle::GetInstance().deletionQueue_.size(), 1u);
        EXPECT_TRUE(AccountDeletionHandle::GetInstance().drainRunning_);
    }
    EXPECT_EQ(AccountDeletionHandle::GetInstance().deletingAccounts_.count(TEST_ACCOUNT_ID_3), 1u);
    EXPECT_TRUE(DeletedMarkerContains(TEST_ACCOUNT_ID_3));

    // cleanup: release the simulated ownership, drop the queued task, clear state
    {
        std::lock_guard<std::mutex> lock(AccountDeletionHandle::GetInstance().queueMutex_);
        AccountDeletionHandle::GetInstance().drainRunning_ = false;
        while (!AccountDeletionHandle::GetInstance().deletionQueue_.empty()) {
            AccountDeletionHandle::GetInstance().deletionQueue_.pop();
        }
        AccountDeletionHandle::GetInstance().deletingAccounts_.erase(TEST_ACCOUNT_ID_3);
    }
    AccountManager::GetInstance().UnmarkAccountDeleted(TEST_ACCOUNT_ID_3);
}

} // namespace Media
} // namespace OHOS

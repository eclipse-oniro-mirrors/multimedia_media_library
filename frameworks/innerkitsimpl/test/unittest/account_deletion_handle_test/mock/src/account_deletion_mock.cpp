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

#include "account_deletion_mock.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <map>
#include <set>
#include <utility>

#include "account_enable_checker.h"
#include "medialibrary_errno.h"
#include "medialibrary_kvstore_manager.h"
#include "os_account_manager.h"
#include "os_account_subprofile_client.h"

// Build-time canaries: if the REAL headers won the include race (mock/include
// must be first in the target include_dirs), the stub macros stay undefined
// and the build fails loudly here - instead of the mocks silently not taking
// effect and the tests failing at runtime with confusing assertion errors.
#ifndef ACCOUNT_DELETION_STUB_OS_ACCOUNT_MANAGER
#error "real os_account_manager.h resolved: mock/include must precede external include paths"
#endif
#ifndef ACCOUNT_DELETION_STUB_SUBPROFILE_CLIENT
#error "real os_account_subprofile_client.h resolved: mock/include must precede external include paths"
#endif
#ifndef ACCOUNT_DELETION_STUB_ACCOUNT_MANAGER
#error "real account_manager.h resolved: mock/include must precede external include paths"
#endif
#ifndef ACCOUNT_DELETION_STUB_ENABLE_CHECKER
#error "real account_enable_checker.h resolved: mock/include must precede external include paths"
#endif
#ifndef ACCOUNT_DELETION_STUB_KV_MANAGER
#error "real medialibrary_kvstore_manager.h resolved: mock/include must precede external include paths"
#endif

#include "medialibrary_rdbstore.h"

namespace OHOS {
namespace AccountSA {
namespace {
struct OsAccountStubState {
    int32_t foregroundErr = ERR_OK;
    int32_t foregroundLocalId = 100;
    int32_t subProfileIdsErr = ERR_OK;
    std::vector<int32_t> subProfileIds;
    int32_t subscribeErr = ERR_OK;
    int32_t subscribeCallCount = 0;
    // GetOsAccountLocalIdForSubProfile probe result; default E_OK + 100
    // (sub-profile exists) so recovery treats known accounts as alive.
    int32_t localIdForSubProfileErr = ERR_OK;
    int32_t localIdForSubProfileOsAccountId = 100;
    // Per-account probe overrides for GetOsAccountLocalIdForSubProfile:
    // accountId -> (err, osAccountId). Map wins over the global defaults;
    // empty map = every account uses the global result.
    std::map<int32_t, std::pair<int32_t, int32_t>> localIdProbeMap;
    // Event-driven drain for async executor tasks: notified on every
    // SubscribeOsAccountSubProfileEvents call, consumed via
    // AccountDeletionMock::WaitSubscribeCount (no usleep polling).
    std::mutex subscribeMtx;
    std::condition_variable subscribeCv;
};

OsAccountStubState &OsAccountStub()
{
    static OsAccountStubState state;
    return state;
}
}  // namespace

ErrCode OsAccountManager::GetForegroundOsAccountLocalId(int32_t &localId)
{
    localId = OsAccountStub().foregroundLocalId;
    return OsAccountStub().foregroundErr;
}

OsAccountSubProfileClient &OsAccountSubProfileClient::GetInstance()
{
    static OsAccountSubProfileClient instance;
    return instance;
}

ErrCode OsAccountSubProfileClient::GetOsAccountSubProfileIds(
    int32_t osAccountId, std::vector<int32_t> &subProfileIds)
{
    (void)osAccountId;
    subProfileIds = OsAccountStub().subProfileIds;
    return OsAccountStub().subProfileIdsErr;
}

ErrCode OsAccountSubProfileClient::GetOsAccountLocalIdForSubProfile(
    int32_t subProfileId, int32_t &osAccountId)
{
    auto &stub = OsAccountStub();
    auto it = stub.localIdProbeMap.find(subProfileId);
    if (it != stub.localIdProbeMap.end()) {
        if (it->second.first != ERR_OK) {
            return it->second.first;
        }
        osAccountId = it->second.second;
        return ERR_OK;
    }
    if (stub.localIdForSubProfileErr != ERR_OK) {
        return stub.localIdForSubProfileErr;
    }
    osAccountId = stub.localIdForSubProfileOsAccountId;
    return ERR_OK;
}

ErrCode OsAccountSubProfileClient::SubscribeOsAccountSubProfileEvents(
    const std::set<OsAccountSubProfileEventType> &types,
    const std::shared_ptr<OsAccountSubProfileSubscribeCallback> &callback)
{
    (void)types;
    (void)callback;
    {
        std::lock_guard<std::mutex> lock(OsAccountStub().subscribeMtx);
        OsAccountStub().subscribeCallCount++;
    }
    OsAccountStub().subscribeCv.notify_all();
    return OsAccountStub().subscribeErr;
}

ErrCode OsAccountSubProfileClient::UnsubscribeOsAccountSubProfileEvents(
    const std::shared_ptr<OsAccountSubProfileSubscribeCallback> &callback)
{
    (void)callback;
    return ERR_OK;
}
}  // namespace AccountSA

namespace Media {
namespace {
struct AccountManagerStubState {
    std::vector<int32_t> allAccounts;
    int32_t deleteRet = E_OK;
    int32_t deleteCallCount = 0;
};

AccountManagerStubState &AccountManagerStub()
{
    static AccountManagerStubState state;
    return state;
}

// MediaLibraryRdbStore::CloseAccountDb stub result (E_OK by default);
// controlled via AccountDeletionMock::SetCloseAccountDbResult
int32_t &CloseAccountDbStubRet()
{
    static int32_t ret = E_OK;
    return ret;
}

// AccountEnableChecker::IsMultiAccountEnabled result (driver screen = true
// by default); controlled via AccountDeletionMock::SetMultiAccountEnabled
bool &MultiAccountEnabledStub()
{
    static bool enabled = true;
    return enabled;
}

// KV manager stub state: DeleteAccountKvStores result + Close call counts,
// controlled via AccountDeletionMock::SetKvDeleteResult
struct KvManagerStubState {
    int32_t deleteStoresRet = 0; // E_OK
    int32_t monthCloseCount = 0;
    int32_t yearCloseCount = 0;
    int32_t deleteStoresCount = 0;
};

KvManagerStubState &KvManagerStub()
{
    static KvManagerStubState state;
    return state;
}
}  // namespace

MediaLibraryKvStoreManager &MediaLibraryKvStoreManager::GetInstance()
{
    static MediaLibraryKvStoreManager instance;
    return instance;
}

bool MediaLibraryKvStoreManager::CloseKvStore(
    const MediaLibraryKvStoreRoute &route, const KvStoreValueType &valueType)
{
    (void)route;
    if (valueType == KvStoreValueType::MONTH_ASTC) {
        KvManagerStub().monthCloseCount++;
    } else {
        KvManagerStub().yearCloseCount++;
    }
    return true;
}

int32_t MediaLibraryKvStoreManager::DeleteAccountKvStores(const MediaLibraryKvStoreRoute &route)
{
    (void)route;
    KvManagerStub().deleteStoresCount++;
    return KvManagerStub().deleteStoresRet;
}

bool AccountEnableChecker::IsMultiAccountEnabled()
{
    return MultiAccountEnabledStub();
}

// Link self-containment: the exe compiles account_deletion_handle.cpp with
// MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT on, which calls the flag-gated
// MediaLibraryRdbStore::CloseAccountDb. The service lib only contains that
// symbol when the global GN arg media_library_multi_account_enable (default
// true) is on. Define it here so the exe links regardless of the global arg;
// when the service lib also provides it, the exe definition simply takes
// precedence (standard symbol interposition, no duplicate-symbol error).
// Must live INSIDE namespace OHOS::Media: MediaLibraryRdbStore is declared
// there (medialibrary_rdbstore.h), and a global-scope definition fails with
// "use of undeclared identifier".
#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
int32_t MediaLibraryRdbStore::CloseAccountDb(int32_t accountId)
{
    (void)accountId;
    // test stub: real per-account store teardown needs the live rdbStoreMap_;
    // covered by CloseAccountDb tests against the service lib elsewhere
    return CloseAccountDbStubRet();
}
#endif

AccountManager &AccountManager::GetInstance()
{
    static AccountManager instance;
    return instance;
}

bool AccountManager::Init()
{
    return false;
}

void AccountManager::EnsureAccountRootDirsReady() {}

bool AccountManager::GetForegroundAccountId(int32_t &accountId) const
{
    (void)accountId;
    return false;
}

std::unordered_set<int32_t> AccountManager::GetAllAccounts() const
{
    const auto &accounts = AccountManagerStub().allAccounts;
    return std::unordered_set<int32_t>(accounts.begin(), accounts.end());
}

// Marker persistence delegates to the real AccountContext (compiled into this
// exe), so the suite asserts against the actual pref-file JSON state
int32_t AccountManager::MarkAccountDeleted(int32_t accountId)
{
    return accountContext_.MarkAccountDeleted(accountId);
}

int32_t AccountManager::UnmarkAccountDeleted(int32_t accountId)
{
    return accountContext_.UnmarkAccountDeleted(accountId);
}

std::vector<int32_t> AccountManager::GetDeletedAccountIds()
{
    return accountContext_.GetDeletedAccountIds();
}

int32_t AccountManager::Delete(int32_t accountId)
{
    (void)accountId;
    AccountManagerStub().deleteCallCount++;
    return AccountManagerStub().deleteRet;
}

void AccountDeletionMock::Reset()
{
    auto &os = AccountSA::OsAccountStub();
    os.foregroundErr = ERR_OK;
    os.foregroundLocalId = 100;
    os.subProfileIdsErr = ERR_OK;
    os.subProfileIds.clear();
    os.localIdForSubProfileErr = ERR_OK;
    os.localIdForSubProfileOsAccountId = 100;
    os.localIdProbeMap.clear();
    {
        std::lock_guard<std::mutex> lock(os.subscribeMtx);
        os.subscribeErr = ERR_OK;
        os.subscribeCallCount = 0;
    }

    auto &acct = AccountManagerStub();
    acct.allAccounts.clear();
    acct.deleteRet = E_OK;
    acct.deleteCallCount = 0;

    CloseAccountDbStubRet() = E_OK;
    MultiAccountEnabledStub() = true;
    KvManagerStub() = KvManagerStubState();
}

void AccountDeletionMock::SetForegroundOsAccountLocalId(int32_t err, int32_t localId)
{
    AccountSA::OsAccountStub().foregroundErr = err;
    AccountSA::OsAccountStub().foregroundLocalId = localId;
}

void AccountDeletionMock::SetSubProfileIds(int32_t err, std::vector<int32_t> ids)
{
    AccountSA::OsAccountStub().subProfileIdsErr = err;
    AccountSA::OsAccountStub().subProfileIds = std::move(ids);
}

void AccountDeletionMock::SetLocalIdForSubProfile(int32_t err, int32_t osAccountId)
{
    AccountSA::OsAccountStub().localIdForSubProfileErr = err;
    AccountSA::OsAccountStub().localIdForSubProfileOsAccountId = osAccountId;
}

void AccountDeletionMock::SetLocalIdForSubProfileForAccount(
    int32_t accountId, int32_t err, int32_t osAccountId)
{
    AccountSA::OsAccountStub().localIdProbeMap[accountId] = std::make_pair(err, osAccountId);
}

void AccountDeletionMock::SetSubscribeResult(int32_t err)
{
    AccountSA::OsAccountStub().subscribeErr = err;
}

int32_t AccountDeletionMock::GetSubscribeCallCount()
{
    auto &os = AccountSA::OsAccountStub();
    std::lock_guard<std::mutex> lock(os.subscribeMtx);
    return os.subscribeCallCount;
}

bool AccountDeletionMock::WaitSubscribeCount(int32_t expected, int32_t timeoutMs)
{
    auto &os = AccountSA::OsAccountStub();
    std::unique_lock<std::mutex> lock(os.subscribeMtx);
    return os.subscribeCv.wait_for(lock, std::chrono::milliseconds(timeoutMs),
        [&os, expected] { return os.subscribeCallCount >= expected; });
}

void AccountDeletionMock::SetAllAccounts(std::vector<int32_t> ids)
{
    AccountManagerStub().allAccounts = std::move(ids);
}

void AccountDeletionMock::SetDeleteResult(int32_t ret)
{
    AccountManagerStub().deleteRet = ret;
}

int32_t AccountDeletionMock::GetDeleteCallCount()
{
    return AccountManagerStub().deleteCallCount;
}

void AccountDeletionMock::SetCloseAccountDbResult(int32_t ret)
{
    CloseAccountDbStubRet() = ret;
}

void AccountDeletionMock::SetMultiAccountEnabled(bool enabled)
{
    MultiAccountEnabledStub() = enabled;
}

void AccountDeletionMock::SetKvDeleteResult(int32_t ret)
{
    KvManagerStub().deleteStoresRet = ret;
}

int32_t AccountDeletionMock::GetKvCloseCount(int32_t valueType)
{
    return valueType == 0 ? KvManagerStub().monthCloseCount : KvManagerStub().yearCloseCount;
}

int32_t AccountDeletionMock::GetKvDeleteStoresCount()
{
    return KvManagerStub().deleteStoresCount;
}
}  // namespace Media
}  // namespace OHOS

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

#ifndef OHOS_MEDIA_ACCOUNT_DELETION_MOCK_H
#define OHOS_MEDIA_ACCOUNT_DELETION_MOCK_H

#include <cstdint>
#include <vector>

// Pulls in the stubbed Media AccountManager so E_ACCOUNT_NOT_FOUND is visible
// wherever the mock control API is used.
#include "account_manager.h"

namespace OHOS {
namespace Media {

// Link-time stub control. The shadowing headers under mock/include route the
// os_account / AccountManager calls of account_deletion_handle.cpp into this
// state; tests dial scenarios in through these setters. Reset() is called in
// SetUp so every case starts from the default success scenario.
class AccountDeletionMock {
public:
    static void Reset();

    // OHOS::AccountSA::OsAccountManager
    static void SetForegroundOsAccountLocalId(int32_t err, int32_t localId);

    // OHOS::AccountSA::OsAccountSubProfileClient
    static void SetSubProfileIds(int32_t err, std::vector<int32_t> ids);
    static void SetLocalIdForSubProfile(int32_t err, int32_t osAccountId);
    // per-account probe override; wins over SetLocalIdForSubProfile
    static void SetLocalIdForSubProfileForAccount(int32_t accountId, int32_t err, int32_t osAccountId);
    static void SetSubscribeResult(int32_t err);
    static int32_t GetSubscribeCallCount();

    // Event-driven (CV in the subscribe stub): block until the subscribe
    // count reaches expected or timeoutMs elapses; false on timeout.
    // Replaces raw usleep polling in async-drain scenarios.
    static bool WaitSubscribeCount(int32_t expected, int32_t timeoutMs = 3000);

    // OHOS::Media::AccountManager
    static void SetAllAccounts(std::vector<int32_t> ids);
    static void SetDeleteResult(int32_t ret);
    static int32_t GetDeleteCallCount();

    // MediaLibraryRdbStore::CloseAccountDb (stub below the AccountManager
    // section in the mock cpp; controlled here for CleanRdbData's fatal path)
    static void SetCloseAccountDbResult(int32_t ret);

    // AccountEnableChecker::IsMultiAccountEnabled (driver-screen gate);
    // default true = driver screen, false = other screens (skip paths)
    static void SetMultiAccountEnabled(bool enabled);

    // MediaLibraryKvStoreManager account-scoped stubs (CleanKvData, Step 2).
    // valueType for GetKvCloseCount: 0 = MONTH_ASTC, 1 = YEAR_ASTC
    static void SetKvDeleteResult(int32_t ret);
    static int32_t GetKvCloseCount(int32_t valueType);
    static int32_t GetKvDeleteStoresCount();
};

}  // namespace Media
}  // namespace OHOS

#endif  // OHOS_MEDIA_ACCOUNT_DELETION_MOCK_H

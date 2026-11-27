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

// Test stub: shadows the real Media AccountManager header
// (services/media_multi_account/include/account_manager.h, prerequisite PR).
// Shape mirrors the real interface so code compiling against the stub also
// compiles against the real header; behavior is driven by AccountDeletionMock
// setters.
// NOTE: no EXPORT macro here - the real header's #define EXPORT (inside the
// namespace, with its own spacing) collides with the 85+ repo headers defining
// the same macro with a different body; the stub lives inside the test exe and
// needs no visibility attributes.
#ifndef OHOS_MEDIA_ACCOUNT_MANAGER_MOCK_H
#define OHOS_MEDIA_ACCOUNT_MANAGER_MOCK_H

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

// Real header (not shadowed): marker persistence ("deleted_accounts" key)
#include "account_context.h"

#define ACCOUNT_DELETION_STUB_ACCOUNT_MANAGER 1

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

namespace OHOS {
namespace Media {

// --- Account manager orchestrator (stub) ---
class AccountManager {
public:
    static AccountManager &GetInstance();

    bool Init();
    static void EnsureAccountRootDirsReady();

    bool GetForegroundAccountId(int32_t &accountId) const;
    std::unordered_set<int32_t> GetAllAccounts() const;
    int32_t Delete(int32_t accountId);

    int32_t MarkAccountDeleted(int32_t accountId);
    int32_t UnmarkAccountDeleted(int32_t accountId);
    std::vector<int32_t> GetDeletedAccountIds();

private:
    AccountManager() = default;
    ~AccountManager() = default;
    AccountManager(const AccountManager &) = delete;
    AccountManager &operator=(const AccountManager &) = delete;

    // Real per-file marker persistence; the deletion suite asserts against it
    AccountContext accountContext_;
};

}  // namespace Media
}  // namespace OHOS

#endif  // MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

#endif  // OHOS_MEDIA_ACCOUNT_MANAGER_MOCK_H
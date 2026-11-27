/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_ACCOUNT_CONTEXT_H
#define OHOS_MEDIA_ACCOUNT_CONTEXT_H

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

#include <memory>
#include "nlohmann/json.hpp"
#include "preferences.h"

namespace OHOS {
namespace Media {

class AccountContext {
public:
    AccountContext() = default;
    ~AccountContext() = default;

    /**
     * @brief Initialize account context from persistent storage
     */
    void Init();

    /**
     * @brief Add an account to the account list
     * @return True if the account was newly added, false if it already existed
     */
    bool AddAccount(int32_t accountId);

    /**
     * @brief Remove an account from the account list
     */
    int32_t RemoveAccount(int32_t accountId);

    /**
     * @brief Set the specified account as foreground account
     */
    void SetForegroundAccount(int32_t accountId);

    /**
     * @brief Get all accounts as a const reference
     */
    std::unordered_set<int32_t> GetAllAccounts() const;

    /**
     * @brief Get the total number of accounts
     */
    size_t GetAccountCount() const;

    /**
     * @brief Check if the specified account exists in the account list
     */
    bool HasAccount(int32_t accountId) const;

    /**
     * @brief Get the default account ID = 100001
     */
    static int32_t GetDefaultAccountId();

    /**
     * @brief Get the foreground account ID
     */
    int32_t GetForegroundAccountId() const;

    /**
     * @brief Check if the specified account is the foreground account
     */
    bool IsForegroundAccount(int32_t accountId) const;

    /**
     * @brief Mark the account as being deleted ("deleted_accounts" pref key, FlushSync)
     * @return E_OK on success (idempotent when already marked), E_FAIL when prefs unavailable
     */
    int32_t MarkAccountDeleted(int32_t accountId);

    /**
     * @brief Remove the deletion marker (FlushSync); idempotent
     */
    int32_t UnmarkAccountDeleted(int32_t accountId);

    /**
     * @brief Get the account IDs currently marked as deleting
     */
    std::vector<int32_t> GetDeletedAccountIds();

private:
    /**
     * @brief Check if the account ID is valid (6-digit range)
     */
    static bool IsValidAccountId(int32_t accountId);

    /**
     * @brief Load JSON configuration from preferences
     */
    static nlohmann::json LoadJson(std::shared_ptr<NativePreferences::Preferences>& prefs);

    /**
     * @brief Save account ID to persistent preferences
     * @param accountId The account ID to save
     */
    static void SaveAccountToPref(int32_t accountId);

    /**
     * @brief Remove account ID from persistent preferences
     * @param accountId The account ID to remove
     */
    static void RemoveAccountFromPref(int32_t accountId);

    /**
     * @brief Save the foreground account ID to persistent storage
     */
    void SaveForegroundAccountIdToPref();

    /**
     * @brief Ensure the shared xml file exists with 771 permission before writing
     */
    static void EnsureSharedXmlReady();

private:
    static constexpr int32_t DEFAULT_ACCOUNT_ID = 100001;

    mutable std::mutex mutex_;
    std::unordered_set<int32_t> accounts_;
    int32_t foregroundAccountId_ = DEFAULT_ACCOUNT_ID;
};

}  // namespace Media
}  // namespace OHOS

#endif  // MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

#endif  // OHOS_MEDIA_ACCOUNT_CONTEXT_H

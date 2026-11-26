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

#ifndef OHOS_MEDIA_ACCOUNT_MANAGER_H
#define OHOS_MEDIA_ACCOUNT_MANAGER_H

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <functional>
#include <unordered_set>
#include <vector>

#include "account_context.h"

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

using HwidOnStartHandler = std::function<void(int32_t accountId)>;
using HwidOnStopHandler = std::function<void(int32_t accountId)>;

/**
 * @brief Base class for account event observers
 * Subclasses override OnAccountLogout to implement cleanup logic on account logout,
 * then register via AccountManager::RegisterObserver.
 */
class AccountObserver {
public:
    virtual ~AccountObserver() = default;

    /**
     * @brief Callback invoked when an account logs out
     * @param accountId The account ID that is logging out
     */
    virtual void OnAccountLogout(int32_t accountId) {}
};

class AccountManager {
public:
    EXPORT static AccountManager& GetInstance();
    EXPORT bool Init();

    /**
     * @brief Login the current account
     */
    EXPORT int32_t Login();

    /**
     * @brief Logout the current account
     */
    EXPORT int32_t Logout();

    /**
     * @brief Get the foreground account ID
     * @param accountId Output the foreground account ID if multi-account is enabled
     * @return True if multi-account is enabled and accountId is valid, false otherwise
     */
    EXPORT bool GetForegroundAccountId(int32_t& accountId) const;

    /**
     * @brief Get all accounts as a const reference
     */
    EXPORT std::unordered_set<int32_t> GetAllAccounts() const;

    /**
     * @brief Ensure root directories are ready for all accounts
     */
    EXPORT static void EnsureAccountRootDirsReady();

    // Set callbacks for MediaLibrary to handle account events
    /**
     * @brief Set callback for account login (OnStart) event
     */
    EXPORT void SetHwidOnStartHandler(HwidOnStartHandler handler);

    /**
     * @brief Set callback for account logout (OnStop) event
     */
    EXPORT void SetHwidOnStopHandler(HwidOnStopHandler handler);

    /**
     * @brief Register an account event observer
     * @param observer The observer object that will receive OnAccountLogout callback on account logout
     */
    EXPORT void RegisterObserver(std::shared_ptr<AccountObserver> observer);

    /**
     * @brief Unregister an account event observer
     * @param observer The observer object to unregister
     */
    EXPORT void UnregisterObserver(std::shared_ptr<AccountObserver> observer);

    // delete
    EXPORT int32_t Delete(int32_t accountId);

    // Deletion markers (persisted by AccountContext, crash-recovery "deleted_accounts" key)
    EXPORT int32_t MarkAccountDeleted(int32_t accountId);
    EXPORT int32_t UnmarkAccountDeleted(int32_t accountId);
    EXPORT std::vector<int32_t> GetDeletedAccountIds();

private:
    AccountManager() = default;
    ~AccountManager() = default;
    AccountManager(const AccountManager&) = delete;
    AccountManager& operator=(const AccountManager&) = delete;
    AccountManager(AccountManager&&) = delete;
    AccountManager& operator=(AccountManager&&) = delete;

    // Callbacks
    void HwidLoginInternal(int32_t accountId);
    void HwidLogoutInternal(int32_t accountId);

    // State
    AccountContext accountContext_;
    bool isInitialized_ = false;

    HwidOnStartHandler hwidOnStartHandler_;         // On account login, execute OnStart business -> OnAccountLogin
    HwidOnStopHandler hwidOnStopHandler_;           // On account logout, execute OnStop business -> OnAccountLogout
    std::mutex handlerMutex_;                       // Protects hwidOnStartHandler_ / hwidOnStopHandler_

    std::mutex observerMutex_;
    std::vector<std::shared_ptr<AccountObserver>> observers_;
};

}  // namespace Media
}  // namespace OHOS

#endif  // MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

#endif  // OHOS_MEDIA_ACCOUNT_MANAGER_H

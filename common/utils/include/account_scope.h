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

#ifndef OHOS_MEDIA_ACCOUNT_SCOPE_H
#define OHOS_MEDIA_ACCOUNT_SCOPE_H

#include <cstdint>

namespace OHOS {
namespace Media {

/**
 * @brief Sentinel value meaning "no account specified".
 *
 * When AccountScope::Get() returns this value, the caller should fall back
 * to the foreground account.
 */
static constexpr int32_t INVALID_ACCOUNT_ID = -1;

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

/**
 * @brief RAII guard that sets the current accountId in thread-local storage.
 */
class AccountScope {
public:
    /**
     * @brief Sets the current thread's accountId, saving the previous value.
     */
    explicit AccountScope(int32_t accountId);

    /**
     * @brief Restores the previous accountId on scope exit.
     */
    ~AccountScope();

    AccountScope(const AccountScope &) = delete;
    AccountScope &operator=(const AccountScope &) = delete;

    /**
     * @brief Returns the current thread's accountId.
     */
    static int32_t Get();

private:
    int32_t prev_;

    static int32_t &Current();
};

#endif  // MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

}  // namespace Media
}  // namespace OHOS

#endif  // OHOS_MEDIA_ACCOUNT_SCOPE_H

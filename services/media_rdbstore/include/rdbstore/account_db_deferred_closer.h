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

#ifndef OHOS_ACCOUNT_DB_DEFERRED_CLOSER_H
#define OHOS_ACCOUNT_DB_DEFERRED_CLOSER_H

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

#include <cstdint>
#include <mutex>
#include <unordered_map>

#include "timer.h"

namespace OHOS {
namespace Media {

class AccountDbDeferredCloser {
public:
    static constexpr uint32_t CLOSE_DELAY_MS = 5 * 60 * 1000; // 5min

    static AccountDbDeferredCloser& GetInstance();
    void Start(int32_t accountId);          // 登出时启动/重置该账号倒计时
    void ResetIfPending(int32_t accountId); // 命中倒计时账号时重置为 5min；命中 needShutdown_ 则关闭 timer
    void Cancel(int32_t accountId);         // 登录时取消该账号倒计时；命中 needShutdown_ 则关闭 timer
    void MarkExpired(int32_t accountId);    // 回调(timer线程)：清理 pending_，空则置 needShutdown_

private:
    AccountDbDeferredCloser();
    ~AccountDbDeferredCloser();
    AccountDbDeferredCloser(const AccountDbDeferredCloser&) = delete;
    AccountDbDeferredCloser& operator=(const AccountDbDeferredCloser&) = delete;
    void EnsureTimerSetupLocked();          // 锁内：timer 未拉起则 Setup（已拉起返回 INVALID_VALUE，属预期）

    std::mutex mtx_;
    Utils::Timer timer_;
    std::unordered_map<int32_t, uint32_t> pending_;  // accountId -> timerId
    bool needShutdown_ = false;                       // MarkExpired 置位，委托业务线程 Shutdown
};

} // namespace Media
} // namespace OHOS
#endif // MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
#endif // OHOS_ACCOUNT_DB_DEFERRED_CLOSER_H

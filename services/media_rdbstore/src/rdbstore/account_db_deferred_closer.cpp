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

#define MLOG_TAG "AccountDbDeferredCloser"

#include "account_db_deferred_closer.h"

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"

namespace OHOS {
namespace Media {

namespace {
// 倒计时到期回调：关闭该账号 DB，并清理 pending_。
// 在 timer 线程执行，只做 erase 不做 Shutdown（避免 timer 线程自我 join 死锁）。
// timer 的 Shutdown 交给业务线程的 ResetIfPending/Cancel 在 pending_ 为空时触发。
void OnDeferredCloseExpired(int32_t accountId)
{
    MEDIA_INFO_LOG("Deferred close expired, closing accountId=%{public}d", accountId);
    int32_t ret = MediaLibraryRdbStore::CloseAccountDb(accountId);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CloseAccountDb failed, accountId=%{public}d, ret=%{public}d", accountId, ret);
    }
    // 清理 pending_（once 定时器已由 Timer 内部自动清理，无需 Unregister）
    auto &closer = AccountDbDeferredCloser::GetInstance();
    closer.MarkExpired(accountId);
}
} // namespace

AccountDbDeferredCloser& AccountDbDeferredCloser::GetInstance()
{
    static AccountDbDeferredCloser instance;
    return instance;
}

AccountDbDeferredCloser::AccountDbDeferredCloser() : timer_("AccountDbDeferredCloser")
{
    MEDIA_INFO_LOG("AccountDbDeferredCloser created, delay=%{public}u ms", CLOSE_DELAY_MS);
}

AccountDbDeferredCloser::~AccountDbDeferredCloser()
{
    std::lock_guard<std::mutex> lock(mtx_);
    pending_.clear();
    timer_.Shutdown();
}

void AccountDbDeferredCloser::EnsureTimerSetupLocked()
{
    // Setup 在 timer 线程未拉起时创建并返回 TIMER_ERR_OK；已拉起则返回 TIMER_ERR_INVALID_VALUE，属预期。
    uint32_t ret = timer_.Setup();
    if (ret == 0) { // TIMER_ERR_OK
        MEDIA_INFO_LOG("Timer Setup");
    }
}

void AccountDbDeferredCloser::MarkExpired(int32_t accountId)
{
    // timer 线程执行：只做 erase，绝不 Shutdown（否则自我 join 死锁）。
    // pending_ 因到期变空时置 needShutdown_，委托后续业务线程在锁内安全 Shutdown。
    std::lock_guard<std::mutex> lock(mtx_);
    pending_.erase(accountId);
    if (pending_.empty()) {
        needShutdown_ = true;
    }
    MEDIA_INFO_LOG("MarkExpired, accountId=%{public}d, pendingSize=%{public}zu, needShutdown=%{public}d",
        accountId, pending_.size(), needShutdown_ ? 1 : 0);
}

void AccountDbDeferredCloser::Start(int32_t accountId)
{
    std::lock_guard<std::mutex> lock(mtx_);
    // needShutdown_ 置位时 timer 仍存活（MarkExpired 未 Shutdown）：复用，清标志，不 Shutdown。
    needShutdown_ = false;
    auto it = pending_.find(accountId);
    if (it != pending_.end() && it->second != 0) {
        timer_.Unregister(it->second);
    }
    EnsureTimerSetupLocked(); // 未拉起才创建；已拉起返回 INVALID_VALUE，无副作用
    uint32_t timerId = timer_.Register(
        [accountId]() { OnDeferredCloseExpired(accountId); }, CLOSE_DELAY_MS, true);
    pending_[accountId] = timerId;
    MEDIA_INFO_LOG("Start deferred close, accountId=%{public}d, timerId=%{public}u, delay=%{public}u ms",
        accountId, timerId, CLOSE_DELAY_MS);
}

void AccountDbDeferredCloser::ResetIfPending(int32_t accountId)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (needShutdown_) {
        // pending_ 已因到期变空，timer 线程已回到空闲等待：锁内 Shutdown(join) 安全，不死锁。
        timer_.Shutdown();
        needShutdown_ = false;
        MEDIA_INFO_LOG("Timer Shutdown (no pending tasks, triggered by ResetIfPending)");
        return;
    }
    auto it = pending_.find(accountId);
    if (it != pending_.end() && it->second != 0) {
        // 该账号正在倒计时，重置为 5min
        timer_.Unregister(it->second);
        uint32_t timerId = timer_.Register(
            [accountId]() { OnDeferredCloseExpired(accountId); }, CLOSE_DELAY_MS, true);
        it->second = timerId;
        MEDIA_INFO_LOG("Reset deferred close, accountId=%{public}d, timerId=%{public}u", accountId, timerId);
    }
}

void AccountDbDeferredCloser::Cancel(int32_t accountId)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (needShutdown_) {
        // pending_ 已因到期变空，timer 线程已回到空闲等待：锁内 Shutdown(join) 安全，不死锁。
        timer_.Shutdown();
        needShutdown_ = false;
        MEDIA_INFO_LOG("Timer Shutdown (no pending tasks, triggered by Cancel)");
        return;
    }
    auto it = pending_.find(accountId);
    if (it == pending_.end()) {
        return;
    }
    if (it->second != 0) {
        timer_.Unregister(it->second);
    }
    pending_.erase(it);
    // 注意：此处即使 pending_ 变空也不 Shutdown/置 needShutdown_——此时 MarkExpired 可能仍在途，
    // 锁内 Shutdown 会与其争锁死锁。留待下次到期回调或下次 Start 处理。
    MEDIA_INFO_LOG("Cancel deferred close, accountId=%{public}d, pendingSize=%{public}zu",
        accountId, pending_.size());
}

} // namespace Media
} // namespace OHOS

#endif // MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

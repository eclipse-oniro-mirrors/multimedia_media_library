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

#define MLOG_TAG "Lcd_Aging"

#include "deep_optimize_space_napi_callback.h"

#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <condition_variable>
#include <new>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "medialibrary_errno.h"
#include "media_library_error_code.h"
#include "medialibrary_napi_utils.h"
#include "medialibrary_business_code.h"
#include "deep_optimize_space_vo.h"
#include "user_define_ipc_client.h"

namespace OHOS::Media {
namespace {
const std::string DEEP_OPTIMIZE_SPACE_STATE_FIELD = "state";
const std::string DEEP_OPTIMIZE_SPACE_PROGRESS_FIELD = "progress";
constexpr size_t MAX_DEEP_OPTIMIZE_SPACE_CALLBACK_RECORDS = 64;
constexpr int64_t DEEP_OPTIMIZE_SPACE_CALLBACK_TIMEOUT_SECONDS = 8;
constexpr auto DEEP_OPTIMIZE_SPACE_CALLBACK_TIMEOUT =
    std::chrono::seconds(DEEP_OPTIMIZE_SPACE_CALLBACK_TIMEOUT_SECONDS);
using ThreadSafeFunctionHandle = std::remove_pointer_t<napi_threadsafe_function>;

struct DeepOptimizeSpaceRegistryRecord {
    std::shared_ptr<DeepOptimizeSpaceJsCallbackHolder> holder;
    sptr<DeepOptimizeSpaceCallbackStub> callbackStub;
    sptr<IRemoteObject> callbackRemote;
    std::chrono::steady_clock::time_point deadline;
    int32_t lastProgress = 0;
};

struct DeepOptimizeSpaceJsCallbackData {
    DeepOptimizeSpaceState state = DeepOptimizeSpaceState::RUNNING;
    int32_t progress = 0;
};

struct ThreadSafeFunctionReleaser {
    void operator()(ThreadSafeFunctionHandle *threadSafeFunc) const
    {
        if (threadSafeFunc == nullptr) {
            return;
        }
        napi_release_threadsafe_function(
            reinterpret_cast<napi_threadsafe_function>(threadSafeFunc), napi_tsfn_release);
    }
};

using ThreadSafeFunctionGuard = std::unique_ptr<ThreadSafeFunctionHandle, ThreadSafeFunctionReleaser>;

class DeepOptimizeSpaceCallbackRegistryImpl final {
public:
    using TimeoutRecord = std::tuple<uint64_t, std::shared_ptr<DeepOptimizeSpaceJsCallbackHolder>, int32_t>;

    static DeepOptimizeSpaceCallbackRegistryImpl &GetInstance()
    {
        static DeepOptimizeSpaceCallbackRegistryImpl instance;
        return instance;
    }

    uint64_t Register(const std::shared_ptr<DeepOptimizeSpaceJsCallbackHolder> &holder,
        const sptr<DeepOptimizeSpaceCallbackStub> &callbackStub, const sptr<IRemoteObject> &callbackRemote)
    {
        if (holder == nullptr || callbackStub == nullptr) {
            NAPI_WARN_LOG("Skip register deep optimize space callback registry, invalid callback objects");
            return 0;
        }

        std::thread finishedWatcher;
        uint64_t registryId = 0;
        size_t registrySize = 0;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (watcherState_ == WatcherState::SHUTDOWN) {
                NAPI_ERR_LOG("Deep optimize space callback registry is shutting down");
                return 0;
            }
            TakeFinishedWatcherLocked(finishedWatcher);
            if (records_.size() >= MAX_DEEP_OPTIMIZE_SPACE_CALLBACK_RECORDS) {
                registrySize = records_.size();
            } else {
                registryId = RegisterLocked(holder, callbackStub, callbackRemote);
            }
        }
        JoinWatcher(finishedWatcher);
        if (registryId == 0) {
            NAPI_ERR_LOG("Deep optimize space callback registry is full, size: %{public}zu", registrySize);
            return 0;
        }

        holder->SetRegistryId(registryId);
        cv_.notify_one();
        return registryId;
    }

    void Unregister(uint64_t registryId)
    {
        if (registryId == 0) {
            return;
        }

        std::thread finishedWatcher;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            (void)records_.erase(registryId);
            TakeFinishedWatcherLocked(finishedWatcher);
        }
        cv_.notify_one();
        JoinWatcher(finishedWatcher);
    }

    void UpdateDeadlineAndProgress(uint64_t registryId, int32_t progress)
    {
        if (registryId == 0) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        auto it = records_.find(registryId);
        if (it != records_.end()) {
            it->second.deadline = std::chrono::steady_clock::now() + DEEP_OPTIMIZE_SPACE_CALLBACK_TIMEOUT;
            it->second.lastProgress = progress;
        }
    }

private:
    DeepOptimizeSpaceCallbackRegistryImpl() = default;

    ~DeepOptimizeSpaceCallbackRegistryImpl()
    {
        Shutdown();
    }

    DeepOptimizeSpaceCallbackRegistryImpl(const DeepOptimizeSpaceCallbackRegistryImpl &) = delete;
    DeepOptimizeSpaceCallbackRegistryImpl &operator=(const DeepOptimizeSpaceCallbackRegistryImpl &) = delete;

    void Shutdown()
    {
        std::thread watcherThread;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            watcherState_ = WatcherState::SHUTDOWN;
            watcherThread = std::move(watcherThread_);
        }
        cv_.notify_all();
        if (watcherThread.joinable()) {
            watcherThread.join();
        }
    }

    void StartWatcherLocked()
    {
        watcherThread_ = std::thread([this]() { RunWatcherUntilIdle(); });
        watcherState_ = WatcherState::RUNNING;
    }

    void TakeFinishedWatcherLocked(std::thread &finishedWatcher)
    {
        if (watcherState_ != WatcherState::IDLE || !watcherThread_.joinable()) {
            return;
        }
        finishedWatcher = std::move(watcherThread_);
    }

    uint64_t RegisterLocked(const std::shared_ptr<DeepOptimizeSpaceJsCallbackHolder> &holder,
        const sptr<DeepOptimizeSpaceCallbackStub> &callbackStub, const sptr<IRemoteObject> &callbackRemote)
    {
        uint64_t registryId = nextRegistryId_++;
        records_.emplace(registryId, DeepOptimizeSpaceRegistryRecord {
            holder, callbackStub, callbackRemote,
            std::chrono::steady_clock::now() + DEEP_OPTIMIZE_SPACE_CALLBACK_TIMEOUT});
        if (watcherState_ == WatcherState::IDLE) {
            StartWatcherLocked();
        }
        return registryId;
    }

    static void JoinWatcher(std::thread &watcherThread)
    {
        if (watcherThread.joinable()) {
            watcherThread.join();
        }
    }

    void RunWatcherUntilIdle()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (watcherState_ != WatcherState::SHUTDOWN) {
            if (records_.empty()) {
                watcherState_ = WatcherState::IDLE;
                return;
            }

            std::vector<TimeoutRecord> timeoutRecords;
            auto nextDeadline = CollectExpiredLocked(timeoutRecords);
            if (!timeoutRecords.empty()) {
                lock.unlock();
                NotifyExpired(timeoutRecords);
                lock.lock();
                continue;
            }

            cv_.wait_until(lock, nextDeadline, [this]() {
                return watcherState_ == WatcherState::SHUTDOWN || records_.empty();
            });
        }
    }

    std::chrono::steady_clock::time_point CollectExpiredLocked(std::vector<TimeoutRecord> &timeoutRecords)
    {
        const auto now = std::chrono::steady_clock::now();
        auto nextDeadline = now + DEEP_OPTIMIZE_SPACE_CALLBACK_TIMEOUT;
        for (auto it = records_.begin(); it != records_.end();) {
            if (it->second.deadline <= now) {
                timeoutRecords.emplace_back(it->first, it->second.holder, it->second.lastProgress);
                it = records_.erase(it);
                continue;
            }
            nextDeadline = std::min(nextDeadline, it->second.deadline);
            ++it;
        }
        return nextDeadline;
    }

    static void NotifyExpired(const std::vector<TimeoutRecord> &timeoutRecords)
    {
        for (const auto &[registryId, holder, lastProgress] : timeoutRecords) {
            if (holder == nullptr) {
                continue;
            }
            NAPI_WARN_LOG("Deep optimize space callback timeout reached, registryId: %{public}" PRIu64
                ", lastProgress: %{public}d", registryId, lastProgress);
            
            (void)holder->NotifyProgress(DeepOptimizeSpaceState::FAILED, lastProgress, "watchdog_timeout");

            int32_t ret = IPC::UserDefineIPCClient().Call(
                static_cast<uint32_t>(MediaLibraryBusinessCode::STOP_DEEP_OPTIMIZE_SPACE));
            NAPI_INFO_LOG("StopDeepOptimizeSpace on timeout returned, ret: %{public}d", ret);
        }
    }

    std::mutex mutex_;
    std::condition_variable cv_;
    std::unordered_map<uint64_t, DeepOptimizeSpaceRegistryRecord> records_;
    uint64_t nextRegistryId_ = 1;
    std::thread watcherThread_;
    enum class WatcherState : uint8_t {
        IDLE = 0,
        RUNNING,
        SHUTDOWN,
    };
    WatcherState watcherState_ = WatcherState::IDLE;
};

static void CallJsDeepOptimizeSpaceCallback(napi_env env, napi_value jsCallback, void *context, void *data)
{
    (void)context;
    std::unique_ptr<DeepOptimizeSpaceJsCallbackData> callbackData(static_cast<DeepOptimizeSpaceJsCallbackData *>(data));
    CHECK_NULL_PTR_RETURN_VOID(env, "env is nullptr");
    CHECK_NULL_PTR_RETURN_VOID(jsCallback, "js callback is nullptr");
    if (callbackData == nullptr) {
        NAPI_ERR_LOG("Deep optimize space callback data is nullptr");
        return;
    }

    napi_value callbackArg = nullptr;
    napi_value stateValue = nullptr;
    napi_value progressValue = nullptr;
    napi_value result = nullptr;
    napi_status status = napi_create_object(env, &callbackArg);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Failed to create deep optimize space callback object, status: %{public}d",
            static_cast<int32_t>(status));
        return;
    }
    status = napi_create_int32(env, static_cast<int32_t>(callbackData->state), &stateValue);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Failed to create deep optimize space callback state value, status: %{public}d",
            static_cast<int32_t>(status));
        return;
    }
    status = napi_set_named_property(env, callbackArg, DEEP_OPTIMIZE_SPACE_STATE_FIELD.c_str(), stateValue);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Failed to set deep optimize space callback state property, status: %{public}d",
            static_cast<int32_t>(status));
        return;
    }
    status = napi_create_int32(env, callbackData->progress, &progressValue);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Failed to create deep optimize space callback progress value, status: %{public}d",
            static_cast<int32_t>(status));
        return;
    }
    status = napi_set_named_property(env, callbackArg, DEEP_OPTIMIZE_SPACE_PROGRESS_FIELD.c_str(), progressValue);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Failed to set deep optimize space callback progress property, status: %{public}d",
            static_cast<int32_t>(status));
        return;
    }
    status = napi_call_function(env, nullptr, jsCallback, 1, &callbackArg, &result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Failed to invoke deep optimize space JS callback, status: %{public}d",
            static_cast<int32_t>(status));
        return;
    }
}
} // namespace

DeepOptimizeSpaceJsCallbackHolder::DeepOptimizeSpaceJsCallbackHolder(napi_threadsafe_function threadSafeFunc)
    : threadSafeFunc_(threadSafeFunc)
{
}

DeepOptimizeSpaceJsCallbackHolder::~DeepOptimizeSpaceJsCallbackHolder()
{
    Release();
}

napi_status DeepOptimizeSpaceJsCallbackHolder::Create(
    napi_env env, napi_value callback, std::shared_ptr<DeepOptimizeSpaceJsCallbackHolder> &holder)
{
    napi_threadsafe_function threadSafeFunc = nullptr;
    napi_value workName = nullptr;
    CHECK_STATUS_RET(napi_create_string_utf8(env, "DeepOptimizeSpaceCallback", NAPI_AUTO_LENGTH, &workName),
        "Failed to create deep optimize space callback work name");
    CHECK_STATUS_RET(napi_create_threadsafe_function(env, callback, nullptr, workName, 0, 1, nullptr, nullptr,
        nullptr, CallJsDeepOptimizeSpaceCallback, &threadSafeFunc), "Failed to create deep optimize space tsfn");
    ThreadSafeFunctionGuard threadSafeFuncGuard(reinterpret_cast<ThreadSafeFunctionHandle *>(threadSafeFunc));
    holder = std::shared_ptr<DeepOptimizeSpaceJsCallbackHolder>(
        new (std::nothrow) DeepOptimizeSpaceJsCallbackHolder(threadSafeFunc));
    if (holder == nullptr) {
        NAPI_ERR_LOG("Failed to allocate deep optimize space callback holder");
        return napi_generic_failure;
    }
    (void)threadSafeFuncGuard.release();
    return napi_ok;
}

int32_t DeepOptimizeSpaceJsCallbackHolder::PrepareNotifyProgress(const char *source,
    napi_threadsafe_function &threadSafeFunc, uint64_t &registryId)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (released_) {
            return E_OK;
        }
        threadSafeFunc = threadSafeFunc_;
        registryId = registryId_.load();
    }

    if (threadSafeFunc == nullptr) {
        NAPI_ERR_LOG("Deep optimize space callback threadSafeFunc is null, source: %{public}s,"
            " registryId: %{public}" PRIu64,
            source, registryId);
        return MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR;
    }
    return E_OK;
}

void DeepOptimizeSpaceJsCallbackHolder::Release()
{
    napi_threadsafe_function threadSafeFunc = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (released_) {
            return;
        }
        released_ = true;
        threadSafeFunc = threadSafeFunc_;
        threadSafeFunc_ = nullptr;
    }
    if (threadSafeFunc != nullptr) {
        napi_release_threadsafe_function(threadSafeFunc, napi_tsfn_release);
    }
}

void DeepOptimizeSpaceJsCallbackHolder::SetRegistryId(uint64_t registryId)
{
    registryId_.store(registryId);
}

void DeepOptimizeSpaceJsCallbackHolder::CleanupRegistry()
{
    uint64_t registryId = registryId_.exchange(0);
    if (registryId == 0) {
        return;
    }
    DeepOptimizeSpaceJsCallbackRegistry::Unregister(registryId);
}

int32_t DeepOptimizeSpaceJsCallbackHolder::NotifyProgress(DeepOptimizeSpaceState state, int32_t progress,
    const char *source)
{
    napi_threadsafe_function threadSafeFunc = nullptr;
    uint64_t registryId = 0;
    int32_t prepareRet = PrepareNotifyProgress(source, threadSafeFunc, registryId);
    if (prepareRet != E_OK) {
        Release();
        CleanupRegistry();
        return prepareRet;
    }
    if (threadSafeFunc == nullptr) {
        return E_OK;
    }
    auto *callbackData = new (std::nothrow) DeepOptimizeSpaceJsCallbackData();
    if (callbackData == nullptr) {
        NAPI_ERR_LOG("Failed to allocate deep optimize space callback data, source: %{public}s,"
            " registryId: %{public}" PRIu64, source, registryId);
        Release();
        CleanupRegistry();
        return MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR;
    }
    
    callbackData->state = state;
    callbackData->progress = progress;
    
    napi_status status = napi_call_threadsafe_function(threadSafeFunc, callbackData, napi_tsfn_blocking);
    if (status != napi_ok) {
        NAPI_ERR_LOG("napi_call_threadsafe_function failed, status: %{public}d, source: %{public}s,"
            " registryId: %{public}" PRIu64, static_cast<int32_t>(status), source, registryId);
        delete callbackData;
        Release();
        CleanupRegistry();
        return MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR;
    }
    if (state != DeepOptimizeSpaceState::RUNNING) {
        Release();
        CleanupRegistry();
    } else {
        DeepOptimizeSpaceJsCallbackRegistry::UpdateDeadlineAndProgress(registryId, progress);
    }
    return E_OK;
}

DeepOptimizeSpaceJsCallbackStub::DeepOptimizeSpaceJsCallbackStub(
    std::shared_ptr<DeepOptimizeSpaceJsCallbackHolder> holder)
    : holder_(std::move(holder))
{
}

int32_t DeepOptimizeSpaceJsCallbackStub::OnProgressUpdate(const DeepOptimizeSpaceProgress &progress)
{
    if (holder_ == nullptr) {
        NAPI_WARN_LOG("Deep optimize space callback stub holder is null, skip notification (dummy stub)");
        return E_OK;
    }
    return holder_->NotifyProgress(progress.state, progress.progress, "service_callback");
}

int32_t DeepOptimizeSpaceDummyJsCallbackStub::OnProgressUpdate(const DeepOptimizeSpaceProgress &progress)
{
    NAPI_INFO_LOG("Dummy stub received progress update, state: %{public}d, progress: %{public}d, "
        "no actual callback to invoke", static_cast<int32_t>(progress.state), progress.progress);
    return E_OK;
}

uint64_t DeepOptimizeSpaceJsCallbackRegistry::Register(
    const std::shared_ptr<DeepOptimizeSpaceJsCallbackHolder> &holder,
    const sptr<DeepOptimizeSpaceCallbackStub> &callbackStub, const sptr<IRemoteObject> &callbackRemote)
{
    return DeepOptimizeSpaceCallbackRegistryImpl::GetInstance().Register(holder, callbackStub, callbackRemote);
}

void DeepOptimizeSpaceJsCallbackRegistry::Unregister(uint64_t registryId)
{
    DeepOptimizeSpaceCallbackRegistryImpl::GetInstance().Unregister(registryId);
}

void DeepOptimizeSpaceJsCallbackRegistry::UpdateDeadlineAndProgress(uint64_t registryId, int32_t progress)
{
    DeepOptimizeSpaceCallbackRegistryImpl::GetInstance().UpdateDeadlineAndProgress(registryId, progress);
}
} // namespace OHOS::Media
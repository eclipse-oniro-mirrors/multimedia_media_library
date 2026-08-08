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

#define MLOG_TAG "ActiveAnalysisNapiCallback"

#include "active_analysis_napi_callback.h"

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

#include "active_analysis_error_utils.h"
#include "medialibrary_errno.h"
#include "media_library_error_code.h"
#include "medialibrary_napi_utils.h"

namespace OHOS::Media {
namespace {
const std::string ACTIVE_ANALYSIS_RESULT_FIELD = "result";
const std::string ANALYSIS_TOOL_CODE_FIELD = "errCode";
const std::string ANALYSIS_TOOL_RESULT_FIELD = "result";
constexpr size_t MAX_ACTIVE_ANALYSIS_CALLBACK_RECORDS = 512;
constexpr int64_t ACTIVE_ANALYSIS_CALLBACK_TIMEOUT_MINUTES = 10;
constexpr auto ACTIVE_ANALYSIS_CALLBACK_TIMEOUT =
    std::chrono::minutes(ACTIVE_ANALYSIS_CALLBACK_TIMEOUT_MINUTES);
using ThreadSafeFunctionHandle = std::remove_pointer_t<napi_threadsafe_function>;

struct ActiveAnalysisRegistryRecord {
    std::shared_ptr<ActiveAnalysisJsCallbackHolder> holder;
    sptr<ActiveAnalysisJsCallbackStub> callbackStub;
    sptr<IRemoteObject> callbackRemote;
    std::chrono::steady_clock::time_point deadline;
};

struct ActiveAnalysisJsCallbackData {
    int32_t result = 0;
    bool isToolResult = false;
    int32_t toolCode = 0;
    std::string toolResult;
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

class ActiveAnalysisCallbackRegistryImpl final {
public:
    using TimeoutRecord = std::pair<uint64_t, std::shared_ptr<ActiveAnalysisJsCallbackHolder>>;

    static ActiveAnalysisCallbackRegistryImpl &GetInstance()
    {
        static ActiveAnalysisCallbackRegistryImpl instance;
        return instance;
    }

    uint64_t Register(const std::shared_ptr<ActiveAnalysisJsCallbackHolder> &holder,
        const sptr<ActiveAnalysisJsCallbackStub> &callbackStub, const sptr<IRemoteObject> &callbackRemote)
    {
        if (holder == nullptr || callbackStub == nullptr || callbackRemote == nullptr) {
            NAPI_WARN_LOG("Skip register active analysis callback registry, invalid callback objects");
            return 0;
        }

        std::thread finishedWatcher;
        uint64_t registryId = 0;
        size_t registrySize = 0;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (watcherState_ == WatcherState::SHUTDOWN) {
                NAPI_ERR_LOG("Active analysis callback registry is shutting down");
                return 0;
            }
            TakeFinishedWatcherLocked(finishedWatcher);
            if (records_.size() >= MAX_ACTIVE_ANALYSIS_CALLBACK_RECORDS) {
                registrySize = records_.size();
            } else {
                registryId = RegisterLocked(holder, callbackStub, callbackRemote);
            }
        }
        JoinWatcher(finishedWatcher);
        if (registryId == 0) {
            NAPI_ERR_LOG("Active analysis callback registry is full, size: %{public}zu", registrySize);
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

private:
    ActiveAnalysisCallbackRegistryImpl() = default;

    ~ActiveAnalysisCallbackRegistryImpl()
    {
        Shutdown();
    }

    ActiveAnalysisCallbackRegistryImpl(const ActiveAnalysisCallbackRegistryImpl &) = delete;
    ActiveAnalysisCallbackRegistryImpl &operator=(const ActiveAnalysisCallbackRegistryImpl &) = delete;

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

    uint64_t RegisterLocked(const std::shared_ptr<ActiveAnalysisJsCallbackHolder> &holder,
        const sptr<ActiveAnalysisJsCallbackStub> &callbackStub, const sptr<IRemoteObject> &callbackRemote)
    {
        uint64_t registryId = nextRegistryId_++;
        records_.emplace(registryId, ActiveAnalysisRegistryRecord {
            holder, callbackStub, callbackRemote, std::chrono::steady_clock::now() + ACTIVE_ANALYSIS_CALLBACK_TIMEOUT});
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
        auto nextDeadline = now + ACTIVE_ANALYSIS_CALLBACK_TIMEOUT;
        for (auto it = records_.begin(); it != records_.end();) {
            if (it->second.deadline <= now) {
                timeoutRecords.emplace_back(it->first, it->second.holder);
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
        for (const auto &[registryId, holder] : timeoutRecords) {
            if (holder == nullptr) {
                continue;
            }
            NAPI_WARN_LOG("Active analysis callback timeout reached before any result, registryId: %{public}" PRIu64,
                registryId);
            (void)holder->NotifyResult(MEDIA_LIBRARY_ACTIVE_ANALYSIS_OTHER_ERROR, "watchdog_timeout");
        }
    }

    std::mutex mutex_;
    std::condition_variable cv_;
    std::unordered_map<uint64_t, ActiveAnalysisRegistryRecord> records_;
    uint64_t nextRegistryId_ = 1;
    std::thread watcherThread_;
    enum class WatcherState : uint8_t {
        IDLE = 0,
        RUNNING,
        SHUTDOWN,
    };
    WatcherState watcherState_ = WatcherState::IDLE;
};

static bool BuildToolCallbackArg(napi_env env, napi_value callbackArg,
    const ActiveAnalysisJsCallbackData &callbackData)
{
    napi_value codeValue = nullptr;
    napi_status status = napi_create_int32(env, callbackData.toolCode, &codeValue);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Failed to create tool callback code value, status: %{public}d",
            static_cast<int32_t>(status));
        return false;
    }
    status = napi_set_named_property(env, callbackArg, ANALYSIS_TOOL_CODE_FIELD.c_str(), codeValue);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Failed to set tool callback code property, status: %{public}d",
            static_cast<int32_t>(status));
        return false;
    }
    if (!callbackData.toolResult.empty()) {
        napi_value resultValue = nullptr;
        status = napi_create_string_utf8(env, callbackData.toolResult.c_str(), NAPI_AUTO_LENGTH, &resultValue);
        if (status != napi_ok) {
            NAPI_ERR_LOG("Failed to create tool callback result value, status: %{public}d",
                static_cast<int32_t>(status));
            return false;
        }
        status = napi_set_named_property(env, callbackArg, ANALYSIS_TOOL_RESULT_FIELD.c_str(), resultValue);
        if (status != napi_ok) {
            NAPI_ERR_LOG("Failed to set tool callback result property, status: %{public}d",
                static_cast<int32_t>(status));
            return false;
        }
    }
    return true;
}

static void CallJsActiveAnalysisCallback(napi_env env, napi_value jsCallback, void *context, void *data)
{
    (void)context;
    std::unique_ptr<ActiveAnalysisJsCallbackData> callbackData(static_cast<ActiveAnalysisJsCallbackData *>(data));
    CHECK_NULL_PTR_RETURN_VOID(env, "env is nullptr");
    CHECK_NULL_PTR_RETURN_VOID(jsCallback, "js callback is nullptr");
    if (callbackData == nullptr) {
        NAPI_ERR_LOG("Active analysis callback data is nullptr");
        return;
    }

    napi_value callbackArg = nullptr;
    napi_value result = nullptr;
    napi_status status = napi_create_object(env, &callbackArg);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Failed to create active analysis callback object, status: %{public}d",
            static_cast<int32_t>(status));
        return;
    }

    if (callbackData->isToolResult) {
        if (!BuildToolCallbackArg(env, callbackArg, *callbackData)) {
            return;
        }
    } else {
        // Active analysis result: { result:number
        napi_value resultValue = nullptr;
        status = napi_create_int32(env, callbackData->result, &resultValue);
        if (status != napi_ok) {
            NAPI_ERR_LOG("Failed to create active analysis callback result value, status: %{public}d",
                static_cast<int32_t>(status));
            return;
        }
        status = napi_set_named_property(env, callbackArg, ACTIVE_ANALYSIS_RESULT_FIELD.c_str(), resultValue);
        if (status != napi_ok) {
            NAPI_ERR_LOG("Failed to set active analysis callback result property, status: %{public}d",
                static_cast<int32_t>(status));
            return;
        }
    }

    status = napi_call_function(env, nullptr, jsCallback, 1, &callbackArg, &result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Failed to invoke active analysis JS callback, status: %{public}d",
            static_cast<int32_t>(status));
        return;
    }
}
} // namespace

ActiveAnalysisSaDeathRecipient::ActiveAnalysisSaDeathRecipient(std::weak_ptr<ActiveAnalysisJsCallbackHolder> holder)
    : holder_(std::move(holder))
{
}

void ActiveAnalysisSaDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &remote)
{
    (void)remote;
    auto holder = holder_.lock();
    if (holder == nullptr) {
        return;
    }
    holder->HandleSaDied();
}

ActiveAnalysisJsCallbackHolder::ActiveAnalysisJsCallbackHolder(napi_threadsafe_function threadSafeFunc)
    : threadSafeFunc_(threadSafeFunc)
{
}

ActiveAnalysisJsCallbackHolder::~ActiveAnalysisJsCallbackHolder()
{
    Release();
}

napi_status ActiveAnalysisJsCallbackHolder::Create(
    napi_env env, napi_value callback, std::shared_ptr<ActiveAnalysisJsCallbackHolder> &holder)
{
    napi_threadsafe_function threadSafeFunc = nullptr;
    napi_value workName = nullptr;
    CHECK_STATUS_RET(napi_create_string_utf8(env, "ActiveAnalysisCallback", NAPI_AUTO_LENGTH, &workName),
        "Failed to create active analysis callback work name");
    CHECK_STATUS_RET(napi_create_threadsafe_function(env, callback, nullptr, workName, 0, 1, nullptr, nullptr,
        nullptr, CallJsActiveAnalysisCallback, &threadSafeFunc), "Failed to create active analysis tsfn");
    ThreadSafeFunctionGuard threadSafeFuncGuard(reinterpret_cast<ThreadSafeFunctionHandle *>(threadSafeFunc));
    holder = std::shared_ptr<ActiveAnalysisJsCallbackHolder>(
        new (std::nothrow) ActiveAnalysisJsCallbackHolder(threadSafeFunc));
    if (holder == nullptr) {
        NAPI_ERR_LOG("Failed to allocate active analysis callback holder");
        return napi_generic_failure;
    }
    (void)threadSafeFuncGuard.release();
    return napi_ok;
}

int32_t ActiveAnalysisJsCallbackHolder::PrepareNotifyResult(const char *source,
    napi_threadsafe_function &threadSafeFunc, uint64_t &registryId)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (released_ || resultReceived_) {
            return E_OK;
        }
        resultReceived_ = true;
        threadSafeFunc = threadSafeFunc_;
        registryId = registryId_.load();
    }

    if (threadSafeFunc == nullptr) {
        NAPI_ERR_LOG("Active analysis callback threadSafeFunc is null, source: %{public}s,"
            " registryId: %{public}" PRIu64,
            source, registryId);
        return MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR;
    }
    return E_OK;
}

void ActiveAnalysisJsCallbackHolder::MarkResultPostedToJs()
{
    std::lock_guard<std::mutex> lock(mutex_);
    resultPostedToJs_ = true;
}

int32_t ActiveAnalysisJsCallbackHolder::NotifyResult(int32_t result, const char *source)
{
    napi_threadsafe_function threadSafeFunc = nullptr;
    uint64_t registryId = 0;
    int32_t prepareRet = PrepareNotifyResult(source, threadSafeFunc, registryId);
    if (prepareRet != E_OK && threadSafeFunc == nullptr) {
        Release();
        CleanupRegistry();
        return prepareRet;
    }
    if (threadSafeFunc == nullptr) {
        return E_OK;
    }
    int32_t normalizedResult = NormalizeActiveAnalysisErrorCode(result);
    auto *callbackData = new (std::nothrow) ActiveAnalysisJsCallbackData();
    if (callbackData == nullptr) {
        NAPI_ERR_LOG("Failed to allocate active analysis callback data, source: %{public}s,"
            " registryId: %{public}" PRIu64,
            source, registryId);
        Release();
        CleanupRegistry();
        return MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR;
    }
    callbackData->result = normalizedResult;
    callbackData->isToolResult = false;
    napi_status status = napi_call_threadsafe_function(threadSafeFunc, callbackData, napi_tsfn_blocking);
    if (status != napi_ok) {
        NAPI_ERR_LOG("napi_call_threadsafe_function failed, status: %{public}d, source: %{public}s,"
            " registryId: %{public}" PRIu64, static_cast<int32_t>(status), source, registryId);
        delete callbackData;
        Release();
        CleanupRegistry();
        return MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR;
    }
    MarkResultPostedToJs();
    Release();
    CleanupRegistry();
    return E_OK;
}

int32_t ActiveAnalysisJsCallbackHolder::NotifyToolResult(int32_t code, const std::string &result,
    const char *source)
{
    NAPI_INFO_LOG("NotifyToolResult, code: %{public}d, result: %{public}s", code, result.c_str());
    napi_threadsafe_function threadSafeFunc = nullptr;
    uint64_t registryId = 0;
    int32_t prepareRet = PrepareNotifyResult(source, threadSafeFunc, registryId);
    if (prepareRet != E_OK && threadSafeFunc == nullptr) {
        Release();
        CleanupRegistry();
        return prepareRet;
    }
    if (threadSafeFunc == nullptr) {
        return E_OK;
    }
    int32_t normalizedCode = NormalizeActiveAnalysisErrorCode(code);
    auto *callbackData = new (std::nothrow) ActiveAnalysisJsCallbackData();
    if (callbackData == nullptr) {
        NAPI_ERR_LOG("Failed to allocate analysis tool callback: data, source: %{public}s,"
            " registryId: %{public}" PRIu64, source, registryId);
        Release();
        CleanupRegistry();
        return MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR;
    }
    callbackData->isToolResult = true;
    callbackData->toolCode = normalizedCode;
    callbackData->toolResult = result;
    napi_status status = napi_call_threadsafe_function(threadSafeFunc, callbackData, napi_tsfn_blocking);
    if (status != napi_ok) {
        NAPI_ERR_LOG("napi_call_threadsafe_function failed, status: %{public}d, source: %{public}s,"
            " registryId: %{public}" PRIu64, static_cast<int32_t>(status), source, registryId);
        delete callbackData;
        Release();
        CleanupRegistry();
        return MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR;
    }
    MarkResultPostedToJs();
    Release();
    CleanupRegistry();
    return E_OK;
}

void ActiveAnalysisJsCallbackHolder::MarkAsToolCallback()
{
    isToolCallback_ = true;
}

void ActiveAnalysisJsCallbackHolder::HandleSaDied()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (released_) {
            return;
        }
        if (resultReceived_) {
            return;
        }
    }
    NAPI_WARN_LOG("Active analysis SA died before media library received callback result, registryId: %{public}" PRIu64,
        registryId_.load());
    if (isToolCallback_) {
        (void)NotifyToolResult(MEDIA_LIBRARY_ACTIVE_ANALYSIS_OTHER_ERROR, "sa_died", "sa_died");
    } else {
        (void)NotifyResult(MEDIA_LIBRARY_ACTIVE_ANALYSIS_OTHER_ERROR, "sa_died");
    }
}

ActiveAnalysisSaBindResult ActiveAnalysisJsCallbackHolder::BindSaRemote(const sptr<IRemoteObject> &saRemote)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (saRemote == nullptr) {
        NAPI_WARN_LOG("Skip binding active analysis SA remote, released: %{public}d, remoteValid: 0",
            released_);
        return ActiveAnalysisSaBindResult::INVALID_REMOTE;
    }
    if (released_ && resultPostedToJs_) {
        NAPI_WARN_LOG("Skip binding active analysis SA remote because result was already posted to JS");
        return ActiveAnalysisSaBindResult::ALREADY_COMPLETED;
    }
    if (released_) {
        NAPI_WARN_LOG("Skip binding active analysis SA remote because callback holder was already released");
        return ActiveAnalysisSaBindResult::HOLDER_RELEASED;
    }
    saRemote_ = saRemote;
    saDeathRecipient_ = sptr<IRemoteObject::DeathRecipient>(new ActiveAnalysisSaDeathRecipient(weak_from_this()));
    if (!saRemote_->AddDeathRecipient(saDeathRecipient_)) {
        NAPI_WARN_LOG("Failed to add active analysis SA death recipient");
        saDeathRecipient_ = nullptr;
        saRemote_ = nullptr;
        return ActiveAnalysisSaBindResult::ADD_DEATH_RECIPIENT_FAILED;
    }
    return ActiveAnalysisSaBindResult::BOUND;
}

void ActiveAnalysisJsCallbackHolder::Release()
{
    sptr<IRemoteObject> saRemote;
    sptr<IRemoteObject::DeathRecipient> saDeathRecipient;
    napi_threadsafe_function threadSafeFunc = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (released_) {
            return;
        }
        released_ = true;
        saRemote = saRemote_;
        saDeathRecipient = saDeathRecipient_;
        threadSafeFunc = threadSafeFunc_;
        saDeathRecipient_ = nullptr;
        saRemote_ = nullptr;
        threadSafeFunc_ = nullptr;
    }
    if (saRemote != nullptr && saDeathRecipient != nullptr) {
        saRemote->RemoveDeathRecipient(saDeathRecipient);
    }
    if (threadSafeFunc != nullptr) {
        napi_release_threadsafe_function(threadSafeFunc, napi_tsfn_release);
    }
}

void ActiveAnalysisJsCallbackHolder::SetRegistryId(uint64_t registryId)
{
    registryId_.store(registryId);
}

void ActiveAnalysisJsCallbackHolder::CleanupRegistry()
{
    uint64_t registryId = registryId_.exchange(0);
    if (registryId == 0) {
        return;
    }
    ActiveAnalysisJsCallbackRegistry::Unregister(registryId);
}

ActiveAnalysisJsCallbackStub::ActiveAnalysisJsCallbackStub(std::shared_ptr<ActiveAnalysisJsCallbackHolder> holder)
    : holder_(std::move(holder))
{
}

int32_t ActiveAnalysisJsCallbackStub::OnAnalysisFinished(const ActiveAnalysisCallbackResult &result)
{
    if (holder_ == nullptr) {
        NAPI_WARN_LOG("Active analysis callback stub holder is null, result: %{public}d", result.result);
        return E_OK;
    }
    return holder_->NotifyResult(result.result, "service_callback");
}

int32_t ActiveAnalysisJsCallbackStub::OnToolFinished(const AnalysisToolCallbackResult &result)
{
    NAPI_INFO_LOG("[tool] code: %{public}d, result: %{public}s", result.code, result.result.c_str());
    if (holder_ == nullptr) {
        NAPI_WARN_LOG("Analysis tool callback stub holder is null, code: %{public}d", result.code);
        return E_OK;
    }
    return holder_->NotifyToolResult(result.code, result.result, "service_callback");
}

uint64_t ActiveAnalysisJsCallbackRegistry::Register(const std::shared_ptr<ActiveAnalysisJsCallbackHolder> &holder,
    const sptr<ActiveAnalysisJsCallbackStub> &callbackStub, const sptr<IRemoteObject> &callbackRemote)
{
    return ActiveAnalysisCallbackRegistryImpl::GetInstance().Register(holder, callbackStub, callbackRemote);
}

void ActiveAnalysisJsCallbackRegistry::Unregister(uint64_t registryId)
{
    ActiveAnalysisCallbackRegistryImpl::GetInstance().Unregister(registryId);
}
} // namespace OHOS::Media

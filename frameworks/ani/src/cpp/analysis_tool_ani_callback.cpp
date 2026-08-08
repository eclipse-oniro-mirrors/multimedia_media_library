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

#define MLOG_TAG "AnalysisToolAniCallback"

#include "analysis_tool_ani_callback.h"

#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <condition_variable>
#include <new>
#include <thread>
#include <unordered_map>
#include <vector>

#include "ani_class_name.h"
#include "medialibrary_ani_log.h"
#include "medialibrary_ani_utils.h"
#include "medialibrary_notify_ani_utils.h"
#include "medialibrary_errno.h"
#include "media_library_error_code.h"
#include "media_library_enum_ani.h"

namespace OHOS::Media {
namespace {
class AniThreadGuard {
public:
    AniThreadGuard(ani_vm *vm, ani_options *options, ani_env **env)
    {
        vm_ = vm;
        attached_ = (vm != nullptr && vm->AttachCurrentThread(options, ANI_VERSION_1, env) == ANI_OK);
        if (!attached_) {
            ANI_ERR_LOG("AttachCurrentThread fail");
        }
    }
    ~AniThreadGuard()
    {
        if (attached_ && vm_ != nullptr) {
            vm_->DetachCurrentThread();
        }
    }
    bool IsAttached() const { return attached_; }
private:
    ani_vm *vm_ = nullptr;
    bool attached_ = false;
};

constexpr size_t MAX_ANALYSIS_TOOL_ANI_CALLBACK_RECORDS = 512;
constexpr int64_t ANALYSIS_TOOL_ANI_CALLBACK_TIMEOUT_MINUTES = 10;
constexpr auto ANALYSIS_TOOL_ANI_CALLBACK_TIMEOUT =
    std::chrono::minutes(ANALYSIS_TOOL_ANI_CALLBACK_TIMEOUT_MINUTES);

struct AnalysisToolAniRegistryRecord {
    std::shared_ptr<AnalysisToolAniCallbackHolder> holder;
    sptr<AnalysisToolAniCallbackStub> callbackStub;
    sptr<IRemoteObject> callbackRemote;
    std::chrono::steady_clock::time_point deadline;
};

class AnalysisToolAniCallbackRegistryImpl final {
public:
    static AnalysisToolAniCallbackRegistryImpl &GetInstance()
    {
        static AnalysisToolAniCallbackRegistryImpl instance;
        return instance;
    }

    uint64_t Register(const std::shared_ptr<AnalysisToolAniCallbackHolder> &holder,
        const sptr<AnalysisToolAniCallbackStub> &callbackStub, const sptr<IRemoteObject> &callbackRemote)
    {
        if (holder == nullptr || callbackStub == nullptr) {
            ANI_WARN_LOG("Skip register analysis tool ani callback registry, invalid callback objects");
            return 0;
        }

        std::thread finishedWatcher;
        uint64_t registryId = 0;
        size_t registrySize = 0;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (watcherState_ == WatcherState::SHUTDOWN) {
                ANI_ERR_LOG("Analysis tool ani callback registry is shutting down");
                return 0;
            }
            TakeFinishedWatcherLocked(finishedWatcher);
            if (records_.size() >= MAX_ANALYSIS_TOOL_ANI_CALLBACK_RECORDS) {
                registrySize = records_.size();
            } else {
                registryId = RegisterLocked(holder, callbackStub, callbackRemote);
            }
        }
        JoinWatcher(finishedWatcher);
        if (registryId == 0) {
            ANI_ERR_LOG("Analysis tool ani callback registry is full, size: %{public}zu", registrySize);
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
    AnalysisToolAniCallbackRegistryImpl()
    {
        watcherState_ = WatcherState::IDLE;
        watcherThread_ = std::thread(&AnalysisToolAniCallbackRegistryImpl::WatcherLoop, this);
    }

    ~AnalysisToolAniCallbackRegistryImpl()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            watcherState_ = WatcherState::SHUTDOWN;
        }
        cv_.notify_one();
        if (watcherThread_.joinable()) {
            watcherThread_.join();
        }
    }

    uint64_t RegisterLocked(const std::shared_ptr<AnalysisToolAniCallbackHolder> &holder,
        const sptr<AnalysisToolAniCallbackStub> &callbackStub, const sptr<IRemoteObject> &callbackRemote)
    {
        uint64_t registryId = nextRegistryId_++;
        records_[registryId] = AnalysisToolAniRegistryRecord {
            .holder = holder,
            .callbackStub = callbackStub,
            .callbackRemote = callbackRemote,
            .deadline = std::chrono::steady_clock::now() + ANALYSIS_TOOL_ANI_CALLBACK_TIMEOUT
        };
        ANI_INFO_LOG("Registered analysis tool ani callback, registryId: %{public}" PRIu64, registryId);
        return registryId;
    }

    void TakeFinishedWatcherLocked(std::thread &finishedWatcher)
    {
        finishedWatcher = std::move(finishedWatcherToJoin_);
    }

    void JoinWatcher(std::thread &finishedWatcher)
    {
        if (finishedWatcher.joinable()) {
            finishedWatcher.join();
        }
    }

    void WatcherLoop()
    {
        while (watcherState_ != WatcherState::SHUTDOWN) {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] {
                return watcherState_ == WatcherState::SHUTDOWN || !records_.empty();
            });

            if (watcherState_ == WatcherState::SHUTDOWN) {
                break;
            }

            auto now = std::chrono::steady_clock::now();
            std::vector<uint64_t> expiredIds;
            for (auto it = records_.begin(); it != records_.end(); ++it) {
                if (it->second.deadline <= now) {
                    expiredIds.push_back(it->first);
                }
            }

            for (const auto &expiredId : expiredIds) {
                auto it = records_.find(expiredId);
                if (it != records_.end()) {
                    auto holder = it->second.holder;
                    records_.erase(it);
                    lock.unlock();
                    ANI_WARN_LOG("Analysis tool ani callback timeout, registryId: %{public}" PRIu64, expiredId);
                    (void)holder->NotifyToolResult(MEDIA_LIBRARY_ACTIVE_ANALYSIS_OTHER_ERROR,
                        "watchdog_timeout", "watchdog_timeout");
                    lock.lock();
                }
            }

            if (!records_.empty()) {
                auto earliestDeadline = std::min_element(records_.begin(), records_.end(),
                    [](const auto &a, const auto &b) { return a.second.deadline < b.second.deadline; });
                cv_.wait_until(lock, earliestDeadline->second.deadline);
            }
        }
    }

    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread watcherThread_;
    std::thread finishedWatcherToJoin_;
    std::unordered_map<uint64_t, AnalysisToolAniRegistryRecord> records_;
    uint64_t nextRegistryId_ = 1;

    enum class WatcherState {
        IDLE,
        RUNNING,
        SHUTDOWN,
    };
    WatcherState watcherState_ = WatcherState::IDLE;
};
} // namespace

AnalysisToolAniSaDeathRecipient::AnalysisToolAniSaDeathRecipient(
    std::weak_ptr<AnalysisToolAniCallbackHolder> holder)
    : holder_(std::move(holder))
{
}

void AnalysisToolAniSaDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &remote)
{
    (void)remote;
    auto holder = holder_.lock();
    if (holder == nullptr) {
        return;
    }
    holder->HandleSaDied();
}

AnalysisToolAniCallbackHolder::AnalysisToolAniCallbackHolder(ani_vm *vm, ani_ref callbackRef)
    : aniVm_(vm), callbackRef_(callbackRef)
{
}

AnalysisToolAniCallbackHolder::~AnalysisToolAniCallbackHolder()
{
    Release();
}

ani_status AnalysisToolAniCallbackHolder::Create(ani_env *env, ani_fn_object callback,
    std::shared_ptr<AnalysisToolAniCallbackHolder> &holder)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");

    ani_vm *vm = nullptr;
    CHECK_STATUS_RET(env->GetVM(&vm), "Failed to get VM");

    ani_ref callbackRef = nullptr;
    ani_status createRet = env->GlobalReference_Create(static_cast<ani_ref>(callback), &callbackRef);
    CHECK_STATUS_RET(createRet, "Failed to create global reference");

    holder = std::shared_ptr<AnalysisToolAniCallbackHolder>(
        new (std::nothrow) AnalysisToolAniCallbackHolder(vm, callbackRef));
    if (holder == nullptr) {
        ANI_ERR_LOG("Failed to allocate analysis tool ani callback holder");
        env->GlobalReference_Delete(callbackRef);
        return ANI_ERROR;
    }

    return ANI_OK;
}

PrepareNotifyStatus AnalysisToolAniCallbackHolder::PrepareNotifyResult(const char *source, uint64_t &registryId)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (released_ || resultReceived_) {
            return PrepareNotifyStatus::ALREADY_COMPLETED;
        }
        resultReceived_ = true;
        registryId = registryId_.load();
    }

    if (aniVm_ == nullptr || callbackRef_ == nullptr) {
        ANI_ERR_LOG("Analysis tool ani callback vm or callbackRef is null, source: %{public}s,"
            " registryId: %{public}" PRIu64, source, registryId);
        return PrepareNotifyStatus::ERROR;
    }
    return PrepareNotifyStatus::SHOULD_NOTIFY;
}

void AnalysisToolAniCallbackHolder::CallAniCallback(int32_t code, const std::string &result)
{
    ani_env *env = nullptr;
    ani_option interopEnabled {"--interop=disable", nullptr};
    ani_options aniArgs {1, &interopEnabled};
    AniThreadGuard guard(aniVm_, &aniArgs, &env);
    if (!guard.IsAttached()) {
        ANI_ERR_LOG("AttachCurrentThread fail for analysis tool callback");
        return;
    }

    ani_object retObj = nullptr;
    if (MediaLibraryNotifyAniUtils::CreateAniObject(env,
        PAH_ANI_CLASS_ANALYSIS_TOOL_RESULT_HANDLE, retObj) != ANI_OK || retObj == nullptr) {
        ANI_ERR_LOG("CallAniCallback CreateAniObject failed");
        return;
    }

    ani_status setRet = MediaLibraryNotifyAniUtils::SetValueInt32(env, "errCode", code, retObj);
    if (setRet != ANI_OK) {
        ANI_ERR_LOG("CallAniCallback SetValueInt32 errCode fail, ret=%{public}d code=%{public}d", setRet, code);
    }
    if (!result.empty()) {
        setRet = MediaLibraryNotifyAniUtils::SetValueString(env, "result", result, retObj);
        if (setRet != ANI_OK) {
            ANI_ERR_LOG("CallAniCallback SetValueString result fail, ret=%{public}d", setRet);
        }
    }

    ani_fn_object aniCallback = static_cast<ani_fn_object>(callbackRef_);
    if (aniCallback == nullptr) {
        ANI_ERR_LOG("CallAniCallback null callbackRef");
        return;
    }

    std::vector<ani_ref> args = { retObj };
    ani_ref returnVal;
    ani_status status = env->FunctionalObject_Call(aniCallback, 1, args.data(), &returnVal);
    if (status != ANI_OK) {
        ANI_ERR_LOG("CallAniCallback fail status=%{public}d code=%{public}d", status, code);
        return;
    }
    ANI_INFO_LOG("[tool] CallAniCallback success code=%{public}d", code);
}

int32_t AnalysisToolAniCallbackHolder::NotifyToolResult(int32_t code, const std::string &result,
    const char *source)
{
    ANI_ERR_LOG("code: %{public}d, result: %{public}s", code, result.c_str());
    uint64_t registryId = 0;
    PrepareNotifyStatus prepareRet = PrepareNotifyResult(source, registryId);
    if (prepareRet == PrepareNotifyStatus::ALREADY_COMPLETED) {
        return E_OK;
    }
    if (prepareRet != PrepareNotifyStatus::SHOULD_NOTIFY) {
        Release();
        CleanupRegistry();
        return MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR;
    }

    CallAniCallback(code, result);

    Release();
    CleanupRegistry();
    return E_OK;
}

void AnalysisToolAniCallbackHolder::HandleSaDied()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (released_ || resultReceived_) {
            return;
        }
    }
    ANI_WARN_LOG("Analysis tool SA died before receiving callback result, registryId: %{public}" PRIu64,
        registryId_.load());
    (void)NotifyToolResult(MEDIA_LIBRARY_ACTIVE_ANALYSIS_OTHER_ERROR, "sa_died", "sa_died");
}

AnalysisToolAniSaBindResult AnalysisToolAniCallbackHolder::BindSaRemote(const sptr<IRemoteObject> &saRemote)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (saRemote == nullptr) {
        return AnalysisToolAniSaBindResult::INVALID_REMOTE;
    }
    if (resultReceived_) {
        return AnalysisToolAniSaBindResult::ALREADY_COMPLETED;
    }
    if (released_) {
        return AnalysisToolAniSaBindResult::HOLDER_RELEASED;
    }
    saRemote_ = saRemote;
    saDeathRecipient_ = sptr<IRemoteObject::DeathRecipient>(
        new AnalysisToolAniSaDeathRecipient(weak_from_this()));
    if (!saRemote_->AddDeathRecipient(saDeathRecipient_)) {
        ANI_WARN_LOG("Failed to add analysis tool SA death recipient");
        saDeathRecipient_ = nullptr;
        saRemote_ = nullptr;
        return AnalysisToolAniSaBindResult::ADD_DEATH_RECIPIENT_FAILED;
    }
    return AnalysisToolAniSaBindResult::BOUND;
}

void AnalysisToolAniCallbackHolder::Release()
{
    sptr<IRemoteObject> saRemote;
    sptr<IRemoteObject::DeathRecipient> saDeathRecipient;
    ani_ref callbackRef = nullptr;
    ani_vm *vm = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (released_) {
            return;
        }
        released_ = true;
        saRemote = saRemote_;
        saDeathRecipient = saDeathRecipient_;
        callbackRef = callbackRef_;
        callbackRef_ = nullptr;
        saDeathRecipient_ = nullptr;
        saRemote_ = nullptr;
        vm = aniVm_;
    }
    if (saRemote != nullptr && saDeathRecipient != nullptr) {
        saRemote->RemoveDeathRecipient(saDeathRecipient);
    }
    if (vm != nullptr && callbackRef != nullptr) {
        ani_env *env = nullptr;
        if (vm->AttachCurrentThread(nullptr, ANI_VERSION_1, &env) == ANI_OK && env != nullptr) {
            env->GlobalReference_Delete(callbackRef);
            vm->DetachCurrentThread();
        } else {
            ANI_ERR_LOG("Failed to AttachCurrentThread for release");
        }
    }
}

void AnalysisToolAniCallbackHolder::SetRegistryId(uint64_t registryId)
{
    registryId_.store(registryId);
}

void AnalysisToolAniCallbackHolder::CleanupRegistry()
{
    uint64_t registryId = registryId_.exchange(0);
    if (registryId == 0) {
        return;
    }
    AnalysisToolAniCallbackRegistry::Unregister(registryId);
}

AnalysisToolAniCallbackStub::AnalysisToolAniCallbackStub(
    std::shared_ptr<AnalysisToolAniCallbackHolder> holder)
    : holder_(std::move(holder))
{
}

int32_t AnalysisToolAniCallbackStub::OnToolFinished(const AnalysisToolCallbackResult &result)
{
    if (holder_ == nullptr) {
        ANI_WARN_LOG("Analysis tool ani callback stub holder is null, code: %{public}d", result.code);
        return E_OK;
    }
    return holder_->NotifyToolResult(result.code, result.result, "service_callback");
}

uint64_t AnalysisToolAniCallbackRegistry::Register(
    const std::shared_ptr<AnalysisToolAniCallbackHolder> &holder,
    const sptr<AnalysisToolAniCallbackStub> &callbackStub, const sptr<IRemoteObject> &callbackRemote)
{
    return AnalysisToolAniCallbackRegistryImpl::GetInstance().Register(holder, callbackStub, callbackRemote);
}

void AnalysisToolAniCallbackRegistry::Unregister(uint64_t registryId)
{
    AnalysisToolAniCallbackRegistryImpl::GetInstance().Unregister(registryId);
}
} // namespace OHOS::Media

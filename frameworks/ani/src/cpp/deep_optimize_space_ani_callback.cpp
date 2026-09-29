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

#include "deep_optimize_space_ani_callback.h"

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
#include "medialibrary_ani_log.h"
#include "medialibrary_ani_utils.h"
#include "medialibrary_business_code.h"
#include "deep_optimize_space_vo.h"
#include "media_library_enum_ani.h"
#include "user_define_ipc_client.h"

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

constexpr size_t MAX_DEEP_OPTIMIZE_SPACE_ANI_CALLBACK_RECORDS = 64;
constexpr int64_t DEEP_OPTIMIZE_SPACE_ANI_CALLBACK_TIMEOUT_SECONDS = 8;
constexpr auto DEEP_OPTIMIZE_SPACE_ANI_CALLBACK_TIMEOUT =
    std::chrono::seconds(DEEP_OPTIMIZE_SPACE_ANI_CALLBACK_TIMEOUT_SECONDS);

const std::string DEEP_OPTIMIZE_SPACE_STATE_FIELD = "state";
const std::string DEEP_OPTIMIZE_SPACE_PROGRESS_FIELD = "progress";

struct DeepOptimizeSpaceAniRegistryRecord {
    std::shared_ptr<DeepOptimizeSpaceAniCallbackHolder> holder;
    sptr<DeepOptimizeSpaceCallbackStub> callbackStub;
    sptr<IRemoteObject> callbackRemote;
    std::chrono::steady_clock::time_point deadline;
    int32_t lastProgress = 0;
};

class DeepOptimizeSpaceAniCallbackRegistryImpl final {
public:
    static DeepOptimizeSpaceAniCallbackRegistryImpl &GetInstance()
    {
        static DeepOptimizeSpaceAniCallbackRegistryImpl instance;
        return instance;
    }

    uint64_t Register(const std::shared_ptr<DeepOptimizeSpaceAniCallbackHolder> &holder,
        const sptr<DeepOptimizeSpaceCallbackStub> &callbackStub, const sptr<IRemoteObject> &callbackRemote)
    {
        if (holder == nullptr || callbackStub == nullptr) {
            ANI_WARN_LOG("Skip register deep optimize space ani callback registry, invalid callback objects");
            return 0;
        }

        std::thread finishedWatcher;
        uint64_t registryId = 0;
        size_t registrySize = 0;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (watcherState_ == WatcherState::SHUTDOWN) {
                ANI_ERR_LOG("Deep optimize space ani callback registry is shutting down");
                return 0;
            }
            TakeFinishedWatcherLocked(finishedWatcher);
            if (records_.size() >= MAX_DEEP_OPTIMIZE_SPACE_ANI_CALLBACK_RECORDS) {
                registrySize = records_.size();
            } else {
                registryId = RegisterLocked(holder, callbackStub, callbackRemote);
            }
        }
        JoinWatcher(finishedWatcher);
        if (registryId == 0) {
            ANI_ERR_LOG("Deep optimize space ani callback registry is full, size: %{public}zu", registrySize);
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
            it->second.deadline = std::chrono::steady_clock::now() + DEEP_OPTIMIZE_SPACE_ANI_CALLBACK_TIMEOUT;
            it->second.lastProgress = progress;
        }
    }

private:
    DeepOptimizeSpaceAniCallbackRegistryImpl()
    {
        watcherState_ = WatcherState::IDLE;
        watcherThread_ = std::thread(&DeepOptimizeSpaceAniCallbackRegistryImpl::WatcherLoop, this);
    }

    ~DeepOptimizeSpaceAniCallbackRegistryImpl()
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

    uint64_t RegisterLocked(const std::shared_ptr<DeepOptimizeSpaceAniCallbackHolder> &holder,
        const sptr<DeepOptimizeSpaceCallbackStub> &callbackStub, const sptr<IRemoteObject> &callbackRemote)
    {
        uint64_t registryId = nextRegistryId_++;
        records_[registryId] = DeepOptimizeSpaceAniRegistryRecord {
            .holder = holder,
            .callbackStub = callbackStub,
            .callbackRemote = callbackRemote,
            .deadline = std::chrono::steady_clock::now() + DEEP_OPTIMIZE_SPACE_ANI_CALLBACK_TIMEOUT
        };
        ANI_INFO_LOG("Registered deep optimize space ani callback, registryId: %{public}" PRIu64, registryId);
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
            std::vector<std::pair<uint64_t, int32_t>> expiredRecords;
            for (auto it = records_.begin(); it != records_.end(); ++it) {
                if (it->second.deadline <= now) {
                    expiredRecords.emplace_back(it->first, it->second.lastProgress);
                }
            }

            for (const auto &[expiredId, lastProgress] : expiredRecords) {
                auto it = records_.find(expiredId);
                if (it != records_.end()) {
                    auto holder = it->second.holder;
                    it = records_.erase(it);
                    lock.unlock();
                    ANI_WARN_LOG("Deep optimize space ani callback timeout, registryId: %{public}" PRIu64
                        ", lastProgress: %{public}d", expiredId, lastProgress);
                    (void)holder->NotifyProgress(DeepOptimizeSpaceState::FAILED, lastProgress, "watchdog_timeout");
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
    std::unordered_map<uint64_t, DeepOptimizeSpaceAniRegistryRecord> records_;
    uint64_t nextRegistryId_ = 1;

    enum class WatcherState {
        IDLE,
        RUNNING,
        SHUTDOWN,
    };
    WatcherState watcherState_ = WatcherState::IDLE;
};
} // namespace

DeepOptimizeSpaceAniCallbackHolder::DeepOptimizeSpaceAniCallbackHolder(ani_vm *vm, ani_ref callbackRef)
    : aniVm_(vm), callbackRef_(callbackRef)
{
}

DeepOptimizeSpaceAniCallbackHolder::~DeepOptimizeSpaceAniCallbackHolder()
{
    Release();
}

ani_status DeepOptimizeSpaceAniCallbackHolder::Create(ani_env *env, ani_fn_object callback,
    std::shared_ptr<DeepOptimizeSpaceAniCallbackHolder> &holder)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");

    ani_vm *vm = nullptr;
    CHECK_STATUS_RET(env->GetVM(&vm), "Failed to get VM");

    ani_ref callbackRef = nullptr;
    CHECK_STATUS_RET(env->GlobalReference_Create(callback, &callbackRef), "Failed to create global reference");
    
    holder = std::shared_ptr<DeepOptimizeSpaceAniCallbackHolder>(
        new (std::nothrow) DeepOptimizeSpaceAniCallbackHolder(vm, callbackRef));
    if (holder == nullptr) {
        ANI_ERR_LOG("Failed to allocate deep optimize space ani callback holder");
        env->GlobalReference_Delete(callbackRef);
        return ANI_ERROR;
    }

    return ANI_OK;
}

int32_t DeepOptimizeSpaceAniCallbackHolder::PrepareNotifyProgress(const char *source, uint64_t &registryId)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (released_) {
            return E_OK;
        }
        registryId = registryId_.load();
    }

    if (aniVm_ == nullptr || callbackRef_ == nullptr) {
        ANI_ERR_LOG("Deep optimize space ani callback vm or callbackRef is null, source: %{public}s,"
            " registryId: %{public}" PRIu64, source, registryId);
        return MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR;
    }
    return E_OK;
}

void DeepOptimizeSpaceAniCallbackHolder::Release()
{
    ani_ref callbackRef = nullptr;
    ani_vm *vm = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (released_) {
            return;
        }
        released_ = true;
        callbackRef = callbackRef_;
        callbackRef_ = nullptr;
        vm = aniVm_;
    }
    
    if (vm != nullptr && callbackRef != nullptr) {
        ani_env *env = nullptr;
        if (vm->AttachCurrentThread(nullptr, ANI_VERSION_1, &env) == ANI_OK && env != nullptr) {
            env->GlobalReference_Delete(callbackRef);
            vm->DetachCurrentThread();
        } else {
            ANI_ERR_LOG("Failed to AttachCurrentThread");
        }
    }
}

void DeepOptimizeSpaceAniCallbackHolder::SetRegistryId(uint64_t registryId)
{
    registryId_.store(registryId);
}

void DeepOptimizeSpaceAniCallbackHolder::CleanupRegistry()
{
    uint64_t registryId = registryId_.exchange(0);
    if (registryId == 0) {
        return;
    }
    DeepOptimizeSpaceAniCallbackRegistry::Unregister(registryId);
}

void DeepOptimizeSpaceAniCallbackHolder::CallAniCallback(DeepOptimizeSpaceState state, int32_t progress)
{
    ani_env *env = nullptr;
    ani_option interopEnabled {"--interop=disable", nullptr};
    ani_options aniArgs {1, &interopEnabled};
    AniThreadGuard guard(aniVm_, &aniArgs, &env);
    CHECK_IF_EQUAL(guard.IsAttached(), "AttachCurrentThread fail");

    ani_class objectClass = nullptr;
    static const std::string className = "std.core.Object";
    CHECK_IF_EQUAL(env->FindClass(className.c_str(), &objectClass) == ANI_OK,
        "Failed to find class std.core.Object");

    ani_method ctorMethod = nullptr;
    CHECK_IF_EQUAL(env->Class_FindMethod(objectClass, "<ctor>", nullptr, &ctorMethod) == ANI_OK,
        "Failed to find ctor method");

    ani_object progressObj = nullptr;
    CHECK_IF_EQUAL(env->Object_New(objectClass, ctorMethod, &progressObj) == ANI_OK,
        "Failed to create progress object");

    ani_enum_item stateEnumItem = nullptr;
    CHECK_IF_EQUAL(MediaLibraryEnumAni::ToAniEnum(env, state, stateEnumItem) == ANI_OK,
        "Failed to convert DeepOptimizeSpaceState to ani enum");

    CHECK_IF_EQUAL(env->Object_SetPropertyByName_Ref(progressObj, DEEP_OPTIMIZE_SPACE_STATE_FIELD.c_str(),
        static_cast<ani_ref>(stateEnumItem)) == ANI_OK, "Failed to set state property");

    CHECK_IF_EQUAL(env->Object_SetPropertyByName_Int(progressObj, DEEP_OPTIMIZE_SPACE_PROGRESS_FIELD.c_str(),
        static_cast<ani_int>(progress)) == ANI_OK, "Failed to set progress property");

    ani_fn_object aniCallback = static_cast<ani_fn_object>(callbackRef_);
    std::vector<ani_ref> args = { static_cast<ani_ref>(progressObj) };
    ani_ref returnVal;
    ani_status status = env->FunctionalObject_Call(aniCallback, 1, args.data(), &returnVal);
    if (status != ANI_OK) {
        ANI_ERR_LOG("CallAniCallback FunctionalObject_Call fail, status: %{public}d", status);
    }
}

int32_t DeepOptimizeSpaceAniCallbackHolder::NotifyProgress(DeepOptimizeSpaceState state, int32_t progress,
    const char *source)
{
    uint64_t registryId = 0;
    int32_t prepareRet = PrepareNotifyProgress(source, registryId);
    if (prepareRet != E_OK) {
        Release();
        CleanupRegistry();
        return prepareRet;
    }

    CallAniCallback(state, progress);

    if (state != DeepOptimizeSpaceState::RUNNING) {
        Release();
        CleanupRegistry();
    } else {
        DeepOptimizeSpaceAniCallbackRegistry::UpdateDeadlineAndProgress(registryId, progress);
    }
    return E_OK;
}

DeepOptimizeSpaceAniCallbackStub::DeepOptimizeSpaceAniCallbackStub(
    std::shared_ptr<DeepOptimizeSpaceAniCallbackHolder> holder)
    : holder_(std::move(holder))
{
}

int32_t DeepOptimizeSpaceAniCallbackStub::OnProgressUpdate(const DeepOptimizeSpaceProgress &progress)
{
    if (holder_ == nullptr) {
        ANI_WARN_LOG("Deep optimize space ani callback stub holder is null, skip notification (dummy stub)");
        return E_OK;
    }
    return holder_->NotifyProgress(progress.state, progress.progress, "service_callback");
}

int32_t DeepOptimizeSpaceDummyAniCallbackStub::OnProgressUpdate(const DeepOptimizeSpaceProgress &progress)
{
    ANI_INFO_LOG("Dummy ani stub received progress update, state: %{public}d, progress: %{public}d, "
        "no actual callback to invoke", static_cast<int32_t>(progress.state), progress.progress);
    return E_OK;
}

uint64_t DeepOptimizeSpaceAniCallbackRegistry::Register(
    const std::shared_ptr<DeepOptimizeSpaceAniCallbackHolder> &holder,
    const sptr<DeepOptimizeSpaceCallbackStub> &callbackStub, const sptr<IRemoteObject> &callbackRemote)
{
    return DeepOptimizeSpaceAniCallbackRegistryImpl::GetInstance().Register(holder, callbackStub, callbackRemote);
}

void DeepOptimizeSpaceAniCallbackRegistry::Unregister(uint64_t registryId)
{
    DeepOptimizeSpaceAniCallbackRegistryImpl::GetInstance().Unregister(registryId);
}

void DeepOptimizeSpaceAniCallbackRegistry::UpdateDeadlineAndProgress(uint64_t registryId, int32_t progress)
{
    DeepOptimizeSpaceAniCallbackRegistryImpl::GetInstance().UpdateDeadlineAndProgress(registryId, progress);
}
} // namespace OHOS::Media
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

#ifndef FRAMEWORKS_ANI_SRC_INCLUDE_DEEP_OPTIMIZE_SPACE_ANI_CALLBACK_H
#define FRAMEWORKS_ANI_SRC_INCLUDE_DEEP_OPTIMIZE_SPACE_ANI_CALLBACK_H

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>

#include "ani.h"
#include "deep_optimize_space_callback.h"
#include "iremote_object.h"

namespace OHOS::Media {
class DeepOptimizeSpaceAniCallbackHolder;

class DeepOptimizeSpaceAniCallbackHolder final
    : public std::enable_shared_from_this<DeepOptimizeSpaceAniCallbackHolder> {
public:
    explicit DeepOptimizeSpaceAniCallbackHolder(ani_vm *vm, ani_ref callbackRef);
    ~DeepOptimizeSpaceAniCallbackHolder();

    static ani_status Create(ani_env *env, ani_fn_object callback,
        std::shared_ptr<DeepOptimizeSpaceAniCallbackHolder> &holder);

    int32_t NotifyProgress(DeepOptimizeSpaceState state, int32_t progress, const char *source = "unknown");
    void Release();
    void SetRegistryId(uint64_t registryId);

private:
    int32_t PrepareNotifyProgress(const char *source, uint64_t &registryId);
    void CleanupRegistry();
    void CallAniCallback(DeepOptimizeSpaceState state, int32_t progress);

    std::mutex mutex_;
    bool released_ = false;
    std::atomic<uint64_t> registryId_ {0};
    ani_vm *aniVm_ = nullptr;
    ani_ref callbackRef_ = nullptr;
};

class DeepOptimizeSpaceAniCallbackStub final : public DeepOptimizeSpaceCallbackStub {
public:
    explicit DeepOptimizeSpaceAniCallbackStub(std::shared_ptr<DeepOptimizeSpaceAniCallbackHolder> holder);

    int32_t OnProgressUpdate(const DeepOptimizeSpaceProgress &progress) override;

private:
    std::shared_ptr<DeepOptimizeSpaceAniCallbackHolder> holder_;
};

class DeepOptimizeSpaceDummyAniCallbackStub final : public DeepOptimizeSpaceCallbackStub {
public:
    int32_t OnProgressUpdate(const DeepOptimizeSpaceProgress &progress) override;
};

class DeepOptimizeSpaceAniCallbackRegistry final {
public:
    static uint64_t Register(const std::shared_ptr<DeepOptimizeSpaceAniCallbackHolder> &holder,
        const sptr<DeepOptimizeSpaceCallbackStub> &callbackStub, const sptr<IRemoteObject> &callbackRemote);
    static void Unregister(uint64_t registryId);
    static void UpdateDeadlineAndProgress(uint64_t registryId, int32_t progress);
};
} // namespace OHOS::Media

#endif // FRAMEWORKS_ANI_SRC_INCLUDE_DEEP_OPTIMIZE_SPACE_ANI_CALLBACK_H
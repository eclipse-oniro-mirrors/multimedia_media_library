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

#ifndef FRAMEWORKS_ANI_SRC_INCLUDE_ANALYSIS_TOOL_ANI_CALLBACK_HOLDER_H
#define FRAMEWORKS_ANI_SRC_INCLUDE_ANALYSIS_TOOL_ANI_CALLBACK_HOLDER_H

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>

#include "ani.h"
#include "iremote_object.h"

namespace OHOS::Media {
class AnalysisToolAniCallbackHolder;

enum class PrepareNotifyStatus : int32_t {
    SHOULD_NOTIFY = 0,
    ALREADY_COMPLETED = 1,
    ERROR = -1,
};

enum class AnalysisToolAniSaBindResult : uint8_t {
    BOUND = 0,
    ALREADY_COMPLETED,
    INVALID_REMOTE,
    HOLDER_RELEASED,
    ADD_DEATH_RECIPIENT_FAILED,
};

class AnalysisToolAniSaDeathRecipient final : public IRemoteObject::DeathRecipient {
public:
    explicit AnalysisToolAniSaDeathRecipient(std::weak_ptr<AnalysisToolAniCallbackHolder> holder);
    void OnRemoteDied(const wptr<IRemoteObject> &remote) override;

private:
    std::weak_ptr<AnalysisToolAniCallbackHolder> holder_;
};

class AnalysisToolAniCallbackHolder final
    : public std::enable_shared_from_this<AnalysisToolAniCallbackHolder> {
public:
    explicit AnalysisToolAniCallbackHolder(ani_vm *vm, ani_ref callbackRef);
    ~AnalysisToolAniCallbackHolder();

    static ani_status Create(ani_env *env, ani_fn_object callback,
        std::shared_ptr<AnalysisToolAniCallbackHolder> &holder);

    int32_t NotifyToolResult(int32_t code, const std::string &result, const char *source = "unknown");
    void HandleSaDied();
    AnalysisToolAniSaBindResult BindSaRemote(const sptr<IRemoteObject> &saRemote);
    void Release();
    void SetRegistryId(uint64_t registryId);

private:
    PrepareNotifyStatus PrepareNotifyResult(const char *source, uint64_t &registryId);
    void CallAniCallback(int32_t code, const std::string &result);
    void CleanupRegistry();

    std::mutex mutex_;
    bool released_ = false;
    bool resultReceived_ = false;
    std::atomic<uint64_t> registryId_ {0};
    ani_vm *aniVm_ = nullptr;
    ani_ref callbackRef_ = nullptr;
    sptr<IRemoteObject> saRemote_;
    sptr<IRemoteObject::DeathRecipient> saDeathRecipient_;
};
} // namespace OHOS::Media

#endif // FRAMEWORKS_ANI_SRC_INCLUDE_ANALYSIS_TOOL_ANI_CALLBACK_HOLDER_H

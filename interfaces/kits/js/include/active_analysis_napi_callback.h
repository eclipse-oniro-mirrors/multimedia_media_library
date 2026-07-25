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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_ACTIVE_ANALYSIS_NAPI_CALLBACK_H_
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_ACTIVE_ANALYSIS_NAPI_CALLBACK_H_

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>

#include "active_analysis/active_analysis_callback.h"
#include "iremote_object.h"
#include "napi/native_api.h"

namespace OHOS::Media {
class ActiveAnalysisJsCallbackHolder;

enum class ActiveAnalysisSaBindResult : uint8_t {
    BOUND = 0,
    ALREADY_COMPLETED,
    INVALID_REMOTE,
    HOLDER_RELEASED,
    ADD_DEATH_RECIPIENT_FAILED,
};

class ActiveAnalysisSaDeathRecipient final : public IRemoteObject::DeathRecipient {
public:
    explicit ActiveAnalysisSaDeathRecipient(std::weak_ptr<ActiveAnalysisJsCallbackHolder> holder);

    void OnRemoteDied(const wptr<IRemoteObject> &remote) override;

private:
    std::weak_ptr<ActiveAnalysisJsCallbackHolder> holder_;
};

class ActiveAnalysisJsCallbackHolder final : public std::enable_shared_from_this<ActiveAnalysisJsCallbackHolder> {
public:
    explicit ActiveAnalysisJsCallbackHolder(napi_threadsafe_function threadSafeFunc);
    ~ActiveAnalysisJsCallbackHolder();

    static napi_status Create(
        napi_env env, napi_value callback, std::shared_ptr<ActiveAnalysisJsCallbackHolder> &holder);

    int32_t NotifyResult(int32_t result, const char *source = "unknown");
    int32_t NotifyToolResult(int32_t code, const std::string &result, const char *source = "unknown");

    void HandleSaDied();
    void MarkAsToolCallback();
    ActiveAnalysisSaBindResult BindSaRemote(const sptr<IRemoteObject> &saRemote);
    void Release();
    void SetRegistryId(uint64_t registryId);

private:
    int32_t PrepareNotifyResult(const char *source, napi_threadsafe_function &threadSafeFunc, uint64_t &registryId);
    void MarkResultPostedToJs();
    void CleanupRegistry();

    std::mutex mutex_;
    bool released_ = false;
    bool resultReceived_ = false;
    bool resultPostedToJs_ = false;
    bool isToolCallback_ = false;
    std::atomic<uint64_t> registryId_ {0};
    napi_threadsafe_function threadSafeFunc_ = nullptr;
    sptr<IRemoteObject> saRemote_;
    sptr<IRemoteObject::DeathRecipient> saDeathRecipient_;
};

class ActiveAnalysisJsCallbackStub final : public ActiveAnalysisCallbackStub {
public:
    explicit ActiveAnalysisJsCallbackStub(std::shared_ptr<ActiveAnalysisJsCallbackHolder> holder);

    int32_t OnAnalysisFinished(const ActiveAnalysisCallbackResult &result) override;
    int32_t OnToolFinished(const AnalysisToolCallbackResult &result) override;

private:
    std::shared_ptr<ActiveAnalysisJsCallbackHolder> holder_;
};

class ActiveAnalysisJsCallbackRegistry final {
public:
    static uint64_t Register(const std::shared_ptr<ActiveAnalysisJsCallbackHolder> &holder,
        const sptr<ActiveAnalysisJsCallbackStub> &callbackStub, const sptr<IRemoteObject> &callbackRemote);
    static void Unregister(uint64_t registryId);
};
} // namespace OHOS::Media

#endif // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_ACTIVE_ANALYSIS_NAPI_CALLBACK_H_

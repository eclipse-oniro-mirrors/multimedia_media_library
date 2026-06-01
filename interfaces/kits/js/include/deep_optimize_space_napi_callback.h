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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_DEEP_OPTIMIZE_SPACE_NAPI_CALLBACK_H_
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_DEEP_OPTIMIZE_SPACE_NAPI_CALLBACK_H_

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>

#include "deep_optimize_space_callback.h"
#include "iremote_object.h"
#include "napi/native_api.h"

namespace OHOS::Media {
class DeepOptimizeSpaceJsCallbackHolder;

class DeepOptimizeSpaceJsCallbackHolder final : public std::enable_shared_from_this<DeepOptimizeSpaceJsCallbackHolder> {
public:
    explicit DeepOptimizeSpaceJsCallbackHolder(napi_threadsafe_function threadSafeFunc);
    ~DeepOptimizeSpaceJsCallbackHolder();

    static napi_status Create(
        napi_env env, napi_value callback, std::shared_ptr<DeepOptimizeSpaceJsCallbackHolder> &holder);

    int32_t NotifyProgress(DeepOptimizeSpaceState state, int32_t progress, const char *source = "unknown");
    void Release();
    void SetRegistryId(uint64_t registryId);

private:
    int32_t PrepareNotifyProgress(const char *source, napi_threadsafe_function &threadSafeFunc, uint64_t &registryId);
    void CleanupRegistry();

    std::mutex mutex_;
    bool released_ = false;
    std::atomic<uint64_t> registryId_ {0};
    napi_threadsafe_function threadSafeFunc_ = nullptr;
};

class DeepOptimizeSpaceJsCallbackStub final : public DeepOptimizeSpaceCallbackStub {
public:
    explicit DeepOptimizeSpaceJsCallbackStub(std::shared_ptr<DeepOptimizeSpaceJsCallbackHolder> holder);

    int32_t OnProgressUpdate(const DeepOptimizeSpaceProgress &progress) override;

private:
    std::shared_ptr<DeepOptimizeSpaceJsCallbackHolder> holder_;
};

class DeepOptimizeSpaceDummyJsCallbackStub final : public DeepOptimizeSpaceCallbackStub {
public:
    int32_t OnProgressUpdate(const DeepOptimizeSpaceProgress &progress) override;
};

class DeepOptimizeSpaceJsCallbackRegistry final {
public:
    static uint64_t Register(const std::shared_ptr<DeepOptimizeSpaceJsCallbackHolder> &holder,
        const sptr<DeepOptimizeSpaceCallbackStub> &callbackStub, const sptr<IRemoteObject> &callbackRemote);
    static void Unregister(uint64_t registryId);
    static void UpdateDeadlineAndProgress(uint64_t registryId, int32_t progress);
};
} // namespace OHOS::Media

#endif // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_DEEP_OPTIMIZE_SPACE_NAPI_CALLBACK_H_
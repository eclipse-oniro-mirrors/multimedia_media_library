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

#include "local_enhancement_task_state_napi.h"

#include "medialibrary_client_errno.h"
#include "medialibrary_napi_utils.h"
#include "medialibrary_errno.h"

using namespace std;

namespace OHOS {
namespace Media {

static const string LOCAL_ENHANCEMENT_TASK_STATE_CLASS = "LocalEnhancementTaskState";
thread_local napi_ref LocalEnhancementTaskStateNapi::constructor_ = nullptr;

napi_value LocalEnhancementTaskStateNapi::Init(napi_env env, napi_value exports)
{
    NapiClassInfo info = {
        .name = LOCAL_ENHANCEMENT_TASK_STATE_CLASS,
        .ref = &constructor_,
        .constructor = Constructor,
        .props = {
            DECLARE_NAPI_GETTER("progress", JSGetProgress),
        }
    };
    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);
    return exports;
}

napi_value LocalEnhancementTaskStateNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value newTarget = nullptr;
    CHECK_ARGS(env, napi_get_new_target(env, info, &newTarget), JS_INNER_FAIL);
    CHECK_COND_RET(newTarget != nullptr, nullptr, "Invalid call to constructor");

    size_t argc = ARGS_ZERO;
    napi_value thisVar = nullptr;
    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, nullptr, &thisVar, nullptr), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, argc == ARGS_ZERO, "Number of args is invalid");

    unique_ptr<LocalEnhancementTaskStateNapi> obj = make_unique<LocalEnhancementTaskStateNapi>();
    CHECK_ARGS(env,
        napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()), LocalEnhancementTaskStateNapi::Destructor,
        nullptr, nullptr), JS_INNER_FAIL);
    obj.release();
    return thisVar;
}

void LocalEnhancementTaskStateNapi::Destructor(napi_env env, void* nativeObject, void* finalizeHint)
{
    auto* localEnhancementTaskStateNapi = reinterpret_cast<LocalEnhancementTaskStateNapi*>(nativeObject);
    if (localEnhancementTaskStateNapi == nullptr) {
        return;
    }
    delete localEnhancementTaskStateNapi;
    localEnhancementTaskStateNapi = nullptr;
}

int32_t LocalEnhancementTaskStateNapi::GetProgress() const
{
    return this->progress_;
}

void LocalEnhancementTaskStateNapi::SetProgress(int32_t progress)
{
    this->progress_ = progress;
}

napi_value LocalEnhancementTaskStateNapi::NewLocalEnhancementTaskStateNapi(napi_env env,
    CloudEnhancementAsyncContext* context)
{
    CHECK_COND_RET(context != nullptr, nullptr, "Invalid context");
    napi_value constructor = nullptr;
    napi_value instance = nullptr;
    napi_status status = napi_get_reference_value(env, constructor_, &constructor);
    CHECK_COND_RET(status == napi_ok, nullptr, "Failed to get reference of constructor, napi status: %{public}d",
        static_cast<int>(status));
    status = napi_new_instance(env, constructor, 0, nullptr, &instance);
    CHECK_COND_RET(status == napi_ok, nullptr,
        "Failed to get new instance of local enhancement task state, napi status: %{public}d",
        static_cast<int>(status));
    CHECK_COND_RET(instance != nullptr, nullptr, "Instance is nullptr");

    LocalEnhancementTaskStateNapi* localEnhancementTaskStateNapi = nullptr;
    status = napi_unwrap(env, instance, reinterpret_cast<void**>(&localEnhancementTaskStateNapi));
    CHECK_COND_RET(status == napi_ok, nullptr, "Failed to unwarp instance of LocalEnhancementTaskStateNapi");
    CHECK_COND_RET(localEnhancementTaskStateNapi != nullptr, nullptr, "localEnhancementTaskStateNapi is nullptr");
    localEnhancementTaskStateNapi->SetProgress(context->localEnhancementProgress_);
    return instance;
}

napi_value LocalEnhancementTaskStateNapi::JSGetProgress(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value thisVar = nullptr;
    napi_value result = nullptr;
    LocalEnhancementTaskStateNapi* obj = nullptr;

    napi_get_undefined(env, &result);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return result;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&obj));
    if (status == napi_ok && obj != nullptr) {
        int32_t progress = obj->GetProgress();
        if (progress != UNDEFINED) {
            napi_create_int32(env, progress, &result);
        }
    } else {
        NAPI_ERR_LOG("Failed to get progress");
    }
    return result;
}

} // namespace Media
} // namespace OHOS
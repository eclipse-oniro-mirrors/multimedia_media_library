/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORKS_JS_NAPI_COMMON_MEDIA_NAPI_UTILS_
#define FRAMEWORKS_JS_NAPI_COMMON_MEDIA_NAPI_UTILS_

#include "media_napi_utils.h"

#include "ability.h"
#include "napi_base_context.h"

#include "medialibrary_napi_log.h"
#include "medialibrary_napi_utils.h"

namespace OHOS {
namespace Media {
shared_ptr<DataShare::DataShareHelper> MediaNapiUtils::InitNapiToken(napi_env env, napi_callback_info info)
{
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr));

    bool isStageMode = false;
    napi_status status = OHOS::AbilityRuntime::IsStageContext(env, argv[0], isStageMode);
    if (status != napi_ok || !isStageMode) {
        NAPI_INFO_LOG("status: %{public}d, isStageMode: %{public}d", status, static_cast<int32_t>(isStageMode));
        auto ability = OHOS::AbilityRuntime::GetCurrentAbility(env);
        if (ability == nullptr) {
            NAPI_ERR_LOG("Failed to get native ability instance");
            return nullptr;
        }
        auto context = ability->GetContext();
        if (context == nullptr) {
            NAPI_ERR_LOG("Failed to get native context instance");
            return nullptr;
        }
        return context->GetToken();
    } else {
        auto context = OHOS::AbilityRuntime::GetStageModeContext(env, argv[0]);
        if (context == nullptr) {
            NAPI_ERR_LOG("Failed to get native stage context instance");
            return nullptr;
        }
        return context->GetToken();
    }
}

napi_status MediaNapiUtils::CheckIsStage(napi_env env, napi_callback_info info, bool &result)
{
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Failed to get cb info, status=%{public}d", (int) status);
        return status;
    }

    result = false;
    status = OHOS::AbilityRuntime::IsStageContext(env, argv[0], result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Failed to get stage mode, status=%{public}d", (int) status);
        return status;
    }
    return napi_ok;
}

sptr<IRemoteObject> MediaNapiUtils::ParseTokenInStageMode(napi_env env, napi_callback_info info)
{
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    if (napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr) != napi_ok) {
        NAPI_ERR_LOG("Failed to get cb info");
        return nullptr;
    }

    auto context = OHOS::AbilityRuntime::GetStageModeContext(env, argv[0]);
    if (context == nullptr) {
        NAPI_ERR_LOG("Failed to get native stage context instance");
        return nullptr;
    }
    return context->GetToken();
}

sptr<IRemoteObject> MediaNapiUtils::ParseTokenInAbility(napi_env env, napi_callback_info info)
{
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    if (napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr) != napi_ok) {
        NAPI_ERR_LOG("Failed to get cb info");
        return nullptr;
    }

    auto ability = OHOS::AbilityRuntime::GetCurrentAbility(env);
    if (ability == nullptr) {
        NAPI_ERR_LOG("Failed to get native ability instance");
        return nullptr;
    }
    auto context = ability->GetContext();
    if (context == nullptr) {
        NAPI_ERR_LOG("Failed to get native context instance");
        return nullptr;
    }
    return context->GetToken();
}
} // namespace Media
} // namespace OHOS

#endif  // FRAMEWORKS_JS_NAPI_COMMON_MEDIA_NAPI_UTILS_
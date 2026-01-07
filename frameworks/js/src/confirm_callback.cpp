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
#include "confirm_callback.h"

#include "media_library_napi.h"
#include "medialibrary_napi_utils.h"
#include "medialibrary_napi_log.h"

namespace OHOS {
namespace Media {
namespace {
static constexpr int32_t CONFIRM_CODE_SUCCESS = 0;
static constexpr int32_t CONFIRM_CODE_USER_DENY = -1;
static const std::string CONFIRM_BOX_DES_FILE_URIS = "desFileUris";
static const std::string RESULT_PARAM = "result";
static const std::string DATA_PARAM = "data";
}
#ifdef HAS_ACE_ENGINE_PART
ConfirmCallback::ConfirmCallback(napi_env env, Ace::UIContent *uiContent)
{
    this->env_ = env;
    this->uiContent = uiContent;
}
#else
ConfirmCallback::ConfirmCallback(napi_env env)
{
    this->env_ = env;
}
#endif

ConfirmCallback::~ConfirmCallback()
{
    if (callbackRef != nullptr && env_ != nullptr) {
        napi_delete_reference(env_, callbackRef);
    }
}

void ConfirmCallback::OnRelease(int32_t releaseCode)
{
    NAPI_INFO_LOG("ReleaseCode is %{public}d.", releaseCode);

    CloseModalUIExtension();
}

void ConfirmCallback::OnResult(int32_t resultCode, const OHOS::AAFwk::Want &want)
{
    NAPI_INFO_LOG("ResultCode is %{public}d.", resultCode);

    this->resultCode_ = resultCode;

    std::vector<std::string> desFileUris;
    if (resultCode == CONFIRM_CODE_SUCCESS) {
        // check if the desFileUris exsit
        if (!want.HasParameter(CONFIRM_BOX_DES_FILE_URIS)) {
            NAPI_ERR_LOG("Can't get string array from want.");
            CHECK_ARGS_RET_VOID(this->env_, true, JS_INNER_FAIL);
            return;
        }

        // get desFileUris from want
        desFileUris = want.GetStringArrayParam(CONFIRM_BOX_DES_FILE_URIS);
    } else if (resultCode == CONFIRM_CODE_USER_DENY) {
        this->resultCode_ = CONFIRM_CODE_SUCCESS; // user deny return success with empty uris
    }

    SendMessageBack(desFileUris);
}

void ConfirmCallback::OnError(int32_t code, const std::string &name, const std::string &message)
{
    NAPI_INFO_LOG("Code is %{public}d, name is %{public}s, message is %{public}s.", code, name.c_str(),
        message.c_str());

    this->resultCode_ = JS_INNER_FAIL;
    std::vector<std::string> desFileUris;
    SendMessageBack(desFileUris);
}

void ConfirmCallback::OnReceive(const OHOS::AAFwk::WantParams &request)
{
    NAPI_INFO_LOG("Called.");
}

void ConfirmCallback::SetSessionId(int32_t sessionId)
{
    this->sessionId_ = sessionId;
}

void ConfirmCallback::SetFunc(napi_value func)
{
    napi_valuetype valueType = napi_undefined;
    napi_typeof(this->env_, func, &valueType);
    if (valueType == napi_function) {
        napi_create_reference(this->env_, func, ARGS_ONE, &this->callbackRef);
    }
}

static void GenerateStringArrayValue(napi_env &env, const std::vector<std::string> &desFileUris, const size_t len,
    napi_value &value)
{
    if (len == 0) {
        return;
    }

    CHECK_ARGS_RET_VOID(env, napi_create_array_with_length(env, len, &value), JS_INNER_FAIL); // set array length

    for (size_t i = 0; i < len; ++i) {
        // create string value and set it as array element
        napi_value element = nullptr;
        CHECK_ARGS_RET_VOID(env, napi_create_string_utf8(env, desFileUris[i].c_str(), NAPI_AUTO_LENGTH, &element),
            JS_INNER_FAIL);

        if ((element == nullptr) || (napi_set_element(env, value, i, element) != napi_ok)) {
            NAPI_ERR_LOG("Failed to set element %{public}s to array.", desFileUris[i].c_str());
            break;
        }
    }
}

void ConfirmCallback::SendMessageBack(const std::vector<std::string> &desFileUris)
{
    CloseModalUIExtension();

    NapiScopeHandler scopeHandler(this->env_);
    CHECK_IF_EQUAL(scopeHandler.IsValid(), "scopeHandler is invalid");

    napi_value undefined = nullptr;
    CHECK_ARGS_RET_VOID(this->env_, napi_get_undefined(this->env_, &undefined), JS_INNER_FAIL);

    napi_value results[ARGS_ONE] = {nullptr};
    CHECK_ARGS_RET_VOID(this->env_, napi_create_object(this->env_, &results[PARAM0]), JS_INNER_FAIL);

    // create int32_t value bind result code as first napi value
    napi_value result = nullptr;
    CHECK_ARGS_RET_VOID(this->env_, napi_create_int32(this->env_, this->resultCode_, &result),
        JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(this->env_, napi_set_named_property(this->env_, results[PARAM0], RESULT_PARAM.c_str(), result),
        JS_INNER_FAIL);

    size_t len = desFileUris.size();
    if (len > 0) {
        // transfer desFileUris to a napi value as second param
        napi_value data = nullptr;
        GenerateStringArrayValue(this->env_, desFileUris, len, data);

        CHECK_ARGS_RET_VOID(this->env_, napi_set_named_property(this->env_, results[PARAM0], DATA_PARAM.c_str(), data),
            JS_INNER_FAIL);
    }

    napi_value callback = nullptr;
    CHECK_ARGS_RET_VOID(this->env_, napi_get_reference_value(this->env_, this->callbackRef, &callback), JS_INNER_FAIL);

    napi_value returnVal;
    CHECK_ARGS_RET_VOID(this->env_, napi_call_function(this->env_, undefined, callback, ARGS_ONE, results, &returnVal),
        JS_INNER_FAIL);
}

void ConfirmCallback::CloseModalUIExtension()
{
    NAPI_INFO_LOG("Called.");

#ifdef HAS_ACE_ENGINE_PART
    if (this->uiContent != nullptr) {
        uiContent->CloseModalUIExtension(this->sessionId_);
    }
#endif
}
}
}
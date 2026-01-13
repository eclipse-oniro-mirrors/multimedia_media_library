/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include "default_album_name_callback.h"
#include "userfile_client.h"

using namespace std;

namespace OHOS {
namespace Media {
namespace {
static constexpr int32_t DEFAULT_ALBUM_NAME_CODE_SUCCESS = 0;
static const string DEFAULT_ALBUM_NAME_KEY = "defaultAlbumName";
static const string RESULT_PARAM = "result";
static const string DATA_PARAM = "data";
}

DefaultAlbumNameCallback::DefaultAlbumNameCallback(napi_env env, Ace::UIContent *uiContent)
{
    this->env_ = env;
    this->uiContent = uiContent;
}

void DefaultAlbumNameCallback::OnRelease(int32_t releaseCode)
{
    CloseModalUIExtension();
}

void DefaultAlbumNameCallback::OnResult(int32_t resultCode, const OHOS::AAFwk::Want &want)
{
    string defaultAlbumName;
    if (resultCode == DEFAULT_ALBUM_NAME_CODE_SUCCESS) {
        this->resultCode_ = resultCode;
        if (!want.HasParameter(DEFAULT_ALBUM_NAME_KEY)) {
            NAPI_ERR_LOG("Can't get string from want.");
            CHECK_ARGS_RET_VOID(this->env_, true, JS_INNER_FAIL);
            return;
        }
        defaultAlbumName = want.GetStringParam(DEFAULT_ALBUM_NAME_KEY);
    } else {
        this->resultCode_ = JS_INNER_FAIL;
    }
    SendMessageBack(defaultAlbumName);
}

void DefaultAlbumNameCallback::OnError(int32_t code, const std::string &name, const std::string &message)
{
    NAPI_INFO_LOG("Code is %{public}d, name is %{public}s, message is %{public}s.", code, name.c_str(),
        message.c_str());
    this->resultCode_ = JS_INNER_FAIL;
    string defaultAlbumName;
    SendMessageBack(defaultAlbumName);
}

void DefaultAlbumNameCallback::OnReceive(const OHOS::AAFwk::WantParams &request)
{
    NAPI_INFO_LOG("OnReceive enter.");
}

void DefaultAlbumNameCallback::SetSessionId(int32_t sessionId)
{
    this->sessionId_ = sessionId;
}

void DefaultAlbumNameCallback::SetFunc(napi_value func)
{
    napi_valuetype valueType = napi_undefined;
    napi_typeof(this->env_, func, &valueType);
    if (valueType == napi_function) {
        napi_create_reference(this->env_, func, ARGS_ONE, &this->callbackRef);
    }
}

void DefaultAlbumNameCallback::SendMessageBack(const string &defaultAlbumName)
{
    CloseModalUIExtension();

    NapiScopeHandler scopeHandler(this->env_);
    CHECK_IF_EQUAL(scopeHandler.IsValid(), "scopeHandler is invalid");
    
    napi_value undefined;
    CHECK_ARGS_RET_VOID(this->env_, napi_get_undefined(this->env_, &undefined), JS_INNER_FAIL);

    napi_value results[ARGS_ONE] = {nullptr};
    CHECK_ARGS_RET_VOID(this->env_, napi_create_object(this->env_, &results[PARAM0]), JS_INNER_FAIL);

    napi_value result = nullptr;
    CHECK_ARGS_RET_VOID(this->env_, napi_create_int32(this->env_, this->resultCode_, &result),
        JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(this->env_, napi_set_named_property(this->env_, results[PARAM0], RESULT_PARAM.c_str(), result),
        JS_INNER_FAIL);

    napi_value data = nullptr;
    CHECK_ARGS_RET_VOID(this->env_,
        napi_create_string_utf8(this->env_, defaultAlbumName.c_str(), NAPI_AUTO_LENGTH, &data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(this->env_, napi_set_named_property(this->env_, results[PARAM0], DATA_PARAM.c_str(), data),
        JS_INNER_FAIL);

    napi_value callback = nullptr;
    CHECK_ARGS_RET_VOID(this->env_, napi_get_reference_value(this->env_, this->callbackRef, &callback), JS_INNER_FAIL);

    napi_value returnVal;
    CHECK_ARGS_RET_VOID(this->env_, napi_call_function(this->env_, undefined, callback, ARGS_ONE, results, &returnVal),
        JS_INNER_FAIL);
}

void DefaultAlbumNameCallback::CloseModalUIExtension()
{
    if (this->uiContent != nullptr) {
        uiContent->CloseModalUIExtension(this->sessionId_);
    }
}
}
}
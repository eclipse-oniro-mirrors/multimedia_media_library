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
#include "short_term_callback.h"

#include "media_library_napi.h"
#include "medialibrary_napi_utils.h"
#include "medialibrary_napi_log.h"
#include "media_file_utils.h"
#include "userfile_client.h"

using namespace std;

namespace OHOS {
namespace Media {
namespace {
static constexpr int32_t SHORT_TERM_CODE_SUCCESS = 0;
static const string SHORT_TERM_DES_FILE_URIS = "desFileUris";
static const string SHORT_TERM_RESULT_CODE = "saveResultCode";
static const string RESULT_PARAM = "result";
static const string DATA_PARAM = "data";
}

ShortTermCallback::ShortTermCallback(napi_env env, Ace::UIContent *uiContent)
{
    this->env_ = env;
    this->uiContent = uiContent;
}

void ShortTermCallback::OnRelease(int32_t releaseCode)
{
    CloseModalUIExtension();
}

void ShortTermCallback::OnResult(int32_t resultCode, const OHOS::AAFwk::Want &want)
{
    vector<string> desFileUris;
    if (resultCode == SHORT_TERM_CODE_SUCCESS) {
        this->resultCode_ = resultCode;
        if (!want.HasParameter(SHORT_TERM_DES_FILE_URIS)) {
            NAPI_ERR_LOG("Can't get string array from want.");
            CHECK_ARGS_RET_VOID(this->env_, true, JS_INNER_FAIL);
            return;
        }
        desFileUris = want.GetStringArrayParam(SHORT_TERM_DES_FILE_URIS);
    } else {
        this->resultCode_ = want.GetIntParam(SHORT_TERM_RESULT_CODE, JS_INNER_FAIL);
        NAPI_INFO_LOG("ShortTermCallback.OnResult errCode is %{public}d", this->resultCode_);
    }
    size_t len  = desFileUris.size();
    if (len > 0) {
        SendMessageBack(desFileUris[0]);
    } else {
        string desFileUri;
        SendMessageBack(desFileUri);
    }
}

void ShortTermCallback::OnError(int32_t code, const std::string &name, const std::string &message)
{
    NAPI_INFO_LOG("Code is %{public}d, name is %{public}s, message is %{public}s.", code, name.c_str(),
        message.c_str());
    this->resultCode_ = JS_INNER_FAIL;
    string desFileUri;
    SendMessageBack(desFileUri);
}

void ShortTermCallback::OnReceive(const OHOS::AAFwk::WantParams &request)
{
    NAPI_INFO_LOG("OnReceive enter.");
}

void ShortTermCallback::SetSessionId(int32_t sessionId)
{
    this->sessionId_ = sessionId;
}

void ShortTermCallback::SetFunc(napi_value func)
{
    napi_valuetype valueType = napi_undefined;
    napi_typeof(this->env_, func, &valueType);
    if (valueType == napi_function) {
        napi_create_reference(this->env_, func, ARGS_ONE, &this->callbackRef);
    }
}

void ShortTermCallback::SendMessageBack(const string &desFileUri)
{
    CloseModalUIExtension();

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
    CHECK_ARGS_RET_VOID(this->env_, napi_create_string_utf8(this->env_, desFileUri.c_str(), NAPI_AUTO_LENGTH, &data),
        JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(this->env_, napi_set_named_property(this->env_, results[PARAM0], DATA_PARAM.c_str(), data),
        JS_INNER_FAIL);
    
    napi_value callback = nullptr;
    CHECK_ARGS_RET_VOID(this->env_, napi_get_reference_value(this->env_, this->callbackRef, &callback), JS_INNER_FAIL);

    napi_value returnVal;
    CHECK_ARGS_RET_VOID(this->env_, napi_call_function(this->env_, undefined, callback, ARGS_ONE, results, &returnVal),
        JS_INNER_FAIL);
}

void ShortTermCallback::CloseModalUIExtension()
{
    if (this->uiContent != nullptr) {
        uiContent->CloseModalUIExtension(this->sessionId_);
    }
}
}
}
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
#include "request_photo_uris_read_permission_callback.h"

#include "media_library_napi.h"
#include "medialibrary_napi_utils.h"
#include "medialibrary_napi_log.h"
#include "media_file_utils.h"
#include "userfile_client.h"

using namespace std;

namespace OHOS {
namespace Media {
namespace {
static constexpr int32_t REQUEST_SUCCESS = 0;
static const string REQUEST_PHOTO_URIS_DES_FILE_URIS = "desFileUris";
static const string REQUEST_PHOTO_URIS_INVALID_URIS = "invalidUris";
static const string RESULT_PARAM = "result";
static const string DATA_PARAM = "data";
}

RequestPhotoUrisReadPermissionCallback::RequestPhotoUrisReadPermissionCallback(napi_env env,
    Ace::UIContent *uiContent)
{
    this->env_ = env;
    this->uiContent = uiContent;
}

RequestPhotoUrisReadPermissionCallback::~RequestPhotoUrisReadPermissionCallback()
{
    napi_delete_reference(this->env_, this->callbackRef);
    this->env_ = nullptr;
    this->callbackRef = nullptr;
    this->uiContent = nullptr;
}

void RequestPhotoUrisReadPermissionCallback::OnRelease(int32_t releaseCode)
{
    NAPI_INFO_LOG("ReleaseCode is %{public}d.", releaseCode);
    CloseModalUIExtension();
}

void RequestPhotoUrisReadPermissionCallback::OnResult(int32_t resultCode, const OHOS::AAFwk::Want &want)
{
    NAPI_INFO_LOG("ResultCode is %{public}d.", resultCode);

    std::vector<std::string> desFileUris;
    if (resultCode == REQUEST_SUCCESS) {
        this->resultCode_ = resultCode;

        // check if the desFileUris exsit
        if (!want.HasParameter(REQUEST_PHOTO_URIS_DES_FILE_URIS)) {
            NAPI_ERR_LOG("Can't get string array from want.");
            CHECK_ARGS_RET_VOID(this->env_, true, JS_INNER_FAIL);
            return;
        }

        // get desFileUris from want
        desFileUris = want.GetStringArrayParam(REQUEST_PHOTO_URIS_DES_FILE_URIS);
        for (std::string mem : desFileUris) {
            NAPI_INFO_LOG("mem %{public}s", mem.c_str());
        }
    } else {
        NAPI_INFO_LOG("ResultCode is %{public}d.", resultCode);
        this->resultCode_ = JS_INNER_FAIL;
    }

    SendMessageBack(desFileUris);
}

void RequestPhotoUrisReadPermissionCallback::OnResultEx(int32_t resultCode, const OHOS::AAFwk::Want &want)
{
    NAPI_INFO_LOG("ResultCode is %{public}d.", resultCode);
 
    std::vector<std::string> desFileUris;
    std::vector<std::string> invalidUris;
    if (resultCode == REQUEST_SUCCESS) {
        this->resultCode_ = resultCode;
 
        // check if the desFileUris exsit
        if (!want.HasParameter(REQUEST_PHOTO_URIS_DES_FILE_URIS)) {
            NAPI_ERR_LOG("Can't get string array from want.");
            CHECK_ARGS_RET_VOID(this->env_, true, JS_INNER_FAIL);
            return;
        }
         // get desFileUris from want
        desFileUris = want.GetStringArrayParam(REQUEST_PHOTO_URIS_DES_FILE_URIS);
 
        // check if the invalidUris exsit
        if (want.HasParameter(REQUEST_PHOTO_URIS_INVALID_URIS)) {
            // get invalidUris from want
            invalidUris = want.GetStringArrayParam(REQUEST_PHOTO_URIS_INVALID_URIS);
        }
        for (std::string mem : desFileUris) {
            NAPI_INFO_LOG("mem %{public}s", mem.c_str());
        }
        for (std::string mem : invalidUris) {
            NAPI_INFO_LOG("mem %{public}s", mem.c_str());
        }
    } else {
        NAPI_INFO_LOG("ResultCode is %{public}d.", resultCode);
        this->resultCode_ = JS_INNER_FAIL;
    }
 
    SendMessageBackEx(desFileUris, invalidUris);
}

void RequestPhotoUrisReadPermissionCallback::OnError(int32_t code, const std::string &name,
    const std::string &message)
{
    NAPI_INFO_LOG("Code is %{public}d, name is %{public}s, message is %{public}s.", code, name.c_str(),
        message.c_str());

    this->resultCode_ = JS_INNER_FAIL;
    std::vector<std::string> desFileUris;
    SendMessageBack(desFileUris);
}

void RequestPhotoUrisReadPermissionCallback::OnErrorEx(int32_t code, const std::string &name,
    const std::string &message)
{
    NAPI_INFO_LOG("Code is %{public}d, name is %{public}s, message is %{public}s.", code, name.c_str(),
        message.c_str());
 
    this->resultCode_ = JS_INNER_FAIL;
    std::vector<std::string> desFileUris;
    std::vector<std::string> invalidUris;
    SendMessageBackEx(desFileUris, invalidUris);
}

void RequestPhotoUrisReadPermissionCallback::OnReceive(const OHOS::AAFwk::WantParams &request)
{
    NAPI_INFO_LOG("RequestPhotoUrisReadPermissionCallback Called.");
}

void RequestPhotoUrisReadPermissionCallback::SetSessionId(int32_t sessionId)
{
    this->sessionId_ = sessionId;
}

void RequestPhotoUrisReadPermissionCallback::SetFunc(napi_value func)
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

void RequestPhotoUrisReadPermissionCallback::SendMessageBack(const std::vector<std::string> &desFileUris)
{
    NAPI_INFO_LOG("SendMessageBack enter.");
    CloseModalUIExtension();

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
    } else {
        NAPI_ERR_LOG("desFileUris size is zero.");
    }

    napi_value callback = nullptr;
    CHECK_ARGS_RET_VOID(this->env_, napi_get_reference_value(this->env_, this->callbackRef, &callback), JS_INNER_FAIL);

    napi_value returnVal;
    CHECK_ARGS_RET_VOID(this->env_, napi_call_function(this->env_, undefined, callback, ARGS_ONE, results, &returnVal),
        JS_INNER_FAIL);
}

void RequestPhotoUrisReadPermissionCallback::SendMessageBackEx(
    const std::vector<std::string> &FileUris, const std::vector<std::string> &invalidUris)
{
    NAPI_INFO_LOG("SendMessageBack enter.");
    CloseModalUIExtension();
 
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
 
    napi_value resultObject = nullptr;
    CHECK_ARGS_RET_VOID(this->env_, napi_create_object(this->env_, &resultObject), JS_INNER_FAIL);
 
    size_t len = FileUris.size();
    if (len > 0) {
        napi_value authorizedUrisValue = nullptr;
        GenerateStringArrayValue(this->env_, FileUris, len, authorizedUrisValue);
 
        CHECK_ARGS_RET_VOID(this->env_,
                            napi_set_named_property(this->env_, resultObject, "authorizedUris", authorizedUrisValue),
                            JS_INNER_FAIL);
    } else {
        NAPI_ERR_LOG("authorizedUris size is zero.");
    }
 
    size_t lenInvaildUris = invalidUris.size();
    if (lenInvaildUris > 0) {
        napi_value invalidUrisValue  = nullptr;
        GenerateStringArrayValue(this->env_, invalidUris, lenInvaildUris, invalidUrisValue);
 
        CHECK_ARGS_RET_VOID(this->env_,
                            napi_set_named_property(this->env_, resultObject, "invalidUris", invalidUrisValue),
                            JS_INNER_FAIL);
    } else {
        NAPI_ERR_LOG("invalidUris size is zero.");
    }
 
    CHECK_ARGS_RET_VOID(this->env_, napi_set_named_property(this->env_, results[PARAM0],
        DATA_PARAM.c_str(), resultObject), JS_INNER_FAIL);
 
    napi_value callback = nullptr;
    CHECK_ARGS_RET_VOID(this->env_, napi_get_reference_value(this->env_, this->callbackRef, &callback), JS_INNER_FAIL);
 
    napi_value returnVal;
    CHECK_ARGS_RET_VOID(this->env_, napi_call_function(this->env_, undefined, callback, ARGS_ONE, results, &returnVal),
        JS_INNER_FAIL);
}

void RequestPhotoUrisReadPermissionCallback::CloseModalUIExtension()
{
    NAPI_INFO_LOG("Called.");

    if (this->uiContent != nullptr) {
        uiContent->CloseModalUIExtension(this->sessionId_);
    }
}
}
}
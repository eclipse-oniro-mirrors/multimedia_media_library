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

#include "member_info_napi.h"

#include "medialibrary_client_errno.h"
#include "medialibrary_napi_log.h"
#include "big_integer.h"
#include "asset_value.h"

using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

namespace OHOS {
namespace Media {
using namespace std;
using namespace OHOS::NativeRdb;

static const std::string MEMBER_INFO_NAPI_CLASS_NAME = "MemberInfo";
thread_local napi_ref MemberInfoNapi::constructor_ = nullptr;

static MemberInfoNapi *GetMemberInfoNapi(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value thisVar = nullptr;
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    CHECK_WITH_INT_ERR_MESSAGE(env, status == napi_ok, JS_E_INNER_FAIL, "GET_JS_OBJ_WITH_ZERO_ARGS failed");
    MemberInfoNapi *obj = nullptr;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    CHECK_WITH_INT_ERR_MESSAGE(env, status == napi_ok && obj != nullptr, JS_E_INNER_FAIL, "NAPI_unwrap failed");
    return obj;
}

MemberInfoNapi::MemberInfoNapi(): env_(nullptr)
{
}

MemberInfoNapi::~MemberInfoNapi()
{
}


napi_value MemberInfoNapi::Init(napi_env env, napi_value exports)
{
    NapiClassInfo info = {
        .name = MEMBER_INFO_NAPI_CLASS_NAME,
        .ref = &constructor_,
        .constructor = MemberInfoNapiConstructor,
        .props = {
            // 属性
            DECLARE_NAPI_GETTER_SETTER("status", JSGetStatus, JSSetStatus),
            DECLARE_NAPI_GETTER_SETTER("member", JSGetMember, JSSetMember),
        }};
    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);
    return exports;
}


napi_value MemberInfoNapi::CreateMemberInfoNapi(napi_env env, std::shared_ptr<MemberInfo> &memberInfo)
{
    if (memberInfo == nullptr) {
        return nullptr;
    }
    napi_value constructor = nullptr;
    napi_value result = nullptr;

    CHECK_ARGS_WITH_INT(env, napi_get_reference_value(env, constructor_, &constructor), JS_E_INNER_FAIL);
    CHECK_ARGS_WITH_INT(env, napi_new_instance(env, constructor, 0, nullptr, &result), JS_E_INNER_FAIL);
    CHECK_COND_WITH_INT(env, result != nullptr, JS_E_INNER_FAIL);

    MemberInfoNapi* memberInfoNapi = nullptr;
    CHECK_ARGS_WITH_INT(env, napi_unwrap(env, result, reinterpret_cast<void**>(&memberInfoNapi)), JS_E_INNER_FAIL);
    CHECK_COND_WITH_INT(env, memberInfoNapi != nullptr, JS_E_INNER_FAIL);
    memberInfoNapi->memberInfoPtr = memberInfo;
    return result;
}

void MemberInfoNapi::SetMemberInfoNapiProperties()
{
    memberInfoPtr = std::make_shared<MemberInfo>();
}

// COnstructor callback
napi_value MemberInfoNapi::MemberInfoNapiConstructor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    CHECK_ARGS_WITH_INT(env, napi_get_undefined(env, &result), JS_INNER_FAIL);

    napi_value thisVar = nullptr;
    CHECK_ARGS_WITH_INT(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr), JS_INNER_FAIL);
    if (thisVar == nullptr) {
        NapiError::ThrowErrorWithIntCode(env, JS_ERR_PARAMETER_INVALID);
        return result;
    }

    unique_ptr<MemberInfoNapi> obj = make_unique<MemberInfoNapi>();
    obj->env_ = env;
    obj->SetMemberInfoNapiProperties();
    CHECK_ARGS_WITH_INT(env, napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()),
        MemberInfoNapi::MemberInfoNapiDestructor, nullptr, nullptr), JS_INNER_FAIL);
    obj.release();
    return thisVar;
}

void MemberInfoNapi::MemberInfoNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    auto *memberInfo = reinterpret_cast<MemberInfoNapi*>(nativeObject);
    if (memberInfo != nullptr) {
        delete memberInfo;
        memberInfo = nullptr;
    }
}

napi_value UnwrapMemberInfoObject(napi_env env, napi_callback_info info, MemberInfoNapi** obj)
{
    napi_value result = nullptr;
    CHECK_ARGS_WITH_INT(env, napi_get_undefined(env, &result), JS_INNER_FAIL);

    napi_value thisVar = nullptr;
    CHECK_ARGS_WITH_INT(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr), JS_INNER_FAIL);
    if (thisVar == nullptr) {
        NapiError::ThrowErrorWithIntCode(env, JS_ERR_PARAMETER_INVALID);
        return result;
    }

    CHECK_ARGS_WITH_INT(env, napi_unwrap(env, result, reinterpret_cast<void**>(&obj)), JS_E_INNER_FAIL);
    if (obj == nullptr) {
        NapiError::ThrowErrorWithIntCode(env, JS_ERR_PARAMETER_INVALID);
        return result;
    }
    
    CHECK_ARGS_WITH_INT(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

napi_value MemberInfoNapi::JSGetStatus(napi_env env, napi_callback_info info)
{
    MemberInfoNapi* obj = nullptr;
    CHECK_NULLPTR_RET(UnwrapMemberInfoObject(env, info, &obj));

    napi_value jsResult = nullptr;
    CHECK_ARGS_WITH_INT(env, napi_create_int32(env, obj->GetStatus(), &jsResult), JS_INNER_FAIL);
    return jsResult;
}

napi_value MemberInfoNapi::JSSetStatus(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value undefinedResult = nullptr;
    MemberInfoNapi *obj = nullptr;
    napi_valuetype valueType = napi_undefined;
    int32_t targetVal;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    napi_get_undefined(env, &undefinedResult);

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc == ARGS_ONE, "requires 1 parameter");

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        if (napi_typeof(env, argv[PARAM0], &valueType) != napi_ok || valueType != napi_number) {
            NAPI_ERR_LOG("Invalid arguments type! valueType: %{public}d", valueType);
            return undefinedResult;
        }

        status = napi_get_value_int32(env, argv[PARAM0], &targetVal);
        if (status == napi_ok) {
            CHECK_COND_WITH_INT(env, obj->memberInfoPtr != nullptr, JS_E_PARAM_INVALID);
            obj->memberInfoPtr->SetStatus(targetVal);
        }
    }

    return undefinedResult;
}

napi_value MemberInfoNapi::JSGetMember(napi_env env, napi_callback_info info)
{
    MemberInfoNapi* obj = nullptr;
    CHECK_NULLPTR_RET(UnwrapMemberInfoObject(env, info, &obj));

    napi_value jsResult = nullptr;
    CHECK_ARGS_WITH_INT(env, napi_create_string_utf8(env, obj->GetMember().c_str(), NAPI_AUTO_LENGTH, &jsResult),
        JS_INNER_FAIL);
    return jsResult;
}

napi_value MemberInfoNapi::JSSetMember(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    size_t res = 0;
    char buffer[FILENAME_MAX];
    MemberInfoNapi* obj = nullptr;
    napi_value thisVar = nullptr;
    napi_valuetype valueType = napi_undefined;

    napi_get_undefined(env, &jsResult);
    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc == ARGS_ONE, "requires 1 parameter");
    if (thisVar == nullptr || napi_typeof(env, argv[PARAM0], &valueType) != napi_ok
        || valueType != napi_string) {
        NAPI_ERR_LOG("Invalid arguments type! valueType: %{public}d", valueType);
        return jsResult;
    }

    napi_get_value_string_utf8(env, argv[PARAM0], buffer, FILENAME_MAX, &res);

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        obj->memberInfoPtr->SetMember(std::string(buffer));
    } else {
        NAPI_ERR_LOG("status = %{public}d", status);
    }
    return jsResult;
}

int32_t MemberInfoNapi::GetStatus() const
{
    return memberInfoPtr->GetStatus();
}

const string& MemberInfoNapi::GetMember() const
{
    return memberInfoPtr->GetMember();
}

}
}
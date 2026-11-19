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

#include <vector>

#include "share_album_member_info_napi.h"
#include "member_info_napi.h"

#include "medialibrary_client_errno.h"
#include "medialibrary_napi_log.h"

using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

namespace OHOS {
namespace Media {
using namespace std;
using namespace OHOS::NativeRdb;

thread_local napi_ref ShareAlbumMemberInfoNapi::constructor_ = nullptr;

static napi_value UnwrapShareAlbumMemberInfoObject(napi_env env, napi_callback_info info,
    ShareAlbumMemberInfoNapi** obj);

static ShareAlbumMemberInfoNapi *GetShareAlbumMemberInfoNapi(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value thisVar = nullptr;
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    CHECK_WITH_INT_ERR_MESSAGE(env, status == napi_ok, JS_E_INNER_FAIL, "GET_JS_OBJ_WITH_ZERO_ARGS failed");
    ShareAlbumMemberInfoNapi *obj = nullptr;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    CHECK_WITH_INT_ERR_MESSAGE(env, status == napi_ok && obj != nullptr, JS_E_INNER_FAIL, "NAPI_unwrap failed");
    return obj;
}

ShareAlbumMemberInfoNapi::ShareAlbumMemberInfoNapi() : env_(nullptr), memberInfosArrayRef_(nullptr)
{
    shareAlbumMemberInfoPtr_ = std::make_shared<ShareAlbumMemberInfo>();
}

ShareAlbumMemberInfoNapi::~ShareAlbumMemberInfoNapi()
{
}

napi_value ShareAlbumMemberInfoNapi::Init(napi_env env, napi_value exports)
{
    NapiClassInfo info = {
        .name = SHARE_ALBUM_MEMBER_INFO_NAPI_CLASS_NAME,
        .ref = &constructor_,
        .constructor = ShareAlbumMemberInfoNapiConstructor,
        .props = {
            DECLARE_NAPI_GETTER_SETTER("memberInfos", JSGetMemberInfos, JSSetMemberInfos),
            DECLARE_NAPI_GETTER_SETTER("shareAlbumOwner", JSGetShareAlbumOwner, JSSetShareAlbumOwner),
        }
    };

    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);
    return exports;
}

napi_value ShareAlbumMemberInfoNapi::CreatShareAlbumMemberInfoNapi(napi_env env,
    vector<shared_ptr<MemberInfo>>& memberInfos, const string& shareAlbumOwner)
{
    napi_value constructor = nullptr;
    napi_value result = nullptr;
    CHECK_ARGS_WITH_INT(env, napi_get_reference_value(env, constructor_, &constructor), JS_INNER_FAIL);
    CHECK_ARGS_WITH_INT(env, napi_new_instance(env, constructor, 0, nullptr, &result), JS_INNER_FAIL);
    CHECK_COND_WITH_INT(env, result != nullptr, JS_INNER_FAIL);

    ShareAlbumMemberInfoNapi* shareAlbumMemberInfoNapi = nullptr;
    CHECK_ARGS_WITH_INT(env, napi_unwrap(env, result, reinterpret_cast<void**>(&shareAlbumMemberInfoNapi)),
        JS_INNER_FAIL);
    CHECK_COND_WITH_INT(env, shareAlbumMemberInfoNapi != nullptr, JS_INNER_FAIL);
    shareAlbumMemberInfoNapi->shareAlbumMemberInfoPtr_->SetMemberInfos(memberInfos);
    shareAlbumMemberInfoNapi->shareAlbumMemberInfoPtr_->SetShareAlbumOwner(shareAlbumOwner);
    return result;
}

napi_value ShareAlbumMemberInfoNapi::ShareAlbumMemberInfoNapiConstructor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    CHECK_ARGS_WITH_INT(env, napi_get_undefined(env, &result), JS_INNER_FAIL);

    napi_value thisVar = nullptr;
    CHECK_ARGS_WITH_INT(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr), JS_INNER_FAIL);
    if (thisVar == nullptr) {
        NapiError::ThrowErrorWithIntCode(env, JS_ERR_PARAMETER_INVALID);
        return result;
    }

    unique_ptr<ShareAlbumMemberInfoNapi> obj = make_unique<ShareAlbumMemberInfoNapi>();
    obj->env_ = env;
    CHECK_ARGS_WITH_INT(env, napi_wrap(env, thisVar, reinterpret_cast<void *>(obj.get()),
        ShareAlbumMemberInfoNapi::ShareAlbumMemberInfoNapiDestructor, nullptr, nullptr), JS_INNER_FAIL);
    obj.release();
    return thisVar;
}

void ShareAlbumMemberInfoNapi::ShareAlbumMemberInfoNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    auto *shareAlbumMemberInfoNapiObj = reinterpret_cast<ShareAlbumMemberInfoNapi*>(nativeObject);
    if (shareAlbumMemberInfoNapiObj != nullptr) {
        if (shareAlbumMemberInfoNapiObj->memberInfosArrayRef_ != nullptr) {
            napi_delete_reference(env, shareAlbumMemberInfoNapiObj->memberInfosArrayRef_);
            shareAlbumMemberInfoNapiObj->memberInfosArrayRef_ = nullptr;
        }
        delete shareAlbumMemberInfoNapiObj;
        shareAlbumMemberInfoNapiObj = nullptr;
    }
}

napi_value ShareAlbumMemberInfoNapi::JSGetMemberInfos(napi_env env, napi_callback_info info)
{
    ShareAlbumMemberInfoNapi *obj = nullptr;
    CHECK_NULLPTR_RET(UnwrapShareAlbumMemberInfoObject(env, info, &obj));

    if (obj->memberInfosArrayRef_ != nullptr) {
        napi_value cachedArray = nullptr;
        napi_status status = napi_get_reference_value(env, obj->memberInfosArrayRef_, &cachedArray);
        if (status == napi_ok && cachedArray != nullptr) {
            return cachedArray;
        }
    }

    napi_value jsResult = nullptr;
    CHECK_ARGS_WITH_INT(env, napi_create_array(env, &jsResult), JS_INNER_FAIL);

    size_t index = 0;
    for (const auto& memberInfo : obj->shareAlbumMemberInfoPtr_->GetMemberInfos()) {
        auto memberInfoCopy = make_shared<MemberInfo>(*memberInfo);
        napi_value memberInfoNapi = MemberInfoNapi::CreateMemberInfoNapi(env, memberInfoCopy);
        if (memberInfoNapi != nullptr) {
            CHECK_ARGS_WITH_INT(env, napi_set_element(env, jsResult, index, memberInfoNapi), JS_INNER_FAIL);
            index++;
        }
    }

    if (obj->memberInfosArrayRef_ != nullptr) {
        napi_delete_reference(env, obj->memberInfosArrayRef_);
    }
    napi_create_reference(env, jsResult, 1, &obj->memberInfosArrayRef_);

    return jsResult;
}

napi_value ShareAlbumMemberInfoNapi::JSSetMemberInfos(napi_env env, napi_callback_info info)
{
    ShareAlbumMemberInfoNapi *obj = nullptr;
    napi_value undefinedResult = nullptr;
    CHECK_ARGS_WITH_INT(env, napi_get_undefined(env, &undefinedResult), JS_INNER_FAIL);

    CHECK_NULLPTR_RET(UnwrapShareAlbumMemberInfoObject(env, info, &obj));

    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc == ARGS_ONE, "requires 1 parameter");

    bool isArray = false;
    CHECK_ARGS_WITH_INT(env, napi_is_array(env, argv[PARAM0], &isArray), JS_INNER_FAIL);
    if (!isArray) {
        NAPI_ERR_LOG("JSSetMemberInfos: argument is not an array");
        return undefinedResult;
    }

    uint32_t arrayLength = 0;
    CHECK_ARGS_WITH_INT(env, napi_get_array_length(env, argv[PARAM0], &arrayLength), JS_INNER_FAIL);

    if (obj->memberInfosArrayRef_ != nullptr) {
        napi_delete_reference(env, obj->memberInfosArrayRef_);
        obj->memberInfosArrayRef_ = nullptr;
    }

    std::vector<std::shared_ptr<MemberInfo>> memberInfos;
    for (uint32_t i = 0; i < arrayLength; i++) {
        napi_value element = nullptr;
        CHECK_ARGS_WITH_INT(env, napi_get_element(env, argv[PARAM0], i, &element), JS_INNER_FAIL);

        MemberInfoNapi *memberInfoNapi = nullptr;
        CHECK_ARGS_WITH_INT(env, napi_unwrap(env, element, reinterpret_cast<void**>(&memberInfoNapi)), JS_INNER_FAIL);

        if (memberInfoNapi != nullptr) {
            auto memberInfoCopy = std::make_shared<MemberInfo>();
            memberInfoCopy->SetStatus(memberInfoNapi->GetStatus());
            memberInfoCopy->SetMember(memberInfoNapi->GetMember());
            memberInfos.push_back(memberInfoCopy);
        }
    }

    obj->shareAlbumMemberInfoPtr_->SetMemberInfos(memberInfos);

    return undefinedResult;
}

napi_value ShareAlbumMemberInfoNapi::JSGetShareAlbumOwner(napi_env env, napi_callback_info info)
{
    ShareAlbumMemberInfoNapi *obj = nullptr;
    CHECK_NULLPTR_RET(UnwrapShareAlbumMemberInfoObject(env, info, &obj));

    napi_value jsResult = nullptr;
    CHECK_ARGS_WITH_INT(env, napi_create_string_utf8(env, obj->shareAlbumMemberInfoPtr_->GetShareAlbumOwner().c_str(),
        NAPI_AUTO_LENGTH, &jsResult), JS_INNER_FAIL);
    return jsResult;
}

napi_value ShareAlbumMemberInfoNapi::JSSetShareAlbumOwner(napi_env env, napi_callback_info info)
{
    ShareAlbumMemberInfoNapi *obj = nullptr;
    napi_value undefinedResult = nullptr;
    CHECK_ARGS_WITH_INT(env, napi_get_undefined(env, &undefinedResult), JS_INNER_FAIL);

    CHECK_NULLPTR_RET(UnwrapShareAlbumMemberInfoObject(env, info, &obj));

    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc == ARGS_ONE, "requires 1 parameter");

    napi_valuetype valueType = napi_undefined;
    CHECK_ARGS_WITH_INT(env, napi_typeof(env, argv[PARAM0], &valueType), JS_INNER_FAIL);
    if (valueType != napi_string) {
        NAPI_ERR_LOG("JSSetShareAlbumOwner: argument is not an string");
        return undefinedResult;
    }

    char buffer[FILENAME_MAX] = {0};
    size_t res = 0;
    CHECK_ARGS_WITH_INT(env, napi_get_value_string_utf8(env, argv[PARAM0], buffer, FILENAME_MAX, &res), JS_INNER_FAIL);

    obj->shareAlbumMemberInfoPtr_->SetShareAlbumOwner(string(buffer));
    return undefinedResult;
}

std::vector<std::shared_ptr<MemberInfo>> ShareAlbumMemberInfoNapi::GetMemberInfos() const
{
    return shareAlbumMemberInfoPtr_->GetMemberInfos();
}

const std::string& ShareAlbumMemberInfoNapi::GetShareAlbumOwner() const
{
    return shareAlbumMemberInfoPtr_->GetShareAlbumOwner();
}


napi_value UnwrapShareAlbumMemberInfoObject(napi_env env, napi_callback_info info, ShareAlbumMemberInfoNapi** obj)
{
    napi_value result = nullptr;
    CHECK_ARGS_WITH_INT(env, napi_get_undefined(env, &result), JS_INNER_FAIL);

    napi_value thisVar = nullptr;
    CHECK_ARGS_WITH_INT(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr), JS_INNER_FAIL);
    if (thisVar == nullptr) {
        NapiError::ThrowErrorWithIntCode(env, JS_ERR_PARAMETER_INVALID);
        return result;
    }

    CHECK_ARGS_WITH_INT(env, napi_unwrap(env, thisVar, reinterpret_cast<void **>(obj)), JS_INNER_FAIL);
    if (obj == nullptr) {
        NapiError::ThrowErrorWithIntCode(env, JS_ERR_PARAMETER_INVALID);
        return result;
    }

    CHECK_ARGS_WITH_INT(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

} //namespace Media
} //namespace OHOS
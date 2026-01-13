/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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
#define MLOG_TAG "SmartAlbumNapi"

#include "smart_album_napi.h"

#include "media_file_asset_columns.h"
#include "media_library_napi.h"
#include "media_smart_album_column.h"
#include "media_smart_map_column.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_napi_log.h"
#include "medialibrary_tracer.h"
#include "media_file_utils.h"
#include "userfile_client.h"
#include "media_file_uri.h"

using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

namespace OHOS {
namespace Media {
using namespace std;
thread_local napi_ref SmartAlbumNapi::sConstructor_ = nullptr;
thread_local SmartAlbumAsset *SmartAlbumNapi::sAlbumData_ = nullptr;
using CompleteCallback = napi_async_complete_callback;
thread_local napi_ref SmartAlbumNapi::userFileMgrConstructor_ = nullptr;
constexpr int32_t INVALID_EXPIREDTIME = -1;
SmartAlbumNapi::SmartAlbumNapi()
    : env_(nullptr) {}

SmartAlbumNapi::~SmartAlbumNapi() = default;

void SmartAlbumNapi::SmartAlbumNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    SmartAlbumNapi *album = reinterpret_cast<SmartAlbumNapi*>(nativeObject);
    if (album != nullptr) {
        delete album;
        album = nullptr;
    }
}

napi_value SmartAlbumNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value ctorObj;
    int32_t refCount = 1;

    napi_property_descriptor album_props[] = {
        DECLARE_NAPI_GETTER("albumId", JSGetSmartAlbumId),
        DECLARE_NAPI_GETTER("albumUri", JSGetSmartAlbumUri),
        DECLARE_NAPI_GETTER("albumType", JSGetSmartAlbumType),
        DECLARE_NAPI_GETTER_SETTER("albumName", JSGetSmartAlbumName, JSSmartAlbumNameSetter),
        DECLARE_NAPI_GETTER_SETTER("description", JSGetSmartAlbumDescription, JSSmartAlbumDescriptionSetter),
        DECLARE_NAPI_GETTER("albumTag", JSGetSmartAlbumTag),
        DECLARE_NAPI_GETTER("size", JSGetSmartAlbumCapacity),
        DECLARE_NAPI_GETTER("categoryId", JSGetSmartAlbumCategoryId),
        DECLARE_NAPI_GETTER("categoryName", JSGetSmartAlbumCategoryName),
        DECLARE_NAPI_GETTER_SETTER("coverURI", JSGetSmartAlbumCoverUri, JSSmartAlbumCoverUriSetter),
        DECLARE_NAPI_GETTER_SETTER("expiredTime", JSGetSmartAlbumExpiredTime, JSSmartAlbumExpiredTimeSetter),
        DECLARE_NAPI_FUNCTION("commitModify", JSCommitModify),
        DECLARE_NAPI_FUNCTION("addFileAssets", JSAddFileAssets),
        DECLARE_NAPI_FUNCTION("removeFileAssets", JSRemoveFileAssets),
        DECLARE_NAPI_FUNCTION("getFileAssets", JSGetSmartAlbumFileAssets)
    };

    status = napi_define_class(env, SMART_ALBUM_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH,
                               SmartAlbumNapiConstructor, nullptr,
                               sizeof(album_props) / sizeof(album_props[PARAM0]),
                               album_props, &ctorObj);
    if (status == napi_ok) {
        status = napi_create_reference(env, ctorObj, refCount, &sConstructor_);
        if (status == napi_ok) {
            status = napi_set_named_property(env, exports, SMART_ALBUM_NAPI_CLASS_NAME.c_str(), ctorObj);
            if (status == napi_ok) {
                return exports;
            }
        }
    }
    NAPI_DEBUG_LOG("SmartAlbumNapi::Init nullptr, status: %{public}d", status);
    return nullptr;
}

napi_value SmartAlbumNapi::UserFileMgrInit(napi_env env, napi_value exports)
{
    NapiClassInfo info = {
        .name = USERFILEMGR_SMART_ALBUM_NAPI_CLASS_NAME,
        .ref = &userFileMgrConstructor_,
        .constructor = SmartAlbumNapiConstructor,
        .props = {
            DECLARE_NAPI_GETTER_SETTER("albumName", JSGetSmartAlbumName, JSSmartAlbumNameSetter),
            DECLARE_NAPI_GETTER("albumUri", JSGetSmartAlbumUri),
            DECLARE_NAPI_GETTER("dateModified", JSGetSmartAlbumDateModified),
            DECLARE_NAPI_GETTER("count", JSGetSmartAlbumCapacity),
            DECLARE_NAPI_GETTER("coverUri", JSGetSmartAlbumCoverUri),
            DECLARE_NAPI_FUNCTION("getPhotoAssets", UserFileMgrGetAssets),
            DECLARE_NAPI_FUNCTION("delete", UserFileMgrDeleteAsset),
            DECLARE_NAPI_FUNCTION("recover", UserFileMgrRecoverAsset),
        }
    };

    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);
    return exports;
}

void SmartAlbumNapi::SetSmartAlbumNapiProperties()
{
    smartAlbumAssetPtr = std::shared_ptr<SmartAlbumAsset>(sAlbumData_);
    NAPI_INFO_LOG("SetSmartAlbumNapiProperties name = %{public}s",
        smartAlbumAssetPtr->GetAlbumName().c_str());
}

// Constructor callback
napi_value SmartAlbumNapi::SmartAlbumNapiConstructor(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;
    napi_get_undefined(env, &result);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status == napi_ok && thisVar != nullptr) {
        std::unique_ptr<SmartAlbumNapi> obj = std::make_unique<SmartAlbumNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            if (sAlbumData_ != nullptr) {
                obj->SetSmartAlbumNapiProperties();
            }
            status = napi_wrap(env, thisVar, reinterpret_cast<void *>(obj.get()),
                               SmartAlbumNapi::SmartAlbumNapiDestructor, nullptr, nullptr);
            if (status == napi_ok) {
                obj.release();
                return thisVar;
            } else {
                NAPI_ERR_LOG("Failure wrapping js to native napi, status: %{public}d", status);
            }
        }
    }

    return result;
}

napi_value SmartAlbumNapi::CreateSmartAlbumNapi(napi_env env, unique_ptr<SmartAlbumAsset> &albumData)
{
    if (albumData == nullptr) {
        NAPI_ERR_LOG("CreateSmartAlbumNapi albumData is null");
        return nullptr;
    }

    napi_value constructor;
    napi_ref constructorRef = (albumData->GetResultNapiType() == ResultNapiType::TYPE_MEDIALIBRARY) ?
        (sConstructor_) : (userFileMgrConstructor_);
    NAPI_CALL(env, napi_get_reference_value(env, constructorRef, &constructor));

    napi_value result = nullptr;
    sAlbumData_ = albumData.release();
    NAPI_CALL(env, napi_new_instance(env, constructor, 0, nullptr, &result));
    sAlbumData_ = nullptr;
    return result;
}

std::string SmartAlbumNapi::GetSmartAlbumName() const
{
    return smartAlbumAssetPtr->GetAlbumName();
}

int32_t SmartAlbumNapi::GetAlbumPrivateType() const
{
    return smartAlbumAssetPtr->GetAlbumPrivateType();
}

std::string SmartAlbumNapi::GetSmartAlbumUri() const
{
    return smartAlbumAssetPtr->GetAlbumUri();
}

int32_t SmartAlbumNapi::GetSmartAlbumId() const
{
    return smartAlbumAssetPtr->GetAlbumId();
}
std::string SmartAlbumNapi::GetDescription() const
{
    return smartAlbumAssetPtr->GetDescription();
}

std::string SmartAlbumNapi::GetCoverUri() const
{
    return smartAlbumAssetPtr->GetCoverUri();
}

int32_t SmartAlbumNapi::GetExpiredTime() const
{
    return smartAlbumAssetPtr->GetExpiredTime();
}

void SmartAlbumNapi::SetAlbumCapacity(int32_t albumCapacity)
{
    smartAlbumAssetPtr->SetAlbumCapacity(albumCapacity);
}

std::string SmartAlbumNapi::GetNetworkId() const
{
    return MediaFileUtils::GetNetworkIdFromUri(GetSmartAlbumUri());
}

void SmartAlbumNapi::SetCoverUri(string &coverUri)
{
    smartAlbumAssetPtr->SetCoverUri(coverUri);
}

void SmartAlbumNapi::SetDescription(string &description)
{
    smartAlbumAssetPtr->SetDescription(description);
}

void SmartAlbumNapi::SetExpiredTime(int32_t expiredTime)
{
    smartAlbumAssetPtr->SetExpiredTime(expiredTime);
}

napi_value SmartAlbumNapi::JSGetSmartAlbumId(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    SmartAlbumNapi* obj = nullptr;
    int32_t id;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &undefinedResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if ((status != napi_ok) || (thisVar == nullptr)) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if ((status == napi_ok) && (obj != nullptr)) {
        id = obj->GetSmartAlbumId();
        status = napi_create_int32(env, id, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        }
    }

    return undefinedResult;
}

napi_value SmartAlbumNapi::JSGetSmartAlbumName(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    SmartAlbumNapi* obj = nullptr;
    std::string name = "";
    napi_value thisVar = nullptr;
    napi_get_undefined(env, &undefinedResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if ((status != napi_ok) || (thisVar == nullptr)) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return undefinedResult;
    }
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if ((status == napi_ok) && (obj != nullptr)) {
        name = obj->GetSmartAlbumName();
        NAPI_DEBUG_LOG("JSGetSmartAlbumName name = %{private}s", name.c_str());
        status = napi_create_string_utf8(env, name.c_str(), NAPI_AUTO_LENGTH, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        }
    }

    return undefinedResult;
}

napi_value SmartAlbumNapi::JSSmartAlbumNameSetter(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    size_t res = 0;
    char buffer[FILENAME_MAX];
    SmartAlbumNapi* obj = nullptr;
    napi_value thisVar = nullptr;
    napi_valuetype valueType = napi_undefined;

    napi_get_undefined(env, &jsResult);
    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc == ARGS_ONE, "requires 1 parameter");

    if (thisVar == nullptr || napi_typeof(env, argv[PARAM0], &valueType) != napi_ok || valueType != napi_string) {
        NAPI_ERR_LOG("Invalid arguments type! valueType: %{public}d", valueType);
        return jsResult;
    }

    napi_get_value_string_utf8(env, argv[PARAM0], buffer, FILENAME_MAX, &res);

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        obj->smartAlbumAssetPtr->SetAlbumName(std::string(buffer));
    }

    return jsResult;
}

napi_value SmartAlbumNapi::JSGetSmartAlbumTag(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    SmartAlbumNapi* obj = nullptr;
    std::string albumTag = "";
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &undefinedResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if ((status != napi_ok) || (thisVar == nullptr)) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        albumTag = obj->smartAlbumAssetPtr->GetAlbumTag();
        status = napi_create_string_utf8(env, albumTag.c_str(), NAPI_AUTO_LENGTH, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        }
    }

    return undefinedResult;
}

napi_value SmartAlbumNapi::JSGetSmartAlbumCapacity(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    SmartAlbumNapi* obj = nullptr;
    int32_t albumCapacity;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &undefinedResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if ((status != napi_ok) || (thisVar == nullptr)) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        albumCapacity = obj->smartAlbumAssetPtr->GetAlbumCapacity();
        status = napi_create_int32(env, albumCapacity, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        }
    }

    return undefinedResult;
}

napi_value SmartAlbumNapi::JSGetSmartAlbumCategoryId(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    SmartAlbumNapi* obj = nullptr;
    int32_t categoryId;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &undefinedResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if ((status != napi_ok) || (thisVar == nullptr)) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        categoryId = obj->smartAlbumAssetPtr->GetCategoryId();
        status = napi_create_int32(env, categoryId, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        }
    }

    return undefinedResult;
}

napi_value SmartAlbumNapi::JSGetSmartAlbumCategoryName(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    SmartAlbumNapi* obj = nullptr;
    std::string categoryName = "";
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &undefinedResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if ((status != napi_ok) || (thisVar == nullptr)) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        categoryName = obj->smartAlbumAssetPtr->GetCategoryName();
        status = napi_create_string_utf8(env, categoryName.c_str(), NAPI_AUTO_LENGTH, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        }
    }

    return undefinedResult;
}

napi_value SmartAlbumNapi::JSGetSmartAlbumCoverUri(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    SmartAlbumNapi* obj = nullptr;
    std::string coverUri = "";
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &undefinedResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if ((status != napi_ok) || (thisVar == nullptr)) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        coverUri = obj->smartAlbumAssetPtr->GetCoverUri();
        status = napi_create_string_utf8(env, coverUri.c_str(), NAPI_AUTO_LENGTH, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        }
    }

    return undefinedResult;
}

napi_value SmartAlbumNapi::JSSmartAlbumCoverUriSetter(napi_env env, napi_callback_info info)
{
    napi_value jsResult = nullptr;
    napi_get_undefined(env, &jsResult);
    napi_value argv[ARGS_ONE] = {0};
    size_t argc = ARGS_ONE;
    napi_value thisVar = nullptr;
    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc == ARGS_ONE, "requires 1 parameter");
    napi_valuetype valueType = napi_undefined;
    if (thisVar == nullptr || napi_typeof(env, argv[PARAM0], &valueType) != napi_ok || valueType != napi_string) {
        NAPI_ERR_LOG("Invalid arguments type! valueType: %{private}d", valueType);
        return jsResult;
    }
    SmartAlbumNapi* obj = nullptr;
    napi_status status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) || (obj == nullptr)) {
        NAPI_ERR_LOG("Unwrap object failed");
        return jsResult;
    }
    size_t res = 0;
    char buffer[FILENAME_MAX];
    status = napi_get_value_string_utf8(env, argv[PARAM0], buffer, FILENAME_MAX, &res);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Get coverUri value string failed");
        return jsResult;
    }
    obj->smartAlbumAssetPtr->SetCoverUri(std::string(buffer));
    return jsResult;
}

napi_value SmartAlbumNapi::JSGetSmartAlbumUri(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    napi_status status;
    napi_value thisVar = nullptr;
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if ((status != napi_ok) || (thisVar == nullptr)) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return undefinedResult;
    }
    SmartAlbumNapi* obj = nullptr;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) || (obj == nullptr)) {
        NAPI_ERR_LOG("Unwrap object failed");
        return undefinedResult;
    }
    napi_value jsResult = nullptr;
    status = napi_create_string_utf8(env, obj->smartAlbumAssetPtr->GetAlbumUri().c_str(), NAPI_AUTO_LENGTH, &jsResult);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Create albumUri string failed");
        return undefinedResult;
    }
    return jsResult;
}

napi_value SmartAlbumNapi::JSGetSmartAlbumDateModified(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value undefinedResult = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &undefinedResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if ((status != napi_ok) || (thisVar == nullptr)) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return undefinedResult;
    }
    SmartAlbumNapi* obj = nullptr;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if ((status == napi_ok) && (obj != nullptr)) {
        int64_t dateModified = obj->smartAlbumAssetPtr->GetAlbumDateModified();
        napi_value jsResult = nullptr;
        status = napi_create_int64(env, dateModified, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        }
    }
    return undefinedResult;
}

napi_value SmartAlbumNapi::JSGetSmartAlbumType(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    napi_status status;
    napi_value thisVar = nullptr;
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if ((status != napi_ok) || (thisVar == nullptr)) {
        NAPI_ERR_LOG("Invalid arguments! status: %{private}d", status);
        return undefinedResult;
    }
    SmartAlbumNapi* obj = nullptr;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) || (obj == nullptr)) {
        NAPI_ERR_LOG("Unwrap object failed");
        return undefinedResult;
    }
    napi_value jsResult = nullptr;
    status = napi_create_int32(env, obj->smartAlbumAssetPtr->GetAlbumPrivateType(), &jsResult);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Create albumPrivateType int32 failed");
        return undefinedResult;
    }
    return jsResult;
}

napi_value SmartAlbumNapi::JSGetSmartAlbumDescription(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    napi_status status;
    napi_value thisVar = nullptr;
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if ((status != napi_ok) || (thisVar == nullptr)) {
        NAPI_ERR_LOG("Invalid arguments! status: %{private}d", status);
        return undefinedResult;
    }
    SmartAlbumNapi* obj = nullptr;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) || (obj == nullptr)) {
        NAPI_ERR_LOG("Unwrap object failed");
        return undefinedResult;
    }
    napi_value jsResult = nullptr;
    status = napi_create_string_utf8(env, obj->smartAlbumAssetPtr->GetDescription().c_str(), NAPI_AUTO_LENGTH,
        &jsResult);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Create description string failed");
        return undefinedResult;
    }
    return jsResult;
}

napi_value SmartAlbumNapi::JSSmartAlbumDescriptionSetter(napi_env env, napi_callback_info info)
{
    napi_value jsResult = nullptr;
    napi_get_undefined(env, &jsResult);
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc == ARGS_ONE, "requires 1 parameter");
    napi_valuetype valueType = napi_undefined;
    if (thisVar == nullptr || napi_typeof(env, argv[PARAM0], &valueType) != napi_ok || valueType != napi_string) {
        NAPI_ERR_LOG("Invalid arguments type! valueType: %{private}d", valueType);
        return jsResult;
    }
    SmartAlbumNapi* obj = nullptr;
    napi_status status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) || (obj == nullptr)) {
        NAPI_ERR_LOG("Unwrap object failed");
        return jsResult;
    }
    size_t res = 0;
    char buffer[FILENAME_MAX];
    status = napi_get_value_string_utf8(env, argv[PARAM0], buffer, FILENAME_MAX, &res);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Get description value string failed");
        return jsResult;
    }
    obj->smartAlbumAssetPtr->SetDescription(std::string(buffer));
    return jsResult;
}

napi_value SmartAlbumNapi::JSGetSmartAlbumExpiredTime(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    napi_status status;
    napi_value thisVar = nullptr;
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if ((status != napi_ok) || (thisVar == nullptr)) {
        NAPI_ERR_LOG("Invalid arguments! status: %{private}d", status);
        return undefinedResult;
    }
    SmartAlbumNapi* obj = nullptr;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) || (obj == nullptr)) {
        NAPI_ERR_LOG("Unwrap object failed");
        return undefinedResult;
    }
    napi_value jsResult = nullptr;
    status = napi_create_int32(env, obj->smartAlbumAssetPtr->GetExpiredTime(), &jsResult);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Create expiredTime int32 failed");
        return undefinedResult;
    }
    return jsResult;
}

napi_value SmartAlbumNapi::JSSmartAlbumExpiredTimeSetter(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc == ARGS_ONE, "requires 1 parameter");
    SmartAlbumNapi* obj = nullptr;
    napi_status status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) || (obj == nullptr)) {
        NAPI_ERR_LOG("Failed to get expiredTime obj");
        return undefinedResult;
    }
    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, argv[PARAM0], &valueType) != napi_ok || valueType != napi_number) {
        NAPI_ERR_LOG("Invalid arguments type! valueType: %{private}d", valueType);
        obj->smartAlbumAssetPtr->SetExpiredTime(INVALID_EXPIREDTIME);
        return undefinedResult;
    }
    int32_t expiredTime;
    status = napi_get_value_int32(env, argv[PARAM0], &expiredTime);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Failed to get expiredTime");
        return undefinedResult;
    }
    obj->smartAlbumAssetPtr->SetExpiredTime(expiredTime);
    return undefinedResult;
}

static void CommitModifyNative(const SmartAlbumNapiAsyncContext &albumContext)
{
    SmartAlbumNapiAsyncContext *context = const_cast<SmartAlbumNapiAsyncContext *>(&albumContext);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    NAPI_DEBUG_LOG("CommitModifyNative = %{private}s", context->objectInfo->GetSmartAlbumName().c_str());
    if (MediaFileUtils::CheckAlbumName(context->objectInfo->GetSmartAlbumName()) < 0) {
        context->error = JS_E_DISPLAYNAME;
        NAPI_ERR_LOG("Failed to checkDisplayName");
        return;
    }
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(SMARTALBUM_DB_DESCRIPTION, context->objectInfo->GetDescription());
    string coverUri = context->objectInfo->GetCoverUri();
    if (coverUri.empty() || (coverUri.find(MEDIALIBRARY_MEDIA_PREFIX) == string::npos)) {
        context->error = E_VIOLATION_PARAMETERS;
        NAPI_ERR_LOG("CoverUri is invalid");
        return;
    }
    valuesBucket.Put(SMARTALBUM_DB_COVER_URI, coverUri);
    if (context->objectInfo->GetExpiredTime() < 0) {
        context->error = E_VIOLATION_PARAMETERS;
        NAPI_ERR_LOG("ExpiredTime is invalid");
        return;
    }
    valuesBucket.Put(SMARTALBUM_DB_EXPIRED_TIME, context->objectInfo->GetExpiredTime());
    DataShare::DataSharePredicates predicates;
    predicates.SetWhereClause(SMARTALBUM_DB_ID + " = " + std::to_string(context->objectInfo->GetSmartAlbumId()));
    Uri commitModifyUri(MEDIALIBRARY_DATA_URI + "/" + MEDIA_SMARTALBUMOPRN + "/" + MEDIA_SMARTALBUMOPRN_MODIFYALBUM);
    context->changedRows = UserFileClient::Update(commitModifyUri, predicates, valuesBucket);
    context->SaveError(context->changedRows);
}

static void JSAddAssetExecute(SmartAlbumNapiAsyncContext *context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    int32_t smartAlbumId = context->objectInfo->GetSmartAlbumId();
    if ((smartAlbumId == TRASH_ALBUM_ID_VALUES) || (smartAlbumId == FAVOURITE_ALBUM_ID_VALUES)) {
        context->error = E_INVALID_VALUES;
        NAPI_ERR_LOG("SmartAlbumId is invalid, smartAlbumId = %{private}d", smartAlbumId);
        return;
    }
    vector<DataShare::DataShareValuesBucket> values;
    for (int32_t id : context->assetIds) {
        DataShare::DataShareValuesBucket valuesBucket;
        valuesBucket.Put(SMARTALBUMMAP_DB_ALBUM_ID, smartAlbumId);
        valuesBucket.Put(SMARTALBUMMAP_DB_CHILD_ASSET_ID, id);
        values.push_back(valuesBucket);
    }
    Uri addAssetUri(MEDIALIBRARY_DATA_URI + "/" + MEDIA_SMARTALBUMMAPOPRN + "/" +
        MEDIA_SMARTALBUMMAPOPRN_ADDSMARTALBUM);
    context->changedRows = UserFileClient::BatchInsert(addAssetUri, values);
    if (context->changedRows != static_cast<int32_t>(context->assetIds.size())) {
        context->error = E_INVALID_VALUES;
    }
}

static void JSRemoveAssetExecute(SmartAlbumNapiAsyncContext *context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    int32_t smartAlbumId = context->objectInfo->GetSmartAlbumId();
    if ((smartAlbumId == TRASH_ALBUM_ID_VALUES) || (smartAlbumId == FAVOURITE_ALBUM_ID_VALUES)) {
        NAPI_ERR_LOG("SmartAlbumId is invalid, smartAlbumId = %{private}d", smartAlbumId);
        context->error = E_INVALID_VALUES;
        return;
    }
    vector<DataShare::DataShareValuesBucket> values;
    for (int32_t id : context->assetIds) {
        DataShare::DataShareValuesBucket valuesBucket;
        valuesBucket.Put(SMARTALBUMMAP_DB_ALBUM_ID, smartAlbumId);
        valuesBucket.Put(SMARTALBUMMAP_DB_CHILD_ASSET_ID, id);
        values.push_back(valuesBucket);
    }
    Uri removeAssetUri(MEDIALIBRARY_DATA_URI + "/" + MEDIA_SMARTALBUMMAPOPRN + "/" +
        MEDIA_SMARTALBUMMAPOPRN_REMOVESMARTALBUM);
    context->changedRows = UserFileClient::BatchInsert(removeAssetUri, values);
    if (context->changedRows != static_cast<int32_t>(context->assetIds.size())) {
        context->error = E_INVALID_VALUES;
    }
}

static void JSCommitModifyCompleteCallback(napi_env env, napi_status status, SmartAlbumNapiAsyncContext *context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    if (context->error == ERR_DEFAULT) {
        napi_create_int32(env, context->changedRows, &jsContext->data);
        napi_get_undefined(env, &jsContext->error);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, context->error,
                                                     "Failed to commit smart album");
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

static void JSAddAssetCompleteCallback(napi_env env, napi_status status, SmartAlbumNapiAsyncContext *context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    if (context->error == ERR_DEFAULT) {
        napi_create_int32(env, context->changedRows, &jsContext->data);
        napi_get_undefined(env, &jsContext->error);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error,  context->error,
                                                     "Failed to add smartalbum asset");
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

static void JSRemoveAssetCompleteCallback(napi_env env, napi_status status, SmartAlbumNapiAsyncContext *context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    if (context->error == ERR_DEFAULT) {
        napi_create_int32(env, context->changedRows, &jsContext->data);
        napi_get_undefined(env, &jsContext->error);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error,  context->error,
                                                     "Failed to remove smartalbum asset");
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

static napi_value ConvertCommitJSArgsToNative(napi_env env, size_t argc, const napi_value argv[],
    SmartAlbumNapiAsyncContext &asyncContext)
{
    const int32_t refCount = 1;
    napi_value result;
    auto context = &asyncContext;

    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");

    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);
        if (i == PARAM0 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &context->callbackRef);
        } else {
            NAPI_ASSERT(env, false, "type mismatch");
        }
    }

    // Return true napi_value if params are successfully obtained
    napi_get_boolean(env, true, &result);
    return result;
}

static napi_value GetAssetIds(napi_env env, napi_value param, SmartAlbumNapiAsyncContext &context)
{
    uint32_t arraySize = 0;
    if (!MediaLibraryNapiUtils::IsArrayForNapiValue(env, param, arraySize)) {
        NAPI_ERR_LOG("GetAssetIds get args fail, not array");
        return nullptr;
    }
    string uri = "";
    for (uint32_t i = 0; i < arraySize; i++) {
        napi_value jsValue = nullptr;
        int32_t result;
        if ((napi_get_element(env, param, i, &jsValue)) != napi_ok) {
            NAPI_ERR_LOG("GetAssetIds get args fail");
            return nullptr;
        }
        if (napi_get_value_int32(env, jsValue, &result) != napi_ok) {
            NAPI_ERR_LOG("Get ids value fail");
            return nullptr;
        } else {
            if (result < 0) {
                NAPI_ERR_LOG("GetAssetIds < 0 is invalid , id = %{public}d", result);
                return nullptr;
            }
            context.assetIds.push_back(result);
        }
    }
    napi_value res;
    napi_get_undefined(env, &res);
    return res;
}

napi_value GetJSArgsForAsset(napi_env env, size_t argc,
                             const napi_value argv[],
                             SmartAlbumNapiAsyncContext &asyncContext)
{
    const int32_t refCount = 1;
    napi_value result = nullptr;
    auto context = &asyncContext;

    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");

    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);
        if (i == PARAM0) {
            napi_value res = GetAssetIds(env, argv[PARAM0], asyncContext);
            if (res == nullptr) {
                napi_throw_error(env, std::to_string(ERR_INVALID_OUTPUT).c_str(), "Failed to obtain arguments ids");
                return nullptr;
            }
        } else if (i == PARAM1 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &context->callbackRef);
            break;
        } else {
            NAPI_ASSERT(env, false, "type mismatch");
        }
    }
    // Return true napi_value if params are successfully obtained
    napi_get_boolean(env, true, &result);
    return result;
}

napi_value SmartAlbumNapi::JSAddFileAssets(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;
    napi_value resource = nullptr;
    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameter maximum");
    napi_get_undefined(env, &result);
    std::unique_ptr<SmartAlbumNapiAsyncContext> asyncContext = std::make_unique<SmartAlbumNapiAsyncContext>();

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = GetJSArgsForAsset(env, argc, argv, *asyncContext);
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "JSAddFileAssets fail");
        NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        NAPI_CREATE_RESOURCE_NAME(env, resource, "JSAddFileAssets", asyncContext);

        asyncContext->objectPtr = asyncContext->objectInfo->smartAlbumAssetPtr;
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "SmartAlbumAsset is nullptr");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void *data) {
                auto context = static_cast<SmartAlbumNapiAsyncContext*>(data);
                JSAddAssetExecute(context);
            },
            reinterpret_cast<CompleteCallback>(JSAddAssetCompleteCallback),
            static_cast<void *>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value SmartAlbumNapi::JSRemoveFileAssets(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;
    napi_value resource = nullptr;
    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameter maximum");
    napi_get_undefined(env, &result);
    std::unique_ptr<SmartAlbumNapiAsyncContext> asyncContext = std::make_unique<SmartAlbumNapiAsyncContext>();

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = GetJSArgsForAsset(env, argc, argv, *asyncContext);
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "JSRemoveFileAssets fail ");

        NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        NAPI_CREATE_RESOURCE_NAME(env, resource, "JSRemoveFileAssets", asyncContext);

        asyncContext->objectPtr = asyncContext->objectInfo->smartAlbumAssetPtr;
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "SmartAlbumAsset is nullptr");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void *data) {
                auto context = static_cast<SmartAlbumNapiAsyncContext*>(data);
                JSRemoveAssetExecute(context);
            },
            reinterpret_cast<CompleteCallback>(JSRemoveAssetCompleteCallback),
            static_cast<void *>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value SmartAlbumNapi::JSCommitModify(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    napi_value resource = nullptr;
    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ZERO || argc == ARGS_ONE), "requires 1 parameter maximum");
    napi_get_undefined(env, &result);
    std::unique_ptr<SmartAlbumNapiAsyncContext> asyncContext = std::make_unique<SmartAlbumNapiAsyncContext>();

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = ConvertCommitJSArgsToNative(env, argc, argv, *asyncContext);
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "JSCommitModify fail ");

        NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        NAPI_CREATE_RESOURCE_NAME(env, resource, "JSCommitModify", asyncContext);

        asyncContext->objectPtr = asyncContext->objectInfo->smartAlbumAssetPtr;
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "SmartAlbumAsset is nullptr");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void *data) {
                auto context = static_cast<SmartAlbumNapiAsyncContext*>(data);
                CommitModifyNative(*context);
            },
            reinterpret_cast<CompleteCallback>(JSCommitModifyCompleteCallback),
            static_cast<void *>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

static void GetFetchOptionsParam(napi_env env, napi_value arg, const SmartAlbumNapiAsyncContext &context, bool &err)
{
    SmartAlbumNapiAsyncContext *asyncContext = const_cast<SmartAlbumNapiAsyncContext *>(&context);
    CHECK_NULL_PTR_RETURN_VOID(asyncContext, "Async context is null");
    char buffer[PATH_MAX];
    size_t res;
    uint32_t len = 0;
    napi_value property = nullptr;
    napi_value stringItem = nullptr;
    bool present = false;
    bool boolResult = false;

    string propertyName = "selections";
    string tmp = MediaLibraryNapiUtils::GetStringFetchProperty(env, arg, err, present, propertyName);
    if (!tmp.empty()) {
        asyncContext->selection = tmp;
    }

    propertyName = "order";
    tmp = MediaLibraryNapiUtils::GetStringFetchProperty(env, arg, err, present, propertyName);
    if (!tmp.empty()) {
        asyncContext->order = tmp;
    }

    napi_has_named_property(env, arg, "selectionArgs", &present);
    if (present && napi_get_named_property(env, arg, "selectionArgs", &property) == napi_ok &&
        napi_is_array(env, property, &boolResult) == napi_ok && boolResult) {
        napi_get_array_length(env, property, &len);
        for (size_t i = 0; i < len; i++) {
            napi_get_element(env, property, i, &stringItem);
            napi_get_value_string_utf8(env, stringItem, buffer, PATH_MAX, &res);
            asyncContext->selectionArgs.push_back(std::string(buffer));
            CHECK_IF_EQUAL(memset_s(buffer, PATH_MAX, 0, sizeof(buffer)) == 0, "Memset for buffer failed");
        }
    } else {
        NAPI_ERR_LOG("Could not get the string argument!");
        err = true;
    }
}

static napi_value ConvertJSArgsToNative(napi_env env, size_t argc, const napi_value argv[],
    SmartAlbumNapiAsyncContext &asyncContext)
{
    string str = "";
    std::vector<string> strArr;
    string order = "";
    bool err = false;
    const int32_t refCount = 1;
    napi_value result;
    auto context = &asyncContext;

    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");

    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);

        if (i == PARAM0 && valueType == napi_object) {
            GetFetchOptionsParam(env, argv[PARAM0], asyncContext, err);
            if (err) {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        } else if (i == PARAM0 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &context->callbackRef);
            break;
        } else if (i == PARAM1 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &context->callbackRef);
            break;
        } else {
            NAPI_ASSERT(env, false, "type mismatch");
        }
    }

    // Return true napi_value if params are successfully obtained
    napi_get_boolean(env, true, &result);
    return result;
}

static void UpdateSelection(SmartAlbumNapiAsyncContext *context)
{
    if (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR) {
        context->predicates.EqualTo(SMARTALBUMMAP_DB_ALBUM_ID, context->objectPtr->GetAlbumId());
        context->predicates.EqualTo(MEDIA_DATA_DB_TIME_PENDING, to_string(0));
        if (context->objectPtr->GetAlbumId() == TRASH_ALBUM_ID_VALUES) {
            context->predicates.NotEqualTo(MEDIA_DATA_DB_DATE_TRASHED, "0");
        } else {
            context->predicates.EqualTo(MEDIA_DATA_DB_DATE_TRASHED, "0");
        }
        MediaLibraryNapiUtils::UpdateMediaTypeSelections(context);
    } else {
        string trashPrefix;
        if (context->objectPtr->GetAlbumId() == TRASH_ALBUM_ID_VALUES) {
            trashPrefix = MEDIA_DATA_DB_DATE_TRASHED + " <> ? AND " + SMARTALBUMMAP_DB_ALBUM_ID + " = ? ";
        } else {
            trashPrefix = MEDIA_DATA_DB_DATE_TRASHED + " = ? AND " + SMARTALBUMMAP_DB_ALBUM_ID + " = ? ";
        }
        MediaLibraryNapiUtils::AppendFetchOptionSelection(context->selection, trashPrefix);
        context->selectionArgs.emplace_back("0");
        context->selectionArgs.emplace_back(std::to_string(context->objectPtr->GetAlbumId()));
        MediaLibraryNapi::ReplaceSelection(context->selection, context->selectionArgs,
            MEDIA_DATA_DB_RELATIVE_PATH, MEDIA_DATA_DB_RELATIVE_PATH, ReplaceSelectionMode::ADD_DOCS_TO_RELATIVE_PATH);
    }
}

static void GetFileAssetsNative(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetFileAssetsNative");

    auto context = static_cast<SmartAlbumNapiAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    UpdateSelection(context);
    MediaLibraryNapiUtils::FixSpecialDateType(context->selection);
    context->predicates.SetWhereClause(context->selection);
    context->predicates.SetWhereArgs(context->selectionArgs);
    context->predicates.SetOrder(context->order);

    if (context->resultNapiType == ResultNapiType::TYPE_MEDIALIBRARY) {
        context->fetchColumn = FILE_ASSET_COLUMNS;
    } else {
        context->fetchColumn.push_back(MEDIA_DATA_DB_ID);
        context->fetchColumn.push_back(MEDIA_DATA_DB_NAME);
        context->fetchColumn.push_back(MEDIA_DATA_DB_MEDIA_TYPE);
    }

    string queryUri = MEDIALIBRARY_DATA_ABILITY_PREFIX +
        (MediaFileUtils::GetNetworkIdFromUri(context->objectPtr->GetAlbumUri())) +
        MEDIALIBRARY_DATA_URI_IDENTIFIER + "/" + MEDIA_ALBUMOPRN_QUERYALBUM + "/" + ASSETMAP_VIEW_NAME;
    Uri uri(queryUri);
    int errCode = 0;
    auto resultSet = UserFileClient::Query(uri, context->predicates, context->fetchColumn, errCode);
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("resultSet == nullptr, errCode is %{public}d", errCode);
        return;
    }
    context->fetchResult = std::make_unique<FetchResult<FileAsset>>(move(resultSet));
    context->fetchResult->SetNetworkId(
        MediaFileUtils::GetNetworkIdFromUri(context->objectPtr->GetAlbumUri()));
    if (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR) {
        context->fetchResult->SetResultNapiType(context->resultNapiType);
    }
}

static void JSGetFileAssetsCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetFileAssetsCompleteCallback");

    auto context = static_cast<SmartAlbumNapiAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    if (context->fetchResult != nullptr) {
        if (context->fetchResult->GetCount() < 0) {
            napi_get_undefined(env, &jsContext->data);
            MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_MEM_ALLOCATION,
                "Find no data by options");
        } else {
            napi_value fetchRes = FetchFileResultNapi::CreateFetchFileResult(env, move(context->fetchResult));
            if (fetchRes == nullptr) {
                NAPI_ERR_LOG("Failed to get file asset napi object");
                napi_get_undefined(env, &jsContext->data);
                MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_MEM_ALLOCATION,
                    "Failed to create js object for FetchFileResult");
            } else {
                jsContext->data = fetchRes;
                napi_get_undefined(env, &jsContext->error);
                jsContext->status = true;
            }
        }
    } else {
        NAPI_ERR_LOG("No fetch file result found!");
        napi_get_undefined(env, &jsContext->data);
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
                                                     "Failed to get fetchFileResult from DB");
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value SmartAlbumNapi::JSGetSmartAlbumFileAssets(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    constexpr int maxArgs = 2;
    size_t argc = maxArgs;
    napi_value argv[maxArgs] = {0};
    napi_value thisVar = nullptr;

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, ((argc == ARGS_ZERO) || (argc == ARGS_ONE) || (argc == ARGS_TWO)),
                "requires 2 parameter maximum");

    napi_get_undefined(env, &result);
    std::unique_ptr<SmartAlbumNapiAsyncContext> asyncContext = std::make_unique<SmartAlbumNapiAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    asyncContext->resultNapiType = ResultNapiType::TYPE_MEDIALIBRARY;
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = ConvertJSArgsToNative(env, argc, argv, *asyncContext);
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");

        asyncContext->objectPtr = asyncContext->objectInfo->smartAlbumAssetPtr;
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "SmartAlbumAsset is nullptr");

        result = MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSGetSmartAlbumFileAssets",
            GetFileAssetsNative, JSGetFileAssetsCompleteCallback);
    }

    return result;
}

napi_value SmartAlbumNapi::UserFileMgrGetAssets(napi_env env, napi_callback_info info)
{
    napi_value ret = nullptr;
    unique_ptr<SmartAlbumNapiAsyncContext> asyncContext = make_unique<SmartAlbumNapiAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, ret, "AsyncContext context is null");

    asyncContext->mediaTypes.push_back(MEDIA_TYPE_IMAGE);
    asyncContext->mediaTypes.push_back(MEDIA_TYPE_VIDEO);
    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseAssetFetchOptCallback(env, info, asyncContext),
        JS_ERR_PARAMETER_INVALID);
    asyncContext->resultNapiType = ResultNapiType::TYPE_USERFILE_MGR;

    asyncContext->objectPtr = asyncContext->objectInfo->smartAlbumAssetPtr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, ret, "SmartAlbumAsset is nullptr");

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "UserFileMgrGetAssets", GetFileAssetsNative,
        JSGetFileAssetsCompleteCallback);
}

static void JSRecoverAssetExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSRecoverAssetExecute");

    auto context = static_cast<SmartAlbumNapiAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    string recoverUri = MEDIALIBRARY_DATA_URI + "/" + MEDIA_SMARTALBUMMAPOPRN + "/" +
        MEDIA_SMARTALBUMMAPOPRN_REMOVESMARTALBUM;
    Uri recoverAssetUri(recoverUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(SMARTALBUMMAP_DB_ALBUM_ID, context->objectPtr->GetAlbumId());
    string fileId = MediaLibraryNapiUtils::GetFileIdFromUri(context->uri);
    if (!MediaFileUtils::IsValidInteger(fileId)) {
        context->SaveError(E_FAIL);
        return;
    }
    valuesBucket.Put(SMARTALBUMMAP_DB_CHILD_ASSET_ID, stoi(fileId));
    int retVal = UserFileClient::Insert(recoverAssetUri, valuesBucket);
    context->SaveError(retVal);
}

static void JSRecoverAssetCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSRecoverAssetCompleteCallback");

    SmartAlbumNapiAsyncContext *context = static_cast<SmartAlbumNapiAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    CHECK_NULL_PTR_RETURN_VOID(jsContext, "jsContext context is null");
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);
    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
        Media::MediaType mediaType = MediaLibraryNapiUtils::GetMediaTypeFromUri(context->uri);
        string notifyUri = MediaFileUtils::GetMediaTypeUri(mediaType);
        Uri modifyNotify(notifyUri);
        UserFileClient::NotifyChange(modifyNotify);
    } else {
        context->HandleError(env, jsContext->error);
    }
    if (context->work != nullptr) {
        tracer.Finish();
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }

    delete context;
}

napi_value SmartAlbumNapi::UserFileMgrRecoverAsset(napi_env env, napi_callback_info info)
{
    napi_value ret = nullptr;
    unique_ptr<SmartAlbumNapiAsyncContext> asyncContext = make_unique<SmartAlbumNapiAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, ret, "AsyncContext context is null");

    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseArgsStringCallback(env, info, asyncContext, asyncContext->uri),
        JS_ERR_PARAMETER_INVALID);
    asyncContext->resultNapiType = ResultNapiType::TYPE_USERFILE_MGR;
    asyncContext->objectPtr = asyncContext->objectInfo->smartAlbumAssetPtr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, ret, "SmartAlbumAsset is nullptr");

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "UserFileMgrRecoverAsset",
        JSRecoverAssetExecute, JSRecoverAssetCompleteCallback);
}

static void JSDeleteAssetExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSDeleteAssetExecute");

    auto context = static_cast<SmartAlbumNapiAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    string deleteUri = MEDIALIBRARY_DATA_URI + "/" + MEDIA_FILEOPRN + "/" + MEDIA_FILEOPRN_DELETEASSET;
    Uri deleteAssetUri(deleteUri);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MEDIA_DATA_DB_ID, MediaLibraryNapiUtils::GetFileIdFromUri(context->uri));
    int retVal = UserFileClient::Delete(deleteAssetUri, {});
    context->SaveError(retVal);
}

static void JSDeleteAssetCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSDeleteAssetCompleteCallback");

    SmartAlbumNapiAsyncContext *context = static_cast<SmartAlbumNapiAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    CHECK_NULL_PTR_RETURN_VOID(jsContext, "JsContext context is null");
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);
    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
        Media::MediaType mediaType = MediaLibraryNapiUtils::GetMediaTypeFromUri(context->uri);
        string notifyUri = MediaFileUtils::GetMediaTypeUri(mediaType);
        Uri modifyNotify(notifyUri);
        UserFileClient::NotifyChange(modifyNotify);
    } else {
        context->HandleError(env, jsContext->error);
    }
    if (context->work != nullptr) {
        tracer.Finish();
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }

    delete context;
}

napi_value SmartAlbumNapi::UserFileMgrDeleteAsset(napi_env env, napi_callback_info info)
{
    napi_value ret = nullptr;
    unique_ptr<SmartAlbumNapiAsyncContext> asyncContext = make_unique<SmartAlbumNapiAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, ret, "AsyncContext context is null");

    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseArgsStringCallback(env, info, asyncContext, asyncContext->uri),
        JS_ERR_PARAMETER_INVALID);
    asyncContext->resultNapiType = ResultNapiType::TYPE_USERFILE_MGR;

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "UserFileMgrDeleteAsset", JSDeleteAssetExecute,
        JSDeleteAssetCompleteCallback);
}
} // namespace Media
} // namespace OHOS

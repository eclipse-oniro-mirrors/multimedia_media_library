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
#define MLOG_TAG "AlbumNapi"

#include "album_napi.h"

#include "media_file_asset_columns.h"
#include "media_file_utils.h"
#include "media_library_napi.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_tracer.h"
#include "userfile_client.h"
#include "media_file_uri.h"
#include "data_secondary_directory_uri.h"
#include "media_upgrade.h"

using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

namespace OHOS {
namespace Media {
using namespace std;
using namespace OHOS::DataShare;
thread_local napi_ref AlbumNapi::sConstructor_ = nullptr;
thread_local AlbumAsset *AlbumNapi::sAlbumData_ = nullptr;
using CompleteCallback = napi_async_complete_callback;

thread_local napi_ref AlbumNapi::userFileMgrConstructor_ = nullptr;
thread_local napi_ref AlbumNapi::photoAccessHelperConstructor_ = nullptr;

AlbumNapi::AlbumNapi()
    : env_(nullptr) {}

AlbumNapi::~AlbumNapi() = default;

void AlbumNapi::AlbumNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    AlbumNapi *album = reinterpret_cast<AlbumNapi*>(nativeObject);
    if (album != nullptr) {
        delete album;
        album = nullptr;
    }
}

napi_value AlbumNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value ctorObj;
    int32_t refCount = 1;

    napi_property_descriptor album_props[] = {
        DECLARE_NAPI_GETTER("albumId", JSGetAlbumId),
        DECLARE_NAPI_GETTER_SETTER("albumName", JSGetAlbumName, JSAlbumNameSetter),
        DECLARE_NAPI_GETTER("albumUri", JSGetAlbumUri),
        DECLARE_NAPI_GETTER("dateModified", JSGetAlbumDateModified),
        DECLARE_NAPI_GETTER("count", JSGetCount),
        DECLARE_NAPI_GETTER("relativePath", JSGetAlbumRelativePath),
        DECLARE_NAPI_GETTER("coverUri", JSGetCoverUri),
        DECLARE_NAPI_FUNCTION("commitModify", JSCommitModify),
        DECLARE_NAPI_GETTER_SETTER("path", JSGetAlbumPath, JSSetAlbumPath),
        DECLARE_NAPI_GETTER("virtual", JSGetAlbumVirtual),
        DECLARE_NAPI_FUNCTION("getFileAssets", JSGetAlbumFileAssets)
    };

    status = napi_define_class(env, ALBUM_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH,
                               AlbumNapiConstructor, nullptr,
                               sizeof(album_props) / sizeof(album_props[PARAM0]),
                               album_props, &ctorObj);
    if (status == napi_ok) {
        status = napi_create_reference(env, ctorObj, refCount, &sConstructor_);
        if (status == napi_ok) {
            status = napi_set_named_property(env, exports, ALBUM_NAPI_CLASS_NAME.c_str(), ctorObj);
            if (status == napi_ok) {
                return exports;
            }
        }
    }

    return nullptr;
}

napi_value AlbumNapi::UserFileMgrInit(napi_env env, napi_value exports)
{
    NapiClassInfo info = {
        .name = USERFILEMGR_ALBUM_NAPI_CLASS_NAME,
        .ref = &userFileMgrConstructor_,
        .constructor = AlbumNapiConstructor,
        .props = {
            DECLARE_NAPI_FUNCTION("getPhotoAssets", UserFileMgrGetAssets),
            DECLARE_NAPI_FUNCTION("commitModify", UserFileMgrCommitModify),
            DECLARE_NAPI_GETTER_SETTER("albumName", JSGetAlbumName, JSAlbumNameSetter),
            DECLARE_NAPI_GETTER("albumUri", JSGetAlbumUri),
            DECLARE_NAPI_GETTER("dateModified", JSGetAlbumDateModified),
            DECLARE_NAPI_GETTER("count", JSGetCount),
            DECLARE_NAPI_GETTER("relativePath", JSGetAlbumRelativePath),
            DECLARE_NAPI_GETTER("coverUri", JSGetCoverUri)
        }
    };

    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);
    return exports;
}

napi_value AlbumNapi::PhotoAccessHelperInit(napi_env env, napi_value exports)
{
    NapiClassInfo info = {
        .name = PHOTOACCESSHELPER_ALBUM_NAPI_CLASS_NAME,
        .ref = &photoAccessHelperConstructor_,
        .constructor = AlbumNapiConstructor,
        .props = {
            DECLARE_NAPI_FUNCTION("getAssets", PhotoAccessHelperGetAssets),
            DECLARE_NAPI_FUNCTION("commitModify", PhotoAccessHelperCommitModify),
            DECLARE_NAPI_GETTER_SETTER("albumName", JSGetAlbumName, JSAlbumNameSetter),
            DECLARE_NAPI_GETTER("albumUri", JSGetAlbumUri),
            DECLARE_NAPI_GETTER("count", JSGetCount),
            DECLARE_NAPI_GETTER("coverUri", JSGetCoverUri)
        }
    };

    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);
    return exports;
}

void AlbumNapi::SetAlbumNapiProperties()
{
    albumAssetPtr = std::shared_ptr<AlbumAsset>(sAlbumData_);
}

// Constructor callback
napi_value AlbumNapi::AlbumNapiConstructor(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status == napi_ok && thisVar != nullptr) {
        std::unique_ptr<AlbumNapi> obj = std::make_unique<AlbumNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            if (sAlbumData_ != nullptr) {
                obj->SetAlbumNapiProperties();
            }

            status = napi_wrap(env, thisVar, reinterpret_cast<void *>(obj.get()),
                               AlbumNapi::AlbumNapiDestructor, nullptr, nullptr);
            if (status == napi_ok) {
                obj.release();
                return thisVar;
            } else {
                NAPI_ERR_LOG("Failure wrapping js to native napi. status: %{public}d", status);
            }
        }
    }

    return result;
}

napi_value AlbumNapi::CreateAlbumNapi(napi_env env, unique_ptr<AlbumAsset> &albumData)
{
    if (albumData == nullptr) {
        NAPI_ERR_LOG("CreateAlbumNapi albumData is null");
        return nullptr;
    }

    napi_value constructor;
    napi_ref constructorRef;
    if (albumData->GetResultNapiType() == ResultNapiType::TYPE_USERFILE_MGR) {
        constructorRef = userFileMgrConstructor_;
    } else if (albumData->GetResultNapiType() == ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        constructorRef = photoAccessHelperConstructor_;
    } else {
        constructorRef = sConstructor_;
    }
    NAPI_CALL(env, napi_get_reference_value(env, constructorRef, &constructor));

    napi_value result = nullptr;
    sAlbumData_ = albumData.release();
    NAPI_CALL(env, napi_new_instance(env, constructor, 0, nullptr, &result));
    sAlbumData_ = nullptr;
    return result;
}

std::string AlbumNapi::GetAlbumName() const
{
    return albumAssetPtr->GetAlbumName();
}

std::string AlbumNapi::GetAlbumPath() const
{
    return albumAssetPtr->GetAlbumPath();
}

int32_t AlbumNapi::GetAlbumId() const
{
    return albumAssetPtr->GetAlbumId();
}

std::string AlbumNapi::GetAlbumUri() const
{
    return albumAssetPtr->GetAlbumUri();
}

std::string AlbumNapi::GetNetworkId() const
{
    return MediaFileUtils::GetNetworkIdFromUri(GetAlbumUri());
}

#ifdef MEDIALIBRARY_COMPATIBILITY
PhotoAlbumType AlbumNapi::GetAlbumType() const
{
    return albumAssetPtr->GetAlbumType();
}
PhotoAlbumSubType AlbumNapi::GetAlbumSubType() const
{
    return albumAssetPtr->GetAlbumSubType();
}
#endif

napi_value AlbumNapi::JSGetAlbumId(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    AlbumNapi* obj = nullptr;
    int32_t id;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &undefinedResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        id = obj->GetAlbumId();
        status = napi_create_int32(env, id, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        }
    }

    return undefinedResult;
}

napi_value AlbumNapi::JSGetAlbumName(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    AlbumNapi* obj = nullptr;
    std::string name = "";
    napi_value thisVar = nullptr;
    napi_get_undefined(env, &undefinedResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        name = obj->GetAlbumName();
        status = napi_create_string_utf8(env, name.c_str(), NAPI_AUTO_LENGTH, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        }
    }

    return undefinedResult;
}

napi_value AlbumNapi::JSAlbumNameSetter(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    size_t res = 0;
    char buffer[FILENAME_MAX];
    AlbumNapi* obj = nullptr;
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
        obj->albumAssetPtr->SetAlbumName(std::string(buffer));
    } else {
        NAPI_ERR_LOG("status = %{public}d", status);
    }
    return jsResult;
}
napi_value AlbumNapi::JSGetAlbumUri(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    AlbumNapi* obj = nullptr;
    std::string uri = "";
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &undefinedResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        uri = obj->GetAlbumUri();
        status = napi_create_string_utf8(env, uri.c_str(), NAPI_AUTO_LENGTH, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        }
    }

    return undefinedResult;
}
napi_value AlbumNapi::JSGetAlbumDateModified(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    AlbumNapi* obj = nullptr;
    int64_t dateModified;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &undefinedResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        dateModified = obj->albumAssetPtr->GetAlbumDateModified() / MSEC_TO_SEC;
        status = napi_create_int64(env, dateModified, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        }
    }

    return undefinedResult;
}
napi_value AlbumNapi::JSGetCount(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    AlbumNapi *obj = nullptr;
    int32_t count;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &undefinedResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return undefinedResult;
    }
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        count = obj->albumAssetPtr->GetCount();
        status = napi_create_int32(env, count, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        }
    }
    return undefinedResult;
}
napi_value AlbumNapi::JSGetAlbumRelativePath(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    AlbumNapi* obj = nullptr;
    std::string relativePath = "";
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &undefinedResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        relativePath = obj->albumAssetPtr->GetAlbumRelativePath();
        status = napi_create_string_utf8(env, relativePath.c_str(), NAPI_AUTO_LENGTH, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        }
    }

    return undefinedResult;
}
napi_value AlbumNapi::JSGetCoverUri(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    AlbumNapi* obj = nullptr;
    std::string coverUri = "";
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &undefinedResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        coverUri = obj->albumAssetPtr->GetCoverUri();
        status = napi_create_string_utf8(env, coverUri.c_str(), NAPI_AUTO_LENGTH, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        }
    }

    return undefinedResult;
}

napi_value AlbumNapi::JSSetAlbumPath(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    size_t res = 0;
    char buffer[PATH_MAX];
    AlbumNapi* obj = nullptr;
    napi_value thisVar = nullptr;
    napi_valuetype valueType = napi_undefined;

    napi_get_undefined(env, &jsResult);
    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc == ARGS_ONE, "requires 1 parameter");

    if (thisVar == nullptr || napi_typeof(env, argv[PARAM0], &valueType) != napi_ok
        || valueType != napi_string) {
        NAPI_ERR_LOG("Invalid arguments type! type: %{public}d", valueType);
        return jsResult;
    }

    napi_get_value_string_utf8(env, argv[PARAM0], buffer, PATH_MAX, &res);

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        obj->albumAssetPtr->SetAlbumPath(std::string(buffer));
    }

    return jsResult;
}

napi_value AlbumNapi::JSGetAlbumPath(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    AlbumNapi* obj = nullptr;
    std::string path = "";
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &undefinedResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        path = obj->GetAlbumPath();
        status = napi_create_string_utf8(env, path.c_str(), NAPI_AUTO_LENGTH, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        }
    }

    return undefinedResult;
}

napi_value AlbumNapi::JSGetAlbumVirtual(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    AlbumNapi* obj = nullptr;
    bool virtualAlbum = false;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &undefinedResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        virtualAlbum = obj->albumAssetPtr->GetAlbumVirtual();
        status = napi_get_boolean(env, virtualAlbum, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        }
    }

    return undefinedResult;
}

static void GetFetchOptionsParam(napi_env env, napi_value arg, const AlbumNapiAsyncContext &context, bool &err)
{
    AlbumNapiAsyncContext *asyncContext = const_cast<AlbumNapiAsyncContext *>(&context);
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
    AlbumNapiAsyncContext &asyncContext)
{
    string str = "";
    std::vector<string> strArr;
    string order = "";
    bool err = false;
    const int32_t refCount = 1;
    napi_value result;
    auto context = &asyncContext;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, context, result, "Async context is null");
    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");
    if (argc == ARGS_ONE) {
        napi_valuetype valueType = napi_undefined;
        if (napi_typeof(env, argv[PARAM0], &valueType) == napi_ok &&
            (valueType == napi_undefined || valueType == napi_null)) {
            argc -= 1;
        }
    }

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
static napi_value ConvertCommitJSArgsToNative(napi_env env, size_t argc, const napi_value argv[],
    AlbumNapiAsyncContext &asyncContext)
{
    string str = "";
    vector<string> strArr;
    string order = "";
    bool err = false;
    const int32_t refCount = 1;
    napi_value result;
    auto context = &asyncContext;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, context, result, "Async context is null");
    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");

    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);

        if (i == PARAM0 && valueType == napi_object) {
            GetFetchOptionsParam(env, argv[PARAM0], asyncContext, err);
            if (err) {
                NAPI_ERR_LOG("fetch options retrieval failed. err %{public}d", err);
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

#ifdef MEDIALIBRARY_COMPATIBILITY
static void UpdateCompatAlbumSelection(AlbumNapiAsyncContext *context)
{
    PhotoAlbumSubType subType = context->objectPtr->GetAlbumSubType();
    string filterClause;
    switch (subType) {
        case PhotoAlbumSubType::CAMERA: {
            static const string CAMERA_FILTER = PhotoColumn::PHOTO_SUBTYPE + "=" +
                to_string(static_cast<int32_t>(PhotoSubType::CAMERA)) + " AND " + MediaUpgrade::ASSETS_QUERY_FILTER;
            filterClause = CAMERA_FILTER;
            break;
        }
        case PhotoAlbumSubType::SCREENSHOT: {
            static const string SCREENSHOT_FILTER = PhotoColumn::PHOTO_SUBTYPE + "=" +
                to_string(static_cast<int32_t>(PhotoSubType::SCREENSHOT)) + " AND " + MediaUpgrade::ASSETS_QUERY_FILTER;
            filterClause = SCREENSHOT_FILTER;
            break;
        }
        case PhotoAlbumSubType::FAVORITE: {
            static const string FAVORITE_FILTER = PhotoColumn::MEDIA_IS_FAV + " = 1" + " AND " +
                MediaUpgrade::ASSETS_QUERY_FILTER;
            filterClause = FAVORITE_FILTER;
            break;
        }
        case PhotoAlbumSubType::TRASH: {
            static const string TRASH_FILTER =
                PhotoColumn::MEDIA_DATE_TRASHED + " > 0 AND " + MEDIA_DATA_DB_IS_TRASH + " <> " +
                to_string(static_cast<int32_t>(TRASHED_DIR_CHILD));
            filterClause = TRASH_FILTER;
            break;
        }
        default: {
            NAPI_ERR_LOG("Album subtype not support for compatibility: %{public}d", subType);
            context->SaveError(-EINVAL);
            return;
        }
    }
    if (!context->selection.empty()) {
        context->selection = filterClause + " AND (" + context->selection + ")";
    } else {
        context->selection = filterClause;
    }
    MediaLibraryNapi::ReplaceSelection(context->selection, context->selectionArgs,
        MEDIA_DATA_DB_RELATIVE_PATH, MEDIA_DATA_DB_RELATIVE_PATH);
}
#endif

static void UpdateSelection(AlbumNapiAsyncContext *context)
{
    if (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR ||
        context->resultNapiType == ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        context->predicates.EqualTo(MEDIA_DATA_DB_DATE_TRASHED, 0);
        context->predicates.NotEqualTo(MEDIA_DATA_DB_MEDIA_TYPE, MEDIA_TYPE_ALBUM);
        context->predicates.EqualTo(MEDIA_DATA_DB_BUCKET_ID, context->objectPtr->GetAlbumId());
        context->predicates.EqualTo(MediaColumn::MEDIA_TIME_PENDING, to_string(0));
        context->predicates.EqualTo(PhotoColumn::PHOTO_IS_TEMP, to_string(0));
        context->predicates.EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL,
            to_string(static_cast<int32_t>(BurstCoverLevelType::COVER)));
        MediaLibraryNapiUtils::UpdateMediaTypeSelections(context);
    } else {
#ifdef MEDIALIBRARY_COMPATIBILITY
        UpdateCompatAlbumSelection(context);
#else
        string trashPrefix = MEDIA_DATA_DB_DATE_TRASHED + " = ? ";
        MediaLibraryNapiUtils::AppendFetchOptionSelection(context->selection, trashPrefix);
        context->selectionArgs.emplace_back("0");

        string prefix = MEDIA_DATA_DB_MEDIA_TYPE + " <> ? ";
        MediaLibraryNapiUtils::AppendFetchOptionSelection(context->selection, prefix);
        context->selectionArgs.emplace_back(to_string(MEDIA_TYPE_ALBUM));

        string idPrefix = MEDIA_DATA_DB_BUCKET_ID + " = ? ";
        MediaLibraryNapiUtils::AppendFetchOptionSelection(context->selection, idPrefix);
        context->selectionArgs.emplace_back(std::to_string(context->objectPtr->GetAlbumId()));
#endif
    }
}

static void GetFileAssetsNative(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetFileAssetsNative");

    AlbumNapiAsyncContext *context = static_cast<AlbumNapiAsyncContext*>(data);

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
        (MediaFileUtils::GetNetworkIdFromUri(context->objectPtr->GetAlbumUri())) + MEDIALIBRARY_DATA_URI_IDENTIFIER;
    NAPI_DEBUG_LOG("queryUri is = %{private}s", queryUri.c_str());
    Uri uri(queryUri);
    int errCode = 0;
    int userId = context->fetchResult != nullptr ? context->fetchResult->GetUserId() : -1;
    std::shared_ptr<OHOS::DataShare::DataShareResultSet> resultSet = UserFileClient::Query(uri,
        context->predicates, context->fetchColumn, errCode, userId);
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("GetFileAssetsNative called, UserFileClient::Query errorCode is = %{public}d", errCode);
        context->SaveError(errCode);
        return;
    }
    context->fetchResult = std::make_unique<FetchResult<FileAsset>>(move(resultSet));
    context->fetchResult->SetNetworkId(MediaFileUtils::GetNetworkIdFromUri(context->objectPtr->GetAlbumUri()));
    if (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR ||
        context->resultNapiType == ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        context->fetchResult->SetResultNapiType(context->resultNapiType);
    }
}

static void JSGetFileAssetsCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetFileAssetsCompleteCallback");

    AlbumNapiAsyncContext *context = static_cast<AlbumNapiAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    if (context->fetchResult != nullptr) {
        if (context->fetchResult->GetCount() < 0) {
            napi_get_undefined(env, &jsContext->data);
            MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_MEM_ALLOCATION,
                                                         "find no data by options");
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
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

static void CommitModifyNative(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("CommitModifyNative");

    auto *context = static_cast<AlbumNapiAsyncContext*>(data);
    auto objectPtr = context->objectPtr;
    if (MediaFileUtils::CheckAlbumName(objectPtr->GetAlbumName()) < 0) {
        context->error = JS_E_DISPLAYNAME;
        NAPI_ERR_LOG("album name invalid = %{private}s", objectPtr->GetAlbumName().c_str());
        return;
    }
#ifdef MEDIALIBRARY_COMPATIBILITY
    context->changedRows = 0;
#else
    DataSharePredicates predicates;
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MEDIA_DATA_DB_TITLE, objectPtr->GetAlbumName());
    predicates.SetWhereClause(MEDIA_DATA_DB_ID + " = ? ");
    predicates.SetWhereArgs({ std::to_string(objectPtr->GetAlbumId()) });

    string updateUri = MEDIALIBRARY_DATA_URI + "/" +
        MEDIA_ALBUMOPRN + "/" + MEDIA_ALBUMOPRN_MODIFYALBUM + "/" + std::to_string(objectPtr->GetAlbumId());
    Uri uri(updateUri);
    int changedRows = UserFileClient::Update(uri, predicates, valuesBucket);
    if (changedRows > 0) {
        DataSharePredicates filePredicates;
        DataShareValuesBucket fileValuesBucket;
        fileValuesBucket.Put(MEDIA_DATA_DB_BUCKET_NAME, objectPtr->GetAlbumName());
        filePredicates.SetWhereClause(MEDIA_DATA_DB_BUCKET_ID + " = ? ");
        filePredicates.SetWhereArgs({ std::to_string(objectPtr->GetAlbumId()) });

        string fileUriStr = MEDIALIBRARY_DATA_URI;
        Uri fileUri(fileUriStr);
        changedRows = UserFileClient::Update(fileUri, filePredicates, fileValuesBucket);
    }
    context->SaveError(changedRows);
    context->changedRows = changedRows;
#endif
}

static void JSCommitModifyCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSCommitModifyCompleteCallback");

    AlbumNapiAsyncContext *context = static_cast<AlbumNapiAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    if (context->error == ERR_DEFAULT) {
        napi_create_int32(env, context->changedRows, &jsContext->data);
        napi_get_undefined(env, &jsContext->error);
        jsContext->status = true;
        auto contextUri = make_unique<Uri>(MEDIALIBRARY_ALBUM_URI);
        UserFileClient::NotifyChange(*contextUri);
    } else {
        napi_get_undefined(env, &jsContext->data);
        context->HandleError(env, jsContext->error);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }

    delete context;
}
napi_value AlbumNapi::JSGetAlbumFileAssets(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    MediaLibraryTracer tracer;
    tracer.Start("JSGetAlbumFileAssets");

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, ((argc == ARGS_ZERO) || (argc == ARGS_ONE) || (argc == ARGS_TWO)),
                "requires 2 parameter maximum");
    napi_get_undefined(env, &result);

    std::unique_ptr<AlbumNapiAsyncContext> asyncContext = std::make_unique<AlbumNapiAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_MEDIALIBRARY;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = ConvertJSArgsToNative(env, argc, argv, *asyncContext);
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
        asyncContext->objectPtr = asyncContext->objectInfo->albumAssetPtr;
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "AlbumAsset is nullptr");

        result = MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSGetAlbumFileAssets",
            GetFileAssetsNative, JSGetFileAssetsCompleteCallback);
    }

    return result;
}
napi_value AlbumNapi::JSCommitModify(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    MediaLibraryTracer tracer;
    tracer.Start("JSCommitModify");

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ZERO || argc == ARGS_ONE), "requires 1 parameter maximum");
    napi_get_undefined(env, &result);

    std::unique_ptr<AlbumNapiAsyncContext> asyncContext = std::make_unique<AlbumNapiAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, result, "asyncContext context is null");
    asyncContext->resultNapiType = ResultNapiType::TYPE_MEDIALIBRARY;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = ConvertCommitJSArgsToNative(env, argc, argv, *asyncContext);
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "JSCommitModify fail ");
        asyncContext->objectPtr = asyncContext->objectInfo->albumAssetPtr;
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "AlbumAsset is nullptr");

        result = MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSCommitModify", CommitModifyNative,
            JSCommitModifyCompleteCallback);
    }

    return result;
}

napi_value AlbumNapi::UserFileMgrGetAssets(napi_env env, napi_callback_info info)
{
    napi_value ret = nullptr;
    unique_ptr<AlbumNapiAsyncContext> asyncContext = make_unique<AlbumNapiAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, ret, "asyncContext context is null");

    asyncContext->mediaTypes.push_back(MEDIA_TYPE_IMAGE);
    asyncContext->mediaTypes.push_back(MEDIA_TYPE_VIDEO);
    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseAssetFetchOptCallback(env, info, asyncContext),
        JS_ERR_PARAMETER_INVALID);
    asyncContext->resultNapiType = ResultNapiType::TYPE_USERFILE_MGR;
    asyncContext->objectPtr = asyncContext->objectInfo->albumAssetPtr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, ret, "AlbumAsset is nullptr");

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "UserFileMgrGetAssets", GetFileAssetsNative,
        JSGetFileAssetsCompleteCallback);
}

napi_value AlbumNapi::UserFileMgrCommitModify(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrCommitModify");

    napi_value ret = nullptr;
    unique_ptr<AlbumNapiAsyncContext> asyncContext = make_unique<AlbumNapiAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, ret, "asyncContext context is null");
    asyncContext->resultNapiType = ResultNapiType::TYPE_USERFILE_MGR;
    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseArgsOnlyCallBack(env, info, asyncContext), JS_ERR_PARAMETER_INVALID);
    asyncContext->objectPtr = asyncContext->objectInfo->albumAssetPtr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, ret, "AlbumAsset is nullptr");

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "UserFileMgrCommitModify", CommitModifyNative,
        JSCommitModifyCompleteCallback);
}

napi_value AlbumNapi::PhotoAccessHelperGetAssets(napi_env env, napi_callback_info info)
{
    napi_value ret = nullptr;
    unique_ptr<AlbumNapiAsyncContext> asyncContext = make_unique<AlbumNapiAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, ret, "asyncContext context is null");

    asyncContext->mediaTypes.push_back(MEDIA_TYPE_IMAGE);
    asyncContext->mediaTypes.push_back(MEDIA_TYPE_VIDEO);
    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseAssetFetchOptCallback(env, info, asyncContext),
        JS_ERR_PARAMETER_INVALID);
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->objectPtr = asyncContext->objectInfo->albumAssetPtr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, ret, "AlbumAsset is nullptr");

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "UserFileMgrGetAssets", GetFileAssetsNative,
        JSGetFileAssetsCompleteCallback);
}

napi_value AlbumNapi::PhotoAccessHelperCommitModify(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrCommitModify");

    napi_value ret = nullptr;
    unique_ptr<AlbumNapiAsyncContext> asyncContext = make_unique<AlbumNapiAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, ret, "asyncContext context is null");
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseArgsOnlyCallBack(env, info, asyncContext), JS_ERR_PARAMETER_INVALID);
    asyncContext->objectPtr = asyncContext->objectInfo->albumAssetPtr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, ret, "AlbumAsset is nullptr");

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "UserFileMgrCommitModify", CommitModifyNative,
        JSCommitModifyCompleteCallback);
}
} // namespace Media
} // namespace OHOS

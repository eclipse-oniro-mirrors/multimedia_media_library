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
#define MLOG_TAG "FetchResultNapi"

#include "fetch_file_result_napi.h"
#include <set>

#include "album_napi.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_tracer.h"
#include "smart_album_napi.h"
#include "media_file_utils.h"
#include "album_order_napi.h"
#include "photo_album_napi.h"
#include "media_log.h"

using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;
using namespace std;

namespace OHOS {
namespace Media {
thread_local napi_ref FetchFileResultNapi::sConstructor_ = nullptr;
thread_local napi_ref FetchFileResultNapi::userFileMgrConstructor_ = nullptr;
thread_local napi_ref FetchFileResultNapi::photoAccessHelperConstructor_ = nullptr;

const uint32_t MAX_SIZE = 500;

FetchFileResultNapi::FetchFileResultNapi()
    : env_(nullptr) {}

FetchFileResultNapi::~FetchFileResultNapi()
{
    propertyPtr = nullptr;
}

void FetchFileResultNapi::FetchFileResultNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    FetchFileResultNapi *fetchFileResultObj = reinterpret_cast<FetchFileResultNapi*>(nativeObject);
    if (fetchFileResultObj != nullptr) {
        delete fetchFileResultObj;
        fetchFileResultObj = nullptr;
    }
}

napi_value FetchFileResultNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value ctorObj;
    int32_t refCount = 1;

    napi_property_descriptor fetch_file_result_props[] = {
        DECLARE_NAPI_FUNCTION("getCount", JSGetCount),
        DECLARE_NAPI_FUNCTION("isAfterLast", JSIsAfterLast),
        DECLARE_NAPI_FUNCTION("getFirstObject", JSGetFirstObject),
        DECLARE_NAPI_FUNCTION("getNextObject", JSGetNextObject),
        DECLARE_NAPI_FUNCTION("getLastObject", JSGetLastObject),
        DECLARE_NAPI_FUNCTION("getPositionObject", JSGetPositionObject),
        DECLARE_NAPI_FUNCTION("getAllObject", JSGetAllObject),
        DECLARE_NAPI_FUNCTION("close", JSClose)
    };

    status = napi_define_class(env, FETCH_FILE_RESULT_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH,
                               FetchFileResultNapiConstructor, nullptr,
                               sizeof(fetch_file_result_props) / sizeof(fetch_file_result_props[PARAM0]),
                               fetch_file_result_props, &ctorObj);
    if (status == napi_ok) {
        status = napi_create_reference(env, ctorObj, refCount, &sConstructor_);
        if (status == napi_ok) {
            status = napi_set_named_property(env, exports, FETCH_FILE_RESULT_CLASS_NAME.c_str(), ctorObj);
            if (status == napi_ok) {
                return exports;
            }
        }
    }
    NAPI_DEBUG_LOG("Init success");
    return nullptr;
}

void FetchFileResultNapi::GetFetchResult(unique_ptr<FetchFileResultNapi> &obj)
{
    switch (sFetchResType_) {
        case FetchResType::TYPE_FILE: {
            auto fileResult = make_shared<FetchResult<FileAsset>>(move(sFetchFileResult_->GetDataShareResultSet()));
            obj->propertyPtr->fetchFileResult_ = fileResult;
            obj->propertyPtr->fetchFileResult_->SetInfo(sFetchFileResult_);
            obj->propertyPtr->fetchFileResult_->SetUserId(sFetchFileResult_->GetUserId());
            break;
        }
        case FetchResType::TYPE_ALBUM: {
            auto albumResult = make_shared<FetchResult<AlbumAsset>>(move(sFetchAlbumResult_->GetDataShareResultSet()));
            obj->propertyPtr->fetchAlbumResult_ = albumResult;
            obj->propertyPtr->fetchAlbumResult_->SetInfo(sFetchAlbumResult_);
            obj->propertyPtr->fetchAlbumResult_->SetUserId(sFetchAlbumResult_->GetUserId());
            break;
        }
        case FetchResType::TYPE_PHOTOALBUM: {
            auto photoAlbumResult =
                make_shared<FetchResult<PhotoAlbum>>(move(sFetchPhotoAlbumResult_->GetDataShareResultSet()));
            obj->propertyPtr->fetchPhotoAlbumResult_ = photoAlbumResult;
            obj->propertyPtr->fetchPhotoAlbumResult_->SetInfo(sFetchPhotoAlbumResult_);
            obj->propertyPtr->fetchPhotoAlbumResult_->SetUserId(sFetchPhotoAlbumResult_->GetUserId());
            break;
        }
        case FetchResType::TYPE_SMARTALBUM: {
            auto smartResult =
                make_shared<FetchResult<SmartAlbumAsset>>(move(sFetchSmartAlbumResult_->GetDataShareResultSet()));
            obj->propertyPtr->fetchSmartAlbumResult_ = smartResult;
            obj->propertyPtr->fetchSmartAlbumResult_->SetInfo(sFetchSmartAlbumResult_);
            break;
        }
        case FetchResType::TYPE_CUSTOMRECORD: {
            auto cRecordResult = make_shared<FetchResult<PhotoAssetCustomRecord>>(
                move(sFetchPhotoAssetCustomRecordResult_->GetDataShareResultSet()));
            obj->propertyPtr->fetchCustomRecordResult_ = cRecordResult;
            obj->propertyPtr->fetchCustomRecordResult_->SetInfo(sFetchPhotoAssetCustomRecordResult_);
            break;
        }
        case FetchResType::TYPE_ALBUMORDER: {
            auto albumOrderResult = make_shared<FetchResult<AlbumOrder>>(
                move(sFetchAlbumOrderResult_->GetDataShareResultSet()));
            obj->propertyPtr->fetchAlbumOrderResult_ = albumOrderResult;
            obj->propertyPtr->fetchAlbumOrderResult_->SetInfo(sFetchAlbumOrderResult_);
            break;
        }
        default:
            NAPI_ERR_LOG("unsupported FetchResType");
            break;
    }
}

// Constructor callback
napi_value FetchFileResultNapi::FetchFileResultNapiConstructor(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("FetchFileResultNapiConstructor");

    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Get Js obj failed, status: %{public}d, (thisVar == nullptr) = %{public}d",
            status, (thisVar == nullptr));
        return result;
    }

    unique_ptr<FetchFileResultNapi> obj = make_unique<FetchFileResultNapi>();
    if (obj == nullptr) {
        NAPI_ERR_LOG("Get FetchFileResultNapi failed");
        return result;
    }
    obj->env_ = env;
    obj->propertyPtr = make_shared<FetchResultProperty>();
    GetFetchResult(obj);
    obj->propertyPtr->fetchResType_ = sFetchResType_;
    status = napi_wrap(env, thisVar, reinterpret_cast<void *>(obj.get()),
                       FetchFileResultNapi::FetchFileResultNapiDestructor, nullptr, nullptr);
    if (status == napi_ok) {
        obj.release();
        return thisVar;
    } else {
        NAPI_ERR_LOG("Failure wrapping js to native napi, status: %{public}d", status);
    }
    return result;
}

FetchResType FetchFileResultNapi::GetFetchResType()
{
    return propertyPtr->fetchResType_;
}

void FetchFileResultNapi::SolveConstructorRef(unique_ptr<FetchResult<FileAsset>> &fileResult,
    napi_ref &constructorRef)
{
    switch(fileResult->GetResultNapiType()) {
        case ResultNapiType::TYPE_USERFILE_MGR: {
            constructorRef = userFileMgrConstructor_;
            break;
        }
        case ResultNapiType::TYPE_PHOTOACCESS_HELPER: {
            constructorRef = photoAccessHelperConstructor_;
            break;
        }
        default:
            constructorRef = sConstructor_;
            break;
    }
}

void FetchFileResultNapi::SolveConstructorRef(unique_ptr<FetchResult<AlbumAsset>> &fileResult,
    napi_ref &constructorRef)
{
    switch(fileResult->GetResultNapiType()) {
        case ResultNapiType::TYPE_USERFILE_MGR: {
            constructorRef = userFileMgrConstructor_;
            break;
        }
        case ResultNapiType::TYPE_PHOTOACCESS_HELPER: {
            constructorRef = photoAccessHelperConstructor_;
            break;
        }
        default:
            constructorRef = sConstructor_;
            break;
    }
}

void FetchFileResultNapi::SolveConstructorRef(unique_ptr<FetchResult<SmartAlbumAsset>> &fileResult,
    napi_ref &constructorRef)
{
    switch(fileResult->GetResultNapiType()) {
        case ResultNapiType::TYPE_USERFILE_MGR: {
            constructorRef = userFileMgrConstructor_;
            break;
        }
        case ResultNapiType::TYPE_PHOTOACCESS_HELPER: {
            constructorRef = photoAccessHelperConstructor_;
            break;
        }
        default:
            constructorRef = sConstructor_;
            break;
    }
}

void FetchFileResultNapi::SolveConstructorRef(unique_ptr<FetchResult<PhotoAlbum>> &fileResult,
    napi_ref &constructorRef)
{
    switch(fileResult->GetResultNapiType()) {
        case ResultNapiType::TYPE_USERFILE_MGR: {
            constructorRef = userFileMgrConstructor_;
            break;
        }
        case ResultNapiType::TYPE_PHOTOACCESS_HELPER: {
            constructorRef = photoAccessHelperConstructor_;
            break;
        }
        default:
            constructorRef = sConstructor_;
            break;
    }
}

void FetchFileResultNapi::SolveConstructorRef(unique_ptr<FetchResult<PhotoAssetCustomRecord>> &fileResult,
    napi_ref &constructorRef)
{
    CHECK_NULL_PTR_RETURN_VOID(fileResult, "fileResult is nullptr");
    switch(fileResult->GetResultNapiType()) {
        case ResultNapiType::TYPE_USERFILE_MGR: {
            constructorRef = userFileMgrConstructor_;
            break;
        }
        case ResultNapiType::TYPE_PHOTOACCESS_HELPER: {
            constructorRef = photoAccessHelperConstructor_;
            break;
        }
        default:
            constructorRef = sConstructor_;
            break;
    }
}

void FetchFileResultNapi::SolveConstructorRef(unique_ptr<FetchResult<AlbumOrder>> &fileResult,
    napi_ref &constructorRef)
{
    CHECK_NULL_PTR_RETURN_VOID(fileResult, "fileResult is nullptr");
    constructorRef = photoAccessHelperConstructor_;
}

napi_value FetchFileResultNapi::CreateFetchFileResult(napi_env env, unique_ptr<FetchResult<FileAsset>> fileResult)
{
    MediaLibraryTracer tracer;
    tracer.Start("CreateFetchFileResult");
    napi_value constructor;
    napi_ref constructorRef;

    FetchFileResultNapi::SolveConstructorRef(fileResult, constructorRef);
    NAPI_CALL(env, napi_get_reference_value(env, constructorRef, &constructor));
    sFetchResType_ = fileResult->GetFetchResType();
    sFetchFileResult_ = move(fileResult);
    napi_value result = nullptr;
    NAPI_CALL(env, napi_new_instance(env, constructor, 0, nullptr, &result));
    sFetchFileResult_ = nullptr;
    return result;
}

napi_value FetchFileResultNapi::CreateFetchFileResult(napi_env env, unique_ptr<FetchResult<AlbumAsset>> fileResult)
{
    MediaLibraryTracer tracer;
    tracer.Start("CreateFetchFileResult");
    napi_value constructor;
    napi_ref constructorRef;
    FetchFileResultNapi::SolveConstructorRef(fileResult, constructorRef);
    NAPI_CALL(env, napi_get_reference_value(env, constructorRef, &constructor));
    sFetchResType_ = fileResult->GetFetchResType();
    sFetchAlbumResult_ = move(fileResult);
    napi_value result = nullptr;
    NAPI_CALL(env, napi_new_instance(env, constructor, 0, nullptr, &result));
    sFetchAlbumResult_ = nullptr;
    return result;
}

napi_value FetchFileResultNapi::CreateFetchFileResult(napi_env env, unique_ptr<FetchResult<PhotoAlbum>> fileResult)
{
    MediaLibraryTracer tracer;
    tracer.Start("CreateFetchFileResult");
    napi_value constructor;
    napi_ref constructorRef;
    FetchFileResultNapi::SolveConstructorRef(fileResult, constructorRef);
    NAPI_CALL(env, napi_get_reference_value(env, constructorRef, &constructor));
    sFetchResType_ = fileResult->GetFetchResType();
    sFetchPhotoAlbumResult_ = move(fileResult);
    napi_value result = nullptr;
    NAPI_CALL(env, napi_new_instance(env, constructor, 0, nullptr, &result));
    sFetchPhotoAlbumResult_ = nullptr;
    return result;
}

napi_value FetchFileResultNapi::CreateFetchFileResult(napi_env env, unique_ptr<FetchResult<SmartAlbumAsset>> fileResult)
{
    MediaLibraryTracer tracer;
    tracer.Start("CreateFetchFileResult");
    napi_value constructor;
    napi_ref constructorRef;
    FetchFileResultNapi::SolveConstructorRef(fileResult, constructorRef);
    NAPI_CALL(env, napi_get_reference_value(env, constructorRef, &constructor));
    sFetchResType_ = fileResult->GetFetchResType();
    sFetchSmartAlbumResult_ = move(fileResult);
    napi_value result = nullptr;
    NAPI_CALL(env, napi_new_instance(env, constructor, 0, nullptr, &result));
    sFetchSmartAlbumResult_ = nullptr;
    return result;
}

napi_value FetchFileResultNapi::CreateFetchFileResult(napi_env env,
    unique_ptr<FetchResult<PhotoAssetCustomRecord>> fileResult)
{
    MediaLibraryTracer tracer;
    tracer.Start("CreateFetchFileResult");
    napi_value constructor;
    napi_ref constructorRef;
    FetchFileResultNapi::SolveConstructorRef(fileResult, constructorRef);
    NAPI_CALL(env, napi_get_reference_value(env, constructorRef, &constructor));
    sFetchResType_ = fileResult->GetFetchResType();
    sFetchPhotoAssetCustomRecordResult_ = move(fileResult);
    napi_value result = nullptr;
    NAPI_CALL(env, napi_new_instance(env, constructor, 0, nullptr, &result));
    sFetchPhotoAssetCustomRecordResult_ = nullptr;
    return result;
}

napi_value FetchFileResultNapi::CreateFetchFileResult(napi_env env,
    unique_ptr<FetchResult<AlbumOrder>> fileResult)
{
    MediaLibraryTracer tracer;
    tracer.Start("CreateFetchFileResult");
    napi_value constructor;
    napi_ref constructorRef;
    FetchFileResultNapi::SolveConstructorRef(fileResult, constructorRef);
    NAPI_CALL(env, napi_get_reference_value(env, constructorRef, &constructor));
    sFetchResType_ = fileResult->GetFetchResType();
    sFetchAlbumOrderResult_ = move(fileResult);
    napi_value result = nullptr;
    NAPI_CALL(env, napi_new_instance(env, constructor, 0, nullptr, &result));
    sFetchAlbumOrderResult_ = nullptr;
    return result;
}

std::shared_ptr<FetchResult<FileAsset>> FetchFileResultNapi::GetFetchFileResult() const
{
    return propertyPtr->fetchFileResult_;
}

napi_value FetchFileResultNapi::UserFileMgrInit(napi_env env, napi_value exports)
{
    NapiClassInfo info = {
        .name = UFM_FETCH_FILE_RESULT_CLASS_NAME,
        .ref = &userFileMgrConstructor_,
        .constructor = FetchFileResultNapiConstructor,
        .props = {
            DECLARE_NAPI_FUNCTION("getCount", JSGetCount),
            DECLARE_NAPI_FUNCTION("isAfterLast", JSIsAfterLast),
            DECLARE_NAPI_FUNCTION("getFirstObject", JSGetFirstObject),
            DECLARE_NAPI_FUNCTION("getNextObject", JSGetNextObject),
            DECLARE_NAPI_FUNCTION("getLastObject", JSGetLastObject),
            DECLARE_NAPI_FUNCTION("getPositionObject", JSGetPositionObject),
            DECLARE_NAPI_FUNCTION("getAllObject", JSGetAllObject),
            DECLARE_NAPI_FUNCTION("close", JSClose)
        }
    };
    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);
    return exports;
}

napi_value FetchFileResultNapi::PhotoAccessHelperInit(napi_env env, napi_value exports)
{
    NapiClassInfo info = {
        .name = PAH_FETCH_FILE_RESULT_CLASS_NAME,
        .ref = &photoAccessHelperConstructor_,
        .constructor = FetchFileResultNapiConstructor,
        .props = {
            DECLARE_NAPI_FUNCTION("getCount", JSGetCount),
            DECLARE_NAPI_FUNCTION("isAfterLast", JSIsAfterLast),
            DECLARE_NAPI_FUNCTION("getFirstObject", JSGetFirstObject),
            DECLARE_NAPI_FUNCTION("getNextObject", JSGetNextObject),
            DECLARE_NAPI_FUNCTION("getLastObject", JSGetLastObject),
            DECLARE_NAPI_FUNCTION("getObjectByPosition", JSGetPositionObject),
            DECLARE_NAPI_FUNCTION("getAllObjects", JSGetAllObject),
            DECLARE_NAPI_FUNCTION("getRangeObjects", JSGetRangeObjects),
            DECLARE_NAPI_FUNCTION("contains", JSContains),
            DECLARE_NAPI_FUNCTION("getObjectsByIndexSet", JSGetObjectsByIndexSet),
            DECLARE_NAPI_FUNCTION("getIndex", JSGetIndex),
            DECLARE_NAPI_FUNCTION("close", JSClose)
        }
    };
    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);
    return exports;
}

static bool CheckIfFFRNapiNotEmpty(FetchFileResultNapi* obj)
{
    if (obj == nullptr) {
        NAPI_INFO_LOG("FetchFileResultNapi is nullptr");
        return false;
    }
    if (obj->CheckIfPropertyPtrNull()) {
        NAPI_INFO_LOG("PropertyPtr in FetchFileResultNapi is nullptr");
        return false;
    }
    return true;
}

napi_value FetchFileResultNapi::JSGetCount(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FetchFileResultNapi* obj = nullptr;
    int32_t count = 0;
    napi_value thisVar = nullptr;

    MediaLibraryTracer tracer;
    tracer.Start("JSGetCount");

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("JSGetCount Invalid arguments!, status: %{public}d", status);
        NAPI_ASSERT(env, false, "JSGetCount thisVar == nullptr");
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if ((status == napi_ok) && CheckIfFFRNapiNotEmpty(obj)) {
        switch (obj->GetFetchResType()) {
            case FetchResType::TYPE_FILE:
                count = obj->GetFetchFileResultObject()->GetCount();
                break;
            case FetchResType::TYPE_ALBUM:
                count = obj->GetFetchAlbumResultObject()->GetCount();
                break;
            case FetchResType::TYPE_PHOTOALBUM:
                count = obj->GetFetchPhotoAlbumResultObject()->GetCount();
                break;
            case FetchResType::TYPE_SMARTALBUM:
                count = obj->GetFetchSmartAlbumResultObject()->GetCount();
                break;
            case FetchResType::TYPE_CUSTOMRECORD:
                count = obj->GetFetchCustomRecordResultObject()->GetCount();
                break;
            case FetchResType::TYPE_ALBUMORDER:
                count = obj->GetFetchAlbumOrderResultObject()->GetCount();
                break;
            default:
                NAPI_ERR_LOG("unsupported FetchResType");
                break;
        }
        if (count < 0) {
            NapiError::ThrowError(env, JS_INNER_FAIL, "Failed to get count");
            return nullptr;
        }
        napi_create_int32(env, count, &jsResult);
    } else {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to get native obj");
        return nullptr;
    }

    return jsResult;
}

napi_value FetchFileResultNapi::JSIsAfterLast(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FetchFileResultNapi* obj = nullptr;
    bool isAfterLast = false;
    napi_value thisVar = nullptr;

    MediaLibraryTracer tracer;
    tracer.Start("JSIsAfterLast");

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("JSIsAfterLast Invalid arguments!, status: %{public}d", status);
        NAPI_ASSERT(env, false, "JSIsAfterLast thisVar == nullptr");
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if ((status == napi_ok) && CheckIfFFRNapiNotEmpty(obj)) {
        switch (obj->GetFetchResType()) {
            case FetchResType::TYPE_FILE:
                isAfterLast = obj->GetFetchFileResultObject()->IsAtLastRow();
                break;
            case FetchResType::TYPE_ALBUM:
                isAfterLast = obj->GetFetchAlbumResultObject()->IsAtLastRow();
                break;
            case FetchResType::TYPE_PHOTOALBUM:
                isAfterLast = obj->GetFetchPhotoAlbumResultObject()->IsAtLastRow();
                break;
            case FetchResType::TYPE_SMARTALBUM:
                isAfterLast = obj->GetFetchSmartAlbumResultObject()->IsAtLastRow();
                break;
            case FetchResType::TYPE_CUSTOMRECORD:
                isAfterLast = obj->GetFetchCustomRecordResultObject()->IsAtLastRow();
                break;
            case FetchResType::TYPE_ALBUMORDER:
                isAfterLast = obj->GetFetchAlbumOrderResultObject()->IsAtLastRow();
                break;
            default:
                NAPI_ERR_LOG("unsupported FetchResType");
                break;
        }
        napi_get_boolean(env, isAfterLast, &jsResult);
    } else {
        NAPI_ERR_LOG("JSIsAfterLast obj == nullptr, status: %{public}d", status);
        NAPI_ASSERT(env, false, "JSIsAfterLast obj == nullptr");
    }

    return jsResult;
}

static void GetNapiResFromAsset(napi_env env, FetchFileResultAsyncContext *context,
    unique_ptr<JSAsyncContextOutput> &jsContext)
{
    napi_value jsAsset;
    CHECK_NULL_PTR_RETURN_VOID(context, "GetNapiResFromAsset async context is nullptr");
    switch (context->objectPtr->fetchResType_) {
        NAPI_INFO_LOG("GetNapiResFromAsset fetchResType: %{public}d",
            static_cast<int32_t>(context->objectPtr->fetchResType_));
        case FetchResType::TYPE_FILE:
            if (context->fileAsset != nullptr && context->objectPtr->fetchFileResult_ != nullptr) {
                context->fileAsset->SetUserId(context->objectPtr->fetchFileResult_->GetUserId());
            }
            jsAsset = FileAssetNapi::CreateFileAsset(env, context->fileAsset);
            break;
        case FetchResType::TYPE_ALBUM:
            jsAsset = AlbumNapi::CreateAlbumNapi(env, context->albumAsset);
            break;
        case FetchResType::TYPE_PHOTOALBUM:
            if (context->photoAlbum != nullptr && context->objectPtr->fetchPhotoAlbumResult_ != nullptr) {
                context->photoAlbum->SetUserId(context->objectPtr->fetchPhotoAlbumResult_->GetUserId());
            }
            jsAsset = PhotoAlbumNapi::CreatePhotoAlbumNapi(env, context->photoAlbum);
            break;
        case FetchResType::TYPE_SMARTALBUM:
            jsAsset = SmartAlbumNapi::CreateSmartAlbumNapi(env, context->smartAlbumAsset);
            break;
        case FetchResType::TYPE_CUSTOMRECORD:
            jsAsset = PhotoAssetCustomRecordNapi::CreateCustomRecordNapi(env, context->customRecordAsset);
            break;
        case FetchResType::TYPE_ALBUMORDER:
            jsAsset = AlbumOrderNapi::CreateAlbumOrderNapi(env, context->albumOrder);
            break;
        default:
            NAPI_ERR_LOG("unsupported FetchResType");
            break;
    }

    if (jsAsset == nullptr) {
        NAPI_ERR_LOG("Failed to get file asset napi object");
        napi_get_undefined(env, &jsContext->data);
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, JS_INNER_FAIL,
            "System inner fail");
    } else {
        jsContext->data = jsAsset;
        napi_get_undefined(env, &jsContext->error);
        jsContext->status = true;
    }
}

static void GetPositionObjectCompleteCallback(napi_env env, napi_status status, FetchFileResultAsyncContext* context)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetPositionObjectCompleteCallback");

    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    GetNapiResFromAsset(env, context, jsContext);

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }

    delete context;
}

napi_value FetchFileResultNapi::JSGetFirstObject(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    MediaLibraryTracer tracer;
    tracer.Start("JSGetFirstObject");

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_ONE, "requires 1 parameter");
    napi_get_undefined(env, &result);

    unique_ptr<FetchFileResultAsyncContext> asyncContext = make_unique<FetchFileResultAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && CheckIfFFRNapiNotEmpty(asyncContext->objectInfo)) {
        if (argc == ARGS_ONE) {
            GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        NAPI_CREATE_RESOURCE_NAME(env, resource, "JSGetFirstObject", asyncContext);

        asyncContext->objectPtr = asyncContext->objectInfo->propertyPtr;
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "propertyPtr is nullptr");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void *data) {
                auto context = static_cast<FetchFileResultAsyncContext*>(data);
                context->GetFirstAsset();
            },
            reinterpret_cast<napi_async_complete_callback>(GetPositionObjectCompleteCallback),
            static_cast<void *>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
            asyncContext.release();
        }
    } else {
        NAPI_ERR_LOG("JSGetFirstObject obj == nullptr, status: %{public}d", status);
        NAPI_ASSERT(env, false, "JSGetFirstObject obj == nullptr");
    }

    return result;
}

napi_value FetchFileResultNapi::JSGetNextObject(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    MediaLibraryTracer tracer;
    tracer.Start("JSGetNextObject");

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_ONE, "requires 1 parameter");

    napi_get_undefined(env, &result);
    unique_ptr<FetchFileResultAsyncContext> asyncContext = make_unique<FetchFileResultAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && CheckIfFFRNapiNotEmpty(asyncContext->objectInfo)) {
        if (argc == ARGS_ONE) {
            GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        NAPI_CREATE_RESOURCE_NAME(env, resource, "JSGetNextObject", asyncContext);

        asyncContext->objectPtr = asyncContext->objectInfo->propertyPtr;
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "propertyPtr is nullptr");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void *data) {
                auto context = static_cast<FetchFileResultAsyncContext*>(data);
                context->GetNextObject();
            },
            reinterpret_cast<napi_async_complete_callback>(GetPositionObjectCompleteCallback),
            static_cast<void *>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
            asyncContext.release();
        }
    } else {
        NAPI_ERR_LOG("JSGetNextObject obj == nullptr, status: %{public}d", status);
        NAPI_ASSERT(env, false, "JSGetNextObject obj == nullptr");
    }

    return result;
}

napi_value FetchFileResultNapi::JSGetLastObject(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    MediaLibraryTracer tracer;
    tracer.Start("JSGetLastObject");

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_ONE, "requires 1 parameter");

    napi_get_undefined(env, &result);
    unique_ptr<FetchFileResultAsyncContext> asyncContext = make_unique<FetchFileResultAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && CheckIfFFRNapiNotEmpty(asyncContext->objectInfo)) {
        if (argc == ARGS_ONE) {
            GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        NAPI_CREATE_RESOURCE_NAME(env, resource, "JSGetLastObject", asyncContext);

        asyncContext->objectPtr = asyncContext->objectInfo->propertyPtr;
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "propertyPtr is nullptr");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void *data) {
                auto context = static_cast<FetchFileResultAsyncContext*>(data);
                context->GetLastObject();
            },
            reinterpret_cast<napi_async_complete_callback>(GetPositionObjectCompleteCallback),
            static_cast<void *>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
            asyncContext.release();
        }
    } else {
        NAPI_ERR_LOG("JSGetLastObject obj == nullptr, status: %{public}d", status);
        NAPI_ASSERT(env, false, "JSGetLastObject obj == nullptr");
    }

    return result;
}

napi_value FetchFileResultNapi::JSGetPositionObject(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_valuetype type = napi_undefined;
    napi_value resource = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    MediaLibraryTracer tracer;
    tracer.Start("JSGetPositionObject");

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameter maximum");

    napi_get_undefined(env, &result);
    unique_ptr<FetchFileResultAsyncContext> asyncContext = make_unique<FetchFileResultAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && CheckIfFFRNapiNotEmpty(asyncContext->objectInfo)) {
        // Check the arguments and their types
        napi_typeof(env, argv[PARAM0], &type);
        if (type == napi_number) {
            napi_get_value_int32(env, argv[PARAM0], &(asyncContext->position));
        } else {
            NAPI_ERR_LOG("Argument mismatch, type: %{public}d", type);
            return result;
        }

        if (argc == ARGS_TWO) {
            GET_JS_ASYNC_CB_REF(env, argv[PARAM1], refCount, asyncContext->callbackRef);
        }

        NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        NAPI_CREATE_RESOURCE_NAME(env, resource, "JSGetPositionObject", asyncContext);

        asyncContext->objectPtr = asyncContext->objectInfo->propertyPtr;
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "propertyPtr is nullptr");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void *data) {
                auto context = static_cast<FetchFileResultAsyncContext*>(data);
                context->GetObjectAtPosition();
            },
            reinterpret_cast<napi_async_complete_callback>(GetPositionObjectCompleteCallback),
            static_cast<void *>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
            asyncContext.release();
        }
    } else {
        NAPI_ERR_LOG("JSGetPositionObject obj == nullptr, status: %{public}d", status);
        NAPI_ASSERT(env, false, "JSGetPositionObject obj == nullptr");
    }

    return result;
}

static napi_value GetAsset(napi_env env, vector<std::unique_ptr<FileAsset>> &array, int index)
{
    return FileAssetNapi::CreateFileAsset(env, array[index]);
}

static napi_value GetAsset(napi_env env, vector<std::unique_ptr<AlbumAsset>> &array, int index)
{
    return AlbumNapi::CreateAlbumNapi(env, array[index]);
}

static napi_value GetAsset(napi_env env, vector<std::unique_ptr<PhotoAlbum>> &array, int index)
{
    return PhotoAlbumNapi::CreatePhotoAlbumNapi(env, array[index]);
}

static napi_value GetAsset(napi_env env, vector<std::unique_ptr<SmartAlbumAsset>> &array, int index)
{
    return SmartAlbumNapi::CreateSmartAlbumNapi(env, array[index]);
}

static napi_value GetAsset(napi_env env, vector<std::unique_ptr<AlbumOrder>> &array, int index)
{
    return AlbumOrderNapi::CreateAlbumOrderNapi(env, array[index]);
}

static napi_value GetAsset(napi_env env, vector<std::unique_ptr<PhotoAssetCustomRecord>> &array, int index)
{
    return PhotoAssetCustomRecordNapi::CreateCustomRecordNapi(env, array[index]);
}

template<class T>
static void GetAssetFromArray(napi_env env, FetchFileResultAsyncContext* context, T& array,
    unique_ptr<JSAsyncContextOutput> &jsContext)
{
    napi_value jsFileArray = nullptr;
    napi_create_array_with_length(env, array.size(), &jsFileArray);
    napi_value jsFileAsset = nullptr;
    size_t i = 0;
    for (i = 0; i < array.size(); i++) {
        jsFileAsset = GetAsset(env, array, i);
        if ((jsFileAsset == nullptr) || (napi_set_element(env, jsFileArray, i, jsFileAsset) != napi_ok)) {
            NAPI_ERR_LOG("Failed to get file asset napi object");
            napi_get_undefined(env, &jsContext->data);
            MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_MEM_ALLOCATION,
                "Failed to create js object");
            break;
        }
    }
    if (i == array.size()) {
        jsContext->data = jsFileArray;
        napi_get_undefined(env, &jsContext->error);
        jsContext->status = true;
    }
}

static void GetAllObjectCompleteCallback(napi_env env, napi_status status, FetchFileResultAsyncContext* context)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetAllObjectCompleteCallback");
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    switch (context->objectPtr->fetchResType_) {
        case FetchResType::TYPE_FILE:
            GetAssetFromArray(env, context, context->fileAssetArray, jsContext);
            break;
        case FetchResType::TYPE_ALBUM:
            GetAssetFromArray(env, context, context->fileAlbumArray, jsContext);
            break;
        case FetchResType::TYPE_PHOTOALBUM:
            GetAssetFromArray(env, context, context->filePhotoAlbumArray, jsContext);
            break;
        case FetchResType::TYPE_SMARTALBUM:
            GetAssetFromArray(env, context, context->fileSmartAlbumArray, jsContext);
            break;
        case FetchResType::TYPE_ALBUMORDER:
            GetAssetFromArray(env, context, context->fileAlbumOrderArray, jsContext);
            break;
        case FetchResType::TYPE_CUSTOMRECORD:
            GetAssetFromArray(env, context, context->customRecordArray, jsContext);
            break;
        default:
            NAPI_ERR_LOG("unsupported FetchResType");
            napi_get_undefined(env, &jsContext->data);
            MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
                "Failed to obtain fileAsset array from DB");
    }

    if (context->work != nullptr) {
        int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
        int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
        int64_t normalTime = 500;
        if ((long)(end - start) >= normalTime) {
            NAPI_INFO_LOG("InvokeJSAsync dir cost: %{public}ld", (long)(end - start));
        }
    }

    delete context;
}

std::shared_ptr<FetchResult<FileAsset>> FetchFileResultNapi::GetFetchFileResultObject()
{
    return propertyPtr->fetchFileResult_;
}

std::shared_ptr<FetchResult<AlbumAsset>> FetchFileResultNapi::GetFetchAlbumResultObject()
{
    return propertyPtr->fetchAlbumResult_;
}

std::shared_ptr<FetchResult<PhotoAlbum>> FetchFileResultNapi::GetFetchPhotoAlbumResultObject()
{
    return propertyPtr->fetchPhotoAlbumResult_;
}

std::shared_ptr<FetchResult<SmartAlbumAsset>> FetchFileResultNapi::GetFetchSmartAlbumResultObject()
{
    return propertyPtr->fetchSmartAlbumResult_;
}

std::shared_ptr<FetchResult<PhotoAssetCustomRecord>> FetchFileResultNapi::GetFetchCustomRecordResultObject()
{
    if (propertyPtr == nullptr) {
        return nullptr;
    }
    return propertyPtr->fetchCustomRecordResult_;
}

std::shared_ptr<FetchResult<AlbumOrder>> FetchFileResultNapi::GetFetchAlbumOrderResultObject()
{
    CHECK_NULLPTR_RET(propertyPtr);
    return propertyPtr->fetchAlbumOrderResult_;
}

void GetAllObjectFromFetchResult(const FetchFileResultAsyncContext &asyncContext)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetAllObjectFromFetchResult");

    FetchFileResultAsyncContext *context = const_cast<FetchFileResultAsyncContext *>(&asyncContext);
    context->GetAllObjectFromFetchResult();
}

napi_value FetchFileResultNapi::JSGetAllObject(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    MediaLibraryTracer tracer;
    tracer.Start("JSGetAllObject");

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_ONE, "requires 1 parameter maximum");

    napi_get_undefined(env, &result);
    unique_ptr<FetchFileResultAsyncContext> asyncContext = make_unique<FetchFileResultAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && CheckIfFFRNapiNotEmpty(asyncContext->objectInfo)) {
        if (argc == ARGS_ONE) {
            GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        NAPI_CREATE_RESOURCE_NAME(env, resource, "JSGetAllObject", asyncContext);

        asyncContext->objectPtr = asyncContext->objectInfo->propertyPtr;
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "propertyPtr is nullptr");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void *data) {
                auto context = static_cast<FetchFileResultAsyncContext*>(data);
                GetAllObjectFromFetchResult(*context);
            },
            reinterpret_cast<napi_async_complete_callback>(GetAllObjectCompleteCallback),
            static_cast<void *>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
            asyncContext.release();
        }
    } else {
        NAPI_ERR_LOG("JSGetAllObject obj == nullptr, status: %{public}d", status);
        NAPI_ASSERT(env, false, "JSGetAllObject obj == nullptr");
    }

    return result;
}

void GetCountFromObject(FetchFileResultNapi* obj, int32_t &count)
{
    switch (obj->GetFetchResType()) {
        case FetchResType::TYPE_FILE:
            count = obj->GetFetchFileResultObject()->GetCount();
            break;
        case FetchResType::TYPE_ALBUM:
            count = obj->GetFetchAlbumResultObject()->GetCount();
            break;
        case FetchResType::TYPE_PHOTOALBUM:
            count = obj->GetFetchPhotoAlbumResultObject()->GetCount();
            break;
        case FetchResType::TYPE_SMARTALBUM:
            count = obj->GetFetchSmartAlbumResultObject()->GetCount();
            break;
        case FetchResType::TYPE_CUSTOMRECORD:
            count = obj->GetFetchCustomRecordResultObject()->GetCount();
            break;
        case FetchResType::TYPE_ALBUMORDER:
            count = obj->GetFetchAlbumOrderResultObject()->GetCount();
            break;
        default:
            NAPI_ERR_LOG("unsupported FetchResType");
            break;
    }
}

napi_value FetchFileResultNapi::ProcessValidContext(
    napi_env env, unique_ptr<FetchFileResultAsyncContext> &asyncContext, napi_value argv[], napi_value &result)
{
    napi_valuetype type = napi_undefined;
    // Parse offset parameter
    napi_typeof(env, argv[PARAM0], &type);
    if (type == napi_number) {
        napi_get_value_int32(env, argv[PARAM0], &(asyncContext->offset));
    } else {
        NAPI_ERR_LOG("Invalid offset type: %{public}d", type);
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Invalid offset type");
        return nullptr;
    }
    // Parse length parameter
    napi_typeof(env, argv[PARAM1], &type);
    if (type == napi_number) {
        napi_get_value_int32(env, argv[PARAM1], &(asyncContext->length));
    } else {
        NAPI_ERR_LOG("Invalid length type: %{public}d", type);
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Invalid length type");
        return nullptr;
    }
    int32_t total_count = 0;
    GetCountFromObject(asyncContext->objectInfo, total_count);
    if (asyncContext->offset < 0 || asyncContext->length <= 0 ||
        (asyncContext->offset + asyncContext->length) > total_count) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Index exceeds range of objects");
        return nullptr;
    }
    NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
    napi_value resource = nullptr;
    NAPI_CREATE_RESOURCE_NAME(env, resource, "JSGetRangeObjects", asyncContext);
    asyncContext->objectPtr = asyncContext->objectInfo->propertyPtr;
    return resource;
}

napi_value FetchFileResultNapi::JSGetRangeObjects(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    size_t argc = ARGS_THREE;
    napi_value argv[ARGS_THREE] = {0};
    napi_value thisVar = nullptr;
    MediaLibraryTracer tracer;
    tracer.Start("JSGetRangeObjects");
    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_THREE, "requires 3 parameter maximum");
    napi_get_undefined(env, &result);
    unique_ptr<FetchFileResultAsyncContext> asyncContext = make_unique<FetchFileResultAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && CheckIfFFRNapiNotEmpty(asyncContext->objectInfo)) {
        if (argc == ARGS_THREE) {
            GET_JS_ASYNC_CB_REF(env, argv[PARAM2], refCount, asyncContext->callbackRef);
        }
        napi_value resource = ProcessValidContext(env, asyncContext, argv, result);
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "propertyPtr is nullptr");
        napi_status status = napi_create_async_work(
            env,
            nullptr,
            resource,
            [](napi_env env, void *data) {
                auto context = static_cast<FetchFileResultAsyncContext *>(data);
                context->GetObjectsInRange();
            },
            reinterpret_cast<napi_async_complete_callback>(GetAllObjectCompleteCallback),
            static_cast<void *>(asyncContext.get()),
            &asyncContext->work);
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
            asyncContext.release();
        }
    } else {
        NAPI_ERR_LOG("JSGetRangeObjects obj == nullptr, status: %{public}d", status);
        NAPI_ASSERT(env, false, "JSGetRangeObjects obj == nullptr");
    }
    return result;
}

static void GetContainsCompleteCallback(napi_env env, napi_status status, FetchFileResultAsyncContext* context)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetContainsCompleteCallback");
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_E_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_E_INNER_FAIL);

    if (context->error == ERR_DEFAULT) {
        CHECK_ARGS_RET_VOID(env,
            napi_get_boolean(env, context->fetchResultIndexId != -1, &jsContext->data), JS_E_INNER_FAIL);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

static void GetIndexCompleteCallback(napi_env env, napi_status status, FetchFileResultAsyncContext* context)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetIndexCompleteCallback");
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_E_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_E_INNER_FAIL);

    if (context->error == ERR_DEFAULT) {
        CHECK_ARGS_RET_VOID(env,
            napi_create_int32(env, context->fetchResultIndexId, &jsContext->data), JS_E_INNER_FAIL);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

static napi_status CheckArgValuePre(napi_env env, void* data, const napi_value arg)
{
    auto* context = static_cast<FetchFileResultAsyncContext*>(data);
    if (context == nullptr) {
        NAPI_ERR_LOG("context is nullptr.");
        return napi_invalid_arg;
    }
    if (context->objectInfo == nullptr) {
        NAPI_ERR_LOG("context->objectInfo is nullptr.");
        return napi_invalid_arg;
    }
    switch (context->objectInfo->GetFetchResType()) {
        case FetchResType::TYPE_FILE:
            CHECK_STATUS_RET(napi_unwrap(env, arg, reinterpret_cast<void**>(&context->fileAssetParameter)),
                "Failed to unwrap arg.");
            CHECK_COND_RET(context->fileAssetParameter != nullptr, napi_invalid_arg,
                "Failed to get FileAssetNapi object");
            context->indexObjectId = context->fileAssetParameter->GetFileId();
            break;
        case FetchResType::TYPE_ALBUM:
            CHECK_STATUS_RET(napi_unwrap(env, arg, reinterpret_cast<void**>(&context->albumAssetParameter)),
                "Failed to unwrap arg.");
            CHECK_COND_RET(context->albumAssetParameter != nullptr, napi_invalid_arg,
                "Failed to get AlbumAssetNapi object");
            context->indexObjectId = context->albumAssetParameter->GetAlbumId();
            break;
        case FetchResType::TYPE_PHOTOALBUM:
            CHECK_STATUS_RET(napi_unwrap(env, arg, reinterpret_cast<void**>(&context->photoAlbumParameter)),
                "Failed to unwrap arg.");
            CHECK_COND_RET(context->photoAlbumParameter != nullptr, napi_invalid_arg,
                "Failed to get PhotoAlbumNapi object");
            context->indexObjectId = context->photoAlbumParameter->GetAlbumId();
            context->albumType = static_cast<int32_t>(context->photoAlbumParameter->GetPhotoAlbumType());
            context->albumSubType = static_cast<int32_t>(context->photoAlbumParameter->GetPhotoAlbumSubType());
            break;
        default:
            NAPI_INFO_LOG("need more check.");
            return napi_invalid_arg;
    }
    return napi_ok;
}

static napi_status CheckArgValue(napi_env env, void* data, const napi_value arg)
{
    if (CheckArgValuePre(env, data, arg) == napi_ok) {
        return napi_ok;
    }
    auto* context = static_cast<FetchFileResultAsyncContext*>(data);
    if (context == nullptr) {
        NAPI_ERR_LOG("context is nullptr.");
        return napi_invalid_arg;
    }
    if (context->objectInfo == nullptr) {
        NAPI_ERR_LOG("context->objectInfo is nullptr.");
        return napi_invalid_arg;
    }
    switch (context->objectInfo->GetFetchResType()) {
        case FetchResType::TYPE_SMARTALBUM:
            CHECK_STATUS_RET(napi_unwrap(env, arg, reinterpret_cast<void**>(&context->smartAlbumAssetParameter)),
                "Failed to unwrap arg.");
            CHECK_COND_RET(context->smartAlbumAssetParameter != nullptr, napi_invalid_arg,
                "Failed to get SmartAlbumNapi object");
            context->indexObjectId = context->smartAlbumAssetParameter->GetSmartAlbumId();
            break;
        case FetchResType::TYPE_CUSTOMRECORD:
            CHECK_STATUS_RET(napi_unwrap(env, arg, reinterpret_cast<void**>(&context->customRecordAssetParameter)),
                "Failed to unwrap arg.");
            CHECK_COND_RET(context->customRecordAssetParameter != nullptr, napi_invalid_arg,
                "Failed to get CustomRecordNapi object");
            context->indexObjectId = context->customRecordAssetParameter->GetFileId();
            break;
        case FetchResType::TYPE_ALBUMORDER:
            CHECK_STATUS_RET(napi_unwrap(env, arg, reinterpret_cast<void**>(&context->albumOrderParameter)),
                "Failed to unwrap arg.");
            CHECK_COND_RET(context->albumOrderParameter != nullptr, napi_invalid_arg,
                "Failed to get AlbumOrderNapi object");
            context->indexObjectId = context->albumOrderParameter->GetAlbumId();
            break;
        default:
            NAPI_ERR_LOG("unsupported FetchResType");
            return napi_invalid_arg;
    }
    return napi_ok;
}

static napi_status ProcessIndexSet(napi_env env, void* data, const napi_value arg)
{
    auto* context = static_cast<FetchFileResultAsyncContext*>(data);
    if (context == nullptr) {
        NAPI_ERR_LOG("context is nullptr.");
        return napi_invalid_arg;
    }
    if (context->objectInfo == nullptr) {
        NAPI_ERR_LOG("context->objectInfo is nullptr.");
        return napi_invalid_arg;
    }
    napi_valuetype valueType;
    napi_typeof(env, arg, &valueType);
    if (valueType == napi_undefined || valueType == napi_null) {
        NAPI_ERR_LOG("JSGetObjectsByIndexSet indexSet is null or undefined");
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "indexSet is null or undefined");
        return napi_invalid_arg;
    }
    napi_status status = MediaLibraryNapiUtils::GetInt32Array(env, arg, context->indexSet);
    if (status != napi_ok) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "indexSet is not int.");
        return napi_invalid_arg;
    }
    if (context->indexSet.empty()) {
        NAPI_ERR_LOG("JSGetObjectsByIndexSet indexSet is empty!");
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "indexSet is empty");
        return napi_invalid_arg;
    }
    if (context->indexSet.size() > MAX_SIZE) {
        NAPI_ERR_LOG("JSGetObjectsByIndexSet indexSet.size exceeds MAX_SIZE");
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "indexSet.size exceeds MAX_SIZE");
        return napi_invalid_arg;
    }
    std::set<int32_t> tempIndexSet(context->indexSet.begin(), context->indexSet.end());
    int32_t minIndex = *tempIndexSet.begin();
    int32_t maxIndex = *tempIndexSet.rbegin();
    int32_t total_count = 0;
    GetCountFromObject(context->objectInfo, total_count);
    if (minIndex < 0 || maxIndex >= total_count) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Index exceeds range of objects");
        return napi_invalid_arg;
    }
    return napi_ok;
}

napi_value FetchFileResultNapi::JSContains(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = { 0 };
    napi_value thisVar = nullptr;
    napi_valuetype valueType;
    MediaLibraryTracer tracer;
    tracer.Start("JSContains");

    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr), JS_E_INNER_FAIL);
    NAPI_ASSERT(env, argc == ARGS_ONE, "Number of args is invalid");
    CHECK_ARGS(env, napi_typeof(env, argv[PARAM0], &valueType), JS_E_INNER_FAIL);
    NAPI_ASSERT(env, valueType == napi_object || valueType == napi_undefined || valueType == napi_null,
        "Invalid argument type");
    napi_get_undefined(env, &result);
    unique_ptr<FetchFileResultAsyncContext> asyncContext = make_unique<FetchFileResultAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && CheckIfFFRNapiNotEmpty(asyncContext->objectInfo)) {
        if (valueType == napi_object) {
            status = CheckArgValue(env, asyncContext.get(), argv[PARAM0]);
            NAPI_ASSERT(env, status == napi_ok, "CheckArgValue failed");
        }
        NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        napi_value resource = nullptr;
        NAPI_CREATE_RESOURCE_NAME(env, resource, "JSContains", asyncContext);
        asyncContext->objectPtr = asyncContext->objectInfo->propertyPtr;
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "propertyPtr is nullptr");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void *data) {
                auto context = static_cast<FetchFileResultAsyncContext*>(data);
                context->fetchResultIndexId = context->GetObjectIndexById();
            },
            reinterpret_cast<napi_async_complete_callback>(GetContainsCompleteCallback),
            static_cast<void *>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
            asyncContext.release();
        }
    } else {
        NAPI_ERR_LOG("JSContains obj == nullptr, status: %{public}d", status);
        NAPI_ASSERT(env, false, "JSContains obj == nullptr");
    }
    return result;
}

napi_value FetchFileResultNapi::JSGetObjectsByIndexSet(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = { 0 };
    napi_value thisVar = nullptr;
    MediaLibraryTracer tracer;
    tracer.Start("JSGetObjectsByIndexSet");

    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr), JS_E_INNER_FAIL);
    NAPI_ASSERT(env, argc == ARGS_ONE, "Number of args is invalid");
    bool isArray = false;
    CHECK_ARGS(env, napi_is_array(env, argv[PARAM0], &isArray), JS_E_PARAM_INVALID);
    napi_get_undefined(env, &result);
    unique_ptr<FetchFileResultAsyncContext> asyncContext = make_unique<FetchFileResultAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && CheckIfFFRNapiNotEmpty(asyncContext->objectInfo)) {
        status = ProcessIndexSet(env, asyncContext.get(), argv[PARAM0]);
        NAPI_ASSERT(env, status == napi_ok, "ProcessIndexSet failed");
        NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        napi_value resource = nullptr;
        NAPI_CREATE_RESOURCE_NAME(env, resource, "JSGetObjectsByIndexSet", asyncContext);
        asyncContext->objectPtr = asyncContext->objectInfo->propertyPtr;
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "propertyPtr is nullptr");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void *data) {
                auto context = static_cast<FetchFileResultAsyncContext *>(data);
                context->GetObjectsByIndexSet();
            },
            reinterpret_cast<napi_async_complete_callback>(GetAllObjectCompleteCallback),
            static_cast<void *>(asyncContext.get()),
            &asyncContext->work);
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
            asyncContext.release();
        }
    } else {
        NAPI_ERR_LOG("JSGetObjectsByIndexSet obj == nullptr, status: %{public}d", status);
        NAPI_ASSERT(env, false, "JSGetObjectsByIndexSet obj == nullptr");
    }
    return result;
}

napi_value FetchFileResultNapi::JSGetIndex(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = { 0 };
    napi_value thisVar = nullptr;
    napi_valuetype valueType;
    MediaLibraryTracer tracer;
    tracer.Start("JSGetIndex");

    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr), JS_E_INNER_FAIL);
    NAPI_ASSERT(env, argc == ARGS_ONE, "Number of args is invalid");
    CHECK_ARGS(env, napi_typeof(env, argv[PARAM0], &valueType), JS_E_INNER_FAIL);
    NAPI_ASSERT(env, valueType == napi_object || valueType == napi_undefined || valueType == napi_null,
        "Invalid argument type");
    napi_get_undefined(env, &result);
    unique_ptr<FetchFileResultAsyncContext> asyncContext = make_unique<FetchFileResultAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && CheckIfFFRNapiNotEmpty(asyncContext->objectInfo)) {
        if (valueType == napi_object) {
            status = CheckArgValue(env, asyncContext.get(), argv[PARAM0]);
            NAPI_ASSERT(env, status == napi_ok, "CheckArgValue failed");
        }
        NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        napi_value resource = nullptr;
        NAPI_CREATE_RESOURCE_NAME(env, resource, "JSGetIndex", asyncContext);
        asyncContext->objectPtr = asyncContext->objectInfo->propertyPtr;
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "propertyPtr is nullptr");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void *data) {
                auto context = static_cast<FetchFileResultAsyncContext*>(data);
                context->fetchResultIndexId = context->GetObjectIndexById();
            },
            reinterpret_cast<napi_async_complete_callback>(GetIndexCompleteCallback),
            static_cast<void *>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
            asyncContext.release();
        }
    } else {
        NAPI_ERR_LOG("JSGetIndex obj == nullptr, status: %{public}d", status);
        NAPI_ASSERT(env, false, "JSGetIndex obj == nullptr");
    }
    return result;
}

napi_value FetchFileResultNapi::JSClose(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FetchFileResultNapi* obj = nullptr;
    napi_value thisVar = nullptr;

    MediaLibraryTracer tracer;
    tracer.Start("JSClose");

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments!, status: %{public}d", status);
        return jsResult;
    }
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if ((status == napi_ok) && (obj != nullptr)) {
        obj->propertyPtr = nullptr;
    }
    status = napi_remove_wrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if ((status == napi_ok) && (obj != nullptr)) {
        napi_create_int32(env, E_SUCCESS, &jsResult);
    } else {
        NAPI_DEBUG_LOG("JSClose obj == nullptr");
    }

    NAPI_DEBUG_LOG("JSClose OUT!");
    return jsResult;
}

void FetchFileResultAsyncContext::GetFirstAsset()
{
    switch (objectPtr->fetchResType_) {
        case FetchResType::TYPE_FILE: {
            fileAsset = objectPtr->fetchFileResult_->GetFirstObject();
            break;
        }
        case FetchResType::TYPE_ALBUM: {
            albumAsset = objectPtr->fetchAlbumResult_->GetFirstObject();
            break;
        }
        case FetchResType::TYPE_PHOTOALBUM: {
            photoAlbum = objectPtr->fetchPhotoAlbumResult_->GetFirstObject();
            break;
        }
        case FetchResType::TYPE_SMARTALBUM: {
            smartAlbumAsset = objectPtr->fetchSmartAlbumResult_->GetFirstObject();
            break;
        }
        case FetchResType::TYPE_CUSTOMRECORD: {
            CHECK_NULL_PTR_RETURN_VOID(objectPtr, "objectPtr is nullptr");
            CHECK_NULL_PTR_RETURN_VOID(objectPtr->fetchCustomRecordResult_, "fetchCustomRecordResult_ is nullptr");
            customRecordAsset = objectPtr->fetchCustomRecordResult_->GetFirstObject();
            break;
        }
        case FetchResType::TYPE_ALBUMORDER: {
            CHECK_NULL_PTR_RETURN_VOID(objectPtr, "objectPtr is nullptr");
            CHECK_NULL_PTR_RETURN_VOID(objectPtr->fetchAlbumOrderResult_, "fetchAlbumOrderResult_ is nullptr");
            albumOrder = objectPtr->fetchAlbumOrderResult_->GetFirstObject();
            break;
        }
        default:
            NAPI_ERR_LOG("unsupported FetchResType");
            break;
    }
}

void FetchFileResultAsyncContext::GetObjectAtPosition()
{
    switch (objectPtr->fetchResType_) {
        case FetchResType::TYPE_FILE: {
            fileAsset = objectPtr->fetchFileResult_->GetObjectAtPosition(position);
            break;
        }
        case FetchResType::TYPE_ALBUM: {
            albumAsset = objectPtr->fetchAlbumResult_->GetObjectAtPosition(position);
            break;
        }
        case FetchResType::TYPE_PHOTOALBUM: {
            photoAlbum = objectPtr->fetchPhotoAlbumResult_->GetObjectAtPosition(position);
            break;
        }
        case FetchResType::TYPE_SMARTALBUM: {
            smartAlbumAsset = objectPtr->fetchSmartAlbumResult_->GetObjectAtPosition(position);
            break;
        }
        case FetchResType::TYPE_CUSTOMRECORD: {
            CHECK_NULL_PTR_RETURN_VOID(objectPtr, "objectPtr is nullptr");
            CHECK_NULL_PTR_RETURN_VOID(objectPtr->fetchCustomRecordResult_, "fetchCustomRecordResult_ is nullptr");
            customRecordAsset = objectPtr->fetchCustomRecordResult_->GetObjectAtPosition(position);
            break;
        }
        case FetchResType::TYPE_ALBUMORDER: {
            CHECK_NULL_PTR_RETURN_VOID(objectPtr, "objectPtr is nullptr");
            CHECK_NULL_PTR_RETURN_VOID(objectPtr->fetchAlbumOrderResult_, "fetchAlbumOrderResult_ is nullptr");
            albumOrder = objectPtr->fetchAlbumOrderResult_->GetObjectAtPosition(position);
            break;
        }
        default:
            NAPI_ERR_LOG("unsupported FetchResType");
            break;
    }
}

void FetchFileResultAsyncContext::GetLastObject()
{
    switch (objectPtr->fetchResType_) {
        case FetchResType::TYPE_FILE: {
            fileAsset = objectPtr->fetchFileResult_->GetLastObject();
            break;
        }
        case FetchResType::TYPE_ALBUM: {
            albumAsset = objectPtr->fetchAlbumResult_->GetLastObject();
            break;
        }
        case FetchResType::TYPE_PHOTOALBUM: {
            photoAlbum = objectPtr->fetchPhotoAlbumResult_->GetLastObject();
            break;
        }
        case FetchResType::TYPE_SMARTALBUM: {
            smartAlbumAsset = objectPtr->fetchSmartAlbumResult_->GetLastObject();
            break;
        }
        case FetchResType::TYPE_CUSTOMRECORD: {
            CHECK_NULL_PTR_RETURN_VOID(objectPtr, "objectPtr is nullptr");
            CHECK_NULL_PTR_RETURN_VOID(objectPtr->fetchCustomRecordResult_, "fetchCustomRecordResult_ is nullptr");
            customRecordAsset = objectPtr->fetchCustomRecordResult_->GetLastObject();
            break;
        }
        case FetchResType::TYPE_ALBUMORDER: {
            CHECK_NULL_PTR_RETURN_VOID(objectPtr, "objectPtr is nullptr");
            CHECK_NULL_PTR_RETURN_VOID(objectPtr->fetchAlbumOrderResult_, "fetchAlbumOrderResult_ is nullptr");
            albumOrder = objectPtr->fetchAlbumOrderResult_->GetLastObject();
            break;
        }
        default:
            NAPI_ERR_LOG("unsupported FetchResType");
            break;
    }
}

void FetchFileResultAsyncContext::GetNextObject()
{
    switch (objectPtr->fetchResType_) {
        case FetchResType::TYPE_FILE: {
            fileAsset = objectPtr->fetchFileResult_->GetNextObject();
            break;
        }
        case FetchResType::TYPE_ALBUM: {
            albumAsset = objectPtr->fetchAlbumResult_->GetNextObject();
            break;
        }
        case FetchResType::TYPE_PHOTOALBUM: {
            photoAlbum = objectPtr->fetchPhotoAlbumResult_->GetNextObject();
            break;
        }
        case FetchResType::TYPE_SMARTALBUM: {
            smartAlbumAsset = objectPtr->fetchSmartAlbumResult_->GetNextObject();
            break;
        }
        case FetchResType::TYPE_CUSTOMRECORD: {
            CHECK_NULL_PTR_RETURN_VOID(objectPtr, "objectPtr is nullptr");
            CHECK_NULL_PTR_RETURN_VOID(objectPtr->fetchCustomRecordResult_, "fetchCustomRecordResult_ is nullptr");
            customRecordAsset = objectPtr->fetchCustomRecordResult_->GetNextObject();
            break;
        }
        case FetchResType::TYPE_ALBUMORDER: {
            CHECK_NULL_PTR_RETURN_VOID(objectPtr, "objectPtr is nullptr");
            CHECK_NULL_PTR_RETURN_VOID(objectPtr->fetchAlbumOrderResult_, "fetchAlbumOrderResult_ is nullptr");
            albumOrder = objectPtr->fetchAlbumOrderResult_->GetNextObject();
            break;
        }
        default:
            NAPI_ERR_LOG("unsupported FetchResType");
            break;
    }
}

void FetchFileResultAsyncContext::GetAllObjectFromFetchResult()
{
    switch (objectPtr->fetchResType_) {
        case FetchResType::TYPE_FILE: {
            auto fetchResult = objectPtr->fetchFileResult_;
            auto file = fetchResult->GetFirstObject();
            while (file != nullptr) {
                file->SetUserId(fetchResult->GetUserId());
                fileAssetArray.push_back(move(file));
                file = fetchResult->GetNextObject();
            }
            break;
        }
        case FetchResType::TYPE_ALBUM: {
            auto fetchResult = objectPtr->fetchAlbumResult_;
            auto album = fetchResult->GetFirstObject();
            while (album != nullptr) {
                fileAlbumArray.push_back(move(album));
                album = fetchResult->GetNextObject();
            }
            break;
        }
        case FetchResType::TYPE_PHOTOALBUM: {
            auto fetchResult = objectPtr->fetchPhotoAlbumResult_;
            auto photoAlbum = fetchResult->GetFirstObject();
            while (photoAlbum != nullptr) {
                photoAlbum->SetUserId(fetchResult->GetUserId());
                filePhotoAlbumArray.push_back(move(photoAlbum));
                photoAlbum = fetchResult->GetNextObject();
            }
            break;
        }
        case FetchResType::TYPE_SMARTALBUM: {
            auto fetchResult = objectPtr->fetchSmartAlbumResult_;
            auto smartAlbum = fetchResult->GetFirstObject();
            while (smartAlbum != nullptr) {
                fileSmartAlbumArray.push_back(move(smartAlbum));
                smartAlbum = fetchResult->GetNextObject();
            }
            break;
        }
        case FetchResType::TYPE_CUSTOMRECORD: {
            CHECK_NULL_PTR_RETURN_VOID(objectPtr, "objectPtr is nullptr");
            CHECK_NULL_PTR_RETURN_VOID(objectPtr->fetchCustomRecordResult_, "fetchCustomRecordResult_ is nullptr");
            auto fetchResult = objectPtr->fetchCustomRecordResult_;
            auto customRecord = fetchResult->GetFirstObject();
            while (customRecord != nullptr) {
                customRecordArray.push_back(move(customRecord));
                customRecord = fetchResult->GetNextObject();
            }
            break;
        }
        case FetchResType::TYPE_ALBUMORDER: {
            CHECK_NULL_PTR_RETURN_VOID(objectPtr, "objectPtr is nullptr");
            CHECK_NULL_PTR_RETURN_VOID(objectPtr->fetchAlbumOrderResult_, "fetchAlbumOrderResult_ is nullptr");
            auto fetchResult = objectPtr->fetchAlbumOrderResult_;
            auto albumOrder = fetchResult->GetFirstObject();
            while (albumOrder != nullptr) {
                fileAlbumOrderArray.push_back(move(albumOrder));
                albumOrder = fetchResult->GetNextObject();
            }
            break;
        }
        default:
            NAPI_ERR_LOG("unsupported FetchResType");
            break;
    }
}

void FetchFileResultAsyncContext::GetPhotosInRange()
{
    switch (objectPtr->fetchResType_) {
        case FetchResType::TYPE_ALBUM: {
            auto fetchResult = objectPtr->fetchAlbumResult_;
            auto album = fetchResult->GetObjectAtPosition(offset);
            while (album != nullptr && (length--) > 0) {
                fileAlbumArray.push_back(move(album));
                album = fetchResult->GetNextObject();
            }
            break;
        }
        case FetchResType::TYPE_PHOTOALBUM: {
            auto fetchResult = objectPtr->fetchPhotoAlbumResult_;
            auto photoAlbum = fetchResult->GetObjectAtPosition(offset);
            while (photoAlbum != nullptr && (length--) > 0) {
                photoAlbum->SetUserId(fetchResult->GetUserId());
                filePhotoAlbumArray.push_back(move(photoAlbum));
                photoAlbum = fetchResult->GetNextObject();
            }
            break;
        }
        case FetchResType::TYPE_SMARTALBUM: {
            auto fetchResult = objectPtr->fetchSmartAlbumResult_;
            auto smartAlbum = fetchResult->GetObjectAtPosition(offset);
            while (smartAlbum != nullptr && (length--) > 0) {
                fileSmartAlbumArray.push_back(move(smartAlbum));
                smartAlbum = fetchResult->GetNextObject();
            }
            break;
        }
        default:
            break;
    }
}

void FetchFileResultAsyncContext::GetObjectsInRange()
{
    FetchFileResultAsyncContext::GetPhotosInRange();
    switch (objectPtr->fetchResType_) {
        case FetchResType::TYPE_FILE: {
            auto fetchResult = objectPtr->fetchFileResult_;
            auto file = fetchResult->GetObjectAtPosition(offset);
            while (file != nullptr && (length--) > 0) {
                file->SetUserId(fetchResult->GetUserId());
                fileAssetArray.push_back(move(file));
                file = fetchResult->GetNextObject();
            }
            break;
        }
        case FetchResType::TYPE_CUSTOMRECORD: {
            CHECK_NULL_PTR_RETURN_VOID(objectPtr, "objectPtr is nullptr");
            CHECK_NULL_PTR_RETURN_VOID(objectPtr->fetchCustomRecordResult_, "fetchCustomRecordResult_ is nullptr");
            auto fetchResult = objectPtr->fetchCustomRecordResult_;
            auto customRecord = fetchResult->GetObjectAtPosition(offset);
            while (customRecord != nullptr && (length--) > 0) {
                customRecordArray.push_back(move(customRecord));
                customRecord = fetchResult->GetNextObject();
            }
            break;
        }
        case FetchResType::TYPE_ALBUMORDER: {
            CHECK_NULL_PTR_RETURN_VOID(objectPtr, "objectPtr is nullptr");
            CHECK_NULL_PTR_RETURN_VOID(objectPtr->fetchAlbumOrderResult_, "fetchAlbumOrderResult_ is nullptr");
            auto fetchResult = objectPtr->fetchAlbumOrderResult_;
            auto albumOrder = fetchResult->GetObjectAtPosition(offset);
            while (albumOrder != nullptr && (length--) > 0) {
                fileAlbumOrderArray.push_back(move(albumOrder));
                albumOrder = fetchResult->GetNextObject();
            }
            break;
        }
        default:
            NAPI_ERR_LOG("unsupported FetchResType");
            break;
    }
}

void FetchFileResultAsyncContext::GetPhotosInIndexSet()
{
    CHECK_NULL_PTR_RETURN_VOID(objectPtr, "objectPtr is nullptr");
    if (indexSet.empty()) {
        return;
    }
    switch (objectPtr->fetchResType_) {
        case FetchResType::TYPE_ALBUM: {
            CHECK_NULL_PTR_RETURN_VOID(objectPtr->fetchAlbumResult_, "fetchAlbumResult_ is nullptr");
            auto fetchResult = objectPtr->fetchAlbumResult_;
            for (size_t i = 0; i < indexSet.size(); i++) {
                auto album = fetchResult->GetObjectAtPosition(indexSet[i]);
                CHECK_AND_CONTINUE(album != nullptr);
                fileAlbumArray.push_back(move(album));
            }
            break;
        }
        case FetchResType::TYPE_PHOTOALBUM: {
            CHECK_NULL_PTR_RETURN_VOID(objectPtr->fetchPhotoAlbumResult_, "fetchPhotoAlbumResult_ is nullptr");
            auto fetchResult = objectPtr->fetchPhotoAlbumResult_;
            for (size_t i = 0; i < indexSet.size(); i++) {
                auto photoAlbum = fetchResult->GetObjectAtPosition(indexSet[i]);
                CHECK_AND_CONTINUE(photoAlbum != nullptr);
                photoAlbum->SetUserId(fetchResult->GetUserId());
                filePhotoAlbumArray.push_back(move(photoAlbum));
            }
            break;
        }
        case FetchResType::TYPE_SMARTALBUM: {
            CHECK_NULL_PTR_RETURN_VOID(objectPtr->fetchSmartAlbumResult_, "fetchSmartAlbumResult_ is nullptr");
            auto fetchResult = objectPtr->fetchSmartAlbumResult_;
            for (size_t i = 0; i < indexSet.size(); i++) {
                auto smartAlbum = fetchResult->GetObjectAtPosition(indexSet[i]);
                CHECK_AND_CONTINUE(smartAlbum != nullptr);
                fileSmartAlbumArray.push_back(move(smartAlbum));
            }
            break;
        }
        default:
            NAPI_INFO_LOG("need more check.");
            break;
    }
}

void FetchFileResultAsyncContext::GetObjectsByIndexSet()
{
    CHECK_NULL_PTR_RETURN_VOID(objectPtr, "objectPtr is nullptr");
    if (indexSet.empty()) {
        return;
    }
    FetchFileResultAsyncContext::GetPhotosInIndexSet();
    switch (objectPtr->fetchResType_) {
        case FetchResType::TYPE_FILE: {
            CHECK_NULL_PTR_RETURN_VOID(objectPtr->fetchFileResult_, "fetchFileResult_ is nullptr");
            auto fetchResult = objectPtr->fetchFileResult_;
            for (size_t i = 0; i < indexSet.size(); i++) {
                auto file = fetchResult->GetObjectAtPosition(indexSet[i]);
                CHECK_AND_CONTINUE(file != nullptr);
                file->SetUserId(fetchResult->GetUserId());
                fileAssetArray.push_back(move(file));
            }
            break;
        }
        case FetchResType::TYPE_CUSTOMRECORD: {
            CHECK_NULL_PTR_RETURN_VOID(objectPtr->fetchCustomRecordResult_, "fetchCustomRecordResult_ is nullptr");
            auto fetchResult = objectPtr->fetchCustomRecordResult_;
            for (size_t i = 0; i < indexSet.size(); i++) {
                auto customRecord = fetchResult->GetObjectAtPosition(indexSet[i]);
                CHECK_AND_CONTINUE(customRecord != nullptr);
                customRecordArray.push_back(move(customRecord));
            }
            break;
        }
        case FetchResType::TYPE_ALBUMORDER: {
            CHECK_NULL_PTR_RETURN_VOID(objectPtr->fetchAlbumOrderResult_, "fetchAlbumOrderResult_ is nullptr");
            auto fetchResult = objectPtr->fetchAlbumOrderResult_;
            for (size_t i = 0; i < indexSet.size(); i++) {
                auto albumOrder = fetchResult->GetObjectAtPosition(indexSet[i]);
                CHECK_AND_CONTINUE(albumOrder != nullptr);
                fileAlbumOrderArray.push_back(move(albumOrder));
            }
            break;
        }
        default:
            NAPI_ERR_LOG("unsupported FetchResType");
            break;
    }
}

int32_t FetchFileResultAsyncContext::GetObjectIndexById()
{
    int32_t index = -1;
    CHECK_COND_RET(objectPtr != nullptr, index, "objectPtr is nullptr");
    switch (objectPtr->fetchResType_) {
        case FetchResType::TYPE_FILE: {
            CHECK_COND_RET(objectPtr->fetchFileResult_, index, "fetchFileResult_ is nullptr");
            index = objectPtr->fetchFileResult_->GetObjectIndexById(indexObjectId);
            break;
        }
        case FetchResType::TYPE_ALBUM: {
            CHECK_COND_RET(objectPtr->fetchAlbumResult_, index, "fetchAlbumResult_ is nullptr");
            index = objectPtr->fetchAlbumResult_->GetObjectIndexById(indexObjectId);
            break;
        }
        case FetchResType::TYPE_PHOTOALBUM: {
            CHECK_COND_RET(objectPtr->fetchPhotoAlbumResult_, index, "fetchPhotoAlbumResult_ is nullptr");
            index = objectPtr->fetchPhotoAlbumResult_->GetAlbumIndex(indexObjectId, albumType, albumSubType);
            break;
        }
        case FetchResType::TYPE_SMARTALBUM: {
            CHECK_COND_RET(objectPtr->fetchSmartAlbumResult_, index, "fetchSmartAlbumResult_ is nullptr");
            index = objectPtr->fetchSmartAlbumResult_->GetObjectIndexById(indexObjectId);
            break;
        }
        case FetchResType::TYPE_CUSTOMRECORD: {
            CHECK_COND_RET(objectPtr->fetchCustomRecordResult_, index, "fetchCustomRecordResult_ is nullptr");
            index = objectPtr->fetchCustomRecordResult_->GetObjectIndexById(indexObjectId);
            break;
        }
        case FetchResType::TYPE_ALBUMORDER: {
            CHECK_COND_RET(objectPtr->fetchAlbumOrderResult_, index, "fetchAlbumOrderResult_ is nullptr");
            index = objectPtr->fetchAlbumOrderResult_->GetObjectIndexById(indexObjectId);
            break;
        }
        default:
            NAPI_ERR_LOG("unsupported FetchResType");
            break;
    }
    return index;
}

bool FetchFileResultNapi::CheckIfPropertyPtrNull()
{
    return propertyPtr == nullptr;
}
} // namespace Media
} // namespace OHOS
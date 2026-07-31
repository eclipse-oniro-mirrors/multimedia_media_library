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
#define MLOG_TAG "FetchResultNapi"

#include "sendable_fetch_file_result_napi.h"

#include "album_napi.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_tracer.h"
#include "sendable_photo_album_napi.h"
#include "sendable_medialibrary_napi_utils.h"
#include "smart_album_napi.h"
#include "media_file_utils.h"
#include "user_define_ipc_client.h"
#include "compatible_info_vo.h"
#include "userfile_client.h"
#include "medialibrary_business_code.h"
#include "preferred_compatible_mode_check_utils.h"

using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;
using namespace std;

namespace OHOS {
namespace Media {
thread_local napi_ref SendableFetchFileResultNapi::photoAccessHelperConstructor_ = nullptr;

SendableFetchFileResultNapi::SendableFetchFileResultNapi()
    : env_(nullptr) {}

SendableFetchFileResultNapi::~SendableFetchFileResultNapi()
{
    propertyPtr = nullptr;
}

static void SetTranscodeInfo(FetchFileResultSendableAsyncContext* context)
{
    if (context->objectPtr->fetchResType_ == FetchResType::TYPE_FILE &&
        (context->objectPtr->fetchFileResult_->supportedHeif == -1 ||
        context->objectPtr->fetchFileResult_->supportedHighResolution == -1)) {
        GetTranscodeCheckInfoReqBody reqBody;
        GetTranscodeCheckInfoRespBody respBody;
        AppExecFwk::BundleInfo bundleInfo;
        UserFileClient::GetBundleInfo(bundleInfo);
        reqBody.bundleName = bundleInfo.name;
        int32_t ret = IPC::UserDefineIPCClient().Call(
            static_cast<uint32_t>(MediaLibraryBusinessCode::GET_TRANSCODE_CHECK_INFO), reqBody, respBody);
        if (ret != 0) {
            NAPI_ERR_LOG("UserDefineIPCClient().Call failed, ret: %{public}d", ret);
            return;
        }
        if (respBody.preferredCompatibleMode == static_cast<int32_t>(PreferredCompatibleMode::CURRENT)) {
            context->objectPtr->fetchFileResult_->supportedHeif = 1;
            context->objectPtr->fetchFileResult_->supportedHighResolution = 1;
            return;
        }
        if (respBody.preferredCompatibleMode == static_cast<int32_t>(PreferredCompatibleMode::COMPATIBLE)) {
            context->objectPtr->fetchFileResult_->supportedHeif = 0;
            context->objectPtr->fetchFileResult_->supportedHighResolution = 0;
            return;
        }
        auto types = respBody.supportedMimeTypes;
        context->objectPtr->fetchFileResult_->supportedHeif =
            (std::find(types.begin(), types.end(), "image/heic") != types.end() ||
            std::find(types.begin(), types.end(), "image/heif") != types.end());
        context->objectPtr->fetchFileResult_->supportedHighResolution = respBody.supportedHighResolution;
    }
}

void SendableFetchFileResultNapi::FetchFileResultNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    SendableFetchFileResultNapi *fetchFileResultObj = reinterpret_cast<SendableFetchFileResultNapi*>(nativeObject);
    if (fetchFileResultObj != nullptr) {
        delete fetchFileResultObj;
        fetchFileResultObj = nullptr;
    }
}

void SendableFetchFileResultNapi::GetFetchResult(unique_ptr<SendableFetchFileResultNapi> &obj)
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
        default:
            NAPI_ERR_LOG("unsupported FetchResType");
            break;
    }
}

// Constructor callback
napi_value SendableFetchFileResultNapi::FetchFileResultNapiConstructor(napi_env env, napi_callback_info info)
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

    unique_ptr<SendableFetchFileResultNapi> obj = make_unique<SendableFetchFileResultNapi>();
    if (obj == nullptr) {
        NAPI_ERR_LOG("Get SendableFetchFileResultNapi failed");
        return result;
    }
    obj->env_ = env;
    obj->propertyPtr = make_shared<SendableFetchResultProperty>();
    GetFetchResult(obj);
    obj->propertyPtr->fetchResType_ = sFetchResType_;
    status = napi_wrap_sendable(env, thisVar, reinterpret_cast<void *>(obj.get()),
        SendableFetchFileResultNapi::FetchFileResultNapiDestructor, nullptr);
    if (status == napi_ok) {
        obj.release();
        return thisVar;
    } else {
        NAPI_ERR_LOG("Failure wrapping js to native napi, status: %{public}d", status);
    }
    return result;
}

FetchResType SendableFetchFileResultNapi::GetFetchResType()
{
    return propertyPtr->fetchResType_;
}

void SendableFetchFileResultNapi::SolveConstructorRef(unique_ptr<FetchResult<FileAsset>> &fileResult,
    napi_ref &constructorRef)
{
    switch (fileResult->GetResultNapiType()) {
        case ResultNapiType::TYPE_PHOTOACCESS_HELPER: {
            constructorRef = photoAccessHelperConstructor_;
            break;
        }
        default:
            NAPI_ERR_LOG("Invalid result napi type: %{public}d", static_cast<int>(fileResult->GetResultNapiType()));
            constructorRef = nullptr;
            break;
    }
}

void SendableFetchFileResultNapi::SolveConstructorRef(unique_ptr<FetchResult<AlbumAsset>> &fileResult,
    napi_ref &constructorRef)
{
    switch (fileResult->GetResultNapiType()) {
        case ResultNapiType::TYPE_PHOTOACCESS_HELPER: {
            constructorRef = photoAccessHelperConstructor_;
            break;
        }
        default:
            NAPI_ERR_LOG("Invalid result napi type: %{public}d", static_cast<int>(fileResult->GetResultNapiType()));
            constructorRef = nullptr;
            break;
    }
}

void SendableFetchFileResultNapi::SolveConstructorRef(unique_ptr<FetchResult<SmartAlbumAsset>> &fileResult,
    napi_ref &constructorRef)
{
    switch (fileResult->GetResultNapiType()) {
        case ResultNapiType::TYPE_PHOTOACCESS_HELPER: {
            constructorRef = photoAccessHelperConstructor_;
            break;
        }
        default:
            NAPI_ERR_LOG("Invalid result napi type: %{public}d", static_cast<int>(fileResult->GetResultNapiType()));
            constructorRef = nullptr;
            break;
    }
}

void SendableFetchFileResultNapi::SolveConstructorRef(unique_ptr<FetchResult<PhotoAlbum>> &fileResult,
    napi_ref &constructorRef)
{
    switch (fileResult->GetResultNapiType()) {
        case ResultNapiType::TYPE_PHOTOACCESS_HELPER: {
            constructorRef = photoAccessHelperConstructor_;
            break;
        }
        default:
            NAPI_ERR_LOG("Invalid result napi type: %{public}d", static_cast<int>(fileResult->GetResultNapiType()));
            constructorRef = nullptr;
            break;
    }
}

napi_value SendableFetchFileResultNapi::CreateFetchFileResult(napi_env env,
    unique_ptr<FetchResult<FileAsset>> fileResult)
{
    if (photoAccessHelperConstructor_ == nullptr) {
        napi_value exports = nullptr;
        napi_create_object(env, &exports);
        SendableFetchFileResultNapi::PhotoAccessHelperInit(env, exports);
    }

    MediaLibraryTracer tracer;
    tracer.Start("CreateFetchFileResult");
    napi_value constructor;
    napi_ref constructorRef;

    SendableFetchFileResultNapi::SolveConstructorRef(fileResult, constructorRef);
    NAPI_CALL(env, napi_get_reference_value(env, constructorRef, &constructor));
    sFetchResType_ = fileResult->GetFetchResType();
    sFetchFileResult_ = move(fileResult);
    napi_value result = nullptr;
    NAPI_CALL(env, napi_new_instance(env, constructor, 0, nullptr, &result));
    sFetchFileResult_ = nullptr;
    return result;
}

napi_value SendableFetchFileResultNapi::CreateFetchFileResult(napi_env env,
    unique_ptr<FetchResult<AlbumAsset>> fileResult)
{
    if (photoAccessHelperConstructor_ == nullptr) {
        napi_value exports = nullptr;
        napi_create_object(env, &exports);
        SendableFetchFileResultNapi::PhotoAccessHelperInit(env, exports);
    }
    
    MediaLibraryTracer tracer;
    tracer.Start("CreateFetchFileResult");
    napi_value constructor;
    napi_ref constructorRef;
    SendableFetchFileResultNapi::SolveConstructorRef(fileResult, constructorRef);
    NAPI_CALL(env, napi_get_reference_value(env, constructorRef, &constructor));
    sFetchResType_ = fileResult->GetFetchResType();
    sFetchAlbumResult_ = move(fileResult);
    napi_value result = nullptr;
    NAPI_CALL(env, napi_new_instance(env, constructor, 0, nullptr, &result));
    sFetchAlbumResult_ = nullptr;
    return result;
}

napi_value SendableFetchFileResultNapi::CreateFetchFileResult(napi_env env,
    unique_ptr<FetchResult<PhotoAlbum>> fileResult)
{
    if (photoAccessHelperConstructor_ == nullptr) {
        napi_value exports = nullptr;
        napi_create_object(env, &exports);
        SendableFetchFileResultNapi::PhotoAccessHelperInit(env, exports);
    }

    MediaLibraryTracer tracer;
    tracer.Start("CreateFetchFileResult");
    napi_value constructor;
    napi_ref constructorRef;
    SendableFetchFileResultNapi::SolveConstructorRef(fileResult, constructorRef);
    NAPI_CALL(env, napi_get_reference_value(env, constructorRef, &constructor));
    sFetchResType_ = fileResult->GetFetchResType();
    sFetchPhotoAlbumResult_ = move(fileResult);
    napi_value result = nullptr;
    NAPI_CALL(env, napi_new_instance(env, constructor, 0, nullptr, &result));
    sFetchPhotoAlbumResult_ = nullptr;
    return result;
}

napi_value SendableFetchFileResultNapi::CreateFetchFileResult(napi_env env,
    unique_ptr<FetchResult<SmartAlbumAsset>> fileResult)
{
    if (photoAccessHelperConstructor_ == nullptr) {
        napi_value exports = nullptr;
        napi_create_object(env, &exports);
        SendableFetchFileResultNapi::PhotoAccessHelperInit(env, exports);
    }

    MediaLibraryTracer tracer;
    tracer.Start("CreateFetchFileResult");
    napi_value constructor;
    napi_ref constructorRef;
    SendableFetchFileResultNapi::SolveConstructorRef(fileResult, constructorRef);
    NAPI_CALL(env, napi_get_reference_value(env, constructorRef, &constructor));
    sFetchResType_ = fileResult->GetFetchResType();
    sFetchSmartAlbumResult_ = move(fileResult);
    napi_value result = nullptr;
    NAPI_CALL(env, napi_new_instance(env, constructor, 0, nullptr, &result));
    sFetchSmartAlbumResult_ = nullptr;
    return result;
}

std::shared_ptr<FetchResult<FileAsset>> SendableFetchFileResultNapi::GetFetchFileResult() const
{
    return propertyPtr->fetchFileResult_;
}

napi_value SendableFetchFileResultNapi::PhotoAccessHelperInit(napi_env env, napi_value exports)
{
    napi_value ctorObj;
    napi_property_descriptor props[] = {
        DECLARE_NAPI_FUNCTION("getCount", JSGetCount),
        DECLARE_NAPI_FUNCTION("isAfterLast", JSIsAfterLast),
        DECLARE_NAPI_FUNCTION("getFirstObject", JSGetFirstObject),
        DECLARE_NAPI_FUNCTION("getNextObject", JSGetNextObject),
        DECLARE_NAPI_FUNCTION("getAllObjects", JSGetAllObject),
        DECLARE_NAPI_FUNCTION("getLastObject", JSGetLastObject),
        DECLARE_NAPI_FUNCTION("getObjectByPosition", JSGetPositionObject),
        DECLARE_NAPI_FUNCTION("close", JSClose)
    };
    napi_define_sendable_class(env, PAH_FETCH_FILE_RESULT_SENDABLE_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH,
                               FetchFileResultNapiConstructor, nullptr,
                               sizeof(props) / sizeof(props[0]), props, nullptr, &ctorObj);
    NAPI_CALL(env, napi_create_reference(env, ctorObj, NAPI_INIT_REF_COUNT, &photoAccessHelperConstructor_));
    NAPI_CALL(env, napi_set_named_property(env, exports, PAH_FETCH_FILE_RESULT_SENDABLE_CLASS_NAME.c_str(), ctorObj));
    return exports;
}

static bool CheckIfFFRNapiNotEmpty(SendableFetchFileResultNapi* obj)
{
    if (obj == nullptr) {
        NAPI_INFO_LOG("SendableFetchFileResultNapi is nullptr");
        return false;
    }
    if (obj->CheckIfPropertyPtrNull()) {
        NAPI_INFO_LOG("PropertyPtr in SendableFetchFileResultNapi is nullptr");
        return false;
    }
    return true;
}

static bool CheckIfFetchResultNotEmpty(SendableFetchFileResultNapi* obj, FetchResType fetchResType)
{
    if (fetchResType == FetchResType::TYPE_FILE && obj->GetFetchFileResultObject() == nullptr) {
        NAPI_ERR_LOG("GetFetchFileResultObject is nullptr");
        return false;
    } else if (fetchResType == FetchResType::TYPE_ALBUM && obj->GetFetchAlbumResultObject() == nullptr) {
        NAPI_ERR_LOG("GetFetchAlbumResultObject is nullptr");
        return false;
    } else if (fetchResType == FetchResType::TYPE_PHOTOALBUM && obj->GetFetchPhotoAlbumResultObject() == nullptr) {
        NAPI_ERR_LOG("GetFetchPhotoAlbumResultObject is nullptr");
        return false;
    } else if (fetchResType == FetchResType::TYPE_SMARTALBUM && obj->GetFetchSmartAlbumResultObject() == nullptr) {
        NAPI_ERR_LOG("GetFetchSmartAlbumResultObject is nullptr");
        return false;
    }
    return true;
}

napi_value SendableFetchFileResultNapi::JSGetCount(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    SendableFetchFileResultNapi* obj = nullptr;
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

    status = napi_unwrap_sendable(env, thisVar, reinterpret_cast<void **>(&obj));
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

static void GetNapiResFromAsset(napi_env env, FetchFileResultSendableAsyncContext *context,
    unique_ptr<SendableJSAsyncContextOutput> &jsContext)
{
    napi_value jsAsset = nullptr;
    switch (context->objectPtr->fetchResType_) {
        case FetchResType::TYPE_FILE:
            if (context->fileAsset != nullptr && context->objectPtr->fetchFileResult_ != nullptr) {
                context->fileAsset->SetUserId(context->objectPtr->fetchFileResult_->GetUserId());
                jsAsset = SendableFileAssetNapi::CreateFileAsset(env, context->fileAsset);
            }
            break;
        case FetchResType::TYPE_ALBUM:
            jsAsset = AlbumNapi::CreateAlbumNapi(env, context->albumAsset);
            break;
        case FetchResType::TYPE_PHOTOALBUM:
            context->photoAlbum->SetUserId(context->objectPtr->fetchPhotoAlbumResult_->GetUserId());
            jsAsset = SendablePhotoAlbumNapi::CreatePhotoAlbumNapi(env, context->photoAlbum);
            break;
        case FetchResType::TYPE_SMARTALBUM:
            jsAsset = SmartAlbumNapi::CreateSmartAlbumNapi(env, context->smartAlbumAsset);
            break;
        default:
            NAPI_ERR_LOG("unsupported FetchResType");
            break;
    }

    if (jsAsset == nullptr) {
        NAPI_ERR_LOG("Failed to get file asset napi object");
        napi_get_undefined(env, &jsContext->data);
        SendableMediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, JS_INNER_FAIL,
            "System inner fail");
    } else {
        jsContext->data = jsAsset;
        napi_get_undefined(env, &jsContext->error);
        jsContext->status = true;
    }
}

static void GetPositionObjectCompleteCallback(napi_env env, napi_status status,
    FetchFileResultSendableAsyncContext* context)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetPositionObjectCompleteCallback");

    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<SendableJSAsyncContextOutput> jsContext = make_unique<SendableJSAsyncContextOutput>();
    jsContext->status = false;

    GetNapiResFromAsset(env, context, jsContext);

    if (context->work != nullptr) {
        SendableMediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }

    delete context;
}

napi_value SendableFetchFileResultNapi::JSGetFirstObject(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_ONE, "requires 1 parameter");
    napi_get_undefined(env, &result);

    unique_ptr<FetchFileResultSendableAsyncContext> asyncContext = make_unique<FetchFileResultSendableAsyncContext>();
    status = napi_unwrap_sendable(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
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
                auto context = static_cast<FetchFileResultSendableAsyncContext*>(data);
                SetTranscodeInfo(context);
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

napi_value SendableFetchFileResultNapi::JSGetNextObject(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_ONE, "requires 1 parameter");

    napi_get_undefined(env, &result);
    unique_ptr<FetchFileResultSendableAsyncContext> asyncContext = make_unique<FetchFileResultSendableAsyncContext>();
    status = napi_unwrap_sendable(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
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
                auto context = static_cast<FetchFileResultSendableAsyncContext*>(data);
                SetTranscodeInfo(context);
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

static napi_value GetAsset(napi_env env, vector<std::unique_ptr<FileAsset>> &array, int index)
{
    return SendableFileAssetNapi::CreateFileAsset(env, array[index]);
}

static napi_value GetAsset(napi_env env, vector<std::unique_ptr<AlbumAsset>> &array, int index)
{
    return AlbumNapi::CreateAlbumNapi(env, array[index]);
}

static napi_value GetAsset(napi_env env, vector<std::unique_ptr<PhotoAlbum>> &array, int index)
{
    return SendablePhotoAlbumNapi::CreatePhotoAlbumNapi(env, array[index]);
}

static napi_value GetAsset(napi_env env, vector<std::unique_ptr<SmartAlbumAsset>> &array, int index)
{
    return SmartAlbumNapi::CreateSmartAlbumNapi(env, array[index]);
}

template<class T>
static void GetAssetFromArray(napi_env env, FetchFileResultSendableAsyncContext* context, T& array,
    unique_ptr<SendableJSAsyncContextOutput> &jsContext)
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
            SendableMediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_MEM_ALLOCATION,
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

static void GetAllObjectCompleteCallback(napi_env env, napi_status status, FetchFileResultSendableAsyncContext* context)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetAllObjectCompleteCallback");
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<SendableJSAsyncContextOutput> jsContext = make_unique<SendableJSAsyncContextOutput>();
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
        default:
            NAPI_ERR_LOG("unsupported FetchResType");
            napi_get_undefined(env, &jsContext->data);
            SendableMediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
                "Failed to obtain fileAsset array from DB");
    }

    if (context->work != nullptr) {
        int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
        SendableMediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
        int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
        int64_t normalTime = 500;
        if ((long)(end - start) >= normalTime) {
            NAPI_INFO_LOG("InvokeJSAsync dir cost: %{public}ld", (long)(end - start));
        }
    }

    delete context;
}

std::shared_ptr<FetchResult<FileAsset>> SendableFetchFileResultNapi::GetFetchFileResultObject()
{
    return propertyPtr->fetchFileResult_;
}

std::shared_ptr<FetchResult<AlbumAsset>> SendableFetchFileResultNapi::GetFetchAlbumResultObject()
{
    return propertyPtr->fetchAlbumResult_;
}

std::shared_ptr<FetchResult<PhotoAlbum>> SendableFetchFileResultNapi::GetFetchPhotoAlbumResultObject()
{
    return propertyPtr->fetchPhotoAlbumResult_;
}

std::shared_ptr<FetchResult<SmartAlbumAsset>> SendableFetchFileResultNapi::GetFetchSmartAlbumResultObject()
{
    return propertyPtr->fetchSmartAlbumResult_;
}

void GetAllObjectFromFetchResult(const FetchFileResultSendableAsyncContext &asyncContext)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetAllObjectFromFetchResult");

    FetchFileResultSendableAsyncContext *context = const_cast<FetchFileResultSendableAsyncContext *>(&asyncContext);
    context->GetAllObjectFromFetchResult();
}

napi_value SendableFetchFileResultNapi::JSGetAllObject(napi_env env, napi_callback_info info)
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
    unique_ptr<FetchFileResultSendableAsyncContext> asyncContext = make_unique<FetchFileResultSendableAsyncContext>();
    status = napi_unwrap_sendable(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
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
                auto context = static_cast<FetchFileResultSendableAsyncContext*>(data);
                SetTranscodeInfo(context);
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

napi_value SendableFetchFileResultNapi::JSClose(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    SendableFetchFileResultNapi* obj = nullptr;
    napi_value thisVar = nullptr;

    MediaLibraryTracer tracer;
    tracer.Start("JSClose");

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments!, status: %{public}d", status);
        return jsResult;
    }
    status = napi_unwrap_sendable(env, thisVar, reinterpret_cast<void **>(&obj));
    if ((status == napi_ok) && (obj != nullptr)) {
        obj->propertyPtr = nullptr;
    }
    status = napi_remove_wrap_sendable(env, thisVar, reinterpret_cast<void **>(&obj));
    if ((status == napi_ok) && (obj != nullptr)) {
        napi_create_int32(env, E_SUCCESS, &jsResult);
    } else {
        NAPI_INFO_LOG("JSClose obj == nullptr");
    }

    NAPI_DEBUG_LOG("JSClose OUT!");
    return jsResult;
}

void FetchFileResultSendableAsyncContext::GetFirstAsset()
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
        default:
            NAPI_ERR_LOG("unsupported FetchResType");
            break;
    }
}

void FetchFileResultSendableAsyncContext::GetObjectAtPosition()
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
        default:
            NAPI_ERR_LOG("unsupported FetchResType");
            break;
    }
}

void FetchFileResultSendableAsyncContext::GetLastObject()
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
        default:
            NAPI_ERR_LOG("unsupported FetchResType");
            break;
    }
}

void FetchFileResultSendableAsyncContext::GetNextObject()
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
        default:
            NAPI_ERR_LOG("unsupported FetchResType");
            break;
    }
}

void FetchFileResultSendableAsyncContext::GetAllObjectFromFetchResult()
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
        default:
            NAPI_ERR_LOG("unsupported FetchResType");
            break;
    }
}

bool SendableFetchFileResultNapi::CheckIfPropertyPtrNull()
{
    return propertyPtr == nullptr;
}

napi_value SendableFetchFileResultNapi::JSIsAfterLast(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    SendableFetchFileResultNapi* obj = nullptr;
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

    status = napi_unwrap_sendable(env, thisVar, reinterpret_cast<void **>(&obj));
    FetchResType fetchResType = obj->GetFetchResType();
    if ((status == napi_ok) && CheckIfFFRNapiNotEmpty(obj) && CheckIfFetchResultNotEmpty(obj, fetchResType)) {
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

napi_value SendableFetchFileResultNapi::JSGetLastObject(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_ONE, "requires 1 parameter");

    napi_get_undefined(env, &result);
    unique_ptr<FetchFileResultSendableAsyncContext> asyncContext = make_unique<FetchFileResultSendableAsyncContext>();
    status = napi_unwrap_sendable(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
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
                auto context = static_cast<FetchFileResultSendableAsyncContext*>(data);
                SetTranscodeInfo(context);
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

napi_value SendableFetchFileResultNapi::JSGetPositionObject(napi_env env, napi_callback_info info)
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
    unique_ptr<FetchFileResultSendableAsyncContext> asyncContext = make_unique<FetchFileResultSendableAsyncContext>();
    status = napi_unwrap_sendable(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
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
                auto context = static_cast<FetchFileResultSendableAsyncContext*>(data);
                SetTranscodeInfo(context);
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
        NAPI_ASSERT(env, false, "JSGetPositionObject obj == nullptr");
    }

    return result;
}
} // namespace Media
} // namespace OHOS
/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "CloneToAlbumNapi"

#include "media_library_napi.h"

#include "clone_to_album_vo.h"
#include "medialibrary_napi_utils.h"
#include "medialibrary_tracer.h"
#include "media_log.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "medialibrary_errno.h"
#include "medialibrary_business_code.h"
#include "user_define_ipc_client.h"
#include "medialibrary_client_errno.h"
#include "photo_album_napi.h"
#include "task_signal_napi.h"
#include "photo_file_utils.h"
#include "media_file_utils.h"
#include "userfile_manager_types.h"

using namespace std;

namespace OHOS {
namespace Media {

static bool CheckPropertyListner(napi_env env, napi_value arg, const string &property, napi_value &listener)
{
    bool present = false;
    napi_status result = napi_has_named_property(env, arg, property.c_str(), &present);
    if (result == napi_ok && present &&
        napi_get_named_property(env, arg, property.c_str(), &listener) == napi_ok) {
        return true;
    }
    return false;
}

static void ParseProgressListener(napi_env env, napi_value options, MediaLibraryAsyncContext *ctx)
{
    CHECK_NULL_PTR_RETURN_VOID(ctx, "Context is null");
    napi_value countListener = nullptr;
    napi_value sizeListener = nullptr;
    const int32_t refCount = 1;
    if (CheckPropertyListner(env, options, "sizeProgressListener", sizeListener)) {
        if (MediaLibraryNapiUtils::CheckJSArgsTypeAsFunc(env, sizeListener)) {
            CHECK_ARGS_RET_VOID(env,
                napi_create_reference(env, sizeListener, refCount, &ctx->cloneCtx.sizeProgressListener),
                JS_INNER_FAIL);
        }
    }
    if (CheckPropertyListner(env, options, "countProgressListener", countListener)) {
        if (MediaLibraryNapiUtils::CheckJSArgsTypeAsFunc(env, countListener)) {
            CHECK_ARGS_RET_VOID(env,
                napi_create_reference(env, countListener, refCount, &ctx->cloneCtx.countProgressListener),
                JS_INNER_FAIL);
        }
    }
}

static void ParseTaskSignalListener(napi_env env, napi_value options, MediaLibraryAsyncContext *ctx)
{
    napi_value taskSignal = nullptr;
    if (!CheckPropertyListner(env, options, "taskSignal", taskSignal)) {
        NAPI_INFO_LOG("no taskSignal to parse");
        return;
    }

    ctx->cloneCtx.requestId = MediaLibraryNapi::AssignRequestId();
    ctx->cloneCtx.callback = new CloneToAlbumCallbackNapi(env,
        ctx->cloneCtx.sizeProgressListener, ctx->cloneCtx.countProgressListener, ctx->cloneCtx.resultListener);
    CHECK_NULL_PTR_RETURN_VOID(ctx->cloneCtx.callback, "callback is null");

    auto cancelCallback = [ctx]() {
        NAPI_INFO_LOG("TaskSignal cancel callback triggered, %{public}d", ctx->cloneCtx.requestId);
        if (ctx != nullptr && ctx->cloneCtx.callback != nullptr) {
            ctx->cloneCtx.callback->SetCancelled(ctx->cloneCtx.requestId);
        }
    };
    if (taskSignal != nullptr) {
        napi_valuetype signalType = napi_undefined;
        napi_typeof(env, taskSignal, &signalType);
        if (signalType == napi_object) {
            TaskSignalNapi *taskSignalNapi = nullptr;
            auto status = napi_unwrap(env, taskSignal, reinterpret_cast<void **>(&taskSignalNapi));
            if (status != napi_ok || taskSignalNapi == nullptr) {
                NAPI_ERR_LOG("Failed to unwrap TaskSignalNapi");
                return;
            }
            taskSignalNapi->RegisterCancelCallback(env, cancelCallback);
        }
    }
}

static void ParseOptions(napi_env env, napi_value arg, MediaLibraryAsyncContext *ctx)
{
    CHECK_NULL_PTR_RETURN_VOID(ctx, "Context is null");
    napi_valuetype type = napi_undefined;
    if (napi_typeof(env, arg, &type) != napi_ok || type != napi_object) {
        NAPI_ERR_LOG("ParseOptions error");
        return;
    }

    ParseProgressListener(env, arg, ctx);

    napi_value resultListener = nullptr;
    if (CheckPropertyListner(env, arg, "resultListener", resultListener)) {
        if (MediaLibraryNapiUtils::CheckJSArgsTypeAsFunc(env, resultListener)) {
            CHECK_ARGS_RET_VOID(env, napi_create_reference(env, resultListener, 1, &ctx->cloneCtx.resultListener),
            JS_INNER_FAIL);
        }
    }

    napi_value modeValue = nullptr;
    if (CheckPropertyListner(env, arg, "mode", modeValue)) {
        CHECK_ARGS_RET_VOID(env, MediaLibraryNapiUtils::GetInt32(env, modeValue, ctx->cloneCtx.mode),
            JS_INNER_FAIL);
    }

    ParseTaskSignalListener(env, arg, ctx);
}

static void CleanupReferences(napi_env env, MediaLibraryAsyncContext *ctx)
{
    if (ctx->cloneCtx.callback != nullptr) {
        ctx->cloneCtx.callback = nullptr;
    }
    if (ctx->cloneCtx.sizeProgressListener != nullptr) {
        napi_delete_reference(env, ctx->cloneCtx.sizeProgressListener);
        ctx->cloneCtx.sizeProgressListener = nullptr;
    }
    if (ctx->cloneCtx.countProgressListener != nullptr) {
        napi_delete_reference(env, ctx->cloneCtx.countProgressListener);
        ctx->cloneCtx.countProgressListener = nullptr;
    }
    if (ctx->cloneCtx.resultListener != nullptr) {
        napi_delete_reference(env, ctx->cloneCtx.resultListener);
        ctx->cloneCtx.resultListener = nullptr;
    }
    delete ctx;
}

static void ExecuteCloneToAlbum(napi_env env, void *data)
{
    NAPI_INFO_LOG("ExecuteCloneToAlbum start");
    MediaLibraryTracer tracer;
    tracer.Start("ExecuteCloneToAlbum");
    auto *ctx = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(ctx, "Context is null");

    if (ctx->cloneCtx.fileUris.empty()) {
        NAPI_ERR_LOG("assetUris is empty");
        ctx->SaveSceneErr(E_SCENE_PARAM_INVALID);
        return;
    }

    if (!ctx->cloneCtx.callback) {
        ctx->cloneCtx.callback = new CloneToAlbumCallbackNapi(env,
            ctx->cloneCtx.sizeProgressListener, ctx->cloneCtx.countProgressListener, ctx->cloneCtx.resultListener);
    }
    CHECK_NULL_PTR_RETURN_VOID(ctx->cloneCtx.callback, "callback is null");

    CloneToAlbumReqBody reqBody;
    reqBody.assetsArray = ctx->cloneCtx.fileUris;
    reqBody.albumId = ctx->cloneCtx.albumId;
    reqBody.mode = ctx->cloneCtx.mode;
    reqBody.progressCallback = ctx->cloneCtx.callback->AsObject();
    reqBody.requestId = ctx->cloneCtx.requestId;

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CLONE_TO_ALBUM);
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(ctx->userId).Call(businessCode, reqBody);
    if (ret != E_OK) {
        NAPI_ERR_LOG("IPC call failed: %{public}d", ret);
        ctx->SaveSceneErr(ret);
        return;
    }

    ret = ctx->cloneCtx.callback->WaitForCloneResult();
    if (ret != E_OK) {
        NAPI_ERR_LOG("wait for failed: %{public}d", ret);
        ctx->SaveSceneErr(ret);
        return;
    }
    ctx->SaveSceneErr(ctx->cloneCtx.callback->GetErrorCode());
    NAPI_INFO_LOG("ExecuteCloneToAlbum end error:%{public}d", ctx->cloneCtx.callback->GetErrorCode());
}

static std::vector<std::shared_ptr<FileAsset>> GetFileAssetFormResult(
    std::shared_ptr<DataShare::DataShareResultSet> resultSet)
{
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("resultSet error is null");
        return {};
    }
    auto fetchResult = make_unique<FetchResult<FileAsset>>(resultSet);
    if (fetchResult == nullptr) {
        NAPI_ERR_LOG("fetchResult error is null");
        return {};
    }
    std::vector<std::shared_ptr<FileAsset>> newFileAssets;
    auto file = fetchResult->GetFirstObject();
    while (file != nullptr) {
        auto newFileAsset = std::shared_ptr<FileAsset>(std::move(file));
        newFileAssets.push_back(newFileAsset);
        file = fetchResult->GetNextObject();
    }
    return newFileAssets;
}

static void CompleteCloneToAlbum(napi_env env, napi_status status, void *data)
{
    NAPI_INFO_LOG("CompleteCloneToAlbum start");
    MediaLibraryTracer tracer;
    tracer.Start("CompleteCloneToAlbum");
    auto* context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    CHECK_NULL_PTR_RETURN_VOID(context->cloneCtx.callback, "Async context callback is null");
    auto jsContext = make_unique<JSAsyncContextOutput>();
    napi_value jsFileArray = nullptr;
    size_t i = 0;
    auto cloneErrCode = context->cloneCtx.callback->GetErrorCode();
    std::vector<std::shared_ptr<FileAsset>> newFileAssets =
        GetFileAssetFormResult(context->cloneCtx.callback->GetResultSet());
    napi_create_array_with_length(env, newFileAssets.size(), &jsFileArray);
    if (cloneErrCode == ERR_DEFAULT) {
        for (i = 0; i < newFileAssets.size(); i++) {
            std::shared_ptr<FileAsset> newFileAsset = newFileAssets.at(i);
            CHECK_NULL_PTR_RETURN_VOID(newFileAsset, "newFileAset is null.");
            std::string newFileAssetUri = MediaFileUtils::GetFileAssetUri(newFileAsset->GetPath(),
                newFileAsset->GetDisplayName(), newFileAsset->GetId());
            newFileAsset->SetUri(MediaFileUtils::Encode(newFileAssetUri));
            newFileAsset->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
            napi_value jsFileAsset = FileAssetNapi::CreatePhotoAsset(env, newFileAsset);
            if ((jsFileAsset == nullptr) || (napi_set_element(env, jsFileArray, i, jsFileAsset) != napi_ok)) {
                NAPI_ERR_LOG("Failed to get file asset napi object");
                napi_get_undefined(env, &jsContext->data);
                MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_MEM_ALLOCATION,
                    "Failed to create js object");
                break;
            }
        }
        if (i == newFileAssets.size()) {
            jsContext->data = jsFileArray;
            napi_get_undefined(env, &jsContext->error);
            jsContext->status = true;
        }
    } else {
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
    }
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    CleanupReferences(env, context);
}

napi_value GetNapiValueArray(napi_env env, napi_value arg, vector<napi_value> &values)
{
    bool isArray = false;
    CHECK_ARGS(env, napi_is_array(env, arg, &isArray), JS_E_PARAM_INVALID);
    if (!isArray) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Failed to check array type");
        return nullptr;
    }

    uint32_t len = 0;
    CHECK_ARGS(env, napi_get_array_length(env, arg, &len), JS_E_PARAM_INVALID);
    if (len == 0) {
        napi_value result = nullptr;
        CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_E_PARAM_INVALID);
        return result;
    }

    for (uint32_t i = 0; i < len; i++) {
        napi_value value = nullptr;
        CHECK_ARGS(env, napi_get_element(env, arg, i, &value), JS_E_PARAM_INVALID);
        if (value == nullptr) {
            NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Failed to get element");
            return nullptr;
        }
        values.push_back(value);
    }

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_E_PARAM_INVALID);
    return result;
}

static napi_value ParseFileAssetArray(napi_env env, napi_value arg, std::vector<std::string>& assetArray)
{
    vector<napi_value> napiValues;
    napi_valuetype valueType = napi_undefined;
    FileAssetNapi *obj = nullptr;
    CHECK_NULLPTR_RET(GetNapiValueArray(env, arg, napiValues));
    for (const auto &napiValue : napiValues) {
        CHECK_ARGS(env, napi_typeof(env, napiValue, &valueType), JS_E_PARAM_INVALID);
        CHECK_COND_WITH_ERR_MESSAGE(env, valueType == napi_object, JS_E_PARAM_INVALID, "Invalid argument type");
        CHECK_ARGS(env, napi_unwrap(env, napiValue, reinterpret_cast<void **>(&obj)), JS_E_PARAM_INVALID);
        CHECK_COND_WITH_ERR_MESSAGE(env, obj != nullptr, JS_E_PARAM_INVALID, "Failed to get asset napi object");
        auto fileAsset = obj->GetFileAssetInstance();
        CHECK_COND_WITH_ERR_MESSAGE(env, fileAsset != nullptr, JS_E_PARAM_INVALID, "FileAsset instance is null");
        //待复制资产是否是隐藏或回收站资产,或是纯云资产
        CHECK_COND_WITH_ERR_MESSAGE(env, !fileAsset->IsHidden(), JS_E_PARAM_INVALID, "asset is hidden");
        CHECK_COND_WITH_ERR_MESSAGE(env, fileAsset->GetIsTrash() == 0, JS_E_PARAM_INVALID, "asset is in trash");
        CHECK_COND_WITH_ERR_MESSAGE(env,
            fileAsset->GetPosition() != static_cast<int32_t>(PhotoPositionType::CLOUD), JS_E_PARAM_INVALID,
            "asset is pure cloud");
        assetArray.push_back(fileAsset->GetUri());
    }
    RETURN_NAPI_TRUE(env);
}

static napi_value ParsePhotoAlbum(napi_env env, napi_value arg, shared_ptr<PhotoAlbum>& photoAlbum)
{
    napi_valuetype valueType;
    PhotoAlbumNapi* photoAlbumNapi;
    CHECK_ARGS(env, napi_typeof(env, arg, &valueType), JS_E_PARAM_INVALID);
    CHECK_COND_WITH_ERR_MESSAGE(env, valueType == napi_object, JS_E_PARAM_INVALID, "Invalid argument type");
    CHECK_ARGS(env, napi_unwrap(env, arg, reinterpret_cast<void**>(&photoAlbumNapi)), JS_E_PARAM_INVALID);
    CHECK_COND_WITH_ERR_MESSAGE(env, photoAlbumNapi != nullptr, JS_E_PARAM_INVALID,
        "Failed to get PhotoAlbumNapi object");
 
    auto photoAlbumPtr = photoAlbumNapi->GetPhotoAlbumInstance();
    CHECK_COND_WITH_ERR_MESSAGE(env, photoAlbumPtr != nullptr, JS_E_PARAM_INVALID, "photoAlbum is null");
    //目标相册必须是用户相册或来源相册
    CHECK_COND_WITH_ERR_MESSAGE(env, PhotoAlbum::IsUserPhotoAlbum(photoAlbumPtr->GetPhotoAlbumType(),
        photoAlbumPtr->GetPhotoAlbumSubType()) || PhotoAlbum::IsSourceAlbum(photoAlbumPtr->GetPhotoAlbumType(),
        photoAlbumPtr->GetPhotoAlbumSubType()), JS_E_PARAM_INVALID, "Unsupported type of photoAlbum");
    photoAlbum = photoAlbumPtr;
    RETURN_NAPI_TRUE(env);
}

napi_value MediaLibraryNapi::JSCloneToAlbum(napi_env env, napi_callback_info info)
{
    NAPI_INFO_LOG("JSCloneToAlbum start");
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JSCloneToAlbum");

    auto ctx = make_unique<MediaLibraryAsyncContext>();
    napi_status status = MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, ctx, ARGS_TWO, ARGS_THREE);
    CHECK_COND_RET(status == napi_ok, nullptr, "Failed to get object info");

    std::vector<std::string> assetArray;
    CHECK_COND_WITH_ERR_MESSAGE(env, ParseFileAssetArray(env, ctx->argv[PARAM0], ctx->cloneCtx.fileUris),
        JS_E_PARAM_INVALID, "Failed to parse assets");

    shared_ptr<PhotoAlbum> targetAlbum = nullptr;
    CHECK_COND_WITH_ERR_MESSAGE(env, ParsePhotoAlbum(env, ctx->argv[PARAM1], targetAlbum),
        JS_E_PARAM_INVALID, "4.The target album does not exist");
    NAPI_ASSERT(env, targetAlbum != nullptr, "targetAlbum == nullptr");
    ctx->cloneCtx.albumId = targetAlbum->GetAlbumId();

    if (ctx->argc >= ARGS_THREE) {
        ParseOptions(env, ctx->argv[PARAM2], ctx.get());
    }

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, ctx, "JSCloneToAlbum",
        ExecuteCloneToAlbum, CompleteCloneToAlbum);
}

static void ExecuteCloneToDir(napi_env env, void *data)
{
    NAPI_INFO_LOG("ExecuteCloneToDir start");
    MediaLibraryTracer tracer;
    tracer.Start("ExecuteCloneToDir");
    auto *ctx = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(ctx, "Context is null");

    if (ctx->cloneCtx.fileUris.empty()) {
        NAPI_ERR_LOG("assetUris is empty");
        ctx->SaveSceneErr(E_SCENE_PARAM_INVALID);
        return;
    }

    if (!ctx->cloneCtx.callback) {
        ctx->cloneCtx.callback = new CloneToAlbumCallbackNapi(env,
            ctx->cloneCtx.sizeProgressListener, ctx->cloneCtx.countProgressListener, ctx->cloneCtx.resultListener);
    }
    CHECK_NULL_PTR_RETURN_VOID(ctx->cloneCtx.callback, "callback is null");

    CloneToAlbumReqBody reqBody;
    reqBody.assetsArray = ctx->cloneCtx.fileUris;
    reqBody.mode = ctx->cloneCtx.mode;
    reqBody.progressCallback = ctx->cloneCtx.callback->AsObject();
    reqBody.requestId = ctx->cloneCtx.requestId;
    reqBody.targetDir = ctx->cloneCtx.targetDir;

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CLONE_TO_DIR);
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(ctx->userId).Call(businessCode, reqBody);
    if (ret != E_OK) {
        NAPI_ERR_LOG("IPC call failed: %{public}d", ret);
        ctx->SaveSceneErr(ret);
        return;
    }

    ret = ctx->cloneCtx.callback->WaitForCloneResult();
    if (ret != E_OK) {
        NAPI_ERR_LOG("wait for failed: %{public}d", ret);
        ctx->SaveSceneErr(ret);
        return;
    }
    ctx->SaveSceneErr(ctx->cloneCtx.callback->GetErrorCode());
    NAPI_INFO_LOG("ExecuteCloneToDir end");
}

static void CompleteCloneToDir(napi_env env, napi_status status, void *data)
{
    NAPI_INFO_LOG("CompleteCloneToDir begin");
    MediaLibraryTracer tracer;
    tracer.Start("CompleteCloneToDir");
    auto* context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    CHECK_NULL_PTR_RETURN_VOID(context->cloneCtx.callback, "Async callback is null");
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_value jsFileArray = nullptr;
    napi_value jsFileAsset = nullptr;
    size_t i = 0;
    if (context->error == ERR_DEFAULT) {
        auto fileUris = context->cloneCtx.callback->GetSuccessUris();
        napi_create_array_with_length(env, fileUris.size(), &jsFileArray);
        for (i = 0; i < fileUris.size(); i++) {
            napi_get_undefined(env, &jsFileAsset);
            if (napi_create_string_utf8(env,
                MediaFileUtils::Encode(fileUris[i]).c_str(), NAPI_AUTO_LENGTH, &jsFileAsset) != napi_ok) {
                NAPI_ERR_LOG("Failed to create file uri string");
                napi_get_undefined(env, &jsContext->data);
                MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_MEM_ALLOCATION,
                    "Failed to create js object");
                break;
            }
            if (napi_set_element(env, jsFileArray, i, jsFileAsset) != napi_ok) {
                NAPI_ERR_LOG("Failed to get file asset napi object");
                napi_get_undefined(env, &jsContext->data);
                MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_MEM_ALLOCATION,
                    "Failed to create js object");
                break;
            }
        }
        if (i == fileUris.size()) {
            jsContext->data = jsFileArray;
            napi_get_undefined(env, &jsContext->error);
            jsContext->status = true;
        }
    } else {
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    CleanupReferences(env, context);
    NAPI_INFO_LOG("CompleteCloneToDir end");
}

napi_value MediaLibraryNapi::JSCloneToDir(napi_env env, napi_callback_info info)
{
    NAPI_INFO_LOG("JSCloneToDir start");
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JSCloneToDir");

    auto ctx = make_unique<MediaLibraryAsyncContext>();
    napi_status status = MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, ctx, ARGS_TWO, ARGS_THREE);
    CHECK_COND_RET(status == napi_ok, nullptr, "Failed to get object info");

    std::vector<std::string> assetArray;
    CHECK_COND_WITH_ERR_MESSAGE(env,
        MediaLibraryNapiUtils::GetStringArray(env, ctx->argv[PARAM0], ctx->cloneCtx.fileUris) == napi_ok,
        JS_E_PARAM_INVALID, "Failed to get assets");
    MediaLibraryNapiUtils::GetParamStringPathMax(env, ctx->argv[PARAM1], ctx->cloneCtx.targetDir);
    CHECK_COND_WITH_ERR_MESSAGE(env, !ctx->cloneCtx.targetDir.empty(), JS_E_PARAM_INVALID,
        "4.The target album does not exist");
    if (ctx->argc >= ARGS_THREE) {
        ParseOptions(env, ctx->argv[PARAM2], ctx.get());
    }

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, ctx, "JSCloneToDir",
        ExecuteCloneToDir, CompleteCloneToDir);
}

static void ExecuteCloneAssetsByPath(napi_env env, void *data)
{
    NAPI_INFO_LOG("ExecuteCloneAssetsByPath start");
    MediaLibraryTracer tracer;
    tracer.Start("ExecuteCloneAssetsByPath");
    auto *ctx = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(ctx, "Context is null");

    if (ctx->cloneCtx.fileUris.empty()) {
        NAPI_ERR_LOG("assetUris is empty");
        ctx->SaveSceneErr(E_SCENE_PARAM_INVALID);
        return;
    }

    if (!ctx->cloneCtx.callback) {
        ctx->cloneCtx.callback = new CloneToAlbumCallbackNapi(env,
            ctx->cloneCtx.sizeProgressListener, ctx->cloneCtx.countProgressListener, ctx->cloneCtx.resultListener);
    }
    CHECK_NULL_PTR_RETURN_VOID(ctx->cloneCtx.callback, "callback is null");

    CloneToAlbumReqBody reqBody;
    reqBody.assetsArray = ctx->cloneCtx.fileUris;
    reqBody.mode = ctx->cloneCtx.mode;
    reqBody.progressCallback = ctx->cloneCtx.callback->AsObject();
    reqBody.requestId = ctx->cloneCtx.requestId;
    reqBody.albumId = ctx->cloneCtx.albumId;

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CLONE_ASSETS_BY_PATH);
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(ctx->userId).Call(businessCode, reqBody);
    if (ret != E_OK) {
        NAPI_ERR_LOG("IPC call failed: %{public}d", ret);
        ctx->SaveSceneErr(ret);
        return;
    }

    ret = ctx->cloneCtx.callback->WaitForCloneResult();
    if (ret != E_OK) {
        NAPI_ERR_LOG("wait for failed: %{public}d", ret);
        ctx->SaveSceneErr(ret);
        return;
    }
    ctx->SaveSceneErr(ctx->cloneCtx.callback->GetErrorCode());
    NAPI_INFO_LOG("ExecuteCloneAssetsByPath end");
}

static void CompleteCloneAssetsByPath(napi_env env, napi_status status, void *data)
{
    NAPI_INFO_LOG("CompleteCloneAssetsByPath begin");
    MediaLibraryTracer tracer;
    tracer.Start("CompleteCloneAssetsByPath");
    auto* context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    CHECK_NULL_PTR_RETURN_VOID(context->cloneCtx.callback, "Async callback is null");
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_value jsFileArray = nullptr;
    napi_value jsFileAsset = nullptr;
    size_t i = 0;
    if (context->error == ERR_DEFAULT) {
        auto fileUris = context->cloneCtx.callback->GetSuccessUris();
        napi_create_array_with_length(env, fileUris.size(), &jsFileArray);
        for (i = 0; i < fileUris.size(); i++) {
            napi_get_undefined(env, &jsFileAsset);
            napi_create_string_utf8(env, MediaFileUtils::Encode(fileUris[i]).c_str(), NAPI_AUTO_LENGTH, &jsFileAsset);
            if (napi_set_element(env, jsFileArray, i, jsFileAsset) != napi_ok) {
                NAPI_ERR_LOG("Failed to get file asset napi object");
                napi_get_undefined(env, &jsContext->data);
                MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_MEM_ALLOCATION,
                    "Failed to create js object");
                break;
            }
        }
        if (i == fileUris.size()) {
            jsContext->data = jsFileArray;
            napi_get_undefined(env, &jsContext->error);
            jsContext->status = true;
        }
    } else {
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    CleanupReferences(env, context);
    NAPI_INFO_LOG("CompleteCloneAssetsByPath end");
}

napi_value MediaLibraryNapi::JSCloneAssetsByPath(napi_env env, napi_callback_info info)
{
    NAPI_INFO_LOG("JSCloneAssetsByPath start");
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JSCloneAssetsByPath");

    auto ctx = make_unique<MediaLibraryAsyncContext>();
    napi_status status = MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, ctx, ARGS_TWO, ARGS_THREE);
    CHECK_COND_RET(status == napi_ok, nullptr, "Failed to get object info");

    std::vector<std::string> assetArray;
    CHECK_COND_WITH_ERR_MESSAGE(env,
        MediaLibraryNapiUtils::GetStringArray(env, ctx->argv[PARAM0], ctx->cloneCtx.fileUris) == napi_ok,
        JS_E_PARAM_INVALID, "Failed to get assets");

    shared_ptr<PhotoAlbum> targetAlbum = nullptr;
    CHECK_COND_WITH_ERR_MESSAGE(env, ParsePhotoAlbum(env, ctx->argv[PARAM1], targetAlbum),
        JS_E_PARAM_INVALID, "4.The target album does not exist");
    NAPI_ASSERT(env, targetAlbum != nullptr, "targetAlbum == nullptr");
    ctx->cloneCtx.albumId = targetAlbum->GetAlbumId();

    if (ctx->argc >= ARGS_THREE) {
        ParseOptions(env, ctx->argv[PARAM2], ctx.get());
    }

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, ctx, "JSCloneAssetsByPath",
        ExecuteCloneAssetsByPath, CompleteCloneAssetsByPath);
}

} // namespace Media
} // namespace OHOS
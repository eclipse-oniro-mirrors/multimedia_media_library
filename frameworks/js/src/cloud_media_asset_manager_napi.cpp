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

#define MLOG_TAG "CloudMediaAssetManagerNapi"

#include "cloud_media_asset_manager_napi.h"

#include "medialibrary_client_errno.h"
#include "medialibrary_tracer.h"
#include "userfile_client.h"
#include "cloud_media_asset_status_napi.h"
#include "cloud_media_asset_uri.h"
#include "start_download_cloud_media_vo.h"
#include "retain_cloud_media_asset_vo.h"
#include "medialibrary_business_code.h"
#include "user_define_ipc_client.h"
#include "medialibrary_business_code.h"
#include "get_cloudmedia_asset_status_vo.h"
#include "user_define_ipc_client.h"
#include "start_batch_download_cloud_resources_vo.h"
#include "resume_batch_download_cloud_resources_vo.h"
#include "pause_batch_download_cloud_resources_vo.h"
#include "cancel_batch_download_cloud_resources_vo.h"
#include "get_batch_download_cloud_resources_status_vo.h"
#include "get_batch_download_cloud_resources_count_vo.h"
#include "cloud_media_download_resources_status_napi.h"
#include "medialibrary_napi_utils.h"
#include "js_proxy.h"
#include "datashare_predicates.h"
#include "datashare_result_set.h"
#include "rdb_utils.h"
#include "permission_utils.h"
#include "access_token.h"
#include "accesstoken_kit.h"
#include "medialibrary_napi_enum_comm.h"

using namespace std;
using namespace OHOS::Security::AccessToken;
namespace OHOS::Media {
static const string CLOUD_MEDIA_ASSET_MANAGER_CLASS = "CloudMediaAssetManager";
thread_local napi_ref CloudMediaAssetManagerNapi::constructor_ = nullptr;
thread_local napi_ref CloudMediaAssetManagerNapi::sdownloadCloudAssetCodeeEnumRef_ = nullptr;
thread_local napi_ref CloudMediaAssetManagerNapi::sdownloadAssetsNotifyTypeEnumRef_ = nullptr;

thread_local unique_ptr<AssetManagerChangeListenerNapi> g_listAssetListenerObj = nullptr;
const size_t TYPE_SIZE = 6;
const int32_t INDEX_ZERO = 0;
const int32_t INDEX_ONE = 1;
const int32_t INDEX_TWO = 2;
const int32_t INDEX_THREE = 3;
const int32_t INDEX_FOUR = 4;
const int32_t INDEX_FIVE = 5;
const int32_t BATCH_DOWNLOAD_LIMIT = 500;

napi_value CloudMediaAssetManagerNapi::Init(napi_env env, napi_value exports)
{
    NapiClassInfo info = {
        .name = CLOUD_MEDIA_ASSET_MANAGER_CLASS,
        .ref = &constructor_,
        .constructor = Constructor,
        .props = {
            DECLARE_NAPI_STATIC_FUNCTION("getCloudMediaAssetManagerInstance", JSGetCloudMediaAssetManagerInstance),
            DECLARE_NAPI_FUNCTION("startDownloadCloudMedia", JSStartDownloadCloudMedia),
            DECLARE_NAPI_FUNCTION("pauseDownloadCloudMedia", JSPauseDownloadCloudMedia),
            DECLARE_NAPI_FUNCTION("cancelDownloadCloudMedia", JSCancelDownloadCloudMedia),
            DECLARE_NAPI_FUNCTION("retainCloudMediaAsset", JSRetainCloudMediaAsset),
            DECLARE_NAPI_FUNCTION("getCloudMediaAssetStatus", JSGetCloudMediaAssetStatus),
            DECLARE_NAPI_FUNCTION("startDownloadSpecificCloudMedia", JSStartBatchDownloadCloudResources),
            DECLARE_NAPI_FUNCTION("pauseDownloadSpecificCloudMedia", JSPauseDownloadCloudResources),
            DECLARE_NAPI_FUNCTION("resumeDownloadSpecificCloudMedia", JSResumeBatchDownloadCloudResources),
            DECLARE_NAPI_FUNCTION("cancelDownloadSpecificCloudMedia", JSCancelDownloadCloudResources),
            DECLARE_NAPI_FUNCTION("queryDownloadSpecificCloudMediaDetails", JSGetBatchDownloadCloudResourcesStatus),
            DECLARE_NAPI_FUNCTION("queryDownloadSpecificCloudMediaTaskCount", JSGetBatchDownloadSpecificTaskCount),
            DECLARE_NAPI_FUNCTION("onDownloadProgressChange", JsBatchDownloadRegisterCallback),
            DECLARE_NAPI_FUNCTION("offDownloadProgressChange", JsBatchDownloadUnRegisterCallback),
        } };
    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);

    const vector<napi_property_descriptor> staticProps = {
        DECLARE_NAPI_PROPERTY("CloudAssetDownloadCode", CreateDownloadCloudAssetCodeEnum(env)),
        DECLARE_NAPI_PROPERTY("CloudAssetDownloadNotifyType", CreateDownloadAssetsNotifyTypeEnum(env)),
    };
    MediaLibraryNapiUtils::NapiAddStaticProps(env, exports, staticProps);
    return exports;
}

napi_value CloudMediaAssetManagerNapi::Constructor(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL,
            "The cloud media asset manager instance can be called only by system apps");
        return nullptr;
    }
    napi_value newTarget = nullptr;
    CHECK_ARGS(env, napi_get_new_target(env, info, &newTarget), JS_INNER_FAIL);
    CHECK_COND_RET(newTarget != nullptr, nullptr, "Failed to check new.target");

    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = { 0 };
    napi_value thisVar = nullptr;
    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, argc == ARGS_ONE, "Number of args is invalid");
    if (!InitUserFileClient(env, info)) {
        NAPI_ERR_LOG("Failed to init UserFileClient");
        return nullptr;
    }

    unique_ptr<CloudMediaAssetManagerNapi> obj = make_unique<CloudMediaAssetManagerNapi>();
    CHECK_COND(env, obj != nullptr, JS_INNER_FAIL);
    // Initialize the ChangeListener object
    if (g_listAssetListenerObj == nullptr) {
        g_listAssetListenerObj = make_unique<AssetManagerChangeListenerNapi>(env);
    }
    CHECK_ARGS(env,
        napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()), CloudMediaAssetManagerNapi::Destructor, nullptr,
            nullptr),
        JS_INNER_FAIL);

    obj.release();
    return thisVar;
}

void CloudMediaAssetManagerNapi::Destructor(napi_env env, void* nativeObject, void* finalizeHint)
{
    auto* cloudMediaAssetManager = reinterpret_cast<CloudMediaAssetManagerNapi*>(nativeObject);
    if (cloudMediaAssetManager == nullptr) {
        NAPI_ERR_LOG("cloudMediaAssetManager is nullptr");
        return;
    }
    delete cloudMediaAssetManager;
    cloudMediaAssetManager = nullptr;
}

static bool CheckWhetherInitSuccess(napi_env env, napi_value value, bool checkIsValid)
{
    napi_value propertyNames;
    uint32_t propertyLength;
    napi_valuetype valueType = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, value, &valueType), false);
    if (valueType != napi_object) {
        NAPI_ERR_LOG("valueType is not valid");
        return false;
    }

    NAPI_CALL_BASE(env, napi_get_property_names(env, value, &propertyNames), false);
    NAPI_CALL_BASE(env, napi_get_array_length(env, propertyNames, &propertyLength), false);
    if (propertyLength == 0) {
        NAPI_ERR_LOG("propertyLength is 0");
        return false;
    }
    if (checkIsValid && (!UserFileClient::IsValid())) {
        NAPI_ERR_LOG("UserFileClient is not valid");
        return false;
    }
    return true;
}

napi_value CloudMediaAssetManagerNapi::JSGetCloudMediaAssetManagerInstance(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetCloudMediaAssetManagerInstance");

    constexpr size_t ARG_CONTEXT = 1;
    size_t argc = ARG_CONTEXT;
    napi_value argv[ARGS_TWO] = {0};

    napi_value thisVar = nullptr;
    napi_value ctor = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr));
    NAPI_CALL(env, napi_get_reference_value(env, constructor_, &ctor));

    napi_value result = nullptr;
    NAPI_CALL(env, napi_new_instance(env, ctor, argc, argv, &result));
    if (!CheckWhetherInitSuccess(env, result, false)) {
        NAPI_ERR_LOG("Init Cloud Media Asset Manager Instance is failed");
        NAPI_CALL(env, napi_get_undefined(env, &result));
    }
    return result;
}

bool CloudMediaAssetManagerNapi::InitUserFileClient(napi_env env, napi_callback_info info)
{
    if (UserFileClient::IsValid()) {
        return true;
    }

    std::unique_lock<std::mutex> helperLock(MediaLibraryNapi::sUserFileClientMutex_);
    if (!UserFileClient::IsValid()) {
        UserFileClient::Init(env, info);
    }
    helperLock.unlock();
    return UserFileClient::IsValid();
}

static napi_status ParseArgCloudMediaDownloadType(napi_env env, napi_callback_info info,
    unique_ptr<CloudMediaAssetAsyncContext>& context)
{
    CHECK_STATUS_RET(MediaLibraryNapiUtils::AsyncContextGetArgs(env, info, context, ARGS_ONE, ARGS_ONE),
        "Failed to get args");
    napi_valuetype valueType = napi_undefined;
    CHECK_STATUS_RET(napi_typeof(env, context->argv[ARGS_ZERO], &valueType), "Failed to get type");
    CHECK_COND_RET(valueType == napi_number, napi_number_expected, "Type is not as expected number");
    CHECK_STATUS_RET(napi_get_value_int32(env, context->argv[ARGS_ZERO], &context->cloudMediaDownloadType),
        "Failed to get int32 value");
    return napi_ok;
}

static napi_status ParseArgCloudMediaRetainType(napi_env env, napi_callback_info info,
    unique_ptr<CloudMediaAssetAsyncContext>& context)
{
    CHECK_STATUS_RET(MediaLibraryNapiUtils::AsyncContextGetArgs(env, info, context, ARGS_ONE, ARGS_ONE),
        "Failed to get args");
    napi_valuetype valueType = napi_undefined;
    CHECK_STATUS_RET(napi_typeof(env, context->argv[ARGS_ZERO], &valueType), "Failed to get type");
    CHECK_COND_RET(valueType == napi_number, napi_number_expected, "Type is not as expected number");
    CHECK_STATUS_RET(napi_get_value_int32(env, context->argv[ARGS_ZERO], &context->cloudMediaRetainType),
        "Failed to get int32 value");
    return napi_ok;
}

static void StartDownloadCloudMediaExecute(napi_env env, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("StartDownloadCloudMediaExecute");
    NAPI_ERR_LOG("enter StartDownloadCloudMediaExecute");

    auto* context = static_cast<CloudMediaAssetAsyncContext*>(data);
    StartDownloadCloudMediaReqBody reqBody;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::START_DOWNLOAD_CLOUDMEDIA);
    reqBody.cloudMediaType = context->cloudMediaDownloadType;
    NAPI_ERR_LOG("before IPC::UserDefineIPCClient().Call, retain type: %{public}d", reqBody.cloudMediaType);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    NAPI_ERR_LOG("after IPC::UserDefineIPCClient().Call, retain type: %{public}d", reqBody.cloudMediaType);
    if (ret < 0) {
        context->SaveError(ret);
        NAPI_ERR_LOG("Start download cloud media failed, err: %{public}d", ret);
    }
}

static void StartDownloadCloudMediaCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto* context = static_cast<CloudMediaAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);
    napi_get_undefined(env, &jsContext->error);
    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    delete context;
}

napi_value CloudMediaAssetManagerNapi::JSStartDownloadCloudMedia(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JSStartDownloadCloudMedia");

    auto asyncContext = make_unique<CloudMediaAssetAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env, ParseArgCloudMediaDownloadType(env, info, asyncContext) == napi_ok,
        "Failed to parse args");
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "StartDownloadCloudMedia",
        StartDownloadCloudMediaExecute, StartDownloadCloudMediaCompleteCallback);
}

static void PauseDownloadCloudMediaExecute(napi_env env, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PauseDownloadCloudMediaExecute");
    NAPI_ERR_LOG("enter PauseDownloadCloudMediaExecute");

    auto* context = static_cast<CloudMediaAssetAsyncContext*>(data);
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAUSE_DOWNLOAD_CLOUDMEDIA);
    NAPI_ERR_LOG("before IPC::UserDefineIPCClient().Call");
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode);
    NAPI_ERR_LOG("after IPC::UserDefineIPCClient().Call");
    if (ret < 0) {
        context->SaveError(ret);
        NAPI_ERR_LOG("Pause download cloud media failed, err: %{public}d", ret);
    }
}

static void PauseDownloadCloudMediaCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto* context = static_cast<CloudMediaAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);
    napi_get_undefined(env, &jsContext->error);
    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    delete context;
}

napi_value CloudMediaAssetManagerNapi::JSPauseDownloadCloudMedia(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JSPauseDownloadCloudMedia");

    auto asyncContext = make_unique<CloudMediaAssetAsyncContext>();
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PauseDownloadCloudMedia",
        PauseDownloadCloudMediaExecute, PauseDownloadCloudMediaCompleteCallback);
}

static void CancelDownloadCloudMediaExecute(napi_env env, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("CancelDownloadCloudMediaExecute");
    NAPI_ERR_LOG("enter CancelDownloadCloudMediaExecute");

    auto* context = static_cast<CloudMediaAssetAsyncContext*>(data);
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CANCEL_DOWNLOAD_CLOUDMEDIA);
    NAPI_ERR_LOG("before IPC::UserDefineIPCClient().Call");
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode);
    NAPI_ERR_LOG("after IPC::UserDefineIPCClient().Call");
    if (ret < 0) {
        context->SaveError(ret);
        NAPI_ERR_LOG("Cancel download cloud media failed, err: %{public}d", ret);
    }
}

static void CancelDownloadCloudMediaCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto* context = static_cast<CloudMediaAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);
    napi_get_undefined(env, &jsContext->error);
    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    delete context;
}

napi_value CloudMediaAssetManagerNapi::JSCancelDownloadCloudMedia(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JSCancelDownloadCloudMedia");

    auto asyncContext = make_unique<CloudMediaAssetAsyncContext>();
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "CancelDownloadCloudMedia",
        CancelDownloadCloudMediaExecute, CancelDownloadCloudMediaCompleteCallback);
}

static void RetainCloudMediaAssetExecute(napi_env env, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("RetainCloudMediaAssetExecute");
    NAPI_INFO_LOG("enter RetainCloudMediaAssetExecute");

    auto* context = static_cast<CloudMediaAssetAsyncContext*>(data);
    RetainCloudMediaAssetReqBody reqBody;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::RETAIN_CLOUDMEDIA_ASSET);
    reqBody.cloudMediaRetainType = context->cloudMediaRetainType;
    NAPI_INFO_LOG("before IPC::UserDefineIPCClient().Call, retain type: %{public}d", reqBody.cloudMediaRetainType);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    NAPI_INFO_LOG("after IPC::UserDefineIPCClient().Call, retain type: %{public}d", reqBody.cloudMediaRetainType);
    if (ret < 0) {
        context->SaveError(ret);
        NAPI_ERR_LOG("Retain cloud media asset failed, err: %{public}d", ret);
    }
}

static void RetainCloudMediaAssetCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto* context = static_cast<CloudMediaAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);
    napi_get_undefined(env, &jsContext->error);
    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    delete context;
}

napi_value CloudMediaAssetManagerNapi::JSRetainCloudMediaAsset(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JSRetainCloudMediaAsset");

    auto asyncContext = make_unique<CloudMediaAssetAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env, ParseArgCloudMediaRetainType(env, info, asyncContext) == napi_ok,
        "Failed to parse args");
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "RetainCloudMediaAsset",
        RetainCloudMediaAssetExecute, RetainCloudMediaAssetCompleteCallback);
}

static bool CanConvertToInt32(const std::string &str)
{
    std::istringstream stringStream(str);
    int32_t num = 0;
    stringStream >> num;
    return stringStream.eof() && !stringStream.fail();
}

static bool SplitUriString(const std::string& str, std::vector<std::string> &type)
{
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (item.empty()) {
            return false;
        }
        type.emplace_back(item);
    }
    return type.size() == TYPE_SIZE;
}

static void GetCloudMediaAssetStatusExecute(napi_env env, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetCloudMediaAssetStatusExecute");
    NAPI_ERR_LOG("enter GetCloudMediaAssetStatusExecute");

    auto* context = static_cast<CloudMediaAssetAsyncContext*>(data);
    GetCloudMediaAssetStatusReqBody reqBody;
    GetCloudMediaAssetStatusReqBody respBody;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_GET_CLOUDMEDIA_ASSET_STATUS);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    if (ret != 0) {
        context->SaveError(ret);
        NAPI_ERR_LOG("Get cloud media asset status failed, err: %{public}d", ret);
        return;
    }

    NAPI_ERR_LOG("Get cloud media asset, res: %{public}s.", respBody.status.c_str());
    std::vector<std::string> type;
    if (!SplitUriString(respBody.status, type)) {
        NAPI_ERR_LOG("GetType failed");
        return;
    }
    if (!CanConvertToInt32(type[INDEX_ZERO]) || !CanConvertToInt32(type[INDEX_FIVE])) {
        NAPI_ERR_LOG("GetType failed");
        return;
    }
    context->cloudMediaAssetTaskStatus_ = static_cast<CloudMediaAssetTaskStatus>(std::stoi(type[INDEX_ZERO]));
    context->cloudMediaTaskPauseCause_ = static_cast<CloudMediaTaskPauseCause>(std::stoi(type[INDEX_FIVE]));
    std::string taskInfo = "totalcount: " + type[INDEX_ONE] + "," +
                           "totalSize: " + type[INDEX_TWO] + "," +
                           "remainCount: " + type[INDEX_THREE] + "," +
                           "remainSize: " + type[INDEX_FOUR];
    context->taskInfo_ = taskInfo;
}

static void GetCloudMediaAssetStatusCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto* context = static_cast<CloudMediaAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);
    napi_get_undefined(env, &jsContext->error);
    if (context->error == ERR_DEFAULT) {
        napi_value cloudMediaAssetStatus = CloudMediaAssetStatusNapi::NewCloudMediaAssetStatusNapi(env, context);
        jsContext->data = cloudMediaAssetStatus;
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    delete context;
}

napi_value CloudMediaAssetManagerNapi::JSGetCloudMediaAssetStatus(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JSGetCloudMediaAssetStatus");

    auto asyncContext = make_unique<CloudMediaAssetAsyncContext>();
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "GetCloudMediaAssetStatus",
        GetCloudMediaAssetStatusExecute, GetCloudMediaAssetStatusCompleteCallback);
}

// ---start-------

static bool HasReadPermission()
{
    AccessTokenID tokenCaller = IPCSkeleton::GetSelfTokenID();
    int result = AccessTokenKit::VerifyAccessToken(tokenCaller, PERM_READ_IMAGEVIDEO);
    return result == PermissionState::PERMISSION_GRANTED;
}

static bool CheckNapiCallerPermission(napi_env env)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL,
            "This interface can be called only by system apps with read permission");
        return false;
    }
    if (!HasReadPermission()) {
        NapiError::ThrowError(env, OHOS_PERMISSION_DENIED_CODE, "Have no read permission");
        return false;
    }
    return true;
}

static void StartBatchDownloadCloudResourcesExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("StartBatchDownloadCloudResourcesExecute");
    NAPI_INFO_LOG("Enter StartBatchDownloadCloudResourcesExecute");
    auto* context = static_cast<CloudMediaAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    StartBatchDownloadCloudResourcesReqBody reqBody;
    StartBatchDownloadCloudResourcesRespBody respBody;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::START_BATCH_DOWNLOAD_CLOUD_RESOURCES);
    reqBody.uris = context->startBatchDownloadUris;
    NAPI_INFO_LOG("Before StartBatchDownloadCloudResources IPC::UserDefineIPCClient().Call");
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    NAPI_INFO_LOG("After StartBatchDownloadCloudResources IPC::UserDefineIPCClient().Call %{public}d", ret);
    if (ret < 0) {
        context->SaveError(E_INNER_FAIL);
        NAPI_ERR_LOG("Start download cloud media failed, err: %{public}d", ret);
    }
    if (!respBody.uriStatusMap.empty()) {
        context->startBatchDownloadResp = respBody.uriStatusMap;
    }
    NAPI_INFO_LOG("After IPC StartBatchDownloadCloudResourcesExecute resp size: %{public}zu",
        respBody.uriStatusMap.size());
    tracer.Finish();
}

static napi_value GetUriStatusMap(napi_env env, std::map<std::string, int32_t> fileResult)
{
    napi_status status;
    napi_value mapNapiValue {nullptr};
    status = napi_create_map(env, &mapNapiValue);
    CHECK_COND_RET(status == napi_ok && mapNapiValue != nullptr, nullptr,
        "Failed to create map napi value, napi status: %{public}d", static_cast<int>(status));

    NAPI_INFO_LOG("GetUriStatusMap size: %{public}d", static_cast<int32_t>(fileResult.size()));
    for (auto &iter : fileResult) {
        napi_value uriStr {nullptr};
        status = napi_create_string_utf8(env, iter.first.c_str(), NAPI_AUTO_LENGTH, &uriStr);
        CHECK_COND_RET(status == napi_ok && uriStr != nullptr, nullptr,
            "Failed to create uriStr, napi status: %{public}d", static_cast<int>(status));
        napi_value uriStatus {nullptr};
        status = napi_create_int32(env, iter.second, &uriStatus);
        CHECK_COND_RET(status == napi_ok && uriStatus != nullptr, nullptr,
            "Failed to create uriStatus, napi status: %{public}d", static_cast<int>(status));
        status = napi_map_set_property(env, mapNapiValue, uriStr, uriStatus);
        CHECK_COND_RET(status == napi_ok, nullptr, "Failed to set albumMap, napi status: %{public}d",
            static_cast<int>(status));
    }
    return mapNapiValue;
}

static void StartBatchDownloadCloudResourcesCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("StartBatchDownloadCloudResourcesCallback");
    NAPI_INFO_LOG("Enter StartBatchDownloadCloudResourcesCallback");
    auto* context = static_cast<CloudMediaAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    NAPI_INFO_LOG("After IPC StartBatchDownloadCloudResourcesCallback %{public}d", context->error);
    if (context->error == ERR_DEFAULT) {
        napi_value fileResult = GetUriStatusMap(env, move(context->startBatchDownloadResp));
        jsContext->data = fileResult;
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }
    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    delete context;
}

static napi_status ParseArgsStartBatchDownloadCloudResources(napi_env env, napi_callback_info info,
    unique_ptr<CloudMediaAssetAsyncContext> &context)
{
    /* Parse the first argument */
    constexpr size_t minArgs = ARGS_ONE;
    CHECK_STATUS_RET(MediaLibraryNapiUtils::AsyncContextGetArgs(env, info, context, minArgs, minArgs),
        "Failed to get args");
    vector<string> uris;
    CHECK_STATUS_RET(MediaLibraryNapiUtils::GetStringArray(env, context->argv[ARGS_ZERO], uris), "Failed to get uris");
    if (uris.size() > BATCH_DOWNLOAD_LIMIT) { // not allow add more than 500 a batch
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Failed to start batch download, more than 500 piece!");
        return napi_invalid_arg;
    }
    for (const auto &uri : uris) {
        if (!MediaFileUtils::StartsWith(uri, PhotoColumn::PHOTO_URI_PREFIX)) {
            NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Failed to check uri format, not a photo uri!");
            return napi_invalid_arg;
        }
        string fileId = MediaFileUtils::GetIdFromUri(uri);
        if (fileId.empty() || !all_of(fileId.begin(), fileId.end(), ::isdigit)) {
            NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Failed to check uri format, not valid photo uri!");
            return napi_invalid_arg;
        }
    }
    if (uris.empty()) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Failed parse uri, no valid photo uri!");
        return napi_invalid_arg;
    }
    context->startBatchDownloadUris = uris;
    return napi_ok;
}

napi_value CloudMediaAssetManagerNapi::JSStartBatchDownloadCloudResources(napi_env env, napi_callback_info info)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    if (!CheckNapiCallerPermission(env)) {
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JSStartBatchDownloadCloudResources");
    NAPI_INFO_LOG("Enter JSStartBatchDownloadCloudResources");
    unique_ptr<CloudMediaAssetAsyncContext> asyncContext = make_unique<CloudMediaAssetAsyncContext>();
    CHECK_COND_WITH_ERR_MESSAGE(env, ParseArgsStartBatchDownloadCloudResources(env, info, asyncContext) == napi_ok,
        JS_E_PARAM_INVALID, "Failed to parse js args");
    tracer.Finish();
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSStartBatchDownloadCloudResources",
        StartBatchDownloadCloudResourcesExecute, StartBatchDownloadCloudResourcesCallback);
#else
    NapiError::ThrowError(env, MediaLibraryNotifyUtils::ConvertToJsError(JS_E_INNER_FAIL));
    return nullptr;
#endif
}

// ------resume----
static void ResumeBatchDownloadCloudResourcesExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("ResumeBatchDownloadCloudResourcesExecute");
    NAPI_INFO_LOG("Enter ResumeBatchDownloadCloudResourcesExecute");
    auto* context = static_cast<CloudMediaAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    ResumeBatchDownloadCloudResourcesReqBody reqBody;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::RESUME_BATCH_DOWNLOAD_CLOUD_RESOURCES);
    reqBody.uris = context->resumeBatchDownloadUris;
    NAPI_INFO_LOG("Before ResumeBatchDownloadCloudResourcesExecute IPC::UserDefineIPCClient().Call");
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    NAPI_INFO_LOG("After ResumeBatchDownloadCloudResourcesExecute IPC::UserDefineIPCClient().Call %{public}d", ret);
    NAPI_INFO_LOG("After IPC ResumeBatchDownloadCloudResourcesExecute ret ");
    if (ret < 0) {
        context->SaveError(E_INNER_FAIL);
        NAPI_ERR_LOG("Resume download cloud media failed, err: %{public}d", ret);
    }
    tracer.Finish();
}

static void ResumeBatchDownloadCloudResourcesCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("ResumeBatchDownloadCloudResourcesCallback");
    NAPI_INFO_LOG("Enter ResumeBatchDownloadCloudResourcesCallback");
    auto* context = static_cast<CloudMediaAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    NAPI_INFO_LOG("After IPC ResumeBatchDownloadCloudResourcesCallback %{public}d", context->error);
    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }
    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    delete context;
}

static napi_status ParseArgsResumeBatchDownloadCloudResources(napi_env env, napi_callback_info info,
    unique_ptr<CloudMediaAssetAsyncContext> &context)
{
    /* Parse the first argument */
    constexpr size_t minArgs = ARGS_ONE;
    CHECK_STATUS_RET(MediaLibraryNapiUtils::AsyncContextGetArgs(env, info, context, minArgs, minArgs),
        "Failed to get args");
    vector<string> uris;
    MediaLibraryNapiUtils::GetStringArray(env, context->argv[ARGS_ZERO], uris); // 接受null
    for (const auto &uri : uris) {
        if (!MediaFileUtils::StartsWith(uri, PhotoColumn::PHOTO_URI_PREFIX)) {
            NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Failed to check uri format, not a photo uri!");
            return napi_invalid_arg;
        }
        string fileId = MediaFileUtils::GetIdFromUri(uri);
        if (fileId.empty() || !all_of(fileId.begin(), fileId.end(), ::isdigit)) {
            NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Failed to check uri format, not valid photo uri!");
            return napi_invalid_arg;
        }
    }
    if (uris.size() > BATCH_DOWNLOAD_LIMIT) { // not allow add more than 500 a batch
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Failed to resume batch download, more than 500 piece!");
        return napi_invalid_arg;
    }
    context->resumeBatchDownloadUris = uris;
    return napi_ok;
}

napi_value CloudMediaAssetManagerNapi::JSResumeBatchDownloadCloudResources(napi_env env, napi_callback_info info)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    if (!CheckNapiCallerPermission(env)) {
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JSResumeBatchDownloadCloudResources");
    NAPI_INFO_LOG("Enter JSResumeBatchDownloadCloudResources");
    unique_ptr<CloudMediaAssetAsyncContext> asyncContext = make_unique<CloudMediaAssetAsyncContext>();
    CHECK_COND_WITH_ERR_MESSAGE(env, ParseArgsResumeBatchDownloadCloudResources(env, info, asyncContext) == napi_ok,
        JS_E_PARAM_INVALID, "Failed to parse js args");
    tracer.Finish();
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSResumeBatchDownloadCloudResources",
        ResumeBatchDownloadCloudResourcesExecute, ResumeBatchDownloadCloudResourcesCallback);
#else
    NapiError::ThrowError(env, MediaLibraryNotifyUtils::ConvertToJsError(JS_E_INNER_FAIL));
    return nullptr;
#endif
}
// ----pause------
static void PauseDownloadCloudResourcesExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PauseDownloadCloudResourcesExecute");
    NAPI_INFO_LOG("Enter PauseDownloadCloudResourcesExecute");
    auto* context = static_cast<CloudMediaAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    PauseBatchDownloadCloudResourcesReqBody reqBody;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAUSE_BATCH_DOWNLOAD_CLOUD_RESOURCES);
    reqBody.uris = context->pauseBatchDownloadUris;
    NAPI_INFO_LOG("Before PauseDownloadCloudResources IPC::UserDefineIPCClient().Call");
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    NAPI_INFO_LOG("After PauseDownloadCloudResources IPC::UserDefineIPCClient().Call %{public}d", ret);
    if (ret < 0) {
        context->SaveError(E_INNER_FAIL);
        NAPI_ERR_LOG("Pause batch download cloud media failed, err: %{public}d", ret);
    }
    tracer.Finish();
}

static void PauseDownloadCloudResourcesCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PauseDownloadCloudResourcesCallback");
    NAPI_INFO_LOG("Enter PauseDownloadCloudResourcesCallback");
    auto* context = static_cast<CloudMediaAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    NAPI_INFO_LOG("After IPC PauseDownloadCloudResourcesCallback %{public}d", context->error);
    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }
    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    delete context;
}

static napi_status ParseArgsPauseDownloadCloudResources(napi_env env, napi_callback_info info,
    unique_ptr<CloudMediaAssetAsyncContext> &context)
{
    /* Parse the first argument */
    constexpr size_t minArgs = ARGS_ONE;
    CHECK_STATUS_RET(MediaLibraryNapiUtils::AsyncContextGetArgs(env, info, context, minArgs, minArgs),
        "Failed to get args");
    vector<string> uris;
    MediaLibraryNapiUtils::GetStringArray(env, context->argv[ARGS_ZERO], uris); // 接受null
    for (const auto &uri : uris) {
        if (!MediaFileUtils::StartsWith(uri, PhotoColumn::PHOTO_URI_PREFIX)) {
            NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Failed to check uri format, not a photo uri!");
            return napi_invalid_arg;
        }
        string fileId = MediaFileUtils::GetIdFromUri(uri);
        if (fileId.empty() || !all_of(fileId.begin(), fileId.end(), ::isdigit)) {
            NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Failed to check uri format, not valid photo uri!");
            return napi_invalid_arg;
        }
    }
    if (uris.size() > BATCH_DOWNLOAD_LIMIT) { // not allow add more than 500 a batch
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Failed to pause batch download, more than 500 piece!");
        return napi_invalid_arg;
    }
    context->pauseBatchDownloadUris = uris;
    return napi_ok;
}

napi_value CloudMediaAssetManagerNapi::JSPauseDownloadCloudResources(napi_env env, napi_callback_info info)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    if (!CheckNapiCallerPermission(env)) {
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JSPauseDownloadCloudResources");
    NAPI_INFO_LOG("Enter JSPauseDownloadCloudResources");
    unique_ptr<CloudMediaAssetAsyncContext> asyncContext = make_unique<CloudMediaAssetAsyncContext>();
    CHECK_COND_WITH_ERR_MESSAGE(env, ParseArgsPauseDownloadCloudResources(env, info, asyncContext) == napi_ok,
        JS_E_PARAM_INVALID, "Failed to parse js args");
    tracer.Finish();
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSPauseDownloadCloudResources",
        PauseDownloadCloudResourcesExecute, PauseDownloadCloudResourcesCallback);
#else
    NapiError::ThrowError(env, MediaLibraryNotifyUtils::ConvertToJsError(JS_E_INNER_FAIL));
    return nullptr;
#endif
}
// ----cancel------
static void CancelDownloadCloudResourcesExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("CancelDownloadCloudResourcesExecute");
    NAPI_INFO_LOG("Enter CancelDownloadCloudResourcesExecute");
    auto* context = static_cast<CloudMediaAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    CancelBatchDownloadCloudResourcesReqBody reqBody;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CANCEL_BATCH_DOWNLOAD_CLOUD_RESOURCES);
    reqBody.uris = context->cancelBatchDownloadUris;
    NAPI_INFO_LOG("Before CancelDownloadCloudResources IPC::UserDefineIPCClient().Call");
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    NAPI_INFO_LOG("After CancelDownloadCloudResources IPC::UserDefineIPCClient().Call %{public}d", ret);
    if (ret < 0) {
        context->SaveError(E_INNER_FAIL);
        NAPI_ERR_LOG("Cancel Batch Download Cloud Media Failed, err: %{public}d", ret);
    }
    tracer.Finish();
}

static void CancelDownloadCloudResourcesCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("CancelDownloadCloudResourcesCallback");
    NAPI_INFO_LOG("Enter CancelDownloadCloudResourcesCallback");
    auto* context = static_cast<CloudMediaAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    NAPI_INFO_LOG("After IPC CancelDownloadCloudResourcesCallback %{public}d", context->error);
    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }
    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    delete context;
}

static napi_status ParseArgsCancelDownloadCloudResources(napi_env env, napi_callback_info info,
    unique_ptr<CloudMediaAssetAsyncContext> &context)
{
    /* Parse the first argument */
    constexpr size_t minArgs = ARGS_ONE;
    CHECK_STATUS_RET(MediaLibraryNapiUtils::AsyncContextGetArgs(env, info, context, minArgs, minArgs),
        "Failed to get args");
    vector<string> uris;
    MediaLibraryNapiUtils::GetStringArray(env, context->argv[ARGS_ZERO], uris); // 接受null
    for (const auto &uri : uris) {
        if (!MediaFileUtils::StartsWith(uri, PhotoColumn::PHOTO_URI_PREFIX)) {
            NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Failed to check uri format, not a photo uri!");
            return napi_invalid_arg;
        }
        string fileId = MediaFileUtils::GetIdFromUri(uri);
        if (fileId.empty() || !all_of(fileId.begin(), fileId.end(), ::isdigit)) {
            NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Failed to check uri format, not valid photo uri!");
            return napi_invalid_arg;
        }
    }
    if (uris.size() > BATCH_DOWNLOAD_LIMIT) { // not allow add more than 500 a batch
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Failed to cancel batch download, more than 500 piece!");
        return napi_invalid_arg;
    }
    context->cancelBatchDownloadUris = uris;
    return napi_ok;
}

napi_value CloudMediaAssetManagerNapi::JSCancelDownloadCloudResources(napi_env env, napi_callback_info info)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    if (!CheckNapiCallerPermission(env)) {
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JSCancelDownloadCloudResources");
    NAPI_INFO_LOG("Enter JSCancelDownloadCloudResources");
    unique_ptr<CloudMediaAssetAsyncContext> asyncContext = make_unique<CloudMediaAssetAsyncContext>();
    CHECK_COND_WITH_ERR_MESSAGE(env, ParseArgsCancelDownloadCloudResources(env, info, asyncContext) == napi_ok,
        JS_E_PARAM_INVALID, "Failed to parse js args");
    tracer.Finish();
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSCancelDownloadCloudResources",
        CancelDownloadCloudResourcesExecute, CancelDownloadCloudResourcesCallback);
#else
    NapiError::ThrowError(env, MediaLibraryNotifyUtils::ConvertToJsError(JS_E_INNER_FAIL));
    return nullptr;
#endif
}
// ---------- get
static void GetBatchDownloadCloudResourcesStatusExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetBatchDownloadCloudResourcesStatusExecute");
    NAPI_INFO_LOG("Enter GetBatchDownloadCloudResourcesStatusExecute");
    auto* context = static_cast<CloudMediaAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    GetBatchDownloadCloudResourcesStatusReqBody reqBody;
    GetBatchDownloadCloudResourcesStatusRespBody respBody;
    uint32_t businessCode =
        static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_GET_CLOUDMEDIA_BATCH_RESOURCES_STATUS);
    reqBody.predicates = context->getStatusBatchDownloadPredicates;
    NAPI_INFO_LOG("Before GetBatchDownloadCloudResources IPC::UserDefineIPCClient().Call");
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    NAPI_INFO_LOG("After GetBatchDownloadCloudResources IPC::UserDefineIPCClient().Call %{public}d", ret);
    if (ret < 0) {
        context->SaveError(E_INNER_FAIL);
        NAPI_ERR_LOG("Get download cloud media status failed, err: %{public}d", ret);
        return;
    }
    context->allBatchDownloadStatus = respBody.downloadResourcesStatus;
    NAPI_INFO_LOG("After IPC GetBatchDownloadCloudResourcesStatusExecute size %{public}zu",
        respBody.downloadResourcesStatus.size());
    tracer.Finish();
}

static void GetBatchDownloadCloudResourcesStatusCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetBatchDownloadCloudResourcesStatusCallback");
    NAPI_INFO_LOG("Enter GetBatchDownloadCloudResourcesStatusCallback");
    auto* context = static_cast<CloudMediaAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    NAPI_INFO_LOG("After IPC GetBatchDownloadCloudResourcesStatusCallback %{public}d", context->error);
    if (context->error == ERR_DEFAULT) {
        napi_value downloadCloudResourcesStatus =
            CloudMediaDownloadResourcesStatusNapi::NewCloudMediaDownloadResourcesStatusNapi(env, context);
        jsContext->data = downloadCloudResourcesStatus;
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }
    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    delete context;
}

napi_status ParseBatchDownloadPredicates(napi_env env, const napi_value arg,
    unique_ptr<CloudMediaAssetAsyncContext> &context)
{
    JSProxy::JSProxy<DataShare::DataShareAbsPredicates> *jsProxy = nullptr;
    napi_unwrap(env, arg, reinterpret_cast<void **>(&jsProxy));
    if (jsProxy == nullptr) {
        NAPI_ERR_LOG("jsProxy is invalid");
        return napi_invalid_arg;
    }
    shared_ptr<DataShare::DataShareAbsPredicates> predicate = jsProxy->GetInstance();
    vector<DataShare::OperationItem> operations;
    auto &items = predicate->GetOperationList();
    for (auto &item : items) {
        operations.push_back(item);
    }
    context->getStatusBatchDownloadPredicates = DataShare::DataSharePredicates(move(operations));
    return napi_ok;
}

static napi_status ParseArgsGetBatchDownloadCloudResourcesStatus(napi_env env, napi_callback_info info,
    unique_ptr<CloudMediaAssetAsyncContext> &context)
{
    /* Parse the first argument */
    constexpr size_t minArgs = ARGS_ONE;
    CHECK_STATUS_RET(MediaLibraryNapiUtils::AsyncContextGetArgs(env, info, context, minArgs, minArgs),
        "Failed to get args");
    CHECK_STATUS_RET(ParseBatchDownloadPredicates(env, context->argv[ARGS_ZERO], context), "Failed to get args");
    return napi_ok;
}

napi_value CloudMediaAssetManagerNapi::JSGetBatchDownloadCloudResourcesStatus(napi_env env, napi_callback_info info)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    if (!CheckNapiCallerPermission(env)) {
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JSGetBatchDownloadCloudResourcesStatus");
    NAPI_INFO_LOG("Enter JSGetBatchDownloadCloudResourcesStatus");
    unique_ptr<CloudMediaAssetAsyncContext> asyncContext = make_unique<CloudMediaAssetAsyncContext>();
    CHECK_COND_WITH_ERR_MESSAGE(env, ParseArgsGetBatchDownloadCloudResourcesStatus(env, info, asyncContext) == napi_ok,
        JS_E_PARAM_INVALID, "Failed to parse js args");
    tracer.Finish();
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSGetBatchDownloadCloudResourcesStatus",
        GetBatchDownloadCloudResourcesStatusExecute, GetBatchDownloadCloudResourcesStatusCallback);
#else
    NapiError::ThrowError(env, MediaLibraryNotifyUtils::ConvertToJsError(JS_E_INNER_FAIL));
    return nullptr;
#endif
}

//--------get count
static void GetBatchDownloadSpecificTaskCountExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetBatchDownloadSpecificTaskCountExecute");
    NAPI_INFO_LOG("Enter GetBatchDownloadSpecificTaskCountExecute");
    auto* context = static_cast<CloudMediaAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    GetBatchDownloadCloudResourcesCountReqBody reqBody;
    GetBatchDownloadCloudResourcesCountRespBody respBody;
    uint32_t businessCode =
        static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_GET_CLOUDMEDIA_BATCH_RESOURCES_COUNT);
    reqBody.predicates = context->getCountBatchDownloadPredicates;
    NAPI_INFO_LOG("Before GetBatchDownloadCloudResources IPC::UserDefineIPCClient().Call");
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    NAPI_INFO_LOG("After GetBatchDownloadCloudResources IPC::UserDefineIPCClient().Call %{public}d", ret);
    if (ret < 0) {
        context->SaveError(E_INNER_FAIL);
        NAPI_ERR_LOG("Get download cloud media status failed, err: %{public}d", ret);
        return;
    }
    context->allBatchDownloadCount = respBody.count;
    NAPI_INFO_LOG("After IPC GetBatchDownloadSpecificTaskCountExecute size %{public}d", respBody.count);
    tracer.Finish();
}

static void GetBatchDownloadSpecificTaskCountCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetBatchDownloadSpecificTaskCountCallback");
    NAPI_INFO_LOG("Enter GetBatchDownloadSpecificTaskCountCallback");
    auto* context = static_cast<CloudMediaAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    NAPI_INFO_LOG("After IPC GetBatchDownloadSpecificTaskCountCallback %{public}d", context->error);
    if (context->error == ERR_DEFAULT) {
        CHECK_ARGS_RET_VOID(env, napi_create_int32(env, context->allBatchDownloadCount, &jsContext->data),
            JS_INNER_FAIL);
        jsContext->status = true;
    } else {
        CHECK_ARGS_RET_VOID(env, napi_create_int32(env, -1, &jsContext->data), JS_INNER_FAIL);
        context->HandleError(env, jsContext->error);
    }
    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    delete context;
}

napi_status ParseBatchDownloadCountPredicates(napi_env env, const napi_value arg,
    unique_ptr<CloudMediaAssetAsyncContext> &context)
{
    JSProxy::JSProxy<DataShare::DataShareAbsPredicates> *jsProxy = nullptr;
    napi_unwrap(env, arg, reinterpret_cast<void **>(&jsProxy));
    if (jsProxy == nullptr) {
        NAPI_ERR_LOG("jsProxy is invalid");
        return napi_invalid_arg;
    }
    shared_ptr<DataShare::DataShareAbsPredicates> predicate = jsProxy->GetInstance();
    vector<DataShare::OperationItem> operations;
    auto &items = predicate->GetOperationList();
    for (auto &item : items) {
        operations.push_back(item);
    }
    context->getCountBatchDownloadPredicates = DataShare::DataSharePredicates(move(operations));
    return napi_ok;
}

static napi_status ParseArgsGetBatchDownloadSpecificTaskCount(napi_env env, napi_callback_info info,
    unique_ptr<CloudMediaAssetAsyncContext> &context)
{
    /* Parse the first argument */
    constexpr size_t minArgs = ARGS_ONE;
    CHECK_STATUS_RET(MediaLibraryNapiUtils::AsyncContextGetArgs(env, info, context, minArgs, minArgs),
        "Failed to get args");
    CHECK_STATUS_RET(ParseBatchDownloadCountPredicates(env, context->argv[ARGS_ZERO], context), "Failed to get args");
    return napi_ok;
}

napi_value CloudMediaAssetManagerNapi::JSGetBatchDownloadSpecificTaskCount(napi_env env, napi_callback_info info)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    if (!CheckNapiCallerPermission(env)) {
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JSGetBatchDownloadSpecificTaskCount");
    NAPI_INFO_LOG("Enter JSGetBatchDownloadSpecificTaskCount");
    unique_ptr<CloudMediaAssetAsyncContext> asyncContext = make_unique<CloudMediaAssetAsyncContext>();
    CHECK_COND_WITH_ERR_MESSAGE(env, ParseArgsGetBatchDownloadSpecificTaskCount(env, info, asyncContext) == napi_ok,
        JS_E_PARAM_INVALID, "Failed to parse js args");
    tracer.Finish();
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSGetBatchDownloadSpecificTaskCount",
        GetBatchDownloadSpecificTaskCountExecute, GetBatchDownloadSpecificTaskCountCallback);
#else
    NapiError::ThrowError(env, MediaLibraryNotifyUtils::ConvertToJsError(JS_E_INNER_FAIL));
    return nullptr;
#endif
}
// ----------
napi_value CloudMediaAssetManagerNapi::JsBatchDownloadRegisterCallback(napi_env env, napi_callback_info info)
{
    if (!CheckNapiCallerPermission(env)) {
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JsBatchDownloadRegisterCallback");
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {nullptr};
    napi_value thisVar = nullptr;
    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc == ARGS_ONE, "requires 1 parameters");
    CloudMediaAssetManagerNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&obj));
    if (status == napi_ok && obj != nullptr) {
        napi_valuetype valueType = napi_undefined;
        if (napi_typeof(env, argv[PARAM0], &valueType) != napi_ok || valueType != napi_function) {
            return undefinedResult;
        }
        Notification::NotifyUriType uriType = Notification::NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI;
        const int32_t refCount = 1;
        napi_ref cbOnRef = nullptr;
        napi_create_reference(env, argv[PARAM0], refCount, &cbOnRef);
        int32_t ret = RegisterObserverExecute(env, cbOnRef, *g_listAssetListenerObj, uriType);
        if (ret == E_OK) {
            NAPI_INFO_LOG("JsBatchDownloadRegisterCallback success");
        } else {
            NapiError::ThrowError(env, MediaLibraryNotifyUtils::ConvertToJsError(JS_E_INNER_FAIL));
            napi_delete_reference(env, cbOnRef);
            return undefinedResult;
        }
    }
    return undefinedResult;
}

static napi_value CheckUnregisterCallbackArgs(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    napi_value thisVar = nullptr;
    context->argc = ARGS_ONE;
    GET_JS_ARGS(env, info, context->argc, context->argv, thisVar);

    if (context->argc > ARGS_ONE) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "requires parameters.");
        return nullptr;
    }

    if (thisVar == nullptr || context->argv[PARAM0] == nullptr) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID);
        return nullptr;
    }
    return thisVar;
}

napi_value CloudMediaAssetManagerNapi::JsBatchDownloadUnRegisterCallback(napi_env env, napi_callback_info info)
{
    if (!CheckNapiCallerPermission(env)) {
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JsBatchDownloadUnRegisterCallback");
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    if (g_listAssetListenerObj == nullptr) {
        return undefinedResult;
    }
    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    if (CheckUnregisterCallbackArgs(env, info, context) == nullptr) {
        return undefinedResult;
    }
    Notification::NotifyUriType uriType = Notification::NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI;
    napi_valuetype valueType = napi_undefined;
    napi_ref cbOffRef = nullptr;
    if (context->argc == ARGS_ONE) {
        if (napi_typeof(env, context->argv[PARAM0], &valueType) != napi_ok || valueType != napi_function) {
            NapiError::ThrowError(env, JS_E_PARAM_INVALID);
            return undefinedResult;
        }
        const int32_t refCount = 1;
        napi_create_reference(env, context->argv[PARAM0], refCount, &cbOffRef);
    }
    int32_t ret = UnregisterObserverExecute(env, uriType, cbOffRef, *g_listAssetListenerObj);
    if (ret != E_OK) {
        NapiError::ThrowError(env, MediaLibraryNotifyUtils::ConvertToJsError(JS_E_INNER_FAIL));
    }
    if (cbOffRef != nullptr) {
        napi_delete_reference(env, cbOffRef);
    }
    return undefinedResult;
}

// 遍历ClientObserver 没有的新增
int32_t CloudMediaAssetManagerNapi::AddClientObserver(napi_env env, napi_ref ref,
    std::map<Notification::NotifyUriType, std::vector<std::shared_ptr<ClientObserver>>> &clientObservers,
    const Notification::NotifyUriType uriType)
{
    auto iter = clientObservers.find(uriType);
    if (iter == clientObservers.end()) {
        shared_ptr<ClientObserver> clientObserver = make_shared<ClientObserver>(uriType, ref);
        clientObservers[uriType].push_back(clientObserver);
        return E_OK;
    }
    napi_value callback = nullptr;
    napi_status status = napi_get_reference_value(env, ref, &callback);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Create reference fail, status: %{public}d", status);
        return JS_E_PARAM_INVALID;
    }

    bool hasRegister = false;
    auto observers = iter->second;
    for (auto &observer : observers) {
        napi_value onCallback = nullptr;
        status = napi_get_reference_value(env, observer->ref_, &onCallback);
        if (status != napi_ok) {
            NAPI_ERR_LOG("Create reference fail, status: %{public}d", status);
            return JS_E_PARAM_INVALID;
        }
        napi_strict_equals(env, callback, onCallback, &hasRegister);
        if (hasRegister) {
            NAPI_INFO_LOG("clientObserver hasRegister");
            return E_OK;
        }
    }
    if (!hasRegister) {
        shared_ptr<ClientObserver> clientObserver = make_shared<ClientObserver>(uriType, ref);
        clientObservers[uriType].push_back(clientObserver);
    }
    return E_OK;
}

int32_t CloudMediaAssetManagerNapi::RegisterObserverExecute(napi_env env, napi_ref ref,
    AssetManagerChangeListenerNapi &listObj, const Notification::NotifyUriType uriType)
{
    // 根据uri获取对应的 注册uri
    Notification::NotifyUriType registerUriType = Notification::NotifyUriType::INVALID;
    std::string registerUri = "";
    if (MediaLibraryNotifyUtils::GetAssetManagerNotifyTypeAndUri(uriType, registerUriType, registerUri) != E_OK) {
        return JS_E_PARAM_INVALID;
    }

    for (auto it = listObj.observers_.begin(); it != listObj.observers_.end(); it++) {
        Notification::NotifyUriType observerUri = (*it)->uriType_;
        if (observerUri == registerUriType) {
            //判断是否已有callback，没有则加入，有则返回false
            auto& clientObservers = (*it)->clientObservers_;
            return AddClientObserver(env, ref, clientObservers, uriType);
        }
    }
    // list 中没有，新建一个，并且服务端注册
    shared_ptr<MediaOnNotifyAssetManagerObserver> observer =
        make_shared<MediaOnNotifyAssetManagerObserver>(registerUriType, registerUri, env);
    Uri notifyUri(registerUri);
    int32_t ret = UserFileClient::RegisterObserverExtProvider(notifyUri,
        static_cast<shared_ptr<DataShare::DataShareObserver>>(observer), false);
    if (ret != E_OK) {
        NAPI_ERR_LOG("failed to register observer, ret: %{public}d, uri: %{public}s", ret, registerUri.c_str());
        return ret;
    }
    NAPI_INFO_LOG("Enter Register DownloadProgressChange Observer");
    shared_ptr<ClientObserver> clientObserver = make_shared<ClientObserver>(uriType, ref);
    observer->clientObservers_[uriType].push_back(clientObserver);
    listObj.observers_.push_back(observer);
    NAPI_INFO_LOG("Success To Register Observer, ret: %{public}d, uri: %{public}s", ret, registerUri.c_str());
    return ret;
}


int32_t CloudMediaAssetManagerNapi::RemoveClientObserver(napi_env env, napi_ref ref,
    map<Notification::NotifyUriType, vector<shared_ptr<ClientObserver>>> &clientObservers,
    const Notification::NotifyUriType uriType)
{
    if (clientObservers.find(uriType) == clientObservers.end()) {
        NAPI_ERR_LOG("invalid register uriType");
        return JS_E_PARAM_INVALID;
    }
    if (ref == nullptr) {
        NAPI_INFO_LOG("remove all client observers of uriType");
        clientObservers.erase(uriType);
        return E_OK;
    }
    napi_value offCallback = nullptr;
    napi_status status = napi_get_reference_value(env, ref, &offCallback);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Create reference fail, status: %{public}d", status);
        return JS_E_PARAM_INVALID;
    }

    bool hasRegister = false;
    for (auto iter = clientObservers[uriType].begin(); iter != clientObservers[uriType].end(); iter++) {
        napi_value onCallback = nullptr;
        status = napi_get_reference_value(env, (*iter)->ref_, &onCallback);
        if (status != napi_ok) {
            NAPI_ERR_LOG("Create reference fail, status: %{public}d", status);
            return JS_E_PARAM_INVALID;
        }
        napi_strict_equals(env, offCallback, onCallback, &hasRegister);
        if (!hasRegister) {
            continue;
        }

        clientObservers[uriType].erase(iter);
        if (clientObservers[uriType].empty()) {
            clientObservers.erase(uriType);
        }
        return E_OK;
    }
    NAPI_ERR_LOG("failed to find observer");
    return E_OK;
}

int32_t CloudMediaAssetManagerNapi::UnregisterObserverExecute(napi_env env,
    const Notification::NotifyUriType uriType, napi_ref ref, AssetManagerChangeListenerNapi &listObj)
{
    if (listObj.observers_.size() == 0) {
        NAPI_INFO_LOG("listObj.observers_ size 0");
        return E_OK;
    }
    // 根据uri获取对应的 注册uri
    Notification::NotifyUriType registerUriType = Notification::NotifyUriType::INVALID;
    std::string registerUri = "";
    if (MediaLibraryNotifyUtils::GetAssetManagerNotifyTypeAndUri(uriType, registerUriType, registerUri) != E_OK) {
        return JS_E_PARAM_INVALID;
    }
    // 如果注册uri对应的newObserver不存在，无需解注册
    // 如果注册uri对应的newObserver存在
    // 参数：对应的newObserver的clientobserver中是否存在对应callback，存在，删除并且看看是否删除为空的对应的newObserver
    int32_t ret = JS_E_PARAM_INVALID;
    for (auto it = listObj.observers_.begin(); it != listObj.observers_.end(); it++) {
        Notification::NotifyUriType observerUri = (*it)->uriType_;
        if (observerUri != registerUriType) {
            continue;
        }
        auto& clientObservers = (*it)->clientObservers_;

        ret = RemoveClientObserver(env, ref, clientObservers, uriType);
        if (ret == E_OK && clientObservers.empty()) {
            ret = UserFileClient::UnregisterObserverExtProvider(Uri(registerUri),
                static_cast<shared_ptr<DataShare::DataShareObserver>>(*it));
            if (ret != E_OK) {
                NAPI_ERR_LOG("failed to unregister observer, ret: %{public}d, uri: %{public}s",
                    ret, registerUri.c_str());
                return ret;
            }
            std::vector<shared_ptr<MediaOnNotifyAssetManagerObserver>>::iterator tmp = it;
            listObj.observers_.erase(tmp);
            NAPI_INFO_LOG("Success To UnRegister DownloadProgressChange Observer, ret: %{public}d, uri: %{public}s",
                ret, registerUri.c_str());
        }
        return ret;
    }
    return ret;
}

static napi_status AddIntegerNamedProperty(napi_env env, napi_value object,
    const string &name, int32_t enumValue)
{
    napi_value enumNapiValue;
    napi_status status = napi_create_int32(env, enumValue, &enumNapiValue);
    if (status == napi_ok) {
        status = napi_set_named_property(env, object, name.c_str(), enumNapiValue);
    }
    return status;
}

static napi_value CreateNumberEnumProperty(napi_env env, vector<string> properties, napi_ref &ref, int32_t offset = 0)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    for (size_t i = 0; i < properties.size(); i++) {
        NAPI_CALL(env, AddIntegerNamedProperty(env, result, properties[i], static_cast<int32_t>(i) + offset));
    }
    NAPI_CALL(env, napi_create_reference(env, result, NAPI_INIT_REF_COUNT, &ref));
    return result;
}

napi_value CloudMediaAssetManagerNapi::CreateDownloadCloudAssetCodeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, downloadCloudAssetCodeEnum, sdownloadCloudAssetCodeeEnumRef_);
}

napi_value CloudMediaAssetManagerNapi::CreateDownloadAssetsNotifyTypeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, downloadAssetsNotifyTypeEnum, sdownloadAssetsNotifyTypeEnumRef_);
}
} // namespace OHOS::Media
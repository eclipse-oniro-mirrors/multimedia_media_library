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

#define MLOG_TAG "CloudEnhancementNapi"

#include "cloud_enhancement_napi.h"

#include "medialibrary_client_errno.h"
#include "medialibrary_errno.h"
#include "medialibrary_tracer.h"
#include "userfile_client.h"
#include "result_set_utils.h"
#include "cloud_enhancement_task_state_napi.h"
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
#include "media_enhance_constants_c_api.h"
#include "media_enhance_handles.h"
#endif
#include "cloud_enhancement_uri.h"
#include "cloud_enhancement_vo.h"
#include "medialibrary_business_code.h"
#include "user_define_ipc_client.h"
#include "get_cloud_enhancement_pair_vo.h"
#include "query_cloud_enhancement_task_state_vo.h"

using namespace std;
using namespace OHOS::DataShare;
using namespace OHOS::NativeRdb;
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
using namespace OHOS::MediaEnhance;
#endif

namespace OHOS::Media {
static const string CLOUD_ENHANCEMENT_CLASS = "CloudEnhancement";
thread_local napi_ref CloudEnhancementNapi::constructor_ = nullptr;
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
static void* dynamicHandler = nullptr;
static MediaEnhanceClientHandle* clientWrapper = nullptr;
static mutex mtx;

using CreateMCEClient = MediaEnhanceClientHandle* (*)(MediaEnhance_TASK_TYPE taskType);
using DestroyMCEClient = void (*)(MediaEnhanceClientHandle* client);
using CreateMCEBundle = MediaEnhanceBundleHandle* (*)();
using DestroyMCEBundle = void (*)(MediaEnhanceBundleHandle* bundle);
using ClientLoadSA = int32_t (*)(MediaEnhanceClientHandle* client);
using ClientIsConnected = bool (*)(MediaEnhanceClientHandle* client);
using ClientQueryTaskState = MediaEnhanceBundleHandle* (*)(MediaEnhanceClientHandle* client, const char* taskId,
                                uint32_t taskIdLen);
using BundleHandleGetInt = int32_t (*)(MediaEnhanceBundleHandle* bundle, const char* key, uint32_t keyLen);


static CreateMCEClient createMCEClientFunc = nullptr;
static DestroyMCEClient destroyMCEClientFunc = nullptr;
static CreateMCEBundle createMCEBundleFunc = nullptr;
static DestroyMCEBundle destroyMCEBundleFunc = nullptr;
static ClientLoadSA clientLoadSaFunc = nullptr;
static ClientIsConnected clientIsConnectedFunc = nullptr;
static ClientQueryTaskState clientQueryTaskStateFunc = nullptr;
static BundleHandleGetInt bundleHandleGetIntFunc = nullptr;

static void InitCloudEnhancementBasicFunc(void* dynamicHandler)
{
    if (dynamicHandler == nullptr) {
        NAPI_ERR_LOG("dynamicHandler is null. error:%{public}s", dlerror());
        return;
    }

    if (createMCEClientFunc == nullptr) {
        createMCEClientFunc = (CreateMCEClient)dlsym(dynamicHandler, "CreateMediaEnhanceClient");
    }
    if (createMCEClientFunc == nullptr) {
        NAPI_ERR_LOG("CreateMediaEnhanceClient dlsym failed.error:%{public}s", dlerror());
        return;
    }

    if (destroyMCEClientFunc == nullptr) {
        destroyMCEClientFunc = (DestroyMCEClient)dlsym(dynamicHandler, "DestroyMediaEnhanceClient");
    }
    if (destroyMCEClientFunc == nullptr) {
        NAPI_ERR_LOG("DestroyMediaEnhanceClient dlsym failed.error:%{public}s", dlerror());
        return;
    }

    if (createMCEBundleFunc == nullptr) {
        createMCEBundleFunc = (CreateMCEBundle)dlsym(dynamicHandler, "CreateMediaEnhanceBundle");
    }
    if (createMCEBundleFunc == nullptr) {
        NAPI_ERR_LOG("CreateMediaEnhanceBundle dlsym failed.error:%{public}s", dlerror());
        return;
    }

    if (destroyMCEBundleFunc == nullptr) {
        destroyMCEBundleFunc = (DestroyMCEBundle)dlsym(dynamicHandler, "DestroyMediaEnhanceBundle");
    }
    if (destroyMCEBundleFunc == nullptr) {
        NAPI_ERR_LOG("DestroyMediaEnhanceBundle dlsym failed.error:%{public}s", dlerror());
        return;
    }
}

static void InitCloudEnhancementExtraFunc(void* dynamicHandler)
{
    if (dynamicHandler == nullptr) {
        NAPI_ERR_LOG("dynamicHandler is null. error:%{public}s", dlerror());
        return;
    }

    if (clientLoadSaFunc == nullptr) {
        clientLoadSaFunc = (ClientLoadSA)dlsym(dynamicHandler, "MediaEnhanceClient_LoadSA");
    }
    if (clientLoadSaFunc == nullptr) {
        NAPI_ERR_LOG("MediaEnhanceClient_LoadSA dlsym failed.error:%{public}s", dlerror());
        return;
    }

    if (clientIsConnectedFunc == nullptr) {
        clientIsConnectedFunc = (ClientIsConnected)dlsym(dynamicHandler, "MediaEnhanceClient_IsConnected");
    }
    if (clientIsConnectedFunc == nullptr) {
        NAPI_ERR_LOG("MediaEnhanceClient_IsConnected dlsym failed.error:%{public}s", dlerror());
        return;
    }

    if (clientQueryTaskStateFunc == nullptr) {
        clientQueryTaskStateFunc = (ClientQueryTaskState)dlsym(dynamicHandler, "MediaEnhanceClient_QueryTaskState");
    }
    if (clientQueryTaskStateFunc == nullptr) {
        NAPI_ERR_LOG("MediaEnhanceClient_QueryTaskState dlsym failed. error:%{public}s", dlerror());
        return;
    }

    if (bundleHandleGetIntFunc == nullptr) {
        bundleHandleGetIntFunc = (BundleHandleGetInt)dlsym(dynamicHandler, "MediaEnhanceBundle_GetInt");
    }
    if (bundleHandleGetIntFunc == nullptr) {
        NAPI_ERR_LOG("MediaEnhanceBundle_GetInt dlsym failed. error:%{public}s", dlerror());
        return;
    }
}

static void InitEnhancementClient()
{
    if (createMCEClientFunc == nullptr) {
        createMCEClientFunc = (CreateMCEClient)dlsym(dynamicHandler, "CreateMediaEnhanceClient");
    }
    if (createMCEClientFunc == nullptr) {
        NAPI_ERR_LOG("CreateMediaEnhanceClient dlsym failed.error:%{public}s", dlerror());
        return;
    }
    if (clientWrapper == nullptr && createMCEClientFunc != nullptr) {
        NAPI_INFO_LOG("createMCEClientFunc by dlopen func.");
        clientWrapper = createMCEClientFunc(MediaEnhance_TASK_TYPE::TYPE_CAMERA);
    }
}

static void DestroyEnhancementClient()
{
    if (destroyMCEClientFunc == nullptr) {
        destroyMCEClientFunc = (DestroyMCEClient)dlsym(dynamicHandler, "DestroyMediaEnhanceClient");
    }
    if (destroyMCEClientFunc == nullptr) {
        NAPI_ERR_LOG("DestroyMediaEnhanceClient dlsym failed.error:%{public}s", dlerror());
        return;
    }
    destroyMCEClientFunc(clientWrapper);
    clientWrapper = nullptr;
}

static MediaEnhanceBundleHandle* CreateBundle()
{
    if (createMCEBundleFunc == nullptr) {
        createMCEBundleFunc = (CreateMCEBundle)dlsym(dynamicHandler, "CreateMediaEnhanceBundle");
    }
    if (createMCEBundleFunc == nullptr) {
        NAPI_ERR_LOG("createMCEBundleFunc dlsym failed.error:%{public}s", dlerror());
        return nullptr;
    }
    return createMCEBundleFunc();
}

static void DestroyBundle(MediaEnhanceBundleHandle* bundle)
{
    if (destroyMCEBundleFunc == nullptr) {
        destroyMCEBundleFunc = (DestroyMCEBundle)dlsym(dynamicHandler,
            "DestroyMediaEnhanceBundle");
    }
    if (destroyMCEBundleFunc == nullptr) {
        NAPI_ERR_LOG("destroyMCEBundleFunc dlsym failed.error:%{public}s", dlerror());
        return;
    }
    destroyMCEBundleFunc(bundle);
}

static int32_t LoadSA()
{
    if (clientWrapper == nullptr) {
        NAPI_ERR_LOG("clientWrapper is nullptr!");
        return E_ERR;
    }
    if (clientLoadSaFunc == nullptr) {
        clientLoadSaFunc = (ClientLoadSA)dlsym(dynamicHandler, "MediaEnhanceClient_LoadSA");
    }
    if (clientLoadSaFunc == nullptr) {
        NAPI_ERR_LOG("MediaEnhanceClient_LoadSA dlsym failed.error:%{public}s", dlerror());
        return E_ERR;
    }
    int32_t ret = clientLoadSaFunc(clientWrapper);
    if (ret != E_OK) {
        NAPI_ERR_LOG("Enhancement Service LoadSA failed:%{public}d", ret);
    }
    return ret;
}

static bool IsConnected(MediaEnhanceClientHandle* clientWrapper)
{
    if (clientWrapper == nullptr) {
        NAPI_ERR_LOG("clientWrapper is nullptr!");
        return E_ERR;
    }
    if (clientIsConnectedFunc == nullptr) {
        clientIsConnectedFunc = (ClientIsConnected)dlsym(dynamicHandler,
            "MediaEnhanceClient_IsConnected");
    }
    if (clientIsConnectedFunc == nullptr) {
        NAPI_ERR_LOG("MediaEnhanceClient_IsConnected dlsym failed.error:%{public}s", dlerror());
        return false;
    }
    return clientIsConnectedFunc(clientWrapper);
}

static MediaEnhanceBundleHandle* QueryTaskState(const string &photoId)
{
    if (clientWrapper == nullptr) {
        NAPI_ERR_LOG("clientWrapper is nullptr!");
        return nullptr;
    }
    if (clientQueryTaskStateFunc == nullptr) {
        clientQueryTaskStateFunc = (ClientQueryTaskState)dlsym(dynamicHandler, "MediaEnhanceClient_QueryTaskState");
    }
    if (clientQueryTaskStateFunc == nullptr) {
        NAPI_ERR_LOG("MediaEnhanceClient_QueryTaskState dlsym failed. error:%{public}s", dlerror());
        return nullptr;
    }
    NAPI_INFO_LOG("QueryTaskState photoId: %{public}s", photoId.c_str());
    return clientQueryTaskStateFunc(clientWrapper, photoId.c_str(), strlen(photoId.c_str()));
}

static int32_t GetInt(MediaEnhanceBundleHandle* bundle, const char* key)
{
    if (bundleHandleGetIntFunc == nullptr) {
        bundleHandleGetIntFunc = (BundleHandleGetInt)dlsym(dynamicHandler, "MediaEnhanceBundle_GetInt");
    }
    if (bundleHandleGetIntFunc == nullptr) {
        NAPI_ERR_LOG("MediaEnhanceBundle_GetInt dlsym failed. error:%{public}s", dlerror());
        return E_ERR;
    }
    return bundleHandleGetIntFunc(bundle, key, strlen(key));
}

static void InitCloudEnhancementFunc()
{
    string path = "/system/lib64/platformsdk/libmedia_cloud_enhance_plugin.z.so";
    dynamicHandler = dlopen(path.c_str(), RTLD_NOW);
    InitCloudEnhancementBasicFunc(dynamicHandler);
    InitCloudEnhancementExtraFunc(dynamicHandler);
}
#endif

napi_value CloudEnhancementNapi::Init(napi_env env, napi_value exports)
{
    NapiClassInfo info = { .name = CLOUD_ENHANCEMENT_CLASS,
        .ref = &constructor_,
        .constructor = Constructor,
        .props = {
            DECLARE_NAPI_STATIC_FUNCTION("getCloudEnhancementInstance", JSGetCloudEnhancementInstance),
            DECLARE_NAPI_FUNCTION("submitCloudEnhancementTasks", JSSubmitCloudEnhancementTasks),
            DECLARE_NAPI_FUNCTION("prioritizeCloudEnhancementTask", JSPrioritizeCloudEnhancementTask),
            DECLARE_NAPI_FUNCTION("cancelCloudEnhancementTasks", JSCancelCloudEnhancementTasks),
            DECLARE_NAPI_FUNCTION("cancelAllCloudEnhancementTasks", JSCancelAllCloudEnhancementTasks),
            DECLARE_NAPI_FUNCTION("queryCloudEnhancementTaskState", JSQueryCloudEnhancementTaskState),
            DECLARE_NAPI_FUNCTION("syncCloudEnhancementTaskStatus", JSSyncCloudEnhancementTaskStatus),
            DECLARE_NAPI_FUNCTION("getCloudEnhancementPair", JSGetCloudEnhancementPair),
        } };
    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    InitCloudEnhancementFunc();
#endif
    return exports;
}

napi_value CloudEnhancementNapi::Constructor(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL,
            "The cloud enhancement instance can be called only by system apps");
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

    unique_ptr<CloudEnhancementNapi> obj = make_unique<CloudEnhancementNapi>();
    CHECK_COND(env, obj != nullptr, JS_INNER_FAIL);
    CHECK_ARGS(env,
        napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()), CloudEnhancementNapi::Destructor, nullptr,
            nullptr),
        JS_INNER_FAIL);
    obj.release();
    return thisVar;
}

void CloudEnhancementNapi::Destructor(napi_env env, void* nativeObject, void* finalizeHint)
{
    auto* cloudEnhancement = reinterpret_cast<CloudEnhancementNapi*>(nativeObject);
    if (cloudEnhancement == nullptr) {
        return;
    }
    delete cloudEnhancement;
    cloudEnhancement = nullptr;
}

static bool CheckWhetherInitSuccess(napi_env env, napi_value value, bool checkIsValid)
{
    napi_value propertyNames;
    uint32_t propertyLength;
    napi_valuetype valueType = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, value, &valueType), false);
    if (valueType != napi_object) {
        return false;
    }

    NAPI_CALL_BASE(env, napi_get_property_names(env, value, &propertyNames), false);
    NAPI_CALL_BASE(env, napi_get_array_length(env, propertyNames, &propertyLength), false);
    if (propertyLength == 0) {
        return false;
    }
    if (checkIsValid && (!UserFileClient::IsValid())) {
        NAPI_ERR_LOG("UserFileClient is not valid");
        return false;
    }
    return true;
}

napi_value CloudEnhancementNapi::JSGetCloudEnhancementInstance(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetCloudEnhancementInstance");

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
        NAPI_ERR_LOG("Init Cloud Enhancement Instance is failed");
        NAPI_CALL(env, napi_get_undefined(env, &result));
    }
    return result;
}

bool CloudEnhancementNapi::InitUserFileClient(napi_env env, napi_callback_info info)
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

napi_status CloudEnhancementNapi::ParseArgGetPhotoAsset(napi_env env, napi_value arg, int &fileId, std::string &uri,
    std::string &displayName)
{
    if (arg == nullptr) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "ParseArgGetPhotoAsset failed to get photoAsset");
        return napi_invalid_arg;
    }
    FileAssetNapi *obj = nullptr;
    napi_unwrap(env, arg, reinterpret_cast<void**>(&obj));
    if (obj == nullptr) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "failed to get asset napi object");
        return napi_invalid_arg;
    }
    fileId = obj->GetFileId();
    uri = obj->GetFileUri();
    displayName = obj->GetFileDisplayName();
    return napi_ok;
}

static napi_value ParseArgsSubmitCloudEnhancementTasks(napi_env env, napi_callback_info info,
    unique_ptr<CloudEnhancementAsyncContext>& context)
{
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextGetArgs(env, info, context, ARGS_TWO, ARGS_THREE) == napi_ok,
        "Failed to get args");
    CHECK_COND(env, CloudEnhancementNapi::InitUserFileClient(env, info), JS_INNER_FAIL);

    napi_valuetype valueType = napi_undefined;
    CHECK_ARGS(env, napi_typeof(env, context->argv[PARAM1], &valueType), JS_INNER_FAIL);
    CHECK_COND(env, valueType == napi_boolean, JS_INNER_FAIL);

    bool hasCloudWatermark = false;
    if (napi_get_value_bool(env, context->argv[PARAM1], &hasCloudWatermark) != napi_ok) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }

    int32_t triggerMode = 0;
    if (context->argc == ARGS_THREE) {
        CHECK_ARGS(env, napi_typeof(env, context->argv[PARAM2], &valueType), JS_INNER_FAIL);
        CHECK_COND(env, valueType == napi_number, JS_INNER_FAIL);
        if (napi_get_value_int32(env, context->argv[PARAM2], &triggerMode) != napi_ok) {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
            return nullptr;
        }
    }

    vector<string> uris;
    vector<napi_value> napiValues;
    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetNapiValueArray(env, context->argv[PARAM0], napiValues));
    CHECK_COND_WITH_MESSAGE(env, !napiValues.empty(), "array is empty");
    CHECK_ARGS(env, napi_typeof(env, napiValues.front(), &valueType), JS_INNER_FAIL);
    if (valueType == napi_object) { // array of asset object
        CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetUriArrayFromAssets(env, napiValues, uris));
    } else {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Invalid type");
        return nullptr;
    }

    CHECK_COND_WITH_MESSAGE(env, !uris.empty(), "Failed to check empty array");
    for (const auto& uri : uris) {
        CHECK_COND_WITH_MSG(env, uri.find(PhotoColumn::PHOTO_URI_PREFIX) != string::npos, JS_E_URI,
            "cloud enhancement task uri mush start with " + PhotoColumn::PHOTO_URI_PREFIX);
    }
    
    context->hasCloudWatermark_ = hasCloudWatermark;
    context->triggerMode_ = triggerMode;
    context->predicates.In(PhotoColumn::MEDIA_ID, uris);
    context->uris.assign(uris.begin(), uris.end());
    RETURN_NAPI_TRUE(env);
}

static void SubmitCloudEnhancementTasksExecute(napi_env env, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("SubmitCloudEnhancementTasksExecute");

    auto* context = static_cast<CloudEnhancementAsyncContext*>(data);
    CloudEnhancementReqBody reqBody;
    reqBody.hasCloudWatermark = context->hasCloudWatermark_;
    reqBody.triggerMode = context->triggerMode_;
    reqBody.fileUris = context->uris;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::SUBMIT_CLOUD_ENHANCEMENT_TASKS);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret < 0) {
        context->SaveError(ret);
        NAPI_ERR_LOG("Submit cloud enhancement tasks failed, err: %{public}d", ret);
        return;
    }
    NAPI_INFO_LOG("SubmitCloudEnhancementTasksExecute Success");
}

static void SubmitCloudEnhancementTasksCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto* context = static_cast<CloudEnhancementAsyncContext*>(data);
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

napi_value CloudEnhancementNapi::JSSubmitCloudEnhancementTasks(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JSSubmitCloudEnhancementTasks");

    auto asyncContext = make_unique<CloudEnhancementAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env, ParseArgsSubmitCloudEnhancementTasks(env, info, asyncContext), "Failed to parse args");
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "SubmitCloudEnhancementTasks",
        SubmitCloudEnhancementTasksExecute, SubmitCloudEnhancementTasksCompleteCallback);
}

static napi_status ParseArgPrioritize(napi_env env, napi_callback_info info,
    unique_ptr<CloudEnhancementAsyncContext>& context)
{
    napi_value thisVar = nullptr;
    context->argc = ARGS_ONE;
    GET_JS_ARGS(env, info, context->argc, context->argv, thisVar);

    if (CloudEnhancementNapi::ParseArgGetPhotoAsset(env, context->argv[PARAM0], context->fileId, context->photoUri,
        context->displayName) != napi_ok) {
        NAPI_ERR_LOG("requestMedia ParseArgGetPhotoAsset error");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetPhotoAsset error");
        return napi_invalid_arg;
    }

    NAPI_INFO_LOG("Parse Arg: %{public}d, %{private}s, %{public}s", context->fileId, context->photoUri.c_str(),
        context->displayName.c_str());

    return napi_ok;
}

static void PrioritizeCloudEnhancementTaskExecute(napi_env env, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PrioritizeCloudEnhancementTaskExecute");

    auto* context = static_cast<CloudEnhancementAsyncContext*>(data);
    CloudEnhancementReqBody reqBody;
    reqBody.fileUris.emplace_back(context->photoUri);
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PRIORITIZE_CLOUD_ENHANCEMENT_TASK);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret < 0) {
        context->SaveError(ret);
        NAPI_ERR_LOG("Prioritize cloud enhancement task failed, err: %{public}d", ret);
        return;
    }
    NAPI_INFO_LOG("PrioritizeCloudEnhancementTaskExecute Success");
}

static void PrioritizeCloudEnhancementTaskCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto* context = static_cast<CloudEnhancementAsyncContext*>(data);
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

napi_value CloudEnhancementNapi::JSPrioritizeCloudEnhancementTask(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JSPrioritizeCloudEnhancementTask");

    auto asyncContext = make_unique<CloudEnhancementAsyncContext>();
    CHECK_COND(env, CloudEnhancementNapi::InitUserFileClient(env, info), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, ParseArgPrioritize(env, info, asyncContext) == napi_ok, "Failed to parse args");
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSPrioritizeCloudEnhancementTask",
        PrioritizeCloudEnhancementTaskExecute, PrioritizeCloudEnhancementTaskCompleteCallback);
}

static napi_value ParseArgsCancelCloudEnhancementTasks(napi_env env, napi_callback_info info,
    unique_ptr<CloudEnhancementAsyncContext>& context)
{
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextGetArgs(env, info, context, ARGS_ONE, ARGS_ONE) == napi_ok,
        "Failed to get args");
    CHECK_COND(env, CloudEnhancementNapi::InitUserFileClient(env, info), JS_INNER_FAIL);

    napi_valuetype valueType = napi_undefined;
    CHECK_ARGS(env, napi_typeof(env, context->argv[PARAM0], &valueType), JS_INNER_FAIL);
    CHECK_COND(env, valueType == napi_object, JS_INNER_FAIL);

    vector<string> uris;
    vector<napi_value> napiValues;
    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetNapiValueArray(env, context->argv[PARAM0], napiValues));
    CHECK_COND_WITH_MESSAGE(env, !napiValues.empty(), "array is empty");
    CHECK_ARGS(env, napi_typeof(env, napiValues.front(), &valueType), JS_INNER_FAIL);
    if (valueType == napi_object) { // array of asset object
        CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetUriArrayFromAssets(env, napiValues, uris));
    } else {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Invalid type");
        return nullptr;
    }

    CHECK_COND_WITH_MESSAGE(env, !uris.empty(), "Failed to check empty array");
    for (const auto& uri : uris) {
        NAPI_INFO_LOG("CloudEnhancementNapi ParseArgsCancelCloudEnhancementTasks: %{public}s", uri.c_str());
        CHECK_COND_WITH_MSG(env, uri.find(PhotoColumn::PHOTO_URI_PREFIX) != string::npos, JS_E_URI,
            "uri must start with " + PhotoColumn::PHOTO_URI_PREFIX);
    }

    context->predicates.In(PhotoColumn::MEDIA_ID, uris);
    context->uris.assign(uris.begin(), uris.end());
    RETURN_NAPI_TRUE(env);
}

static void CancelCloudEnhancementTasksExecute(napi_env env, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("CancelCloudEnhancementTasksExecute");

    auto* context = static_cast<CloudEnhancementAsyncContext*>(data);
    CloudEnhancementReqBody reqBody;
    reqBody.hasCloudWatermark = context->hasCloudWatermark_;
    reqBody.triggerMode = context->triggerMode_;
    reqBody.fileUris = context->uris;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CANCEL_CLOUD_ENHANCEMENT_TASKS);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret < 0) {
        context->SaveError(ret);
        NAPI_ERR_LOG("cancel cloud enhancement tasks failed, err: %{public}d", ret);
        return;
    }
    NAPI_INFO_LOG("CancelCloudEnhancementTasksExecute Success");
}

static void CancelCloudEnhancementTasksCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto* context = static_cast<CloudEnhancementAsyncContext*>(data);
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

napi_value CloudEnhancementNapi::JSCancelCloudEnhancementTasks(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JSCancelCloudEnhancementTasks");

    auto asyncContext = make_unique<CloudEnhancementAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env, ParseArgsCancelCloudEnhancementTasks(env, info, asyncContext), "Failed to parse args");
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "CancelCloudEnhancementTasks",
        CancelCloudEnhancementTasksExecute, CancelCloudEnhancementTasksCompleteCallback);
}

static void CancelAllCloudEnhancementTasksExecute(napi_env env, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("CancelAllCloudEnhancementTasksExecute");

    auto* context = static_cast<CloudEnhancementAsyncContext*>(data);
    CloudEnhancementReqBody reqBody;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CANCEL_ALL_CLOUD_ENHANCEMENT_TASKS);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret < 0) {
        context->SaveError(ret);
        NAPI_ERR_LOG("Cancel all cloud enhancement tasks failed, err: %{public}d", ret);
        return;
    }
    NAPI_INFO_LOG("CancelAllCloudEnhancementTasksExecute Success");
}

static void CancelAllCloudEnhancementTasksCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto* context = static_cast<CloudEnhancementAsyncContext*>(data);
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

napi_value CloudEnhancementNapi::JSCancelAllCloudEnhancementTasks(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JSCancelAllCloudEnhancementTasks");

    auto asyncContext = make_unique<CloudEnhancementAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextGetArgs(env, info, asyncContext, ARGS_ZERO, ARGS_ZERO) == napi_ok,
        "Failed to parse args");
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "CancelAllCloudEnhancementTasks",
        CancelAllCloudEnhancementTasksExecute, CancelAllCloudEnhancementTasksCompleteCallback);
}

static napi_status ParseArgQuery(napi_env env, napi_callback_info info,
    unique_ptr<CloudEnhancementAsyncContext>& context)
{
    napi_value thisVar = nullptr;
    context->argc = ARGS_ONE;
    GET_JS_ARGS(env, info, context->argc, context->argv, thisVar);

    if (CloudEnhancementNapi::ParseArgGetPhotoAsset(env, context->argv[PARAM0], context->fileId, context->photoUri,
        context->displayName) != napi_ok) {
        NAPI_ERR_LOG("requestMedia ParseArgGetPhotoAsset error");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetPhotoAsset error");
        return napi_invalid_arg;
    }

    NAPI_INFO_LOG("Parse Arg: %{public}d, %{private}s, %{public}s", context->fileId, context->photoUri.c_str(),
        context->displayName.c_str());

    return napi_ok;
}

static void FillTaskStageWithClientQuery(CloudEnhancementAsyncContext* context, string &photoId)
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    lock_guard<mutex> lock(mtx);
    InitCloudEnhancementFunc();
    if (dynamicHandler == nullptr) {
        NAPI_ERR_LOG("dynamicHandler is nullptr!");
        return;
    }
    InitEnhancementClient();
    if (clientWrapper == nullptr) {
        NAPI_ERR_LOG("clientWrapper is nullptr!");
        return;
    }
    if (!IsConnected(clientWrapper)) {
        LoadSA();
    }
    MediaEnhanceBundleHandle* bundle = CreateBundle();
    bundle = QueryTaskState(photoId);
    if (bundle == nullptr) {
        NAPI_ERR_LOG("queryTaskState result is nullptr!");
        DestroyEnhancementClient();
        return;
    }
    int32_t currentState = GetInt(bundle, MediaEnhance_Query::CURRENT_STATE);
    NAPI_INFO_LOG("clientQueryTaskStateFunc stage = %{public}d", currentState);
    if (currentState == MediaEnhance_Query::EN_EXCEPTION) {
        context->cloudEnhancementTaskStage_ = CloudEnhancementTaskStage::TASK_STAGE_EXCEPTION;
    } else if (currentState == MediaEnhance_Query::EN_PREPARING) {
        context->cloudEnhancementTaskStage_ = CloudEnhancementTaskStage::TASK_STAGE_PREPARING;
    } else if (currentState == MediaEnhance_Query::EN_UPLOADING) {
        context->cloudEnhancementTaskStage_ = CloudEnhancementTaskStage::TASK_STAGE_UPLOADING;
        context->transferredFileSize_ = GetInt(bundle, MediaEnhance_Query::UPLOAD_PROGRESS);
        context->totalFileSize_ = GetInt(bundle, MediaEnhance_Query::UPLOAD_SIZE);
    } else if (currentState == MediaEnhance_Query::EN_EXECUTING) {
        context->cloudEnhancementTaskStage_ = CloudEnhancementTaskStage::TASK_STAGE_EXECUTING;
        context->expectedDuration_ = GetInt(bundle, MediaEnhance_Query::EXECUTE_TIME);
    } else if (currentState == MediaEnhance_Query::EN_DOWNLOADING) {
        context->cloudEnhancementTaskStage_ = CloudEnhancementTaskStage::TASK_STAGE_DOWNLOADING;
        context->transferredFileSize_ = GetInt(bundle, MediaEnhance_Query::DOWNLOAD_PROGRESS);
        context->totalFileSize_ = GetInt(bundle, MediaEnhance_Query::DOWNLOAD_SIZE);
    }
    DestroyBundle(bundle);
    DestroyEnhancementClient();
#endif
}

static void QueryCloudEnhancementTaskStateExecute(napi_env env, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("QueryCloudEnhancementTaskStateExecute");

    auto* context = static_cast<CloudEnhancementAsyncContext*>(data);
    QueryCloudEnhancementTaskStateReqBody reqBody;
    QueryCloudEnhancementTaskStateRespBody respBody;
    reqBody.photoUri = context->photoUri;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_CLOUD_ENHANCEMENT_TASK_STATE);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    if (ret < 0) {
        context->SaveError(JS_INNER_FAIL);
        NAPI_ERR_LOG("Failed to query cloud enhancement task state, err: %{public}d", ret);
        return;
    }
    int32_t fileId = respBody.fileId;
    string photoId = respBody.photoId;
    int32_t ceAvailable = respBody.ceAvailable;
    int32_t ceErrorCode = respBody.ceErrorCode;
    NAPI_INFO_LOG("query fileId: %{public}d, photoId: %{private}s, ceAvailable: %{public}d",
        fileId, photoId.c_str(), ceAvailable);

    if (ceAvailable == static_cast<int32_t>(CE_AVAILABLE::NOT_SUPPORT)) {
        NAPI_ERR_LOG("photo not support cloud enhancement, fileId: %{public}d", fileId);
        return;
    }
    // 任务成功或失败，不能再查云增强，否则会让云增强误报准备中
    if (ceAvailable == static_cast<int32_t>(CE_AVAILABLE::SUCCESS)) {
        context->cloudEnhancementTaskStage_ = CloudEnhancementTaskStage::TASK_STAGE_COMPLETED;
        return;
    }
    if (ceAvailable == static_cast<int32_t>(CE_AVAILABLE::FAILED_RETRY) ||
        ceAvailable == static_cast<int32_t>(CE_AVAILABLE::FAILED)) {
        context->cloudEnhancementTaskStage_ = CloudEnhancementTaskStage::TASK_STAGE_FAILED;
        context->statusCode_ = ceErrorCode;
        NAPI_INFO_LOG("TASK_STAGE_FAILED, fileId: %{public}d, statusCode: %{public}d", fileId, ceAvailable);
        return;
    }
    FillTaskStageWithClientQuery(context, photoId);
}

static void QueryCloudEnhancementTaskStateCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto* context = static_cast<CloudEnhancementAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);
    napi_get_undefined(env, &jsContext->error);
    if (context->error == ERR_DEFAULT) {
        napi_value cloudEnhancementTaskState =
            CloudEnhancementTaskStateNapi::NewCloudEnhancementTaskStateNapi(env, context);
        jsContext->data = cloudEnhancementTaskState;
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

napi_value CloudEnhancementNapi::JSQueryCloudEnhancementTaskState(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JSQueryCloudEnhancementTaskState");

    auto asyncContext = make_unique<CloudEnhancementAsyncContext>();
    CHECK_COND(env, CloudEnhancementNapi::InitUserFileClient(env, info), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, ParseArgQuery(env, info, asyncContext) == napi_ok, "Failed to parse args");
    
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "QueryCloudEnhancementTaskState",
        QueryCloudEnhancementTaskStateExecute, QueryCloudEnhancementTaskStateCompleteCallback);
}

static void SyncCloudEnhancementTaskStatusExecute(napi_env env, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("SyncCloudEnhancementTaskStatusExecute");

    auto* context = static_cast<CloudEnhancementAsyncContext*>(data);
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::SYNC_CLOUD_ENHANCEMENT_TASK_STATUS);
    CloudEnhancementReqBody reqBody;
    int32_t changeRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (changeRows < 0) {
        context->SaveError(changeRows);
        NAPI_ERR_LOG("sync cloud enhancement failed, err: %{public}d", changeRows);
    }
    NAPI_INFO_LOG("CloudEnhancementNapi SyncCloudEnhancementTaskStatusExecute Success");
}

static void SyncCloudEnhancementTaskStatusCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto* context = static_cast<CloudEnhancementAsyncContext*>(data);
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

napi_value CloudEnhancementNapi::JSSyncCloudEnhancementTaskStatus(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JSSyncCloudEnhancementTaskStatus");

    auto asyncContext = make_unique<CloudEnhancementAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextGetArgs(env, info, asyncContext, ARGS_ZERO, ARGS_ZERO) == napi_ok,
        "Failed to parse args");
   
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "SyncCloudEnhancementTaskStatus",
        SyncCloudEnhancementTaskStatusExecute, SyncCloudEnhancementTaskStatusCompleteCallback);
}

bool CloudEnhancementAsyncContext::GetPairAsset()
{
    if (fetchFileResult->GetCount() != 1) {
        NAPI_ERR_LOG("Object number of fetchfileResult is not one");
        return false;
    }
    fileAsset = fetchFileResult->GetFirstObject();
    if (fileAsset == nullptr) {
        NAPI_ERR_LOG("Fail to get fileAsset from fetchFileResult");
        return false;
    }
    return true;
}

static void GetCloudEnhancementPairExecute(napi_env env, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetCloudEnhancementPairExecute");

    auto* context = static_cast<CloudEnhancementAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "AsyncContext is null");
    GetCloudEnhancementPairReqBody reqBody;
    GetCloudEnhancementPairRespBody respBody;
    reqBody.photoUri = context->photoUri;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::GET_CLOUD_ENHANCEMENT_PAIR);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    if (ret < 0) {
        context->SaveError(ret);
        NAPI_ERR_LOG("Failed to get cloud enhancement pair, err: %{public}d", ret);
        return;
    }
    auto resultSet = respBody.resultSet;
    context->fetchFileResult = make_unique<FetchResult<FileAsset>>(move(resultSet));
    if (!context->GetPairAsset()) {
        NAPI_ERR_LOG("Fail to getPairAsset");
        return;
    }

    NAPI_INFO_LOG("CloudEnhancementNapi GetCloudEnhancementPairExecute Success");
}

static void GetNapiPairFileAsset(napi_env env, CloudEnhancementAsyncContext *context,
    unique_ptr<JSAsyncContextOutput> &jsContext)
{
    // Create PhotiAsset object using the contents of fileAsset
    if (context->fileAsset == nullptr) {
        NAPI_ERR_LOG("No fetch file result found!");
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
            "Failed to obtain Fetch File Result");
        return;
    }
    if (context->fileAsset->GetPhotoEditTime() != 0) {
        NAPI_WARN_LOG("PhotoAsset is edited");
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
            "Failed to obtain Fetch File Result");
        return;
    }
    context->fileAsset->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    napi_value jsFileAsset = FileAssetNapi::CreateFileAsset(env, context->fileAsset);
    if (jsFileAsset == nullptr) {
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
            "Failed to create js object for Fetch File Result");
    } else {
        jsContext->data = jsFileAsset;
        jsContext->status = true;
        napi_get_undefined(env, &jsContext->error);
    }
}

static void GetCloudEnhancementPairCompleteCallback(napi_env env, napi_status status, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetCloudEnhancementPairCompleteCallback");

    CloudEnhancementAsyncContext *context = static_cast<CloudEnhancementAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);

    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, jsContext->error);
    } else {
        GetNapiPairFileAsset(env, context, jsContext);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }
    delete context;
}

napi_value CloudEnhancementNapi::JSGetCloudEnhancementPair(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetCloudEnhancementPair");
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    auto asyncContext = make_unique<CloudEnhancementAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->assetType = TYPE_PHOTO;

    CHECK_COND(env, CloudEnhancementNapi::InitUserFileClient(env, info), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, ParseArgQuery(env, info, asyncContext) == napi_ok, "Failed to parse args");
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "GetCloudEnhancementPair",
        GetCloudEnhancementPairExecute, GetCloudEnhancementPairCompleteCallback);
}
} // namespace OHOS::Media

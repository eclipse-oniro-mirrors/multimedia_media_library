/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "cloud_enhancement_ani.h"
#include "ani_class_name.h"
#include "cloud_enhancement_task_state_ani.h"
#include "cloud_enhancement_uri.h"
#include "media_library_ani.h"
#include "medialibrary_ani_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_tracer.h"
#include "media_column.h"
#include "media_file_uri.h"
#include "result_set_utils.h"
#include "userfile_client.h"
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
#include "media_enhance_constants_c_api.h"
#include "media_enhance_handles.h"
#include "media_enhance_client_c_api.h"
#include "media_enhance_bundle_c_api.h"
#endif
#include "cloud_enhancement_vo.h"
#include "medialibrary_business_code.h"
#include "user_define_ipc_client.h"
#include "get_cloud_enhancement_pair_vo.h"
#include "query_cloud_enhancement_task_state_vo.h"

using namespace std;
using namespace OHOS::DataShare;
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
using namespace OHOS::MediaEnhance;
#endif

namespace OHOS {
namespace Media {
struct SubmitCloudEnhancementParams {
    ani_object photoAssets;
    ani_boolean hasCloudWatermark;
    int triggerMode;
};

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
using ClientQueryTaskState = MediaEnhanceBundleHandle* (*)(MediaEnhanceClientHandle* client, const char* taskId);
using BundleHandleGetInt = int32_t (*)(MediaEnhanceBundleHandle* bundle, const char* key);


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
        ANI_ERR_LOG("dynamicHandler is null. error:%{public}s", dlerror());
        return;
    }

    if (createMCEClientFunc == nullptr) {
        createMCEClientFunc = (CreateMCEClient)dlsym(dynamicHandler, "CreateMediaEnhanceClient");
    }
    if (createMCEClientFunc == nullptr) {
        ANI_ERR_LOG("CreateMediaEnhanceClient dlsym failed.error:%{public}s", dlerror());
        return;
    }

    if (destroyMCEClientFunc == nullptr) {
        destroyMCEClientFunc = (DestroyMCEClient)dlsym(dynamicHandler, "DestroyMediaEnhanceClient");
    }
    if (destroyMCEClientFunc == nullptr) {
        ANI_ERR_LOG("DestroyMediaEnhanceClient dlsym failed.error:%{public}s", dlerror());
        return;
    }

    if (createMCEBundleFunc == nullptr) {
        createMCEBundleFunc = (CreateMCEBundle)dlsym(dynamicHandler, "CreateMediaEnhanceBundle");
    }
    if (createMCEBundleFunc == nullptr) {
        ANI_ERR_LOG("CreateMediaEnhanceBundle dlsym failed.error:%{public}s", dlerror());
        return;
    }

    if (destroyMCEBundleFunc == nullptr) {
        destroyMCEBundleFunc = (DestroyMCEBundle)dlsym(dynamicHandler, "DestroyMediaEnhanceBundle");
    }
    if (destroyMCEBundleFunc == nullptr) {
        ANI_ERR_LOG("DestroyMediaEnhanceBundle dlsym failed.error:%{public}s", dlerror());
        return;
    }
}

static void InitCloudEnhancementExtraFunc(void* dynamicHandler)
{
    if (dynamicHandler == nullptr) {
        ANI_ERR_LOG("dynamicHandler is null. error:%{public}s", dlerror());
        return;
    }

    if (clientLoadSaFunc == nullptr) {
        clientLoadSaFunc = (ClientLoadSA)dlsym(dynamicHandler, "MediaEnhanceClient_LoadSA");
    }
    if (clientLoadSaFunc == nullptr) {
        ANI_ERR_LOG("MediaEnhanceClient_LoadSA dlsym failed.error:%{public}s", dlerror());
        return;
    }

    if (clientIsConnectedFunc == nullptr) {
        clientIsConnectedFunc = (ClientIsConnected)dlsym(dynamicHandler, "MediaEnhanceClient_IsConnected");
    }
    if (clientIsConnectedFunc == nullptr) {
        ANI_ERR_LOG("MediaEnhanceClient_IsConnected dlsym failed.error:%{public}s", dlerror());
        return;
    }

    if (clientQueryTaskStateFunc == nullptr) {
        clientQueryTaskStateFunc = (ClientQueryTaskState)dlsym(dynamicHandler, "MediaEnhanceClient_QueryTaskState");
    }
    if (clientQueryTaskStateFunc == nullptr) {
        ANI_ERR_LOG("MediaEnhanceClient_QueryTaskState dlsym failed. error:%{public}s", dlerror());
        return;
    }

    if (bundleHandleGetIntFunc == nullptr) {
        bundleHandleGetIntFunc = (BundleHandleGetInt)dlsym(dynamicHandler, "MediaEnhanceBundle_GetInt");
    }
    if (bundleHandleGetIntFunc == nullptr) {
        ANI_ERR_LOG("MediaEnhanceBundle_GetInt dlsym failed. error:%{public}s", dlerror());
        return;
    }
}

static void InitEnhancementClient()
{
    if (createMCEClientFunc == nullptr) {
        createMCEClientFunc = (CreateMCEClient)dlsym(dynamicHandler, "CreateMediaEnhanceClient");
    }
    if (createMCEClientFunc == nullptr) {
        ANI_ERR_LOG("CreateMediaEnhanceClient dlsym failed.error:%{public}s", dlerror());
        return;
    }
    if (clientWrapper == nullptr && createMCEClientFunc != nullptr) {
        ANI_INFO_LOG("createMCEClientFunc by dlopen func.");
        clientWrapper = createMCEClientFunc(MediaEnhance_TASK_TYPE::TYPE_CAMERA);
    }
}

static void DestroyEnhancementClient()
{
    if (destroyMCEClientFunc == nullptr) {
        destroyMCEClientFunc = (DestroyMCEClient)dlsym(dynamicHandler, "DestroyMediaEnhanceClient");
    }
    if (destroyMCEClientFunc == nullptr) {
        ANI_ERR_LOG("DestroyMediaEnhanceClient dlsym failed.error:%{public}s", dlerror());
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
        ANI_ERR_LOG("createMCEBundleFunc dlsym failed.error:%{public}s", dlerror());
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
        ANI_ERR_LOG("destroyMCEBundleFunc dlsym failed.error:%{public}s", dlerror());
        return;
    }
    destroyMCEBundleFunc(bundle);
}

static int32_t LoadSA()
{
    if (clientWrapper == nullptr) {
        ANI_ERR_LOG("clientWrapper is nullptr!");
        return E_ERR;
    }
    if (clientLoadSaFunc == nullptr) {
        clientLoadSaFunc = (ClientLoadSA)dlsym(dynamicHandler, "MediaEnhanceClient_LoadSA");
    }
    if (clientLoadSaFunc == nullptr) {
        ANI_ERR_LOG("MediaEnhanceClient_LoadSA dlsym failed.error:%{public}s", dlerror());
        return E_ERR;
    }
    int32_t ret = clientLoadSaFunc(clientWrapper);
    if (ret != E_OK) {
        ANI_ERR_LOG("Enhancement Service LoadSA failed:%{public}d", ret);
    }
    return ret;
}

static bool IsConnected(MediaEnhanceClientHandle* clientWrapper)
{
    if (clientWrapper == nullptr) {
        ANI_ERR_LOG("clientWrapper is nullptr!");
        return E_ERR;
    }
    if (clientIsConnectedFunc == nullptr) {
        clientIsConnectedFunc = (ClientIsConnected)dlsym(dynamicHandler,
            "MediaEnhanceClient_IsConnected");
    }
    if (clientIsConnectedFunc == nullptr) {
        ANI_ERR_LOG("MediaEnhanceClient_IsConnected dlsym failed.error:%{public}s", dlerror());
        return false;
    }
    return clientIsConnectedFunc(clientWrapper);
}

static MediaEnhanceBundleHandle* QueryTaskState(const string &photoId)
{
    if (clientWrapper == nullptr) {
        ANI_ERR_LOG("clientWrapper is nullptr!");
        return nullptr;
    }
    if (clientQueryTaskStateFunc == nullptr) {
        clientQueryTaskStateFunc = (ClientQueryTaskState)dlsym(dynamicHandler, "MediaEnhanceClient_QueryTaskState");
    }
    if (clientQueryTaskStateFunc == nullptr) {
        ANI_ERR_LOG("MediaEnhanceClient_QueryTaskState dlsym failed. error:%{public}s", dlerror());
        return nullptr;
    }
    ANI_INFO_LOG("QueryTaskState photoId: %{public}s", photoId.c_str());
    return clientQueryTaskStateFunc(clientWrapper, photoId.c_str());
}

static int32_t GetInt(MediaEnhanceBundleHandle* bundle, const char* key)
{
    if (bundleHandleGetIntFunc == nullptr) {
        bundleHandleGetIntFunc = (BundleHandleGetInt)dlsym(dynamicHandler, "MediaEnhanceBundle_GetInt");
    }
    if (bundleHandleGetIntFunc == nullptr) {
        ANI_ERR_LOG("MediaEnhanceBundle_GetInt dlsym failed. error:%{public}s", dlerror());
        return E_ERR;
    }
    return bundleHandleGetIntFunc(bundle, key);
}

static void InitCloudEnhancementFunc()
{
    string path = "/system/lib64/platformsdk/libmedia_cloud_enhance_plugin.z.so";
    dynamicHandler = dlopen(path.c_str(), RTLD_NOW);
    InitCloudEnhancementBasicFunc(dynamicHandler);
    InitCloudEnhancementExtraFunc(dynamicHandler);
}
#endif

ani_status CloudEnhancementAni::Init(ani_env *env)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    static const char *className = PAH_ANI_CLASS_CLOUD_ENHANCEMENT.c_str();
    ani_class cls;
    ani_status status = env->FindClass(className, &cls);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to find class: %{public}s", className);
        return status;
    }

    std::array methods = {
        ani_native_function {"submitCloudEnhancementTasksSync", nullptr,
            reinterpret_cast<void *>(CloudEnhancementAni::SubmitCloudEnhancementTasks)},
        ani_native_function {"prioritizeCloudEnhancementTaskSync", nullptr,
            reinterpret_cast<void *>(CloudEnhancementAni::PrioritizeCloudEnhancementTask)},
        ani_native_function {"cancelCloudEnhancementTasksSync", nullptr,
            reinterpret_cast<void *>(CloudEnhancementAni::CancelCloudEnhancementTasks)},
        ani_native_function {"cancelAllCloudEnhancementTasksSync", nullptr,
            reinterpret_cast<void *>(CloudEnhancementAni::CancelAllCloudEnhancementTasks)},
        ani_native_function {"queryCloudEnhancementTaskStateSync", nullptr,
            reinterpret_cast<void *>(CloudEnhancementAni::QueryCloudEnhancementTaskState)},
        ani_native_function {"syncCloudEnhancementTaskStatusSync", nullptr,
            reinterpret_cast<void *>(CloudEnhancementAni::SyncCloudEnhancementTaskStatus)},
        ani_native_function {"getCloudEnhancementPairSync", nullptr,
            reinterpret_cast<void *>(CloudEnhancementAni::GetCloudEnhancementPair)},
    };
    status = env->Class_BindNativeMethods(cls, methods.data(), methods.size());
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to bind native methods to: %{public}s", className);
        return status;
    }

    std::array staticMethods = {
        ani_native_function {"getCloudEnhancementInstance", nullptr,
            reinterpret_cast<void *>(CloudEnhancementAni::Constructor)},
    };
    status = env->Class_BindStaticNativeMethods(cls, staticMethods.data(), staticMethods.size());
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to bind static native methods to: %{public}s", className);
        return status;
    }

    return ANI_OK;
}

ani_object CloudEnhancementAni::Constructor(ani_env *env, ani_class clazz, ani_object context)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL,
            "The cloud enhancement instance can be called only by system apps");
        return nullptr;
    }

    std::unique_ptr<CloudEnhancementAni> nativeHandle = std::make_unique<CloudEnhancementAni>();
    CHECK_COND_RET(nativeHandle != nullptr, nullptr, "nativeHandle is nullptr");
    ani_method ctor;
    if (ANI_OK != env->Class_FindMethod(clazz, "<ctor>", nullptr, &ctor)) {
        ANI_ERR_LOG("Failed to find method: %{public}s", "ctor");
        return nullptr;
    }

    ani_object aniObject;
    if (ANI_OK != env->Object_New(clazz, ctor, &aniObject, reinterpret_cast<ani_long>(nativeHandle.get()))) {
        ANI_ERR_LOG("New MediaAssetChangeRequest Fail");
        return nullptr;
    }
    (void)nativeHandle.release();
    return aniObject;
}

CloudEnhancementAni* CloudEnhancementAni::Unwrap(ani_env *env, ani_object aniObject)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    ani_long context;
    if (ANI_OK != env->Object_GetFieldByName_Long(aniObject, "nativeHandle", &context)) {
        return nullptr;
    }
    return reinterpret_cast<CloudEnhancementAni*>(context);
}

bool CloudEnhancementAni::InitUserFileClient(ani_env *env, ani_object aniObject)
{
    if (UserFileClient::IsValid()) {
        return true;
    }

    std::unique_lock<std::mutex> helperLock(MediaLibraryAni::sUserFileClientMutex_);
    if (!UserFileClient::IsValid()) {
        UserFileClient::Init(env, aniObject);
    }
    helperLock.unlock();
    return UserFileClient::IsValid();
}

static ani_object ParseArgsSubmitCloudEnhancementTasks(ani_env *env, ani_object aniObject,
    const SubmitCloudEnhancementParams &params, unique_ptr<CloudEnhancementAniContext> &context)
{
    CHECK_COND(env, CloudEnhancementAni::InitUserFileClient(env, aniObject), JS_INNER_FAIL);
    bool hasCloudWatermarkBool = false;
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryAniUtils::GetBool(
        env, params.hasCloudWatermark, hasCloudWatermarkBool) == ANI_OK, "Failed to get hasCloudWatermark");
    vector<string> uris;
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryAniUtils::GetUriArrayFromAssets(env, params.photoAssets, uris) == ANI_OK,
        "Failed to get uris");
    CHECK_COND_WITH_MESSAGE(env, !uris.empty(), "Failed to check empty array");
    for (const auto& uri : uris) {
        CHECK_COND(env, uri.find(PhotoColumn::PHOTO_URI_PREFIX) != string::npos, JS_E_URI);
    }
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    context->hasCloudWatermark_ = hasCloudWatermarkBool;
    context->triggerMode_ = params.triggerMode;
    context->predicates.In(PhotoColumn::MEDIA_ID, uris);
    context->uris.assign(uris.begin(), uris.end());
    return reinterpret_cast<ani_object>(true);
}

static void CommonComplete(ani_env *env, std::unique_ptr<CloudEnhancementAniContext> &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("CommonComplete");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");

    ani_object errorObj = {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    }

    tracer.Finish();
    context.reset();
}

static void SubmitCloudEnhancementTasksExecute(std::unique_ptr<CloudEnhancementAniContext> &aniContext)
{
    MediaLibraryTracer tracer;
    tracer.Start("SubmitCloudEnhancementTasksExecute");
    CHECK_NULL_PTR_RETURN_VOID(aniContext, "aniContext is nullptr");

    CloudEnhancementReqBody reqBody;
    reqBody.hasCloudWatermark = aniContext->hasCloudWatermark_;
    reqBody.triggerMode = aniContext->triggerMode_;
    reqBody.fileUris = aniContext->uris;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::SUBMIT_CLOUD_ENHANCEMENT_TASKS);
    int32_t changeRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (changeRows < 0) {
        aniContext->SaveError(changeRows);
        ANI_ERR_LOG("Submit cloud enhancement tasks failed, err: %{public}d", changeRows);
        return;
    }
    ANI_INFO_LOG("SubmitCloudEnhancementTasksExecute Success");
}

void CloudEnhancementAni::SubmitCloudEnhancementTasks(ani_env *env, ani_object aniObject,
    ani_object photoAssets, ani_boolean hasCloudWatermark, ani_object triggerMode)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is nullptr");
    ani_ref ref = static_cast<ani_ref>(triggerMode);
    ani_boolean isUndefined;
    env->Reference_IsUndefined(ref, &isUndefined);
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return;
    }
    MediaLibraryTracer tracer;
    tracer.Start("SubmitCloudEnhancementTasks");

    int32_t triMode = 0;
    if (!isUndefined) {
        CHECK_ARGS_RET_VOID(env, MediaLibraryAniUtils::GetInt32(env, triggerMode, triMode), OHOS_INVALID_PARAM_CODE);
    }
    auto aniContext = make_unique<CloudEnhancementAniContext>();
    SubmitCloudEnhancementParams params;
    params.photoAssets = photoAssets;
    params.hasCloudWatermark = hasCloudWatermark;
    params.triggerMode = triMode;
    CHECK_NULL_PTR_RETURN_VOID(ParseArgsSubmitCloudEnhancementTasks(
        env, aniObject, params, aniContext), "Failed to parse args");

    SubmitCloudEnhancementTasksExecute(aniContext);
    CommonComplete(env, aniContext);
}

static bool ParseArgPrioritize(ani_env *env, ani_object aniObject, ani_object photoAsset,
    unique_ptr<CloudEnhancementAniContext> &context)
{
    CHECK_COND_RET(env != nullptr, false, "env is nullptr");
    CHECK_COND_RET(context != nullptr, false, "context is nullptr");
    if (CloudEnhancementAni::ParseArgGetPhotoAsset(env, photoAsset, context->fileId, context->photoUri,
        context->displayName) != ANI_OK) {
        ANI_ERR_LOG("requestMedia ParseArgGetPhotoAsset error");
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetPhotoAsset error");
        return false;
    }

    ANI_INFO_LOG("Parse Arg: %{public}d, %{private}s, %{public}s", context->fileId, context->photoUri.c_str(),
        context->displayName.c_str());
    return true;
}

static void PrioritizeCloudEnhancementTaskExecute(std::unique_ptr<CloudEnhancementAniContext> &aniContext)
{
    MediaLibraryTracer tracer;
    tracer.Start("PrioritizeCloudEnhancementTaskExecute");
    CHECK_NULL_PTR_RETURN_VOID(aniContext, "aniContext is nullptr");
    CloudEnhancementReqBody reqBody;
    reqBody.fileUris.emplace_back(aniContext->photoUri);
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PRIORITIZE_CLOUD_ENHANCEMENT_TASK);
    int32_t changedRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (changedRows < 0) {
        aniContext->SaveError(changedRows);
        ANI_ERR_LOG("Prioritize cloud enhancement task failed, err: %{public}d", changedRows);
        return;
    }
    ANI_INFO_LOG("PrioritizeCloudEnhancementTaskExecute Success");
}

void CloudEnhancementAni::PrioritizeCloudEnhancementTask(ani_env *env, ani_object aniObject,
    ani_object photoAsset)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is nullptr");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return;
    }
    MediaLibraryTracer tracer;
    tracer.Start("PrioritizeCloudEnhancementTask");

    auto aniContext = make_unique<CloudEnhancementAniContext>();
    if (!CloudEnhancementAni::InitUserFileClient(env, aniObject)) {
        AniError::ThrowError(env, JS_INNER_FAIL, "InitUserFileClient failed");
        return;
    }
    CHECK_IF_EQUAL(ParseArgPrioritize(env, aniObject, photoAsset, aniContext) == true,
        "Failed to parse args");
    PrioritizeCloudEnhancementTaskExecute(aniContext);
    CommonComplete(env, aniContext);
}

bool CloudEnhancementAniContext::GetPairAsset()
{
    CHECK_COND_RET(fetchFileResult != nullptr, false, "fetchFileResult is nullptr");
    if (fetchFileResult->GetCount() != 1) {
        ANI_ERR_LOG("Object number of fetchfileResult is not one");
        return false;
    }
    fileAsset = fetchFileResult->GetFirstObject();
    if (fileAsset == nullptr) {
        ANI_ERR_LOG("Fail to get fileAsset from fetchFileResult");
        return false;
    }
    return true;
}

ani_status CloudEnhancementAni::ParseArgGetPhotoAsset(ani_env *env, ani_object photoAsset, int &fileId,
    std::string &uri, std::string &displayName)
{
    if (photoAsset == nullptr) {
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "ParseArgGetPhotoAsset failed to get photoAsset");
        return ANI_ERROR;
    }
    FileAssetAni *obj = FileAssetAni::Unwrap(env, photoAsset);
    if (obj == nullptr) {
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "failed to get asset napi object");
        return ANI_ERROR;
    }
    fileId = obj->GetFileId();
    uri = obj->GetFileUri();
    displayName = obj->GetFileDisplayName();
    return ANI_OK;
}

static ani_object ParseArgsCancelCloudEnhancementTasks(ani_env *env, ani_object aniObject, ani_object photoAssets,
    unique_ptr<CloudEnhancementAniContext> &context)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    CHECK_COND(env, CloudEnhancementAni::InitUserFileClient(env, aniObject), JS_INNER_FAIL);

    std::vector<std::string> uris;
    CHECK_COND_WITH_MESSAGE(env, (MediaLibraryAniUtils::GetUriArrayFromAssets(env, photoAssets, uris) == ANI_OK),
        "Failed to get uris");
    CHECK_COND_WITH_MESSAGE(env, !uris.empty(), "Failed to check empty array");
    for (const auto& uri : uris) {
        ANI_INFO_LOG("CloudEnhancementAni ParseArgsCancelCloudEnhancementTasks: %{public}s", uri.c_str());
        CHECK_COND(env, uri.find(PhotoColumn::PHOTO_URI_PREFIX) != string::npos, JS_E_URI);
    }

    context->predicates.In(PhotoColumn::MEDIA_ID, uris);
    context->uris.assign(uris.begin(), uris.end());
    ani_object result {};
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryAniUtils::ToAniBooleanObject(env, true, result) == ANI_OK,
        "Failed to convert result to ani_object");
    return result;
}

static void CancelCloudEnhancementTasksExecute(std::unique_ptr<CloudEnhancementAniContext> &aniContext)
{
    MediaLibraryTracer tracer;
    tracer.Start("CancelCloudEnhancementTasksExecute");
    CHECK_NULL_PTR_RETURN_VOID(aniContext, "aniContext is nullptr");

    CloudEnhancementReqBody reqBody;
    reqBody.hasCloudWatermark = aniContext->hasCloudWatermark_;
    reqBody.triggerMode = aniContext->triggerMode_;
    reqBody.fileUris = aniContext->uris;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CANCEL_CLOUD_ENHANCEMENT_TASKS);
    int32_t changeRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (changeRows < 0) {
        aniContext->SaveError(changeRows);
        ANI_ERR_LOG("Cancel cloud enhancement tasks failed, err: %{public}d", changeRows);
        return;
    }
    ANI_INFO_LOG("CancelCloudEnhancementTasksExecute Success");
}

void CloudEnhancementAni::CancelCloudEnhancementTasks(ani_env *env, ani_object aniObject,
    ani_object photoAssets)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JSCancelCloudEnhancementTasks");

    auto aniContext = make_unique<CloudEnhancementAniContext>();
    CHECK_NULL_PTR_RETURN_VOID(ParseArgsCancelCloudEnhancementTasks(env,
        aniObject, photoAssets, aniContext), "Failed to parse args");

    CancelCloudEnhancementTasksExecute(aniContext);
    CommonComplete(env, aniContext);
}

static void CancelAllCloudEnhancementTasksExecute(std::unique_ptr<CloudEnhancementAniContext> &aniContext)
{
    MediaLibraryTracer tracer;
    tracer.Start("CancelCloudEnhancementTasksExecute");
    CHECK_NULL_PTR_RETURN_VOID(aniContext, "aniContext is nullptr");

    CloudEnhancementReqBody reqBody;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CANCEL_ALL_CLOUD_ENHANCEMENT_TASKS);
    int32_t changeRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (changeRows < 0) {
        aniContext->SaveError(changeRows);
        ANI_ERR_LOG("Cancel all cloud enhancement tasks failed, err: %{public}d", changeRows);
        return;
    }
    ANI_INFO_LOG("CancelAllCloudEnhancementTasksExecute Success");
}

void CloudEnhancementAni::CancelAllCloudEnhancementTasks(ani_env *env, ani_object aniObject)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JSCancelAllCloudEnhancementTasks");

    auto aniContext = make_unique<CloudEnhancementAniContext>();

    CancelAllCloudEnhancementTasksExecute(aniContext);
    CommonComplete(env, aniContext);
}

static bool ParseArgQuery(ani_env *env, ani_object aniObject, ani_object photoAsset,
    unique_ptr<CloudEnhancementAniContext> &context)
{
    CHECK_COND_RET(env != nullptr, false, "env is nullptr");
    CHECK_COND_RET(context != nullptr, false, "context is nullptr");
    if (CloudEnhancementAni::ParseArgGetPhotoAsset(env, photoAsset, context->fileId, context->photoUri,
        context->displayName) != ANI_OK) {
        ANI_ERR_LOG("requestMedia ParseArgGetPhotoAsset error");
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetPhotoAsset error");
        return false;
    }

    ANI_INFO_LOG("Parse Arg: %{public}d, %{private}s, %{public}s", context->fileId, context->photoUri.c_str(),
        context->displayName.c_str());
    return true;
}

static void FillTaskStageWithClientQuery(CloudEnhancementAniContext* context, string &photoId)
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    lock_guard<mutex> lock(mtx);
    InitCloudEnhancementFunc();
    if (dynamicHandler == nullptr) {
        ANI_ERR_LOG("dynamicHandler is nullptr!");
        return;
    }
    InitEnhancementClient();
    if (clientWrapper == nullptr) {
        ANI_ERR_LOG("clientWrapper is nullptr!");
        return;
    }
    if (!IsConnected(clientWrapper)) {
        LoadSA();
    }
    MediaEnhanceBundleHandle* bundle = CreateBundle();
    bundle = QueryTaskState(photoId);
    if (bundle == nullptr) {
        ANI_ERR_LOG("queryTaskState result is nullptr!");
        DestroyEnhancementClient();
        return;
    }
    int32_t currentState = GetInt(bundle, MediaEnhance_Query::CURRENT_STATE);
    ANI_INFO_LOG("clientQueryTaskStateFunc stage = %{public}d", currentState);
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

static void QueryCloudEnhancementTaskStateExecute(std::unique_ptr<CloudEnhancementAniContext> &aniContext)
{
    MediaLibraryTracer tracer;
    tracer.Start("QueryCloudEnhancementTaskStateExecute");
    CHECK_NULL_PTR_RETURN_VOID(aniContext, "aniContext is nullptr");

    QueryCloudEnhancementTaskStateReqBody reqBody;
    QueryCloudEnhancementTaskStateRespBody respBody;
    reqBody.photoUri = aniContext->photoUri;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_CLOUD_ENHANCEMENT_TASK_STATE);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    if (ret < 0) {
        aniContext->SaveError(JS_INNER_FAIL);
        ANI_ERR_LOG("Failed to query cloud enhancement task state, err: %{public}d", ret);
        return;
    }
    int32_t fileId = respBody.fileId;
    string photoId = respBody.photoId;
    int32_t ceAvailable = respBody.ceAvailable;
    int32_t ceErrorCode = respBody.ceErrorCode;

    ANI_INFO_LOG("query fileId: %{public}d, photoId: %{private}s, ceAvailable: %{public}d",
        fileId, photoId.c_str(), ceAvailable);

    if (ceAvailable == static_cast<int32_t>(CE_AVAILABLE::NOT_SUPPORT)) {
        ANI_ERR_LOG("photo not support cloud enhancement, fileId: %{public}d", fileId);
        return;
    }
    if (ceAvailable == static_cast<int32_t>(CE_AVAILABLE::SUCCESS)) {
        aniContext->cloudEnhancementTaskStage_ = CloudEnhancementTaskStage::TASK_STAGE_COMPLETED;
        return;
    }
    if (ceAvailable == static_cast<int32_t>(CE_AVAILABLE::FAILED_RETRY) ||
        ceAvailable == static_cast<int32_t>(CE_AVAILABLE::FAILED)) {
        aniContext->cloudEnhancementTaskStage_ = CloudEnhancementTaskStage::TASK_STAGE_FAILED;
        aniContext->statusCode_ = ceErrorCode;
        ANI_INFO_LOG("TASK_STAGE_FAILED, fileId: %{public}d, statusCode: %{public}d", fileId, ceAvailable);
        return;
    }
    FillTaskStageWithClientQuery(aniContext.release(), photoId);
}

static ani_object QueryCloudEnhancementTaskStateComplete(ani_env *env, unique_ptr<CloudEnhancementAniContext> &context)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    ani_object taskStateObj {};
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        taskStateObj = CloudEnhancementTaskStateAni::NewCloudEnhancementTaskStateAni(env, context);
    } else {
        context->HandleError(env, errorObj);
    }
    context.reset();
    return taskStateObj;
}

ani_object CloudEnhancementAni::QueryCloudEnhancementTaskState(ani_env *env, ani_object aniObject,
    ani_object photoAsset)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JSQueryCloudEnhancementTaskState");

    auto aniContext = make_unique<CloudEnhancementAniContext>();
    CHECK_COND(env, CloudEnhancementAni::InitUserFileClient(env, aniObject), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, ParseArgQuery(
        env, aniObject, photoAsset, aniContext), "Failed to parse args");

    QueryCloudEnhancementTaskStateExecute(aniContext);
    return QueryCloudEnhancementTaskStateComplete(env, aniContext);
}

static void SyncCloudEnhancementTaskStatusExecute(std::unique_ptr<CloudEnhancementAniContext> &aniContext)
{
    MediaLibraryTracer tracer;
    tracer.Start("SyncCloudEnhancementTaskStatusExecute");
    CHECK_NULL_PTR_RETURN_VOID(aniContext, "aniContext is nullptr");

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::SYNC_CLOUD_ENHANCEMENT_TASK_STATUS);
    CloudEnhancementReqBody reqBody;
    int32_t changeRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (changeRows < 0) {
        aniContext->SaveError(changeRows);
        ANI_ERR_LOG("Sync Cloud Enhancement Task Status Execute, err: %{public}d", changeRows);
        return;
    }
    ANI_INFO_LOG("SyncCloudEnhancementTaskStatusExecute Success");
}

void CloudEnhancementAni::SyncCloudEnhancementTaskStatus(ani_env *env, ani_object aniObject)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is nullptr");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return;
    }
    MediaLibraryTracer tracer;
    tracer.Start("SyncCloudEnhancementTaskStatus");

    auto aniContext = make_unique<CloudEnhancementAniContext>();
    SyncCloudEnhancementTaskStatusExecute(aniContext);
    CommonComplete(env, aniContext);
}

static ani_object GetAniPairFileAsset(ani_env *env, std::unique_ptr<CloudEnhancementAniContext> &context)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    ani_object errorObj {};
    ani_object pairRes = nullptr;
    // Create PhotiAsset object using the contents of fileAsset
    if (context->fileAsset == nullptr) {
        ANI_ERR_LOG("No fetch file result found!");
        MediaLibraryAniUtils::CreateAniErrorObject(env, errorObj, ERR_INVALID_OUTPUT,
            "Failed to obtain Fetch File Result");
        env->ThrowError(static_cast<ani_error>(errorObj));
        return pairRes;
    }
    if (context->fileAsset->GetPhotoEditTime() != 0) {
        ANI_ERR_LOG("PhotoAsset is edited");
        MediaLibraryAniUtils::CreateAniErrorObject(env, errorObj, ERR_INVALID_OUTPUT,
            "Failed to obtain Fetch File Result");
        env->ThrowError(static_cast<ani_error>(errorObj));
        return pairRes;
    }
    context->fileAsset->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    FileAssetAniMethod fileAssetAniMethod;
    if (ANI_OK != FileAssetAni::InitFileAssetAniMethod(env, ResultNapiType::TYPE_PHOTOACCESS_HELPER,
        fileAssetAniMethod)) {
        ANI_ERR_LOG("InitFileAssetAniMethod failed");
        return nullptr;
    }
    pairRes = FileAssetAni::Wrap(env, FileAssetAni::CreateFileAsset(env, context->fileAsset), fileAssetAniMethod);
    if (pairRes == nullptr) {
        MediaLibraryAniUtils::CreateAniErrorObject(env, errorObj, ERR_INVALID_OUTPUT,
            "Failed to create js object for Fetch File Result");
        env->ThrowError(static_cast<ani_error>(errorObj));
    }
    return pairRes;
}

static ani_object GetCloudEnhancementPairComplete(ani_env *env, std::unique_ptr<CloudEnhancementAniContext> &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetCloudEnhancementPairComplete");
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");

    ani_object pairRes {};
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    } else {
        pairRes = GetAniPairFileAsset(env, context);
    }

    tracer.Finish();
    context.reset();
    return pairRes;
}

static void GetCloudEnhancementPairExecute(ani_env *env, std::unique_ptr<CloudEnhancementAniContext> &aniContext)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetCloudEnhancementPairExecute");
    CHECK_NULL_PTR_RETURN_VOID(aniContext, "aniContext is nullptr");

    GetCloudEnhancementPairReqBody reqBody;
    GetCloudEnhancementPairRespBody respBody;
    reqBody.photoUri = aniContext->photoUri;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::GET_CLOUD_ENHANCEMENT_PAIR);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    if (ret < 0) {
        aniContext->SaveError(ret);
        ANI_ERR_LOG("Failed to get cloud enhancement pair, err: %{public}d", ret);
        return;
    }
    auto resultSet = respBody.resultSet;
    if (resultSet == nullptr || resultSet->GoToNextRow() != E_OK) {
        ANI_ERR_LOG("Resultset is nullptr, errCode is %{public}d", ret);
        aniContext->SaveError(JS_INNER_FAIL);
        return;
    }
    aniContext->fetchFileResult = make_unique<FetchResult<FileAsset>>(move(resultSet));
    if (!aniContext->GetPairAsset()) {
        ANI_ERR_LOG("Fail to getPairAsset");
        return;
    }

    ANI_INFO_LOG("GetCloudEnhancementPairExecute Success");
}

ani_object CloudEnhancementAni::GetCloudEnhancementPair(ani_env *env, ani_object aniObject, ani_object asset)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetCloudEnhancementPair");
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    auto aniContext = make_unique<CloudEnhancementAniContext>();
    CHECK_COND_RET(aniContext != nullptr, nullptr, "aniContext is nullptr");
    aniContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    aniContext->assetType = TYPE_PHOTO;

    CHECK_COND(env, CloudEnhancementAni::InitUserFileClient(env, aniObject), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, ParseArgQuery(env, aniObject, asset, aniContext) == true, "Failed to parse args");
    GetCloudEnhancementPairExecute(env, aniContext);
    return GetCloudEnhancementPairComplete(env, aniContext);
}
} // namespace Media
} // namespace OHOS

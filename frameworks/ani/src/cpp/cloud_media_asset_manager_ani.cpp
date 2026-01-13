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

#include "cloud_media_asset_manager_ani.h"

#include <sstream>
#include "ani_class_name.h"
#include "cloud_media_asset_status_ani.h"
#include "cloud_media_asset_uri.h"
#include "media_column.h"
#include "media_library_ani.h"
#include "medialibrary_ani_utils.h"
#include "medialibrary_tracer.h"
#include "userfile_client.h"
#include "start_download_cloud_media_vo.h"
#include "retain_cloud_media_asset_vo.h"
#include "medialibrary_business_code.h"
#include "user_define_ipc_client.h"
#include "medialibrary_business_code.h"
#include "get_cloudmedia_asset_status_vo.h"
#include "user_define_ipc_client.h"

namespace OHOS::Media {
const size_t TYPE_SIZE = 6;
const int32_t INDEX_ZERO = 0;
const int32_t INDEX_ONE = 1;
const int32_t INDEX_TWO = 2;
const int32_t INDEX_THREE = 3;
const int32_t INDEX_FOUR = 4;
const int32_t INDEX_FIVE = 5;

ani_status CloudMediaAssetManagerAni::Init(ani_env *env)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    static const char *className = PAH_ANI_CLASS_CLOUD_MEDIA_ASSET_MANAGER.c_str();
    ani_class cls;
    ani_status status = env->FindClass(className, &cls);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to find class: %{public}s", className);
        return status;
    }

    std::array methods = {
        ani_native_function {"startDownloadCloudMediaInner", nullptr,
            reinterpret_cast<void *>(CloudMediaAssetManagerAni::StartDownloadCloudMedia)},
        ani_native_function {"pauseDownloadCloudMediaInner", nullptr,
            reinterpret_cast<void *>(CloudMediaAssetManagerAni::PauseDownloadCloudMedia)},
        ani_native_function {"cancelDownloadCloudMediaInner", nullptr,
            reinterpret_cast<void *>(CloudMediaAssetManagerAni::CancelDownloadCloudMedia)},
        ani_native_function {"retainCloudMediaAssetInner", nullptr,
            reinterpret_cast<void *>(CloudMediaAssetManagerAni::RetainCloudMediaAsset)},
        ani_native_function {"getCloudMediaAssetStatusInner", nullptr,
            reinterpret_cast<void *>(CloudMediaAssetManagerAni::GetCloudMediaAssetStatus)},
    };
    status = env->Class_BindNativeMethods(cls, methods.data(), methods.size());
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to bind native methods to: %{public}s", className);
        return status;
    }

    std::array staticMethods = {
        ani_native_function {"getCloudMediaAssetManagerInstance", nullptr,
            reinterpret_cast<void *>(CloudMediaAssetManagerAni::Constructor)},
    };
    status = env->Class_BindStaticNativeMethods(cls, staticMethods.data(), staticMethods.size());
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to bind static native methods to: %{public}s", className);
        return status;
    }
    return ANI_OK;
}

ani_object CloudMediaAssetManagerAni::Constructor(ani_env *env, ani_class clazz, ani_object aniObject)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL,
            "The cloud CloudMediaAssetManager instance can be called only by system apps");
        return nullptr;
    }

    std::unique_ptr<CloudMediaAssetManagerAni> nativeHandle = std::make_unique<CloudMediaAssetManagerAni>();
    CHECK_COND_RET(nativeHandle != nullptr, nullptr, "nativeHandle is nullptr");
    ani_method ctor;
    if (ANI_OK != env->Class_FindMethod(clazz, "<ctor>", nullptr, &ctor)) {
        ANI_ERR_LOG("Failed to find method: %{public}s", "ctor");
        return nullptr;
    }
    if (!InitUserFileClient(env, aniObject)) {
        ANI_ERR_LOG("Failed to init UserFileClient");
        return nullptr;
    }

    ani_object aniObject_out;
    if (ANI_OK != env->Object_New(clazz, ctor, &aniObject_out, reinterpret_cast<ani_long>(nativeHandle.get()))) {
        ANI_ERR_LOG("New MediaAssetChangeRequest Fail");
        return nullptr;
    }
    (void)nativeHandle.release();
    return aniObject_out;
}


bool CloudMediaAssetManagerAni::InitUserFileClient(ani_env *env, ani_object aniObject)
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


static void StartDownloadCloudMediaExecute(ani_env *env, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("StartDownloadCloudMediaExecute");
    ANI_INFO_LOG("enter StartDownloadCloudMediaExecute");

    auto* context = static_cast<CloudMediaAssetAsyncAniContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    StartDownloadCloudMediaReqBody reqBody;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::START_DOWNLOAD_CLOUDMEDIA);
    reqBody.cloudMediaType = context->cloudMediaDownloadType;
    ANI_INFO_LOG("before IPC::UserDefineIPCClient().Call");
    int32_t changedRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    ANI_INFO_LOG("after IPC::UserDefineIPCClient().Call");
    if (changedRows < 0) {
        context->SaveError(changedRows);
        ANI_ERR_LOG("Start download cloud media failed, err: %{public}d", changedRows);
    }
}

static void CommonComplete(ani_env *env, std::unique_ptr<CloudMediaAssetAsyncAniContext> &context)
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

ani_object CloudMediaAssetManagerAni::StartDownloadCloudMedia(ani_env *env, ani_object aniObject,
    ani_enum_item downloadType)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("StartDownloadCloudMedia");

    auto asyncContext = make_unique<CloudMediaAssetAsyncAniContext>();
    CHECK_COND_RET(asyncContext != nullptr, nullptr, "asyncContext is nullptr");
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    int32_t downloadTypeInt = 0;
    CHECK_COND_WITH_RET_MESSAGE(env, MediaLibraryEnumAni::EnumGetValueInt32(
        env, downloadType, downloadTypeInt) == ANI_OK, nullptr, "Failed to get downloadType");
    asyncContext->cloudMediaDownloadType = downloadTypeInt;
    ANI_INFO_LOG("[CPP] enter downloadType: %{public}d", asyncContext->cloudMediaDownloadType);

    StartDownloadCloudMediaExecute(env, asyncContext.get());
    CommonComplete(env, asyncContext);
    ani_object result {};
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryAniUtils::ToAniBooleanObject(env, true, result) == ANI_OK,
        "Failed to convert result to ani_object");
    return result;
}

static void PauseDownloadCloudMediaExecute(ani_env *env, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PauseDownloadCloudMediaExecute");
    ANI_INFO_LOG("enter PauseDownloadCloudMediaExecute");

    auto* context = static_cast<CloudMediaAssetAsyncAniContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAUSE_DOWNLOAD_CLOUDMEDIA);
    ANI_INFO_LOG("before IPC::UserDefineIPCClient().Call");
    int32_t changedRows = IPC::UserDefineIPCClient().Call(businessCode);
    ANI_INFO_LOG("after IPC::UserDefineIPCClient().Call");
    if (changedRows < 0) {
        context->SaveError(changedRows);
        ANI_ERR_LOG("Pause download cloud media failed, err: %{public}d", changedRows);
    }
}

ani_object CloudMediaAssetManagerAni::PauseDownloadCloudMedia(ani_env *env, ani_object aniObject)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("PauseDownloadCloudMedia");

    auto asyncContext = make_unique<CloudMediaAssetAsyncAniContext>();
    CHECK_COND_RET(asyncContext != nullptr, nullptr, "asyncContext is nullptr");
    PauseDownloadCloudMediaExecute(env, asyncContext.get());
    CommonComplete(env, asyncContext);
    ani_object result {};
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryAniUtils::ToAniBooleanObject(env, true, result) == ANI_OK,
        "Failed to convert result to ani_object");
    return result;
}

static void CancelDownloadCloudMediaExecute(ani_env *env, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("CancelDownloadCloudMediaExecute");
    ANI_INFO_LOG("enter CancelDownloadCloudMediaExecute");

    auto* context = static_cast<CloudMediaAssetAsyncAniContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CANCEL_DOWNLOAD_CLOUDMEDIA);
    ANI_INFO_LOG("before IPC::UserDefineIPCClient().Call");
    int32_t changedRows = IPC::UserDefineIPCClient().Call(businessCode);
    ANI_INFO_LOG("after IPC::UserDefineIPCClient().Call");
    if (changedRows < 0) {
        context->SaveError(changedRows);
        ANI_ERR_LOG("Cancel download cloud media failed, err: %{public}d", changedRows);
    }
}

ani_object CloudMediaAssetManagerAni::CancelDownloadCloudMedia(ani_env *env, ani_object aniObject)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("CancelDownloadCloudMedia");

    auto asyncContext = make_unique<CloudMediaAssetAsyncAniContext>();
    CHECK_COND_RET(asyncContext != nullptr, nullptr, "asyncContext is nullptr");
    CancelDownloadCloudMediaExecute(env, asyncContext.get());
    CommonComplete(env, asyncContext);
    ani_object result {};
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryAniUtils::ToAniBooleanObject(env, true, result) == ANI_OK,
        "Failed to convert result to ani_object");
    return result;
}

static void RetainCloudMediaAssetExecute(ani_env *env, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("RetainCloudMediaAssetExecute");
    ANI_INFO_LOG("enter RetainCloudMediaAssetExecute");

    auto* context = static_cast<CloudMediaAssetAsyncAniContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    RetainCloudMediaAssetReqBody reqBody;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::RETAIN_CLOUDMEDIA_ASSET);
    reqBody.cloudMediaRetainType = context->cloudMediaRetainType;
    ANI_INFO_LOG("before IPC::UserDefineIPCClient().Call");
    int32_t changedRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    ANI_INFO_LOG("after IPC::UserDefineIPCClient().Call");
    if (changedRows < 0) {
        context->SaveError(changedRows);
        ANI_ERR_LOG("Retain cloud media asset failed, err: %{public}d", changedRows);
    }
}

ani_object CloudMediaAssetManagerAni::RetainCloudMediaAsset(ani_env *env, ani_object aniObject,
    ani_enum_item retainType)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("RetainCloudMediaAsset");

    auto asyncContext = make_unique<CloudMediaAssetAsyncAniContext>();
    CHECK_COND_RET(asyncContext != nullptr, nullptr, "asyncContext is nullptr");
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    int32_t retainTypeInt = 0;
    CHECK_COND_WITH_RET_MESSAGE(env, MediaLibraryEnumAni::EnumGetValueInt32(env, retainType, retainTypeInt)== ANI_OK,
        nullptr, "Failed to get retainTypeInt");
    asyncContext->cloudMediaRetainType = retainTypeInt;
    ANI_INFO_LOG("[CPP] enter cloudMediaRetainType: %{public}d", asyncContext->cloudMediaRetainType);

    RetainCloudMediaAssetExecute(env, asyncContext.get());
    CommonComplete(env, asyncContext);
    ani_object result {};
    return result;
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

static bool CanConvertToInt32(const std::string &str)
{
    std::istringstream stringStream(str);
    int32_t num = 0;
    stringStream >> num;
    return stringStream.eof() && !stringStream.fail();
}

static void GetCloudMediaAssetStatusExecute(ani_env *env, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetCloudMediaAssetStatusExecute");
    ANI_INFO_LOG("enter GetCloudMediaAssetStatusExecute");

    auto* context = static_cast<CloudMediaAssetAsyncAniContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");

    GetCloudMediaAssetStatusReqBody reqBody;
    GetCloudMediaAssetStatusReqBody rspBody;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_GET_CLOUDMEDIA_ASSET_STATUS);
    IPC::UserDefineIPCClient().Call(businessCode, reqBody, rspBody);
    ANI_INFO_LOG("Get cloud media asset, res: %{public}s.", rspBody.status.c_str());
    std::vector<std::string> type;
    if (!SplitUriString(rspBody.status, type)) {
        ANI_ERR_LOG("GetType failed");
        return;
    }
    if (!CanConvertToInt32(type[INDEX_ZERO]) || !CanConvertToInt32(type[INDEX_FIVE])) {
        ANI_ERR_LOG("GetType failed");
        return;
    }
    context->cloudMediaAssetTaskStatus_ = static_cast<CloudMediaAssetTaskStatus>(std::atoi(type[INDEX_ZERO].c_str()));
    context->cloudMediaTaskPauseCause_ = static_cast<CloudMediaTaskPauseCause>(std::atoi(type[INDEX_FIVE].c_str()));
    std::string taskInfo = "totalcount: " + type[INDEX_ONE] + "," +
                        "totalSize: " + type[INDEX_TWO] + "," +
                        "remainCount: " + type[INDEX_THREE] + "," +
                        "remainSize: " + type[INDEX_FOUR];
    context->taskInfo_ = taskInfo;
}

static ani_object GetCloudMediaAssetStatusComplete(ani_env *env, unique_ptr<CloudMediaAssetAsyncAniContext> &context)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    ani_object AssetStateObj {};
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    } else {
        AssetStateObj = CloudMediaAssetStatusAni::NewCloudMediaAssetStatusAni(env, context.get());
    }
    context.reset();
    return AssetStateObj;
}

ani_object CloudMediaAssetManagerAni::GetCloudMediaAssetStatus(ani_env *env, ani_object info)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("GetCloudMediaAssetStatus");

    auto asyncContext = make_unique<CloudMediaAssetAsyncAniContext>();
    CHECK_COND_RET(asyncContext != nullptr, nullptr, "asyncContext is nullptr");
    GetCloudMediaAssetStatusExecute(env, asyncContext.get());
    return GetCloudMediaAssetStatusComplete(env, asyncContext);
}
} // namespace OHOS::Media

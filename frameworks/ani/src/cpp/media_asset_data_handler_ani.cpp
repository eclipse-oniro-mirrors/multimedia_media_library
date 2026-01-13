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

#include "media_asset_data_handler_ani.h"
#include "ani_class_name.h"
#include "medialibrary_ani_utils.h"

namespace OHOS {
namespace Media {
std::mutex AniMediaAssetDataHandler::dataHandlerRefMutex_;

AniMediaAssetDataHandler::AniMediaAssetDataHandler(ani_env *env, ani_ref dataHandler, ReturnDataType dataType,
    const std::string &uri, const std::string &destUri, SourceMode sourceMode)
{
    dataType_ = dataType;
    env_ = env;
    requestUri_ = uri;
    destUri_ = destUri;
    sourceMode_ = sourceMode;
    dataHandlerRef_ = dataHandler;
    compatibleMode_ = CompatibleMode::ORIGINAL_FORMAT_MODE;
}

void AniMediaAssetDataHandler::DeleteAniReference(ani_env *env)
{
    std::unique_lock<std::mutex> dataHandlerLock(dataHandlerRefMutex_);
    if (dataHandlerRef_ != nullptr) {
        if (env != nullptr) {
            env->GlobalReference_Delete(dataHandlerRef_);
        } else {
            env_->GlobalReference_Delete(dataHandlerRef_);
        }
        dataHandlerRef_ = nullptr;
    }
    dataHandlerLock.unlock();
}

ReturnDataType AniMediaAssetDataHandler::GetReturnDataType()
{
    return dataType_;
}

std::string AniMediaAssetDataHandler::GetRequestUri()
{
    return requestUri_;
}

std::string AniMediaAssetDataHandler::GetDestUri()
{
    return destUri_;
}

SourceMode AniMediaAssetDataHandler::GetSourceMode()
{
    return sourceMode_;
}

void AniMediaAssetDataHandler::SetNotifyMode(NotifyMode notifyMode)
{
    notifyMode_ = notifyMode;
}

NotifyMode AniMediaAssetDataHandler::GetNotifyMode()
{
    return notifyMode_;
}

void AniMediaAssetDataHandler::SetRequestId(std::string requestId)
{
    requestId_ = requestId;
}

std::string AniMediaAssetDataHandler::GetRequestId()
{
    return requestId_;
}

void AniMediaAssetDataHandler::SetCompatibleMode(const CompatibleMode &compatibleMode)
{
    compatibleMode_ = compatibleMode;
}

CompatibleMode AniMediaAssetDataHandler::GetCompatibleMode()
{
    return compatibleMode_;
}

ani_ref AniMediaAssetDataHandler::GetProgressHandlerRef()
{
    return progressHandlerRef_;
}

void AniMediaAssetDataHandler::SetProgressHandlerRef(ani_ref &progressHandlerRef)
{
    progressHandlerRef_ = progressHandlerRef;
}

ThreadFunctionOnProgress AniMediaAssetDataHandler::GetThreadsafeFunction()
{
    return threadsafeFunction_;
}

void AniMediaAssetDataHandler::SetThreadsafeFunction(ThreadFunctionOnProgress &threadsafeFunction)
{
    threadsafeFunction_ = threadsafeFunction;
}

void AniMediaAssetDataHandler::EtsOnDataPrepared(ani_env *env, ani_object arg, ani_object extraInfo)
{
    static const char *className = PAH_ANI_CLASS_MEDIA_MANAGER.c_str();
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    ani_class cls {};
    ani_status status = env->FindClass(className, &cls);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to find class: %{public}s, ani status: %{public}d", className, static_cast<int>(status));
        return;
    }
    static const char *methodName = ON_MEDIA_ASSET_DATA_PREPARED_FUNC;
    ani_static_method etsOnDataPrepared {};
    status = env->Class_FindStaticMethod(cls, methodName, nullptr, &etsOnDataPrepared);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to find static method: %{public}s, ani status: %{public}d",
            methodName, static_cast<int>(status));
        return;
    }

    std::unique_lock<std::mutex> dataHandlerLock(dataHandlerRefMutex_);
    if (dataHandlerRef_ == nullptr) {
        ANI_ERR_LOG("Ets dataHandler reference is null");
        dataHandlerLock.unlock();
        return;
    }
    ani_object callback = static_cast<ani_object>(dataHandlerRef_);
    status = env->Class_CallStaticMethod_Void(cls, etsOnDataPrepared, arg, callback, extraInfo);
    dataHandlerLock.unlock();
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to execure static method: %{public}s, ani status: %{public}d",
            methodName, static_cast<int>(status));
        AniError::ThrowError(env, JS_INNER_FAIL, "calling onDataPrepared failed");
        return;
    }
}

void AniMediaAssetDataHandler::EtsOnDataPrepared(ani_env *env, ani_object pictures, ani_object arg,
    ani_object extraInfo)
{
    static const char *className = PAH_ANI_CLASS_MEDIA_MANAGER.c_str();
    ani_class cls {};
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    ani_status status = env->FindClass(className, &cls);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to find class: %{public}s, ani status: %{public}d", className, static_cast<int>(status));
        return;
    }
    static const char *methodName = ON_QUICK_IMAGE_DATA_PREPARED_FUNC;
    ani_static_method etsOnDataPrepared {};
    status = env->Class_FindStaticMethod(cls, methodName, nullptr, &etsOnDataPrepared);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to find static method: %{public}s, ani status: %{public}d",
            methodName, static_cast<int>(status));
        return;
    }

    std::unique_lock<std::mutex> dataHandlerLock(dataHandlerRefMutex_);
    if (dataHandlerRef_ == nullptr) {
        ANI_ERR_LOG("Ets dataHandler reference is null");
        dataHandlerLock.unlock();
        return;
    }
    ani_object callback = static_cast<ani_object>(dataHandlerRef_);
    status = env->Class_CallStaticMethod_Void(cls, etsOnDataPrepared, pictures, arg, extraInfo, callback);
    dataHandlerLock.unlock();
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to execure static method: %{public}s, ani status: %{public}d",
            methodName, static_cast<int>(status));
        AniError::ThrowError(env, JS_INNER_FAIL, "calling onDataPrepared failed");
        return;
    }
}
} // namespace Media
} // namespace OHOS

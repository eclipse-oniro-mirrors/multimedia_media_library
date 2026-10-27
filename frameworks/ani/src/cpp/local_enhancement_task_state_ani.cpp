/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "ani_class_name.h"
#include "local_enhancement_task_state_ani.h"

using namespace std;

namespace OHOS {
namespace Media {
ani_status LocalEnhancementTaskStateAni::Init(ani_env *env)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    static const char *className = PAH_ANI_CLASS_LOCAL_ENHANCEMENT_TASK_STATE_HANDLE.c_str();
    ani_class cls;
    ani_status status = env->FindClass(className, &cls);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to find class: %{public}s", className);
        return status;
    }

    std::array methods = {
        ani_native_function {"getProgress", nullptr,
            reinterpret_cast<void *>(LocalEnhancementTaskStateAni::GetProgress)},
    };

    status = env->Class_BindNativeMethods(cls, methods.data(), methods.size());
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to bind native methods to: %{public}s", className);
        return status;
    }
    return ANI_OK;
}

ani_object LocalEnhancementTaskStateAni::NewLocalEnhancementTaskStateAni(ani_env *env,
    unique_ptr<CloudEnhancementAniContext> &context)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_RET(context != nullptr, nullptr, "CloudEnhancementAniContext is nullptr");
    static const char *className = PAH_ANI_CLASS_LOCAL_ENHANCEMENT_TASK_STATE_HANDLE.c_str();
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        ANI_ERR_LOG("Failed to find class: %{public}s", className);
        return nullptr;
    }
    unique_ptr<LocalEnhancementTaskStateAni> nativeHandle = make_unique<LocalEnhancementTaskStateAni>();
    CHECK_COND_RET(nativeHandle != nullptr, nullptr, "LocalEnhancementTaskStateAni is nullptr");
    nativeHandle->SetProgress(context->localEnhancementProgress_);

    ani_object aniObject = Constructor(env, cls, nativeHandle);
    CHECK_COND_RET(aniObject != nullptr, nullptr, "Failed to construct LocalEnhancementTaskStateAni");
    (void)nativeHandle.release();
    return aniObject;
}

ani_object LocalEnhancementTaskStateAni::Constructor(ani_env *env, ani_class cls,
    unique_ptr<LocalEnhancementTaskStateAni> &nativeHandle)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_RET(nativeHandle != nullptr, nullptr, "nativeHandle is nullptr");
    ani_method ctor;
    if (ANI_OK != env->Class_FindMethod(cls, "<ctor>", nullptr, &ctor)) {
        ANI_ERR_LOG("Failed to find method: %{public}s", "ctor");
        return nullptr;
    }

    ani_object aniObject;
    if (ANI_OK != env->Object_New(cls, ctor, &aniObject, reinterpret_cast<ani_long>(nativeHandle.get()))) {
        ANI_ERR_LOG("New LocalEnhancementTaskStateAni Fail");
        return nullptr;
    }
    return aniObject;
}

LocalEnhancementTaskStateAni* LocalEnhancementTaskStateAni::Unwrap(ani_env *env, ani_object aniObject)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    ani_long nativeHandle;
    if (ANI_OK != env->Object_GetFieldByName_Long(aniObject, "nativeHandle", &nativeHandle)) {
        ANI_ERR_LOG("Failed to get nativeHandle");
        return nullptr;
    }
    return reinterpret_cast<LocalEnhancementTaskStateAni*>(nativeHandle);
}

ani_int LocalEnhancementTaskStateAni::GetProgress(ani_env *env, ani_object object)
{
    CHECK_COND_RET(env != nullptr, 0, "env is nullptr");
    LocalEnhancementTaskStateAni* nativeHandle = Unwrap(env, object);
    CHECK_COND_RET(nativeHandle != nullptr, 0, "LocalEnhancementTaskStateAni is nullptr");
    return static_cast<ani_int>(nativeHandle->progress_);
}

void LocalEnhancementTaskStateAni::SetProgress(int32_t progress)
{
    this->progress_ = progress;
}
} // namespace Media
} // namespace OHOS
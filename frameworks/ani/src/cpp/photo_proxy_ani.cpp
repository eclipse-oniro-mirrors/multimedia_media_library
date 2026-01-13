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

#include "photo_proxy_ani.h"
#include "ani_class_name.h"
#include "medialibrary_ani_utils.h"

namespace OHOS {
namespace Media {
thread_local sptr<PhotoProxy> PhotoProxyAni::sPhotoProxy_ = nullptr;
PhotoProxyAni::PhotoProxyAni() : env_(nullptr), wrapper_(nullptr)
{
}

PhotoProxyAni::~PhotoProxyAni()
{
    if (wrapper_ != nullptr && env_!= nullptr) {
        env_->GlobalReference_Delete(wrapper_);
        wrapper_ = nullptr;
    }
    if (photoProxy_ != nullptr) {
        photoProxy_ = nullptr;
    }
    if (env_ != nullptr) {
        env_ = nullptr;
    }
}

ani_status PhotoProxyAni::Init(ani_env *env)
{
    static const char *className = PAH_ANI_CLASS_PHOTO_PROXY_HANDLE.c_str();
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    ani_class cls;
    ani_status status = env->FindClass(className, &cls);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to find class: %{public}s", className);
        return status;
    }

    std::array methods = {
        ani_native_function {"create", nullptr,
            reinterpret_cast<void *>(PhotoProxyAni::PhotoProxyAniConstructor)},
    };

    status = env->Class_BindStaticNativeMethods(cls, methods.data(), methods.size());
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to bind static native methods to: %{public}s", className);
        return status;
    }
    return ANI_OK;
}

// Constructor callback
ani_object PhotoProxyAni::PhotoProxyAniConstructor(ani_env *env, [[maybe_unused]] ani_class clazz)
{
    ANI_DEBUG_LOG("PhotoProxyAniConstructor is called");
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    std::unique_ptr<PhotoProxyAni> nativeHandle = std::make_unique<PhotoProxyAni>();
    CHECK_COND_RET(nativeHandle != nullptr, nullptr, "Failed to create PhotoProxyAni");
    nativeHandle->env_ = env;
    nativeHandle->photoProxy_ = sPhotoProxy_;

    static const char *className = PAH_ANI_CLASS_PHOTO_PROXY_HANDLE.c_str();
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        ANI_ERR_LOG("Failed to find class: %{public}s", className);
        return nullptr;
    }

    ani_method ctor;
    if (ANI_OK != env->Class_FindMethod(cls, "<ctor>", "l:", &ctor)) {
        ANI_ERR_LOG("Failed to find method: %{public}s", "ctor");
        return nullptr;
    }

    // wrap nativeHandle to aniObject
    ani_object aniObject;
    if (ANI_OK != env->Object_New(cls, ctor, &aniObject, reinterpret_cast<ani_long>(nativeHandle.get()))) {
        ANI_ERR_LOG("New PhotoProxy Fail");
        return nullptr;
    }
    (void)nativeHandle.release();
    return aniObject;
}

void PhotoProxyAni::PhotoProxyAniDestructor(ani_env *env, void *nativeObject, void *finalize_hint)
{
    ANI_DEBUG_LOG("PhotoProxyAniDestructor is called");
    PhotoProxyAni* deferredPhotoProxy = reinterpret_cast<PhotoProxyAni*>(nativeObject);
    if (deferredPhotoProxy != nullptr) {
        delete deferredPhotoProxy;
    }
}
} // namespace Media
} // namespace OHOS

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

#define MLOG_TAG "EnhancementDynamicLoader"

#include "dynamic_loader.h"

#include <dlfcn.h>
#include "media_log.h"
#include "medialibrary_tracer.h"

namespace OHOS {
namespace Media {
using namespace std;
static const char *K_LIBRARY_SUFFIX = ".so";
DynamicLoader::DynamicLoader()
{
    MEDIA_INFO_LOG("EnhancementDynamicLoader ctor");
}

DynamicLoader::~DynamicLoader()
{
    MEDIA_INFO_LOG("EnhancementDynamicLoader dtor");
    for (auto iterator = dynamicLibHandle_.begin(); iterator != dynamicLibHandle_.end(); ++iterator) {
        dlclose(iterator->second);
        MEDIA_INFO_LOG("close library media_cloud_enhancement_dynamic success: %{public}s", iterator->first.c_str());
    }
}

void* DynamicLoader::OpenDynamicHandle(std::string dynamicLibrary)
{
    MediaLibraryTracer tracer;
    tracer.Start("EnhancementDynamicLoader::OpenDynamicHandle");
    std::lock_guard loaderLock(libLock_);
    if (!EndsWith(dynamicLibrary, K_LIBRARY_SUFFIX)) {
        MEDIA_ERR_LOG("CloseDynamicHandle with error name!");
        return nullptr;
    }
    if (dynamicLibHandle_[dynamicLibrary] == nullptr) {
        void* dynamicLibHandle = dlopen(dynamicLibrary.c_str(), RTLD_NOW);
        if (dynamicLibHandle == nullptr) {
            MEDIA_ERR_LOG("Failed to open %{public}s, reason: %{public}sn", dynamicLibrary.c_str(), dlerror());
            return nullptr;
        }
        MEDIA_INFO_LOG("open library %{public}s success", dynamicLibrary.c_str());
        dynamicLibHandle_[dynamicLibrary] = dynamicLibHandle;
    }
    return dynamicLibHandle_[dynamicLibrary];
}
 
void* DynamicLoader::GetFunction(const string dynamicLibrary, const string function)
{
    MediaLibraryTracer tracer;
    tracer.Start("EnhancementDynamicLoader::GetFunction");
    std::lock_guard loaderLock(libLock_);
    // if not opened, then open directly
    if (dynamicLibHandle_[dynamicLibrary] == nullptr) {
        OpenDynamicHandle(dynamicLibrary);
    }
 
    void* handle = nullptr;
    if (dynamicLibHandle_[dynamicLibrary] != nullptr) {
        handle = dlsym(dynamicLibHandle_[dynamicLibrary], function.c_str());
        if (handle == nullptr) {
            MEDIA_ERR_LOG("Failed to load %{public}s, reason: %{public}sn", function.c_str(), dlerror());
            return nullptr;
        }
        MEDIA_INFO_LOG("GetFunction %{public}s success", function.c_str());
    }
    return handle;
}
 
void DynamicLoader::CloseDynamicHandle(std::string dynamicLibrary)
{
    MediaLibraryTracer tracer;
    tracer.Start("EnhancementDynamicLoader::CloseDynamicHandle");
    std::lock_guard loaderLock(libLock_);
    if (!EndsWith(dynamicLibrary, K_LIBRARY_SUFFIX)) {
        MEDIA_ERR_LOG("CloseDynamicHandle with error name!");
        return;
    }
    if (dynamicLibHandle_[dynamicLibrary] != nullptr) {
        dlclose(dynamicLibHandle_[dynamicLibrary]);
        dynamicLibHandle_[dynamicLibrary] = nullptr;
        MEDIA_INFO_LOG("close library media_cloud_enhancement_dynamic success: %{public}s",
            dynamicLibrary.c_str());
    }
}
}  // namespace Media
}  // namespace OHOS
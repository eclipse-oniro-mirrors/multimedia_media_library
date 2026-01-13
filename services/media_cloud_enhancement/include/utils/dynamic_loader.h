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

#ifndef FRAMEWORKS_SERVICES_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_DYNAMIC_LOADER_H
#define FRAMEWORKS_SERVICES_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_DYNAMIC_LOADER_H

#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

const std::string MEDIA_CLOUD_ENHANCE_LIB_SO = "/system/lib64/platformsdk/libmedia_cloud_enhance_plugin.z.so";

class DynamicLoader {
public:
    DynamicLoader();
    ~DynamicLoader();
 
    void* OpenDynamicHandle(std::string dynamicLibrary);
    void CloseDynamicHandle(std::string dynamicLibrary);
    void* GetFunction(const std::string dynamicLibrary, const std::string function);
    inline bool EndsWith(const std::string& str, const std::string& suffix)
    {
        if (str.length() >= suffix.length()) {
            return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
        }
        return false;
    }

private:
    std::map<std::string, void *> dynamicLibHandle_;
    std::recursive_mutex libLock_;
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_DYNAMIC_LOADER_H
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

#ifndef HEIF_BUNDLE_INFO_CACHE_H
#define HEIF_BUNDLE_INFO_CACHE_H

#include <list>
#include <mutex>
#include <string>
#include <unordered_map>

namespace OHOS {
namespace Media {

#define EXPORT __attribute__ ((visibility ("default")))

class HeifBundleInfoCache {
public:
    HeifBundleInfoCache() = delete;
    ~HeifBundleInfoCache() = delete;
    EXPORT static void ClearBundleInfoInCache();
    EXPORT static bool GetBundleCacheInfo(const std::string &bundleName, bool &isSupport);
    EXPORT static void InsertBundleCacheInfo(const std::string &bundleName, bool isSupport);
private:
    static inline std::mutex cacheMutex_;
    static inline std::list<std::pair<std::string, bool>> bundleInfoList_;
    static inline std::unordered_map<std::string, std::list<std::pair<std::string, bool>>::iterator> bundleInfoMap_;
};

} // namespace Media
} // namespace OHOS

#endif // HEIF_BUNDLE_INFO_CACHE_H
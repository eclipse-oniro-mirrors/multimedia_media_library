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

#define MLOG_TAG "HeifBundleInfoCache"

#include "heif_bundle_info_cache.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
namespace {
const uint32_t CAPACITY_BUNDLE_INFO = 50;
}

void HeifBundleInfoCache::ClearBundleInfoInCache()
{
    std::lock_guard<std::mutex> lock(cacheMutex_);
    bundleInfoMap_.clear();
    bundleInfoList_.clear();
}

bool HeifBundleInfoCache::GetBundleCacheInfo(const std::string &bundleName, bool &isSupport)
{
    std::lock_guard<std::mutex> lock(cacheMutex_);
    auto it = bundleInfoMap_.find(bundleName);
    if (it != bundleInfoMap_.end() && it->second != bundleInfoList_.end()) {
        isSupport = it->second->second;
        bundleInfoList_.splice(bundleInfoList_.begin(), bundleInfoList_, it->second);
        return true;
    }
    return false;
}

void HeifBundleInfoCache::InsertBundleCacheInfo(const std::string &bundleName, bool isSupport)
{
    std::lock_guard<std::mutex> lock(cacheMutex_);
    bundleInfoList_.push_front(std::make_pair(bundleName, isSupport));
    bundleInfoMap_[bundleName] = bundleInfoList_.begin();
    if (bundleInfoMap_.size() > CAPACITY_BUNDLE_INFO) {
        auto eraseKey = bundleInfoList_.back().first;
        bundleInfoMap_.erase(eraseKey);
        bundleInfoList_.pop_back();
        MEDIA_INFO_LOG("[cache] %{public}s is erased", eraseKey.c_str());
    }
}
} // namespace Media
} // namespace OHOS
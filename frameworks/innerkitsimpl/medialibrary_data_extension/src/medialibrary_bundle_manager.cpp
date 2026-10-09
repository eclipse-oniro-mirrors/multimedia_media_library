/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#define MLOG_TAG "BundleManager"

#include "medialibrary_bundle_manager.h"

#include <memory>
#include <mutex>

#include "ipc_skeleton.h"
#include "permission_utils.h"

using namespace std;

namespace OHOS {
namespace Media {

std::once_flag MediaLibraryBundleManager::oc_;
shared_ptr<MediaLibraryBundleManager> MediaLibraryBundleManager::instance_ = nullptr;
shared_ptr<MediaLibraryBundleManager> MediaLibraryBundleManager::GetInstance()
{
    std::call_once(oc_, []() {
        instance_ = std::make_shared<MediaLibraryBundleManager>();
    });
    return instance_;
}

void MediaLibraryBundleManager::GetBundleNameByUID(const int32_t uid, string &bundleName)
{
    PermissionUtils::GetClientBundle(uid, bundleName);
}

void MediaLibraryBundleManager::GetBundleNameByTokenId(const uint64_t tokenId, string &bundleName)
{
    if (tokenIdToBundleNameCache_.Find(tokenId, bundleName)) {
        return;
    }
    bundleName = GetClientBundleName();
    tokenIdToBundleNameCache_.EnsureInsert(tokenId, bundleName);
}

/**
 * if it is called by SA, bundlename is null. we should not log everytime
 */
std::string MediaLibraryBundleManager::GetClientBundleName()
{
    int32_t uid = IPCSkeleton::GetCallingUid();
    {
        lock_guard<mutex> lock(uninstallMutex_);
        auto iter = cacheMap_.find(uid);
        if (iter != cacheMap_.end()) {
            cacheList_.splice(cacheList_.begin(), cacheList_, iter->second);
            return iter->second->second;
        }
    }
    string bundleName;
    GetBundleNameByUID(uid, bundleName);
    if (bundleName.empty()) {
        return bundleName;
    }
    lock_guard<mutex> lock(uninstallMutex_);
    auto it = cacheMap_.find(uid);
    if (it != cacheMap_.end()) {
        cacheList_.erase(it->second);
    }
    cacheList_.push_front(make_pair(uid, bundleName));
    cacheMap_[uid] = cacheList_.begin();
    if (cacheMap_.size() > CAPACITY) {
        int32_t deleteKey = cacheList_.back().first;
        cacheMap_.erase(deleteKey);
        cacheList_.pop_back();
    }
    return bundleName;
}

void MediaLibraryBundleManager::Clear()
{
    lock_guard<mutex> lock(uninstallMutex_);
    cacheList_.clear();
    cacheMap_.clear();
}
} // Media
} // OHOS

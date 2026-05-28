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

#ifndef FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_HELPER_INCLUDE_KEYD_MUTEX_H
#define FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_HELPER_INCLUDE_KEYD_MUTEX_H

#include <mutex>
#include <unordered_map>
#include <memory>
#include <functional>
#include <utility>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

template <typename KeyType,
          typename Hash     = std::hash<KeyType>,
          typename KeyEqual = std::equal_to<KeyType>>
class KeydMutex {
    std::unordered_map<KeyType, std::weak_ptr<std::mutex>, Hash, KeyEqual> mutexMap_;
    std::mutex metaMutex_;

public:
    std::shared_ptr<std::mutex> Get(const KeyType& key)
    {
        std::lock_guard<std::mutex> mapLock(metaMutex_);
        auto& mutexObserver = mutexMap_[key];
        if (auto mutexSharedPtr = mutexObserver.lock()) {
            return mutexSharedPtr;
        }
        std::shared_ptr<std::mutex> mutexSharedPtr(new std::mutex(),
            [this, key] (std::mutex* mutexPtr) {
                {
                    std::lock_guard<std::mutex> mapLock(metaMutex_);
                    auto it = mutexMap_.find(key);
                    if (it != mutexMap_.end() && it->second.expired()) {
                        mutexMap_.erase(it);
                    }
                }
                delete mutexPtr;
            });
        mutexObserver = mutexSharedPtr;
        return mutexSharedPtr;
    }
};

} // Media
} // OHOS

#endif // FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_HELPER_INCLUDE_KEYD_MUTEX_H
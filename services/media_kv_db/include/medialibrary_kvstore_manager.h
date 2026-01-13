/*
 * Copyright (C) 2023-2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIALIBRARY_KVSTORE_MANAGER_H
#define OHOS_MEDIALIBRARY_KVSTORE_MANAGER_H

#include <atomic>
#include <safe_map.h>

#include "medialibrary_kvstore.h"
#include "timer.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
using KvStoreSharedPtr = std::shared_ptr<MediaLibraryKvStore>;
constexpr size_t KVSTORE_INSERT_COUNT = 20;
constexpr size_t CLOSE_KVSTORE_TIME_INTERVAL = 270000;

class MediaLibraryKvStoreManager {
public:
    EXPORT static MediaLibraryKvStoreManager &GetInstance()
    {
        static MediaLibraryKvStoreManager instance;
        return instance;
    }

    EXPORT std::shared_ptr<MediaLibraryKvStore> InitKvStore(const KvStoreRoleType &roleType,
        const KvStoreValueType &valueType);
    EXPORT std::shared_ptr<MediaLibraryKvStore> GetKvStore(
        const KvStoreRoleType &roleType, const KvStoreValueType &valueType);
    EXPORT bool InitMonthAndYearKvStore(const KvStoreRoleType &roleType);
    EXPORT bool CloseKvStore(const KvStoreValueType &valueType);
    EXPORT void CloseAllKvStore();
    EXPORT void TryCloseAllKvStore();
    EXPORT bool IsKvStoreValid(const KvStoreValueType &valueType);
    EXPORT int32_t RebuildInvalidKvStore(const KvStoreValueType &valueType);
    EXPORT std::shared_ptr<MediaLibraryKvStore> GetSingleKvStore(const KvStoreRoleType &roleType,
        const std::string &storeId, const std::string &baseDir);
    EXPORT int32_t CloneKvStore(const std::string &oldKvStoreId, const std::string &oldBaseDir,
        const std::string &newKvStoreId, const std::string &newBaseDir);

private:
    MediaLibraryKvStoreManager() = default;
    ~MediaLibraryKvStoreManager() = default;

    SafeMap<KvStoreValueType, KvStoreSharedPtr> kvStoreMap_;
    std::mutex mutex_;
    std::atomic<int64_t> kvStoreEvokedTimeStamp_{0};
};
} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIALIBRARY_KVSTORE_MANAGER_H
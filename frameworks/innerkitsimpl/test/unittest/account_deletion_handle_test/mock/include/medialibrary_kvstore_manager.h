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

// Test stub for the account-scoped KV store manager APIs. The REAL
// signatures live in services/media_kv_db (added there: MediaLibraryKvStoreRoute
// in medialibrary_kvstore.h; CloseKvStore(route, type) / DeleteAccountKvStores(route)
// in medialibrary_kvstore_manager.h). This stub shadows the real header so the
// test exe stays self-contained (no kvdb service needed); counts and
// DeleteAccountKvStores' return value are controlled via AccountDeletionMock.
// KvStoreValueType comes from the real medialibrary_kvstore.h.
#ifndef OHOS_MEDIALIBRARY_KVSTORE_MANAGER_MOCK_H
#define OHOS_MEDIALIBRARY_KVSTORE_MANAGER_MOCK_H

#include <cstdint>

#include "medialibrary_kvstore.h" // real: KvStoreValueType, MediaLibraryKvStoreRoute

#define ACCOUNT_DELETION_STUB_KV_MANAGER 1

namespace OHOS {
namespace Media {

class MediaLibraryKvStoreManager {
public:
    static MediaLibraryKvStoreManager &GetInstance();

    // real impl in media_kv_db (no-op until the per-account cache lands)
    bool CloseKvStore(const MediaLibraryKvStoreRoute &route, const KvStoreValueType &valueType);

    // real impl in media_kv_db (kvdb framework physical delete)
    int32_t DeleteAccountKvStores(const MediaLibraryKvStoreRoute &route);

private:
    MediaLibraryKvStoreManager() = default;
    ~MediaLibraryKvStoreManager() = default;
};

}  // namespace Media
}  // namespace OHOS

#endif  // OHOS_MEDIALIBRARY_KVSTORE_MANAGER_MOCK_H

/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
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

#include "medialibrary_kvstore_manager.h"
#include <sys/stat.h>

#include "medialibrary_errno.h"
#include "media_file_utils.h"
#include "media_log.h"
#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
#include "account_scope.h"
#include "media_path_utils.h"
#endif


using namespace OHOS::DistributedKv;
namespace OHOS::Media {

MediaLibraryKvStoreManager &MediaLibraryKvStoreManager::GetInstance()
{
    static MediaLibraryKvStoreManager instance;
    return instance;
}

std::shared_ptr<MediaLibraryKvStore> MediaLibraryKvStoreManager::InitKvStore(const KvStoreRoleType &roleType,
    const KvStoreValueType &valueType)
{
#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
    MediaLibraryKvStoreRoute route;
    CHECK_AND_RETURN_RET(ResolveRoute(route), nullptr);
    if (route.multiAccountEnabled) {
        return GetKvStore(roleType, valueType, route);
    }
#endif
    std::lock_guard<std::mutex> lock(mutex_);
    KvStoreSharedPtr ptr;
    if (kvStoreMap_.Find(valueType, ptr)) {
        return ptr;
    }

    std::string baseDir = "";
    if (roleType == KvStoreRoleType::OWNER) {
        baseDir = KV_STORE_OWNER_DIR;
    } else if (roleType == KvStoreRoleType::VISITOR) {
        baseDir = KV_STORE_VISITOR_DIR;
    } else {
        MEDIA_ERR_LOG("invalid role type");
        return nullptr;
    }

    ptr = std::make_shared<MediaLibraryKvStore>();
    int32_t status = ptr->Init(roleType, valueType, baseDir);
    if (status != E_OK) {
        MEDIA_ERR_LOG("init kvStore failed, status %{public}d", status);
        return nullptr;
    }
    kvStoreMap_.Insert(valueType, ptr);
    return ptr;
}

std::shared_ptr<MediaLibraryKvStore> MediaLibraryKvStoreManager::GetKvStore(
    const KvStoreRoleType &roleType, const KvStoreValueType &valueType)
{
#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
    MediaLibraryKvStoreRoute route;
    CHECK_AND_RETURN_RET(ResolveRoute(route), nullptr);
    if (route.multiAccountEnabled) {
        return GetKvStore(roleType, valueType, route);
    }
#endif
    int64_t currentTimestamp = MediaFileUtils::UTCTimeMilliSeconds();
    kvStoreEvokedTimeStamp_.store(currentTimestamp);
    KvStoreSharedPtr ptr;
    CHECK_AND_RETURN_RET(!kvStoreMap_.Find(valueType, ptr), ptr);

    return InitKvStore(roleType, valueType);
}

void MediaLibraryKvStoreManager::EnsureKvStoreFresh(const KvStoreRoleType &roleType,
    const KvStoreValueType &valueType)
{
    struct stat st;
    CHECK_AND_RETURN_LOG(stat(KVDB_SWAP_MARKER_FILE.c_str(), &st) == 0, "KVDB swap marker not found");
    int64_t markerMtimeMs = MediaFileUtils::Timespec2Millisecond(st.st_mtim);
    if (markerMtimeMs <= clientOpenKvdbTime_.load(std::memory_order_relaxed)) {
        return;
    }
    std::lock_guard<std::mutex> lock(kvdbMutex_);
    if (markerMtimeMs <= clientOpenKvdbTime_.load(std::memory_order_relaxed)) {
        return;
    }
    DoRefreshKvStore(markerMtimeMs);
}

void MediaLibraryKvStoreManager::DoRefreshKvStore(int64_t markerMtimeMs)
{
    std::lock_guard<std::mutex> lock(mutex_);
    kvStoreMap_.Clear();
    clientOpenKvdbTime_.store(markerMtimeMs);
    MEDIA_INFO_LOG("KVDB refreshed after swap, markerMtimeMs %{public}lld", static_cast<long long>(markerMtimeMs));
}

void MediaLibraryKvStoreManager::CloseAllKvStore()
{
    std::lock_guard<std::mutex> lock(mutex_);
#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
    // Keep retained handles tracked so account deletion can detect in-flight work.
    for (auto it = accountStores_.begin(); it != accountStores_.end();) {
        it = it->second.use_count() == 1 && it->second->Close() ? accountStores_.erase(it) : std::next(it);
    }
#endif
    if (kvStoreMap_.IsEmpty()) {
        return;
    }

    kvStoreMap_.Clear();
}

bool MediaLibraryKvStoreManager::ForceCloseAllKvStore()
{
    std::lock_guard<std::mutex> lock(mutex_);
    bool allClosed = true;
    kvStoreMap_.Iterate([&allClosed](const KvStoreValueType &key, KvStoreSharedPtr &ptr) {
        if (ptr != nullptr) {
            if (ptr->Close()) {
                MEDIA_INFO_LOG("ForceCloseAllKvStore closed kvstore, type=%{public}d", static_cast<int32_t>(key));
            } else {
                MEDIA_ERR_LOG("ForceCloseAllKvStore failed to close kvstore, type=%{public}d",
                    static_cast<int32_t>(key));
                allClosed = false;
            }
        }
    });
    kvStoreMap_.Clear();
    return allClosed;
}

void MediaLibraryKvStoreManager::TryCloseAllKvStore()
{
#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (MediaFileUtils::UTCTimeMilliSeconds() - kvStoreEvokedTimeStamp_.load() >
            static_cast<int64_t>(CLOSE_KVSTORE_TIME_INTERVAL)) {
            for (auto it = accountStores_.begin(); it != accountStores_.end();) {
                it = it->second.use_count() == 1 && it->second->Close() ?
                    accountStores_.erase(it) : std::next(it);
            }
        }
    }
#endif
    int64_t kvIdleTime = MediaFileUtils::UTCTimeMilliSeconds() - kvStoreEvokedTimeStamp_.load();
    if (!kvStoreMap_.IsEmpty() && kvIdleTime > static_cast<int64_t>(CLOSE_KVSTORE_TIME_INTERVAL)) {
        std::lock_guard<std::mutex> lock(mutex_);
        kvStoreMap_.Clear();
    }
}

bool MediaLibraryKvStoreManager::CloseKvStore(const KvStoreValueType &valueType)
{
#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
    MediaLibraryKvStoreRoute route;
    CHECK_AND_RETURN_RET(ResolveRoute(route), false);
    if (route.multiAccountEnabled) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = accountStores_.find({route.accountId, KvStoreRoleType::OWNER, valueType});
        CHECK_AND_RETURN_RET(it != accountStores_.end() && it->second.use_count() == 1, false);
        CHECK_AND_RETURN_RET(it->second->Close(), false);
        accountStores_.erase(it);
        return true;
    }
#endif
    std::lock_guard<std::mutex> lock(mutex_);
    KvStoreSharedPtr ptr;
    if (!kvStoreMap_.Find(valueType, ptr)) {
        return false;
    }

    if (ptr != nullptr && ptr->Close()) {
        kvStoreMap_.Erase(valueType);
        MEDIA_INFO_LOG("CloseKvStore success, valueType %{public}d", valueType);
        return true;
    }
    return false;
}

bool MediaLibraryKvStoreManager::InitMonthAndYearKvStore(const KvStoreRoleType& roleType)
{
#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
    MediaLibraryKvStoreRoute route;
    CHECK_AND_RETURN_RET(ResolveRoute(route), false);
    if (route.multiAccountEnabled) {
        CHECK_AND_RETURN_RET(roleType == KvStoreRoleType::OWNER, false);
        std::lock_guard<std::mutex> lock(mutex_);
        return GetAccountStoreLocked(roleType, KvStoreValueType::MONTH_ASTC, route) != nullptr &&
            GetAccountStoreLocked(roleType, KvStoreValueType::YEAR_ASTC, route) != nullptr;
    }
#endif
    if (roleType != KvStoreRoleType::OWNER) {
        return false;
    }
    if (GetKvStore(roleType, KvStoreValueType::MONTH_ASTC) == nullptr ||
        GetKvStore(roleType, KvStoreValueType::YEAR_ASTC) == nullptr) {
        return false;
    }
    return true;
}

bool MediaLibraryKvStoreManager::IsKvStoreValid(const KvStoreValueType &valueType)
{
#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
    MediaLibraryKvStoreRoute route;
    CHECK_AND_RETURN_RET(ResolveRoute(route), false);
    if (route.multiAccountEnabled) {
        std::lock_guard<std::mutex> lock(mutex_);
        CHECK_AND_RETURN_RET(deletingAccounts_.count(route.accountId) == 0, false);
        const auto id = MediaLibraryKvStore::GetStoreId(valueType, route);
        CHECK_AND_RETURN_RET(!id.empty(), false);
        if (accountStores_.count({route.accountId, KvStoreRoleType::OWNER, valueType}) != 0) {
            return true;
        }
        auto store = std::make_shared<MediaLibraryKvStore>();
        auto status = store->InitSingleKvstore(KvStoreRoleType::OWNER, id, KV_STORE_OWNER_DIR);
        return status != static_cast<int32_t>(Status::DATA_CORRUPTED);
    }
#endif
    KvStoreSharedPtr ptr;
    CHECK_AND_RETURN_RET(!kvStoreMap_.Find(valueType, ptr), true);

    ptr = std::make_shared<MediaLibraryKvStore>();
    int32_t status = ptr->Init(KvStoreRoleType::OWNER, valueType, KV_STORE_OWNER_DIR);
    CHECK_AND_RETURN_RET_LOG(status != static_cast<int32_t>(Status::DATA_CORRUPTED), false,
        "KvStore is invalid and needs to be deleted, status %{public}d, type %{public}d",
        status, valueType);

    if (status == E_OK && ptr != nullptr) {
        ptr->Close();
    }
    return true;
}

int32_t MediaLibraryKvStoreManager::RebuildInvalidKvStore(const KvStoreValueType &valueType)
{
#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
    MediaLibraryKvStoreRoute route;
    CHECK_AND_RETURN_RET(ResolveRoute(route), E_ERR);
    if (route.multiAccountEnabled) {
        std::lock_guard<std::mutex> lock(mutex_);
        CHECK_AND_RETURN_RET(deletingAccounts_.count(route.accountId) == 0, E_ERR);
        // A rebuild is not allowed to invalidate retained task/reader handles.
        for (const auto role : {KvStoreRoleType::OWNER, KvStoreRoleType::VISITOR}) {
            auto it = accountStores_.find({route.accountId, role, valueType});
            CHECK_AND_RETURN_RET(it == accountStores_.end() || it->second.use_count() == 1, E_ERR);
        }
        for (const auto role : {KvStoreRoleType::OWNER, KvStoreRoleType::VISITOR}) {
            auto it = accountStores_.find({route.accountId, role, valueType});
            if (it != accountStores_.end()) {
                CHECK_AND_RETURN_RET(it->second->Close(), E_ERR);
                accountStores_.erase(it);
            }
        }
        MediaLibraryKvStore store;
        return store.RebuildKvStore(valueType, KV_STORE_OWNER_DIR, route);
    }
#endif
    std::lock_guard<std::mutex> lock(mutex_);
    kvStoreMap_.Erase(valueType);
    KvStoreSharedPtr ptr = std::make_shared<MediaLibraryKvStore>();
    return ptr->RebuildKvStore(valueType, KV_STORE_OWNER_DIR);
}

std::shared_ptr<MediaLibraryKvStore> MediaLibraryKvStoreManager::GetSingleKvStore(
    const KvStoreRoleType &roleType, const std::string &storeId, const std::string &baseDir)
{
    KvStoreSharedPtr ptr = std::make_shared<MediaLibraryKvStore>();
    int32_t status = ptr->InitSingleKvstore(roleType, storeId, baseDir);
    CHECK_AND_RETURN_RET_LOG(status == E_OK, nullptr, "Init kvStore failed, status %{public}d", status);
    return ptr;
}

int32_t MediaLibraryKvStoreManager::CloneKvStore(const std::string &oldKvStoreId, const std::string &oldBaseDir,
    const std::string &newKvStoreId, const std::string &newBaseDir)
{
    KvStoreSharedPtr oldKvStore = std::make_shared<MediaLibraryKvStore>();
    int32_t status = oldKvStore->InitSingleKvstore(KvStoreRoleType::OWNER, oldKvStoreId, oldBaseDir);
    CHECK_AND_RETURN_RET_LOG(status == E_OK, status, "Init old kvStore failed, status %{public}d", status);

    KvStoreSharedPtr newKvStore = std::make_shared<MediaLibraryKvStore>();
    status = newKvStore->InitSingleKvstore(KvStoreRoleType::OWNER, newKvStoreId, newBaseDir);
    CHECK_AND_RETURN_RET_LOG(status == E_OK, status, "Init new kvStore failed, status %{public}d", status);

    status = oldKvStore->PutAllValueToNewKvStore(newKvStore);
    CHECK_AND_RETURN_RET_LOG(status == E_OK, status, "Clone kvstore failed, status %{public}d", status);
    return E_OK;
}

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
bool MediaLibraryKvStoreManager::ResolveRoute(MediaLibraryKvStoreRoute &route)
{
    route = {};
    int32_t foreground = INVALID_ACCOUNT_ID;
    if (!MediaPathUtils::GetForegroundAccountId(foreground)) {
        return true;
    }
    route.multiAccountEnabled = true;
    route.accountId = AccountScope::Get();
    if (route.accountId == INVALID_ACCOUNT_ID) {
        route.accountId = foreground;
    }
    CHECK_AND_RETURN_RET_LOG(route.accountId > 0, false, "Invalid KV account route");
    return true;
}

KvStoreSharedPtr MediaLibraryKvStoreManager::GetAccountStoreLocked(const KvStoreRoleType &roleType,
    const KvStoreValueType &valueType, const MediaLibraryKvStoreRoute &route)
{
    if (roleType != KvStoreRoleType::OWNER && roleType != KvStoreRoleType::VISITOR) {
        return nullptr;
    }
    const auto storeId = MediaLibraryKvStore::GetStoreId(valueType, route);
    CHECK_AND_RETURN_RET(!storeId.empty(), nullptr);
    const int32_t accountId = route.multiAccountEnabled ? route.accountId : INVALID_ACCOUNT_ID;
    CHECK_AND_RETURN_RET(deletingAccounts_.count(accountId) == 0, nullptr);
    AccountStoreKey key {accountId, roleType, valueType};
    auto found = accountStores_.find(key);
    if (found != accountStores_.end()) {
        return found->second;
    }
    auto store = std::make_shared<MediaLibraryKvStore>();
    auto ret = store->InitSingleKvstore(roleType, storeId, KV_STORE_OWNER_DIR);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, nullptr, "KV open failed, accountId=%{public}d, error=%{public}d",
        accountId, ret);
    accountStores_.emplace(key, store);
    MEDIA_INFO_LOG("KV store opened, accountId=%{public}d, role=%{public}d, type=%{public}d, storeId=%{public}s",
        accountId, static_cast<int32_t>(roleType), static_cast<int32_t>(valueType), storeId.c_str());
    return store;
}

std::shared_ptr<MediaLibraryKvStore> MediaLibraryKvStoreManager::GetKvStore(const KvStoreRoleType &roleType,
    const KvStoreValueType &valueType, const MediaLibraryKvStoreRoute &route)
{
    kvStoreEvokedTimeStamp_.store(MediaFileUtils::UTCTimeMilliSeconds());
    std::lock_guard<std::mutex> lock(mutex_);
    return GetAccountStoreLocked(roleType, valueType, route);
}

bool MediaLibraryKvStoreManager::InitAccountKvStores(int32_t accountId)
{
    CHECK_AND_RETURN_RET(accountId > 0, false);
    const MediaLibraryKvStoreRoute route {true, accountId};
    std::lock_guard<std::mutex> lock(mutex_);
    if (deletedAccounts_.erase(accountId) != 0) {
        deletingAccounts_.erase(accountId);
    }
    return GetAccountStoreLocked(KvStoreRoleType::OWNER, KvStoreValueType::MONTH_ASTC, route) != nullptr &&
        GetAccountStoreLocked(KvStoreRoleType::OWNER, KvStoreValueType::YEAR_ASTC, route) != nullptr;
}

std::shared_ptr<MediaLibraryKvStore> MediaLibraryKvStoreManager::GetKvStoreForPath(const KvStoreValueType &valueType,
    const std::string &path)
{
    MediaLibraryKvStoreRoute route;
    CHECK_AND_RETURN_RET(ResolveRoute(route), nullptr);
    if (!route.multiAccountEnabled) {
        return GetKvStore(KvStoreRoleType::OWNER, valueType);
    }
    CHECK_AND_RETURN_RET_LOG(MediaPathUtils::CheckAccountId(path, route.accountId), nullptr,
        "KV asset path/account mismatch, accountId=%{public}d", route.accountId);
    return GetKvStore(KvStoreRoleType::OWNER, valueType, route);
}

bool MediaLibraryKvStoreManager::CloseAccountStoresLocked(int32_t accountId)
{
    for (const auto &[key, store] : accountStores_) {
        if (std::get<0>(key) == accountId && store.use_count() > 1) {
            return false;
        }
    }
    for (auto it = accountStores_.begin(); it != accountStores_.end();) {
        if (std::get<0>(it->first) != accountId) {
            ++it;
            continue;
        }
        CHECK_AND_RETURN_RET(it->second->Close(), false);
        it = accountStores_.erase(it);
    }
    return true;
}

int32_t MediaLibraryKvStoreManager::DeleteAccountKvStores(const MediaLibraryKvStoreRoute &route)
{
    CHECK_AND_RETURN_RET(route.multiAccountEnabled && route.accountId > 0, E_ERR);
    std::lock_guard<std::mutex> lock(mutex_);
    deletingAccounts_.insert(route.accountId);
    bool closed = CloseAccountStoresLocked(route.accountId);
    if (!closed) {
        deletingAccounts_.erase(route.accountId);
        MEDIA_ERR_LOG("Cannot close account stores, accountId=%{public}d", route.accountId);
        return E_ERR;
    }
    for (const auto valueType : {KvStoreValueType::MONTH_ASTC, KvStoreValueType::YEAR_ASTC}) {
        MediaLibraryKvStore store;
        store.RebuildKvStore(valueType, KV_STORE_OWNER_DIR, route);
    }
    deletedAccounts_.insert(route.accountId);
    deletingAccounts_.erase(route.accountId);
    return E_OK;
}
#endif
} // namespace OHOS::Media
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
#include "medialibrary_errno.h"
#include "media_file_utils.h"
#include "media_log.h"

using namespace OHOS::DistributedKv;
namespace OHOS::Media {

std::shared_ptr<MediaLibraryKvStore> MediaLibraryKvStoreManager::InitKvStore(const KvStoreRoleType &roleType,
    const KvStoreValueType &valueType)
{
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
    int64_t currentTimestamp = MediaFileUtils::UTCTimeMilliSeconds();
    kvStoreEvokedTimeStamp_.store(currentTimestamp);
    KvStoreSharedPtr ptr;
    CHECK_AND_RETURN_RET(!kvStoreMap_.Find(valueType, ptr), ptr);

    return InitKvStore(roleType, valueType);
}

void MediaLibraryKvStoreManager::CloseAllKvStore()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (kvStoreMap_.IsEmpty()) {
        return;
    }

    kvStoreMap_.Clear();
}

void MediaLibraryKvStoreManager::TryCloseAllKvStore()
{
    int64_t kvIdleTime = MediaFileUtils::UTCTimeMilliSeconds() - kvStoreEvokedTimeStamp_.load();
    if (!kvStoreMap_.IsEmpty() && kvIdleTime > static_cast<int64_t>(CLOSE_KVSTORE_TIME_INTERVAL)) {
        std::lock_guard<std::mutex> lock(mutex_);
        kvStoreMap_.Clear();
    }
}

bool MediaLibraryKvStoreManager::CloseKvStore(const KvStoreValueType &valueType)
{
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
} // namespace OHOS::Media
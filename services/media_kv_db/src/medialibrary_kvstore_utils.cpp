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

#include "medialibrary_kvstore_utils.h"

#include "medialibrary_errno.h"
#include "medialibrary_kvstore_manager.h"
#include "media_log.h"

namespace OHOS::Media {
int32_t MediaLibraryKvStoreUtils::CopyAstcDataToKvStoreByType(const KvStoreValueType &type, const std::string &oldKey,
    const std::string &newKey)
{
    std::shared_ptr<MediaLibraryKvStore> kvStore;
    switch (type)  {
        case KvStoreValueType::MONTH_ASTC:
            kvStore = MediaLibraryKvStoreManager::GetInstance()
                .GetKvStore(KvStoreRoleType::OWNER, KvStoreValueType::MONTH_ASTC);
            break;
        case KvStoreValueType::YEAR_ASTC:
            kvStore = MediaLibraryKvStoreManager::GetInstance()
                .GetKvStore(KvStoreRoleType::OWNER, KvStoreValueType::YEAR_ASTC);
            break;
        default:
            MEDIA_ERR_LOG("Invalid thumbnailType");
            return E_ERR;
    }

    if (kvStore == nullptr) {
        MEDIA_ERR_LOG("KvStore is nullptr");
        return E_ERR;
    }

    std::vector<uint8_t> value;
    int32_t ret = kvStore->Query(oldKey, value);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Query failed, type:%{public}d, field_id:%{public}s, ret:%{public}d", type, oldKey.c_str(), ret);
        return E_ERR;
    }

    ret = kvStore->Insert(newKey, value);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Insert failed,type:%{public}d, field_id:%{public}s, ret:%{public}d", type, newKey.c_str(), ret);
        return E_ERR;
    }
    MEDIA_INFO_LOG("Success to save astc data, type:%{public}d, oldKey:%{public}s, newKey:%{public}s", type,
        oldKey.c_str(), newKey.c_str());
    return ret;
}
} // namespace OHOS::Media
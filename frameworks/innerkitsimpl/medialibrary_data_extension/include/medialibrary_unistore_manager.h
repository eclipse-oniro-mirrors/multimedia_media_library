/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIALIBRARY_UNISTORE_MANAGER_H
#define OHOS_MEDIALIBRARY_UNISTORE_MANAGER_H

#include <atomic>
#include <memory>
#include <thread>

#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore.h"
#include "settings_data_manager.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class MediaLibraryUnistoreManager {
public:
    EXPORT static MediaLibraryUnistoreManager &GetInstance()
    {
        static MediaLibraryUnistoreManager instance;
        return instance;
    }

    EXPORT int32_t Init(const std::shared_ptr<OHOS::AbilityRuntime::Context> &context)
    {
        if (std::atomic_load(&rdbStorePtr_) != nullptr) {
            return E_OK;
        }
        auto store = std::make_shared<MediaLibraryRdbStore>(context);
        if (!store) {
            MEDIA_ERR_LOG("create rdbStore failed");
            return E_ERR;
        }
        int32_t ret = store->Init();
        if (ret != E_OK) {
            return ret;
        }
        std::atomic_store(&rdbStorePtr_, store);
        return E_OK;
    }

    EXPORT int32_t Init(const std::shared_ptr<OHOS::AbilityRuntime::Context> &context,
        const NativeRdb::RdbStoreConfig &config, int version, NativeRdb::RdbOpenCallback &openCallback)
    {
        if (std::atomic_load(&rdbStorePtr_) != nullptr) {
            return E_OK;
        }
        auto store = std::make_shared<MediaLibraryRdbStore>(context);
        if (!store) {
            MEDIA_ERR_LOG("create rdbStore failed");
            return E_ERR;
        }
        int32_t ret = store->Init(config, version, openCallback);
        if (ret != E_OK) {
            return ret;
        }
        std::atomic_store(&rdbStorePtr_, store);
        return E_OK;
    }

    EXPORT void Stop()
    {
        auto old = std::atomic_load(&rdbStorePtr_);
        if (old) {
            old->Stop();
        }
        std::atomic_store(&rdbStorePtr_, std::shared_ptr<MediaLibraryRdbStore>());
    }

    EXPORT void CloseDatabase(bool async, int32_t timeOut)
    {
        bool expected = false;
        if (!isClosing_.compare_exchange_strong(expected, true)) {
            MEDIA_INFO_LOG("CloseDatabase already in progress, skip");
            return;
        }
        MEDIA_INFO_LOG("CloseDatabase started");
        bool closeReady = false;

        auto old = std::atomic_load(&rdbStorePtr_);
        if (old) {
            const auto &config = old->GetConfig();
            int32_t ret = old->Close(timeOut);
            if (ret == E_OK) {
                std::atomic_store(&rdbStorePtr_, std::shared_ptr<MediaLibraryRdbStore>());
                NativeRdb::RdbHelper::ClearStoreCache(config);
                closeReady = true;
            } else {
                MEDIA_ERR_LOG("CloseDatabase: Close failed, ret=%{public}d", ret);
            }
        }

        if (async && closeReady) {
            const std::string CLOSE_READY = "0";
            int32_t ret = SettingsDataManager::SetCloseDatabaseStatus(CLOSE_READY);
            if (ret != E_OK) {
                MEDIA_ERR_LOG("CloseDatabaseAsync: SetCloseDatabaseStatus failed, ret=%{public}d", ret);
            } else {
                MEDIA_INFO_LOG("CloseDatabaseAsync: SetCloseDatabaseStatus success");
            }
        }
        isClosing_ = false;
    }

    EXPORT std::shared_ptr<MediaLibraryRdbStore> GetRdbStore() const
    {
        auto store = std::atomic_load(&rdbStorePtr_);
        if (store != nullptr && store->CheckRdbStore()) {
            return store;
        }
        MEDIA_ERR_LOG("MediaLibraryRdbStore or rdbStore is nullptr");
        return nullptr;
    }

private:
    MediaLibraryUnistoreManager() = default;
    virtual ~MediaLibraryUnistoreManager() = default;

    mutable std::shared_ptr<MediaLibraryRdbStore> rdbStorePtr_;
    std::atomic<bool> isClosing_{false};
};
} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIALIBRARY_UNISTORE_MANAGER_H

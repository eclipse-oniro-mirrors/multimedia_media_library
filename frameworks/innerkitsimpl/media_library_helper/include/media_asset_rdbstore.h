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

#ifndef FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_INCLUDE_MEDIA_ASSET_RDBSTORE_H
#define FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_INCLUDE_MEDIA_ASSET_RDBSTORE_H

#include "application_context.h"
#include "datashare_helper.h"
#include "datashare_predicates.h"
#include "medialibrary_db_const.h"
#include "medialibrary_operation.h"
#include "rdb_errno.h"
#include "rdb_helper.h"
#include "rdb_open_callback.h"
#include "rdb_sql_utils.h"
#include "rdb_store.h"
#include "rdb_store_config.h"
#include "rdb_types.h"
#include "rdb_utils.h"
#include <mutex>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
#define COMPILE_HIDDEN __attribute__ ((visibility ("hidden")))

class MediaAssetRdbStore {
public:
    ~MediaAssetRdbStore() = default;
    EXPORT static MediaAssetRdbStore* GetInstance();
    EXPORT std::shared_ptr<DataShare::DataShareResultSet> Query(const DataShare::DataSharePredicates& predicates,
        std::vector<std::string>& columns, OperationObject& object, int& errCode);
    EXPORT bool IsQueryAccessibleViaSandBox(Uri& uri, OperationObject& object,
        const DataShare::DataSharePredicates& predicates, bool isIgnoreSELinux = false);
    EXPORT bool IsSupportSharedAssetQuery(Uri& uri, OperationObject& object, bool isIgnoreSELinux = false);
    EXPORT std::shared_ptr<NativeRdb::ResultSet> QueryRdb(const DataShare::DataSharePredicates& predicates,
        std::vector<std::string>& columns, OperationObject& object);
    EXPORT int32_t QueryTimeIdBatch(int32_t start, int32_t count, std::vector<std::string> &batchKeys);
    EXPORT std::shared_ptr<NativeRdb::ResultSet> QueryByStep(const std::string &sql);

private:
    MediaAssetRdbStore();
    int32_t TryGetRdbStore(bool isIngnoreSELinux = false);
    EXPORT static const std::string CloudSyncTriggerFunc(const std::vector<std::string>& args);
    EXPORT static const std::string IsCallerSelfFunc(const std::vector<std::string>& args);
    EXPORT static const std::string PhotoAlbumNotifyFunc(const std::vector<std::string>& args);
    bool IsQueryGroupPhotoAlbumAssets(const std::string& albumId);
    std::shared_ptr<NativeRdb::RdbStore> rdbStore_ {nullptr};
    std::mutex mutex_;
};

class COMPILE_HIDDEN MediaLibraryDataCallBack : public NativeRdb::RdbOpenCallback {
public:
    int32_t OnCreate(NativeRdb::RdbStore& rdbStore) override;
    int32_t OnUpgrade(NativeRdb::RdbStore& rdbStore, int32_t oldVersion, int32_t newVersion) override;
};

} // Media
} // OHOS

#endif // FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_INCLUDE_MEDIA_ASSET_RDBSTORE_H
/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIALIBRARY_DATA_MANAGER_H
#define OHOS_MEDIALIBRARY_DATA_MANAGER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <shared_mutex>

#include "ability_context.h"
#include "cloud_thumbnail_observer.h"
#include "context/context.h"
#include "dir_asset.h"
#include "datashare_predicates.h"
#include "datashare_values_bucket.h"
#include "distributed_kv_data_manager.h"
#include "imedia_scanner_callback.h"
#include "medialibrary_command.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_helper_container.h"
#include "medialibrary_db_const.h"
#include "rdb_predicates.h"
#include "rdb_store.h"
#include "result_set_bridge.h"
#include "uri.h"
#include "values_bucket.h"
#include "thumbnail_service.h"
#include "bundle_mgr_interface.h"

#define EXPORT __attribute__ ((visibility ("default")))

namespace OHOS {
namespace AbilityRuntime {
class MediaDataShareExtAbility;
}
namespace Media {
using OHOS::AbilityRuntime::MediaDataShareExtAbility;
class MediaLibraryDataManager {
public:
    EXPORT MediaLibraryDataManager();
    EXPORT ~MediaLibraryDataManager();
    EXPORT static MediaLibraryDataManager* GetInstance();

    EXPORT int32_t Insert(MediaLibraryCommand &cmd, const DataShare::DataShareValuesBucket &value);
    EXPORT int32_t InsertExt(MediaLibraryCommand &cmd, const DataShare::DataShareValuesBucket &value,
        std::string &result);
    EXPORT int32_t Delete(MediaLibraryCommand &cmd, const DataShare::DataSharePredicates &predicates);
    EXPORT int32_t BatchInsert(MediaLibraryCommand &cmd,
        const std::vector<DataShare::DataShareValuesBucket> &values);
    EXPORT int32_t Update(MediaLibraryCommand &cmd, const DataShare::DataShareValuesBucket &value,
        const DataShare::DataSharePredicates &predicates);
    EXPORT std::shared_ptr<DataShare::ResultSetBridge> Query(MediaLibraryCommand &cmd,
        const std::vector<std::string> &columns, const DataShare::DataSharePredicates &predicates, int &errCode);
    EXPORT std::shared_ptr<NativeRdb::ResultSet>
    EXPORT QueryRdb(MediaLibraryCommand &cmd, const std::vector<std::string> &columns,
        const DataShare::DataSharePredicates &predicates, int &errCode);
    EXPORT int32_t OpenFile(MediaLibraryCommand &cmd, const std::string &mode);
    EXPORT std::string GetType(const Uri &uri);
    EXPORT void NotifyChange(const Uri &uri);
    EXPORT int32_t GenerateThumbnails();
    void InterruptBgworker();
    EXPORT int32_t DoAging();
    EXPORT int32_t DoTrashAging(std::shared_ptr<int> countPtr = nullptr);
    /**
     * @brief Revert the pending state through the package name
     * @param bundleName packageName
     * @return revert result
     */
    EXPORT int32_t RevertPendingByPackage(const std::string &bundleName);

    EXPORT std::shared_ptr<NativeRdb::RdbStore> rdbStore_;

    EXPORT int32_t InitMediaLibraryMgr(const std::shared_ptr<OHOS::AbilityRuntime::Context> &context,
        const std::shared_ptr<OHOS::AbilityRuntime::Context> &extensionContext);
    EXPORT void ClearMediaLibraryMgr();
    EXPORT int32_t MakeDirQuerySetMap(std::unordered_map<std::string, DirAsset> &outDirQuerySetMap);
    EXPORT void CreateThumbnailAsync(const std::string &uri, const std::string &path);
    EXPORT static std::unordered_map<std::string, DirAsset> GetDirQuerySetMap();
    EXPORT std::shared_ptr<MediaDataShareExtAbility> GetOwner();
    EXPORT void SetOwner(const std::shared_ptr<MediaDataShareExtAbility> &datashareExtension);
    EXPORT int GetThumbnail(const std::string &uri);
    int32_t GetAgingDataSize(const int64_t &time, int &count);
    int32_t QueryNewThumbnailCount(const int64_t &time, int &count);
    EXPORT void SetStartupParameter();

private:
    int32_t InitMediaLibraryRdbStore();

#ifdef DISTRIBUTED
    bool QuerySync(const std::string &networkId, const std::string &tableName);
#endif
    int32_t HandleThumbnailOperations(MediaLibraryCommand &cmd);

    EXPORT int32_t SolveInsertCmd(MediaLibraryCommand &cmd);
    EXPORT int32_t SetCmdBundleAndDevice(MediaLibraryCommand &outCmd);
#ifdef DISTRIBUTED
    int32_t InitDeviceData();
#endif
    EXPORT int32_t InitialiseThumbnailService(const std::shared_ptr<OHOS::AbilityRuntime::Context> &extensionContext);
    std::shared_ptr<NativeRdb::ResultSet> QuerySet(MediaLibraryCommand &cmd, const std::vector<std::string> &columns,
        const DataShare::DataSharePredicates &predicates, int &errCode);
    void InitACLPermission();
    void InitDatabaseACLPermission();
    std::shared_ptr<NativeRdb::ResultSet> QueryInternal(MediaLibraryCommand &cmd,
        const std::vector<std::string> &columns, const DataShare::DataSharePredicates &predicates);
#ifdef DISTRIBUTED
    int32_t LcdDistributeAging();
    int32_t DistributeDeviceAging();
#endif
    EXPORT std::shared_ptr<ThumbnailService> thumbnailService_;
    int32_t RevertPendingByFileId(const std::string &fileId);
#ifdef DISTRIBUTED
    int32_t SyncPullThumbnailKeys(const Uri &uri);
#endif
    int32_t DeleteInRdbPredicates(MediaLibraryCommand &cmd, NativeRdb::RdbPredicates &rdbPredicate);
    int32_t DeleteInRdbPredicatesAnalysis(MediaLibraryCommand &cmd, NativeRdb::RdbPredicates &rdbPredicate);
    int32_t UpdateInternal(MediaLibraryCommand &cmd, NativeRdb::ValuesBucket &value,
        const DataShare::DataSharePredicates &predicates);
    void InitRefreshAlbum();
    std::shared_mutex mgrSharedMutex_;
#ifdef DISTRIBUTED
    std::shared_ptr<DistributedKv::SingleKvStore> kvStorePtr_;
    DistributedKv::DistributedKvDataManager dataManager_;
#endif
    std::shared_ptr<OHOS::AbilityRuntime::Context> context_;
    std::string bundleName_ {BUNDLE_NAME};
    static std::mutex mutex_;
    static std::unique_ptr<MediaLibraryDataManager> instance_;
    static std::unordered_map<std::string, DirAsset> dirQuerySetMap_;
    std::atomic<int> refCnt_ {0};
    std::shared_ptr<MediaDataShareExtAbility> extension_;
    std::shared_ptr<CloudThumbnailObserver> cloudDataObserver_;
};

// Scanner callback objects
class ScanFileCallback : public IMediaScannerCallback {
public:
    ScanFileCallback() = default;
    ~ScanFileCallback() = default;
    int32_t OnScanFinished(const int32_t status, const std::string &uri, const std::string &path) override;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_DATA_ABILITY_H

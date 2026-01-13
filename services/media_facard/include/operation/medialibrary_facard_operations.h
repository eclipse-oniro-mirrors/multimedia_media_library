/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
 
#ifndef MEDIALIBRARY_FACARD_OPERATIONS_H
#define MEDIALIBRARY_FACARD_OPERATIONS_H
 
#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>
#include <mutex>
 
#include "abs_shared_result_set.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_command.h"
#include "rdb_predicates.h"
#include "datashare_helper.h"
#include "data_ability_observer_stub.h"
#include "rdb_predicates.h"
#include <unordered_set>
#include "event_handler.h"
 
namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
using namespace OHOS::NativeRdb;
class MediaLibraryFaCardOperations : public MediaLibraryAssetOperations {
public:
    EXPORT static int32_t HandleStoreGalleryFormOperation(MediaLibraryCommand &cmd);
    EXPORT static int32_t HandleRemoveGalleryFormOperation(NativeRdb::RdbPredicates &rdbPredicate);
    EXPORT static void RegisterObserver(const std::string &formId, const std::string &registerUri);
    EXPORT static void UnregisterObserver(const std::string &formId);
    EXPORT static std::map<std::string, std::vector<std::string>> GetUris();
    EXPORT static void InitRegisterObserver();
    EXPORT static void InitFaCard();
    
private:
    EXPORT static std::mutex mutex_;
};

class FaCloudSyncSwitchObserver : public AAFwk::DataAbilityObserverStub {
public:
    explicit FaCloudSyncSwitchObserver(const std::string &cloudSyncChangeUri) :
    cloudSyncChangeUri(cloudSyncChangeUri) {}
    ~FaCloudSyncSwitchObserver() override = default;
    void PostAssetChangeTask();
    void OnChange() override;
    struct CloudSyncChangeInfo {
        std::string cloudSyncChangeUri;
        int cloudSyncChangeType;
        CloudSyncChangeInfo(const std::string& uri, int type)
            : cloudSyncChangeUri(uri),
              cloudSyncChangeType(type) {}
        bool operator==(const CloudSyncChangeInfo& other) const
        {
            return cloudSyncChangeUri == other.cloudSyncChangeUri && cloudSyncChangeType == other.cloudSyncChangeType;
        }
    };
    struct CloudSyncChangeInfoHash {
        std::size_t operator()(const CloudSyncChangeInfo& info) const
        {
            std::hash<std::string> hashStr;
            std::hash<int> hashInt;
            return hashStr(info.cloudSyncChangeUri) ^ (hashInt(info.cloudSyncChangeType) << 1);
        }
    };
    const std::string cloudSyncChangeUri;
    static std::unordered_set<CloudSyncChangeInfo, CloudSyncChangeInfoHash> cloudSyncChanges;
    static bool isTaskPosted;
    static std::mutex mtx;
};
 
class CardAssetUriObserver : public DataShare::DataShareObserver {
public:
    explicit CardAssetUriObserver(const std::string &assetChangeUri) : assetChangeUri(assetChangeUri) {}
    ~CardAssetUriObserver() override = default;
    void PostAssetChangeTask();
    void OnChange(const ChangeInfo &changeInfo) override;
    struct AssetChangeInfo {
        std::string assetChangeUri;
        int assetChangeType;
        AssetChangeInfo(const std::string& uri, int type)
            : assetChangeUri(uri),
              assetChangeType(type) {}
        bool operator==(const AssetChangeInfo& other) const
        {
            return assetChangeUri == other.assetChangeUri && assetChangeType == other.assetChangeType;
        }
    };
    struct AssetChangeInfoHash {
        std::size_t operator()(const AssetChangeInfo& info) const
        {
            std::hash<std::string> hashStr;
            std::hash<int> hashInt;
            return hashStr(info.assetChangeUri) ^ (hashInt(info.assetChangeType) << 1);
        }
    };
    const std::string assetChangeUri;
    static std::unordered_set<AssetChangeInfo, AssetChangeInfoHash> assetChanges;
    static bool isTaskPosted;
    static std::mutex mtx;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIALIBRARY_FORMMAP_OPERATIONS_H
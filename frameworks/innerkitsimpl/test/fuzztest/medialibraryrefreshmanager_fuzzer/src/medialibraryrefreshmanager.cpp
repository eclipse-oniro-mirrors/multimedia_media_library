/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "medialibraryrefreshmanager.h"

#include <fstream>
#include <fuzzer/FuzzedDataProvider.h>

#include "albums_refresh_manager.h"
#include "ability_context_impl.h"
#include "medialibrary_errno.h"
#include "medialibrary_kvstore_manager.h"
#include "medialibrary_type_const.h"
#include "medialibrary_tracer.h"
#include "media_log.h"
#include "result_set_utils.h"
#include "media_file_utils.h"
#include "media_file_uri.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_notify.h"
#include "userfilemgr_uri.h"
#include "media_refresh_album_column.h"
#include "albums_refresh_worker.h"
#include "albums_refresh_notify.h"
#include "rdb_sql_utils.h"
#include "result_set_utils.h"
#include "photo_album_column.h"
#include "medialibrary_rdb_utils.h"
#include "vision_column.h"
#include "medialibrary_restore.h"
#include "post_event_utils.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unistore.h"
#include "media_datashare_ext_ability.h"
#ifdef HAS_POWER_MANAGER_PART
#include "power_mgr_client.h"
#endif
#include "media_upgrade.h"

namespace OHOS {
using namespace std;
using namespace DataShare;
using namespace Media;
using ChangeType = DataShare::DataShareObserver::ChangeType;

std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;
static const int32_t NUM_BYTES = 8;
static const int32_t MAX_BYTE_VALUE = 256;
static const int32_t SEED_SIZE = 1024;
static const int32_t VAL_ONE = 1;
FuzzedDataProvider *provider = nullptr;

static inline Uri FuzzUri()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0,
        static_cast<uint8_t>(Media::EXTENSION_FUZZER_URI_LISTS.size() - VAL_ONE));
    return Uri(Media::EXTENSION_FUZZER_URI_LISTS[value]);
}

static inline std::list<Uri> FuzzListUri()
{
    std::list<Uri> uri = {FuzzUri()};
    return uri;
}

static inline std::vector<std::string> FuzzVector()
{
    return {provider->ConsumeBytesAsString(NUM_BYTES)};
}

static inline std::unordered_set<std::string> FuzzUnorderedSet()
{
    std::unordered_set<std::string> orderset = {provider->ConsumeBytesAsString(NUM_BYTES)};
    return orderset;
}

static inline Media::NotifyType FuzzNotifyTypeCause()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0,
        static_cast<int32_t>(Media::NotifyType::NOTIFY_INVALID));
    return static_cast<Media::NotifyType>(value);
}

static inline ChangeType FuzzChangeType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0,
        static_cast<int32_t>(ChangeType::OTHER));
    return static_cast<ChangeType>(value);
}

static inline Media::ForceRefreshType FuzzForceRefreshTypeCause()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0,
        static_cast<int32_t>(Media::ForceRefreshType::EXCEPTION));
    return static_cast<Media::ForceRefreshType>(value);
}

static inline Media::CloudSyncNotifyInfo FuzzCloudSyncNotifyInfo()
{
    Media::CloudSyncNotifyInfo info;
    const void* infodata = nullptr;
    ChangeType type = FuzzChangeType();
    info.uris = FuzzListUri();
    info.type = type;
    info.data = infodata;
    return info;
}

static Media::SyncNotifyInfo FuzzSyncNotifyInfo()
{
    Media::SyncNotifyInfo info;
    info.taskType = provider->ConsumeIntegralInRange<uint16_t>(0, TIME_IN_SYNC);
    info.syncType = provider->ConsumeIntegralInRange<uint16_t>(0, TIME_IN_SYNC);
    info.notifyType = FuzzNotifyTypeCause();
    info.syncId = provider->ConsumeBytesAsString(NUM_BYTES);
    info.totalAssets = provider->ConsumeIntegralInRange<uint32_t>(0, INT32_MAX);
    info.totalAlbums = provider->ConsumeIntegralInRange<uint32_t>(0, INT32_MAX);
    info.uriType = provider->ConsumeIntegralInRange<uint8_t>(0, OTHER_URI_TYPE);
    info.reserve = provider->ConsumeIntegralInRange<uint8_t>(0, TIME_IN_SYNC);
    info.urisSize = provider->ConsumeIntegralInRange<uint16_t>(0, INT16_MAX);
    info.uris = FuzzListUri();
    info.extraUris = FuzzListUri();
    info.uriIds = FuzzUnorderedSet();
    info.notifyAssets = provider->ConsumeBool();
    info.notifyAlbums = provider->ConsumeBool();
    info.refreshResult = provider->ConsumeIntegralInRange<uint32_t>(0, INT32_MAX);
    info.forceRefreshType = FuzzForceRefreshTypeCause();
    return info;
}

static void RefreshNotifyInfoTest()
{
    Media::SyncNotifyInfo fuzznotifyinfo = FuzzSyncNotifyInfo();
    Media::CloudSyncNotifyInfo fuzzsyncnotifyinfo = FuzzCloudSyncNotifyInfo();
    std::vector<std::string> fuzzvector = FuzzVector();
    Media::AlbumsRefreshManager &instance = Media::AlbumsRefreshManager::GetInstance();
    std::shared_ptr<Media::AlbumsRefreshWorker> refreshWorker = std::make_shared<Media::AlbumsRefreshWorker>();
    instance.RefreshPhotoAlbums(fuzznotifyinfo);
    instance.AddAlbumRefreshTask(fuzznotifyinfo);
    instance.NotifyPhotoAlbums(fuzznotifyinfo);
    instance.HasRefreshingSystemAlbums();
    instance.GetSyncNotifyInfo(fuzzsyncnotifyinfo,
        provider->ConsumeIntegralInRange<uint8_t>(0, OTHER_URI_TYPE));
    instance.CovertCloudId2AlbumId(g_rdbStore, fuzzvector);
    instance.CovertCloudId2FileId(g_rdbStore, fuzzvector);
    instance.RefreshPhotoAlbumsBySyncNotifyInfo(g_rdbStore, fuzznotifyinfo);
    refreshWorker->StartConsumerThread();
    refreshWorker->AddAlbumRefreshTask(fuzznotifyinfo);
    refreshWorker->GetSystemAlbumIds(fuzznotifyinfo, fuzzvector);
    refreshWorker->TryDeleteAlbum(fuzznotifyinfo, fuzzvector);
}

void SetTables()
{
    vector<string> createTableSqlList = { Media::PhotoUpgrade::CREATE_PHOTO_TABLE };
    for (auto &createTableSql : createTableSqlList) {
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
}

static void Init()
{
    auto stageContext = std::make_shared<AbilityRuntime::ContextImpl>();
    auto abilityContextImpl = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    abilityContextImpl->SetStageContext(stageContext);
    int32_t sceneCode = 0;
    auto ret = Media::MediaLibraryDataManager::GetInstance()->InitMediaLibraryMgr(abilityContextImpl,
        abilityContextImpl, sceneCode);
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK, "InitMediaLibraryMgr failed, ret: %{public}d", ret);

    auto rdbStore = Media::MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr");
        return;
    }
    g_rdbStore = rdbStore;
    SetTables();
}

static int32_t AddSeed()
{
    char *seedData = new char[OHOS::SEED_SIZE];
    for (int i = 0; i < OHOS::SEED_SIZE; i++) {
        seedData[i] = static_cast<char>(i % MAX_BYTE_VALUE);
    }

    const char* filename = "corpus/seed.txt";
    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file) {
        MEDIA_ERR_LOG("Cannot open file filename:%{public}s", filename);
        delete[] seedData;
        return Media::E_ERR;
    }
    file.write(seedData, OHOS::SEED_SIZE);
    file.close();
    delete[] seedData;
    MEDIA_INFO_LOG("seedData has been successfully written to file filename:%{public}s", filename);
    return Media::E_OK;
}

} // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::AddSeed();
    OHOS::Init();
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    OHOS::provider = &fdp;
    if (data == nullptr) {
        return 0;
    }
    /* Run your code on data */
    OHOS::RefreshNotifyInfoTest();
    OHOS::MediaLibraryKvStoreManager::GetInstance().CloseAllKvStore();
    return 0;
}
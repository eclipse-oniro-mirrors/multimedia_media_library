/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "medialibrarycloudmediadatahandler_fuzzer.h"

#include <cstdint>
#include <string>
#include <fuzzer/FuzzedDataProvider.h>
#define private public
#include "cloud_media_data_handler.h"
#undef private
#include "media_log.h"

#include "ability_context_impl.h"
#include "medialibrary_command.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "photo_album_column.h"
#include "medialibrary_kvstore_manager.h"
#include "media_upgrade.h"

namespace OHOS {
using namespace std;
using namespace OHOS::Media;
using namespace OHOS::Media::CloudSync;
const int32_t NUM_BYTES = 1;
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;
FuzzedDataProvider* provider;

static shared_ptr<CloudMediaDataHandler> InitCloudMediaDataHandlerFuzzer()
{
    MEDIA_INFO_LOG("InitCloudMediaDataHandlerFuzzer enter");
    string tableName = "PhotoAlbum";
    int32_t cloudType = provider->ConsumeIntegral<int32_t>();
    MEDIA_INFO_LOG("InitCloudMediaDataHandlerFuzzer: cloudType=%{public}d", cloudType);
    int32_t userId = provider->ConsumeIntegral<int32_t>();
    MEDIA_INFO_LOG("InitCloudMediaDataHandlerFuzzer: userId=%{public}d", userId);
    shared_ptr<CloudMediaDataHandler> dataHandler = make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    return dataHandler;
}

static void GetCheckRecordsFuzzer()
{
    shared_ptr<CloudMediaDataHandler> dataHandler = InitCloudMediaDataHandlerFuzzer();
    if (dataHandler == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return;
    }
    MEDIA_INFO_LOG("GetCheckRecordsFuzzer enter");

    vector<std::string> cloudIds = { provider->ConsumeBytesAsString(NUM_BYTES) };
    unordered_map<std::string, CloudCheckData> checkRecords;
    dataHandler->GetCheckRecords(cloudIds, checkRecords);
    dataHandler->dataHandler_ = nullptr;
    dataHandler->GetCheckRecords(cloudIds, checkRecords);
}

static void MediaLibraryICloudMediaDataHandlerFuzzer()
{
    shared_ptr<CloudMediaDataHandler> dataHandler = make_shared<CloudMediaDataHandler>();
    if (dataHandler == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return;
    }
    string traceId = provider->ConsumeBytesAsString(NUM_BYTES);
    dataHandler->SetTraceId(traceId);
    dataHandler->GetTraceId();

    vector<MDKRecord> records;
    int32_t recordSize = provider->ConsumeIntegral<int32_t>();
    dataHandler->GetCreatedRecords(records, recordSize);
    dataHandler->GetMetaModifiedRecords(records, recordSize);
    dataHandler->GetFileModifiedRecords(records, recordSize);
    dataHandler->GetDeletedRecords(records, recordSize);
    dataHandler->GetCopyRecords(records, recordSize);

    map<std::string, MDKRecordOperResult> map;
    int32_t failSize = 0;
    dataHandler->OnCreateRecords(map, failSize);
    dataHandler->OnMdirtyRecords(map, failSize);
    dataHandler->OnFdirtyRecords(map, failSize);
    dataHandler->OnDeleteRecords(map, failSize);
    dataHandler->OnCopyRecords(map, failSize);

    vector<std::string> failedRecords;
    vector<CloudMetaData> newData;
    vector<CloudMetaData> fdirtyData;
    vector<int32_t> stats;
    vector<std::string> strRecords;
    dataHandler->OnFetchRecords(records, newData, fdirtyData, failedRecords, stats);
    dataHandler->OnDentryFileInsert(records, failedRecords);
    dataHandler->GetRetryRecords(strRecords);

    MediaOperateResult optRet = {"", 0, ""};
    dataHandler->OnStartSync();
    dataHandler->OnCompleteSync();
    dataHandler->OnCompletePull(optRet);
    dataHandler->OnCompletePush();
    dataHandler->OnCompleteCheck();

    string tableName = "PhotoAlbum";
    dataHandler->SetTableName(tableName);
    dataHandler->GetTableName();
    dataHandler->SetTraceId(traceId);
    dataHandler->GetTraceId();
}

static void MediaLibraryCloudMediaDataHandlerFuzzer()
{
    std::shared_ptr<CloudMediaDataHandler> dataHandler = InitCloudMediaDataHandlerFuzzer();
    if (dataHandler == nullptr) {
        MEDIA_ERR_LOG("No data handler found!");
        return;
    }
    MEDIA_INFO_LOG("MediaLibraryCloudMediaDataHandlerFuzzer enter");
    dataHandler->SetCloudType(provider->ConsumeIntegral<int32_t>());
    dataHandler->GetCloudType();

    string tableName = "PhotoAlbum";
    dataHandler->SetTableName(tableName);
    dataHandler->GetTableName();

    dataHandler->SetUserId(provider->ConsumeIntegral<int32_t>());
    dataHandler->GetUserId();

    vector<MDKRecord> records;
    int32_t recordSize = provider->ConsumeIntegral<int32_t>();
    dataHandler->GetCreatedRecords(records, recordSize);
    dataHandler->GetMetaModifiedRecords(records, recordSize);
    dataHandler->GetFileModifiedRecords(records, recordSize);
    dataHandler->GetDeletedRecords(records, recordSize);
    dataHandler->GetCopyRecords(records, recordSize);

    map<std::string, MDKRecordOperResult> map;
    int32_t failSize = 0;
    dataHandler->OnCreateRecords(map, failSize);
    dataHandler->OnMdirtyRecords(map, failSize);
    dataHandler->OnFdirtyRecords(map, failSize);
    dataHandler->OnDeleteRecords(map, failSize);
    dataHandler->OnCopyRecords(map, failSize);

    vector<std::string> failedRecords;
    vector<CloudMetaData> newData;
    vector<CloudMetaData> fdirtyData;
    vector<int32_t> stats;
    vector<std::string> strRecords;
    dataHandler->OnFetchRecords(records, newData, fdirtyData, failedRecords, stats);
    dataHandler->OnDentryFileInsert(records, failedRecords);
    dataHandler->GetRetryRecords(strRecords);

    MediaOperateResult optRet = {"", 0, ""};
    dataHandler->OnStartSync();
    dataHandler->OnCompleteSync();
    dataHandler->OnCompletePull(optRet);
    dataHandler->OnCompletePush();
    dataHandler->OnCompleteCheck();
}

void SetTables()
{
    vector<string> createTableSqlList = {
        Media::PhotoUpgrade::CREATE_PHOTO_TABLE,
        Media::PhotoAlbumColumns::CREATE_TABLE,
    };
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

static inline void ClearKvStore()
{
    Media::MediaLibraryKvStoreManager::GetInstance().CloseAllKvStore();
}
} // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::Init();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    if (data == nullptr) {
        return 0;
    }
    OHOS::provider = &fdp;
    OHOS::MediaLibraryICloudMediaDataHandlerFuzzer();
    OHOS::InitCloudMediaDataHandlerFuzzer();
    OHOS::GetCheckRecordsFuzzer();
    OHOS::MediaLibraryCloudMediaDataHandlerFuzzer();
    OHOS::ClearKvStore();
    return 0;
}
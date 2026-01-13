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

#include "medialibrarycloudmediaphotohandler_fuzzer.h"

#include <cstdint>
#include <string>
#include <fuzzer/FuzzedDataProvider.h>
#define private public
#include "cloud_media_photo_handler.h"
#undef private
#include "media_log.h"

#include "ability_context_impl.h"
#include "medialibrary_command.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_kvstore_manager.h"
#include "media_upgrade.h"

namespace OHOS {
using namespace std;
using namespace OHOS::Media;
using namespace OHOS::Media::CloudSync;
const int32_t NUM_BYTES = 1;

std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;
FuzzedDataProvider* provider;

static void MediaLibraryCloudMediaPhotoHandlerFuzzer()
{
    CloudMediaPhotoHandler cloudMediaPhotoHandler;
    std::vector<std::string> cloudIds = { provider->ConsumeBytesAsString(NUM_BYTES)};
    std::vector<MDKRecord> records;
    std::unordered_map<std::string, CloudCheckData> checkRecords;
    int32_t recordsSize =  provider->ConsumeIntegral<int32_t>();
    std::string traceId = provider->ConsumeBytesAsString(NUM_BYTES);
    cloudMediaPhotoHandler.SetTraceId(traceId);
    cloudMediaPhotoHandler.GetTraceId();
    cloudMediaPhotoHandler.GetCheckRecords(cloudIds, checkRecords);
    cloudMediaPhotoHandler.GetCreatedRecords(records, recordsSize);
    cloudMediaPhotoHandler.GetMetaModifiedRecords(records, recordsSize);
    cloudMediaPhotoHandler.GetFileModifiedRecords(records, recordsSize);
    cloudMediaPhotoHandler.GetDeletedRecords(records, recordsSize);
    cloudMediaPhotoHandler.GetCopyRecords(records, recordsSize);
    std::map<std::string, MDKRecordOperResult> map;
    int32_t failSize = 0;
    cloudMediaPhotoHandler.OnCreateRecords(map, failSize);
    cloudMediaPhotoHandler.OnMdirtyRecords(map, failSize);
    cloudMediaPhotoHandler.OnFdirtyRecords(map, failSize);
    cloudMediaPhotoHandler.OnDeleteRecords(map, failSize);
    cloudMediaPhotoHandler.OnCopyRecords(map, failSize);

    std::vector<std::string> failedRecords;
    cloudMediaPhotoHandler.OnDentryFileInsert(records, failedRecords);
    std::vector<std::string> recordsVec;
    cloudMediaPhotoHandler.GetRetryRecords(recordsVec);
    cloudMediaPhotoHandler.OnStartSync();
    cloudMediaPhotoHandler.OnCompleteSync();
    MediaOperateResult optRet = {"", 0, ""};
    cloudMediaPhotoHandler.OnCompletePull(optRet);
    cloudMediaPhotoHandler.OnCompletePush();
    cloudMediaPhotoHandler.OnCompleteCheck();
}

void SetTables()
{
    vector<string> createTableSqlList = {
        Media::PhotoUpgrade::CREATE_PHOTO_TABLE,
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
    OHOS::MediaLibraryCloudMediaPhotoHandlerFuzzer();
    OHOS::ClearKvStore();
    return 0;
}
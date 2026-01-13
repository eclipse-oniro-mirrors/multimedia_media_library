/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "medialibraryappuripermissionoperations_fuzzer.h"

#include <cstdint>
#include <string>
#include <fstream>
#include <vector>
#include <fuzzer/FuzzedDataProvider.h>

#include "ability_context_impl.h"
#include "datashare_predicates.h"
#include "media_app_uri_permission_column.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_command.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_operation.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_unistore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_kvstore_manager.h"
#include "rdb_store.h"
#include "rdb_utils.h"
#include "userfile_manager_types.h"
#include "values_bucket.h"

#define private public
#include "medialibrary_app_uri_permission_operations.h"
#undef private
#include "media_upgrade.h"

namespace OHOS {
using namespace std;
using namespace DataShare;
const int32_t PERMISSION_DEFAULT = -1;
const int32_t URI_DEFAULT = 0;
const int32_t BatchInsertNumber = 5;
static const int32_t E_ERR = -1;
static const int32_t NUM_BYTES = 1;
static const int32_t MAX_PERMISSION_TYPE = 6;
static const int32_t MAX_URI_TYPE = 2;
static const int32_t MAX_DATA = 1;
static const int32_t MAX_BYTE_VALUE = 256;
static const int32_t SEED_SIZE = 1024;
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;
FuzzedDataProvider *FDP = nullptr;

static int FuzzPermissionType()
{
    vector<int> vecPermissionType;
    vecPermissionType.assign(Media::AppUriPermissionColumn::PERMISSION_TYPES_ALL.begin(),
        Media::AppUriPermissionColumn::PERMISSION_TYPES_ALL.end());
    vecPermissionType.push_back(PERMISSION_DEFAULT);
    uint8_t data = FDP->ConsumeIntegralInRange<uint8_t>(0, MAX_PERMISSION_TYPE);
    return vecPermissionType[data];
}

static int FuzzUriType()
{
    vector<int> vecUriType;
    vecUriType.assign(Media::AppUriPermissionColumn::URI_TYPES_ALL.begin(),
        Media::AppUriPermissionColumn::URI_TYPES_ALL.end());
    vecUriType.push_back(URI_DEFAULT);
    uint8_t data = FDP->ConsumeIntegralInRange<uint8_t>(0, MAX_URI_TYPE);
    return vecUriType[data];
}

static inline void FuzzMimeTypeAndDisplayNameExtension(string &mimeType, string &displayName)
{
    uint8_t data = FDP->ConsumeIntegralInRange<uint8_t>(0, MAX_DATA);
    mimeType = Media::MIMETYPE_FUZZER_LISTS[data];
    displayName = FDP->ConsumeBytesAsString(NUM_BYTES) + Media::DISPLAY_NAME_EXTENSION_FUZZER_LISTS[data];
}

static int32_t InsertPhotoAsset(string photoId)
{
    if (g_rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutString(Media::PhotoColumn::PHOTO_ID, photoId);
    values.PutString(Media::MediaColumn::MEDIA_FILE_PATH, FDP->ConsumeBytesAsString(NUM_BYTES));

    string mimeType = "undefined";
    string displayName = ".undefined";
    FuzzMimeTypeAndDisplayNameExtension(mimeType, displayName);
    values.PutString(Media::MediaColumn::MEDIA_NAME, displayName);
    values.PutString(Media::MediaColumn::MEDIA_MIME_TYPE, mimeType);

    int64_t fileId = 0;
    g_rdbStore->Insert(fileId, Media::PhotoColumn::PHOTOS_TABLE, values);
    return static_cast<int32_t>(fileId);
}

static void HandleInsertOperationFuzzer(string appId, int32_t photoId, int32_t permissionType, int32_t uriType)
{
    MEDIA_INFO_LOG("HandleInsertOperationFuzzer start");
    DataShareValuesBucket values;
    values.Put(Media::AppUriPermissionColumn::APP_ID, appId);
    values.Put(Media::AppUriPermissionColumn::FILE_ID, photoId);
    values.Put(Media::AppUriPermissionColumn::PERMISSION_TYPE, permissionType);
    values.Put(Media::AppUriPermissionColumn::URI_TYPE, uriType);
    Media::MediaLibraryCommand cmd(Media::OperationObject::MEDIA_APP_URI_PERMISSION, Media::OperationType::CREATE,
        Media::MediaLibraryApi::API_10);
    NativeRdb::ValuesBucket rdbValue = RdbDataShareAdapter::RdbUtils::ToValuesBucket(values);
    cmd.SetValueBucket(rdbValue);
    Media::MediaLibraryAppUriPermissionOperations::HandleInsertOperation(cmd);
    MEDIA_INFO_LOG("HandleInsertOperationFuzzer end");
}

static void DeleteOperationFuzzer(string appId, int32_t photoId, int32_t permissionType)
{
    MEDIA_INFO_LOG("DeleteOperationFuzzer start");
    DataSharePredicates predicates;
    predicates.And()->EqualTo(Media::AppUriPermissionColumn::APP_ID, appId);
    predicates.And()->EqualTo(Media::AppUriPermissionColumn::FILE_ID, photoId);
    predicates.And()->EqualTo(Media::AppUriPermissionColumn::PERMISSION_TYPE, permissionType);
    NativeRdb::RdbPredicates rdbPredicate = RdbDataShareAdapter::RdbUtils::ToPredicates(predicates,
        Media::AppUriPermissionColumn::APP_URI_PERMISSION_TABLE);
    Media::MediaLibraryAppUriPermissionOperations::DeleteOperation(rdbPredicate);
    MEDIA_INFO_LOG("DeleteOperationFuzzer end");
}

static void BatchInsertFuzzer()
{
    MEDIA_INFO_LOG("BatchInsertFuzzer start");
    vector<DataShare::DataShareValuesBucket> dataShareValues;
    for (int32_t i = 0; i < BatchInsertNumber; i++) {
        DataShareValuesBucket value;
        string photoId = FDP->ConsumeBytesAsString(NUM_BYTES);
        int32_t fileId = InsertPhotoAsset(photoId);
        string appId = FDP->ConsumeBytesAsString(NUM_BYTES);
        value.Put(Media::AppUriPermissionColumn::APP_ID, appId);
        value.Put(Media::AppUriPermissionColumn::FILE_ID, fileId);
        int32_t permissionType = FuzzPermissionType();
        value.Put(Media::AppUriPermissionColumn::PERMISSION_TYPE, permissionType);
        int32_t uriType = FuzzUriType();
        value.Put(Media::AppUriPermissionColumn::URI_TYPE, uriType);
        dataShareValues.push_back(value);
    }
    Media::MediaLibraryCommand cmd(Media::OperationObject::MEDIA_APP_URI_PERMISSION, Media::OperationType::CREATE,
        Media::MediaLibraryApi::API_10);
    Media::MediaLibraryAppUriPermissionOperations::BatchInsert(cmd, dataShareValues);
    MEDIA_INFO_LOG("BatchInsertFuzzer end");
}

static void BatchInsertInnerFuzzer()
{
    MEDIA_INFO_LOG("BatchInsertInnerFuzzer start");
    vector<DataShare::DataShareValuesBucket> dataShareValues;
    for (int32_t i = 0; i < BatchInsertNumber; i++) {
        DataShareValuesBucket value;
        string photoId = FDP->ConsumeBytesAsString(NUM_BYTES);
        int32_t fileId = InsertPhotoAsset(photoId);
        string appId = FDP->ConsumeBytesAsString(NUM_BYTES);
        value.Put(Media::AppUriPermissionColumn::APP_ID, appId);
        value.Put(Media::AppUriPermissionColumn::FILE_ID, fileId);
        int32_t permissionType = FuzzPermissionType();
        value.Put(Media::AppUriPermissionColumn::PERMISSION_TYPE, permissionType);
        int32_t uriType = FuzzUriType();
        value.Put(Media::AppUriPermissionColumn::URI_TYPE, uriType);
        value.Put(Media::AppUriPermissionColumn::DATE_MODIFIED, FDP->ConsumeIntegral<int64_t>());
        value.Put(Media::AppUriPermissionColumn::SOURCE_TOKENID, FDP->ConsumeIntegral<int64_t>());
        value.Put(Media::AppUriPermissionColumn::TARGET_TOKENID, FDP->ConsumeIntegral<int64_t>());
        dataShareValues.push_back(value);
    }
    Media::MediaLibraryCommand cmd(Media::OperationObject::MEDIA_APP_URI_PERMISSION, Media::OperationType::CREATE,
        Media::MediaLibraryApi::API_10);
    std::string funcName = FDP->ConsumeBytesAsString(NUM_BYTES);
    std::shared_ptr<Media::TransactionOperations> trans = std::make_shared<Media::TransactionOperations>(funcName);
    Media::MediaLibraryAppUriPermissionOperations::BatchInsertInner(cmd, dataShareValues, trans);
    MEDIA_INFO_LOG("BatchInsertInnerFuzzer end");
}

static void UpdatePermissionTypeFuzzer()
{
    MEDIA_INFO_LOG("UpdatePermissionTypeFuzzer start");
    NativeRdb::RdbPredicates predicates(Media::AppUriPermissionColumn::APP_URI_PERMISSION_TABLE);
    if (FDP->ConsumeBool()) {
        predicates.EqualTo(Media::AppUriPermissionColumn::FILE_ID, FDP->ConsumeIntegral<int64_t>());
    }
    predicates.EqualTo(Media::AppUriPermissionColumn::PERMISSION_TYPE, FuzzPermissionType());
    std::shared_ptr<OHOS::NativeRdb::ResultSet> resultSet = g_rdbStore->Query(predicates, {});
    CHECK_AND_RETURN_LOG(resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK, "failed to query");

    int permissionType = FuzzPermissionType();
    std::shared_ptr<Media::TransactionOperations> trans;
    if (FDP->ConsumeBool()) {
        trans = make_shared<Media::TransactionOperations>(FDP->ConsumeBytesAsString(NUM_BYTES));
    }
    Media::MediaLibraryAppUriPermissionOperations::UpdatePermissionType(resultSet, permissionType, trans);
    MEDIA_INFO_LOG("UpdatePermissionTypeFuzzer end");
}

static void AppUriPermissionOperationsFuzzer()
{
    string photoId = FDP->ConsumeBytesAsString(NUM_BYTES);
    int32_t fileId = InsertPhotoAsset(photoId);
    string appId = FDP->ConsumeBytesAsString(NUM_BYTES);
    int32_t permissionType = FuzzPermissionType();
    int32_t uriType = FuzzUriType();
    HandleInsertOperationFuzzer(appId, fileId, permissionType, uriType);
    permissionType = FuzzPermissionType();
    HandleInsertOperationFuzzer(appId, fileId, permissionType, uriType);    // if exit, run UpdatePermissionType();
    DeleteOperationFuzzer(appId, fileId, permissionType);

    BatchInsertFuzzer();
    BatchInsertInnerFuzzer();
    UpdatePermissionTypeFuzzer();
}

void SetTables()
{
    vector<string> createTableSqlList = {
        Media::PhotoUpgrade::CREATE_PHOTO_TABLE,
        Media::AppUriPermissionColumn::CREATE_APP_URI_PERMISSION_TABLE,
        Media::AppUriSensitiveColumn::CREATE_APP_URI_SENSITIVE_TABLE,
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

static inline void ClearKvStore()
{
    Media::MediaLibraryKvStoreManager::GetInstance().CloseAllKvStore();
}
} // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::AddSeed();
    OHOS::Init();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    OHOS::FDP = &fdp;
    if (data == nullptr) {
        return 0;
    }
    OHOS::AppUriPermissionOperationsFuzzer();
    OHOS::ClearKvStore();
    return 0;
}

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

#include "medialibraryurisensitiveoperations_fuzzer.h"

#include <cstdint>
#include <string>
#include <vector>
#include <fuzzer/FuzzedDataProvider.h>

#include "ability_context_impl.h"
#include "medialibrary_urisensitive_operations.h"
#include "medialibrary_app_uri_permission_operations.h"
#include "medialibrary_app_uri_sensitive_operations.h"
#include "datashare_predicates.h"
#include "media_app_uri_permission_column.h"
#include "media_app_uri_sensitive_column.h"
#include "media_column.h"
#include "media_log.h"
#include "medialibrary_command.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_operation.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_unistore.h"
#include "medialibrary_unistore_manager.h"
#include "rdb_store.h"
#include "rdb_utils.h"
#include "userfile_manager_types.h"
#include "values_bucket.h"
#include "medialibrary_kvstore_manager.h"
#include "media_upgrade.h"

namespace OHOS {
using namespace std;
using namespace DataShare;
const int32_t PERMISSION_DEFAULT = -1;
const int32_t SENSITIVE_DEFAULT = -1;
const int32_t URI_DEFAULT = 0;
const int32_t BatchInsertNumber = 5;
static const int32_t NUM_BYTES = 1;
static const int32_t MAX_PERMISSION_TYPE = 6;
static const int32_t MAX_URI_TYPE = 2;
static const int32_t MAX_SENSITIVE_TYPE = 4;
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;
FuzzedDataProvider *FDP = nullptr;

static int FuzzUriType()
{
    vector<int> vecUriType;
    vecUriType.assign(Media::AppUriSensitiveColumn::URI_TYPES_ALL.begin(),
        Media::AppUriSensitiveColumn::URI_TYPES_ALL.end());
    vecUriType.push_back(URI_DEFAULT);
    uint8_t data = FDP->ConsumeIntegralInRange<uint8_t>(0, MAX_URI_TYPE);
    return vecUriType[data];
}

static int FuzzPermissionType()
{
    vector<int> vecPermissionType;
    vecPermissionType.assign(Media::AppUriPermissionColumn::PERMISSION_TYPES_ALL.begin(),
        Media::AppUriPermissionColumn::PERMISSION_TYPES_ALL.end());
    vecPermissionType.push_back(PERMISSION_DEFAULT);
    uint8_t data = FDP->ConsumeIntegralInRange<uint8_t>(0, MAX_PERMISSION_TYPE);
    return vecPermissionType[data];
}

static int FuzzHideSensitiveType()
{
    vector<int> vecHideSensitiveType;
    vecHideSensitiveType.assign(Media::AppUriSensitiveColumn::SENSITIVE_TYPES_ALL.begin(),
        Media::AppUriSensitiveColumn::SENSITIVE_TYPES_ALL.end());
    vecHideSensitiveType.push_back(SENSITIVE_DEFAULT);
    uint8_t data = FDP->ConsumeIntegralInRange<uint8_t>(0, MAX_SENSITIVE_TYPE);
    return vecHideSensitiveType[data];
}

static void InsertOperationFuzzer(string appId, string photoId, int32_t sensitiveType, int32_t uriType)
{
    DataShareValuesBucket values;
    values.Put(Media::AppUriSensitiveColumn::APP_ID, appId);
    values.Put(Media::AppUriSensitiveColumn::FILE_ID, photoId);
    values.Put(Media::AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, sensitiveType);
    values.Put(Media::AppUriSensitiveColumn::URI_TYPE, uriType);

    Media::MediaLibraryCommand cmd(Media::OperationObject::APP_URI_PERMISSION_INNER, Media::OperationType::CREATE,
        Media::MediaLibraryApi::API_10);
    NativeRdb::ValuesBucket rdbValue = RdbDataShareAdapter::RdbUtils::ToValuesBucket(values);
    cmd.SetValueBucket(rdbValue);
    Media::UriSensitiveOperations::InsertOperation(cmd);
}

static void DeleteOperationFuzzer(string appId, string photoId)
{
    DataShare::DataSharePredicates dataSharePredicate;
    dataSharePredicate.And()->EqualTo(Media::AppUriSensitiveColumn::APP_ID, appId);
    dataSharePredicate.And()->EqualTo(Media::AppUriSensitiveColumn::FILE_ID, photoId);
    Media::MediaLibraryCommand cmd(Media::OperationObject::APP_URI_PERMISSION_INNER, Media::OperationType::DELETE,
        Media::MediaLibraryApi::API_10);
    cmd.SetTableName(Media::AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    NativeRdb::RdbPredicates rdbPredicate = RdbDataShareAdapter::RdbUtils::ToPredicates(dataSharePredicate,
        Media::AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    cmd.SetDataSharePred(dataSharePredicate);
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    Media::UriSensitiveOperations::DeleteOperation(cmd);
}

static void BatchInsertFuzzer(string appId, string photoId)
{
    vector<DataShare::DataShareValuesBucket> dataShareValues;
    for (int32_t i = 0; i < BatchInsertNumber; i++) {
        DataShareValuesBucket value;
        value.Put(Media::AppUriSensitiveColumn::APP_ID, appId);
        value.Put(Media::AppUriSensitiveColumn::FILE_ID, photoId);
        value.Put(Media::AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, FuzzHideSensitiveType());
        value.Put(Media::AppUriPermissionColumn::PERMISSION_TYPE, FuzzPermissionType());
        value.Put(Media::AppUriSensitiveColumn::URI_TYPE, FuzzUriType());
        value.Put(Media::AppUriSensitiveColumn::SOURCE_TOKENID, FDP->ConsumeIntegral<int32_t>());
        value.Put(Media::AppUriSensitiveColumn::TARGET_TOKENID, FDP->ConsumeIntegral<int32_t>());
        dataShareValues.push_back(value);
    }
    Media::MediaLibraryCommand cmd(Media::OperationObject::APP_URI_PERMISSION_INNER, Media::OperationType::CREATE,
        Media::MediaLibraryApi::API_10);
    Media::UriSensitiveOperations::GrantUriSensitive(cmd, dataShareValues);
}

static void QuerySensitiveTypeFuzzer()
{
    uint32_t tokenId = FDP->ConsumeIntegral<uint32_t>();
    std::string fileId = to_string(FDP->ConsumeIntegral<uint32_t>());
    Media::UriSensitiveOperations::QuerySensitiveType(tokenId, fileId);
}

static void QueryForceSensitiveFuzzer()
{
    uint32_t tokenId = FDP->ConsumeIntegral<uint32_t>();
    std::string fileId = to_string(FDP->ConsumeIntegral<uint32_t>());
    Media::UriSensitiveOperations::QueryForceSensitive(tokenId, fileId);
}

static void UpdateOperationFuzzer(string appId, string photoId)
{
    DataShare::DataSharePredicates dataSharePredicate;
    dataSharePredicate.And()->EqualTo(Media::AppUriSensitiveColumn::APP_ID, appId);
    dataSharePredicate.And()->EqualTo(Media::AppUriSensitiveColumn::FILE_ID, photoId);
    Media::MediaLibraryCommand cmd(Media::OperationObject::APP_URI_PERMISSION_INNER, Media::OperationType::UPDATE,
        Media::MediaLibraryApi::API_10);
    cmd.SetTableName(Media::AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    NativeRdb::RdbPredicates rdbPredicate = RdbDataShareAdapter::RdbUtils::ToPredicates(dataSharePredicate,
        Media::AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    Media::UriSensitiveOperations::UpdateOperation(cmd, rdbPredicate);
    std::string funcName = FDP->ConsumeBytesAsString(NUM_BYTES);
    std::shared_ptr<Media::TransactionOperations> trans = std::make_shared<Media::TransactionOperations>(funcName);
    Media::UriSensitiveOperations::UpdateOperation(cmd, rdbPredicate, trans);
}

static void UriSensitiveOperationsFuzzer()
{
    string photoId = FDP->ConsumeBytesAsString(NUM_BYTES);
    string appId = FDP->ConsumeBytesAsString(NUM_BYTES);
    int32_t sensitiveType = FuzzHideSensitiveType();
    int32_t uriType = FuzzUriType();

    InsertOperationFuzzer(appId, photoId, sensitiveType, uriType);
    DeleteOperationFuzzer(appId, photoId);
    BatchInsertFuzzer(appId, photoId);
    QuerySensitiveTypeFuzzer();
    QueryForceSensitiveFuzzer();
    UpdateOperationFuzzer(appId, photoId);
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
    OHOS::FDP = &fdp;
    if (data == nullptr) {
        return 0;
    }
    OHOS::UriSensitiveOperationsFuzzer();
    OHOS::ClearKvStore();
    return 0;
}

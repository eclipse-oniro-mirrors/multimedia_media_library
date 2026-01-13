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

#include "media_library_duplicate_photo_operation_fuzzer.h"

#include <cstdint>
#include <string>
#include <vector>
#include <fuzzer/FuzzedDataProvider.h>

#include "duplicate_photo_operation.h"
#include "ability_context_impl.h"
#include "rdb_predicates.h"
#include "medialibrary_rdbstore.h"
#include "deferred_photo_proc_adapter.h"
#include "multistages_capture_deferred_photo_proc_session_callback.h"

#include "medialibrary_app_uri_permission_operations.h"
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
#include "media_upgrade.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace DataShare;
using namespace OHOS::NativeRdb;
using namespace OHOS::RdbDataShareAdapter;

static const std::string APP_URI_PERMISSION_TABLE = "UriPermission";
static const int32_t NUM_BYTES = 1;
static const int32_t PERMISSION_DEFAULT = -1;
static const int32_t MAX_PERMISSION_TYPE = 6;
static const int32_t MAX_DATA = 1;

FuzzedDataProvider *provider = nullptr;
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;

static inline void FuzzMimeTypeAndDisplayNameExtension(string &mimeType, string &displayName)
{
    uint8_t data = provider->ConsumeIntegralInRange<uint8_t>(0, MAX_DATA);
    mimeType = MIMETYPE_FUZZER_LISTS[data];
    displayName = provider->ConsumeBytesAsString(NUM_BYTES) + DISPLAY_NAME_EXTENSION_FUZZER_LISTS[data];
}

static int32_t InsertPhotoAsset(string photoId)
{
    if (g_rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutString(Media::PhotoColumn::PHOTO_ID, photoId);
    values.PutString(Media::MediaColumn::MEDIA_FILE_PATH, provider->ConsumeBytesAsString(NUM_BYTES));

    string mimeType = "undefined";
    string displayName = ".undefined";
    FuzzMimeTypeAndDisplayNameExtension(mimeType, displayName);
    values.PutString(Media::MediaColumn::MEDIA_NAME, displayName);
    values.PutString(Media::MediaColumn::MEDIA_MIME_TYPE, mimeType);

    int64_t fileId = 0;
    g_rdbStore->Insert(fileId, Media::PhotoColumn::PHOTOS_TABLE, values);
    return static_cast<int32_t>(fileId);
}

static int FuzzPermissionType()
{
    vector<int> vecPermissionType;
    vecPermissionType.assign(Media::AppUriPermissionColumn::PERMISSION_TYPES_ALL.begin(),
        Media::AppUriPermissionColumn::PERMISSION_TYPES_ALL.end());
    vecPermissionType.push_back(PERMISSION_DEFAULT);
    uint8_t data = provider->ConsumeIntegralInRange<uint8_t>(0, MAX_PERMISSION_TYPE);
    return vecPermissionType[data];
}

static NativeRdb::RdbPredicates GetRdbPredicates(void)
{
    string photoId = provider->ConsumeBytesAsString(NUM_BYTES);
    int32_t fileId = InsertPhotoAsset(photoId);
    string appId = provider->ConsumeBytesAsString(NUM_BYTES);
    int32_t permissionType = FuzzPermissionType();
    static DataSharePredicates predicates;

    predicates.And()->EqualTo(Media::AppUriPermissionColumn::APP_ID, appId);
    predicates.And()->EqualTo(Media::AppUriPermissionColumn::FILE_ID, fileId);
    predicates.And()->EqualTo(Media::AppUriPermissionColumn::PERMISSION_TYPE, permissionType);
    NativeRdb::RdbPredicates rdbPredicate = RdbDataShareAdapter::RdbUtils::ToPredicates(predicates,
        Media::AppUriPermissionColumn::APP_URI_PERMISSION_TABLE);
    return rdbPredicate;
}

static void DuplicatePhotoOperationTest()
{
    shared_ptr<DuplicatePhotoOperation> duplicatePhotoOperation =
        make_shared<DuplicatePhotoOperation>();
    if (duplicatePhotoOperation == nullptr) {
        MEDIA_ERR_LOG("duplicatePhotoOperation is nullptr.");
        return;
    }

    RdbPredicates predicates = GetRdbPredicates();
    std::vector<string> columns = { provider->ConsumeBytesAsString(NUM_BYTES) };

    duplicatePhotoOperation->GetAllDuplicateAssets(predicates, columns);
    duplicatePhotoOperation->GetDuplicateAssetsToDelete(predicates, columns);
}

static void SetTables()
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
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK, "InitMediaLibrary Mgr failed, ret: %{public}d.", ret);
    auto rdbStore = Media::MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr.");
        return;
    }
    g_rdbStore = rdbStore;
    SetTables();
}

static inline void ClearKvStore()
{
    Media::MediaLibraryKvStoreManager::GetInstance().CloseAllKvStore();
}
} // namespace Media
} // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::Media::Init();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    OHOS::Media::provider = &fdp;
    if (data == nullptr) {
        return 0;
    }
    OHOS::Media::DuplicatePhotoOperationTest();
    OHOS::Media::ClearKvStore();
    return 0;
}

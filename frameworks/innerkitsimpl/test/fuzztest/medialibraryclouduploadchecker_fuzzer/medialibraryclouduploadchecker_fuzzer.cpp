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

#include "medialibraryclouduploadchecker_fuzzer.h"

#include <cstdint>
#include <string>
#include <fuzzer/FuzzedDataProvider.h>

#define private public
#include "cloud_upload_checker.h"
#undef private
#include "ability_context_impl.h"
#include "media_log.h"
#include "medialibrary_command.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_kvstore_manager.h"
#include "media_upgrade.h"

namespace OHOS {
using namespace std;
using namespace AbilityRuntime;
static const string PHOTOS_TABLE = "Photos";
static const int32_t E_ERR = -1;
static const int32_t NUM_BYTES = 1;
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;
FuzzedDataProvider *provider;

static int32_t InsertPhotoAsset()
{
    if (g_rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutString(Media::MediaColumn::MEDIA_FILE_PATH, provider->ConsumeBytesAsString(NUM_BYTES));
    values.PutInt(Media::PhotoColumn::PHOTO_DIRTY, provider->ConsumeIntegral<int32_t>());
    values.PutLong(Media::MediaColumn::MEDIA_SIZE, provider->ConsumeIntegral<int64_t>());
    int64_t fileId = 0;
    g_rdbStore->Insert(fileId, PHOTOS_TABLE, values);
    return static_cast<int32_t>(fileId);
}

static Media::CheckedPhotoInfo FuzzCheckedPhotoInfo()
{
    Media::CheckedPhotoInfo checkedPhotoInfo = {
        .fileId = InsertPhotoAsset(),
        .path = provider->ConsumeBytesAsString(NUM_BYTES)
    };
    return checkedPhotoInfo;
}

static vector<Media::CheckedPhotoInfo> FuzzVectorCheckedPhotoInfo()
{
    return {FuzzCheckedPhotoInfo()};
}

static void RepairNoOriginPhotoTest()
{
    MEDIA_INFO_LOG("RepairNoOriginPhotoTest start");
    Media::CloudUploadChecker::QueryLcdPhotoCount(provider->ConsumeIntegral<int32_t>());
    int32_t startFileId = InsertPhotoAsset();
    int32_t curFileId = -1;
    Media::CloudUploadChecker::QueryPhotoInfo(startFileId);
    Media::CloudUploadChecker::HandlePhotoInfos(FuzzVectorCheckedPhotoInfo(), curFileId);
    Media::CloudUploadChecker::RepairNoOriginPhoto();
    MEDIA_INFO_LOG("RepairNoOriginPhotoTest end");
}

static void CloudUploadCheckerTest()
{
    RepairNoOriginPhotoTest();
}

void SetTables()
{
    vector<string> createTableSqlList = { Media::PhotoUpgrade::CREATE_PHOTO_TABLE };
    for (auto &createTableSql : createTableSqlList) {
        CHECK_AND_RETURN_LOG(g_rdbStore != nullptr, "g_rdbStore is null");
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
    OHOS::provider = &fdp;
    if (data == nullptr) {
        return 0;
    }
    OHOS::CloudUploadCheckerTest();
    OHOS::ClearKvStore();
    return 0;
}
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

#include "medialibraryphotoowneralbumidoperation_fuzzer.h"

#include <cstdint>
#include <string>
#include <fstream>
#include <vector>
#include <fuzzer/FuzzedDataProvider.h>

#include "ability_context_impl.h"
#include "media_log.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_kvstore_manager.h"
#include "photo_album_column.h"
#include "photo_owner_album_id_operation.h"
#include "media_upgrade.h"

namespace OHOS {
namespace Media {
using namespace std;
static const int32_t NUM_BYTES = 1;
static const int32_t MAX_MIME_TYPE = 1;
static const int32_t DEFAULT_INDEX  = 1;
static const int32_t MAX_SUB_TYPE = 6;
static const int32_t MAX_MEDIA_TYPE = 15;
static const int32_t MAX_BYTE_VALUE = 256;
static const int32_t SEED_SIZE = 1024;
const std::string LPATH_SCREEN_SHOTS = "/Pictures/Screenshots";
const std::string SOURCE_PATH = "/storage/emulated/0/Pictures/Screenshots/IMG_20240829_072213.jpg";
const string TABLE = "PhotoAlbum";
const string PHOTOS_TABLE = "Photos";
FuzzedDataProvider *provider;
static shared_ptr<MediaLibraryRdbStore> g_rdbStore;

static inline MediaType FuzzMediaType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_MEDIA_TYPE);
    return static_cast<MediaType>(value);
}

static int32_t InsertPhotoAsset(int32_t albumId)
{
    if (g_rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::MEDIA_TYPE, static_cast<int32_t>(FuzzMediaType()));
    values.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, albumId);
    values.PutString(PhotoColumn::PHOTO_SOURCE_PATH, SOURCE_PATH);
    int64_t fileId = 0;
    int32_t ret = g_rdbStore->Insert(fileId, PHOTOS_TABLE, values);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_ERR, "g_rdbStore failed to insert ret=%{public}d", ret);
    return static_cast<int32_t>(fileId);
}

static int32_t InsertAlbumAsset()
{
    if (g_rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    int32_t albumType = provider->ConsumeBool() ? PhotoAlbumType::USER : PhotoAlbumType::SOURCE;
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, albumType);
    values.PutString(PhotoAlbumColumns::ALBUM_LPATH, LPATH_SCREEN_SHOTS);
    int64_t fileId = 0;
    int32_t ret = g_rdbStore->Insert(fileId, TABLE, values);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_ERR, "g_rdbStore failed to insert ret=%{public}d", ret);
    return static_cast<int32_t>(fileId);
}

static void PhotoOwnerAlbumIdOperationTest()
{
    MEDIA_INFO_LOG("PhotoOwnerAlbumIdOperationTest start");
    if (g_rdbStore == nullptr) {
        return;
    }
    int32_t albumId = InsertAlbumAsset();
    int32_t fileId1 = InsertPhotoAsset(albumId);

    albumId = -1;
    int32_t fileId2 = InsertPhotoAsset(albumId);
    std::vector<std::string> fileIds = { to_string(fileId1), to_string(fileId2) };
    std::shared_ptr<MediaLibraryRdbStore> rdbStore = g_rdbStore;
    PhotoOwnerAlbumIdOperation().SetRdbStore(rdbStore).SetFileIds(fileIds).FixPhotoRelation();
    MEDIA_INFO_LOG("PhotoOwnerAlbumIdOperationTest end");
}

void SetTables()
{
    vector<string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        PhotoAlbumColumns::CREATE_TABLE,
    };
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
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK, "InitMediaLibrary Mgr failed, ret: %{public}d.", ret);
    auto rdbStore = Media::MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr.");
        return;
    }
    g_rdbStore = rdbStore;
    SetTables();
}

static int32_t AddSeed()
{
    char *seedData = new char[SEED_SIZE];
    for (int i = 0; i < SEED_SIZE; i++) {
        seedData[i] = static_cast<char>(i % MAX_BYTE_VALUE);
    }

    const char* filename = "corpus/seed.txt";
    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file) {
        MEDIA_ERR_LOG("Cannot open file filename:%{public}s", filename);
        delete[] seedData;
        seedData = nullptr;
        return Media::E_ERR;
    }
    file.write(seedData, SEED_SIZE);
    file.close();
    delete[] seedData;
    seedData = nullptr;
    MEDIA_INFO_LOG("seedData has been successfully written to file filename:%{public}s", filename);
    return Media::E_OK;
}

static void ClearKvStore()
{
    MediaLibraryKvStoreManager::GetInstance().CloseAllKvStore();
}
} // namespace Media
} // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::Media::Init();
    OHOS::Media::AddSeed();
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }
    FuzzedDataProvider fdp(data, size);
    OHOS::Media::provider = &fdp;
    if (OHOS::Media::provider == nullptr) {
        return 0;
    }
    OHOS::Media::PhotoOwnerAlbumIdOperationTest();
    OHOS::Media::ClearKvStore();
    return 0;
}
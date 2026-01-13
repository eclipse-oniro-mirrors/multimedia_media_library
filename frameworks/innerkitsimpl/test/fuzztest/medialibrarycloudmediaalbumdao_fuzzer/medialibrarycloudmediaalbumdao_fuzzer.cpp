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

#include "medialibrarycloudmediaalbumdao_fuzzer.h"

#include <cstdint>
#include <string>
#include <fstream>
#include <fuzzer/FuzzedDataProvider.h>
#define private public
#define protected public
#include "cloud_media_album_dao.h"
#include "cloud_media_common_dao.h"
#undef private
#include "ability_context_impl.h"
#include "media_log.h"
#include "medialibrary_command.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "photo_map_column.h"
#include "accurate_common_data.h"
#include "asset_accurate_refresh.h"
#include "album_accurate_refresh.h"
#include "medialibrary_errno.h"
#include "medialibrary_kvstore_manager.h"
#include "media_upgrade.h"

namespace OHOS {
using namespace std;
using namespace OHOS::Media;
using namespace OHOS::Media::CloudSync;
const int32_t NUM_BYTES = 1;
const int32_t ALBUM_ID_2 = 2;
const int32_t PHOTO_OWNER_ALBUM_ID_2 = 2;
const int32_t MAX_BYTE_VALUE = 256;
const int32_t SEED_SIZE = 1024;
const string TABLE = "PhotoAlbum";
const string PHOTOS_TABLE = "Photos";
FuzzedDataProvider* provider = nullptr;

std::shared_ptr<AccurateRefresh::AlbumAccurateRefresh> g_albumRefresh;
std::shared_ptr<CloudMediaAlbumDao> cloudMediaAlbumDao = std::make_shared<CloudMediaAlbumDao>();

static inline Media::DirtyType FuzzDirtyType()
{
    int32_t value = provider->ConsumeIntegral<int32_t>() % 8;
    if (value >= static_cast<int32_t>(Media::DirtyType::TYPE_SYNCED) &&
        value <= static_cast<int32_t>(Media::DirtyType::TYPE_COPY)) {
        return static_cast<Media::DirtyType>(value);
    }
    return Media::DirtyType::TYPE_RETRY;
}

static PhotoAlbumDto FuzzPhotoAlbumDto1(string &cloudId)
{
    PhotoAlbumDto record;
    record.albumName = "albumName";
    record.bundleName = "bundleName";
    record.lPath = provider->ConsumeBool() ? "/pictures/users/" : "";
    record.albumType = static_cast<int32_t>(PhotoAlbumType::INVALID);
    record.albumSubType = provider->ConsumeIntegral<int32_t>();
    record.albumDateAdded = provider->ConsumeIntegral<int64_t>();
    record.albumDateCreated = provider->ConsumeIntegral<int64_t>();
    record.albumDateModified = provider->ConsumeIntegral<int64_t>();
    record.localLanguage = provider->ConsumeBytesAsString(NUM_BYTES);
    record.cloudId = cloudId;
    record.isSuccess = false;
    return record;
}

static PhotoAlbumDto FuzzPhotoAlbumDto2()
{
    PhotoAlbumDto record;
    record.albumName = "albumName";
    record.bundleName = "bundleName";
    record.lPath = "";
    record.albumType = static_cast<int32_t>(AlbumType::SOURCE);
    record.albumSubType = provider->ConsumeIntegral<int32_t>();
    record.albumDateAdded = provider->ConsumeIntegral<int64_t>();
    record.albumDateModified = provider->ConsumeIntegral<int64_t>();
    record.localLanguage = provider->ConsumeBytesAsString(NUM_BYTES);
    record.cloudId = "";
    record.isSuccess = false;
    return record;
}

static int32_t InsertPhotoAsset()
{
    if (g_albumRefresh == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, PHOTO_OWNER_ALBUM_ID_2);
    values.PutInt(MediaColumn::MEDIA_HIDDEN, 0);
    values.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, 0);
    values.PutLong(MediaColumn::MEDIA_DATE_TRASHED, 0);
    int64_t fileId = 0;
    g_albumRefresh->Insert(fileId, PHOTOS_TABLE, values);
    return static_cast<int32_t>(fileId);
}

static int32_t InsertAlbumAsset(string &cloudId)
{
    if (g_albumRefresh == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_PRIORITY, 1);
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, static_cast<int32_t>(PhotoAlbumType::USER));
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, provider->ConsumeIntegral<int32_t>());
    values.PutInt(PhotoAlbumColumns::ALBUM_ID, ALBUM_ID_2);
    values.PutInt(PhotoAlbumColumns::ALBUM_DIRTY, static_cast<int32_t>(FuzzDirtyType()));
    values.PutLong(PhotoAlbumColumns::ALBUM_DATE_ADDED, provider->ConsumeIntegral<int64_t>());
    values.PutLong(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, provider->ConsumeIntegral<int64_t>());
    values.PutString(PhotoAlbumColumns::ALBUM_LOCAL_LANGUAGE, provider->ConsumeBytesAsString(NUM_BYTES));
    values.PutString(PhotoAlbumColumns::ALBUM_CLOUD_ID, cloudId);
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, "albumName");
    values.PutString(PhotoAlbumColumns::ALBUM_BUNDLE_NAME, "bundleName");
    values.PutString(PhotoAlbumColumns::ALBUM_LPATH, "/pictures/users/");
    int64_t fileId = 0;
    g_albumRefresh->Insert(fileId, TABLE, values);
    return static_cast<int32_t>(fileId);
}

static int32_t InsertDeleteAsset(string &cloudId)
{
    if (g_albumRefresh == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_DIRTY, static_cast<int32_t>(DirtyType::TYPE_SDIRTY));
    values.PutInt(PhotoAlbumColumns::ALBUM_ID, 1);
    values.PutInt(PhotoAlbumColumns::ALBUM_COUNT, provider->ConsumeIntegral<int32_t>());
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, provider->ConsumeIntegral<int32_t>());
    values.PutLong(PhotoAlbumColumns::ALBUM_DATE_ADDED, provider->ConsumeIntegral<int64_t>());
    values.PutLong(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, provider->ConsumeIntegral<int64_t>());
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, provider->ConsumeBytesAsString(NUM_BYTES));
    values.PutString(PhotoAlbumColumns::ALBUM_LPATH, "/pictures/users/");
    values.PutString(PhotoAlbumColumns::ALBUM_CLOUD_ID, cloudId);
    values.PutString(PhotoAlbumColumns::ALBUM_BUNDLE_NAME, provider->ConsumeBytesAsString(NUM_BYTES));
    values.PutString(PhotoAlbumColumns::ALBUM_LOCAL_LANGUAGE, provider->ConsumeBytesAsString(NUM_BYTES));
    int64_t fileId = 0;
    g_albumRefresh->Insert(fileId, TABLE, values);
    return static_cast<int32_t>(fileId);
}

static int32_t InsertQueryAsset()
{
    if (g_albumRefresh == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_PRIORITY, 1);
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, provider->ConsumeBool() ? static_cast<int32_t>(AlbumType::SOURCE) :
        static_cast<int32_t>(AlbumType::NORMAL));
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, provider->ConsumeIntegral<int32_t>());
    values.PutInt(PhotoAlbumColumns::ALBUM_DIRTY, static_cast<int32_t>(DirtyType::TYPE_SDIRTY));
    values.PutInt(PhotoAlbumColumns::ALBUM_ID, ALBUM_ID_2);
    values.PutLong(PhotoAlbumColumns::ALBUM_DATE_ADDED, provider->ConsumeIntegral<int64_t>());
    values.PutLong(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, provider->ConsumeIntegral<int64_t>());
    values.PutString(PhotoAlbumColumns::ALBUM_LOCAL_LANGUAGE, provider->ConsumeBytesAsString(NUM_BYTES));
    values.PutString(PhotoAlbumColumns::ALBUM_CLOUD_ID, "");
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, "albumName");
    values.PutString(PhotoAlbumColumns::ALBUM_BUNDLE_NAME, "bundleName");
    values.PutString(PhotoAlbumColumns::ALBUM_LPATH, "");
    int64_t fileId = 0;
    g_albumRefresh->Insert(fileId, TABLE, values);
    return static_cast<int32_t>(fileId);
}

static void CloudMediaAlbumDaoFuzzer()
{
    string field1 = "cloud_id";
    string value1 = provider->ConsumeBytesAsString(NUM_BYTES);
    PhotoAlbumDto record1 = FuzzPhotoAlbumDto1(value1);
    PhotoAlbumDto record2 = FuzzPhotoAlbumDto2();
    InsertAlbumAsset(value1);
    InsertQueryAsset();

    CHECK_AND_RETURN_LOG(cloudMediaAlbumDao != nullptr, "cloudMediaAlbumDao is nullptr");
    cloudMediaAlbumDao->HandleLPathAndAlbumType(record1);

    cloudMediaAlbumDao->InsertCloudByLPath(record1, g_albumRefresh);
    cloudMediaAlbumDao->UpdateCloudAlbum(record1, field1, value1, g_albumRefresh);

    cloudMediaAlbumDao->InsertCloudByCloudId(record2, g_albumRefresh);
    InsertQueryAsset();
    cloudMediaAlbumDao->InsertCloudByCloudId(record2, g_albumRefresh);

    string cloudId = value1;
    cloudMediaAlbumDao->QueryLocalMatchAlbum(cloudId);

    vector<string> failedAlbumIds;
    failedAlbumIds.emplace_back(to_string(provider->ConsumeIntegral<uint8_t>()));
    failedAlbumIds.emplace_back(to_string(1));
    InsertDeleteAsset(cloudId);
    cloudMediaAlbumDao->OnDeleteAlbums(failedAlbumIds);

    vector<PhotoAlbumDto> albums = { FuzzPhotoAlbumDto2() };
    int32_t failSize = 0;
    cloudMediaAlbumDao->OnCreateRecords(albums, failSize);
    InsertPhotoAsset();
    cloudMediaAlbumDao->OnDeleteAlbumRecords(cloudId);
    cloudMediaAlbumDao->OnMdirtyAlbumRecords(cloudId);

    vector<PhotoAlbumPo> cloudRecordPoList;
    int32_t limitSize = DEFAULT_VALUE;
    cloudMediaAlbumDao->GetCopyAlbum(limitSize, cloudRecordPoList);
    cloudMediaAlbumDao->GetDeletedRecordsAlbum(limitSize, cloudRecordPoList);
    cloudMediaAlbumDao->GetMetaModifiedAlbum(limitSize, cloudRecordPoList);
    cloudMediaAlbumDao->GetCreatedRecords(limitSize, cloudRecordPoList);
    InsertDeleteAsset(cloudId);
    limitSize = LIMIT_SIZE;
    cloudMediaAlbumDao->GetDeletedRecordsAlbum(limitSize, cloudRecordPoList);
    cloudMediaAlbumDao->GetMetaModifiedAlbum(limitSize, cloudRecordPoList);
    cloudMediaAlbumDao->GetCreatedRecords(limitSize, cloudRecordPoList);

    string key = "cloud_id";
    vector<string> argrs = { value1 };
    InsertAlbumAsset(value1);

    string field2 = PhotoAlbumColumns::ALBUM_LPATH;
    string value2 = "/Pictures/hiddenAlbum";
    cloudMediaAlbumDao->DeleteCloudAlbum(field2, value2, g_albumRefresh);
    field2 = provider->ConsumeBytesAsString(NUM_BYTES);
    value2 = "/pictures/users/";
    InsertDeleteAsset(cloudId);
    cloudMediaAlbumDao->DeleteCloudAlbum(field2, value2, g_albumRefresh);
}

static void InsertAndRemoveAlbumFailedRecoredFuzzer()
{
    string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    if (cloudMediaAlbumDao == nullptr) {
        return;
    }
    cloudMediaAlbumDao->InsertAlbumModifyFailedRecord(cloudId);
    cloudMediaAlbumDao->InsertAlbumInsertFailedRecord(cloudId);
    cloudMediaAlbumDao->InsertAlbumCreateFailedRecord(cloudId);
    cloudMediaAlbumDao->RemoveAlbumModifyFailedRecord(cloudId);
    cloudMediaAlbumDao->RemoveAlbumInsertFailedRecord(cloudId);
    cloudMediaAlbumDao->RemoveAlbumCreateFailedRecord(cloudId);
    cloudMediaAlbumDao->ClearAlbumFailedRecords();
}

void SetTables()
{
    auto rdbStore = Media::MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr");
        return;
    }
    vector<string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        PhotoAlbumColumns::TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        int32_t ret = rdbStore->ExecuteSql(createTableSql);
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

    auto albumRefresh = std::make_shared<AccurateRefresh::AlbumAccurateRefresh>();
    if (albumRefresh == nullptr) {
        MEDIA_ERR_LOG("albumRefresh is nullptr");
        return;
    }
    g_albumRefresh = albumRefresh;
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
    /* Run your code on data */
    FuzzedDataProvider fdp(data, size);
    if (data == nullptr) {
        return 0;
    }
    OHOS::provider = &fdp;
    OHOS::CloudMediaAlbumDaoFuzzer();
    OHOS::InsertAndRemoveAlbumFailedRecoredFuzzer();
    OHOS::ClearKvStore();
    return 0;
}
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

#include "medialibrarydfx_fuzzer.h"

#include <cstdint>
#include <string>
#include <fstream>
#include <fuzzer/FuzzedDataProvider.h>

#include "ability_context_impl.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_command.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_kvstore_manager.h"
#include "photo_album_column.h"
#include "photo_file_utils.h"

#define private public
#include "dfx_collector.h"
#include "dfx_database_utils.h"
#include "dfx_deprecated_perm_usage.h"
#include "dfx_moving_photo.h"
#include "dfx_timer.h"
#include "dfx_transaction.h"
#include "dfx_worker.h"
#include "dfx_reporter.h"
#undef private
#include "media_upgrade.h"

namespace OHOS {
using namespace std;
using namespace AbilityRuntime;
using namespace OHOS::Media;
static const int32_t NUM_BYTES = 1;
static const int32_t THUMBNAIL_READY_2 = 2;
static const int32_t THUMBNAIL_READY_3 = 3;
static const int32_t MIN_PHOTO_POSITION = 1;
static const int32_t MIN_DFX_TYPE = 17;
static const int32_t MAX_PHOTO_THUMB_STATUS = 2;
static const int32_t MAX_PHOTO_POSITION = 3;
static const int32_t MAX_DIRTY_TYPE = 8;
static const int32_t MAX_MEDIA_TYPE = 14;
static const int32_t MAX_DFX_TYPE = 20;
static const int32_t MAX_BYTE_VALUE = 256;
static const int32_t SEED_SIZE = 1024;
static const std::string PHOTO_DIR = "/storage/cloud/files/photo/16/";
const string TABLE = "PhotoAlbum";
const string PHOTOS_TABLE = "Photos";
FuzzedDataProvider *provider = nullptr;
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;

static inline Media::DfxType FuzzDfxType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(MIN_DFX_TYPE, MAX_DFX_TYPE);
    return static_cast<Media::DfxType>(value);
}

static inline Media::DirtyType FuzzDirtyType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_DIRTY_TYPE);
    return static_cast<Media::DirtyType>(value);
}

static inline int32_t FuzzPhotoPosition()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(MIN_PHOTO_POSITION, MAX_PHOTO_POSITION);
    return static_cast<Media::PhotoPosition>(value);
}

static inline int32_t FuzzPhotoThumbStatus()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_PHOTO_THUMB_STATUS);
    return static_cast<Media::PhotoThumbStatus>(value);
}

static inline int32_t FuzzMediaType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_MEDIA_TYPE);
    return static_cast<Media::MediaType>(value);
}

static int32_t InsertAlbumAsset()
{
    NativeRdb::ValuesBucket values;
    values.PutInt(Media::PhotoAlbumColumns::ALBUM_SUBTYPE, provider->ConsumeIntegral<int32_t>());
    values.PutInt(Media::PhotoAlbumColumns::ALBUM_COUNT, provider->ConsumeIntegral<int32_t>());
    values.PutInt(Media::PhotoAlbumColumns::ALBUM_IMAGE_COUNT, provider->ConsumeIntegral<int32_t>());
    values.PutInt(Media::PhotoAlbumColumns::ALBUM_VIDEO_COUNT, provider->ConsumeIntegral<int32_t>());
    values.PutString(Media::PhotoAlbumColumns::ALBUM_CLOUD_ID, provider->ConsumeBytesAsString(NUM_BYTES));
    int64_t fileId = 0;
    g_rdbStore->Insert(fileId, TABLE, values);
    return static_cast<int32_t>(fileId);
}

static int32_t InsertPhotoAsset()
{
    NativeRdb::ValuesBucket values;
    values.PutInt(Media::PhotoColumn::PHOTO_POSITION, FuzzPhotoPosition());
    values.PutInt(Media::PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(FuzzDirtyType()));
    values.PutInt(Media::PhotoColumn::PHOTO_THUMB_STATUS, FuzzPhotoThumbStatus());
    values.PutInt(Media::PhotoColumn::PHOTO_WIDTH, provider->ConsumeIntegral<uint16_t>());
    values.PutInt(Media::PhotoColumn::PHOTO_HEIGHT, provider->ConsumeIntegral<uint16_t>());
    values.PutInt(Media::MediaColumn::MEDIA_TYPE, FuzzMediaType());
    int64_t thumbnailReady = provider->ConsumeBool() ? THUMBNAIL_READY_3 : THUMBNAIL_READY_2;
    values.PutLong(Media::PhotoColumn::PHOTO_THUMBNAIL_READY, thumbnailReady);
    values.PutLong(Media::MediaColumn::MEDIA_SIZE, provider->ConsumeIntegral<int64_t>());
    values.PutString(Media::MediaColumn::MEDIA_FILE_PATH, provider->ConsumeBytesAsString(NUM_BYTES));
    values.PutString(Media::PhotoColumn::PHOTO_CLOUD_ID, provider->ConsumeBytesAsString(NUM_BYTES));
    values.PutString(Media::MediaColumn::MEDIA_MIME_TYPE, provider->ConsumeBytesAsString(NUM_BYTES));
    int64_t fileId = 0;
    g_rdbStore->Insert(fileId, PHOTOS_TABLE, values);
    return static_cast<int32_t>(fileId);
}

static void DfxCollectorFuzzer()
{
    std::shared_ptr<Media::DfxCollector> dfxCollector = std::make_shared<Media::DfxCollector>();
    std::string bundleName = provider->ConsumeBytesAsString(NUM_BYTES);
    int32_t type = FuzzDfxType();
    int32_t value = -1;
    dfxCollector->CollectDeleteBehavior(bundleName, type, value);

    type = Media::DfxType::ALBUM_REMOVE_PHOTOS;
    dfxCollector->CollectDeleteBehavior(bundleName, type, value);

    type = Media::DfxType::TRASH_PHOTO;
    dfxCollector->CollectDeleteBehavior(bundleName, type, value);

    type = Media::DfxType::ALBUM_DELETE_ASSETS;
    dfxCollector->CollectDeleteBehavior(bundleName, type, value);

    std::string appName = provider->ConsumeBytesAsString(NUM_BYTES);
    bool adapted = provider->ConsumeBool();
    dfxCollector->CollectAdaptationToMovingPhotoInfo(appName, adapted);
}

static void DfxDatabaseUtilsFuzzer()
{
    int32_t fileId = InsertAlbumAsset();
    MEDIA_INFO_LOG("fileId: %{public}d.", fileId);
    int32_t albumSubtype = provider->ConsumeIntegral<int32_t>();
    Media::DfxDatabaseUtils::QueryAlbumInfoBySubtype(albumSubtype);
    fileId = InsertPhotoAsset();
    MEDIA_INFO_LOG("fileId: %{public}d.", fileId);
    Media::DfxDatabaseUtils::QueryDirtyCloudPhoto();

    int32_t downloadedThumb = provider->ConsumeIntegral<int32_t>();
    int32_t generatedThumb = provider->ConsumeIntegral<int32_t>();
    Media::DfxDatabaseUtils::QueryDownloadedAndGeneratedThumb(downloadedThumb, generatedThumb);

    bool isLocal = provider->ConsumeBool();
    Media::DfxDatabaseUtils::QueryASTCThumb(isLocal);
    Media::DfxDatabaseUtils::QueryLCDThumb(isLocal);
    Media::DfxDatabaseUtils::QueryPhotoErrorCount();

    int32_t totalDownload = provider->ConsumeIntegral<int32_t>();
    Media::DfxDatabaseUtils::QueryTotalCloudThumb(totalDownload);

    string photoMimeType;
    Media::QuerySizeAndResolution queryInfo;
    Media::DfxDatabaseUtils::GetPhotoMimeType(photoMimeType);
    Media::DfxDatabaseUtils::GetSizeAndResolutionInfo(queryInfo);
}

static void DfxTimerFuzzer()
{
    int32_t type = provider->ConsumeIntegral<int32_t>();
    int32_t object = provider->ConsumeIntegral<int32_t>();
    int64_t timeOut = provider->ConsumeIntegral<int64_t>();
    bool isReport = provider->ConsumeBool();
    std::shared_ptr<Media::DfxTimer> dfxTimer = std::make_shared<Media::DfxTimer>(type, object, timeOut, isReport);
    dfxTimer->SetCallerUid(provider->ConsumeIntegral<int32_t>());
    dfxTimer->End();
}

static void DfxTransactionFuzzer()
{
    std::string funcName = provider->ConsumeBytesAsString(NUM_BYTES);
    std::shared_ptr<Media::DfxTransaction> dfxTransaction = std::make_shared<Media::DfxTransaction>(funcName);
    dfxTransaction->Restart();
    dfxTransaction->ReportIfTimeout();
    uint8_t abnormalType = provider->ConsumeIntegral<uint8_t>();
    int32_t errCode = provider->ConsumeIntegral<int32_t>();
    dfxTransaction->ReportError(abnormalType, errCode);
}

static void DfxWorkerFuzzer()
{
    auto dfxWorker = Media::DfxWorker::GetInstance();
    Media::DfxExecute execute;
    Media::DfxData *dfxData = nullptr;
    std::shared_ptr<Media::DfxTask> task = std::make_shared<Media::DfxTask>(execute, dfxData);
    dfxWorker->StartLoopTaskDelay();
    dfxWorker->End();
}

static void DfxDeprecatedPermUsageFuzzer()
{
    uint32_t code = provider->ConsumeIntegral<uint32_t>();
    DfxDeprecatedPermUsage::Record(code, 0);

    uint32_t object = provider->ConsumeIntegral<uint32_t>();
    uint32_t type = provider->ConsumeIntegral<uint32_t>();
    DfxDeprecatedPermUsage::Record(object, type);

    DfxDeprecatedPermUsage::Statistics();
}

void SetTables()
{
    vector<string> createTableSqlList = {
        Media::PhotoUpgrade::CREATE_PHOTO_TABLE,
        Media::PhotoAlbumColumns::CREATE_TABLE,
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
    OHOS::provider = &fdp;
    if (data == nullptr) {
        return 0;
    }
    OHOS::DfxCollectorFuzzer();
    OHOS::DfxDatabaseUtilsFuzzer();
    OHOS::DfxTimerFuzzer();
    OHOS::DfxTransactionFuzzer();
    OHOS::DfxWorkerFuzzer();
    OHOS::DfxDeprecatedPermUsageFuzzer();
    OHOS::ClearKvStore();
    return 0;
}
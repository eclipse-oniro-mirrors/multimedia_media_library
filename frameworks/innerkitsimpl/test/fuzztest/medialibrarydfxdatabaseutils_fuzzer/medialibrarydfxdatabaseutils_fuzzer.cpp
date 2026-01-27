/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "medialibrarydfxdatabaseutils_fuzzer.h"

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
#include "medialibrary_db_const.h"
#include "media_column.h"
#include "media_upgrade.h"
#include "photo_album_column.h"
#include "photo_file_utils.h"

#include "power_efficiency_manager.h"
#include "settings_data_manager.h"

#define private public
#include "dfx_database_utils.h"
#undef private

namespace OHOS {
namespace Media {
using namespace std;
static constexpr int32_t NUM_BYTES = 1;
static constexpr int32_t INDEX = 1;
static constexpr int32_t LCD_VISIT_TIME = 2;
static constexpr int32_t MIN_ALBUM_UPLOAD_SWITCH_STATUS = -1;
static constexpr int32_t MAX_ALBUM_UPLOAD_SWITCH_STATUS = 0;
static constexpr int32_t MAX_DIRTY_TYPE = 8;
static constexpr int32_t MAX_BYTE_VALUE = 256;
static constexpr int32_t SEED_SIZE = 1024;
static constexpr int64_t SIZE = 1024 * 1024 * 1024;
const string TABLE = "PhotoAlbum";
const string PHOTOS_TABLE = "Photos";
FuzzedDataProvider *provider;
std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;

int32_t g_photoAlbumTypeArray[] = {
    0, 2048
};

static inline int32_t FuzzPhotoAlbumType()
{
    if (provider == nullptr) {
        return E_ERR;
    }
    return provider->PickValueInArray(g_photoAlbumTypeArray);
}

static inline DirtyType FuzzDirtyType()
{
    if (provider == nullptr) {
        return DirtyType::TYPE_SYNCED;
    }
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_DIRTY_TYPE);
    return static_cast<DirtyType>(value);
}

static inline int32_t FuzzPhotoPosition()
{
    if (provider == nullptr) {
        return static_cast<int32_t>(PhotoPositionType::LOCAL);
    }
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, PHOTO_POSITION_TYPE_LIST.size() - INDEX);
    return static_cast<int32_t>(PHOTO_POSITION_TYPE_LIST[value]);
}

static inline int32_t FuzzPhotoThumbStatus()
{
    if (provider == nullptr) {
        return static_cast<int32_t>(ThumbState::DOWNLOADED);
    }
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, THUMB_STATUS_LIST.size() - INDEX);
    return static_cast<int32_t>(THUMB_STATUS_LIST[value]);
}

static inline int64_t FuzzThumbnailReady()
{
    if (provider == nullptr) {
        return static_cast<int64_t>(ThumbnailReady::GENERATE_THUMB_RETRY);
    }
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, THUMBNAIL_READY_LIST.size() - INDEX);
    return static_cast<int64_t>(THUMBNAIL_READY_LIST[value]);
}

static inline int32_t FuzzMediaType()
{
    if (provider == nullptr) {
        return MediaType::MEDIA_TYPE_IMAGE;
    }
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MEDIA_TYPE_LISTS.size() - INDEX);
    return static_cast<int32_t>(MEDIA_TYPE_LISTS[value]);
}

static inline int32_t FuzzFHeightAndWidth()
{
    if (provider == nullptr) {
        return FILE_HEIGHT_AND_WIDTH_120;
    }
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, FILE_HEIGHT_AND_WIDTH_LISTS.size() - INDEX);
    return FILE_HEIGHT_AND_WIDTH_LISTS[value];
}

static inline string FuzzMimeType()
{
    if (provider == nullptr) {
        return MIMETYPE_LISTS[0];
    }
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MIMETYPE_LISTS.size() - INDEX);
    return MIMETYPE_LISTS[value];
}

static inline AlbumUploadSwitchStatus FuzzAlbumUploadSwitchStatus()
{
    if (provider == nullptr) {
        return AlbumUploadSwitchStatus::NONE;
    }
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(MIN_ALBUM_UPLOAD_SWITCH_STATUS,
        MAX_ALBUM_UPLOAD_SWITCH_STATUS);
    return static_cast<AlbumUploadSwitchStatus>(value);
}

static int32_t InsertAlbumAsset(int32_t albumSubtype, int32_t uploadStatus)
{
    if (g_rdbStore == nullptr && provider == nullptr) {
        MEDIA_ERR_LOG("g_rdbStore or provider is nullptr");
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumType::USER);
    values.PutInt(PhotoAlbumColumns::UPLOAD_STATUS, uploadStatus);
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, albumSubtype);
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, provider->ConsumeBytesAsString(NUM_BYTES));
    int64_t albumId = 0;
    int32_t ret = g_rdbStore->Insert(albumId, TABLE, values);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_ERR, "g_rdbStore failed to insert ret=%{public}d", ret);
    return static_cast<int32_t>(albumId);
}

static int32_t InsertPhotoAsset()
{
    if (g_rdbStore == nullptr && provider == nullptr) {
        MEDIA_ERR_LOG("g_rdbStore or provider is nullptr");
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_POSITION, FuzzPhotoPosition());
    values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(FuzzDirtyType()));
    values.PutInt(PhotoColumn::PHOTO_THUMB_STATUS, FuzzPhotoThumbStatus());
    values.PutInt(PhotoColumn::PHOTO_LCD_VISIT_TIME, LCD_VISIT_TIME);
    values.PutInt(PhotoColumn::PHOTO_WIDTH, FuzzFHeightAndWidth());
    values.PutInt(PhotoColumn::PHOTO_HEIGHT, FuzzFHeightAndWidth());
    values.PutInt(MediaColumn::MEDIA_TYPE, FuzzMediaType());
    values.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE));
    values.PutLong(PhotoColumn::PHOTO_THUMBNAIL_READY, FuzzThumbnailReady());
    values.PutLong(MediaColumn::MEDIA_SIZE, SIZE);
    values.PutLong(PhotoColumn::PHOTO_CLOUD_VERSION, provider->ConsumeIntegral<uint64_t>());
    values.PutString(MediaColumn::MEDIA_MIME_TYPE, FuzzMimeType());
    values.PutString(PhotoColumn::PHOTO_MEDIA_SUFFIX, "jpeg");
    int64_t fileId = 0;
    int32_t ret = g_rdbStore->Insert(fileId, PHOTOS_TABLE, values);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_ERR, "g_rdbStore failed to insert ret=%{public}d", ret);
    return static_cast<int32_t>(fileId);
}

static void DfxDatabaseUtilsFuzzer()
{
    MEDIA_INFO_LOG("DfxDatabaseUtilsFuzzer start");
    int32_t albumSubtype = FuzzPhotoAlbumType();
    int32_t uploadStatus = static_cast<int32_t>(FuzzAlbumUploadSwitchStatus());
    int32_t albumId = InsertAlbumAsset(albumSubtype, uploadStatus);
    MEDIA_INFO_LOG("DfxDatabaseUtilsFuzzer albumId: %{public}d.", albumId);
    DfxDatabaseUtils::QueryAlbumInfoBySubtype(albumSubtype);

    int32_t fileId = InsertPhotoAsset();
    MEDIA_INFO_LOG("DfxDatabaseUtilsFuzzer fileId: %{public}d.", fileId);
    int32_t mediaType = FuzzMediaType();
    int32_t position = FuzzPhotoPosition();
    DfxDatabaseUtils::QueryFromPhotos(mediaType, position);
    DfxDatabaseUtils::QueryDirtyCloudPhoto();

    InsertPhotoAsset();
    int32_t downloadedThumb = -1;
    int32_t generatedThumb = -1;
    DfxDatabaseUtils::QueryDownloadedAndGeneratedThumb(downloadedThumb, generatedThumb);

    std::string table = PhotoColumn::PHOTOS_TABLE;
    std::string column = "cloud_version";
    DfxDatabaseUtils::QueryAnalysisVersion(table, column);
    DfxDatabaseUtils::QueryDbVersion();
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    std::vector<std::string> columns = {MediaColumn::MEDIA_TYPE};
    std::string queryColumn = "media_type";
    double value = 0.0;
    DfxDatabaseUtils::QueryDouble(predicates, columns, queryColumn, value);

    CHECK_AND_RETURN_LOG(provider != nullptr, "provider is nullptr");
    bool isLocal = provider->ConsumeBool();
    DfxDatabaseUtils::QueryASTCThumb(isLocal);
    DfxDatabaseUtils::QueryLCDThumb(isLocal);

    if (provider->ConsumeBool()) {
        PowerEfficiencyManager::SetSubscriberStatus(true, true);
    }
    DfxDatabaseUtils::QueryPhotoErrorCount();

    int32_t totalDownload = provider->ConsumeIntegral<int32_t>();
    DfxDatabaseUtils::QueryTotalCloudThumb(totalDownload);

    string photoMimeType;
    QuerySizeAndResolution queryInfo;
    DfxDatabaseUtils::GetPhotoMimeType(photoMimeType);
    DfxDatabaseUtils::GetSizeAndResolutionInfo(queryInfo);

    AncoCountFormatInfo reportData;
    DfxDatabaseUtils::QueryAncoPhotosFormatAndCount(reportData);

    uploadStatus = static_cast<int32_t>(FuzzAlbumUploadSwitchStatus());
    albumSubtype = FuzzPhotoAlbumType();
    InsertAlbumAsset(albumSubtype, uploadStatus);
    DfxDatabaseUtils::QueryAlbumNamesByUploadStatus(uploadStatus);
    MEDIA_INFO_LOG("DfxDatabaseUtilsFuzzer end");
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

static inline void ClearKvStore()
{
    MediaLibraryKvStoreManager::GetInstance().CloseAllKvStore();
}
} // namespace Media
} // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::Media::AddSeed();
    OHOS::Media::Init();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }
    FuzzedDataProvider fdp(data, size);
    OHOS::Media::provider = &fdp;
    OHOS::Media::DfxDatabaseUtilsFuzzer();
    OHOS::Media::ClearKvStore();
    return 0;
}
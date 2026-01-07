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

#include "medialibrarycloudmediaphotosdao_fuzzer.h"

#include <cstdint>
#include <string>
#include <fstream>
#include <fuzzer/FuzzedDataProvider.h>
#define private public
#include "cloud_media_photos_dao.h"
#undef private
#include "ability_context_impl.h"
#include "media_log.h"
#include "medialibrary_command.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_db_const.h"
#include "photo_map_column.h"
#include "medialibrary_kvstore_manager.h"
#include "hi_audit.h"

namespace OHOS {
using namespace std;
using namespace OHOS::Media;
using namespace OHOS::Media::CloudSync;
const int32_t FILE_ID = 1;
const int32_t NUM_BYTES = 1;
const int32_t VECTOR_SIZE = 5;
const int32_t MAX_BYTE_VALUE = 256;
const int32_t SEED_SIZE = 1024;
const string PHOTO_MAP_TABLE = "PhotoMap";
const string PHOTOS_TABLE = "Photos";
const string PHOTO_ALBUM_TABLE = "PhotoAlbum";
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;
FuzzedDataProvider* provider = nullptr;
std::shared_ptr<CloudMediaPhotosDao> cloudMediaPhotosDao = std::make_shared<CloudMediaPhotosDao>();

static inline Media::DirtyTypes FuzzDirtyTypes()
{
    int32_t value = provider->ConsumeIntegral<int32_t>() % 8;
    if (value >= static_cast<int32_t>(Media::DirtyTypes::TYPE_SYNCED) &&
        value <= static_cast<int32_t>(Media::DirtyTypes::TYPE_COPY)) {
        return static_cast<Media::DirtyTypes>(value);
    }
    return Media::DirtyTypes::TYPE_SDIRTY;
}

static inline Media::CleanType FuzzCleanType()
{
    return provider->ConsumeBool() ? Media::CleanType::TYPE_NEED_CLEAN : Media::CleanType::TYPE_NOT_CLEAN;
}

static inline Media::DirtyType FuzzDirtyType()
{
    int32_t value = provider->ConsumeIntegral<int32_t>() % 8;
    if (value >= static_cast<int32_t>(Media::DirtyType::TYPE_SYNCED) &&
        value <= static_cast<int32_t>(Media::DirtyType::TYPE_COPY)) {
        return static_cast<Media::DirtyType>(value);
    }
    return Media::DirtyType::TYPE_RETRY;
}

static inline Media::CloudFilePosition FuzzCloudFilePosition()
{
    int32_t value = provider->ConsumeIntegral<int32_t>() % 2;
    if (value >= static_cast<int32_t>(Media::CloudFilePosition::POSITION_LOCAL) &&
        value <= static_cast<int32_t>(Media::CloudFilePosition::POSITION_CLOUD)) {
        return static_cast<Media::CloudFilePosition>(value);
    }
    return Media::CloudFilePosition::POSITION_LOCAL;
}

static inline Media::CloudSync::PhotoPosition FuzzPhotoPosition()
{
    int32_t value = provider->ConsumeIntegral<int32_t>() % 3;
    if (value >= static_cast<int32_t>(Media::CloudSync::PhotoPosition::POSITION_LOCAL) &&
        value <= static_cast<int32_t>(Media::CloudSync::PhotoPosition::POSITION_BOTH)) {
        return static_cast<Media::CloudSync::PhotoPosition>(value);
    }
    return Media::CloudSync::PhotoPosition::POSITION_BOTH;
}

static inline Media::SyncStatusType FuzzSyncStatusType()
{
    int32_t value = provider->ConsumeIntegral<int32_t>() % 2;
    if (value >= static_cast<int32_t>(Media::SyncStatusType::TYPE_BACKUP) &&
        value <= static_cast<int32_t>(Media::SyncStatusType::TYPE_UPLOAD)) {
        return static_cast<Media::SyncStatusType>(value);
    }
    return Media::SyncStatusType::TYPE_VISIBLE;
}

static CloudMediaPullDataDto FuzzCloudMediaPullDataDto(string &cloudId)
{
    CloudMediaPullDataDto pullData;
    pullData.attributesTitle = provider->ConsumeBool() ? provider->ConsumeBytesAsString(NUM_BYTES) : "";
    pullData.hasProperties = true;
    pullData.propertiesSourceFileName = "." + provider->ConsumeBytesAsString(NUM_BYTES);
    pullData.basicFileType = provider->ConsumeBool() ? FILE_TYPE_LIVEPHOTO : FILE_TYPE_VIDEO;
    pullData.basicEditedTime = provider->ConsumeIntegral<int64_t>();
    pullData.basicFileName = "IMG_20250425_123456.jpg";
    pullData.cloudId = cloudId;
    pullData.propertiesSourcePath =  provider->ConsumeBool() ? "/Pictures/Screenshots/DCIM/Camera" : "";
    pullData.hasAttributes = true;
    pullData.attributesMediaType = provider->ConsumeIntegral<int64_t>();
    pullData.duration = provider->ConsumeIntegral<int32_t>();
    pullData.attributesHidden = provider->ConsumeIntegral<int32_t>();
    pullData.attributesHiddenTime = provider->ConsumeIntegral<int64_t>();
    pullData.attributesRelativePath = provider->ConsumeBytesAsString(NUM_BYTES);
    pullData.attributesVirtualPath = provider->ConsumeBytesAsString(NUM_BYTES);
    pullData.attributesMetaDateModified = provider->ConsumeIntegral<int64_t>();
    pullData.attributesSubtype = provider->ConsumeIntegral<int32_t>();
    pullData.attributesBurstCoverLevel = provider->ConsumeIntegral<int32_t>();
    pullData.attributesBurstKey = provider->ConsumeBytesAsString(NUM_BYTES);
    pullData.attributesDateYear = provider->ConsumeBytesAsString(NUM_BYTES);
    pullData.attributesDateMonth = provider->ConsumeBytesAsString(NUM_BYTES);
    pullData.attributesDateDay = provider->ConsumeBytesAsString(NUM_BYTES);
    pullData.attributesShootingMode = provider->ConsumeBytesAsString(NUM_BYTES);
    pullData.attributesShootingModeTag = provider->ConsumeBytesAsString(NUM_BYTES);
    pullData.attributesDynamicRangeType = provider->ConsumeIntegral<int32_t>();
    pullData.attributesFrontCamera = provider->ConsumeBytesAsString(NUM_BYTES);
    pullData.attributesEditTime = provider->ConsumeIntegral<int64_t>();
    pullData.attributesOriginalSubtype = provider->ConsumeIntegral<int32_t>();
    pullData.attributesCoverPosition = provider->ConsumeIntegral<int64_t>();
    pullData.attributesMovingPhotoEffectMode = provider->ConsumeIntegral<int32_t>();
    pullData.attributesSupportedWatermarkType = provider->ConsumeIntegral<int32_t>();
    pullData.attributesStrongAssociation = provider->ConsumeIntegral<int32_t>();
    pullData.attributesFileId = FILE_ID;
    return pullData;
}

static int32_t InsertPhotoAsset(string &cloudId)
{
    if (g_rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutString(PhotoColumn::PHOTO_CLOUD_ID, cloudId);
    values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(FuzzDirtyTypes()));
    values.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(FuzzCloudFilePosition()));
    values.PutInt(PhotoColumn::PHOTO_THUMBNAIL_READY, static_cast<int32_t>(ThumbnailReady::GENERATE_THUMB_COMPLETED));
    values.PutInt(PhotoColumn::PHOTO_LCD_VISIT_TIME, static_cast<int32_t>(LcdReady::GENERATE_LCD_COMPLETED));
    values.PutLong(PhotoColumn::MEDIA_DATE_TRASHED, 0);
    values.PutLong(PhotoColumn::MEDIA_TIME_PENDING, 0);
    values.PutString(PhotoColumn::MEDIA_NAME, "IMG_20250425_123456.jpg");
    int64_t fileId = 0;
    g_rdbStore->Insert(fileId, PHOTOS_TABLE, values);
    return static_cast<int32_t>(fileId);
}

static int32_t UpdatePhotoAsset(string &cloudId)
{
    if (g_rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutString(PhotoColumn::PHOTO_CLOUD_ID, cloudId);
    values.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(FuzzCleanType()));
    values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(FuzzDirtyType()));
    values.PutInt(PhotoColumn::PHOTO_POSITION, FuzzPhotoPosition());
    values.PutLong(PhotoColumn::PHOTO_CLOUD_VERSION, 0);
    values.PutInt(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(FuzzSyncStatusType()));
    values.PutNull(PhotoColumn::PHOTO_ORIGINAL_ASSET_CLOUD_ID);
    values.PutLong(PhotoColumn::PHOTO_META_DATE_MODIFIED, provider->ConsumeIntegral<int32_t>());
    int64_t fileId = 0;
    g_rdbStore->Insert(fileId, PHOTOS_TABLE, values);
    return static_cast<int32_t>(fileId);
}

static int32_t QueryPhotoAsset()
{
    if (g_rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::MEDIA_SIZE, provider->ConsumeIntegral<int64_t>());
    values.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, provider->ConsumeIntegral<int32_t>());
    values.PutInt(PhotoColumn::MEDIA_HIDDEN, provider->ConsumeBool());
    values.PutInt(PhotoColumn::PHOTO_ORIENTATION, provider->ConsumeIntegral<int32_t>());
    values.PutString(PhotoColumn::MEDIA_NAME, "IMG_20250425_123456.jpg");
    int64_t fileId = 0;
    g_rdbStore->Insert(fileId, PHOTOS_TABLE, values);
    return static_cast<int32_t>(fileId);
}

static int32_t QueryPhotomapAsset()
{
    if (g_rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoMap::ASSET_ID, 1);
    int64_t fileId = 0;
    g_rdbStore->Insert(fileId, PHOTO_MAP_TABLE, values);
    return static_cast<int32_t>(fileId);
}

static int32_t QueryAlbumAsset()
{
    if (g_rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, static_cast<int32_t>(PhotoAlbumType::USER));
    values.PutInt(PhotoAlbumColumns::ALBUM_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
    values.PutString(PhotoAlbumColumns::ALBUM_CLOUD_ID, HIDDEN_ALBUM);
    values.PutString(PhotoAlbumColumns::ALBUM_LPATH, "records");
    int64_t fileId = 0;
    g_rdbStore->Insert(fileId, PHOTO_ALBUM_TABLE, values);
    return static_cast<int32_t>(fileId);
}

static int32_t InsertDeleteAsset(string &cloudId)
{
    if (g_rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutString(Media::PhotoColumn::PHOTO_CLOUD_ID, cloudId);
    values.PutString(MediaColumn::MEDIA_FILE_PATH, provider->ConsumeBytesAsString(NUM_BYTES));
    int64_t fileId = 0;
    g_rdbStore->Insert(fileId, PHOTOS_TABLE, values);
    return static_cast<int32_t>(fileId);
}

static void BatchInsertFileFuzzer()
{
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    std::map<std::string, int> recordAnalysisAlbumMaps;
    recordAnalysisAlbumMaps.insert(std::make_pair(PhotoMap::ALBUM_ID, provider->ConsumeIntegral<int32_t>()));

    std::map<std::string, std::set<int>> recordAlbumMaps;
    recordAlbumMaps.insert(std::make_pair(PhotoMap::ALBUM_ID, std::set<int>({provider->ConsumeIntegral<int32_t>()})));
    recordAlbumMaps.insert(std::make_pair(PhotoMap::DIRTY,
        std::set<int>({static_cast<int32_t>(FuzzDirtyTypes())})));

    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutString(MediaColumn::MEDIA_FILE_PATH, provider->ConsumeBytesAsString(NUM_BYTES));
    std::vector<NativeRdb::ValuesBucket> insertFiles;
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    cloudMediaPhotosDao->BatchInsertFile(recordAnalysisAlbumMaps, recordAlbumMaps, insertFiles, photoRefresh);

    insertFiles.push_back(valuesBucket);
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    InsertPhotoAsset(cloudId);
    cloudMediaPhotosDao->BatchInsertFile(recordAnalysisAlbumMaps, recordAlbumMaps, insertFiles, photoRefresh);
}

static void UpdateRecordToDatabaseFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    CloudMediaPullDataDto pullData = FuzzCloudMediaPullDataDto(cloudId);
    bool isLocal = true;
    bool mtimeChanged = provider->ConsumeBool();
    std::set<std::string> refreshAlbums;
    std::vector<int32_t> stats(VECTOR_SIZE, 0);
    UpdatePhotoAsset(cloudId);
    QueryPhotomapAsset();
    cloudMediaPhotosDao->UpdateRecordToDatabase(pullData, isLocal, mtimeChanged, refreshAlbums, stats, photoRefresh);
}

static void ConflictDataMergeFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    CloudMediaPullDataDto pullData = FuzzCloudMediaPullDataDto(cloudId);
    std::string fullPath;
    bool cloudStd = provider->ConsumeBool();
    std::set<int32_t> albumIds;
    std::set<std::string> refreshAlbums;
    cloudMediaPhotosDao->ConflictDataMerge(pullData, fullPath, cloudStd, albumIds, refreshAlbums, photoRefresh);
}

static void GetInsertParamsFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    CloudMediaPullDataDto pullData = FuzzCloudMediaPullDataDto(cloudId);
    std::map<std::string, int> recordAnalysisAlbumMaps;
    std::map<std::string, std::set<int>> recordAlbumMaps;
    std::set<std::string> refreshAlbums;
    std::vector<NativeRdb::ValuesBucket> insertFiles;
    cloudMediaPhotosDao->GetInsertParams(pullData, recordAnalysisAlbumMaps, recordAlbumMaps,
        refreshAlbums, insertFiles);
}

static void BatchQueryLocalFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    vector<CloudMediaPullDataDto> datas;
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    CloudMediaPullDataDto pullData = FuzzCloudMediaPullDataDto(cloudId);
    datas.emplace_back(pullData);
    std::vector<std::string> columns;
    int32_t rowCount = -1;
    cloudMediaPhotosDao->BatchQueryLocal(datas, columns, rowCount);
}

static void GetLocalKeyDataFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    KeyData localKeyData;
    QueryPhotoAsset();
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_NAME, "IMG_20250425_123456.jpg");
    std::vector<std::string> columns;
    columns.emplace_back(PhotoColumn::PHOTO_OWNER_ALBUM_ID);
    auto resultSet = g_rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Failed to query.");
    QueryAlbumAsset();
    cloudMediaPhotosDao->GetLocalKeyData(localKeyData, resultSet);
}

static void JudgeConflictFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    CloudMediaPullDataDto pullData = FuzzCloudMediaPullDataDto(cloudId);
    KeyData localKeyData;
    KeyData cloudKeyData;
    cloudMediaPhotosDao->JudgeConflict(pullData, localKeyData, cloudKeyData);

    pullData.attributesSrcAlbumIds.emplace_back("default-album-4");
    cloudMediaPhotosDao->JudgeConflict(pullData, localKeyData, cloudKeyData);

    localKeyData.sourceAlbum = "default-album-4";
    localKeyData.lPath = provider->ConsumeBool() ? "localKeyData_lPath" : "lPath";
    cloudKeyData.lPath = provider->ConsumeBool() ? "cloudKeyData_lPath" : "lPath";
    cloudMediaPhotosDao->JudgeConflict(pullData, localKeyData, cloudKeyData);

    localKeyData.isize = provider->ConsumeIntegral<int64_t>();
    cloudKeyData.isize = provider->ConsumeIntegral<int64_t>();
    cloudMediaPhotosDao->JudgeConflict(pullData, localKeyData, cloudKeyData);

    localKeyData.lPath = provider->ConsumeBool() ? "localKeyData_lPath" : "lPath";
    cloudKeyData.lPath = provider->ConsumeBool() ? "cloudKeyData_lPath" : "lPath";
    cloudMediaPhotosDao->JudgeConflict(pullData, localKeyData, cloudKeyData);

    localKeyData.displayName = "localKeyData_displayName";
    cloudKeyData.displayName = "cloudKeyData_displayName";
    cloudMediaPhotosDao->JudgeConflict(pullData, localKeyData, cloudKeyData);
}

static void GetRecordsFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    std::vector<std::string> cloudIds;
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    InsertPhotoAsset(cloudId);
    cloudMediaPhotosDao->GetRetryRecords(cloudIds);
    cloudMediaPhotosDao->GetCheckRecords(cloudIds);
    cloudIds.emplace_back(cloudId);
    cloudIds.emplace_back(provider->ConsumeBytesAsString(NUM_BYTES));
    cloudMediaPhotosDao->GetRetryRecords(cloudIds);
    cloudMediaPhotosDao->GetCheckRecords(cloudIds);

    int32_t querySize = provider->ConsumeIntegral<uint32_t>() & 0xf;
    std::vector<PhotosPo> createdRecords;
    std::vector<PhotosPo> cloudRecordPoList;
    std::vector<PhotosPo> copyRecords;
    cloudMediaPhotosDao->GetCreatedRecords(querySize, createdRecords);
    int32_t dirtyType = static_cast<int32_t>(FuzzDirtyType());
    cloudMediaPhotosDao->GetMetaModifiedRecords(querySize, cloudRecordPoList, dirtyType);
    cloudMediaPhotosDao->GetFileModifiedRecords(querySize, cloudRecordPoList);
    cloudMediaPhotosDao->GetCopyRecords(querySize, copyRecords);
}

static void GetDeletedRecordsAssetFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    int32_t limitSize = provider->ConsumeBool() ? provider->ConsumeIntegral<int32_t>() : -1;
    std::vector<PhotosPo> cloudRecordPoList;
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    InsertPhotoAsset(cloudId);
    cloudMediaPhotosDao->GetDeletedRecordsAsset(limitSize, cloudRecordPoList);
}

static void GetPhotoLocalInfoFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    std::vector<PhotosDto> records;
    PhotosDto photo;
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    photo.cloudId = cloudId;
    photo.fileId = provider->ConsumeIntegral<int32_t>();
    records.emplace_back(photo);
    std::unordered_map<std::string, LocalInfo> infoMap;
    std::string type = provider->ConsumeBool() ? PhotoColumn::PHOTO_CLOUD_ID : PhotoColumn::PHOTO_CLOUD_VERSION;
    InsertPhotoAsset(cloudId);
    cloudMediaPhotosDao->GetPhotoLocalInfo(records, infoMap, type);
}

static void UpdateLocalAlbumMapFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    InsertPhotoAsset(cloudId);
    cloudMediaPhotosDao->UpdateLocalAlbumMap(cloudId);
}

static void GetSameNamePhotoCountFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    PhotosDto photo;
    photo.displayName = "IMG_20250425_123456.jpg";
    photo.size = provider->ConsumeIntegral<int64_t>() & 0xff;
    photo.ownerAlbumId = provider->ConsumeIntegral<int32_t>() & 0xf;
    photo.mediaType = MediaType::MEDIA_TYPE_IMAGE;
    photo.rotation = provider->ConsumeIntegral<int32_t>() & 0xf;
    bool isHide = provider->ConsumeBool();
    int32_t count = 1;
    QueryPhotoAsset();
    cloudMediaPhotosDao->GetSameNamePhotoCount(photo, isHide, count);
}

static void UpdatePhotoCreatedRecordFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    PhotosDto record;
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    record.fileId = provider->ConsumeIntegral<int32_t>() & 0xf;
    record.cloudId = cloudId;
    record.version = provider->ConsumeIntegral<int64_t>() & 0xf;
    std::unordered_map<std::string, LocalInfo> localMap;
    UpdatePhotoAsset(cloudId);
    cloudMediaPhotosDao->UpdatePhotoCreatedRecord(record, localMap, photoRefresh);
}

static void OnModifyPhotoRecordFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    PhotosDto record;
    cloudMediaPhotosDao->OnModifyPhotoRecord(record, photoRefresh);
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    record.cloudId = cloudId;
    record.version = provider->ConsumeIntegral<int64_t>();
    UpdatePhotoAsset(cloudId);
    cloudMediaPhotosDao->OnModifyPhotoRecord(record, photoRefresh);
}

static void UpdateFdirtyVersionFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    PhotosDto record;
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    record.cloudId = cloudId;
    record.version = provider->ConsumeIntegral<int64_t>();
    UpdatePhotoAsset(cloudId);
    cloudMediaPhotosDao->UpdateFdirtyVersion(record, photoRefresh);
}

static void OnDeleteRecordsAssetFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    PhotosDto record;
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    record.cloudId = cloudId;
    record.version = provider->ConsumeIntegral<int64_t>();
    UpdatePhotoAsset(cloudId);
    cloudMediaPhotosDao->OnDeleteRecordsAsset(record, photoRefresh);
}

static void OnCopyPhotoRecordFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    PhotosDto record;
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    record.cloudId = cloudId;
    record.fileId = provider->ConsumeIntegral<int32_t>() & 0xf;
    record.path = provider->ConsumeBytesAsString(NUM_BYTES);
    record.version = provider->ConsumeIntegral<int64_t>() & 0xf;
    record.cloudVersion = 0;
    UpdatePhotoAsset(cloudId);
    cloudMediaPhotosDao->OnCopyPhotoRecord(record, photoRefresh);
}

static void ClearCloudInfoFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    UpdatePhotoAsset(cloudId);
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> photoRefresh;
    cloudMediaPhotosDao->ClearCloudInfo(cloudId, photoRefresh);
}

static void DeleteFileNotExistPhotoFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    std::string path;
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    InsertDeleteAsset(cloudId);
    cloudMediaPhotosDao->DeleteFileNotExistPhoto(path, photoRefresh);
}

static void HandleSameNameRenameFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    PhotosDto photo;
    photo.fileName = "image.jpg";
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    UpdatePhotoAsset(cloudId);
    cloudMediaPhotosDao->HandleSameNameRename(photo, photoRefresh);
}

static void UpdatePhotoVisibleFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    UpdatePhotoAsset(cloudId);
    cloudMediaPhotosDao->UpdatePhotoVisible();
}

static void UpdateAlbumInternalFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    std::set<std::string> refreshAlbums;
    refreshAlbums.insert(provider->ConsumeBytesAsString(NUM_BYTES));
    cloudMediaPhotosDao->UpdateAlbumInternal(refreshAlbums);
}

static void SetRetryFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    UpdatePhotoAsset(cloudId);
    cloudMediaPhotosDao->SetRetry(cloudId);
}

static void DeleteLocalByCloudIdFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    InsertDeleteAsset(cloudId);
    cloudMediaPhotosDao->DeleteLocalByCloudId(cloudId, photoRefresh);
}

static void UpdateFailRecordsCloudIdFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    PhotosDto record;
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    record.cloudId = cloudId;
    record.fileId = 1;
    std::unordered_map<std::string, LocalInfo> localMap;
    localMap.insert({"1", LocalInfo()});
    cloudMediaPhotosDao->UpdateFailRecordsCloudId(record, localMap, photoRefresh);
}

static void InsertAndRemoveFailedRecordFuzzer()
{
    if (cloudMediaPhotosDao == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosDao is nullptr");
        return;
    }
    int32_t fileId = provider->ConsumeIntegral<int32_t>();
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    cloudMediaPhotosDao->InsertPhotoCreateFailedRecord(fileId);
    cloudMediaPhotosDao->InsertPhotoModifyFailedRecord(cloudId);
    cloudMediaPhotosDao->InsertPhotoCopyFailedRecord(fileId);
    cloudMediaPhotosDao->RemovePhotoCreateFailedRecord(fileId);
    cloudMediaPhotosDao->RemovePhotoModifyFailedRecord(cloudId);
    cloudMediaPhotosDao->RemovePhotoCopyFailedRecord(fileId);
    cloudMediaPhotosDao->ClearPhotoFailedRecords();
}

static void MediaLibraryCloudMediaPhotosDaoFuzzer()
{
    BatchInsertFileFuzzer();
    UpdateRecordToDatabaseFuzzer();
    ConflictDataMergeFuzzer();
    GetInsertParamsFuzzer();
    BatchQueryLocalFuzzer();
    GetLocalKeyDataFuzzer();
    JudgeConflictFuzzer();
    GetRecordsFuzzer();

    GetDeletedRecordsAssetFuzzer();
    GetPhotoLocalInfoFuzzer();
    UpdateLocalAlbumMapFuzzer();
    GetSameNamePhotoCountFuzzer();

    UpdatePhotoCreatedRecordFuzzer();
    OnModifyPhotoRecordFuzzer();
    UpdateFdirtyVersionFuzzer();
    OnDeleteRecordsAssetFuzzer();
    OnCopyPhotoRecordFuzzer();
    ClearCloudInfoFuzzer();

    DeleteFileNotExistPhotoFuzzer();
    HandleSameNameRenameFuzzer();
    UpdatePhotoVisibleFuzzer();
    UpdateAlbumInternalFuzzer();
    SetRetryFuzzer();
    DeleteLocalByCloudIdFuzzer();
    UpdateFailRecordsCloudIdFuzzer();
    InsertAndRemoveFailedRecordFuzzer();
}

void SetTables()
{
    vector<string> createTableSqlList = {
        PhotoColumn::CREATE_PHOTO_TABLE,
        PhotoAlbumColumns::CREATE_TABLE,
        PhotoMap::CREATE_TABLE,
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
    /* Run your code on data */
    FuzzedDataProvider fdp(data, size);
    OHOS::provider = &fdp;
    if (data == nullptr) {
        return 0;
    }
    OHOS::MediaLibraryCloudMediaPhotosDaoFuzzer();
    OHOS::ClearKvStore();
    return 0;
}
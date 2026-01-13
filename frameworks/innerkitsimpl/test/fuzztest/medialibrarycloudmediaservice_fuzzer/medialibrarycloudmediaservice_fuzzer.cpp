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

#include "medialibrarycloudmediaservice_fuzzer.h"

#include <cstdint>
#include <string>
#include <fstream>
#include <fuzzer/FuzzedDataProvider.h>
#define private public
#define protected public
#include "cloud_media_album_service.h"
#include "cloud_media_data_service.h"
#include "cloud_media_photos_service.h"
#include "cloud_media_download_service.h"
#include "cloud_media_data_service_processor.h"
#include "cloud_media_photo_service_processor.h"
#undef private
#include "ability_context_impl.h"
#include "media_log.h"
#include "medialibrary_command.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "cloud_media_operation_code.h"
#include "medialibrary_kvstore_manager.h"
#include "media_upgrade.h"

using ChangeType = OHOS::AAFwk::ChangeInfo::ChangeType;
namespace OHOS {
using namespace std;
using namespace OHOS::Media;
using namespace OHOS::Media::CloudSync;
const int32_t FILEID = 1;
const int32_t NUM_BYTES = 1;
const int32_t VECTOR_SIZE = 5;
const int32_t MAX_PROPERTIES_ROTATE = 8;
const int32_t MAX_BYTE_VALUE = 256;
const int32_t SEED_SIZE = 1024;
const string PHOTOS_TABLE = "Photos";
FuzzedDataProvider* provider;
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;
shared_ptr<CloudMediaAlbumService> cloudMediaAlbumService = make_shared<CloudMediaAlbumService>();
shared_ptr<CloudMediaDataService> cloudMediaDataService = make_shared<CloudMediaDataService>();
shared_ptr<CloudMediaPhotosService> cloudMediaPhotosService = make_shared<CloudMediaPhotosService>();
shared_ptr<CloudMediaDownloadService> cloudMediaDownloadService = make_shared<CloudMediaDownloadService>();

static inline MediaType FuzzMediaType()
{
    int32_t value = provider->ConsumeIntegral<int32_t>() % 14;
    if (value >= static_cast<int32_t>(MediaType::MEDIA_TYPE_FILE) &&
        value <= static_cast<int32_t>(MediaType::MEDIA_TYPE_DEFAULT)) {
        return static_cast<Media::MediaType>(value);
    }
    return MediaType::MEDIA_TYPE_VIDEO;
}

static inline DirtyType FuzzDirtyType()
{
    int32_t value = provider->ConsumeIntegral<int32_t>() % 8;
    if (value >= static_cast<int32_t>(DirtyType::TYPE_SYNCED) &&
        value <= static_cast<int32_t>(DirtyType::TYPE_COPY)) {
        return static_cast<DirtyType>(value);
    }
    return DirtyType::TYPE_RETRY;
}

static inline PhotoPositionType FuzzPhotoPositionType()
{
    int32_t value = provider->ConsumeIntegral<int32_t>() % 3;
    if (value >= static_cast<int32_t>(PhotoPositionType::LOCAL) &&
        value <= static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD)) {
        return static_cast<PhotoPositionType>(value);
    }
    return PhotoPositionType::CLOUD;
}

static inline Media::SyncStatusType FuzzSyncStatusType()
{
    int32_t value = provider->ConsumeIntegral<int32_t>() % 2;
    if (value >= static_cast<int32_t>(SyncStatusType::TYPE_BACKUP) &&
        value <= static_cast<int32_t>(SyncStatusType::TYPE_UPLOAD)) {
        return static_cast<SyncStatusType>(value);
    }
    return SyncStatusType::TYPE_VISIBLE;
}

static inline CloudSync::ThumbState FuzzThumbState()
{
    int32_t value = provider->ConsumeIntegral<int32_t>() % 3;
    if (value >= static_cast<int32_t>(CloudSync::ThumbState::DOWNLOADED) &&
        value <= static_cast<int32_t>(CloudSync::ThumbState::TO_DOWNLOAD)) {
        return static_cast<CloudSync::ThumbState>(value);
    }
    return CloudSync::ThumbState::TO_DOWNLOAD;
}

static inline Media::CloudSync::Clean FuzzClean()
{
    return provider->ConsumeBool() ? CloudSync::Clean::NEED_CLEAN : CloudSync::Clean::NOT_NEED_CLEAN;
}

static int32_t InsertPhotoAsset()
{
    if (g_rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::MEDIA_TYPE, static_cast<int32_t>(FuzzMediaType()));
    values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(FuzzDirtyType()));
    values.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(FuzzPhotoPositionType()));
    values.PutInt(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(FuzzSyncStatusType()));
    values.PutInt(PhotoColumn::PHOTO_THUMB_STATUS, static_cast<int32_t>(FuzzThumbState()));
    values.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(FuzzClean()));
    values.PutInt(PhotoColumn::PHOTO_ORIENTATION, provider->ConsumeIntegral<int32_t>());
    values.PutLong(PhotoColumn::MEDIA_DATE_TAKEN, provider->ConsumeIntegral<int64_t>());
    values.PutString(PhotoColumn::PHOTO_CLOUD_ID, "default-album-2");
    values.PutString(PhotoColumn::MEDIA_FILE_PATH, "/storage/cloud/files/Photo/16/IMG_1744362716_000.jpg");
    int64_t fileId = 0;
    g_rdbStore->Insert(fileId, PHOTOS_TABLE, values);
    return static_cast<int32_t>(fileId);
}

static int32_t InsertQueryAsset()
{
    if (g_rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::MEDIA_TYPE, static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO));
    values.PutInt(PhotoColumn::MEDIA_ID, 1);
    values.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::CLOUD));
    values.PutInt(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE));
    values.PutInt(PhotoColumn::PHOTO_THUMB_STATUS, static_cast<int32_t>(FuzzThumbState()));
    values.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN));
    values.PutLong(PhotoColumn::MEDIA_DATE_TAKEN, provider->ConsumeIntegral<int64_t>());
    values.PutString(PhotoColumn::MEDIA_FILE_PATH, "/storage/cloud/files/Photo/16/IMG_1744362716_000.jpg");
    int64_t fileId = 0;
    g_rdbStore->Insert(fileId, PHOTOS_TABLE, values);
    return static_cast<int32_t>(fileId);
}

static PhotoAlbumDto FuzzPhotoAlbumDto()
{
    PhotoAlbumDto record;
    record.albumName = "albumName";
    record.bundleName = "bundleName";
    record.lPath = provider->ConsumeBytesAsString(NUM_BYTES);
    record.albumType = static_cast<int32_t>(PhotoAlbumType::INVALID);
    record.albumSubType = provider->ConsumeIntegral<int32_t>();
    record.albumDateAdded = provider->ConsumeIntegral<int64_t>();
    record.albumDateCreated = provider->ConsumeIntegral<int64_t>();
    record.albumDateModified = provider->ConsumeIntegral<int64_t>();
    record.localLanguage = provider->ConsumeBytesAsString(NUM_BYTES);
    record.cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    record.isSuccess = provider->ConsumeBool();
    return record;
}

static PhotosDto FuzzPhotosDto()
{
    PhotosDto photo;
    photo.cloudId = "default-album-2";
    photo.fileId = provider->ConsumeIntegral<uint32_t>() & 0xf;
    photo.localId = provider->ConsumeIntegral<int32_t>();
    photo.isSuccess = true;
    return photo;
}

static PhotosPo FuzzPhotosPo()
{
    PhotosPo photosPos;
    photosPos.data = provider->ConsumeBytesAsString(NUM_BYTES);
    return photosPos;
}

static AgingFileQueryDto FuzzAgingFileQueryDto()
{
    AgingFileQueryDto queryDto;
    queryDto.time = provider->ConsumeIntegral<int64_t>();
    queryDto.mediaType = static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO);
    queryDto.sizeLimit = provider->ConsumeIntegral<int32_t>();
    return queryDto;
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
    pullData.attributesFileId = FILEID;
    pullData.localPath = "file://media/Photo/1/IMG_1744362716_000/IMG_1744362716_000.jpg";
    pullData.propertiesRotate = provider->ConsumeIntegral<uint32_t>() % MAX_PROPERTIES_ROTATE;
    pullData.localThumbState = provider->ConsumeIntegral<int32_t>();
    pullData.localDirty = static_cast<int32_t>(FuzzDirtyType());
    pullData.localDateModified = provider->ConsumeBytesAsString(NUM_BYTES);
    pullData.attributesEditedTimeMs = provider->ConsumeIntegral<int64_t>();
    pullData.localDateAdded = provider->ConsumeBytesAsString(NUM_BYTES);
    pullData.basicSize = -1;
    return pullData;
}

static DownloadThumbnailQueryDto FuzzDownloadThumbnailQueryDto()
{
    DownloadThumbnailQueryDto queryDto;
    queryDto.size = provider->ConsumeIntegral<int32_t>();
    queryDto.type = provider->ConsumeIntegral<int32_t>();
    queryDto.offset  = provider->ConsumeIntegral<int32_t>();
    queryDto.isDownloadDisplayFirst = true;
    return queryDto;
}

static void CloudMediaAlbumServiceFuzzer()
{
    if (cloudMediaAlbumService == nullptr) {
        MEDIA_ERR_LOG("cloudMediaAlbumService is nuulptr");
        return;
    }
    int32_t limitSize = LIMIT_SIZE;
    bool isCloudSpaceFull = true;
    std::vector<PhotoAlbumPo> photoAlbumList;
    cloudMediaAlbumService->GetCreatedRecords(limitSize, isCloudSpaceFull, photoAlbumList);
    cloudMediaAlbumService->GetAlbumMetaModifiedRecords(limitSize);
    cloudMediaAlbumService->GetAlbumFileModifiedRecords(limitSize);
    cloudMediaAlbumService->GetAlbumDeletedRecords(limitSize);
    cloudMediaAlbumService->GetAlbumCopyRecords(limitSize);
    
    vector<PhotoAlbumDto> albumDtoList = { FuzzPhotoAlbumDto() };
    int32_t failSize = -1;
    OnFetchRecordsAlbumRespBody resp;
    cloudMediaAlbumService->OnCreateRecords(albumDtoList, failSize);
    cloudMediaAlbumService->OnMdirtyRecords(albumDtoList, failSize);
    cloudMediaAlbumService->OnFdirtyRecords();
    cloudMediaAlbumService->OnDeleteRecords(albumDtoList, failSize);

    cloudMediaAlbumService->OnFetchRecords(albumDtoList, resp);

    albumDtoList.clear();
    PhotoAlbumDto record = FuzzPhotoAlbumDto();
    albumDtoList.emplace_back(record);
    record.lPath = "";
    cloudMediaAlbumService->OnFetchRecords(albumDtoList, resp);

    record.lPath = "/Pictures/Screenshots";
    record.cloudId = "default-album-2";
    cloudMediaAlbumService->OnFetchRecords(albumDtoList, resp);
    cloudMediaAlbumService->ConvertToSingleScreenshots(record, albumDtoList);
    vector<string> failedAlbumIds = { "default" };
    cloudMediaAlbumService->OnDeleteAlbums(failedAlbumIds);
    record.isDelete = false;
    ChangeType type;
    cloudMediaAlbumService->HandleFetchOldRecordNew(record, type, resp);
    bool bContinue;
    cloudMediaAlbumService->HandleFetchOldRecord(record, bContinue, type, resp);
    cloudMediaAlbumService->OnStartSync();
    cloudMediaAlbumService->OnCompleteSync();
    MediaOperateResult optRet = {"", 0, ""};
    cloudMediaAlbumService->OnCompletePull(optRet);
    cloudMediaAlbumService->OnCompletePush();
    cloudMediaAlbumService->OnCompleteCheck();
}

static void CloudMediadataServiceFuzzer()
{
    if (cloudMediaDataService == nullptr) {
        MEDIA_ERR_LOG("cloudMediaDataService is nuulptr");
        return;
    }
    InsertPhotoAsset();
    string cloudId = "default-album-2";
    vector<string> cloudIds = { cloudId };
    
    cloudMediaDataService->UpdateDirty(cloudId, static_cast<int32_t>(FuzzDirtyType()));
    cloudMediaDataService->UpdatePosition(cloudIds, static_cast<int32_t>(FuzzPhotoPositionType()));
    cloudMediaDataService->UpdateSyncStatus(cloudId, static_cast<int32_t>(FuzzSyncStatusType()));
    cloudMediaDataService->UpdateThmStatus(cloudId,  static_cast<int32_t>(FuzzThumbState()));

    AgingFileQueryDto queryDto = FuzzAgingFileQueryDto();
    vector<PhotosDto> photosDtos = { FuzzPhotosDto() };
    vector<string> cloudIdList = { cloudId };
    cloudMediaDataService->GetAgingFile(queryDto, photosDtos);
    cloudMediaDataService->GetActiveAgingFile(queryDto, photosDtos);
    cloudMediaDataService->UpdateLocalFileDirty(cloudIdList);

    cloudMediaDataService->GetVideoToCache(photosDtos);

    cloudMediaDataService->GetFilePosStat();
    cloudMediaDataService->GetCloudThmStat();
    std::vector<uint64_t> dirtyTypeStat;
    cloudMediaDataService->GetDirtyTypeStat(dirtyTypeStat);
}

static void CloudMediaDownloadServiceFuzzer()
{
    if (cloudMediaDownloadService == nullptr) {
        MEDIA_ERR_LOG("cloudMediaDownloadService is nuulptr");
        return;
    }
    string cloudId = "default-album-2";
    vector<string> cloudIds = { cloudId };
    int32_t type = provider->ConsumeIntegral<int32_t>() & 0x7fffffff;
    vector<MediaOperateResultDto> result;
    std::unordered_map<string, int32_t> downloadThumbnailMap = {
        {"thblum", CloudMediaDownloadService::TYPE_THM},
        {"lcd", CloudMediaDownloadService::TYPE_LCD},
        {"thm_and_lcd", CloudMediaDownloadService::TYPE_THM_AND_LCD}
    };
    int32_t totalNum = 0;
    vector<int32_t> fileIds = { 1 };
    DownloadThumbnailQueryDto queryDto = FuzzDownloadThumbnailQueryDto();
    vector<PhotosDto> photosDtos = { FuzzPhotosDto() };
    InsertQueryAsset();
    cloudMediaDownloadService->GetDownloadAsset(fileIds);
    cloudMediaDownloadService->GetDownloadThmsByUri(fileIds, type);
    cloudMediaDownloadService->OnDownloadAsset(cloudIds, result);
    cloudMediaDownloadService->GetDownloadThms(queryDto, photosDtos);
    cloudMediaDownloadService->OnDownloadThms(downloadThumbnailMap, result);
    cloudMediaDownloadService->GetDownloadThmNum(type, totalNum);
}

static void OnRecordFailedFuzzer()
{
    if (cloudMediaPhotosService == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosService is nuulptr");
        return;
    }
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    PhotosDto photo;
    photo.serverErrorCode = ServerErrorCode::NETWORK_ERROR;
    cloudMediaPhotosService->OnRecordFailed(photo, photoRefresh);

    photo.serverErrorCode = ServerErrorCode::UID_EMPTY;
    cloudMediaPhotosService->OnRecordFailed(photo, photoRefresh);

    photo.serverErrorCode = ServerErrorCode::INVALID_LOCK_PARAM;
    cloudMediaPhotosService->OnRecordFailed(photo, photoRefresh);

    photo.serverErrorCode = ServerErrorCode::RESPONSE_TIME_OUT;
    photo.errorType = ErrorType::TYPE_UNKNOWN;
    cloudMediaPhotosService->OnRecordFailed(photo, photoRefresh);

    photo.errorType = ErrorType::TYPE_NOT_NEED_RETRY;
    cloudMediaPhotosService->OnRecordFailed(photo, photoRefresh);

    photo.serverErrorCode = ServerErrorCode::NO_NETWORK;
    photo.errorType = ErrorType::TYPE_UNKNOWN;
    CloudErrorDetail cloudErrorDetail;
    cloudErrorDetail.detailCode = ErrorDetailCode::SPACE_FULL;
    photo.errorDetails = { cloudErrorDetail };
    cloudMediaPhotosService->OnRecordFailed(photo, photoRefresh);

    cloudErrorDetail.detailCode = ErrorDetailCode::BUSINESS_MODEL_CHANGE_DATA_UPLOAD_FORBIDDEN;
    photo.errorDetails = { cloudErrorDetail };
    cloudMediaPhotosService->OnRecordFailed(photo, photoRefresh);

    cloudErrorDetail.detailCode = ErrorDetailCode::SAME_FILENAME_NOT_ALLOWED;
    photo.errorDetails = { cloudErrorDetail };
    cloudMediaPhotosService->OnRecordFailed(photo, photoRefresh);

    cloudErrorDetail.detailCode = ErrorDetailCode::CONTENT_NOT_FIND;
    photo.errorDetails = { cloudErrorDetail };
    cloudMediaPhotosService->OnRecordFailed(photo, photoRefresh);

    cloudErrorDetail.detailCode = ErrorDetailCode::FILE_REFERENCED;
    photo.errorDetails = { cloudErrorDetail };
    photo.errorType = ErrorType::TYPE_NOT_NEED_RETRY;
    cloudMediaPhotosService->OnRecordFailed(photo, photoRefresh);
}

static void PullDeleteAndUpdateFuzzer()
{
    if (cloudMediaPhotosService == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosService is nuulptr");
        return;
    }
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    CloudMediaPullDataDto pullData = FuzzCloudMediaPullDataDto(cloudId);
    std::set<string> refreshAlbums;
    vector<PhotosDto> fdirtyData;
    vector<int32_t> stats(VECTOR_SIZE, 0);
    cloudMediaPhotosService->PullDelete(pullData, refreshAlbums, photoRefresh);
    cloudMediaPhotosService->PullUpdate(pullData, refreshAlbums, fdirtyData, stats, photoRefresh);
}

static void GetMergeDataMapFuzzer()
{
    if (cloudMediaPhotosService == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosService is nuulptr");
        return;
    }
    vector<CloudMediaPullDataDto> pullDatas;
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    CloudMediaPullDataDto pullData = FuzzCloudMediaPullDataDto(cloudId);
    pullDatas.emplace_back(pullData);
    std::map<string, KeyData> mergeDataMap;
    cloudMediaPhotosService->GetMergeDataMap(pullDatas, mergeDataMap);
}

static void OnRecordFuzzer()
{
    if (cloudMediaPhotosService == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotosService is nuulptr");
        return;
    }
    vector<PhotosDto> photos;
    PhotosDto photo;
    photos.emplace_back(photo);
    int32_t failedSize = -1;
    InsertQueryAsset();
    cloudMediaPhotosService->OnCreateRecords(photos, failedSize);
    cloudMediaPhotosService->OnMdirtyRecords(photos, failedSize);
    cloudMediaPhotosService->OnFdirtyRecords(photos, failedSize);
    cloudMediaPhotosService->OnDeleteRecords(photos, failedSize);
    cloudMediaPhotosService->OnCopyRecords(photos, failedSize);

    photos.clear();
    photo = FuzzPhotosDto();
    photos.emplace_back(photo);
    cloudMediaPhotosService->OnCreateRecords(photos, failedSize);
    cloudMediaPhotosService->OnMdirtyRecords(photos, failedSize);
    cloudMediaPhotosService->OnFdirtyRecords(photos, failedSize);
    cloudMediaPhotosService->OnDeleteRecords(photos, failedSize);
    cloudMediaPhotosService->OnCopyRecords(photos, failedSize);
}

static void CloudMediadPhotoServiceFuzzer()
{
    CHECK_AND_RETURN_LOG(cloudMediaPhotosService != nullptr, "cloudMediaPhotosService is nuulptr");

    MediaOperateResult optRet = {"", 0, ""};
    cloudMediaPhotosService->OnStartSync();
    cloudMediaPhotosService->OnCompleteSync();
    cloudMediaPhotosService->OnCompletePull(optRet);
    cloudMediaPhotosService->OnCompletePush();
    cloudMediaPhotosService->OnCompleteCheck();
    PullDeleteAndUpdateFuzzer();
    GetMergeDataMapFuzzer();

    string cloudId = "default-album-2";
    vector<string> cloudIds = { cloudId };
    int32_t recordsSize = provider->ConsumeIntegral<uint32_t>() & 0xf;
    vector<PhotosPo> photosPo;
    cloudMediaPhotosService->GetCreatedRecords(recordsSize, photosPo);
    int32_t dirtyType = static_cast<int32_t>(FuzzDirtyType());
    cloudMediaPhotosService->GetMetaModifiedRecords(recordsSize, photosPo, dirtyType);
    cloudMediaPhotosService->GetFileModifiedRecords(recordsSize, photosPo);
    cloudMediaPhotosService->GetCopyRecords(recordsSize, photosPo);
    cloudMediaPhotosService->GetRetryRecords(cloudIds);

    PhotosDto photo;
    OnRecordFuzzer();
    OnRecordFailedFuzzer();

    cloudIds.clear();
    cloudIds.emplace_back(cloudId);
    std::map<string, CloudMediaPullDataDto> cloudIdRelativeMap = { {"default-album-2", CloudMediaPullDataDto()} };
    vector<PhotosDto> newData;
    vector<PhotosDto> fdirtyData;
    vector<int32_t> stats(VECTOR_SIZE, 0);
    vector<string> failedRecords;
    cloudMediaPhotosService->HandleRecord(cloudIds, cloudIdRelativeMap, newData, fdirtyData, stats, failedRecords);

    cloudIdRelativeMap = { {"default-album-2", FuzzCloudMediaPullDataDto(cloudId)} };
    cloudMediaPhotosService->HandleRecord(cloudIds, cloudIdRelativeMap, newData, fdirtyData, stats, failedRecords);

    CloudMediaPullDataDto pullData = FuzzCloudMediaPullDataDto(cloudId);
    pullData.basicIsDelete = true;
    cloudIdRelativeMap = { {"default-album-2", pullData} };
    cloudMediaPhotosService->HandleRecord(cloudIds, cloudIdRelativeMap, newData, fdirtyData, stats, failedRecords);

    CloudMediaPullDataDto data = FuzzCloudMediaPullDataDto(cloudId);
    cloudMediaPhotosService->ConvertPullDataToPhotosDto(data, photo);
    vector<CloudMediaPullDataDto> pullDatas = { FuzzCloudMediaPullDataDto(cloudId) };
    cloudMediaPhotosService->OnDentryFileInsert(pullDatas, failedRecords);

    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    PhotosDto record = FuzzPhotosDto();
    std::unordered_map<std::string, LocalInfo> localMap = { {"1", LocalInfo()} };
    cloudMediaPhotosService->OnCreateRecordSuccess(record, localMap, photoRefresh);

    record.localId = 1;
    cloudMediaPhotosService->OnCreateRecordSuccess(record, localMap, photoRefresh);
}

static void CloudMediaServiceProcessorFuzzer()
{
    shared_ptr<CloudMediaDataServiceProcessor> cloudMediaDataServiceProcessor =
        make_shared<CloudMediaDataServiceProcessor>();
    shared_ptr<CloudMediaPhotoServiceProcessor> cloudMediaPhotoServiceProcessor =
        make_shared<CloudMediaPhotoServiceProcessor>();
    vector<PhotosPo> photosPos = { FuzzPhotosPo() };
    vector<PhotosDto> photosDtos;
    if (cloudMediaDataServiceProcessor == nullptr || cloudMediaPhotoServiceProcessor == nullptr) {
        MEDIA_ERR_LOG("cloudMediaDataServiceProcessor and cloudMediaPhotoServiceProcessor is nuulptr");
        return;
    }
    cloudMediaDataServiceProcessor->GetPhotosDto(photosPos, photosDtos);
    cloudMediaPhotoServiceProcessor->GetPhotosDtos(photosPos);
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
    OHOS::CloudMediaAlbumServiceFuzzer();
    OHOS::CloudMediadataServiceFuzzer();
    OHOS::CloudMediaDownloadServiceFuzzer();
    OHOS::CloudMediadPhotoServiceFuzzer();
    OHOS::CloudMediaServiceProcessorFuzzer();
    OHOS::ClearKvStore();
    return 0;
}
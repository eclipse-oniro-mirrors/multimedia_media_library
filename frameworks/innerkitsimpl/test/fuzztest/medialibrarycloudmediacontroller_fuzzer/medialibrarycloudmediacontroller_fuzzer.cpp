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

#include "medialibrarycloudmediacontroller_fuzzer.h"

#include <cstdint>
#include <string>
#include <fstream>
#include <fuzzer/FuzzedDataProvider.h>
#define private public
#include "cloud_media_album_controller_service.h"
#include "cloud_media_data_controller_service.h"
#include "cloud_media_download_controller_service.h"
#include "cloud_media_photo_controller_service.h"
#include "cloud_media_album_controller_processor.h"
#include "cloud_media_data_controller_processor.h"
#include "cloud_media_download_controller_processor.h"
#include "cloud_media_photo_controller_processor.h"
#undef private
#include "ability_context_impl.h"
#include "media_log.h"
#include "medialibrary_command.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"

namespace OHOS {
using namespace std;
using namespace OHOS::Media;
using namespace OHOS::Media::CloudSync;
static constexpr const auto DESCRIPTOR = u"ohos.nfc.INfcControllerService";
const int32_t NUM_BYTES = 1;
const int32_t MAX_BYTE_VALUE = 256;
const int32_t SEED_SIZE = 1024;
FuzzedDataProvider* provider = nullptr;

shared_ptr<CloudMediaAlbumControllerService> cloudMediaAlbumControllerService = nullptr;
shared_ptr<CloudMediaDataControllerService> cloudMediaDataControllerService = nullptr;
shared_ptr<CloudMediaDownloadControllerService> cloudMediaDownloadControllerService = nullptr;
shared_ptr<CloudMediaPhotoControllerService> cloudMediaPhotoControllerService = nullptr;
shared_ptr<CloudMediaAlbumControllerProcessor> cloudMediaAlbumControllerProcessor = nullptr;
shared_ptr<CloudMediaDataControllerProcessor> cloudMediaDataControllerProcessor = nullptr;
shared_ptr<CloudMediaDownloadControllerProcessor> cloudMediaDownloadControllerProcessor = nullptr;
shared_ptr<CloudMediaPhotoControllerProcessor> cloudMediaPhotoControllerProcessor = nullptr;

static PhotosVo FuzzPhotosVo(string &cloudId)
{
    PhotosVo photosVo;
    photosVo.cloudId = cloudId;
    photosVo.size = provider->ConsumeIntegral<int64_t>();
    photosVo.path = provider->ConsumeBytesAsString(NUM_BYTES);
    photosVo.fileName = provider->ConsumeBytesAsString(NUM_BYTES);
    photosVo.type = provider->ConsumeBool() ? MediaType::MEDIA_TYPE_IMAGE : MediaType::MEDIA_TYPE_VIDEO;
    CloudFileDataVo cloudFileDataVo;
    cloudFileDataVo.fileName = provider->ConsumeBytesAsString(NUM_BYTES);
    cloudFileDataVo.filePath = provider->ConsumeBytesAsString(NUM_BYTES);
    cloudFileDataVo.size = provider->ConsumeIntegral<int64_t>();
    photosVo.attachment.insert({"attachment", cloudFileDataVo});
    return photosVo;
}

static CloudErrorDetail FuzzCloudErrorDetail()
{
    CloudErrorDetail errorDetail;
    errorDetail.domain = provider->ConsumeBytesAsString(NUM_BYTES);
    errorDetail.reason = provider->ConsumeBytesAsString(NUM_BYTES);
    errorDetail.errorCode = provider->ConsumeBytesAsString(NUM_BYTES);
    errorDetail.description = provider->ConsumeBytesAsString(NUM_BYTES);
    errorDetail.errorPos = provider->ConsumeBytesAsString(NUM_BYTES);
    errorDetail.errorParam = provider->ConsumeBytesAsString(NUM_BYTES);
    errorDetail.detailCode = provider->ConsumeIntegral<int32_t>();
    return errorDetail;
}

static inline Media::CloudSync::ErrorType FuzzErrorType()
{
    int32_t value = provider->ConsumeIntegral<int32_t>() % 3;
    if (value >= static_cast<int32_t>(Media::CloudSync::ErrorType::TYPE_UNKNOWN) &&
        value <= static_cast<int32_t>(Media::CloudSync::ErrorType::TYPE_MAX)) {
        return static_cast<Media::CloudSync::ErrorType>(value);
    }
    return Media::CloudSync::ErrorType::TYPE_NEED_UPLOAD;
}

static PhotosDto FuzzPhotosDto(string &cloudId)
{
    PhotosDto photosDto;
    photosDto.fileId = provider->ConsumeIntegral<int32_t>();
    photosDto.cloudId = cloudId;
    photosDto.displayName = provider->ConsumeBytesAsString(NUM_BYTES);
    photosDto.size = provider->ConsumeIntegral<int64_t>();
    photosDto.path = provider->ConsumeBytesAsString(NUM_BYTES);
    photosDto.fileName = provider->ConsumeBytesAsString(NUM_BYTES);
    photosDto.mediaType = provider->ConsumeBool() ? MediaType::MEDIA_TYPE_IMAGE : MediaType::MEDIA_TYPE_VIDEO;
    photosDto.originalCloudId = cloudId;
    photosDto.modifiedTime = provider->ConsumeIntegral<int64_t>();
    photosDto.rotation = provider->ConsumeIntegral<int32_t>();
    photosDto.fileType = provider->ConsumeIntegral<int32_t>();
    photosDto.createTime = provider->ConsumeIntegral<int64_t>();
    photosDto.modifiedTime = provider->ConsumeIntegral<int64_t>();
    photosDto.version = provider->ConsumeIntegral<int64_t>();
    photosDto.sourcePath = provider->ConsumeBytesAsString(NUM_BYTES);
    photosDto.metaDateModified = provider->ConsumeIntegral<int64_t>();
    photosDto.errorDetails = { FuzzCloudErrorDetail() };
    photosDto.serverErrorCode = provider->ConsumeIntegral<int32_t>();
    photosDto.errorType = FuzzErrorType();
    photosDto.isSuccess = provider->ConsumeBool();
    CloudFileDataDto cloudFileDataDto;
    cloudFileDataDto.fileName = provider->ConsumeBytesAsString(NUM_BYTES);
    cloudFileDataDto.path = provider->ConsumeBytesAsString(NUM_BYTES);
    cloudFileDataDto.size = provider->ConsumeIntegral<int64_t>();
    photosDto.attachment.insert({"attachment", cloudFileDataDto});
    return photosDto;
}

static MediaOperateResultDto FuzzMediaOperateResultDto(string &cloudId)
{
    MediaOperateResultDto operateResultDtoDto;
    operateResultDtoDto.cloudId = cloudId;
    operateResultDtoDto.errorCode = provider->ConsumeIntegral<int32_t>();
    operateResultDtoDto.errorMsg = provider->ConsumeBytesAsString(NUM_BYTES);
    return operateResultDtoDto;
}

static PhotoAlbumPo FuzzPhotoAlbumPo()
{
    PhotoAlbumPo photoAlbumPo;
    photoAlbumPo.albumId = provider->ConsumeIntegral<int32_t>();
    photoAlbumPo.albumType = provider->ConsumeIntegral<int32_t>();
    photoAlbumPo.albumName = provider->ConsumeBytesAsString(NUM_BYTES);
    photoAlbumPo.lpath = provider->ConsumeBytesAsString(NUM_BYTES);
    photoAlbumPo.cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    photoAlbumPo.albumSubtype = provider->ConsumeIntegral<int32_t>();
    photoAlbumPo.dateAdded = provider->ConsumeIntegral<int64_t>();
    photoAlbumPo.dateModified = provider->ConsumeIntegral<int64_t>();
    photoAlbumPo.bundleName = provider->ConsumeBytesAsString(NUM_BYTES);
    photoAlbumPo.localLanguage = provider->ConsumeBytesAsString(NUM_BYTES);
    photoAlbumPo.albumOrder = provider->ConsumeIntegral<int32_t>();
    photoAlbumPo.dirty = provider->ConsumeIntegral<int32_t>();
    photoAlbumPo.albumPluginCloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    photoAlbumPo.albumNameEn = provider->ConsumeBytesAsString(NUM_BYTES);
    photoAlbumPo.dualAlbumName = provider->ConsumeBytesAsString(NUM_BYTES);
    photoAlbumPo.priority = provider->ConsumeIntegral<int32_t>();
    photoAlbumPo.isInWhiteList = provider->ConsumeBool();
    return photoAlbumPo;
}

static GetAgingFileReqBody FuzzGetAgingFileReqBody()
{
    GetAgingFileReqBody agingFileReqBody;
    agingFileReqBody.time = provider->ConsumeIntegral<int64_t>();
    agingFileReqBody.mediaType = provider->ConsumeIntegral<int32_t>();
    agingFileReqBody.sizeLimit = provider->ConsumeIntegral<int32_t>();
    agingFileReqBody.offset = provider->ConsumeIntegral<int32_t>();
    return agingFileReqBody;
}

static GetDownloadThmReqBody FuzzGetDownloadThmReqBody()
{
    GetDownloadThmReqBody downloadThmReqBody;
    downloadThmReqBody.size = provider->ConsumeIntegral<int32_t>();
    downloadThmReqBody.type = provider->ConsumeIntegral<int32_t>();
    downloadThmReqBody.offset = provider->ConsumeIntegral<int32_t>();
    downloadThmReqBody.isDownloadDisplayFirst = provider->ConsumeBool();
    return downloadThmReqBody;
}

static void CloudMediaAlbumControllerServiceFuzzer()
{
    MessageParcel dataParcel;
    MessageParcel reply;
    if (cloudMediaAlbumControllerService == nullptr) {
        MEDIA_ERR_LOG("cloudMediaAlbumControllerService is nullptr");
        return;
    }
    cloudMediaAlbumControllerService->OnFetchRecords(dataParcel, reply);
    bool errConn = !dataParcel.WriteInterfaceToken(DESCRIPTOR);
    CHECK_AND_RETURN_LOG(!errConn, "WriteInterfaceToken failed");
    cloudMediaAlbumControllerService->OnFetchRecords(dataParcel, reply);
    cloudMediaAlbumControllerService->OnDentryFileInsert(dataParcel, reply);
    cloudMediaAlbumControllerService->GetCreatedRecords(dataParcel, reply);
    cloudMediaAlbumControllerService->GetMetaModifiedRecords(dataParcel, reply);
    cloudMediaAlbumControllerService->GetDeletedRecords(dataParcel, reply);
    cloudMediaAlbumControllerService->OnCreateRecords(dataParcel, reply);
    cloudMediaAlbumControllerService->OnMdirtyRecords(dataParcel, reply);
    cloudMediaAlbumControllerService->OnFdirtyRecords(dataParcel, reply);
    cloudMediaAlbumControllerService->OnDeleteRecords(dataParcel, reply);
    cloudMediaAlbumControllerService->OnCopyRecords(dataParcel, reply);
}

static void CloudMediaDataControllerServiceFuzzer()
{
    MessageParcel dataParcel;
    MessageParcel reply;
    if (cloudMediaDataControllerService == nullptr) {
        MEDIA_ERR_LOG("cloudMediaDataControllerService is nullptr");
        return;
    }
    cloudMediaDataControllerService->UpdateDirty(dataParcel, reply);
    cloudMediaDataControllerService->UpdatePosition(dataParcel, reply);
    cloudMediaDataControllerService->UpdateThmStatus(dataParcel, reply);
    cloudMediaDataControllerService->GetAgingFile(dataParcel, reply);
    cloudMediaDataControllerService->GetActiveAgingFile(dataParcel, reply);
    cloudMediaDataControllerService->GetVideoToCache(dataParcel, reply);
    cloudMediaDataControllerService->GetFilePosStat(dataParcel, reply);
    cloudMediaDataControllerService->GetCloudThmStat(dataParcel, reply);
    cloudMediaDataControllerService->GetDirtyTypeStat(dataParcel, reply);
    cloudMediaDataControllerService->UpdateLocalFileDirty(dataParcel, reply);
    cloudMediaDataControllerService->UpdateSyncStatus(dataParcel, reply);
}

static void CloudMediaDownloadControllerServiceFuzzer()
{
    MessageParcel dataParcel;
    MessageParcel reply;
    if (cloudMediaDownloadControllerService == nullptr) {
        MEDIA_ERR_LOG("cloudMediaDownloadControllerService is nullptr");
        return;
    }
    cloudMediaDownloadControllerService->GetDownloadThms(dataParcel, reply);
    cloudMediaDownloadControllerService->GetDownloadThmNum(dataParcel, reply);
    cloudMediaDownloadControllerService->GetDownloadThmsByUri(dataParcel, reply);
    cloudMediaDownloadControllerService->OnDownloadThms(dataParcel, reply);
    cloudMediaDownloadControllerService->GetDownloadAsset(dataParcel, reply);
    cloudMediaDownloadControllerService->OnDownloadAsset(dataParcel, reply);
}

static void CloudMediaPhotoControllerServiceFuzzer()
{
    MessageParcel dataParcel;
    MessageParcel reply;
    if (cloudMediaPhotoControllerService == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotoControllerService is nullptr");
        return;
    }
    cloudMediaPhotoControllerService->OnFetchRecords(dataParcel, reply);
    cloudMediaPhotoControllerService->OnDentryFileInsert(dataParcel, reply);
    cloudMediaPhotoControllerService->GetCreatedRecords(dataParcel, reply);
    cloudMediaPhotoControllerService->GetMetaModifiedRecords(dataParcel, reply);
    cloudMediaPhotoControllerService->GetFileModifiedRecords(dataParcel, reply);
    cloudMediaPhotoControllerService->GetDeletedRecords(dataParcel, reply);
    cloudMediaPhotoControllerService->GetCopyRecords(dataParcel, reply);
    cloudMediaPhotoControllerService->GetCheckRecords(dataParcel, reply);
    cloudMediaPhotoControllerService->OnCreateRecords(dataParcel, reply);
    cloudMediaPhotoControllerService->OnMdirtyRecords(dataParcel, reply);
    cloudMediaPhotoControllerService->OnFdirtyRecords(dataParcel, reply);
    cloudMediaPhotoControllerService->OnDeleteRecords(dataParcel, reply);
    cloudMediaPhotoControllerService->OnCopyRecords(dataParcel, reply);
    cloudMediaPhotoControllerService->GetRetryRecords(dataParcel, reply);
}

static void CloudMediaAlbumControllerProcessorFuzzer()
{
    PhotoAlbumPo record = FuzzPhotoAlbumPo();
    if (cloudMediaAlbumControllerProcessor == nullptr) {
        MEDIA_ERR_LOG("cloudMediaAlbumControllerProcessor is nullptr");
        return;
    }
    cloudMediaAlbumControllerProcessor->ConvertRecordPoToVo(record);
}

static void CloudMediaDataControllerProcessorFuzzer()
{
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    PhotosVo photosVo = FuzzPhotosVo(cloudId);
    PhotosDto photosDto = FuzzPhotosDto(cloudId);
    vector<MediaOperateResultDto> mediaOperateResultDto = { FuzzMediaOperateResultDto(cloudId) };
    GetAgingFileReqBody reqBody = FuzzGetAgingFileReqBody();
    AgingFileQueryDto queryDto;
    if (cloudMediaDataControllerProcessor == nullptr) {
        MEDIA_ERR_LOG("cloudMediaDataControllerProcessor is nullptr");
        return;
    }
    cloudMediaDataControllerProcessor->ConvertPhotosVoToPhotosDto(photosVo);
    cloudMediaDataControllerProcessor->ConvertPhotosDtoToPhotosVo(photosDto);
    cloudMediaDataControllerProcessor->GetMediaOperateResult(mediaOperateResultDto);
    cloudMediaDataControllerProcessor->GetAgingFileQueryDto(reqBody, queryDto);
}

static void CloudMediaDownloadControllerProcessorFuzzer()
{
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    PhotosDto photosDto = FuzzPhotosDto(cloudId);
    GetDownloadThmReqBody reqBody = FuzzGetDownloadThmReqBody();
    vector<MediaOperateResultDto> mediaOperateResultDto = { FuzzMediaOperateResultDto(cloudId) };
    if (cloudMediaDownloadControllerProcessor == nullptr) {
        MEDIA_ERR_LOG("cloudMediaDownloadControllerProcessor is nullptr");
        return;
    }
    cloudMediaDownloadControllerProcessor->ConvertPhotosDtoToPhotosVo(photosDto);
    cloudMediaDownloadControllerProcessor->GetDownloadThumbnailQueryDto(reqBody);
    cloudMediaDownloadControllerProcessor->GetMediaOperateResult(mediaOperateResultDto);
}

static void CloudMediaPhotoControllerProcessorFuzzer()
{
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    vector<PhotosDto> photosDto = { FuzzPhotosDto(cloudId) };
    PhotosDto dto = FuzzPhotosDto(cloudId);
    PhotosPo record;
    OnFetchPhotosVo photosVo;
    OnCreateRecord createRecordrecordVo;
    OnFileDirtyRecord fileDirtyRecordVo;
    OnModifyRecord modifyRecordVo;

    if (cloudMediaPhotoControllerProcessor == nullptr) {
        MEDIA_ERR_LOG("cloudMediaPhotoControllerProcessor is nullptr");
        return;
    }
    cloudMediaPhotoControllerProcessor->SetFdirtyDataVoFromDto(photosDto);
    cloudMediaPhotoControllerProcessor->SetNewDataVoFromDto(photosDto);
    cloudMediaPhotoControllerProcessor->GetCheckRecordsRespBody(photosDto);
    cloudMediaPhotoControllerProcessor->ConvertRecordPoToVo(record);
    cloudMediaPhotoControllerProcessor->ConvertToCloudMediaPullData(photosVo);
    cloudMediaPhotoControllerProcessor->ConvertToPhotoDto(createRecordrecordVo);
    cloudMediaPhotoControllerProcessor->ConvertToPhotosDto(fileDirtyRecordVo, dto);
    cloudMediaPhotoControllerProcessor->ConvertToPhotosDto(modifyRecordVo, dto);
}

static void Init()
{
    cloudMediaAlbumControllerService = make_shared<CloudMediaAlbumControllerService>();
    cloudMediaDataControllerService = make_shared<CloudMediaDataControllerService>();
    cloudMediaDownloadControllerService = make_shared<CloudMediaDownloadControllerService>();
    cloudMediaPhotoControllerService = make_shared<CloudMediaPhotoControllerService>();
    cloudMediaAlbumControllerProcessor = make_shared<CloudMediaAlbumControllerProcessor>();
    cloudMediaDataControllerProcessor = make_shared<CloudMediaDataControllerProcessor>();
    cloudMediaDownloadControllerProcessor = make_shared<CloudMediaDownloadControllerProcessor>();
    cloudMediaPhotoControllerProcessor = make_shared<CloudMediaPhotoControllerProcessor>();
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
    OHOS::CloudMediaAlbumControllerServiceFuzzer();
    OHOS::CloudMediaDataControllerServiceFuzzer();
    OHOS::CloudMediaDownloadControllerServiceFuzzer();
    OHOS::CloudMediaPhotoControllerServiceFuzzer();
    OHOS::CloudMediaAlbumControllerProcessorFuzzer();
    OHOS::CloudMediaDataControllerProcessorFuzzer();
    OHOS::CloudMediaDownloadControllerProcessorFuzzer();
    OHOS::CloudMediaPhotoControllerProcessorFuzzer();
    return 0;
}
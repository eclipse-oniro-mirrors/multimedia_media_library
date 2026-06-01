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

#include "medialibrarycloudmediaservice3_fuzzer.h"
#include "medialibrary_rdbstore_utils_fuzzer.h"

#include <cstdint>
#include <string>
#include <fstream>
#include <fuzzer/FuzzedDataProvider.h>
#include "media_log.h"
#include "media_upgrade.h"

#include "cloud_media_album_service.h"
#include "cloud_media_data_service_processor.h"
#include "cloud_media_photo_service_processor.h"

namespace OHOS {
using namespace std;
using namespace OHOS::Media;
using namespace OHOS::Media::CloudSync;
using ChangeType = OHOS::AAFwk::ChangeInfo::ChangeType;
const int32_t NUM_BYTES = 1;
const int32_t MAX_BYTE_VALUE = 256;
const int32_t SEED_SIZE = 1024;
FuzzedDataProvider* provider;
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;
shared_ptr<CloudMediaAlbumService> cloudMediaAlbumService = make_shared<CloudMediaAlbumService>();

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

static PhotosPo FuzzPhotosPo()
{
    PhotosPo photosPos;
    photosPos.data = provider->ConsumeBytesAsString(NUM_BYTES);
    return photosPos;
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
    cloudMediaAlbumService->OnDeleteRecords(albumDtoList, failSize);

    albumDtoList.clear();
    PhotoAlbumDto record = FuzzPhotoAlbumDto();
    albumDtoList.emplace_back(record);
    record.lPath = "/Pictures/Screenshots";
    cloudMediaAlbumService->OnFetchRecords(albumDtoList, resp);

    cloudMediaAlbumService->ConvertToSingleScreenshots(record, albumDtoList);
    vector<string> failedAlbumIds = { "default" };
    cloudMediaAlbumService->OnDeleteAlbums(failedAlbumIds);
    ChangeType type;
    bool bContinue;
    cloudMediaAlbumService->HandleFetchOldRecord(record, bContinue, type, resp);
    cloudMediaAlbumService->OnStartSync();
    cloudMediaAlbumService->OnCompleteSync();
    MediaOperateResult optRet = {"", 0, ""};
    cloudMediaAlbumService->OnCompletePull(optRet);
    cloudMediaAlbumService->OnCompletePush();
    cloudMediaAlbumService->OnCompleteCheck();
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
        MEDIA_ERR_LOG("cloudMediaDataServiceProcessor or cloudMediaPhotoServiceProcessor is nullptr");
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
    auto rdbStore = Media::MediaLibraryRdbStoreUtilsTest::InitMediaLibraryRdbStore(abilityContextImpl);
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
    if (data == nullptr) {
        return 0;
    }
    FuzzedDataProvider fdp(data, size);
    OHOS::provider = &fdp;

    OHOS::CloudMediaAlbumServiceFuzzer();
    OHOS::CloudMediaServiceProcessorFuzzer();
    return 0;
}
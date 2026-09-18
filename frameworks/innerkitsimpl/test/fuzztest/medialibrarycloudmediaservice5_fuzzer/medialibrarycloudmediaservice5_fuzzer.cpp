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

#include "medialibrarycloudmediaservice5_fuzzer.h"
#include "medialibrary_rdbstore_utils_fuzzer.h"

#include <cstdint>
#include <string>
#include <fstream>
#include <fuzzer/FuzzedDataProvider.h>
#include "cloud_media_photos_service.h"
#include "media_log.h"
#include "cloud_media_operation_code.h"
#include "media_upgrade.h"

namespace OHOS::Media {
using namespace std;
using namespace OHOS::Media::CloudSync;
static const int32_t NUM_BYTES = 1;
static const int32_t INDEX = 1;
static const int32_t VECTOR_SIZE = 5;
static const int32_t MXA_CLEAN_TYPE = 1;
static const int32_t MAX_DIRTY_TYPE = 8;
static const int32_t MIN_PHOTO_POSITION_TYPE = -1;
static const int32_t MAX_PHOTO_POSITION_TYPE = 3;
static const int32_t MIN_SYNC_STATUS_TYPE = -1;
static const int32_t MAX_SYNC_STATUS_TYPE = 2;
static const int32_t MAX_THUMB_SATUS = 3;
static const int32_t MAX_MEDIA_TYPE = 14;
static const int32_t MAX_BYTE_VALUE = 256;
static const int32_t SEED_SIZE = 1024;
static const string PHOTOS_TABLE = "Photos";
FuzzedDataProvider *provider;
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;

static inline DirtyType FuzzDirtyType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_DIRTY_TYPE);
    return static_cast<DirtyType>(value);
}

static inline CloudSync::ThumbState FuzzThumbState()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_THUMB_SATUS);
    return static_cast<CloudSync::ThumbState>(value);
}

static inline MediaType FuzzMediaType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_MEDIA_TYPE);
    return static_cast<MediaType>(value);
}

static inline PhotoPositionType FuzzPhotoPositionType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(MIN_PHOTO_POSITION_TYPE, MAX_PHOTO_POSITION_TYPE);
    return static_cast<PhotoPositionType>(value);
}

static inline SyncStatusType FuzzSyncStatusType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(MIN_SYNC_STATUS_TYPE, MAX_SYNC_STATUS_TYPE);
    return static_cast<SyncStatusType>(value);
}

static inline Media::CleanType FuzzCleanType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MXA_CLEAN_TYPE);
    return static_cast<CleanType>(value);
}

static inline int32_t FuzzFileType()
{
    int32_t data = provider->ConsumeIntegralInRange<int32_t>(0, FILE_TYPE_LIST.size() - INDEX);
    return FILE_TYPE_LIST[data];
}

static inline CloudSync::ServerErrorCode FuzzServerErrorCode()
{
    int32_t data = provider->ConsumeIntegralInRange<int32_t>(0, SERVER_ERROR_CODE_LIST.size() - INDEX);
    return SERVER_ERROR_CODE_LIST[data];
}

static inline CloudSync::ErrorType FuzzErrorType()
{
    int32_t data = provider->ConsumeIntegralInRange<int32_t>(0, ERROR_TYPE_LIST.size() - INDEX);
    return ERROR_TYPE_LIST[data];
}

static inline CloudSync::ErrorDetailCode FuzzErrorDetailCode()
{
    int32_t data = provider->ConsumeIntegralInRange<int32_t>(0, ERROR_DETAIL_CODE_LIST.size() - INDEX);
    return ERROR_DETAIL_CODE_LIST[data];
}

static int32_t InsertQueryAsset()
{
    if (g_rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::MEDIA_TYPE, static_cast<int32_t>(FuzzMediaType()));
    values.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(FuzzPhotoPositionType()));
    values.PutInt(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(FuzzSyncStatusType()));
    values.PutInt(PhotoColumn::PHOTO_THUMB_STATUS, static_cast<int32_t>(FuzzThumbState()));
    values.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(FuzzCleanType()));
    values.PutLong(PhotoColumn::MEDIA_DATE_TAKEN, provider->ConsumeIntegral<int64_t>());
    int64_t fileId = 0;
    int32_t ret = g_rdbStore->Insert(fileId, PHOTOS_TABLE, values);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_ERR, "g_rdbStore failed to insert ret=%{public}d", ret);
    return static_cast<int32_t>(fileId);
}

static void OnStartAndOnCompleteFuzzer()
{
    CloudMediaPhotosService service;
    MediaOperateResult optRet = {
        provider->ConsumeBytesAsString(NUM_BYTES), 0,
        provider->ConsumeBytesAsString(NUM_BYTES)
    };
    service.OnStartSync();
    service.OnCompleteSync();
    service.OnCompletePull(optRet);
    service.OnCompletePush();
    service.OnCompleteCheck();
}

static void OnRecordFailedFuzzer()
{
    CloudMediaPhotosService service;
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    PhotosDto photo;
    photo.serverErrorCode = FuzzServerErrorCode();
    service.OnRecordFailed(photo, photoRefresh);

    photo.errorType = FuzzErrorType();
    service.OnRecordFailed(photo, photoRefresh);

    CloudErrorDetail cloudErrorDetail;
    cloudErrorDetail.detailCode = FuzzErrorDetailCode();
    photo.errorDetails = { cloudErrorDetail };
    service.OnRecordFailed(photo, photoRefresh);
}

static void GetMergeDataMapFuzzer()
{
    CloudMediaPhotosService service;
    CloudMediaPullDataDto data;
    data.hasAttributes = provider->ConsumeBool();
    data.basicFileName = provider->ConsumeBytesAsString(NUM_BYTES);
    data.propertiesRotate = provider->ConsumeIntegral<int32_t>();
    data.basicFileType = FuzzFileType();
    std::vector<CloudMediaPullDataDto> pullDatas = {data};
    std::map<string, KeyData> mergeDataMap;
    service.GetMergeDataMap(pullDatas, mergeDataMap);
}

static void GetRecordsFuzzer()
{
    CloudMediaPhotosService service;
    int32_t recordsSize = provider->ConsumeIntegral<uint8_t>();
    vector<PhotosPo> photosPo;
    service.GetFileModifiedRecords(recordsSize, photosPo);
    service.GetCopyRecords(recordsSize, photosPo);
}

static void PullDeleteFuzzer()
{
    CloudMediaPhotosService service;
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    std::string cloudId = "cloud_id";
    std::set<string> refreshAlbums;
    vector<PhotosDto> fdirtyData;
    vector<int32_t> stats(VECTOR_SIZE, 0);

    CloudMediaPullDataDto data;
    data.localPosition = provider->ConsumeIntegral<int32_t>();
    data.localDirty = static_cast<int32_t>(FuzzDirtyType());
    data.localPath = "/data/fuzztest.txt";
    data.cloudId = cloudId;
    service.PullDelete(data, refreshAlbums, photoRefresh);
}

static void OnRecordFuzzer()
{
    CloudMediaPhotosService service;
    vector<PhotosDto> photos;
    PhotosDto photo;
    photos.emplace_back(photo);
    int32_t failedSize = -1;
    InsertQueryAsset();
    service.OnCreateRecords(photos, failedSize);
    service.OnMdirtyRecords(photos, failedSize);
    service.OnFdirtyRecords(photos, failedSize);
    service.OnDeleteRecords(photos, failedSize);
    service.OnCopyRecords(photos, failedSize);
}

static void CloudMediadPhotoService5Fuzzer()
{
    InsertQueryAsset();
    OnStartAndOnCompleteFuzzer();
    OnRecordFailedFuzzer();
    GetMergeDataMapFuzzer();
    GetRecordsFuzzer();
    PullDeleteFuzzer();
    OnRecordFuzzer();
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
    std::unique_ptr<char[]> seedData = std::make_unique<char[]>(SEED_SIZE);
    for (int i = 0; i < SEED_SIZE; i++) {
        seedData[i] = static_cast<char>(i % MAX_BYTE_VALUE);
    }

    const char* filename = "corpus/seed.txt";
    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file) {
        MEDIA_ERR_LOG("Cannot open file filename:%{public}s", filename);
        return Media::E_ERR;
    }
    file.write(seedData.get(), SEED_SIZE);
    file.close();
    MEDIA_INFO_LOG("seedData has been successfully written to file filename:%{public}s", filename);
    return Media::E_OK;
}
} // namespace OHOS::Media

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::Media::AddSeed();
    OHOS::Media::Init();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    CHECK_AND_RETURN_RET_LOG(data != nullptr, OHOS::Media::E_ERR, "data is nullptr");
    FuzzedDataProvider fdp(data, size);
    OHOS::Media::provider = &fdp;
    CHECK_AND_RETURN_RET_LOG(OHOS::Media::provider != nullptr, OHOS::Media::E_ERR, "provider is nullptr");

    OHOS::Media::CloudMediadPhotoService5Fuzzer();
    return 0;
}
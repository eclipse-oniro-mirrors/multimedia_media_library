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

#include "medialibrarycloudmediaservice2_fuzzer.h"

#include <cstdint>
#include <string>
#include <fstream>
#include <fuzzer/FuzzedDataProvider.h>

#include "ability_context_impl.h"
#include "cloud_media_pull_data_dto.h"
#include "media_log.h"
#include "medialibrary_command.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_kvstore_manager.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
using namespace std;
constexpr int32_t INDEX = 1;
constexpr int32_t MAX_ERROR_TYPE = 3;
constexpr int32_t MAX_COVER_URI_SOURCE = 3;
constexpr int32_t NUM_BYTES = 8;
constexpr int32_t MAX_BYTE_VALUE = 256;
constexpr int32_t SEED_SIZE = 1024;
const string PHOTOS_TABLE = "Photos";
const string TABLE = "PhotoAlbum";
const string CLOUD_PATH = "/storage/cloud/test";
const string COVER_CLOUD_ID = "test,cloudId";
const string ALBUM_COVER_URI = "file://media/Photo/1/test.jpg";
FuzzedDataProvider* provider;
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;
shared_ptr<CloudMediaAlbumService> cloudMediaAlbumService = nullptr;
shared_ptr<CloudMediaPhotosService> cloudMediaPhotosService = nullptr;

int32_t g_photoAlbumSubTypeArray[] = {
    1, 2049
};

static inline int32_t FuzzPhotoAlbumSubType()
{
    return provider->PickValueInArray(g_photoAlbumSubTypeArray);
}

static inline KeyData FuzzKeyData()
{
    KeyData KeyData;
    KeyData.modifyTime = provider->ConsumeIntegral<int64_t>();
    KeyData.createTime = provider->ConsumeIntegral<int64_t>();
    return KeyData;
}

static inline CloudMediaPhotoOperationCode FuzzApiCode()
{
    int32_t data = provider->ConsumeIntegralInRange<int32_t>(0, PHOTO_OPERATION_CODE_LIST.size() - INDEX);
    return static_cast<CloudMediaPhotoOperationCode>(PHOTO_OPERATION_CODE_LIST[data]);
}

static inline CloudSyncServiceErrCode FuzzErrorCode()
{
    int32_t data = provider->ConsumeIntegralInRange<int32_t>(0, ERR_CODE_LIST.size() - INDEX);
    return static_cast<CloudSyncServiceErrCode>(ERR_CODE_LIST[data]);
}

static inline ServerErrorCode FuzzServerErrorCode()
{
    int32_t data = provider->ConsumeIntegralInRange<int32_t>(0, SERVER_ERROR_CODE_LIST.size() - INDEX);
    return static_cast<ServerErrorCode>(SERVER_ERROR_CODE_LIST[data]);
}

static inline ErrorDetailCode FuzzErrorDetailCode()
{
    int32_t data = provider->ConsumeIntegralInRange<int32_t>(0, ERROR_DETAIL_CODE_LIST.size() - INDEX);
    return static_cast<ErrorDetailCode>(ERROR_DETAIL_CODE_LIST[data]);
}

static inline int32_t FuzzFileType()
{
    int32_t data = provider->ConsumeIntegralInRange<int32_t>(0, FILE_TYPE_LIST.size() - INDEX);
    return FILE_TYPE_LIST[data];
}

static inline ErrorType FuzzErrorType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_ERROR_TYPE);
    return static_cast<ErrorType>(value);
}

static inline CoverUriSource FuzzCoverUriSource()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_COVER_URI_SOURCE);
    return static_cast<CoverUriSource>(value);
}

static int32_t InsertAlbumAsset()
{
    if (g_rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, FuzzPhotoAlbumSubType());
    values.PutInt(PhotoAlbumColumns::COVER_URI_SOURCE, FuzzCoverUriSource());
    values.PutString(PhotoAlbumColumns::COVER_CLOUD_ID, COVER_CLOUD_ID);
    values.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, ALBUM_COVER_URI);
    int64_t fileId = 0;
    int32_t ret = g_rdbStore->Insert(fileId, TABLE, values);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_ERR, "g_rdbStore failed to insert ret=%{public}d", ret);
    return static_cast<int32_t>(fileId);
}

static int32_t InsertPhotoAsset()
{
    NativeRdb::ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_FILE_PATH, provider->ConsumeBytesAsString(NUM_BYTES));
    values.PutString(PhotoColumn::PHOTO_CLOUD_ID, "cloudId");
    values.PutString(PhotoColumn::MEDIA_NAME, provider->ConsumeBytesAsString(NUM_BYTES));
    int64_t fileId = 0;
    int32_t ret = g_rdbStore->Insert(fileId, PHOTOS_TABLE, values);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_ERR, "g_rdbStore failed to insert ret=%{public}d", ret);
    return static_cast<int32_t>(fileId);
}

static void ExtractEditDataCameraTest()
{
    CloudMediaPullDataDto pullData;
    pullData.attributesEditDataCamera = provider->ConsumeBytesAsString(NUM_BYTES);
    pullData.localPath = ROOT_MEDIA_DIR + provider->ConsumeBytesAsString(NUM_BYTES);
    CHECK_AND_RETURN_LOG(cloudMediaPhotosService != nullptr, "cloudMediaPhotosService is nullptr");
    cloudMediaPhotosService->ExtractEditDataCamera(pullData);
}

static void ClearLocalDataTest()
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    CloudMediaPullDataDto pullData;
    PhotosPo po;
    pullData.localPhotosPoOp = po;
    std::vector<PhotosDto> fdirtyData;
    CHECK_AND_RETURN_LOG(cloudMediaPhotosService != nullptr, "cloudMediaPhotosService is nullptr");
    cloudMediaPhotosService->ClearLocalData(pullData, fdirtyData);
#endif
}

static void GetCloudKeyDataTest()
{
    CloudMediaPullDataDto pullData;
    pullData.hasAttributes = true;
    pullData.basicFileName = provider->ConsumeBytesAsString(NUM_BYTES);
    pullData.basicSize = provider->ConsumeIntegral<int64_t>();
    pullData.propertiesRotate = provider->ConsumeIntegral<int32_t>();
    pullData.basicFileType = FuzzFileType();
    KeyData keyData;
    CHECK_AND_RETURN_LOG(cloudMediaPhotosService != nullptr, "cloudMediaPhotosService is nullptr");
    cloudMediaPhotosService->GetCloudKeyData(pullData, keyData);
}

static void DoDataMergeTest()
{
    CloudMediaPullDataDto pullData;
    KeyData localKeyData = FuzzKeyData();
    KeyData cloudKeyData = FuzzKeyData();
    std::set<std::string> refreshAlbums;
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    CHECK_AND_RETURN_LOG(cloudMediaPhotosService != nullptr, "cloudMediaPhotosService is nullptr");
    cloudMediaPhotosService->DoDataMerge(pullData, localKeyData, cloudKeyData, refreshAlbums, photoRefresh);
}

static void PullRecordsDataMergeTest()
{
    CloudMediaPullDataDto pullData;
    string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    pullData.cloudId = cloudId;
    std::vector<CloudMediaPullDataDto> allPullDatas = {pullData};
    string lPath = provider->ConsumeBytesAsString(NUM_BYTES);
    KeyData localKeyData;
    localKeyData.displayName = provider->ConsumeBytesAsString(NUM_BYTES);
    localKeyData.lPath = lPath;
    KeyData cloudKeyData;
    cloudKeyData.displayName = provider->ConsumeBytesAsString(NUM_BYTES);
    cloudKeyData.lPath = lPath;
    std::map<std::string, KeyData> mergeDataMap = {{cloudId, cloudKeyData}};
    CloudSync::CloudMediaPhotosService::DataMergeResult mergeResult;
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();

    CHECK_AND_RETURN_LOG(cloudMediaPhotosService != nullptr, "cloudMediaPhotosService is nullptr");
    cloudMediaPhotosService->PullRecordsDataMerge(allPullDatas, localKeyData, mergeDataMap, mergeResult, photoRefresh);
}

static void SetPullDataFromPhotosPoTest()
{
    CloudMediaPullDataDto pullData;
    PhotosPo photo;
    CHECK_AND_RETURN_LOG(cloudMediaPhotosService != nullptr, "cloudMediaPhotosService is nullptr");
    cloudMediaPhotosService->SetPullDataFromPhotosPo(pullData, photo);
}

static void HandleCloudDeleteRecordTest()
{
    CloudMediaPullDataDto pullData;
    pullData.localPath = provider->ConsumeBytesAsString(NUM_BYTES);
    pullData.basicIsDelete = provider->ConsumeBool();
    pullData.localFileId = provider->ConsumeIntegral<int32_t>();
    std::map<std::string, CloudMediaPullDataDto> cloudIdRelativeMap = {{"cloudId", pullData}};

    CHECK_AND_RETURN_LOG(cloudMediaPhotosService != nullptr, "cloudMediaPhotosService is nullptr");
    cloudMediaPhotosService->HandleCloudDeleteRecord(cloudIdRelativeMap);
}

static void GetCheckRecordsTest()
{
    std::vector<std::string> cloudIds = {provider->ConsumeBytesAsString(NUM_BYTES)};
    CHECK_AND_RETURN_LOG(cloudMediaPhotosService != nullptr, "cloudMediaPhotosService is nullptr");
    cloudMediaPhotosService->GetCheckRecords(cloudIds);
}

static void GetDeletedRecordsTest()
{
    int32_t size = provider->ConsumeIntegral<int32_t>();
    CHECK_AND_RETURN_LOG(cloudMediaPhotosService != nullptr, "cloudMediaPhotosService is nullptr");
    cloudMediaPhotosService->GetDeletedRecords(size);
}

static void GetCloudPathTest()
{
    std::string filePath = CLOUD_PATH;
    CHECK_AND_RETURN_LOG(cloudMediaPhotosService != nullptr, "cloudMediaPhotosService is nullptr");
    cloudMediaPhotosService->GetCloudPath(filePath);
}

static void ReportFailureTest()
{
    ReportFailureDto failureDto;
    failureDto.apiCode = static_cast<int32_t>(FuzzApiCode());
    failureDto.fileId = provider->ConsumeIntegral<int32_t>();
    failureDto.cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    failureDto.errorCode = static_cast<int32_t>(FuzzErrorCode());

    CHECK_AND_RETURN_LOG(cloudMediaPhotosService != nullptr, "cloudMediaPhotosService is nullptr");
    cloudMediaPhotosService->ReportFailure(failureDto);
}

static void OnRecordFailedTest()
{
    PhotoAlbumDto album;
    album.serverErrorCode = FuzzServerErrorCode();
    CloudErrorDetail errorDetail;
    errorDetail.detailCode = FuzzErrorDetailCode();
    if (provider->ConsumeBool()) {
        album.errorDetails = {errorDetail};
    }
    album.errorType = FuzzErrorType();
    CHECK_AND_RETURN_LOG(cloudMediaAlbumService != nullptr, "cloudMediaAlbumService is nullptr");
    cloudMediaAlbumService->OnRecordFailed(album);
}

static void OnCompleteSyncTest()
{
    InsertAlbumAsset();
    InsertPhotoAsset();
    CHECK_AND_RETURN_LOG(cloudMediaAlbumService != nullptr, "cloudMediaAlbumService is nullptr");
    cloudMediaAlbumService->OnCompleteSync();
}

static void CloudMediaAlbumService2Test()
{
    OnRecordFailedTest();
    OnCompleteSyncTest();
}

static void CloudMediadPhotoService2Test()
{
    ExtractEditDataCameraTest();
    ClearLocalDataTest();
    GetCloudKeyDataTest();
    DoDataMergeTest();
    PullRecordsDataMergeTest();
    SetPullDataFromPhotosPoTest();
    HandleCloudDeleteRecordTest();
    GetCheckRecordsTest();
    GetDeletedRecordsTest();
    GetCloudPathTest();
    ReportFailureTest();
}

void SetTables()
{
    vector<string> createTableSqlList = {
        PhotoColumn::CREATE_PHOTO_TABLE,
        PhotoAlbumColumns::CREATE_TABLE
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
    auto g_cloudMediaAlbumService = make_shared<CloudMediaAlbumService>();
    CHECK_AND_RETURN_LOG(g_cloudMediaAlbumService != nullptr, "g_cloudMediaAlbumService is nullptr");
    cloudMediaAlbumService = g_cloudMediaAlbumService;
    auto g_cloudMediaPhotosService = make_shared<CloudMediaPhotosService>();
    CHECK_AND_RETURN_LOG(g_cloudMediaPhotosService != nullptr, "g_cloudMediaPhotosService is nullptr");
    cloudMediaPhotosService = g_cloudMediaPhotosService;
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
    Media::MediaLibraryKvStoreManager::GetInstance().CloseAllKvStore();
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
    /* Run your code on data */
    if (data == nullptr) {
        return 0;
    }
    FuzzedDataProvider fdp(data, size);
    OHOS::Media::provider = &fdp;
    OHOS::Media::CloudMediaAlbumService2Test();
    OHOS::Media::CloudMediadPhotoService2Test();
    OHOS::Media::ClearKvStore();
    return 0;
}
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

#include "medialibrary_multistages_deferred_capture2_fuzzer.h"

#include <cstdint>
#include <string>
#include <fstream>
#include <thread>
#include <fuzzer/FuzzedDataProvider.h>
#include "ability_context_impl.h"
#include "media_log.h"
#include "rdb_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_kvstore_manager.h"
#include "picture_adapter.h"
#include "dfx_const.h"

#define private public
#define protected public
#include "multistages_capture_deferred_photo_proc_session_callback.h"
#undef private
#undef protected

namespace OHOS {
namespace Media {
using namespace std;
static const int32_t IS_TEMP = 0;
static const int32_t NUM_BYTES = 1;
static const int32_t MAX_DATA = 1;
static const int32_t MAX_PHOTO_QUALITY = 1;
static const int32_t MAX_MODIFY_TYPE = 2;
static const int32_t MAX_DPS_ERROR_CODE = 10;
static const int32_t MAX_MEDIA_TYPE = 14;
static const int32_t MAX_BYTE_VALUE = 256;
static const int32_t SEED_SIZE = 1024;
static const string PHOTOS_TABLE = "Photos";
FuzzedDataProvider *provider = nullptr;
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;

static inline CameraStandard::DpsErrorCode FuzzDpsErrorCode()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_DPS_ERROR_CODE);
    return static_cast<CameraStandard::DpsErrorCode>(value);
}

static inline MultiStagesPhotoQuality FuzzMultiStagesPhotoQuality()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_PHOTO_QUALITY);
    return static_cast<MultiStagesPhotoQuality>(value);
}

static inline FirstStageModifyType FuzzFirstStageModifyType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_MODIFY_TYPE);
    return static_cast<FirstStageModifyType>(value);
}

static inline string FuzzMimeTypeAndDisplayNameExtension()
{
    uint8_t data = provider->ConsumeIntegralInRange<uint8_t>(0, MAX_DATA);
    string displayName = provider->ConsumeBytesAsString(NUM_BYTES) + Media::DISPLAY_NAME_EXTENSION_FUZZER_LISTS[data];
    return displayName;
}

static inline int32_t FuzzMediaType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_MEDIA_TYPE);
    return static_cast<Media::MediaType>(value);
}

static int32_t InsertAsset(string photoId)
{
    if (g_rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(Media::PhotoColumn::PHOTO_IS_TEMP, IS_TEMP);
    values.PutInt(Media::PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(FuzzMultiStagesPhotoQuality()));
    values.PutInt(Media::MediaColumn::MEDIA_TYPE, FuzzMediaType());
    values.PutLong(Media::PhotoColumn::PHOTO_EDIT_TIME, provider->ConsumeIntegral<int64_t>());
    values.PutLong(Media::MediaColumn::MEDIA_DATE_TRASHED, provider->ConsumeIntegral<int64_t>());
    values.PutString(Media::PhotoColumn::PHOTO_ID, photoId);
    values.PutString(Media::MediaColumn::MEDIA_NAME, FuzzMimeTypeAndDisplayNameExtension());
    values.PutString(Media::MediaColumn::MEDIA_FILE_PATH, CLOUD_PHOTO_PATH +
        provider->ConsumeBytesAsString(NUM_BYTES) + "1234.jpg");
    int64_t fileId = 0;
    g_rdbStore->Insert(fileId, PHOTOS_TABLE, values);
    return static_cast<int32_t>(fileId);
}

unique_ptr<FileAsset> QueryPhotoAsset(const string &columnName, const string &value)
{
    string querySql = "SELECT * FROM " + PhotoColumn::PHOTOS_TABLE + " WHRE " +
        columnName + "='" + value + "';";
    MEDIA_DEBUG_LOG("querySql: %{public}s", querySql.c_str());
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("g_rdbStore is nullptr");
        return nullptr;
    }
    auto resultSet = g_rdbStore->QuerySql(querySql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Get resultSet failed");
        return nullptr;
    }

    int32_t resultSetCount = 0;
    int32_t ret = resultSet->GetRowCount(resultSetCount);
    if (ret != NativeRdb::E_OK || resultSetCount <= 0) {
        MEDIA_ERR_LOG("resultSet row count is 0");
        return nullptr;
    }

    shared_ptr<FetchResult<FileAsset>> fetchFileResult = make_shared<FetchResult<FileAsset>>();
    if (fetchFileResult == nullptr) {
        MEDIA_ERR_LOG("Get fetchFileResult failed");
        return nullptr;
    }
    auto fileAsset = fetchFileResult->GetObjectFromRdb(resultSet, 0);
    if (fileAsset == nullptr || fileAsset->GetId() < 0) {
        return nullptr;
    }
    return fileAsset;
}

void SetTables()
{
    vector<string> createTableSqlList = { Media::PhotoColumn::CREATE_PHOTO_TABLE };
    for (auto &createTableSql : createTableSqlList) {
        CHECK_AND_RETURN_LOG(g_rdbStore != nullptr, "g_rdbStore is null.");
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed.", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success.", createTableSql.c_str());
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

static void MultistagesCaptureDeferredPhotoProcSessionCallbackTest()
{
    MEDIA_INFO_LOG("MultistagesCaptureDeferredPhotoProcSessionCallbackTest start");
    shared_ptr<Media::MultiStagesCaptureDeferredPhotoProcSessionCallback> callback =
        make_shared<Media::MultiStagesCaptureDeferredPhotoProcSessionCallback>();
    std::string photoId = provider->ConsumeBytesAsString(NUM_BYTES);
    int32_t fileId = InsertAsset(photoId);
    MEDIA_DEBUG_LOG("fileId: %{public}d.", fileId);
    CameraStandard::DpsErrorCode errCode = FuzzDpsErrorCode();
    callback->OnError(photoId, errCode);

    auto fileAssetPtr = QueryPhotoAsset(PhotoColumn::MEDIA_ID, to_string(fileId));
    shared_ptr<FileAsset> fileAsset = std::move(fileAssetPtr);
    callback->NotifyIfTempFile(fileAsset, provider->ConsumeBool());
    callback->UpdateHighQualityPictureInfo(fileId, provider->ConsumeBool(),
        static_cast<int32_t>(FuzzFirstStageModifyType()));

    sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
    std::shared_ptr<CameraStandard::PictureIntf> picture = std::make_shared<CameraStandard::PictureAdapter>();
    picture->Create(surfaceBuffer);
    uint8_t *addr = new uint8_t();
    long bytes = provider->ConsumeIntegral<long>();
    uint32_t cloudImageEnhanceFlag = provider->ConsumeIntegral<uint32_t>();
    DpsMetadata metadata;
    constexpr const char* cloudFlag = "cloudImageEnhanceFlag";
    metadata.Set(cloudFlag, cloudImageEnhanceFlag);
    callback->OnProcessImageDone(photoId, picture, metadata);
    callback->OnProcessImageDone(photoId, addr, bytes, cloudImageEnhanceFlag);

    photoId = "/test" + SPLIT_PATH + provider->ConsumeBytesAsString(NUM_BYTES);
    callback->OnDeliveryLowQualityImage(photoId, picture);
    delete addr;
    addr = nullptr;
    MEDIA_INFO_LOG("MultistagesCaptureDeferredPhotoProcSessionCallbackTest end");
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
        return Media::E_ERR;
    }
    file.write(seedData, SEED_SIZE);
    file.close();
    delete[] seedData;
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

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    OHOS::Media::provider = &fdp;
    if (data == nullptr) {
        return 0;
    }
    OHOS::Media::MultistagesCaptureDeferredPhotoProcSessionCallbackTest();
    OHOS::Media::ClearKvStore();
    return 0;
}
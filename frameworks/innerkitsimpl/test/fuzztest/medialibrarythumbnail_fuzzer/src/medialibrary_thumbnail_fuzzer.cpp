/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "medialibrary_thumbnail_fuzzer.h"
#include "medialibrary_rdbstore_utils_fuzzer.h"

#include <cstdint>
#include <string>
#include <pixel_map.h>
#include <fstream>
#include <fuzzer/FuzzedDataProvider.h>

#include "ability_context_impl.h"
#include "abs_rdb_predicates.h"
#include "cloud_thumbnail_observer.h"
#include "datashare_helper.h"
#include "image_type.h"
#include "ithumbnail_helper.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_kvstore_manager.h"
#include "medialibrary_unistore_manager.h"
#include "rdb_predicates.h"
#include "userfile_manager_types.h"

#define private public
#include "thumbnail_aging_helper.h"
#include "thumbnail_file_utils.h"
#include "thumbnail_generate_helper.h"
#include "thumbnail_generate_worker.h"
#include "thumbnail_generate_worker_manager.h"
#include "thumbnail_generation_post_process.h"
#include "thumbnail_image_framework_utils.h"
#include "thumbnail_rdb_utils.h"
#include "thumbnail_restore_manager.h"
#include "thumbnail_service.h"
#include "thumbnail_source_loading.h"
#include "thumbnail_uri_utils.h"
#undef private
#include "media_upgrade.h"

namespace OHOS {
using namespace std;
using namespace DataShare;
using namespace DistributedKv;
using namespace NativeRdb;
using namespace AAFwk;
using ChangeType = DataShare::DataShareObserver::ChangeType;
const string PHOTOS_TABLE = "Photos";
const string TEST_IMAGE_PATH = "/storage/cloud/files/Photo/1/CreateImageLcdTest_001.jpg";
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;
const int32_t PIXELMAP_WIDTH_AND_HEIGHT = 1000;
const int32_t NUM_BYTES = 1;
const int32_t REMAINDER_1 = 1;
const int32_t REMAINDER_2 = 2;
const int32_t REMAINDER_3 = 3;
const int32_t REMAINDER_4 = 4;
const int32_t MIN_THUMBNAIL_TYPE = -1;
const int32_t MAX_THUMBNAIL_TYPE = 8;
const int32_t MAX_MEDIA_TYPE = 14;
const int32_t MAX_BYTE_VALUE = 256;
const int32_t SEED_SIZE = 1024;
FuzzedDataProvider *provider = nullptr;

static inline Media::ThumbnailType FuzzThumbnailType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(MIN_THUMBNAIL_TYPE, MAX_THUMBNAIL_TYPE);
    return static_cast<Media::ThumbnailType>(value);
}

static inline Media::Size FuzzSize()
{
    Media::Size value;
    value.width = provider->ConsumeIntegral<int32_t>();
    value.height = provider->ConsumeIntegral<int32_t>();
    return value;
}

static Media::ThumbRdbOpt FuzzThumbRdbOpt(bool isNeedNullptr)
{
    std::shared_ptr<Media::MediaLibraryRdbStore> store;
    if (isNeedNullptr) {
        store = provider->ConsumeBool() ?
            Media::MediaLibraryUnistoreManager::GetInstance().GetRdbStore() : nullptr;
    } else {
        store = Media::MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    }

    Media::ThumbRdbOpt opt = {
        .store = store,
        .networkId = provider->ConsumeBytesAsString(NUM_BYTES),
        .path = provider->ConsumeBytesAsString(NUM_BYTES),
        .table = provider->ConsumeBool() ? PHOTOS_TABLE : provider->ConsumeBytesAsString(NUM_BYTES),
        .row = provider->ConsumeBytesAsString(NUM_BYTES),
        .dateTaken = provider->ConsumeBytesAsString(NUM_BYTES),
        .fileUri = provider->ConsumeBytesAsString(NUM_BYTES)
    };
    return opt;
}

static inline Media::MediaType FuzzMediaType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_MEDIA_TYPE);
    return static_cast<Media::MediaType>(value);
}

static Media::ThumbnailData FuzzThumbnailData()
{
    Media::ThumbnailData datas;
    datas.path = provider->ConsumeBytesAsString(NUM_BYTES);
    datas.mediaType = FuzzMediaType();
    if (provider->ConsumeBool()) {
        datas.id = to_string(provider->ConsumeIntegral<int32_t>());
        datas.dateTaken = to_string(provider->ConsumeIntegral<int32_t>());
    }
    return datas;
}

static std::shared_ptr<Media::PixelMap> CreateTestPixelMap(Media::PixelFormat format, bool useDMA)
{
    Media::InitializationOptions opts;
    opts.size.width = PIXELMAP_WIDTH_AND_HEIGHT;
    opts.size.height = PIXELMAP_WIDTH_AND_HEIGHT;
    opts.srcPixelFormat = format;
    opts.pixelFormat = format;
    opts.useDMA = useDMA;
    std::shared_ptr<Media::PixelMap> pixelMap = Media::PixelMap::Create(opts);
    return pixelMap;
}

static std::shared_ptr<Media::PixelMap> FuzzNormalPixelMap(bool isNeedNullptr = false)
{
    int32_t value = abs(provider->ConsumeIntegral<int32_t>()) % 3;
    Media::PixelFormat format = Media::PixelFormat::RGBA_8888;
    bool useDMA = provider->ConsumeBool();
    if (value == REMAINDER_1) {
        format = Media::PixelFormat::RGBA_8888;
    } else if (value == REMAINDER_2) {
        format = Media::PixelFormat::RGBA_1010102;
        useDMA = true;
    } else if (!isNeedNullptr) {
        format = Media::PixelFormat::RGBA_8888;
    } else {
        return nullptr;
    }
    return CreateTestPixelMap(format, useDMA);
}

static std::shared_ptr<Media::PixelMap> FuzzYuvPixelMap(bool isNeedNullptr = false)
{
    int32_t value = abs(provider->ConsumeIntegral<int32_t>()) % 5;
    Media::PixelFormat format = Media::PixelFormat::NV12;
    bool useDMA = provider->ConsumeBool();
    if (value == REMAINDER_1) {
        format = Media::PixelFormat::NV12;
    } else if (value == REMAINDER_2) {
        format = Media::PixelFormat::NV21;
    } else if (value == REMAINDER_3) {
        format = Media::PixelFormat::YCBCR_P010;
        useDMA = true;
    } else if (value == REMAINDER_4) {
        format = Media::PixelFormat::YCRCB_P010;
        useDMA = true;
    } else if (!isNeedNullptr) {
        format = Media::PixelFormat::NV12;
    } else {
        return nullptr;
    }
    return CreateTestPixelMap(format, useDMA);
}

static std::shared_ptr<Media::Picture> FuzzPicture(bool isNeedGainMap, bool isYuv, bool isNeedNullptr = false)
{
    std::shared_ptr<Media::PixelMap> pixelMap;
    if (isYuv) {
        pixelMap = FuzzYuvPixelMap(isNeedNullptr);
    } else {
        pixelMap = FuzzNormalPixelMap(isNeedNullptr);
    }
    if (pixelMap == nullptr) {
        return nullptr;
    }

    auto sourcePtr = Media::Picture::Create(pixelMap);
    std::shared_ptr<Media::Picture> picture = std::move(sourcePtr);
    if (!isNeedGainMap) {
        return picture;
    }

    std::shared_ptr<Media::PixelMap> gainMap;
    if (isYuv) {
        gainMap = FuzzYuvPixelMap(isNeedNullptr);
    } else {
        gainMap = FuzzNormalPixelMap(isNeedNullptr);
    }
    if (gainMap == nullptr) {
        return nullptr;
    }

    Media::Size gainMapSize = {gainMap->GetWidth(), gainMap->GetHeight()};
    auto auxiliaryPicturePtr = Media::AuxiliaryPicture::Create(gainMap,
        Media::AuxiliaryPictureType::GAINMAP, gainMapSize);
    std::shared_ptr<Media::AuxiliaryPicture> auxiliaryPicture = std::move(auxiliaryPicturePtr);
    CHECK_AND_RETURN_RET_LOG(auxiliaryPicture != nullptr, nullptr, "Create auxiliaryPicture failed");
    picture->SetAuxiliaryPicture(auxiliaryPicture);
    return picture;
}

static void ThumbnailGenerateHelperTestPart2()
{
    Media::ThumbRdbOpt opts = FuzzThumbRdbOpt(true);
    vector<Media::ThumbnailData> outDatas;
    Media::ThumbnailGenerateHelper::GetLocalNoLcdData(opts, outDatas);
    outDatas.clear();
    int64_t time = provider->ConsumeIntegral<int64_t>();
    int32_t count;
    Media::ThumbnailGenerateHelper::GetNewThumbnailCount(opts, time, count);
    Media::ThumbnailData thumbData = FuzzThumbnailData();
    string fileName;
    int32_t thumbType = abs(provider->ConsumeIntegral<int32_t>()) % 4;
    int32_t timeStamp = provider->ConsumeIntegral<int32_t>();
    Media::ThumbnailGenerateHelper::GetAvailableFile(opts, thumbData, FuzzThumbnailType(), fileName);
    Media::ThumbnailGenerateHelper::GetAvailableKeyFrameFile(opts, thumbData, thumbType, fileName);
    Media::ThumbnailGenerateHelper::GetThumbnailPixelMap(thumbData, opts, FuzzThumbnailType());
    Media::ThumbnailGenerateHelper::UpgradeThumbnailBackground(opts, provider->ConsumeBool());
    outDatas.clear();
    Media::ThumbnailGenerateHelper::GetThumbnailDataNeedUpgrade(opts, outDatas, provider->ConsumeBool());
    Media::ThumbnailGenerateHelper::CheckMonthAndYearKvStoreValid(opts);
    Media::ThumbnailGenerateHelper::GenerateHighlightThumbnailBackground(opts);
    std::string id = to_string(provider->ConsumeIntegral<int64_t>());
    std::string tracks = provider->ConsumeBytesAsString(NUM_BYTES);
    std::string trigger = provider->ConsumeBytesAsString(NUM_BYTES);
    std::string gentype = provider->ConsumeBytesAsString(NUM_BYTES);
    Media::ThumbnailGenerateHelper::TriggerHighlightThumbnail(opts, id, tracks, trigger, gentype);
    Media::ThumbRdbOpt optsWithRdb = FuzzThumbRdbOpt(false);
    if (optsWithRdb.store != nullptr) {
        outDatas.clear();
        Media::ThumbnailGenerateHelper::GetNoHighlightData(optsWithRdb, outDatas);
        outDatas.clear();
        Media::ThumbnailGenerateHelper::GetKeyFrameThumbnailPixelMap(optsWithRdb, timeStamp, thumbType);
        Media::ThumbnailGenerateHelper::CreateThumbnailFileScanedWithPicture(
            optsWithRdb, FuzzPicture(true, true, true), provider->ConsumeBool());
    }
}

void SetTables()
{
    vector<string> createTableSqlList = { Media::PhotoUpgrade::CREATE_PHOTO_TABLE };
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

static void ThumbnailSourceTest2()
{
    Media::Size desiredSize = FuzzSize();
    Media::ThumbnailData thumbnailData = FuzzThumbnailData();
    Media::SourceLoader sourceLoader(desiredSize, thumbnailData);
    sourceLoader.CreateVideoFramePixelMap();
    sourceLoader.SetCurrentStateFunction();

    thumbnailData = FuzzThumbnailData();
    thumbnailData.originalPhotoPicture = FuzzPicture(true, true, true);
    Media::SourceLoader sourceLoader2(desiredSize, thumbnailData);
    sourceLoader2.RunLoading();
    thumbnailData = FuzzThumbnailData();
    thumbnailData.originalPhotoPicture = FuzzPicture(false, false, true);
    Media::SourceLoader sourceLoader3(desiredSize, thumbnailData);
    sourceLoader3.CreateSourceFromOriginalPhotoPicture();

    thumbnailData = FuzzThumbnailData();
    thumbnailData.originalPhotoPicture = FuzzPicture(true, false, true);
    Media::SourceLoader sourceLoader4(desiredSize, thumbnailData);
    sourceLoader4.CreateSourceWithWholeOriginalPicture();

    thumbnailData = FuzzThumbnailData();
    thumbnailData.originalPhotoPicture = FuzzPicture(false, false, true);
    Media::SourceLoader sourceLoader5(desiredSize, thumbnailData);
    sourceLoader5.CreateSourceWithOriginalPictureMainPixel();

    thumbnailData = FuzzThumbnailData();
    desiredSize = FuzzSize();
    thumbnailData.path = TEST_IMAGE_PATH;
    thumbnailData.mediaType = Media::MediaType::MEDIA_TYPE_IMAGE;
    thumbnailData.loaderOpts.loadingStates = {
        { Media::SourceState::BEGIN, Media::SourceState::LOCAL_ORIGIN },
        { Media::SourceState::LOCAL_ORIGIN, Media::SourceState::FINISH },
    };
    Media::SourceLoader sourceLoader6(desiredSize, thumbnailData);
    int32_t ret = sourceLoader6.RunLoading();
    MEDIA_INFO_LOG("sourceLoader6.RunLoading image path: %{public}s. ret: %{public}d", TEST_IMAGE_PATH.c_str(), ret);
}

static void ThumbnailImageFrameworkUtilsTest()
{
    std::shared_ptr<Media::PixelMap> pixelMap = FuzzNormalPixelMap(true);
    std::shared_ptr<Media::Picture> picture = FuzzPicture(true, false, true);
    Media::ThumbnailImageFrameWorkUtils::IsYuvPixelMap(pixelMap);
    Media::ThumbnailImageFrameWorkUtils::IsSupportCopyPixelMap(pixelMap);
    Media::ThumbnailImageFrameWorkUtils::CopyPictureSource(picture);
    Media::ThumbnailImageFrameWorkUtils::CopyPixelMapSource(pixelMap);

    pixelMap = FuzzYuvPixelMap(false);
    picture = FuzzPicture(true, true, false);
    Media::ThumbnailImageFrameWorkUtils::IsYuvPixelMap(pixelMap);
    Media::ThumbnailImageFrameWorkUtils::IsSupportCopyPixelMap(pixelMap);
    Media::ThumbnailImageFrameWorkUtils::CopyPictureSource(picture);
    Media::ThumbnailImageFrameWorkUtils::CopyPixelMapSource(pixelMap);
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

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    OHOS::provider = &fdp;
    if (data == nullptr) {
        return 0;
    }
    OHOS::ThumbnailGenerateHelperTestPart2();
    OHOS::ThumbnailSourceTest2();
    OHOS::ThumbnailImageFrameworkUtilsTest();
    return 0;
}
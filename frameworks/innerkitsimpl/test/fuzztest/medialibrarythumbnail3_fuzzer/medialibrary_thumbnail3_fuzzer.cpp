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
#include "medialibrary_thumbnail3_fuzzer.h"
#include "medialibrary_rdbstore_utils_fuzzer.h"

#include <cstdint>
#include <string>
#include <pixel_map.h>
#include <fstream>
#include <fuzzer/FuzzedDataProvider.h>

#include "ability_context_impl.h"
#include "datashare_helper.h"
#include "media_file_utils.h"
#include "media_pure_file_utils.h"
#include "media_upgrade.h"
#include "thumbnail_file_utils.h"
#include "thumbnail_generation_post_process.h"
#include "thumbnail_rdb_utils.h"
#include "thumbnail_restore_manager.h"
#include "thumbnail_source_loading.h"
#include "thumbnail_uri_utils.h"

namespace OHOS {
using namespace std;
using namespace DataShare;
using namespace DistributedKv;
using namespace NativeRdb;
using namespace AAFwk;
using ChangeType = DataShare::DataShareObserver::ChangeType;
const string PHOTOS_TABLE = "Photos";
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;
const int32_t NUM_BYTES = 1;
const int32_t MIN_THUMBNAIL_TYPE = -1;
const int32_t MAX_THUMBNAIL_TYPE = 8;
const int32_t MAX_PIXEL_FORMAT = 15;
const int32_t MAX_MEDIA_TYPE = 14;
const int32_t MAX_BYTE_VALUE = 256;
const int32_t SEED_SIZE = 1024;
const int32_t MAX_NOTIFY_TYPE = 8;
const int32_t MAX_DIRTY_TYPE = 8;
FuzzedDataProvider *provider = nullptr;

static inline Media::ThumbnailType FuzzThumbnailType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(MIN_THUMBNAIL_TYPE, MAX_THUMBNAIL_TYPE);
    return static_cast<Media::ThumbnailType>(value);
}

static inline Media::NotifyType FuzzNotifyType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_NOTIFY_TYPE);
    return static_cast<Media::NotifyType>(value);
}

static inline Media::DirtyType FuzzDirtyType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_DIRTY_TYPE);
    return static_cast<Media::DirtyType>(value);
}

static inline Media::Size FuzzSize()
{
    Media::Size value;
    value.width = provider->ConsumeIntegral<int32_t>();
    value.height = provider->ConsumeIntegral<int32_t>();
    return value;
}

static inline Media::PixelFormat FuzzPixelFormat()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_PIXEL_FORMAT);
    return static_cast<Media::PixelFormat>(value);
}

static inline Media::DecodeOptions FuzzDecodeOptions()
{
    Media::DecodeOptions value;
    value.desiredPixelFormat = FuzzPixelFormat();
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

static string FuzzThumbnailUri(bool isNeedPath)
{
    if (!provider->ConsumeBool()) {
        return provider->ConsumeBytesAsString(NUM_BYTES);
    }

    string thumUri = "file://media/Photo/1?operation=thumbnail";
    Media::Size value = FuzzSize();
    thumUri += "&width=" + to_string(value.width) + "&height=" + to_string(value.height);
    thumUri += "&date_modified=" + to_string(provider->ConsumeIntegral<int64_t>());
    int32_t thumbType = abs(provider->ConsumeIntegral<int32_t>()) % 4;
    thumUri += "&type=" + to_string(thumbType);
    thumUri += "&begin_stamp=" + to_string(provider->ConsumeIntegral<int32_t>());
    if (isNeedPath) {
        thumUri += "&path=" + provider->ConsumeBytesAsString(NUM_BYTES);
    }
    thumUri += "&date_taken=" + to_string(provider->ConsumeIntegral<int64_t>());

    return thumUri;
}

static std::string FuzzThumbnailPath()
{
    std::string path = "/storage/cloud/files/" + to_string(provider->ConsumeIntegral<int32_t>()) + "/fuzztest";
    Media::ThumbnailData thumbnailData;
    thumbnailData.path = path;
    std::string thumbnailDir = Media::ThumbnailFileUtils::GetThumbnailDir(thumbnailData);
    Media::MediaPureFileUtils::CreateDirectory(thumbnailDir);

    std::string lcdPath = thumbnailDir + "/LCD.jpg";
    Media::MediaFileUtils::CreateFile(lcdPath);

    std::string thumbPath = thumbnailDir + "/THM.jpg";
    Media::MediaFileUtils::CreateFile(thumbPath);

    std::string astcPath = thumbnailDir + "/THM_ASTC.astc";
    Media::MediaFileUtils::CreateFile(astcPath);

    std::string thumbExDir = thumbnailDir + "/THM_EX";
    Media::MediaPureFileUtils::CreateDirectory(thumbExDir);
    std::string thumbExFile = thumbExDir + "/THM.jpg";
    Media::MediaFileUtils::CreateFile(thumbExFile);
    thumbExFile = thumbExDir + "/LCD.jpg";
    Media::MediaFileUtils::CreateFile(thumbExFile);

    std::string beginTimeStampDir = thumbnailDir + "/beginTimeStampDir0";
    Media::MediaPureFileUtils::CreateDirectory(beginTimeStampDir);
    std::string beginTimeStampFile = beginTimeStampDir + "/THM.jpg";
    Media::MediaFileUtils::CreateFile(beginTimeStampFile);
    beginTimeStampFile = beginTimeStampDir + "/LCD.jpg";
    Media::MediaFileUtils::CreateFile(beginTimeStampFile);
    return path;
}

static void ThumbnailGenerationPostProcessTest()
{
    Media::ThumbnailData thumbnailData = FuzzThumbnailData();
    Media::ThumbRdbOpt opts = FuzzThumbRdbOpt(true);
    Media::ThumbnailGenerationPostProcess::UpdateCachedRdbValueAndNotify(thumbnailData, opts);
    Media::ThumbnailGenerationPostProcess::Notify(thumbnailData, FuzzNotifyType());
    Media::NotifyType notifyType = FuzzNotifyType();
    Media::ThumbnailGenerationPostProcess::GetNotifyType(thumbnailData, opts, notifyType);
}

static void ThumbnailRestoreManagerTest()
{
    auto& restoreManager = Media::ThumbnailRestoreManager::GetInstance();
    restoreManager.InitializeRestore(provider->ConsumeIntegral<int64_t>());
    restoreManager.AddCompletedTasks(provider->ConsumeIntegral<int64_t>());
    restoreManager.StartProgressReporting(provider->ConsumeIntegral<int32_t>());
    restoreManager.StopProgressReporting();
    restoreManager.OnScreenStateChanged(provider->ConsumeBool());
    restoreManager.ReportProgressBegin();
    restoreManager.ReportProgress(provider->ConsumeBool());
    Media::ThumbRdbOpt opts = FuzzThumbRdbOpt(true);
    Media::ThumbnailData thumbnailData = FuzzThumbnailData();
    std::shared_ptr<Media::ThumbnailTaskData> taskData =
        std::make_shared<Media::ThumbnailTaskData>(opts, thumbnailData);
    Media::ThumbnailRestoreManager::RestoreAstcDualFrameTask(taskData);
    restoreManager.RestoreAstcDualFrame(opts, provider->ConsumeIntegral<int32_t>());
    restoreManager.Reset();
}

static void ThumbnailRdbUtilsTest()
{
    Media::ThumbnailData thumbnailData = FuzzThumbnailData();
    vector<string> columns = {Media::PhotoColumn::PHOTO_EXIF_ROTATE};
    NativeRdb::RdbPredicates rdbPredicates(Media::PhotoColumn::PHOTOS_TABLE);
    auto resultSet = g_rdbStore->QueryByStep(rdbPredicates, columns);
    Media::ThumbnailRdbUtils::HandleId(resultSet, provider->ConsumeIntegral<int>(), thumbnailData);
    Media::ThumbnailRdbUtils::HandleFilePath(resultSet, provider->ConsumeIntegral<int>(), thumbnailData);
    Media::ThumbnailRdbUtils::HandleDateAdded(resultSet, provider->ConsumeIntegral<int>(), thumbnailData);
    Media::ThumbnailRdbUtils::HandleDisplayName(resultSet, provider->ConsumeIntegral<int>(), thumbnailData);
    Media::ThumbnailRdbUtils::HandleDateTaken(resultSet, provider->ConsumeIntegral<int>(), thumbnailData);
    Media::ThumbnailRdbUtils::HandleDateModified(resultSet, provider->ConsumeIntegral<int>(), thumbnailData);
    Media::ThumbnailRdbUtils::HandleMediaType(resultSet, provider->ConsumeIntegral<int>(), thumbnailData);
    Media::ThumbnailRdbUtils::HandleOrientation(resultSet, provider->ConsumeIntegral<int>(), thumbnailData);
    Media::ThumbnailRdbUtils::HandleExifRotate(resultSet, provider->ConsumeIntegral<int>(), thumbnailData);
    Media::ThumbnailRdbUtils::HandlePosition(resultSet, provider->ConsumeIntegral<int>(), thumbnailData);
    Media::ThumbnailRdbUtils::HandlePhotoHeight(resultSet, provider->ConsumeIntegral<int>(), thumbnailData);
    Media::ThumbnailRdbUtils::HandlePhotoWidth(resultSet, provider->ConsumeIntegral<int>(), thumbnailData);
    Media::ThumbnailRdbUtils::HandleDirty(resultSet, provider->ConsumeIntegral<int>(), thumbnailData);
    Media::ThumbnailRdbUtils::HandleReady(resultSet, provider->ConsumeIntegral<int>(), thumbnailData);
    Media::ThumbnailRdbUtils::HandleLcdVisitTime(resultSet, provider->ConsumeIntegral<int>(), thumbnailData);
    vector<Media::ThumbnailData> outdatas;
    int err;
    Media::ThumbnailRdbUtils::QueryThumbnailDataInfos(resultSet, columns, outdatas, err);
    outdatas.clear();
    Media::ThumbnailRdbUtils::QueryThumbnailDataInfo(g_rdbStore, rdbPredicates, columns, thumbnailData);
    Media::ThumbnailRdbUtils::QueryThumbnailDataInfo(resultSet, columns, thumbnailData, err);
    Media::ThumbnailRdbUtils::CheckResultSetCount(resultSet, err);
    Media::ThumbnailRdbUtils::ParseQueryResult(resultSet, thumbnailData, err, columns);
    Media::ThumbRdbOpt opts = FuzzThumbRdbOpt(true);
    Media::ThumbnailRdbUtils::QueryLocalNoExifRotateInfos(opts, outdatas);
    Media::ThumbnailRdbUtils::UpdateExifRotateAndDirty(thumbnailData, FuzzDirtyType(), provider->ConsumeBool());
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

static void ThumbnailSourceTest()
{
    Media::GetLocalThumbnailPath(provider->ConsumeBytesAsString(NUM_BYTES), provider->ConsumeBytesAsString(NUM_BYTES));
    int32_t error;
    int32_t minSize = provider->ConsumeIntegral<int32_t>();
    Media::ThumbnailData thumbnailData = FuzzThumbnailData();
    Media::LocalThumbSource::GetSourcePath(thumbnailData, error);
    Media::LocalThumbSource::IsSizeLargeEnough(thumbnailData, minSize);
    thumbnailData = FuzzThumbnailData();
    Media::LocalLcdSource::GetSourcePath(thumbnailData, error);
    Media::LocalLcdSource::IsSizeLargeEnough(thumbnailData, minSize);
    thumbnailData = FuzzThumbnailData();
    Media::LocalOriginSource::GetSourcePath(thumbnailData, error);
    Media::LocalOriginSource::IsSizeLargeEnough(thumbnailData, minSize);

    thumbnailData = FuzzThumbnailData();
    Media::CloudThumbSource::GetSourcePath(thumbnailData, error);
    Media::CloudThumbSource::IsSizeLargeEnough(thumbnailData, minSize);

    thumbnailData = FuzzThumbnailData();
    Media::CloudLcdSource::GetSourcePath(thumbnailData, error);
    Media::CloudLcdSource::IsSizeLargeEnough(thumbnailData, minSize);

    thumbnailData = FuzzThumbnailData();
    Media::CloudOriginSource::GetSourcePath(thumbnailData, error);
    Media::CloudOriginSource::IsSizeLargeEnough(thumbnailData, minSize);
    Media::NeedAutoResize(FuzzSize());

    Media::DecodeOptions decodeOpts = FuzzDecodeOptions();
    Media::GenDecodeOpts(FuzzSize(), FuzzSize(), decodeOpts);

    thumbnailData = FuzzThumbnailData();
    Media::Size sourceSize = FuzzSize();
    Media::Size desiredSize = FuzzSize();
    Media::ConvertDecodeSize(thumbnailData, sourceSize, desiredSize);
    uint32_t err = 0;
    Media::LoadImageSource(provider->ConsumeBytesAsString(NUM_BYTES), err);
}

static void ParseFileUriTest()
{
    string outFileId;
    string outNetworkId;
    string outTableName;
    Media::Size outSize;
    string outPath;
    int32_t outType = 0;
    int32_t outBeginStamp = 0;
    string uri = FuzzThumbnailUri(true);
    Media::ThumbnailUriUtils::ParseFileUri(uri, outFileId, outNetworkId, outTableName);
    Media::ThumbnailUriUtils::ParseThumbnailInfo(uri, outFileId, outSize, outPath, outTableName);
    Media::ThumbnailUriUtils::ParseKeyFrameThumbnailInfo(uri, outFileId, outBeginStamp, outType, outPath);
    Media::ThumbnailUriUtils::GetDateTakenFromUri(uri);
    Media::ThumbnailUriUtils::GetDateModifiedFromUri(uri);
    Media::ThumbnailUriUtils::GetFileUriFromUri(uri);
    Media::Size checkSize = FuzzSize();
    Media::ThumbnailUriUtils::IsOriginalImg(checkSize, outPath);
    Media::ThumbnailUriUtils::CheckSize(checkSize, outPath);
    Media::ThumbnailUriUtils::GetTableFromUri(uri);
}

static void ThumbnailFileUtilsTest()
{
    Media::ThumbnailData thumbnailData;
    thumbnailData.path = FuzzThumbnailPath();
    Media::ThumbnailFileUtils::DeleteThumbnailDir(thumbnailData);
    thumbnailData.path = FuzzThumbnailPath();
    size_t size;
    Media::ThumbnailFileUtils::GetThumbFileSize(thumbnailData, FuzzThumbnailType(), size);
    Media::ThumbnailFileUtils::DeleteAllThumbFiles(thumbnailData);
    Media::ThumbnailFileUtils::DeleteMonthAndYearAstc(thumbnailData);
    Media::ThumbnailFileUtils::CheckRemainSpaceMeetCondition(provider->ConsumeIntegral<int32_t>());
    Media::ThumbnailFileUtils::DeleteAstcDataFromKvStore(thumbnailData, FuzzThumbnailType());
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
    if (data == nullptr) {
        return 0;
    }
    FuzzedDataProvider fdp(data, size);
    OHOS::provider = &fdp;

    OHOS::ThumbnailSourceTest();
    OHOS::ParseFileUriTest();
    OHOS::ThumbnailFileUtilsTest();
    OHOS::ThumbnailGenerationPostProcessTest();
    OHOS::ThumbnailRestoreManagerTest();
    OHOS::ThumbnailRdbUtilsTest();
    return 0;
}
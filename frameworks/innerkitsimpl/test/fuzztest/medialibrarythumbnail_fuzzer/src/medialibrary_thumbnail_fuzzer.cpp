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

namespace OHOS {
using namespace std;
using namespace DataShare;
using namespace DistributedKv;
using namespace NativeRdb;
using namespace AAFwk;
using ChangeType = DataShare::DataShareObserver::ChangeType;
using ThumbnailGenerateExecute = void (*)(std::shared_ptr<Media::ThumbnailTaskData> &data);
using ThumbnailWorkerPtr = std::shared_ptr<Media::ThumbnailGenerateWorker>;
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
const int32_t MIN_TASK_TYPE = -1;
const int32_t MIN_TASK_PRIORITY = -1;
const int32_t MAX_TASK_TYPE = 1;
const int32_t MAX_TASK_PRIORITY = 2;
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

static inline Media::ThumbnailTaskType FuzzThumbnailTaskType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(MIN_TASK_TYPE, MAX_TASK_TYPE);
    return static_cast<Media::ThumbnailTaskType>(value);
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

static Media::ThumbnailTaskPriority FuzzThumbnailTaskPriority()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(MIN_TASK_PRIORITY, MAX_TASK_PRIORITY);
    return static_cast<Media::ThumbnailTaskPriority>(value);
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

static std::string FuzzThumbnailPath()
{
    std::string path = "/storage/cloud/files/" + to_string(provider->ConsumeIntegral<int32_t>()) + "/fuzztest";
    Media::ThumbnailData thumbnailData;
    thumbnailData.path = path;
    std::string thumbnailDir = Media::ThumbnailFileUtils::GetThumbnailDir(thumbnailData);
    Media::MediaFileUtils::CreateDirectory(thumbnailDir);

    std::string lcdPath = thumbnailDir + "/LCD.jpg";
    Media::MediaFileUtils::CreateFile(lcdPath);

    std::string thumbPath = thumbnailDir + "/THM.jpg";
    Media::MediaFileUtils::CreateFile(thumbPath);

    std::string astcPath = thumbnailDir + "/THM_ASTC.astc";
    Media::MediaFileUtils::CreateFile(astcPath);

    std::string thumbExDir = thumbnailDir + "/THM_EX";
    Media::MediaFileUtils::CreateDirectory(thumbExDir);
    std::string thumbExFile = thumbExDir + "/THM.jpg";
    Media::MediaFileUtils::CreateFile(thumbExFile);
    thumbExFile = thumbExDir + "/LCD.jpg";
    Media::MediaFileUtils::CreateFile(thumbExFile);

    std::string beginTimeStampDir = thumbnailDir + "/beginTimeStampDir0";
    Media::MediaFileUtils::CreateDirectory(beginTimeStampDir);
    std::string beginTimeStampFile = beginTimeStampDir + "/THM.jpg";
    Media::MediaFileUtils::CreateFile(beginTimeStampFile);
    beginTimeStampFile = beginTimeStampDir + "/LCD.jpg";
    Media::MediaFileUtils::CreateFile(beginTimeStampFile);
    return path;
}

static void ThumbnailAgingHelperTest()
{
    Media::ThumbRdbOpt opt = FuzzThumbRdbOpt(false);
    Media::ThumbnailAgingHelper::AgingLcdBatch(opt);
    int64_t time = provider->ConsumeIntegral<int64_t>();
    bool before = provider->ConsumeBool();
    int outLcdCount;
    Media::ThumbnailAgingHelper::GetAgingDataCount(time, before, opt, outLcdCount);

    vector<Media::ThumbnailData> infos;
    Media::ThumbnailAgingHelper::GetAgingLcdData(opt, provider->ConsumeIntegral<int32_t>(), infos);
    Media::ThumbnailAgingHelper::GetLcdCountByTime(provider->ConsumeIntegral<int64_t>(), provider->ConsumeBool(),
        opt, outLcdCount);
}

static void ThumbnailGenerateHelperTestPart1()
{
    Media::ThumbRdbOpt opts = FuzzThumbRdbOpt(true);
    Media::ThumbnailGenerateHelper::CreateThumbnailFileScaned(opts, provider->ConsumeBool());
    Media::ThumbnailGenerateHelper::CreateThumbnailBackground(opts);
    Media::ThumbnailGenerateHelper::CreateAstcBackground(opts);
    Media::ThumbnailGenerateHelper::CreateAstcCloudDownload(opts, provider->ConsumeBool());
    Media::ThumbnailGenerateHelper::CreateAstcMthAndYear(opts);
    Media::ThumbnailGenerateHelper::RegenerateThumbnailFromCloud(opts);
    Media::ThumbnailGenerateHelper::RepairExifRotateBackground(opts);
    RdbPredicates predicates(PHOTOS_TABLE);
    Media::ThumbnailGenerateHelper::CreateLcdBackground(opts);
    Media::ThumbnailGenerateHelper::CheckLcdSizeAndUpdateStatus(opts);
    int32_t outLcdCount;
    Media::ThumbnailGenerateHelper::GetLcdCount(opts, outLcdCount);
}

static void ThumbnailGenerateHelperTestPart2()
{
    Media::ThumbRdbOpt opts = FuzzThumbRdbOpt(true);
    vector<Media::ThumbnailData> outDatas;
    Media::ThumbnailGenerateHelper::GetNoLcdData(opts, outDatas);
    outDatas.clear();
    Media::ThumbnailGenerateHelper::GetNoThumbnailData(opts, outDatas);
    outDatas.clear();
    Media::ThumbnailGenerateHelper::GetNoAstcData(opts, outDatas);
    outDatas.clear();
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

static void ThumbnailGenerateWorkerTest()
{
    Media::ThumbRdbOpt opts = FuzzThumbRdbOpt(false);
    Media::ThumbnailData thumbnailData = FuzzThumbnailData();
    std::shared_ptr<Media::ThumbnailTaskData> taskData =
        std::make_shared<Media::ThumbnailTaskData>(opts, thumbnailData, provider->ConsumeIntegral<int32_t>());
    std::shared_ptr<Media::ThumbnailGenerateTask> task =
        std::make_shared<Media::ThumbnailGenerateTask>(Media::IThumbnailHelper::CreateLcdAndThumbnail, taskData);

    std::shared_ptr<Media::ThumbnailGenerateWorker> workerPtr = std::make_shared<Media::ThumbnailGenerateWorker>();
    Media::ThumbnailTaskPriority priority = FuzzThumbnailTaskPriority();
    workerPtr->AddTask(task, priority);
    workerPtr->ReleaseTaskQueue(priority);
}

static void ThumbnailGenerateWorkerManagerTest()
{
    Media::ThumbnailTaskType type = FuzzThumbnailTaskType();
    auto& manager = Media::ThumbnailGenerateWorkerManager::GetInstance();
    manager.InitThumbnailWorker(type);
    manager.ClearAllTask();

    manager.InitThumbnailWorker(type);
    manager.TryCloseThumbnailWorkerTimer();
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
        std::make_shared<Media::ThumbnailTaskData>(opts, thumbnailData, provider->ConsumeIntegral<int32_t>());
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
    vector<string> createTableSqlList = { Media::PhotoColumn::CREATE_PHOTO_TABLE };
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

static void Finish()
{
    Media::MediaLibraryKvStoreManager::GetInstance().CloseAllKvStore();
}

static void ThumhnailTest()
{
    Media::ThumbnailService::GetInstance()->GetThumbnailFd(provider->ConsumeBytesAsString(NUM_BYTES),
        provider->ConsumeIntegral<int32_t>());
    string thumUri = "file://media/Photo/1?operation=thumbnail&width=-1&height=-1";
    Media::ThumbnailService::GetInstance()->GetThumbnailFd(thumUri, provider->ConsumeIntegral<int32_t>());
    Media::ThumbnailService::GetInstance()->LcdAging();
    Media::ThumbnailService::GetInstance()->CreateThumbnailFileScaned(provider->ConsumeBytesAsString(NUM_BYTES),
        provider->ConsumeBytesAsString(NUM_BYTES), provider->ConsumeIntegral<int32_t>());
    NativeRdb::RdbPredicates rdbPredicate("Photos");
    Media::ThumbnailService::GetInstance()->CancelAstcBatchTask(provider->ConsumeIntegral<int32_t>());
    Media::ThumbnailService::GetInstance()->GenerateThumbnailBackground();
    Media::ThumbnailService::GetInstance()->UpgradeThumbnailBackground(false);
    Media::ThumbnailService::GetInstance()->RestoreThumbnailDualFrame();
    Media::ThumbnailService::GetInstance()->CheckCloudThumbnailDownloadFinish();
    Media::ThumbnailService::GetInstance()->InterruptBgworker();
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
    FuzzedDataProvider fdp(data, size);
    OHOS::provider = &fdp;
    if (data == nullptr) {
        return 0;
    }
    OHOS::ThumhnailTest();
    OHOS::ThumbnailAgingHelperTest();
    OHOS::ThumbnailGenerateHelperTestPart1();
    OHOS::ThumbnailGenerateHelperTestPart2();
    OHOS::ThumbnailGenerateWorkerTest();
    OHOS::ThumbnailGenerateWorkerManagerTest();
    OHOS::ThumbnailSourceTest();
    OHOS::ThumbnailSourceTest2();
    OHOS::ParseFileUriTest();
    OHOS::ThumbnailImageFrameworkUtilsTest();
    OHOS::ThumbnailFileUtilsTest();
    OHOS::ThumbnailGenerationPostProcessTest();
    OHOS::ThumbnailRestoreManagerTest();
    OHOS::ThumbnailRdbUtilsTest();
    OHOS::Finish();
    return 0;
}
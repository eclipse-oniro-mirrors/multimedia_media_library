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
#include "medialibrary_thumbnail2_fuzzer.h"
#include "medialibrary_rdbstore_utils_fuzzer.h"

#include <cstdint>
#include <string>
#include <pixel_map.h>
#include <fstream>
#include <fuzzer/FuzzedDataProvider.h>

#include "ability_context_impl.h"
#include "datashare_helper.h"
#include "ithumbnail_helper.h"
#include "media_upgrade.h"
#include "thumbnail_aging_helper.h"
#include "thumbnail_generate_helper.h"
#include "thumbnail_generate_worker.h"
#include "thumbnail_generate_worker_manager.h"
#include "thumbnail_service.h"

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
const int32_t MIN_TASK_TYPE = -1;
const int32_t MIN_TASK_PRIORITY = -1;
const int32_t MAX_TASK_TYPE = 1;
const int32_t MAX_TASK_PRIORITY = 2;
const int32_t MAX_MEDIA_TYPE = 14;
const int32_t MAX_BYTE_VALUE = 256;
const int32_t SEED_SIZE = 1024;
FuzzedDataProvider *provider = nullptr;

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

static void ThumbnailAgingHelperTest()
{
    Media::ThumbRdbOpt opt = FuzzThumbRdbOpt(false);
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

static void ThumbnailGenerateWorkerTest()
{
    Media::ThumbRdbOpt opts = FuzzThumbRdbOpt(false);
    Media::ThumbnailData thumbnailData = FuzzThumbnailData();
    std::shared_ptr<Media::ThumbnailTaskData> taskData =
        std::make_shared<Media::ThumbnailTaskData>(opts, thumbnailData);
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

    OHOS::ThumhnailTest();
    OHOS::ThumbnailAgingHelperTest();
    OHOS::ThumbnailGenerateHelperTestPart1();
    OHOS::ThumbnailGenerateWorkerTest();
    OHOS::ThumbnailGenerateWorkerManagerTest();
    return 0;
}
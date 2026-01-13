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

#include "medialibrarybatchdownloadresourcestaskdao_fuzzer.h"

#include <cstddef>
#include <sstream>
#include <string>
#include <fstream>
#include <fuzzer/FuzzedDataProvider.h>

#include "ability_context_impl.h"
#include "batch_download_resources_task_dao.h"
#include "media_column.h"
#include "media_log.h"
#include "medialibrary_command.h"
#include "medialibrary_errno.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unistore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_kvstore_manager.h"
#include "values_bucket.h"
#include "media_upgrade.h"

namespace OHOS {
namespace Media {
using namespace std;
static constexpr int32_t CASE_0 = 0;
static constexpr int32_t CASE_1 = 1;
static constexpr int32_t CASE_2 = 2;
static constexpr int32_t CASE_3 = 3;
static constexpr int32_t CASE_4 = 4;
static constexpr int32_t CASE_5 = 5;
static constexpr int32_t CASE_6 = 6;
static constexpr int32_t NUM_BYTES = 1;
static constexpr int32_t MIN_MEDIA_PERCENT = -1;
static constexpr int32_t MAX_MEDIA_PERCENT = 0;
static constexpr int32_t MIN_PHOTO_POSITION = 1;
static constexpr int32_t MAX_PHOTO_POSITION = 2;
static constexpr int32_t MIN_CLEAN_TYPE = 1;
static constexpr int32_t MAX_CLEAN_TYPE = 2;
static constexpr int32_t MAX_BATCH_DOWNLOAD_STATUS_TYPE = 6;
static constexpr int32_t MAX_BYTE_VALUE = 256;
static constexpr int32_t SEED_SIZE = 1024;
const std::string PhotoColumn::PHOTO_URI_PREFIX = "file://media/Photo/";
const string PHOTOS_TABLE = "Photos";
const string DOWNLOAD_RESOURCES_TABLE = "download_resources_task_records";
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;
FuzzedDataProvider *provider = nullptr;

static inline int32_t FuzzPhotoPosition()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(MIN_PHOTO_POSITION, MAX_PHOTO_POSITION);
    return static_cast<CloudFilePosition>(value);
}

static inline CleanType FuzzCleanType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(MIN_CLEAN_TYPE, MAX_CLEAN_TYPE);
    return static_cast<CleanType>(value);
}

static inline BatchDownloadStatusType FuzzBatchDownloadStatusType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_BATCH_DOWNLOAD_STATUS_TYPE);
    return static_cast<BatchDownloadStatusType>(value);
}

static int32_t InsertPhotoAsset()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("InsertPhotoAsset g_rdbStore is null");
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_POSITION, FuzzPhotoPosition());
    values.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(FuzzCleanType()));
    values.PutInt(MediaColumn::MEDIA_SIZE, provider->ConsumeIntegral<uint32_t>());
    values.PutString(PhotoColumn::MEDIA_FILE_PATH, provider->ConsumeBytesAsString(NUM_BYTES));
    int64_t fileId = 0;
    int32_t ret = g_rdbStore->Insert(fileId, PHOTOS_TABLE, values);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_ERR, "g_rdbStore failed to insert ret=%{public}d", ret);
    return static_cast<int32_t>(fileId);
}

static int32_t InsertDownloadResources()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("InsertDownloadResources g_rdbStore is null");
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS, static_cast<int32_t>(FuzzBatchDownloadStatusType()));
    if (provider->ConsumeBool()) {
        values.PutInt(DownloadResourcesColumn::MEDIA_PERCENT, MAX_MEDIA_PERCENT);
        } else {
        values.PutInt(DownloadResourcesColumn::MEDIA_PERCENT, MIN_MEDIA_PERCENT);
    }
    int64_t fileId = 0;
    int32_t ret = g_rdbStore->Insert(fileId, DOWNLOAD_RESOURCES_TABLE, values);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_ERR, "g_rdbStore failed to insert ret=%{public}d", ret);
    return static_cast<int32_t>(fileId);
}

static void FromUriToAllFileIdsTest()
{
    MEDIA_INFO_LOG("FromUriToAllFileIdsTest start");
    BatchDownloadResourcesTaskDao batchDownloadResourcesTaskDao;
    vector<std::string> uris;
    int32_t fileId = InsertPhotoAsset();
    string uri = PHOTO_URI_PREFIX + to_string(fileId) + "/" + "test.jpg";
    uris.emplace_back(uri);
    vector<std::string> fileIds;
    batchDownloadResourcesTaskDao.FromUriToAllFileIds(uris, fileIds);
    MEDIA_INFO_LOG("FromUriToAllFileIdsTest end");
}

static void QueryValidBatchDownloadPoFromPhotosTest()
{
    MEDIA_INFO_LOG("QueryValidBatchDownloadPoFromPhotosTest start");
    BatchDownloadResourcesTaskDao batchDownloadResourcesTaskDao;
    int32_t fileId = InsertPhotoAsset();
    MEDIA_INFO_LOG("fileId: %{public}d", fileId);
    vector<std::string> fileIds;
    fileIds.emplace_back(to_string(fileId));
    vector<DownloadResourcesTaskPo> downloadResourcesTasks;
    batchDownloadResourcesTaskDao.QueryValidBatchDownloadPoFromPhotos(fileIds, downloadResourcesTasks);
    MEDIA_INFO_LOG("QueryValidBatchDownloadPoFromPhotosTest end");
}

static void BatchInsertTest()
{
    MEDIA_INFO_LOG("BatchInsertTest start");
    BatchDownloadResourcesTaskDao batchDownloadResourcesTaskDao;
    int64_t insertCount = 0;
    std::string table = DOWNLOAD_RESOURCES_TABLE;
    NativeRdb::ValuesBucket value;
    value.PutInt(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS, static_cast<int32_t>(FuzzBatchDownloadStatusType()));
    vector<NativeRdb::ValuesBucket> initialBatchValues;
    initialBatchValues.emplace_back(value);
    batchDownloadResourcesTaskDao.BatchInsert(insertCount, table, initialBatchValues);
    MEDIA_INFO_LOG("BatchInsertTest end");
}

static void QueryPauseDownloadingStatusResourcesTest()
{
    MEDIA_INFO_LOG("QueryPauseDownloadingStatusResourcesTest start");
    BatchDownloadResourcesTaskDao batchDownloadResourcesTaskDao;
    int32_t fileId = InsertDownloadResources();
    MEDIA_INFO_LOG("fileId: %{public}d", fileId);
    vector<std::string> fileIds;
    fileIds.emplace_back(to_string(fileId));
    vector<std::string> fileIdsDownloading;
    vector<std::string> fileIdsNotInDownloading;
    batchDownloadResourcesTaskDao.QueryPauseDownloadingStatusResources(fileIds,
        fileIdsDownloading, fileIdsNotInDownloading);
    MEDIA_INFO_LOG("QueryPauseDownloadingStatusResourcesTest end");
}

static void UpdatePauseDownloadResourcesInfoTest()
{
    MEDIA_INFO_LOG("UpdatePauseDownloadResourcesInfoTest start");
    BatchDownloadResourcesTaskDao batchDownloadResourcesTaskDao;
    int32_t fileId = InsertDownloadResources();
    MEDIA_INFO_LOG("fileId: %{public}d", fileId);
    vector<std::string> fileIds;
    fileIds.emplace_back(to_string(fileId));
    batchDownloadResourcesTaskDao.UpdatePauseDownloadResourcesInfo(fileIds);
    batchDownloadResourcesTaskDao.UpdateAllPauseDownloadResourcesInfo();
    MEDIA_INFO_LOG("UpdatePauseDownloadResourcesInfoTest end");
}

static void UpdateResumeDownloadResourcesInfoTest()
{
    MEDIA_INFO_LOG("UpdateResumeDownloadResourcesInfoTest start");
    BatchDownloadResourcesTaskDao batchDownloadResourcesTaskDao;
    int32_t fileId = InsertDownloadResources();
    MEDIA_INFO_LOG("fileId: %{public}d", fileId);
    vector<std::string> fileIds;
    fileIds.emplace_back(to_string(fileId));
    batchDownloadResourcesTaskDao.UpdateResumeDownloadResourcesInfo(fileIds);

    InsertDownloadResources();
    batchDownloadResourcesTaskDao.UpdateResumeAllDownloadResourcesInfo();
    MEDIA_INFO_LOG("UpdateResumeDownloadResourcesInfoTest end");
}

static void QueryCancelDownloadingStatusResourcesTest()
{
    MEDIA_INFO_LOG("QueryCancelDownloadingStatusResourcesTest start");
    BatchDownloadResourcesTaskDao batchDownloadResourcesTaskDao;
    int32_t fileId = InsertDownloadResources();
    MEDIA_INFO_LOG("fileId: %{public}d", fileId);
    vector<std::string> fileIds;
    fileIds.emplace_back(to_string(fileId));
    vector<std::string> fileIdsDownloading;
    vector<std::string> fileIdsNotInDownloading;
    batchDownloadResourcesTaskDao.QueryCancelDownloadingStatusResources(fileIds,
        fileIdsDownloading, fileIdsNotInDownloading);
    MEDIA_INFO_LOG("QueryCancelDownloadingStatusResourcesTest end");
}

static void DeleteCancelStateDownloadResourcesTest()
{
    MEDIA_INFO_LOG("DeleteCancelStateDownloadResourcesTest start");
    BatchDownloadResourcesTaskDao batchDownloadResourcesTaskDao;
    int32_t fileId = InsertDownloadResources();
    MEDIA_INFO_LOG("fileId: %{public}d", fileId);
    vector<std::string> fileIds;
    fileIds.emplace_back(to_string(fileId));
    batchDownloadResourcesTaskDao.DeleteCancelStateDownloadResources(fileIds);

    batchDownloadResourcesTaskDao.DeleteAllDownloadResourcesInfo();
    MEDIA_INFO_LOG("DeleteCancelStateDownloadResourcesTest end");
}

static void ClassifyExistedDownloadTasksTest()
{
    MEDIA_INFO_LOG("ClassifyExistedDownloadTasksTest start");
    BatchDownloadResourcesTaskDao batchDownloadResourcesTaskDao;
    int32_t fileId = InsertDownloadResources();
    MEDIA_INFO_LOG("fileId: %{public}d", fileId);
    vector<std::string> allFileIds = { to_string(fileId)};
    vector<std::string> newIds;
    vector<std::string> existedIds = { to_string(provider->ConsumeIntegral<int32_t>()) };
    batchDownloadResourcesTaskDao.ClassifyExistedDownloadTasks(allFileIds, newIds, existedIds);
    MEDIA_INFO_LOG("ClassifyExistedDownloadTasksTest end");
}

static void ClassifyInvalidDownloadTasksTest()
{
    MEDIA_INFO_LOG("ClassifyInvalidDownloadTasksTest start");
    BatchDownloadResourcesTaskDao batchDownloadResourcesTaskDao;
    int32_t fileId = InsertPhotoAsset();
    MEDIA_INFO_LOG("fileId: %{public}d", fileId);
    vector<std::string> newIds = { to_string(fileId) };
    vector<std::string> invalidIds;
    batchDownloadResourcesTaskDao.ClassifyInvalidDownloadTasks(newIds, invalidIds);
    MEDIA_INFO_LOG("ClassifyInvalidDownloadTasksTest end");
}

static void HandleAddExistedDownloadTasksTest()
{
    MEDIA_INFO_LOG("HandleAddExistedDownloadTasksTest start");
    BatchDownloadResourcesTaskDao batchDownloadResourcesTaskDao;
    int32_t fileId = InsertDownloadResources();
    MEDIA_INFO_LOG("fileId: %{public}d", fileId);
    vector<std::string> fileIds = { to_string(fileId) };
    batchDownloadResourcesTaskDao.HandleAddExistedDownloadTasks(fileIds);
    MEDIA_INFO_LOG("HandleAddExistedDownloadTasksTest end");
}

static void QueryCloudMediaBatchDownloadResourcesStatusTest()
{
    MEDIA_INFO_LOG("QueryCloudMediaBatchDownloadResourcesStatusTest start");
    BatchDownloadResourcesTaskDao batchDownloadResourcesTaskDao;
    NativeRdb::RdbPredicates predicates(DownloadResourcesColumn::TABLE);
    predicates.EqualTo(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
        static_cast<int32_t>(FuzzBatchDownloadStatusType()));
    vector<DownloadResourcesTaskPo> downloadResourcesTasks;
    batchDownloadResourcesTaskDao.QueryCloudMediaBatchDownloadResourcesStatus(predicates, downloadResourcesTasks);
    MEDIA_INFO_LOG("QueryCloudMediaBatchDownloadResourcesStatusTest end");
}

static void QueryCloudMediaBatchDownloadResourcesCountTest()
{
    MEDIA_INFO_LOG("QueryCloudMediaBatchDownloadResourcesCountTest start");
    BatchDownloadResourcesTaskDao batchDownloadResourcesTaskDao;
    NativeRdb::RdbPredicates predicates(DownloadResourcesColumn::TABLE);
    predicates.EqualTo(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
        static_cast<int32_t>(FuzzBatchDownloadStatusType()));
    int32_t count = 0;
    batchDownloadResourcesTaskDao.QueryCloudMediaBatchDownloadResourcesCount(predicates, count);
    MEDIA_INFO_LOG("QueryCloudMediaBatchDownloadResourcesCountTest end");
}

static void BatchDownloadResourcesTaskDaoTest1()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, CASE_6);
    switch (value) {
        case CASE_0: {
            FromUriToAllFileIdsTest();
            break;
        }
        case CASE_1: {
            QueryValidBatchDownloadPoFromPhotosTest();
            break;
        }
        case CASE_2: {
            BatchInsertTest();
            break;
        }
        case CASE_3: {
            QueryPauseDownloadingStatusResourcesTest();
            break;
        }
        case CASE_4: {
            UpdatePauseDownloadResourcesInfoTest();
            break;
        }
        case CASE_5: {
            UpdateResumeDownloadResourcesInfoTest();
            break;
        }
        case CASE_6: {
            QueryCancelDownloadingStatusResourcesTest();
            break;
        }
        default:
            MEDIA_ERR_LOG("no case");
            break;
    }
}

static void BatchDownloadResourcesTaskDaoTest2()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, CASE_5);
    switch (value) {
        case CASE_0: {
            DeleteCancelStateDownloadResourcesTest();
            break;
        }
        case CASE_1: {
            ClassifyExistedDownloadTasksTest();
            break;
        }
        case CASE_2: {
            ClassifyInvalidDownloadTasksTest();
            break;
        }
        case CASE_3: {
            HandleAddExistedDownloadTasksTest();
            break;
        }
        case CASE_4: {
            QueryCloudMediaBatchDownloadResourcesStatusTest();
            break;
        }
        case CASE_5: {
            QueryCloudMediaBatchDownloadResourcesCountTest();
            break;
        }
        default:
            MEDIA_ERR_LOG("no case");
            break;
    }
}

void SetTables()
{
    vector<string> createTableSqlList = {
        Media::PhotoUpgrade::CREATE_PHOTO_TABLE,
        DownloadResourcesColumn::CREATE_TABLE
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
        return;
    }
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
} // namedpace Media
} // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::Media::AddSeed();
    OHOS::Media::Init();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }
    FuzzedDataProvider fdp(data, size);
    OHOS::Media::provider = &fdp;
    if (OHOS::Media::provider == nullptr) {
        return 0;
    }
    OHOS::Media::BatchDownloadResourcesTaskDaoTest1();
    OHOS::Media::BatchDownloadResourcesTaskDaoTest2();
    OHOS::Media::ClearKvStore();
    return 0;
}
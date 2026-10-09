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

#include "photocustomrestoreoperation_fuzzer.h"
#include "medialibrary_rdbstore_utils_fuzzer.h"

#include <cstdint>
#include <string>
#include <fstream>
#include <vector>
#include <fuzzer/FuzzedDataProvider.h>

#include "ability_context_impl.h"
#include "media_log.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_kvstore_manager.h"
#include "media_app_uri_permission_column.h"

namespace OHOS {
namespace Media {
using namespace std;
static const int32_t NUM_BYTES = 1;
static const int32_t MAX_MIME_TYPE = 1;
static const int32_t DEFAULT_INDEX  = 1;
static const int32_t MAX_SUB_TYPE = 6;
static const int32_t MAX_MEDIA_TYPE = 15;
static const int32_t MAX_BYTE_VALUE = 256;
static const int32_t SEED_SIZE = 1024;
FuzzedDataProvider *provider;
static shared_ptr<MediaLibraryRdbStore> g_rdbStore;

static inline int32_t FuzzNotifyType()
{
    return provider->ConsumeIntegralInRange<int32_t>(0, NOTIFY_TYPE_LIST.size() - DEFAULT_INDEX);
}

static inline MediaType FuzzMediaType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_MEDIA_TYPE);
    return static_cast<MediaType>(value);
}

static inline PhotoSubType FuzzPhotoSubType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_SUB_TYPE);
    return static_cast<PhotoSubType>(value);
}

static inline string FuzzMimeType()
{
    int32_t data = provider->ConsumeIntegralInRange<int32_t>(0, MAX_MIME_TYPE);
    return MIMETYPE_FUZZER_LISTS[data];
}

static inline int32_t FuzzUriType()
{
    return provider->ConsumeIntegralInRange<int32_t>(RESTORE_URI_TYPE_PHOTO, RESTORE_URI_TYPE_ALBUM);
}

static inline UniqueNumber FuzzUniqueNumber()
{
    UniqueNumber uniqueNumber = {
        .imageTotalNumber = provider->ConsumeIntegral<int32_t>(),
        .videoTotalNumber = provider->ConsumeIntegral<int32_t>(),
        .imageCurrentNumber = provider->ConsumeIntegral<int32_t>(),
        .videoCurrentNumber = provider->ConsumeIntegral<int32_t>()
    };
    return uniqueNumber;
}

static RestoreTaskInfo FuzzRestoreTaskInfo()
{
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.albumLpath = provider->ConsumeBytesAsString(NUM_BYTES);
    restoreTaskInfo.keyPath = provider->ConsumeBytesAsString(NUM_BYTES);
    restoreTaskInfo.isDeduplication = provider->ConsumeBool();
    restoreTaskInfo.hasPhotoCache = provider->ConsumeBool();
    restoreTaskInfo.bundleName = provider->ConsumeBytesAsString(NUM_BYTES);
    restoreTaskInfo.packageName = provider->ConsumeBytesAsString(NUM_BYTES);
    restoreTaskInfo.uriType = FuzzUriType();
    restoreTaskInfo.uri = provider->ConsumeBytesAsString(NUM_BYTES);
    restoreTaskInfo.totalNum = provider->ConsumeIntegral<int32_t>();
    restoreTaskInfo.firstFileUri = provider->ConsumeBytesAsString(NUM_BYTES);
    restoreTaskInfo.appId = provider->ConsumeBytesAsString(NUM_BYTES);
    restoreTaskInfo.albumId = 0;
    restoreTaskInfo.imageAlbumUri = provider->ConsumeBytesAsString(NUM_BYTES);
    restoreTaskInfo.videoAlbumUri = provider->ConsumeBytesAsString(NUM_BYTES);
    restoreTaskInfo.sourceDir = CUSTOM_RESTORE_DIR;
    restoreTaskInfo.imageAlbumId = provider->ConsumeIntegral<int32_t>();
    restoreTaskInfo.videoAlbumId = provider->ConsumeIntegral<int32_t>();
    return restoreTaskInfo;
}

static RestoreFileInfo FuzzFileInfo()
{
    RestoreFileInfo fileInfo;
    fileInfo.originFilePath = provider->ConsumeBytesAsString(NUM_BYTES);
    fileInfo.filePath = provider->ConsumeBytesAsString(NUM_BYTES);
    fileInfo.fileName = provider->ConsumeBytesAsString(NUM_BYTES);
    fileInfo.displayName = provider->ConsumeBytesAsString(NUM_BYTES);
    fileInfo.title = provider->ConsumeBytesAsString(NUM_BYTES);
    fileInfo.extension = provider->ConsumeBytesAsString(NUM_BYTES);
    fileInfo.mediaType = FuzzMediaType();
    fileInfo.size = provider->ConsumeIntegral<int32_t>();
    fileInfo.orientation = provider->ConsumeIntegral<int32_t>();
    fileInfo.isLivePhoto = provider->ConsumeBool();
    fileInfo.fileId = provider->ConsumeIntegral<int32_t>();
    fileInfo.mimeType = FuzzMimeType();
    fileInfo.subtype = static_cast<int32_t>(FuzzPhotoSubType());
    fileInfo.movingPhotoEffectMode = provider->ConsumeIntegral<int32_t>();
    fileInfo.frontCamera = provider->ConsumeBytesAsString(NUM_BYTES);
    fileInfo.shootingMode = provider->ConsumeBytesAsString(NUM_BYTES);
    return fileInfo;
}

static unordered_map<string, TimeInfo> FuzzTimeInfoMap(size_t mapSize)
{
    unordered_map<string, TimeInfo> timeInfoMap;
    for (size_t i = 0; i < mapSize; i++) {
        TimeInfo timeInfo;
        timeInfo.dateAdded = provider->ConsumeIntegral<int64_t>();
        timeInfo.dateTaken = provider->ConsumeIntegral<int64_t>();
        timeInfo.detailTime = provider->ConsumeBytesAsString(NUM_BYTES);
        std::string fileName = provider->ConsumeBytesAsString(NUM_BYTES);
        timeInfoMap.insert({fileName, timeInfo});
    }
    return timeInfoMap;
}

static void PhotoCustomRestoreOperationTest()
{
    MEDIA_INFO_LOG("PhotoCustomRestoreOperationTest enter");
    RestoreTaskInfo restoreTaskInfo = FuzzRestoreTaskInfo();
    PhotoCustomRestoreOperation &operation = PhotoCustomRestoreOperation::GetInstance();
    operation.isRunning_.store(provider->ConsumeBool());
    if (provider->ConsumeBool()) {
        operation.AddTask(restoreTaskInfo);
        if (provider->ConsumeBool()) {
            operation.CancelTask(restoreTaskInfo);
        }
    }

    operation.Start();
    operation.ReleaseCustomRestoreTask(restoreTaskInfo);
    operation.ReportCustomRestoreTask(restoreTaskInfo);

    vector<string> files = {provider->ConsumeBytesAsString(NUM_BYTES)};
    int32_t notifyType = FuzzNotifyType();
    unordered_map<string, TimeInfo> timeInfoMap = FuzzTimeInfoMap(files.size());
    operation.HandleBatchCustomRestore(timeInfoMap, restoreTaskInfo, notifyType, files);

    int32_t fileNum = provider->ConsumeIntegral<int32_t>();
    operation.ApplyEfficiencyQuota(fileNum);
    operation.InitRestoreTask(restoreTaskInfo, fileNum);

    UniqueNumber uniqueNumber = FuzzUniqueNumber();
    operation.HandleCustomRestore(timeInfoMap, restoreTaskInfo, files, provider->ConsumeBool(), uniqueNumber);

    RestoreFileInfo fileInfo = FuzzFileInfo();
    operation.UpdatePhotoAlbum(restoreTaskInfo, fileInfo);
    operation.GenerateCustomRestoreNotify(restoreTaskInfo, notifyType);

    int32_t errCode = provider->ConsumeBool() ? E_OK : provider->ConsumeIntegral<int32_t>();
    operation.SendNotifyMessage(restoreTaskInfo, notifyType, errCode, fileNum, uniqueNumber);

    vector<RestoreFileInfo> fileInfos = {fileInfo};
    operation.SetDestinationPath(fileInfos, uniqueNumber);

    std::string result = "";
    int32_t mediaType = FuzzMediaType();
    operation.GetAssetRootDir(mediaType, result);

    int32_t sameFileNum = provider->ConsumeIntegral<int32_t>();
    operation.QueryAlbumId(restoreTaskInfo);
    operation.InitPhotoCache(restoreTaskInfo);

    operation.UpdateUniqueNumber(uniqueNumber);
    operation.GetFileInfos(files, uniqueNumber);
    operation.RenameFiles(fileInfos);
    MEDIA_INFO_LOG("PhotoCustomRestoreOperationTest end");
}

void SetTables()
{
    vector<string> createTableSqlList = {
        AppUriPermissionColumn::CREATE_APP_URI_PERMISSION_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        CHECK_AND_RETURN_LOG(g_rdbStore != nullptr, "g_rdbStore is null");
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
        MEDIA_ERR_LOG("rdbStore is nullptr.");
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
        return Media::E_ERR;
    }
    file.write(seedData, SEED_SIZE);
    file.close();
    delete[] seedData;
    MEDIA_INFO_LOG("seedData has been successfully written to file filename:%{public}s", filename);
    return Media::E_OK;
}
} // namespace Media
} // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::Media::Init();
    OHOS::Media::AddSeed();
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    if (data == nullptr) {
        return 0;
    }
    OHOS::Media::provider = &fdp;
    OHOS::Media::PhotoCustomRestoreOperationTest();
    return 0;
}
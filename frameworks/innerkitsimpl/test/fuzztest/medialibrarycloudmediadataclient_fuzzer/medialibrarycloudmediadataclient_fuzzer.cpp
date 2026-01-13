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

#include "medialibrarycloudmediadataclient_fuzzer.h"

#include <cstdint>
#include <string>
#include <fstream>
#define private public
#include "cloud_media_data_client.h"
#undef private
#include "media_log.h"
#include <fuzzer/FuzzedDataProvider.h>
#include "userfile_manager_types.h"
#include "medialibrary_type_const.h"

#include "ability_context_impl.h"
#include "medialibrary_command.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_kvstore_manager.h"
#include "media_upgrade.h"

namespace OHOS {
using namespace std;
using namespace OHOS::Media;
using namespace OHOS::Media::CloudSync;
const int32_t NUM_BYTES = 1;
const int32_t MAX_BYTE_VALUE = 256;
const int32_t SEED_SIZE = 1024;
const string PHOTOS_TABLE = "Photos";
FuzzedDataProvider* provider;
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;

static inline ThmsType FuzzThmsType()
{
    int32_t value = provider->ConsumeIntegral<int32_t>();
    if (value >= static_cast<int32_t>(ThmsType::TYPE_THM) &&
        value <= static_cast<int32_t>(ThmsType::TYPE_ASTC)) {
        return static_cast<ThmsType>(value);
    }
    return ThmsType::TYPE_THM_AND_LCD;
}

static inline SyncStatusType FuzzSyncStatusType()
{
    int32_t value = provider->ConsumeIntegral<int32_t>();
    if (value >= static_cast<int32_t>(SyncStatusType::TYPE_BACKUP) &&
        value <= static_cast<int32_t>(SyncStatusType::TYPE_UPLOAD)) {
        return static_cast<SyncStatusType>(value);
    }
    return SyncStatusType::TYPE_UPLOAD;
}

static inline Media::DirtyTypes FuzzDirtyTypes()
{
    int32_t value = provider->ConsumeIntegral<int32_t>();
    if (value >= static_cast<int32_t>(Media::DirtyTypes::TYPE_SYNCED) &&
        value <= static_cast<int32_t>(Media::DirtyTypes::TYPE_COPY)) {
        return static_cast<Media::DirtyTypes>(value);
    }
    return Media::DirtyTypes::TYPE_COPY;
}

static inline Media::PhotoPositionType FuzzPhotoPositionType()
{
    int32_t value = provider->ConsumeIntegral<int32_t>();
    if (value >= static_cast<int32_t>(Media::PhotoPositionType::LOCAL) &&
        value <= static_cast<int32_t>(Media::PhotoPositionType::LOCAL_AND_CLOUD)) {
        return static_cast<Media::PhotoPositionType>(value);
    }
    return Media::PhotoPositionType::LOCAL_AND_CLOUD;
}

static int32_t InsertPhotoAsset(string &cloudId)
{
    if (g_rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutString(Media::PhotoColumn::PHOTO_CLOUD_ID, cloudId);
    int32_t dirtyTypes = static_cast<int32_t>(FuzzDirtyTypes());
    values.PutInt(Media::PhotoColumn::PHOTO_DIRTY, dirtyTypes);
    values.PutInt(Media::PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(FuzzPhotoPositionType()));
    int64_t fileId = 0;
    g_rdbStore->Insert(fileId, PHOTOS_TABLE, values);
    return static_cast<int32_t>(fileId);
}

static void UpdateDirtyFuzzer()
{
    CloudMediaDataClient cloudMediaDataClient(1, 1);
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    InsertPhotoAsset(cloudId);
    cloudMediaDataClient.UpdateDirty(cloudId, FuzzDirtyTypes());

    cloudMediaDataClient.dataHandler_ = nullptr;
    cloudMediaDataClient.UpdateDirty(cloudId, FuzzDirtyTypes());
}

static void UpdatePositionFuzzer()
{
    CloudMediaDataClient cloudMediaDataClient(1, 1);
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    InsertPhotoAsset(cloudId);
    PhotoPositionType position = FuzzPhotoPositionType();
    std::vector<std::string> cloudIds;
    cloudIds.push_back(cloudId);
    cloudMediaDataClient.UpdatePosition(cloudIds, static_cast<int32_t>(position));

    cloudMediaDataClient.dataHandler_ = nullptr;
    cloudMediaDataClient.UpdatePosition(cloudIds, static_cast<int32_t>(position));
}

static void UpdateSyncStatusFuzzer()
{
    CloudMediaDataClient cloudMediaDataClient(1, 1);
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    InsertPhotoAsset(cloudId);
    int32_t syncStatus = static_cast<int32_t>(FuzzSyncStatusType());
    cloudMediaDataClient.UpdateSyncStatus(cloudId, syncStatus);

    cloudMediaDataClient.dataHandler_ = nullptr;
    cloudMediaDataClient.UpdateSyncStatus(cloudId, syncStatus);
}

static void UpdateThmStatusFuzzer()
{
    CloudMediaDataClient cloudMediaDataClient(1, 1);
    std::string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    InsertPhotoAsset(cloudId);
    ThumbStatus thumbStatus = provider->ConsumeBool() ? ThumbStatus::DOWNLOADED : ThumbStatus::TO_DOWNLOAD;
    cloudMediaDataClient.UpdateThmStatus(cloudId, static_cast<int32_t>(thumbStatus));

    cloudMediaDataClient.dataHandler_ = nullptr;
    cloudMediaDataClient.UpdateThmStatus(cloudId, static_cast<int32_t>(thumbStatus));
}

static void GetAgingFileFuzzer()
{
    int64_t time = provider->ConsumeIntegral<int64_t>();
    int32_t mediaType = provider->ConsumeIntegral<int32_t>();
    int32_t sizeLimit = provider->ConsumeIntegral<int32_t>();
    int32_t offset = 0;
    vector<CloudMetaData> metaDataList;
    CloudMediaDataClient cloudMediaDataClient(1, 1);
    cloudMediaDataClient.GetAgingFile(time, mediaType, sizeLimit, offset, metaDataList);

    cloudMediaDataClient.dataHandler_ = nullptr;
    cloudMediaDataClient.GetAgingFile(time, mediaType, sizeLimit, offset, metaDataList);
}

static void GetActiveAgingFileFuzzer()
{
    int64_t time = provider->ConsumeIntegral<int64_t>();
    int32_t mediaType = provider->ConsumeIntegral<int32_t>();
    int32_t sizeLimit = provider->ConsumeIntegral<int32_t>();
    int32_t offset = 0;
    vector<CloudMetaData> metaDataList;
    CloudMediaDataClient cloudMediaDataClient(1, 1);
    cloudMediaDataClient.GetActiveAgingFile(time, mediaType, sizeLimit, offset, metaDataList);

    cloudMediaDataClient.dataHandler_ = nullptr;
    cloudMediaDataClient.GetActiveAgingFile(time, mediaType, sizeLimit, offset, metaDataList);
}

static void GetDownloadAssetFuzzer()
{
    CloudMediaDataClient cloudMediaDataClient(1, 1);
    vector<string> uris;
    string uri = provider->ConsumeBytesAsString(NUM_BYTES);
    uris.push_back(uri);
    vector<CloudMetaData> cloudMetaDataVec;
    cloudMediaDataClient.GetDownloadAsset(uris, cloudMetaDataVec);

    cloudMediaDataClient.dataHandler_ = nullptr;
    cloudMediaDataClient.GetDownloadAsset(uris, cloudMetaDataVec);
}

static void GetDownloadThmsByUriFuzzer()
{
    CloudMediaDataClient cloudMediaDataClient(1, 1);
    vector<string> uris;
    int32_t thmType = FuzzThmsType();
    vector<CloudMetaData> cloudMetaDataVec;
    string uri = provider->ConsumeBytesAsString(NUM_BYTES);
    uris.push_back(uri);

    cloudMediaDataClient.GetDownloadThmsByUri(uris, thmType, cloudMetaDataVec);

    cloudMediaDataClient.dataHandler_ = nullptr;
    cloudMediaDataClient.GetDownloadThmsByUri(uris, thmType, cloudMetaDataVec);
}

static void OnDownloadAssetFuzzer()
{
    string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    vector<string> cloudIds;
    cloudIds.push_back(cloudId);
    CloudMediaDataClient cloudMediaDataClient(1, 1);
    vector<MediaOperateResult> result;
    cloudMediaDataClient.OnDownloadAsset(cloudIds, result);

    cloudMediaDataClient.dataHandler_ = nullptr;
    cloudMediaDataClient.OnDownloadAsset(cloudIds, result);
}

static void GetDownloadThmsFuzzer()
{
    std::vector<CloudMetaData> cloudMetaDataVec;
    int32_t paramSize = provider->ConsumeIntegral<int32_t>();
    int32_t offset = 0;
    int32_t type = provider->ConsumeIntegral<int32_t>();
    CloudMediaDataClient cloudMediaDataClient(1, 1);
    DownloadThumPara param;
    param.size = paramSize;
    param.offset = offset;
    param.type = type;
    param.isDownloadDisplayFirst = provider->ConsumeBool();
    cloudMediaDataClient.GetDownloadThms(cloudMetaDataVec, param);

    cloudMediaDataClient.dataHandler_ = nullptr;
    cloudMediaDataClient.GetDownloadThms(cloudMetaDataVec, param);
}

static void OnDownloadThmsFuzzer()
{
    std::unordered_map<std::string, int32_t> cloudIdThmStatusMap = {
        { provider->ConsumeBytesAsString(NUM_BYTES), FuzzThmsType() },
    };
    CloudMediaDataClient cloudMediaDataClient(1, 1);
    int32_t failSize = provider->ConsumeIntegral<int32_t>();
    cloudMediaDataClient.OnDownloadThms(cloudIdThmStatusMap, failSize);

    cloudMediaDataClient.dataHandler_ = nullptr;
    cloudMediaDataClient.OnDownloadThms(cloudIdThmStatusMap, failSize);
}

static void GetVideoToCacheFuzzer()
{
    CloudMediaDataClient cloudMediaDataClient(1, 1);
    std::vector<CloudMetaData> cloudMetaDataList;
    int32_t sizeTest = provider->ConsumeIntegral<int32_t>();
    cloudMediaDataClient.GetVideoToCache(cloudMetaDataList, sizeTest);

    cloudMediaDataClient.dataHandler_ = nullptr;
    cloudMediaDataClient.GetVideoToCache(cloudMetaDataList, sizeTest);
}

static void GetDownloadThmNumFuzzer()
{
    CloudMediaDataClient cloudMediaDataClient(1, 1);
    std::vector<CloudMetaData> cloudMetaDataList;
    int32_t totalNum = 0;
    int32_t type = FuzzThmsType();
    cloudMediaDataClient.GetDownloadThmNum(totalNum, type);

    cloudMediaDataClient.dataHandler_ = nullptr;
    cloudMediaDataClient.GetDownloadThmNum(totalNum, type);
}

static void GetFilePosStatFuzzer()
{
    CloudMediaDataClient cloudMediaDataClient(1, 1);
    vector<uint64_t> filePosStat = { 0, 0 };
    cloudMediaDataClient.GetFilePosStat(filePosStat);
    uint64_t filePos = provider->ConsumeIntegral<uint64_t>();
    filePosStat.push_back(filePos);
    cloudMediaDataClient.GetFilePosStat(filePosStat);

    cloudMediaDataClient.dataHandler_ = nullptr;
    cloudMediaDataClient.GetFilePosStat(filePosStat);
}

static void GetCloudThmStatFuzzer()
{
    std::vector<uint64_t> cloudThmStat{ 0, 0, 0 };
    CloudMediaDataClient cloudMediaDataClient(1, 1);
    cloudMediaDataClient.GetCloudThmStat(cloudThmStat);
    uint64_t cloudThm = provider->ConsumeIntegral<uint64_t>();
    cloudThmStat.push_back(cloudThm);
    cloudMediaDataClient.GetCloudThmStat(cloudThmStat);

    cloudMediaDataClient.dataHandler_ = nullptr;
    cloudMediaDataClient.GetCloudThmStat(cloudThmStat);
}

static void GetDirtyTypeStatFuzzer()
{
    vector<uint64_t> dirtyTypeStat{ 0, 0, 0, 0 };
    CloudMediaDataClient cloudMediaDataClient(1, 1);
    cloudMediaDataClient.GetDirtyTypeStat(dirtyTypeStat);
    uint64_t dirtyType = provider->ConsumeIntegral<uint64_t>();
    dirtyTypeStat.push_back(dirtyType);
    cloudMediaDataClient.GetDirtyTypeStat(dirtyTypeStat);

    cloudMediaDataClient.dataHandler_ = nullptr;
    cloudMediaDataClient.GetDirtyTypeStat(dirtyTypeStat);
}

static void UpdateLocalFileDirtyFuzzer()
{
    MDKRecord mDKRecord;
    vector<MDKRecord> records;
    string id = provider->ConsumeBytesAsString(NUM_BYTES);
    mDKRecord.SetRecordId(id);
    CloudMediaDataClient cloudMediaDataClient(1, 1);
    cloudMediaDataClient.UpdateLocalFileDirty(records);

    cloudMediaDataClient.dataHandler_ = nullptr;
    cloudMediaDataClient.UpdateLocalFileDirty(records);
}

static void TraceIdFuzzer()
{
    string traceId = provider->ConsumeBytesAsString(NUM_BYTES);
    CloudMediaDataClient cloudMediaDataClient(1, 1);
    cloudMediaDataClient.SetTraceId(traceId);
    cloudMediaDataClient.GetTraceId();

    cloudMediaDataClient.dataHandler_ = nullptr;
    cloudMediaDataClient.SetTraceId(traceId);
    cloudMediaDataClient.GetTraceId();
}

static void MediaLibraryCloudMediaDataClientFuzzer()
{
    UpdateDirtyFuzzer();
    UpdatePositionFuzzer();
    UpdateSyncStatusFuzzer();
    UpdateThmStatusFuzzer();
    GetAgingFileFuzzer();
    GetActiveAgingFileFuzzer();
    GetDownloadAssetFuzzer();
    GetDownloadThmsByUriFuzzer();
    OnDownloadAssetFuzzer();
    GetDownloadThmsFuzzer();
    OnDownloadThmsFuzzer();
    GetVideoToCacheFuzzer();
    GetDownloadThmNumFuzzer();
    GetFilePosStatFuzzer();
    GetCloudThmStatFuzzer();
    GetDirtyTypeStatFuzzer();
    UpdateLocalFileDirtyFuzzer();
    TraceIdFuzzer();
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

static inline void ClearKvStore()
{
    Media::MediaLibraryKvStoreManager::GetInstance().CloseAllKvStore();
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
    FuzzedDataProvider fdp(data, size);
    if (data == nullptr) {
        return 0;
    }
    OHOS::provider = &fdp;
    OHOS::MediaLibraryCloudMediaDataClientFuzzer();
    OHOS::ClearKvStore();
    return 0;
}
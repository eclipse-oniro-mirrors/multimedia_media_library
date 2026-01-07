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
#include "medialibrary_util_fuzzer.h"

#include <cstdint>
#include <string>
#include <thread>
#include <fuzzer/FuzzedDataProvider.h>

#include "ability_context_impl.h"
#include "app_mgr_interface.h"
#include "cloud_sync_utils.h"
#include "cpu_utils.h"
#include "medialibrary_album_operations.h"
#include "medialibrary_analysis_album_operations.h"
#include "medialibrary_app_uri_permission_operations.h"
#include "medialibrary_appstate_observer.h"
#include "media_log.h"
#include "medialibrary_command.h"
#include "medialibrary_dir_operations.h"
#include "medialibrary_uripermission_operations.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_data_manager_utils.h"
#include "multistages_capture_dfx_first_visit.h"
#include "rdb_predicates.h"
#include "datashare_values_bucket.h"
#include "media_analysis_proxy.h"
#include "media_analysis_helper.h"
#include "background_cloud_file_processor.h"
#include "medialibrary_db_const.h"
#include "scanner_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "database_adapter.h"
#include "userfile_manager_types.h"
#include "medialibrary_operation.h"
#include "datashare_helper.h"
#include "medialibrary_kvstore_manager.h"

#define private public
#include "medialibrary_common_utils.h"
#include "permission_utils.h"
#include "photo_file_utils.h"
#undef private

namespace OHOS {
using namespace std;
const std::string ROOT_MEDIA_DIR = "/storage/cloud/files/";
const std::string DISPLAY_NAME = "IMG_20250306_202859.jpg";
const std::string FILE_HIDDEN = ".FileHidden/";
static const int32_t E_ERR = -1;
const int32_t NUM_BYTES = 1;
static const int32_t MIN_CPU_AFFINITY_TYPE = -1;
static const int32_t MAX_CPU_AFFINITY_TYPE = 11;
static const string PHOTOS_TABLE = "Photos";
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;
FuzzedDataProvider *FDP = nullptr;

static inline Uri FuzzUri()
{
    return Uri(FDP->ConsumeBytesAsString(NUM_BYTES));
}

static inline Media::CpuAffinityType FuzzCpuAffinityType()
{
    int32_t value = FDP->ConsumeIntegralInRange<int32_t>(MIN_CPU_AFFINITY_TYPE, MAX_CPU_AFFINITY_TYPE);
    return static_cast<Media::CpuAffinityType>(value);
}

static inline Media::MediaLibraryCommand FuzzMediaLibraryCmd()
{
    return Media::MediaLibraryCommand(FuzzUri());
}

static int32_t InsertAsset(string photoId)
{
    if (g_rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutString(Media::PhotoColumn::PHOTO_ID, photoId);
    values.PutString(Media::MediaColumn::MEDIA_FILE_PATH, FDP->ConsumeBytesAsString(NUM_BYTES));
    values.PutString(Media::PhotoColumn::PHOTO_LAST_VISIT_TIME, FDP->ConsumeBytesAsString(NUM_BYTES));
    int64_t fileId = 0;
    g_rdbStore->Insert(fileId, PHOTOS_TABLE, values);
    return static_cast<int32_t>(fileId);
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

static void CommandTest()
{
    NativeRdb::ValuesBucket value;
    int32_t operationObject1 = FDP->ConsumeIntegral<int32_t>();
    int32_t operationType1 = FDP->ConsumeIntegral<int32_t>();
    int32_t mediaLibraryApi1 = FDP->ConsumeIntegral<int32_t>();
    Media::MediaLibraryCommand cmd(static_cast<Media::OperationObject>(operationObject1),
        static_cast<Media::OperationType>(operationType1), static_cast<Media::MediaLibraryApi>(mediaLibraryApi1));
    cmd.SetTableName(FDP->ConsumeBytesAsString(NUM_BYTES));
    cmd.SetBundleName(FDP->ConsumeBytesAsString(NUM_BYTES));
    cmd.SetDeviceName(FDP->ConsumeBytesAsString(NUM_BYTES));
    cmd.SetResult(FDP->ConsumeBytesAsString(NUM_BYTES));
    int32_t operationObject2 = FDP->ConsumeIntegral<int32_t>();
    cmd.SetOprnObject(static_cast<Media::OperationObject>(operationObject2));
    cmd.GetOprnFileId();
    cmd.SetOprnAssetId(FDP->ConsumeBytesAsString(NUM_BYTES));
    DataShare::DataSharePredicates pred;
    cmd.SetDataSharePred(pred);
    cmd.SetValueBucket(value);
    Media::MediaLibraryCommand cmdValueBucket(FuzzUri(), value);
    int32_t operationObject3 = FDP->ConsumeIntegral<int32_t>();
    int32_t operationType2 = FDP->ConsumeIntegral<int32_t>();
    int32_t mediaLibraryApi2 = FDP->ConsumeIntegral<int32_t>();
    Media::MediaLibraryCommand cmdValueBucket2(static_cast<Media::OperationObject>(operationObject3),
        static_cast<Media::OperationType>(operationType2), value,
        static_cast<Media::MediaLibraryApi>(mediaLibraryApi2));
    int32_t operationObject4 = FDP->ConsumeIntegral<int32_t>();
    int32_t operationType3 = FDP->ConsumeIntegral<int32_t>();
    int32_t mediaLibraryApi3 = FDP->ConsumeIntegral<int32_t>();
    Media::MediaLibraryCommand cmdDevice(static_cast<Media::OperationObject>(operationObject4),
        static_cast<Media::OperationType>(operationType3), FDP->ConsumeBytesAsString(NUM_BYTES),
        static_cast<Media::MediaLibraryApi>(mediaLibraryApi3));
}

static void DirOperationTest()
{
    int32_t operationObject = FDP->ConsumeIntegral<int32_t>();
    int32_t operationType = FDP->ConsumeIntegral<int32_t>();
    int32_t mediaLibraryApi = FDP->ConsumeIntegral<int32_t>();
    Media::MediaLibraryCommand cmd(static_cast<Media::OperationObject>(operationObject),
        static_cast<Media::OperationType>(operationType), static_cast<Media::MediaLibraryApi>(mediaLibraryApi));
    Media::MediaLibraryDirOperations::HandleDirOperation(cmd);
    Media::MediaLibraryDirOperations::CreateDirOperation(cmd);
    Media::MediaLibraryDirOperations::TrashDirOperation(cmd);
}

static void UriPermissionTest()
{
    int32_t operationObject = FDP->ConsumeIntegral<int32_t>();
    int32_t operationType = FDP->ConsumeIntegral<int32_t>();
    int32_t mediaLibraryApi = FDP->ConsumeIntegral<int32_t>();
    Media::MediaLibraryCommand cmd(static_cast<Media::OperationObject>(operationObject),
        static_cast<Media::OperationType>(operationType), static_cast<Media::MediaLibraryApi>(mediaLibraryApi));
    NativeRdb::ValuesBucket rdbValueBucket;
    rdbValueBucket.Put(Media::PERMISSION_FILE_ID, FDP->ConsumeIntegral<int32_t>());
    rdbValueBucket.Put(Media::PERMISSION_BUNDLE_NAME, FDP->ConsumeBytesAsString(NUM_BYTES));
    rdbValueBucket.Put(Media::PERMISSION_MODE, "r");
    rdbValueBucket.Put(Media::PERMISSION_TABLE_TYPE, FDP->ConsumeBytesAsString(NUM_BYTES));
    cmd.SetValueBucket(rdbValueBucket);
    Media::UriPermissionOperations::HandleUriPermOperations(cmd);
    Media::UriPermissionOperations::HandleUriPermInsert(cmd);
    Media::UriPermissionOperations::InsertBundlePermission(FDP->ConsumeIntegral<int32_t>(),
        FDP->ConsumeBytesAsString(NUM_BYTES), FDP->ConsumeBytesAsString(NUM_BYTES),
        FDP->ConsumeBytesAsString(NUM_BYTES));
    Media::UriPermissionOperations::DeleteBundlePermission(FDP->ConsumeBytesAsString(NUM_BYTES),
        FDP->ConsumeBytesAsString(NUM_BYTES), FDP->ConsumeBytesAsString(NUM_BYTES));
    string mode = "r";
    Media::UriPermissionOperations::CheckUriPermission(FDP->ConsumeBytesAsString(NUM_BYTES), mode);

    Media::UriPermissionOperations::GetUriPermissionMode(FDP->ConsumeBytesAsString(NUM_BYTES),
        FDP->ConsumeBytesAsString(NUM_BYTES), FDP->ConsumeIntegral<int32_t>(), mode);
    Media::UriPermissionOperations::UpdateOperation(cmd);
    Media::UriPermissionOperations::InsertOperation(cmd);
    std::vector<NativeRdb::ValuesBucket> rdbValues;
    Media::UriPermissionOperations::BatchInsertOperation(cmd, rdbValues);
    Media::UriPermissionOperations::DeleteOperation(cmd);
    std::vector<DataShare::DataShareValuesBucket> sharedValues;
    DataShare::DataShareValuesBucket valueTest1;
    valueTest1.Put(Media::AppUriPermissionColumn::FILE_ID, "file_id");
    valueTest1.Put(Media::AppUriPermissionColumn::APP_ID, "appid");
    DataShare::DataShareValuesBucket valueTest2;
    valueTest2.Put(Media::AppUriPermissionColumn::FILE_ID, "file_id");
    valueTest2.Put(Media::AppUriPermissionColumn::APP_ID, "appid");
    sharedValues.push_back(valueTest1);
    sharedValues.push_back(valueTest2);
    Media::UriPermissionOperations::GrantUriPermission(cmd, sharedValues);
    Media::UriPermissionOperations::DeleteAllTemporaryAsync();
}

static void AnalysisTest()
{
    std::vector<std::string> columns;
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates pred;
    int32_t operationObject = FDP->ConsumeIntegral<int32_t>();
    int32_t operationType = FDP->ConsumeIntegral<int32_t>();
    int32_t mediaLibraryApi = FDP->ConsumeIntegral<int32_t>();
    Media::MediaLibraryCommand cmd(static_cast<Media::OperationObject>(operationObject),
        static_cast<Media::OperationType>(operationType), static_cast<Media::MediaLibraryApi>(mediaLibraryApi));
    cmd.SetTableName("Photos");
    Media::MergeAlbumInfo info1;
    info1.albumId = FDP->ConsumeIntegral<int32_t>();
    Media::MergeAlbumInfo info2;
    info2.albumId = FDP->ConsumeIntegral<int32_t>();
    std::vector<Media::MergeAlbumInfo> infos;
    infos.push_back(info1);
    infos.push_back(info2);
    Media::MediaLibraryAnalysisAlbumOperations::UpdateMergeGroupAlbumsInfo(infos);
    Media::MediaLibraryAnalysisAlbumOperations::HandleGroupPhotoAlbum(
        static_cast<Media::OperationType>(FDP->ConsumeIntegral<int32_t>()), values, pred);
    Media::MediaLibraryAnalysisAlbumOperations::QueryGroupPhotoAlbum(cmd, columns);
    Media::MediaLibraryAnalysisAlbumOperations::UpdateGroupPhotoAlbumById(FDP->ConsumeIntegral<int32_t>());
}

static void AppPermissionTest()
{
    std::vector<std::string> columns;
    NativeRdb::RdbPredicates rdbPred("Photos");
    DataShare::DataSharePredicates sharedPred;
    int32_t operationObject = FDP->ConsumeIntegral<int32_t>();
    int32_t operationType = FDP->ConsumeIntegral<int32_t>();
    int32_t mediaLibraryApi = FDP->ConsumeIntegral<int32_t>();
    Media::MediaLibraryCommand cmd(static_cast<Media::OperationObject>(operationObject),
        static_cast<Media::OperationType>(operationType), static_cast<Media::MediaLibraryApi>(mediaLibraryApi));
    Media::MediaLibraryAppUriPermissionOperations::HandleInsertOperation(cmd);
    std::vector<DataShare::DataShareValuesBucket> sharedValues;
    DataShare::DataShareValuesBucket values;
    values.Put(Media::AppUriPermissionColumn::FILE_ID, "file_id");
    values.Put(Media::AppUriPermissionColumn::APP_ID, "appid");
    sharedValues.push_back(values);
    Media::MediaLibraryAppUriPermissionOperations::BatchInsert(cmd, sharedValues);
    Media::MediaLibraryAppUriPermissionOperations::DeleteOperation(rdbPred);
    Media::MediaLibraryAppUriPermissionOperations::QueryOperation(sharedPred, columns);
}

static void AppStateTest()
{
    Media::MedialibraryAppStateObserverManager::GetInstance().SubscribeAppState();
    Media::MedialibraryAppStateObserverManager::GetInstance().UnSubscribeAppState();
}

static void MediaLibraryManagerTest()
{
    Media::MediaLibraryDataManagerUtils::IsNumber(FDP->ConsumeBytesAsString(NUM_BYTES));
    Media::MediaLibraryDataManagerUtils::GetOperationType(FDP->ConsumeBytesAsString(NUM_BYTES));
    Media::MediaLibraryDataManagerUtils::GetDisPlayNameFromPath(FDP->ConsumeBytesAsString(NUM_BYTES));
    std::vector<std::string> whereArgs;
    std::string str = FDP->ConsumeBytesAsString(NUM_BYTES);
    Media::MediaLibraryDataManagerUtils::ObtionCondition(str, whereArgs);
    Media::MediaLibraryDataManagerUtils::GetTypeUriByUri(str);
}

static void MultistageAdapterTest()
{
    Media::MediaLibraryCommand cmd = FuzzMediaLibraryCmd();
    Media::DatabaseAdapter::Update(cmd);
    MEDIA_INFO_LOG("MultistageAdapterTest");
}

static void MultistageTest()
{
    std::string photoId = FDP->ConsumeBytesAsString(NUM_BYTES);
    int32_t fileId = InsertAsset(photoId);
    MEDIA_INFO_LOG("fileId: %{public}d, photoId: %{public}s", fileId, photoId.c_str());
    Media::MultiStagesCaptureDfxFirstVisit::GetInstance().Report(photoId, fileId);
    MEDIA_INFO_LOG("MultistageTest");
}

static void ActiveAnalysisTest()
{
    std::vector<std::string> fileIds;
    fileIds.push_back("1");
    Media::MediaAnalysisHelper::StartMediaAnalysisServiceSync(
        static_cast<int32_t>(Media::MediaAnalysisProxy::ActivateServiceType::START_SERVICE_OCR), fileIds);
    Media::MediaAnalysisHelper::StartMediaAnalysisServiceAsync(
        static_cast<int32_t>(Media::MediaAnalysisProxy::ActivateServiceType::START_SERVICE_OCR), fileIds);
    Media::MediaAnalysisHelper::AsyncStartMediaAnalysisService(
        static_cast<int32_t>(Media::MediaAnalysisProxy::ActivateServiceType::START_SERVICE_OCR), fileIds);
    Media::MediaAnalysisHelper::StartPortraitCoverSelectionAsync(fileIds.at(0));
    (void)Media::MediaAnalysisHelper::ParseGeoInfo(fileIds, true);
}

#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
static void CloudDownloadTest()
{
    Media::BackgroundCloudFileProcessor::StartTimer();
    int sleepTime = 200;
    std::this_thread::sleep_for(std::chrono::microseconds(sleepTime));
    Media::BackgroundCloudFileProcessor::StopTimer();
}
#endif

static void CpuUtilsTest()
{
    Media::CpuUtils::SlowDown();
    Media::CpuAffinityType cpuAffinityType = FuzzCpuAffinityType();
    Media::CpuUtils::SetSelfThreadAffinity(cpuAffinityType);
    Media::CpuUtils::ResetSelfThreadAffinity();
    Media::CpuUtils::ResetCpu();
}

static void CommonUtilsTest()
{
    std::string str = FDP->ConsumeBytesAsString(NUM_BYTES);
    Media::MediaLibraryCommonUtils::CanConvertStrToInt32(str);
}

static void CloudSyncUtilsTest()
{
    Media::CloudSyncUtils::IsUnlimitedTrafficStatusOn();
    Media::CloudSyncUtils::IsCloudSyncSwitchOn();
    Media::CloudSyncUtils::IsCloudDataAgingPolicyOn();
}

static void ScannerUtilsTest()
{
    std::string pathOrDisplayName = FDP->ConsumeBool() ? DISPLAY_NAME : FDP->ConsumeBytesAsString(NUM_BYTES);
    Media::ScannerUtils::GetFileExtension(pathOrDisplayName);

    std::string path = FDP->ConsumeBool() ? "" : FDP->ConsumeBytesAsString(NUM_BYTES);
    Media::ScannerUtils::IsDirectory(path);
    Media::ScannerUtils::IsRegularFile(path);

    path = FDP->ConsumeBool() ? FILE_HIDDEN : FDP->ConsumeBytesAsString(NUM_BYTES);
    Media::ScannerUtils::IsFileHidden(path);

    std::string dir = FDP->ConsumeBytesAsString(NUM_BYTES);
    Media::ScannerUtils::GetRootMediaDir(dir);

    std::string displayName = FDP->ConsumeBytesAsString(NUM_BYTES);
    Media::ScannerUtils::GetFileTitle(displayName);

    path = FDP->ConsumeBool() ? FILE_HIDDEN : FDP->ConsumeBytesAsString(NUM_BYTES);
    Media::ScannerUtils::IsDirHidden(path, true);

    path = FDP->ConsumeBool() ? ROOT_MEDIA_DIR + "Pictures": FDP->ConsumeBytesAsString(NUM_BYTES);
    Media::ScannerUtils::CheckSkipScanList(path);
}

static inline void ClearKvStore()
{
    Media::MediaLibraryKvStoreManager::GetInstance().CloseAllKvStore();
}
} // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::Init();
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    int sleepTime = 100;
    std::this_thread::sleep_for(std::chrono::microseconds(sleepTime));
    FuzzedDataProvider fdp(data, size);
    OHOS::FDP = &fdp;
    if (data == nullptr) {
        return 0;
    }
    OHOS::CommandTest();
    OHOS::DirOperationTest();
    OHOS::UriPermissionTest();
    OHOS::AnalysisTest();
    OHOS::AppPermissionTest();
    OHOS::AppStateTest();
    OHOS::MediaLibraryManagerTest();
    OHOS::MultistageAdapterTest();
    OHOS::MultistageTest();
    OHOS::ActiveAnalysisTest();
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    OHOS::CloudDownloadTest();
#endif
    OHOS::CpuUtilsTest();
    OHOS::CommonUtilsTest();
    OHOS::CloudSyncUtilsTest();
    OHOS::ScannerUtilsTest();
    OHOS::ClearKvStore();
    return 0;
}

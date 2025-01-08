/*
 * Copyright (C) 2021-2024 Huawei Device Co., Ltd.
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
 
#define MLOG_TAG "DataManager"

#include "medialibrary_data_manager.h"

#include <cstdlib>
#include <future>
#include <shared_mutex>
#include <unordered_set>
#include <sstream>

#include "ability_scheduler_interface.h"
#include "abs_rdb_predicates.h"
#include "acl.h"
#include "background_cloud_file_processor.h"
#include "background_task_mgr_helper.h"
#include "cloud_media_asset_manager.h"
#include "cloud_sync_switch_observer.h"
#include "datashare_abs_result_set.h"
#ifdef DISTRIBUTED
#include "device_manager.h"
#include "device_manager_callback.h"
#endif
#include "dfx_manager.h"
#include "dfx_reporter.h"
#include "dfx_utils.h"
#include "directory_ex.h"
#include "efficiency_resource_info.h"
#include "hitrace_meter.h"
#include "ipc_skeleton.h"
#include "location_column.h"
#include "media_analysis_helper.h"
#include "media_column.h"
#include "media_datashare_ext_ability.h"
#include "media_directory_type_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_old_photos_column.h"
#include "media_scanner_manager.h"
#include "media_smart_album_column.h"
#include "media_smart_map_column.h"
#include "medialibrary_album_operations.h"
#include "medialibrary_analysis_album_operations.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_app_uri_permission_operations.h"
#include "medialibrary_app_uri_sensitive_operations.h"
#include "medialibrary_async_worker.h"
#include "medialibrary_audio_operations.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_common_utils.h"
#ifdef DISTRIBUTED
#include "medialibrary_device.h"
#include "medialibrary_device_info.h"
#endif
#include "medialibrary_dir_operations.h"
#include "medialibrary_errno.h"
#include "medialibrary_file_operations.h"
#include "medialibrary_inotify.h"
#include "medialibrary_kvstore_manager.h"
#include "medialibrary_location_operations.h"
#include "medialibrary_meta_recovery.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_restore.h"
#include "medialibrary_smartalbum_map_operations.h"
#include "medialibrary_smartalbum_operations.h"
#include "medialibrary_story_operations.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_sync_operation.h"
#include "medialibrary_tab_old_photos_operations.h"
#include "medialibrary_tracer.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_uripermission_operations.h"
#include "medialibrary_urisensitive_operations.h"
#include "medialibrary_vision_operations.h"
#include "medialibrary_search_operations.h"
#include "mimetype_utils.h"
#include "multistages_capture_manager.h"
#include "enhancement_manager.h"
#include "permission_utils.h"
#include "photo_album_column.h"
#include "photo_day_month_year_operation.h"
#include "photo_map_operations.h"
#include "resource_type.h"
#include "rdb_store.h"
#include "rdb_utils.h"
#include "result_set_utils.h"
#include "system_ability_definition.h"
#include "timer.h"
#include "trash_async_worker.h"
#include "value_object.h"
#include "post_event_utils.h"
#include "medialibrary_formmap_operations.h"
#include "ithumbnail_helper.h"
#include "vision_face_tag_column.h"
#include "vision_photo_map_column.h"
#include "parameter.h"
#include "parameters.h"
#include "uuid.h"
#ifdef DEVICE_STANDBY_ENABLE
#include "medialibrary_standby_service_subscriber.h"
#endif
#ifdef HAS_THERMAL_MANAGER_PART
#include "thermal_mgr_client.h"
#endif
#include "zip_util.h"

using namespace std;
using namespace OHOS::AppExecFwk;
using namespace OHOS::AbilityRuntime;
using namespace OHOS::NativeRdb;
using namespace OHOS::DistributedKv;
using namespace OHOS::DataShare;
using namespace OHOS::RdbDataShareAdapter;

namespace {
const OHOS::DistributedKv::AppId KVSTORE_APPID = {"com.ohos.medialibrary.medialibrarydata"};
const OHOS::DistributedKv::StoreId KVSTORE_STOREID = {"medialibrary_thumbnail"};
};

namespace OHOS {
namespace Media {
unique_ptr<MediaLibraryDataManager> MediaLibraryDataManager::instance_ = nullptr;
unordered_map<string, DirAsset> MediaLibraryDataManager::dirQuerySetMap_ = {};
mutex MediaLibraryDataManager::mutex_;
static const int32_t UUID_STR_LENGTH = 37;
const int32_t PROPER_DEVICE_TEMPERATURE_LEVEL = 2;
const int32_t LARGE_FILE_SIZE_MB = 200;
const int32_t WRONG_VALUE = 0;
const int32_t BATCH_QUERY_NUMBER = 200;

#ifdef DEVICE_STANDBY_ENABLE
static const std::string SUBSCRIBER_NAME = "POWER_USAGE";
static const std::string MODULE_NAME = "com.ohos.medialibrary.medialibrarydata";
#endif
#ifdef DISTRIBUTED
static constexpr int MAX_QUERY_THUMBNAIL_KEY_COUNT = 20;
#endif
MediaLibraryDataManager::MediaLibraryDataManager(void)
{
}

MediaLibraryDataManager::~MediaLibraryDataManager(void)
{
#ifdef DISTRIBUTED
    if (kvStorePtr_ != nullptr) {
        dataManager_.CloseKvStore(KVSTORE_APPID, kvStorePtr_);
        kvStorePtr_ = nullptr;
    }
#endif
}

MediaLibraryDataManager* MediaLibraryDataManager::GetInstance()
{
    if (instance_ == nullptr) {
        lock_guard<mutex> lock(mutex_);
        if (instance_ == nullptr) {
            instance_ = make_unique<MediaLibraryDataManager>();
        }
    }
    return instance_.get();
}

static DataShare::DataShareExtAbility *MediaDataShareCreator(const unique_ptr<Runtime> &runtime)
{
    MEDIA_INFO_LOG("MediaLibraryCreator::%{public}s", __func__);
    return  MediaDataShareExtAbility::Create(runtime);
}

__attribute__((constructor)) void RegisterDataShareCreator()
{
    MEDIA_INFO_LOG("MediaLibraryDataManager::%{public}s", __func__);
    DataShare::DataShareExtAbility::SetCreator(MediaDataShareCreator);
    MEDIA_INFO_LOG("MediaLibraryDataManager::%{public}s End", __func__);
}

static void MakeRootDirs(AsyncTaskData *data)
{
    const unordered_set<string> DIR_CHECK_SET = { ROOT_MEDIA_DIR + BACKUP_DATA_DIR_VALUE,
        ROOT_MEDIA_DIR + BACKUP_SINGLE_DATA_DIR_VALUE };
    for (auto &dir : PRESET_ROOT_DIRS) {
        Uri createAlbumUri(MEDIALIBRARY_DATA_URI + "/" + MEDIA_ALBUMOPRN + "/" + MEDIA_ALBUMOPRN_CREATEALBUM);
        ValuesBucket valuesBucket;
        valuesBucket.PutString(MEDIA_DATA_DB_FILE_PATH, ROOT_MEDIA_DIR + dir);
        MediaLibraryCommand cmd(createAlbumUri, valuesBucket);
        auto ret = MediaLibraryAlbumOperations::CreateAlbumOperation(cmd);
        if (ret == E_FILE_EXIST) {
            MEDIA_INFO_LOG("Root dir: %{private}s is exist", dir.c_str());
        } else if (ret <= 0) {
            MEDIA_ERR_LOG("Failed to preset root dir: %{private}s", dir.c_str());
        }
        MediaFileUtils::CheckDirStatus(DIR_CHECK_SET, ROOT_MEDIA_DIR + dir);
    }
    MediaFileUtils::MediaFileDeletionRecord();
    // recover temp dir
    MediaFileUtils::RecoverMediaTempDir();
}

void MediaLibraryDataManager::ReCreateMediaDir()
{
    MediaFileUtils::BackupPhotoDir();
    // delete E policy dir
    for (const string &dir : E_POLICY_DIRS) {
        if (!MediaFileUtils::DeleteDir(dir)) {
            MEDIA_ERR_LOG("Delete dir fail, dir: %{public}s", DfxUtils::GetSafePath(dir).c_str());
        }
    }
    // create C policy dir
    InitACLPermission();
    shared_ptr<MediaLibraryAsyncWorker> asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    if (asyncWorker == nullptr) {
        MEDIA_ERR_LOG("Can not get asyncWorker");
        return;
    }
    AsyncTaskData* taskData = new (std::nothrow) AsyncTaskData();
    if (taskData == nullptr) {
        MEDIA_ERR_LOG("Failed to new taskData");
        return;
    }
    shared_ptr<MediaLibraryAsyncTask> makeRootDirTask = make_shared<MediaLibraryAsyncTask>(MakeRootDirs, taskData);
    if (makeRootDirTask != nullptr) {
        asyncWorker->AddTask(makeRootDirTask, true);
    } else {
        MEDIA_WARN_LOG("Can not init make root dir task");
    }
}

static int32_t ReconstructMediaLibraryPhotoMap()
{
    if (system::GetParameter("persist.multimedia.medialibrary.albumFusion.status", "1") == "1") {
        return E_OK;
    }
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Failed to get rdbstore, try again!");
        rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_DB_FAIL,
            "Fatal error! Failed to get rdbstore, new cloud data is not processed!!");
    }
    MediaLibraryRdbStore::ReconstructMediaLibraryStorageFormat(rdbStore);
    return E_OK;
}

void MediaLibraryDataManager::HandleOtherInitOperations()
{
    InitRefreshAlbum();
    UriPermissionOperations::DeleteAllTemporaryAsync();
    UriSensitiveOperations::DeleteAllSensitiveAsync();
}

static int32_t ExcuteAsyncWork()
{
    shared_ptr<MediaLibraryAsyncWorker> asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    if (asyncWorker == nullptr) {
        MEDIA_ERR_LOG("Can not get asyncWorker");
        return E_ERR;
    }
    AsyncTaskData* taskData = new (std::nothrow) AsyncTaskData();
    if (taskData == nullptr) {
        MEDIA_ERR_LOG("Failed to new taskData");
        return E_ERR;
    }
    taskData->dataDisplay = E_POLICY;
    shared_ptr<MediaLibraryAsyncTask> makeRootDirTask = make_shared<MediaLibraryAsyncTask>(MakeRootDirs, taskData);
    if (makeRootDirTask != nullptr) {
        asyncWorker->AddTask(makeRootDirTask, true);
    } else {
        MEDIA_WARN_LOG("Can not init make root dir task");
    }
    return E_OK;
}

__attribute__((no_sanitize("cfi"))) int32_t MediaLibraryDataManager::InitMediaLibraryMgr(
    const shared_ptr<OHOS::AbilityRuntime::Context> &context,
    const shared_ptr<OHOS::AbilityRuntime::Context> &extensionContext, int32_t &sceneCode, bool isNeedCreateDir)
{
    lock_guard<shared_mutex> lock(mgrSharedMutex_);

    if (refCnt_.load() > 0) {
        MEDIA_DEBUG_LOG("already initialized");
        refCnt_++;
        return E_OK;
    }

    InitResourceInfo();
    context_ = context;
    int32_t errCode = InitMediaLibraryRdbStore();
    if (errCode != E_OK) {
        sceneCode = DfxType::START_RDB_STORE_FAIL;
        return errCode;
    }
    if (!MediaLibraryKvStoreManager::GetInstance().InitMonthAndYearKvStore(KvStoreRoleType::OWNER)) {
        MEDIA_ERR_LOG("failed at InitMonthAndYearKvStore");
    }
#ifdef DISTRIBUTED
    errCode = InitDeviceData();
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "failed at InitDeviceData");
#endif
    MimeTypeUtils::InitMimeTypeMap();
    errCode = MakeDirQuerySetMap(dirQuerySetMap_);
    CHECK_AND_WARN_LOG(errCode == E_OK, "failed at MakeDirQuerySetMap");
    InitACLPermission();
    InitDatabaseACLPermission();
    if (isNeedCreateDir) {
        errCode = ExcuteAsyncWork();
        CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "failed at ExcuteAsyncWork");
    }
    errCode = InitialiseThumbnailService(extensionContext);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "failed at InitialiseThumbnailService");
    ReconstructMediaLibraryPhotoMap();
    HandleOtherInitOperations();

    auto shareHelper = MediaLibraryHelperContainer::GetInstance()->GetDataShareHelper();
    cloudPhotoObserver_ = std::make_shared<CloudSyncObserver>();
    cloudPhotoAlbumObserver_ = std::make_shared<CloudSyncObserver>();
    shareHelper->RegisterObserverExt(Uri(PhotoColumn::PHOTO_CLOUD_URI_PREFIX), cloudPhotoObserver_, true);
    shareHelper->RegisterObserverExt(Uri(PhotoAlbumColumns::ALBUM_CLOUD_URI_PREFIX), cloudPhotoAlbumObserver_, true);
    HandleUpgradeRdbAsync();
    CloudSyncSwitchManager cloudSyncSwitchManager;
    cloudSyncSwitchManager.RegisterObserver();
    SubscriberPowerConsumptionDetection();

    refCnt_++;

#ifdef META_RECOVERY_SUPPORT
    // TEMP: avoid Process backup call StartAsyncRecovery
    // Should remove this judgment at refactor in OpenHarmony5.1
    if (extensionContext != nullptr) {
        MediaLibraryMetaRecovery::GetInstance().StartAsyncRecovery();
    }
#endif

    return E_OK;
}

void HandleUpgradeRdbAsyncExtension(const shared_ptr<MediaLibraryRdbStore> rdbStore, int32_t oldVersion)
{
    if (oldVersion < VERSION_ADD_READY_COUNT_INDEX) {
        MediaLibraryRdbStore::AddReadyCountIndex(rdbStore);
        rdbStore->SetOldVersion(VERSION_ADD_READY_COUNT_INDEX);
    }

    if (oldVersion < VERSION_FIX_PICTURE_LCD_SIZE) {
        MediaLibraryRdbStore::UpdateLcdStatusNotUploaded(rdbStore);
        rdbStore->SetOldVersion(VERSION_FIX_PICTURE_LCD_SIZE);
    }

    if (oldVersion < VERSION_REVERT_FIX_DATE_ADDED_INDEX) {
        MediaLibraryRdbStore::RevertFixDateAddedIndex(rdbStore);
        rdbStore->SetOldVersion(VERSION_REVERT_FIX_DATE_ADDED_INDEX);
    }

    if (oldVersion < VERSION_ADD_CLOUD_ENHANCEMENT_ALBUM_INDEX) {
        MediaLibraryRdbStore::AddCloudEnhancementAlbumIndex(rdbStore);
        rdbStore->SetOldVersion(VERSION_ADD_CLOUD_ENHANCEMENT_ALBUM_INDEX);
    }

    if (oldVersion < VERSION_ADD_PHOTO_DATEADD_INDEX) {
        MediaLibraryRdbStore::AddPhotoDateAddedIndex(rdbStore);
        rdbStore->SetOldVersion(VERSION_ADD_PHOTO_DATEADD_INDEX);
    }

    if (oldVersion < VERSION_ADD_ALBUM_INDEX) {
        MediaLibraryRdbStore::AddAlbumIndex(rdbStore);
        rdbStore->SetOldVersion(VERSION_ADD_ALBUM_INDEX);
    }

    if (oldVersion < VERSION_REFRESH_PERMISSION_APPID) {
        MediaLibraryRdbUtils::TrasformAppId2TokenId(rdbStore);
        rdbStore->SetOldVersion(VERSION_REFRESH_PERMISSION_APPID);
    }

    if (oldVersion < VERSION_UPDATE_PHOTOS_DATE_AND_IDX) {
        PhotoDayMonthYearOperation::UpdatePhotosDateAndIdx(rdbStore);
        rdbStore->SetOldVersion(VERSION_UPDATE_PHOTOS_DATE_AND_IDX);
    }
}

void MediaLibraryDataManager::HandleUpgradeRdbAsync()
{
    std::thread([&] {
        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        if (rdbStore == nullptr) {
            MEDIA_ERR_LOG("rdbStore is nullptr!");
            return;
        }
        int32_t oldVersion = rdbStore->GetOldVersion();
        if (oldVersion == -1 || oldVersion >= MEDIA_RDB_VERSION) {
            MEDIA_INFO_LOG("No need to upgrade rdb, oldVersion: %{public}d", oldVersion);
            return;
        }
        MEDIA_INFO_LOG("oldVersion:%{public}d", oldVersion);
        // compare older version, update and set old version
        if (oldVersion < VERSION_CREATE_BURSTKEY_INDEX) {
            MediaLibraryRdbStore::CreateBurstIndex(rdbStore);
            rdbStore->SetOldVersion(VERSION_CREATE_BURSTKEY_INDEX);
        }

        if (oldVersion < VERSION_UPDATE_BURST_DIRTY) {
            MediaLibraryRdbStore::UpdateBurstDirty(rdbStore);
            rdbStore->SetOldVersion(VERSION_UPDATE_BURST_DIRTY);
        }

        if (oldVersion < VERSION_UPGRADE_THUMBNAIL) {
            MediaLibraryRdbStore::UpdateReadyOnThumbnailUpgrade(rdbStore);
            rdbStore->SetOldVersion(VERSION_UPGRADE_THUMBNAIL);
        }
        if (oldVersion < VERSION_ADD_DETAIL_TIME) {
            MediaLibraryRdbStore::UpdateDateTakenToMillionSecond(rdbStore);
            MediaLibraryRdbStore::UpdateDateTakenIndex(rdbStore);
            ThumbnailService::GetInstance()->AstcChangeKeyFromDateAddedToDateTaken();
            rdbStore->SetOldVersion(VERSION_ADD_DETAIL_TIME);
        }
        if (oldVersion < VERSION_MOVE_AUDIOS) {
            MediaLibraryAudioOperations::MoveToMusic();
            MediaLibraryRdbStore::ClearAudios(rdbStore);
            rdbStore->SetOldVersion(VERSION_MOVE_AUDIOS);
        }
        if (oldVersion < VERSION_UPDATE_INDEX_FOR_COVER) {
            MediaLibraryRdbStore::UpdateIndexForCover(rdbStore);
            rdbStore->SetOldVersion(VERSION_UPDATE_INDEX_FOR_COVER);
        }
        if (oldVersion < VERSION_UPDATE_DATETAKEN_AND_DETAILTIME) {
            MediaLibraryRdbStore::UpdateDateTakenAndDetalTime(rdbStore);
            rdbStore->SetOldVersion(VERSION_UPDATE_DATETAKEN_AND_DETAILTIME);
        }

        HandleUpgradeRdbAsyncExtension(rdbStore, oldVersion);
        // !! Do not add upgrade code here !!
        rdbStore->SetOldVersion(MEDIA_RDB_VERSION);
    }).detach();
}

void MediaLibraryDataManager::InitResourceInfo()
{
    BackgroundTaskMgr::EfficiencyResourceInfo resourceInfo =
        BackgroundTaskMgr::EfficiencyResourceInfo(BackgroundTaskMgr::ResourceType::CPU, true, 0, "apply", true, true);
    BackgroundTaskMgr::BackgroundTaskMgrHelper::ApplyEfficiencyResources(resourceInfo);
}

#ifdef DISTRIBUTED
int32_t MediaLibraryDataManager::InitDeviceData()
{
    if (rdbStore_ == nullptr) {
        MEDIA_ERR_LOG("MediaLibraryDataManager InitDeviceData rdbStore is null");
        return E_ERR;
    }

    MediaLibraryTracer tracer;
    tracer.Start("InitDeviceRdbStoreTrace");
    if (!MediaLibraryDevice::GetInstance()->InitDeviceRdbStore(rdbStore_)) {
        MEDIA_ERR_LOG("MediaLibraryDataManager InitDeviceData failed!");
        return E_ERR;
    }
    return E_OK;
}
#endif

__attribute__((no_sanitize("cfi"))) void MediaLibraryDataManager::ClearMediaLibraryMgr()
{
    lock_guard<shared_mutex> lock(mgrSharedMutex_);

    refCnt_--;
    if (refCnt_.load() > 0) {
        MEDIA_DEBUG_LOG("still other extension exist");
        return;
    }

    BackgroundCloudFileProcessor::StopTimer();

    auto shareHelper = MediaLibraryHelperContainer::GetInstance()->GetDataShareHelper();
    if (shareHelper == nullptr) {
        MEDIA_ERR_LOG("DataShareHelper is null");
        return;
    }
    shareHelper->UnregisterObserverExt(Uri(PhotoColumn::PHOTO_CLOUD_URI_PREFIX), cloudPhotoObserver_);
    shareHelper->UnregisterObserverExt(Uri(PhotoAlbumColumns::ALBUM_CLOUD_URI_PREFIX), cloudPhotoAlbumObserver_);
    rdbStore_ = nullptr;
    MediaLibraryKvStoreManager::GetInstance().CloseAllKvStore();
    MEDIA_INFO_LOG("CloseKvStore success");

#ifdef DISTRIBUTED
    if (kvStorePtr_ != nullptr) {
        dataManager_.CloseKvStore(KVSTORE_APPID, kvStorePtr_);
        kvStorePtr_ = nullptr;
    }

    if (MediaLibraryDevice::GetInstance()) {
        MediaLibraryDevice::GetInstance()->Stop();
    };
#endif

    if (thumbnailService_ != nullptr) {
        thumbnailService_->ReleaseService();
        thumbnailService_ = nullptr;
    }
    auto watch = MediaLibraryInotify::GetInstance();
    if (watch != nullptr) {
        watch->DoStop();
    }
    MediaLibraryUnistoreManager::GetInstance().Stop();
    extension_ = nullptr;
}

int32_t MediaLibraryDataManager::InitMediaLibraryRdbStore()
{
    if (rdbStore_) {
        return E_OK;
    }

    int32_t ret = MediaLibraryUnistoreManager::GetInstance().Init(context_);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("init MediaLibraryUnistoreManager failed");
        return ret;
    }
    rdbStore_ = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (ret != E_OK) {
        MEDIA_ERR_LOG("rdbStore is nullptr");
        return E_ERR;
    }

    return E_OK;
}

void MediaLibraryDataManager::InitRefreshAlbum()
{
    bool isNeedRefresh = false;
    int32_t ret = MediaLibraryRdbUtils::IsNeedRefreshByCheckTable(rdbStore_, isNeedRefresh);
    if (ret != E_OK || isNeedRefresh) {
        // Only set flag here, should not do any task in InitDataMgr
        MediaLibraryRdbUtils::SetNeedRefreshAlbum(true);
    }
}

shared_ptr<MediaDataShareExtAbility> MediaLibraryDataManager::GetOwner()
{
    return extension_;
}

void MediaLibraryDataManager::SetOwner(const shared_ptr<MediaDataShareExtAbility> &datashareExternsion)
{
    extension_ = datashareExternsion;
}

string MediaLibraryDataManager::GetType(const Uri &uri)
{
    MEDIA_DEBUG_LOG("MediaLibraryDataManager::GetType");
    MediaLibraryCommand cmd(uri);
    switch (cmd.GetOprnObject()) {
        case OperationObject::CLOUD_MEDIA_ASSET_OPERATE:
            return CloudMediaAssetManager::GetInstance().HandleCloudMediaAssetGetTypeOperations(cmd);
        default:
            break;
    }

    MEDIA_INFO_LOG("GetType uri: %{private}s", uri.ToString().c_str());
    return "";
}

int32_t MediaLibraryDataManager::MakeDirQuerySetMap(unordered_map<string, DirAsset> &outDirQuerySetMap)
{
    int32_t count = -1;
    vector<string> columns;
    AbsRdbPredicates dirAbsPred(MEDIATYPE_DIRECTORY_TABLE);
    if (rdbStore_ == nullptr) {
        MEDIA_ERR_LOG("rdbStore_ is nullptr");
        return E_ERR;
    }
    auto queryResultSet = rdbStore_->QueryByStep(dirAbsPred, columns);
    if (queryResultSet == nullptr) {
        MEDIA_ERR_LOG("queryResultSet is nullptr");
        return E_ERR;
    }
    auto ret = queryResultSet->GetRowCount(count);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdb failed");
        return E_ERR;
    }
    MEDIA_INFO_LOG("MakeDirQuerySetMap count = %{public}d", count);
    if (count == 0) {
        MEDIA_ERR_LOG("can not find any dirAsset");
        return E_ERR;
    }
    DirAsset dirAsset;
    string dirVal;
    outDirQuerySetMap.clear();
    while (queryResultSet->GoToNextRow() == NativeRdb::E_OK) {
        dirVal = get<string>(
            ResultSetUtils::GetValFromColumn(DIRECTORY_DB_DIRECTORY, queryResultSet, TYPE_STRING));
        dirAsset.SetDirectory(dirVal);
        dirAsset.SetDirType(get<int32_t>(
            ResultSetUtils::GetValFromColumn(DIRECTORY_DB_DIRECTORY_TYPE, queryResultSet, TYPE_INT32)));
        dirAsset.SetMediaTypes(get<string>(
            ResultSetUtils::GetValFromColumn(DIRECTORY_DB_MEDIA_TYPE, queryResultSet, TYPE_STRING)));
        dirAsset.SetExtensions(get<string>(
            ResultSetUtils::GetValFromColumn(DIRECTORY_DB_EXTENSION, queryResultSet, TYPE_STRING)));
        outDirQuerySetMap.insert(make_pair(dirVal, dirAsset));
    }
    return E_OK;
}

unordered_map<string, DirAsset> MediaLibraryDataManager::GetDirQuerySetMap()
{
    return dirQuerySetMap_;
}

#ifdef MEDIALIBRARY_COMPATIBILITY
static void ChangeUriFromValuesBucket(ValuesBucket &values)
{
    if (!values.HasColumn(MEDIA_DATA_DB_URI)) {
        return;
    }

    ValueObject value;
    if (!values.GetObject(MEDIA_DATA_DB_URI, value)) {
        return;
    }
    string oldUri;
    if (value.GetString(oldUri) != NativeRdb::E_OK) {
        return;
    }
    string newUri = MediaFileUtils::GetRealUriFromVirtualUri(oldUri);
    values.Delete(MEDIA_DATA_DB_URI);
    values.PutString(MEDIA_DATA_DB_URI, newUri);
}
#endif

int32_t MediaLibraryDataManager::SolveInsertCmd(MediaLibraryCommand &cmd)
{
    switch (cmd.GetOprnObject()) {
        case OperationObject::FILESYSTEM_ASSET:
            return MediaLibraryFileOperations::HandleFileOperation(cmd);

        case OperationObject::FILESYSTEM_PHOTO:
        case OperationObject::FILESYSTEM_AUDIO:
        case OperationObject::PTP_OPERATION:
            return MediaLibraryAssetOperations::HandleInsertOperation(cmd);

        case OperationObject::FILESYSTEM_ALBUM:
            return MediaLibraryAlbumOperations::CreateAlbumOperation(cmd);

        case OperationObject::ANALYSIS_PHOTO_ALBUM:
        case OperationObject::PHOTO_ALBUM:
            return MediaLibraryAlbumOperations::HandlePhotoAlbumOperations(cmd);

        case OperationObject::FILESYSTEM_DIR:
            return MediaLibraryDirOperations::HandleDirOperation(cmd);

        case OperationObject::SMART_ALBUM: {
            string packageName = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
            MEDIA_INFO_LOG("%{public}s call smart album insert!", packageName.c_str());
            return MediaLibrarySmartAlbumOperations::HandleSmartAlbumOperation(cmd);
        }
        case OperationObject::SMART_ALBUM_MAP:
            return MediaLibrarySmartAlbumMapOperations::HandleSmartAlbumMapOperation(cmd);

        case OperationObject::THUMBNAIL:
            return HandleThumbnailOperations(cmd);

        case OperationObject::BUNDLE_PERMISSION:
            return UriPermissionOperations::HandleUriPermOperations(cmd);
        case OperationObject::APP_URI_PERMISSION_INNER: {
            int32_t ret = UriSensitiveOperations::InsertOperation(cmd);
            CHECK_AND_RETURN_RET(ret >= 0, ret);
            return UriPermissionOperations::InsertOperation(cmd);
        }
        case OperationObject::MEDIA_APP_URI_PERMISSION: {
            int32_t ret = MediaLibraryAppUriSensitiveOperations::HandleInsertOperation(cmd);
            CHECK_AND_RETURN_RET(ret == MediaLibraryAppUriSensitiveOperations::SUCCEED, ret);
            return MediaLibraryAppUriPermissionOperations::HandleInsertOperation(cmd);
        }
        default:
            break;
    }
    return SolveInsertCmdSub(cmd);
}

int32_t MediaLibraryDataManager::SolveInsertCmdSub(MediaLibraryCommand &cmd)
{
    if (MediaLibraryRestore::GetInstance().IsRealBackuping()) {
        MEDIA_INFO_LOG("[SolveInsertCmdSub] rdb is backuping");
        return E_FAIL;
    }
    switch (cmd.GetOprnObject()) {
        case OperationObject::VISION_START ... OperationObject::VISION_END:
            return MediaLibraryVisionOperations::InsertOperation(cmd);

        case OperationObject::GEO_DICTIONARY:
        case OperationObject::GEO_KNOWLEDGE:
        case OperationObject::GEO_PHOTO:
            return MediaLibraryLocationOperations::InsertOperation(cmd);
        case OperationObject::PAH_FORM_MAP:
            return MediaLibraryFormMapOperations::HandleStoreFormIdOperation(cmd);
        case OperationObject::SEARCH_TOTAL: {
            return MediaLibrarySearchOperations::InsertOperation(cmd);
        }
        case OperationObject::STORY_ALBUM:
        case OperationObject::STORY_COVER:
        case OperationObject::STORY_PLAY:
        case OperationObject::USER_PHOTOGRAPHY:
        case OperationObject::ANALYSIS_ASSET_SD_MAP:
        case OperationObject::ANALYSIS_ALBUM_ASSET_MAP:
            return MediaLibraryStoryOperations::InsertOperation(cmd);
        case OperationObject::ANALYSIS_PHOTO_MAP: {
            return MediaLibrarySearchOperations::InsertOperation(cmd);
        }
        default:
            MEDIA_ERR_LOG("MediaLibraryDataManager SolveInsertCmd: unsupported OperationObject: %{public}d",
                cmd.GetOprnObject());
            break;
    }
    return E_FAIL;
}

static int32_t LogMovingPhoto(MediaLibraryCommand &cmd, const DataShareValuesBucket &dataShareValue)
{
    bool isValid = false;
    bool adapted = bool(dataShareValue.Get("adapted", isValid));
    if (!isValid) {
        MEDIA_ERR_LOG("Invalid adapted value");
        return E_ERR;
    }
    string packageName = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    CHECK_AND_WARN_LOG(!packageName.empty(), "Package name is empty, adapted: %{public}d", static_cast<int>(adapted));
    DfxManager::GetInstance()->HandleAdaptationToMovingPhoto(packageName, adapted);
    return E_OK;
}

static int32_t LogMedialibraryAPI(MediaLibraryCommand &cmd, const DataShareValuesBucket &dataShareValue)
{
    string packageName = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    bool isValid = false;
    string saveUri = string(dataShareValue.Get("saveUri", isValid));
    CHECK_AND_RETURN_RET_LOG(isValid, E_FAIL, "Invalid saveUri value");
    int32_t ret = DfxReporter::ReportMedialibraryAPI(packageName, saveUri);
    CHECK_AND_PRINT_LOG(ret == E_SUCCESS, "Log medialibrary API failed");
    return ret;
}

static int32_t SolveOtherInsertCmd(MediaLibraryCommand &cmd, const DataShareValuesBucket &dataShareValue,
    bool &solved)
{
    solved = false;
    switch (cmd.GetOprnObject()) {
        case OperationObject::MISCELLANEOUS: {
            if (cmd.GetOprnType() == OperationType::LOG_MOVING_PHOTO) {
                solved = true;
                return LogMovingPhoto(cmd, dataShareValue);
            }
            if (cmd.GetOprnType() == OperationType::LOG_MEDIALIBRARY_API) {
                solved = true;
                return LogMedialibraryAPI(cmd, dataShareValue);
            }
            return E_OK;
        }
        default:
            return E_FAIL;
    }
}

int32_t MediaLibraryDataManager::Insert(MediaLibraryCommand &cmd, const DataShareValuesBucket &dataShareValue)
{
    shared_lock<shared_mutex> sharedLock(mgrSharedMutex_);
    if (refCnt_.load() <= 0) {
        MEDIA_DEBUG_LOG("MediaLibraryDataManager is not initialized");
        return E_FAIL;
    }

    ValuesBucket value = RdbUtils::ToValuesBucket(dataShareValue);
    if (value.IsEmpty()) {
        MEDIA_ERR_LOG("MediaLibraryDataManager Insert: Input parameter is invalid");
        return E_INVALID_VALUES;
    }
#ifdef MEDIALIBRARY_COMPATIBILITY
    ChangeUriFromValuesBucket(value);
#endif
    cmd.SetValueBucket(value);

    OperationType oprnType = cmd.GetOprnType();
    if (oprnType == OperationType::CREATE || oprnType == OperationType::SUBMIT_CACHE
        || oprnType == OperationType::ADD_FILTERS) {
        if (SetCmdBundleAndDevice(cmd) != ERR_OK) {
            MEDIA_ERR_LOG("MediaLibraryDataManager SetCmdBundleAndDevice failed.");
        }
    }
    // boardcast operation
    if (oprnType == OperationType::SCAN) {
        return MediaScannerManager::GetInstance()->ScanDir(ROOT_MEDIA_DIR, nullptr);
    } else if (oprnType == OperationType::DELETE_TOOL) {
        return MediaLibraryAssetOperations::DeleteToolOperation(cmd);
    }

    bool solved = false;
    int32_t ret = SolveOtherInsertCmd(cmd, dataShareValue, solved);
    if (solved) {
        return ret;
    }
    return SolveInsertCmd(cmd);
}

int32_t MediaLibraryDataManager::InsertExt(MediaLibraryCommand &cmd, const DataShareValuesBucket &dataShareValue,
    string &result)
{
    int32_t ret = Insert(cmd, dataShareValue);
    result = cmd.GetResult();
    return ret;
}

int32_t MediaLibraryDataManager::HandleThumbnailOperations(MediaLibraryCommand &cmd)
{
    if (thumbnailService_ == nullptr) {
        return E_THUMBNAIL_SERVICE_NULLPTR;
    }
    int32_t result = E_FAIL;
    switch (cmd.GetOprnType()) {
        case OperationType::GENERATE:
            result = thumbnailService_->GenerateThumbnailBackground();
            break;
        case OperationType::AGING:
            result = thumbnailService_->LcdAging();
            break;
#ifdef DISTRIBUTED
        case OperationType::DISTRIBUTE_AGING:
            result = DistributeDeviceAging();
            break;
#endif
        default:
            MEDIA_ERR_LOG("bad operation type %{public}u", cmd.GetOprnType());
    }
    return result;
}

int32_t MediaLibraryDataManager::BatchInsert(MediaLibraryCommand &cmd, const vector<DataShareValuesBucket> &values)
{
    shared_lock<shared_mutex> sharedLock(mgrSharedMutex_);
    if (refCnt_.load() <= 0) {
        MEDIA_ERR_LOG("MediaLibraryDataManager is not initialized");
        return E_FAIL;
    }

    string uriString = cmd.GetUri().ToString();
    if (uriString == UFM_PHOTO_ALBUM_ADD_ASSET || uriString == PAH_PHOTO_ALBUM_ADD_ASSET) {
        return PhotoMapOperations::AddPhotoAssets(values);
    } else if (cmd.GetOprnObject() == OperationObject::ANALYSIS_PHOTO_MAP) {
        return PhotoMapOperations::AddAnaLysisPhotoAssets(values);
    } else if (cmd.GetOprnObject() == OperationObject::APP_URI_PERMISSION_INNER) {
        int32_t ret = UriSensitiveOperations::GrantUriSensitive(cmd, values);
        CHECK_AND_RETURN_RET(ret >= 0, ret);
        return UriPermissionOperations::GrantUriPermission(cmd, values);
    } else if (cmd.GetOprnObject() == OperationObject::MEDIA_APP_URI_PERMISSION) {
        int32_t ret = MediaLibraryAppUriSensitiveOperations::BatchInsert(cmd, values);
        CHECK_AND_RETURN_RET(ret == MediaLibraryAppUriSensitiveOperations::SUCCEED, ret);
        CHECK_AND_RETURN_RET(!MediaLibraryAppUriSensitiveOperations::BeForceSensitive(cmd, values), ret);
        return MediaLibraryAppUriPermissionOperations::BatchInsert(cmd, values);
    }
    if (uriString.find(MEDIALIBRARY_DATA_URI) == string::npos) {
        MEDIA_ERR_LOG("MediaLibraryDataManager BatchInsert: Input parameter is invalid");
        return E_INVALID_URI;
    }
    int32_t rowCount = 0;
    for (auto it = values.begin(); it != values.end(); it++) {
        if (Insert(cmd, *it) >= 0) {
            rowCount++;
        }
    }

    return rowCount;
}

int32_t MediaLibraryDataManager::Delete(MediaLibraryCommand &cmd, const DataSharePredicates &predicates)
{
    shared_lock<shared_mutex> sharedLock(mgrSharedMutex_);
    if (refCnt_.load() <= 0) {
        MEDIA_DEBUG_LOG("MediaLibraryDataManager is not initialized");
        return E_FAIL;
    }

    string uriString = cmd.GetUri().ToString();
    if (MediaFileUtils::StartsWith(uriString, PhotoColumn::PHOTO_CACHE_URI_PREFIX)) {
        return MediaLibraryAssetOperations::DeleteOperation(cmd);
    }

    if (uriString.find(MEDIALIBRARY_DATA_URI) == string::npos) {
        MEDIA_ERR_LOG("Not Data ability Uri");
        return E_INVALID_URI;
    }
    MediaLibraryTracer tracer;
    tracer.Start("CheckWhereClause");
    auto whereClause = predicates.GetWhereClause();
    if (!MediaLibraryCommonUtils::CheckWhereClause(whereClause)) {
        MEDIA_ERR_LOG("illegal query whereClause input %{private}s", whereClause.c_str());
        return E_SQL_CHECK_FAIL;
    }
    tracer.Finish();

    // MEDIALIBRARY_TABLE just for RdbPredicates
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicates,
        cmd.GetTableName());
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    return DeleteInRdbPredicates(cmd, rdbPredicate);
}

bool CheckIsDismissAsset(NativeRdb::RdbPredicates &rdbPredicate)
{
    auto whereClause = rdbPredicate.GetWhereClause();
    if (whereClause.find(MAP_ALBUM) != string::npos && whereClause.find(MAP_ASSET) != string::npos) {
        return true;
    }
    return false;
}

int32_t MediaLibraryDataManager::DeleteInRdbPredicates(MediaLibraryCommand &cmd, NativeRdb::RdbPredicates &rdbPredicate)
{
    switch (cmd.GetOprnObject()) {
        case OperationObject::FILESYSTEM_ASSET:
        case OperationObject::FILESYSTEM_DIR:
        case OperationObject::FILESYSTEM_ALBUM: {
            vector<string> columns = { MEDIA_DATA_DB_ID, MEDIA_DATA_DB_FILE_PATH, MEDIA_DATA_DB_PARENT_ID,
                MEDIA_DATA_DB_MEDIA_TYPE, MEDIA_DATA_DB_IS_TRASH, MEDIA_DATA_DB_RELATIVE_PATH };
            auto fileAsset = MediaLibraryObjectUtils::GetFileAssetByPredicates(*cmd.GetAbsRdbPredicates(), columns);
            CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_ARGUMENTS, "Get fileAsset failed.");
            if (fileAsset->GetRelativePath() == "") {
                return E_DELETE_DENIED;
            }
            return (fileAsset->GetMediaType() != MEDIA_TYPE_ALBUM) ?
                MediaLibraryObjectUtils::DeleteFileObj(move(fileAsset)) :
                MediaLibraryObjectUtils::DeleteDirObj(move(fileAsset));
        }
        case OperationObject::PHOTO_ALBUM: {
            return MediaLibraryAlbumOperations::DeletePhotoAlbum(rdbPredicate);
        }
        case OperationObject::HIGHLIGHT_DELETE: {
            return MediaLibraryAlbumOperations::DeleteHighlightAlbums(rdbPredicate);
        }
        case OperationObject::PHOTO_MAP: {
            return PhotoMapOperations::RemovePhotoAssets(rdbPredicate);
        }
        case OperationObject::ANALYSIS_PHOTO_MAP: {
            if (CheckIsDismissAsset(rdbPredicate)) {
                return PhotoMapOperations::DismissAssets(rdbPredicate);
            }
            break;
        }
        case OperationObject::MEDIA_APP_URI_PERMISSION:
        case OperationObject::APP_URI_PERMISSION_INNER: {
            return MediaLibraryAppUriPermissionOperations::DeleteOperation(rdbPredicate);
        }
        case OperationObject::FILESYSTEM_PHOTO:
        case OperationObject::FILESYSTEM_AUDIO: {
            return MediaLibraryAssetOperations::DeleteOperation(cmd);
        }
        case OperationObject::PAH_FORM_MAP: {
            return MediaLibraryFormMapOperations::RemoveFormIdOperations(rdbPredicate);
        }
        default:
            break;
    }

    return DeleteInRdbPredicatesAnalysis(cmd, rdbPredicate);
}

int32_t MediaLibraryDataManager::DeleteInRdbPredicatesAnalysis(MediaLibraryCommand &cmd,
    NativeRdb::RdbPredicates &rdbPredicate)
{
    if (MediaLibraryRestore::GetInstance().IsRealBackuping()) {
        MEDIA_INFO_LOG("[DeleteInRdbPredicatesAnalysis] rdb is backuping");
        return E_FAIL;
    }
    switch (cmd.GetOprnObject()) {
        case OperationObject::VISION_START ... OperationObject::VISION_END: {
            return MediaLibraryVisionOperations::DeleteOperation(cmd);
        }
        case OperationObject::GEO_DICTIONARY:
        case OperationObject::GEO_KNOWLEDGE:
        case OperationObject::GEO_PHOTO: {
            return MediaLibraryLocationOperations::DeleteOperation(cmd);
        }

        case OperationObject::STORY_ALBUM:
        case OperationObject::STORY_COVER:
        case OperationObject::STORY_PLAY:
        case OperationObject::USER_PHOTOGRAPHY: {
            return MediaLibraryStoryOperations::DeleteOperation(cmd);
        }
        case OperationObject::SEARCH_TOTAL: {
            return MediaLibrarySearchOperations::DeleteOperation(cmd);
        }
        default:
            break;
    }

    return E_FAIL;
}

int32_t MediaLibraryDataManager::Update(MediaLibraryCommand &cmd, const DataShareValuesBucket &dataShareValue,
    const DataSharePredicates &predicates)
{
    MEDIA_DEBUG_LOG("MediaLibraryDataManager::Update");
    shared_lock<shared_mutex> sharedLock(mgrSharedMutex_);
    if (refCnt_.load() <= 0) {
        MEDIA_DEBUG_LOG("MediaLibraryDataManager is not initialized");
        return E_FAIL;
    }

    ValuesBucket value = RdbUtils::ToValuesBucket(dataShareValue);
    if (value.IsEmpty()) {
        MEDIA_ERR_LOG("MediaLibraryDataManager Update:Input parameter is invalid ");
        return E_INVALID_VALUES;
    }

#ifdef MEDIALIBRARY_COMPATIBILITY
    ChangeUriFromValuesBucket(value);
#endif

    cmd.SetValueBucket(value);
    cmd.SetDataSharePred(predicates);
    // MEDIALIBRARY_TABLE just for RdbPredicates
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicates,
        cmd.GetTableName());
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());

    return UpdateInternal(cmd, value, predicates);
}

static std::vector<std::string> SplitUriString(const std::string& str, char delimiter)
{
    std::vector<std::string> elements;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        if (!item.empty()) {
            elements.emplace_back(item);
        }
    }
    return elements;
}

static std::string ExtractFileIdFromUri(const std::string& uri)
{
    auto uriParts = SplitUriString(uri, '/');
    if (uriParts.size() >= MediaLibraryDataManager::URI_MIN_NUM) {
        return uriParts[uriParts.size() - MediaLibraryDataManager::URI_MIN_NUM];
    }
    return "";
}

static std::string BuildWhereClause(const std::vector<std::string>& dismissAssetArray, int32_t albumId)
{
    std::string whereClause = MediaColumn::MEDIA_ID + " IN (";

    for (size_t i = 0; i < dismissAssetArray.size(); ++i) {
        std::string fileId = ExtractFileIdFromUri(dismissAssetArray[i]);
        if (fileId.empty()) {
            continue;
        }

        if (i > 0) {
            whereClause += ",";
        }

        whereClause += "'" + fileId + "'";
    }

    whereClause += ") AND EXISTS (SELECT 1 FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE " + ANALYSIS_ALBUM_TABLE + "." + PhotoAlbumColumns::ALBUM_ID +
        " = " + std::to_string(albumId) + " AND " +
        ANALYSIS_ALBUM_TABLE + ".tag_id = " + VISION_IMAGE_FACE_TABLE + ".tag_id)";

    return whereClause;
}

static int HandleAnalysisFaceUpdate(MediaLibraryCommand& cmd, NativeRdb::ValuesBucket &value,
    const DataShare::DataSharePredicates &predicates)
{
    string keyOperation = cmd.GetQuerySetParam(MEDIA_OPERN_KEYWORD);
    if (keyOperation.empty() || keyOperation != UPDATE_DISMISS_ASSET) {
        cmd.SetValueBucket(value);
        return MediaLibraryObjectUtils::ModifyInfoByIdInDb(cmd);
    }

    const string &clause = predicates.GetWhereClause();
    std::vector<std::string> clauses = SplitUriString(clause, ',');
    CHECK_AND_RETURN_RET_LOG(!clause.empty(), E_INVALID_FILEID, "Clause is empty, cannot extract album ID.");
    std::string albumStr = clauses[0];
    int32_t albumId {0};
    std::stringstream ss(albumStr);
    CHECK_AND_RETURN_RET_LOG((ss >> albumId), E_INVALID_FILEID, "Unable to convert albumId string to integer.");

    std::vector<std::string> uris;
    for (size_t i = 1; i < clauses.size(); ++i) {
        uris.push_back(clauses[i]);
    }
    if (uris.empty()) {
        MEDIA_ERR_LOG("No URIs found after album ID.");
        return E_INVALID_FILEID;
    }
    std::string predicate = BuildWhereClause(uris, albumId);
    cmd.SetValueBucket(value);
    cmd.GetAbsRdbPredicates()->SetWhereClause(predicate);
    return MediaLibraryObjectUtils::ModifyInfoByIdInDb(cmd);
}

static int32_t HandleFilesystemOperations(MediaLibraryCommand &cmd)
{
    switch (cmd.GetOprnObject()) {
        case OperationObject::FILESYSTEM_ASSET: {
            auto ret = MediaLibraryFileOperations::ModifyFileOperation(cmd);
            if (ret == E_SAME_PATH) {
                return E_OK;
            } else {
                return ret;
            }
        }
        case OperationObject::FILESYSTEM_DIR:
            // supply a ModifyDirOperation here to replace
            // modify in the HandleDirOperations in Insert function, if need
            return E_OK;

        case OperationObject::FILESYSTEM_ALBUM: {
            return MediaLibraryAlbumOperations::ModifyAlbumOperation(cmd);
        }
        default:
            return E_OK;
    }
}

int32_t MediaLibraryDataManager::UpdateInternal(MediaLibraryCommand &cmd, NativeRdb::ValuesBucket &value,
    const DataShare::DataSharePredicates &predicates)
{
    int32_t result = HandleFilesystemOperations(cmd);
    if (result != E_OK) {
        return result;
    }
    switch (cmd.GetOprnObject()) {
        case OperationObject::PAH_PHOTO:
        case OperationObject::PAH_VIDEO:
        case OperationObject::FILESYSTEM_PHOTO:
        case OperationObject::FILESYSTEM_AUDIO:
        case OperationObject::PTP_OPERATION: {
            return MediaLibraryAssetOperations::UpdateOperation(cmd);
        }
        case OperationObject::ANALYSIS_PHOTO_ALBUM: {
            if ((cmd.GetOprnType() >= OperationType::PORTRAIT_DISPLAY_LEVEL &&
                 cmd.GetOprnType() <= OperationType::GROUP_COVER_URI)) {
                return MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(cmd.GetOprnType(), value, predicates);
            }
            break;
        }
        case OperationObject::PHOTO_ALBUM:
            return MediaLibraryAlbumOperations::HandlePhotoAlbum(cmd.GetOprnType(), value, predicates);
        case OperationObject::GEO_DICTIONARY:
        case OperationObject::GEO_KNOWLEDGE:
            return MediaLibraryLocationOperations::UpdateOperation(cmd);
        case OperationObject::STORY_ALBUM:
        case OperationObject::STORY_COVER:
        case OperationObject::STORY_PLAY:
        case OperationObject::USER_PHOTOGRAPHY:
            return MediaLibraryStoryOperations::UpdateOperation(cmd);
        case OperationObject::PAH_MULTISTAGES_CAPTURE: {
            std::vector<std::string> columns;
            MultiStagesPhotoCaptureManager::GetInstance().HandleMultiStagesOperation(cmd, columns);
            return E_OK;
        }
        case OperationObject::PAH_BATCH_THUMBNAIL_OPERATE:
            return ProcessThumbnailBatchCmd(cmd, value, predicates);
        case OperationObject::PAH_CLOUD_ENHANCEMENT_OPERATE:
            return EnhancementManager::GetInstance().HandleEnhancementUpdateOperation(cmd);
        case OperationObject::VISION_IMAGE_FACE:
            return HandleAnalysisFaceUpdate(cmd, value, predicates);
        default:
            break;
    }
    // ModifyInfoByIdInDb can finish the default update of smartalbum and smartmap,
    // so no need to distinct them in switch-case deliberately
    cmd.SetValueBucket(value);
    return MediaLibraryObjectUtils::ModifyInfoByIdInDb(cmd);
}

void MediaLibraryDataManager::InterruptBgworker()
{
    shared_lock<shared_mutex> sharedLock(mgrSharedMutex_);
    if (refCnt_.load() <= 0) {
        MEDIA_DEBUG_LOG("MediaLibraryDataManager is not initialized");
        return;
    }
    shared_ptr<MediaLibraryAsyncWorker> mediaAsyncWorker = MediaLibraryAsyncWorker::GetInstance();
    if (mediaAsyncWorker != nullptr) {
        mediaAsyncWorker->Interrupt();
    }
    shared_ptr<TrashAsyncTaskWorker> asyncWorker = TrashAsyncTaskWorker::GetInstance();
    CHECK_AND_RETURN_LOG(asyncWorker != nullptr, "asyncWorker null");
    asyncWorker->Interrupt();
}

void MediaLibraryDataManager::InterruptThumbnailBgWorker()
{
    shared_lock<shared_mutex> sharedLock(mgrSharedMutex_);
    if (refCnt_.load() <= 0) {
        MEDIA_DEBUG_LOG("MediaLibraryDataManager is not initialized");
        return;
    }
    CHECK_AND_RETURN_LOG(thumbnailService_ != nullptr, "thumbnailService_ is nullptr");
    thumbnailService_->InterruptBgworker();
}

int32_t MediaLibraryDataManager::GenerateThumbnailBackground()
{
    shared_lock<shared_mutex> sharedLock(mgrSharedMutex_);
    if (refCnt_.load() <= 0) {
        MEDIA_DEBUG_LOG("MediaLibraryDataManager is not initialized");
        return E_FAIL;
    }

    if (thumbnailService_ == nullptr) {
        return E_THUMBNAIL_SERVICE_NULLPTR;
    }
    return thumbnailService_->GenerateThumbnailBackground();
}

int32_t MediaLibraryDataManager::GenerateHighlightThumbnailBackground()
{
    shared_lock<shared_mutex> sharedLock(mgrSharedMutex_);
    if (refCnt_.load() <= 0) {
        MEDIA_DEBUG_LOG("MediaLibraryDataManager is not initialized");
        return E_FAIL;
    }

    if (thumbnailService_ == nullptr) {
        return E_THUMBNAIL_SERVICE_NULLPTR;
    }
    return thumbnailService_->GenerateHighlightThumbnailBackground();
}

int32_t MediaLibraryDataManager::UpgradeThumbnailBackground(bool isWifiConnected)
{
    shared_lock<shared_mutex> sharedLock(mgrSharedMutex_);
    if (refCnt_.load() <= 0) {
        MEDIA_DEBUG_LOG("MediaLibraryDataManager is not initialized");
        return E_FAIL;
    }

    if (thumbnailService_ == nullptr) {
        return E_THUMBNAIL_SERVICE_NULLPTR;
    }
    return thumbnailService_->UpgradeThumbnailBackground(isWifiConnected);
}

int32_t MediaLibraryDataManager::RestoreThumbnailDualFrame()
{
    shared_lock<shared_mutex> sharedLock(mgrSharedMutex_);
    if (refCnt_.load() <= 0) {
        MEDIA_DEBUG_LOG("MediaLibraryDataManager is not initialized");
        return E_FAIL;
    }

    if (thumbnailService_ == nullptr) {
        return E_THUMBNAIL_SERVICE_NULLPTR;
    }
    return thumbnailService_->RestoreThumbnailDualFrame();
}

static void CacheAging()
{
    if (!MediaFileUtils::IsDirectory(MEDIA_CACHE_DIR)) {
        return;
    }
    time_t now = time(nullptr);
    constexpr int thresholdSeconds = 24 * 60 * 60; // 24 hours
    vector<string> files;
    GetDirFiles(MEDIA_CACHE_DIR, files);
    for (auto &file : files) {
        struct stat statInfo {};
        if (stat(file.c_str(), &statInfo) != 0) {
            MEDIA_WARN_LOG("skip %{private}s , stat errno: %{public}d", file.c_str(), errno);
            continue;
        }
        time_t timeModified = statInfo.st_mtime;
        double duration = difftime(now, timeModified); // diff in seconds
        if (duration < thresholdSeconds) {
            continue;
        }
        if (!MediaFileUtils::DeleteFile(file)) {
            MEDIA_ERR_LOG("delete failed %{public}s, errno: %{public}d", file.c_str(), errno);
        }
    }
}

static int32_t ClearInvalidDeletedAlbum()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr");
        return E_FAIL;
    }

    const std::string QUERY_NO_CLOUD_DELETED_ALBUM_INFO =
        "SELECT album_id, album_name FROM PhotoAlbum WHERE " + PhotoAlbumColumns::ALBUM_DIRTY +
        " = " + std::to_string(static_cast<int32_t>(DirtyTypes::TYPE_DELETED)) +
        " AND " + PhotoColumn::PHOTO_CLOUD_ID + " is NULL";
    shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(QUERY_NO_CLOUD_DELETED_ALBUM_INFO);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query not match data fails");
        return E_HAS_DB_ERROR;
    }

    vector<string> albumIds;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int columnIndex = 0;
        int32_t albumId = -1;
        if (resultSet->GetColumnIndex(PhotoAlbumColumns::ALBUM_ID, columnIndex) == NativeRdb::E_OK) {
            resultSet->GetInt(columnIndex, albumId);
            albumIds.emplace_back(to_string(albumId));
        }
        std::string albumName = "";
        if (resultSet->GetColumnIndex(PhotoAlbumColumns::ALBUM_NAME, columnIndex) == NativeRdb::E_OK) {
            resultSet->GetString(columnIndex, albumName);
        }
        MEDIA_INFO_LOG("Handle name %{public}s id %{public}d", DfxUtils::GetSafeAlbumName(albumName).c_str(), albumId);
    }

    NativeRdb::RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.In(PhotoAlbumColumns::ALBUM_ID, albumIds);
    int deleteRow = -1;
    auto ret = rdbStore->Delete(deleteRow, predicates);
    MEDIA_INFO_LOG("Delete invalid album, deleteRow is %{public}d", deleteRow);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Delete invalid album failed, ret = %{public}d, deleteRow is %{public}d", ret, deleteRow);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

int32_t MediaLibraryDataManager::DoAging()
{
    shared_lock<shared_mutex> sharedLock(mgrSharedMutex_);
    MEDIA_DEBUG_LOG("MediaLibraryDataManager::DoAging IN");
    if (refCnt_.load() <= 0) {
        MEDIA_DEBUG_LOG("MediaLibraryDataManager is not initialized");
        return E_FAIL;
    }

    CacheAging(); // aging file in .cache

    ClearInvalidDeletedAlbum(); // Clear invalid album data with null cloudid and dirty '4'

    shared_ptr<TrashAsyncTaskWorker> asyncWorker = TrashAsyncTaskWorker::GetInstance();
    if (asyncWorker == nullptr) {
        MEDIA_ERR_LOG("asyncWorker null");
        return E_FAIL;
    }
    asyncWorker->Init();
    return E_OK;
}

static string GenerateUuid()
{
    uuid_t uuid;
    uuid_generate(uuid);
    char str[UUID_STR_LENGTH] = {};
    uuid_unparse(uuid, str);
    return str;
}

static string generateRegexpMatchForNumber(const int32_t num)
{
    string regexpMatchNumber = "[0-9]";
    string strRegexpMatch = "";
    for (int i = 0; i < num; i++) {
        strRegexpMatch += regexpMatchNumber;
    }
    return strRegexpMatch;
}

static string generateUpdateSql(const bool isCover, const string title, const int32_t ownerAlbumId)
{
    uint32_t index = title.find_first_of("BURST");
    string globMember = title.substr(0, index) + "BURST" + generateRegexpMatchForNumber(3);
    string globCover = globMember + "_COVER";
    string updateSql;
    if (isCover) {
        string burstkey = GenerateUuid();
        updateSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_SUBTYPE + " = " +
            to_string(static_cast<int32_t>(PhotoSubType::BURST)) + ", " + PhotoColumn::PHOTO_BURST_KEY + " = '" +
            burstkey + "', " + PhotoColumn::PHOTO_BURST_COVER_LEVEL + " = CASE WHEN " + MediaColumn::MEDIA_TITLE +
            " NOT LIKE '%COVER%' THEN " + to_string(static_cast<int32_t>(BurstCoverLevelType::MEMBER)) + " ELSE " +
            to_string(static_cast<int32_t>(BurstCoverLevelType::COVER)) + " END WHERE " + MediaColumn::MEDIA_TYPE +
            " = " + to_string(static_cast<int32_t>(MEDIA_TYPE_IMAGE)) + " AND " + PhotoColumn::PHOTO_SUBTYPE + " != " +
            to_string(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) + " AND " + PhotoColumn::PHOTO_OWNER_ALBUM_ID +
            " = " + to_string(ownerAlbumId) + " AND (LOWER(" + MediaColumn::MEDIA_TITLE + ") GLOB LOWER('" +
            globMember + "') OR LOWER(" + MediaColumn::MEDIA_TITLE + ") GLOB LOWER('" + globCover + "'));";
    } else {
        string subWhere = "FROM " + PhotoColumn::PHOTOS_TABLE + " AS p2 WHERE LOWER(p2." + MediaColumn::MEDIA_TITLE +
            ") GLOB LOWER('" + globCover + "') AND p2." + PhotoColumn::PHOTO_OWNER_ALBUM_ID + " = " +
            to_string(ownerAlbumId);

        updateSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE + " AS p1 SET " + PhotoColumn::PHOTO_BURST_KEY +
            " = (SELECT CASE WHEN p2." + PhotoColumn::PHOTO_BURST_KEY + " IS NOT NULL THEN p2." +
            PhotoColumn::PHOTO_BURST_KEY + " ELSE NULL END " + subWhere + " LIMIT 1 ), " +
            PhotoColumn::PHOTO_BURST_COVER_LEVEL + " = (SELECT CASE WHEN COUNT(1) > 0 THEN " +
            to_string(static_cast<int32_t>(BurstCoverLevelType::MEMBER)) + " ELSE " +
            to_string(static_cast<int32_t>(BurstCoverLevelType::COVER)) + " END " + subWhere + "), " +
            PhotoColumn::PHOTO_SUBTYPE + " = (SELECT CASE WHEN COUNT(1) > 0 THEN " +
            to_string(static_cast<int32_t>(PhotoSubType::BURST)) + " ELSE p1." + PhotoColumn::PHOTO_SUBTYPE + " END " +
            subWhere + ") WHERE p1." + MediaColumn::MEDIA_TITLE + " = '" + title + "' AND p1." +
            PhotoColumn::PHOTO_OWNER_ALBUM_ID + " = " + to_string(ownerAlbumId);
    }
    return updateSql;
}

static int32_t UpdateBurstPhoto(const bool isCover, const shared_ptr<MediaLibraryRdbStore> rdbStore,
    shared_ptr<NativeRdb::ResultSet> resultSet)
{
    int32_t count;
    int32_t retCount = resultSet->GetRowCount(count);
    if (count == 0) {
        if (isCover) {
            MEDIA_INFO_LOG("No burst cover need to update");
        } else {
            MEDIA_INFO_LOG("No burst member need to update");
        }
        return E_SUCCESS;
    }
    if (retCount != E_SUCCESS || count < 0) {
        return E_ERR;
    }

    int32_t ret = E_ERR;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int columnIndex = 0;
        string title;
        if (resultSet->GetColumnIndex(MediaColumn::MEDIA_TITLE, columnIndex) == NativeRdb::E_OK) {
            resultSet->GetString(columnIndex, title);
        }
        int32_t ownerAlbumId = 0;
        if (resultSet->GetColumnIndex(PhotoColumn::PHOTO_OWNER_ALBUM_ID, columnIndex) == NativeRdb::E_OK) {
            resultSet->GetInt(columnIndex, ownerAlbumId);
        }

        string updateSql = generateUpdateSql(isCover, title, ownerAlbumId);
        ret = rdbStore->ExecuteSql(updateSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("rdbStore->ExecuteSql failed, ret = %{public}d", ret);
            return E_HAS_DB_ERROR;
        }
    }
    return ret;
}

static shared_ptr<NativeRdb::ResultSet> QueryBurst(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const string globNameRule1, const string globNameRule2)
{
    string querySql = "SELECT " + MediaColumn::MEDIA_TITLE + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID +
        " FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " + MediaColumn::MEDIA_TYPE + " = " +
        to_string(static_cast<int32_t>(MEDIA_TYPE_IMAGE)) + " AND " + PhotoColumn::PHOTO_SUBTYPE + " != " +
        to_string(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) + " AND " + PhotoColumn::PHOTO_BURST_KEY +
        " IS NULL AND (LOWER(" + MediaColumn::MEDIA_TITLE + ") GLOB LOWER('" + globNameRule1 + "') OR LOWER(" +
        MediaColumn::MEDIA_TITLE + ") GLOB LOWER('" + globNameRule2 + "'))";
    
    auto resultSet = rdbStore->QueryByStep(querySql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("failed to acquire result from visitor query.");
    }
    return resultSet;
}

int32_t MediaLibraryDataManager::UpdateBurstFromGallery()
{
    MEDIA_INFO_LOG("Begin UpdateBurstFromGallery");
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryDataManager::UpdateBurstFromGallery");
    shared_lock<shared_mutex> sharedLock(mgrSharedMutex_);
    if (refCnt_.load() <= 0) {
        MEDIA_DEBUG_LOG("MediaLibraryDataManager is not initialized");
        return E_FAIL;
    }
    if (rdbStore_ == nullptr) {
        MEDIA_DEBUG_LOG("rdbStore_ is nullptr");
        return E_FAIL;
    }

    string globNameRule = "IMG_" + generateRegexpMatchForNumber(8) + "_" + generateRegexpMatchForNumber(6) + "_";

    // regexp match IMG_xxxxxxxx_xxxxxx_BURSTxxx, 'x' represents a number
    string globMemberStr1 = globNameRule + "BURST" + generateRegexpMatchForNumber(3);
    string globMemberStr2 = globNameRule + "[0-9]_BURST" + generateRegexpMatchForNumber(3);
    // regexp match IMG_xxxxxxxx_xxxxxx_BURSTxxx_COVER, 'x' represents a number
    string globCoverStr1 = globMemberStr1 + "_COVER";
    string globCoverStr2 = globMemberStr2 + "_COVER";
    
    auto resultSet = QueryBurst(rdbStore_, globCoverStr1, globCoverStr2);
    int32_t ret = UpdateBurstPhoto(true, rdbStore_, resultSet);
    if (ret != E_SUCCESS) {
        MEDIA_ERR_LOG("failed to UpdateBurstPhotoByCovers.");
        return E_FAIL;
    }

    resultSet = QueryBurst(rdbStore_, globMemberStr1, globMemberStr2);
    ret = UpdateBurstPhoto(false, rdbStore_, resultSet);
    if (ret != E_SUCCESS) {
        MEDIA_ERR_LOG("failed to UpdateBurstPhotoByMembers.");
        return E_FAIL;
    }
    MEDIA_INFO_LOG("End UpdateBurstFromGallery");
    return ret;
}

#ifdef DISTRIBUTED
int32_t MediaLibraryDataManager::LcdDistributeAging()
{
    MEDIA_DEBUG_LOG("MediaLibraryDataManager::LcdDistributeAging IN");
    auto deviceInstance = MediaLibraryDevice::GetInstance();
    if ((thumbnailService_ == nullptr) || (deviceInstance == nullptr)) {
        return E_THUMBNAIL_SERVICE_NULLPTR;
    }
    int32_t result = E_SUCCESS;
    vector<string> deviceUdids;
    deviceInstance->QueryAllDeviceUdid(deviceUdids);
    for (string &udid : deviceUdids) {
        result = thumbnailService_->LcdDistributeAging(udid);
        if (result != E_SUCCESS) {
            MEDIA_ERR_LOG("LcdDistributeAging fail result is %{public}d", result);
            break;
        }
    }
    return result;
}

int32_t MediaLibraryDataManager::DistributeDeviceAging()
{
    MEDIA_DEBUG_LOG("MediaLibraryDataManager::DistributeDeviceAging IN");
    auto deviceInstance = MediaLibraryDevice::GetInstance();
    if ((thumbnailService_ == nullptr) || (deviceInstance == nullptr)) {
        return E_FAIL;
    }
    int32_t result = E_FAIL;
    vector<MediaLibraryDeviceInfo> deviceDataBaseList;
    deviceInstance->QueryAgingDeviceInfos(deviceDataBaseList);
    MEDIA_DEBUG_LOG("MediaLibraryDevice InitDeviceRdbStore deviceDataBaseList size =  %{public}d",
        (int) deviceDataBaseList.size());
    for (MediaLibraryDeviceInfo deviceInfo : deviceDataBaseList) {
        result = thumbnailService_->InvalidateDistributeThumbnail(deviceInfo.deviceUdid);
        if (result != E_SUCCESS) {
            MEDIA_ERR_LOG("invalidate fail %{public}d", result);
            continue;
        }
    }
    return result;
}
#endif

int MediaLibraryDataManager::GetThumbnail(const string &uri)
{
    if (thumbnailService_ == nullptr) {
        return E_THUMBNAIL_SERVICE_NULLPTR;
    }
    if (!uri.empty() && MediaLibraryObjectUtils::CheckUriPending(uri)) {
        MEDIA_ERR_LOG("failed to get thumbnail, the file:%{private}s is pending", uri.c_str());
        return E_FAIL;
    }
    return thumbnailService_->GetThumbnailFd(uri);
}

void MediaLibraryDataManager::CreateThumbnailAsync(const string &uri, const string &path)
{
    shared_lock<shared_mutex> sharedLock(mgrSharedMutex_);
    if (refCnt_.load() <= 0) {
        MEDIA_DEBUG_LOG("MediaLibraryDataManager is not initialized");
        return;
    }

    if (thumbnailService_ == nullptr) {
        return;
    }
    if (!uri.empty()) {
        if (MediaLibraryObjectUtils::CheckUriPending(uri)) {
            MEDIA_ERR_LOG("failed to get thumbnail, the file:%{private}s is pending", uri.c_str());
            return;
        }
        int32_t err = thumbnailService_->CreateThumbnailFileScaned(uri, path);
        if (err != E_SUCCESS) {
            MEDIA_ERR_LOG("ThumbnailService CreateThumbnailFileScaned failed : %{public}d", err);
        }
    }
}

shared_ptr<ResultSetBridge> MediaLibraryDataManager::Query(MediaLibraryCommand &cmd,
    const vector<string> &columns, const DataSharePredicates &predicates, int &errCode)
{
    shared_lock<shared_mutex> sharedLock(mgrSharedMutex_);
    if (refCnt_.load() <= 0) {
        errCode = E_FAIL;
        MEDIA_DEBUG_LOG("MediaLibraryDataManager is not initialized");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, errCode},
            {KEY_OPT_TYPE, OptType::QUERY}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return nullptr;
    }

    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryDataManager::Query");
    if (rdbStore_ == nullptr) {
        errCode = E_FAIL;
        MEDIA_ERR_LOG("Rdb Store is not initialized");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, errCode},
            {KEY_OPT_TYPE, OptType::QUERY}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return nullptr;
    }

    auto absResultSet = QueryRdb(cmd, columns, predicates, errCode);
    if (absResultSet == nullptr) {
        errCode = (errCode != E_OK) ? errCode : E_FAIL;
        MEDIA_ERR_LOG("Query rdb failed, errCode: %{public}d", errCode);
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, errCode},
            {KEY_OPT_TYPE, OptType::QUERY}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return nullptr;
    }
    return RdbUtils::ToResultSetBridge(absResultSet);
}

#ifdef DISTRIBUTED
int32_t MediaLibraryDataManager::SyncPullThumbnailKeys(const Uri &uri)
{
    if (MediaLibraryDevice::GetInstance() == nullptr || !MediaLibraryDevice::GetInstance()->IsHasActiveDevice()) {
        return E_ERR;
    }
    if (kvStorePtr_ == nullptr) {
        return E_ERR;
    }

    MediaLibraryCommand cmd(uri, OperationType::QUERY);
    cmd.GetAbsRdbPredicates()->BeginWrap()->EqualTo(MEDIA_DATA_DB_MEDIA_TYPE, to_string(MEDIA_TYPE_IMAGE))
        ->Or()->EqualTo(MEDIA_DATA_DB_MEDIA_TYPE, to_string(MEDIA_TYPE_VIDEO))->EndWrap()
        ->And()->EqualTo(MEDIA_DATA_DB_DATE_TRASHED, to_string(0))
        ->And()->NotEqualTo(MEDIA_DATA_DB_MEDIA_TYPE, to_string(MEDIA_TYPE_ALBUM))
        ->OrderByDesc(MEDIA_DATA_DB_DATE_ADDED);
    vector<string> columns = { MEDIA_DATA_DB_THUMBNAIL, MEDIA_DATA_DB_LCD };
    auto resultset = MediaLibraryFileOperations::QueryFileOperation(cmd, columns);
    if (resultset == nullptr) {
        return E_HAS_DB_ERROR;
    }

    vector<string> thumbnailKeys;
    int count = 0;
    while (resultset->GoToNextRow() == NativeRdb::E_OK && count < MAX_QUERY_THUMBNAIL_KEY_COUNT) {
        string thumbnailKey =
            get<string>(ResultSetUtils::GetValFromColumn(MEDIA_DATA_DB_THUMBNAIL, resultset, TYPE_STRING));
        thumbnailKeys.push_back(thumbnailKey);
        string lcdKey = get<string>(ResultSetUtils::GetValFromColumn(MEDIA_DATA_DB_LCD, resultset, TYPE_STRING));
        thumbnailKeys.push_back(lcdKey);
        count++;
    }

    if (thumbnailKeys.empty()) {
        return E_NO_SUCH_FILE;
    }
    MediaLibrarySyncOperation::SyncPullKvstore(kvStorePtr_, thumbnailKeys,
        MediaFileUtils::GetNetworkIdFromUri(uri.ToString()));
    return E_SUCCESS;
}
#endif

static const map<OperationObject, string> QUERY_CONDITION_MAP {
    { OperationObject::SMART_ALBUM, SMARTALBUM_DB_ID },
    { OperationObject::SMART_ALBUM_MAP, SMARTALBUMMAP_DB_ALBUM_ID },
    { OperationObject::FILESYSTEM_DIR, MEDIA_DATA_DB_ID },
    { OperationObject::ALL_DEVICE, "" },
    { OperationObject::ACTIVE_DEVICE, "" },
    { OperationObject::ASSETMAP, "" },
    { OperationObject::SMART_ALBUM_ASSETS, "" },
    { OperationObject::BUNDLE_PERMISSION, "" },
};

bool CheckIsPortraitAlbum(MediaLibraryCommand &cmd)
{
    auto predicates = cmd.GetAbsRdbPredicates();
    auto whereClause = predicates->GetWhereClause();
    if (whereClause.find(USER_DISPLAY_LEVEL) != string::npos || whereClause.find(IS_ME) != string::npos ||
        whereClause.find(ALBUM_NAME_NOT_NULL) != string::npos) {
        return true;
    }
    return false;
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryDataManager::QuerySet(MediaLibraryCommand &cmd,
    const vector<string> &columns, const DataSharePredicates &predicates, int &errCode)
{
    MediaLibraryTracer tracer;
    tracer.Start("QueryRdb");
    tracer.Start("CheckWhereClause");
    MEDIA_DEBUG_LOG("CheckWhereClause start %{public}s", cmd.GetUri().ToString().c_str());
    auto whereClause = predicates.GetWhereClause();
    if (!MediaLibraryCommonUtils::CheckWhereClause(whereClause)) {
        errCode = E_INVALID_VALUES;
        MEDIA_ERR_LOG("illegal query whereClause input %{private}s", whereClause.c_str());
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, errCode},
            {KEY_OPT_TYPE, OptType::QUERY}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return nullptr;
    }
    MEDIA_DEBUG_LOG("CheckWhereClause end");
    tracer.Finish();

    cmd.SetDataSharePred(predicates);
    // MEDIALIBRARY_TABLE just for RdbPredicates
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicates, MEDIALIBRARY_TABLE);
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    cmd.GetAbsRdbPredicates()->SetOrder(rdbPredicate.GetOrder());
    MediaLibraryRdbUtils::AddVirtualColumnsOfDateType(const_cast<vector<string> &>(columns));

    OperationObject oprnObject = cmd.GetOprnObject();
    auto it = QUERY_CONDITION_MAP.find(oprnObject);
    if (it != QUERY_CONDITION_MAP.end()) {
        return MediaLibraryObjectUtils::QueryWithCondition(cmd, columns, it->second);
    }

    return QueryInternal(cmd, columns, predicates);
}

shared_ptr<NativeRdb::ResultSet> QueryAnalysisAlbum(MediaLibraryCommand &cmd,
    const vector<string> &columns, const DataSharePredicates &predicates)
{
    RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, cmd.GetTableName());
    int32_t albumSubtype = MediaLibraryRdbUtils::GetAlbumSubtypeArgument(rdbPredicates);
    MEDIA_DEBUG_LOG("Query analysis album of subtype: %{public}d", albumSubtype);
    if (albumSubtype == PhotoAlbumSubType::GROUP_PHOTO) {
        return MediaLibraryAnalysisAlbumOperations::QueryGroupPhotoAlbum(cmd, columns);
    }
    if (CheckIsPortraitAlbum(cmd)) {
        return MediaLibraryAlbumOperations::QueryPortraitAlbum(cmd, columns);
    }
    return MediaLibraryRdbStore::QueryWithFilter(rdbPredicates, columns);
}

shared_ptr<NativeRdb::ResultSet> QueryGeo(const RdbPredicates &rdbPredicates, const vector<string> &columns)
{
    auto queryResult = MediaLibraryRdbStore::QueryWithFilter(rdbPredicates, columns);
    if (queryResult == nullptr) {
        MEDIA_ERR_LOG("Query Geographic Information Failed, queryResult is nullptr");
        return queryResult;
    }

    const vector<string> &whereArgs = rdbPredicates.GetWhereArgs();
    if (whereArgs.empty() || whereArgs.front().empty()) {
        MEDIA_ERR_LOG("Query Geographic Information can not get fileId");
        return queryResult;
    }

    string fileId = whereArgs.front();
    if (queryResult->GoToNextRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Query Geographic Information Failed, fileId: %{public}s", fileId.c_str());
        return queryResult;
    }

    string latitude = GetStringVal(PhotoColumn::PHOTOS_TABLE + "." + LATITUDE, queryResult);
    string longitude = GetStringVal(PhotoColumn::PHOTOS_TABLE + "." + LONGITUDE, queryResult);
    string addressDescription = GetStringVal(ADDRESS_DESCRIPTION, queryResult);
    MEDIA_INFO_LOG(
        "QueryGeo, fileId: %{public}s, latitude: %{public}s, longitude: %{public}s, addressDescription: %{private}s",
        fileId.c_str(), latitude.c_str(), longitude.c_str(), addressDescription.c_str());

    if (!(latitude == "0" && longitude == "0") && addressDescription.empty()) {
        const std::vector<std::string> geoInfo{ fileId, latitude, longitude };
        std::future<bool> futureResult =
            std::async(std::launch::async, MediaAnalysisHelper::ParseGeoInfo, std::move(geoInfo));

        bool parseResult = false;
        const int timeout = 5;
        std::future_status futureStatus = futureResult.wait_for(std::chrono::seconds(timeout));
        if (futureStatus == std::future_status::ready) {
            parseResult = futureResult.get();
        } else {
            MEDIA_ERR_LOG("ParseGeoInfo Failed, fileId: %{public}s, futureStatus: %{public}d", fileId.c_str(),
                static_cast<int>(futureStatus));
        }

        if (parseResult) {
            queryResult = MediaLibraryRdbStore::QueryWithFilter(rdbPredicates, columns);
        }
        MEDIA_INFO_LOG("ParseGeoInfo completed, fileId: %{public}s, parseResult: %{public}d", fileId.c_str(),
            parseResult);
    }
    return queryResult;
}

shared_ptr<NativeRdb::ResultSet> QueryIndex(MediaLibraryCommand &cmd, const vector<string> &columns,
    const DataSharePredicates &predicates)
{
    switch (cmd.GetOprnType()) {
        case OperationType::UPDATE_SEARCH_INDEX:
            return MediaLibraryRdbStore::Query(RdbUtils::ToPredicates(predicates, cmd.GetTableName()), columns);
        default:
            /* add filter */
            return MediaLibraryRdbStore::QueryWithFilter(RdbUtils::ToPredicates(predicates, cmd.GetTableName()),
                columns);
    }
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryDataManager::QueryInternal(MediaLibraryCommand &cmd,
    const vector<string> &columns, const DataSharePredicates &predicates)
{
    MediaLibraryTracer tracer;
    switch (cmd.GetOprnObject()) {
        case OperationObject::FILESYSTEM_ALBUM:
        case OperationObject::MEDIA_VOLUME:
            return MediaLibraryAlbumOperations::QueryAlbumOperation(cmd, columns);
        case OperationObject::INDEX_CONSTRUCTION_STATUS:
            return MediaLibrarySearchOperations::QueryIndexConstructProgress();
        case OperationObject::PHOTO_ALBUM:
            return MediaLibraryAlbumOperations::QueryPhotoAlbum(cmd, columns);
        case OperationObject::ANALYSIS_PHOTO_ALBUM:
            return QueryAnalysisAlbum(cmd, columns, predicates);
        case OperationObject::PHOTO_MAP:
        case OperationObject::ANALYSIS_PHOTO_MAP:
            return PhotoMapOperations::QueryPhotoAssets(
                RdbUtils::ToPredicates(predicates, PhotoColumn::PHOTOS_TABLE), columns);
        case OperationObject::FILESYSTEM_PHOTO:
        case OperationObject::FILESYSTEM_AUDIO:
        case OperationObject::PAH_MOVING_PHOTO:
        case OperationObject::EDIT_DATA_EXISTS:
            return MediaLibraryAssetOperations::QueryOperation(cmd, columns);
        case OperationObject::VISION_START ... OperationObject::VISION_END:
            return MediaLibraryRdbStore::QueryWithFilter(RdbUtils::ToPredicates(predicates, cmd.GetTableName()),
                columns);
        case OperationObject::GEO_DICTIONARY:
        case OperationObject::GEO_KNOWLEDGE:
        case OperationObject::GEO_PHOTO:
        case OperationObject::CONVERT_PHOTO:
        case OperationObject::STORY_ALBUM:
        case OperationObject::STORY_COVER:
        case OperationObject::STORY_PLAY:
        case OperationObject::USER_PHOTOGRAPHY:
        case OperationObject::APP_URI_PERMISSION_INNER:
            return MediaLibraryRdbStore::QueryWithFilter(RdbUtils::ToPredicates(predicates, cmd.GetTableName()),
                columns);
        case OperationObject::SEARCH_TOTAL:
            return QueryIndex(cmd, columns, predicates);
        case OperationObject::PAH_MULTISTAGES_CAPTURE:
            return MultiStagesPhotoCaptureManager::GetInstance().HandleMultiStagesOperation(cmd, columns);
        case OperationObject::PAH_CLOUD_ENHANCEMENT_OPERATE:
            return EnhancementManager::GetInstance().HandleEnhancementQueryOperation(cmd, columns);
        case OperationObject::ANALYSIS_ADDRESS:
            return QueryGeo(RdbUtils::ToPredicates(predicates, cmd.GetTableName()), columns);
        case OperationObject::TAB_OLD_PHOTO:
            return MediaLibraryTabOldPhotosOperations().Query(
                RdbUtils::ToPredicates(predicates, TabOldPhotosColumn::OLD_PHOTOS_TABLE), columns);
        default:
            tracer.Start("QueryFile");
            return MediaLibraryFileOperations::QueryFileOperation(cmd, columns);
    }
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryDataManager::QueryRdb(MediaLibraryCommand &cmd,
    const vector<string> &columns, const DataSharePredicates &predicates, int &errCode)
{
    shared_lock<shared_mutex> sharedLock(mgrSharedMutex_);
    if (refCnt_.load() <= 0) {
        errCode = E_FAIL;
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, errCode},
            {KEY_OPT_TYPE, OptType::QUERY}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        MEDIA_DEBUG_LOG("MediaLibraryDataManager is not initialized");
        return nullptr;
    }

    return QuerySet(cmd, columns, predicates, errCode);
}

#ifdef DISTRIBUTED
bool MediaLibraryDataManager::QuerySync(const string &networkId, const string &tableName)
{
    if (networkId.empty() || tableName.empty()) {
        return false;
    }

    OHOS::DistributedHardware::DmDeviceInfo deviceInfo;
    auto &deviceManager = OHOS::DistributedHardware::DeviceManager::GetInstance();
    auto ret = deviceManager.GetLocalDeviceInfo(bundleName_, deviceInfo);
    if (ret != ERR_OK) {
        MEDIA_ERR_LOG("MediaLibraryDataManager QuerySync Failed to get local device info.");
        return false;
    }

    if (networkId == string(deviceInfo.networkId)) {
        return true;
    }

    int32_t syncStatus = DEVICE_SYNCSTATUSING;
    auto result = MediaLibraryDevice::GetInstance()->GetDeviceSyncStatus(networkId, tableName, syncStatus);
    if (result && syncStatus == DEVICE_SYNCSTATUS_COMPLETE) {
        return true;
    }

    vector<string> devices = { networkId };
    MediaLibrarySyncOpts syncOpts;
    syncOpts.rdbStore = rdbStore_;
    syncOpts.table = tableName;
    syncOpts.bundleName = bundleName_;
    return MediaLibrarySyncOperation::SyncPullTable(syncOpts, devices);
}
#endif

int32_t MediaLibraryDataManager::OpenFile(MediaLibraryCommand &cmd, const string &mode)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryDataManager::OpenFile");
    auto oprnObject = cmd.GetOprnObject();
    if (oprnObject == OperationObject::FILESYSTEM_PHOTO || oprnObject == OperationObject::FILESYSTEM_AUDIO ||
        oprnObject == OperationObject::HIGHLIGHT_COVER  || oprnObject == OperationObject::HIGHLIGHT_URI ||
        oprnObject == OperationObject::PTP_OPERATION) {
        return MediaLibraryAssetOperations::OpenOperation(cmd, mode);
    }

#ifdef MEDIALIBRARY_COMPATIBILITY
    if (oprnObject != OperationObject::THUMBNAIL && oprnObject != OperationObject::THUMBNAIL_ASTC &&
        oprnObject != OperationObject::REQUEST_PICTURE && oprnObject != OperationObject::PHOTO_REQUEST_PICTURE_BUFFER &&
        oprnObject != OperationObject::KEY_FRAME) {
        string opObject = MediaFileUri::GetPathFirstDentry(const_cast<Uri &>(cmd.GetUri()));
        if (opObject == IMAGE_ASSET_TYPE || opObject == VIDEO_ASSET_TYPE || opObject == URI_TYPE_PHOTO) {
            cmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
            return MediaLibraryAssetOperations::OpenOperation(cmd, mode);
        }
        if (opObject == AUDIO_ASSET_TYPE || opObject == URI_TYPE_AUDIO_V10) {
            cmd.SetOprnObject(OperationObject::FILESYSTEM_AUDIO);
            return MediaLibraryAssetOperations::OpenOperation(cmd, mode);
        }
    }
#endif

    return MediaLibraryObjectUtils::OpenFile(cmd, mode);
}

void MediaLibraryDataManager::NotifyChange(const Uri &uri)
{
    shared_lock<shared_mutex> sharedLock(mgrSharedMutex_);
    if (refCnt_.load() <= 0) {
        MEDIA_DEBUG_LOG("MediaLibraryDataManager is not initialized");
        return;
    }

    if (extension_ == nullptr) {
        MEDIA_ERR_LOG("MediaLibraryDataManager::NotifyChange failed");
        return;
    }

    extension_->NotifyChange(uri);
}

int32_t MediaLibraryDataManager::InitialiseThumbnailService(
    const shared_ptr<OHOS::AbilityRuntime::Context> &extensionContext)
{
    if (thumbnailService_ != nullptr) {
        return E_OK;
    }
    thumbnailService_ = ThumbnailService::GetInstance();
    if (thumbnailService_ == nullptr) {
        return E_THUMBNAIL_SERVICE_NULLPTR;
    }
#ifdef DISTRIBUTED
    thumbnailService_->Init(rdbStore_, kvStorePtr_, extensionContext);
#else
    thumbnailService_->Init(rdbStore_,  extensionContext);
#endif
    return E_OK;
}

void MediaLibraryDataManager::InitACLPermission()
{
    if (access(THUMB_DIR.c_str(), F_OK) == 0) {
        return;
    }

    if (!MediaFileUtils::CreateDirectory(THUMB_DIR)) {
        MEDIA_ERR_LOG("Failed create thumbs Photo dir");
        return;
    }

    if (Acl::AclSetDefault() != E_OK) {
        MEDIA_ERR_LOG("Failed to set the acl read permission for the thumbs Photo dir");
    }
}

void MediaLibraryDataManager::InitDatabaseACLPermission()
{
    if (access(RDB_DIR.c_str(), F_OK) != E_OK) {
        if (!MediaFileUtils::CreateDirectory(RDB_DIR)) {
            MEDIA_ERR_LOG("Failed create media rdb dir");
            return;
        }
    }

    if (access(KVDB_DIR.c_str(), F_OK) != E_OK) {
        if (!MediaFileUtils::CreateDirectory(KVDB_DIR)) {
            MEDIA_ERR_LOG("Failed create media kvdb dir");
            return;
        }
    }

    if (Acl::AclSetDatabase() != E_OK) {
        MEDIA_ERR_LOG("Failed to set the acl db permission for the media db dir");
    }
}

int32_t ScanFileCallback::OnScanFinished(const int32_t status, const string &uri, const string &path)
{
    auto instance = MediaLibraryDataManager::GetInstance();
    if (instance != nullptr) {
        instance->CreateThumbnailAsync(uri, path);
    }
    return E_OK;
}

int32_t MediaLibraryDataManager::SetCmdBundleAndDevice(MediaLibraryCommand &outCmd)
{
    string clientBundle = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    if (clientBundle.empty()) {
        MEDIA_ERR_LOG("GetClientBundleName failed");
        return E_GET_CLIENTBUNDLE_FAIL;
    }
    outCmd.SetBundleName(clientBundle);
#ifdef DISTRIBUTED
    OHOS::DistributedHardware::DmDeviceInfo deviceInfo;
    auto &deviceManager = OHOS::DistributedHardware::DeviceManager::GetInstance();
    int32_t ret = deviceManager.GetLocalDeviceInfo(bundleName_, deviceInfo);
    if (ret < 0) {
        MEDIA_ERR_LOG("GetLocalDeviceInfo ret = %{public}d", ret);
    } else {
        outCmd.SetDeviceName(deviceInfo.deviceName);
    }
    return ret;
#endif
    return 0;
}

int32_t MediaLibraryDataManager::DoTrashAging(shared_ptr<int> countPtr)
{
    shared_ptr<int> smartAlbumTrashPtr = make_shared<int>();
    MediaLibrarySmartAlbumMapOperations::HandleAgingOperation(smartAlbumTrashPtr);

    shared_ptr<int> albumTrashtPtr = make_shared<int>();
    MediaLibraryAlbumOperations::HandlePhotoAlbum(OperationType::AGING, {}, {}, albumTrashtPtr);

    shared_ptr<int> audioTrashtPtr = make_shared<int>();
    MediaLibraryAudioOperations::TrashAging(audioTrashtPtr);

    if (countPtr != nullptr) {
      *countPtr = *smartAlbumTrashPtr + *albumTrashtPtr + *audioTrashtPtr;
    }
    return E_SUCCESS;
}

int32_t MediaLibraryDataManager::RevertPendingByFileId(const std::string &fileId)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::UPDATE);
    ValuesBucket values;
    values.PutLong(Media::MEDIA_DATA_DB_TIME_PENDING, 0);
    cmd.SetValueBucket(values);
    int32_t retVal = MediaLibraryObjectUtils::ModifyInfoByIdInDb(cmd, fileId);
    if (retVal <= 0) {
        MEDIA_ERR_LOG("failed to revert pending error, fileId:%{private}s", fileId.c_str());
        return retVal;
    }
    auto fileAsset = MediaLibraryObjectUtils::GetFileAssetFromId(fileId);
    string srcPath = fileAsset->GetPath();
    MediaLibraryObjectUtils::ScanFileAsync(srcPath, fileId, MediaLibraryApi::API_10);
    return E_SUCCESS;
}

int32_t MediaLibraryDataManager::RevertPendingByPackage(const std::string &bundleName)
{
    MediaLibraryCommand queryCmd(OperationObject::FILESYSTEM_ASSET, OperationType::QUERY);
    queryCmd.GetAbsRdbPredicates()
        ->EqualTo(MEDIA_DATA_DB_OWNER_PACKAGE, bundleName)
        ->And()
        ->NotEqualTo(MEDIA_DATA_DB_TIME_PENDING, to_string(0));
    vector<string> columns = { MEDIA_DATA_DB_ID };
    auto result = MediaLibraryObjectUtils::QueryWithCondition(queryCmd, columns);
    if (result == nullptr) {
        return E_HAS_DB_ERROR;
    }

    int32_t ret = E_SUCCESS;
    while (result->GoToNextRow() == NativeRdb::E_OK) {
        int32_t id = GetInt32Val(MEDIA_DATA_DB_ID, result);
        int32_t retVal = RevertPendingByFileId(to_string(id));
        if (retVal != E_SUCCESS) {
            ret = retVal;
            MEDIA_ERR_LOG("Revert file %{public}d failed, ret=%{public}d", id, retVal);
            continue;
        }
    }
    return ret;
}

void MediaLibraryDataManager::SetStartupParameter()
{
    static constexpr uint32_t BASE_USER_RANGE = 200000; // for get uid
    uid_t uid = getuid() / BASE_USER_RANGE;
    const string key = "multimedia.medialibrary.startup." + to_string(uid);
    string value = "true";
    int32_t ret = SetParameter(key.c_str(), value.c_str());
    if (ret != 0) {
        MEDIA_ERR_LOG("Failed to set startup, result: %{public}d", ret);
    } else {
        MEDIA_INFO_LOG("Set startup success: %{public}s", to_string(uid).c_str());
    }
    // init backup param
    std::string backupParam = "persist.multimedia.medialibrary.rdb_switch_status";
    ret = system::SetParameter(backupParam, "0");
    if (ret != 0) {
        MEDIA_ERR_LOG("Failed to set parameter backup, ret:%{public}d", ret);
    }
    std::string nextFlag = "persist.update.hmos_to_next_flag";
    auto isUpgrade = system::GetParameter(nextFlag, "");
    MEDIA_INFO_LOG("isUpgrade:%{public}s", isUpgrade.c_str());
    if (isUpgrade != "1") {
        return;
    }
    std::string CLONE_FLAG = "multimedia.medialibrary.cloneFlag";
    auto currentTime = to_string(MediaFileUtils::UTCTimeSeconds());
    MEDIA_INFO_LOG("SetParameterForClone currentTime:%{public}s", currentTime.c_str());
    bool retFlag = system::SetParameter(CLONE_FLAG, currentTime);
    if (!retFlag) {
        MEDIA_ERR_LOG("Failed to set parameter cloneFlag, retFlag:%{public}d", retFlag);
    }
}

int32_t MediaLibraryDataManager::ProcessThumbnailBatchCmd(const MediaLibraryCommand &cmd,
    const NativeRdb::ValuesBucket &value, const DataShare::DataSharePredicates &predicates)
{
    CHECK_AND_RETURN_RET(thumbnailService_ != nullptr, E_THUMBNAIL_SERVICE_NULLPTR);

    int32_t requestId = 0;
    ValueObject valueObject;
    if (value.GetObject(THUMBNAIL_BATCH_GENERATE_REQUEST_ID, valueObject)) {
        valueObject.GetInt(requestId);
    }

    if (cmd.GetOprnType() == OperationType::START_GENERATE_THUMBNAILS) {
        NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicates, PhotoColumn::PHOTOS_TABLE);
        return thumbnailService_->CreateAstcBatchOnDemand(rdbPredicate, requestId);
    } else if (cmd.GetOprnType() == OperationType::STOP_GENERATE_THUMBNAILS) {
        thumbnailService_->CancelAstcBatchTask(requestId);
        return E_OK;
    } else if (cmd.GetOprnType() == OperationType::GENERATE_THUMBNAILS_RESTORE) {
        int32_t restoreAstcCount = 0;
        if (value.GetObject(RESTORE_REQUEST_ASTC_GENERATE_COUNT, valueObject)) {
            valueObject.GetInt(restoreAstcCount);
        }
        return thumbnailService_->RestoreThumbnailDualFrame(restoreAstcCount);
    } else if (cmd.GetOprnType() == OperationType::LOCAL_THUMBNAIL_GENERATION) {
        return thumbnailService_->LocalThumbnailGeneration();
    } else {
        MEDIA_ERR_LOG("invalid mediaLibrary command");
        return E_INVALID_ARGUMENTS;
    }
}

int32_t MediaLibraryDataManager::CheckCloudThumbnailDownloadFinish()
{
    CHECK_AND_RETURN_RET_LOG(thumbnailService_ != nullptr, E_THUMBNAIL_SERVICE_NULLPTR, "thumbanilService is nullptr");
    return thumbnailService_->CheckCloudThumbnailDownloadFinish();
}

void MediaLibraryDataManager::UploadDBFileInner(int64_t totalFileSize)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is nullptr!");
    std::string tmpPath = MEDIA_DB_DIR + "/rdb/media_library_tmp.db";
    int32_t errCode = rdbStore->Backup(tmpPath);
    CHECK_AND_RETURN_LOG(errCode == 0, "rdb backup fail: %{public}d", errCode);
    std::string destDbPath = "/data/storage/el2/log/logpack/media_library.db";
    if (totalFileSize < LARGE_FILE_SIZE_MB) {
        MediaFileUtils::CopyFileUtil(tmpPath, destDbPath);
        return;
    }

    std::string destPath = "/data/storage/el2/log/logpack/media_library.db.zip";
    int64_t begin = MediaFileUtils::UTCTimeMilliSeconds();
    std::string zipFileName = tmpPath;
    if (MediaFileUtils::IsFileExists(destPath)) {
        CHECK_AND_RETURN_LOG(MediaFileUtils::DeleteFile(destPath),
            "Failed to delete destDb file, path:%{private}s", destPath.c_str());
    }
    if (MediaFileUtils::IsFileExists(destDbPath)) {
        CHECK_AND_RETURN_LOG(MediaFileUtils::DeleteFile(destDbPath),
            "Failed to delete destDb file, path:%{private}s", destDbPath.c_str());
    }
    zipFile compressZip = Media::ZipUtil::CreateZipFile(destPath);
    if (compressZip == nullptr) {
        MEDIA_ERR_LOG("open zip file failed.");
        return;
    }
    auto errcode = Media::ZipUtil::AddFileInZip(compressZip, zipFileName, Media::KEEP_NONE_PARENT_PATH);
    if (errcode != 0) {
        MEDIA_ERR_LOG("AddFileInZip failed, errCode = %{public}d", errcode);
    }
    Media::ZipUtil::CloseZipFile(compressZip);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("Zip db file success, cost %{public}ld ms", (long)(end - begin));
}

void MediaLibraryDataManager::SubscriberPowerConsumptionDetection()
{
#ifdef DEVICE_STANDBY_ENABLE
    auto subscriber = new (std::nothrow) MediaLibraryStandbyServiceSubscriber();
    subscriber->SetSubscriberName(SUBSCRIBER_NAME);
    subscriber->SetModuleName(MODULE_NAME);
    DevStandbyMgr::StandbyServiceClient::GetInstance().SubscribeStandbyCallback(subscriber);
#endif
}

static int32_t SearchDateTakenWhenZero(const shared_ptr<MediaLibraryRdbStore> rdbStore, bool &needUpdate,
    unordered_map<string, string> &updateData)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_FAIL, "rdbStore is nullptr");
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_DATE_TAKEN, "0");
    vector<string> columns = {MediaColumn::MEDIA_ID, MediaColumn::MEDIA_DATE_MODIFIED};
    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "failed to acquire result from visitor query.");
    int32_t count;
    int32_t retCount = resultSet->GetRowCount(count);
    if (retCount != E_SUCCESS || count < 0) {
        return E_HAS_DB_ERROR;
    }
    CHECK_AND_RETURN_RET_LOG(count != 0, E_OK, "No dateTaken need to update");

    needUpdate = true;
    MEDIA_INFO_LOG("Have dateTaken need to update, count = %{public}d", count);
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId =
            get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_ID, resultSet, TYPE_INT32));
        int64_t newDateTaken =
            get<int64_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_DATE_MODIFIED, resultSet, TYPE_INT64));
        updateData.emplace(to_string(fileId), to_string(newDateTaken));
    }
    return E_OK;
}

int32_t MediaLibraryDataManager::UpdateDateTakenWhenZero()
{
    MEDIA_DEBUG_LOG("UpdateDateTakenWhenZero start");
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, E_FAIL, "rdbStore_ is nullptr");
    bool needUpdate = false;
    unordered_map<string, string> updateData;
    int32_t ret = SearchDateTakenWhenZero(rdbStore_, needUpdate, updateData);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "SerchDateTaken failed, ret = %{public}d", ret);
    CHECK_AND_RETURN_RET(needUpdate, E_OK);

    string updateSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + MediaColumn::MEDIA_DATE_TAKEN +
        " = " + PhotoColumn::MEDIA_DATE_MODIFIED + "," + PhotoColumn::PHOTO_DETAIL_TIME +
        " = strftime('%Y:%m:%d %H:%M:%S', date_modified/1000, 'unixepoch', 'localtime')" +
        " WHERE " + MediaColumn::MEDIA_DATE_TAKEN + " = 0";
    ret = rdbStore_->ExecuteSql(updateSql);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_HAS_DB_ERROR,
        "rdbStore->ExecuteSql failed, ret = %{public}d", ret);

    for (const auto& data : updateData) {
        ThumbnailService::GetInstance()->UpdateAstcWithNewDateTaken(data.first, data.second, "0");
    }
    MEDIA_DEBUG_LOG("UpdateDateTakenWhenZero end");
    return ret;
}

static int32_t DoUpdateBurstCoverLevelOperation(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const std::vector<std::string> &fileIdVec)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_FAIL, "rdbStore is nullptr");
    AbsRdbPredicates updatePredicates = AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    updatePredicates.In(MediaColumn::MEDIA_ID, fileIdVec);
    updatePredicates.BeginWrap();
    updatePredicates.EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL, WRONG_VALUE);
    updatePredicates.Or();
    updatePredicates.IsNull(PhotoColumn::PHOTO_BURST_COVER_LEVEL);
    updatePredicates.EndWrap();
    ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_BURST_COVER_LEVEL, static_cast<int32_t>(BurstCoverLevelType::COVER));

    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, values, updatePredicates);
    CHECK_AND_RETURN_RET_LOG((ret == E_OK && changedRows > 0), E_FAIL,
        "Failed to UpdateBurstCoverLevelFromGallery, ret: %{public}d, updateRows: %{public}d", ret, changedRows);
    MEDIA_INFO_LOG("UpdateBurstCoverLevelFromGallery success, changedRows: %{public}d, fileIdVec.size(): %{public}d.",
        changedRows, static_cast<int32_t>(fileIdVec.size()));
    return ret;
}

int32_t MediaLibraryDataManager::UpdateBurstCoverLevelFromGallery()
{
    MEDIA_INFO_LOG("UpdateBurstCoverLevelFromGallery start");
    CHECK_AND_RETURN_RET_LOG(refCnt_.load() > 0, E_FAIL, "MediaLibraryDataManager is not initialized");
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, E_FAIL, "rdbStore_ is nullptr");

    const std::vector<std::string> columns = { MediaColumn::MEDIA_ID };
    AbsRdbPredicates predicates = AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.BeginWrap();
    predicates.EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL, WRONG_VALUE);
    predicates.Or();
    predicates.IsNull(PhotoColumn::PHOTO_BURST_COVER_LEVEL);
    predicates.EndWrap();
    predicates.Limit(BATCH_QUERY_NUMBER);

    bool nextUpdate = true;
    while (nextUpdate && MedialibrarySubscriber::IsCurrentStatusOn()) {
        auto resultSet = rdbStore_->Query(predicates, columns);
        CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_FAIL, "Failed to query resultSet");
        int32_t rowCount = 0;
        int32_t ret = resultSet->GetRowCount(rowCount);
        CHECK_AND_RETURN_RET_LOG((ret == E_OK && rowCount >= 0), E_FAIL, "Failed to GetRowCount");
        if (rowCount == 0) {
            MEDIA_INFO_LOG("No need to UpdateBurstCoverLevelFromGallery.");
            return E_OK;
        }
        if (rowCount < BATCH_QUERY_NUMBER) {
            nextUpdate = false;
        }

        std::vector<std::string> fileIdVec;
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            std::string fileId = GetStringVal(MediaColumn::MEDIA_ID, resultSet);
            fileIdVec.push_back(fileId);
        }
        resultSet->Close();

        CHECK_AND_RETURN_RET_LOG(DoUpdateBurstCoverLevelOperation(rdbStore_, fileIdVec) == E_OK,
            E_FAIL, "Failed to DoUpdateBurstCoverLevelOperation");
    }
    return E_OK;
}
}  // namespace Media
}  // namespace OHOS

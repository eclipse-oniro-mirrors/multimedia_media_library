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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <regex>
#include <iomanip>
#include <dirent.h>
#include <chrono>
#include <thread>
#include <unistd.h>

#include "acl.h"
#include "albums_refresh_manager.h"
#include "asset_compress_version_manager.h"
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
#include "background_cloud_file_processor.h"
#endif
#include "background_task_mgr_helper.h"
#include "cloud_media_asset_manager.h"
#include "cloud_sync_switch_observer.h"
#include "dfx_anco_manager.h"
#include "dfx_manager.h"
#include "dfx_utils.h"
#include "ffrt_inner.h"
#include "location_column.h"
#include "map_operation_flag.h"
#include "media_analysis_helper.h"
#include "media_datashare_ext_ability.h"
#include "media_directory_type_column.h"
#include "media_file_utils.h"
#include "media_old_photos_column.h"
#include "media_old_albums_column.h"
#include "media_scanner_manager.h"
#include "media_smart_album_column.h"
#include "media_smart_map_column.h"
#include "media_visit_count_manager.h"
#include "medialibrary_analysis_album_operations.h"
#include "medialibrary_app_uri_permission_operations.h"
#include "medialibrary_app_uri_sensitive_operations.h"
#include "medialibrary_audio_operations.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_common_utils.h"
#include "medialibrary_dir_operations.h"
#include "medialibrary_file_operations.h"
#include "medialibrary_inotify.h"
#include "medialibrary_kvstore_manager.h"
#include "medialibrary_location_operations.h"
#include "medialibrary_meta_recovery.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_ptp_operations.h"
#include "medialibrary_restore.h"
#include "medialibrary_smartalbum_map_operations.h"
#include "medialibrary_smartalbum_operations.h"
#include "medialibrary_story_operations.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_tab_old_photos_operations.h"
#include "medialibrary_tab_old_albums_operations.h"
#include "medialibrary_tab_asset_and_album_operations.h"
#include "medialibrary_tracer.h"
#include "medialibrary_uripermission_operations.h"
#include "medialibrary_urisensitive_operations.h"
#include "medialibrary_vision_operations.h"
#include "medialibrary_search_operations.h"
#include "medialibrary_tab_asset_and_album_operations.h"
#include "mimetype_utils.h"
#include "multistages_capture_manager.h"
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
#include "enhancement_manager.h"
#endif
#include "permission_utils.h"
#include "photo_day_month_year_operation.h"
#include "photo_map_operations.h"
#include "power_efficiency_manager.h"
#include "preferences_helper.h"
#include "resource_type.h"
#include "result_set_utils.h"
#include "shooting_mode_column.h"
#include "trash_async_worker.h"
#include "photo_storage_operation.h"
#include "post_event_utils.h"
#include "medialibrary_formmap_operations.h"
#ifdef MEDIALIBRARY_FACARD_SUPPORT
#include "medialibrary_facard_operations.h"
#endif
#include "vision_db_sqls_more.h"
#include "parameters.h"
#include "uuid.h"
#ifdef DEVICE_STANDBY_ENABLE
#include "medialibrary_standby_service_subscriber.h"
#endif
#ifdef HAS_THERMAL_MANAGER_PART
#include "thermal_mgr_client.h"
#endif
#include "zip_util.h"
#include "photo_custom_restore_operation.h"
#include "vision_column.h"
#include "album_operation_uri.h"
#include "custom_record_operations.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_upgrade_utils.h"
#include "settings_data_manager.h"
#include "media_image_framework_utils.h"
#include "global_scanner.h"
#include "media_upgrade.h"

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
const int32_t UPDATE_BATCH_SIZE = 200;
const int32_t DELETE_BATCH_SIZE = 1000;
const int32_t PHOTO_CLOUD_POSITION = 2;
const int32_t PHOTO_LOCAL_CLOUD_POSITION = 3;
const int32_t UPDATE_DIRTY_CLOUD_CLONE_V1 = 1;
const int32_t UPDATE_DIRTY_CLOUD_CLONE_V2 = 2;
const int32_t ERROR_OLD_FILE_ID_OFFSET = -1000000;
constexpr int32_t DEFAULT_THUMBNAIL_SIZE = 256;
constexpr int32_t MAX_DEFAULT_THUMBNAIL_SIZE = 768;
constexpr double EPSILON_DOUBLE = 1e-9;
static const std::string TASK_PROGRESS_XML = "/data/storage/el2/base/preferences/task_progress.xml";
static const std::string NO_UPDATE_DIRTY = "no_update_dirty";
static const std::string NO_UPDATE_DIRTY_CLOUD_CLONE_V2 = "no_update_dirty_cloud_clone_v2";
static const std::string NO_DELETE_DIRTY_HDC_DATA = "no_delete_dirty_hdc_data";
static const std::string CLOUD_PREFIX_PATH = "/storage/cloud/files";
static const std::string THUMB_PREFIX_PATH = "/storage/cloud/files/.thumbs";
static const std::string COLUMN_OLD_FILE_ID = "old_file_id";
static const std::string NO_DELETE_DISK_DATA_INDEX = "no_delete_disk_data_index";
static const std::string NO_UPDATE_EDITDATA_SIZE = "no_update_editdata_size";
static const std::string UPDATE_EDITDATA_SIZE_COUNT = "update_editdata_size_count";
static const std::string BETA_DEBUG_DB_FILE_PATH = "/data/storage/el2/log/logpack/";
static constexpr int64_t MAX_DEBUG_DB_FILE_SIZE_BYTE = 1LL * 1024 * 1024 * 1024;
static int32_t g_updateBurstMaxId = 0;
static int32_t g_updateHdrModeId = -1;
static const std::string BROKER_ADD_MSG = "broker_add";
static const std::string BROKER_REMOVE_MSG = "broker_remove";
static const std::string BROKER_START_SCAN = "start_scan";
static const int MAX_LOOP_CNT = 10;
static const std::string MEDIA_LIBRARY_PREF_XML = "/data/storage/el2/base/preferences/media_library_preferences.xml";
static const std::string MEDIA_LIBRARY_RECOVERY_FLAG_KEY = "media_library_preferences_recovery_flag";

#ifdef DEVICE_STANDBY_ENABLE
static const std::string SUBSCRIBER_NAME = "POWER_USAGE";
static const std::string MODULE_NAME = "com.ohos.medialibrary.medialibrarydata";
#endif
static constexpr int ADD_ASYNC_TASK_SUCCESS = 0;

const std::vector<std::string> PRESET_ROOT_DIRS = {
    CAMERA_DIR_VALUES, VIDEO_DIR_VALUES, PIC_DIR_VALUES, AUDIO_DIR_VALUES,
    PHOTO_BUCKET + "/", AUDIO_BUCKET + "/", BACKUP_DATA_DIR_VALUE, EDIT_DATA_DIR_VALUE + "/",
    BACKUP_SINGLE_DATA_DIR_VALUE, CACHE_DIR_VALUE, CUSTOM_RESTORE_VALUES
};

const std::vector<std::string> E_POLICY_DIRS = {
    ROOT_MEDIA_DIR + CAMERA_DIR_VALUES,
    ROOT_MEDIA_DIR + VIDEO_DIR_VALUES,
    ROOT_MEDIA_DIR + PIC_DIR_VALUES,
    ROOT_MEDIA_DIR + PHOTO_BUCKET,
    ROOT_MEDIA_DIR + BACKUP_SINGLE_DATA_DIR_VALUE,
    ROOT_MEDIA_DIR + THUMB_DIR_VALUE,
    ROOT_MEDIA_DIR + CACHE_DIR_VALUE,
    ROOT_MEDIA_DIR + EDIT_DATA_DIR_VALUE,
};

MediaLibraryDataManager::MediaLibraryDataManager(void)
{
}

MediaLibraryDataManager::~MediaLibraryDataManager(void)
{
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
    CHECK_AND_RETURN_LOG(asyncWorker != nullptr, "Can not get asyncWorker");

    AsyncTaskData* taskData = new (std::nothrow) AsyncTaskData();
    CHECK_AND_RETURN_LOG(taskData != nullptr, "Failed to new taskData");

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
    MediaLibraryTracer tracer;
    tracer.Start("HandleOtherInitOperations");
    InitRefreshAlbum();
    UriPermissionOperations::DeleteAllTemporaryAsync();
    UriSensitiveOperations::DeleteAllSensitiveAsync();
}

static int32_t ExcuteAsyncWork()
{
    shared_ptr<MediaLibraryAsyncWorker> asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    CHECK_AND_RETURN_RET_LOG(asyncWorker != nullptr, E_ERR, "Can not get asyncWorker");

    AsyncTaskData* taskData = new (std::nothrow) AsyncTaskData();
    CHECK_AND_RETURN_RET_LOG(taskData != nullptr, E_ERR, "Failed to new taskData");

    taskData->dataDisplay = E_POLICY;
    shared_ptr<MediaLibraryAsyncTask> makeRootDirTask = make_shared<MediaLibraryAsyncTask>(MakeRootDirs, taskData);
    if (makeRootDirTask != nullptr) {
        asyncWorker->AddTask(makeRootDirTask, true);
    } else {
        MEDIA_WARN_LOG("Can not init make root dir task");
    }
    return E_OK;
}

static void ExecuteInitMimeTypeWork()
{
    std::thread([&]() {
        MEDIA_INFO_LOG("start ExecuteInitMimeTypeWork");
        MimeTypeUtils::InitMimeTypeMap();
    }).detach();
}

static void ExecuteInitThumnailWork()
{
    std::thread([&]() {
        MEDIA_INFO_LOG("start ExecuteInitThumnailWork");
        if (!MediaLibraryKvStoreManager::GetInstance().InitMonthAndYearKvStore(KvStoreRoleType::OWNER)) {
            MEDIA_ERR_LOG("failed at InitMonthAndYearKvStore");
        }
        if (Acl::AclSetDatabase() != E_OK) {
            MEDIA_ERR_LOG("Failed to set the acl db permission for the media db dir");
        }
    }).detach();
}

__attribute__((no_sanitize("cfi"))) int32_t MediaLibraryDataManager::InitMediaLibraryMgr(
    const shared_ptr<OHOS::AbilityRuntime::Context> &context,
    const shared_ptr<OHOS::AbilityRuntime::Context> &extensionContext, int32_t &sceneCode, bool isNeedCreateDir,
    bool isInMediaLibraryOnStart)
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
    if (access(KVDB_DIR.c_str(), F_OK) != E_OK) {
        ExecuteInitThumnailWork();
    } else {
        if (!MediaLibraryKvStoreManager::GetInstance().InitMonthAndYearKvStore(KvStoreRoleType::OWNER)) {
            MEDIA_ERR_LOG("failed at InitMonthAndYearKvStore");
        }
    }
    ExecuteInitMimeTypeWork();
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

    if (AlbumsRefreshManager::GetInstance().HasRefreshingSystemAlbums()) {
        SyncNotifyInfo info;
        info.forceRefreshType = ForceRefreshType::EXCEPTION;
        AlbumsRefreshManager::GetInstance().AddAlbumRefreshTask(info);
    }
    auto shareHelper = MediaLibraryHelperContainer::GetInstance()->GetDataShareHelper();
    cloudPhotoObserver_ = std::make_shared<CloudSyncObserver>();
    cloudPhotoAlbumObserver_ = std::make_shared<CloudSyncObserver>();
    galleryRebuildObserver_= std::make_shared<CloudSyncObserver>();
    cloudGalleryPhotoObserver_ = std::make_shared<CloudSyncObserver>();
    cloudGalleryDownloadObserver_ = std::make_shared<CloudSyncObserver>();
    shareHelper->RegisterObserverExt(Uri(PhotoColumn::PHOTO_CLOUD_URI_PREFIX), cloudPhotoObserver_, true);
    shareHelper->RegisterObserverExt(
        Uri(PhotoColumn::PHOTO_CLOUD_GALLERY_REBUILD_URI_PREFIX),
        galleryRebuildObserver_, true);
    shareHelper->RegisterObserverExt(Uri(PhotoAlbumColumns::ALBUM_GALLERY_CLOUD_URI_PREFIX),
        cloudPhotoAlbumObserver_, true);
    shareHelper->RegisterObserverExt(Uri(PhotoAlbumColumns::PHOTO_GALLERY_CLOUD_SYNC_INFO_URI_PREFIX),
        cloudPhotoAlbumObserver_, true);
    shareHelper->RegisterObserverExt(Uri(PhotoColumn::PHOTO_GALLERY_CLOUD_URI_PREFIX),
        cloudGalleryPhotoObserver_, true);
    shareHelper->RegisterObserverExt(Uri(PhotoAlbumColumns::PHOTO_GALLERY_DOWNLOAD_URI_PREFIX),
        cloudGalleryDownloadObserver_, true);

    HandleUpgradeRdbAsync(isInMediaLibraryOnStart);
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

static void FillMediaSuffixForHistoryData(const shared_ptr<MediaLibraryRdbStore>& store)
{
    MEDIA_INFO_LOG("start to fill media suffix for history data");

    // Calculate the substring after the last dot in the display_name. If there is no dot, return an empty string.
    const string calculatedSuffix = "CASE WHEN (INSTR(display_name, '.') > 0) THEN "
        "REPLACE(display_name, RTRIM(display_name, REPLACE(display_name, '.', '')), '') ELSE '' END";
    const string sql = "UPDATE " + PhotoColumn::PHOTOS_TABLE +
                 " SET " + PhotoColumn::PHOTO_MEDIA_SUFFIX + " = " + calculatedSuffix +
                 " WHERE " + PhotoColumn::PHOTO_MEDIA_SUFFIX + " IS NULL";
    int ret = store->ExecuteSql(sql);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "FillMediaSuffixForHistoryData failed: execute sql failed");
    MEDIA_INFO_LOG("end fill media suffix for history data");
}

static void AddImageFaceTagIdIndex(const shared_ptr<MediaLibraryRdbStore> store)
{
    MEDIA_INFO_LOG("Adding TAG_ID index for VISION_IMAGE_FACE_TABLE");
    int ret = store->ExecuteSql(CREATE_IMAGE_FACE_TAG_ID_INDEX);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "AddImageFaceTagIdIndex failed: execute sql failed");
    MEDIA_INFO_LOG("end TAG_ID index for VISION_IMAGE_FACE_TABLE");
}

static void AddGroupTagIndex(const shared_ptr<MediaLibraryRdbStore>& store)
{
    MEDIA_INFO_LOG("start to add group tag index");

    int ret = store->ExecuteSql(CREATE_ANALYSIS_ALBUM_GROUP_TAG_INDEX);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "AddGroupTagIndex failed: execute sql failed");

    MEDIA_INFO_LOG("end add group tag index");
}

static void AddAnalysisPhotoMapAssetIndex(const shared_ptr<MediaLibraryRdbStore> store)
{
    MEDIA_INFO_LOG("Adding map_asset index for ANALYSIS_PHOTO_MAP");
    int ret = store->ExecuteSql(CREATE_ANALYSIS_PHOTO_MAP_MAP_ASSET_INDEX);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "AddAnalysisPhotoMapAssetIndex failed: execute sql failed");
    MEDIA_INFO_LOG("end map_asset index for ANALYSIS_PHOTO_MAP");
}

static void FixTabExtDirtyData(const shared_ptr<MediaLibraryRdbStore>& store)
{
    MEDIA_INFO_LOG("start to fix tab ext dirty data");
    std::string cleanExtDirtyDataSql = "DELETE FROM " + PhotoExtColumn::PHOTOS_EXT_TABLE +
                            " WHERE NOT EXISTS (" +
                            " SELECT 1 " +
                            " FROM " + PhotoColumn::PHOTOS_TABLE +
                            " WHERE " + PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_ID +
                            " = " + PhotoExtColumn::PHOTOS_EXT_TABLE + "." + PhotoExtColumn::PHOTO_ID +
                            ");";
    int ret = store->ExecuteSql(cleanExtDirtyDataSql);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "FixTabExtDirtyData failed: execute sql failed");
    MEDIA_INFO_LOG("end fix tab ext dirty data");
}

static void UpdateIsRectificationCover(const shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "RdbStore is null!");

    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.NotEqualTo(PhotoColumn::PHOTO_COVER_POSITION, 0);

    ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_IS_RECTIFICATION_COVER, 1);

    int32_t changedRows = 0;
    int32_t err = rdbStore->Update(changedRows, values, predicates);
    CHECK_AND_PRINT_LOG(err == NativeRdb::E_OK, "RdbStore Update is_rectification_cover failed, err: %{public}d", err);
}

static void FixOrientation180DirtyThumbnail(const shared_ptr<MediaLibraryRdbStore>& store)
{
    MEDIA_INFO_LOG("Start to fix dirty thumbnail");
    std::string sql =
        "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_THUMBNAIL_READY + " = 6" +
        " WHERE " + PhotoColumn::PHOTO_ORIENTATION + " = 180 AND " + MediaColumn::MEDIA_TYPE + " = 1";
    int ret = store->ExecuteSql(sql);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Execute sql failed");
    MEDIA_INFO_LOG("End fix dirty thumbnail");
}

static void AddShootingModeAlbumIndex(const shared_ptr<MediaLibraryRdbStore>& store)
{
    MEDIA_INFO_LOG("Start to add shooting mode album index");
    const vector<string> sqls = {
        PhotoUpgrade::CREATE_PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX,
        PhotoUpgrade::CREATE_PHOTO_BURST_MODE_ALBUM_INDEX,
        PhotoUpgrade::CREATE_PHOTO_FRONT_CAMERA_ALBUM_INDEX,
        PhotoUpgrade::CREATE_PHOTO_RAW_IMAGE_ALBUM_INDEX,
        PhotoUpgrade::CREATE_PHOTO_MOVING_PHOTO_ALBUM_INDEX,
    };
    for (const auto& sql : sqls) {
        int ret = store->ExecuteSql(sql);
        CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Execute sql failed");
    }
    MEDIA_INFO_LOG("End add shooting mode album index");
}

static void UpdateBurstModeAlbumIndex(const shared_ptr<MediaLibraryRdbStore>& store, int32_t version)
{
    MEDIA_INFO_LOG("Start to Update burst mode album index");
    const vector<string> sqls = {
        PhotoUpgrade::DROP_BURST_MODE_ALBUM_INDEX,
        PhotoUpgrade::CREATE_PHOTO_BURST_MODE_ALBUM_INDEX,
    };
    for (size_t i = 0; i < sqls.size(); i++) {
        int ret = store->ExecuteSql(sqls[i]);
        RdbUpgradeUtils::AddUpgradeDfxMessages(version, i, ret);
        CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Execute sql failed");
    }
    MEDIA_INFO_LOG("End Update burst mode album index");
}

static void DeleteDirtytagIdFromFaceTagTable(const shared_ptr<MediaLibraryRdbStore>& store,
    int32_t version, int32_t upgradedIndex)
{
    MEDIA_INFO_LOG("Start to update face tag table");
    std::string sql =
        "DELETE FROM " + VISION_FACE_TAG_TABLE +
        " WHERE " + TAG_ID + " IN ( SELECT DISTINCT " + VISION_FACE_TAG_TABLE + "." + TAG_ID + " FROM " +
        VISION_FACE_TAG_TABLE + " WHERE EXISTS ( SELECT 1 FROM " + VISION_VIDEO_FACE_TABLE + " WHERE " +
        VISION_FACE_TAG_TABLE + "." + TAG_ID + " = " + VISION_VIDEO_FACE_TABLE + "." + TAG_ID +
        " ) AND NOT EXISTS ( SELECT 1 FROM " + VISION_IMAGE_FACE_TABLE + " WHERE " + VISION_FACE_TAG_TABLE + "." +
        TAG_ID + " = " + VISION_IMAGE_FACE_TABLE + "." + TAG_ID + " ) )";
    int ret = store->ExecuteSql(sql);
    RdbUpgradeUtils::AddUpgradeDfxMessages(version, upgradedIndex, ret);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Execute sql failed");
    MEDIA_INFO_LOG("End update face tag table");
}

static void UpdateVideoFaceTagId(const shared_ptr<MediaLibraryRdbStore>& store,
    int32_t version, int32_t upgradedIndex)
{
    MEDIA_INFO_LOG("Start to update video face tagId");
    std::string sql =
        "UPDATE " + VISION_VIDEO_FACE_TABLE + " SET " + TAG_ID + " = -1 WHERE " + TAG_ID + " IN ( SELECT DISTINCT " +
        VISION_VIDEO_FACE_TABLE + "." + TAG_ID + " FROM " + VISION_VIDEO_FACE_TABLE +
        " WHERE NOT EXISTS ( SELECT 1 FROM " + VISION_FACE_TAG_TABLE + " WHERE " + VISION_FACE_TAG_TABLE + "." +
        TAG_ID + " = " + VISION_VIDEO_FACE_TABLE + "." + TAG_ID + " ) ) OR " + TAG_ID + " = -2";
    int ret = store->ExecuteSql(sql);
    RdbUpgradeUtils::AddUpgradeDfxMessages(version, upgradedIndex, ret);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Execute sql failed");
    MEDIA_INFO_LOG("End update video face tagId");
}

static void UpdateVideoTotalFaceId(const shared_ptr<MediaLibraryRdbStore>& store,
    int32_t version, int32_t upgradedIndex)
{
    MEDIA_INFO_LOG("Start to update face id of video total");
    std::string sql =
        "UPDATE " + VISION_VIDEO_TOTAL_TABLE + " SET " + FACE + " = ( SELECT CASE WHEN ser_info.has_ser > 0 AND " +
        " ser_info.no_ser = 0 THEN 3 WHEN ser_info.has_ser > 0 AND ser_info.no_ser > 0 THEN 4 ELSE 2 END FROM " +
        " ( SELECT " + VISION_VIDEO_FACE_TABLE + "." + FILE_ID + ", COUNT(CASE WHEN " + TAG_ID +
        " LIKE 'ser%' THEN 1 END) AS has_ser, " + " COUNT(CASE WHEN " + TAG_ID +
        " NOT LIKE 'ser%' THEN 1 END) AS no_ser FROM " + VISION_VIDEO_FACE_TABLE + " GROUP BY " + FILE_ID +
        ") AS ser_info WHERE ser_info." + FILE_ID + " = " + VISION_VIDEO_TOTAL_TABLE + "." + FILE_ID + " ) WHERE " +
        " EXISTS (SELECT 1 FROM " + VISION_VIDEO_FACE_TABLE + " WHERE " + VISION_VIDEO_FACE_TABLE + "." + FILE_ID +
        " = " + VISION_VIDEO_TOTAL_TABLE + "." + FILE_ID + ") AND " + VISION_VIDEO_TOTAL_TABLE + "." + FACE + " = 2";
    int ret = store->ExecuteSql(sql);
    RdbUpgradeUtils::AddUpgradeDfxMessages(version, upgradedIndex, ret);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Execute sql failed");
    MEDIA_INFO_LOG("End update face id of video total");
}

static void Update3DGSAlbumInternal(const shared_ptr<MediaLibraryRdbStore>& store)
{
    MEDIA_INFO_LOG("Start to Update 3DGS mode album");
    vector<string> albumIdsStr;
    int32_t albumId = -1;
    CHECK_AND_RETURN_LOG(MediaLibraryRdbUtils::QueryShootingModeAlbumIdByType(
        ShootingModeAlbumType::MP4_3DGS_ALBUM, albumId), "Failed to query 3DGS album id");
    albumIdsStr.push_back(to_string(albumId));
    MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(store, albumIdsStr);
    MEDIA_INFO_LOG("End Update 3DGS mode album");
}

static void InsertLabelAndFaceToAnalysisVideoTotalTable(const shared_ptr<MediaLibraryRdbStore>& store,
    int32_t version, int32_t upgradedIndex)
{
    const string sql = "INSERT INTO " + VISION_VIDEO_TOTAL_TABLE + " ( " + FILE_ID + ", " + STATUS + ", " + LABEL +
        ", " + FACE + " ) SELECT " + FILE_ID + ", " + STATUS + ", " + LABEL + ", " + FACE + " FROM " +
        VISION_TOTAL_TABLE + " WHERE " + FILE_ID + " IN ( SELECT " + FILE_ID + " FROM " + PHOTOS_TABLE +
        " WHERE media_type = 2 )";
    int ret = store->ExecuteSql(sql);
    RdbUpgradeUtils::AddUpgradeDfxMessages(version, upgradedIndex, ret);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Execute sql failed");
    MEDIA_INFO_LOG("End insert face and label to vision");
}

static void CheckFaceToAnalysisVideoTotalTable(const shared_ptr<MediaLibraryRdbStore>& store,
    int32_t version, int32_t upgradedIndex)
{
    const std::string checkFaceSql = "UPDATE " + VISION_VIDEO_TOTAL_TABLE + " SET " +
        FACE + " = 0, " + STATUS + " = CASE WHEN " + STATUS + " = 1 THEN 0 ELSE " + STATUS + " END " +
        "WHERE NOT EXISTS (" + "SELECT 1 FROM " + VISION_VIDEO_FACE_TABLE + " WHERE " + FILE_ID + " = " +
        VISION_VIDEO_TOTAL_TABLE + "." + FILE_ID + ");";
    int ret = store->ExecuteSql(checkFaceSql);
    RdbUpgradeUtils::AddUpgradeDfxMessages(version, upgradedIndex, ret);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Execute sql failed");
    MEDIA_INFO_LOG("End check face to vision");
}

static void CheckLabelToAnalysisVideoTotalTable(const shared_ptr<MediaLibraryRdbStore>& store,
    int32_t version, int32_t upgradedIndex)
{
    const std::string checkLabelSql = "UPDATE " + VISION_VIDEO_TOTAL_TABLE + " SET " +
        LABEL + " = 0, " + STATUS + " = CASE WHEN " + STATUS + " = 1 THEN 0 ELSE " + STATUS + " END " +
        "WHERE NOT EXISTS (" + "SELECT 1 FROM " + VISION_VIDEO_LABEL_TABLE + " WHERE " + FILE_ID + " = " +
        VISION_VIDEO_TOTAL_TABLE + "." + FILE_ID + ");";
    int ret = store->ExecuteSql(checkLabelSql);
    RdbUpgradeUtils::AddUpgradeDfxMessages(version, upgradedIndex, ret);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Execute sql failed");
    MEDIA_INFO_LOG("End check label to vision");
}

static void UpdateFaceToAnalysisVideoTotalTable(const shared_ptr<MediaLibraryRdbStore>& store,
    int32_t version, int32_t upgradedIndex)
{
    const string sql = "UPDATE " + VISION_VIDEO_TOTAL_TABLE + " SET " + FACE + " = 2 WHERE " + FACE + " = 1";
    int ret = store->ExecuteSql(sql);
    RdbUpgradeUtils::AddUpgradeDfxMessages(version, upgradedIndex, ret);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Execute sql failed");
    MEDIA_INFO_LOG("End upgrade face for video total");
}

static void UpdateLabelAndFaceToAnalysisTable(const shared_ptr<MediaLibraryRdbStore>& store,
    int32_t version, int32_t upgradedIndex)
{
    const string sql = "UPDATE " + VISION_TOTAL_TABLE + " SET " + LABEL + " = 0, " + FACE + " = 0 WHERE " +
        FILE_ID + " IN ( SELECT " + FILE_ID + " FROM " + PHOTOS_TABLE + " WHERE media_type = 2 )";
    int ret = store->ExecuteSql(sql);
    RdbUpgradeUtils::AddUpgradeDfxMessages(version, upgradedIndex, ret);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Execute sql failed");
    MEDIA_INFO_LOG("End upgrade face and label");
}

static void UpdateStatusToAnalysisTable(const shared_ptr<MediaLibraryRdbStore>& store,
    int32_t version, int32_t upgradedIndex)
{
    const string sql = "UPDATE " + VISION_TOTAL_TABLE + " SET " + STATUS + " = 0 WHERE " + FILE_ID +" IN (SELECT " +
        FILE_ID + " FROM " + PHOTOS_TABLE + " WHERE media_type = 2) AND " + STATUS + " = 1";
    int ret = store->ExecuteSql(sql);
    RdbUpgradeUtils::AddUpgradeDfxMessages(version, upgradedIndex, ret);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Execute sql failed");
    MEDIA_INFO_LOG("End upgrade status of video data");
}


static void AddHistoryPanoramaModeAlbumData(const shared_ptr<MediaLibraryRdbStore>& store)
{
    MEDIA_INFO_LOG("Start to add panorama mode album data");
    const string panoramaModeAlbumType = to_string(static_cast<int32_t>(ShootingModeAlbumType::PANORAMA_MODE));
    const string sql = "UPDATE Photos SET shooting_mode = " + panoramaModeAlbumType +
        " WHERE shooting_mode_tag IN (" + CAMERA_CUSTOM_SM_PANORAMA +
        ", " + CAMERA_CUSTOM_SM_PHOTO_STITCHING + ")";
    int ret = store->ExecuteSql(sql);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Execute sql failed");
    MEDIA_INFO_LOG("End add panorama mode album data");
}

static void AddHistoryAIHighPixelModeAlbumData(const shared_ptr<MediaLibraryRdbStore>& store)
{
    MEDIA_INFO_LOG("Start to add ai high pixel mode album data");
    const string highPixelModeAlbumType = to_string(static_cast<int32_t>(ShootingModeAlbumType::HIGH_PIXEL));
    const string sql = "UPDATE Photos SET shooting_mode = " + highPixelModeAlbumType +
        " WHERE shooting_mode_tag = " + AI_HIGH_PIXEL_TAG;
    int ret = store->ExecuteSql(sql);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Execute sql failed");
    MEDIA_INFO_LOG("End add ai high pixel mode album data");
}

static void UpdateAllShootingModeAlbums(const shared_ptr<MediaLibraryRdbStore>& rdbStore)
{
    vector<int32_t> albumIds;
    vector<string> albumIdsStr;
    CHECK_AND_RETURN_LOG(MediaLibraryRdbUtils::QueryAllShootingModeAlbumIds(albumIds),
        "Failed to query shooting mode album ids");
    for (auto albumId : albumIds) {
        albumIdsStr.push_back(to_string(albumId));
    }

    MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(rdbStore, albumIdsStr);
}

static void UpdateQuickCaptureAndTimeLapseAlbums(const shared_ptr<MediaLibraryRdbStore>& rdbStore)
{
    vector<string> albumIdsStr;
    vector<ShootingModeAlbumType> albumTypes;
    albumTypes.push_back(ShootingModeAlbumType::TIME_LAPSE);
    albumTypes.push_back(ShootingModeAlbumType::QUICK_CAPTURE_ALBUM);

    for (const auto& type : albumTypes) {
        int32_t albumId;
        if (MediaLibraryRdbUtils::QueryShootingModeAlbumIdByType(type, albumId)) {
            albumIdsStr.push_back(to_string(albumId));
        }
    }

    MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(rdbStore, albumIdsStr);
}

static void SetExifRotateAfterAddColumn(const shared_ptr<MediaLibraryRdbStore>& store)
{
    MEDIA_INFO_LOG("Start to set exif rotate");
    std::string sql =
        "UPDATE " + PhotoColumn::PHOTOS_TABLE +
        " SET " + PhotoColumn::PHOTO_EXIF_ROTATE + " = " +
        " CASE " +
            " WHEN exif_rotate = 0 AND media_type = 1 AND orientation = 90 THEN 6 " +
            " WHEN exif_rotate = 0 AND media_type = 1 AND orientation = 180 THEN 3 " +
            " WHEN exif_rotate = 0 AND media_type = 1 AND orientation = 270 THEN 8 "
            " ELSE exif_rotate " +
        " END ";
    int ret = store->ExecuteSql(sql);
    MEDIA_INFO_LOG("End set exif rotate");
}

static void AsyncUpgradeFromAllVersionFirstPart(const shared_ptr<MediaLibraryRdbStore>& rdbStore)
{
    MEDIA_INFO_LOG("Start VERSION_ADD_DETAIL_TIME");
    int32_t errCode = 0;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(RDB_FIX_RECORDS, errCode);
    if (prefs != nullptr) {
        int32_t detailTimeFixed = prefs->GetInt(DETAIL_TIME_FIXED, 0);
        MEDIA_INFO_LOG("prefs current detailTimeFixed: %{public}d", detailTimeFixed);
        if (detailTimeFixed == NEED_FIXED) {
            MediaLibraryRdbStore::UpdateDateTakenToMillionSecond(rdbStore);
            MediaLibraryRdbStore::UpdateDateTakenIndex(rdbStore);
            ThumbnailService::GetInstance()->AstcChangeKeyFromDateAddedToDateTaken();
            prefs->PutInt(DETAIL_TIME_FIXED, ALREADY_FIXED);
            prefs->FlushSync();
            MEDIA_INFO_LOG("detailTimeFixed set to: %{public}d", ALREADY_FIXED);
        }
        MEDIA_INFO_LOG("prefs errCode: %{public}d", errCode);
    }
    MEDIA_INFO_LOG("End VERSION_ADD_DETAIL_TIME");

    MEDIA_INFO_LOG("Start VERSION_ADD_INDEX_FOR_FILEID");
    MediaLibraryRdbStore::AddIndexForFileIdAsync(rdbStore);
    MEDIA_INFO_LOG("End VERSION_ADD_INDEX_FOR_FILEID");

    MEDIA_INFO_LOG("Start VERSION_UPDATE_INDEX_FOR_COVER");
    MediaLibraryRdbStore::UpdateIndexForCover(rdbStore);
    MEDIA_INFO_LOG("End VERSION_UPDATE_INDEX_FOR_COVER");

    MEDIA_INFO_LOG("Start VERSION_ADD_THUMBNAIL_VISIBLE");
    if (prefs != nullptr) {
        int32_t thumbnailVisibleFixed = prefs->GetInt(THUMBNAIL_VISIBLE_FIXED, 0);
        MEDIA_INFO_LOG("prefs current thumbnailVisibleFixed: %{public}d", thumbnailVisibleFixed);
        if (thumbnailVisibleFixed == NEED_FIXED) {
            MediaLibraryRdbStore::UpdateThumbnailVisibleAndIdx(rdbStore);
            prefs->PutInt(THUMBNAIL_VISIBLE_FIXED, ALREADY_FIXED);
            prefs->FlushSync();
            MEDIA_INFO_LOG("thumbnailVisibleFixed set to: %{public}d", ALREADY_FIXED);
        }
        MEDIA_INFO_LOG("prefs errCode: %{public}d", errCode);
    }
    MEDIA_INFO_LOG("End VERSION_ADD_THUMBNAIL_VISIBLE");

    MEDIA_INFO_LOG("Start VERSION_UPDATE_DATETAKEN_AND_DETAILTIME");
    MediaLibraryRdbStore::UpdateDateTakenAndDetalTime(rdbStore);
    MEDIA_INFO_LOG("End VERSION_UPDATE_DATETAKEN_AND_DETAILTIME");

    MEDIA_INFO_LOG("Start VERSION_ADD_READY_COUNT_INDEX");
    MediaLibraryRdbStore::AddReadyCountIndex(rdbStore);
    MEDIA_INFO_LOG("End VERSION_ADD_READY_COUNT_INDEX");
}

static void AsyncUpgradeFromAllVersionSecondPart(const shared_ptr<MediaLibraryRdbStore>& rdbStore)
{
    MEDIA_INFO_LOG("Start VERSION_REVERT_FIX_DATE_ADDED_INDEX");
    MediaLibraryRdbStore::RevertFixDateAddedIndex(rdbStore);
    MEDIA_INFO_LOG("End VERSION_REVERT_FIX_DATE_ADDED_INDEX");

    MEDIA_INFO_LOG("Start VERSION_ADD_ALBUM_INDEX");
    MediaLibraryRdbStore::AddAlbumIndex(rdbStore);
    MEDIA_INFO_LOG("End VERSION_ADD_ALBUM_INDEX");

    MEDIA_INFO_LOG("Start VERSION_ADD_PHOTO_DATEADD_INDEX");
    MediaLibraryRdbStore::AddPhotoDateAddedIndex(rdbStore);
    MEDIA_INFO_LOG("End VERSION_ADD_PHOTO_DATEADD_INDEX");

    MEDIA_INFO_LOG("Start VERSION_REFRESH_PERMISSION_APPID");
    MediaLibraryRdbUtils::TransformAppId2TokenId(rdbStore);
    MEDIA_INFO_LOG("End VERSION_REFRESH_PERMISSION_APPID");

    MEDIA_INFO_LOG("Start VERSION_ADD_CLOUD_ENHANCEMENT_ALBUM_INDEX");
    MediaLibraryRdbStore::AddCloudEnhancementAlbumIndex(rdbStore);
    MEDIA_INFO_LOG("End VERSION_ADD_CLOUD_ENHANCEMENT_ALBUM_INDEX");

    MEDIA_INFO_LOG("Start VERSION_UPDATE_PHOTOS_DATE_AND_IDX");
    PhotoDayMonthYearOperation::UpdatePhotosDateAndIdx(rdbStore);
    MEDIA_INFO_LOG("End VERSION_UPDATE_PHOTOS_DATE_AND_IDX");

    MEDIA_INFO_LOG("Start VERSION_UPDATE_PHOTOS_DATE_IDX");
    PhotoDayMonthYearOperation::UpdatePhotosDateIdx(rdbStore);
    MEDIA_INFO_LOG("End VERSION_UPDATE_PHOTOS_DATE_IDX");

    MEDIA_INFO_LOG("Start VERSION_UPDATE_MEDIA_TYPE_AND_THUMBNAIL_READY_IDX");
    MediaLibraryRdbStore::UpdateMediaTypeAndThumbnailReadyIdx(rdbStore);
    MEDIA_INFO_LOG("End VERSION_UPDATE_MEDIA_TYPE_AND_THUMBNAIL_READY_IDX");

    MEDIA_INFO_LOG("Start VERSION_UPDATE_LOCATION_KNOWLEDGE_INDEX");
    MediaLibraryRdbStore::UpdateLocationKnowledgeIdx(rdbStore);
    MEDIA_INFO_LOG("End VERSION_UPDATE_LOCATION_KNOWLEDGE_INDEX");

    MEDIA_INFO_LOG("Start VERSION_ADD_ALBUM_SUBTYPE_AND_NAME_INDEX");
    MediaLibraryRdbStore::AddAlbumSubtypeAndNameIdx(rdbStore);
    MEDIA_INFO_LOG("End VERSION_ADD_ALBUM_SUBTYPE_AND_NAME_INDEX");
}

static void UpdatePhotoChangeTime(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore, int32_t version,
    int32_t index)
{
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "RdbStore is null!");

    MEDIA_INFO_LOG("Start updating photo change time");
    const string sql = "UPDATE " + PhotoColumn::PHOTOS_TABLE +
        " SET " + PhotoColumn::PHOTO_CHANGE_TIME + " = " + PhotoColumn::MEDIA_DATE_MODIFIED;
    int ret = rdbStore->ExecuteSql(sql);
    RdbUpgradeUtils::AddUpgradeDfxMessages(version, index, ret);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Execute sql failed");
    MEDIA_INFO_LOG("End updating photo change time");
}

static void UpdatePhotoAlbumChangeTime(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore, int32_t version,
    int32_t index)
{
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "RdbStore is null!");

    MEDIA_INFO_LOG("Start updating photo album change time");
    const string sql = "UPDATE " + PhotoAlbumColumns::TABLE +
        " SET " + PhotoAlbumColumns::CHANGE_TIME + " = " + PhotoAlbumColumns::ALBUM_DATE_MODIFIED;
    int ret = rdbStore->ExecuteSql(sql);
    RdbUpgradeUtils::AddUpgradeDfxMessages(version, index, ret);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Execute sql failed");
    MEDIA_INFO_LOG("End updating photo album change time");
}

static void FillSouthDeviceType(const shared_ptr<MediaLibraryRdbStore>& rdbStore, int32_t version)
{
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "RdbStore is null!");

    auto southDeviceType = SouthDeviceType::SOUTH_DEVICE_CLOUD;
    auto switchStatus = SettingsDataManager::GetPhotosSyncSwitchStatus();
    if (switchStatus == SwitchStatus::HDC) {
        southDeviceType = SouthDeviceType::SOUTH_DEVICE_HDC;
    }

    MEDIA_INFO_LOG("Start updating south_device_type for photos stored in cloud space");
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::CLOUD))
        ->Or()
        ->EqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
    ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE, static_cast<int32_t>(southDeviceType));
    int32_t changedRows = 0;
    int32_t ret = rdbStore->Update(changedRows, values, predicates);
    RdbUpgradeUtils::AddUpgradeDfxMessages(version, 0, ret);
    MEDIA_INFO_LOG("Update south_device_type for %{public}d photos in cloud space, ret: %{public}d", changedRows, ret);
}

static void UpdatePhotoAlbumHidden(const shared_ptr<MediaLibraryRdbStore>& rdbStore, int32_t version)
{
    MEDIA_INFO_LOG("Start to Update photoalbum hidden column");
    std::string sql =
        "UPDATE " + PhotoAlbumColumns::TABLE +
        " SET " + PhotoAlbumColumns::ALBUM_HIDDEN + " = 1" +
        " WHERE " + PhotoAlbumColumns::ALBUM_TYPE + " = " + std::to_string(PhotoAlbumType::SOURCE) +
        " AND " + PhotoAlbumColumns::ALBUM_SUBTYPE + " = " + std::to_string(PhotoAlbumSubType::SOURCE_GENERIC) +
        " AND " + PhotoAlbumColumns::ALBUM_COUNT + " = 0";
    int ret = rdbStore->ExecuteSql(sql);
    RdbUpgradeUtils::AddUpgradeDfxMessages(version, 0, ret);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Execute sql failed");
    MEDIA_INFO_LOG("End Update photoalbum hidden column");
}

void HandleUpgradeRdbAsyncPart6(const shared_ptr<MediaLibraryRdbStore> rdbStore, int32_t oldVersion)
{
    if (oldVersion < VERSION_ADD_CHANGE_TIME &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_CHANGE_TIME, false)) {
        int32_t index = 0;
        UpdatePhotoChangeTime(rdbStore, VERSION_ADD_CHANGE_TIME, index++);
        UpdatePhotoAlbumChangeTime(rdbStore, VERSION_ADD_CHANGE_TIME, index++);
        rdbStore->SetOldVersion(VERSION_ADD_CHANGE_TIME);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_CHANGE_TIME, false);
    }

    if (oldVersion < VERSION_DROP_PHOTO_STATUS_FOR_SEARCH_INDEX &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_DROP_PHOTO_STATUS_FOR_SEARCH_INDEX, false)) {
        MediaLibraryRdbStore::DropPhotoStatusForSearchIndex(rdbStore, VERSION_DROP_PHOTO_STATUS_FOR_SEARCH_INDEX);
        rdbStore->SetOldVersion(VERSION_DROP_PHOTO_STATUS_FOR_SEARCH_INDEX);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_DROP_PHOTO_STATUS_FOR_SEARCH_INDEX, false);
    }
}

void HandleUpgradeRdbAsyncPart5(const shared_ptr<MediaLibraryRdbStore> rdbStore, int32_t oldVersion)
{
    if (oldVersion < VERSION_CREATE_VIDEO_FACE_TAG_ID_INDEX &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_CREATE_VIDEO_FACE_TAG_ID_INDEX, false)) {
        MediaLibraryRdbStore::AddVideoFaceTagIdIndex(rdbStore, VERSION_CREATE_VIDEO_FACE_TAG_ID_INDEX);
        rdbStore->SetOldVersion(VERSION_CREATE_VIDEO_FACE_TAG_ID_INDEX);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_CREATE_VIDEO_FACE_TAG_ID_INDEX, false);
    }

    if (oldVersion < VERSION_UPDATE_VIDEO_LABLE_FACE &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_UPDATE_VIDEO_LABLE_FACE, false)) {
        int32_t upgradedIndex = 0;
        InsertLabelAndFaceToAnalysisVideoTotalTable(rdbStore, VERSION_UPDATE_VIDEO_LABLE_FACE, upgradedIndex++);
        CheckFaceToAnalysisVideoTotalTable(rdbStore, VERSION_UPDATE_VIDEO_LABLE_FACE, upgradedIndex++);
        CheckLabelToAnalysisVideoTotalTable(rdbStore, VERSION_UPDATE_VIDEO_LABLE_FACE, upgradedIndex++);
        UpdateFaceToAnalysisVideoTotalTable(rdbStore, VERSION_UPDATE_VIDEO_LABLE_FACE, upgradedIndex++);
        UpdateLabelAndFaceToAnalysisTable(rdbStore, VERSION_UPDATE_VIDEO_LABLE_FACE, upgradedIndex++);
        UpdateStatusToAnalysisTable(rdbStore, VERSION_UPDATE_VIDEO_LABLE_FACE, upgradedIndex++);
        rdbStore->SetOldVersion(VERSION_UPDATE_VIDEO_LABLE_FACE);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_UPDATE_VIDEO_LABLE_FACE, false);
    }

    if (oldVersion < VERSION_UPDATE_VIDEO_FACE_TAGID &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_UPDATE_VIDEO_FACE_TAGID, false)) {
        int32_t upgradedIndex = 0;
        DeleteDirtytagIdFromFaceTagTable(rdbStore, VERSION_UPDATE_VIDEO_FACE_TAGID, upgradedIndex++);
        UpdateVideoFaceTagId(rdbStore, VERSION_UPDATE_VIDEO_FACE_TAGID, upgradedIndex++);
        UpdateVideoTotalFaceId(rdbStore, VERSION_UPDATE_VIDEO_FACE_TAGID, upgradedIndex++);
        rdbStore->SetOldVersion(VERSION_UPDATE_VIDEO_FACE_TAGID);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_UPDATE_VIDEO_FACE_TAGID, false);
    }

    if (oldVersion < VERSION_ADD_QUICK_CAPTURE_AND_TIME_LAPSE &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_QUICK_CAPTURE_AND_TIME_LAPSE, false)) {
        UpdateQuickCaptureAndTimeLapseAlbums(rdbStore);
        rdbStore->SetOldVersion(VERSION_ADD_QUICK_CAPTURE_AND_TIME_LAPSE);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_QUICK_CAPTURE_AND_TIME_LAPSE, false);
    }

    if (oldVersion < VERSION_ADD_PET_TABLES &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_PET_TABLES, false)) {
        MediaLibraryRdbStore::AddPetTagIdIndex(rdbStore, VERSION_ADD_PET_TABLES);
        rdbStore->SetOldVersion(VERSION_ADD_PET_TABLES);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_PET_TABLES, false);
    }

    if (oldVersion < VERSION_ADD_PHOTO_ALBUM_HIDDEN &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_PHOTO_ALBUM_HIDDEN, false)) {
        UpdatePhotoAlbumHidden(rdbStore, VERSION_ADD_PHOTO_ALBUM_HIDDEN);
        rdbStore->SetOldVersion(VERSION_ADD_PHOTO_ALBUM_HIDDEN);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_PHOTO_ALBUM_HIDDEN, false);
    }
    HandleUpgradeRdbAsyncPart6(rdbStore, oldVersion);
    // !! Do not add upgrade code here !!
}

void HandleUpgradeRdbAsyncPart4(const shared_ptr<MediaLibraryRdbStore> rdbStore, int32_t oldVersion)
{
    if (oldVersion < VERSION_ADD_INDEX_FOR_PHOTO_SORT_IN_ALBUM &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_INDEX_FOR_PHOTO_SORT_IN_ALBUM, false)) {
        MediaLibraryRdbStore::AddIndexForPhotoSortInAlbum(rdbStore, VERSION_ADD_INDEX_FOR_PHOTO_SORT_IN_ALBUM);
        rdbStore->SetOldVersion(VERSION_ADD_INDEX_FOR_PHOTO_SORT_IN_ALBUM);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_INDEX_FOR_PHOTO_SORT_IN_ALBUM, false);
    }

    if (oldVersion < VERSION_ADD_INDEX_FOR_CLOUD_AND_PITAYA &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_INDEX_FOR_CLOUD_AND_PITAYA, false)) {
        MediaLibraryRdbStore::AddIndexForCloudAndPitaya(rdbStore, VERSION_ADD_INDEX_FOR_CLOUD_AND_PITAYA);
        rdbStore->SetOldVersion(VERSION_ADD_INDEX_FOR_CLOUD_AND_PITAYA);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_INDEX_FOR_CLOUD_AND_PITAYA, false);
    }

    if (oldVersion < VERSION_ADD_3DGS_MODE &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_3DGS_MODE, false)) {
        UpdateBurstModeAlbumIndex(rdbStore, VERSION_ADD_3DGS_MODE);
        Update3DGSAlbumInternal(rdbStore);
        rdbStore->SetOldVersion(VERSION_ADD_3DGS_MODE);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_3DGS_MODE, false);
    }

    if (oldVersion < VERSION_UPGRADE_IDX_SCHPT_HIDDEN_TIME &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_UPGRADE_IDX_SCHPT_HIDDEN_TIME, false)) {
        MediaLibraryRdbStore::UpdateIndexHiddenTime(rdbStore, VERSION_UPGRADE_IDX_SCHPT_HIDDEN_TIME);
        rdbStore->SetOldVersion(VERSION_UPGRADE_IDX_SCHPT_HIDDEN_TIME);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_UPGRADE_IDX_SCHPT_HIDDEN_TIME, false);
    }

    if (oldVersion < VERSION_UPDATE_BURST_MODE_ALBUM_INDEX &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_UPDATE_BURST_MODE_ALBUM_INDEX, false)) {
        UpdateBurstModeAlbumIndex(rdbStore, VERSION_UPDATE_BURST_MODE_ALBUM_INDEX);
        rdbStore->SetOldVersion(VERSION_UPDATE_BURST_MODE_ALBUM_INDEX);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_UPDATE_BURST_MODE_ALBUM_INDEX, false);
    }

    if (oldVersion < VERSION_UPGRADE_IDX_SCHPT_DATE_ADDED &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_UPGRADE_IDX_SCHPT_DATE_ADDED, false)) {
        MediaLibraryRdbStore::UpdateIndexDateAdded(rdbStore, VERSION_UPGRADE_IDX_SCHPT_DATE_ADDED);
        rdbStore->SetOldVersion(VERSION_UPGRADE_IDX_SCHPT_DATE_ADDED);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_UPGRADE_IDX_SCHPT_DATE_ADDED, false);
    }
    HandleUpgradeRdbAsyncPart5(rdbStore, oldVersion);
    // !! Do not add upgrade code here !!
}

void HandleUpgradeRdbAsyncPart3(const shared_ptr<MediaLibraryRdbStore> rdbStore, int32_t oldVersion)
{
    if (oldVersion < VERSION_FIX_DB_UPGRADE_FROM_API18) {
        MEDIA_INFO_LOG("Start VERSION_ADD_GROUP_TAG_INDEX");
        AddGroupTagIndex(rdbStore);
        MEDIA_INFO_LOG("End VERSION_ADD_GROUP_TAG_INDEX");

        MEDIA_INFO_LOG("Start VERSION_IMAGE_FACE_TAG_ID_INDEX");
        AddImageFaceTagIdIndex(rdbStore);
        MEDIA_INFO_LOG("End VERSION_IMAGE_FACE_TAG_ID_INDEX");

        MEDIA_INFO_LOG("Start VERSION_ADD_INDEX_FOR_PHOTO_SORT");
        MediaLibraryRdbStore::AddPhotoSortIndex(rdbStore);
        MEDIA_INFO_LOG("End VERSION_ADD_INDEX_FOR_PHOTO_SORT");

        rdbStore->SetOldVersion(VERSION_FIX_DB_UPGRADE_FROM_API18);
    }

    if (oldVersion < VERSION_TRANSFER_OWNERAPPID_TO_TOKENID) {
        MediaLibraryRdbUtils::TransformOwnerAppIdToTokenId(rdbStore);
        rdbStore->SetOldVersion(VERSION_TRANSFER_OWNERAPPID_TO_TOKENID);
    }

    if (oldVersion < VERSION_ADD_EXIF_ROTATE_COLUMN_AND_SET_VALUE) {
        SetExifRotateAfterAddColumn(rdbStore);
        rdbStore->SetOldVersion(VERSION_ADD_EXIF_ROTATE_COLUMN_AND_SET_VALUE);
    }

    if (oldVersion < VERSION_FIX_DB_UPGRADE_TO_API20 &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_FIX_DB_UPGRADE_TO_API20, false)) {
        AsyncUpgradeFromAllVersionFirstPart(rdbStore);
        AsyncUpgradeFromAllVersionSecondPart(rdbStore);
        rdbStore->SetOldVersion(VERSION_FIX_DB_UPGRADE_TO_API20);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_FIX_DB_UPGRADE_TO_API20, false);
    }

    if (oldVersion < VERSION_ADD_SOUTH_DEVICE_TYPE &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_SOUTH_DEVICE_TYPE, false)) {
        FillSouthDeviceType(rdbStore, VERSION_ADD_SOUTH_DEVICE_TYPE);
        rdbStore->SetOldVersion(VERSION_ADD_SOUTH_DEVICE_TYPE);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_SOUTH_DEVICE_TYPE, false);
    }
    HandleUpgradeRdbAsyncPart4(rdbStore, oldVersion);
    // !! Do not add upgrade code here !!
}

void HandleUpgradeRdbAsyncPart2(const shared_ptr<MediaLibraryRdbStore> rdbStore, int32_t oldVersion)
{
    if (oldVersion < VERSION_FIX_DB_UPGRADE_FROM_API15) {
        MEDIA_INFO_LOG("Start VERSION_ANALYZE_PHOTOS");
        MediaLibraryRdbUtils::AnalyzePhotosData();
        MEDIA_INFO_LOG("End VERSION_ANALYZE_PHOTOS");

        MEDIA_INFO_LOG("Start VERSION_ADD_MEDIA_SUFFIX_COLUMN");
        FillMediaSuffixForHistoryData(rdbStore);
        MEDIA_INFO_LOG("End VERSION_ADD_MEDIA_SUFFIX_COLUMN");

        rdbStore->SetOldVersion(VERSION_FIX_DB_UPGRADE_FROM_API15);
    }

    if (oldVersion < VERSION_FIX_TAB_EXT_DIRTY_DATA) {
        FixTabExtDirtyData(rdbStore);
        rdbStore->SetOldVersion(VERSION_FIX_TAB_EXT_DIRTY_DATA);
    }

    if (oldVersion < VERSION_ADD_IS_RECTIFICATION_COVER) {
        MEDIA_INFO_LOG("Start VERSION_ADD_IS_RECTIFICATION_COVER");
        UpdateIsRectificationCover(rdbStore);
        MEDIA_INFO_LOG("End VERSION_ADD_IS_RECTIFICATION_COVER");

        rdbStore->SetOldVersion(VERSION_ADD_IS_RECTIFICATION_COVER);
    }

    if (oldVersion < VERSION_FIX_ORIENTATION_180_DIRTY_THUMBNAIL) {
        FixOrientation180DirtyThumbnail(rdbStore);
        rdbStore->SetOldVersion(VERSION_FIX_ORIENTATION_180_DIRTY_THUMBNAIL);
    }

    if (oldVersion < VERSION_ADD_INDEX_FOR_PHOTO_SORT) {
        MediaLibraryRdbStore::AddPhotoSortIndex(rdbStore);
        rdbStore->SetOldVersion(VERSION_ADD_INDEX_FOR_PHOTO_SORT);
    }

    if (oldVersion < VERSION_ADD_PHOTO_QUERY_THUMBNAIL_WHITE_BLOCKS_INDEX) {
        MediaLibraryRdbStore::AddPhotoWhiteBlocksIndex(rdbStore);
        rdbStore->SetOldVersion(VERSION_ADD_PHOTO_QUERY_THUMBNAIL_WHITE_BLOCKS_INDEX);
    }

    if (oldVersion < VERSION_SHOOTING_MODE_ALBUM_SECOND_INTERATION) {
        AddShootingModeAlbumIndex(rdbStore);
        AddHistoryPanoramaModeAlbumData(rdbStore);
        AddHistoryAIHighPixelModeAlbumData(rdbStore);
        UpdateAllShootingModeAlbums(rdbStore);
        rdbStore->SetOldVersion(VERSION_SHOOTING_MODE_ALBUM_SECOND_INTERATION);
    }

    HandleUpgradeRdbAsyncPart3(rdbStore, oldVersion);
    // !! Do not add upgrade code here !!
}

void HandleUpgradeRdbAsyncPart1(const shared_ptr<MediaLibraryRdbStore> rdbStore, int32_t oldVersion)
{
    if (oldVersion < VERSION_FIX_PHOTO_QUALITY_CLONED) {
        MediaLibraryRdbStore::UpdatePhotoQualityCloned(rdbStore);
        rdbStore->SetOldVersion(VERSION_FIX_PHOTO_QUALITY_CLONED);
    }

    if (oldVersion < VERSION_ADD_MEDIA_SUFFIX_COLUMN) {
        FillMediaSuffixForHistoryData(rdbStore);
        rdbStore->SetOldVersion(VERSION_ADD_MEDIA_SUFFIX_COLUMN);
    }

    if (oldVersion < VERSION_UPDATE_LOCATION_KNOWLEDGE_INDEX) {
        MediaLibraryRdbStore::UpdateLocationKnowledgeIdx(rdbStore);
        rdbStore->SetOldVersion(VERSION_UPDATE_LOCATION_KNOWLEDGE_INDEX);
    }

    if (oldVersion < VERSION_IMAGE_FACE_TAG_ID_INDEX) {
        AddImageFaceTagIdIndex(rdbStore);
        rdbStore->SetOldVersion(VERSION_IMAGE_FACE_TAG_ID_INDEX);
    }

    if (oldVersion < VERSION_ADD_GROUP_TAG_INDEX) {
        AddGroupTagIndex(rdbStore);
        rdbStore->SetOldVersion(VERSION_ADD_GROUP_TAG_INDEX);
    }

    if (oldVersion < VERSION_ANALYZE_PHOTOS) {
        MediaLibraryRdbUtils::AnalyzePhotosData();
        rdbStore->SetOldVersion(VERSION_ANALYZE_PHOTOS);
    }

    if (oldVersion < VERSION_ADD_ANALYSIS_PHOTO_MAP_MAP_ASSET_INDEX) {
        AddAnalysisPhotoMapAssetIndex(rdbStore);
        rdbStore->SetOldVersion(VERSION_ADD_ANALYSIS_PHOTO_MAP_MAP_ASSET_INDEX);
    }

    if (oldVersion < VERSION_UPDATE_MDIRTY_TRIGGER_FOR_TDIRTY) {
        MediaLibraryRdbStore::UpdateMdirtyTriggerForTdirty(rdbStore);
        rdbStore->SetOldVersion(VERSION_UPDATE_MDIRTY_TRIGGER_FOR_TDIRTY);
    }

    if (oldVersion < VERSION_ADD_ALBUM_SUBTYPE_AND_NAME_INDEX) {
        MediaLibraryRdbStore::AddAlbumSubtypeAndNameIdx(rdbStore);
        rdbStore->SetOldVersion(VERSION_ADD_ALBUM_SUBTYPE_AND_NAME_INDEX);
    }

    HandleUpgradeRdbAsyncPart2(rdbStore, oldVersion);
    // !! Do not add upgrade code here !!
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
        MediaLibraryRdbUtils::TransformAppId2TokenId(rdbStore);
        rdbStore->SetOldVersion(VERSION_REFRESH_PERMISSION_APPID);
    }

    if (oldVersion < VERSION_UPDATE_PHOTOS_DATE_AND_IDX) {
        PhotoDayMonthYearOperation::UpdatePhotosDateAndIdx(rdbStore);
        rdbStore->SetOldVersion(VERSION_UPDATE_PHOTOS_DATE_AND_IDX);
    }

    if (oldVersion < VERSION_UPDATE_LATITUDE_AND_LONGITUDE_DEFAULT_NULL) {
        MediaLibraryRdbStore::UpdateLatitudeAndLongitudeDefaultNull(rdbStore);
        rdbStore->SetOldVersion(VERSION_UPDATE_LATITUDE_AND_LONGITUDE_DEFAULT_NULL);
    }

    if (oldVersion < VERSION_UPDATE_PHOTOS_DATE_IDX) {
        PhotoDayMonthYearOperation::UpdatePhotosDateIdx(rdbStore);
        rdbStore->SetOldVersion(VERSION_UPDATE_PHOTOS_DATE_IDX);
    }

    if (oldVersion < VERSION_UPDATE_MEDIA_TYPE_AND_THUMBNAIL_READY_IDX) {
        MediaLibraryRdbStore::UpdateMediaTypeAndThumbnailReadyIdx(rdbStore);
        rdbStore->SetOldVersion(VERSION_UPDATE_MEDIA_TYPE_AND_THUMBNAIL_READY_IDX);
    }

    HandleUpgradeRdbAsyncPart1(rdbStore, oldVersion);
    // !! Do not add upgrade code here !!
}

static void MultiStagesInitOperation()
{
    MultiStagesPhotoCaptureManager::GetInstance().Init();
    MultiStagesVideoCaptureManager::GetInstance().Init();
}

void UpgradeAsync(const shared_ptr<MediaLibraryRdbStore> rdbStore, int32_t oldVersion)
{
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
    if (oldVersion < VERSION_ADD_THUMBNAIL_VISIBLE) {
        MediaLibraryRdbStore::UpdateThumbnailVisibleAndIdx(rdbStore);
        rdbStore->SetOldVersion(VERSION_ADD_THUMBNAIL_VISIBLE);
    }
    if (oldVersion < VERSION_UPDATE_DATETAKEN_AND_DETAILTIME) {
        MediaLibraryRdbStore::UpdateDateTakenAndDetalTime(rdbStore);
        rdbStore->SetOldVersion(VERSION_UPDATE_DATETAKEN_AND_DETAILTIME);
    }

    HandleUpgradeRdbAsyncExtension(rdbStore, oldVersion);
    // !! Do not add upgrade code here !!
    rdbStore->SetOldVersion(MEDIA_RDB_VERSION);
}
void MediaLibraryDataManager::HandleUpgradeRdbAsync(bool isInMediaLibraryOnStart)
{
    std::thread([isInMediaLibraryOnStart] {
        if (isInMediaLibraryOnStart) {
            MultiStagesInitOperation();
        }

        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is nullptr!");
        int32_t oldVersion = rdbStore->GetOldVersion();
        int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
        if (oldVersion != -1 && oldVersion < MEDIA_RDB_VERSION) {
            UpgradeAsync(rdbStore, oldVersion);
        }
        // !! Do not add index here !!
        MediaLibraryRdbStore::AddUpgradeIndex(rdbStore);
        RdbUpgradeUtils::ReportUpgradeDfxMessages(startTime, oldVersion, MEDIA_RDB_VERSION,
            false);
    }).detach();
}

void MediaLibraryDataManager::InitResourceInfo()
{
    BackgroundTaskMgr::EfficiencyResourceInfo resourceInfo =
        BackgroundTaskMgr::EfficiencyResourceInfo(BackgroundTaskMgr::ResourceType::CPU, true, 0, "apply", true, true);
    BackgroundTaskMgr::BackgroundTaskMgrHelper::ApplyEfficiencyResources(resourceInfo);
}

__attribute__((no_sanitize("cfi"))) void MediaLibraryDataManager::ClearMediaLibraryMgr()
{
    lock_guard<shared_mutex> lock(mgrSharedMutex_);

    refCnt_--;
    if (refCnt_.load() > 0) {
        MEDIA_DEBUG_LOG("still other extension exist");
        return;
    }
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    BackgroundCloudFileProcessor::StopTimer();
#endif
    auto shareHelper = MediaLibraryHelperContainer::GetInstance()->GetDataShareHelper();
    CHECK_AND_RETURN_LOG(shareHelper != nullptr, "DataShareHelper is null");

    shareHelper->UnregisterObserverExt(Uri(PhotoColumn::PHOTO_CLOUD_URI_PREFIX), cloudPhotoObserver_);
    shareHelper->UnregisterObserverExt(Uri(PhotoColumn::PHOTO_CLOUD_GALLERY_REBUILD_URI_PREFIX), cloudPhotoObserver_);
    shareHelper->UnregisterObserverExt(Uri(PhotoColumn::PHOTO_GALLERY_CLOUD_URI_PREFIX), cloudPhotoAlbumObserver_);
    shareHelper->UnregisterObserverExt(
        Uri(PhotoAlbumColumns::ALBUM_GALLERY_CLOUD_URI_PREFIX), cloudPhotoAlbumObserver_);
    shareHelper->UnregisterObserverExt(
        Uri(PhotoAlbumColumns::PHOTO_GALLERY_CLOUD_SYNC_INFO_URI_PREFIX), cloudPhotoAlbumObserver_);
    shareHelper->UnregisterObserverExt(
        Uri(PhotoAlbumColumns::PHOTO_GALLERY_DOWNLOAD_URI_PREFIX), cloudGalleryDownloadObserver_);
    rdbStore_ = nullptr;
    MediaLibraryKvStoreManager::GetInstance().CloseAllKvStore();
    MEDIA_INFO_LOG("CloseKvStore success");

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
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "init MediaLibraryUnistoreManager failed");

    rdbStore_ = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, E_ERR, "rdbStore is nullptr");

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
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, E_ERR, "rdbStore_ is nullptr");

    auto queryResultSet = rdbStore_->QueryByStep(dirAbsPred, columns);
    CHECK_AND_RETURN_RET_LOG(queryResultSet != nullptr, E_ERR, "queryResultSet is nullptr");

    auto ret = queryResultSet->GetRowCount(count);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_ERR, "rdb failed");

    MEDIA_INFO_LOG("MakeDirQuerySetMap count = %{public}d", count);
    CHECK_AND_RETURN_RET_LOG(count != 0, E_ERR, "can not find any dirAsset");

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
        case OperationObject::PTP_ALBUM_OPERATION:
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
        case OperationObject::PAH_FORM_MAP: {
            if (MAP_OPERATION_FLAG) {
                MEDIA_INFO_LOG("map operation flag is true");
                return MediaLibraryFormMapOperations::HandleStoreFormIdOperation(cmd);
            }
            return E_OK;
        }

        case OperationObject::TAB_FACARD_PHOTO: {
#ifdef MEDIALIBRARY_FACARD_SUPPORT
            return MediaLibraryFaCardOperations::HandleStoreGalleryFormOperation(cmd);
#else
            MEDIA_ERR_LOG("unsupported OperationObject: %{public}d", cmd.GetOprnObject());
            return E_FAIL;
#endif
        }

        case OperationObject::SEARCH_TOTAL: {
            return MediaLibrarySearchOperations::InsertOperation(cmd);
        }
        case OperationObject::STORY_ALBUM:
        case OperationObject::STORY_COVER:
        case OperationObject::STORY_PLAY:
        case OperationObject::USER_PHOTOGRAPHY:
        case OperationObject::ANALYSIS_PROGRESS:
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
    CHECK_AND_RETURN_RET_LOG(isValid, E_ERR, "Invalid adapted value");

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
            if (cmd.GetOprnType() == OperationType::QUERY_ACTIVE_USER_ID) {
                solved = true;
                constexpr int32_t baseUserRange = 200000;
                uid_t activeUserId = getuid() / baseUserRange;
                cmd.SetResult(to_string(activeUserId));
                return E_OK;
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
    MEDIA_INFO_LOG("MediaLibraryDataManager Insert enter");
    if (refCnt_.load() <= 0) {
        MEDIA_DEBUG_LOG("MediaLibraryDataManager is not initialized");
        return E_FAIL;
    }
    // visit count
    if (cmd.GetUri().ToString().find(MEDIA_DATA_DB_THUMBNAIL) != string::npos &&
        cmd.GetUri().ToString().find(PhotoColumn::PHOTO_LCD_VISIT_COUNT) != string::npos) {
        auto fileId = cmd.GetOprnFileId();
        MediaVisitCountManager::AddVisitCount(MediaVisitCountManager::VisitCountType::PHOTO_LCD, std::move(fileId));
        return E_SUCCESS;
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
        CHECK_AND_PRINT_LOG(SetCmdBundleAndDevice(cmd) == ERR_OK,
            "MediaLibraryDataManager SetCmdBundleAndDevice failed.");
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
    CHECK_AND_RETURN_RET(thumbnailService_ != nullptr, E_THUMBNAIL_SERVICE_NULLPTR);
    int32_t result = E_FAIL;
    switch (cmd.GetOprnType()) {
        case OperationType::GENERATE:
            result = thumbnailService_->GenerateThumbnailBackground();
            break;
        case OperationType::AGING:
            result = thumbnailService_->LcdAging();
            break;
        default:
            MEDIA_ERR_LOG("bad operation type %{public}u", cmd.GetOprnType());
    }
    return result;
}

int32_t MediaLibraryDataManager::BatchInsert(MediaLibraryCommand &cmd, const vector<DataShareValuesBucket> &values)
{
    shared_lock<shared_mutex> sharedLock(mgrSharedMutex_);
    CHECK_AND_RETURN_RET_LOG(refCnt_.load() > 0, E_FAIL, "MediaLibraryDataManager is not initialized");

    string uriString = cmd.GetUri().ToString();
    if (uriString == UFM_PHOTO_ALBUM_ADD_ASSET || uriString == PAH_PHOTO_ALBUM_ADD_ASSET) {
        return PhotoMapOperations::AddPhotoAssets(values);
    } else if (cmd.GetOprnObject() == OperationObject::ANALYSIS_PHOTO_MAP) {
        return PhotoMapOperations::AddAnaLysisPhotoAssets(values);
    } else if (cmd.GetOprnObject() == OperationObject::ADD_ASSET_HIGHLIGHT_ALBUM) {
        return PhotoMapOperations::AddHighlightPhotoAssets(values);
    } else if (cmd.GetOprnObject() == OperationObject::APP_URI_PERMISSION_INNER) {
        int32_t ret = UriSensitiveOperations::GrantUriSensitive(cmd, values);
        CHECK_AND_RETURN_RET(ret >= 0, ret);
        return UriPermissionOperations::GrantUriPermission(cmd, values);
    } else if (cmd.GetOprnObject() == OperationObject::MEDIA_APP_URI_PERMISSION) {
        int32_t ret = MediaLibraryAppUriSensitiveOperations::BatchInsert(cmd, values);
        CHECK_AND_RETURN_RET(ret == MediaLibraryAppUriSensitiveOperations::SUCCEED, ret);
        CHECK_AND_RETURN_RET(!MediaLibraryAppUriSensitiveOperations::BeForceSensitive(cmd, values), ret);
        return MediaLibraryAppUriPermissionOperations::BatchInsert(cmd, values);
    } else if (cmd.GetOprnObject() == OperationObject::MTH_AND_YEAR_ASTC) {
        return AstcMthAndYearInsert(cmd, values);
    } else if (cmd.GetOprnObject() == OperationObject::CUSTOM_RECORDS_OPERATION) {
        return CustomRecordOperations::BatchAddCustomRecords(cmd, values);
    }
    if (uriString.find(MEDIALIBRARY_DATA_URI) == string::npos) {
        MEDIA_ERR_LOG("MediaLibraryDataManager BatchInsert: Input parameter is invalid");
        return E_INVALID_URI;
    }

    int insertResult = BatchInsertMediaAnalysisData(cmd, values);
    if (insertResult > 0) {
        return insertResult;
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
    if (MediaFileUtils::StartsWith(uriString, CustomRecordsColumns::CUSTOM_RECORDS_URI_PREFIX)) {
        NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicates,
            cmd.GetTableName());
        cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
        cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
        return MediaLibraryAppUriPermissionOperations::DeleteOperation(rdbPredicate);
    }
    CHECK_AND_RETURN_RET_LOG(uriString.find(MEDIALIBRARY_DATA_URI) != string::npos,
        E_INVALID_URI, "Not Data ability Uri");

    MediaLibraryTracer tracer;
    tracer.Start("CheckWhereClause");
    auto whereClause = predicates.GetWhereClause();
    CHECK_AND_RETURN_RET_LOG(MediaLibraryCommonUtils::CheckWhereClause(whereClause), E_SQL_CHECK_FAIL,
        "illegal query whereClause input %{private}s", whereClause.c_str());
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
            CHECK_AND_RETURN_RET_LOG(!fileAsset->GetRelativePath().empty(), E_DELETE_DENIED, "relative_path is empty");
            return (fileAsset->GetMediaType() != MEDIA_TYPE_ALBUM) ?
                MediaLibraryObjectUtils::DeleteFileObj(move(fileAsset)) :
                MediaLibraryObjectUtils::DeleteDirObj(move(fileAsset));
        }
        case OperationObject::PHOTO_ALBUM:
            return MediaLibraryAlbumOperations::DeletePhotoAlbum(rdbPredicate);
        case OperationObject::HIGHLIGHT_DELETE:
            return MediaLibraryAlbumOperations::DeleteHighlightAlbums(rdbPredicate);
        case OperationObject::PHOTO_MAP:
            return PhotoMapOperations::RemovePhotoAssets(rdbPredicate);
        case OperationObject::ANALYSIS_PHOTO_MAP: {
            if (CheckIsDismissAsset(rdbPredicate)) {
                return PhotoMapOperations::DismissAssets(rdbPredicate);
            }
            break;
        }
        case OperationObject::MEDIA_APP_URI_PERMISSION:
        case OperationObject::APP_URI_PERMISSION_INNER:
            return MediaLibraryAppUriPermissionOperations::DeleteOperation(rdbPredicate);
        case OperationObject::FILESYSTEM_PHOTO:
        case OperationObject::FILESYSTEM_AUDIO:
            return MediaLibraryAssetOperations::DeleteOperation(cmd);
        case OperationObject::PAH_FORM_MAP:
            return MediaLibraryFormMapOperations::RemoveFormIdOperations(rdbPredicate);

        case OperationObject::TAB_FACARD_PHOTO: {
#ifdef MEDIALIBRARY_FACARD_SUPPORT
            return MediaLibraryFaCardOperations::HandleRemoveGalleryFormOperation(rdbPredicate);
#else
            MEDIA_ERR_LOG("unsupported OperationObject: %{public}d", cmd.GetOprnObject());
            return E_FAIL;
#endif
        }

        case OperationObject::ASSET_ALBUM_OPERATION:
            return MediaLibraryTableAssetAlbumOperations::Delete(rdbPredicate);
        default:
            return DeleteInRdbPredicatesMore(cmd, rdbPredicate);
    }
    return DeleteInRdbPredicatesAnalysis(cmd, rdbPredicate);
}

int32_t MediaLibraryDataManager::DeleteInRdbPredicatesMore(MediaLibraryCommand &cmd,
    NativeRdb::RdbPredicates &rdbPredicate)
{
    switch (cmd.GetOprnObject()) {
        case OperationObject::PTP_OPERATION: {
            return MediaLibraryPtpOperations::DeletePtpPhoto(rdbPredicate);
        }
        case OperationObject::PTP_ALBUM_OPERATION: {
            return MediaLibraryPtpOperations::DeletePtpAlbum(rdbPredicate);
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
        case OperationObject::VISION_START ... OperationObject::VISION_END:
            return MediaLibraryVisionOperations::DeleteOperation(cmd);

        case OperationObject::GEO_DICTIONARY:
        case OperationObject::GEO_KNOWLEDGE:
        case OperationObject::GEO_PHOTO:
            return MediaLibraryLocationOperations::DeleteOperation(cmd);

        case OperationObject::STORY_ALBUM:
        case OperationObject::STORY_COVER:
        case OperationObject::STORY_PLAY:
        case OperationObject::USER_PHOTOGRAPHY:
        case OperationObject::ANALYSIS_PROGRESS:
            return MediaLibraryStoryOperations::DeleteOperation(cmd);

        case OperationObject::SEARCH_TOTAL:
            return MediaLibrarySearchOperations::DeleteOperation(cmd);
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
    CHECK_AND_RETURN_RET(uriParts.size() < MediaLibraryDataManager::URI_MIN_NUM,
        uriParts[uriParts.size() - MediaLibraryDataManager::URI_MIN_NUM]);
    return "";
}

static std::string BuildWhereClause(const std::vector<std::string>& dismissAssetArray, int32_t albumId)
{
    std::string whereClause = MediaColumn::MEDIA_ID + " IN (";

    for (size_t i = 0; i < dismissAssetArray.size(); ++i) {
        std::string fileId = ExtractFileIdFromUri(dismissAssetArray[i]);
        CHECK_AND_CONTINUE(!fileId.empty());
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

int MediaLibraryDataManager::HandleAnalysisFaceUpdate(MediaLibraryCommand& cmd, NativeRdb::ValuesBucket &value,
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
    CHECK_AND_RETURN_RET_LOG(!uris.empty(), E_INVALID_FILEID, "No URIs found after album ID.");

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
        case OperationObject::PTP_ALBUM_OPERATION:
            return MediaLibraryAlbumOperations::HandlePhotoAlbum(cmd.GetOprnType(), value, predicates);
        case OperationObject::GEO_DICTIONARY:
        case OperationObject::GEO_KNOWLEDGE:
            return MediaLibraryLocationOperations::UpdateOperation(cmd);
        case OperationObject::STORY_ALBUM:
        case OperationObject::STORY_COVER:
        case OperationObject::STORY_PLAY:
        case OperationObject::USER_PHOTOGRAPHY:
        case OperationObject::ANALYSIS_PROGRESS:
            return MediaLibraryStoryOperations::UpdateOperation(cmd);
        case OperationObject::PAH_MULTISTAGES_CAPTURE: {
            std::vector<std::string> columns;
            MultiStagesPhotoCaptureManager::GetInstance().HandleMultiStagesOperation(cmd, columns);
            return E_OK;
        }
        case OperationObject::PAH_MULTISTAGES_VIDEO: {
            std::vector<std::string> columns;
            MultiStagesVideoCaptureManager::GetInstance().HandleMultiStagesOperation(cmd, columns);
            return E_OK;
        }
        case OperationObject::PAH_BATCH_THUMBNAIL_OPERATE:
            return ProcessThumbnailBatchCmd(cmd, value, predicates);
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
        case OperationObject::PAH_CLOUD_ENHANCEMENT_OPERATE:
            return EnhancementManager::GetInstance().HandleEnhancementUpdateOperation(cmd);
#endif
        case OperationObject::VISION_IMAGE_FACE:
            return HandleAnalysisFaceUpdate(cmd, value, predicates);
        case OperationObject::ANALYSIS_PHOTO_MAP:
            if (cmd.GetOprnType() == OperationType::UPDATE_ORDER) {
                return MediaLibraryAnalysisAlbumOperations::SetAnalysisAlbumOrderPosition(cmd);
            }
            break;
        case OperationObject::PAH_BACKUP_POSTPROCESS:
            return RestoreInvalidHDCCloudDataPos();
            break;
        default:
            break;
    }
    // ModifyInfoByIdInDb can finish the default update of smartalbum and smartmap,
    // so no need to distinct them in switch-case deliberately
    cmd.SetValueBucket(value);
    return MediaLibraryObjectUtils::ModifyInfoByIdInDb(cmd);
}

static void RestoreInvalidatedPos(AsyncTaskData *data)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore, "rdbStore is nullptr");

    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int>(PhotoPositionType::INVALID));
    ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int>(PhotoPositionType::CLOUD));
    int32_t changedRows = -1;
    CHECK_AND_RETURN_LOG(rdbStore->Update(changedRows, values, predicates) == NativeRdb::E_OK,
        "fail to update invalid pos");
    MEDIA_INFO_LOG("RestoreInvalidHDCCloudDataPos, %{public}d rows updated", changedRows);
}

int32_t MediaLibraryDataManager::RestoreInvalidHDCCloudDataPos()
{
    shared_ptr<MediaLibraryAsyncWorker> asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    CHECK_AND_RETURN_RET_LOG(asyncWorker != nullptr, NativeRdb::E_ERROR, "Can not get asyncWorker");

    AsyncTaskData* taskData = new (std::nothrow) AsyncTaskData();
    CHECK_AND_RETURN_RET_LOG(taskData != nullptr, NativeRdb::E_ERROR, "Failed to allocate new taskData");

    shared_ptr<MediaLibraryAsyncTask> restoreInvalidPosTask = \
        make_shared<MediaLibraryAsyncTask>(RestoreInvalidatedPos, taskData);
    CHECK_AND_RETURN_RET_LOG(restoreInvalidPosTask, NativeRdb::E_ERROR, "fail to create medialibrary async task");
    int32_t ret = asyncWorker->AddTask(restoreInvalidPosTask, false);
    CHECK_AND_RETURN_RET_LOG(ret == ADD_ASYNC_TASK_SUCCESS, NativeRdb::E_ERROR,
        "fail to add restore-invalid-pos-task to asyncWorker, ret %{public}d", ret);
    return NativeRdb::E_OK;
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

int32_t MediaLibraryDataManager::RepairExifRotateBackground()
{
    shared_lock<shared_mutex> sharedLock(mgrSharedMutex_);
    CHECK_AND_RETURN_RET_LOG(refCnt_.load() > 0, E_FAIL, "MediaLibraryDataManager is not initialized");
    CHECK_AND_RETURN_RET_LOG(thumbnailService_ != nullptr, E_THUMBNAIL_SERVICE_NULLPTR, "ThumbnailService is nullptr");
    return thumbnailService_->RepairExifRotateBackground();
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
        CHECK_AND_CONTINUE(duration >= thresholdSeconds);
        CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteFile(file),
            "delete failed %{public}s, errno: %{public}d", file.c_str(), errno);
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
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_HAS_DB_ERROR,
        "Delete invalid album failed, ret = %{public}d, deleteRow is %{public}d", ret, deleteRow);
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

    PhotoCustomRestoreOperation::GetInstance().CleanTimeoutCustomRestoreTaskDir();

    ClearInvalidDeletedAlbum(); // Clear invalid album data with null cloudid and dirty '4'

    MediaLibraryTableAssetAlbumOperations().OprnTableOversizeChecker();

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

static shared_ptr<NativeRdb::ResultSet> QueryGenerateSql(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const string title, const int32_t ownerAlbumId)
{
    uint32_t index = title.find_first_of("BURST");
    CHECK_AND_RETURN_RET_LOG(index != string::npos, nullptr, "find BURST failed");
    string globMember = title.substr(0, index) + "BURST" + generateRegexpMatchForNumber(3);
    string globCover = globMember + "_COVER";
    string querySql = "SELECT " + PhotoColumn::PHOTO_BURST_KEY + " FROM " + PhotoColumn::PHOTOS_TABLE +
        " WHERE LOWER(" + MediaColumn::MEDIA_TITLE +
        ") GLOB LOWER('" + globCover + "') AND " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + " = " +
        to_string(ownerAlbumId);
    return rdbStore->QueryByStep(querySql);
}

static int32_t UpdateBurstPhoto(const bool isCover, const shared_ptr<MediaLibraryRdbStore> rdbStore,
    shared_ptr<NativeRdb::ResultSet> resultSet)
{
    int32_t count = 0;
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "resultSet is nullptr");
    int32_t retCount = resultSet->GetRowCount(count);
    CHECK_AND_RETURN_RET_LOG(retCount == E_SUCCESS && count >= 0, E_ERR, "Failed to GetRowCount");
    if (count == 0) {
        MEDIA_INFO_LOG("%{public}s", isCover ? "No burst cover need to update" : "No burst member need to update");
        return E_SUCCESS;
    }

    int32_t ret = E_SUCCESS;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        if (!isChargingAndScreenOffPtr()) {
            ret = E_ERR;
            MEDIA_ERR_LOG("current status is not charging or screenOn");
            break;
        }
        string title = GetStringVal(MediaColumn::MEDIA_TITLE, resultSet);
        int32_t ownerAlbumId = GetInt32Val(PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSet);
        if (!isCover) {
            auto generateResultSet = QueryGenerateSql(rdbStore, title, ownerAlbumId);
            CHECK_AND_CONTINUE_ERR_LOG(generateResultSet != nullptr, "generateResultSet is nullptr");
            if (generateResultSet->GoToFirstRow() != NativeRdb::E_OK) {
                MEDIA_INFO_LOG("No burst member need to query");
                generateResultSet->Close();
                continue;
            }
            generateResultSet->Close();
        }

        string updateSql = generateUpdateSql(isCover, title, ownerAlbumId);
        ret = rdbStore->ExecuteSql(updateSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("rdbStore->ExecuteSql failed, ret = %{public}d", ret);
            continue;
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

static int32_t QueryMaxFileId(const shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    MEDIA_INFO_LOG("Begin QueryMaxFileId");
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_FAIL, "rdbStore_ is nullptr");

    std::string querySql = "SELECT MAX(file_id) AS file_id FROM Photos";
    auto resultSet = rdbStore->QueryByStep(querySql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultSet is nullptr.");
        return g_updateBurstMaxId;
    }
    if (resultSet->GoToNextRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("resultSet is empty.");
        return g_updateBurstMaxId;
    }
    return GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
}

int32_t MediaLibraryDataManager::UpdateBurstFromGallery()
{
    MEDIA_INFO_LOG("Begin UpdateBurstFromGallery");
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryDataManager::UpdateBurstFromGallery");
    shared_lock<shared_mutex> sharedLock(mgrSharedMutex_);
    CHECK_AND_RETURN_RET_LOG(refCnt_.load() > 0, E_FAIL, "MediaLibraryDataManager is not initialized");
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, E_FAIL, "rdbStore_ is nullptr");

    int32_t lastMaxId = QueryMaxFileId(rdbStore_);
    if (lastMaxId <= g_updateBurstMaxId) {
        MEDIA_INFO_LOG("No need to update burst, lastMaxId: %{public}d, g_updateBurstMaxId: %{public}d.",
            lastMaxId, g_updateBurstMaxId);
        return E_SUCCESS;
    }
    MEDIA_INFO_LOG("Current lastMaxId: %{public}d, g_updateBurstMaxId: %{public}d.", lastMaxId, g_updateBurstMaxId);

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
    g_updateBurstMaxId = lastMaxId;
    MEDIA_INFO_LOG("End UpdateBurstFromGallery, g_updateBurstMaxId: %{public}d.", g_updateBurstMaxId);
    return ret;
}

int MediaLibraryDataManager::GetThumbnail(const string &uri)
{
    CHECK_AND_RETURN_RET(thumbnailService_ != nullptr, E_THUMBNAIL_SERVICE_NULLPTR);

    if (!uri.empty() && MediaLibraryObjectUtils::CheckUriPending(uri)) {
        MEDIA_ERR_LOG("failed to get thumbnail, the file:%{private}s is pending", uri.c_str());
        return E_FAIL;
    }
    return thumbnailService_->GetThumbnailFd(uri);
}

void MediaLibraryDataManager::CreateThumbnailAsync(const string &uri, const string &path,
    std::shared_ptr<Media::Picture> originalPhotoPicture)
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
        int32_t err = 0;
        if (originalPhotoPicture == nullptr) {
            err = thumbnailService_->CreateThumbnailFileScaned(uri, path);
        } else {
            err = thumbnailService_->CreateThumbnailFileScanedWithPicture(uri, path, originalPhotoPicture, false);
        }
        CHECK_AND_PRINT_LOG(err == E_SUCCESS, "ThumbnailService CreateThumbnailFileScaned failed : %{public}d", err);
    }
}

static shared_ptr<NativeRdb::ResultSet> HandleAnalysisAlbumQuery(MediaLibraryCommand &cmd,
    const vector<string> &columns, const DataSharePredicates &predicates)
{
    if (cmd.GetOprnType() == OperationType::QUERY_ORDER) {
        return MediaLibraryRdbStore::QueryWithFilter(RdbUtils::ToPredicates(predicates, cmd.GetTableName()), columns);
    }
    return PhotoMapOperations::QueryPhotoAssets(RdbUtils::ToPredicates(predicates, PhotoColumn::PHOTOS_TABLE), columns);
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

shared_ptr<NativeRdb::ResultSet> MediaLibraryDataManager::QueryAnalysisAlbum(MediaLibraryCommand &cmd,
    const vector<string> &columns, const DataSharePredicates &predicates)
{
    if (cmd.GetOprnType() == OperationType::QUERY_HIGHLIGHT_DIRECTORY_SIZE) {
        auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, nullptr, "uniStore is nullptr!");
        return PhotoStorageOperation().QueryHighlightDirectorySize(uniStore);
    }
    RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, cmd.GetTableName());
    int32_t albumSubtype = MediaLibraryRdbUtils::GetAlbumSubtypeArgument(rdbPredicates);
    MEDIA_DEBUG_LOG("Query analysis album of subtype: %{public}d", albumSubtype);
    if (cmd.GetOprnType() == OperationType::QUERY_RAW_ANALYSIS_ALBUM) {
        return MediaLibraryRdbStore::QueryWithFilter(rdbPredicates, columns);
    }
    if (albumSubtype == PhotoAlbumSubType::GROUP_PHOTO) {
        return MediaLibraryAnalysisAlbumOperations::QueryGroupPhotoAlbum(cmd, columns);
    }
    if (CheckIsPortraitAlbum(cmd)) {
        return MediaLibraryAlbumOperations::QueryPortraitAlbum(cmd, columns);
    }
    return MediaLibraryRdbStore::QueryWithFilter(rdbPredicates, columns);
}

inline bool CheckLatitudeAndLongitudeVal(double latitudeVal, double longitudeVal)
{
    return !(std::abs(latitudeVal) < EPSILON_DOUBLE && std::abs(longitudeVal) < EPSILON_DOUBLE);
}

inline bool CheckLatitudeAndLongitude(const string &latitude, const string &longitude)
{
    return latitude != "" && longitude != "" && !(latitude == "0" && longitude == "0");
}

static string ConvertDoubleToString(double value)
{
    const int precision = 17;
    std::ostringstream stringStream;
    stringStream << std::setprecision(precision) << std::fixed  << value; //
    return stringStream.str();
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryDataManager::QueryGeo(const RdbPredicates &rdbPredicates,
    const vector<string> &columns)
{
    auto queryResult = MediaLibraryRdbStore::QueryWithFilter(rdbPredicates, columns);
    CHECK_AND_RETURN_RET_LOG(queryResult != nullptr, queryResult,
        "Query Geographic Information Failed, queryResult is nullptr");

    const vector<string> &whereArgs = rdbPredicates.GetWhereArgs();
    bool cond = (whereArgs.empty() || whereArgs.front().empty());
    CHECK_AND_RETURN_RET_LOG(!cond, queryResult, "Query Geographic Information can not get fileId");

    string fileId = whereArgs.front();
    CHECK_AND_RETURN_RET_LOG(queryResult->GoToNextRow() == NativeRdb::E_OK, queryResult,
        "Query Geographic Information Failed, fileId: %{public}s", fileId.c_str());

    double latitudeVal = GetDoubleVal(PhotoColumn::PHOTOS_TABLE + "." + LATITUDE, queryResult);
    double longitudeVal = GetDoubleVal(PhotoColumn::PHOTOS_TABLE + "." + LONGITUDE, queryResult);
    string addressDescription = GetStringVal(ADDRESS_DESCRIPTION, queryResult);
    MEDIA_INFO_LOG(
        "QueryGeo, fileId: %{public}s, latitude: %{private}f, longitude: %{private}f, addressDescription: %{private}s",
        fileId.c_str(), latitudeVal, longitudeVal, addressDescription.c_str());

    if (CheckLatitudeAndLongitudeVal(latitudeVal, longitudeVal) && addressDescription.empty()) {
        string latitude = ConvertDoubleToString(latitudeVal);
        string longitude = ConvertDoubleToString(longitudeVal);
        CHECK_AND_RETURN_RET(latitude != "" && longitude != "", queryResult);
        std::packaged_task<bool()> pt(
            [=] { return MediaAnalysisHelper::ParseGeoInfo({ fileId + "," + latitude + "," + longitude }, false); });
        std::future<bool> futureResult = pt.get_future();
        ffrt::thread(std::move(pt)).detach();

        bool parseResult = false;
        const int timeout = 2;
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

shared_ptr<NativeRdb::ResultSet> QueryGeoAssets(const RdbPredicates &rdbPredicates, const vector<string> &columns,
    bool isForce)
{
    MEDIA_INFO_LOG("Query Geo Assets");
    auto queryResult = MediaLibraryRdbStore::QueryWithFilter(rdbPredicates, columns);
    if (queryResult == nullptr) {
        MEDIA_ERR_LOG("Query Geographic Information Failed, queryResult is nullptr");
        return queryResult;
    }
    const vector<string> &whereArgs = rdbPredicates.GetWhereArgs();
    bool cond = (whereArgs.empty() || whereArgs.front().empty());
    CHECK_AND_RETURN_RET_LOG(!cond, queryResult, "Query Geographic Information can not get info");
 
    if (isForce) {
        std::vector<std::string> geoInfo;
        while (queryResult->GoToNextRow() == NativeRdb::E_OK) {
            string fileId = to_string(GetInt32Val(MediaColumn::MEDIA_ID, queryResult));
            string latitude = GetStringVal(PhotoColumn::PHOTOS_TABLE + "." + LATITUDE, queryResult);
            string longitude = GetStringVal(PhotoColumn::PHOTOS_TABLE + "." + LONGITUDE, queryResult);
            string addressDescription = GetStringVal(ADDRESS_DESCRIPTION, queryResult);
            if (CheckLatitudeAndLongitude(latitude, longitude) && addressDescription.empty()) {
                geoInfo.push_back(fileId + "," + latitude + "," + longitude);
            }
        }

        CHECK_AND_RETURN_RET_INFO_LOG(!geoInfo.empty(), queryResult, "No need to query geo info assets");
        std::packaged_task<bool()> pt(
            [geoInfo = std::move(geoInfo)] { return MediaAnalysisHelper::ParseGeoInfo(std::move(geoInfo), true); });
        std::future<bool> futureResult = pt.get_future();
        ffrt::thread(std::move(pt)).detach();

        bool parseResult = false;
        const int timeout = 5;
        std::future_status futureStatus = futureResult.wait_for(std::chrono::seconds(timeout));
        if (futureStatus == std::future_status::ready) {
            parseResult = futureResult.get();
        } else {
            MEDIA_ERR_LOG("ParseGeoInfoAssets Failed, futureStatus: %{public}d", static_cast<int>(futureStatus));
        }

        if (parseResult) {
            queryResult = MediaLibraryRdbStore::QueryWithFilter(rdbPredicates, columns);
        }
        MEDIA_INFO_LOG("ParseGeoInfoAssets completed, parseResult: %{public}d", parseResult);
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

shared_ptr<NativeRdb::ResultSet> QueryCvInfo(MediaLibraryCommand &cmd, const vector<string> &columns,
    const DataSharePredicates &predicates)
{
    switch (cmd.GetOprnType()) {
        case OperationType::QUERY_RAW_VISION_TOTAL:
        case OperationType::QUERY_RAW_VISION_VIDEO_TOTAL:
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
            return HandleAnalysisAlbumQuery(cmd, columns, predicates);
        case OperationObject::FILESYSTEM_PHOTO:
        case OperationObject::FILESYSTEM_AUDIO:
        case OperationObject::PAH_MOVING_PHOTO:
        case OperationObject::EDIT_DATA_EXISTS:
        case OperationObject::MOVING_PHOTO_VIDEO_READY:
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
        case OperationObject::ANALYSIS_PROGRESS:
        case OperationObject::APP_URI_PERMISSION_INNER:
            return MediaLibraryRdbStore::QueryWithFilter(RdbUtils::ToPredicates(predicates, cmd.GetTableName()),
                columns);
        case OperationObject::SEARCH_TOTAL:
            return QueryIndex(cmd, columns, predicates);
        case OperationObject::PAH_MULTISTAGES_CAPTURE:
            return MultiStagesPhotoCaptureManager::GetInstance().HandleMultiStagesOperation(cmd, columns);
        case OperationObject::PAH_MULTISTAGES_VIDEO:
            return MultiStagesVideoCaptureManager::GetInstance().HandleMultiStagesOperation(cmd, columns);
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
        case OperationObject::PAH_CLOUD_ENHANCEMENT_OPERATE:
            return EnhancementManager::GetInstance().HandleEnhancementQueryOperation(cmd, columns);
#endif
        case OperationObject::ANALYSIS_ADDRESS:
            return QueryGeo(RdbUtils::ToPredicates(predicates, cmd.GetTableName()), columns);
        case OperationObject::ANALYSIS_ADDRESS_ASSETS:
            return QueryGeoAssets(RdbUtils::ToPredicates(predicates, cmd.GetTableName()), columns, false);
        case OperationObject::ANALYSIS_ADDRESS_ASSETS_ACTIVE:
            return QueryGeoAssets(RdbUtils::ToPredicates(predicates, cmd.GetTableName()), columns, true);
        case OperationObject::TAB_OLD_PHOTO:
            return MediaLibraryTabOldPhotosOperations().Query(
                RdbUtils::ToPredicates(predicates, TabOldPhotosColumn::OLD_PHOTOS_TABLE), columns);
        case OperationObject::TAB_OLD_ALBUM:
            return MediaLibraryTabOldAlbumsOperations().Query(
                RdbUtils::ToPredicates(predicates, TabOldAlbumsColumn::OLD_ALBUM_TABLE), columns);
        case OperationObject::ASSET_ALBUM_OPERATION:
            return MediaLibraryTableAssetAlbumOperations().Query(
                RdbUtils::ToPredicates(predicates, PhotoColumn::TAB_ASSET_AND_ALBUM_OPERATION_TABLE), columns);
        case OperationObject::ANALYSIS_FOREGROUND:
            return MediaLibraryVisionOperations::HandleForegroundAnalysisOperation(cmd);
        case OperationObject::CUSTOM_RECORDS_OPERATION:
            return MediaLibraryRdbStore::QueryWithFilter(RdbUtils::ToPredicates(predicates, cmd.GetTableName()),
                columns);
        case OperationObject::ANALYSIS_ASSET_SD_MAP:
        case OperationObject::ANALYSIS_ALBUM_ASSET_MAP:
            return MediaLibraryRdbStore::Query(RdbUtils::ToPredicates(predicates, cmd.GetTableName()), columns);
        case OperationObject::VISION_ANALYSIS:
        case OperationObject::VISION_ANALYSIS_VIDEO:
            return QueryCvInfo(cmd, columns, predicates);
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

static void ExecuteDfxWork(const std::string &fileId)
{
    std::thread([fileId]() {
        MEDIA_INFO_LOG("start ExecuteDfxWork");
        DfxManager::GetInstance()->HandleCinematicVideoAccessTimes(true, true, fileId);
    }).detach();
}

static void AddToMediaVisitCount(OperationObject &oprnObject, MediaLibraryCommand &cmd)
{
    bool isValidCount = false;
    auto visitType = MediaVisitCountManager::VisitCountType::PHOTO_FS;
    if (oprnObject == OperationObject::FILESYSTEM_PHOTO) {
        isValidCount = true;
        auto fileId = cmd.GetOprnFileId();
        ExecuteDfxWork(fileId);
    } else if (oprnObject == OperationObject::THUMBNAIL) {
        visitType = MediaVisitCountManager::VisitCountType::PHOTO_LCD;
        auto height = std::atoi(cmd.GetQuerySetParam(MEDIA_DATA_DB_HEIGHT).c_str());
        auto width = std::atoi(cmd.GetQuerySetParam(MEDIA_DATA_DB_WIDTH).c_str());
        int min = std::min(width, height);
        int max = std::max(width, height);
        if (min == DEFAULT_ORIGINAL && max == DEFAULT_ORIGINAL) {
            isValidCount = true;
        } else if (min <= DEFAULT_THUMBNAIL_SIZE && max <= MAX_DEFAULT_THUMBNAIL_SIZE) {
            isValidCount = false;
        } else {
            isValidCount = true;
        }
    } else {
        MEDIA_DEBUG_LOG("AddToMediaVisitCount oprnObject: %{public}d", static_cast<int>(oprnObject));
    }

    if (isValidCount) {
        auto fileId = cmd.GetOprnFileId();
        MediaVisitCountManager::AddVisitCount(visitType, std::move(fileId));
    }
}

int32_t MediaLibraryDataManager::OpenFile(MediaLibraryCommand &cmd, const string &mode)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryDataManager::OpenFile");
    auto oprnObject = cmd.GetOprnObject();
    AddToMediaVisitCount(oprnObject, cmd);
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
    thumbnailService_->Init(rdbStore_,  extensionContext);
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

    if (Acl::AclSetSlaveDatabase() != E_OK) {
        MEDIA_ERR_LOG("Failed to set the slave db permission for the media db dir");
    }
}

int32_t ScanFileCallback::OnScanFinished(const int32_t status, const string &uri, const string &path)
{
    CHECK_AND_EXECUTE(this->callback_ == nullptr, this->callback_->OnScanFinished(status, uri, path));
    auto instance = MediaLibraryDataManager::GetInstance();
    if (instance != nullptr) {
        instance->CreateThumbnailAsync(uri, path, originalPhotoPicture);
    }
    return E_OK;
}

void ScanFileCallback::SetCallback(std::shared_ptr<IMediaScannerCallback> &callback)
{
    this->callback_ = callback;
}

int32_t MediaLibraryDataManager::SetCmdBundleAndDevice(MediaLibraryCommand &outCmd)
{
    string clientBundle = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    if (clientBundle.empty()) {
        MEDIA_ERR_LOG("GetClientBundleName failed");
        return E_GET_CLIENTBUNDLE_FAIL;
    }
    outCmd.SetBundleName(clientBundle);
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
    CHECK_AND_RETURN_RET_LOG(retVal > 0, retVal, "failed to revert pending error, fileId:%{private}s", fileId.c_str());
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

static bool IsMediaLibraryRecoveryDone()
{
    int32_t errCode;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(MEDIA_LIBRARY_PREF_XML, errCode);
    if (prefs == nullptr) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return false;
    }
    int32_t done = prefs->GetInt(MEDIA_LIBRARY_RECOVERY_FLAG_KEY, 0);
    return done == 1;
}

static void SetParameterForClone()
{
    std::string nextFlag = "persist.update.hmos_to_next_flag";
    auto isUpgrade = system::GetParameter(nextFlag, "");
    MEDIA_INFO_LOG("isUpgrade:%{public}s", isUpgrade.c_str());
    if (isUpgrade != "1") {
        return;
    }
    if (IsMediaLibraryRecoveryDone()) {
        return;
    }

    std::string cloneFlag = "multimedia.medialibrary.cloneFlag";
    auto currentTime = to_string(MediaFileUtils::UTCTimeSeconds());
    MEDIA_INFO_LOG("SetParameterForClone currentTime:%{public}s", currentTime.c_str());
    bool retFlag = system::SetParameter(cloneFlag, currentTime);
    CHECK_AND_PRINT_LOG(retFlag, "Failed to set parameter cloneFlag, retFlag:%{public}d", retFlag);
}

void MediaLibraryDataManager::SetStartupParameter()
{
    MEDIA_INFO_LOG("Start to set parameter.");
    MediaLibraryTracer tracer;
    tracer.Start("InitCheckList Excute");
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

    SetParameterForClone();
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
    CHECK_AND_RETURN_LOG(compressZip != nullptr, "open zip file failed");

    auto errcode = Media::ZipUtil::AddFileInZip(compressZip, zipFileName, Media::KEEP_NONE_PARENT_PATH);
    CHECK_AND_PRINT_LOG(errcode == 0, "AddFileInZip failed, errCode = %{public}d", errcode);

    Media::ZipUtil::CloseZipFile(compressZip);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("Zip db file success, cost %{public}ld ms", (long)(end - begin));
}

void MediaLibraryDataManager::SubscriberPowerConsumptionDetection()
{
#ifdef DEVICE_STANDBY_ENABLE
    auto subscriber = new (std::nothrow) MediaLibraryStandbyServiceSubscriber();
    if (subscriber == nullptr) {
        return;
    }
    subscriber->SetSubscriberName(SUBSCRIBER_NAME);
    subscriber->SetModuleName(MODULE_NAME);
    DevStandbyMgr::StandbyServiceClient::GetInstance().SubscribeStandbyCallback(subscriber);
#endif
}

int32_t MediaLibraryDataManager::AstcMthAndYearInsert(MediaLibraryCommand &cmd,
    const std::vector<DataShare::DataShareValuesBucket> &values)
{
    int32_t insertCount = 0;
    int32_t successCount = 0;
    for (auto value : values) {
        for (auto iter = value.valuesMap.begin(); iter != value.valuesMap.end(); iter++) {
            insertCount++;
            string idString = iter->first;
            CHECK_AND_BREAK(ThumbnailService::GetInstance()->CreateAstcMthAndYear(idString));
            successCount++;
        }
    }
    CHECK_AND_RETURN_RET(successCount != 0, -1);
    CHECK_AND_RETURN_RET(successCount != insertCount, 1);
    return E_OK;
}

static void DealUpdateForDirty(const shared_ptr<NativeRdb::ResultSet> &resultSet, bool fileExist,
    std::vector<std::string> &dirtyToZeroFileIds, std::vector<std::string> &dirtyToThreeFileIds)
{
    int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    int32_t position = GetInt32Val(PhotoColumn::PHOTO_POSITION, resultSet);
    int32_t effectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
    int64_t editTime = GetInt64Val(PhotoColumn::PHOTO_EDIT_TIME, resultSet);

    // position = 2：update dirty 0
    // position = 3: if edit, update dirty 3; else update dirty 0
    if (position == PHOTO_CLOUD_POSITION) {
        if (fileExist) {
            MEDIA_WARN_LOG("File exists while position is 2, file_id: %{public}d", fileId);
            return;
        } else {
            dirtyToZeroFileIds.push_back(to_string(fileId));
        }
    } else if (position == PHOTO_LOCAL_CLOUD_POSITION) {
        if (!fileExist) {
            MEDIA_WARN_LOG("File not exists while position is 3, file_id: %{public}d", fileId);
            return;
        } else {
            if (editTime > 0 || effectMode > 0) {
                dirtyToThreeFileIds.push_back(to_string(fileId));
            } else {
                dirtyToZeroFileIds.push_back(to_string(fileId));
            }
        }
    }
}

static int32_t DoUpdateDirtyForCloudCloneOperation(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const std::vector<std::string> &fileIds, bool updateToZero)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_FAIL, "rdbStore is nullptr");
    CHECK_AND_RETURN_RET_INFO_LOG(!fileIds.empty(), E_OK, "No cloud data need to update dirty for clone found.");
    ValuesBucket updatePostBucket;
    if (updateToZero) {
        updatePostBucket.Put(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_SYNCED));
    } else {
        updatePostBucket.Put(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_FDIRTY));
    }
    AbsRdbPredicates updatePredicates = AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    updatePredicates.In(MediaColumn::MEDIA_ID, fileIds);
    int32_t changeRows = -1;
    int32_t ret = rdbStore->Update(changeRows, updatePostBucket, updatePredicates);
    CHECK_AND_RETURN_RET_LOG((ret == E_OK && changeRows > 0), E_FAIL,
        "Failed to UpdateDirtyForCloudClone, ret: %{public}d, updateRows: %{public}d", ret, changeRows);
    return ret;
}

static int32_t DoUpdateDirtyForCloudCloneOperationV2(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const std::vector<std::string> &fileIds)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_FAIL, "rdbStore is nullptr");
    CHECK_AND_RETURN_RET_INFO_LOG(!fileIds.empty(), E_OK, "No cloud data need to update dirty for clone found.");
    ValuesBucket updatePostBucket;
    updatePostBucket.Put(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL));
    updatePostBucket.Put(PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE,
        static_cast<int32_t>(SouthDeviceType::SOUTH_DEVICE_NULL));
    AbsRdbPredicates updatePredicates = AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    updatePredicates.In(MediaColumn::MEDIA_ID, fileIds);
    int32_t changeRows = -1;
    int32_t ret = rdbStore->Update(changeRows, updatePostBucket, updatePredicates);
    CHECK_AND_RETURN_RET_LOG((ret == E_OK && changeRows > 0), E_FAIL,
        "Failed to UpdateDirtyForCloudClone, ret: %{public}d, updateRows: %{public}d", ret, changeRows);

    string updateSql = "UPDATE " + PhotoColumn::TAB_OLD_PHOTOS_TABLE + " SET " +
        COLUMN_OLD_FILE_ID + " = (" + std::to_string(ERROR_OLD_FILE_ID_OFFSET) + " - " + MediaColumn::MEDIA_ID + ") "+
        "WHERE " +  MediaColumn::MEDIA_ID + " IN (";
    vector<ValueObject> bindArgs;
    for (auto fileId : fileIds) {
        bindArgs.push_back(fileId);
        updateSql.append("?,");
    }
    updateSql = updateSql.substr(0, updateSql.length() -1);
    updateSql.append(")");
    ret = rdbStore->ExecuteSql(updateSql, bindArgs);
    return ret;
}

static int32_t DoDeleteHdcDataOperation(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const std::vector<std::string> &fileIds)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_FAIL, "rdbStore is nullptr");
    CHECK_AND_RETURN_RET_INFO_LOG(!fileIds.empty(), E_OK, "Not need to delete dirty data.");
    AbsRdbPredicates deletePredicates = AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    deletePredicates.In(MediaColumn::MEDIA_ID, fileIds);
    int32_t deletedRows = -1;
    int32_t ret = rdbStore->Delete(deletedRows, deletePredicates);
    CHECK_AND_RETURN_RET_LOG((ret == E_OK && deletedRows > 0), E_FAIL,
        "Failed to DoDeleteHdcDataOperation, ret: %{public}d, deletedRows: %{public}d", ret, deletedRows);
    return ret;
}

int32_t MediaLibraryDataManager::UpdateDirtyForCloudClone(int32_t version)
{
    switch (version) {
        case UPDATE_DIRTY_CLOUD_CLONE_V1: {
            return UpdateDirtyForCloudClone();
        }
        case UPDATE_DIRTY_CLOUD_CLONE_V2: {
            return UpdateDirtyForCloudCloneV2();
        }
        default: {
            break;
        }
    }
    return E_OK;
}

int32_t MediaLibraryDataManager::UpdateDirtyForCloudClone()
{
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, E_FAIL, "rdbStore is nullptr");
    MEDIA_INFO_LOG("MediaLibraryDataManager::UpdateDirtyForCloudClone");
    const std::string QUERY_DIRTY_FOR_CLOUD_CLONE_INFO =
        "SELECT p.file_id, p.data, p.position, p.edit_time, p.moving_photo_effect_mode "
        "FROM Photos p "
        "JOIN tab_old_photos t ON p.file_id = t.file_id "
        "WHERE (p.position = 2 OR p.position = 3) AND p.dirty = 1 "
        "LIMIT " + std::to_string(UPDATE_BATCH_SIZE);

    bool nextUpdate = true;
    while (nextUpdate && MedialibrarySubscriber::IsCurrentStatusOn()) {
        shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore_->QuerySql(QUERY_DIRTY_FOR_CLOUD_CLONE_INFO);
        CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_FAIL, "Failed to query resultSet");
        int32_t count = -1;
        int32_t err = resultSet->GetRowCount(count);
        MEDIA_INFO_LOG("the resultSet size is %{public}d", count);
        if (count < UPDATE_BATCH_SIZE) {
            nextUpdate = false;
        }

        // get file id need to update
        vector<std::string> dirtyToZeroFileIds;
        vector<std::string> dirtyToThreeFileIds;
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            std::string dataPath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
            dataPath.replace(0, PhotoColumn::FILES_CLOUD_DIR.length(), PhotoColumn::FILES_LOCAL_DIR);
            CHECK_AND_CONTINUE_INFO_LOG(dataPath != "",
                "The data path is empty, data path: %{public}s", dataPath.c_str());
            bool fileExist = MediaFileUtils::IsFileExists(dataPath);
            DealUpdateForDirty(resultSet, fileExist, dirtyToZeroFileIds, dirtyToThreeFileIds);
        }

        resultSet->Close();
        CHECK_AND_PRINT_LOG(DoUpdateDirtyForCloudCloneOperation(rdbStore_, dirtyToZeroFileIds, true) == E_OK,
            "Failed to DoUpdateDirtyForCloudCloneOperation for dirtyToZeroFileIds");
        CHECK_AND_PRINT_LOG(DoUpdateDirtyForCloudCloneOperation(rdbStore_, dirtyToThreeFileIds, false) == E_OK,
            "Failed to DoUpdateDirtyForCloudCloneOperation for dirtyToThreeFileIds");
    }
    if (!nextUpdate) {
        int32_t errCode;
        shared_ptr<NativePreferences::Preferences> prefs =
            NativePreferences::PreferencesHelper::GetPreferences(TASK_PROGRESS_XML, errCode);
        CHECK_AND_RETURN_RET_LOG(prefs, E_FAIL, "Get preferences error: %{public}d", errCode);
        prefs->PutInt(NO_UPDATE_DIRTY, 1);
    }
    return E_OK;
}

int32_t MediaLibraryDataManager::UpdateDirtyForCloudCloneV2()
{
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, E_FAIL, "rdbStore is nullptr");
    MEDIA_INFO_LOG("MediaLibraryDataManager::UpdateDirtyForCloudCloneV2");
    const std::string QUERY_DIRTY_FOR_CLOUD_CLONE_INFO_V2 =
        "SELECT p.file_id, p.data, p.position, p.cloud_id "
        "FROM Photos p "
        "JOIN tab_old_photos t ON p.file_id = t.file_id "
        "WHERE p.position = 2 AND COALESCE(cloud_id,'') = '' AND t.old_file_id = -1 "
        "LIMIT " + std::to_string(UPDATE_BATCH_SIZE);
    bool nextUpdate = true;
    while (nextUpdate && MedialibrarySubscriber::IsCurrentStatusOn()) {
        shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore_->QuerySql(QUERY_DIRTY_FOR_CLOUD_CLONE_INFO_V2);
        CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_FAIL, "Failed to query resultSet");
        int32_t count = -1;
        int32_t err = resultSet->GetRowCount(count);
        MEDIA_INFO_LOG("the resultSet size is %{public}d", count);
        if (count < UPDATE_BATCH_SIZE) {
            nextUpdate = false;
        }
        // get file id need to update
        vector<std::string> dirtyFileIds;
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            std::string dataPath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
            dataPath.replace(0, PhotoColumn::FILES_CLOUD_DIR.length(), PhotoColumn::FILES_LOCAL_DIR);
            bool cond = (dataPath == "" || !MediaFileUtils::IsFileExists(dataPath));
            CHECK_AND_CONTINUE_INFO_LOG(!cond, "The data path is empty, data path: %{public}s",
                MediaFileUtils::DesensitizePath(dataPath).c_str());

            int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
            dirtyFileIds.push_back(to_string(fileId));
        }
        resultSet->Close();
        CHECK_AND_PRINT_LOG(DoUpdateDirtyForCloudCloneOperationV2(rdbStore_, dirtyFileIds) == E_OK,
            "Failed to DoUpdateDirtyForCloudCloneOperationV2 for dirtyFileIds");
    }
    if (!nextUpdate) {
        int32_t errCode;
        shared_ptr<NativePreferences::Preferences> prefs =
            NativePreferences::PreferencesHelper::GetPreferences(TASK_PROGRESS_XML, errCode);
        CHECK_AND_RETURN_RET_LOG(prefs, E_FAIL, "Get preferences error: %{public}d", errCode);
        prefs->PutInt(NO_UPDATE_DIRTY_CLOUD_CLONE_V2, 1);
    }
    return E_OK;
}

int32_t MediaLibraryDataManager::UpdateDirtyHdcDataStatus()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(TASK_PROGRESS_XML, errCode);
    CHECK_AND_RETURN_RET_LOG(prefs, E_FAIL, "Get preferences error: %{public}d", errCode);
    prefs->PutInt(NO_DELETE_DIRTY_HDC_DATA, 1);
    return E_OK;
}

void MediaLibraryDataManager::DeleteDirtyFileAndDir(const std::vector<std::string>& deleteFilePaths)
{
    for (auto path : deleteFilePaths) {
        bool deleteFileRet = MediaFileUtils::DeleteFileOrFolder(path, true);
        std::string thumbsFolder =
            MediaFileUtils::GetReplacedPathByPrefix(CLOUD_PREFIX_PATH, THUMB_PREFIX_PATH, path);
        bool deleteThumbsRet = MediaFileUtils::DeleteFileOrFolder(thumbsFolder, false);
        bool cond = (!deleteFileRet || !deleteThumbsRet);
        CHECK_AND_PRINT_LOG(!cond, "Clean file failed, path: %{public}s, deleteFileRet: %{public}d, "
            "deleteThumbsRet: %{public}d, errno: %{public}d",
            MediaFileUtils::DesensitizePath(path).c_str(),
            static_cast<int32_t>(deleteFileRet), static_cast<int32_t>(deleteThumbsRet), errno);
    }
}

int32_t MediaLibraryDataManager::ClearDirtyHdcData()
{
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, E_FAIL, "rdbStore is nullptr");
    MEDIA_INFO_LOG("MediaLibraryDataManager::ClearDirtyHdcData");
    const std::string QUERY_DIRTY_HDC_INFO =
        "SELECT p.file_id, p.data, p.position, p.cloud_id, p.display_name FROM Photos p "
        "JOIN tab_old_photos t ON p.file_id = t.file_id "
        "WHERE p.position = 2  AND COALESCE(cloud_id,'') = '' "
        "LIMIT " + std::to_string(DELETE_BATCH_SIZE);
    bool nextDelete = true;
    while (nextDelete && MedialibrarySubscriber::IsCurrentStatusOn()) {
        shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore_->QuerySql(QUERY_DIRTY_HDC_INFO);
        CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_FAIL, "Failed to query resultSet");
        int32_t count = -1;
        int32_t err = resultSet->GetRowCount(count);
        MEDIA_INFO_LOG("the resultSet size is %{public}d", count);
        if (count < DELETE_BATCH_SIZE) {
            nextDelete = false;
        }

        vector<std::string> dirtyFileIds;
        vector<std::string> deleteUris;
        vector<std::string> deleteFilePaths;
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            std::string dataPath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
            dataPath.replace(0, PhotoColumn::FILES_CLOUD_DIR.length(), PhotoColumn::FILES_LOCAL_DIR);
            bool cond = (dataPath == "" || MediaFileUtils::IsFileExists(dataPath));
            CHECK_AND_CONTINUE_INFO_LOG(!cond, "The data path is empty or file exist, data path: %{public}s",
                MediaFileUtils::DesensitizePath(dataPath).c_str());

            int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
            dirtyFileIds.push_back(to_string(fileId));
            string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
            string filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
            string uri = MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, to_string(fileId),
                MediaFileUtils::GetExtraUri(displayName, filePath));
            deleteUris.push_back(uri);
            deleteFilePaths.push_back(filePath);
        }
        resultSet->Close();
        DeleteDirtyFileAndDir(deleteFilePaths);
        CHECK_AND_RETURN_RET_LOG(DoDeleteHdcDataOperation(rdbStore_, dirtyFileIds) == E_OK,
            E_FAIL, "Failed to DoDeleteHdcDataOperation for dirtyFileIds");
        MediaLibraryRdbUtils::UpdateAllAlbums(rdbStore_, deleteUris);
    }

    CHECK_AND_RETURN_RET(nextDelete, UpdateDirtyHdcDataStatus());
    return E_OK;
}

static void Update500EditDataSize(const shared_ptr<MediaLibraryRdbStore> rdbStore, std::string startFileId,
    bool &hasMore)
{
    std::vector<std::string> filePaths;
    std::vector<std::string> fileIds;
    int32_t ret = MediaLibraryPhotoOperations::Get500FileIdsAndPathS(rdbStore, fileIds, filePaths,
        startFileId, hasMore);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to get filePaths and IDs, error code: %{public}d", ret);
        return;
    }

    if (filePaths.empty() || fileIds.empty()) {
        hasMore = false;
        MEDIA_INFO_LOG("No files need to update edit data size");
        return;
    }

    MEDIA_INFO_LOG("Start to update edit data size for %{public}zu files", fileIds.size());
    int32_t successCount = 0;
    int32_t failedCount = 0;

    for (size_t i = 0; i < fileIds.size(); ++i) {
        const auto &fileId = fileIds[i];
        const auto &filePath = filePaths[i];

        std::string editDataFilePath;
        ret = MediaLibraryPhotoOperations::ConvertPhotoCloudPathToLocalData(filePath, editDataFilePath);
        if (ret != E_OK) {
            MEDIA_WARN_LOG("Skip invalid file ID: %{public}s (error code: %{public}d)",
                fileId.c_str(), ret);
            failedCount++;
            continue;
        }

        ret = MediaLibraryRdbStore::UpdateEditDataSize(rdbStore, fileId, editDataFilePath);
        if (ret == E_OK) {
            successCount++;
        } else {
            MEDIA_ERR_LOG("Update failed for ID: %{public}s, Path: %{public}s (error code: %{public}d)",
                fileId.c_str(), editDataFilePath.c_str(), ret);
            failedCount++;
        }
    }

    MEDIA_INFO_LOG("Edit data size update completed: success=%{public}d, failed=%{public}d",
                   successCount, failedCount);
    if (failedCount > 0) {
        MEDIA_WARN_LOG("%{public}d files failed to update, check above logs for details", failedCount);
    }
}

int32_t MediaLibraryDataManager::UpdateMediaSizeFromStorage()
{
    if (!MedialibrarySubscriber::IsCurrentStatusOn()) {
        MEDIA_INFO_LOG("Current status is off, skip disk cleanup");
        return E_OK;
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (!rdbStore) {
        MEDIA_ERR_LOG("RdbStore is null");
        return E_HAS_DB_ERROR;
    }

    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(TASK_PROGRESS_XML, errCode);
    if (prefs == nullptr) {
        MEDIA_ERR_LOG("Get preferences error: %{public}d", errCode);
        return errCode;
    }

    int32_t startFileId = prefs->GetInt(UPDATE_EDITDATA_SIZE_COUNT, 0);
    bool hasMore = true;
    while (hasMore) {
        if (!MedialibrarySubscriber::IsCurrentStatusOn()) {
            MEDIA_INFO_LOG("Current status is off, skip disk cleanup");
            return E_OK;
        }
        prefs->PutInt(UPDATE_EDITDATA_SIZE_COUNT, startFileId);
        prefs->FlushSync();
        Update500EditDataSize(rdbStore, std::to_string(startFileId), hasMore);
        // 一次500张图片
        startFileId += 500;
    }

    prefs->PutInt(NO_UPDATE_EDITDATA_SIZE, 1);
    prefs->FlushSync();
    return E_OK;
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
        CHECK_AND_RETURN_RET_INFO_LOG(rowCount != 0, E_OK, "No need to UpdateBurstCoverLevelFromGallery.");

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

static shared_ptr<NativeRdb::ResultSet> BatchQueryUninitHdrPhoto(const shared_ptr<MediaLibraryRdbStore> &rdbStore)
{
    string querySql = "SELECT " + MediaColumn::MEDIA_FILE_PATH + ", " + MediaColumn::MEDIA_ID + " FROM " +
        PhotoColumn::PHOTOS_TABLE + " WHERE " + MediaColumn::MEDIA_ID + " > " + to_string(g_updateHdrModeId) +
        " AND " + MediaColumn::MEDIA_TYPE + " = " + to_string(MEDIA_TYPE_IMAGE) +
        " AND " + PhotoColumn::PHOTO_HDR_MODE + " = " + to_string(static_cast<int32_t>(HdrMode::DEFAULT)) +
        " AND " + PhotoColumn::PHOTO_POSITION + " != " + to_string(static_cast<int32_t>(PhotoPositionType::CLOUD)) +
        " ORDER BY " + MediaColumn::MEDIA_ID + " LIMIT " + to_string(UPDATE_BATCH_SIZE);

    auto resultSet = rdbStore->QueryByStep(querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet, nullptr, "failed to acquire result from visitor query");
    return resultSet;
}

static int32_t UpdateHdrMode(const shared_ptr<MediaLibraryRdbStore> &rdbStore,
    shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    CHECK_AND_RETURN_RET_LOG(resultSet, E_ERR, "resultSet is nullptr");
    int32_t count = -1;
    int32_t retCount = resultSet->GetRowCount(count);
    CHECK_AND_RETURN_RET_LOG(retCount == NativeRdb::E_OK && count >= 0, E_ERR, "rdb failed");
    if (count == 0) {
        MEDIA_INFO_LOG("no HDR mode need to update");
        return E_SUCCESS;
    }

    while (resultSet->GoToNextRow() == NativeRdb::E_OK && MedialibrarySubscriber::IsCurrentStatusOn()) {
        string filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        g_updateHdrModeId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        SourceOptions opts;
        uint32_t err = E_OK;
        std::unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(filePath, opts, err);
        if (err != E_OK || imageSource == nullptr) {
            MEDIA_ERR_LOG("CreateImageSource failed, fileId: %{public}d, filePath: %{public}s", g_updateHdrModeId,
                filePath.c_str());
            continue;
        }
        HdrMode hdrMode = HdrMode::DEFAULT;
        if (imageSource->IsHdrImage()) {
            hdrMode = MediaImageFrameWorkUtils::ConvertImageHdrTypeToHdrMode(imageSource->CheckHdrType());
        }

        if (hdrMode != HdrMode::DEFAULT) {
            string updateSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " +
                PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE + " = " +
                to_string(static_cast<int32_t>(DynamicRangeType::HDR)) + ", " + PhotoColumn::PHOTO_HDR_MODE + " = " +
                to_string(static_cast<int32_t>(hdrMode)) + ", " + PhotoColumn::PHOTO_META_DATE_MODIFIED + " = " +
                to_string(MediaFileUtils::UTCTimeMilliSeconds()) + " WHERE " + MediaColumn::MEDIA_ID + " = " +
                to_string(g_updateHdrModeId);
            int32_t ret = rdbStore->ExecuteSql(updateSql);
            if (ret != NativeRdb::E_OK) {
                MEDIA_ERR_LOG("Failed to update rdb");
                continue;
            }
        }
    }
    return E_SUCCESS;
}

int32_t MediaLibraryDataManager::UpdatePhotoHdrMode()
{
    MEDIA_INFO_LOG("Begin UpdatePhotoHdrMode");
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryDataManager::UpdatePhotoHdrMode");
    shared_lock<shared_mutex> sharedLock(mgrSharedMutex_);
    CHECK_AND_RETURN_RET_LOG(refCnt_.load() > 0, E_FAIL, "MediaLibraryDataManager is not initialized");
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, E_FAIL, "rdbStore_ is nullptr");

    while (MedialibrarySubscriber::IsCurrentStatusOn()) {
        auto resultSet = BatchQueryUninitHdrPhoto(rdbStore_);
        CHECK_AND_RETURN_RET_LOG(resultSet, E_ERR, "resultSet is nullptr");
        int32_t count = -1;
        auto ret = resultSet->GetRowCount(count);
        CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK && count >= 0, E_ERR, "rdb failed");
        if (count == 0) {
            break;
        }
        int32_t updateRet = UpdateHdrMode(rdbStore_, resultSet);
        CHECK_AND_RETURN_RET_LOG(updateRet == E_SUCCESS, E_FAIL, "failed to UpdateHdrMode");
    }
    MEDIA_INFO_LOG("End UpdatePhotoHdrMode, file_id: %{public}d", g_updateHdrModeId);
    return E_SUCCESS;
}

int32_t MediaLibraryDataManager::BatchInsertMediaAnalysisData(MediaLibraryCommand &cmd,
    const vector<DataShareValuesBucket> &values)
{
    if (values.empty()) {
        return E_FAIL;
    }

    if (MediaLibraryRestore::GetInstance().IsRealBackuping()) {
        MEDIA_INFO_LOG("[BatchInsertMediaAnalysisData] rdb is backuping");
        return E_FAIL;
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET(rdbStore != nullptr, E_HAS_DB_ERROR);
    switch (cmd.GetOprnObject()) {
        case OperationObject::VISION_START ... OperationObject::VISION_END:
        case OperationObject::GEO_DICTIONARY:
        case OperationObject::GEO_KNOWLEDGE:
        case OperationObject::GEO_PHOTO:
        case OperationObject::SEARCH_TOTAL:
        case OperationObject::STORY_ALBUM:
        case OperationObject::STORY_COVER:
        case OperationObject::STORY_PLAY:
        case OperationObject::USER_PHOTOGRAPHY:
        case OperationObject::ANALYSIS_PROGRESS:
        case OperationObject::ANALYSIS_ASSET_SD_MAP:
        case OperationObject::ANALYSIS_ALBUM_ASSET_MAP:
        case OperationObject::ANALYSIS_PHOTO_MAP: {
            std::vector<ValuesBucket> insertValues;
            for (auto value : values) {
                ValuesBucket valueInsert = RdbUtils::ToValuesBucket(value);
                insertValues.push_back(valueInsert);
            }
            int64_t outRowId = -1;
            int32_t ret = rdbStore->BatchInsert(outRowId, cmd.GetTableName(), insertValues);
            bool cond = (ret != NativeRdb::E_OK || outRowId < 0);
            CHECK_AND_RETURN_RET_LOG(!cond, E_FAIL, "Batch insert media analysis values fail, err = %{public}d", ret);
            return outRowId;
        }
        default:
            break;
    }
    return E_FAIL;
}

static int32_t GetExistsDupSize(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
    int32_t &totalCount, int64_t &totalSize)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_INNER_FAIL, "[HeifDup] rdbStore is nullptr");

    const std::string sql = R"(SELECT SUM(trans_code_file_size) AS total_size, COUNT(1) AS total_count FROM Photos
        WHERE exist_compatible_duplicate = 1)";
    auto resultSet = rdbStore->QuerySql(sql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK, E_INNER_FAIL,
        "[HeifDup] Query dup size, resultSet is nullptr or empty.");

    totalCount = GetInt32Val("total_count", resultSet);
    if (totalCount > 0) {
        totalSize = GetInt64Val("total_size", resultSet);
    }
    return E_OK;
}

static int32_t GetExpiredCount(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore, int64_t threshold)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, 0, "[HeifDup] rdbStore is nullptr");

    const std::string sql = R"(SELECT COUNT(1) AS expired_count FROM Photos
        WHERE transcode_time > 0 and transcode_time < ?)";
    std::vector<NativeRdb::ValueObject> params = { threshold };
    auto resultSet = rdbStore->QuerySql(sql, params);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK, 0,
        "[HeifDup] Query dup size, resultSet is nullptr or empty.");

    return GetInt32Val("expired_count", resultSet);
}

int32_t MediaLibraryDataManager::RestoreInvalidPosData()
{
    MEDIA_INFO_LOG("MediaLibraryDataManager::RestoreInvalidPosData Start");
    std::string sql =
        "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_POSITION + " = 2" +
        " WHERE " + PhotoColumn::PHOTO_POSITION + " = -1";
    auto ret = rdbStore_->ExecuteSql(sql);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret, "Execute sql failed");
    MEDIA_INFO_LOG("MediaLibraryDataManager::RestoreInvalidPosData End");
    return ret;
}

static int32_t BackupDebugDatabase(const std::string &path)
{
    auto rdbStore = MediaLibraryDataManager::GetInstance()->rdbStore_;
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_OPR_DEBUG_DB_FAIL, "RdbStore is nullptr");
    int32_t errCode = rdbStore->Backup(path);
    CHECK_AND_RETURN_RET(errCode == 0, E_BACK_UP_DB_FAIL);
    return E_SUCCESS;
}

static int64_t GetDatabaseFileSize(const std::string &destFileName)
{
    struct stat statInfo {};
    CHECK_AND_RETURN_RET(stat(destFileName.c_str(), &statInfo) == 0, static_cast<int64_t>(E_OPR_DEBUG_DB_FAIL));
    return statInfo.st_size;
}

static int32_t GetZipFile(const std::string &srcPath, const std::string &destPath)
{
    std::string zipFileName = srcPath;
    zipFile compressZip = Media::ZipUtil::CreateZipFile(destPath);
    CHECK_AND_RETURN_RET_LOG(compressZip != nullptr, E_OPR_DEBUG_DB_FAIL, "Failed to open zipFile");
    int32_t errCode = Media::ZipUtil::AddFileInZip(compressZip, zipFileName, Media::KEEP_NONE_PARENT_PATH);
    CHECK_AND_RETURN_RET_LOG(errCode == 0, E_OPR_DEBUG_DB_FAIL, "AddFileInZip failed, errCode = %{public}d", errCode);
    Media::ZipUtil::CloseZipFile(compressZip);
    return E_SUCCESS;
}

static int32_t DropAnalysisTables(shared_ptr<NativeRdb::RdbStore> &store)
{
    MEDIA_INFO_LOG("Start drop analysis tables");
    int32_t err = store->ExecuteSql("DROP TRIGGER IF EXISTS delete_vision_trigger");
    CHECK_AND_PRINT_LOG(err == NativeRdb::E_OK, "Fail to execute drop vision_trigger");
    for (const auto &tableName : VISION_HIGHLIGHT_TABLES) {
        string sql = "DROP TABLE IF EXISTS " + tableName;
        err = store->ExecuteSql(sql);
        CHECK_AND_PRINT_LOG(err == NativeRdb::E_OK, "Fail to execute: %{private}s", sql.c_str());
    }
    return E_SUCCESS;
}

static int32_t DebugDatabaseFilter(const std::string &path, const std::string &name)
{
    MediaLibraryDataCallBack rdbDataCallBack;
    NativeRdb::RdbStoreConfig config("");
    std::string filePath = path + name;
    config.SetName(name);
    config.SetPath(filePath);
    int32_t errCode;
    shared_ptr<NativeRdb::RdbStore> store = RdbHelper::GetRdbStore(config, MEDIA_RDB_VERSION, rdbDataCallBack, errCode);
    CHECK_AND_RETURN_RET_LOG(store != nullptr && errCode == E_OK, errCode, "DatabaseDFXFilter: fail to get rdb");
    errCode = DropAnalysisTables(store);
    return errCode;
}

static int32_t DeleteTempDatabaseFile(const std::string &tempFilePath)
{
    const vector<std::string> fileExtension = {"-compare", "-dwr", "-shm", "-wal"};
    for (const auto &ext : fileExtension) {
        string tempFile = tempFilePath + ext;
        MediaFileUtils::DeleteFile(tempFile);
    }
    return E_SUCCESS;
}

static int64_t HandleDebugDatabaseUpload(const string &filePath, const string &tempName, const string &destName)
{
    string tempFile = filePath + tempName;
    string destFile = filePath + destName;
    CHECK_AND_RETURN_RET_LOG(GetZipFile(tempFile, destFile) == E_OK, E_OPR_DEBUG_DB_FAIL, "Fail to zip DBFile");
    int64_t totalSize = GetDatabaseFileSize(destFile);
    if (totalSize > MAX_DEBUG_DB_FILE_SIZE_BYTE) {
        MEDIA_WARN_LOG("%{public}s exceed 3GB, size:%{public}s b", destName.c_str(), std::to_string(totalSize).c_str());
        MediaFileUtils::DeleteFile(destFile);
        int32_t errCode = DebugDatabaseFilter(filePath, tempName);
        CHECK_AND_RETURN_RET_LOG(errCode == E_OK, E_OPR_DEBUG_DB_FAIL, "Init Rdb fail, errCode: %{public}d", errCode);
        CHECK_AND_RETURN_RET_LOG(GetZipFile(tempFile, destFile) == E_OK, E_OPR_DEBUG_DB_FAIL, "Fail to zip DBFile");
        errCode = DeleteTempDatabaseFile(tempFile);
        CHECK_AND_WARN_LOG(errCode == E_SUCCESS, "Failed to delete temp Database file");
        totalSize = GetDatabaseFileSize(destFile);
    }
    MediaFileUtils::DeleteFile(tempFile);
    return totalSize;
}

int32_t MediaLibraryDataManager::AcquireDebugDatabase(const string &betaIssueId, const string &betaScenario,
    string &fileName, string &fileSize)
{
    MEDIA_INFO_LOG("MediaLibraryDataManager::AcquireDebugDatabase start");
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryDataManager::AcquireDebugDatabase");
    CHECK_AND_RETURN_RET_LOG(PermissionUtils::IsBetaVersion(), E_BETA_VERSION_FAIL, "Caller not beta version");
    
    const std::string filePath = BETA_DEBUG_DB_FILE_PATH;
    CHECK_AND_RETURN_RET_LOG(access(filePath.c_str(), F_OK) == 0 || MediaFileUtils::CreateDirectory(filePath),
        E_OPR_DEBUG_DB_FAIL, "Create dir failed, dir = %{private}s", filePath.c_str());
    fileName = "media_library_" + betaIssueId + ".db.zip";
    const std::string destFile = filePath + fileName;
    if (MediaFileUtils::IsFileExists(destFile)) {
        MediaFileUtils::DeleteFile(destFile);
        MEDIA_WARN_LOG("Same betaIssueId File has been deleted, destFile = %{private}s", destFile.c_str());
    }
    std::string tempFileName = "media_library_temp_" + betaIssueId + ".db";
    std::string tempFile = filePath + tempFileName;

    tracer.Start("BackupDebugDatabase");
    int32_t errCode = BackupDebugDatabase(tempFile);
    CHECK_AND_RETURN_RET_LOG(errCode == E_SUCCESS, errCode, "Rdb backup fail: %{public}d", errCode);

    //数据库上传 故障路径 betaScenario = 1024_1041_1018
    tracer.Start("HandleDebugDatabaseUpload");
    int64_t ret = HandleDebugDatabaseUpload(filePath, tempFileName, fileName);
    CHECK_AND_RETURN_RET_LOG(ret != E_OPR_DEBUG_DB_FAIL, static_cast<int32_t>(ret), "Failed to handle debug database");
    fileSize = std::to_string(ret);
    return E_SUCCESS;
}

int32_t MediaLibraryDataManager::ReleaseDebugDatabase(const std::string &betaIssueId)
{
    MEDIA_INFO_LOG("MediaLibraryDataManager::ReleaseDebugDatabase start");
    CHECK_AND_RETURN_RET_LOG(PermissionUtils::IsBetaVersion(), E_BETA_VERSION_FAIL, "Caller not beta version");
    const std::string filePath = BETA_DEBUG_DB_FILE_PATH + "media_library_" + betaIssueId + ".db.zip";
    CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteFile(filePath), "Failed to release database file, path: %{private}s",
        filePath.c_str());
    return E_SUCCESS;
}
shared_ptr<NativeRdb::ResultSet> MediaLibraryDataManager::ProcessBrokerChangeMsg(const std::string &operation)
{
    MEDIA_INFO_LOG("anco_media change operation");
    if (operation.empty()) {
        MEDIA_ERR_LOG("anco_media change operation");
        return nullptr;
    }
    if (operation == BROKER_REMOVE_MSG) {
        MEDIA_INFO_LOG("anco_media receive broker remove msg");
    } else if (operation == BROKER_START_SCAN) {
        MEDIA_INFO_LOG("anco_media start scan");
        ScanLakeAsset();
    }
    return nullptr;
}

void MediaLibraryDataManager::CheckCleanLakeAsset()
{
    // 1. 卸载后persist.hmos_fusion_mgr.ctl.support_hmos为false，过一段时间需要延时读取
    thread scanTask([this]() {
        int loopCnt = 0;
        while (this->ReadSupportHmos()) {
            loopCnt++;
            if (loopCnt > MAX_LOOP_CNT) {
                MEDIA_ERR_LOG("support hmos return.");
                supportHmos_ = "";
                break;
            }
            MEDIA_INFO_LOG("anco_media loop count: %{public}d.", loopCnt);
            const int sleepDurationMilliseconds = 2000;
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepDurationMilliseconds));
        }

        // 2. LAKE_SCAN_DIR文件夹为空
        if (!this->IsDirectoryEmpty()) {
            MEDIA_ERR_LOG("dir is not empty.");
            return;
        }

        MEDIA_INFO_LOG("anco_media delete lake assets");
        // 3. 清空所有的数据库数据
        const string DELETE_LAKE_ASSETS_SQL = "delete from Photos where file_source_type = 3";
        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        CHECK_AND_RETURN_LOG(rdbStore != nullptr, "Failed to get rdbStore.");
        int ret = rdbStore->ExecuteSql(DELETE_LAKE_ASSETS_SQL);
        CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK, "exe delete lake assets error:{%{public}d}", ret);

        MEDIA_INFO_LOG("anco_media refresh all album");
        // 4. 刷新全量相册
        UpdateAllAlbumsData updateAlbumsData;
        updateAlbumsData.shouldUpdateDateModified = true;
        MediaLibraryRdbUtils::UpdateAllAlbums(rdbStore, {}, updateAlbumsData);
        this->SetIsLakeAssetScanned(false);
    });
    scanTask.detach();
}

bool MediaLibraryDataManager::IsLakeAssetScanned()
{
    if (scanStatus_.empty()) {
        scanStatus_ = system::GetParameter("multimedia.medialibrary.lake_asset.scan_status", "false");
        MEDIA_INFO_LOG("anco_media read scan_status[%{public}s]", scanStatus_.c_str());
    }
    return scanStatus_ == "true";
}

bool MediaLibraryDataManager::ReadSupportHmos()
{
    supportHmos_ = system::GetParameter("persist.hmos_fusion_mgr.ctl.support_hmos", "true");
    MEDIA_INFO_LOG("anco_media ReadSupportHmos[%{public}s]", supportHmos_.c_str());
    return supportHmos_ == "true";
}

bool MediaLibraryDataManager::IsSupportHmos()
{
    if (supportHmos_.empty()) {
        supportHmos_ = system::GetParameter("persist.hmos_fusion_mgr.ctl.support_hmos", "true");
        MEDIA_INFO_LOG("anco_media init ReadSupportHmos[%{public}s]", supportHmos_.c_str());
    }

    return supportHmos_ == "true";
}

void MediaLibraryDataManager::SetIsLakeAssetScanned(bool isScanned)
{
    scanStatus_ = isScanned ? "true" : "false";
    auto ret = system::SetParameter("multimedia.medialibrary.lake_asset.scan_status", scanStatus_);
    MEDIA_INFO_LOG("anco_media SetIsLakeAssetScanned[%{public}d], ret: %{public}d", isScanned, ret);
}

void MediaLibraryDataManager::ScanLakeAsset()
{
    MEDIA_INFO_LOG("ScanLakeAsset enter.");

    if (!IsSupportHmos()) {
        MEDIA_INFO_LOG("anco_media no support hmos");
        return;
    }

    if (IsLakeAssetScanned()) {
        return;
    }

    if (access(LAKE_SCAN_DIR.c_str(), F_OK) != 0) {
        MEDIA_ERR_LOG("anco_media dir[%{public}s] not exist.", LAKE_SCAN_DIR.c_str());
        return;
    }

    thread scanTask([this]() {
        MEDIA_INFO_LOG("ScanLakeAsset thread start.");
        DIR* dir = opendir(LAKE_SCAN_DIR.c_str());
        int loopCnt = 0;
        while (dir == nullptr) {
            loopCnt++;
            if (loopCnt > MAX_LOOP_CNT) {
                MEDIA_ERR_LOG("dir[%{public}s] not access.", LAKE_SCAN_DIR.c_str());
                this->SetIsLakeAssetScanned(true);
                return;
            }
            MEDIA_INFO_LOG("anco_media wait dir ready[%{public}d]", loopCnt);
            const int WAIT_TIME_MILLISECONDS = 4000;
            std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_MILLISECONDS));
            dir = opendir(LAKE_SCAN_DIR.c_str());
        }
        closedir(dir);
        int64_t startLoadTime = MediaFileUtils::UTCTimeMilliSeconds();
        GlobalScanner::GetInstance().Run(LAKE_SCAN_DIR.c_str());
        int64_t endLoadTime = MediaFileUtils::UTCTimeMilliSeconds();
        AncoDfxManager::GetInstance().ReportFirstLoadInfo(startLoadTime, endLoadTime);
        this->SetIsLakeAssetScanned(true);
        MEDIA_INFO_LOG("ScanLakeAsset thread end.");
    });
    scanTask.detach();
    AncoDfxManager::GetInstance().RunDfx();
}

bool MediaLibraryDataManager::IsDirectoryEmpty()
{
    DIR* dir = opendir(LAKE_SCAN_DIR.c_str());
    if (!dir) {
        MEDIA_ERR_LOG("dir is null.");
        return true;
    }

    bool isEmpty = true;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (std::string(entry->d_name) == "." || std::string(entry->d_name) == "..") {
            continue;
        }
        isEmpty = false;
        break;
    }

    closedir(dir);
    return isEmpty;
}

int32_t MediaLibraryDataManager::OpenAssetCompress(const std::string &uri, const int32_t type, const int32_t version,
    int32_t &fd)
{
    MEDIA_INFO_LOG("MediaLibraryDataManager::OpenAssetCompress begin");
    fd = -1;
    CHECK_AND_RETURN_RET_LOG(version > 0, E_INVALID_VALUES, "Invalid version: %{public}d", version);
    string assetUri = uri;
    MediaFileUtils::UriAppendKeyValue(assetUri, "type", to_string(static_cast<int32_t>(type)));
    MEDIA_DEBUG_LOG("merged uri = %{public}s", assetUri.c_str());
    Uri openUri(assetUri);

#ifdef MEDIALIBRARY_COMPATIBILITY
    string realUriStr = MediaFileUtils::GetRealUriFromVirtualUri(openUri.ToString());
    Uri realUri(realUriStr);
    MediaLibraryCommand cmd(realUri, Media::OperationType::OPEN);

#else
    MediaLibraryCommand cmd(openUri, Media::OperationType::OPEN);
#endif
    std::string tlvPath = "";
    int32_t ret =
        MediaLibraryPhotoOperations::OpenAssetCompress(cmd, tlvPath, version, fd);
    if (ret != E_OK || fd < 0) {
        MEDIA_ERR_LOG("OpenAssetCompress failed, clear compress file.");
        CHECK_AND_EXECUTE(!MediaFileUtils::IsFileExists(tlvPath), MediaFileUtils::DeleteFile(tlvPath));
        return E_ERR;
    }
    return ret;
}

int32_t MediaLibraryDataManager::NotifyAssetSended(const std::string &uri)
{
    MEDIA_INFO_LOG("NotifyAssetSended begin");
    CHECK_AND_RETURN_RET_LOG(!uri.empty(), E_OK, "Invalid uri");
    std::string id = MediaFileUtils::GetIdFromUri(uri);
    std::string tempFilePath = MediaLibraryAssetOperations::GetAssetCompressCachePath(id);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(tempFilePath), E_OK, "Temp compress asset file not exists");
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::DeleteFile(tempFilePath), E_OK, "Clean temp compress asset file failed");
    MEDIA_INFO_LOG("NotifyAssetSended delete tempFilePath: %{public}s",
        DfxUtils::GetSafePath(tempFilePath).c_str());
    return E_OK;
}

int32_t MediaLibraryDataManager::GetAssetCompressVersion()
{
    int32_t compressVersion = AssetCompressVersionManager::GetAssetCompressVersion();
    MEDIA_INFO_LOG("GetAssetCompressVersion is called, version: %{public}d", compressVersion);
    return compressVersion;
}
}  // namespace Media
}  // namespace OHOS

/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaLibraryCloneRestore"

#include "clone_restore.h"
#include "backup_const_column.h"

#include "application_context.h"
#include "backup_dfx_utils.h"
#include "backup_file_utils.h"
#include "backup_log_utils.h"
#include "clone_restore_classify.h"
#include "clone_restore_cv_analysis.h"
#include "clone_restore_pet_album.h"
#include "clone_restore_portrait_album.h"
#include "clone_restore_highlight.h"
#include "clone_restore_geo.h"
#include "clone_restore_group_photo.h"
#include "cloud_sync_utils.h"
#include "database_report.h"
#include "exif_rotate_utils.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_library_db_upgrade.h"
#include "media_log.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_notify.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_type_const.h"
#include "ohos_account_kits.h"
#include "photo_file_utils.h"
#include "photos_dao.h"
#include "rdb_store.h"
#include "result_set_utils.h"
#include "upgrade_restore_task_report.h"
#include "userfile_manager_types.h"
#include "media_config_info_column.h"
#include "settings_data_manager.h"

#ifdef CLOUD_SYNC_MANAGER
#include "cloud_sync_manager.h"
#endif

using namespace std;
namespace OHOS {
namespace Media {
const int32_t CLONE_QUERY_COUNT = 200;
const string MEDIA_DB_PATH = "/data/storage/el2/database/rdb/media_library.db";
const std::string THM_SAVE_WITHOUT_ROTATE_PATH = "/THM_EX";
constexpr int64_t SECONDS_LEVEL_LIMIT = 1e10;
const int32_t ORIETATION_ZERO = 0;
const int32_t MIGRATE_CLOUD_THM_TYPE = 0;
const int32_t MIGRATE_CLOUD_LCD_TYPE = 1;
const int32_t MIGRATE_CLOUD_ASTC_TYPE = 2;
const int32_t RELEATED_TO_PHOTO_MAP = 1;
const int32_t MAX_ALBUM_NAME_SEQUENCE = 1000;
const unordered_map<string, unordered_set<string>> NEEDED_COLUMNS_MAP = {
    { PhotoColumn::PHOTOS_TABLE,
        {
            MediaColumn::MEDIA_ID,
            MediaColumn::MEDIA_FILE_PATH,
            MediaColumn::MEDIA_SIZE,
            MediaColumn::MEDIA_TYPE,
            MediaColumn::MEDIA_NAME,
            MediaColumn::MEDIA_DATE_ADDED,
            MediaColumn::MEDIA_DATE_MODIFIED,
            PhotoColumn::PHOTO_ORIENTATION,
            PhotoColumn::PHOTO_SUBTYPE,
            MediaColumn::MEDIA_DATE_TRASHED,
            MediaColumn::MEDIA_HIDDEN,
        }},
    { PhotoAlbumColumns::TABLE,
        {
            PhotoAlbumColumns::ALBUM_ID,
            PhotoAlbumColumns::ALBUM_TYPE,
            PhotoAlbumColumns::ALBUM_SUBTYPE,
            PhotoAlbumColumns::ALBUM_NAME,
            PhotoAlbumColumns::ALBUM_BUNDLE_NAME,
        }},
    { PhotoMap::TABLE,
        {
            PhotoMap::ALBUM_ID,
            PhotoMap::ASSET_ID,
        }},
    { ANALYSIS_ALBUM_TABLE,
        {
            PhotoAlbumColumns::ALBUM_ID,
            PhotoAlbumColumns::ALBUM_TYPE,
            PhotoAlbumColumns::ALBUM_SUBTYPE,
            PhotoAlbumColumns::ALBUM_NAME,
        }},
    { ANALYSIS_PHOTO_MAP_TABLE,
        {
            PhotoMap::ALBUM_ID,
            PhotoMap::ASSET_ID,
        }},
    { AudioColumn::AUDIOS_TABLE,
        {
            MediaColumn::MEDIA_ID,
            MediaColumn::MEDIA_FILE_PATH,
            MediaColumn::MEDIA_SIZE,
            MediaColumn::MEDIA_TYPE,
            MediaColumn::MEDIA_NAME,
            MediaColumn::MEDIA_DATE_ADDED,
            MediaColumn::MEDIA_DATE_MODIFIED,
        }},
};
const unordered_map<string, unordered_set<string>> NEEDED_COLUMNS_EXCEPTION_MAP = {
    { PhotoAlbumColumns::TABLE,
        {
            PhotoAlbumColumns::ALBUM_BUNDLE_NAME,
        }},
};
const unordered_map<string, unordered_set<string>> EXCLUDED_COLUMNS_MAP = {
    { PhotoColumn::PHOTOS_TABLE,
        {
            PhotoColumn::PHOTO_CLOUD_ID, PhotoColumn::PHOTO_DIRTY, PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE,
            PhotoColumn::PHOTO_SYNC_STATUS, PhotoColumn::PHOTO_CLOUD_VERSION, PhotoColumn::PHOTO_POSITION,
            PhotoColumn::PHOTO_THUMB_STATUS, PhotoColumn::PHOTO_CLEAN_FLAG, // cloud related
            PhotoColumn::PHOTO_THUMBNAIL_READY, PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, // thumbnail related
            PhotoColumn::PHOTO_LCD_VISIT_TIME, // lcd related
            PhotoColumn::PHOTO_CE_AVAILABLE, PhotoColumn::PHOTO_CE_STATUS_CODE, // cloud enhancement
            PhotoColumn::PHOTO_METADATA_FLAGS, // meta recovery related
            PhotoColumn::PHOTO_EXIF_ROTATE, PhotoColumn::PHOTO_TRANSCODE_TIME,
            PhotoColumn::PHOTO_TRANS_CODE_FILE_SIZE, PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE,
            PhotoColumn::PHOTO_FILE_INODE, PhotoColumn::PHOTO_STORAGE_PATH,
            PhotoColumn::PHOTO_FILE_SOURCE_TYPE, // east lake related
        }},
    { PhotoAlbumColumns::TABLE,
        {
            PhotoAlbumColumns::ALBUM_COVER_URI, PhotoAlbumColumns::ALBUM_COUNT, PhotoAlbumColumns::CONTAINS_HIDDEN,
            PhotoAlbumColumns::HIDDEN_COUNT, PhotoAlbumColumns::HIDDEN_COVER, PhotoAlbumColumns::ALBUM_IMAGE_COUNT,
            PhotoAlbumColumns::ALBUM_VIDEO_COUNT, // updated by album udpate
            PhotoAlbumColumns::ALBUM_DIRTY, PhotoAlbumColumns::ALBUM_CLOUD_ID, // cloud related
            PhotoAlbumColumns::ALBUM_ORDER, // created by trigger
        }},
    { ANALYSIS_ALBUM_TABLE,
        {
            PhotoAlbumColumns::ALBUM_COVER_URI,
            PhotoAlbumColumns::ALBUM_COUNT,
        }},
};
const unordered_map<string, unordered_map<string, string>> TABLE_QUERY_WHERE_CLAUSE_MAP = {
    { PhotoColumn::PHOTOS_TABLE,
        {
            { PhotoColumn::PHOTO_POSITION, PhotoColumn::PHOTO_POSITION + " IN (1, 3) "},
            { PhotoColumn::PHOTO_SYNC_STATUS, PhotoColumn::PHOTO_SYNC_STATUS + " = " +
                to_string(static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE)) },
            { PhotoColumn::PHOTO_CLEAN_FLAG, PhotoColumn::PHOTO_CLEAN_FLAG + " = " +
                to_string(static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN)) },
            { MediaColumn::MEDIA_TIME_PENDING, MediaColumn::MEDIA_TIME_PENDING + " = 0" },
            { PhotoColumn::PHOTO_IS_TEMP, PhotoColumn::PHOTO_IS_TEMP + " = 0" },
            { PhotoColumn::PHOTO_FILE_SOURCE_TYPE, PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " = 0" },
        }},
    { PhotoAlbumColumns::TABLE,
        {
            { PhotoAlbumColumns::ALBUM_NAME, PhotoAlbumColumns::ALBUM_NAME + " IS NOT NULL" },
            { PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumColumns::ALBUM_TYPE + " != " +
                to_string(PhotoAlbumType::SYSTEM)},
        }},
    { ANALYSIS_ALBUM_TABLE,
        {
            { PhotoAlbumColumns::ALBUM_NAME, PhotoAlbumColumns::ALBUM_NAME + " IS NOT NULL" },
            { PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumColumns::ALBUM_SUBTYPE + " IN (" +
                to_string(PhotoAlbumSubType::SHOOTING_MODE) + ", " +
                to_string(PhotoAlbumSubType::GEOGRAPHY_CITY) + ", " +
                to_string(PhotoAlbumSubType::CLASSIFY) + ")" }
        }},
};
const unordered_map<string, unordered_map<string, string>> TABLE_QUERY_WHERE_CLAUSE_MAP_WITH_CLOUD = {
    { PhotoColumn::PHOTOS_TABLE,
        {
            { PhotoColumn::PHOTO_POSITION, PhotoColumn::PHOTO_POSITION + " IN (1, 2, 3) "},
            { PhotoColumn::PHOTO_SYNC_STATUS, PhotoColumn::PHOTO_SYNC_STATUS + " = " +
                to_string(static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE)) },
            { PhotoColumn::PHOTO_CLEAN_FLAG, PhotoColumn::PHOTO_CLEAN_FLAG + " = " +
                to_string(static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN)) },
            { MediaColumn::MEDIA_TIME_PENDING, MediaColumn::MEDIA_TIME_PENDING + " = 0" },
            { PhotoColumn::PHOTO_IS_TEMP, PhotoColumn::PHOTO_IS_TEMP + " = 0" },
            { PhotoColumn::PHOTO_FILE_SOURCE_TYPE, PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " = 0" },
        }},
    { PhotoAlbumColumns::TABLE,
        {
            { PhotoAlbumColumns::ALBUM_NAME, PhotoAlbumColumns::ALBUM_NAME + " IS NOT NULL" },
            { PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumColumns::ALBUM_TYPE + " != " +
                to_string(PhotoAlbumType::SYSTEM)},
        }},
    { ANALYSIS_ALBUM_TABLE,
        {
            { PhotoAlbumColumns::ALBUM_NAME, PhotoAlbumColumns::ALBUM_NAME + " IS NOT NULL" },
            { PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumColumns::ALBUM_SUBTYPE + " IN (" +
                to_string(PhotoAlbumSubType::SHOOTING_MODE) + ", " +
                to_string(PhotoAlbumSubType::GEOGRAPHY_CITY) + ", " +
                to_string(PhotoAlbumSubType::CLASSIFY) + ")" },
        }},
};
const vector<string> CLONE_ALBUMS = { PhotoAlbumColumns::TABLE, ANALYSIS_ALBUM_TABLE };
const unordered_map<string, string> CLONE_ALBUM_MAP = {
    { PhotoAlbumColumns::TABLE, PhotoMap::TABLE },
    { ANALYSIS_ALBUM_TABLE, ANALYSIS_PHOTO_MAP_TABLE },
};
const unordered_map<string, ResultSetDataType> COLUMN_TYPE_MAP = {
    { "INT", ResultSetDataType::TYPE_INT32 },
    { "INTEGER", ResultSetDataType::TYPE_INT32 },
    { "BIGINT", ResultSetDataType::TYPE_INT64 },
    { "DOUBLE", ResultSetDataType::TYPE_DOUBLE },
    { "REAL", ResultSetDataType::TYPE_DOUBLE },
    { "TEXT", ResultSetDataType::TYPE_STRING },
    { "BLOB", ResultSetDataType::TYPE_BLOB },
};
const unordered_map<string, string> ALBUM_URI_PREFIX_MAP = {
    { PhotoAlbumColumns::TABLE, PhotoAlbumColumns::ALBUM_URI_PREFIX },
    { ANALYSIS_ALBUM_TABLE, PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX },
};

const std::unordered_map<SwitchStatus, SouthDeviceType> PHOTO_SYNC_OPTION_SOUTH_DEVICE_TYPE_MAP = {
    {SwitchStatus::CLOSE, SouthDeviceType::SOUTH_DEVICE_NULL},
    {SwitchStatus::CLOUD, SouthDeviceType::SOUTH_DEVICE_CLOUD},
    {SwitchStatus::HDC, SouthDeviceType::SOUTH_DEVICE_HDC},
};

static const unordered_set<string> TIME_INFO_COLUMNS{PhotoColumn::MEDIA_DATE_ADDED,
    PhotoColumn::MEDIA_DATE_MODIFIED,
    PhotoColumn::MEDIA_DATE_TAKEN,
    PhotoColumn::PHOTO_DETAIL_TIME,
    PhotoColumn::PHOTO_DATE_YEAR,
    PhotoColumn::PHOTO_DATE_MONTH,
    PhotoColumn::PHOTO_DATE_DAY};

static std::string GetConfigInfoInsertValue(ConfigInfoSceneId sceneId,
    const std::string key, const std::string value)
{
    return "(" + std::to_string(static_cast<int>(sceneId)) + ", '" + key + "', '" + value + "')";
}

bool CloneRestore::UpdateConfigInfo()
{
    CHECK_AND_RETURN_RET_LOG(mediaLibraryRdb_, false, "mediaLibraryRdb_ is null");

    MEDIA_INFO_LOG("current device CloneConfigInfo: %{public}s",
        srcCloneRestoreConfigInfo_.ToString().c_str());

    std::string underInsertedValues =  \
        GetConfigInfoInsertValue(ConfigInfoSceneId::CLONE_RESTORE,
            CONFIG_INFO_CLONE_PHOTO_SYNC_OPTION_KEY,
            std::to_string(static_cast<int>(srcCloneRestoreConfigInfo_.switchStatus))) + ", " +
        GetConfigInfoInsertValue(ConfigInfoSceneId::CLONE_RESTORE,
            CONFIG_INFO_CLONE_HDC_DEVICE_ID_KEY,
            srcCloneRestoreConfigInfo_.deviceId);

    std::string sqlStr = "INSERT INTO " + ConfigInfoColumn::MEDIA_CONFIG_INFO_TABLE_NAME +
        " (" + ConfigInfoColumn::MEDIA_CONFIG_INFO_SCENE_ID + ", " +
        ConfigInfoColumn::MEDIA_CONFIG_INFO_KEY + ", " +
        ConfigInfoColumn::MEDIA_CONFIG_INFO_VALUE + ") " +
        "VALUES " + underInsertedValues +
        " ON CONFLICT(" + ConfigInfoColumn::MEDIA_CONFIG_INFO_SCENE_ID + ", " +
        ConfigInfoColumn::MEDIA_CONFIG_INFO_KEY + ") DO UPDATE SET " +
        ConfigInfoColumn::MEDIA_CONFIG_INFO_VALUE + " = excluded." + ConfigInfoColumn::MEDIA_CONFIG_INFO_VALUE + ";";
    MEDIA_DEBUG_LOG("insert into configinfo sql:%{public}s", sqlStr.c_str());

    TransactionOperations trans{__func__};
    trans.SetBackupRdbStore(mediaLibraryRdb_);
    std::function<int(void)> func = [&]()->int {
        auto ret = trans.ExecuteSql(sqlStr);
        CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "fail to insert into ConfigInfo, ret:%{public}d", ret);
        return ret;
    };
    auto ret = trans.RetryTrans(func, true);
    UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_)
        .Report("UpdateConfigInfo", "",
                "sceneId: " + std::to_string(static_cast<int>(ConfigInfoSceneId::CLONE_RESTORE)) +
                ", deviceId: " + srcCloneRestoreConfigInfo_.deviceId +
                ", srcSwitchStatus: " + std::to_string(static_cast<int>(srcCloneRestoreConfigInfo_.switchStatus)) +
                ", result: " + (ret == NativeRdb::E_OK ? "success" : "failure"));
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, false, "fail to insert backupInfo into ConfigInfo.");
    return true;
}

bool CloneRestore::ShouldRestoreFromCloud()
{
    const std::string QUERY_SQL = "SELECT DISTINCT south_device_type FROM Photos "
        "WHERE position = 2 "
        "AND sync_status = 0 "
        "AND clean_flag = 0 "
        "AND time_pending = 0 "
        "AND is_temp = 0";
    std::shared_ptr<NativeRdb::ResultSet> resultSet = BackupDatabaseUtils::QuerySql(this->mediaRdb_, QUERY_SQL, {});
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "query photos south_device_type failed.");

    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_INFO_LOG("resultSet is empty");
        resultSet->Close();
        return false;
    }

    int32_t count = 0;
    int32_t errCode = resultSet->GetRowCount(count);
    if (errCode != NativeRdb::E_OK) {
        MEDIA_WARN_LOG("Can not get row count from resultSet, errCode=%{public}d", errCode);
        return false;
    }
    if (count == 0) {
        MEDIA_WARN_LOG("Can not get the count of south_device_type");
        return false;
    }
    int32_t southDeviceType = GetInt32Val(PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE, resultSet);
    resultSet->Close();

    CHECK_AND_RETURN_RET_LOG(count == 1, false, "the count of south_device_type is not 1");
    MEDIA_INFO_LOG("south_device_type: %{public}d", southDeviceType);
    return southDeviceType == static_cast<int32_t>(SouthDeviceType::SOUTH_DEVICE_CLOUD);
}

CloneRestoreConfigInfo CloneRestore::GetCloneConfigInfoFromOriginDB()
{
    CloneRestoreConfigInfo cloneConfigInfo;

    CHECK_AND_RETURN_RET_LOG(this->mediaRdb_, cloneConfigInfo, "mediaRdb_ is nullptr");

    bool isConfigInfoExistInOriginDb = false;
    CHECK_AND_RETURN_RET_LOG(BackupDatabaseUtils::isTableExist(this->mediaRdb_,
        ConfigInfoColumn::MEDIA_CONFIG_INFO_TABLE_NAME, isConfigInfoExistInOriginDb),
        cloneConfigInfo, "fail to check whether configInfo exists");
    if (!isConfigInfoExistInOriginDb) {
        MEDIA_WARN_LOG("no ConfigInfo found in origin db");
        cloneConfigInfo.switchStatus = SwitchStatus::CLOUD;
        cloneConfigInfo.deviceId = "";
    } else {
        auto configInfo = BackupDatabaseUtils::QueryConfigInfo(this->mediaRdb_);
        bool hasRequiredKeys = configInfo.count(ConfigInfoSceneId::CLONE_RESTORE) &&
            configInfo[ConfigInfoSceneId::CLONE_RESTORE].count(CONFIG_INFO_CLONE_PHOTO_SYNC_OPTION_KEY) \
            && \
            configInfo[ConfigInfoSceneId::CLONE_RESTORE].count(CONFIG_INFO_CLONE_HDC_DEVICE_ID_KEY);
        if (!hasRequiredKeys) {
            if (ShouldRestoreFromCloud()) {
                // configinfo is empty while phototype is cloud
                MEDIA_INFO_LOG("configinfo is empty");
                cloneConfigInfo.switchStatus = SwitchStatus::CLOUD;
                cloneConfigInfo.deviceId = "";
            } else {
                MEDIA_WARN_LOG("fail to find sufficient config info for CLONE_RESTORE");
                return cloneConfigInfo;
            }
        } else {
            std::string srcswitchStatusStr = \
                configInfo[ConfigInfoSceneId::CLONE_RESTORE][CONFIG_INFO_CLONE_PHOTO_SYNC_OPTION_KEY];
            CHECK_AND_RETURN_RET_LOG(STRING_SWITCH_STATUS_MAP.count(srcswitchStatusStr), cloneConfigInfo,
                "fail to parse switchStatus of source device from %{public}s", srcswitchStatusStr.c_str());
            cloneConfigInfo.switchStatus = STRING_SWITCH_STATUS_MAP.at(srcswitchStatusStr);
            cloneConfigInfo.deviceId = \
                configInfo[ConfigInfoSceneId::CLONE_RESTORE][CONFIG_INFO_CLONE_HDC_DEVICE_ID_KEY];
            CHECK_AND_RETURN_RET_LOG(CheckSouthDeviceTypeMatchSwitchStatus(cloneConfigInfo.switchStatus),
                CloneRestoreConfigInfo{}, "south_device_type and switch status do not match");
        }
    }
    cloneConfigInfo.isValid = true;
    MEDIA_INFO_LOG("Config of original DB: %{public}s", cloneConfigInfo.ToString().c_str());
    return cloneConfigInfo;
}

void CloneRestore::CheckSrcDstSwitchStatusMatch()
{
    isSrcDstSwitchStatusMatch_ = false;

    MEDIA_INFO_LOG("srcCloneRestoreConfigInfo_: %{public}s, dstCloneRestoreConfigInfo_: %{public}s",
        srcCloneRestoreConfigInfo_.ToString().c_str(), dstCloneRestoreConfigInfo_.ToString().c_str());
    
    CHECK_AND_RETURN_LOG(dstCloneRestoreConfigInfo_.isValid && srcCloneRestoreConfigInfo_.isValid,
        "dstCloneRestoreConfigInfo_/srcCloneRestoreConfigInfo_ is not valid.");
    
    CHECK_AND_RETURN_INFO_LOG(srcCloneRestoreConfigInfo_.switchStatus != SwitchStatus::CLOSE &&
        dstCloneRestoreConfigInfo_.switchStatus != SwitchStatus::CLOSE,
        "source/dst device photo sync option is off");

    CHECK_AND_RETURN_INFO_LOG(srcCloneRestoreConfigInfo_ == dstCloneRestoreConfigInfo_,
        "src device and dst device photo sync status not macth.");

    isSrcDstSwitchStatusMatch_ = true;
}

bool CloneRestore::IsCloudRestoreSatisfied()
{
    return isAccountValid_ && isSrcDstSwitchStatusMatch_;
}

void CloneRestore::ParseDstDeviceBackupInfo()
{
    MEDIA_INFO_LOG("ParseDstDeviceBackupInfo, restoreInfo_:%{public}s", restoreInfo_.c_str());
    dstDeviceBackupInfo_.hdcEnabled = false;
    
    CHECK_AND_RETURN_WARN_LOG(!restoreInfo_.empty(), "restoreInfo_ is empty");

    nlohmann::json jsonArray = nlohmann::json::parse(restoreInfo_, nullptr, false);
    CHECK_AND_RETURN_LOG(!jsonArray.is_discarded(), "ParseDstDeviceBackupInfo parse restoreInfo_ failed");

    std::string compatibilityInfoStr;
    for (const auto& item : jsonArray) {
        bool cond = (!item.contains("type") || !item.contains("detail"));
        CHECK_AND_CONTINUE(!cond);
        cond = (!item["type"].is_string() || !item["detail"].is_string());
        CHECK_AND_CONTINUE(!cond);
        if (item["type"] == "compatibility_info") {
            compatibilityInfoStr = item["detail"];
            break;
        }
    }

    CHECK_AND_RETURN_WARN_LOG(!compatibilityInfoStr.empty(), "compatibilityInfoStr is empty");

    nlohmann::json jsonObject = nlohmann::json::parse(compatibilityInfoStr, nullptr, false);
    CHECK_AND_RETURN_LOG(!jsonObject.is_discarded(), "ParseDstDeviceBackupInfo parse compatibilityInfoStr failed");

    CHECK_AND_RETURN_LOG(jsonObject.contains(BACKUP_DST_DEVICE_HDC_ENABLE_KEY) &&
        jsonObject[BACKUP_DST_DEVICE_HDC_ENABLE_KEY].is_boolean(),
        "invalid value for BackupDeviceInfo %{public}s", compatibilityInfoStr.c_str());
    dstDeviceBackupInfo_.hdcEnabled = jsonObject[BACKUP_DST_DEVICE_HDC_ENABLE_KEY].get<bool>();

    MEDIA_INFO_LOG("dstDeviceBackupInfo_.hdcEnabled: %{public}d", dstDeviceBackupInfo_.hdcEnabled);
}

bool CloneRestore::BackupPreprocess()
{
    ParseDstDeviceBackupInfo();
    if (!dstDeviceBackupInfo_.hdcEnabled && (!srcCloneRestoreConfigInfo_.isValid ||
        srcCloneRestoreConfigInfo_.switchStatus == SwitchStatus::HDC)) {
        MEDIA_INFO_LOG("dst device does not support hdc while current hdc sync is on");
        MEDIA_INFO_LOG("Start BackupDb");
        // create temp DB path
        CHECK_AND_RETURN_RET_LOG(mediaLibraryRdb_ != nullptr, false, "mediaLibraryRdb_ is nullptr!");
        std::string tmpDir = backupRestoreDir_ + "/storage/media/local/files/.backup/backup/media_temp_rdb";
        tmpDbPath_ = tmpDir + "/media_library_temp.db";
        CHECK_AND_EXECUTE(!MediaFileUtils::IsFileExists(tmpDir),
            MediaFileUtils::DeleteDir(tmpDir));
        CHECK_AND_RETURN_RET_LOG(BackupFileUtils::PreparePath(tmpDbPath_) == E_OK,
            false, "Prepare backup dir failed");
        int32_t errCode = mediaLibraryRdb_->Backup(tmpDbPath_);
        CHECK_AND_RETURN_RET_LOG(errCode == 0, E_FAIL, "rdb backup fail: %{public}d", errCode);
        MEDIA_INFO_LOG("End BackupDb");
        auto context = AbilityRuntime::Context::GetApplicationContext();
        CHECK_AND_RETURN_RET_LOG(context != nullptr, E_FAIL, "Failed to get context");
        std::shared_ptr<NativeRdb::RdbStore> backupRdb;
        int32_t err = BackupDatabaseUtils::InitDb(backupRdb, MEDIA_DATA_ABILITY_DB_NAME, tmpDbPath_, BUNDLE_NAME, true,
            context->GetArea());
        CHECK_AND_RETURN_RET_LOG(backupRdb != nullptr, E_FAIL, "Init remote medialibrary rdb fail, err = %{public}d",
            err);
        bool ret = InvalidateHdcCloudData(backupRdb);
        MEDIA_INFO_LOG("add restore dir");
        dirMappingList_.push_back("/data/storage/el2/database/rdb");
        if (!ret) {
            MEDIA_ERR_LOG("fail to delete hdc data");
            SetErrorCode(RestoreError::BACKUP_INVALIDATE_HDC_CLOUD_DATA_FAILED);
            ErrorInfo errorInfo(RestoreError::BACKUP_INVALIDATE_HDC_CLOUD_DATA_FAILED, 0, "",
                "CloneBackup clear hdc data failed");
            UpgradeRestoreTaskReport(sceneCode_, taskId_).ReportError(errorInfo);
            return false;
        }
    }
    return true;
}

bool CloneRestore::InvalidateHdcCloudData(std::shared_ptr<NativeRdb::RdbStore> &rdbStore)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore, false, "backupRdb is nullptr");

    std::unique_ptr<NativeRdb::AbsRdbPredicates> predicates =
        std::make_unique<NativeRdb::AbsRdbPredicates>(PhotoColumn::PHOTOS_TABLE);
    predicates->EqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int>(PhotoPositionType::CLOUD));
    NativeRdb::ValuesBucket updateBucket;
    updateBucket.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int>(PhotoPositionType::INVALID));
    int32_t changedRows = -1;
    CHECK_AND_RETURN_RET_LOG(BackupDatabaseUtils::Update(rdbStore, changedRows, updateBucket,
        predicates) == NativeRdb::E_OK, false, "fail to invalid hdc cloud data");
    MEDIA_INFO_LOG("InvalidateHdcCloudData %{public}d rows updated", changedRows);
    return true;
}


CloneRestoreConfigInfo CloneRestore::GetCurrentDeviceCloneConfigInfo()
{
    CloneRestoreConfigInfo cloneConfigInfo;
    cloneConfigInfo.switchStatus = SettingsDataManager::GetPhotosSyncSwitchStatus();
    bool isSyncSwitchStatusValid = (cloneConfigInfo.switchStatus != SwitchStatus::NONE);
    bool isDeviceIdValid = (cloneConfigInfo.switchStatus == SwitchStatus::HDC ?
        SettingsDataManager::GetHdcDeviceId(cloneConfigInfo.deviceId) : true);
    if (!isDeviceIdValid) {
        MEDIA_ERR_LOG("fail to get deviceId of current device");
        cloneConfigInfo.switchStatus = SwitchStatus::NONE;
        cloneConfigInfo.deviceId = "";
        cloneConfigInfo.isValid = false;
    }
    cloneConfigInfo.isValid = (isSyncSwitchStatusValid && isDeviceIdValid);
    MEDIA_INFO_LOG("GetCurrentDeviceCloneConfigInfo, %{public}s",
        cloneConfigInfo.ToString().c_str());
    return cloneConfigInfo;
}

template<typename Key, typename Value>
Value GetValueFromMap(const unordered_map<Key, Value> &map, const Key &key, const Value &defaultValue = Value())
{
    auto it = map.find(key);
    CHECK_AND_RETURN_RET(it != map.end(), defaultValue);
    return it->second;
}

CloneRestore::CloneRestore()
{
    sceneCode_ = CLONE_RESTORE_ID;
    ffrt_disable_worker_escape();
    MEDIA_INFO_LOG("Set ffrt_disable_worker_escape");
}

void CloneRestore::StartRestore(const string &backupRestoreDir, const string &upgradePath)
{
    if (WaitSouthDeviceExitTimeout()) {
        MEDIA_ERR_LOG("restore, wait south device cleaning data timeout.");
        SetErrorCode(RestoreError::RETAIN_FORCE_TIMEOUT);
        ErrorInfo errorInfo(RestoreError::RETAIN_FORCE_TIMEOUT, 0, "",
            "restore, wait south device cleaning data timeout.");
        UpgradeRestoreTaskReport(sceneCode_, taskId_).ReportError(errorInfo);
        return;
    }
    MEDIA_INFO_LOG("Start clone restore");
    SetParameterForClone();
    SetParameterForRestore();
    GetAccountValid();
    GetSyncSwitchOn();
#ifdef CLOUD_SYNC_MANAGER
    FileManagement::CloudSync::CloudSyncManager::GetInstance().StopSync("com.ohos.medialibrary.medialibrarydata");
#endif
    backupRestoreDir_ = backupRestoreDir;
    garbagePath_ = backupRestoreDir_ + "/storage/media/local/files";
    int32_t errorCode = Init(backupRestoreDir, upgradePath, true);
    MEDIA_INFO_LOG("the isAccountValid_ is %{public}d,"
        " the isSrcDstSwitchStatusMatch_ is %{public}d",
        isAccountValid_, isSrcDstSwitchStatusMatch_);
    if (errorCode == E_OK) {
        RestoreGallery();
        RestoreMusic();
        UpdateDatabase();
        (void)NativeRdb::RdbHelper::DeleteRdbStore(dbPath_);
    } else {
        SetErrorCode(RestoreError::INIT_FAILED);
        ErrorInfo errorInfo(RestoreError::INIT_FAILED, 0, errorCode);
        UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
    }
    HandleRestData();
    StopParameterForRestore();
    StopParameterForClone();
    CloseAllKvStore();
    MEDIA_INFO_LOG("End clone restore");
}

void CloneRestore::InitThumbnailStatus()
{
    std::string cloneThumbnailDir = backupRestoreDir_ + RESTORE_FILES_LOCAL_DIR + ".thumbs";
    if (!MediaFileUtils::IsFileExists(cloneThumbnailDir)) {
        MEDIA_WARN_LOG("Uncloned thumbnail dir, no need to clone thumbnail");
        return;
    }
    hasCloneThumbnailDir_ = true;
    isInitKvstoreSuccess_ = InitAllKvStore();
}

int32_t CloneRestore::Init(const string &backupRestoreDir, const string &upgradePath, bool isUpgrade)
{
    dbPath_ = backupRestoreDir_ + MEDIA_DB_PATH;
    filePath_ = backupRestoreDir_ + "/storage/media/local/files";
    if (!MediaFileUtils::IsFileExists(dbPath_)) {
        MEDIA_ERR_LOG("Media db is not exist.");
        return E_FAIL;
    }
    if (isUpgrade && BaseRestore::Init() != E_OK) {
        return E_FAIL;
    }

    auto context = AbilityRuntime::Context::GetApplicationContext();
    CHECK_AND_RETURN_RET_LOG(context != nullptr, E_FAIL, "Failed to get context");
    int32_t err = BackupDatabaseUtils::InitDb(mediaRdb_, MEDIA_DATA_ABILITY_DB_NAME, dbPath_, BUNDLE_NAME, true,
        context->GetArea());
    CHECK_AND_RETURN_RET_LOG(mediaRdb_ != nullptr, E_FAIL, "Init remote medialibrary rdb fail, err = %{public}d", err);

    BackupDatabaseUtils::CheckDbIntegrity(mediaRdb_, sceneCode_, "OLD_MEDIA_LIBRARY");
    srcCloneRestoreConfigInfo_ = GetCloneConfigInfoFromOriginDB();
    dstCloneRestoreConfigInfo_ = GetCurrentDeviceCloneConfigInfo();
    CheckSrcDstSwitchStatusMatch();
    InitThumbnailStatus();
    this->photoAlbumClone_.OnStart(this->mediaRdb_, this->mediaLibraryRdb_, IsCloudRestoreSatisfied());
    this->photosClone_.OnStart(this->mediaLibraryRdb_, this->mediaRdb_);
    cloneRestoreGeoDictionary_.Init(this->sceneCode_, this->taskId_, this->mediaLibraryRdb_, this->mediaRdb_);
    MEDIA_INFO_LOG("Init db succ.");
    return E_OK;
}

void CloneRestore::RestorePhoto()
{
    MEDIA_INFO_LOG("Start clone restore: photos");
    CHECK_AND_RETURN_LOG(IsReadyForRestore(PhotoColumn::PHOTOS_TABLE),
        "Column status is not ready for restore photo, quit");
    unordered_map<string, string> srcColumnInfoMap = BackupDatabaseUtils::GetColumnInfoMap(mediaRdb_,
        PhotoColumn::PHOTOS_TABLE);
    unordered_map<string, string> dstColumnInfoMap = BackupDatabaseUtils::GetColumnInfoMap(mediaLibraryRdb_,
        PhotoColumn::PHOTOS_TABLE);
    UpdateExistNewAddColumnSet(srcColumnInfoMap);
    if (!PrepareCommonColumnInfoMap(PhotoColumn::PHOTOS_TABLE, srcColumnInfoMap, dstColumnInfoMap)) {
        MEDIA_ERR_LOG("Prepare common column info failed");
        return;
    }
    // The begining of the restore process
    // Start clone restore
    this->photosClone_.LoadPhotoAlbums();
    // Scenario 1, clone photos from PhotoAlbum, PhotoMap and Photos.
    int totalNumberInPhotoMap = this->photosClone_.GetPhotosRowCountInPhotoMap();
    MEDIA_INFO_LOG("GetPhotosRowCountInPhotoMap, totalNumber = %{public}d", totalNumberInPhotoMap);
    totalNumber_ += static_cast<uint64_t>(totalNumberInPhotoMap);
    MEDIA_INFO_LOG("onProcess Update totalNumber_: %{public}lld", (long long)totalNumber_);
    ffrt_set_cpu_worker_max_num(ffrt::qos_utility, MAX_THREAD_NUM);
    needReportFailed_ = false;
    for (int32_t offset = 0; offset < totalNumberInPhotoMap; offset += CLONE_QUERY_COUNT) {
        ffrt::submit([this, offset]() { RestorePhotoBatch(offset, RELEATED_TO_PHOTO_MAP); }, {&offset}, {},
            ffrt::task_attr().qos(static_cast<int32_t>(ffrt::qos_utility)));
    }
    ffrt::wait();
    ProcessPhotosBatchFailedOffsets(RELEATED_TO_PHOTO_MAP);
    needReportFailed_ = false;
    // Scenario 2, clone photos from Photos only.
    int32_t totalNumber = this->photosClone_.GetPhotosRowCountNotInPhotoMap();
    MEDIA_INFO_LOG("QueryTotalNumberNot, totalNumber = %{public}d", totalNumber);
    totalNumber_ += static_cast<uint64_t>(totalNumber);
    MEDIA_INFO_LOG("onProcess Update totalNumber_: %{public}lld", (long long)totalNumber_);
    for (int32_t offset = 0; offset < totalNumber; offset += CLONE_QUERY_COUNT) {
        ffrt::submit([this, offset]() { RestorePhotoBatch(offset); }, { &offset }, {},
            ffrt::task_attr().qos(static_cast<int32_t>(ffrt::qos_utility)));
    }
    ffrt::wait();
    ProcessPhotosBatchFailedOffsets();
    this->photosClone_.OnStop(otherTotalNumber_, otherProcessStatus_);
}

void CloneRestore::GetAccountValid()
{
    string oldId = "";
    string newId = "";
    nlohmann::json jsonArr = nlohmann::json::parse(restoreInfo_, nullptr, false);
    CHECK_AND_RETURN_LOG(!jsonArr.is_discarded(), "cloud account parse failed");
    for (const auto& item : jsonArr) {
        bool cond = (!item.contains("type") || !item.contains("detail"));
        CHECK_AND_CONTINUE(!cond);
        cond = (!item["type"].is_string() ||  item["type"] != "singleAccountId");
        CHECK_AND_CONTINUE(!cond);
        oldId = item["detail"].is_string() ? item["detail"] : "";
        MEDIA_INFO_LOG("the old is %{public}s", oldId.c_str());
        break;
    }
    std::pair<bool, OHOS::AccountSA::OhosAccountInfo> ret =
        OHOS::AccountSA::OhosAccountKits::GetInstance().QueryOhosAccountInfo();
    if (ret.first) {
        OHOS::AccountSA::OhosAccountInfo& resultInfo = ret.second;
        newId = resultInfo.uid_;
    } else {
        MEDIA_ERR_LOG("new account logins failed");
        return;
    }
    MEDIA_INFO_LOG("clone the old id is %{public}s, new id is %{public}s",
        BackupFileUtils::GarbleFilePath(oldId, sceneCode_).c_str(),
        BackupFileUtils::GarbleFilePath(newId, sceneCode_).c_str());
    isAccountValid_ = (oldId != "" && oldId == newId);
}

void CloneRestore::AddToPhotosFailedOffsets(int32_t offset)
{
    if (needReportFailed_) {
        return;
    }
    std::lock_guard<ffrt::mutex> lock(photosFailedMutex_);
    photosFailedOffsets_.push_back(offset);
}

void CloneRestore::ProcessPhotosBatchFailedOffsets(int32_t isRelatedToPhotoMap)
{
    size_t vectorLen = photosFailedOffsets_.size();
    needReportFailed_ = true;
    for (size_t offset = 0; offset < vectorLen; offset++) {
        RestorePhotoBatch(photosFailedOffsets_[offset], isRelatedToPhotoMap);
    }
    photosFailedOffsets_.clear();
}

void CloneRestore::ProcessCloudPhotosFailedOffsets(int32_t isRelatedToPhotoMap)
{
    size_t vectorLen = photosFailedOffsets_.size();
    needReportFailed_ = true;
    for (size_t offset = 0; offset < vectorLen; offset++) {
        RestoreBatchForCloud(photosFailedOffsets_[offset], isRelatedToPhotoMap);
    }
    photosFailedOffsets_.clear();
}

void CloneRestore::RestorePhotoForCloud()
{
    MEDIA_INFO_LOG("singleClone start clone restore: photos");
    CHECK_AND_RETURN_LOG(IsReadyForRestore(PhotoColumn::PHOTOS_TABLE),
        "singleClone column status is not ready for restore photo, quit");
    unordered_map<string, string> srcColumnInfoMap = BackupDatabaseUtils::GetColumnInfoMap(mediaRdb_,
        PhotoColumn::PHOTOS_TABLE);
    unordered_map<string, string> dstColumnInfoMap = BackupDatabaseUtils::GetColumnInfoMap(mediaLibraryRdb_,
        PhotoColumn::PHOTOS_TABLE);
    CHECK_AND_RETURN_LOG(PrepareCommonColumnInfoMap(PhotoColumn::PHOTOS_TABLE, srcColumnInfoMap, dstColumnInfoMap),
        "singleClone Prepare common column info failed");

    this->photosClone_.LoadPhotoAlbums();
    int totalNumberInPhotoMap = this->photosClone_.GetCloudPhotosRowCountInPhotoMap();
    MEDIA_INFO_LOG("singleClone getPhotosRowCountInPhotoMap, totalNumber = %{public}d", totalNumberInPhotoMap);
    totalNumber_ += static_cast<uint64_t>(totalNumberInPhotoMap);
    MEDIA_INFO_LOG("singleClone onProcess Update totalNumber_: %{public}lld", (long long)totalNumber_);
    ffrt_set_cpu_worker_max_num(ffrt::qos_utility, MAX_THREAD_NUM);
    needReportFailed_ = false;
    for (int32_t offset = 0; offset < totalNumberInPhotoMap; offset += CLONE_QUERY_COUNT) {
        ffrt::submit([this, offset]() { RestoreBatchForCloud(offset, RELEATED_TO_PHOTO_MAP); }, {&offset}, {},
            ffrt::task_attr().qos(static_cast<int32_t>(ffrt::qos_utility)));
    }
    ffrt::wait();
    ProcessCloudPhotosFailedOffsets(RELEATED_TO_PHOTO_MAP);
    needReportFailed_ = false;
    int32_t totalNumber = this->photosClone_.GetCloudPhotosRowCountNotInPhotoMap();
    MEDIA_INFO_LOG("singleClone queryTotalNumberNot, totalNumber = %{public}d", totalNumber);
    totalNumber_ += static_cast<uint64_t>(totalNumber);
    MEDIA_INFO_LOG("singleClone onProcess Update totalNumber_: %{public}lld", (long long)totalNumber_);
    for (int32_t offset = 0; offset < totalNumber; offset += CLONE_QUERY_COUNT) {
        ffrt::submit([this, offset]() { RestoreBatchForCloud(offset); }, { &offset }, {},
            ffrt::task_attr().qos(static_cast<int32_t>(ffrt::qos_utility)));
    }
    ffrt::wait();
    ProcessCloudPhotosFailedOffsets();
    this->photosClone_.OnStop(otherTotalNumber_, otherProcessStatus_);
}

void CloneRestore::RestoreAlbum()
{
    MEDIA_INFO_LOG("Start clone restore: albums");
    SetAggregateBitThird();
    maxSearchId_ = BackupDatabaseUtils::QueryMaxId(mediaLibraryRdb_,
        ANALYSIS_SEARCH_INDEX_TABLE, SEARCH_IDX_COL_ID);
    maxBeautyFileId_ = BackupDatabaseUtils::QueryMaxId(mediaLibraryRdb_,
        ANALYSIS_BEAUTY_SCORE_TABLE, BEAUTY_SCORE_COL_FILE_ID);

    for (const auto &tableName : CLONE_ALBUMS) {
        if (!IsReadyForRestore(tableName)) {
            MEDIA_ERR_LOG("Column status of %{public}s is not ready for restore album, quit",
                BackupDatabaseUtils::GarbleInfoName(tableName).c_str());
            continue;
        }
        unordered_map<string, string> srcColumnInfoMap = BackupDatabaseUtils::GetColumnInfoMap(mediaRdb_, tableName);
        unordered_map<string, string> dstColumnInfoMap = BackupDatabaseUtils::GetColumnInfoMap(mediaLibraryRdb_,
            tableName);
        if (!PrepareCommonColumnInfoMap(tableName, srcColumnInfoMap, dstColumnInfoMap)) {
            MEDIA_ERR_LOG("Prepare common column info failed");
            continue;
        }
        GetAlbumExtraQueryWhereClause(tableName);
        int32_t totalNumber = QueryTotalNumber(tableName);
        MEDIA_INFO_LOG(
            "QueryAlbumTotalNumber, tableName=%{public}s, totalNumber=%{public}d", tableName.c_str(), totalNumber);
        for (int32_t offset = 0; offset < totalNumber; offset += CLONE_QUERY_COUNT) {
            vector<AlbumInfo> albumInfos = QueryAlbumInfos(tableName, offset);
            this->photoAlbumClone_.TRACE_LOG(tableName, albumInfos);
            InsertAlbum(albumInfos, tableName);
        }
        UpdateSystemAlbumColumns(tableName);
    }

    cloneRestoreGeoDictionary_.RestoreAlbums();
}

int32_t CloneRestore::GetHighlightCloudMediaCnt()
{
    const std::string QUERY_SQL = "SELECT COUNT(1) AS count FROM AnalysisAlbum AS a "
        "INNER JOIN AnalysisPhotoMap AS m ON a.album_id = m.map_album "
        "INNER JOIN Photos AS p ON p.file_id = m.map_asset "
        "WHERE a.album_subtype IN (4104, 4105) AND p.position = 2";
    std::shared_ptr<NativeRdb::ResultSet> resultSet = BackupDatabaseUtils::QuerySql(this->mediaRdb_, QUERY_SQL, {});
    bool cond = (resultSet == nullptr);
    CHECK_AND_RETURN_RET_LOG(!cond, -1, "query count of highlight cloud media failed.");
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return -1;
    }
    int32_t cnt = GetInt32Val("count", resultSet);
    MEDIA_INFO_LOG("GetHighlightCloudMediaCnt is %{public}d", cnt);
    resultSet->Close();
    return cnt;
}

void CloneRestore::RestoreHighlightAlbums()
{
    int32_t highlightCloudMediaCnt = GetHighlightCloudMediaCnt();
    UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_)
        .Report("CLONE_RESTORE_HIGHLIGHT_CHECK", "",
            "highlightCloudMediaCnt: " + std::to_string(highlightCloudMediaCnt) +
            ", isCloudRestoreSatisfied: " + std::to_string(IsCloudRestoreSatisfied()));
    CHECK_AND_RETURN(highlightCloudMediaCnt == 0 || IsCloudRestoreSatisfied());

    CloneRestoreHighlight cloneRestoreHighlight;
    CloneRestoreHighlight::InitInfo initInfo = { sceneCode_, taskId_, mediaLibraryRdb_, mediaRdb_, backupRestoreDir_,
        photoInfoMap_ };
    cloneRestoreHighlight.Init(initInfo);
    cloneRestoreHighlight.Restore();

    CloneRestoreCVAnalysis cloneRestoreCVAnalysis;
    cloneRestoreCVAnalysis.Init(sceneCode_, taskId_, mediaLibraryRdb_, mediaRdb_, backupRestoreDir_);
    cloneRestoreCVAnalysis.RestoreAlbums(cloneRestoreHighlight);
    StoreHighlightAlbumMappings(cloneRestoreHighlight);

    cloneRestoreHighlight.ReportCloneRestoreHighlightTask();
}

void CloneRestore::MoveMigrateFile(std::vector<FileInfo> &fileInfos, int64_t &fileMoveCount,
    int64_t &videoFileMoveCount)
{
    vector<std::string> moveFailedData;
    for (size_t i = 0; i < fileInfos.size(); i++) {
        if (!MediaFileUtils::IsFileExists(fileInfos[i].filePath) ||
            fileInfos[i].cloudPath.empty() || !fileInfos[i].needMove) {
            fileInfos[i].needVisible = false;
            fileInfos[i].needMove = false;
            MEDIA_ERR_LOG("File is not visible");
            continue;
        }

        int32_t errCode = MoveAsset(fileInfos[i]);
        if (errCode != E_OK) {
            fileInfos[i].needUpdate = false;
            fileInfos[i].needVisible = false;
            MEDIA_ERR_LOG("MoveFile failed, filePath = %{public}s, error:%{public}s",
                BackupFileUtils::GarbleFilePath(fileInfos[i].filePath, CLONE_RESTORE_ID, garbagePath_).c_str(),
                strerror(errno));
            UpdateFailedFiles(fileInfos[i].fileType, fileInfos[i], RestoreError::MOVE_FAILED);
            ErrorInfo errorInfo(RestoreError::MOVE_FAILED, 1, std::to_string(errCode),
                BackupLogUtils::FileInfoToString(sceneCode_, fileInfos[i]));
            UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
            moveFailedData.push_back(fileInfos[i].cloudPath);
            continue;
        }
        fileMoveCount++;
        videoFileMoveCount += fileInfos[i].fileType == MediaType::MEDIA_TYPE_VIDEO;
    }
    DeleteMoveFailedData(moveFailedData);
    SetVisiblePhoto(fileInfos);
    migrateFileNumber_ += fileMoveCount;
    migrateVideoFileNumber_ += videoFileMoveCount;
}

static void UpdateThumbnailStatusToFailed(std::shared_ptr<NativeRdb::RdbStore> &rdbStore, std::string id,
    bool isThumbnailStatusNeedUpdate, bool isLcdStatusNeedUpdate)
{
    bool cond = (rdbStore == nullptr || id.empty());
    CHECK_AND_RETURN_LOG(!cond, "singleClone rdb is nullptr or id is empty");

    NativeRdb::ValuesBucket values;
    int changedRows;
    if (isThumbnailStatusNeedUpdate) {
        values.PutLong(PhotoColumn::PHOTO_THUMBNAIL_READY, RESTORE_THUMBNAIL_READY_NO_THUMBNAIL);
        values.PutInt(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, RESTORE_THUMBNAIL_VISIBLE_FALSE);
    }
    if (isLcdStatusNeedUpdate) {
        values.PutInt(PhotoColumn::PHOTO_LCD_VISIT_TIME, RESTORE_LCD_VISIT_TIME_NO_LCD);
    }
    int32_t err = rdbStore->Update(changedRows, PhotoColumn::PHOTOS_TABLE,
        values, MEDIA_DATA_DB_ID + " = ?", vector<string> { id });
    CHECK_AND_PRINT_LOG(err == NativeRdb::E_OK, "singleClone rdbStore Update failed! %{public}d", err);
}

void CloneRestore::GetCloudPhotoFileExistFlag(const FileInfo &fileInfo, CloudPhotoFileExistFlag &resultExistFlag)
{
    std::string dirPath = GetThumbnailLocalPath(fileInfo.cloudPath);
    CHECK_AND_RETURN_LOG(MediaFileUtils::IsFileExists(dirPath),
        "GetCloudPhotoFileExistFlag %{public}s not exist!", fileInfo.cloudPath.c_str());
    
    std::string lcdPath = dirPath + "/LCD.jpg";
    resultExistFlag.isLcdExist = MediaFileUtils::IsFileExists(lcdPath) ? true : false;
    std::string thmPath = dirPath + "/THM.jpg";
    resultExistFlag.isThmExist = MediaFileUtils::IsFileExists(thmPath) ? true : false;
    std::string astcPath = dirPath + "/THM_ASTC.astc";
    resultExistFlag.isDayAstcExist = MediaFileUtils::IsFileExists(astcPath) ? true : false;

    if (HasExThumbnail(fileInfo)) {
        std::string exLcdPath = dirPath + "/THM_EX/LCD.jpg";
        resultExistFlag.isExLcdExist = MediaFileUtils::IsFileExists(exLcdPath) ? true : false;
        std::string exThmPath = dirPath + "/THM_EX/THM.jpg";
        resultExistFlag.isExThmExist = MediaFileUtils::IsFileExists(exThmPath) ? true : false;
    }
    MEDIA_DEBUG_LOG("%{public}s, isexist lcd:%{public}d, thm:%{public}d, astc:%{public}d,"
        "yearastc:%{public}d, exlcd:%{public}d, exthm:%{public}d",
        dirPath.c_str(), resultExistFlag.isLcdExist, resultExistFlag.isThmExist,
        resultExistFlag.isDayAstcExist, resultExistFlag.isYearAstcExist,
        resultExistFlag.isExLcdExist, resultExistFlag.isExThmExist);
}

void CloneRestore::CloudPhotoFilesVerify(const std::vector<FileInfo> &fileInfos, std::vector<FileInfo> &LCDNotFound,
    std::vector<FileInfo> &THMNotFound, unordered_map<string, CloudPhotoFileExistFlag> &resultExistMap)
{
    for (size_t i = 0; i < fileInfos.size(); i++) {
        CHECK_AND_CONTINUE(fileInfos[i].needMove);
        CloudPhotoFileExistFlag fileExistFlag;
        unordered_map<string, CloudPhotoFileExistFlag>::iterator iter = resultExistMap.find(fileInfos[i].cloudPath);
        CHECK_AND_EXECUTE(iter == resultExistMap.end(), fileExistFlag = iter->second);
        GetCloudPhotoFileExistFlag(fileInfos[i], fileExistFlag);
        resultExistMap[fileInfos[i].cloudPath] = fileExistFlag;
        if (HasExThumbnail(fileInfos[i])) {
            CHECK_AND_EXECUTE(fileExistFlag.isExLcdExist, LCDNotFound.push_back(fileInfos[i]));
            CHECK_AND_EXECUTE(fileExistFlag.isExThmExist, THMNotFound.push_back(fileInfos[i]));
        } else {
            CHECK_AND_EXECUTE(fileExistFlag.isLcdExist, LCDNotFound.push_back(fileInfos[i]));
            CHECK_AND_EXECUTE(fileExistFlag.isThmExist, THMNotFound.push_back(fileInfos[i]));
        }
    }
}

void CloneRestore::MoveMigrateCloudFile(std::vector<FileInfo> &fileInfos, int32_t &fileMoveCount,
    int32_t &videoFileMoveCount, int32_t sceneCode)
{
    MEDIA_INFO_LOG("singleClone MoveMigrateCloudFile start");
    unordered_map<string, CloudPhotoFileExistFlag> resultExistMap;
    for (size_t i = 0; i < fileInfos.size(); i++) {
        CHECK_AND_CONTINUE(fileInfos[i].needMove);
        MoveCloudThumbnailDir(fileInfos[i]);
        CHECK_AND_CONTINUE_ERR_LOG(isInitKvstoreSuccess_,
            "singleClone isInitKvstoreSuccess_ false, id:%{public}d, path:%{public}s",
            fileInfos[i].fileIdNew, MediaFileUtils::DesensitizePath(fileInfos[i].cloudPath).c_str());

        CHECK_AND_CONTINUE_ERR_LOG(fileInfos[i].thumbnailReady >= RESTORE_THUMBNAIL_READY_SUCCESS,
            "singleClone Astc does not exist, id:%{public}d, path:%{public}s",
                fileInfos[i].fileIdNew, MediaFileUtils::DesensitizePath(fileInfos[i].cloudPath).c_str());
        if (MoveAstc(fileInfos[i]) != E_OK) {
            UpdateThumbnailStatusToFailed(mediaLibraryRdb_, to_string(fileInfos[i].fileIdNew), true, false);
            MEDIA_ERR_LOG("Move astc failed, id:%{public}d, path:%{public}s",
                fileInfos[i].fileIdNew, MediaFileUtils::DesensitizePath(fileInfos[i].cloudPath).c_str());
        }
        CloudPhotoFileExistFlag tmpFlag;
        tmpFlag.isYearAstcExist = true;
        resultExistMap[fileInfos[i].cloudPath] = tmpFlag;
        videoFileMoveCount += fileInfos[i].fileType == MediaType::MEDIA_TYPE_VIDEO;
    }
    std::vector<FileInfo> LCDNotFound;
    std::vector<FileInfo> THMNotFound;
    CloudPhotoFilesVerify(fileInfos, LCDNotFound, THMNotFound, resultExistMap);
    MEDIA_INFO_LOG("singleClone LCDNotFound:%{public}zu, THMNotFound:%{public}zu",
        LCDNotFound.size(), THMNotFound.size());
    std::vector<std::string> dentryFailedLCD;
    std::vector<std::string> dentryFailedThumb;
    CHECK_AND_EXECUTE(BatchCreateDentryFile(LCDNotFound, dentryFailedLCD, DENTRY_INFO_LCD) != E_OK,
        HandleFailData(fileInfos, dentryFailedLCD, DENTRY_INFO_LCD));
    CHECK_AND_EXECUTE(BatchCreateDentryFile(THMNotFound, dentryFailedThumb, DENTRY_INFO_THM) != E_OK,
        HandleFailData(fileInfos, dentryFailedThumb, DENTRY_INFO_THM));

    BatchUpdateFileInfoData(fileInfos, resultExistMap);
    fileMoveCount = SetVisiblePhoto(fileInfos);
    successCloudMetaNumber_ += fileMoveCount;
    migrateFileNumber_ += fileMoveCount;
    migrateVideoFileNumber_ += videoFileMoveCount;
    MEDIA_INFO_LOG("singleClone MoveMigrateCloudFile end");
}

bool CloneRestore::CheckDestDbHasCriticalTypeColumn()
{
    unordered_map<string, string> dstColumnInfoMap = BackupDatabaseUtils::GetColumnInfoMap(mediaLibraryRdb_,
        PhotoColumn::PHOTOS_TABLE);
    return HasColumn(dstColumnInfoMap, PhotoColumn::PHOTO_CRITICAL_TYPE);
}
 
bool CloneRestore::CheckSrcDbHasCriticalTypeColumn()
{
    unordered_map<string, string> srcColumnInfoMap = BackupDatabaseUtils::GetColumnInfoMap(mediaRdb_,
        PhotoColumn::PHOTOS_TABLE);
    return HasColumn(srcColumnInfoMap, PhotoColumn::PHOTO_CRITICAL_TYPE);
}
 
void CloneRestore::UpdateCriticalTypeForSamePhotos(vector<FileInfo> &fileInfos)
{
    for (FileInfo &fileInfo : fileInfos) {
        if (fileInfo.fileIdNew <= 0 || fileInfo.isNew) {
            continue;
        }

        if (CheckSrcDbHasCriticalTypeColumn() &&
            fileInfo.criticalType != static_cast<int32_t>(CriticalType::UNKNOWN_CRITICAL_TYPE)) {
            auto ret = mediaLibraryRdb_->ExecuteSql("UPDATE Photos SET critical = " +
                                            std::to_string(fileInfo.criticalType) +
                                            " WHERE file_id " + std::to_string(fileInfo.fileIdNew));
            if (ret != NativeRdb::E_OK) {
                MEDIA_ERR_LOG("Update failed for file_id: %{public}d with critical_type: %{public}d, error: %{public}d",
                              fileInfo.fileIdNew, fileInfo.criticalType, ret);
                continue;
            }
        }
    }
}

int CloneRestore::InsertPhoto(vector<FileInfo> &fileInfos)
{
    CHECK_AND_RETURN_RET_LOG(mediaLibraryRdb_ != nullptr, E_OK, "mediaLibraryRdb_ is null");
    CHECK_AND_RETURN_RET_LOG(!fileInfos.empty(), E_OK, "fileInfos are empty");
    int64_t startGenerate = MediaFileUtils::UTCTimeMilliSeconds();
    vector<NativeRdb::ValuesBucket> values = GetInsertValues(CLONE_RESTORE_ID, fileInfos, SourceType::PHOTOS);
    UpdateCriticalTypeForSamePhotos(fileInfos);
    int64_t startInsertPhoto = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t photoRowNum = 0;
    int32_t errCode = BatchInsertWithRetry(PhotoColumn::PHOTOS_TABLE, values, photoRowNum);
    if (errCode != E_OK) {
        if (needReportFailed_) {
            UpdateFailedFiles(fileInfos, RestoreError::INSERT_FAILED);
            ErrorInfo errorInfo(RestoreError::INSERT_FAILED, static_cast<int32_t>(fileInfos.size()), errCode);
            UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
        }
        return errCode;
    }
    migrateDatabaseNumber_ += photoRowNum;

    int64_t startInsertRelated = MediaFileUtils::UTCTimeMilliSeconds();
    InsertPhotoRelated(fileInfos, SourceType::PHOTOS);

    int64_t startMove = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t fileMoveCount = 0;
    int64_t videoFileMoveCount = 0;
    MoveMigrateFile(fileInfos, fileMoveCount, videoFileMoveCount);
    int64_t startUpdate = MediaFileUtils::UTCTimeMilliSeconds();
    UpdatePhotosByFileInfoMap(mediaLibraryRdb_, fileInfos);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("generate cost %{public}ld, insert %{public}ld assets cost %{public}ld, insert photo related cost "
        "%{public}ld, and move %{public}ld files (%{public}ld + %{public}ld) cost %{public}ld. update cost %{public}ld",
        (long)(startInsertPhoto - startGenerate), (long)photoRowNum, (long)(startInsertRelated - startInsertPhoto),
        (long)(startMove - startInsertRelated), (long)fileMoveCount, (long)(fileMoveCount - videoFileMoveCount),
        (long)videoFileMoveCount, (long)(startUpdate - startMove), (long)(end - startUpdate));
    return E_OK;
}

vector<NativeRdb::ValuesBucket> CloneRestore::GetInsertValues(int32_t sceneCode, vector<FileInfo> &fileInfos,
    int32_t sourceType)
{
    vector<NativeRdb::ValuesBucket> values;
    for (size_t i = 0; i < fileInfos.size(); i++) {
        int32_t errCode = BackupFileUtils::IsFileValid(fileInfos[i].filePath, CLONE_RESTORE_ID);
        if (errCode != E_OK) {
            ErrorInfo errorInfo(RestoreError::FILE_INVALID, 1, std::to_string(errCode),
                BackupLogUtils::FileInfoToString(sceneCode, fileInfos[i]));
            UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
            continue;
        }
        CHECK_AND_CONTINUE(PrepareCloudPath(PhotoColumn::PHOTOS_TABLE, fileInfos[i]));
        if (fileInfos[i].isNew) {
            NativeRdb::ValuesBucket value = GetInsertValue(fileInfos[i], fileInfos[i].cloudPath, sourceType);
            values.emplace_back(value);
        }
    }
    return values;
}

vector<NativeRdb::ValuesBucket> CloneRestore::GetCloudInsertValues(int32_t sceneCode, vector<FileInfo> &fileInfos,
    int32_t sourceType)
{
    MEDIA_INFO_LOG("singleClone GetCloudInsertValues: %{public}zu", fileInfos.size());
    vector<NativeRdb::ValuesBucket> values;
    for (size_t i = 0; i < fileInfos.size(); i++) {
        CHECK_AND_CONTINUE(PrepareCloudPath(PhotoColumn::PHOTOS_TABLE, fileInfos[i]));
        NativeRdb::ValuesBucket value = GetCloudInsertValue(fileInfos[i], fileInfos[i].cloudPath, sourceType);
        fileInfos[i].isNew = true;
        values.emplace_back(value);
    }
    return values;
}

void CloneRestore::HandleRestData(void)
{
    MEDIA_INFO_LOG("Start to handle rest data in native.");
}

vector<FileInfo> CloneRestore::QueryFileInfos(int32_t offset, int32_t isRelatedToPhotoMap)
{
    vector<FileInfo> result;
    result.reserve(CLONE_QUERY_COUNT);
    std::shared_ptr<NativeRdb::ResultSet> resultSet;
    if (isRelatedToPhotoMap == 1) {
        resultSet = this->photosClone_.GetPhotosInPhotoMap(offset, CLONE_QUERY_COUNT);
    } else {
        resultSet = this->photosClone_.GetPhotosNotInPhotoMap(offset, CLONE_QUERY_COUNT);
    }
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSql is null.");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        FileInfo fileInfo;
        fileInfo.isRelatedToPhotoMap = isRelatedToPhotoMap;
        CHECK_AND_EXECUTE(!ParseResultSet(resultSet, fileInfo), result.emplace_back(fileInfo));
    }
    return result;
}

vector<FileInfo> CloneRestore::QueryCloudFileInfos(int32_t offset, int32_t isRelatedToPhotoMap)
{
    MEDIA_INFO_LOG("singleClone QueryCloudFileInfos");
    vector<FileInfo> result;
    result.reserve(CLONE_QUERY_COUNT);
    std::shared_ptr<NativeRdb::ResultSet> resultSet;
    if (isRelatedToPhotoMap == 1) {
        resultSet = this->photosClone_.GetCloudPhotosInPhotoMap(offset, CLONE_QUERY_COUNT);
    } else {
        resultSet = this->photosClone_.GetCloudPhotosNotInPhotoMap(offset, CLONE_QUERY_COUNT);
    }
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "singleClone Query resultSql is null.");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        FileInfo fileInfo;
        fileInfo.isRelatedToPhotoMap = isRelatedToPhotoMap;
        CHECK_AND_EXECUTE(!ParseResultSet(resultSet, fileInfo), result.emplace_back(fileInfo));
    }
    return result;
}

bool CloneRestore::ParseResultSet(const shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &fileInfo,
    string dbName)
{
    return ParseResultSet(PhotoColumn::PHOTOS_TABLE, resultSet, fileInfo);
}

int32_t CloneRestore::QueryTotalNumber(const string &tableName)
{
    CHECK_AND_RETURN_RET(tableName != PhotoAlbumColumns::TABLE,
        this->photoAlbumClone_.GetPhotoAlbumCountInOriginalDb());
    CHECK_AND_RETURN_RET(tableName != PhotoColumn::PHOTOS_TABLE,
        this->photosClone_.GetPhotosRowCountNotInPhotoMap());
    string querySql = "SELECT " + MEDIA_COLUMN_COUNT_1 + " FROM " + tableName;
    string whereClause = GetQueryWhereClauseByTable(tableName);
    querySql += whereClause.empty() ? "" : " WHERE " + whereClause;
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, querySql);
    if (resultSet == nullptr) {
        return 0;
    }
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return 0;
    }
    int32_t result = GetInt32Val(MEDIA_COLUMN_COUNT_1, resultSet);
    resultSet->Close();
    return result;
}

vector<AlbumInfo> CloneRestore::QueryAlbumInfos(const string &tableName, int32_t offset)
{
    vector<AlbumInfo> result;
    result.reserve(CLONE_QUERY_COUNT);
    std::shared_ptr<NativeRdb::ResultSet> resultSet = nullptr;
    if (tableName == PhotoAlbumColumns::TABLE) {
        resultSet = this->photoAlbumClone_.GetPhotoAlbumInOriginalDb(offset, CLONE_QUERY_COUNT);
    } else {
        string querySql = "SELECT * FROM " + tableName;
        string whereClause = GetQueryWhereClauseByTable(tableName);
        querySql += whereClause.empty() ? "" : " WHERE " + whereClause;
        querySql += " LIMIT " + to_string(offset) + ", " + to_string(CLONE_QUERY_COUNT);
        resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, querySql);
    }
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query resultSql is null.");
        return result;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        AlbumInfo albumInfo;
        if (ParseAlbumResultSet(tableName, resultSet, albumInfo)) {
            result.emplace_back(albumInfo);
        }
    }
    resultSet->Close();
    return result;
}

bool CloneRestore::ParseAlbumResultSet(const string &tableName, const shared_ptr<NativeRdb::ResultSet> &resultSet,
    AlbumInfo &albumInfo)
{
    albumInfo.albumIdOld = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
    albumInfo.albumName = GetStringVal(PhotoAlbumColumns::ALBUM_NAME, resultSet);
    albumInfo.albumType = static_cast<PhotoAlbumType>(GetInt32Val(PhotoAlbumColumns::ALBUM_TYPE, resultSet));
    albumInfo.albumSubType = static_cast<PhotoAlbumSubType>(GetInt32Val(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet));
    albumInfo.lPath = GetStringVal(PhotoAlbumColumns::ALBUM_LPATH, resultSet);
    albumInfo.albumBundleName = GetStringVal(PhotoAlbumColumns::ALBUM_BUNDLE_NAME, resultSet);
    albumInfo.dateModified = GetInt64Val(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, resultSet);
    if (tableName == PhotoAlbumColumns::TABLE) {
        albumInfo.uploadStatus = GetInt32Val(PhotoAlbumColumns::UPLOAD_STATUS, resultSet);
    }
    auto commonColumnInfoMap = GetValueFromMap(tableCommonColumnInfoMap_, tableName);
    for (auto it = commonColumnInfoMap.begin(); it != commonColumnInfoMap.end(); ++it) {
        string columnName = it->first;
        string columnType = it->second;
        GetValFromResultSet(resultSet, albumInfo.valMap, columnName, columnType);
    }
    return true;
}

void CloneRestore::AnalyzeSource()
{
    MEDIA_INFO_LOG("analyze source later");
}

int32_t CloneRestore::MovePicture(FileInfo &fileInfo)
{
    bool deleteOriginalFile = fileInfo.isRelatedToPhotoMap == 1 ? false : true;
    string localPath = BackupFileUtils::GetReplacedPathByPrefixType(PrefixType::CLOUD, PrefixType::LOCAL,
        fileInfo.cloudPath);
    int32_t opRet = E_FAIL;
    if (deleteOriginalFile) {
        opRet = this->MoveFile(fileInfo.filePath, localPath);
    } else {
        opRet = this->CopyFile(fileInfo.filePath, localPath);
    }
    CHECK_AND_RETURN_RET_LOG(opRet == E_OK, E_FAIL,
        "Move photo file failed, filePath = %{public}s, deleteOriginalFile = %{public}d",
        BackupFileUtils::GarbleFilePath(fileInfo.filePath, CLONE_RESTORE_ID, garbagePath_).c_str(),
        deleteOriginalFile);
    MediaFileUtils::UpdateModifyTimeInMsec(localPath, fileInfo.dateModified);
    return E_OK;
}

int32_t CloneRestore::MoveMovingPhotoVideo(FileInfo &fileInfo)
{
    CHECK_AND_RETURN_RET(fileInfo.subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ||
        fileInfo.effectMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY), E_OK);
    bool deleteOriginalFile = fileInfo.isRelatedToPhotoMap == 1 ? false : true;
    std::string localPath = BackupFileUtils::GetReplacedPathByPrefixType(PrefixType::CLOUD, PrefixType::LOCAL,
        fileInfo.cloudPath);
    std::string srcLocalVideoPath = MediaFileUtils::GetMovingPhotoVideoPath(fileInfo.filePath);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(srcLocalVideoPath), E_OK,
        "video of moving photo does not exist: %{private}s", srcLocalVideoPath.c_str());
    std::string localVideoPath = MediaFileUtils::GetMovingPhotoVideoPath(localPath);
    int32_t opVideoRet = E_FAIL;
    if (deleteOriginalFile) {
        opVideoRet = this->MoveFile(srcLocalVideoPath, localVideoPath);
    } else {
        opVideoRet = this->CopyFile(srcLocalVideoPath, localVideoPath);
    }
    CHECK_AND_RETURN_RET_LOG(opVideoRet == E_OK, E_FAIL, "Move video of moving photo failed");
    MediaFileUtils::UpdateModifyTimeInMsec(localVideoPath, fileInfo.dateModified);
    return E_OK;
}

int32_t CloneRestore::MoveEditedData(FileInfo &fileInfo)
{
    bool deleteOriginalFile = fileInfo.isRelatedToPhotoMap == 1 ? false : true;
    string localPath =
        BackupFileUtils::GetReplacedPathByPrefixType(PrefixType::CLOUD, PrefixType::LOCAL, fileInfo.cloudPath);
    string srcEditDataPath = this->backupRestoreDir_ +
        BackupFileUtils::GetFullPathByPrefixType(PrefixType::LOCAL_EDIT_DATA, fileInfo.relativePath);
    string dstEditDataPath = BackupFileUtils::GetReplacedPathByPrefixType(
        PrefixType::CLOUD, PrefixType::LOCAL_EDIT_DATA, fileInfo.cloudPath);
    bool cond = (this->IsFilePathExist(srcEditDataPath) &&
        this->MoveDirectory(srcEditDataPath, dstEditDataPath, deleteOriginalFile) != E_OK);
    CHECK_AND_RETURN_RET_LOG(!cond, E_FAIL, "Move editData file failed");
    return E_OK;
}

std::string CloneRestore::GetThumbnailLocalPath(const string path)
{
    size_t cloudDirLength = RESTORE_FILES_CLOUD_DIR.length();
    if (path.length() <= cloudDirLength || path.substr(0, cloudDirLength).compare(RESTORE_FILES_CLOUD_DIR) != 0) {
        return "";
    }

    std::string suffixStr = path.substr(cloudDirLength);
    return RESTORE_FILES_LOCAL_DIR + ".thumbs/" + suffixStr;
}

int32_t CloneRestore::MoveAstc(FileInfo &fileInfo)
{
    bool cond = (oldMonthKvStorePtr_ == nullptr || oldYearKvStorePtr_ == nullptr ||
        newMonthKvStorePtr_ == nullptr || newYearKvStorePtr_ == nullptr);
    CHECK_AND_RETURN_RET_LOG(!cond, E_FAIL, "Kvstore is nullptr");
    if (fileInfo.fileIdOld <= 0 || fileInfo.fileIdNew <= 0) {
        MEDIA_ERR_LOG("Old fileId:%{public}d or new fileId:%{public}d is invalid",
            fileInfo.fileIdOld, fileInfo.fileIdNew);
        return E_FAIL;
    }

    string oldKey;
    string newKey;
    if (!MediaFileUtils::GenerateKvStoreKey(to_string(fileInfo.fileIdOld), fileInfo.oldAstcDateKey, oldKey) ||
        !MediaFileUtils::GenerateKvStoreKey(to_string(fileInfo.fileIdNew), fileInfo.newAstcDateKey, newKey)) {
        return E_FAIL;
    }

    std::vector<uint8_t> monthValue;
    std::vector<uint8_t> yearValue;
    if (oldMonthKvStorePtr_->Query(oldKey, monthValue) != E_OK ||
        newMonthKvStorePtr_->Insert(newKey, monthValue) != E_OK) {
        MEDIA_ERR_LOG("MonthValue move failed, fileID %{public}s", newKey.c_str());
        return E_FAIL;
    }
    if (oldYearKvStorePtr_->Query(oldKey, yearValue) != E_OK ||
        newYearKvStorePtr_->Insert(newKey, yearValue) != E_OK) {
        MEDIA_ERR_LOG("YearValue move failed, fileID %{public}s", newKey.c_str());
        return E_FAIL;
    }
    if (fileInfo.isRelatedToPhotoMap != 1) {
        oldMonthKvStorePtr_->Delete(oldKey);
        oldYearKvStorePtr_->Delete(oldKey);
    }
    return E_OK;
}

int32_t CloneRestore::MoveThumbnailDir(FileInfo &fileInfo)
{
    string thumbnailOldDir = backupRestoreDir_ + RESTORE_FILES_LOCAL_DIR + ".thumbs" + fileInfo.relativePath;
    string thumbnailNewDir = GetThumbnailLocalPath(fileInfo.cloudPath);
    if (fileInfo.relativePath.empty() || thumbnailNewDir.empty()) {
        MEDIA_ERR_LOG("Old path:%{public}s or new path:%{public}s is invalid",
            fileInfo.relativePath.c_str(), MediaFileUtils::DesensitizePath(fileInfo.cloudPath).c_str());
        return E_FAIL;
    }
    if (!MediaFileUtils::IsDirectory(thumbnailOldDir)) {
        MEDIA_ERR_LOG("Old dir is not a direcrory or does not exist, errno:%{public}d, dir:%{public}s",
            errno, MediaFileUtils::DesensitizePath(thumbnailOldDir).c_str());
        return E_FAIL;
    }
    CHECK_AND_RETURN_RET_LOG(BackupFileUtils::PreparePath(thumbnailNewDir) == E_OK, E_FAIL,
        "Prepare thumbnail dir path failed");
    bool cond = (MediaFileUtils::IsFileExists(thumbnailNewDir) && !MediaFileUtils::DeleteDir(thumbnailNewDir));
    CHECK_AND_RETURN_RET_LOG(!cond, E_FAIL, "Delete thumbnail new dir failed, errno:%{public}d", errno);

    int32_t opRet = E_FAIL;
    if (fileInfo.isRelatedToPhotoMap != 1) {
        opRet = MediaFileUtils::ModifyAsset(thumbnailOldDir, thumbnailNewDir);
    } else {
        opRet = MediaFileUtils::CopyDirectory(thumbnailOldDir, thumbnailNewDir);
    }
    if (opRet != E_OK) {
        CHECK_AND_RETURN_RET(MediaFileUtils::IsFileExists(thumbnailNewDir), opRet);
        MEDIA_WARN_LOG("MoveThumbnailDir failed but thumbnailNewDir exists");
        CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteDir(thumbnailNewDir),
            "Delete existential thumbnailNewDir failed, errno:%{public}d", errno);
        return opRet;
    }
    return E_OK;
}

int32_t CloneRestore::MoveCloudThumbnailDir(FileInfo &fileInfo)
{
    string thumbnailOldDir = backupRestoreDir_ + RESTORE_FILES_LOCAL_DIR + ".thumbs" + fileInfo.relativePath;
    string thumbnailNewDir = GetThumbnailLocalPath(fileInfo.cloudPath);
    bool cond = (fileInfo.relativePath.empty() || thumbnailNewDir.empty());
    CHECK_AND_RETURN_RET_LOG(!cond, E_FAIL, "singleCloud Old path:%{public}s or new path:%{public}s is invalid",
        fileInfo.relativePath.c_str(), MediaFileUtils::DesensitizePath(fileInfo.cloudPath).c_str());

    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsDirectory(thumbnailOldDir), E_FAIL,
        "singleCloud Old dir is not a direcrory or does not exist, errno:%{public}d, dir:%{public}s",
        errno, MediaFileUtils::DesensitizePath(thumbnailOldDir).c_str());

    CHECK_AND_RETURN_RET_LOG(BackupFileUtils::PreparePath(thumbnailNewDir) == E_OK, E_FAIL,
        "singleCloud Prepare thumbnail dir path failed");
    cond = (MediaFileUtils::IsFileExists(thumbnailNewDir) && !MediaFileUtils::DeleteDir(thumbnailNewDir));
    CHECK_AND_RETURN_RET_LOG(!cond, E_FAIL, "singleCloud Delete thumbnail new dir failed, errno:%{public}d", errno);

    int32_t opRet = E_FAIL;
    if (fileInfo.isRelatedToPhotoMap != 1) {
        opRet = MediaFileUtils::ModifyAsset(thumbnailOldDir, thumbnailNewDir);
    } else {
        opRet = MediaFileUtils::CopyDirectory(thumbnailOldDir, thumbnailNewDir);
    }
    if (opRet != E_OK) {
        CHECK_AND_RETURN_RET(MediaFileUtils::IsFileExists(thumbnailNewDir), opRet);
        MEDIA_WARN_LOG("singleCloud MoveThumbnailDir failed but thumbnailNewDir exists");
        return opRet;
    }
    return E_OK;
}

/**
 * The processing logic of the MoveThumbnail function must match the logic of the GetThumbnailInsertValue function.
 * If the status indicates that the thumbnail does not exist, the thumbnail does not need to be cloned and
 * the status of the thumbnail needs to be set to the initial status in the GetThumbnailInsertValue function.
 * If the status indicates that the thumbnail exists but the thumbnail fails to be transferred,
 * the thumbnail status needs to be set to the initial status.
*/
int32_t CloneRestore::MoveThumbnail(FileInfo &fileInfo)
{
    CHECK_AND_RETURN_RET(hasCloneThumbnailDir_, E_NO_SUCH_FILE);
    if (fileInfo.thumbnailReady < RESTORE_THUMBNAIL_READY_SUCCESS &&
        fileInfo.lcdVisitTime < RESTORE_LCD_VISIT_TIME_SUCCESS) {
        MEDIA_INFO_LOG("Thumbnail dose not exist, id:%{public}d, path:%{public}s",
            fileInfo.fileIdNew, MediaFileUtils::DesensitizePath(fileInfo.cloudPath).c_str());
        return E_NO_SUCH_FILE;
    }
    if (MoveThumbnailDir(fileInfo) != E_OK) {
        UpdateThumbnailStatusToFailed(mediaLibraryRdb_, to_string(fileInfo.fileIdNew), true, true);
        MEDIA_ERR_LOG("Move thumbnail failed, id:%{public}d, path:%{public}s",
            fileInfo.fileIdNew, MediaFileUtils::DesensitizePath(fileInfo.cloudPath).c_str());
        return E_FAIL;
    }

    CHECK_AND_RETURN_RET(isInitKvstoreSuccess_, E_NO_SUCH_FILE);
    CHECK_AND_RETURN_RET_LOG(fileInfo.thumbnailReady >= RESTORE_THUMBNAIL_READY_SUCCESS, E_NO_SUCH_FILE,
        "Astc does not exist, id:%{public}d, path:%{public}s",
        fileInfo.fileIdNew, MediaFileUtils::DesensitizePath(fileInfo.cloudPath).c_str());
    if (MoveAstc(fileInfo) != E_OK) {
        UpdateThumbnailStatusToFailed(mediaLibraryRdb_, to_string(fileInfo.fileIdNew), true, false);
        MEDIA_ERR_LOG("Move astc failed, id:%{public}d, path:%{public}s",
            fileInfo.fileIdNew, MediaFileUtils::DesensitizePath(fileInfo.cloudPath).c_str());
        return E_FAIL;
    }

    return E_OK;
}

int32_t CloneRestore::MoveAsset(FileInfo &fileInfo)
{
    // Picture files.
    int32_t optRet = this->MovePicture(fileInfo);
    CHECK_AND_RETURN_RET(optRet == E_OK, E_FAIL);
    // Video files of moving photo.
    optRet = this->MoveMovingPhotoVideo(fileInfo);
    CHECK_AND_RETURN_RET(optRet == E_OK, E_FAIL);
    // Edit Data.
    optRet = this->MoveEditedData(fileInfo);
    CHECK_AND_RETURN_RET(optRet == E_OK, E_FAIL);
    // Thumbnail of photos.
    this->MoveThumbnail(fileInfo);

    MediaLibraryPhotoOperations::StoreThumbnailAndEditSize(to_string(fileInfo.fileIdNew), fileInfo.cloudPath);
    return E_OK;
}

bool CloneRestore::IsFilePathExist(const string &filePath) const
{
    if (!MediaFileUtils::IsFileExists(filePath)) {
        MEDIA_DEBUG_LOG("%{private}s doesn't exist", filePath.c_str());
        return false;
    }
    bool cond = (MediaFileUtils::IsDirectory(filePath) && MediaFileUtils::IsDirEmpty(filePath));
    CHECK_AND_RETURN_RET_LOG(!cond, false, "%{private}s is an empty directory", filePath.c_str());
    return true;
}

void CloneRestore::GetThumbnailInsertValue(const FileInfo &fileInfo, NativeRdb::ValuesBucket &values)
{
    if (!hasCloneThumbnailDir_) {
        // If there is no thumbnail directory, all statuses of thumbnail are set to the initial status.
        values.PutInt(PhotoColumn::PHOTO_LCD_VISIT_TIME, RESTORE_LCD_VISIT_TIME_NO_LCD);
        values.PutLong(PhotoColumn::PHOTO_THUMBNAIL_READY, RESTORE_THUMBNAIL_READY_NO_THUMBNAIL);
        values.PutInt(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, RESTORE_THUMBNAIL_VISIBLE_FALSE);
        return;
    }

    // The LCD status is same as the origin status.
    values.PutInt(PhotoColumn::PHOTO_LCD_VISIT_TIME, fileInfo.lcdVisitTime);
    if (!isInitKvstoreSuccess_ || fileInfo.thumbnailReady < RESTORE_THUMBNAIL_READY_SUCCESS) {
        // The kvdb does not exist or the THM status indicates that there is no THM.
        // Therefore, the THM status needs to be set to the initial status.
        values.PutLong(PhotoColumn::PHOTO_THUMBNAIL_READY, RESTORE_THUMBNAIL_READY_NO_THUMBNAIL);
        values.PutInt(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, RESTORE_THUMBNAIL_VISIBLE_FALSE);
        return;
    }
    values.PutLong(PhotoColumn::PHOTO_THUMBNAIL_READY, fileInfo.thumbnailReady);
    values.PutInt(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, RESTORE_THUMBNAIL_VISIBLE_TRUE);
}

void CloneRestore::GetCloudThumbnailInsertValue(const FileInfo &fileInfo, NativeRdb::ValuesBucket &values)
{
    values.PutInt(PhotoColumn::PHOTO_POSITION, fileInfo.position);
    values.PutString(PhotoColumn::PHOTO_CLOUD_ID, fileInfo.cloudId);
    CHECK_AND_EXECUTE(!PHOTO_SYNC_OPTION_SOUTH_DEVICE_TYPE_MAP.count(dstCloneRestoreConfigInfo_.switchStatus),
        values.PutInt(PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE,
        static_cast<int32_t>(PHOTO_SYNC_OPTION_SOUTH_DEVICE_TYPE_MAP.at(dstCloneRestoreConfigInfo_.switchStatus))));
    values.PutInt(PhotoColumn::PHOTO_CLOUD_VERSION, fileInfo.cloudVersion);
    values.PutInt(PhotoColumn::PHOTO_DIRTY, 0);
    values.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, 0);
    values.PutInt(PhotoColumn::PHOTO_THUMB_STATUS, RESTORE_THUMBNAIL_STATUS_NOT_ALL);
    values.PutInt(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_BACKUP));
    values.PutLong(PhotoColumn::PHOTO_THUMBNAIL_READY, 0);
    values.PutInt(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, RESTORE_THUMBNAIL_VISIBLE_FALSE);
    values.PutInt(PhotoColumn::PHOTO_LCD_VISIT_TIME, 0);
}

void CloneRestore::PrepareShootingModeVal(const FileInfo &fileInfo, NativeRdb::ValuesBucket &values)
{
    values.Delete(PhotoColumn::PHOTO_SHOOTING_MODE);
    auto it = fileInfo.valMap.find(PhotoColumn::PHOTO_SHOOTING_MODE_TAG);
    if (it == fileInfo.valMap.end()) {
        values.PutString(PhotoColumn::PHOTO_SHOOTING_MODE, "");
        return;
    }
    string shootingModeTag = get<string>(it->second);
    values.PutString(PhotoColumn::PHOTO_SHOOTING_MODE,
        ShootingModeAlbum::MapShootingModeTagToShootingMode(shootingModeTag));
}

void CloneRestore::GetInsertValueFromValMap(const FileInfo &fileInfo, NativeRdb::ValuesBucket &values)
{
    unordered_map<string, string> commonColumnInfoMap = GetValueFromMap(tableCommonColumnInfoMap_,
        PhotoColumn::PHOTOS_TABLE);
    for (auto it = fileInfo.valMap.begin(); it != fileInfo.valMap.end(); ++it) {
        string columnName = it->first;
        if (TIME_INFO_COLUMNS.count(columnName) > 0) {
            continue;
        }
        auto columnVal = it->second;
        if (columnName == PhotoColumn::PHOTO_EDIT_TIME) {
            PrepareEditTimeVal(values, get<int64_t>(columnVal), fileInfo, commonColumnInfoMap);
            continue;
        }
        PrepareCommonColumnVal(values, columnName, columnVal, commonColumnInfoMap);
    }
    PrepareShootingModeVal(fileInfo, values);
}

void CloneRestore::SetTimeInfo(const FileInfo &info, NativeRdb::ValuesBucket &values)
{
    int64_t dateAdded = info.dateAdded > SECONDS_LEVEL_LIMIT ? info.dateAdded : info.dateAdded * MSEC_TO_SEC;
    int64_t dateModified =
        info.dateModified > SECONDS_LEVEL_LIMIT ? info.dateModified : info.dateModified * MSEC_TO_SEC;
    int64_t dateTaken = info.dateTaken > SECONDS_LEVEL_LIMIT ? info.dateTaken : info.dateTaken * MSEC_TO_SEC;

    dateAdded = PhotoFileUtils::NormalizeTimestamp(dateAdded, MediaFileUtils::UTCTimeMilliSeconds());
    dateModified = PhotoFileUtils::NormalizeTimestamp(dateModified, dateAdded);
    dateTaken = PhotoFileUtils::NormalizeTimestamp(dateTaken, min(dateAdded, dateModified));

    values.Put(PhotoColumn::MEDIA_DATE_ADDED, dateAdded);
    values.Put(PhotoColumn::MEDIA_DATE_MODIFIED, dateModified);
    values.Put(PhotoColumn::MEDIA_DATE_TAKEN, dateTaken);

    std::string detailTime = info.detailTime;
    const auto [normalizeDateTaken, normalizeDetailTime] =
        PhotoFileUtils::ExtractTimeInfo(detailTime, PhotoColumn::PHOTO_DETAIL_TIME_FORMAT);
    if (normalizeDateTaken < MIN_MILSEC_TIMESTAMP || normalizeDateTaken > MAX_MILSEC_TIMESTAMP ||
        abs(dateTaken - normalizeDateTaken) > MAX_TIMESTAMP_DIFF) {
        MEDIA_ERR_LOG("invalid detailTime: %{public}s, dateTaken: %{public}lld",
            detailTime.c_str(),
            static_cast<long long>(dateTaken));
        detailTime = MediaFileUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DETAIL_TIME_FORMAT, dateTaken);
    } else {
        detailTime = normalizeDetailTime;
    }

    values.Put(PhotoColumn::PHOTO_DETAIL_TIME, detailTime);

    const auto [dateYear, dateMonth, dateDay] = PhotoFileUtils::ExtractYearMonthDay(detailTime);
    values.Put(PhotoColumn::PHOTO_DATE_YEAR, dateYear);
    values.Put(PhotoColumn::PHOTO_DATE_MONTH, dateMonth);
    values.Put(PhotoColumn::PHOTO_DATE_DAY, dateDay);
}

NativeRdb::ValuesBucket CloneRestore::GetInsertValue(const FileInfo &fileInfo, const string &newPath,
    int32_t sourceType)
{
    NativeRdb::ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_FILE_PATH, newPath);
    values.PutLong(MediaColumn::MEDIA_SIZE, fileInfo.fileSize);
    values.PutInt(MediaColumn::MEDIA_TYPE, fileInfo.fileType);
    values.PutString(MediaColumn::MEDIA_NAME, fileInfo.displayName);
    values.PutString(PhotoColumn::PHOTO_MEDIA_SUFFIX, ScannerUtils::GetFileExtension(fileInfo.displayName));
    // [time info]date_added, date_modified, date_taken, detail_time, date_year, date_month, date_day
    SetTimeInfo(fileInfo, values);
    values.PutInt(PhotoColumn::PHOTO_ORIENTATION, fileInfo.orientation); // photos need orientation
    values.PutInt(PhotoColumn::PHOTO_EXIF_ROTATE, fileInfo.exifRotate);
    values.PutInt(PhotoColumn::PHOTO_SUBTYPE, fileInfo.subtype);
    // use owner_album_id to mark the album id which the photo is in.
    values.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, fileInfo.ownerAlbumId);
    // Only SOURCE album has package_name and owner_package.
    values.PutString(MediaColumn::MEDIA_PACKAGE_NAME, fileInfo.packageName);
    values.PutString(MediaColumn::MEDIA_OWNER_PACKAGE, fileInfo.bundleName);
    if (fileInfo.packageName.empty() && fileInfo.bundleName.empty()) {
    // package_name and owner_package are empty, clear owner_appid
        values.PutString(MediaColumn::MEDIA_OWNER_APPID, "");
    }
    values.PutInt(PhotoColumn::PHOTO_QUALITY, fileInfo.photoQuality);
    values.PutInt(PhotoColumn::STAGE_VIDEO_TASK_STATUS, static_cast<int32_t>(StageVideoTaskStatus::NO_NEED_TO_STAGE));
    values.PutLong(MediaColumn::MEDIA_DATE_TRASHED, fileInfo.recycledTime);
    values.PutInt(MediaColumn::MEDIA_HIDDEN, fileInfo.hidden);
    values.PutString(PhotoColumn::PHOTO_SOURCE_PATH, fileInfo.sourcePath);
    values.PutInt(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_BACKUP));
    GetThumbnailInsertValue(fileInfo, values);
    GetInsertValueFromValMap(fileInfo, values);
    if (CheckDestDbHasCriticalTypeColumn()) {
        values.PutInt(PhotoColumn::PHOTO_CRITICAL_TYPE, fileInfo.criticalType);
    }
    return values;
}

NativeRdb::ValuesBucket CloneRestore::GetCloudInsertValue(const FileInfo &fileInfo, const string &newPath,
    int32_t sourceType)
{
    NativeRdb::ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_FILE_PATH, newPath);
    values.PutLong(MediaColumn::MEDIA_SIZE, fileInfo.fileSize);
    values.PutInt(MediaColumn::MEDIA_TYPE, fileInfo.fileType);
    values.PutString(MediaColumn::MEDIA_NAME, fileInfo.displayName);
    // [time info]date_added, date_modified, date_taken, detail_time, date_year, date_month, date_day
    SetTimeInfo(fileInfo, values);
    values.PutInt(PhotoColumn::PHOTO_ORIENTATION, fileInfo.orientation); // photos need orientation
    values.PutInt(PhotoColumn::PHOTO_EXIF_ROTATE, fileInfo.exifRotate);
    values.PutInt(PhotoColumn::PHOTO_SUBTYPE, fileInfo.subtype);
    // use owner_album_id to mark the album id which the photo is in.
    values.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, fileInfo.ownerAlbumId);
    // Only SOURCE album has package_name and owner_package.
    values.PutString(MediaColumn::MEDIA_PACKAGE_NAME, fileInfo.packageName);
    values.PutString(MediaColumn::MEDIA_OWNER_PACKAGE, fileInfo.bundleName);

    bool cond = (fileInfo.packageName.empty() && fileInfo.bundleName.empty());
    // package_name and owner_package are empty, clear owner_appid
    CHECK_AND_EXECUTE(!cond, values.PutString(MediaColumn::MEDIA_OWNER_APPID, ""));
    values.PutInt(PhotoColumn::PHOTO_QUALITY, fileInfo.photoQuality);
    values.PutLong(MediaColumn::MEDIA_DATE_TRASHED, fileInfo.recycledTime);
    values.PutInt(MediaColumn::MEDIA_HIDDEN, fileInfo.hidden);
    values.PutString(PhotoColumn::PHOTO_SOURCE_PATH, fileInfo.sourcePath);
    values.PutInt(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_BACKUP));
    GetCloudThumbnailInsertValue(fileInfo, values);
    GetInsertValueFromValMap(fileInfo, values);
    return values;
}

bool CloneRestore::PrepareCommonColumnInfoMap(const string &tableName,
    const unordered_map<string, string> &srcColumnInfoMap, const unordered_map<string, string> &dstColumnInfoMap)
{
    auto neededColumns = GetValueFromMap(NEEDED_COLUMNS_MAP, tableName);
    auto neededColumnsException = GetValueFromMap(NEEDED_COLUMNS_EXCEPTION_MAP, tableName);
    auto excludedColumns = GetValueFromMap(EXCLUDED_COLUMNS_MAP, tableName);
    auto &commonColumnInfoMap = tableCommonColumnInfoMap_[tableName];
    CHECK_AND_RETURN_RET_LOG(HasColumns(dstColumnInfoMap, neededColumns), false, "Destination lack needed columns");
    for (auto it = dstColumnInfoMap.begin(); it != dstColumnInfoMap.end(); ++it) {
        if (!HasSameColumn(srcColumnInfoMap, it->first, it->second) || excludedColumns.count(it->first) > 0) {
            continue;
        }
        if (neededColumns.count(it->first) > 0 && (neededColumnsException.empty() ||
            neededColumnsException.count(it->first) == 0)) {
            continue;
        }
        commonColumnInfoMap[it->first] = it->second;
    }
    MEDIA_INFO_LOG("Table %{public}s has %{public}zu common columns",
        BackupDatabaseUtils::GarbleInfoName(tableName).c_str(), commonColumnInfoMap.size());
    return true;
}

bool CloneRestore::HasSameColumn(const unordered_map<string, string> &columnInfoMap, const string &columnName,
    const string &columnType)
{
    auto it = columnInfoMap.find(columnName);
    return it != columnInfoMap.end() && it->second == columnType;
}

void CloneRestore::GetValFromResultSet(const shared_ptr<NativeRdb::ResultSet> &resultSet,
    unordered_map<string, variant<int32_t, int64_t, double, string>> &valMap, const string &columnName,
    const string &columnType)
{
    int32_t columnIndex = 0;
    int32_t errCode = resultSet->GetColumnIndex(columnName, columnIndex);
    CHECK_AND_RETURN_LOG(errCode == 0, "Get column index errCode: %{public}d", errCode);
    bool isNull = false;
    errCode = resultSet->IsColumnNull(columnIndex, isNull);
    if (errCode || isNull) {
        return;
    }
    ResultSetDataType dataType = GetValueFromMap(COLUMN_TYPE_MAP, columnType, ResultSetDataType::TYPE_NULL);
    switch (dataType) {
        case ResultSetDataType::TYPE_INT32: {
            int32_t int32Val;
            if (resultSet->GetInt(columnIndex, int32Val) == E_OK) {
                valMap[columnName] = int32Val;
            }
            break;
        }
        case ResultSetDataType::TYPE_INT64: {
            int64_t int64Val;
            if (resultSet->GetLong(columnIndex, int64Val) == E_OK) {
                valMap[columnName] = int64Val;
            }
            break;
        }
        case ResultSetDataType::TYPE_DOUBLE: {
            double doubleVal;
            if (resultSet->GetDouble(columnIndex, doubleVal) == E_OK) {
                valMap[columnName] = doubleVal;
            }
            break;
        }
        case ResultSetDataType::TYPE_STRING: {
            string stringVal;
            if (resultSet->GetString(columnIndex, stringVal) == E_OK) {
                valMap[columnName] = stringVal;
            }
            break;
        }
        default:
            MEDIA_ERR_LOG("No such column type: %{public}s", columnType.c_str());
    }
}

void CloneRestore::PrepareCommonColumnVal(NativeRdb::ValuesBucket &values, const string &columnName,
    const variant<int32_t, int64_t, double, string> &columnVal,
    const unordered_map<string, string> &commonColumnInfoMap) const
{
    string columnType = GetValueFromMap(commonColumnInfoMap, columnName);
    CHECK_AND_RETURN_LOG(!columnType.empty(), "No such column %{public}s", columnName.c_str());
    ResultSetDataType dataType = GetValueFromMap(COLUMN_TYPE_MAP, columnType, ResultSetDataType::TYPE_NULL);
    switch (dataType) {
        case ResultSetDataType::TYPE_INT32: {
            values.PutInt(columnName, get<int32_t>(columnVal));
            break;
        }
        case ResultSetDataType::TYPE_INT64: {
            values.PutLong(columnName, get<int64_t>(columnVal));
            break;
        }
        case ResultSetDataType::TYPE_DOUBLE: {
            values.PutDouble(columnName, get<double>(columnVal));
            break;
        }
        case ResultSetDataType::TYPE_STRING: {
            values.PutString(columnName, get<string>(columnVal));
            break;
        }
        default:
            MEDIA_ERR_LOG("No such column type: %{public}s", columnType.c_str());
    }
}

void CloneRestore::GetQueryWhereClause(const string &tableName, const unordered_map<string, string> &columnInfoMap)
{
    unordered_map<string, string> queryWhereClauseMap;
    if (IsCloudRestoreSatisfied()) {
        queryWhereClauseMap = GetValueFromMap(TABLE_QUERY_WHERE_CLAUSE_MAP_WITH_CLOUD, tableName);
    } else {
        queryWhereClauseMap = GetValueFromMap(TABLE_QUERY_WHERE_CLAUSE_MAP, tableName);
    }
    
    if (queryWhereClauseMap.empty()) {
        return;
    }
    string &queryWhereClause = tableQueryWhereClauseMap_[tableName];
    queryWhereClause.clear();
    for (auto it = queryWhereClauseMap.begin(); it != queryWhereClauseMap.end(); ++it) {
        CHECK_AND_CONTINUE(columnInfoMap.count(it->first) != 0);
        if (!queryWhereClause.empty()) {
            queryWhereClause += " AND ";
        }
        queryWhereClause += it->second + " ";
    }
}

void CloneRestore::GetAlbumExtraQueryWhereClause(const string &tableName)
{
    string mapTableName = GetValueFromMap(CLONE_ALBUM_MAP, tableName);
    CHECK_AND_RETURN_LOG(!mapTableName.empty(), "Get map of table %{public}s failed",
        BackupDatabaseUtils::GarbleInfoName(tableName).c_str());
    string albumQueryWhereClause = "EXISTS (SELECT " + PhotoMap::ASSET_ID + " FROM " + mapTableName + " WHERE " +
        PhotoMap::ALBUM_ID + " = " + PhotoAlbumColumns::ALBUM_ID + " AND EXISTS (SELECT " + MediaColumn::MEDIA_ID +
        " FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " + MediaColumn::MEDIA_ID + " = " + PhotoMap::ASSET_ID;
    string photoQueryWhereClause = GetValueFromMap(tableQueryWhereClauseMap_, PhotoColumn::PHOTOS_TABLE);
    if (!photoQueryWhereClause.empty()) {
        albumQueryWhereClause += " AND " + photoQueryWhereClause;
    }
    albumQueryWhereClause += "))";
    tableExtraQueryWhereClauseMap_[tableName] = albumQueryWhereClause;
}

NativeRdb::ValuesBucket CloneRestore::GetInsertValue(const AlbumInfo &albumInfo, const string &tableName) const
{
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, static_cast<int32_t>(albumInfo.albumType));
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, static_cast<int32_t>(albumInfo.albumSubType));
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, albumInfo.albumName);

    if (tableName == PhotoAlbumColumns::TABLE) {
        values.PutLong(PhotoAlbumColumns::ALBUM_DATE_MODIFIED,
            (albumInfo.dateModified ? albumInfo.dateModified : MediaFileUtils::UTCTimeMilliSeconds()));
        values.PutInt(PhotoAlbumColumns::UPLOAD_STATUS, this->photoAlbumClone_.FindUploadStatus(albumInfo));
    }

    unordered_map<string, string> commonColumnInfoMap = GetValueFromMap(tableCommonColumnInfoMap_, tableName);
    for (auto it = albumInfo.valMap.begin(); it != albumInfo.valMap.end(); ++it) {
        string columnName = it->first;
        auto columnVal = it->second;
        PrepareCommonColumnVal(values, columnName, columnVal, commonColumnInfoMap);
    }
    return values;
}

void CloneRestore::BatchQueryPhoto(vector<FileInfo> &fileInfos)
{
    string selection;
    unordered_map<string, size_t> fileIndexMap;
    for (size_t index = 0; index < fileInfos.size(); index++) {
        CHECK_AND_CONTINUE(!fileInfos[index].cloudPath.empty());
        BackupDatabaseUtils::UpdateSelection(selection, fileInfos[index].cloudPath, true);
        fileIndexMap[fileInfos[index].cloudPath] = index;
    }
    string querySql = "SELECT " + MediaColumn::MEDIA_ID + ", " + MediaColumn::MEDIA_FILE_PATH + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + " FROM " + PhotoColumn::PHOTOS_TABLE +
        " WHERE " + MediaColumn::MEDIA_FILE_PATH + " IN (" + selection + ")";
    querySql += " LIMIT " + to_string(fileIndexMap.size());
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaLibraryRdb_, querySql);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Query resultSql is null.");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        string cloudPath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        string dateTaken = GetStringVal(MediaColumn::MEDIA_DATE_TAKEN, resultSet);
        CHECK_AND_CONTINUE_ERR_LOG(fileId > 0, "Get fileId invalid: %{public}d", fileId);
        CHECK_AND_CONTINUE(fileIndexMap.count(cloudPath) != 0);
        size_t index = fileIndexMap.at(cloudPath);
        fileInfos[index].fileIdNew = fileId;
        fileInfos[index].newAstcDateKey = dateTaken;
    }
    resultSet->Close();
    BackupDatabaseUtils::UpdateAssociateFileId(mediaLibraryRdb_, fileInfos);
}

void CloneRestore::UpdateAlbumOrderColumns(const AlbumInfo &albumInfo, const string &tableName)
{
    CHECK_AND_RETURN(tableName == PhotoAlbumColumns::TABLE);
    CHECK_AND_RETURN_LOG(mediaLibraryRdb_ != nullptr, "rdbStore is null");

    std::unique_ptr<NativeRdb::AbsRdbPredicates> predicates =
        make_unique<NativeRdb::AbsRdbPredicates>(PhotoAlbumColumns::TABLE);
    predicates->EqualTo(PhotoAlbumColumns::ALBUM_ID, albumInfo.albumIdNew);

    NativeRdb::ValuesBucket values;
    unordered_map<string, string> commonColumnInfoMap = GetValueFromMap(tableCommonColumnInfoMap_, tableName);
    for (const auto &columns : PhotoAlbumColumns::ORDER_COLUMN_STYLE_MAP) {
        for (const auto &columnName : columns.second) {
            auto iter = albumInfo.valMap.find(columnName);
            CHECK_AND_EXECUTE(iter == albumInfo.valMap.end(),
                PrepareCommonColumnVal(values, columnName, iter->second, commonColumnInfoMap));
        }
    }

    int32_t changeRows = 0;
    int32_t ret = BackupDatabaseUtils::Update(mediaLibraryRdb_, changeRows, values, predicates);
    bool cond = (changeRows < 0 || ret < 0);
    CHECK_AND_RETURN_LOG(!cond,
        "Failed to update albumOrder columns, ret: %{public}d, updateRows: %{public}d", ret, changeRows);
}

void CloneRestore::UpdateSystemAlbumColumns(const string &tableName)
{
    CHECK_AND_RETURN(tableName == PhotoAlbumColumns::TABLE);
    CHECK_AND_RETURN_LOG(this->mediaRdb_ != nullptr, "original rdbStore is null");

    const std::string querySql =
        "SELECT PhotoAlbum.* FROM PhotoAlbum WHERE PhotoAlbum.album_type = 1024 ORDER BY PhotoAlbum.album_id";
    const vector<string> bindArgs = {};
    auto resultSet = this->mediaRdb_->QuerySql(querySql, bindArgs);
    CHECK_AND_RETURN_LOG(resultSet != nullptr,
        "Failed to query system album! querySql = %{public}s", querySql.c_str());
    
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        AlbumInfo albumInfo;
        bool ret = ParseAlbumResultSet(tableName, resultSet, albumInfo);
        CHECK_AND_CONTINUE_INFO_LOG(ret == true, "Update system album columns failed");
        albumInfo.albumIdNew = albumInfo.albumIdOld;
        UpdateAlbumOrderColumns(albumInfo, tableName);
    }
    resultSet->Close();
}

void CloneRestore::PopulateSystemAlbumIdMap()
{
    MEDIA_INFO_LOG("Mapping system albums to tableAlbumIdMap_");
    
    CHECK_AND_RETURN_LOG(this->mediaRdb_ != nullptr, "source rdbStore is null");
    CHECK_AND_RETURN_LOG(this->mediaLibraryRdb_ != nullptr, "destination rdbStore is null");
    
    std::string dstSql = "SELECT album_id, album_type, album_subtype FROM PhotoAlbum "
                        "WHERE album_type = " + std::to_string(PhotoAlbumType::SYSTEM) + " "
                        "ORDER BY album_id";
    
    auto dstResultSet = mediaLibraryRdb_->QuerySql(dstSql);
    CHECK_AND_RETURN_LOG(dstResultSet != nullptr, "Failed to query destination system albums");
    
    // Build a map: (album_type, album_subtype) -> album_id for destination albums
    std::map<std::pair<int32_t, int32_t>, int32_t> dstAlbumMap;
    while (dstResultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t newAlbumId = GetInt32Val("album_id", dstResultSet);
        int32_t albumType = GetInt32Val("album_type", dstResultSet);
        int32_t albumSubtype = GetInt32Val("album_subtype", dstResultSet);
        
        std::pair<int32_t, int32_t> key = std::make_pair(albumType, albumSubtype);
        // If multiple albums with same type/subtype exist, keep the first one
        if (dstAlbumMap.find(key) == dstAlbumMap.end()) {
            dstAlbumMap[key] = newAlbumId;
        }
    }
    dstResultSet->Close();

    std::string srcSql = "SELECT album_id, album_type, album_subtype FROM PhotoAlbum "
                        "WHERE album_type = " + std::to_string(PhotoAlbumType::SYSTEM) + " "
                        "ORDER BY album_id";
    
    auto srcResultSet = mediaRdb_->QuerySql(srcSql);
    CHECK_AND_RETURN_LOG(srcResultSet != nullptr, "Failed to query source system albums");
    
    auto& albumIdMap = tableAlbumIdMap_[PhotoAlbumColumns::TABLE];
    int32_t mappedCount = 0;
    
    while (srcResultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t oldAlbumId = GetInt32Val("album_id", srcResultSet);
        int32_t albumType = GetInt32Val("album_type", srcResultSet);
        int32_t albumSubtype = GetInt32Val("album_subtype", srcResultSet);
        
        // Skip if already mapped (shouldn't happen for system albums)
        if (albumIdMap.find(oldAlbumId) != albumIdMap.end()) {
            continue;
        }
        
        std::pair<int32_t, int32_t> key = std::make_pair(albumType, albumSubtype);
        auto dstIt = dstAlbumMap.find(key);
        if (dstIt != dstAlbumMap.end()) {
            int32_t newAlbumId = dstIt->second;
            albumIdMap[oldAlbumId] = newAlbumId;
            mappedCount++;
            
            MEDIA_DEBUG_LOG("Mapped system album (subtype=%{public}d): old=%{public}d -> new=%{public}d",
                albumSubtype, oldAlbumId, newAlbumId);
        }
    }
    srcResultSet->Close();
    
    MEDIA_INFO_LOG("System album mapping completed. Mapped %{public}d albums", mappedCount);
}

void CloneRestore::InsertAlbum(vector<AlbumInfo> &albumInfos, const string &tableName)
{
    CHECK_AND_RETURN_LOG(mediaLibraryRdb_ != nullptr, "mediaLibraryRdb_ is null");
    CHECK_AND_RETURN_LOG(!albumInfos.empty(), "albumInfos are empty");
    int64_t startInsert = MediaFileUtils::UTCTimeMilliSeconds();
    vector<string> albumIds{};
    vector<NativeRdb::ValuesBucket> values = GetInsertValues(albumInfos, albumIds, tableName);
    UpdatePhotoAlbumDateModified(albumIds, tableName);
    int64_t rowNum = 0;
    int32_t errCode = BatchInsertWithRetry(tableName, values, rowNum);
    CHECK_AND_RETURN_LOG(errCode == E_OK, "Batc insert failed");
    migrateDatabaseAlbumNumber_ += rowNum;
    int64_t startQuery = MediaFileUtils::UTCTimeMilliSeconds();
    BatchQueryAlbum(albumInfos, tableName);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("insert %{public}ld albums cost %{public}ld, query cost %{public}ld.", (long)rowNum,
        (long)(startQuery - startInsert), (long)(end - startQuery));
}

void CloneRestore::UpdateSourceAlbumName(bool &isUinque, vector<AlbumInfo> &albumInfos,
    vector<string> &repetedAlbumName, size_t index)
{
    CHECK_AND_RETURN_LOG(index < albumInfos.size(), "index exceeds the limit");
    string albumName = "";
    int32_t sequence = 1;
    while (!isUinque && sequence < MAX_ALBUM_NAME_SEQUENCE) {
        albumName = albumInfos[index].albumName + " " + std::to_string(sequence);
        MEDIA_INFO_LOG("check album sequence: %{public}d, albumName: %{private}s",
            sequence, albumName.c_str());
        sequence++;
        isUinque = CheckAlbumNameUnique(albumName, repetedAlbumName);
        CHECK_AND_PRINT_LOG(sequence < MAX_ALBUM_NAME_SEQUENCE, "sequence exceed the limit");
    }
    albumInfos[index].albumName = albumName;
    repetedAlbumName.push_back(StringUtils::ToLower(albumName));
}

vector<NativeRdb::ValuesBucket> CloneRestore::GetInsertValues(vector<AlbumInfo> &albumInfos, vector<string> &albumIds,
    const string &tableName)
{
    vector<NativeRdb::ValuesBucket> values;
    vector<string> repetedAlbumName;
    for (size_t i = 0; i < albumInfos.size(); i++) {
        if (HasSameAlbum(albumInfos[i], tableName)) {
            albumIds.emplace_back(to_string(albumInfos[i].albumIdNew));
            UpdateAlbumOrderColumns(albumInfos[i], tableName);
            CHECK_AND_EXECUTE(
                tableName != PhotoAlbumColumns::TABLE, this->photoAlbumClone_.UpdatePhotoAlbum(albumInfos[i]));
            MEDIA_WARN_LOG("Album (%{public}d, %{public}d, %{public}d, %{public}s) already exists.",
                albumInfos[i].albumIdOld, static_cast<int32_t>(albumInfos[i].albumType),
                static_cast<int32_t>(albumInfos[i].albumSubType), albumInfos[i].albumName.c_str());
            continue;
        } else {
            bool isUinque = CheckAlbumNameUnique(albumInfos[i].albumName, repetedAlbumName);
            if (!isUinque && albumInfos[i].albumType == PhotoAlbumType::SOURCE) {
                UpdateSourceAlbumName(isUinque, albumInfos, repetedAlbumName, i);
            }
        }
        NativeRdb::ValuesBucket value = GetInsertValue(albumInfos[i], tableName);
        values.emplace_back(value);
    }
    return values;
}

bool CloneRestore::CheckAlbumNameUnique(std::string albumName, const std::vector<std::string> &repetedAlbumName)
{
    CHECK_AND_RETURN_RET(std::count(repetedAlbumName.begin(), repetedAlbumName.end(),
        StringUtils::ToLower(albumName)) == 0, true);
    return !this->photoAlbumClone_.HasSameAlbumName(albumName);
}

bool CloneRestore::HasSameAlbum(AlbumInfo &albumInfo, const string &tableName)
{
    // check if the album already exists
    CHECK_AND_RETURN_RET(tableName != PhotoAlbumColumns::TABLE,
        this->photoAlbumClone_.HasSameAlbum(albumInfo.lPath, albumInfo.albumIdNew));
    string querySql = "SELECT " + MEDIA_COLUMN_COUNT_1 + " FROM " + tableName + " WHERE " +
        PhotoAlbumColumns::ALBUM_TYPE + " = " + to_string(albumInfo.albumType) + " AND " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + " = " + to_string(albumInfo.albumSubType) + " AND " +
        PhotoAlbumColumns::ALBUM_NAME + " = '" + albumInfo.albumName + "'";
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaLibraryRdb_, querySql);
    bool cond = (resultSet == nullptr);
    CHECK_AND_RETURN_RET(!cond, false);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return false;
    }
    int32_t count = GetInt32Val(MEDIA_COLUMN_COUNT_1, resultSet);
    resultSet->Close();
    return count > 0;
}

void CloneRestore::BatchQueryAlbum(vector<AlbumInfo> &albumInfos, const string &tableName)
{
    auto &albumIdMap = tableAlbumIdMap_[tableName];
    for (auto &albumInfo : albumInfos) {
        CHECK_AND_CONTINUE(albumInfo.albumIdOld > 0);
        string querySql = "SELECT " + PhotoAlbumColumns::ALBUM_ID + " FROM " + tableName + " WHERE " +
            PhotoAlbumColumns::ALBUM_TYPE + " = " + to_string(albumInfo.albumType) + " AND " +
            PhotoAlbumColumns::ALBUM_SUBTYPE + " = " + to_string(albumInfo.albumSubType) + " AND " +
            PhotoAlbumColumns::ALBUM_NAME + " = '" + albumInfo.albumName + "'";
        auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaLibraryRdb_, querySql);
        CHECK_AND_CONTINUE(resultSet != nullptr);
        if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
            resultSet->Close();
            continue;
        }
        albumInfo.albumIdNew = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
        CHECK_AND_CONTINUE(albumInfo.albumIdNew > 0);
        albumIdMap[albumInfo.albumIdOld] = albumInfo.albumIdNew;
        resultSet->Close();
    }
}

void CloneRestore::BatchInsertMap(const vector<FileInfo> &fileInfos, int64_t &totalRowNum)
{
    string selection;
    unordered_map<int32_t, int32_t> fileIdMap;
    SetFileIdReference(fileInfos, selection, fileIdMap);
    std::string tableName = ANALYSIS_ALBUM_TABLE;
    string garbledTableName = BackupDatabaseUtils::GarbleInfoName(tableName);
    string mapTableName = GetValueFromMap(CLONE_ALBUM_MAP, tableName);
    CHECK_AND_RETURN_LOG(!mapTableName.empty(),
        "Get map of table %{public}s failed", garbledTableName.c_str());
    auto albumIdMap = GetValueFromMap(tableAlbumIdMap_, tableName);
    CHECK_AND_RETURN_LOG(!albumIdMap.empty(),
        "Get album id map of table %{public}s empty, skip", garbledTableName.c_str());
    string albumSelection = GetValueFromMap(tableQueryWhereClauseMap_, tableName);
    unordered_set<int32_t> currentTableAlbumSet;
    string baseQuerySql = mapTableName + " INNER JOIN " + tableName + " ON " +
        mapTableName + "." + PhotoMap::ALBUM_ID + " = " + tableName + "." + PhotoAlbumColumns::ALBUM_ID +
        " WHERE " + mapTableName + "." + PhotoMap::ASSET_ID + " IN (" + selection + ")";
    baseQuerySql += albumSelection.empty() ? "" : " AND " + albumSelection;
    int32_t totalNumber = QueryMapTotalNumber(baseQuerySql);
    MEDIA_INFO_LOG("QueryMapTotalNumber of table %{public}s, totalNumber = %{public}d", garbledTableName.c_str(),
        totalNumber);
    for (int32_t offset = 0; offset < totalNumber; offset += QUERY_COUNT) {
        int64_t startQuery = MediaFileUtils::UTCTimeMilliSeconds();
        vector<MapInfo> mapInfos = QueryMapInfos(mapTableName, baseQuerySql, offset, fileIdMap, albumIdMap);
        int64_t startInsert = MediaFileUtils::UTCTimeMilliSeconds();
        int64_t rowNum = InsertMapByTable(mapTableName, mapInfos, currentTableAlbumSet);
        totalRowNum += rowNum;
        int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
        MEDIA_INFO_LOG("query %{public}zu map infos cost %{public}ld, insert %{public}ld maps cost %{public}ld",
            mapInfos.size(), (long)(startInsert - startQuery), (long)rowNum, (long)(end - startInsert));
    }
}

NativeRdb::ValuesBucket CloneRestore::GetInsertValue(const MapInfo &mapInfo) const
{
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoMap::ASSET_ID, mapInfo.fileId);
    values.PutInt(PhotoMap::ALBUM_ID, mapInfo.albumId);
    return values;
}

void CloneRestore::CheckTableColumnStatus(shared_ptr<NativeRdb::RdbStore> rdbStore,
    const vector<vector<string>> &cloneTableList)
{
    unordered_map<string, unordered_map<string, string>> tableColumnInfoMap;
    for (const auto &tableList : cloneTableList) {
        bool columnStatusGlobal = true;
        for (const auto &tableName : tableList) {
            auto &columnInfoMap = tableColumnInfoMap[tableName];
            columnInfoMap = BackupDatabaseUtils::GetColumnInfoMap(rdbStore, tableName);
            auto neededColumns = GetValueFromMap(NEEDED_COLUMNS_MAP, tableName);
            columnStatusGlobal = columnStatusGlobal && HasColumns(columnInfoMap, neededColumns);
        }
        for (const auto &tableName : tableList) {
            tableColumnStatusMap_[tableName] = columnStatusGlobal;
        }
    }
    for (const auto &tableList : cloneTableList) {
        for (const auto &tableName : tableList) {
            if (!IsReadyForRestore(tableName)) {
                MEDIA_ERR_LOG("Column status is false");
                break;
            }
            auto columnInfoMap = GetValueFromMap(tableColumnInfoMap, tableName);
            GetQueryWhereClause(tableName, columnInfoMap);
        }
    }
}

bool CloneRestore::HasColumns(const unordered_map<string, string> &columnInfoMap,
    const unordered_set<string> &columnSet)
{
    for (const auto &columnName : columnSet) {
        if (!HasColumn(columnInfoMap, columnName)) {
            MEDIA_ERR_LOG("Lack of column %{public}s", columnName.c_str());
            return false;
        }
    }
    return true;
}

bool CloneRestore::HasColumn(const unordered_map<string, string> &columnInfoMap, const string &columnName)
{
    return columnInfoMap.count(columnName) > 0;
}

bool CloneRestore::IsReadyForRestore(const string &tableName)
{
    return GetValueFromMap(tableColumnStatusMap_, tableName, false);
}

void CloneRestore::PrepareEditTimeVal(NativeRdb::ValuesBucket &values, int64_t editTime, const FileInfo &fileInfo,
    const unordered_map<string, string> &commonColumnInfoMap) const
{
    string editDataPath = backupRestoreDir_ +
        BackupFileUtils::GetFullPathByPrefixType(PrefixType::LOCAL_EDIT_DATA, fileInfo.relativePath);
    int64_t newEditTime = editTime > 0 && IsFilePathExist(editDataPath) ? editTime : 0;
    PrepareCommonColumnVal(values, PhotoColumn::PHOTO_EDIT_TIME, newEditTime, commonColumnInfoMap);
}

void CloneRestore::RestoreGallery()
{
    CheckTableColumnStatus(mediaRdb_, CLONE_TABLE_LISTS_PHOTO);
    // Upgrade original MediaLibrary Database
    DataTransfer::MediaLibraryDbUpgrade medialibraryDbUpgrade;
    medialibraryDbUpgrade.OnUpgrade(*this->mediaRdb_);
    // Report the old db info.
    DatabaseReport()
        .SetSceneCode(this->sceneCode_)
        .SetTaskId(this->taskId_)
        .ReportMedia(this->mediaRdb_, DatabaseReport::PERIOD_OLD)
        .ReportMedia(this->mediaLibraryRdb_, DatabaseReport::PERIOD_BEFORE);
    UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_)
        .Report("RESTORE_CLOUD_STATUS", "",
            "isAccountValid_: " + std::to_string(isAccountValid_) +
            "isSrcDstSwitchStatusMatch_: " + std::to_string(isSrcDstSwitchStatusMatch_));
    // Restore the backup db info.
    RestoreAlbum();
    RestorePhoto();
    if (IsCloudRestoreSatisfied()) {
        MEDIA_INFO_LOG("singlCloud cloud clone");
        RestorePhotoForCloud();
    }
    MEDIA_INFO_LOG("singlCloud migrate database photo number: %{public}lld, file number: %{public}lld (%{public}lld + "
        "%{public}lld), duplicate number: %{public}lld + %{public}lld, album number: %{public}lld, map number: "
        "%{public}lld", (long long)migrateDatabaseNumber_, (long long)migrateFileNumber_,
        (long long)(migrateFileNumber_ - migrateVideoFileNumber_), (long long)migrateVideoFileNumber_,
        (long long)migratePhotoDuplicateNumber_, (long long)migrateVideoDuplicateNumber_,
        (long long)migrateDatabaseAlbumNumber_, (long long)migrateDatabaseMapNumber_);
    MEDIA_INFO_LOG("singlCloud Start update group tags");
    RestoreAnalysisClassify();
    RestoreAnalysisGeo();
    RestoreAnalysisPortrait();
    RestoreAnalysisPet();
    RestoreGroupPhoto();
    cloneRestoreGeoDictionary_.ReportGeoRestoreTask();
    RestoreAnalysisData();
    InheritManualCover();
}

void CloneRestore::RestoreAnalysisTablesData()
{
    cloneRestoreAnalysisData_.Init(this->sceneCode_, this->taskId_, mediaRdb_, mediaLibraryRdb_);
    std::unordered_set<std::string> excludedColumns = {"id", "file_id"};
    vector<std::string> analysisTables = {
        "tab_analysis_head",
        "tab_analysis_pose",
        "tab_analysis_composition",
        "tab_analysis_ocr",
        "tab_analysis_segmentation",
        "tab_analysis_object",
        "tab_analysis_saliency_detect",
        "tab_analysis_recommendation"
    };

    vector<std::string> totalTypes = {
        "head",
        "pose",
        "composition",
        "ocr",
        "segmentation",
        "object",
        "saliency",
        "recommendation"
    };

    for (size_t index = 0; index < analysisTables.size(); index++) {
        std::string table = analysisTables[index];
        std::string type = totalTypes[index];
        cloneRestoreAnalysisData_.CloneAnalysisData(table, type, photoInfoMap_, excludedColumns);
    }
}

void CloneRestore::RestoreAnalysisData()
{
    RestoreAssetMapData();
    RestoreSearchIndexData();
    RestoreBeautyScoreData();
    RestoreVideoFaceData();
    RestoreAnalysisTablesData();
    RestoreHighlightAlbums();
    PopulateAnalysisAlbumIdMap();
    PopulateSystemAlbumIdMap();
    RestoreTabOldAlbumsData();
}

void CloneRestore::RestoreAssetMapData()
{
    AssetMapClone assetMapClone(mediaRdb_, mediaLibraryRdb_, photoInfoMap_);
    assetMapClone.CloneAssetMapInfo();
}

void CloneRestore::RestoreSearchIndexData()
{
    SearchIndexClone searchIndexClone(mediaRdb_, mediaLibraryRdb_, photoInfoMap_, maxSearchId_);
    searchIndexClone.Clone();
}

void CloneRestore::RestoreBeautyScoreData()
{
    BeautyScoreClone beautyScoreClone(mediaRdb_, mediaLibraryRdb_, photoInfoMap_, maxBeautyFileId_);
    beautyScoreClone.CloneBeautyScoreInfo();
}

void CloneRestore::RestoreVideoFaceData()
{
    VideoFaceClone videoFaceClone(mediaRdb_, mediaLibraryRdb_, photoInfoMap_);
    videoFaceClone.CloneVideoFaceInfo();
}

void CloneRestore::RestoreTabOldAlbumsData()
{
    TabOldAlbumsClone tabOldAlbumsClone(mediaRdb_, mediaLibraryRdb_, tableAlbumIdMap_);
    tabOldAlbumsClone.CloneAlbums(CLONE_ALBUMS);
}

bool CloneRestore::PrepareCloudPath(const string &tableName, FileInfo &fileInfo)
{
    fileInfo.cloudPath = BackupFileUtils::GetFullPathByPrefixType(PrefixType::CLOUD, fileInfo.relativePath);
    CHECK_AND_RETURN_RET_LOG(!fileInfo.cloudPath.empty(), false, "Get cloudPath empty");
    if (IsSameFileForClone(tableName, fileInfo)) {
        // should not delete the file, if the FileInfo is came from PhotoMap.
        if (fileInfo.isRelatedToPhotoMap != 1) {
            MEDIA_INFO_LOG("File (%{public}s) already exists. delete it.",
                BackupFileUtils::GarbleFilePath(fileInfo.filePath, CLONE_RESTORE_ID, garbagePath_).c_str());
            (void)MediaFileUtils::DeleteFile(fileInfo.filePath);
        }
        UpdateDuplicateNumber(fileInfo.fileType);
        return false;
    }
    // If the device originally has dentry file in the cloud path, no need to generate new cloud path.
    if (fileInfo.isNew && (MediaFileUtils::IsFileExists(fileInfo.cloudPath) || fileInfo.isRelatedToPhotoMap == 1)) {
        int32_t uniqueId = GetUniqueId(fileInfo.fileType);
        int32_t errCode = BackupFileUtils::CreateAssetPathById(uniqueId, fileInfo.fileType,
            MediaFileUtils::GetExtensionFromPath(fileInfo.displayName), fileInfo.cloudPath);
        if (errCode != E_OK) {
            ErrorInfo errorInfo(RestoreError::CREATE_PATH_FAILED, 1, std::to_string(errCode),
                BackupLogUtils::FileInfoToString(sceneCode_, fileInfo));
            UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
            fileInfo.cloudPath.clear();
            return false;
        }
    }
    int32_t errCode = BackupFileUtils::PreparePath(
        BackupFileUtils::GetReplacedPathByPrefixType(PrefixType::CLOUD, PrefixType::LOCAL, fileInfo.cloudPath));
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("Prepare cloudPath failed, path: %{public}s, cloudPath: %{public}s",
            BackupFileUtils::GarbleFilePath(fileInfo.filePath, CLONE_RESTORE_ID, garbagePath_).c_str(),
            BackupFileUtils::GarbleFilePath(fileInfo.cloudPath, DEFAULT_RESTORE_ID, garbagePath_).c_str());
        ErrorInfo errorInfo(RestoreError::PREPARE_PATH_FAILED, 1, std::to_string(errCode),
            BackupLogUtils::FileInfoToString(sceneCode_, fileInfo));
        UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
        fileInfo.cloudPath.clear();
        return false;
    }
    return true;
}

void CloneRestore::RestoreMusic()
{
    CheckTableColumnStatus(mediaRdb_, CLONE_TABLE_LISTS_AUDIO);
    RestoreAudio();
    MEDIA_INFO_LOG("migrate database audio number: %{public}lld, file number: %{public}lld, duplicate number: "
        "%{public}lld", (long long)migrateAudioDatabaseNumber_, (long long)migrateAudioFileNumber_,
        (long long)migrateAudioDuplicateNumber_);
}

void CloneRestore::RestoreAudio(void)
{
    MEDIA_INFO_LOG("Start clone restore: audio");
    CHECK_AND_RETURN_LOG(IsReadyForRestore(AudioColumn::AUDIOS_TABLE),
        "Column status is not ready for restore audio, quit");
    unordered_map<string, string> srcColumnInfoMap = BackupDatabaseUtils::GetColumnInfoMap(mediaRdb_,
        AudioColumn::AUDIOS_TABLE);
    unordered_map<string, string> dstColumnInfoMap = BackupDatabaseUtils::GetColumnInfoMap(mediaLibraryRdb_,
        AudioColumn::AUDIOS_TABLE);
    CHECK_AND_RETURN_LOG(PrepareCommonColumnInfoMap(AudioColumn::AUDIOS_TABLE, srcColumnInfoMap,
        dstColumnInfoMap), "Prepare common column info failed");
    int32_t totalNumber = QueryTotalNumber(AudioColumn::AUDIOS_TABLE);
    MEDIA_INFO_LOG("QueryAudioTotalNumber, totalNumber = %{public}d", totalNumber);
    CHECK_AND_RETURN(totalNumber > 0);

    audioTotalNumber_ += static_cast<uint64_t>(totalNumber);
    MEDIA_INFO_LOG("onProcess Update audioTotalNumber_: %{public}lld", (long long)audioTotalNumber_);
    if (!MediaFileUtils::IsFileExists(RESTORE_MUSIC_LOCAL_DIR)) {
        MEDIA_INFO_LOG("music dir is not exists!!!");
        MediaFileUtils::CreateDirectory(RESTORE_MUSIC_LOCAL_DIR);
    }
    for (int32_t offset = 0; offset < totalNumber; offset += CLONE_QUERY_COUNT) {
        ffrt::submit([this, offset]() { RestoreAudioBatch(offset); }, { &offset }, {},
            ffrt::task_attr().qos(static_cast<int32_t>(ffrt::qos_utility)));
    }
    ffrt::wait();
}

vector<FileInfo> CloneRestore::QueryFileInfos(const string &tableName, int32_t offset)
{
    vector<FileInfo> result;
    result.reserve(CLONE_QUERY_COUNT);
    string querySql = "SELECT * FROM " + tableName;
    string whereClause = GetQueryWhereClauseByTable(tableName);
    querySql += whereClause.empty() ? "" : " WHERE " + whereClause;
    querySql += " LIMIT " + to_string(offset) + ", " + to_string(CLONE_QUERY_COUNT);
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSql is null.");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        FileInfo fileInfo;
        if (ParseResultSet(tableName, resultSet, fileInfo)) {
            result.emplace_back(fileInfo);
        }
    }
    resultSet->Close();
    return result;
}

static void PrintCinematicCloneSize(const FileInfo &fileInfo)
{
    if (fileInfo.subtype == static_cast<int32_t>(PhotoSubType::CINEMATIC_VIDEO)) {
        MEDIA_INFO_LOG("Clone cinematic video, displayName: %{public}s, videoSize: %{public}lld",
            fileInfo.displayName.c_str(), fileInfo.fileSize);
    }
}

bool CloneRestore::ParseResultSet(const string &tableName, const shared_ptr<NativeRdb::ResultSet> &resultSet,
    FileInfo &fileInfo)
{
    fileInfo.fileType = GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet);
    fileInfo.fileIdOld = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    fileInfo.displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    fileInfo.oldPath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    if (!ConvertPathToRealPath(fileInfo.oldPath, filePath_, fileInfo.filePath, fileInfo.relativePath)) {
        ErrorInfo errorInfo(RestoreError::PATH_INVALID, 1, "", BackupLogUtils::FileInfoToString(sceneCode_, fileInfo));
        UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
        return false;
    }
    fileInfo.fileSize = GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet);
    if (fileInfo.fileSize <= 0) {
        ErrorInfo errorInfo(RestoreError::SIZE_INVALID, 1, "Db size <= 0",
            BackupLogUtils::FileInfoToString(sceneCode_, fileInfo));
        UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
        return false;
    }

    fileInfo.dateAdded = GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet);
    fileInfo.dateModified = GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet);
    fileInfo.dateTaken = GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet);
    fileInfo.detailTime = GetStringVal(PhotoColumn::PHOTO_DETAIL_TIME, resultSet);
    fileInfo.thumbnailReady = GetInt64Val(PhotoColumn::PHOTO_THUMBNAIL_READY, resultSet);
    fileInfo.lcdVisitTime = GetInt32Val(PhotoColumn::PHOTO_LCD_VISIT_TIME, resultSet);
    fileInfo.position = GetInt32Val(PhotoColumn::PHOTO_POSITION, resultSet);
    fileInfo.cloudVersion = GetInt32Val(PhotoColumn::PHOTO_CLOUD_VERSION, resultSet);
    fileInfo.effectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
    fileInfo.cloudId = GetStringVal(PhotoColumn::PHOTO_CLOUD_ID, resultSet);
    fileInfo.oldAstcDateKey = to_string(fileInfo.dateTaken);
    SetSpecialAttributes(tableName, resultSet, fileInfo);
    PrintCinematicCloneSize(fileInfo);

    auto commonColumnInfoMap = GetValueFromMap(tableCommonColumnInfoMap_, tableName);
    for (auto it = commonColumnInfoMap.begin(); it != commonColumnInfoMap.end(); ++it) {
        string columnName = it->first;
        string columnType = it->second;
        GetValFromResultSet(resultSet, fileInfo.valMap, columnName, columnType);
    }
    int32_t criticalTypeValue = CheckSrcDbHasCriticalTypeColumn() ?
                                GetInt32Val(PhotoColumn::PHOTO_CRITICAL_TYPE, resultSet) :
                                static_cast<int32_t>(CriticalType::UNKNOWN_CRITICAL_TYPE);
    fileInfo.criticalType = criticalTypeValue;
    return true;
}

bool CloneRestore::ParseResultSetForAudio(const shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &fileInfo)
{
    return ParseResultSet(AudioColumn::AUDIOS_TABLE, resultSet, fileInfo);
}

void CloneRestore::InsertAudio(vector<FileInfo> &fileInfos)
{
    CHECK_AND_RETURN_LOG(mediaLibraryRdb_ != nullptr, "mediaLibraryRdb_ is null");
    CHECK_AND_RETURN_LOG(!fileInfos.empty(), "fileInfos are empty");
    int64_t startMove = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t fileMoveCount = 0;
    for (auto& fileInfo : fileInfos) {
        CHECK_AND_CONTINUE_ERR_LOG(BackupFileUtils::IsFileValid(fileInfo.filePath, CLONE_RESTORE_ID) == E_OK,
            "File is invalid: size: %{public}lld, name: %{public}s, filePath: %{public}s",
            (long long)fileInfo.fileSize, BackupFileUtils::GarbleFileName(fileInfo.displayName).c_str(),
            BackupFileUtils::GarbleFilePath(fileInfo.filePath, CLONE_RESTORE_ID, garbagePath_).c_str());
        CHECK_AND_CONTINUE(PrepareCloudPath(AudioColumn::AUDIOS_TABLE, fileInfo));
        string localPath = RESTORE_MUSIC_LOCAL_DIR + fileInfo.displayName;
        if (MediaFileUtils::IsFileExists(localPath)) {
            MEDIA_INFO_LOG("localPath %{public}s already exists.",
                BackupFileUtils::GarbleFilePath(fileInfo.filePath, CLONE_RESTORE_ID, garbagePath_).c_str());
            UpdateDuplicateNumber(fileInfo.fileType);
            continue;
        }

        int32_t moveErrCode = MoveFile(fileInfo.filePath, localPath);
        if (moveErrCode != E_OK) {
            MEDIA_ERR_LOG("MoveFile failed, filePath: %{public}s, errCode: %{public}d, errno: %{public}d",
                BackupFileUtils::GarbleFilePath(fileInfo.filePath, CLONE_RESTORE_ID, garbagePath_).c_str(), moveErrCode,
                errno);
            UpdateFailedFiles(fileInfo.fileType, fileInfo, RestoreError::MOVE_FAILED);
            continue;
        }
        MediaFileUtils::UpdateModifyTimeInMsec(localPath, fileInfo.dateModified);
        fileMoveCount++;
    }
    migrateAudioFileNumber_ += fileMoveCount;
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("move %{public}ld files cost %{public}ld.", (long)fileMoveCount, (long)(end - startMove));
}

size_t CloneRestore::StatClonetotalSize(std::shared_ptr<NativeRdb::RdbStore> mediaRdb)
{
    CHECK_AND_RETURN_RET_LOG(mediaRdb != nullptr, 0, "rdbStore is nullptr");

    string thumbSizeSql {};
    thumbSizeSql = "SELECT SUM(CAST(" + PhotoExtColumn::THUMBNAIL_SIZE + " AS BIGINT)) AS " + MEDIA_DATA_DB_SIZE +
                          ", -1 AS " + MediaColumn::MEDIA_TYPE +
                          " FROM " + PhotoExtColumn::PHOTOS_EXT_TABLE;

    string mediaVolumeQuery = PhotoColumn::QUERY_MEDIA_VOLUME + " UNION ALL " +
                              AudioColumn::QUERY_MEDIA_VOLUME + " UNION ALL " +
                              thumbSizeSql;

    auto resultSet = mediaRdb->QuerySql(mediaVolumeQuery);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, 0, "Failed to execute media volume query");

    int64_t totalVolume = 0;
    MEDIA_INFO_LOG("Initial totalVolume: %{public}" PRId64, totalVolume);

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int64_t mediaSize = GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet);
        int32_t mediatype = GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet);
        MEDIA_INFO_LOG("mediatype is %{public}d, current media asset size is: %{public}" PRId64, mediatype, mediaSize);
        if (mediaSize < 0) {
            MEDIA_ERR_LOG("ill mediaSize: %{public}" PRId64 " for mediatype: %{public}d", mediaSize, mediatype);
        }

        totalVolume += mediaSize;
        MEDIA_INFO_LOG("current totalVolume: %{public}" PRId64, totalVolume);
    }
    resultSet->Close();
    MEDIA_INFO_LOG("media db media asset size is: %{public}" PRId64, totalVolume);

    if (totalVolume < 0) {
        MEDIA_ERR_LOG("totalVolume is negative: %{public}" PRId64 ". Returning 0.", totalVolume);
        return 0;
    }

    size_t totalAssetSize = static_cast<size_t>(totalVolume);
    // other meta data dir size
    size_t editDataTotalSize {0};
    size_t rdbTotalSize {0};
    size_t kvdbTotalSize {0};
    size_t highlightTotalSize {0};
    MediaFileUtils::StatDirSize(CLONE_STAT_EDIT_DATA_DIR, editDataTotalSize);
    MediaFileUtils::StatDirSize(CLONE_STAT_RDB_DIR, rdbTotalSize);
    MediaFileUtils::StatDirSize(CLONE_STAT_KVDB_DIR, kvdbTotalSize);
    MediaFileUtils::StatDirSize(CLONE_STAT_HIGHLIGHT_DIR, highlightTotalSize);
    size_t totalSize = totalAssetSize + editDataTotalSize + rdbTotalSize + kvdbTotalSize + highlightTotalSize;
    return totalSize;
}

string CloneRestore::GetBackupInfo()
{
    CHECK_AND_RETURN_RET_LOG(BaseRestore::Init() == E_OK, "", "GetBackupInfo init failed");
    CHECK_AND_RETURN_RET_LOG(mediaLibraryRdb_ != nullptr, "", "GetBackupInfo Rdbstore is null");

    CheckTableColumnStatus(mediaLibraryRdb_, CLONE_TABLE_LISTS_OLD_DEVICE);
    int32_t photoCount = QueryTotalNumberByMediaType(mediaLibraryRdb_, PhotoColumn::PHOTOS_TABLE,
        MediaType::MEDIA_TYPE_IMAGE);
    int32_t videoCount = QueryTotalNumberByMediaType(mediaLibraryRdb_, PhotoColumn::PHOTOS_TABLE,
        MediaType::MEDIA_TYPE_VIDEO);
    int32_t audioCount = QueryTotalNumberByMediaType(mediaLibraryRdb_, AudioColumn::AUDIOS_TABLE,
        MediaType::MEDIA_TYPE_AUDIO);

    size_t totalSize = StatClonetotalSize(mediaLibraryRdb_);
    MEDIA_INFO_LOG("QueryTotalNumber, photo: %{public}d, video: %{public}d, audio: %{public}d, totalSize: "
        "%{public}lld bytes", photoCount, videoCount, audioCount, static_cast<long long>(totalSize));

    return GetBackupInfoByCount(photoCount, videoCount, audioCount, totalSize);
}

int32_t CloneRestore::QueryTotalNumberByMediaType(shared_ptr<NativeRdb::RdbStore> rdbStore, const string &tableName,
    MediaType mediaType)
{
    string querySql = "SELECT " + MEDIA_COLUMN_COUNT_1 + " FROM " + tableName + " WHERE " + MediaColumn::MEDIA_TYPE +
        " = " + to_string(static_cast<int32_t>(mediaType));
    string whereClause = GetQueryWhereClauseByTable(tableName);
    querySql += whereClause.empty() ? "" : " AND " + whereClause;
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(rdbStore, querySql);
    bool cond = (resultSet == nullptr);
    CHECK_AND_RETURN_RET(!cond, 0);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return 0;
    }
    int32_t result = GetInt32Val(MEDIA_COLUMN_COUNT_1, resultSet);
    resultSet->Close();
    return result;
}

string CloneRestore::GetBackupInfoByCount(int32_t photoCount, int32_t videoCount, int32_t audioCount, size_t totalSize)
{
    nlohmann::json jsonObject = {
        {
            { STAT_KEY_BACKUP_INFO, STAT_TYPE_PHOTO },
            { STAT_KEY_NUMBER, photoCount }
        },
        {
            { STAT_KEY_BACKUP_INFO, STAT_TYPE_VIDEO },
            { STAT_KEY_NUMBER, videoCount }
        },
        {
            { STAT_KEY_BACKUP_INFO, STAT_TYPE_AUDIO },
            { STAT_KEY_NUMBER, audioCount }
        },
        {
            { STAT_KEY_BACKUP_INFO, STAT_TYPE_TOTAL_SIZE },
            { STAT_KEY_NUMBER, totalSize }
        }
    };
    return jsonObject.dump();
}

void CloneRestore::RestorePhotoBatch(int32_t offset, int32_t isRelatedToPhotoMap)
{
    MEDIA_INFO_LOG(
        "start restore photo, offset: %{public}d, isRelatedToPhotoMap: %{public}d", offset, isRelatedToPhotoMap);
    vector<FileInfo> fileInfos = QueryFileInfos(offset, isRelatedToPhotoMap);
    CHECK_AND_EXECUTE(InsertPhoto(fileInfos) == E_OK, AddToPhotosFailedOffsets(offset));
    MEDIA_INFO_LOG("end restore photo, offset: %{public}d", offset);
}

void CloneRestore::RestoreBatchForCloud(int32_t offset, int32_t isRelatedToPhotoMap)
{
    MEDIA_INFO_LOG(
        "singlCloud restore photo, offset: %{public}d, isRelated: %{public}d", offset, isRelatedToPhotoMap);
    vector<FileInfo> fileInfos = QueryCloudFileInfos(offset, isRelatedToPhotoMap);
    CHECK_AND_EXECUTE(InsertCloudPhoto(sceneCode_, fileInfos, SourceType::PHOTOS) == E_OK,
        AddToPhotosFailedOffsets(offset));

    MEDIA_INFO_LOG("singleCloud end restore photo, offset: %{public}d", offset);
}

int CloneRestore::InsertCloudPhoto(int32_t sceneCode, std::vector<FileInfo> &fileInfos, int32_t sourceType)
{
    MEDIA_INFO_LOG("singleCloud start insert cloud %{public}zu photos", fileInfos.size());
    CHECK_AND_RETURN_RET_LOG(mediaLibraryRdb_ != nullptr, E_OK,
        "singleCloud mediaLibraryRdb_ iS null in cloud clone");
    CHECK_AND_RETURN_RET_LOG(!fileInfos.empty(), E_OK, "singleCloud fileInfos are empty in cloud clone");
    int64_t startGenerate = MediaFileUtils::UTCTimeMilliSeconds();
    vector<NativeRdb::ValuesBucket> values = GetCloudInsertValues(sceneCode, fileInfos, sourceType);
    int64_t startInsert = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t rowNum = 0;
    int32_t errCode = BatchInsertWithRetry(PhotoColumn::PHOTOS_TABLE, values, rowNum);
    MEDIA_INFO_LOG("singleCloud insertCloudPhoto is %{public}d, the rowNum is %{public}" PRId64, errCode, rowNum);
    migrateCloudSuccessNumber_ += rowNum;
    if (errCode != E_OK) {
        if (needReportFailed_) {
            UpdateFailedFiles(fileInfos, RestoreError::INSERT_FAILED);
            ErrorInfo errorInfo(RestoreError::INSERT_FAILED, static_cast<int32_t>(fileInfos.size()), errCode);
            UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
        }
        return errCode;
    }

    int64_t startInsertRelated = MediaFileUtils::UTCTimeMilliSeconds();
    InsertPhotoRelated(fileInfos, SourceType::PHOTOS);

    // create dentry file for cloud origin, save failed cloud id
    std::vector<std::string> dentryFailedOrigin;
    CHECK_AND_EXECUTE(BatchCreateDentryFile(fileInfos, dentryFailedOrigin, DENTRY_INFO_ORIGIN) != E_OK,
        HandleFailData(fileInfos, dentryFailedOrigin, DENTRY_INFO_ORIGIN));

    int64_t startMove = MediaFileUtils::UTCTimeMilliSeconds();
    migrateDatabaseNumber_ += rowNum;
    int32_t fileMoveCount = 0;
    int32_t videoFileMoveCount = 0;
    MoveMigrateCloudFile(fileInfos, fileMoveCount, videoFileMoveCount, sceneCode);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("singleCloud generate values cost %{public}ld, insert %{public}ld assets cost %{public}ld, insert"
        " photo related cost %{public}ld, and move %{public}ld files (%{public}ld + %{public}ld) cost %{public}ld.",
        (long)(startInsert - startGenerate), (long)rowNum, (long)(startInsertRelated - startInsert),
        (long)(startMove - startInsertRelated), (long)fileMoveCount, (long)(fileMoveCount - videoFileMoveCount),
        (long)videoFileMoveCount, (long)(end - startMove));
    MEDIA_INFO_LOG("singleCLoud  insert cloud end");
    return E_OK;
}

void CloneRestore::RestoreAudioBatch(int32_t offset)
{
    MEDIA_INFO_LOG("start restore audio, offset: %{public}d", offset);
    vector<FileInfo> fileInfos = QueryFileInfos(AudioColumn::AUDIOS_TABLE, offset);
    InsertAudio(fileInfos);
    MEDIA_INFO_LOG("end restore audio, offset: %{public}d", offset);
}

void CloneRestore::AddToPhotoInfoMaps(std::vector<FileInfo> &fileInfos)
{
    std::lock_guard<ffrt::mutex> lock(photosInfoMutex_);
    for (auto fileInfo: fileInfos) {
        PhotoInfo photoInfo;
        photoInfo.fileIdNew = fileInfo.fileIdNew;
        photoInfo.fileType = fileInfo.fileType;
        photoInfo.displayName = fileInfo.displayName;
        photoInfo.cloudPath = fileInfo.cloudPath;
        photoInfoMap_.insert(std::make_pair(fileInfo.fileIdOld, photoInfo));
    }
}

void CloneRestore::InsertPhotoRelated(vector<FileInfo> &fileInfos, int32_t sourceType)
{
    int64_t startQuery = MediaFileUtils::UTCTimeMilliSeconds();
    BatchQueryPhoto(fileInfos);
    AddToPhotoInfoMaps(fileInfos);
    int64_t startInsert = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t mapRowNum = 0;
    BatchInsertMap(fileInfos, mapRowNum);
    migrateDatabaseMapNumber_ += mapRowNum;
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("query new file_id cost %{public}ld, insert %{public}ld maps cost %{public}ld",
        (long)(startInsert - startQuery), (long)mapRowNum, (long)(end - startInsert));
}

void CloneRestore::SetFileIdReference(const vector<FileInfo> &fileInfos, string &selection,
    unordered_map<int32_t, int32_t> &fileIdMap)
{
    for (const auto &fileInfo : fileInfos) {
        if (fileInfo.fileIdOld <= 0 || fileInfo.fileIdNew <= 0) {
            continue;
        }
        BackupDatabaseUtils::UpdateSelection(selection, to_string(fileInfo.fileIdOld), false);
        fileIdMap[fileInfo.fileIdOld] = fileInfo.fileIdNew;
    }
}

int32_t CloneRestore::QueryMapTotalNumber(const string &baseQuerySql)
{
    string querySql = "SELECT count(1) as count FROM " + baseQuerySql;
    return BackupDatabaseUtils::QueryInt(mediaRdb_, querySql, CUSTOM_COUNT);
}

vector<MapInfo> CloneRestore::QueryMapInfos(const string &tableName, const string &baseQuerySql, int32_t offset,
    const unordered_map<int32_t, int32_t> &fileIdMap, const unordered_map<int32_t, int32_t> &albumIdMap)
{
    vector<MapInfo> mapInfos;
    mapInfos.reserve(CLONE_QUERY_COUNT);
    string columnMapAlbum = tableName + "." + PhotoMap::ALBUM_ID;
    string columnMapAsset = tableName + "." + PhotoMap::ASSET_ID;
    string querySql = "SELECT " + columnMapAlbum + ", " + columnMapAsset + " FROM " + baseQuerySql;
    querySql += " LIMIT " + to_string(offset) + ", " + to_string(CLONE_QUERY_COUNT);
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, mapInfos, "Query resultSql is null.");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t albumIdOld = GetInt32Val(columnMapAlbum, resultSet);
        int32_t fileIdOld = GetInt32Val(columnMapAsset, resultSet);
        if (albumIdOld <= 0 || albumIdMap.count(albumIdOld) == 0 || fileIdOld <= 0 || fileIdMap.count(fileIdOld) <= 0) {
            continue;
        }
        MapInfo mapInfo;
        mapInfo.albumId = albumIdMap.at(albumIdOld);
        mapInfo.fileId = fileIdMap.at(fileIdOld);
        mapInfos.emplace_back(mapInfo);
    }
    resultSet->Close();
    return mapInfos;
}

int64_t CloneRestore::InsertMapByTable(const string &tableName, const vector<MapInfo> &mapInfos,
    unordered_set<int32_t> &albumSet)
{
    vector<NativeRdb::ValuesBucket> values = GetInsertValues(mapInfos);
    int64_t rowNum = 0;
    int32_t errCode = BatchInsertWithRetry(tableName, values, rowNum);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, 0,
        "Batch insert map failed, errCode: %{public}d", errCode);
    for (const auto &mapInfo : mapInfos) {
        albumSet.insert(mapInfo.albumId);
    }
    return rowNum;
}

vector<NativeRdb::ValuesBucket> CloneRestore::GetInsertValues(const vector<MapInfo> &mapInfos)
{
    vector<NativeRdb::ValuesBucket> values;
    for (const auto &mapInfo : mapInfos) {
        NativeRdb::ValuesBucket value = GetInsertValue(mapInfo);
        values.emplace_back(value);
    }
    return values;
}

string CloneRestore::GetQueryWhereClauseByTable(const string &tableName)
{
    string whereClause;
    if (tableQueryWhereClauseMap_.count(tableName)) {
        whereClause += tableQueryWhereClauseMap_.at(tableName);
    }
    if (tableExtraQueryWhereClauseMap_.count(tableName)) {
        whereClause += whereClause.empty() ? "" : " AND " + tableExtraQueryWhereClauseMap_.at(tableName);
    }
    return whereClause;
}

void CloneRestore::SetSpecialAttributes(const string &tableName, const shared_ptr<NativeRdb::ResultSet> &resultSet,
    FileInfo &fileInfo)
{
    if (tableName != PhotoColumn::PHOTOS_TABLE) {
        return;
    }
    fileInfo.lPath = GetStringVal(PhotoAlbumColumns::ALBUM_LPATH, resultSet);
    fileInfo.sourcePath = GetStringVal(PhotoColumn::PHOTO_SOURCE_PATH, resultSet);
    fileInfo.subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    fileInfo.associateFileId = GetInt32Val(PhotoColumn::PHOTO_ASSOCIATE_FILE_ID, resultSet);
    fileInfo.photoQuality = GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet);
    fileInfo.recycledTime = GetInt64Val(MediaColumn::MEDIA_DATE_TRASHED, resultSet);
    fileInfo.hidden = GetInt32Val(MediaColumn::MEDIA_HIDDEN, resultSet);
    fileInfo.compositeDisplayStatus = GetInt32Val(PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS, resultSet);
    // find PhotoAlbum info in target database. PackageName and BundleName should be fixed after clone.
    fileInfo.lPath = this->photosClone_.FindlPath(fileInfo);
    fileInfo.ownerAlbumId = this->photosClone_.FindAlbumId(fileInfo);
    fileInfo.packageName = this->photosClone_.FindPackageName(fileInfo);
    fileInfo.bundleName = this->photosClone_.FindBundleName(fileInfo);
    fileInfo.photoQuality = this->photosClone_.FindPhotoQuality(fileInfo);
    fileInfo.sourcePath = this->photosClone_.FindSourcePath(fileInfo);
    fileInfo.latitude = GetDoubleVal("latitude", resultSet);
    fileInfo.longitude = GetDoubleVal("longitude", resultSet);
    GetOrientationAndExifRotateValue(resultSet, fileInfo);
}

bool CloneRestore::IsSameFileForClone(const string &tableName, FileInfo &fileInfo)
{
    CHECK_AND_RETURN_RET(tableName == PhotoColumn::PHOTOS_TABLE,
        IsSameAudioFile(mediaLibraryRdb_, tableName, fileInfo));
    PhotosDao::PhotosRowData rowData = this->photosClone_.FindSameFile(fileInfo);
    bool cond = rowData.IsValid();
    CHECK_AND_RETURN_RET(cond, false);
    // Meed extra check to determine whether or not to drop the duplicate file.
    return ExtraCheckForCloneSameFile(fileInfo, rowData);
}

NativeRdb::ValuesBucket CloneRestore::GetInsertValue(const AnalysisAlbumTbl &portraitAlbumInfo)
{
    NativeRdb::ValuesBucket values;

    PutIfPresent(values, ANALYSIS_COL_ALBUM_TYPE, portraitAlbumInfo.albumType);
    PutIfPresent(values, ANALYSIS_COL_ALBUM_SUBTYPE, portraitAlbumInfo.albumSubtype);
    PutIfPresent(values, ANALYSIS_COL_ALBUM_NAME, portraitAlbumInfo.albumName);
    PutIfPresent(values, ANALYSIS_COL_TAG_ID, portraitAlbumInfo.tagId);
    PutIfPresent(values, ANALYSIS_COL_USER_OPERATION, portraitAlbumInfo.userOperation);
    PutIfPresent(values, ANALYSIS_COL_GROUP_TAG, portraitAlbumInfo.groupTag);
    PutIfPresent(values, ANALYSIS_COL_USER_DISPLAY_LEVEL, portraitAlbumInfo.userDisplayLevel);
    PutIfPresent(values, ANALYSIS_COL_IS_ME, portraitAlbumInfo.isMe);
    PutIfPresent(values, ANALYSIS_COL_IS_REMOVED, portraitAlbumInfo.isRemoved);
    PutIfPresent(values, ANALYSIS_COL_RENAME_OPERATION, portraitAlbumInfo.renameOperation);
    PutIfPresent(values, ANALYSIS_COL_IS_LOCAL, portraitAlbumInfo.isLocal);
    PutIfPresent(values, ANALYSIS_COL_RELATIONSHIP, portraitAlbumInfo.relationship);

    return values;
}

bool CloneRestore::InitAllKvStore()
{
    std::string oldBaseDir = backupRestoreDir_ + CLONE_KVDB_BACKUP_DIR;
    std::string newBaseDir = MEDIA_KVDB_DIR;
    oldMonthKvStorePtr_ = MediaLibraryKvStoreManager::GetInstance()
        .GetSingleKvStore(KvStoreRoleType::OWNER, CLONE_KVSTORE_MONTH_STOREID, oldBaseDir);
    oldYearKvStorePtr_ = MediaLibraryKvStoreManager::GetInstance()
        .GetSingleKvStore(KvStoreRoleType::OWNER, CLONE_KVSTORE_YEAR_STOREID, oldBaseDir);
    newMonthKvStorePtr_ = MediaLibraryKvStoreManager::GetInstance()
        .GetSingleKvStore(KvStoreRoleType::OWNER, MEDIA_KVSTORE_MONTH_STOREID, newBaseDir);
    newYearKvStorePtr_ = MediaLibraryKvStoreManager::GetInstance()
        .GetSingleKvStore(KvStoreRoleType::OWNER, MEDIA_KVSTORE_YEAR_STOREID, newBaseDir);

    bool cond = (oldMonthKvStorePtr_ == nullptr || oldYearKvStorePtr_ == nullptr ||
        newMonthKvStorePtr_ == nullptr || newYearKvStorePtr_ == nullptr);
    CHECK_AND_RETURN_RET_LOG(!cond, false, "Init all kvstore failed");
    return true;
}

void CloneRestore::CloseAllKvStore()
{
    oldMonthKvStorePtr_ != nullptr && oldMonthKvStorePtr_->Close();
    oldYearKvStorePtr_ != nullptr && oldYearKvStorePtr_->Close();
    newMonthKvStorePtr_ != nullptr && newMonthKvStorePtr_->Close();
    newYearKvStorePtr_ != nullptr && newYearKvStorePtr_->Close();
}

void CloneRestore::StartBackup()
{
    MEDIA_INFO_LOG("enter clone backup");
    if (BaseRestore::Init() != E_OK || !mediaLibraryRdb_) {
        MEDIA_ERR_LOG("init db failed when start backup");
        SetErrorCode(RestoreError::INIT_FAILED);
        return;
    }
    if (!BackupDatabaseUtils::ClearConfigInfo(mediaLibraryRdb_)) {
        MEDIA_ERR_LOG("Clear ConfigInfo failed");
        SetErrorCode(RestoreError::BACKUP_CLEAR_CONFIGINFO_FAILED);
        return;
    }
    bool cond = (!BackupKvStore() && !MediaFileUtils::DeleteDir(CLONE_KVDB_BACKUP_DIR));
    CHECK_AND_PRINT_LOG(!cond, "BackupKvStore failed and delete old backup kvdb failed, errno:%{public}d", errno);
    if (WaitSouthDeviceExitTimeout()) {
        MEDIA_ERR_LOG("backup, wait south device cleaning data timeout.");
        SetErrorCode(RestoreError::RETAIN_FORCE_TIMEOUT);
        ErrorInfo errorInfo(RestoreError::RETAIN_FORCE_TIMEOUT, 0, "",
            "backup, wait south device cleaning data timeout.");
        UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
        return;
    }
    MEDIA_INFO_LOG("Start clone backup");
    SetParameterForBackup();
    srcCloneRestoreConfigInfo_ = GetCurrentDeviceCloneConfigInfo();
    CHECK_AND_RETURN_LOG(BackupPreprocess(), "backup preprocess failed");
    if (!UpdateConfigInfo()) {
        MEDIA_ERR_LOG("update configInfo failed when start backup");
        SetErrorCode(RestoreError::BACKUP_UPDATE_CONFIG_INFO_FAILED);
        return;
    }
    MEDIA_INFO_LOG("End clone backup");
}

void CloneRestore::InheritManualCover()
{
    std::string querySql = "SELECT album_id, cover_uri, cover_uri_source, cover_cloud_id"
        " FROM PhotoAlbum WHERE cover_uri_source > 0";
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, querySql);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Query resultSql is null.");

    vector<AlbumCoverInfo> albumCoverinfos;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        AlbumCoverInfo albumCoverInfo;
        int32_t albumIdOld = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
        string coverUriOld = GetStringVal(PhotoAlbumColumns::ALBUM_COVER_URI, resultSet);
        int32_t fileIdOld = atoi(MediaLibraryDataManagerUtils::GetFileIdFromPhotoUri(coverUriOld).c_str());
        int32_t coverUriSourceOld = GetInt32Val(PhotoAlbumColumns::COVER_URI_SOURCE, resultSet);
        string coverCloudIdOld = GetStringVal(PhotoAlbumColumns::COVER_CLOUD_ID, resultSet);

        int32_t albumIdNew = tableAlbumIdMap_[PhotoAlbumColumns::TABLE][albumIdOld];
        auto photoInfo = photoInfoMap_[fileIdOld];
        int32_t fileIdNew = photoInfo.fileIdNew;
        string coverUriNew = MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, to_string(fileIdNew),
            MediaFileUtils::GetExtraUri(photoInfo.displayName, photoInfo.cloudPath));
        albumCoverInfo.albumId = albumIdNew;
        albumCoverInfo.coverUri = coverUriNew;
        albumCoverInfo.coverUriSource = coverUriSourceOld;
        albumCoverInfo.coverCloudId = coverCloudIdOld;
        albumCoverinfos.emplace_back(albumCoverInfo);
    }
    resultSet->Close();

    UpdatePhotoAlbumCoverUri(albumCoverinfos);
}

bool CloneRestore::BackupKvStore()
{
    MEDIA_INFO_LOG("Start BackupKvstore");
    // Delete only redundant data and do not need to be returned.
    CHECK_AND_EXECUTE(!MediaFileUtils::IsFileExists(CLONE_KVDB_BACKUP_DIR),
        MediaFileUtils::DeleteDir(CLONE_KVDB_BACKUP_DIR));
    std::string backupKvdbPath = CLONE_KVDB_BACKUP_DIR + "/kvdb";
    CHECK_AND_RETURN_RET_LOG(BackupFileUtils::PreparePath(backupKvdbPath) == E_OK,
        false, "Prepare backup dir failed");

    int32_t status = MediaLibraryKvStoreManager::GetInstance().CloneKvStore(
        MEDIA_KVSTORE_MONTH_STOREID, MEDIA_KVDB_DIR, CLONE_KVSTORE_MONTH_STOREID, CLONE_KVDB_BACKUP_DIR);
    CHECK_AND_RETURN_RET(status == E_OK, false);
    status = MediaLibraryKvStoreManager::GetInstance().CloneKvStore(
        MEDIA_KVSTORE_YEAR_STOREID, MEDIA_KVDB_DIR, CLONE_KVSTORE_YEAR_STOREID, CLONE_KVDB_BACKUP_DIR);
    CHECK_AND_RETURN_RET(status == E_OK, false);
    MEDIA_INFO_LOG("End BackupKvstore");
    return true;
}

void CloneRestore::BatchUpdateFileInfoData(std::vector<FileInfo> &fileInfos,
    unordered_map<string, CloudPhotoFileExistFlag> &resultExistMap)
{
    for (size_t i = 0; i < fileInfos.size(); i++) {
        CHECK_AND_CONTINUE(fileInfos[i].needMove);
        CloudPhotoFileExistFlag fileExistFlag;
        unordered_map<string, CloudPhotoFileExistFlag>::iterator iter =
            resultExistMap.find(fileInfos[i].cloudPath);
        CHECK_AND_CONTINUE(iter != resultExistMap.end());
        fileExistFlag = iter->second;
        int32_t thumbReady = CheckThumbReady(fileInfos[i], fileExistFlag);
        int32_t thumbStatus = CheckThumbStatus(fileInfos[i], fileExistFlag);
        int32_t lcdVisitTime = CheckLcdVisitTime(fileExistFlag);

        std::unique_ptr<NativeRdb::AbsRdbPredicates> predicates =
            std::make_unique<NativeRdb::AbsRdbPredicates>(PhotoColumn::PHOTOS_TABLE);
        std::string whereClause = "data = '" + fileInfos[i].cloudPath + "'";
        predicates->SetWhereClause(whereClause);

        int32_t updatedRows = 0;
        NativeRdb::ValuesBucket updateBucket;
        updateBucket.PutInt(PhotoColumn::PHOTO_THUMB_STATUS, thumbStatus);
        updateBucket.PutLong(PhotoColumn::PHOTO_THUMBNAIL_READY,
            thumbReady == 0 ? 0 : fileInfos[i].thumbnailReady);
        updateBucket.PutInt(PhotoColumn::PHOTO_LCD_VISIT_TIME, lcdVisitTime);
        updateBucket.PutInt(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE,
            thumbReady == 0 ? 0 : RESTORE_THUMBNAIL_VISIBLE_TRUE);

        int32_t ret = BackupDatabaseUtils::Update(mediaLibraryRdb_, updatedRows, updateBucket, predicates);
        bool cond = (updatedRows < 0 || ret < 0);
        CHECK_AND_PRINT_LOG(!cond, "BatchInsertFileInfoData Failed to update column: %s",
            fileInfos[i].cloudPath.c_str());
        MediaLibraryPhotoOperations::StoreThumbnailSize(to_string(fileInfos[i].fileIdNew),
            fileInfos[i].cloudPath);
    }
}

int32_t CloneRestore::CheckThumbReady(const FileInfo &fileInfo,
    const CloudPhotoFileExistFlag &cloudPhotoFileExistFlag)
{
    bool cond = (cloudPhotoFileExistFlag.isThmExist &&
            cloudPhotoFileExistFlag.isDayAstcExist &&
            cloudPhotoFileExistFlag.isYearAstcExist);
    CHECK_AND_RETURN_RET(!cond, RESTORE_THUMBNAIL_READY_ALL_SUCCESS);
    return RESTORE_THUMBNAIL_READY_FAIL;
}

int32_t CloneRestore::CheckThumbStatus(const FileInfo &fileInfo,
    const CloudPhotoFileExistFlag &cloudPhotoFileExistFlag)
{
    if (!HasExThumbnail(fileInfo)) {
        if (cloudPhotoFileExistFlag.isThmExist &&
            cloudPhotoFileExistFlag.isLcdExist) {
                return RESTORE_THUMBNAIL_STATUS_ALL;
        } else if (cloudPhotoFileExistFlag.isThmExist &&
            !cloudPhotoFileExistFlag.isLcdExist) {
                return RESTORE_THUMBNAIL_STATUS_NOT_LCD;
        } else if (!cloudPhotoFileExistFlag.isThmExist &&
            cloudPhotoFileExistFlag.isLcdExist) {
                return RESTORE_THUMBNAIL_STATUS_NOT_THUMB;
        } else {
            return RESTORE_THUMBNAIL_STATUS_NOT_ALL;
        }
    }
    if (cloudPhotoFileExistFlag.isExThmExist &&
        cloudPhotoFileExistFlag.isExLcdExist) {
            return RESTORE_THUMBNAIL_STATUS_ALL;
    } else if (cloudPhotoFileExistFlag.isExThmExist &&
        !cloudPhotoFileExistFlag.isExLcdExist) {
            return RESTORE_THUMBNAIL_STATUS_NOT_LCD;
    } else if (!cloudPhotoFileExistFlag.isExThmExist &&
        cloudPhotoFileExistFlag.isExLcdExist) {
            return RESTORE_THUMBNAIL_STATUS_NOT_THUMB;
    }
    return RESTORE_THUMBNAIL_STATUS_NOT_ALL;
}

int32_t CloneRestore::CheckLcdVisitTime(const CloudPhotoFileExistFlag &cloudPhotoFileExistFlag)
{
    CHECK_AND_RETURN_RET(!cloudPhotoFileExistFlag.isLcdExist, RESTORE_LCD_VISIT_TIME_SUCCESS);
    return RESTORE_LCD_VISIT_TIME_NO_LCD;
}

void CloneRestore::SetRestoreFailedAndErrorCount(uint64_t &failed, uint64_t &error)
{
    uint64_t success = migrateFileNumber_ + migrateLakeFileNumber_;
    uint64_t duplicate = migratePhotoDuplicateNumber_ + migrateVideoDuplicateNumber_ +
        migrateLakePhotoDuplicateNumber_ + migrateLakeVideoDuplicateNumber_;
    failed = static_cast<uint64_t>(GetFailedFiles(STAT_TYPE_PHOTO).size() +
        GetFailedFiles(STAT_TYPE_VIDEO).size());
    error = totalNumber_ - success - duplicate - failed - notFoundNumber_;
}

int32_t CloneRestore::GetNoNeedMigrateCount()
{
    return this->photosClone_.GetNoNeedMigrateCount();
}

void CloneRestore::RestoreAnalysisClassify()
{
    CloneRestoreClassify cloneRestoreClassify;
    cloneRestoreClassify.Init(sceneCode_, taskId_, mediaLibraryRdb_, mediaRdb_);
    cloneRestoreClassify.Restore(photoInfoMap_);
}

void CloneRestore::RestoreAnalysisPortrait()
{
    CloneRestorePortrait portraitAlbumClone;
    bool isCloudRestoreSatisfied = IsCloudRestoreSatisfied();
    portraitAlbumClone.Init(sceneCode_, taskId_, mediaLibraryRdb_, mediaRdb_, photoInfoMap_, isCloudRestoreSatisfied);
    portraitAlbumClone.Preprocess();
    portraitAlbumClone.Restore();
}

void CloneRestore::RestoreAnalysisPet()
{
    CloneRestorePet petAlbumClone;
    bool isCloudRestoreSatisfied = IsCloudRestoreSatisfied();
    petAlbumClone.Init(sceneCode_, taskId_, mediaLibraryRdb_, mediaRdb_, photoInfoMap_, isCloudRestoreSatisfied);
    petAlbumClone.Preprocess();
    petAlbumClone.Restore();
}

void CloneRestore::RestoreAnalysisGeo()
{
    CloneRestoreGeo cloneRestoreGeo;
    cloneRestoreGeo.Init(sceneCode_, taskId_, mediaLibraryRdb_, mediaRdb_);
    cloneRestoreGeo.Restore(photoInfoMap_);
}

void CloneRestore::RestoreGroupPhoto()
{
    MEDIA_INFO_LOG("start RestoreGroupPhoto");
    CloneRestoreGroupPhoto cloneRestoreGroupPhoto;
    bool isCloudRestoreSatisfied = IsCloudRestoreSatisfied();
    cloneRestoreGroupPhoto.Init(sceneCode_, taskId_, restoreInfo_,
        mediaLibraryRdb_, mediaRdb_, isCloudRestoreSatisfied);
    cloneRestoreGroupPhoto.Restore(photoInfoMap_);
    MEDIA_INFO_LOG("end RestoreGroupPhoto");
}

void CloneRestore::UpdatePhotoAlbumCoverUri(vector<AlbumCoverInfo>& albumCoverInfos)
{
    for (auto& albumCoverInfo : albumCoverInfos) {
        int32_t changeRows = 0;
        std::unique_ptr<NativeRdb::AbsRdbPredicates> predicates =
            make_unique<NativeRdb::AbsRdbPredicates>(PhotoAlbumColumns::TABLE);
        predicates->EqualTo(PhotoAlbumColumns::ALBUM_ID, albumCoverInfo.albumId);
        NativeRdb::ValuesBucket updateBucket;
        updateBucket.PutInt(PhotoAlbumColumns::COVER_URI_SOURCE, albumCoverInfo.coverUriSource);
        updateBucket.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, albumCoverInfo.coverUri);
        updateBucket.PutString(PhotoAlbumColumns::COVER_CLOUD_ID, albumCoverInfo.coverCloudId);
        BackupDatabaseUtils::Update(mediaLibraryRdb_, changeRows, updateBucket, predicates);
        if (changeRows != 1) {
            MEDIA_ERR_LOG("UpdatePhotoAlbumCoverUri failed, expected count 1, but got %{public}d", changeRows);
        }
    }
}

void CloneRestore::UpdateExistNewAddColumnSet(const std::unordered_map<string, string> &srcColumnInfoMap)
{
    CHECK_AND_EXECUTE(srcColumnInfoMap.find(PhotoColumn::PHOTO_EXIF_ROTATE) == srcColumnInfoMap.end(),
        existNewAddColumnSet_.emplace(PhotoColumn::PHOTO_EXIF_ROTATE));
}

void CloneRestore::GetOrientationAndExifRotateValue(const shared_ptr<NativeRdb::ResultSet> &resultSet,
    FileInfo &fileInfo)
{
    fileInfo.orientation = GetInt32Val(PhotoColumn::PHOTO_ORIENTATION, resultSet);
    if (existNewAddColumnSet_.count(PhotoColumn::PHOTO_EXIF_ROTATE) != 0) {
        fileInfo.exifRotate = GetInt32Val(PhotoColumn::PHOTO_EXIF_ROTATE, resultSet);
    } else if (fileInfo.orientation == 0 || fileInfo.fileType != MediaType::MEDIA_TYPE_IMAGE) {
        fileInfo.exifRotate = 0;
    } else {
        ExifRotateUtils::ConvertOrientationToExifRotate(fileInfo.orientation, fileInfo.exifRotate);
    }
}

bool CloneRestore::HasExThumbnail(const FileInfo &info)
{
    CHECK_AND_RETURN_RET(info.position == static_cast<int32_t>(PhotoPositionType::LOCAL) ||
        info.position == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD),
        BackupFileUtils::HasOrientationOrExifRotate(info));
    return info.fileType == MediaType::MEDIA_TYPE_IMAGE && BackupFileUtils::HasOrientationOrExifRotate(info);
}

void CloneRestore::BackupRelease()
{
    StopParameterForBackup();
}

bool CloneRestore::CheckSouthDeviceTypeMatchSwitchStatus(SwitchStatus switchStatus)
{
    CHECK_AND_RETURN_RET_LOG(this->mediaRdb_, false, "mediaRdb_ is nullptr");
    std::vector<SouthDeviceType> srcUniqueSouthDeviceTypes;
    CHECK_AND_RETURN_RET_LOG(
        BackupDatabaseUtils::QueryPhotoUniqueSouthDeviceType(this->mediaRdb_, srcUniqueSouthDeviceTypes), false,
        "fail to query unique south_device_type");

    CHECK_AND_RETURN_RET_LOG(PHOTO_SYNC_OPTION_SOUTH_DEVICE_TYPE_MAP.count(switchStatus),
        false, "cannot transfer srcSwitchStatus:%{public}d to srcSouthDeviceType",
        static_cast<int>(switchStatus));
    auto expectedSrcSouthDeviceType = PHOTO_SYNC_OPTION_SOUTH_DEVICE_TYPE_MAP.at(switchStatus);
    for (const auto& southDeviceType : srcUniqueSouthDeviceTypes) {
        CHECK_AND_RETURN_RET_LOG(southDeviceType == expectedSrcSouthDeviceType, false,
            "south_device_type:%{public}d is not expected, expect %{public}d",
            static_cast<int32_t>(southDeviceType), static_cast<int32_t>(expectedSrcSouthDeviceType));
    }
    return true;
}

void CloneRestore::SetAggregateBitThird()
{
    CHECK_AND_RETURN_LOG(mediaLibraryRdb_ != nullptr, "SetAggregateBitThird failed, rdbStore is nullptr");
    DataTransfer::MediaLibraryDbUpgrade medialibraryDbUpgrade;
    std::vector<NativeRdb::ValueObject> params = {};
    params.push_back(NativeRdb::ValueObject(std::to_string(PhotoAlbumType::SMART)));
    params.push_back(NativeRdb::ValueObject(std::to_string(PhotoAlbumSubType::CLASSIFY)));
    auto resultSet = mediaLibraryRdb_->QuerySql(SQL_QUERY_CLASSIFY_ALBUM_EXIST, params);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "resultSet is nullptr");
    if (resultSet->GoToNextRow() == NativeRdb::E_OK && GetInt32Val("count", resultSet) > 0) {
        resultSet->Close();
        MEDIA_INFO_LOG("classify album already exist, no need to SetAggregateBitThird");
        return;
    }
    resultSet->Close();
    int32_t bitPosition = 2;
    medialibraryDbUpgrade.SetAggregateBit(bitPosition);
}

void CloneRestore::PopulateAnalysisAlbumIdMap()
{
    MEDIA_INFO_LOG("Mapping portrait, group photo, and shooting mode albums");
    
    std::string srcSql = "SELECT album_id, album_type, album_name, tag_id, album_subtype FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE album_subtype IN (" + std::to_string(PhotoAlbumSubType::PORTRAIT) +
        ", " + std::to_string(PhotoAlbumSubType::GROUP_PHOTO) +
        ", " + std::to_string(PhotoAlbumSubType::SHOOTING_MODE) + ")";
    
    auto srcResultSet = mediaRdb_->QuerySql(srcSql);
    CHECK_AND_RETURN_LOG(srcResultSet != nullptr, "Failed to query special album types from source");
    
    auto& albumIdMap = tableAlbumIdMap_[ANALYSIS_ALBUM_TABLE];
    int32_t mappedCount = 0;
    
    while (srcResultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t oldAlbumId = GetInt32Val("album_id", srcResultSet);
        int32_t albumType = GetInt32Val("album_type", srcResultSet);
        int32_t albumSubtype = GetInt32Val("album_subtype", srcResultSet);
        std::string albumName = GetStringVal("album_name", srcResultSet);
        std::string tagId = GetStringVal("tag_id", srcResultSet);
        
        // Skip if already mapped
        if (albumIdMap.find(oldAlbumId) != albumIdMap.end()) {
            continue;
        }
        
        std::string dstSql = "SELECT album_id FROM " + ANALYSIS_ALBUM_TABLE +
            " WHERE album_type = " + std::to_string(albumType) +
            " AND album_subtype = " + std::to_string(albumSubtype);
        
        if (albumName.empty()) {
            dstSql += " AND (album_name IS NULL OR album_name = '')";
        } else {
            dstSql += " AND album_name = '" + albumName + "'";
        }

        if (tagId.empty()) {
            dstSql += " AND (tag_id IS NULL OR tag_id = '')";
        } else {
            dstSql += " AND tag_id = '" + tagId + "'";
        }
        
        auto dstResultSet = mediaLibraryRdb_->QuerySql(dstSql);
        if (dstResultSet != nullptr && dstResultSet->GoToNextRow() == NativeRdb::E_OK) {
            int32_t newAlbumId = GetInt32Val("album_id", dstResultSet);
            albumIdMap[oldAlbumId] = newAlbumId;
            mappedCount++;
            MEDIA_DEBUG_LOG("Mapped album (subtype=%{public}d): old=%{public}d -> new=%{public}d",
                albumSubtype, oldAlbumId, newAlbumId);
        }
        if (dstResultSet != nullptr) {
            dstResultSet->Close();
        }
    }
    srcResultSet->Close();
    
    MEDIA_INFO_LOG("Album mapping completed. Mapped %{public}d albums", mappedCount);
}

void CloneRestore::StoreHighlightAlbumMappings(CloneRestoreHighlight& cloneRestoreHighlight)
{
    MEDIA_INFO_LOG("Storing highlight album mappings from analysisInfos_");
    
    const auto& analysisInfos = cloneRestoreHighlight.GetAnalysisInfos();
    auto& albumIdMap = tableAlbumIdMap_[ANALYSIS_ALBUM_TABLE];
    int32_t mappedCount = 0;
    
    for (const auto& info : analysisInfos) {
        if (info.albumIdOld.has_value() && info.albumIdNew.has_value()) {
            int32_t oldId = info.albumIdOld.value();
            int32_t newId = info.albumIdNew.value();
            
            // Skip if already mapped
            if (albumIdMap.find(oldId) == albumIdMap.end()) {
                albumIdMap[oldId] = newId;
                mappedCount++;
                MEDIA_DEBUG_LOG("Stored highlight album mapping: old_id=%{public}d -> new_id=%{public}d", oldId, newId);
            }
        }
    }
    
    MEDIA_INFO_LOG("Highlight album mappings stored from analysisInfos_. Mapped %{public}d albums", mappedCount);
}
} // namespace Media
} // namespace OHOS

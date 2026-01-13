/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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
#define MLOG_TAG "RdbStore"

#include "medialibrary_rdbstore.h"

#include <regex>
#include <thread>
#include <chrono>

#include "album_plugin_table_event_handler.h"
#include "cloud_sync_helper.h"
#include "dfx_timer.h"
#include "dfx_const.h"
#include "dfx_reporter.h"
#include "media_app_uri_permission_column.h"
#include "media_old_photos_column.h"
#include "medialibrary_album_fusion_utils.h"
#include "medialibrary_business_record_column.h"
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_restore.h"
#include "medialibrary_tracer.h"
#include "media_container_types.h"
#include "media_scanner_manager.h"
#ifdef META_RECOVERY_SUPPORT
#include "medialibrary_meta_recovery.h"
#endif
#include "medialibrary_notify.h"
#include "medialibrary_operation_record.h"
#include "moving_photo_processor.h"
#include "parameters.h"
#include "parameter.h"
#include "photo_file_utils.h"
#include "post_event_utils.h"
#include "rdb_sql_utils.h"
#include "result_set_utils.h"
#include "tab_old_photos_table_event_handler.h"
#include "tab_facard_photos_table_event_handler.h"
#include "tab_old_albums_table_event_handler.h"
#include "vision_ocr_column.h"
#include "form_map.h"
#include "search_column.h"
#include "shooting_mode_column.h"
#include "story_db_sqls.h"
#include "dfx_const.h"
#include "dfx_timer.h"
#include "dfx_utils.h"
#include "preferences_helper.h"
#include "thumbnail_service.h"
#include "table_event_handler.h"
#include "values_buckets.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_notify_new.h"
#include "photo_map_table_event_handler.h"
#include "download_resources_table_event_handler.h"
#include "media_app_uri_sensitive_column.h"
#include "medialibrary_upgrade_utils.h"
#include "media_config_info_column.h"
#include "download_resources_column.h"
#include "tab_cloned_old_photos_table_event_handler.h"
#include "media_audio_column.h"


using namespace std;
using namespace OHOS::NativeRdb;
namespace OHOS::Media {
const std::string DIR_ALL_AUDIO_CONTAINER_TYPE = "." + AUDIO_CONTAINER_TYPE_AAC + "?" +
                                                 "." + AUDIO_CONTAINER_TYPE_MP3 + "?" +
                                                 "." + AUDIO_CONTAINER_TYPE_FLAC + "?" +
                                                 "." + AUDIO_CONTAINER_TYPE_WAV + "?" +
                                                 "." + AUDIO_CONTAINER_TYPE_OGG + "?" +
                                                 "." + AUDIO_CONTAINER_TYPE_M4A + "?";

const std::string DIR_ALL_VIDEO_CONTAINER_TYPE = "." + VIDEO_CONTAINER_TYPE_MP4 + "?" +
                                                 "." + VIDEO_CONTAINER_TYPE_3GP + "?" +
                                                 "." + VIDEO_CONTAINER_TYPE_MPG + "?" +
                                                 "." + VIDEO_CONTAINER_TYPE_MOV + "?" +
                                                 "." + VIDEO_CONTAINER_TYPE_WEBM + "?" +
                                                 "." + VIDEO_CONTAINER_TYPE_MKV + "?" +
                                                 "." + VIDEO_CONTAINER_TYPE_H264 + "?" +
                                                 "." + VIDEO_CONTAINER_TYPE_MPEG + "?" +
                                                 "." + VIDEO_CONTAINER_TYPE_TS + "?" +
                                                 "." + VIDEO_CONTAINER_TYPE_M4V + "?" +
                                                 "." + VIDEO_CONTAINER_TYPE_3G2 + "?";

const std::string DIR_ALL_IMAGE_CONTAINER_TYPE = "." + IMAGE_CONTAINER_TYPE_BMP + "?" +
                                                 "." + IMAGE_CONTAINER_TYPE_BM + "?" +
                                                 "." + IMAGE_CONTAINER_TYPE_GIF + "?" +
                                                 "." + IMAGE_CONTAINER_TYPE_JPG + "?" +
                                                 "." + IMAGE_CONTAINER_TYPE_JPEG + "?" +
                                                 "." + IMAGE_CONTAINER_TYPE_JPE + "?" +
                                                 "." + IMAGE_CONTAINER_TYPE_PNG + "?" +
                                                 "." + IMAGE_CONTAINER_TYPE_WEBP + "?" +
                                                 "." + IMAGE_CONTAINER_TYPE_RAW + "?" +
                                                 "." + IMAGE_CONTAINER_TYPE_SVG + "?" +
                                                 "." + IMAGE_CONTAINER_TYPE_HEIF + "?";

const std::string CAMERA_EXTENSION_VALUES = DIR_ALL_IMAGE_CONTAINER_TYPE + DIR_ALL_VIDEO_CONTAINER_TYPE;

const std::string VIDEO_EXTENSION_VALUES = DIR_ALL_VIDEO_CONTAINER_TYPE;

const std::string PIC_EXTENSION_VALUES = DIR_ALL_IMAGE_CONTAINER_TYPE;

const std::string AUDIO_EXTENSION_VALUES = DIR_ALL_AUDIO_CONTAINER_TYPE;

const std::string RDB_CONFIG = "/data/storage/el2/base/preferences/rdb_config.xml";

const std::string RDB_OLD_VERSION = "rdb_old_version";

const std::string SLAVE = "slave";

constexpr ssize_t RDB_WAL_LIMIT_SIZE = 1024 * 1024 * 1024; /* default wal file maximum size : 1GB */
constexpr ssize_t RDB_CHECK_WAL_SIZE = 50 * 1024 * 1024;   /* check wal file size : 50MB */
std::mutex MediaLibraryRdbStore::walCheckPointMutex_;

shared_ptr<NativeRdb::RdbStore> MediaLibraryRdbStore::rdbStore_;

std::mutex MediaLibraryRdbStore::reconstructLock_;

int32_t oldVersion_ = -1;

constexpr int32_t POS_ALBUM_ID = 0;

constexpr int32_t POS_PATH = 1;

const int TRASH_ALBUM_TYPE_VALUES = 2;

const int32_t ARG_COUNT = 2;
const std::string TRASH_ALBUM_NAME_VALUES = "TrashAlbum";

struct UniqueMemberValuesBucket {
    std::string assetMediaType;
    int32_t startNumber;
};


struct ShootingModeValueBucket {
    int32_t albumType;
    int32_t albumSubType;
    std::string albumName;
};

static int32_t ExecSqlWithRetry(std::function<int32_t()> execSql)
{
    int32_t currentTime = 0;
    int32_t busyRetryTime = 0;
    int32_t err = NativeRdb::E_OK;
    bool isSkipCloudSync = false;
    while (busyRetryTime < MAX_BUSY_TRY_TIMES && currentTime <= MAX_TRY_TIMES) {
        err = execSql();
        if (err == NativeRdb::E_OK) {
            break;
        } else if (err == NativeRdb::E_SQLITE_LOCKED) {
            std::this_thread::sleep_for(std::chrono::milliseconds(TRANSACTION_WAIT_INTERVAL));
            currentTime++;
            MEDIA_ERR_LOG("execSql busy, err: %{public}d, currentTime: %{public}d", err, currentTime);
        } else if (err == NativeRdb::E_SQLITE_BUSY || err == NativeRdb::E_DATABASE_BUSY) {
            busyRetryTime++;
            MEDIA_ERR_LOG("execSql busy, err:%{public}d, busyRetryTime:%{public}d", err, busyRetryTime);
            if (err == NativeRdb::E_SQLITE_BUSY && !isSkipCloudSync) {
                MEDIA_INFO_LOG("Stop cloud sync");
                FileManagement::CloudSync::CloudSyncManager::GetInstance()
                    .StopSync("com.ohos.medialibrary.medialibrarydata");
                isSkipCloudSync = true;
            }
        } else {
            MEDIA_ERR_LOG("execSql failed, err: %{public}d, currentTime: %{public}d", err, currentTime);
            break;
        }
    }
    if (isSkipCloudSync) {
        MEDIA_INFO_LOG("recover cloud sync after execsql busy");
        CloudSyncHelper::GetInstance()->StartSync();
    }
    return err;
}

const std::string MediaLibraryRdbStore::CloudSyncTriggerFunc(const std::vector<std::string> &args)
{
    CloudSyncHelper::GetInstance()->StartSync();
    return "";
}

const std::string MediaLibraryRdbStore::BeginGenerateHighlightThumbnail(const std::vector<std::string> &args)
{
    if (args.size() < STAMP_PARAM || args[STAMP_PARAM_ZERO].empty() || args[STAMP_PARAM_ONE].empty() ||
        args[STAMP_PARAM_TWO].empty() || args[STAMP_PARAM_THREE].empty()) {
            MEDIA_ERR_LOG("Invalid input: args must contain at least 4 non-empty strings");
            return "";
    }
    std::string id = args[STAMP_PARAM_ZERO].c_str();
    std::string tracks = args[STAMP_PARAM_ONE].c_str();
    std::string trigger = args[STAMP_PARAM_TWO].c_str();
    std::string genType = args[STAMP_PARAM_THREE].c_str();
    MEDIA_INFO_LOG("id = %{public}s, tracks = %{public}s, trigger = %{public}s", id.c_str(),
        tracks.c_str(), trigger.c_str());
    ThumbnailService::GetInstance()->TriggerHighlightThumbnail(id, tracks, trigger, genType);
    return "";
}

const std::string MediaLibraryRdbStore::IsCallerSelfFunc(const std::vector<std::string> &args)
{
    return "true";
}

constexpr int REGEXP_REPLACE_PARAM_NUM = 3;
const std::string MediaLibraryRdbStore::RegexReplaceFunc(const std::vector<std::string> &args)
{
    if (args.size() < REGEXP_REPLACE_PARAM_NUM) {
        MEDIA_ERR_LOG("Invalid arg count %{public}zu: args must contain at least 3 strings", args.size());
        return "";
    }
    const std::string &input = args[0];
    const std::string &pattern = args[1];
    const std::string &replacement = args[2];

    std::regex re(pattern);
    return std::regex_replace(input, re, replacement);
}

const std::string MediaLibraryRdbStore::PhotoAlbumNotifyFunc(const std::vector<std::string> &args)
{
    if (args.size() < 1) {
        MEDIA_ERR_LOG("Invalid input: args must contain at least 1 strings");
        return "";
    }
    std::string path = args[POS_PATH].c_str();
    size_t slavePosition = path.find(SLAVE);
    if (slavePosition != string::npos) {
        MEDIA_DEBUG_LOG("not notify slave db");
        return "";
    }
    std::string albumId = args[POS_ALBUM_ID].c_str();
    if (!all_of(albumId.begin(), albumId.end(), ::isdigit)) {
        MEDIA_ERR_LOG("Invalid albunId PhotoAlbumNotifyFunc Abortion");
        return "";
    }

    MEDIA_DEBUG_LOG("albumId = %{public}s", albumId.c_str());
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, "", "Failed to get MediaLibraryNotify");
    watch->Notify(MediaFileUtils::GetUriByExtrConditions(PhotoAlbumColumns::ALBUM_URI_PREFIX, albumId),
        NotifyType::NOTIFY_ADD);

    Notification::MediaLibraryNotifyNew::AddAlbum(albumId);
    MEDIA_INFO_LOG("AccurateRefresh PhotoAlbumNotifyFunc albumId = %{public}s", albumId.c_str());
    return "";
}

MediaLibraryRdbStore::MediaLibraryRdbStore(const shared_ptr<OHOS::AbilityRuntime::Context> &context)
{
    if (context == nullptr) {
        MEDIA_ERR_LOG("Failed to get context");
        return;
    }
    string databaseDir = context->GetDatabaseDir();
    string name = MEDIA_DATA_ABILITY_DB_NAME;
    int32_t errCode = 0;
    string realPath = RdbSqlUtils::GetDefaultDatabasePath(databaseDir, name, errCode);
    config_.SetHaMode(HAMode::MANUAL_TRIGGER);
    config_.SetAllowRebuild(true);
    config_.SetName(move(name));
    config_.SetPath(move(realPath));
    config_.SetBundleName(context->GetBundleName());
    config_.SetArea(context->GetArea());
    config_.SetSecurityLevel(SecurityLevel::S3);
    config_.SetScalarFunction("cloud_sync_func", 0, CloudSyncTriggerFunc);
    config_.SetScalarFunction("is_caller_self_func", 0, IsCallerSelfFunc);
    config_.SetScalarFunction("REGEXP_REPLACE", REGEXP_REPLACE_PARAM_NUM, RegexReplaceFunc);
    config_.SetScalarFunction("begin_generate_highlight_thumbnail", STAMP_PARAM, BeginGenerateHighlightThumbnail);
    config_.SetWalLimitSize(RDB_WAL_LIMIT_SIZE);
    config_.SetScalarFunction("photo_album_notify_func", ARG_COUNT, PhotoAlbumNotifyFunc);
}

bool g_upgradeErr = false;
void UpdateFail(const string &errFile, const int &errLine)
{
    g_upgradeErr = true;
    VariantMap map = {{KEY_ERR_FILE, errFile}, {KEY_ERR_LINE, errLine}};
    PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_UPGRADE_ERR, map);
}

static int32_t ExecSqls(const vector<string> &sqls, RdbStore &store)
{
    int32_t err = NativeRdb::E_OK;
    for (const auto &sql : sqls) {
        err = ExecSqlWithRetry([&]() { return store.ExecuteSql(sql); });
        if (err != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Failed to exec: %{private}s", sql.c_str());
            /* try update as much as possible */
            UpdateFail(__FILE__, __LINE__);
            continue;
        }
    }
    return NativeRdb::E_OK;
}

static int32_t ExecSqlsWithDfx(const vector<string> &sqls, RdbStore &store, int32_t version)
{
    for (size_t i = 0; i < sqls.size(); ++i) {
        int32_t err = ExecSqlWithRetry([&]() { return store.ExecuteSql(sqls[i]); });
        if (err != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Failed to exec: %{private}s", sqls[i].c_str());
            /* try update as much as possible */
            UpdateFail(__FILE__, __LINE__);
            RdbUpgradeUtils::AddUpgradeDfxMessages(version, i, err);
            continue;
        }
    }
    return NativeRdb::E_OK;
}

void MediaLibraryRdbStore::DropPhotoStatusForSearchIndex(const shared_ptr<MediaLibraryRdbStore> store, int32_t version)
{
    MEDIA_INFO_LOG("Drop photo status for search index start");
    const vector<string> sqls = {
        "DROP INDEX IF EXISTS idx_photo_status_for_search_index",
    };
    ExecSqlsWithDfx(sqls, *store->GetRaw().get(), version);
    MEDIA_INFO_LOG("Drop photo status for search index end");
}

void MediaLibraryRdbStore::AddPetTagIdIndex(const shared_ptr<MediaLibraryRdbStore> store, int32_t version)
{
    const vector<string> executeSqlStrs = {
        CREATE_PET_INDEX,
        CREATE_PET_TAG_ID_INDEX,
        UPDATE_PET_TOTAL_VALUE,
    };
    MEDIA_INFO_LOG("add pet face index start");
    ExecSqlsWithDfx(executeSqlStrs, *store->GetRaw().get(), version);
    MEDIA_INFO_LOG("add pet face index end");
}

void MediaLibraryRdbStore::CreateBurstIndex(const shared_ptr<MediaLibraryRdbStore> store)
{
    const vector<string> sqls = {
        PhotoUpgrade::DROP_SCHPT_DAY_INDEX,
        PhotoUpgrade::CREATE_SCHPT_DAY_INDEX,
        PhotoUpgrade::DROP_SCHPT_HIDDEN_TIME_INDEX,
        PhotoUpgrade::CREATE_SCHPT_HIDDEN_TIME_INDEX,
        PhotoUpgrade::DROP_PHOTO_FAVORITE_INDEX,
        PhotoUpgrade::CREATE_PHOTO_FAVORITE_INDEX,
        PhotoUpgrade::DROP_INDEX_SCTHP_ADDTIME,
        PhotoUpgrade::INDEX_SCTHP_ADDTIME,
        PhotoUpgrade::DROP_SCHPT_MEDIA_TYPE_INDEX,
        PhotoUpgrade::CREATE_SCHPT_MEDIA_TYPE_INDEX,
        PhotoUpgrade::CREATE_PHOTO_BURSTKEY_INDEX
    };
    MEDIA_INFO_LOG("start create idx_burstkey");
    ExecSqls(sqls, *store->GetRaw().get());
    MEDIA_INFO_LOG("end create idx_burstkey");
}
void MediaLibraryRdbStore::AddUpgradeIndex(const shared_ptr<MediaLibraryRdbStore> store)
{
    const vector<string> sqls = {
        PhotoUpgrade::CREATE_SCHPT_DAY_INDEX,
        PhotoUpgrade::CREATE_SCHPT_MEDIA_TYPE_INDEX,
        PhotoUpgrade::CREATE_SCHPT_HIDDEN_TIME_INDEX,
        PhotoUpgrade::CREATE_PHOTO_FAVORITE_INDEX,
        PhotoUpgrade::CREATE_PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX,
        PhotoUpgrade::CREATE_PHOTO_BURST_MODE_ALBUM_INDEX,
        PhotoUpgrade::CREATE_PHOTO_FRONT_CAMERA_ALBUM_INDEX,
        PhotoUpgrade::CREATE_PHOTO_RAW_IMAGE_ALBUM_INDEX,
        PhotoUpgrade::INDEX_QUERY_THUMBNAIL_WHITE_BLOCKS
    };
    MEDIA_INFO_LOG("start create idx again");
    ExecSqls(sqls, *store->GetRaw().get());
    MEDIA_INFO_LOG("end create idx again");
}

void MediaLibraryRdbStore::UpdateIndexHiddenTime(const shared_ptr<MediaLibraryRdbStore> store, int32_t version)
{
    const vector<string> sqls = {
        BaseColumn::DropIndex() + PhotoColumn::PHOTO_SCHPT_HIDDEN_TIME_INDEX,
        PhotoUpgrade::CREATE_SCHPT_HIDDEN_TIME_INDEX,
    };
    MEDIA_INFO_LOG("start update idx_schpt_hidden_time");
    ExecSqlsWithDfx(sqls, *store->GetRaw().get(), version);
    MEDIA_INFO_LOG("end update idx_schpt_hidden_time");
}

void MediaLibraryRdbStore::UpdateIndexDateAdded(const shared_ptr<MediaLibraryRdbStore> store, int32_t version)
{
    const vector<string> sqls = {
        BaseColumn::DropIndex() + PhotoColumn::PHOTO_SCHPT_PHOTO_DATEADDED_INDEX,
        PhotoUpgrade::INDEX_SCTHP_PHOTO_DATEADDED
    };
    MEDIA_INFO_LOG("start update idx_schpt_date_added_new");
    ExecSqlsWithDfx(sqls, *store->GetRaw().get(), version);
    MEDIA_INFO_LOG("end update idx_schpt_date_added_new");
}

void MediaLibraryRdbStore::AddVideoFaceTagIdIndex(const shared_ptr<MediaLibraryRdbStore> store, int32_t version)
{
    const vector<string> sqls = {
        CREATE_VIDEO_FACE_TAG_ID_INDEX,
    };
    MEDIA_INFO_LOG("start add video_face_tag_id_index");
    ExecSqlsWithDfx(sqls, *store->GetRaw().get(), version);
    MEDIA_INFO_LOG("end add video_face_tag_id_index");
}

void MediaLibraryRdbStore::UpdateBurstDirty(const shared_ptr<MediaLibraryRdbStore> store)
{
    const vector<string> sqls = {
        "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_DIRTY + " = " +
        to_string(static_cast<int32_t>(DirtyTypes::TYPE_NEW)) + " WHERE " + PhotoColumn::PHOTO_SUBTYPE + " = " +
        to_string(static_cast<int32_t>(PhotoSubType::BURST)) + " AND " + PhotoColumn::PHOTO_DIRTY + " = -1 ",
    };
    MEDIA_INFO_LOG("start UpdateBurstDirty");
    ExecSqls(sqls, *store->GetRaw().get());
    MEDIA_INFO_LOG("end UpdateBurstDirty");
}

void MediaLibraryRdbStore::UpdateReadyOnThumbnailUpgrade(const shared_ptr<MediaLibraryRdbStore> store)
{
    const vector<string> sqls = {
        PhotoUpgrade::UPDATE_READY_ON_THUMBNAIL_UPGRADE,
    };
    MEDIA_INFO_LOG("start update ready for thumbnail upgrade");
    ExecSqls(sqls, *store->GetRaw().get());
    MEDIA_INFO_LOG("finish update ready for thumbnail upgrade");
}

void MediaLibraryRdbStore::UpdateDateTakenToMillionSecond(const shared_ptr<MediaLibraryRdbStore> store)
{
    MEDIA_INFO_LOG("UpdateDateTakenToMillionSecond start");
    const vector<string> updateSql = {
        "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " +
            MediaColumn::MEDIA_DATE_TAKEN + " = " + MediaColumn::MEDIA_DATE_TAKEN +  "*1000 WHERE " +
            MediaColumn::MEDIA_DATE_TAKEN + " < 1e10",
    };
    ExecSqls(updateSql, *store->GetRaw().get());
    MEDIA_INFO_LOG("UpdateDateTakenToMillionSecond end");
}

void MediaLibraryRdbStore::UpdateDateTakenIndex(const shared_ptr<MediaLibraryRdbStore> store)
{
    const vector<string> sqls = {
        PhotoUpgrade::DROP_SCHPT_MEDIA_TYPE_INDEX,
        PhotoUpgrade::DROP_PHOTO_FAVORITE_INDEX,
        PhotoUpgrade::DROP_INDEX_SCTHP_ADDTIME,
        PhotoUpgrade::DROP_INDEX_SCHPT_READY,
        PhotoUpgrade::CREATE_SCHPT_MEDIA_TYPE_INDEX,
        PhotoUpgrade::CREATE_PHOTO_FAVORITE_INDEX,
        PhotoUpgrade::INDEX_SCTHP_ADDTIME,
        PhotoUpgrade::INDEX_SCHPT_READY,
    };
    MEDIA_INFO_LOG("update index for datetaken change start");
    ExecSqls(sqls, *store->GetRaw().get());
    MEDIA_INFO_LOG("update index for datetaken change end");
}

// 更新单条编辑数据大小
int32_t MediaLibraryRdbStore::UpdateEditDataSize(std::shared_ptr<MediaLibraryRdbStore> rdbStore,
    const std::string &photoId, const std::string &editDataDir)
{
    size_t size = 0;
    MediaFileUtils::StatDirSize(editDataDir, size);
    std::string sql = "UPDATE " + PhotoExtColumn::PHOTOS_EXT_TABLE + " "
                    "SET " + PhotoExtColumn::EDITDATA_SIZE + " = " + std::to_string(size) + " "
                    "WHERE " + PhotoExtColumn::PHOTO_ID + " = '" + photoId + "'";

    int32_t ret = rdbStore->ExecuteSql(sql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute SQL failed: %{public}d", ret);
        return E_DB_FAIL;
    }
    return E_OK;
}

void MediaLibraryRdbStore::UpdateDateTakenAndDetalTime(const shared_ptr<MediaLibraryRdbStore> store)
{
    MEDIA_INFO_LOG("UpdateDateTakenAndDetalTime start");
    string updateDateTakenSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + MediaColumn::MEDIA_DATE_TAKEN +
            " = " + PhotoColumn::MEDIA_DATE_MODIFIED + "," + PhotoColumn::PHOTO_DETAIL_TIME +
            " = strftime('%Y:%m:%d %H:%M:%S', " + MediaColumn::MEDIA_DATE_MODIFIED +
            "/1000, 'unixepoch', 'localtime')" + " WHERE " + MediaColumn::MEDIA_DATE_TAKEN + " = 0";
    string updateDetalTimeSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_DETAIL_TIME +
            " = strftime('%Y:%m:%d %H:%M:%S', " + MediaColumn::MEDIA_DATE_TAKEN + "/1000, 'unixepoch', 'localtime')" +
            " WHERE " + PhotoColumn::PHOTO_DETAIL_TIME + " IS NULL";
    const vector<string> updateSql = {
        updateDateTakenSql,
        updateDetalTimeSql,
    };
    ExecSqls(updateSql, *store->GetRaw().get());
    MEDIA_INFO_LOG("UpdateDateTakenAndDetalTime end");
}

void MediaLibraryRdbStore::ClearAudios(const shared_ptr<MediaLibraryRdbStore> store)
{
    const vector<string> sqls = {
        "DELETE From Audios",
    };
    MEDIA_INFO_LOG("clear audios start");
    ExecSqls(sqls, *store->GetRaw().get());
    MEDIA_INFO_LOG("clear audios end");
}

void MediaLibraryRdbStore::UpdateIndexForCover(const shared_ptr<MediaLibraryRdbStore> store)
{
    const vector<string> sqls = {
        PhotoUpgrade::DROP_SCHPT_MEDIA_TYPE_INDEX,
        PhotoUpgrade::CREATE_SCHPT_MEDIA_TYPE_INDEX,
    };
    MEDIA_INFO_LOG("update index for photo album cover start");
    ExecSqls(sqls, *store->GetRaw().get());
    MEDIA_INFO_LOG("update index for photo album cover end");
}

void MediaLibraryRdbStore::UpdateLcdStatusNotUploaded(const std::shared_ptr<MediaLibraryRdbStore> store)
{
    const vector<string> sqls = {
        PhotoUpgrade::UPDATE_LCD_STATUS_NOT_UPLOADED,
    };
    MEDIA_INFO_LOG("start update lcd status for photos have not been uploaded");
    ExecSqls(sqls, *store->GetRaw().get());
    MEDIA_INFO_LOG("finish update lcd status for photos have not been uploaded");

    MEDIA_INFO_LOG("start CheckLcdSizeAndUpdateStatus");
    ThumbnailService::GetInstance()->CheckLcdSizeAndUpdateStatus();
    MEDIA_INFO_LOG("finish CheckLcdSizeAndUpdateStatus");
}

void MediaLibraryRdbStore::AddReadyCountIndex(const shared_ptr<MediaLibraryRdbStore> store)
{
    MEDIA_INFO_LOG("start add ready count index");
    const vector<string> sqls = {
        PhotoUpgrade::CREATE_SCHPT_MEDIA_TYPE_COUNT_READY_INDEX,
        PhotoUpgrade::CREATE_SCHPT_YEAR_COUNT_READY_INDEX,
        PhotoUpgrade::CREATE_SCHPT_MONTH_COUNT_READY_INDEX,
    };
    ExecSqls(sqls, *store->GetRaw().get());
    MEDIA_INFO_LOG("end add ready count index");
}

void MediaLibraryRdbStore::RevertFixDateAddedIndex(const shared_ptr<MediaLibraryRdbStore> store)
{
    MEDIA_INFO_LOG("start revert fix date added index");
    const vector<string> sqls = {
        PhotoUpgrade::DROP_INDEX_SCTHP_ADDTIME,
        PhotoUpgrade::INDEX_SCTHP_ADDTIME,
        PhotoUpgrade::DROP_INDEX_SCHPT_ADDTIME_ALBUM,
    };
    ExecSqls(sqls, *store->GetRaw().get());
    MEDIA_INFO_LOG("end revert fix date added index");
}

void MediaLibraryRdbStore::AddCloudEnhancementAlbumIndex(const shared_ptr<MediaLibraryRdbStore> store)
{
    const vector<string> sqls = {
        PhotoUpgrade::CREATE_SCHPT_CLOUD_ENHANCEMENT_ALBUM_INDEX
    };
    MEDIA_INFO_LOG("start create idx_schpt_cloud_enhancement_album_index");
    ExecSqls(sqls, *store->GetRaw().get());
    MEDIA_INFO_LOG("end create idx_schpt_cloud_enhancement_album_index");
}

void MediaLibraryRdbStore::AddAlbumIndex(const shared_ptr<MediaLibraryRdbStore> store)
{
    MEDIA_INFO_LOG("start add album index");
    const vector<string> sqls = {
        PhotoUpgrade::INDEX_SCHPT_ALBUM_GENERAL,
        PhotoUpgrade::INDEX_SCHPT_ALBUM,
    };
    ExecSqls(sqls, *store->GetRaw().get());
    MEDIA_INFO_LOG("end add album index");
}

void MediaLibraryRdbStore::UpdateLocationKnowledgeIdx(const shared_ptr<MediaLibraryRdbStore> store)
{
    MEDIA_INFO_LOG("start update location knowledge index");
    const vector<string> sqls = {
        DROP_KNOWLEDGE_INDEX,
        CREATE_NEW_KNOWLEDGE_INDEX
    };
    ExecSqls(sqls, *store->GetRaw().get());
    MEDIA_INFO_LOG("end update location knowledge index");
}

void MediaLibraryRdbStore::AddAlbumSubtypeAndNameIdx(const shared_ptr<MediaLibraryRdbStore> store)
{
    MEDIA_INFO_LOG("start to add album subtype and name index");
    const vector<string> sqls = {
        CREATE_ANALYSIS_ALBUM_SUBTYPE_NAME_INDEX,
        CREATE_ANALYSIS_ALBUM_TAG_ID_INDEX
    };
    ExecSqls(sqls, *store->GetRaw().get());
    MEDIA_INFO_LOG("end add album subtype and name index");
}

void MediaLibraryRdbStore::UpdateThumbnailVisibleAndIdx(const shared_ptr<MediaLibraryRdbStore> store)
{
    MEDIA_INFO_LOG("start to update thumbnailvisible and index");
    const vector<string> sqls = {
        "UPDATE " + PhotoColumn::PHOTOS_TABLE +
        " SET thumbnail_visible = "
        " CASE "
            " WHEN thumbnail_ready > 0 THEN 1 "
            " ELSE 0 "
        " END ",
        PhotoUpgrade::DROP_INDEX_SCHPT_READY,
        PhotoUpgrade::INDEX_SCHPT_READY,
    };
    ExecSqls(sqls, *store->GetRaw().get());
    MEDIA_INFO_LOG("end update thumbnailvisible and index");
}

void MediaLibraryRdbStore::UpdateMediaTypeAndThumbnailReadyIdx(const shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    bool cond = (rdbStore == nullptr || !rdbStore->CheckRdbStore());
    CHECK_AND_RETURN_LOG(!cond, "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");

    const vector<string> sqls = {
        PhotoUpgrade::DROP_SCHPT_MEDIA_TYPE_COUNT_READY_INDEX,
        PhotoUpgrade::CREATE_SCHPT_MEDIA_TYPE_COUNT_READY_INDEX,
        PhotoUpgrade::DROP_INDEX_SCHPT_READY,
        PhotoUpgrade::INDEX_SCHPT_READY,
    };

    MEDIA_INFO_LOG("start update idx_schpt_media_type_ready and idx_schpt_thumbnail_ready");
    ExecSqls(sqls, *rdbStore->GetRaw().get());
    MEDIA_INFO_LOG("end update idx_schpt_media_type_ready and idx_schpt_thumbnail_ready");
}

int32_t MediaLibraryRdbStore::Init()
{
    MEDIA_INFO_LOG("Init rdb store: [version: %{public}d]", MEDIA_RDB_VERSION);
    CHECK_AND_RETURN_RET(rdbStore_ == nullptr, E_OK);

    int32_t errCode = 0;
    MediaLibraryDataCallBack rdbDataCallBack;
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRdbStore::Init GetRdbStore");
    rdbStore_ = RdbHelper::GetRdbStore(config_, MEDIA_RDB_VERSION, rdbDataCallBack, errCode);
    tracer.Finish();
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, errCode, "GetRdbStore is failed");
    int version = 0;
    NativeRdb::RebuiltType rebuilt;
    bool isRebuilt = rdbStore_->GetRebuilt(rebuilt) == NativeRdb::E_OK && rebuilt == NativeRdb::RebuiltType::REBUILT;
    bool isInitVersion = rdbStore_->GetVersion(version) == NativeRdb::E_OK && version == 0;
    if (isRebuilt || isInitVersion) {
        MEDIA_INFO_LOG("MediaLibraryRdbStore::Init, OnCreate, isRebuilt: %{public}d isInitVersion: %{public}d",
            static_cast<uint32_t>(isRebuilt), static_cast<uint32_t>(isInitVersion));
        rdbDataCallBack.OnCreate(*rdbStore_);
    }
    MEDIA_INFO_LOG("MediaLibraryRdbStore::Init(), SUCCESS");
    return E_OK;
}

void MediaLibraryRdbStore::AddPhotoDateAddedIndex(const shared_ptr<MediaLibraryRdbStore> store)
{
    MEDIA_INFO_LOG("start AddPhotoDateAddedIndex");
    const vector<string> sqls = {
        PhotoUpgrade::INDEX_SCTHP_PHOTO_DATEADDED,
    };
    ExecSqls(sqls, *store->GetRaw().get());
    MEDIA_INFO_LOG("end AddPhotoDateAddedIndex");
}

void MediaLibraryRdbStore::AddPhotoWhiteBlocksIndex(const shared_ptr<MediaLibraryRdbStore> store)
{
    MEDIA_INFO_LOG("start AddPhotoWhiteBlocksIndex");
    const vector<string> sqls = {
        PhotoUpgrade::INDEX_QUERY_THUMBNAIL_WHITE_BLOCKS,
    };
    ExecSqls(sqls, *store->GetRaw().get());
    MEDIA_INFO_LOG("end AddPhotoWhiteBlocksIndex");
}

void MediaLibraryRdbStore::AddPhotoSortIndex(const std::shared_ptr<MediaLibraryRdbStore> store)
{
    MEDIA_INFO_LOG("start AddPhotoSortIndex");
    const vector<string> sqls = {
        PhotoUpgrade::CREATE_PHOTO_SORT_MEDIA_TYPE_DATE_ADDED_INDEX,
        PhotoUpgrade::CREATE_PHOTO_SORT_MEDIA_TYPE_DATE_TAKEN_INDEX,
        PhotoUpgrade::CREATE_PHOTO_SORT_IN_ALBUM_DATE_ADDED_INDEX,
        PhotoUpgrade::CREATE_PHOTO_SORT_IN_ALBUM_DATE_TAKEN_INDEX,
    };
    ExecSqls(sqls, *store->GetRaw().get());
    MEDIA_INFO_LOG("End AddPhotoSortIndex");
}

void MediaLibraryRdbStore::UpdateLatitudeAndLongitudeDefaultNull(const std::shared_ptr<MediaLibraryRdbStore> store)
{
    MEDIA_INFO_LOG("start Update LatitudeAndLongitude Default Null");
    const vector<string> sqls = {
        PhotoUpgrade::INDEX_LATITUDE,
        PhotoUpgrade::INDEX_LONGITUDE,
        PhotoUpgrade::UPDATE_LATITUDE_AND_LONGITUDE_DEFAULT_NULL,
    };
    ExecSqls(sqls, *store->GetRaw().get());
    MEDIA_INFO_LOG("end  Update LatitudeAndLongitude Default Null");
}

void MediaLibraryRdbStore::UpdatePhotoQualityCloned(const std::shared_ptr<MediaLibraryRdbStore> store)
{
    MEDIA_INFO_LOG("start UpdatePhotoQualityCloned");
    const vector<string> sqls = {
        PhotoUpgrade::UPDATE_PHOTO_QUALITY_OF_NULL_PHOTO_ID,
    };
    ExecSqls(sqls, *store->GetRaw().get());
    MEDIA_INFO_LOG("end UpdatePhotoQualityCloned");
}

void MediaLibraryRdbStore::UpdateMdirtyTriggerForTdirty(const std::shared_ptr<MediaLibraryRdbStore> store)
{
    MEDIA_INFO_LOG("start UpdateMdirtyTriggerForTdirty");
    const string dropMdirtyCreateTrigger = "DROP TRIGGER IF EXISTS photos_mdirty_trigger";
    int32_t ret = ExecSqlWithRetry([&]() { return store->ExecuteSql(dropMdirtyCreateTrigger); });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("drop photos_mdirty_trigger fail, ret = %{public}d", ret);
        UpdateFail(__FILE__, __LINE__);
    }

    ret = ExecSqlWithRetry([&]() { return store->ExecuteSql(PhotoUpgrade::CREATE_PHOTOS_MDIRTY_TRIGGER); });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("add photos_mdirty_trigger fail, ret = %{public}d", ret);
        UpdateFail(__FILE__, __LINE__);
    }
    MEDIA_INFO_LOG("end UpdateMdirtyTriggerForTdirty");
}

int32_t MediaLibraryRdbStore::Init(const RdbStoreConfig &config, int version, RdbOpenCallback &openCallback)
{
    MEDIA_INFO_LOG("Init rdb store: [version: %{public}d]", version);
    if (rdbStore_ != nullptr) {
        return E_OK;
    }
    int32_t errCode = 0;
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRdbStore::Init GetRdbStore with config");
    rdbStore_ = RdbHelper::GetRdbStore(config, version, openCallback, errCode);
    tracer.Finish();
    if (rdbStore_ == nullptr) {
        MEDIA_ERR_LOG("GetRdbStore with config is failed");
        return errCode;
    }
    MEDIA_INFO_LOG("MediaLibraryRdbStore::Init with config, SUCCESS");
    return E_OK;
}

MediaLibraryRdbStore::~MediaLibraryRdbStore() = default;

void MediaLibraryRdbStore::Stop()
{
    rdbStore_ = nullptr;
}

bool MediaLibraryRdbStore::CheckRdbStore()
{
    return rdbStore_ != nullptr;
}

shared_ptr<NativeRdb::RdbStore> MediaLibraryRdbStore::GetRaw()
{
    return rdbStore_;
}

static void AddDefaultPhotoValues(ValuesBucket& values)
{
    ValueObject tmpValue;
    if (values.GetObject(MediaColumn::MEDIA_NAME, tmpValue)) {
        string newDisplayName {};
        tmpValue.GetString(newDisplayName);
        values.PutString(PhotoColumn::PHOTO_MEDIA_SUFFIX, ScannerUtils::GetFileExtension(newDisplayName));
    }
}

int32_t MediaLibraryRdbStore::Insert(MediaLibraryCommand &cmd, int64_t &rowId)
{
    DfxTimer dfxTimer(DfxType::RDB_INSERT, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRdbStore::Insert");
    if (!MediaLibraryRdbStore::CheckRdbStore()) {
        MEDIA_ERR_LOG("Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
        return E_HAS_DB_ERROR;
    }
    if (cmd.GetTableName() == PhotoColumn::PHOTOS_TABLE) {
        AddDefaultPhotoValues(cmd.GetValueBucket());
    }

    int32_t ret = ExecSqlWithRetry([&]() {
        return MediaLibraryRdbStore::GetRaw()->Insert(rowId, cmd.GetTableName(), cmd.GetValueBucket());
    });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdbStore_->Insert failed, ret = %{public}d", ret);
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
        return E_HAS_DB_ERROR;
    }

    MEDIA_DEBUG_LOG("rdbStore_->Insert end, rowId = %d, ret = %{public}d", (int)rowId, ret);
    return ret;
}

int32_t MediaLibraryRdbStore::BatchInsert(int64_t &outRowId, const std::string &table,
    std::vector<NativeRdb::ValuesBucket> &values)
{
    DfxTimer dfxTimer(DfxType::RDB_INSERT, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRdbStore::BatchInsert");
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    if (table == PhotoColumn::PHOTOS_TABLE) {
        for (auto& value : values) {
            AddDefaultPhotoValues(value);
        }
    }
    int32_t ret = ExecSqlWithRetry([&]() {
        return MediaLibraryRdbStore::GetRaw()->BatchInsert(outRowId, table, values);
    });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdbStore_->BatchInsert failed, ret = %{public}d", ret);
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
        return E_HAS_DB_ERROR;
    }

    MEDIA_DEBUG_LOG("rdbStore_->BatchInsert end, rowId = %d, ret = %{public}d", (int)outRowId, ret);
    return ret;
}

int32_t MediaLibraryRdbStore::BatchInsert(MediaLibraryCommand &cmd, int64_t& outInsertNum,
    std::vector<ValuesBucket>& values)
{
    DfxTimer dfxTimer(DfxType::RDB_BATCHINSERT, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRdbStore::BatchInsert");
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    if (cmd.GetTableName() == PhotoColumn::PHOTOS_TABLE) {
        for (auto& value : values) {
            AddDefaultPhotoValues(value);
        }
    }
    int32_t ret = ExecSqlWithRetry([&]() {
        return MediaLibraryRdbStore::GetRaw()->BatchInsert(outInsertNum, cmd.GetTableName(), values);
    });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdbStore_->BatchInsert failed, ret = %{public}d", ret);
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
        return E_HAS_DB_ERROR;
    }
    tracer.Finish();
    MEDIA_DEBUG_LOG("rdbStore_->BatchInsert end, rowId = %d, ret = %{public}d", (int)outInsertNum, ret);
    return ret;
}

int32_t MediaLibraryRdbStore::InsertInternal(int64_t &outRowId, const std::string &table,
    NativeRdb::ValuesBucket &row)
{
    return ExecSqlWithRetry([&]() { return MediaLibraryRdbStore::GetRaw()->Insert(outRowId, table, row); });
}

int32_t MediaLibraryRdbStore::DoDeleteFromPredicates(const AbsRdbPredicates &predicates, int32_t &deletedRows)
{
    DfxTimer dfxTimer(DfxType::RDB_DELETE, INVALID_DFX, RDB_TIME_OUT, false);
    if (!MediaLibraryRdbStore::CheckRdbStore()) {
        MEDIA_ERR_LOG("Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
        return E_HAS_DB_ERROR;
    }
    int32_t ret = NativeRdb::E_ERROR;
    string tableName = predicates.GetTableName();
    ValuesBucket valuesBucket;
    if (tableName == MEDIALIBRARY_TABLE || tableName == PhotoColumn::PHOTOS_TABLE) {
        valuesBucket.PutInt(MEDIA_DATA_DB_DIRTY, static_cast<int32_t>(DirtyType::TYPE_DELETED));
        valuesBucket.PutInt(MEDIA_DATA_DB_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_UPLOAD));
        valuesBucket.PutLong(PhotoColumn::PHOTO_META_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
        ret = ExecSqlWithRetry([&]() {
            return MediaLibraryRdbStore::GetRaw()->Update(deletedRows, tableName, valuesBucket,
                predicates.GetWhereClause(), predicates.GetWhereArgs());
        });
        MEDIA_INFO_LOG("delete photos permanently, ret: %{public}d", ret);
    } else if (tableName == PhotoAlbumColumns::TABLE) {
        valuesBucket.PutInt(PhotoAlbumColumns::ALBUM_DIRTY, static_cast<int32_t>(DirtyType::TYPE_DELETED));
        ret = ExecSqlWithRetry([&]() {
            return MediaLibraryRdbStore::GetRaw()->Update(deletedRows, tableName, valuesBucket,
                predicates.GetWhereClause(), predicates.GetWhereArgs());
        });
    } else if (tableName == PhotoMap::TABLE) {
        valuesBucket.PutInt(PhotoMap::DIRTY, static_cast<int32_t>(DirtyType::TYPE_DELETED));
        ret = ExecSqlWithRetry([&]() {
            return MediaLibraryRdbStore::GetRaw()->Update(deletedRows, tableName, valuesBucket,
                predicates.GetWhereClause(), predicates.GetWhereArgs());
        });
    } else {
        ret = ExecSqlWithRetry([&]() {
            return MediaLibraryRdbStore::GetRaw()->Delete(deletedRows, tableName, predicates.GetWhereClause(),
                predicates.GetWhereArgs());
        });
    }
    bool isValid = (tableName == PhotoColumn::PHOTOS_TABLE) || (tableName == PhotoAlbumColumns::TABLE);
    isValid = isValid && (deletedRows > 0);
    CHECK_AND_EXECUTE(!isValid, CloudSyncHelper::GetInstance()->StartSync());
    return ret;
}

int32_t MediaLibraryRdbStore::Delete(MediaLibraryCommand &cmd, int32_t &deletedRows)
{
    MediaLibraryTracer tracer;
    tracer.Start("RdbStore->DeleteByCmd");
    /* local delete */
    int32_t ret = DoDeleteFromPredicates(*(cmd.GetAbsRdbPredicates()), deletedRows);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdbStore_->Delete failed, ret = %{public}d", ret);
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
        return E_HAS_DB_ERROR;
    }
    return ret;
}

int32_t MediaLibraryRdbStore::Update(MediaLibraryCommand &cmd, int32_t &changedRows)
{
    if (!MediaLibraryRdbStore::CheckRdbStore()) {
        MEDIA_ERR_LOG("rdbStore_ is nullptr");
        return E_HAS_DB_ERROR;
    }

    if (cmd.GetTableName() == PhotoColumn::PHOTOS_TABLE) {
        cmd.GetValueBucket().PutLong(PhotoColumn::PHOTO_META_DATE_MODIFIED,
            MediaFileUtils::UTCTimeMilliSeconds());
        cmd.GetValueBucket().PutLong(PhotoColumn::PHOTO_LAST_VISIT_TIME,
            MediaFileUtils::UTCTimeMilliSeconds());
    }

    DfxTimer dfxTimer(DfxType::RDB_UPDATE_BY_CMD, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("RdbStore->UpdateByCmd");
    int32_t ret = ExecSqlWithRetry([&]() {
        return MediaLibraryRdbStore::GetRaw()->Update(changedRows, cmd.GetTableName(), cmd.GetValueBucket(),
            cmd.GetAbsRdbPredicates()->GetWhereClause(), cmd.GetAbsRdbPredicates()->GetWhereArgs());
    });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdbStore_->Update failed, ret = %{public}d", ret);
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
        return E_HAS_DB_ERROR;
    }
    return ret;
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryRdbStore::GetIndexOfUri(const AbsRdbPredicates &predicates,
    const vector<string> &columns, const string &id)
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), nullptr, "rdbStore_ is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("GetIndexOfUri");
    string sql;
    sql.append("SELECT ").append(PHOTO_INDEX).append(" From (");
    sql.append(RdbSqlUtils::BuildQueryString(predicates, columns));
    sql.append(") where "+ MediaColumn::MEDIA_ID + " = ").append(id);
    MEDIA_DEBUG_LOG("sql = %{private}s", sql.c_str());
    const vector<string> &args = predicates.GetWhereArgs();
    for (const auto &arg : args) {
        MEDIA_DEBUG_LOG("arg = %{private}s", arg.c_str());
    }
    auto resultSet = MediaLibraryRdbStore::GetRaw()->QuerySql(sql, args);
    MediaLibraryRestore::GetInstance().CheckResultSet(resultSet);
    return resultSet;
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryRdbStore::QueryEditDataExists(
    const NativeRdb::AbsRdbPredicates &predicates)
{
    vector<string> columns = { MediaColumn::MEDIA_FILE_PATH };
    shared_ptr<NativeRdb::ResultSet> resultSet = Query(predicates, columns);
    bool cond = (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK);
    CHECK_AND_RETURN_RET_LOG(!cond, nullptr, "query edit data err");

    CHECK_AND_RETURN_RET_LOG(CheckRdbStore(), nullptr, "rdbStore_ is nullptr. Maybe it didn't init successfully.");

    string photoPath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);

    cond = MediaFileUtils::IsFileExists(PhotoFileUtils::GetEditDataPath(photoPath)) ||
        MediaFileUtils::IsFileExists(PhotoFileUtils::GetEditDataCameraPath(photoPath));
    CHECK_AND_RETURN_RET(!cond,
        MediaLibraryRdbStore::GetRaw()->QuerySql("SELECT 1 AS hasEditData"));
    return MediaLibraryRdbStore::GetRaw()->QuerySql("SELECT 0 AS hasEditData");
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryRdbStore::QueryMovingPhotoVideoReady(
    const NativeRdb::AbsRdbPredicates &predicates)
{
    vector<string> columns = { MediaColumn::MEDIA_FILE_PATH };
    shared_ptr<NativeRdb::ResultSet> resultSet = Query(predicates, columns);
    bool cond = (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK);
    CHECK_AND_RETURN_RET_LOG(!cond, resultSet, "query moving photo video ready err");

    string photoPath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    size_t fileSize;
    size_t oriFileSize;
    auto videoPath = MediaFileUtils::GetMovingPhotoVideoPath(photoPath);
    auto oriVideoPath = MediaFileUtils::GetOriMovingPhotoVideoPath(photoPath);
    cond = MediaFileUtils::GetFileSize(videoPath, fileSize) && (fileSize > 0) &&
        MediaFileUtils::GetFileSize(oriVideoPath, oriFileSize) && (oriFileSize > 0);
    MEDIA_DEBUG_LOG("photoPath:%{public}s, videoPath:%{public}s, video size:%zu",
        DfxUtils::GetSafePath(photoPath).c_str(), DfxUtils::GetSafePath(videoPath).c_str(), fileSize);
    CHECK_AND_RETURN_RET(!cond, MediaLibraryRdbStore::GetRaw()->QuerySql("SELECT 1 AS movingPhotoVideoReady"));
    return MediaLibraryRdbStore::GetRaw()->QuerySql("SELECT 0 AS movingPhotoVideoReady");
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryRdbStore::GetIndexOfUriForPhotos(const AbsRdbPredicates &predicates,
    const vector<string> &columns, const string &id)
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), nullptr, "rdbStore_ is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("GetIndexOfUriForPhotos");
    string sql;
    sql.append(RdbSqlUtils::BuildQueryString(predicates, columns));
    MEDIA_DEBUG_LOG("sql = %{private}s", sql.c_str());
    const vector<string> &args = predicates.GetWhereArgs();
    for (const auto &arg : args) {
        MEDIA_DEBUG_LOG("arg = %{private}s", arg.c_str());
    }
    auto resultSet = MediaLibraryRdbStore::GetRaw()->QuerySql(sql, args);
    MediaLibraryRestore::GetInstance().CheckResultSet(resultSet);
    return resultSet;
}

int32_t MediaLibraryRdbStore::UpdateLastVisitTime(const string &id)
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR, "rdbStore_ is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("UpdateLastVisitTime");
    ValuesBucket values;
    int32_t changedRows = 0;
    values.PutLong(PhotoColumn::PHOTO_LAST_VISIT_TIME, MediaFileUtils::UTCTimeMilliSeconds());
    string whereClause = MediaColumn::MEDIA_ID + " = ?";
    vector<string> whereArgs = {id};
    int32_t ret = ExecSqlWithRetry([&]() {
        return MediaLibraryRdbStore::GetRaw()->Update(changedRows, PhotoColumn::PHOTOS_TABLE, values, whereClause,
            whereArgs);
    });
    if (ret != NativeRdb::E_OK || changedRows <= 0) {
        MEDIA_ERR_LOG("rdbStore_->UpdateLastVisitTime failed, changedRows = %{public}d, ret = %{public}d",
            changedRows, ret);
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
    }
    return changedRows;
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryRdbStore::Query(MediaLibraryCommand &cmd,
    const vector<string> &columns)
{
    if (!MediaLibraryRdbStore::CheckRdbStore()) {
        MEDIA_ERR_LOG("rdbStore_ is nullptr");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::QUERY}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return nullptr;
    }

    MediaLibraryTracer tracer;
    tracer.Start("RdbStore->QueryByCmd");
#ifdef MEDIALIBRARY_COMPATIBILITY
    auto predicates = cmd.GetAbsRdbPredicates();
    MEDIA_DEBUG_LOG("tablename = %{private}s", predicates->GetTableName().c_str());
    for (const auto &col : columns) {
        MEDIA_DEBUG_LOG("col = %{private}s", col.c_str());
    }
    MEDIA_DEBUG_LOG("whereClause = %{private}s", predicates->GetWhereClause().c_str());
    const vector<string> &args = predicates->GetWhereArgs();
    for (const auto &arg : args) {
        MEDIA_DEBUG_LOG("whereArgs = %{private}s", arg.c_str());
    }
    MEDIA_DEBUG_LOG("limit = %{public}d", predicates->GetLimit());
#endif

    /*
     * adapter pattern:
     * Reuse predicates-based query so that no need to modify both func
     * if later logic changes take place
     */
    auto resultSet = QueryWithFilter(*cmd.GetAbsRdbPredicates(), columns);
    if (resultSet == nullptr) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::QUERY}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
    }
    return resultSet;
}

static void PrintPredicatesInfo(const AbsRdbPredicates& predicates, const vector<string>& columns)
{
    string argsInfo;
    for (const auto& arg : predicates.GetWhereArgs()) {
        if (!argsInfo.empty()) {
            argsInfo += ", ";
        }
        argsInfo += arg;
    }
    MEDIA_DEBUG_LOG("Predicates Statement is %{private}s", RdbSqlUtils::BuildQueryString(predicates, columns).c_str());
    MEDIA_DEBUG_LOG("PhotosApp Predicates Args are %{private}s", argsInfo.c_str());
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryRdbStore::QueryWithFilter(const AbsRdbPredicates &predicates,
    const vector<string> &columns)
{
    if (!MediaLibraryRdbStore::CheckRdbStore()) {
        MEDIA_ERR_LOG("rdbStore_ is nullptr");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::QUERY}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return nullptr;
    }

    /* add filter */
    MediaLibraryRdbUtils::AddQueryFilter(const_cast<AbsRdbPredicates &>(predicates));
    DfxTimer dfxTimer(RDB_QUERY, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("RdbStore->QueryByPredicates");
    PrintPredicatesInfo(predicates, columns);
    auto resultSet = MediaLibraryRdbStore::GetRaw()->QueryByStep(predicates, columns);
    MediaLibraryRestore::GetInstance().CheckResultSet(resultSet);
    if (resultSet == nullptr) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::QUERY}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
    }
    return resultSet;
}

int32_t MediaLibraryRdbStore::ExecuteSql(const string &sql)
{
    if (!MediaLibraryRdbStore::CheckRdbStore()) {
        MEDIA_ERR_LOG("Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
        return E_HAS_DB_ERROR;
    }
    DfxTimer dfxTimer(RDB_EXECUTE_SQL, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("RdbStore->ExecuteSql");
    int32_t ret = ExecSqlWithRetry([&]() { return MediaLibraryRdbStore::GetRaw()->ExecuteSql(sql); });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdbStore_->ExecuteSql failed, ret = %{public}d", ret);
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
        return E_HAS_DB_ERROR;
    }
    return ret;
}

int32_t MediaLibraryRdbStore::QueryPragma(const string &key, int64_t &value)
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    std::shared_ptr<ResultSet> resultSet = MediaLibraryRdbStore::GetRaw()->QuerySql("PRAGMA " + key);
    MediaLibraryRestore::GetInstance().CheckResultSet(resultSet);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdbStore_->QuerySql failed");
        return E_HAS_DB_ERROR;
    }
    resultSet->GetLong(0, value);
    resultSet->Close();
    return E_OK;
}

void MediaLibraryRdbStore::BuildValuesSql(const NativeRdb::ValuesBucket &values, vector<ValueObject> &bindArgs,
    string &sql)
{
    map<string, ValueObject> valuesMap;
    values.GetAll(valuesMap);
    sql.append("(");
    for (auto iter = valuesMap.begin(); iter != valuesMap.end(); iter++) {
        sql.append(((iter == valuesMap.begin()) ? "" : ", "));
        sql.append(iter->first);               // columnName
        bindArgs.push_back(iter->second); // columnValue
    }

    sql.append(") select ");
    for (size_t i = 0; i < valuesMap.size(); i++) {
        sql.append(((i == 0) ? "?" : ", ?"));
    }
    sql.append(" ");
}

void MediaLibraryRdbStore::BuildQuerySql(const AbsRdbPredicates &predicates, const vector<string> &columns,
    vector<ValueObject> &bindArgs, string &sql)
{
    sql.append(RdbSqlUtils::BuildQueryString(predicates, columns));
    const vector<string> &args = predicates.GetWhereArgs();
    for (const auto &arg : args) {
        bindArgs.emplace_back(arg);
    }
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryRdbStore::StepQueryWithoutCheck(const AbsRdbPredicates &predicates,
    const vector<string> &columns)
{
    if (!MediaLibraryRdbStore::CheckRdbStore()) {
        MEDIA_ERR_LOG("rdbStore_ is nullptr");
        VariantMap map = { { KEY_ERR_FILE, __FILE__ },
            { KEY_ERR_LINE, __LINE__ },
            { KEY_ERR_CODE, E_HAS_DB_ERROR },
            { KEY_OPT_TYPE, OptType::QUERY } };
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return nullptr;
    }

    MediaLibraryRdbUtils::AddQueryFilter(const_cast<AbsRdbPredicates &>(predicates));
    DfxTimer dfxTimer(RDB_QUERY, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("StepQueryWithoutCheck");
    MEDIA_DEBUG_LOG("Predicates Statement is %{public}s", predicates.GetStatement().c_str());
    auto resultSet = MediaLibraryRdbStore::GetRaw()->QueryByStep(predicates, columns, false);
    if (resultSet == nullptr) {
        VariantMap map = { { KEY_ERR_FILE, __FILE__ },
            { KEY_ERR_LINE, __LINE__ },
            { KEY_ERR_CODE, E_HAS_DB_ERROR },
            { KEY_OPT_TYPE, OptType::QUERY } };
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
    }
    return resultSet;
}

/**
 * Returns last insert row id. If insert succeed but no new rows inserted, then return -1.
 * Return E_HAS_DB_ERROR on error cases.
 */
int32_t MediaLibraryRdbStore::ExecuteForLastInsertedRowId(const string &sql, const vector<ValueObject> &bindArgs)
{
    if (!MediaLibraryRdbStore::CheckRdbStore()) {
        MEDIA_ERR_LOG("Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
        return E_HAS_DB_ERROR;
    }

    int64_t lastInsertRowId = 0;
    int32_t err = ExecSqlWithRetry(
        [&]() { return MediaLibraryRdbStore::GetRaw()->ExecuteForLastInsertedRowId(lastInsertRowId, sql, bindArgs); });
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to execute insert, err: %{public}d", err);
        MediaLibraryRestore::GetInstance().CheckRestore(err);
        return E_HAS_DB_ERROR;
    }
    return lastInsertRowId;
}

int32_t MediaLibraryRdbStore::Delete(const AbsRdbPredicates &predicates)
{
    int err = E_ERR;
    int32_t deletedRows = 0;
    err = DoDeleteFromPredicates(predicates, deletedRows);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to execute delete, err: %{public}d", err);
        MediaLibraryRestore::GetInstance().CheckRestore(err);
        return E_HAS_DB_ERROR;
    }
    return deletedRows;
}

/**
 * Return changed rows on success, or negative values on error cases.
 */
int32_t MediaLibraryRdbStore::UpdateWithDateTime(ValuesBucket &values,
    const AbsRdbPredicates &predicates)
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");

    if (predicates.GetTableName() == PhotoColumn::PHOTOS_TABLE) {
        values.PutLong(PhotoColumn::PHOTO_META_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
        values.PutLong(PhotoColumn::PHOTO_LAST_VISIT_TIME, MediaFileUtils::UTCTimeMilliSeconds());
    }

    DfxTimer dfxTimer(DfxType::RDB_UPDATE, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRdbStore::Update by predicates");
    int32_t changedRows = -1;
    int32_t err =
        ExecSqlWithRetry([&]() { return MediaLibraryRdbStore::GetRaw()->Update(changedRows, values, predicates); });
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to execute update, err: %{public}d", err);
        MediaLibraryRestore::GetInstance().CheckRestore(err);
        return E_HAS_DB_ERROR;
    }

    return changedRows;
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryRdbStore::QuerySql(const string &sql, const vector<string> &selectionArgs)
{
    if (!MediaLibraryRdbStore::CheckRdbStore()) {
        MEDIA_ERR_LOG("Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::QUERY}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return nullptr;
    }

    MediaLibraryTracer tracer;
    tracer.Start("RdbStore->QuerySql");
    auto resultSet = MediaLibraryRdbStore::GetRaw()->QuerySql(sql, selectionArgs);
    MediaLibraryRestore::GetInstance().CheckResultSet(resultSet);
    if (resultSet == nullptr) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::QUERY}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
    }

    return resultSet;
}

void MediaLibraryRdbStore::ReplacePredicatesUriToId(AbsRdbPredicates &predicates)
{
    const vector<string> &whereUriArgs = predicates.GetWhereArgs();
    vector<string> whereIdArgs;
    whereIdArgs.reserve(whereUriArgs.size());
    for (const auto &arg : whereUriArgs) {
        if (!MediaFileUtils::StartsWith(arg, PhotoColumn::PHOTO_URI_PREFIX)) {
            whereIdArgs.push_back(arg);
            continue;
        }
        whereIdArgs.push_back(MediaFileUri::GetPhotoId(arg));
    }

    predicates.SetWhereArgs(whereIdArgs);
}

int32_t MediaLibraryRdbStore::GetInt(const shared_ptr<ResultSet> &resultSet, const string &column)
{
    return get<int32_t>(ResultSetUtils::GetValFromColumn(column, resultSet, TYPE_INT32));
}

string MediaLibraryRdbStore::GetString(const shared_ptr<ResultSet> &resultSet, const string &column)
{
    return get<string>(ResultSetUtils::GetValFromColumn(column, resultSet, TYPE_STRING));
}

inline void BuildInsertSystemAlbumSql(const ValuesBucket &values, const AbsRdbPredicates &predicates,
    string &sql, vector<ValueObject> &bindArgs)
{
    // Build insert sql
    sql.append("INSERT").append(" OR ROLLBACK ").append(" INTO ").append(PhotoAlbumColumns::TABLE).append(" ");
    MediaLibraryRdbStore::BuildValuesSql(values, bindArgs, sql);
    sql.append(" WHERE NOT EXISTS (");
    MediaLibraryRdbStore::BuildQuerySql(predicates, { PhotoAlbumColumns::ALBUM_ID }, bindArgs, sql);
    sql.append(");");
}

int32_t PrepareAlbumPlugin(RdbStore &store)
{
    AlbumPluginTableEventHandler albumPluginTableEventHander;
    int32_t ret = albumPluginTableEventHander.OnCreate(store);
    // after initiate album_plugin table, add 2 default album into PhotoAlbum.
    ExecSqlWithRetry([&]() {
        return store.ExecuteSql(CREATE_DEFALUT_ALBUM_FOR_NO_RELATIONSHIP_ASSET);
    });
    ExecSqlWithRetry([&]() {
        return store.ExecuteSql(CREATE_HIDDEN_ALBUM_FOR_DUAL_ASSET);
    });
    return ret;
}

int32_t PrepareSystemAlbums(RdbStore &store)
{
    ValuesBucket values;
    int32_t err = E_FAIL;
    MEDIA_INFO_LOG("PrepareSystemAlbums start");
    auto [errCode, transaction] = store.CreateTransaction(OHOS::NativeRdb::Transaction::DEFERRED);
    DfxTransaction reporter{ __func__ };
    if (errCode != NativeRdb::E_OK || transaction == nullptr) {
        reporter.ReportError(DfxTransaction::AbnormalType::CREATE_ERROR, errCode);
        MEDIA_ERR_LOG("transaction failed, err:%{public}d", errCode);
        return errCode;
    }
    for (int32_t i = PhotoAlbumSubType::SYSTEM_START; i <= PhotoAlbumSubType::SYSTEM_END; i++) {
        values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumType::SYSTEM);
        values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, i);
        values.PutInt(PhotoAlbumColumns::ALBUM_ORDER, i - PhotoAlbumSubType::SYSTEM_START);

        AbsRdbPredicates predicates(PhotoAlbumColumns::TABLE);
        predicates.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(PhotoAlbumType::SYSTEM));
        predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(i));

        string sql;
        vector<ValueObject> bindArgs;
        BuildInsertSystemAlbumSql(values, predicates, sql, bindArgs);
        auto res = transaction->Execute(sql, bindArgs);
        err = res.first;
        if (err != E_OK) {
            reporter.ReportError(DfxTransaction::AbnormalType::EXECUTE_ERROR, err);
            transaction->Rollback();
            MEDIA_ERR_LOG("Execute sql failed, err: %{public}d", err);
            return err;
        }
        values.Clear();
    }
    err = transaction->Commit();
    if (err != NativeRdb::E_OK) {
        reporter.ReportError(DfxTransaction::AbnormalType::COMMIT_ERROR, err);
        MEDIA_ERR_LOG("transaction Commit failed, err: %{public}d", err);
    } else {
        reporter.ReportIfTimeout();
    }
    return E_OK;
}

int32_t MediaLibraryDataCallBack::PrepareDir(RdbStore &store)
{
    DirValuesBucket cameraDir = {
        CAMERA_DIRECTORY_TYPE_VALUES, CAMERA_DIR_VALUES, CAMERA_TYPE_VALUES, CAMERA_EXTENSION_VALUES
    };
    DirValuesBucket videoDir = {
        VIDEO_DIRECTORY_TYPE_VALUES, VIDEO_DIR_VALUES, VIDEO_TYPE_VALUES, VIDEO_EXTENSION_VALUES
    };
    DirValuesBucket pictureDir = {
        PIC_DIRECTORY_TYPE_VALUES, PIC_DIR_VALUES, PIC_TYPE_VALUES, PIC_EXTENSION_VALUES
    };
    DirValuesBucket audioDir = {
        AUDIO_DIRECTORY_TYPE_VALUES, AUDIO_DIR_VALUES, AUDIO_TYPE_VALUES, AUDIO_EXTENSION_VALUES
    };
    DirValuesBucket documentDir = {
        DOC_DIRECTORY_TYPE_VALUES, DOCS_PATH, DOC_TYPE_VALUES, DOC_EXTENSION_VALUES
    };
    DirValuesBucket downloadDir = {
        DOWNLOAD_DIRECTORY_TYPE_VALUES, DOCS_PATH, DOWNLOAD_TYPE_VALUES, DOWNLOAD_EXTENSION_VALUES
    };

    vector<DirValuesBucket> dirValuesBuckets = {
        cameraDir, videoDir, pictureDir, audioDir, documentDir, downloadDir
    };

    for (const auto& dirValuesBucket : dirValuesBuckets) {
        int32_t insertResult = InsertDirValues(dirValuesBucket, store);
        CHECK_AND_PRINT_LOG(insertResult == NativeRdb::E_OK,
            "insert failed, insertResult: %{public}d", insertResult);
    }
    return NativeRdb::E_OK;
}

int32_t MediaLibraryDataCallBack::InsertDirValues(const DirValuesBucket &dirValuesBucket, RdbStore &store)
{
    ValuesBucket valuesBucket;
    valuesBucket.PutInt(DIRECTORY_DB_DIRECTORY_TYPE, dirValuesBucket.directoryType);
    valuesBucket.PutString(DIRECTORY_DB_DIRECTORY, dirValuesBucket.dirValues);
    valuesBucket.PutString(DIRECTORY_DB_MEDIA_TYPE, dirValuesBucket.typeValues);
    valuesBucket.PutString(DIRECTORY_DB_EXTENSION, dirValuesBucket.extensionValues);
    int64_t outRowId = -1;
    int32_t insertResult = ExecSqlWithRetry([&]() {
        return store.InsertWithConflictResolution(outRowId, MEDIATYPE_DIRECTORY_TABLE, valuesBucket,
            ConflictResolution::ON_CONFLICT_REPLACE);
    });
    MEDIA_DEBUG_LOG("insert dir outRowId: %{public}ld insertResult: %{public}d", (long)outRowId, insertResult);
    return insertResult;
}

static void UpdateAlbumPlugin(RdbStore &store, int32_t version)
{
    MEDIA_INFO_LOG("Start updating album plugin");
    const vector<string> sqls = {
        "DROP TABLE IF EXISTS album_plugin;"
    };
    ExecSqlsWithDfx(sqls, store, version);
    AlbumPluginTableEventHandler().OnCreate(store);
    MEDIA_INFO_LOG("End updating album plugin");
}

int32_t MediaLibraryDataCallBack::PrepareSmartAlbum(RdbStore &store)
{
    SmartAlbumValuesBucket trashAlbum = {
        TRASH_ALBUM_ID_VALUES, TRASH_ALBUM_NAME_VALUES, TRASH_ALBUM_TYPE_VALUES
    };

    SmartAlbumValuesBucket favAlbum = {
        FAVOURITE_ALBUM_ID_VALUES, FAVOURTIE_ALBUM_NAME_VALUES, FAVOURITE_ALBUM_TYPE_VALUES
    };

    vector<SmartAlbumValuesBucket> smartAlbumValuesBuckets = {
        trashAlbum, favAlbum
    };

    for (const auto& smartAlbum : smartAlbumValuesBuckets) {
        if (InsertSmartAlbumValues(smartAlbum, store) != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Prepare smartAlbum failed");
            return NativeRdb::E_ERROR;
        }
    }
    return NativeRdb::E_OK;
}

static int32_t InsertShootingModeAlbumValues(
    const string& albumName, RdbStore &store)
{
    ValuesBucket valuesBucket;
    valuesBucket.PutInt(SMARTALBUM_DB_ALBUM_TYPE, SHOOTING_MODE_TYPE);
    valuesBucket.PutInt(COMPAT_ALBUM_SUBTYPE, SHOOTING_MODE_SUB_TYPE);
    valuesBucket.PutString(MEDIA_DATA_DB_ALBUM_NAME, albumName);
    valuesBucket.PutInt(MEDIA_DATA_DB_COUNT, 0);
    valuesBucket.PutInt(MEDIA_DATA_DB_IS_LOCAL, 1); // local album is 1.
    int64_t outRowId = -1;
    int32_t insertResult = ExecSqlWithRetry([&]() {
        return store.InsertWithConflictResolution(outRowId, ANALYSIS_ALBUM_TABLE, valuesBucket,
            ConflictResolution::ON_CONFLICT_REPLACE);
    });
    return insertResult;
}

static int32_t QueryExistingShootingModeAlbumNames(RdbStore& store, vector<string>& existingAlbumNames)
{
    string queryRowSql = "SELECT album_name FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE album_subtype = " + to_string(PhotoAlbumSubType::SHOOTING_MODE);
    auto resultSet = store.QuerySql(queryRowSql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_FAIL,
        "Can not get shootingMode album names, resultSet is nullptr");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        string albumName = GetStringVal("album_name", resultSet);
        if (!albumName.empty()) {
            existingAlbumNames.push_back(albumName);
        }
    }
    return E_SUCCESS;
}

static int32_t PrepareShootingModeAlbum(RdbStore &store)
{
    vector<string> existingAlbumNames;
    if (QueryExistingShootingModeAlbumNames(store, existingAlbumNames) != E_SUCCESS) {
        MEDIA_ERR_LOG("Query existing shootingMode album names failed");
        return NativeRdb::E_ERROR;
    }
    for (int i = static_cast<int>(ShootingModeAlbumType::START);
        i <= static_cast<int>(ShootingModeAlbumType::END); ++i) {
        string albumName = to_string(i);
        if (find(existingAlbumNames.begin(), existingAlbumNames.end(), albumName) != existingAlbumNames.end()) {
            continue;
        }
        if (InsertShootingModeAlbumValues(albumName, store) != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Prepare shootingMode album failed");
            return NativeRdb::E_ERROR;
        }
    }
    return NativeRdb::E_OK;
}

static int32_t Prepare3DGSModeAlbum(RdbStore &store, int32_t version)
{
    vector<string> existingAlbumNames;
    if (QueryExistingShootingModeAlbumNames(store, existingAlbumNames) != E_SUCCESS) {
        MEDIA_ERR_LOG("Query existing shootingMode album names failed");
        return NativeRdb::E_ERROR;
    }
    string albumName = to_string(static_cast<int>(ShootingModeAlbumType::MP4_3DGS_ALBUM));
    if (find(existingAlbumNames.begin(), existingAlbumNames.end(), albumName) != existingAlbumNames.end()) {
        return NativeRdb::E_OK;
    }
    auto ret = InsertShootingModeAlbumValues(albumName, store);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Prepare shootingMode album failed");
        RdbUpgradeUtils::AddUpgradeDfxMessages(version, 0, ret);
        return NativeRdb::E_ERROR;
    }
    return NativeRdb::E_OK;
}

static int32_t PrepareOtherShootingModeAlbum(RdbStore &store, int32_t version)
{
    vector<string> existingAlbumNames;
    if (QueryExistingShootingModeAlbumNames(store, existingAlbumNames) != E_SUCCESS) {
        MEDIA_ERR_LOG("Query existing shootingMode album names failed");
        return NativeRdb::E_ERROR;
    }
    for (int i = static_cast<int>(ShootingModeAlbumType::START);
        i <= static_cast<int>(ShootingModeAlbumType::END); ++i) {
        string albumName = to_string(i);
        if (find(existingAlbumNames.begin(), existingAlbumNames.end(), albumName) != existingAlbumNames.end()) {
            continue;
        }
        auto ret = InsertShootingModeAlbumValues(albumName, store);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Prepare shootingMode album failed");
            RdbUpgradeUtils::AddUpgradeDfxMessages(version, i, ret);
            return NativeRdb::E_ERROR;
        }
    }
    return NativeRdb::E_OK;
}

int32_t MediaLibraryDataCallBack::InsertSmartAlbumValues(const SmartAlbumValuesBucket &smartAlbum, RdbStore &store)
{
    ValuesBucket valuesBucket;
    valuesBucket.PutInt(SMARTALBUM_DB_ID, smartAlbum.albumId);
    valuesBucket.PutString(SMARTALBUM_DB_NAME, smartAlbum.albumName);
    valuesBucket.PutInt(SMARTALBUM_DB_ALBUM_TYPE, smartAlbum.albumType);
    int64_t outRowId = -1;
    int32_t insertResult = ExecSqlWithRetry([&]() {
        return store.InsertWithConflictResolution(outRowId, SMARTALBUM_TABLE, valuesBucket,
            ConflictResolution::ON_CONFLICT_REPLACE);
    });
    return insertResult;
}

static int32_t InsertUniqueMemberTableValues(const UniqueMemberValuesBucket &uniqueMemberValues,
    RdbStore &store)
{
    ValuesBucket valuesBucket;
    valuesBucket.PutString(ASSET_MEDIA_TYPE, uniqueMemberValues.assetMediaType);
    valuesBucket.PutInt(UNIQUE_NUMBER, uniqueMemberValues.startNumber);
    int64_t outRowId = -1;
    int32_t insertResult = ExecSqlWithRetry([&]() {
        return store.InsertWithConflictResolution(outRowId, ASSET_UNIQUE_NUMBER_TABLE, valuesBucket,
            ConflictResolution::ON_CONFLICT_REPLACE);
    });
    return insertResult;
}

static int32_t PrepareUniqueMemberTable(RdbStore &store)
{
    string queryRowSql = "SELECT COUNT(*) as count FROM " + ASSET_UNIQUE_NUMBER_TABLE;
    auto resultSet = store.QuerySql(queryRowSql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get AssetUniqueNumberTable count");
        UpdateFail(__FILE__, __LINE__);
        return NativeRdb::E_ERROR;
    }
    if (GetInt32Val("count", resultSet) != 0) {
        MEDIA_DEBUG_LOG("AssetUniqueNumberTable is already inited");
        return E_OK;
    }

    UniqueMemberValuesBucket imageBucket = { IMAGE_ASSET_TYPE, 0 };
    UniqueMemberValuesBucket videoBucket = { VIDEO_ASSET_TYPE, 0 };
    UniqueMemberValuesBucket audioBucket = { AUDIO_ASSET_TYPE, 0 };

    vector<UniqueMemberValuesBucket> uniqueNumberValueBuckets = {
        imageBucket, videoBucket, audioBucket
    };

    for (const auto& uniqueNumberValueBucket : uniqueNumberValueBuckets) {
        if (InsertUniqueMemberTableValues(uniqueNumberValueBucket, store) != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Prepare smartAlbum failed");
            UpdateFail(__FILE__, __LINE__);
            return NativeRdb::E_ERROR;
        }
    }
    return NativeRdb::E_OK;
}

static const string &TriggerDeleteAlbumClearMap()
{
    static const string TRIGGER_CLEAR_MAP = BaseColumn::CreateTrigger() + "photo_album_clear_map" +
    " AFTER DELETE ON " + PhotoAlbumColumns::TABLE +
    " BEGIN " +
        "DELETE FROM " + PhotoMap::TABLE +
        " WHERE " + PhotoMap::ALBUM_ID + "=" + "OLD." + PhotoAlbumColumns::ALBUM_ID + ";" +
    " END;";
    return TRIGGER_CLEAR_MAP;
}

static const string &TriggerAddAssets()
{
    static const string TRIGGER_ADD_ASSETS = BaseColumn::CreateTrigger() + "photo_album_insert_asset" +
    " AFTER INSERT ON " + PhotoMap::TABLE +
    " BEGIN " +
        "UPDATE " + PhotoAlbumColumns::TABLE + " SET " +
            PhotoAlbumColumns::ALBUM_COUNT + " = " + PhotoAlbumColumns::ALBUM_COUNT + " + 1 " +
        "WHERE " + PhotoAlbumColumns::ALBUM_ID + " = " + "NEW." + PhotoMap::ALBUM_ID + ";" +
    " END;";
    return TRIGGER_ADD_ASSETS;
}

static const string &TriggerRemoveAssets()
{
    static const string TRIGGER_REMOVE_ASSETS = BaseColumn::CreateTrigger() + "photo_album_delete_asset" +
    " AFTER DELETE ON " + PhotoMap::TABLE +
    " BEGIN " +
        "UPDATE " + PhotoAlbumColumns::TABLE + " SET " +
            PhotoAlbumColumns::ALBUM_COUNT + " = " + PhotoAlbumColumns::ALBUM_COUNT + " - 1 " +
        "WHERE " + PhotoAlbumColumns::ALBUM_ID + " = " + "OLD." + PhotoMap::ALBUM_ID + ";" +
    " END;";
    return TRIGGER_REMOVE_ASSETS;
}

static const string &TriggerDeletePhotoClearMap()
{
    static const string TRIGGER_DELETE_ASSETS = BaseColumn::CreateTrigger() + "delete_photo_clear_map" +
    " AFTER DELETE ON " + PhotoColumn::PHOTOS_TABLE +
    " BEGIN " +
        "DELETE FROM " + PhotoMap::TABLE +
        " WHERE " + PhotoMap::ASSET_ID + "=" + "OLD." + MediaColumn::MEDIA_ID + ";" +
        "DELETE FROM " + ANALYSIS_PHOTO_MAP_TABLE +
        " WHERE " + PhotoMap::ASSET_ID + "=" + "OLD." + MediaColumn::MEDIA_ID + ";" +
    " END;";
    return TRIGGER_DELETE_ASSETS;
}

static const string &QueryAlbumJoinMap()
{
    static const string QUERY_ALBUM_JOIN_MAP = " SELECT " + PhotoAlbumColumns::ALBUM_ID +
        " FROM " + PhotoAlbumColumns::TABLE + " INNER JOIN " + PhotoMap::TABLE + " ON " +
            PhotoAlbumColumns::ALBUM_ID + " = " + PhotoMap::ALBUM_ID + " AND " +
            PhotoMap::ASSET_ID + " = " + "NEW." + MediaColumn::MEDIA_ID;
    return QUERY_ALBUM_JOIN_MAP;
}

static const string &SetHiddenUpdateCount()
{
    // Photos.hidden 1 -> 0
    static const string SET_HIDDEN_UPDATE_COUNT = " UPDATE " + PhotoAlbumColumns::TABLE +
        " SET " + PhotoAlbumColumns::ALBUM_COUNT + " = " + PhotoAlbumColumns::ALBUM_COUNT + " + 1" +
        " WHERE " + PhotoAlbumColumns::ALBUM_ID + " IN (" +
            QueryAlbumJoinMap() + " WHERE " +
                "NEW." + MediaColumn::MEDIA_HIDDEN + " = 0" + " AND " +
                "(OLD." + MediaColumn::MEDIA_HIDDEN + " - NEW." + MediaColumn::MEDIA_HIDDEN + " > 0)" +
        ");";
    return SET_HIDDEN_UPDATE_COUNT;
}

static const string &SetTrashUpdateCount()
{
    // Photos.date_trashed timestamp -> 0
    static const string SET_TRASH_UPDATE_COUNT = " UPDATE " + PhotoAlbumColumns::TABLE +
        " SET " + PhotoAlbumColumns::ALBUM_COUNT + " = " + PhotoAlbumColumns::ALBUM_COUNT + " + 1" +
        " WHERE " + PhotoAlbumColumns::ALBUM_ID + " IN (" +
            QueryAlbumJoinMap() + " WHERE " +
                "SIGN(NEW." + MediaColumn::MEDIA_DATE_TRASHED + ") = 0" + " AND " +
                "NEW." + MediaColumn::MEDIA_HIDDEN + " = 0" + " AND " +
                "(" +
                    "SIGN(OLD." + MediaColumn::MEDIA_DATE_TRASHED + ") - " +
                    "SIGN(NEW." + MediaColumn::MEDIA_DATE_TRASHED + ") > 0" +
                ")" +
        ");";
    return SET_TRASH_UPDATE_COUNT;
}

static const string &UnSetHiddenUpdateCount()
{
    // Photos.hidden 0 -> 1
    static const string UNSET_HIDDEN_UPDATE_COUNT = " UPDATE " + PhotoAlbumColumns::TABLE +
        " SET " + PhotoAlbumColumns::ALBUM_COUNT + " = " + PhotoAlbumColumns::ALBUM_COUNT + " - 1" +
        " WHERE " + PhotoAlbumColumns::ALBUM_ID + " IN (" +
            QueryAlbumJoinMap() + " WHERE " +
                "NEW." + MediaColumn::MEDIA_HIDDEN + " = 1" + " AND " +
                "(NEW." + MediaColumn::MEDIA_HIDDEN + " - OLD." + MediaColumn::MEDIA_HIDDEN + " > 0)" +
        ");";
    return UNSET_HIDDEN_UPDATE_COUNT;
}

static const string &UnSetTrashUpdateCount()
{
    // Photos.date_trashed 0 -> timestamp
    static const string UNSET_TRASH_UPDATE_COUNT = " UPDATE " + PhotoAlbumColumns::TABLE +
        " SET " + PhotoAlbumColumns::ALBUM_COUNT + " = " + PhotoAlbumColumns::ALBUM_COUNT + " - 1" +
        " WHERE " + PhotoAlbumColumns::ALBUM_ID + " IN (" +
            QueryAlbumJoinMap() + " WHERE " +
                "SIGN(NEW." + MediaColumn::MEDIA_DATE_TRASHED + ") = 1" + " AND " +
                "NEW." + MediaColumn::MEDIA_HIDDEN + " = 0" + " AND " +
                "(" +
                    "SIGN(NEW." + MediaColumn::MEDIA_DATE_TRASHED + ") - "
                    "SIGN(OLD." + MediaColumn::MEDIA_DATE_TRASHED + ") > 0" +
                ")" +
        ");";
    return UNSET_TRASH_UPDATE_COUNT;
}

static const string &TriggerUpdateUserAlbumCount()
{
    static const string TRIGGER_UPDATE_USER_ALBUM_COUNT = BaseColumn::CreateTrigger() + "update_user_album_count" +
        " AFTER UPDATE ON " + PhotoColumn::PHOTOS_TABLE +
        " BEGIN " +
            SetHiddenUpdateCount() +
            SetTrashUpdateCount() +
            UnSetHiddenUpdateCount() +
            UnSetTrashUpdateCount() +
        " END;";
    return TRIGGER_UPDATE_USER_ALBUM_COUNT;
}

static const string &TriggerDeletePhotoClearAppUriPermission()
{
    static const string TRIGGER_PHOTO_DELETE_APP_URI_PERMISSION = BaseColumn::CreateTrigger() +
    "delete_photo_clear_App_uri_permission" + " AFTER DELETE ON " + PhotoColumn::PHOTOS_TABLE +
    " BEGIN " +
        "DELETE FROM " + AppUriPermissionColumn::APP_URI_PERMISSION_TABLE +
        " WHERE " + AppUriPermissionColumn::FILE_ID + "=" + "OLD." + MediaColumn::MEDIA_ID +
        " AND " + AppUriPermissionColumn::URI_TYPE + "=" + std::to_string(AppUriPermissionColumn::URI_PHOTO) + ";" +
    " END;";
    return TRIGGER_PHOTO_DELETE_APP_URI_PERMISSION;
}

static const string &TriggerDeleteAudioClearAppUriPermission()
{
    static const string TRIGGER_AUDIO_DELETE_APP_URI_PERMISSION = BaseColumn::CreateTrigger() +
    "delete_audio_clear_App_uri_permission" + " AFTER DELETE ON " + AudioColumn::AUDIOS_TABLE +
    " BEGIN " +
        "DELETE FROM " + AppUriPermissionColumn::APP_URI_PERMISSION_TABLE +
        " WHERE " + AppUriPermissionColumn::FILE_ID + "=" + "OLD." + MediaColumn::MEDIA_ID +
        " AND " + AppUriPermissionColumn::URI_TYPE + "=" + std::to_string(AppUriPermissionColumn::URI_AUDIO) + ";" +
    " END;";
    return TRIGGER_AUDIO_DELETE_APP_URI_PERMISSION;
}

static const vector<string> onCreateSqlStrs = {
    CREATE_MEDIA_TABLE,
    PhotoUpgrade::CREATE_PHOTO_TABLE,
    PhotoUpgrade::CREATE_CLOUD_ID_INDEX,
    PhotoUpgrade::CREATE_PHOTO_SORT_IN_ALBUM_DATE_TAKEN_INDEX,
    PhotoUpgrade::INDEX_SCTHP_PHOTO_DATEADDED,
    PhotoUpgrade::CREATE_PHOTO_SORT_IN_ALBUM_DATE_ADDED_INDEX,
    PhotoUpgrade::CREATE_PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX,
    PhotoUpgrade::CREATE_PHOTO_BURST_MODE_ALBUM_INDEX,
    PhotoUpgrade::CREATE_PHOTO_FRONT_CAMERA_ALBUM_INDEX,
    PhotoUpgrade::CREATE_PHOTO_RAW_IMAGE_ALBUM_INDEX,
    PhotoUpgrade::CREATE_PHOTO_MOVING_PHOTO_ALBUM_INDEX,
    PhotoUpgrade::INDEX_QUERY_THUMBNAIL_WHITE_BLOCKS,
    PhotoUpgrade::INDEX_CAMERA_SHOT_KEY,
    PhotoUpgrade::INDEX_SCHPT_READY,
    PhotoUpgrade::CREATE_SCHPT_MEDIA_TYPE_INDEX,
    PhotoUpgrade::CREATE_SCHPT_DAY_INDEX,
    PhotoUpgrade::CREATE_SCHPT_YEAR_COUNT_READY_INDEX,
    PhotoUpgrade::CREATE_SCHPT_MONTH_COUNT_READY_INDEX,
    PhotoUpgrade::CREATE_SCHPT_HIDDEN_TIME_INDEX,
    PhotoUpgrade::CREATE_PHOTO_FAVORITE_INDEX,
    PhotoUpgrade::CREATE_PHOTOS_DELETE_TRIGGER,
    PhotoUpgrade::CREATE_PHOTOS_FDIRTY_TRIGGER,
    PhotoUpgrade::CREATE_PHOTOS_MDIRTY_TRIGGER,
    PhotoUpgrade::CREATE_PHOTOS_INSERT_CLOUD_SYNC,
    PhotoUpgrade::CREATE_PHOTOS_UPDATE_CLOUD_SYNC,
    PhotoUpgrade::CREATE_PHOTOS_METADATA_DIRTY_TRIGGER,
    AudioColumn::CREATE_AUDIO_TABLE,
    CREATE_SMARTALBUM_TABLE,
    CREATE_SMARTALBUMMAP_TABLE,
    CREATE_DEVICE_TABLE,
    CREATE_CATEGORY_SMARTALBUMMAP_TABLE,
    CREATE_ASSET_UNIQUE_NUMBER_TABLE,
    CREATE_ALBUM_REFRESH_TABLE,
    CREATE_IMAGE_VIEW,
    CREATE_VIDEO_VIEW,
    CREATE_AUDIO_VIEW,
    CREATE_ALBUM_VIEW,
    CREATE_SMARTALBUMASSETS_VIEW,
    CREATE_ASSETMAP_VIEW,
    CREATE_MEDIATYPE_DIRECTORY_TABLE,
    CREATE_BUNDLE_PREMISSION_TABLE,
    CREATE_MEDIALIBRARY_ERROR_TABLE,
    CREATE_REMOTE_THUMBNAIL_TABLE,
    CREATE_FILES_DELETE_TRIGGER,
    CREATE_FILES_MDIRTY_TRIGGER,
    CREATE_FILES_FDIRTY_TRIGGER,
    CREATE_INSERT_CLOUD_SYNC_TRIGGER,
    PhotoAlbumColumns::CREATE_TABLE,
    PhotoAlbumColumns::INDEX_ALBUM_TYPES,
    PhotoAlbumColumns::CREATE_ALBUM_INSERT_TRIGGER,
    PhotoAlbumColumns::CREATE_ALBUM_MDIRTY_TRIGGER,
    PhotoAlbumColumns::CREATE_ALBUM_DELETE_TRIGGER,
    PhotoAlbumColumns::ALBUM_DELETE_ORDER_TRIGGER,
    PhotoAlbumColumns::ALBUM_INSERT_ORDER_TRIGGER,
    PhotoMap::CREATE_TABLE,
    TriggerDeletePhotoClearMap(),
    CREATE_TAB_ANALYSIS_OCR,
    CREATE_TAB_ANALYSIS_AFFECTIVE,
    CREATE_TAB_ANALYSIS_LABEL,
    CREATE_TAB_ANALYSIS_VIDEO_LABEL,
    CREATE_TAB_ANALYSIS_AESTHETICS,
    CREATE_TAB_VIDEO_ANALYSIS_AESTHETICS,
    CREATE_TAB_ANALYSIS_SALIENCY_DETECT,
    CREATE_TAB_ANALYSIS_OBJECT,
    CREATE_TAB_ANALYSIS_RECOMMENDATION,
    CREATE_TAB_ANALYSIS_SEGMENTATION,
    CREATE_TAB_ANALYSIS_COMPOSITION,
    CREATE_TAB_ANALYSIS_HEAD,
    CREATE_TAB_ANALYSIS_POSE,
    CREATE_TAB_ANALYSIS_PET_FACE,
    CREATE_TAB_ANALYSIS_PET_TAG,
    CREATE_TAB_IMAGE_FACE,
    CREATE_TAB_VIDEO_FACE,
    CREATE_TAB_FACE_TAG,
    CREATE_TAB_ANALYSIS_TOTAL_FOR_ONCREATE,
    CREATE_TAB_ANALYSIS_VIDEO_TOTAL,
    CREATE_VISION_UPDATE_TRIGGER,
    CREATE_VISION_DELETE_TRIGGER,
    CREATE_VISION_INSERT_TRIGGER_FOR_ONCREATE,
    CREATE_IMAGE_FACE_INDEX,
    CREATE_IMAGE_FACE_TAG_ID_INDEX,
    CREATE_VIDEO_FACE_INDEX,
    CREATE_VIDEO_FACE_TAG_ID_INDEX,
    CREATE_OBJECT_INDEX,
    CREATE_RECOMMENDATION_INDEX,
    CREATE_COMPOSITION_INDEX,
    CREATE_HEAD_INDEX,
    CREATE_POSE_INDEX,
    CREATE_PET_INDEX,
    CREATE_PET_TAG_ID_INDEX,
    CREATE_GEO_KNOWLEDGE_TABLE,
    CREATE_GEO_DICTIONARY_TABLE,
    CREATE_ANALYSIS_ALBUM_FOR_ONCREATE,
    CREATE_ANALYSIS_ALBUM_GROUP_TAG_INDEX,
    CREATE_ANALYSIS_ALBUM_SUBTYPE_NAME_INDEX,
    CREATE_ANALYSIS_ALBUM_TAG_ID_INDEX,
    CREATE_ANALYSIS_ALBUM_MAP,
    CREATE_HIGHLIGHT_ALBUM_TABLE,
    CREATE_HIGHLIGHT_COVER_INFO_TABLE,
    CREATE_HIGHLIGHT_PLAY_INFO_TABLE,
    CREATE_USER_PHOTOGRAPHY_INFO_TABLE,
    CREATE_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER,
    CREATE_INSERT_SOURCE_UPDATE_ALBUM_ID_TRIGGER,
    INSERT_PHOTO_UPDATE_ALBUM_BUNDLENAME,
    CREATE_SOURCE_ALBUM_INDEX,
    FormMap::CREATE_FORM_MAP_TABLE,
    CREATE_DICTIONARY_INDEX,
    DROP_KNOWLEDGE_INDEX,
    CREATE_NEW_KNOWLEDGE_INDEX,
    CREATE_CITY_NAME_INDEX,
    CREATE_LOCATION_KEY_INDEX,
    CREATE_IDX_FILEID_FOR_ANALYSIS_TOTAL,
    CREATE_IDX_FILEID_FOR_ANALYSIS_PHOTO_MAP,
    CREATE_TAB_ANALYSIS_ALBUM_TOTAL,
    CREATE_TOTAL_INSERT_TRIGGER_FOR_ADD_ANALYSIS_ALBUM_TOTAL,
    CREATE_VISION_UPDATE_TRIGGER_FOR_UPDATE_ANALYSIS_ALBUM_TOTAL_STATUS,
    CREATE_ANALYSIS_ALBUM_ASET_MAP_TABLE,
    CREATE_ANALYSIS_ASSET_SD_MAP_TABLE,
    CREATE_TAB_ASSET_ALBUM_OPERATION,
    CREATE_OPERATION_ASSET_INSERT_TRIGGER,
    CREATE_OPERATION_ASSET_DELETE_TRIGGER,
    CREATE_OPERATION_ASSET_UPDATE_TRIGGER,
    CREATE_OPERATION_ALBUM_INSERT_TRIGGER,
    CREATE_OPERATION_ALBUM_DELETE_TRIGGER,
    CREATE_OPERATION_ALBUM_UPDATE_TRIGGER,
    CREATE_ANALYSIS_PHOTO_MAP_MAP_ASSET_INDEX,
    ConfigInfoColumn::CREATE_CONFIG_INFO_TABLE,
    CREATE_ALBUM_ORDER_BACK_TABLE,
    CREATE_LAKE_ALBUM_TABLE,

    // search
    CREATE_SEARCH_TOTAL_TABLE,
    CREATE_SEARCH_INSERT_TRIGGER,
    CREATE_SEARCH_UPDATE_TRIGGER,
    CREATE_SEARCH_UPDATE_STATUS_TRIGGER,
    CREATE_SEARCH_DELETE_TRIGGER,
    CREATE_IDX_FILEID_FOR_SEARCH_INDEX,
    CREATE_ALBUM_UPDATE_SEARCH_TRIGGER,
    CREATE_ANALYSIS_UPDATE_SEARCH_TRIGGER,
    CREATE_ANALYSIS_UPDATE_VIDEO_SEARCH_TRIGGER,
    CREATE_ANALYSIS_ALBUM_UPDATE_SEARCH_TRIGGER,
    MedialibraryBusinessRecordColumn::CREATE_TABLE,
    MedialibraryBusinessRecordColumn::CREATE_BUSINESS_KEY_INDEX,
    PhotoExtUpgrade::CREATE_PHOTO_EXT_TABLE,
    PhotoUpgrade::CREATE_PHOTO_DISPLAYNAME_INDEX,
    AppUriPermissionColumn::CREATE_APP_URI_PERMISSION_TABLE,
    AppUriPermissionColumn::CREATE_URI_URITYPE_TOKENID_INDEX,
    TriggerDeletePhotoClearAppUriPermission(),
    TriggerDeleteAudioClearAppUriPermission(),
    PhotoUpgrade::CREATE_PHOTO_BURSTKEY_INDEX,
    PhotoUpgrade::UPDATA_PHOTOS_DATA_UNIQUE,
    PhotoUpgrade::INSERT_GENERATE_HIGHLIGHT_THUMBNAIL,
    PhotoUpgrade::UPDATE_GENERATE_HIGHLIGHT_THUMBNAIL,
    PhotoUpgrade::INDEX_HIGHLIGHT_FILEID,
    PhotoUpgrade::INDEX_LATITUDE,
    PhotoUpgrade::INDEX_LONGITUDE,
    CustomRecordsColumns::CREATE_TABLE,
    PhotoUpgrade::CREATE_PHOTO_SORT_MEDIA_TYPE_DATE_TAKEN_INDEX,
    PhotoUpgrade::CREATE_PHOTO_SORT_MEDIA_TYPE_DATE_ADDED_INDEX,
    PhotoUpgrade::CREATE_PHOTO_SORT_IN_ALBUM_SIZE_INDEX,
    PhotoUpgrade::CREATE_PHOTO_SORT_MEDIA_TYPE_SIZE_INDEX,
    PhotoUpgrade::CREATE_PHOTO_SORT_IN_ALBUM_DISPLAY_NAME_INDEX,
    PhotoUpgrade::CREATE_PHOTO_SORT_MEDIA_TYPE_DISPLAY_NAME_INDEX,

    // tab_analysis_progress
    CREATE_TAB_ANALYSIS_PROGRESS,
    DownloadResourcesColumn::CREATE_TABLE,
    DownloadResourcesColumn::INDEX_DRTR_ID_STATUS,
};

static int32_t ExecuteSql(RdbStore &store)
{
    for (const string& sqlStr : onCreateSqlStrs) {
        auto ret = ExecSqlWithRetry([&]() { return store.ExecuteSql(sqlStr); });
        CHECK_AND_RETURN_RET(ret == NativeRdb::E_OK, NativeRdb::E_ERROR);
    }
    CHECK_AND_RETURN_RET(TabOldPhotosTableEventHandler().OnCreate(store) == NativeRdb::E_OK,
        NativeRdb::E_ERROR);
    CHECK_AND_RETURN_RET(TabOldAlbumTableEventHandler().OnCreate(store) == NativeRdb::E_OK,
        NativeRdb::E_ERROR);
    CHECK_AND_RETURN_RET(TabFaCardPhotosTableEventHandler().OnCreate(store) == NativeRdb::E_OK,
        NativeRdb::E_ERROR);
    if (TabFaCardPhotosTableEventHandler().OnCreate(store) != NativeRdb::E_OK) {
        return NativeRdb::E_ERROR;
    }
    if (TabClonedOldPhotosTableEventHandler().OnCreate(store) != NativeRdb::E_OK) {
        return NativeRdb::E_ERROR;
    }
    TableEventHandler().OnCreate(MediaLibraryUnistoreManager::GetInstance().GetRdbStore());
    return NativeRdb::E_OK;
}

int32_t MediaLibraryDataCallBack::OnCreate(RdbStore &store)
{
    MEDIA_INFO_LOG("Rdb OnCreate");
#ifdef META_RECOVERY_SUPPORT
    NativeRdb::RebuiltType rebuilt = NativeRdb::RebuiltType::NONE;
    store.GetRebuilt(rebuilt);
#endif

    if (ExecuteSql(store) != NativeRdb::E_OK) {
        return NativeRdb::E_ERROR;
    }

#ifdef META_RECOVERY_SUPPORT
    if (rebuilt == NativeRdb::RebuiltType::REBUILT) {
        // set Rebuilt flag
        MediaLibraryMetaRecovery::GetInstance().SetRdbRebuiltStatus(true);
    }
#endif

    if (PrepareSystemAlbums(store) != NativeRdb::E_OK) {
        return NativeRdb::E_ERROR;
    }

    if (PrepareAlbumPlugin(store) != NativeRdb::E_OK) {
        return NativeRdb::E_ERROR;
    }

    if (PrepareDir(store) != NativeRdb::E_OK) {
        return NativeRdb::E_ERROR;
    }

    if (PrepareSmartAlbum(store) != NativeRdb::E_OK) {
        return NativeRdb::E_ERROR;
    }

    if (PrepareUniqueMemberTable(store) != NativeRdb::E_OK) {
        return NativeRdb::E_ERROR;
    }

    PrepareShootingModeAlbum(store);

    MediaLibraryRdbStore::SetOldVersion(MEDIA_RDB_VERSION);
    RdbUpgradeUtils::AddMapValueToPreference();
    return NativeRdb::E_OK;
}

void VersionAddCloud(RdbStore &store)
{
    const std::string alterCloudId = "ALTER TABLE " + MEDIALIBRARY_TABLE +
        " ADD COLUMN " + MEDIA_DATA_DB_CLOUD_ID +" TEXT";
    int32_t result = ExecSqlWithRetry([&]() { return store.ExecuteSql(alterCloudId); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Upgrade rdb cloud_id error %{private}d", result);
    }
    const std::string alterDirty = "ALTER TABLE " + MEDIALIBRARY_TABLE +
        " ADD COLUMN " + MEDIA_DATA_DB_DIRTY +" INT DEFAULT 0";
    result = ExecSqlWithRetry([&]() { return store.ExecuteSql(alterDirty); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Upgrade rdb dirty error %{private}d", result);
    }
    const std::string alterSyncStatus = "ALTER TABLE " + MEDIALIBRARY_TABLE +
        " ADD COLUMN " + MEDIA_DATA_DB_SYNC_STATUS +" INT DEFAULT 0";
    result = ExecSqlWithRetry([&]() { return store.ExecuteSql(alterSyncStatus); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Upgrade rdb syncStatus error %{private}d", result);
    }
    const std::string alterPosition = "ALTER TABLE " + MEDIALIBRARY_TABLE +
        " ADD COLUMN " + MEDIA_DATA_DB_POSITION +" INT DEFAULT 1";
    result = ExecSqlWithRetry([&]() { return store.ExecuteSql(alterPosition); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Upgrade rdb position error %{private}d", result);
    }
}

static void AddPortraitInAnalysisAlbum(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        ADD_TAG_ID_COLUMN_FOR_ALBUM,
        ADD_USER_OPERATION_COLUMN_FOR_ALBUM,
        ADD_GROUP_TAG_COLUMN_FOR_ALBUM,
        ADD_USER_DISPLAY_LEVEL_COLUMN_FOR_ALBUM,
        ADD_IS_ME_COLUMN_FOR_ALBUM,
        ADD_IS_REMOVED_COLUMN_FOR_ALBUM,
        ADD_RENAME_OPERATION_COLUMN_FOR_ALBUM,
        CREATE_ANALYSIS_ALBUM_UPDATE_SEARCH_TRIGGER
    };
    MEDIA_INFO_LOG("start add aesthetic composition tables");
    ExecSqls(executeSqlStrs, store);
}

void AddMetaModifiedColumn(RdbStore &store)
{
    const std::string alterMetaModified =
        "ALTER TABLE " + MEDIALIBRARY_TABLE + " ADD COLUMN " +
        MEDIA_DATA_DB_META_DATE_MODIFIED + " BIGINT DEFAULT 0";
    int32_t result = ExecSqlWithRetry([&]() { return store.ExecuteSql(alterMetaModified); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Upgrade rdb meta_date_modified error %{private}d", result);
    }
    const std::string alterSyncStatus = "ALTER TABLE " + MEDIALIBRARY_TABLE +
        " ADD COLUMN " + MEDIA_DATA_DB_SYNC_STATUS + " INT DEFAULT 0";
    result = ExecSqlWithRetry([&]() { return store.ExecuteSql(alterSyncStatus); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Upgrade rdb syncStatus error %{private}d", result);
    }
}

void AddTableType(RdbStore &store)
{
    const std::string alterTableName =
        "ALTER TABLE " + BUNDLE_PERMISSION_TABLE + " ADD COLUMN " + PERMISSION_TABLE_TYPE + " INT";
    int32_t result = ExecSqlWithRetry([&]() { return store.ExecuteSql(alterTableName); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Upgrade rdb table_name error %{private}d", result);
    }
}

void API10TableCreate(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        PhotoUpgrade::INDEX_SCTHP_ADDTIME,
        PhotoUpgrade::INDEX_CAMERA_SHOT_KEY,
        PhotoUpgrade::CREATE_PHOTOS_DELETE_TRIGGER,
        PhotoUpgrade::CREATE_PHOTOS_FDIRTY_TRIGGER,
        PhotoUpgrade::CREATE_PHOTOS_MDIRTY_TRIGGER,
        PhotoUpgrade::CREATE_PHOTOS_INSERT_CLOUD_SYNC,
        AudioColumn::CREATE_AUDIO_TABLE,
        CREATE_ASSET_UNIQUE_NUMBER_TABLE,
        CREATE_FILES_DELETE_TRIGGER,
        CREATE_FILES_MDIRTY_TRIGGER,
        CREATE_FILES_FDIRTY_TRIGGER,
        CREATE_INSERT_CLOUD_SYNC_TRIGGER,
        PhotoAlbumColumns::CREATE_TABLE,
        PhotoAlbumColumns::INDEX_ALBUM_TYPES,
        PhotoMap::CREATE_TABLE,
        FormMap::CREATE_FORM_MAP_TABLE,
        TriggerDeleteAlbumClearMap(),
        TriggerAddAssets(),
        TriggerRemoveAssets(),
        TriggerDeletePhotoClearMap(),
        TriggerUpdateUserAlbumCount(),
    };

    for (size_t i = 0; i < executeSqlStrs.size(); i++) {
        auto result = ExecSqlWithRetry([&]() { return store.ExecuteSql(executeSqlStrs[i]); });
        if (result != NativeRdb::E_OK) {
            UpdateFail(__FILE__, __LINE__);
            MEDIA_ERR_LOG("upgrade fail idx:%{public}zu", i);
        }
    }
}

void ModifySyncStatus(RdbStore &store)
{
    const std::string dropSyncStatus = "ALTER TABLE " + MEDIALIBRARY_TABLE + " DROP column syncing";
    auto result = ExecSqlWithRetry([&]() { return store.ExecuteSql(dropSyncStatus); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Upgrade rdb syncing error %{private}d", result);
    }

    const std::string addSyncStatus = "ALTER TABLE " + MEDIALIBRARY_TABLE + " ADD COLUMN " +
        MEDIA_DATA_DB_SYNC_STATUS +" INT DEFAULT 0";
    result = ExecSqlWithRetry([&]() { return store.ExecuteSql(addSyncStatus); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Upgrade rdb syncStatus error %{private}d", result);
    }
}

void ModifyDeleteTrigger(RdbStore &store)
{
    /* drop old delete trigger */
    const std::string dropDeleteTrigger = "DROP TRIGGER IF EXISTS photos_delete_trigger";
    if (ExecSqlWithRetry([&]() { return store.ExecuteSql(dropDeleteTrigger); }) != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("upgrade fail: drop old delete trigger");
    }

    /* create new delete trigger */
    if (ExecSqlWithRetry([&]() { return store.ExecuteSql(PhotoUpgrade::CREATE_PHOTOS_DELETE_TRIGGER); }) !=
        NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("upgrade fail: create new delete trigger");
    }
}

void AddCloudVersion(RdbStore &store)
{
    const std::string addSyncStatus = "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
        PhotoColumn::PHOTO_CLOUD_VERSION +" BIGINT DEFAULT 0";
    auto result = ExecSqlWithRetry([&]() { return store.ExecuteSql(addSyncStatus); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Upgrade rdb cloudVersion error %{private}d", result);
    }
}

static string UpdateCloudPathSql(const string &table, const string &column)
{
    static const string LOCAL_PATH = "/storage/media/local/";
    static const string CLOUD_PATH = "/storage/cloud/";
    /*
     * replace only once:
     * UPDATE photos
     * SET data = ([replace](substring(data, 1, len(local_path)), local_path, cloud_path) ||
     * substring(data, len(local_path) + 1));
     */
    return "UPDATE " + table + " SET " + column + " = (REPLACE(SUBSTRING(" +
        column + ", 1, " + to_string(LOCAL_PATH.length()) + "), '" +
        LOCAL_PATH + "', '" + CLOUD_PATH + "') || SUBSTRING(" + column + ", " +
        to_string(LOCAL_PATH.length() + 1) + "))" +
        " WHERE " + column + " LIKE '" + LOCAL_PATH + "%';";
}

static void UpdateMdirtyTriggerForSdirty(RdbStore &store)
{
    const string dropMdirtyCreateTrigger = "DROP TRIGGER IF EXISTS photos_mdirty_trigger";
    int32_t ret = ExecSqlWithRetry([&]() { return store.ExecuteSql(dropMdirtyCreateTrigger); });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("drop photos_mdirty_trigger fail, ret = %{public}d", ret);
        UpdateFail(__FILE__, __LINE__);
    }

    ret = ExecSqlWithRetry([&]() { return store.ExecuteSql(PhotoUpgrade::CREATE_PHOTOS_MDIRTY_TRIGGER); });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("add photos_mdirty_trigger fail, ret = %{public}d", ret);
        UpdateFail(__FILE__, __LINE__);
    }
}

static int32_t UpdateCloudPath(RdbStore &store)
{
    const vector<string> updateCloudPath = {
        UpdateCloudPathSql(MEDIALIBRARY_TABLE, MEDIA_DATA_DB_FILE_PATH),
        UpdateCloudPathSql(MEDIALIBRARY_TABLE, MEDIA_DATA_DB_RECYCLE_PATH),
        UpdateCloudPathSql(MEDIALIBRARY_ERROR_TABLE, MEDIA_DATA_ERROR),
        UpdateCloudPathSql(PhotoColumn::PHOTOS_TABLE, MediaColumn::MEDIA_FILE_PATH),
    };
    auto result = ExecSqls(updateCloudPath, store);
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
    }
    return result;
}

void UpdateAPI10Table(RdbStore &store)
{
    ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP INDEX IF EXISTS idx_sthp_dateadded"); });
    ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP INDEX IF EXISTS photo_album_types"); });
    ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS photos_delete_trigger"); });
    ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS photos_fdirty_trigger"); });
    ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS photos_mdirty_trigger"); });
    ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS photo_insert_cloud_sync_trigger"); });
    ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS delete_trigger"); });
    ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS mdirty_trigger"); });
    ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS fdirty_trigger"); });
    ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS insert_cloud_sync_trigger"); });
    ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS photo_album_clear_map"); });
    ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS photo_album_insert_asset"); });
    ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS photo_album_delete_asset"); });
    ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS delete_photo_clear_map"); });
    ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS update_user_album_count"); });
    ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TABLE IF EXISTS Photos"); });
    ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TABLE IF EXISTS Audios"); });
    ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TABLE IF EXISTS UniqueNumber"); });
    ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TABLE IF EXISTS PhotoAlbum"); });
    ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TABLE IF EXISTS PhotoMap"); });
    ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TABLE IF EXISTS FormMap"); });

    API10TableCreate(store);
    if (PrepareSystemAlbums(store) != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
    }

    if (PrepareUniqueMemberTable(store) != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
    }

    // set scan error
    MediaScannerManager::GetInstance()->ErrorRecord();
}

static void AddLocationTables(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        CREATE_GEO_DICTIONARY_TABLE,
        CREATE_GEO_KNOWLEDGE_TABLE,
    };
    MEDIA_INFO_LOG("start init location db");
    ExecSqls(executeSqlStrs, store);
}

static void UpdateLocationTables(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        "DROP TABLE IF EXISTS tab_geo_dictionary",
        "DROP TABLE IF EXISTS tab_geo_knowledge",
        CREATE_GEO_DICTIONARY_TABLE,
        CREATE_GEO_KNOWLEDGE_TABLE,
    };
    MEDIA_INFO_LOG("fix location db");
    ExecSqls(executeSqlStrs, store);
}

static void AddAnalysisTables(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        "DROP TABLE IF EXISTS tab_analysis_label",
        CREATE_TAB_ANALYSIS_OCR,
        CREATE_TAB_ANALYSIS_LABEL,
        CREATE_TAB_ANALYSIS_AESTHETICS,
        CREATE_TAB_ANALYSIS_TOTAL,
        CREATE_VISION_UPDATE_TRIGGER,
        CREATE_VISION_DELETE_TRIGGER,
        CREATE_VISION_INSERT_TRIGGER,
        INIT_TAB_ANALYSIS_TOTAL,
    };
    MEDIA_INFO_LOG("start init vision db");
    ExecSqls(executeSqlStrs, store);
}

static void AddFaceTables(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        CREATE_TAB_IMAGE_FACE,
        CREATE_TAB_FACE_TAG,
        DROP_INSERT_VISION_TRIGGER,
        CREATE_INSERT_VISION_TRIGGER_FOR_ADD_FACE,
        ADD_FACE_STATUS_COLUMN,
        UPDATE_TOTAL_VALUE,
        UPDATE_NOT_SUPPORT_VALUE,
        CREATE_IMAGE_FACE_INDEX
    };
    MEDIA_INFO_LOG("start add face tables");
    ExecSqls(executeSqlStrs, store);
}

static void AddSaliencyTables(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        CREATE_TAB_ANALYSIS_SALIENCY_DETECT,
        DROP_INSERT_VISION_TRIGGER,
        CREATE_VISION_INSERT_TRIGGER_FOR_ADD_SALIENCY,
        ADD_SALIENCY_STATUS_COLUMN,
        UPDATE_SALIENCY_TOTAL_VALUE,
        UPDATE_SALIENCY_NOT_SUPPORT_VALUE
    };
    MEDIA_INFO_LOG("start add saliency tables");
    ExecSqls(executeSqlStrs, store);
}

static void AddVideoLabelTable(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        CREATE_TAB_ANALYSIS_VIDEO_LABEL,
        DROP_INSERT_VISION_TRIGGER,
        DROP_UPDATE_VISION_TRIGGER,
        CREATE_VISION_INSERT_TRIGGER_FOR_ADD_VIDEO_LABEL,
        CREATE_VISION_UPDATE_TRIGGER_FOR_ADD_VIDEO_LABEL,
    };
    MEDIA_INFO_LOG("start add video label tables");
    ExecSqls(executeSqlStrs, store);
}

static void UpdateVideoLabelTable(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        "DROP TABLE IF EXISTS tab_analysis_video_label",
        CREATE_TAB_ANALYSIS_VIDEO_LABEL,
    };
    MEDIA_INFO_LOG("start update video label tables");
    ExecSqls(executeSqlStrs, store);
}

static void AddSourceAlbumTrigger(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        DROP_INSERT_PHOTO_INSERT_SOURCE_ALBUM,
        DROP_INSERT_PHOTO_UPDATE_SOURCE_ALBUM,
        DROP_UPDATE_PHOTO_UPDATE_SOURCE_ALBUM,
        DROP_DELETE_PHOTO_UPDATE_SOURCE_ALBUM,
        INSERT_PHOTO_INSERT_SOURCE_ALBUM,
        INSERT_PHOTO_UPDATE_SOURCE_ALBUM,
        UPDATE_PHOTO_UPDATE_SOURCE_ALBUM,
        DELETE_PHOTO_UPDATE_SOURCE_ALBUM,
    };
    MEDIA_INFO_LOG("start add source album trigger");
    ExecSqls(executeSqlStrs, store);
}

static void RemoveSourceAlbumToAnalysis(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        DROP_INSERT_PHOTO_INSERT_SOURCE_ALBUM,
        DROP_INSERT_PHOTO_UPDATE_SOURCE_ALBUM,
        DROP_UPDATE_PHOTO_UPDATE_SOURCE_ALBUM,
        DROP_DELETE_PHOTO_UPDATE_SOURCE_ALBUM,
        CLEAR_SOURCE_ALBUM_PHOTO_MAP,
        CLEAR_SYSTEM_SOURCE_ALBUM,
        INSERT_PHOTO_INSERT_SOURCE_ALBUM,
        INSERT_PHOTO_UPDATE_SOURCE_ALBUM,
        UPDATE_PHOTO_UPDATE_SOURCE_ALBUM,
        DELETE_PHOTO_UPDATE_SOURCE_ALBUM,
    };
    MEDIA_INFO_LOG("start add source album trigger");
    ExecSqls(executeSqlStrs, store);
}

static void MoveSourceAlbumToPhotoAlbumAndAddColumns(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        DROP_INSERT_PHOTO_INSERT_SOURCE_ALBUM,
        DROP_INSERT_PHOTO_UPDATE_SOURCE_ALBUM,
        DROP_UPDATE_PHOTO_UPDATE_SOURCE_ALBUM,
        DROP_DELETE_PHOTO_UPDATE_SOURCE_ALBUM,
        ADD_SOURCE_ALBUM_BUNDLE_NAME,
        INSERT_SOURCE_ALBUMS_FROM_PHOTOS,
        INSERT_SOURCE_ALBUM_MAP_FROM_PHOTOS,
        CLEAR_SOURCE_ALBUM_ANALYSIS_PHOTO_MAP,
        CLEAR_ANALYSIS_SOURCE_ALBUM,
        INSERT_PHOTO_INSERT_SOURCE_ALBUM,
        INSERT_PHOTO_UPDATE_SOURCE_ALBUM,
        UPDATE_PHOTO_UPDATE_SOURCE_ALBUM,
        DELETE_PHOTO_UPDATE_SOURCE_ALBUM,
    };
    MEDIA_INFO_LOG("start move source album to photo album & add columns");
    ExecSqls(executeSqlStrs, store);
}

static void ModifySourceAlbumTriggers(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        DROP_INSERT_PHOTO_INSERT_SOURCE_ALBUM,
        DROP_INSERT_PHOTO_UPDATE_SOURCE_ALBUM,
        DROP_UPDATE_PHOTO_UPDATE_SOURCE_ALBUM,
        DROP_DELETE_PHOTO_UPDATE_SOURCE_ALBUM,
        DROP_SOURCE_ALBUM_INDEX,
        ADD_SOURCE_ALBUM_LOCAL_LANGUAGE,
        CREATE_SOURCE_ALBUM_INDEX,
        INSERT_SOURCE_ALBUMS_FROM_PHOTOS_FULL,
        INSERT_SOURCE_ALBUM_MAP_FROM_PHOTOS_FULL,
        INSERT_PHOTO_INSERT_SOURCE_ALBUM,
        INSERT_PHOTO_UPDATE_SOURCE_ALBUM,
    };
    MEDIA_INFO_LOG("start modify source album triggers");
    ExecSqls(executeSqlStrs, store);
    MediaLibraryRdbUtils::UpdateSourceAlbumInternal(MediaLibraryUnistoreManager::GetInstance().GetRdbStore());
    MEDIA_INFO_LOG("end modify source album triggers");
}

static void AddAnalysisAlbum(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        "ALTER TABLE tab_analysis_ocr ADD COLUMN width INT;",
        "ALTER TABLE tab_analysis_ocr ADD COLUMN height INT;",
        CREATE_ANALYSIS_ALBUM,
        CREATE_ANALYSIS_ALBUM_MAP,
    };
    MEDIA_INFO_LOG("start init vision album");
    ExecSqls(executeSqlStrs, store);
}

static void AddAestheticCompositionTables(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        CREATE_TAB_ANALYSIS_OBJECT,
        CREATE_TAB_ANALYSIS_RECOMMENDATION,
        CREATE_TAB_ANALYSIS_SEGMENTATION,
        CREATE_TAB_ANALYSIS_COMPOSITION,
        DROP_INSERT_VISION_TRIGGER,
        CREATE_VISION_INSERT_TRIGGER_FOR_ADD_AC,
        AC_ADD_OBJECT_COLUMN_FOR_TOTAL,
        AC_UPDATE_OBJECT_TOTAL_VALUE,
        AC_UPDATE_OBJECT_TOTAL_NOT_SUPPORT_VALUE,
        AC_ADD_RECOMMENDATION_COLUMN_FOR_TOTAL,
        AC_UPDATE_RECOMMENDATION_TOTAL_VALUE,
        AC_UPDATE_RECOMMENDATION_TOTAL_NOT_SUPPORT_VALUE,
        AC_ADD_SEGMENTATION_COLUMN_FOR_TOTAL,
        AC_UPDATE_SEGMENTATION_TOTAL_VALUE,
        AC_UPDATE_SEGMENTATION_TOTAL_NOT_SUPPORT_VALUE,
        AC_ADD_COMPOSITION_COLUMN_FOR_TOTAL,
        AC_UPDATE_COMPOSITION_TOTAL_VALUE,
        AC_UPDATE_COMPOSITION_TOTAL_NOT_SUPPORT_VALUE,
        CREATE_OBJECT_INDEX,
        CREATE_RECOMMENDATION_INDEX,
        CREATE_COMPOSITION_INDEX,
    };
    MEDIA_INFO_LOG("start add aesthetic composition tables");
    ExecSqls(executeSqlStrs, store);
}

static void UpdateSpecForAddScreenshot(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        DROP_INSERT_VISION_TRIGGER,
        CREATE_VISION_INSERT_TRIGGER_FOR_UPDATE_SPEC,
    };
    MEDIA_INFO_LOG("update media analysis service specifications for add screenshot");
    ExecSqls(executeSqlStrs, store);
}

static void AddHeadAndPoseTables(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        CREATE_TAB_ANALYSIS_HEAD,
        CREATE_TAB_ANALYSIS_POSE,
        DROP_INSERT_VISION_TRIGGER,
        CREATE_VISION_INSERT_TRIGGER_FOR_ADD_HEAD_AND_POSE,
        ADD_HEAD_STATUS_COLUMN,
        UPDATE_HEAD_TOTAL_VALUE,
        UPDATE_HEAD_NOT_SUPPORT_VALUE,
        ADD_POSE_STATUS_COLUMN,
        UPDATE_POSE_TOTAL_VALUE,
        UPDATE_POSE_NOT_SUPPORT_VALUE,
        CREATE_HEAD_INDEX,
        CREATE_POSE_INDEX,
    };
    MEDIA_INFO_LOG("start add head and pose tables");
    ExecSqls(executeSqlStrs, store);
}

static void AddFaceOcclusionAndPoseTypeColumn(RdbStore &store)
{
    MEDIA_INFO_LOG("start add face occlusion and pose type column");
    MediaLibraryRdbStore::AddColumnIfNotExists(store, FACE_OCCLUSION, "INT", VISION_IMAGE_FACE_TABLE);
    MediaLibraryRdbStore::AddColumnIfNotExists(store, POSE_TYPE, "INT", VISION_POSE_TABLE);
}

static void AddSegmentationColumns(RdbStore &store)
{
    const string addNameOnSegmentation = "ALTER TABLE " + VISION_SEGMENTATION_TABLE + " ADD COLUMN " +
        SEGMENTATION_NAME + " INT";
    const string addProbOnSegmentation = "ALTER TABLE " + VISION_SEGMENTATION_TABLE + " ADD COLUMN " +
        PROB + " REAL";

    const vector<string> addSegmentationColumns = { addNameOnSegmentation, addProbOnSegmentation };
    ExecSqls(addSegmentationColumns, store);
}

static void AddSearchTable(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        "DROP TABLE IF EXISTS " + SEARCH_TOTAL_TABLE,
        "DROP TRIGGER IF EXISTS " + INSERT_SEARCH_TRIGGER,
        "DROP TRIGGER IF EXISTS " + UPDATE_SEARCH_TRIGGER,
        "DROP TRIGGER IF EXISTS " + UPDATE_SEARCH_STATUS_TRIGGER,
        "DROP TRIGGER IF EXISTS " + DELETE_SEARCH_TRIGGER,
        "DROP TRIGGER IF EXISTS " + ALBUM_MAP_INSERT_SEARCH_TRIGGER,
        "DROP TRIGGER IF EXISTS " + ALBUM_MAP_DELETE_SEARCH_TRIGGER,
        "DROP TRIGGER IF EXISTS " + ALBUM_UPDATE_SEARCH_TRIGGER,
        "DROP TRIGGER IF EXISTS " + ANALYSIS_UPDATE_SEARCH_TRIGGER,
        CREATE_SEARCH_TOTAL_TABLE,
        CREATE_SEARCH_INSERT_TRIGGER,
        CREATE_SEARCH_UPDATE_TRIGGER,
        CREATE_SEARCH_UPDATE_STATUS_TRIGGER,
        CREATE_SEARCH_DELETE_TRIGGER,
        CREATE_ALBUM_UPDATE_SEARCH_TRIGGER,
        CREATE_ANALYSIS_UPDATE_SEARCH_TRIGGER,
    };
    MEDIA_INFO_LOG("start init search db");
    ExecSqls(executeSqlStrs, store);
}

static void UpdateInsertPhotoUpdateAlbumTrigger(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        INSERT_PHOTO_UPDATE_ALBUM_BUNDLENAME,
    };
    MEDIA_INFO_LOG("start update insert photo update album");
    ExecSqls(executeSqlStrs, store);
}

bool MediaLibraryRdbStore::ResetSearchTables()
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), false,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");

    static const vector<string> executeSqlStrs = {
        "DROP TABLE IF EXISTS " + SEARCH_TOTAL_TABLE,
        "DROP TRIGGER IF EXISTS " + INSERT_SEARCH_TRIGGER,
        "DROP TRIGGER IF EXISTS " + UPDATE_SEARCH_TRIGGER,
        "DROP TRIGGER IF EXISTS " + UPDATE_SEARCH_STATUS_TRIGGER,
        "DROP TRIGGER IF EXISTS " + DELETE_SEARCH_TRIGGER,
        "DROP TRIGGER IF EXISTS " + ALBUM_MAP_INSERT_SEARCH_TRIGGER,
        "DROP TRIGGER IF EXISTS " + ALBUM_MAP_DELETE_SEARCH_TRIGGER,
        "DROP TRIGGER IF EXISTS " + ALBUM_UPDATE_SEARCH_TRIGGER,
        "DROP TRIGGER IF EXISTS " + ANALYSIS_UPDATE_SEARCH_TRIGGER,
    };
    MEDIA_INFO_LOG("start update search db");
    ExecSqls(executeSqlStrs, *rdbStore_);
    AddSearchTable(*rdbStore_);
    return true;
}

bool MediaLibraryRdbStore::ResetAnalysisTables()
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), false,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");

    static const vector<string> executeSqlStrs = {
        "DROP TRIGGER IF EXISTS delete_vision_trigger",
        "DROP TRIGGER IF EXISTS insert_vision_trigger",
        "DROP TRIGGER IF EXISTS update_vision_trigger",
        "DROP TABLE IF EXISTS tab_analysis_ocr",
        "DROP TABLE IF EXISTS tab_analysis_label",
        "DROP TABLE IF EXISTS tab_analysis_saliency_detect",
        "DROP TABLE IF EXISTS tab_analysis_aesthetics_score",
        "DROP TABLE IF EXISTS tab_analysis_object",
        "DROP TABLE IF EXISTS tab_analysis_recommendation",
        "DROP TABLE IF EXISTS tab_analysis_segmentation",
        "DROP TABLE IF EXISTS tab_analysis_composition",
        "DROP TABLE IF EXISTS tab_analysis_total",
        "DROP TABLE IF EXISTS tab_analysis_image_face",
        "DROP TABLE IF EXISTS tab_analysis_face_tag",
        "DROP TABLE IF EXISTS tab_analysis_head",
        "DROP TABLE IF EXISTS tab_analysis_pose",
    };
    MEDIA_INFO_LOG("start update analysis table");
    ExecSqls(executeSqlStrs, *rdbStore_);
    AddAnalysisTables(*rdbStore_);
    AddFaceTables(*rdbStore_);
    AddAestheticCompositionTables(*rdbStore_);
    AddSaliencyTables(*rdbStore_);
    UpdateSpecForAddScreenshot(*rdbStore_);
    AddHeadAndPoseTables(*rdbStore_);
    AddSegmentationColumns(*rdbStore_);
    AddFaceOcclusionAndPoseTypeColumn(*rdbStore_);
    AddVideoLabelTable(*rdbStore_);

    return true;
}

static void AddPackageNameColumnOnTables(RdbStore &store)
{
    static const string ADD_PACKAGE_NAME_ON_PHOTOS = "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE +
        " ADD COLUMN " + PhotoColumn::MEDIA_PACKAGE_NAME + " TEXT";
    static const string ADD_PACKAGE_NAME_ON_AUDIOS = "ALTER TABLE " + AudioColumn::AUDIOS_TABLE +
        " ADD COLUMN " + AudioColumn::MEDIA_PACKAGE_NAME + " TEXT";
    static const string ADD_PACKAGE_NAME_ON_FILES = "ALTER TABLE " + MEDIALIBRARY_TABLE +
        " ADD COLUMN " + MEDIA_DATA_DB_PACKAGE_NAME + " TEXT";

    int32_t result = ExecSqlWithRetry([&]() { return store.ExecuteSql(ADD_PACKAGE_NAME_ON_PHOTOS); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Failed to update PHOTOS");
    }
    result = ExecSqlWithRetry([&]() { return store.ExecuteSql(ADD_PACKAGE_NAME_ON_AUDIOS); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Failed to update AUDIOS");
    }
    result = ExecSqlWithRetry([&]() { return store.ExecuteSql(ADD_PACKAGE_NAME_ON_FILES); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Failed to update FILES");
    }
}

void UpdateCloudAlbum(RdbStore &store)
{
    /* album - add columns */
    const std::string addAlbumDirty = "ALTER TABLE " + PhotoAlbumColumns::TABLE +
        " ADD COLUMN " + PhotoAlbumColumns::ALBUM_DIRTY + " INT DEFAULT " +
        to_string(static_cast<int32_t>(DirtyTypes::TYPE_NEW)) + ";";
    int32_t ret = ExecSqlWithRetry([&]() { return store.ExecuteSql(addAlbumDirty); });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("upgrade fail %{public}d: add ablum dirty", ret);
        UpdateFail(__FILE__, __LINE__);
    }
    const std::string addAlbumCloudId = "ALTER TABLE " + PhotoAlbumColumns::TABLE +
        " ADD COLUMN " + PhotoAlbumColumns::ALBUM_CLOUD_ID + " TEXT;";
    ret = ExecSqlWithRetry([&]() { return store.ExecuteSql(addAlbumCloudId); });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("upgrade fail %{public}d: add ablum cloud id", ret);
        UpdateFail(__FILE__, __LINE__);
    }
    /* album - add triggers */
    ret = ExecSqlWithRetry([&]() { return store.ExecuteSql(PhotoAlbumColumns::CREATE_ALBUM_INSERT_TRIGGER); });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("upgrade fail %{public}d: create album insert trigger", ret);
        UpdateFail(__FILE__, __LINE__);
    }
    ret = ExecSqlWithRetry([&]() { return store.ExecuteSql(PhotoAlbumColumns::CREATE_ALBUM_MDIRTY_TRIGGER); });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("upgrade fail %{public}d: create album modify trigger", ret);
        UpdateFail(__FILE__, __LINE__);
    }
    ret = ExecSqlWithRetry([&]() { return store.ExecuteSql(PhotoAlbumColumns::CREATE_ALBUM_DELETE_TRIGGER); });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("upgrade fail %{public}d: create album delete trigger", ret);
        UpdateFail(__FILE__, __LINE__);
    }
    /* album map - add columns */
    const std::string addAlbumMapColumns = "ALTER TABLE " + PhotoMap::TABLE +
        " ADD COLUMN " + PhotoMap::DIRTY +" INT DEFAULT " +
        to_string(static_cast<int32_t>(DirtyTypes::TYPE_NEW)) + ";";
    ret = ExecSqlWithRetry([&]() { return store.ExecuteSql(addAlbumMapColumns); });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("upgrade fail %{public}d: add ablum columns", ret);
        UpdateFail(__FILE__, __LINE__);
    }
}

static void AddCameraShotKey(RdbStore &store)
{
    static const string ADD_CAMERA_SHOT_KEY_ON_PHOTOS = "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE +
        " ADD COLUMN " + PhotoColumn::CAMERA_SHOT_KEY + " TEXT";
    int32_t result = ExecSqlWithRetry([&]() { return store.ExecuteSql(ADD_CAMERA_SHOT_KEY_ON_PHOTOS); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Failed to update PHOTOS");
    }
    result = ExecSqlWithRetry([&]() { return store.ExecuteSql(PhotoUpgrade::INDEX_CAMERA_SHOT_KEY); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Failed to create CAMERA_SHOT_KEY index");
    }
}

void RemoveAlbumCountTrigger(RdbStore &store)
{
    const vector<string> removeAlbumCountTriggers = {
        BaseColumn::DropTrigger() + "update_user_album_count",
        BaseColumn::DropTrigger() + "photo_album_insert_asset",
        BaseColumn::DropTrigger() + "photo_album_delete_asset",
    };
    ExecSqls(removeAlbumCountTriggers, store);
}

void AddExifAndUserComment(RdbStore &store)
{
    const string addUserCommentOnPhotos = "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE +
        " ADD COLUMN " + PhotoColumn::PHOTO_USER_COMMENT + " TEXT";

    const string addAllExifOnPhotos = "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE +
        " ADD COLUMN " + PhotoColumn::PHOTO_ALL_EXIF + " TEXT";

    const vector<string> addExifColumns = { addUserCommentOnPhotos, addAllExifOnPhotos };
    ExecSqls(addExifColumns, store);
}

void AddUpdateCloudSyncTrigger(RdbStore &store)
{
    const vector<string> addUpdateCloudSyncTrigger = { PhotoUpgrade::CREATE_PHOTOS_UPDATE_CLOUD_SYNC };
    ExecSqls(addUpdateCloudSyncTrigger, store);
}

void UpdateYearMonthDayData(RdbStore &store)
{
    MEDIA_DEBUG_LOG("UpdateYearMonthDayData start");
    const vector<string> updateSql = {
        "DROP TRIGGER IF EXISTS naturalbase_rdb_Audios_ON_DELETE",
        "DROP TRIGGER IF EXISTS naturalbase_rdb_Audios_ON_INSERT",
        "DROP TRIGGER IF EXISTS naturalbase_rdb_Audios_ON_UPDATE",
        "DROP TRIGGER IF EXISTS naturalbase_rdb_Files_ON_DELETE",
        "DROP TRIGGER IF EXISTS naturalbase_rdb_Files_ON_INSERT",
        "DROP TRIGGER IF EXISTS naturalbase_rdb_Files_ON_UPDATE",
        "DROP TRIGGER IF EXISTS naturalbase_rdb_Photos_ON_DELETE",
        "DROP TRIGGER IF EXISTS naturalbase_rdb_Photos_ON_INSERT",
        "DROP TRIGGER IF EXISTS naturalbase_rdb_Photos_ON_UPDATE",
        "DROP INDEX IF EXISTS " + PhotoColumn::PHOTO_DATE_YEAR_INDEX,
        "DROP INDEX IF EXISTS " + PhotoColumn::PHOTO_DATE_MONTH_INDEX,
        "DROP INDEX IF EXISTS " + PhotoColumn::PHOTO_DATE_DAY_INDEX,
        "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " +
            PhotoColumn::PHOTO_DATE_YEAR + " = strftime('%Y', datetime(date_added, 'unixepoch', 'localtime')), " +
            PhotoColumn::PHOTO_DATE_MONTH + " = strftime('%Y%m', datetime(date_added, 'unixepoch', 'localtime')), " +
            PhotoColumn::PHOTO_DATE_DAY + " = strftime('%Y%m%d', datetime(date_added, 'unixepoch', 'localtime'))",
        PhotoUpgrade::CREATE_YEAR_INDEX,
        PhotoUpgrade::CREATE_MONTH_INDEX,
        PhotoUpgrade::CREATE_DAY_INDEX,
        PhotoUpgrade::CREATE_SCHPT_MEDIA_TYPE_INDEX,
    };
    ExecSqls(updateSql, store);
    MEDIA_DEBUG_LOG("UpdateYearMonthDayData end");
}

void FixIndexOrder(RdbStore &store)
{
    const vector<string> updateSql = {
        "DROP INDEX IF EXISTS " + PhotoColumn::PHOTO_DATE_YEAR_INDEX,
        "DROP INDEX IF EXISTS " + PhotoColumn::PHOTO_DATE_MONTH_INDEX,
        "DROP INDEX IF EXISTS " + PhotoColumn::PHOTO_DATE_DAY_INDEX,
        "DROP INDEX IF EXISTS idx_media_type",
        "DROP INDEX IF EXISTS idx_sthp_dateadded",
        PhotoUpgrade::CREATE_YEAR_INDEX,
        PhotoUpgrade::CREATE_MONTH_INDEX,
        PhotoUpgrade::CREATE_DAY_INDEX,
        PhotoUpgrade::INDEX_SCTHP_ADDTIME,
        PhotoUpgrade::CREATE_SCHPT_MEDIA_TYPE_INDEX,
        PhotoUpgrade::CREATE_SCHPT_DAY_INDEX,
    };
    ExecSqls(updateSql, store);
}

void AddYearMonthDayColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_DATE_YEAR + " TEXT",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_DATE_MONTH + " TEXT",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_DATE_DAY + " TEXT",
    };
    ExecSqls(sqls, store);
}

void AddCleanFlagAndThumbStatus(RdbStore &store)
{
    const vector<string> addSyncStatus = {
        "DROP INDEX IF EXISTS idx_shpt_date_added",
        "DROP INDEX IF EXISTS idx_shpt_media_type",
        "DROP INDEX IF EXISTS idx_shpt_date_day",
        BaseColumn::AlterTableAddIntColumn(PhotoColumn::PHOTOS_TABLE, PhotoColumn::PHOTO_CLEAN_FLAG),
        BaseColumn::AlterTableAddIntColumn(PhotoColumn::PHOTOS_TABLE, PhotoColumn::PHOTO_THUMB_STATUS),
        PhotoUpgrade::INDEX_SCTHP_ADDTIME,
        PhotoUpgrade::CREATE_SCHPT_MEDIA_TYPE_INDEX,
        PhotoUpgrade::CREATE_SCHPT_DAY_INDEX,
    };
    int32_t result = ExecSqls(addSyncStatus, store);
    CHECK_AND_PRINT_LOG(result == NativeRdb::E_OK,
        "Upgrade rdb need clean and thumb status error %{private}d", result);
}

void AddCloudIndex(RdbStore &store)
{
    const vector<string> sqls = {
        "DROP INDEX IF EXISTS " + PhotoColumn::PHOTO_CLOUD_ID_INDEX,
        PhotoUpgrade::CREATE_CLOUD_ID_INDEX,
    };
    ExecSqls(sqls, store);
}

void AddCompositeDisplayStatusColumn(RdbStore &store, int32_t version)
{
    const string addCompositeDisplayStatusOnPhotos = "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE +
        " ADD COLUMN " + PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS + " INT NOT NULL DEFAULT 0";
    const vector<string> addCompositeDisplayStatus = {addCompositeDisplayStatusOnPhotos};
    ExecSqlsWithDfx(addCompositeDisplayStatus, store, version);
}

static void AddPhotoEditTimeColumn(RdbStore &store)
{
    const string addEditTimeOnPhotos = "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE +
        " ADD COLUMN " + PhotoColumn::PHOTO_EDIT_TIME + " BIGINT DEFAULT 0";
    const vector<string> addEditTime = { addEditTimeOnPhotos };
    ExecSqls(addEditTime, store);
}

void AddShootingModeColumn(RdbStore &store)
{
    const std::string addShootringMode =
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
        PhotoColumn::PHOTO_SHOOTING_MODE + " TEXT";
    const vector<string> addShootingModeColumn = { addShootringMode };
    int32_t result = ExecSqls(addShootingModeColumn, store);
    CHECK_AND_PRINT_LOG(result == NativeRdb::E_OK, "Upgrade rdb shooting_mode error %{private}d", result);
}

void AddShootingModeTagColumn(RdbStore &store)
{
    const std::string addShootringModeTag =
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
        PhotoColumn::PHOTO_SHOOTING_MODE_TAG + " TEXT";
    const std::string dropExpiredClearMapTrigger =
        "DROP TRIGGER IF EXISTS delete_photo_clear_map";
    const vector<string> addShootingModeTagColumn = {addShootringModeTag,
        dropExpiredClearMapTrigger, TriggerDeletePhotoClearMap()};
    int32_t result = ExecSqls(addShootingModeTagColumn, store);
    CHECK_AND_PRINT_LOG(result == NativeRdb::E_OK, "Upgrade rdb shooting_mode error %{private}d", result);
}

static void AddHiddenViewColumn(RdbStore &store)
{
    vector<string> upgradeSqls = {
        BaseColumn::AlterTableAddIntColumn(PhotoAlbumColumns::TABLE, PhotoAlbumColumns::CONTAINS_HIDDEN),
        BaseColumn::AlterTableAddIntColumn(PhotoAlbumColumns::TABLE, PhotoAlbumColumns::HIDDEN_COUNT),
        BaseColumn::AlterTableAddTextColumn(PhotoAlbumColumns::TABLE, PhotoAlbumColumns::HIDDEN_COVER),
    };
    ExecSqls(upgradeSqls, store);
}

static void ModifyMdirtyTriggers(RdbStore &store)
{
    /* drop old mdirty trigger */
    const vector<string> dropMdirtyTriggers = {
        "DROP TRIGGER IF EXISTS photos_mdirty_trigger",
        "DROP TRIGGER IF EXISTS mdirty_trigger",
    };
    if (ExecSqls(dropMdirtyTriggers, store) != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("upgrade fail: drop old mdirty trigger");
    }

    /* create new mdirty trigger */
    if (ExecSqlWithRetry([&]() { return store.ExecuteSql(PhotoUpgrade::CREATE_PHOTOS_MDIRTY_TRIGGER); }) !=
        NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("upgrade fail: create new photos mdirty trigger");
    }

    if (ExecSqlWithRetry([&]() { return store.ExecuteSql(CREATE_FILES_MDIRTY_TRIGGER); }) != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("upgrade fail: create new mdirty trigger");
    }
}

static void AddLastVisitTimeColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + AudioColumn::AUDIOS_TABLE + " DROP time_visit ",
        "ALTER TABLE " + REMOTE_THUMBNAIL_TABLE + " DROP time_visit ",
        "ALTER TABLE " + MEDIALIBRARY_TABLE + " DROP time_visit ",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " DROP time_visit ",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
        PhotoColumn::PHOTO_LAST_VISIT_TIME + " BIGINT DEFAULT 0",
    };
    int32_t result = ExecSqls(sqls, store);
    CHECK_AND_PRINT_LOG(result == NativeRdb::E_OK, "Upgrade rdb last_visit_time error %{private}d", result);
}

void AddHiddenTimeColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE +
            " ADD COLUMN " + PhotoColumn::PHOTO_HIDDEN_TIME + " BIGINT DEFAULT 0",
        PhotoUpgrade::CREATE_HIDDEN_TIME_INDEX,
    };
    ExecSqls(sqls, store);
}

void AddAlbumOrderColumn(RdbStore &store)
{
    const std::string addAlbumOrderColumn =
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
        PhotoAlbumColumns::ALBUM_ORDER + " INT";
    const std::string initOriginOrder =
        "UPDATE " + PhotoAlbumColumns::TABLE + " SET " +
        PhotoAlbumColumns::ALBUM_ORDER + " = rowid";
    const std::string albumDeleteTrigger =
        " CREATE TRIGGER IF NOT EXISTS update_order_trigger AFTER DELETE ON " + PhotoAlbumColumns::TABLE +
        " FOR EACH ROW " +
        " BEGIN " +
        " UPDATE " + PhotoAlbumColumns::TABLE + " SET album_order = album_order - 1" +
        " WHERE album_order > old.album_order;" +
        " END";
    const std::string albumInsertTrigger =
        " CREATE TRIGGER IF NOT EXISTS insert_order_trigger AFTER INSERT ON " + PhotoAlbumColumns::TABLE +
        " BEGIN " +
        " UPDATE " + PhotoAlbumColumns::TABLE + " SET album_order = (" +
        " SELECT COALESCE(MAX(album_order), 0) + 1 FROM " + PhotoAlbumColumns::TABLE +
        ") WHERE rowid = new.rowid;" +
        " END";

    const vector<string> addAlbumOrder = { addAlbumOrderColumn, initOriginOrder,
        albumDeleteTrigger, albumInsertTrigger};
    int32_t result = ExecSqls(addAlbumOrder, store);
    CHECK_AND_PRINT_LOG(result == NativeRdb::E_OK, "Upgrade rdb album order error %{private}d", result);
}

static void AddFormMap(RdbStore &store)
{
    int32_t result = ExecSqlWithRetry([&]() { return store.ExecuteSql(FormMap::CREATE_FORM_MAP_TABLE); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Failed to update PHOTOS");
    }
}

static void FixDocsPath(RdbStore &store)
{
    vector<string> sqls = {
        "UPDATE Files SET "
            " data = REPLACE(data, '/storage/cloud/files/Documents', '/storage/cloud/files/Docs/Documents'),"
            " data = REPLACE(data, '/storage/cloud/files/Download', '/storage/cloud/files/Docs/Download'),"
            " relative_path = REPLACE(relative_path, 'Documents/', 'Docs/Documents/'),"
            " relative_path = REPLACE(relative_path, 'Download/', 'Docs/Download/')"
        " WHERE data LIKE '/storage/cloud/files/Documents%' OR "
            " data LIKE '/storage/cloud/files/Download%' OR"
            " relative_path LIKE 'Documents/%' OR"
            " relative_path LIKE 'Download/%';",
        "UPDATE MediaTypeDirectory SET directory = 'Docs/' WHERE directory_type = 4 OR directory_type = 5",
    };

    ExecSqls(sqls, store);
}

static void AddImageVideoCount(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoAlbumColumns::TABLE +
                " ADD COLUMN " + PhotoAlbumColumns::ALBUM_IMAGE_COUNT + " INT DEFAULT 0",
        "ALTER TABLE " + PhotoAlbumColumns::TABLE +
                " ADD COLUMN " + PhotoAlbumColumns::ALBUM_VIDEO_COUNT + " INT DEFAULT 0",
    };
}

static void AddSCHPTHiddenTimeIndex(RdbStore &store)
{
    const vector<string> sqls = {
        PhotoUpgrade::CREATE_SCHPT_HIDDEN_TIME_INDEX,
    };
    ExecSqls(sqls, store);
}

static void UpdateClassifyDirtyData(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        DROP_TABLE_ANALYSISALBUM,
        DROP_TABLE_ANALYSISPHOTOMAP,
        ALTER_WIDTH_COLUMN,
        ALTER_HEIGHT_COLUMN,
        CREATE_ANALYSIS_ALBUM,
        CREATE_ANALYSIS_ALBUM_MAP,
        CREATE_TAB_IMAGE_FACE,
        CREATE_TAB_FACE_TAG,
        DROP_INSERT_VISION_TRIGGER,
        CREATE_INSERT_VISION_TRIGGER_FOR_ADD_FACE,
        ADD_FACE_STATUS_COLUMN,
        UPDATE_TOTAL_VALUE,
        UPDATE_NOT_SUPPORT_VALUE,
        CREATE_IMAGE_FACE_INDEX
    };
    MEDIA_INFO_LOG("start clear dirty data");
    ExecSqls(executeSqlStrs, store);
}

static void UpdateGeoTables(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE tab_geo_dictionary RENAME TO " +  GEO_DICTIONARY_TABLE,
        "ALTER TABLE tab_geo_knowledge RENAME TO " +  GEO_KNOWLEDGE_TABLE,
        CREATE_DICTIONARY_INDEX,
        DROP_KNOWLEDGE_INDEX,
        CREATE_NEW_KNOWLEDGE_INDEX,
        CREATE_CITY_NAME_INDEX,
        CREATE_LOCATION_KEY_INDEX,
    };
    ExecSqls(sqls, store);
}

static void UpdatePhotosMdirtyTrigger(RdbStore& store)
{
    string dropSql = "DROP TRIGGER IF EXISTS photos_mdirty_trigger";
    if (ExecSqlWithRetry([&]() { return store.ExecuteSql(dropSql); }) != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to drop old photos_mdirty_trigger: %{private}s", dropSql.c_str());
        UpdateFail(__FILE__, __LINE__);
    }

    if (ExecSqlWithRetry([&]() { return store.ExecuteSql(PhotoUpgrade::CREATE_PHOTOS_MDIRTY_TRIGGER); }) !=
        NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Failed to upgrade new photos_mdirty_trigger, %{private}s",
            PhotoUpgrade::CREATE_PHOTOS_MDIRTY_TRIGGER.c_str());
    }
}

static void AddIndexForFileId(RdbStore& store)
{
    const vector<string> sqls = {
        CREATE_IDX_FILEID_FOR_SEARCH_INDEX,
        CREATE_IDX_FILEID_FOR_ANALYSIS_TOTAL,
        CREATE_IDX_FILEID_FOR_ANALYSIS_PHOTO_MAP,
    };
    MEDIA_INFO_LOG("start AddIndexForFileId");
    ExecSqls(sqls, store);
}

void MediaLibraryRdbStore::AddIndexForFileIdAsync(const shared_ptr<MediaLibraryRdbStore> store)
{
    MEDIA_INFO_LOG("AddIndexForFileIdAsync start");
    const vector<string> updateSql = {
        CREATE_IDX_FILEID_FOR_SEARCH_INDEX,
        CREATE_IDX_FILEID_FOR_ANALYSIS_TOTAL,
        CREATE_IDX_FILEID_FOR_ANALYSIS_PHOTO_MAP,
    };
    ExecSqls(updateSql, *store->GetRaw().get());
    MEDIA_INFO_LOG("AddIndexForFileIdAsync end");
}

void MediaLibraryRdbStore::AddIndexForPhotoSortInAlbum(const std::shared_ptr<MediaLibraryRdbStore> store,
    int32_t version)
{
    const std::vector<std::string> sqls = {
        PhotoUpgrade::CREATE_PHOTO_SORT_IN_ALBUM_SIZE_INDEX,
        PhotoUpgrade::CREATE_PHOTO_SORT_MEDIA_TYPE_SIZE_INDEX,
        PhotoUpgrade::CREATE_PHOTO_SORT_IN_ALBUM_DISPLAY_NAME_INDEX,
        PhotoUpgrade::CREATE_PHOTO_SORT_MEDIA_TYPE_DISPLAY_NAME_INDEX,
    };
    MEDIA_INFO_LOG("Start add index for photo sort in album");
    ExecSqlsWithDfx(sqls, *store->GetRaw().get(), version);
    MEDIA_INFO_LOG("End add index for photo sort in album");
}

void MediaLibraryRdbStore::AddIndexForCloudAndPitaya(const std::shared_ptr<MediaLibraryRdbStore> store,
    int32_t version)
{
    const vector<string> sqls = {
        BaseColumn::DropIndex() + PhotoColumn::PHOTO_SCHPT_CLOUD_ENHANCEMENT_ALBUM_INDEX,
        BaseColumn::DropIndex() + PhotoColumn::PHOTO_SCHPT_ADDED_INDEX,
        BaseColumn::DropIndex() + PhotoColumn::PHOTO_SCHPT_ALBUM_INDEX,
        BaseColumn::DropIndex() + PhotoColumn::PHOTO_HIDDEN_TIME_INDEX,
        BaseColumn::DropIndex() + PhotoColumn::PHOTO_DATE_DAY_INDEX,
        BaseColumn::DropIndex() + PhotoColumn::PHOTO_DATE_MONTH_INDEX,
        BaseColumn::DropIndex() + PhotoColumn::PHOTO_DATE_YEAR_INDEX,
        BaseColumn::DropIndex() + PhotoColumn::PHOTO_SCHPT_ALBUM_GENERAL_INDEX,
        BaseColumn::DropIndex() + PhotoColumn::PHOTO_SCHPT_MEDIA_TYPE_COUNT_READY_INDEX,
        BaseColumn::DropIndex() + PhotoColumn::PHOTO_SCHPT_MEDIA_TYPE_INDEX,
        BaseColumn::DropIndex() + PhotoColumn::PHOTO_SORT_MEDIA_TYPE_DATE_ADDED_INDEX,
        BaseColumn::DropIndex() + PhotoColumn::PHOTO_SORT_MEDIA_TYPE_DATE_TAKEN_INDEX,
        BaseColumn::DropIndex() + PhotoColumn::PHOTO_SORT_IN_ALBUM_DATE_TAKEN_INDEX,
        BaseColumn::DropIndex() + PhotoColumn::PHOTO_SORT_IN_ALBUM_DATE_ADDED_INDEX,
        PhotoUpgrade::CREATE_SCHPT_MEDIA_TYPE_INDEX,
        PhotoUpgrade::CREATE_PHOTO_SORT_MEDIA_TYPE_DATE_ADDED_INDEX,
        PhotoUpgrade::CREATE_PHOTO_SORT_MEDIA_TYPE_DATE_TAKEN_INDEX,
        PhotoUpgrade::CREATE_PHOTO_SORT_IN_ALBUM_DATE_ADDED_INDEX,
        PhotoUpgrade::CREATE_PHOTO_SORT_IN_ALBUM_DATE_TAKEN_INDEX,
    };
    MEDIA_INFO_LOG("Start add index for Cloud Enhancement and Pitaya");
    ExecSqlsWithDfx(sqls, *store->GetRaw().get(), version);
    MEDIA_INFO_LOG("End add index for Cloud Enhancement and Pitaya");
}

static void AddMetaRecovery(RdbStore& store)
{
    const vector<string> sqls = {"ALTER TABLE " + PhotoColumn::PHOTOS_TABLE +
        " ADD COLUMN " + PhotoColumn::PHOTO_METADATA_FLAGS + " INT DEFAULT 0",
        PhotoUpgrade::CREATE_PHOTOS_METADATA_DIRTY_TRIGGER,
    };
    MEDIA_INFO_LOG("start AddMetaRecovery");
    ExecSqls(sqls, store);
}

static void AddCloudEnhancementAlbum(RdbStore& store)
{
    ValuesBucket values;
    int32_t err = E_FAIL;
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumType::SYSTEM);
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::CLOUD_ENHANCEMENT);
    values.PutInt(PhotoAlbumColumns::ALBUM_ORDER,
        PhotoAlbumSubType::CLOUD_ENHANCEMENT - PhotoAlbumSubType::SYSTEM_START);

    AbsRdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(PhotoAlbumType::SYSTEM));
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::CLOUD_ENHANCEMENT));

    string sql;
    vector<ValueObject> bindArgs;
    // Build insert sql
    sql.append("INSERT").append(" INTO ").append(PhotoAlbumColumns::TABLE).append(" ");
    MediaLibraryRdbStore::BuildValuesSql(values, bindArgs, sql);
    sql.append(" WHERE NOT EXISTS (");
    MediaLibraryRdbStore::BuildQuerySql(predicates, { PhotoAlbumColumns::ALBUM_ID }, bindArgs, sql);
    sql.append(");");
    err = store.ExecuteSql(sql, bindArgs);
    values.Clear();
    if (err != E_OK) {
        MEDIA_ERR_LOG("Add cloud enhancement album failed, err: %{public}d", err);
    }
}

void AddHighlightTriggerColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::HIGHLIGHT_TABLE + " ADD COLUMN " +
            PhotoColumn::MEDIA_DATA_DB_HIGHLIGHT_TRIGGER + " INT DEFAULT 0"
    };
    MEDIA_INFO_LOG("start add highlight trigger column");
    ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end add highlight trigger column");
}

void AddHighlightInsertAndUpdateTrigger(RdbStore &store)
{
    const vector<string> sqls = {
        PhotoUpgrade::INSERT_GENERATE_HIGHLIGHT_THUMBNAIL,
        PhotoUpgrade::UPDATE_GENERATE_HIGHLIGHT_THUMBNAIL
    };
    MEDIA_INFO_LOG("start add highlight insert and update trigger");
    ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end add highlight insert and update trigger");
}

void AddHighlightIndex(RdbStore &store)
{
    const vector<string> addHighlightIndex = { PhotoUpgrade::INDEX_HIGHLIGHT_FILEID };
    MEDIA_INFO_LOG("start add highlight index");
    ExecSqls(addHighlightIndex, store);
    MEDIA_INFO_LOG("end add highlight index");
}

static void UpdateSearchIndexTriggerForCleanFlag(RdbStore& store)
{
    const vector<string> sqls = {
        "DROP TRIGGER IF EXISTS update_search_status_trigger",
        CREATE_SEARCH_UPDATE_STATUS_TRIGGER,
    };
    MEDIA_INFO_LOG("start update search index for clean flag");
    ExecSqls(sqls, store);
}

static void UpdateAlbumRefreshTable(RdbStore &store)
{
    const vector<string> sqls = {
        CREATE_ALBUM_REFRESH_TABLE,
    };
    ExecSqls(sqls, store);
}

static void AddCoverPlayVersionColumns(RdbStore& store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + HIGHLIGHT_COVER_INFO_TABLE +
            " ADD COLUMN " + COVER_SERVICE_VERSION + " INT DEFAULT 0",
        "ALTER TABLE " + HIGHLIGHT_PLAY_INFO_TABLE +
            " ADD COLUMN " + PLAY_SERVICE_VERSION + " INT DEFAULT 0",
    };
    MEDIA_INFO_LOG("start add cover play version columns");
    ExecSqls(sqls, store);
}

static void AddMovingPhotoRelatedData(RdbStore &store)
{
    const vector<string> sqls = {
        CREATE_TAB_VIDEO_ANALYSIS_AESTHETICS,
    };
    MEDIA_INFO_LOG("start create video aesthetics score table");
    ExecSqls(sqls, store);
}

static void UpdateFavoriteIndex(RdbStore &store)
{
    MEDIA_INFO_LOG("Upgrade rdb UpdateFavoriteIndex");
    const vector<string> sqls = {
        PhotoUpgrade::CREATE_PHOTO_FAVORITE_INDEX,
        PhotoUpgrade::DROP_SCHPT_MEDIA_TYPE_INDEX,
        PhotoUpgrade::CREATE_SCHPT_MEDIA_TYPE_INDEX,
    };
    ExecSqls(sqls, store);
}

static void AddMissingUpdates(RdbStore &store)
{
    MEDIA_INFO_LOG("start add missing updates");
    vector<string> sqls;
    bool hasShootingModeTag = MediaLibraryRdbStore::HasColumnInTable(store, PhotoColumn::PHOTO_SHOOTING_MODE_TAG,
        PhotoColumn::PHOTOS_TABLE);
    if (!hasShootingModeTag) {
        MEDIA_INFO_LOG("start add shooting mode tag");
        const vector<string> sqls = {
            "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_SHOOTING_MODE_TAG +
                " TEXT",
        };
        ExecSqls(sqls, store);
    }
    bool hasBundleName = MediaLibraryRdbStore::HasColumnInTable(store, PhotoAlbumColumns::ALBUM_BUNDLE_NAME,
        PhotoAlbumColumns::TABLE);
    bool hasLocalLanguage = MediaLibraryRdbStore::HasColumnInTable(store, PhotoAlbumColumns::ALBUM_LOCAL_LANGUAGE,
        PhotoAlbumColumns::TABLE);
    if (!hasBundleName) {
        MoveSourceAlbumToPhotoAlbumAndAddColumns(store);
        ModifySourceAlbumTriggers(store);
    } else if (!hasLocalLanguage) {
        ModifySourceAlbumTriggers(store);
    } else {
        MEDIA_INFO_LOG("both columns exist, no need to start source album related updates");
    }
    MEDIA_INFO_LOG("start add cloud index");
    AddCloudIndex(store);
    MEDIA_INFO_LOG("start update photos mdirty trigger");
    UpdatePhotosMdirtyTrigger(store);
    MEDIA_INFO_LOG("end add missing updates");
}

void AddMultiStagesCaptureColumns(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_ID + " TEXT",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_QUALITY + " INT",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_FIRST_VISIT_TIME +
            " BIGINT DEFAULT 0",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_DEFERRED_PROC_TYPE +
            " INT DEFAULT 0",
    };
    ExecSqls(sqls, store);
}

static void CreateBackupInfoTable(RdbStore& store, int32_t version)
{
    MEDIA_INFO_LOG("create table ConfigInfo start");
    ExecSqlsWithDfx({ConfigInfoColumn::CREATE_CONFIG_INFO_TABLE}, store, version);
    MEDIA_INFO_LOG("create table ConfigInfo end");
}

void UpdateMillisecondDate(RdbStore &store)
{
    MEDIA_DEBUG_LOG("UpdateMillisecondDate start");
    const vector<string> updateSql = {
        "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " +
        MediaColumn::MEDIA_DATE_ADDED + " = " + MediaColumn::MEDIA_DATE_ADDED + "*1000," +
        MediaColumn::MEDIA_DATE_MODIFIED + " = " + MediaColumn::MEDIA_DATE_MODIFIED + "*1000," +
        MediaColumn::MEDIA_DATE_TRASHED + " = " + MediaColumn::MEDIA_DATE_TRASHED + "*1000;",
        "UPDATE " + AudioColumn::AUDIOS_TABLE + " SET " +
        MediaColumn::MEDIA_DATE_ADDED + " = " + MediaColumn::MEDIA_DATE_ADDED + "*1000," +
        MediaColumn::MEDIA_DATE_MODIFIED + " = " + MediaColumn::MEDIA_DATE_MODIFIED + "*1000," +
        MediaColumn::MEDIA_DATE_TRASHED + " = " + MediaColumn::MEDIA_DATE_TRASHED + "*1000;",
        "UPDATE " + PhotoAlbumColumns::TABLE + " SET " +
        MediaColumn::MEDIA_DATE_MODIFIED + " = " +  MediaColumn::MEDIA_DATE_MODIFIED + "*1000;",
        "UPDATE " + MEDIALIBRARY_TABLE + " SET " +
        MediaColumn::MEDIA_DATE_ADDED + " = " + MediaColumn::MEDIA_DATE_ADDED + "*1000," +
        MediaColumn::MEDIA_DATE_MODIFIED + " = " + MediaColumn::MEDIA_DATE_MODIFIED + "*1000;",
    };
    ExecSqls(updateSql, store);
    MEDIA_DEBUG_LOG("UpdateMillisecondDate end");
}

void AddHasAstcColumns(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_HAS_ASTC + " INT DEFAULT 0 ",
    };
    ExecSqls(sqls, store);
}

void AddAddressDescriptionColumns(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + GEO_KNOWLEDGE_TABLE + " ADD COLUMN " + CITY_NAME + " TEXT",
        "ALTER TABLE " + GEO_KNOWLEDGE_TABLE + " ADD COLUMN " + ADDRESS_DESCRIPTION + " TEXT",
    };
    ExecSqls(sqls, store);
}

void AddIsLocalAlbum(RdbStore &store)
{
    const vector<string> sqls = {
        ADD_IS_LOCAL_COLUMN_FOR_ALBUM,
        ADD_PHOTO_ALBUM_IS_LOCAL,
    };
    MEDIA_INFO_LOG("start add islocal column");
    ExecSqls(sqls, store);
}

void AddSourceAndTargetTokenForUriPermission(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + AppUriPermissionColumn::APP_URI_PERMISSION_TABLE + " ADD COLUMN " +
            AppUriPermissionColumn::SOURCE_TOKENID + " BIGINT",
        "ALTER TABLE " + AppUriPermissionColumn::APP_URI_PERMISSION_TABLE + " ADD COLUMN " +
            AppUriPermissionColumn::TARGET_TOKENID + " BIGINT",
        AppUriPermissionColumn::DROP_URI_URITYPE_APPID_INDEX,
        AppUriPermissionColumn::CREATE_URI_URITYPE_TOKENID_INDEX,
    };
    MEDIA_INFO_LOG("start add islocal column");
    ExecSqls(sqls, store);
}

void UpdateAOI(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + GEO_KNOWLEDGE_TABLE + " ADD COLUMN " + AOI + " TEXT ",
        "ALTER TABLE " + GEO_KNOWLEDGE_TABLE + " ADD COLUMN " + POI + " TEXT ",
        "ALTER TABLE " + GEO_KNOWLEDGE_TABLE + " ADD COLUMN " + FIRST_AOI + " TEXT ",
        "ALTER TABLE " + GEO_KNOWLEDGE_TABLE + " ADD COLUMN " + FIRST_POI + " TEXT ",
        "ALTER TABLE " + GEO_KNOWLEDGE_TABLE + " ADD COLUMN " + LOCATION_VERSION + " TEXT ",
        "ALTER TABLE " + GEO_KNOWLEDGE_TABLE + " ADD COLUMN " + FIRST_AOI_CATEGORY + " TEXT ",
        "ALTER TABLE " + GEO_KNOWLEDGE_TABLE + " ADD COLUMN " + FIRST_POI_CATEGORY + " TEXT ",
        "ALTER TABLE " + GEO_KNOWLEDGE_TABLE + " ADD COLUMN " + FILE_ID + " INT ",
        DROP_KNOWLEDGE_INDEX,
        CREATE_NEW_KNOWLEDGE_INDEX,
    };
    MEDIA_INFO_LOG("start init aoi info of geo db");
    ExecSqls(sqls, store);
}

void UpdateVideoFaceTable(RdbStore &store)
{
    const vector<string> sqls = {
        "DROP TABLE IF EXISTS " + VISION_VIDEO_FACE_TABLE,
        CREATE_TAB_VIDEO_FACE,
    };
    MEDIA_INFO_LOG("start update video face db");
    ExecSqls(sqls, store);
}

void AddHighlightChangeFunction(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + ANALYSIS_PHOTO_MAP_TABLE + " ADD COLUMN " + ORDER_POSITION + " INT ",
        "ALTER TABLE " + HIGHLIGHT_COVER_INFO_TABLE + " ADD COLUMN " + COVER_STATUS + " INT ",
        "ALTER TABLE " + HIGHLIGHT_PLAY_INFO_TABLE + " ADD COLUMN " + PLAY_INFO_STATUS + " INT ",
        "ALTER TABLE " + HIGHLIGHT_ALBUM_TABLE + " ADD COLUMN " + HIGHLIGHT_PIN_TIME + " BIGINT ",
    };
    MEDIA_INFO_LOG("start add highlight change function");
    ExecSqls(sqls, store);
}

static void AddHighlightViewedNotification(RdbStore &store, int32_t version)
{
    MEDIA_INFO_LOG("start AddHighlightViewedNotification");
    const vector<string> sqls = {
        "ALTER TABLE " + HIGHLIGHT_ALBUM_TABLE + " ADD COLUMN " +
            HIGHLIGHT_IS_VIEWED + " BOOL NOT NULL DEFAULT 0 ",
        "ALTER TABLE " + HIGHLIGHT_ALBUM_TABLE + " ADD COLUMN " +
            HIGHLIGHT_NOTIFICATION_TIME + " BIGINT NOT NULL DEFAULT 0 ",
    };
    ExecSqlsWithDfx(sqls, store, version);
    MEDIA_INFO_LOG("end AddHighlightViewedNotification");
}

void AddAestheticsScoreFileds(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + VISION_AESTHETICS_TABLE + " ADD COLUMN " + AESTHETICS_ALL_VERSION + " TEXT ",
        "ALTER TABLE " + VISION_AESTHETICS_TABLE + " ADD COLUMN " + AESTHETICS_SCORE_ALL + " INT NOT NULL DEFAULT 0 ",
        "ALTER TABLE " + VISION_AESTHETICS_TABLE + " ADD COLUMN " + IS_FILTERED_HARD + " BOOLEAN NOT NULL DEFAULT 0 ",
        "ALTER TABLE " + VISION_AESTHETICS_TABLE + " ADD COLUMN " + CLARITY_SCORE_ALL + " DOUBLE NOT NULL DEFAULT 0 ",
        "ALTER TABLE " + VISION_AESTHETICS_TABLE + " ADD COLUMN " +
            SATURATION_SCORE_ALL + " DOUBLE NOT NULL DEFAULT 0 ",
        "ALTER TABLE " + VISION_AESTHETICS_TABLE + " ADD COLUMN " + LUMINANCE_SCORE_ALL + " DOUBLE NOT NULL DEFAULT 0 ",
        "ALTER TABLE " + VISION_AESTHETICS_TABLE + " ADD COLUMN " + SEMANTICS_SCORE + " DOUBLE NOT NULL DEFAULT 0 ",
        "ALTER TABLE " + VISION_AESTHETICS_TABLE + " ADD COLUMN " +
            IS_BLACK_WHITE_STRIPE + " BOOLEAN NOT NULL DEFAULT 0 ",
        "ALTER TABLE " + VISION_AESTHETICS_TABLE + " ADD COLUMN " + IS_BLURRY + " BOOLEAN NOT NULL DEFAULT 0 ",
        "ALTER TABLE " + VISION_AESTHETICS_TABLE + " ADD COLUMN " + IS_MOSAIC + " BOOLEAN NOT NULL DEFAULT 0 ",
        "ALTER TABLE " + VISION_TOTAL_TABLE + " ADD COLUMN " + AESTHETICS_SCORE_ALL_STATUS + " INT NOT NULL DEFAULT 0 ",
    };
    MEDIA_INFO_LOG("start add aesthetics score fields");
    ExecSqls(sqls, store);
}

static void AddAlbumsOrderKeysColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
            PhotoAlbumColumns::ALBUMS_ORDER + " INT NOT NULL DEFAULT -1",
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
            PhotoAlbumColumns::ORDER_SECTION + " INT NOT NULL DEFAULT -1",
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
            PhotoAlbumColumns::ORDER_TYPE + " INT NOT NULL DEFAULT -1",
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
            PhotoAlbumColumns::ORDER_STATUS + " INT NOT NULL DEFAULT 0",
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
            PhotoAlbumColumns::STYLE2_ALBUMS_ORDER + " INT NOT NULL DEFAULT -1",
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
            PhotoAlbumColumns::STYLE2_ORDER_SECTION + " INT NOT NULL DEFAULT -1",
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
            PhotoAlbumColumns::STYLE2_ORDER_TYPE + " INT NOT NULL DEFAULT -1",
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
            PhotoAlbumColumns::STYLE2_ORDER_STATUS + " INT NOT NULL DEFAULT 0",
    };
    MEDIA_INFO_LOG("Add album order keys column start");
    ExecSqls(sqls, store);
}

void AddStoryTables(RdbStore &store)
{
    const vector<string> executeSqlStrs = {
        CREATE_HIGHLIGHT_ALBUM_TABLE,
        CREATE_HIGHLIGHT_COVER_INFO_TABLE,
        CREATE_HIGHLIGHT_PLAY_INFO_TABLE,
        CREATE_USER_PHOTOGRAPHY_INFO_TABLE,
        "ALTER TABLE " + VISION_LABEL_TABLE + " ADD COLUMN " + SALIENCY_SUB_PROB + " TEXT",
    };
    MEDIA_INFO_LOG("start init story db");
    ExecSqls(executeSqlStrs, store);
}

void UpdateAnalysisTables(RdbStore &store)
{
    const vector<string> executeSqlStrs = {
        "ALTER TABLE " + VISION_OCR_TABLE + " ADD COLUMN " + ANALYSIS_VERSION + " TEXT ",
        "ALTER TABLE " + VISION_LABEL_TABLE + " ADD COLUMN " + ANALYSIS_VERSION + " TEXT ",
        "ALTER TABLE " + VISION_VIDEO_LABEL_TABLE + " ADD COLUMN " + ANALYSIS_VERSION + " TEXT ",
        "ALTER TABLE " + VISION_AESTHETICS_TABLE + " ADD COLUMN " + ANALYSIS_VERSION + " TEXT ",
        "ALTER TABLE " + VISION_SALIENCY_TABLE + " ADD COLUMN " + ANALYSIS_VERSION + " TEXT ",
        "ALTER TABLE " + VISION_OBJECT_TABLE + " ADD COLUMN " + SCALE_X + " REAL DEFAULT 0 ",
        "ALTER TABLE " + VISION_OBJECT_TABLE + " ADD COLUMN " + SCALE_Y + " REAL DEFAULT 0 ",
        "ALTER TABLE " + VISION_OBJECT_TABLE + " ADD COLUMN " + SCALE_WIDTH + " REAL DEFAULT 0 ",
        "ALTER TABLE " + VISION_OBJECT_TABLE + " ADD COLUMN " + SCALE_HEIGHT + " REAL DEFAULT 0 ",
        "ALTER TABLE " + VISION_OBJECT_TABLE + " ADD COLUMN " + ANALYSIS_VERSION + " TEXT ",
        "ALTER TABLE " + VISION_RECOMMENDATION_TABLE + " ADD COLUMN " + SCALE_X + " REAL DEFAULT 0 ",
        "ALTER TABLE " + VISION_RECOMMENDATION_TABLE + " ADD COLUMN " + SCALE_Y + " REAL DEFAULT 0 ",
        "ALTER TABLE " + VISION_RECOMMENDATION_TABLE + " ADD COLUMN " + SCALE_WIDTH + " REAL DEFAULT 0 ",
        "ALTER TABLE " + VISION_RECOMMENDATION_TABLE + " ADD COLUMN " + SCALE_HEIGHT + " REAL DEFAULT 0 ",
        "ALTER TABLE " + VISION_RECOMMENDATION_TABLE + " ADD COLUMN " + ANALYSIS_VERSION + " TEXT ",
        "ALTER TABLE " + VISION_SEGMENTATION_TABLE + " ADD COLUMN " + ANALYSIS_VERSION + " TEXT ",
        "ALTER TABLE " + VISION_COMPOSITION_TABLE + " ADD COLUMN " + SCALE_X + " REAL DEFAULT 0 ",
        "ALTER TABLE " + VISION_COMPOSITION_TABLE + " ADD COLUMN " + SCALE_Y + " REAL DEFAULT 0 ",
        "ALTER TABLE " + VISION_COMPOSITION_TABLE + " ADD COLUMN " + SCALE_WIDTH + " REAL DEFAULT 0 ",
        "ALTER TABLE " + VISION_COMPOSITION_TABLE + " ADD COLUMN " + SCALE_HEIGHT + " REAL DEFAULT 0 ",
        "ALTER TABLE " + VISION_COMPOSITION_TABLE + " ADD COLUMN " + ANALYSIS_VERSION + " TEXT ",
        "ALTER TABLE " + VISION_HEAD_TABLE + " ADD COLUMN " + SCALE_X + " REAL DEFAULT 0 ",
        "ALTER TABLE " + VISION_HEAD_TABLE + " ADD COLUMN " + SCALE_Y + " REAL DEFAULT 0 ",
        "ALTER TABLE " + VISION_HEAD_TABLE + " ADD COLUMN " + SCALE_WIDTH + " REAL DEFAULT 0 ",
        "ALTER TABLE " + VISION_HEAD_TABLE + " ADD COLUMN " + SCALE_HEIGHT + " REAL DEFAULT 0 ",
        "ALTER TABLE " + VISION_HEAD_TABLE + " ADD COLUMN " + ANALYSIS_VERSION + " TEXT ",
        "ALTER TABLE " + VISION_POSE_TABLE + " ADD COLUMN " + SCALE_X + " REAL DEFAULT 0 ",
        "ALTER TABLE " + VISION_POSE_TABLE + " ADD COLUMN " + SCALE_Y + " REAL DEFAULT 0 ",
        "ALTER TABLE " + VISION_POSE_TABLE + " ADD COLUMN " + SCALE_WIDTH + " REAL DEFAULT 0 ",
        "ALTER TABLE " + VISION_POSE_TABLE + " ADD COLUMN " + SCALE_HEIGHT + " REAL DEFAULT 0 ",
        "ALTER TABLE " + VISION_POSE_TABLE + " ADD COLUMN " + ANALYSIS_VERSION + " TEXT ",
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + ANALYSIS_VERSION + " TEXT ",
        "ALTER TABLE " + VISION_FACE_TAG_TABLE + " ADD COLUMN " + ANALYSIS_VERSION + " TEXT ",
    };
    MEDIA_INFO_LOG("update analysis tables of db");
    ExecSqls(executeSqlStrs, store);
}

void UpdateHighlightTables(RdbStore &store)
{
    const vector<string> executeSqlStrs = {
        "DROP TABLE IF EXISTS tab_story_album",
        "DROP TABLE IF EXISTS tab_story_cover_info",
        "DROP TABLE IF EXISTS tab_story_play_info",
        CREATE_HIGHLIGHT_ALBUM_TABLE,
        CREATE_HIGHLIGHT_COVER_INFO_TABLE,
        CREATE_HIGHLIGHT_PLAY_INFO_TABLE,
        "ALTER TABLE " + GEO_KNOWLEDGE_TABLE + " ADD COLUMN " + LOCATION_TYPE + " TEXT",
    };
    MEDIA_INFO_LOG("update highlight db");
    ExecSqls(executeSqlStrs, store);
}

void UpdateHighlightCoverTables(RdbStore &store)
{
    const vector<string> executeSqlStrs = {
        "DROP TABLE IF EXISTS tab_highlight_cover_info",
        CREATE_HIGHLIGHT_COVER_INFO_TABLE,
    };
    MEDIA_INFO_LOG("update highlight cover db");
    ExecSqls(executeSqlStrs, store);
}

void UpdateHighlightTablePrimaryKey(RdbStore &store)
{
    const vector<string> executeSqlStrs = {
        "DROP TABLE IF EXISTS tab_highlight_album",
        "DROP TABLE IF EXISTS tab_highlight_cover_info",
        CREATE_HIGHLIGHT_ALBUM_TABLE,
        CREATE_HIGHLIGHT_COVER_INFO_TABLE,
    };
    MEDIA_INFO_LOG("update primary key of highlight db");
    ExecSqls(executeSqlStrs, store);
}

void AddBussinessRecordAlbum(RdbStore &store)
{
    string updateDirtyForShootingMode = "UPDATE Photos SET dirty = 2 WHERE position <> 1 AND " +
        PhotoColumn::PHOTO_SHOOTING_MODE + " is not null AND " +
        PhotoColumn::PHOTO_SHOOTING_MODE + " != ''";
    const vector<string> sqls = {
        MedialibraryBusinessRecordColumn::CREATE_TABLE,
        MedialibraryBusinessRecordColumn::CREATE_BUSINESS_KEY_INDEX,
        updateDirtyForShootingMode,
    };

    MEDIA_INFO_LOG("start add bussiness record album");
    ExecSqls(sqls, store);
    UpdatePhotosMdirtyTrigger(store);
}

void AddIsCoverSatisfiedColumn(RdbStore &store)
{
    const vector<string> sqls = {
        ADD_IS_COVER_SATISFIED_FOR_ALBUM,
    };
    ExecSqls(sqls, store);
}

void AddOwnerAppId(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + MediaColumn::MEDIA_OWNER_APPID + " TEXT",
        "ALTER TABLE " + AudioColumn::AUDIOS_TABLE + " ADD COLUMN " + MediaColumn::MEDIA_OWNER_APPID + " TEXT"
    };
    MEDIA_INFO_LOG("start add owner_appid column");
    ExecSqls(sqls, store);
}

void UpdateThumbnailReadyColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " RENAME COLUMN " + PhotoColumn::PHOTO_HAS_ASTC
            + " TO " + PhotoColumn::PHOTO_THUMBNAIL_READY,
    };
    MEDIA_INFO_LOG("update has_astc to thumbnail_ready begin");
    ExecSqls(sqls, store);
    MEDIA_INFO_LOG("update has_astc to thumbnail_ready finished");
}

void AddOwnerAppIdToFiles(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + MEDIALIBRARY_TABLE + " ADD COLUMN " + MediaColumn::MEDIA_OWNER_APPID + " TEXT"
    };
    MEDIA_INFO_LOG("start add owner_appid column to files table");
    ExecSqls(sqls, store);
    MEDIA_INFO_LOG("add owner_appid column to files table finished");
}

void AddDynamicRangeType(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE + " INT DEFAULT 0"
    };
    MEDIA_INFO_LOG("start add dynamic_range_type column");
    ExecSqls(sqls, store);
}

void AddHdrMode(RdbStore &store, int32_t version)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_HDR_MODE + " INT DEFAULT 0 NOT NULL"
    };
    MEDIA_INFO_LOG("start add hdr_mode column");
    ExecSqlsWithDfx(sqls, store, version);
}

void AddCriticalTypeColumns(RdbStore &store, int32_t version)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_IS_CRITICAL + " INT DEFAULT 0 NOT NULL"
    };

    MEDIA_INFO_LOG("add Photos is_critical column starts");
    int32_t ret = ExecSqlsWithDfx(sqls, store, version);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to execute SQLs, error code: %d", ret);
        return;
    }
    MEDIA_INFO_LOG("add Photos is_critical column ends");
}

void AddPhotoRiskStatusColumnsAndDeleteCritical(RdbStore &store, int32_t version)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_RISK_STATUS + " INT DEFAULT 0 NOT NULL"
    };

    MEDIA_INFO_LOG("add Photos photo_risk_status column starts");
    int32_t ret = ExecSqlsWithDfx(sqls, store, version);
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK, "Failed to execute SQLs, error code: %d", ret);
    MEDIA_INFO_LOG("add Photos photo_risk_status column ends");

    const vector<string> sqlsDrop = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " DROP COLUMN " + PhotoColumn::PHOTO_CRITICAL_TYPE
    };

    MEDIA_INFO_LOG("drop Photos critical_type column starts");
    ret = ExecSqlsWithDfx(sqlsDrop, store, version);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Failed to execute SQLs, error code: %d", ret);
    MEDIA_INFO_LOG("drop Photos critical_type column ends");
}

void AddLcdAndThumbSizeColumns(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_LCD_SIZE + " TEXT",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_THUMB_SIZE + " TEXT",
    };
    ExecSqls(sqls, store);
}

void UpdatePhotoAlbumTigger(RdbStore &store, int32_t version)
{
    static const vector<string> executeSqlStrs = {
        "DROP TRIGGER IF EXISTS album_modify_trigger",
        PhotoAlbumColumns::CREATE_ALBUM_MDIRTY_TRIGGER,
    };
    MEDIA_INFO_LOG("Start update album modify trigger");
    ExecSqlsWithDfx(executeSqlStrs, store, version);
}

static void UpdateAnalysisAlbumRelationship(RdbStore &store, int32_t version)
{
    const string addRelationColumn = "ALTER TABLE " + ANALYSIS_ALBUM_TABLE + " ADD COLUMN " +
        ALBUM_RELATIONSHIP + " TEXT(64) ";
    static const vector<string> executeSqlStrs = {
        addRelationColumn,
        "DROP TRIGGER IF EXISTS " + ANALYSIS_ALBUM_UPDATE_SEARCH_TRIGGER,
        CREATE_ANALYSIS_ALBUM_UPDATE_SEARCH_TRIGGER,
    };
    MEDIA_INFO_LOG("Start update album modify trigger");
    ExecSqlsWithDfx(executeSqlStrs, store, version);
}

static void AddMovingPhotoEffectMode(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::MOVING_PHOTO_EFFECT_MODE + " INT DEFAULT 0"
    };
    MEDIA_INFO_LOG("start add moving_photo_effect_mode column");
    ExecSqls(sqls, store);
}

void AddBurstCoverLevelAndBurstKey(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_BURST_COVER_LEVEL +
            " INT DEFAULT 1",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_BURST_KEY + " TEXT",
    };
    MEDIA_INFO_LOG("start add burst_cover_level and burst_key column");
    ExecSqls(sqls, store);
}

static void AddCloudEnhancementColumns(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_CE_AVAILABLE + " INT DEFAULT 0",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_CE_STATUS_CODE + " INT ",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_STRONG_ASSOCIATION + " INT DEFAULT 0",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_ASSOCIATE_FILE_ID + " INT DEFAULT 0",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_HAS_CLOUD_WATERMARK + " INT DEFAULT 0",
    };
    MEDIA_INFO_LOG("start add cloud enhancement columns");
    ExecSqls(sqls, store);
}

static void AddAnalysisStatus(RdbStore &store, int32_t version)
{
    static const vector<string> executeSqlStrs = {
        "ALTER TABLE " + ANALYSIS_ALBUM_TABLE = " ADD COLUNN " + ANALYSIS_STATUS + " INT ",
    };
    MEDIA_INFO_LOG("start add analysis status column");
    ExecSqlsWithDfx(executeSqlStrs, store, version);
}

static void AddIsAutoColumns(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_IS_AUTO + " INT DEFAULT 0 NOT NULL",
    };
    MEDIA_INFO_LOG("start add is_auto column for auto cloud enhancement");
    ExecSqls(sqls, store);
}

static void AddThumbnailReady(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_THUMBNAIL_READY + " INT DEFAULT 0",
    };
    MEDIA_INFO_LOG("start add thumbnail ready columns");
    ExecSqls(sqls, store);
}

static void AddCheckFlag(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_CHECK_FLAG + " INT DEFAULT 0",
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
            PhotoAlbumColumns::ALBUM_CHECK_FLAG + " INT DEFAULT 0",
    };
    MEDIA_INFO_LOG("start add check_flag columns");
    ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end add check_flag columns");
}

static bool IsColumnExists(RdbStore &store, const std::string& tableName,
    const std::string& columnName)
{
    std::string checkSql = "PRAGMA table_info(" + tableName + ")";
    std::vector<NativeRdb::ValueObject> args;
    auto resultSet = store.QuerySql(checkSql, args);
    CHECK_AND_RETURN_RET(resultSet != nullptr, false);

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string name;
        resultSet->GetString(1, name);
        CHECK_AND_RETURN_RET(name != columnName, true);
    }

    return false;
}

static void CheckIfPhotoColumnExists(RdbStore &store, unordered_map<string, bool> &photoColumnExists)
{
    std::string checkSql = "PRAGMA table_info(" + PhotoColumn::PHOTOS_TABLE + ")";
    std::vector<NativeRdb::ValueObject> args;
    auto resultSet = store.QuerySql(checkSql, args);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Failed to query %{private}s", checkSql.c_str());

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string name;
        resultSet->GetString(1, name);
        if (photoColumnExists.find(name) != photoColumnExists.end()) {
            photoColumnExists[name] = true;
        }
    }
}

static void AddCloudEnhanceColumnsFix(RdbStore& store)
{
    MEDIA_INFO_LOG("Start checking cloud enhancement column");
    bool hasColumn = IsColumnExists(store, PhotoColumn::PHOTOS_TABLE, PhotoColumn::PHOTO_CE_AVAILABLE);
    MEDIA_INFO_LOG("End checking cloud enhancement column: %{public}d", hasColumn);
    if (!hasColumn) {
        AddCloudEnhancementColumns(store);
        MEDIA_INFO_LOG("Add Cloud Enhance Cols completed successfully");
    }
}

static void AddDynamicRangeColumnsFix(RdbStore& store)
{
    MEDIA_INFO_LOG("Start checking dynamic_range_type column");
    bool hasColumn = IsColumnExists(store, PhotoColumn::PHOTOS_TABLE, PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE);
    MEDIA_INFO_LOG("End checking dynamic_range_type column: %{public}d", hasColumn);
    if (!hasColumn) {
        AddDynamicRangeType(store);
        MEDIA_INFO_LOG("Add Dynamic Range Cols completed successfully");
    }
}

static void AddThumbnailReadyColumnsFix(RdbStore& store)
{
    MEDIA_INFO_LOG("Start checking thumbnail_ready column");
    bool hasColumn = IsColumnExists(store, PhotoColumn::PHOTOS_TABLE, PhotoColumn::PHOTO_THUMBNAIL_READY);
    MEDIA_INFO_LOG("End checking thumbnail_ready column: %{public}d", hasColumn);
    if (!hasColumn) {
        AddThumbnailReady(store);
        MEDIA_INFO_LOG("Add ThumbnailReady Column");
    }
}

static void UpdateSourcePhotoAlbumTrigger(RdbStore &store)
{
    MEDIA_INFO_LOG("start update source photo album trigger");
    const vector<string> sqls = {
        DROP_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER,
        CREATE_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER,
    };
    ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end update source photo album trigger");
}

static void UpdateSearchStatusTriggerForOwnerAlbumId(RdbStore &store)
{
    MEDIA_INFO_LOG("start update search status trigger for owner album id");
    const vector<string> sqls = {
        "DROP TRIGGER IF EXISTS " + UPDATE_SEARCH_STATUS_TRIGGER,
        CREATE_SEARCH_UPDATE_STATUS_TRIGGER,
    };
    ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end update search status trigger for owner album id");
}

static void UpdateSearchStatusTriggerForIsFavorite(RdbStore &store)
{
    MEDIA_INFO_LOG("start update search status trigger for is favorite");
    const vector<string> sqls = {
        "DROP TRIGGER IF EXISTS " + UPDATE_SEARCH_STATUS_TRIGGER,
        CREATE_SEARCH_UPDATE_STATUS_TRIGGER,
    };
    ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end update search status trigger for is favorite");
}

static void AddHighlightAnalysisProgress(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + USER_PHOTOGRAPHY_INFO_TABLE + " ADD COLUMN " + HIGHLIGHT_ANALYSIS_PROGRESS + " TEXT"
    };
    MEDIA_INFO_LOG("start add highlight_analysis_progress column");
    ExecSqls(sqls, store);
}

static void AddRefreshAlbumStatusColumn(RdbStore &store)
{
    MEDIA_INFO_LOG("start add status column for refresh album table");
    const vector<string> sqls = {
        "ALTER TABLE " + ALBUM_REFRESH_TABLE + " ADD COLUMN " +
            ALBUM_REFRESH_STATUS + " INT DEFAULT 0 NOT NULL"
    };
    ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end add status column for refresh album table");
}

static void UpdateCloudTrigger(RdbStore &store)
{
    MEDIA_INFO_LOG("start update cloud trigger");
    const vector<string> sqls = {
        "DROP TRIGGER IF EXISTS photos_delete_trigger",
        PhotoUpgrade::CREATE_PHOTOS_DELETE_TRIGGER,
        "DROP TRIGGER IF EXISTS photos_fdirty_trigger",
        PhotoUpgrade::CREATE_PHOTOS_FDIRTY_TRIGGER,
        "DROP TRIGGER IF EXISTS photos_mdirty_trigger",
        PhotoUpgrade::CREATE_PHOTOS_MDIRTY_TRIGGER,
        "DROP TRIGGER IF EXISTS delete_trigger",
        CREATE_FILES_DELETE_TRIGGER,
        "DROP TRIGGER IF EXISTS fdirty_trigger",
        CREATE_FILES_FDIRTY_TRIGGER,
        "DROP TRIGGER IF EXISTS mdirty_trigger",
        CREATE_FILES_MDIRTY_TRIGGER,
    };
    ExecSqls(sqls, store);
}

static void AddSupportedWatermarkType(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::SUPPORTED_WATERMARK_TYPE + " INT "
    };
    MEDIA_INFO_LOG("start add supported_watermark_type column");
    ExecSqls(sqls, store);
}

static void AddStageVideoTaskStatus(RdbStore &store)
{
    MEDIA_INFO_LOG("start add stage_video_task_status column");
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::STAGE_VIDEO_TASK_STATUS + " INT NOT NULL DEFAULT 0 "
    };
    ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end add stage_video_task_status column");
}

static void AddHighlightUseSubtitle(RdbStore &store)
{
    MEDIA_INFO_LOG("start add use_subtitle column");
    const vector<string> sqls = {
        "ALTER TABLE " + HIGHLIGHT_ALBUM_TABLE + " ADD COLUMN " +
            HIGHLIGHT_USE_SUBTITLE + " INT DEFAULT 0"
    };
    ExecSqls(sqls, store);
    MEDIA_INFO_LOG("start add use_subtitle column");
}

static void UpdateVisionTriggerForVideoLabel(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        DROP_UPDATE_VISION_TRIGGER,
        CREATE_VISION_UPDATE_TRIGGER_FOR_ADD_VIDEO_LABEL,
    };
    MEDIA_INFO_LOG("start update vision trigger for video label");
    ExecSqls(executeSqlStrs, store);
}

static void UpdateIndexForAlbumQuery(RdbStore &store)
{
    const vector<string> sqls = {
        PhotoUpgrade::DROP_INDEX_SCTHP_ADDTIME,
        PhotoUpgrade::INDEX_SCTHP_ADDTIME,
        PhotoUpgrade::DROP_SCHPT_MEDIA_TYPE_INDEX,
        PhotoUpgrade::CREATE_SCHPT_MEDIA_TYPE_INDEX,
    };
    MEDIA_INFO_LOG("start updating photo index");
    ExecSqls(sqls, store);
}

static void UpdateVideoLabelTableForSubLabelType(RdbStore &store)
{
    const vector<string> sqls = {
        "DROP TABLE IF EXISTS " + VISION_VIDEO_LABEL_TABLE,
        CREATE_TAB_ANALYSIS_VIDEO_LABEL,
        UPDATE_VIDEO_LABEL_TOTAL_VALUE,
        UPDATE_SEARCH_INDEX_FOR_VIDEO,
    };
    MEDIA_INFO_LOG("start update video label table for sub_label_type");
    ExecSqls(sqls, store);
}

static void UpdateDataAddedIndexWithFileId(RdbStore &store)
{
    const vector<string> sqls = {
        PhotoUpgrade::DROP_INDEX_SCTHP_ADDTIME,
        PhotoUpgrade::INDEX_SCTHP_ADDTIME,
    };
    MEDIA_INFO_LOG("start update index of date added with file desc");
    ExecSqls(sqls, store);
}

static void UpdateMultiCropInfo(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        "ALTER TABLE " + VISION_RECOMMENDATION_TABLE + " ADD COLUMN " + MOVEMENT_CROP + " TEXT",
        "ALTER TABLE " + VISION_RECOMMENDATION_TABLE + " ADD COLUMN " + MOVEMENT_VERSION + " TEXT",
    };
    MEDIA_INFO_LOG("start update multi crop triggers");
    ExecSqls(executeSqlStrs, store);
}

static void UpdateSearchIndexTrigger(RdbStore &store)
{
    const vector<string> sqls = {
        "DROP TRIGGER IF EXISTS update_search_status_trigger",
        CREATE_SEARCH_UPDATE_STATUS_TRIGGER,
        "DROP TRIGGER IF EXISTS album_map_insert_search_trigger",
        "DROP TRIGGER IF EXISTS album_map_delete_search_trigger",
    };
    MEDIA_INFO_LOG("start update search index");
    ExecSqls(sqls, store);
}

static void AddOriginalSubtype(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_ORIGINAL_SUBTYPE + " INT"
    };
    MEDIA_INFO_LOG("start add original_subtype column");
    ExecSqls(sqls, store);
}

static void ReportFailInfoAsync(AsyncTaskData *data)
{
    MEDIA_INFO_LOG("Start ReportFailInfoAsync");
    const int32_t sleepTimeMs = 1000;
    this_thread::sleep_for(chrono::milliseconds(sleepTimeMs));
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr,
        "MediaDataAbility insert functionality rebStore is null.");

    string querySql = "SELECT data FROM Photos GROUP BY data HAVING COUNT(*) > 1";
    auto result = rdbStore->QuerySql(querySql);
    int32_t count = 0;
    CHECK_AND_RETURN_LOG(result != nullptr, "result is null");
    CHECK_AND_PRINT_LOG(result->GetRowCount(count) == NativeRdb::E_OK, "GetRowCount fail");
    result->Close();
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    DfxReporter::ReportStartResult(DfxType::ADD_DATA_UNIQUE_INDEX_FAIL, count, startTime);
    bool ret = system::SetParameter("persist.multimedia.medialibrary.data_unique", "1");
    CHECK_AND_PRINT_LOG(ret, "Failed to set parameter, ret:%{public}d", ret);
    MEDIA_INFO_LOG("HasDirtyData count:%{public}d", count);
}

static void ReportFailInfo()
{
    MEDIA_INFO_LOG("Start ReportFailInfo");
    auto asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    CHECK_AND_RETURN_LOG(asyncWorker != nullptr, "Failed to get async worker instance!");
    shared_ptr<MediaLibraryAsyncTask> reportTask =
        make_shared<MediaLibraryAsyncTask>(ReportFailInfoAsync, nullptr);
    CHECK_AND_RETURN_LOG(reportTask != nullptr, "Failed to create async task for reportTask!");
    asyncWorker->AddTask(reportTask, false);
}

static void UpdateDataUniqueIndex(RdbStore &store)
{
    MEDIA_INFO_LOG("Start UpdateDataUniqueIndex");
    string sql = PhotoUpgrade::UPDATA_PHOTOS_DATA_UNIQUE;
    auto err = ExecSqlWithRetry([&]() { return store.ExecuteSql(sql); });
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to exec: %{public}s", sql.c_str());
        ReportFailInfo();
    }
    MEDIA_INFO_LOG("End UpdateDataUniqueIndex");
}

static void FixPhotoSchptMediaTypeIndex(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        PhotoUpgrade::CREATE_SCHPT_MEDIA_TYPE_INDEX,
    };
    MEDIA_INFO_LOG("Fix idx_schpt_media_type index");
    ExecSqls(executeSqlStrs, store);
    MEDIA_INFO_LOG("End fix idx_schpt_media_type index.");
}

static void AddAnalysisAlbumTotalTable(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        CREATE_TAB_ANALYSIS_ALBUM_TOTAL,
        INIT_TAB_ANALYSIS_ALBUM_TOTAL,
        CREATE_TOTAL_INSERT_TRIGGER_FOR_ADD_ANALYSIS_ALBUM_TOTAL,
        CREATE_VISION_UPDATE_TRIGGER_FOR_UPDATE_ANALYSIS_ALBUM_TOTAL_STATUS,
    };
    MEDIA_INFO_LOG("Start add analysis album total table");
    ExecSqls(executeSqlStrs, store);
}

static void CompatLivePhoto(RdbStore &store, int32_t oldVersion)
{
    MEDIA_INFO_LOG("Start configuring param for live photo compatibility");
    bool ret = false;
    // there is no need to ResetCursor() twice if album fusion is included
    if (oldVersion >= VERSION_ADD_OWNER_ALBUM_ID) {
        ret = system::SetParameter(REFRESH_CLOUD_LIVE_PHOTO_FLAG, CLOUD_LIVE_PHOTO_NOT_REFRESHED);
        MEDIA_INFO_LOG("Set parameter for refreshing cloud live photo, ret: %{public}d", ret);
    }

    ret = system::SetParameter(COMPAT_LIVE_PHOTO_FILE_ID, "1"); // start compating from file_id: 1
    MEDIA_INFO_LOG("Set parameter for compating local live photo, ret: %{public}d", ret);
}

static void ResetCloudCursorAfterInitFinish()
{
    MEDIA_INFO_LOG("Try reset cloud cursor after storage reconstruct");
    static uint32_t baseUserRange = 200000; // uid base offset
    uid_t uid = getuid() / baseUserRange;
    const string paramKey = "multimedia.medialibrary.startup." + to_string(uid);
    int32_t maxTryTimes = 10;
    if (WaitParameter(paramKey.c_str(), "true", maxTryTimes) == E_OK) {
        MEDIA_INFO_LOG("medialibrary init finish start reset cloud cursor");
        FileManagement::CloudSync::CloudSyncManager::GetInstance().ResetCursor();
        MEDIA_INFO_LOG("End reset cloud cursor");
    } else {
        MEDIA_INFO_LOG("try max time start reset cloud cursor");
        FileManagement::CloudSync::CloudSyncManager::GetInstance().ResetCursor();
        MEDIA_INFO_LOG("End reset cloud cursor");
    }
    MEDIA_INFO_LOG("Reset cloud cursor after storage reconstruct end");
}

static int32_t MatchedDataFusion(CompensateAlbumIdData* compensateData)
{
    int32_t matchedDataHandleResult = MediaLibraryAlbumFusionUtils::HandleMatchedDataFusion(
        compensateData->upgradeStore_);
    if (matchedDataHandleResult != E_OK) {
        MEDIA_ERR_LOG("Fatal err, handle matched relationship fail by %{public}d", matchedDataHandleResult);
        // This should not happen, try again
        matchedDataHandleResult = MediaLibraryAlbumFusionUtils::HandleMatchedDataFusion(
            compensateData->upgradeStore_);
        CHECK_AND_PRINT_LOG(matchedDataHandleResult == E_OK,
            "Fatal err, handle matched relationship again by %{public}d", matchedDataHandleResult);
    }
    return matchedDataHandleResult;
}

static void ReconstructMediaLibraryStorageFormatExecutor(AsyncTaskData *data)
{
    CHECK_AND_RETURN_LOG(data != nullptr, "task data is nullptr");
    MediaLibraryAlbumFusionUtils::SetParameterToStopSync();
    MediaLibraryAlbumFusionUtils::SetAlbumFuseUpgradeStatus(0); // 0: set upgrade status fail
    CompensateAlbumIdData* compensateData = static_cast<CompensateAlbumIdData*>(data);
    MEDIA_INFO_LOG("ALBUM_FUSE: Processing old data start");
    MEDIA_INFO_LOG("ALBUM_FUSE: Compensating album id for old asset start");
    int64_t beginTime = MediaFileUtils::UTCTimeMilliSeconds();
    CHECK_AND_PRINT_LOG(MediaLibraryAlbumFusionUtils::RemoveMisAddedHiddenData(compensateData->upgradeStore_) == E_OK,
        "Failed to remove misadded hidden data");
    int64_t cleanDataBeginTime = MediaFileUtils::UTCTimeMilliSeconds();
    MediaLibraryAlbumFusionUtils::ReportAlbumFusionData(cleanDataBeginTime, AlbumFusionState::START,
        compensateData->upgradeStore_);
    if (MatchedDataFusion(compensateData) != E_OK) {
        MediaLibraryAlbumFusionUtils::ReportAlbumFusionData(cleanDataBeginTime, AlbumFusionState::FAILED,
            compensateData->upgradeStore_);
        return;
    }
    int32_t notMatchedDataHandleResult = MediaLibraryAlbumFusionUtils::HandleNotMatchedDataFusion(
        compensateData->upgradeStore_);
    if (notMatchedDataHandleResult != E_OK) {
        MEDIA_ERR_LOG("Fatal err, handle not matched relationship fail by %{public}d", notMatchedDataHandleResult);
        // This should not happen, and if it does, avoid cleaning up more data.
        MediaLibraryAlbumFusionUtils::ReportAlbumFusionData(cleanDataBeginTime, AlbumFusionState::FAILED,
            compensateData->upgradeStore_);
        return;
    }
    MEDIA_INFO_LOG("ALBUM_FUSE: End compensate album id for old asset cost %{public}ld",
        (long)(MediaFileUtils::UTCTimeMilliSeconds() - cleanDataBeginTime));
    MEDIA_INFO_LOG("ALBUM_FUSE: Start rebuild album and update relationship");
    int64_t albumCleanBeginTime = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t rebuildResult = MediaLibraryAlbumFusionUtils::RebuildAlbumAndFillCloudValue(compensateData->upgradeStore_);
    MEDIA_INFO_LOG("ALBUM_FUSE: End rebuild album and update relationship cost %{public}ld",
        (long)(MediaFileUtils::UTCTimeMilliSeconds() - albumCleanBeginTime));
    // Restore cloud sync
    MediaLibraryAlbumFusionUtils::SetParameterToStartSync();
    MediaLibraryAlbumFusionUtils::SetAlbumFuseUpgradeStatus(1); // 1: set upgrade status success
    ResetCloudCursorAfterInitFinish();
    MediaLibraryAlbumFusionUtils::RefreshAllAlbums();
    MediaLibraryAlbumFusionUtils::ReportAlbumFusionData(cleanDataBeginTime, AlbumFusionState::SUCCESS,
        compensateData->upgradeStore_);
    MEDIA_INFO_LOG("ALBUM_FUSE: Processing old data end, cost %{public}ld",
        (long)(MediaFileUtils::UTCTimeMilliSeconds() - beginTime));
}

static void ReconstructMediaLibraryStorageFormatWithLock(AsyncTaskData *data)
{
    CHECK_AND_RETURN_LOG(data != nullptr, "task data is nullptr");
    CompensateAlbumIdData *compensateData = static_cast<CompensateAlbumIdData *>(data);
    CHECK_AND_RETURN_LOG(compensateData != nullptr, "compensateData is nullptr");
    std::unique_lock<std::mutex> reconstructLock(compensateData->lock_, std::defer_lock);
    if (reconstructLock.try_lock()) {
        ReconstructMediaLibraryStorageFormatExecutor(data);
    } else {
        MEDIA_WARN_LOG("Failed to acquire lock, skipping task Reconstruct.");
    }
}

static void AddOwnerAlbumIdAndRefractorTrigger(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_OWNER_ALBUM_ID + " INT DEFAULT 0",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_ORIGINAL_ASSET_CLOUD_ID + " TEXT",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_SOURCE_PATH + " TEXT",
        "DROP TABLE IF EXISTS album_plugin ",
        DROP_PHOTO_ALBUM_CLEAR_MAP_SQL,
        DROP_INSERT_PHOTO_INSERT_SOURCE_ALBUM_SQL,
        DROP_INSERT_PHOTO_UPDATE_SOURCE_ALBUM_SQL,
        DROP_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER,
        DROP_INSERT_SOURCE_PHOTO_UPDATE_ALBUM_ID_TRIGGER,
        "DROP TRIGGER IF EXISTS photos_mdirty_trigger",
        PhotoUpgrade::CREATE_PHOTOS_MDIRTY_TRIGGER,
        CREATE_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER,
        CREATE_INSERT_SOURCE_UPDATE_ALBUM_ID_TRIGGER,

    };
    MEDIA_INFO_LOG("Add owner_album_id column for Photos");
    ExecSqls(sqls, store);
}

static void AddMergeInfoColumnForAlbum(RdbStore &store)
{
    const vector<string> addMergeInfoSql = {
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
        PhotoAlbumColumns::ALBUM_DATE_ADDED + " BIGINT DEFAULT 0",
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
        PhotoAlbumColumns::ALBUM_PRIORITY + " INT",
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
        PhotoAlbumColumns::ALBUM_LPATH + " TEXT",
        DROP_INDEX_SOURCE_ALBUM_INDEX,
        CREATE_SOURCE_ALBUM_INDEX,
        CREATE_DEFALUT_ALBUM_FOR_NO_RELATIONSHIP_ASSET,
    };
    MEDIA_INFO_LOG("Add merge info for PhotoAlbum");
    ExecSqls(addMergeInfoSql, store);
    const std::string queryHiddenAlbumId =
        "SELECT album_id FROM PhotoAlbum WHERE album_name = '.hiddenAlbum'";
    auto resultSet = store.QuerySql(queryHiddenAlbumId);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        int32_t err = ExecSqlWithRetry([&]() { return store.ExecuteSql(CREATE_HIDDEN_ALBUM_FOR_DUAL_ASSET); });
        CHECK_AND_PRINT_LOG(err == NativeRdb::E_OK,
            "Failed to exec: %{private}s", CREATE_HIDDEN_ALBUM_FOR_DUAL_ASSET.c_str());
    }
}

int32_t MediaLibraryRdbStore::ReconstructMediaLibraryStorageFormat(const std::shared_ptr<MediaLibraryRdbStore> store)
{
    MEDIA_INFO_LOG("ALBUM_FUSE: Start reconstruct medialibrary storage format task!");
    auto asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    CHECK_AND_RETURN_RET_LOG(asyncWorker !=  nullptr, E_FAIL, "Failed to get aysnc worker instance!");

    auto *taskData = new (std::nothrow) CompensateAlbumIdData(store, MediaLibraryRdbStore::reconstructLock_);
    CHECK_AND_RETURN_RET_LOG(taskData != nullptr, E_NO_MEMORY, "Failed to alloc async data for compensate album id");

    auto asyncTask = std::make_shared<MediaLibraryAsyncTask>(ReconstructMediaLibraryStorageFormatWithLock, taskData);
    asyncWorker->AddTask(asyncTask, false);
    return E_OK;
}

void AddHighlightMapTable(RdbStore &store)
{
    const vector<string> executeSqlStrs = {
        CREATE_ANALYSIS_ASSET_SD_MAP_TABLE,
        CREATE_ANALYSIS_ALBUM_ASET_MAP_TABLE,
        "ALTER TABLE " + HIGHLIGHT_PLAY_INFO_TABLE + " ADD COLUMN " + HIGHLIGHTING_ALGO_VERSION + " TEXT",
        "ALTER TABLE " + HIGHLIGHT_PLAY_INFO_TABLE + " ADD COLUMN " + CAMERA_MOVEMENT_ALGO_VERSION + " TEXT",
        "ALTER TABLE " + HIGHLIGHT_PLAY_INFO_TABLE + " ADD COLUMN " + TRANSITION_ALGO_VERSION + " TEXT",
    };
    MEDIA_INFO_LOG("add analysis map table of highlight db");
    ExecSqls(executeSqlStrs, store);
}

static void AddPetFaceTable(RdbStore &store, int32_t version)
{
    const vector<string> executeSqlStrs = {
        CREATE_TAB_ANALYSIS_PET_FACE,
        CREATE_TAB_ANALYSIS_PET_TAG,
        ADD_PET_STATUS_COLUMN,
    };
    MEDIA_INFO_LOG("add pet tables start");
    ExecSqlsWithDfx(executeSqlStrs, store, version);
    MEDIA_INFO_LOG("add pet tables end");
}

static void UpgradeOtherTable(RdbStore &store, int32_t oldVersion)
{
    if (oldVersion < VERSION_ADD_PACKAGE_NAME) {
        AddPackageNameColumnOnTables(store);
    }

    if (oldVersion < VERSION_ADD_CLOUD_ALBUM) {
        UpdateCloudAlbum(store);
    }

    if (oldVersion < VERSION_ADD_CAMERA_SHOT_KEY) {
        AddCameraShotKey(store);
    }

    if (oldVersion < VERSION_REMOVE_ALBUM_COUNT_TRIGGER) {
        RemoveAlbumCountTrigger(store);
    }

    if (oldVersion < VERSION_ADD_ALL_EXIF) {
        AddExifAndUserComment(store);
    }

    if (oldVersion < VERSION_ADD_UPDATE_CLOUD_SYNC_TRIGGER) {
        AddUpdateCloudSyncTrigger(store);
    }

    if (oldVersion < VERSION_ADD_YEAR_MONTH_DAY) {
        AddYearMonthDayColumn(store);
    }

    if (oldVersion < VERSION_UPDATE_YEAR_MONTH_DAY) {
        UpdateYearMonthDayData(store);
    }

    if (oldVersion < VERSION_ADD_PHOTO_EDIT_TIME) {
        AddPhotoEditTimeColumn(store);
    }

    if (oldVersion < VERSION_ADD_SHOOTING_MODE) {
        AddShootingModeColumn(store);
    }

    if (oldVersion < VERSION_FIX_INDEX_ORDER) {
        FixIndexOrder(store);
    }

    if (oldVersion < VERSION_FIX_DOCS_PATH) {
        FixDocsPath(store);
    }
    if (oldVersion < VERSION_ADD_SHOOTING_MODE_TAG) {
        AddShootingModeTagColumn(store);
    }

    if (oldVersion < VERSION_ADD_PORTRAIT_IN_ALBUM) {
        AddPortraitInAnalysisAlbum(store);
    }

    if (oldVersion < VERSION_UPDATE_GEO_TABLE) {
        UpdateGeoTables(store);
    }

    if (oldVersion < VERSION_ADD_MULTISTAGES_CAPTURE) {
        AddMultiStagesCaptureColumns(store);
    }
    // !! Do not add upgrade code here !!
}

static void UpgradeGalleryFeatureTable(RdbStore &store, int32_t oldVersion)
{
    if (oldVersion < VERSION_ADD_HIDDEN_VIEW_COLUMNS) {
        AddHiddenViewColumn(store);
    }

    if (oldVersion < VERSION_ADD_LAST_VISIT_TIME) {
        ModifyMdirtyTriggers(store);
        AddLastVisitTimeColumn(store);
    }

    if (oldVersion < VERSION_ADD_HIDDEN_TIME) {
        AddHiddenTimeColumn(store);
    }

    if (oldVersion < VERSION_ADD_LOCATION_TABLE) {
        AddLocationTables(store);
    }

    if (oldVersion < VERSION_ADD_ALBUM_ORDER) {
        AddAlbumOrderColumn(store);
    }

    if (oldVersion < VERSION_ADD_FORM_MAP) {
        AddFormMap(store);
    }

    if (oldVersion < VERSION_UPDATE_LOCATION_TABLE) {
        UpdateLocationTables(store);
    }

    if (oldVersion < VERSION_ADD_IMAGE_VIDEO_COUNT) {
        AddImageVideoCount(store);
    }

    if (oldVersion < VERSION_ADD_SCHPT_HIDDEN_TIME_INDEX) {
        AddSCHPTHiddenTimeIndex(store);
    }

    if (oldVersion < VERSION_UPDATE_PHOTOS_MDIRTY_TRIGGER) {
        UpdatePhotosMdirtyTrigger(store);
    }

    if (oldVersion < VERSION_ALBUM_REFRESH) {
        UpdateAlbumRefreshTable(store);
    }

    if (oldVersion < VERSION_ADD_FAVORITE_INDEX) {
        UpdateFavoriteIndex(store);
    }

    if (oldVersion < VERSION_ADD_OWNER_APPID) {
        AddOwnerAppId(store);
    }

    if (oldVersion < VERSION_ADD_DYNAMIC_RANGE_TYPE) {
        AddDynamicRangeType(store);
    }
    // !! Do not add upgrade code here !!
}

static void UpgradeVisionTable(RdbStore &store, int32_t oldVersion)
{
    if (oldVersion < VERSION_ADD_VISION_TABLE) {
        AddAnalysisTables(store);
    }

    if (oldVersion < VERSION_ADD_FACE_TABLE) {
        AddFaceTables(store);
    }

    if (oldVersion < VERSION_ADD_SOURCE_ALBUM_TRIGGER) {
        AddSourceAlbumTrigger(store);
    }

    if (oldVersion < VERSION_ADD_VISION_ALBUM) {
        AddAnalysisAlbum(store);
    }

    if (oldVersion < VERSION_ADD_AESTHETIC_COMPOSITION_TABLE) {
        AddAestheticCompositionTables(store);
    }

    if (oldVersion < VERSION_ADD_SEARCH_TABLE) {
        AddSearchTable(store);
    }

    if (oldVersion < VERSION_ADD_SALIENCY_TABLE) {
        AddSaliencyTables(store);
    }

    if (oldVersion < VERSION_UPDATE_SOURCE_ALBUM_TRIGGER) {
        AddSourceAlbumTrigger(store);
    }

    if (oldVersion < VERSION_CLEAR_LABEL_DATA) {
        UpdateClassifyDirtyData(store);
    }

    if (oldVersion < VERSION_REOMOVE_SOURCE_ALBUM_TO_ANALYSIS) {
        RemoveSourceAlbumToAnalysis(store);
    }

    if (oldVersion < VERSION_UPDATE_DATE_TO_MILLISECOND) {
        UpdateMillisecondDate(store);
    }

    if (oldVersion < VERSION_ADD_HAS_ASTC) {
        AddHasAstcColumns(store);
    }

    if (oldVersion < VERSION_ADD_ADDRESS_DESCRIPTION) {
        AddAddressDescriptionColumns(store);
    }

    if (oldVersion < VERSION_UPDATE_SPEC_FOR_ADD_SCREENSHOT) {
        UpdateSpecForAddScreenshot(store);
    }

    if (oldVersion < VERSION_MOVE_SOURCE_ALBUM_TO_PHOTO_ALBUM_AND_ADD_COLUMNS) {
        MoveSourceAlbumToPhotoAlbumAndAddColumns(store);
    }

    if (oldVersion < VERSION_MODIFY_SOURCE_ALBUM_TRIGGERS) {
        ModifySourceAlbumTriggers(store);
    }
    // !! Do not add upgrade code here !!
}

static void UpgradeExtendedVisionTable(RdbStore &store, int32_t oldVersion)
{
    if (oldVersion < VERSION_ADD_HEAD_AND_POSE_TABLE) {
        AddHeadAndPoseTables(store);
    }

    if (oldVersion < VERSION_ADD_IS_COVER_SATISFIED_COLUMN) {
        AddIsCoverSatisfiedColumn(store);
    }

    if (oldVersion < VERSION_ADD_VIDEO_LABEL_TABEL) {
        AddVideoLabelTable(store);
    }

    if (oldVersion < VERSION_ADD_SEGMENTATION_COLUMNS) {
        AddSegmentationColumns(store);
    }
    // !! Do not add upgrade code here !!
}

static void UpgradeAlbumTable(RdbStore &store, int32_t oldVersion)
{
    if (oldVersion < VERSION_ADD_IS_LOCAL_ALBUM) {
        AddIsLocalAlbum(store);
    }
    // !! Do not add upgrade code here !!
}

static void UpgradeUriPermissionTable(RdbStore &store, int32_t oldVersion)
{
    if (oldVersion < VERSION_UPDATE_URIPERMISSION_SOURCE_TOKEN_AND_TARGET_TOKEN) {
        AddSourceAndTargetTokenForUriPermission(store);
    }
}

static void UpgradeHighlightAlbumChange(RdbStore &store, int32_t oldVersion)
{
    if (oldVersion < VERSION_HIGHLIGHT_CHANGE_FUNCTION) {
        AddHighlightChangeFunction(store);
    }
}

static void UpgradeHistory(RdbStore &store, int32_t oldVersion)
{
    if (oldVersion < VERSION_ADD_MISSING_UPDATES) {
        AddMissingUpdates(store);
    }

    if (oldVersion < VERSION_UPDATE_MDIRTY_TRIGGER_FOR_SDIRTY) {
        UpdateMdirtyTriggerForSdirty(store);
    }

    if (oldVersion < VERSION_SHOOTING_MODE_CLOUD) {
        AddBussinessRecordAlbum(store);
    }
    // !! Do not add upgrade code here !!
}

static void UpdatePhotosSearchUpdateTrigger(RdbStore& store)
{
    static const vector<string> executeSqlStrs = {
        "DROP TRIGGER IF EXISTS update_search_status_trigger",
        CREATE_SEARCH_UPDATE_STATUS_TRIGGER,
    };
    MEDIA_INFO_LOG("Start update photos search trigger");
    ExecSqls(executeSqlStrs, store);
}

static void AddIsTemp(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_IS_TEMP + " INT DEFAULT 0"
    };
    MEDIA_INFO_LOG("Start add is_temp on Photos in upgrade");
    ExecSqls(executeSqlStrs, store);
}

static void AddIsTempToTrigger(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        "DROP INDEX IF EXISTS " + PhotoColumn::PHOTO_SCHPT_DAY_INDEX,
        "DROP INDEX IF EXISTS " + PhotoColumn::PHOTO_SCHPT_MEDIA_TYPE_INDEX,
        "DROP INDEX IF EXISTS " + PhotoColumn::PHOTO_SCHPT_HIDDEN_TIME_INDEX,
        "DROP INDEX IF EXISTS " + PhotoColumn::PHOTO_FAVORITE_INDEX,
        "DROP INDEX IF EXISTS " + PhotoColumn::PHOTO_SCHPT_ADDED_INDEX,
        PhotoUpgrade::CREATE_SCHPT_DAY_INDEX,
        PhotoUpgrade::CREATE_SCHPT_MEDIA_TYPE_INDEX,
        PhotoUpgrade::CREATE_SCHPT_HIDDEN_TIME_INDEX,
        PhotoUpgrade::CREATE_PHOTO_FAVORITE_INDEX,
        PhotoUpgrade::INDEX_SCTHP_ADDTIME,
    };
    MEDIA_INFO_LOG("Add is_temp to trigger in upgrade");
    ExecSqls(executeSqlStrs, store);
}

static void AddFrontCameraType(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_FRONT_CAMERA + " TEXT"
    };
    MEDIA_INFO_LOG("Start add front column");
    ExecSqls(sqls, store);
}

static void AddDisplayNameIndex(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        PhotoUpgrade::CREATE_PHOTO_DISPLAYNAME_INDEX,
    };
    MEDIA_INFO_LOG("Add displayname index");
    ExecSqls(executeSqlStrs, store);
}

static void AddPortraitCoverSelectionColumn(RdbStore &store)
{
    MEDIA_INFO_LOG("Start add portrait cover selection column");

    const vector<string> sqls = {
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + BEAUTY_BOUNDER_X + " REAL",
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + BEAUTY_BOUNDER_Y + " REAL",
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + BEAUTY_BOUNDER_WIDTH + " REAL",
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + BEAUTY_BOUNDER_HEIGHT + " REAL",
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + FACE_AESTHETICS_SCORE + " REAL",
    };
    ExecSqls(sqls, store);
}

static void UpdatePortraitCoverSelectionColumns(RdbStore &store)
{
    MEDIA_INFO_LOG("Start update portrait cover selection columns");

    const vector<string> sqls = {
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + BEAUTY_BOUNDER_VERSION + " TEXT default '' ",
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + IS_EXCLUDED + " INT default 0 ",
    };
    ExecSqls(sqls, store);
}

static void AddAppUriPermissionInfo(RdbStore &store)
{
    const std::string SYNC_DATA_FROM_PHOTOS_SQL =
        "insert into "+ AppUriPermissionColumn::APP_URI_PERMISSION_TABLE + "(" +
        AppUriPermissionColumn::APP_ID + ", " + AppUriPermissionColumn::FILE_ID + ", " +
        AppUriPermissionColumn::URI_TYPE + ", " + AppUriPermissionColumn::PERMISSION_TYPE + ", " +
        AppUriPermissionColumn::DATE_MODIFIED + ") " +
        "select " +
        MediaColumn::MEDIA_OWNER_APPID + ", " + MediaColumn::MEDIA_ID + ", " +
        std::to_string(AppUriPermissionColumn::URI_PHOTO) + ", " +
        std::to_string(AppUriPermissionColumn::PERMISSION_PERSIST_READ_WRITE) + ", " +
        MediaColumn::MEDIA_DATE_ADDED +
        " from " + PhotoColumn::PHOTOS_TABLE +
        " where " + MediaColumn::MEDIA_OWNER_APPID + " is not null";

    const std::string SYNC_DATA_FROM_AUDIOS_SQL =
        "insert into "+ AppUriPermissionColumn::APP_URI_PERMISSION_TABLE + "(" +
        AppUriPermissionColumn::APP_ID + ", " + AppUriPermissionColumn::FILE_ID + ", " +
        AppUriPermissionColumn::URI_TYPE + ", " + AppUriPermissionColumn::PERMISSION_TYPE + ", " +
        AppUriPermissionColumn::DATE_MODIFIED + ") " +
        "select " +
        MediaColumn::MEDIA_OWNER_APPID + ", " + MediaColumn::MEDIA_ID + ", " +
        std::to_string(AppUriPermissionColumn::URI_AUDIO) + ", " +
        std::to_string(AppUriPermissionColumn::PERMISSION_PERSIST_READ_WRITE) + ", " +
        MediaColumn::MEDIA_DATE_ADDED +
        " from " + AudioColumn::AUDIOS_TABLE +
        " where " + MediaColumn::MEDIA_OWNER_APPID + " is not null";
    const vector<string> sqls = {
        AppUriPermissionColumn::CREATE_APP_URI_PERMISSION_TABLE,
        AppUriPermissionColumn::CREATE_URI_URITYPE_APPID_INDEX,
        SYNC_DATA_FROM_PHOTOS_SQL,
        SYNC_DATA_FROM_AUDIOS_SQL,
        TriggerDeletePhotoClearAppUriPermission(),
        TriggerDeleteAudioClearAppUriPermission(),
    };
    MEDIA_INFO_LOG("add uriPermission table info when upgrade phone");
    ExecSqls(sqls, store);
}

static void AddCoverPosition(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_COVER_POSITION +
            " BIGINT DEFAULT 0",
    };
    MEDIA_INFO_LOG("start add cover_position column");
    ExecSqls(sqls, store);
}

static void AddSchptReadyIndex(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        PhotoUpgrade::INDEX_SCHPT_READY,
    };
    MEDIA_INFO_LOG("Add schpt ready index");
    ExecSqls(executeSqlStrs, store);
}

static void UpdateSourceAlbumAndAlbumBundlenameTriggers(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        DROP_INSERT_PHOTO_INSERT_SOURCE_ALBUM,
        DROP_INSERT_PHOTO_UPDATE_SOURCE_ALBUM,
        DROP_INSERT_PHOTO_UPDATE_ALBUM_BUNDLENAME,
        INSERT_PHOTO_INSERT_SOURCE_ALBUM,
        INSERT_PHOTO_UPDATE_SOURCE_ALBUM,
        INSERT_PHOTO_UPDATE_ALBUM_BUNDLENAME,
    };
    MEDIA_INFO_LOG("start update source album and album bundlename triggers");
    ExecSqls(executeSqlStrs, store);
}

static void AddBestFaceBoundingColumnForGroupAlbum(RdbStore &store)
{
    MEDIA_INFO_LOG("Start add best face bounding column for group album");

    const vector<string> sqls = {
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + JOINT_BEAUTY_BOUNDER_X + " REAL",
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + JOINT_BEAUTY_BOUNDER_Y + " REAL",
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + JOINT_BEAUTY_BOUNDER_WIDTH + " REAL",
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + JOINT_BEAUTY_BOUNDER_HEIGHT + " REAL",
    };

    ExecSqls(sqls, store);
}

static void AddGroupVersion(RdbStore &store)
{
    MEDIA_INFO_LOG("Start add group_version column");
    const vector<string> sqls = {
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + GROUP_VERSION + " TEXT"
    };

    ExecSqls(sqls, store);
}

static void AddCreateTmpCompatibleDup(RdbStore &store, int32_t version)
{
    MEDIA_INFO_LOG("Start add create_tmp_compatible_dup columns");
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE +
            " ADD COLUMN " + PhotoColumn::PHOTO_TRANSCODE_TIME  + " BIGINT NOT NULL DEFAULT 0",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE +
            " ADD COLUMN " + PhotoColumn::PHOTO_TRANS_CODE_FILE_SIZE  + " BIGINT NOT NULL DEFAULT 0",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE +
            " ADD COLUMN " + PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE  + " INT NOT NULL DEFAULT 0"
    };
    ExecSqlsWithDfx(sqls, store, version);
}

static void AddSouthDeviceType(RdbStore &store, int32_t version)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE +
            " INT NOT NULL DEFAULT 0",
    };
    MEDIA_INFO_LOG("Add south_device_type column start");
    ExecSqlsWithDfx(sqls, store, version);
    MEDIA_INFO_LOG("Add south_device_type column end");
}

static void AddAffective(RdbStore &store, int32_t version)
{
    MEDIA_INFO_LOG("Start add affective");
    const vector<string> sqls = {
        "ALTER TABLE " + VISION_TOTAL_TABLE + " ADD COLUMN " + AFFECTIVE +  + " INT NOT NULL DEFAULT 0 ",
        CREATE_TAB_ANALYSIS_AFFECTIVE,
    };
    ExecSqlsWithDfx(sqls, store, version);
    MEDIA_INFO_LOG("End add affective");
}

static void AddVideoMode(RdbStore &store, int32_t version)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_VIDEO_MODE +
            " INT NOT NULL DEFAULT -1",
    };
    ExecSqlsWithDfx(sqls, store, version);
    MEDIA_INFO_LOG("Add VideoMode column end");
}

static void AddUploadStatus(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " + PhotoAlbumColumns::UPLOAD_STATUS +
            " INT NOT NULL DEFAULT 0",
        DROP_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER,
        CREATE_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER,
        "UPDATE PhotoAlbum SET upload_status = 1 WHERE album_type IN (0, 2048)",
    };
    MEDIA_INFO_LOG("Add upload status column start");
    ExecSqls(sqls, store);
    MEDIA_INFO_LOG("Add upload status column end");
}

static void AddDetailTimeToPhotos(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_DETAIL_TIME + " TEXT"
    };
    MEDIA_INFO_LOG("Add detail_time column start");
    ExecSqls(sqls, store);
}

static void AddVideoFaceTable(RdbStore &store)
{
    const vector<string> sqls = {
        CREATE_TAB_VIDEO_FACE,
        CREATE_VIDEO_FACE_INDEX,
        "ALTER TABLE " + VISION_TOTAL_TABLE + " ADD COLUMN " + GEO + " INT"
    };
    MEDIA_INFO_LOG("Add video face table start");
    ExecSqls(sqls, store);
}

static void AddGeoDefaultValue(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + VISION_TOTAL_TABLE + " DROP COLUMN " + GEO,
        "ALTER TABLE " + VISION_TOTAL_TABLE + " ADD COLUMN " + GEO + " INT DEFAULT 0",
    };
    MEDIA_INFO_LOG("Add geo deault value start");
    ExecSqls(sqls, store);
}

static void AddOCRCardColumns(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + VISION_OCR_TABLE + " ADD COLUMN " + OCR_CARD_TEXT + " TEXT",
        "ALTER TABLE " + VISION_OCR_TABLE + " ADD COLUMN " + OCR_CARD_TEXT_MSG + " TEXT",
    };
    MEDIA_INFO_LOG("Add video face table start");
    ExecSqls(sqls, store);
}

static void DropPhotoAlbumClearMap(RdbStore& store)
{
    const vector<string> sqls = {
        DROP_PHOTO_ALBUM_CLEAR_MAP_SQL,
        DROP_INSERT_PHOTO_UPDATE_ALBUM_BUNDLENAME,
        INSERT_PHOTO_UPDATE_ALBUM_BUNDLENAME,
    };
    MEDIA_INFO_LOG("Drop photoAlbum clear map start");
    ExecSqls(sqls, store);
}

static void AddThumbnailVisible(RdbStore& store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_THUMBNAIL_VISIBLE +
        " INT DEFAULT 0"
    };
    MEDIA_INFO_LOG("Add video face table start");
    ExecSqls(sqls, store);
}

static void AlterThumbnailVisible(RdbStore& store)
{
    const vector<string> sqls = {
        PhotoUpgrade::DROP_INDEX_SCHPT_READY,
        PhotoUpgrade::INDEX_SCHPT_READY,
    };
    MEDIA_INFO_LOG("Add AlterThumbnailVisible");
    ExecSqls(sqls, store);
}

static void AddHighlightVideoCountCanPack(RdbStore& store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + HIGHLIGHT_ALBUM_TABLE + " ADD COLUMN " + HIGHLIGHT_VIDEO_COUNT_CAN_PACK + " INT",
    };
    MEDIA_INFO_LOG("Add key: hilghlight video count can pack Start");
    ExecSqls(sqls, store);
}

static void FixSourceAlbumUpdateTriggerToUseLPath(RdbStore& store)
{
    const vector<string> sqls = {
        DROP_INSERT_SOURCE_PHOTO_UPDATE_ALBUM_ID_TRIGGER,
        CREATE_INSERT_SOURCE_UPDATE_ALBUM_ID_TRIGGER
    };
    MEDIA_INFO_LOG("Fix source album update trigger to use lpath start");
    ExecSqls(sqls, store);
}

static void AddMediaSuffixColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_MEDIA_SUFFIX + " TEXT",
    };
    ExecSqls(sqls, store);
}

static void AddAppLinkColumn(RdbStore &store, int32_t version)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_HAS_APPLINK +
            " INT NOT NULL DEFAULT 0",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_APPLINK +
            " TEXT",
    };
    MEDIA_INFO_LOG("add has_applink/applink column start");
    ExecSqlsWithDfx(sqls, store, version);
    MEDIA_INFO_LOG("add has_applink/applink column end");
}

static void AddVisitCountColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE +
            " ADD COLUMN " + PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME  + " BIGINT NOT NULL DEFAULT 0",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE +
            " ADD COLUMN " + PhotoColumn::PHOTO_VISIT_COUNT  + " INT NOT NULL DEFAULT 0",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE +
            " ADD COLUMN " + PhotoColumn::PHOTO_LCD_VISIT_COUNT  + " INT NOT NULL DEFAULT 0"
    };
    MEDIA_INFO_LOG("add real_lcd_visit_time/visit_count/lcd_visit_count column start");
    ExecSqls(sqls, store);
    MEDIA_INFO_LOG("add real_lcd_visit_time/visit_count/lcd_visit_count column end");
}

static void AddIsRecentShow(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_IS_RECENT_SHOW  +
            " INT NOT NULL DEFAULT 1",
    };
    MEDIA_INFO_LOG("add is_recent_show column start");
    ExecSqls(sqls, store);
}

static void AddFrontAnalysisColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + USER_PHOTOGRAPHY_INFO_TABLE + " ADD COLUMN " + FRONT_INDEX_LIMIT + " INT DEFAULT 0",
        "ALTER TABLE " + USER_PHOTOGRAPHY_INFO_TABLE + " ADD COLUMN " + FRONT_INDEX_MODIFIED + " BIGINT DEFAULT 0",
        "ALTER TABLE " + USER_PHOTOGRAPHY_INFO_TABLE + " ADD COLUMN " + FRONT_INDEX_COUNT + " INT DEFAULT 0",
        "ALTER TABLE " + USER_PHOTOGRAPHY_INFO_TABLE + " ADD COLUMN " + FRONT_CV_MODIFIED + " BIGINT DEFAULT 0",
        "ALTER TABLE " + USER_PHOTOGRAPHY_INFO_TABLE + " ADD COLUMN " + FRONT_CV_COUNT + " INT DEFAULT 0",
    };
    MEDIA_INFO_LOG("Add front analysis column start");
    ExecSqls(sqls, store);
}

static void AddDcAnalysisColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + USER_PHOTOGRAPHY_INFO_TABLE + " ADD COLUMN " + DC_INDEX_COUNT + " INT DEFAULT 0",
        "ALTER TABLE " + USER_PHOTOGRAPHY_INFO_TABLE + " ADD COLUMN " + DC_OCR_COUNT + " INT DEFAULT 0",
        "ALTER TABLE " + USER_PHOTOGRAPHY_INFO_TABLE + " ADD COLUMN " + DC_LABEL_COUNT + " INT DEFAULT 0",
        "ALTER TABLE " + USER_PHOTOGRAPHY_INFO_TABLE + " ADD COLUMN " + DC_MODIFY_TIME_STAMP + " BIGINT DEFAULT 0",
    };
    MEDIA_INFO_LOG("Add DC analysis column start");
    ExecSqls(sqls, store);
    MEDIA_INFO_LOG("Add DC analysis column end");
}

static void AddEditDataSizeColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoExtColumn::PHOTOS_EXT_TABLE + " ADD COLUMN " + PhotoExtColumn::EDITDATA_SIZE +
        " BIGINT DEFAULT 0",
    };
    MEDIA_INFO_LOG("Add editdata size column start");
    ExecSqls(sqls, store);
    MEDIA_INFO_LOG("Add editdata size column end");
}

static void AddDcAnalysisIndexUpdateColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + USER_PHOTOGRAPHY_INFO_TABLE + " ADD COLUMN " + DC_INDEX_UPDATE_COUNT + " INT DEFAULT 0",
    };
    MEDIA_INFO_LOG("Add DC analysis index update column start");
    ExecSqls(sqls, store);
    MEDIA_INFO_LOG("Add DC analysis index update column end");
}

static void AddCoverColumns(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " + PhotoAlbumColumns::COVER_URI_SOURCE +
            " INT NOT NULL DEFAULT 0",
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " + PhotoAlbumColumns::COVER_CLOUD_ID +
            " TEXT",
    };
    MEDIA_INFO_LOG("add cover columns start");
    ExecSqls(sqls, store);
    MEDIA_INFO_LOG("add cover columns end");
}

static void FixSourceAlbumCreateTriggersToUseLPath(RdbStore& store)
{
    const vector<string> sqls = {
        DROP_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER,
        CREATE_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER
    };
    MEDIA_INFO_LOG("Fix source album other triggers to use lpath start");
    ExecSqls(sqls, store);
}

static void AddAlbumPluginBundleName(RdbStore &store)
{
    MEDIA_INFO_LOG("Start updating album plugin");
    const vector<string> sqls = {
        "DROP TABLE IF EXISTS album_plugin;"
    };
    ExecSqls(sqls, store);
    AlbumPluginTableEventHandler().OnCreate(store);
    MEDIA_INFO_LOG("End updating album plugin");
}

static void FixMdirtyTriggerToUploadDetailTime(RdbStore &store)
{
    MEDIA_INFO_LOG("Start updating mdirty trigger to upload detail_time");
    const vector<string> sqls = {
        "DROP TRIGGER IF EXISTS photos_mdirty_trigger",
        PhotoUpgrade::CREATE_PHOTOS_MDIRTY_TRIGGER,
    };
    ExecSqls(sqls, store);
    MEDIA_INFO_LOG("End updating mdirty trigger to upload detail_time");
}

static void ReAddInsertTrigger(RdbStore &store)
{
    MEDIA_INFO_LOG("Start readd insert trigger");
    const vector<string> sqls = {
        DROP_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER,
        CREATE_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER,
        DROP_INSERT_SOURCE_PHOTO_UPDATE_ALBUM_ID_TRIGGER,
        CREATE_INSERT_SOURCE_UPDATE_ALBUM_ID_TRIGGER,
        DROP_INSERT_PHOTO_UPDATE_ALBUM_BUNDLENAME,
        INSERT_PHOTO_UPDATE_ALBUM_BUNDLENAME
    };
    ExecSqls(sqls, store);
    MEDIA_INFO_LOG("End readd insert trigger");
}

static void AddLakeFileColumn(RdbStore &store, int32_t version)
{
    MEDIA_INFO_LOG("Start add lake file column");
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_FILE_INODE + " TEXT",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_STORAGE_PATH + " TEXT",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_FILE_SOURCE_TYPE +
            " INT NOT NULL DEFAULT 0",
    };
    ExecSqlsWithDfx(sqls, store, version);
    MEDIA_INFO_LOG("End add lake file column");
}

static void AddLakeAlbumTable(RdbStore &store, int32_t version)
{
    MEDIA_INFO_LOG("Start add lake album table");
    const vector<string> sqls = {
        CREATE_LAKE_ALBUM_TABLE,
    };
    ExecSqlsWithDfx(sqls, store, version);
    MEDIA_INFO_LOG("End add lake album table");
}

static void AddPhotoMovingphotoEnhancementType(RdbStore &store, int32_t version)
{
    MEDIA_INFO_LOG("Add moving photo enhancement column start");
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE +
            " INT NOT NULL DEFAULT 0",
    };
    ExecSqlsWithDfx(sqls, store, version);
    MEDIA_INFO_LOG("Add moving photo enhancement column end");
}

static void UpgradeFromAPI15(RdbStore &store, unordered_map<string, bool> &photoColumnExists)
{
    MEDIA_INFO_LOG("Start VERSION_UPDATE_SOURCE_PHOTO_ALBUM_TRIGGER_AGAIN");
    UpdateSourcePhotoAlbumTrigger(store);
    MEDIA_INFO_LOG("End VERSION_UPDATE_SOURCE_PHOTO_ALBUM_TRIGGER_AGAIN");

    MEDIA_INFO_LOG("Start VERSION_ADD_MEDIA_IS_RECENT_SHOW_COLUMN");
    if (photoColumnExists.find(PhotoColumn::PHOTO_IS_RECENT_SHOW) == photoColumnExists.end() ||
        !photoColumnExists.at(PhotoColumn::PHOTO_IS_RECENT_SHOW)) {
        AddIsRecentShow(store);
    }
    MEDIA_INFO_LOG("End VERSION_ADD_MEDIA_IS_RECENT_SHOW_COLUMN");

    MEDIA_INFO_LOG("Start VERSION_FIX_SOURCE_ALBUM_CREATE_TRIGGERS_TO_USE_LPATH");
    FixSourceAlbumCreateTriggersToUseLPath(store);
    MEDIA_INFO_LOG("End VERSION_FIX_SOURCE_ALBUM_CREATE_TRIGGERS_TO_USE_LPATH");

    MEDIA_INFO_LOG("Start VERSION_ADD_IS_AUTO");
    if (photoColumnExists.find(PhotoColumn::PHOTO_IS_AUTO) == photoColumnExists.end() ||
        !photoColumnExists.at(PhotoColumn::PHOTO_IS_AUTO)) {
        AddIsAutoColumns(store);
    }
    MEDIA_INFO_LOG("End VERSION_ADD_IS_AUTO");

    MEDIA_INFO_LOG("Start VERSION_ADD_ALBUM_PLUGIN_BUNDLE_NAME");
    AddAlbumPluginBundleName(store);
    MEDIA_INFO_LOG("End VERSION_ADD_ALBUM_PLUGIN_BUNDLE_NAME");

    MEDIA_INFO_LOG("Start VERSION_ADD_MEDIA_SUFFIX_COLUMN");
    if (photoColumnExists.find(PhotoColumn::PHOTO_MEDIA_SUFFIX) == photoColumnExists.end() ||
        !photoColumnExists.at(PhotoColumn::PHOTO_MEDIA_SUFFIX)) {
        AddMediaSuffixColumn(store);
    }
    MEDIA_INFO_LOG("End VERSION_ADD_MEDIA_SUFFIX_COLUMN");

    MEDIA_INFO_LOG("Start VERSION_HIGHLIGHT_SUBTITLE");
    if (!IsColumnExists(store, HIGHLIGHT_ALBUM_TABLE, HIGHLIGHT_USE_SUBTITLE)) {
        AddHighlightUseSubtitle(store);
    }
    MEDIA_INFO_LOG("End VERSION_HIGHLIGHT_SUBTITLE");
}

static void UpgradeAPI18(RdbStore &store, unordered_map<string, bool> &photoColumnExists)
{
    MEDIA_INFO_LOG("Start VERSION_ADD_METARECOVERY");
    if (photoColumnExists.find(PhotoColumn::PHOTO_METADATA_FLAGS) == photoColumnExists.end() ||
        !photoColumnExists.at(PhotoColumn::PHOTO_METADATA_FLAGS)) {
        AddMetaRecovery(store);
    }
    MEDIA_INFO_LOG("End VERSION_ADD_METARECOVERY");

    MEDIA_INFO_LOG("Start VERSION_ADD_HIGHLIGHT_TRIGGER");
    if (!IsColumnExists(store, PhotoColumn::HIGHLIGHT_TABLE, PhotoColumn::MEDIA_DATA_DB_HIGHLIGHT_TRIGGER)) {
        AddHighlightTriggerColumn(store);
        AddHighlightInsertAndUpdateTrigger(store);
        AddHighlightIndex(store);
    }
    MEDIA_INFO_LOG("End VERSION_ADD_HIGHLIGHT_TRIGGER");

    MEDIA_INFO_LOG("Start VERSION_UPDATE_SEARCH_STATUS_TRIGGER_FOR_OWNER_ALBUM_ID");
    UpdateSearchStatusTriggerForOwnerAlbumId(store);
    MEDIA_INFO_LOG("End VERSION_UPDATE_SEARCH_STATUS_TRIGGER_FOR_OWNER_ALBUM_ID");

    MEDIA_INFO_LOG("Start VERSION_HIGHLIGHT_MOVING_PHOTO");
    AddMovingPhotoRelatedData(store);
    MEDIA_INFO_LOG("End VERSION_HIGHLIGHT_MOVING_PHOTO");

    MEDIA_INFO_LOG("Start VERSION_CREATE_TAB_FACARD_PHOTOS");
    TabFaCardPhotosTableEventHandler().OnCreate(store);
    MEDIA_INFO_LOG("End VERSION_CREATE_TAB_FACARD_PHOTOS");

    MEDIA_INFO_LOG("Start VERSION_ADD_FOREGROUND_ANALYSIS");
    if (!IsColumnExists(store, USER_PHOTOGRAPHY_INFO_TABLE, FRONT_INDEX_LIMIT)) {
        AddFrontAnalysisColumn(store);
    }
    MEDIA_INFO_LOG("End VERSION_ADD_FOREGROUND_ANALYSIS");
}

static void AddAssetAlbumOperationTable(RdbStore &store)
{
    const vector<string> executeSqlStrs = {
        "DROP TABLE IF EXISTS tab_asset_and_album_operation",
        CREATE_TAB_ASSET_ALBUM_OPERATION,
        "DROP TABLE IF EXISTS operation_asset_insert_trigger",
        CREATE_OPERATION_ASSET_INSERT_TRIGGER,
        "DROP TABLE IF EXISTS operation_asset_delete_trigger",
        CREATE_OPERATION_ASSET_DELETE_TRIGGER,
        "DROP TABLE IF EXISTS operation_asset_update_trigger",
        CREATE_OPERATION_ASSET_UPDATE_TRIGGER,
        "DROP TABLE IF EXISTS operation_album_insert_trigger",
        CREATE_OPERATION_ALBUM_INSERT_TRIGGER,
        "DROP TABLE IF EXISTS operation_album_delete_trigger",
        CREATE_OPERATION_ALBUM_DELETE_TRIGGER,
        "DROP TABLE IF EXISTS operation_album_update_trigger",
        CREATE_OPERATION_ALBUM_UPDATE_TRIGGER,
    };
    ExecSqls(executeSqlStrs, store);
    MEDIA_INFO_LOG("create asset and album operation table end");
}

static void AddAssetAlbumOperationTableForSync(RdbStore &store)
{
    const vector<string> executeSqlStrs = {
        CREATE_TAB_ASSET_ALBUM_OPERATION,
        "DROP TABLE IF EXISTS operation_asset_insert_trigger",
        CREATE_OPERATION_ASSET_INSERT_TRIGGER,
        "DROP TABLE IF EXISTS operation_asset_delete_trigger",
        CREATE_OPERATION_ASSET_DELETE_TRIGGER,
        "DROP TABLE IF EXISTS operation_asset_update_trigger",
        CREATE_OPERATION_ASSET_UPDATE_TRIGGER,
        "DROP TABLE IF EXISTS operation_album_insert_trigger",
        CREATE_OPERATION_ALBUM_INSERT_TRIGGER,
        "DROP TABLE IF EXISTS operation_album_delete_trigger",
        CREATE_OPERATION_ALBUM_DELETE_TRIGGER,
        "DROP TABLE IF EXISTS operation_album_update_trigger",
        CREATE_OPERATION_ALBUM_UPDATE_TRIGGER,
    };
    ExecSqls(executeSqlStrs, store);
    MEDIA_INFO_LOG("create asset and album operation table sync end");
}

static void UpgradeAnalysisUpdateSearchTrigger(RdbStore &store)
{
    MEDIA_INFO_LOG("start upgrade analysis update search trigger");
    const vector<string> sqls = {
        "DROP TRIGGER IF EXISTS " + ANALYSIS_UPDATE_SEARCH_TRIGGER,
        CREATE_ANALYSIS_UPDATE_SEARCH_TRIGGER,
    };
    ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end upgrade analysis update search trigger");
}

static void AddAnalysisUpdateVideoSearchTrigger(RdbStore &store, int32_t version)
{
    MEDIA_INFO_LOG("Start add analysis update video search trigger");
    const vector<string> sqls = {
        "DROP TRIGGER IF EXISTS " + ANALYSIS_UPDATE_SEARCH_TRIGGER,
        CREATE_ANALYSIS_UPDATE_SEARCH_TRIGGER,
        CREATE_ANALYSIS_UPDATE_VIDEO_SEARCH_TRIGGER,
    };
    ExecSqlsWithDfx(sqls, store, version);
    MEDIA_INFO_LOG("End add analysis update video search trigger");
}

static void CreateTabCustomRecords(RdbStore &store)
{
    const vector<string> executeSqlStrs = {
        CustomRecordsColumns::CREATE_TABLE,
    };
    ExecSqls(executeSqlStrs, store);
    MEDIA_INFO_LOG("create custom and records end");
}

static int32_t AddIsRectificationCover(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_IS_RECTIFICATION_COVER +
            " INT NOT NULL DEFAULT 0",
    };
    MEDIA_INFO_LOG("start add is_rectification_cover column");
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end add is_rectification_cover column");
    return ret;
}

void AddPhotoAlbumRefreshColumns(RdbStore &store)
{
    const vector<string> exeSqls = {
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
        PhotoAlbumColumns::COVER_DATE_TIME + " BIGINT DEFAULT 0",
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
        PhotoAlbumColumns::HIDDEN_COVER_DATE_TIME + " BIGINT DEFAULT 0",
    };

    MEDIA_INFO_LOG("start add photo album cover_date_time and hidden_cover_date_time for AccurateRefresh");
    ExecSqls(exeSqls, store);
    MEDIA_INFO_LOG("end add photo album cover_date_time and hidden_cover_date_time for AccurateRefresh");
}

void AddHighlightLocation(RdbStore &store)
{
    const vector<string> sql = {
        " ALTER TABLE " + HIGHLIGHT_ALBUM_TABLE + " ADD COLUMN " + HIGHLIGHT_LOCATION + " TEXT ",
    };

    MEDIA_INFO_LOG("start add highlight location column");
    ExecSqls(sql, store);
}

void AddIsPrimaryFace(RdbStore &store)
{
    const vector<string> sql = {
        " ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + IS_PRIMARY_FACE + " REAL ",
    };

    MEDIA_INFO_LOG("Adding IS_PRIMARY_FACE column for VISION_IMAGE_FACE_TABLE");
    ExecSqls(sql, store);
}

void AddTotalPriority(RdbStore &store)
{
    const vector<string> sql = {
        "ALTER TABLE " + VISION_TOTAL_TABLE + " ADD COLUMN " + PRIORITY + " INT NOT NULL DEFAULT 1",
    };

    MEDIA_INFO_LOG("Addding priority for VISION_TOTAL_TABLE");
    ExecSqls(sql, store);
}

static void DropInsertSourcePhotoCreateSourceAlbumTrigger(RdbStore &store)
{
    MEDIA_INFO_LOG("drop trigger insert_source_photo_create_source_album_trigger start");
    ExecSqls({DROP_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER}, store);
    MEDIA_INFO_LOG("drop trigger insert_source_photo_create_source_album_trigger end");
}

static void DropInsertPhotoUpdateAlbumBundleNameTrigger(RdbStore &store)
{
    MEDIA_INFO_LOG("drop trigger insert_photo_update_album_bundlename start");
    ExecSqls({DROP_INSERT_PHOTO_UPDATE_ALBUM_BUNDLENAME}, store);
    MEDIA_INFO_LOG("drop trigger insert_photo_update_album_bundlename end");
}

static void DropInsertSourcePhotoUpdateAlbumIdTrigger(RdbStore &store)
{
    MEDIA_INFO_LOG("drop trigger insert_source_photo_update_album_id_trigger start");
    ExecSqls({DROP_INSERT_SOURCE_PHOTO_UPDATE_ALBUM_ID_TRIGGER}, store);
    MEDIA_INFO_LOG("drop trigger insert_source_photo_update_album_id_trigger end");
}

static void AddExifRotateColumn(RdbStore& store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_EXIF_ROTATE +
        " INT NOT NULL DEFAULT 0",
    };
    MEDIA_INFO_LOG("Start add exif_rotate column");
    ExecSqls(sqls, store);
    MEDIA_INFO_LOG("End add exif_rotate column");
}

static void DealWithAlbumMapTrigger(RdbStore &store)
{
    const vector<std::string> exeSqls = {
        SQL_DROP_NEW_TRIGGER,
        SQL_DROP_DELETE_TRIGGER,
        SQL_DROP_INSERT_SEARCH_TRIGGER,
        SQL_DROP_DELETE_SEARCH_TRIGGER,
    };
    MEDIA_INFO_LOG("DealWithAlbumMapTrigger start");
    ExecSqls(exeSqls, store);
    MEDIA_INFO_LOG("DealWithAlbumMapTrigger end");
}

static void AddUriSensitiveColumns(RdbStore &store)
{
    const vector<std::string> exeSqls = {
        "ALTER TABLE " + AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE + " ADD COLUMN " +
        AppUriSensitiveColumn::IS_FORCE_SENSITIVE + " INT DEFAULT 0",
        "ALTER TABLE " + AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE + " ADD COLUMN " +
        AppUriSensitiveColumn::SOURCE_TOKENID + " BIGINT DEFAULT 0",
        "ALTER TABLE " + AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE + " ADD COLUMN " +
        AppUriSensitiveColumn::TARGET_TOKENID + " BIGINT DEFAULT 0",
    };
    MEDIA_INFO_LOG("AddUriSensitiveColumns start");
    ExecSqls(exeSqls, store);
    MEDIA_INFO_LOG("AddUriSensitiveColumns end");
}

static void AddFileSourceType(RdbStore &store, int32_t version)
{
    const vector<string> sqls = {
        "DROP TRIGGER IF EXISTS " + INSERT_SEARCH_TRIGGER,
        CREATE_SEARCH_INSERT_TRIGGER,
        DROP_INSERT_VISION_TRIGGER,
        CREATE_VISION_INSERT_TRIGGER_FOR_ONCREATE,
        "DROP TRIGGER IF EXISTS operation_asset_insert_trigger",
        CREATE_OPERATION_ASSET_INSERT_TRIGGER,
        "DROP TRIGGER IF EXISTS operation_asset_delete_trigger",
        CREATE_OPERATION_ASSET_DELETE_TRIGGER,
        "DROP TRIGGER IF EXISTS operation_asset_update_trigger",
        CREATE_OPERATION_ASSET_UPDATE_TRIGGER,
        DROP_INSERT_PHOTO_UPDATE_ALBUM_BUNDLENAME,
        INSERT_PHOTO_UPDATE_ALBUM_BUNDLENAME,
        DROP_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER,
        CREATE_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER,
        "DROP TRIGGER IF EXISTS photos_metadata_dirty_trigger",
        PhotoUpgrade::CREATE_PHOTOS_METADATA_DIRTY_TRIGGER,
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " INT NOT NULL DEFAULT 0 ",
        "ALTER TABLE " + AudioColumn::AUDIOS_TABLE + " ADD COLUMN " +
            AudioColumn::AUDIO_FILE_SOURCE_TYPE + " INT NOT NULL DEFAULT 0 "
    };
    MEDIA_INFO_LOG("AddFileSourceType start");
    ExecSqlsWithDfx(sqls, store, version);
    MEDIA_INFO_LOG("AddFileSourceType end");
}

static void AddAspectRatio(RdbStore &store, int32_t version)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
        PhotoColumn::PHOTO_ASPECT_RATIO + " DOUBLE NOT NULL DEFAULT -2 ",
    };
    MEDIA_INFO_LOG("AddAspectRatio start");
    ExecSqlsWithDfx(sqls, store, version);
    MEDIA_INFO_LOG("AddAspectRatio end");
}

static void AddTempFileAssetsCreateAlbum(RdbStore &store)
{
    const vector<string> sqls = {
        DROP_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER,
        CREATE_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER
    };
    MEDIA_INFO_LOG("AddTempFileAssetsCreateAlbum start");
    ExecSqls(sqls, store);
    MEDIA_INFO_LOG("AddTempFileAssetsCreateAlbum end");
}

static void UpgradeFromAllVersionFirstPart(RdbStore &store, unordered_map<string, bool> &photoColumnExists)
{
    MEDIA_INFO_LOG("Start VERSION_ADD_DETAIL_TIME");
    if (photoColumnExists.find(PhotoColumn::PHOTO_DETAIL_TIME) == photoColumnExists.end() ||
        !photoColumnExists.at(PhotoColumn::PHOTO_DETAIL_TIME)) {
        int32_t errCode = 0;
        shared_ptr<NativePreferences::Preferences> prefs =
            NativePreferences::PreferencesHelper::GetPreferences(RDB_FIX_RECORDS, errCode);
        if (prefs != nullptr) {
            // before current version, detail time column has existed, need to fix other information
            prefs->PutInt(DETAIL_TIME_FIXED, NEED_FIXED);
            prefs->FlushSync();
            MEDIA_INFO_LOG("DETAIL_TIME_FIXED set to: %{public}d", NEED_FIXED);
        }
        MEDIA_INFO_LOG("DETAIL_TIME_FIXED prefs errCode: %{public}d", errCode);
        AddDetailTimeToPhotos(store);
    }
    MEDIA_INFO_LOG("End VERSION_ADD_DETAIL_TIME");

    MEDIA_INFO_LOG("Start VERSION_ADD_OWNER_ALBUM_ID");
    DropPhotoAlbumClearMap(store);
    MEDIA_INFO_LOG("End VERSION_ADD_OWNER_ALBUM_ID");

    MEDIA_INFO_LOG("Start VERSION_ADD_THUMBNAIL_VISIBLE");
    if (photoColumnExists.find(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE) == photoColumnExists.end() ||
        !photoColumnExists.at(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE)) {
        int32_t errCode = 0;
        shared_ptr<NativePreferences::Preferences> prefs =
            NativePreferences::PreferencesHelper::GetPreferences(RDB_FIX_RECORDS, errCode);
        if (prefs != nullptr) {
            // before current version, thumbnail visible column has existed, need to fix other information
            prefs->PutInt(THUMBNAIL_VISIBLE_FIXED, NEED_FIXED);
            prefs->FlushSync();
            MEDIA_INFO_LOG("THUMBNAIL_VISIBLE_FIXED set to: %{public}d", NEED_FIXED);
        }
        MEDIA_INFO_LOG("THUMBNAIL_VISIBLE_FIXED prefs errCode: %{public}d", errCode);
        AddThumbnailVisible(store);
    }
    MEDIA_INFO_LOG("End VERSION_ADD_THUMBNAIL_VISIBLE");

    MEDIA_INFO_LOG("Start VERSION_ADD_VIDEO_FACE_TABLE");
    if (!IsColumnExists(store, VISION_TOTAL_TABLE, GEO)) {
        AddVideoFaceTable(store);
    }
    MEDIA_INFO_LOG("End VERSION_ADD_VIDEO_FACE_TABLE");

    MEDIA_INFO_LOG("Start VERSION_ADD_HIGHLIGHT_MAP_TABLES");
    if (!IsColumnExists(store, HIGHLIGHT_PLAY_INFO_TABLE, HIGHLIGHTING_ALGO_VERSION)) {
        AddHighlightMapTable(store);
    }
    MEDIA_INFO_LOG("End VERSION_ADD_HIGHLIGHT_MAP_TABLES");

    MEDIA_INFO_LOG("Start VERSION_ADD_CLOUD_ENHANCEMENT_ALBUM");
    AddCloudEnhancementAlbum(store);
    MEDIA_INFO_LOG("End VERSION_ADD_CLOUD_ENHANCEMENT_ALBUM");
}

static void UpgradeFromAllVersionSecondPart(RdbStore &store, unordered_map<string, bool> &photoColumnExists)
{
    MEDIA_INFO_LOG("Start VERSION_CREATE_TAB_OLD_PHOTOS");
    TabOldPhotosTableEventHandler().OnCreate(store);
    MEDIA_INFO_LOG("End VERSION_CREATE_TAB_OLD_PHOTOS");

    MEDIA_INFO_LOG("Start VERSION_UPDATE_SEARCH_INDEX_TRIGGER_FOR_CLEAN_FLAG");
    UpdateSearchIndexTriggerForCleanFlag(store);
    MEDIA_INFO_LOG("End VERSION_UPDATE_SEARCH_INDEX_TRIGGER_FOR_CLEAN_FLAG");

    MEDIA_INFO_LOG("Start VERSION_ADD_COVER_PLAY_SERVICE_VERSION");
    if (!IsColumnExists(store, HIGHLIGHT_COVER_INFO_TABLE, COVER_SERVICE_VERSION)) {
        AddCoverPlayVersionColumns(store);
    }
    MEDIA_INFO_LOG("End VERSION_ADD_COVER_PLAY_SERVICE_VERSION");

    MEDIA_INFO_LOG("Start VERSION_ADD_SUPPORTED_WATERMARK_TYPE");
    if (photoColumnExists.find(PhotoColumn::SUPPORTED_WATERMARK_TYPE) == photoColumnExists.end() ||
        !photoColumnExists.at(PhotoColumn::SUPPORTED_WATERMARK_TYPE)) {
        AddSupportedWatermarkType(store);
    }
    MEDIA_INFO_LOG("End VERSION_ADD_SUPPORTED_WATERMARK_TYPE");

    MEDIA_INFO_LOG("Start VERSION_UDAPTE_AOI");
    if (!IsColumnExists(store, GEO_KNOWLEDGE_TABLE, AOI)) {
        UpdateAOI(store);
        AddGeoDefaultValue(store);
    }
    MEDIA_INFO_LOG("End VERSION_UDAPTE_AOI");

    MEDIA_INFO_LOG("Start VERSION_HDR_AND_CLOUD_ENHANCEMENT_FIX");
    AddDynamicRangeColumnsFix(store);
    AddCloudEnhanceColumnsFix(store);
    MEDIA_INFO_LOG("End VERSION_HDR_AND_CLOUD_ENHANCEMENT_FIX");

    MEDIA_INFO_LOG("Start VERSION_THUMBNAIL_READY_FIX");
    AddThumbnailReadyColumnsFix(store);
    MEDIA_INFO_LOG("End VERSION_THUMBNAIL_READY_FIX");

    MEDIA_INFO_LOG("Start VERSION_UPDATE_SOURCE_PHOTO_ALBUM_TRIGGER");
    UpdateSourcePhotoAlbumTrigger(store);
    MEDIA_INFO_LOG("End VERSION_UPDATE_SOURCE_PHOTO_ALBUM_TRIGGER");

    MEDIA_INFO_LOG("Start VERSION_UPDATE_URIPERMISSION_SOURCE_TOKEN_AND_TARGET_TOKEN");
    if (!IsColumnExists(store, AppUriPermissionColumn::APP_URI_PERMISSION_TABLE,
        AppUriPermissionColumn::SOURCE_TOKENID)) {
        AddSourceAndTargetTokenForUriPermission(store);
    }
    MEDIA_INFO_LOG("End VERSION_UPDATE_URIPERMISSION_SOURCE_TOKEN_AND_TARGET_TOKEN");
}

static void UpgradeFromAllVersionThirdPart(RdbStore &store, unordered_map<string, bool> &photoColumnExists)
{
    MEDIA_INFO_LOG("Start VERSION_HIGHLIGHT_CHANGE_FUNCTION");
    if (!IsColumnExists(store, ANALYSIS_PHOTO_MAP_TABLE, ORDER_POSITION)) {
        AddHighlightChangeFunction(store);
    }
    MEDIA_INFO_LOG("End VERSION_HIGHLIGHT_CHANGE_FUNCTION");

    MEDIA_INFO_LOG("Start VERSION_ADD_HIGHLIGHT_ANALYSIS_PROGRESS");
    if (!IsColumnExists(store, USER_PHOTOGRAPHY_INFO_TABLE, HIGHLIGHT_ANALYSIS_PROGRESS)) {
        AddHighlightAnalysisProgress(store);
    }
    MEDIA_INFO_LOG("End VERSION_ADD_HIGHLIGHT_ANALYSIS_PROGRESS");

    MEDIA_INFO_LOG("Start VERSION_ADD_CHECK_FLAG");
    if (photoColumnExists.find(PhotoColumn::PHOTO_CHECK_FLAG) == photoColumnExists.end() ||
        !photoColumnExists.at(PhotoColumn::PHOTO_CHECK_FLAG)) {
        AddCheckFlag(store);
    }
    MEDIA_INFO_LOG("End VERSION_ADD_CHECK_FLAG");

    MEDIA_INFO_LOG("Start VERSION_FIX_SOURCE_PHOTO_ALBUM_DATE_MODIFIED");
    UpdateSourcePhotoAlbumTrigger(store);
    MEDIA_INFO_LOG("End VERSION_FIX_SOURCE_PHOTO_ALBUM_DATE_MODIFIED");

    MEDIA_INFO_LOG("Start VERSION_FIX_SOURCE_ALBUM_UPDATE_TRIGGER_TO_USE_LPATH");
    FixSourceAlbumUpdateTriggerToUseLPath(store);
    MEDIA_INFO_LOG("End VERSION_FIX_SOURCE_ALBUM_UPDATE_TRIGGER_TO_USE_LPATH");

    MEDIA_INFO_LOG("Start VERSION_ADD_REFRESH_ALBUM_STATUS_COLUMN");
    if (!IsColumnExists(store, ALBUM_REFRESH_TABLE, ALBUM_REFRESH_STATUS)) {
        AddRefreshAlbumStatusColumn(store);
    }
    MEDIA_INFO_LOG("End VERSION_ADD_REFRESH_ALBUM_STATUS_COLUMN");

    MEDIA_INFO_LOG("Start VERSION_UPDATE_CLOUD_TRIGGER");
    UpdateCloudTrigger(store);
    MEDIA_INFO_LOG("End VERSION_UPDATE_CLOUD_TRIGGER");

    MEDIA_INFO_LOG("Start VERSION_ADD_STAGE_VIDEO_TASK_STATUS");
    if (photoColumnExists.find(PhotoColumn::STAGE_VIDEO_TASK_STATUS) == photoColumnExists.end() ||
        !photoColumnExists.at(PhotoColumn::STAGE_VIDEO_TASK_STATUS)) {
        AddStageVideoTaskStatus(store);
    }
    MEDIA_INFO_LOG("End VERSION_ADD_STAGE_VIDEO_TASK_STATUS");
}

static void UpgradeFromAllVersionFourthPart(RdbStore &store, unordered_map<string, bool> &photoColumnExists)
{
    MEDIA_INFO_LOG("Start VERSION_CREATE_TAB_ASSET_ALBUM_OPERATION_FOR_SYNC");
    AddAssetAlbumOperationTableForSync(store);
    MEDIA_INFO_LOG("End VERSION_CREATE_TAB_ASSET_ALBUM_OPERATION_FOR_SYNC");

    MEDIA_INFO_LOG("Start VERSION_UPGRADE_ANALYSIS_UPDATE_SEARCH_TRIGGER");
    UpgradeAnalysisUpdateSearchTrigger(store);
    MEDIA_INFO_LOG("End VERSION_UPGRADE_ANALYSIS_UPDATE_SEARCH_TRIGGER");

    MEDIA_INFO_LOG("Start VERSION_ADD_DC_ANALYSIS");
    if (!IsColumnExists(store, USER_PHOTOGRAPHY_INFO_TABLE, DC_INDEX_COUNT)) {
        AddDcAnalysisColumn(store);
    }
    MEDIA_INFO_LOG("End VERSION_ADD_DC_ANALYSIS");

    MEDIA_INFO_LOG("Start VERSION_CLOUD_MEDIA_UPGRADE");
    DealWithAlbumMapTrigger(store);
    MEDIA_INFO_LOG("End VERSION_CLOUD_MEDIA_UPGRADE");

    MEDIA_INFO_LOG("Start VERSION_ADD_DC_ANALYSIS_INDEX_UPDATE");
    if (!IsColumnExists(store, USER_PHOTOGRAPHY_INFO_TABLE, DC_INDEX_UPDATE_COUNT)) {
        AddDcAnalysisIndexUpdateColumn(store);
    }
    MEDIA_INFO_LOG("End VERSION_ADD_DC_ANALYSIS_INDEX_UPDATE");

    MEDIA_INFO_LOG("Start VERSION_ADD_VISIT_COUNT");
    if (photoColumnExists.find(PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME) == photoColumnExists.end() ||
        !photoColumnExists.at(PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME)) {
        AddVisitCountColumn(store);
    }
    MEDIA_INFO_LOG("End VERSION_ADD_VISIT_COUNT");

    MEDIA_INFO_LOG("Start ADD_URI_SENSITIVE_COLUMNS");
    if (!IsColumnExists(store,  AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE,
        AppUriSensitiveColumn::IS_FORCE_SENSITIVE)) {
        AddUriSensitiveColumns(store);
    }
    MEDIA_INFO_LOG("End ADD_URI_SENSITIVE_COLUMNS");
}

static void AddAnalysisProgress(RdbStore &store, int32_t version)
{
    const vector<string> exeSqls = {
        CREATE_TAB_ANALYSIS_PROGRESS,
    };
    MEDIA_INFO_LOG("start add analysis progress table");
    ExecSqlsWithDfx(exeSqls, store, version);
    MEDIA_INFO_LOG("end add analysis progress table");
}

static void AddCloneSequenceColumns(RdbStore &store, int32_t version)
{
    const vector<string> sqls = {
        "ALTER TABLE " + TabOldPhotosColumn::OLD_PHOTOS_TABLE + " ADD COLUMN " +
            TabOldPhotosColumn::MEDIA_CLONE_SEQUENCE + " INTEGER"
    };
    MEDIA_INFO_LOG("add tab_old_photos clone_sequence columns start");
    ExecSqlsWithDfx(sqls, store, version);
    MEDIA_INFO_LOG("add tab_old_photos clone_sequence columns end");
}

static void AddAlbumOrderBackTable(RdbStore &store, int32_t version)
{
    const vector<string> sqls = { CREATE_ALBUM_ORDER_BACK_TABLE };
    MEDIA_INFO_LOG("create album_order_back table start");
    ExecSqlsWithDfx(sqls, store, version);
    MEDIA_INFO_LOG("create album_order_back table end");
}

static void AddImageFaceDetail(RdbStore &store, int32_t version)
{
    MEDIA_INFO_LOG("start to add image face detail");
    const vector<string> sqls = {
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + FACE_EYE_CLOSE + " REAL",
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + FACE_DETAIL_VERSION + " TEXT",
    };
    ExecSqlsWithDfx(sqls, store, version);
}

static void AddImageFaceAndFaceTagAgeGender(RdbStore &store, int32_t version)
{
    MEDIA_INFO_LOG("start to add age and gender for image face and face tag");
    const vector<string> sqls = {
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + AGE + " DOUBLE",
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + GENDER + " INTEGER",
        "ALTER TABLE " + VISION_FACE_TAG_TABLE + " ADD COLUMN " + AGE + " DOUBLE",
        "ALTER TABLE " + VISION_FACE_TAG_TABLE + " ADD COLUMN " + GENDER + " INTEGER",
    };
    ExecSqlsWithDfx(sqls, store, version);
    MEDIA_INFO_LOG("end to add age and gender for image face and face tag");
}

static void AddAnalysisProgressColumns(RdbStore &store, int32_t version)
{
    const vector<string> sqls = {
        ADD_EXTRA_QUOTA_INDEX_BUILD_CNT_COLUMN,
        ADD_EXTRA_QUOTA_INDEX_UPDATE_CNT_COLUMN,
        ADD_EXTRA_QUOTA_INDEX_DELETE_CNT_COLUMN,
        ADD_EXTRA_QUOTA_OCR_CNT_COLUMN,
        ADD_EXTRA_QUOTA_SHARED_BACKBONE_CNT_COLUMN,
        ADD_EXTRA_QUOTA_MODIFY_TIME_COLUMN,
        ADD_BASE_QUOTA_INDEX_BUILD_CNT_COLUMN,
        ADD_BASE_QUOTA_INDEX_UPDATE_CNT_COLUMN,
        ADD_BASE_QUOTA_INDEX_DELETE_CNT_COLUMN,
        ADD_BASE_QUOTA_OCR_CNT_COLUMN,
        ADD_BASE_QUOTA_SHARED_BACKBONE_CNT_COLUMN,
        ADD_BASE_QUOTA_LABEL_CNT_COLUMN,
        ADD_BASE_QUOTA_MODIFY_TIME_COLUMN,
    };
    MEDIA_INFO_LOG("start add analysis progress columns");
    ExecSqlsWithDfx(sqls, store, version);
    MEDIA_INFO_LOG("end add analysis progress columns");
}

static void AddAnalysisProgressCheckSpaceColumn(RdbStore &store, int32_t version)
{
    const vector<string> sqls = {
        ADD_CHECK_SPACE_FLAG_COLUMN,
    };
    MEDIA_INFO_LOG("start add tab_analysis_progress check_space_flag column");
    ExecSqlsWithDfx(sqls, store, version);
    MEDIA_INFO_LOG("end add tab_analysis_progress check_space_flag column");
}

static void CreateVisionVideoTotal(RdbStore& store, int32_t version)
{
    const vector<string> sqls = {
        CREATE_TAB_ANALYSIS_VIDEO_TOTAL,
        DROP_INSERT_VISION_TRIGGER,
        CREATE_VISION_INSERT_TRIGGER_FOR_ONCREATE,
        DROP_UPDATE_VISION_TRIGGER,
        CREATE_VISION_UPDATE_TRIGGER,
        DROP_DELETE_VISION_TRIGGER,
        CREATE_VISION_DELETE_TRIGGER,
    };
    MEDIA_INFO_LOG("start create video total");
    ExecSqlsWithDfx(sqls, store, version);
}

static void CreateChangeTime(RdbStore &store, int32_t version)
{
    MEDIA_INFO_LOG("start add change_time column");
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_CHANGE_TIME +
            " BIGINT NOT NULL DEFAULT 0",
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " + PhotoAlbumColumns::CHANGE_TIME +
            " BIGINT NOT NULL DEFAULT 0",
    };
    ExecSqlsWithDfx(sqls, store, version);
    MEDIA_INFO_LOG("end add change_time column");
}

static void AddAudioIsTemp(RdbStore &store, int32_t version)
{
    const vector<string> sqls = {
        "ALTER TABLE " + AudioColumn::AUDIOS_TABLE + " ADD COLUMN " +
            AudioColumn::AUDIO_IS_TEMP + " INT DEFAULT 0"
    };
    MEDIA_INFO_LOG("AddAudioIsTemp start");
    ExecSqlsWithDfx(sqls, store, version);
    MEDIA_INFO_LOG("AddAudioIsTemp end");
}

static void CreateBatchDownloadRecords(RdbStore &store, int32_t version)
{
    MEDIA_INFO_LOG("create batchdownload records begin");
    const vector<string> executeSqlStrs = {
        DownloadResourcesColumn::CREATE_TABLE,
        DownloadResourcesColumn::INDEX_DRTR_ID_STATUS,
    };
    ExecSqlsWithDfx(executeSqlStrs, store, version);
    MEDIA_INFO_LOG("create batchdownload records end");
}

static void UpdateMdirtyTriggerForStrongAssociation(RdbStore &store, int32_t version)
{
    const vector<string> sqls = {
        "DROP TRIGGER IF EXISTS photos_mdirty_trigger",
        PhotoUpgrade::CREATE_PHOTOS_MDIRTY_TRIGGER,
    };
    ExecSqlsWithDfx(sqls, store, version);
    MEDIA_INFO_LOG("Update mdirty trigger for strong association end");
}

static void UpdateSourceAlbumBundleNameTriggerUseLpath(RdbStore &store, int32_t version)
{
    const vector<string> sqls = {
        DROP_INSERT_PHOTO_UPDATE_ALBUM_BUNDLENAME,
        INSERT_PHOTO_UPDATE_ALBUM_BUNDLENAME,
    };
    MEDIA_INFO_LOG("start update source album bundle name trigger use lpath");
    ExecSqlsWithDfx(sqls, store, version);
    MEDIA_INFO_LOG("end update source album bundle name trigger use lpath");
}

static void AddEditOperation(RdbStore &store, int32_t version)
{
    static const vector<string> executeSqlStrs = {
        "ALTER TABLE " + ANALYSIS_ALBUM_TABLE + " ADD COLUMN " + EDIT_OPERATION + " INT ",
    };
    MEDIA_INFO_LOG("add edit operation column start");
    ExecSqlsWithDfx(executeSqlStrs, store, version);
    MEDIA_INFO_LOG("start add edit operation column end");
}

static void AddPhotoAlbumHidden(RdbStore &store, int32_t version)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
            PhotoAlbumColumns::ALBUM_HIDDEN + " INT NOT NULL DEFAULT 0",
    };
    MEDIA_INFO_LOG("Add photoalbum hidden column start");
    ExecSqlsWithDfx(sqls, store, version);
    MEDIA_INFO_LOG("Add photoalbum hidden column end");
}

static void UpdateMdirtyTrigger(RdbStore &store, int32_t version, const std::string &columnName)
{
    MEDIA_INFO_LOG("Update mdirty trigger for %{public}s start", columnName.c_str());
    const vector<string> sqls = {
        "DROP TRIGGER IF EXISTS photos_mdirty_trigger",
        PhotoUpgrade::CREATE_PHOTOS_MDIRTY_TRIGGER,
    };
    ExecSqlsWithDfx(sqls, store, version);
    MEDIA_INFO_LOG("Update mdirty trigger for %{public}s end", columnName.c_str());
}

static void UpgradeExtensionPart14(RdbStore &store, int32_t oldVersion)
{
    if (oldVersion < VERSION_ADD_PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE, true)) {
        AddPhotoMovingphotoEnhancementType(store, VERSION_ADD_PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE, true);
    }

    if (oldVersion < VERSION_ADD_CRITICAL_TYPE_COLUMN_ON_PHOTOS &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_CRITICAL_TYPE_COLUMN_ON_PHOTOS, true)) {
        AddCriticalTypeColumns(store, VERSION_ADD_CRITICAL_TYPE_COLUMN_ON_PHOTOS);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_CRITICAL_TYPE_COLUMN_ON_PHOTOS, true);
    }

    if (oldVersion < VERSION_ADD_PHOTO_RISK_STATUS &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_PHOTO_RISK_STATUS, true)) {
        AddPhotoRiskStatusColumnsAndDeleteCritical(store, VERSION_ADD_PHOTO_RISK_STATUS);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_PHOTO_RISK_STATUS, true);
    }

    if (oldVersion < VERSION_FIX_MINECRAFT_PE && !RdbUpgradeUtils::HasUpgraded(VERSION_FIX_MINECRAFT_PE, true)) {
        int32_t ret = AlbumPluginTableEventHandler().FixMinecraftPE(store, oldVersion, MEDIA_RDB_VERSION);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_FIX_MINECRAFT_PE, true);
        RdbUpgradeUtils::AddUpgradeDfxMessages(VERSION_FIX_MINECRAFT_PE, 0, ret);
    }
}

static void UpgradeExtensionPart13(RdbStore &store, int32_t oldVersion)
{
    if (oldVersion < VERSION_ADD_PET_TABLES &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_PET_TABLES, true)) {
        AddPetFaceTable(store, VERSION_ADD_PET_TABLES);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_PET_TABLES, true);
    }

    if (oldVersion < VERSION_ADD_ASPECT_RATIO &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_ASPECT_RATIO, true)) {
        AddAspectRatio(store, VERSION_ADD_ASPECT_RATIO);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_ASPECT_RATIO, true);
    }

    if (oldVersion < VERSION_SOURCE_ALBUM_BUNDLE_UPDATE_TRIGGER_USE_LPATH &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_SOURCE_ALBUM_BUNDLE_UPDATE_TRIGGER_USE_LPATH, true)) {
        UpdateSourceAlbumBundleNameTriggerUseLpath(store, VERSION_SOURCE_ALBUM_BUNDLE_UPDATE_TRIGGER_USE_LPATH);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_SOURCE_ALBUM_BUNDLE_UPDATE_TRIGGER_USE_LPATH, true);
    }

    if (oldVersion < VERSION_ADD_PHOTO_ALBUM_HIDDEN &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_PHOTO_ALBUM_HIDDEN, true)) {
        AddPhotoAlbumHidden(store, VERSION_ADD_PHOTO_ALBUM_HIDDEN);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_PHOTO_ALBUM_HIDDEN, true);
    }

    if (oldVersion < VERSION_ADD_CHANGE_TIME &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_CHANGE_TIME, true)) {
        CreateChangeTime(store, VERSION_ADD_CHANGE_TIME);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_CHANGE_TIME, true);
    }

    if (oldVersion < VERSION_ADD_EDIT_OPERATION &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_EDIT_OPERATION, true)) {
        AddEditOperation(store, VERSION_ADD_EDIT_OPERATION);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_EDIT_OPERATION, true);
    }

    if (oldVersion < VERSION_UPDATE_MDIRTY_TRIGGER_FOR_HDR_MODE &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_UPDATE_MDIRTY_TRIGGER_FOR_HDR_MODE, true)) {
        CreateChangeTime(store, VERSION_UPDATE_MDIRTY_TRIGGER_FOR_HDR_MODE);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_UPDATE_MDIRTY_TRIGGER_FOR_HDR_MODE, true);
    }

    if (oldVersion < VERSION_ADD_AUDIO_IS_TEMP &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_AUDIO_IS_TEMP, true)) {
        AddAudioIsTemp(store, VERSION_ADD_AUDIO_IS_TEMP);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_AUDIO_IS_TEMP, true);
    }

    if (oldVersion < VERSION_ADD_ANALYSIS_UPDATE_SEARCH_TRIGGER &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_ANALYSIS_UPDATE_SEARCH_TRIGGER, true)) {
        AddAnalysisUpdateVideoSearchTrigger(store, VERSION_ADD_ANALYSIS_UPDATE_SEARCH_TRIGGER);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_ANALYSIS_UPDATE_SEARCH_TRIGGER, true);
    }
    UpgradeExtensionPart14(store, oldVersion);
}

static void UpgradeExtensionPart12(RdbStore &store, int32_t oldVersion)
{
    MEDIA_INFO_LOG("start update vision trigger");
    if (oldVersion < VERSION_UPDATE_VIDEO_LABLE_FACE &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_UPDATE_VIDEO_LABLE_FACE, true)) {
        CreateVisionVideoTotal(store, VERSION_UPDATE_VIDEO_LABLE_FACE);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_UPDATE_VIDEO_LABLE_FACE, true);
    }

    if (oldVersion < VERSION_UPDATE_MDIRTY_TRIGGER_FOR_STRONG_ASSOCIATION &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_UPDATE_MDIRTY_TRIGGER_FOR_STRONG_ASSOCIATION, true)) {
        MEDIA_INFO_LOG("Update mdirty trigger for strong association start");
        UpdateMdirtyTriggerForStrongAssociation(store, VERSION_UPDATE_MDIRTY_TRIGGER_FOR_STRONG_ASSOCIATION);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_UPDATE_MDIRTY_TRIGGER_FOR_STRONG_ASSOCIATION, true);
    }

    if (oldVersion < VERSION_ADD_IMAGE_FACE_AND_FACE_TAG_AGE_GENDER &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_IMAGE_FACE_AND_FACE_TAG_AGE_GENDER, true)) {
        AddImageFaceAndFaceTagAgeGender(store, VERSION_ADD_IMAGE_FACE_AND_FACE_TAG_AGE_GENDER);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_IMAGE_FACE_AND_FACE_TAG_AGE_GENDER, true);
    }

    if (oldVersion < VERSION_ADD_QUICK_CAPTURE_AND_TIME_LAPSE &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_QUICK_CAPTURE_AND_TIME_LAPSE, true)) {
        PrepareOtherShootingModeAlbum(store, VERSION_ADD_QUICK_CAPTURE_AND_TIME_LAPSE);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_QUICK_CAPTURE_AND_TIME_LAPSE, true);
    }

    if (oldVersion < VERSION_ADD_LAKE_COLUMN &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_LAKE_COLUMN, true)) {
        AddLakeFileColumn(store, VERSION_ADD_LAKE_COLUMN);
        AddLakeAlbumTable(store, VERSION_ADD_LAKE_COLUMN);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_LAKE_COLUMN, true);
    }

    if (oldVersion < VERSION_FILTER_TAB_ASSET_ALBUM_OPERATION &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_FILTER_TAB_ASSET_ALBUM_OPERATION, true)) {
        AddAssetAlbumOperationTableForSync(store);
        UpdateAlbumPlugin(store, VERSION_FILTER_TAB_ASSET_ALBUM_OPERATION);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_FILTER_TAB_ASSET_ALBUM_OPERATION, true);
    }

    if (oldVersion < VERSION_PHOTO_ALBUM_ADD_UPLOAD_STATUS &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_PHOTO_ALBUM_ADD_UPLOAD_STATUS, true)) {
        AddUploadStatus(store);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_PHOTO_ALBUM_ADD_UPLOAD_STATUS, true);
    }

    if (oldVersion < VERSION_ADD_TAB_ANALYSIS_PROGRESS_CHECK_SPACE_COLUMN &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_TAB_ANALYSIS_PROGRESS_CHECK_SPACE_COLUMN, true)) {
        AddAnalysisProgressCheckSpaceColumn(store, VERSION_ADD_TAB_ANALYSIS_PROGRESS_CHECK_SPACE_COLUMN);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_TAB_ANALYSIS_PROGRESS_CHECK_SPACE_COLUMN, true);
    }
    UpgradeExtensionPart13(store, oldVersion);
}

static void UpgradeExtensionPart11(RdbStore &store, int32_t oldVersion)
{
    if (oldVersion < VERSION_CREATE_TAB_OLD_ALBUM &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_CREATE_TAB_OLD_ALBUM, true)) {
        TabOldAlbumTableEventHandler().OnCreate(store);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_CREATE_TAB_OLD_ALBUM, true);
    }

    if (oldVersion < VERSION_ADD_IMAGE_FACE_DETAIL &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_IMAGE_FACE_DETAIL, true)) {
        AddImageFaceDetail(store, VERSION_ADD_IMAGE_FACE_DETAIL);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_IMAGE_FACE_DETAIL, true);
    }

    if (oldVersion < VERSION_ADD_TAB_ANALYSIS_PROGRESS_COLUMNS &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_TAB_ANALYSIS_PROGRESS_COLUMNS, true)) {
        AddAnalysisProgressColumns(store, VERSION_ADD_TAB_ANALYSIS_PROGRESS_COLUMNS);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_TAB_ANALYSIS_PROGRESS_COLUMNS, true);
    }

    if (oldVersion < VERSION_ADD_BATCH_DOWNLOAD &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_BATCH_DOWNLOAD, true)) {
        CreateBatchDownloadRecords(store, VERSION_ADD_BATCH_DOWNLOAD);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_BATCH_DOWNLOAD, true);
    }

    if (oldVersion < VERSION_VIDEO_MODE &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_VIDEO_MODE, true)) {
        MEDIA_INFO_LOG("AddVideoMode start");
        AddVideoMode(store, VERSION_VIDEO_MODE);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_VIDEO_MODE, true);
    }

    if (oldVersion < VERSION_CREATE_TAB_CLONED_OLD_PHOTOS &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_CREATE_TAB_CLONED_OLD_PHOTOS, true)) {
        MEDIA_INFO_LOG("CreateTabClonedOldPhotos start");
        int32_t ret = TabClonedOldPhotosTableEventHandler().OnCreate(store);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_CREATE_TAB_CLONED_OLD_PHOTOS, true);
        RdbUpgradeUtils::AddUpgradeDfxMessages(VERSION_CREATE_TAB_CLONED_OLD_PHOTOS, 0, ret);
    }
    UpgradeExtensionPart12(store, oldVersion);
}

static void UpgradeExtensionPart10(RdbStore &store, int32_t oldVersion)
{
    if (oldVersion < VERSION_ADD_ALBUM_ORDER_BACK_VERSION &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_ALBUM_ORDER_BACK_VERSION, true)) {
        AddAlbumOrderBackTable(store, VERSION_ADD_ALBUM_ORDER_BACK_VERSION);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_ALBUM_ORDER_BACK_VERSION, true);
    }

    if (oldVersion < VERSION_ADD_TAB_OLD_PHOTOS_CLONE_SEQUENCE &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_TAB_OLD_PHOTOS_CLONE_SEQUENCE, true)) {
        AddCloneSequenceColumns(store, VERSION_ADD_TAB_OLD_PHOTOS_CLONE_SEQUENCE);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_TAB_OLD_PHOTOS_CLONE_SEQUENCE, true);
    }

    if (oldVersion < VERSION_ADD_FILE_SOURCE_TYPE &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_FILE_SOURCE_TYPE, true)) {
        AddFileSourceType(store, VERSION_ADD_FILE_SOURCE_TYPE);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_FILE_SOURCE_TYPE, true);
    }

    if (oldVersion < VERSION_ADD_HDR_MODE &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_HDR_MODE, true)) {
        AddHdrMode(store, VERSION_ADD_HDR_MODE);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_HDR_MODE, true);
    }

    if (oldVersion < VERSION_ADD_ANALYSIS_STATUS &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_ANALYSIS_STATUS, true)) {
        AddAnalysisStatus(store, VERSION_ADD_ANALYSIS_STATUS);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_ANALYSIS_STATUS, true);
    }

    if (oldVersion < VERSION_ADD_TEMP_FILE_ASSETS_CREATE_ALBUM &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_TEMP_FILE_ASSETS_CREATE_ALBUM, true)) {
        AddTempFileAssetsCreateAlbum(store);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_TEMP_FILE_ASSETS_CREATE_ALBUM, true);
    }

    if (oldVersion < VERSION_ADD_AFFECTIVE_TABLE &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_AFFECTIVE_TABLE, true)) {
        AddAffective(store, VERSION_ADD_AFFECTIVE_TABLE);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_AFFECTIVE_TABLE, true);
    }

    if (oldVersion < VERSION_ADD_3DGS_MODE &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_3DGS_MODE, true)) {
        Prepare3DGSModeAlbum(store, VERSION_ADD_3DGS_MODE);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_3DGS_MODE, true);
    }

    UpgradeExtensionPart11(store, oldVersion);
}

static void UpgradeExtensionPart9(RdbStore &store, int32_t oldVersion)
{
    if (oldVersion < VERSION_ADD_RELATIONSHIP_AND_UPDATE_TRIGGER &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_RELATIONSHIP_AND_UPDATE_TRIGGER, true)) {
        UpdateAnalysisAlbumRelationship(store, VERSION_ADD_RELATIONSHIP_AND_UPDATE_TRIGGER);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_RELATIONSHIP_AND_UPDATE_TRIGGER, true);
    }

    if (oldVersion < VERSION_ADD_APPLINK_VERSION &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_APPLINK_VERSION, true)) {
        AddAppLinkColumn(store, VERSION_ADD_APPLINK_VERSION);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_APPLINK_VERSION, true);
    }
    if (oldVersion < VERSION_CREATE_TMP_COMPATIBLE_DUP &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_CREATE_TMP_COMPATIBLE_DUP, true)) {
        AddCreateTmpCompatibleDup(store, VERSION_CREATE_TMP_COMPATIBLE_DUP);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_CREATE_TMP_COMPATIBLE_DUP, true);
    }

    if (oldVersion < VERSION_ADD_MEDIA_BACKUP_INFO &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_MEDIA_BACKUP_INFO, true)) {
        CreateBackupInfoTable(store, VERSION_ADD_MEDIA_BACKUP_INFO);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_MEDIA_BACKUP_INFO, true);
    }

    if (oldVersion < VERSION_ADD_HIGHLIGHT_VIEWED_NOTIFICATION &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_HIGHLIGHT_VIEWED_NOTIFICATION, true)) {
        AddHighlightViewedNotification(store, VERSION_ADD_HIGHLIGHT_VIEWED_NOTIFICATION);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_HIGHLIGHT_VIEWED_NOTIFICATION, true);
    }

    if (oldVersion < VERSION_ADD_SOUTH_DEVICE_TYPE &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_SOUTH_DEVICE_TYPE, true)) {
        AddSouthDeviceType(store, VERSION_ADD_SOUTH_DEVICE_TYPE);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_SOUTH_DEVICE_TYPE, true);
    }

    if (oldVersion < VERSION_ADD_TAB_ANALYSIS_PROGRESS &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_TAB_ANALYSIS_PROGRESS, true)) {
        AddAnalysisProgress(store, VERSION_ADD_TAB_ANALYSIS_PROGRESS);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_TAB_ANALYSIS_PROGRESS, true);
    }

    if (oldVersion < VERSION_ADD_COMPOSITE_DISPLAY_STATUS_COLUMNS &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_ADD_COMPOSITE_DISPLAY_STATUS_COLUMNS, true)) {
        AddCompositeDisplayStatusColumn(store, VERSION_ADD_COMPOSITE_DISPLAY_STATUS_COLUMNS);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_ADD_COMPOSITE_DISPLAY_STATUS_COLUMNS, true);
    }

    UpgradeExtensionPart10(store, oldVersion);
}

static void UpgradeExtensionPart8(RdbStore &store, int32_t oldVersion)
{
    if (oldVersion < VERSION_SHOOTING_MODE_ALBUM_SECOND_INTERATION) {
        PrepareShootingModeAlbum(store);
    }

    if (oldVersion < VERSION_FIX_DB_UPGRADE_FROM_API18) {
        MEDIA_INFO_LOG("Start VERSION_MDIRTY_TRIGGER_UPLOAD_DETAIL_TIME & VERSION_UPDATE_MDIRTY_TRIGGER_FOR_TDIRTY");
        FixMdirtyTriggerToUploadDetailTime(store);
        MEDIA_INFO_LOG("End VERSION_MDIRTY_TRIGGER_UPLOAD_DETAIL_TIME & VERSION_UPDATE_MDIRTY_TRIGGER_FOR_TDIRTY");
    }

    if (oldVersion < VERSION_ADD_BEST_FACE_BOUNDING) {
        AddBestFaceBoundingColumnForGroupAlbum(store);
    }

    if (oldVersion < VERSION_ADD_GROUP_VERSION) {
        AddGroupVersion(store);
    }

    if (oldVersion < VERSION_ADD_EXIF_ROTATE_COLUMN_AND_SET_VALUE) {
        AddExifRotateColumn(store);
    }

    if (oldVersion < VERSION_FIX_DB_UPGRADE_TO_API20 &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_FIX_DB_UPGRADE_TO_API20, true)) {
        unordered_map<string, bool> photoColumnExists = {
            { PhotoColumn::PHOTO_DETAIL_TIME, false },          // VERSION_ADD_DETAIL_TIME
            { PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, false },    // VERSION_ADD_THUMBNAIL_VISIBLE
            { PhotoColumn::SUPPORTED_WATERMARK_TYPE, false },   // VERSION_ADD_SUPPORTED_WATERMARK_TYPE
            { PhotoColumn::PHOTO_CHECK_FLAG, false },           // VERSION_ADD_CHECK_FLAG
            { PhotoColumn::STAGE_VIDEO_TASK_STATUS, false },    // VERSION_ADD_STAGE_VIDEO_TASK_STATUS
            { PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME, false },  // VERSION_ADD_VISIT_COUNT
        };
        CheckIfPhotoColumnExists(store, photoColumnExists);
        UpgradeFromAllVersionFirstPart(store, photoColumnExists);
        UpgradeFromAllVersionSecondPart(store, photoColumnExists);
        UpgradeFromAllVersionThirdPart(store, photoColumnExists);
        UpgradeFromAllVersionFourthPart(store, photoColumnExists);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_FIX_DB_UPGRADE_TO_API20, true);
    }

    if (oldVersion < VERSION_UPDATE_PHOTO_ALBUM_DATEMODIFIED_TIGGER &&
        !RdbUpgradeUtils::HasUpgraded(VERSION_UPDATE_PHOTO_ALBUM_DATEMODIFIED_TIGGER, true)) {
        UpdatePhotoAlbumTigger(store, VERSION_UPDATE_PHOTO_ALBUM_DATEMODIFIED_TIGGER);
        RdbUpgradeUtils::SetUpgradeStatus(VERSION_UPDATE_PHOTO_ALBUM_DATEMODIFIED_TIGGER, true);
    }

    UpgradeExtensionPart9(store, oldVersion);
}

static void UpgradeExtensionPart7(RdbStore &store, int32_t oldVersion)
{
    if (oldVersion < VERSION_ADD_IS_RECTIFICATION_COVER) {
        if (AddIsRectificationCover(store) == NativeRdb::E_OK) {
            UpdatePhotosMdirtyTrigger(store);
        }
    }

    if (oldVersion < VERSION_ADD_PHOTO_ALBUM_REFRESH_COLUMNS) {
        AddPhotoAlbumRefreshColumns(store);
    }

    TableEventHandler().OnUpgrade(
        MediaLibraryUnistoreManager::GetInstance().GetRdbStore(), oldVersion, MEDIA_RDB_VERSION);

    if (oldVersion < VERSION_ADD_COVER_URI_SOURCE) {
        AddCoverColumns(store);
    }

    if (oldVersion < VERSION_ADD_EDITDATA_SIZE_COLUMN) {
        AddEditDataSizeColumn(store);
    }

    if (oldVersion < VERSION_ADD_AESTHETICS_SCORE_FIELDS) {
        AddAestheticsScoreFileds(store);
    }

    if (oldVersion < VERSION_ADD_HIGHLIGHT_LOCATION) {
        AddHighlightLocation(store);
    }

    if (oldVersion < VERSION_ADD_IS_PRIMARY_FACE) {
        AddIsPrimaryFace(store);
    }

    if (oldVersion < VERSION_ADD_PRIORITY_COLUMN) {
        AddTotalPriority(store);
    }

    if (oldVersion < VERSION_ADD_ALBUMS_ORDER_KEYS_COLUMNS) {
        AddAlbumsOrderKeysColumn(store);
    }

    if (oldVersion < VERSION_DROP_PHOTO_INSERT_SOURCE_PHOTO_TRIGGER) {
        DropInsertSourcePhotoCreateSourceAlbumTrigger(store);
        DropInsertPhotoUpdateAlbumBundleNameTrigger(store);
        DropInsertSourcePhotoUpdateAlbumIdTrigger(store);
    }

    UpgradeExtensionPart8(store, oldVersion);
}

static void UpgradeExtensionPart6(RdbStore &store, int32_t oldVersion)
{
    if (oldVersion < VERSION_FIX_DB_UPGRADE_FROM_API15) {
        unordered_map<string, bool> photoColumnExists = {
            { PhotoColumn::PHOTO_IS_RECENT_SHOW, false },
            { PhotoColumn::PHOTO_IS_AUTO, false },
            { PhotoColumn::PHOTO_MEDIA_SUFFIX, false },
            { PhotoColumn::PHOTO_METADATA_FLAGS, false },
        };
        CheckIfPhotoColumnExists(store, photoColumnExists);
        UpgradeFromAPI15(store, photoColumnExists);
        UpgradeAPI18(store, photoColumnExists);
    }

    if (oldVersion < VERSION_CREATE_TAB_ASSET_ALBUM_OPERATION_FOR_SYNC) {
        AddAssetAlbumOperationTableForSync(store);
    }

    if (oldVersion < VERSION_CREATE_TAB_FACARD_PHOTOS_RETRY) {
        TabFaCardPhotosTableEventHandler().OnCreate(store);
    }

    if (oldVersion < VERSION_UPDATE_SEARCH_STATUS_TRIGGER_FOR_IS_FAVORITE) {
        UpdateSearchStatusTriggerForIsFavorite(store);
    }

    if (oldVersion < VERSION_UPGRADE_ANALYSIS_UPDATE_SEARCH_TRIGGER) {
        UpgradeAnalysisUpdateSearchTrigger(store);
    }

    if (oldVersion < VERSION_ADD_DC_ANALYSIS) {
        AddDcAnalysisColumn(store);
    }

    if (oldVersion < VERSION_ADD_VISIT_COUNT) {
        AddVisitCountColumn(store);
    }

    if (oldVersion < VERSION_CREATE_TAB_CUSTOM_RECORDS) {
        CreateTabCustomRecords(store);
    }

    if (oldVersion < VERSION_ADD_DC_ANALYSIS_INDEX_UPDATE) {
        AddDcAnalysisIndexUpdateColumn(store);
    }

    UpgradeExtensionPart7(store, oldVersion);
}

static void UpgradeExtensionPart5(RdbStore &store, int32_t oldVersion)
{
    if (oldVersion < VERSION_ADD_STAGE_VIDEO_TASK_STATUS) {
        AddStageVideoTaskStatus(store);
    }

    if (oldVersion < VERSION_HIGHLIGHT_SUBTITLE) {
        AddHighlightUseSubtitle(store);
    }

    if (oldVersion < VERSION_ADD_IS_AUTO) {
        AddIsAutoColumns(store);
    }

    if (oldVersion < VERSION_ADD_MEDIA_SUFFIX_COLUMN) {
        AddMediaSuffixColumn(store);
    }

    if (oldVersion < VERSION_UPDATE_SOURCE_PHOTO_ALBUM_TRIGGER_AGAIN) {
        UpdateSourcePhotoAlbumTrigger(store);
    }

    if (oldVersion < VERSION_ADD_MEDIA_IS_RECENT_SHOW_COLUMN) {
        AddIsRecentShow(store);
    }

    if (oldVersion < VERSION_CREATE_TAB_FACARD_PHOTOS) {
        TabFaCardPhotosTableEventHandler().OnCreate(store);
    }

    if (oldVersion < VERSION_FIX_SOURCE_ALBUM_CREATE_TRIGGERS_TO_USE_LPATH) {
        FixSourceAlbumCreateTriggersToUseLPath(store);
    }
    if (oldVersion < VERSION_ADD_ALBUM_PLUGIN_BUNDLE_NAME) {
        AddAlbumPluginBundleName(store);
    }

    if (oldVersion < VERSION_ADD_FOREGROUND_ANALYSIS) {
        AddFrontAnalysisColumn(store);
    }

    if (oldVersion < VERSION_HIGHLIGHT_MOVING_PHOTO) {
        AddMovingPhotoRelatedData(store);
    }

    if (oldVersion < VERSION_CREATE_TAB_ASSET_ALBUM_OPERATION) {
        AddAssetAlbumOperationTable(store);
    }

    if (oldVersion < VERSION_MDIRTY_TRIGGER_UPLOAD_DETAIL_TIME) {
        FixMdirtyTriggerToUploadDetailTime(store);
    }

    UpgradeExtensionPart6(store, oldVersion);
}

static void UpgradeExtensionPart4(RdbStore &store, int32_t oldVersion)
{
    if (oldVersion < VERSION_CREATE_TAB_OLD_PHOTOS) {
        TabOldPhotosTableEventHandler().OnCreate(store);
    }

    if (oldVersion < VERSION_ADD_HIGHLIGHT_TRIGGER) {
        AddHighlightTriggerColumn(store);
        AddHighlightInsertAndUpdateTrigger(store);
        AddHighlightIndex(store);
    }

    if (oldVersion < VERSION_ALTER_THUMBNAIL_VISIBLE) {
        AlterThumbnailVisible(store);
    }

    if (oldVersion < VERSION_ADD_HIGHLIGHT_VIDEO_COUNT_CAN_PACK) {
        AddHighlightVideoCountCanPack(store);
    }

    if (oldVersion < VERSION_ADD_GEO_DEFAULT_VALUE) {
        AddGeoDefaultValue(store);
    }

    if (oldVersion < VERSION_HDR_AND_CLOUD_ENHANCEMENT_FIX) {
        AddDynamicRangeColumnsFix(store);
        AddCloudEnhanceColumnsFix(store);
    }

    if (oldVersion < VERSION_THUMBNAIL_READY_FIX) {
        AddThumbnailReadyColumnsFix(store);
    }

    if (oldVersion < VERSION_UPDATE_SOURCE_PHOTO_ALBUM_TRIGGER) {
        UpdateSourcePhotoAlbumTrigger(store);
    }

    if (oldVersion < VERSION_UPDATE_SEARCH_STATUS_TRIGGER_FOR_OWNER_ALBUM_ID) {
        UpdateSearchStatusTriggerForOwnerAlbumId(store);
    }

    if (oldVersion < VERSION_ADD_CHECK_FLAG) {
        AddCheckFlag(store);
    }

    if (oldVersion < VERSION_ADD_HIGHLIGHT_ANALYSIS_PROGRESS) {
        AddHighlightAnalysisProgress(store);
    }

    if (oldVersion < VERSION_FIX_SOURCE_PHOTO_ALBUM_DATE_MODIFIED) {
        UpdateSourcePhotoAlbumTrigger(store);
    }

    if (oldVersion < VERSION_ADD_REFRESH_ALBUM_STATUS_COLUMN) {
        AddRefreshAlbumStatusColumn(store);
    }

    if (oldVersion < VERSION_FIX_SOURCE_ALBUM_UPDATE_TRIGGER_TO_USE_LPATH) {
        FixSourceAlbumUpdateTriggerToUseLPath(store);
    }

    UpgradeExtensionPart5(store, oldVersion);
}

static void UpgradeExtensionPart3(RdbStore &store, int32_t oldVersion)
{
    if (oldVersion < VERSION_CLOUD_ENAHCNEMENT) {
        AddCloudEnhancementColumns(store);
    }

    if (oldVersion < VERSION_UPDATE_MDIRTY_TRIGGER_FOR_UPLOADING_MOVING_PHOTO) {
        UpdatePhotosMdirtyTrigger(store);
    }

    if (oldVersion < VERSION_ADD_INDEX_FOR_FILEID) {
        AddIndexForFileId(store);
    }

    if (oldVersion < VERSION_ADD_OCR_CARD_COLUMNS) {
        AddOCRCardColumns(store);
    }

    if (oldVersion < VERSION_UPDATE_AOI) {
        UpdateAOI(store);
    }

    if (oldVersion < VERSION_UPDATE_VIDEO_FACE_TABLE) {
        UpdateVideoFaceTable(store);
    }

    if (oldVersion < VERSION_ADD_SUPPORTED_WATERMARK_TYPE) {
        AddSupportedWatermarkType(store);
    }

    if (oldVersion < VERSION_FIX_PHOTO_SCHPT_MEDIA_TYPE_INDEX) {
        FixPhotoSchptMediaTypeIndex(store);
    }

    if (oldVersion < VERSION_ADD_ANALYSIS_ALBUM_TOTAL_TABLE) {
        AddAnalysisAlbumTotalTable(store);
    }

    if (oldVersion < VERSION_ADD_THUMBNAIL_VISIBLE) {
        AddThumbnailVisible(store);
    }

    if (oldVersion < VERSION_ADD_METARECOVERY) {
        AddMetaRecovery(store);
    }
    if (oldVersion < VERSION_UPDATE_SEARCH_INDEX_TRIGGER_FOR_CLEAN_FLAG) {
        UpdateSearchIndexTriggerForCleanFlag(store);
    }
    if (oldVersion < VERSION_ADD_COVER_PLAY_SERVICE_VERSION) {
        AddCoverPlayVersionColumns(store);
    }
    if (oldVersion < VERSION_ADD_HIGHLIGHT_MAP_TABLES) {
        AddHighlightMapTable(store);
    }

    if (oldVersion < VERSION_COMPAT_LIVE_PHOTO) {
        CompatLivePhoto(store, oldVersion);
    }

    UpgradeExtensionPart4(store, oldVersion);
}

static void UpgradeExtensionPart2(RdbStore &store, int32_t oldVersion)
{
    if (oldVersion < VERSION_UPDATE_PHOTO_INDEX_FOR_ALBUM_COUNT_COVER) {
        UpdateIndexForAlbumQuery(store);
    }

    if (oldVersion < VERSION_UPDATE_VIDEO_LABEL_TABLE_FOR_SUB_LABEL_TYPE) {
        UpdateVideoLabelTableForSubLabelType(store);
    }

    // VERSION_UPGRADE_THUMBNAIL move to HandleUpgradeRdbAsync()

    if (oldVersion < VISION_UPDATE_DATA_ADDED_INDEX) {
        UpdateDataAddedIndexWithFileId(store);
    }

    if (oldVersion < VISION_UPDATE_SEARCH_INDEX_TRIGGER) {
        UpdateSearchIndexTrigger(store);
    }

    if (oldVersion < VISION_UPDATE_MULTI_CROP_INFO) {
        UpdateMultiCropInfo(store);
    }

    if (oldVersion < VISION_ADD_ORIGINAL_SUBTYPE) {
        AddOriginalSubtype(store);
    }

    // VERSION_UPDATE_BURST_DIRTY = 106 move to UpgradeRdbStoreAsync(), avoid to cost for long time.

    if (oldVersion < VERSION_UDAPTE_DATA_UNIQUE) {
        UpdateDataUniqueIndex(store);
    }

    if (oldVersion < VERSION_ADD_DETAIL_TIME) {
        AddDetailTimeToPhotos(store);
    }

    if (oldVersion < VERSION_ADD_VIDEO_FACE_TABLE) {
        AddVideoFaceTable(store);
    }

    if (oldVersion < VERSION_ADD_OWNER_ALBUM_ID) {
        AddOwnerAlbumIdAndRefractorTrigger(store);
        AlbumPluginTableEventHandler albumPluginTableEventHandler;
        albumPluginTableEventHandler.OnUpgrade(store, oldVersion, oldVersion);
        AddMergeInfoColumnForAlbum(store);
        MEDIA_INFO_LOG("ALBUM_FUSE: set album fuse upgrade status");
        MediaLibraryAlbumFusionUtils::SetAlbumFuseUpgradeStatus(0);
    }

    UpgradeExtensionPart3(store, oldVersion);
    // !! Do not add upgrade code here !!
}

static void UpgradeExtensionPart1(RdbStore &store, int32_t oldVersion)
{
    if (oldVersion < VERSION_ADD_OWNER_APPID_TO_FILES_TABLE) {
        AddOwnerAppIdToFiles(store);
    }

    if (oldVersion < VERSION_ADD_IS_TEMP_TO_TRIGGER) {
        AddIsTempToTrigger(store);
    }

    if (oldVersion < VERSION_UPDATE_ANALYSIS_TABLES) {
        UpdateAnalysisTables(store);
    }

    if (oldVersion < VERSION_UPDATE_PHOTO_THUMBNAIL_READY) {
        UpdateThumbnailReadyColumn(store);
    }

    if (oldVersion < VERSION_ADD_FRONT_CAMERA_TYPE) {
        AddFrontCameraType(store);
    }

    if (oldVersion < PHOTOS_CREATE_DISPLAYNAME_INDEX) {
        AddDisplayNameIndex(store);
    }

    if (oldVersion < VERSION_PORTRAIT_COVER_SELECTION_ADD_COLUMNS) {
        AddPortraitCoverSelectionColumn(store);
    }

    if (oldVersion < VERSION_ADD_BURST_COVER_LEVEL_AND_BURST_KEY) {
        AddBurstCoverLevelAndBurstKey(store);
    }

    if (oldVersion < VERSION_ADD_COVER_POSITION) {
        AddCoverPosition(store);
    }

    if (oldVersion < VERSION_ADD_SCHPT_READY_INEDX) {
        AddSchptReadyIndex(store);
    }

    if (oldVersion < VERSION_UPDATE_PORTRAIT_COVER_SELECTION_COLUMNS) {
        UpdatePortraitCoverSelectionColumns(store);
    }

    if (oldVersion < VERSION_ADD_APP_URI_PERMISSION_INFO) {
        AddAppUriPermissionInfo(store);
    }

    if (oldVersion < VERSION_UPDATE_SOURCE_ALBUM_AND_ALBUM_BUNDLENAME_TRIGGERS) {
        UpdateSourceAlbumAndAlbumBundlenameTriggers(store);
    }

    // VERSION_CREATE_BURSTKEY_INDEX = 98 move to UpgradeRdbStoreAsync(), avoid to cost for long time.

    UpgradeExtensionPart2(store, oldVersion);
    // !! Do not add upgrade code here !!
}

static void CreatePhotosExtTable(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        PhotoExtUpgrade::CREATE_PHOTO_EXT_TABLE
    };
    MEDIA_INFO_LOG("Start create photo ext table in update");
    ExecSqls(executeSqlStrs, store);
}

static void UpgradeExtension(RdbStore &store, int32_t oldVersion)
{
    if (oldVersion < VERSION_ADD_STOYR_TABLE) {
        AddStoryTables(store);
    }

    if (oldVersion < VERSION_UPDATE_HIGHLIGHT_TABLE) {
        UpdateHighlightTables(store);
    }

    if (oldVersion < VERSION_UPDATE_SEARCH_INDEX) {
        UpdatePhotosSearchUpdateTrigger(store);
    }

    if (oldVersion < VERSION_UPDATE_HIGHLIGHT_COVER_TABLE) {
        UpdateHighlightCoverTables(store);
    }

    if (oldVersion < VERSION_CREATE_PHOTOS_EXT_TABLE) {
        CreatePhotosExtTable(store);
    }

    if (oldVersion < VERSION_UPDATE_VIDEO_LABEL_TABEL) {
        UpdateVideoLabelTable(store);
    }

    if (oldVersion < VERSION_ADD_FACE_OCCLUSION_AND_POSE_TYPE_COLUMN) {
        AddFaceOcclusionAndPoseTypeColumn(store);
    }

    if (oldVersion < VERSION_UPDATE_PHOTO_ALBUM_BUNDLENAME) {
        UpdateInsertPhotoUpdateAlbumTrigger(store);
    }

    if (oldVersion < VERSION_UPDATE_PHOTO_ALBUM_TIGGER) {
        UpdatePhotoAlbumTigger(store, VERSION_UPDATE_PHOTO_ALBUM_DATEMODIFIED_TIGGER);
    }

    if (oldVersion < VERSION_ADD_THUMB_LCD_SIZE_COLUMN) {
        AddLcdAndThumbSizeColumns(store);
    }

    if (oldVersion < VERSION_UPDATE_HIGHLIGHT_TABLE_PRIMARY_KEY) {
        UpdateHighlightTablePrimaryKey(store);
    }

    if (oldVersion < VERSION_ADD_MOVING_PHOTO_EFFECT_MODE) {
        AddMovingPhotoEffectMode(store);
    }

    if (oldVersion < VERSION_UPDATE_VISION_TRIGGER_FOR_VIDEO_LABEL) {
        UpdateVisionTriggerForVideoLabel(store);
    }

    if (oldVersion < VERSION_ADD_IS_TEMP) {
        AddIsTemp(store);
    }

    UpgradeExtensionPart1(store, oldVersion);
    // !! Do not add upgrade code here !!
}

static void AddVisionColumnSearchTag(RdbStore &store, int32_t oldVersion)
{
    if (oldVersion < VERSION_ADD_SEARCH_TAG) {
        vector<string> upgradeSqls = {
            BaseColumn::AlterTableAddTextColumn(VISION_LABEL_TABLE, SEARCH_TAG_TYPE),
            BaseColumn::AlterTableAddBlobColumn(VISION_LABEL_TABLE, SEARCH_TAG_TYPE),
        };
        ExecSqls(upgradeSqls, store);
    }
}

static void UpgradeOriginalVersion(RdbStore &store, int32_t oldVersion)
{
    if (oldVersion < VERSION_ADD_CLOUD) {
        VersionAddCloud(store);
    }

    if (oldVersion < VERSION_ADD_META_MODIFED) {
        AddMetaModifiedColumn(store);
    }

    if (oldVersion < VERSION_MODIFY_SYNC_STATUS) {
        ModifySyncStatus(store);
    }

    if (oldVersion < VERSION_ADD_API10_TABLE) {
        API10TableCreate(store);
    }

    if (oldVersion < VERSION_MODIFY_DELETE_TRIGGER) {
        ModifyDeleteTrigger(store);
    }

    if (oldVersion < VERSION_ADD_CLOUD_VERSION) {
        AddCloudVersion(store);
    }

    if (oldVersion < VERSION_UPDATE_CLOUD_PATH) {
        UpdateCloudPath(store);
    }

    if (oldVersion < VERSION_UPDATE_API10_TABLE) {
        UpdateAPI10Table(store);
    }

    if (oldVersion < VERSION_ADD_TABLE_TYPE) {
        AddTableType(store);
    }

    if (oldVersion < VERSION_ADD_PHOTO_CLEAN_FLAG_AND_THUMB_STATUS) {
        AddCleanFlagAndThumbStatus(store);
    }

    if (oldVersion < VERSION_ADD_CLOUD_ID_INDEX) {
        AddCloudIndex(store);
    }
}

int32_t MediaLibraryDataCallBack::OnUpgrade(RdbStore &store, int32_t oldVersion, int32_t newVersion)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryDataCallBack::OnUpgrade");
    if (MediaLibraryRdbStore::GetOldVersion() == -1) {
        MediaLibraryRdbStore::SetOldVersion(oldVersion);
    }
    MEDIA_INFO_LOG("OnUpgrade old:%{public}d, new:%{public}d", oldVersion, newVersion);
    g_upgradeErr = false;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    UpgradeOriginalVersion(store, oldVersion);
    UpgradeOtherTable(store, oldVersion);
    UpgradeGalleryFeatureTable(store, oldVersion);
    UpgradeVisionTable(store, oldVersion);
    UpgradeExtendedVisionTable(store, oldVersion);
    UpgradeAlbumTable(store, oldVersion);
    UpgradeHistory(store, oldVersion);
    UpgradeExtension(store, oldVersion);
    UpgradeUriPermissionTable(store, oldVersion);
    UpgradeHighlightAlbumChange(store, oldVersion);
    AddVisionColumnSearchTag(store, oldVersion);

    if (!g_upgradeErr) {
        VariantMap map = {{KEY_PRE_VERSION, oldVersion}, {KEY_AFTER_VERSION, newVersion}};
        PostEventUtils::GetInstance().PostStatProcess(StatType::DB_UPGRADE_STAT, map);
    }
    RdbUpgradeUtils::ReportUpgradeDfxMessages(startTime, oldVersion, MEDIA_RDB_VERSION, true);
    return NativeRdb::E_OK;
}

void MediaLibraryRdbStore::SetOldVersion(int32_t oldVersion)
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(RDB_CONFIG, errCode);
    CHECK_AND_RETURN_LOG(prefs, "get preferences error: %{public}d", errCode);
    prefs->PutInt(RDB_OLD_VERSION, oldVersion);
    prefs->FlushSync();
}

int32_t MediaLibraryRdbStore::GetOldVersion()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(RDB_CONFIG, errCode);
    CHECK_AND_RETURN_RET_LOG(prefs, oldVersion_, "get preferences error: %{public}d", errCode);
    return prefs->GetInt(RDB_OLD_VERSION, oldVersion_);
}

bool MediaLibraryRdbStore::HasColumnInTable(RdbStore &store, const string &columnName, const string &tableName)
{
    string querySql = "SELECT " + MEDIA_COLUMN_COUNT_1 + " FROM pragma_table_info('" + tableName + "') WHERE name = '" +
        columnName + "'";
    auto resultSet = store.QuerySql(querySql);
    bool cond = (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK);
    CHECK_AND_RETURN_RET_LOG(!cond, false, "Get column count failed");

    int32_t count = GetInt32Val(MEDIA_COLUMN_COUNT_1, resultSet);
    MEDIA_DEBUG_LOG("%{private}s in %{private}s: %{public}d", columnName.c_str(), tableName.c_str(), count);
    return count > 0;
}

void MediaLibraryRdbStore::AddColumnIfNotExists(
    RdbStore &store, const string &columnName, const string &columnType, const string &tableName)
{
    if (!HasColumnInTable(store, columnName, tableName)) {
        string sql = "ALTER TABLE " + tableName + " ADD COLUMN " + columnName + " " + columnType;
        ExecSqlWithRetry([&]() { return store.ExecuteSql(sql); });
    }
}

int MediaLibraryRdbStore::Update(int &changedRows, const std::string &table, const ValuesBucket &row,
    const std::string &whereClause, const std::vector<std::string> &args)
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return ExecSqlWithRetry(
        [&]() { return MediaLibraryRdbStore::GetRaw()->Update(changedRows, table, row, whereClause, args); });
}

std::string MediaLibraryRdbStore::ObtainDistributedTableName(const std::string &device, const std::string &table,
    int &errCode)
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), "",
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return MediaLibraryRdbStore::GetRaw()->ObtainDistributedTableName(device, table, errCode);
}

int MediaLibraryRdbStore::Backup(const std::string &databasePath, const std::vector<uint8_t> &encryptKey)
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return ExecSqlWithRetry([&]() { return MediaLibraryRdbStore::GetRaw()->Backup(databasePath, encryptKey); });
}

int MediaLibraryRdbStore::Backup(const std::string &databasePath, bool integrityCheck,
    const std::vector<uint8_t> &encryptKey)
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return ExecSqlWithRetry([&]() {
        return MediaLibraryRdbStore::GetRaw()->Backup(databasePath, encryptKey, integrityCheck);
    });
}

int MediaLibraryRdbStore::Sync(const DistributedRdb::SyncOption &option, const AbsRdbPredicates &predicate,
    const DistributedRdb::AsyncBrief &async)
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return ExecSqlWithRetry([&]() { return MediaLibraryRdbStore::GetRaw()->Sync(option, predicate, async); });
}

std::shared_ptr<NativeRdb::ResultSet> MediaLibraryRdbStore::QueryByStep(const std::string &sql,
    const std::vector<ValueObject> &args)
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), nullptr,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return MediaLibraryRdbStore::GetRaw()->QueryByStep(sql, args);
}

std::shared_ptr<NativeRdb::ResultSet> MediaLibraryRdbStore::QueryByStep(const AbsRdbPredicates &predicates,
    const std::vector<std::string> &columns)
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), nullptr,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return MediaLibraryRdbStore::GetRaw()->QueryByStep(predicates, columns);
}

int MediaLibraryRdbStore::Update(int &changedRows, const ValuesBucket &row, const AbsRdbPredicates &predicates)
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return ExecSqlWithRetry([&]() { return MediaLibraryRdbStore::GetRaw()->Update(changedRows, row, predicates); });
}

int MediaLibraryRdbStore::Insert(int64_t &outRowId, const std::string &table, ValuesBucket &row)
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    if (table == PhotoColumn::PHOTOS_TABLE) {
        AddDefaultPhotoValues(row);
    }
    return ExecSqlWithRetry([&]() { return MediaLibraryRdbStore::GetRaw()->Insert(outRowId, table, row); });
}

int MediaLibraryRdbStore::Delete(int &deletedRows, const std::string &table, const std::string &whereClause,
    const std::vector<std::string> &args)
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return ExecSqlWithRetry(
        [&]() { return MediaLibraryRdbStore::GetRaw()->Delete(deletedRows, table, whereClause, args); });
}

int MediaLibraryRdbStore::Delete(int &deletedRows, const AbsRdbPredicates &predicates)
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return ExecSqlWithRetry([&]() { return MediaLibraryRdbStore::GetRaw()->Delete(deletedRows, predicates); });
}

pair<int32_t, NativeRdb::Results> MediaLibraryRdbStore::BatchInsert(const string &table,
    vector<ValuesBucket> &values, const string &returningField)
{
    DfxTimer dfxTimer(DfxType::RDB_BATCHINSERT, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRdbStore::BatchInsert");
    if (!MediaLibraryRdbStore::CheckRdbStore()) {
        MEDIA_ERR_LOG("Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
        return {E_HAS_DB_ERROR, -1};
    }
    if (table == PhotoColumn::PHOTOS_TABLE) {
        for (auto& value : values) {
            AddDefaultPhotoValues(value);
        }
    }

    ValuesBuckets refRows;
    for (auto &value : values) {
        refRows.Put(value);
    }

    pair<int32_t, NativeRdb::Results> retWithResults = {E_HAS_DB_ERROR, -1};
    int32_t ret = ExecSqlWithRetry([&]() {
        retWithResults = MediaLibraryRdbStore::GetRaw()->BatchInsert(table, values, { returningField });
        return retWithResults.first;
    });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdbStore_->BatchInsert failed, ret = %{public}d", ret);
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
        return {ret, -1};
    }

    MEDIA_DEBUG_LOG("rdbStore_->BatchInsert end, ret = %{public}d", ret);
    return retWithResults;
}

pair<int32_t, NativeRdb::Results> MediaLibraryRdbStore::Execute(const std::string &sql,
    const std::vector<NativeRdb::ValueObject> &args, const std::string &returningField)
{
    pair<int32_t, NativeRdb::Results> retWithResults = {E_HAS_DB_ERROR, -1};
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), retWithResults,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    DfxTimer dfxTimer(RDB_EXECUTE_SQL, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("RdbStore->ExecuteSql");

    string execSql = sql;
    execSql.append(" returning ").append(returningField);
    MEDIA_INFO_LOG("AccurateRefresh, sql:%{public}s", execSql.c_str());
    int32_t ret = ExecSqlWithRetry([&]() {
        retWithResults = MediaLibraryRdbStore::GetRaw()->ExecuteExt(execSql, args);
        return retWithResults.first;
    });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdbStore_->ExecuteSql failed, ret = %{public}d", ret);
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
        return {E_HAS_DB_ERROR, -1};
    }
    return retWithResults;
}

pair<int32_t, NativeRdb::Results> MediaLibraryRdbStore::Update(const ValuesBucket &row,
    const AbsRdbPredicates &predicates, const string &returningField)
{
    pair<int32_t, NativeRdb::Results> retWithResults = {E_HAS_DB_ERROR, -1};
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), retWithResults,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");

    ExecSqlWithRetry([&]() {
        retWithResults = MediaLibraryRdbStore::GetRaw()->Update(row, predicates, { returningField });
        return retWithResults.first;
    });

    return retWithResults;
}

pair<int32_t, NativeRdb::Results> MediaLibraryRdbStore::Delete(const AbsRdbPredicates &predicates,
    const string &returningField)
{
    pair<int32_t, NativeRdb::Results> retWithResults = {E_HAS_DB_ERROR, -1};
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), retWithResults,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");

    ExecSqlWithRetry([&]() {
        retWithResults = MediaLibraryRdbStore::GetRaw()->Delete(predicates, { returningField });
        return retWithResults.first;
    });
    return retWithResults;
}

std::shared_ptr<NativeRdb::ResultSet> MediaLibraryRdbStore::Query(const NativeRdb::AbsRdbPredicates &predicates,
    const std::vector<std::string> &columns)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRdbStore::Query");
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), nullptr,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return MediaLibraryRdbStore::GetRaw()->Query(predicates, columns);
}

std::shared_ptr<AbsSharedResultSet> MediaLibraryRdbStore::QuerySql(const std::string &sql,
    const std::vector<ValueObject> &args)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRdbStore::QuerySql");
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), nullptr,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return MediaLibraryRdbStore::GetRaw()->QuerySql(sql, args);
}

int MediaLibraryRdbStore::InterruptBackup()
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return ExecSqlWithRetry([&]() { return MediaLibraryRdbStore::GetRaw()->InterruptBackup(); });
}

bool MediaLibraryRdbStore::IsSlaveDiffFromMaster() const
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), false,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return MediaLibraryRdbStore::GetRaw()->IsSlaveDiffFromMaster();
}

int MediaLibraryRdbStore::Restore(const std::string &backupPath, const std::vector<uint8_t> &newKey)
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return ExecSqlWithRetry([&]() { return MediaLibraryRdbStore::GetRaw()->Restore(backupPath, newKey); });
}

int32_t MediaLibraryRdbStore::DataCallBackOnCreate()
{
    MediaLibraryDataCallBack callback;
    int32_t ret = callback.OnCreate(*GetRaw());
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK,
        "MediaLibraryDataCallBack OnCreate error, ret: %{public}d", ret);
    return ret;
}

void MediaLibraryRdbStore::WalCheckPoint()
{
    std::unique_lock<std::mutex> lock(walCheckPointMutex_, std::defer_lock);
    if (!lock.try_lock()) {
        MEDIA_WARN_LOG("wal_checkpoint in progress, skip this operation");
        return;
    }

    struct stat fileStat;
    const std::string walFile = MEDIA_DB_DIR + "/rdb/media_library.db-wal";
    if (stat(walFile.c_str(), &fileStat) < 0) {
        CHECK_AND_PRINT_LOG(errno == ENOENT, "wal_checkpoint stat failed, errno: %{public}d", errno);
        return;
    }
    ssize_t size = fileStat.st_size;
    CHECK_AND_RETURN_LOG(size >= 0, "Invalid size for wal_checkpoint, size: %{public}zd", size);
    CHECK_AND_RETURN(size > RDB_CHECK_WAL_SIZE);

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "wal_checkpoint rdbStore is nullptr!");

    auto errCode = rdbStore->ExecuteSql("PRAGMA wal_checkpoint(TRUNCATE)");
    CHECK_AND_PRINT_LOG(errCode == NativeRdb::E_OK, "wal_checkpoint ExecuteSql failed, errCode: %{public}d", errCode);
}

int MediaLibraryRdbStore::ExecuteForChangedRowCount(int64_t &outValue, const std::string &sql,
    const std::vector<NativeRdb::ValueObject> &args)
{
    CHECK_AND_RETURN_RET_LOG(CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return ExecSqlWithRetry([&]() { return GetRaw()->ExecuteForChangedRowCount(outValue, sql, args); });
}
} // namespace OHOS::Media
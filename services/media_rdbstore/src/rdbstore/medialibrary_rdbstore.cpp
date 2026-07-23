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

#include "album_scan_info_column.h"
#include "album_plugin_table_event_handler.h"
#include "cloud_sync_helper.h"
#include "dfx_timer.h"
#include "dfx_const.h"
#include "dfx_reporter.h"
#include "media_app_uri_permission_column.h"
#include "persist_permission_column.h"
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
#include "media_library_upgrade_manager.h"
#include "media_config_info_column.h"
#include "download_resources_column.h"
#include "tab_cloned_old_photos_table_event_handler.h"
#include "media_audio_column.h"
#include "media_string_utils.h"
#include "media_operation_log_column.h"
#include "media_compatible_info_column.h"
#include "vision_portrait_nickname_column.h"
#include "media_library_upgrade_macros.h"
#include "medialibrary_audio_operations.h"
#include "photo_day_month_year_operation.h"
#include "medialibrary_rdb_utils.h"
#include "media_fileinterwork_column.h"
#include "medialibrary_album_compatibility_fusion_sql.h"
#include "cover_record_columns.h"
#include "medialibrary_rdb_helper.h"
#include "rdb_table_strategy_manager.h"

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

const std::string ATTACHMENT_FILE_EDITDATA = "editdata";
const std::string ATTACHMENT_FILE_EDITDATA_CAMERA = "editdata_camera";
const std::string ATTACHMENT_FILE_SOURCE = "source";
const std::string ATTACHMENT_FILE_SOURCE_BACK = "source_back";

static bool IsAttachmentWhitelistFile(const std::string &fileName)
{
    MEDIA_DEBUG_LOG("Checking file: %{public}s", MediaFileUtils::DesensitizeName(fileName).c_str());
    if (fileName == ATTACHMENT_FILE_EDITDATA || fileName == ATTACHMENT_FILE_EDITDATA_CAMERA) {
        return true;
    }

    // Match source/source_back by base name and allow any extension (e.g. source.jpg/source.mp4).
    size_t dotPos = fileName.find('.');
    std::string baseName = (dotPos == std::string::npos) ? fileName : fileName.substr(0, dotPos);
    return baseName == ATTACHMENT_FILE_SOURCE || baseName == ATTACHMENT_FILE_SOURCE_BACK;
}

void MediaLibraryRdbStore::StatEditAndAttachmentSize(const std::string &rootPath,
    uint64_t &editDataSize, uint64_t &attachmentSize)
{
    size_t editDataSizeCalc = 0;
    size_t attachmentSizeCalc = 0;
    MediaFileUtils::StatDirSize(rootPath, editDataSizeCalc, attachmentSizeCalc, IsAttachmentWhitelistFile);
    editDataSize = editDataSizeCalc;
    attachmentSize = attachmentSizeCalc;
}

const std::string SLAVE = "slave";

constexpr ssize_t RDB_WAL_LIMIT_SIZE = 1024 * 1024 * 1024; /* default wal file maximum size : 1GB */

shared_ptr<NativeRdb::RdbStore> MediaLibraryRdbStore::rdbStore_;

std::mutex MediaLibraryRdbStore::reconstructLock_;

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
    string name = CONST_MEDIA_DATA_ABILITY_DB_NAME;
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
        err = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql(sql); });
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
        int32_t err = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql(sqls[i]); });
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

static int32_t AddPetFaceIndex(RdbStore& store)
{
    const vector<string> executeSqlStrs = {
         CREATE_PET_INDEX,
         CREATE_PET_TAG_ID_INDEX,
     };
    MEDIA_INFO_LOG("add pet face index start");
    int32_t ret = ExecSqlsWithDfx(executeSqlStrs, store, VERSION_ADD_PET_TABLES);
    MEDIA_INFO_LOG("add pet face index end");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_ADD_PET_TABLES, "Vision", AddPetFaceIndex);

static int32_t CreateBurstIndex(RdbStore &store)
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
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end create idx_burstkey");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_CREATE_BURSTKEY_INDEX, "Photos", CreateBurstIndex);

static int32_t UpdateBurstDirty(RdbStore &store)
{
    const vector<string> sqls = {
        "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_DIRTY + " = " +
        to_string(static_cast<int32_t>(DirtyTypes::TYPE_NEW)) + " WHERE " + PhotoColumn::PHOTO_SUBTYPE + " = " +
        to_string(static_cast<int32_t>(PhotoSubType::BURST)) + " AND " + PhotoColumn::PHOTO_DIRTY + " = -1 ",
    };
    MEDIA_INFO_LOG("start UpdateBurstDirty");
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end UpdateBurstDirty");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_UPDATE_BURST_DIRTY, "Photos", UpdateBurstDirty);

static int32_t UpdateReadyOnThumbnailUpgrade(RdbStore &store)
{
    const vector<string> sqls = {
        PhotoUpgrade::UPDATE_READY_ON_THUMBNAIL_UPGRADE,
    };
    MEDIA_INFO_LOG("start update ready for thumbnail upgrade");
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("finish update ready for thumbnail upgrade");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_UPGRADE_THUMBNAIL, "Photos", UpdateReadyOnThumbnailUpgrade);

static int32_t UpdateDateTakenToMillionSecond(RdbStore &store)
{
    MEDIA_INFO_LOG("UpdateDateTakenToMillionSecond start");
    const vector<string> updateSql = {
        "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " +
            MediaColumn::MEDIA_DATE_TAKEN + " = " + MediaColumn::MEDIA_DATE_TAKEN +  "*1000 WHERE " +
            MediaColumn::MEDIA_DATE_TAKEN + " < 1e10",
    };
    int32_t ret = ExecSqls(updateSql, store);
    MEDIA_INFO_LOG("UpdateDateTakenToMillionSecond end");
    return ret;
}

static int32_t UpdateDateTakenIndex(RdbStore &store)
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
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("update index for datetaken change end");
    return ret;
}

static int32_t AddDetailTime(RdbStore &store)
{
    MEDIA_INFO_LOG("Start VERSION_ADD_DETAIL_TIME");
    int32_t ret = UpdateDateTakenToMillionSecond(store);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("UpdateDateTakenToMillionSecond failed");
    }
    ret = UpdateDateTakenIndex(store);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("UpdateDateTakenIndex failed");
    }
    ThumbnailService::GetInstance()->AstcChangeKeyFromDateAddedToDateTaken();
    MEDIA_INFO_LOG("End VERSION_ADD_DETAIL_TIME");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_ADD_DETAIL_TIME, "Photos", AddDetailTime);

static int32_t ClearAudios(RdbStore &store)
{
    const vector<string> sqls = {
        "DELETE From Audios",
    };
    MEDIA_INFO_LOG("clear audios start");
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("clear audios end");
    return ret;
}

static int32_t MoveAudios(RdbStore &store)
{
    MEDIA_INFO_LOG("Start VERSION_MOVE_AUDIOS");
    MediaLibraryAudioOperations::MoveToMusic();
    int32_t ret = ClearAudios(store);
    MEDIA_INFO_LOG("End VERSION_MOVE_AUDIOS");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_MOVE_AUDIOS, "OtherTable", MoveAudios);

static int32_t UpdateIndexForCover(RdbStore &store)
{
    const vector<string> sqls = {
        PhotoUpgrade::DROP_SCHPT_MEDIA_TYPE_INDEX,
        PhotoUpgrade::CREATE_SCHPT_MEDIA_TYPE_INDEX,
    };
    MEDIA_INFO_LOG("update index for photo album cover start");
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("update index for photo album cover end");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_UPDATE_INDEX_FOR_COVER, "Photos", UpdateIndexForCover);

static int32_t UpdateThumbnailVisibleAndIdx(RdbStore &store)
{
    MEDIA_INFO_LOG("Start VERSION_ADD_THUMBNAIL_VISIBLE");
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
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("End VERSION_ADD_THUMBNAIL_VISIBLE");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_ADD_THUMBNAIL_VISIBLE, "Photos", UpdateThumbnailVisibleAndIdx);

static int32_t UpdateDateTakenAndDetailTime(RdbStore &store)
{
    MEDIA_INFO_LOG("UpdateDateTakenAndDetailTime start");
    string updateDateTakenSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + MediaColumn::MEDIA_DATE_TAKEN +
            " = " + PhotoColumn::MEDIA_DATE_MODIFIED + "," + PhotoColumn::PHOTO_DETAIL_TIME +
            " = strftime('%Y:%m:%d %H:%M:%S', " + MediaColumn::MEDIA_DATE_MODIFIED +
            "/1000, 'unixepoch', 'localtime')" + " WHERE " + MediaColumn::MEDIA_DATE_TAKEN + " = 0";
    string updateDetailTimeSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_DETAIL_TIME +
            " = strftime('%Y:%m:%d %H:%M:%S', " + MediaColumn::MEDIA_DATE_TAKEN + "/1000, 'unixepoch', 'localtime')" +
            " WHERE " + PhotoColumn::PHOTO_DETAIL_TIME + " IS NULL";
    const vector<string> updateSql = {
        updateDateTakenSql,
        updateDetailTimeSql,
    };
    int32_t ret = ExecSqls(updateSql, store);
    MEDIA_INFO_LOG("UpdateDateTakenAndDetailTime end");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_UPDATE_DATETAKEN_AND_DETAILTIME, "Photos", UpdateDateTakenAndDetailTime);

static int32_t AddReadyCountIndex(RdbStore &store)
{
    MEDIA_INFO_LOG("start add ready count index");
    const vector<string> sqls = {
        PhotoUpgrade::CREATE_SCHPT_MEDIA_TYPE_COUNT_READY_INDEX,
        PhotoUpgrade::CREATE_SCHPT_YEAR_COUNT_READY_INDEX,
        PhotoUpgrade::CREATE_SCHPT_MONTH_COUNT_READY_INDEX,
    };
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end add ready count index");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_ADD_READY_COUNT_INDEX, "Photos", AddReadyCountIndex);

static int32_t RevertFixDateAddedIndex(RdbStore &store)
{
    MEDIA_INFO_LOG("start revert fix date added index");
    const vector<string> sqls = {
        PhotoUpgrade::DROP_INDEX_SCTHP_ADDTIME,
        PhotoUpgrade::INDEX_SCTHP_ADDTIME,
        PhotoUpgrade::DROP_INDEX_SCHPT_ADDTIME_ALBUM,
    };
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end revert fix date added index");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_REVERT_FIX_DATE_ADDED_INDEX, "Photos", RevertFixDateAddedIndex);

static int32_t UpdateLcdStatusNotUploaded(RdbStore &store)
{
    MEDIA_INFO_LOG("start FixPictureLcdSize");
    const vector<string> sqls = {
        PhotoUpgrade::UPDATE_LCD_STATUS_NOT_UPLOADED,
    };
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("finish CheckLcdSizeAndUpdateStatus");
    ThumbnailService::GetInstance()->CheckLcdSizeAndUpdateStatus();
    MEDIA_INFO_LOG("end FixPictureLcdSize");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_FIX_PICTURE_LCD_SIZE, "Photos", UpdateLcdStatusNotUploaded);

static int32_t AddAlbumIndex(RdbStore &store)
{
    MEDIA_INFO_LOG("start add album index");
    const vector<string> sqls = {
        PhotoUpgrade::INDEX_SCHPT_ALBUM_GENERAL,
        PhotoUpgrade::INDEX_SCHPT_ALBUM,
    };
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end add album index");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_ADD_ALBUM_INDEX, "Album", AddAlbumIndex);

static int32_t AddPhotoDateAddedIndex(RdbStore &store)
{
    MEDIA_INFO_LOG("start AddPhotoDateAddedIndex");
    const vector<string> sqls = {
        PhotoUpgrade::INDEX_SCTHP_PHOTO_DATEADDED,
    };
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end AddPhotoDateAddedIndex");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_ADD_PHOTO_DATEADD_INDEX, "Photos", AddPhotoDateAddedIndex);

static int32_t RefreshPermissionAppid(RdbStore &store)
{
    MEDIA_INFO_LOG("start RefreshPermissionAppid");
    MediaLibraryRdbUtils::TransformAppId2TokenId(store);
    MEDIA_INFO_LOG("end RefreshPermissionAppid");
    return NativeRdb::E_OK;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_REFRESH_PERMISSION_APPID, "Photos", RefreshPermissionAppid);

static int32_t AddCloudEnhancementAlbumIndex(RdbStore &store)
{
    const vector<string> sqls = {
        PhotoUpgrade::CREATE_SCHPT_CLOUD_ENHANCEMENT_ALBUM_INDEX
    };
    MEDIA_INFO_LOG("start create idx_schpt_cloud_enhancement_album_index");
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end create idx_schpt_cloud_enhancement_album_index");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_ADD_CLOUD_ENHANCEMENT_ALBUM_INDEX, "Photos", AddCloudEnhancementAlbumIndex);

static int32_t UpdatePhotosDateAndIdx(RdbStore &store)
{
    MEDIA_INFO_LOG("start UpdatePhotosDateAndIdx");
    int32_t ret = PhotoDayMonthYearOperation::UpdatePhotosDateAndIdx(store);
    MEDIA_INFO_LOG("end UpdatePhotosDateAndIdx");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_UPDATE_PHOTOS_DATE_AND_IDX, "Photos", UpdatePhotosDateAndIdx);

static int32_t UpdatePhotosDateIdx(RdbStore &store)
{
    MEDIA_INFO_LOG("start UpdatePhotosDateIdx");
    int32_t ret = PhotoDayMonthYearOperation::UpdatePhotosDateIdx(store);
    MEDIA_INFO_LOG("end UpdatePhotosDateIdx");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_UPDATE_PHOTOS_DATE_IDX, "Photos", UpdatePhotosDateIdx);

static int32_t UpdateMediaTypeAndThumbnailReadyIdx(RdbStore &store)
{
    const vector<string> sqls = {
        PhotoUpgrade::DROP_SCHPT_MEDIA_TYPE_COUNT_READY_INDEX,
        PhotoUpgrade::CREATE_SCHPT_MEDIA_TYPE_COUNT_READY_INDEX,
        PhotoUpgrade::DROP_INDEX_SCHPT_READY,
        PhotoUpgrade::INDEX_SCHPT_READY,
    };
    MEDIA_INFO_LOG("start update idx_schpt_media_type_ready and idx_schpt_thumbnail_ready");
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end update idx_schpt_media_type_ready and idx_schpt_thumbnail_ready");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_UPDATE_MEDIA_TYPE_AND_THUMBNAIL_READY_IDX,
    "Photos", UpdateMediaTypeAndThumbnailReadyIdx);

static int32_t UpdatePhotoQualityCloned(RdbStore &store)
{
    MEDIA_INFO_LOG("start UpdatePhotoQualityCloned");
    const vector<string> sqls = {
        PhotoUpgrade::UPDATE_PHOTO_QUALITY_OF_NULL_PHOTO_ID,
    };
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end UpdatePhotoQualityCloned");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_FIX_PHOTO_QUALITY_CLONED, "Photos", UpdatePhotoQualityCloned);

static int32_t AnalyzePhotos(RdbStore &store)
{
    MEDIA_INFO_LOG("start AnalyzePhotos");
    MediaLibraryRdbUtils::AnalyzePhotosData();
    MEDIA_INFO_LOG("end AnalyzePhotos");
    return NativeRdb::E_OK;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_ANALYZE_PHOTOS, "Photos", AnalyzePhotos);

static int32_t AddIndexForPhotoSortInAlbum(RdbStore &store)
{
    const vector<string> sqls = {
        PhotoUpgrade::CREATE_PHOTO_SORT_IN_ALBUM_SIZE_INDEX,
        PhotoUpgrade::CREATE_PHOTO_SORT_MEDIA_TYPE_SIZE_INDEX,
        PhotoUpgrade::CREATE_PHOTO_SORT_IN_ALBUM_DISPLAY_NAME_INDEX,
        PhotoUpgrade::CREATE_PHOTO_SORT_MEDIA_TYPE_DISPLAY_NAME_INDEX,
    };
    MEDIA_INFO_LOG("Start add index for photo sort in album");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_INDEX_FOR_PHOTO_SORT_IN_ALBUM);
    MEDIA_INFO_LOG("End add index for photo sort in album");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_ADD_INDEX_FOR_PHOTO_SORT_IN_ALBUM, "Photos", AddIndexForPhotoSortInAlbum);

static int32_t AddIndexForCloudAndPitaya(RdbStore &store)
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
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_INDEX_FOR_CLOUD_AND_PITAYA);
    MEDIA_INFO_LOG("End add index for Cloud Enhancement and Pitaya");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_ADD_INDEX_FOR_CLOUD_AND_PITAYA, "Photos", AddIndexForCloudAndPitaya);

static int32_t UpdateIndexHiddenTime(RdbStore &store)
{
    const vector<string> sqls = {
        BaseColumn::DropIndex() + PhotoColumn::PHOTO_SCHPT_HIDDEN_TIME_INDEX,
        PhotoUpgrade::CREATE_SCHPT_HIDDEN_TIME_INDEX,
    };
    MEDIA_INFO_LOG("start update idx_schpt_hidden_time");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_UPGRADE_IDX_SCHPT_HIDDEN_TIME);
    MEDIA_INFO_LOG("end update idx_schpt_hidden_time");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_UPGRADE_IDX_SCHPT_HIDDEN_TIME, "Photos", UpdateIndexHiddenTime);

static int32_t UpdateIndexDateAdded(RdbStore &store)
{
    const vector<string> sqls = {
        BaseColumn::DropIndex() + PhotoColumn::PHOTO_SCHPT_PHOTO_DATEADDED_INDEX,
        PhotoUpgrade::INDEX_SCTHP_PHOTO_DATEADDED,
    };
    MEDIA_INFO_LOG("start update idx_schpt_date_added");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_UPGRADE_IDX_SCHPT_DATE_ADDED);
    MEDIA_INFO_LOG("end update idx_schpt_date_added");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_UPGRADE_IDX_SCHPT_DATE_ADDED, "Photos", UpdateIndexDateAdded);

static int32_t AddIndexForPhotoSort(RdbStore &store)
{
    MEDIA_INFO_LOG("start AddPhotoSortIndex");
    const vector<string> sqls = {
        PhotoUpgrade::CREATE_PHOTO_SORT_MEDIA_TYPE_DATE_ADDED_INDEX,
        PhotoUpgrade::CREATE_PHOTO_SORT_MEDIA_TYPE_DATE_TAKEN_INDEX,
        PhotoUpgrade::CREATE_PHOTO_SORT_IN_ALBUM_DATE_ADDED_INDEX,
        PhotoUpgrade::CREATE_PHOTO_SORT_IN_ALBUM_DATE_TAKEN_INDEX,
    };
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("End AddPhotoSortIndex");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_ADD_INDEX_FOR_PHOTO_SORT, "Photos", AddIndexForPhotoSort);

static int32_t AddGroupTagIndex(RdbStore& store)
{
    MEDIA_INFO_LOG("start to add group tag index");

    int32_t ret = store.ExecuteSql(CREATE_ANALYSIS_ALBUM_GROUP_TAG_INDEX);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "AddGroupTagIndex failed: execute sql failed");
    MEDIA_INFO_LOG("end add group tag index");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_ADD_GROUP_TAG_INDEX, "Photos", AddGroupTagIndex);

static int32_t AddImageFaceTagIdIndex(RdbStore& store)
{
    int32_t ret = store.ExecuteSql(CREATE_IMAGE_FACE_TAG_ID_INDEX);
    MEDIA_INFO_LOG("Adding TAG_ID index for VISION_IMAGE_FACE_TABLE");
    return ret;
}

static int32_t FixDbUpgradeFromApi18(RdbStore &store)
{
    MEDIA_INFO_LOG("start FixDbUpgradeFromApi18");
    int32_t ret = AddGroupTagIndex(store);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("AddGroupTagIndex failed");
    }
    ret = AddImageFaceTagIdIndex(store);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("AddImageFaceTagIdIndex failed");
    }
    ret = AddIndexForPhotoSort(store);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("AddIndexForPhotoSort failed");
    }
    MEDIA_INFO_LOG("end FixDbUpgradeFromApi18");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_FIX_DB_UPGRADE_FROM_API18, "Photos", FixDbUpgradeFromApi18);

static int32_t AddPhotoQueryThumbnailWhiteBlocksIndex(RdbStore &store)
{
    MEDIA_INFO_LOG("start AddPhotoWhiteBlocksIndex");
    const vector<string> sqls = {
        PhotoUpgrade::INDEX_QUERY_THUMBNAIL_WHITE_BLOCKS,
    };
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end AddPhotoWhiteBlocksIndex");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_ADD_PHOTO_QUERY_THUMBNAIL_WHITE_BLOCKS_INDEX,
    "Photos", AddPhotoQueryThumbnailWhiteBlocksIndex);

static int32_t TransferOwnerappidToTokenid(RdbStore &store)
{
    MEDIA_INFO_LOG("start TransferOwnerappidToTokenid");
    MediaLibraryRdbUtils::TransformOwnerAppIdToTokenId(store);
    MEDIA_INFO_LOG("end TransferOwnerappidToTokenid");
    return NativeRdb::E_OK;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_TRANSFER_OWNERAPPID_TO_TOKENID, "OtherTable", TransferOwnerappidToTokenid);

static int32_t DropPhotoStatusForSearchIndex(RdbStore &store)
{
    MEDIA_INFO_LOG("Drop photo status for search index start");
    const vector<string> sqls = {
        "DROP INDEX IF EXISTS idx_photo_status_for_search_index",
    };
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_DROP_PHOTO_STATUS_FOR_SEARCH_INDEX);
    MEDIA_INFO_LOG("Drop photo status for search index end");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_DROP_PHOTO_STATUS_FOR_SEARCH_INDEX, "Photos", DropPhotoStatusForSearchIndex);

namespace {
    struct ColumnInfo {
        const std::string& name;
        const std::string& type;
    };

    const ColumnInfo PHOTO_TABLE_COLUMNS[] = {
        {MediaColumn::MEDIA_ID, "INTEGER PRIMARY KEY AUTOINCREMENT"},
        {MediaColumn::MEDIA_FILE_PATH, "TEXT"},
        {MediaColumn::MEDIA_SIZE, "BIGINT"},
        {MediaColumn::MEDIA_TITLE, "TEXT"},
        {MediaColumn::MEDIA_NAME, "TEXT"},
        {MediaColumn::MEDIA_TYPE, "INT"},
        {MediaColumn::MEDIA_MIME_TYPE, "TEXT"},
        {MediaColumn::MEDIA_OWNER_PACKAGE, "TEXT"},
        {MediaColumn::MEDIA_OWNER_APPID, "TEXT"},
        {MediaColumn::MEDIA_PACKAGE_NAME, "TEXT"},
        {MediaColumn::MEDIA_DEVICE_NAME, "TEXT"},
        {MediaColumn::MEDIA_DATE_ADDED, "BIGINT"},
        {MediaColumn::MEDIA_DATE_MODIFIED, "BIGINT"},
        {MediaColumn::MEDIA_DATE_TAKEN, "BIGINT DEFAULT 0"},
        {MediaColumn::MEDIA_DURATION, "INT"},
        {MediaColumn::MEDIA_TIME_PENDING, "BIGINT DEFAULT 0"},
        {MediaColumn::MEDIA_IS_FAV, "INT DEFAULT 0"},
        {MediaColumn::MEDIA_DATE_TRASHED, "BIGINT DEFAULT 0"},
        {MediaColumn::MEDIA_DATE_DELETED, "BIGINT DEFAULT 0"},
        {MediaColumn::MEDIA_HIDDEN, "INT DEFAULT 0"},
        {MediaColumn::MEDIA_PARENT_ID, "INT DEFAULT 0"},
        {MediaColumn::MEDIA_RELATIVE_PATH, "TEXT"},
        {MediaColumn::MEDIA_VIRTUAL_PATH, "TEXT UNIQUE"},

        {PhotoColumn::PHOTO_DIRTY, "INT DEFAULT 1"},
        {PhotoColumn::PHOTO_CLOUD_ID, "TEXT"},
        {PhotoColumn::PHOTO_META_DATE_MODIFIED, "BIGINT DEFAULT 0"},
        {PhotoColumn::PHOTO_SYNC_STATUS, "INT DEFAULT 0"},
        {PhotoColumn::PHOTO_CLOUD_VERSION, "BIGINT DEFAULT 0"},
        {PhotoColumn::PHOTO_ORIENTATION, "INT DEFAULT 0"},
        {PhotoColumn::PHOTO_EXIF_ROTATE, "INT NOT NULL DEFAULT 0"},
        {PhotoColumn::PHOTO_LATITUDE, "DOUBLE DEFAULT 0"},
        {PhotoColumn::PHOTO_LONGITUDE, "DOUBLE DEFAULT 0"},
        {PhotoColumn::PHOTO_HEIGHT, "INT"},
        {PhotoColumn::PHOTO_WIDTH, "INT"},
        {PhotoColumn::PHOTO_EDIT_TIME, "BIGINT DEFAULT 0"},
        {PhotoColumn::PHOTO_LCD_VISIT_TIME, "BIGINT DEFAULT 0"},
        {PhotoColumn::PHOTO_POSITION, "INT DEFAULT 1"},
        {PhotoColumn::PHOTO_SUBTYPE, "INT DEFAULT 0"},
        {PhotoColumn::PHOTO_ORIGINAL_SUBTYPE, "INT"},
        {PhotoColumn::CAMERA_SHOT_KEY, "TEXT"},
        {PhotoColumn::PHOTO_USER_COMMENT, "TEXT"},
        {PhotoColumn::PHOTO_ALL_EXIF, "TEXT"},
        {PhotoColumn::PHOTO_DATE_YEAR, "TEXT"},
        {PhotoColumn::PHOTO_DATE_MONTH, "TEXT"},
        {PhotoColumn::PHOTO_DATE_DAY, "TEXT"},
        {PhotoColumn::PHOTO_SHOOTING_MODE, "TEXT"},
        {PhotoColumn::PHOTO_SHOOTING_MODE_TAG, "TEXT"},
        {PhotoColumn::PHOTO_LAST_VISIT_TIME, "BIGINT DEFAULT 0"},
        {PhotoColumn::PHOTO_HIDDEN_TIME, "BIGINT DEFAULT 0"},
        {PhotoColumn::PHOTO_THUMB_STATUS, "INT DEFAULT 0"},
        {PhotoColumn::PHOTO_CLEAN_FLAG, "INT DEFAULT 0"},
        {PhotoColumn::PHOTO_ID, "TEXT"},
        {PhotoColumn::PHOTO_QUALITY, "INT"},
        {PhotoColumn::PHOTO_FIRST_VISIT_TIME, "BIGINT DEFAULT 0"},
        {PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, "INT DEFAULT 0"},
        {PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, "INT DEFAULT 0"},
        {PhotoColumn::MOVING_PHOTO_EFFECT_MODE, "INT DEFAULT 0"},
        {PhotoColumn::PHOTO_COVER_POSITION, "BIGINT DEFAULT 0"},
        {PhotoColumn::PHOTO_IS_RECTIFICATION_COVER, "INT NOT NULL DEFAULT 0"},
        {PhotoColumn::PHOTO_THUMBNAIL_READY, "BIGINT DEFAULT 0"},
        {PhotoColumn::PHOTO_LCD_SIZE, "TEXT"},
        {PhotoColumn::PHOTO_THUMB_SIZE, "TEXT"},
        {PhotoColumn::PHOTO_FRONT_CAMERA, "TEXT"},
        {PhotoColumn::PHOTO_IS_TEMP, "INT DEFAULT 0"},
        {PhotoColumn::PHOTO_BURST_COVER_LEVEL, "INT DEFAULT 1"},
        {PhotoColumn::PHOTO_BURST_KEY, "TEXT"},
        {PhotoColumn::PHOTO_CE_AVAILABLE, "INT DEFAULT 0"},
        {PhotoColumn::PHOTO_CE_STATUS_CODE, "INT"},
        {PhotoColumn::PHOTO_STRONG_ASSOCIATION, "INT DEFAULT 0"},
        {PhotoColumn::PHOTO_ASSOCIATE_FILE_ID, "INT DEFAULT 0"},
        {PhotoColumn::PHOTO_HAS_CLOUD_WATERMARK, "INT DEFAULT 0"},
        {PhotoColumn::PHOTO_DETAIL_TIME, "TEXT"},
        {PhotoColumn::PHOTO_OWNER_ALBUM_ID, "INT DEFAULT 0"},
        {PhotoColumn::PHOTO_ORIGINAL_ASSET_CLOUD_ID, "TEXT"},
        {PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, "INT DEFAULT 0"},
        {PhotoColumn::PHOTO_SOURCE_PATH, "TEXT"},
        {PhotoColumn::SUPPORTED_WATERMARK_TYPE, "INT"},
        {PhotoColumn::PHOTO_METADATA_FLAGS, "INT DEFAULT 0"},
        {PhotoColumn::PHOTO_CHECK_FLAG, "INT DEFAULT 0"},
        {PhotoColumn::STAGE_VIDEO_TASK_STATUS, "INT NOT NULL DEFAULT 0"},
        {PhotoColumn::PHOTO_IS_AUTO, "INT NOT NULL DEFAULT 0"},
        {PhotoColumn::PHOTO_MEDIA_SUFFIX, "TEXT"},
        {PhotoColumn::PHOTO_IS_RECENT_SHOW, "INT NOT NULL DEFAULT 1"},
        {PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME, "BIGINT NOT NULL DEFAULT 0"},
        {PhotoColumn::PHOTO_VISIT_COUNT, "INT NOT NULL DEFAULT 0"},
        {PhotoColumn::PHOTO_LCD_VISIT_COUNT, "INT NOT NULL DEFAULT 0"},
        {PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE, "INT NOT NULL DEFAULT 0"},
        {PhotoColumn::PHOTO_HAS_APPLINK, "INT NOT NULL DEFAULT 0"},
        {PhotoColumn::PHOTO_APPLINK, "TEXT"},
        {PhotoColumn::PHOTO_TRANSCODE_TIME, "BIGINT NOT NULL DEFAULT 0"},
        {PhotoColumn::PHOTO_TRANS_CODE_FILE_SIZE, "BIGINT NOT NULL DEFAULT 0"},
        {PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE, "INT NOT NULL DEFAULT 0"},
        {PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS, "INT NOT NULL DEFAULT 0"},
        {PhotoColumn::PHOTO_FILE_INODE, "TEXT"},
        {PhotoColumn::PHOTO_STORAGE_PATH, "TEXT"},
        {PhotoColumn::PHOTO_FILE_SOURCE_TYPE, "INT NOT NULL DEFAULT 0"},
        {PhotoColumn::PHOTO_HDR_MODE, "INT NOT NULL DEFAULT 0"},
        {PhotoColumn::PHOTO_VIDEO_MODE, "INT NOT NULL DEFAULT -1"},
        {PhotoColumn::PHOTO_EDIT_DATA_EXIST, "INT NOT NULL DEFAULT 0"},
        {PhotoColumn::PHOTO_ASPECT_RATIO, "DOUBLE NOT NULL DEFAULT -2"},
        {PhotoColumn::PHOTO_CHANGE_TIME, "BIGINT NOT NULL DEFAULT 0"},
        {PhotoColumn::PHOTO_IS_CRITICAL, "INT NOT NULL DEFAULT 0"},
        {PhotoColumn::PHOTO_RISK_STATUS, "INT NOT NULL DEFAULT 0"},
        {PhotoColumn::PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE, "INT NOT NULL DEFAULT 0"},
        {PhotoColumn::PHOTO_DATE_ADDED_DAY, "TEXT"},
        {PhotoColumn::PHOTO_DATE_ADDED_MONTH, "TEXT"},
        {PhotoColumn::PHOTO_DATE_ADDED_YEAR, "TEXT"},
        {PhotoColumn::UNIQUE_ID, "TEXT DEFAULT '-1'"},
        {PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_STATUS, "INT NOT NULL DEFAULT 0"},
        {PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_LATEST_PAIR, "TEXT"},
        {PhotoColumn::LOCAL_ASSET_SIZE, "BIGINT NOT NULL DEFAULT 0"},
        {PhotoColumn::ATTACHMENT_SIZE, "BIGINT NOT NULL DEFAULT 0"},
        {PhotoColumn::COMPRESSION_QUALITY, "INT NOT NULL DEFAULT -1"},
    };

    constexpr size_t PHOTO_TABLE_COLUMN_COUNT = sizeof(PHOTO_TABLE_COLUMNS) / sizeof(ColumnInfo);
}

void MediaLibraryRdbStore::CheckAndAddPhotoTableColumns(const shared_ptr<MediaLibraryRdbStore> store)
{
    MEDIA_INFO_LOG("start check and add photo table columns");
    auto &rdbStore = *store->GetRaw().get();

    for (size_t i = 0; i < PHOTO_TABLE_COLUMN_COUNT; ++i) {
        MediaLibraryRdbHelper::AddColumnIfNotExists(rdbStore, PHOTO_TABLE_COLUMNS[i].name, PHOTO_TABLE_COLUMNS[i].type,
            PhotoColumn::PHOTOS_TABLE);
    }

    MEDIA_INFO_LOG("end check and add photo table columns");
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

static int32_t AddVideoFaceTagIdIndex(RdbStore& store)
{
    const vector<string> sqls = {
        CREATE_VIDEO_FACE_TAG_ID_INDEX,
    };
    MEDIA_INFO_LOG("start add video_face_tag_id_index");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_CREATE_VIDEO_FACE_TAG_ID_INDEX);
    MEDIA_INFO_LOG("end add video_face_tag_id_index");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_CREATE_VIDEO_FACE_TAG_ID_INDEX, "Photos", AddVideoFaceTagIdIndex);

// 更新单条编辑数据大小
int32_t MediaLibraryRdbStore::UpdateEditDataSize(std::shared_ptr<MediaLibraryRdbStore> rdbStore,
    const std::string &photoId, const std::string &editDataDir, EditAndAttachmentUpdateType updateType)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_DB_FAIL, "rdbStore is nullptr");

    uint64_t editDataSize = 0;
    uint64_t attachmentSize = 0;
    StatEditAndAttachmentSize(editDataDir, editDataSize, attachmentSize);

    std::string updatePhotoExtSql = "UPDATE " + PhotoExtColumn::PHOTOS_EXT_TABLE + " "
        "SET " + PhotoExtColumn::EDITDATA_SIZE + " = ? "
        "WHERE " + PhotoExtColumn::PHOTO_ID + " = ? ;";
    std::string updatePhotoSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE +
        " SET " + PhotoColumn::ATTACHMENT_SIZE + " = ?"
        " WHERE " + MediaColumn::MEDIA_ID + " = ? ;";

    std::vector<ValueObject> editDataBindArgs = {
        ValueObject(to_string(editDataSize)),
        ValueObject(photoId)
    };
    std::vector<ValueObject> attachmentBindArgs = {
        ValueObject(to_string(attachmentSize)),
        ValueObject(photoId)
    };

    std::shared_ptr<TransactionOperations> trans = std::make_shared<TransactionOperations>(__func__);
    std::function<int(void)> updateFunc = [&]() -> int32_t {
        int32_t ret = E_OK;
        if (updateType != EditAndAttachmentUpdateType::ATTACHMENT_ONLY) {
            ret = trans->ExecuteSql(updatePhotoExtSql, editDataBindArgs);
            CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_DB_FAIL,
                "Update editdata_size failed, photoId:%{public}s", photoId.c_str());
        }

        if (updateType != EditAndAttachmentUpdateType::EDIT_ONLY) {
            ret = trans->ExecuteSql(updatePhotoSql, attachmentBindArgs);
            CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_DB_FAIL,
                "Update attachment_size failed, photoId:%{public}s", photoId.c_str());
        }
        return E_OK;
    };
    int32_t errCode = trans->RetryTrans(updateFunc);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Trans fail!, ret:%{public}d", errCode);
    MEDIA_DEBUG_LOG("end update edit data size and attachment size");
    return errCode;
}

int32_t MediaLibraryRdbStore::UpdateAttachmentSize(std::shared_ptr<MediaLibraryRdbStore> rdbStore,
    const std::string &photoId, uint64_t attachmentSize)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_DB_FAIL, "rdbStore is nullptr");

    std::string updatePhotoSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE +
        " SET " + PhotoColumn::ATTACHMENT_SIZE + " = ?"
        " WHERE " + MediaColumn::MEDIA_ID + " = ? ;";

    std::vector<ValueObject> attachmentBindArgs = {
        ValueObject(to_string(attachmentSize)),
        ValueObject(photoId)
    };

    std::shared_ptr<TransactionOperations> trans = std::make_shared<TransactionOperations>(__func__);
    std::function<int(void)> updateFunc = [&]() -> int32_t {
        int32_t ret = trans->ExecuteSql(updatePhotoSql, attachmentBindArgs);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_DB_FAIL,
            "Update attachment_size failed, photoId:%{public}s", photoId.c_str());
        return E_OK;
    };

    int32_t errCode = trans->RetryTrans(updateFunc);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Trans fail!, ret:%{public}d", errCode);
    return errCode;
}

static int32_t UpdateLocationKnowledgeIdx(RdbStore& store)
{
    MEDIA_INFO_LOG("start update location knowledge index");
    const vector<string> sqls = {
        DROP_KNOWLEDGE_INDEX,
        CREATE_NEW_KNOWLEDGE_INDEX
    };
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end update location knowledge index");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_UPDATE_LOCATION_KNOWLEDGE_INDEX, "Photos", UpdateLocationKnowledgeIdx);

static int32_t AddAlbumSubtypeAndNameIdx(RdbStore& store)
{
    MEDIA_INFO_LOG("start to add album subtype and name index");
    const vector<string> sqls = {
        CREATE_ANALYSIS_ALBUM_SUBTYPE_NAME_INDEX,
        CREATE_ANALYSIS_ALBUM_TAG_ID_INDEX
    };
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end add album subtype and name index");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_ADD_ALBUM_SUBTYPE_AND_NAME_INDEX, "Vision", AddAlbumSubtypeAndNameIdx);

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

static int32_t UpdateLocationDefaultNull(RdbStore& store)
{
    MEDIA_INFO_LOG("start Update LatitudeAndLongitude Default Null");
    const vector<string> sqls = {
        PhotoUpgrade::INDEX_LATITUDE,
        PhotoUpgrade::INDEX_LONGITUDE,
        PhotoUpgrade::UPDATE_LATITUDE_AND_LONGITUDE_DEFAULT_NULL,
    };
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end  Update LatitudeAndLongitude Default Null");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_UPDATE_LATITUDE_AND_LONGITUDE_DEFAULT_NULL,
    "Photos", UpdateLocationDefaultNull);

static int32_t UpdateMdirtyTriggerForTdirty(RdbStore& store)
{
    MEDIA_INFO_LOG("start UpdateMdirtyTriggerForTdirty");
    const string dropMdirtyCreateTrigger = "DROP TRIGGER IF EXISTS photos_mdirty_trigger";
    int32_t ret = ExecSqls({dropMdirtyCreateTrigger, PhotoUpgrade::CREATE_PHOTOS_MDIRTY_TRIGGER}, store);
    MEDIA_INFO_LOG("end UpdateMdirtyTriggerForTdirty");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_UPDATE_MDIRTY_TRIGGER_FOR_TDIRTY, "Photos", UpdateMdirtyTriggerForTdirty);

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

int32_t MediaLibraryRdbStore::Insert(MediaLibraryCommand &cmd, int64_t &rowId)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRdbStore::InsertCmd");

    return InsertInternal(rowId, cmd.GetTableName(), cmd.GetValueBucket());
}

int MediaLibraryRdbStore::Insert(int64_t &outRowId, const std::string &table, ValuesBucket &row)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRdbStore::InsertRowId");

    return InsertInternal(outRowId, table, row);
}

int32_t MediaLibraryRdbStore::InsertInternal(int64_t &outRowId, const std::string &table,
    const NativeRdb::ValuesBucket &row)
{
    DfxTimer dfxTimer(DfxType::RDB_INSERT, INVALID_DFX, RDB_TIME_OUT, false);
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");

    NativeRdb::ValuesBucket tmpValues = row;
    TableStrategyConfig config = {
        .enableDefault = true,
    };
    int32_t strategyRet = RdbTableStrategyManager::GetInstance().ExtendInsertValues(
        table, tmpValues, *GetRaw().get(), config);
    if (strategyRet != E_OK) {
        MEDIA_ERR_LOG("Failed to ExtendInsertValues, ret: %{public}d.", strategyRet);
        return strategyRet;
    }

    int32_t ret = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        return MediaLibraryRdbStore::GetRaw()->Insert(outRowId, table, tmpValues);
    });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdbStore_->Insert failed, ret = %{public}d", ret);
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
        return E_HAS_DB_ERROR;
    }
    MEDIA_DEBUG_LOG("rdbStore_->Insert end, outRowId = %d, ret = %{public}d", (int)outRowId, ret);
    return ret;
}

int32_t MediaLibraryRdbStore::BatchInsert(int64_t &outRowId, const std::string &table,
    std::vector<NativeRdb::ValuesBucket> &values)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRdbStore::BatchInsertRowId");

    return BatchInsertInternal(outRowId, table, values);
}

int32_t MediaLibraryRdbStore::BatchInsert(MediaLibraryCommand &cmd, int64_t& outInsertNum,
    std::vector<ValuesBucket>& values)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRdbStore::BatchInsertCmd");
    
    return BatchInsertInternal(outInsertNum, cmd.GetTableName(), values);
}

int32_t MediaLibraryRdbStore::BatchInsertInternal(int64_t &outRowId, const std::string &table,
    const std::vector<NativeRdb::ValuesBucket> &values)
{
    DfxTimer dfxTimer(DfxType::RDB_BATCHINSERT, INVALID_DFX, RDB_TIME_OUT, false);
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    
    std::vector<NativeRdb::ValuesBucket> tmpValues = values;
    TableStrategyConfig config = {
        .enableDefault = true,
    };
    int32_t strategyRet =
        RdbTableStrategyManager::GetInstance().ExtendBatchInsertValues(table, tmpValues, *GetRaw().get(), config);
    if (strategyRet != E_OK) {
        MEDIA_ERR_LOG("Failed to ExtendBatchInsertValues, ret: %{public}d.", strategyRet);
        return strategyRet;
    }

    int32_t ret = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        return MediaLibraryRdbStore::GetRaw()->BatchInsert(outRowId, table, tmpValues);
    });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdbStore_->BatchInsert failed, ret = %{public}d", ret);
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
        return E_HAS_DB_ERROR;
    }
    MEDIA_DEBUG_LOG("rdbStore_->BatchInsert end, rowId = %d, ret = %{public}d", (int)outRowId, ret);
    return ret;
}

int32_t MediaLibraryRdbStore::DeleteInternal(const AbsRdbPredicates &predicates, int32_t &deletedRows)
{
    DfxTimer dfxTimer(DfxType::RDB_DELETE, INVALID_DFX, RDB_TIME_OUT, false);
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");

    int32_t ret = NativeRdb::E_ERROR;
    string tableName = predicates.GetTableName();
    ValuesBucket valuesBucket;

    TableStrategyConfig config = {
        .enableDefault = true,
    };
    TableStrategyErrno err = RdbTableStrategyManager::GetInstance().ExtendDeleteValues(tableName, valuesBucket, config);
    if (err != TableStrategyErrno::NO_SUCH_STRATEGY) {
        ret = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
            return MediaLibraryRdbStore::GetRaw()->Update(deletedRows, tableName, valuesBucket,
                predicates.GetWhereClause(), predicates.GetWhereArgs());
        });
        MEDIA_INFO_LOG("delete photos permanently, ret: %{public}d", ret);
    } else {
        ret = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
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
    int32_t ret = DeleteInternal(*(cmd.GetAbsRdbPredicates()), deletedRows);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdbStore_->Delete failed, ret = %{public}d", ret);
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
        return E_HAS_DB_ERROR;
    }
    return ret;
}

int32_t MediaLibraryRdbStore::Update(MediaLibraryCommand &cmd, int32_t &changedRows)
{
    DfxTimer dfxTimer(DfxType::RDB_UPDATE_BY_CMD, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("RdbStore->UpdateByCmd");

    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");

    NativeRdb::ValuesBucket tmpValues = cmd.GetValueBucket();
    TableStrategyConfig config = {
        .enableDefault = true,
    };
    RdbTableStrategyManager::GetInstance().ExtendUpdateValues(cmd.GetTableName(), tmpValues, config);

    int32_t ret = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        return MediaLibraryRdbStore::GetRaw()->Update(changedRows, cmd.GetTableName(), tmpValues,
            cmd.GetAbsRdbPredicates()->GetWhereClause(), cmd.GetAbsRdbPredicates()->GetWhereArgs());
    });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdbStore_->Update failed, ret = %{public}d", ret);
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
        return E_HAS_DB_ERROR;
    }
    return ret;
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryRdbStore::Query(MediaLibraryCommand &cmd,
    const vector<string> &columns)
{
    MediaLibraryTracer tracer;
    tracer.Start("RdbStore->QueryByCmd");

    return QueryInternal(*cmd.GetAbsRdbPredicates(), columns, true);
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
    MEDIA_DEBUG_LOG("Predicates Statement is %{private}s", predicates.GetStatement().c_str());
    MEDIA_DEBUG_LOG("PhotosApp Predicates Args are %{private}s", argsInfo.c_str());
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryRdbStore::QueryWithFilter(const AbsRdbPredicates &predicates,
    const vector<string> &columns)
{
    MediaLibraryTracer tracer;
    tracer.Start("RdbStore->QueryByPredicates");

    return QueryInternal(predicates, columns, true);
}

int32_t MediaLibraryRdbStore::ExecuteSql(const string &sql, const std::vector<NativeRdb::ValueObject> &args)
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");

    DfxTimer dfxTimer(RDB_EXECUTE_SQL, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("RdbStore->ExecuteSql");

    int32_t ret = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        return MediaLibraryRdbStore::GetRaw()->ExecuteSql(sql, args);
    });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdbStore_->ExecuteSql failed, ret = %{public}d", ret);
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
        return E_HAS_DB_ERROR;
    }
    return ret;
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryRdbStore::QueryByStepWithoutCount(const AbsRdbPredicates &predicates,
    const vector<string> &columns, bool isAlbumRefresh)
{
    MediaLibraryTracer tracer;
    tracer.Start("QueryByStepWithoutCount");

    return QueryInternal(predicates, columns, false, isAlbumRefresh);
}

std::shared_ptr<NativeRdb::ResultSet> MediaLibraryRdbStore::QueryInternal(const NativeRdb::AbsRdbPredicates &predicates,
    const std::vector<std::string> &columns, bool preCount, bool isAlbumRefresh)
{
    DfxTimer dfxTimer(RDB_QUERY, INVALID_DFX, RDB_TIME_OUT, false);
    if (!MediaLibraryRdbStore::CheckRdbStore()) {
        MEDIA_ERR_LOG("rdbStore_ is nullptr");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::QUERY}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return nullptr;
    }

    /* add filter */
    TableStrategyConfig config = {
        .enableDefault = true,
        .isAlbumRefresh = isAlbumRefresh,
    };
    RdbTableStrategyManager::GetInstance().ExtendQueryFilters(const_cast<AbsRdbPredicates &>(predicates), config);
    PrintPredicatesInfo(predicates, columns);

    auto resultSet = MediaLibraryRdbStore::GetRaw()->QueryByStep(predicates, columns, preCount);
    MediaLibraryRestore::GetInstance().CheckResultSet(resultSet);
    if (resultSet == nullptr) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::QUERY}};
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
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    DfxTimer dfxTimer(RDB_EXECUTE_SQL, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("RdbStore->ExecuteForLastInsertedRowId");

    int64_t lastInsertRowId = 0;
    int32_t err = MediaLibraryRdbHelper::ExecSqlWithRetry(
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
    MediaLibraryTracer tracer;
    tracer.Start("RdbStore->DeleteByPredicates");

    int32_t deletedRows = 0;
    int32_t err = DeleteInternal(predicates, deletedRows);
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
int32_t MediaLibraryRdbStore::UpdateWithDateTime(ValuesBucket &values, const AbsRdbPredicates &predicates)
{
    DfxTimer dfxTimer(DfxType::RDB_UPDATE, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRdbStore::UpdateWithDateTime by predicates");

    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    
    NativeRdb::ValuesBucket tmpValues = values;
    TableStrategyConfig config = {
        .enableDefault = true,
    };
    RdbTableStrategyManager::GetInstance().ExtendUpdateValues(predicates.GetTableName(), tmpValues, config);

    int32_t changedRows = -1;
    int32_t err = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        return MediaLibraryRdbStore::GetRaw()->Update(changedRows, tmpValues, predicates);
    });
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

    DfxTimer dfxTimer(RDB_EXECUTE_SQL, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("RdbStore->QuerySql by selectionArgs");
    auto resultSet = MediaLibraryRdbStore::GetRaw()->QuerySql(sql, selectionArgs);
    MediaLibraryRestore::GetInstance().CheckResultSet(resultSet);
    if (resultSet == nullptr) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::QUERY}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
    }

    return resultSet;
}

inline void BuildInsertSystemAlbumSql(const ValuesBucket &values, const AbsRdbPredicates &predicates,
    string &sql, vector<ValueObject> &bindArgs)
{
    // Build insert sql
    sql.append("INSERT").append(" OR ROLLBACK ").append(" INTO ").append(PhotoAlbumColumns::TABLE).append(" ");
    MediaLibraryRdbHelper::BuildValuesSql(values, bindArgs, sql);
    sql.append(" WHERE NOT EXISTS (");
    MediaLibraryRdbHelper::BuildQuerySql(predicates, { PhotoAlbumColumns::ALBUM_ID }, bindArgs, sql);
    sql.append(");");
}

int32_t PrepareAlbumPlugin(RdbStore &store)
{
    AlbumPluginTableEventHandler albumPluginTableEventHander;
    int32_t ret = albumPluginTableEventHander.OnCreate(store);
    // after initiate album_plugin table, add 2 default album into PhotoAlbum.
    MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        return store.ExecuteSql(CREATE_DEFALUT_ALBUM_FOR_NO_RELATIONSHIP_ASSET);
    });
    MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
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
    int32_t insertResult = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        return store.InsertWithConflictResolution(outRowId, CONST_MEDIATYPE_DIRECTORY_TABLE, valuesBucket,
            ConflictResolution::ON_CONFLICT_REPLACE);
    });
    MEDIA_DEBUG_LOG("insert dir outRowId: %{public}ld insertResult: %{public}d", (long)outRowId, insertResult);
    return insertResult;
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

static int32_t Prepare3DGSModeAlbum(RdbStore &store)
{
    vector<string> existingAlbumNames;
    if (MediaLibraryRdbHelper::QueryExistingShootingModeAlbumNames(store, existingAlbumNames) != E_SUCCESS) {
        MEDIA_ERR_LOG("Query existing shootingMode album names failed");
        return NativeRdb::E_ERROR;
    }
    string albumName = to_string(static_cast<int>(ShootingModeAlbumType::MP4_3DGS_ALBUM));
    if (find(existingAlbumNames.begin(), existingAlbumNames.end(), albumName) != existingAlbumNames.end()) {
        return NativeRdb::E_OK;
    }
    auto ret = MediaLibraryRdbHelper::InsertShootingModeAlbumValues(albumName, store);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Prepare shootingMode album failed");
        RdbUpgradeUtils::AddUpgradeDfxMessages(VERSION_ADD_3DGS_MODE, 0, ret);
        return NativeRdb::E_ERROR;
    }
    return NativeRdb::E_OK;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_3DGS_MODE, "Album", Prepare3DGSModeAlbum);

static int32_t PrepareOtherShootingModeAlbum(RdbStore &store)
{
    vector<string> existingAlbumNames;
    if (MediaLibraryRdbHelper::QueryExistingShootingModeAlbumNames(store, existingAlbumNames) != E_SUCCESS) {
        MEDIA_ERR_LOG("Query existing shootingMode album names failed");
        return NativeRdb::E_ERROR;
    }
    for (int i = static_cast<int>(ShootingModeAlbumType::START);
        i <= static_cast<int>(ShootingModeAlbumType::END); ++i) {
        string albumName = to_string(i);
        if (find(existingAlbumNames.begin(), existingAlbumNames.end(), albumName) != existingAlbumNames.end()) {
            continue;
        }
        auto ret = MediaLibraryRdbHelper::InsertShootingModeAlbumValues(albumName, store);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Prepare shootingMode album failed");
            RdbUpgradeUtils::AddUpgradeDfxMessages(VERSION_ADD_QUICK_CAPTURE_AND_TIME_LAPSE, i, ret);
            return NativeRdb::E_ERROR;
        }
    }
    return NativeRdb::E_OK;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_QUICK_CAPTURE_AND_TIME_LAPSE, "Album", PrepareOtherShootingModeAlbum);

int32_t MediaLibraryDataCallBack::InsertSmartAlbumValues(const SmartAlbumValuesBucket &smartAlbum, RdbStore &store)
{
    ValuesBucket valuesBucket;
    valuesBucket.PutInt(SMARTALBUM_DB_ID, smartAlbum.albumId);
    valuesBucket.PutString(SMARTALBUM_DB_NAME, smartAlbum.albumName);
    valuesBucket.PutInt(SMARTALBUM_DB_ALBUM_TYPE, smartAlbum.albumType);
    int64_t outRowId = -1;
    int32_t insertResult = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        return store.InsertWithConflictResolution(outRowId, CONST_SMARTALBUM_TABLE, valuesBucket,
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
    int32_t insertResult = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
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

    UniqueMemberValuesBucket imageBucket = { CONST_IMAGE_ASSET_TYPE, 0 };
    UniqueMemberValuesBucket videoBucket = { CONST_VIDEO_ASSET_TYPE, 0 };
    UniqueMemberValuesBucket audioBucket = { CONST_AUDIO_ASSET_TYPE, 0 };

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
    CREATE_TAB_ANALYSIS_WATERMARK,
    SQL_CREATE_TAB_ANALYSIS_CAPTION,
    CREATE_TAB_IMAGE_FACE,
    CREATE_TAB_VIDEO_FACE,
    CREATE_TAB_FACE_TAG,
    CREATE_TAB_ANALYSIS_TOTAL_FOR_ONCREATE,
    CREATE_TAB_ANALYSIS_VIDEO_TOTAL,
    CREATE_TAB_ANALYSIS_DEDUP_SELECTION,
    CREATE_TAB_ANALYSIS_PROFILE,
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
    CREATE_ANALYSIS_NICK_NAME_TABLE,
    CREATE_ANALYSIS_NICK_NAME_UNIQUE_INDEX,
    CREATE_ANALYSIS_NICK_NAME_DELETE_TRIGGER,
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
    SQL_CREATE_TAB_ASSET_ALBUM_OPERATION,
    SQL_CREATE_OPERATION_ASSET_INSERT_TRIGGER,
    SQL_CREATE_OPERATION_ASSET_DELETE_TRIGGER,
    SQL_CREATE_OPERATION_ASSET_UPDATE_TRIGGER,
    SQL_CREATE_OPERATION_ALBUM_INSERT_TRIGGER,
    SQL_CREATE_OPERATION_ALBUM_DELETE_TRIGGER,
    SQL_CREATE_OPERATION_ALBUM_UPDATE_TRIGGER,
    CREATE_ANALYSIS_PHOTO_MAP_MAP_ASSET_INDEX,
    ConfigInfoColumn::CREATE_CONFIG_INFO_TABLE,
    CREATE_ALBUM_ORDER_BACK_TABLE,
    CREATE_LAKE_ALBUM_TABLE,
    // interwork of photos-file
    AlbumScanInfoColumn::CREATE_TABLE,
    AlbumScanInfoColumn::CREATE_INDEX_ON_ALBUM_ID_STORAGE_PATH,
    MediaFileInterworkColumn::CREATE_FILE_OPT_TABLE,
    CoverRecordColumns::CREATE_COVER_RECORD_TABLE,
    CoverRecordColumns::CREATE_ALBUM_LPATH_INDEX,

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
    CREATE_ANALYSIS_ALBUM_UPDATE_ALBUM_STATUS_TRIGGER,
    MedialibraryBusinessRecordColumn::CREATE_TABLE,
    MedialibraryBusinessRecordColumn::CREATE_BUSINESS_KEY_INDEX,
    PhotoExtUpgrade::CREATE_PHOTO_EXT_TABLE,
    PhotoUpgrade::CREATE_PHOTO_DISPLAYNAME_INDEX,
    AppUriPermissionColumn::CREATE_APP_URI_PERMISSION_TABLE,
    AppUriPermissionColumn::CREATE_URI_URITYPE_TOKENID_INDEX,
    AppUriPermissionColumn::CREATE_URI_PERMISSION_FILE_TOKEN_INDEX,
    PersistPermissionColumn::CREATE_PERSIST_PERMISSION_TABLE,
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
    TabOperationLogColumn::CREATE_TABLE,
    DownloadResourcesColumn::CREATE_TABLE,
    DownloadResourcesColumn::INDEX_DRTR_ID_STATUS,

    TabCompatibleInfoColumn::CREATE_TABLE,
};

static int32_t ExecuteSql(RdbStore &store)
{
    for (const string& sqlStr : onCreateSqlStrs) {
        auto ret = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql(sqlStr); });
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

    MediaLibraryRdbHelper::PrepareShootingModeAlbum(store);

    RdbUpgradeUtils::SetOldVersion(MEDIA_RDB_VERSION);
    RdbUpgradeUtils::AddMapValueToPreference();
    return NativeRdb::E_OK;
}

int32_t VersionAddCloud(RdbStore &store)
{
    const std::string alterCloudId = std::string("ALTER TABLE ") + CONST_MEDIALIBRARY_TABLE +
        " ADD COLUMN " + CONST_MEDIA_DATA_DB_CLOUD_ID +" TEXT";
    int32_t result = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql(alterCloudId); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Upgrade rdb cloud_id error %{private}d", result);
    }
    const std::string alterDirty = std::string("ALTER TABLE ") + CONST_MEDIALIBRARY_TABLE +
        " ADD COLUMN " + CONST_MEDIA_DATA_DB_DIRTY +" INT DEFAULT 0";
    result = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql(alterDirty); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Upgrade rdb dirty error %{private}d", result);
    }
    const std::string alterSyncStatus = std::string("ALTER TABLE ") + CONST_MEDIALIBRARY_TABLE +
        " ADD COLUMN " + CONST_MEDIA_DATA_DB_SYNC_STATUS +" INT DEFAULT 0";
    result = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql(alterSyncStatus); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Upgrade rdb syncStatus error %{private}d", result);
    }
    const std::string alterPosition = std::string("ALTER TABLE ") + CONST_MEDIALIBRARY_TABLE +
        " ADD COLUMN " + CONST_MEDIA_DATA_DB_POSITION +" INT DEFAULT 1";
    result = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql(alterPosition); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Upgrade rdb position error %{private}d", result);
    }
    return result;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_CLOUD, "OtherTable", VersionAddCloud);

static int32_t AddPortraitInAnalysisAlbum(RdbStore &store)
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
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_PORTRAIT_IN_ALBUM, "Vision", AddPortraitInAnalysisAlbum);

int32_t AddMetaModifiedColumn(RdbStore &store)
{
    const std::string alterMetaModified =
        std::string("ALTER TABLE ") + CONST_MEDIALIBRARY_TABLE + " ADD COLUMN " +
        CONST_MEDIA_DATA_DB_META_DATE_MODIFIED + " BIGINT DEFAULT 0";
    int32_t result = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql(alterMetaModified); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Upgrade rdb meta_date_modified error %{private}d", result);
    }
    const std::string alterSyncStatus = std::string("ALTER TABLE ") + CONST_MEDIALIBRARY_TABLE +
        " ADD COLUMN " + CONST_MEDIA_DATA_DB_SYNC_STATUS + " INT DEFAULT 0";
    result = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql(alterSyncStatus); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Upgrade rdb syncStatus error %{private}d", result);
    }
    return result;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_META_MODIFED, "OtherTable", AddMetaModifiedColumn);

int32_t AddTableType(RdbStore &store)
{
    const std::string alterTableName =
        std::string("ALTER TABLE ") + CONST_BUNDLE_PERMISSION_TABLE + " ADD COLUMN " + CONST_PERMISSION_TABLE_TYPE +
        " INT";
    int32_t result = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql(alterTableName); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Upgrade rdb table_name error %{private}d", result);
    }
    return result;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_TABLE_TYPE, "OtherTable", AddTableType);

int32_t API10TableCreate(RdbStore &store)
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

    int32_t result = 0;
    for (size_t i = 0; i < executeSqlStrs.size(); i++) {
        auto result = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql(executeSqlStrs[i]); });
        if (result != NativeRdb::E_OK) {
            UpdateFail(__FILE__, __LINE__);
            MEDIA_ERR_LOG("upgrade fail idx:%{public}zu", i);
        }
    }
    return result;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_API10_TABLE, "Photos", API10TableCreate);

int32_t ModifySyncStatus(RdbStore &store)
{
    const std::string dropSyncStatus = std::string("ALTER TABLE ") + CONST_MEDIALIBRARY_TABLE + " DROP column syncing";
    auto result = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql(dropSyncStatus); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Upgrade rdb syncing error %{private}d", result);
    }

    const std::string addSyncStatus = std::string("ALTER TABLE ") + CONST_MEDIALIBRARY_TABLE + " ADD COLUMN " +
        CONST_MEDIA_DATA_DB_SYNC_STATUS +" INT DEFAULT 0";
    result = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql(addSyncStatus); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Upgrade rdb syncStatus error %{private}d", result);
    }
    return result;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_MODIFY_SYNC_STATUS, "OtherTable", ModifySyncStatus);

int32_t ModifyDeleteTrigger(RdbStore &store)
{
    /* drop old delete trigger */
    const std::string dropDeleteTrigger = "DROP TRIGGER IF EXISTS photos_delete_trigger";
    if (MediaLibraryRdbHelper::ExecSqlWithRetry(
        [&]() { return store.ExecuteSql(dropDeleteTrigger); }) != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("upgrade fail: drop old delete trigger");
    }

    /* create new delete trigger */
    if (MediaLibraryRdbHelper::ExecSqlWithRetry(
        [&]() { return store.ExecuteSql(PhotoUpgrade::CREATE_PHOTOS_DELETE_TRIGGER); }) !=
        NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("upgrade fail: create new delete trigger");
    }
    return NativeRdb::E_OK;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_MODIFY_DELETE_TRIGGER, "Photos", ModifyDeleteTrigger);

int32_t AddCloudVersion(RdbStore &store)
{
    const std::string addSyncStatus = "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
        PhotoColumn::PHOTO_CLOUD_VERSION +" BIGINT DEFAULT 0";
    auto result = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql(addSyncStatus); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Upgrade rdb cloudVersion error %{private}d", result);
    }
    return result;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_CLOUD_VERSION, "Photos", AddCloudVersion);

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

static int32_t UpdateMdirtyTriggerForSdirty(RdbStore &store)
{
    const string dropMdirtyCreateTrigger = "DROP TRIGGER IF EXISTS photos_mdirty_trigger";
    int32_t ret = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql(dropMdirtyCreateTrigger); });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("drop photos_mdirty_trigger fail, ret = %{public}d", ret);
        UpdateFail(__FILE__, __LINE__);
    }

    ret = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        return store.ExecuteSql(PhotoUpgrade::CREATE_PHOTOS_MDIRTY_TRIGGER);
    });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("add photos_mdirty_trigger fail, ret = %{public}d", ret);
        UpdateFail(__FILE__, __LINE__);
    }
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_MDIRTY_TRIGGER_FOR_SDIRTY, "Photos", UpdateMdirtyTriggerForSdirty);

static int32_t UpdateCloudPath(RdbStore &store)
{
    const vector<string> updateCloudPath = {
        UpdateCloudPathSql(CONST_MEDIALIBRARY_TABLE, CONST_MEDIA_DATA_DB_FILE_PATH),
        UpdateCloudPathSql(CONST_MEDIALIBRARY_TABLE, CONST_MEDIA_DATA_DB_RECYCLE_PATH),
        UpdateCloudPathSql(MEDIALIBRARY_ERROR_TABLE, MEDIA_DATA_ERROR),
        UpdateCloudPathSql(PhotoColumn::PHOTOS_TABLE, MediaColumn::MEDIA_FILE_PATH),
    };
    auto result = ExecSqls(updateCloudPath, store);
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
    }
    return result;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_CLOUD_PATH, "OtherTable", UpdateCloudPath);

int32_t UpdateAPI10Table(RdbStore &store)
{
    MediaLibraryRdbHelper::ExecSqlWithRetry(
        [&]() { return store.ExecuteSql("DROP INDEX IF EXISTS idx_sthp_dateadded"); });
    MediaLibraryRdbHelper::ExecSqlWithRetry(
        [&]() { return store.ExecuteSql("DROP INDEX IF EXISTS photo_album_types"); });
    MediaLibraryRdbHelper::ExecSqlWithRetry(
        [&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS photos_delete_trigger"); });
    MediaLibraryRdbHelper::ExecSqlWithRetry(
        [&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS photos_fdirty_trigger"); });
    MediaLibraryRdbHelper::ExecSqlWithRetry(
        [&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS photos_mdirty_trigger"); });
    MediaLibraryRdbHelper::ExecSqlWithRetry(
        [&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS photo_insert_cloud_sync_trigger"); });
    MediaLibraryRdbHelper::ExecSqlWithRetry(
        [&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS delete_trigger"); });
    MediaLibraryRdbHelper::ExecSqlWithRetry(
        [&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS mdirty_trigger"); });
    MediaLibraryRdbHelper::ExecSqlWithRetry(
        [&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS fdirty_trigger"); });
    MediaLibraryRdbHelper::ExecSqlWithRetry(
        [&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS insert_cloud_sync_trigger"); });
    MediaLibraryRdbHelper::ExecSqlWithRetry(
        [&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS photo_album_clear_map"); });
    MediaLibraryRdbHelper::ExecSqlWithRetry(
        [&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS photo_album_insert_asset"); });
    MediaLibraryRdbHelper::ExecSqlWithRetry(
        [&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS photo_album_delete_asset"); });
    MediaLibraryRdbHelper::ExecSqlWithRetry(
        [&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS delete_photo_clear_map"); });
    MediaLibraryRdbHelper::ExecSqlWithRetry(
        [&]() { return store.ExecuteSql("DROP TRIGGER IF EXISTS update_user_album_count"); });
    MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TABLE IF EXISTS Photos"); });
    MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TABLE IF EXISTS Audios"); });
    MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TABLE IF EXISTS UniqueNumber"); });
    MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TABLE IF EXISTS PhotoAlbum"); });
    MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TABLE IF EXISTS PhotoMap"); });
    MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql("DROP TABLE IF EXISTS FormMap"); });

    API10TableCreate(store);
    if (PrepareSystemAlbums(store) != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
    }

    if (PrepareUniqueMemberTable(store) != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
    }

    // set scan error
    auto scannerManager = MediaScannerManager::GetInstance();
    if (scannerManager != nullptr) {
        scannerManager->ErrorRecord();
    }
    return NativeRdb::E_OK;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_API10_TABLE, "Photos", UpdateAPI10Table);

static int32_t AddLocationTables(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        CREATE_GEO_DICTIONARY_TABLE,
        CREATE_GEO_KNOWLEDGE_TABLE,
    };
    MEDIA_INFO_LOG("start init location db");
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_LOCATION_TABLE, "Vision", AddLocationTables);

static int32_t UpdateLocationTables(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        "DROP TABLE IF EXISTS tab_geo_dictionary",
        "DROP TABLE IF EXISTS tab_geo_knowledge",
        CREATE_GEO_DICTIONARY_TABLE,
        CREATE_GEO_KNOWLEDGE_TABLE,
    };
    MEDIA_INFO_LOG("fix location db");
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_LOCATION_TABLE, "Vision", UpdateLocationTables);

static int32_t AddAnalysisTables(RdbStore &store)
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
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_VISION_TABLE, "Vision", AddAnalysisTables);

static int32_t AddFaceTables(RdbStore &store)
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
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_FACE_TABLE, "Vision", AddFaceTables);

static int32_t AddSaliencyTables(RdbStore &store)
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
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_SALIENCY_TABLE, "Vision", AddSaliencyTables);

static int32_t AddVideoLabelTable(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        CREATE_TAB_ANALYSIS_VIDEO_LABEL,
        DROP_INSERT_VISION_TRIGGER,
        DROP_UPDATE_VISION_TRIGGER,
        CREATE_VISION_INSERT_TRIGGER_FOR_ADD_VIDEO_LABEL,
        CREATE_VISION_UPDATE_TRIGGER_FOR_ADD_VIDEO_LABEL,
    };
    MEDIA_INFO_LOG("start add video label tables");
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_VIDEO_LABEL_TABEL, "Vision", AddVideoLabelTable);

static int32_t UpdateVideoLabelTable(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        "DROP TABLE IF EXISTS tab_analysis_video_label",
        CREATE_TAB_ANALYSIS_VIDEO_LABEL,
    };
    MEDIA_INFO_LOG("start update video label tables");
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_VIDEO_LABEL_TABEL, "Vision", UpdateVideoLabelTable);

static int32_t AddSourceAlbumTrigger(RdbStore &store)
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
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_SOURCE_ALBUM_TRIGGER, "Photos", AddSourceAlbumTrigger);

static int32_t RemoveSourceAlbumToAnalysis(RdbStore &store)
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
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_REOMOVE_SOURCE_ALBUM_TO_ANALYSIS, "Photos", RemoveSourceAlbumToAnalysis);

static int32_t MoveSourceAlbumToPhotoAlbumAndAddColumns(RdbStore &store)
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
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_MOVE_SOURCE_ALBUM_TO_PHOTO_ALBUM_AND_ADD_COLUMNS,
    "Album", MoveSourceAlbumToPhotoAlbumAndAddColumns);

static int32_t ModifySourceAlbumTriggers(RdbStore &store)
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
    int32_t ret = ExecSqls(executeSqlStrs, store);
    MediaLibraryRdbUtils::UpdateSourceAlbumInternal(MediaLibraryUnistoreManager::GetInstance().GetRdbStore());
    MEDIA_INFO_LOG("end modify source album triggers");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_MODIFY_SOURCE_ALBUM_TRIGGERS,
    "Album", ModifySourceAlbumTriggers);

static int32_t AddAnalysisAlbum(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        "ALTER TABLE tab_analysis_ocr ADD COLUMN width INT;",
        "ALTER TABLE tab_analysis_ocr ADD COLUMN height INT;",
        CREATE_ANALYSIS_ALBUM,
        CREATE_ANALYSIS_ALBUM_MAP,
    };
    MEDIA_INFO_LOG("start init vision album");
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_VISION_ALBUM, "Vision", AddAnalysisAlbum);

static int32_t AddAestheticCompositionTables(RdbStore &store)
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
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_AESTHETIC_COMPOSITION_TABLE, "Vision", AddAestheticCompositionTables);

int32_t UpdateSpecForAddScreenshot(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        DROP_INSERT_VISION_TRIGGER,
        CREATE_VISION_INSERT_TRIGGER_FOR_UPDATE_SPEC,
    };
    MEDIA_INFO_LOG("update media analysis service specifications for add screenshot");
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_SPEC_FOR_ADD_SCREENSHOT, "Vision", UpdateSpecForAddScreenshot);

static int32_t AddHeadAndPoseTables(RdbStore &store)
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
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_HEAD_AND_POSE_TABLE, "Vision", AddHeadAndPoseTables);

static int32_t AddWatermarkTable(RdbStore &store)
{
    MEDIA_INFO_LOG("Start add tab_analysis_watermark table");
    const vector<string> sqls = {
        CREATE_TAB_ANALYSIS_WATERMARK,
    };
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_WATERMARK_TABLE);
    MEDIA_INFO_LOG("End add tab_analysis_watermark table");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_WATERMARK_TABLE, "Vision", AddWatermarkTable);

static int32_t AddFaceOcclusionAndPoseTypeColumn(RdbStore &store)
{
    MEDIA_INFO_LOG("start add face occlusion and pose type column");
    MediaLibraryRdbHelper::AddColumnIfNotExists(store, FACE_OCCLUSION, "INT", VISION_IMAGE_FACE_TABLE);
    MediaLibraryRdbHelper::AddColumnIfNotExists(store, POSE_TYPE, "INT", VISION_POSE_TABLE);
    return NativeRdb::E_OK;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_FACE_OCCLUSION_AND_POSE_TYPE_COLUMN,
    "Gallery", AddFaceOcclusionAndPoseTypeColumn);

static int32_t AddSegmentationColumns(RdbStore &store)
{
    const string addNameOnSegmentation = "ALTER TABLE " + VISION_SEGMENTATION_TABLE + " ADD COLUMN " +
        SEGMENTATION_NAME + " INT";
    const string addProbOnSegmentation = "ALTER TABLE " + VISION_SEGMENTATION_TABLE + " ADD COLUMN " +
        PROB + " REAL";

    const vector<string> addSegmentationColumns = { addNameOnSegmentation, addProbOnSegmentation };
    return ExecSqls(addSegmentationColumns, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_SEGMENTATION_COLUMNS,
    "Vision", AddSegmentationColumns);

static int32_t AddSearchTable(RdbStore &store)
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
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_SEARCH_TABLE, "Vision", AddSearchTable);

static int32_t UpdateInsertPhotoUpdateAlbumTrigger(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        INSERT_PHOTO_UPDATE_ALBUM_BUNDLENAME,
    };
    MEDIA_INFO_LOG("start update insert photo update album");
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_PHOTO_ALBUM_BUNDLENAME, "Album", UpdateInsertPhotoUpdateAlbumTrigger);

static int32_t AddPackageNameColumnOnTables(RdbStore &store)
{
    static const string ADD_PACKAGE_NAME_ON_PHOTOS = "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE +
        " ADD COLUMN " + PhotoColumn::MEDIA_PACKAGE_NAME + " TEXT";
    static const string ADD_PACKAGE_NAME_ON_AUDIOS = "ALTER TABLE " + AudioColumn::AUDIOS_TABLE +
        " ADD COLUMN " + AudioColumn::MEDIA_PACKAGE_NAME + " TEXT";
    static const string addPackageNameOnFiles = "ALTER TABLE " + std::string(CONST_MEDIALIBRARY_TABLE) +
        " ADD COLUMN " + CONST_MEDIA_DATA_DB_PACKAGE_NAME + " TEXT";

    int32_t result = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        return store.ExecuteSql(ADD_PACKAGE_NAME_ON_PHOTOS);
    });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Failed to update PHOTOS");
    }
    result = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql(ADD_PACKAGE_NAME_ON_AUDIOS); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Failed to update AUDIOS");
    }
    result = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql(addPackageNameOnFiles); });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Failed to update FILES");
    }
    return result;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_PACKAGE_NAME, "OtherTable", AddPackageNameColumnOnTables);

int32_t UpdateCloudAlbum(RdbStore &store)
{
    /* album - add columns */
    const std::string addAlbumDirty = "ALTER TABLE " + PhotoAlbumColumns::TABLE +
        " ADD COLUMN " + PhotoAlbumColumns::ALBUM_DIRTY + " INT DEFAULT " +
        to_string(static_cast<int32_t>(DirtyTypes::TYPE_NEW)) + ";";
    int32_t ret = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql(addAlbumDirty); });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("upgrade fail %{public}d: add ablum dirty", ret);
        UpdateFail(__FILE__, __LINE__);
    }
    const std::string addAlbumCloudId = "ALTER TABLE " + PhotoAlbumColumns::TABLE +
        " ADD COLUMN " + PhotoAlbumColumns::ALBUM_CLOUD_ID + " TEXT;";
    ret = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql(addAlbumCloudId); });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("upgrade fail %{public}d: add ablum cloud id", ret);
        UpdateFail(__FILE__, __LINE__);
    }
    /* album - add triggers */
    ret = MediaLibraryRdbHelper::ExecSqlWithRetry(
        [&]() { return store.ExecuteSql(PhotoAlbumColumns::CREATE_ALBUM_INSERT_TRIGGER); });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("upgrade fail %{public}d: create album insert trigger", ret);
        UpdateFail(__FILE__, __LINE__);
    }
    ret = MediaLibraryRdbHelper::ExecSqlWithRetry(
        [&]() { return store.ExecuteSql(PhotoAlbumColumns::CREATE_ALBUM_MDIRTY_TRIGGER); });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("upgrade fail %{public}d: create album modify trigger", ret);
        UpdateFail(__FILE__, __LINE__);
    }
    ret = MediaLibraryRdbHelper::ExecSqlWithRetry(
        [&]() { return store.ExecuteSql(PhotoAlbumColumns::CREATE_ALBUM_DELETE_TRIGGER); });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("upgrade fail %{public}d: create album delete trigger", ret);
        UpdateFail(__FILE__, __LINE__);
    }
    /* album map - add columns */
    const std::string addAlbumMapColumns = "ALTER TABLE " + PhotoMap::TABLE +
        " ADD COLUMN " + PhotoMap::DIRTY +" INT DEFAULT " +
        to_string(static_cast<int32_t>(DirtyTypes::TYPE_NEW)) + ";";
    ret = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql(addAlbumMapColumns); });
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("upgrade fail %{public}d: add ablum columns", ret);
        UpdateFail(__FILE__, __LINE__);
    }
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_CLOUD_ALBUM, "Album", UpdateCloudAlbum);

static int32_t AddCameraShotKey(RdbStore &store)
{
    static const string ADD_CAMERA_SHOT_KEY_ON_PHOTOS = "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE +
        " ADD COLUMN " + PhotoColumn::CAMERA_SHOT_KEY + " TEXT";
    int32_t result = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        return store.ExecuteSql(ADD_CAMERA_SHOT_KEY_ON_PHOTOS);
    });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Failed to update PHOTOS");
    }
    result = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        return store.ExecuteSql(PhotoUpgrade::INDEX_CAMERA_SHOT_KEY);
    });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Failed to create CAMERA_SHOT_KEY index");
    }
    return result;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_CAMERA_SHOT_KEY, "Photos", AddCameraShotKey);

int32_t RemoveAlbumCountTrigger(RdbStore &store)
{
    const vector<string> removeAlbumCountTriggers = {
        BaseColumn::DropTrigger() + "update_user_album_count",
        BaseColumn::DropTrigger() + "photo_album_insert_asset",
        BaseColumn::DropTrigger() + "photo_album_delete_asset",
    };
    return ExecSqls(removeAlbumCountTriggers, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_REMOVE_ALBUM_COUNT_TRIGGER, "Album", RemoveAlbumCountTrigger);

int32_t AddExifAndUserComment(RdbStore &store)
{
    const string addUserCommentOnPhotos = "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE +
        " ADD COLUMN " + PhotoColumn::PHOTO_USER_COMMENT + " TEXT";

    const string addAllExifOnPhotos = "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE +
        " ADD COLUMN " + PhotoColumn::PHOTO_ALL_EXIF + " TEXT";

    const vector<string> addExifColumns = { addUserCommentOnPhotos, addAllExifOnPhotos };
    return ExecSqls(addExifColumns, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_ALL_EXIF, "Photos", AddExifAndUserComment);

int32_t AddUpdateCloudSyncTrigger(RdbStore &store)
{
    const vector<string> addUpdateCloudSyncTrigger = { PhotoUpgrade::CREATE_PHOTOS_UPDATE_CLOUD_SYNC };
    return ExecSqls(addUpdateCloudSyncTrigger, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_UPDATE_CLOUD_SYNC_TRIGGER, "Photos", AddUpdateCloudSyncTrigger);

int32_t UpdateYearMonthDayData(RdbStore &store)
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
    return ExecSqls(updateSql, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_YEAR_MONTH_DAY, "Photos", UpdateYearMonthDayData);

int32_t FixIndexOrder(RdbStore &store)
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
    return ExecSqls(updateSql, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_FIX_INDEX_ORDER, "Photos", FixIndexOrder);

int32_t AddYearMonthDayColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_DATE_YEAR + " TEXT",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_DATE_MONTH + " TEXT",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_DATE_DAY + " TEXT",
    };
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_YEAR_MONTH_DAY, "Photos", AddYearMonthDayColumn);

int32_t AddCleanFlagAndThumbStatus(RdbStore &store)
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
    return result;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_PHOTO_CLEAN_FLAG_AND_THUMB_STATUS, "Photos", AddCleanFlagAndThumbStatus);

int32_t AddCloudIndex(RdbStore &store)
{
    const vector<string> sqls = {
        "DROP INDEX IF EXISTS " + PhotoColumn::PHOTO_CLOUD_ID_INDEX,
        PhotoUpgrade::CREATE_CLOUD_ID_INDEX,
    };
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_CLOUD_ID_INDEX, "Photos", AddCloudIndex);

int32_t AddCompositeDisplayStatusColumn(RdbStore &store)
{
    const string addCompositeDisplayStatusOnPhotos = "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE +
        " ADD COLUMN " + PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS + " INT NOT NULL DEFAULT 0";
    const vector<string> addCompositeDisplayStatus = {addCompositeDisplayStatusOnPhotos};
    return ExecSqlsWithDfx(addCompositeDisplayStatus, store, VERSION_ADD_COMPOSITE_DISPLAY_STATUS_COLUMNS);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_COMPOSITE_DISPLAY_STATUS_COLUMNS, "Photos", AddCompositeDisplayStatusColumn);

static int32_t AddPhotoEditTimeColumn(RdbStore &store)
{
    const string addEditTimeOnPhotos = "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE +
        " ADD COLUMN " + PhotoColumn::PHOTO_EDIT_TIME + " BIGINT DEFAULT 0";
    const vector<string> addEditTime = { addEditTimeOnPhotos };
    return ExecSqls(addEditTime, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_PHOTO_EDIT_TIME, "Photos", AddPhotoEditTimeColumn);

int32_t AddShootingModeColumn(RdbStore &store)
{
    const std::string addShootringMode =
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
        PhotoColumn::PHOTO_SHOOTING_MODE + " TEXT";
    const vector<string> addShootingModeColumn = { addShootringMode };
    int32_t result = ExecSqls(addShootingModeColumn, store);
    CHECK_AND_PRINT_LOG(result == NativeRdb::E_OK, "Upgrade rdb shooting_mode error %{private}d", result);
    return result;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_SHOOTING_MODE, "Photos", AddShootingModeColumn);

int32_t AddShootingModeTagColumn(RdbStore &store)
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
    return result;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_SHOOTING_MODE_TAG, "Photos", AddShootingModeTagColumn);

static int32_t AddHiddenViewColumn(RdbStore &store)
{
    vector<string> upgradeSqls = {
        BaseColumn::AlterTableAddIntColumn(PhotoAlbumColumns::TABLE, PhotoAlbumColumns::CONTAINS_HIDDEN),
        BaseColumn::AlterTableAddIntColumn(PhotoAlbumColumns::TABLE, PhotoAlbumColumns::HIDDEN_COUNT),
        BaseColumn::AlterTableAddTextColumn(PhotoAlbumColumns::TABLE, PhotoAlbumColumns::HIDDEN_COVER),
    };
    return ExecSqls(upgradeSqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_HIDDEN_VIEW_COLUMNS, "Album", AddHiddenViewColumn);

static int32_t ModifyMdirtyTriggers(RdbStore &store)
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
    if (MediaLibraryRdbHelper::ExecSqlWithRetry(
        [&]() { return store.ExecuteSql(PhotoUpgrade::CREATE_PHOTOS_MDIRTY_TRIGGER); }) !=
        NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("upgrade fail: create new photos mdirty trigger");
    }

    if (MediaLibraryRdbHelper::ExecSqlWithRetry(
        [&]() { return store.ExecuteSql(CREATE_FILES_MDIRTY_TRIGGER); }) != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("upgrade fail: create new mdirty trigger");
    }
    return NativeRdb::E_OK;
}

static int32_t AddLastVisitTimeColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + AudioColumn::AUDIOS_TABLE + " DROP time_visit ",
        "ALTER TABLE " + REMOTE_THUMBNAIL_TABLE + " DROP time_visit ",
        std::string("ALTER TABLE ") + CONST_MEDIALIBRARY_TABLE + " DROP time_visit ",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " DROP time_visit ",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
        PhotoColumn::PHOTO_LAST_VISIT_TIME + " BIGINT DEFAULT 0",
    };
    int32_t result = ExecSqls(sqls, store);
    CHECK_AND_PRINT_LOG(result == NativeRdb::E_OK, "Upgrade rdb last_visit_time error %{private}d", result);
    return result;
}

static int32_t VersionAddLastVisitTime(RdbStore &store)
{
    int32_t ret = ModifyMdirtyTriggers(store);
    ret = AddLastVisitTimeColumn(store);
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_LAST_VISIT_TIME, "Photos", VersionAddLastVisitTime);

int32_t AddHiddenTimeColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE +
            " ADD COLUMN " + PhotoColumn::PHOTO_HIDDEN_TIME + " BIGINT DEFAULT 0",
        PhotoUpgrade::CREATE_HIDDEN_TIME_INDEX,
    };
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_HIDDEN_TIME, "Photos", AddHiddenTimeColumn);

int32_t AddAlbumOrderColumn(RdbStore &store)
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
    return result;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_ALBUM_ORDER, "Album", AddAlbumOrderColumn);

static int32_t AddFormMap(RdbStore &store)
{
    int32_t result = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        return store.ExecuteSql(FormMap::CREATE_FORM_MAP_TABLE);
    });
    if (result != NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Failed to update PHOTOS");
    }
    return result;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_FORM_MAP, "OtherTable", AddFormMap);

static int32_t FixDocsPath(RdbStore &store)
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

    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_FIX_DOCS_PATH, "Photos", FixDocsPath);

static int32_t AddImageVideoCount(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoAlbumColumns::TABLE +
                " ADD COLUMN " + PhotoAlbumColumns::ALBUM_IMAGE_COUNT + " INT DEFAULT 0",
        "ALTER TABLE " + PhotoAlbumColumns::TABLE +
                " ADD COLUMN " + PhotoAlbumColumns::ALBUM_VIDEO_COUNT + " INT DEFAULT 0",
    };
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_IMAGE_VIDEO_COUNT, "Album", AddImageVideoCount);

static int32_t AddSCHPTHiddenTimeIndex(RdbStore &store)
{
    const vector<string> sqls = {
        PhotoUpgrade::CREATE_SCHPT_HIDDEN_TIME_INDEX,
    };
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_SCHPT_HIDDEN_TIME_INDEX, "Photos", AddSCHPTHiddenTimeIndex);

static int32_t UpdateClassifyDirtyData(RdbStore &store)
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
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_CLEAR_LABEL_DATA, "Photos", UpdateClassifyDirtyData);

static int32_t UpdateGeoTables(RdbStore &store)
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
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_GEO_TABLE, "Vision", UpdateGeoTables);

static int32_t UpdatePhotosMdirtyTrigger(RdbStore& store)
{
    string dropSql = "DROP TRIGGER IF EXISTS photos_mdirty_trigger";
    if (MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql(dropSql); }) != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to drop old photos_mdirty_trigger: %{private}s", dropSql.c_str());
        UpdateFail(__FILE__, __LINE__);
        return NativeRdb::E_ERROR;
    }

    if (MediaLibraryRdbHelper::ExecSqlWithRetry(
        [&]() { return store.ExecuteSql(PhotoUpgrade::CREATE_PHOTOS_MDIRTY_TRIGGER); }) !=
        NativeRdb::E_OK) {
        UpdateFail(__FILE__, __LINE__);
        MEDIA_ERR_LOG("Failed to upgrade new photos_mdirty_trigger, %{private}s",
            PhotoUpgrade::CREATE_PHOTOS_MDIRTY_TRIGGER.c_str());
        return NativeRdb::E_ERROR;
    }
    return NativeRdb::E_OK;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_PHOTOS_MDIRTY_TRIGGER, "Photos", UpdatePhotosMdirtyTrigger);
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_MDIRTY_TRIGGER_FOR_UPLOADING_MOVING_PHOTO,
    "Photos", UpdatePhotosMdirtyTrigger);
REGISTER_SYNC_UPGRADE_TASK(VERSION_CREATE_TAB_OLD_PHOTOS,
    "OtherTable", TabOldPhotosTableEventHandler().OnCreate);

static int32_t AddIndexForFileId(RdbStore& store)
{
    const vector<string> sqls = {
        CREATE_IDX_FILEID_FOR_SEARCH_INDEX,
        CREATE_IDX_FILEID_FOR_ANALYSIS_TOTAL,
        CREATE_IDX_FILEID_FOR_ANALYSIS_PHOTO_MAP,
    };
    MEDIA_INFO_LOG("start AddIndexForFileId");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_INDEX_FOR_FILEID, "Photos", AddIndexForFileId);

int32_t AddIndexForFileIdAsync(RdbStore& store)
{
    MEDIA_INFO_LOG("AddIndexForFileIdAsync start");
    const vector<string> updateSql = {
        CREATE_IDX_FILEID_FOR_SEARCH_INDEX,
        CREATE_IDX_FILEID_FOR_ANALYSIS_TOTAL,
        CREATE_IDX_FILEID_FOR_ANALYSIS_PHOTO_MAP,
    };
    int32_t ret = ExecSqls(updateSql, store);
    MEDIA_INFO_LOG("AddIndexForFileIdAsync end");
    return ret;
}

static int32_t AddMetaRecovery(RdbStore& store)
{
    const vector<string> sqls = {"ALTER TABLE " + PhotoColumn::PHOTOS_TABLE +
        " ADD COLUMN " + PhotoColumn::PHOTO_METADATA_FLAGS + " INT DEFAULT 0",
        PhotoUpgrade::CREATE_PHOTOS_METADATA_DIRTY_TRIGGER,
    };
    MEDIA_INFO_LOG("start AddMetaRecovery");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_METARECOVERY, "Photos", AddMetaRecovery);

static int32_t AddCloudEnhancementAlbum(RdbStore& store)
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
    MediaLibraryRdbHelper::BuildValuesSql(values, bindArgs, sql);
    sql.append(" WHERE NOT EXISTS (");
    MediaLibraryRdbHelper::BuildQuerySql(predicates, { PhotoAlbumColumns::ALBUM_ID }, bindArgs, sql);
    sql.append(");");
    err = store.ExecuteSql(sql, bindArgs);
    values.Clear();
    if (err != E_OK) {
        MEDIA_ERR_LOG("Add cloud enhancement album failed, err: %{public}d", err);
    }
    return err;
}

static int32_t AddHighlightTriggerColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::HIGHLIGHT_TABLE + " ADD COLUMN " +
            PhotoColumn::MEDIA_DATA_DB_HIGHLIGHT_TRIGGER + " INT DEFAULT 0"
    };
    MEDIA_INFO_LOG("start add highlight trigger column");
    ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end add highlight trigger column");
    return NativeRdb::E_OK;
}

int32_t AddHighlightInsertAndUpdateTrigger(RdbStore &store)
{
    const vector<string> sqls = {
        PhotoUpgrade::INSERT_GENERATE_HIGHLIGHT_THUMBNAIL,
        PhotoUpgrade::UPDATE_GENERATE_HIGHLIGHT_THUMBNAIL
    };
    MEDIA_INFO_LOG("start add highlight insert and update trigger");
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end add highlight insert and update trigger");
    return ret;
}

static int32_t AddHighlightIndex(RdbStore &store)
{
    const vector<string> addHighlightIndex = { PhotoUpgrade::INDEX_HIGHLIGHT_FILEID };
    MEDIA_INFO_LOG("start add highlight index");
    int32_t ret = ExecSqls(addHighlightIndex, store);
    MEDIA_INFO_LOG("end add highlight index");
    return ret;
}

static int32_t UpdateSearchIndexTriggerForCleanFlag(RdbStore& store)
{
    const vector<string> sqls = {
        "DROP TRIGGER IF EXISTS update_search_status_trigger",
        CREATE_SEARCH_UPDATE_STATUS_TRIGGER,
    };
    MEDIA_INFO_LOG("start update search index for clean flag");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_SEARCH_INDEX_TRIGGER_FOR_CLEAN_FLAG,
    "Photos", UpdateSearchIndexTriggerForCleanFlag);

static int32_t UpdateAlbumRefreshTable(RdbStore &store)
{
    const vector<string> sqls = {
        CREATE_ALBUM_REFRESH_TABLE,
    };
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ALBUM_REFRESH, "Album", UpdateAlbumRefreshTable);

static int32_t AddCoverPlayVersionColumns(RdbStore& store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + HIGHLIGHT_COVER_INFO_TABLE +
            " ADD COLUMN " + COVER_SERVICE_VERSION + " INT DEFAULT 0",
        "ALTER TABLE " + HIGHLIGHT_PLAY_INFO_TABLE +
            " ADD COLUMN " + PLAY_SERVICE_VERSION + " INT DEFAULT 0",
    };
    MEDIA_INFO_LOG("start add cover play version columns");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_COVER_PLAY_SERVICE_VERSION, "Album", AddCoverPlayVersionColumns);

static int32_t AddMovingPhotoRelatedData(RdbStore &store)
{
    const vector<string> sqls = {
        CREATE_TAB_VIDEO_ANALYSIS_AESTHETICS,
    };
    MEDIA_INFO_LOG("start create video aesthetics score table");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_HIGHLIGHT_MOVING_PHOTO, "Album", AddMovingPhotoRelatedData);

static int32_t UpdateFavoriteIndex(RdbStore &store)
{
    MEDIA_INFO_LOG("Upgrade rdb UpdateFavoriteIndex");
    const vector<string> sqls = {
        PhotoUpgrade::CREATE_PHOTO_FAVORITE_INDEX,
        PhotoUpgrade::DROP_SCHPT_MEDIA_TYPE_INDEX,
        PhotoUpgrade::CREATE_SCHPT_MEDIA_TYPE_INDEX,
    };
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_FAVORITE_INDEX, "Photos", UpdateFavoriteIndex);

static int32_t AddMissingUpdates(RdbStore &store)
{
    MEDIA_INFO_LOG("start add missing updates");
    vector<string> sqls;
    bool hasShootingModeTag = MediaLibraryRdbHelper::HasColumnInTable(store, PhotoColumn::PHOTO_SHOOTING_MODE_TAG,
        PhotoColumn::PHOTOS_TABLE);
    if (!hasShootingModeTag) {
        MEDIA_INFO_LOG("start add shooting mode tag");
        const vector<string> sqls = {
            "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_SHOOTING_MODE_TAG +
                " TEXT",
        };
        ExecSqls(sqls, store);
    }
    bool hasBundleName = MediaLibraryRdbHelper::HasColumnInTable(store, PhotoAlbumColumns::ALBUM_BUNDLE_NAME,
        PhotoAlbumColumns::TABLE);
    bool hasLocalLanguage = MediaLibraryRdbHelper::HasColumnInTable(store, PhotoAlbumColumns::ALBUM_LOCAL_LANGUAGE,
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
    return NativeRdb::E_OK;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_MISSING_UPDATES, "Photos", AddMissingUpdates);

int32_t AddMultiStagesCaptureColumns(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_ID + " TEXT",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_QUALITY + " INT",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_FIRST_VISIT_TIME +
            " BIGINT DEFAULT 0",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_DEFERRED_PROC_TYPE +
            " INT DEFAULT 0",
    };
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_MULTISTAGES_CAPTURE, "Photos", AddMultiStagesCaptureColumns);

static void AddAlbumSceneIdAndShareTypeColumns(RdbStore &store, int32_t version)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
            PhotoAlbumColumns::ALBUM_SCENE_ID + " INT DEFAULT 0",
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
            PhotoAlbumColumns::ALBUM_SHARE_TYPE + " INT DEFAULT 0",
    };
    MEDIA_INFO_LOG("Add scene_id and share_type columns for PhotoAlbum start");
    ExecSqlsWithDfx(sqls, store, version);
    MEDIA_INFO_LOG("Add scene_id and share_type columns for PhotoAlbum end");
}

static int32_t CreateBackupInfoTable(RdbStore& store)
{
    MEDIA_INFO_LOG("create table ConfigInfo start");
    int32_t ret = ExecSqlsWithDfx({ConfigInfoColumn::CREATE_CONFIG_INFO_TABLE}, store, VERSION_ADD_MEDIA_BACKUP_INFO);
    MEDIA_INFO_LOG("create table ConfigInfo end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_MEDIA_BACKUP_INFO, "OtherTable", CreateBackupInfoTable);

int32_t UpdateMillisecondDate(RdbStore &store)
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
        std::string("UPDATE ") + CONST_MEDIALIBRARY_TABLE + " SET " +
        MediaColumn::MEDIA_DATE_ADDED + " = " + MediaColumn::MEDIA_DATE_ADDED + "*1000," +
        MediaColumn::MEDIA_DATE_MODIFIED + " = " + MediaColumn::MEDIA_DATE_MODIFIED + "*1000;",
    };
    int32_t ret = ExecSqls(updateSql, store);
    MEDIA_DEBUG_LOG("UpdateMillisecondDate end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_DATE_TO_MILLISECOND, "Photos", UpdateMillisecondDate);

int32_t AddHasAstcColumns(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_HAS_ASTC + " INT DEFAULT 0 ",
    };
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_HAS_ASTC, "Photos", AddHasAstcColumns);

int32_t AddAddressDescriptionColumns(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + GEO_KNOWLEDGE_TABLE + " ADD COLUMN " + CITY_NAME + " TEXT",
        "ALTER TABLE " + GEO_KNOWLEDGE_TABLE + " ADD COLUMN " + ADDRESS_DESCRIPTION + " TEXT",
    };
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_ADDRESS_DESCRIPTION, "Photos", AddAddressDescriptionColumns);

int32_t AddIsLocalAlbum(RdbStore &store)
{
    const vector<string> sqls = {
        ADD_IS_LOCAL_COLUMN_FOR_ALBUM,
        ADD_PHOTO_ALBUM_IS_LOCAL,
    };
    MEDIA_INFO_LOG("start add islocal column");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_IS_LOCAL_ALBUM, "Vision", AddIsLocalAlbum);

int32_t AddSourceAndTargetTokenForUriPermission(RdbStore &store)
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
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_URIPERMISSION_SOURCE_TOKEN_AND_TARGET_TOKEN, "OtherTable",
    AddSourceAndTargetTokenForUriPermission);

static int32_t UpdateAOI(RdbStore &store)
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
    return ExecSqls(sqls, store);
}

static int32_t AddGeoDefaultValue(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + VISION_TOTAL_TABLE + " DROP COLUMN " + GEO,
        "ALTER TABLE " + VISION_TOTAL_TABLE + " ADD COLUMN " + GEO + " INT DEFAULT 0",
    };
    MEDIA_INFO_LOG("Add geo deault value start");
    return ExecSqls(sqls, store);
}

static int32_t UpdateAOIUpgradeWrapper(RdbStore &store)
{
    int32_t ret = UpdateAOI(store);
    ret = AddGeoDefaultValue(store);
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_AOI, "Vision", UpdateAOIUpgradeWrapper);

static int32_t FixSourceAlbumUpdateTriggerToUseLPath(RdbStore& store)
{
    const vector<string> sqls = {
        DROP_INSERT_SOURCE_PHOTO_UPDATE_ALBUM_ID_TRIGGER,
        CREATE_INSERT_SOURCE_UPDATE_ALBUM_ID_TRIGGER
    };
    MEDIA_INFO_LOG("Fix source album update trigger to use lpath start");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_FIX_SOURCE_ALBUM_UPDATE_TRIGGER_TO_USE_LPATH,
    "Album", FixSourceAlbumUpdateTriggerToUseLPath);

static int32_t AddMediaSuffixColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_MEDIA_SUFFIX + " TEXT",
    };
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_MEDIA_SUFFIX_COLUMN, "Photos", AddMediaSuffixColumn);

static int32_t AddAppLinkColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_HAS_APPLINK +
            " INT NOT NULL DEFAULT 0",
    };
    MEDIA_INFO_LOG("Start add has_applink column");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_APPLINK_VERSION);
    MEDIA_INFO_LOG("End add has_applink column");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_APPLINK_VERSION, "Photos", AddAppLinkColumn);

static int32_t AddDcAnalysisColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + USER_PHOTOGRAPHY_INFO_TABLE + " ADD COLUMN " + DC_MODIFY_TIME_STAMP + " BIGINT DEFAULT 0",
        "ALTER TABLE " + USER_PHOTOGRAPHY_INFO_TABLE + " ADD COLUMN " + DC_MODIFY_TIME_STAMP + " BIGINT DEFAULT 0",
    };
    MEDIA_INFO_LOG("Add DC analysis column start");
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("Add DC analysis column end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_DC_ANALYSIS, "Vision", AddDcAnalysisColumn);

static int32_t AddDcAnalysisIndexUpdateColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + USER_PHOTOGRAPHY_INFO_TABLE + " ADD COLUMN " + DC_INDEX_UPDATE_COUNT + " INT DEFAULT 0",
    };
    MEDIA_INFO_LOG("Add DC analysis index update column start");
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("Add DC analysis index update column end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_DC_ANALYSIS_INDEX_UPDATE, "Vision", AddDcAnalysisIndexUpdateColumn);

static int32_t AddIsRectificationCover(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_IS_RECTIFICATION_COVER +
            " INT NOT NULL DEFAULT 0",
    };
    MEDIA_INFO_LOG("Add is_rectification_cover column start");
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("Add is_rectification_cover column end");
    return ret;
}

static int32_t AddIsRectificationCoverWithTriggerUpdate(RdbStore &store)
{
    MEDIA_INFO_LOG("Start add is_rectification_cover with trigger update");
    int32_t ret = AddIsRectificationCover(store);
    if (ret == NativeRdb::E_OK) {
        ret = UpdatePhotosMdirtyTrigger(store);
    }
    MEDIA_INFO_LOG("End add is_rectification_cover with trigger update");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_IS_RECTIFICATION_COVER, "Album", AddIsRectificationCoverWithTriggerUpdate);

static int32_t AddIsPrimaryFace(RdbStore &store)
{
    const vector<string> sql = {
        " ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + IS_PRIMARY_FACE + " REAL ",
    };
    MEDIA_INFO_LOG("Adding IS_PRIMARY_FACE column for VISION_IMAGE_FACE_TABLE");
    int32_t ret = ExecSqls(sql, store);
    MEDIA_INFO_LOG("end add is_primary_face column");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_IS_PRIMARY_FACE, "Vision", AddIsPrimaryFace);

static int32_t AddCoverColumns(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " + PhotoAlbumColumns::COVER_URI_SOURCE +
            " INT NOT NULL DEFAULT 0",
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " + PhotoAlbumColumns::COVER_CLOUD_ID +
            " TEXT",
    };
    MEDIA_INFO_LOG("add cover columns start");
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("add cover columns end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_COVER_URI_SOURCE, "Album", AddCoverColumns);

static int32_t AddHighlightLocation(RdbStore &store)
{
    const vector<string> sql = {
        " ALTER TABLE " + HIGHLIGHT_ALBUM_TABLE + " ADD COLUMN " + HIGHLIGHT_LOCATION + " TEXT ",
    };
    MEDIA_INFO_LOG("start add highlight location column");
    int32_t ret = ExecSqls(sql, store);
    MEDIA_INFO_LOG("end add highlight location column");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_HIGHLIGHT_LOCATION, "Album", AddHighlightLocation);

static int32_t AddTotalPriority(RdbStore &store)
{
    const vector<string> sql = {
        "ALTER TABLE " + VISION_TOTAL_TABLE + " ADD COLUMN " + PRIORITY + " INT NOT NULL DEFAULT 1",
    };
    MEDIA_INFO_LOG("Addding priority for VISION_TOTAL_TABLE");
    int32_t ret = ExecSqls(sql, store);
    MEDIA_INFO_LOG("end add priority column");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_PRIORITY_COLUMN, "Album", AddTotalPriority);

static int32_t AddPhotoAlbumRefreshColumns(RdbStore &store)
{
    const vector<string> exeSqls = {
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
            PhotoAlbumColumns::COVER_DATE_TIME + " BIGINT DEFAULT 0",
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
            PhotoAlbumColumns::HIDDEN_COVER_DATE_TIME + " BIGINT DEFAULT 0",
        DROP_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER,
        CREATE_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER,
    };
    MEDIA_INFO_LOG("start add photo album cover_date_time and hidden_cover_date_time for AccurateRefresh");
    int32_t ret = ExecSqls(exeSqls, store);
    MEDIA_INFO_LOG("end add photo album cover_date_time and hidden_cover_date_time for AccurateRefresh");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_PHOTO_ALBUM_REFRESH_COLUMNS, "Album", AddPhotoAlbumRefreshColumns);

static int32_t AddEditDataSizeColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoExtColumn::PHOTOS_EXT_TABLE + " ADD COLUMN " + PhotoExtColumn::EDITDATA_SIZE +
        " BIGINT NOT NULL DEFAULT 0",
    };
    MEDIA_INFO_LOG("Add editdata size column start");
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("Add editdata size column end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_EDITDATA_SIZE_COLUMN, "Photos", AddEditDataSizeColumn);

static int32_t FixSourceAlbumCreateTriggersToUseLPath(RdbStore& store)
{
    const vector<string> sqls = {
        DROP_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER,
        CREATE_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER
    };
    MEDIA_INFO_LOG("Fix source album other triggers to use lpath start");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_FIX_SOURCE_ALBUM_CREATE_TRIGGERS_TO_USE_LPATH,
    "Album", FixSourceAlbumCreateTriggersToUseLPath);

static int32_t AddAlbumPluginBundleName(RdbStore &store)
{
    MEDIA_INFO_LOG("Start updating album plugin");
    const vector<string> sqls = {
        "DROP TABLE IF EXISTS album_plugin;"
    };
    int32_t ret = ExecSqls(sqls, store);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Drop album_plugin table failed");
        return ret;
    }
    AlbumPluginTableEventHandler().OnCreate(store);
    MEDIA_INFO_LOG("End updating album plugin");
    return NativeRdb::E_OK;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_ALBUM_PLUGIN_BUNDLE_NAME, "Album", AddAlbumPluginBundleName);

static int32_t FixMdirtyTriggerToUploadDetailTime(RdbStore &store)
{
    MEDIA_INFO_LOG("Start updating mdirty trigger to upload detail_time");
    const vector<string> sqls = {
        "DROP TRIGGER IF EXISTS photos_mdirty_trigger",
        PhotoUpgrade::CREATE_PHOTOS_MDIRTY_TRIGGER,
    };
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("End updating mdirty trigger to upload detail_time");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_MDIRTY_TRIGGER_UPLOAD_DETAIL_TIME, "Photos", FixMdirtyTriggerToUploadDetailTime);

static int32_t FixDbUpgradeFromApi18Wrapper(RdbStore &store)
{
    MEDIA_INFO_LOG("Start VERSION_MDIRTY_TRIGGER_UPLOAD_DETAIL_TIME & VERSION_UPDATE_MDIRTY_TRIGGER_FOR_TDIRTY");
    int32_t ret = FixMdirtyTriggerToUploadDetailTime(store);
    MEDIA_INFO_LOG("End VERSION_MDIRTY_TRIGGER_UPLOAD_DETAIL_TIME & VERSION_UPDATE_MDIRTY_TRIGGER_FOR_TDIRTY");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_FIX_DB_UPGRADE_FROM_API18, "Photos", FixDbUpgradeFromApi18Wrapper);

static int32_t AddLakeFileColumn(RdbStore &store)
{
    MEDIA_INFO_LOG("Start add lake file column");
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_FILE_INODE + " TEXT",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_STORAGE_PATH + " TEXT",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_FILE_SOURCE_TYPE +
            " INT NOT NULL DEFAULT 0",
    };
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_LAKE_COLUMN);
    MEDIA_INFO_LOG("End add lake file column");
    return ret;
}

static int32_t AddLakeAlbumTable(RdbStore &store)
{
    MEDIA_INFO_LOG("Start add lake album table");
    const vector<string> sqls = {
        CREATE_LAKE_ALBUM_TABLE,
    };
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_LAKE_COLUMN);
    MEDIA_INFO_LOG("End add lake album table");
    return ret;
}

static int32_t AddLakeColumnWithAlbumTable(RdbStore &store)
{
    MEDIA_INFO_LOG("Start VERSION_ADD_LAKE_COLUMN");
    int32_t ret = AddLakeFileColumn(store);
    if (ret == NativeRdb::E_OK) {
        ret = AddLakeAlbumTable(store);
    }
    MEDIA_INFO_LOG("End VERSION_ADD_LAKE_COLUMN");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_LAKE_COLUMN, "Album", AddLakeColumnWithAlbumTable);

static int32_t AddPhotoMovingphotoEnhancementType(RdbStore &store)
{
    MEDIA_INFO_LOG("Add moving photo enhancement column start");
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE +
            " INT NOT NULL DEFAULT 0",
    };
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE);
    MEDIA_INFO_LOG("Add moving photo enhancement column end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE,
    "Photos", AddPhotoMovingphotoEnhancementType);

void UpdateVideoFaceTable(RdbStore &store)
{
    const vector<string> sqls = {
        "DROP TABLE IF EXISTS " + VISION_VIDEO_FACE_TABLE,
        CREATE_TAB_VIDEO_FACE,
    };
    MEDIA_INFO_LOG("start update video face db");
    ExecSqls(sqls, store);
}

int32_t AddHighlightChangeFunction(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + ANALYSIS_PHOTO_MAP_TABLE + " ADD COLUMN " + ORDER_POSITION + " INT ",
        "ALTER TABLE " + HIGHLIGHT_COVER_INFO_TABLE + " ADD COLUMN " + COVER_STATUS + " INT ",
        "ALTER TABLE " + HIGHLIGHT_PLAY_INFO_TABLE + " ADD COLUMN " + PLAY_INFO_STATUS + " INT ",
        "ALTER TABLE " + HIGHLIGHT_ALBUM_TABLE + " ADD COLUMN " + HIGHLIGHT_PIN_TIME + " BIGINT ",
    };
    MEDIA_INFO_LOG("start add highlight change function");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_HIGHLIGHT_CHANGE_FUNCTION, "Vision",
    AddHighlightChangeFunction);

static int32_t AddHighlightViewedNotification(RdbStore &store)
{
    MEDIA_INFO_LOG("start AddHighlightViewedNotification");
    const vector<string> sqls = {
        "ALTER TABLE " + HIGHLIGHT_ALBUM_TABLE + " ADD COLUMN " +
            HIGHLIGHT_IS_VIEWED + " BOOL NOT NULL DEFAULT 0 ",
        "ALTER TABLE " + HIGHLIGHT_ALBUM_TABLE + " ADD COLUMN " +
            HIGHLIGHT_NOTIFICATION_TIME + " BIGINT NOT NULL DEFAULT 0 ",
    };
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_HIGHLIGHT_VIEWED_NOTIFICATION);
    MEDIA_INFO_LOG("end AddHighlightViewedNotification");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_HIGHLIGHT_VIEWED_NOTIFICATION, "Vision", AddHighlightViewedNotification);

static int32_t AddAestheticsScoreFileds(RdbStore &store)
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
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end add aesthetics score fields");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_AESTHETICS_SCORE_FIELDS, "Vision", AddAestheticsScoreFileds);

static int32_t AddAlbumsOrderKeysColumn(RdbStore &store)
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
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("Add album order keys column end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_ALBUMS_ORDER_KEYS_COLUMNS, "Album", AddAlbumsOrderKeysColumn);

static int32_t AddStoryTables(RdbStore &store)
{
    const vector<string> executeSqlStrs = {
        CREATE_HIGHLIGHT_ALBUM_TABLE,
        CREATE_HIGHLIGHT_COVER_INFO_TABLE,
        CREATE_HIGHLIGHT_PLAY_INFO_TABLE,
        CREATE_USER_PHOTOGRAPHY_INFO_TABLE,
        "ALTER TABLE " + VISION_LABEL_TABLE + " ADD COLUMN " + SALIENCY_SUB_PROB + " TEXT",
    };
    MEDIA_INFO_LOG("start init story db");
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_STOYR_TABLE, "Album", AddStoryTables);

int32_t UpdateAnalysisTables(RdbStore &store)
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
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_ANALYSIS_TABLES, "Vision", UpdateAnalysisTables);

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

int32_t UpdateHighlightTablePrimaryKey(RdbStore &store)
{
    const vector<string> executeSqlStrs = {
        "DROP TABLE IF EXISTS tab_highlight_album",
        "DROP TABLE IF EXISTS tab_highlight_cover_info",
        CREATE_HIGHLIGHT_ALBUM_TABLE,
        CREATE_HIGHLIGHT_COVER_INFO_TABLE,
    };
    MEDIA_INFO_LOG("update primary key of highlight db");
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_HIGHLIGHT_TABLE_PRIMARY_KEY, "Album", UpdateHighlightTablePrimaryKey);

static int32_t AddBussinessRecordAlbum(RdbStore &store)
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
    int32_t ret = ExecSqls(sqls, store);
    UpdatePhotosMdirtyTrigger(store);
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_SHOOTING_MODE_CLOUD, "Album", AddBussinessRecordAlbum);

int32_t AddIsCoverSatisfiedColumn(RdbStore &store)
{
    const vector<string> sqls = {
        ADD_IS_COVER_SATISFIED_FOR_ALBUM,
    };
    MEDIA_INFO_LOG("start add is cover satisfied column");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_IS_COVER_SATISFIED_COLUMN, "Vision", AddIsCoverSatisfiedColumn);

static int32_t AddOwnerAppId(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + MediaColumn::MEDIA_OWNER_APPID + " TEXT",
        "ALTER TABLE " + AudioColumn::AUDIOS_TABLE + " ADD COLUMN " + MediaColumn::MEDIA_OWNER_APPID + " TEXT"
    };
    MEDIA_INFO_LOG("start add owner_appid column");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_OWNER_APPID, "Photos", AddOwnerAppId);

int32_t UpdateThumbnailReadyColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " RENAME COLUMN " + PhotoColumn::PHOTO_HAS_ASTC
            + " TO " + PhotoColumn::PHOTO_THUMBNAIL_READY,
    };
    MEDIA_INFO_LOG("update has_astc to thumbnail_ready begin");
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("update has_astc to thumbnail_ready finished");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_PHOTO_THUMBNAIL_READY, "Photos", UpdateThumbnailReadyColumn);

int32_t AddOwnerAppIdToFiles(RdbStore &store)
{
    const vector<string> sqls = {
        string("ALTER TABLE ") + CONST_MEDIALIBRARY_TABLE + " ADD COLUMN " + MediaColumn::MEDIA_OWNER_APPID + " TEXT"
    };
    MEDIA_INFO_LOG("start add owner_appid column to files table");
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("add owner_appid column to files table finished");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_OWNER_APPID_TO_FILES_TABLE, "Photos", AddOwnerAppIdToFiles);

static int32_t AddDynamicRangeType(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE + " INT DEFAULT 0"
    };
    MEDIA_INFO_LOG("start add dynamic_range_type column");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_DYNAMIC_RANGE_TYPE, "Photos", AddDynamicRangeType);

int32_t AddHdrMode(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_HDR_MODE + " INT DEFAULT 0 NOT NULL"
    };
    MEDIA_INFO_LOG("start add hdr_mode column");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_HDR_MODE);
    MEDIA_INFO_LOG("end add hdr_mode column");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_HDR_MODE, "Photos", AddHdrMode);

int32_t AddCriticalTypeColumns(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_IS_CRITICAL + " INT DEFAULT 0 NOT NULL"
    };

    MEDIA_INFO_LOG("add Photos is_critical column starts");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_CRITICAL_TYPE_COLUMN_ON_PHOTOS);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to execute SQLs, error code: %d", ret);
        return ret;
    }
    MEDIA_INFO_LOG("add Photos is_critical column ends");
    return NativeRdb::E_OK;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_CRITICAL_TYPE_COLUMN_ON_PHOTOS,
    "Photos", AddCriticalTypeColumns);

static int32_t AddLcdAndThumbSizeColumns(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_LCD_SIZE + " TEXT",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_THUMB_SIZE + " TEXT",
    };
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_THUMB_LCD_SIZE_COLUMN, "Photos", AddLcdAndThumbSizeColumns);

int32_t AddUniqueIdColumns(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::UNIQUE_ID + " TEXT DEFAULT NULL",
    };

    MEDIA_INFO_LOG("add Photos unique_id columns starts");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_UNIQUE_ID_COLUMN_ON_PHOTOS);
    MEDIA_INFO_LOG("add Photos unique_id columns ends");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_UNIQUE_ID_COLUMN_ON_PHOTOS, "Photos", AddUniqueIdColumns);

static int32_t UpdatePhotoAlbumTigger(RdbStore &store, int32_t version)
{
    static const vector<string> executeSqlStrs = {
        "DROP TRIGGER IF EXISTS album_modify_trigger",
        PhotoAlbumColumns::CREATE_ALBUM_MDIRTY_TRIGGER,
    };
    MEDIA_INFO_LOG("Start update album modify trigger");
    return ExecSqlsWithDfx(executeSqlStrs, store, version);
}

static int32_t UpdatePhotoAlbumTiggerWrapper(RdbStore &store)
{
    return UpdatePhotoAlbumTigger(store, VERSION_UPDATE_PHOTO_ALBUM_TIGGER);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_PHOTO_ALBUM_TIGGER, "Album", UpdatePhotoAlbumTiggerWrapper);

static int32_t UpdatePhotoAlbumDateModifiedTiggerWrapper(RdbStore &store)
{
    return UpdatePhotoAlbumTigger(store, VERSION_UPDATE_PHOTO_ALBUM_DATEMODIFIED_TIGGER);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_PHOTO_ALBUM_DATEMODIFIED_TIGGER, "Album",
    UpdatePhotoAlbumDateModifiedTiggerWrapper);

static int32_t UpdateAnalysisAlbumRelationship(RdbStore &store)
{
    const string addRelationColumn = "ALTER TABLE " + ANALYSIS_ALBUM_TABLE + " ADD COLUMN " +
        ALBUM_RELATIONSHIP + " TEXT(64) ";
    static const vector<string> executeSqlStrs = {
        addRelationColumn,
        "DROP TRIGGER IF EXISTS " + ANALYSIS_ALBUM_UPDATE_SEARCH_TRIGGER,
        CREATE_ANALYSIS_ALBUM_UPDATE_SEARCH_TRIGGER,
    };
    MEDIA_INFO_LOG("Start update album modify trigger");
    int32_t ret = ExecSqlsWithDfx(executeSqlStrs, store, VERSION_ADD_RELATIONSHIP_AND_UPDATE_TRIGGER);
    MEDIA_INFO_LOG("End update album modify trigger");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_RELATIONSHIP_AND_UPDATE_TRIGGER, "Vision", UpdateAnalysisAlbumRelationship);

static int32_t AddMovingPhotoEffectMode(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::MOVING_PHOTO_EFFECT_MODE + " INT DEFAULT 0"
    };
    MEDIA_INFO_LOG("start add moving_photo_effect_mode column");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_MOVING_PHOTO_EFFECT_MODE, "Photos", AddMovingPhotoEffectMode);

int32_t AddBurstCoverLevelAndBurstKey(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_BURST_COVER_LEVEL +
            " INT DEFAULT 1",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_BURST_KEY + " TEXT",
    };
    MEDIA_INFO_LOG("start add burst_cover_level and burst_key column");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_BURST_COVER_LEVEL_AND_BURST_KEY, "Photos", AddBurstCoverLevelAndBurstKey);

static int32_t AddCloudEnhancementColumns(RdbStore &store)
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
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_CLOUD_ENAHCNEMENT, "Photos", AddCloudEnhancementColumns);

static int32_t AddAnalysisStatus(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        "ALTER TABLE " + ANALYSIS_ALBUM_TABLE + " ADD COLUMN " + ANALYSIS_STATUS + " INT ",
    };
    MEDIA_INFO_LOG("start add analysis status column");
    int32_t ret = ExecSqlsWithDfx(executeSqlStrs, store, VERSION_ADD_ANALYSIS_STATUS);
    MEDIA_INFO_LOG("end add analysis status column");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_ANALYSIS_STATUS, "Vision", AddAnalysisStatus);

static int32_t AddIsAutoColumns(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_IS_AUTO + " INT DEFAULT 0 NOT NULL",
    };
    MEDIA_INFO_LOG("start add is_auto column for auto cloud enhancement");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_IS_AUTO, "Photos", AddIsAutoColumns);

static int32_t AddThumbnailReady(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_THUMBNAIL_READY + " INT DEFAULT 0",
    };
    MEDIA_INFO_LOG("start add thumbnail ready columns");
    return ExecSqls(sqls, store);
}

static int32_t AddCheckFlag(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_CHECK_FLAG + " INT DEFAULT 0",
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
            PhotoAlbumColumns::ALBUM_CHECK_FLAG + " INT DEFAULT 0",
    };
    MEDIA_INFO_LOG("start add check_flag columns");
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end add check_flag columns");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_CHECK_FLAG, "Photos", AddCheckFlag);

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

static int32_t CreateTabFacardPhotos(RdbStore &store)
{
    TabFaCardPhotosTableEventHandler().OnCreate(store);
    return NativeRdb::E_OK;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_CREATE_TAB_FACARD_PHOTOS, "Photos", CreateTabFacardPhotos);

static int32_t CreateTabFacardPhotosRetry(RdbStore &store)
{
    TabFaCardPhotosTableEventHandler().OnCreate(store);
    return NativeRdb::E_OK;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_CREATE_TAB_FACARD_PHOTOS_RETRY, "Photos", CreateTabFacardPhotosRetry);

static int32_t AddCloudEnhanceColumnsFix(RdbStore& store)
{
    MEDIA_INFO_LOG("Start checking cloud enhancement column");
    bool hasColumn = IsColumnExists(store, PhotoColumn::PHOTOS_TABLE, PhotoColumn::PHOTO_CE_AVAILABLE);
    MEDIA_INFO_LOG("End checking cloud enhancement column: %{public}d", hasColumn);
    if (!hasColumn) {
        AddCloudEnhancementColumns(store);
        MEDIA_INFO_LOG("Add Cloud Enhance Cols completed successfully");
    }
    return NativeRdb::E_OK;
}

static int32_t AddDynamicRangeColumnsFix(RdbStore& store)
{
    MEDIA_INFO_LOG("Start checking dynamic_range_type column");
    bool hasColumn = IsColumnExists(store, PhotoColumn::PHOTOS_TABLE, PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE);
    MEDIA_INFO_LOG("End checking dynamic_range_type column: %{public}d", hasColumn);
    if (!hasColumn) {
        AddDynamicRangeType(store);
        MEDIA_INFO_LOG("Add Dynamic Range Cols completed successfully");
    }
    return NativeRdb::E_OK;
}

static int32_t AddThumbnailReadyColumnsFix(RdbStore& store)
{
    MEDIA_INFO_LOG("Start checking thumbnail_ready column");
    bool hasColumn = IsColumnExists(store, PhotoColumn::PHOTOS_TABLE, PhotoColumn::PHOTO_THUMBNAIL_READY);
    MEDIA_INFO_LOG("End checking thumbnail_ready column: %{public}d", hasColumn);
    if (!hasColumn) {
        AddThumbnailReady(store);
        MEDIA_INFO_LOG("Add ThumbnailReady Column");
    }
    return NativeRdb::E_OK;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_THUMBNAIL_READY_FIX, "Photos", AddThumbnailReadyColumnsFix);

static int32_t UpdateSourcePhotoAlbumTrigger(RdbStore &store)
{
    MEDIA_INFO_LOG("start update source photo album trigger");
    const vector<string> sqls = {
        DROP_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER,
        CREATE_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER,
    };
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end update source photo album trigger");
    return ret;
}

static int32_t AddHighlightTriggerUpgradeWrapper(RdbStore &store)
{
    int32_t ret = AddHighlightTriggerColumn(store);
    ret = AddHighlightInsertAndUpdateTrigger(store);
    ret = AddHighlightIndex(store);
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_HIGHLIGHT_TRIGGER, "Album", AddHighlightTriggerUpgradeWrapper);

static int32_t AddDynamicRangeAndCloudEnhanceFixWrapper(RdbStore &store)
{
    int32_t ret = AddDynamicRangeColumnsFix(store);
    ret = AddCloudEnhanceColumnsFix(store);
    return NativeRdb::E_OK;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_HDR_AND_CLOUD_ENHANCEMENT_FIX, "Photos", AddDynamicRangeAndCloudEnhanceFixWrapper);

REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_SOURCE_PHOTO_ALBUM_TRIGGER, "Album", UpdateSourcePhotoAlbumTrigger);
REGISTER_SYNC_UPGRADE_TASK(VERSION_FIX_SOURCE_PHOTO_ALBUM_DATE_MODIFIED, "Album", UpdateSourcePhotoAlbumTrigger);
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_SOURCE_PHOTO_ALBUM_TRIGGER_AGAIN, "Album", UpdateSourcePhotoAlbumTrigger);

static int32_t UpdateSearchStatusTriggerForOwnerAlbumId(RdbStore &store)
{
    MEDIA_INFO_LOG("start update search status trigger for owner album id");
    const vector<string> sqls = {
        "DROP TRIGGER IF EXISTS " + UPDATE_SEARCH_STATUS_TRIGGER,
        CREATE_SEARCH_UPDATE_STATUS_TRIGGER,
    };
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end update search status trigger for owner album id");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_SEARCH_STATUS_TRIGGER_FOR_OWNER_ALBUM_ID,
    "Album", UpdateSearchStatusTriggerForOwnerAlbumId);

static int32_t UpdateSearchStatusTriggerForIsFavorite(RdbStore &store)
{
    MEDIA_INFO_LOG("start update search status trigger for is favorite");
    const vector<string> sqls = {
        "DROP TRIGGER IF EXISTS " + UPDATE_SEARCH_STATUS_TRIGGER,
        CREATE_SEARCH_UPDATE_STATUS_TRIGGER,
    };
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end update search status trigger for is favorite");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_SEARCH_STATUS_TRIGGER_FOR_IS_FAVORITE,
    "Photos", UpdateSearchStatusTriggerForIsFavorite);

static int32_t AddHighlightAnalysisProgress(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + USER_PHOTOGRAPHY_INFO_TABLE + " ADD COLUMN " + HIGHLIGHT_ANALYSIS_PROGRESS + " TEXT"
    };
    MEDIA_INFO_LOG("start add highlight_analysis_progress column");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_HIGHLIGHT_ANALYSIS_PROGRESS, "Vision", AddHighlightAnalysisProgress);

static int32_t AddRefreshAlbumStatusColumn(RdbStore &store)
{
    MEDIA_INFO_LOG("start add status column for refresh album table");
    const vector<string> sqls = {
        "ALTER TABLE " + ALBUM_REFRESH_TABLE + " ADD COLUMN " +
            ALBUM_REFRESH_STATUS + " INT DEFAULT 0 NOT NULL"
    };
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end add status column for refresh album table");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_REFRESH_ALBUM_STATUS_COLUMN, "Album", AddRefreshAlbumStatusColumn);

static int32_t UpdateCloudTrigger(RdbStore &store)
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
    return ExecSqls(sqls, store);
}

static int32_t AddSupportedWatermarkType(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::SUPPORTED_WATERMARK_TYPE + " INT "
    };
    MEDIA_INFO_LOG("start add supported_watermark_type column");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_SUPPORTED_WATERMARK_TYPE, "Photos", AddSupportedWatermarkType);

static int32_t AddStageVideoTaskStatus(RdbStore &store)
{
    MEDIA_INFO_LOG("start add stage_video_task_status column");
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::STAGE_VIDEO_TASK_STATUS + " INT NOT NULL DEFAULT 0 "
    };
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end add stage_video_task_status column");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_STAGE_VIDEO_TASK_STATUS, "Photos", AddStageVideoTaskStatus);

static int32_t AddHighlightUseSubtitle(RdbStore &store)
{
    MEDIA_INFO_LOG("start add use_subtitle column");
    const vector<string> sqls = {
        "ALTER TABLE " + HIGHLIGHT_ALBUM_TABLE + " ADD COLUMN " +
            HIGHLIGHT_USE_SUBTITLE + " INT DEFAULT 0"
    };
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("start add use_subtitle column");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_HIGHLIGHT_SUBTITLE, "Album", AddHighlightUseSubtitle);

static int32_t UpdateVisionTriggerForVideoLabel(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        DROP_UPDATE_VISION_TRIGGER,
        CREATE_VISION_UPDATE_TRIGGER_FOR_ADD_VIDEO_LABEL,
    };
    MEDIA_INFO_LOG("start update vision trigger for video label");
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_VISION_TRIGGER_FOR_VIDEO_LABEL, "Vision", UpdateVisionTriggerForVideoLabel);

static int32_t UpdateIndexForAlbumQuery(RdbStore &store)
{
    const vector<string> sqls = {
        PhotoUpgrade::DROP_INDEX_SCTHP_ADDTIME,
        PhotoUpgrade::INDEX_SCTHP_ADDTIME,
        PhotoUpgrade::DROP_SCHPT_MEDIA_TYPE_INDEX,
        PhotoUpgrade::CREATE_SCHPT_MEDIA_TYPE_INDEX,
    };
    MEDIA_INFO_LOG("start updating photo index");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_PHOTO_INDEX_FOR_ALBUM_COUNT_COVER, "Photos", UpdateIndexForAlbumQuery);

static int32_t UpdateVideoLabelTableForSubLabelType(RdbStore &store)
{
    const vector<string> sqls = {
        "DROP TABLE IF EXISTS " + VISION_VIDEO_LABEL_TABLE,
        CREATE_TAB_ANALYSIS_VIDEO_LABEL,
        UPDATE_VIDEO_LABEL_TOTAL_VALUE,
        UPDATE_SEARCH_INDEX_FOR_VIDEO,
    };
    MEDIA_INFO_LOG("start update video label table for sub_label_type");
    return ExecSqls(sqls, store);
}

static int32_t UpdateDataAddedIndexWithFileId(RdbStore &store)
{
    const vector<string> sqls = {
        PhotoUpgrade::DROP_INDEX_SCTHP_ADDTIME,
        PhotoUpgrade::INDEX_SCTHP_ADDTIME,
    };
    MEDIA_INFO_LOG("start update index of date added with file desc");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VISION_UPDATE_DATA_ADDED_INDEX, "Photos", UpdateDataAddedIndexWithFileId);

static int32_t UpdateMultiCropInfo(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        "ALTER TABLE " + VISION_RECOMMENDATION_TABLE + " ADD COLUMN " + MOVEMENT_CROP + " TEXT",
        "ALTER TABLE " + VISION_RECOMMENDATION_TABLE + " ADD COLUMN " + MOVEMENT_VERSION + " TEXT",
    };
    MEDIA_INFO_LOG("start update multi crop triggers");
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VISION_UPDATE_MULTI_CROP_INFO, "Photos", UpdateMultiCropInfo);

static int32_t UpdateSearchIndexTrigger(RdbStore &store)
{
    const vector<string> sqls = {
        "DROP TRIGGER IF EXISTS update_search_status_trigger",
        CREATE_SEARCH_UPDATE_STATUS_TRIGGER,
        "DROP TRIGGER IF EXISTS album_map_insert_search_trigger",
        "DROP TRIGGER IF EXISTS album_map_delete_search_trigger",
    };
    MEDIA_INFO_LOG("start update search index");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VISION_UPDATE_SEARCH_INDEX_TRIGGER, "Photos", UpdateSearchIndexTrigger);

static int32_t UpdatePhotosSearchUpdateTrigger(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        "DROP TRIGGER IF EXISTS update_search_status_trigger",
        CREATE_SEARCH_UPDATE_STATUS_TRIGGER,
    };
    MEDIA_INFO_LOG("Start update photos search trigger");
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_SEARCH_INDEX, "Photos", UpdatePhotosSearchUpdateTrigger);

static int32_t AddIsTemp(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_IS_TEMP + " INT DEFAULT 0"
    };
    MEDIA_INFO_LOG("Start add is_temp on Photos in upgrade");
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_IS_TEMP, "Photos", AddIsTemp);

static int32_t AddIsTempToTrigger(RdbStore &store)
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
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_IS_TEMP_TO_TRIGGER, "Photos", AddIsTempToTrigger);

static int32_t AddDisplayNameIndex(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        PhotoUpgrade::CREATE_PHOTO_DISPLAYNAME_INDEX,
    };
    MEDIA_INFO_LOG("Add displayname index");
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(PHOTOS_CREATE_DISPLAYNAME_INDEX, "Photos", AddDisplayNameIndex);

static int32_t UpdateSourceAlbumAndAlbumBundlenameTriggers(RdbStore &store)
{
    const vector<string> executeSqlStrs = {
        DROP_INSERT_PHOTO_UPDATE_ALBUM_BUNDLENAME,
        INSERT_PHOTO_UPDATE_ALBUM_BUNDLENAME,
    };
    MEDIA_INFO_LOG("start update source album and album bundlename triggers");
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_SOURCE_ALBUM_AND_ALBUM_BUNDLENAME_TRIGGERS,
    "Album", UpdateSourceAlbumAndAlbumBundlenameTriggers);

static int32_t AddFrontCameraType(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_FRONT_CAMERA + " TEXT"
    };
    MEDIA_INFO_LOG("Start add front column");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_FRONT_CAMERA_TYPE, "Photos", AddFrontCameraType);

static int32_t AddPortraitCoverSelectionColumn(RdbStore &store)
{
    MEDIA_INFO_LOG("Start add portrait cover selection column");

    const vector<string> sqls = {
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + BEAUTY_BOUNDER_X + " REAL",
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + BEAUTY_BOUNDER_Y + " REAL",
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + BEAUTY_BOUNDER_WIDTH + " REAL",
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + BEAUTY_BOUNDER_HEIGHT + " REAL",
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + FACE_AESTHETICS_SCORE + " REAL",
    };
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_PORTRAIT_COVER_SELECTION_ADD_COLUMNS, "Album", AddPortraitCoverSelectionColumn);

static void AddBestFaceBoundingColumnForGroupAlbum(RdbStore &store, int32_t version)
{
    const vector<string> sqls = {
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + JOINT_BEAUTY_BOUNDER_X + " REAL",
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + JOINT_BEAUTY_BOUNDER_Y + " REAL",
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + JOINT_BEAUTY_BOUNDER_WIDTH + " REAL",
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + JOINT_BEAUTY_BOUNDER_HEIGHT + " REAL",
    };
    MEDIA_INFO_LOG("Add best face bounding column for group album start");
    ExecSqlsWithDfx(sqls, store, version);
}

static void AddGroupVersion(RdbStore &store, int32_t version)
{
    const vector<string> sqls = {
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + GROUP_VERSION + " TEXT ",
    };
    MEDIA_INFO_LOG("Add group_version column start");
    ExecSqlsWithDfx(sqls, store, version);
}

static int32_t AddBestFaceBoundingAndGroupVersion(RdbStore &store)
{
    MEDIA_INFO_LOG("Start add best face bounding and group version");
    AddBestFaceBoundingColumnForGroupAlbum(store, VERSION_ADD_BEST_FACE_BOUNDING);
    AddGroupVersion(store, VERSION_ADD_BEST_FACE_BOUNDING);
    MEDIA_INFO_LOG("End add best face bounding and group version");
    return NativeRdb::E_OK;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_BEST_FACE_BOUNDING, "Vision",
    AddBestFaceBoundingAndGroupVersion);

static int32_t AddSouthDeviceType(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE +
            " INT NOT NULL DEFAULT 0",
    };
    MEDIA_INFO_LOG("Add south_device_type column start");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_SOUTH_DEVICE_TYPE);
    MEDIA_INFO_LOG("Add south_device_type column end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_SOUTH_DEVICE_TYPE, "Photos", AddSouthDeviceType);

static int32_t AddCreateTmpCompatibleDup(RdbStore &store)
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
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_CREATE_TMP_COMPATIBLE_DUP);
    MEDIA_INFO_LOG("End add create_tmp_compatible_dup columns");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_CREATE_TMP_COMPATIBLE_DUP, "Photos", AddCreateTmpCompatibleDup);

static int32_t AddUploadStatus(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " + PhotoAlbumColumns::UPLOAD_STATUS +
            " INT NOT NULL DEFAULT 0",
        "UPDATE PhotoAlbum SET upload_status = 1 WHERE album_type IN (0, 2048)",
    };
    MEDIA_INFO_LOG("Add upload status column start");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_PHOTO_ALBUM_ADD_UPLOAD_STATUS);
    MEDIA_INFO_LOG("Add upload status column end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_PHOTO_ALBUM_ADD_UPLOAD_STATUS, "Album", AddUploadStatus);

static int32_t CreateTabOldAlbum(RdbStore &store)
{
    MEDIA_INFO_LOG("Start VERSION_CREATE_TAB_OLD_ALBUM");
    int32_t ret = TabOldAlbumTableEventHandler().OnCreate(store);
    MEDIA_INFO_LOG("End VERSION_CREATE_TAB_OLD_ALBUM");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_CREATE_TAB_OLD_ALBUM, "Album", CreateTabOldAlbum);

static int32_t AddAffective(RdbStore &store)
{
    MEDIA_INFO_LOG("Start add affective");
    const vector<string> sqls = {
        "ALTER TABLE " + VISION_TOTAL_TABLE + " ADD COLUMN " + AFFECTIVE  + " INT NOT NULL DEFAULT 0 ",
        CREATE_TAB_ANALYSIS_AFFECTIVE,
    };
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_AFFECTIVE_TABLE);
    MEDIA_INFO_LOG("End add affective");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_AFFECTIVE_TABLE, "Vision", AddAffective);

static int32_t AddVideoMode(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_VIDEO_MODE +
        " INT NOT NULL DEFAULT -1",
    };
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_VIDEO_MODE);
    MEDIA_INFO_LOG("Add VideoMode column end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_VIDEO_MODE, "Photos", AddVideoMode);

static int32_t AddDetailTimeToPhotos(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_DETAIL_TIME + " TEXT"
    };
    MEDIA_INFO_LOG("Add detail_time column start");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_DETAIL_TIME, "Photos", AddDetailTimeToPhotos);

static int32_t AddThumbnailVisible(RdbStore& store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_THUMBNAIL_VISIBLE +
        " INT DEFAULT 0"
    };
    MEDIA_INFO_LOG("Add video face table start");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_THUMBNAIL_VISIBLE, "Photos", AddThumbnailVisible);

static int32_t AddVideoFaceTable(RdbStore &store)
{
    const vector<string> sqls = {
        CREATE_TAB_VIDEO_FACE,
        CREATE_VISION_INSERT_TRIGGER,
        CREATE_VISION_DELETE_TRIGGER,
        "ALTER TABLE " + VISION_TOTAL_TABLE + " ADD COLUMN " + GEO + " INT"
    };
    MEDIA_INFO_LOG("Add video face table start");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_VIDEO_FACE_TABLE, "Vision", AddVideoFaceTable);

static int32_t AlterThumbnailVisible(RdbStore& store)
{
    const vector<string> sqls = {
        PhotoUpgrade::DROP_INDEX_SCHPT_READY,
        PhotoUpgrade::INDEX_SCHPT_READY,
    };
    MEDIA_INFO_LOG("Add AlterThumbnailVisible");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ALTER_THUMBNAIL_VISIBLE, "Photos", AlterThumbnailVisible);

static int32_t AddOriginalSubtype(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_ORIGINAL_SUBTYPE + " INT"
    };
    MEDIA_INFO_LOG("start add original_subtype column");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VISION_ADD_ORIGINAL_SUBTYPE, "Photos", AddOriginalSubtype);

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

static int32_t UpdateDataUniqueIndex(RdbStore &store)
{
    MEDIA_INFO_LOG("Start UpdateDataUniqueIndex");
    string sql = PhotoUpgrade::UPDATA_PHOTOS_DATA_UNIQUE;
    auto err = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() { return store.ExecuteSql(sql); });
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to exec: %{public}s", sql.c_str());
        ReportFailInfo();
    }
    MEDIA_INFO_LOG("End UpdateDataUniqueIndex");
    return err;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UDAPTE_DATA_UNIQUE, "Photos", UpdateDataUniqueIndex);

static int32_t FixPhotoSchptMediaTypeIndex(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        PhotoUpgrade::CREATE_SCHPT_MEDIA_TYPE_INDEX,
    };
    MEDIA_INFO_LOG("Fix idx_schpt_media_type index");
    int32_t ret = ExecSqls(executeSqlStrs, store);
    MEDIA_INFO_LOG("End fix idx_schpt_media_type index.");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_FIX_PHOTO_SCHPT_MEDIA_TYPE_INDEX, "Photos", FixPhotoSchptMediaTypeIndex);

static int32_t AddAnalysisAlbumTotalTable(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        CREATE_TAB_ANALYSIS_ALBUM_TOTAL,
        INIT_TAB_ANALYSIS_ALBUM_TOTAL,
        CREATE_TOTAL_INSERT_TRIGGER_FOR_ADD_ANALYSIS_ALBUM_TOTAL,
        CREATE_VISION_UPDATE_TRIGGER_FOR_UPDATE_ANALYSIS_ALBUM_TOTAL_STATUS,
    };
    MEDIA_INFO_LOG("Start add analysis album total table");
    return ExecSqls(executeSqlStrs, store);
}

static int32_t CompatLivePhoto(RdbStore &store)
{
    MEDIA_INFO_LOG("Start configuring param for live photo compatibility");
    bool ret = false;
    ret = system::SetParameter(REFRESH_CLOUD_LIVE_PHOTO_FLAG, CLOUD_LIVE_PHOTO_NOT_REFRESHED);
    MEDIA_INFO_LOG("Set parameter for refreshing cloud live photo, ret: %{public}d", ret);

    ret = system::SetParameter(COMPAT_LIVE_PHOTO_FILE_ID, "1");
    MEDIA_INFO_LOG("Set parameter for compating local live photo, ret: %{public}d", ret);
    return ret ? NativeRdb::E_OK : E_FAIL;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_COMPAT_LIVE_PHOTO, "Photos", CompatLivePhoto);

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

static int32_t AddOwnerAlbumIdAndRefractorTrigger(RdbStore &store)
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
        DROP_INSERT_PHOTO_INSERT_SOURCE_ALBUM,
        DROP_INSERT_PHOTO_UPDATE_SOURCE_ALBUM,
        DROP_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER,
        DROP_INSERT_SOURCE_PHOTO_UPDATE_ALBUM_ID_TRIGGER,
        "DROP TRIGGER IF EXISTS photos_mdirty_trigger",
        PhotoUpgrade::CREATE_PHOTOS_MDIRTY_TRIGGER,
        CREATE_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER,
        CREATE_INSERT_SOURCE_UPDATE_ALBUM_ID_TRIGGER,

    };
    MEDIA_INFO_LOG("Add owner_album_id column for Photos");
    return ExecSqls(sqls, store);
}

static int32_t AddMergeInfoColumnForAlbum(RdbStore &store)
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
    int32_t ret = ExecSqls(addMergeInfoSql, store);
    const std::string queryHiddenAlbumId =
        "SELECT album_id FROM PhotoAlbum WHERE album_name = '.hiddenAlbum'";
    auto resultSet = store.QuerySql(queryHiddenAlbumId);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        int32_t err = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
            return store.ExecuteSql(CREATE_HIDDEN_ALBUM_FOR_DUAL_ASSET);
        });
        CHECK_AND_PRINT_LOG(err == NativeRdb::E_OK,
            "Failed to exec: %{private}s", CREATE_HIDDEN_ALBUM_FOR_DUAL_ASSET.c_str());
        return err;
    }
    return ret;
}

static int32_t AddOwnerAlbumIdUpgradeWrapper(RdbStore &store)
{
    int32_t ret = AddOwnerAlbumIdAndRefractorTrigger(store);
    if (ret != NativeRdb::E_OK) {
        return ret;
    }
    AlbumPluginTableEventHandler albumPluginTableEventHandler;
    ret = albumPluginTableEventHandler.OnUpgrade(store, VERSION_ADD_OWNER_ALBUM_ID, VERSION_ADD_OWNER_ALBUM_ID);
    if (ret != NativeRdb::E_OK) {
        return ret;
    }
    ret = AddMergeInfoColumnForAlbum(store);
    if (ret != NativeRdb::E_OK) {
        return ret;
    }
    MEDIA_INFO_LOG("ALBUM_FUSE: set album fuse upgrade status");
    MediaLibraryAlbumFusionUtils::SetAlbumFuseUpgradeStatus(0);
    return NativeRdb::E_OK;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_OWNER_ALBUM_ID, "Album", AddOwnerAlbumIdUpgradeWrapper);

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

static int32_t AddHighlightMapTable(RdbStore &store)
{
    const vector<string> executeSqlStrs = {
        CREATE_ANALYSIS_ASSET_SD_MAP_TABLE,
        CREATE_ANALYSIS_ALBUM_ASET_MAP_TABLE,
        "ALTER TABLE " + HIGHLIGHT_PLAY_INFO_TABLE + " ADD COLUMN " + HIGHLIGHTING_ALGO_VERSION + " TEXT",
        "ALTER TABLE " + HIGHLIGHT_PLAY_INFO_TABLE + " ADD COLUMN " + CAMERA_MOVEMENT_ALGO_VERSION + " TEXT",
        "ALTER TABLE " + HIGHLIGHT_PLAY_INFO_TABLE + " ADD COLUMN " + TRANSITION_ALGO_VERSION + " TEXT",
    };
    MEDIA_INFO_LOG("add analysis map table of highlight db");
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_HIGHLIGHT_MAP_TABLES, "Album", AddHighlightMapTable);

static int32_t AddPetTables(RdbStore &store)
{
    const vector<string> executeSqlStrs = {
        CREATE_TAB_ANALYSIS_PET_FACE,
        CREATE_TAB_ANALYSIS_PET_TAG,
        ADD_PET_STATUS_COLUMN,
    };
    MEDIA_INFO_LOG("add pet tables start");
    int32_t ret = ExecSqlsWithDfx(executeSqlStrs, store, VERSION_ADD_PET_TABLES);
    MEDIA_INFO_LOG("add pet tables end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_PET_TABLES, "Vision", AddPetTables);

static int32_t UpdatePortraitCoverSelectionColumns(RdbStore &store)
{
    MEDIA_INFO_LOG("Start update portrait cover selection columns");

    const vector<string> sqls = {
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + BEAUTY_BOUNDER_VERSION + " TEXT default '' ",
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + IS_EXCLUDED + " INT default 0 ",
    };
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_PORTRAIT_COVER_SELECTION_COLUMNS, "Vision",
    UpdatePortraitCoverSelectionColumns);

static int32_t AddAppUriPermissionInfo(RdbStore &store)
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
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_APP_URI_PERMISSION_INFO, "OtherTable",
    AddAppUriPermissionInfo);

static int32_t AddCoverPosition(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_COVER_POSITION +
            " BIGINT DEFAULT 0",
    };
    MEDIA_INFO_LOG("start add cover_position column");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_COVER_POSITION, "Photos", AddCoverPosition);

static int32_t AddSchptReadyIndex(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        PhotoUpgrade::INDEX_SCHPT_READY,
    };
    MEDIA_INFO_LOG("Add schpt ready index");
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_SCHPT_READY_INEDX, "Photos", AddSchptReadyIndex);

static int32_t AddOCRCardColumns(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + VISION_OCR_TABLE + " ADD COLUMN " + OCR_CARD_TEXT + " TEXT",
        "ALTER TABLE " + VISION_OCR_TABLE + " ADD COLUMN " + OCR_CARD_TEXT_MSG + " TEXT",
    };
    MEDIA_INFO_LOG("Add video face table start");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_OCR_CARD_COLUMNS, "Vision", AddOCRCardColumns);

static int32_t DropPhotoAlbumClearMap(RdbStore& store)
{
    const vector<string> sqls = {
        DROP_PHOTO_ALBUM_CLEAR_MAP_SQL,
        DROP_INSERT_PHOTO_UPDATE_ALBUM_BUNDLENAME,
        INSERT_PHOTO_UPDATE_ALBUM_BUNDLENAME,
    };
    MEDIA_INFO_LOG("Drop photoAlbum clear map start");
    return ExecSqls(sqls, store);
}

static int32_t AddHighlightVideoCountCanPack(RdbStore& store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + HIGHLIGHT_ALBUM_TABLE + " ADD COLUMN " + HIGHLIGHT_VIDEO_COUNT_CAN_PACK + " INT",
    };
    MEDIA_INFO_LOG("Add key: hilghlight video count can pack Start");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_HIGHLIGHT_VIDEO_COUNT_CAN_PACK, "Vision",
    AddHighlightVideoCountCanPack);

static int32_t AddVisitCountColumn(RdbStore &store)
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
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("add real_lcd_visit_time/visit_count/lcd_visit_count column end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_VISIT_COUNT, "Photos", AddVisitCountColumn);

static int32_t AddIsRecentShow(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_IS_RECENT_SHOW  +
            " INT NOT NULL DEFAULT 1",
    };
    MEDIA_INFO_LOG("add is_recent_show column start");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_MEDIA_IS_RECENT_SHOW_COLUMN, "Photos", AddIsRecentShow);

static int32_t AddFrontAnalysisColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + USER_PHOTOGRAPHY_INFO_TABLE + " ADD COLUMN " + FRONT_INDEX_LIMIT + " INT DEFAULT 0",
        "ALTER TABLE " + USER_PHOTOGRAPHY_INFO_TABLE + " ADD COLUMN " + FRONT_INDEX_MODIFIED + " BIGINT DEFAULT 0",
        "ALTER TABLE " + USER_PHOTOGRAPHY_INFO_TABLE + " ADD COLUMN " + FRONT_INDEX_COUNT + " INT DEFAULT 0",
        "ALTER TABLE " + USER_PHOTOGRAPHY_INFO_TABLE + " ADD COLUMN " + FRONT_CV_MODIFIED + " BIGINT DEFAULT 0",
        "ALTER TABLE " + USER_PHOTOGRAPHY_INFO_TABLE + " ADD COLUMN " + FRONT_CV_COUNT + " INT DEFAULT 0",
    };
    MEDIA_INFO_LOG("Add front analysis column start");
    return ExecSqls(sqls, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_FOREGROUND_ANALYSIS, "Vision", AddFrontAnalysisColumn);

static int32_t AddLcdFileModifyTimeColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoExtColumn::PHOTOS_EXT_TABLE + " ADD COLUMN " +
        PhotoExtColumn::LCD_FILE_MODIFY_TIME + " BIGINT NOT NULL DEFAULT 0",
    };
    MEDIA_INFO_LOG("Add lcd_file_modify_time column start");
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("Add lcd_file_modify_time column end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_LCD_AGING, "Photos", AddLcdFileModifyTimeColumn);

static int32_t AddTableAnalysisDedupSelection(RdbStore &store)
{
    MEDIA_INFO_LOG("AddTableAnalysisDedupSelection start");
    const vector<string> sqls = {
        "ALTER TABLE " + VISION_TOTAL_TABLE + " ADD COLUMN " + SIMILARITY + " INT NOT NULL DEFAULT 0 ",
        "ALTER TABLE " + VISION_TOTAL_TABLE + " ADD COLUMN " + DUPLICATE + " INT NOT NULL DEFAULT 0 ",
        "ALTER TABLE " + VISION_TOTAL_TABLE + " ADD COLUMN " + TOTAL_SCORE_STATUS + " INT NOT NULL DEFAULT 0 ",
        CREATE_TAB_ANALYSIS_DEDUP_SELECTION,
        CREATE_TAB_ANALYSIS_PROFILE,
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + EMOTION + " STRING ",
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + COMPLETENESS + " INT ",
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + SIMPLE_FACE_SCORE + " INT ",
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + SIMPLE_FACE_SCORE_VERSION + " TEXT ",
        "ALTER TABLE " + VISION_AFFECTIVE_TABLE + " ADD COLUMN " + AFFECTIVE_SCORE + " INT ",
        "ALTER TABLE " + VISION_AFFECTIVE_TABLE + " ADD COLUMN " + AFFECTIVE_SCORE_VERSION + " TEXT ",
        "ALTER TABLE " + VISION_LABEL_TABLE + " ADD COLUMN " + SIGNIFICANCE_SCORE + " INT ",
        "ALTER TABLE " + VISION_LABEL_TABLE + " ADD COLUMN " + SIGNIFICANCE_SCORE_VERSION + " TEXT ",
    };
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_TAB_ANALYSIS_DEDUP_SELECTION);
    MEDIA_INFO_LOG("AddTableAnalysisDedupSelection end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_TAB_ANALYSIS_DEDUP_SELECTION, "Vision", AddTableAnalysisDedupSelection);

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

static int32_t AddAssetAlbumOperationTable(RdbStore &store)
{
    const vector<string> executeSqlStrs = {
        "DROP TABLE IF EXISTS tab_asset_and_album_operation",
        SQL_CREATE_TAB_ASSET_ALBUM_OPERATION,
        "DROP TABLE IF EXISTS operation_asset_insert_trigger",
        SQL_CREATE_OPERATION_ASSET_INSERT_TRIGGER,
        "DROP TABLE IF EXISTS operation_asset_delete_trigger",
        SQL_CREATE_OPERATION_ASSET_DELETE_TRIGGER,
        "DROP TABLE IF EXISTS operation_asset_update_trigger",
        SQL_CREATE_OPERATION_ASSET_UPDATE_TRIGGER,
        "DROP TABLE IF EXISTS operation_album_insert_trigger",
        SQL_CREATE_OPERATION_ALBUM_INSERT_TRIGGER,
        "DROP TABLE IF EXISTS operation_album_delete_trigger",
        SQL_CREATE_OPERATION_ALBUM_DELETE_TRIGGER,
        "DROP TABLE IF EXISTS operation_album_update_trigger",
        SQL_CREATE_OPERATION_ALBUM_UPDATE_TRIGGER,
    };
    int32_t ret = ExecSqls(executeSqlStrs, store);
    MEDIA_INFO_LOG("create asset and album operation table end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_FILTER_TAB_ASSET_ALBUM_OPERATION, "Album", AddAssetAlbumOperationTable);


static int32_t AddAssetAlbumOperationTableForSync(RdbStore &store)
{
    const vector<string> executeSqlStrs = {
        SQL_CREATE_TAB_ASSET_ALBUM_OPERATION,
        "DROP TABLE IF EXISTS operation_asset_insert_trigger",
        SQL_CREATE_OPERATION_ASSET_INSERT_TRIGGER,
        "DROP TABLE IF EXISTS operation_asset_delete_trigger",
        SQL_CREATE_OPERATION_ASSET_DELETE_TRIGGER,
        "DROP TABLE IF EXISTS operation_asset_update_trigger",
        SQL_CREATE_OPERATION_ASSET_UPDATE_TRIGGER,
        "DROP TABLE IF EXISTS operation_album_insert_trigger",
        SQL_CREATE_OPERATION_ALBUM_INSERT_TRIGGER,
        "DROP TABLE IF EXISTS operation_album_delete_trigger",
        SQL_CREATE_OPERATION_ALBUM_DELETE_TRIGGER,
        "DROP TABLE IF EXISTS operation_album_update_trigger",
        SQL_CREATE_OPERATION_ALBUM_UPDATE_TRIGGER,
    };
    int32_t ret = ExecSqls(executeSqlStrs, store);
    MEDIA_INFO_LOG("create asset and album operation table sync end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_CREATE_TAB_ASSET_ALBUM_OPERATION_FOR_SYNC, "Album",
    AddAssetAlbumOperationTableForSync);


static int32_t UpgradeAnalysisUpdateSearchTrigger(RdbStore &store)
{
    MEDIA_INFO_LOG("start upgrade analysis update search trigger");
    const vector<string> sqls = {
        "DROP TRIGGER IF EXISTS " + ANALYSIS_UPDATE_SEARCH_TRIGGER,
        CREATE_ANALYSIS_UPDATE_SEARCH_TRIGGER,
    };
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("end upgrade analysis update search trigger");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPGRADE_ANALYSIS_UPDATE_SEARCH_TRIGGER, "Vision",
    UpgradeAnalysisUpdateSearchTrigger);

static int32_t AddAnalysisUpdateVideoSearchTrigger(RdbStore &store)
{
    MEDIA_INFO_LOG("Start add analysis update video search trigger");
    const vector<string> sqls = {
        "DROP TRIGGER IF EXISTS " + ANALYSIS_UPDATE_SEARCH_TRIGGER,
        CREATE_ANALYSIS_UPDATE_SEARCH_TRIGGER,
        CREATE_ANALYSIS_UPDATE_VIDEO_SEARCH_TRIGGER,
    };
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_ANALYSIS_UPDATE_SEARCH_TRIGGER);
    MEDIA_INFO_LOG("End add analysis update video search trigger");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_ANALYSIS_UPDATE_SEARCH_TRIGGER, "Vision",
    AddAnalysisUpdateVideoSearchTrigger);

static int32_t CreateTabCustomRecords(RdbStore &store)
{
    const vector<string> executeSqlStrs = {
        CustomRecordsColumns::CREATE_TABLE,
    };
    int32_t ret = ExecSqls(executeSqlStrs, store);
    MEDIA_INFO_LOG("create custom and records end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_CREATE_TAB_CUSTOM_RECORDS, "Album", CreateTabCustomRecords);

static int32_t AddExifRotateColumn(RdbStore& store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_EXIF_ROTATE +
        " INT NOT NULL DEFAULT 0",
    };
    MEDIA_INFO_LOG("Start add exif_rotate column");
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("End add exif_rotate column");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_EXIF_ROTATE_COLUMN_AND_SET_VALUE, "Photos", AddExifRotateColumn);

static int32_t DealWithAlbumMapTrigger(RdbStore &store)
{
    const vector<std::string> exeSqls = {
        SQL_DROP_NEW_TRIGGER,
        SQL_DROP_DELETE_TRIGGER,
        SQL_DROP_INSERT_SEARCH_TRIGGER,
        SQL_DROP_DELETE_SEARCH_TRIGGER,
    };
    MEDIA_INFO_LOG("DealWithAlbumMapTrigger start");
    int32_t ret = ExecSqls(exeSqls, store);
    MEDIA_INFO_LOG("DealWithAlbumMapTrigger end");
    return ret;
}

static int32_t AddUriSensitiveColumns(RdbStore &store)
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
    int32_t ret = ExecSqls(exeSqls, store);
    MEDIA_INFO_LOG("AddUriSensitiveColumns end");
    return ret;
}

static int32_t AddFileSourceType(RdbStore &store)
{
    const vector<string> sqls = {
        "DROP TRIGGER IF EXISTS " + INSERT_SEARCH_TRIGGER,
        CREATE_SEARCH_INSERT_TRIGGER,
        DROP_INSERT_VISION_TRIGGER,
        UPGRADE_VISION_INSERT_TRIGGER_FOR_FILE_SOURCE_TYPE,
        "DROP TRIGGER IF EXISTS operation_asset_insert_trigger",
        SQL_CREATE_OPERATION_ASSET_INSERT_TRIGGER,
        "DROP TRIGGER IF EXISTS operation_asset_delete_trigger",
        SQL_CREATE_OPERATION_ASSET_DELETE_TRIGGER,
        "DROP TRIGGER IF EXISTS operation_asset_update_trigger",
        SQL_CREATE_OPERATION_ASSET_UPDATE_TRIGGER,
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
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_FILE_SOURCE_TYPE);
    MEDIA_INFO_LOG("AddFileSourceType end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_FILE_SOURCE_TYPE, "Photos", AddFileSourceType);

static int32_t AddAspectRatio(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
        PhotoColumn::PHOTO_ASPECT_RATIO + " DOUBLE NOT NULL DEFAULT -2 ",
    };
    MEDIA_INFO_LOG("AddAspectRatio start");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_FILE_SOURCE_TYPE);
    MEDIA_INFO_LOG("AddAspectRatio end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_ASPECT_RATIO, "Photos", AddAspectRatio);

static int32_t AddTempFileAssetsCreateAlbum(RdbStore &store)
{
    const vector<string> sqls = {
        DROP_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER,
        CREATE_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER
    };
    MEDIA_INFO_LOG("AddTempFileAssetsCreateAlbum start");
    int32_t ret = ExecSqls(sqls, store);
    MEDIA_INFO_LOG("AddTempFileAssetsCreateAlbum end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_TEMP_FILE_ASSETS_CREATE_ALBUM, "Album", AddTempFileAssetsCreateAlbum);

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

static int32_t FixDbUpgradeToAPI20(RdbStore &store)
{
    MEDIA_INFO_LOG("Start fix db upgrade to API20");
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
    MEDIA_INFO_LOG("End fix db upgrade to API20");
    return NativeRdb::E_OK;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_FIX_DB_UPGRADE_TO_API20, "Photos", FixDbUpgradeToAPI20);

static int32_t AddAnalysisProgress(RdbStore &store)
{
    const vector<string> exeSqls = {
        CREATE_TAB_ANALYSIS_PROGRESS,
    };
    MEDIA_INFO_LOG("start add analysis progress table");
    int32_t ret = ExecSqlsWithDfx(exeSqls, store, VERSION_ADD_TAB_ANALYSIS_PROGRESS);
    MEDIA_INFO_LOG("end add analysis progress table");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_TAB_ANALYSIS_PROGRESS, "Vision", AddAnalysisProgress);

static int32_t AddCloneSequenceColumns(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + TabOldPhotosColumn::OLD_PHOTOS_TABLE + " ADD COLUMN " +
            TabOldPhotosColumn::MEDIA_CLONE_SEQUENCE + " INTEGER"
    };
    MEDIA_INFO_LOG("add tab_old_photos clone_sequence columns start");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_TAB_OLD_PHOTOS_CLONE_SEQUENCE);
    MEDIA_INFO_LOG("add tab_old_photos clone_sequence columns end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_TAB_OLD_PHOTOS_CLONE_SEQUENCE, "Vision", AddCloneSequenceColumns);

static int32_t AddAlbumOrderBackTable(RdbStore &store)
{
    const vector<string> sqls = { CREATE_ALBUM_ORDER_BACK_TABLE };
    MEDIA_INFO_LOG("create album_order_back table start");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_ALBUM_ORDER_BACK_VERSION);
    MEDIA_INFO_LOG("create album_order_back table end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_ALBUM_ORDER_BACK_VERSION, "Album", AddAlbumOrderBackTable);

static int32_t AddImageFaceDetail(RdbStore &store)
{
    MEDIA_INFO_LOG("start to add image face detail");
    const vector<string> sqls = {
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + FACE_EYE_CLOSE + " REAL",
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + FACE_DETAIL_VERSION + " TEXT",
    };
    return ExecSqlsWithDfx(sqls, store, VERSION_ADD_IMAGE_FACE_DETAIL);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_IMAGE_FACE_DETAIL, "Vision", AddImageFaceDetail);

static int32_t AddImageFaceAndFaceTagAgeGender(RdbStore &store)
{
    MEDIA_INFO_LOG("start to add age and gender for image face and face tag");
    const vector<string> sqls = {
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + AGE + " DOUBLE",
        "ALTER TABLE " + VISION_IMAGE_FACE_TABLE + " ADD COLUMN " + GENDER + " INTEGER",
        "ALTER TABLE " + VISION_FACE_TAG_TABLE + " ADD COLUMN " + AGE + " DOUBLE",
        "ALTER TABLE " + VISION_FACE_TAG_TABLE + " ADD COLUMN " + GENDER + " INTEGER",
    };
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_IMAGE_FACE_AND_FACE_TAG_AGE_GENDER);
    MEDIA_INFO_LOG("end to add age and gender for image face and face tag");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_IMAGE_FACE_AND_FACE_TAG_AGE_GENDER, "Vision", AddImageFaceAndFaceTagAgeGender);

static int32_t AddNetSelectedDownloadColumns(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + DownloadResourcesColumn::TABLE + " ADD COLUMN " +
            DownloadResourcesColumn::MEDIA_TASK_SEQ + " INT NOT NULL DEFAULT 0",
        "ALTER TABLE " + DownloadResourcesColumn::TABLE + " ADD COLUMN " +
            DownloadResourcesColumn::MEDIA_NETWORK_POLICY + " INT NOT NULL DEFAULT 0"
    };
    MEDIA_INFO_LOG("add download_resources_task_records network_policy columns start");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_NETWORK_SELECTED_IN_DRTR);
    MEDIA_INFO_LOG("add download_resources_task_records network_policy columns end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_NETWORK_SELECTED_IN_DRTR, "OtherTable", AddNetSelectedDownloadColumns);

static int32_t AddAnalysisProgressColumns(RdbStore &store)
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
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_TAB_ANALYSIS_PROGRESS_COLUMNS);
    MEDIA_INFO_LOG("end add analysis progress columns");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_TAB_ANALYSIS_PROGRESS_COLUMNS, "Vision", AddAnalysisProgressColumns);

static int32_t AddAnalysisProgressCheckSpaceColumn(RdbStore &store)
{
    const vector<string> sqls = {
        ADD_CHECK_SPACE_FLAG_COLUMN,
    };
    MEDIA_INFO_LOG("start add tab_analysis_progress check_space_flag column");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_TAB_ANALYSIS_PROGRESS_CHECK_SPACE_COLUMN);
    MEDIA_INFO_LOG("end add tab_analysis_progress check_space_flag column");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_TAB_ANALYSIS_PROGRESS_CHECK_SPACE_COLUMN, "Vision",
    AddAnalysisProgressCheckSpaceColumn);

static int32_t CreateVisionVideoTotal(RdbStore& store)
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
    return ExecSqlsWithDfx(sqls, store, VERSION_UPDATE_VIDEO_LABLE_FACE);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_VIDEO_LABLE_FACE, "Vision", CreateVisionVideoTotal);

static int32_t CreateChangeTime(RdbStore &store)
{
    MEDIA_INFO_LOG("start add change_time column");
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_CHANGE_TIME +
            " BIGINT NOT NULL DEFAULT 0",
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " + PhotoAlbumColumns::CHANGE_TIME +
            " BIGINT NOT NULL DEFAULT 0",
    };
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_CHANGE_TIME);
    MEDIA_INFO_LOG("end add change_time column");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_CHANGE_TIME, "Photos", CreateChangeTime);

static int32_t AddAudioIsTemp(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + AudioColumn::AUDIOS_TABLE + " ADD COLUMN " +
            AudioColumn::AUDIO_IS_TEMP + " INT DEFAULT 0"
    };
    MEDIA_INFO_LOG("AddAudioIsTemp start");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_AUDIO_IS_TEMP);
    MEDIA_INFO_LOG("AddAudioIsTemp end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_AUDIO_IS_TEMP, "OtherTable", AddAudioIsTemp);

static int32_t CreateBatchDownloadRecords(RdbStore &store)
{
    MEDIA_INFO_LOG("create batchdownload records begin");
    const vector<string> executeSqlStrs = {
        DownloadResourcesColumn::CREATE_TABLE,
        DownloadResourcesColumn::INDEX_DRTR_ID_STATUS,
    };
    int32_t ret = ExecSqlsWithDfx(executeSqlStrs, store, VERSION_ADD_BATCH_DOWNLOAD);
    MEDIA_INFO_LOG("create batchdownload records end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_BATCH_DOWNLOAD, "OtherTable", CreateBatchDownloadRecords);

static int32_t UpdateMdirtyTriggerForStrongAssociation(RdbStore &store)
{
    const vector<string> sqls = {
        "DROP TRIGGER IF EXISTS photos_mdirty_trigger",
        PhotoUpgrade::CREATE_PHOTOS_MDIRTY_TRIGGER,
    };
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_UPDATE_MDIRTY_TRIGGER_FOR_STRONG_ASSOCIATION);
    MEDIA_INFO_LOG("Update mdirty trigger for strong association end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_MDIRTY_TRIGGER_FOR_STRONG_ASSOCIATION,
    "Photos", UpdateMdirtyTriggerForStrongAssociation);

static int32_t UpdateSourceAlbumBundleNameTriggerUseLpath(RdbStore &store)
{
    const vector<string> sqls = {
        DROP_INSERT_PHOTO_UPDATE_ALBUM_BUNDLENAME,
        INSERT_PHOTO_UPDATE_ALBUM_BUNDLENAME,
    };
    MEDIA_INFO_LOG("start update source album bundle name trigger use lpath");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_SOURCE_ALBUM_BUNDLE_UPDATE_TRIGGER_USE_LPATH);
    MEDIA_INFO_LOG("end update source album bundle name trigger use lpath");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_SOURCE_ALBUM_BUNDLE_UPDATE_TRIGGER_USE_LPATH,
    "Photos", UpdateSourceAlbumBundleNameTriggerUseLpath);

static int32_t AddEditOperation(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        "ALTER TABLE " + ANALYSIS_ALBUM_TABLE + " ADD COLUMN " + EDIT_OPERATION + " INT ",
    };
    MEDIA_INFO_LOG("add edit operation column start");
    int32_t ret = ExecSqlsWithDfx(executeSqlStrs, store, VERSION_ADD_EDIT_OPERATION);
    MEDIA_INFO_LOG("start add edit operation column end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_EDIT_OPERATION, "Vision", AddEditOperation);

static int32_t AddPhotoAlbumHidden(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
            PhotoAlbumColumns::ALBUM_HIDDEN + " INT NOT NULL DEFAULT 0",
    };
    MEDIA_INFO_LOG("Add photoalbum hidden column start");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_PHOTO_ALBUM_HIDDEN);
    MEDIA_INFO_LOG("Add photoalbum hidden column end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_PHOTO_ALBUM_HIDDEN, "Album", AddPhotoAlbumHidden);

static int32_t AddDateAddedYearMonthDay(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_DATE_ADDED_YEAR + " TEXT ",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_DATE_ADDED_MONTH + " TEXT ",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_DATE_ADDED_DAY + " TEXT ",
    };
    MEDIA_INFO_LOG("Add date_added year month day columns start");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_DATE_ADDED_YEAR_MONTH_DAY);
    MEDIA_INFO_LOG("Add date_added year month day columns end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_DATE_ADDED_YEAR_MONTH_DAY, "Photos", AddDateAddedYearMonthDay);

static int32_t AddPersonScoreAndHighlightFlush(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + HIGHLIGHT_ALBUM_TABLE + " ADD COLUMN " + HIGHLIGHT_FLUSH + " INT NOT NULL DEFAULT 0 ",
        "ALTER TABLE " + VISION_PROFILE + " ADD COLUMN " + PERSONALIZATION_SCORE + " INT ",
        "ALTER TABLE " + VISION_PROFILE + " ADD COLUMN " + PERSONALIZATION_SCORE_VERSION + " TEXT ",
    };
    MEDIA_INFO_LOG("Add personalization_score and highlight_flush columns start");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_PERSON_SCORE_AND_HIGHLIGHT_FLUSH);
    MEDIA_INFO_LOG("Add personalization_score and highlight_flush columns end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_PERSON_SCORE_AND_HIGHLIGHT_FLUSH, "Vision", AddPersonScoreAndHighlightFlush);

static int32_t AddCinematicVideoAlbum(RdbStore &store)
{
    MEDIA_INFO_LOG("Start add cinematic video album");
    int32_t err = MediaLibraryRdbHelper::PrepareShootingModeAlbum(store);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Prepare cinematic video album failed, ret: %{public}d", err);
        RdbUpgradeUtils::AddUpgradeDfxMessages(VERSION_ADD_CINEMATIC_VIDEO_ALBUM, 0, err);
    }
    MEDIA_INFO_LOG("End add cinematic video album");
    return err;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_CINEMATIC_VIDEO_ALBUM, "Album", AddCinematicVideoAlbum);

static int32_t UpdateTriggerForAnalysisAlbum(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        "ALTER TABLE " + SEARCH_TOTAL_TABLE + " ADD COLUMN " + TBL_SEARCH_FACE_STATUS + " INT DEFAULT 0 ",
        "ALTER TABLE " + SEARCH_TOTAL_TABLE + " ADD COLUMN " + TBL_SEARCH_ALBUM_STATUS + " INT DEFAULT 0 ",
        "DROP TRIGGER IF EXISTS " + ANALYSIS_ALBUM_UPDATE_SEARCH_TRIGGER,
        CREATE_ANALYSIS_ALBUM_UPDATE_SEARCH_TRIGGER,
    };
    MEDIA_INFO_LOG("Start update album modify trigger");
    int32_t ret = ExecSqlsWithDfx(executeSqlStrs, store, VERSION_UPDATE_TRIGGER_FOR_ANALYSIS_ALBUM);
    MEDIA_INFO_LOG("End update album modify trigger");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_TRIGGER_FOR_ANALYSIS_ALBUM, "Vision", UpdateTriggerForAnalysisAlbum);

static int32_t CreateTabComPatibleInfo(RdbStore &store)
{
    MEDIA_INFO_LOG("create tab_compatible_info starts");
    const vector<string> sqls = {
        TabCompatibleInfoColumn::CREATE_TABLE
    };
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_CREATE_TAB_COMPATIBLE_INFO);
    MEDIA_INFO_LOG("create tab_compatible_info ends");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_CREATE_TAB_COMPATIBLE_INFO, "OtherTable", CreateTabComPatibleInfo);

static int32_t Add4DLivePhotoStatusAndLatestPair(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_STATUS + " INT NOT NULL DEFAULT 0 ",
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_LATEST_PAIR + " TEXT ",
    };
    MEDIA_INFO_LOG("Add live_Photo_4d_status and  livePhoto_4d_latest_pair columns start");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_LIVEPHOTO_4D_COLUMN_ON_PHOTOS);
    MEDIA_INFO_LOG("Add live_Photo_4d_status and  livePhoto_4d_latest_pair columns end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_LIVEPHOTO_4D_COLUMN_ON_PHOTOS, "Photos", Add4DLivePhotoStatusAndLatestPair);

static int32_t AddAnalysisAlbumUpdateAlbumStatusTrigger(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        "DROP TRIGGER IF EXISTS " + ANALYSIS_ALBUM_UPDATE_SEARCH_TRIGGER,
        CREATE_ANALYSIS_ALBUM_UPDATE_SEARCH_TRIGGER,
        CREATE_ANALYSIS_ALBUM_UPDATE_ALBUM_STATUS_TRIGGER,
    };
    MEDIA_INFO_LOG("Start update album modify trigger");
    int32_t ret = ExecSqlsWithDfx(executeSqlStrs, store, VERSION_ADD_ANALYSIS_ALBUM_UPDATE_ALBUM_STATUS_TRIGGER);
    MEDIA_INFO_LOG("End update album modify trigger");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_ANALYSIS_ALBUM_UPDATE_ALBUM_STATUS_TRIGGER, "Vision",
    AddAnalysisAlbumUpdateAlbumStatusTrigger);

static int32_t AddHighlightGrowingTime(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + HIGHLIGHT_ALBUM_TABLE + " ADD COLUMN " + GROWING_TIME + " TEXT ",
    };
    MEDIA_INFO_LOG("Add tab_highlight_album growing_time columns start");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_HIGHLIGHT_GROWING_TIME);
    MEDIA_INFO_LOG("Add tab_highlight_album growing_time columns end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_HIGHLIGHT_GROWING_TIME, "Vision", AddHighlightGrowingTime);

static int32_t AddPreferredCompatibleMode(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + TabCompatibleInfoColumn::TABLE + " ADD COLUMN " +
            TabCompatibleInfoColumn::PREFERRED_COMPATIBLE_MODE + " INT NOT NULL DEFAULT 0 ",
    };
    MEDIA_INFO_LOG("Add tab_compatible_info preferred_compatible_mode column start");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_PREFERRED_COMPATIBLE_MODE);
    MEDIA_INFO_LOG("Add tab_compatible_info preferred_compatible_mode column end");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_PREFERRED_COMPATIBLE_MODE, "OtherTable", AddPreferredCompatibleMode);

static int32_t AddUniqueIdColumnsToAlbums(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
            PhotoAlbumColumns::UNIQUE_ID + " TEXT DEFAULT NULL",
    };
 
    MEDIA_INFO_LOG("add PhotoAlbum unique_id columns starts");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_UNIQUE_ID_COLUMN_ON_PHOTO_ALBUM);
    MEDIA_INFO_LOG("add PhotoAlbum unique_id columns ends");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_UNIQUE_ID_COLUMN_ON_PHOTO_ALBUM, "Album", AddUniqueIdColumnsToAlbums);

static int32_t UpdateUniqueIdColumnOfPhotoAlbums(RdbStore &store)
{
    const vector<string> sqls = {
        DROP_INSERT_PHOTO_INSERT_SOURCE_ALBUM,
        INSERT_PHOTO_INSERT_SOURCE_ALBUM,
        DROP_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER,
        CREATE_INSERT_SOURCE_PHOTO_CREATE_SOURCE_ALBUM_TRIGGER,
    };

    MEDIA_INFO_LOG("update unique_id column PhotoAlbum starts");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_UPDATE_UNIQUE_ID_COLUMN_PHOTO_ALBUM);
    MEDIA_INFO_LOG("update unique_id column PhotoAlbum ends");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_UPDATE_UNIQUE_ID_COLUMN_PHOTO_ALBUM, "Album", UpdateUniqueIdColumnOfPhotoAlbums);

static int32_t AddPersistPermissionTable(RdbStore &store)
{
    MEDIA_INFO_LOG("Start add Persist_Permission table");
    const vector<string> sqls = {
        PersistPermissionColumn::CREATE_PERSIST_PERMISSION_TABLE,
    };
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_PERSIST_PERMISSION_TABLE);
    MEDIA_INFO_LOG("End add Persist_Permission table");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_PERSIST_PERMISSION_TABLE, "OtherTable", AddPersistPermissionTable);

static int32_t AddLocalAssetSizeColumn(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::LOCAL_ASSET_SIZE + " BIGINT NOT NULL DEFAULT 0",
    };
    MEDIA_INFO_LOG("Add Photos local_asset_size columns starts");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_LOCAL_ASSET_SIZE_COLUMN);
    MEDIA_INFO_LOG("Add Photos local_asset_size columns ends");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_LOCAL_ASSET_SIZE_COLUMN, "Photos", AddLocalAssetSizeColumn);

static int32_t AddPortraitNicknameTable(RdbStore &store)
{
    const vector<string> sqls = {
        CREATE_ANALYSIS_NICK_NAME_TABLE,
        CREATE_ANALYSIS_NICK_NAME_UNIQUE_INDEX,
        "DROP TRIGGER IF EXISTS portrait_nickname_delete_trigger",
        CREATE_ANALYSIS_NICK_NAME_DELETE_TRIGGER,
    };
    MEDIA_INFO_LOG("add portrait nickname table starts");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_PORTRAIT_NICKNAME_TABLE);
    MEDIA_INFO_LOG("add portrait nickname table ends");
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_PORTRAIT_NICKNAME_TABLE, "Vision", AddPortraitNicknameTable);

static int32_t CreatePhotosExtTable(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        PhotoExtUpgrade::CREATE_PHOTO_EXT_TABLE
    };
    MEDIA_INFO_LOG("Start create photo ext table in update");
    return ExecSqls(executeSqlStrs, store);
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_CREATE_PHOTOS_EXT_TABLE, "OtherTable", CreatePhotosExtTable);

static int32_t AddSearchTag(RdbStore &store)
{
    const vector<string> executeSqlStrs = {
        BaseColumn::AlterTableAddTextColumn(VISION_LABEL_TABLE, SEARCH_TAG_TYPE),
        BaseColumn::AlterTableAddBlobColumn(VISION_LABEL_TABLE, SEARCH_TAG_VECTOR),
    };
    MEDIA_INFO_LOG("start add search tag column");
    int32_t ret = ExecSqlsWithDfx(executeSqlStrs, store, VERSION_ADD_SEARCH_TAG);
    return ret;
}
REGISTER_SYNC_UPGRADE_TASK(VERSION_ADD_SEARCH_TAG, "Photos", AddSearchTag);

static int32_t AsyncUpgradeFromAllVersionFirstPart(RdbStore& rdbStore)
{
    MEDIA_INFO_LOG("Start VERSION_ADD_DETAIL_TIME");
    int32_t errCode = 0;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(RDB_FIX_RECORDS, errCode);
    if (prefs != nullptr) {
        MEDIA_INFO_LOG("prefs errCode: %{public}d", errCode);
        int32_t detailTimeFixed = prefs->GetInt(DETAIL_TIME_FIXED, 0);
        MEDIA_INFO_LOG("prefs current detailTimeFixed: %{public}d", detailTimeFixed);
        if (detailTimeFixed == NEED_FIXED) {
            errCode = UpdateDateTakenToMillionSecond(rdbStore);
            errCode = UpdateDateTakenIndex(rdbStore);
            ThumbnailService::GetInstance()->AstcChangeKeyFromDateAddedToDateTaken();
            prefs->PutInt(DETAIL_TIME_FIXED, ALREADY_FIXED);
            prefs->FlushSync();
            MEDIA_INFO_LOG("detailTimeFixed set to: %{public}d", ALREADY_FIXED);
        }
    }
    MEDIA_INFO_LOG("End VERSION_ADD_DETAIL_TIME");

    MEDIA_INFO_LOG("Start VERSION_ADD_INDEX_FOR_FILEID");
    errCode = AddIndexForFileIdAsync(rdbStore);
    MEDIA_INFO_LOG("End VERSION_ADD_INDEX_FOR_FILEID");

    MEDIA_INFO_LOG("Start VERSION_UPDATE_INDEX_FOR_COVER");
    errCode = UpdateIndexForCover(rdbStore);
    MEDIA_INFO_LOG("End VERSION_UPDATE_INDEX_FOR_COVER");

    MEDIA_INFO_LOG("Start VERSION_ADD_THUMBNAIL_VISIBLE");
    if (prefs != nullptr) {
        int32_t thumbnailVisibleFixed = prefs->GetInt(THUMBNAIL_VISIBLE_FIXED, 0);
        MEDIA_INFO_LOG("prefs current thumbnailVisibleFixed: %{public}d", thumbnailVisibleFixed);
        if (thumbnailVisibleFixed == NEED_FIXED) {
            errCode = UpdateThumbnailVisibleAndIdx(rdbStore);
            prefs->PutInt(THUMBNAIL_VISIBLE_FIXED, ALREADY_FIXED);
            prefs->FlushSync();
            MEDIA_INFO_LOG("thumbnailVisibleFixed set to: %{public}d", ALREADY_FIXED);
        }
        MEDIA_INFO_LOG("prefs errCode: %{public}d", errCode);
    }
    MEDIA_INFO_LOG("End VERSION_ADD_THUMBNAIL_VISIBLE");

    MEDIA_INFO_LOG("Start VERSION_UPDATE_DATETAKEN_AND_DETAILTIME");
    errCode = UpdateDateTakenAndDetailTime(rdbStore);
    MEDIA_INFO_LOG("End VERSION_UPDATE_DATETAKEN_AND_DETAILTIME");

    MEDIA_INFO_LOG("Start VERSION_ADD_READY_COUNT_INDEX");
    errCode = AddReadyCountIndex(rdbStore);
    MEDIA_INFO_LOG("End VERSION_ADD_READY_COUNT_INDEX");
    return errCode;
}

static int32_t AsyncUpgradeFromAllVersionSecondPart(RdbStore& rdbStore)
{
    MEDIA_INFO_LOG("Start VERSION_REVERT_FIX_DATE_ADDED_INDEX");
    int32_t ret = RevertFixDateAddedIndex(rdbStore);
    MEDIA_INFO_LOG("End VERSION_REVERT_FIX_DATE_ADDED_INDEX");

    MEDIA_INFO_LOG("Start VERSION_ADD_ALBUM_INDEX");
    ret = AddAlbumIndex(rdbStore);
    MEDIA_INFO_LOG("End VERSION_ADD_ALBUM_INDEX");

    MEDIA_INFO_LOG("Start VERSION_ADD_PHOTO_DATEADD_INDEX");
    ret = AddPhotoDateAddedIndex(rdbStore);
    MEDIA_INFO_LOG("End VERSION_ADD_PHOTO_DATEADD_INDEX");

    MEDIA_INFO_LOG("Start VERSION_REFRESH_PERMISSION_APPID");
    MediaLibraryRdbUtils::TransformAppId2TokenId(rdbStore);
    MEDIA_INFO_LOG("End VERSION_REFRESH_PERMISSION_APPID");

    MEDIA_INFO_LOG("Start VERSION_ADD_CLOUD_ENHANCEMENT_ALBUM_INDEX");
    ret = AddCloudEnhancementAlbumIndex(rdbStore);
    MEDIA_INFO_LOG("End VERSION_ADD_CLOUD_ENHANCEMENT_ALBUM_INDEX");

    MEDIA_INFO_LOG("Start VERSION_UPDATE_PHOTOS_DATE_AND_IDX");
    ret = UpdatePhotosDateAndIdx(rdbStore);
    MEDIA_INFO_LOG("End VERSION_UPDATE_PHOTOS_DATE_AND_IDX");

    MEDIA_INFO_LOG("Start VERSION_UPDATE_LATITUDE_AND_LONGITUDE_DEFAULT_NULL");
    ret = UpdateLocationDefaultNull(rdbStore);
    MEDIA_INFO_LOG("End VERSION_UPDATE_LATITUDE_AND_LONGITUDE_DEFAULT_NULL");

    MEDIA_INFO_LOG("Start VERSION_UPDATE_PHOTOS_DATE_IDX");
    ret = PhotoDayMonthYearOperation::UpdatePhotosDateIdx(rdbStore);
    MEDIA_INFO_LOG("End VERSION_UPDATE_PHOTOS_DATE_IDX");

    MEDIA_INFO_LOG("Start VERSION_UPDATE_MEDIA_TYPE_AND_THUMBNAIL_READY_IDX");
    ret = UpdateMediaTypeAndThumbnailReadyIdx(rdbStore);
    MEDIA_INFO_LOG("End VERSION_UPDATE_MEDIA_TYPE_AND_THUMBNAIL_READY_IDX");

    MEDIA_INFO_LOG("Start VERSION_UPDATE_LOCATION_KNOWLEDGE_INDEX");
    ret = UpdateLocationKnowledgeIdx(rdbStore);
    MEDIA_INFO_LOG("End VERSION_UPDATE_LOCATION_KNOWLEDGE_INDEX");

    MEDIA_INFO_LOG("Start VERSION_ADD_ALBUM_SUBTYPE_AND_NAME_INDEX");
    ret = AddAlbumSubtypeAndNameIdx(rdbStore);
    MEDIA_INFO_LOG("End VERSION_ADD_ALBUM_SUBTYPE_AND_NAME_INDEX");
    return ret;
}

static int32_t FixDbUpgradeToApi20(RdbStore &store)
{
    MEDIA_INFO_LOG("start FixDbUpgradeToApi20");
    int32_t ret = AsyncUpgradeFromAllVersionFirstPart(store);
    ret = AsyncUpgradeFromAllVersionSecondPart(store);
    MEDIA_INFO_LOG("end FixDbUpgradeToApi20");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_FIX_DB_UPGRADE_TO_API20, "Photos", FixDbUpgradeToApi20);

static int32_t AddFileManagerRelatedTables(RdbStore &store)
{
    const vector<string> sqls = {
        AlbumScanInfoColumn::CREATE_TABLE,
        MediaFileInterworkColumn::CREATE_FILE_OPT_TABLE,
    };
    MEDIA_INFO_LOG("AddFileManagerRelatedTables start");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_FILE_MANAGER_RELATED);
    MEDIA_INFO_LOG("AddFileManagerRelatedTables end");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_ADD_FILE_MANAGER_RELATED, "Album", AddFileManagerRelatedTables);

static int32_t AddFileHiddenColumns(RdbStore &store)
{
    const std::vector<std::string> sqls = {
        "ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_FILE_HIDDEN + " INT NOT NULL DEFAULT 0",
        "ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " +
            PhotoAlbumColumns::ALBUM_FILE_HIDDEN + " INT NOT NULL DEFAULT 0"
    };
    MEDIA_INFO_LOG("Add file_hidden columns to Photos and PhotoAlbum start");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_FILE_HIDDEN_COLUMN);
    MEDIA_INFO_LOG("Add file_hidden columns to Photos and PhotoAlbum end");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_ADD_FILE_HIDDEN_COLUMN,
    "Photos", AddFileHiddenColumns);

static int32_t AddLakeTriggerFilterForFileManager(RdbStore &store)
{
    const std::vector<std::string> sqls = {
        "DROP TRIGGER IF EXISTS operation_asset_insert_trigger",
        SQL_CREATE_OPERATION_ASSET_INSERT_TRIGGER,
        "DROP TRIGGER IF EXISTS operation_asset_delete_trigger",
        SQL_CREATE_OPERATION_ASSET_DELETE_TRIGGER,
        "DROP TRIGGER IF EXISTS operation_asset_update_trigger",
        SQL_CREATE_OPERATION_ASSET_UPDATE_TRIGGER,
        "DROP TRIGGER IF EXISTS operation_album_insert_trigger",
        SQL_CREATE_OPERATION_ALBUM_INSERT_TRIGGER,
        "DROP TRIGGER IF EXISTS operation_album_delete_trigger",
        SQL_CREATE_OPERATION_ALBUM_DELETE_TRIGGER,
        "DROP TRIGGER IF EXISTS operation_album_update_trigger",
        SQL_CREATE_OPERATION_ALBUM_UPDATE_TRIGGER,
    };
    MEDIA_INFO_LOG("Add lake trigger filter for file manager start");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_LAKE_TRIGGER_FILTER_FOR_FILE_MANAGER);
    MEDIA_INFO_LOG("Add lake trigger filter for file manager end");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_ADD_LAKE_TRIGGER_FILTER_FOR_FILE_MANAGER,
    "Photos", AddLakeTriggerFilterForFileManager);

static int32_t AddLcdFileSize(RdbStore &store)
{
    const vector<string> sqls = {
        "ALTER TABLE " + PHOTOS_TABLE + " ADD COLUMN " +
            PhotoColumn::PHOTO_LCD_FILE_SIZE + " INT NOT NULL DEFAULT 0 "
    };
    MEDIA_INFO_LOG("Add lcd aging columns start");
    int32_t ret = ExecSqlsWithDfx(sqls, store, VERSION_ADD_PHOTO_LCD_FILE_SIZE);
    MEDIA_INFO_LOG("Add lcd aging columns end");
    return ret;
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_ADD_PHOTO_LCD_FILE_SIZE,
    "Photos", AddLcdFileSize);

static int32_t CreateUriPermissionFileTokenIndex(RdbStore &store)
{
    static const vector<string> executeSqlStrs = {
        AppUriPermissionColumn::CREATE_URI_PERMISSION_FILE_TOKEN_INDEX
    };
    MEDIA_INFO_LOG("Start create idx_uri_permission_file_token in update");
    return ExecSqlsWithDfx(executeSqlStrs, store, VERSION_ADD_URI_PERMISSION_FILE_TOKEN_INDEX);
}
REGISTER_ASYNC_UPGRADE_TASK(VERSION_ADD_URI_PERMISSION_FILE_TOKEN_INDEX, "UriPermission",
    CreateUriPermissionFileTokenIndex);


int32_t MediaLibraryDataCallBack::OnUpgrade(RdbStore &store, int32_t oldVersion, int32_t newVersion)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryDataCallBack::OnUpgrade");
    if (RdbUpgradeUtils::GetOldVersion() == -1) {
        RdbUpgradeUtils::SetOldVersion(oldVersion);
    }
    MEDIA_INFO_LOG("OnUpgrade old:%{public}d, new:%{public}d", oldVersion, newVersion);
    g_upgradeErr = false;
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();

    // 使用新的数据库同步升级框架
    UpgradeManagerConfig config(false, RDB_UPGRADE_EVENT, RDB_CONFIG, oldVersion, newVersion);
    UpgradeManager::GetInstance().Initialize(config);
    int32_t ret = UpgradeManager::GetInstance().UpgradeSync(store);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("UpgradeSync failed: %{public}d", ret);
        g_upgradeErr = true;
    }

    // 上报升级打点信息
    if (!g_upgradeErr) {
        VariantMap map = {{KEY_PRE_VERSION, oldVersion}, {KEY_AFTER_VERSION, newVersion}};
        PostEventUtils::GetInstance().PostStatProcess(StatType::DB_UPGRADE_STAT, map);
    }

    return NativeRdb::E_OK;
}

int MediaLibraryRdbStore::Update(int &changedRows, const std::string &table, const ValuesBucket &row,
    const std::string &whereClause, const std::vector<std::string> &args)
{
    DfxTimer dfxTimer(DfxType::RDB_UPDATE, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRdbStore::UpdateByWhereArgs");

    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        return MediaLibraryRdbStore::GetRaw()->Update(changedRows, table, row, whereClause, args);
    });
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
    return MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        return MediaLibraryRdbStore::GetRaw()->Backup(databasePath, encryptKey);
    });
}

int MediaLibraryRdbStore::Backup(const std::string &databasePath, bool integrityCheck,
    const std::vector<uint8_t> &encryptKey)
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        return MediaLibraryRdbStore::GetRaw()->Backup(databasePath, encryptKey, integrityCheck);
    });
}

int MediaLibraryRdbStore::Sync(const DistributedRdb::SyncOption &option, const AbsRdbPredicates &predicate,
    const DistributedRdb::AsyncBrief &async)
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        return MediaLibraryRdbStore::GetRaw()->Sync(option, predicate, async);
    });
}

std::shared_ptr<NativeRdb::ResultSet> MediaLibraryRdbStore::QueryByStep(const std::string &sql,
    const std::vector<ValueObject> &args)
{
    DfxTimer dfxTimer(RDB_QUERY, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("QueryByStep by sql");

    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), nullptr,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return MediaLibraryRdbStore::GetRaw()->QueryByStep(sql, args);
}

std::shared_ptr<NativeRdb::ResultSet> MediaLibraryRdbStore::QueryByStep(const AbsRdbPredicates &predicates,
    const std::vector<std::string> &columns)
{
    DfxTimer dfxTimer(RDB_QUERY, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("QueryByStep by predicates");

    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), nullptr,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return MediaLibraryRdbStore::GetRaw()->QueryByStep(predicates, columns);
}

int MediaLibraryRdbStore::Update(int &changedRows, const ValuesBucket &row, const AbsRdbPredicates &predicates)
{
    DfxTimer dfxTimer(DfxType::RDB_UPDATE, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRdbStore::UpdateByPredicates");

    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        return MediaLibraryRdbStore::GetRaw()->Update(changedRows, row, predicates);
    });
}

int MediaLibraryRdbStore::Delete(int &deletedRows, const std::string &table, const std::string &whereClause,
    const std::vector<std::string> &args)
{
    DfxTimer dfxTimer(DfxType::RDB_DELETE, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRdbStore::DeleteByWhereArgs");

    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        return MediaLibraryRdbStore::GetRaw()->Delete(deletedRows, table, whereClause, args);
    });
}

int MediaLibraryRdbStore::Delete(int &deletedRows, const AbsRdbPredicates &predicates)
{
    DfxTimer dfxTimer(DfxType::RDB_DELETE, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRdbStore::DeleteByPredicates");

    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        return MediaLibraryRdbStore::GetRaw()->Delete(deletedRows, predicates);
    });
}

pair<int32_t, NativeRdb::Results> MediaLibraryRdbStore::BatchInsertWithReturn(const string &table,
    vector<ValuesBucket> &values, const string &returningField)
{
    DfxTimer dfxTimer(DfxType::RDB_BATCHINSERT, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRdbStore::BatchInsertWithReturn");

    pair<int32_t, NativeRdb::Results> retWithResults = {E_HAS_DB_ERROR, -1};
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), retWithResults,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");

    std::vector<NativeRdb::ValuesBucket> tmpValues = values;
    TableStrategyConfig config = {
        .enableDefault = true,
    };
    int32_t strategyRet =
        RdbTableStrategyManager::GetInstance().ExtendBatchInsertValues(table, tmpValues, *GetRaw().get(), config);
    if (strategyRet != E_OK) {
        MEDIA_ERR_LOG("Failed to ExtendBatchInsertValues, ret: %{public}d.", strategyRet);
        return retWithResults;
    }

    int32_t ret = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        retWithResults = MediaLibraryRdbStore::GetRaw()->BatchInsert(table, tmpValues, { returningField });
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

pair<int32_t, NativeRdb::Results> MediaLibraryRdbStore::ExecuteSqlWithReturn(const std::string &sql,
    const std::vector<NativeRdb::ValueObject> &args, const std::string &returningField)
{
    pair<int32_t, NativeRdb::Results> retWithResults = {E_HAS_DB_ERROR, -1};
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), retWithResults,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");

    DfxTimer dfxTimer(RDB_EXECUTE_SQL, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("RdbStore->ExecuteSqlWithReturn");

    string execSql = sql;
    execSql.append(" returning ").append(returningField);
    MEDIA_INFO_LOG("AccurateRefresh, sql:%{public}s", execSql.c_str());
    int32_t ret = MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
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

pair<int32_t, NativeRdb::Results> MediaLibraryRdbStore::UpdateWithReturn(const ValuesBucket &row,
    const AbsRdbPredicates &predicates, const string &returningField)
{
    DfxTimer dfxTimer(DfxType::RDB_UPDATE, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRdbStore::UpdateWithReturn");

    pair<int32_t, NativeRdb::Results> retWithResults = {E_HAS_DB_ERROR, -1};
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), retWithResults,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");

    MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        retWithResults = MediaLibraryRdbStore::GetRaw()->Update(row, predicates, { returningField });
        return retWithResults.first;
    });

    return retWithResults;
}

pair<int32_t, NativeRdb::Results> MediaLibraryRdbStore::DeleteWithReturn(const AbsRdbPredicates &predicates,
    const string &returningField)
{
    DfxTimer dfxTimer(DfxType::RDB_DELETE, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("RdbStore->DeleteWithReturn");

    pair<int32_t, NativeRdb::Results> retWithResults = {E_HAS_DB_ERROR, -1};
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), retWithResults,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");

    MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        retWithResults = MediaLibraryRdbStore::GetRaw()->Delete(predicates, { returningField });
        return retWithResults.first;
    });
    return retWithResults;
}

std::shared_ptr<NativeRdb::ResultSet> MediaLibraryRdbStore::Query(const NativeRdb::AbsRdbPredicates &predicates,
    const std::vector<std::string> &columns)
{
    DfxTimer dfxTimer(RDB_QUERY, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRdbStore::Query");
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), nullptr,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return MediaLibraryRdbStore::GetRaw()->Query(predicates, columns);
}

std::shared_ptr<AbsSharedResultSet> MediaLibraryRdbStore::QuerySql(const std::string &sql,
    const std::vector<ValueObject> &args)
{
    DfxTimer dfxTimer(RDB_EXECUTE_SQL, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRdbStore::QuerySql by args");
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), nullptr,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return MediaLibraryRdbStore::GetRaw()->QuerySql(sql, args);
}

int MediaLibraryRdbStore::InterruptBackup()
{
    CHECK_AND_RETURN_RET_LOG(MediaLibraryRdbStore::CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        return MediaLibraryRdbStore::GetRaw()->InterruptBackup();
    });
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
    return MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        return MediaLibraryRdbStore::GetRaw()->Restore(backupPath, newKey);
    });
}

int32_t MediaLibraryRdbStore::DataCallBackOnCreate()
{
    MediaLibraryDataCallBack callback;
    int32_t ret = callback.OnCreate(*GetRaw());
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK,
        "MediaLibraryDataCallBack OnCreate error, ret: %{public}d", ret);
    return ret;
}

int MediaLibraryRdbStore::ExecuteForChangedRowCount(int64_t &outValue, const std::string &sql,
    const std::vector<NativeRdb::ValueObject> &args)
{
    DfxTimer dfxTimer(RDB_EXECUTE_SQL, INVALID_DFX, RDB_TIME_OUT, false);
    MediaLibraryTracer tracer;
    tracer.Start("RdbStore->ExecuteForChangedRowCount");

    CHECK_AND_RETURN_RET_LOG(CheckRdbStore(), E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");
    return MediaLibraryRdbHelper::ExecSqlWithRetry([&]() {
        return GetRaw()->ExecuteForChangedRowCount(outValue, sql, args);
    });
}

void MediaLibraryRdbStore::AddUpgradeTable(const shared_ptr<MediaLibraryRdbStore> store)
{
    const vector<string> sqls = {
        CREATE_TAB_ANALYSIS_TOTAL_FOR_ONCREATE,
        CREATE_TAB_ANALYSIS_VIDEO_TOTAL,
        CREATE_SEARCH_TOTAL_TABLE,
    };
    MEDIA_INFO_LOG("start create table");
    ExecSqls(sqls, *store->GetRaw().get());
    MEDIA_INFO_LOG("end create table");
}

void MediaLibraryRdbStore::CheckAndAddPhotoAlbumColumns(const shared_ptr<MediaLibraryRdbStore> store)
{
    MEDIA_INFO_LOG("start check and add columns");
    std::vector<std::string> sqls;
    if (!MediaLibraryRdbHelper::HasColumnInTable(
        *store->GetRaw(), PhotoColumn::PHOTO_CHANGE_TIME, PhotoColumn::PHOTOS_TABLE)) {
        sqls.push_back("ALTER TABLE " + PhotoColumn::PHOTOS_TABLE + " ADD COLUMN " + PhotoColumn::PHOTO_CHANGE_TIME +
            " BIGINT NOT NULL DEFAULT 0");
    }
    if (!MediaLibraryRdbHelper::HasColumnInTable(
        *store->GetRaw(), PhotoAlbumColumns::CHANGE_TIME, PhotoAlbumColumns::TABLE)) {
        sqls.push_back("ALTER TABLE " + PhotoAlbumColumns::TABLE + " ADD COLUMN " + PhotoAlbumColumns::CHANGE_TIME +
            " BIGINT NOT NULL DEFAULT 0");
    }
    ExecSqls(sqls, *store->GetRaw().get());
    MEDIA_INFO_LOG("end check and add columns");
}
} // namespace OHOS::Media

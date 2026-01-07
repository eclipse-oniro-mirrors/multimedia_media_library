/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "medialibrary_asset_operations.h"

#include "cloud_media_asset_manager.h"
#include "dfx_utils.h"
#include "directory_ex.h"
#include "heif_transcoding_check_utils.h"
#include "map_operation_flag.h"
#include "media_app_uri_permission_column.h"
#include "media_exif.h"
#include "media_scanner_manager.h"
#include "media_unique_number_column.h"
#include "medialibrary_audio_operations.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_inotify.h"
#ifdef META_RECOVERY_SUPPORT
#include "medialibrary_meta_recovery.h"
#endif
#include "medialibrary_notify.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_tracer.h"
#include "medialibrary_urisensitive_operations.h"
#include "media_privacy_manager.h"
#include "mimetype_utils.h"
#include "multistages_capture_manager.h"
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
#include "enhancement_manager.h"
#endif
#include "permission_utils.h"
#include "result_set_utils.h"
#include "thumbnail_service.h"
#include "medialibrary_formmap_operations.h"
#include "medialibrary_vision_operations.h"
#include "dfx_manager.h"
#include "moving_photo_file_utils.h"
#include "medialibrary_album_fusion_utils.h"
#include "data_secondary_directory_uri.h"
#include "medialibrary_restore.h"
#include "cloud_sync_helper.h"
#include "refresh_business_name.h"
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
#include "background_cloud_batch_selected_file_processor.h"
#endif
#include "media_file_manager_temp_file_aging_task.h"
#include "preferences_helper.h"
#include "file_utils.h"
#include "medialibrary_transcode_data_aging_operation.h"
#include "lake_file_utils.h"
#include "cloud_media_common.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
mutex g_uniqueNumberLock;

constexpr int32_t NO_DESENSITIZE = 3;
const string PHOTO_ALBUM_URI_PREFIX = "file://media/PhotoAlbum/";
constexpr int32_t UNKNOWN_VALUE = -1;
constexpr int32_t LOCAL_PHOTO_POSITION = 1;
constexpr int32_t BOTH_LOCAL_CLOUD_PHOTO_POSITION = 3;
constexpr int32_t MAX_PROCESS_NUM = 200;
constexpr int64_t INVALID_SIZE = 0;
constexpr int64_t SHARE_UID = 5520;
static const std::string ANALYSIS_FILE_PATH = "/storage/cloud/files/highlight/music";
static const std::string DELETED_FILE_EVENT = "/data/storage/el2/base/preferences/deleted_file_events.xml";
const double TIMER_MULTIPLIER = 60.0;

struct DeletedFilesParams {
    vector<string> ids;
    vector<string> paths;
    vector<string> dateTakens;
    vector<int32_t> subTypes;
    vector<int32_t> isTemps;
    map<string, string> displayNames;
    map<string, string> albumNames;
    map<string, string> ownerAlbumIds;
    bool containsHidden = false;
};

const static vector<string> PHOTO_COLUMN_VECTOR = {
    MediaColumn::MEDIA_NAME,
    PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE,
};

const std::unordered_map<std::string, int> FILEASSET_MEMBER_MAP = {
    { MediaColumn::MEDIA_ID, MEMBER_TYPE_INT32 },
    { MediaColumn::MEDIA_FILE_PATH, MEMBER_TYPE_STRING },
    { MediaColumn::MEDIA_SIZE, MEMBER_TYPE_INT64 },
    { MediaColumn::MEDIA_TITLE, MEMBER_TYPE_STRING },
    { MediaColumn::MEDIA_NAME, MEMBER_TYPE_STRING },
    { MediaColumn::MEDIA_TYPE, MEMBER_TYPE_INT32 },
    { MediaColumn::MEDIA_MIME_TYPE, MEMBER_TYPE_STRING },
    { MediaColumn::MEDIA_OWNER_PACKAGE, MEMBER_TYPE_STRING },
    { MediaColumn::MEDIA_OWNER_APPID, MEMBER_TYPE_STRING },
    { MediaColumn::MEDIA_PACKAGE_NAME, MEMBER_TYPE_STRING },
    { MediaColumn::MEDIA_DEVICE_NAME, MEMBER_TYPE_STRING },
    { MediaColumn::MEDIA_DATE_ADDED, MEMBER_TYPE_INT64 },
    { MediaColumn::MEDIA_DATE_MODIFIED, MEMBER_TYPE_INT64 },
    { MediaColumn::MEDIA_DATE_TAKEN, MEMBER_TYPE_INT64 },
    { MediaColumn::MEDIA_DATE_DELETED, MEMBER_TYPE_INT64 },
    { MediaColumn::MEDIA_DURATION, MEMBER_TYPE_INT32 },
    { MediaColumn::MEDIA_TIME_PENDING, MEMBER_TYPE_INT64 },
    { MediaColumn::MEDIA_IS_FAV, MEMBER_TYPE_INT32 },
    { MediaColumn::MEDIA_DATE_TRASHED, MEMBER_TYPE_INT64 },
    { MediaColumn::MEDIA_HIDDEN, MEMBER_TYPE_INT32 },
    { MediaColumn::MEDIA_PARENT_ID, MEMBER_TYPE_INT32 },
    { MediaColumn::MEDIA_RELATIVE_PATH, MEMBER_TYPE_STRING },
    { MediaColumn::MEDIA_VIRTURL_PATH, MEMBER_TYPE_STRING },
    { PhotoColumn::PHOTO_ORIENTATION, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_LATITUDE, MEMBER_TYPE_DOUBLE },
    { PhotoColumn::PHOTO_LONGITUDE, MEMBER_TYPE_DOUBLE },
    { PhotoColumn::PHOTO_HEIGHT, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_WIDTH, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_ALL_EXIF, MEMBER_TYPE_STRING },
    { PhotoColumn::PHOTO_LCD_VISIT_TIME, MEMBER_TYPE_INT64 },
    { PhotoColumn::PHOTO_EDIT_TIME, MEMBER_TYPE_INT64 },
    { PhotoColumn::PHOTO_TRANSCODE_TIME, MEMBER_TYPE_INT64 },
    { PhotoColumn::PHOTO_TRANS_CODE_FILE_SIZE, MEMBER_TYPE_INT64 },
    { PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_SUBTYPE, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_ORIGINAL_SUBTYPE, MEMBER_TYPE_INT32 },
    { PhotoColumn::MOVING_PHOTO_EFFECT_MODE, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_COVER_POSITION, MEMBER_TYPE_INT64 },
    { PhotoColumn::PHOTO_CE_AVAILABLE, MEMBER_TYPE_INT32 },
    { AudioColumn::AUDIO_ALBUM, MEMBER_TYPE_STRING },
    { AudioColumn::AUDIO_ARTIST, MEMBER_TYPE_STRING },
    { PhotoColumn::PHOTO_OWNER_ALBUM_ID, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_BURST_KEY, MEMBER_TYPE_STRING },
    { PhotoColumn::PHOTO_BURST_COVER_LEVEL, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_THUMBNAIL_READY, MEMBER_TYPE_INT64 },
    { PhotoColumn::PHOTO_POSITION, MEMBER_TYPE_INT32 },
    { PhotoColumn::SUPPORTED_WATERMARK_TYPE, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_SOURCE_PATH, MEMBER_TYPE_STRING },
    { PhotoColumn::PHOTO_DIRTY, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_CLOUD_ID, MEMBER_TYPE_STRING },
    { PhotoColumn::PHOTO_META_DATE_MODIFIED, MEMBER_TYPE_INT64 },
    { PhotoColumn::PHOTO_SYNC_STATUS, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_CLOUD_VERSION, MEMBER_TYPE_INT64 },
    { PhotoColumn::CAMERA_SHOT_KEY, MEMBER_TYPE_STRING },
    { PhotoColumn::PHOTO_USER_COMMENT, MEMBER_TYPE_STRING },
    { PhotoColumn::PHOTO_DATE_YEAR, MEMBER_TYPE_STRING },
    { PhotoColumn::PHOTO_DATE_MONTH, MEMBER_TYPE_STRING },
    { PhotoColumn::PHOTO_DATE_DAY, MEMBER_TYPE_STRING },
    { PhotoColumn::PHOTO_SHOOTING_MODE, MEMBER_TYPE_STRING },
    { PhotoColumn::PHOTO_SHOOTING_MODE_TAG, MEMBER_TYPE_STRING },
    { PhotoColumn::PHOTO_LAST_VISIT_TIME, MEMBER_TYPE_INT64 },
    { PhotoColumn::PHOTO_HIDDEN_TIME, MEMBER_TYPE_INT64 },
    { PhotoColumn::PHOTO_THUMB_STATUS, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_CLEAN_FLAG, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_ID, MEMBER_TYPE_STRING },
    { PhotoColumn::PHOTO_QUALITY, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_FIRST_VISIT_TIME, MEMBER_TYPE_INT64 },
    { PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_HDR_MODE, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_LCD_SIZE, MEMBER_TYPE_STRING },
    { PhotoColumn::PHOTO_THUMB_SIZE, MEMBER_TYPE_STRING },
    { PhotoColumn::PHOTO_FRONT_CAMERA, MEMBER_TYPE_STRING },
    { PhotoColumn::PHOTO_IS_TEMP, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_CE_STATUS_CODE, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_STRONG_ASSOCIATION, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_ASSOCIATE_FILE_ID, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_HAS_CLOUD_WATERMARK, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_DETAIL_TIME, MEMBER_TYPE_STRING },
    { PhotoColumn::PHOTO_ORIGINAL_ASSET_CLOUD_ID, MEMBER_TYPE_STRING },
    { PhotoColumn::PHOTO_METADATA_FLAGS, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_IS_AUTO, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_MEDIA_SUFFIX, MEMBER_TYPE_STRING },
    { PhotoColumn::STAGE_VIDEO_TASK_STATUS, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_VIDEO_MODE, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_FILE_INODE, MEMBER_TYPE_STRING },
    { PhotoColumn::PHOTO_STORAGE_PATH, MEMBER_TYPE_STRING },
    { PhotoColumn::PHOTO_FILE_SOURCE_TYPE, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_IS_RECTIFICATION_COVER, MEMBER_TYPE_INT32 },
    { PhotoColumn::PHOTO_CHANGE_TIME, MEMBER_TYPE_INT64 },
};

const std::unordered_map<std::string, int>& GetFileAssetMemberMap()
{
    return FILEASSET_MEMBER_MAP;
}

int32_t MediaLibraryAssetOperations::HandleInsertOperationExt(MediaLibraryCommand& cmd)
{
    int errCode = E_ERR;
    switch (cmd.GetOprnType()) {
        case OperationType::LS_MEDIA_FILES:
            errCode = MediaLibraryPhotoOperations::LSMediaFiles(cmd);
            break;
        default:
            MEDIA_ERR_LOG("unknown operation type %{public}d", cmd.GetOprnType());
            break;
    }
    return errCode;
}

int32_t MediaLibraryAssetOperations::HandleInsertOperation(MediaLibraryCommand &cmd)
{
    int errCode = E_ERR;
    switch (cmd.GetOprnType()) {
        case OperationType::CREATE:
            errCode = CreateOperation(cmd);
            break;
        case OperationType::CLOSE:
            errCode = CloseOperation(cmd);
            break;
        case OperationType::COMMIT_EDIT:
            errCode = MediaLibraryPhotoOperations::CommitEditInsert(cmd);
            if (errCode == E_SUCCESS) {
                MediaLibraryVisionOperations::EditCommitOperation(cmd);
            }
            break;
        case OperationType::REVERT_EDIT:
            errCode = MediaLibraryPhotoOperations::RevertToOrigin(cmd);
            if (errCode == E_SUCCESS) {
                MediaLibraryVisionOperations::EditCommitOperation(cmd);
            }
            break;
        case OperationType::SUBMIT_CACHE:
            errCode = MediaLibraryPhotoOperations::SubmitCache(cmd);
            break;
        case OperationType::CUSTOM_RESTORE:
            errCode = MediaLibraryPhotoOperations::ProcessCustomRestore(cmd);
            break;
        case OperationType::CUSTOM_RESTORE_CANCEL:
            errCode = MediaLibraryPhotoOperations::CancelCustomRestore(cmd);
            break;
        case OperationType::ADD_FILTERS:
            errCode = MediaLibraryPhotoOperations::AddFilters(cmd);
            break;
        case OperationType::SCAN_WITHOUT_ALBUM_UPDATE:
            errCode = MediaLibraryPhotoOperations::ScanFileWithoutAlbumUpdate(cmd);
            break;
        case OperationType::FINISH_REQUEST_PICTURE:
            errCode = MediaLibraryPhotoOperations::FinishRequestPicture(cmd);
            break;
        case OperationType::CLONE_ASSET:
            errCode = MediaLibraryPhotoOperations::CloneSingleAsset(cmd);
            break;
        default:
            errCode = HandleInsertOperationExt(cmd);
            break;
    }
    return errCode;
}

int32_t MediaLibraryAssetOperations::CreateOperation(MediaLibraryCommand &cmd)
{
    // CreateAsset specify type
    switch (cmd.GetOprnObject()) {
        case OperationObject::FILESYSTEM_PHOTO:
        case OperationObject::PTP_OPERATION:
            return MediaLibraryPhotoOperations::Create(cmd);
        case OperationObject::FILESYSTEM_AUDIO:
            return MediaLibraryAudioOperations::Create(cmd);
        case OperationObject::FILESYSTEM_ASSET:
            MEDIA_ERR_LOG("create asset by FileSysetm_Asset is deperated");
            return E_INVALID_VALUES;
        default:
            MEDIA_ERR_LOG("error operation object: %{public}d", cmd.GetOprnObject());
            return E_INVALID_VALUES;
    }
}

int32_t MediaLibraryAssetOperations::DeleteOperation(MediaLibraryCommand &cmd)
{
    // delete Asset specify type
    switch (cmd.GetOprnObject()) {
        case OperationObject::FILESYSTEM_PHOTO:
            return MediaLibraryPhotoOperations::Delete(cmd);
        case OperationObject::FILESYSTEM_AUDIO:
            return MediaLibraryAudioOperations::Delete(cmd);
        case OperationObject::FILESYSTEM_ASSET:
            MEDIA_ERR_LOG("delete asset by FILESYSTEM_ASSET is deperated");
            return E_INVALID_VALUES;
        default:
            MEDIA_ERR_LOG("error operation object: %{public}d", cmd.GetOprnObject());
            return E_INVALID_VALUES;
    }
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryAssetOperations::QueryOperation(
    MediaLibraryCommand &cmd, const vector<string> &columns)
{
    // query asset specify type
    switch (cmd.GetOprnObject()) {
        case OperationObject::FILESYSTEM_PHOTO:
        case OperationObject::EDIT_DATA_EXISTS:
        case OperationObject::MOVING_PHOTO_VIDEO_READY:
            return MediaLibraryPhotoOperations::Query(cmd, columns);
        case OperationObject::FILESYSTEM_AUDIO:
            return MediaLibraryAudioOperations::Query(cmd, columns);
        case OperationObject::FILESYSTEM_ASSET:
            MEDIA_ERR_LOG("api9 operation is not finished");
            return nullptr;
        case OperationObject::PAH_MOVING_PHOTO:
            return MediaLibraryPhotoOperations::ScanMovingPhoto(cmd, columns);
        default:
            MEDIA_ERR_LOG("error operation objec: %{public}d", cmd.GetOprnObject());
            return nullptr;
    }
}

int32_t MediaLibraryAssetOperations::UpdateOperation(MediaLibraryCommand &cmd)
{
    if (!AssetInputParamVerification::CheckParamForUpdate(cmd)) {
        return E_INVALID_VALUES;
    }

    switch (cmd.GetOprnObject()) {
        case OperationObject::PAH_PHOTO:
        case OperationObject::PAH_VIDEO:
        case OperationObject::FILESYSTEM_PHOTO:
        case OperationObject::PTP_OPERATION:
            return MediaLibraryPhotoOperations::Update(cmd);
        case OperationObject::FILESYSTEM_AUDIO:
            return MediaLibraryAudioOperations::Update(cmd);
        case OperationObject::FILESYSTEM_ASSET:
            MEDIA_ERR_LOG("create asset by FILESYSTEM_ASSET is deperated");
            return E_INVALID_VALUES;
        default:
            MEDIA_ERR_LOG("error operation object: %{public}d", cmd.GetOprnObject());
            return E_INVALID_VALUES;
    }
}

int32_t MediaLibraryAssetOperations::OpenOperation(MediaLibraryCommand &cmd, const string &mode)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryAssetOperations::OpenOperation");

    // Open specify type
    switch (cmd.GetOprnObject()) {
        case OperationObject::FILESYSTEM_PHOTO:
        case OperationObject::PTP_OPERATION:
            return MediaLibraryPhotoOperations::Open(cmd, mode);
        case OperationObject::FILESYSTEM_AUDIO:
            return MediaLibraryAudioOperations::Open(cmd, mode);
        case OperationObject::HIGHLIGHT_COVER:
            return MediaLibraryAssetOperations::OpenHighlightCover(cmd, mode);
        case OperationObject::HIGHLIGHT_URI:
            return MediaLibraryAssetOperations::OpenHighlightVideo(cmd, mode);
        case OperationObject::FILESYSTEM_ASSET:
            MEDIA_ERR_LOG("open by FILESYSTEM_ASSET is deperated");
            return E_INVALID_VALUES;
        default:
            MEDIA_ERR_LOG("error operation object: %{public}d", cmd.GetOprnObject());
            return E_INVALID_VALUES;
    }
}

int32_t MediaLibraryAssetOperations::CloseOperation(MediaLibraryCommand &cmd)
{
    // Close specify type
    switch (cmd.GetOprnObject()) {
        case OperationObject::FILESYSTEM_PHOTO:
        case OperationObject::PTP_OPERATION:
            return MediaLibraryPhotoOperations::Close(cmd);
        case OperationObject::FILESYSTEM_AUDIO:
            return MediaLibraryAudioOperations::Close(cmd);
        case OperationObject::FILESYSTEM_ASSET:
            MEDIA_ERR_LOG("close by FILESYSTEM_ASSET is deperated");
            return E_INVALID_VALUES;
        default:
            MEDIA_ERR_LOG("error operation object: %{public}d", cmd.GetOprnObject());
            return E_INVALID_VALUES;
    }
}

static int32_t DropAllTables(const shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    string dropSqlRowName = "drop_table_and_view_sql";
    string queryDropSql =
        "SELECT 'DROP ' || type || ' IF EXISTS ' || name || ';' as " + dropSqlRowName +
        " FROM sqlite_master" +
        " WHERE type IN ('table', 'view') AND name NOT LIKE 'sqlite_%';";
    auto dropSqlsResultSet = rdbStore->QuerySql(queryDropSql);
    CHECK_AND_RETURN_RET_LOG(dropSqlsResultSet != nullptr, E_HAS_DB_ERROR, "query Drop Sql failed");

    vector<string> dropSqlsVec;
    while (dropSqlsResultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t columnIndex = 0;
        CHECK_AND_RETURN_RET_LOG(dropSqlsResultSet->GetColumnIndex(dropSqlRowName, columnIndex) == NativeRdb::E_OK,
            E_HAS_DB_ERROR, "Get drop_table_and_view_sql column failed");

        string sql;
        CHECK_AND_RETURN_RET_LOG(dropSqlsResultSet->GetString(columnIndex, sql) == NativeRdb::E_OK,
            E_HAS_DB_ERROR, "Get drop_table_and_view_sql sql failed");
        if (!sql.empty()) {
            dropSqlsVec.push_back(sql);
        }
    }

    for (const auto &dropSql : dropSqlsVec) {
        rdbStore->ExecuteSql(dropSql);
    }
    return E_OK;
}

static void DeleteFileManagerTempFileAgingXml()
{
    CHECK_AND_RETURN_LOG(MediaFileUtils::IsFileExists(Background::FILE_MANAGER_TEMP_FILE_AGING_EVENT),
        "File manager temp file aging xml not exist");
    int32_t errCode = NativePreferences::PreferencesHelper::DeletePreferences(
        Background::FILE_MANAGER_TEMP_FILE_AGING_EVENT);
    CHECK_AND_RETURN_LOG(errCode == E_OK,
        "Failed to delete file manager temp file aging xml, errCode: %{public}d", errCode);
}

static void ResetFileIdProgressData()
{
    const string taskProgressXml = "/data/storage/el2/base/preferences/task_progress.xml";
    int32_t errCode = 0;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(taskProgressXml, errCode);
    CHECK_AND_RETURN_LOG(prefs, "get preferences error: %{public}d", errCode);
    prefs->PutInt("recognize_ce_photos_file_id_progress", 0);
    prefs->FlushSync();
}

int32_t MediaLibraryAssetOperations::DeleteToolOperation(MediaLibraryCommand &cmd)
{
    auto valuesBucket = cmd.GetValueBucket();
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "Can not get rdb store");
    int32_t errCode = DropAllTables(rdbStore);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Drop table failed, errCode=%{public}d", errCode);
    errCode = rdbStore->DataCallBackOnCreate();
    UriSensitiveOperations::DeleteAllSensitiveAsync();
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "DataCallBackOnCreate failed, errCode=%{public}d", errCode);

    const static vector<string> DELETE_DIR_LIST = {
        ROOT_MEDIA_DIR + PHOTO_BUCKET,
        ROOT_MEDIA_DIR + AUDIO_BUCKET,
        ROOT_MEDIA_DIR + CAMERA_DIR_VALUES,
        ROOT_MEDIA_DIR + VIDEO_DIR_VALUES,
        ROOT_MEDIA_DIR + PIC_DIR_VALUES,
        ROOT_MEDIA_DIR + AUDIO_DIR_VALUES,
        ROOT_MEDIA_DIR + ".thumbs",
        ROOT_MEDIA_DIR + ".editData"
    };

    for (const string &dir : DELETE_DIR_LIST) {
        if (!MediaFileUtils::DeleteDir(dir)) {
            MEDIA_ERR_LOG("Delete dir %{public}s failed", dir.c_str());
            continue;
        }
        if (!MediaFileUtils::CreateDirectory(dir)) {
            MEDIA_ERR_LOG("Create dir %{public}s failed", dir.c_str());
        };
    }

    string photoThumbsPath = ROOT_MEDIA_DIR + ".thumbs/Photo";
    CHECK_AND_PRINT_LOG(MediaFileUtils::CreateDirectory(photoThumbsPath),
        "Create dir %{public}s failed", photoThumbsPath.c_str());
    DeleteFileManagerTempFileAgingXml();
    ResetFileIdProgressData();
    return E_OK;
}

static bool CheckOprnObject(OperationObject object)
{
    const set<OperationObject> validOprnObjectet = {
        OperationObject::FILESYSTEM_PHOTO,
        OperationObject::FILESYSTEM_AUDIO
    };
    if (validOprnObjectet.find(object) == validOprnObjectet.end()) {
        MEDIA_ERR_LOG("input OperationObject %{public}d error!", object);
        return false;
    }
    return true;
}

static OperationObject GetOprnObjectByMediaType(int32_t type)
{
    switch (type) {
        case MediaType::MEDIA_TYPE_IMAGE:
        case MediaType::MEDIA_TYPE_VIDEO: {
            return OperationObject::FILESYSTEM_PHOTO;
        }
        case MediaType::MEDIA_TYPE_AUDIO: {
            return OperationObject::FILESYSTEM_AUDIO;
        }
        case MediaType::MEDIA_TYPE_FILE: {
            return OperationObject::FILESYSTEM_ASSET;
        }
        default: {
            return OperationObject::UNKNOWN_OBJECT;
        }
    }
}

static shared_ptr<FileAsset> FetchFileAssetFromResultSet(
    const shared_ptr<NativeRdb::ResultSet> &resultSet, const vector<string> &columns)
{
    int32_t count = 0;
    int32_t currentRowIndex = 0;
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, nullptr, "resultSet is nullptr");
    CHECK_AND_RETURN_RET_LOG(
        resultSet->GetRowCount(count) == NativeRdb::E_OK, nullptr, "Cannot get row count of resultset");
    CHECK_AND_RETURN_RET_LOG(
        resultSet->GetRowIndex(currentRowIndex) == NativeRdb::E_OK, nullptr, "Cannot get row index of resultset");
    CHECK_AND_RETURN_RET_LOG(currentRowIndex >= 0 && currentRowIndex < count, nullptr, "Invalid row index");

    auto fileAsset = make_shared<FileAsset>();
    auto &map = fileAsset->GetMemberMap();
    for (const auto &column : columns) {
        int32_t columnIndex = 0;
        CHECK_AND_RETURN_RET_LOG(resultSet->GetColumnIndex(column, columnIndex) == NativeRdb::E_OK,
            nullptr, "Can not get column %{private}s index", column.c_str());
        CHECK_AND_RETURN_RET_LOG(GetFileAssetMemberMap().find(column) != GetFileAssetMemberMap().end(), nullptr,
            "Can not find column %{private}s from member map", column.c_str());
        switch (GetFileAssetMemberMap().at(column)) {
            case MEMBER_TYPE_INT32: {
                int32_t value = 0;
                CHECK_AND_RETURN_RET_LOG(resultSet->GetInt(columnIndex, value) == NativeRdb::E_OK, nullptr,
                    "Can not get int value from column %{private}s", column.c_str());
                map[column] = value;
                break;
            }
            case MEMBER_TYPE_INT64: {
                int64_t value = 0;
                CHECK_AND_RETURN_RET_LOG(resultSet->GetLong(columnIndex, value) == NativeRdb::E_OK, nullptr,
                    "Can not get long value from column %{private}s", column.c_str());
                map[column] = value;
                break;
            }
            case MEMBER_TYPE_STRING: {
                string value;
                CHECK_AND_RETURN_RET_LOG(resultSet->GetString(columnIndex, value) == NativeRdb::E_OK, nullptr,
                    "Can not get string value from column %{private}s", column.c_str());
                map[column] = value;
                break;
            }
            case MEMBER_TYPE_DOUBLE: {
                double value;
                CHECK_AND_RETURN_RET_LOG(resultSet->GetDouble(columnIndex, value) == NativeRdb::E_OK, nullptr,
                    "Can not get double value from column %{private}s", column.c_str());
                map[column] = value;
                break;
            }
        }
    }
    return fileAsset;
}

shared_ptr<FileAsset> MediaLibraryAssetOperations::GetAssetFromResultSet(
    const shared_ptr<NativeRdb::ResultSet> &resultSet, const vector<string> &columns)
{
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, nullptr, "resultSet is nullptr");
    int32_t count = 0;
    CHECK_AND_RETURN_RET_LOG(resultSet->GetRowCount(count) == NativeRdb::E_OK, nullptr,
        "Cannot get row count of resultset");
    CHECK_AND_RETURN_RET_LOG(count == 1, nullptr, "ResultSet count is %{public}d, not 1", count);
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK, nullptr, "Cannot go to first row");
    return FetchFileAssetFromResultSet(resultSet, columns);
}

static int32_t GetAssetVectorFromResultSet(const shared_ptr<NativeRdb::ResultSet> &resultSet,
    const vector<string> &columns, vector<shared_ptr<FileAsset>> &fileAssetVector)
{
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "resultSet is nullptr");
    int32_t count = 0;
    CHECK_AND_RETURN_RET_LOG(resultSet->GetRowCount(count) == NativeRdb::E_OK, E_HAS_DB_ERROR,
        "Cannot get row count of resultset");
    CHECK_AND_RETURN_RET_LOG(count > 0, E_HAS_DB_ERROR, "ResultSet count is %{public}d", count);

    fileAssetVector.reserve(count);
    for (int32_t i = 0; i < count; i++) {
        CHECK_AND_RETURN_RET_LOG(
            resultSet->GoToNextRow() == NativeRdb::E_OK, E_HAS_DB_ERROR, "Failed to go to next row");
        auto fileAsset = FetchFileAssetFromResultSet(resultSet, columns);
        CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_HAS_DB_ERROR, "Failed to fetch fileAsset from resultSet");
        fileAssetVector.push_back(fileAsset);
    }
    return E_OK;
}

static int32_t GetAlbumVectorFromResultSet(const shared_ptr<NativeRdb::ResultSet> &resultSet,
    const vector<string> &columns, vector<shared_ptr<PhotoAlbum>> &PhotoAlbumVector)
{
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "resultSet is nullptr");
    int32_t count = 0;
    CHECK_AND_RETURN_RET_LOG(resultSet->GetRowCount(count) == NativeRdb::E_OK, E_HAS_DB_ERROR,
        "Cannot get row count of resultset");
    CHECK_AND_RETURN_RET_LOG(count > 0, E_HAS_DB_ERROR, "ResultSet count is %{public}d", count);

    PhotoAlbumVector.reserve(count);
    for (int32_t i = 0; i < count; i++) {
        CHECK_AND_RETURN_RET_LOG(
            resultSet->GoToNextRow() == NativeRdb::E_OK, E_HAS_DB_ERROR, "Failed to go to next row");

        PhotoAlbumVector.push_back(make_shared<PhotoAlbum>());
        PhotoAlbumVector.back()->SetPhotoAlbumType(
            static_cast<PhotoAlbumType>(GetInt32Val(PhotoAlbumColumns::ALBUM_TYPE, resultSet)));
        PhotoAlbumVector.back()->SetPhotoAlbumSubType(
            static_cast<PhotoAlbumSubType>(GetInt32Val(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet)));
        PhotoAlbumVector.back()->SetAlbumName(GetStringVal(PhotoAlbumColumns::ALBUM_NAME, resultSet));
        PhotoAlbumVector.back()->SetDateModified(GetInt64Val(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, resultSet));
        PhotoAlbumVector.back()->SetContainsHidden(GetInt32Val(PhotoAlbumColumns::CONTAINS_HIDDEN, resultSet));
        PhotoAlbumVector.back()->SetOrder(GetInt32Val(PhotoAlbumColumns::ALBUM_ORDER, resultSet));
        PhotoAlbumVector.back()->SetBundleName(GetStringVal(PhotoAlbumColumns::ALBUM_BUNDLE_NAME, resultSet));
        PhotoAlbumVector.back()->SetLocalLanguage(GetStringVal(PhotoAlbumColumns::ALBUM_LOCAL_LANGUAGE, resultSet));
        PhotoAlbumVector.back()->SetDateAdded(GetInt64Val(PhotoAlbumColumns::ALBUM_DATE_ADDED, resultSet));
        PhotoAlbumVector.back()->SetIsLocal(GetInt32Val(PhotoAlbumColumns::ALBUM_IS_LOCAL, resultSet));
        PhotoAlbumVector.back()->SetLPath(GetStringVal(PhotoAlbumColumns::ALBUM_LPATH, resultSet));
        PhotoAlbumVector.back()->SetPriority(GetInt32Val(PhotoAlbumColumns::ALBUM_PRIORITY, resultSet));
        PhotoAlbumVector.back()->SetAlbumId(GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet));
    }
    return E_OK;
}

int32_t MediaLibraryAssetOperations::CheckExist(const std::string &path)
{
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    vector<string> columns = {MediaColumn::MEDIA_FILE_PATH};

    MEDIA_DEBUG_LOG("query media_file_path=%{public}s start\n", DfxUtils::GetSafePath(path).c_str());
    predicates.EqualTo(PhotoColumn::MEDIA_FILE_PATH, path);

    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "MediaLibraryPhotoOperations error\n");

    int rowCount = 0;
    if (resultSet->GetRowCount(rowCount) != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("GetRowCount error\n");
        return E_HAS_DB_ERROR;
    }

    MEDIA_INFO_LOG("query media_file_path end, rowCount=%{public}d\n", rowCount);
    return (rowCount > 0) ? E_OK : E_FAIL;
}

std::vector<std::string> MediaLibraryAssetOperations::QueryPhotosTableColumnInfo()
{
    MEDIA_DEBUG_LOG("QueryPhotosTableColumnInfo");
    std::vector<std::string> columnInfo;
    std::shared_ptr<MediaLibraryRdbStore> rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore == nullptr");
        return columnInfo;
    }

    std::string querySql = "SELECT name FROM pragma_table_info('" + PhotoColumn::PHOTOS_TABLE + "')";
    std::vector<std::string> sqlArgs;
    auto resultSet = rdbStore->QuerySql(querySql, sqlArgs);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultSet is nullptr");
        return columnInfo;
    }

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string columnName = GetStringVal("name", resultSet);
        if (GetFileAssetMemberMap().count(columnName) == 0) {
            MEDIA_WARN_LOG("FILEASSET_MEMBER_MAP not find column: %{public}s", columnName.c_str());
            continue;
        }
        columnInfo.emplace_back(columnName);
    }

    return columnInfo;
}

int32_t MediaLibraryAssetOperations::QueryTotalPhoto(vector<shared_ptr<FileAsset>> &fileAssetVector,
    int32_t batchSize)
{
    MEDIA_INFO_LOG("query total photo start\n");
    std::vector<std::string> columnInfo = QueryPhotosTableColumnInfo();
    if (columnInfo.empty()) {
        MEDIA_ERR_LOG("QueryPhotosTableColumnInfo failed");
        return E_ERR;
    }

    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.BeginWrap()->EqualTo(PhotoColumn::PHOTO_METADATA_FLAGS, static_cast<int>(MetadataFlags::TYPE_NEW))->Or()
        ->EqualTo(PhotoColumn::PHOTO_METADATA_FLAGS, static_cast<int>(MetadataFlags::TYPE_DIRTY))->Or()
        ->IsNull(PhotoColumn::PHOTO_METADATA_FLAGS)->EndWrap();
    predicates.And()->BeginWrap()->EqualTo(PhotoColumn::PHOTO_POSITION, "1")->Or()
        ->EqualTo(PhotoColumn::PHOTO_POSITION, "3")->EndWrap();
    predicates.OrderByAsc(PhotoColumn::PHOTO_METADATA_FLAGS);
    predicates.Limit(0, batchSize);
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columnInfo);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("MediaLibraryPhotoOperations error\n");
        return E_OK;
    }

    GetAssetVectorFromResultSet(resultSet, columnInfo, fileAssetVector);

    MEDIA_INFO_LOG("query total photo end\n");
    return E_OK;
}

std::shared_ptr<FileAsset> MediaLibraryAssetOperations::QuerySinglePhoto(int32_t rowId)
{
    std::vector<std::string> columnInfo = QueryPhotosTableColumnInfo();
    if (columnInfo.empty()) {
        MEDIA_ERR_LOG("QueryPhotosTableColumnInfo failed");
        return nullptr;
    }

    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, rowId);
    predicates.NotEqualTo(PhotoColumn::PHOTO_FILE_SOURCE_TYPE,
        to_string(static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)));
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columnInfo);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("MediaLibraryPhotoOperations error\n");
        return nullptr;
    }

    return GetAssetFromResultSet(resultSet, columnInfo);
}

int32_t MediaLibraryAssetOperations::QueryTotalAlbum(vector<shared_ptr<PhotoAlbum>> &photoAlbumVector)
{
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    vector<string> columns = {PhotoAlbumColumns::ALBUM_ID,
        PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumColumns::ALBUM_SUBTYPE,
        PhotoAlbumColumns::ALBUM_NAME, PhotoAlbumColumns::ALBUM_DATE_MODIFIED,
        PhotoAlbumColumns::CONTAINS_HIDDEN, PhotoAlbumColumns::ALBUM_ORDER,
        PhotoAlbumColumns::ALBUM_BUNDLE_NAME, PhotoAlbumColumns::ALBUM_LOCAL_LANGUAGE,
        PhotoAlbumColumns::ALBUM_IS_LOCAL, PhotoAlbumColumns::ALBUM_DATE_ADDED,
        PhotoAlbumColumns::ALBUM_LPATH, PhotoAlbumColumns::ALBUM_PRIORITY};

    MEDIA_INFO_LOG("Start query total photo album");
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query failed, resultSet is null");
        return E_INVALID_ARGUMENTS;
    }

    return GetAlbumVectorFromResultSet(resultSet, columns, photoAlbumVector);
}

shared_ptr<FileAsset> MediaLibraryAssetOperations::GetFileAssetFromDb(const string &column,
    const string &value, OperationObject oprnObject, const vector<string> &columns, const string &networkId)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryAssetOperations::GetFileAssetFromDb");
    if (!CheckOprnObject(oprnObject) || column.empty() || value.empty()) {
        return nullptr;
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return nullptr;
    }

    MediaLibraryCommand cmd(oprnObject, OperationType::QUERY, networkId);
    cmd.GetAbsRdbPredicates()->EqualTo(column, value);

    auto resultSet = rdbStore->Query(cmd, columns);
    if (resultSet == nullptr) {
        return nullptr;
    }
    return GetAssetFromResultSet(resultSet, columns);
}

static shared_ptr<NativeRdb::ResultSet> QueryByPredicates(AbsPredicates &predicates,
    OperationObject oprnObject, const vector<string> &columns, const string &networkId)
{
    if (!CheckOprnObject(oprnObject)) {
        return nullptr;
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return nullptr;
    }

    MediaLibraryCommand cmd(oprnObject, OperationType::QUERY, networkId);
    cmd.GetAbsRdbPredicates()->SetWhereClause(predicates.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(predicates.GetWhereArgs());
    cmd.GetAbsRdbPredicates()->SetOrder(predicates.GetOrder());
    return rdbStore->Query(cmd, columns);
}

shared_ptr<FileAsset> MediaLibraryAssetOperations::GetFileAssetFromDb(AbsPredicates &predicates,
    OperationObject oprnObject, const vector<string> &columns, const string &networkId)
{
    auto resultSet = QueryByPredicates(predicates, oprnObject, columns, networkId);
    if (resultSet == nullptr) {
        return nullptr;
    }
    return GetAssetFromResultSet(resultSet, columns);
}

int32_t MediaLibraryAssetOperations::GetFileAssetVectorFromDb(AbsPredicates &predicates, OperationObject oprnObject,
    vector<shared_ptr<FileAsset>> &fileAssetVector, const vector<string> &columns, const string &networkId)
{
    auto resultSet = QueryByPredicates(predicates, oprnObject, columns, networkId);
    if (resultSet == nullptr) {
        return E_HAS_DB_ERROR;
    }
    return GetAssetVectorFromResultSet(resultSet, columns, fileAssetVector);
}

shared_ptr<FileAsset> MediaLibraryAssetOperations::GetFileAssetByUri(const string &uri, bool isPhoto,
    const std::vector<std::string> &columns, const string &pendingStatus)
{
    if (uri.empty()) {
        MEDIA_ERR_LOG("fileUri is empty");
        return nullptr;
    }

    string id = MediaFileUtils::GetIdFromUri(uri);
    if (uri.empty() || (!MediaLibraryDataManagerUtils::IsNumber(id))) {
        return nullptr;
    }
    shared_ptr<FileAsset> fileAsset = make_shared<FileAsset>();
    MediaFileUri fileUri(uri);
    if (pendingStatus.empty() || !fileUri.IsApi10()) {
        if (isPhoto) {
            fileAsset = GetFileAssetFromDb(MediaColumn::MEDIA_ID, id, OperationObject::FILESYSTEM_PHOTO, columns);
        } else {
            fileAsset = GetFileAssetFromDb(MediaColumn::MEDIA_ID, id, OperationObject::FILESYSTEM_AUDIO, columns);
        }
    } else {
        string path = MediaFileUri::GetPathFromUri(uri, isPhoto);
        if (path.empty()) {
            if (isPhoto) {
                fileAsset = GetFileAssetFromDb(MediaColumn::MEDIA_ID, id, OperationObject::FILESYSTEM_PHOTO, columns);
            } else {
                fileAsset = GetFileAssetFromDb(MediaColumn::MEDIA_ID, id, OperationObject::FILESYSTEM_AUDIO, columns);
            }
        } else {
            if (isPhoto) {
                fileAsset = GetFileAssetFromDb(MediaColumn::MEDIA_ID, id, OperationObject::FILESYSTEM_PHOTO,
                PHOTO_COLUMN_VECTOR);
            }
            CHECK_AND_RETURN_RET(fileAsset != nullptr, nullptr);
            fileAsset->SetPath(path);
            fileAsset->SetMediaType(MediaFileUtils::GetMediaType(path));
            if (MediaFileUtils::IsValidInteger(pendingStatus)) {
                int32_t timePending = stoi(pendingStatus);
                fileAsset->SetTimePending((timePending > 0) ? MediaFileUtils::UTCTimeSeconds() : timePending);
            }
        }
    }

    CHECK_AND_RETURN_RET(fileAsset != nullptr, nullptr);
    if (!isPhoto) {
        fileAsset->SetMediaType(MediaType::MEDIA_TYPE_AUDIO);
    }
    if (MediaFileUtils::IsValidInteger(id)) {
        fileAsset->SetId(stoi(id));
    }
    fileAsset->SetUri(uri);
    return fileAsset;
}

static inline string GetVirtualPath(const string &relativePath, const string &displayName)
{
    if (relativePath[relativePath.size() - 1] != SLASH_CHAR) {
        return relativePath + SLASH_CHAR + displayName;
    } else {
        return relativePath + displayName;
    }
}

static string GetAssetPackageName(const FileAsset &fileAsset, const string &bundleName)
{
    if (fileAsset.GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::SCREENSHOT)) {
        if (fileAsset.GetMediaType() == static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE) ||
            fileAsset.GetMediaType() == static_cast<int32_t>(MediaType::MEDIA_TYPE_PHOTO)) {
            return "截图";
        } else if (fileAsset.GetMediaType() == static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO)) {
            return "屏幕录制";
        }
    }
    return PermissionUtils::GetPackageNameByBundleName(bundleName);
}

static void HandleDateAdded(const int64_t dateAdded, const MediaType type, ValuesBucket &outValues)
{
    outValues.PutLong(MediaColumn::MEDIA_DATE_ADDED, dateAdded);
    if (type != MEDIA_TYPE_PHOTO) {
        return;
    }
    outValues.PutString(PhotoColumn::PHOTO_DATE_YEAR,
        MediaFileUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DATE_YEAR_FORMAT, dateAdded));
    outValues.PutString(PhotoColumn::PHOTO_DATE_MONTH,
        MediaFileUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DATE_MONTH_FORMAT, dateAdded));
    outValues.PutString(PhotoColumn::PHOTO_DATE_DAY,
        MediaFileUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DATE_DAY_FORMAT, dateAdded));
    outValues.PutLong(MediaColumn::MEDIA_DATE_TAKEN, dateAdded);
}

static void HandleOwnerAlbumId(MediaLibraryCommand &cmd, ValuesBucket &outValues)
{
    string ownerAlbumId;
    ValueObject valueOwnerAlbumId;
    if (cmd.GetValueBucket().GetObject(PhotoColumn::PHOTO_OWNER_ALBUM_ID, valueOwnerAlbumId)) {
        valueOwnerAlbumId.GetString(ownerAlbumId);
    }
    if (!ownerAlbumId.empty()) {
        outValues.PutString(PhotoColumn::PHOTO_OWNER_ALBUM_ID, ownerAlbumId);
        MEDIA_INFO_LOG("insert ownerAlbumId: %{public}s", ownerAlbumId.c_str());
    }
}

static void HandleCallingPackage(MediaLibraryCommand &cmd, const FileAsset &fileAsset, ValuesBucket &outValues)
{
    if (!fileAsset.GetOwnerPackage().empty() && PermissionUtils::IsNativeSAApp()) {
        outValues.PutString(MediaColumn::MEDIA_OWNER_PACKAGE, fileAsset.GetOwnerPackage());

        int32_t callingUid = 0;
        ValueObject value;
        if (cmd.GetValueBucket().GetObject(MEDIA_DATA_CALLING_UID, value)) {
            value.GetInt(callingUid);
        }
        outValues.PutString(MediaColumn::MEDIA_OWNER_APPID,
            PermissionUtils::GetAppIdByBundleName(fileAsset.GetOwnerPackage(), callingUid));
        outValues.PutString(MediaColumn::MEDIA_PACKAGE_NAME, fileAsset.GetPackageName());
        return;
    }

    string bundleName;
    ValueObject valueBundleName;
    if (cmd.GetValueBucket().GetObject(MEDIA_DATA_DB_OWNER_PACKAGE, valueBundleName)) {
        valueBundleName.GetString(bundleName);
    }
    if (bundleName.empty()) {
        bundleName = cmd.GetBundleName();
    }
    outValues.PutString(MediaColumn::MEDIA_OWNER_PACKAGE, bundleName);

    string appId;
    ValueObject valueAppId;
    if (cmd.GetValueBucket().GetObject(MEDIA_DATA_DB_OWNER_APPID, valueAppId)) {
        valueAppId.GetString(appId);
    }
    if (appId.empty()) {
        appId = PermissionUtils::GetAppIdByBundleName(cmd.GetBundleName());
    }
    outValues.PutString(MediaColumn::MEDIA_OWNER_APPID, appId);
    string packageName;
    ValueObject valuePackageName;
    if (cmd.GetValueBucket().GetObject(MEDIA_DATA_DB_PACKAGE_NAME, valuePackageName)) {
        valuePackageName.GetString(packageName);
    }
    if (packageName.empty() && !cmd.GetBundleName().empty()) {
        packageName = GetAssetPackageName(fileAsset, cmd.GetBundleName());
    }
    if (!packageName.empty()) {
        outValues.PutString(MediaColumn::MEDIA_PACKAGE_NAME, packageName);
    }
    HandleOwnerAlbumId(cmd, outValues);
}

static void HandleBurstPhoto(MediaLibraryCommand &cmd, ValuesBucket &outValues, const std::string displayName)
{
#ifdef MEDIA_NATIVE_SA_APP_TEST
    CHECK_AND_RETURN_LOG(PermissionUtils::IsNativeSAApp(),
        "do not have permission to set burst_key or burst_cover_level");
#endif

    string burstKey;
    ValueObject value;
    if (cmd.GetValueBucket().GetObject(PhotoColumn::PHOTO_BURST_KEY, value)) {
        value.GetString(burstKey);
    }
    if (!burstKey.empty()) {
        outValues.PutString(PhotoColumn::PHOTO_BURST_KEY, burstKey);
    }

    int32_t burstCoverLevel = 0;
    if (cmd.GetValueBucket().GetObject(PhotoColumn::PHOTO_BURST_COVER_LEVEL, value)) {
        value.GetInt(burstCoverLevel);
    }
    if (burstCoverLevel != 0) {
        outValues.PutInt(PhotoColumn::PHOTO_BURST_COVER_LEVEL, burstCoverLevel);
    }

    int32_t dirty = static_cast<int32_t>(DirtyTypes::TYPE_NEW);
    if (cmd.GetValueBucket().GetObject(PhotoColumn::PHOTO_DIRTY, value)) {
        value.GetInt(dirty);
    }
    if (dirty != static_cast<int32_t>(DirtyTypes::TYPE_NEW)) {
        outValues.PutInt(PhotoColumn::PHOTO_DIRTY, dirty);
    }
    stringstream result;
    for (size_t i = 0; i < displayName.length(); i++) {
        if (isdigit(displayName[i])) {
            result << displayName[i];
        }
    }
    outValues.Put(PhotoColumn::PHOTO_ID, result.str());
    outValues.PutInt(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(MultiStagesPhotoQuality::FULL));
}

static void ExtractHandlePhotoInfo(MediaLibraryCommand &cmd,
    ValuesBucket &outValues, const FileAsset &fileAsset)
{
    ValueObject value;

    int32_t stageVideoTaskStatus = UNKNOWN_VALUE;
    if (cmd.GetValueBucket().GetObject(PhotoColumn::STAGE_VIDEO_TASK_STATUS, value)) {
        value.GetInt(stageVideoTaskStatus);
    }
    if (stageVideoTaskStatus != UNKNOWN_VALUE && \
        fileAsset.GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
        outValues.PutInt(PhotoColumn::STAGE_VIDEO_TASK_STATUS, stageVideoTaskStatus);
    }
}

static void UpdateEnhanceParam(MediaLibraryCommand &cmd, ValuesBucket &outValues, ValueObject &value)
{
    int32_t ceAvailable = static_cast<int32_t>(CloudEnhancementAvailableType::NOT_SUPPORT);
    if (cmd.GetValueBucket().GetObject(PhotoColumn::PHOTO_CE_AVAILABLE, value)) {
        value.GetInt(ceAvailable);
        MEDIA_INFO_LOG("set ce_available: %{public}d", ceAvailable);
    }
    outValues.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE, ceAvailable);

    int32_t isAuto = static_cast<int32_t>(CloudEnhancementIsAutoType::NOT_AUTO);
    if (cmd.GetValueBucket().GetObject(PhotoColumn::PHOTO_IS_AUTO, value)) {
        value.GetInt(isAuto);
        MEDIA_INFO_LOG("set is_auto: %{public}d", isAuto);
    }
    outValues.PutInt(PhotoColumn::PHOTO_IS_AUTO, isAuto);
}

static void HandlePhotoInfo(MediaLibraryCommand &cmd, ValuesBucket &outValues, const FileAsset &fileAsset)
{
#ifdef MEDIA_NATIVE_SA_APP_TEST
    if (!PermissionUtils::IsNativeSAApp()) {
        MEDIA_DEBUG_LOG("do not have permission to set is_temp");
        return;
    }
#endif

    ValueObject value;
    bool isTemp = 0;
    if (cmd.GetValueBucket().GetObject(PhotoColumn::PHOTO_IS_TEMP, value)) {
        value.GetBool(isTemp);
    }
    outValues.PutBool(PhotoColumn::PHOTO_IS_TEMP, isTemp);

    int32_t deferredProcType = UNKNOWN_VALUE;
    if (cmd.GetValueBucket().GetObject(PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, value)) {
        value.GetInt(deferredProcType);
    }
    if (deferredProcType != UNKNOWN_VALUE) {
        outValues.PutInt(PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, deferredProcType);
    }

    // quality、photoId、dirty for burst has been handled in HandleBurstPhoto
    if (fileAsset.GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::BURST)) {
        return;
    }

    int32_t photoQuality = UNKNOWN_VALUE;
    if (cmd.GetValueBucket().GetObject(PhotoColumn::PHOTO_QUALITY, value)) {
        value.GetInt(photoQuality);
    }
    if (photoQuality != UNKNOWN_VALUE) {
        outValues.PutInt(PhotoColumn::PHOTO_QUALITY, photoQuality);
    }
    if (photoQuality == static_cast<int32_t>(MultiStagesPhotoQuality::LOW)) {
        outValues.PutInt(PhotoColumn::PHOTO_DIRTY, -1); // prevent uploading low-quality photo
    }

    std::string photoId;
    if (cmd.GetValueBucket().GetObject(PhotoColumn::PHOTO_ID, value)) {
        value.GetString(photoId);
    }
    if (!photoId.empty()) {
        outValues.PutString(PhotoColumn::PHOTO_ID, photoId);
    }

    UpdateEnhanceParam(cmd, outValues, value);

    ExtractHandlePhotoInfo(cmd, outValues, fileAsset);
}

static void FillAssetInfo(MediaLibraryCommand &cmd, const FileAsset &fileAsset)
{
    // Fill basic file information into DB
    const string& displayName = fileAsset.GetDisplayName();
    int64_t nowTime = MediaFileUtils::UTCTimeMilliSeconds();
    ValuesBucket assetInfo;
    assetInfo.PutInt(MediaColumn::MEDIA_TYPE, fileAsset.GetMediaType());
    string extension = ScannerUtils::GetFileExtension(displayName);
    assetInfo.PutString(MediaColumn::MEDIA_MIME_TYPE,
        MimeTypeUtils::GetMimeTypeFromExtension(extension));
    assetInfo.PutString(MediaColumn::MEDIA_FILE_PATH, fileAsset.GetPath());
    if (cmd.GetApi() == MediaLibraryApi::API_OLD) {
        assetInfo.PutString(MediaColumn::MEDIA_RELATIVE_PATH, fileAsset.GetRelativePath());
        assetInfo.PutString(MediaColumn::MEDIA_VIRTURL_PATH,
            GetVirtualPath(fileAsset.GetRelativePath(), fileAsset.GetDisplayName()));
    } else {
        assetInfo.PutLong(MediaColumn::MEDIA_TIME_PENDING, fileAsset.GetTimePending());
    }
    assetInfo.PutString(MediaColumn::MEDIA_NAME, displayName);
    assetInfo.PutString(MediaColumn::MEDIA_TITLE, MediaFileUtils::GetTitleFromDisplayName(displayName));
    if (cmd.GetOprnObject() == OperationObject::FILESYSTEM_PHOTO) {
        assetInfo.PutString(PhotoColumn::PHOTO_MEDIA_SUFFIX, extension);
        assetInfo.PutInt(PhotoColumn::PHOTO_SUBTYPE, fileAsset.GetPhotoSubType());
        assetInfo.PutString(PhotoColumn::CAMERA_SHOT_KEY, fileAsset.GetCameraShotKey());
        HandlePhotoInfo(cmd, assetInfo, fileAsset);
        if (fileAsset.GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::BURST)) {
            HandleBurstPhoto(cmd, assetInfo, displayName);
        }
    }
    assetInfo.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, fileAsset.GetFileResourceType());
    if (fileAsset.GetFileResourceType() == static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)) {
        assetInfo.PutInt(PhotoColumn::PHOTO_DIRTY, -1);
    }

    HandleCallingPackage(cmd, fileAsset, assetInfo);

    assetInfo.PutString(MediaColumn::MEDIA_DEVICE_NAME, cmd.GetDeviceName());
    HandleDateAdded(nowTime,
        cmd.GetOprnObject() == OperationObject::FILESYSTEM_PHOTO ? MEDIA_TYPE_PHOTO : MEDIA_TYPE_DEFAULT,
        assetInfo);
    cmd.SetValueBucket(assetInfo);
}

static ValuesBucket GetOwnerPermissionBucket(MediaLibraryCommand &cmd, int64_t fileId, int32_t callingUid)
{
    int64_t tokenId = 0;
    string tokenIdFromClient = cmd.GetQuerySetParam("tokenId");
    tokenId = static_cast<int64_t>(std::atoi(tokenIdFromClient.c_str()));
    if (tokenId == 0) {
        if (callingUid > 0 && PermissionUtils::IsNativeSAApp()) {
            string bundleName;
            PermissionUtils::GetClientBundle(callingUid, bundleName);
            string appId = PermissionUtils::GetAppIdByBundleName(bundleName, callingUid);
            PermissionUtils::GetMainTokenId(appId, tokenId);
        }
    }
    if (tokenId == 0) {
        tokenId = PermissionUtils::GetTokenId();
    }
    string tableName = cmd.GetTableName();
    TableType mediaType;
    if (tableName == PhotoColumn::PHOTOS_TABLE) {
        mediaType = TableType::TYPE_PHOTOS;
    } else {
        mediaType = TableType::TYPE_AUDIOS;
    }
    ValuesBucket valuesBucket;
    valuesBucket.Put(AppUriPermissionColumn::FILE_ID, static_cast<int32_t>(fileId));
    valuesBucket.Put(AppUriPermissionColumn::URI_TYPE, static_cast<int32_t>(mediaType));
    valuesBucket.Put(AppUriPermissionColumn::PERMISSION_TYPE,
        AppUriPermissionColumn::PERMISSION_PERSIST_READ_WRITE);
    valuesBucket.Put(AppUriPermissionColumn::TARGET_TOKENID, (int64_t)tokenId);
    valuesBucket.Put(AppUriPermissionColumn::SOURCE_TOKENID, (int64_t)tokenId);
    valuesBucket.Put(AppUriPermissionColumn::DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
    return valuesBucket;
}

int32_t MediaLibraryAssetOperations::InsertAssetInDb(std::shared_ptr<TransactionOperations> trans,
    MediaLibraryCommand &cmd, const FileAsset &fileAsset,
    shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh)
{
    // All values inserted in this function are the base property for files
    if (trans == nullptr) {
        return E_HAS_DB_ERROR;
    }

    if (!fileAsset.GetPath().empty() && MediaFileUtils::IsFileExists(fileAsset.GetPath())) {
        MEDIA_ERR_LOG("file %{private}s exists now", fileAsset.GetPath().c_str());
        return E_FILE_EXIST;
    }
    int32_t callingUid = 0;
    ValueObject value;
    if (cmd.GetValueBucket().GetObject(MEDIA_DATA_CALLING_UID, value)) {
        value.GetInt(callingUid);
    }
    FillAssetInfo(cmd, fileAsset);

    int64_t outRowId = -1;
    int32_t ret = NativeRdb::E_OK;
    if (assetRefresh) {
        ret = assetRefresh->Insert(cmd, outRowId);
    } else {
        ret = trans->Insert(cmd, outRowId);
    }
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Insert into db failed, ret = %{public}d", ret);
        return E_HAS_DB_ERROR;
    }
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} insert success, rowId = %{public}d",
        MLOG_TAG, __FUNCTION__, __LINE__, (int)outRowId);
    auto fileId = outRowId;
    ValuesBucket valuesBucket = GetOwnerPermissionBucket(cmd, fileId, callingUid);
    int64_t tmpOutRowId = -1;
    MediaLibraryCommand cmdPermission(Uri(MEDIALIBRARY_GRANT_URIPERM_URI), valuesBucket);
    auto errCode = trans->Insert(cmdPermission, tmpOutRowId);
    if (errCode != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Insert into db failed, errCode = %{public}d", errCode);
        return E_HAS_DB_ERROR;
    }
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} insert uripermission success, rowId = %{public}d",
        MLOG_TAG, __FUNCTION__, __LINE__, (int)tmpOutRowId);
    return static_cast<int32_t>(outRowId);
}

static bool CheckTypeFromRootDir(const std::string &rootDirName, int32_t type)
{
    // "Camera/"
    if (!strcmp(rootDirName.c_str(), CAMERA_DIR_VALUES.c_str())) {
        if (type == MEDIA_TYPE_IMAGE || type == MEDIA_TYPE_VIDEO) {
            return true;
        }
    }
    // "Videos/"
    if (!strcmp(rootDirName.c_str(), VIDEO_DIR_VALUES.c_str())) {
        if (type == MEDIA_TYPE_VIDEO) {
            return true;
        }
    }
    // "Pictures/"
    if (!strcmp(rootDirName.c_str(), PIC_DIR_VALUES.c_str())) {
        if (type == MEDIA_TYPE_IMAGE) {
            return true;
        }
    }
    // "Audios/"
    if (!strcmp(rootDirName.c_str(), AUDIO_DIR_VALUES.c_str())) {
        if (type == MEDIA_TYPE_AUDIO) {
            return true;
        }
    }
    // "Docs/Documents/" and "Docs/Download"
    if (!strcmp(rootDirName.c_str(), DOCS_PATH.c_str())) {
        return true;
    }
    MEDIA_ERR_LOG("Cannot match rootDir %{private}s and mediaType %{public}d",
        rootDirName.c_str(), type);
    return false;
}

int32_t MediaLibraryAssetOperations::CheckWithType(bool isContains, const string &displayName,
    const string &extention, int32_t mediaType)
{
    string name = isContains ? displayName : extention;
    int32_t errCode =  isContains ? CheckDisplayNameWithType(name, mediaType) : CheckExtWithType(name, mediaType);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode,
        "Failed to Check Dir and extention, (displayName or extention)=%{private}s, mediaType=%{public}d",
        name.c_str(), mediaType);
    return errCode;
}

int32_t MediaLibraryAssetOperations::CheckDisplayNameWithType(const string &displayName, int32_t mediaType)
{
    int32_t ret = MediaFileUtils::CheckDisplayName(displayName);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_INVALID_DISPLAY_NAME, "Check DisplayName failed, "
        "displayName=%{private}s", displayName.c_str());

    string ext = MediaFileUtils::GetExtensionFromPath(displayName);
    CHECK_AND_RETURN_RET_LOG(!ext.empty(), E_INVALID_DISPLAY_NAME, "invalid extension, displayName=%{private}s",
        displayName.c_str());

    auto typeFromExt = MediaFileUtils::GetMediaType(displayName);
    CHECK_AND_RETURN_RET_LOG(typeFromExt == mediaType, E_CHECK_MEDIATYPE_MATCH_EXTENSION_FAIL,
        "cannot match, mediaType=%{public}d, ext=%{private}s, type from ext=%{public}d",
        mediaType, ext.c_str(), typeFromExt);
    return E_OK;
}

int32_t MediaLibraryAssetOperations::CheckExtWithType(const string &extention, int32_t mediaType)
{
    string mimeType = MimeTypeUtils::GetMimeTypeFromExtension(extention);
    auto typeFromExt = MimeTypeUtils::GetMediaTypeFromMimeType(mimeType);
    CHECK_AND_RETURN_RET_LOG(typeFromExt == mediaType, E_CHECK_MEDIATYPE_MATCH_EXTENSION_FAIL,
        "cannot match, mediaType=%{public}d, ext=%{public}s, type from ext=%{public}d",
        mediaType, extention.c_str(), typeFromExt);
    return E_OK;
}

int32_t MediaLibraryAssetOperations::CheckRelativePathWithType(const string &relativePath, int32_t mediaType)
{
    int32_t ret = MediaFileUtils::CheckRelativePath(relativePath);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_INVALID_PATH, "Check relativePath failed, "
        "relativePath=%{private}s", relativePath.c_str());

    // get rootdir and check if it match mediatype
    string rootDirName;
    MediaFileUtils::GetRootDirFromRelativePath(relativePath, rootDirName);
    CHECK_AND_RETURN_RET_LOG(!rootDirName.empty(), E_INVALID_PATH, "Cannot get rootdirName");

    bool isValid = CheckTypeFromRootDir(rootDirName, mediaType);
    CHECK_AND_RETURN_RET(isValid, E_CHECK_MEDIATYPE_FAIL);
    return E_OK;
}

void MediaLibraryAssetOperations::GetAssetRootDir(int32_t mediaType, string &rootDirPath)
{
    map<int, string> rootDir = {
        { MEDIA_TYPE_FILE, DOCUMENT_BUCKET + SLASH_CHAR },
        { MEDIA_TYPE_VIDEO, PHOTO_BUCKET + SLASH_CHAR },
        { MEDIA_TYPE_IMAGE, PHOTO_BUCKET + SLASH_CHAR },
        { MEDIA_TYPE_AUDIO, AUDIO_BUCKET + SLASH_CHAR },
    };
    if (rootDir.count(mediaType) == 0) {
        rootDirPath = rootDir[MEDIA_TYPE_FILE];
    } else {
        rootDirPath = rootDir[mediaType];
    }
}

int32_t MediaLibraryAssetOperations::SetAssetPathInCreate(FileAsset &fileAsset,
    std::shared_ptr<TransactionOperations> trans)
{
    if (!fileAsset.GetPath().empty()) {
        return E_OK;
    }
    string extension = MediaFileUtils::GetExtensionFromPath(fileAsset.GetDisplayName());
    string filePath;
    int32_t uniqueId = CreateAssetUniqueId(fileAsset.GetMediaType(), trans);
    int32_t errCode = CreateAssetPathById(uniqueId, fileAsset.GetMediaType(), extension, filePath);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("Create Asset Path failed, errCode=%{public}d", errCode);
        return errCode;
    }

    // filePath can not be empty
    fileAsset.SetPath(filePath);
    return E_OK;
}

int32_t MediaLibraryAssetOperations::SetAssetPath(FileAsset &fileAsset, const string &extension,
    std::shared_ptr<TransactionOperations> trans)
{
    string filePath;
    int32_t uniqueId = CreateAssetUniqueId(fileAsset.GetMediaType(), trans);
    int32_t errCode = CreateAssetPathById(uniqueId, fileAsset.GetMediaType(), extension, filePath);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("Create Asset Path failed, errCode=%{public}d", errCode);
        return errCode;
    }

    // filePath can not be empty
    fileAsset.SetPath(filePath);
    string fileName = MediaFileUtils::GetFileName(filePath);
    string displayName = fileName.substr(0, fileName.find('_')) + '_' + fileName.substr(fileName.rfind('_') + 1);
    fileAsset.SetDisplayName(displayName);
    return E_OK;
}

int32_t MediaLibraryAssetOperations::DeleteAssetInDb(MediaLibraryCommand &cmd,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return E_HAS_DB_ERROR;
    }

    string strDeleteCondition = cmd.GetAbsRdbPredicates()->GetWhereClause();
    if (strDeleteCondition.empty()) {
        string strRow = cmd.GetOprnFileId();
        if (strRow.empty() || !MediaLibraryDataManagerUtils::IsNumber(strRow)) {
            MEDIA_ERR_LOG("MediaLibraryAssetOperations DeleteFile: Index not digit, fileIdStr=%{private}s",
                strRow.c_str());
            return E_INVALID_FILEID;
        }
        cmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, strRow);
    }

    int32_t deletedRows = E_HAS_DB_ERROR;
    int32_t result = -1;
    if (assetRefresh == nullptr) {
        result = rdbStore->Delete(cmd, deletedRows);
    } else {
        result = assetRefresh->LogicalDeleteReplaceByUpdate(cmd, deletedRows);
    }
    if (result != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Delete operation failed. Result %{public}d.", result);
    }

    return deletedRows;
}

int32_t MediaLibraryAssetOperations::UpdateFileName(MediaLibraryCommand &cmd,
    const shared_ptr<FileAsset> &fileAsset, bool &isNameChanged)
{
    ValuesBucket &values = cmd.GetValueBucket();
    ValueObject valueObject;
    string newTitle;
    string newDisplayName;
    bool containsTitle = false;
    bool containsDisplayName = false;

    if (values.GetObject(MediaColumn::MEDIA_TITLE, valueObject)) {
        valueObject.GetString(newTitle);
        containsTitle = true;
    }
    if (values.GetObject(MediaColumn::MEDIA_NAME, valueObject)) {
        valueObject.GetString(newDisplayName);
        containsDisplayName = true;
    }
    if ((!containsTitle) && (!containsDisplayName)) {
        // do not need to update
        return E_OK;
    }
    if (containsTitle && containsDisplayName &&
        (MediaFileUtils::GetTitleFromDisplayName(newDisplayName) != newTitle)) {
        MEDIA_ERR_LOG("new displayName [%{private}s] and new title [%{private}s] is not same",
            newDisplayName.c_str(), newTitle.c_str());
        return E_INVALID_DISPLAY_NAME;
    }
    if (!containsTitle) {
        newTitle = MediaFileUtils::GetTitleFromDisplayName(newDisplayName);
    }
    if (!containsDisplayName) {
        newDisplayName = newTitle + "." + MediaFileUtils::SplitByChar(fileAsset->GetDisplayName(), '.');
    }

    int32_t ret = CheckDisplayNameWithType(newDisplayName, fileAsset->GetMediaType());
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Input displayName invalid %{private}s", newDisplayName.c_str());
    values.PutString(MediaColumn::MEDIA_TITLE, newTitle);
    values.PutString(MediaColumn::MEDIA_NAME, newDisplayName);
    if (cmd.GetTableName() == PhotoColumn::PHOTOS_TABLE) {
        values.PutString(PhotoColumn::PHOTO_MEDIA_SUFFIX, ScannerUtils::GetFileExtension(newDisplayName));
    }
    isNameChanged = true;
    return E_OK;
}

int32_t MediaLibraryAssetOperations::SetUserComment(MediaLibraryCommand &cmd,
    const shared_ptr<FileAsset> &fileAsset)
{
    ValuesBucket &values = cmd.GetValueBucket();
    ValueObject valueObject;
    string newUserComment;

    if (values.GetObject(PhotoColumn::PHOTO_USER_COMMENT, valueObject)) {
        valueObject.GetString(newUserComment);
    } else {
        return E_OK;
    }

    uint32_t err = 0;
    SourceOptions opts;
    string filePath = fileAsset->GetFilePath();
    string extension = MediaFileUtils::GetExtensionFromPath(filePath);
    opts.formatHint = "image/" + extension;
    std::unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(filePath, opts, err);
    bool cond = (err != 0 || imageSource == nullptr);
    CHECK_AND_RETURN_RET_LOG(!cond, E_OK, "Failed to obtain image source, err = %{public}d", err);

    string userComment;
    err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_USER_COMMENT, userComment);
    CHECK_AND_RETURN_RET_LOG(err == 0, E_OK, "Image does not exist user comment in exif, no need to modify");
    err = imageSource->ModifyImageProperty(0, PHOTO_DATA_IMAGE_USER_COMMENT, newUserComment, filePath);
    CHECK_AND_PRINT_LOG(err == 0, "Modify image property user comment failed");
    TransCodeExifInfo transCodeExifInfo;
    transCodeExifInfo.userComment = newUserComment;
    MediaLibraryTranscodeDataAgingOperation::ModifyTransCodeFileExif(ExifType::EXIF_USER_COMMENT,
        filePath, transCodeExifInfo, __func__);
    return E_OK;
}

int32_t MediaLibraryAssetOperations::UpdateRelativePath(MediaLibraryCommand &cmd,
    const shared_ptr<FileAsset> &fileAsset, bool &isNameChanged)
{
    string newRelativePath;
    ValuesBucket &values = cmd.GetValueBucket();
    ValueObject valueObject;
    if (values.GetObject(MediaColumn::MEDIA_RELATIVE_PATH, valueObject)) {
        valueObject.GetString(newRelativePath);
    } else {
        // relativePath is not modified
        return E_OK;
    }
    MediaFileUtils::FormatRelativePath(newRelativePath);

    if (newRelativePath == fileAsset->GetRelativePath()) {
        // relativepath has not been modified
        return E_OK;
    }

    int32_t errCode = CheckRelativePathWithType(newRelativePath, fileAsset->GetMediaType());
    if (errCode != E_SUCCESS) {
        MEDIA_ERR_LOG("Check RelativePath failed");
        return errCode;
    }
    values.Delete(MediaColumn::MEDIA_RELATIVE_PATH);
    values.PutString(MediaColumn::MEDIA_RELATIVE_PATH, newRelativePath);

    isNameChanged = true;
    return E_OK;
}

void MediaLibraryAssetOperations::UpdateVirtualPath(MediaLibraryCommand &cmd,
    const shared_ptr<FileAsset> &fileAsset)
{
    string relativePath;
    string displayName;
    ValuesBucket &values = cmd.GetValueBucket();
    ValueObject valueObject;

    if (values.GetObject(MediaColumn::MEDIA_NAME, valueObject)) {
        valueObject.GetString(displayName);
    } else {
        displayName = fileAsset->GetDisplayName();
    }

    if (values.GetObject(MediaColumn::MEDIA_RELATIVE_PATH, valueObject)) {
        valueObject.GetString(relativePath);
    } else {
        relativePath = fileAsset->GetRelativePath();
    }

    if (relativePath.back() != '/') {
        relativePath += '/';
    }
    string virtualPath = relativePath + displayName;
    values.PutString(MediaColumn::MEDIA_VIRTURL_PATH, virtualPath);
}

int32_t MediaLibraryAssetOperations::UpdateFileInDb(MediaLibraryCommand &cmd)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return E_HAS_DB_ERROR;
    }

    int32_t updateRows = 0;
    int32_t result = rdbStore->Update(cmd, updateRows);
    if (result != NativeRdb::E_OK || updateRows <= 0) {
        MEDIA_ERR_LOG("Update File failed. Result %{public}d.", result);
        return E_HAS_DB_ERROR;
    }

    return updateRows;
}

int32_t MediaLibraryAssetOperations::OpenFileWithPrivacy(const string &filePath, const string &mode,
    const string &fileId, int32_t type)
{
    MEDIA_DEBUG_LOG("Open with privacy type:%{public}d", type);
    return MediaPrivacyManager(filePath, mode, fileId, type).Open();
}

static int32_t SetPendingTime(const shared_ptr<FileAsset> &fileAsset, int64_t pendingTime)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return E_HAS_DB_ERROR;
    }

    MediaLibraryCommand updatePendingCmd(GetOprnObjectByMediaType(fileAsset->GetMediaType()),
        OperationType::UPDATE);
    updatePendingCmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID,
        to_string(fileAsset->GetId()));
    ValuesBucket values;
    values.PutLong(MediaColumn::MEDIA_TIME_PENDING, pendingTime);
    updatePendingCmd.SetValueBucket(values);
    int32_t rowId = 0;
    int32_t result = rdbStore->Update(updatePendingCmd, rowId);
    if (result != NativeRdb::E_OK || rowId <= 0) {
        MEDIA_ERR_LOG("Update File pending failed. Result %{public}d.", result);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

static int32_t CreateFileAndSetPending(const shared_ptr<FileAsset> &fileAsset, int64_t pendingTime)
{
    int32_t errCode = MediaFileUtils::CreateAsset(fileAsset->GetPath());
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("Create asset failed, path=%{private}s", fileAsset->GetPath().c_str());
        return errCode;
    }

    return SetPendingTime(fileAsset, pendingTime);
}

static int32_t SolvePendingStatus(const shared_ptr<FileAsset> &fileAsset, const string &mode)
{
    int64_t pendingTime = fileAsset->GetTimePending();
    if (pendingTime != 0) {
        if (mode == MEDIA_FILEMODE_READONLY) {
            MEDIA_ERR_LOG("FileAsset [%{private}s] pending status is %{public}ld and open mode is READ_ONLY",
                fileAsset->GetUri().c_str(), (long) pendingTime);
            return E_IS_PENDING_ERROR;
        }
        string networkId = MediaFileUtils::GetNetworkIdFromUri(fileAsset->GetUri());
        if (!networkId.empty()) {
            MEDIA_ERR_LOG("Can not open remote [%{private}s] pending file", networkId.c_str());
            return E_IS_PENDING_ERROR;
        }
        if (pendingTime == UNCREATE_FILE_TIMEPENDING) {
            int32_t errCode = CreateFileAndSetPending(fileAsset, UNCLOSE_FILE_TIMEPENDING);
            return errCode;
        }
        if (pendingTime == UNOPEN_FILE_COMPONENT_TIMEPENDING) {
            int32_t errCode = SetPendingTime(fileAsset, UNCLOSE_FILE_TIMEPENDING);
            return errCode;
        }
    }
    return E_OK;
}

static int32_t CreateDirectoryAndAsset(const string path)
{
    string dir = MediaFileUtils::GetParentPath(path);
    if (!MediaFileUtils::CreateDirectory(dir)) {
        MEDIA_ERR_LOG("Create dir failed, dir=%{private}s", dir.c_str());
        return E_INVALID_VALUES;
    }
    int32_t errCode = MediaFileUtils::CreateAsset(path);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("Create asset failed, path=%{private}s", path.c_str());
        return errCode;
    }
    return E_OK;
}

static int32_t SolveMovingPhotoVideoCreation(const string &imagePath, const string &mode, bool isMovingPhotoVideo)
{
    if (mode == MEDIA_FILEMODE_READONLY || !isMovingPhotoVideo) {
        return E_OK;
    }
    string videoPath = MediaFileUtils::GetMovingPhotoVideoPath(imagePath);
    if (MediaFileUtils::IsFileExists(videoPath)) {
        return E_OK;
    }
    int32_t errCode = MediaFileUtils::CreateAsset(videoPath);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode,
        "Create moving photo asset failed, path=%{private}s", videoPath.c_str());
    return E_OK;
}

static int32_t SolveMoviePhotoVideoCreation(const string &videoPath, const string &mode)
{
    CHECK_AND_RETURN_RET(mode != MEDIA_FILEMODE_READONLY, E_OK);
    CHECK_AND_RETURN_RET_INFO_LOG(!MediaFileUtils::IsFileExists(videoPath), E_OK,
        "videoPath is Exists, videoPath %{private}s", videoPath.c_str());

    int32_t errCode = MediaFileUtils::CreateAsset(videoPath);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode,
        "Create movie video asset failed, path=%{private}s", videoPath.c_str());
    return E_OK;
}

static bool IsNotMusicFile(const std::string &path)
{
    return (path.find(ANALYSIS_FILE_PATH) == string::npos);
}

int32_t MediaLibraryAssetOperations::OpenAsset(const shared_ptr<FileAsset> &fileAsset, const string &mode,
    MediaLibraryApi api, bool isMovingPhotoVideo, int32_t type)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryAssetOperations::OpenAsset");
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INVALID_VALUES, "fileAsset is nullptr");

    string lowerMode = mode;
    transform(lowerMode.begin(), lowerMode.end(), lowerMode.begin(), ::tolower);
    CHECK_AND_RETURN_RET(MediaFileUtils::CheckMode(lowerMode), E_INVALID_MODE);

    string path;
    if (api == MediaLibraryApi::API_10) {
        int32_t errCode = SolvePendingStatus(fileAsset, mode);
        CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode,
            "Solve pending status failed, errCode=%{public}d", errCode);
        path = fileAsset->GetPath();
        MEDIA_DEBUG_LOG("##### file path is %{private}s", path.c_str());
        SolveMovingPhotoVideoCreation(path, mode, isMovingPhotoVideo);
        if (fileAsset->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::CINEMATIC_VIDEO)) {
            SolveMoviePhotoVideoCreation(path, mode);
        }
    } else {
        // If below API10, TIME_PENDING is 0 after asset created, so if file is not exist, create an empty one
        if (!MediaFileUtils::IsFileExists(fileAsset->GetPath())) {
            MEDIA_INFO_LOG("create empty file for %{public}s, path: %{private}s", fileAsset->GetUri().c_str(),
                fileAsset->GetPath().c_str());
            int32_t errCode = CreateDirectoryAndAsset(fileAsset->GetPath());
            CHECK_AND_RETURN_RET(errCode == E_OK, errCode);
        }
        path = MediaFileUtils::UpdatePath(fileAsset->GetPath(), fileAsset->GetUri());
    }

    string fileId = MediaFileUtils::GetIdFromUri(fileAsset->GetUri());

    int32_t fd = OpenFileWithPrivacy(path, lowerMode, fileId, type);
    CHECK_AND_RETURN_RET_LOG(fd >= 0, E_HAS_FS_ERROR,
        "open file, userId: %{public}d, uri: %{public}s, path: %{public}s, fd %{public}d, errno %{public}d",
        fileAsset->GetUserId(), fileAsset->GetUri().c_str(), fileAsset->GetPath().c_str(), fd, errno);

    tracer.Start("AddWatchList");
    if (mode.find(MEDIA_FILEMODE_WRITEONLY) != string::npos && !isMovingPhotoVideo && IsNotMusicFile(path)) {
        auto watch = MediaLibraryInotify::GetInstance();
        if (watch != nullptr) {
            MEDIA_INFO_LOG("enter inotify, path = %{public}s, fileId = %{public}d",
                DfxUtils::GetSafePath(path).c_str(), fileAsset->GetId());
            watch->AddWatchList(path, fileAsset->GetUri(), MediaLibraryApi::API_10);
        }
    }
    tracer.Finish();
    return fd;
}

int32_t MediaLibraryAssetOperations::CloseAsset(const shared_ptr<FileAsset> &fileAsset, bool isCreateThumbSync)
{
    if (fileAsset == nullptr) {
        return E_INVALID_VALUES;
    }

    // remove inotify event since there is close cmd
    auto watch = MediaLibraryInotify::GetInstance();
    if (watch != nullptr) {
        string uri = fileAsset->GetUri();
        watch->RemoveByFileUri(uri, MediaLibraryApi::API_10);
        MEDIA_DEBUG_LOG("watch RemoveByFileUri, uri:%{private}s", uri.c_str());
    }

    string path = fileAsset->GetPath();
    // if pending == 0, scan
    // if pending == UNCREATE_FILE_TIMEPENDING, not occur under normal conditions
    // if pending == UNCLOSE_FILE_TIMEPENDING, set pending = 0 and scan
    // if pending == UNOPEN_FILE_COMPONENT_TIMEPENDING, not allowed to close
    // if pending is timestamp, do nothing
    if (fileAsset->GetTimePending() == 0 || fileAsset->GetTimePending() == UNCLOSE_FILE_TIMEPENDING) {
        if (fileAsset->GetTimePending() == UNCLOSE_FILE_TIMEPENDING) {
            ScanFile(path, isCreateThumbSync, false);
        } else {
            ScanFile(path, isCreateThumbSync, true);
        }
        return E_OK;
    } else if (fileAsset->GetTimePending() == UNCREATE_FILE_TIMEPENDING ||
        fileAsset->GetTimePending() == UNOPEN_FILE_COMPONENT_TIMEPENDING) {
        MEDIA_ERR_LOG("This asset [%{public}d] pending status cannot close", fileAsset->GetId());
        return E_IS_PENDING_ERROR;
    } else if (fileAsset->GetTimePending() > 0) {
        MEDIA_WARN_LOG("This asset [%{public}d] is in pending", fileAsset->GetId());
        return E_OK;
    } else {
        MEDIA_ERR_LOG("This asset [%{public}d] pending status is invalid", fileAsset->GetId());
        return E_INVALID_VALUES;
    }
}

int32_t MediaLibraryAssetOperations::OpenHighlightCover(MediaLibraryCommand &cmd, const string &mode)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryAssetOperations::OpenHighlightCover");
    string uriStr = cmd.GetUriStringWithoutSegment();
    string path = MediaFileUtils::GetHighlightPath(uriStr);
    CHECK_AND_RETURN_RET_LOG(path.length() != 0, E_INVALID_URI,
        "Open highlight cover invalid uri : %{public}s", uriStr.c_str());
    shared_ptr<FileAsset> fileAsset = make_shared<FileAsset>();
    fileAsset->SetPath(path);
    fileAsset->SetUri(uriStr);
    return OpenAsset(fileAsset, mode, cmd.GetApi(), false);
}

int32_t MediaLibraryAssetOperations::OpenHighlightVideo(MediaLibraryCommand &cmd, const string &mode)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryAssetOperations::OpenHighlightVideo");
    string uriStr = cmd.GetUriStringWithoutSegment();
    string path = MediaFileUtils::GetHighlightVideoPath(uriStr);

    CHECK_AND_RETURN_RET_LOG(path.length() != 0, E_INVALID_URI,
        "Open highlight video invalid uri : %{public}s", uriStr.c_str());
    shared_ptr<FileAsset> fileAsset = make_shared<FileAsset>();
    fileAsset->SetPath(path);
    fileAsset->SetUri(uriStr);
    return OpenAsset(fileAsset, mode, cmd.GetApi(), false);
}

void MediaLibraryAssetOperations::InvalidateThumbnail(const string &fileId, int32_t type)
{
    string tableName;
    switch (type) {
        case MediaType::MEDIA_TYPE_IMAGE:
        case MediaType::MEDIA_TYPE_VIDEO: {
            tableName = PhotoColumn::PHOTOS_TABLE;
            break;
        }
        case MediaType::MEDIA_TYPE_AUDIO: {
            tableName = AudioColumn::AUDIOS_TABLE;
            break;
        }
        default: {
            MEDIA_ERR_LOG("Can not match this type %{public}d", type);
            return;
        }
    }
    ThumbnailService::GetInstance()->HasInvalidateThumbnail(fileId, tableName);
}

void MediaLibraryAssetOperations::ScanFile(const string &path, bool isCreateThumbSync, bool isInvalidateThumb,
    bool isForceScan, int32_t fileId, std::shared_ptr<Media::Picture> resultPicture)
{
    // Force Scan means medialibrary will scan file without checking E_SCANNED
    shared_ptr<ScanAssetCallback> scanAssetCallback = make_shared<ScanAssetCallback>();
    if (scanAssetCallback == nullptr) {
        MEDIA_ERR_LOG("Failed to create scan file callback object");
        return;
    }
    scanAssetCallback->SetOriginalPhotoPicture(resultPicture);
    if (isCreateThumbSync) {
        scanAssetCallback->SetSync(true);
    }
    if (!isInvalidateThumb) {
        scanAssetCallback->SetIsInvalidateThumb(false);
    }

    int ret = MediaScannerManager::GetInstance()->ScanFileSync(path, scanAssetCallback, MediaLibraryApi::API_10,
        isForceScan, fileId);
    if (ret != 0) {
        MEDIA_ERR_LOG("Scan file failed with error: %{public}d", ret);
    }
}

void MediaLibraryAssetOperations::ScanFileWithoutAlbumUpdate(const string &path, bool isCreateThumbSync,
    bool isInvalidateThumb, bool isForceScan, int32_t fileId, std::shared_ptr<Media::Picture> resultPicture)
{
    // Force Scan means medialibrary will scan file without checking E_SCANNED
    shared_ptr<ScanAssetCallback> scanAssetCallback = make_shared<ScanAssetCallback>();
    if (scanAssetCallback == nullptr) {
        MEDIA_ERR_LOG("Failed to create scan file callback object");
        return;
    }
    scanAssetCallback->SetOriginalPhotoPicture(resultPicture);
    if (isCreateThumbSync) {
        scanAssetCallback->SetSync(true);
    }
    if (!isInvalidateThumb) {
        scanAssetCallback->SetIsInvalidateThumb(false);
    }

    int ret = MediaScannerManager::GetInstance()->ScanFileSyncWithoutAlbumUpdate(path, scanAssetCallback,
        MediaLibraryApi::API_10, isForceScan, fileId);
    CHECK_AND_PRINT_LOG(ret == 0, "Scan file failed with error: %{public}d", ret);
}

string MediaLibraryAssetOperations::GetEditDataDirPath(const string &path)
{
    if (path.length() < ROOT_MEDIA_DIR.length()) {
        return "";
    }
    return MEDIA_EDIT_DATA_DIR + path.substr(ROOT_MEDIA_DIR.length());
}

string MediaLibraryAssetOperations::GetEditDataSourcePath(const string &path)
{
    string parentPath = GetEditDataDirPath(path);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/source." + MediaFileUtils::GetExtensionFromPath(path);
}

string MediaLibraryAssetOperations::GetEditDataSourceBackPath(const string& path)
{
    string parentPath = GetEditDataDirPath(path);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/source_back." + MediaFileUtils::GetExtensionFromPath(path);
}

string MediaLibraryAssetOperations::GetEditDataPath(const string &path)
{
    string parentPath = GetEditDataDirPath(path);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/editdata";
}

string MediaLibraryAssetOperations::GetEditDataCameraPath(const string &path)
{
    string parentPath = GetEditDataDirPath(path);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/editdata_camera";
}

string MediaLibraryAssetOperations::GetAssetCacheDir()
{
    string cacheOwner = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    if (cacheOwner.empty()) {
        cacheOwner = "common"; // Create cache file in common dir if there is no bundleName.
    }
    return MEDIA_CACHE_DIR + cacheOwner;
}

string MediaLibraryAssetOperations::GetAssetCompressCachePath(const string &id)
{
    return GetAssetCacheDir() + "/compressCache/" + id + ".tlv";
}

string MediaLibraryAssetOperations::GetAssetCompressJsonPath(const string &id)
{
    return GetAssetCacheDir() + "/compressCache/" + id + "_editdata.json";
}

static void UpdateAlbumsAndSendNotifyInTrash(AsyncTaskData *data)
{
    if (data == nullptr) {
        return;
    }
    DeleteNotifyAsyncTaskData* notifyData = static_cast<DeleteNotifyAsyncTaskData*>(data);

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Can not get rdbstore");
        return;
    }
    if (notifyData->refresh_ != nullptr) {
        notifyData->refresh_->RefreshAlbum();
        notifyData->refresh_->Notify();
    } else {
        MediaLibraryRdbUtils::UpdateAllAlbums(rdbStore, {notifyData->notifyUri});
    }

    auto watch = MediaLibraryNotify::GetInstance();
    if (watch == nullptr) {
        MEDIA_ERR_LOG("Can not get MediaLibraryNotify");
        return;
    }
    if (notifyData->trashDate > 0) {
        watch->Notify(notifyData->notifyUri, NotifyType::NOTIFY_REMOVE);
        watch->Notify(notifyData->notifyUri, NotifyType::NOTIFY_ALBUM_REMOVE_ASSET);
    } else {
        watch->Notify(notifyData->notifyUri, NotifyType::NOTIFY_ADD);
        watch->Notify(notifyData->notifyUri, NotifyType::NOTIFY_ALBUM_ADD_ASSET);
    }

    int trashAlbumId = watch->GetAlbumIdBySubType(PhotoAlbumSubType::TRASH);
    if (trashAlbumId <= 0) {
        return;
    }
    NotifyType type = (notifyData->trashDate > 0) ? NotifyType::NOTIFY_ALBUM_ADD_ASSET :
        NotifyType::NOTIFY_ALBUM_REMOVE_ASSET;
    watch->Notify(notifyData->notifyUri, type, trashAlbumId);

    if (MAP_OPERATION_FLAG) {
        MEDIA_INFO_LOG("map operation flag true");
        vector<int64_t> formIds;
        MediaLibraryFormMapOperations::GetFormMapFormId(notifyData->notifyUri, formIds);
        if (!formIds.empty()) {
            MediaLibraryFormMapOperations::PublishedChange("", formIds, false);
        }
    }
}

int32_t MediaLibraryAssetOperations::SendTrashNotify(MediaLibraryCommand &cmd, int32_t rowId, const string &extraUri,
    shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh)
{
    ValueObject value;
    int64_t trashDate = 0;
    if (!cmd.GetValueBucket().GetObject(PhotoColumn::MEDIA_DATE_TRASHED, value)) {
        return E_DO_NOT_NEDD_SEND_NOTIFY;
    }

    value.GetLong(trashDate);

    string prefix;
    if (cmd.GetOprnObject() == OperationObject::FILESYSTEM_PHOTO) {
        prefix = PhotoColumn::PHOTO_URI_PREFIX;
    } else if (cmd.GetOprnObject() == OperationObject::FILESYSTEM_AUDIO) {
        prefix = AudioColumn::AUDIO_URI_PREFIX;
    } else {
        return E_OK;
    }

    string notifyUri = MediaFileUtils::GetUriByExtrConditions(prefix, to_string(rowId), extraUri);
    shared_ptr<MediaLibraryAsyncWorker> asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    if (asyncWorker == nullptr) {
        MEDIA_ERR_LOG("Can not get asyncWorker");
        return E_ERR;
    }
    DeleteNotifyAsyncTaskData* taskData = new (std::nothrow) DeleteNotifyAsyncTaskData();
    if (taskData == nullptr) {
        MEDIA_ERR_LOG("Failed to new taskData");
        return E_ERR;
    }
    taskData->notifyUri = notifyUri;
    taskData->trashDate = trashDate;
    taskData->refresh_ = assetRefresh;
    shared_ptr<MediaLibraryAsyncTask> notifyAsyncTask = make_shared<MediaLibraryAsyncTask>(
        UpdateAlbumsAndSendNotifyInTrash, taskData);
    if (notifyAsyncTask != nullptr) {
        asyncWorker->AddTask(notifyAsyncTask, true);
    } else {
        MEDIA_ERR_LOG("Start UpdateAlbumsAndSendNotifyInTrash failed");
    }
    return E_OK;
}

void MediaLibraryAssetOperations::SendFavoriteNotify(MediaLibraryCommand &cmd, shared_ptr<FileAsset> &fileAsset,
    const string &extraUri, shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh)
{
    ValueObject value;
    int32_t isFavorite = 0;
    if (!cmd.GetValueBucket().GetObject(PhotoColumn::MEDIA_IS_FAV, value)) {
        return;
    }
    value.GetInt(isFavorite);

    if (assetRefresh == nullptr) {
        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        CHECK_AND_RETURN_LOG(rdbStore != nullptr, "Failed to get rdbStore.");
        MediaLibraryRdbUtils::UpdateSystemAlbumInternal(rdbStore, { to_string(PhotoAlbumSubType::FAVORITE) });
        CHECK_AND_RETURN_LOG(fileAsset != nullptr, "fileAsset is nullptr");
        if (fileAsset->IsHidden()) {
            MediaLibraryRdbUtils::UpdateSysAlbumHiddenState(rdbStore, { to_string(PhotoAlbumSubType::FAVORITE) });
        }
    } else {
        assetRefresh->RefreshAlbum();
    }

    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
    if (cmd.GetOprnObject() != OperationObject::FILESYSTEM_PHOTO) {
        return;
    }
    int favAlbumId = watch->GetAlbumIdBySubType(PhotoAlbumSubType::FAVORITE);
    if (favAlbumId <= 0) {
        return;
    }

    NotifyType type = (isFavorite) ? NotifyType::NOTIFY_ALBUM_ADD_ASSET : NotifyType::NOTIFY_ALBUM_REMOVE_ASSET;
    watch->Notify(
        MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, to_string(fileAsset->GetId()), extraUri),
        type, favAlbumId);
}

int32_t MediaLibraryAssetOperations::SendModifyUserCommentNotify(MediaLibraryCommand &cmd, int32_t rowId,
    const string &extraUri)
{
    if (cmd.GetOprnType() != OperationType::SET_USER_COMMENT) {
        return E_DO_NOT_NEDD_SEND_NOTIFY;
    }

    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");
    watch->Notify(MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, to_string(rowId), extraUri),
        NotifyType::NOTIFY_UPDATE);
    return E_OK;
}

int32_t MediaLibraryAssetOperations::GetAlbumIdByPredicates(const string &whereClause, const vector<string> &whereArgs)
{
    size_t pos = whereClause.find(PhotoColumn::PHOTO_OWNER_ALBUM_ID);
    CHECK_AND_RETURN_RET_LOG(pos != string::npos, E_ERR, "Predicates whereClause is invalid");
    size_t argsIndex = 0;
    for (size_t i = 0; i < pos; ++i) {
        if (whereClause[i] == '?') {
            argsIndex++;
        }
    }
    if (argsIndex > whereArgs.size() - 1) {
        MEDIA_ERR_LOG("whereArgs is invalid");
        return E_ERR;
    }
    auto albumId = whereArgs[argsIndex];
    CHECK_AND_RETURN_RET(!MediaLibraryDataManagerUtils::IsNumber(albumId), std::atoi(albumId.c_str()));
    return E_ERR;
}

void MediaLibraryAssetOperations::UpdateOwnerAlbumIdOnMove(MediaLibraryCommand &cmd,
    int32_t &targetAlbumId, int32_t &oriAlbumId)
{
    ValueObject value;
    if (!cmd.GetValueBucket().GetObject(PhotoColumn::PHOTO_OWNER_ALBUM_ID, value)) {
        return;
    }
    value.GetInt(targetAlbumId);
    auto whereClause = cmd.GetAbsRdbPredicates()->GetWhereClause();
    auto whereArgs = cmd.GetAbsRdbPredicates()->GetWhereArgs();
    oriAlbumId = GetAlbumIdByPredicates(whereClause, whereArgs);

    MEDIA_INFO_LOG("Move Assets, ori album id is %{public}d, target album id is %{public}d", oriAlbumId, targetAlbumId);
}

int32_t MediaLibraryAssetOperations::SetPendingTrue(const shared_ptr<FileAsset> &fileAsset)
{
    // time_pending = 0, means file is created, not allowed
    // time_pending = UNCREATE_FILE_TIMEPENDING, means file is not created yet, create an empty one
    // time_pending = UNCLOSE_FILE_TIMEPENDING, means file is not close yet, set pending time
    // time_pending = UNOPEN_FILE_COMPONENT_TIMEPENDING, means file is created but not open, set pending time
    // time_pending is timestamp, update it
    int64_t timestamp = MediaFileUtils::UTCTimeSeconds();
    if (timestamp <= 0) {
        MEDIA_ERR_LOG("Get timestamp failed, timestamp:%{public}ld", (long) timestamp);
        return E_INVALID_TIMESTAMP;
    }
    if (fileAsset->GetTimePending() == 0) {
        MEDIA_ERR_LOG("fileAsset time_pending is 0, not allowed");
        return E_INVALID_VALUES;
    } else if (fileAsset->GetTimePending() == UNCREATE_FILE_TIMEPENDING) {
        int32_t errCode = CreateFileAndSetPending(fileAsset, timestamp);
        if (errCode != E_OK) {
            MEDIA_ERR_LOG("Create asset failed, id=%{public}d", fileAsset->GetId());
            return errCode;
        }
    } else if (fileAsset->GetTimePending() == UNCLOSE_FILE_TIMEPENDING ||
        fileAsset->GetTimePending() == UNOPEN_FILE_COMPONENT_TIMEPENDING ||
        fileAsset->GetTimePending() > 0) {
        int32_t errCode = SetPendingTime(fileAsset, timestamp);
        if (errCode != E_OK) {
            MEDIA_ERR_LOG("Set pending time failed, id=%{public}d", fileAsset->GetId());
            return errCode;
        }
    } else {
        MEDIA_ERR_LOG("fileAsset time_pending is invalid, time_pending:%{public}ld, id=%{public}d",
            (long) fileAsset->GetTimePending(), fileAsset->GetId());
        return E_INVALID_VALUES;
    }

    return E_OK;
}

int32_t MediaLibraryAssetOperations::SetPendingFalse(const shared_ptr<FileAsset> &fileAsset)
{
    // time_pending = 0, only return
    // time_pending = UNCREATE_FILE_TIMEPENDING, means file is not created yet, not allowed
    // time_pending = UNCLOSE_FILE_TIMEPENDING, means file is not close yet, not allowed
    // time_pending = UNOPEN_FILE_COMPONENT_TIMEPENDING, means file is created but not open, not allowed
    // time_pending is timestamp, scan and set pending time = 0
    if (fileAsset->GetTimePending() == 0) {
        return E_OK;
    } else if (fileAsset->GetTimePending() == UNCREATE_FILE_TIMEPENDING) {
        MEDIA_ERR_LOG("file is not created yet, not allowed, id=%{public}d", fileAsset->GetId());
        return E_INVALID_VALUES;
    } else if (fileAsset->GetTimePending() == UNCLOSE_FILE_TIMEPENDING) {
        MEDIA_ERR_LOG("file is not close yet, not allowed, id=%{public}d", fileAsset->GetId());
        return E_INVALID_VALUES;
    } else if (fileAsset->GetTimePending() == UNOPEN_FILE_COMPONENT_TIMEPENDING) {
        MEDIA_ERR_LOG("file is created but not open, not allowed, id=%{public}d", fileAsset->GetId());
        return E_INVALID_VALUES;
    } else if (fileAsset->GetTimePending() > 0) {
        ScanFile(fileAsset->GetPath(), true, true);
    } else {
        MEDIA_ERR_LOG("fileAsset time_pending is invalid, time_pending:%{public}ld, id=%{public}d",
            (long) fileAsset->GetTimePending(), fileAsset->GetId());
        return E_INVALID_VALUES;
    }
    return E_OK;
}

int32_t MediaLibraryAssetOperations::SetPendingStatus(MediaLibraryCommand &cmd)
{
    int32_t pendingStatus = 0;
    if (!GetInt32FromValuesBucket(cmd.GetValueBucket(), MediaColumn::MEDIA_TIME_PENDING, pendingStatus)) {
        return E_INVALID_VALUES;
    }

    vector<string> columns = {
        MediaColumn::MEDIA_ID,
        MediaColumn::MEDIA_FILE_PATH,
        MediaColumn::MEDIA_TYPE,
        MediaColumn::MEDIA_TIME_PENDING
    };
    auto fileAsset = GetFileAssetFromDb(*(cmd.GetAbsRdbPredicates()), cmd.GetOprnObject(), columns);
    if (fileAsset == nullptr) {
        return E_INVALID_VALUES;
    }
    if (pendingStatus == 1) {
        return SetPendingTrue(fileAsset);
    } else if (pendingStatus == 0) {
        return SetPendingFalse(fileAsset);
    } else {
        MEDIA_ERR_LOG("pendingStatus is invalid, pendingStatus:%{public}d", pendingStatus);
        return E_INVALID_VALUES;
    }
}

static string ConvertMediaPathFromCloudPath(const string &path)
{
    // if input path is /storage/cloud/xxx, return /storage/media/local/xxx
    string mediaPath = "/storage/media/local/";
    string cloudPath = "/storage/cloud/";
    string newPath = path;
    if (newPath.find(cloudPath) != string::npos) {
        newPath.replace(newPath.find(cloudPath), cloudPath.length(), mediaPath);
    }
    return newPath;
}

int32_t MediaLibraryAssetOperations::GrantUriPermission(const string &uri, const string &bundleName,
    const string &path, bool isMovingPhoto)
{
    if (uri.empty() || path.empty()) {
        MEDIA_ERR_LOG("uri or path is empty, uri:%{private}s, path:%{private}s", uri.c_str(), path.c_str());
        return E_INVALID_VALUES;
    }
    if (bundleName.empty()) {
        MEDIA_WARN_LOG("bundleName is empty, bundleName:%{private}s", bundleName.c_str());
        return E_OK;
    }
    if (!MediaFileUtils::CreateFile(path)) {
        MEDIA_ERR_LOG("Can not create file, path: %{private}s, errno: %{public}d", path.c_str(), errno);
        return E_HAS_FS_ERROR;
    }

    if (isMovingPhoto && !MediaFileUtils::CreateFile(MediaFileUtils::GetMovingPhotoVideoPath(path))) {
        MEDIA_ERR_LOG("Failed to create video of moving photo, errno: %{public}d", errno);
        return E_HAS_FS_ERROR;
    }

    return E_OK;
}

bool MediaLibraryAssetOperations::GetInt32FromValuesBucket(const NativeRdb::ValuesBucket &values,
    const std::string &column, int32_t &value)
{
    ValueObject valueObject;
    if (values.GetObject(column, valueObject)) {
        valueObject.GetInt(value);
    } else {
        return false;
    }
    return true;
}

std::string MediaLibraryAssetOperations::CreateExtUriForV10Asset(FileAsset &fileAsset)
{
    const std::string &filePath = fileAsset.GetPath();
    const std::string &displayName = fileAsset.GetDisplayName();
    auto mediaType = fileAsset.GetMediaType();
    if (filePath.empty() || displayName.empty() || mediaType < 0) {
        MEDIA_ERR_LOG("param invalid, filePath %{private}s or displayName %{private}s invalid failed.",
            filePath.c_str(), displayName.c_str());
        return "";
    }

    string extrUri = MediaFileUtils::GetExtraUri(displayName, filePath);
    return MediaFileUtils::GetUriByExtrConditions(ML_FILE_URI_PREFIX + MediaFileUri::GetMediaTypeUri(mediaType,
        MEDIA_API_VERSION_V10) + "/", to_string(fileAsset.GetId()), extrUri);
}

bool MediaLibraryAssetOperations::GetStringFromValuesBucket(const NativeRdb::ValuesBucket &values,
    const std::string &column, string &value)
{
    ValueObject valueObject;
    if (values.GetObject(column, valueObject)) {
        valueObject.GetString(value);
    } else {
        return false;
    }
    return true;
}

int32_t MediaLibraryAssetOperations::CreateAssetUniqueId(int32_t type,
    std::shared_ptr<TransactionOperations> trans)
{
    string typeString;
    switch (type) {
        case MediaType::MEDIA_TYPE_IMAGE:
            typeString += IMAGE_ASSET_TYPE;
            break;
        case MediaType::MEDIA_TYPE_VIDEO:
            typeString += VIDEO_ASSET_TYPE;
            break;
        case MediaType::MEDIA_TYPE_AUDIO:
            typeString += AUDIO_ASSET_TYPE;
            break;
        default:
            MEDIA_ERR_LOG("This type %{public}d can not get unique id", type);
            return E_INVALID_VALUES;
    }

    const string updateSql = "UPDATE " + ASSET_UNIQUE_NUMBER_TABLE + " SET " + UNIQUE_NUMBER +
        "=" + UNIQUE_NUMBER + "+1" " WHERE " + ASSET_MEDIA_TYPE + "='" + typeString + "';";
    const string querySql = "SELECT " + UNIQUE_NUMBER + " FROM " + ASSET_UNIQUE_NUMBER_TABLE +
        " WHERE " + ASSET_MEDIA_TYPE + "='" + typeString + "';";

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return E_HAS_DB_ERROR;
    }
    lock_guard<mutex> lock(g_uniqueNumberLock);
    int32_t errCode;
    if (trans == nullptr) {
        errCode = rdbStore->ExecuteSql(updateSql);
    } else {
        errCode = trans->ExecuteSql(updateSql);
    }
    if (errCode < 0) {
        MEDIA_ERR_LOG("execute update unique number failed, ret=%{public}d", errCode);
        return errCode;
    }

    auto resultSet = rdbStore->QuerySql(querySql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return E_HAS_DB_ERROR;
    }
    return GetInt32Val(UNIQUE_NUMBER, resultSet);
}

int32_t MediaLibraryAssetOperations::CreateAssetUniqueIds(int32_t type, int32_t num, int32_t &startUniqueNumber)
{
    if (num == 0) {
        return E_OK;
    }
    string typeString;
    switch (type) {
        case MediaType::MEDIA_TYPE_IMAGE:
            typeString = IMAGE_ASSET_TYPE;
            break;
        case MediaType::MEDIA_TYPE_VIDEO:
            typeString = VIDEO_ASSET_TYPE;
            break;
        case MediaType::MEDIA_TYPE_AUDIO:
            typeString = AUDIO_ASSET_TYPE;
            break;
        default:
            MEDIA_ERR_LOG("This type %{public}d can not get unique id", type);
            return E_INVALID_VALUES;
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET(rdbStore != nullptr, E_HAS_DB_ERROR);
    const string updateSql = "UPDATE " + ASSET_UNIQUE_NUMBER_TABLE + " SET " + UNIQUE_NUMBER +
        "=" + UNIQUE_NUMBER + "+" + to_string(num) + " WHERE " + ASSET_MEDIA_TYPE + "='" + typeString + "';";
    const string querySql = "SELECT " + UNIQUE_NUMBER + " FROM " + ASSET_UNIQUE_NUMBER_TABLE +
        " WHERE " + ASSET_MEDIA_TYPE + "='" + typeString + "';";
    lock_guard<mutex> lock(g_uniqueNumberLock);
    int32_t errCode = E_OK;
    std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>(__func__);
    std::function<int(void)> func = [&]()->int {
        errCode = trans->ExecuteSql(updateSql);
        CHECK_AND_RETURN_RET_LOG(errCode >= 0, errCode, "CreateAssetUniqueIds ExecuteSql err, ret=%{public}d",
            errCode);
        auto resultSet = trans->QueryByStep(querySql);
        CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "CreateAssetUniqueIds resultSet is null");
        if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("CreateAssetUniqueIds first row empty");
            resultSet->Close();
            return E_HAS_DB_ERROR;
        }
        int32_t endUniqueNumber = GetInt32Val(UNIQUE_NUMBER, resultSet);
        resultSet->Close();
        startUniqueNumber = endUniqueNumber - num;
        MEDIA_INFO_LOG("CreateAssetUniqueIds type: %{public}d, num: %{public}d, startUniqueNumber: %{public}d",
            type, num, startUniqueNumber);
        return E_OK;
    };
    errCode = trans->RetryTrans(func);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "CreateAssetUniqueIds err, ret=%{public}d", errCode);
    return E_OK;
}

int32_t MediaLibraryAssetOperations::CreateAssetRealName(int32_t fileId, int32_t mediaType,
    const string &extension, string &name)
{
    string fileNumStr = to_string(fileId);
    if (fileId <= ASSET_MAX_COMPLEMENT_ID) {
        size_t fileIdLen = fileNumStr.length();
        fileNumStr = ("00" + fileNumStr).substr(fileIdLen - 1);
    }

    string mediaTypeStr;
    switch (mediaType) {
        case MediaType::MEDIA_TYPE_IMAGE:
            mediaTypeStr = DEFAULT_IMAGE_NAME;
            break;
        case MediaType::MEDIA_TYPE_VIDEO:
            mediaTypeStr = DEFAULT_VIDEO_NAME;
            break;
        case MediaType::MEDIA_TYPE_AUDIO:
            mediaTypeStr = DEFAULT_AUDIO_NAME;
            break;
        default:
            MEDIA_ERR_LOG("This mediatype %{public}d can not get real name", mediaType);
            return E_INVALID_VALUES;
    }

    static const int32_t CONFLICT_TIME = 100;
    if (extension.length() == 0) {
        name = mediaTypeStr + to_string(MediaFileUtils::UTCTimeSeconds() + CONFLICT_TIME) + "_" + fileNumStr;
    } else {
        name = mediaTypeStr + to_string(MediaFileUtils::UTCTimeSeconds() + CONFLICT_TIME) + "_" +
            fileNumStr + "." + extension;
    }
    return E_OK;
}

static inline int32_t PrepareAssetDir(const string &dirPath)
{
    CHECK_AND_RETURN_RET(!dirPath.empty(), E_INVALID_PATH);
    if (!MediaFileUtils::IsFileExists(dirPath)) {
        bool ret = MediaFileUtils::CreateDirectory(dirPath);
        CHECK_AND_RETURN_RET_LOG(ret, E_CHECK_DIR_FAIL, "Create Dir Failed! dirPath=%{private}s",
            dirPath.c_str());
    }
    return E_OK;
}

int32_t MediaLibraryAssetOperations::CreateAssetPathById(int32_t fileId, int32_t mediaType,
    const string &extension, string &filePath)
{
    string mediaDirPath;
    GetAssetRootDir(mediaType, mediaDirPath);
    if (mediaDirPath.empty()) {
        return E_INVALID_VALUES;
    }

    int32_t bucketNum = 0;
    int32_t errCode = MediaFileUri::CreateAssetBucket(fileId, bucketNum);
    if (errCode != E_OK) {
        return errCode;
    }

    string realName;
    errCode = CreateAssetRealName(fileId, mediaType, extension, realName);
    if (errCode != E_OK) {
        return errCode;
    }

    string dirPath = ROOT_MEDIA_DIR + mediaDirPath + to_string(bucketNum);
    errCode = PrepareAssetDir(dirPath);
    if (errCode != E_OK) {
        return errCode;
    }

    filePath = dirPath + "/" + realName;
    return E_OK;
}

const std::unordered_map<std::string, std::vector<VerifyFunction>>
    AssetInputParamVerification::UPDATE_VERIFY_PARAM_MAP = {
    { MediaColumn::MEDIA_ID, { Forbidden } },
    { MediaColumn::MEDIA_FILE_PATH, { Forbidden } },
    { MediaColumn::MEDIA_SIZE, { Forbidden } },
    { MediaColumn::MEDIA_TITLE, { IsStringNotNull } },
    { MediaColumn::MEDIA_NAME, { IsStringNotNull } },
    { MediaColumn::MEDIA_TYPE, { Forbidden } },
    { MediaColumn::MEDIA_MIME_TYPE, { Forbidden } },
    { MediaColumn::MEDIA_OWNER_PACKAGE, { Forbidden } },
    { MediaColumn::MEDIA_OWNER_APPID, { Forbidden } },
    { MediaColumn::MEDIA_PACKAGE_NAME, { Forbidden } },
    { MediaColumn::MEDIA_DEVICE_NAME, { Forbidden } },
    { MediaColumn::MEDIA_DATE_MODIFIED, { Forbidden } },
    { MediaColumn::MEDIA_DATE_ADDED, { Forbidden } },
    { MediaColumn::MEDIA_DATE_TAKEN, { Forbidden } },
    { MediaColumn::MEDIA_DURATION, { Forbidden } },
    { MediaColumn::MEDIA_TIME_PENDING, { IsInt64, IsUniqueValue } },
    { MediaColumn::MEDIA_IS_FAV, { IsBool, IsUniqueValue } },
    { MediaColumn::MEDIA_DATE_TRASHED, { IsInt64, IsUniqueValue } },
    { MediaColumn::MEDIA_DATE_DELETED, { IsInt64, IsUniqueValue } },
    { MediaColumn::MEDIA_HIDDEN, { IsBool, IsUniqueValue } },
    { MediaColumn::MEDIA_PARENT_ID, { IsInt64, IsBelowApi9 } },
    { MediaColumn::MEDIA_RELATIVE_PATH, { IsString, IsBelowApi9 } },
    { MediaColumn::MEDIA_VIRTURL_PATH, { Forbidden } },
    { PhotoColumn::PHOTO_ORIENTATION, { IsInt64 } },
    { PhotoColumn::PHOTO_LATITUDE, { Forbidden } },
    { PhotoColumn::PHOTO_LONGITUDE, { Forbidden } },
    { PhotoColumn::PHOTO_HEIGHT, { Forbidden } },
    { PhotoColumn::PHOTO_WIDTH, { Forbidden } },
    { PhotoColumn::PHOTO_LCD_VISIT_TIME, { IsInt64 } },
    { PhotoColumn::PHOTO_EDIT_TIME, { IsInt64 } },
    { AudioColumn::AUDIO_ALBUM, { Forbidden } },
    { AudioColumn::AUDIO_ARTIST, { Forbidden } },
    { PhotoColumn::CAMERA_SHOT_KEY, { IsString } },
    { PhotoColumn::PHOTO_USER_COMMENT, { IsString } },
    { PhotoColumn::PHOTO_ID, { IsString } },
    { PhotoColumn::PHOTO_QUALITY, { IsInt32 } },
    { PhotoColumn::PHOTO_FIRST_VISIT_TIME, { IsInt64 } },
    { PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, { IsInt32 } },
    { PhotoColumn::PHOTO_SUBTYPE, { IsInt32 } },
    { PhotoColumn::MOVING_PHOTO_EFFECT_MODE, { IsInt32 } },
    { PhotoColumn::PHOTO_COVER_POSITION, { IsInt64 } },
    { PhotoColumn::PHOTO_IS_TEMP, { IsBool } },
    { PhotoColumn::PHOTO_DIRTY, { IsInt32 } },
    { PhotoColumn::PHOTO_DETAIL_TIME, { IsStringNotNull } },
    { PhotoColumn::PHOTO_OWNER_ALBUM_ID, { IsInt32 } },
    { PhotoColumn::PHOTO_CE_AVAILABLE, { IsInt32 } },
    { PhotoColumn::SUPPORTED_WATERMARK_TYPE, { IsInt32 } },
    { PhotoColumn::PHOTO_IS_AUTO, { IsInt32 } },
    { PhotoColumn::PHOTO_IS_RECENT_SHOW, { IsBool, IsUniqueValue } },
    { PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS, { IsInt32 } },
};

bool AssetInputParamVerification::CheckParamForUpdate(MediaLibraryCommand &cmd)
{
    ValuesBucket &values = cmd.GetValueBucket();
    map<string, ValueObject> valuesMap;
    values.GetAll(valuesMap);
    for (auto &iter : valuesMap) {
        if (UPDATE_VERIFY_PARAM_MAP.find(iter.first) == UPDATE_VERIFY_PARAM_MAP.end()) {
            MEDIA_ERR_LOG("param [%{private}s] is not allowed", iter.first.c_str());
            return false;
        }
        for (auto &verifyFunc : UPDATE_VERIFY_PARAM_MAP.at(iter.first)) {
            if (!verifyFunc(iter.second, cmd)) {
                MEDIA_ERR_LOG("verify param [%{private}s] failed", iter.first.c_str());
                return false;
            }
        }
    }
    return true;
}

bool AssetInputParamVerification::Forbidden(ValueObject &value, MediaLibraryCommand &cmd)
{
    return false;
}

bool AssetInputParamVerification::IsInt32(ValueObject &value, MediaLibraryCommand &cmd)
{
    if (value.GetType() == ValueObjectType::TYPE_INT) {
        return true;
    }
    return false;
}

bool AssetInputParamVerification::IsInt64(ValueObject &value, MediaLibraryCommand &cmd)
{
    if (value.GetType() == ValueObjectType::TYPE_INT) {
        return true;
    }
    return false;
}

bool AssetInputParamVerification::IsBool(ValueObject &value, MediaLibraryCommand &cmd)
{
    if (value.GetType() == ValueObjectType::TYPE_BOOL) {
        return true;
    }
    if (value.GetType() == ValueObjectType::TYPE_INT) {
        int32_t ret;
        value.GetInt(ret);
        if (ret == 0 || ret == 1) {
            return true;
        }
    }
    return false;
}

bool AssetInputParamVerification::IsString(ValueObject &value, MediaLibraryCommand &cmd)
{
    CHECK_AND_RETURN_RET(value.GetType() != ValueObjectType::TYPE_STRING, true);
    return false;
}

bool AssetInputParamVerification::IsDouble(ValueObject &value, MediaLibraryCommand &cmd)
{
    CHECK_AND_RETURN_RET(value.GetType() != ValueObjectType::TYPE_DOUBLE, true);
    return false;
}

bool AssetInputParamVerification::IsBelowApi9(ValueObject &value, MediaLibraryCommand &cmd)
{
    CHECK_AND_RETURN_RET(cmd.GetApi() != MediaLibraryApi::API_OLD, true);
    return false;
}

bool AssetInputParamVerification::IsStringNotNull(ValueObject &value, MediaLibraryCommand &cmd)
{
    if (value.GetType() != ValueObjectType::TYPE_STRING) {
        return false;
    }
    string str;
    value.GetString(str);
    if (str.empty()) {
        return false;
    }
    return true;
}

bool AssetInputParamVerification::IsUniqueValue(ValueObject &value, MediaLibraryCommand &cmd)
{
    // whether this is the unique value in ValuesBucket
    map<string, ValueObject> valuesMap;
    cmd.GetValueBucket().GetAll(valuesMap);
    if (valuesMap.size() != 1) {
        return false;
    }
    return true;
}

static void CreateThumbnailFileScaned(const string &uri, const string &path, bool isSync,
    std::shared_ptr<Media::Picture> originalPhotoPicture)
{
    if (ThumbnailService::GetInstance() == nullptr) {
        return;
    }
    CHECK_AND_RETURN_LOG(!uri.empty(), "Uri is empty");
    int32_t err = 0;
    if (originalPhotoPicture != nullptr) {
        err = ThumbnailService::GetInstance()->CreateThumbnailFileScanedWithPicture(
            uri, path, originalPhotoPicture, isSync);
    } else {
        err = ThumbnailService::GetInstance()->CreateThumbnailFileScaned(uri, path, isSync);
    }
    CHECK_AND_RETURN_LOG(err == E_SUCCESS, "ThumbnailService CreateThumbnailFileScaned failed : %{public}d", err);
}

int32_t MediaLibraryAssetOperations::ScanAssetCallback::OnScanFinished(const int32_t status,
    const string &uri, const string &path)
{
    if (status == E_SCANNED) {
        MEDIA_DEBUG_LOG("Asset is scannned");
        return E_OK;
    } else if (status != E_OK) {
        MEDIA_ERR_LOG("Scan is failed, status = %{public}d, skip create thumbnail", status);
        return status;
    }

    string fileId = MediaFileUtils::GetIdFromUri(uri);
    int32_t type = MediaFileUtils::GetMediaType(path);
    bool isThumbnailExist = PhotoFileUtils::IsThumbnailExists(path);
    if (this->isInvalidateThumb && isThumbnailExist && !PhotoFileUtils::IsThumbnailLatest(path)) {
        InvalidateThumbnail(fileId, type);
    }
    CreateThumbnailFileScaned(uri, path, this->isCreateThumbSync, originalPhotoPicture);
    MediaFileUtils::DeleteFile(MovingPhotoFileUtils::GetLivePhotoCachePath(path));

#ifdef META_RECOVERY_SUPPORT
    int32_t id;
    if (StrToInt(fileId, id)) {
        MediaLibraryMetaRecovery::GetInstance().WriteSingleMetaDataById(id);
    }
#endif

    return E_OK;
}

void MediaLibraryAssetOperations::SaveDeletedFile(const std::vector<std::string> &ids,
    const std::vector<std::string> &paths, const std::string &table, const std::vector<std::string> &dateTakens,
    std::vector<int32_t> &subTypes)
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DELETED_FILE_EVENT, errCode);
    if (prefs == nullptr) {
        MEDIA_ERR_LOG("Get preferences error: %{public}d", errCode);
        return;
    }

    for (size_t i = 0; i < ids.size(); i++) {
        std::string key = table + "?" + ids[i];
        std::string value = paths[i] + "?" + dateTakens[i] + "?" + std::to_string(subTypes[i]);
        prefs->PutString(key, value);
    }
    prefs->FlushSync();
}

void MediaLibraryAssetOperations::ClearDeletedFile(const std::vector<std::string> &ids, const std::string &table)
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DELETED_FILE_EVENT, errCode);
    if (prefs == nullptr) {
        MEDIA_ERR_LOG("Get preferences error: %{public}d", errCode);
        return;
    }

    for (size_t i = 0; i < ids.size(); i++) {
        std::string key = table + "?" + ids[i];
        prefs->Delete(key);
    }
    prefs->FlushSync();
}

void MediaLibraryAssetOperations::TaskDataFileProcess(const std::vector<std::string> &ids,
    const std::vector<std::string> &paths, const std::string &table, const std::vector<std::string> &dateTakens,
    std::vector<int32_t> &subTypes)
{
    for (size_t i = 0; i < paths.size(); i++) {
        string filePath = paths[i];
        string fileId = i < ids.size() ? ids[i] : "";
        MEDIA_INFO_LOG("Delete file id: %{public}s, path: %{private}s", fileId.c_str(), filePath.c_str());
        if (!MediaFileUtils::DeleteFile(filePath) && (errno != ENOENT)) {
            MEDIA_WARN_LOG("Failed to delete file, errno: %{public}d, path: %{private}s", errno, filePath.c_str());
        }

#ifdef META_RECOVERY_SUPPORT
        MediaLibraryMetaRecovery::DeleteMetaDataByPath(filePath);
#endif
        if (i < subTypes.size() && subTypes[i] == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
            // delete video file of moving photo
            string videoPath = MediaFileUtils::GetMovingPhotoVideoPath(filePath);
            if (!LakeFileUtils::DeleteFile(videoPath) && (errno != ENOENT)) {
                MEDIA_WARN_LOG("Failed to delete video file, errno: %{public}d, path: %{private}s", errno,
                    videoPath.c_str());
            }
            string livePhotoPath = MovingPhotoFileUtils::GetLivePhotoCachePath(filePath);
            bool conds = (MediaFileUtils::IsFileExists(livePhotoPath) && !MediaFileUtils::DeleteFile(livePhotoPath));
            CHECK_AND_WARN_LOG(!conds, "Failed to delete cache live photo, errno: %{public}d, path: %{private}s",
                errno, livePhotoPath.c_str());
        }
    }

    ThumbnailService::GetInstance()->BatchDeleteThumbnailDirAndAstc(
        table, ids, paths, dateTakens);
    if (table == PhotoColumn::PHOTOS_TABLE) {
        for (const auto &path : paths) {
            MediaLibraryPhotoOperations::DeleteRevertMessage(path);
        }
    }
    MediaLibraryAssetOperations::ClearDeletedFile(ids, table);
}

static void DeleteFiles(AsyncTaskData *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("DeleteFiles");
    if (data == nullptr) {
        return;
    }
    auto *taskData = static_cast<DeleteFilesData *>(data);

#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    // 检查点 批量下载 本地删除 停止并清理 通知应用 notify type 3
    MEDIA_INFO_LOG("BatchSelectFileDownload DeleteFiles DealWithBatchDownloadingFilesById");
    MediaLibraryAssetOperations::DealWithBatchDownloadingFilesById(taskData->ids_);
#endif
    MediaLibraryAssetOperations::TaskDataFileProcess(taskData->ids_, taskData->paths_, taskData->table_,
        taskData->dateTakens_, taskData->subTypes_);
}

void HandleAudiosResultSet(const shared_ptr<NativeRdb::ResultSet> &resultSet, DeletedFilesParams &filesParams)
{
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        filesParams.ids.push_back(
            to_string(get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_ID, resultSet, TYPE_INT32))));
        filesParams.paths.push_back(get<string>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_FILE_PATH,
            resultSet, TYPE_STRING)));
        filesParams.dateTakens.push_back(get<string>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_DATE_TAKEN,
            resultSet, TYPE_STRING)));
        filesParams.subTypes.push_back(static_cast<int32_t>(PhotoSubType::DEFAULT));
    }
}

void HandlePhotosResultSet(const shared_ptr<NativeRdb::ResultSet> &resultSet, DeletedFilesParams &filesParams)
{
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        filesParams.ids.push_back(
            to_string(get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_ID, resultSet, TYPE_INT32))));
        if (MediaLibraryRdbStore::GetInt(resultSet, PhotoColumn::PHOTO_FILE_SOURCE_TYPE)
            == static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE)) {
            filesParams.paths.push_back(get<string>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_STORAGE_PATH,
                resultSet, TYPE_STRING)));
        } else {
            filesParams.paths.push_back(get<string>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_FILE_PATH,
                resultSet, TYPE_STRING)));
        }
        filesParams.dateTakens.push_back(get<string>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_DATE_TAKEN,
            resultSet, TYPE_STRING)));
        filesParams.subTypes.push_back(
            get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_SUBTYPE, resultSet, TYPE_INT32)));
        filesParams.isTemps.push_back(
            get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_IS_TEMP, resultSet, TYPE_INT32)));
        filesParams.displayNames.insert({
            to_string(get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_ID, resultSet, TYPE_INT32))),
            get<string>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_NAME, resultSet, TYPE_STRING))});
        filesParams.ownerAlbumIds.insert({
            to_string(get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_ID, resultSet, TYPE_INT32))),
            to_string(get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSet,
                TYPE_INT32)))});
        MultiStagesCaptureManager::RemovePhotosWithResultSet(resultSet, false);
        if (filesParams.containsHidden) {
            continue;
        }
        int32_t hidden = get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_HIDDEN, resultSet,
            TYPE_INT32));
        if (hidden > 0) {
            filesParams.containsHidden = true;
        }
    }
}

static shared_ptr<NativeRdb::ResultSet> GetBurstMemberResultSet(const shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    vector<string> burstKeyList;
    int32_t ret = resultSet->GoToFirstRow();
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, resultSet, "GetBurstMemberResultSet Failed to GoToFirstRow");
    do {
        int32_t burstLevel = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_BURST_COVER_LEVEL,
            resultSet, TYPE_INT32));
        int32_t subtype = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_SUBTYPE,
            resultSet, TYPE_INT32));
        bool isBurstCover = (burstLevel == static_cast<int32_t>(BurstCoverLevelType::COVER)) &&
            (subtype == static_cast<int32_t>(PhotoSubType::BURST));
        if (isBurstCover) {
            string burstKey = get<string>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_BURST_KEY,
                resultSet, TYPE_STRING));
            MEDIA_DEBUG_LOG("delete brust burstKey is %{public}s", burstKey.c_str());
            burstKeyList.push_back(burstKey);
        }
    } while (resultSet->GoToNextRow() == NativeRdb::E_OK);

    AbsRdbPredicates rbdPredicates(PhotoColumn::PHOTOS_TABLE);
    rbdPredicates.In(PhotoColumn::PHOTO_BURST_KEY, burstKeyList);
    rbdPredicates.EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL, static_cast<int32_t>(BurstCoverLevelType::MEMBER));
    vector<string> columns = {};
    auto burstMemberResultSet = MediaLibraryRdbStore::Query(rbdPredicates, columns);
    return burstMemberResultSet;
}

int32_t QueryFileInfoAndHandleRemovePhotos(const AbsRdbPredicates &predicates, DeletedFilesParams &filesParams)
{
    vector<string> columns = {
        MediaColumn::MEDIA_ID,
        MediaColumn::MEDIA_FILE_PATH,
        MediaColumn::MEDIA_DATE_TAKEN,
        PhotoColumn::PHOTO_IS_TEMP
    };

    if (predicates.GetTableName() == PhotoColumn::PHOTOS_TABLE) {
        columns.push_back(PhotoColumn::PHOTO_SUBTYPE);
        columns.push_back(MediaColumn::MEDIA_NAME);
        columns.push_back(PhotoColumn::PHOTO_OWNER_ALBUM_ID);
        columns.push_back(MEDIA_DATA_DB_PHOTO_ID);
        columns.push_back(MEDIA_DATA_DB_PHOTO_QUALITY);
        columns.push_back(MEDIA_DATA_DB_MEDIA_TYPE);
        columns.push_back(MEDIA_DATA_DB_STAGE_VIDEO_TASK_STATUS);
        columns.push_back(MediaColumn::MEDIA_HIDDEN);
        columns.push_back(PhotoColumn::PHOTO_BURST_COVER_LEVEL);
        columns.push_back(PhotoColumn::PHOTO_BURST_KEY);
        columns.push_back(PhotoColumn::PHOTO_FILE_SOURCE_TYPE);
        columns.push_back(PhotoColumn::PHOTO_STORAGE_PATH);
    }

    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    if (resultSet == nullptr) {
        return E_HAS_DB_ERROR;
    }

    if (predicates.GetTableName() == PhotoColumn::PHOTOS_TABLE) {
        HandlePhotosResultSet(resultSet, filesParams);
        // delete brust member photo
        auto burstMemberResultSet = GetBurstMemberResultSet(resultSet);
        if (burstMemberResultSet) {
            HandlePhotosResultSet(burstMemberResultSet, filesParams);
        }
    } else if (predicates.GetTableName() == AudioColumn::AUDIOS_TABLE) {
        HandleAudiosResultSet(resultSet, filesParams);
    } else {
        MEDIA_WARN_LOG("Invalid table name.");
    }
    return E_OK;
}

static int32_t GetFileAssetsFromResultSet(const shared_ptr<NativeRdb::ResultSet> &resultSet,
    const vector<string> &fileColumns, vector<shared_ptr<FileAsset>> &fileAssets)
{
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "The provided resultSet is nullptr");
    int32_t fileCount = 0;
    CHECK_AND_RETURN_RET_LOG(resultSet->GetRowCount(fileCount) == NativeRdb::E_OK, E_HAS_DB_ERROR,
        "Cannot get fileCount of resultset");
    CHECK_AND_RETURN_RET_LOG(fileCount > 0, E_HAS_DB_ERROR, "ResultSet fileCount is %{public}d", fileCount);

    fileAssets.reserve(fileCount);
    for (int32_t i = 0; i < fileCount; i++) {
        CHECK_AND_RETURN_RET_LOG(
            resultSet->GoToNextRow() == NativeRdb::E_OK, E_HAS_DB_ERROR, "Failed to go to next row");
        auto fileAsset = FetchFileAssetFromResultSet(resultSet, fileColumns);
        CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_HAS_DB_ERROR, "Failed to fetch fileAsset from resultSet");
        fileAssets.push_back(fileAsset);
    }
    resultSet->Close();
    return E_OK;
}

static int64_t GetAssetSize(const std::string &extraPath)
{
    MEDIA_DEBUG_LOG("GetAssetSize start.");
    UniqueFd fd(LakeFileUtils::OpenFile(extraPath, O_RDONLY));
    if (fd.Get() == E_ERR) {
        MEDIA_ERR_LOG("failed to open extra file");
        return static_cast<int64_t>(E_ERR);
    }
    struct stat st;
    CHECK_AND_RETURN_RET_LOG(fstat(fd.Get(), &st) == E_OK, static_cast<int64_t>(E_ERR),
        "failed to get file size");
    off_t fileSize = st.st_size;
    return static_cast<int64_t>(fileSize);
}

static void PushMovingPhotoExternalPath(const std::string &path, const std::string &logTarget,
    vector<string> &attachment)
{
    CHECK_AND_RETURN_WARN_LOG(!path.empty(), "%{public}s is invalid.", logTarget.c_str());
    attachment.push_back(path);
}

static void GetMovingPhotoExternalInfo(ExternalInfo &exInfo, vector<string> &attachment)
{
    MEDIA_DEBUG_LOG("GetMovingPhotoExternalInfo start.");
    CHECK_AND_RETURN(MovingPhotoFileUtils::IsMovingPhoto(exInfo.subType, exInfo.effectMode, exInfo.originalSubType));

    exInfo.videoPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(exInfo.path);
    exInfo.extraPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(exInfo.path);
    exInfo.photoImagePath = MovingPhotoFileUtils::GetSourceMovingPhotoImagePath(exInfo.path);
    exInfo.photoVideoPath = MovingPhotoFileUtils::GetSourceMovingPhotoVideoPath(exInfo.path);
    exInfo.cachePath = MovingPhotoFileUtils::GetLivePhotoCachePath(exInfo.path);
    PushMovingPhotoExternalPath(exInfo.videoPath, "videoPath", attachment);
    PushMovingPhotoExternalPath(exInfo.extraPath, "extraPath", attachment);
    PushMovingPhotoExternalPath(exInfo.photoImagePath, "photoImagePath", attachment);
    PushMovingPhotoExternalPath(exInfo.photoVideoPath, "photoVideoPath", attachment);
    PushMovingPhotoExternalPath(exInfo.cachePath, "cachePath", attachment);
    MEDIA_INFO_LOG("videoPath is %{public}s, extraPath is %{public}s, photoImagePath is %{public}s, \
        photoVideoPath is %{public}s, cachePath is %{public}s.", DfxUtils::GetSafePath(exInfo.videoPath).c_str(),
        DfxUtils::GetSafePath(exInfo.extraPath).c_str(), DfxUtils::GetSafePath(exInfo.photoImagePath).c_str(),
        DfxUtils::GetSafePath(exInfo.photoVideoPath).c_str(), DfxUtils::GetSafePath(exInfo.cachePath).c_str());
    exInfo.sizeMp4 = GetAssetSize(exInfo.videoPath);
    exInfo.sizeExtra = GetAssetSize(exInfo.extraPath);
    if (exInfo.sizeMp4 <= INVALID_SIZE) {
        MEDIA_WARN_LOG("failed to get mp4 size.");
    } else {
        exInfo.size += exInfo.sizeMp4;
    }
    if (exInfo.sizeExtra <= INVALID_SIZE) {
        MEDIA_WARN_LOG("failed to get extra size.");
    } else {
        exInfo.size += exInfo.sizeExtra;
    }
    MEDIA_DEBUG_LOG("MovingPhoto size is %{public}" PRId64, exInfo.size);
}

static void GetEditPhotoExternalInfo(ExternalInfo &exInfo, vector<string> &attachment)
{
    MEDIA_DEBUG_LOG("GetEditPhotoExternalInfo start.");
    CHECK_AND_RETURN_LOG(exInfo.editTime != 0, "editTime is zero");

    exInfo.editDataPath = PhotoFileUtils::GetEditDataPath(exInfo.path);
    exInfo.editDataCameraPath = PhotoFileUtils::GetEditDataCameraPath(exInfo.path);
    exInfo.editDataSourcePath = PhotoFileUtils::GetEditDataSourcePath(exInfo.path);
    PushMovingPhotoExternalPath(exInfo.editDataPath, "editDataPath", attachment);
    PushMovingPhotoExternalPath(exInfo.editDataCameraPath, "editDataCameraPath", attachment);
    PushMovingPhotoExternalPath(exInfo.editDataSourcePath, "editDataSourcePath", attachment);
    MEDIA_INFO_LOG("editDataPath is %{public}s, editDataCameraPath is %{public}s, editDataSourcePath is %{public}s",
        DfxUtils::GetSafePath(exInfo.editDataPath).c_str(), DfxUtils::GetSafePath(exInfo.editDataCameraPath).c_str(),
        DfxUtils::GetSafePath(exInfo.editDataSourcePath).c_str());
}

CleanFileInfo MediaLibraryAssetOperations::GetCleanFileInfo(shared_ptr<FileAsset> &fileAssetPtr)
{
    MEDIA_DEBUG_LOG("GetCleanFileInfo start.");
    CHECK_AND_RETURN_RET_LOG(fileAssetPtr != nullptr, {}, "GetCleanFileInfo fileAssetPtr is nullptr.");

    CleanFileInfo cleanFileInfo;
    ExternalInfo externalInfo;
    externalInfo.path = fileAssetPtr->GetPath();
    externalInfo.size = fileAssetPtr->GetSize();
    if (externalInfo.size == INVALID_SIZE) {
        externalInfo.size = GetAssetSize(externalInfo.path);
        CHECK_AND_RETURN_RET_LOG(externalInfo.size > INVALID_SIZE, {}, "failed to get asset size.");
    }
    MEDIA_INFO_LOG("path is %{public}s", DfxUtils::GetSafePath(externalInfo.path).c_str());
    externalInfo.cloudId = fileAssetPtr->GetCloudId();
    externalInfo.subType = fileAssetPtr->GetPhotoSubType();
    externalInfo.effectMode = fileAssetPtr->GetMovingPhotoEffectMode();
    externalInfo.originalSubType = fileAssetPtr->GetOriginalSubType();
    GetMovingPhotoExternalInfo(externalInfo, cleanFileInfo.attachment);
    MEDIA_DEBUG_LOG("size is %{public}" PRId64, externalInfo.size);
    externalInfo.dateModified = fileAssetPtr->GetDateModified();
    externalInfo.displayName = fileAssetPtr->GetDisplayName();
    externalInfo.editTime = fileAssetPtr->GetPhotoEditTime();
    GetEditPhotoExternalInfo(externalInfo, cleanFileInfo.attachment);
    cleanFileInfo.cloudId = externalInfo.cloudId;
    cleanFileInfo.size = externalInfo.size;
    cleanFileInfo.modifiedTime = externalInfo.dateModified;
    cleanFileInfo.path = externalInfo.path;
    cleanFileInfo.fileName = MediaFileUtils::GetFileName(externalInfo.path);
    cleanFileInfo.fileSourceType = fileAssetPtr->GetFileSourceType();
    cleanFileInfo.storagePath = fileAssetPtr->GetStoragePath();
    return cleanFileInfo;
}

static int32_t GetBurstFileInfo(const string &key, vector<CleanFileInfo> &fileInfos)
{
    MEDIA_DEBUG_LOG("GetBurstFileInfo start.");
    AbsRdbPredicates rbdPredicates(PhotoColumn::PHOTOS_TABLE);
    rbdPredicates.EqualTo(PhotoColumn::PHOTO_BURST_KEY, key);
    vector<string> columns = {
        PhotoColumn::PHOTO_CLOUD_ID,
        MediaColumn::MEDIA_SIZE,
        MediaColumn::MEDIA_DATE_MODIFIED,
        MediaColumn::MEDIA_FILE_PATH,
        MediaColumn::MEDIA_NAME,
        MediaColumn::MEDIA_ID,
        PhotoColumn::PHOTO_POSITION,
        PhotoColumn::PHOTO_BURST_KEY,
        PhotoColumn::PHOTO_SUBTYPE,
        PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
        PhotoColumn::PHOTO_ORIGINAL_SUBTYPE,
        PhotoColumn::PHOTO_EDIT_TIME,
        PhotoColumn::PHOTO_FILE_SOURCE_TYPE,
        PhotoColumn::PHOTO_STORAGE_PATH,
    };

    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(rbdPredicates, columns);
    vector<shared_ptr<FileAsset>> fileAssetVector;
    GetFileAssetsFromResultSet(resultSet, columns, fileAssetVector);
    CHECK_AND_RETURN_RET_LOG(!fileAssetVector.empty(), E_HAS_DB_ERROR,
        "GetBurstFileInfo fileAssetVector is empty.");
    for (auto& fileAssetPtr : fileAssetVector) {
        fileInfos.push_back(MediaLibraryAssetOperations::GetCleanFileInfo(fileAssetPtr));
    }
    return E_OK;
}

static void BatchDeleteLocalAndCloud(const vector<CleanFileInfo> &fileInfos, const map<string, int32_t> &notifyMap)
{
    CHECK_AND_RETURN_LOG(!fileInfos.empty(), "Batch delete local and cloud fileInfo is empty.");
    vector<string> failCloudId;
    MediaLibraryTracer tracer;
    tracer.Start("BatchDeleteLocalAndCloud");
    auto ret = CloudSyncManager::GetInstance().BatchCleanFile(fileInfos, failCloudId);
    if (ret != 0) {
        MEDIA_ERR_LOG("Failed to delete local and cloud photos permanently.");
        return;
    }

    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "watch is nullptr");
    for (const auto &fileInfo : fileInfos) {
        string cloudId = fileInfo.cloudId;
        if (find(failCloudId.begin(), failCloudId.end(), cloudId) != failCloudId.end()) {
            MEDIA_ERR_LOG("Failed to delete, cloudId is %{public}s.", cloudId.c_str());
            continue;
        }
        if (notifyMap.find(cloudId) != notifyMap.end()) {
            watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(notifyMap.at(cloudId)), NotifyType::NOTIFY_UPDATE);
        }
    }
}

static int32_t DeleteLocalAndCloudPhotos(vector<shared_ptr<FileAsset>> &subFileAsset)
{
    MediaLibraryTracer tracer;
    tracer.Start("DeleteLocalAndCloudPhotos");
    MEDIA_DEBUG_LOG("DeleteLocalAndCloudPhotos start.");
    vector<CleanFileInfo> fileInfos;
    if (subFileAsset.empty()) {
        MEDIA_INFO_LOG("DeleteLocalAndCloudPhotos subFileAsset is empty.");
        return E_OK;
    }

    map<string, int32_t> notifyMap;
    for (auto& fileAssetPtr : subFileAsset) {
        CHECK_AND_CONTINUE(fileAssetPtr != nullptr);
        notifyMap[fileAssetPtr->GetCloudId()] = fileAssetPtr->GetId();
        string burst_key = fileAssetPtr->GetBurstKey();
        if (burst_key != "") {
            GetBurstFileInfo(burst_key, fileInfos);
            continue;
        }
        fileInfos.push_back(MediaLibraryAssetOperations::GetCleanFileInfo(fileAssetPtr));
    }
    vector<CleanFileInfo> subFileInfo;
    int32_t count = 0;
    for (const auto& element : fileInfos) {
        MEDIA_INFO_LOG("delete local and cloud file displayName is %{public}s", element.fileName.c_str());
        subFileInfo.push_back(element);
        count++;
        if (count == MAX_PROCESS_NUM) {
            BatchDeleteLocalAndCloud(subFileInfo, notifyMap);
            subFileInfo.clear();
            count = 0;
        }
    }
    if (!subFileInfo.empty()) {
        BatchDeleteLocalAndCloud(subFileInfo, notifyMap);
    }
    return E_OK;
}

static int32_t DeleteDbByIds(const string &table, vector<string> &ids, const bool compatible,
    shared_ptr<AccurateRefresh::AssetAccurateRefresh> refresh)
{
    AbsRdbPredicates predicates(table);
    predicates.In(MediaColumn::MEDIA_ID, ids);
    if (!compatible) {
        predicates.GreaterThan(MediaColumn::MEDIA_DATE_TRASHED, to_string(0));
    }
    int32_t deletedRows = 0;
    int32_t err = refresh->LogicalDeleteReplaceByUpdate(predicates, deletedRows);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to execute delete, err: %{public}d", err);
        MediaLibraryRestore::GetInstance().CheckRestore(err);
        return E_HAS_DB_ERROR;
    }
    CloudSyncHelper::GetInstance()->StartSync();
    return deletedRows;
}

static void GetAlbumNamesById(DeletedFilesParams &filesParams)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetAlbumNamesById");
    CHECK_AND_RETURN_LOG(!filesParams.ownerAlbumIds.empty(), "ownerAlbumIds is empty");
    vector<string> ownerAlbumIdList;
    set<string> albumIdSet;
    for (const auto &fileId : filesParams.ownerAlbumIds) {
        albumIdSet.insert(fileId.second);
    }
    ownerAlbumIdList.assign(albumIdSet.begin(), albumIdSet.end());
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(uniStore != nullptr, "rdbstore is nullptr");
    MediaLibraryCommand queryAlbumMapCmd(OperationObject::PAH_ALBUM, OperationType::QUERY);
    queryAlbumMapCmd.GetAbsRdbPredicates()->In(PhotoAlbumColumns::ALBUM_ID, ownerAlbumIdList);
    auto resultSet = uniStore->Query(queryAlbumMapCmd, {PhotoAlbumColumns::ALBUM_ID, PhotoAlbumColumns::ALBUM_NAME});
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Failed to query resultSet");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        filesParams.albumNames.insert({
            to_string(get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_ID, resultSet,
                TYPE_INT32))),
            get<string>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_NAME, resultSet,
                TYPE_STRING))});
    }
    resultSet->Close();
}

void notifyOld(const std::vector<std::string> &notifyUris, std::vector<int32_t> &isTemps)
{
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
    int trashAlbumId = watch->GetAlbumIdBySubType(PhotoAlbumSubType::TRASH);
    if (trashAlbumId <= 0) {
        MEDIA_WARN_LOG("Failed to get trash album id: %{public}d", trashAlbumId);
        return;
    }
    size_t uriSize = notifyUris.size() > isTemps.size() ? isTemps.size() :
        notifyUris.size();
    for (size_t index = 0; index < uriSize; index++) {
        if (isTemps[index]) {
            continue;
        }
        watch->Notify(MediaFileUtils::Encode(notifyUris[index]), NotifyType::NOTIFY_ALBUM_REMOVE_ASSET,
            trashAlbumId);
    }
}

/**
 * @brief Delete files permanently from system.
 *
 * @param predicates Files to delete.
 * @param isAging Whether in aging process.
 * @param compatible API8 interfaces can delete files directly without trash.
 *   true: Delete files, may including non-trashed files.
 *   false: Only delete files that were already trashed.
 * @return Return deleted rows
 */
int32_t MediaLibraryAssetOperations::DeleteFromDisk(AbsRdbPredicates &predicates,
    const bool isAging, const bool compatible)
{
    MediaLibraryTracer tracer;
    tracer.Start("DeleteFromDisk");
    MEDIA_INFO_LOG("DeleteFromDisk start");
    vector<string> whereArgs = predicates.GetWhereArgs();
    MediaLibraryRdbStore::ReplacePredicatesUriToId(predicates);
    vector<string> agingNotifyUris;

    // Query asset uris for notify before delete.
    if (isAging) {
        MediaLibraryNotify::GetNotifyUris(predicates, agingNotifyUris);
    }
    DeletedFilesParams fileParams;
    int32_t deletedRows = 0;
    int32_t ret = QueryFileInfoAndHandleRemovePhotos(predicates, fileParams);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_HAS_DB_ERROR, "query db error");
    SaveDeletedFile(fileParams.ids, fileParams.paths, predicates.GetTableName(),
        fileParams.dateTakens, fileParams.subTypes);
    GetAlbumNamesById(fileParams);
    CHECK_AND_RETURN_RET_LOG(!fileParams.ids.empty(), deletedRows, "Failed to delete files in db, ids size: 0");

    // delete cloud enhanacement task
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
    vector<string> photoIds;
    EnhancementManager::GetInstance().RemoveTasksInternal(fileParams.ids, photoIds);
#endif
    auto assetRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>(
        AccurateRefresh::DELETE_PHOTOS_BUSSINESS_NAME);
    deletedRows = DeleteDbByIds(predicates.GetTableName(), fileParams.ids, compatible, assetRefresh);
    CHECK_AND_RETURN_RET_LOG(deletedRows > 0, deletedRows,
        "Failed to delete files in db, deletedRows: %{public}d, ids size: %{public}zu",
        deletedRows, fileParams.ids.size());

    MEDIA_INFO_LOG("Delete files in db, deletedRows: %{public}d", deletedRows);
    assetRefresh->RefreshAlbum();
    assetRefresh->Notify();
    auto asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    CHECK_AND_RETURN_RET_LOG(asyncWorker != nullptr, E_ERR, "Can not get asyncWorker");

    vector<string> &notifyUris = isAging ? agingNotifyUris : whereArgs;
    string bundleName = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    auto *taskData = new (nothrow) DeleteFilesData(fileParams.ids, fileParams.paths, fileParams.dateTakens,
        fileParams.subTypes, predicates.GetTableName(), deletedRows, bundleName, fileParams.containsHidden);
    CHECK_AND_RETURN_RET_LOG(taskData != nullptr, E_ERR, "Failed to alloc async data for Delete From Disk!");
    notifyOld(notifyUris, fileParams.isTemps);
    DeleteBehaviorData dataInfo {fileParams.displayNames, fileParams.albumNames, fileParams.ownerAlbumIds};
    DfxManager::GetInstance()->HandleDeleteBehavior(DfxType::ALBUM_DELETE_ASSETS, deletedRows, notifyUris, bundleName,
        dataInfo);
    auto deleteFilesTask = make_shared<MediaLibraryAsyncTask>(DeleteFiles, taskData);
    CHECK_AND_RETURN_RET_LOG(deleteFilesTask != nullptr, E_ERR, "Failed to create async task for deleting files.");
    asyncWorker->AddTask(deleteFilesTask, true);
    
    CloudMediaAssetManager::GetInstance().SetIsThumbnailUpdate();
    return deletedRows;
}

static int32_t GetAlbumTypeSubTypeById(const string &albumId, PhotoAlbumType &type, PhotoAlbumSubType &subType)
{
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    vector<string> columns = { PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumColumns::ALBUM_SUBTYPE };
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("album id %{private}s is not exist", albumId.c_str());
        return E_INVALID_ARGUMENTS;
    }
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK, E_INVALID_ARGUMENTS,
        "album id is not exist");
    type = static_cast<PhotoAlbumType>(GetInt32Val(PhotoAlbumColumns::ALBUM_TYPE, resultSet));
    subType = static_cast<PhotoAlbumSubType>(GetInt32Val(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet));
    resultSet->Close();
    return E_SUCCESS;
}

static void NotifyPhotoAlbum(const vector<int32_t> &changedAlbumIds,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh)
{
    if (changedAlbumIds.size() <= 0) {
        return;
    }
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Can not get rdbstore");
        return;
    }
    for (int32_t albumId : changedAlbumIds) {
        MEDIA_DEBUG_LOG("NotifyPortraitAlbum album id is %{public}d", albumId);
        PhotoAlbumType type;
        PhotoAlbumSubType subType;
        int32_t ret = GetAlbumTypeSubTypeById(to_string(albumId), type, subType);
        if (ret != E_SUCCESS) {
            MEDIA_ERR_LOG("Get album type and subType by album id failed");
            continue;
        }
    }
    if (assetRefresh == nullptr) {
        MEDIA_ERR_LOG("assetRefresh is nullptr");
        return;
    }
    assetRefresh->RefreshAlbum(static_cast<NotifyAlbumType>(NotifyAlbumType::SYS_ALBUM | NotifyAlbumType::USER_ALBUM |
        NotifyAlbumType::SOURCE_ALBUM));
    assetRefresh->Notify();
}

int32_t MediaLibraryAssetOperations::DeleteNormalPhotoPermanently(shared_ptr<FileAsset> &fileAsset,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh)
{
    MediaLibraryTracer tracer;
    tracer.Start("DeleteNormalPhotoPermanently");
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_HAS_DB_ERROR,
        "Photo Asset is nullptr");
    string filePath = fileAsset->GetPath();
    string displayName = fileAsset->GetDisplayName();
    MEDIA_INFO_LOG("Delete Photo displayName is %{public}s", displayName.c_str());
    MEDIA_DEBUG_LOG("Delete Photo path is %{public}s", filePath.c_str());
    CHECK_AND_RETURN_RET_LOG(!filePath.empty(), E_INVALID_PATH, "get file path failed");
    FileUtils::DeleteTempVideoFile(filePath);
    bool res = LakeFileUtils::DeleteFile(filePath);
    CHECK_AND_RETURN_RET_LOG(res, E_HAS_FS_ERROR, "Delete photo file failed, errno: %{public}d", errno);

    //delete thumbnail
    int32_t fileId = fileAsset->GetId();
    MediaLibraryAssetOperations::InvalidateThumbnail(to_string(fileId), fileAsset->GetMediaType());

    // delete file in db
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::DELETE);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    int32_t deleteRows = DeleteAssetInDb(cmd, assetRefresh);
    MEDIA_DEBUG_LOG("Total delete row in db is %{public}d", deleteRows);
    CHECK_AND_RETURN_RET_LOG(deleteRows > 0, E_HAS_DB_ERROR,
        "Delete photo in database failed, errCode=%{public}d", deleteRows);

    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_HAS_FS_ERROR, "watch is nullptr");
    string notifyDeleteUri =
        MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, to_string(fileId),
            MediaFileUtils::GetExtraUri(displayName, filePath));
    watch->Notify(notifyDeleteUri, NotifyType::NOTIFY_REMOVE);
    std::vector<std::string> notifyDeleteUris;
    notifyDeleteUris.push_back(notifyDeleteUri);
    auto dfxManager = DfxManager::GetInstance();
    if (dfxManager != nullptr) {
        DeletedFilesParams fileParams;
        fileParams.ownerAlbumIds.insert({to_string(fileId), to_string(fileAsset->GetOwnerAlbumId())});
        GetAlbumNamesById(fileParams);
        DeleteBehaviorData dataInfo {{{to_string(fileId), displayName}},
            fileParams.albumNames, fileParams.ownerAlbumIds };
        dfxManager->HandleDeleteBehavior(DfxType::DELETE_LOCAL_ASSETS_PERMANENTLY, deleteRows,
            notifyDeleteUris, "", dataInfo);
    }
    MediaLibraryPhotoOperations::DeleteRevertMessage(filePath);
    return E_OK;
}

static int32_t DeletePhotoPermanentlyFromVector(vector<shared_ptr<FileAsset>> &fileAssetVector,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh)
{
    for (auto& fileAssetPtr : fileAssetVector) {
        MEDIA_DEBUG_LOG("Delete photo display name %{public}s", fileAssetPtr->GetDisplayName().c_str());
        CHECK_AND_RETURN_RET_LOG(fileAssetPtr != nullptr, E_HAS_DB_ERROR,
            "Photo Asset is nullptr");
        MediaLibraryAssetOperations::DeleteNormalPhotoPermanently(fileAssetPtr, assetRefresh);
    }
    return E_OK;
}

static int32_t DeleteBurstPhotoPermanently(shared_ptr<FileAsset> &fileAsset,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh)
{
    MEDIA_DEBUG_LOG("DeleteBurstPhotoPermanently begin");
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_HAS_DB_ERROR,
        "Photo Asset is nullptr");
    string burstKey = fileAsset->GetBurstKey();
    MEDIA_DEBUG_LOG("Deleteburst is %{public}s", burstKey.c_str());
    CHECK_AND_RETURN_RET(burstKey != "", E_OK);

    vector<shared_ptr<FileAsset>> burstFileAssetVector;
    AbsRdbPredicates rbdPredicates(PhotoColumn::PHOTOS_TABLE);
    rbdPredicates.EqualTo(PhotoColumn::PHOTO_BURST_KEY, burstKey);
    vector<string> columns = {
        MediaColumn::MEDIA_FILE_PATH,
        MediaColumn::MEDIA_ID,
        MediaColumn::MEDIA_TYPE,
    };
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(rbdPredicates, columns);
    GetFileAssetsFromResultSet(resultSet, columns, burstFileAssetVector);
    DeletePhotoPermanentlyFromVector(burstFileAssetVector, assetRefresh);
    return E_OK;
}

static int32_t DeleteMovingPhotoPermanently(shared_ptr<FileAsset> &fileAsset)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_HAS_DB_ERROR, "Photo Asset is nullptr");
    int32_t subType = fileAsset->GetPhotoSubType();
    int32_t effectMode = fileAsset->GetMovingPhotoEffectMode();
    int32_t originalSubType = fileAsset->GetOriginalSubType();
    if (MovingPhotoFileUtils::IsMovingPhoto(subType, effectMode, originalSubType)) {
        string path = fileAsset->GetPath();
        int32_t id = fileAsset->GetId();
        MultiStagesVideoCaptureManager::GetInstance().RemoveVideo(std::to_string(id), path, subType, false);
        string videoPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(path);
        if (!MediaFileUtils::DeleteFile(videoPath)) {
            MEDIA_INFO_LOG("delete video path is %{public}s, errno: %{public}d",
                DfxUtils::GetSafePath(videoPath).c_str(), errno);
        }
        string exVideoPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(path);
        if (!MediaFileUtils::DeleteFile(exVideoPath)) {
            MEDIA_INFO_LOG("delete extra video path is %{public}s, errno: %{public}d",
                DfxUtils::GetSafePath(exVideoPath).c_str(), errno);
        }
        string sourceImagePath = MovingPhotoFileUtils::GetSourceMovingPhotoImagePath(path);
        if (!MediaFileUtils::DeleteFile(sourceImagePath)) {
            MEDIA_INFO_LOG("delete source image path is %{public}s, errno: %{public}d",
                DfxUtils::GetSafePath(sourceImagePath).c_str(), errno);
        }
        string sourceVideoPath = MovingPhotoFileUtils::GetSourceMovingPhotoVideoPath(path);
        if (!MediaFileUtils::DeleteFile(sourceVideoPath)) {
            MEDIA_INFO_LOG("delete source video path is %{public}s, errno: %{public}d",
                DfxUtils::GetSafePath(sourceVideoPath).c_str(), errno);
        }
        string livePhotoCachePath = MovingPhotoFileUtils::GetLivePhotoCachePath(path);
        if (!MediaFileUtils::DeleteFile(livePhotoCachePath)) {
            MEDIA_INFO_LOG("delete live photo cache path is %{public}s, errno: %{public}d",
                DfxUtils::GetSafePath(livePhotoCachePath).c_str(), errno);
        }
    }
    return E_OK;
}

static int32_t DeleteEditPhotoPermanently(shared_ptr<FileAsset> &fileAsset)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_HAS_DB_ERROR,
        "Photo Asset is nullptr");
    int64_t editTime = fileAsset->GetPhotoEditTime();
    if (editTime != 0) {
        string path = fileAsset->GetPath();
        string editDataPath = PhotoFileUtils::GetEditDataPath(path);
        MEDIA_DEBUG_LOG("edit photo editDataPath path is %{public}s", editDataPath.c_str());
        if (!MediaFileUtils::DeleteFile(editDataPath)) {
            MEDIA_INFO_LOG("delete edit data path is %{public}s, errno: %{public}d",
                DfxUtils::GetSafePath(editDataPath).c_str(), errno);
        }
        string editDataCameraPath = PhotoFileUtils::GetEditDataCameraPath(path);
        if (!MediaFileUtils::DeleteFile(editDataCameraPath)) {
            MEDIA_INFO_LOG("delete edit data camera path is %{public}s, errno: %{public}d",
                DfxUtils::GetSafePath(editDataCameraPath).c_str(), errno);
        }
    }
    return E_OK;
}

static int32_t DeleteTransCodePhotoPermanently(shared_ptr<FileAsset> &fileAsset)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_HAS_DB_ERROR,
        "Photo Asset is nullptr");
    string path = fileAsset->GetPath();
    string transCodePath = PhotoFileUtils::GetTransCodePath(path);
    MEDIA_DEBUG_LOG("transCodePath path is %{public}s", transCodePath.c_str());
    if (!MediaFileUtils::IsFileExists(transCodePath)) {
        return E_OK;
    }
    if (!MediaFileUtils::DeleteFile(transCodePath)) {
        MEDIA_INFO_LOG("[deletePermanently] delete transCodePath path is %{public}s, errno: %{public}d",
            DfxUtils::GetSafePath(transCodePath).c_str(), errno);
    } else {
        MEDIA_INFO_LOG("[deletePermanently] Delete transCode file Success!");
    }
    return E_OK;
}

static int32_t DeleteLocalPhotoPermanently(shared_ptr<FileAsset> &fileAsset,
    vector<shared_ptr<FileAsset>> &subFileAssetVector,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_HAS_DB_ERROR,
        "Photo Asset is nullptr");
    int32_t position = fileAsset->GetPosition();
    MEDIA_DEBUG_LOG("Photo position is %{public}d", position);
    if (position == LOCAL_PHOTO_POSITION) {
        int32_t id = fileAsset->GetId();
        CHECK_AND_PRINT_LOG(DeleteEditPhotoPermanently(fileAsset) == E_OK,
            "Delete edit photo file failed id %{public}d", id);

        CHECK_AND_PRINT_LOG(DeleteTransCodePhotoPermanently(fileAsset) == E_OK,
            "Delete transCode photo file failed id %{public}d", id);

        CHECK_AND_PRINT_LOG(DeleteMovingPhotoPermanently(fileAsset) == E_OK,
            "Delete moving photo file failed id %{public}d", id);

        CHECK_AND_PRINT_LOG(DeleteBurstPhotoPermanently(fileAsset, assetRefresh) == E_OK,
            "Delete burst photo file failed id %{public}d", id);

        CHECK_AND_PRINT_LOG(MediaLibraryAssetOperations::DeleteNormalPhotoPermanently(fileAsset, assetRefresh) == E_OK,
            "Delete normal photo file failed id %{public}d", id);
    }
    if (position == CLOUD_PHOTO_POSITION) {
        MEDIA_DEBUG_LOG("Don't delete cloud Photo");
    }
    if (position == BOTH_LOCAL_CLOUD_PHOTO_POSITION) {
        if (fileAsset->GetExistCompatibleDuplicate() != 0) {
            MediaLibraryTranscodeDataAgingOperation::DeleteTransCodeInfo(fileAsset->GetFilePath(),
                to_string(fileAsset->GetId()), __func__);
        }
        subFileAssetVector.push_back(fileAsset);
    }
    return E_OK;
}

int32_t MediaLibraryAssetOperations::AddOtherBurstIdsToFileIds(std::vector<std::string> &fileIds)
{
    #ifdef MEDIALIBRARY_CLOUD_SYNC_SERVICE_SUPPORT
    CHECK_AND_RETURN_RET_LOG(!fileIds.empty(), E_ERR, "AddOtherBurstIdsToFileIds No uris");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "QueryDownloadResources Failed to get rdbStore.");
    std::string sqlBefore = "SELECT p1." + PhotoColumn::MEDIA_ID + ", p1." + PhotoColumn::PHOTO_BURST_KEY +
        ", p2." + PhotoColumn::MEDIA_ID + " AS related_file_id FROM " +
        PhotoColumn::PHOTOS_TABLE +" p1 JOIN "+ PhotoColumn::PHOTOS_TABLE +
        " p2 ON p1." + PhotoColumn::PHOTO_BURST_KEY + " = p2." + PhotoColumn::PHOTO_BURST_KEY +
        " WHERE p1." + PhotoColumn::MEDIA_ID + " IN ({0}) AND p1." + PhotoColumn::PHOTO_BURST_COVER_LEVEL +
        " = 1 AND p1." + PhotoColumn::PHOTO_SUBTYPE + " = " + to_string(static_cast<int32_t>(PhotoSubType::BURST)) +
        " AND p1." + PhotoColumn::PHOTO_BURST_KEY + " IS NOT NULL AND p1." + PhotoColumn::MEDIA_ID +
        " != p2." + PhotoColumn::MEDIA_ID;
    std::string inClause = CloudMediaCommon::ToStringWithComma(fileIds);
    std::string sql = CloudMediaCommon::FillParams(sqlBefore, {inClause});
    std::shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(sql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "Failed to query batch selected files!");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t relatedFileId = get<int32_t>(ResultSetUtils::GetValFromColumn("related_file_id",
            resultSet, TYPE_INT32));
        MEDIA_INFO_LOG("BatchSelectFileDownload Add burst relatedFileId %{public}d", relatedFileId);
        fileIds.emplace_back(std::to_string(relatedFileId));
    }
    resultSet->Close();
    #endif
    return NativeRdb::E_OK;
}

#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
int32_t MediaLibraryAssetOperations::DealWithBatchDownloadingFilesById(std::vector<std::string> &fileIds)
{
    MEDIA_INFO_LOG("BatchSelectFileDownload DealWithBatchDownloadingFilesById ids In"); // 自动取消
    // 检查点 批量下载 通知应用 notify type 3 任务应该被通知并删任务表
    std::vector<std::string> needStopDownloadFileIds;
    for (auto& fileId : fileIds) {
        needStopDownloadFileIds.push_back(fileId);
    }
    AddOtherBurstIdsToFileIds(needStopDownloadFileIds);
    BackgroundCloudBatchSelectedFileProcessor::TriggerCancelBatchDownloadProcessor(needStopDownloadFileIds, true);
    MEDIA_INFO_LOG("BatchSelectFileDownload ManualCancel DealWithBatchDownloadingFilesById");
    return E_OK;
}

int32_t MediaLibraryAssetOperations::DealWithBatchDownloadingFiles(vector<shared_ptr<FileAsset>> &subFileAssetVector)
{
    MEDIA_INFO_LOG("BatchSelectFileDownload DealWithBatchDownloadingFiles FileAsset In");
    std::vector<std::string> needStopDownloadFileIds;
    for (auto& fileAssetPtr : subFileAssetVector) {
        needStopDownloadFileIds.push_back(to_string(fileAssetPtr->GetId()));
    }
    return DealWithBatchDownloadingFilesById(needStopDownloadFileIds);
}
#endif

static void GetAnalysisAlbumIdsOfAssets(const vector<shared_ptr<FileAsset>> fileAssetVector, set<string>& albumIds)
{
    vector<string> assetIds;
    for (auto& fileAssetPtr : fileAssetVector) {
        CHECK_AND_CONTINUE(fileAssetPtr != nullptr);
        assetIds.push_back(std::to_string(fileAssetPtr->GetId()));
    }
    MediaLibraryRdbUtils::QueryAnalysisAlbumIdOfAssets(assetIds, albumIds);
    MEDIA_INFO_LOG("Number of Analysis Album is %{public}zu", albumIds.size());
}

int32_t MediaLibraryAssetOperations::DeletePermanently(AbsRdbPredicates &predicates, const bool isAging,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh)
{
    if (assetRefresh == nullptr) {
        assetRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>(
            AccurateRefresh::DELETE_PHOTOS_COMPLETED_BUSSINESS_NAME);
    }
    MEDIA_DEBUG_LOG("DeletePermanently begin");
    MediaLibraryTracer tracer;
    tracer.Start("DeletePermanently");
    MediaLibraryRdbStore::ReplacePredicatesUriToId(predicates);
    vector<string> agingNotifyUris;

    // Query asset uris for notify before delete.
    if (isAging) {
        MediaLibraryNotify::GetNotifyUris(predicates, agingNotifyUris);
    }

    vector<string> columns = {
        PhotoColumn::PHOTO_CLOUD_ID, MediaColumn::MEDIA_SIZE, MediaColumn::MEDIA_DATE_MODIFIED,
        MediaColumn::MEDIA_FILE_PATH, MediaColumn::MEDIA_NAME, MediaColumn::MEDIA_ID,
        PhotoColumn::PHOTO_POSITION, PhotoColumn::PHOTO_BURST_KEY, MediaColumn::MEDIA_TYPE,
        PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::MOVING_PHOTO_EFFECT_MODE, PhotoColumn::PHOTO_ORIGINAL_SUBTYPE,
        PhotoColumn::PHOTO_EDIT_TIME, PhotoColumn::PHOTO_OWNER_ALBUM_ID, PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE,
        PhotoColumn::PHOTO_FILE_SOURCE_TYPE, PhotoColumn::PHOTO_STORAGE_PATH,
    };
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    vector<shared_ptr<FileAsset>> fileAssetVector;
    vector<shared_ptr<FileAsset>> subFileAssetVector;
    std::set<int32_t> changedAlbumIds;
    GetAssetVectorFromResultSet(resultSet, columns, fileAssetVector);
    if (resultSet != nullptr) {
        resultSet->Close();
    }
    set<string> analysisAlbumIds;
    GetAnalysisAlbumIdsOfAssets(fileAssetVector, analysisAlbumIds);
    for (auto& fileAssetPtr : fileAssetVector) {
        DeleteLocalPhotoPermanently(fileAssetPtr, subFileAssetVector, assetRefresh);
        changedAlbumIds.insert(fileAssetPtr->GetOwnerAlbumId());
    }
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    MEDIA_INFO_LOG("BatchSelectFileDownload DeletePermanently DealWithBatchDownloadingFiles");
    // 检查点 批量下载 本地删除 停止并清理 通知应用 notify type 3
    DealWithBatchDownloadingFiles(fileAssetVector);
#endif
    //delete both local and cloud image
    DeleteLocalAndCloudPhotos(subFileAssetVector);
    vector<string> albumIds(analysisAlbumIds.begin(), analysisAlbumIds.end());
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (albumIds.size() > 0 && rdbStore != nullptr) {
        MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(rdbStore, albumIds);
    }
    NotifyPhotoAlbum(std::vector<int32_t>(changedAlbumIds.begin(), changedAlbumIds.end()), assetRefresh);
    return E_OK;
}
} // namespace Media
} // namespace OHOS

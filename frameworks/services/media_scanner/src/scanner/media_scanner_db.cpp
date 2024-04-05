/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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
#define MLOG_TAG "Scanner"

#include "media_scanner_db.h"

#include "abs_rdb_predicates.h"
#include "ipc_skeleton.h"
#include "medialibrary_asset_operations.h"
#include "media_column.h"
#include "media_error_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_command.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_smartalbum_map_operations.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "rdb_errno.h"
#include "rdb_utils.h"
#include "result_set.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"
#include "userfilemgr_uri.h"
#include "values_bucket.h"
#include "post_event_utils.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;

MediaScannerDb::MediaScannerDb() {}

unique_ptr<MediaScannerDb> MediaScannerDb::GetDatabaseInstance()
{
    unique_ptr<MediaScannerDb> database = make_unique<MediaScannerDb>();
    return database;
}

void MediaScannerDb::SetRdbHelper(void)
{
}

#ifdef MEDIALIBRARY_COMPATIBILITY
static inline void SetVirtualPath(const Metadata &metadata, ValuesBucket &values)
{
    string relativePath = metadata.GetRelativePath();
    string displayName = metadata.GetFileName();
    string virtualPath = (relativePath.back() == '/' ? relativePath : relativePath + "/") + displayName;
    values.PutString(MediaColumn::MEDIA_VIRTURL_PATH, virtualPath);
}
#endif

static inline void SetRemainFileMetadataApi9(const Metadata &metadata, ValuesBucket &values)
{
    values.PutString(MEDIA_DATA_DB_AUDIO_ALBUM, metadata.GetAlbum());
    values.PutString(MEDIA_DATA_DB_ARTIST, metadata.GetFileArtist());
    values.PutInt(MEDIA_DATA_DB_HEIGHT, metadata.GetFileHeight());
    values.PutInt(MEDIA_DATA_DB_WIDTH, metadata.GetFileWidth());
    values.PutInt(MEDIA_DATA_DB_ORIENTATION, metadata.GetOrientation());
    values.PutString(MEDIA_DATA_DB_BUCKET_NAME, metadata.GetAlbumName());
    values.PutInt(MEDIA_DATA_DB_PARENT_ID, metadata.GetParentId());
    values.PutInt(MEDIA_DATA_DB_BUCKET_ID, metadata.GetParentId());
    values.PutDouble(MEDIA_DATA_DB_LATITUDE, metadata.GetLatitude());
    values.PutDouble(MEDIA_DATA_DB_LONGITUDE, metadata.GetLongitude());
}

static void SetValuesFromMetaDataAndType(const Metadata &metadata, ValuesBucket &values, MediaType mediaType,
    const string &tableName)
{
#ifdef MEDIALIBRARY_COMPATIBILITY
    if (mediaType == MediaType::MEDIA_TYPE_IMAGE || mediaType == MEDIA_TYPE_VIDEO) {
        if (tableName == MEDIALIBRARY_TABLE) {
            SetRemainFileMetadataApi9(metadata, values);
        } else {
            values.PutInt(MEDIA_DATA_DB_HEIGHT, metadata.GetFileHeight());
            values.PutInt(MEDIA_DATA_DB_WIDTH, metadata.GetFileWidth());
            values.PutInt(MEDIA_DATA_DB_ORIENTATION, metadata.GetOrientation());
            values.PutDouble(MEDIA_DATA_DB_LATITUDE, metadata.GetLatitude());
            values.PutDouble(MEDIA_DATA_DB_LONGITUDE, metadata.GetLongitude());
            SetVirtualPath(metadata, values);
            if (metadata.GetPhotoSubType() != 0) {
                values.PutInt(PhotoColumn::PHOTO_SUBTYPE, metadata.GetPhotoSubType());
            }
        }
    } else if (mediaType == MediaType::MEDIA_TYPE_AUDIO) {
        if (tableName == MEDIALIBRARY_TABLE) {
            SetRemainFileMetadataApi9(metadata, values);
        } else {
            values.PutString(MEDIA_DATA_DB_AUDIO_ALBUM, metadata.GetAlbum());
            values.PutString(MEDIA_DATA_DB_ARTIST, metadata.GetFileArtist());
            SetVirtualPath(metadata, values);
        }
    } else {
        SetRemainFileMetadataApi9(metadata, values);
    }
#else
    SetRemainFileMetadataApi9(metadata, values);
#endif
}

static inline void SetDateDay(const int64_t dateAdded, ValuesBucket &outValues)
{
    outValues.PutString(PhotoColumn::PHOTO_DATE_YEAR,
        MediaFileUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DATE_YEAR_FORMAT, dateAdded));
    outValues.PutString(PhotoColumn::PHOTO_DATE_MONTH,
        MediaFileUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DATE_MONTH_FORMAT, dateAdded));
    outValues.PutString(PhotoColumn::PHOTO_DATE_DAY,
        MediaFileUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DATE_DAY_FORMAT, dateAdded));
}

static inline void SetDateAdded(const int64_t dateAdded, const Metadata &metadata, ValuesBucket &outValues)
{
    outValues.PutLong(MediaColumn::MEDIA_DATE_ADDED, dateAdded);
    MediaType type = metadata.GetFileMediaType();
    if ((type != MEDIA_TYPE_PHOTO) && (type != MEDIA_TYPE_IMAGE) && (type != MEDIA_TYPE_VIDEO)) {
        return;
    }
    SetDateDay(dateAdded, outValues);
}

static void InsertDateAdded(const Metadata &metadata, ValuesBucket &outValues)
{
    int64_t dateAdded = metadata.GetFileDateAdded();
    if (dateAdded == 0) {
        int64_t dateTaken = metadata.GetDateTaken();
        if (dateTaken == 0) {
            int64_t dateModified = metadata.GetFileDateModified();
            if (dateModified == 0) {
                dateAdded = MediaFileUtils::UTCTimeMilliSeconds();
                MEDIA_WARN_LOG("Invalid dateAdded time, use current time instead: %{public}lld",
                    static_cast<long long>(dateAdded));
            } else {
                dateAdded = dateModified;
                MEDIA_WARN_LOG("Invalid dateAdded time, use dateModified instead: %{public}lld",
                    static_cast<long long>(dateAdded));
            }
        } else {
            dateAdded = dateTaken * MSEC_TO_SEC;
            MEDIA_WARN_LOG("Invalid dateAdded time, use dateTaken instead: %{public}lld",
                static_cast<long long>(dateAdded));
        }
    }
    SetDateAdded(dateAdded, metadata, outValues);
}

static inline void FixDateDayIfNeeded(const Metadata &metadata, ValuesBucket &outValues)
{
    MediaType type = metadata.GetFileMediaType();
    if ((type != MEDIA_TYPE_PHOTO) && (type != MEDIA_TYPE_IMAGE) && (type != MEDIA_TYPE_VIDEO)) {
        return;
    }
    int64_t dateAdded = metadata.GetFileDateAdded();
    string dateDayOld = metadata.GetDateDay();
    string dateDayNew = MediaFileUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DATE_DAY_FORMAT, dateAdded);
    if (dateDayOld != dateDayNew) {
        SetDateDay(dateAdded, outValues);
    }
}

static inline void HandleDateAdded(const Metadata &metadata, const bool isInsert, ValuesBucket &outValues)
{
    if (isInsert) {
        InsertDateAdded(metadata, outValues);
        return;
    }

    FixDateDayIfNeeded(metadata, outValues);
}

static void SetValuesFromMetaDataApi9(const Metadata &metadata, ValuesBucket &values, bool isInsert,
    const string &table)
{
    MediaType mediaType = metadata.GetFileMediaType();
    values.PutString(MEDIA_DATA_DB_FILE_PATH, metadata.GetFilePath());
    values.PutString(MEDIA_DATA_DB_RELATIVE_PATH, metadata.GetRelativePath());
    values.PutString(MEDIA_DATA_DB_MIME_TYPE, metadata.GetFileMimeType());
    values.PutInt(MEDIA_DATA_DB_MEDIA_TYPE, mediaType);
    values.PutString(MEDIA_DATA_DB_NAME, metadata.GetFileName());
    values.PutString(MEDIA_DATA_DB_TITLE, metadata.GetFileTitle());
    values.PutLong(MEDIA_DATA_DB_SIZE, metadata.GetFileSize());
    values.PutLong(MEDIA_DATA_DB_DATE_MODIFIED, metadata.GetFileDateModified());
    values.PutInt(MEDIA_DATA_DB_DURATION, metadata.GetFileDuration());
    values.PutLong(MEDIA_DATA_DB_DATE_TAKEN, metadata.GetDateTaken());
    values.PutLong(MEDIA_DATA_DB_TIME_PENDING, 0);

    SetValuesFromMetaDataAndType(metadata, values, mediaType, table);
    HandleDateAdded(metadata, isInsert, values);
}

static void SetValuesFromMetaDataApi10(const Metadata &metadata, ValuesBucket &values, bool isInsert,
    bool skipPhoto = true)
{
    MediaType mediaType = metadata.GetFileMediaType();

    values.PutString(MediaColumn::MEDIA_FILE_PATH, metadata.GetFilePath());
    values.PutString(MediaColumn::MEDIA_MIME_TYPE, metadata.GetFileMimeType());
    values.PutInt(MediaColumn::MEDIA_TYPE, mediaType);
    if (skipPhoto) {
        values.PutString(MediaColumn::MEDIA_NAME, metadata.GetFileName());
    }
    values.PutString(MediaColumn::MEDIA_TITLE, metadata.GetFileTitle());

    values.PutLong(MediaColumn::MEDIA_SIZE, metadata.GetFileSize());
    values.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, metadata.GetFileDateModified());
    values.PutInt(MediaColumn::MEDIA_DURATION, metadata.GetFileDuration());
    values.PutLong(MediaColumn::MEDIA_DATE_TAKEN, metadata.GetDateTaken());
    values.PutLong(MediaColumn::MEDIA_TIME_PENDING, 0);

    if (mediaType == MediaType::MEDIA_TYPE_IMAGE || mediaType == MEDIA_TYPE_VIDEO) {
        values.PutInt(PhotoColumn::PHOTO_HEIGHT, metadata.GetFileHeight());
        values.PutInt(PhotoColumn::PHOTO_WIDTH, metadata.GetFileWidth());
        values.PutInt(PhotoColumn::PHOTO_ORIENTATION, metadata.GetOrientation());
        values.PutDouble(PhotoColumn::PHOTO_LONGITUDE, metadata.GetLongitude());
        values.PutDouble(PhotoColumn::PHOTO_LATITUDE, metadata.GetLatitude());
        if (skipPhoto) {
            values.PutString(PhotoColumn::PHOTO_USER_COMMENT, metadata.GetUserComment());
        }
        values.PutString(PhotoColumn::PHOTO_ALL_EXIF, metadata.GetAllExif());
        values.PutString(PhotoColumn::PHOTO_SHOOTING_MODE, metadata.GetShootingMode());
        values.PutString(PhotoColumn::PHOTO_SHOOTING_MODE_TAG, metadata.GetShootingModeTag());
        values.PutLong(PhotoColumn::PHOTO_LAST_VISIT_TIME, metadata.GetLastVisitTime());

#ifdef MEDIALIBRARY_COMPATIBILITY
        if (metadata.GetPhotoSubType() != 0) {
            values.PutInt(PhotoColumn::PHOTO_SUBTYPE, metadata.GetPhotoSubType());
        }
#endif
    } else if (mediaType == MediaType::MEDIA_TYPE_AUDIO) {
        values.PutString(AudioColumn::AUDIO_ALBUM, metadata.GetAlbum());
        values.PutString(AudioColumn::AUDIO_ARTIST, metadata.GetFileArtist());
    }

    HandleDateAdded(metadata, isInsert, values);
}

static void GetTableNameByPath(int32_t mediaType, string &tableName, const string &path = "")
{
    if (!path.empty() && MediaFileUtils::IsFileTablePath(path)) {
        tableName = MEDIALIBRARY_TABLE;
        return;
    }
    switch (mediaType) {
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
            tableName = MEDIALIBRARY_TABLE;
            break;
        }
    }
}

bool MediaScannerDb::InsertData(const ValuesBucket values, const string &tableName, int64_t &rowNum)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("MediaDataAbility Insert functionality rdbStore is null");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return false;
    }
    auto rdbStorePtr = rdbStore->GetRaw();
    if (rdbStorePtr == nullptr) {
        MEDIA_ERR_LOG("MediaDataAbility Insert functionality rdbStorePtr is null");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return false;
    }

    int32_t result = rdbStorePtr->Insert(rowNum, tableName, values);
    if (rowNum <= 0) {
        MEDIA_ERR_LOG("MediaDataAbility Insert functionality is failed, rowNum %{public}ld", (long)rowNum);
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__},
            {KEY_ERR_CODE, static_cast<int32_t>(rowNum)}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return false;
    }

    if (result != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("MediaDataAbility Insert functionality is failed, return %{public}d", result);
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, result},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return false;
    }

    return true;
}

string MediaScannerDb::InsertMetadata(const Metadata &metadata, string &tableName, MediaLibraryApi api)
{
    MediaType mediaType = metadata.GetFileMediaType();
    string mediaTypeUri;
    ValuesBucket values;
    if (api == MediaLibraryApi::API_10) {
        mediaTypeUri = MediaFileUtils::GetMediaTypeUriV10(mediaType);
    } else {
        mediaTypeUri = MediaFileUtils::GetMediaTypeUri(mediaType);
#ifdef MEDIALIBRARY_COMPATIBILITY
        if ((mediaType != MediaType::MEDIA_TYPE_IMAGE) && (mediaType != MediaType::MEDIA_TYPE_VIDEO) &&
            (mediaType != MediaType::MEDIA_TYPE_AUDIO)) {
            values.PutString(MEDIA_DATA_DB_URI, mediaTypeUri);
        }
#else
        values.PutString(MEDIA_DATA_DB_URI, mediaTypeUri);
#endif
    }

    tableName = MEDIALIBRARY_TABLE;
    if (api == MediaLibraryApi::API_10) {
        SetValuesFromMetaDataApi10(metadata, values, true);
        GetTableNameByPath(mediaType, tableName);
    } else {
#ifdef MEDIALIBRARY_COMPATIBILITY
        GetTableNameByPath(mediaType, tableName, metadata.GetFilePath());
#endif
        SetValuesFromMetaDataApi9(metadata, values, true, tableName);
    }

    int64_t rowNum = 0;
    if (!InsertData(values, tableName, rowNum)) {
        return "";
    }

    if (mediaTypeUri.empty()) {
        return "";
    }
    if (api == MediaLibraryApi::API_10) {
        return MediaFileUtils::GetUriByExtrConditions(mediaTypeUri + "/", to_string(rowNum),
            MediaFileUtils::GetExtraUri(metadata.GetFileName(), metadata.GetFilePath())) + "?api_version=10";
    }
    return MediaFileUtils::GetUriByExtrConditions(mediaTypeUri + "/", to_string(rowNum));
}

static inline void GetUriStringInUpdate(MediaType mediaType, MediaLibraryApi api, string &mediaTypeUri,
    ValuesBucket &values)
{
    if (api == MediaLibraryApi::API_10) {
        mediaTypeUri = MediaFileUtils::GetMediaTypeUriV10(mediaType);
    } else {
        mediaTypeUri = MediaFileUtils::GetMediaTypeUri(mediaType);
#ifdef MEDIALIBRARY_COMPATIBILITY
        if ((mediaType != MediaType::MEDIA_TYPE_IMAGE) && (mediaType != MediaType::MEDIA_TYPE_VIDEO) &&
            (mediaType != MediaType::MEDIA_TYPE_AUDIO)) {
            values.PutString(MEDIA_DATA_DB_URI, mediaTypeUri);
        }
#else
        values.PutString(MEDIA_DATA_DB_URI, mediaTypeUri);
#endif
    }
}

/**
 * @brief Update single metadata in the media database
 *
 * @param metadata The metadata object which has the information about the file
 * @return string The mediatypeUri corresponding to the given metadata
 */
string MediaScannerDb::UpdateMetadata(const Metadata &metadata, string &tableName, MediaLibraryApi api, bool skipPhoto)
{
    int32_t updateCount(0);
    ValuesBucket values;
    string whereClause = MEDIA_DATA_DB_ID + " = ?";
    vector<string> whereArgs = { to_string(metadata.GetFileId()) };
    MediaType mediaType = metadata.GetFileMediaType();
    string mediaTypeUri;
    GetUriStringInUpdate(mediaType, api, mediaTypeUri, values);

    tableName = MEDIALIBRARY_TABLE;
    if (api == MediaLibraryApi::API_10) {
        SetValuesFromMetaDataApi10(metadata, values, false, skipPhoto);
        GetTableNameByPath(mediaType, tableName);
    } else {
#ifdef MEDIALIBRARY_COMPATIBILITY
        GetTableNameByPath(mediaType, tableName, metadata.GetFilePath());
#endif
        SetValuesFromMetaDataApi9(metadata, values, false, tableName);
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw();
    if (rdbStore == nullptr) {
        return "";
    }
    auto rdbStorePtr = rdbStore->GetRaw();
    if (rdbStorePtr == nullptr) {
        return "";
    }
    int32_t result = rdbStorePtr->Update(updateCount, tableName, values, whereClause, whereArgs);
    if (result != NativeRdb::E_OK || updateCount <= 0) {
        MEDIA_ERR_LOG("Update operation failed. Result %{public}d. Updated %{public}d", result, updateCount);
        if (result != NativeRdb::E_OK) {
            VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, result},
                {KEY_OPT_TYPE, OptType::SCAN}};
            PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        }
        if (updateCount <= 0) {
            VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, updateCount},
                {KEY_OPT_TYPE, OptType::SCAN}};
            PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        }
        return "";
    }
    if (mediaTypeUri.empty()) {
        return "";
    }
    if (api == MediaLibraryApi::API_10) {
        return MakeFileUri(mediaTypeUri, metadata);
    }
    return MediaFileUtils::GetUriByExtrConditions(mediaTypeUri + "/", to_string(metadata.GetFileId()));
}

/**
 * @brief Deletes particular entry in database based on row id
 *
 * @param idList The list of IDs to be deleted from the media db
 * @return bool Status of the delete operation
 */
bool MediaScannerDb::DeleteMetadata(const vector<string> &idList, const string &tableName)
{
    if (idList.size() == 0) {
        MEDIA_ERR_LOG("to-deleted idList size equals to 0");
        return false;
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw();
    if (rdbStore == nullptr) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        MEDIA_ERR_LOG("rdbStore is nullptr");
        return E_ERR;
    }

    NativeRdb::RdbPredicates rdbPredicate(tableName);
    rdbPredicate.In(MEDIA_DATA_DB_ID, idList);
    int32_t ret = rdbStore->Delete(rdbPredicate);
    return ret == static_cast<int32_t>(idList.size());
}

static OperationObject GetOprnObjectFromPath(const string &path)
{
    const map<string, OperationObject> oprnMap = {
        { ROOT_MEDIA_DIR + PHOTO_BUCKET, OperationObject::FILESYSTEM_PHOTO },
        { ROOT_MEDIA_DIR + AUDIO_BUCKET, OperationObject::FILESYSTEM_AUDIO },
#ifdef MEDIALIBRARY_COMPATIBILITY
        { ROOT_MEDIA_DIR + PIC_DIR_VALUES, OperationObject::FILESYSTEM_PHOTO },
        { ROOT_MEDIA_DIR + AUDIO_DIR_VALUES, OperationObject::FILESYSTEM_AUDIO },
        { ROOT_MEDIA_DIR + VIDEO_DIR_VALUES, OperationObject::FILESYSTEM_PHOTO },
        { ROOT_MEDIA_DIR + CAMERA_DIR_VALUES, OperationObject::FILESYSTEM_PHOTO }
#endif
    };

    for (const auto &iter : oprnMap) {
        if (path.find(iter.first) != string::npos) {
            return iter.second;
        }
    }
    return OperationObject::FILESYSTEM_ASSET;
}

static void GetQueryParamsByPath(const string &path, MediaLibraryApi api, vector<string> &columns,
    OperationObject &oprnObject, string &whereClause)
{
    oprnObject = GetOprnObjectFromPath(path);
    if (api == MediaLibraryApi::API_10) {
        whereClause = MediaColumn::MEDIA_FILE_PATH + " = ?";
        if (oprnObject == OperationObject::FILESYSTEM_PHOTO) {
            columns = {
                MediaColumn::MEDIA_ID, MediaColumn::MEDIA_SIZE, MediaColumn::MEDIA_DATE_MODIFIED,
                MediaColumn::MEDIA_NAME, PhotoColumn::PHOTO_ORIENTATION, MediaColumn::MEDIA_TIME_PENDING,
                MediaColumn::MEDIA_DATE_ADDED, PhotoColumn::PHOTO_DATE_DAY, MediaColumn::MEDIA_OWNER_PACKAGE,
                PhotoColumn::PHOTO_SUBTYPE
            };
        } else if (oprnObject == OperationObject::FILESYSTEM_AUDIO) {
            columns = {
                MediaColumn::MEDIA_ID, MediaColumn::MEDIA_SIZE, MediaColumn::MEDIA_DATE_MODIFIED,
                MediaColumn::MEDIA_NAME, MediaColumn::MEDIA_TIME_PENDING
            };
        }
    } else {
        if (oprnObject == OperationObject::FILESYSTEM_PHOTO) {
            whereClause = MediaColumn::MEDIA_FILE_PATH + " = ?";
            columns = {
                MediaColumn::MEDIA_ID, MediaColumn::MEDIA_SIZE, MediaColumn::MEDIA_DATE_MODIFIED,
                MediaColumn::MEDIA_NAME, PhotoColumn::PHOTO_ORIENTATION, MediaColumn::MEDIA_TIME_PENDING,
                MediaColumn::MEDIA_DATE_ADDED, PhotoColumn::PHOTO_DATE_DAY, MediaColumn::MEDIA_OWNER_PACKAGE
            };
        } else if (oprnObject == OperationObject::FILESYSTEM_AUDIO) {
            whereClause = MediaColumn::MEDIA_FILE_PATH + " = ?";
            columns = {
                MediaColumn::MEDIA_ID, MediaColumn::MEDIA_SIZE, MediaColumn::MEDIA_DATE_MODIFIED,
                MediaColumn::MEDIA_NAME, MediaColumn::MEDIA_TIME_PENDING
            };
        } else {
            whereClause = MEDIA_DATA_DB_FILE_PATH + " = ? And " + MEDIA_DATA_DB_IS_TRASH + " = ? ";
            columns = {
                MEDIA_DATA_DB_ID, MEDIA_DATA_DB_SIZE, MEDIA_DATA_DB_DATE_MODIFIED,
                MEDIA_DATA_DB_NAME, MEDIA_DATA_DB_ORIENTATION, MEDIA_DATA_DB_RECYCLE_PATH
            };
        }
    }
}

int32_t MediaScannerDb::GetFileSet(MediaLibraryCommand &cmd, const vector<string> &columns,
    shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw();
    if (rdbStore == nullptr) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_RDB},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return E_RDB;
    }
    resultSet = rdbStore->Query(cmd, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("return nullptr when query rdb");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_RDB},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return E_RDB;
    }

    int32_t rowCount = 0;
    int32_t ret = resultSet->GetRowCount(rowCount);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("failed to get row count");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, ret},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return E_RDB;
    }

    if (rowCount == 0) {
        return E_OK;
    }

    ret = resultSet->GoToFirstRow();
    if (ret != NativeRdb::E_OK) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, ret},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        MEDIA_ERR_LOG("failed to go to first row");
        return E_RDB;
    }
    return E_OK;
}

/**
 * @brief Get date modified, id, size and name info for a file
 *
 * @param path The file path for which to obtain the latest modification info from the db
 * @return unique_ptr<Metadata> The metadata object representing the latest info for the given filepath
 */
int32_t MediaScannerDb::GetFileBasicInfo(const string &path, unique_ptr<Metadata> &ptr, MediaLibraryApi api)
{
    vector<string> columns;
    string whereClause;
    OperationObject oprnObject = OperationObject::FILESYSTEM_ASSET;
    GetQueryParamsByPath(path, api, columns, oprnObject, whereClause);

    vector<string> args;
    if (oprnObject == OperationObject::FILESYSTEM_PHOTO || oprnObject == OperationObject::FILESYSTEM_AUDIO) {
        args = { path };
    } else {
        args = { path, to_string(NOT_TRASHED) };
    }

    MediaLibraryCommand cmd(oprnObject, OperationType::QUERY, api);
    cmd.GetAbsRdbPredicates()->SetWhereClause(whereClause);
    cmd.GetAbsRdbPredicates()->SetWhereArgs(args);

    shared_ptr<NativeRdb::ResultSet> resultSet;
    int32_t ret = GetFileSet(cmd, columns, resultSet);
    if (ret != E_OK) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, ret},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return ret;
    }
    ptr->SetTableName(cmd.GetTableName());

    return FillMetadata(resultSet, ptr);
}

static void PreparePredicatesAndColumns(const string &path, const string &tableName, const string &whitePath,
    AbsRdbPredicates &predicates, vector<string> &columns)
{
    string querySql;
    vector<string> args;
    if (whitePath.empty()) {
        if (tableName == MEDIALIBRARY_TABLE) {
            querySql = MEDIA_DATA_DB_FILE_PATH + " LIKE ? AND " + MEDIA_DATA_DB_IS_TRASH + " = ? ";
            args = { path.back() != '/' ? path + "/%" : path + "%", to_string(NOT_TRASHED) };
            columns = { MEDIA_DATA_DB_ID, MEDIA_DATA_DB_MEDIA_TYPE, MEDIA_DATA_DB_RECYCLE_PATH };
        } else {
            querySql = MEDIA_DATA_DB_FILE_PATH + " LIKE ? AND " + MediaColumn::MEDIA_TIME_PENDING + " <> ? ";
            args= { path.back() != '/' ? path + "/%" : path + "%", to_string(UNCREATE_FILE_TIMEPENDING) };
            columns = { MEDIA_DATA_DB_ID, MEDIA_DATA_DB_MEDIA_TYPE };
        }
    } else {
        if (tableName == MEDIALIBRARY_TABLE) {
            querySql = MEDIA_DATA_DB_FILE_PATH + " LIKE ? AND " + MEDIA_DATA_DB_FILE_PATH + " NOT LIKE ? AND " +
                MEDIA_DATA_DB_IS_TRASH + " = ? ";
            args = { path.back() != '/' ? path + "/%" : path + "%",
            whitePath.back() != '/' ? whitePath + "/%" : whitePath + "%", to_string(NOT_TRASHED) };
            columns = { MEDIA_DATA_DB_ID, MEDIA_DATA_DB_MEDIA_TYPE, MEDIA_DATA_DB_RECYCLE_PATH };
        } else {
            querySql = MEDIA_DATA_DB_FILE_PATH + " LIKE ? AND " + MEDIA_DATA_DB_FILE_PATH + " NOT LIKE ? AND " +
                MediaColumn::MEDIA_TIME_PENDING + " <> ? ";
            args= { path.back() != '/' ? path + "/%" : path + "%",
                whitePath.back() != '/' ? whitePath + "/%" : whitePath + "%", to_string(UNCREATE_FILE_TIMEPENDING) };
            columns = { MEDIA_DATA_DB_ID, MEDIA_DATA_DB_MEDIA_TYPE };
        }
    }

    predicates.SetWhereClause(querySql);
    predicates.SetWhereArgs(args);
}

/**
 * @brief Get the list of all IDs corresponding to given path
 *
 * @param path The path from which to obtain the list of child IDs
 * @return unordered_map<int32_t, MediaType> The list of IDS along with mediaType information
 */
unordered_map<int32_t, MediaType> MediaScannerDb::GetIdsFromFilePath(const string &path, const string &tableName,
    const string &whitePath)
{
    unordered_map<int32_t, MediaType> idMap = {};
    AbsRdbPredicates predicates(tableName);
    vector<string> columns;
    PreparePredicatesAndColumns(path, tableName, whitePath, predicates, columns);

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw();
    if (rdbStore == nullptr) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return idMap;
    }
    auto rdbStorePtr = rdbStore->GetRaw();
    if (rdbStorePtr == nullptr) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return idMap;
    }
    auto resultSet = rdbStorePtr->Query(predicates, columns);
    if (resultSet == nullptr) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return idMap;
    }

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        if (tableName == MEDIALIBRARY_TABLE) {
            string recyclePath = GetStringVal(MEDIA_DATA_DB_RECYCLE_PATH, resultSet);
            if (!recyclePath.empty()) {
                continue;
            }
        }
        int32_t id = GetInt32Val(MEDIA_DATA_DB_ID, resultSet);
        int32_t mediaType = GetInt32Val(MEDIA_DATA_DB_MEDIA_TYPE, resultSet);
        idMap.emplace(make_pair(id, static_cast<MediaType>(mediaType)));
    }
    return idMap;
}

string MediaScannerDb::GetFileDBUriFromPath(const string &path)
{
    string uri;

    vector<string> columns = {};
    columns.push_back(MEDIA_DATA_DB_URI);

    DataShare::DataSharePredicates predicates;
    predicates.SetWhereClause(MEDIA_DATA_DB_FILE_PATH + " = ? AND " + MEDIA_DATA_DB_IS_TRASH + " = ?");
    vector<string> args = { path, to_string(NOT_TRASHED) };
    predicates.SetWhereArgs(args);

    Uri queryUri(MEDIALIBRARY_DATA_URI);
    MediaLibraryCommand cmd(queryUri, OperationType::QUERY);
    int errCode = 0;
    auto resultSet = MediaLibraryDataManager::GetInstance()->QueryRdb(cmd, columns, predicates, errCode);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("No result found for this path");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return uri;
    }
    auto ret = resultSet->GoToFirstRow();
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Get data error for this path");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, ret},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return uri;
    }

    int32_t intValue(0);
    int32_t columnIndex(0);
    resultSet->GetColumnIndex(MEDIA_DATA_DB_ID, columnIndex);
    resultSet->GetInt(columnIndex, intValue);
    uri = MEDIALIBRARY_DATA_URI + "/" + to_string(intValue);
    return uri;
}

int32_t MediaScannerDb::GetIdFromPath(const string &path)
{
    int32_t id = UNKNOWN_ID;
    int32_t columnIndex = -1;

    DataShare::DataSharePredicates predicates;
    predicates.SetWhereClause(MEDIA_DATA_DB_FILE_PATH + " = ? AND " + MEDIA_DATA_DB_IS_TRASH + " = ?");
    vector<string> args = { path, to_string(NOT_TRASHED) };
    predicates.SetWhereArgs(args);

    Uri uri(MEDIALIBRARY_DATA_URI);
    MediaLibraryCommand cmd(uri, OperationType::QUERY);
    vector<string> columns = {MEDIA_DATA_DB_ID};
    int errCode = 0;
    auto resultSet = MediaLibraryDataManager::GetInstance()->QueryRdb(cmd, columns, predicates, errCode);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("No data found for the given path %{private}s", path.c_str());
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return id;
    }
    auto ret = resultSet->GoToFirstRow();
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Get data for the given path %{private}s error", path.c_str());
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, ret},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return id;
    }

    resultSet->GetColumnIndex(MEDIA_DATA_DB_ID, columnIndex);
    resultSet->GetInt(columnIndex, id);

    return id;
}

int32_t MediaScannerDb::ReadAlbums(const string &path, unordered_map<string, Metadata> &albumMap)
{
    if ((path + "/").find(ROOT_MEDIA_DIR) != 0) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_INVALID_ARGUMENTS},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return E_INVALID_ARGUMENTS;
    }

    AbsRdbPredicates predicates(MEDIALIBRARY_TABLE);
    string queryCmd = MEDIA_DATA_DB_MEDIA_TYPE + " = ? AND " + MEDIA_DATA_DB_FILE_PATH + " like ? AND " +
        MEDIA_DATA_DB_IS_TRASH + " = ?";
    string queryPath = path.back() != '/' ? path + "/%" : path + "%";
    vector<string> args = { to_string(MediaType::MEDIA_TYPE_ALBUM), queryPath, to_string(NOT_TRASHED) };
    predicates.SetWhereClause(queryCmd);
    predicates.SetWhereArgs(args);
    vector<string> columns = {MEDIA_DATA_DB_ID, MEDIA_DATA_DB_FILE_PATH, MEDIA_DATA_DB_DATE_MODIFIED};

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw();
    if (rdbStore == nullptr) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return E_HAS_DB_ERROR;
    }
    auto rdbStorePtr = rdbStore->GetRaw();
    if (rdbStorePtr == nullptr) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return E_HAS_DB_ERROR;
    }
    auto resultSet = rdbStorePtr->Query(predicates, columns);
    if (resultSet == nullptr) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return E_HAS_DB_ERROR;
    }

    albumMap.clear();
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        Metadata metadata;
        int32_t intValue = GetInt32Val(MEDIA_DATA_DB_ID, resultSet);
        metadata.SetFileId(intValue);
        string strValue = GetStringVal(MEDIA_DATA_DB_FILE_PATH, resultSet);
        metadata.SetFilePath(strValue);
        int64_t dateModified = GetInt64Val(MEDIA_DATA_DB_DATE_MODIFIED, resultSet);
        metadata.SetFileDateModified(dateModified);
        albumMap.insert(make_pair(strValue, metadata));
    }

    return E_OK;
}

int32_t MediaScannerDb::InsertAlbum(const Metadata &metadata)
{
    int32_t id = 0;

    string tableName;
    string uri = InsertMetadata(metadata, tableName);
    id = stoi(MediaFileUtils::GetIdFromUri(uri));

    return id;
}

int32_t MediaScannerDb::UpdateAlbum(const Metadata &metadata)
{
    int32_t id = 0;

    string tableName;
    string uri = UpdateMetadata(metadata, tableName);
    id = stoi(MediaFileUtils::GetIdFromUri(uri));

    return id;
}

void MediaScannerDb::NotifyDatabaseChange(const MediaType mediaType)
{
    string notifyUri = MediaFileUtils::GetMediaTypeUri(mediaType);
    Uri uri(notifyUri);

    MediaLibraryDataManager::GetInstance()->NotifyChange(uri);
}

void MediaScannerDb::ExtractMetaFromColumn(const shared_ptr<NativeRdb::ResultSet> &resultSet,
                                           unique_ptr<Metadata> &metadata, const std::string &col)
{
    ResultSetDataType dataType = ResultSetDataType::TYPE_NULL;
    Metadata::MetadataFnPtr requestFunc = nullptr;
    auto itr = metadata->memberFuncMap_.find(col);
    if (itr != metadata->memberFuncMap_.end()) {
        dataType = itr->second.first;
        requestFunc = itr->second.second;
    } else {
        MEDIA_ERR_LOG("invalid column name %{private}s", col.c_str());
        return;
    }

    std::variant<int32_t, std::string, int64_t, double> data =
        ResultSetUtils::GetValFromColumn<const shared_ptr<NativeRdb::ResultSet>>(col, resultSet, dataType);

    // Use the function pointer from map and pass data to fn ptr
    if (requestFunc != nullptr) {
        (metadata.get()->*requestFunc)(data);
    }
}

int32_t MediaScannerDb::FillMetadata(const shared_ptr<NativeRdb::ResultSet> &resultSet,
    unique_ptr<Metadata> &ptr)
{
    std::vector<std::string> columnNames;
    int32_t err = resultSet->GetAllColumnNames(columnNames);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("failed to get all column names");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, err},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return E_RDB;
    }

    for (const auto &col : columnNames) {
        ExtractMetaFromColumn(resultSet, ptr, col);
    }

    return E_OK;
}

int32_t MediaScannerDb::RecordError(const std::string &err)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw();
    if (rdbStore == nullptr) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        MEDIA_ERR_LOG("rdbStore is nullptr");
        return E_ERR;
    }

    ValuesBucket valuesBucket;
    valuesBucket.PutString(MEDIA_DATA_ERROR, err);
    int64_t outRowId = -1;
    auto rdbStorePtr = rdbStore->GetRaw();
    if (rdbStorePtr == nullptr) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        MEDIA_ERR_LOG("rdbStorePtr is nullptr");
        return E_ERR;
    }
    int32_t ret = rdbStorePtr->Insert(outRowId, MEDIALIBRARY_ERROR_TABLE, valuesBucket);
    if (ret) {
        MEDIA_ERR_LOG("rdb insert err %{public}d", ret);
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, ret},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return E_ERR;
    }

    return E_OK;
}

std::set<std::string> MediaScannerDb::ReadError()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return {};
    }

    AbsRdbPredicates predicates(MEDIALIBRARY_ERROR_TABLE);
    vector<string> columns = { MEDIA_DATA_ERROR };
    auto rdbStorePtr = rdbStore->GetRaw();
    if (rdbStorePtr == nullptr) {
        MEDIA_ERR_LOG("rdbStorePtr is nullptr");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return {};
    }
    auto resultSet = rdbStorePtr->Query(predicates, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("rdb query return nullptr");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return {};
    }

    int32_t rowCount = 0;
    auto ret = resultSet->GetRowCount(rowCount);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("failed to get row count");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, ret},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return {};
    }

    if (rowCount == 0) {
        return {};
    }

    string str;
    set<string> errSet;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        resultSet->GetString(0, str);
        errSet.insert(move(str));
    }

    return errSet;
}

int32_t MediaScannerDb::DeleteError(const std::string &err)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return E_ERR;
    }

    int32_t outRowId = -1;
    string whereClause = MEDIA_DATA_ERROR + " = ?";
    vector<string> whereArgs= { err };
    auto rdbStorePtr = rdbStore->GetRaw();
    if (rdbStorePtr == nullptr) {
        MEDIA_ERR_LOG("rdbStorePtr is nullptr");
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_HAS_DB_ERROR},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return E_ERR;
    }
    int32_t ret = rdbStorePtr->Delete(outRowId, MEDIALIBRARY_ERROR_TABLE, whereClause, whereArgs);
    if (ret) {
        MEDIA_ERR_LOG("rdb delete err %{public}d", ret);
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, ret},
            {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return E_ERR;
    }

    return E_OK;
}

void MediaScannerDb::UpdateAlbumInfo(const std::vector<std::string> &subtypes,
    const std::vector<std::string> &userAlbumIds, const std::vector<std::string> &sourceAlbumIds)
{
    MediaLibraryRdbUtils::UpdateSystemAlbumInternal(
        MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw()->GetRaw(), {
        to_string(PhotoAlbumSubType::IMAGE),
        to_string(PhotoAlbumSubType::VIDEO),
        to_string(PhotoAlbumSubType::SCREENSHOT),
        to_string(PhotoAlbumSubType::FAVORITE),
    });
    std::unordered_map<int32_t, int32_t> updateResult;
    MediaLibraryRdbUtils::UpdateSourceAlbumInternal(
        MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw()->GetRaw(), updateResult, sourceAlbumIds);
}

void MediaScannerDb::UpdateAlbumInfoByMetaData(const Metadata &metadata)
{
    if (metadata.GetFileMediaType() == MEDIA_TYPE_IMAGE) {
        MediaLibraryRdbUtils::UpdateSystemAlbumInternal(
            MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw()->GetRaw(),
            { to_string(PhotoAlbumSubType::IMAGE) }
        );
    } else if (metadata.GetFileMediaType() == MEDIA_TYPE_VIDEO) {
        MediaLibraryRdbUtils::UpdateSystemAlbumInternal(
            MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw()->GetRaw(),
            { to_string(PhotoAlbumSubType::VIDEO) }
        );
    }
    if (!metadata.GetOwnerPackage().empty()) {
        if (metadata.GetFileId() != FILE_ID_DEFAULT) {
            std::string uri = PhotoColumn::PHOTO_URI_PREFIX + to_string(metadata.GetFileId());
            std::unordered_map<int32_t, int32_t> updateResult;
            MediaLibraryRdbUtils::UpdateSourceAlbumByUri(
                MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw()->GetRaw(),
                updateResult, {uri});
        }
    }
}

std::string MediaScannerDb::MakeFileUri(const std::string &mediaTypeUri, const Metadata &metadata)
{
    return MediaFileUtils::GetUriByExtrConditions(mediaTypeUri + "/", to_string(metadata.GetFileId()),
        MediaFileUtils::GetExtraUri(metadata.GetFileName(), metadata.GetFilePath())) + "?api_version=10" +
        "&date_added=" + to_string(metadata.GetFileDateAdded());
}
} // namespace Media
} // namespace OHOS

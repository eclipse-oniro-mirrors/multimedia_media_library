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
#define MLOG_TAG "FileOperation"

#include "medialibrary_file_operations.h"

#include "datashare_predicates.h"
#include "datashare_values_bucket.h"
#include "file_asset.h"
#include "hitrace_meter.h"
#ifdef MEDIALIBRARY_COMPATIBILITY
#include "media_file_asset_columns.h"
#endif
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_notify.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_smartalbum_map_operations.h"
#include "medialibrary_tracer.h"
#include "medialibrary_unistore_manager.h"
#include "native_album_asset.h"
#include "rdb_utils.h"
#include "userfilemgr_uri.h"
#include "media_audio_column.h"

using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using namespace OHOS::RdbDataShareAdapter;

namespace OHOS {
namespace Media {
using ChangeType = AAFwk::ChangeInfo::ChangeType;

int32_t MediaLibraryFileOperations::HandleFileOperation(MediaLibraryCommand &cmd)
{
    int32_t errCode = E_FAIL;
    auto values = cmd.GetValueBucket();
    string actualUri;

    ValueObject valueObject;
    if (values.GetObject(MEDIA_DATA_DB_URI, valueObject)) {
        valueObject.GetString(actualUri);
    }

    // only support CloseAsset when networkId is not empty
    string networkId = MediaFileUtils::GetNetworkIdFromUri(actualUri);
    if (!networkId.empty() && cmd.GetOprnType() != OperationType::CLOSE) {
        return E_PERMISSION_DENIED;
    }

    switch (cmd.GetOprnType()) {
        case OperationType::CREATE:
            errCode = CreateFileOperation(cmd);
            break;
        case OperationType::CLOSE:
            errCode = CloseFileOperation(cmd);
            break;
        case OperationType::GETCAPACITY:
            errCode = GetAlbumCapacityOperation(cmd);
            break;
        case OperationType::COPY:
            errCode = CopyFileOperation(cmd);
            break;
        default:
            MEDIA_ERR_LOG("unknown operation type %{public}d", cmd.GetOprnType());
            break;
    }
    return errCode;
}

int32_t MediaLibraryFileOperations::CreateFileOperation(MediaLibraryCommand &cmd)
{
    return MediaLibraryObjectUtils::CreateFileObj(cmd);
}

int32_t MediaLibraryFileOperations::CloseFileOperation(MediaLibraryCommand &cmd)
{
    return MediaLibraryObjectUtils::CloseFile(cmd);
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryFileOperations::QueryFavFiles(MediaLibraryCommand &cmd)
{
    cmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_IS_FAV, "1");
    cmd.GetAbsRdbPredicates()->And()->NotEqualTo(MEDIA_DATA_DB_MEDIA_TYPE, std::to_string(MEDIA_TYPE_ALBUM));

    return MediaLibraryObjectUtils::QueryWithCondition(cmd, {});
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryFileOperations::QueryTrashFiles(MediaLibraryCommand &cmd)
{
    cmd.GetAbsRdbPredicates()
        ->GreaterThan(MEDIA_DATA_DB_DATE_TRASHED, std::to_string(NOT_TRASHED))
        ->And()
        ->NotEqualTo(MEDIA_DATA_DB_MEDIA_TYPE, std::to_string(MEDIA_TYPE_ALBUM));

    return MediaLibraryObjectUtils::QueryWithCondition(cmd, {});
}

int32_t MediaLibraryFileOperations::GetAlbumCapacityOperation(MediaLibraryCommand &cmd)
{
    int32_t errorCode = E_FAIL;
    shared_ptr<NativeRdb::ResultSet> resultSet = nullptr;

    auto values = cmd.GetValueBucket();
    ValueObject valueObject;
    int32_t isFavourite = 0;
    bool isTrash = false;
    if (values.GetObject(MEDIA_DATA_DB_IS_FAV, valueObject)) {
        valueObject.GetInt(isFavourite);
    }
    if (values.GetObject(MEDIA_DATA_DB_IS_TRASH, valueObject)) {
        valueObject.GetBool(isTrash);
    }

    if (isFavourite != 0) {
        resultSet = QueryFavFiles(cmd);
    } else if (isTrash) {
        resultSet = QueryTrashFiles(cmd);
    }

    if (resultSet != nullptr) {
        resultSet->GetRowCount(errorCode);
        MEDIA_INFO_LOG("GetRowCount %{private}d", errorCode);
    }

    return errorCode;
}

int32_t MediaLibraryFileOperations::ModifyFileOperation(MediaLibraryCommand &cmd)
{
    string strFileId = cmd.GetOprnFileId();
    if (strFileId.empty()) {
        MEDIA_ERR_LOG("MediaLibraryFileOperations::ModifyFileOperation Get id from uri or valuesBucket failed!");
        return E_INVALID_FILEID;
    }

    string srcPath = MediaLibraryObjectUtils::GetPathByIdFromDb(strFileId);
    if (srcPath.empty()) {
        MEDIA_ERR_LOG("MediaLibraryFileOperations::ModifyFileOperation Get path of id %{private}s from database file!",
            strFileId.c_str());
        return E_INVALID_FILEID;
    }

    string dstFileName;
    string dstReFilePath;
    auto values = cmd.GetValueBucket();
    ValueObject valueObject;
    if (values.GetObject(MEDIA_DATA_DB_NAME, valueObject)) {
        valueObject.GetString(dstFileName);
    }
    if (values.GetObject(MEDIA_DATA_DB_RELATIVE_PATH, valueObject)) {
        valueObject.GetString(dstReFilePath);
    }
    string dstFilePath = ROOT_MEDIA_DIR + dstReFilePath + dstFileName;

    if (srcPath.compare(dstFilePath) == 0) {
        return E_SAME_PATH;
    }
    return MediaLibraryObjectUtils::RenameFileObj(cmd, srcPath, dstFilePath);
}

constexpr bool START_PENDING = false;
static void SolvePendingInQuery(AbsRdbPredicates* predicates)
{
    string whereClause = predicates->GetWhereClause();
    size_t groupByPoint = whereClause.rfind("GROUP BY");
    string groupBy;
    if (groupByPoint != string::npos) {
        groupBy = whereClause.substr(groupByPoint);
        whereClause = whereClause.substr(0, groupByPoint);
    }

    predicates->SetWhereClause(whereClause);
    predicates->EqualTo(MEDIA_DATA_DB_TIME_PENDING, "0");
    if (!groupBy.empty()) {
        predicates->SetWhereClause(predicates->GetWhereClause() + groupBy);
    }
}

#ifdef MEDIALIBRARY_COMPATIBILITY
const std::string EMPTY_COLUMN_AS = "'' AS ";
const std::string DEFAULT_INT_COLUMN_AS = "0 AS ";
const std::string COMPAT_COLUMN_ARTIST = EMPTY_COLUMN_AS + MEDIA_DATA_DB_ARTIST;
const std::string COMPAT_COLUMN_AUDIO_ALBUM = EMPTY_COLUMN_AS + MEDIA_DATA_DB_AUDIO_ALBUM;
const std::string COMPAT_COLUMN_ORIENTATION = DEFAULT_INT_COLUMN_AS + MEDIA_DATA_DB_ORIENTATION;
const std::string COMPAT_COLUMN_BUCKET_ID = DEFAULT_INT_COLUMN_AS + MEDIA_DATA_DB_BUCKET_ID;
const std::string COMPAT_COLUMN_BUCKET_NAME = EMPTY_COLUMN_AS + MEDIA_DATA_DB_BUCKET_NAME;
const std::string COMPAT_COLUMN_IS_TRASH = MEDIA_DATA_DB_DATE_TRASHED + " AS " + MEDIA_DATA_DB_IS_TRASH;
const std::string COMPAT_COLUMN_WIDTH = DEFAULT_INT_COLUMN_AS + MEDIA_DATA_DB_WIDTH;
const std::string COMPAT_COLUMN_HEIGHT = DEFAULT_INT_COLUMN_AS + MEDIA_DATA_DB_HEIGHT;
const std::string COMPAT_COLUMN_URI = EMPTY_COLUMN_AS + MEDIA_DATA_DB_URI;

static const vector<string> &PhotosCompatColumns()
{
    /*
     * Caution: Columns MUST KEEP SAME ORDER for sqlite UNION operation in:
     *     o PHOTOS_COMPAT_COLUMNS
     *     o AUDIOS_COMPAT_COLUMNS
     *     o FILES_COMPAT_COLUMNS
     */
    static const vector<string> PHOTOS_COMPAT_COLUMNS = {
        MEDIA_DATA_DB_ID,
        MEDIA_DATA_DB_FILE_PATH,
        COMPAT_COLUMN_URI,
        MEDIA_DATA_DB_MIME_TYPE,
        MEDIA_DATA_DB_MEDIA_TYPE,
        MEDIA_DATA_DB_NAME,
        MEDIA_DATA_DB_TITLE,
        MEDIA_DATA_DB_RELATIVE_PATH,
        MEDIA_DATA_DB_PARENT_ID,
        MEDIA_DATA_DB_SIZE,
        MEDIA_DATA_DB_DATE_ADDED,
        MEDIA_DATA_DB_DATE_MODIFIED,
        MEDIA_DATA_DB_DATE_ADDED_TO_SECOND,
        MEDIA_DATA_DB_DATE_MODIFIED_TO_SECOND,
        MEDIA_DATA_DB_DATE_TAKEN,
        MEDIA_DATA_DB_DATE_TAKEN_TO_SECOND,
        COMPAT_COLUMN_ARTIST,
        COMPAT_COLUMN_AUDIO_ALBUM,
        MEDIA_DATA_DB_WIDTH,
        MEDIA_DATA_DB_HEIGHT,
        MEDIA_DATA_DB_ORIENTATION,
        MEDIA_DATA_DB_DURATION,
        COMPAT_COLUMN_BUCKET_ID,
        COMPAT_COLUMN_BUCKET_NAME,
        COMPAT_COLUMN_IS_TRASH,
        MEDIA_DATA_DB_IS_FAV,
        MEDIA_DATA_DB_DATE_TRASHED,
        MEDIA_DATA_DB_DATE_TRASHED_TO_SECOND,

        MediaColumn::MEDIA_HIDDEN,
        PhotoColumn::PHOTO_SYNC_STATUS,
        PhotoColumn::PHOTO_SUBTYPE,
    };
    return PHOTOS_COMPAT_COLUMNS;
}

static const vector<string> &AudiosCompatColumns()
{
    /*
     * Caution: Columns MUST KEEP SAME ORDER for sqlite UNION operation in:
     *     o PHOTOS_COMPAT_COLUMNS
     *     o AUDIOS_COMPAT_COLUMNS
     *     o FILES_COMPAT_COLUMNS
     */
    static const vector<string> AUDIOS_COMPAT_COLUMNS = {
        MEDIA_DATA_DB_ID,
        MEDIA_DATA_DB_FILE_PATH,
        COMPAT_COLUMN_URI,
        MEDIA_DATA_DB_MIME_TYPE,
        MEDIA_DATA_DB_MEDIA_TYPE,
        MEDIA_DATA_DB_NAME,
        MEDIA_DATA_DB_TITLE,
        MEDIA_DATA_DB_RELATIVE_PATH,
        MEDIA_DATA_DB_PARENT_ID,
        MEDIA_DATA_DB_SIZE,
        MEDIA_DATA_DB_DATE_ADDED,
        MEDIA_DATA_DB_DATE_MODIFIED,
        MEDIA_DATA_DB_DATE_ADDED_TO_SECOND,
        MEDIA_DATA_DB_DATE_MODIFIED_TO_SECOND,
        MEDIA_DATA_DB_DATE_TAKEN,
        MEDIA_DATA_DB_DATE_TAKEN_TO_SECOND,
        MEDIA_DATA_DB_ARTIST,
        MEDIA_DATA_DB_AUDIO_ALBUM,
        COMPAT_COLUMN_WIDTH,
        COMPAT_COLUMN_HEIGHT,
        COMPAT_COLUMN_ORIENTATION,
        MEDIA_DATA_DB_DURATION,
        COMPAT_COLUMN_BUCKET_ID,
        COMPAT_COLUMN_BUCKET_NAME,
        COMPAT_COLUMN_IS_TRASH,
        MEDIA_DATA_DB_IS_FAV,
        MEDIA_DATA_DB_DATE_TRASHED,
        MEDIA_DATA_DB_DATE_TRASHED_TO_SECOND,

        DEFAULT_INT_COLUMN_AS + MediaColumn::MEDIA_HIDDEN,
        DEFAULT_INT_COLUMN_AS + PhotoColumn::PHOTO_SYNC_STATUS,
        DEFAULT_INT_COLUMN_AS + PhotoColumn::PHOTO_SUBTYPE,
    };
    return AUDIOS_COMPAT_COLUMNS;
}

static const vector<string> &FilesCompatColumns()
{
    /*
     * Caution: KEEP SAME ORDER for sqlite UNION operation in columns below:
     *     o PHOTOS_COMPAT_COLUMNS
     *     o AUDIOS_COMPAT_COLUMNS
     *     o FILES_COMPAT_COLUMNS
     */
    static const vector<string> FILES_COMPAT_COLUMNS = {
        MEDIA_DATA_DB_ID,
        MEDIA_DATA_DB_FILE_PATH,
        MEDIA_DATA_DB_URI,
        MEDIA_DATA_DB_MIME_TYPE,
        MEDIA_DATA_DB_MEDIA_TYPE,
        MEDIA_DATA_DB_NAME,
        MEDIA_DATA_DB_TITLE,
        MEDIA_DATA_DB_RELATIVE_PATH,
        MEDIA_DATA_DB_PARENT_ID,
        MEDIA_DATA_DB_SIZE,
        MEDIA_DATA_DB_DATE_ADDED,
        MEDIA_DATA_DB_DATE_MODIFIED,
        MEDIA_DATA_DB_DATE_TAKEN,
        MEDIA_DATA_DB_DATE_ADDED_TO_SECOND,
        MEDIA_DATA_DB_DATE_MODIFIED_TO_SECOND,
        MEDIA_DATA_DB_DATE_TAKEN_TO_SECOND,
        MEDIA_DATA_DB_ARTIST,
        COMPAT_COLUMN_AUDIO_ALBUM,
        MEDIA_DATA_DB_WIDTH,
        MEDIA_DATA_DB_HEIGHT,
        MEDIA_DATA_DB_ORIENTATION,
        MEDIA_DATA_DB_DURATION,
        MEDIA_DATA_DB_BUCKET_ID,
        MEDIA_DATA_DB_BUCKET_NAME,
        MEDIA_DATA_DB_IS_TRASH,
        MEDIA_DATA_DB_IS_FAV,
        MEDIA_DATA_DB_DATE_TRASHED,
        MEDIA_DATA_DB_DATE_TRASHED_TO_SECOND,

        DEFAULT_INT_COLUMN_AS + MediaColumn::MEDIA_HIDDEN,
        PhotoColumn::PHOTO_SYNC_STATUS,
        DEFAULT_INT_COLUMN_AS + PhotoColumn::PHOTO_SUBTYPE,
    };

    return FILES_COMPAT_COLUMNS;
}

static void BuildQueryColumns(const vector<string> &columns, string &sql)
{
    for (const auto &col : columns) {
        sql += col + ',';
    }
    sql.pop_back();         // Remove last ','
}

static void ReplaceAlbumName(const string &arg, string &argInstead)
{
    if (arg == CAMERA_ALBUM_NAME) {
        argInstead = to_string(static_cast<int32_t>(PhotoSubType::CAMERA));
    } else if (arg == SCREEN_SHOT_ALBUM_NAME || arg == SCREEN_RECORD_ALBUM_NAME) {
        argInstead = to_string(static_cast<int32_t>(PhotoSubType::SCREENSHOT));
    } else {
        argInstead = arg;
    }
}

static void ReplaceId(const string &fileId, string &idInstead, const string &tableName)
{
    if (!all_of(fileId.begin(), fileId.end(), ::isdigit)) {
        return;
    }
    int32_t id;
    if (!StrToInt(fileId, id)) {
        MEDIA_ERR_LOG("invalid fileuri %{private}s", fileId.c_str());
        return;
    }
    idInstead = to_string(MediaFileUtils::GetRealIdByTable(id, tableName));
}

static void ReplaceSelectionAndArgsInQuery(string &selection, vector<string> &selectionArgs,
    const string &tableName, const string &key, const string &keyInstead = "")
{
    if (selection.empty()) {
        return;
    }

    for (size_t pos = 0; pos != string::npos;) {
        pos = selection.find(key, pos);
        if (pos == string::npos) {
            break;
        }
        if (!keyInstead.empty()) {
            selection.replace(pos, key.length(), keyInstead);
        }
        size_t argPos = selection.find('?', pos);
        if (argPos == string::npos) {
            break;
        }
        size_t argIndex = 0;
        for (size_t i = 0; i < argPos; i++) {
            if (selection[i] == '?') {
                argIndex++;
            }
        }
        if (argIndex > selectionArgs.size() - 1) {
            MEDIA_INFO_LOG("SelectionArgs size is not valid, selection format maybe incorrect: %{private}s",
                selection.c_str());
            break;
        }
        const string &arg = selectionArgs[argIndex];
        string argInstead = arg;
        if (key == MEDIA_DATA_DB_BUCKET_NAME) {
            ReplaceAlbumName(arg, argInstead);
        } else if (key == MEDIA_DATA_DB_ID) {
            ReplaceId(arg, argInstead, tableName);
        }
        selectionArgs[argIndex] = argInstead;
        pos = argPos + 1;
    }
}

static void BuildCompatQuerySql(MediaLibraryCommand &cmd, const string table, const vector<string> &columns,
    vector<string> &selectionArgs, string &sql)
{
    sql += "SELECT ";
    BuildQueryColumns(columns, sql);
    sql += " FROM " + table;

    string whereClause = cmd.GetAbsRdbPredicates()->GetWhereClause();
    vector<string> whereArgs = cmd.GetAbsRdbPredicates()->GetWhereArgs();
    if (table == PhotoColumn::PHOTOS_TABLE) {
        ReplaceSelectionAndArgsInQuery(whereClause, whereArgs, table, MEDIA_DATA_DB_BUCKET_NAME,
            PhotoColumn::PHOTO_SUBTYPE);
    }
    ReplaceSelectionAndArgsInQuery(whereClause, whereArgs, table, MEDIA_DATA_DB_ID);

    if (!whereClause.empty()) {
        sql += " WHERE " + whereClause;
    }

    if (!whereArgs.empty()) {
        selectionArgs.insert(selectionArgs.end(), whereArgs.begin(), whereArgs.end());
    }
}

static string RemoveWhereSuffix(MediaLibraryCommand &cmd, const string &key)
{
    string suffix;
    string whereClause = cmd.GetAbsRdbPredicates()->GetWhereClause();
    size_t keyPos = MediaFileUtils::FindIgnoreCase(whereClause, key);
    if (keyPos != string::npos) {
        suffix = whereClause.substr(keyPos);
        whereClause = whereClause.substr(0, keyPos);
    }
    cmd.GetAbsRdbPredicates()->SetWhereClause(whereClause);
    return suffix;
}

static void BuildQueryFileSql(MediaLibraryCommand &cmd, vector<string> &selectionArgs, string &sql)
{
    string groupBy = RemoveWhereSuffix(cmd, " GROUP BY ");
    string having = RemoveWhereSuffix(cmd, " HAVING ");
    string orderBy = RemoveWhereSuffix(cmd, " ORDER BY ");
    string limit = RemoveWhereSuffix(cmd, " LIMIT ");

    sql = "SELECT ";
    if (!groupBy.empty()) {
        sql += "count(*),";
    }
    BuildQueryColumns(FILE_ASSET_COLUMNS, sql);

    sql += " FROM (";
    BuildCompatQuerySql(cmd, PhotoColumn::PHOTOS_TABLE, PhotosCompatColumns(), selectionArgs, sql);
    sql += " UNION ";
    BuildCompatQuerySql(cmd, AudioColumn::AUDIOS_TABLE, AudiosCompatColumns(), selectionArgs, sql);
    sql += " UNION ";
    BuildCompatQuerySql(cmd, MEDIALIBRARY_TABLE, FilesCompatColumns(), selectionArgs, sql);
    sql += ") ";

    if (!groupBy.empty()) {
        sql += groupBy;
    }
    if (!having.empty()) {
        sql += having;
    }

    const string &order = cmd.GetAbsRdbPredicates()->GetOrder();
    if ((!order.empty()) && (!orderBy.empty())) {
        MEDIA_WARN_LOG("ORDER BY found both in whereClause and predicates, use the predicates one");
        sql += " ORDER BY " + order;
    } else if (!order.empty()) {
        sql += " ORDER BY " + order;
    } else if (!orderBy.empty()) {
        sql += orderBy;
    }

    if (!limit.empty()) {
        sql += limit;
    }
}

static void CampatQueryDebug(const string &sql, const vector<string> &selectionArgs,
    const shared_ptr<MediaLibraryRdbStore> store)
{
    constexpr int32_t printMax = 512;
    for (size_t pos = 0; pos < sql.size(); pos += printMax) {
        MEDIA_DEBUG_LOG("Quering file sql: %{private}s", sql.substr(pos, printMax).c_str());
    }
    for (const auto &arg : selectionArgs) {
        MEDIA_DEBUG_LOG("Quering file, arg: %{private}s", arg.c_str());
    }
    auto resultSet = store->QuerySql(sql, selectionArgs);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query file!");
        return;
    }
    int32_t count = -1;
    int32_t err = resultSet->GetRowCount(count);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to get count, err: %{public}d", err);
        return;
    }
    MEDIA_DEBUG_LOG("Quering file, count: %{public}d", count);
}
#endif

shared_ptr<NativeRdb::ResultSet> MediaLibraryFileOperations::QueryFileOperation(
    MediaLibraryCommand &cmd, const vector<string> &columns)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr");
        return nullptr;
    }

    string fileId = cmd.GetOprnFileId();
    if (cmd.GetAbsRdbPredicates()->GetWhereClause().empty() && !fileId.empty()) {
        cmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_ID, fileId);
    }

    if (START_PENDING) {
        SolvePendingInQuery(cmd.GetAbsRdbPredicates());
    }
    string networkId = cmd.GetOprnDevice();
    if (!networkId.empty()) {
        std::vector<string> devices;
        devices.push_back(networkId);
        cmd.GetAbsRdbPredicates()->InDevices(devices);
    }
    MediaLibraryTracer tracer;
    tracer.Start("QueryFile RdbStore->Query");

#ifdef MEDIALIBRARY_COMPATIBILITY
    string sql;
    vector<string> selectionArgs;
    BuildQueryFileSql(cmd, selectionArgs, sql);
    CampatQueryDebug(sql, selectionArgs, uniStore);
    return uniStore->QuerySql(sql, selectionArgs);
#else
    return uniStore->Query(cmd, columns);
#endif
}

int32_t MediaLibraryFileOperations::CopyFileOperation(MediaLibraryCommand &cmd)
{
    auto values = cmd.GetValueBucket();
    auto assetId = cmd.GetOprnFileId();
    ValueObject valueObject;
    string relativePath;
    if (values.GetObject(MEDIA_DATA_DB_RELATIVE_PATH, valueObject)) {
        valueObject.GetString(relativePath);
    }
    Uri srcUri(MEDIALIBRARY_DATA_URI + "/" + assetId);
    string srcUriString = srcUri.ToString();
    shared_ptr<FileAsset> srcFileAsset = MediaLibraryObjectUtils::GetFileAssetFromUri(srcUriString);
    if (srcFileAsset == nullptr) {
        return E_INVALID_URI;
    }
    if (srcFileAsset->GetMediaType() == MEDIA_TYPE_ALBUM) {
        return MediaLibraryObjectUtils::CopyDir(srcFileAsset, relativePath);
    } else {
        return MediaLibraryObjectUtils::CopyAsset(srcFileAsset, relativePath);
    }
}
} // namespace Media
} // namespace OHOS

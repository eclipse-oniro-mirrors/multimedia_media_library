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
#include <type_traits>
#define MLOG_TAG "AlbumOperation"

#include "medialibrary_album_operations.h"

#include "album_plugin_config.h"
#include "dfx_utils.h"
#include "media_analysis_helper.h"
#include "medialibrary_album_fusion_utils.h"
#include "medialibrary_analysis_album_operations.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_notify.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_tracer.h"
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
#include "enhancement_manager.h"
#endif
#include "multistages_capture_manager.h"

#include "result_set_utils.h"
#include "story_album_column.h"
#include "story_cover_info_column.h"
#include "medialibrary_formmap_operations.h"
#include "vision_face_tag_column.h"
#include "vision_image_face_column.h"
#include "vision_photo_map_column.h"
#include "vision_total_column.h"
#include "photo_owner_album_id_operation.h"
#include "photo_storage_operation.h"
#include "asset_accurate_refresh.h"
#include "refresh_business_name.h"
#include "parameters.h"
#include "media_album_order_back.h"

#include "lake_file_operations.h"
#include "photo_album_upload_status_operation.h"

using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using namespace OHOS::RdbDataShareAdapter;
using namespace OHOS::Media::AccurateRefresh;

namespace OHOS::Media {
using ChangeType = AAFwk::ChangeInfo::ChangeType;
constexpr int32_t THAN_AGR_SIZE = 1;
constexpr int32_t MERGE_ALBUM_COUNT = 2;
constexpr int32_t E_INDEX = -1;
constexpr int32_t PORTRAIT_FIRST_PAGE_MIN_COUNT = 50;
constexpr int32_t PORTRAIT_FIRST_PAGE_MIN_COUNT_RELATED_ME = 20;
constexpr int32_t PORTRAIT_SECOND_PAGE_MIN_PICTURES_COUNT = 10;
constexpr int32_t SUPPORT_QUERY_ISME_MIN_COUNT = 80;
constexpr int32_t PERCENTAGE_FOR_SUPPORT_QUERY_ISME = 100;
constexpr int32_t QUERY_PROB_IS_ME_VALUE = 1;
constexpr int32_t QUERY_IS_ME_VALUE = 2;
constexpr int32_t FACE_ANALYSISED_STATE = 3;
constexpr int32_t FACE_NO_NEED_ANALYSIS_STATE = -2;
constexpr int32_t ALBUM_NAME_NOT_NULL_ENABLED = 1;
constexpr int32_t ALBUM_PRIORITY_DEFAULT = 1;
constexpr int32_t ALBUM_SETNAME_OK = 1;
constexpr int32_t HIGHLIGHT_DELETED = -3;
constexpr int32_t HIGHLIGHT_COVER_STATUS_TITLE = 2;
constexpr int32_t HIGHLIGHT_COVER_STATUS_COVER = 1;
constexpr int32_t ALBUM_RENAMED = 2;
constexpr int32_t ALBUM_TO_RENAME_FOR_ANALYSIS = 3;
constexpr int32_t IS_ME_ALBUM = 1;
constexpr int32_t STORE_PORTRAIT_FIRST_PAGE_MIN_COUNT = 5;
constexpr int32_t STORE_PORTRAIT_SECOND_PAGE_MIN_COUNT = 1;
constexpr int32_t ANALYSIS_STATUS_TO_REFRESH = 1;
constexpr int32_t NO_FIRST_PAGE_SINGLE_PORTRAIT = -2;
constexpr int32_t DISPLAY_LEVEL_NUMBER = 1;
constexpr int32_t LOCAL_NUMBER = 1;
constexpr int32_t RENAME_OPERATION_NUMBER = 1;
constexpr int32_t NO_FIRST_PAGE_SINGLE_PORTRAIT_MANUAL = 2;
constexpr int32_t ANALYSISED_DISPLAY_LEVEL_HIDDEN = -1;
constexpr int32_t ANALYSISED_DISPLAY_LEVEL_SECOND_PAGE = -2;
const std::string USER_ALBUM_LPATH_PREFIX = "/Pictures/Users/";
const std::string SOURCE_ALBUM_LPATH_PREFIX = "/Pictures/";
const std::string SOURCE_PATH_PREFIX = "/storage/emulated/0";
const std::string ME_RELATIONSHIP = "me";
const std::string RETAIL_MODE_KEY = "const.dfx.enable_retail";
const std::string SQL_DELETE_ALL_ALBUM_ORDER_BACK =
    "DELETE FROM " + ALBUM_ORDER_BACK_TABLE;
const vector<string> NOT_CHANGEABLE_ALBUM = {
    AlbumPlugin::LPATH_SCREEN_RECORDS,
    AlbumPlugin::LPATH_SCREEN_SHOTS,
    AlbumPlugin::LPATH_HIDDEN_ALBUM,
    AlbumPlugin::LPATH_CAMERA
};
const std::string MINUS_5_SOMETHING = "-5_";

int32_t MediaLibraryAlbumOperations::CreateAlbumOperation(MediaLibraryCommand &cmd)
{
    int64_t outRow = -1;
    int32_t errCode = MediaLibraryObjectUtils::CreateDirObj(cmd, outRow);
    if (errCode == E_SUCCESS) {
        return outRow;
    }
    return errCode;
}

// only support modify in the same parent folder, like: a/b/c --> a/b/d
int32_t MediaLibraryAlbumOperations::ModifyAlbumOperation(MediaLibraryCommand &cmd)
{
    string strId = cmd.GetOprnFileId();
    string srcDirPath = MediaLibraryObjectUtils::GetPathByIdFromDb(strId);
    if (srcDirPath.empty()) {
        MEDIA_ERR_LOG("Get path of id %{private}s from database file!", strId.c_str());
        return E_INVALID_PATH;
    }

    auto values = cmd.GetValueBucket();
    string dstDirName;
    ValueObject valueObject;
    if (values.GetObject(MEDIA_DATA_DB_NAME, valueObject)) {
        valueObject.GetString(dstDirName);
    }
    int ret;
    if (dstDirName.empty() && !values.IsEmpty()) {
        ret = MediaLibraryObjectUtils::ModifyInfoByIdInDb(cmd);
    } else {
        string dstDirPath = MediaFileUtils::GetParentPath(srcDirPath) + "/" + dstDirName;
        ret = MediaLibraryObjectUtils::RenameDirObj(cmd, srcDirPath, dstDirPath);
    }
    return ret;
}

#ifdef MEDIALIBRARY_COMPATIBILITY
static void ReplaceRelativePath(string &selection, vector<string> &selectionArgs)
{
    for (size_t pos = 0; pos != string::npos;) {
        pos = selection.find(MEDIA_DATA_DB_RELATIVE_PATH, pos);
        if (pos == string::npos) {
            break;
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
            MEDIA_WARN_LOG("SelectionArgs size is not valid, selection format maybe incorrect: %{private}s",
                selection.c_str());
            break;
        }
        const string &arg = selectionArgs[argIndex];
        if (!arg.empty()) {
            MEDIA_WARN_LOG("No empty args in ReplaceRelativePath");
            return;
        }
        selection.replace(argPos, 1, "? OR 1=1)");
        selection.replace(pos, MEDIA_DATA_DB_RELATIVE_PATH.length(), "(" + PhotoAlbumColumns::ALBUM_ID);

        selectionArgs[argIndex] = "1";
        pos = argPos + 1;
    }
}

static void ReplaceMediaType(string &selection, vector<string> &selectionArgs)
{
    for (size_t pos = 0; pos != string::npos;) {
        pos = selection.find(MEDIA_DATA_DB_MEDIA_TYPE, pos);
        if (pos == string::npos) {
            break;
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
            MEDIA_WARN_LOG("SelectionArgs size is not valid, selection format maybe incorrect: %{private}s",
                selection.c_str());
            break;
        }
        selection.replace(argPos, 1, "? OR 1=1)");
        selection.replace(pos, MEDIA_DATA_DB_MEDIA_TYPE.length(), "(" + PhotoAlbumColumns::ALBUM_ID);

        selectionArgs[argIndex] = "1";
        pos = argPos + 1;
    }
}

static void GetSqlArgs(MediaLibraryCommand &cmd, string &sql, vector<string> &selectionArgs,
    const vector<string> &columns)
{
    string clause = cmd.GetAbsRdbPredicates()->GetWhereClause();
    selectionArgs = cmd.GetAbsRdbPredicates()->GetWhereArgs();
    sql = "SELECT ";
    for (size_t i = 0; i < columns.size(); i++) {
        if (i != columns.size() - 1) {
            sql += columns[i] + ",";
        } else {
            sql += columns[i];
        }
    }
    sql += " FROM " + cmd.GetAbsRdbPredicates()->GetTableName();
    sql += " WHERE ";
    ReplaceRelativePath(clause, selectionArgs);
    ReplaceMediaType(clause, selectionArgs);
    sql += clause;
}

static void QueryAlbumDebug(MediaLibraryCommand &cmd, const vector<string> &columns,
    const shared_ptr<MediaLibraryRdbStore> store)
{
    MEDIA_DEBUG_LOG("Querying album, table: %{private}s selections: %{private}s",
        cmd.GetAbsRdbPredicates()->GetTableName().c_str(), cmd.GetAbsRdbPredicates()->GetWhereClause().c_str());
    for (const auto &arg : cmd.GetAbsRdbPredicates()->GetWhereArgs()) {
        MEDIA_DEBUG_LOG("Querying album, arg: %{private}s", arg.c_str());
    }
    for (const auto &col : columns) {
        MEDIA_DEBUG_LOG("Querying album, col: %{private}s", col.c_str());
    }

    auto resultSet = store->Query(cmd, columns);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Failed to query file!");
    int32_t count = -1;
    int32_t err = resultSet->GetRowCount(count);
    CHECK_AND_RETURN_LOG(err == E_OK, "Failed to get count, err: %{public}d", err);
    MEDIA_DEBUG_LOG("Querying album, count: %{public}d", count);
}

static void QuerySqlDebug(const string &sql, const vector<string> &selectionArgs, const vector<string> &columns,
    const shared_ptr<MediaLibraryRdbStore> store)
{
    constexpr int32_t printMax = 512;
    for (size_t pos = 0; pos < sql.size(); pos += printMax) {
        MEDIA_DEBUG_LOG("Quering album sql: %{private}s", sql.substr(pos, printMax).c_str());
    }
    for (const auto &arg : selectionArgs) {
        MEDIA_DEBUG_LOG("Quering album, arg: %{private}s", arg.c_str());
    }
    for (const auto &col : columns) {
        MEDIA_DEBUG_LOG("Quering album, col: %{private}s", col.c_str());
    }
    auto resultSet = store->QuerySql(sql, selectionArgs);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Failed to query album!");

    int32_t count = -1;
    int32_t err = resultSet->GetRowCount(count);
    CHECK_AND_RETURN_LOG(err == E_OK, "Failed to get count, err: %{public}d", err);
    MEDIA_DEBUG_LOG("Quering album, count: %{public}d", count);
}
#endif

shared_ptr<ResultSet> MediaLibraryAlbumOperations::QueryAlbumOperation(
    MediaLibraryCommand &cmd, const vector<string> &columns)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, nullptr, "uniStore is nullptr!");

    if (cmd.GetOprnObject() == OperationObject::MEDIA_VOLUME) {
        return PhotoStorageOperation().FindStorage(uniStore);
    }

    string whereClause = cmd.GetAbsRdbPredicates()->GetWhereClause();
    if (whereClause.find(MEDIA_DATA_DB_RELATIVE_PATH) != string::npos ||
        whereClause.find(MEDIA_DATA_DB_MEDIA_TYPE) != string::npos) {
        string sql;
        vector<string> selectionArgs;
        GetSqlArgs(cmd, sql, selectionArgs, columns);
        QuerySqlDebug(sql, selectionArgs, columns, uniStore);
        return uniStore->QuerySql(sql, selectionArgs);
    }

    QueryAlbumDebug(cmd, columns, uniStore);
    return uniStore->Query(cmd, columns);
}

inline int32_t GetStringObject(const ValuesBucket &values, const string &key, string &value)
{
    value = "";
    ValueObject valueObject;
    if (values.GetObject(key, valueObject)) {
        valueObject.GetString(value);
    } else {
        return -EINVAL;
    }
    return E_OK;
}

inline int32_t GetIntVal(const ValuesBucket &values, const string &key, int32_t &value)
{
    value = 0;
    ValueObject valueObject;
    if (values.GetObject(key, valueObject)) {
        valueObject.GetInt(value);
    } else {
        return -EINVAL;
    }
    return E_OK;
}

static int32_t ObtainMaxAlbumOrder(int32_t &maxAlbumOrder)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, -E_HAS_DB_ERROR, "uniStore is nullptr! failed query album order");
    std::string queryMaxOrderSql = "SELECT Max(album_order) FROM " + PhotoAlbumColumns::TABLE;
    auto resultSet = uniStore->QuerySql(queryMaxOrderSql);
    bool cond = (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK);
    CHECK_AND_RETURN_RET_LOG(!cond, -E_HAS_DB_ERROR, "Failed to query album!");
    return resultSet->GetInt(0, maxAlbumOrder);
}

static void PrepareUserAlbum(const string &albumName, ValuesBucket &values)
{
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, albumName);
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumType::USER);
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::USER_GENERIC);
    values.PutLong(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
    values.PutLong(PhotoAlbumColumns::CHANGE_TIME, MediaFileUtils::UTCTimeMilliSeconds());
    values.PutInt(PhotoAlbumColumns::ALBUM_IS_LOCAL, 1); // local album is 1.
    values.PutInt(PhotoAlbumColumns::ALBUM_PRIORITY, ALBUM_PRIORITY_DEFAULT);
    values.PutString(PhotoAlbumColumns::ALBUM_LPATH, USER_ALBUM_LPATH_PREFIX + albumName);
    values.PutLong(PhotoAlbumColumns::ALBUM_DATE_ADDED, MediaFileUtils::UTCTimeMilliSeconds());
    values.PutInt(PhotoAlbumColumns::UPLOAD_STATUS, PhotoAlbumUploadStatusOperation::GetAlbumUploadStatus());
}

static void PrepareSmartAlbum(const string &albumName, ValuesBucket &values)
{
    int64_t timestamp = MediaFileUtils::UTCTimeNanoSeconds();  // 统一时间戳
    string current_time = MINUS_5_SOMETHING + to_string(timestamp);
    values.PutString(ALBUM_NAME, albumName);
    values.PutInt(ALBUM_TYPE, PhotoAlbumType::SMART);
    values.PutInt(ALBUM_SUBTYPE, PhotoAlbumSubType::PORTRAIT);
    values.PutString(TAG_ID, current_time);
    values.PutString(GROUP_TAG, current_time);
    values.PutInt(USER_DISPLAY_LEVEL, DISPLAY_LEVEL_NUMBER);
    values.PutInt(IS_LOCAL, LOCAL_NUMBER); // local album is 1.
    values.PutInt(RENAME_OPERATION, RENAME_OPERATION_NUMBER);
}

inline void PrepareWhere(const string &albumName, const string &relativePath, RdbPredicates &predicates)
{
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_NAME, albumName);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(PhotoAlbumType::USER));
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::USER_GENERIC));
    if (relativePath.empty()) {
        predicates.IsNull(PhotoAlbumColumns::ALBUM_RELATIVE_PATH);
    } else {
        predicates.EqualTo(PhotoAlbumColumns::ALBUM_RELATIVE_PATH, relativePath);
    }
    predicates.NotEqualTo(PhotoAlbumColumns::ALBUM_DIRTY,
        to_string(static_cast<int32_t>(DirtyTypes::TYPE_DELETED)));
}

// Caller is responsible for checking @albumName AND @relativePath
int DoCreatePhotoAlbum(const string &albumName, const string &relativePath, const ValuesBucket& albumValues)
{
    // Build insert sql
    string sql;
    vector<ValueObject> bindArgs;
    sql.append("INSERT").append(" OR ROLLBACK").append(" INTO ").append(PhotoAlbumColumns::TABLE).append(" ");

    MediaLibraryRdbStore::BuildValuesSql(albumValues, bindArgs, sql);

    RdbPredicates wherePredicates(PhotoAlbumColumns::TABLE);
    PrepareWhere(albumName, relativePath, wherePredicates);
    sql.append(" WHERE NOT EXISTS (");
    MediaLibraryRdbStore::BuildQuerySql(wherePredicates, { PhotoAlbumColumns::ALBUM_ID }, bindArgs, sql);
    sql.append(")");
    MEDIA_DEBUG_LOG("DoCreatePhotoAlbum InsertSql: %{private}s", sql.c_str());

    int64_t lastInsertRowId = 0;
    AlbumAccurateRefresh albumRefresh(AccurateRefresh::CREATE_PHOTO_TABLE_BUSSINESS_NAME);
    albumRefresh.Init();
    lastInsertRowId = albumRefresh.ExecuteForLastInsertedRowId(sql, bindArgs, RdbOperation::RDB_OPERATION_ADD);
    CHECK_AND_RETURN_RET_LOG(lastInsertRowId >= 0, lastInsertRowId, "insert fail and rollback");
    MEDIA_INFO_LOG("Create photo album success, id: %{public}" PRId64, lastInsertRowId);
    if (lastInsertRowId > 0) {
        albumRefresh.Notify();
    }
    return lastInsertRowId;
}

static int32_t QueryExistingAlbumByLpath(const string& albumName, bool& isDeleted, bool& isSameName)
{
    const string lpath = USER_ALBUM_LPATH_PREFIX + albumName;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_FAIL, "fail to get rdbstore, lpath is: %{public}s", lpath.c_str());

    const string sql = "SELECT album_id, album_name, dirty FROM " + PhotoAlbumColumns::TABLE +
        " WHERE LOWER(lpath) = LOWER(?)";
    const vector<ValueObject> bindArgs { lpath };
    auto resultSet = rdbStore->QueryByStep(sql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_FAIL, "Query failed, lpath is: %{public}s", lpath.c_str());

    int32_t rowCount;
    CHECK_AND_RETURN_RET_LOG(resultSet->GetRowCount(rowCount) == NativeRdb::E_OK, E_FAIL,
        "Failed to get row count, lpath is: %{public}s", lpath.c_str());
    if (rowCount == 0) {
        // No existing album with same lpath is found
        return E_OK;
    }
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK, E_FAIL,
        "Failed to go to first row, lpath is: %{public}s", lpath.c_str());
    isDeleted =
        GetInt32Val(PhotoAlbumColumns::ALBUM_DIRTY, resultSet) == static_cast<int32_t>(DirtyTypes::TYPE_DELETED);
    string existingName = GetStringVal(PhotoAlbumColumns::ALBUM_NAME, resultSet);
    isSameName = existingName == albumName;
    return GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
}

static void UseDefaultCreateValue(const string& columnName, const ColumnSchema& schema,
    string& sql, vector<ValueObject>& bindArgs, int32_t albumId)
{
    // needs to set album order to the max value + 1
    if (columnName == PhotoAlbumColumns::ALBUM_ORDER) {
        sql.append("( SELECT COALESCE(MAX(album_order), 0) + 1 FROM PhotoAlbum WHERE album_id <> ?)");
        bindArgs.push_back(ValueObject {albumId});
    // otherwise, set the default value
    } else if (schema.defaultNull) {
        sql.append("NULL");
    } else {
        sql += '"';
        sql.append(schema.defaultValue);
        sql += '"';
    }
}

static string BuildReuseSql(int32_t id, const ValuesBucket& albumValues,
    const unordered_map<string, ColumnSchema>& photoAlbumSchema, vector<ValueObject>& bindArgs)
{
    map<string, ValueObject> createUserValuesMap;
    albumValues.GetAll(createUserValuesMap);
    string sql;
    sql.append("UPDATE PhotoAlbum SET ");
    for (auto schemaIter = photoAlbumSchema.begin(); schemaIter != photoAlbumSchema.end(); schemaIter++) {
        sql.append(schemaIter->first); // columnName
        sql.append(" = ");
        auto userValueIter = createUserValuesMap.find(schemaIter->first);
        if (userValueIter != createUserValuesMap.end()) {
            // Use the value from createUserValuesMap
            sql.append("?");
            bindArgs.push_back(userValueIter->second);
        } else {
            UseDefaultCreateValue(schemaIter->first, schemaIter->second, sql, bindArgs, id);
        }
        CHECK_AND_EXECUTE(std::next(schemaIter) == photoAlbumSchema.end(), sql.append(", "));
    }
    sql.append(" WHERE album_id = ?");
    bindArgs.push_back(ValueObject {id});
    return sql;
}

static unordered_map<string, ColumnSchema> QueryPhotoAlbumSchema()
{
    unordered_map<string, ColumnSchema> photoAlbumSchema;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, {}, "fail to get rdbstore");
    const string queryScheme = "PRAGMA table_info([PhotoAlbum])";
    auto resultSet = rdbStore->QueryByStep(queryScheme);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, {}, "Query failed");

    while (resultSet->GoToNextRow() == E_OK) {
        bool isPk = GetInt32Val("pk", resultSet) == 1;
        if (!isPk) {
            string colName = GetStringVal("name", resultSet);
            string defaultValue = GetStringVal("dflt_value", resultSet);
            string columnType = GetStringVal("type", resultSet);
            bool isNull;
            int32_t dfltIdx;
            resultSet->GetColumnIndex("dflt_value", dfltIdx);
            resultSet->IsColumnNull(dfltIdx, isNull);
            photoAlbumSchema[colName] = ColumnSchema {isNull, defaultValue, columnType};
        }
    }
    return photoAlbumSchema;
}

const unordered_map<string, ColumnSchema>& MediaLibraryAlbumOperations::GetPhotoAlbumTableSchema()
{
    static unordered_map<string, ColumnSchema> photoAlbumSchema {};

    if (photoAlbumSchema.empty()) {
        photoAlbumSchema = QueryPhotoAlbumSchema();
    }

    return photoAlbumSchema;
}

int32_t MediaLibraryAlbumOperations::RenewDeletedPhotoAlbum(int32_t id, const ValuesBucket& albumValues,
    std::shared_ptr<TransactionOperations> trans, AlbumAccurateRefresh *albumRefresh)
{
    MEDIA_INFO_LOG("Renew deleted PhotoAlbum start, album id: %{public}d", id);
    unordered_map<string, ColumnSchema> photoAlbumSchema = GetPhotoAlbumTableSchema();
    vector<NativeRdb::ValueObject> bindArgs;
    string sql = BuildReuseSql(id, albumValues, photoAlbumSchema, bindArgs);
    int32_t ret {};
    if (albumRefresh) {
        RdbPredicates predicates(PhotoAlbumColumns::TABLE);
        predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(id));
        albumRefresh->Init(predicates);
        ret = albumRefresh->ExecuteSql(sql, bindArgs, RdbOperation::RDB_OPERATION_UPDATE);
    } else if (trans) {
        ret = trans->ExecuteSql(sql, bindArgs);
    } else {
        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR,
            "Renew deleted PhotoAlbum execute sql failed, RdbStore is nullptr");
        ret = rdbStore->ExecuteSql(sql, bindArgs);
    }
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_HAS_DB_ERROR,
        "Renew deleted PhotoAlbum execute sql failed, id is: %{public}d", id);
    return E_OK;
}

bool MediaLibraryAlbumOperations::IsCoverInSystemAlbum(RdbPredicates &predicates, int32_t albumSubtype)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "Failed to get rdbStore when query");
    vector<string> columns = {PhotoColumn::MEDIA_IS_FAV, PhotoColumn::MEDIA_TYPE, PhotoColumn::MEDIA_DATE_TRASHED,
        PhotoColumn::PHOTO_STRONG_ASSOCIATION};
    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "failed to acquire result from visitor query.");
    bool ret = false;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        switch (albumSubtype) {
            case PhotoAlbumSubType::FAVORITE:
                ret = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_IS_FAV,
                    resultSet, TYPE_INT32)) == 1;
                break;
            case PhotoAlbumSubType::VIDEO:
                ret = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_TYPE,
                    resultSet, TYPE_INT32)) == MediaType::MEDIA_TYPE_VIDEO;
                break;
            case PhotoAlbumSubType::IMAGE:
                ret = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_TYPE,
                    resultSet, TYPE_INT32)) == MediaType::MEDIA_TYPE_IMAGE;
                break;
            case PhotoAlbumSubType::TRASH:
                ret = get<int64_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_DATE_TRASHED,
                    resultSet, TYPE_INT64)) == 0;
                break;
            case PhotoAlbumSubType::CLOUD_ENHANCEMENT:
                ret = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_STRONG_ASSOCIATION,
                    resultSet, TYPE_INT32)) == static_cast<int32_t>(StrongAssociationType::CLOUD_ENHANCEMENT);
                break;
            default:
                MEDIA_ERR_LOG("albumSubtype is invalid: %{public}d", albumSubtype);
                break;
        }
    } else {
        MEDIA_ERR_LOG("resultSet GoToNextRow failed, albumSubtype: %{public}d", albumSubtype);
    }
    return ret;
}

bool MediaLibraryAlbumOperations::IsCoverInAlbum(const string &fileId, int32_t albumSubtype, int32_t albumId)
{
    // determine if the cover belongs to the album
    // query owner_album_id
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "Failed to get rdbStore when query owner_album_id");
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);

    if (albumSubtype != -1) { // system album
        return IsCoverInSystemAlbum(predicates, albumSubtype);
    }
    // user album or source album
    vector<string> columns = {PhotoColumn::PHOTO_OWNER_ALBUM_ID};
    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "failed to acquire result from visitor query.");
    int32_t ownerAlbumId = -1;
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToNextRow() == NativeRdb::E_OK, false, "resultSet GoToNextRow failed.");
    ownerAlbumId =
        get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSet, TYPE_INT32));
    return ownerAlbumId == albumId;
}

int CreatePhotoAlbum(const string &albumName)
{
    int32_t err = MediaFileUtils::CheckAlbumName(albumName);
    if (err < 0) {
        MEDIA_ERR_LOG("Check album name failed, album name: %{private}s", albumName.c_str());
        return err;
    }

    ValuesBucket albumValues;
    PrepareUserAlbum(albumName, albumValues);

    // try to reuse existing record with same lpath first
    int32_t id = -1;
    bool isDeleted = false;
    bool isSameName = false;
    std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>(__func__);
    AlbumAccurateRefresh albumRefresh(AccurateRefresh::CREATE_PHOTO_TABLE_BUSSINESS_NAME, trans);
    std::function<int(void)> tryReuseDeleted = [&]()->int {
        id = QueryExistingAlbumByLpath(albumName, isDeleted, isSameName);
        if (id <= 0) {
            // id < 0 means error has occurred, id == 0 means no existing record.
            // needs to return in either case.
            return id;
        }
        MEDIA_INFO_LOG("%{public}s photo album with the same lpath exists, reuse the record id %{public}d.",
            isDeleted ? "Deleted" : "Existing", id);
        if (isDeleted) {
            int32_t ret = MediaLibraryAlbumOperations::RenewDeletedPhotoAlbum(id, albumValues, trans, &albumRefresh);
            CHECK_AND_PRINT_LOG(ret == E_OK, "Failed to update deleted album: %{public}s", albumName.c_str());
            return ret;
        }
        return E_OK;
    };
    int ret = trans->RetryTrans(tryReuseDeleted);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_HAS_DB_ERROR, "Try trans fail!, ret: %{public}d", ret);
    if (id > 0 && (!isSameName || isDeleted)) {
        albumRefresh.Notify();
        return id;
    }
    albumRefresh.CloseDfxReport();
    // no existing record available, create a new one
    return DoCreatePhotoAlbum(albumName, "", albumValues);
}

int32_t MediaLibraryAlbumOperations::CreatePortraitAlbum(const string &albumName)
{
    int32_t err = MediaFileUtils::CheckAlbumName(albumName);
    if (err < 0) {
        MEDIA_ERR_LOG("Check album name failed, album name: %{private}s", albumName.c_str());
        return err;
    }

    ValuesBucket albumValues;
    PrepareSmartAlbum(albumName, albumValues);

    // Build insert sql
    string insertsql;
    vector<ValueObject> bindArgs;
    insertsql.append("INSERT").append(" OR ROLLBACK").append(" INTO ").append("AnalysisAlbum").append(" ");

    MediaLibraryRdbStore::BuildValuesSql(albumValues, bindArgs, insertsql);

    RdbPredicates wherePredicates("AnalysisAlbum");
    wherePredicates.EqualTo(ALBUM_NAME, albumName);
    wherePredicates.EqualTo(ALBUM_TYPE, to_string(PhotoAlbumType::SMART));
    wherePredicates.EqualTo(ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::PORTRAIT));
    insertsql.append(" WHERE NOT EXISTS (");

    MediaLibraryRdbStore::BuildQuerySql(wherePredicates, { ALBUM_ID }, bindArgs, insertsql);
    insertsql.append(")");
    MEDIA_DEBUG_LOG("CreatePortraitAlbum InsertSql: %{private}s", insertsql.c_str());

    int64_t lastInsertRowId = 0;
    AlbumAccurateRefresh albumRefresh(AccurateRefresh::CREATE_PHOTO_TABLE_BUSSINESS_NAME);
    albumRefresh.Init();
    lastInsertRowId = albumRefresh.ExecuteForLastInsertedRowId(insertsql, bindArgs, RdbOperation::RDB_OPERATION_ADD);
    CHECK_AND_RETURN_RET_LOG(lastInsertRowId >= 0, lastInsertRowId, "insert fail and rollback");
    MEDIA_INFO_LOG("Create photo album success, id: %{public}" PRId64, lastInsertRowId);
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");
    watch->Notify(MediaFileUtils::GetUriByExtrConditions(PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX,
        std::to_string(lastInsertRowId)), NotifyType::NOTIFY_ADD);
    return lastInsertRowId;
}

int CreatePhotoAlbum(MediaLibraryCommand &cmd)
{
    string albumName;
    string type;
    string subtype;
    int err = GetStringObject(cmd.GetValueBucket(), PhotoAlbumColumns::ALBUM_NAME, albumName);
    GetStringObject(cmd.GetValueBucket(), PhotoAlbumColumns::ALBUM_SUBTYPE, subtype);
    if (err < 0 && subtype != to_string(PORTRAIT) && subtype != to_string(GROUP_PHOTO) && subtype != to_string(PET)) {
        return err;
    }
    int rowId;
    if (OperationObject::ANALYSIS_PHOTO_ALBUM == cmd.GetOprnObject()) {
        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        CHECK_AND_RETURN_RET(rdbStore != nullptr, E_HAS_DB_ERROR);
        int64_t outRowId = 0;
        auto ret = rdbStore->Insert(cmd, outRowId);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, outRowId, "insert fail, ret: %{public}d", ret);
        rowId = outRowId;
    } else {
        rowId = CreatePhotoAlbum(albumName);
    }
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");
    if (rowId > 0) {
        watch->Notify(MediaFileUtils::GetUriByExtrConditions(PhotoAlbumColumns::ALBUM_URI_PREFIX, to_string(rowId)),
            NotifyType::NOTIFY_ADD);
    }
    return rowId;
}

int32_t IsAllUserOrSourcePhotoAlbum(std::shared_ptr<MediaLibraryRdbStore> rdbStore, RdbPredicates &predicates)
{
    vector<string> albumIds = predicates.GetWhereArgs();
    RdbPredicates queryPredicates(PhotoAlbumColumns::TABLE);
    queryPredicates.In(PhotoAlbumColumns::ALBUM_ID, albumIds);
    queryPredicates.BeginWrap();
    queryPredicates.And()->BeginWrap()->EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(PhotoAlbumType::USER));
    queryPredicates.EndWrap();
    queryPredicates.Or();
    queryPredicates.And()->BeginWrap()->EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(PhotoAlbumType::SOURCE));
    queryPredicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::SOURCE_GENERIC));
    queryPredicates.EndWrap();
    queryPredicates.EndWrap();
    queryPredicates.And()->BeginWrap()->NotIn(PhotoAlbumColumns::ALBUM_LPATH, NOT_CHANGEABLE_ALBUM);
    queryPredicates.EndWrap();

    vector<string> columns = {PhotoAlbumColumns::ALBUM_ID};
    shared_ptr<ResultSet> resultSet = rdbStore->Query(queryPredicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "queryResultSet is null!");

    int32_t rowCount;
    int32_t ret = resultSet->GetRowCount(rowCount);
    if (ret != 0 || rowCount < 0) {
        MEDIA_ERR_LOG("result set get row count err %{public}d", ret);
        return E_HAS_DB_ERROR;
    }
    resultSet->Close();
    if (static_cast<size_t>(rowCount) != albumIds.size()) {
        int32_t nonUserAlbumUriCount = static_cast<int32_t>(albumIds.size()) - rowCount;
        MEDIA_ERR_LOG("deleted Albums Uri Contains non-user albums count is %{public}d",
            (int)nonUserAlbumUriCount);
        return E_INVALID_URI;
    }
    return E_OK;
}

static bool DeleteContainsOtherAlbum(RdbPredicates &predicates, const shared_ptr<MediaLibraryRdbStore>& rdbStore)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "DeleteContainsOtherAlbum failed. rdbStore is null");
    vector<string> albumIds = predicates.GetWhereArgs();
    RdbPredicates queryPredicates(PhotoAlbumColumns::TABLE);
    queryPredicates.In(PhotoAlbumColumns::ALBUM_ID, albumIds);
    queryPredicates.EqualTo(PhotoAlbumColumns::ALBUM_LPATH, "/Pictures/其它");

    vector<string> columns = {PhotoAlbumColumns::ALBUM_ID};
    shared_ptr<ResultSet> resultSet = rdbStore->Query(queryPredicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "queryResultSet is null!");

    int32_t rowCount = -1;
    int32_t ret = resultSet->GetRowCount(rowCount);
    if (ret != 0 || rowCount < 0) {
        MEDIA_ERR_LOG("result set get row count err %{public}d", ret);
        return false;
    }
    resultSet->Close();
    return rowCount > 0;
}

static int32_t RecreateOtherAlbum(AlbumAccurateRefresh *albumRefresh)
{
    CHECK_AND_RETURN_RET_LOG(albumRefresh != nullptr, E_HAS_DB_ERROR,
        "RecreateOtherAlbum failed. albumRefresh is null");
    int32_t ret = albumRefresh->ExecuteSql(CREATE_DEFALUT_ALBUM_FOR_NO_RELATIONSHIP_ASSET,
        AccurateRefresh::RDB_OPERATION_ADD);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_HAS_DB_ERROR,
        "Cannot insert new othersAlbum into PhotoAlbum table, ret: %{public}d", ret);
    return E_OK;
}

int32_t MediaLibraryAlbumOperations::DeletePhotoAlbum(RdbPredicates &predicates)
{
    // Only user generic albums can be deleted
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "DeletePhotoAlbum failed. rdbStore is null");

    int32_t ret = IsAllUserOrSourcePhotoAlbum(rdbStore, predicates);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("deleted Album is all not userAlbum type or sourceAlbum type");
        return E_INVALID_URI;
    }

    MEDIA_INFO_LOG("Delete user generic albums start");
    if (MediaLibraryRdbUtils::UpdateTrashedAssetOnAlbum(rdbStore, predicates) <= 0) {
        MEDIA_ERR_LOG("Update trashed asset failed");
        return E_HAS_DB_ERROR;
    }
    bool isOthers = DeleteContainsOtherAlbum(predicates, rdbStore);
    AlbumAccurateRefresh albumRefresh(AccurateRefresh::DELETE_PHOTO_ALBUMS_BUSSINESS_NAME);
    int deleteRow = -1;
    albumRefresh.LogicalDeleteReplaceByUpdate(predicates, deleteRow);
    if (deleteRow > 0) {
        if (isOthers) {
            ret = RecreateOtherAlbum(&albumRefresh);
        }
        albumRefresh.Notify();
    }
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");
    const vector<string> &notifyUris = predicates.GetWhereArgs();
    size_t count = notifyUris.size();
    for (size_t i = 0; i < count; i++) {
        if (deleteRow > 0) {
            watch->Notify(MediaFileUtils::GetUriByExtrConditions(PhotoAlbumColumns::ALBUM_URI_PREFIX,
                notifyUris[i]), NotifyType::NOTIFY_REMOVE);
        }
    }
    return deleteRow;
}

int32_t MediaLibraryAlbumOperations::DeleteHighlightAlbums(RdbPredicates &predicates)
{
    // Only Highlight albums can be deleted by this way
    MEDIA_INFO_LOG("Delete highlight albums");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "DeleteHighlightAlbum failed. rdbStore is null");

    ValuesBucket values;
    values.PutInt(HIGHLIGHT_STATUS, HIGHLIGHT_DELETED);
    int32_t changedRows = 0;
    int32_t result = rdbStore->Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(result == NativeRdb::E_OK, E_HAS_DB_ERROR,
        "Delete highlight album failed, result is %{private}d", result);
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");

    const vector<string> &notifyUris = predicates.GetWhereArgs();
    if (changedRows > 0) {
        for (size_t i = 0; i < notifyUris.size(); ++i) {
            watch->Notify(MediaFileUtils::GetUriByExtrConditions(PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX,
                notifyUris[i]), NotifyType::NOTIFY_REMOVE);
        }
    }
    return changedRows;
}

static void NotifyPortraitAlbum(const vector<int32_t> &changedAlbumIds)
{
    if (changedAlbumIds.size() <= 0) {
        return;
    }
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
    for (int32_t albumId : changedAlbumIds) {
        watch->Notify(MediaFileUtils::GetUriByExtrConditions(
            PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX, to_string(albumId)), NotifyType::NOTIFY_UPDATE);
    }
}

static void NotifyHighlightAlbum(const vector<int32_t> &changedAlbumIds)
{
    CHECK_AND_RETURN(changedAlbumIds.size() > 0);
    auto watch = MediaLibraryNotify::GetInstance();
    for (int32_t albumId : changedAlbumIds) {
        watch->Notify(MediaFileUtils::GetUriByExtrConditions(
            PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX, to_string(albumId)), NotifyType::NOTIFY_UPDATE);
    }
}

int32_t GetIntValueFromResultSet(shared_ptr<ResultSet> resultSet, const string &column, int &value)
{
    int index = E_INDEX;
    resultSet->GetColumnIndex(column, index);
    if (index == E_INDEX) {
        return E_HAS_DB_ERROR;
    }
    if (resultSet->GetInt(index, value) != NativeRdb::E_OK) {
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

int32_t GetStringValueFromResultSet(shared_ptr<ResultSet> resultSet, const string &column, string &value)
{
    if (resultSet == nullptr) {
        return E_HAS_DB_ERROR;
    }
    int index = E_INDEX;
    resultSet->GetColumnIndex(column, index);
    if (index == E_INDEX) {
        return E_HAS_DB_ERROR;
    }
    if (resultSet->GetString(index, value) != NativeRdb::E_OK) {
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

void GetDisplayLevelAlbumPredicates(const int32_t value, DataShare::DataSharePredicates &predicates)
{
    string whereClause;
    string whereClauseRelatedMe = "(SELECT " + MAP_ALBUM + " FROM " + ANALYSIS_PHOTO_MAP_TABLE +
            " WHERE " + MAP_ASSET + " IN ( SELECT " + MediaColumn::MEDIA_ID + " FROM " + PhotoColumn::PHOTOS_TABLE +
            " WHERE " + MediaColumn::MEDIA_ID + " IN (SELECT " + MAP_ASSET + " FROM " + ANALYSIS_PHOTO_MAP_TABLE +
            " WHERE " + MAP_ASSET + " IN (SELECT " + MAP_ASSET + " FROM " + ANALYSIS_PHOTO_MAP_TABLE + " WHERE " +
            MAP_ALBUM + " IN(SELECT " + ALBUM_ID + " FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " + IS_ME + " = 1))" +
            " GROUP BY " + MAP_ASSET + " HAVING count(" + MAP_ASSET + ") > 1) AND " + MediaColumn::MEDIA_DATE_TRASHED +
            " = 0) AND " + MAP_ALBUM + " NOT IN (SELECT " + ALBUM_ID + " FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " +
            IS_ME + " = 1)" + " GROUP BY " + MAP_ALBUM + " HAVING count(" + MAP_ALBUM + ") >= " +
            to_string(PORTRAIT_FIRST_PAGE_MIN_COUNT_RELATED_ME) + ")";
    std::string whereClauseAlbumName = ALBUM_NAME + " IS NOT NULL AND " + ALBUM_NAME + " != ''";
    std::string whereClauseDisplayNull = USER_DISPLAY_LEVEL + " IS NULL";

    bool isStore = OHOS::system::GetBoolParameter(RETAIL_MODE_KEY, false);
    int32_t minFirstPageCount = isStore ? STORE_PORTRAIT_FIRST_PAGE_MIN_COUNT : PORTRAIT_FIRST_PAGE_MIN_COUNT;
    int32_t minSecondPageCount = isStore ?
        STORE_PORTRAIT_SECOND_PAGE_MIN_COUNT : PORTRAIT_SECOND_PAGE_MIN_PICTURES_COUNT;
    if (value == FIRST_PAGE) {
        string relatedMeFirstPage = ALBUM_ID + " IN " + whereClauseRelatedMe;
        string whereClauseDisplay = USER_DISPLAY_LEVEL + " = 1 OR " + USER_DISPLAY_LEVEL + " = 0";
        string whereClauseIsMe = IS_ME + " = 1";
        string whereClauseSatifyCount = COUNT + " >= " + to_string(minFirstPageCount);
        string whereClauseWhenDisplayNull = "(" + whereClauseIsMe + ") OR (" + relatedMeFirstPage + ") OR (" +
                                            whereClauseSatifyCount + ") OR (" + whereClauseAlbumName + ")";
        whereClause = ALBUM_SUBTYPE + " = " + to_string(PORTRAIT) + " AND ((" + whereClauseDisplay + ") OR ((" +
                      whereClauseDisplayNull + ") AND (" + whereClauseWhenDisplayNull + "))) GROUP BY " +
                      GROUP_TAG + " ORDER BY CASE WHEN " + IS_ME + " != 0 THEN 0 ELSE 1 END, CASE WHEN " +
                      RENAME_OPERATION + " != 0 THEN 0 ELSE 1 END, " + COUNT + " DESC";
    } else if (value == SECOND_PAGE) {
        string whereClauseIsNotMe = IS_ME + " != 1 OR " + IS_ME + " IS NULL";
        string whereClauseDisplay = USER_DISPLAY_LEVEL + " = 2 OR " + USER_DISPLAY_LEVEL + " = -2";
        string whereClauseSatifyCount =
            COUNT + " < " + to_string(minFirstPageCount) + " AND " + COUNT + " >= " + to_string(minSecondPageCount);
        string whereClauseWhenDisplayNull = "(" + whereClauseIsNotMe + ") AND (" + whereClauseSatifyCount +
                                            ") AND NOT (" + whereClauseAlbumName + ") AND (" + ALBUM_ID + " NOT IN " +
                                            whereClauseRelatedMe + ")";
        whereClause = ALBUM_SUBTYPE + " = " + to_string(PORTRAIT) + " AND ((" + whereClauseDisplay + ") OR ((" +
                      whereClauseDisplayNull + ") AND (" + whereClauseWhenDisplayNull + "))) GROUP BY " +
                      GROUP_TAG + " ORDER BY CASE WHEN " + IS_ME + " != 0 THEN 0 ELSE 1 END, CASE WHEN " +
                      RENAME_OPERATION + " != 0 THEN 0 ELSE 1 END, " + COUNT + " DESC";
    } else if (value == FAVORITE_PAGE) {
        whereClause = ALBUM_SUBTYPE + " = " + to_string(PORTRAIT) + " AND (" + USER_DISPLAY_LEVEL + " = 3 )GROUP BY " +
            GROUP_TAG + " ORDER BY " + RANK;
    } else {
        MEDIA_ERR_LOG("The display level is invalid");
        whereClause = "";
    }
    predicates.SetWhereClause(whereClause);
}

int32_t GetPortraitSubtype(const string &subtypeName, const string &whereClause, const vector<string> &whereArgs)
{
    size_t pos = whereClause.find(subtypeName);
    if (pos == string::npos) {
        MEDIA_ERR_LOG("whereClause is invalid");
        return E_INDEX;
    }
    size_t argsIndex = 0;
    for (size_t i = 0; i < pos; i++) {
        if (whereClause[i] == '?') {
            argsIndex++;
        }
    }
    if (argsIndex > whereArgs.size() - 1) {
        MEDIA_ERR_LOG("whereArgs is invalid");
        return E_INDEX;
    }
    return atoi(whereArgs[argsIndex].c_str());
}

bool IsSupportQueryIsMe()
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr! failed query album order");
        return false;
    }
    const std::string queryAnalyzedPic = "SELECT " + MEDIA_COLUMN_COUNT_1 + " FROM " + VISION_TOTAL_TABLE + " WHERE " +
        FACE + " = " + to_string(FACE_ANALYSISED_STATE) + " OR " +
        FACE + " = " + to_string(FACE_NO_NEED_ANALYSIS_STATE);
    auto resultSetAnalyzed = uniStore->QuerySql(queryAnalyzedPic);
    bool cond = (resultSetAnalyzed == nullptr || resultSetAnalyzed->GoToFirstRow() != NativeRdb::E_OK);
    CHECK_AND_RETURN_RET(!cond, false);

    int analyzedCount;
    if (GetIntValueFromResultSet(resultSetAnalyzed, MEDIA_COLUMN_COUNT_1, analyzedCount) != NativeRdb::E_OK) {
        return false;
    }
    if (analyzedCount <= 0) {
        return false;
    }

    const std::string queryAllPic = "SELECT " + MEDIA_COLUMN_COUNT_1 + " FROM " + VISION_TOTAL_TABLE;
    auto resultSetTotal = uniStore->QuerySql(queryAnalyzedPic);
    cond = (resultSetTotal == nullptr || resultSetTotal->GoToFirstRow() != NativeRdb::E_OK);
    CHECK_AND_RETURN_RET(!cond, false);
    int totleCount;
    if (GetIntValueFromResultSet(resultSetTotal, MEDIA_COLUMN_COUNT_1, totleCount) != NativeRdb::E_OK) {
        return false;
    }
    if (totleCount == 0 ||
        (analyzedCount * PERCENTAGE_FOR_SUPPORT_QUERY_ISME / totleCount <= SUPPORT_QUERY_ISME_MIN_COUNT)) {
        MEDIA_INFO_LOG("Analyzed proportion less than 80");
        return false;
    }
    return true;
}

void GetIsMeAlbumPredicates(const int32_t value, DataShare::DataSharePredicates &predicates)
{
    string selection;
    if (value == QUERY_PROB_IS_ME_VALUE) {
        if (!IsSupportQueryIsMe()) {
            MEDIA_ERR_LOG("Not support to query isMe");
            return;
        }
        selection = ANALYSIS_ALBUM_TABLE + "." + ALBUM_SUBTYPE + " = " + to_string(PORTRAIT) +
            " GROUP BY " + ANALYSIS_ALBUM_TABLE + "." + GROUP_TAG + " HAVING SUM(CASE WHEN " +
            PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_FRONT_CAMERA + " = 1 THEN 1 ELSE " +
            " 0 END) > 0 " + " ORDER BY SUM(CASE WHEN " + PhotoColumn::PHOTOS_TABLE + "." +
            PhotoColumn::PHOTO_FRONT_CAMERA + " = 1 THEN 1 ELSE 0 END) DESC ";
    } else if (value == QUERY_IS_ME_VALUE) {
        selection = ALBUM_SUBTYPE + " = " + to_string(PORTRAIT) + " AND " + IS_ME + " = 1 GROUP BY " + GROUP_TAG;
    } else {
        MEDIA_ERR_LOG("The value is not support for query is me");
        return;
    }
    predicates.SetWhereClause(selection);
}

void GetAlbumNameNotNullPredicates(int32_t value, DataShare::DataSharePredicates &predicates)
{
    if (value != ALBUM_NAME_NOT_NULL_ENABLED) {
        MEDIA_ERR_LOG("The value is not support for query not null");
        return;
    }
    string selection = ALBUM_SUBTYPE + " = " + to_string(PORTRAIT) + " AND " + PhotoAlbumColumns::ALBUM_NAME +
        " IS NOT NULL GROUP BY " + GROUP_TAG;
    predicates.SetWhereClause(selection);
}

void GetIsMeLeftJoinPredicates(RdbPredicates &rdbPredicates)
{
    std::string onClause = ANALYSIS_ALBUM_TABLE + "." + ALBUM_ID + " = " +
        ANALYSIS_PHOTO_MAP_TABLE + "." + MAP_ALBUM;
    rdbPredicates.LeftOuterJoin(ANALYSIS_PHOTO_MAP_TABLE)->On({ onClause });
    onClause = ANALYSIS_PHOTO_MAP_TABLE + "." + MAP_ASSET + " = " +
        PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_ID;
    rdbPredicates.LeftOuterJoin(PhotoColumn::PHOTOS_TABLE)->On({ onClause });
}

static void ApplyLimitOffsetFromAbs(NativeRdb::RdbPredicates &rdbPredicates,
    const NativeRdb::AbsRdbPredicates *absPredicates)
{
    if (absPredicates == nullptr) {
        return;
    }
    int limit = absPredicates->GetLimit();
    if (limit >= 0) {
        rdbPredicates.Limit(limit);
    }
    int offset = absPredicates->GetOffset();
    if (offset >= 0) {
        rdbPredicates.Offset(offset);
    }
}

std::shared_ptr<NativeRdb::ResultSet> MediaLibraryAlbumOperations::QueryPortraitAlbum(MediaLibraryCommand &cmd,
    const std::vector<std::string> &columns)
{
    auto predicates = cmd.GetAbsRdbPredicates();
    auto whereClause = predicates->GetWhereClause();
    auto whereArgs = predicates->GetWhereArgs();
    DataShare::DataSharePredicates predicatesPortrait;
    if (whereClause.find(USER_DISPLAY_LEVEL) != string::npos) {
        int32_t value = GetPortraitSubtype(USER_DISPLAY_LEVEL, whereClause, whereArgs);
        if (value == E_INDEX) {
            return nullptr;
        }
        GetDisplayLevelAlbumPredicates(value, predicatesPortrait);
    } else if (whereClause.find(IS_ME) != string::npos) {
        int32_t value = GetPortraitSubtype(IS_ME, whereClause, whereArgs);
        bool cond = (value == E_INDEX || (value != QUERY_PROB_IS_ME_VALUE && value != QUERY_IS_ME_VALUE));
        CHECK_AND_RETURN_RET(!cond, nullptr);
        GetIsMeAlbumPredicates(value, predicatesPortrait);
    } else if (whereClause.find(ALBUM_NAME_NOT_NULL) != string::npos) {
        int32_t value = GetPortraitSubtype(ALBUM_NAME_NOT_NULL, whereClause, whereArgs);
        bool cond = (value == E_INDEX || value != ALBUM_NAME_NOT_NULL_ENABLED);
        CHECK_AND_RETURN_RET(!cond, nullptr);
        GetAlbumNameNotNullPredicates(value, predicatesPortrait);
    } else {
        MEDIA_INFO_LOG("QueryPortraitAlbum whereClause is error");
        return nullptr;
    }
    if (predicatesPortrait.GetWhereClause().empty()) {
        return nullptr;
    }
    auto rdbPredicates = RdbUtils::ToPredicates(predicatesPortrait, ANALYSIS_ALBUM_TABLE);
    ApplyLimitOffsetFromAbs(rdbPredicates, predicates);
    if (whereClause.find(IS_ME) != string::npos &&
        GetPortraitSubtype(IS_ME, whereClause, whereArgs) == QUERY_PROB_IS_ME_VALUE) {
        GetIsMeLeftJoinPredicates(rdbPredicates);
        std::vector<std::string> ismeColumns;
        for (auto &item : columns) {
            if (item.find(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, 0) == string::npos) {
                ismeColumns.push_back(item);
            }
        }
        ismeColumns.push_back(ANALYSIS_ALBUM_TABLE + "." + PhotoAlbumColumns::ALBUM_DATE_MODIFIED);
        ismeColumns.push_back("CAST(" + ANALYSIS_ALBUM_TABLE + "." + PhotoAlbumColumns::ALBUM_DATE_MODIFIED +
            " / 1000 AS BIGINT) AS date_modified_s");
        MEDIA_INFO_LOG("start query prob is me!!!");
        return MediaLibraryRdbStore::QueryWithFilter(rdbPredicates, ismeColumns);
    }
    return MediaLibraryRdbStore::QueryWithFilter(rdbPredicates, columns);
}

shared_ptr<ResultSet> MediaLibraryAlbumOperations::QueryPhotoAlbum(MediaLibraryCommand &cmd,
    const vector<string> &columns)
{
    if (cmd.GetAbsRdbPredicates()->GetOrder().empty()) {
        cmd.GetAbsRdbPredicates()->OrderByAsc(PhotoAlbumColumns::ALBUM_ORDER);
    }
    return MediaLibraryRdbStore::QueryWithFilter(*(cmd.GetAbsRdbPredicates()), columns);
}

static int32_t CheckIsSpecialSourceAlbum(const shared_ptr<MediaLibraryRdbStore>& rdbStore, int32_t oldAlbumId)
{
    const std::string QUERY_ALBUM_LPATH_TO_RENAME =
        "SELECT lpath FROM PhotoAlbum WHERE album_id = "+ std::to_string(oldAlbumId);
    std::string albumLPath = "";
    shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(QUERY_ALBUM_LPATH_TO_RENAME);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_OK, "resultSet is nullptr! failed query lpath");
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        GetStringValueFromResultSet(resultSet, PhotoAlbumColumns::ALBUM_LPATH, albumLPath);
    }
    CHECK_AND_RETURN_RET_LOG(
        std::find(NOT_CHANGEABLE_ALBUM.begin(), NOT_CHANGEABLE_ALBUM.end(), albumLPath) == NOT_CHANGEABLE_ALBUM.end(),
        E_HAS_DB_ERROR, "Conflict with not changeable album");
    return E_OK;
}

static int32_t CheckConflictsWithAlbumPlugin(const NativeRdb::ValuesBucket &newAlbumValues, int32_t oldAlbumType,
    const shared_ptr<MediaLibraryRdbStore>& rdbStore)
{
    if (oldAlbumType != PhotoAlbumType::SOURCE) {
        return E_OK;
    }
    string newAlbumName;
    string newLPath;
    ValueObject tmpObject;
    if (newAlbumValues.GetObject(PhotoAlbumColumns::ALBUM_NAME, tmpObject)) {
        tmpObject.GetString(newAlbumName);
    }
    if (newAlbumValues.GetObject(PhotoAlbumColumns::ALBUM_LPATH, tmpObject)) {
        tmpObject.GetString(newLPath);
    }
    int32_t rowCount = 0;
    std::string albumPluginlPathSql = "SELECT * FROM album_plugin WHERE LOWER(lpath) = LOWER(?)";
    shared_ptr<NativeRdb::ResultSet> resultSetAlbum = rdbStore->QueryByStep(albumPluginlPathSql, { newLPath });
    CHECK_AND_RETURN_RET_LOG(resultSetAlbum != nullptr, E_ERR, "Query albums with same lpath in album plugin failed");
    CHECK_AND_RETURN_RET_LOG(resultSetAlbum->GetRowCount(rowCount) == NativeRdb::E_OK, E_ERR,
        "Get albums with same lpath in album plugin row count failed");
    if (rowCount > 0) {
        MEDIA_ERR_LOG("same lpath exists in album plugin");
        return E_ERR;
    }

    std::string albumPluginAlbumNameSql =
        "SELECT * FROM album_plugin WHERE LOWER(album_name) = LOWER(?) OR LOWER(album_name_en) = LOWER(?)";
    resultSetAlbum = rdbStore->QueryByStep(albumPluginAlbumNameSql, { newAlbumName, newAlbumName });
    CHECK_AND_RETURN_RET_LOG(resultSetAlbum != nullptr, E_ERR, "Query albums with same name in album plugin failed");
    CHECK_AND_RETURN_RET_LOG(resultSetAlbum->GetRowCount(rowCount) == NativeRdb::E_OK, E_ERR,
        "Get albums with same name in album plugin row count failed");
    if (rowCount > 0) {
        MEDIA_ERR_LOG("same name exists in album plugin");
        return E_ERR;
    }
    return E_OK;
}

static int32_t HasSameEmptySourceAlbum(const shared_ptr<MediaLibraryRdbStore>& rdbStore,
    const string &newAlbumName, const string &newLPath)
{
    std::string querySql = "SELECT " + PhotoAlbumColumns::ALBUM_ID + " FROM " + PhotoAlbumColumns::TABLE +
                            " WHERE (" + PhotoAlbumColumns::ALBUM_NAME + " = ? OR " +
                            PhotoAlbumColumns::ALBUM_LPATH + " = ?) AND " +
                            PhotoAlbumColumns::ALBUM_TYPE + " = " + std::to_string(PhotoAlbumType::SOURCE) + " AND " +
                            PhotoAlbumColumns::ALBUM_COUNT + " = 0 AND " + PhotoAlbumColumns::HIDDEN_COUNT + " = 0";
    int32_t rowCount = 0;
    shared_ptr<NativeRdb::ResultSet> resultSetAlbum = rdbStore->QueryByStep(querySql, { newAlbumName, newLPath });
    CHECK_AND_RETURN_RET_LOG(resultSetAlbum != nullptr, E_ERR, "Query same empty source album failed");
    CHECK_AND_RETURN_RET_LOG(resultSetAlbum->GetRowCount(rowCount) == NativeRdb::E_OK, E_ERR,
        "Get same empty source album row count failed");
    if (rowCount <= 0) {
        return E_ERR;
    }
    CHECK_AND_RETURN_RET_LOG(resultSetAlbum->GoToFirstRow() == NativeRdb::E_OK, E_ERR,
        "Same empty source album go to first row failed, row count is %{public}d", rowCount);
    int32_t emptyAlbumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSetAlbum);
    MEDIA_INFO_LOG("Find empty source album with same name and lpath, id: %{public}d", emptyAlbumId);
    return emptyAlbumId;
}

/*
 * Check for conflicts with existing albums when setting album name
 * returns:
 *     - positive integer album id if needs to combine with an deleted album of same lpath
 *     - 0 if no conflicts is found and new name album can be created
 *     - negative integer if a conflict is found or error occurs and needs to abort
 */
static int32_t CheckConflictsWithExistingAlbum(const NativeRdb::ValuesBucket& newAlbumValues,
    const shared_ptr<MediaLibraryRdbStore>& rdbStore, const int32_t oldAlbumType)
{
    string newAlbumName;
    string newLPath;
    ValueObject tmpObject;
    if (newAlbumValues.GetObject(PhotoAlbumColumns::ALBUM_NAME, tmpObject)) {
        tmpObject.GetString(newAlbumName);
    }
    if (newAlbumValues.GetObject(PhotoAlbumColumns::ALBUM_LPATH, tmpObject)) {
        tmpObject.GetString(newLPath);
    }
    if (oldAlbumType == PhotoAlbumType::SOURCE) {
        int32_t emptyAlbumId = HasSameEmptySourceAlbum(rdbStore, newAlbumName, newLPath);
        if (emptyAlbumId > 0) {
            return emptyAlbumId;
        }
    }
    // Check if non-deleted album with same name exists
    std::string sql = "SELECT * FROM PhotoAlbum WHERE album_name = ? AND dirty <> ?";
    shared_ptr<NativeRdb::ResultSet> resultSetAlbum =
        rdbStore->QueryByStep(sql, { newAlbumName, static_cast<int32_t>(DirtyTypes::TYPE_DELETED) });
    CHECK_AND_RETURN_RET_LOG(resultSetAlbum != nullptr, E_ERR, "Query non-deleted album with same name failed");
    int32_t rowCount = 0;
    CHECK_AND_RETURN_RET_LOG(resultSetAlbum->GetRowCount(rowCount) == NativeRdb::E_OK, E_ERR,
        "Get non-deleted album with same name row count failed");
    CHECK_AND_RETURN_RET_LOG(rowCount <= 0, E_ERR, "Non-deleted album with same name exists");

    // Check albums with same lpath
    sql = "SELECT * FROM PhotoAlbum WHERE lpath = ?";
    resultSetAlbum = rdbStore->QueryByStep(sql, { newLPath });
    CHECK_AND_RETURN_RET_LOG(resultSetAlbum != nullptr, E_ERR, "Query albums with same lpath failed");
    CHECK_AND_RETURN_RET_LOG(resultSetAlbum->GetRowCount(rowCount) == NativeRdb::E_OK, E_ERR,
        "Get albums with same lpath row count failed");
    if (rowCount > 0) {
        CHECK_AND_RETURN_RET_LOG(resultSetAlbum->GoToFirstRow() == NativeRdb::E_OK, E_ERR,
            "Albums with same lpath go to first row failed, row count is %{public}d", rowCount);
        bool isDeleted = GetInt32Val(PhotoAlbumColumns::ALBUM_DIRTY, resultSetAlbum) ==
            static_cast<int32_t>(DirtyTypes::TYPE_DELETED);
        if (isDeleted) {
            int32_t albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSetAlbum);
            MEDIA_INFO_LOG("Found deleted album %{public}d with same lpath", albumId);
            return albumId;
        } else {
            MEDIA_ERR_LOG("Non-deleted album with same lpath exists");
            return E_ERR;
        }
    }
    return E_OK;
}

static bool BuildNewNameValuesBucket(const shared_ptr<MediaLibraryRdbStore>& rdbStore, int32_t albumId,
    NativeRdb::ValuesBucket& newNameValues, const string& newAlbumName, bool& isCloudAlbum)
{
    const std::string QUERY_OLD_ALBUM_INFO =
        "SELECT * FROM PhotoAlbum WHERE " + PhotoAlbumColumns::ALBUM_ID + " = " + to_string(albumId) +
        " AND (" + PhotoAlbumColumns::ALBUM_TYPE + " = " + to_string(PhotoAlbumType::USER) + " OR " +
        PhotoAlbumColumns::ALBUM_TYPE + " = " + to_string(PhotoAlbumType::SOURCE) + ")";
    shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(QUERY_OLD_ALBUM_INFO);
    CHECK_AND_RETURN_RET_LOG(TryToGoToFirstRow(resultSet), false,
        "Rename user album failed. Query old album info failed");
    MediaLibraryAlbumFusionUtils::BuildAlbumInsertValuesSetName(rdbStore, newNameValues, resultSet, newAlbumName);
    const string albumCloudId = GetStringVal(PhotoAlbumColumns::ALBUM_CLOUD_ID, resultSet);
    isCloudAlbum = !albumCloudId.empty();
    return true;
}

namespace {
    struct NewNameExecuteInfo {
        int32_t oldAlbumId;
        int64_t newAlbumId;
        bool isCloudAlbum;
    };
}

static bool SetNewNameExecute(MediaLibraryAlbumFusionUtils::ExecuteObject& executeObject,
    NewNameExecuteInfo& executeInfo, NativeRdb::ValuesBucket& newNameValues, vector<string>& fileIdsInAlbum)
{
    if (executeInfo.newAlbumId > 0) {
        // Deleted album with same lpath exists
        int changeRows = 0;
        newNameValues.PutInt(PhotoAlbumColumns::ALBUM_DIRTY, static_cast<int32_t>(DirtyTypes::TYPE_MDIRTY));
        RdbPredicates rdbPredicatesNew(PhotoAlbumColumns::TABLE);
        rdbPredicatesNew.EqualTo(PhotoAlbumColumns::ALBUM_ID, executeInfo.newAlbumId);
        CHECK_AND_RETURN_RET_LOG(
            executeObject.albumRefresh->Update(changeRows, newNameValues, rdbPredicatesNew) == NativeRdb::E_OK,
            false, "Failed to update deleted album with same name");
        CHECK_AND_RETURN_RET_LOG(MediaLibraryAlbumFusionUtils::DeleteAlbumAndUpdateRelationship(executeObject,
            executeInfo.oldAlbumId, executeInfo.newAlbumId, executeInfo.isCloudAlbum, &fileIdsInAlbum) == E_OK,
            false, "Failed to merge old name album with new name album");
        MEDIA_INFO_LOG("Set photo album name: update deleted album with same name success,"
            "old album id: %{public}d, new album id: %{public}" PRId64,
            executeInfo.oldAlbumId, executeInfo.newAlbumId);
    } else {
        newNameValues.PutInt(PhotoAlbumColumns::ALBUM_DIRTY, static_cast<int32_t>(DirtyTypes::TYPE_NEW));

        CHECK_AND_RETURN_RET_LOG(executeObject.albumRefresh->Insert(executeInfo.newAlbumId, PhotoAlbumColumns::TABLE,
            newNameValues) == NativeRdb::E_OK, false, "Failed to insert new name album");
        CHECK_AND_RETURN_RET_LOG(MediaLibraryAlbumFusionUtils::DeleteAlbumAndUpdateRelationship(executeObject,
            executeInfo.oldAlbumId, executeInfo.newAlbumId, executeInfo.isCloudAlbum, &fileIdsInAlbum) == E_OK,
            false, "Failed to merge old name album with new name album");
        MEDIA_INFO_LOG("Set photo album name: insert new name album success,"
            "old album id: %{public}d, new album id: %{public}" PRId64,
            executeInfo.oldAlbumId, executeInfo.newAlbumId);
    }
    return true;
}

static void UpdateAnalysisIndexAfterRename(const vector<string>& fileIdsToUpdateIndex)
{
    MEDIA_INFO_LOG("update index fileIdsToUpdateIndex size: %{public}zu", fileIdsToUpdateIndex.size());
    if (fileIdsToUpdateIndex.size() > 0) {
        MediaAnalysisHelper::AsyncStartMediaAnalysisService(
            static_cast<int32_t>(MediaAnalysisProxy::ActivateServiceType::START_UPDATE_INDEX),
            fileIdsToUpdateIndex);
    }
}

static vector<string> GetAssetIdsFromOldAlbum(const shared_ptr<MediaLibraryRdbStore>& rdbStore,
    int32_t oldAlbumId)
{
    const std::string  QUERY_FILEID_TO_UPDATE_INDEX =
        "SELECT file_id FROM Photos WHERE owner_album_id = " + to_string(oldAlbumId);
    vector<string> fileIds;
    shared_ptr<NativeRdb::ResultSet> queryIdsResultSet = rdbStore->QuerySql(QUERY_FILEID_TO_UPDATE_INDEX);
    CHECK_AND_PRINT_LOG(queryIdsResultSet != nullptr, "Query file id to update index failed");
    while (queryIdsResultSet != nullptr && queryIdsResultSet->GoToNextRow() == NativeRdb::E_OK) {
        fileIds.push_back(to_string(GetInt32Val("file_id", queryIdsResultSet)));
    }
    return fileIds;
}

static int32_t GetAlbumTypeFromOldAlbum(const shared_ptr<MediaLibraryRdbStore>& rdbStore,
    int32_t oldAlbumId)
{
    const std::string QUERY_ALBUM_TYPE_TO_RENAME =
        "SELECT album_type FROM PhotoAlbum WHERE album_id = " + to_string(oldAlbumId);
    int32_t oldAlbumType = -1;
    shared_ptr<NativeRdb::ResultSet> queryTypeResultSet = rdbStore->QuerySql(QUERY_ALBUM_TYPE_TO_RENAME);
    CHECK_AND_PRINT_LOG(queryTypeResultSet != nullptr, "Query album type to rename album failed");
    if (queryTypeResultSet->GoToNextRow() == NativeRdb::E_OK) {
        oldAlbumType = GetInt32Val("album_type", queryTypeResultSet);
    }
    return oldAlbumType;
}

static int32_t NotifyForRenameUserAlbum(std::shared_ptr<AccurateRefresh::AlbumAccurateRefresh> &albumRefresh,
    std::shared_ptr<DfxRefreshManager> &dfxRefreshManager, const int64_t newAlbumId,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &assetRefresh)
{
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");
    std::vector<AlbumChangeData> albumChangeDatas = albumRefresh->GetAlbumChangeDatas();
    for (size_t i = 0; i < albumChangeDatas.size(); i++) {
        if (albumChangeDatas[i].infoAfterChange_.albumId_ == newAlbumId &&
            albumChangeDatas[i].infoAfterChange_.coverInfo_.fileId_ > 0) {
            albumChangeDatas[i].infoAfterChange_.coverInfo_.ownerAlbumId_ = newAlbumId;
            albumChangeDatas[i].infoAfterChange_.coverInfo_.ownerAlbumUri_ = MediaFileUtils::GetUriByExtrConditions(
                PhotoAlbumColumns::ALBUM_URI_PREFIX, to_string(newAlbumId));
        }
        if (albumChangeDatas[i].infoAfterChange_.albumId_ == newAlbumId &&
            albumChangeDatas[i].infoAfterChange_.hiddenCoverInfo_.fileId_ > 0) {
            albumChangeDatas[i].infoAfterChange_.hiddenCoverInfo_.ownerAlbumId_ = newAlbumId;
            albumChangeDatas[i].infoAfterChange_.hiddenCoverInfo_.ownerAlbumUri_ =
                MediaFileUtils::GetUriByExtrConditions(PhotoAlbumColumns::ALBUM_URI_PREFIX, to_string(newAlbumId));
        }
    }
    albumRefresh->Notify(albumChangeDatas, dfxRefreshManager);
    assetRefresh->Notify();
    watch->Notify(MediaFileUtils::GetUriByExtrConditions(PhotoAlbumColumns::ALBUM_URI_PREFIX,
        to_string(newAlbumId)), NotifyType::NOTIFY_UPDATE);
    return E_OK;
}

// Set album name: delete old and build a new one
static int32_t RenameUserAlbum(int32_t oldAlbumId, const string &newAlbumName)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "Rename album failed. RdbStore is null");
    CHECK_AND_RETURN_RET_LOG(oldAlbumId > 0, E_INVALID_ARGS, "Rename album failed. Invalid album id: %{public}d",
        oldAlbumId);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CheckAlbumName(newAlbumName) == E_OK, E_INVALID_ARGS,
        "Check album name failed");

    vector<string> fileIdsInAlbum = GetAssetIdsFromOldAlbum(rdbStore, oldAlbumId);
    int32_t oldAlbumType = GetAlbumTypeFromOldAlbum(rdbStore, oldAlbumId);
    CHECK_AND_RETURN_RET_LOG(oldAlbumType != PhotoAlbumType::SOURCE ||
        CheckIsSpecialSourceAlbum(rdbStore, oldAlbumId) == E_OK, E_HAS_DB_ERROR, "Check album name renameable failed");

    MEDIA_INFO_LOG("Start to set %{public}s album name of id %{public}d",
        oldAlbumType == PhotoAlbumType::SOURCE ? "source" : "user", oldAlbumId);

    bool argInvalid { false };
    std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>(__func__);
    int64_t newAlbumId = -1;

    auto albumRefresh = make_shared<AccurateRefresh::AlbumAccurateRefresh>(
        AccurateRefresh::RENAME_USER_ALBUM_BUSSINESS_NAME, trans);
    auto dfxRefreshManager = albumRefresh->GetDfxRefreshManager();
    albumRefresh->CloseDfxReport();
    auto assetRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>(
        AccurateRefresh::RENAME_USER_ALBUM_BUSSINESS_NAME, trans);
    if (dfxRefreshManager != nullptr) {
        assetRefresh->SetDfxRefreshManager(dfxRefreshManager);
    }
    std::function<int(void)> trySetUserAlbumName = [&]()->int {
        NativeRdb::ValuesBucket newNameValues {};
        bool isCloudAlbum {};
        if (!BuildNewNameValuesBucket(rdbStore, oldAlbumId, newNameValues, newAlbumName, isCloudAlbum)) {
            MEDIA_ERR_LOG("Build new name values bucket failed");
            argInvalid = true;
            return E_OK;
        }
        newAlbumId = -1;
        int32_t ret = CheckConflictsWithAlbumPlugin(newNameValues, oldAlbumType, rdbStore);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "New name conflicts with existing album plugin");
        ret = CheckConflictsWithExistingAlbum(newNameValues, rdbStore, oldAlbumType);
        if (ret < 0) {
            MEDIA_ERR_LOG("New name conflicts with existing album");
            argInvalid = true;
            return E_OK;
        } else if (ret > 0) {
            newAlbumId = ret;
        }
        MediaLibraryAlbumFusionUtils::ExecuteObject executeObject{rdbStore, trans, albumRefresh, assetRefresh};
        NewNameExecuteInfo executeInfo{oldAlbumId, newAlbumId, isCloudAlbum};
        CHECK_AND_RETURN_RET_LOG(
            SetNewNameExecute(executeObject, executeInfo, newNameValues, fileIdsInAlbum),
            E_HAS_DB_ERROR, "Set new name execute failed");
        newAlbumId = executeInfo.newAlbumId;
        return E_OK;
    };
    int ret = trans->RetryTrans(trySetUserAlbumName);
    if (argInvalid) {
        return E_INVALID_ARGUMENTS;
    }
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_HAS_DB_ERROR, "Try trans fail!, ret: %{public}d", ret);

    auto assetsRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    AccurateRefreshBase &refreshBase = *assetsRefresh;
    ret = LakeFileOperations::MoveInnerLakeAssetsToNewAlbum(refreshBase, fileIdsInAlbum, newAlbumId);
    CHECK_AND_PRINT_LOG(ret == E_OK, "move inner anco asset to new album error");

    UpdateAnalysisIndexAfterRename(fileIdsInAlbum);
    ret = NotifyForRenameUserAlbum(albumRefresh, dfxRefreshManager, newAlbumId, assetRefresh);
    CHECK_AND_PRINT_LOG(ret == E_OK, "failed to send notification for rename user album");
    return ALBUM_SETNAME_OK;
}

int32_t PrepareCoverUriUpdateValues(ValuesBucket& updateValues, const string& newCoverUri)
{
    updateValues.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, newCoverUri);

    if (updateValues.IsEmpty()) {
        return -EINVAL;
    }
    updateValues.PutLong(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
    return E_OK;
}

static bool GetOldAlbumInfo(int32_t albumId, string& oldAlbumName, string& oldAlbumCoverUri)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Get old album info failed. RdbStore is null");
        return false;
    }
    const std::string QUERY_ALBUM_INFO =
        "SELECT album_name, cover_uri FROM PhotoAlbum WHERE " +
        PhotoAlbumColumns::ALBUM_ID + " = " + to_string(albumId);
    shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(QUERY_ALBUM_INFO);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Get old album info failed. Query album info failed");
        return false;
    }
    if (resultSet->GoToNextRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Get old album info failed. Go to next row failed");
        return false;
    }
    oldAlbumName = GetStringVal(PhotoAlbumColumns::ALBUM_NAME, resultSet);
    oldAlbumCoverUri = GetStringVal(PhotoAlbumColumns::ALBUM_COVER_URI, resultSet);
    return true;
}

int32_t NotifyAlbumUpdate(const AbsRdbPredicates &predicates, const int32_t notIdArgs)
{
    int32_t ret = E_OK;
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");
        const vector<string> &notifyIds = predicates.GetWhereArgs();
    size_t count = notifyIds.size() - notIdArgs;
    for (size_t i = 0; i < count; i++) {
        ret = watch->Notify(MediaFileUtils::GetUriByExtrConditions(PhotoAlbumColumns::ALBUM_URI_PREFIX,
            notifyIds[i]), NotifyType::NOTIFY_UPDATE);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Notify album update failed");
    }
    return ret;
}

static int32_t CommitModifyUpdateAlbumCoverUri(int32_t albumId, RdbPredicates& rdbPredicates,
    const string& newAlbumCoverUri, AlbumAccurateRefresh& albumRefresh)
{
    ValuesBucket rdbValues;
    int32_t err = PrepareCoverUriUpdateValues(rdbValues, newAlbumCoverUri);
    CHECK_AND_RETURN_RET_LOG(err >= 0, err, "No values to update");
    // Only user generic albums can be updated
    rdbPredicates.And()->BeginWrap()->EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(PhotoAlbumType::USER));
    rdbPredicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::USER_GENERIC));
    rdbPredicates.Or()->EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(PhotoAlbumType::SMART));
    rdbPredicates.EndWrap();

    int32_t changedRows = -1;
    err = albumRefresh.Update(changedRows, rdbValues, rdbPredicates);
    CHECK_AND_PRINT_LOG(changedRows >= 0, "Update photo album failed: %{public}d", err);
    return changedRows;
}

static bool IsAlbumExist(int32_t albumId)
{
    RdbPredicates rdbPredicates(PhotoAlbumColumns::TABLE);
    rdbPredicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(albumId));
    rdbPredicates.And()->BeginWrap()->EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(PhotoAlbumType::USER));
    rdbPredicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::USER_GENERIC));
    rdbPredicates.Or()->EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(PhotoAlbumType::SMART));
    rdbPredicates.EndWrap();
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "RdbStore is null");
    auto resultSet = rdbStore->QueryByStep(rdbPredicates, { PhotoAlbumColumns::ALBUM_ID });
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "Query photo album failed");
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        return true;
    }
    return false;
}

int32_t MediaLibraryAlbumOperations::UpdatePhotoAlbum(const ValuesBucket &values, const DataSharePredicates &predicates)
{
    RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, PhotoAlbumColumns::TABLE);
    CHECK_AND_RETURN_RET_LOG(!rdbPredicates.GetWhereArgs().empty(), E_INVALID_ARGS,
        "Update photo album failed. Predicates empty");
    int32_t albumId = atoi(rdbPredicates.GetWhereArgs()[0].c_str());
    CHECK_AND_RETURN_RET_LOG(albumId > 0, E_INVALID_ARGS,
        "Invalid album id: %{public}s", rdbPredicates.GetWhereArgs()[0].c_str());

    string oldAlbumName {};
    string oldAlbumCoverUri {};
    string newAlbumName {};
    string newAlbumCoverUri {};
    bool needChangeCover = false;
    bool needRename = false;

    CHECK_AND_RETURN_RET_LOG(GetOldAlbumInfo(albumId, oldAlbumName, oldAlbumCoverUri), E_OK,
        "Get old album info failed");

    if (GetStringObject(values, PhotoAlbumColumns::ALBUM_COVER_URI, newAlbumCoverUri) == E_OK &&
        oldAlbumCoverUri != newAlbumCoverUri) {
        needChangeCover = true;
    }
    if (GetStringObject(values, PhotoAlbumColumns::ALBUM_NAME, newAlbumName) == E_OK &&
        oldAlbumName != newAlbumName) {
        needRename = true;
    }
    if (!needChangeCover && !needRename) {
        MEDIA_WARN_LOG("Update photo album %{public}d with empty values, return", albumId);
        return E_OK;
    }

    MEDIA_INFO_LOG("Start to update album %{public}d, new name: %{public}s, new cover: %{public}s", albumId,
        needRename ? DfxUtils::GetSafeAlbumName(newAlbumName).c_str() : "null",
        needChangeCover ? DfxUtils::GetSafeUri(newAlbumCoverUri).c_str() : "null");

    int32_t changedRows = 0;
    if (needChangeCover) {
        AlbumAccurateRefresh albumRefresh(AccurateRefresh::UPDATE_PHOTO_ALBUM_BUSSINESS_NAME);
        changedRows = CommitModifyUpdateAlbumCoverUri(albumId, rdbPredicates, newAlbumCoverUri, albumRefresh);
        if (changedRows > 0 && !needRename) { // No need to notify if album is to be renamed. Rename process will notify
            albumRefresh.Notify();
            const int32_t notIdArgs = 3;
            auto ret = NotifyAlbumUpdate(rdbPredicates, notIdArgs);
            CHECK_AND_PRINT_LOG(ret == E_OK, "Notify album update failed, ret:%{public}d", ret);
        }
    } else {
        changedRows = (IsAlbumExist(albumId) ? 1 : 0);
    }

    if (needRename) {
        // Rename process changes the album id, so put the rename process at the end
        int32_t ret = RenameUserAlbum(albumId, newAlbumName);
        CHECK_AND_RETURN_RET_LOG(ret >= 0, ret, "Rename user album failed");
    }

    return changedRows;
}

bool MediaLibraryAlbumOperations::IsManunalCloudCover(const string &fileId, string &coverCloudId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "Failed to get rdbStore when query");
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);

    vector<string> columns = {PhotoColumn::PHOTO_CLOUD_ID};
    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "failed to acquire result from visitor query.");
    bool ret = false;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        coverCloudId = get<std::string>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_CLOUD_ID,
            resultSet, TYPE_STRING));
        ret = coverCloudId != "";
    } else {
        MEDIA_ERR_LOG("resultSet GoToNextRow failed, fileId:%{public}s, coverCloudId:%{public}s",
            fileId.c_str(), coverCloudId.c_str());
    }
    resultSet->Close();
    return ret;
}

int32_t MediaLibraryAlbumOperations::UpdateCoverUriExecute(const SetCoverUriAlbumInfo& albumInfo,
    const string &coverUri, const string &fileId, int64_t coverDateTime, AlbumAccurateRefresh& albumRefresh)
{
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    ValuesBucket values;
    values.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, coverUri);
    auto dateModified = MediaFileUtils::UTCTimeMilliSeconds();
    values.PutLong(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, dateModified);
    values.PutLong(PhotoAlbumColumns::COVER_DATE_TIME, coverDateTime);
    string cloudId;
    bool isManunalCloudCover = IsManunalCloudCover(fileId, cloudId);
    MEDIA_INFO_LOG("albumId:%{public}d, coverUri:%{public}s, isManunalCloudCover:%{public}d",
        albumInfo.albumId, coverUri.c_str(), isManunalCloudCover);
    if (isManunalCloudCover) {
        values.PutInt(PhotoAlbumColumns::COVER_URI_SOURCE, CoverUriSource::MANUAL_CLOUD_COVER);
        values.PutString(PhotoAlbumColumns::COVER_CLOUD_ID, to_string(dateModified) + "," + cloudId);
        if (albumInfo.dirty == static_cast<int32_t>(DirtyTypes::TYPE_SYNCED)) {
            values.PutInt(PhotoAlbumColumns::ALBUM_DIRTY, static_cast<int32_t>(DirtyTypes::TYPE_MDIRTY));
        }
    } else {
        values.PutInt(PhotoAlbumColumns::COVER_URI_SOURCE, CoverUriSource::MANUAL_LOCAL_COVER);
        values.PutString(PhotoAlbumColumns::COVER_CLOUD_ID, to_string(dateModified) + ",");
    }

    string CHECK_COVER_VALID =
        PhotoAlbumColumns::ALBUM_ID + " = " + to_string(albumInfo.albumId) + " AND EXISTS (SELECT 1 FROM " +
        PhotoColumn::PHOTOS_TABLE + " WHERE " + MediaColumn::MEDIA_ID + " = " + fileId + " AND " +
        MediaColumn::MEDIA_DATE_TRASHED + " >= 0 AND " + MediaColumn::MEDIA_HIDDEN + " = 0 AND " +
        MediaColumn::MEDIA_TIME_PENDING + " = 0 AND " + PhotoColumn::PHOTO_IS_TEMP + " = 0 AND " +
        PhotoColumn::PHOTO_BURST_COVER_LEVEL + " = " + to_string(static_cast<int32_t>(BurstCoverLevelType::COVER)) +
        " AND " + PhotoColumn::PHOTO_SYNC_STATUS + " = 0 AND " + PhotoColumn::PHOTO_CLEAN_FLAG + " = 0)";

    predicates.SetWhereClause(CHECK_COVER_VALID);
    int32_t changedRows = albumRefresh.UpdateWithDateTime(values, predicates);
    CHECK_AND_PRINT_LOG(changedRows >= 0, "Update photo album failed: %{public}d", changedRows);

    return changedRows;
}

std::string GetQueryColumn(int32_t albumSubtype)
{
    string queryColumn = MediaColumn::MEDIA_DATE_TAKEN;
    if (albumSubtype == PhotoAlbumSubType::VIDEO || albumSubtype == PhotoAlbumSubType::IMAGE) {
        queryColumn = MediaColumn::MEDIA_DATE_ADDED;
    }
    return queryColumn;
}

int64_t GetCoverDateTime(const string &fileId, int32_t albumSubtype)
{
    auto columnName = GetQueryColumn(albumSubtype);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "Failed to get rdbStore when query");
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);

    vector<string> columns = {columnName};
    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "failed to acquire result from visitor query.");
    int64_t coverDateTime = 0;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        coverDateTime = get<int64_t>(ResultSetUtils::GetValFromColumn(columnName,
            resultSet, TYPE_INT64));
    } else {
        MEDIA_ERR_LOG("resultSet GoToNextRow failed, fileId:%{public}s", fileId.c_str());
    }
    resultSet->Close();
    return coverDateTime;
}

static int32_t QueryAlbumInfo(int32_t albumId, SetCoverUriAlbumInfo& albumInfo)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_FAIL, "Failed to get rdbStore when query");
    const std::string QUERY_ALBUM_INFO =
        "SELECT dirty FROM PhotoAlbum WHERE album_id = ?";
    shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(QUERY_ALBUM_INFO, { albumId });
    CHECK_AND_RETURN_RET_LOG(TryToGoToFirstRow(resultSet), E_FAIL, "Get album info failed. Query album info failed");
    albumInfo.albumId = albumId;
    albumInfo.dirty = GetInt32Val(PhotoAlbumColumns::ALBUM_DIRTY, resultSet);
    return E_OK;
}

int32_t MediaLibraryAlbumOperations::UpdateAlbumCoverUri(const ValuesBucket &values,
    const DataSharePredicates &predicates, bool isSystemAlbum)
{
    // 1.get album id
    RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, PhotoAlbumColumns::TABLE);
    CHECK_AND_RETURN_RET_LOG(!rdbPredicates.GetWhereArgs().empty(), E_INVALID_ARGS,
        "Update photo album failed. Predicates empty");
    int32_t albumId = atoi(rdbPredicates.GetWhereArgs()[0].c_str());
    CHECK_AND_RETURN_RET_LOG(albumId > 0, E_INVALID_ARGS,
        "Invalid album id: %{public}s", rdbPredicates.GetWhereArgs()[0].c_str());
    
    // 2.get fileId and determine if cover in the album
    string coverUri;
    int32_t ret = GetStringObject(values, PhotoAlbumColumns::ALBUM_COVER_URI, coverUri);
    CHECK_AND_RETURN_RET_LOG((ret == E_OK), ret, "GetStringObject error");

    int32_t albumSubtype = -1;
    if (isSystemAlbum) {
        int ret = GetIntVal(values, PhotoAlbumColumns::ALBUM_SUBTYPE, albumSubtype);
        CHECK_AND_RETURN_RET_LOG((ret == E_OK), ret, "GetIntVal error");
    }
    string fileId = MediaLibraryDataManagerUtils::GetFileIdFromPhotoUri(coverUri);
    auto coverDateTime = GetCoverDateTime(fileId, albumSubtype);

    // 3.update cover uri
    SetCoverUriAlbumInfo albumInfo;
    CHECK_AND_RETURN_RET_LOG(QueryAlbumInfo(albumId, albumInfo) == E_OK, E_ERR, "Query album info failed");
    AlbumAccurateRefresh albumRefresh;
    auto updateRows = UpdateCoverUriExecute(albumInfo, coverUri, fileId, coverDateTime, albumRefresh);
    CHECK_AND_RETURN_RET_LOG(updateRows == 1, E_ERR,
        "update coverUri failed ,maybe cover is invalid, updateRows = %{public}d", updateRows);

    // 4.notify photoalbum update
    albumRefresh.Notify();
    const int32_t notIdArgs = 0;
    ret = NotifyAlbumUpdate(rdbPredicates, notIdArgs);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Notify album update failed");
    return updateRows;
}

int32_t MediaLibraryAlbumOperations::ResetCoverUri(const ValuesBucket &values, const DataSharePredicates &predicates)
{
    RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, PhotoAlbumColumns::TABLE);
    CHECK_AND_RETURN_RET_LOG(!rdbPredicates.GetWhereArgs().empty(), E_INVALID_ARGS,
        "Update photo album failed. Predicates empty");
    string albumId = rdbPredicates.GetWhereArgs()[0];
    CHECK_AND_RETURN_RET_LOG(atoi(albumId.c_str()) > 0, E_INVALID_ARGS,
        "Invalid album id: %{public}s", rdbPredicates.GetWhereArgs()[0].c_str());

    RdbPredicates newPredicates(PhotoAlbumColumns::TABLE);
    ValuesBucket updateValues;
    updateValues.PutLong(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
    updateValues.PutInt(PhotoAlbumColumns::COVER_URI_SOURCE, CoverUriSource::DEFAULT_COVER);

    string UPDATE_CONDITION = PhotoAlbumColumns::ALBUM_ID + " = " + albumId + " AND " +
        PhotoAlbumColumns::COVER_URI_SOURCE + " > " + to_string(CoverUriSource::DEFAULT_COVER);

    newPredicates.SetWhereClause(UPDATE_CONDITION);

    int32_t changedRows = OHOS::Media::MediaLibraryRdbStore::UpdateWithDateTime(updateValues, newPredicates);
    CHECK_AND_PRINT_LOG(changedRows >= 0, "Update photo album failed: %{public}d", changedRows);

    int32_t albumSubtype = -1;
    int ret = GetIntVal(values, PhotoAlbumColumns::ALBUM_SUBTYPE, albumSubtype);
    CHECK_AND_RETURN_RET_LOG((ret == E_OK), ret, "GetIntVal error");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "Failed to get rdbStore.");

    const vector<string> subtypes = { to_string(albumSubtype) };
    const vector<string> albumIds = { albumId };
    if (albumSubtype == static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC)) {
        MediaLibraryRdbUtils::UpdateUserAlbumInternal(rdbStore, albumIds, true);
    } else if (albumSubtype == static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC)) {
        MediaLibraryRdbUtils::UpdateSourceAlbumInternal(rdbStore, albumIds, true);
    } else {
        MediaLibraryRdbUtils::UpdateSystemAlbumInternal(rdbStore, subtypes, true);
    }
    return changedRows;
}

int32_t MediaLibraryAlbumOperations::GetLPathFromSourcePath(const string& sourcePath, string& lPath,
                                                            int32_t mediaType)
{
    size_t pos1 = SOURCE_PATH_PREFIX.length();
    size_t pos2 = sourcePath.find_last_of("/");
    CHECK_AND_RETURN_RET_LOG(
        sourcePath.find(SOURCE_PATH_PREFIX) != std::string::npos && pos2 != string::npos && pos1 < pos2,
        E_INDEX,
        "get no valid source path: %{public}s", sourcePath.c_str());
    lPath = sourcePath.substr(pos1, pos2 - pos1);
    /*
        if lPath from source path is /Pictures/Screenshots,
        it should be converted to /Pictures/Screenrecords if the asset is a video
    */
    bool cond = (lPath == AlbumPlugin::LPATH_SCREEN_SHOTS && mediaType == MEDIA_TYPE_VIDEO);
    CHECK_AND_EXECUTE(!cond, lPath = AlbumPlugin::LPATH_SCREEN_RECORDS);
    return E_OK;
}

static bool HasSameLpath(const string &lPath, const string &assetId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "Failed to get rdbStore.");
    const std::string QUERY_LPATH = "SELECT * FROM PhotoAlbum WHERE lpath = '" + lPath + "'";
    shared_ptr<NativeRdb::ResultSet> albumResultSet = rdbStore->QuerySql(QUERY_LPATH);
    if (albumResultSet == nullptr || albumResultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return false;
    } else {
        int albumIdIndex;
        int32_t albumId;
        albumResultSet->GetColumnIndex(PhotoAlbumColumns::ALBUM_ID, albumIdIndex);
        CHECK_AND_RETURN_RET(albumResultSet->GetInt(albumIdIndex, albumId) == NativeRdb::E_OK, false);
        const std::string UPDATE_ALBUM_ID_IN_PHOTOS = "UPDATE Photos Set owner_album_id = " +
            to_string(albumId) + " WHERE file_id = " + assetId;
        int ret = rdbStore->ExecuteSql(UPDATE_ALBUM_ID_IN_PHOTOS);
        CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, false, "Update new album is fails");
    }
    return true;
}

void MediaLibraryAlbumOperations::RecoverAlbum(const string& assetId, const string& lPath,
                                               bool& isUserAlbum, int64_t& newAlbumId)
{
    CHECK_AND_RETURN_LOG(!lPath.empty(), "lPath empty, cannot recover album");
    CHECK_AND_RETURN_LOG(!HasSameLpath(lPath, assetId), "Has same lpath, no need to build new one");
    const string userAlbumMark = "/Users/";
    CHECK_AND_EXECUTE(lPath.find(userAlbumMark) == string::npos, isUserAlbum = true);

    MEDIA_INFO_LOG("new album need to build, lpath is %{public}s", lPath.c_str());
    string albumName;
    string bundleName = "";
    auto albumType = PhotoAlbumType::SOURCE;
    auto albumSubType = PhotoAlbumSubType::SOURCE_GENERIC;
    string queryExpiredAlbumInfo = "SELECT * FROM album_plugin WHERE lpath = '" +
        lPath + "' AND priority = '1'";
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "Failed to get rdbStore.");
    shared_ptr<NativeRdb::ResultSet> albumPluginResultSet = rdbStore->QuerySql(queryExpiredAlbumInfo);
    if (albumPluginResultSet == nullptr || albumPluginResultSet->GoToFirstRow() != NativeRdb::E_OK) {
        size_t pos = lPath.find_last_of("/");
        CHECK_AND_RETURN_LOG(
            pos != string::npos && pos + 1 < lPath.length(),
            "get album name fail, lpath is %{public}s", lPath.c_str());
        albumName = lPath.substr(pos + 1);
        if (isUserAlbum) {
            albumType = PhotoAlbumType::USER;
            albumSubType = PhotoAlbumSubType::USER_GENERIC;
        }
    } else {
        GetStringValueFromResultSet(albumPluginResultSet, PhotoAlbumColumns::ALBUM_BUNDLE_NAME, bundleName);
        GetStringValueFromResultSet(albumPluginResultSet, PhotoAlbumColumns::ALBUM_NAME, albumName);
    }

    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_PRIORITY, 1);
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, albumType);
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, albumSubType);
    values.PutString(PhotoAlbumColumns::ALBUM_LPATH, lPath);
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, albumName);
    values.PutString(PhotoAlbumColumns::ALBUM_BUNDLE_NAME, bundleName);
    values.PutLong(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
    values.PutLong(PhotoAlbumColumns::ALBUM_DATE_ADDED, MediaFileUtils::UTCTimeMilliSeconds());
    values.PutInt(
        PhotoAlbumColumns::UPLOAD_STATUS, PhotoAlbumUploadStatusOperation::GetAlbumUploadStatusWithLpath(lPath));
    int32_t ret = rdbStore->Insert(newAlbumId, PhotoAlbumColumns::TABLE, values);

    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK, "Insert album failed on recover assets");
    const std::string UPDATE_NEW_ALBUM_ID_IN_PHOTOS = "UPDATE Photos SET owner_album_id = " +
        to_string(newAlbumId) + " WHERE file_id = " + assetId;
    ret = rdbStore->ExecuteSql(UPDATE_NEW_ALBUM_ID_IN_PHOTOS);
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK, "Update new album is fails");
}

void MediaLibraryAlbumOperations::DealwithNoAlbumAssets(const vector<string> &whereArgs)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryAlbumOperations::DealwithNoAlbumAssets");
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("get uniStore fail");
        return;
    }
    int32_t ret = PhotoOwnerAlbumIdOperation().SetRdbStore(uniStore).SetFileIds(whereArgs).FixPhotoRelation();
    CHECK_AND_RETURN_LOG(ret == E_OK, "Fix photo relation failed");
}

static set<string> GetHiddenUri(const vector<string> &uris)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetHiddenUri");
    size_t count = uris.size() - THAN_AGR_SIZE;
    vector<string> ids;
    unordered_map<string, string> maps;
    for (size_t i = 0; i < count; i++) {
        string fileId = MediaFileUtils::GetIdFromUri(uris[i]);
        ids.push_back(fileId);
        maps[fileId] = uris[i];
    }
    set<string> result;
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("get uniStore fail");
        return result;
    }
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(MediaColumn::MEDIA_ID, ids);
    predicates.EqualTo(MediaColumn::MEDIA_HIDDEN, to_string(1));
    vector<string> columns = { MediaColumn::MEDIA_ID };
    auto resultSet = uniStore->Query(predicates, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("fail to query file on photo");
        return result;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = -1;
        if (resultSet->GetInt(0, fileId) == NativeRdb::E_OK) {
            result.insert(maps[to_string(fileId)]);
        }
    }
    return result;
}

int32_t MediaLibraryAlbumOperations::RecoverPhotoAssets(const DataSharePredicates &predicates)
{
    RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.GreaterThan(MediaColumn::MEDIA_DATE_TRASHED, to_string(0));
    vector<string> whereArgs = rdbPredicates.GetWhereArgs();
    MediaLibraryRdbStore::ReplacePredicatesUriToId(rdbPredicates);
    MEDIA_INFO_LOG("Start recover %{public}d assets from trash",
        static_cast<int32_t>(whereArgs.size()) - THAN_AGR_SIZE);

    MediaLibraryAlbumOperations::DealwithNoAlbumAssets(rdbPredicates.GetWhereArgs());
    // notify deferred processing session to restore image
    MultiStagesCaptureManager::RestorePhotos(rdbPredicates);

    ValuesBucket rdbValues;
    rdbValues.PutInt(MediaColumn::MEDIA_DATE_TRASHED, 0);

    AssetAccurateRefresh assetRefresh(AccurateRefresh::RECOVER_ASSETS_BUSSINESS_NAME);
    int32_t changedRows = assetRefresh.UpdateWithDateTime(rdbValues, rdbPredicates);
    if (changedRows < 0) {
        return changedRows;
    }
    int32_t ret = LakeFileOperations::MoveAssetsToLake(assetRefresh, rdbPredicates.GetWhereArgs());
    CHECK_AND_PRINT_LOG(ret == E_OK, "recover inner anco file error when recover asset");

    // set cloud enhancement to available
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
    EnhancementManager::GetInstance().RecoverTrashUpdateInternal(rdbPredicates.GetWhereArgs());
#endif
    MediaAnalysisHelper::StartMediaAnalysisServiceAsync(
        static_cast<int32_t>(MediaAnalysisProxy::ActivateServiceType::START_UPDATE_INDEX), whereArgs);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "Failed to get rdbStore");
    MediaLibraryRdbUtils::UpdateAnalysisAlbumByUri(rdbStore, whereArgs);
    assetRefresh.RefreshAlbum(NotifyAlbumType::SYS_ALBUM);
    std::set<string> hiddenSet = GetHiddenUri(whereArgs);
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");
    size_t count = whereArgs.size() - THAN_AGR_SIZE;
    for (size_t i = 0; i < count; i++) {
        string notifyUri = MediaFileUtils::Encode(whereArgs[i]);
        if (hiddenSet.count(whereArgs[i]) == 1) {
            watch->Notify(notifyUri, NotifyType::NOTIFY_UPDATE);
        } else {
            watch->Notify(notifyUri, NotifyType::NOTIFY_ADD);
            watch->Notify(notifyUri, NotifyType::NOTIFY_THUMB_ADD);
        }
        watch->Notify(notifyUri, NotifyType::NOTIFY_ALBUM_ADD_ASSET);
    }
    int trashAlbumId = watch->GetAlbumIdBySubType(PhotoAlbumSubType::TRASH);
    if (trashAlbumId > 0) {
        for (size_t i = 0; i < count; i++) {
            watch->Notify(MediaFileUtils::Encode(whereArgs[i]), NotifyType::NOTIFY_ALBUM_REMOVE_ASSET, trashAlbumId);
        }
    }
    assetRefresh.Notify();
    return changedRows;
}

void DealWithHighlightSdTable(const DataSharePredicates &predicates)
{
    RdbPredicates assetMapPredicates = RdbUtils::ToPredicates(predicates, ANALYSIS_ALBUM_ASSET_MAP_TABLE);
    const vector<string> &whereUriArgs = assetMapPredicates.GetWhereArgs();
    vector<string> whereIdArgs;
    whereIdArgs.reserve(whereUriArgs.size());
    for (const auto &arg : whereUriArgs) {
        if (!MediaFileUtils::StartsWith(arg, PhotoColumn::PHOTO_URI_PREFIX)) {
            continue;
        }
        whereIdArgs.push_back(MediaFileUri::GetPhotoId(arg));
    }
    assetMapPredicates.SetWhereArgs(whereIdArgs);
 
    RdbPredicates predicatesSdMap(ANALYSIS_ASSET_SD_MAP_TABLE);
    predicatesSdMap.And()->In(MAP_ASSET_SOURCE, assetMapPredicates.GetWhereArgs());
    vector<string> columns = { MAP_ASSET_SOURCE, MAP_ASSET_DESTINATION };
    auto resultSetQuery = MediaLibraryRdbStore::QueryWithFilter(predicatesSdMap, columns);
    if (resultSetQuery == nullptr) {
        MEDIA_ERR_LOG("get highlight video failed");
        return;
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Can not get rdbstore");
        return;
    }
    while (resultSetQuery->GoToNextRow() == NativeRdb::E_OK) {
        string assetId = to_string(GetInt32Val(MAP_ASSET_SOURCE, resultSetQuery));
        int32_t mapAssetDestination = GetInt32Val(MAP_ASSET_DESTINATION, resultSetQuery);

        string highlightVideoPath = "/storage/cloud/files/highlight/video/" + to_string(mapAssetDestination);
        MediaFileUtils::DeleteDir(highlightVideoPath);
        MEDIA_INFO_LOG("Delete highlight video path is: %{public}s", highlightVideoPath.c_str());
 
        const std::string DELETE_ITEM_FROM_SD_MAP =
            "DELETE FROM tab_analysis_asset_sd_map WHERE map_asset_source = " + assetId;
        int32_t ret = rdbStore->ExecuteSql(DELETE_ITEM_FROM_SD_MAP);
        CHECK_AND_CONTINUE_ERR_LOG(ret == NativeRdb::E_OK,
            "DELETE highlight video failed, id is: %{public}s", assetId.c_str());

        const std::string DELETE_ITEM_FROM_ALBUM_MAP =
            "DELETE FROM tab_analysis_album_asset_map WHERE map_asset = " + assetId;
        ret = rdbStore->ExecuteSql(DELETE_ITEM_FROM_ALBUM_MAP);
        CHECK_AND_CONTINUE_ERR_LOG(ret == NativeRdb::E_OK,
            "DELETE highlight video failed, id is: %{public}s", assetId.c_str());
    }
    MEDIA_INFO_LOG("Deal with highlight video finished");
}

int32_t MediaLibraryAlbumOperations::DeletePhotoAssets(const DataSharePredicates &predicates,
    const bool isAging, const bool compatible)
{
    MEDIA_INFO_LOG("DeletePhotoAssets start.");
    DealWithHighlightSdTable(predicates);
    RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, PhotoColumn::PHOTOS_TABLE);
    int32_t deletedRows = MediaLibraryAssetOperations::DeleteFromDisk(rdbPredicates, isAging, compatible);
    if (!isAging) {
        MediaAnalysisHelper::StartMediaAnalysisServiceAsync(
            static_cast<int32_t>(MediaAnalysisProxy::ActivateServiceType::START_DELETE_INDEX));
    }
    MEDIA_INFO_LOG("DeletePhotoAssets end.");
    return deletedRows;
}

int32_t MediaLibraryAlbumOperations::DeletePhotoAssetsCompleted(
    const DataSharePredicates &predicates, const bool isAging)
{
    MEDIA_INFO_LOG("DeletePhotoAssetsCompleted start.");
    DealWithHighlightSdTable(predicates);
    RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, PhotoColumn::PHOTOS_TABLE);
    int32_t deletedRows = MediaLibraryAssetOperations::DeletePermanently(rdbPredicates, isAging);
    CHECK_AND_EXECUTE(isAging,  MediaAnalysisHelper::StartMediaAnalysisServiceAsync(
        static_cast<int32_t>(MediaAnalysisProxy::ActivateServiceType::START_DELETE_INDEX)));
    return deletedRows;
}

int32_t AgingPhotoAssets(shared_ptr<int> countPtr)
{
    auto time = MediaFileUtils::UTCTimeMilliSeconds();
    DataSharePredicates predicates;
    predicates.GreaterThan(MediaColumn::MEDIA_DATE_TRASHED, to_string(0));
    predicates.And()->LessThanOrEqualTo(MediaColumn::MEDIA_DATE_TRASHED, to_string(time - AGING_TIME));
    int32_t ret = MediaLibraryAlbumOperations::DeletePhotoAssets(predicates, true, false);
    if (ret < 0) {
        return ret;
    }
    if (countPtr != nullptr) {
        *countPtr = ret;
    }
    return E_OK;
}

static int32_t ObtainAlbumOrders(const int32_t &currentAlbumId, const int32_t referenceAlbumId,
    int32_t &currentAlbumOrder, int32_t &referenceAlbumOrder)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr! failed query album order");
        return E_HAS_DB_ERROR;
    }
    const std::string queryCurrentAlbumOrder = "SELECT " + PhotoAlbumColumns::ALBUM_ORDER + " FROM " +
        PhotoAlbumColumns::TABLE + " WHERE " + PhotoAlbumColumns::ALBUM_ID + " = " + to_string(currentAlbumId);
    const std::string queryReferenceAlbumOrder = "SELECT " + PhotoAlbumColumns::ALBUM_ORDER + " FROM " +
        PhotoAlbumColumns::TABLE + " WHERE " + PhotoAlbumColumns::ALBUM_ID + " = " + to_string(referenceAlbumId);
    auto resultSet = uniStore->QuerySql(queryCurrentAlbumOrder);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return E_HAS_DB_ERROR;
    }
    int colIndex = -1;
    resultSet->GetColumnIndex(PhotoAlbumColumns::ALBUM_ORDER, colIndex);
    if (resultSet->GetInt(colIndex, currentAlbumOrder) != NativeRdb::E_OK) {
        return E_HAS_DB_ERROR;
    }
    resultSet = uniStore->QuerySql(queryReferenceAlbumOrder);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK
        || resultSet->GetInt(colIndex, referenceAlbumOrder) != NativeRdb::E_OK) {
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

static int32_t ExecuteSqls(AlbumAccurateRefresh &albumRefresh, const vector<string> &sqls, RdbOperation operation)
{
    int32_t err = NativeRdb::E_OK;
    for (const auto &sql : sqls) {
        err = albumRefresh.ExecuteSql(sql, operation);
        if (err != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Failed to exec: %{private}s", sql.c_str());
            break;
        }
    }
    return NativeRdb::E_OK;
}

static int32_t ExecSqls(const vector<string> &sqls, const shared_ptr<MediaLibraryRdbStore> store)
{
    int32_t err = NativeRdb::E_OK;
    for (const auto &sql : sqls) {
        err = store->ExecuteSql(sql);
        if (err != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Failed to exec: %{private}s", sql.c_str());
            break;
        }
    }
    return NativeRdb::E_OK;
}

static int32_t ObtainNotifyAlbumIds(int32_t &currentAlbumOrder, int32_t referenceAlbumOrder,
    vector<int32_t> &changedAlbumIds)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr! failed query album order");
        return E_DB_FAIL;
    }
    std::string queryAlbumIds = "";
    if (currentAlbumOrder < referenceAlbumOrder) {
        queryAlbumIds = "SELECT " + PhotoAlbumColumns::ALBUM_ID + " FROM " +
            PhotoAlbumColumns::TABLE + " WHERE " + PhotoAlbumColumns::ALBUM_ORDER + " >= " +
            to_string(currentAlbumOrder) + " AND " + PhotoAlbumColumns::ALBUM_ORDER +
            " < " + to_string(referenceAlbumOrder);
    } else {
        queryAlbumIds = "SELECT " + PhotoAlbumColumns::ALBUM_ID + " FROM " +
            PhotoAlbumColumns::TABLE + " WHERE " + PhotoAlbumColumns::ALBUM_ORDER + " >= " +
            to_string(referenceAlbumOrder) + " AND " + PhotoAlbumColumns::ALBUM_ORDER +
            " <= " + to_string(currentAlbumOrder);
    }
    auto resultSet = uniStore->QuerySql(queryAlbumIds);
    if (resultSet == nullptr) {
        return E_DB_FAIL;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        changedAlbumIds.push_back(GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet));
    }
    return E_OK;
}

static void NotifyOrderChange(vector<int32_t> &changedAlbumIds)
{
    if (changedAlbumIds.size() <= 0) {
        return;
    }
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
    for (int32_t &albumId : changedAlbumIds) {
        watch->Notify(MediaFileUtils::GetUriByExtrConditions(
            PhotoAlbumColumns::ALBUM_URI_PREFIX, to_string(albumId)), NotifyType::NOTIFY_UPDATE);
    }
}

static int32_t UpdateSortedOrder(AlbumAccurateRefresh &albumRefresh, const int32_t &currentAlbumId,
    const int32_t referenceAlbumId, int32_t &currentAlbumOrder, int32_t &referenceAlbumOrder)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr! failed query album order");
        return E_DB_FAIL;
    }
    std::string updateOtherAlbumOrder = "";
    std::string updateCurrentAlbumOrder = "";
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    if (currentAlbumOrder < referenceAlbumOrder) {
        predicates.BeginWrap();
        predicates.GreaterThan(PhotoAlbumColumns::ALBUM_ORDER, to_string(currentAlbumOrder));
        predicates.LessThan(PhotoAlbumColumns::ALBUM_ORDER, to_string(referenceAlbumOrder));
        predicates.EndWrap();
        updateOtherAlbumOrder = "UPDATE " + PhotoAlbumColumns::TABLE +
            " SET " + PhotoAlbumColumns::ALBUM_ORDER + " = " +
            PhotoAlbumColumns::ALBUM_ORDER + " -1 WHERE " + PhotoAlbumColumns::ALBUM_ORDER +
            " > " + to_string(currentAlbumOrder) +
            " and " + PhotoAlbumColumns::ALBUM_ORDER + " < " + to_string(referenceAlbumOrder);
        updateCurrentAlbumOrder = "UPDATE " + PhotoAlbumColumns::TABLE + " SET " + PhotoAlbumColumns::ALBUM_ORDER +
            " = " + to_string(referenceAlbumOrder) + " -1 WHERE " +
            PhotoAlbumColumns::ALBUM_ID + " = " + to_string(currentAlbumId);
    } else {
        predicates.BeginWrap();
        predicates.GreaterThanOrEqualTo(PhotoAlbumColumns::ALBUM_ORDER, to_string(referenceAlbumOrder));
        predicates.LessThan(PhotoAlbumColumns::ALBUM_ORDER, to_string(currentAlbumOrder));
        predicates.EndWrap();
        updateOtherAlbumOrder = "UPDATE " + PhotoAlbumColumns::TABLE +
            " SET " + PhotoAlbumColumns::ALBUM_ORDER + " = " +
            PhotoAlbumColumns::ALBUM_ORDER + " +1 WHERE " + PhotoAlbumColumns::ALBUM_ORDER + " >= " +
            to_string(referenceAlbumOrder) + " AND " + PhotoAlbumColumns::ALBUM_ORDER +
            " < " + to_string(currentAlbumOrder);
        updateCurrentAlbumOrder = "UPDATE " + PhotoAlbumColumns::TABLE +
            " SET " + PhotoAlbumColumns::ALBUM_ORDER + " = " +
            to_string(referenceAlbumOrder) + " WHERE " +
            PhotoAlbumColumns::ALBUM_ID + " = " + to_string(currentAlbumId);
    }
    predicates.Or();
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(currentAlbumId));
    albumRefresh.Init(predicates);
    vector<string> updateSortedAlbumsSqls = { updateOtherAlbumOrder, updateCurrentAlbumOrder};
    return ExecuteSqls(albumRefresh, updateSortedAlbumsSqls, RdbOperation::RDB_OPERATION_UPDATE);
}

static int32_t ObtainCurrentAlbumOrder(const int32_t &albumId, int32_t &albumOrder)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr! failed query album order");
        return E_HAS_DB_ERROR;
    }
    const std::string queryAlbumOrder = "SELECT " + PhotoAlbumColumns::ALBUM_ORDER + " FROM " +
        PhotoAlbumColumns::TABLE + " WHERE " + PhotoAlbumColumns::ALBUM_ID + " = " + to_string(albumId);
    auto resultSet = uniStore->QuerySql(queryAlbumOrder);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return E_HAS_DB_ERROR;
    }
    int colIndex = -1;
    resultSet->GetColumnIndex(PhotoAlbumColumns::ALBUM_ORDER, colIndex);
    if (resultSet->GetInt(colIndex, albumOrder) != NativeRdb::E_OK) {
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

static int32_t UpdateNullReferenceOrder(const int32_t &currentAlbumId,
    const int32_t &currentAlbumOrder, const int32_t &maxAlbumOrder)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr! failed query album order");
        return E_DB_FAIL;
    }
    std::string updateOtherAlbumOrder = "UPDATE " + PhotoAlbumColumns::TABLE + " SET " +
        PhotoAlbumColumns::ALBUM_ORDER + " = " + PhotoAlbumColumns::ALBUM_ORDER + " -1 WHERE " +
        PhotoAlbumColumns::ALBUM_ORDER + " > " + to_string(currentAlbumOrder) + " and " +
        PhotoAlbumColumns::ALBUM_ORDER + " <= " + to_string(maxAlbumOrder);
    std::string updateCurrentAlbumOrder = "UPDATE " + PhotoAlbumColumns::TABLE +
        " SET " + PhotoAlbumColumns::ALBUM_ORDER + " = " + to_string(maxAlbumOrder) +
        " WHERE " + PhotoAlbumColumns::ALBUM_ID + " = " + to_string(currentAlbumId);
    vector<string> updateSortedAlbumsSqls = { updateOtherAlbumOrder, updateCurrentAlbumOrder};
    return ExecSqls(updateSortedAlbumsSqls, uniStore);
}

static int32_t HandleNullReferenceCondition(const int32_t &currentAlbumId)
{
    int32_t maxAlbumOrder = 0;
    int err = ObtainMaxAlbumOrder(maxAlbumOrder);
    if (err != E_OK) {
        return E_HAS_DB_ERROR;
    }
    int32_t currentAlbumOrder = -1;
    err = ObtainCurrentAlbumOrder(currentAlbumId, currentAlbumOrder);
    if (err != E_OK) {
        return err;
    }
    vector<int32_t> changedAlbumIds;
    ObtainNotifyAlbumIds(currentAlbumOrder, maxAlbumOrder + 1, changedAlbumIds); // 1: move order curosr to the end
    err = UpdateNullReferenceOrder(currentAlbumId, currentAlbumOrder, maxAlbumOrder);
    if (err == E_OK) {
        NotifyOrderChange(changedAlbumIds);
    }
    return err;
}

static int32_t UpdatePortraitNullReferenceOrder(const int32_t currentAlbumId,
    const int32_t currentAlbumOrder, const int32_t maxAlbumOrder)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_DB_FAIL, "uniStore is nullptr! failed query album order");
    std::string updateOtherAlbumOrder = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " +
        RANK + " = " + RANK + " -1 WHERE " +
        RANK + " > " + to_string(currentAlbumOrder) + " and " +
        RANK + " <= " + to_string(maxAlbumOrder);
    std::string updateCurrentAlbumOrder = "UPDATE " + ANALYSIS_ALBUM_TABLE +
        " SET " + RANK + " = " + to_string(maxAlbumOrder) +
        " WHERE " + GROUP_TAG + " IN (SELECT " + GROUP_TAG + " FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE " + ALBUM_ID + " = " + to_string(currentAlbumId) + ")";
    vector<string> updateSortedAlbumsSqls = { updateOtherAlbumOrder, updateCurrentAlbumOrder };
    return ExecSqls(updateSortedAlbumsSqls, uniStore);
}

static int32_t ObtainNotifyPortraitAlbumIds(const int32_t currentAlbumOrder, const int32_t referenceAlbumOrder,
    vector<int32_t> &changedAlbumIds)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr! failed query album order");
        return E_DB_FAIL;
    }
    std::string queryAlbumIds = "";
    if (currentAlbumOrder < referenceAlbumOrder) {
        queryAlbumIds = "SELECT " + ALBUM_ID + " FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " + RANK + " >= " +
            to_string(currentAlbumOrder) + " AND " + RANK + " < " + to_string(referenceAlbumOrder);
    } else {
        queryAlbumIds = "SELECT " + ALBUM_ID + " FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " + RANK + " >= " +
            to_string(referenceAlbumOrder) + " AND " + RANK + " <= " + to_string(currentAlbumOrder);
    }
    auto resultSet = uniStore->QuerySql(queryAlbumIds);
    if (resultSet == nullptr) {
        return E_DB_FAIL;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        changedAlbumIds.push_back(GetInt32Val(ALBUM_ID, resultSet));
    }
    return E_OK;
}

static int32_t ObtainCurrentPortraitAlbumOrder(const int32_t albumId, int32_t &albumOrder)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr! failed query album order");
        return E_HAS_DB_ERROR;
    }
    const std::string queryAlbumOrder = "SELECT " + RANK + " FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " +
        PhotoAlbumColumns::ALBUM_ID + " = " + to_string(albumId);
    auto resultSet = uniStore->QuerySql(queryAlbumOrder);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return E_HAS_DB_ERROR;
    }
    return GetIntValueFromResultSet(resultSet, RANK, albumOrder);
}

static int32_t ObtainMaxPortraitAlbumOrder(int32_t &maxAlbumOrder)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr! failed query album order");
        return -E_HAS_DB_ERROR;
    }
    std::string queryMaxOrderSql = "SELECT Max(rank) FROM " + ANALYSIS_ALBUM_TABLE;
    auto resultSet = uniStore->QuerySql(queryMaxOrderSql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to query album!");
        return -E_HAS_DB_ERROR;
    }

    return resultSet->GetInt(0, maxAlbumOrder);
}

static int32_t HandlePortraitNullReferenceCondition(const int32_t currentAlbumId)
{
    int32_t maxAlbumOrder = 0;
    int err = ObtainMaxPortraitAlbumOrder(maxAlbumOrder);
    CHECK_AND_RETURN_RET(err == E_OK, E_HAS_DB_ERROR);
    int32_t currentAlbumOrder = -1;
    err = ObtainCurrentPortraitAlbumOrder(currentAlbumId, currentAlbumOrder);
    CHECK_AND_RETURN_RET(err == E_OK, err);

    vector<int32_t> changedAlbumIds;
     // move order curosr to the end
    ObtainNotifyPortraitAlbumIds(currentAlbumOrder, maxAlbumOrder + 1, changedAlbumIds);
    err = UpdatePortraitNullReferenceOrder(currentAlbumId, currentAlbumOrder, maxAlbumOrder);
    CHECK_AND_EXECUTE(err != E_OK, NotifyPortraitAlbum(changedAlbumIds));
    return err;
}

static int32_t ObtainPortraitAlbumOrders(const int32_t currentAlbumId, const int32_t referenceAlbumId,
    int32_t &currentAlbumOrder, int32_t &referenceAlbumOrder)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr! failed query album order");
        return E_HAS_DB_ERROR;
    }
    const std::string queryCurrentAlbumOrder = "SELECT " + RANK + " FROM " +
        ANALYSIS_ALBUM_TABLE + " WHERE " + ALBUM_ID + " = " + to_string(currentAlbumId);
    auto resultSet = uniStore->QuerySql(queryCurrentAlbumOrder);
    bool cond = (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK);
    CHECK_AND_RETURN_RET(!cond, E_HAS_DB_ERROR);
    if (GetIntValueFromResultSet(resultSet, RANK, currentAlbumOrder) != NativeRdb::E_OK) {
        return E_HAS_DB_ERROR;
    }

    const std::string queryReferenceAlbumOrder = "SELECT " + RANK + " FROM " +
        ANALYSIS_ALBUM_TABLE + " WHERE " + ALBUM_ID + " = " + to_string(referenceAlbumId);
    resultSet = uniStore->QuerySql(queryReferenceAlbumOrder);
    cond = (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK);
    CHECK_AND_RETURN_RET(!cond, E_HAS_DB_ERROR);
    if (GetIntValueFromResultSet(resultSet, RANK, referenceAlbumOrder) != NativeRdb::E_OK) {
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

static int32_t UpdatePortraitSortedOrder(const int32_t currentAlbumId, const int32_t referenceAlbumId,
    const int32_t currentAlbumOrder, const int32_t referenceAlbumOrder)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr! failed query album order");
        return E_DB_FAIL;
    }
    std::string updateOtherAlbumOrder = "";
    std::string updateCurrentAlbumOrder = "";
    if (currentAlbumOrder < referenceAlbumOrder) {
        updateOtherAlbumOrder = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + RANK + " = " + RANK + " -1 WHERE " +
            RANK + " > " + to_string(currentAlbumOrder) + " and " + RANK + " < " + to_string(referenceAlbumOrder);
        updateCurrentAlbumOrder = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + RANK + " = " +
            to_string(referenceAlbumOrder) + " -1 WHERE " + GROUP_TAG + " IN (SELECT " + GROUP_TAG + " FROM " +
            ANALYSIS_ALBUM_TABLE + " WHERE " + ALBUM_ID + " = " + to_string(currentAlbumId) + ")";
    } else {
        updateOtherAlbumOrder = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + RANK + " = " + RANK + " +1 WHERE " +
            RANK + " >= " + to_string(referenceAlbumOrder) + " AND " + RANK + " < " + to_string(currentAlbumOrder);
        updateCurrentAlbumOrder = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + RANK + " = " +
            to_string(referenceAlbumOrder) + " WHERE " + GROUP_TAG + " IN (SELECT " + GROUP_TAG + " FROM " +
            ANALYSIS_ALBUM_TABLE + " WHERE " + ALBUM_ID + " = " + to_string(currentAlbumId) + ")";
    }
    vector<string> updateSortedAlbumsSqls = { updateOtherAlbumOrder, updateCurrentAlbumOrder};
    return ExecSqls(updateSortedAlbumsSqls, uniStore);
}

bool CheckIsFavoritePortraitAlbum(const int32_t currentAlbumId, const int32_t referenceAlbumId)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr! failed query album order");
        return false;
    }
    std::string queryDisplayLevel = "SELECT " + USER_DISPLAY_LEVEL + " FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " +
        ALBUM_ID + " IN (" + to_string(currentAlbumId) + "," + to_string(referenceAlbumId) + ")";
    auto resultSet = uniStore->QuerySql(queryDisplayLevel);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query display level!");
        return false;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t displayLevel;
        if (GetIntValueFromResultSet(resultSet, USER_DISPLAY_LEVEL, displayLevel) != E_OK) {
            MEDIA_ERR_LOG("Get display level fail");
            return false;
        }
        if (displayLevel != FAVORITE_PAGE) {
            MEDIA_ERR_LOG("this album is not favorite portrait album");
            return false;
        }
    }
    return true;
}

int32_t OrderPortraitFavoriteAlbum(const int32_t currentAlbumId, const int32_t referenceAlbumId)
{
    if (!CheckIsFavoritePortraitAlbum(currentAlbumId, referenceAlbumId)) {
        return E_INVALID_VALUES;
    }
    if (referenceAlbumId == NULL_REFERENCE_ALBUM_ID) {
        return HandlePortraitNullReferenceCondition(currentAlbumId);
    }

    int32_t currentAlbumOrder = -1; // -1: default invalid value
    int32_t referenceAlbumOrder = -1; // -1: default invalid value
    int err = ObtainPortraitAlbumOrders(currentAlbumId, referenceAlbumId, currentAlbumOrder, referenceAlbumOrder);
    if (err != E_OK) {
        MEDIA_ERR_LOG("obtains album order error");
        return err;
    }
    vector<int32_t> changedAlbumIds;
    ObtainNotifyPortraitAlbumIds(currentAlbumOrder, referenceAlbumOrder, changedAlbumIds);
    err = UpdatePortraitSortedOrder(currentAlbumId, referenceAlbumId, currentAlbumOrder, referenceAlbumOrder);
    if (err == E_OK) {
        NotifyPortraitAlbum(changedAlbumIds);
    }
    return E_OK;
}

/**
 * Place the current album before the reference album
 * @param values contains current and reference album_id
 */
int32_t MediaLibraryAlbumOperations::OrderSingleAlbum(const ValuesBucket &values)
{
    int32_t currentAlbumId;
    int32_t referenceAlbumId;
    int err = GetIntVal(values, PhotoAlbumColumns::ALBUM_ID, currentAlbumId);
    if (err < 0 || currentAlbumId <= 0) {
        MEDIA_ERR_LOG("invalid album id");
        return E_INVALID_VALUES;
    }
    err = GetIntVal(values, PhotoAlbumColumns::REFERENCE_ALBUM_ID, referenceAlbumId);
    if (err < 0 || referenceAlbumId == 0 || referenceAlbumId < NULL_REFERENCE_ALBUM_ID) {
        MEDIA_ERR_LOG("invalid reference album id");
        return E_INVALID_VALUES;
    }
    if (currentAlbumId == referenceAlbumId) { // same album, no need to order
        return E_OK;
    }

    int32_t albumType;
    int32_t albumSubtype;
    err = GetIntVal(values, PhotoAlbumColumns::ALBUM_TYPE, albumType);
    int errorSubtype = GetIntVal(values, PhotoAlbumColumns::ALBUM_SUBTYPE, albumSubtype);
    bool cond = (err == E_OK && errorSubtype == E_OK &&
        (albumType == PhotoAlbumType::SMART && albumSubtype == PORTRAIT));
    CHECK_AND_RETURN_RET(!cond, OrderPortraitFavoriteAlbum(currentAlbumId, referenceAlbumId));
    if (referenceAlbumId == NULL_REFERENCE_ALBUM_ID) {
        return HandleNullReferenceCondition(currentAlbumId);
    }
    int32_t currentAlbumOrder = -1; // -1: default invalid value
    int32_t referenceAlbumOrder = -1;
    err = ObtainAlbumOrders(currentAlbumId, referenceAlbumId, currentAlbumOrder, referenceAlbumOrder);
    if (err != E_OK) {
        MEDIA_ERR_LOG("obtains album order error");
        return err;
    }
    vector<int32_t> changedAlbumIds;
    ObtainNotifyAlbumIds(currentAlbumOrder, referenceAlbumOrder, changedAlbumIds);
    AlbumAccurateRefresh albumRefresh(AccurateRefresh::ORDER_SINGLE_ALBUM_BUSSINESS_NAME);
    err = UpdateSortedOrder(albumRefresh, currentAlbumId, referenceAlbumId, currentAlbumOrder, referenceAlbumOrder);
    if (err == E_OK) {
        albumRefresh.Notify();
        NotifyOrderChange(changedAlbumIds);
    }
    return E_OK;
}

inline int32_t GetStringVal(const ValuesBucket &values, const string &key, string &value)
{
    value = "";
    ValueObject valueObject;
    if (values.GetObject(key, valueObject)) {
        valueObject.GetString(value);
    } else {
        return -EINVAL;
    }
    return E_OK;
}

int GetMergeAlbumCount(const int32_t currentAlbumId, const int32_t targetAlbumId)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr! failed query merge album info");
        return E_DB_FAIL;
    }

    string queryCount = "SELECT COUNT(DISTINCT file_id) FROM " + PhotoColumn::PHOTOS_TABLE + " p INNER JOIN " +
        ANALYSIS_PHOTO_MAP_TABLE + " apm ON p." + PhotoColumn::MEDIA_ID + " = apm." + MAP_ASSET + " INNER JOIN " +
        ANALYSIS_ALBUM_TABLE + " aa ON aa." + ALBUM_ID + " = apm." + MAP_ALBUM + " INNER JOIN (SELECT " + GROUP_TAG +
        " FROM "+ ANALYSIS_ALBUM_TABLE + " WHERE " + ALBUM_ID + " IN (" + to_string(currentAlbumId) + "," +
        to_string(targetAlbumId) + ")) ag ON ag." + GROUP_TAG + " = " + " aa." + GROUP_TAG + " WHERE " +
        PhotoColumn::MEDIA_DATE_TRASHED + " = 0 AND " + PhotoColumn::MEDIA_TIME_PENDING + " = 0 AND " +
        PhotoColumn::MEDIA_HIDDEN + " = 0";
    auto resultSet = uniStore->QuerySql(queryCount);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to query album!");
        return E_HAS_DB_ERROR;
    }
    int count;
    if (resultSet->GetInt(0, count) != E_OK) {
        return E_HAS_DB_ERROR;
    }
    return count;
}

string ParseFileIdFromCoverUri(const string &uri)
{
    if (PhotoColumn::PHOTO_URI_PREFIX.size() >= uri.size()) {
        return "";
    }
    string midStr = uri.substr(PhotoColumn::PHOTO_URI_PREFIX.size());
    string delimiter = "/";
    size_t pos = midStr.find(delimiter);
    if (pos == string::npos) {
        MEDIA_ERR_LOG("ParseFileIdFromCoverUri fail");
        return "";
    }
    return midStr.substr(0, pos);
}

static int32_t UpdateForReduceOneOrder(const int32_t referenceOrder)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr! failed query album order");
        return E_DB_FAIL;
    }
    std::string updateOtherAlbumOrder = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + RANK + " = " + RANK +
        " -1 WHERE " + RANK + " > " + to_string(referenceOrder);
    vector<string> updateSortedAlbumsSqls = { updateOtherAlbumOrder};
    return ExecSqls(updateSortedAlbumsSqls, uniStore);
}

int32_t UpdateForMergeAlbums(const MergeAlbumInfo &updateAlbumInfo, const int32_t currentAlbumId,
    const int32_t targetAlbumId)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr! failed update for merge albums");
        return E_DB_FAIL;
    }

    std::string updateForMergeAlbums = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + GROUP_TAG + " = " +
        updateAlbumInfo.groupTag + "," + COUNT + " = " + to_string(updateAlbumInfo.count) + "," + IS_ME + " = " +
        to_string(updateAlbumInfo.isMe) + "," + COVER_URI + " = '" + updateAlbumInfo.coverUri + "'," +
        USER_DISPLAY_LEVEL + " = " + to_string(updateAlbumInfo.userDisplayLevel) + "," + RANK + " = " +
        to_string(updateAlbumInfo.rank) + "," + USER_OPERATION + " = " + to_string(updateAlbumInfo.userOperation) +
        "," + RENAME_OPERATION + " = " + to_string(updateAlbumInfo.renameOperation) + "," + ALBUM_NAME + " = '" +
        updateAlbumInfo.albumName + "'," + IS_COVER_SATISFIED + " = " + to_string(updateAlbumInfo.isCoverSatisfied) +
        "," + ALBUM_RELATIONSHIP + " = '" + updateAlbumInfo.relationship + "'" +
        " WHERE " + GROUP_TAG + " IN(SELECT " + GROUP_TAG + " FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " + ALBUM_ID +
        " = " + to_string(currentAlbumId) + " OR " + ALBUM_ID + " = " + to_string(targetAlbumId) + ")";
    vector<string> updateSqls = { updateForMergeAlbums};
    return ExecSqls(updateSqls, uniStore);
}

int32_t GetMergeAlbumsInfo(vector<MergeAlbumInfo> &mergeAlbumInfo, const int32_t currentAlbumId,
    const int32_t targetAlbumId)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr! failed query merge album info");
        return E_DB_FAIL;
    }
    const std::string queryAlbumInfo = "SELECT " + ALBUM_ID + "," + GROUP_TAG + "," + COUNT + "," + IS_ME + "," +
        COVER_URI + "," + USER_DISPLAY_LEVEL + "," + RANK + "," + USER_OPERATION + "," + RENAME_OPERATION + "," +
        ALBUM_NAME + "," + IS_COVER_SATISFIED + "," + ALBUM_TYPE + "," + ALBUM_SUBTYPE + "," +
        ALBUM_RELATIONSHIP + " FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " + ALBUM_ID + " = " +
        to_string(currentAlbumId) + " OR " + ALBUM_ID + " = " + to_string(targetAlbumId);

    auto resultSet = uniStore->QuerySql(queryAlbumInfo);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Database query failed");
        return E_HAS_DB_ERROR;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        MergeAlbumInfo albumInfo;
        int isCoverSatisfied = 0;
        if (GetIntValueFromResultSet(resultSet, ALBUM_ID, albumInfo.albumId) != E_OK ||
            GetStringValueFromResultSet(resultSet, GROUP_TAG, albumInfo.groupTag) != E_OK ||
            GetIntValueFromResultSet(resultSet, COUNT, albumInfo.count) != E_OK ||
            GetIntValueFromResultSet(resultSet, IS_ME, albumInfo.isMe) != E_OK ||
            GetStringValueFromResultSet(resultSet, COVER_URI, albumInfo.coverUri) != E_OK ||
            GetIntValueFromResultSet(resultSet, USER_DISPLAY_LEVEL, albumInfo.userDisplayLevel) != E_OK ||
            GetIntValueFromResultSet(resultSet, RANK, albumInfo.rank) != E_OK ||
            GetIntValueFromResultSet(resultSet, USER_OPERATION, albumInfo.userOperation) != E_OK ||
            GetIntValueFromResultSet(resultSet, RENAME_OPERATION, albumInfo.renameOperation) != E_OK ||
            GetStringValueFromResultSet(resultSet, ALBUM_NAME, albumInfo.albumName) != E_OK ||
            GetIntValueFromResultSet(resultSet, IS_COVER_SATISFIED, isCoverSatisfied) != E_OK ||
            GetIntValueFromResultSet(resultSet, ALBUM_TYPE, albumInfo.albumType) != E_OK ||
            GetIntValueFromResultSet(resultSet, ALBUM_SUBTYPE, albumInfo.albumSubtype) != E_OK ||
            GetStringValueFromResultSet(resultSet, ALBUM_RELATIONSHIP, albumInfo.relationship) != E_OK) {
                MEDIA_ERR_LOG("Failed to get values from result set");
                return E_HAS_DB_ERROR;
            }
        albumInfo.isCoverSatisfied = static_cast<uint8_t>(isCoverSatisfied);
        mergeAlbumInfo.push_back(albumInfo);
    }
    return E_OK;
}

inline string JoinCandidateIds(const vector<string> &candidateIds)
{
    return accumulate(candidateIds.begin() + 1, candidateIds.end(), candidateIds[0],
        [](const string &a, const string &b) { return a + ", " + b; });
}

string GetCandidateIdsAndSetCoverSatisfied(MergeAlbumInfo &updateAlbumInfo, const MergeAlbumInfo &currentAlbum,
    const MergeAlbumInfo &targetAlbum, const string &currentFileId, const string &targetFileId)
{
    vector<string> candidateIds;
    if (currentAlbum.isCoverSatisfied & static_cast<uint8_t>(CoverSatisfiedType::USER_SETTING)) {
        candidateIds.push_back(currentFileId);
    }
    if (targetAlbum.isCoverSatisfied & static_cast<uint8_t>(CoverSatisfiedType::USER_SETTING)) {
        candidateIds.push_back(targetFileId);
    }
    if (!candidateIds.empty()) {
        updateAlbumInfo.isCoverSatisfied = static_cast<uint8_t>(CoverSatisfiedType::USER_SETTING_EDITE);
        return JoinCandidateIds(candidateIds);
    }

    if (currentAlbum.isCoverSatisfied & static_cast<uint8_t>(CoverSatisfiedType::BEAUTY_SETTING)) {
        candidateIds.push_back(currentFileId);
    }
    if (targetAlbum.isCoverSatisfied & static_cast<uint8_t>(CoverSatisfiedType::BEAUTY_SETTING)) {
        candidateIds.push_back(targetFileId);
    }
    if (!candidateIds.empty()) {
        updateAlbumInfo.isCoverSatisfied = static_cast<uint8_t>(CoverSatisfiedType::BEAUTY_SETTING_EDITE);
        return JoinCandidateIds(candidateIds);
    }

    if (currentAlbum.isCoverSatisfied & static_cast<uint8_t>(CoverSatisfiedType::DEFAULT_SETTING)) {
        candidateIds.push_back(currentFileId);
    }
    if (targetAlbum.isCoverSatisfied & static_cast<uint8_t>(CoverSatisfiedType::DEFAULT_SETTING)) {
        candidateIds.push_back(targetFileId);
    }
    if (!candidateIds.empty()) {
        updateAlbumInfo.isCoverSatisfied = static_cast<uint8_t>(CoverSatisfiedType::DEFAULT_SETTING);
        return JoinCandidateIds(candidateIds);
    }

    updateAlbumInfo.isCoverSatisfied = static_cast<uint8_t>(CoverSatisfiedType::DEFAULT_SETTING);
    return currentFileId + ", " + targetFileId;
}

int32_t GetMergeAlbumCoverUriAndSatisfied(MergeAlbumInfo &updateAlbumInfo, const MergeAlbumInfo &currentAlbum,
    const MergeAlbumInfo &targetAlbum)
{
    string currentFileId = ParseFileIdFromCoverUri(currentAlbum.coverUri);
    string targetFileId = ParseFileIdFromCoverUri(targetAlbum.coverUri);
    if (currentFileId.empty() || targetFileId.empty()) {
        return E_DB_FAIL;
    }
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr! failed query get merge album cover uri");
        return E_DB_FAIL;
    }
    string candidateIds =
        GetCandidateIdsAndSetCoverSatisfied(updateAlbumInfo, currentAlbum, targetAlbum, currentFileId, targetFileId);

    const std::string queryAlbumInfo = "SELECT " + MediaColumn::MEDIA_ID + "," + MediaColumn::MEDIA_TITLE + "," +
        MediaColumn::MEDIA_NAME + ", MAX(" + MediaColumn::MEDIA_DATE_ADDED + ") FROM " + PhotoColumn::PHOTOS_TABLE +
        " WHERE " + MediaColumn::MEDIA_ID + " IN (" + candidateIds + " )";

    auto resultSet = uniStore->QuerySql(queryAlbumInfo);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("resultSet is nullptr! failed query get merge album cover uri");
        return E_HAS_DB_ERROR;
    }
    int mergeFileId;
    if (GetIntValueFromResultSet(resultSet, MediaColumn::MEDIA_ID, mergeFileId) != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("resultSet is error! failed query get merge album cover uri");
        return E_HAS_DB_ERROR;
    }

    string mergeTitle;
    if (GetStringValueFromResultSet(resultSet, MediaColumn::MEDIA_TITLE, mergeTitle) != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("resultSet is error! failed query get merge album cover uri");
        return E_HAS_DB_ERROR;
    }

    string mergeDisplayName;
    if (GetStringValueFromResultSet(resultSet, MediaColumn::MEDIA_NAME, mergeDisplayName) != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("resultSet is error! failed query get merge album cover uri");
        return E_HAS_DB_ERROR;
    }
    updateAlbumInfo.coverUri = "file://media/Photo/" + to_string(mergeFileId) + "/" + mergeTitle + "/" +
        mergeDisplayName;
    return E_OK;
}

static string MergeAlbumInfoToString(const MergeAlbumInfo& mergeAlbumInfo)
{
    string info = "album " + to_string(mergeAlbumInfo.albumId) + ", groupTag " + mergeAlbumInfo.groupTag + ", count " +
        to_string(mergeAlbumInfo.count) + ", isMe " + to_string(mergeAlbumInfo.isMe) + ", coverUri " +
        DfxUtils::GetSafeUri(mergeAlbumInfo.coverUri) + ", userDisplayLevel " +
        to_string(mergeAlbumInfo.userDisplayLevel) + ", rank " +
        to_string(mergeAlbumInfo.rank) + ", userOperation " + to_string(mergeAlbumInfo.userOperation) +
        ", renameOperation " + to_string(mergeAlbumInfo.renameOperation) + ", albumName " +
        DfxUtils::GetSafeAlbumName(mergeAlbumInfo.albumName) +
        ", isCoverSatisfied " + to_string(mergeAlbumInfo.isCoverSatisfied);
    return info;
}

static int32_t GetMergedAlbumInfo(const vector<MergeAlbumInfo> &mergeAlbumInfo,
    MergeAlbumInfo &updateAlbumInfo)
{
    DataShare::DataSharePredicates predicatesPortrait;
    DataShare::DataSharePredicates predicates;
    int albumId = -1;
    std::vector<int32_t> PAGE_POSITION = { FAVORITE_PAGE, FIRST_PAGE, SECOND_PAGE };
    for (int32_t page : PAGE_POSITION) {
        GetDisplayLevelAlbumPredicates(page, predicates);
        std::string whereClause = predicates.GetWhereClause();
        CHECK_AND_CONTINUE(!whereClause.empty());
        whereClause = "(" + ALBUM_ID + " = " + std::to_string(mergeAlbumInfo[0].albumId) + " OR "
            + ALBUM_ID + " = " + std::to_string(mergeAlbumInfo[1].albumId) + ") AND " + whereClause + " Limit 1";
        predicatesPortrait.SetWhereClause(whereClause);
        CHECK_AND_CONTINUE(!whereClause.empty());
        auto rdbPredicates = RdbUtils::ToPredicates(predicatesPortrait, ANALYSIS_ALBUM_TABLE);
        vector<string> columns = { ALBUM_ID };
        auto resultSet = MediaLibraryRdbStore::QueryWithFilter(rdbPredicates, columns);
        CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "resultSet is nullptr");
        if (resultSet->GoToFirstRow() == NativeRdb::E_OK &&
            GetIntValueFromResultSet(resultSet, ALBUM_ID, albumId) == E_OK) {
            MEDIA_INFO_LOG("current page: %{public}d, select albumId: %{public}d", page, albumId);
            resultSet->Close();
            break;
        }
        resultSet->Close();
    }
    CHECK_AND_RETURN_RET_LOG(albumId != -1, E_HAS_DB_ERROR, "selectAlbumId invalid");
    updateAlbumInfo.albumName =
        mergeAlbumInfo[0].albumId == albumId ? mergeAlbumInfo[0].albumName : mergeAlbumInfo[1].albumName;
    updateAlbumInfo.relationship =
        mergeAlbumInfo[0].albumId == albumId ? mergeAlbumInfo[0].relationship : mergeAlbumInfo[1].relationship;
    updateAlbumInfo.userDisplayLevel =
        mergeAlbumInfo[0].albumId == albumId ? mergeAlbumInfo[0].userDisplayLevel : mergeAlbumInfo[1].userDisplayLevel;
    if (updateAlbumInfo.albumName == "") {
        updateAlbumInfo.albumName =
            mergeAlbumInfo[0].albumId == albumId ? mergeAlbumInfo[1].albumName : mergeAlbumInfo[0].albumName;
    }
    if (updateAlbumInfo.relationship == "") {
        updateAlbumInfo.relationship =
            mergeAlbumInfo[0].albumId == albumId ? mergeAlbumInfo[1].relationship : mergeAlbumInfo[0].relationship;
    }
    return E_OK;
}

static int32_t UpdateMergeAlbumsInfo(const vector<MergeAlbumInfo> &mergeAlbumInfo, int32_t currentAlbumId)
{
    MergeAlbumInfo updateAlbumInfo;
    if (GetMergeAlbumCoverUriAndSatisfied(updateAlbumInfo, mergeAlbumInfo[0], mergeAlbumInfo[1]) != E_OK) {
        return E_HAS_DB_ERROR;
    }
    updateAlbumInfo.count = GetMergeAlbumCount(mergeAlbumInfo[0].albumId, mergeAlbumInfo[1].albumId);
    updateAlbumInfo.groupTag = "'" + mergeAlbumInfo[0].groupTag + "|" + mergeAlbumInfo[1].groupTag + "'";
    updateAlbumInfo.isMe = (mergeAlbumInfo[0].isMe == 1 || mergeAlbumInfo[1].isMe == 1) ? 1 : 0;
    updateAlbumInfo.userOperation = 1;
    CHECK_AND_RETURN_RET(GetMergedAlbumInfo(mergeAlbumInfo, updateAlbumInfo) == E_OK, E_HAS_DB_ERROR);
    updateAlbumInfo.renameOperation =
        (mergeAlbumInfo[0].albumName != "" || mergeAlbumInfo[1].albumName != "") ? 1 : 0;
    CHECK_AND_EXECUTE(updateAlbumInfo.userDisplayLevel == FAVORITE_PAGE, updateAlbumInfo.rank = 0);
    if (updateAlbumInfo.userDisplayLevel == FAVORITE_PAGE) {
        if (mergeAlbumInfo[0].userDisplayLevel == mergeAlbumInfo[1].userDisplayLevel) {
            updateAlbumInfo.rank = min(mergeAlbumInfo[0].rank, mergeAlbumInfo[1].rank);
            CHECK_AND_RETURN_RET(UpdateForReduceOneOrder(max(mergeAlbumInfo[0].rank, mergeAlbumInfo[1].rank)) == E_OK,
                E_HAS_DB_ERROR);
        } else {
            updateAlbumInfo.rank = max(mergeAlbumInfo[0].rank, mergeAlbumInfo[1].rank);
        }
    }
    MEDIA_INFO_LOG("After merge: %{public}s", MergeAlbumInfoToString(updateAlbumInfo).c_str());
    return UpdateForMergeAlbums(updateAlbumInfo, mergeAlbumInfo[0].albumId, mergeAlbumInfo[1].albumId);
}

bool MediaLibraryAlbumOperations::IsOnlyPortraitOrPetAlbumMerge(const vector<MergeAlbumInfo>& mergeAlbumInfo)
{
    if (mergeAlbumInfo.size() != MERGE_ALBUM_COUNT) { // merge album count
        MEDIA_ERR_LOG("invalid mergeAlbumInfo size");
        return false;
    }
    if (((mergeAlbumInfo[0].albumType == PhotoAlbumType::SMART &&
          mergeAlbumInfo[0].albumSubtype == PhotoAlbumSubType::PORTRAIT) &&
         (mergeAlbumInfo[1].albumType == PhotoAlbumType::SMART &&
          mergeAlbumInfo[1].albumSubtype == PhotoAlbumSubType::PORTRAIT)) ||
        ((mergeAlbumInfo[0].albumType == PhotoAlbumType::SMART &&
          mergeAlbumInfo[0].albumSubtype == PhotoAlbumSubType::PET) &&
         (mergeAlbumInfo[1].albumType == PhotoAlbumType::SMART &&
          mergeAlbumInfo[1].albumSubtype == PhotoAlbumSubType::PET))) {
        return true;
    }
    return false;
}

int32_t MediaLibraryAlbumOperations::MergeAlbums(const NativeRdb::ValuesBucket &values)
{
    int32_t currentAlbumId;
    int32_t targetAlbumId;
    int err = GetIntVal(values, ALBUM_ID, currentAlbumId);
    if (err < 0 || currentAlbumId <= 0) {
        MEDIA_ERR_LOG("invalid album id");
        return E_INVALID_VALUES;
    }
    err = GetIntVal(values, TARGET_ALBUM_ID, targetAlbumId);
    if (err < 0 || targetAlbumId <= 0) {
        MEDIA_ERR_LOG("invalid target album id");
        return E_INVALID_VALUES;
    }
    if (currentAlbumId == targetAlbumId) { // same album, no need to merge
        MEDIA_WARN_LOG("Ignore invalid portrait album merge request, "
            "current album id and target album id are the same");
        return E_OK;
    }
    MEDIA_INFO_LOG("Start merge portrait albums, current album id: %{public}d, target album id: %{public}d",
        currentAlbumId, targetAlbumId);
    vector<MergeAlbumInfo> mergeAlbumInfo;
    if (GetMergeAlbumsInfo(mergeAlbumInfo, currentAlbumId, targetAlbumId)) {
        MEDIA_ERR_LOG("Get merge albums info fail");
        return E_HAS_DB_ERROR;
    }
    if (mergeAlbumInfo.size() != MERGE_ALBUM_COUNT) { // merge album count
        MEDIA_ERR_LOG("invalid mergeAlbumInfo size");
        return E_INVALID_VALUES;
    }
    if (!IsOnlyPortraitOrPetAlbumMerge(mergeAlbumInfo)) {
        MEDIA_ERR_LOG("Only portrait or only pet album can merge");
        return E_INVALID_VALUES;
    }
    MEDIA_INFO_LOG("Before merge: %{public}s", MergeAlbumInfoToString(mergeAlbumInfo[0]).c_str());
    MEDIA_INFO_LOG("Before merge: %{public}s", MergeAlbumInfoToString(mergeAlbumInfo[1]).c_str());
    err = UpdateMergeAlbumsInfo(mergeAlbumInfo, currentAlbumId);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "MergeAlbum failed, err %{public}d", err);
    err = MediaLibraryAnalysisAlbumOperations::UpdateMergeGroupAlbumsInfo(mergeAlbumInfo);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "MergeGroupAlbum failed, err %{public}d", err);
    vector<int32_t> changeAlbumIds = { currentAlbumId };
    NotifyPortraitAlbum(changeAlbumIds);
    return err;
}

static int32_t UpdateDisplayLevel(const int32_t value, const int32_t albumId)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr! failed query album order");
        return E_DB_FAIL;
    }
    vector<string> updateDisplayLevelAlbumsSqls = {};
    if (value == FIRST_PAGE) {
        std::string setAnalysisStatusSqls = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + ANALYSIS_STATUS + " = " +
            std::to_string(ANALYSIS_STATUS_TO_REFRESH) + " WHERE " + ALBUM_ID + " IN (SELECT " + ALBUM_ID + " FROM " +
            ANALYSIS_ALBUM_TABLE + " WHERE " + GROUP_TAG + " LIKE ( SELECT CONCAT('%', " + GROUP_TAG + " , '%') FROM " +
            ANALYSIS_ALBUM_TABLE + " WHERE " + ALBUM_ID + " = " + to_string(albumId) + " AND (" + USER_DISPLAY_LEVEL +
            " = " + to_string(NO_FIRST_PAGE_SINGLE_PORTRAIT_MANUAL) + " OR " + USER_DISPLAY_LEVEL + " = " +
            std::to_string(NO_FIRST_PAGE_SINGLE_PORTRAIT) + " OR " + USER_DISPLAY_LEVEL + " IS NULL))) AND " +
            PhotoAlbumColumns::ALBUM_SUBTYPE + " = " + to_string(PhotoAlbumSubType::GROUP_PHOTO);
        updateDisplayLevelAlbumsSqls.push_back(setAnalysisStatusSqls);
    }
    std::string updateDisplayLevel = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + USER_DISPLAY_LEVEL + " = " +
        to_string(value) + " WHERE " + GROUP_TAG + " IN (SELECT " + GROUP_TAG + " FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE " + ALBUM_ID + " = " + to_string(albumId) + ")";
    updateDisplayLevelAlbumsSqls.push_back(updateDisplayLevel);
    return ExecSqls(updateDisplayLevelAlbumsSqls, uniStore);
}

static int32_t UpdateFavoritesOrder(const int32_t value, const int32_t currentAlbumId)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr! failed query album order");
        return E_DB_FAIL;
    }
    std::string updateOtherAlbumOrder;
    std::string updateCurrentAlbumOrder;
    vector<string> updateSortedAlbumsSqls;
    if (value == FAVORITE_PAGE) {
        int maxAlbumOrder;
        ObtainMaxPortraitAlbumOrder(maxAlbumOrder);
        updateCurrentAlbumOrder = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + RANK + " = " + to_string(maxAlbumOrder) +
            " +1 WHERE " + GROUP_TAG + " IN (SELECT " + GROUP_TAG + " FROM " + ANALYSIS_ALBUM_TABLE +
            " WHERE " + ALBUM_ID + " = " + to_string(currentAlbumId) + ")";
        updateSortedAlbumsSqls.push_back(updateCurrentAlbumOrder);
    } else {
        int rank;
        int err = ObtainCurrentPortraitAlbumOrder(currentAlbumId, rank);
        if (err != E_OK) {
            return err;
        }
        updateOtherAlbumOrder = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + RANK + " = " + RANK + " -1 WHERE " +
            USER_DISPLAY_LEVEL + " = " + to_string(FAVORITE_PAGE) + " AND " + RANK + ">" + to_string(rank);
        updateCurrentAlbumOrder = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + RANK + " = 0" +
            " WHERE " + GROUP_TAG + " IN (SELECT " + GROUP_TAG + " FROM " + ANALYSIS_ALBUM_TABLE +
            " WHERE " + ALBUM_ID + " = " + to_string(currentAlbumId) + ")";
        updateSortedAlbumsSqls.push_back(updateOtherAlbumOrder);
        updateSortedAlbumsSqls.push_back(updateCurrentAlbumOrder);
    }
    return ExecSqls(updateSortedAlbumsSqls, uniStore);
}

static int32_t UpdateFavorites(int32_t value, const int32_t albumId)
{
    int err = UpdateFavoritesOrder(value, albumId);
    if (err != E_OK) {
        MEDIA_ERR_LOG("UpdateFavoritesOrder fail");
        return E_DB_FAIL;
    }
    if (value == UNFAVORITE_PAGE) {
        value = FIRST_PAGE;
    }
    return UpdateDisplayLevel(value, albumId);
}

int32_t MediaLibraryAlbumOperations::SetDisplayLevel(const ValuesBucket &values, const DataSharePredicates &predicates)
{
    int32_t displayLevelValue;
    int err = GetIntVal(values, USER_DISPLAY_LEVEL, displayLevelValue);
    if (err < 0 || !MediaFileUtils::CheckDisplayLevel(displayLevelValue)) {
        MEDIA_ERR_LOG("invalid display level");
        return E_INVALID_VALUES;
    }

    RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, ANALYSIS_ALBUM_TABLE);
    auto whereArgs = rdbPredicates.GetWhereArgs();
    if (whereArgs.size() == 0) {
        MEDIA_ERR_LOG("no target album id");
        return E_INVALID_VALUES;
    }
    int32_t albumId = atoi(whereArgs[0].c_str());
    if (albumId <= 0) {
        MEDIA_ERR_LOG("invalid album id");
        return E_INVALID_VALUES;
    }
    MEDIA_INFO_LOG("Start set display level, album id: %{public}d, display level: %{public}d",
        albumId, displayLevelValue);
    vector<int32_t> changedAlbumIds;
    if (displayLevelValue == FIRST_PAGE || displayLevelValue == SECOND_PAGE) {
        err = UpdateDisplayLevel(displayLevelValue, albumId);
        changedAlbumIds.push_back(albumId);
    } else {
        err = UpdateFavorites(displayLevelValue, albumId);
        changedAlbumIds.push_back(albumId);
    }
    if (err == E_OK) {
        if (displayLevelValue == SECOND_PAGE) {
            std::vector<std::string> albumList = { to_string(albumId) };
            MediaAnalysisHelper::PortraitDisplayGraphChange(
                IMediaAnalysisService::ActivateServiceType::PORTRAIT_DISPLAY_CHANGE, albumList);
        }
        NotifyPortraitAlbum(changedAlbumIds);
    }
    return err;
}

void QueryIsMeAndRelationshipAlbumIds(vector<string> &albumIds, shared_ptr<MediaLibraryRdbStore> uniStore)
{
    std::string queryAlbumIds = "SELECT DISTINCT " + ALBUM_ID + " FROM " + ANALYSIS_ALBUM_TABLE +
                            " WHERE (is_me = 1 or ( relationship is not null and relationship != '' )) "
                            " AND album_subtype = " + to_string(PhotoAlbumSubType::PORTRAIT);
    auto resultSet = uniStore->QuerySql(queryAlbumIds);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Failed to query isMe and relationship albumIds!");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int albumId = 0;
        if (GetIntValueFromResultSet(resultSet, ALBUM_ID, albumId) == E_OK) {
            albumIds.push_back(to_string(albumId));
        }
    }
    resultSet->Close();
}

void QueryAlbumIdsByPredicate(vector<string> &albumIds, RdbPredicates rdbPredicates,
    shared_ptr<MediaLibraryRdbStore> uniStore)
{
    std::string queryAlbumIds = "SELECT DISTINCT " + ALBUM_ID + " FROM " + ANALYSIS_ALBUM_TABLE +
                                " WHERE GROUP_TAG IN (SELECT " + GROUP_TAG + " FROM " + ANALYSIS_ALBUM_TABLE +
                                " WHERE " + rdbPredicates.GetWhereClause() + ")"
                                " AND album_subtype = " + to_string(PhotoAlbumSubType::PORTRAIT);
    auto bindArgs = rdbPredicates.GetBindArgs();
    auto resultSet = uniStore->QuerySql(queryAlbumIds, bindArgs);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Failed to query albumIds by predicates!");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int albumId = 0;
        if (GetIntValueFromResultSet(resultSet, ALBUM_ID, albumId) == E_OK) {
            albumIds.push_back(to_string(albumId));
        }
    }
    resultSet->Close();
}

static void DoUpdateAlbumSearchIndex(const std::vector<string> &albumIds)
{
    if (albumIds.empty()) {
        MEDIA_ERR_LOG("albumIds is empty");
        return;
    }
    MediaAnalysisHelper::AsyncStartMediaAnalysisService(
        static_cast<int32_t>(MediaAnalysisProxy::ActivateServiceType::ALBUM_UPDATE_INDEX_TASK), albumIds);
}

void SetMyOldAlbum(vector<string>& updateSqls, shared_ptr<MediaLibraryRdbStore> uniStore)
{
    std::string queryIsMe = "SELECT COUNT(DISTINCT album_id)," + ALBUM_NAME +
        " FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " + IS_ME + " = 1 ";
    auto resultSet = uniStore->QuerySql(queryIsMe);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to query isMe!");
        return;
    }
    int count;
    if (resultSet->GetInt(0, count) != E_OK) {
        return;
    }
    std::string clearIsMeAlbum = "";
    if (count > 0) {
        string albumName = "";
        GetStringValueFromResultSet(resultSet, ALBUM_NAME, albumName);
        int renameOperation = albumName != "" ? 1 : 0;
        clearIsMeAlbum= "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + IS_ME + " = 0, " +
            ALBUM_RELATIONSHIP + " = '', " + RENAME_OPERATION +
            " = " + to_string(renameOperation) + " WHERE " + IS_ME + " = 1";
        std::string updateSqlForGroupPhoto = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + RENAME_OPERATION + " = " +
            to_string(ALBUM_TO_RENAME_FOR_ANALYSIS) + " WHERE " + ALBUM_ID + " IN (SELECT " + ALBUM_ID + " FROM " +
            ANALYSIS_ALBUM_TABLE + " WHERE " + GROUP_TAG + " LIKE ( SELECT CONCAT('%', " + GROUP_TAG + ", '%') FROM " +
            ANALYSIS_ALBUM_TABLE + " WHERE " + IS_ME + " =  1 AND " + PhotoAlbumColumns::ALBUM_SUBTYPE + " = " +
            to_string(PhotoAlbumSubType::PORTRAIT) +  " LIMIT 1) AND " +PhotoAlbumColumns::ALBUM_SUBTYPE + " = " +
            to_string(PhotoAlbumSubType::GROUP_PHOTO) + " AND (" + RENAME_OPERATION + " != " + to_string(ALBUM_RENAMED)
            + " OR " + RENAME_OPERATION + " IS NULL))";
        updateSqls.push_back(updateSqlForGroupPhoto);
        updateSqls.push_back(clearIsMeAlbum);
    }
}

static void GetHighlightCoverStatus(const string &albumId, int32_t &currentCoverStatus)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is nullptr! failed update index");
    const std::string queryCoverStatus =
        "SELECT status FROM " + HIGHLIGHT_COVER_INFO_TABLE + " WHERE album_id = "
        "(SELECT id FROM tab_highlight_album WHERE album_id = " + albumId + " LIMIT 1)";
    shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(queryCoverStatus);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "resultSet is nullptr! failed update index");
    CHECK_AND_EXECUTE(resultSet->GoToNextRow() != NativeRdb::E_OK,
        currentCoverStatus = GetInt32Val(COVER_STATUS, resultSet));
}

int32_t MediaLibraryAlbumOperations::SetHighlightCoverUri(const ValuesBucket &values,
    const DataSharePredicates &predicates)
{
    MEDIA_INFO_LOG("Start set highlight cover uri");
    RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, ANALYSIS_ALBUM_TABLE);
    auto whereArgs = rdbPredicates.GetWhereArgs();
    CHECK_AND_RETURN_RET_LOG(whereArgs.size() != 0, E_INVALID_VALUES, "no target album id");

    string targetAlbumId = whereArgs[0];
    CHECK_AND_RETURN_RET_LOG(!targetAlbumId.empty() && MediaLibraryDataManagerUtils::IsNumber(targetAlbumId),
        E_INVALID_VALUES, "target album id not exists");
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_DB_FAIL,
        "uniStore is nullptr! failed update for set highlight cover uri");

    string coverUri;
    int err = GetStringVal(values, COVER_URI, coverUri);
    bool cond = (err < 0 || coverUri.empty());
    CHECK_AND_RETURN_RET_LOG(!cond, E_INVALID_VALUES, "invalid album cover uri");

    int32_t currentCoverStatus = 0;
    GetHighlightCoverStatus(targetAlbumId, currentCoverStatus);
    int32_t newCoverStatus = static_cast<uint32_t>(currentCoverStatus) | HIGHLIGHT_COVER_STATUS_COVER;
    std::string updateAnalysisAlbum = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + COVER_URI + " = '" + coverUri +
        "', " + IS_COVER_SATISFIED + " = 2 WHERE " + ALBUM_ID + " = " + targetAlbumId;
    std::string updateCoverInfoTable = "UPDATE " + HIGHLIGHT_COVER_INFO_TABLE +
        " SET " + COVER_STATUS + " = " + to_string(newCoverStatus) +
        " WHERE " + ALBUM_ID + " = (SELECT id FROM tab_highlight_album WHERE album_id = " + targetAlbumId + " LIMIT 1)";
    vector<string> updateSqls = { updateAnalysisAlbum, updateCoverInfoTable };
    err = ExecSqls(updateSqls, uniStore);
    if (err == E_OK) {
        vector<int32_t> changeAlbumIds = { atoi(targetAlbumId.c_str()) };
        NotifyHighlightAlbum(changeAlbumIds);
    }
    return err;
}

int32_t MediaLibraryAlbumOperations::SetHighlightAlbumName(const ValuesBucket &values,
    const DataSharePredicates &predicates)
{
    MEDIA_INFO_LOG("Start set highlight album name");
    RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, ANALYSIS_ALBUM_TABLE);
    auto whereArgs = rdbPredicates.GetWhereArgs();
    CHECK_AND_RETURN_RET_LOG(whereArgs.size() != 0, E_INVALID_VALUES, "no target highlight album id");

    string highlightAlbumId = whereArgs[0];
    CHECK_AND_RETURN_RET_LOG(!highlightAlbumId.empty() && MediaLibraryDataManagerUtils::IsNumber(highlightAlbumId),
        E_INVALID_VALUES, "highlight album id not exists");
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_DB_FAIL,
        "uniStore is nullptr! failed update for set highlight album name");

    string albumName;
    int err = GetStringVal(values, ALBUM_NAME, albumName);
    bool cond = (err < 0 || albumName.empty());
    CHECK_AND_RETURN_RET_LOG(!cond, E_INVALID_VALUES, "invalid album name");

    int32_t currentCoverStatus = 0;
    GetHighlightCoverStatus(highlightAlbumId, currentCoverStatus);
    int32_t newCoverStatus = static_cast<uint32_t>(currentCoverStatus) | HIGHLIGHT_COVER_STATUS_TITLE;
    std::string updateAlbumName = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + ALBUM_NAME + " = '" + albumName +
        "' WHERE " + ALBUM_ID + " = " + highlightAlbumId;
    std::string updateCoverInfoTable = "UPDATE " + HIGHLIGHT_COVER_INFO_TABLE + " SET " +
        COVER_STATUS + " = " + to_string(newCoverStatus) + " WHERE " +
        ALBUM_ID + " = (SELECT id FROM tab_highlight_album WHERE album_id = " + highlightAlbumId + " LIMIT 1)";
    vector<string> updateSqls = { updateAlbumName, updateCoverInfoTable };
    err = ExecSqls(updateSqls, uniStore);
    if (err == E_OK) {
        vector<int32_t> changeAlbumIds = { atoi(highlightAlbumId.c_str()) };
        NotifyHighlightAlbum(changeAlbumIds);
    }
    return err;
}

int32_t MediaLibraryAlbumOperations::SetHighlightSubtitle(const ValuesBucket &values,
    const DataSharePredicates &predicates)
{
    MEDIA_INFO_LOG("Start set highlight subtitle");
    RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, ANALYSIS_ALBUM_TABLE);
    auto whereArgs = rdbPredicates.GetWhereArgs();
    CHECK_AND_RETURN_RET_LOG(whereArgs.size() != 0, E_INVALID_VALUES, "no target highlight album id");

    string highlightAlbumId = whereArgs[0];
    CHECK_AND_RETURN_RET_LOG(!highlightAlbumId.empty() && MediaLibraryDataManagerUtils::IsNumber(highlightAlbumId),
        E_INVALID_VALUES, "highlight album id not exists");
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_DB_FAIL,
        "uniStore is nullptr! failed update for set highlight album name");

    string albumSubtitle;
    int err = GetStringVal(values, SUB_TITLE, albumSubtitle);
    CHECK_AND_RETURN_RET_LOG(err >= 0, E_INVALID_VALUES, "invalid album name");

    MEDIA_INFO_LOG("New highlight subtitle is: %{public}s, album id is %{public}s",
        albumSubtitle.c_str(), highlightAlbumId.c_str());
    int32_t currentCoverStatus = 0;
    GetHighlightCoverStatus(highlightAlbumId, currentCoverStatus);
    int32_t newCoverStatus = static_cast<uint32_t>(currentCoverStatus) | HIGHLIGHT_COVER_STATUS_TITLE;
    std::string updateAlbumName = "UPDATE " + HIGHLIGHT_ALBUM_TABLE + " SET " + SUB_TITLE + " = '" + albumSubtitle +
        "', " + HIGHLIGHT_USE_SUBTITLE + " = 1" + " WHERE " + ALBUM_ID + " = " + highlightAlbumId;
    std::string updateCoverInfoTable = "UPDATE " + HIGHLIGHT_COVER_INFO_TABLE + " SET " +
        COVER_STATUS + " = " + to_string(newCoverStatus) + " WHERE " +
        ALBUM_ID + " = (SELECT id FROM tab_highlight_album WHERE album_id = " + highlightAlbumId + " LIMIT 1)";
    vector<string> updateSqls = { updateAlbumName, updateCoverInfoTable };
    err = ExecSqls(updateSqls, uniStore);
    if (err == E_OK) {
        vector<int32_t> changeAlbumIds = { atoi(highlightAlbumId.c_str()) };
        NotifyHighlightAlbum(changeAlbumIds);
    }
    return err;
}

int32_t SetAnalysisAlbumRelationship(const ValuesBucket &values, const DataSharePredicates &predicates)
{
    string relationship;
    int err = GetStringVal(values, ALBUM_RELATIONSHIP, relationship);
    CHECK_AND_RETURN_RET_LOG(err >= 0, E_INVALID_VALUES, "Invalid relationship");

    stringstream sql;
    sql << "UPDATE " << ANALYSIS_ALBUM_TABLE << " SET " << ALBUM_RELATIONSHIP;
    if (relationship.empty()) {
        sql << " = NULL";
    } else {
        sql << " = \"" << relationship << "\"";
    }
    RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, ANALYSIS_ALBUM_TABLE);
    sql << " WHERE " << rdbPredicates.GetWhereClause();
    string sqlStr = sql.str();

    auto bindArgs = rdbPredicates.GetBindArgs();

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "Failed to get rdbStore.");
    vector<string> needUpdateSearchAlbumIds;
    QueryAlbumIdsByPredicate(needUpdateSearchAlbumIds, rdbPredicates, rdbStore);
    int ret = rdbStore->ExecuteSql(sqlStr, bindArgs);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret, "Update relationship failed, error id: %{public}d", ret);
    DoUpdateAlbumSearchIndex(needUpdateSearchAlbumIds);
    return ret;
}

int32_t MediaLibraryAlbumOperations::SetPortraitAlbumRelationship(const ValuesBucket &values,
    const DataSharePredicates &predicates, const int32_t isMeAlbum)
{
    MEDIA_INFO_LOG("SetPortraitAlbumRelationship start");
    string relationship;
    int err = GetStringVal(values, ALBUM_RELATIONSHIP, relationship);
    CHECK_AND_RETURN_RET_LOG(err >= 0, E_INVALID_VALUES, "Invalid relationship");
    if (relationship == ME_RELATIONSHIP) {
        MEDIA_INFO_LOG("SetPortraitAlbumRelationship is me");
        return MediaLibraryAlbumOperations::SetIsMe(values, predicates);
    }

    vector<string> needUpdateSearchAlbumIds;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "Failed to get rdbStore.");
    // 如果当前相册是“我”，设置为其它相册，则清理is_me等数据
    if (isMeAlbum == IS_ME_ALBUM) {
        QueryIsMeAndRelationshipAlbumIds(needUpdateSearchAlbumIds, rdbStore);
        vector<string> updateSqls;
        SetMyOldAlbum(updateSqls, rdbStore);
        int32_t err = ExecSqls(updateSqls, rdbStore);
        CHECK_AND_RETURN_RET_LOG(err == NativeRdb::E_OK, err, "SetMyOldAlbum failed, error id: %{public}d", err);
    }

    stringstream sql;
    sql << "UPDATE " << ANALYSIS_ALBUM_TABLE << " SET " << ALBUM_RELATIONSHIP;
    if (relationship.empty()) {
        sql << " = NULL";
    } else {
        sql << " = \"" << relationship << "\"";
    }
    RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, ANALYSIS_ALBUM_TABLE);
    sql << " WHERE " << GROUP_TAG + " IN(SELECT " +
        GROUP_TAG + " FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " + rdbPredicates.GetWhereClause() + ")";
    string sqlStr = sql.str();
    auto bindArgs = rdbPredicates.GetBindArgs();
    QueryAlbumIdsByPredicate(needUpdateSearchAlbumIds, rdbPredicates, rdbStore);
    int ret = rdbStore->ExecuteSql(sqlStr, bindArgs);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret, "Update relationship failed, error id: %{public}d", ret);
    DoUpdateAlbumSearchIndex(needUpdateSearchAlbumIds);

    auto whereArgs = rdbPredicates.GetWhereArgs();
    if (whereArgs.empty()) {
        return E_INVALID_VALUES;
    }
    string targetAlbumId = whereArgs[0];
    vector<int32_t> changeAlbumIds = { atoi(targetAlbumId.c_str()) };
    NotifyPortraitAlbum(changeAlbumIds);
    return ret;
}

static int32_t GetIsMeAlbumDisplayLevelClause(
    shared_ptr<MediaLibraryRdbStore> uniStore, std::string targetAlbumId, std::string &displayLevelClause)
{
    std::string queryTargetAlbum =
        "SELECT " + USER_DISPLAY_LEVEL + " FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " + ALBUM_ID + " = " + targetAlbumId;
    auto targetResultSet = uniStore->QuerySql(queryTargetAlbum);
    if (targetResultSet == nullptr || targetResultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return -E_HAS_DB_ERROR;
    }
    int32_t userDisplaylevel = -1;
    GetIntValueFromResultSet(targetResultSet, USER_DISPLAY_LEVEL, userDisplaylevel);
    targetResultSet->Close();

    if (userDisplaylevel == ANALYSISED_DISPLAY_LEVEL_HIDDEN ||
        userDisplaylevel == ANALYSISED_DISPLAY_LEVEL_SECOND_PAGE) {
        MEDIA_INFO_LOG("Album %{public}s has display level: %{public}d", targetAlbumId.c_str(), userDisplaylevel);
        displayLevelClause = ", " + USER_DISPLAY_LEVEL + " = " + std::to_string(FIRST_PAGE);
    }
    return E_OK;
}

/**
 * set target album is me
 * @param values is_me
 * @param predicates target album
 */
int32_t MediaLibraryAlbumOperations::SetIsMe(const ValuesBucket &values, const DataSharePredicates &predicates)
{
    RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, ANALYSIS_ALBUM_TABLE);
    auto whereArgs = rdbPredicates.GetWhereArgs();
    if (whereArgs.size() == 0) {
        MEDIA_ERR_LOG("no target album id");
        return E_INVALID_VALUES;
    }
    string targetAlbumId = whereArgs[0];
    CHECK_AND_RETURN_RET_LOG(!targetAlbumId.empty() && MediaLibraryDataManagerUtils::IsNumber(targetAlbumId),
        E_INVALID_VALUES, "target album id not exists");
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_DB_FAIL, "uniStore is nullptr! failed update for merge albums");
    vector<string> updateSqls;
    SetMyOldAlbum(updateSqls, uniStore);

    std::string updateDisplayLevelClause = "";
    int32_t getDisplayLevelRet = GetIsMeAlbumDisplayLevelClause(uniStore, targetAlbumId, updateDisplayLevelClause);
    CHECK_AND_RETURN_RET_LOG(getDisplayLevelRet == E_OK, getDisplayLevelRet, "Failed to query target album!");

    MEDIA_INFO_LOG("Start set is me, album id: %{public}s", targetAlbumId.c_str());
    std::string updateForSetIsMe = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + IS_ME + " = 1, " +
        ALBUM_RELATIONSHIP + " = 'me', " + RENAME_OPERATION + " = 1" + updateDisplayLevelClause
        + " WHERE " + GROUP_TAG + " IN(SELECT " +
        GROUP_TAG + " FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " + ALBUM_ID + " = " + targetAlbumId + ")";
    std::string updateSqlForGroupPhoto = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + RENAME_OPERATION + " = " +
        std::to_string(ALBUM_TO_RENAME_FOR_ANALYSIS) + " , " + IS_ME + " = 1 WHERE " + ALBUM_ID + " IN (SELECT " +
        ALBUM_ID + " FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " + GROUP_TAG + " LIKE ( SELECT CONCAT('%', " +
        GROUP_TAG + ", '%') FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " + ALBUM_ID + " = " + targetAlbumId + ") AND " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + " = " + std::to_string(PhotoAlbumSubType::GROUP_PHOTO) + " AND (" +
        RENAME_OPERATION + " != " + std::to_string(ALBUM_RENAMED) + " OR " + RENAME_OPERATION + " IS NULL))";
    updateSqls.push_back(updateForSetIsMe);
    updateSqls.push_back(updateSqlForGroupPhoto);
    vector<string> needUpdateSearchAlbumIds;
    QueryIsMeAndRelationshipAlbumIds(needUpdateSearchAlbumIds, uniStore);
    int32_t err = ExecSqls(updateSqls, uniStore);
    if (err == E_OK) {
        vector<int32_t> changeAlbumIds = { atoi(targetAlbumId.c_str()) };
        NotifyPortraitAlbum(changeAlbumIds);
    }
    QueryIsMeAndRelationshipAlbumIds(needUpdateSearchAlbumIds, uniStore);
    DoUpdateAlbumSearchIndex(needUpdateSearchAlbumIds);
    return err;
}

std::vector<std::string> GetUpdateSqlForSetAlbumName(std::string targetAlbumId, std::string albumName)
{
    std::string updateForSetAlbumName = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + ALBUM_NAME + " = '" + albumName +
        "' , " + RENAME_OPERATION + " = 1 WHERE " + GROUP_TAG + " IN(SELECT " + GROUP_TAG + " FROM " +
        ANALYSIS_ALBUM_TABLE + " WHERE " + ALBUM_ID + " = " + targetAlbumId + ")";
    std::string updateReNameOperation = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + RENAME_OPERATION + " = " +
        std::to_string(ALBUM_TO_RENAME_FOR_ANALYSIS) + " WHERE " + ALBUM_ID + " IN (SELECT " + ALBUM_ID + " FROM " +
        ANALYSIS_ALBUM_TABLE + " WHERE " + GROUP_TAG + " LIKE ( SELECT CONCAT('%', " + GROUP_TAG + ", '%') FROM " +
        ANALYSIS_ALBUM_TABLE + " WHERE " + ALBUM_ID + " = " + targetAlbumId + ") AND " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + " = " + std::to_string(PhotoAlbumSubType::GROUP_PHOTO) + " AND (" +
        RENAME_OPERATION + " != " + std::to_string(ALBUM_RENAMED) + " OR " + RENAME_OPERATION + " IS NULL))";
    vector<string> updateSqls = {updateForSetAlbumName, updateReNameOperation};
    return updateSqls;
}

static int32_t GetTargetAlbumId(const DataSharePredicates &predicates, std::string &targetAlbumId)
{
    RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, ANALYSIS_ALBUM_TABLE);

    auto whereArgs = rdbPredicates.GetWhereArgs();
    if (whereArgs.empty()) {
        MEDIA_ERR_LOG("no target album id");
        return E_INVALID_VALUES;
    }

    targetAlbumId = whereArgs[0];
    CHECK_AND_RETURN_RET_LOG(!targetAlbumId.empty() && MediaLibraryDataManagerUtils::IsNumber(targetAlbumId),
        E_INVALID_VALUES, "target album id not exists");

    return E_OK;
}

/**
 * set target album name
 * @param values album_name
 * @param predicates target album
 */
int32_t MediaLibraryAlbumOperations::SetAlbumName(const ValuesBucket &values, const DataSharePredicates &predicates)
{
    std::string targetAlbumId;
    int32_t err = GetTargetAlbumId(predicates, targetAlbumId);
    CHECK_AND_RETURN_RET(err == E_OK, err);
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_DB_FAIL, "uniStore is nullptr! failed update for set album name");
    string albumName;
    err = GetStringVal(values, ALBUM_NAME, albumName);
    if (err < 0 || albumName.empty()) {
        MEDIA_ERR_LOG("invalid album name");
        return E_INVALID_VALUES;
    }
    MEDIA_INFO_LOG("Set analysis album name, album id: %{public}s, album name: %{public}s",
        targetAlbumId.c_str(), DfxUtils::GetSafeAlbumName(albumName).c_str());
    std::vector<std::string> updateSqls = GetUpdateSqlForSetAlbumName(targetAlbumId, albumName);
    err = ExecSqls(updateSqls, uniStore);
    if (err == E_OK) {
        vector<int32_t> changeAlbumIds = { atoi(targetAlbumId.c_str()) };
        NotifyPortraitAlbum(changeAlbumIds);
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr! failed update index");
        return err;
    }
    const std::string  QUERY_MAP_ASSET_TO_UPDATE_INDEX =
        "SELECT map_asset FROM AnalysisPhotoMap WHERE map_album = " + targetAlbumId;
    vector<string> mapAssets;
    shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(QUERY_MAP_ASSET_TO_UPDATE_INDEX);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultSet is nullptr! failed update index");
        return err;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        mapAssets.push_back(to_string(GetInt32Val("map_asset", resultSet)));
    }
    MEDIA_INFO_LOG("update index map_asset size: %{public}zu", mapAssets.size());
    MediaAnalysisHelper::AsyncStartMediaAnalysisService(
        static_cast<int32_t>(MediaAnalysisProxy::ActivateServiceType::START_UPDATE_INDEX), mapAssets);

    return err;
}

/**
 * set target album uri
 * @param values cover_uri
 * @param predicates target album
 */
int32_t MediaLibraryAlbumOperations::SetCoverUri(const ValuesBucket &values, const DataSharePredicates &predicates)
{
    RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, ANALYSIS_ALBUM_TABLE);
    auto whereArgs = rdbPredicates.GetWhereArgs();
    if (whereArgs.size() == 0) {
        MEDIA_ERR_LOG("no target album id");
        return E_INVALID_VALUES;
    }
    string targetAlbumId = whereArgs[0];
    CHECK_AND_RETURN_RET_LOG(!targetAlbumId.empty() && MediaLibraryDataManagerUtils::IsNumber(targetAlbumId),
        E_INVALID_VALUES, "target album id not exists");
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr! failed update for set album cover uri");
        return E_DB_FAIL;
    }
    string coverUri;
    int err = GetStringVal(values, COVER_URI, coverUri);
    if (err < 0 || coverUri.empty()) {
        MEDIA_ERR_LOG("invalid album cover uri");
        return E_INVALID_VALUES;
    }
    MEDIA_INFO_LOG("Set analysis album cover uri, album id: %{public}s, cover uri: %{public}s",
        targetAlbumId.c_str(), DfxUtils::GetSafeUri(coverUri).c_str());
    std::string updateForSetCoverUri = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + COVER_URI + " = '" + coverUri +
        "', " + IS_COVER_SATISFIED + " = " + to_string(static_cast<uint8_t>(CoverSatisfiedType::USER_SETTING)) +
        " WHERE " + GROUP_TAG + " IN(SELECT " + GROUP_TAG + " FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " + ALBUM_ID +
        " = " + targetAlbumId + ")";
    vector<string> updateSqls = { updateForSetCoverUri };
    err = ExecSqls(updateSqls, uniStore);
    if (err == E_OK) {
        vector<int32_t> changeAlbumIds = { atoi(targetAlbumId.c_str()) };
        NotifyPortraitAlbum(changeAlbumIds);
    }
    return err;
}

static bool GetArgsSetUserAlbumName(const ValuesBucket& values,
    const DataSharePredicates& predicates, int32_t& oldAlbumId, string& newAlbumName)
{
    RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, PhotoAlbumColumns::TABLE);
    CHECK_AND_RETURN_RET_LOG(!rdbPredicates.GetWhereArgs().empty(), false,
        "Rename user album failed. Args is empty");
    oldAlbumId = atoi(rdbPredicates.GetWhereArgs()[0].c_str());
    CHECK_AND_RETURN_RET_LOG(oldAlbumId > 0, false, "Rename user album failed. Invalid album id: %{public}s",
        rdbPredicates.GetWhereArgs()[0].c_str());
    CHECK_AND_RETURN_RET_LOG(GetStringObject(values, PhotoAlbumColumns::ALBUM_NAME, newAlbumName) == E_OK, false,
        "Rename user album failed. Get new album name failed");
    return true;
}

int32_t MediaLibraryAlbumOperations::HandleSetAlbumNameRequest(const ValuesBucket &values,
    const DataSharePredicates &predicates)
{
    int32_t oldAlbumId {};
    string newAlbumName {};
    CHECK_AND_RETURN_RET_LOG(GetArgsSetUserAlbumName(values, predicates, oldAlbumId, newAlbumName),
        E_INVALID_ARGS, "Set album name args invalid");
    return RenameUserAlbum(oldAlbumId, newAlbumName);
}

int32_t MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(const OperationType &opType,
    const NativeRdb::ValuesBucket &values, const DataShare::DataSharePredicates &predicates,
    std::shared_ptr<int> countPtr)
{
    switch (opType) {
        case OperationType::PORTRAIT_DISPLAY_LEVEL:
            return SetDisplayLevel(values, predicates);
        case OperationType::PORTRAIT_MERGE_ALBUM:
            return MergeAlbums(values);
        case OperationType::PORTRAIT_IS_ME:
            return SetIsMe(values, predicates);
        case OperationType::PORTRAIT_ALBUM_NAME:
            return SetAlbumName(values, predicates);
        case OperationType::PORTRAIT_COVER_URI:
            return SetCoverUri(values, predicates);
        case OperationType::HIGHLIGHT_ALBUM_NAME:
            return SetHighlightAlbumName(values, predicates);
        case OperationType::HIGHLIGHT_COVER_URI:
            return SetHighlightCoverUri(values, predicates);
        case OperationType::HIGHLIGHT_SUBTITLE:
            return SetHighlightSubtitle(values, predicates);
        case OperationType::RELATIONSHIP:
            return SetAnalysisAlbumRelationship(values, predicates);
        case OperationType::DISMISS:
        case OperationType::GROUP_ALBUM_NAME:
        case OperationType::GROUP_COVER_URI:
            return MediaLibraryAnalysisAlbumOperations::HandleGroupPhotoAlbum(opType, values, predicates);
        default:
            MEDIA_ERR_LOG("Unknown operation type: %{public}d", opType);
            return E_ERR;
    }
}

int32_t MediaLibraryAlbumOperations::HandlePhotoAlbum(const OperationType &opType, const ValuesBucket &values,
    const DataSharePredicates &predicates, shared_ptr<int> countPtr)
{
    switch (opType) {
        case OperationType::UPDATE:
            return UpdatePhotoAlbum(values, predicates);
        case OperationType::SET_USER_ALBUM_COVER_URI:
        case OperationType::SET_SOURCE_ALBUM_COVER_URI:
            return UpdateAlbumCoverUri(values, predicates, false);
        case OperationType::SET_SYSTEM_ALBUM_COVER_URI:
            return UpdateAlbumCoverUri(values, predicates, true);
        case OperationType::RESET_COVER_URI:
            return ResetCoverUri(values, predicates);
        case OperationType::ALBUM_RECOVER_ASSETS:
            return MediaLibraryAlbumOperations::RecoverPhotoAssets(predicates);
        case OperationType::ALBUM_DELETE_ASSETS:
            return DeletePhotoAssets(predicates, false, false);
        case OperationType::COMPAT_ALBUM_DELETE_ASSETS:
            return DeletePhotoAssets(predicates, false, true);
        case OperationType::DELETE_LOCAL_ASSETS_PERMANENTLY:
            return DeletePhotoAssetsCompleted(predicates, false);
        case OperationType::AGING:
            return AgingPhotoAssets(countPtr);
        case OperationType::ALBUM_ORDER:
            return OrderSingleAlbum(values);
        case OperationType::ALBUM_SET_NAME:
            return HandleSetAlbumNameRequest(values, predicates);
        default:
            MEDIA_ERR_LOG("Unknown operation type: %{public}d", opType);
            return E_ERR;
    }
}

int MediaLibraryAlbumOperations::HandlePhotoAlbumOperations(MediaLibraryCommand &cmd)
{
    switch (cmd.GetOprnType()) {
        case OperationType::CREATE:
            return CreatePhotoAlbum(cmd);
        default:
            MEDIA_ERR_LOG("Unknown operation type: %{public}d", cmd.GetOprnType());
            return E_ERR;
    }
}

int32_t UpdateOneAlbumOrder(const shared_ptr<MediaLibraryRdbStore> &rdbStore,
    const NativeRdb::ValuesBucket &ValuesBucket, const NativeRdb::RdbPredicates &predicates)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "get rdbStore failed");

    int32_t changeRows = 0;
    auto ret = rdbStore->Update(changeRows, ValuesBucket, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret, "Failed to update album order");
    return ret;
}

int32_t MediaLibraryAlbumOperations::UpdatePhotoAlbumOrder(const vector<NativeRdb::ValuesBucket> &valuesBuckets,
    const vector<NativeRdb::RdbPredicates> &predicatesArray)
{
    CHECK_AND_RETURN_RET_LOG(valuesBuckets.size() == predicatesArray.size(), E_INNER_FAIL,
        "valuesBuckets.size() not equal to predicatesArray.size(), can not update album order");
    
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_INNER_FAIL, "get rdbStore failed");

    int32_t err = 0;
    int32_t successSetOrderRows = 0;
    if (!valuesBuckets.empty() && !predicatesArray.empty()) {
        for (size_t i = 0; i < valuesBuckets.size(); i++) {
            err = UpdateOneAlbumOrder(rdbStore, valuesBuckets[i], predicatesArray[i]);
            CHECK_AND_EXECUTE(err != NativeRdb::E_OK, ++successSetOrderRows);
        }
        MEDIA_INFO_LOG("success update order album size: %{public}d", successSetOrderRows);

        auto watch = MediaLibraryNotify::GetInstance();
        CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");
        watch->Notify(PhotoAlbumColumns::ALBUM_URI_PREFIX, NotifyType::NOTIFY_UPDATE);
    }
    int32_t ret = rdbStore->ExecuteSql(SQL_DELETE_ALL_ALBUM_ORDER_BACK);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, false, "Failed to delete albumOrderBackTable data");
    return successSetOrderRows == 0 ? E_INNER_FAIL : E_OK;
}
} // namespace OHOS::Media

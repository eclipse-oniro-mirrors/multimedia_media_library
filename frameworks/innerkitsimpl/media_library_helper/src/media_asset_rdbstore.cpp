/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "media_asset_rdbstore.h"

#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_tracer.h"
#include "parameters.h"
#include "photo_album_column.h"
#include "photo_map_column.h"
#include "vision_column.h"
#include "rdb_sql_utils.h"

using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::RdbDataShareAdapter;
using namespace OHOS::Media::MediaOperation;

namespace OHOS {
namespace Media {

const std::string MEDIA_LIBRARY_STARTUP_PARAM_PREFIX = "multimedia.medialibrary.startup.";
constexpr uint32_t BASE_USER_RANGE = 200000;
const int32_t ARG_COUNT = 2;
const std::unordered_set<OperationObject> OPERATION_OBJECT_SET = {
    OperationObject::UFM_PHOTO,
    OperationObject::UFM_AUDIO,
    OperationObject::PAH_PHOTO,
    OperationObject::PAH_MAP,
};
const std::unordered_set<OperationType> OPERATION_TYPE_SET = {
    OperationType::QUERY,
};
// LCOV_EXCL_START
std::string GetTableNameFromOprnObject(const OperationObject& object)
{
    CHECK_AND_RETURN_RET(object != OperationObject::PAH_MAP, PhotoColumn::PHOTOS_TABLE);
    if (TABLE_NAME_MAP.find(object) != TABLE_NAME_MAP.end()) {
        auto cmdObj = TABLE_NAME_MAP.at(object);
        return cmdObj.begin()->second;
    } else {
        return MEDIALIBRARY_TABLE;
    }
}

OperationType GetOprnTypeFromUri(Uri& uri)
{
    const std::string opType = MediaFileUri::GetPathSecondDentry(uri);
    if (OPRN_TYPE_MAP.find(opType) != OPRN_TYPE_MAP.end()) {
        return OPRN_TYPE_MAP.at(opType);
    } else {
        return OperationType::QUERY;
    }
}

OperationObject GetOprnObjectFromUri(Uri& uri)
{
    const string opObject = MediaFileUri::GetPathFirstDentry(uri);
    CHECK_AND_RETURN_RET(OPRN_OBJ_MAP.find(opObject) == OPRN_OBJ_MAP.end(),
        OPRN_OBJ_MAP.at(opObject));
    std::string uriString = uri.ToString();
    CHECK_AND_RETURN_RET(!MediaFileUtils::StartsWith(uriString, PhotoColumn::PHOTO_CACHE_URI_PREFIX),
        OperationObject::PAH_PHOTO);

    for (const auto &item : OPRN_MAP) {
        CHECK_AND_RETURN_RET(!MediaFileUtils::StartsWith(uriString, item.first), item.second);
    }
    return OperationObject::UNKNOWN_OBJECT;
}

MediaAssetRdbStore* MediaAssetRdbStore::GetInstance()
{
    static MediaAssetRdbStore instance;
    return &instance;
}

const std::string MediaAssetRdbStore::CloudSyncTriggerFunc(const std::vector<std::string>& args)
{
    return "true";
}

const std::string MediaAssetRdbStore::IsCallerSelfFunc(const std::vector<std::string>& args)
{
    return "false";
}

const std::string MediaAssetRdbStore::PhotoAlbumNotifyFunc(const std::vector<std::string> &args)
{
    return "";
}

MediaAssetRdbStore::MediaAssetRdbStore()
{
    MEDIA_INFO_LOG("init visitor rdb");
    CHECK_AND_RETURN_INFO_LOG(rdbStore_ == nullptr, "visitor rdb exists");
    CHECK_AND_RETURN(TryGetRdbStore() == NativeRdb::E_OK);
    MEDIA_INFO_LOG("success to init visitor rdb");
}

int32_t MediaAssetRdbStore::TryGetRdbStore(bool isIgnoreSELinux)
{
    std::unique_lock<std::mutex> lock(mutex_);
    auto context = AbilityRuntime::Context::GetApplicationContext();
    CHECK_AND_RETURN_RET_LOG(context != nullptr, NativeRdb::E_ERROR, "fail to acquire application Context");
    uid_t uid = getuid() / BASE_USER_RANGE;
    const string key = MEDIA_LIBRARY_STARTUP_PARAM_PREFIX + to_string(uid);
    auto rdbInitFlag = system::GetBoolParameter(key, false);
    bool cond = (!rdbInitFlag && !isIgnoreSELinux);
    CHECK_AND_RETURN_RET_LOG(!cond, NativeRdb::E_ERROR,
        "media library db update not complete, key:%{public}s", key.c_str());

    string name = MEDIA_DATA_ABILITY_DB_NAME;
    string databaseDir = MEDIA_DB_DIR + "/rdb";
    if (access(databaseDir.c_str(), E_OK) != 0) {
        MEDIA_WARN_LOG("can not get rdb through sandbox");
        return NativeRdb::E_ERROR;
    }
    string dbPath = databaseDir.append("/").append(name);
    int32_t errCode = 0;
    NativeRdb::RdbStoreConfig config {""};
    config.SetName(name);
    config.SetVisitorDir(dbPath);
    config.SetBundleName(context->GetBundleName());
    config.SetArea(context->GetArea());
    config.SetSecurityLevel(SecurityLevel::S3);
    config.SetRoleType(RoleType::VISITOR);
    config.SetScalarFunction("cloud_sync_func", 0, CloudSyncTriggerFunc);
    config.SetScalarFunction("is_caller_self_func", 0, IsCallerSelfFunc);
    config.SetScalarFunction("photo_album_notify_func", ARG_COUNT, PhotoAlbumNotifyFunc);
    config.SetCollatorLocales("zh_CN");

    MediaLibraryDataCallBack rdbDataCallBack;
    rdbStore_ = RdbHelper::GetRdbStore(config, MEDIA_RDB_VERSION, rdbDataCallBack, errCode);
    if (rdbStore_ == nullptr || errCode != NativeRdb::E_OK) {
        MEDIA_WARN_LOG("Failed to get visitor RdbStore, errCode: %{public}d", errCode);
        rdbStore_ = nullptr;
        return errCode;
    }
    return NativeRdb::E_OK;
}

void AddVirtualColumnsOfDateType(vector<string>& columns)
{
    vector<string> dateTypes = { MEDIA_DATA_DB_DATE_ADDED, MEDIA_DATA_DB_DATE_TRASHED, MEDIA_DATA_DB_DATE_MODIFIED,
            MEDIA_DATA_DB_DATE_TAKEN };
    vector<string> dateTypeSeconds = { MEDIA_DATA_DB_DATE_ADDED_TO_SECOND,
            MEDIA_DATA_DB_DATE_TRASHED_TO_SECOND, MEDIA_DATA_DB_DATE_MODIFIED_TO_SECOND,
            MEDIA_DATA_DB_DATE_TAKEN_TO_SECOND };
    for (size_t i = 0; i < dateTypes.size(); i++) {
        auto it = find(columns.begin(), columns.end(), dateTypes[i]);
        if (it != columns.end()) {
            columns.push_back(dateTypeSeconds[i]);
        }
    }
}

void AddQueryIndex(AbsPredicates& predicates, const vector<string>& columns)
{
    auto it = find(columns.begin(), columns.end(), MEDIA_COLUMN_COUNT);
    if (it == columns.end()) {
        return;
    }
    const string &group = predicates.GetGroup();
    const string &whereInfo = predicates.GetWhereClause();
    if (group.empty()) {
        predicates.GroupBy({ PhotoColumn::PHOTO_DATE_DAY });
        predicates.IndexedBy(PhotoColumn::PHOTO_SCHPT_DAY_INDEX);
        return;
    }
    if (group == PhotoColumn::MEDIA_TYPE && (whereInfo.find(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE) == string::npos)) {
        predicates.IndexedBy(PhotoColumn::PHOTO_SCHPT_MEDIA_TYPE_INDEX);
        return;
    }
    if (group == PhotoColumn::PHOTO_DATE_DAY) {
        predicates.IndexedBy(PhotoColumn::PHOTO_SCHPT_DAY_INDEX);
        return;
    }
}

static string GetQueryFilter(const string &tableName)
{
    if (tableName == MEDIALIBRARY_TABLE) {
        return MEDIALIBRARY_TABLE + "." + MEDIA_DATA_DB_SYNC_STATUS + " = " +
            to_string(static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE));
    }
    if (tableName == PhotoColumn::PHOTOS_TABLE) {
        return PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_SYNC_STATUS + " = " +
            to_string(static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE)) + " AND " +
            PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_CLEAN_FLAG + " = " +
            to_string(static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN));
    }
    if (tableName == PhotoAlbumColumns::TABLE) {
        return PhotoAlbumColumns::TABLE + "." + PhotoAlbumColumns::ALBUM_DIRTY + " != " +
            to_string(static_cast<int32_t>(DirtyTypes::TYPE_DELETED));
    }
    if (tableName == PhotoMap::TABLE) {
        return PhotoMap::TABLE + "." + PhotoMap::DIRTY + " != " + to_string(static_cast<int32_t>(
            DirtyTypes::TYPE_DELETED));
    }
    return "";
}

void AddQueryFilter(AbsRdbPredicates &predicates)
{
    /* build all-table vector */
    string tableName = predicates.GetTableName();
    vector<string> joinTables = predicates.GetJoinTableNames();
    joinTables.push_back(tableName);
    /* add filters */
    string filters;
    for (auto &t : joinTables) {
        string filter = GetQueryFilter(t);
        CHECK_AND_CONTINUE(!filter.empty());
        if (filters.empty()) {
            filters += filter;
        } else {
            filters += " AND " + filter;
        }
    }
    CHECK_AND_RETURN(!filters.empty());
    /* rebuild */
    string queryCondition = predicates.GetWhereClause();
    MEDIA_DEBUG_LOG("queryCondition: %{public}s", queryCondition.c_str());
    queryCondition = queryCondition.empty() ? filters : filters + " AND " + queryCondition;
    predicates.SetWhereClause(queryCondition);
}

std::shared_ptr<DataShare::DataShareResultSet> MediaAssetRdbStore::Query(
    const DataShare::DataSharePredicates& predicates,
    std::vector<std::string>& columns, OperationObject& object, int& errCode)
{
    auto resultSet = QueryRdb(predicates, columns, object);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, nullptr, "fail to acquire result from visitor query");
    auto resultSetBridge = RdbUtils::ToResultSetBridge(resultSet);
    return make_shared<DataShare::DataShareResultSet>(resultSetBridge);
}

bool IsNumber(const string& str)
{
    CHECK_AND_RETURN_RET_LOG(!str.empty(), false, "IsNumber input is empty");
    for (char const& c : str) {
        CHECK_AND_RETURN_RET(isdigit(c) != 0, false);
    }
    return true;
}

int32_t GetInt32Val(const string& column, std::shared_ptr<NativeRdb::AbsSharedResultSet>& resultSet)
{
    int index;
    int32_t value = -1;
    int err = resultSet->GetColumnIndex(column, index);
    if (err == E_OK) {
        err = resultSet->GetInt(index, value);
    }
    return value;
}

bool MediaAssetRdbStore::IsQueryGroupPhotoAlbumAssets(const string &albumId)
{
    bool cond = (albumId.empty() || !IsNumber(albumId));
    CHECK_AND_RETURN_RET(!cond, false);
    RdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    vector<string> columns = {PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumColumns::ALBUM_SUBTYPE};
    auto resultSet = rdbStore_->Query(predicates, columns);
    if (resultSet == nullptr) {
        return false;
    }
    if (resultSet->GoToFirstRow() != E_OK) {
        resultSet->Close();
        return false;
    }
    int32_t albumType = GetInt32Val(PhotoAlbumColumns::ALBUM_TYPE, resultSet);
    int32_t albumSubtype = GetInt32Val(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet);
    resultSet->Close();
    return albumType == PhotoAlbumType::SMART && albumSubtype == PhotoAlbumSubType::GROUP_PHOTO;
}

bool MediaAssetRdbStore::IsQueryAccessibleViaSandBox(Uri& uri, OperationObject& object,
    const DataShare::DataSharePredicates& predicates, bool isIgnoreSELinux)
{
    CHECK_AND_RETURN_RET(access(MEDIA_DB_DIR.c_str(), E_OK) == 0, false);
    if (rdbStore_ == nullptr) {
        CHECK_AND_RETURN_RET_LOG(TryGetRdbStore(isIgnoreSELinux) == NativeRdb::E_OK, false,
            "fail to acquire rdb when query");
    }

    object = GetOprnObjectFromUri(uri);
    CHECK_AND_RETURN_RET(OPERATION_OBJECT_SET.count(object) != 0, false);
    OperationType type = GetOprnTypeFromUri(uri);
    CHECK_AND_RETURN_RET(OPERATION_TYPE_SET.count(type) != 0, false);
    CHECK_AND_RETURN_RET(object == OperationObject::PAH_MAP, true);

    std::string tableName = GetTableNameFromOprnObject(object);
    NativeRdb::RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, tableName);
    auto whereArgs = rdbPredicates.GetWhereArgs();
    if (!whereArgs.empty()) {
        string albumId = whereArgs[0];
        CHECK_AND_RETURN_RET(!IsQueryGroupPhotoAlbumAssets(albumId), false);
    }
    return true;
}

static void PrintPredicatesInfo(const NativeRdb::RdbPredicates& predicates, const vector<string>& columns)
{
    string argsInfo;
    for (const auto& arg : predicates.GetWhereArgs()) {
        if (!argsInfo.empty()) {
            argsInfo += ", ";
        }
        argsInfo += arg;
    }
    MEDIA_DEBUG_LOG("PhotosApp Predicates Statement is %{private}s",
        RdbSqlUtils::BuildQueryString(predicates, columns).c_str());
    MEDIA_DEBUG_LOG("PhotosApp Predicates Args are %{private}s", argsInfo.c_str());
}

std::shared_ptr<NativeRdb::ResultSet> MediaAssetRdbStore::QueryRdb(
    const DataShare::DataSharePredicates& predicates, std::vector<std::string>& columns, OperationObject& object)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, nullptr, "fail to acquire rdb when query");
    std::string tableName = GetTableNameFromOprnObject(object);
    NativeRdb::RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, tableName);
    AddVirtualColumnsOfDateType(const_cast<vector<string> &>(columns));
    if (object == OperationObject::UFM_PHOTO || object == OperationObject::PAH_PHOTO) {
        AddQueryIndex(rdbPredicates, columns);
    }

    AddQueryFilter(rdbPredicates);
    PrintPredicatesInfo(rdbPredicates, columns);
    MediaLibraryTracer tracer;
    tracer.Start("MediaAssetRdbStore::QueryRdb QueryByStep");
    auto resultSet = rdbStore_->QueryByStep(rdbPredicates, columns, false);
    tracer.Finish();
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, nullptr, "fail to acquire result from visitor query");
    return resultSet;
}

std::shared_ptr<NativeRdb::ResultSet> MediaAssetRdbStore::QueryByStep(const std::string &sql)
{
    if (rdbStore_ == nullptr) {
        if (TryGetRdbStore(false) != NativeRdb::E_OK) {
            return nullptr;
        }
    }
    return rdbStore_->QueryByStep(sql);
}

bool MediaAssetRdbStore::IsSupportSharedAssetQuery(Uri& uri, OperationObject& object, bool isIgnoreSELinux)
{
    CHECK_AND_RETURN_RET(access(MEDIA_DB_DIR.c_str(), E_OK) == 0, false);
    if (rdbStore_ == nullptr) {
        CHECK_AND_RETURN_RET_LOG(TryGetRdbStore(isIgnoreSELinux) == NativeRdb::E_OK, false,
            "fail to acquire rdb when query");
    }

    OperationType type = GetOprnTypeFromUri(uri);
    CHECK_AND_RETURN_RET(OPERATION_TYPE_SET.count(type) != 0, false);
    object = GetOprnObjectFromUri(uri);
    return true;
}

int32_t MediaAssetRdbStore::QueryTimeIdBatch(int32_t start, int32_t count, std::vector<std::string> &batchKeys)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaAssetRdbStore::QueryTimeIdBatch");
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, NativeRdb::E_DB_NOT_EXIST, "rdbStore_ is nullptr when query");
    DataShare::DataSharePredicates predicates;
    predicates.And()->OrderByDesc(MediaColumn::MEDIA_DATE_TAKEN)
                    ->Limit(count, start)
                    ->EqualTo(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, "1")
                    ->EqualTo(MediaColumn::MEDIA_DATE_TRASHED, "0")
                    ->EqualTo(MediaColumn::MEDIA_TIME_PENDING, "0")
                    ->EqualTo(MediaColumn::MEDIA_HIDDEN, "0")
                    ->EqualTo(PhotoColumn::PHOTO_IS_TEMP, "0")
                    ->EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL,
                        to_string(static_cast<int32_t>(BurstCoverLevelType::COVER)));
    std::vector<std::string> columns = {MediaColumn::MEDIA_ID, MediaColumn::MEDIA_DATE_TAKEN};
    NativeRdb::RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, PhotoColumn::PHOTOS_TABLE);
    AddQueryFilter(rdbPredicates);
    auto resultSet = rdbStore_->Query(rdbPredicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, NativeRdb::E_ERROR, "fail to acquire result from visitor query");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int columnIndex = 0;
        int64_t dateTakenTime = 0;
        int fileId = 0;
        bool cond = (resultSet->GetColumnIndex(MediaColumn::MEDIA_DATE_TAKEN, columnIndex) != NativeRdb::E_OK ||
            resultSet->GetLong(columnIndex, dateTakenTime) != NativeRdb::E_OK);
        CHECK_AND_RETURN_RET_LOG(!cond, NativeRdb::E_ERROR, "Fail to get dateTaken");

        cond = (resultSet->GetColumnIndex(MediaColumn::MEDIA_ID, columnIndex) != NativeRdb::E_OK ||
            resultSet->GetInt(columnIndex, fileId) != NativeRdb::E_OK);
        CHECK_AND_RETURN_RET_LOG(!cond,  NativeRdb::E_ERROR, "Fail to get fileId");

        std::string timeId;
        CHECK_AND_CONTINUE_ERR_LOG(MediaFileUtils::GenerateKvStoreKey(to_string(fileId),
            to_string(dateTakenTime), timeId),
            "Fail to generate kvStore key, fileId:%{public}d", fileId);
        batchKeys.emplace_back(std::move(timeId));
    }
    resultSet->Close();
    return NativeRdb::E_OK;
}

int32_t MediaLibraryDataCallBack::OnCreate(NativeRdb::RdbStore& rdbStore)
{
    return 0;
}

int32_t MediaLibraryDataCallBack::OnUpgrade(NativeRdb::RdbStore& rdbStore, int32_t oldVersion, int32_t newVersion)
{
    return 0;
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS
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
#include <cerrno>

#include "iservice_registry.h"
#include <fstream>
#include <charconv>

#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_tracer.h"
#include "parameters.h"
#include "photo_album_column.h"
#include "photo_map_column.h"
#include "vision_column.h"
#include "rdb_sql_utils.h"
#include "media_string_utils.h"
#include "medialibrary_errno.h"
#include "media_column.h"
#include "ipc_skeleton.h"
#include "permission_utils.h"

#include <cerrno>
#include <sys/stat.h>

using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::RdbDataShareAdapter;
using namespace OHOS::Media::MediaOperation;
using namespace OHOS::Security::AccessToken;

namespace OHOS {
namespace Media {

const std::string MEDIA_LIBRARY_STARTUP_PARAM_PREFIX = "multimedia.medialibrary.startup.";
constexpr uint32_t BASE_USER_RANGE = 200000;
const int32_t ARG_COUNT = 2;
const int32_t MAP_CODE_PARAM = 3;
constexpr int STORAGE_MANAGER_MANAGER_ID = 5003;
constexpr int32_t MILLIS_PER_SEC = 1000;
constexpr int32_t MILLIS_FIVE_SECONDS = 5 * 1000;
static const std::string CONST_MEDIA_SECURE_ALBUM = "const.media.secure_album";

// Inline implementations to avoid dependency on media_library_helper symbols
static const std::string KVSTORE_FILE_ID_TEMPLATE = "0000000000";
static const std::string KVSTORE_DATE_KEY_TEMPLATE = "0000000000000";

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
const std::string FOREGROUND_ACCOUNT_ID_XML = std::string(CONST_MEDIA_DB_DIR) +
    "/rdb/medialibrary_foreground_account_id.xml";
#endif

static int32_t UriValidCheck(Uri &uri)
{
    std::string scheme = uri.GetScheme();
    if (scheme != "file" && scheme != "datashare") {
        MEDIA_ERR_LOG("scheme is invalid, uri is %{private}s", uri.ToString().c_str());
        return E_INVALID_URI;
    }

    if (uri.GetAuthority() != "media" &&
        uri.GetPath().find("/media") != 0) {
        MEDIA_ERR_LOG("failed to find /media, uri is %{private}s", uri.ToString().c_str());
        return E_INVALID_URI;
    }
    return E_OK;
}

static inline void HandleOldUriPath(std::string &path)
{
    // Handle datashare:///media and datashare:///media/file_operation case
    if (MediaStringUtils::StartsWith(path, "/media")) {
        path = path.substr(string("/media").size());
        return;
    }
}

static inline void RemovePrecedSlash(std::string &path)
{
    if (MediaStringUtils::StartsWith(path, SLASH_STR)) {
        path = path.substr(SLASH_STR.size());
    }
}

static void GetValidPath(Uri &uri, std::string &path)
{
    if (UriValidCheck(uri) < 0) {
        path = "";
        return;
    }

    path = uri.GetPath();
    HandleOldUriPath(path);
    RemovePrecedSlash(path);
}

static std::string GetPathFirstDentry(Uri &uri)
{
    std::string path;
    GetValidPath(uri, path);
    // Example: file:://media/photo_operation/query, return the "photo_operation" part
    return path.substr(0, path.find_first_of('/'));
}

static std::string GetPathSecondDentry(Uri &uri)
{
    std::string ret;
    std::string firstDentry = GetPathFirstDentry(uri);
    if (firstDentry.empty()) {
        return ret;
    }
    std::string path;
    GetValidPath(uri, path);
    if (path.size() < firstDentry.size() + 1) {
        return ret;
    }
    // Example: file:://media/photo_operation/query, return the "query" part
    return path.substr(firstDentry.size() + 1);
}

static bool GenerateKvStoreKey(const std::string &fileId, const std::string &dateKey, std::string &key)
{
    if (fileId.empty() || dateKey.empty()) {
        MEDIA_ERR_LOG("GenerateKvStoreKey: fileId or dateKey is empty");
        return false;
    }
    if (fileId.length() > KVSTORE_FILE_ID_TEMPLATE.length() ||
        dateKey.length() > KVSTORE_DATE_KEY_TEMPLATE.length()) {
        MEDIA_ERR_LOG("GenerateKvStoreKey: fileId or dateKey is too long");
        return false;
    }
    key = KVSTORE_DATE_KEY_TEMPLATE.substr(dateKey.length()) + dateKey +
          KVSTORE_FILE_ID_TEMPLATE.substr(fileId.length()) + fileId;
    return true;
}

const std::unordered_set<OperationObject> OPERATION_OBJECT_SET = {
    OperationObject::UFM_PHOTO,
    OperationObject::UFM_AUDIO,
    OperationObject::PAH_PHOTO,
    OperationObject::PAH_MAP,
};
const std::unordered_set<OperationType> OPERATION_TYPE_SET = {
    OperationType::QUERY,
};
const std::string REVERSE_CLONE_RESTORE_MARKER_XML = std::string(CONST_MEDIA_DB_DIR) +
    "/rdb/media_library_reverse_restore_marker.xml";
constexpr int64_t MARKER_MILLISECONDS_PER_SECOND = 1000;
constexpr int64_t MARKER_NANOSECONDS_PER_MILLISECOND = 1000000;

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
const std::string FOREGROUND_ACCOUNT_ID_XML = std::string(CONST_MEDIA_DB_DIR) +
    "/rdb/medialibrary_foreground_account_id.xml";
#endif

// LCOV_EXCL_START
bool GetReverseRestoreMarkerTime(int64_t &recordTimeMilliseconds)
{
    MEDIA_INFO_LOG("query reverse clone restore marker xml, path=%{public}s",
        REVERSE_CLONE_RESTORE_MARKER_XML.c_str());
    struct stat statInfo {};
    if (stat(REVERSE_CLONE_RESTORE_MARKER_XML.c_str(), &statInfo) != 0) {
        int32_t err = errno;
        if (err == ENOENT) {
            MEDIA_DEBUG_LOG("reverse clone restore marker file does not exist, path=%{public}s",
                REVERSE_CLONE_RESTORE_MARKER_XML.c_str());
        } else {
            MEDIA_WARN_LOG("stat reverse clone restore marker file failed, err=%{public}d, path=%{public}s",
                err, REVERSE_CLONE_RESTORE_MARKER_XML.c_str());
        }
        return false;
    }
    recordTimeMilliseconds = static_cast<int64_t>(statInfo.st_mtim.tv_sec) * MARKER_MILLISECONDS_PER_SECOND +
        static_cast<int64_t>(statInfo.st_mtim.tv_nsec) / MARKER_NANOSECONDS_PER_MILLISECOND;
    MEDIA_INFO_LOG("reverse clone restore marker time queried, timeMs=%{public}lld",
        static_cast<long long>(recordTimeMilliseconds));
    CHECK_AND_RETURN_RET_LOG(recordTimeMilliseconds > 0, false,
        "reverse clone marker time invalid, timeMs=%{public}lld", static_cast<long long>(recordTimeMilliseconds));
    return true;
}

std::string GetTableNameFromOprnObject(const OperationObject& object)
{
    CHECK_AND_RETURN_RET(object != OperationObject::PAH_MAP, PhotoColumn::PHOTOS_TABLE);
    if (TABLE_NAME_MAP.find(object) != TABLE_NAME_MAP.end()) {
        auto cmdObj = TABLE_NAME_MAP.at(object);
        return cmdObj.begin()->second;
    } else {
        return CONST_MEDIALIBRARY_TABLE;
    }
}

OperationType GetOprnTypeFromUri(Uri& uri)
{
    const std::string opType = GetPathSecondDentry(uri);
    if (OPRN_TYPE_MAP.find(opType) != OPRN_TYPE_MAP.end()) {
        return OPRN_TYPE_MAP.at(opType);
    } else {
        return OperationType::QUERY;
    }
}

OperationObject GetOprnObjectFromUri(Uri& uri)
{
    const string opObject = GetPathFirstDentry(uri);
    CHECK_AND_RETURN_RET(OPRN_OBJ_MAP.find(opObject) == OPRN_OBJ_MAP.end(),
        OPRN_OBJ_MAP.at(opObject));
    std::string uriString = uri.ToString();
    CHECK_AND_RETURN_RET(!MediaStringUtils::StartsWith(uriString, PhotoColumn::PHOTO_CACHE_URI_PREFIX),
        OperationObject::PAH_PHOTO);

    for (const auto &item : OPRN_MAP) {
        CHECK_AND_RETURN_RET(!MediaStringUtils::StartsWith(uriString, item.first), item.second);
    }
    return OperationObject::UNKNOWN_OBJECT;
}

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
static bool ReadForegroundAccountId(int32_t& accountId)
{
    std::ifstream ifs(FOREGROUND_ACCOUNT_ID_XML);
    if (!ifs.is_open()) {
        MEDIA_INFO_LOG("file not found, multi-account not supported");
        return false;
    }
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    // Preferences XML format: <int key="foregroundAccountId" value="100xxx"/>
    const std::string keyAttr = "key=\"foregroundAccountId\"";
    auto keyPos = content.find(keyAttr);
    if (keyPos == std::string::npos) {
        MEDIA_WARN_LOG("key attr not found, multi-account not supported");
        return false;
    }
    const std::string valueTag = "value=\"";
    auto valueStart = content.find(valueTag, keyPos + keyAttr.length());
    if (valueStart == std::string::npos) {
        MEDIA_WARN_LOG("value attr not found, multi-account not supported");
        return false;
    }
    size_t valBegin = valueStart + valueTag.length();
    auto valEnd = content.find("\"", valBegin);
    if (valEnd == std::string::npos) {
        MEDIA_WARN_LOG("value attr end not found, multi-account not supported");
        return false;
    }
    std::string valueStr = content.substr(valBegin, valEnd - valBegin);
    if (!MediaStringUtils::IsAccountId(valueStr)) {
        MEDIA_WARN_LOG("invalid accountId=[%{public}s], multi-account not supported", valueStr.c_str());
        return false;
    }

    int32_t val = 0;
    auto [ptr, ec] = std::from_chars(valueStr.data(), valueStr.data() + valueStr.size(), val);
    if (ec != std::errc() || ptr != valueStr.data() + valueStr.size()) {
        MEDIA_WARN_LOG("invalid value=[%{public}s], multi-account not supported", valueStr.c_str());
        return false;
    }
    accountId = val;
    MEDIA_INFO_LOG("foregroundAccountId = %{public}d", accountId);
    return true;
}

static bool ReadKvAccountId(int32_t &accountId)
{
    accountId = -1;
    // Only a genuinely absent publication means the legacy client layout.
    // Denied/corrupt publication must not redirect a client to the legacy DB.
    if (access(FOREGROUND_ACCOUNT_ID_XML.c_str(), F_OK) != 0) {
        return errno == ENOENT;
    }
    return ReadForegroundAccountId(accountId) && accountId > 0;
}
#endif

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

const std::string MediaAssetRdbStore::PhotoMapCodeFunc(const std::vector<std::string> &args)
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

bool ConnectMediaLibrary()
{
    MediaLibraryTracer tracer;
    tracer.Start("ConnectMediaLibrary");
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHECK_AND_RETURN_RET_LOG(saManager != nullptr, false, "Get system ability mgr failed.");

    auto remoteObj = saManager->GetSystemAbility(STORAGE_MANAGER_MANAGER_ID);
    CHECK_AND_RETURN_RET_LOG(remoteObj != nullptr, false, "GetSystemAbility Service failed.");

    auto dataShareHelper = DataShare::DataShareHelper::Creator(remoteObj, MEDIALIBRARY_DATA_URI);
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, false, "Connect media library failed");
    MEDIA_INFO_LOG("ConnectMediaLibrary successfully");
    return true;
}

int32_t MediaAssetRdbStore::TryGetRdbStore(bool isIgnoreSELinux)
{
    std::unique_lock<std::mutex> lock(mutex_);
    return OpenRdbStore(isIgnoreSELinux);
}

int32_t MediaAssetRdbStore::OpenRdbStore(bool isIgnoreSELinux)
{
    auto context = AbilityRuntime::Context::GetApplicationContext();
    CHECK_AND_RETURN_RET_LOG(context != nullptr, NativeRdb::E_ERROR, "fail to acquire application Context");
    uid_t uid = getuid() / BASE_USER_RANGE;
    const string key = MEDIA_LIBRARY_STARTUP_PARAM_PREFIX + to_string(uid);
    auto rdbInitFlag = system::GetBoolParameter(key, false);
    bool cond = (!rdbInitFlag && !isIgnoreSELinux);
    if (cond) {
        MEDIA_WARN_LOG("Failed to get rdb init flag");
        cond = !ConnectMediaLibrary();
    }
    CHECK_AND_RETURN_RET_LOG(!cond, NativeRdb::E_ERROR,
        "media library db update not complete, key:%{public}s", key.c_str());

    string name = CONST_MEDIA_DATA_ABILITY_DB_NAME;
    string databaseDir = string(CONST_MEDIA_DB_DIR) + "/rdb";

    int32_t currentAccountId = 0;
#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
    CHECK_AND_RETURN_RET_LOG(ReadKvAccountId(currentAccountId), NativeRdb::E_ERROR,
        "Cannot read published media account");
    if (currentAccountId > 0) {
        databaseDir = string(CONST_MEDIA_DB_DIR) + "/" + std::to_string(currentAccountId) + "/rdb";
    }
#endif
    MEDIA_INFO_LOG("no ipc databaseDir=%{private}s, accountId=%{public}d", databaseDir.c_str(), currentAccountId);

    if (access(databaseDir.c_str(), E_OK) != 0) {
        MEDIA_WARN_LOG("can not get rdb through sandbox");
        return NativeRdb::E_ERROR;
    }
    string dbPath = databaseDir.append("/").append(name);
    int32_t errCode = 0;
    config_.SetName(name);
    config_.SetVisitorDir(dbPath);
    config_.SetBundleName(context->GetBundleName());
    config_.SetArea(context->GetArea());
    config_.SetSecurityLevel(SecurityLevel::S3);
    config_.SetRoleType(RoleType::VISITOR);
    config_.SetReadOnly(true);
    config_.SetScalarFunction("cloud_sync_func", 0, CloudSyncTriggerFunc);
    config_.SetScalarFunction("is_caller_self_func", 0, IsCallerSelfFunc);
    config_.SetScalarFunction("photo_album_notify_func", ARG_COUNT, PhotoAlbumNotifyFunc);
    config_.SetScalarFunction("photo_map_code_func", MAP_CODE_PARAM, PhotoMapCodeFunc);
    config_.SetCollatorLocales("zh_CN");

    // Snapshot the marker before opening to detect changes during the open.
    int64_t markerTime = 0;
    const bool hasMarker = GetReverseRestoreMarkerTime(markerTime);
    MediaLibraryAssetCallBack rdbDataCallBack;
    rdbStore_ = RdbHelper::GetRdbStore(config_, MEDIA_RDB_VERSION, rdbDataCallBack, errCode);
    if (rdbStore_ == nullptr || errCode != NativeRdb::E_OK) {
        MEDIA_WARN_LOG("Failed to get visitor RdbStore, errCode: %{public}d", errCode);
        rdbStore_ = nullptr;
        return errCode;
    }
    openedMarkerTimeMs_ = hasMarker ? markerTime : 0;
#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
    openedAccountId_ = currentAccountId;
#endif
    MEDIA_INFO_LOG("visitor rdb opened, openedMarkerTimeMs=%{public}lld",
        static_cast<long long>(openedMarkerTimeMs_));
    return NativeRdb::E_OK;
}

static void CloseRdbStore(std::shared_ptr<NativeRdb::RdbStore> &store, const NativeRdb::RdbStoreConfig& config,
    int32_t timeOut = MILLIS_PER_SEC)
{
    if (store == nullptr) {
        MEDIA_ERR_LOG("store is nullptr");
        return;
    }
    NativeRdb::RdbStore::ReleaseOption option;
    option.waitTime = std::max(MILLIS_PER_SEC, timeOut);
    option.clearMetadata = false;
    int32_t ret = store->Release(option);
    if (ret == NativeRdb::E_OK) {
        MEDIA_INFO_LOG("CloseRdbStore: Release succeeded");
    } else {
        MEDIA_ERR_LOG("CloseRdbStore: Release failed, ret=%{public}d", ret);
    }
    std::atomic_store(&store, std::shared_ptr<NativeRdb::RdbStore>());
    NativeRdb::RdbHelper::ClearStoreCache(config);
}

std::shared_ptr<NativeRdb::RdbStore> MediaAssetRdbStore::GetRdbStoreForQuery(bool isIgnoreSELinux, int32_t *accountId)
{
    std::unique_lock<std::mutex> lock(mutex_);
#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
    int32_t currentAccountId = -1;
    CHECK_AND_RETURN_RET_LOG(ReadKvAccountId(currentAccountId), nullptr, "Cannot resolve visitor account");
    if (rdbStore_ != nullptr && openedAccountId_ != currentAccountId) {
        // Existing query snapshots may finish; never explicitly close their RDB underneath them.
        rdbStore_.reset();
        NativeRdb::RdbHelper::ClearStoreCache(config_);
    }
#endif
    int64_t markerTime = 0;
    if (rdbStore_ != nullptr && GetReverseRestoreMarkerTime(markerTime) &&
        markerTime != openedMarkerTimeMs_) {
        MEDIA_INFO_LOG("reverse restore marker changed, openedMarkerTimeMs=%{public}lld, markerTimeMs=%{public}lld",
            static_cast<long long>(openedMarkerTimeMs_), static_cast<long long>(markerTime));
        // 5s内链接未释放完毕，也要清理缓存
        CloseRdbStore(rdbStore_, config_, MILLIS_FIVE_SECONDS);
    }
    CHECK_AND_RETURN_RET(rdbStore_ != nullptr || OpenRdbStore(isIgnoreSELinux) == NativeRdb::E_OK, nullptr);
    if (accountId != nullptr) {
#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
        *accountId = openedAccountId_;
#else
        *accountId = -1;
#endif
    }
    return rdbStore_;
}

void AddVirtualColumnsOfDateType(vector<string>& columns)
{
    vector<string> dateTypes = { CONST_MEDIA_DATA_DB_DATE_ADDED, CONST_MEDIA_DATA_DB_DATE_TRASHED,
            CONST_MEDIA_DATA_DB_DATE_MODIFIED, CONST_MEDIA_DATA_DB_DATE_TAKEN };
    vector<string> dateTypeSeconds = { CONST_MEDIA_DATA_DB_DATE_ADDED_TO_SECOND,
            CONST_MEDIA_DATA_DB_DATE_TRASHED_TO_SECOND, CONST_MEDIA_DATA_DB_DATE_MODIFIED_TO_SECOND,
            CONST_MEDIA_DATA_DB_DATE_TAKEN_TO_SECOND };
    for (size_t i = 0; i < dateTypes.size(); i++) {
        auto it = find(columns.begin(), columns.end(), dateTypes[i]);
        if (it != columns.end()) {
            columns.push_back(dateTypeSeconds[i]);
        }
    }
}

void AddQueryIndex(AbsPredicates& predicates, const vector<string>& columns)
{
    auto it = find(columns.begin(), columns.end(), CONST_MEDIA_COLUMN_COUNT);
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
    if (tableName == CONST_MEDIALIBRARY_TABLE) {
        return string(CONST_MEDIALIBRARY_TABLE) + "." + CONST_MEDIA_DATA_DB_SYNC_STATUS + " = " +
            to_string(static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE));
    }
    if (tableName == PhotoColumn::PHOTOS_TABLE) {
        std::string filter = PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_SYNC_STATUS + " = " +
            to_string(static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE)) + " AND " +
            PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_CLEAN_FLAG + " = " +
            to_string(static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN));
#ifdef MEDIALIBRARY_SECURE_ALBUM_ENABLE
        if (OHOS::system::GetParameter(CONST_MEDIA_SECURE_ALBUM, "") == "true") {
            // Check if the caller has MANAGE_RISK_PHOTOS permission
            AccessTokenID tokenCaller = IPCSkeleton::GetCallingTokenID();
            int res = AccessTokenKit::VerifyAccessToken(tokenCaller, MANAGE_RISK_PHOTOS);
            if (res != PermissionState::PERMISSION_GRANTED) {
                filter += " AND " + PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_IS_CRITICAL + " = 0";
                MEDIA_DEBUG_LOG("MANAGE_RISK_PHOTOS permission denied, filter: %{public}s", filter.c_str());
            }
        }
#endif
        MEDIA_DEBUG_LOG("MANAGE_RISK_PHOTOS permission granted, filter: %{public}s", filter.c_str());
        return filter;
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

bool MediaAssetRdbStore::IsQueryGroupPhotoAlbumAssets(const shared_ptr<NativeRdb::RdbStore> &rdbStore,
    const string &albumId)
{
    bool cond = (albumId.empty() || !IsNumber(albumId));
    CHECK_AND_RETURN_RET(!cond, false);
    CHECK_AND_RETURN_RET(rdbStore != nullptr, false);
    RdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    vector<string> columns = {PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumColumns::ALBUM_SUBTYPE};
    auto resultSet = rdbStore->Query(predicates, columns);
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
    CHECK_AND_RETURN_RET(access(CONST_MEDIA_DB_DIR, E_OK) == 0, false);
    auto rdbStore = GetRdbStoreForQuery(isIgnoreSELinux);
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "fail to acquire rdb when query");

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
        CHECK_AND_RETURN_RET(!IsQueryGroupPhotoAlbumAssets(rdbStore, albumId), false);
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
    auto rdbStore = GetRdbStoreForQuery();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, nullptr, "fail to acquire rdb when query");
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
    auto resultSet = rdbStore->QueryByStep(rdbPredicates, columns, false);
    tracer.Finish();
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, nullptr, "fail to acquire result from visitor query");
    return resultSet;
}

std::shared_ptr<NativeRdb::ResultSet> MediaAssetRdbStore::QueryByStep(const std::string &sql)
{
    auto rdbStore = GetRdbStoreForQuery();
    CHECK_AND_RETURN_RET(rdbStore != nullptr, nullptr);
    return rdbStore->QueryByStep(sql);
}

bool MediaAssetRdbStore::IsSupportSharedAssetQuery(Uri& uri, OperationObject& object, bool isIgnoreSELinux)
{
    CHECK_AND_RETURN_RET(access(CONST_MEDIA_DB_DIR, E_OK) == 0, false);
    CHECK_AND_RETURN_RET_LOG(GetRdbStoreForQuery(isIgnoreSELinux) != nullptr, false, "fail to acquire rdb when query");

    OperationType type = GetOprnTypeFromUri(uri);
    CHECK_AND_RETURN_RET(OPERATION_TYPE_SET.count(type) != 0, false);
    object = GetOprnObjectFromUri(uri);
    return true;
}

static int32_t QueryTimeIdBatchInStore(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore,
    int32_t start, int32_t count, std::vector<std::string> &batchKeys)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaAssetRdbStore::QueryTimeIdBatch");
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, NativeRdb::E_DB_NOT_EXIST, "rdbStore is nullptr when query");
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
    auto resultSet = rdbStore->Query(rdbPredicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, NativeRdb::E_ERROR, "fail to acquire result from visitor query");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int columnIndex = 0;
        int64_t dateTakenTime = 0;
        int fileId = 0;
        if (resultSet->GetColumnIndex(MediaColumn::MEDIA_DATE_TAKEN, columnIndex) != NativeRdb::E_OK ||
            resultSet->GetLong(columnIndex, dateTakenTime) != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Fail to get dateTaken");
            resultSet->Close();
            return NativeRdb::E_ERROR;
        }
        if (resultSet->GetColumnIndex(MediaColumn::MEDIA_ID, columnIndex) != NativeRdb::E_OK ||
            resultSet->GetInt(columnIndex, fileId) != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Fail to get fileId");
            resultSet->Close();
            return NativeRdb::E_ERROR;
        }
        std::string timeId;
        CHECK_AND_CONTINUE_ERR_LOG(GenerateKvStoreKey(to_string(fileId),
            to_string(dateTakenTime), timeId),
            "Fail to generate kvStore key, fileId:%{public}d", fileId);
        batchKeys.emplace_back(std::move(timeId));
    }
    resultSet->Close();
    return NativeRdb::E_OK;
}

int32_t MediaAssetRdbStore::QueryTimeIdBatch(int32_t start, int32_t count, std::vector<std::string> &batchKeys)
{
    return QueryTimeIdBatchInStore(GetRdbStoreForQuery(), start, count, batchKeys);
}

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
int32_t MediaAssetRdbStore::QueryTimeIdBatch(int32_t start, int32_t count, std::vector<std::string> &batchKeys,
    int32_t &accountId)
{
    accountId = -1;
    auto store = GetRdbStoreForQuery(false, &accountId);
    return QueryTimeIdBatchInStore(store, start, count, batchKeys);
}

bool MediaAssetRdbStore::GetKvStoreAccountId(int32_t &accountId)
{
    accountId = -1;
    CHECK_AND_RETURN_RET(ReadKvAccountId(accountId), false);
    if (accountId == -1) {
        // Disabled/legacy clients historically read the fixed KV store without opening RDB.
        return true;
    }
    return GetRdbStoreForQuery(false, &accountId) != nullptr;
}
#endif

int32_t MediaLibraryAssetCallBack::OnCreate(NativeRdb::RdbStore& rdbStore)
{
    return 0;
}

int32_t MediaLibraryAssetCallBack::OnUpgrade(NativeRdb::RdbStore& rdbStore, int32_t oldVersion, int32_t newVersion)
{
    return 0;
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS
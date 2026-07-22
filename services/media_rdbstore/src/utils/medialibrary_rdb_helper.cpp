/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaLibraryRdbHelper"

#include "medialibrary_rdb_helper.h"

#include <map>

#include "cloud_sync_helper.h"
#include "media_log.h"
#include "media_smart_album_column.h"
#include "media_string_utils.h"
#include "media_uri_utils.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "rdb_sql_utils.h"
#include "result_set_utils.h"
#include "shooting_mode_column.h"
#include "vision_column.h"

using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
constexpr int32_t MAX_TRY_TIMES = 30;
constexpr int32_t MAX_BUSY_TRY_TIMES = 2;
constexpr int32_t TRANSACTION_WAIT_INTERVAL = 50; // in milliseconds.

static const std::string MODULE_NAME = "com.ohos.medialibrary.medialibrarydata";

int32_t MediaLibraryRdbHelper::ExecSqlWithRetry(std::function<int32_t()> execSql)
{
    if (execSql == nullptr) {
        MEDIA_ERR_LOG("execSql is null");
        return E_ERR;
    }
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
                FileManagement::CloudSync::CloudSyncManager::GetInstance().StopSync(MODULE_NAME);
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

void MediaLibraryRdbHelper::BuildValuesSql(const ValuesBucket &values, std::vector<ValueObject> &bindArgs,
    std::string &sql)
{
    std::map<std::string, ValueObject> valuesMap;
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

void MediaLibraryRdbHelper::BuildQuerySql(const AbsRdbPredicates &predicates, const std::vector<std::string> &columns,
    std::vector<ValueObject> &bindArgs, std::string &sql)
{
    sql.append(RdbSqlUtils::BuildQueryString(predicates, columns));
    const std::vector<std::string> &args = predicates.GetWhereArgs();
    for (const auto &arg : args) {
        bindArgs.emplace_back(arg);
    }
}

void MediaLibraryRdbHelper::ReplacePredicatesUriToId(AbsRdbPredicates &predicates)
{
    const std::vector<std::string> &whereUriArgs = predicates.GetWhereArgs();
    std::vector<std::string> whereIdArgs;
    whereIdArgs.reserve(whereUriArgs.size());
    for (const auto &arg : whereUriArgs) {
        if (!MediaStringUtils::StartsWith(arg, PhotoColumn::PHOTO_URI_PREFIX)) {
            whereIdArgs.push_back(arg);
            continue;
        }
        whereIdArgs.push_back(std::to_string(MediaUriUtils::GetFileId(arg)));
    }

    predicates.SetWhereArgs(whereIdArgs);
}

bool MediaLibraryRdbHelper::HasColumnInTable(RdbStore &store, const std::string &columnName,
    const std::string &tableName)
{
    std::string querySql = std::string("SELECT ") + CONST_MEDIA_COLUMN_COUNT_1 +
        " FROM pragma_table_info('" + tableName + "') WHERE name = '" + columnName + "'";
    auto resultSet = store.QuerySql(querySql);
    bool cond = (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK);
    CHECK_AND_RETURN_RET_LOG(!cond, false, "Get column count failed");

    int32_t count = GetInt32Val(CONST_MEDIA_COLUMN_COUNT_1, resultSet);
    MEDIA_DEBUG_LOG("%{private}s in %{private}s: %{public}d", columnName.c_str(), tableName.c_str(), count);
    return count > 0;
}

void MediaLibraryRdbHelper::AddColumnIfNotExists(
    RdbStore &store, const std::string &columnName, const std::string &columnType, const std::string &tableName)
{
    if (!HasColumnInTable(store, columnName, tableName)) {
        std::string sql = "ALTER TABLE " + tableName + " ADD COLUMN " + columnName + " " + columnType;
        ExecSqlWithRetry([&]() { return store.ExecuteSql(sql); });
    }
}

int32_t MediaLibraryRdbHelper::QueryExistingShootingModeAlbumNames(RdbStore& store,
    std::vector<std::string>& existingAlbumNames)
{
    std::string queryRowSql = "SELECT album_name FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE album_subtype = " + std::to_string(PhotoAlbumSubType::SHOOTING_MODE);
    auto resultSet = store.QuerySql(queryRowSql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_FAIL,
        "Can not get shootingMode album names, resultSet is nullptr");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string albumName = GetStringVal("album_name", resultSet);
        if (!albumName.empty()) {
            existingAlbumNames.push_back(albumName);
        }
    }
    return E_SUCCESS;
}

int32_t MediaLibraryRdbHelper::InsertShootingModeAlbumValues(const std::string& albumName, RdbStore &store)
{
    ValuesBucket valuesBucket;
    valuesBucket.PutInt(SMARTALBUM_DB_ALBUM_TYPE, SHOOTING_MODE_TYPE);
    valuesBucket.PutInt(CONST_COMPAT_ALBUM_SUBTYPE, SHOOTING_MODE_SUB_TYPE);
    valuesBucket.PutString(CONST_MEDIA_DATA_DB_ALBUM_NAME, albumName);
    valuesBucket.PutInt(CONST_MEDIA_DATA_DB_COUNT, 0);
    valuesBucket.PutInt(CONST_MEDIA_DATA_DB_IS_LOCAL, 1); // local album is 1.
    int64_t outRowId = -1;
    int32_t insertResult = ExecSqlWithRetry([&]() {
        return store.InsertWithConflictResolution(outRowId, ANALYSIS_ALBUM_TABLE, valuesBucket,
            ConflictResolution::ON_CONFLICT_REPLACE);
    });
    return insertResult;
}

int32_t MediaLibraryRdbHelper::PrepareShootingModeAlbum(RdbStore &store)
{
    std::vector<std::string> existingAlbumNames;
    if (QueryExistingShootingModeAlbumNames(store, existingAlbumNames) != E_SUCCESS) {
        MEDIA_ERR_LOG("Query existing shootingMode album names failed");
        return NativeRdb::E_ERROR;
    }
    for (int i = static_cast<int>(ShootingModeAlbumType::START);
        i <= static_cast<int>(ShootingModeAlbumType::END); ++i) {
        std::string albumName = std::to_string(i);
        if (find(existingAlbumNames.begin(), existingAlbumNames.end(), albumName) != existingAlbumNames.end()) {
            continue;
        }
        int32_t insertResult = InsertShootingModeAlbumValues(albumName, store);
        if (insertResult != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Prepare shootingMode album failed");
            return insertResult;
        }
    }
    return NativeRdb::E_OK;
}
} // namespace Media
} // namespace OHOS
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

#include "medialibrary_urisensitive_operations.h"

#include "common_func.h"
#include "ipc_skeleton.h"
#include "medialibrary_errno.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_type_const.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_app_uri_sensitive_column.h"
#include "media_column.h"
#include "medialibrary_appstate_observer.h"
#include "medialibrary_rdb_transaction.h"
#include "media_library_manager.h"
#include "permission_utils.h"
#include "result_set_utils.h"
#include "rdb_utils.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using namespace OHOS::RdbDataShareAdapter;

constexpr int32_t NO_DB_OPERATION = -1;
constexpr int32_t UPDATE_DB_OPERATION = 0;
constexpr int32_t INSERT_DB_OPERATION = 1;
constexpr int32_t PHOTOSTYPE = 1;
constexpr int32_t AUDIOSTYPE = 2;

constexpr int32_t FILE_ID_INDEX = 0;
constexpr int32_t URI_TYPE_INDEX = 1;
constexpr int32_t SENSITIVE_TYPE_INDEX = 2;
constexpr int32_t APP_ID_INDEX = 3;

const string DB_OPERATION = "uriSensitive_operation";

int32_t UriSensitiveOperations::UpdateOperation(MediaLibraryCommand &cmd,
    NativeRdb::RdbPredicates &rdbPredicate, std::shared_ptr<TransactionOperations> trans)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("UriSensitive update operation, rdbStore is null.");
        return E_HAS_DB_ERROR;
    }
    cmd.SetTableName(AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    int32_t updateRows;
    if (trans == nullptr) {
        updateRows = MediaLibraryRdbStore::UpdateWithDateTime(cmd.GetValueBucket(), rdbPredicate);
    } else {
        updateRows = trans->Update(cmd.GetValueBucket(), rdbPredicate);
    }
    if (updateRows < 0) {
        MEDIA_ERR_LOG("UriSensitive Update db failed, errCode = %{public}d", updateRows);
        return E_HAS_DB_ERROR;
    }
    return static_cast<int32_t>(updateRows);
}

static void DeleteAllSensitiveOperation(AsyncTaskData *data)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("UriSensitive update operation, rdbStore is null.");
    }

    int32_t ret = rdbStore->ExecuteSql(AppUriSensitiveColumn::CREATE_APP_URI_SENSITIVE_TABLE);
    if (ret < 0) {
        MEDIA_ERR_LOG("UriSensitive table delete all temporary Sensitive failed");
        return;
    }

    ret = rdbStore->ExecuteSql(AppUriSensitiveColumn::CREATE_URI_URITYPE_APPID_INDEX);
    if (ret < 0) {
        MEDIA_ERR_LOG("UriSensitive table delete all temporary Sensitive failed");
        return;
    }

    ret = rdbStore->ExecuteSql(AppUriSensitiveColumn::DELETE_APP_URI_SENSITIVE_TABLE);
    if (ret < 0) {
        MEDIA_ERR_LOG("UriSensitive table delete all temporary Sensitive failed");
        return;
    }
    MEDIA_INFO_LOG("UriSensitive table delete all %{public}d rows temporary Sensitive success", ret);
}

void UriSensitiveOperations::DeleteAllSensitiveAsync()
{
    shared_ptr<MediaLibraryAsyncWorker> asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    if (asyncWorker == nullptr) {
        MEDIA_ERR_LOG("Can not get asyncWorker");
        return;
    }
    shared_ptr<MediaLibraryAsyncTask> notifyAsyncTask =
        make_shared<MediaLibraryAsyncTask>(DeleteAllSensitiveOperation, nullptr);
    asyncWorker->AddTask(notifyAsyncTask, true);
}

int32_t UriSensitiveOperations::DeleteOperation(MediaLibraryCommand &cmd)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("UriSensitive update operation, rdbStore is null.");
        return E_HAS_DB_ERROR;
    }
    cmd.SetTableName(AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    int32_t deleteRows = -1;
    int32_t errCode = rdbStore->Delete(cmd, deleteRows);
    if (errCode != NativeRdb::E_OK || deleteRows < 0) {
        MEDIA_ERR_LOG("UriSensitive delete db failed, errCode = %{public}d", errCode);
        return E_HAS_DB_ERROR;
    }
    return static_cast<int32_t>(deleteRows);
}

int32_t UriSensitiveOperations::InsertOperation(MediaLibraryCommand &cmd)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("UriSensitive insert operation, rdbStore is null.");
        return E_HAS_DB_ERROR;
    }
    cmd.SetTableName(AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    int64_t rowId = -1;
    int32_t errCode = rdbStore->Insert(cmd, rowId);
    if (errCode != NativeRdb::E_OK || rowId < 0) {
        MEDIA_ERR_LOG("UriSensitive insert db failed, errCode = %{public}d", errCode);
        return E_HAS_DB_ERROR;
    }
    return static_cast<int32_t>(rowId);
}

int32_t UriSensitiveOperations::BatchInsertOperation(MediaLibraryCommand &cmd,
    const std::vector<ValuesBucket> &values, std::shared_ptr<TransactionOperations> trans)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("UriSensitive insert operation, rdbStore is null.");
        return E_HAS_DB_ERROR;
    }
    cmd.SetTableName(AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    int64_t outInsertNum = -1;
    int32_t errCode;
    if (trans == nullptr) {
        errCode = rdbStore->BatchInsert(cmd, outInsertNum, values);
    } else {
        errCode = trans->BatchInsert(cmd, outInsertNum, values);
    }
    if (errCode != NativeRdb::E_OK || outInsertNum < 0) {
        MEDIA_ERR_LOG("UriSensitive Insert into db failed, errCode = %{public}d", errCode);
        return E_HAS_DB_ERROR;
    }
    return static_cast<int32_t>(outInsertNum);
}

static void QueryUriSensitive(MediaLibraryCommand &cmd, const std::vector<DataShareValuesBucket> &values,
    std::shared_ptr<OHOS::NativeRdb::ResultSet> &resultSet)
{
    vector<string> columns;
    vector<string> predicateInColumns;
    DataSharePredicates predicates;
    bool isValid;
    string appid = values.at(0).Get(AppUriSensitiveColumn::APP_ID, isValid);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("UriSensitive query operation, rdbStore is null.");
        return;
    }
    cmd.SetTableName(AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    for (const auto &val : values) {
        predicateInColumns.push_back(static_cast<string>(val.Get(AppUriSensitiveColumn::FILE_ID, isValid)));
    }
    predicates.In(AppUriSensitiveColumn::FILE_ID, predicateInColumns);
    predicates.And()->EqualTo(AppUriSensitiveColumn::APP_ID, appid);
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicates, cmd.GetTableName());
    resultSet = MediaLibraryRdbStore::QueryWithFilter(rdbPredicate, columns);
    return;
}

static void GetSingleDbOperation(const vector<DataShareValuesBucket> &values, vector<int32_t> &dbOperation,
    vector<int32_t> &querySingleResultSet, int index)
{
    bool isValid;
    int32_t fileId = std::stoi((static_cast<string>(values.at(index).Get(AppUriSensitiveColumn::FILE_ID, isValid))));
    int32_t uriType = values.at(index).Get(AppUriSensitiveColumn::URI_TYPE, isValid);
    int32_t sensitiveType = values.at(index).Get(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, isValid);
    if ((fileId == querySingleResultSet.at(FILE_ID_INDEX)) && (uriType == querySingleResultSet.at(URI_TYPE_INDEX))) {
        if (sensitiveType == querySingleResultSet.at(SENSITIVE_TYPE_INDEX)) {
            dbOperation[index] = NO_DB_OPERATION;
        } else {
            dbOperation[index] = UPDATE_DB_OPERATION;
        }
    }
}

static void GetAllUriDbOperation(const vector<DataShareValuesBucket> &values, vector<int32_t> &dbOperation,
    std::shared_ptr<OHOS::NativeRdb::ResultSet> &queryResult)
{
    for (const auto &val : values) {
        dbOperation.push_back(INSERT_DB_OPERATION);
    }
    if ((queryResult == nullptr) || (queryResult->GoToFirstRow() != NativeRdb::E_OK)) {
        MEDIA_INFO_LOG("UriSensitive query result is null.");
        return;
    }
    do {
        vector<int32_t> querySingleResultSet;
        querySingleResultSet.push_back(GetInt32Val(AppUriSensitiveColumn::FILE_ID, queryResult));
        querySingleResultSet.push_back(GetInt32Val(AppUriSensitiveColumn::URI_TYPE, queryResult));
        querySingleResultSet.push_back(GetInt32Val(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, queryResult));
        for (size_t i = 0; i < values.size(); i++) {
            GetSingleDbOperation(values, dbOperation, querySingleResultSet, i);
        }
    } while (!queryResult->GoToNextRow());
}

static void BatchUpdate(MediaLibraryCommand &cmd, std::vector<string> inColumn, int32_t tableType,
    const std::vector<DataShareValuesBucket> &values, std::shared_ptr<TransactionOperations> trans)
{
    cmd.SetTableName(AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    bool isValid;
    string appid = values.at(0).Get(AppUriSensitiveColumn::APP_ID, isValid);
    int32_t sensitiveType = values.at(0).Get(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, isValid);
    DataShareValuesBucket valuesBucket;
    DataSharePredicates predicates;
    predicates.In(AppUriSensitiveColumn::FILE_ID, inColumn);
    predicates.EqualTo(AppUriSensitiveColumn::APP_ID, appid);
    predicates.And()->EqualTo(AppUriSensitiveColumn::URI_TYPE, to_string(tableType));
    valuesBucket.Put(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, sensitiveType);
    ValuesBucket value = RdbUtils::ToValuesBucket(valuesBucket);
    if (value.IsEmpty()) {
        MEDIA_ERR_LOG("MediaLibraryDataManager Insert: Input parameter is invalid");
        return;
    }
    cmd.SetValueBucket(value);
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicates, cmd.GetTableName());
    UriSensitiveOperations::UpdateOperation(cmd, rdbPredicate, trans);
}

static void AppstateOberserverBuild(int32_t sensitiveType)
{
    MedialibraryAppStateObserverManager::GetInstance().SubscribeAppState();
}

static int32_t ValueBucketCheck(const std::vector<DataShareValuesBucket> &values)
{
    bool isValidArr[] = {false, false, false, false};
    if (values.empty()) {
        return E_ERR;
    }
    for (const auto &val : values) {
        val.Get(AppUriSensitiveColumn::FILE_ID, isValidArr[FILE_ID_INDEX]);
        val.Get(AppUriSensitiveColumn::URI_TYPE, isValidArr[URI_TYPE_INDEX]);
        val.Get(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, isValidArr[SENSITIVE_TYPE_INDEX]);
        val.Get(AppUriSensitiveColumn::APP_ID, isValidArr[APP_ID_INDEX]);
        for (size_t i = 0; i < sizeof(isValidArr); i++) {
            if ((isValidArr[i]) == false) {
                return E_ERR;
            }
        }
    }
    return E_OK;
}

static void InsertValueBucketPrepare(const std::vector<DataShareValuesBucket> &values, int32_t fileId,
    int32_t uriType, std::vector<ValuesBucket> &batchInsertBucket)
{
    bool isValid;
    ValuesBucket insertValues;
    string appid = values.at(0).Get(AppUriSensitiveColumn::APP_ID, isValid);
    int32_t sensitiveType = values.at(0).Get(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, isValid);
    insertValues.Put(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, sensitiveType);
    insertValues.Put(AppUriSensitiveColumn::FILE_ID, fileId);
    insertValues.Put(AppUriSensitiveColumn::APP_ID, appid);
    insertValues.Put(AppUriSensitiveColumn::URI_TYPE, uriType);
    insertValues.Put(AppUriSensitiveColumn::DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
    batchInsertBucket.push_back(insertValues);
}

int32_t UriSensitiveOperations::GrantUriSensitive(MediaLibraryCommand &cmd,
    const std::vector<DataShareValuesBucket> &values)
{
    std::vector<string> photosValues;
    std::vector<string> audiosValues;
    std::vector<int32_t> dbOperation;
    std::shared_ptr<OHOS::NativeRdb::ResultSet> resultSet;
    std::vector<ValuesBucket>  batchInsertBucket;
    bool photoNeedToUpdate = false;
    bool audioNeedToUpdate = false;
    bool needToInsert = false;
    bool isValid = false;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>();
    int32_t err = trans->Start(__func__);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("transaction failed :%{public}d", err);
        return err;
    }
    if (ValueBucketCheck(values) != E_OK) {
        return E_ERR;
    }
    string appid = values.at(0).Get(AppUriSensitiveColumn::APP_ID, isValid);
    int32_t sensitiveType = values.at(0).Get(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, isValid);
    AppstateOberserverBuild(sensitiveType);
    QueryUriSensitive(cmd, values, resultSet);
    GetAllUriDbOperation(values, dbOperation, resultSet);
    for (size_t i = 0; i < values.size(); i++) {
        int32_t fileId = std::stoi((static_cast<string>(values.at(i).Get(AppUriSensitiveColumn::FILE_ID, isValid))));
        int32_t uriType = values.at(i).Get(AppUriSensitiveColumn::URI_TYPE, isValid);
        if ((dbOperation.at(i) == UPDATE_DB_OPERATION) && (uriType == PHOTOSTYPE)) {
            photoNeedToUpdate = true;
            photosValues.push_back(static_cast<string>(values.at(i).Get(AppUriSensitiveColumn::FILE_ID, isValid)));
        } else if ((dbOperation.at(i) == UPDATE_DB_OPERATION) && (uriType == AUDIOSTYPE)) {
            audioNeedToUpdate = true;
            audiosValues.push_back(static_cast<string>(values.at(i).Get(AppUriSensitiveColumn::FILE_ID, isValid)));
        } else if (dbOperation.at(i) == INSERT_DB_OPERATION) {
            needToInsert = true;
            InsertValueBucketPrepare(values, fileId, uriType, batchInsertBucket);
        }
    }
    if (photoNeedToUpdate) {
        BatchUpdate(cmd, photosValues, PHOTOSTYPE, values, trans);
    }
    if (audioNeedToUpdate) {
        BatchUpdate(cmd, audiosValues, AUDIOSTYPE, values, trans);
    }
    if (needToInsert) {
        UriSensitiveOperations::BatchInsertOperation(cmd, batchInsertBucket, trans);
    }
    err = trans->Finish();
    if (err != E_OK) {
        MEDIA_ERR_LOG("GrantUriSensitive: tans finish fail!, ret:%{public}d", err);
    }
    return E_OK;
}

int32_t UriSensitiveOperations::QuerySensitiveType(const std::string &appId, const std::string &fileId)
{
    NativeRdb::RdbPredicates rdbPredicate(AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    rdbPredicate.And()->EqualTo(AppUriSensitiveColumn::APP_ID, appId);
    rdbPredicate.And()->EqualTo(AppUriSensitiveColumn::FILE_ID, fileId);

    vector<string> columns;
    columns.push_back(AppUriSensitiveColumn::ID);
    columns.push_back(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE);

    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(rdbPredicate, columns);
    if (resultSet == nullptr) {
        return 0;
    }

    int32_t numRows = 0;
    resultSet->GetRowCount(numRows);
    if (numRows == 0) {
        return 0;
    }
    resultSet->GoToFirstRow();
    return MediaLibraryRdbStore::GetInt(resultSet, AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE);
}
std::string UriSensitiveOperations::QueryAppId(const std::string &fileId)
{
    NativeRdb::RdbPredicates rdbPredicate(PhotoColumn::PHOTOS_TABLE);
    rdbPredicate.And()->EqualTo(MediaColumn::MEDIA_ID, fileId);

    vector<string> columns;
    columns.push_back(MediaColumn::MEDIA_ID);
    columns.push_back(MediaColumn::MEDIA_OWNER_APPID);

    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(rdbPredicate, columns);
    if (resultSet == nullptr) {
        return 0;
    }

    int32_t numRows = 0;
    resultSet->GetRowCount(numRows);
    if (numRows == 0) {
        return 0;
    }
    resultSet->GoToFirstRow();
    return MediaLibraryRdbStore::GetString(resultSet, MediaColumn::MEDIA_OWNER_APPID);
}
}
}
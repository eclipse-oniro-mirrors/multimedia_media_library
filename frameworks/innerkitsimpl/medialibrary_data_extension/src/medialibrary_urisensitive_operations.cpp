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

#include "medialibrary_object_utils.h"
#include "media_file_utils.h"
#include "media_app_uri_permission_column.h"
#include "media_app_uri_sensitive_column.h"
#include "result_set_utils.h"
#include "medialibrary_appstate_observer.h"
#include "medialibrary_data_manager_utils.h"

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
constexpr int32_t SRC_TOKEN_ID_INDEX = 3;
constexpr int32_t TARGET_TOKEN_ID_INDEX = 4;

const string DB_OPERATION = "uriSensitive_operation";

int32_t UriSensitiveOperations::UpdateOperation(MediaLibraryCommand &cmd,
    NativeRdb::RdbPredicates &rdbPredicate, std::shared_ptr<TransactionOperations> trans)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "UriSensitive update operation, rdbStore is null.");
    cmd.SetTableName(AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    int32_t updateRows;
    if (trans == nullptr) {
        updateRows = MediaLibraryRdbStore::UpdateWithDateTime(cmd.GetValueBucket(), rdbPredicate);
    } else {
        updateRows = trans->Update(cmd.GetValueBucket(), rdbPredicate);
    }
    CHECK_AND_RETURN_RET_LOG(updateRows >= 0, E_HAS_DB_ERROR,
        "UriSensitive Update db failed, errCode = %{public}d", updateRows);
    return static_cast<int32_t>(updateRows);
}

static void DeleteAllSensitiveOperation(AsyncTaskData *data)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "UriSensitive delete operation fail, rdbStore is null.");

    int32_t ret = rdbStore->ExecuteSql(AppUriSensitiveColumn::CREATE_APP_URI_SENSITIVE_TABLE);
    CHECK_AND_RETURN_LOG(ret >= 0, "UriSensitive table delete all temporary Sensitive failed");

    ret = rdbStore->ExecuteSql(AppUriSensitiveColumn::CREATE_URI_URITYPE_APPID_INDEX);
    CHECK_AND_RETURN_LOG(ret >= 0, "UriSensitive table delete all temporary Sensitive failed");

    ret = rdbStore->ExecuteSql(AppUriSensitiveColumn::DELETE_APP_URI_SENSITIVE_TABLE);
    CHECK_AND_RETURN_LOG(ret >= 0, "UriSensitive table delete all temporary Sensitive failed");

    MEDIA_INFO_LOG("UriSensitive table delete all %{public}d rows temporary Sensitive success", ret);
}

void UriSensitiveOperations::DeleteAllSensitiveAsync()
{
    shared_ptr<MediaLibraryAsyncWorker> asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    CHECK_AND_RETURN_LOG(asyncWorker != nullptr, "Can not get asyncWorker");
    shared_ptr<MediaLibraryAsyncTask> notifyAsyncTask =
        make_shared<MediaLibraryAsyncTask>(DeleteAllSensitiveOperation, nullptr);
    asyncWorker->AddTask(notifyAsyncTask, true);
}

int32_t UriSensitiveOperations::DeleteOperation(MediaLibraryCommand &cmd)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "UriSensitive update operation, rdbStore is null.");
    cmd.SetTableName(AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    int32_t deleteRows = -1;
    int32_t errCode = rdbStore->Delete(cmd, deleteRows);
    bool cond = (errCode != NativeRdb::E_OK || deleteRows < 0);
    CHECK_AND_RETURN_RET_LOG(!cond, E_HAS_DB_ERROR, "UriSensitive delete db failed, errCode = %{public}d", errCode);
    return static_cast<int32_t>(deleteRows);
}

int32_t UriSensitiveOperations::InsertOperation(MediaLibraryCommand &cmd)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "UriSensitive insert operation, rdbStore is null.");
    cmd.SetTableName(AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    int64_t rowId = -1;
    int32_t errCode = rdbStore->Insert(cmd, rowId);
    bool cond = (errCode != NativeRdb::E_OK || rowId < 0);
    CHECK_AND_RETURN_RET_LOG(!cond, E_HAS_DB_ERROR, "UriSensitive insert db failed, errCode = %{public}d", errCode);
    return static_cast<int32_t>(rowId);
}

int32_t UriSensitiveOperations::BatchInsertOperation(MediaLibraryCommand &cmd,
    std::vector<ValuesBucket> &values, std::shared_ptr<TransactionOperations> trans)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "UriSensitive insert operation, rdbStore is null.");

    cmd.SetTableName(AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    int64_t outInsertNum = -1;
    int32_t errCode;
    if (trans == nullptr) {
        errCode = rdbStore->BatchInsert(cmd, outInsertNum, values);
    } else {
        errCode = trans->BatchInsert(cmd, outInsertNum, values);
    }
    bool cond = (errCode != NativeRdb::E_OK || outInsertNum < 0);
    CHECK_AND_RETURN_RET_LOG(!cond, E_HAS_DB_ERROR,
        "UriSensitive Insert into db failed, errCode = %{public}d", errCode);
    return static_cast<int32_t>(outInsertNum);
}

static void QueryUriSensitive(MediaLibraryCommand &cmd, const std::vector<DataShareValuesBucket> &values,
    std::shared_ptr<OHOS::NativeRdb::ResultSet> &resultSet)
{
    vector<string> columns;
    vector<string> predicateInColumns;
    DataSharePredicates predicates;
    bool isValid;
    int64_t targetTokenId = values.at(0).Get(AppUriSensitiveColumn::TARGET_TOKENID, isValid);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "UriSensitive query operation, rdbStore is null.");

    cmd.SetTableName(AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    for (const auto &val : values) {
        predicateInColumns.push_back(static_cast<string>(val.Get(AppUriSensitiveColumn::FILE_ID, isValid)));
    }
    predicates.In(AppUriSensitiveColumn::FILE_ID, predicateInColumns);
    predicates.And()->EqualTo(AppUriSensitiveColumn::TARGET_TOKENID, (int64_t)targetTokenId);
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicates, cmd.GetTableName());
    resultSet = MediaLibraryRdbStore::QueryWithFilter(rdbPredicate, columns);
    return;
}

static bool CanConvertToInt32(const std::string &str)
{
    std::istringstream iss(str);
    int32_t num = 0;
    iss >> num;
    return iss.eof() && !iss.fail();
}

static int32_t GetFileId(const DataShareValuesBucket &values, bool &isValid)
{
    int32_t ret = E_ERR;
    string fileIdStr = static_cast<string>(values.Get(AppUriSensitiveColumn::FILE_ID, isValid));
    if (CanConvertToInt32(fileIdStr) && MediaLibraryDataManagerUtils::IsNumber(fileIdStr)) {
        ret = static_cast<int32_t>(std::stoi(fileIdStr));
    }
    return ret;
}

static void GetSingleDbOperation(const vector<DataShareValuesBucket> &values, vector<int32_t> &dbOperation,
    vector<int32_t> &querySingleResultSet, int index)
{
    bool isValid;
    int32_t fileId = GetFileId(values.at(index), isValid);
    CHECK_AND_RETURN_LOG(fileId != E_ERR, "Failed GetFileId");

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
    bool cond = ((queryResult == nullptr) || (queryResult->GoToFirstRow() != NativeRdb::E_OK));
    CHECK_AND_RETURN_INFO_LOG(!cond, "UriSensitive query result is null.");

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
    int64_t targetTokenId = values.at(0).Get(AppUriSensitiveColumn::TARGET_TOKENID, isValid);
    int64_t srcTokenId = values.at(0).Get(AppUriSensitiveColumn::SOURCE_TOKENID, isValid);
    int32_t sensitiveType = values.at(0).Get(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, isValid);
    DataShareValuesBucket valuesBucket;
    DataSharePredicates predicates;
    predicates.In(AppUriSensitiveColumn::FILE_ID, inColumn);
    predicates.EqualTo(AppUriSensitiveColumn::TARGET_TOKENID, (int64_t)targetTokenId);
    predicates.EqualTo(AppUriSensitiveColumn::SOURCE_TOKENID, (int64_t)srcTokenId);
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

static void AppstateOberserverBuild(int64_t destTokenId)
{
    MedialibraryAppStateObserverManager::GetInstance().SubscribeAppState();
    MedialibraryAppStateObserverManager::GetInstance().AddTokenId(destTokenId, false);
}

static int32_t ValueBucketCheck(const std::vector<DataShareValuesBucket> &values)
{
    bool isValidArr[] = {false, false, false, false, false};
    if (values.empty()) {
        return E_ERR;
    }
    for (const auto &val : values) {
        val.Get(AppUriSensitiveColumn::FILE_ID, isValidArr[FILE_ID_INDEX]);
        val.Get(AppUriSensitiveColumn::URI_TYPE, isValidArr[URI_TYPE_INDEX]);
        val.Get(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, isValidArr[SENSITIVE_TYPE_INDEX]);
        val.Get(AppUriSensitiveColumn::SOURCE_TOKENID, isValidArr[SRC_TOKEN_ID_INDEX]);
        val.Get(AppUriSensitiveColumn::TARGET_TOKENID, isValidArr[TARGET_TOKEN_ID_INDEX]);
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
    int64_t targetTokenId = values.at(0).Get(AppUriSensitiveColumn::TARGET_TOKENID, isValid);
    int32_t sensitiveType = values.at(0).Get(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, isValid);
    int64_t srcTokenId = values.at(0).Get(AppUriSensitiveColumn::SOURCE_TOKENID, isValid);
    insertValues.Put(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, sensitiveType);
    insertValues.Put(AppUriSensitiveColumn::FILE_ID, fileId);
    insertValues.Put(AppUriSensitiveColumn::TARGET_TOKENID, (int64_t)targetTokenId);
    insertValues.Put(AppUriSensitiveColumn::URI_TYPE, uriType);
    insertValues.Put(AppUriSensitiveColumn::SOURCE_TOKENID, (int64_t)srcTokenId);
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
    std::vector<ValuesBucket> batchInsertBucket;
    bool photoNeedToUpdate = false;
    bool audioNeedToUpdate = false;
    bool needToInsert = false;
    bool isValid = false;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "Failed to get rdbStore.");
    std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>(__func__);
    int32_t err = E_OK;
    std::function<int(void)> func = [&]()->int {
        if (ValueBucketCheck(values) != E_OK) {
            return E_ERR;
        }
        int32_t sensitiveType = values.at(0).Get(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, isValid);
        int64_t targetTokenId = values.at(0).Get(AppUriSensitiveColumn::TARGET_TOKENID, isValid);
        AppstateOberserverBuild(targetTokenId);
        QueryUriSensitive(cmd, values, resultSet);
        GetAllUriDbOperation(values, dbOperation, resultSet);
        for (size_t i = 0; i < values.size(); i++) {
            int32_t fileId = GetFileId(values.at(i), isValid);
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
        return err;
    };
    err = trans->RetryTrans(func);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "GrantUriSensitive: tans finish fail!, ret:%{public}d", err);
    return E_OK;
}

static bool IsOwnerPriviledge(const uint32_t &tokenId, const std::string &fileId)
{
    NativeRdb::RdbPredicates rdbPredicate(AppUriPermissionColumn::APP_URI_PERMISSION_TABLE);
    rdbPredicate.EqualTo(AppUriPermissionColumn::TARGET_TOKENID, (int64_t)tokenId);
    rdbPredicate.EqualTo(AppUriPermissionColumn::FILE_ID, fileId);
    rdbPredicate.EqualTo(AppUriPermissionColumn::PERMISSION_TYPE,
        AppUriPermissionColumn::PERMISSION_PERSIST_READ_WRITE);
    vector<string> columns;
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(rdbPredicate, columns);
    CHECK_AND_RETURN_RET(resultSet != nullptr, false);
    int32_t numRows = 0;
    resultSet->GetRowCount(numRows);
    return numRows > 0;
}

int32_t UriSensitiveOperations::QuerySensitiveType(const uint32_t &tokenId, const std::string &fileId)
{
    // OwnerPriviledge donot need anonymize
    if (IsOwnerPriviledge(tokenId, fileId)) {
        return AppUriSensitiveColumn::SENSITIVE_NO_DESENSITIZE;
    }

    NativeRdb::RdbPredicates rdbPredicate(AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    rdbPredicate.BeginWrap();
    rdbPredicate.And()->EqualTo(AppUriSensitiveColumn::TARGET_TOKENID, (int64_t)tokenId);
    rdbPredicate.EndWrap();
    rdbPredicate.And()->EqualTo(AppUriSensitiveColumn::FILE_ID, fileId);

    vector<string> columns;
    columns.push_back(AppUriSensitiveColumn::ID);
    columns.push_back(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE);

    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(rdbPredicate, columns);
    CHECK_AND_RETURN_RET(resultSet != nullptr, 0);

    int32_t numRows = 0;
    resultSet->GetRowCount(numRows);
    if (numRows == 0) {
        // Refer MEDIA_LOCATION permission to hide location and shooting parameters.
        return AppUriSensitiveColumn::SENSITIVE_DEFAULT;
    }
    resultSet->GoToFirstRow();
    return MediaLibraryRdbStore::GetInt(resultSet, AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE);
}

bool UriSensitiveOperations::QueryForceSensitive(const uint32_t &tokenId,
    const std::string &fileId)
{
    NativeRdb::RdbPredicates rdbPredicate(AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    rdbPredicate.BeginWrap();
    rdbPredicate.And()->EqualTo(AppUriSensitiveColumn::TARGET_TOKENID, (int64_t)tokenId);
    rdbPredicate.EndWrap();
    rdbPredicate.And()->EqualTo(AppUriSensitiveColumn::FILE_ID, fileId);

    vector<string> columns;
    columns.push_back(AppUriSensitiveColumn::ID);
    columns.push_back(AppUriSensitiveColumn::IS_FORCE_SENSITIVE);

    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    CHECK_AND_RETURN_RET(resultSet != nullptr, false);

    int32_t numRows = 0;
    resultSet->GetRowCount(numRows);
    if (numRows == 0) {
        return false;
    }
    resultSet->GoToFirstRow();
    return MediaLibraryRdbStore::GetInt(resultSet, AppUriSensitiveColumn::IS_FORCE_SENSITIVE) > 0;
}
}
}
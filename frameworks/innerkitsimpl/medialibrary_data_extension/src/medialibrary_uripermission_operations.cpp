/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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

#include "medialibrary_uripermission_operations.h"

#include <iostream>
#include <sstream>
#include <string>
#include <cstdint>

#include "common_func.h"
#include "ipc_skeleton.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_type_const.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_app_uri_permission_column.h"
#include "medialibrary_appstate_observer.h"
#include "medialibrary_rdb_transaction.h"
#include "permission_utils.h"
#include "result_set_utils.h"
#include "rdb_utils.h"
#include "userfilemgr_uri.h"
#include "media_file_uri.h"
#include "data_secondary_directory_uri.h"
#include "media_audio_column.h"

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
constexpr int32_t PERMISSION_TYPE_INDEX = 2;
constexpr int32_t SRC_TOKEN_ID_INDEX = 3;
constexpr int32_t TARGET_TOKEN_ID_INDEX = 4;

const string DB_OPERATION = "uriPermission_operation";

static bool CheckMode(string& mode)
{
    transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
    if (MEDIA_OPEN_MODES.find(mode) == MEDIA_OPEN_MODES.end()) {
        MEDIA_ERR_LOG("mode format is error: %{private}s", mode.c_str());
        return false;
    }
    string tempMode;
    if (mode.find(MEDIA_FILEMODE_READONLY) != string::npos) {
        tempMode += MEDIA_FILEMODE_READONLY;
    }
    if (mode.find(MEDIA_FILEMODE_WRITEONLY) != string::npos) {
        tempMode += MEDIA_FILEMODE_WRITEONLY;
    }
    mode = tempMode;
    return true;
}

int32_t UriPermissionOperations::UpdateOperation(MediaLibraryCommand &cmd,
    std::shared_ptr<TransactionOperations> trans)
{
    cmd.SetTableName(AppUriPermissionColumn::APP_URI_PERMISSION_TABLE);
    int32_t updateRows = -1;
    int errCode;
    if (trans == nullptr) {
        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr,
            E_HAS_DB_ERROR, "UriPermission update operation, rdbStore is null.");
        errCode = rdbStore->Update(cmd, updateRows);
    } else {
        errCode = trans->Update(cmd, updateRows);
    }
    bool cond = (errCode != NativeRdb::E_OK || updateRows < 0);
    CHECK_AND_RETURN_RET_LOG(!cond, E_HAS_DB_ERROR, "UriPermission Update db failed, errCode = %{public}d", errCode);
    return updateRows;
}

static void DeleteAllTemporaryOperation(AsyncTaskData *data)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("UriPermission update operation, rdbStore is null.");
        return;
    }
    NativeRdb::RdbPredicates rdbPredicate(AppUriPermissionColumn::APP_URI_PERMISSION_TABLE);
    vector<string> permissionTypes;
    permissionTypes.emplace_back(to_string(static_cast<int32_t>(PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO)));
    permissionTypes.emplace_back(to_string(static_cast<int32_t>(PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO)));
    permissionTypes.emplace_back(to_string(static_cast<int32_t>(PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO)));
    rdbPredicate.And()->In(AppUriPermissionColumn::PERMISSION_TYPE, permissionTypes);
    int32_t ret = rdbStore->Delete(rdbPredicate);
    CHECK_AND_RETURN_LOG(ret >= 0, "UriPermission table delete all temporary permission failed");
    MEDIA_INFO_LOG("UriPermission table delete all %{public}d rows temporary permission success", ret);
}

void UriPermissionOperations::DeleteAllTemporaryAsync()
{
    shared_ptr<MediaLibraryAsyncWorker> asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    if (asyncWorker == nullptr) {
        MEDIA_ERR_LOG("Can not get asyncWorker");
        return;
    }
    shared_ptr<MediaLibraryAsyncTask> notifyAsyncTask =
        make_shared<MediaLibraryAsyncTask>(DeleteAllTemporaryOperation, nullptr);
    if (notifyAsyncTask != nullptr) {
        asyncWorker->AddTask(notifyAsyncTask, false);
    } else {
        MEDIA_ERR_LOG("Failed to create async task for UriPermission");
    }
}

int32_t UriPermissionOperations::DeleteOperation(MediaLibraryCommand &cmd)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "UriPermission update operation, rdbStore is null.");
    cmd.SetTableName(AppUriPermissionColumn::APP_URI_PERMISSION_TABLE);
    int32_t deleteRows = -1;
    int32_t errCode = rdbStore->Delete(cmd, deleteRows);
    if (errCode != NativeRdb::E_OK || deleteRows < 0) {
        MEDIA_ERR_LOG("UriPermission delete db failed, errCode = %{public}d", errCode);
        return E_HAS_DB_ERROR;
    }
    return static_cast<int32_t>(deleteRows);
}

int32_t UriPermissionOperations::InsertOperation(MediaLibraryCommand &cmd)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "UriPermission insert operation, rdbStore is null.");

    cmd.SetTableName(AppUriPermissionColumn::APP_URI_PERMISSION_TABLE);
    int64_t rowId = -1;
    int32_t errCode = rdbStore->Insert(cmd, rowId);
    bool cond = (errCode != NativeRdb::E_OK || rowId < 0);
    CHECK_AND_RETURN_RET_LOG(!cond, E_HAS_DB_ERROR, "UriPermission insert db failed, errCode = %{public}d", errCode);
    return static_cast<int32_t>(rowId);
}

int32_t UriPermissionOperations::BatchInsertOperation(MediaLibraryCommand &cmd,
    std::vector<ValuesBucket> &values, std::shared_ptr<TransactionOperations> trans)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "UriPermission insert operation, rdbStore is null.");

    cmd.SetTableName(AppUriPermissionColumn::APP_URI_PERMISSION_TABLE);
    int64_t outInsertNum = -1;
    int32_t errCode;
    if (trans == nullptr) {
        errCode = rdbStore->BatchInsert(cmd, outInsertNum, values);
    } else {
        errCode = trans->BatchInsert(cmd, outInsertNum, values);
    }
    bool cond = (errCode != NativeRdb::E_OK || outInsertNum < 0);
    CHECK_AND_RETURN_RET_LOG(!cond, E_HAS_DB_ERROR,
        "UriPermission Insert into db failed, errCode = %{public}d", errCode);
    return static_cast<int32_t>(outInsertNum);
}

static void QueryUriPermission(MediaLibraryCommand &cmd, const std::vector<DataShareValuesBucket> &values,
    std::shared_ptr<OHOS::NativeRdb::ResultSet> &resultSet)
{
    vector<string> columns;
    vector<string> predicateInColumns;
    DataSharePredicates predicates;
    bool isValid;
    int64_t targetTokenId = values.at(0).Get(AppUriPermissionColumn::TARGET_TOKENID, isValid);
    int64_t srcTokenId = values.at(0).Get(AppUriPermissionColumn::SOURCE_TOKENID, isValid);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("UriPermission query operation, rdbStore is null.");
        return;
    }
    cmd.SetTableName(AppUriPermissionColumn::APP_URI_PERMISSION_TABLE);
    for (const auto &val : values) {
        predicateInColumns.push_back(static_cast<string>(val.Get(AppUriPermissionColumn::FILE_ID, isValid)));
    }
    predicates.In(AppUriPermissionColumn::FILE_ID, predicateInColumns);
    predicates.And()->EqualTo(AppUriPermissionColumn::TARGET_TOKENID, (int64_t)targetTokenId);
    predicates.And()->EqualTo(AppUriPermissionColumn::SOURCE_TOKENID, (int64_t)srcTokenId);
    cmd.SetDataSharePred(predicates);
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicates, cmd.GetTableName());
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    resultSet = rdbStore->Query(cmd, columns);
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
    string fileIdStr = static_cast<string>(values.Get(AppUriPermissionColumn::FILE_ID, isValid));
    if (CanConvertToInt32(fileIdStr)) {
        ret = static_cast<int32_t>(std::stoi(fileIdStr));
    }
    return ret;
}

static void GetSingleDbOperation(const vector<DataShareValuesBucket> &values, vector<int32_t> &dbOperation,
    vector<int32_t> &querySingleResultSet, int index)
{
    bool isValid;
    int32_t fileId = GetFileId(values.at(index), isValid);
    if (fileId == E_ERR) {
        MEDIA_ERR_LOG("Failed GetFileId");
        return;
    }
    int32_t uriType = values.at(index).Get(AppUriPermissionColumn::URI_TYPE, isValid);
    int32_t permissionType = values.at(index).Get(AppUriPermissionColumn::PERMISSION_TYPE, isValid);
    if ((fileId == querySingleResultSet.at(FILE_ID_INDEX)) && (uriType == querySingleResultSet.at(URI_TYPE_INDEX))
        && (permissionType == querySingleResultSet.at(PERMISSION_TYPE_INDEX))) {
        dbOperation[index] = NO_DB_OPERATION;
    }
}

static void GetMediafileQueryResult(const vector<string>& predicateInColumns, OperationObject object,
    std::vector<int32_t>& fileIdList)
{
    MediaLibraryCommand queryCmd(object, OperationType::QUERY);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("UriPermission query operation, rdbStore is null.");
        return;
    }
    vector<string> columns;
    queryCmd.GetAbsRdbPredicates()->In(AppUriPermissionColumn::FILE_ID, predicateInColumns);
    auto queryResult = rdbStore->Query(queryCmd, columns);
    if ((queryResult == nullptr) || (queryResult->GoToFirstRow() != NativeRdb::E_OK)) {
        MEDIA_INFO_LOG("UriPermission query result is null.");
        return;
    }
    do {
        fileIdList.push_back(GetInt32Val(AppUriPermissionColumn::FILE_ID, queryResult));
    } while (!queryResult->GoToNextRow());
}

static void FilterNotExistUri(const std::vector<DataShareValuesBucket> &values, vector<int32_t>& dbOperation)
{
    vector<int32_t> photoFileIdList;
    vector<int32_t> audioFileIdList;
    vector<string> photosColumns;
    vector<string> audioColumns;
    bool isValid;
    for (const auto &val : values) {
        if (static_cast<int32_t>(val.Get(AppUriPermissionColumn::URI_TYPE, isValid)) == PHOTOSTYPE) {
            photosColumns.push_back(val.Get(AppUriPermissionColumn::FILE_ID, isValid));
        } else if (static_cast<int32_t>(val.Get(AppUriPermissionColumn::URI_TYPE, isValid)) == AUDIOSTYPE) {
            audioColumns.push_back(val.Get(AppUriPermissionColumn::FILE_ID, isValid));
        }
    }
    if (!photosColumns.empty()) {
        GetMediafileQueryResult(photosColumns, OperationObject::FILESYSTEM_PHOTO, photoFileIdList);
    }
    if (!audioColumns.empty()) {
        GetMediafileQueryResult(audioColumns, OperationObject::FILESYSTEM_AUDIO, audioFileIdList);
    }
    for (size_t i = 0; i < values.size(); i++) {
        int32_t fileId = GetFileId(values.at(i), isValid);
        int32_t uriType = values[i].Get(AppUriPermissionColumn::URI_TYPE, isValid);
        if (uriType == PHOTOSTYPE) {
            auto notExistIt = std::find(photoFileIdList.begin(), photoFileIdList.end(), fileId);
            if (notExistIt == photoFileIdList.end()) {
                dbOperation[i] = NO_DB_OPERATION;
            }
        } else if (uriType == AUDIOSTYPE) {
            auto it = std::find(audioFileIdList.begin(), audioFileIdList.end(), fileId);
            if (it == audioFileIdList.end()) {
                dbOperation[i] = NO_DB_OPERATION;
            }
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
        MEDIA_INFO_LOG("UriPermission query result is null.");
        return;
    }
    do {
        vector<int32_t> querySingleResultSet;
        querySingleResultSet.push_back(GetInt32Val(AppUriPermissionColumn::FILE_ID, queryResult));
        querySingleResultSet.push_back(GetInt32Val(AppUriPermissionColumn::URI_TYPE, queryResult));
        querySingleResultSet.push_back(GetInt32Val(AppUriPermissionColumn::PERMISSION_TYPE, queryResult));
        for (size_t i = 0; i < values.size(); i++) {
            GetSingleDbOperation(values, dbOperation, querySingleResultSet, i);
        }
    } while (!queryResult->GoToNextRow());
}

static int32_t ValueBucketCheck(const std::vector<DataShareValuesBucket> &values)
{
    bool isValidArr[] = {false, false, false, false, false};
    if (values.empty()) {
        return E_ERR;
    }
    for (const auto &val : values) {
        val.Get(AppUriPermissionColumn::FILE_ID, isValidArr[FILE_ID_INDEX]);
        val.Get(AppUriPermissionColumn::URI_TYPE, isValidArr[URI_TYPE_INDEX]);
        val.Get(AppUriPermissionColumn::PERMISSION_TYPE, isValidArr[PERMISSION_TYPE_INDEX]);
        val.Get(AppUriPermissionColumn::SOURCE_TOKENID, isValidArr[SRC_TOKEN_ID_INDEX]);
        val.Get(AppUriPermissionColumn::TARGET_TOKENID, isValidArr[TARGET_TOKEN_ID_INDEX]);
        for (size_t i = 0; i < sizeof(isValidArr); i++) {
            if ((isValidArr[i]) == false) {
                return E_ERR;
            }
        }
    }
    return E_OK;
}

static void InsertValueBucketPrepare(const std::vector<DataShareValuesBucket> &values, int32_t fileId,
    int32_t uriType, int32_t permissionType, std::vector<ValuesBucket> &batchInsertBucket)
{
    bool isValid;
    ValuesBucket insertValues;
    int64_t srcTokenId = values.at(0).Get(AppUriPermissionColumn::SOURCE_TOKENID, isValid);
    int64_t targetTokenId = values.at(0).Get(AppUriPermissionColumn::TARGET_TOKENID, isValid);
    insertValues.Put(AppUriPermissionColumn::PERMISSION_TYPE, permissionType);
    insertValues.Put(AppUriPermissionColumn::FILE_ID, fileId);
    insertValues.Put(AppUriPermissionColumn::TARGET_TOKENID, (int64_t)targetTokenId);
    insertValues.Put(AppUriPermissionColumn::SOURCE_TOKENID, (int64_t)srcTokenId);
    insertValues.Put(AppUriPermissionColumn::URI_TYPE, uriType);
    insertValues.Put(AppUriPermissionColumn::DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
    batchInsertBucket.push_back(insertValues);
}

// SubscribeAppState && add tokenid to cache
static void DoSubscribeForAppStop(const std::vector<DataShare::DataShareValuesBucket> &values)
{
    if (values.size() == 0) {
        MEDIA_WARN_LOG("values is empty");
        return;
    }
    auto it = values.begin();
    ValuesBucket valueBucket = RdbUtils::ToValuesBucket(*it);
    int64_t destTokenId = -1;
    ValueObject valueObject;
    if (valueBucket.GetObject(AppUriPermissionColumn::TARGET_TOKENID, valueObject)) {
        valueObject.GetLong(destTokenId);
    }
    int permissionTypeParam = -1;
    if (valueBucket.GetObject(AppUriPermissionColumn::PERMISSION_TYPE, valueObject)) {
        valueObject.GetInt(permissionTypeParam);
    }
    if (AppUriPermissionColumn::PERMISSION_TYPES_TEMPORARY.find(permissionTypeParam) !=
        AppUriPermissionColumn::PERMISSION_TYPES_TEMPORARY.end()) {
        MedialibraryAppStateObserverManager::GetInstance().SubscribeAppState();
        MedialibraryAppStateObserverManager::GetInstance().AddTokenId(destTokenId, false);
    }
}

static void GrantPermissionPrepareHandle(MediaLibraryCommand &cmd, const std::vector<DataShareValuesBucket> &values,
    std::vector<int32_t>& dbOperation)
{
    DoSubscribeForAppStop(values);
    std::shared_ptr<OHOS::NativeRdb::ResultSet> resultSet;
    QueryUriPermission(cmd, values, resultSet);
    GetAllUriDbOperation(values, dbOperation, resultSet);
    FilterNotExistUri(values, dbOperation);
}

int32_t UriPermissionOperations::GrantUriPermission(MediaLibraryCommand &cmd,
    const std::vector<DataShareValuesBucket> &values)
{
    std::vector<int32_t> dbOperation;
    std::vector<ValuesBucket>  batchInsertBucket;
    bool needToInsert = false;
    bool isValid = false;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (ValueBucketCheck(values) != E_OK || rdbStore == nullptr) {
        return E_ERR;
    }
    GrantPermissionPrepareHandle(cmd, values, dbOperation);
    for (size_t i = 0; i < values.size(); i++) {
        int32_t fileId = GetFileId(values.at(i), isValid);
        int32_t uriType = values.at(i).Get(AppUriPermissionColumn::URI_TYPE, isValid);
        int32_t permissioinType = values.at(i).Get(AppUriPermissionColumn::PERMISSION_TYPE, isValid);
        if (dbOperation.at(i) == INSERT_DB_OPERATION) {
            needToInsert = true;
            InsertValueBucketPrepare(values, fileId, uriType, permissioinType, batchInsertBucket);
        }
    }
    if (!needToInsert) {
        MEDIA_INFO_LOG("GrantUriPermission: no data need to insert!");
        return E_OK;
    }
    std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>(__func__);
    int32_t errCode = E_OK;
    std::function<int(void)> func = [&]()->int {
        UriPermissionOperations::BatchInsertOperation(cmd, batchInsertBucket, trans);
        return errCode;
    };
    errCode = trans->RetryTrans(func);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("GrantUriPermission: trans retry fail!, ret:%{public}d", errCode);
        return errCode;
    }
    return E_OK;
}

int32_t UriPermissionOperations::HandleUriPermOperations(MediaLibraryCommand &cmd)
{
    if (!PermissionUtils::CheckIsSystemAppByUid()) {
        MEDIA_ERR_LOG("the caller is not system app");
        return E_PERMISSION_DENIED;
    }
    string bundleName = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();

    int32_t errCode = E_FAIL;
    switch (cmd.GetOprnType()) {
        case OperationType::INSERT_PERMISSION:
            errCode = HandleUriPermInsert(cmd);
            break;
        default:
            MEDIA_ERR_LOG("unknown operation type %{public}d", cmd.GetOprnType());
            break;
    }
    return errCode;
}

int32_t UriPermissionOperations::GetUriPermissionMode(const string &fileId, const string &bundleName,
    int32_t tableType, string &mode)
{
    MediaLibraryCommand cmd(Uri(MEDIALIBRARY_BUNDLEPERM_URI), OperationType::QUERY);
    cmd.GetAbsRdbPredicates()->EqualTo(PERMISSION_FILE_ID, fileId);
    cmd.GetAbsRdbPredicates()->And()->EqualTo(PERMISSION_BUNDLE_NAME, bundleName);
    cmd.GetAbsRdbPredicates()->And()->EqualTo(PERMISSION_TABLE_TYPE, to_string(tableType));
    auto queryResult = MediaLibraryObjectUtils::QueryWithCondition(cmd, {}, "");
    CHECK_AND_RETURN_RET_LOG(queryResult != nullptr, E_HAS_DB_ERROR, "Failed to obtain value from database");
    int count = -1;
    CHECK_AND_RETURN_RET_LOG(queryResult->GetRowCount(count) == NativeRdb::E_OK, E_HAS_DB_ERROR,
        "Failed to get query result row count");
    if (count <= 0) {
        return E_PERMISSION_DENIED;
    }
    CHECK_AND_RETURN_RET_LOG(queryResult->GoToFirstRow() == NativeRdb::E_OK, E_HAS_DB_ERROR,
        "Failed to go to first row");
    mode = GetStringVal(PERMISSION_MODE, queryResult);
    return E_SUCCESS;
}

static int32_t CheckUriPermValues(ValuesBucket &valuesBucket, int32_t &fileId, string &bundleName, int32_t &tableType,
    string &inputMode)
{
    ValueObject valueObject;
    if (valuesBucket.GetObject(PERMISSION_FILE_ID, valueObject)) {
        valueObject.GetInt(fileId);
    } else {
        MEDIA_ERR_LOG("ValueBucket does not have PERMISSION_FILE_ID");
        return E_INVALID_VALUES;
    }

    if (valuesBucket.GetObject(PERMISSION_BUNDLE_NAME, valueObject)) {
        valueObject.GetString(bundleName);
    } else {
        MEDIA_ERR_LOG("ValueBucket does not have PERMISSION_BUNDLE_NAME");
        return E_INVALID_VALUES;
    }

    if (valuesBucket.GetObject(PERMISSION_MODE, valueObject)) {
        valueObject.GetString(inputMode);
    } else {
        MEDIA_ERR_LOG("ValueBucket does not have PERMISSION_MODE");
        return E_INVALID_VALUES;
    }

    if (valuesBucket.GetObject(PERMISSION_TABLE_TYPE, valueObject)) {
        valueObject.GetInt(tableType);
    } else {
        MEDIA_ERR_LOG("ValueBucket does not have PERMISSION_TABLE_TYPE");
        return E_INVALID_VALUES;
    }
    if (!CheckMode(inputMode)) {
        return E_INVALID_MODE;
    }
    valuesBucket.Delete(PERMISSION_MODE);
    valuesBucket.PutString(PERMISSION_MODE, inputMode);
    return E_SUCCESS;
}

#ifdef MEDIALIBRARY_COMPATIBILITY
static void ConvertVirtualIdToRealId(ValuesBucket &valuesBucket, int32_t &fileId, int32_t tableType)
{
    if (tableType == static_cast<int32_t>(TableType::TYPE_PHOTOS)) {
        fileId = MediaFileUtils::GetRealIdByTable(fileId, PhotoColumn::PHOTOS_TABLE);
    } else if (tableType == static_cast<int32_t>(TableType::TYPE_AUDIOS)) {
        fileId = MediaFileUtils::GetRealIdByTable(fileId, AudioColumn::AUDIOS_TABLE);
    } else {
        fileId = MediaFileUtils::GetRealIdByTable(fileId, MEDIALIBRARY_TABLE);
    }

    valuesBucket.Delete(PERMISSION_FILE_ID);
    valuesBucket.PutInt(PERMISSION_FILE_ID, fileId);
}
#endif


int32_t UriPermissionOperations::HandleUriPermInsert(MediaLibraryCommand &cmd)
{
    return E_SUCCESS;
}

static inline int32_t GetTableTypeFromTableName(const std::string &tableName)
{
    if (tableName == PhotoColumn::PHOTOS_TABLE) {
        return static_cast<int32_t>(TableType::TYPE_PHOTOS);
    } else if (tableName == AudioColumn::AUDIOS_TABLE) {
        return static_cast<int32_t>(TableType::TYPE_AUDIOS);
    } else {
        return static_cast<int32_t>(TableType::TYPE_FILES);
    }
}

int32_t UriPermissionOperations::InsertBundlePermission(const int32_t &fileId, const std::string &bundleName,
    const std::string &mode, const std::string &tableName)
{
    string curMode;
    int32_t tableType = GetTableTypeFromTableName(tableName);
    auto ret = GetUriPermissionMode(to_string(fileId), bundleName, tableType, curMode);
    if ((ret != E_SUCCESS) && (ret != E_PERMISSION_DENIED)) {
        return ret;
    }
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_HAS_DB_ERROR, "uniStore is nullptr!");
    if (ret == E_PERMISSION_DENIED) {
        ValuesBucket addValues;
        addValues.PutInt(PERMISSION_FILE_ID, fileId);
        addValues.PutString(PERMISSION_BUNDLE_NAME, bundleName);
        addValues.PutString(PERMISSION_MODE, mode);
        addValues.PutInt(PERMISSION_TABLE_TYPE, tableType);
        MediaLibraryCommand cmd(Uri(MEDIALIBRARY_BUNDLEPERM_URI), addValues);
        int64_t outRowId = -1;
        return uniStore->Insert(cmd, outRowId);
    }
    CHECK_AND_RETURN_RET(curMode.find(mode) == string::npos, E_SUCCESS);
    ValuesBucket updateValues;
    updateValues.PutString(PERMISSION_MODE, mode);
    MediaLibraryCommand updateCmd(Uri(MEDIALIBRARY_BUNDLEPERM_URI), updateValues);
    updateCmd.GetAbsRdbPredicates()->EqualTo(PERMISSION_FILE_ID, to_string(fileId))->And()->
        EqualTo(PERMISSION_BUNDLE_NAME, bundleName)->And()->
        EqualTo(PERMISSION_TABLE_TYPE, to_string(tableType));
    int32_t updatedRows = -1;
    return uniStore->Update(updateCmd, updatedRows);
}

int32_t UriPermissionOperations::DeleteBundlePermission(const std::string &fileId, const std::string &bundleName,
    const std::string &tableName)
{
    int32_t tableType = GetTableTypeFromTableName(tableName);

    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_HAS_DB_ERROR, "uniStore is nullptr!");
    Uri uri(MEDIALIBRARY_BUNDLEPERM_URI);
    MediaLibraryCommand deleteCmd(uri);
    deleteCmd.GetAbsRdbPredicates()->EqualTo(PERMISSION_FILE_ID, fileId)->And()->
        EqualTo(PERMISSION_BUNDLE_NAME, bundleName)->And()->
        EqualTo(PERMISSION_TABLE_TYPE, to_string(tableType));
    int32_t deleteRows = -1;
    int32_t ret = uniStore->Delete(deleteCmd, deleteRows);
    if (deleteRows > 0 && ret == NativeRdb::E_OK) {
        MEDIA_DEBUG_LOG("DeleteBundlePermission success:fileId:%{private}s, bundleName:%{private}s, table:%{private}s",
            fileId.c_str(), bundleName.c_str(), tableName.c_str());
        return E_OK;
    }
    return E_HAS_DB_ERROR;
}

int32_t UriPermissionOperations::CheckUriPermission(const std::string &fileUri, std::string mode)
{
    if (!CheckMode(mode)) {
        return E_INVALID_MODE;
    }
    string bundleName = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    string fileId = MediaFileUtils::GetIdFromUri(fileUri);
    TableType tableType = TableType::TYPE_FILES;
    static map<string, TableType> tableMap = {
        { MEDIALIBRARY_TYPE_IMAGE_URI, TableType::TYPE_PHOTOS },
        { MEDIALIBRARY_TYPE_VIDEO_URI, TableType::TYPE_PHOTOS },
        { MEDIALIBRARY_TYPE_AUDIO_URI, TableType::TYPE_AUDIOS },
        { MEDIALIBRARY_TYPE_FILE_URI, TableType::TYPE_FILES },
        { PhotoColumn::PHOTO_TYPE_URI, TableType::TYPE_PHOTOS },
        { AudioColumn::AUDIO_TYPE_URI, TableType::TYPE_AUDIOS }
    };
    for (const auto &iter : tableMap) {
        if (fileUri.find(iter.first) != string::npos) {
            tableType = iter.second;
        }
    }
    string permissionMode;
    int32_t ret = GetUriPermissionMode(fileId, bundleName, static_cast<int32_t>(tableType), permissionMode);
    CHECK_AND_RETURN_RET(ret == E_SUCCESS, ret);
    return (permissionMode.find(mode) != string::npos) ? E_SUCCESS : E_PERMISSION_DENIED;
}
}   // Media
}   // OHOS

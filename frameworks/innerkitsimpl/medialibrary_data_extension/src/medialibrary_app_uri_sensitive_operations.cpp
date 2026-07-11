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
 * See the License for the specific language governing Sensitives and
 * limitations under the License.
 */

#include <vector>
#include <map>
#include "medialibrary_app_uri_sensitive_operations.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_rdb_utils.h"
#include "media_column.h"
#include "datashare_predicates.h"
#include "rdb_utils.h"
#include "media_file_uri.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_rdb_transaction.h"
#include "media_file_utils.h"
#include "medialibrary_appstate_observer.h"
#include "datashare_predicates_objects.h"

using namespace OHOS::DataShare;
using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::RdbDataShareAdapter;

namespace OHOS {
namespace Media {

const int MediaLibraryAppUriSensitiveOperations::ERROR = -1;
const int MediaLibraryAppUriSensitiveOperations::SUCCEED = 0;
const int MediaLibraryAppUriSensitiveOperations::ALREADY_EXIST = 1;
const int MediaLibraryAppUriSensitiveOperations::NO_DATA_EXIST = 0;

static void DoSubscribeAppStop(const ValuesBucket &value)
{
    ValueObject valueObject;
    int64_t destTokenId = -1;
    if (value.GetObject(AppUriPermissionColumn::TARGET_TOKENID, valueObject)) {
        valueObject.GetLong(destTokenId);
    }
    // delete the temporary permission when the app dies
    MedialibraryAppStateObserverManager::GetInstance().SubscribeAppState();
    MedialibraryAppStateObserverManager::GetInstance().AddTokenId(destTokenId, false);
}

int32_t MediaLibraryAppUriSensitiveOperations::HandleInsertOperation(MediaLibraryCommand &cmd)
{
    MEDIA_INFO_LOG("insert appUriSensitive begin");
    // sensitiveType from param
    int sensitiveTypeParam = -1;
    if (!GetIntFromValuesBucket(cmd.GetValueBucket(), AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE,
        sensitiveTypeParam)) {
        return ERROR;
    }
    if (!IsValidSensitiveType(sensitiveTypeParam)) {
        return ERROR;
    }
    
    // query sensitive data before insert
    int queryFlag = ERROR;
    shared_ptr<NativeRdb::ResultSet> resultSet = QueryNewData(cmd.GetValueBucket(), queryFlag);
    // Update the sensitiveType
    if (queryFlag > 0) {
        return UpdateSensitiveType(resultSet, sensitiveTypeParam);
    }
    if (queryFlag < 0) {
        return ERROR;
    }

    // delete the temporary Sensitive when the app dies
    DoSubscribeAppStop(cmd.GetValueBucket());

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("get rdbStore error");
        return ERROR;
    }
    // insert data
    int64_t outRowId = -1;
    cmd.GetValueBucket().PutLong(AppUriSensitiveColumn::DATE_MODIFIED,
        MediaFileUtils::UTCTimeMilliSeconds());
    
    int permissionTypeParam = -1;
    if (!GetIntFromValuesBucket(cmd.GetValueBucket(), AppUriPermissionColumn::PERMISSION_TYPE,
        permissionTypeParam)) {
        return ERROR;
    }
    cmd.GetValueBucket().Delete(AppUriPermissionColumn::PERMISSION_TYPE);
    cmd.SetTableName(AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);

    int32_t errCode = rdbStore->Insert(cmd, outRowId);
    if (errCode != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("insert into db error, errCode=%{public}d", errCode);
        return ERROR;
    }
    cmd.GetValueBucket().PutInt(AppUriPermissionColumn::PERMISSION_TYPE,
        permissionTypeParam);
    MEDIA_INFO_LOG("insert appUriSensitive ok");
    return SUCCEED;
}

bool MediaLibraryAppUriSensitiveOperations::BeForceSensitive(MediaLibraryCommand &cmd,
    const std::vector<DataShare::DataShareValuesBucket> &values)
{
    bool hasForce = false;
    for (auto it = values.begin(); it != values.end(); it++) {
        ValuesBucket value = RdbUtils::ToValuesBucket(*it);
        int beForce = -1;
        if (!GetIntFromValuesBucket(value, AppUriSensitiveColumn::IS_FORCE_SENSITIVE,
            beForce)) {
            continue;
        }
        if (beForce > 0) {
            hasForce = true;
            break;
        }
    }
    return hasForce;
}

int32_t MediaLibraryAppUriSensitiveOperations::BatchInsert(
    MediaLibraryCommand &cmd, const std::vector<DataShare::DataShareValuesBucket> &values)
{
    MEDIA_INFO_LOG("batch insert begin");
    std::vector<ValuesBucket> insertVector;
    for (auto it = values.begin(); it != values.end(); it++) {
        ValuesBucket value = RdbUtils::ToValuesBucket(*it);
        int queryFlag = -1;
        std::shared_ptr<OHOS::NativeRdb::ResultSet> resultSet = QueryNewData(value, queryFlag);
        if (queryFlag < 0) {
            return ERROR;
        }
        // sensitiveType from param
        int sensitiveTypeParam = -1;
        if (!GetIntFromValuesBucket(value, AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE,
            sensitiveTypeParam)) {
            return ERROR;
        }

        value.Delete(AppUriPermissionColumn::PERMISSION_TYPE);

        if (queryFlag == 0) {
            // delete the temporary sensitive when the app dies
            DoSubscribeAppStop(value);
            value.PutLong(AppUriSensitiveColumn::DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
            insertVector.push_back(value);
        } else if (UpdateSensitiveTypeAndForceHideSensitive(resultSet, sensitiveTypeParam, value) == ERROR) {
            return ERROR;
        }
    }
    if (!insertVector.empty()) {
        int64_t outRowId = -1;
        int32_t ret = MediaLibraryRdbStore::BatchInsert(outRowId, AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE,
            insertVector);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("batch insert err=%{public}d", ret);
            return ERROR;
        }
    }
    MEDIA_INFO_LOG("batch insert ok");
    return SUCCEED;
}

int32_t MediaLibraryAppUriSensitiveOperations::DeleteOperation(NativeRdb::RdbPredicates &predicates)
{
    MEDIA_INFO_LOG("delete begin");
    int deleteRow = MediaLibraryRdbStore::Delete(predicates);
    MEDIA_INFO_LOG("deleted row=%{public}d", deleteRow);
    return deleteRow < 0 ? ERROR : SUCCEED;
}

std::shared_ptr<OHOS::NativeRdb::ResultSet> MediaLibraryAppUriSensitiveOperations::QueryOperation(
    DataShare::DataSharePredicates &predicates, std::vector<std::string> &fetchColumns)
{
    RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates,
        AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    std::shared_ptr<OHOS::NativeRdb::ResultSet> resultSet =
        MediaLibraryRdbStore::QueryWithFilter(rdbPredicates, fetchColumns);
    if (resultSet == nullptr) {
        return nullptr;
    }
    int32_t numRows = 0;
    resultSet->GetRowCount(numRows);
    if (numRows == 0) {
        return nullptr;
    }
    resultSet->GoToFirstRow();
    return resultSet;
}

std::shared_ptr<OHOS::NativeRdb::ResultSet> MediaLibraryAppUriSensitiveOperations::QueryNewData(
    OHOS::NativeRdb::ValuesBucket &valueBucket, int &resultFlag)
{
    // target tokenId
    int64_t targetTokenId;
    ValueObject valueObject;
    if (valueBucket.GetObject(AppUriSensitiveColumn::TARGET_TOKENID, valueObject)) {
        valueObject.GetLong(targetTokenId);
    }

    // parse fileId
    int fileId = -1;
    if (!GetIntFromValuesBucket(valueBucket, AppUriSensitiveColumn::FILE_ID, fileId)) {
        resultFlag = ERROR;
        return nullptr;
    }

    // parse uriType
    int uriType = -1;
    if (!GetIntFromValuesBucket(valueBucket, AppUriSensitiveColumn::URI_TYPE, uriType)) {
        resultFlag = ERROR;
        return nullptr;
    }

    OHOS::DataShare::DataSharePredicates sensitivePredicates;
    sensitivePredicates.And()->EqualTo(AppUriSensitiveColumn::FILE_ID, fileId);
    sensitivePredicates.And()->EqualTo(AppUriSensitiveColumn::URI_TYPE, uriType);
    sensitivePredicates.And()->EqualTo(AppUriSensitiveColumn::TARGET_TOKENID, targetTokenId);
    vector<string> fetchColumns;
    fetchColumns.push_back(AppUriSensitiveColumn::ID);
    fetchColumns.push_back(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE);

    shared_ptr<NativeRdb::ResultSet> resultSet = QueryOperation(sensitivePredicates, fetchColumns);
    resultFlag = (resultSet == nullptr ? NO_DATA_EXIST : ALREADY_EXIST);
    return resultSet;
}

bool MediaLibraryAppUriSensitiveOperations::GetIntFromValuesBucket(
    OHOS::NativeRdb::ValuesBucket &valueBucket, const std::string &column, int &result)
{
    ValueObject valueObject;
    bool ret = valueBucket.GetObject(column, valueObject);
    if (!ret) {
        MEDIA_ERR_LOG("valueBucket param without %{public}s", column.c_str());
        return false;
    }
    result = valueObject;
    return true;
}

int MediaLibraryAppUriSensitiveOperations::UpdateSensitiveType(shared_ptr<NativeRdb::ResultSet> &resultSetDB,
    int &sensitiveTypeParam)
{
    // delete the temporary Sensitive when the app dies
    MedialibraryAppStateObserverManager::GetInstance().SubscribeAppState();

    // update sensitive type
    ValuesBucket updateVB;
    updateVB.PutInt(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, sensitiveTypeParam);
    updateVB.PutLong(AppUriSensitiveColumn::DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
    int32_t idDB = GetInt32Val(AppUriSensitiveColumn::ID, resultSetDB);

    RdbPredicates updateRdbPredicates(AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    updateRdbPredicates.EqualTo(AppUriSensitiveColumn::ID, idDB);
    int32_t updateRows = MediaLibraryRdbStore::UpdateWithDateTime(updateVB, updateRdbPredicates);
    if (updateRows < 1) {
        MEDIA_ERR_LOG("upgrade SensitiveType error,idDB=%{public}d", idDB);
        return ERROR;
    }
    MEDIA_INFO_LOG("update ok,Rows=%{public}d", updateRows);
    return SUCCEED;
}

int MediaLibraryAppUriSensitiveOperations::UpdateSensitiveTypeAndForceHideSensitive(
    shared_ptr<NativeRdb::ResultSet> &resultSetDB,
    int &sensitiveTypeParam, OHOS::NativeRdb::ValuesBucket &valueBucket)
{
    // get force value
    int isForce = -1;
    bool hasSetForce = GetIntFromValuesBucket(valueBucket, AppUriSensitiveColumn::IS_FORCE_SENSITIVE,
        isForce);
    // delete the temporary Sensitive when the app dies
    MedialibraryAppStateObserverManager::GetInstance().SubscribeAppState();

    // update is_force_hideSensitive value
    ValuesBucket updateVB;
    updateVB.PutInt(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, sensitiveTypeParam);
    updateVB.PutLong(AppUriSensitiveColumn::DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
    if (hasSetForce && isForce > 0) {
        updateVB.PutInt(AppUriSensitiveColumn::IS_FORCE_SENSITIVE, 1);
    }
    int32_t idDB = GetInt32Val(AppUriSensitiveColumn::ID, resultSetDB);

    RdbPredicates updateRdbPredicates(AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    updateRdbPredicates.EqualTo(AppUriSensitiveColumn::ID, idDB);
    int32_t updateRows = MediaLibraryRdbStore::UpdateWithDateTime(updateVB, updateRdbPredicates);
    if (updateRows < 1) {
        MEDIA_ERR_LOG("upgrade SensitiveType error,idDB=%{public}d", idDB);
        return ERROR;
    }
    MEDIA_INFO_LOG("update ok,Rows=%{public}d", updateRows);
    return SUCCEED;
}

bool MediaLibraryAppUriSensitiveOperations::IsValidSensitiveType(int &sensitiveType)
{
    bool isValid = AppUriSensitiveColumn::SENSITIVE_TYPES_ALL.find(sensitiveType)
        != AppUriSensitiveColumn::SENSITIVE_TYPES_ALL.end();
    if (!isValid) {
        MEDIA_ERR_LOG("invalid SensitiveType=%{public}d", sensitiveType);
    }
    return isValid;
}
} // namespace Media
} // namespace OHOS
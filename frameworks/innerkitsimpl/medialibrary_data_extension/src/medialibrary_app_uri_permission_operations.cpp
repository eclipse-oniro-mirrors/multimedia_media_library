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

#include <vector>
#include <map>
#include "medialibrary_app_uri_permission_operations.h"
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
#include "media_values_bucket_utils.h"

using namespace OHOS::DataShare;
using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::RdbDataShareAdapter;

namespace OHOS {
namespace Media {

const int MediaLibraryAppUriPermissionOperations::ERROR = -1;
const int MediaLibraryAppUriPermissionOperations::SUCCEED = 0;
const int MediaLibraryAppUriPermissionOperations::ALREADY_EXIST = 1;
const int MediaLibraryAppUriPermissionOperations::NO_DATA_EXIST = 0;

static void DoSubscribeAppStop(const ValuesBucket &value)
{
    ValueObject valueObject;
    int64_t destTokenId = -1;
    if (value.GetObject(AppUriPermissionColumn::TARGET_TOKENID, valueObject)) {
        valueObject.GetLong(destTokenId);
    }

    int permissionTypeParam = -1;
    if (value.GetObject(AppUriPermissionColumn::PERMISSION_TYPE, valueObject)) {
        valueObject.GetInt(permissionTypeParam);
    }
    // delete the temporary permission when the app dies
    if (AppUriPermissionColumn::PERMISSION_TYPES_TEMPORARY.find(permissionTypeParam) !=
        AppUriPermissionColumn::PERMISSION_TYPES_TEMPORARY.end()) {
        MedialibraryAppStateObserverManager::GetInstance().SubscribeAppState();
        MedialibraryAppStateObserverManager::GetInstance().AddTokenId(destTokenId, false);
    }
}

int32_t MediaLibraryAppUriPermissionOperations::HandleInsertOperation(MediaLibraryCommand &cmd)
{
    MEDIA_INFO_LOG("insert appUriPermission begin");
    // permissionType from param
    int permissionTypeParam = -1;
    if (!MediaValuesBucketUtils::GetInt(cmd.GetValueBucket(), AppUriPermissionColumn::PERMISSION_TYPE,
        permissionTypeParam)) {
        return ERROR;
    }
    if (!IsValidPermissionType(permissionTypeParam)) {
        return ERROR;
    }
    // parse fileId
    int fileId = -1;
    if (!MediaValuesBucketUtils::GetInt(cmd.GetValueBucket(), AppUriPermissionColumn::FILE_ID, fileId)) {
        return ERROR;
    }
    if (!IsPhotoExist(fileId)) {
        return ERROR;
    }
    // query permission data before insert
    int queryFlag = ERROR;
    shared_ptr<NativeRdb::ResultSet> resultSet = QueryNewData(cmd.GetValueBucket(), queryFlag);
    // Update the permissionType
    if (queryFlag > 0) {
        return UpdatePermissionType(resultSet, permissionTypeParam);
    }
    if (queryFlag < 0) {
        return ERROR;
    }
    DoSubscribeAppStop(cmd.GetValueBucket());

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("get rdbStore error");
        return ERROR;
    }
    // insert data
    int64_t outRowId = -1;
    cmd.GetValueBucket().PutLong(AppUriPermissionColumn::DATE_MODIFIED,
        MediaFileUtils::UTCTimeMilliSeconds());
    
    cmd.GetValueBucket().Delete(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE);
    cmd.GetValueBucket().Delete(AppUriSensitiveColumn::IS_FORCE_SENSITIVE);
    cmd.SetTableName(AppUriPermissionColumn::APP_URI_PERMISSION_TABLE);

    int32_t errCode = rdbStore->Insert(cmd, outRowId);
    if (errCode != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("insert into db error, errCode=%{public}d", errCode);
        return ERROR;
    }
    MEDIA_INFO_LOG("insert appUriPermission ok");
    return SUCCEED;
}

int32_t MediaLibraryAppUriPermissionOperations::BatchInsert(
    MediaLibraryCommand &cmd, const std::vector<DataShare::DataShareValuesBucket> &values)
{
    MEDIA_INFO_LOG("batch insert begin");

    if (!IsPhotosAllExist(values)) {
        return ERROR;
    }
    std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>(__func__);
    std::function<int(void)> func = [&]()->int {
        return BatchInsertInner(cmd, values, trans);
    };
    int32_t errCode = trans->RetryTrans(func);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("BatchInsert: trans retry fail!, ret:%{public}d", errCode);
        return errCode;
    }
    MEDIA_INFO_LOG("batch insert ok");
    return SUCCEED;
}

int32_t MediaLibraryAppUriPermissionOperations::BatchInsertInner(
    MediaLibraryCommand &cmd, const std::vector<DataShare::DataShareValuesBucket> &values,
    std::shared_ptr<TransactionOperations> trans)
{
    int32_t errCode = NativeRdb::E_OK;
    bool isValid;
    int64_t destTokenId = values.at(0).Get(AppUriPermissionColumn::TARGET_TOKENID, isValid);
    int permissionTypeParam = values.at(0).Get(AppUriPermissionColumn::PERMISSION_TYPE, isValid);
    // delete the temporary permission when the app dies
    if (AppUriPermissionColumn::PERMISSION_TYPES_TEMPORARY.find(permissionTypeParam) !=
        AppUriPermissionColumn::PERMISSION_TYPES_TEMPORARY.end()) {
        MedialibraryAppStateObserverManager::GetInstance().SubscribeAppState();
        MedialibraryAppStateObserverManager::GetInstance().AddTokenId(destTokenId, false);
    }
    std::vector<ValuesBucket> insertVector;
    for (auto it = values.begin(); it != values.end(); it++) {
        ValuesBucket value = RdbUtils::ToValuesBucket(*it);
        int queryFlag = -1;
        std::shared_ptr<OHOS::NativeRdb::ResultSet> resultSet = QueryNewData(value, queryFlag);
        if (queryFlag < 0) {
            return ERROR;
        }

        value.Delete(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE);
        value.Delete(AppUriSensitiveColumn::IS_FORCE_SENSITIVE);

        if (queryFlag == 0) {
            value.PutLong(AppUriPermissionColumn::DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
            insertVector.push_back(value);
        } else if (UpdatePermissionType(resultSet, permissionTypeParam) == ERROR) {
            return ERROR;
        }
    }

    if (!insertVector.empty()) {
        if (errCode != E_OK) {
            MEDIA_ERR_LOG("start transaction error, errCode = %{public}d", errCode);
            return ERROR;
        }
        int64_t outRowId = -1;
        errCode = trans->BatchInsert(outRowId, AppUriPermissionColumn::APP_URI_PERMISSION_TABLE, insertVector);
        if (errCode != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("batch insert err=%{public}d", errCode);
            return ERROR;
        }
    }
    return errCode;
}

int32_t MediaLibraryAppUriPermissionOperations::DeleteOperation(NativeRdb::RdbPredicates &predicates)
{
    MEDIA_INFO_LOG("delete begin");
    int deleteRow = MediaLibraryRdbStore::Delete(predicates);
    MEDIA_INFO_LOG("deleted row=%{public}d", deleteRow);
    return deleteRow < 0 ? ERROR : SUCCEED;
}

std::shared_ptr<OHOS::NativeRdb::ResultSet> MediaLibraryAppUriPermissionOperations::QueryOperation(
    DataShare::DataSharePredicates &predicates, std::vector<std::string> &fetchColumns)
{
    RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates,
        AppUriPermissionColumn::APP_URI_PERMISSION_TABLE);
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

std::shared_ptr<OHOS::NativeRdb::ResultSet> MediaLibraryAppUriPermissionOperations::QueryNewData(
    OHOS::NativeRdb::ValuesBucket &valueBucket, int &resultFlag)
{
    // parse tokenId
    int64_t srcTokenId;
    ValueObject valueObject;
    if (valueBucket.GetObject(AppUriPermissionColumn::SOURCE_TOKENID, valueObject)) {
        valueObject.GetLong(srcTokenId);
    }
    int64_t destTokenId;
    if (valueBucket.GetObject(AppUriPermissionColumn::TARGET_TOKENID, valueObject)) {
        valueObject.GetLong(destTokenId);
    }
    // parse fileId
    int fileId = -1;
    if (!MediaValuesBucketUtils::GetInt(valueBucket, AppUriPermissionColumn::FILE_ID, fileId)) {
        resultFlag = ERROR;
        return nullptr;
    }

    // parse uriType
    int uriType = -1;
    if (!MediaValuesBucketUtils::GetInt(valueBucket, AppUriPermissionColumn::URI_TYPE, uriType)) {
        resultFlag = ERROR;
        return nullptr;
    }

    OHOS::DataShare::DataSharePredicates permissionPredicates;
    permissionPredicates.And()->EqualTo(AppUriPermissionColumn::FILE_ID, fileId);
    permissionPredicates.And()->EqualTo(AppUriPermissionColumn::URI_TYPE, uriType);
    permissionPredicates.And()->EqualTo(AppUriPermissionColumn::SOURCE_TOKENID, srcTokenId);
    permissionPredicates.And()->EqualTo(AppUriPermissionColumn::TARGET_TOKENID, destTokenId);
    
    vector<string> fetchColumns;
    fetchColumns.push_back(AppUriPermissionColumn::ID);
    fetchColumns.push_back(AppUriPermissionColumn::PERMISSION_TYPE);

    shared_ptr<NativeRdb::ResultSet> resultSet = QueryOperation(permissionPredicates, fetchColumns);
    resultFlag = (resultSet == nullptr ? NO_DATA_EXIST : ALREADY_EXIST);
    return resultSet;
}

int MediaLibraryAppUriPermissionOperations::UpdatePermissionType(shared_ptr<NativeRdb::ResultSet> &resultSetDB,
    int &permissionTypeParam, std::shared_ptr<TransactionOperations> trans)
{
    if (resultSetDB == nullptr) {
        MEDIA_ERR_LOG("resultSetDB is null in UpdatePermissionType");
        return ERROR;
    }
    int32_t permissionTypeDB = GetInt32Val(AppUriPermissionColumn::PERMISSION_TYPE, resultSetDB);
    if (!CanOverride(permissionTypeParam, permissionTypeDB)) {
        return ALREADY_EXIST;
    }

    // update permission type
    ValuesBucket updateVB;
    updateVB.PutInt(AppUriPermissionColumn::PERMISSION_TYPE, permissionTypeParam);
    updateVB.PutLong(AppUriPermissionColumn::DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
    int32_t idDB = GetInt32Val(AppUriPermissionColumn::ID, resultSetDB);

    RdbPredicates updateRdbPredicates(AppUriPermissionColumn::APP_URI_PERMISSION_TABLE);
    updateRdbPredicates.EqualTo(AppUriPermissionColumn::ID, idDB);
    int32_t updateRows = 0;
    if (trans == nullptr) {
        updateRows = MediaLibraryRdbStore::UpdateWithDateTime(updateVB, updateRdbPredicates);
    } else {
        updateRows = trans->UpdateWithDateTime(updateVB, updateRdbPredicates);
    }
    if (updateRows < 1) {
        MEDIA_ERR_LOG("upgrade permissionType error,idDB=%{public}d", idDB);
        return ERROR;
    }
    MEDIA_INFO_LOG("update ok,Rows=%{public}d", updateRows);
    return SUCCEED;
}

bool MediaLibraryAppUriPermissionOperations::IsValidPermissionType(int &permissionType)
{
    bool isValid = AppUriPermissionColumn::PERMISSION_TYPES_ALL.find(permissionType)
        != AppUriPermissionColumn::PERMISSION_TYPES_ALL.end();
    if (!isValid) {
        MEDIA_ERR_LOG("invalid permissionType=%{public}d", permissionType);
    }
    return isValid;
}

bool MediaLibraryAppUriPermissionOperations::CanOverride(int &permissionTypeParam, int &permissionTypeDB)
{
    MEDIA_INFO_LOG("permissionTypeParam=%{public}d,permissionTypeDB=%{public}d",
        permissionTypeParam, permissionTypeDB);
    // Equal permissions do not need to be overridden
    if (permissionTypeParam == permissionTypeDB) {
        return false;
    }
    // temporary permission can't override persist permission
    if (AppUriPermissionColumn::PERMISSION_TYPES_TEMPORARY.find(permissionTypeParam) !=
        AppUriPermissionColumn::PERMISSION_TYPES_TEMPORARY.end()
        && AppUriPermissionColumn::PERMISSION_TYPES_PERSIST.find(permissionTypeDB) !=
        AppUriPermissionColumn::PERMISSION_TYPES_PERSIST.end()
    ) {
        return false;
    }
    // persist permission can override temporary permission
    if (AppUriPermissionColumn::PERMISSION_TYPES_TEMPORARY.find(permissionTypeDB) !=
        AppUriPermissionColumn::PERMISSION_TYPES_TEMPORARY.end()
        && AppUriPermissionColumn::PERMISSION_TYPES_PERSIST.find(permissionTypeParam) !=
        AppUriPermissionColumn::PERMISSION_TYPES_PERSIST.end()
    ) {
        return true;
    }
    // Temporary permissions can override each other.
    if (AppUriPermissionColumn::PERMISSION_TYPES_TEMPORARY.find(permissionTypeParam) !=
        AppUriPermissionColumn::PERMISSION_TYPES_TEMPORARY.end()
        && AppUriPermissionColumn::PERMISSION_TYPES_TEMPORARY.find(permissionTypeDB) !=
        AppUriPermissionColumn::PERMISSION_TYPES_PERSIST.end()
    ) {
        return true;
    }
    // PERMISSION_PERSIST_READ_WRITE can override PERMISSION_PERSIST_READ, but not vice verse.
    return permissionTypeParam == AppUriPermissionColumn::PERMISSION_PERSIST_READ_WRITE;
}

bool MediaLibraryAppUriPermissionOperations::IsPhotoExist(int32_t &photoFileId)
{
    // query whether photo exists.
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.And()->EqualTo(PhotoColumn::MEDIA_ID, std::to_string(photoFileId));
    vector<string> photoColumns;
    photoColumns.push_back(PhotoColumn::MEDIA_ID);
    shared_ptr<NativeRdb::ResultSet> photoRestultSet =
        MediaLibraryRdbStore::QueryWithFilter(rdbPredicates, photoColumns);
    if (photoRestultSet == nullptr) {
        MEDIA_ERR_LOG("query photoRestultSet is null,fileId=%{public}d", photoFileId);
        return false;
    }
    int32_t photoNumRows = 0;
    photoRestultSet->GetRowCount(photoNumRows);
    if (photoNumRows == 0) {
        MEDIA_ERR_LOG("query photo not exist,fileId=%{public}d", photoFileId);
        return false;
    }
    return true;
}

bool MediaLibraryAppUriPermissionOperations::IsPhotosAllExist(
    const std::vector<DataShare::DataShareValuesBucket> &values)
{
    std::vector<std::string> fileIds;
    bool isValid = false;
    for (auto it = values.begin(); it != values.end(); it++) {
        int fileId = it->Get(AppUriPermissionColumn::FILE_ID, isValid);
        if (!isValid) {
            MEDIA_ERR_LOG("get fileId error");
            return false;
        }
        fileIds.push_back(std::to_string(static_cast<int32_t>(fileId)));
    }
    // query whether photos exists.
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.And()->In(MediaColumn::MEDIA_ID, fileIds);
    vector<string> photoColumns;
    photoColumns.push_back(PhotoColumn::MEDIA_ID);
    shared_ptr<NativeRdb::ResultSet> photoRestultSet =
        MediaLibraryRdbStore::QueryWithFilter(rdbPredicates, photoColumns);
    if (photoRestultSet == nullptr) {
        MEDIA_ERR_LOG("query photoRestultSet is null");
        return false;
    }
    int32_t photoNumRows = 0;
    photoRestultSet->GetRowCount(photoNumRows);
    size_t photoNum = static_cast<size_t>(photoNumRows);
    if (photoNum != fileIds.size()) {
        MEDIA_ERR_LOG("some photo not exist");
        return false;
    }
    return true;
}

} // namespace Media
} // namespace OHOS
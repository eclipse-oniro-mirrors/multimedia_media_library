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

#define MLOG_TAG "DbPermissionHandler"

#include "db_permission_handler.h"

#include <cstdlib>

#include "media_file_uri.h"
#include "media_file_utils.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_rdbstore.h"
#include "rdb_utils.h"
#include "medialibrary_uripermission_operations.h"
#include "permission_utils.h"
#include "medialibrary_type_const.h"
#include "medialibrary_data_manager.h"
#include "media_column.h"

using namespace std;
using namespace OHOS::RdbDataShareAdapter;

namespace OHOS::Media {
static set<int> readPermSet{0, 1, 3, 4};

static set<int> writePermSet{2, 3, 4};
static string UFM_PHOTO_PREFIX = "datashare:///media/userfilemgr_photo_operation";
static string UFM_AUDIO_PREFIX = "datashare:///media/userfilemgr_audio_operation";
static string PATH_PHOTO_PREFIX = "datashare:///media/phaccess_photo_operation";

static bool ParseFileIdFromPredicates(const DataShare::DataSharePredicates &predicates, string &fileId)
{
    // parse fileId from operationList
    constexpr int32_t FIELD_IDX = 0;
    constexpr int32_t VALUE_IDX = 1;
    constexpr int32_t OPERATION_SIZE = 2;
    auto operationItems = predicates.GetOperationList();
    for (DataShare::OperationItem item : operationItems) {
        if (item.singleParams.size() < OPERATION_SIZE) {
            continue;
        }
        if (!MediaLibraryDataManagerUtils::IsNumber(static_cast<string>(item.GetSingle(VALUE_IDX)))) {
            continue;
        }
        if (static_cast<string>(item.GetSingle(FIELD_IDX)) == MediaColumn::MEDIA_ID) {
            fileId = static_cast<string>(item.GetSingle(VALUE_IDX));
            return true;
        }
    }

    // parse fileId from whereClause
    const string &clause = predicates.GetWhereClause();
    const vector<string> &values = predicates.GetWhereArgs();
    size_t pos = clause.find(MediaColumn::MEDIA_ID);
    if (pos == string::npos) {
        MEDIA_ERR_LOG("whereClause not include fileId");
        return false;
    }
    size_t argIndex = 0;
    constexpr char placeholder = '?';
    for (size_t i = 0; i < pos; ++i) {
        if (clause[i] == placeholder) {
            ++argIndex;
        }
    }
    if (argIndex >= values.size()) {
        MEDIA_ERR_LOG("argIndex should less than values size");
        return false;
    }
    fileId = values[argIndex];
    if (!MediaLibraryDataManagerUtils::IsNumber(fileId)) {
        MEDIA_ERR_LOG("whereArgs fileId=%{public}s is not num", fileId.c_str());
        return false;
    }
    return true;
}

static bool ParseInfoFromCmd(MediaLibraryCommand &cmd, string &fileId, int32_t &uriType)
{
    if (MediaFileUtils::StartsWith(cmd.GetUri().ToString(), PhotoColumn::PHOTO_URI_PREFIX)) {
        uriType = static_cast<int32_t>(TableType::TYPE_PHOTOS);
        fileId = MediaFileUtils::GetIdFromUri(cmd.GetUri().ToString());
        return true;
    }
    if (MediaFileUtils::StartsWith(cmd.GetUri().ToString(), AudioColumn::AUDIO_URI_PREFIX)) {
        uriType = static_cast<int32_t>(TableType::TYPE_AUDIOS);
        fileId = MediaFileUtils::GetIdFromUri(cmd.GetUri().ToString());
        return true;
    }

    if (cmd.IsDataSharePredNull()) {
        MEDIA_DEBUG_LOG("DataSharePred is nullptr");
        return false;
    }
    bool isPhotoType = MediaFileUtils::StartsWith(cmd.GetUri().ToString(), UFM_PHOTO_PREFIX)
        || MediaFileUtils::StartsWith(cmd.GetUri().ToString(), PATH_PHOTO_PREFIX);
    if (isPhotoType) {
        uriType = static_cast<int32_t>(TableType::TYPE_PHOTOS);
        return ParseFileIdFromPredicates(cmd.GetDataSharePred(), fileId);
    }
    if (MediaFileUtils::StartsWith(cmd.GetUri().ToString(), UFM_AUDIO_PREFIX)) {
        uriType = static_cast<int32_t>(TableType::TYPE_AUDIOS);
        return ParseFileIdFromPredicates(cmd.GetDataSharePred(), fileId);
    }
    MEDIA_ERR_LOG("parse fileId and uriType from cmd fail");
    return false;
}

int32_t DbPermissionHandler::ExecuteCheckPermission(MediaLibraryCommand &cmd, PermParam &permParam)
{
    MEDIA_DEBUG_LOG("DbPermissionHandler enter");
    bool isWrite = permParam.isWrite;
    string appId = GetClientAppId();
    string fileId = "";
    int32_t uriType = 0;
    if (!ParseInfoFromCmd(cmd, fileId, uriType)) {
        return E_INVALID_URI;
    }
    MEDIA_DEBUG_LOG("isWrite=%{public}d,appId=%{public}s,fileId=%{public}s,uriType=%{public}d",
        isWrite, appId.c_str(), fileId.c_str(), uriType);
    if (appId.empty() || fileId.empty()) {
        MEDIA_ERR_LOG("invalid input");
        return E_INVALID_FILEID;
    }
    DataShare::DataSharePredicates predicates;
    predicates.SetWhereClause("file_id = ? and appid = ? and uri_type = ?");
    predicates.SetWhereArgs({fileId, appId, to_string(uriType)});
    vector<string> columns;
    auto queryResultSet =
        MediaLibraryRdbStore::QueryWithFilter(RdbUtils::ToPredicates(predicates, TABLE_PERMISSION), columns);
    CHECK_AND_RETURN_RET_LOG(queryResultSet != nullptr, E_PERMISSION_DENIED, "queryResultSet is nullptr");
    int count = 0;
    auto ret = queryResultSet->GetRowCount(count);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK && count > 0, E_PERMISSION_DENIED, "db is no permission record");
    ret = queryResultSet->GoToFirstRow();
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_PERMISSION_DENIED, "GoToFirstRow fail");
    int index = -1;
    ret = queryResultSet->GetColumnIndex(FIELD_PERMISSION_TYPE, index);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_PERMISSION_DENIED, "GetColumnIndex fail");
    int32_t permissionType = 0;
    ret = queryResultSet->GetInt(index, permissionType);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_PERMISSION_DENIED, "GetInt fail");
    if (!isWrite) {
        return ConvertPermResult(readPermSet.count(permissionType));
    }
    return ConvertPermResult(writePermSet.count(permissionType));
}

} // namespace name
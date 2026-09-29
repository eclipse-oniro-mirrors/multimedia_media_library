/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MediaPermissionCheck"
#include <string>
#include "media_db_permission_check.h"
#include "medialibrary_bundle_manager.h"

using namespace std;
namespace OHOS::Media {
static const std::string TABLE_PERMISSION = "UriPermission"; // 权限表
static const std::string FIELD_PERMISSION_TYPE = "permission_type";

int32_t DbPermissionCheck::GetPermissionType(uint32_t code, const PermissionHeaderReq &request,
    int32_t &permissionType)
{
    MEDIA_INFO_LOG("DbPermissionHandler enter");
    string fileId = request.getFileId();
    if (fileId.empty()) {
        MEDIA_ERR_LOG("invalid fileId input");
        return E_INVALID_FILEID;
    }
    int32_t uriType = request.getUriType();
    string appId = PermissionUtils::GetAppIdByBundleName(
        MediaLibraryBundleManager::GetInstance()->GetClientBundleName());
    uint32_t tokenId = PermissionUtils::GetTokenId();
    MEDIA_DEBUG_LOG("fileId=%{public}s", fileId.c_str());
    if ((appId.empty() && !tokenId)) {
        MEDIA_ERR_LOG("invalid input");
        return E_INVALID_FILEID;
    }
    DataShare::DataSharePredicates predicates;
    predicates.SetWhereClause("file_id = ? and (appid = ? or target_tokenId = ?) and uri_type = ?"
        " order by permission_type desc");
    predicates.SetWhereArgs({fileId, appId, to_string(tokenId), to_string(uriType)});
    vector<string> columns;
    auto queryResultSet = MediaLibraryRdbStore::QueryWithFilter(
        RdbDataShareAdapter::RdbUtils::ToPredicates(predicates, TABLE_PERMISSION), columns);
    CHECK_AND_RETURN_RET_LOG(queryResultSet != nullptr, E_PERMISSION_DENIED, "queryResultSet is nullptr");
    int count = 0;
    auto ret = queryResultSet->GetRowCount(count);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK && count > 0,
        E_PERMISSION_DENIED, "db is no permission record");
    ret = queryResultSet->GoToFirstRow();
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_PERMISSION_DENIED, "GoToFirstRow fail");
    int index = -1;
    ret = queryResultSet->GetColumnIndex(FIELD_PERMISSION_TYPE, index);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_PERMISSION_DENIED, "GetColumnIndex fail");
    
    ret = queryResultSet->GetInt(index, permissionType);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_PERMISSION_DENIED, "GetInt fail");
    return E_SUCCESS;
}

int32_t DbPermissionCheck::CheckDBPermissionBypass(uint32_t code, const PermissionHeaderReq &request)
{
    MEDIA_INFO_LOG("CheckDBPermissionBypass enter");
    uint32_t tokenId = PermissionUtils::GetTokenId();
    CHECK_AND_RETURN_RET_LOG(tokenId, E_PERMISSION_DENIED, "Get tokenId fail");

    DataShare::DataSharePredicates predicates;
    predicates.SetWhereClause("target_tokenId = ? order by permission_type desc");
    predicates.SetWhereArgs({to_string(tokenId)});
    vector<string> columns;
    auto queryResultSet = MediaLibraryRdbStore::QueryWithFilter(
        RdbDataShareAdapter::RdbUtils::ToPredicates(predicates, TABLE_PERMISSION), columns);
    CHECK_AND_RETURN_RET_LOG(queryResultSet != nullptr, E_PERMISSION_DENIED, "queryResultSet is nullptr");
    int count = 0;
    auto ret = queryResultSet->GetRowCount(count);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK && count > 0, E_PERMISSION_DENIED, "db is no permission record");
    return E_PERMISSION_DB_BYPASS;
}
} // namespace OHOS::Media

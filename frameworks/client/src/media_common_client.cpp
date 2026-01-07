/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "media_common_client.h"

#include "media_asset_rdbstore.h"
#include "medialibrary_errno.h"
#include "medialibrary_napi_log.h"

namespace OHOS::Media::IPC {
MediaCommonClient::MediaCommonClient() {}
MediaCommonClient::~MediaCommonClient() {}
// LCOV_EXCL_START
MediaCommonClient &MediaCommonClient::GetInstance()
{
    static MediaCommonClient instance;
    return instance;
}

std::shared_ptr<NativeRdb::ResultSet> MediaCommonClient::QueryByStep(const std::string &sql)
{
    std::shared_ptr<NativeRdb::ResultSet> resultSet = MediaAssetRdbStore::GetInstance()->QueryByStep(sql);
    return resultSet;
}

std::pair<bool, std::shared_ptr<DataShare::DataShareResultSet>> MediaCommonClient::QueryAccessibleViaSandBox(Uri &uri,
    const DataShare::DataSharePredicates &predicates, std::vector<std::string> &columns, int &errCode,
    const int32_t userId)
{
    OperationObject object = OperationObject::UNKNOWN_OBJECT;
    if (MediaAssetRdbStore::GetInstance()->IsQueryAccessibleViaSandBox(uri, object, predicates) && userId == -1) {
        return {true, MediaAssetRdbStore::GetInstance()->Query(predicates, columns, object, errCode)};
    }
    return {false, nullptr};
}

std::shared_ptr<NativeRdb::ResultSet> MediaCommonClient::QueryRdb(Uri &uri,
    const DataShare::DataSharePredicates &predicates, std::vector<std::string> &columns)
{
    std::shared_ptr<NativeRdb::ResultSet> resultSet = nullptr;
    OperationObject object = OperationObject::UNKNOWN_OBJECT;
    if (MediaAssetRdbStore::GetInstance()->IsSupportSharedAssetQuery(uri, object)) {
        resultSet = MediaAssetRdbStore::GetInstance()->QueryRdb(predicates, columns, object);
    }
    return resultSet;
}

bool MediaCommonClient::IsNoIpc(Uri &uri, OperationObject &object, const DataShare::DataSharePredicates &predicates,
    bool isIgnoreSELinux)
{
    return MediaAssetRdbStore::GetInstance()->IsQueryAccessibleViaSandBox(uri, object, predicates);
}
// LCOV_EXCL_STOP
}
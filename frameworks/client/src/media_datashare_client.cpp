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

#include "media_datashare_client.h"

#include "media_uri_utils.h"
#include "medialibrary_napi_log.h"
#include "medialibrary_errno.h"

namespace OHOS::Media::IPC {
MediaDataShareClient::MediaDataShareClient() {}
MediaDataShareClient::~MediaDataShareClient() {}
// LCOV_EXCL_START
MediaDataShareClient& MediaDataShareClient::GetInstance()
{
    static MediaDataShareClient instance;
    return instance;
}

std::shared_ptr<DataShare::DataShareResultSet> MediaDataShareClient::Query(Uri &uri,
    const DataShare::DataSharePredicates &predicates, std::vector<std::string> &columns, int &errCode,
    const int32_t userId)
{
    if (!IsValid(userId)) {
        NAPI_ERR_LOG("Query fail, helper null, userId is %{public}d", userId);
        return nullptr;
    }

    std::shared_ptr<DataShare::DataShareResultSet> resultSet = nullptr;
    OperationObject object = OperationObject::UNKNOWN_OBJECT;
    if (IsNoIpc(uri, object, predicates) && userId == -1) {
        resultSet = QueryWithoutIpc(predicates, columns, object, errCode);
    } else {
        uri = MediaUriUtils::GetMultiUri(uri, userId);
        DataShare::DatashareBusinessError businessError;
        resultSet = GetDataShareHelperByUser(userId)->Query(uri, predicates, columns, &businessError);
        errCode = businessError.GetCode();
    }
    return resultSet;
}

int MediaDataShareClient::Insert(Uri &uri, const DataShare::DataShareValuesBucket &value, const int32_t userId)
{
    if (!IsValid(userId)) {
        NAPI_ERR_LOG("insert fail, helper null, userId is %{public}d", userId);
        return E_FAIL;
    }
    int index = GetDataShareHelperByUser(userId)->Insert(uri, value);
    return index;
}

int MediaDataShareClient::InsertExt(Uri &uri, const DataShare::DataShareValuesBucket &value, std::string &result,
    const int32_t userId)
{
    if (!IsValid(userId)) {
        NAPI_ERR_LOG("insert fail, helper null, userId is %{public}d", userId);
        return E_FAIL;
    }
    int index = GetDataShareHelperByUser(userId)->InsertExt(uri, value, result);
    return index;
}

int MediaDataShareClient::BatchInsert(Uri &uri, const std::vector<DataShare::DataShareValuesBucket> &values)
{
    if (!IsValid(GetUserId())) {
        NAPI_ERR_LOG("Batch insert fail, helper null, userId is %{public}d", GetUserId());
        return E_FAIL;
    }
    return GetDataShareHelperByUser(GetUserId())->BatchInsert(uri, values);
}

int MediaDataShareClient::Delete(Uri &uri, const DataShare::DataSharePredicates &predicates)
{
    if (!IsValid(GetUserId())) {
        NAPI_ERR_LOG("delete fail, helper null, userId is %{public}d", GetUserId());
        return E_FAIL;
    }
    return GetDataShareHelperByUser(GetUserId())->Delete(uri, predicates);
}

void MediaDataShareClient::NotifyChange(const Uri &uri)
{
    if (!IsValid(GetUserId())) {
        NAPI_ERR_LOG("notify change fail, helper null, userId is %{public}d", GetUserId());
        return;
    }
    GetDataShareHelperByUser(GetUserId())->NotifyChange(uri);
}

void MediaDataShareClient::RegisterObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver)
{
    if (!IsValid(GetUserId())) {
        NAPI_ERR_LOG("register observer fail, helper null, userId is %{public}d", GetUserId());
        return;
    }
    GetDataShareHelperByUser(GetUserId())->RegisterObserver(uri, dataObserver);
}

void MediaDataShareClient::UnregisterObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver)
{
    if (!IsValid(GetUserId())) {
        NAPI_ERR_LOG("unregister observer fail, helper null, userId is %{public}d", GetUserId());
        return;
    }
    GetDataShareHelperByUser(GetUserId())->UnregisterObserver(uri, dataObserver);
}

int MediaDataShareClient::OpenFile(Uri &uri, const std::string &mode, const int32_t userId)
{
    if (!IsValid(userId)) {
        NAPI_ERR_LOG("Open file fail, helper null, userId is %{public}d", userId);
        return E_FAIL;
    }
    uri = MediaUriUtils::GetMultiUri(uri, userId);
    return GetDataShareHelperByUser(userId)->OpenFile(uri, mode);
}

int MediaDataShareClient::Update(Uri &uri, const DataShare::DataSharePredicates &predicates,
    const DataShare::DataShareValuesBucket &value, const int32_t userId)
{
    if (!IsValid(userId)) {
        NAPI_ERR_LOG("update fail, helper null, userId is %{public}d", userId);
        return E_FAIL;
    }
    return GetDataShareHelperByUser(userId)->Update(uri, predicates, value);
}

void MediaDataShareClient::RegisterObserverExt(const Uri &uri,
    std::shared_ptr<DataShare::DataShareObserver> dataObserver, bool isDescendants)
{
    if (!IsValid(GetUserId())) {
        NAPI_ERR_LOG("register observer fail, helper null, userId is %{public}d", GetUserId());
        return;
    }
    GetDataShareHelperByUser(GetUserId())->RegisterObserverExt(uri, std::move(dataObserver), isDescendants);
}

void MediaDataShareClient::UnregisterObserverExt(const Uri &uri,
    std::shared_ptr<DataShare::DataShareObserver> dataObserver)
{
    if (!IsValid(GetUserId())) {
        NAPI_ERR_LOG("unregister observer fail, helper null, userId is %{public}d", GetUserId());
        return;
    }
    GetDataShareHelperByUser(GetUserId())->UnregisterObserverExt(uri, std::move(dataObserver));
}

std::string MediaDataShareClient::GetType(Uri &uri)
{
    if (!IsValid(GetUserId())) {
        NAPI_ERR_LOG("get type fail, helper null, userId is %{public}d", GetUserId());
        return "";
    }
    return GetDataShareHelperByUser(GetUserId())->GetType(uri);
}

int32_t MediaDataShareClient::RegisterObserverExtProvider(const Uri &uri,
    std::shared_ptr<DataShare::DataShareObserver> dataObserver, bool isDescendants)
{
    if (!IsValid(GetUserId())) {
        NAPI_ERR_LOG("register observer fail, helper null, userId is %{public}d", GetUserId());
        return E_FAIL;
    }
    return
        GetDataShareHelperByUser(GetUserId())->RegisterObserverExtProvider(uri, std::move(dataObserver), isDescendants);
}

int32_t MediaDataShareClient::UnregisterObserverExtProvider(const Uri &uri,
    std::shared_ptr<DataShare::DataShareObserver> dataObserver)
{
    if (!IsValid(GetUserId())) {
        NAPI_ERR_LOG("unregister observer fail, helper null, userId is %{public}d", GetUserId());
        return E_FAIL;
    }
    return GetDataShareHelperByUser(GetUserId())->UnregisterObserverExtProvider(uri, std::move(dataObserver));
}

std::shared_ptr<DataShare::DataShareResultSet> MediaDataShareClient::QueryWithoutIpc(
    const DataShare::DataSharePredicates &predicates, std::vector<std::string> &columns, OperationObject &object,
    int &errCode)
{
    return nullptr;
}

bool MediaDataShareClient::IsNoIpc(Uri &uri, OperationObject &object, const DataShare::DataSharePredicates &predicates,
    bool isIgnoreSELinux)
{
    return false;
}
// LCOV_EXCL_STOP
}
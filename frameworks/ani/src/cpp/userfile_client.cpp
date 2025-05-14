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

#include "userfile_client.h"

#include "ability.h"
#include "ani_base_context.h"
#include "media_asset_rdbstore.h"
#include "medialibrary_errno.h"
#include "medialibrary_ani_log.h"
#include "medialibrary_ani_utils.h"
#include "medialibrary_helper_container.h"
#include "media_file_utils.h"
#include "userfilemgr_uri.h"
#include "safe_map.h"

using namespace std;
using namespace OHOS::DataShare;
using namespace OHOS::AppExecFwk;
using namespace OHOS::AbilityRuntime;

namespace OHOS {
namespace Media {

int32_t UserFileClient::userId_ = -1;
std::string MULTI_USER_URI_FLAG = "user=";
std::string USER_STR = "user";
SafeMap<int32_t, std::shared_ptr<DataShare::DataShareHelper>> UserFileClient::dataShareHelperMap_ = {};

static std::string GetMediaLibraryDataUri(const int32_t userId)
{
    std::string mediaLibraryDataUri = MEDIALIBRARY_DATA_URI;
    if (userId != -1) {
        mediaLibraryDataUri = mediaLibraryDataUri + "?" + MULTI_USER_URI_FLAG + to_string(userId);
    }
    return mediaLibraryDataUri;
}

static Uri MultiUserUriRecognition(Uri &uri, const int32_t userId)
{
    if (userId == -1) {
        return uri;
    }
    std::string uriString = uri.ToString();
    MediaLibraryAniUtils::UriAppendKeyValue(uriString, USER_STR, to_string(userId));
    return Uri(uriString);
}

static void DataShareCreator(const sptr<IRemoteObject> &token,
    shared_ptr<DataShare::DataShareHelper> &dataShareHelper, const int32_t userId)
{
    dataShareHelper = DataShare::DataShareHelper::Creator(token, GetMediaLibraryDataUri(userId));
    if (dataShareHelper == nullptr) {
        ANI_ERR_LOG("dataShareHelper Creator failed");
        dataShareHelper = DataShare::DataShareHelper::Creator(token, GetMediaLibraryDataUri(userId));
    }
}

shared_ptr<DataShare::DataShareHelper> UserFileClient::GetDataShareHelper(ani_env *env,
    ani_object object, const int32_t userId)
{
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper = nullptr;
    auto context = GetStageModeContext(env, object);
    if (context == nullptr) {
        ANI_ERR_LOG("Failed to get native stage context instance");
        return nullptr;
    }
    DataShareCreator(context->GetToken(), dataShareHelper, userId);
    auto container = MediaLibraryHelperContainer::GetInstance();
    if (container == nullptr) {
        ANI_ERR_LOG("Failed to get native stage container instance");
        return nullptr;
    }
    container->SetDataShareHelper(dataShareHelper);
    return dataShareHelper;
}

ani_status UserFileClient::CheckIsStage(ani_env *env, ani_object object, bool &result)
{
    ani_boolean isStageMode = false;
    CHECK_STATUS_RET(IsStageContext(env, object, isStageMode), "IsStageContext failed.");
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetBool(env, isStageMode, result), "GetBool failed.");
    return ANI_OK;
}

sptr<IRemoteObject> UserFileClient::ParseTokenInStageMode(ani_env *env, ani_object object)
{
    auto context = GetStageModeContext(env, object);
    if (context == nullptr) {
        ANI_ERR_LOG("Failed to get native stage context instance");
        return nullptr;
    }
    return context->GetToken();
}

sptr<IRemoteObject> UserFileClient::ParseTokenInAbility(ani_env *env, ani_object object)
{
    auto ability = GetCurrentAbility(env);
    if (ability == nullptr) {
        ANI_ERR_LOG("Failed to get native ability instance");
        return nullptr;
    }
    auto context = ability->GetContext();
    if (context == nullptr) {
        ANI_ERR_LOG("Failed to get native context instance");
        return nullptr;
    }
    return context->GetToken();
}

bool UserFileClient::IsValid(const int32_t userId)
{
    std::shared_ptr<DataShare::DataShareHelper> helper;
    if (dataShareHelperMap_.Find(userId, helper)) {
        return helper != nullptr;
    }
    return false;
}

std::shared_ptr<DataShare::DataShareHelper> UserFileClient::GetDataShareHelperByUser(const int32_t userId)
{
    return dataShareHelperMap_.ReadVal(userId);
}

void UserFileClient::Init(const sptr<IRemoteObject> &token, bool isSetHelper, const int32_t userId)
{
    if (GetDataShareHelperByUser(userId) == nullptr) {
        std::shared_ptr<DataShare::DataShareHelper> dataShareHelper =
            DataShare::DataShareHelper::Creator(token, GetMediaLibraryDataUri(userId));
        if (isSetHelper) {
            auto container = MediaLibraryHelperContainer::GetInstance();
            if (container == nullptr) {
                ANI_ERR_LOG("Failed to get native stage container instance");
                return;
            }
            container->SetDataShareHelper(dataShareHelper);
        }
        if (dataShareHelper != nullptr) {
            if (!IsValid(userId)) {
                dataShareHelperMap_.EnsureInsert(userId, dataShareHelper);
            } else {
                ANI_ERR_LOG("dataShareHelperMap has userId and value");
            }
        } else {
            ANI_ERR_LOG("Failed to getDataShareHelper, dataShareHelper is null");
        }
    }
}

void UserFileClient::Init(ani_env *env, ani_object object, const int32_t userId)
{
    if (GetDataShareHelperByUser(userId) == nullptr) {
        std::shared_ptr<DataShare::DataShareHelper> dataShareHelper = GetDataShareHelper(env, object, userId);
        if (dataShareHelper != nullptr) {
            if (!IsValid(userId)) {
                dataShareHelperMap_.EnsureInsert(userId, dataShareHelper);
            } else {
                ANI_ERR_LOG("dataShareHelperMap has userId and value");
            }
        } else {
            ANI_ERR_LOG("Failed to getDataShareHelper, dataShareHelper is null");
        }
    }
}

shared_ptr<DataShareResultSet> UserFileClient::Query(Uri &uri, const DataSharePredicates &predicates,
    std::vector<std::string> &columns, int &errCode, const int32_t userId)
{
    if (!IsValid(userId)) {
        ANI_ERR_LOG("Query fail, helper null, userId is %{public}d", userId);
        return nullptr;
    }

    shared_ptr<DataShareResultSet> resultSet = nullptr;
    OperationObject object = OperationObject::UNKNOWN_OBJECT;
    if (MediaAssetRdbStore::GetInstance()->IsQueryAccessibleViaSandBox(uri, object, predicates) && userId == -1) {
        resultSet = MediaAssetRdbStore::GetInstance()->Query(predicates, columns, object, errCode);
    } else {
        uri = MultiUserUriRecognition(uri, userId);
        DatashareBusinessError businessError;
        auto result = GetDataShareHelperByUser(userId);
        CHECK_COND_RET(result != nullptr, nullptr, "result is nullptr");
        resultSet = result->Query(uri, predicates, columns, &businessError);
        errCode = businessError.GetCode();
    }
    return resultSet;
}
std::shared_ptr<NativeRdb::ResultSet> UserFileClient::QueryRdb(Uri &uri,
    const DataShare::DataSharePredicates &predicates, std::vector<std::string> &columns)
{
    shared_ptr<NativeRdb::ResultSet> resultSet = nullptr;
    OperationObject object = OperationObject::UNKNOWN_OBJECT;
    auto container = MediaAssetRdbStore::GetInstance();
    if (container == nullptr) {
        ANI_ERR_LOG("Failed to get native stage containerRdb instance");
        return nullptr;
    }
    if (container->IsSupportSharedAssetQuery(uri, object)) {
        resultSet = container->QueryRdb(predicates, columns, object);
    }
    return resultSet;
}

int UserFileClient::Insert(Uri &uri, const DataShareValuesBucket &value, const int32_t userId)
{
    if (!IsValid(userId)) {
        ANI_ERR_LOG("insert fail, helper null, userId is %{public}d", userId);
        return E_FAIL;
    }
    auto helper = GetDataShareHelperByUser(userId);
    if (helper == nullptr) {
        ANI_ERR_LOG("insert fail, helper null, userId is %{public}d", userId);
        return E_FAIL;
    }
    int index = helper->Insert(uri, value);
    return index;
}

int UserFileClient::InsertExt(Uri &uri, const DataShareValuesBucket &value, string &result, const int32_t userId)
{
    if (!IsValid(userId)) {
        ANI_ERR_LOG("insert fail, helper null, userId is %{public}d", userId);
        return E_FAIL;
    }
    auto helper = GetDataShareHelperByUser(userId);
    if (helper == nullptr) {
        ANI_ERR_LOG("insert fail, helper null, userId is %{public}d", userId);
        return E_FAIL;
    }
    int index = helper->InsertExt(uri, value, result);
    return index;
}

int UserFileClient::BatchInsert(Uri &uri, const std::vector<DataShare::DataShareValuesBucket> &values)
{
    if (!IsValid(GetUserId())) {
        ANI_ERR_LOG("Batch insert fail, helper null, userId is %{public}d", GetUserId());
        return E_FAIL;
    }
    auto helper = GetDataShareHelperByUser(GetUserId());
    if (helper == nullptr) {
        ANI_ERR_LOG("Batch insert fail, helper null, userId is %{public}d", GetUserId());
        return E_FAIL;
    }
    return helper->BatchInsert(uri, values);
}

int UserFileClient::Delete(Uri &uri, const DataSharePredicates &predicates)
{
    if (!IsValid(GetUserId())) {
        ANI_ERR_LOG("delete fail, helper null, userId is %{public}d", GetUserId());
        return E_FAIL;
    }
    auto helper = GetDataShareHelperByUser(GetUserId());
    if (helper == nullptr) {
        ANI_ERR_LOG("Batch insert fail, helper null, userId is %{public}d", GetUserId());
        return E_FAIL;
    }
    return helper->Delete(uri, predicates);
}

void UserFileClient::NotifyChange(const Uri &uri)
{
    if (!IsValid(GetUserId())) {
        ANI_ERR_LOG("notify change fail, helper null, userId is %{public}d", GetUserId());
        return;
    }
    auto helper = GetDataShareHelperByUser(GetUserId());
    if (helper == nullptr) {
        ANI_ERR_LOG("Batch insert fail, helper null, userId is %{public}d", GetUserId());
        return;
    }
    helper->NotifyChange(uri);
}

void UserFileClient::RegisterObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver)
{
    if (!IsValid(GetUserId())) {
        ANI_ERR_LOG("register observer fail, helper null, userId is %{public}d", GetUserId());
        return;
    }
    auto helper = GetDataShareHelperByUser(GetUserId());
    if (helper == nullptr) {
        ANI_ERR_LOG("Batch insert fail, helper null, userId is %{public}d", GetUserId());
        return;
    }
    helper->RegisterObserver(uri, dataObserver);
}

void UserFileClient::UnregisterObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver)
{
    if (!IsValid(GetUserId())) {
        ANI_ERR_LOG("unregister observer fail, helper null, userId is %{public}d", GetUserId());
        return;
    }
    auto helper = GetDataShareHelperByUser(GetUserId());
    if (helper == nullptr) {
        ANI_ERR_LOG("Batch insert fail, helper null, userId is %{public}d", GetUserId());
        return;
    }
    helper->UnregisterObserver(uri, dataObserver);
}

int UserFileClient::OpenFile(Uri &uri, const std::string &mode, const int32_t userId)
{
    if (!IsValid(userId)) {
        ANI_ERR_LOG("Open file fail, helper null, userId is %{public}d", userId);
        return E_FAIL;
    }
    auto helper = GetDataShareHelperByUser(userId);
    if (helper == nullptr) {
        ANI_ERR_LOG("Open file fail, helper null, userId is %{public}d", userId);
        return E_FAIL;
    }
    uri = MultiUserUriRecognition(uri, userId);
    return helper->OpenFile(uri, mode);
}

int UserFileClient::Update(Uri &uri, const DataSharePredicates &predicates,
    const DataShareValuesBucket &value, const int32_t userId)
{
    if (!IsValid(userId)) {
        ANI_ERR_LOG("update fail, helper null, userId is %{public}d", userId);
        return E_FAIL;
    }
    auto helper = GetDataShareHelperByUser(userId);
    if (helper == nullptr) {
        ANI_ERR_LOG("update fail, helper null, userId is %{public}d", userId);
        return E_FAIL;
    }
    return helper->Update(uri, predicates, value);
}

void UserFileClient::RegisterObserverExt(const Uri &uri,
    shared_ptr<DataShare::DataShareObserver> dataObserver, bool isDescendants)
{
    if (!IsValid(GetUserId())) {
        ANI_ERR_LOG("register observer fail, helper null, userId is %{public}d", GetUserId());
        return;
    }
    auto helper = GetDataShareHelperByUser(GetUserId());
    if (helper == nullptr) {
        ANI_ERR_LOG("Batch insert fail, helper null, userId is %{public}d", GetUserId());
        return;
    }
    helper->RegisterObserverExt(uri, std::move(dataObserver), isDescendants);
}

void UserFileClient::UnregisterObserverExt(const Uri &uri, std::shared_ptr<DataShare::DataShareObserver> dataObserver)
{
    if (!IsValid(GetUserId())) {
        ANI_ERR_LOG("unregister observer fail, helper null, userId is %{public}d", GetUserId());
        return;
    }
    auto helper = GetDataShareHelperByUser(GetUserId());
    if (helper == nullptr) {
        ANI_ERR_LOG("Batch insert fail, helper null, userId is %{public}d", GetUserId());
        return;
    }
    helper->UnregisterObserverExt(uri, std::move(dataObserver));
}

std::string UserFileClient::GetType(Uri &uri)
{
    if (!IsValid(GetUserId())) {
        ANI_ERR_LOG("get type fail, helper null, userId is %{public}d", GetUserId());
        return "";
    }
    auto helper = GetDataShareHelperByUser(GetUserId());
    if (helper == nullptr) {
        ANI_ERR_LOG("Batch insert fail, helper null, userId is %{public}d", GetUserId());
        return "";
    }
    return helper->GetType(uri);
}

void UserFileClient::Clear()
{
    dataShareHelperMap_.Clear();
}

void UserFileClient::SetUserId(const int32_t userId)
{
    userId_ = userId;
}

int32_t UserFileClient::GetUserId()
{
    return userId_;
}
} // namespace Media
} // namespace OHOS

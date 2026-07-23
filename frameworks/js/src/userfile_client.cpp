/*
 * Copyright (C) 2022-2024 Huawei Device Co., Ltd.
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
#include "iservice_registry.h"

#include "media_asset_rdbstore.h"
#include "medialibrary_errno.h"
#include "medialibrary_napi_log.h"
#include "medialibrary_helper_container.h"
#include "media_file_utils.h"
#include "safe_map.h"

// LCOV_EXCL_START
using namespace std;
using namespace OHOS::DataShare;
using namespace OHOS::AppExecFwk;
namespace OHOS {
namespace Media {

constexpr int32_t BUNDLE_MGR_SERVICE_SYS_ABILITY_ID = 401;
int32_t UserFileClient::userId_ = -1;
string UserFileClient::bundleName_ = "";
std::string MULTI_USER_URI_FLAG = "user=";
std::string USER_STR = "user";

SafeMap<int32_t, std::shared_ptr<DataShare::DataShareHelper>> UserFileClient::dataShareHelperMap_ = {};
sptr<AppExecFwk::IBundleMgr> UserFileClient::bundleMgr_ = nullptr;
mutex UserFileClient::bundleMgrMutex_;
static std::string GetMediaLibraryDataUri(const int32_t userId)
{
    std::string mediaLibraryDataUri = MEDIALIBRARY_DATA_URI;
    if (userId != -1) {
        mediaLibraryDataUri = mediaLibraryDataUri + "?" + MULTI_USER_URI_FLAG + to_string(userId);
    }
    return mediaLibraryDataUri;
}

void UriAppendKeyValue(std::string &uri, const std::string &key, const std::string &value)
{
    std::string uriKey = key + '=';
    if (uri.find(uriKey) != std::string::npos) {
        return;
    }

    char queryMark = (uri.find('?') == std::string::npos) ? '?' : '&';
    std::string append = queryMark + key + '=' + value;

    size_t posJ = uri.find('#');
    if (posJ == std::string::npos) {
        uri += append;
    } else {
        uri.insert(posJ, append);
    }
}

static Uri MultiUserUriRecognition(Uri &uri, const int32_t userId)
{
    if (userId == -1) {
        return uri;
    }
    std::string uriString = uri.ToString();
    UriAppendKeyValue(uriString, USER_STR, to_string(userId));
    return Uri(uriString);
}

static void DataShareCreator(const sptr<IRemoteObject> &token,
    shared_ptr<DataShare::DataShareHelper> &dataShareHelper, const int32_t userId)
{
    dataShareHelper = DataShare::DataShareHelper::Creator(token, GetMediaLibraryDataUri(userId));
    if (dataShareHelper == nullptr) {
        NAPI_ERR_LOG("dataShareHelper Creator failed");
        dataShareHelper = DataShare::DataShareHelper::Creator(token, GetMediaLibraryDataUri(userId));
    }
}

shared_ptr<DataShare::DataShareHelper> UserFileClient::GetDataShareHelper(napi_env env,
    napi_callback_info info, const int32_t userId)
{
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr));

    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper = nullptr;
    bool isStageMode = false;
    napi_status status = OHOS::AbilityRuntime::IsStageContext(env, argv[0], isStageMode);
    if (status != napi_ok || !isStageMode) {
        NAPI_INFO_LOG("status: %{public}d, isStageMode: %{public}d", status, static_cast<int32_t>(isStageMode));
        auto ability = OHOS::AbilityRuntime::GetCurrentAbility(env);
        if (ability == nullptr) {
            NAPI_ERR_LOG("Failed to get native ability instance");
            return nullptr;
        }
        auto context = ability->GetContext();
        if (context == nullptr) {
            NAPI_ERR_LOG("Failed to get native context instance");
            return nullptr;
        }
        DataShareCreator(context->GetToken(), dataShareHelper, userId);
    } else {
        auto context = OHOS::AbilityRuntime::GetStageModeContext(env, argv[0]);
        if (context == nullptr) {
            NAPI_ERR_LOG("Failed to get native stage context instance");
            return nullptr;
        }
        DataShareCreator(context->GetToken(), dataShareHelper, userId);
    }
    MediaLibraryHelperContainer::GetInstance()->SetDataShareHelper(dataShareHelper);
    return dataShareHelper;
}

napi_status UserFileClient::CheckIsStage(napi_env env, napi_callback_info info, bool &result)
{
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Failed to get cb info, status=%{public}d", (int) status);
        return status;
    }

    result = false;
    status = OHOS::AbilityRuntime::IsStageContext(env, argv[0], result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Failed to get stage mode, status=%{public}d", (int) status);
        return status;
    }
    return napi_ok;
}

sptr<IRemoteObject> UserFileClient::ParseTokenInStageMode(napi_env env, napi_callback_info info)
{
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    if (napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr) != napi_ok) {
        NAPI_ERR_LOG("Failed to get cb info");
        return nullptr;
    }

    auto context = OHOS::AbilityRuntime::GetStageModeContext(env, argv[0]);
    if (context == nullptr) {
        NAPI_ERR_LOG("Failed to get native stage context instance");
        return nullptr;
    }
    return context->GetToken();
}

sptr<IRemoteObject> UserFileClient::ParseTokenInAbility(napi_env env, napi_callback_info info)
{
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    if (napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr) != napi_ok) {
        NAPI_ERR_LOG("Failed to get cb info");
        return nullptr;
    }

    auto ability = OHOS::AbilityRuntime::GetCurrentAbility(env);
    if (ability == nullptr) {
        NAPI_ERR_LOG("Failed to get native ability instance");
        return nullptr;
    }
    auto context = ability->GetContext();
    if (context == nullptr) {
        NAPI_ERR_LOG("Failed to get native context instance");
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
            MediaLibraryHelperContainer::GetInstance()->SetDataShareHelper(dataShareHelper);
        }
        if (dataShareHelper != nullptr) {
            if (!IsValid(userId)) {
                dataShareHelperMap_.EnsureInsert(userId, dataShareHelper);
            } else {
                NAPI_ERR_LOG("dataShareHelperMap has userId and value");
            }
        } else {
            NAPI_ERR_LOG("Failed to getDataShareHelper, dataShareHelper is null");
        }
    }
}

void UserFileClient::Init(napi_env env, napi_callback_info info, const int32_t userId)
{
    if (GetDataShareHelperByUser(userId) == nullptr) {
        std::shared_ptr<DataShare::DataShareHelper> dataShareHelper = GetDataShareHelper(env, info, userId);
        if (dataShareHelper != nullptr) {
            if (!IsValid(userId)) {
                dataShareHelperMap_.EnsureInsert(userId, dataShareHelper);
            } else {
                NAPI_ERR_LOG("dataShareHelperMap has userId and value");
            }
        } else {
            NAPI_ERR_LOG("Failed to getDataShareHelper, dataShareHelper is null");
        }
    }
}

shared_ptr<DataShareResultSet> UserFileClient::Query(Uri &uri, const DataSharePredicates &predicates,
    std::vector<std::string> &columns, int &errCode, const int32_t userId)
{
    if (!IsValid(userId)) {
        NAPI_ERR_LOG("Query fail, helper null, userId is %{public}d", userId);
        return nullptr;
    }

    shared_ptr<DataShareResultSet> resultSet = nullptr;
    OperationObject object = OperationObject::UNKNOWN_OBJECT;
    if (MediaAssetRdbStore::GetInstance()->IsQueryAccessibleViaSandBox(uri, object, predicates) && userId == -1) {
        resultSet = MediaAssetRdbStore::GetInstance()->Query(predicates, columns, object, errCode);
    } else {
        uri = MultiUserUriRecognition(uri, userId);
        DatashareBusinessError businessError;
        resultSet = GetDataShareHelperByUser(userId)->Query(uri, predicates, columns, &businessError);
        errCode = businessError.GetCode();
    }
    return resultSet;
}

shared_ptr<NativeRdb::ResultSet> UserFileClient::QueryByStep(const std::string &sql)
{
    std::shared_ptr<NativeRdb::ResultSet> resultSet = MediaAssetRdbStore::GetInstance()->QueryByStep(sql);
    return resultSet;
}

std::pair<bool, shared_ptr<DataShareResultSet>> UserFileClient::QueryAccessibleViaSandBox(Uri &uri,
    const DataSharePredicates &predicates, std::vector<std::string> &columns, int &errCode, const int32_t userId)
{
    OperationObject object = OperationObject::UNKNOWN_OBJECT;
    if (MediaAssetRdbStore::GetInstance()->IsQueryAccessibleViaSandBox(uri, object, predicates) && userId == -1) {
        return {true, MediaAssetRdbStore::GetInstance()->Query(predicates, columns, object, errCode)};
    }
    return {false, nullptr};
}

std::shared_ptr<NativeRdb::ResultSet> UserFileClient::QueryRdb(Uri &uri,
    const DataShare::DataSharePredicates &predicates, std::vector<std::string> &columns)
{
    shared_ptr<NativeRdb::ResultSet> resultSet = nullptr;
    OperationObject object = OperationObject::UNKNOWN_OBJECT;
    if (MediaAssetRdbStore::GetInstance()->IsSupportSharedAssetQuery(uri, object)) {
        resultSet = MediaAssetRdbStore::GetInstance()->QueryRdb(predicates, columns, object);
    }
    return resultSet;
}

int UserFileClient::Insert(Uri &uri, const DataShareValuesBucket &value, const int32_t userId)
{
    if (!IsValid(userId)) {
        NAPI_ERR_LOG("insert fail, helper null, userId is %{public}d", userId);
        return E_FAIL;
    }
    int index = GetDataShareHelperByUser(userId)->Insert(uri, value);
    return index;
}

int UserFileClient::InsertExt(Uri &uri, const DataShareValuesBucket &value, string &result, const int32_t userId)
{
    if (!IsValid(userId)) {
        NAPI_ERR_LOG("insert fail, helper null, userId is %{public}d", userId);
        return E_FAIL;
    }
    int index = GetDataShareHelperByUser(userId)->InsertExt(uri, value, result);
    return index;
}

int UserFileClient::BatchInsert(Uri &uri, const std::vector<DataShare::DataShareValuesBucket> &values)
{
    if (!IsValid(GetUserId())) {
        NAPI_ERR_LOG("Batch insert fail, helper null, userId is %{public}d", GetUserId());
        return E_FAIL;
    }
    return GetDataShareHelperByUser(GetUserId())->BatchInsert(uri, values);
}

int UserFileClient::Delete(Uri &uri, const DataSharePredicates &predicates)
{
    if (!IsValid(GetUserId())) {
        NAPI_ERR_LOG("delete fail, helper null, userId is %{public}d", GetUserId());
        return E_FAIL;
    }
    return GetDataShareHelperByUser(GetUserId())->Delete(uri, predicates);
}

void UserFileClient::NotifyChange(const Uri &uri)
{
    if (!IsValid(GetUserId())) {
        NAPI_ERR_LOG("notify change fail, helper null, userId is %{public}d", GetUserId());
        return;
    }
    GetDataShareHelperByUser(GetUserId())->NotifyChange(uri);
}

void UserFileClient::RegisterObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver)
{
    if (!IsValid(GetUserId())) {
        NAPI_ERR_LOG("register observer fail, helper null, userId is %{public}d", GetUserId());
        return;
    }
    GetDataShareHelperByUser(GetUserId())->RegisterObserver(uri, dataObserver);
}

void UserFileClient::UnregisterObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver)
{
    if (!IsValid(GetUserId())) {
        NAPI_ERR_LOG("unregister observer fail, helper null, userId is %{public}d", GetUserId());
        return;
    }
    GetDataShareHelperByUser(GetUserId())->UnregisterObserver(uri, dataObserver);
}

int UserFileClient::OpenFile(Uri &uri, const std::string &mode, const int32_t userId)
{
    if (!IsValid(userId)) {
        NAPI_ERR_LOG("Open file fail, helper null, userId is %{public}d", userId);
        return E_FAIL;
    }
    uri = MultiUserUriRecognition(uri, userId);
    return GetDataShareHelperByUser(userId)->OpenFile(uri, mode);
}

int UserFileClient::OpenFileWithErrCode(Uri &uri, const std::string &mode, int32_t &realErr, const int32_t userId)
{
    if (!IsValid(userId)) {
        NAPI_ERR_LOG("Open file fail, helper null, userId is %{public}d", userId);
        return E_FAIL;
    }
    uri = MultiUserUriRecognition(uri, userId);
    return GetDataShareHelperByUser(userId)->OpenFileWithErrCode(uri, mode, realErr);
}

int UserFileClient::Update(Uri &uri, const DataSharePredicates &predicates,
    const DataShareValuesBucket &value, const int32_t userId)
{
    if (!IsValid(userId)) {
        NAPI_ERR_LOG("update fail, helper null, userId is %{public}d", userId);
        return E_FAIL;
    }
    return GetDataShareHelperByUser(userId)->Update(uri, predicates, value);
}

void UserFileClient::RegisterObserverExt(const Uri &uri,
    shared_ptr<DataShare::DataShareObserver> dataObserver, bool isDescendants)
{
    if (!IsValid(GetUserId())) {
        NAPI_ERR_LOG("register observer fail, helper null, userId is %{public}d", GetUserId());
        return;
    }
    GetDataShareHelperByUser(GetUserId())->RegisterObserverExt(uri, std::move(dataObserver), isDescendants);
}

void UserFileClient::UnregisterObserverExt(const Uri &uri, std::shared_ptr<DataShare::DataShareObserver> dataObserver)
{
    if (!IsValid(GetUserId())) {
        NAPI_ERR_LOG("unregister observer fail, helper null, userId is %{public}d", GetUserId());
        return;
    }
    GetDataShareHelperByUser(GetUserId())->UnregisterObserverExt(uri, std::move(dataObserver));
}

std::string UserFileClient::GetType(Uri &uri)
{
    if (!IsValid(GetUserId())) {
        NAPI_ERR_LOG("get type fail, helper null, userId is %{public}d", GetUserId());
        return "";
    }
    return GetDataShareHelperByUser(GetUserId())->GetType(uri);
}

int32_t UserFileClient::UserDefineFunc(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (!IsValid(GetUserId())) {
        NAPI_ERR_LOG("JS UserDefineFunc fail, helper null %{public}d", GetUserId());
        return E_FAIL;
    }
    return GetDataShareHelperByUser(GetUserId())->UserDefineFunc(data, reply, option);
}

int32_t UserFileClient::UserDefineFunc(const int32_t &userId, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    if (!IsValid(userId)) {
        NAPI_ERR_LOG("JS UserDefineFunc fail, helper null %{public}d", userId);
        return E_FAIL;
    }
    return GetDataShareHelperByUser(userId)->UserDefineFunc(data, reply, option);
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

sptr<AppExecFwk::IBundleMgr> UserFileClient::GetSysBundleManager()
{
    lock_guard<mutex> lock(bundleMgrMutex_);
    if (bundleMgr_ != nullptr) {
        return bundleMgr_;
    }

    auto systemAbilityMgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityMgr == nullptr) {
        NAPI_ERR_LOG("Failed to get SystemAbilityManager.");
        return nullptr;
    }

    auto bundleObj = systemAbilityMgr->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (bundleObj == nullptr) {
        NAPI_ERR_LOG("Remote object is nullptr.");
        return nullptr;
    }

    bundleMgr_ = iface_cast<AppExecFwk::IBundleMgr>(bundleObj);
    if (bundleMgr_ == nullptr) {
        NAPI_ERR_LOG("Failed to iface_cast");
        return nullptr;
    }

    return bundleMgr_;
}

string UserFileClient::GetBundleName()
{
    if (bundleName_ != "") {
        return bundleName_;
    }
    bundleMgr_ = GetSysBundleManager();
    if (bundleMgr_ == nullptr) {
        NAPI_ERR_LOG("bundleMgr_ is null");
        return bundleName_;
    }
    int32_t uid = static_cast<int32_t>(getuid());
    string bundleName;
    auto result = bundleMgr_->GetBundleNameForUid(uid, bundleName);
    if (!result) {
        NAPI_ERR_LOG("result is false");
        return bundleName_;
    }
    NAPI_INFO_LOG("hap bundleName: %{public}s", bundleName.c_str());
    bundleName_ = bundleName;
    return bundleName_;
}

int32_t UserFileClient::RegisterObserverExtProvider(const Uri &uri,
    std::shared_ptr<DataShare::DataShareObserver> dataObserver, bool isDescendants)
{
    if (!IsValid(GetUserId())) {
        NAPI_ERR_LOG("register observer fail, helper null, userId is %{public}d", GetUserId());
        return E_FAIL;
    }
    return
        GetDataShareHelperByUser(GetUserId())->RegisterObserverExtProvider(uri, std::move(dataObserver), isDescendants);
}

int32_t UserFileClient::UnregisterObserverExtProvider(const Uri &uri,
    std::shared_ptr<DataShare::DataShareObserver> dataObserver)
{
    if (!IsValid(GetUserId())) {
        NAPI_ERR_LOG("unregister observer fail, helper null, userId is %{public}d", GetUserId());
        return E_FAIL;
    }
    return GetDataShareHelperByUser(GetUserId())->UnregisterObserverExtProvider(uri, std::move(dataObserver));
}
}
}
// LCOV_EXCL_STOP

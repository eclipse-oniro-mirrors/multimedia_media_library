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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_USER_FILE_CLIENT_H
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_USER_FILE_CLIENT_H

#include "media_common_client.h"
#include "media_client_utils.h"
#include "datashare_helper.h"
#include "datashare_predicates.h"
#include "napi_base_context.h"
#include "ability.h"
#include "napi_error.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_remote_object.h"
#include "rdb_store.h"
#include "uri.h"
#include "safe_map.h"
#include "bundle_mgr_interface.h"
#include <mutex>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

// UserFileClient is now a thin static wrapper around MediaCommonClient.
// All DataShare operations are delegated to the unified MediaCommonClient singleton,
// which manages multi-user DataShareHelper instances via dataShareHelperMap_.
class UserFileClient {
public:
    EXPORT UserFileClient() {}
    EXPORT virtual ~UserFileClient() {}

    EXPORT static bool IsValid(const int32_t userId = -1)
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().IsValid(userId);
    }

    EXPORT static napi_status CheckIsStage(napi_env env, napi_callback_info info, bool &result)
    {
        size_t argc = 1;
        napi_value argv[1] = {0};
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
        }
        return status;
    }

    EXPORT static sptr<IRemoteObject> ParseTokenInStageMode(napi_env env, napi_callback_info info)
    {
        size_t argc = 1;
        napi_value argv[1] = {0};
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

    EXPORT static sptr<IRemoteObject> ParseTokenInAbility(napi_env env, napi_callback_info info)
    {
        size_t argc = 1;
        napi_value argv[1] = {0};
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

    EXPORT static void Init(const sptr<IRemoteObject> &token, bool isSetHelper = false,
        const int32_t userId = -1)
    {
        auto &client = OHOS::Media::IPC::MediaCommonClient::GetInstance();
        client.Init(token, userId);
        client.SetUserId(userId);
    }

    EXPORT static void Init(napi_env env, napi_callback_info info, const int32_t userId = -1)
    {
        sptr<IRemoteObject> token;
        bool isStage = false;
        CheckIsStage(env, info, isStage);
        
        if (isStage) {
            token = ParseTokenInStageMode(env, info);
        } else {
            token = ParseTokenInAbility(env, info);
        }
        if (token == nullptr) {
            NAPI_ERR_LOG("InitFromNapi: failed to parse token from napi env");
            return;
        }

        auto &client = OHOS::Media::IPC::MediaCommonClient::GetInstance();
        client.Init(token, userId);
        client.SetUserId(userId);
    }

    EXPORT static std::shared_ptr<DataShare::DataShareResultSet> Query(Uri &uri,
        const DataShare::DataSharePredicates &predicates, std::vector<std::string> &columns,
        int &errCode, const int32_t userId = -1)
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().Query(
            uri, predicates, columns, errCode, userId);
    }

    EXPORT static std::pair<bool, std::shared_ptr<DataShare::DataShareResultSet>> QueryAccessibleViaSandBox(
        Uri &uri, const DataShare::DataSharePredicates &predicates, std::vector<std::string> &columns,
        int &errCode, const int32_t userId = -1)
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().QueryAccessibleViaSandBox(
            uri, predicates, columns, errCode, userId);
    }

    EXPORT static int Insert(Uri &uri, const DataShare::DataShareValuesBucket &value,
        const int32_t userId = -1)
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().Insert(uri, value, userId);
    }

    EXPORT static int InsertExt(Uri &uri, const DataShare::DataShareValuesBucket &value,
        std::string &result, const int32_t userId = -1)
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().InsertExt(uri, value, result, userId);
    }

    EXPORT static int BatchInsert(Uri &uri, const std::vector<DataShare::DataShareValuesBucket> &values)
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().BatchInsert(uri, values);
    }

    EXPORT static int Delete(Uri &uri, const DataShare::DataSharePredicates &predicates)
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().Delete(uri, predicates);
    }

    EXPORT static void NotifyChange(const Uri &uri)
    {
        OHOS::Media::IPC::MediaCommonClient::GetInstance().NotifyChange(uri);
    }

    EXPORT static void RegisterObserver(const Uri &uri,
        const sptr<AAFwk::IDataAbilityObserver> &dataObserver)
    {
        OHOS::Media::IPC::MediaCommonClient::GetInstance().RegisterObserver(uri, dataObserver);
    }

    EXPORT static void UnregisterObserver(const Uri &uri,
        const sptr<AAFwk::IDataAbilityObserver> &dataObserver)
    {
        OHOS::Media::IPC::MediaCommonClient::GetInstance().UnregisterObserver(uri, dataObserver);
    }

    EXPORT static int OpenFile(Uri &uri, const std::string &mode, const int32_t userId = -1)
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().OpenFile(uri, mode, userId);
    }

    EXPORT static int OpenFileWithErrCode(Uri &uri, const std::string &mode, int32_t &realErr,
        const int32_t userId = -1)
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().OpenFileWithErrCode(uri, mode, realErr, userId);
    }

    EXPORT static int Update(Uri &uri, const DataShare::DataSharePredicates &predicates,
        const DataShare::DataShareValuesBucket &value, const int32_t userId = -1)
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().Update(uri, predicates, value, userId);
    }

    EXPORT static void RegisterObserverExt(const Uri &uri,
        std::shared_ptr<DataShare::DataShareObserver> dataObserver, bool isDescendants)
    {
        OHOS::Media::IPC::MediaCommonClient::GetInstance().RegisterObserverExt(
            uri, std::move(dataObserver), isDescendants);
    }

    EXPORT static void UnregisterObserverExt(const Uri &uri,
        std::shared_ptr<DataShare::DataShareObserver> dataObserver)
    {
        OHOS::Media::IPC::MediaCommonClient::GetInstance().UnregisterObserverExt(uri, std::move(dataObserver));
    }

    EXPORT static std::shared_ptr<NativeRdb::ResultSet> QueryRdb(Uri &uri,
        const DataShare::DataSharePredicates &predicates, std::vector<std::string> &columns)
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().QueryRdb(uri, predicates, columns);
    }

    EXPORT static std::shared_ptr<NativeRdb::ResultSet> QueryByStep(const std::string &sql)
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().QueryByStep(sql);
    }

    EXPORT static std::string GetType(Uri &uri)
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().GetType(uri);
    }

    EXPORT static void SetUserId(const int32_t userId)
    {
        OHOS::Media::IPC::MediaCommonClient::GetInstance().SetUserId(userId);
    }

    EXPORT static int32_t GetUserId()
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().GetUserId();
    }

    EXPORT static std::shared_ptr<DataShare::DataShareHelper> GetDataShareHelperByUser(const int32_t userId)
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().GetOrCreateDataShareHelper(userId);
    }

    EXPORT static std::string GetBundleName()
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().GetBundleName();
    }

    EXPORT static void GetBundleInfo(AppExecFwk::BundleInfo &bundleInfo)
    {
        auto bundleMgr = OHOS::Media::IPC::MediaDataShareClient::GetInstance().GetSysBundleManager();
        if (bundleMgr == nullptr) {
            NAPI_ERR_LOG("bundleMgr is null");
            return;
        }
        int32_t flags = static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_ABILITY) +
            static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_SIGNATURE_INFO);
        auto ret = bundleMgr->GetBundleInfoForSelf(flags, bundleInfo);
        if (ret != ERR_OK) {
            NAPI_ERR_LOG("get bundle info failed");
        }
    }

    EXPORT static int32_t RegisterObserverExtProvider(const Uri &uri,
        std::shared_ptr<DataShare::DataShareObserver> dataObserver, bool isDescendants)
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().RegisterObserverExtProvider(
            uri, std::move(dataObserver), isDescendants);
    }

    EXPORT static int32_t UnregisterObserverExtProvider(const Uri &uri,
        std::shared_ptr<DataShare::DataShareObserver> dataObserver)
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().UnregisterObserverExtProvider(
            uri, std::move(dataObserver));
    }
};
}
}

#endif // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_USER_FILE_CLIENT_H

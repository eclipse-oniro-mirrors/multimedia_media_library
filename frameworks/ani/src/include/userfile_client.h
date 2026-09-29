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

#ifndef FRAMEWORKS_ANI_SRC_INCLUDE_USER_FILE_CLIENT_H
#define FRAMEWORKS_ANI_SRC_INCLUDE_USER_FILE_CLIENT_H

#include <memory>
#include <string>
#include <vector>

#include "ani_error.h"
#include "ani_base_context.h"
#include "datashare_helper.h"
#include "rdb_store.h"
#include "uri.h"
#include "safe_map.h"
#include "message_parcel.h"
#include "bundle_mgr_interface.h"
#include <mutex>
#include "ability.h"

#include "media_common_client.h"
#include "media_client_utils.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

// UserFileClient is now a thin static wrapper around MediaCommonClient.
// All DataShare operations are delegated to the unified MediaCommonClient singleton.
class UserFileClient {
public:
    EXPORT UserFileClient() {}
    EXPORT virtual ~UserFileClient() {}

    EXPORT static bool IsValid(const int32_t userId = -1)
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().IsValid(userId);
    }

    EXPORT static ani_status CheckIsStage(ani_env *env, ani_object object, bool &result)
    {
        // ANI only supports Stage mode
        result = true;
        return ANI_OK;
    }

    EXPORT static sptr<IRemoteObject> ParseTokenInStageMode(ani_env *env, ani_object object)
    {
        auto context = AbilityRuntime::GetStageModeContext(env, object);
        if (context == nullptr) {
            ANI_ERR_LOG("Failed to get native stage context instance");
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

    EXPORT static void Init(ani_env *env, ani_object object, const int32_t userId = -1)
    {
        auto context = OHOS::AbilityRuntime::GetStageModeContext(reinterpret_cast<ani_env*>(env),
        reinterpret_cast<ani_object>(object));
        if (context == nullptr) {
            ANI_ERR_LOG("ParseTokenFromAni: Failed to get native stage context instance");
            return;
        }
        auto token = context->GetToken();
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
        int &errCode, const int32_t userId)
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

    EXPORT static std::string GetBundleName()
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().GetBundleName();
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
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_ANI_SRC_INCLUDE_USER_FILE_CLIENT_H
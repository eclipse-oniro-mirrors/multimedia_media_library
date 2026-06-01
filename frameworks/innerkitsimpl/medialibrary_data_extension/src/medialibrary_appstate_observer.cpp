/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "medialibrary_appstate_observer.h"

#include <cstddef>
#include <sstream>
#include <string>

#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "media_app_uri_permission_column.h"
#include "media_app_uri_sensitive_column.h"
#include "media_log.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_errno.h"
#include "permission_utils.h"
#include "system_ability_definition.h"

using namespace std;

namespace OHOS {
namespace Media {
using namespace OHOS::AppExecFwk;
constexpr int32_t WAITFOR_REVOKE = 2000;

sptr<IAppMgr> MedialibraryAppStateObserverManager::GetAppManagerInstance()
{
    sptr<ISystemAbilityManager> systemAbilityManager =
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHECK_AND_RETURN_RET_LOG(systemAbilityManager != nullptr, nullptr, "systemAbilityManager is nullptr");

    sptr<IRemoteObject> object = systemAbilityManager->GetSystemAbility(APP_MGR_SERVICE_ID);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "systemAbilityManager remote object is nullptr");
    return iface_cast<IAppMgr>(object);
}

void MedialibraryAppStateObserverManager::SubscribeAppState()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_INFO_LOG("SubscribeAppState");
    sptr<IAppMgr> appManager = GetAppManagerInstance();
    CHECK_AND_RETURN_LOG(appManager != nullptr, "GetAppManagerInstance failed");
    CHECK_AND_RETURN_INFO_LOG(appStateObserver_ == nullptr, "appStateObserver has been registed");

    appStateObserver_ = new (std::nothrow) MedialibraryAppStateObserver();
    CHECK_AND_RETURN_LOG(appStateObserver_ != nullptr, "get appStateObserver failed");

    int32_t result = appManager->RegisterApplicationStateObserver(appStateObserver_);
    if (result != E_SUCCESS) {
        MEDIA_ERR_LOG("RegistApplicationStateObserver failed");
        appStateObserver_ = nullptr;
        return;
    }

    MEDIA_INFO_LOG("SubscribeAppState success");
    return;
}

void MedialibraryAppStateObserverManager::UnSubscribeAppState()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(appStateObserver_ != nullptr, "appStateObserver_ is nullptr");

    sptr<IAppMgr> appManager = GetAppManagerInstance();
    CHECK_AND_RETURN_LOG(appManager != nullptr, "GetAppManagerInstance failed");

    int32_t result = appManager->UnregisterApplicationStateObserver(appStateObserver_);
    CHECK_AND_RETURN_LOG(result == E_SUCCESS, "UnregisterApplicationStateObserver failed");

    appStateObserver_ = nullptr;
    MEDIA_INFO_LOG("UnSubscribeAppState success");
    return;
}

void MedialibraryAppStateObserverManager::AddTokenId(int64_t tokenId, bool needRevoke)
{
    revokeMap_.EnsureInsert(tokenId, needRevoke);
}

void MedialibraryAppStateObserverManager::RemoveTokenId(int64_t tokenId)
{
    revokeMap_.Erase(tokenId);
}

bool MedialibraryAppStateObserverManager::NeedRevoke(int64_t tokenId)
{
    bool needRevoke = true;
    revokeMap_.Find(tokenId, needRevoke);
    return needRevoke;
}

bool MedialibraryAppStateObserverManager::IsContainTokenId(int64_t tokenId)
{
    bool needRevoke = true;
    return revokeMap_.Find(tokenId, needRevoke);
}

MedialibraryAppStateObserverManager &MedialibraryAppStateObserverManager::GetInstance()
{
    static MedialibraryAppStateObserverManager instance;
    return instance;
}

static int32_t CountTemporaryPermission(const std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    NativeRdb::AbsRdbPredicates predicatesUnSubscribe(AppUriPermissionColumn::APP_URI_PERMISSION_TABLE);
    vector<string> permissionTypes;
    permissionTypes.emplace_back(to_string(
        static_cast<int32_t>(PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO)));
    permissionTypes.emplace_back(to_string(
        static_cast<int32_t>(PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO)));
    permissionTypes.emplace_back(to_string(
        static_cast<int32_t>(PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO)));
    predicatesUnSubscribe.And()->In(AppUriPermissionColumn::PERMISSION_TYPE, permissionTypes);
    vector<string> columns = { AppUriPermissionColumn::ID };
    auto resultSet = rdbStore->Query(predicatesUnSubscribe, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "Can not query URIPERMISSION");

    int32_t count = 0;
    auto ret = resultSet->GetRowCount(count);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_ERR, "GetRowCount failed ret:%{public}d", ret);
    return count;
}

static int32_t CountHideSensitive(const std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    NativeRdb::AbsRdbPredicates predicatesUnSubscribe(AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    vector<string> columns = { AppUriPermissionColumn::ID };
    auto resultSet = rdbStore->Query(predicatesUnSubscribe, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "Can not query URIPERMISSION");

    int32_t count = 0;
    auto ret = resultSet->GetRowCount(count);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_ERR, "GetRowCount failed ret:%{public}d", ret);
    return count;
}

static void TryUnSubscribeAppState(const std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    int32_t countPermission = CountTemporaryPermission(rdbStore);
    int32_t countSensitive = CountHideSensitive(rdbStore);
    bool cond = (countPermission < 0 || countSensitive < 0);
    CHECK_AND_PRINT_LOG(!cond, "TryUnSubscribeAppState System exception");

    if (countPermission == 0 && countSensitive == 0) {
        MedialibraryAppStateObserverManager::GetInstance().UnSubscribeAppState();
        MEDIA_INFO_LOG("No temporary permission record remains ,UnSubscribeAppState");
    }
}

static int32_t DeleteTemporaryPermission(const std::shared_ptr<MediaLibraryRdbStore> rdbStore, const uint32_t tokenId)
{
    NativeRdb::AbsRdbPredicates predicates(AppUriPermissionColumn::APP_URI_PERMISSION_TABLE);
    predicates.EqualTo(AppUriPermissionColumn::TARGET_TOKENID, static_cast<int64_t>(tokenId));
    vector<string> permissionTypes;
    permissionTypes.emplace_back(to_string(
        static_cast<int32_t>(PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO)));
    permissionTypes.emplace_back(to_string(
        static_cast<int32_t>(PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO)));
    permissionTypes.emplace_back(to_string(
        static_cast<int32_t>(PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO)));
    predicates.And()->In(AppUriPermissionColumn::PERMISSION_TYPE, permissionTypes);
    int32_t deletedRows = -1;

    auto ret = rdbStore->Delete(deletedRows, predicates);
    bool cond = (ret != NativeRdb::E_OK || deletedRows < 0);
    CHECK_AND_PRINT_LOG(!cond, "Story Delete db failed, errCode = %{public}d", ret);
    MEDIA_INFO_LOG("Uripermission Delete retVal: %{public}d, deletedRows: %{public}d", ret, deletedRows);

    return deletedRows;
}

static int32_t DeleteHideSensitive(const std::shared_ptr<MediaLibraryRdbStore> rdbStore, const uint32_t tokenId)
{
    NativeRdb::AbsRdbPredicates predicates(AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    predicates.EqualTo(AppUriPermissionColumn::TARGET_TOKENID, static_cast<int64_t>(tokenId));
    int32_t deletedRows = -1;
    auto ret = rdbStore->Delete(deletedRows, predicates);
    bool cond = (ret != NativeRdb::E_OK || deletedRows < 0);
    CHECK_AND_PRINT_LOG(!cond, "Story Delete db failed, errCode = %{public}d", ret);
    MEDIA_INFO_LOG("Uripermission Delete retVal: %{public}d, deletedRows: %{public}d", ret, deletedRows);

    return deletedRows;
}

void MedialibraryAppStateObserver::WaitAndRevoke(int64_t tokenId)
{
    MedialibraryAppStateObserverManager::GetInstance().AddTokenId(tokenId, true);
    std::this_thread::sleep_for(chrono::milliseconds(WAITFOR_REVOKE));
    if (!MedialibraryAppStateObserverManager::GetInstance().NeedRevoke(tokenId)) {
        MEDIA_INFO_LOG("MedialibraryAppStateObserver stop revoke tokenId:%{public}ld", static_cast<long>(tokenId));
        return;
    }
    MEDIA_INFO_LOG("MedialibraryAppStateObserver OnAppStopped, tokenId:%{public}ld", static_cast<long>(tokenId));
    auto rdbStore = MediaLibraryDataManager::GetInstance()->rdbStore_;
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "Uripermission Delete failed, rdbStore is null.");

    int32_t deletedRowsPermission = DeleteTemporaryPermission(rdbStore, tokenId);
    int32_t deletedRowsSensitive = DeleteHideSensitive(rdbStore, tokenId);
    MedialibraryAppStateObserverManager::GetInstance().RemoveTokenId(tokenId);
    TryUnSubscribeAppState(rdbStore);
}

void MedialibraryAppStateObserver::OnAppStopped(const AppStateData &appStateData)
{
    auto tokenId = appStateData.accessTokenId;
    if (!MedialibraryAppStateObserverManager::GetInstance().IsContainTokenId(tokenId)) {
        return;
    }
    MEDIA_INFO_LOG("MedialibraryAppStateObserver TokenId: %{public}ld OnAppStopped, revoke permission",
        static_cast<long>(tokenId));
    std::thread revokeThread([this, tokenId]() { this->WaitAndRevoke(tokenId); });
    revokeThread.detach();
}

void MedialibraryAppStateObserver::OnAppStarted(const AppStateData &appStateData)
{
    auto tokenId = appStateData.accessTokenId;
    if (MedialibraryAppStateObserverManager::GetInstance().IsContainTokenId(tokenId)) {
        MEDIA_INFO_LOG("MedialibraryAppStateObserver OnAppStarted tokenId: %{public}ld reStart, cancel revoke",
            static_cast<long>(tokenId));
        MedialibraryAppStateObserverManager::GetInstance().AddTokenId(tokenId, false);
    }
}
}  // namespace Media
}  // namespace OHOS

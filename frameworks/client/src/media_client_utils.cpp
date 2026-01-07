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

#include "media_client_utils.h"

#include "iservice_registry.h"
#include "os_account_manager.h"
#include "system_ability_definition.h"
#include "medialibrary_napi_log.h"

namespace OHOS::Media::IPC {
constexpr int32_t DEFUALT_USER_ID = 100;

MediaClientUtils::MediaClientUtils() {}
MediaClientUtils::~MediaClientUtils() {}
// LCOV_EXCL_START
sptr<IRemoteObject> MediaClientUtils::GetSaToken()
{
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saManager == nullptr) {
        NAPI_ERR_LOG("get system ability mgr failed.");
        return nullptr;
    }
    auto remoteObj = saManager->GetSystemAbility(STORAGE_MANAGER_MANAGER_ID);
    if (remoteObj == nullptr) {
        NAPI_ERR_LOG("GetSystemAbility Service failed.");
        return nullptr;
    }
    return remoteObj;
}

int32_t MediaClientUtils::GetCurrentAccountId()
{
    int32_t activeUserId = DEFUALT_USER_ID;
    ErrCode ret = OHOS::AccountSA::OsAccountManager::GetForegroundOsAccountLocalId(activeUserId);
    if (ret != ERR_OK) {
        NAPI_ERR_LOG("fail to get activeUser:%{public}d", ret);
    }
    return activeUserId;
}
// LCOV_EXCL_STOP
}
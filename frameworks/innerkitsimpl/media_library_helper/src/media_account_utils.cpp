/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MediaAccountUtils"

#include "media_account_utils.h"

#include "iservice_registry.h"
#include "media_log.h"
#include "os_account_manager.h"

namespace OHOS {
namespace Media {

static const int STORAGE_MANAGER_MANAGER_ID = 5003;

int32_t MediaAccountUtils::GetCurrentAccountId()
{
    int32_t activeUserId = 100;
    ErrCode ret = OHOS::AccountSA::OsAccountManager::GetForegroundOsAccountLocalId(activeUserId);
    if (ret != ERR_OK) {
        MEDIA_ERR_LOG("GetCurrentAccountId: fail to get activeUser:%{public}d", ret);
    }
    return activeUserId;
}

sptr<IRemoteObject> MediaAccountUtils::GetSaToken()
{
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saManager == nullptr) {
        MEDIA_ERR_LOG("Get system ability mgr failed.");
        return nullptr;
    }
    auto remoteObj = saManager->GetSystemAbility(STORAGE_MANAGER_MANAGER_ID);
    if (remoteObj == nullptr) {
        MEDIA_ERR_LOG("GetSystemAbility Service Failed.");
        return nullptr;
    }
    return remoteObj;
}
} // namespace Media
} // namespace OHOS

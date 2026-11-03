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

#define MLOG_TAG "CustomRestore"

#include "media_library_custom_restore.h"

#include "datashare_helper.h"
#include "iservice_registry.h"
#include "media_log.h"
#include "medialibrary_custom_restore_observer_manager.h"
#include "medialibrary_errno.h"
#include "system_ability_definition.h"
#include "userfilemgr_uri.h"
#include "user_inner_ipc_client.h"
#include "medialibrary_business_code.h"
#include "restore_vo.h"
#include "stop_restore_vo.h"

namespace OHOS {
namespace Media {
using ChangeInfo = DataShare::DataShareObserver::ChangeInfo;
const std::string CustomRestore::NOTIFY_URI_PREFIX = "file://media/custom_restore/";
const std::string ALBUM_PATH_PREFIX = "/Pictures/";

CustomRestore::CustomRestore(string keyPath, bool isDeduplication)
{
    albumLpath_ = ALBUM_PATH_PREFIX + keyPath;
    keyPath_ = keyPath;
    isDeduplication_ = isDeduplication;
}

CustomRestore::CustomRestore(string albumLpath, string keyPath, bool isDeduplication)
{
    albumLpath_ = albumLpath;
    keyPath_ = keyPath;
    isDeduplication_ = isDeduplication;
}

void CustomRestore::Init(string bundleName, string appName, string appId, int32_t tokenId)
{
    MEDIA_DEBUG_LOG("CustomRestore init");
    bundleName_ = bundleName;
    appName_ = appName;
    appId_ = appId;
    tokenId_ = tokenId;
    InitDataShareHelper();
}

void CustomRestore::InitDataShareHelper()
{
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saManager == nullptr) {
        MEDIA_ERR_LOG("get system ability mgr failed.");
        return;
    }
    auto remoteObj = saManager->GetSystemAbility(STORAGE_MANAGER_MANAGER_ID);
    if (remoteObj == nullptr) {
        MEDIA_ERR_LOG("GetSystemAbility Service failed.");
        return;
    }
    if (sDataShareHelper_ == nullptr && remoteObj != nullptr) {
        sDataShareHelper_ = DataShare::DataShareHelper::Creator(remoteObj, MEDIALIBRARY_DATA_URI);
    }
}

int32_t CustomRestore::Restore(std::string dbPath)
{
    MEDIA_DEBUG_LOG("CustomRestore Restore");
    if (sDataShareHelper_ == nullptr) {
        MEDIA_ERR_LOG("sDataShareHelper_ is null.");
        return E_DATASHARE_IS_NULL;
    }
    CHECK_AND_RETURN_RET_LOG(!albumLpath_.empty(), E_INVALID_VALUES, "albumLpath is empty.");
    CHECK_AND_RETURN_RET_LOG(!keyPath_.empty(), E_INVALID_VALUES, "keyPath is empty.");
    CHECK_AND_RETURN_RET_LOG(!bundleName_.empty(), E_INVALID_VALUES, "bundleName is empty.");
    CHECK_AND_RETURN_RET_LOG(!appName_.empty(), E_INVALID_VALUES, "appName is empty.");

    RestoreReqBody reqBody;
    reqBody.dbPath = dbPath;
    reqBody.albumLpath = albumLpath_;
    reqBody.keyPath = keyPath_;
    reqBody.bundleName = bundleName_;
    reqBody.appName = appName_;
    reqBody.appId = appId_;
    reqBody.isDeduplication = isDeduplication_;
    int32_t result = IPC::UserInnerIPCClient()
                         .SetDataShareHelper(sDataShareHelper_)
                         .Call(static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CUSTOM_RESTORE), reqBody);
    MEDIA_DEBUG_LOG("CustomRestore Restore end. %{public}d", result);
    return result;
}

int32_t CustomRestore::StopRestore()
{
    MEDIA_DEBUG_LOG("CustomRestore StopRestore");
    CHECK_AND_RETURN_RET_LOG(sDataShareHelper_ != nullptr, E_DATASHARE_IS_NULL, "sDataShareHelper_ is null.");

    StopRestoreReqBody reqBody;
    reqBody.keyPath = keyPath_;
    int32_t result = IPC::UserInnerIPCClient()
                         .SetDataShareHelper(sDataShareHelper_)
                         .Call(static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CUSTOM_RESTORE_CANCEL), reqBody);
    MEDIA_DEBUG_LOG("CustomRestore StopRestore end. %{public}d", result);
    return result;
}

int32_t CustomRestore::RegisterCustomRestoreCallback(std::shared_ptr<CustomRestoreCallback> callback)
{
    MEDIA_DEBUG_LOG("CustomRestore RegisterCallback");
    if (callback == nullptr) {
        MEDIA_ERR_LOG("CustomRestore RegisterCallback callback is null.");
        return E_CALLBACK_IS_NULL;
    }
    if (sDataShareHelper_ == nullptr) {
        MEDIA_ERR_LOG("CustomRestore RegisterCallback sDataShareHelper_ is null.");
        return E_DATASHARE_IS_NULL;
    }
    std::shared_ptr<CustomRestoreNotifyObserver> notifyObserver =
            std::make_shared<CustomRestoreNotifyObserver>(callback);
    CHECK_AND_RETURN_RET_LOG(notifyObserver != nullptr, E_OBSERVER_IS_NULL,
        "CustomRestore RegisterCallback notifyObserver is null.");

    Uri customRestoreUri(NOTIFY_URI_PREFIX + keyPath_);
    sDataShareHelper_->RegisterObserverExt(customRestoreUri, notifyObserver, true);
    CustomRestoreObserverManager::GetInstance().AttachObserver(callback, notifyObserver);
    return E_OK;
}

int32_t CustomRestore::UnregisterCustomRestoreCallback(std::shared_ptr<CustomRestoreCallback> callback)
{
    MEDIA_DEBUG_LOG("CustomRestore UnRegisterCallback");
    if (callback == nullptr) {
        MEDIA_ERR_LOG("CustomRestore UnRegisterCallback callback is null.");
        return E_CALLBACK_IS_NULL;
    }

    CHECK_AND_RETURN_RET_LOG(sDataShareHelper_ != nullptr, E_DATASHARE_IS_NULL,
        "CustomRestore UnregisterCallback sDataShareHelper_ is null.");
    std::shared_ptr<CustomRestoreNotifyObserver> notifyObserver =
        CustomRestoreObserverManager::GetInstance().QueryObserver(callback);
    CHECK_AND_RETURN_RET_LOG(notifyObserver != nullptr, E_OBSERVER_IS_NULL,
        "CustomRestore UnRegisterCallback notifyObserver is null.");

    Uri customRestoreUri(NOTIFY_URI_PREFIX + keyPath_);
    sDataShareHelper_->UnregisterObserverExt(customRestoreUri, notifyObserver);
    CustomRestoreObserverManager::GetInstance().DetachObserver(callback);
    return E_OK;
}

}  // namespace Media
}  // namespace OHOS
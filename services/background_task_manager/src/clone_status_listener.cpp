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

#include "clone_status_listener.h"
#include <json/json.h>
#include <memory>
#include <string>
#include <parameter.h>
#include "media_log.h"
#include "media_file_utils.h"
#include "parameters.h"
#include "iremote_object.h"
#include "iservice_registry.h"

#include "cloud_sync_helper.h"
#include "medialibrary_async_worker.h"

using namespace std;

namespace OHOS::Media {
namespace {
constexpr int32_t BACKUP_SA_ID = 5203;
constexpr int32_t MIN_TIME_OUT = 4;
const std::string CLONE_STATE = "persist.dataclone.state";
const std::string CLONE_FLAG = "multimedia.medialibrary.cloneFlag";
const std::string NOT_IN_CLONE = "0";
const std::string RESTORE_FLAG = "multimedia.medialibrary.restoreFlag";
const std::string UPGRADE_FLAG = "persist.update.hmos_to_next_flag";
}
CloneStatusListener::CloneStatusListener()
{
    MEDIA_INFO_LOG("CloneStatusListener constructor");
}

CloneStatusListener::~CloneStatusListener()
{
    MEDIA_INFO_LOG("CloneStatusListener destructor");
}

static void Register(AsyncTaskData *data)
{
    CloneStatusListener::GetInstance()->RegisterCloneStatusChangeListener();
}

bool CloneStatusListener::InitAsync()
{
    shared_ptr<MediaLibraryAsyncWorker> asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    CHECK_AND_RETURN_RET_LOG(asyncWorker != nullptr, false, "can not get async worker");
    shared_ptr<MediaLibraryAsyncTask> asyncTask = make_shared<MediaLibraryAsyncTask>(
        Register, nullptr);
    CHECK_AND_RETURN_RET_LOG(asyncTask != nullptr, false, "Register create task fail");
    MEDIA_INFO_LOG("Register add task success");
    asyncWorker->AddTask(asyncTask, false);
    return true;
}

void CloneStatusListener::RegisterCloneStatusChangeListener()
{
    std::unique_lock<std::mutex> lock(registerMutex_);
    CHECK_AND_RETURN(!isCloneStatusChangedListenerRegistered_);
    MEDIA_INFO_LOG("CloneStatusListener RegisterCloneStatusChangeListener");
    int32_t ret = WatchParameter(CLONE_STATE.data(), OnParameterChange, nullptr);
    if (ret != 0) {
        MEDIA_ERR_LOG("CloneStatusListener RegisterParameterListener fail, ret: %{public}d", ret);
        return;
    }
    SetDeathRecipient();
    isCloneStatusChangedListenerRegistered_ = true;
}

void CloneStatusListener::UnRegisterCloneStatusChangeListener()
{
    std::unique_lock<std::mutex> lock(registerMutex_);
    CHECK_AND_RETURN(isCloneStatusChangedListenerRegistered_);
    MEDIA_INFO_LOG("CloneStatusListener UnRegisterCloneStatusChangeListener");
    int32_t ret = RemoveParameterWatcher(CLONE_STATE.data(), OnParameterChange, nullptr);
    if (ret != 0) {
        MEDIA_ERR_LOG("CloneStatusListener UnRegisterParameterListener fail, ret: %{public}d", ret);
        return;
    }
    isCloneStatusChangedListenerRegistered_ = false;
}

void CloneStatusListener::OnParameterChange(const char *key, const char *value, void *context)
{
    MEDIA_INFO_LOG("CloneStatusListener OnParameterChange, key: %{public}s, value: %{public}s", key, value);
    if (std::string(key) == CLONE_STATE.data()) {
        CloneStatusListener::GetInstance()->HandleCloneStatusChanged();
    }
}

void CloneStatusListener::HandleCloneStatusChanged()
{
    if (system::GetParameter(CLONE_STATE, "") != NOT_IN_CLONE) {
        MEDIA_INFO_LOG("CloneStatusListener HandleCloneStatusChanged in clone");
        auto currentTime = std::to_string(MediaFileUtils::UTCTimeSeconds());
        MEDIA_INFO_LOG("CloneStatusListener SetParameterForClone currentTime:%{public}s", currentTime.c_str());
        bool retFlag = system::SetParameter(CLONE_FLAG, currentTime);
        CHECK_AND_PRINT_LOG(retFlag, "Failed to set parameter cloneFlag, retFlag:%{public}d", retFlag);
#ifdef CLOUD_SYNC_MANAGER
        FileManagement::CloudSync::CloudSyncManager::GetInstance().StopSync("com.ohos.medialibrary.medialibrarydata");
#endif
        return;
    }
    MEDIA_INFO_LOG("CloneStatusListener HandleCloneStatusChanged not in clone");
    if (system::GetParameter(RESTORE_FLAG, "0") == "0" && system::GetParameter(UPGRADE_FLAG, "") != "1") {
        bool retFlag = system::SetParameter(CLONE_FLAG, "0");
        CHECK_AND_PRINT_LOG(retFlag, "Failed to set parameter cloneFlag, retFlag:%{public}d", retFlag);
    }
    std::string cloneFlagStr = system::GetParameter(CLONE_FLAG, "0");
    CHECK_AND_RETURN_LOG(cloneFlagStr == "0", "Failed to set parameter cloneFlag, retFlag:%{public}s",
        cloneFlagStr.c_str());
    CloudSyncHelper::GetInstance()->StartSync();
}

void CloneStatusListener::HandleDeathRecipient()
{
    {
        std::lock_guard<std::mutex> lock(deathRecipientMutex_);
        backupSaRemoteObject_ = nullptr;
    }
    MEDIA_INFO_LOG("CloneStatusListener : End handle death recipient");
}

void CloneStatusListener::SetDeathRecipient()
{
    std::lock_guard<std::mutex> lock(deathRecipientMutex_);
    CHECK_AND_RETURN_LOG(backupSaRemoteObject_ == nullptr, "CloneStatusListener: No need to set death recipient");
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHECK_AND_RETURN_LOG(saManager != nullptr, "failed to get SystemAbilityManagerClient");
 
    backupSaRemoteObject_ = saManager->CheckSystemAbility(BACKUP_SA_ID);
    if (backupSaRemoteObject_ == nullptr) {
        MEDIA_INFO_LOG("CloneStatusListener: Try to load Backup SystemAbility");
        backupSaRemoteObject_ = saManager->LoadSystemAbility(BACKUP_SA_ID, MIN_TIME_OUT);
        CHECK_AND_RETURN_LOG(backupSaRemoteObject_ != nullptr, "CloneStatusListener: backupSaRemoteObject_ is null.");
    }
    
    CHECK_AND_PRINT_LOG(backupSaRemoteObject_->AddDeathRecipient(sptr(new CloneStatusListenerDeathRecipient())),
        "CloneStatusListener: Failed to add death recipient.");
}

void CloneStatusListenerDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    MEDIA_INFO_LOG("CloneStatusListener : OnRemoteDied");
    CHECK_AND_RETURN_LOG(object != nullptr, "CloneStatusListener remote object is nullptr");
    sptr<IRemoteObject> objectPtr = object.promote();
    CHECK_AND_RETURN_LOG(objectPtr != nullptr, "CloneStatusListener remote object sptr is nullptr");
    objectPtr->RemoveDeathRecipient(this);
    CloneStatusListener::GetInstance()->HandleDeathRecipient();
}
}  // namespace OHOS::UpdateEngine

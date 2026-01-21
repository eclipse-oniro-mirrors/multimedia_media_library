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
#define MLOG_TAG "CustomRestoreObserverManager"

#include "medialibrary_custom_restore_observer_manager.h"

#include "dataobs_mgr_client.h"
#include "media_log.h"

namespace OHOS::Media {
void CustomRestoreNotifyObserver::OnChange(const ChangeInfo &changeInfo)
{
    MEDIA_DEBUG_LOG("CustomRestoreCallback OnChange");
    CHECK_AND_RETURN_LOG(customRestoreCallback_ != nullptr, "CustomRestoreCallback is nullptr");
    CHECK_AND_RETURN_LOG(!changeInfo.valueBuckets_.empty(), "changeInfo.valueBuckets_ is empty");

    ChangeInfo::VBucket vBucket = changeInfo.valueBuckets_[0];
    CHECK_AND_RETURN_LOG(!vBucket.empty(), "vBucket is empty");

    RestoreResult restoreResult;
    restoreResult.stage = std::get<std::string>(vBucket["stage"]);
    restoreResult.errCode = static_cast<int32_t>(std::get<int64_t>(vBucket["errCode"]));
    restoreResult.progress = static_cast<int32_t>(std::get<int64_t>(vBucket["progress"]));
    restoreResult.uriType = static_cast<int32_t>(std::get<int64_t>(vBucket["uriType"]));
    restoreResult.uri = std::get<std::string>(vBucket["uri"]);
    RestoreInfo restoreInfo;
    restoreInfo.totalNum = static_cast<int32_t>(std::get<int64_t>(vBucket["totalNum"]));
    restoreInfo.successNum = static_cast<int32_t>(std::get<int64_t>(vBucket["successNum"]));
    restoreInfo.failedNum = static_cast<int32_t>(std::get<int64_t>(vBucket["failedNum"]));
    restoreInfo.sameNum = static_cast<int32_t>(std::get<int64_t>(vBucket["sameNum"]));
    restoreInfo.cancelNum = static_cast<int32_t>(std::get<int64_t>(vBucket["cancelNum"]));
    restoreResult.restoreInfo = restoreInfo;
    MEDIA_DEBUG_LOG("CustomRestoreCallback OnRestoreResult");
    customRestoreCallback_->OnRestoreResult(restoreResult);
}

CustomRestoreObserverManager &CustomRestoreObserverManager::GetInstance()
{
    static CustomRestoreObserverManager instance;
    return instance;
}

std::shared_ptr<CustomRestoreNotifyObserver> CustomRestoreObserverManager::QueryObserver(
    std::shared_ptr<CustomRestoreCallback> callback)
{
    MEDIA_DEBUG_LOG("QueryObserver callback");
    if (callback == nullptr) {
        MEDIA_ERR_LOG("QueryObserver callback is nullptr");
        return nullptr;
    }
    std::shared_ptr<CustomRestoreNotifyObserver> result;
    if (!callbackMap_.Find(callback, result)) {
        MEDIA_ERR_LOG("QueryObserver not find");
        return nullptr;
    }
    return result;
}

bool CustomRestoreObserverManager::AttachObserver(std::shared_ptr<CustomRestoreCallback> callback,
    std::shared_ptr<CustomRestoreNotifyObserver> notifyObserver)
{
    MEDIA_DEBUG_LOG("AttachObserver callback");
    if (callback == nullptr) {
        MEDIA_ERR_LOG("AttachObserver callback is nullptr");
        return false;
    }
    if (notifyObserver == nullptr) {
        MEDIA_ERR_LOG("AttachObserver notifyObserver is nullptr");
        return false;
    }
    callbackMap_.Insert(callback, notifyObserver);
    return true;
}

bool CustomRestoreObserverManager::DetachObserver(std::shared_ptr<CustomRestoreCallback> callback)
{
    MEDIA_DEBUG_LOG("DetachObserver callback");
    if (callback == nullptr) {
        MEDIA_ERR_LOG("DetachObserver callback is nullptr");
        return false;
    }
    callbackMap_.Erase(callback);
    return true;
}

} // namespace OHOS::Media

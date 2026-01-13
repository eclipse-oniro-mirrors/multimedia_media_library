/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "media_observer_manager.h"
#include "notify_register_permission.h"
#include "medialibrary_errno.h"
#include "media_log.h"
#include "media_notification_utils.h"
#include "ipc_skeleton.h"

#include <cctype>
#include <algorithm>

namespace OHOS::Media {
using namespace Notification;
MediaObserverManager::MediaObserverManager() {}
MediaObserverManager::~MediaObserverManager() {}

std::shared_ptr<Media::Notification::MediaObserverManager> MediaObserverManager::observerManager_ = nullptr;
std::mutex MediaObserverManager::instanceMutex_;
constexpr size_t MAX_SINGLE_ASSET = 1000 - 1;
constexpr size_t MAX_SINGLE_ALBUM = 250 - 1;
constexpr size_t MAX_SINGLE_ASSET_PROCESS = 50;
constexpr size_t MAX_SINGLE_ALBUM_PROCESS = 50;
static const int32_t MEDIA_LIBRARY_PARAM_INVALID = 23800151;

std::shared_ptr<Notification::MediaObserverManager> MediaObserverManager::GetObserverManager()
{
    if (observerManager_ == nullptr) {
        std::lock_guard<std::mutex> guard(instanceMutex_);
        if (observerManager_ != nullptr) {
            return observerManager_;
        }
        auto *mediaObserverManager = new (std::nothrow)Media::Notification::MediaObserverManager();
        if (mediaObserverManager == nullptr) {
            MEDIA_ERR_LOG("Failed to new MediaObserverManager");
            return nullptr;
        }
        observerManager_ = std::shared_ptr<Media::Notification::MediaObserverManager>(mediaObserverManager);
    }
    return observerManager_;
}

int32_t MediaObserverManager::AddObserver(const NotifyUriType &uri,
    const sptr<AAFwk::IDataAbilityObserver> &dataObserver, bool isReconnect)
{
    MEDIA_INFO_LOG("AddObserver");
    if (dataObserver == nullptr || dataObserver->AsObject() == nullptr) {
        MEDIA_ERR_LOG("register dataAbilityObserver is null");
        return E_DATAOBSERVER_IS_NULL;
    }
    NotifyRegisterPermission permissionHandle;
    int32_t ret = permissionHandle.ExecuteCheckPermission(uri);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Permission verification failed");
        return ret;
    }
    ObserverInfo obsInfo;
    obsInfo.observer = dataObserver;
    obsInfo.isSystem = permissionHandle.isSystemApp();
    obsInfo.callingTokenId = IPCSkeleton::GetCallingTokenID();
    auto retVal = CheckSingleProcessSize(uri);
    CHECK_AND_RETURN_RET_LOG(retVal, MEDIA_LIBRARY_PARAM_INVALID, "Exceeds single register specification limit");
    std::lock_guard<std::mutex> lock(mutex_);
    if (observers_.find(uri) == observers_.end()) {
        observers_[uri].push_back(obsInfo);
    } else {
        auto it = std::find_if(observers_[uri].begin(), observers_[uri].end(),
        [dataObserver](const ObserverInfo& obsInfo) {
            return obsInfo.observer->AsObject() == dataObserver->AsObject();
        });
        if (it == observers_[uri].end()) {
            observers_[uri].push_back(obsInfo);
        } else {
            MEDIA_INFO_LOG("the uri has already been registered with the same observer");
            return E_DATAOBSERVER_IS_REPEATED;
        }
    }
    // 绑定死亡回调函数
    sptr<ObserverCallbackRecipient> observerCallbackRecipient = new ObserverCallbackRecipient();
    if (observerCallbackRecipient == nullptr) {
        MEDIA_ERR_LOG("fail to create ObserverCallbackRecipient");
        return E_OBSERVER_CALLBACK_RECIPIENT_ERROR;
    }
    if (!dataObserver->AsObject()->AddDeathRecipient(observerCallbackRecipient)) {
        MEDIA_ERR_LOG("fail to add ObserverCallbackRecipient");
        return E_OBSERVER_CALLBACK_RECIPIENT_ERROR;
    }
    obsCallbackPecipients_.emplace(dataObserver->AsObject(), observerCallbackRecipient);
    if (isReconnect) {
        ExeForReconnect(uri, dataObserver);
    }
    return E_OK;
}

int32_t MediaObserverManager::RemoveObserver(const wptr<IRemoteObject> &object)
{
    MEDIA_INFO_LOG("enter removeObserver");
    if (object.promote() == nullptr) {
        MEDIA_ERR_LOG("remoteObject is null");
        return E_DATAOBSERVER_IS_NULL;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<NotifyUriType> deleteUris;
    for (auto it = observers_.begin(); it != observers_.end(); ++it) {
        auto index = std::find_if(it->second.begin(), it->second.end(),
        [object](const ObserverInfo& s) {
            return s.observer->AsObject() == object;
        });
        if (index != it->second.end()) {
            it->second.erase(index);
            MEDIA_INFO_LOG("RemoveObserver, uri is %{public}d", static_cast<int>(it->first));
        }
        if (it->second.empty() && observers_.find(it->first) != observers_.end()) {
            deleteUris.push_back(it->first);
        }
    }
    for (const auto &it: deleteUris) {
        observers_.erase(it);
    }
    int32_t ret = RemoveObsDeathRecipient(object);
    if (ret != E_OK) {
        MEDIA_WARN_LOG("failed to remove obsDeathRecipient");
    }
    return E_OK;
}

int32_t MediaObserverManager::RemoveObsDeathRecipient(const wptr<IRemoteObject> &object)
{
    MEDIA_INFO_LOG("enter MediaObserverManager::RemoveObsDeathRecipient");
    sptr<IRemoteObject> objectPtr = object.promote();
    if (objectPtr == nullptr || obsCallbackPecipients_.empty()) {
        MEDIA_ERR_LOG("remoteObject is nullptr");
        return E_DATAOBSERVER_IS_NULL;
    }
    auto iter = obsCallbackPecipients_.find(objectPtr);
    if (iter != obsCallbackPecipients_.end()) {
        objectPtr->RemoveDeathRecipient(obsCallbackPecipients_[objectPtr]);
        obsCallbackPecipients_.erase(iter);
    }
    return E_OK;
}

std::vector<ObserverInfo> MediaObserverManager::FindObserver(const NotifyUriType &uri)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = observers_.find(uri);
    if (iter == observers_.end()) {
        MEDIA_ERR_LOG("failed to find observer, uri is not exist");
        return {};
    }
    return iter->second;
}

int32_t MediaObserverManager::RemoveObserverWithUri(const NotifyUriType &uri,
    const sptr<AAFwk::IDataAbilityObserver> &dataObserver)
{
    MEDIA_INFO_LOG("enter removeObserverWithUri");
    NotifyRegisterPermission permissionHandle;
    int32_t ret = permissionHandle.ExecuteCheckPermission(uri);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Permission verification failed");
        return ret;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    auto observersIter = observers_.find(uri);
    if (observersIter == observers_.end()) {
        MEDIA_ERR_LOG("unRegister uri is not exist");
        return E_URI_NOT_EXIST;
    }
    auto iter = std::remove_if(observersIter->second.begin(), observersIter->second.end(),
        [dataObserver](const ObserverInfo& s) {
        return s.observer->AsObject() == dataObserver->AsObject();
    });
    observersIter->second.erase(iter, observersIter->second.end());
    if (observersIter->second.empty()) {
        observers_.erase(observersIter);
    }
    ret = RemoveObsDeathRecipient(dataObserver->AsObject());
    if (ret != E_OK) {
        MEDIA_WARN_LOG("failed to remove obsDeathRecipient");
    }
    return E_OK;
}

bool MediaObserverManager::FindSingleObserverWithUri(const NotifyUriType &uri,
    const uint32_t callingTokenId)
{
    MEDIA_INFO_LOG("enter FindSingleObserverWithUri");
    NotifyRegisterPermission permissionHandle;
    int32_t ret = permissionHandle.ExecuteCheckPermission(uri);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Permission verification failed");
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    auto observersIter = observers_.find(uri);
    if (observersIter == observers_.end()) {
        MEDIA_ERR_LOG("uri is not exist");
        return false;
    }
    auto iter = std::find_if(observersIter->second.begin(), observersIter->second.end(),
        [callingTokenId](const ObserverInfo& s) {
        return s.callingTokenId == callingTokenId;
    });
    return iter != observersIter->second.end();
}

std::unordered_map<NotifyUriType, std::vector<ObserverInfo>> MediaObserverManager::GetObservers()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return this->observers_;
}

void MediaObserverManager::ExeForReconnect(const NotifyUriType &registerUri,
    const sptr<AAFwk::IDataAbilityObserver> &dataObserver)
{
    shared_ptr<MediaChangeInfo> recheckChangeInfo = make_shared<MediaChangeInfo>();
    recheckChangeInfo->notifyUri = registerUri;
    recheckChangeInfo->isForRecheck = true;
    if (registerUri == PHOTO_URI || registerUri == HIDDEN_PHOTO_URI || registerUri == TRASH_PHOTO_URI) {
        recheckChangeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ASSET_ADD;
    } else {
        recheckChangeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ALBUM_ADD;
    }
    NotificationUtils::SendNotification(dataObserver, recheckChangeInfo);
    MEDIA_WARN_LOG("reconnect server and send recheck for uriType[%{public}d]", registerUri);
}

int32_t MediaObserverManager::ProcessSingleObserverSingleIds(const NotifyUriType &registerUri,
    const sptr<AAFwk::IDataAbilityObserver> &dataObserver, const std::string &singleId, const UriOperation &operation)
{
    NotifyRegisterPermission permissionHandle;
    if (!all_of(singleId.begin(), singleId.end(), ::isdigit)) {
        MEDIA_ERR_LOG("The singleId format is invalid");
        return E_URI_IS_INVALID;
    }
    int32_t ret = permissionHandle.ExecuteCheckPermission(registerUri);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_PERMISSION_DENIED, "Permission verification failed");
    ret = permissionHandle.SinglePermissionCheck(registerUri, singleId);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_PERMISSION_DENIED, "Permission verification failed");
    auto retVal = CheckSingleListenSize(registerUri);
    CHECK_AND_RETURN_RET_LOG(retVal, MEDIA_LIBRARY_PARAM_INVALID, "Exceeds single register specification limit");
    auto uriIter = observers_.find(registerUri);
    if (uriIter == observers_.end()) {
        MEDIA_ERR_LOG("the registerUri not registered");
        return E_URI_NOT_EXIST;
    }
    auto& observerList = uriIter->second;
    auto obsIter = std::find_if(observerList.begin(), observerList.end(),
        [&dataObserver](const ObserverInfo& obsInfo) {
            return obsInfo.observer->AsObject() == dataObserver->AsObject();
        });
    if (obsIter == observerList.end()) {
        MEDIA_ERR_LOG("registerUri has not been registered by the same observer");
        return E_DATAOBSERVER_IS_NULL;
    }
    operation(obsIter->singleIds, singleId);
    return E_OK;
}

int32_t MediaObserverManager::AddSingleObserverSingleIds(const NotifyUriType &registerUri,
    const sptr<AAFwk::IDataAbilityObserver> &dataObserver, const std::string &singleId)
{
    return ProcessSingleObserverSingleIds(registerUri, dataObserver, singleId,
        [this](std::unordered_set<std::string>& singleIds, const std::string& singleId) {
            std::lock_guard<std::mutex> lock(mutex_);
            singleIds.insert(singleId);
        });
}

int32_t MediaObserverManager::RemoveSingleObserverSingleIds(const NotifyUriType &registerUri,
    const sptr<AAFwk::IDataAbilityObserver> &dataObserver, const std::string &singleId)
{
    return ProcessSingleObserverSingleIds(registerUri, dataObserver, singleId,
        [this](std::unordered_set<std::string>& singleIds, const std::string& singleId) {
            std::lock_guard<std::mutex> lock(mutex_);
            singleIds.erase(singleId);
        });
}

bool MediaObserverManager::FindSingleObserver(const NotifyUriType &uri,
    std::vector<ObserverInfo> &obsInfos)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = observers_.find(uri);
    if (iter == observers_.end()) {
        MEDIA_ERR_LOG("failed to find single observer, uri is not exist");
        return false;
    }
    obsInfos = iter->second;
    return true;
}

int32_t MediaObserverManager::RemoveSingleObserverSingleIds(ObserverInfo &singleObserverInfo,
    const std::string &singleId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    singleObserverInfo.singleIds.erase(singleId);
    MEDIA_INFO_LOG("RemoveSingleObserverSingleIds success: uri %{public}s", singleId.c_str());
    return E_OK;
}

bool MediaObserverManager::IsSingleIdDataPresentInSingleObserver(
    const std::unordered_set<std::string> &singleIds, const std::string &singleId)
{
    if (singleIds.count(singleId) > 0) {
        MEDIA_INFO_LOG("IsSingleIdDataPresentInSingleObserver: uri %{public}s exists in observed", singleId.c_str());
        return true;
    }
    MEDIA_INFO_LOG("The target singleObserverInfo does not exist");
    return false;
}

bool MediaObserverManager::CheckSingleListenSize(const NotifyUriType &registerUri)
{
    if (registerUri != NotifyUriType::SINGLE_PHOTO_URI &&
        registerUri != NotifyUriType::SINGLE_PHOTO_ALBUM_URI) {
        return true;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    std::size_t listenSize = 0;
    auto it = observers_.find(registerUri);
    if (it != observers_.end()) {
        const std::vector<ObserverInfo>& observerList = it->second;
        for (const auto& observer : observerList) {
            listenSize += observer.singleIds.size();
        }
    }
    MEDIA_INFO_LOG("CheckSingleListenSize: listenSize is %{public}" PRId64, static_cast<int64_t>(listenSize));
    if (registerUri == NotifyUriType::SINGLE_PHOTO_URI) {
        return listenSize < MAX_SINGLE_ASSET;
    }
    return listenSize < MAX_SINGLE_ALBUM;
}

bool MediaObserverManager::CheckSingleProcessSize(const NotifyUriType &registerUri)
{
    if (registerUri != NotifyUriType::SINGLE_PHOTO_URI &&
        registerUri != NotifyUriType::SINGLE_PHOTO_ALBUM_URI) {
        return true;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    std::size_t processSize = 0;
    auto it = observers_.find(registerUri);
    if (it != observers_.end()) {
        processSize = it->second.size();
    }
    MEDIA_INFO_LOG("CheckSingleRegisterSize: processSize is %{public}" PRId64, static_cast<int64_t>(processSize));
    if (registerUri == NotifyUriType::SINGLE_PHOTO_URI) {
        return processSize < MAX_SINGLE_ASSET_PROCESS;
    }
    return processSize < MAX_SINGLE_ALBUM_PROCESS;
}
} // OHOS::Media

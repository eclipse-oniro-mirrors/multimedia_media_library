/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaLibraryManagerNotifyObserverManager"

#include "medialibrary_manager_notify_observer_manager.h"

#include <algorithm>

#include "check_single_photo_permission_vo.h"
#include "media_log.h"
#include "medialibrary_business_code.h"
#include "medialibrary_errno.h"
#include "medialibrary_manager_notify_observer.h"
#include "medialibrary_manager_notify_utils.h"
#include "user_inner_ipc_client.h"

namespace OHOS {
namespace Media {
namespace {
const std::string URI_SEPARATOR = "file:media";

template<typename CallbackType>
bool HasCallback(const std::vector<std::shared_ptr<CallbackType>> &callbacks,
    const std::shared_ptr<CallbackType> &callback)
{
    return std::find(callbacks.begin(), callbacks.end(), callback) != callbacks.end();
}

template<typename CallbackType>
bool g_removeCallback(std::vector<std::shared_ptr<CallbackType>> &callbacks,
    const std::shared_ptr<CallbackType> &callback)
{
    auto iter = std::find(callbacks.begin(), callbacks.end(), callback);
    CHECK_AND_RETURN_RET(iter != callbacks.end(), false);
    callbacks.erase(iter);
    return true;
}

std::string GetValidId(int32_t id)
{
    if (id == AccurateRefresh::INVALID_INT32_VALUE) {
        return "";
    }
    return std::to_string(id);
}
}

MediaLibraryManagerNotifyObserverManager &MediaLibraryManagerNotifyObserverManager::GetInstance()
{
    static MediaLibraryManagerNotifyObserverManager instance;
    return instance;
}

bool MediaLibraryManagerNotifyObserverManager::IsRecordEmpty(const NotifyObserverRecord &record) const
{
    return record.assetCallbacks.empty() && record.albumCallbacks.empty() && record.singleAssetCallbacks.empty() &&
        record.singleAlbumCallbacks.empty();
}

int32_t MediaLibraryManagerNotifyObserverManager::RegisterBaseObserverWithLockHeld(
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper, Notification::NotifyUriType uriType,
    const std::string &registerUri, NotifyObserverRecord &record)
{
    if (record.observer != nullptr) {
        return E_OK;
    }
    auto observer = std::make_shared<MediaLibraryManagerNotifyObserver>(uriType);
    int32_t ret = dataShareHelper->RegisterObserverExtProvider(Uri(registerUri), observer, false);
    ret = MediaLibraryManagerNotifyUtils::ConvertProviderError(ret);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "register observer failed, ret: %{public}d", ret);
    record.uriType = uriType;
    record.registerUri = registerUri;
    record.observer = observer;
    return E_OK;
}

int32_t MediaLibraryManagerNotifyObserverManager::UnregisterBaseObserverWithLockHeld(
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper, NotifyObserverRecord &record)
{
    CHECK_AND_RETURN_RET_LOG(record.observer != nullptr, E_INVALID_ARGUMENTS, "observer is nullptr");
    int32_t ret = dataShareHelper->UnregisterObserverExtProvider(Uri(record.registerUri), record.observer);
    ret = MediaLibraryManagerNotifyUtils::ConvertProviderError(ret);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "unregister observer failed, ret: %{public}d", ret);
    record.observer = nullptr;
    return E_OK;
}

int32_t MediaLibraryManagerNotifyObserverManager::RegisterSingleIdWithLockHeld(
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper, const std::string &registerUri,
    const std::string &singleId, const std::shared_ptr<DataShare::DataShareObserver> &observer)
{
    CHECK_AND_RETURN_RET_LOG(observer != nullptr, E_INVALID_ARGUMENTS, "observer is nullptr");
    int32_t ret = dataShareHelper->RegisterObserverExtProvider(Uri(registerUri + URI_SEPARATOR + singleId),
        observer, false);
    ret = MediaLibraryManagerNotifyUtils::ConvertProviderError(ret);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "register single id failed, ret: %{public}d", ret);
    return E_OK;
}

int32_t MediaLibraryManagerNotifyObserverManager::UnregisterSingleIdWithLockHeld(
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper, const std::string &registerUri,
    const std::string &singleId, const std::shared_ptr<DataShare::DataShareObserver> &observer)
{
    CHECK_AND_RETURN_RET_LOG(observer != nullptr, E_INVALID_ARGUMENTS, "observer is nullptr");
    int32_t ret = dataShareHelper->UnregisterObserverExtProvider(Uri(registerUri + URI_SEPARATOR + singleId), observer);
    ret = MediaLibraryManagerNotifyUtils::ConvertProviderError(ret);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "unregister single id failed, ret: %{public}d", ret);
    return E_OK;
}

int32_t MediaLibraryManagerNotifyObserverManager::CheckSinglePhotoPermission(
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper, Notification::NotifyUriType uriType,
    const std::string &singleId)
{
    CHECK_AND_RETURN_RET_LOG(uriType == Notification::NotifyUriType::SINGLE_PHOTO_URI, E_INVALID_ARGUMENTS,
        "uriType is invalid: %{public}d", static_cast<int32_t>(uriType));
    CheckSinglePhotoPermissionReqBody reqBody;
    reqBody.registerType = static_cast<int32_t>(uriType);
    reqBody.fileId = singleId;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHECK_SINGLE_PHOTO_CHANGE_PERMISSION);
    int32_t ret = IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper).Call(businessCode, reqBody);
    return MediaLibraryManagerNotifyUtils::ConvertProviderError(ret);
}

int32_t MediaLibraryManagerNotifyObserverManager::RegisterAssetCallback(
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper, Notification::NotifyUriType uriType,
    const std::shared_ptr<PhotoAssetChangeCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, E_DATASHARE_IS_NULL, "dataShareHelper is nullptr");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, E_INVALID_ARGUMENTS, "callback is nullptr");
    Notification::NotifyUriType registerUriType = Notification::NotifyUriType::INVALID;
    std::string registerUri;
    int32_t ret = MediaLibraryManagerNotifyUtils::GetNotifyTypeAndUri(uriType, registerUriType, registerUri);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);

    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = observerRecords_.find(registerUriType);
    CHECK_AND_RETURN_RET_LOG(!(iter != observerRecords_.end() && HasCallback(iter->second.assetCallbacks, callback)),
        E_INVALID_ARGUMENTS, "callback is already registered");
    auto &record = observerRecords_[registerUriType];
    ret = RegisterBaseObserverWithLockHeld(dataShareHelper, registerUriType, registerUri, record);
    if (ret != E_OK) {
        observerRecords_.erase(registerUriType);
        return ret;
    }
    record.assetCallbacks.push_back(callback);
    return E_OK;
}

int32_t MediaLibraryManagerNotifyObserverManager::UnregisterAssetCallback(
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper, Notification::NotifyUriType uriType,
    const std::shared_ptr<PhotoAssetChangeCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, E_DATASHARE_IS_NULL, "dataShareHelper is nullptr");
    Notification::NotifyUriType registerUriType = Notification::NotifyUriType::INVALID;
    std::string registerUri;
    int32_t ret = MediaLibraryManagerNotifyUtils::GetNotifyTypeAndUri(uriType, registerUriType, registerUri);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);

    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = observerRecords_.find(registerUriType);
    CHECK_AND_RETURN_RET_LOG(iter != observerRecords_.end(), E_INVALID_ARGUMENTS, "observer is not registered");
    auto &record = iter->second;
    if (callback == nullptr) {
        ret = UnregisterBaseObserverWithLockHeld(dataShareHelper, record);
        CHECK_AND_RETURN_RET(ret == E_OK, ret);
        observerRecords_.erase(iter);
        return E_OK;
    }
    CHECK_AND_RETURN_RET_LOG(HasCallback(record.assetCallbacks, callback), E_INVALID_ARGUMENTS,
        "callback is not registered");
    bool shouldRemoveBase = record.assetCallbacks.size() == 1 && record.albumCallbacks.empty() &&
        record.singleAssetCallbacks.empty() && record.singleAlbumCallbacks.empty();
    if (shouldRemoveBase) {
        ret = UnregisterBaseObserverWithLockHeld(dataShareHelper, record);
        CHECK_AND_RETURN_RET(ret == E_OK, ret);
    }
    g_removeCallback(record.assetCallbacks, callback);
    if (IsRecordEmpty(record)) {
        observerRecords_.erase(iter);
    }
    return E_OK;
}

int32_t MediaLibraryManagerNotifyObserverManager::RegisterAlbumCallback(
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper, Notification::NotifyUriType uriType,
    const std::shared_ptr<PhotoAlbumChangeCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, E_DATASHARE_IS_NULL, "dataShareHelper is nullptr");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, E_INVALID_ARGUMENTS, "callback is nullptr");
    Notification::NotifyUriType registerUriType = Notification::NotifyUriType::INVALID;
    std::string registerUri;
    int32_t ret = MediaLibraryManagerNotifyUtils::GetNotifyTypeAndUri(uriType, registerUriType, registerUri);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);

    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = observerRecords_.find(registerUriType);
    CHECK_AND_RETURN_RET_LOG(!(iter != observerRecords_.end() && HasCallback(iter->second.albumCallbacks, callback)),
        E_INVALID_ARGUMENTS, "callback is already registered");
    auto &record = observerRecords_[registerUriType];
    ret = RegisterBaseObserverWithLockHeld(dataShareHelper, registerUriType, registerUri, record);
    if (ret != E_OK) {
        observerRecords_.erase(registerUriType);
        return ret;
    }
    record.albumCallbacks.push_back(callback);
    return E_OK;
}

int32_t MediaLibraryManagerNotifyObserverManager::UnregisterAlbumCallback(
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper, Notification::NotifyUriType uriType,
    const std::shared_ptr<PhotoAlbumChangeCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, E_DATASHARE_IS_NULL, "dataShareHelper is nullptr");
    Notification::NotifyUriType registerUriType = Notification::NotifyUriType::INVALID;
    std::string registerUri;
    int32_t ret = MediaLibraryManagerNotifyUtils::GetNotifyTypeAndUri(uriType, registerUriType, registerUri);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);

    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = observerRecords_.find(registerUriType);
    CHECK_AND_RETURN_RET_LOG(iter != observerRecords_.end(), E_INVALID_ARGUMENTS, "observer is not registered");
    auto &record = iter->second;
    if (callback == nullptr) {
        ret = UnregisterBaseObserverWithLockHeld(dataShareHelper, record);
        CHECK_AND_RETURN_RET(ret == E_OK, ret);
        observerRecords_.erase(iter);
        return E_OK;
    }
    CHECK_AND_RETURN_RET_LOG(HasCallback(record.albumCallbacks, callback), E_INVALID_ARGUMENTS,
        "callback is not registered");
    bool shouldRemoveBase = record.albumCallbacks.size() == 1 && record.assetCallbacks.empty() &&
        record.singleAssetCallbacks.empty() && record.singleAlbumCallbacks.empty();
    if (shouldRemoveBase) {
        ret = UnregisterBaseObserverWithLockHeld(dataShareHelper, record);
        CHECK_AND_RETURN_RET(ret == E_OK, ret);
    }
    g_removeCallback(record.albumCallbacks, callback);
    if (IsRecordEmpty(record)) {
        observerRecords_.erase(iter);
    }
    return E_OK;
}

int32_t MediaLibraryManagerNotifyObserverManager::RegisterSingleAssetCallback(
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper, Notification::NotifyUriType uriType,
    const std::string &assetUri, const std::shared_ptr<PhotoAssetChangeCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, E_DATASHARE_IS_NULL, "dataShareHelper is nullptr");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, E_INVALID_ARGUMENTS, "callback is nullptr");
    std::string singleId;
    int32_t ret = MediaLibraryManagerNotifyUtils::ExtractSingleId(assetUri, singleId);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    Notification::NotifyUriType registerUriType = Notification::NotifyUriType::INVALID;
    std::string registerUri;
    ret = MediaLibraryManagerNotifyUtils::GetSingleNotifyTypeAndUri(uriType, registerUriType, registerUri);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);

    std::lock_guard<std::mutex> lock(mutex_);
    auto &record = observerRecords_[registerUriType];
    bool isNewBaseObserver = record.observer == nullptr;
    ret = RegisterBaseObserverWithLockHeld(dataShareHelper, registerUriType, registerUri, record);
    if (ret != E_OK) {
        observerRecords_.erase(registerUriType);
        return ret;
    }
    auto idIter = record.singleAssetCallbacks.find(singleId);
    if (idIter != record.singleAssetCallbacks.end()) {
        ret = CheckSinglePhotoPermission(dataShareHelper, uriType, singleId);
        CHECK_AND_RETURN_RET(ret == E_OK, ret);
        CHECK_AND_RETURN_RET_LOG(!HasCallback(idIter->second, callback), E_INVALID_ARGUMENTS,
            "callback is already registered");
        idIter->second.push_back(callback);
        return E_OK;
    }

    ret = RegisterSingleIdWithLockHeld(dataShareHelper, registerUri, singleId, record.observer);
    if (ret != E_OK) {
        if (isNewBaseObserver) {
            UnregisterBaseObserverWithLockHeld(dataShareHelper, record);
            observerRecords_.erase(registerUriType);
        }
        return ret;
    }
    record.singleAssetCallbacks[singleId].push_back(callback);
    return E_OK;
}

int32_t MediaLibraryManagerNotifyObserverManager::UnregisterSingleAssetCallback(
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper, Notification::NotifyUriType uriType,
    const std::string &assetUri, const std::shared_ptr<PhotoAssetChangeCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, E_DATASHARE_IS_NULL, "dataShareHelper is nullptr");
    CHECK_AND_RETURN_RET_LOG(!(assetUri.empty() && callback != nullptr), E_INVALID_ARGUMENTS,
        "empty uri with callback is invalid");
    Notification::NotifyUriType registerUriType = Notification::NotifyUriType::INVALID;
    std::string registerUri;
    int32_t ret = MediaLibraryManagerNotifyUtils::GetSingleNotifyTypeAndUri(uriType, registerUriType, registerUri);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);

    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = observerRecords_.find(registerUriType);
    CHECK_AND_RETURN_RET_LOG(iter != observerRecords_.end(), E_INVALID_ARGUMENTS, "observer is not registered");
    auto &record = iter->second;
    if (assetUri.empty()) {
        for (const auto &item : record.singleAssetCallbacks) {
            ret = UnregisterSingleIdWithLockHeld(dataShareHelper, registerUri, item.first, record.observer);
            CHECK_AND_RETURN_RET(ret == E_OK, ret);
        }
        ret = UnregisterBaseObserverWithLockHeld(dataShareHelper, record);
        CHECK_AND_RETURN_RET(ret == E_OK, ret);
        observerRecords_.erase(iter);
        return E_OK;
    }

    std::string singleId;
    ret = MediaLibraryManagerNotifyUtils::ExtractSingleId(assetUri, singleId);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    auto idIter = record.singleAssetCallbacks.find(singleId);
    CHECK_AND_RETURN_RET_LOG(idIter != record.singleAssetCallbacks.end(), E_INVALID_ARGUMENTS,
        "single asset is not registered");
    if (callback != nullptr) {
        CHECK_AND_RETURN_RET_LOG(HasCallback(idIter->second, callback), E_INVALID_ARGUMENTS,
            "callback is not registered");
        if (idIter->second.size() > 1) {
            g_removeCallback(idIter->second, callback);
            return E_OK;
        }
    }
    ret = UnregisterSingleIdWithLockHeld(dataShareHelper, registerUri, singleId, record.observer);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    record.singleAssetCallbacks.erase(idIter);
    if (IsRecordEmpty(record)) {
        ret = UnregisterBaseObserverWithLockHeld(dataShareHelper, record);
        CHECK_AND_RETURN_RET(ret == E_OK, ret);
        observerRecords_.erase(iter);
    }
    return E_OK;
}

int32_t MediaLibraryManagerNotifyObserverManager::RegisterSingleAlbumCallback(
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper, Notification::NotifyUriType uriType,
    const std::string &albumUri, const std::shared_ptr<PhotoAlbumChangeCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, E_DATASHARE_IS_NULL, "dataShareHelper is nullptr");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, E_INVALID_ARGUMENTS, "callback is nullptr");
    std::string singleId;
    int32_t ret = MediaLibraryManagerNotifyUtils::ExtractSingleId(albumUri, singleId);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    Notification::NotifyUriType registerUriType = Notification::NotifyUriType::INVALID;
    std::string registerUri;
    ret = MediaLibraryManagerNotifyUtils::GetSingleNotifyTypeAndUri(uriType, registerUriType, registerUri);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);

    std::lock_guard<std::mutex> lock(mutex_);
    auto &record = observerRecords_[registerUriType];
    bool isNewBaseObserver = record.observer == nullptr;
    ret = RegisterBaseObserverWithLockHeld(dataShareHelper, registerUriType, registerUri, record);
    if (ret != E_OK) {
        observerRecords_.erase(registerUriType);
        return ret;
    }
    auto idIter = record.singleAlbumCallbacks.find(singleId);
    if (idIter != record.singleAlbumCallbacks.end()) {
        CHECK_AND_RETURN_RET_LOG(!HasCallback(idIter->second, callback), E_INVALID_ARGUMENTS,
            "callback is already registered");
        idIter->second.push_back(callback);
        return E_OK;
    }

    ret = RegisterSingleIdWithLockHeld(dataShareHelper, registerUri, singleId, record.observer);
    if (ret != E_OK) {
        if (isNewBaseObserver) {
            UnregisterBaseObserverWithLockHeld(dataShareHelper, record);
            observerRecords_.erase(registerUriType);
        }
        return ret;
    }
    record.singleAlbumCallbacks[singleId].push_back(callback);
    return E_OK;
}

int32_t MediaLibraryManagerNotifyObserverManager::UnregisterSingleAlbumCallback(
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper, Notification::NotifyUriType uriType,
    const std::string &albumUri, const std::shared_ptr<PhotoAlbumChangeCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, E_DATASHARE_IS_NULL, "dataShareHelper is nullptr");
    CHECK_AND_RETURN_RET_LOG(!(albumUri.empty() && callback != nullptr), E_INVALID_ARGUMENTS,
        "empty uri with callback is invalid");
    Notification::NotifyUriType registerUriType = Notification::NotifyUriType::INVALID;
    std::string registerUri;
    int32_t ret = MediaLibraryManagerNotifyUtils::GetSingleNotifyTypeAndUri(uriType, registerUriType, registerUri);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);

    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = observerRecords_.find(registerUriType);
    CHECK_AND_RETURN_RET_LOG(iter != observerRecords_.end(), E_INVALID_ARGUMENTS, "observer is not registered");
    auto &record = iter->second;
    if (albumUri.empty()) {
        for (const auto &item : record.singleAlbumCallbacks) {
            ret = UnregisterSingleIdWithLockHeld(dataShareHelper, registerUri, item.first, record.observer);
            CHECK_AND_RETURN_RET(ret == E_OK, ret);
        }
        ret = UnregisterBaseObserverWithLockHeld(dataShareHelper, record);
        CHECK_AND_RETURN_RET(ret == E_OK, ret);
        observerRecords_.erase(iter);
        return E_OK;
    }

    std::string singleId;
    ret = MediaLibraryManagerNotifyUtils::ExtractSingleId(albumUri, singleId);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    auto idIter = record.singleAlbumCallbacks.find(singleId);
    CHECK_AND_RETURN_RET_LOG(idIter != record.singleAlbumCallbacks.end(), E_INVALID_ARGUMENTS,
        "single album is not registered");
    if (callback != nullptr) {
        CHECK_AND_RETURN_RET_LOG(HasCallback(idIter->second, callback), E_INVALID_ARGUMENTS,
            "callback is not registered");
        if (idIter->second.size() > 1) {
            g_removeCallback(idIter->second, callback);
            return E_OK;
        }
    }
    ret = UnregisterSingleIdWithLockHeld(dataShareHelper, registerUri, singleId, record.observer);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    record.singleAlbumCallbacks.erase(idIter);
    if (IsRecordEmpty(record)) {
        ret = UnregisterBaseObserverWithLockHeld(dataShareHelper, record);
        CHECK_AND_RETURN_RET(ret == E_OK, ret);
        observerRecords_.erase(iter);
    }
    return E_OK;
}

void MediaLibraryManagerNotifyObserverManager::DispatchAssetChange(
    const std::shared_ptr<Notification::MediaChangeInfo> &changeInfo, Notification::NotifyUriType uriType)
{
    auto infos = MediaLibraryManagerNotifyUtils::BuildPhotoAssetChangeInfos(changeInfo, uriType);
    std::vector<std::shared_ptr<PhotoAssetChangeCallback>> callbacks;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto iter = observerRecords_.find(uriType);
        if (iter != observerRecords_.end()) {
            callbacks = iter->second.assetCallbacks;
        }
    }
    for (const auto &callback : callbacks) {
        CHECK_AND_CONTINUE(callback != nullptr);
        callback->OnChange(infos);
    }
}

void MediaLibraryManagerNotifyObserverManager::DispatchAlbumChange(
    const std::shared_ptr<Notification::MediaChangeInfo> &changeInfo, Notification::NotifyUriType uriType)
{
    auto infos = MediaLibraryManagerNotifyUtils::BuildAlbumChangeInfos(changeInfo);
    std::vector<std::shared_ptr<PhotoAlbumChangeCallback>> callbacks;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto iter = observerRecords_.find(uriType);
        if (iter != observerRecords_.end()) {
            callbacks = iter->second.albumCallbacks;
        }
    }
    for (const auto &callback : callbacks) {
        CHECK_AND_CONTINUE(callback != nullptr);
        callback->OnChange(infos);
    }
}

void MediaLibraryManagerNotifyObserverManager::DispatchSingleAssetChange(
    const std::shared_ptr<Notification::MediaChangeInfo> &changeInfo)
{
    std::map<std::string, std::vector<std::shared_ptr<PhotoAssetChangeCallback>>> singleCallbacks;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto iter = observerRecords_.find(Notification::NotifyUriType::SINGLE_PHOTO_URI);
        CHECK_AND_RETURN(iter != observerRecords_.end());
        singleCallbacks = iter->second.singleAssetCallbacks;
    }
    if (changeInfo->isForRecheck) {
        auto infos = MediaLibraryManagerNotifyUtils::BuildPhotoAssetRecheckChangeInfos();
        for (const auto &item : singleCallbacks) {
            for (const auto &callback : item.second) {
                CHECK_AND_CONTINUE(callback != nullptr);
                callback->OnChange(infos);
            }
        }
        return;
    }
    for (const auto &item : changeInfo->changeInfos) {
        const auto *assetData = std::get_if<AccurateRefresh::PhotoAssetChangeData>(&item);
        CHECK_AND_CONTINUE(assetData != nullptr);
        std::string beforeId = GetValidId(assetData->infoBeforeChange_.fileId_);
        std::string afterId = GetValidId(assetData->infoAfterChange_.fileId_);
        auto callbackIter = singleCallbacks.find(beforeId);
        if (callbackIter == singleCallbacks.end()) {
            callbackIter = singleCallbacks.find(afterId);
        }
        CHECK_AND_CONTINUE(callbackIter != singleCallbacks.end());
        auto infos = MediaLibraryManagerNotifyUtils::BuildSinglePhotoAssetChangeInfos(*assetData, changeInfo);
        for (const auto &callback : callbackIter->second) {
            CHECK_AND_CONTINUE(callback != nullptr);
            callback->OnChange(infos);
        }
    }
}

void MediaLibraryManagerNotifyObserverManager::DispatchSingleAlbumChange(
    const std::shared_ptr<Notification::MediaChangeInfo> &changeInfo)
{
    std::map<std::string, std::vector<std::shared_ptr<PhotoAlbumChangeCallback>>> singleCallbacks;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto iter = observerRecords_.find(Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI);
        CHECK_AND_RETURN(iter != observerRecords_.end());
        singleCallbacks = iter->second.singleAlbumCallbacks;
    }
    if (changeInfo->isForRecheck) {
        auto infos = MediaLibraryManagerNotifyUtils::BuildAlbumRecheckChangeInfos();
        for (const auto &item : singleCallbacks) {
            for (const auto &callback : item.second) {
                CHECK_AND_CONTINUE(callback != nullptr);
                callback->OnChange(infos);
            }
        }
        return;
    }
    for (const auto &item : changeInfo->changeInfos) {
        const auto *albumData = std::get_if<AccurateRefresh::AlbumChangeData>(&item);
        CHECK_AND_CONTINUE(albumData != nullptr);
        std::string beforeId = GetValidId(albumData->infoBeforeChange_.albumId_);
        std::string afterId = GetValidId(albumData->infoAfterChange_.albumId_);
        auto callbackIter = singleCallbacks.find(beforeId);
        if (callbackIter == singleCallbacks.end()) {
            callbackIter = singleCallbacks.find(afterId);
        }
        CHECK_AND_CONTINUE(callbackIter != singleCallbacks.end());
        auto infos = MediaLibraryManagerNotifyUtils::BuildSingleAlbumChangeInfos(*albumData, changeInfo);
        for (const auto &callback : callbackIter->second) {
            CHECK_AND_CONTINUE(callback != nullptr);
            callback->OnChange(infos);
        }
    }
}

void MediaLibraryManagerNotifyObserverManager::NotifyChange(
    const std::shared_ptr<Notification::MediaChangeInfo> &changeInfo)
{
    CHECK_AND_RETURN(changeInfo != nullptr);
    switch (changeInfo->notifyUri) {
        case Notification::NotifyUriType::PHOTO_URI:
            DispatchAssetChange(changeInfo, Notification::NotifyUriType::PHOTO_URI);
            DispatchSingleAssetChange(changeInfo);
            break;
        case Notification::NotifyUriType::HIDDEN_PHOTO_URI:
            DispatchAssetChange(changeInfo, Notification::NotifyUriType::HIDDEN_PHOTO_URI);
            break;
        case Notification::NotifyUriType::TRASH_PHOTO_URI:
            DispatchAssetChange(changeInfo, Notification::NotifyUriType::TRASH_PHOTO_URI);
            break;
        case Notification::NotifyUriType::SINGLE_PHOTO_URI:
            DispatchSingleAssetChange(changeInfo);
            break;
        case Notification::NotifyUriType::PHOTO_ALBUM_URI:
            DispatchAlbumChange(changeInfo, Notification::NotifyUriType::PHOTO_ALBUM_URI);
            DispatchSingleAlbumChange(changeInfo);
            break;
        case Notification::NotifyUriType::HIDDEN_ALBUM_URI:
            DispatchAlbumChange(changeInfo, Notification::NotifyUriType::HIDDEN_ALBUM_URI);
            break;
        case Notification::NotifyUriType::TRASH_ALBUM_URI:
            DispatchAlbumChange(changeInfo, Notification::NotifyUriType::TRASH_ALBUM_URI);
            break;
        case Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI:
            DispatchSingleAlbumChange(changeInfo);
            break;
        default:
            MEDIA_WARN_LOG("unsupported notify uri type: %{public}d", static_cast<int32_t>(changeInfo->notifyUri));
            break;
    }
}
} // namespace Media
} // namespace OHOS
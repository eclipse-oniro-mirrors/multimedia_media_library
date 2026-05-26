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

#include "notification_helper.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include <mutex>
#include <memory>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <cstring>
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "accesstoken_kit.h"
#include "ipc_skeleton.h"
#include "media_file_utils.h"
#include "medialibrary_db_const.h"
#include "album_change_info.h"
#include "media_change_info.h"
#include "photo_album_column.h"
#include "base_data_uri.h"
#include "message_parcel.h"
#include "securec.h"

#undef MLOG_TAG
#define MLOG_TAG "NotificationHelper"

namespace OHOS {
namespace Media {
namespace NotificationHelper {

namespace {
constexpr const char* PERM_READ_IMAGEVIDEO = "ohos.permission.READ_IMAGEVIDEO";
constexpr size_t MAX_OBSERVER_PARCEL_SIZE = 4 * 1024 * 1024;

static sptr<IRemoteObject> InitToken()
{
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHECK_AND_RETURN_RET_LOG(saManager != nullptr, nullptr, "get system ability mgr failed.");
    auto remoteObj = saManager->GetSystemAbility(STORAGE_MANAGER_MANAGER_ID);
    CHECK_AND_RETURN_RET_LOG(remoteObj != nullptr, nullptr, "GetSystemAbility Service failed.");
    return remoteObj;
}
}

static int32_t CheckRegisterPreconditions()
{
    Security::AccessToken::AccessTokenID tokenToVerify;
    pid_t callingPid = IPCSkeleton::GetCallingPid();
    pid_t selfPid = getpid();
    bool isSameProcess = (callingPid == selfPid) ||
                         (callingPid <= 0) ||
                         (IPCSkeleton::GetCallingTokenID() == 0);
    if (isSameProcess) {
        tokenToVerify = IPCSkeleton::GetSelfTokenID();
    } else {
        tokenToVerify = IPCSkeleton::GetCallingTokenID();
    }
    if (Security::AccessToken::AccessTokenKit::VerifyAccessToken(tokenToVerify, PERM_READ_IMAGEVIDEO)
        != Security::AccessToken::PermissionState::PERMISSION_GRANTED) {
        return NOTIFY_ERR_PERMISSION;
    }
    if (!isSameProcess) {
        if (!MediaFileUtils::IsDirectory(CONST_MEDIA_DB_DIR)) {
            return NOTIFY_ERR_FS_EXCEPTION;
        }
    }
    return NOTIFY_OK;
}

static int32_t CheckUnregisterPermissionOnly()
{
    Security::AccessToken::AccessTokenID tokenToVerify;
    pid_t callingPid = IPCSkeleton::GetCallingPid();
    pid_t selfPid = getpid();
    bool isSameProcess = (callingPid == selfPid) || (callingPid <= 0) ||
                         (IPCSkeleton::GetCallingTokenID() == 0);
    if (isSameProcess) {
        tokenToVerify = IPCSkeleton::GetSelfTokenID();
    } else {
        tokenToVerify = IPCSkeleton::GetCallingTokenID();
    }
    if (Security::AccessToken::AccessTokenKit::VerifyAccessToken(tokenToVerify, PERM_READ_IMAGEVIDEO)
        != Security::AccessToken::PermissionState::PERMISSION_GRANTED) {
        return NOTIFY_ERR_PERMISSION;
    }
    return NOTIFY_OK;
}

// Static members
std::mutex NotificationHelper::callbackMutex_;
std::vector<std::weak_ptr<PhotoAlbumChangeCallback>> NotificationHelper::callbacks_;
std::shared_ptr<DataShare::DataShareHelper> NotificationHelper::dataShareHelper_;
std::shared_ptr<DataShare::DataShareObserver> NotificationHelper::internalObserver_;

static NotifyChangeType ConvertNotifyType(Notification::AccurateNotifyType notifyType)
{
    switch (notifyType) {
        case Notification::NOTIFY_ALBUM_ADD:
            return NotifyChangeType::NOTIFY_CHANGE_ADD;
        case Notification::NOTIFY_ALBUM_UPDATE:
            return NotifyChangeType::NOTIFY_CHANGE_UPDATE;
        case Notification::NOTIFY_ALBUM_REMOVE:
            return NotifyChangeType::NOTIFY_CHANGE_REMOVE;
        default:
            return NotifyChangeType::NOTIFY_CHANGE_INVALID;
    }
}

static AlbumChangeData ConvertChangeData(const AccurateRefresh::AlbumChangeData& changeData)
{
    AlbumChangeData helperChangeData;
    helperChangeData.version = changeData.version_;

    if (changeData.infoBeforeChange_.albumId_ != AccurateRefresh::INVALID_INT32_VALUE) {
        helperChangeData.albumBeforeChange = std::make_shared<AccurateRefresh::AlbumChangeInfo>(
            changeData.infoBeforeChange_);
    }

    if (changeData.infoAfterChange_.albumId_ != AccurateRefresh::INVALID_INT32_VALUE) {
        helperChangeData.albumAfterChange = std::make_shared<AccurateRefresh::AlbumChangeInfo>(
            changeData.infoAfterChange_);
    }

    return helperChangeData;
}

class InternalAlbumObserver : public DataShare::DataShareObserver {
public:
    void OnChange(const ChangeInfo &changeInfo) override
    {
    MEDIA_INFO_LOG("InternalAlbumObserver::OnChange received, pid:%{public}d, "
        "data:%{public}s, size:%{public}d",
        getpid(),
        (changeInfo.data_ == nullptr ? "null" : "non-null"),
        static_cast<int32_t>(changeInfo.size_));
    if (changeInfo.data_ == nullptr || changeInfo.size_ <= 0 ||
        changeInfo.size_ > MAX_OBSERVER_PARCEL_SIZE) {
        MEDIA_INFO_LOG("OnChange: no parcel data or invalid size, skipping. size:%{public}d",
            static_cast<int32_t>(changeInfo.size_));
        return;
    }

    uint8_t *parcelData = static_cast<uint8_t *>(malloc(changeInfo.size_));
    if (parcelData == nullptr) {
        MEDIA_ERR_LOG("Failed to allocate parcel buffer");
        return;
    }
    if (memcpy_s(parcelData, static_cast<size_t>(changeInfo.size_), changeInfo.data_,
        static_cast<size_t>(changeInfo.size_)) != 0) {
        MEDIA_ERR_LOG("Failed to copy parcel data");
        free(parcelData);
        return;
    }

    auto parcel = std::make_shared<MessageParcel>();
    if (!parcel->ParseFrom(reinterpret_cast<uintptr_t>(parcelData), changeInfo.size_)) {
        MEDIA_ERR_LOG("Failed to parse parcel data");
        free(parcelData);
        return;
    }

    Notification::MediaChangeInfo mediaChangeInfo;
    if (!mediaChangeInfo.ReadFromParcelInMultiMode(*parcel)) {
        MEDIA_ERR_LOG("Failed to read MediaChangeInfo from parcel");
        return;
    }

    DispatchMediaChangeInfo(mediaChangeInfo);
    }

private:
    void DispatchMediaChangeInfo(const Notification::MediaChangeInfo& mediaChangeInfo)
    {
        AlbumChangeInfos changeInfos;
        changeInfos.isForRecheck = mediaChangeInfo.isForRecheck;
        changeInfos.type = ConvertNotifyType(mediaChangeInfo.notifyType);
        MEDIA_INFO_LOG("DispatchMediaChangeInfo: notifyType:%{public}d, isForRecheck:%{public}d, "
            "rawItems:%{public}zu",
            static_cast<int32_t>(mediaChangeInfo.notifyType),
            static_cast<int32_t>(mediaChangeInfo.isForRecheck),
            mediaChangeInfo.changeInfos.size());
        if (changeInfos.type == NotifyChangeType::NOTIFY_CHANGE_INVALID && !changeInfos.isForRecheck) {
            MEDIA_INFO_LOG("DispatchMediaChangeInfo: invalid notifyType and not recheck, dropping");
            return;
        }
        if (changeInfos.isForRecheck) {
            changeInfos.type = NotifyChangeType::NOTIFY_CHANGE_ADD;
        }
        for (const auto& item : mediaChangeInfo.changeInfos) {
            if (!std::holds_alternative<AccurateRefresh::AlbumChangeData>(item)) {
                continue;
            }
            const auto& albumData = std::get<AccurateRefresh::AlbumChangeData>(item);
            changeInfos.albumChangeDatas.push_back(ConvertChangeData(albumData));
        }
        MEDIA_INFO_LOG("DispatchMediaChangeInfo: albumChangeDatas built, count:%{public}zu",
            changeInfos.albumChangeDatas.size());
        std::lock_guard<std::mutex> lock(NotificationHelper::callbackMutex_);
        NotificationHelper::NotifyAllCallbacks(changeInfos);
}
};

void NotificationHelper::NotifyAllCallbacks(const AlbumChangeInfos& changeInfos)
{
    MEDIA_INFO_LOG("NotifyAllCallbacks enter: callbacks:%{public}zu, type:%{public}d, "
        "albumChangeDatas:%{public}zu, isForRecheck:%{public}d",
        callbacks_.size(),
        static_cast<int32_t>(changeInfos.type),
        changeInfos.albumChangeDatas.size(),
        static_cast<int32_t>(changeInfos.isForRecheck));
    if (callbacks_.empty()) {
        MEDIA_ERR_LOG("NotifyAllCallbacks: callbacks_ is EMPTY at notify time; observer may have "
            "been registered but client process has no live callback (check cross-process state)");
        return;
    }

    int32_t dispatched = 0;
    int32_t expired = 0;
    auto it = callbacks_.begin();
    while (it != callbacks_.end()) {
        auto callback = it->lock();
        if (callback == nullptr) {
            it = callbacks_.erase(it);
            ++expired;
            continue;
        }

        callback->OnChange(changeInfos);
        ++dispatched;
        ++it;
    }
    MEDIA_INFO_LOG("NotifyAllCallbacks done: dispatched:%{public}d, expired:%{public}d, remaining:%{public}zu",
        dispatched, expired, callbacks_.size());
}

bool NotificationHelper::StartObserverIfNeeded()
{
    MEDIA_INFO_LOG("StartObserverIfNeeded enter: pid:%{public}d, internalObserver_:%{public}s, "
        "dataShareHelper_:%{public}s, callbacks:%{public}zu",
        getpid(),
        (internalObserver_ == nullptr ? "null" : "non-null"),
        (dataShareHelper_ == nullptr ? "null" : "non-null"),
        callbacks_.size());
    if (internalObserver_ != nullptr) {
        MEDIA_INFO_LOG("StartObserverIfNeeded: observer already set, skipping setup");
        return true;
    }

    // Use the common Creator(token, uri) pattern used across MediaLibrary clients.
    auto token = InitToken();
    if (token == nullptr) {
        MEDIA_ERR_LOG("StartObserverIfNeeded: Failed to init token for DataShareHelper");
        return false;
    }
    MEDIA_INFO_LOG("StartObserverIfNeeded: token obtained, proceeding with Creator()");

    // Creating DataShareHelper may fail during early boot / ability not ready.
    // Add minimal retry to reduce flakiness and add high-signal logs for joint debugging.
    constexpr int32_t maxCreateRetry = 3;
    constexpr long long retryDelayMs = 200;
    for (int32_t i = 0; i < maxCreateRetry; ++i) {
        dataShareHelper_ = DataShare::DataShareHelper::Creator(token, MEDIALIBRARY_DATA_URI);
        if (dataShareHelper_ != nullptr) {
            MEDIA_INFO_LOG("StartObserverIfNeeded: DataShareHelper created on attempt %{public}d/%{public}d",
                i + 1, maxCreateRetry);
            break;
        }
        MEDIA_ERR_LOG("Failed to create DataShareHelper (attempt %{public}d/%{public}d), uri:%{public}s",
            i + 1, maxCreateRetry, MEDIALIBRARY_DATA_URI.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
    }
    if (dataShareHelper_ == nullptr) {
        MEDIA_ERR_LOG("StartObserverIfNeeded: DataShareHelper still NULL after %{public}d attempts; "
            "observer will NOT be registered. Future notifications will not be delivered to this client!",
            maxCreateRetry);
        return false;
    }

    internalObserver_ = std::make_shared<InternalAlbumObserver>();
    // DataShareExtAbility expects notify keys like "photoAlbumChange" (mapped to NotifyUriType::PHOTO_ALBUM_URI).
    // Using query URIs like file://media/PhotoAlbum/ will not receive MediaLibraryNotifyNew notifications.
    static const std::string photoAlbumChangeKey = "photoAlbumChange";
    // Note: RegisterObserverExt returns void in this API, so we can't get a ret code here.
    dataShareHelper_->RegisterObserverExtProvider(Uri(photoAlbumChangeKey), internalObserver_, false);
    MEDIA_INFO_LOG("StartObserverIfNeeded: observer registered, key:%{public}s, "
        "observer:%{public}s, helper:%{public}s",
        photoAlbumChangeKey.c_str(),
        (internalObserver_ == nullptr ? "null" : "non-null"),
        (dataShareHelper_ == nullptr ? "null" : "non-null"));
    return true;
}

void NotificationHelper::StopObserverIfNeeded()
{
    MEDIA_INFO_LOG("StopObserverIfNeeded enter: pid:%{public}d, internalObserver_:%{public}s, "
        "dataShareHelper_:%{public}s, callbacks:%{public}zu",
        getpid(),
        (internalObserver_ == nullptr ? "null" : "non-null"),
        (dataShareHelper_ == nullptr ? "null" : "non-null"),
        callbacks_.size());
    if (internalObserver_ == nullptr || !callbacks_.empty()) {
        MEDIA_INFO_LOG("StopObserverIfNeeded: nothing to tear down");
        return;
    }

    if (dataShareHelper_ != nullptr) {
        static const std::string photoAlbumChangeKey = "photoAlbumChange";
        dataShareHelper_->UnregisterObserverExtProvider(Uri(photoAlbumChangeKey), internalObserver_);
        MEDIA_INFO_LOG("Internal album observer unregistered for %{public}s", photoAlbumChangeKey.c_str());
    }
    internalObserver_ = nullptr;
    dataShareHelper_ = nullptr;
    MEDIA_INFO_LOG("StopObserverIfNeeded: observer/helper cleared");
}

int32_t NotificationHelper::RegisterPhotoAlbumCallback(std::shared_ptr<PhotoAlbumChangeCallback> callback)
{
    MEDIA_INFO_LOG("RegisterPhotoAlbumCallback start, pid:%{public}d, tid:%{public}d, "
        "callbackPtr:%{public}s", getpid(), static_cast<int32_t>(gettid()),
        (callback == nullptr ? "null" : "non-null"));
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, NOTIFY_ERR_PERMISSION, "callback is nullptr");
    int32_t precheck = CheckRegisterPreconditions();
    CHECK_AND_RETURN_RET_LOG(precheck == NOTIFY_OK, precheck,
        "RegisterPhotoAlbumCallback precheck failed, ret:%{public}d", precheck);
    bool added = false;
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        auto it = callbacks_.begin();
        while (it != callbacks_.end()) {
            auto existing = it->lock();
            if (existing == nullptr) {
                it = callbacks_.erase(it);
                continue;
            }
            if (existing.get() == callback.get()) {
                MEDIA_INFO_LOG("Same callback already registered, skipping duplicate. "
                    "callbacks:%{public}zu", callbacks_.size());
                return NOTIFY_OK;
            }
            ++it;
        }
        callbacks_.push_back(std::weak_ptr<PhotoAlbumChangeCallback>(callback));
        added = true;
        MEDIA_INFO_LOG("RegisterPhotoAlbumCallback success, callback count: %{public}zu", callbacks_.size());
    }
    // Do not hold callbackMutex_ while creating DataShareHelper / sleeping in retry.
    if (added && !StartObserverIfNeeded()) {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        auto it = callbacks_.begin();
        while (it != callbacks_.end()) {
            auto existing = it->lock();
            if (existing == nullptr || existing.get() == callback.get()) {
                it = callbacks_.erase(it);
                continue;
            }
            ++it;
        }
        MEDIA_ERR_LOG("RegisterPhotoAlbumCallback failed: observer setup failed, "
            "callback removed, observerSet:%{public}s, helperSet:%{public}s",
            (internalObserver_ == nullptr ? "false" : "true"), (dataShareHelper_ == nullptr ? "false" : "true"));
        return NOTIFY_ERR_IPC_TIMEOUT;
    }
    MEDIA_INFO_LOG("RegisterPhotoAlbumCallback return: observerSet:%{public}s, helperSet:%{public}s",
        (internalObserver_ == nullptr ? "false" : "true"), (dataShareHelper_ == nullptr ? "false" : "true"));
    return NOTIFY_OK;
}

int32_t NotificationHelper::unRegisterPhotoAlbumCallback(std::shared_ptr<PhotoAlbumChangeCallback> callback)
{
    MEDIA_INFO_LOG("unRegisterPhotoAlbumCallback start, pid:%{public}d, tid:%{public}d, "
        "callbackPtr:%{public}s, callbacks:%{public}zu",
        getpid(),
        static_cast<int32_t>(gettid()),
        (callback == nullptr ? "null" : "non-null"),
        callbacks_.size());
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, NOTIFY_ERR_UNREGISTER_REPEAT, "callback is nullptr");
    int32_t precheck = CheckUnregisterPermissionOnly();
    CHECK_AND_RETURN_RET_LOG(precheck == NOTIFY_OK, precheck,
        "unRegisterPhotoAlbumCallback permission failed, ret:%{public}d", precheck);

    std::lock_guard<std::mutex> lock(callbackMutex_);

    bool removed = false;
    auto it = callbacks_.begin();
    while (it != callbacks_.end()) {
        auto sharedCallback = it->lock();
        if (sharedCallback == nullptr) {
            it = callbacks_.erase(it);
            continue;
        }
        if (sharedCallback.get() == callback.get()) {
            it = callbacks_.erase(it);
            removed = true;
            continue;
        }
        ++it;
    }
    if (!removed) {
        MEDIA_ERR_LOG("Callback not found for unregistration");
        return NOTIFY_ERR_UNREGISTER_REPEAT;
    }
    MEDIA_INFO_LOG("unRegisterPhotoAlbumCallback success, remaining callbacks: %{public}zu", callbacks_.size());

    StopObserverIfNeeded();

    return NOTIFY_OK;
}

} // namespace NotificationHelper
} // namespace Media
} // namespace OHOS
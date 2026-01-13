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


#include "notification_distribution.h"

#include "media_log.h"
#include "notify_info.h"
#include "media_observer_manager.h"
#include "notify_register_permission.h"
#include "media_column.h"
#include "photo_album_column.h"
#include "media_change_info.h"
#include "medialibrary_errno.h"
#include "observer_info.h"
#include "notification_merging.h"
#include "media_notification_utils.h"
#include "medialibrary_tracer.h"

#include <map>
#include <unordered_set>
#include <securec.h>

using namespace std;

namespace OHOS {
namespace Media {
namespace Notification {
NotificationDistribution::NotificationDistribution() {}

NotificationDistribution::~NotificationDistribution() {}

MediaChangeInfo NotificationDistribution::FilterNotifyInfoByPermission(
    const MediaChangeInfo& changeInfo, NotifyUriType notifyUriType)
{
    const NotifyUriType changeUri = changeInfo.notifyUri;

    if ((notifyUriType == NotifyUriType::PHOTO_URI ||
         notifyUriType == NotifyUriType::PHOTO_ALBUM_URI ||
         notifyUriType == NotifyUriType::SINGLE_PHOTO_URI ||
         notifyUriType == NotifyUriType::SINGLE_PHOTO_ALBUM_URI) &&
         changeUri == notifyUriType) {
        return changeInfo;
    }

    if (notifyUriType == NotifyUriType::HIDDEN_PHOTO_URI ||
        notifyUriType == NotifyUriType::HIDDEN_ALBUM_URI) {
        return changeInfo;
    }

    if ((notifyUriType == NotifyUriType::TRASH_PHOTO_URI ||
         notifyUriType == NotifyUriType::TRASH_ALBUM_URI) &&
        (changeUri == NotifyUriType::PHOTO_URI ||
         changeUri == NotifyUriType::TRASH_PHOTO_URI ||
         changeUri == NotifyUriType::PHOTO_ALBUM_URI ||
         changeUri == NotifyUriType::TRASH_ALBUM_URI)) {
        return changeInfo;
    }

    return MediaChangeInfo{};
}

int32_t NotificationDistribution::SendNotificationWithRecheckChangeInfo(
    const MediaChangeInfo& changeInfo, const ObserverInfo& observerInfo)
{
    MediaChangeInfo recheckChangeInfo = changeInfo;
    recheckChangeInfo.changeInfos.clear();
    recheckChangeInfo.isForRecheck = true;
    recheckChangeInfo.isSystem = observerInfo.isSystem;
    if (changeInfo.notifyType == AccurateNotifyType::NOTIFY_ASSET_ADD ||
        changeInfo.notifyType == AccurateNotifyType::NOTIFY_ASSET_UPDATE ||
        changeInfo.notifyType == AccurateNotifyType::NOTIFY_ASSET_REMOVE) {
        recheckChangeInfo.notifyType = AccurateNotifyType::NOTIFY_ASSET_ADD;
    } else {
        recheckChangeInfo.notifyType = AccurateNotifyType::NOTIFY_ALBUM_ADD;
    }
    shared_ptr<MediaChangeInfo> sharedChangeInfo = make_shared<MediaChangeInfo>(recheckChangeInfo);
    return NotificationUtils::SendNotification(observerInfo.observer, sharedChangeInfo);
}

int32_t NotificationDistribution::ProcessMediaChangeInfos(
    const std::vector<Notification::MediaChangeInfo>& mediaChangeInfos,
    Notification::NotifyUriType notifyUriType,
    const ObserverInfo& observerInfo)
{
    MediaLibraryTracer tracer;
    tracer.Start("ProcessMediaChangeInfos");
    for (const auto& mediaChangeInfo : mediaChangeInfos) {
        // 如果存在一个isForRecheck为true，只需要发送一个add通知
        if (mediaChangeInfo.isForRecheck) {
            return SendNotificationWithRecheckChangeInfo(mediaChangeInfo, observerInfo);
        }
    }
    for (const auto& mediaChangeInfo : mediaChangeInfos) {
        MEDIA_INFO_LOG("mediaChangeInfo:%{public}s", mediaChangeInfo.ToString().c_str());
        if (mediaChangeInfo.changeInfos.empty()) {
            MEDIA_INFO_LOG("mediaChangeInfo changeInfos is null");
            continue;
        }
        MediaChangeInfo filteredInfo = FilterNotifyInfoByPermission(mediaChangeInfo, notifyUriType);
        if (filteredInfo.changeInfos.empty()) {
            MEDIA_INFO_LOG("After filtering mediaChangeInfo changeInfos is null");
            continue;
        }
        filteredInfo.isSystem = observerInfo.isSystem;
        shared_ptr<MediaChangeInfo> sharedChangeInfo = make_shared<MediaChangeInfo>(filteredInfo);
        int32_t ret = NotificationUtils::SendNotification(observerInfo.observer, sharedChangeInfo);
        CHECK_AND_RETURN_RET_LOG(ret != E_OK, ret, "CallbackProcessing fail err:%{public}d", ret);
        MEDIA_INFO_LOG("CallbackProcessing ret:%{public}d", ret);
    }
    return E_OK;
}

int32_t NotificationDistribution::ProcessNotifyInfo(const NotifyInfo& notifyInfo)
{
    MediaLibraryTracer tracer;
    tracer.Start("ProcessNotifyInfo");
    for (const auto& observerInfo : notifyInfo.observerInfos) {
        for (const auto& [notifyUriType, mediaChangeInfos] : notifyInfo.changeInfosMap) {
            if (!mediaChangeInfos.empty()) {
                ProcessMediaChangeInfos(mediaChangeInfos, notifyUriType, observerInfo);
            }
        }
    }
    return E_OK;
}

int32_t NotificationDistribution::DistributeNotifyInfo(const std::vector<NotifyInfo>& notifyInfos)
{
    CHECK_AND_RETURN_RET_LOG(!notifyInfos.empty(), E_OK, "notifyInfos is null");

    MEDIA_INFO_LOG("enter distributeNotifyInfo, notifyInfos size:%{public}d", static_cast<int32_t>(notifyInfos.size()));
    MediaLibraryTracer tracer;
    tracer.Start("DistributeNotifyInfo");
    for (const auto& notifyInfo : notifyInfos) {
        if (notifyInfo.observerInfos.empty() || notifyInfo.changeInfosMap.empty()) {
            MEDIA_INFO_LOG("notifyInfo has empty observerInfos or changeInfosMap");
            continue;
        }

        ProcessNotifyInfo(notifyInfo);
    }
    return E_OK;
}

int32_t NotificationDistribution::ProcessUserDefineNotifyInfo(const UserDefineNotifyInfo &notifyInfo)
{
    MEDIA_INFO_LOG("ProcessUserDefineNotifyInfo %{public}s.", notifyInfo.ToString().c_str());
    if (notifyInfo.notifyUri_ != NotifyUriType::USER_DEFINE_NOTIFY_URI ||
        notifyInfo.notifyUserDefineType_ == NotifyForUserDefineType::UNDEFINED) {
        MEDIA_ERR_LOG("notifyUriType: %{public}d, notifyUserDefineType: %{public}d is nou supported.",
            static_cast<int32_t>(notifyInfo.notifyUri_), static_cast<int32_t>(notifyInfo.notifyUserDefineType_));
        return E_ERR;
    }
 
    std::vector<ObserverInfo> obsInfos = findObservers(notifyInfo.notifyUri_);
    if (obsInfos.empty()) {
        MEDIA_ERR_LOG("obsInfos is empty.");
        return E_ERR;
    }
 
    std::shared_ptr<Parcel> parcel = std::make_shared<Parcel>();
    CHECK_AND_RETURN_RET_LOG(parcel != nullptr, E_ERR, "parcel is null");
    notifyInfo.WriteHeadFromParcel(parcel);
    notifyInfo.WriteBodyFromParcel(parcel);
 
    uintptr_t buf = parcel->GetData();
    auto *uBuf = new (std::nothrow) uint8_t[parcel->GetDataSize()];
    CHECK_AND_RETURN_RET_LOG(uBuf != nullptr, E_ERR, "parcel GetDataSize is null");
    memcpy_s(uBuf, parcel->GetDataSize(), reinterpret_cast<uint8_t *>(buf), parcel->GetDataSize());
 
    std::shared_ptr<AAFwk::ChangeInfo> serverChangeInfo = std::make_shared<AAFwk::ChangeInfo>();
    CHECK_AND_RETURN_RET_LOG(serverChangeInfo != nullptr, E_ERR, "serverChangeInfo is null");
    serverChangeInfo->data_ = uBuf;
    serverChangeInfo->size_ = parcel->GetDataSize();
    MEDIA_INFO_LOG("ProcessUserDefineNotifyInfo serverChangeInfo->size_ is: %{public}zu", parcel->GetDataSize());
 
    for (const ObserverInfo& obsInfo : obsInfos) {
        NotificationUtils::SendUserDefineNotification(obsInfo.observer, serverChangeInfo);
    }
    if (serverChangeInfo->data_ != nullptr) {
        delete[] static_cast<uint8_t*>(serverChangeInfo->data_);
        serverChangeInfo->data_ = nullptr;
    }
    return E_OK;
}
 
int32_t NotificationDistribution::DistributeUserDefineNotifyInfo(const std::vector<UserDefineNotifyInfo>& notifyInfos)
{
    CHECK_AND_RETURN_RET_LOG(!notifyInfos.empty(), E_OK, "notifyInfos is null");
    MEDIA_INFO_LOG("DistributeUserDefineNotifyInfo, size:%{public}d", static_cast<int32_t>(notifyInfos.size()));
 
    for (const auto& notifyInfo : notifyInfos) {
        auto notifyUri = notifyInfo.notifyUri_;
        if (notifyUri != NotifyUriType::USER_DEFINE_NOTIFY_URI) {
            MEDIA_WARN_LOG("notifyInfo is invalid, notifyUri: %{public}d.", static_cast<int32_t>(notifyUri));
            continue;
        }
 
        ProcessUserDefineNotifyInfo(notifyInfo);
    }
    return E_OK;
}
 
std::vector<ObserverInfo> NotificationDistribution::findObservers(NotifyUriType notifyUriType)
{
    MEDIA_INFO_LOG("enter findObservers,notifyUriType:%{public}d", static_cast<int32_t>(notifyUriType));
    static auto manager = MediaObserverManager::GetObserverManager();
    if (manager == nullptr) {
        manager = MediaObserverManager::GetObserverManager();
        if (manager == nullptr) {
            MEDIA_ERR_LOG("Failed to get ObserverManager!");
            return {};
        }
    }
    return manager->FindObserver(notifyUriType);
}
} // Notification
} // Media
} // OHOS
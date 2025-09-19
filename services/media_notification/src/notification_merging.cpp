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
#define MLOG_TAG "NotificationMerging"

#include "notification_merging.h"

#include "media_log.h"
#include "notify_info.h"
#include "media_observer_manager.h"
#include "notify_register_permission.h"
#include "media_column.h"
#include "photo_album_column.h"
#include "media_change_info.h"
#include "medialibrary_errno.h"
#include "observer_info.h"
#include "media_datashare_stub_impl.h"

#include <map>
#include <unordered_set>
#include <set>

using namespace std;
using namespace OHOS::Media::AccurateRefresh;
using namespace OHOS::DataShare;

namespace OHOS {
namespace Media {
namespace Notification {

NotificationMerging::NotificationMerging() {}

NotificationMerging::~NotificationMerging() {}

std::vector<ObserverInfo> NotificationMerging::findObservers(NotifyUriType notifyUriType)
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

std::vector<NotifyInfo> NotificationMerging::ProcessNotifyInfos(const std::vector<MediaChangeInfo> &mediaChangeInfos)
{
    MEDIA_INFO_LOG("Enter ProcessNotifyInfos");
    CHECK_AND_RETURN_RET_LOG(!mediaChangeInfos.empty(), {}, "mediaChangeInfos is empty");

    auto manager = MediaObserverManager::GetObserverManager();
    CHECK_AND_RETURN_RET_LOG(manager != nullptr, {}, "manager is null");

    std::map<std::pair<NotifyUriType, NotifyType>, MediaChangeInfo> mergedChanges;
    std::vector<NotifyInfo> notifyInfos;
    // 对notifyUri和notifyType相同的进行合并
    for (const auto &info : mediaChangeInfos) {
        MEDIA_INFO_LOG("processing info: %{public}s", info.ToString().c_str());
        auto key = std::make_pair(info.notifyUri, info.notifyType);
        if (mergedChanges.find(key) == mergedChanges.end()) {
            mergedChanges[key] = info;
        } else {
            auto& existing = mergedChanges[key];
            existing.changeInfos.insert(
                existing.changeInfos.end(),
                info.changeInfos.begin(),
                info.changeInfos.end()
            );
        }
    }

    // 将NotifyUriType相同的合并在同一个NotifyInfo中
    std::map<Notification::NotifyUriType, Notification::NotifyInfo> uriToNotifyInfoMap;
    for (const auto &[key, mergedInfo] : mergedChanges) {
        auto [notifyUri, notifyType] = key;
        if (uriToNotifyInfoMap.find(notifyUri) == uriToNotifyInfoMap.end()) {
            NotifyInfo notifyInfo;
            const std::vector<ObserverInfo> observerInfos = findObservers(notifyUri);
            if (!observerInfos.empty()) {
                notifyInfo.observerInfos.insert(notifyInfo.observerInfos.end(),
                    observerInfos.begin(), observerInfos.end());
            }
            uriToNotifyInfoMap[notifyUri] = notifyInfo;
        }
        uriToNotifyInfoMap[notifyUri].changeInfosMap[notifyUri].push_back(mergedInfo);
    }

    // 将分组后的结果转移到notifyInfos中
    for (auto &[notifyUri, notifyInfo] : uriToNotifyInfoMap) {
        MEDIA_INFO_LOG("notifyUri is:%{public}d", static_cast<int32_t>(notifyUri));
        notifyInfos.push_back(std::move(notifyInfo));
    }

    MEDIA_INFO_LOG("ProcessNotifyInfos completed");
    return notifyInfos;
}

std::vector<NotifyInfo> NotificationMerging::MergeNotifyInfo(std::vector<MediaChangeInfo> changeInfos)
{
    MEDIA_INFO_LOG("Merging notification information");
    CHECK_AND_RETURN_RET_LOG(!changeInfos.empty(), {}, "changeInfos is null");
    return ProcessNotifyInfos(changeInfos);
}

int32_t NotificationMerging::ProcessNotifyDownloadProgressInfo(Notification::DownloadAssetsNotifyType
    downloadAssetsNotifyType, int32_t fileId, int32_t percent, int32_t autoPauseReason)
{
    MEDIA_INFO_LOG("ProcessNotifyDownloadProgressInfo In downloadAssetsNotifyType: %{public}d, fileId: %{public}d,"
        " percent: %{public}d", static_cast<int32_t>(downloadAssetsNotifyType), fileId, percent);
    std::vector<ObserverInfo> obsInfos = findObservers(NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI);
    // Build AssetManagerNotifyInfo as changeInfo
    std::shared_ptr<Parcel> parcel = std::make_shared<Parcel>();
    parcel->WriteUint16(static_cast<uint16_t>(NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI));
    parcel->WriteUint16(static_cast<uint16_t>(downloadAssetsNotifyType));
    parcel->WriteInt32(fileId);
    parcel->WriteInt32(percent);
    parcel->WriteInt32(autoPauseReason);
    uintptr_t buf = parcel->GetData();
    auto *uBuf = new (std::nothrow) uint8_t[parcel->GetDataSize()];
    CHECK_AND_RETURN_RET_LOG(uBuf != nullptr, E_ERR, "parcel GetDataSize is null");
    memcpy_s(uBuf, parcel->GetDataSize(), reinterpret_cast<uint8_t *>(buf), parcel->GetDataSize());
    std::shared_ptr<AAFwk::ChangeInfo> serverChangeInfo = std::make_shared<AAFwk::ChangeInfo>();
    serverChangeInfo->data_ = uBuf;
    serverChangeInfo->size_ = parcel->GetDataSize();
    MEDIA_INFO_LOG("ProcessNotifyDownloadProgressInfo serverChangeInfo->size_ is: %{public}zu", parcel->GetDataSize());
    for (const ObserverInfo& obsInfo : obsInfos) {
        NotificationUtils::SendDownloadProgressInfoNotification(obsInfo.observer, serverChangeInfo);
    }
    if (serverChangeInfo->data_ != nullptr) {
        delete[] static_cast<uint8_t*>(serverChangeInfo->data_);
        serverChangeInfo->data_ = nullptr;
    }
    return E_OK;
}
} // namespace Notification
} // namespace Media
} // namespace OHOS
/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MtpStoreObserver"

#include "mtp_store_observer.h"
#include <thread>
#include "common_event_manager.h"
#include "common_event_support.h"
#include "media_log.h"
#include "mtp_event.h"

namespace OHOS {
namespace Media {
namespace {
const std::string KEY_FS_UUID                 = "fsUuid";
const std::string KEY_VOLUME_STATE            = "volumeState";
std::shared_ptr<MtpStoreObserver> observer_   = nullptr;
std::shared_ptr<MtpOperationContext> context_ = nullptr;
const std::vector<std::string> events_        = {
    EventFwk::CommonEventSupport::COMMON_EVENT_VOLUME_MOUNTED,
    EventFwk::CommonEventSupport::COMMON_EVENT_VOLUME_UNMOUNTED
};
// copy from foundation/filemanagement/storage_service/interfaces/innerkits/storage_manager/native/volume_core.h
enum MtpVolumeState : int {
    UNMOUNTED = 0,
    CHECKING,
    MOUNTED,
    EJECTING,
    REMOVED,
    BADREMOVABLE
};
} // namespace

MtpStoreObserver::MtpStoreObserver(const EventFwk::CommonEventSubscribeInfo &subscriberInfo)
    : EventFwk::CommonEventSubscriber(subscriberInfo)
{
}

bool MtpStoreObserver::StartObserver()
{
    MEDIA_INFO_LOG("MtpStoreObserver StartObserver");
    CHECK_AND_RETURN_RET_LOG(observer_ == nullptr, false, "observer_ is registered");

    static EventFwk::MatchingSkills matchingSkills;
    for (const auto &event : events_) {
        matchingSkills.AddEvent(event);
    }

    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    observer_ = std::make_shared<MtpStoreObserver>(subscribeInfo);
    CHECK_AND_RETURN_RET_LOG(observer_ != nullptr, false, "observer_ is nullptr");

    bool ret = EventFwk::CommonEventManager::SubscribeCommonEvent(observer_);
    MEDIA_INFO_LOG("MtpStoreObserver StartObserver end, ret = %{public}d", ret);
    return ret;
}

bool MtpStoreObserver::StopObserver()
{
    MEDIA_INFO_LOG("MtpStoreObserver StopObserver");
    CHECK_AND_RETURN_RET_LOG(observer_ != nullptr, false, "observer_ is not registered");

    bool ret = EventFwk::CommonEventManager::UnSubscribeCommonEvent(observer_);
    observer_ = nullptr;
    context_ = nullptr;
    MEDIA_INFO_LOG("MtpStoreObserver StopObserver end, ret = %{public}d", ret);
    return ret;
}

void MtpStoreObserver::OnReceiveEvent(const EventFwk::CommonEventData &eventData)
{
    MEDIA_DEBUG_LOG("MtpStoreObserver OnReceiveEvent");
    const AAFwk::Want &want = eventData.GetWant();
    std::string action = want.GetAction();
    MEDIA_INFO_LOG("MtpStoreObserver OnReceiveEvent action = %{public}s", action.c_str());

    std::string fsUuid = want.GetStringParam(KEY_FS_UUID);
    int volumeState = want.GetIntParam(KEY_VOLUME_STATE, CHECKING);

    std::thread([&, fsUuid, volumeState] {
        CHECK_AND_RETURN_LOG(!fsUuid.empty(), "SendThread: fsUuid is empty");
        CHECK_AND_RETURN_LOG(context_ != nullptr, "SendThread: context_ is nullptr");

        std::shared_ptr<MtpEvent> eventPtr = std::make_shared<MtpEvent>(context_);
        CHECK_AND_RETURN_LOG(eventPtr != nullptr, "SendThread: eventPtr is null");

        switch (volumeState) {
            case MOUNTED:
                eventPtr->SendStoreAdded(fsUuid);
                break;
            case UNMOUNTED:
                eventPtr->SendStoreRemoved(fsUuid);
                break;
            default:
                MEDIA_ERR_LOG("SendThread: wrong state = [%{public}d]", volumeState);
                break;
        }
    }).detach();
}

void MtpStoreObserver::AttachContext(const std::shared_ptr<MtpOperationContext> &context)
{
    context_ = context;
}

} // namespace Media
} // namespace OHOS

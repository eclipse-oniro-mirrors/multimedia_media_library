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
#define MLOG_TAG "MediaMtpSubscriber"

#include "media_mtp_subscriber.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "media_log.h"
#include "media_mtp_service_manager.h"
#include "parameters.h"
#include "usb_srv_client.h"
#include "usb_srv_support.h"

namespace OHOS {
namespace Media {
const char *MTP_SERVER_DISABLE = "persist.edm.mtp_server_disable";

MediaMtpSubscriber::MediaMtpSubscriber(const EventFwk::CommonEventSubscribeInfo &subscriberInfo)
    : EventFwk::CommonEventSubscriber(subscriberInfo)
{
}

bool MediaMtpSubscriber::Subscribe(void)
{
    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_USB_STATE);
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    std::shared_ptr<MediaMtpSubscriber> subscriber = std::make_shared<MediaMtpSubscriber>(subscribeInfo);
    return EventFwk::CommonEventManager::SubscribeCommonEvent(subscriber);
}

void MediaMtpSubscriber::OnReceiveEvent(const EventFwk::CommonEventData &eventData)
{
    MEDIA_INFO_LOG("MediaMtpSubscriber OnReceiveEvent");
    const AAFwk::Want &want = eventData.GetWant();
    std::string action = want.GetAction();
    MEDIA_INFO_LOG("MediaMtpSubscriber OnReceiveEvent action = %{public}s", action.c_str());
    if (action != EventFwk::CommonEventSupport::COMMON_EVENT_USB_STATE) {
        MEDIA_INFO_LOG("MediaMtpSubscriber OnReceiveEvent action = %{public}s", action.c_str());
    }
    for (std::string k : want.GetParams().KeySet()) {
        MEDIA_INFO_LOG("MediaMtpSubscriber OnReceiveEvent key = %{public}s", k.c_str());
    }
    bool isConnected = want.GetBoolParam(std::string {USB::UsbSrvSupport::CONNECTED}, false);
    MEDIA_INFO_LOG("MediaMtpSubscriber OnReceiveEvent eventData.GetCode = %{public}d isConnected= %{public}d",
        eventData.GetCode(), isConnected);
    if (!isConnected) {
        MEDIA_INFO_LOG("MediaMtpSubscriber OnReceiveEvent USB disconnected");
        MediaMtpServiceManager::StopMtpService();
        return;
    }
    MediaMtpServiceManager::StopMtpService();
    bool isMtp = want.GetBoolParam(std::string {USB::UsbSrvSupport::FUNCTION_NAME_MTP}, false);
    if (isMtp) {
        std::string param(MTP_SERVER_DISABLE);
        bool mtpDisable = system::GetBoolParameter(param, false);
        if (mtpDisable) {
            MEDIA_INFO_LOG("MediaMtpSubscriber MTP Manager persist.edm.mtp_server_disable = true");
        } else {
            MEDIA_INFO_LOG("MediaMtpSubscriber OnReceiveEvent USB MTP connected");
            MediaMtpServiceManager::StartMtpService(MtpMode::MTP_MODE);
        }
        return;
    }
    bool isPtp = want.GetBoolParam(std::string {USB::UsbSrvSupport::FUNCTION_NAME_PTP}, false);
    if (isPtp) {
        MEDIA_INFO_LOG("MediaMtpSubscriber OnReceiveEvent USB PTP connected");
        MediaMtpServiceManager::StartMtpService(MtpMode::PTP_MODE);
        return;
    }
    MEDIA_INFO_LOG("MediaMtpSubscriber OnReceiveEvent USB NOT MTP/PTP, Only HDC");
    return;
}
} // namespace Media
} // namespace OHOS

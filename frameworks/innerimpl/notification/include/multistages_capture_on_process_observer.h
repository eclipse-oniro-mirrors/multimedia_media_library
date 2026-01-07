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

#ifndef MEDIALIBRARY_NOTIFY_MULTISTAGES_OBSERVER_H
#define MEDIALIBRARY_NOTIFY_MULTISTAGES_OBSERVER_H

#include <vector>
#include "parcel.h"

#include "camera_character_types.h"
#include "data_ability_helper.h"
#include "data_ability_observer_stub.h"
#include "datashare_helper.h"
#include "media_change_info.h"
#include "medialibrary_notify_callback_wrapper.h"
#include "medialibrary_notify_user_define_observer.h"
#include "multistages_capture_notify_info.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
using namespace Notification;
class MultistagesCaptureOnProcessObserver : public MediaOnNotifyUserDefineObserverBodyBase {
public:
    MultistagesCaptureOnProcessObserver() {}
    MultistagesCaptureOnProcessObserver(const std::string &uri, const ObserverType &observerType)
        : uri_(uri), observerType_(observerType) {}
    virtual ~MultistagesCaptureOnProcessObserver() = default;

    void OnChange(const UserDefineCallbackWrapper &callbackWrapper) override;
    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "{"
            << "\"uri\": \"" << this->uri_.c_str() << "\","
            << "\"observerType\": \"" << std::to_string(static_cast<int32_t>(this->observerType_))
            << "}";
        return ss.str();
    }

private:
    std::shared_ptr<MultistagesCaptureNotifyServerInfo> ConvertWrapperToNotifyInfo(
        const UserDefineCallbackWrapper &callbackWrapper);
    bool MatchNotifyToObserver(const MultistagesCaptureNotifyType &notifyType, const ObserverType &observerType);

public:
    std::string uri_;
    ObserverType observerType_{ObserverType::UNDEFINED};
};

} // Media
} // OHOS
#endif // MEDIALIBRARY_NOTIFY_MULTISTAGES_OBSERVER_H

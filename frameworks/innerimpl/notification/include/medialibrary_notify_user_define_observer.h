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

#ifndef MEDIALIBRARY_NOTIFY_USER_DEFINE_OBSERVER_H
#define MEDIALIBRARY_NOTIFY_USER_DEFINE_OBSERVER_H

#include <vector>
#include "parcel.h"

#include "data_ability_helper.h"
#include "data_ability_observer_stub.h"
#include "datashare_helper.h"
#include "medialibrary_notify_callback_wrapper.h"
#include "user_define_notify_info.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

class MediaOnNotifyUserDefineObserverBodyBase {
public:
    MediaOnNotifyUserDefineObserverBodyBase() {}
    virtual ~MediaOnNotifyUserDefineObserverBodyBase() = default;

    virtual void OnChange(const UserDefineCallbackWrapper &callbackWrapper) = 0;
    virtual std::string ToString() const = 0;
};

class MediaOnNotifyUserDefineObserver : public DataShare::DataShareObserver  {
public:
    MediaOnNotifyUserDefineObserver() {}
    MediaOnNotifyUserDefineObserver(const Notification::NotifyUriType &uriType,
        std::shared_ptr<MediaOnNotifyUserDefineObserverBodyBase> observerBodyBase)
        : uriType_(uriType), observerBody_(observerBodyBase) {}
    ~MediaOnNotifyUserDefineObserver() = default;

    void OnChange(const ChangeInfo &changeInfo) override;

    Notification::NotifyUriType uriType_;
    std::shared_ptr<MediaOnNotifyUserDefineObserverBodyBase> observerBody_;

private:
    std::shared_ptr<Notification::UserDefineNotifyInfo> UnmarshalUserDefineNotify(Parcel &parcel);
};

} // Media
} // OHOS
#endif // MEDIALIBRARY_NOTIFY_USER_DEFINE_OBSERVER_H

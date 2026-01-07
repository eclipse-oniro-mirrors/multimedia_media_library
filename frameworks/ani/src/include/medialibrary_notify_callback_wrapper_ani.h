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

#ifndef MEDIALIBRARY_INNERIMPL_NOTIFY_CALLBACK_WRAPPER_ANI_H
#define MEDIALIBRARY_INNERIMPL_NOTIFY_CALLBACK_WRAPPER_ANI_H

#include "media_change_info.h"
#include "user_define_notify_info.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class ClientObserverAni {
public:
    ClientObserverAni(Notification::NotifyUriType uriType, ani_ref ref)
    {
        uriType_ = uriType;
        ref_ = ref;
    }

    ~ClientObserverAni() = default;

    Notification::NotifyUriType uriType_;
    ani_ref ref_;
};

struct NewJsOnChangeCallbackWrapperAni {
    ani_env *env_;
    ani_vm *etsVm_;
    Notification::NotifyUriType observerUriType_;
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo_;
    std::shared_ptr<Notification::AssetManagerNotifyInfo> assetManagerInfo_;
    std::shared_ptr<Notification::UserDefineNotifyInfo> userDefineInfo_;
    std::vector<std::shared_ptr<ClientObserverAni>> ClientObserverAnis_;
};
} // Media
} // OHOS
#endif // MEDIALIBRARY_INNERIMPL_NOTIFY_CALLBACK_WRAPPER_H
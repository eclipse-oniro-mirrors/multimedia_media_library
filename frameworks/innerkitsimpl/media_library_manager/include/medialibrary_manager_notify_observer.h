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

#ifndef OHOS_MEDIALIBRARY_MANAGER_NOTIFY_OBSERVER_H
#define OHOS_MEDIALIBRARY_MANAGER_NOTIFY_OBSERVER_H

#include "datashare_helper.h"
#include "media_change_info.h"

namespace OHOS {
namespace Media {
class MediaLibraryManagerNotifyObserver : public DataShare::DataShareObserver {
public:
    MediaLibraryManagerNotifyObserver(Notification::NotifyUriType uriType, int32_t userId)
        : uriType_(uriType), userId_(userId) {}
    ~MediaLibraryManagerNotifyObserver() override = default;
    void OnChange(const ChangeInfo &changeInfo) override;

private:
    Notification::NotifyUriType uriType_ = Notification::NotifyUriType::INVALID;
    int32_t userId_ = -1;
};
} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIALIBRARY_MANAGER_NOTIFY_OBSERVER_H

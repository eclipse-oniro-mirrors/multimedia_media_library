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
 
#ifndef OHOS_MEDIA_NOTIFICATION_MERGING_H
#define OHOS_MEDIA_NOTIFICATION_MERGING_H
 
#include <vector>
#include <unordered_map>
#include <functional>
#include <variant>
 
#include "notify_info.h"
#include "media_notification_utils.h"

namespace OHOS {
namespace Media {
namespace Notification {
#define EXPORT __attribute__ ((visibility ("default")))
class NotificationMerging {
public:
    EXPORT NotificationMerging();
    EXPORT ~NotificationMerging();
    EXPORT static std::vector<NotifyInfo> MergeNotifyInfo(std::vector<MediaChangeInfo> changeInfos);
    EXPORT static int32_t ProcessNotifyDownloadProgressInfo(
        Notification::DownloadAssetsNotifyType downloadAssetsNotifyType, int32_t fileId = -1, int32_t percent = -1,
            int32_t autoPauseReason = 0);
private:
    static std::vector<NotifyInfo> ProcessNotifyInfos(const std::vector<MediaChangeInfo>& mediaChangeInfos);
    static std::vector<ObserverInfo> findObservers(NotifyUriType notifyUriType);
};
} // namespace Notification
} // namespace Media
} // namespace OHOS
 
#endif  // OHOS_MEDIA_NOTIFICATION_MERGING_H
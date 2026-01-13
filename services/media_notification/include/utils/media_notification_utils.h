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

#ifndef MEDIA_NOTIFICATION_MARSHALLING_UTILS_H
#define MEDIA_NOTIFICATION_MARSHALLING_UTILS_H

#include <string>
#include <list>
#include <variant>
#include <sstream>
 
#include "parcel.h"
#include "media_log.h"
#include "data_ability_observer_interface.h"
#include "media_change_info.h"
#include "accurate_common_data.h"
#include "album_change_info.h"
#include "photo_asset_change_info.h"

namespace OHOS {
namespace Media {
using namespace Notification;
#define EXPORT __attribute__ ((visibility ("default")))
class NotificationUtils {
public:
    EXPORT static int32_t SendNotification(const sptr<AAFwk::IDataAbilityObserver> &dataObserver,
        const std::shared_ptr<MediaChangeInfo> &mediaChangeInfo);
    EXPORT static std::shared_ptr<MediaChangeInfo> UnmarshalInMultiMode(Parcel &parcel);
    EXPORT static int32_t SendDownloadProgressInfoNotification(const sptr<AAFwk::IDataAbilityObserver> &dataObserver,
        const std::shared_ptr<AAFwk::ChangeInfo> &changeInfo);
    EXPORT static std::shared_ptr<AssetManagerNotifyInfo> UnmarshalAssetManagerNotify(Parcel &parcel);
    EXPORT static void UpdateNotificationProp();

    EXPORT static int32_t SendUserDefineNotification(const sptr<AAFwk::IDataAbilityObserver> &dataObserver,
        const std::shared_ptr<AAFwk::ChangeInfo> &changeInfo);

private:
    EXPORT bool WriteToChangeInfo(const std::shared_ptr<MediaChangeInfo> &mediaChangeInfo,
        std::vector<std::shared_ptr<AAFwk::ChangeInfo>> &changeInfos);
    EXPORT bool Marshalling(const std::shared_ptr<MediaChangeInfo> &mediaChangeInfo,
        std::vector<std::shared_ptr<Parcel>> &parcels);
};

}
}
#endif // MEDIA_NOTIFICATION_MARSHALLING_UTILS_H
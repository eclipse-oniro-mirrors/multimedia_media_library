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

#define MLOG_TAG "MediaLibraryManagerNotifyObserver"

#include "medialibrary_manager_notify_observer.h"

#include <cstdlib>
#include <memory>

#include <securec.h>

#include "media_log.h"
#include "medialibrary_manager_notify_observer_manager.h"
#include "message_parcel.h"

namespace OHOS {
namespace Media {
namespace {
constexpr uint32_t MAX_PARCEL_SIZE = 200 * 1024;
}

void MediaLibraryManagerNotifyObserver::OnChange(const ChangeInfo &changeInfo)
{
    MEDIA_DEBUG_LOG("MediaLibraryManagerNotifyObserver OnChange, uriType: %{public}d", static_cast<int32_t>(uriType_));
    CHECK_AND_RETURN_LOG(changeInfo.data_ != nullptr && changeInfo.size_ > 0,
        "changeInfo data is null or size is invalid");
    CHECK_AND_RETURN_LOG(changeInfo.size_ <= MAX_PARCEL_SIZE, "changeInfo parcel size exceeds limit");

    uint8_t *parcelData = static_cast<uint8_t *>(std::malloc(changeInfo.size_));
    CHECK_AND_RETURN_LOG(parcelData != nullptr, "parcelData malloc failed");
    if (memcpy_s(parcelData, changeInfo.size_, changeInfo.data_, changeInfo.size_) != EOK) {
        MEDIA_ERR_LOG("copy parcel data failed");
        std::free(parcelData);
        return;
    }

    auto parcel = std::make_shared<MessageParcel>();
    // ParseFrom 成功后，parcelData 的所有权转移给 MessageParcel，并在其析构时释放。
    if (!parcel->ParseFrom(reinterpret_cast<uintptr_t>(parcelData), changeInfo.size_)) {
        MEDIA_ERR_LOG("parse parcel data failed");
        std::free(parcelData);
        return;
    }

    auto mediaChangeInfo = std::make_shared<Notification::MediaChangeInfo>();
    CHECK_AND_RETURN_LOG(mediaChangeInfo->ReadFromParcelInMultiMode(*parcel), "unmarshal mediaChangeInfo failed");
    MediaLibraryManagerNotifyObserverManager::GetInstance().NotifyChange(mediaChangeInfo);
}
} // namespace Media
} // namespace OHOS
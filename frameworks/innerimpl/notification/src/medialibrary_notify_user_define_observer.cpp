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

#define MLOG_TAG "MediaOnNotifyMultistagesObserver"
#include "medialibrary_notify_user_define_observer.h"

#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_tracer.h"
#include "multistages_capture_on_process_observer.h"

using namespace std;

namespace OHOS {
namespace Media {
static const uint32_t MAX_PARCEL_SIZE = 200 * 1024;

std::shared_ptr<UserDefineNotifyInfo> MediaOnNotifyUserDefineObserver::UnmarshalUserDefineNotify(Parcel &parcel)
{
    UserDefineNotifyInfo* info = new (std::nothrow)UserDefineNotifyInfo();
    if (info == nullptr) {
        return nullptr;
    }
    if (!info->ReadHeadFromParcel(parcel)) {
        delete info;
        info = nullptr;
        return nullptr;
    }
    if (!info->ReadBodyFromParcel(parcel)) {
        delete info;
        info = nullptr;
        return nullptr;
    }
    return std::shared_ptr<UserDefineNotifyInfo>(info);
}

void MediaOnNotifyUserDefineObserver::OnChange(const ChangeInfo &changeInfo)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaOnNotifyUserDefineObserver::OnChange");
    if (changeInfo.data_ == nullptr || changeInfo.size_ <= 0) {
        MEDIA_ERR_LOG("changeInfo.data_ is null or changeInfo.size_ is invalid");
        return;
    }
    CHECK_AND_RETURN_LOG(changeInfo.size_ < MAX_PARCEL_SIZE, "The size of the parcel exceeds the limit.");
    uint8_t *parcelData = static_cast<uint8_t *>(malloc(changeInfo.size_));
    CHECK_AND_RETURN_LOG(parcelData != nullptr, "parcelData malloc failed");
    if (memcpy_s(parcelData, changeInfo.size_, changeInfo.data_, changeInfo.size_) != 0) {
        MEDIA_ERR_LOG("parcelData copy parcel data failed");
        free(parcelData);
        return;
    }
    shared_ptr<MessageParcel> parcel = make_shared<MessageParcel>();
    // parcel析构函数中会free掉parcelData，成功调用ParseFrom后不可进行free(parcelData)
    if (!parcel->ParseFrom(reinterpret_cast<uintptr_t>(parcelData), changeInfo.size_)) {
        MEDIA_ERR_LOG("Parse parcelData failed");
        free(parcelData);
        return;
    }
    UserDefineCallbackWrapper callbackWrapper;
    // 解析通知内容
    callbackWrapper.userDefineInfo_ = UnmarshalUserDefineNotify(*parcel);
    CHECK_AND_RETURN_LOG(callbackWrapper.userDefineInfo_ != nullptr, "invalid userDefinelnfo");
    MEDIA_INFO_LOG("UserDefinelnfo is: %{public}s", callbackWrapper.userDefineInfo_->ToString().c_str());

    Notification::NotifyUriType infoUriType = callbackWrapper.userDefineInfo_->notifyUri_;
    callbackWrapper.observerUriType_ = infoUriType;

    if (observerBody_ == nullptr) {
        MEDIA_ERR_LOG("observerBody is nullptr.");
        return;
    }
    std::thread(
        [this, callbackWrapper]() {
            observerBody_->OnChange(callbackWrapper);
        }
    ).detach();
}
}  // namespace Media
}  // namespace OHOS
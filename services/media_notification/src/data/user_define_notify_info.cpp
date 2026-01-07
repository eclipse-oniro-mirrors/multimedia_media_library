/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
 
#include "user_define_notify_info.h"
 
#include <map>
#include <string>

#include "low_quality_memory_num_notify_info.h"
#include "multistages_capture_notify_info.h"

namespace OHOS {
namespace Media {
namespace Notification {
// This notification is only for interaction between client and server of media library,
// without applicability of permission verification.
const std::map<NotifyForUserDefineType, std::shared_ptr<UserDefineNotifyBase>> USER_DEFINE_NOTIFY_BODY_MAP = {
    { NotifyForUserDefineType::MULTISTAGES_CAPTURE, std::make_shared<MultistagesCaptureNotifyServerInfo>() },
    { NotifyForUserDefineType::LOW_QUALITY_MEMORY, std::make_shared<LowQualityMemoryNumNotifyInfo>() },
};

bool UserDefineNotifyInfo::ReadHeadFromParcel(Parcel &parcel)
{
    MEDIA_INFO_LOG("ReadHeadFromParcel begin");
    this->notifyUri_ = static_cast<NotifyUriType>(parcel.ReadUint16());
    if (this->notifyUri_ != NotifyUriType::USER_DEFINE_NOTIFY_URI) {
        MEDIA_ERR_LOG("NotityInfo type is invalid, notifyUri: %{public}d.", static_cast<int32_t>(this->notifyUri_));
        return false;
    }
    this->notifyUserDefineType_ = static_cast<NotifyForUserDefineType>(parcel.ReadUint16());
    if (this->notifyUserDefineType_ == NotifyForUserDefineType::UNDEFINED) {
        MEDIA_ERR_LOG("notifyUserDefineType is invalid.");
        return false;
    }
    MEDIA_INFO_LOG("ReadHeadFromParcel, notifyUri: %{public}d, NotifyUserDefineType: %{public}d.",
        static_cast<int32_t>(this->notifyUri_), static_cast<int32_t>(this->notifyUserDefineType_));
    return true;
}

bool UserDefineNotifyInfo::WriteHeadFromParcel(std::shared_ptr<Parcel> &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(parcel != nullptr, false, "notifyBody or parcel is nullptr.");
    if (this->notifyUri_ != NotifyUriType::USER_DEFINE_NOTIFY_URI ||
        this->notifyUserDefineType_ == NotifyForUserDefineType::UNDEFINED) {
        MEDIA_ERR_LOG("NotityInfo type is invalid.");
        return false;
    }
    parcel->WriteUint16(static_cast<uint16_t>(this->notifyUri_));
    parcel->WriteUint16(static_cast<uint16_t>(this->notifyUserDefineType_));
    return true;
}

bool UserDefineNotifyInfo::ReadBodyFromParcel(Parcel &parcel)
{
    MEDIA_INFO_LOG("ReadBodyFromParcel begin");
    auto notifyBodyIter = USER_DEFINE_NOTIFY_BODY_MAP.find(this->notifyUserDefineType_);
    if (notifyBodyIter == USER_DEFINE_NOTIFY_BODY_MAP.end()) {
        MEDIA_ERR_LOG("NotifyForUserDefineType is invalid.");
        return false;
    }
    this->notifyBody_ = notifyBodyIter->second;
    CHECK_AND_RETURN_RET_LOG(this->notifyBody_ != nullptr, false, "notifyBody is nullptr.");
 
    this->readOnly_ = parcel.ReadBool();
    if (!this->readOnly_) {
        MEDIA_ERR_LOG("Failed to ReadBodyFromParcel.");
        return false;
    }
    this->notifyBody_->UnMarshalling(parcel);
    return true;
}

bool UserDefineNotifyInfo::WriteBodyFromParcel(std::shared_ptr<Parcel> &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(notifyBody_ != nullptr && parcel != nullptr, false, "notifyBody or parcel is nullptr.");
    parcel->WriteBool(this->readOnly_);
    return this->notifyBody_->WriteToParcel(parcel);
}

bool UserDefineNotifyInfo::SetUserDefineNotifyBody(const std::shared_ptr<UserDefineNotifyBase> &notifyBody)
{
    if (this->readOnly_) {
        MEDIA_ERR_LOG("NotifyBody not support to write twice!");
        return false;
    }
    if (notifyBody == nullptr) {
        MEDIA_ERR_LOG("NotifyBody is empty!");
        return false;
    }
    this->notifyBody_ = move(notifyBody);
    this->readOnly_ = true;
    return true;
}

std::shared_ptr<UserDefineNotifyBase> UserDefineNotifyInfo::GetUserDefineNotifyBody() const
{
    if (!this->readOnly_) {
        MEDIA_ERR_LOG("NotifyBody not exit!");
        return nullptr;
    }
    return this->notifyBody_;
}

std::string UserDefineNotifyInfo::ToString() const
{
    std::stringstream ss;
    ss << "{"
        << "\"notifyUri\": \"" << std::to_string(static_cast<int32_t>(this->notifyUri_)) << "\","
        << "\"NotifyForUserDefineType\": \""
        << std::to_string(static_cast<int32_t>(this->notifyUserDefineType_)) << "\","
        << "\"readOnly\": \"" << std::to_string(static_cast<int32_t>(this->readOnly_)) << "\","
        << "\"notifyBody\": \"" << (notifyBody_ ? notifyBody_->ToString().c_str() : "nullptr")
        << "}";
    return ss.str();
}
} // namespace Notification
} // namespace Media
} // namespace OHOS
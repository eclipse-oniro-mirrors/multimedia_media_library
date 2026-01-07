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

#define MLOG_TAG "MediaLowQualityMemoryCallback"

#include "media_low_quality_memory_callback.h"

#include "media_change_info.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "multistages_capture_low_quality_memory_num_observer.h"

namespace OHOS {
namespace Media {
const std::string USER_CLIENT_CHANGE = "userDefineChange";

static std::shared_ptr<MediaOnNotifyUserDefineObserver> observer_ = nullptr;
static LowQualityMemoryNumHandler handler_ = nullptr;

MediaLowQualityMemoryCallback::~MediaLowQualityMemoryCallback()
{
    observer_ = nullptr;
    handler_ = nullptr;
}

int32_t MediaLowQualityMemoryCallback::RegisterPhotoStateCallback(
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper, const LowQualityMemoryNumHandler &func)
{
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr && func != nullptr, E_ERR,
        "Failed to RegisterPhotoStateCallback, func or dataShareHelper is nullptr.");
 
    if (observer_ == nullptr) {
        RegisterLowQualityMemoryNumObserver(dataShareHelper);
    }
 
    if (handler_ != nullptr) {
        MEDIA_WARN_LOG("g_handler is already exists and needs to be replaced.");
    }
    handler_ = func;
    MEDIA_INFO_LOG("RegisterPhotoStateCallback end.");
    return E_OK;
}

int32_t MediaLowQualityMemoryCallback::RegisterLowQualityMemoryNumObserver(
    std::shared_ptr<DataShare::DataShareHelper> &dataShareHelper)
{
    auto observerBodyBase = std::make_shared<LowQualityMemoryNumObserver>();
    auto dataObserver = std::make_shared<MediaOnNotifyUserDefineObserver>(
        NotifyUriType::USER_DEFINE_NOTIFY_URI, observerBodyBase);

    int32_t ret = dataShareHelper->RegisterObserverExtProvider(Uri(USER_CLIENT_CHANGE),
        static_cast<std::shared_ptr<DataShare::DataShareObserver>>(dataObserver), false);
 
    observer_ = dataObserver;
    return ret;
}

int32_t MediaLowQualityMemoryCallback::UnregisterPhotoStateCallback(
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper)
{
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, E_ERR,
        "Failed to UnregisterPhotoStateCallback, dataShareHelper is nullptr.");

    if (observer_ != nullptr) {
        UnregisterLowQualityMemoryNumObserver(dataShareHelper);
    }
 
    if (handler_ != nullptr) {
        handler_ = nullptr;
    }
    return E_OK;
}

int32_t MediaLowQualityMemoryCallback::UnregisterLowQualityMemoryNumObserver(
    std::shared_ptr<DataShare::DataShareHelper> &dataShareHelper)
{
    int32_t ret = dataShareHelper->UnregisterObserverExtProvider(Uri(USER_CLIENT_CHANGE),
        static_cast<std::shared_ptr<DataShare::DataShareObserver>>(observer_));
    if (ret == E_OK) {
        MEDIA_INFO_LOG("UnregisterObserverExtProvider success");
        observer_ = nullptr;
    }
    return E_OK;
}

std::shared_ptr<LowQualityMemoryNumNotifyInfo> LowQualityMemoryNumObserver::ConvertWrapperToNotifyInfo(
    const UserDefineCallbackWrapper &wrapper)
{
    if (wrapper.userDefineInfo_ == nullptr ||
        wrapper.userDefineInfo_->notifyUserDefineType_ != NotifyForUserDefineType::LOW_QUALITY_MEMORY) {
        MEDIA_WARN_LOG("Wrapper is invalid.");
        return nullptr;
    }
 
    auto notifyBody = wrapper.userDefineInfo_->GetUserDefineNotifyBody();
    if (notifyBody == nullptr) {
        MEDIA_ERR_LOG("NotifyBody is nullptr.");
        return nullptr;
    }
 
    auto notifyInfo = static_pointer_cast<LowQualityMemoryNumNotifyInfo>(notifyBody);
    if (notifyInfo == nullptr) {
        MEDIA_ERR_LOG("notifyInfo is nullptr.");
        return nullptr;
    }
    MEDIA_INFO_LOG("LowQualityMemoryNumObserver count: %{public}d.", notifyInfo->count_);
    return notifyInfo;
}

void LowQualityMemoryNumObserver::OnChange(const UserDefineCallbackWrapper &wrapper)
{
    MEDIA_INFO_LOG("MultistagesCapture, OnChange called, %{public}s.", ToString().c_str());
    std::shared_ptr<LowQualityMemoryNumNotifyInfo> notifyInfo = ConvertWrapperToNotifyInfo(wrapper);
    if (notifyInfo == nullptr) {
        return;
    }

    if (handler_ == nullptr) {
        MEDIA_ERR_LOG("g_handler is nullptr, no need to callback.");
        return;
    }
    handler_(notifyInfo->count_);
}
} // namespace Media
} // namespace OHOS
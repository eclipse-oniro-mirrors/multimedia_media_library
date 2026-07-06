/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef PROGRESS_CALLBACK_OBSERVER_H
#define PROGRESS_CALLBACK_OBSERVER_H

#include <string>
#include <sstream>
#include "parcel.h"
#include "media_log.h"
#include <securec.h>

#include "data_ability_helper.h"
#include "data_ability_observer_stub.h"
#include "datashare_helper.h"

namespace OHOS {
namespace Media {
using ChangeType = AAFwk::ChangeInfo::ChangeType;
static const uint32_t MAX_PARCEL_SIZE = 200 * 1024;

struct ProgressData {
    int64_t processed;
    int64_t remain;
};

struct ResultInfo {
    int32_t code;
    std::vector<std::string> result;
};

class ProgressCallbackObserver : public DataShare::DataShareObserver {
public:
    explicit ProgressCallbackObserver(const int32_t &requestId,
        napi_threadsafe_function onSizeProgress, napi_threadsafe_function onCountProgress)
        : onSizeProgress_(onSizeProgress), onCountProgress_(onCountProgress) {}

    void OnChange(const ChangeInfo &changeInfo) override
    {
        if (changeInfo.data_ == nullptr || changeInfo.size_ <= 0) {
            NAPI_ERR_LOG("changeInfo.data_ is null or changeInfo.size_ is invalid");
            return;
        }
        CHECK_AND_RETURN_LOG(changeInfo.size_ < MAX_PARCEL_SIZE, "The size of the parcel exceeds the limit.");
        uint8_t *parcelData = static_cast<uint8_t *>(malloc(changeInfo.size_));
        CHECK_AND_RETURN_LOG(parcelData != nullptr, "parcelData malloc failed");
        if (memcpy_s(parcelData, changeInfo.size_, changeInfo.data_, changeInfo.size_) != 0) {
            NAPI_ERR_LOG("parcelData copy parcel data failed");
            free(parcelData);
            return;
        }
        shared_ptr<MessageParcel> parcel = make_shared<MessageParcel>();
        if (!parcel->ParseFrom(reinterpret_cast<uintptr_t>(parcelData), changeInfo.size_)) {
            NAPI_ERR_LOG("Parse parcelData failed");
            free(parcelData);
            return;
        }
        std::shared_ptr<Notification::MediaProgressChangeInfo> progresschangeInfo =
            Notification::MediaProgressChangeInfo::Unmarshalling(*parcel);
        CHECK_AND_RETURN_LOG(progresschangeInfo != nullptr, "progresschangeInfo is null");
        if (onSizeProgress_ != nullptr) {
            auto *sizeProgressData =
                new ProgressData{progresschangeInfo->realTimeprocessSize, progresschangeInfo->remainSize};
            napi_call_threadsafe_function(onSizeProgress_, sizeProgressData, napi_tsfn_nonblocking);
        }

        if (onCountProgress_ != nullptr) {
            auto *countProgressData =
                new ProgressData{progresschangeInfo->processedCount, progresschangeInfo->remainCount};
            napi_call_threadsafe_function(onCountProgress_, countProgressData, napi_tsfn_nonblocking);
        }
    }

private:
    napi_threadsafe_function onSizeProgress_;
    napi_threadsafe_function onCountProgress_;
};
}
}
#endif
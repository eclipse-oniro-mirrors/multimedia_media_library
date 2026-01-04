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

#define MLOG_TAG "MultiStagesCaptureDfxCaptureFault"

#include "multistages_capture_dfx_capture_fault.h"

#include "media_log.h"
#include "post_event_utils.h"

namespace OHOS {
namespace Media {
MultiStagesCaptureDfxCaptureFault::MultiStagesCaptureDfxCaptureFault() {}
MultiStagesCaptureDfxCaptureFault::~MultiStagesCaptureDfxCaptureFault() {}

void MultiStagesCaptureDfxCaptureFault::Report(const std::string &photoId, const int32_t mediaType,
    const CaptureFaultType faultType, const std::string &faultReason)
{
    MEDIA_INFO_LOG("Report photo: %{public}s, faultReason: %{public}s", photoId.c_str(), faultReason.c_str());
    if (photoId.empty()) {
        MEDIA_ERR_LOG("photoId is empty");
        return;
    }
    VariantMap map = {{KEY_PHOTO_ID, photoId}, {KEY_MEDIA_SUBTYPE, mediaType},
        {KEY_FAULT_TYPE, static_cast<int>(faultType)}, {KEY_FAULT_REASON, faultReason}}
    PostEventUtils::GetInstance().PostStatProcess(StatType::MSC_CAPTURE_FAULT_STAT, map);
}
}
}
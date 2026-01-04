/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MultiStagesCaptureDfxResult"

#include "multistages_capture_dfx_result.h"

#include "media_log.h"
#include "post_event_utils.h"

namespace OHOS {
namespace Media {
MultiStagesCaptureDfxResult::MultiStagesCaptureDfxResult() {}

MultiStagesCaptureDfxResult::~MultiStagesCaptureDfxResult() {}

void MultiStagesCaptureDfxResult::Report(const std::string &photoId, const int32_t result, const int32_t mediaType)
{
    MEDIA_INFO_LOG("Report photo: %{public}s, result: %{public}d", photoId.c_str(), result);
    if (photoId.empty()) {
        MEDIA_ERR_LOG("photo id is empty");
        return;
    }
    VariantMap map = {{KEY_PHOTO_ID, photoId}, {KEY_RESULT, result}, {KEY_MEDIA_TYPE, mediaType}};
    PostEventUtils::GetInstance().PostStatProcess(StatType::MSC_RESULT_STAT, map);
}

} // namespace Media
} // namespace OHOS
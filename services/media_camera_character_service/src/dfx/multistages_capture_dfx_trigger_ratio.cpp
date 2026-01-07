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

#define MLOG_TAG "MultiStagesCaptureDfxTriggerRatio"

#include "multistages_capture_dfx_trigger_ratio.h"

#include <cstdlib>

#include "media_file_utils.h"
#include "media_log.h"
#include "post_event_utils.h"

namespace OHOS {
namespace Media {
const int64_t REPORT_TIME_INTERVAL = 24 * 60 * 60 * 1000L; // 24 hours

MultiStagesCaptureDfxTriggerRatio::MultiStagesCaptureDfxTriggerRatio()
    : thirdPartCount_(0), autoCount_(0), lastReportTime_(0), isReporting_(false) {}

MultiStagesCaptureDfxTriggerRatio::~MultiStagesCaptureDfxTriggerRatio() {}

MultiStagesCaptureDfxTriggerRatio& MultiStagesCaptureDfxTriggerRatio::GetInstance()
{
    static MultiStagesCaptureDfxTriggerRatio instance;
    return instance;
}

void MultiStagesCaptureDfxTriggerRatio::SetTrigger(const MultiStagesCaptureTriggerType &type)
{
    if (type == MultiStagesCaptureTriggerType::AUTO) {
        autoCount_++;
    } else {
        thirdPartCount_++;
    }

    if (ShouldReport()) {
        Report();
    }
}

bool MultiStagesCaptureDfxTriggerRatio::ShouldReport()
{
    std::lock_guard<std::mutex> lock(shouldReportMutex_);
    if (isReporting_) {
        return false;
    }

    int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
    if ((currentTime - lastReportTime_) < REPORT_TIME_INTERVAL) {
        return false;
    }

    isReporting_ = true;
    return true;
}

void MultiStagesCaptureDfxTriggerRatio::Report()
{
    MEDIA_INFO_LOG("Report thirdPartCount_ = %{public}d, autoCount_ = %{public}d", thirdPartCount_, autoCount_);
    if (thirdPartCount_ == 0 && autoCount_ == 0) {
        return;
    }

    VariantMap map = {{KEY_THIRD_PART_COUNT, thirdPartCount_}, {KEY_AUTO_COUNT, autoCount_}};
    PostEventUtils::GetInstance().PostStatProcess(StatType::MSC_TRIGGER_RATIO_STAT, map);

    thirdPartCount_ = 0;
    autoCount_ = 0;
    lastReportTime_ = MediaFileUtils::UTCTimeMilliSeconds();
    {
        std::lock_guard<std::mutex> lock(shouldReportMutex_);
        isReporting_ = false;
    }
}

} // namespace Media
} // namespace OHOS
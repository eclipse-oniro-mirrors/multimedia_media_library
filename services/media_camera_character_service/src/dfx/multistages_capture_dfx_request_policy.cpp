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

#define MLOG_TAG "MultiStagesCaptureDfxRequestPolicy"

#include "multistages_capture_dfx_request_policy.h"

#include <cstdlib>

#include "media_file_utils.h"
#include "media_log.h"
#include "post_event_utils.h"

namespace OHOS {
namespace Media {
const int64_t REPORT_TIME_INTERVAL = 24 * 60 * 60 * 1000L; // 24 hour in milliseconds

MultiStagesCaptureDfxRequestPolicy::MultiStagesCaptureDfxRequestPolicy() {}

MultiStagesCaptureDfxRequestPolicy::~MultiStagesCaptureDfxRequestPolicy() {}

MultiStagesCaptureDfxRequestPolicy& MultiStagesCaptureDfxRequestPolicy::GetInstance()
{
    static MultiStagesCaptureDfxRequestPolicy instance;
    return instance;
}

void MultiStagesCaptureDfxRequestPolicy::GetCount(const RequestPolicy policy, RequestCount &count)
{
    switch (policy) {
        case RequestPolicy::HIGH_QUALITY_MODE:
            count.highQualityCount += 1;
            break;
        case RequestPolicy::BALANCE_MODE:
            count.balanceQualityCount += 1;
            break;
        case RequestPolicy::FAST_MODE:
            count.emergencyQualityCount += 1;
            break;
        default:
            break;
    }
}

void MultiStagesCaptureDfxRequestPolicy::SetPolicy(const std::string &callingPackage, const RequestPolicy policy)
{
    RequestCount requestCountForEachCaller { 0, 0, 0 };
    if (requestCountMap_.Find(callingPackage, requestCountForEachCaller)) {
        GetCount(policy, requestCountForEachCaller);
        requestCountMap_.EnsureInsert(callingPackage, requestCountForEachCaller);
    } else {
        GetCount(policy, requestCountForEachCaller);
        requestCountMap_.Insert(callingPackage, requestCountForEachCaller);
    }

    if (ShouldReport()) {
        Report();
    }
}

bool MultiStagesCaptureDfxRequestPolicy::ShouldReport()
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

void MultiStagesCaptureDfxRequestPolicy::Report()
{
    if (requestCountMap_.IsEmpty()) {
        return;
    }

    requestCountMap_.Iterate([](std::string key, RequestCount &val) {
        VariantMap map = {{ KEY_CALLING_PACKAGE, key },
            { KEY_HIGH_QUALITY_COUNT, val.highQualityCount },
            { KEY_BALANCE_QUALITY_COUNT, val.balanceQualityCount },
            { KEY_EMERGENCY_QUALITY_COUNT, val.emergencyQualityCount }};

        MEDIA_INFO_LOG("Report caller:%{public}s, high: %{public}d, balance: %{public}d, emergency: %{public}d",
            key.c_str(), val.highQualityCount, val.balanceQualityCount,
            val.emergencyQualityCount);
        PostEventUtils::GetInstance().PostStatProcess(StatType::MSC_REQUEST_POLICY_STAT, map);
    });

    requestCountMap_.Clear();
    lastReportTime_ = MediaFileUtils::UTCTimeMilliSeconds();
    isReporting_ = false;
}

} // namespace Media
} // namespace OHOS
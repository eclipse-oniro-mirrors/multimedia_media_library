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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTISTAGES_CAPTURE_DFX_REQUEST_POLICY_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTISTAGES_CAPTURE_DFX_REQUEST_POLICY_H

#include <mutex>
#include <safe_map.h>
#include <string>
#include <thread>

#include "request_policy.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
struct RequestCount {
    int32_t highQualityCount;
    int32_t balanceQualityCount;
    int32_t emergencyQualityCount;
};

class MultiStagesCaptureDfxRequestPolicy {
public:
    EXPORT static MultiStagesCaptureDfxRequestPolicy &GetInstance();
    EXPORT void SetPolicy(const std::string &callingPackage, const RequestPolicy policy);

private:
    MultiStagesCaptureDfxRequestPolicy();
    ~MultiStagesCaptureDfxRequestPolicy();

    MultiStagesCaptureDfxRequestPolicy(const MultiStagesCaptureDfxRequestPolicy &policy) = delete;
    const MultiStagesCaptureDfxRequestPolicy &operator=(const MultiStagesCaptureDfxRequestPolicy &policy) = delete;

    EXPORT void GetCount(const RequestPolicy policy, RequestCount &count);
    bool ShouldReport();
    void Report();

    int64_t lastReportTime_ {0};
    volatile bool isReporting_ {false};
    std::mutex shouldReportMutex_;
    SafeMap<std::string, RequestCount> requestCountMap_;
};

} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTISTAGES_CAPTURE_DFX_REQUEST_POLICY_H
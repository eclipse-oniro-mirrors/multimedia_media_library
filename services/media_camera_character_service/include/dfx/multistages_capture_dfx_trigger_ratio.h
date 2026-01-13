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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTISTAGES_CAPTURE_DFX_TRIGGER_RATIO_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTISTAGES_CAPTURE_DFX_TRIGGER_RATIO_H

#include <mutex>
#include <string>
#include <thread>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

enum class MultiStagesCaptureTriggerType : int32_t {
    THIRD_PART,
    AUTO,
};

class MultiStagesCaptureDfxTriggerRatio {
public:
    EXPORT static MultiStagesCaptureDfxTriggerRatio& GetInstance();
    EXPORT void SetTrigger(const MultiStagesCaptureTriggerType &type);

private:
    MultiStagesCaptureDfxTriggerRatio();
    ~MultiStagesCaptureDfxTriggerRatio();

    MultiStagesCaptureDfxTriggerRatio(const MultiStagesCaptureDfxTriggerRatio &totalTime) = delete;
    const MultiStagesCaptureDfxTriggerRatio &operator=(const MultiStagesCaptureDfxTriggerRatio &totalTime) = delete;

    bool ShouldReport();
    void Report();

    int32_t thirdPartCount_;
    int32_t autoCount_;
    int64_t lastReportTime_;
    volatile bool isReporting_;
    std::mutex shouldReportMutex_;
};

} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTISTAGES_CAPTURE_DFX_TRIGGER_RATIO_H
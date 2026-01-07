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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTISTAGES_CAPTURE_DFX_TOTAL_TIME_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTISTAGES_CAPTURE_DFX_TOTAL_TIME_H

#include <string>
#include <mutex>
#include <unordered_map>
#include <utility>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class MultiStagesCaptureDfxTotalTime {
public:
    EXPORT static MultiStagesCaptureDfxTotalTime& GetInstance();

    EXPORT void AddStartTime(const std::string &photoId);
    EXPORT void RemoveStartTime(const std::string &photoId);
    EXPORT void Report(const std::string &photoId, const int32_t mediaType);

private:
    MultiStagesCaptureDfxTotalTime();
    ~MultiStagesCaptureDfxTotalTime();
    MultiStagesCaptureDfxTotalTime(const MultiStagesCaptureDfxTotalTime &totalTime) = delete;
    const MultiStagesCaptureDfxTotalTime &operator=(const MultiStagesCaptureDfxTotalTime &totalTime) = delete;

    std::unordered_map<std::string, int64_t> startTimes_;
    static std::mutex mutex_;
};

} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTISTAGES_CAPTURE_DFX_TOTAL_TIME_H
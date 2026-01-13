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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTISTAGES_CAPTURE_DFX_FIRST_VISIT_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTISTAGES_CAPTURE_DFX_FIRST_VISIT_H

#include <memory>
#include <string>

#include "medialibrary_async_worker.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class FirstVisitAsyncTaskData : public AsyncTaskData {
public:
    FirstVisitAsyncTaskData(int32_t fileId, const std::string &photoId, int64_t startTime, int64_t visitTime)
        : fileId_(fileId), photoId_(photoId), startTime_(startTime), visitTime_(visitTime) {}
    virtual ~FirstVisitAsyncTaskData() override = default;

    int32_t fileId_;
    std::string photoId_;
    int64_t startTime_;
    int64_t visitTime_;
};

class MultiStagesCaptureDfxFirstVisit {
public:
    EXPORT static MultiStagesCaptureDfxFirstVisit& GetInstance();
    EXPORT void Report(const std::string &photoId, const int32_t fileId);

private:
    MultiStagesCaptureDfxFirstVisit();
    virtual ~MultiStagesCaptureDfxFirstVisit();
    MultiStagesCaptureDfxFirstVisit(const MultiStagesCaptureDfxFirstVisit &totalTime) = delete;
    const MultiStagesCaptureDfxFirstVisit &operator=(const MultiStagesCaptureDfxFirstVisit &totalTime) = delete;
};

} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTISTAGES_CAPTURE_DFX_FIRST_VISIT_H
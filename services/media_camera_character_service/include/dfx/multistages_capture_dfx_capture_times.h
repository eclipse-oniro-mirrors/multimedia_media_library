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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTISTAGES_CAPTURE_DFX_CAPTURE_TIMES_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTISTAGES_CAPTURE_DFX_CAPTURE_TIMES_H

#include <string>
#include <mutex>
#include "media_file_utils.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
enum class CaptureMessageType : int32_t {
    CREATE_ASSET,
    CREATE_ASSET_DB_ERROR,
    SAVE_ASSET,
    CAPTURE_IMAGE_TIMES_SUCCESS,
    CAPTURE_VIDEO_TIMES,
    CAPTURE_VIDEO_TIMES_SUCCESS,
};

class MultiStagesCaptureDfxCaptureTimes {
public:
    EXPORT static MultiStagesCaptureDfxCaptureTimes &GetInstance();
    EXPORT void AddCaptureTimes(CaptureMessageType type);
private:
    MultiStagesCaptureDfxCaptureTimes();
    ~MultiStagesCaptureDfxCaptureTimes();
    bool ShouldReport();
    void Clear();
    void Report();
    int32_t createAssetTimes = 0;
    int32_t createAssetDbErrorTimes = 0;
    int32_t saveAssetTimes = 0;
    int32_t captureImageSuccessTimes = 0;
    int32_t captureVideoTimes = 0;
    int32_t captureVideoSuccessTimes = 0;
    int64_t lastReportTime_ {MediaFileUtils::UTCTimeMilliSeconds()};
    std::mutex captureTimeMutex_;
    volatile bool isReporting_ {false};
};
}
}
#endif
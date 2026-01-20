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

#define MLOG_TAG "MultiStagesCaptureDfxCaptureTimes"

#include "multistages_capture_dfx_capture_times.h"

#include "media_log.h"
#include "post_event_utils.h"


namespace OHOS {
namespace Media {
const int64_t REPORT_TIME_INTERVAL = 24 * 60 * 60 * 1000L;
MultiStagesCaptureDfxCaptureTimes::MultiStagesCaptureDfxCaptureTimes() {}
MultiStagesCaptureDfxCaptureTimes::~MultiStagesCaptureDfxCaptureTimes() {}

MultiStagesCaptureDfxCaptureTimes& MultiStagesCaptureDfxCaptureTimes::GetInstance()
{
    static MultiStagesCaptureDfxCaptureTimes instance;
    return instance;
}

void MultiStagesCaptureDfxCaptureTimes::AddCaptureTimes(CaptureMessageType type)
{
    std::lock_guard<std::mutex> lock(captureTimeMutex_);
    if (ShouldReport()) {
        Report();
    }
    switch (type) {
        case CaptureMessageType::CREATE_ASSET:
            createAssetTimes += 1;
            break;
        case CaptureMessageType::CREATE_ASSET_DB_ERROR:
            createAssetDbErrorTimes += 1;
            break;
        case CaptureMessageType::SAVE_ASSET:
            saveAssetTimes += 1;
            break;
        case CaptureMessageType::CAPTURE_IMAGE_TIMES_SUCCESS:
            captureImageSuccessTimes += 1;
            break;
        case CaptureMessageType::CAPTURE_VIDEO_TIMES:
            captureVideoTimes += 1;
            break;
        case CaptureMessageType::CAPTURE_VIDEO_TIMES_SUCCESS:
            captureVideoSuccessTimes +=1;
            break;
        default:
            break;
    }
}
bool MultiStagesCaptureDfxCaptureTimes::ShouldReport()
{
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

void MultiStagesCaptureDfxCaptureTimes::Clear()
{
    createAssetTimes = 0;
    createAssetDbErrorTimes = 0;
    saveAssetTimes = 0;
    captureImageSuccessTimes = 0;
    captureVideoTimes = 0;
    captureVideoSuccessTimes = 0;
}

void MultiStagesCaptureDfxCaptureTimes::Report()
{
    MEDIA_INFO_LOG("Report MultiStagesCaptureDfxCaptureTimes");
    VariantMap map = {
        {KEY_CREATE_ASSET, createAssetTimes},
        {KEY_CREATE_ASSET_DB_ERROR, createAssetDbErrorTimes},
        {KEY_SAVE_ASSET, saveAssetTimes},
        {KEY_CAPTURE_IMAGE_TIMES_SUCCESS, captureImageSuccessTimes},
        {KEY_CAPTURE_VIDEO_TIMES, captureVideoTimes},
        {KEY_CAPTURE_VIDEO_TIMES_SUCCESS, captureVideoSuccessTimes}
    };
    PostEventUtils::GetInstance().PostStatProcess(StatType::MSC_CAPTURE_TIMES, map);
    Clear();
    lastReportTime_ = MediaFileUtils::UTCTimeMilliSeconds();
    isReporting_ = false;
}
}
}
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

#define MLOG_TAG "MultiStagesCaptureDfxTotalTime"

#include "multistages_capture_dfx_total_time.h"

#include "media_file_utils.h"
#include "media_log.h"
#include "post_event_utils.h"

namespace OHOS {
namespace Media {
std::mutex MultiStagesCaptureDfxTotalTime::mutex_;

MultiStagesCaptureDfxTotalTime::MultiStagesCaptureDfxTotalTime() {}

MultiStagesCaptureDfxTotalTime::~MultiStagesCaptureDfxTotalTime() {}

MultiStagesCaptureDfxTotalTime& MultiStagesCaptureDfxTotalTime::GetInstance()
{
    static MultiStagesCaptureDfxTotalTime instance;
    return instance;
}

void MultiStagesCaptureDfxTotalTime::AddStartTime(const std::string &photoId)
{
    std::lock_guard<std::mutex> startTimeLock(mutex_);
    startTimes_.emplace(photoId, MediaFileUtils::UTCTimeMilliSeconds());
}

void MultiStagesCaptureDfxTotalTime::RemoveStartTime(const std::string &photoId)
{
    std::lock_guard<std::mutex> startTimeLock(mutex_);
    if (startTimes_.empty() || startTimes_.find(photoId) == startTimes_.end()) {
        MEDIA_INFO_LOG("RemoveStartTime startTimes_ is empty or photoId is not in startTimes_");
        return;
    }

    startTimes_.erase(photoId);
}

void MultiStagesCaptureDfxTotalTime::Report(const std::string &photoId, const int32_t mediaType)
{
    std::lock_guard<std::mutex> startTimeLock(mutex_);
    if (startTimes_.empty() || startTimes_.find(photoId) == startTimes_.end()) {
        MEDIA_INFO_LOG("startTimes_ is empty or photoId is not in startTimes_");
        return;
    }
    int64_t startTime = startTimes_[photoId];
    int64_t totalTime = MediaFileUtils::UTCTimeMilliSeconds() - startTime;
    startTimes_.erase(photoId);
    VariantMap map = {{KEY_PHOTO_ID, photoId}, {KEY_TOTAL_TIME_COST, totalTime}, {KEY_MEDIA_TYPE, mediaType}};
    PostEventUtils::GetInstance().PostStatProcess(StatType::MSC_TOTAL_TIME_COST_STAT, map);
}

} // namespace Media
} // namespace OHOS
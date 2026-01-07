/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#define MLOG_TAG "CloudEnhancementDfxGetCount"

#include "cloud_enhancement_dfx_get_count.h"

#include <cstdlib>

#include "media_file_utils.h"
#include "media_log.h"
#include "post_event_utils.h"

namespace OHOS {
namespace Media {
const int64_t REPORT_TIME_INTERVAL = 24 * 60 * 60 * 1000L; // 24 hour in milliseconds

CloudEnhancementGetCount::CloudEnhancementGetCount() {}

CloudEnhancementGetCount::~CloudEnhancementGetCount() {}

CloudEnhancementGetCount& CloudEnhancementGetCount::GetInstance()
{
    static CloudEnhancementGetCount instance;
    return instance;
}

void CloudEnhancementGetCount::AddStartTime(const std::string &photoId)
{
    startTimes_.emplace(photoId, MediaFileUtils::UTCTimeMilliSeconds());
}

void CloudEnhancementGetCount::RemoveStartTime(const std::string &photoId)
{
    if (startTimes_.empty() || startTimes_.find(photoId) == startTimes_.end()) {
        MEDIA_INFO_LOG("RemoveStartTime startTimes_ is empty or photoId is not in startTimes_");
        return;
    }

    startTimes_.erase(photoId);
}

void CloudEnhancementGetCount::Report(const std::string &completedType, const std::string &photoId,
    const int32_t finishType, const int32_t fileType)
{
    if (startTimes_.empty() || startTimes_.find(photoId) == startTimes_.end()) {
        MEDIA_INFO_LOG("startTimes_ is empty or photoId is not in startTimes_");
        return;
    }
    int64_t startTime = startTimes_[photoId];
    int64_t totalTime = MediaFileUtils::UTCTimeMilliSeconds() - startTime;
    startTimes_.erase(photoId);
    VariantMap map = {
        {KEY_PHOTO_ID, photoId},
        {KEY_TOTAL_TIME_COST, totalTime},
        {KEY_CLOUD_ENHANCEMENT_COMPLETE_TYPE, completedType},
        {KEY_CLOUD_ENHANCEMENT_FINISH_TYPE, finishType},
        {KEY_CLOUD_ENHANCEMENT_FILE_TYPE, fileType}
    };
    PostEventUtils::GetInstance().PostStatProcess(StatType::CLOUD_ENHANCEMENT_GET_COUNT_STAT, map);
}

std::unordered_map<std::string, int64_t> CloudEnhancementGetCount::GetStartTimes()
{
    return startTimes_;
}
} // namespace Media
} // namespace OHOS
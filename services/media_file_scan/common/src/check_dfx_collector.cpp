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
#include "check_dfx_collector.h"

#include <sstream>

#include "hisysevent.h"
#include "media_log.h"
#include "media_time_utils.h"

namespace OHOS::Media {
static constexpr char MEDIA_LIBRARY[] = "MEDIALIBRARY";
const std::unordered_map<CheckScene, int32_t> CHECK_SCENE_TO_CHECK_TYPE_MAP = {
    {CheckScene::FILE_MANAGER, 1},
    {CheckScene::LAKE, 2},
};

CheckDfxCollector::CheckDfxCollector(CheckScene scene)
{
    scene_ = scene;
}

void CheckDfxCollector::OnCheckStart()
{
    dfxStats_.startTimeInMs = static_cast<uint64_t>(MediaTimeUtils::UTCTimeMilliSeconds());
}

void CheckDfxCollector::OnCheckStart(int64_t timeInMs, ConsistencyCheck::DfxStats dfxStats)
{
    if (dfxStats.startTimeInMs <= 0) {
        dfxStats.startTimeInMs = static_cast<uint64_t>(timeInMs);
    }
    dfxStats_ = dfxStats;
}

void CheckDfxCollector::OnCheckEnd()
{
    dfxStats_.endTimeInMs = static_cast<uint64_t>(MediaTimeUtils::UTCTimeMilliSeconds());
}

void CheckDfxCollector::OnCheckEnd(int64_t timeInMs)
{
    dfxStats_.endTimeInMs = static_cast<uint64_t>(timeInMs);
}

void CheckDfxCollector::OnPhotoAdd(int32_t delta)
{
    dfxStats_.photoAddCount += delta;
}

void CheckDfxCollector::OnPhotoUpdate(int32_t delta)
{
    dfxStats_.photoUpdateCount += delta;
}

void CheckDfxCollector::OnPhotoDelete(int32_t delta)
{
    dfxStats_.photoDeleteCount += delta;
}

void CheckDfxCollector::OnAlbumAdd(int32_t delta)
{
    dfxStats_.albumAddCount += delta;
}

void CheckDfxCollector::OnAlbumUpdate(int32_t delta)
{
    dfxStats_.albumUpdateCount += delta;
}

void CheckDfxCollector::OnAlbumDelete(int32_t delta)
{
    dfxStats_.albumDeleteCount += delta;
}

void CheckDfxCollector::Report()
{
    MEDIA_INFO_LOG("%{public}s", ToString().c_str());

    int32_t checkType = 0;
    auto iter = CHECK_SCENE_TO_CHECK_TYPE_MAP.find(scene_);
    if (iter == CHECK_SCENE_TO_CHECK_TYPE_MAP.end()) {
        MEDIA_ERR_LOG("No check type, scene: %{public}d", static_cast<int32_t>(scene_));
        return;
    }
    checkType = iter->second;

    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_ANCO_CHECK_INFO",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "CHECK_TYPE", checkType,
        "CHECK_START_TIME", dfxStats_.startTimeInMs,
        "CHECK_END_TIME", dfxStats_.endTimeInMs,
        "CHECK_ADD", dfxStats_.photoAddCount + dfxStats_.albumAddCount,
        "CHECK_UPDATE", dfxStats_.photoUpdateCount + dfxStats_.albumUpdateCount,
        "CHECK_DELETE", dfxStats_.photoDeleteCount + dfxStats_.albumDeleteCount);
    CHECK_AND_RETURN_LOG(ret == 0, "Report MEDIALIB_ANCO_CHECK_INFO error: %{public}d", ret);

    Reset();
}

void CheckDfxCollector::Reset()
{
    MEDIA_INFO_LOG("Reset to 0");
    dfxStats_ = {};
}

ConsistencyCheck::DfxStats CheckDfxCollector::GetDfxStats()
{
    return dfxStats_;
}

std::string CheckDfxCollector::ToString() const
{
    std::stringstream ss;
    ss << "scene: " << static_cast<int32_t>(scene_) << ", " << dfxStats_.ToString();
    return ss.str();
}
} // namespace OHOS::Media
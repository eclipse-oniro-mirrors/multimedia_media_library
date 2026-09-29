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

#define MLOG_TAG "Media_Background"

#include "media_reverse_clone_marker_aging_task.h"

#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_subscriber.h"
#include "power_efficiency_manager.h"
#include "reverse_clone_restore_marker.h"

namespace OHOS::Media::Background {
bool MediaReverseCloneMarkerAgingTask::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn() && PowerEfficiencyManager::IsChargingAndScreenOff();
}

void MediaReverseCloneMarkerAgingTask::Execute()
{
    CHECK_AND_RETURN_INFO_LOG(Accept(), "reverse clone marker aging condition not met");
    int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
    CHECK_AND_RETURN_INFO_LOG(ReverseCloneRestoreMarker::DeleteIfExpired(currentTime),
        "reverse clone marker is absent or not expired");
    MEDIA_INFO_LOG("reverse clone marker expired and deleted");
}
} // namespace OHOS::Media::Background

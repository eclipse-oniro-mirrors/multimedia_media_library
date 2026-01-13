/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "media_cloud_sync_backgroud_task.h"

#include "media_hidden_and_recycle_task.h"
#include "repair_video_dirty_and_quality_task.h"
#include "medialibrary_subscriber.h"
#include "media_log.h"
#include "media_location_synchronize_task.h"
#include "repair_future_date_task.h"

namespace OHOS::Media::Background {
// LCOV_EXCL_START
MediaCloudSyncBackgroundTask::MediaCloudSyncBackgroundTask()
{
    this->tasks_ = {
        std::make_shared<MediaHiddenAndRecycleTask>(),
        std::make_shared<RepairFutureDateTask>(),
        std::make_shared<MediaLocationSynchronizeTask>(),
    };
}

bool MediaCloudSyncBackgroundTask::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}

void MediaCloudSyncBackgroundTask::Execute()
{
    for (auto &task : this->tasks_) {
        CHECK_AND_CONTINUE_ERR_LOG(task != nullptr, "task is null");
        CHECK_AND_RETURN_INFO_LOG(this->Accept(), "check accept failed");
        task->Execute();
    }
    return;
}
// LCOV_EXCL_STOP
}  // namespace OHOS::Media::CloudSync
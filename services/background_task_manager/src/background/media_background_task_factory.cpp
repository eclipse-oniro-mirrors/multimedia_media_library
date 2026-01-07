/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "media_background_task_factory.h"
#ifdef MEDIALIBRARY_CLOUD_SYNC_SERVICE_SUPPORT
#include "media_cloud_sync_backgroud_task.h"
#endif
#include "media_file_manager_temp_file_aging_task.h"
#include "media_video_mode_task.h"
#include "media_deleted_file_task.h"
#include "medialibrary_subscriber.h"
#include "media_log.h"

namespace OHOS::Media::Background {
MediaBackgroundTaskFactory::MediaBackgroundTaskFactory()
{
    this->tasks_ = {
        #ifdef MEDIALIBRARY_CLOUD_SYNC_SERVICE_SUPPORT
        std::make_shared<MediaCloudSyncBackgroundTask>(),
        #endif
        std::make_shared<MediaFileManagerTempFileAgingTask>(),
        std::make_shared<MediaVideoModeTask>(),
        std::make_shared<MediaDeletedFileTask>(),
    };
}

// Check if the task can be executed.
bool MediaBackgroundTaskFactory::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}

void MediaBackgroundTaskFactory::Execute()
{
    // Only one task can be executed at the same time.
    std::unique_lock<std::mutex> taskLock(mutex_, std::defer_lock);
    CHECK_AND_RETURN_WARN_LOG(taskLock.try_lock(), "task is running");
    // Execute all tasks.
    for (auto &task : this->tasks_) {
        CHECK_AND_CONTINUE_ERR_LOG(task != nullptr, "task is null");
        CHECK_AND_RETURN_INFO_LOG(this->Accept(), "check accept failed");
        
        task->Execute();
    }
    return;
}
}  // namespace OHOS::Media::Background
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

#ifndef FRAMEWORKS_SERVICES_MEDIA_THUMBNAIL_INCLUDE_THUMBNAIL_RESTORE_MANAGER_H_
#define FRAMEWORKS_SERVICES_MEDIA_THUMBNAIL_INCLUDE_THUMBNAIL_RESTORE_MANAGER_H_

#include <atomic>
#include <memory>
#include <mutex>
#include <timer.h>

#include "media_log.h"
#include "rdb_helper.h"
#include "rdb_predicates.h"

#include "thumbnail_data.h"
#include "thumbnail_generate_worker_manager.h"

namespace OHOS {
namespace Media {

#define EXPORT __attribute__ ((visibility ("default")))

class EXPORT ThumbnailRestoreManager {
public:
    static ThumbnailRestoreManager& GetInstance();
    void OnScreenStateChanged(bool isScreenOn);
    int32_t RestoreAstcDualFrame(ThumbRdbOpt &opts,
        const int32_t &restoreAstcCount = ASTC_GENERATE_COUNT_AFTER_RESTORE);

private:
    ThumbnailRestoreManager() : progressTimer_("ThumbnailRestoreProgress") {}
    ~ThumbnailRestoreManager();

    void InitializeRestore(int64_t totalTasks);
    void AddCompletedTasks(int64_t count = 1);
    void StartProgressReporting(uint32_t reportIntervalMs);
    void StopProgressReporting();
    void ReportProgressBegin();
    void ReportProgress(bool isScreenOn);
    static void RestoreAstcDualFrameTask(std::shared_ptr<ThumbnailTaskData> &data);
    void Reset();

    mutable std::mutex progressMutex_;
    std::atomic<int64_t> readyAstc_{0};
    std::atomic<int64_t> completedTasks_{0};
    std::atomic<int64_t> totalTasks_{0};
    std::atomic<int64_t> startTime_{0};

    std::atomic<bool> lastScreenState_{false};
    std::atomic<bool> isRestoreActive_{false};

    Utils::Timer progressTimer_;
    uint32_t progressTimerId_{0};
    std::atomic<bool> isReporting_{false};
};

} // namespace Media
} // namespace OHOS

#endif // FRAMEWORKS_SERVICES_MEDIA_THUMBNAIL_INCLUDE_THUMBNAIL_RESTORE_MANAGER_H_

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

#ifndef SCAN_TASK_DEDUPLICATOR_H
#define SCAN_TASK_DEDUPLICATOR_H

#include <memory>

#include "deduplication_handler.h"
#include "scan_config.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class EXPORT ScanTaskDeduplicator {
public:
    ScanTaskDeduplicator();
    ~ScanTaskDeduplicator();

    ScanSubmitResult SubmitTask(const std::shared_ptr<ScanTaskContext>& task);
    void UnmarkAsScanning(const std::shared_ptr<ScanTaskContext>& task);
    std::shared_ptr<ScanTaskContext> GetNextWaitingTask(const std::shared_ptr<ScanTaskContext>& task);
    void WaitForSyncScanCompletion(int32_t fileId);
    void NotifySyncTaskCompleted(int32_t fileId);
    std::shared_ptr<ScanTaskContext> GetSyncPendingTask(int32_t fileId);

    void ClearAllWaitingQueues();

private:
    std::shared_ptr<DeduplicationHandler> handler_;
};

} // namespace Media
} // namespace OHOS

#endif // SCAN_TASK_DEDUPLICATOR_H
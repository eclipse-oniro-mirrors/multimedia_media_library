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
#ifndef OHOS_MEDIA_RECOVERY_SUMMARY_H
#define OHOS_MEDIA_RECOVERY_SUMMARY_H

#include <string>
#include "post_event_utils.h"

namespace OHOS::Media {
// The empty task file identifies the window; one progress file stores its state.
int SaveRecoverySummary(const std::string &taskPath, const DatabaseRecoveryReport &report);
// Remove a stale report.
int ForgetRecoverySummary(const std::string &taskPath);
bool DecodeRecoverySummary(const std::string &data, DatabaseRecoveryReport &report);
int LoadRecoverySummary(const std::string &taskPath, DatabaseRecoveryReport &report);
bool IsWaitingForDatabaseRecovery(const DatabaseRecoveryReport &report);
DatabaseRecoveryResult GetBackgroundRecoveryResult(int32_t error, int64_t failed);
void RecordForceAttempt(DatabaseRecoveryReport &report, int32_t error, int64_t elapsedMs);
// Rebind the progress to a merged window, restarting both bucket passes.
int UpdateRecoveryWindowProgress(const std::string &fromTask, const std::string &toTask);
// A usable database allows pending windows to restart with fresh bucket progress.
int PrepareRecoveryWindowsAfterRestore(const std::string &directory, int &updated);
void CleanupRecoverySummary(const std::string &taskPath);
} // namespace OHOS::Media
#endif // OHOS_MEDIA_RECOVERY_SUMMARY_H

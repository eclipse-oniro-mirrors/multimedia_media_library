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
#ifndef OHOS_MEDIA_RECOVERY_WINDOW_H
#define OHOS_MEDIA_RECOVERY_WINDOW_H

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace OHOS::Media {
constexpr char RECOVERY_WINDOW_DIRECTORY[] = "/data/storage/el2/base/preferences";

struct RecoveryWindow {
    int64_t startMs = 0;
    int64_t endMs = 0;
    std::string path;
};

struct RecoveryWindowResult {
    int error = 0;         // Zero on success; otherwise a POSIX errno.
    int cleanupError = 0;  // POSIX errno if a failed creation could not be removed.
    int attempts = 0;
    std::string path;
    int64_t startMs = 0;
    int merged = 0;
    int expired = 0;
};

// Creates an empty task file once. The filename contains the window.
RecoveryWindowResult CreateRecoveryWindowFile(const std::string &directory, int64_t startMs, int64_t endMs);
// Reuses recent pending work and removes tasks whose start is more than 72 hours old.
RecoveryWindowResult RegisterRecoveryWindowFile(const std::string &directory, int64_t startMs, int64_t endMs);
// Returns a POSIX errno if removal fails.
int RemoveRecoveryWindowFile(const std::string &path);
bool ParseRecoveryWindowName(const std::string &name, RecoveryWindow &window);
int ListRecoveryWindows(const std::string &directory, std::vector<RecoveryWindow> &windows);
int RemoveRecoveryWindow(const RecoveryWindow &window);
int IgnoreCompletedRecoveryWindow(const RecoveryWindow &window);
int RunForCurrentRecoveryWindow(const RecoveryWindow &window, const std::function<int()> &operation);
bool TryBeginDatabaseRecovery(std::atomic<bool> &restoring);
int RunWhileDatabaseAvailable(const std::function<bool()> &isRestoring, const std::function<int()> &operation);
// If the task directory cannot be read, abandon this attempt and do not block backup.
bool HasPendingRecoveryWindow(const std::string &directory);
// Task creation and ordinary metadata writes use the same lock.
int RunWithoutPendingRecoveryWindow(const std::string &directory, const std::function<int()> &operation);
bool IsInRecoveryWindow(int64_t mtimeMs, const RecoveryWindow &window);
bool IsAtOrAfterRecoveryStart(int64_t mtimeMs, const RecoveryWindow &window);
bool ShouldApplyRecoveryJson(int64_t mtimeMs, int64_t databaseMtimeMs, const RecoveryWindow &window);
bool ShouldScanRecoveryFile(int64_t mtimeMs, int64_t size, int64_t databaseMtimeMs, int64_t databaseSize,
    const RecoveryWindow &window);

struct ForceRecoveryResult {
    RecoveryWindowResult window;
    bool windowRequired = false;
    bool forceCalled = false;
    int32_t forceError = 0;
};

// The callbacks keep the policy testable without opening a real media database.
ForceRecoveryResult RunForceRecovery(bool windowEligible,
    const std::function<RecoveryWindowResult()> &registerWindow, const std::function<int32_t()> &forceRestore);
} // namespace OHOS::Media
#endif // OHOS_MEDIA_RECOVERY_WINDOW_H

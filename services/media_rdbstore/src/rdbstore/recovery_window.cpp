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
#define MLOG_TAG "RecoveryWindow"

#include "recovery_window.h"
#include "recovery_summary.h"
#include "media_log.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <dirent.h>
#include <fcntl.h>
#include <limits>
#include <mutex>
#include <sys/stat.h>
#include <unistd.h>

namespace OHOS::Media {
namespace {
constexpr char WINDOW_PREFIX[] = "recovery_task_";
constexpr int64_t DECIMAL_BASE = 10;
constexpr int64_t MAX_PENDING_AGE_MS = 72LL * 60 * 60 * 1000;
std::recursive_mutex g_windowMutex;
bool g_ignorePending = false;

bool ReadPositiveNumber(const std::string &text, int64_t &value)
{
    if (text.empty()) {
        return false;
    }
    int64_t result = 0;
    for (char ch : text) {
        if (ch < '0' || ch > '9' || result > (std::numeric_limits<int64_t>::max() - (ch - '0')) / DECIMAL_BASE) {
            return false;
        }
        result = result * DECIMAL_BASE + ch - '0';
    }
    value = result;
    return value > 0;
}

dirent *ReadDirectoryEntry(DIR *directory, int &error)
{
    errno = 0;
    dirent *entry = readdir(directory);
    if (entry == nullptr) {
        error = errno;
    }
    return entry;
}

bool HasPendingLocked(const std::string &directory)
{
    std::vector<RecoveryWindow> windows;
    int error = ListRecoveryWindows(directory, windows);
    if (error != 0 && error != ENOENT) {
        MEDIA_ERR_LOG("Cannot read recovery tasks; abandon this attempt, errno=%{public}d", error);
        g_ignorePending = true;
    }
    return error == 0 && !windows.empty();
}
} // namespace

bool ParseRecoveryWindowName(const std::string &name, RecoveryWindow &window)
{
    if (name.compare(0, sizeof(WINDOW_PREFIX) - 1, WINDOW_PREFIX) != 0) {
        return false;
    }
    const std::string times = name.substr(sizeof(WINDOW_PREFIX) - 1);
    const auto separator = times.find('_');
    if (separator == std::string::npos || times.find('_', separator + 1) != std::string::npos) {
        return false;
    }
    RecoveryWindow parsed;
    if (!ReadPositiveNumber(times.substr(0, separator), parsed.startMs) ||
        !ReadPositiveNumber(times.substr(separator + 1), parsed.endMs) || parsed.startMs > parsed.endMs) {
        return false;
    }
    window = parsed;
    return true;
}

RecoveryWindowResult CreateRecoveryWindowFile(const std::string &directory, int64_t startMs, int64_t endMs)
{
    RecoveryWindowResult result;
    if (directory.empty() || startMs <= 0 || endMs <= 0 || startMs > endMs) {
        result.error = EINVAL;
        return result;
    }
    std::lock_guard<std::recursive_mutex> lock(g_windowMutex);
    result.startMs = startMs;
    result.path = directory + "/" + WINDOW_PREFIX + std::to_string(startMs) + "_" + std::to_string(endMs);
    result.attempts = 1;
    int fd = open(result.path.c_str(), O_CREAT | O_EXCL | O_WRONLY | O_CLOEXEC, 0600);
    if (fd < 0) {
        result.error = errno;
        return result;
    }
    if (close(fd) != 0) {
        result.error = errno;
        if (unlink(result.path.c_str()) != 0) {
            result.cleanupError = errno;
        }
    }
    return result;
}

int RemoveRecoveryWindowFile(const std::string &path)
{
    std::lock_guard<std::recursive_mutex> lock(g_windowMutex);
    if (path.empty()) {
        return EINVAL;
    }
    return unlink(path.c_str()) == 0 || errno == ENOENT ? 0 : errno;
}

int ListRecoveryWindows(const std::string &directory, std::vector<RecoveryWindow> &windows)
{
    std::lock_guard<std::recursive_mutex> lock(g_windowMutex);
    windows.clear();
    if (g_ignorePending) {
        return 0;
    }
    DIR *dir = opendir(directory.c_str());
    if (dir == nullptr) {
        return errno;
    }
    int error = 0;
    for (dirent *entry = ReadDirectoryEntry(dir, error); entry != nullptr; entry = ReadDirectoryEntry(dir, error)) {
        const std::string name(entry->d_name);
        if (name.compare(0, sizeof(WINDOW_PREFIX) - 1, WINDOW_PREFIX) != 0) {
            continue;
        }
        RecoveryWindow window;
        if (!ParseRecoveryWindowName(name, window)) {
            MEDIA_WARN_LOG("Ignore malformed recovery task name");
            continue;
        }
        window.path = directory + "/" + name;
        struct stat info {};
        if (lstat(window.path.c_str(), &info) != 0) {
            error = errno;
            break;
        }
        if (!S_ISREG(info.st_mode) || info.st_size != 0) {
            MEDIA_WARN_LOG("Ignore invalid recovery task file");
            continue;
        }
        windows.push_back(window);
    }
    if (closedir(dir) != 0 && error == 0) {
        error = errno;
    }
    if (error == 0) {
        std::sort(windows.begin(), windows.end(), [](const auto &left, const auto &right) {
            return left.startMs != right.startMs ? left.startMs < right.startMs : left.endMs < right.endMs;
        });
    }
    return error;
}

RecoveryWindowResult RegisterRecoveryWindowFile(
    const std::string &directory, int64_t startMs, int64_t endMs)
{
    std::lock_guard<std::recursive_mutex> lock(g_windowMutex);
    RecoveryWindowResult result;
    if (directory.empty() || startMs <= 0 || endMs <= 0 || startMs > endMs) {
        result.error = EINVAL;
        return result;
    }
    std::vector<RecoveryWindow> oldWindows;
    result.error = ListRecoveryWindows(directory, oldWindows);
    if (result.error != 0) {
        return result;
    }
    if (oldWindows.empty()) {
        return CreateRecoveryWindowFile(directory, startMs, endMs);
    }
    const RecoveryWindow &old = oldWindows.front();
    const bool merge = old.startMs <= endMs && endMs - old.startMs <= MAX_PENDING_AGE_MS;
    if (merge) {
        startMs = std::min(startMs, old.startMs);
    }
    result.startMs = startMs;
    result.path = directory + "/" + WINDOW_PREFIX + std::to_string(startMs) + "_" + std::to_string(endMs);
    result.attempts = 1;
    if (rename(old.path.c_str(), result.path.c_str()) != 0) {
        result.error = errno;
        return result;
    }
    if (merge) {
        result.merged = 1;
        if (UpdateRecoveryWindowProgress(old.path, result.path) != 0) {
            MEDIA_WARN_LOG("Could not carry recovery statistics to the merged window");
        }
    } else {
        result.expired = 1;
        CleanupRecoverySummary(old.path);
    }
    return result;
}

bool TryBeginDatabaseRecovery(std::atomic<bool> &restoring)
{
    std::lock_guard<std::recursive_mutex> lock(g_windowMutex);
    bool expected = false;
    if (!restoring.compare_exchange_strong(expected, true)) {
        return false;
    }
    g_ignorePending = false;
    return true;
}

int RunWhileDatabaseAvailable(const std::function<bool()> &isRestoring, const std::function<int()> &operation)
{
    std::lock_guard<std::recursive_mutex> lock(g_windowMutex);
    if (!isRestoring || !operation || isRestoring()) {
        return EBUSY;
    }
    return operation();
}

int RunForCurrentRecoveryWindow(const RecoveryWindow &window, const std::function<int()> &operation)
{
    std::lock_guard<std::recursive_mutex> lock(g_windowMutex);
    if (!operation) {
        return EINVAL;
    }
    if (access(window.path.c_str(), F_OK) != 0) {
        return errno;
    }
    return operation();
}

int RemoveRecoveryWindow(const RecoveryWindow &window)
{
    return RunForCurrentRecoveryWindow(window, [&window]() {
        return unlink(window.path.c_str()) == 0 ? 0 : errno;
    });
}

int IgnoreCompletedRecoveryWindow(const RecoveryWindow &window)
{
    std::lock_guard<std::recursive_mutex> lock(g_windowMutex);
    if (window.path.empty()) {
        return ESTALE;
    }
    g_ignorePending = true;
    return 0;
}

bool HasPendingRecoveryWindow(const std::string &directory)
{
    std::lock_guard<std::recursive_mutex> lock(g_windowMutex);
    return HasPendingLocked(directory);
}

int RunWithoutPendingRecoveryWindow(const std::string &directory, const std::function<int()> &operation)
{
    if (!operation) {
        return EINVAL;
    }
    std::lock_guard<std::recursive_mutex> lock(g_windowMutex);
    if (HasPendingLocked(directory)) {
        return EBUSY;
    }
    return operation();
}

bool IsInRecoveryWindow(int64_t mtimeMs, const RecoveryWindow &window)
{
    return window.startMs > 0 && window.endMs >= window.startMs &&
        mtimeMs >= window.startMs && mtimeMs <= window.endMs;
}

bool IsAtOrAfterRecoveryStart(int64_t mtimeMs, const RecoveryWindow &window)
{
    return window.startMs > 0 && window.endMs >= window.startMs && mtimeMs >= window.startMs;
}

bool ShouldApplyRecoveryJson(int64_t mtimeMs, int64_t databaseMtimeMs, const RecoveryWindow &window)
{
    return IsInRecoveryWindow(mtimeMs, window) && mtimeMs > databaseMtimeMs;
}

bool ShouldScanRecoveryFile(int64_t mtimeMs, int64_t size, int64_t databaseMtimeMs, int64_t databaseSize,
    const RecoveryWindow &window)
{
    return size >= 0 && IsAtOrAfterRecoveryStart(mtimeMs, window) &&
        (mtimeMs > databaseMtimeMs || size != databaseSize);
}

ForceRecoveryResult RunForceRecovery(bool windowEligible,
    const std::function<RecoveryWindowResult()> &registerWindow, const std::function<int32_t()> &forceRestore)
{
    ForceRecoveryResult result;
    result.windowRequired = windowEligible;
    if (!windowEligible) {
        return result;
    }
    if (!forceRestore || !registerWindow) {
        result.window.error = EINVAL;
        return result;
    }
    result.window = registerWindow();
    if (result.window.error != 0 || result.window.path.empty()) {
        if (result.window.error == 0) {
            result.window.error = EINVAL;
        }
        return result;
    }
    result.forceCalled = true;
    result.forceError = forceRestore();
    return result;
}
} // namespace OHOS::Media

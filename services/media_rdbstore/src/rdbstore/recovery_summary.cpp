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
#include "recovery_summary.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <limits>
#include <nlohmann/json.hpp>
#include <unistd.h>
#include <utility>

#include "recovery_window.h"
#include "replica_recovery_policy.h"
#include "rdb_errno.h"

namespace OHOS::Media {
namespace {
using json = nlohmann::json;
constexpr char PROGRESS_FILE[] = "/recovery_progress.json";
constexpr size_t MAX_PROGRESS_SIZE = 1024 * 1024;

std::string ProgressPath(const std::string &taskPath, std::string &taskName)
{
    auto slash = taskPath.find_last_of('/');
    if (slash == std::string::npos || slash == 0) {
        return {};
    }
    taskName = taskPath.substr(slash + 1);
    RecoveryWindow window;
    if (!ParseRecoveryWindowName(taskName, window)) {
        return {};
    }
    return taskPath.substr(0, slash) + PROGRESS_FILE;
}

json ToJson(const std::string &taskName, const DatabaseRecoveryReport &r)
{
    return {{"window", taskName}, {"wait_for_restore", r.waitForRestore},
        {"json_buckets_done", r.jsonBucketsDone}, {"file_buckets_done", r.fileBucketsDone},
        {"date", r.date}, {"force_attempts", r.forceAttempts}, {"force_failures", r.forceFailures},
        {"last_force_error", r.lastForceError}, {"force_ms", r.forceMs},
        {"json_updated", r.jsonUpdated}, {"json_inserted", r.jsonInserted},
        {"file_scanned", r.fileScanned}, {"file_inserted", r.fileInserted},
        {"failed", r.failed}, {"last_scan_error", r.lastScanError}, {"scan_attempts", r.scanAttempts},
        {"scan_ms", r.scanMs}, {"buckets_checked", r.bucketsChecked}, {"buckets_skipped", r.bucketsSkipped}};
}

template<typename T> void ReadNumber(const json &j, const char *key, T &value)
{
    auto it = j.find(key);
    if (it == j.end() || !it->is_number_integer() ||
        (it->is_number_unsigned() && it->get<uint64_t>() > static_cast<uint64_t>(std::numeric_limits<T>::max()))) {
        return;
    }
    const int64_t number = it->get<int64_t>();
    if (number >= static_cast<int64_t>(std::numeric_limits<T>::min()) &&
        number <= static_cast<int64_t>(std::numeric_limits<T>::max())) {
        value = static_cast<T>(number);
    }
}

bool ReadBuckets(const json &j, const char *key, std::set<std::string> &buckets)
{
    auto it = j.find(key);
    if (it == j.end()) {
        return true;
    }
    if (!it->is_array()) {
        return false;
    }
    for (const auto &item : *it) {
        if (!item.is_string()) {
            return false;
        }
        buckets.insert(item.get<std::string>());
    }
    return true;
}

bool FromJson(const json &j, DatabaseRecoveryReport &r)
{
    if (!j.is_object() || !j.contains("window") || !j["window"].is_string()) {
        return false;
    }
    r.windowName = j["window"].get<std::string>();
    RecoveryWindow window;
    if (!ParseRecoveryWindowName(r.windowName, window) ||
        !ReadBuckets(j, "json_buckets_done", r.jsonBucketsDone) ||
        !ReadBuckets(j, "file_buckets_done", r.fileBucketsDone)) {
        return false;
    }
    if (j.contains("wait_for_restore")) {
        if (!j["wait_for_restore"].is_boolean()) {
            return false;
        }
        r.waitForRestore = j["wait_for_restore"].get<bool>();
    }
    if (j.contains("date") && j["date"].is_string()) {
        r.date = j["date"].get<std::string>();
    }
    ReadNumber(j, "force_attempts", r.forceAttempts);
    ReadNumber(j, "force_failures", r.forceFailures);
    ReadNumber(j, "last_force_error", r.lastForceError);
    ReadNumber(j, "force_ms", r.forceMs);
    ReadNumber(j, "json_updated", r.jsonUpdated);
    ReadNumber(j, "json_inserted", r.jsonInserted);
    ReadNumber(j, "file_scanned", r.fileScanned);
    ReadNumber(j, "file_inserted", r.fileInserted);
    ReadNumber(j, "failed", r.failed);
    ReadNumber(j, "last_scan_error", r.lastScanError);
    ReadNumber(j, "scan_attempts", r.scanAttempts);
    ReadNumber(j, "scan_ms", r.scanMs);
    ReadNumber(j, "buckets_checked", r.bucketsChecked);
    ReadNumber(j, "buckets_skipped", r.bucketsSkipped);
    r.windowStart = window.startMs;
    r.windowEnd = window.endMs;
    r.decision = static_cast<int32_t>(ReplicaRecoveryDecision::ALLOW);
    r.stage = r.waitForRestore ? DatabaseRecoveryStage::FORCE_RESTORE :
        (r.scanAttempts > 0 ? DatabaseRecoveryStage::BACKGROUND_SCAN : DatabaseRecoveryStage::DATABASE_USABLE);
    r.result = r.waitForRestore ? DatabaseRecoveryResult::FAILED :
        (r.scanAttempts > 0 ? DatabaseRecoveryResult::PAUSED : DatabaseRecoveryResult::SUCCESS);
    return true;
}
} // namespace

int SaveRecoverySummary(const std::string &taskPath, const DatabaseRecoveryReport &report)
{
    std::string name;
    const std::string path = ProgressPath(taskPath, name);
    if (path.empty()) {
        return EINVAL;
    }
    const std::string temporary = path + ".tmp";
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        return errno != 0 ? errno : EIO;
    }
    output << ToJson(name, report).dump(-1, ' ', false, json::error_handler_t::replace);
    output.close();
    if (!output) {
        (void)unlink(temporary.c_str());
        return EIO;
    }
    if (rename(temporary.c_str(), path.c_str()) != 0) {
        int error = errno;
        (void)unlink(temporary.c_str());
        return error;
    }
    return 0;
}

bool DecodeRecoverySummary(const std::string &data, DatabaseRecoveryReport &report)
{
    if (data.size() > MAX_PROGRESS_SIZE) {
        return false;
    }
    const auto parsed = json::parse(data, nullptr, false);
    DatabaseRecoveryReport read;
    if (parsed.is_discarded() || !FromJson(parsed, read)) {
        return false;
    }
    report = std::move(read);
    return true;
}

int LoadRecoverySummary(const std::string &taskPath, DatabaseRecoveryReport &report)
{
    std::string name;
    const std::string path = ProgressPath(taskPath, name);
    if (path.empty()) {
        return EINVAL;
    }
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input.is_open()) {
        return errno != 0 ? errno : EIO;
    }
    const auto size = static_cast<std::streamoff>(input.tellg());
    if (size <= 0 || size > static_cast<std::streamoff>(MAX_PROGRESS_SIZE)) {
        return EINVAL;
    }
    input.seekg(0);
    std::string data((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    DatabaseRecoveryReport read;
    if (input.bad() || !DecodeRecoverySummary(data, read)) {
        return EINVAL;
    }
    if (read.windowName != name) {
        return ENOENT;
    }
    report = std::move(read);
    return 0;
}

int ForgetRecoverySummary(const std::string &taskPath)
{
    std::string name;
    const std::string path = ProgressPath(taskPath, name);
    if (path.empty()) {
        return EINVAL;
    }
    DatabaseRecoveryReport report;
    int error = LoadRecoverySummary(taskPath, report);
    if (error == ENOENT) {
        return 0;
    }
    if (error != 0 && error != EINVAL) {
        return error;
    }
    return unlink(path.c_str()) == 0 || errno == ENOENT ? 0 : errno;
}

bool IsWaitingForDatabaseRecovery(const DatabaseRecoveryReport &report)
{
    return report.waitForRestore;
}

DatabaseRecoveryResult GetBackgroundRecoveryResult(int32_t error, int64_t failed)
{
    if (error == NativeRdb::E_CANCEL) {
        return DatabaseRecoveryResult::PAUSED;
    }
    if (error != NativeRdb::E_OK) {
        return DatabaseRecoveryResult::FAILED;
    }
    return failed > 0 ? DatabaseRecoveryResult::PARTIAL : DatabaseRecoveryResult::SUCCESS;
}

void RecordForceAttempt(DatabaseRecoveryReport &report, int32_t error, int64_t elapsedMs)
{
    if (report.forceAttempts < std::numeric_limits<int32_t>::max()) {
        ++report.forceAttempts;
    }
    report.waitForRestore = error != NativeRdb::E_OK;
    if (report.waitForRestore) {
        if (report.forceFailures < std::numeric_limits<int32_t>::max()) {
            ++report.forceFailures;
        }
        report.lastForceError = error;
    }
    elapsedMs = std::max<int64_t>(elapsedMs, 0);
    report.forceMs = std::max<int64_t>(report.forceMs, 0);
    report.forceMs = report.forceMs > std::numeric_limits<int64_t>::max() - elapsedMs ?
        std::numeric_limits<int64_t>::max() : report.forceMs + elapsedMs;
}

int UpdateRecoveryWindowProgress(const std::string &fromTask, const std::string &toTask)
{
    DatabaseRecoveryReport report;
    int error = LoadRecoverySummary(fromTask, report);
    if (error == ENOENT) {
        return 0;
    }
    if (error != 0) {
        return error;
    }
    report.jsonBucketsDone.clear();
    report.fileBucketsDone.clear();
    return SaveRecoverySummary(toTask, report);
}

int PrepareRecoveryWindowsAfterRestore(const std::string &directory, int &updated)
{
    updated = 0;
    std::vector<RecoveryWindow> windows;
    int error = ListRecoveryWindows(directory, windows);
    if (error != 0) {
        return error;
    }
    for (const auto &window : windows) {
        int status = RunForCurrentRecoveryWindow(window, [&window]() {
            DatabaseRecoveryReport report;
            int loadError = LoadRecoverySummary(window.path, report);
            if (loadError != 0) {
                return loadError;
            }
            report.waitForRestore = false;
            report.jsonBucketsDone.clear();
            report.fileBucketsDone.clear();
            int saveError = SaveRecoverySummary(window.path, report);
            if (saveError != 0) {
                (void)ForgetRecoverySummary(window.path);
            }
            return saveError;
        });
        if (status == 0) {
            ++updated;
        } else if (status != ENOENT && status != ESTALE) {
            error = status;
        }
    }
    return error;
}

void CleanupRecoverySummary(const std::string &taskPath)
{
    (void)ForgetRecoverySummary(taskPath);
}
} // namespace OHOS::Media

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
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

#include "gtest/gtest.h"
#include "media_file_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_restore.h"
#include "post_event_utils.h"
#include "recovery_bucket_index.h"
#include "recovery_summary.h"
#include "recovery_window.h"
#include "recovery_json_utils.h"
#include "rdb_errno.h"

namespace OHOS::Media {
using namespace testing::ext;
class RecoveryWindowTest : public testing::Test {};
namespace {
constexpr int64_t TEST_WINDOW_START = 100;
constexpr int64_t TEST_WINDOW_MIDDLE = 200;
constexpr int64_t TEST_WINDOW_END = 300;
constexpr int64_t TEST_WINDOW_EARLIER = 99;
constexpr int64_t TEST_WINDOW_LATER = 400;
constexpr int64_t TEST_PENDING_MAX_AGE = 72LL * 60 * 60 * 1000;

std::string CreateRecoveryTestDirectory()
{
    if (!MediaFileUtils::CreateDirectory("/data/test/media_library_restore")) {
        return {};
    }
    char directory[] = "/data/test/media_library_restore/window_XXXXXX";
    const char *path = mkdtemp(directory);
    return path == nullptr ? std::string() : std::string(path);
}
}

HWTEST_F(RecoveryWindowTest, RestoreEntryIgnoresOtherErrorsAndConcurrentCorruption, TestSize.Level1)
{
    auto &restore = MediaLibraryRestore::GetInstance();
    const bool previous = restore.isRestoring_.exchange(false);
    restore.CheckRestore(NativeRdb::E_OK);
    EXPECT_FALSE(restore.IsRestoring());
    restore.isRestoring_ = true;
    restore.CheckRestore(NativeRdb::E_SQLITE_CORRUPT);
    EXPECT_TRUE(restore.IsRestoring());
    restore.isRestoring_ = previous;
}

HWTEST_F(RecoveryWindowTest, RecoveryReportStageValuesAreCompatible, TestSize.Level1)
{
    EXPECT_EQ(static_cast<int32_t>(DatabaseRecoveryStage::ORDINARY_RESTORE), 2);
    EXPECT_EQ(static_cast<int32_t>(DatabaseRecoveryStage::DIAGNOSIS), 3);
    EXPECT_EQ(static_cast<int32_t>(DatabaseRecoveryStage::FORCE_RESTORE), 4);
    EXPECT_EQ(static_cast<int32_t>(DatabaseRecoveryStage::DATABASE_USABLE), 5);
    EXPECT_EQ(static_cast<int32_t>(DatabaseRecoveryStage::BACKGROUND_SCAN), 6);
    EXPECT_EQ(static_cast<int32_t>(DatabaseRecoveryStage::WINDOW_REGISTRATION), 7);
}

HWTEST_F(RecoveryWindowTest, MultipleForceAttemptsRetainFailuresAfterSuccess, TestSize.Level1)
{
    DatabaseRecoveryReport report;
    RecordForceAttempt(report, 123, TEST_WINDOW_START);
    RecordForceAttempt(report, 456, TEST_WINDOW_MIDDLE);
    RecordForceAttempt(report, 0, TEST_WINDOW_END);
    EXPECT_EQ(report.forceAttempts, 3);
    EXPECT_EQ(report.forceFailures, 2);
    EXPECT_EQ(report.lastForceError, 456);
    EXPECT_FALSE(report.waitForRestore);
    EXPECT_EQ(report.forceMs, TEST_WINDOW_START + TEST_WINDOW_MIDDLE + TEST_WINDOW_END);
    EXPECT_FALSE(report.waitForRestore);
    RecordForceAttempt(report, 123, TEST_WINDOW_START);
    EXPECT_TRUE(report.waitForRestore);
}

HWTEST_F(RecoveryWindowTest, CreateEmptyFileAndDoNotOverwrite, TestSize.Level1)
{
    const std::string tempDir = CreateRecoveryTestDirectory();
    ASSERT_FALSE(tempDir.empty());
    auto result = CreateRecoveryWindowFile(tempDir, 100, 200);
    EXPECT_EQ(result.error, 0);
    EXPECT_EQ(result.attempts, 1);
    struct stat info {};
    ASSERT_EQ(stat(result.path.c_str(), &info), 0);
    EXPECT_EQ(info.st_size, 0);
    auto duplicate = CreateRecoveryWindowFile(tempDir, 100, 200);
    EXPECT_EQ(duplicate.error, EEXIST);
    EXPECT_EQ(duplicate.attempts, 1);
    EXPECT_EQ(RemoveRecoveryWindowFile(result.path), 0);
    EXPECT_EQ(RemoveRecoveryWindowFile(result.path), 0);
    EXPECT_EQ(rmdir(tempDir.c_str()), 0);
}

HWTEST_F(RecoveryWindowTest, InvalidWindowAndMissingDirectory, TestSize.Level1)
{
    auto invalid = CreateRecoveryWindowFile("/data/test", 200, 100);
    EXPECT_EQ(invalid.error, EINVAL);
    EXPECT_EQ(invalid.attempts, 0);
    const std::string tempDir = CreateRecoveryTestDirectory();
    ASSERT_FALSE(tempDir.empty());
    const std::string missing = std::string(tempDir) + "/missing";
    auto result = CreateRecoveryWindowFile(missing, 100, 200);
    EXPECT_EQ(result.error, ENOENT);
    EXPECT_EQ(result.attempts, 1);
    EXPECT_EQ(rmdir(tempDir.c_str()), 0);
}

HWTEST_F(RecoveryWindowTest, RecentWindowIsMergedAndSummaryIsPreserved, TestSize.Level1)
{
    const std::string tempDir = CreateRecoveryTestDirectory();
    ASSERT_FALSE(tempDir.empty());
    auto old = CreateRecoveryWindowFile(tempDir, TEST_WINDOW_START, TEST_WINDOW_MIDDLE);
    ASSERT_EQ(old.error, 0);
    DatabaseRecoveryReport report;
    report.date = "first";
    report.waitForRestore = true;
    report.forceAttempts = 2;
    report.forceFailures = 1;
    report.lastForceError = 123;
    report.jsonBucketsDone = {"1"};
    report.fileBucketsDone = {"1"};
    ASSERT_EQ(SaveRecoverySummary(old.path, report), 0);
    DatabaseRecoveryReport original;
    ASSERT_EQ(LoadRecoverySummary(old.path, original), 0);
    auto merged = RegisterRecoveryWindowFile(tempDir, TEST_WINDOW_MIDDLE, TEST_WINDOW_END);
    ASSERT_EQ(merged.error, 0);
    EXPECT_EQ(merged.merged, 1);
    EXPECT_EQ(merged.expired, 0);
    EXPECT_EQ(merged.startMs, TEST_WINDOW_START);
    EXPECT_NE(access(old.path.c_str(), F_OK), 0);
    DatabaseRecoveryReport restored;
    EXPECT_EQ(LoadRecoverySummary(merged.path, restored), 0);
    EXPECT_TRUE(restored.waitForRestore);
    EXPECT_EQ(restored.forceAttempts, 2);
    EXPECT_EQ(restored.forceFailures, 1);
    EXPECT_EQ(restored.lastForceError, 123);
    EXPECT_EQ(restored.date, original.date);
    EXPECT_TRUE(restored.jsonBucketsDone.empty());
    EXPECT_TRUE(restored.fileBucketsDone.empty());
    auto extended = RegisterRecoveryWindowFile(tempDir, TEST_WINDOW_EARLIER, TEST_WINDOW_LATER);
    ASSERT_EQ(extended.error, 0);
    EXPECT_EQ(extended.startMs, TEST_WINDOW_EARLIER);
    ASSERT_EQ(LoadRecoverySummary(extended.path, restored), 0);
    EXPECT_EQ(restored.forceAttempts, 2);
    EXPECT_EQ(ForgetRecoverySummary(extended.path), 0);
    EXPECT_EQ(LoadRecoverySummary(extended.path, restored), ENOENT);
    EXPECT_EQ(LoadRecoverySummary(extended.path, original), ENOENT);
    EXPECT_EQ(RemoveRecoveryWindowFile(extended.path), 0);
    CleanupRecoverySummary(extended.path);
    EXPECT_EQ(rmdir(tempDir.c_str()), 0);
}

HWTEST_F(RecoveryWindowTest, ExpiredWindowIsReplacedOnlyAfterNewTaskExists, TestSize.Level1)
{
    const std::string tempDir = CreateRecoveryTestDirectory();
    ASSERT_FALSE(tempDir.empty());
    auto old = CreateRecoveryWindowFile(tempDir, TEST_WINDOW_START, TEST_WINDOW_MIDDLE);
    ASSERT_EQ(old.error, 0);
    const int64_t now = TEST_WINDOW_START + TEST_PENDING_MAX_AGE + 1;
    auto result = RegisterRecoveryWindowFile(tempDir, now - 1, now);
    ASSERT_EQ(result.error, 0);
    EXPECT_EQ(result.expired, 1);
    EXPECT_EQ(result.merged, 0);
    EXPECT_EQ(result.startMs, now - 1);
    EXPECT_NE(access(old.path.c_str(), F_OK), 0);
    EXPECT_EQ(access(result.path.c_str(), F_OK), 0);
    EXPECT_EQ(RemoveRecoveryWindowFile(result.path), 0);
    EXPECT_EQ(rmdir(tempDir.c_str()), 0);
}

HWTEST_F(RecoveryWindowTest, SuccessfulRecoveryMakesFailedForceWindowUsable, TestSize.Level1)
{
    const std::string tempDir = CreateRecoveryTestDirectory();
    ASSERT_FALSE(tempDir.empty());
    auto task = CreateRecoveryWindowFile(tempDir, TEST_WINDOW_START, TEST_WINDOW_END);
    ASSERT_EQ(task.error, 0);
    DatabaseRecoveryReport report;
    report.waitForRestore = true;
    report.lastForceError = 123;
    report.stage = DatabaseRecoveryStage::FORCE_RESTORE;
    ASSERT_EQ(SaveRecoverySummary(task.path, report), 0);
    EXPECT_TRUE(IsWaitingForDatabaseRecovery(report));
    int updated = 0;
    ASSERT_EQ(PrepareRecoveryWindowsAfterRestore(tempDir, updated), 0);
    EXPECT_EQ(updated, 1);
    DatabaseRecoveryReport restored;
    ASSERT_EQ(LoadRecoverySummary(task.path, restored), 0);
    EXPECT_EQ(restored.stage, DatabaseRecoveryStage::DATABASE_USABLE);
    EXPECT_EQ(restored.lastForceError, 123);
    EXPECT_FALSE(restored.waitForRestore);
    EXPECT_FALSE(IsWaitingForDatabaseRecovery(restored));
    restored.stage = DatabaseRecoveryStage::BACKGROUND_SCAN;
    EXPECT_FALSE(IsWaitingForDatabaseRecovery(restored));
    EXPECT_EQ(RemoveRecoveryWindowFile(task.path), 0);
    CleanupRecoverySummary(task.path);
    EXPECT_EQ(rmdir(tempDir.c_str()), 0);
}

HWTEST_F(RecoveryWindowTest, ForceDependsOnRegistrationOnlyWhenEligible, TestSize.Level1)
{
    int registrations = 0;
    int forces = 0;
    auto failRegistration = [&registrations]() {
        ++registrations;
        RecoveryWindowResult result;
        result.error = ENOSPC;
        result.attempts = 1;
        return result;
    };
    auto force = [&forces]() {
        ++forces;
        return 0;
    };
    auto unavailable = RunForceRecovery(false, failRegistration, force);
    EXPECT_FALSE(unavailable.forceCalled);
    EXPECT_EQ(registrations, 0);
    EXPECT_EQ(forces, 0);
    auto failed = RunForceRecovery(true, failRegistration, force);
    EXPECT_FALSE(failed.forceCalled);
    EXPECT_EQ(failed.window.error, ENOSPC);
    EXPECT_EQ(failed.window.attempts, 1);
    EXPECT_EQ(forces, 0);
}

HWTEST_F(RecoveryWindowTest, ForceFailureRetainsRegisteredWindow, TestSize.Level1)
{
    const std::string tempDir = CreateRecoveryTestDirectory();
    ASSERT_FALSE(tempDir.empty());
    auto registerWindow = [&tempDir]() { return CreateRecoveryWindowFile(tempDir, 100, 200); };
    auto failForce = []() { return 123; };
    auto result = RunForceRecovery(true, registerWindow, failForce);
    EXPECT_TRUE(result.forceCalled);
    EXPECT_EQ(result.forceError, 123);
    EXPECT_EQ(access(result.window.path.c_str(), F_OK), 0);
    auto noWindow = RunForceRecovery(false, registerWindow, failForce);
    EXPECT_FALSE(noWindow.forceCalled);
    EXPECT_EQ(RemoveRecoveryWindowFile(result.window.path), 0);
    EXPECT_EQ(rmdir(tempDir.c_str()), 0);
}

HWTEST_F(RecoveryWindowTest, ForceSuccessRetainsRegisteredWindow, TestSize.Level1)
{
    auto registerWindow = []() {
        RecoveryWindowResult result;
        result.path = "owned-window";
        result.attempts = 1;
        return result;
    };
    auto force = []() { return 0; };
    auto result = RunForceRecovery(true, registerWindow, force);
    EXPECT_TRUE(result.forceCalled);
    EXPECT_EQ(result.forceError, 0);
    EXPECT_EQ(result.window.attempts, 1);
    EXPECT_EQ(result.window.path, "owned-window");
}

HWTEST_F(RecoveryWindowTest, TwoPassDecisionsRespectWindowAndDatabaseValues, TestSize.Level1)
{
    RecoveryWindow window;
    window.startMs = 100;
    window.endMs = 200;
    EXPECT_FALSE(ShouldApplyRecoveryJson(99, 0, window));
    EXPECT_TRUE(ShouldApplyRecoveryJson(100, 99, window));
    EXPECT_FALSE(ShouldApplyRecoveryJson(100, 100, window));
    EXPECT_TRUE(ShouldApplyRecoveryJson(200, 199, window));
    EXPECT_FALSE(ShouldApplyRecoveryJson(201, 100, window));
    EXPECT_FALSE(ShouldScanRecoveryFile(99, 2, 99, 1, window));
    EXPECT_FALSE(ShouldScanRecoveryFile(100, 1, 100, 1, window));
    EXPECT_TRUE(ShouldScanRecoveryFile(100, 1, 99, 1, window));
    EXPECT_TRUE(ShouldScanRecoveryFile(100, 2, 100, 1, window));
    EXPECT_TRUE(ShouldScanRecoveryFile(100, 2, 100, -1, window));
    EXPECT_FALSE(ShouldScanRecoveryFile(201, 2, 100, 1, window));
    EXPECT_FALSE(ShouldScanRecoveryFile(100, -1, 100, 1, window));
}

HWTEST_F(RecoveryWindowTest, BackgroundFailuresAreReportedAsPartial, TestSize.Level1)
{
    EXPECT_EQ(GetBackgroundRecoveryResult(NativeRdb::E_OK, 0), DatabaseRecoveryResult::SUCCESS);
    EXPECT_EQ(GetBackgroundRecoveryResult(NativeRdb::E_OK, 1), DatabaseRecoveryResult::PARTIAL);
    EXPECT_EQ(GetBackgroundRecoveryResult(NativeRdb::E_CANCEL, 1), DatabaseRecoveryResult::PAUSED);
    EXPECT_EQ(GetBackgroundRecoveryResult(NativeRdb::E_SQLITE_CORRUPT, 1), DatabaseRecoveryResult::FAILED);
}

HWTEST_F(RecoveryWindowTest, DamagedJsonIsDistinguishedFromReadFailure, TestSize.Level1)
{
    const std::string tempDir = CreateRecoveryTestDirectory();
    ASSERT_FALSE(tempDir.empty());
    const std::string path = std::string(tempDir) + "/photo.json";
    nlohmann::json metadata;
    EXPECT_EQ(ReadRecoveryJsonFile(path, metadata), RecoveryJsonStatus::READ_ERROR);
    std::ofstream(path) << R"({"data":"photo"})";
    EXPECT_EQ(ReadRecoveryJsonFile(path, metadata), RecoveryJsonStatus::OK);
    std::ofstream(path, std::ios::trunc) << "{\"data\":";
    EXPECT_EQ(ReadRecoveryJsonFile(path, metadata), RecoveryJsonStatus::INVALID);
    std::ofstream(path, std::ios::trunc) << "[]";
    EXPECT_EQ(ReadRecoveryJsonFile(path, metadata), RecoveryJsonStatus::INVALID);
    EXPECT_EQ(unlink(path.c_str()), 0);
    EXPECT_EQ(rmdir(tempDir.c_str()), 0);
}

HWTEST_F(RecoveryWindowTest, PendingWindowBlocksBackupAndCanBeResumed, TestSize.Level1)
{
    const std::string tempDir = CreateRecoveryTestDirectory();
    ASSERT_FALSE(tempDir.empty());
    int calls = 0;
    auto operation = [&calls]() { ++calls; return 0; };
    EXPECT_EQ(RunWithoutPendingRecoveryWindow(tempDir, operation), 0);
    auto task = CreateRecoveryWindowFile(tempDir, 100, 200);
    ASSERT_EQ(task.error, 0);
    EXPECT_EQ(RunWithoutPendingRecoveryWindow(tempDir, operation), EBUSY);
    EXPECT_EQ(calls, 1);
    std::vector<RecoveryWindow> windows;
    ASSERT_EQ(ListRecoveryWindows(tempDir, windows), 0);
    ASSERT_EQ(windows.size(), 1);
    EXPECT_EQ(windows[0].startMs, 100);
    EXPECT_EQ(windows[0].endMs, 200);
    EXPECT_EQ(RemoveRecoveryWindow(windows[0]), 0);
    EXPECT_EQ(RunWithoutPendingRecoveryWindow(tempDir, operation), 0);
    EXPECT_EQ(calls, 2);
    EXPECT_EQ(rmdir(tempDir.c_str()), 0);
}

HWTEST_F(RecoveryWindowTest, MissingPreferencesDirectoryDoesNotBlockOrdinaryBackup, TestSize.Level1)
{
    const std::string tempDir = CreateRecoveryTestDirectory();
    ASSERT_FALSE(tempDir.empty());
    const std::string missing = std::string(tempDir) + "/not_created";
    EXPECT_FALSE(HasPendingRecoveryWindow(missing));
    int calls = 0;
    EXPECT_EQ(RunWithoutPendingRecoveryWindow(missing, [&calls]() { ++calls; return 0; }), 0);
    EXPECT_EQ(calls, 1);
    auto task = CreateRecoveryWindowFile(missing, 100, 200);
    EXPECT_EQ(task.error, ENOENT); // Force must still be blocked when registration fails.
    EXPECT_EQ(rmdir(tempDir.c_str()), 0);
}

HWTEST_F(RecoveryWindowTest, MalformedTaskIsNeverConsumed, TestSize.Level1)
{
    RecoveryWindow window;
    EXPECT_FALSE(ParseRecoveryWindowName("recovery_task_100", window));
    EXPECT_FALSE(ParseRecoveryWindowName("recovery_task_200_100", window));
    EXPECT_FALSE(ParseRecoveryWindowName("recovery_task_0_100", window));
    EXPECT_FALSE(ParseRecoveryWindowName("recovery_task_9223372036854775808_1", window));
    EXPECT_TRUE(ParseRecoveryWindowName("recovery_task_100_200", window));
    EXPECT_EQ(window.startMs, 100);
    EXPECT_EQ(window.endMs, 200);
}

HWTEST_F(RecoveryWindowTest, InvalidTaskDoesNotBlockValidTasksOrBackup, TestSize.Level1)
{
    const std::string tempDir = CreateRecoveryTestDirectory();
    ASSERT_FALSE(tempDir.empty());
    const std::string badName = std::string(tempDir) + "/recovery_task_bad";
    const std::string badContent = std::string(tempDir) + "/recovery_task_100_200";
    std::ofstream(badName) << "invalid";
    std::ofstream(badContent) << "invalid";
    EXPECT_FALSE(HasPendingRecoveryWindow(tempDir));
    std::vector<RecoveryWindow> windows;
    EXPECT_EQ(ListRecoveryWindows(tempDir, windows), 0);
    EXPECT_TRUE(windows.empty());
    int calls = 0;
    EXPECT_EQ(RunWithoutPendingRecoveryWindow(tempDir, [&calls]() { ++calls; return 0; }), 0);
    EXPECT_EQ(calls, 1);
    EXPECT_EQ(unlink(badContent.c_str()), 0);
    auto valid = CreateRecoveryWindowFile(tempDir, 100, 200);
    ASSERT_EQ(valid.error, 0);
    EXPECT_EQ(ListRecoveryWindows(tempDir, windows), 0);
    EXPECT_EQ(windows.size(), 1U);
    EXPECT_EQ(RemoveRecoveryWindowFile(valid.path), 0);
    EXPECT_EQ(unlink(badName.c_str()), 0);
    EXPECT_EQ(rmdir(tempDir.c_str()), 0);
}

HWTEST_F(RecoveryWindowTest, CompletedTaskCleanupFailureDoesNotBlockUntilNextRecovery, TestSize.Level1)
{
    const std::string tempDir = CreateRecoveryTestDirectory();
    ASSERT_FALSE(tempDir.empty());
    auto task = CreateRecoveryWindowFile(tempDir, TEST_WINDOW_START, TEST_WINDOW_END);
    ASSERT_EQ(task.error, 0);
    std::vector<RecoveryWindow> windows;
    ASSERT_EQ(ListRecoveryWindows(tempDir, windows), 0);
    ASSERT_EQ(windows.size(), 1U);
    ASSERT_EQ(IgnoreCompletedRecoveryWindow(windows[0]), 0);
    EXPECT_FALSE(HasPendingRecoveryWindow(tempDir));
    EXPECT_EQ(ListRecoveryWindows(tempDir, windows), 0);
    EXPECT_TRUE(windows.empty());
    std::atomic<bool> restoring {false};
    EXPECT_TRUE(TryBeginDatabaseRecovery(restoring));
    EXPECT_TRUE(HasPendingRecoveryWindow(tempDir));
    EXPECT_EQ(IgnoreCompletedRecoveryWindow(RecoveryWindow {}), ESTALE);
    EXPECT_EQ(RemoveRecoveryWindowFile(task.path), 0);
    EXPECT_EQ(rmdir(tempDir.c_str()), 0);
}

HWTEST_F(RecoveryWindowTest, UnreadableTaskDirectoryDoesNotBlockBackup, TestSize.Level1)
{
    const std::string tempDir = CreateRecoveryTestDirectory();
    ASSERT_FALSE(tempDir.empty());
    const std::string directory = std::string(tempDir) + "/tasks";
    std::ofstream(directory) << "not a directory";
    EXPECT_FALSE(HasPendingRecoveryWindow(directory));
    int calls = 0;
    EXPECT_EQ(RunWithoutPendingRecoveryWindow(directory, [&calls]() { ++calls; return 0; }), 0);
    EXPECT_EQ(calls, 1);
    ASSERT_EQ(unlink(directory.c_str()), 0);
    ASSERT_EQ(mkdir(directory.c_str(), 0700), 0);
    const std::string oldTask = directory + "/recovery_task_100_200";
    std::ofstream { oldTask };
    EXPECT_FALSE(HasPendingRecoveryWindow(directory));
    EXPECT_EQ(access(oldTask.c_str(), F_OK), 0);
    std::atomic<bool> restoring {false};
    EXPECT_TRUE(TryBeginDatabaseRecovery(restoring));
    EXPECT_TRUE(HasPendingRecoveryWindow(directory));
    EXPECT_EQ(unlink(oldTask.c_str()), 0);
    EXPECT_EQ(rmdir(directory.c_str()), 0);
    EXPECT_EQ(rmdir(tempDir.c_str()), 0);
}

HWTEST_F(RecoveryWindowTest, JsonFieldsOnlyAcceptCompatibleValues, TestSize.Level1)
{
    NativeRdb::ValuesBucket values;
    const nlohmann::json json = {{"size", 42}, {"note", "trip"}, {"missing", nullptr},
        {"invalid", "not-a-number"}};
    EXPECT_TRUE(PutRecoveryJsonValue(values, "size", json["size"], "BIGINT"));
    EXPECT_TRUE(PutRecoveryJsonValue(values, "note", json["note"], "TEXT"));
    EXPECT_FALSE(PutRecoveryJsonValue(values, "missing", json["missing"], "REAL"));
    EXPECT_FALSE(PutRecoveryJsonValue(values, "invalid", json["invalid"], "INTEGER"));
    EXPECT_TRUE(values.HasColumn("size"));
    EXPECT_TRUE(values.HasColumn("note"));
    EXPECT_FALSE(values.HasColumn("missing"));
    EXPECT_FALSE(values.HasColumn("invalid"));
    EXPECT_FALSE(values.HasColumn("absent"));
}

HWTEST_F(RecoveryWindowTest, BackupCanReportCorruptionWithoutDeadlock, TestSize.Level1)
{
    const std::string tempDir = CreateRecoveryTestDirectory();
    ASSERT_FALSE(tempDir.empty());
    std::atomic<bool> restoring {false};
    EXPECT_EQ(RunWithoutPendingRecoveryWindow(tempDir, [&restoring]() {
        return TryBeginDatabaseRecovery(restoring) ? 0 : EBUSY;
    }), 0);
    EXPECT_TRUE(restoring.load());
    EXPECT_EQ(RunWhileDatabaseAvailable(restoring, []() { return 0; }), EBUSY);
    restoring = false;
    EXPECT_EQ(RunWhileDatabaseAvailable(restoring, []() { return 0; }), 0);
    EXPECT_EQ(rmdir(tempDir.c_str()), 0);
}

HWTEST_F(RecoveryWindowTest, BucketIndexSkipsOnlyWhenAllJsonFilesAreOlder, TestSize.Level1)
{
    const std::string tempDir = CreateRecoveryTestDirectory();
    ASSERT_FALSE(tempDir.empty());
    const std::string dir(tempDir);
    const std::vector<std::string> names {"a.json", "b.json", "c.json"};
    for (size_t i = 0; i < names.size(); ++i) {
        const std::string path = dir + "/" + names[i];
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_EXCL, 0600);
        ASSERT_GE(fd, 0);
        ASSERT_EQ(close(fd), 0);
        struct timespec times[2] {{static_cast<time_t>(i + 1), 0}, {static_cast<time_t>(i + 1), 0}};
        ASSERT_EQ(utimensat(AT_FDCWD, path.c_str(), times, 0), 0);
    }
    ASSERT_EQ(PrepareRecoveryBucketWrite(dir, "c.json", 3000), 0);
    EXPECT_TRUE(CanSkipRecoveryBucket(dir, 3001));
    EXPECT_FALSE(CanSkipRecoveryBucket(dir, 3000));
    // A latest file inside or after the window cannot rule out other matching files.
    EXPECT_FALSE(CanSkipRecoveryBucket(dir, 1500));
    InvalidateRecoveryBucketIndex(dir);
    EXPECT_FALSE(CanSkipRecoveryBucket(dir, 3001));
    struct timespec future[2] {{4, 0}, {4, 0}};
    ASSERT_EQ(utimensat(AT_FDCWD, (dir + "/b.json").c_str(), future, 0), 0);
    ASSERT_EQ(PrepareRecoveryBucketWrite(dir, "c.json", 3000), 0);
    // A future-dated existing file prevents certifying this marker.
    EXPECT_FALSE(CanSkipRecoveryBucket(dir, 3001));
    future[1] = {2, 0};
    ASSERT_EQ(utimensat(AT_FDCWD, (dir + "/b.json").c_str(), future, 0), 0);
    ASSERT_EQ(PrepareRecoveryBucketWrite(dir, "c.json", 3000), 0);
    EXPECT_TRUE(CanSkipRecoveryBucket(dir, 3001));
    const std::string marker = dir + "/" + RECOVERY_BUCKET_MARKER_PREFIX + "3000_c.json.idx";
    struct stat markerInfo {};
    ASSERT_EQ(lstat(marker.c_str(), &markerInfo), 0);
    EXPECT_EQ(markerInfo.st_size, 0);
    EXPECT_EQ(unlink(marker.c_str()), 0);
    for (const auto &name : names) {
        EXPECT_EQ(unlink((dir + "/" + name).c_str()), 0);
    }
    EXPECT_EQ(rmdir(dir.c_str()), 0);
}

HWTEST_F(RecoveryWindowTest, BucketIndexFallsBackAfterDirectoryChangeOrCorruption, TestSize.Level1)
{
    const std::string tempDir = CreateRecoveryTestDirectory();
    ASSERT_FALSE(tempDir.empty());
    const std::string dir(tempDir);
    EXPECT_FALSE(CanSkipRecoveryBucket(dir, 1000));
    // The marker is created before the JSON; until the JSON exists it is invalid.
    ASSERT_EQ(PrepareRecoveryBucketWrite(dir, "new.json", 10000), 0);
    EXPECT_FALSE(CanSkipRecoveryBucket(dir, 12000));
    const std::string file = dir + "/new.json";
    int fd = open(file.c_str(), O_CREAT | O_WRONLY | O_EXCL, 0600);
    ASSERT_GE(fd, 0);
    ASSERT_EQ(close(fd), 0);
    struct timespec times[2] {{10, 0}, {10, 0}};
    ASSERT_EQ(utimensat(AT_FDCWD, file.c_str(), times, 0), 0);
    EXPECT_TRUE(CanSkipRecoveryBucket(dir, 10001));
    EXPECT_FALSE(CanSkipRecoveryBucket(dir, 10000));
    const std::string marker = dir + "/" + RECOVERY_BUCKET_MARKER_PREFIX + "10000_new.json.idx";
    struct timespec markerTimes[2] {{8, 999000000}, {8, 999000000}};
    ASSERT_EQ(utimensat(AT_FDCWD, marker.c_str(), markerTimes, 0), 0);
    EXPECT_FALSE(CanSkipRecoveryBucket(dir, 11000));
    markerTimes[1] = {10, 0};
    ASSERT_EQ(utimensat(AT_FDCWD, marker.c_str(), markerTimes, 0), 0);
    EXPECT_TRUE(CanSkipRecoveryBucket(dir, 10001));
    times[1] = {9, 500000000};
    ASSERT_EQ(utimensat(AT_FDCWD, file.c_str(), times, 0), 0);
    EXPECT_FALSE(CanSkipRecoveryBucket(dir, 9750)); // Marker time is newer than the JSON.
    EXPECT_TRUE(CanSkipRecoveryBucket(dir, 10001));
    times[1] = {11, 0};
    ASSERT_EQ(utimensat(AT_FDCWD, file.c_str(), times, 0), 0);
    EXPECT_TRUE(CanSkipRecoveryBucket(dir, 11001)); // Exactly one second is accepted.
    times[1] = {11, 1000000};
    ASSERT_EQ(utimensat(AT_FDCWD, file.c_str(), times, 0), 0);
    EXPECT_FALSE(CanSkipRecoveryBucket(dir, 12000)); // More than one second falls back.
    ASSERT_EQ(PrepareRecoveryBucketWrite(dir, "new.json", 13000), 0);
    EXPECT_FALSE(CanSkipRecoveryBucket(dir, 14000)); // The marker precedes the pending JSON write.
    times[1] = {13, 0};
    ASSERT_EQ(utimensat(AT_FDCWD, file.c_str(), times, 0), 0);
    EXPECT_TRUE(CanSkipRecoveryBucket(dir, 14000));
    InvalidateRecoveryBucketIndex(dir);
    EXPECT_FALSE(CanSkipRecoveryBucket(dir, 13000));
    EXPECT_EQ(unlink(file.c_str()), 0);
    EXPECT_EQ(rmdir(dir.c_str()), 0);
}

HWTEST_F(RecoveryWindowTest, BucketMarkerRejectsInvalidNamesAndTimes, TestSize.Level1)
{
    int64_t time = 0;
    std::string name;
    EXPECT_TRUE(ParseRecoveryBucketMarker(".recovery_latest_1000_a.json.idx", time, name));
    EXPECT_EQ(time, 1000);
    EXPECT_EQ(name, "a.json");
    EXPECT_FALSE(ParseRecoveryBucketMarker(".recovery_latest_bad_a.json.idx", time, name));
    EXPECT_FALSE(ParseRecoveryBucketMarker(".recovery_latest_1_../a.json.idx", time, name));
    EXPECT_FALSE(ParseRecoveryBucketMarker(".recovery_latest_1_a.json", time, name));
    EXPECT_FALSE(ParseRecoveryBucketMarker(".recovery_unverified_1_a.json.idx", time, name));
    EXPECT_FALSE(ParseRecoveryBucketMarker(".recovery_latest_999999999999999999999_a.json.idx", time, name));
}

HWTEST_F(RecoveryWindowTest, RecoverySummaryPersistsAndProgressResets, TestSize.Level1)
{
    const std::string tempDir = CreateRecoveryTestDirectory();
    ASSERT_FALSE(tempDir.empty());
    auto task = CreateRecoveryWindowFile(tempDir, 100, 200);
    ASSERT_EQ(task.error, 0);
    DatabaseRecoveryReport report;
    report.date = "20261031";
    report.stage = DatabaseRecoveryStage::BACKGROUND_SCAN;
    report.result = DatabaseRecoveryResult::PAUSED;
    report.waitForRestore = false;
    report.windowEnd = 200;
    report.scanAttempts = 2;
    report.scanMs = 130;
    report.jsonUpdated = 17;
    report.jsonInserted = 4;
    report.failed = 1;
    report.lastScanError = E_ERR;
    report.jsonBucketsDone = {"1", "2"};
    report.fileBucketsDone = {"1"};
    ASSERT_EQ(SaveRecoverySummary(task.path, report), 0);
    DatabaseRecoveryReport restored;
    ASSERT_EQ(LoadRecoverySummary(task.path, restored), 0);
    EXPECT_EQ(restored.date, report.date);
    EXPECT_EQ(restored.windowStart, 100);
    EXPECT_EQ(restored.windowEnd, 200);
    DatabaseRecoveryReport unrelated;
    EXPECT_EQ(LoadRecoverySummary(std::string(tempDir) + "/recovery_task_99_200", unrelated), ENOENT);
    EXPECT_TRUE(unrelated.jsonBucketsDone.empty());
    EXPECT_TRUE(unrelated.fileBucketsDone.empty());
    EXPECT_EQ(restored.stage, report.stage);
    EXPECT_EQ(restored.result, report.result);
    EXPECT_FALSE(restored.waitForRestore);
    EXPECT_EQ(restored.scanAttempts, 2);
    EXPECT_EQ(restored.failed, 1);
    EXPECT_EQ(restored.scanMs, 130);
    EXPECT_EQ(restored.jsonUpdated, 17);
    EXPECT_EQ(restored.jsonInserted, 4);
    EXPECT_EQ(restored.lastScanError, E_ERR);
    EXPECT_EQ(restored.failed, 1);
    EXPECT_EQ(restored.jsonBucketsDone, report.jsonBucketsDone);
    EXPECT_EQ(restored.fileBucketsDone, report.fileBucketsDone);
    int updated = 0;
    EXPECT_EQ(PrepareRecoveryWindowsAfterRestore(tempDir, updated), 0);
    EXPECT_EQ(updated, 1);
    ASSERT_EQ(LoadRecoverySummary(task.path, restored), 0);
    EXPECT_TRUE(restored.jsonBucketsDone.empty());
    EXPECT_TRUE(restored.fileBucketsDone.empty());
    EXPECT_EQ(PrepareRecoveryWindowsAfterRestore(tempDir, updated), 0);
    ASSERT_EQ(LoadRecoverySummary(task.path, restored), 0);
    EXPECT_TRUE(restored.jsonBucketsDone.empty());
    EXPECT_TRUE(restored.fileBucketsDone.empty());
    EXPECT_TRUE(DecodeRecoverySummary(R"({"window":"recovery_task_100_200","force_attempts":"bad",)"
        R"("json_buckets_done":["1"],"file_buckets_done":[]})", restored));
    EXPECT_EQ(restored.forceAttempts, 0);
    EXPECT_EQ(restored.jsonBucketsDone.count("1"), 1U);
    EXPECT_FALSE(DecodeRecoverySummary("{}", restored));
    EXPECT_FALSE(DecodeRecoverySummary("[false]", restored));
    EXPECT_EQ(LoadRecoverySummary(std::string(tempDir) + "/not_a_task", restored), EINVAL);
    CleanupRecoverySummary(task.path);
    EXPECT_EQ(LoadRecoverySummary(task.path, restored), ENOENT);
    EXPECT_EQ(RemoveRecoveryWindowFile(task.path), 0);
    EXPECT_EQ(rmdir(tempDir.c_str()), 0);
}
} // namespace OHOS::Media

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
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fuzzer/FuzzedDataProvider.h>

#include "rdb_types.h"
#include "rdb_errno.h"
#include "recovery_bucket_index.h"
#include "recovery_event_limiter.h"
#include "recovery_summary.h"
#include "recovery_window.h"
#include "recovery_json_utils.h"
#include "replica_recovery_policy.h"

namespace {
constexpr int64_t VALID_VERSION = 1;
constexpr int64_t VALID_SIZE = 4096;
constexpr int64_t VALID_TIME = 1000000;
constexpr int64_t MAX_REPLICA_GAP_MS = 72LL * 60 * 60 * 1000;
constexpr int BUCKET_SAMPLES = 3;
constexpr size_t MAX_BUCKET_NAME = 64;

void FillReplicaInfo(FuzzedDataProvider &input, OHOS::NativeRdb::PrimaryAndReplicaInfo &info)
{
    info.primaryInfo.dbInfo.size = input.ConsumeIntegral<int64_t>();
    info.replicaInfo.dbInfo.size = input.ConsumeIntegral<int64_t>();
    info.primaryInfo.userVersion = input.ConsumeIntegral<int64_t>();
    info.replicaInfo.userVersion = input.ConsumeIntegral<int64_t>();
    info.primaryInfo.dbInfo.mtime = input.ConsumeIntegral<int64_t>();
    info.primaryInfo.walInfo.mtime = input.ConsumeIntegral<int64_t>();
    info.replicaInfo.dbInfo.mtime = input.ConsumeIntegral<int64_t>();
    info.binlogInfo.exist = input.ConsumeBool();
    info.binlogInfo.lastModifyTime = input.ConsumeIntegral<int64_t>();
    for (const auto &table : {"Photos", "PhotoAlbum"}) {
        info.primaryInfo.tables[table].count = input.ConsumeIntegral<int64_t>();
        info.replicaInfo.tables[table].count = input.ConsumeIntegral<int64_t>();
    }
}

void MakeValidReplicaInfo(OHOS::NativeRdb::PrimaryAndReplicaInfo &info)
{
    info.primaryInfo.dbInfo.size = info.replicaInfo.dbInfo.size = VALID_SIZE;
    info.primaryInfo.userVersion = info.replicaInfo.userVersion = VALID_VERSION;
    info.primaryInfo.dbInfo.mtime = info.primaryInfo.walInfo.mtime = VALID_TIME;
    info.binlogInfo.exist = true;
    info.binlogInfo.lastModifyTime = VALID_TIME;
    for (const auto &table : {"Photos", "PhotoAlbum"}) {
        info.primaryInfo.tables[table].count = info.replicaInfo.tables[table].count = 1;
    }
}

void FuzzDiagnosisAndForce(FuzzedDataProvider &input)
{
    OHOS::NativeRdb::PrimaryAndReplicaInfo info;
    FillReplicaInfo(input, info);
    int32_t diagnosticError = input.ConsumeIntegral<int32_t>();
    int64_t expectedVersion = input.ConsumeIntegral<int64_t>();
    if (input.ConsumeBool()) {
        MakeValidReplicaInfo(info);
        diagnosticError = 0;
        expectedVersion = VALID_VERSION;
    }
    auto decision = OHOS::Media::CheckReplicaRecovery(diagnosticError, info, expectedVersion);
    const bool eligible = decision == OHOS::Media::ReplicaRecoveryDecision::ALLOW;
    if (eligible) {
        const int64_t primaryTime = info.GetPrimaryLastModifyTime();
        const int64_t replicaTime = info.GetReplicaLastModifyTime();
        if (primaryTime <= 0 || replicaTime <= 0 ||
            std::max(primaryTime, replicaTime) - std::min(primaryTime, replicaTime) > MAX_REPLICA_GAP_MS) {
            __builtin_trap();
        }
    }
    const int error = input.ConsumeIntegral<int>();
    int forceCalls = 0;
    auto registerWindow = [error]() {
        OHOS::Media::RecoveryWindowResult result;
        result.error = error;
        result.path = "test-window";
        return result;
    };
    auto force = [&forceCalls]() {
        ++forceCalls;
        return 0;
    };
    const auto result = OHOS::Media::RunForceRecovery(eligible, registerWindow, force);
    if (result.forceCalled != (eligible && error == 0) || forceCalls != static_cast<int>(result.forceCalled)) {
        __builtin_trap();
    }
}

void FuzzWindow(FuzzedDataProvider &input)
{
    OHOS::Media::RecoveryWindow window;
    const std::string name = input.ConsumeRandomLengthString(128);
    if (OHOS::Media::ParseRecoveryWindowName(name, window) &&
        (window.startMs <= 0 || window.endMs < window.startMs)) {
        __builtin_trap();
    }
    window.startMs = input.ConsumeIntegral<int64_t>();
    window.endMs = input.ConsumeIntegral<int64_t>();
    const int64_t mtime = input.ConsumeIntegral<int64_t>();
    const int64_t sizeOnDisk = input.ConsumeIntegral<int64_t>();
    const int64_t databaseTime = input.ConsumeIntegral<int64_t>();
    const int64_t databaseSize = input.ConsumeIntegral<int64_t>();
    const bool inWindow = OHOS::Media::IsInRecoveryWindow(mtime, window);
    const bool atOrAfterStart = OHOS::Media::IsAtOrAfterRecoveryStart(mtime, window);
    if ((!inWindow && OHOS::Media::ShouldApplyRecoveryJson(mtime, databaseTime, window)) ||
        (!atOrAfterStart &&
        OHOS::Media::ShouldScanRecoveryFile(mtime, sizeOnDisk, databaseTime, databaseSize, window))) {
        __builtin_trap();
    }
}

void FuzzForceReport(FuzzedDataProvider &input)
{
    OHOS::Media::DatabaseRecoveryReport report;
    int failures = 0;
    for (int i = 0; i < BUCKET_SAMPLES; ++i) {
        const int32_t error = input.ConsumeIntegral<int32_t>();
        const int64_t elapsed = input.ConsumeIntegral<int64_t>();
        OHOS::Media::RecordForceAttempt(report, error, elapsed);
        failures += error != 0 ? 1 : 0;
        if (report.forceAttempts != i + 1 || report.forceFailures != failures ||
            report.waitForRestore != (error != 0) || report.forceMs < 0) {
            __builtin_trap();
        }
    }
}

void FuzzBackgroundResult(FuzzedDataProvider &input)
{
    const int64_t failures = input.ConsumeIntegralInRange<int64_t>(0, INT64_MAX);
    auto result = OHOS::Media::GetBackgroundRecoveryResult(OHOS::NativeRdb::E_OK, failures);
    const auto expected = failures > 0 ? OHOS::Media::DatabaseRecoveryResult::PARTIAL :
        OHOS::Media::DatabaseRecoveryResult::SUCCESS;
    if (result != expected || OHOS::Media::GetBackgroundRecoveryResult(OHOS::NativeRdb::E_CANCEL, failures) !=
        OHOS::Media::DatabaseRecoveryResult::PAUSED) {
        __builtin_trap();
    }
}

void FuzzBucket(FuzzedDataProvider &input)
{
    for (int i = 0; i < BUCKET_SAMPLES; ++i) {
        int64_t recordedMs = 0;
        std::string jsonName;
        const std::string marker = input.ConsumeRandomLengthString(MAX_BUCKET_NAME);
        if (OHOS::Media::ParseRecoveryBucketMarker(marker, recordedMs, jsonName) &&
            (!OHOS::Media::IsRecoveryBucketMarker(marker) || recordedMs < 0 || jsonName.empty() ||
            jsonName.find('/') != std::string::npos)) {
            __builtin_trap();
        }
    }
    const int64_t expectedMs = input.ConsumeIntegralInRange<int64_t>(0, INT64_MAX);
    const std::string validMarker = std::string(OHOS::Media::RECOVERY_BUCKET_MARKER_PREFIX) +
        std::to_string(expectedMs) + "_photo.jpg.json.idx";
    int64_t parsedMs = -1;
    std::string parsedName;
    if (!OHOS::Media::ParseRecoveryBucketMarker(validMarker, parsedMs, parsedName) ||
        parsedMs != expectedMs || parsedName != "photo.jpg.json") {
        __builtin_trap();
    }
}

void FuzzJson(FuzzedDataProvider &input)
{
    const auto remaining = input.ConsumeRemainingBytesAsString();
    OHOS::Media::DatabaseRecoveryReport report;
    (void)OHOS::Media::DecodeRecoverySummary(remaining, report);
    const auto value = nlohmann::json::parse(remaining, nullptr, false);
    if (!value.is_discarded()) {
        OHOS::NativeRdb::ValuesBucket values;
        for (const auto &type : {"BIGINT", "TEXT", "REAL"}) {
            const bool written = OHOS::Media::PutRecoveryJsonValue(values, type, value, type);
            if (written != values.HasColumn(type)) {
                __builtin_trap();
            }
        }
    }
}

void FuzzBucketProgress(FuzzedDataProvider &input)
{
    nlohmann::json summary = {{"window", "recovery_task_100_200"}, {"date", ""}};
    std::string bucket;
    const size_t count = input.ConsumeIntegralInRange<size_t>(0, 32);
    for (size_t i = 0; i < count; ++i) {
        bucket.push_back(static_cast<char>('a' + input.ConsumeIntegralInRange<int>(0, 'z' - 'a')));
    }
    summary["json_buckets_done"] = nlohmann::json::array({bucket});
    summary["file_buckets_done"] = nlohmann::json::array({bucket});
    OHOS::Media::DatabaseRecoveryReport report;
    if (!OHOS::Media::DecodeRecoverySummary(summary.dump(), report) ||
        report.jsonBucketsDone.count(bucket) != 1 || report.fileBucketsDone.count(bucket) != 1) {
        __builtin_trap();
    }
    summary["json_buckets_done"] = input.ConsumeIntegral<int32_t>();
    if (OHOS::Media::DecodeRecoverySummary(summary.dump(), report)) {
        __builtin_trap();
    }
}

void FuzzEventClock(FuzzedDataProvider &input)
{
    constexpr int64_t hourMs = 60LL * 60 * 1000;
    const int64_t past = input.ConsumeIntegralInRange<int64_t>(0, INT64_MAX - hourMs);
    int64_t now = past + hourMs;
    if (OHOS::Media::IsRecoveryEventRecent(past, now)) {
        __builtin_trap();
    }
    --now;
    if (!OHOS::Media::IsRecoveryEventRecent(past, now)) {
        __builtin_trap();
    }
    now = input.ConsumeIntegral<int64_t>();
    (void)OHOS::Media::IsRecoveryEventRecent(past, now);
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider input(data, size);
    FuzzDiagnosisAndForce(input);
    FuzzForceReport(input);
    FuzzBackgroundResult(input);
    FuzzWindow(input);
    FuzzBucket(input);
    FuzzEventClock(input);
    FuzzBucketProgress(input);
    FuzzJson(input);
    return 0;
}

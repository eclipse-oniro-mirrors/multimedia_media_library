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
#include "replica_recovery_policy.h"
#include <algorithm>
#include "rdb_errno.h"
#include "rdb_types.h"

namespace OHOS::Media {
namespace {
constexpr int64_t MAX_REPLICA_GAP_MS = 72LL * 60 * 60 * 1000;
constexpr int64_t MAX_REPLICA_ROW_GAP = 1000;
ReplicaRecoveryDecision CheckTableCounts(const NativeRdb::PrimaryAndReplicaInfo &info)
{
    for (const auto &table : {"Photos", "PhotoAlbum"}) {
        const auto primary = info.primaryInfo.tables.find(table);
        const auto replica = info.replicaInfo.tables.find(table);
        if (primary == info.primaryInfo.tables.end() || replica == info.replicaInfo.tables.end() ||
            primary->second.count <= 0 || replica->second.count <= 0) {
            // A zero count can be an empty table or an unfilled default.
            return ReplicaRecoveryDecision::TABLE_INFO_UNAVAILABLE;
        }
        const int64_t primaryCount = primary->second.count;
        const int64_t replicaCount = replica->second.count;
        if (std::max(primaryCount, replicaCount) - std::min(primaryCount, replicaCount) > MAX_REPLICA_ROW_GAP) {
            return ReplicaRecoveryDecision::ROW_GAP_TOO_LARGE;
        }
    }
    return ReplicaRecoveryDecision::ALLOW;
}
} // namespace

ReplicaRecoveryDecision CheckReplicaRecovery(int32_t error, const NativeRdb::PrimaryAndReplicaInfo &info,
    int64_t expectedVersion)
{
    if (error != NativeRdb::E_OK) {
        return ReplicaRecoveryDecision::DIAGNOSIS_UNAVAILABLE;
    }
    if (info.primaryInfo.dbInfo.size <= 0 || info.replicaInfo.dbInfo.size <= 0) {
        return ReplicaRecoveryDecision::FILE_INFO_UNAVAILABLE;
    }
    const auto tableDecision = CheckTableCounts(info);
    if (tableDecision != ReplicaRecoveryDecision::ALLOW) {
        return tableDecision;
    }
    if (expectedVersion <= 0 || info.primaryInfo.userVersion <= 0 ||
        info.primaryInfo.userVersion != info.replicaInfo.userVersion ||
        info.replicaInfo.userVersion != expectedVersion) {
        return ReplicaRecoveryDecision::VERSION_MISMATCH;
    }
    // ArkData may leave WAL or writeIndex fields at their defaults when stat fails.
    // Without validity flags, absent files and failed reads cannot be distinguished.
    if (info.primaryInfo.dbInfo.mtime <= 0 || info.primaryInfo.walInfo.mtime <= 0 ||
        !info.binlogInfo.exist || info.binlogInfo.lastModifyTime <= 0) {
        return ReplicaRecoveryDecision::INVALID_TIME;
    }
    const int64_t primaryTime = info.GetPrimaryLastModifyTime();
    const int64_t replicaTime = info.GetReplicaLastModifyTime();
    if (primaryTime <= 0 || replicaTime <= 0) {
        return ReplicaRecoveryDecision::INVALID_TIME;
    }
    if (replicaTime > primaryTime) {
        return ReplicaRecoveryDecision::REPLICA_NEWER;
    }
    const int64_t gap = primaryTime - replicaTime;
    return gap <= MAX_REPLICA_GAP_MS ? ReplicaRecoveryDecision::ALLOW : ReplicaRecoveryDecision::TOO_OLD;
}
} // namespace OHOS::Media

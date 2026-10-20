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

#include "preferences.h"
#include "rdb_errno.h"
#include "rdb_types.h"

namespace OHOS::Media {
int64_t GetRecoveryTableCount(const NativeRdb::PrimaryAndReplicaInfo &info, bool primary, const std::string &table)
{
    const auto &tables = primary ? info.primaryInfo.tables : info.replicaInfo.tables;
    auto it = tables.find(table);
    return it == tables.end() ? -1 : it->second.count;
}

ReplicaRecoveryDecision CheckReplicaRecovery(int32_t error, const NativeRdb::PrimaryAndReplicaInfo &info)
{
    if (error != NativeRdb::E_OK) {
        return ReplicaRecoveryDecision::DIAGNOSIS_UNAVAILABLE;
    }
    int64_t primarySize = info.primaryInfo.dbInfo.size;
    int64_t replicaSize = info.replicaInfo.dbInfo.size;
    if (primarySize <= 0 || replicaSize <= 0) {
        return ReplicaRecoveryDecision::FILE_INFO_UNAVAILABLE;
    }
    for (const auto &table : {"Photos", "PhotoAlbum"}) {
        int64_t primary = GetRecoveryTableCount(info, true, table);
        int64_t replica = GetRecoveryTableCount(info, false, table);
        if (primary < 0 || replica < 0) {
            return ReplicaRecoveryDecision::TABLE_INFO_UNAVAILABLE;
        }
        if (std::max(primary, replica) - std::min(primary, replica) > MAX_REPLICA_ROW_GAP) {
            return ReplicaRecoveryDecision::ROW_GAP_TOO_LARGE;
        }
    }
    if (info.primaryInfo.userVersion <= 0 || info.primaryInfo.userVersion != info.replicaInfo.userVersion) {
        return ReplicaRecoveryDecision::VERSION_MISMATCH;
    }
    int64_t primaryTime = info.GetPrimaryLastModifyTime();
    int64_t replicaTime = info.GetReplicaLastModifyTime();
    if (replicaTime <= 0 || primaryTime <= 0) {
        return ReplicaRecoveryDecision::INVALID_TIME;
    }
    // ArkData file/binlog times are not directional commit watermarks.
    int64_t gap = std::max(primaryTime, replicaTime) - std::min(primaryTime, replicaTime);
    return gap <= MAX_REPLICA_GAP_MS ? ReplicaRecoveryDecision::ALLOW : ReplicaRecoveryDecision::TOO_OLD;
}

bool IsReplicaRecoveryBusy(int32_t error)
{
    return error == NativeRdb::E_DATABASE_BUSY || error == NativeRdb::E_SQLITE_BUSY ||
        error == NativeRdb::E_SQLITE_LOCKED;
}

bool ShouldPauseRecoveryScan(int32_t error)
{
    return IsReplicaRecoveryBusy(error) || error == NativeRdb::E_SQLITE_CORRUPT ||
        error == NativeRdb::E_SQLITE_IOERR || error == NativeRdb::E_ALREADY_CLOSED ||
        error == NativeRdb::E_SQLITE_FULL || error == NativeRdb::E_SQLITE_READONLY ||
        error == NativeRdb::E_SQLITE_CANTOPEN || error == NativeRdb::E_SQLITE_IOERR_FULL ||
        error == NativeRdb::E_WAL_SIZE_OVER_LIMIT || error == NativeRdb::E_SQLITE_NOMEM;
}

bool IsRecoveryFileIdentityValid(int64_t size, int64_t modifiedMs, int64_t fileSize, int64_t fileSeconds)
{
    constexpr int64_t millisecondsPerSecond = 1000;
    return size > 0 && size == fileSize && modifiedMs > 0 && fileSeconds > 0 &&
        modifiedMs / millisecondsPerSecond == fileSeconds;
}

bool HasReplicaRepairTarget(NativePreferences::Preferences &prefs, const std::string &path, int64_t version)
{
    return !path.empty() && version > 0 && prefs.GetBool(REPLICA_REPAIR_PENDING, false) &&
        prefs.GetString(REPLICA_REPAIR_DB_PATH, "") == path && prefs.GetLong(REPLICA_REPAIR_VERSION, 0) == version;
}

int32_t SetRecoveryFlag(NativePreferences::Preferences &prefs, const std::string &key, bool value)
{
    bool previous = prefs.GetBool(key, false);
    if (prefs.PutBool(key, value) != 0) {
        return NativeRdb::E_ERROR;
    }
    if (prefs.FlushSync() == 0) {
        return NativeRdb::E_OK;
    }
    (void)prefs.PutBool(key, previous); // Preferences updates its shared cache before durable flush.
    return NativeRdb::E_ERROR;
}

int32_t PrepareRecoveryRequest(NativePreferences::Preferences &prefs, const DatabaseRecoveryRequest &request)
{
    if (request.path.empty() || request.version <= 0 || request.date.empty()) {
        return NativeRdb::E_INVALID_ARGS;
    }
    if (prefs.GetBool(REPLICA_REPAIR_PENDING, false) &&
        !HasReplicaRepairTarget(prefs, request.path, request.version)) {
        return NativeRdb::E_ERROR;
    }
    DatabaseRecoveryRequest previous {prefs.GetString(REPLICA_REPAIR_DB_PATH, ""),
        prefs.GetLong(REPLICA_REPAIR_VERSION, 0), prefs.GetString(REPLICA_RECOVERY_DATE, "")};
    bool pending = prefs.GetBool(REPLICA_REPAIR_PENDING, false);
    if (prefs.PutString(REPLICA_RECOVERY_DATE, request.date) != 0 ||
        prefs.PutString(REPLICA_REPAIR_DB_PATH, request.path) != 0 ||
        prefs.PutLong(REPLICA_REPAIR_VERSION, request.version) != 0 ||
        prefs.PutBool(REPLICA_REPAIR_PENDING, true) != 0 || prefs.FlushSync() != 0) {
        // A later flush must not publish a partly changed target from the shared Preferences cache.
        (void)prefs.PutString(REPLICA_RECOVERY_DATE, previous.date);
        (void)prefs.PutString(REPLICA_REPAIR_DB_PATH, previous.path);
        (void)prefs.PutLong(REPLICA_REPAIR_VERSION, previous.version);
        (void)prefs.PutBool(REPLICA_REPAIR_PENDING, pending);
        return NativeRdb::E_ERROR;
    }
    return NativeRdb::E_OK;
}

} // namespace OHOS::Media

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
#ifndef OHOS_MEDIA_REPLICA_RECOVERY_POLICY_H
#define OHOS_MEDIA_REPLICA_RECOVERY_POLICY_H

#include <cstdint>
#include <string>

namespace OHOS {
namespace NativePreferences { class Preferences; }
namespace NativeRdb { struct PrimaryAndReplicaInfo; }
namespace Media {
constexpr int64_t MAX_REPLICA_GAP_MS = 72LL * 60 * 60 * 1000;
constexpr int64_t MAX_REPLICA_ROW_GAP = 1000;
inline const std::string REPLICA_REPAIR_PENDING = "REPLICA_REPAIR_PENDING";
inline const std::string REPLICA_REPAIR_DB_PATH = "REPLICA_REPAIR_DB_PATH";
inline const std::string REPLICA_REPAIR_VERSION = "REPLICA_REPAIR_VERSION";
inline const std::string REPLICA_RECOVERY_DATE = "REPLICA_RECOVERY_DATE";

enum class ReplicaRecoveryDecision : int32_t {
    ALLOW, DIAGNOSIS_UNAVAILABLE, VERSION_MISMATCH, INVALID_TIME, TOO_OLD,
    FILE_INFO_UNAVAILABLE, TABLE_INFO_UNAVAILABLE,
    ROW_GAP_TOO_LARGE,
};

int64_t GetRecoveryTableCount(const NativeRdb::PrimaryAndReplicaInfo &info, bool primary, const std::string &table);
ReplicaRecoveryDecision CheckReplicaRecovery(int32_t error, const NativeRdb::PrimaryAndReplicaInfo &info);
bool IsReplicaRecoveryBusy(int32_t error);
bool ShouldPauseRecoveryScan(int32_t error);
bool IsRecoveryFileIdentityValid(int64_t size, int64_t modifiedMs, int64_t fileSize, int64_t fileSeconds);

struct DatabaseRecoveryRequest {
    std::string path;
    int64_t version = 0;
    std::string date;
};

bool HasReplicaRepairTarget(NativePreferences::Preferences &prefs, const std::string &path, int64_t version);
int32_t PrepareRecoveryRequest(NativePreferences::Preferences &prefs, const DatabaseRecoveryRequest &request);
int32_t SetRecoveryFlag(NativePreferences::Preferences &prefs, const std::string &key, bool value);
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIA_REPLICA_RECOVERY_POLICY_H

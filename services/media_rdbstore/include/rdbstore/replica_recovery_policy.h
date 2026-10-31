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
namespace OHOS {
namespace NativeRdb { struct PrimaryAndReplicaInfo; }
namespace Media {
enum class ReplicaRecoveryDecision : int32_t {
    ALLOW, DIAGNOSIS_UNAVAILABLE, VERSION_MISMATCH, INVALID_TIME, TOO_OLD,
    FILE_INFO_UNAVAILABLE, TABLE_INFO_UNAVAILABLE, ROW_GAP_TOO_LARGE, REPLICA_NEWER,
};
// A rejected diagnosis prevents window registration and ForceRestore.
ReplicaRecoveryDecision CheckReplicaRecovery(int32_t error, const NativeRdb::PrimaryAndReplicaInfo &info,
    int64_t expectedVersion);
}
}
#endif

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
#include <limits>
#include "gtest/gtest.h"
#include "rdb_errno.h"
#include "rdb_types.h"
#include "replica_recovery_policy.h"

namespace OHOS::Media {
using namespace testing::ext;
class ReplicaRecoveryPolicyTest : public testing::Test {};
namespace {
constexpr int64_t TEST_VERSION = 100;
constexpr int64_t TEST_DB_SIZE = 4096;
constexpr int64_t TEST_TIME = 1000000000;
constexpr int64_t MAX_GAP_MS = 72LL * 60 * 60 * 1000;
constexpr int64_t MAX_ROW_GAP = 1000;
NativeRdb::PrimaryAndReplicaInfo ValidDiagnosis()
{
    NativeRdb::PrimaryAndReplicaInfo info;
    info.primaryInfo.userVersion = info.replicaInfo.userVersion = TEST_VERSION;
    info.primaryInfo.dbInfo.size = info.replicaInfo.dbInfo.size = TEST_DB_SIZE;
    info.primaryInfo.dbInfo.mtime = info.replicaInfo.dbInfo.mtime = TEST_TIME;
    info.primaryInfo.walInfo.mtime = TEST_TIME;
    info.binlogInfo.exist = true;
    info.binlogInfo.lastModifyTime = TEST_TIME;
    info.primaryInfo.tables["Photos"].count = info.primaryInfo.tables["PhotoAlbum"].count = 1;
    info.replicaInfo.tables = info.primaryInfo.tables;
    return info;
}
}

HWTEST_F(ReplicaRecoveryPolicyTest, DiagnosisBoundaries, TestSize.Level1)
{
    auto info = ValidDiagnosis();
    info.primaryInfo.tables["Photos"].count += MAX_ROW_GAP;
    info.primaryInfo.dbInfo.mtime += MAX_GAP_MS;
    EXPECT_EQ(CheckReplicaRecovery(NativeRdb::E_OK, info, TEST_VERSION), ReplicaRecoveryDecision::ALLOW);
    ++info.primaryInfo.tables["Photos"].count;
    EXPECT_EQ(CheckReplicaRecovery(NativeRdb::E_OK, info, TEST_VERSION), ReplicaRecoveryDecision::ROW_GAP_TOO_LARGE);
    --info.primaryInfo.tables["Photos"].count;
    ++info.primaryInfo.dbInfo.mtime;
    EXPECT_EQ(CheckReplicaRecovery(NativeRdb::E_OK, info, TEST_VERSION), ReplicaRecoveryDecision::TOO_OLD);
}

HWTEST_F(ReplicaRecoveryPolicyTest, DiagnosisUnavailableOrDefault, TestSize.Level1)
{
    auto info = ValidDiagnosis();
    EXPECT_EQ(CheckReplicaRecovery(NativeRdb::E_ERROR, info, TEST_VERSION),
        ReplicaRecoveryDecision::DIAGNOSIS_UNAVAILABLE);
    info.primaryInfo.tables["Photos"].count = 0;
    EXPECT_EQ(CheckReplicaRecovery(NativeRdb::E_OK, info, TEST_VERSION),
        ReplicaRecoveryDecision::TABLE_INFO_UNAVAILABLE);
    info = ValidDiagnosis();
    info.replicaInfo.tables.erase("PhotoAlbum");
    EXPECT_EQ(CheckReplicaRecovery(NativeRdb::E_OK, info, TEST_VERSION),
        ReplicaRecoveryDecision::TABLE_INFO_UNAVAILABLE);
    info = ValidDiagnosis();
    info.replicaInfo.tables["PhotoAlbum"].count = 0;
    EXPECT_EQ(CheckReplicaRecovery(NativeRdb::E_OK, info, TEST_VERSION),
        ReplicaRecoveryDecision::TABLE_INFO_UNAVAILABLE);
    info = ValidDiagnosis();
    info.replicaInfo.dbInfo.size = 0;
    EXPECT_EQ(CheckReplicaRecovery(NativeRdb::E_OK, info, TEST_VERSION),
        ReplicaRecoveryDecision::FILE_INFO_UNAVAILABLE);
    info = ValidDiagnosis();
    info.replicaInfo.userVersion = 0;
    EXPECT_EQ(CheckReplicaRecovery(NativeRdb::E_OK, info, TEST_VERSION), ReplicaRecoveryDecision::VERSION_MISMATCH);
    info = ValidDiagnosis();
    info.primaryInfo.dbInfo.mtime = 0;
    EXPECT_EQ(CheckReplicaRecovery(NativeRdb::E_OK, info, TEST_VERSION), ReplicaRecoveryDecision::INVALID_TIME);
    info = ValidDiagnosis();
    info.primaryInfo.walInfo.mtime = 0;
    EXPECT_EQ(CheckReplicaRecovery(NativeRdb::E_OK, info, TEST_VERSION), ReplicaRecoveryDecision::INVALID_TIME);
    info = ValidDiagnosis();
    info.binlogInfo.exist = false;
    EXPECT_EQ(CheckReplicaRecovery(NativeRdb::E_OK, info, TEST_VERSION), ReplicaRecoveryDecision::INVALID_TIME);
    info = ValidDiagnosis();
    info.binlogInfo.exist = true;
    info.binlogInfo.lastModifyTime = 0;
    EXPECT_EQ(CheckReplicaRecovery(NativeRdb::E_OK, info, TEST_VERSION), ReplicaRecoveryDecision::INVALID_TIME);
}

HWTEST_F(ReplicaRecoveryPolicyTest, DiagnosisExtremeCountAndReplicaTime, TestSize.Level1)
{
    auto info = ValidDiagnosis();
    info.replicaInfo.tables["PhotoAlbum"].count = std::numeric_limits<int64_t>::max();
    EXPECT_EQ(CheckReplicaRecovery(NativeRdb::E_OK, info, TEST_VERSION), ReplicaRecoveryDecision::ROW_GAP_TOO_LARGE);
    const int64_t gaps[] = {0, 1, 284, 5720, MAX_GAP_MS, MAX_GAP_MS + 1};
    for (int64_t gap : gaps) {
        for (bool replicaNewer : {false, true}) {
            info = ValidDiagnosis();
            if (replicaNewer) {
                info.binlogInfo.lastModifyTime += gap;
            } else {
                info.primaryInfo.dbInfo.mtime += gap;
            }
            const auto expected = gap <= MAX_GAP_MS ? ReplicaRecoveryDecision::ALLOW :
                ReplicaRecoveryDecision::TOO_OLD;
            EXPECT_EQ(CheckReplicaRecovery(NativeRdb::E_OK, info, TEST_VERSION), expected);
        }
    }
}
} // namespace OHOS::Media

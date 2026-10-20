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
#include <functional>
#include <map>
#include <variant>

#include "gtest/gtest.h"
#include "preferences.h"
#include "rdb_errno.h"
#include "rdb_types.h"
#include "replica_recovery_policy.h"

namespace OHOS::Media {
using namespace testing::ext;
class ReplicaRecoveryPolicyTest : public testing::Test {};
namespace {
constexpr int64_t TEST_VERSION = 100;
constexpr int64_t TEST_DATABASE_FILE_SIZE = 4096;
constexpr int PREPARATION_WRITES = 4;
const std::string TEST_DATABASE = "/data/test/recovery.db";
using PrefValue = std::variant<bool, int64_t, std::string>;

// Implements the real Preferences interface; cache and disk are deliberately distinct.
class RequestPreferences final : public NativePreferences::Preferences {
public:
    std::map<std::string, PrefValue> values, durable;
    int writes = 0, flushes = 0, failWrite = -1, failFlush = -1;
    template<typename T> T Read(const std::string &key, T fallback)
    {
        auto it = values.find(key);
        auto value = it == values.end() ? nullptr : std::get_if<T>(&it->second);
        return value == nullptr ? fallback : *value;
    }
    template<typename T> int Write(const std::string &key, T value)
    {
        if (++writes == failWrite) {
            return -1;
        }
        values[key] = value;
        return 0;
    }
    bool GetBool(const std::string &key, const bool &value) override { return Read(key, value); }
    int64_t GetLong(const std::string &key, const int64_t &value) override { return Read(key, value); }
    std::string GetString(const std::string &key, const std::string &value) override { return Read(key, value); }
    int PutBool(const std::string &key, bool value) override { return Write(key, value); }
    int PutLong(const std::string &key, int64_t value) override { return Write(key, value); }
    int PutString(const std::string &key, const std::string &value) override { return Write(key, value); }
    int FlushSync() override
    {
        if (++flushes == failFlush) {
            return -1;
        }
        durable = values;
        return 0;
    }
    NativePreferences::PreferencesValue Get(const std::string &,
        const NativePreferences::PreferencesValue &) override { return {}; }
    int Put(const std::string &, const NativePreferences::PreferencesValue &) override { return -1; }
    int GetInt(const std::string &, const int &value) override { return value; }
    float GetFloat(const std::string &, const float &value) override { return value; }
    double GetDouble(const std::string &, const double &value) override { return value; }
    std::map<std::string, NativePreferences::PreferencesValue> GetAll() override { return {}; }
    bool HasKey(const std::string &key) override { return values.count(key) != 0; }
    int PutInt(const std::string &, int) override { return -1; }
    int PutFloat(const std::string &, float) override { return -1; }
    int PutDouble(const std::string &, double) override { return -1; }
    int Delete(const std::string &) override { return -1; }
    int Clear() override { return -1; }
    void Flush() override {}
    int RegisterObserver(std::shared_ptr<NativePreferences::PreferencesObserver>,
        NativePreferences::RegisterMode) override { return -1; }
    int UnRegisterObserver(std::shared_ptr<NativePreferences::PreferencesObserver>,
        NativePreferences::RegisterMode) override { return -1; }
};

NativeRdb::PrimaryAndReplicaInfo ValidDiagnosis()
{
    NativeRdb::PrimaryAndReplicaInfo info;
    info.primaryInfo.userVersion = info.replicaInfo.userVersion = TEST_VERSION;
    info.primaryInfo.dbInfo.size = info.replicaInfo.dbInfo.size = TEST_DATABASE_FILE_SIZE;
    info.primaryInfo.dbInfo.mtime = info.replicaInfo.dbInfo.mtime = 1;
    info.primaryInfo.tables["Photos"].count = info.primaryInfo.tables["PhotoAlbum"].count = 0;
    info.replicaInfo.tables = info.primaryInfo.tables;
    return info;
}

void CheckRequestRollback(int fail)
{
    RequestPreferences prefs;
    ASSERT_EQ(PrepareRecoveryRequest(prefs, {TEST_DATABASE, TEST_VERSION, "old-date"}), NativeRdb::E_OK);
    prefs.failWrite = fail <= PREPARATION_WRITES ? prefs.writes + fail : -1;
    prefs.failFlush = fail > PREPARATION_WRITES ? prefs.flushes + 1 : -1;
    EXPECT_NE(PrepareRecoveryRequest(prefs, {TEST_DATABASE, TEST_VERSION, "new-date"}), NativeRdb::E_OK);
    EXPECT_TRUE(HasReplicaRepairTarget(prefs, TEST_DATABASE, TEST_VERSION));
    EXPECT_EQ(prefs.GetString(REPLICA_RECOVERY_DATE, ""), "old-date");
    prefs.failWrite = prefs.failFlush = -1;
    ASSERT_EQ(prefs.FlushSync(), 0);
    prefs.values = prefs.durable;
    EXPECT_TRUE(HasReplicaRepairTarget(prefs, TEST_DATABASE, TEST_VERSION));
    EXPECT_EQ(prefs.GetString(REPLICA_RECOVERY_DATE, ""), "old-date");
}
}

/** @tc.desc: Inclusive 72h/per-table 1000-row gates; valid file sizes do not impose a size-gap limit. */
HWTEST_F(ReplicaRecoveryPolicyTest, RecoveryDiagnosis_Boundaries, TestSize.Level1)
{
    auto info = ValidDiagnosis();
    EXPECT_EQ(CheckReplicaRecovery(-1, info), ReplicaRecoveryDecision::DIAGNOSIS_UNAVAILABLE);
    for (bool primary : {false, true}) {
        for (const auto &table : {"Photos", "PhotoAlbum"}) {
            for (int64_t gap : {int64_t{1000}, int64_t{1001}, std::numeric_limits<int64_t>::max(), int64_t{-1}}) {
                info = ValidDiagnosis();
                (primary ? info.primaryInfo : info.replicaInfo).tables[table].count = gap;
                EXPECT_EQ(CheckReplicaRecovery(0, info) == ReplicaRecoveryDecision::ALLOW, gap == 1000);
            }
        }
        constexpr int64_t formerSizeLimit = 4LL * 1024 * 1024;
        for (int64_t gap : {formerSizeLimit, formerSizeLimit + 1}) {
            info = ValidDiagnosis();
            (primary ? info.primaryInfo : info.replicaInfo).dbInfo.size += gap;
            EXPECT_EQ(CheckReplicaRecovery(0, info), ReplicaRecoveryDecision::ALLOW);
        }
        info = ValidDiagnosis();
        (primary ? info.primaryInfo : info.replicaInfo).dbInfo.mtime += MAX_REPLICA_GAP_MS;
        EXPECT_EQ(CheckReplicaRecovery(0, info), ReplicaRecoveryDecision::ALLOW);
        ++(primary ? info.primaryInfo : info.replicaInfo).dbInfo.mtime;
        EXPECT_EQ(CheckReplicaRecovery(0, info), ReplicaRecoveryDecision::TOO_OLD);
    }
    const std::vector<std::function<void(NativeRdb::PrimaryAndReplicaInfo &)>> invalid = {
        [](auto &value) { value.replicaInfo.userVersion = TEST_VERSION + 1; },
        [](auto &value) { value.primaryInfo.dbInfo.size = 0; },
        [](auto &value) { value.replicaInfo.tables.erase("Photos"); },
        [](auto &value) { value.primaryInfo.dbInfo.mtime = 0; },
        [](auto &value) { value.replicaInfo.userVersion = 0; }};
    for (const auto &mutate : invalid) {
        info = ValidDiagnosis();
        mutate(info);
        EXPECT_NE(CheckReplicaRecovery(0, info), ReplicaRecoveryDecision::ALLOW);
    }
    info = ValidDiagnosis();
    info.primaryInfo.walInfo.mtime = 2 + MAX_REPLICA_GAP_MS;
    EXPECT_EQ(CheckReplicaRecovery(0, info), ReplicaRecoveryDecision::TOO_OLD);
    info.binlogInfo.exist = true;
    info.binlogInfo.lastModifyTime = info.primaryInfo.walInfo.mtime;
    EXPECT_EQ(CheckReplicaRecovery(0, info), ReplicaRecoveryDecision::ALLOW);
}

/** @tc.desc: A single pending record rolls back failed cache updates and can be saved again. */
HWTEST_F(ReplicaRecoveryPolicyTest, RecoveryRequest_PersistenceFailures, TestSize.Level1)
{
    for (int fail = 1; fail <= PREPARATION_WRITES + 1; ++fail) {
        RequestPreferences prefs;
        prefs.failWrite = fail <= PREPARATION_WRITES ? fail : -1;
        prefs.failFlush = fail > PREPARATION_WRITES ? 1 : -1;
        EXPECT_EQ(PrepareRecoveryRequest(prefs, {TEST_DATABASE, TEST_VERSION, "date"}), NativeRdb::E_ERROR);
        EXPECT_FALSE(prefs.GetBool(REPLICA_REPAIR_PENDING, false));
        EXPECT_TRUE(prefs.GetString(REPLICA_REPAIR_DB_PATH, "").empty());
        EXPECT_EQ(prefs.GetLong(REPLICA_REPAIR_VERSION, 0), 0);
        EXPECT_TRUE(prefs.GetString(REPLICA_RECOVERY_DATE, "").empty());
        prefs.failWrite = prefs.failFlush = -1;
        ASSERT_EQ(PrepareRecoveryRequest(prefs, {TEST_DATABASE, TEST_VERSION, "date"}), NativeRdb::E_OK);
        EXPECT_TRUE(prefs.GetBool(REPLICA_REPAIR_PENDING, false));
    }
    for (int fail = 1; fail <= PREPARATION_WRITES + 1; ++fail) {
        CheckRequestRollback(fail);
    }
}

/** @tc.desc: Preserve another target, reject invalid identity, and clear only durably saved pending. */
HWTEST_F(ReplicaRecoveryPolicyTest, RecoveryRequest_TargetAndFlags, TestSize.Level1)
{
    RequestPreferences prefs;
    ASSERT_EQ(PrepareRecoveryRequest(prefs, {TEST_DATABASE, TEST_VERSION, "date"}), NativeRdb::E_OK);
    prefs.values = prefs.durable;
    EXPECT_TRUE(HasReplicaRepairTarget(prefs, TEST_DATABASE, TEST_VERSION));
    EXPECT_FALSE(HasReplicaRepairTarget(prefs, TEST_DATABASE + ".other", TEST_VERSION));
    EXPECT_FALSE(HasReplicaRepairTarget(prefs, TEST_DATABASE, TEST_VERSION + 1));
    EXPECT_EQ(PrepareRecoveryRequest(prefs, {TEST_DATABASE + ".other", TEST_VERSION, "date"}), NativeRdb::E_ERROR);
    EXPECT_EQ(PrepareRecoveryRequest(prefs, {TEST_DATABASE, TEST_VERSION + 1, "date"}), NativeRdb::E_ERROR);
    EXPECT_TRUE(HasReplicaRepairTarget(prefs, TEST_DATABASE, TEST_VERSION));
    for (const auto &invalid : std::vector<DatabaseRecoveryRequest>{{"", TEST_VERSION, "date"},
        {TEST_DATABASE, 0, "date"}, {TEST_DATABASE, TEST_VERSION, ""}}) {
        EXPECT_EQ(PrepareRecoveryRequest(prefs, invalid), NativeRdb::E_INVALID_ARGS);
    }
    for (bool flush : {false, true}) {
        RequestPreferences flags;
        ASSERT_EQ(PrepareRecoveryRequest(flags, {TEST_DATABASE, TEST_VERSION, "date"}), NativeRdb::E_OK);
        flags.failFlush = flush ? flags.flushes + 1 : -1;
        flags.failWrite = flush ? -1 : flags.writes + 1;
        EXPECT_NE(SetRecoveryFlag(flags, REPLICA_REPAIR_PENDING, false), NativeRdb::E_OK);
        EXPECT_TRUE(flags.GetBool(REPLICA_REPAIR_PENDING, false));
        flags.values = flags.durable;
        EXPECT_TRUE(flags.GetBool(REPLICA_REPAIR_PENDING, false));
    }
    ASSERT_EQ(SetRecoveryFlag(prefs, REPLICA_REPAIR_PENDING, false), NativeRdb::E_OK);
    EXPECT_FALSE(HasReplicaRepairTarget(prefs, TEST_DATABASE, TEST_VERSION));
}

/** @tc.desc: Busy/corruption pause rather than force incorrectly; committed identity remains authoritative. */
HWTEST_F(ReplicaRecoveryPolicyTest, RecoveryErrors_AndMetadataIdentity, TestSize.Level1)
{
    for (int error : {NativeRdb::E_DATABASE_BUSY, NativeRdb::E_SQLITE_BUSY, NativeRdb::E_SQLITE_LOCKED,
        NativeRdb::E_SQLITE_CORRUPT, NativeRdb::E_SQLITE_IOERR, NativeRdb::E_SQLITE_FULL,
        NativeRdb::E_ALREADY_CLOSED, NativeRdb::E_SQLITE_READONLY, NativeRdb::E_SQLITE_CANTOPEN,
        NativeRdb::E_SQLITE_IOERR_FULL, NativeRdb::E_WAL_SIZE_OVER_LIMIT, NativeRdb::E_SQLITE_NOMEM}) {
        EXPECT_TRUE(ShouldPauseRecoveryScan(error));
    }
    EXPECT_FALSE(ShouldPauseRecoveryScan(0));
    EXPECT_TRUE(IsRecoveryFileIdentityValid(100, 7999, 100, 7));
    EXPECT_FALSE(IsRecoveryFileIdentityValid(100, 8000, 100, 7));
    EXPECT_FALSE(IsRecoveryFileIdentityValid(100, 7999, 101, 7));
}

} // namespace OHOS::Media

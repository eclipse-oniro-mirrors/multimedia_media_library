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
#include <fstream>
#include <unistd.h>

#include "gtest/gtest.h"
#include "media_file_utils.h"
#include "recovery_event_limiter.h"

namespace OHOS::Media {
using namespace testing::ext;
namespace {
constexpr int64_t HOUR_MS = 60LL * 60 * 1000;
constexpr int64_t TEST_TIME = 100;
constexpr int32_t INVALID_EVENT_KIND = -1;
const std::string LIMIT_FILE = "/recovery_event_limit_DATABASE_CORRUPTION_ERROR";
const std::string INFO_FILE = "/recovery_event_limit_MEDIALIB_META_RECOVERY_INFO";
const std::string ERROR_FILE = "/recovery_event_limit_MEDIALIB_META_RECOVERY_ERROR";
const std::string LOCK_FILE = "/recovery_event_limit.lock";
}

class RecoveryEventLimiterTest : public testing::Test {
public:
    void SetUp() override
    {
        ASSERT_TRUE(MediaFileUtils::CreateDirectory("/data/test/media_library_restore"));
        char name[] = "/data/test/media_library_restore/event_XXXXXX";
        ASSERT_NE(mkdtemp(name), nullptr);
        directory_ = name;
    }
    void TearDown() override
    {
        if (!directory_.empty()) {
            unlink((directory_ + LIMIT_FILE).c_str());
            unlink((directory_ + INFO_FILE).c_str());
            unlink((directory_ + ERROR_FILE).c_str());
            unlink((directory_ + LOCK_FILE).c_str());
            EXPECT_EQ(rmdir(directory_.c_str()), 0);
        }
    }
    std::string directory_;
};

HWTEST_F(RecoveryEventLimiterTest, EventNamesAndInvalidKind, TestSize.Level1)
{
    EXPECT_STREQ(RecoveryEventName(RecoveryEventKind::DATABASE_CORRUPTION), "DATABASE_CORRUPTION_ERROR");
    EXPECT_STREQ(RecoveryEventName(RecoveryEventKind::META_RECOVERY_INFO), "MEDIALIB_META_RECOVERY_INFO");
    EXPECT_STREQ(RecoveryEventName(RecoveryEventKind::META_RECOVERY_ERROR), "MEDIALIB_META_RECOVERY_ERROR");
    const auto invalid = static_cast<RecoveryEventKind>(INVALID_EVENT_KIND);
    EXPECT_EQ(RecoveryEventName(invalid), nullptr);
    EXPECT_EQ(ReserveRecoveryEvent(directory_, invalid, TEST_TIME), EINVAL);
}

HWTEST_F(RecoveryEventLimiterTest, LimitsEachEventIndependentlyAndExpiresInRollingHour, TestSize.Level1)
{
    int64_t now = TEST_TIME;
    EXPECT_EQ(ReserveRecoveryEvent(directory_, RecoveryEventKind::DATABASE_CORRUPTION, now), 0);
    now++;
    EXPECT_EQ(ReserveRecoveryEvent(directory_, RecoveryEventKind::DATABASE_CORRUPTION, now), 0);
    EXPECT_EQ(ReserveRecoveryEvent(directory_, RecoveryEventKind::DATABASE_CORRUPTION, now), EAGAIN);
    EXPECT_EQ(ReserveRecoveryEvent(directory_, RecoveryEventKind::META_RECOVERY_INFO, now), 0);
    EXPECT_EQ(ReserveRecoveryEvent(directory_, RecoveryEventKind::META_RECOVERY_ERROR, now), 0);
    now = TEST_TIME + HOUR_MS;
    EXPECT_EQ(ReserveRecoveryEvent(directory_, RecoveryEventKind::DATABASE_CORRUPTION, now), 0);
    EXPECT_EQ(ReserveRecoveryEvent(directory_, RecoveryEventKind::DATABASE_CORRUPTION, now), EAGAIN);
}

HWTEST_F(RecoveryEventLimiterTest, RestartDoesNotResetQuotaAndExplicitRemovalDoes, TestSize.Level1)
{
    int64_t now = TEST_TIME;
    EXPECT_EQ(ReserveRecoveryEvent(directory_, RecoveryEventKind::DATABASE_CORRUPTION, now), 0);
    EXPECT_EQ(ReserveRecoveryEvent(directory_, RecoveryEventKind::DATABASE_CORRUPTION, now), 0);
    // The persisted quota still applies when the process starts again.
    now = TEST_TIME + 1;
    EXPECT_EQ(ReserveRecoveryEvent(directory_, RecoveryEventKind::DATABASE_CORRUPTION, now), EAGAIN);
    now = TEST_TIME + HOUR_MS;
    EXPECT_EQ(ReserveRecoveryEvent(directory_, RecoveryEventKind::DATABASE_CORRUPTION, now), 0);
    ASSERT_EQ(unlink((directory_ + LIMIT_FILE).c_str()), 0);
    EXPECT_EQ(ReserveRecoveryEvent(directory_, RecoveryEventKind::DATABASE_CORRUPTION, now), 0);
    EXPECT_EQ(ReserveRecoveryEvent(directory_, RecoveryEventKind::DATABASE_CORRUPTION, now), 0);
    EXPECT_EQ(ReserveRecoveryEvent(directory_, RecoveryEventKind::DATABASE_CORRUPTION, now), EAGAIN);
}

HWTEST_F(RecoveryEventLimiterTest, InvalidOrMissingPersistenceDoesNotPermitReporting, TestSize.Level1)
{
    int64_t now = TEST_TIME;
    std::ofstream output(directory_ + LIMIT_FILE);
    output << "10\ntruncated";
    output.close();
    EXPECT_EQ(ReserveRecoveryEvent(directory_, RecoveryEventKind::DATABASE_CORRUPTION, now), EINVAL);
    EXPECT_NE(ReserveRecoveryEvent(directory_ + "/missing", RecoveryEventKind::DATABASE_CORRUPTION, now), 0);
    now = -1;
    EXPECT_EQ(ReserveRecoveryEvent(directory_, RecoveryEventKind::DATABASE_CORRUPTION, now), EINVAL);
}

HWTEST_F(RecoveryEventLimiterTest, ClockRollbackDoesNotPermitExtraReports, TestSize.Level1)
{
    EXPECT_TRUE(IsRecoveryEventRecent(TEST_TIME, TEST_TIME - 1));
    EXPECT_FALSE(IsRecoveryEventRecent(TEST_TIME, TEST_TIME + HOUR_MS));
}
} // namespace OHOS::Media

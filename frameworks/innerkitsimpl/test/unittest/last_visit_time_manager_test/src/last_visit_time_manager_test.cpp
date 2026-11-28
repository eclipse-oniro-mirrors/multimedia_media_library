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
#define MLOG_TAG "LastVisitTimeManagerTest"
 
#include "last_visit_time_manager_test.h"
 
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>
 
#include "last_visit_time_manager.h"
#include "media_column.h"
#include "media_log.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "result_set.h"
 
using namespace testing::ext;
using namespace OHOS::NativeRdb;
 
namespace OHOS {
namespace Media {
namespace {
std::shared_ptr<MediaLibraryRdbStore> g_rdbStore = nullptr;
 
constexpr int32_t WAIT_FLUSH_TIMEOUT_MS = 5000;
constexpr int32_t POLL_INTERVAL_MS = 50;
constexpr int32_t BATCH_BOUNDARY_COUNT = 1200;
constexpr int32_t MAX_PENDING_SIZE = 5000;
 
int32_t InsertPhotoRows(int32_t count)
{
    if (count <= 0) {
        return NativeRdb::E_OK;
    }
    const std::string sql = "WITH RECURSIVE cnt(x) AS (SELECT 1 UNION ALL SELECT x + 1 FROM cnt WHERE x < " +
        std::to_string(count) + ") INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_LAST_VISIT_TIME + ") SELECT x, 0 FROM cnt";
    return g_rdbStore->ExecuteSql(sql);
}
 
int64_t QueryVisitedCount()
{
    const std::string sql = "SELECT COUNT(*) FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " +
        PhotoColumn::PHOTO_LAST_VISIT_TIME + " > 0";
    auto resultSet = g_rdbStore->QuerySql(sql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return -1;
    }
    int64_t count = -1;
    resultSet->GetLong(0, count);
    resultSet->Close();
    return count;
}
 
int64_t QueryLastVisitTime(const std::string &id)
{
    const std::string sql = "SELECT " + PhotoColumn::PHOTO_LAST_VISIT_TIME + " FROM " +
        PhotoColumn::PHOTOS_TABLE + " WHERE " + MediaColumn::MEDIA_ID + " = ?";
    auto resultSet = g_rdbStore->QuerySql(sql, std::vector<std::string> { id });
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return -1;
    }
    int64_t visitTime = -1;
    resultSet->GetLong(0, visitTime);
    resultSet->Close();
    return visitTime;
}
 
void MarkVisitedRange(int32_t count)
{
    for (int32_t i = 1; i <= count; ++i) {
        LastVisitTimeManager::GetInstance().MarkVisited(std::to_string(i));
    }
}
 
void ClearPhotos()
{
    g_rdbStore->ExecuteSql("DELETE FROM " + PhotoColumn::PHOTOS_TABLE);
}
} // namespace
 
void LastVisitTimeManagerTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::InitUnistore();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateBasicTables(g_rdbStore));
}
 
void LastVisitTimeManagerTest::TearDownTestCase(void)
{
    LastVisitTimeManager::GetInstance().Flush();
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, { PhotoColumn::PHOTOS_TABLE }, true);
    MediaLibraryUnitTestUtils::StopUnistore();
    g_rdbStore = nullptr;
}
 
void LastVisitTimeManagerTest::SetUp()
{
    // drain leftovers of previous case before resetting the table
    LastVisitTimeManager::GetInstance().Flush();
    ClearPhotos();
}
 
void LastVisitTimeManagerTest::TearDown()
{
    LastVisitTimeManager::GetInstance().Flush();
}
 
/*
 * @tc.desc Flush with empty pending ids test: no UPDATE is issued and no row is touched.
 * @tc.Co-authored-by Agent
 */
HWTEST_F(LastVisitTimeManagerTest, Flush_EmptyPending_NoWrite_001, TestSize.Level1)
{
    // Given: a cleared Photos table and no pending id
    ASSERT_EQ(InsertPhotoRows(1), NativeRdb::E_OK);
 
    // When: flush without any MarkVisited
    LastVisitTimeManager::GetInstance().Flush();
 
    // Then: the row keeps its initial last visit time
    EXPECT_EQ(QueryVisitedCount(), 0);
    EXPECT_EQ(QueryLastVisitTime("1"), 0);
}
 
/*
 * @tc.desc MarkVisited then Flush test: the pending id is persisted with a positive visit time.
 * @tc.Co-authored-by Agent
 */
HWTEST_F(LastVisitTimeManagerTest, MarkVisited_Flush_PersistsVisitTime_002, TestSize.Level0)
{
    // Given: one photo row
    ASSERT_EQ(InsertPhotoRows(1), NativeRdb::E_OK);
 
    // When: mark the id and flush synchronously
    LastVisitTimeManager::GetInstance().MarkVisited("1");
    LastVisitTimeManager::GetInstance().Flush();
 
    // Then: exactly the marked row is updated with a positive timestamp
    EXPECT_EQ(QueryVisitedCount(), 1);
    EXPECT_GT(QueryLastVisitTime("1"), 0);
}
 
/*
 * @tc.desc MarkVisited the same id twice then Flush test: the id is deduplicated and the row is updated once.
 * @tc.Co-authored-by Agent
 */
HWTEST_F(LastVisitTimeManagerTest, MarkVisited_DuplicateId_Deduplicated_003, TestSize.Level1)
{
    // Given: two photo rows
    ASSERT_EQ(InsertPhotoRows(2), NativeRdb::E_OK);
 
    // When: mark the same id twice, then flush
    LastVisitTimeManager::GetInstance().MarkVisited("1");
    LastVisitTimeManager::GetInstance().MarkVisited("1");
    LastVisitTimeManager::GetInstance().Flush();
 
    // Then: only the duplicated id is updated, the other row is untouched
    EXPECT_EQ(QueryVisitedCount(), 1);
    EXPECT_GT(QueryLastVisitTime("1"), 0);
    EXPECT_EQ(QueryLastVisitTime("2"), 0);
}
 
/*
 * @tc.desc MarkVisited multiple ids then Flush test: all pending ids are persisted in one flush.
 * @tc.Co-authored-by Agent
 */
HWTEST_F(LastVisitTimeManagerTest, MarkVisited_MultipleIds_AllPersisted_004, TestSize.Level1)
{
    // Given: three photo rows
    ASSERT_EQ(InsertPhotoRows(3), NativeRdb::E_OK);
 
    // When: mark three ids and flush
    MarkVisitedRange(3);
    LastVisitTimeManager::GetInstance().Flush();
 
    // Then: all three rows are updated
    EXPECT_EQ(QueryVisitedCount(), 3);
}
 
/*
 * @tc.desc MarkVisited empty id then Flush test: the invalid id matches no row and does not crash.
 * @tc.Co-authored-by Agent
 */
HWTEST_F(LastVisitTimeManagerTest, MarkVisited_EmptyId_NoRowMatched_005, TestSize.Level1)
{
    // Given: one photo row
    ASSERT_EQ(InsertPhotoRows(1), NativeRdb::E_OK);
 
    // When: mark an empty id and flush
    LastVisitTimeManager::GetInstance().MarkVisited("");
    LastVisitTimeManager::GetInstance().Flush();
 
    // Then: no row is updated
    EXPECT_EQ(QueryVisitedCount(), 0);
}
 
/*
 * @tc.desc MarkVisited without explicit Flush test: the debounce thread persists the id automatically.
 * @tc.Co-authored-by Agent
 */
HWTEST_F(LastVisitTimeManagerTest, MarkVisited_AsyncDebounce_PersistedAutomatically_006, TestSize.Level1)
{
    // Given: one photo row
    ASSERT_EQ(InsertPhotoRows(1), NativeRdb::E_OK);
 
    // When: mark the id and wait for the debounce flush without calling Flush
    LastVisitTimeManager::GetInstance().MarkVisited("1");
    int64_t count = 0;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(WAIT_FLUSH_TIMEOUT_MS);
    while (std::chrono::steady_clock::now() < deadline) {
        count = QueryVisitedCount();
        if (count > 0) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(POLL_INTERVAL_MS));
    }
 
    // Then: the background flusher persisted the id
    EXPECT_EQ(count, 1);
    EXPECT_GT(QueryLastVisitTime("1"), 0);
}
 
/*
 * @tc.desc MarkVisited more ids than one batch：ids are split into MAX_BATCH_SIZE chunks and all persisted.
 * @tc.Co-authored-by Agent
 */
HWTEST_F(LastVisitTimeManagerTest, MarkVisited_ExceedBatchSize_AllPersisted_007, TestSize.Level1)
{
    // Given: more rows than MAX_BATCH_SIZE
    ASSERT_EQ(InsertPhotoRows(BATCH_BOUNDARY_COUNT), NativeRdb::E_OK);
 
    // When: mark all ids and flush once
    MarkVisitedRange(BATCH_BOUNDARY_COUNT);
    LastVisitTimeManager::GetInstance().Flush();
 
    // Then: every row is updated across multiple UPDATE statements
    EXPECT_EQ(QueryVisitedCount(), BATCH_BOUNDARY_COUNT);
}
 
/*
 * @tc.desc MarkVisited exactly MAX_PENDING_SIZE ids then Flush test: the capacity boundary is inclusive.
 * @tc.Co-authored-by Agent
 */
HWTEST_F(LastVisitTimeManagerTest, MarkVisited_ReachPendingCapacity_AllAccepted_008, TestSize.Level1)
{
    // Given: MAX_PENDING_SIZE rows
    ASSERT_EQ(InsertPhotoRows(MAX_PENDING_SIZE), NativeRdb::E_OK);
 
    // When: mark exactly MAX_PENDING_SIZE distinct ids and flush
    MarkVisitedRange(MAX_PENDING_SIZE);
    LastVisitTimeManager::GetInstance().Flush();
 
    // Then: all ids are accepted and persisted
    EXPECT_EQ(QueryVisitedCount(), MAX_PENDING_SIZE);
}
 
/*
 * @tc.desc MarkVisited over MAX_PENDING_SIZE ids then Flush test: the extra id is dropped, the first MAX are kept.
 * @tc.Co-authored-by Agent
 */
HWTEST_F(LastVisitTimeManagerTest, MarkVisited_ExceedPendingCapacity_DropsNewId_009, TestSize.Level1)
{
    // Given: MAX_PENDING_SIZE + 1 rows
    ASSERT_EQ(InsertPhotoRows(MAX_PENDING_SIZE + 1), NativeRdb::E_OK);
 
    // When: mark one id beyond the pending capacity and flush
    MarkVisitedRange(MAX_PENDING_SIZE + 1);
    LastVisitTimeManager::GetInstance().Flush();
 
    // Then: the first MAX ids are persisted and the overflowing id is dropped
    EXPECT_EQ(QueryVisitedCount(), MAX_PENDING_SIZE);
    EXPECT_EQ(QueryLastVisitTime(std::to_string(MAX_PENDING_SIZE + 1)), 0);
}
 
/*
 * @tc.desc Flush with a null rdb store test: pending ids are dropped without crashing.
 * @tc.Co-authored-by Agent
 */
HWTEST_F(LastVisitTimeManagerTest, Flush_NullRdbStore_DropsPendingIds_010, TestSize.Level1)
{
    // Given: the unistore is stopped, so GetRdbStore returns nullptr
    MediaLibraryUnitTestUtils::StopUnistore();
    LastVisitTimeManager::GetInstance().MarkVisited("1");
 
    // When: flush without an available rdb store
    LastVisitTimeManager::GetInstance().Flush();
 
    // Then: no crash, and the dropped id never reaches the database after re-init
    MediaLibraryUnitTestUtils::InitUnistore();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateBasicTables(g_rdbStore));
    EXPECT_EQ(QueryVisitedCount(), 0);
}
} // namespace Media
} // namespace OHOS
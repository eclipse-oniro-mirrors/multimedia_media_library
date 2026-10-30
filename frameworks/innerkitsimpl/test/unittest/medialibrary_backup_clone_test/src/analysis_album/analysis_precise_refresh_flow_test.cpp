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

#include "analysis_precise_refresh_test.h"

using namespace testing::ext;
namespace OHOS::Media {
HWTEST_F(AnalysisPreciseRefreshTest, OverlapKeepsTargetOnlyAddsSourceOnlyAndDeduplicates, TestSize.Level1)
{
    // Target: A(1), X(2). Source: A(11), C(12), failed D(13). Final: A(11), X(2), C(12).
    ASSERT_EQ(Album(TestAlbum(10, 2)), E_OK);
    for (int32_t id : {1, 2}) {
        ASSERT_EQ(Photo(id, id), E_OK);
        ASSERT_EQ(Map(10, id), E_OK);
    }
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(20, 3)}, Side::SOURCE));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1, 11, 12, 13}));
    ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM Photos WHERE file_id=1"), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM AnalysisPhotoMap WHERE map_asset=1"), E_OK);
    for (int32_t id : {11, 12}) {
        ASSERT_EQ(Photo(id, id), E_OK);
        ASSERT_EQ(Map(10, id), E_OK);
    }
    task.RecordPhotoResults({11, 12}, {{1, 11}});
    task.RecordAlbumResult({20, 10, 0, TEST_CLASSIFY_TYPE, true});
    const auto result = task.Refresh(rdb_);
    ExpectSuccess(result);
    EXPECT_EQ(ReadAlbum(10).count, 3);
    EXPECT_EQ(ReadAlbum(10).coverUri, TestPhotoUri(12));
    EXPECT_EQ(result.changedAlbumIds, (std::unordered_set<int32_t> {10}));
}

HWTEST_F(AnalysisPreciseRefreshTest, VisibilityTransitionsCalculateAgainstSavedBefore, TestSize.Level1)
{
    for (const bool beforeVisible : {false, true}) {
        for (const bool afterVisible : {false, true}) {
            SCOPED_TRACE(std::to_string(beforeVisible) + "/" + std::to_string(afterVisible));
            ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM AnalysisAlbum"), E_OK);
            ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM Photos"), E_OK);
            ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM AnalysisPhotoMap"), E_OK);
            ASSERT_EQ(Album(TestAlbum(10, beforeVisible ? 1 : 0)), E_OK);
            ASSERT_EQ(Photo(1), E_OK);
            ASSERT_EQ(Map(10, 1), E_OK);
            ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET hidden=" + std::to_string(!beforeVisible)), E_OK);
            AnalysisAlbumRefreshTask task;
            ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
            ASSERT_TRUE(task.CaptureBefore(rdb_, {1}));
            ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET hidden=" + std::to_string(!afterVisible)), E_OK);
            task.RecordPhotoResults({1}, {});
            ExpectSuccess(task.Refresh(rdb_));
            EXPECT_EQ(ReadAlbum(10).count, afterVisible ? 1 : 0);
        }
    }
}

HWTEST_F(AnalysisPreciseRefreshTest, CaptureTwiceDoesNotOverwriteOriginalContribution, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 1)), E_OK);
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1, 1, 0, -1}));
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET hidden=1"), E_OK);
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1}));
    EXPECT_EQ(task.FindOriginalAlbum(Side::TARGET, 10)->capturedCount, 1);
    task.RecordPhotoResults({1}, {});
    ExpectSuccess(task.Refresh(rdb_));
    EXPECT_EQ(ReadAlbum(10).count, 0);
}

HWTEST_F(AnalysisPreciseRefreshTest, NewAlbumCountsActualSuccessNotSourceCount, TestSize.Level1)
{
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(20, 100)}, Side::SOURCE));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1, 2}));
    ASSERT_EQ(Album(TestAlbum(30, 100)), E_OK);
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(Map(30, 1), E_OK);
    task.RecordPhotoResults({1}, {});
    task.RecordAlbumResult({20, 30, 0, TEST_CLASSIFY_TYPE, true});
    ExpectSuccess(task.Refresh(rdb_));
    EXPECT_EQ(ReadAlbum(30).count, 1);
}

HWTEST_F(AnalysisPreciseRefreshTest, DefaultCoverReselectsEvenWhenDeltaIsZero, TestSize.Level1)
{
    // Old default 1 remains eligible. Replace another photo (3) with newer 2; count stays two.
    ASSERT_EQ(Album(TestAlbum(10, 2, TEST_CLASSIFY_TYPE, 1, TEST_DEFAULT_COVER)), E_OK);
    ASSERT_EQ(Photo(1, 1), E_OK);
    ASSERT_EQ(Photo(3, 0), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    ASSERT_EQ(Map(10, 3), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {3, 2}));
    ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM Photos WHERE file_id=3"), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM AnalysisPhotoMap WHERE map_asset=3"), E_OK);
    ASSERT_EQ(Photo(2, 2), E_OK);
    ASSERT_EQ(Map(10, 2), E_OK);
    task.RecordPhotoResults({2}, {{3, 2}});
    const auto result = task.Refresh(rdb_);
    ExpectSuccess(result);
    EXPECT_TRUE(result.changedAlbumIds.empty()); // count changes only, not cover-only changes.
    EXPECT_EQ(ReadAlbum(10).count, 2);
    EXPECT_EQ(ReadAlbum(10).coverUri, TestPhotoUri(2));
}

HWTEST_F(AnalysisPreciseRefreshTest, ManualCoverPriorityRespectsRetainedSide, TestSize.Level1)
{
    for (const int32_t subtype : {TEST_CLASSIFY_TYPE, PORTRAIT_TYPE,
        static_cast<int32_t>(PhotoAlbumSubType::GROUP_PHOTO)}) {
        for (const auto &[sourceCover, targetCover] : std::vector<std::pair<uint8_t, uint8_t>> {
            {1, 1}, {1, 2}, {1, 3}, {2, 1}, {2, 2}, {2, 3}, {3, 1}, {3, 2}, {3, 3}}) {
            SCOPED_TRACE(std::to_string(subtype) + "/" + std::to_string(sourceCover) +
                "/" + std::to_string(targetCover));
            ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM AnalysisAlbum"), E_OK);
            ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM Photos"), E_OK);
            ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM AnalysisPhotoMap"), E_OK);
            ASSERT_EQ(Album(TestAlbum(10, 1, subtype, 1, targetCover)), E_OK);
            ASSERT_EQ(rdb_->ExecuteSql("UPDATE AnalysisAlbum SET group_tag='g'"), E_OK);
            // Old cover is newer: selecting the new cover must not depend on default ranking.
            ASSERT_EQ(Photo(1, 100), E_OK);
            ASSERT_EQ(Map(10, 1), E_OK);
            AnalysisAlbumRefreshTask task;
            ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
            ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(20, 99, subtype, 2, sourceCover)}, Side::SOURCE));
            ASSERT_TRUE(task.CaptureBefore(rdb_, {2}));
            ASSERT_EQ(Photo(2, 2), E_OK);
            ASSERT_EQ(Map(10, 2), E_OK);
            task.RecordPhotoResults({2}, {});
            task.RecordAlbumResult({20, 10, 0, subtype, true});
            EXPECT_EQ(task.FindOriginalAlbum(Side::SOURCE, 20)->binding.includeBaseline, subtype == TEST_CLASSIFY_TYPE);
            ExpectSuccess(task.Refresh(rdb_));
            // Portrait/group discard the source side, including its manual cover.
            const bool useSource = subtype == TEST_CLASSIFY_TYPE &&
                (sourceCover != TEST_DEFAULT_COVER || targetCover == TEST_DEFAULT_COVER);
            EXPECT_EQ(ReadAlbum(10).count, 2);
            EXPECT_EQ(ReadAlbum(10).coverUri, TestPhotoUri(useSource ? 2 : 1));
            EXPECT_EQ(ReadAlbum(10).coverSatisfied, useSource ? sourceCover : targetCover);
        }
    }
}

HWTEST_F(AnalysisPreciseRefreshTest, MergedDefaultCoverKeepsNewDeviceCoverWithZeroDelta, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 2, TEST_CLASSIFY_TYPE, 1, TEST_DEFAULT_COVER)), E_OK);
    ASSERT_EQ(Photo(1, 100), E_OK);
    ASSERT_EQ(Photo(3, 0), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    ASSERT_EQ(Map(10, 3), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(20, 1, TEST_CLASSIFY_TYPE, 2, TEST_DEFAULT_COVER)}, Side::SOURCE));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {3, 2}));
    ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM Photos WHERE file_id=3"), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM AnalysisPhotoMap WHERE map_asset=3"), E_OK);
    ASSERT_EQ(Photo(2, 1), E_OK);
    ASSERT_EQ(Map(10, 2), E_OK);
    task.RecordPhotoResults({2}, {{3, 2}});
    task.RecordAlbumResult({20, 10, 0, TEST_CLASSIFY_TYPE, true});
    const auto result = task.Refresh(rdb_);
    ExpectSuccess(result);
    EXPECT_TRUE(result.changedAlbumIds.empty());
    EXPECT_EQ(ReadAlbum(10).count, 2);
    EXPECT_EQ(ReadAlbum(10).coverUri, TestPhotoUri(2));
    EXPECT_EQ(ReadAlbum(10).coverSatisfied, TEST_DEFAULT_COVER);
}

HWTEST_F(AnalysisPreciseRefreshTest, DiscardedManualCoverDoesNotTriggerFallback, TestSize.Level1)
{
    for (const int32_t subtype : {PORTRAIT_TYPE, static_cast<int32_t>(PhotoAlbumSubType::GROUP_PHOTO)}) {
        SCOPED_TRACE(std::to_string(subtype));
        ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM AnalysisAlbum"), E_OK);
        ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM Photos"), E_OK);
        ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM AnalysisPhotoMap"), E_OK);
        ASSERT_EQ(Album(TestAlbum(10, 2, subtype, 1, TEST_DEFAULT_COVER)), E_OK);
        ASSERT_EQ(rdb_->ExecuteSql("UPDATE AnalysisAlbum SET group_tag='g'"), E_OK);
        ASSERT_EQ(Photo(1, 1), E_OK);
        ASSERT_EQ(Map(10, 1), E_OK);
        ASSERT_EQ(Photo(2, 2), E_OK);
        ASSERT_EQ(Map(10, 2), E_OK);
        ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET display_name='' WHERE file_id=2"), E_OK);
        AnalysisAlbumRefreshTask task;
        ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
        ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(20, 99, subtype, 2,
            static_cast<uint8_t>(CoverSatisfiedType::USER_SETTING_EDITE))}, Side::SOURCE));
        task.RecordAlbumResult({20, 10, 0, subtype, true});
        const auto result = task.Refresh(rdb_);
        ExpectSuccess(result);
        EXPECT_TRUE(result.changedAlbumIds.empty());
        EXPECT_EQ(ReadAlbum(10).count, 2);
        EXPECT_EQ(ReadAlbum(10).coverUri, TestPhotoUri(1));
        EXPECT_EQ(ReadAlbum(10).coverSatisfied, TEST_DEFAULT_COVER);
    }
}

HWTEST_F(AnalysisPreciseRefreshTest, InvalidManualCoverFallsBackToEligibleDefault, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 1, TEST_CLASSIFY_TYPE, 1, MANUAL_COVER)), E_OK);
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1, 2}));
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET is_shared=1 WHERE file_id=1"), E_OK);
    ASSERT_EQ(Photo(2, 2), E_OK);
    ASSERT_EQ(Map(10, 2), E_OK);
    task.RecordPhotoResults({1, 2}, {});
    ExpectSuccess(task.Refresh(rdb_));
    EXPECT_EQ(ReadAlbum(10).coverUri, TestPhotoUri(2));
    EXPECT_EQ(ReadAlbum(10).coverSatisfied, TEST_DEFAULT_COVER);
}

HWTEST_F(AnalysisPreciseRefreshTest, CoverReplacementUsesFinalIdAndFinalData, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 1, TEST_CLASSIFY_TYPE, 1, MANUAL_COVER)), E_OK);
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1, 101}));
    ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM Photos WHERE file_id=1"), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM AnalysisPhotoMap"), E_OK);
    ASSERT_EQ(Photo(101), E_OK);
    ASSERT_EQ(Map(10, 101), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET data='/storage/cloud/files/Photo/9/final.jpg',"
        "display_name='renamed.jpg' WHERE file_id=101"), E_OK);
    task.RecordPhotoResults({101}, {{1, 101}});
    ExpectSuccess(task.Refresh(rdb_));
    EXPECT_EQ(ReadAlbum(10).coverUri, MediaFileUri::GetPhotoUri("101",
        "/storage/cloud/files/Photo/9/final.jpg", "renamed.jpg"));
    EXPECT_EQ(ReadAlbum(10).coverSatisfied, MANUAL_COVER);
}

HWTEST_F(AnalysisPreciseRefreshTest, ConflictingCoverReplacementFailsOnlyAffectedAlbum, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 1, TEST_CLASSIFY_TYPE, 1, MANUAL_COVER)), E_OK);
    ASSERT_EQ(Album(TestAlbum(20)), E_OK);
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1, 2}));
    task.RecordPhotoResults({}, {{1, 2}}, {1});
    EXPECT_TRUE(task.FindOriginalAlbum(Side::TARGET, 10)->coverUnknown);
    const auto result = task.Refresh(rdb_);
    EXPECT_NE(result.error, E_OK);
    EXPECT_FALSE(result.fullRefresh);
    EXPECT_EQ(result.failedAlbumIds, (std::unordered_set<int32_t> {10}));
}

HWTEST_F(AnalysisPreciseRefreshTest, EmptyAlbumClearsCoverAndSpecializedKeepsCover, TestSize.Level1)
{
    for (const int32_t type : {TEST_CLASSIFY_TYPE, static_cast<int32_t>(PhotoAlbumSubType::HIGHLIGHT),
        static_cast<int32_t>(PhotoAlbumSubType::HIGHLIGHT_SUGGESTIONS)}) {
        ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM AnalysisAlbum"), E_OK);
        ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM Photos"), E_OK);
        ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM AnalysisPhotoMap"), E_OK);
        ASSERT_EQ(Album(TestAlbum(10, 1, type, 1, MANUAL_COVER)), E_OK);
        ASSERT_EQ(Photo(1), E_OK);
        ASSERT_EQ(Map(10, 1), E_OK);
        AnalysisAlbumRefreshTask task;
        ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
        ASSERT_TRUE(task.CaptureBefore(rdb_, {1}));
        ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM Photos"), E_OK);
        ExpectSuccess(task.Refresh(rdb_));
        EXPECT_EQ(ReadAlbum(10).count, 0);
        EXPECT_EQ(ReadAlbum(10).coverUri, type == TEST_CLASSIFY_TYPE ? "" : TestPhotoUri(1));
    }
}

HWTEST_F(AnalysisPreciseRefreshTest, UntouchedAlbumDoesNotWriteOrNeedPhotoMetadata, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 5, TEST_CLASSIFY_TYPE, 1, MANUAL_COVER)), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_EQ(rdb_->ExecuteSql("CREATE TRIGGER no_update BEFORE UPDATE ON AnalysisAlbum "
        "BEGIN SELECT RAISE(ABORT,'untouched must not write'); END"), E_OK);
    const auto result = task.Refresh(rdb_);
    ExpectSuccess(result);
    EXPECT_TRUE(result.changedAlbumIds.empty());
    EXPECT_EQ(ReadAlbum(10).count, 5);
}

HWTEST_F(AnalysisPreciseRefreshTest, BeforeQueryFailureStaysUnavailableAfterDatabaseRecovers, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 1)), E_OK);
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    // Membership still works but visibility query fails: scoped fallback, no stale recapture.
    ASSERT_EQ(rdb_->ExecuteSql("ALTER TABLE Photos RENAME COLUMN sync_status TO missing_sync"), E_OK);
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1}));
    EXPECT_EQ(task.GetFacts().targetCoverage.at(1), Coverage::UNAVAILABLE);
    EXPECT_FALSE(task.FindOriginalAlbum(Side::TARGET, 10)->complete);
    ASSERT_EQ(rdb_->ExecuteSql("ALTER TABLE Photos RENAME COLUMN missing_sync TO sync_status"), E_OK);
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1}));
    EXPECT_EQ(task.GetFacts().targetCoverage.at(1), Coverage::UNAVAILABLE);
    const auto result = task.Refresh(rdb_);
    EXPECT_FALSE(result.fullRefresh);
    EXPECT_EQ(result.failedAlbumIds, (std::unordered_set<int32_t> {10}));
}

HWTEST_F(AnalysisPreciseRefreshTest, BothBeforeQueriesFailForcesTaskFallback, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10)), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_EQ(rdb_->ExecuteSql("DROP TABLE AnalysisPhotoMap"), E_OK);
    EXPECT_FALSE(task.CaptureBefore(rdb_, {1}));
    EXPECT_FALSE(task.IsTaskComplete());
    EXPECT_TRUE(task.Refresh(rdb_).fullRefresh);
}

HWTEST_F(AnalysisPreciseRefreshTest, MissingInitializationAndFinalQueryFailure, TestSize.Level1)
{
    AnalysisAlbumRefreshTask task;
    EXPECT_FALSE(task.CaptureBefore(rdb_, {1}));
    EXPECT_TRUE(task.Refresh(rdb_).fullRefresh);
    task.Clear();
    EXPECT_FALSE(task.InitializeOriginalAlbums(nullptr, Side::TARGET));
    EXPECT_TRUE(task.Refresh(rdb_).fullRefresh);
    task.Clear();
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::SOURCE));
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_EQ(rdb_->ExecuteSql("DROP TABLE AnalysisAlbum"), E_OK);
    EXPECT_TRUE(task.Refresh(rdb_).fullRefresh);
}

HWTEST_F(AnalysisPreciseRefreshTest, MergeChainAccumulatesBaselinesButCycleFallsBack, TestSize.Level1)
{
    for (const bool cycle : {false, true}) {
        ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM AnalysisAlbum"), E_OK);
        ASSERT_EQ(Album(TestAlbum(10, 1)), E_OK);
        ASSERT_EQ(Album(TestAlbum(20, 2)), E_OK);
        ASSERT_EQ(Album(TestAlbum(30, 3)), E_OK);
        AnalysisAlbumRefreshTask task;
        ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
        task.RecordTargetAlbumMerge(10, 20);
        task.RecordTargetAlbumMerge(20, cycle ? 10 : 30);
        if (!cycle) {
            ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM AnalysisAlbum WHERE album_id IN (10,20)"), E_OK);
        }
        const auto result = task.Refresh(rdb_);
        if (cycle) {
            EXPECT_EQ(result.failedAlbumIds, (std::unordered_set<int32_t> {10, 20}));
        } else {
            ExpectSuccess(result);
            EXPECT_EQ(ReadAlbum(30).count, 6);
        }
    }
}

HWTEST_F(AnalysisPreciseRefreshTest, UnregisteredOrNegativeBaselineRequiresAlbumFallback, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, -1)), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_EQ(Album(TestAlbum(20, 4)), E_OK);
    const auto result = task.Refresh(rdb_);
    EXPECT_FALSE(result.fullRefresh);
    EXPECT_EQ(result.failedAlbumIds, (std::unordered_set<int32_t> {10, 20}));
    EXPECT_EQ(ReadAlbum(20).count, 4);
}

HWTEST_F(AnalysisPreciseRefreshTest, FailedWriteReportsAlbumWithoutDiscardingOtherSuccess, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10)), E_OK);
    ASSERT_EQ(Album(TestAlbum(20)), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1, 2}));
    for (int32_t id : {1, 2}) {
        ASSERT_EQ(Photo(id), E_OK);
        ASSERT_EQ(Map(id * 10, id), E_OK);
    }
    task.RecordPhotoResults({1, 2}, {});
    ASSERT_EQ(rdb_->ExecuteSql("CREATE TRIGGER deny_one BEFORE UPDATE ON AnalysisAlbum WHEN OLD.album_id=10 "
        "BEGIN SELECT RAISE(ABORT,'only album 10'); END"), E_OK);
    const auto result = task.Refresh(rdb_);
    EXPECT_NE(result.error, E_OK);
    EXPECT_FALSE(result.fullRefresh);
    EXPECT_EQ(result.failedAlbumIds, (std::unordered_set<int32_t> {10}));
    EXPECT_EQ(result.changedAlbumIds, (std::unordered_set<int32_t> {20}));
    EXPECT_EQ(ReadAlbum(10).count, 0);
    EXPECT_EQ(ReadAlbum(20).count, 1);
}

HWTEST_F(AnalysisPreciseRefreshTest, GroupChangeInvalidatesOnlyGroupedAlbums, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 0, PORTRAIT_TYPE)), E_OK);
    ASSERT_EQ(Album(TestAlbum(20)), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE AnalysisAlbum SET group_tag='old' WHERE album_id=10"), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE AnalysisAlbum SET group_tag='new' WHERE album_id=10"), E_OK);
    const auto result = task.Refresh(rdb_);
    EXPECT_EQ(result.failedAlbumIds, (std::unordered_set<int32_t> {10}));
    EXPECT_FALSE(result.fullRefresh);
}

HWTEST_F(AnalysisPreciseRefreshTest, SameGroupDefaultCoverSharedAndCountsDeduplicated, TestSize.Level1)
{
    for (int32_t id : {10, 20}) {
        ASSERT_EQ(Album(TestAlbum(id, 0, PORTRAIT_TYPE)), E_OK);
    }
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE AnalysisAlbum SET group_tag='g'"), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1}));
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    ASSERT_EQ(Map(20, 1), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("INSERT INTO tab_analysis_image_face VALUES(1,'g',0,1.0,1)"), E_OK);
    task.RecordPhotoResults({1}, {});
    ExpectSuccess(task.Refresh(rdb_));
    for (int32_t id : {10, 20}) {
        EXPECT_EQ(ReadAlbum(id).count, 1);
        EXPECT_EQ(ReadAlbum(id).coverUri, TestPhotoUri(1));
    }
}

HWTEST_F(AnalysisPreciseRefreshTest, SourceRetainedPortraitUsesFinalAssetsOnly, TestSize.Level1)
{
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(20, 99, PORTRAIT_TYPE)}, Side::SOURCE));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1}));
    ASSERT_EQ(Album(TestAlbum(30, 99, PORTRAIT_TYPE)), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE AnalysisAlbum SET group_tag='g'"), E_OK);
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(Map(30, 1), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("INSERT INTO tab_analysis_image_face VALUES(1,'g',0,1.0,1)"), E_OK);
    task.RecordPhotoResults({1}, {});
    task.RecordAlbumResult({20, 30, 0, PORTRAIT_TYPE, true});
    ExpectSuccess(task.Refresh(rdb_));
    EXPECT_EQ(ReadAlbum(30).count, 1);
}

HWTEST_F(AnalysisPreciseRefreshTest, DeletedCoverBeforeMigrationIsNotReused, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 0, TEST_CLASSIFY_TYPE, 1, MANUAL_COVER)), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    task.RecordDeletedTargets({1});
    EXPECT_TRUE(task.FindOriginalAlbum(Side::TARGET, 10)->coverDeleted);
    ExpectSuccess(task.Refresh(rdb_));
    EXPECT_TRUE(ReadAlbum(10).coverUri.empty());
}

HWTEST_F(AnalysisPreciseRefreshTest, LateDeletionAndConflictingReplacementRemainIncomplete, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 0, TEST_CLASSIFY_TYPE, 1, MANUAL_COVER)), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1, 2, 3}));
    task.RecordPhotoResults({}, {{0, 1}, {1, 1}});
    task.RecordPhotoResults({}, {{1, 2}});
    task.RecordPhotoResults({}, {{1, 3}});
    EXPECT_TRUE(task.FindOriginalAlbum(Side::TARGET, 10)->coverUnknown);
    task.RecordDeletedTargets({1});
    EXPECT_FALSE(task.IsTaskComplete());
    EXPECT_TRUE(task.Refresh(rdb_).fullRefresh);
}

HWTEST_F(AnalysisPreciseRefreshTest, RepeatedCaptureDoesNotQueryDatabaseAgain, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 1)), E_OK);
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1}));
    ASSERT_EQ(rdb_->ExecuteSql("DROP TABLE Photos"), E_OK);
    EXPECT_TRUE(task.CaptureBefore(rdb_, {1, 1}));
    EXPECT_EQ(task.FindOriginalAlbum(Side::TARGET, 10)->capturedCount, 1);
}

HWTEST_F(AnalysisPreciseRefreshTest, SevenThousandUntouchedAlbumsRequireNoAssetQueries, TestSize.Level1)
{
    ASSERT_EQ(rdb_->ExecuteSql("WITH RECURSIVE ids(x) AS (SELECT 1 UNION ALL SELECT x+1 FROM ids WHERE x<7000) "
        "INSERT INTO AnalysisAlbum(album_id,album_subtype,count) SELECT x," +
        std::to_string(TEST_CLASSIFY_TYPE) + ",1 FROM ids"), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_EQ(rdb_->ExecuteSql("DROP TABLE Photos"), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("DROP TABLE AnalysisPhotoMap"), E_OK);
    const auto result = task.Refresh(rdb_);
    ExpectSuccess(result);
    EXPECT_TRUE(result.changedAlbumIds.empty());
}

HWTEST_F(AnalysisPreciseRefreshTest, MissingCoverMetadataFailsAlbumButKeepsCorrectCount, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 0, TEST_CLASSIFY_TYPE, 1, MANUAL_COVER)), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1}));
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET data=''"), E_OK);
    task.RecordPhotoResults({1}, {});
    const auto result = task.Refresh(rdb_);
    EXPECT_EQ(result.failedAlbumIds, (std::unordered_set<int32_t> {10}));
    EXPECT_FALSE(result.fullRefresh);
    EXPECT_EQ(ReadAlbum(10).count, 1);
}

HWTEST_F(AnalysisPreciseRefreshTest, FinalCountQueryFailureRequestsFullFallback, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10)), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1}));
    ASSERT_EQ(rdb_->ExecuteSql("DROP TABLE AnalysisPhotoMap"), E_OK);
    const auto result = task.Refresh(rdb_);
    EXPECT_NE(result.error, E_OK);
    EXPECT_TRUE(result.fullRefresh);
}

HWTEST_F(AnalysisPreciseRefreshTest, SubtypeMismatchAndRenamedModeRequireScopedFallback, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10)), E_OK);
    auto mode = TestAlbum(20, 0, MODE_TYPE);
    mode.albumName = std::to_string(static_cast<int32_t>(ShootingModeAlbumType::MOVING_PICTURE));
    ASSERT_EQ(Album(mode), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE AnalysisAlbum SET album_subtype=" +
        std::to_string(PORTRAIT_TYPE) + " WHERE album_id=10"), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE AnalysisAlbum SET album_name='13' WHERE album_id=20"), E_OK);
    const auto result = task.Refresh(rdb_);
    EXPECT_FALSE(result.fullRefresh);
    EXPECT_EQ(result.failedAlbumIds, (std::unordered_set<int32_t> {10, 20}));
}

HWTEST_F(AnalysisPreciseRefreshTest, InvalidModeDirectoryMarksTaskIncomplete, TestSize.Level1)
{
    auto mode = TestAlbum(10, 0, MODE_TYPE);
    mode.albumName = "-1";
    ASSERT_EQ(Album(mode), E_OK);
    AnalysisAlbumRefreshTask task;
    EXPECT_FALSE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    EXPECT_FALSE(task.IsTaskComplete());
    EXPECT_TRUE(task.Refresh(rdb_).fullRefresh);
}

HWTEST_F(AnalysisPreciseRefreshTest, MoreBeforeThanBaselineFallsBackInsteadOfWritingNegativeCount, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 0)), E_OK);
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1}));
    ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM Photos"), E_OK);
    const auto result = task.Refresh(rdb_);
    EXPECT_EQ(result.failedAlbumIds, (std::unordered_set<int32_t> {10}));
    EXPECT_EQ(ReadAlbum(10).count, 0);
}

HWTEST_F(AnalysisPreciseRefreshTest, PortraitTargetRetainedDoesNotAddSourceBaseline, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 1, PORTRAIT_TYPE, 1, MANUAL_COVER)), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE AnalysisAlbum SET group_tag='g'"), E_OK);
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(20, 99, PORTRAIT_TYPE)}, Side::SOURCE));
    task.RecordAlbumResult({20, 10, 0, PORTRAIT_TYPE, true});
    ExpectSuccess(task.Refresh(rdb_));
    EXPECT_EQ(ReadAlbum(10).count, 1);
    EXPECT_EQ(ReadAlbum(10).coverUri, TestPhotoUri(1));
}


HWTEST_F(AnalysisPreciseRefreshTest, CaptureCountOverflowMarksUnavailableWithoutPublishing, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 1)), E_OK);
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    // Existing test target permits private access; exercise the guard without allocating INT64_MAX assets.
    task.data_.targetAlbums.at(10).capturedCount = INT64_MAX;
    EXPECT_FALSE(task.CaptureBefore(rdb_, {1}));
    EXPECT_FALSE(task.IsTaskComplete());
    EXPECT_EQ(task.GetFacts().targetAlbums.at(10).capturedCount, INT64_MAX);
    EXPECT_EQ(task.GetFacts().targetCoverage.at(1), Coverage::UNAVAILABLE);
    EXPECT_TRUE(task.Refresh(rdb_).fullRefresh);
}

HWTEST_F(AnalysisPreciseRefreshTest, AddedMemberToExistingGroupInvalidatesBothAlbums, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 0, PORTRAIT_TYPE)), E_OK);
    ASSERT_EQ(Album(TestAlbum(20, 0, PORTRAIT_TYPE)), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE AnalysisAlbum SET group_tag='g' WHERE album_id=10"), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE AnalysisAlbum SET group_tag='g' WHERE album_id=20"), E_OK);
    const auto result = task.Refresh(rdb_);
    EXPECT_NE(result.error, E_OK);
    EXPECT_FALSE(result.fullRefresh);
    EXPECT_EQ(result.failedAlbumIds, (std::unordered_set<int32_t> {10, 20}));
}

HWTEST_F(AnalysisPreciseRefreshTest, DefaultCoverReadFailureDoesNotReportRefreshSuccess, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10)), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1}));
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET display_name=''"), E_OK);
    task.RecordPhotoResults({1}, {});
    const auto result = task.Refresh(rdb_);
    EXPECT_NE(result.error, E_OK);
    EXPECT_FALSE(result.fullRefresh);
    EXPECT_EQ(result.failedAlbumIds, (std::unordered_set<int32_t> {10}));
    EXPECT_EQ(ReadAlbum(10).count, 1);
    EXPECT_TRUE(ReadAlbum(10).coverUri.empty());
}

HWTEST_F(AnalysisPreciseRefreshTest, ConfirmedSourceDestinationMissingRequiresFallback, TestSize.Level1)
{
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(1, 1)}, Side::SOURCE));
    task.RecordAlbumResult({1, 99, 0, TEST_CLASSIFY_TYPE, true});
    const auto result = task.Refresh(rdb_);
    EXPECT_NE(result.error, E_OK);
    EXPECT_FALSE(result.fullRefresh);
    EXPECT_EQ(result.failedAlbumIds, (std::unordered_set<int32_t> {99}));
}

HWTEST_F(AnalysisPreciseRefreshTest, MalformedFinalAlbumMetadataRequestsFullFallback, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10)), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE AnalysisAlbum SET is_cover_satisfied=-1"), E_OK);
    const auto result = task.Refresh(rdb_);
    EXPECT_NE(result.error, E_OK);
    EXPECT_TRUE(result.fullRefresh);
}


HWTEST_F(AnalysisPreciseRefreshTest, ReinitializationDoesNotOverwriteSavedBaseline, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 3)), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE AnalysisAlbum SET count=99"), E_OK);
    EXPECT_FALSE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    EXPECT_FALSE(task.IsTaskComplete());
    EXPECT_EQ(task.FindOriginalAlbum(Side::TARGET, 10)->count, 3);
    EXPECT_TRUE(task.Refresh(rdb_).fullRefresh);
}

HWTEST_F(AnalysisPreciseRefreshTest, SelfAlbumBindingIsTerminalRatherThanCycle, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 3)), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    task.RecordTargetAlbumMerge(10, 10);
    ExpectSuccess(task.Refresh(rdb_));
    EXPECT_EQ(ReadAlbum(10).count, 3);
}

HWTEST_F(AnalysisPreciseRefreshTest, CoverRoundDoesNotUseAnotherAlbumsCandidate, TestSize.Level1)
{
    for (int32_t id : {1, 2}) {
        ASSERT_EQ(Photo(id), E_OK);
        ASSERT_EQ(Album(TestAlbum(id * 10, 1, TEST_CLASSIFY_TYPE, id, TEST_DEFAULT_COVER)), E_OK);
        ASSERT_EQ(Map(id * 10, id), E_OK);
    }
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(30, 1, TEST_CLASSIFY_TYPE, 2, MANUAL_COVER),
        TestAlbum(40, 1, TEST_CLASSIFY_TYPE, 1, MANUAL_COVER)}, Side::SOURCE));
    task.RecordAlbumResult({30, 10, 0, TEST_CLASSIFY_TYPE, true});
    task.RecordAlbumResult({40, 20, 0, TEST_CLASSIFY_TYPE, true});
    ExpectSuccess(task.Refresh(rdb_));
    EXPECT_EQ(ReadAlbum(10).coverUri, TestPhotoUri(1));
    EXPECT_EQ(ReadAlbum(20).coverUri, TestPhotoUri(2));
    EXPECT_EQ(ReadAlbum(10).count, 1);
    EXPECT_EQ(ReadAlbum(20).count, 1);
}

HWTEST_F(AnalysisPreciseRefreshTest, EmptyAlbumClearsSatisfiedFlagWithoutUri, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 1, TEST_CLASSIFY_TYPE, 0, MANUAL_COVER)), E_OK);
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1}));
    ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM Photos"), E_OK);
    ExpectSuccess(task.Refresh(rdb_));
    EXPECT_EQ(ReadAlbum(10).count, 0);
    EXPECT_EQ(ReadAlbum(10).coverSatisfied, 0);
    EXPECT_TRUE(ReadAlbum(10).coverUri.empty());
}

HWTEST_F(AnalysisPreciseRefreshTest, SameCoverUriCanStillChangeManualSatisfiedState, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 1, TEST_CLASSIFY_TYPE, 1, TEST_DEFAULT_COVER)), E_OK);
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(20, 1, TEST_CLASSIFY_TYPE, 1, MANUAL_COVER)}, Side::SOURCE));
    task.RecordAlbumResult({20, 10, 0, TEST_CLASSIFY_TYPE, true});
    ExpectSuccess(task.Refresh(rdb_));
    EXPECT_EQ(ReadAlbum(10).coverUri, TestPhotoUri(1));
    EXPECT_EQ(ReadAlbum(10).coverSatisfied, MANUAL_COVER);
}

HWTEST_F(AnalysisPreciseRefreshTest, SourceAndTargetModeNameMismatchFallsBack, TestSize.Level1)
{
    auto target = TestAlbum(10, 1, MODE_TYPE);
    target.albumName = "4";
    ASSERT_EQ(Album(target), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    auto source = TestAlbum(20, 1, MODE_TYPE);
    source.albumName = "13";
    ASSERT_TRUE(task.RecordOriginalAlbums({source}, Side::SOURCE));
    task.RecordAlbumResult({20, 10, 0, MODE_TYPE, true});
    const auto result = task.Refresh(rdb_);
    EXPECT_NE(result.error, E_OK);
    EXPECT_FALSE(result.fullRefresh);
    EXPECT_EQ(result.failedAlbumIds, (std::unordered_set<int32_t> {10}));
}

HWTEST_F(AnalysisPreciseRefreshTest, MergedBaselineOverflowCannotWrapIntoSuccessfulCount, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10)), E_OK);
    ASSERT_EQ(Album(TestAlbum(20)), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    task.data_.targetAlbums.at(10).count = INT64_MAX;
    task.data_.targetAlbums.at(20).count = 1;
    task.RecordTargetAlbumMerge(10, 20);
    ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM AnalysisAlbum WHERE album_id=10"), E_OK);
    const auto result = task.Refresh(rdb_);
    EXPECT_NE(result.error, E_OK);
    EXPECT_FALSE(result.fullRefresh);
    EXPECT_EQ(result.failedAlbumIds, (std::unordered_set<int32_t> {20}));
    EXPECT_EQ(ReadAlbum(20).count, 0);
}

} // namespace OHOS::Media

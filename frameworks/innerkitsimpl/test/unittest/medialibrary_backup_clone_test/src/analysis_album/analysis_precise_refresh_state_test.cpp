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
class AnalysisPreciseStateTest : public testing::Test {};

HWTEST_F(AnalysisPreciseStateTest, CountValidBoundaries, TestSize.Level1)
{
    const std::vector<std::tuple<int64_t, int64_t, int64_t, int32_t>> cases = {
        {0, 0, 0, 0}, {80, 1, 2, 81}, {80, 1, 0, 79}, {80, 1, 1, 80},
        {INT32_MAX, 0, 0, INT32_MAX}, {INT64_MAX, INT64_MAX, 1, 1}, {4, 4, 0, 0}
    };
    for (const auto &[base, before, after, expected] : cases) {
        SCOPED_TRACE(base);
        const auto result = AnalysisAlbumRefreshTask::CalculateFinalCount(base, before, after);
        EXPECT_TRUE(result.complete);
        EXPECT_EQ(result.finalCount, expected);
    }
}

HWTEST_F(AnalysisPreciseStateTest, CountRejectsInvalidAndOverflow, TestSize.Level1)
{
    const std::vector<std::tuple<int64_t, int64_t, int64_t>> cases = {
        {-1, 0, 0}, {1, -1, 0}, {1, 2, 0}, {1, 0, -1}, {INT64_MAX, 0, 0},
        {INT32_MAX, 0, 1}, {1, 0, INT64_MAX}, {0, 0, INT64_MIN}
    };
    for (const auto &[base, before, after] : cases) {
        SCOPED_TRACE(base);
        EXPECT_FALSE(AnalysisAlbumRefreshTask::CalculateFinalCount(base, before, after).complete);
    }
}

HWTEST_F(AnalysisPreciseStateTest, SupportedSubtypesAndCoverParsing, TestSize.Level1)
{
    for (auto type : {PhotoAlbumSubType::CLASSIFY, PhotoAlbumSubType::GEOGRAPHY_LOCATION,
        PhotoAlbumSubType::GEOGRAPHY_CITY, PhotoAlbumSubType::SHOOTING_MODE, PhotoAlbumSubType::PORTRAIT,
        PhotoAlbumSubType::GROUP_PHOTO, PhotoAlbumSubType::HIGHLIGHT, PhotoAlbumSubType::HIGHLIGHT_SUGGESTIONS}) {
        EXPECT_TRUE(IsSupportedAnalysisSubtype(static_cast<int32_t>(type)));
    }
    EXPECT_FALSE(IsSupportedAnalysisSubtype(-1));
    EXPECT_EQ(TestAlbum(1, 0, TEST_CLASSIFY_TYPE, 123).CoverFileId(), 123);
    for (const auto &uri : {"", "garbage", "file://media/Photo/0", "file://media/Photo/-1",
        "file://media/Photo/2147483648", "file://media/Photo/12x"}) {
        auto album = TestAlbum(1);
        album.coverUri = uri;
        EXPECT_EQ(album.CoverFileId(), 0) << uri;
    }
}

HWTEST_F(AnalysisPreciseStateTest, RegistrationRejectsWholeInvalidBatch, TestSize.Level1)
{
    AnalysisAlbumRefreshTask task;
    EXPECT_FALSE(task.RecordOriginalAlbums({TestAlbum(1), TestAlbum(1)}, Side::TARGET));
    EXPECT_TRUE(task.GetFacts().targetAlbums.empty());
    EXPECT_FALSE(task.RecordOriginalAlbums({TestAlbum(1), TestAlbum(0)}, Side::TARGET));
    EXPECT_FALSE(task.RecordOriginalAlbums({TestAlbum(1, 0, -1)}, Side::TARGET));
    ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(1)}, Side::TARGET));
    EXPECT_FALSE(task.RecordOriginalAlbums({TestAlbum(1)}, Side::TARGET));
    ASSERT_TRUE(task.PrepareTargetDirectory({}));
    EXPECT_FALSE(task.RecordOriginalAlbums({TestAlbum(2)}, Side::TARGET));
    EXPECT_TRUE(task.RecordOriginalAlbums({TestAlbum(1)}, Side::SOURCE));
    EXPECT_EQ(task.FindOriginalAlbum(Side::SOURCE, 99), nullptr);
}

HWTEST_F(AnalysisPreciseStateTest, SealPreventsWritesAndClearResetsTask, TestSize.Level1)
{
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(1)}, Side::TARGET));
    task.Seal();
    EXPECT_FALSE(task.RecordOriginalAlbums({TestAlbum(2)}, Side::TARGET));
    EXPECT_FALSE(task.PrepareTargetDirectory({}));
    EXPECT_FALSE(task.CaptureBefore(nullptr, {1}));
    EXPECT_FALSE(task.MigrateTargetAssets({}));
    EXPECT_FALSE(task.MigrateTargetAlbums(1, 10));
    task.RecordAlbumResult({2, 3, 0, TEST_CLASSIFY_TYPE, false});
    task.RecordTargetAlbumMerge(1, 2);
    task.MarkTargetAlbumIncomplete(1);
    task.RecordPhotoResults({1}, {});
    task.RecordDeletedTargets({1});
    task.MarkTaskIncomplete();
    EXPECT_TRUE(task.IsTaskComplete());
    EXPECT_TRUE(task.GetFacts().sourceAlbums.empty());
    EXPECT_EQ(task.GetFacts().targetAlbums.at(1).binding.finalAlbumId, 0);
    EXPECT_TRUE(task.GetFacts().targetAlbums.at(1).complete);
    task.Clear();
    EXPECT_TRUE(task.GetFacts().targetAlbums.empty());
    EXPECT_TRUE(task.RecordOriginalAlbums({TestAlbum(2)}, Side::TARGET));
}

HWTEST_F(AnalysisPreciseStateTest, OperationBindingsRespectRetainedSide, TestSize.Level1)
{
    for (const int32_t type : {TEST_CLASSIFY_TYPE, PORTRAIT_TYPE,
        static_cast<int32_t>(PhotoAlbumSubType::GROUP_PHOTO)}) {
        AnalysisAlbumRefreshTask task;
        ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(1, 3, type)}, Side::SOURCE));
        ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(2, 4, type)}, Side::TARGET));
        task.RecordAlbumResult({1, 2, 0, type, true});
        const auto &binding = task.GetFacts().sourceAlbums.at(1).binding;
        EXPECT_EQ(binding.finalAlbumId, 2);
        EXPECT_EQ(binding.includeBaseline, type == TEST_CLASSIFY_TYPE);
    }
}

HWTEST_F(AnalysisPreciseStateTest, FailedOperationInvalidatesPreviousAndReplacement, TestSize.Level1)
{
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(1)}, Side::SOURCE));
    task.RecordAlbumResult({1, 10, 0, TEST_CLASSIFY_TYPE, true});
    task.RecordAlbumResult({1, 20, 30, TEST_CLASSIFY_TYPE, false});
    EXPECT_FALSE(task.GetFacts().sourceAlbums.at(1).complete);
    EXPECT_EQ(task.GetFacts().invalidFinalAlbums, (std::unordered_set<int32_t> {10, 20, 30}));
    task.RecordAlbumResult({99, 0, 0, TEST_CLASSIFY_TYPE, false});
    EXPECT_FALSE(task.IsTaskComplete());
}

HWTEST_F(AnalysisPreciseStateTest, InvalidMergeAndMissingSourceInvalidateTask, TestSize.Level1)
{
    for (const auto &[original, finalId] : std::vector<std::pair<int32_t, int32_t>> {{0, 1}, {1, 0}, {-1, 2}}) {
        AnalysisAlbumRefreshTask task;
        task.RecordTargetAlbumMerge(original, finalId);
        EXPECT_FALSE(task.IsTaskComplete());
    }
    AnalysisAlbumRefreshTask task;
    task.RecordAlbumResult({1, 2, 0, TEST_CLASSIFY_TYPE, true});
    EXPECT_FALSE(task.IsTaskComplete());
    EXPECT_EQ(task.FindOriginalAlbum(Side::SOURCE, 1), nullptr);
}

HWTEST_F(AnalysisPreciseStateTest, AlbumMigrationMovesGroupsAndKeepsCoverOwnership, TestSize.Level1)
{
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(1, 1, PORTRAIT_TYPE, 7), TestAlbum(20)}, Side::TARGET));
    AnalysisAlbumGroupMembers groups {{{1, "g"}}, {{"g", {1}}}};
    ASSERT_TRUE(task.PrepareTargetDirectory(groups));
    ASSERT_TRUE(task.MigrateTargetAlbums(100, 10));
    EXPECT_EQ(task.FindOriginalAlbum(Side::TARGET, 1), nullptr);
    ASSERT_NE(task.FindOriginalAlbum(Side::TARGET, 101), nullptr);
    EXPECT_EQ(task.GetFacts().targetGroups.groups.at("g"), (std::vector<int32_t> {101}));
    ASSERT_TRUE(task.MigrateTargetAssets({{7, 70}}));
    EXPECT_EQ(task.FindOriginalAlbum(Side::TARGET, 101)->coverFileId, 70);
    ASSERT_TRUE(task.PrepareTargetDirectory(task.GetFacts().targetGroups));
    EXPECT_EQ(task.GetFacts().targetDirectory.grouped.count(101), 1U);
    EXPECT_EQ(task.GetFacts().targetDirectory.ordinary.count(20), 1U);
}

HWTEST_F(AnalysisPreciseStateTest, AlbumMigrationRejectsCollisionOverflowAndPublishedBinding, TestSize.Level1)
{
    for (const int64_t offset : {int64_t {0}, int64_t {-1}, int64_t {INT32_MAX}}) {
        AnalysisAlbumRefreshTask task;
        ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(1)}, Side::TARGET));
        EXPECT_EQ(task.MigrateTargetAlbums(offset, 1), offset == 0);
    }
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(1), TestAlbum(2)}, Side::TARGET));
    EXPECT_FALSE(task.MigrateTargetAlbums(1, 1));
    EXPECT_EQ(task.GetFacts().targetAlbums.size(), 2U);
    task.RecordTargetAlbumMerge(1, 2);
    EXPECT_FALSE(task.MigrateTargetAlbums(10, 10));
    task.Clear();
    ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(3)}, Side::SOURCE));
    task.RecordAlbumResult({3, 4, 0, TEST_CLASSIFY_TYPE, true});
    EXPECT_FALSE(task.MigrateTargetAlbums(10, 10));
}

HWTEST_F(AnalysisPreciseStateTest, AssetMigrationRejectsCollisionWithoutPublishing, TestSize.Level1)
{
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(1, 1, TEST_CLASSIFY_TYPE, 7),
        TestAlbum(2, 1, TEST_CLASSIFY_TYPE, 8)}, Side::TARGET));
    EXPECT_FALSE(task.MigrateTargetAssets({{7, 8}}));
    EXPECT_FALSE(task.MigrateTargetAssets({{7, 0}}));
    EXPECT_EQ(task.FindOriginalAlbum(Side::TARGET, 1)->coverFileId, 7);
    ASSERT_TRUE(task.MigrateTargetAssets({{7, 8}, {8, 7}}));
    EXPECT_EQ(task.FindOriginalAlbum(Side::TARGET, 1)->coverFileId, 8);
    task.RecordDeletedTargets({8, 99});
    EXPECT_TRUE(task.FindOriginalAlbum(Side::TARGET, 1)->coverDeleted);
    EXPECT_EQ(task.GetFacts().targetCoverage.count(8), 0U);
}

HWTEST_F(AnalysisPreciseStateTest, UncapturedCommitForcesFallback, TestSize.Level1)
{
    AnalysisAlbumRefreshTask task;
    task.RecordPhotoResults({7}, {});
    EXPECT_FALSE(task.IsTaskComplete());
    EXPECT_FALSE(task.MigrateTargetAssets({{7, 8}}));
    EXPECT_FALSE(task.MigrateTargetAlbums(1, 10));
    EXPECT_TRUE(task.Refresh(nullptr).fullRefresh);
}

HWTEST_F(AnalysisPreciseStateTest, IncompleteRegisteredSourceInvalidatesDestination, TestSize.Level1)
{
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(1)}, Side::SOURCE));
    task.RecordAlbumResult({1, 2, 0, TEST_CLASSIFY_TYPE, false});
    task.RecordAlbumResult({1, 3, 4, TEST_CLASSIFY_TYPE, true});
    EXPECT_TRUE(task.IsTaskComplete());
    EXPECT_FALSE(task.GetFacts().sourceAlbums.at(1).complete);
    EXPECT_EQ(task.GetFacts().invalidFinalAlbums, (std::unordered_set<int32_t> {2, 3, 4}));
}

HWTEST_F(AnalysisPreciseStateTest, GroupMigrationRejectsOverflowAndMergesIncompletePlaceholder, TestSize.Level1)
{
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(1, 1, PORTRAIT_TYPE)}, Side::TARGET));
    task.MarkTargetAlbumIncomplete(101);
    ASSERT_TRUE(task.MigrateTargetAlbums(100, 1));
    ASSERT_NE(task.FindOriginalAlbum(Side::TARGET, 101), nullptr);
    EXPECT_FALSE(task.FindOriginalAlbum(Side::TARGET, 101)->complete);
    task.Clear();
    ASSERT_TRUE(task.PrepareTargetDirectory({{{INT32_MAX, "g"}}, {{"g", {INT32_MAX}}}}));
    EXPECT_FALSE(task.MigrateTargetAlbums(1, INT32_MAX));
}


HWTEST_F(AnalysisPreciseStateTest, AlbumResultRejectsZeroDestinationAndSubtypeMismatch, TestSize.Level1)
{
    for (const auto &operation : std::vector<AnalysisAlbumOperation> {
        {1, 0, 0, TEST_CLASSIFY_TYPE, true}, {1, 10, 0, PORTRAIT_TYPE, true}}) {
        AnalysisAlbumRefreshTask task;
        ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(1)}, Side::SOURCE));
        task.RecordAlbumResult(operation);
        EXPECT_FALSE(task.GetFacts().sourceAlbums.at(1).complete);
        EXPECT_EQ(task.GetFacts().sourceAlbums.at(1).binding.finalAlbumId, 0);
    }
}

HWTEST_F(AnalysisPreciseStateTest, GroupMigrationPreservesMembersBeyondUpperBound, TestSize.Level1)
{
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(1, 0, PORTRAIT_TYPE),
        TestAlbum(20, 0, PORTRAIT_TYPE)}, Side::TARGET));
    ASSERT_TRUE(task.PrepareTargetDirectory({{{1, "g"}, {20, "g"}}, {{"g", {1, 20}}}}));
    ASSERT_TRUE(task.MigrateTargetAlbums(5, 10));
    EXPECT_EQ(task.GetFacts().targetGroups.groups.at("g"), (std::vector<int32_t> {6, 20}));
    ASSERT_TRUE(task.PrepareTargetDirectory(task.GetFacts().targetGroups));
    EXPECT_EQ(task.GetFacts().targetDirectory.grouped, (std::unordered_set<int32_t> {6, 20}));
}

HWTEST_F(AnalysisPreciseStateTest, UncapturedReplacementEndpointInvalidatesTask, TestSize.Level1)
{
    for (bool capturedDonor : {false, true}) {
        AnalysisAlbumRefreshTask task;
        ASSERT_TRUE(task.RecordOriginalAlbums({TestAlbum(10, 1, TEST_CLASSIFY_TYPE, 1)}, Side::TARGET));
        if (capturedDonor) {
            task.data_.targetCoverage.at(1) = Coverage::CAPTURED;
        }
        task.RecordPhotoResults({}, {{1, 2}});
        EXPECT_FALSE(task.IsTaskComplete());
    }
}

} // namespace OHOS::Media

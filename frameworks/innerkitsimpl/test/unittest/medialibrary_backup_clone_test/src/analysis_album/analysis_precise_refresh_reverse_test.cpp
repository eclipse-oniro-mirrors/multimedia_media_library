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
#include "reverse_clone_restore.h"

using namespace testing::ext;
namespace OHOS::Media {
HWTEST_F(AnalysisPreciseRefreshTest, ReverseBatchCapturesAllOldDonorsAndUsesSourceIdForInsert, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 2)), E_OK);
    for (int32_t id : {101, 102}) {
        ASSERT_EQ(Photo(id), E_OK);
        ASSERT_EQ(Map(10, id), E_OK);
    }
    ReverseCloneRestore restore;
    restore.mediaLibraryRdb_ = rdb_;
    ASSERT_TRUE(restore.analysisAlbumRefreshContext_.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ReverseClonePhotoBatchContext batch;
    FileInfo file;
    file.fileIdOld = 7;
    file.fileIdNew = 999; // Reverse insertion must use 7, not this forward-style destination.
    batch.validFileInfos.push_back(file);
    file.fileIdOld = 0;
    batch.validFileInfos.push_back(file);
    batch.duplicateDonorMap = {{101, 7}, {102, 7}, {0, 7}};
    ASSERT_TRUE(restore.CaptureReverseAnalysisContributions(batch));
    const auto &facts = restore.analysisAlbumRefreshContext_.GetFacts();
    EXPECT_EQ(facts.targetCoverage.size(), 3U);
    for (int32_t id : {7, 101, 102}) {
        EXPECT_EQ(facts.targetCoverage.at(id), Coverage::CAPTURED);
    }
    EXPECT_EQ(facts.targetAlbums.at(10).capturedCount, 2);
    EXPECT_EQ(facts.targetCoverage.count(999), 0U);
}

HWTEST_F(AnalysisPreciseRefreshTest, ReverseAlbumMembersIncludeAdditionalAffectedAssets, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 1)), E_OK);
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    ReverseCloneRestore restore;
    restore.mediaLibraryRdb_ = rdb_;
    ASSERT_TRUE(restore.analysisAlbumRefreshContext_.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(restore.CaptureAnalysisAlbumMembers({10}, {2}));
    EXPECT_EQ(restore.analysisAlbumRefreshContext_.GetFacts().targetCoverage.size(), 2U);
    restore.analysisAlbumRefreshContext_.MarkTaskIncomplete();
    EXPECT_FALSE(restore.CaptureAnalysisAlbumMembers({10}));
}

HWTEST_F(AnalysisPreciseRefreshTest, ReverseMemberFailureAndExplicitDatabaseSelection, TestSize.Level1)
{
    ReverseCloneRestore restore;
    ASSERT_TRUE(restore.analysisAlbumRefreshContext_.InitializeOriginalAlbums(rdb_, Side::TARGET));
    // Default store is null; explicit database must be honored.
    ASSERT_TRUE(restore.CaptureAnalysisTargetContributions({1}, rdb_));
    EXPECT_FALSE(restore.CaptureAnalysisAlbumMembers({10}));
    EXPECT_FALSE(restore.analysisAlbumRefreshContext_.IsTaskComplete());
}

HWTEST_F(AnalysisPreciseRefreshTest, ReverseObserverPublishesOnlyConfirmedMappingAndResumeSkips, TestSize.Level1)
{
    ReverseCloneRestore restore;
    ASSERT_TRUE(restore.analysisAlbumRefreshContext_.RecordOriginalAlbums({TestAlbum(1)}, Side::SOURCE));
    auto observer = restore.AnalysisAlbumObserver();
    ASSERT_TRUE(static_cast<bool>(observer));
    observer({1, 10, 0, TEST_CLASSIFY_TYPE, true});
    EXPECT_EQ(restore.tableAlbumIdMap_["AnalysisAlbum"].at(1), 10);
    observer({1, 20, 0, TEST_CLASSIFY_TYPE, false});
    EXPECT_EQ(restore.tableAlbumIdMap_["AnalysisAlbum"].at(1), 10);
    restore.useFullAnalysisRefreshForResume_ = true;
    EXPECT_FALSE(static_cast<bool>(restore.AnalysisAlbumObserver()));
    restore.reversePhotoInfoMap_[99] = {};
    restore.RecordAnalysisPhotoResults();
    EXPECT_TRUE(restore.analysisAlbumRefreshContext_.GetFacts().targetCoverage.empty());
}

HWTEST_F(AnalysisPreciseRefreshTest, ReverseIdentityMapDoesNotExpandQueryWorkingSet, TestSize.Level1)
{
    ReverseCloneRestore restore;
    ASSERT_TRUE(restore.analysisAlbumRefreshContext_.InitializeOriginalAlbums(rdb_, Side::TARGET));
    for (int32_t id = 1; id <= 7000; ++id) {
        restore.duplicateAssetMap_[id] = id;
    }
    restore.RecordAnalysisPhotoResults();
    EXPECT_TRUE(restore.analysisAlbumRefreshContext_.IsTaskComplete());
    EXPECT_TRUE(restore.analysisAlbumRefreshContext_.GetFacts().targetCoverage.empty());
}

HWTEST_F(AnalysisPreciseRefreshTest, ReverseCleanupRecordsOnlyActuallyDeletedAssets, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 1, TEST_CLASSIFY_TYPE, 1, MANUAL_COVER)), E_OK);
    ASSERT_EQ(Photo(1), E_OK);
    ReverseCloneRestore restore;
    ASSERT_TRUE(restore.analysisAlbumRefreshContext_.InitializeOriginalAlbums(rdb_, Side::TARGET));
    restore.RecordAnalysisDeletedTargets(rdb_, {1, 2});
    EXPECT_FALSE(restore.analysisAlbumRefreshContext_.FindOriginalAlbum(Side::TARGET, 10)->coverDeleted);
    ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM Photos"), E_OK);
    restore.RecordAnalysisDeletedTargets(rdb_, {1});
    EXPECT_TRUE(restore.analysisAlbumRefreshContext_.FindOriginalAlbum(Side::TARGET, 10)->coverDeleted);
    restore.RecordAnalysisDeletedTargets(rdb_, {});
    restore.RecordAnalysisDeletedTargets(nullptr, {3});
    EXPECT_FALSE(restore.analysisAlbumRefreshContext_.IsTaskComplete());
}

HWTEST_F(AnalysisPreciseRefreshTest, ReverseMigrationUsesCommittedMappingsAndDropsMissingAssets, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 1, TEST_CLASSIFY_TYPE, 1, MANUAL_COVER)), E_OK);
    ASSERT_EQ(Album(TestAlbum(20, 1, TEST_CLASSIFY_TYPE, 2, MANUAL_COVER)), E_OK);
    ReverseCloneRestore restore;
    ASSERT_TRUE(restore.analysisAlbumRefreshContext_.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_EQ(rdb_->ExecuteSql("INSERT INTO tab_cloned_old_photos VALUES(1,101,1)"), E_OK);
    restore.MigrateAnalysisTargetAssets(rdb_, 100, 10);
    EXPECT_TRUE(restore.analysisAlbumRefreshContext_.IsTaskComplete());
    EXPECT_EQ(restore.analysisAlbumRefreshContext_.FindOriginalAlbum(Side::TARGET, 10)->coverFileId, 101);
    EXPECT_TRUE(restore.analysisAlbumRefreshContext_.FindOriginalAlbum(Side::TARGET, 20)->coverDeleted);
}

HWTEST_F(AnalysisPreciseRefreshTest, ReverseMigrationInvalidOffsetAndQueryErrorForceFallback, TestSize.Level1)
{
    for (const int64_t offset : {int64_t {0}, int64_t {INT32_MAX}, int64_t {1}}) {
        ReverseCloneRestore restore;
        ASSERT_TRUE(restore.analysisAlbumRefreshContext_.RecordOriginalAlbums(
            {TestAlbum(10, 1, TEST_CLASSIFY_TYPE, 1)}, Side::TARGET));
        restore.MigrateAnalysisTargetAssets(nullptr, offset, 10);
        EXPECT_FALSE(restore.analysisAlbumRefreshContext_.IsTaskComplete());
    }
}

namespace {
class CleanupRefreshRestore : public ReverseCloneRestore {
public:
    bool cloudSatisfied = false;
    bool IsCloudRestoreSatisfied() override { return cloudSatisfied; }
};
}

HWTEST_F(AnalysisPreciseRefreshTest, CloudCleanupLocalOnlyDoesNotInvalidateBaseline, TestSize.Level1)
{
    ASSERT_EQ(rdb_->ExecuteSql("ALTER TABLE Photos ADD position INTEGER DEFAULT 1"), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("ALTER TABLE Photos ADD dirty INTEGER DEFAULT 1"), E_OK);
    ASSERT_EQ(Album(TestAlbum(10, 1, TEST_CLASSIFY_TYPE, 1)), E_OK);
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    CleanupRefreshRestore restore;
    ASSERT_TRUE(restore.analysisAlbumRefreshContext_.InitializeOriginalAlbums(rdb_, Side::TARGET));
    EXPECT_TRUE(restore.CaptureCloudCleanupContributions(rdb_).empty());
    EXPECT_TRUE(restore.analysisAlbumRefreshContext_.GetFacts().targetAlbums.at(10).complete);
    ExpectSuccess(restore.analysisAlbumRefreshContext_.Refresh(rdb_));
    EXPECT_EQ(ReadAlbum(10).count, 1);
}

HWTEST_F(AnalysisPreciseRefreshTest, CloudCleanupActualDeletionSubtractsOnlyAffectedContribution, TestSize.Level1)
{
    ASSERT_EQ(rdb_->ExecuteSql("ALTER TABLE Photos ADD position INTEGER DEFAULT 1"), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("ALTER TABLE Photos ADD dirty INTEGER DEFAULT 1"), E_OK);
    ASSERT_EQ(Album(TestAlbum(10, 3)), E_OK);
    ASSERT_EQ(Album(TestAlbum(20, 1)), E_OK);
    for (int32_t id : {1, 2, 3}) {
        ASSERT_EQ(Photo(id), E_OK);
        ASSERT_EQ(Map(10, id), E_OK);
    }
    ASSERT_EQ(Map(20, 1), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET position=2 WHERE file_id=2"), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET dirty=-1 WHERE file_id=3"), E_OK);
    CleanupRefreshRestore restore;
    ASSERT_TRUE(restore.analysisAlbumRefreshContext_.InitializeOriginalAlbums(rdb_, Side::TARGET));
    auto ids = restore.CaptureCloudCleanupContributions(rdb_);
    EXPECT_EQ(ids.size(), 2U);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET clean_flag=1,dirty=-1 WHERE position=2"), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM Photos WHERE dirty=-1"), E_OK);
    restore.RecordAnalysisDeletedTargets(rdb_, ids);
    ExpectSuccess(restore.analysisAlbumRefreshContext_.Refresh(rdb_));
    EXPECT_EQ(ReadAlbum(10).count, 1);
    EXPECT_EQ(ReadAlbum(20).count, 1);
}

HWTEST_F(AnalysisPreciseRefreshTest, CloudCleanupPartialFailureUsesActualFinalRows, TestSize.Level1)
{
    ASSERT_EQ(rdb_->ExecuteSql("ALTER TABLE Photos ADD position INTEGER DEFAULT 2"), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("ALTER TABLE Photos ADD dirty INTEGER DEFAULT 1"), E_OK);
    ASSERT_EQ(Album(TestAlbum(10, 2)), E_OK);
    for (int32_t id : {1, 2}) {
        ASSERT_EQ(Photo(id), E_OK);
        ASSERT_EQ(Map(10, id), E_OK);
    }
    CleanupRefreshRestore restore;
    ASSERT_TRUE(restore.analysisAlbumRefreshContext_.InitializeOriginalAlbums(rdb_, Side::TARGET));
    auto ids = restore.CaptureCloudCleanupContributions(rdb_);
    // One mark succeeds but deletion fails; the other mark fails entirely.
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET clean_flag=1,dirty=-1 WHERE file_id=1"), E_OK);
    restore.RecordAnalysisDeletedTargets(rdb_, ids);
    ExpectSuccess(restore.analysisAlbumRefreshContext_.Refresh(rdb_));
    EXPECT_EQ(ReadAlbum(10).count, 1);
}

HWTEST_F(AnalysisPreciseRefreshTest, CloudCleanupQueryFailureKeepsPersistentFallback, TestSize.Level1)
{
    CleanupRefreshRestore restore;
    ASSERT_TRUE(restore.analysisAlbumRefreshContext_.InitializeOriginalAlbums(rdb_, Side::TARGET));
    // Missing cleanup columns must not be treated as an empty candidate set.
    EXPECT_TRUE(restore.CaptureCloudCleanupContributions(rdb_).empty());
    EXPECT_FALSE(restore.analysisAlbumRefreshContext_.IsTaskComplete());
    EXPECT_TRUE(restore.analysisAlbumRefreshContext_.Refresh(rdb_).fullRefresh);
}

HWTEST_F(AnalysisPreciseRefreshTest, CloudCleanupResumeAndCloudSatisfiedSkipCapture, TestSize.Level1)
{
    CleanupRefreshRestore restore;
    restore.useFullAnalysisRefreshForResume_ = true;
    EXPECT_TRUE(restore.CaptureCloudCleanupContributions(nullptr).empty());
    EXPECT_TRUE(restore.analysisAlbumRefreshContext_.IsTaskComplete());
    restore.useFullAnalysisRefreshForResume_ = false;
    restore.cloudSatisfied = true;
    EXPECT_TRUE(restore.CaptureCloudCleanupContributions(nullptr).empty());
    EXPECT_TRUE(restore.analysisAlbumRefreshContext_.IsTaskComplete());
}

HWTEST_F(AnalysisPreciseRefreshTest, CloudCleanupDaoFailureDoesNotPublishPartialIds, TestSize.Level1)
{
    ASSERT_EQ(rdb_->ExecuteSql("ALTER TABLE Photos ADD position INTEGER DEFAULT 2"), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("ALTER TABLE Photos ADD dirty INTEGER DEFAULT 1"), E_OK);
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(Photo(0), E_OK);
    std::vector<int32_t> ids = {99};
    EXPECT_EQ(AnalysisAlbumRefreshDb(rdb_).QueryCloudCleanupAssetIds(ids), E_HAS_DB_ERROR);
    EXPECT_TRUE(ids.empty());
    ids.push_back(99);
    EXPECT_EQ(AnalysisAlbumRefreshDb(nullptr).QueryCloudCleanupAssetIds(ids), E_HAS_DB_ERROR);
    EXPECT_TRUE(ids.empty());
}

HWTEST_F(AnalysisPreciseRefreshTest, PortraitAndGroupKeepValidDefaultCoverWithZeroDelta, TestSize.Level1)
{
    for (int32_t type : {static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT),
        static_cast<int32_t>(PhotoAlbumSubType::GROUP_PHOTO)}) {
        const int32_t album = type;
        ASSERT_EQ(Album(TestAlbum(album, 2, type, 1, 1)), E_OK);
        ASSERT_EQ(rdb_->ExecuteSql("UPDATE AnalysisAlbum SET group_tag=album_id"), E_OK);
    }
    ASSERT_EQ(Photo(1, 1), E_OK);
    ASSERT_EQ(Photo(2, 100), E_OK);
    for (int32_t album : {static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT),
        static_cast<int32_t>(PhotoAlbumSubType::GROUP_PHOTO)}) {
        ASSERT_EQ(Map(album, 1), E_OK);
        ASSERT_EQ(Map(album, 2), E_OK);
    }
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1, 2}));
    // No face table row: retaining an existing valid cover must not require default ranking.
    ExpectSuccess(task.Refresh(rdb_));
    for (int32_t album : {static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT),
        static_cast<int32_t>(PhotoAlbumSubType::GROUP_PHOTO)}) {
        EXPECT_EQ(ReadAlbum(album).count, 2);
        EXPECT_EQ(ReadAlbum(album).coverUri, TestPhotoUri(1));
        EXPECT_EQ(ReadAlbum(album).coverSatisfied, 1);
    }
}

HWTEST_F(AnalysisPreciseRefreshTest, PortraitKeepsValidDefaultCoverWhenCountIncreases, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 1, PORTRAIT_TYPE, 1, 1)), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE AnalysisAlbum SET group_tag='g'"), E_OK);
    ASSERT_EQ(Photo(1, 1), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {2}));
    ASSERT_EQ(Photo(2, 100), E_OK);
    ASSERT_EQ(Map(10, 2), E_OK);
    task.RecordPhotoResults({2}, {});
    ExpectSuccess(task.Refresh(rdb_));
    EXPECT_EQ(ReadAlbum(10).count, 2);
    EXPECT_EQ(ReadAlbum(10).coverUri, TestPhotoUri(1));
}

HWTEST_F(AnalysisPreciseRefreshTest, PortraitDefaultCoverFollowsMigratedIdAndPath, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 2, PORTRAIT_TYPE, 1, 1)), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE AnalysisAlbum SET group_tag='g'"), E_OK);
    ASSERT_EQ(Photo(1, 1), E_OK);
    ASSERT_EQ(Photo(2, 100), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    ASSERT_EQ(Map(10, 2), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1, 2}));
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET file_id=101,data='/storage/cloud/files/Photo/9/new.jpg',"
        "display_name='new.jpg' WHERE file_id=1"), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE AnalysisPhotoMap SET map_asset=101 WHERE map_asset=1"), E_OK);
    ASSERT_TRUE(task.MigrateTargetAssets({{1, 101}}));
    ExpectSuccess(task.Refresh(rdb_));
    EXPECT_EQ(ReadAlbum(10).count, 2);
    EXPECT_EQ(ReadAlbum(10).coverUri, MediaFileUri::GetPhotoUri("101",
        "/storage/cloud/files/Photo/9/new.jpg", "new.jpg"));
}

HWTEST_F(AnalysisPreciseRefreshTest, PortraitInvalidDefaultCoverStillReselects, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 2, PORTRAIT_TYPE, 1, 1)), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE AnalysisAlbum SET group_tag='g'"), E_OK);
    ASSERT_EQ(Photo(1, 1), E_OK);
    ASSERT_EQ(Photo(2, 100), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    ASSERT_EQ(Map(10, 2), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("INSERT INTO tab_analysis_image_face VALUES(2,'g',0,1.0,1)"), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1}));
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET hidden=1 WHERE file_id=1"), E_OK);
    ExpectSuccess(task.Refresh(rdb_));
    EXPECT_EQ(ReadAlbum(10).count, 1);
    EXPECT_EQ(ReadAlbum(10).coverUri, TestPhotoUri(2));
}

HWTEST_F(AnalysisPreciseRefreshTest, ClassifyCoverUsesTakenTimeInsteadOfAddedTime, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 2)), E_OK);
    ASSERT_EQ(Photo(1, 100), E_OK);
    ASSERT_EQ(Photo(2, 200), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    ASSERT_EQ(Map(10, 2), E_OK);
    // Photo 1 was taken later, but imported earlier than photo 2.
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET date_taken=300 WHERE file_id=1"), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1, 2}));
    ExpectSuccess(task.Refresh(rdb_));
    EXPECT_EQ(ReadAlbum(10).count, 2);
    EXPECT_EQ(ReadAlbum(10).coverUri, TestPhotoUri(1));
}

HWTEST_F(AnalysisPreciseRefreshTest, ClassifyCoverTakenTimeTieUsesDescendingId, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 2)), E_OK);
    ASSERT_EQ(Photo(1, 200), E_OK);
    ASSERT_EQ(Photo(2, 100), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    ASSERT_EQ(Map(10, 2), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET date_taken=300"), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1, 2}));
    ExpectSuccess(task.Refresh(rdb_));
    EXPECT_EQ(ReadAlbum(10).coverUri, TestPhotoUri(2));
}

} // namespace OHOS::Media

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
#include "result_set_utils.h"

using namespace testing::ext;
namespace OHOS::Media {
HWTEST_F(AnalysisPreciseRefreshTest, DirectoryRejectsConflictsAndInvalidModesAtomically, TestSize.Level1)
{
    AnalysisAlbumQueryDirectory directory;
    ASSERT_TRUE(AnalysisAlbumRefreshDb::PrepareQueryDirectory({{1, TEST_CLASSIFY_TYPE, "a"},
        {1, TEST_CLASSIFY_TYPE, "a"}, {2, PORTRAIT_TYPE, "b"}}, directory));
    EXPECT_EQ(directory.ordinary.size(), 1U);
    EXPECT_EQ(directory.grouped.size(), 1U);
    for (const auto &scopes : std::vector<std::vector<AnalysisAlbumQueryScope>> {
        {{0, TEST_CLASSIFY_TYPE, "a"}}, {{1, 0, "a"}}, {{1, TEST_CLASSIFY_TYPE, "a"}, {1, PORTRAIT_TYPE, "a"}},
        {{1, TEST_CLASSIFY_TYPE, "a"}, {1, TEST_CLASSIFY_TYPE, "b"}}, {{3, MODE_TYPE, "-1"}}}) {
        EXPECT_FALSE(AnalysisAlbumRefreshDb::PrepareQueryDirectory(scopes, directory));
        EXPECT_EQ(directory.ordinary, (std::unordered_set<int32_t> {1}));
    }
}

HWTEST_F(AnalysisPreciseRefreshTest, QueryAlbumsFiltersSubtypesAndPublishesGroups, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(1)), E_OK);
    ASSERT_EQ(Album(TestAlbum(2, 0, -1)), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE AnalysisAlbum SET group_tag='g' WHERE album_id=1"), E_OK);
    std::vector<AnalysisAlbumMetadata> albums;
    AnalysisAlbumGroupMembers groups;
    AnalysisAlbumRefreshDb db(rdb_);
    ASSERT_EQ(db.QueryAlbums(albums, true, &groups), E_OK);
    ASSERT_EQ(albums.size(), 1U);
    EXPECT_EQ(albums.front().albumId, 1);
    EXPECT_EQ(groups.groups.at("g"), (std::vector<int32_t> {1}));
    ASSERT_EQ(db.QueryAlbums(albums, false), E_OK);
    EXPECT_EQ(albums.size(), 2U);
}

HWTEST_F(AnalysisPreciseRefreshTest, MalformedAlbumDoesNotPublishPartialResults, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(1)), E_OK);
    ASSERT_EQ(Album(TestAlbum(2, 0, TEST_CLASSIFY_TYPE, 0, 255)), E_OK);
    std::vector<AnalysisAlbumMetadata> albums {TestAlbum(99)};
    AnalysisAlbumGroupMembers groups {{{99, "sentinel"}}, {{"sentinel", {99}}}};
    EXPECT_NE(AnalysisAlbumRefreshDb(rdb_).QueryAlbums(albums, true, &groups), E_OK);
    EXPECT_TRUE(albums.empty());
    EXPECT_EQ(groups.albumGroups.count(99), 1U);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE AnalysisAlbum SET is_cover_satisfied=-1 WHERE album_id=2"), E_OK);
    EXPECT_NE(AnalysisAlbumRefreshDb(rdb_).QueryAlbums(albums), E_OK);
}

HWTEST_F(AnalysisPreciseRefreshTest, AllVisibilityConditionsExcludeCountButNotMembership, TestSize.Level1)
{
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    const std::vector<std::pair<std::string, int32_t>> fields = {
        {"sync_status", 1}, {"clean_flag", 1}, {"hidden", 1}, {"date_trashed", 1},
        {"time_pending", 1}, {"is_temp", 1}, {"is_shared", 1}, {"burst_cover_level", 0}
    };
    const std::vector<AnalysisAlbumQueryScope> scopes {{10, TEST_CLASSIFY_TYPE, "test"}};
    for (const auto &[field, excluded] : fields) {
        SCOPED_TRACE(field);
        ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET " + field + "=" + std::to_string(excluded)), E_OK);
        EXPECT_TRUE(Contributions(scopes, {1}).empty());
        EXPECT_EQ(Contributions(scopes, {1}, {}, true).size(), 1U);
        ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET " + field + "=" +
            std::to_string(field == "burst_cover_level" ? 1 : 0)), E_OK);
    }
    EXPECT_EQ(Contributions(scopes, {1}).size(), 1U);
}

HWTEST_F(AnalysisPreciseRefreshTest, GroupRelationsDeduplicateAcrossMembersAndInputIds, TestSize.Level1)
{
    ASSERT_EQ(Photo(1), E_OK);
    for (int32_t album : {10, 20, 30, 40}) {
        ASSERT_EQ(Map(album, 1), E_OK);
        ASSERT_EQ(Map(album, 1), E_OK);
    }
    AnalysisAlbumGroupMembers groups {{{20, "g"}, {30, "g"}}, {{"g", {20, 30}}}};
    const auto result = Contributions({{10, TEST_CLASSIFY_TYPE, ""}, {20, PORTRAIT_TYPE, ""},
        {30, PORTRAIT_TYPE, ""}}, {1, 1, 2}, groups);
    EXPECT_EQ(result, (std::set<std::pair<int32_t, int32_t>> {{10, 1}, {20, 1}, {30, 1}}));
}

HWTEST_F(AnalysisPreciseRefreshTest, ContributionBatchBoundaryAndDuplicateInput, TestSize.Level1)
{
    ASSERT_EQ(rdb_->ExecuteSql("WITH RECURSIVE ids(x) AS (SELECT 1 UNION ALL SELECT x+1 FROM ids WHERE x<801) "
        "INSERT INTO Photos(file_id) SELECT x FROM ids"), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("INSERT INTO AnalysisPhotoMap SELECT 10,file_id FROM Photos"), E_OK);
    std::vector<int32_t> ids;
    for (int32_t id = 1; id <= 801; ++id) {
        ids.push_back(id);
        ids.push_back(id);
    }
    EXPECT_EQ(Contributions({{10, TEST_CLASSIFY_TYPE, ""}}, ids).size(), 801U);
    EXPECT_TRUE(Contributions({{10, TEST_CLASSIFY_TYPE, ""}}, {}).empty());
    EXPECT_TRUE(Contributions({}, {1}).empty());
}

HWTEST_F(AnalysisPreciseRefreshTest, ShootingModesShareOneProjectionAndRespectVisibility, TestSize.Level1)
{
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(Photo(2), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET subtype=" +
        std::to_string(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO))), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET is_shared=1 WHERE file_id=2"), E_OK);
    const std::string moving = std::to_string(static_cast<int32_t>(ShootingModeAlbumType::MOVING_PICTURE));
    const std::string raw = std::to_string(static_cast<int32_t>(ShootingModeAlbumType::RAW_IMAGE_ALBUM));
    const std::vector<AnalysisAlbumQueryScope> scopes {{10, MODE_TYPE, moving}, {11, MODE_TYPE, moving},
        {12, MODE_TYPE, raw}};
    AnalysisAlbumQueryDirectory directory;
    ASSERT_TRUE(AnalysisAlbumRefreshDb::PrepareQueryDirectory(scopes, directory));
    ASSERT_EQ(directory.shootingQueries.size(), 1U);
    EXPECT_EQ(directory.shootingQueries.front().albums.size(), 2U);
    EXPECT_EQ(Contributions(scopes, {1, 2}),
        (std::set<std::pair<int32_t, int32_t>> {{10, 1}, {11, 1}}));
    EXPECT_EQ(Contributions(scopes, {1, 2}, {}, true).size(), 4U);
}

HWTEST_F(AnalysisPreciseRefreshTest, AllShootingModesBuildAndQuery, TestSize.Level1)
{
    ASSERT_EQ(Photo(1), E_OK);
    std::vector<AnalysisAlbumQueryScope> scopes;
    for (int32_t mode = static_cast<int32_t>(ShootingModeAlbumType::START);
        mode <= static_cast<int32_t>(ShootingModeAlbumType::END); ++mode) {
        scopes.push_back({100 + mode, MODE_TYPE, std::to_string(mode)});
    }
    EXPECT_TRUE(Contributions(scopes, {1}).empty());
    EXPECT_TRUE(Contributions(scopes, {1}, {}, true).empty());
}

HWTEST_F(AnalysisPreciseRefreshTest, NullStoreAndInvalidQueryArguments, TestSize.Level1)
{
    AnalysisAlbumRefreshDb empty(nullptr);
    std::vector<AnalysisAlbumMetadata> albums;
    std::vector<int32_t> ids;
    std::unordered_map<int32_t, std::string> uris;
    std::unordered_map<int32_t, int32_t> mappings;
    NativeRdb::ValuesBucket values;
    std::string cover;
    EXPECT_NE(empty.QueryAlbums(albums), E_OK);
    EXPECT_NE(empty.QueryAssetIds({1}, AnalysisAssetQuery::EXISTING, ids), E_OK);
    EXPECT_FALSE(empty.QueryMigratedPhotoIds({}, mappings));
    EXPECT_NE(empty.QueryPhotoUris({}, uris), E_OK);
    EXPECT_NE(empty.QueryDefaultCover({1, TEST_CLASSIFY_TYPE, ""}, cover), E_OK);
    EXPECT_NE(empty.UpdateAlbum(1, values), E_OK);
    const auto consume = [](int32_t, int32_t) {};
    EXPECT_NE(empty.QueryContributions({}, {1}, consume, {}), E_OK);
    AnalysisAlbumRefreshDb db(rdb_);
    EXPECT_NE(db.QueryContributions({}, {0}, consume, {}), E_OK);
    EXPECT_NE(db.QueryContributions({}, {1}, {}, {}), E_OK);
    EXPECT_NE(db.QueryPhotoUris({-1}, uris), E_OK);
    EXPECT_NE(db.QueryDefaultCover({0, TEST_CLASSIFY_TYPE, ""}, cover), E_OK);
    EXPECT_NE(db.QueryDefaultCover({1, MODE_TYPE, "-1"}, cover), E_OK);
}

HWTEST_F(AnalysisPreciseRefreshTest, DeletionIncludesBurstSiblingsNotUnrelatedOrEmptyBurst, TestSize.Level1)
{
    for (int32_t id : {1, 2, 3, 4}) {
        ASSERT_EQ(Photo(id), E_OK);
    }
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET burst_key='b' WHERE file_id IN (1,2)"), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET burst_key='' WHERE file_id IN (3,4)"), E_OK);
    std::vector<int32_t> ids;
    AnalysisAlbumRefreshDb db(rdb_);
    ASSERT_EQ(db.QueryAssetIds({1, 3}, AnalysisAssetQuery::DELETION_WITH_BURST, ids), E_OK);
    EXPECT_EQ((std::set<int32_t>(ids.begin(), ids.end())), (std::set<int32_t> {1, 2, 3}));
    ASSERT_EQ(Map(10, 2), E_OK);
    ASSERT_EQ(db.QueryAssetIds({10}, AnalysisAssetQuery::ALBUM_MEMBERS, ids), E_OK);
    EXPECT_EQ(ids, (std::vector<int32_t> {2}));
    ASSERT_EQ(db.QueryAssetIds({4, 99}, AnalysisAssetQuery::EXISTING, ids), E_OK);
    EXPECT_EQ(ids, (std::vector<int32_t> {4}));
}

HWTEST_F(AnalysisPreciseRefreshTest, MigratedIdsValidateSequenceIdentityAndDuplicates, TestSize.Level1)
{
    AnalysisAlbumRefreshDb db(rdb_);
    std::unordered_map<int32_t, int32_t> output {{99, 100}};
    EXPECT_FALSE(db.QueryMigratedPhotoIds({{0, 1}}, output));
    EXPECT_FALSE(db.QueryMigratedPhotoIds({{1, 0}}, output));
    EXPECT_FALSE(db.QueryMigratedPhotoIds({{1, 2}, {3, 2}}, output));
    ASSERT_EQ(rdb_->ExecuteSql("INSERT INTO tab_cloned_old_photos VALUES(1,11,1)"), E_OK);
    ASSERT_TRUE(db.QueryMigratedPhotoIds({{1, 11}, {2, 22}}, output));
    EXPECT_EQ(output, (std::unordered_map<int32_t, int32_t> {{1, 11}}));
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE tab_cloned_old_photos SET clone_sequence=2"), E_OK);
    EXPECT_FALSE(db.QueryMigratedPhotoIds({{1, 11}}, output));
    EXPECT_EQ(output, (std::unordered_map<int32_t, int32_t> {{1, 11}}));
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE tab_cloned_old_photos SET clone_sequence=1,old_file_id=3"), E_OK);
    EXPECT_FALSE(db.QueryMigratedPhotoIds({{1, 11}}, output));
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE tab_cloned_old_photos SET old_file_id=1"), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("INSERT INTO tab_cloned_old_photos VALUES(1,11,1)"), E_OK);
    EXPECT_FALSE(db.QueryMigratedPhotoIds({{1, 11}}, output));
}

HWTEST_F(AnalysisPreciseRefreshTest, UriLookupIndependentOfVisibilityAndMissingAssets, TestSize.Level1)
{
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET hidden=1,is_shared=1"), E_OK);
    std::unordered_map<int32_t, std::string> uris;
    ASSERT_EQ(AnalysisAlbumRefreshDb(rdb_).QueryPhotoUris({1, 1, 2}, uris), E_OK);
    EXPECT_EQ(uris.at(1), TestPhotoUri(1));
    EXPECT_TRUE(uris.at(2).empty());
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET display_name=''"), E_OK);
    EXPECT_NE(AnalysisAlbumRefreshDb(rdb_).QueryPhotoUris({1, 2}, uris), E_OK);
    EXPECT_TRUE(uris.empty());
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET display_name='1.jpg',data=''"), E_OK);
    EXPECT_NE(AnalysisAlbumRefreshDb(rdb_).QueryPhotoUris({1}, uris), E_OK);
}

HWTEST_F(AnalysisPreciseRefreshTest, DefaultCoverSelectsNewestEligibleAndHandlesEmpty, TestSize.Level1)
{
    for (int32_t id : {1, 2, 3}) {
        ASSERT_EQ(Photo(id, id), E_OK);
        ASSERT_EQ(Map(10, id), E_OK);
    }
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET is_shared=1 WHERE file_id=3"), E_OK);
    std::string cover;
    AnalysisAlbumRefreshDb db(rdb_);
    ASSERT_EQ(db.QueryDefaultCover({10, TEST_CLASSIFY_TYPE, ""}, cover), E_OK);
    EXPECT_EQ(cover, TestPhotoUri(2));
    ASSERT_EQ(db.QueryDefaultCover({99, TEST_CLASSIFY_TYPE, ""}, cover), E_OK);
    EXPECT_TRUE(cover.empty());
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET data='' WHERE file_id=2"), E_OK);
    EXPECT_NE(db.QueryDefaultCover({10, TEST_CLASSIFY_TYPE, ""}, cover), E_OK);
}

HWTEST_F(AnalysisPreciseRefreshTest, UpdateRequiresExactlyOneRowAndRollsBackFailure, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(1, 2)), E_OK);
    AnalysisAlbumRefreshDb db(rdb_);
    NativeRdb::ValuesBucket empty;
    EXPECT_EQ(db.UpdateAlbum(999, empty), E_OK);
    NativeRdb::ValuesBucket values;
    values.PutInt("count", 3);
    EXPECT_NE(db.UpdateAlbum(999, values), E_OK);
    ASSERT_EQ(db.UpdateAlbum(1, values), E_OK);
    EXPECT_EQ(ReadAlbum(1).count, 3);
    ASSERT_EQ(rdb_->ExecuteSql("CREATE TRIGGER deny_count BEFORE UPDATE ON AnalysisAlbum "
        "BEGIN SELECT RAISE(ABORT,'test failure'); END"), E_OK);
    values.PutInt("count", 4);
    EXPECT_NE(db.UpdateAlbum(1, values), E_OK);
    EXPECT_EQ(ReadAlbum(1).count, 3);
}

HWTEST_F(AnalysisPreciseRefreshTest, MissingTablesFailWithoutPublishingCollections, TestSize.Level1)
{
    ASSERT_EQ(rdb_->ExecuteSql("DROP TABLE Photos"), E_OK);
    AnalysisAlbumRefreshDb db(rdb_);
    std::vector<int32_t> ids {99};
    std::unordered_map<int32_t, std::string> uris {{99, "sentinel"}};
    std::string cover = "sentinel";
    EXPECT_NE(db.QueryAssetIds({1}, AnalysisAssetQuery::EXISTING, ids), E_OK);
    EXPECT_TRUE(ids.empty());
    EXPECT_NE(db.QueryPhotoUris({1}, uris), E_OK);
    EXPECT_TRUE(uris.empty());
    EXPECT_NE(db.QueryDefaultCover({1, TEST_CLASSIFY_TYPE, ""}, cover), E_OK);
    EXPECT_TRUE(cover.empty());
}

HWTEST_F(AnalysisPreciseRefreshTest, ReadRowsRejectsNullAndVisitorFailure, TestSize.Level1)
{
    EXPECT_FALSE(ResultSetUtils::ReadRows(nullptr, [](const auto &) { return true; }));
    EXPECT_TRUE(ResultSetUtils::ReadRows(rdb_->QuerySql("SELECT file_id FROM Photos"),
        [](const auto &) { return true; }));
    ASSERT_EQ(Photo(1), E_OK);
    int visits = 0;
    EXPECT_FALSE(ResultSetUtils::ReadRows(rdb_->QuerySql("SELECT file_id FROM Photos"),
        [&visits](const auto &) { ++visits; return false; }));
    EXPECT_EQ(visits, 1);
    EXPECT_TRUE(ResultSetUtils::ReadRows(rdb_->QuerySql("SELECT file_id FROM Photos"),
        [](const auto &) { return true; }, false));
}

HWTEST_F(AnalysisPreciseRefreshTest, ShootingDefaultCoverUsesDateTakenThenFileId, TestSize.Level1)
{
    for (int32_t id : {1, 2, 3}) {
        ASSERT_EQ(Photo(id, 100), E_OK);
    }
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET mime_type='image/x-adobe-dng'"), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET is_shared=1 WHERE file_id=3"), E_OK);
    std::string uri;
    const std::string mode = std::to_string(static_cast<int32_t>(ShootingModeAlbumType::RAW_IMAGE_ALBUM));
    ASSERT_EQ(AnalysisAlbumRefreshDb(rdb_).QueryDefaultCover({10, MODE_TYPE, mode}, uri), E_OK);
    EXPECT_EQ(uri, TestPhotoUri(2));
}

HWTEST_F(AnalysisPreciseRefreshTest, AlbumZeroIdAndMissingTableAreRejected, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(0)), E_OK);
    std::vector<AnalysisAlbumMetadata> albums;
    AnalysisAlbumGroupMembers groups;
    EXPECT_NE(AnalysisAlbumRefreshDb(rdb_).QueryAlbums(albums), E_OK);
    EXPECT_NE(AnalysisAlbumRefreshDb(rdb_).QueryAlbums(albums, true, &groups), E_OK);
    EXPECT_TRUE(albums.empty());
    ASSERT_EQ(rdb_->ExecuteSql("DROP TABLE AnalysisAlbum"), E_OK);
    EXPECT_NE(AnalysisAlbumRefreshDb(rdb_).QueryAlbums(albums), E_OK);
}

HWTEST_F(AnalysisPreciseRefreshTest, InvalidMigratedRowDoesNotPublishOrOverwritePreviousResult, TestSize.Level1)
{
    ASSERT_EQ(rdb_->ExecuteSql("INSERT INTO tab_cloned_old_photos VALUES(0,11,1)"), E_OK);
    std::unordered_map<int32_t, int32_t> result {{99, 100}};
    EXPECT_FALSE(AnalysisAlbumRefreshDb(rdb_).QueryMigratedPhotoIds({{1, 11}}, result));
    EXPECT_EQ(result, (std::unordered_map<int32_t, int32_t> {{99, 100}}));
    ASSERT_EQ(rdb_->ExecuteSql("DROP TABLE tab_cloned_old_photos"), E_OK);
    EXPECT_FALSE(AnalysisAlbumRefreshDb(rdb_).QueryMigratedPhotoIds({{1, 11}}, result));
}


HWTEST_F(AnalysisPreciseRefreshTest, InvalidPhotoInBatchedUriQueryDiscardsPartialResults, TestSize.Level1)
{
    ASSERT_EQ(rdb_->ExecuteSql("WITH RECURSIVE ids(x) AS (SELECT 1 UNION ALL SELECT x+1 FROM ids WHERE x<801) "
        "INSERT INTO Photos(file_id,data,display_name) SELECT x,'/storage/cloud/files/Photo/1/'||x||'.jpg',"
        "x||'.jpg' FROM ids"), E_OK);
    // QueryPhotoUris internally deduplicates with unordered_set. Make both end ranges invalid,
    // then assert the atomic publication contract regardless of iteration order.
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET display_name='' WHERE file_id IN (1,801)"), E_OK);
    std::vector<int32_t> ids;
    for (int32_t id = 1; id <= 801; ++id) {
        ids.push_back(id);
    }
    std::unordered_map<int32_t, std::string> output {{999, "old"}};
    EXPECT_NE(AnalysisAlbumRefreshDb(rdb_).QueryPhotoUris(ids, output), E_OK);
    EXPECT_TRUE(output.empty());
}

HWTEST_F(AnalysisPreciseRefreshTest, GroupedCleanupDoesNotSubtractInvisiblePhotos, TestSize.Level1)
{
    for (const auto subtype : {PhotoAlbumSubType::PORTRAIT, PhotoAlbumSubType::GROUP_PHOTO}) {
        const int32_t album = static_cast<int32_t>(subtype);
        ASSERT_EQ(Album(TestAlbum(album, 1, album)), E_OK);
        ASSERT_EQ(rdb_->ExecuteSql("UPDATE AnalysisAlbum SET group_tag='g'"), E_OK);
    }
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(Photo(2), E_OK);
    ASSERT_EQ(Photo(3), E_OK);
    ASSERT_EQ(Map(PORTRAIT_TYPE, 1), E_OK);
    ASSERT_EQ(Map(PORTRAIT_TYPE, 2), E_OK);
    ASSERT_EQ(Map(PORTRAIT_TYPE, 3), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET sync_status=1 WHERE file_id=1"), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET clean_flag=1 WHERE file_id=2"), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1, 2}));
    ASSERT_EQ(rdb_->ExecuteSql("DELETE FROM Photos WHERE file_id IN (1,2)"), E_OK);
    task.RecordDeletedTargets({1, 2});
    ExpectSuccess(task.Refresh(rdb_));
    EXPECT_EQ(ReadAlbum(PORTRAIT_TYPE).count, 1);
    EXPECT_EQ(ReadAlbum(static_cast<int32_t>(PhotoAlbumSubType::GROUP_PHOTO)).count, 1);
}

HWTEST_F(AnalysisPreciseRefreshTest, GroupedCountAndCoverExcludeInvisiblePhotos, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 1, PORTRAIT_TYPE, 1, TEST_DEFAULT_COVER)), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE AnalysisAlbum SET group_tag='g'"), E_OK);
    ASSERT_EQ(Photo(1, 100), E_OK);
    ASSERT_EQ(Photo(2, 1), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    ASSERT_EQ(Map(10, 2), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET sync_status=1 WHERE file_id=1"), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("INSERT INTO tab_analysis_image_face VALUES(2,'g',0,100,1)"), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1, 2}));
    ExpectSuccess(task.Refresh(rdb_));
    EXPECT_EQ(ReadAlbum(10).count, 1);
    EXPECT_EQ(ReadAlbum(10).coverUri, TestPhotoUri(2));
}

HWTEST_F(AnalysisPreciseRefreshTest, RawDefaultCoverUsesOriginalNameTieBreakAndEmptyAggregate, TestSize.Level1)
{
    const AnalysisAlbumQueryScope scope {10, MODE_TYPE,
        std::to_string(static_cast<int32_t>(ShootingModeAlbumType::RAW_IMAGE_ALBUM))};
    std::string cover;
    AnalysisAlbumRefreshDb db(rdb_);
    ASSERT_EQ(db.QueryDefaultCover(scope, cover), E_OK);
    EXPECT_TRUE(cover.empty());
    ASSERT_EQ(Photo(1, 100), E_OK);
    ASSERT_EQ(Photo(2, 100), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET mime_type='image/x-adobe-dng'"), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET display_name='Z.dng' WHERE file_id=1"), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET display_name='A.dng' WHERE file_id=2"), E_OK);
    ASSERT_EQ(db.QueryDefaultCover(scope, cover), E_OK);
    EXPECT_EQ(cover, MediaFileUri::GetPhotoUri("1", "/storage/cloud/files/Photo/1/1.jpg", "Z.dng"));
}

HWTEST_F(AnalysisPreciseRefreshTest, UriAndCoverEligibilityRemainIndependent, TestSize.Level1)
{
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(Photo(2), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET clean_flag=1 WHERE file_id=1"), E_OK);
    std::unordered_map<int32_t, std::string> uris;
    std::unordered_set<int32_t> eligible;
    AnalysisAlbumRefreshDb db(rdb_);
    ASSERT_EQ(db.QueryPhotoUris({1, 2, 3}, uris, &eligible), E_OK);
    EXPECT_EQ(uris.at(1), TestPhotoUri(1));
    EXPECT_EQ(eligible, (std::unordered_set<int32_t> {2}));
    EXPECT_TRUE(uris.at(3).empty());
    ASSERT_EQ(rdb_->ExecuteSql("DROP TABLE Photos"), E_OK);
    EXPECT_NE(db.QueryPhotoUris({1}, uris, &eligible), E_OK);
    EXPECT_TRUE(uris.empty());
    EXPECT_TRUE(eligible.empty());
}

HWTEST_F(AnalysisPreciseRefreshTest, MixedRelationsSharePhotoVisibilityRules, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10)), E_OK);
    ASSERT_EQ(Album(TestAlbum(20, 0, PORTRAIT_TYPE)), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE AnalysisAlbum SET group_tag='g' WHERE album_id=20"), E_OK);
    for (int32_t id : {1, 2, 3}) {
        ASSERT_EQ(Photo(id), E_OK);
        ASSERT_EQ(Map(10, id), E_OK);
        ASSERT_EQ(Map(20, id), E_OK);
    }
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET sync_status=1 WHERE file_id=2"), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET clean_flag=1 WHERE file_id=3"), E_OK);
    AnalysisAlbumGroupMembers groups;
    groups.albumGroups[20] = "g";
    groups.groups["g"] = {20};
    const std::set<std::pair<int32_t, int32_t>> expected {{10, 1}, {20, 1}};
    EXPECT_EQ(Contributions({{10, TEST_CLASSIFY_TYPE, "test"}, {20, PORTRAIT_TYPE, "test"}},
        {1, 2, 3}, groups), expected);
}

HWTEST_F(AnalysisPreciseRefreshTest, GroupedCleanupMarkExcludesUndeletedPhoto, TestSize.Level1)
{
    ASSERT_EQ(Album(TestAlbum(10, 1, PORTRAIT_TYPE)), E_OK);
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE AnalysisAlbum SET group_tag='g'"), E_OK);
    ASSERT_EQ(Photo(1), E_OK);
    ASSERT_EQ(Map(10, 1), E_OK);
    AnalysisAlbumRefreshTask task;
    ASSERT_TRUE(task.InitializeOriginalAlbums(rdb_, Side::TARGET));
    ASSERT_TRUE(task.CaptureBefore(rdb_, {1}));
    ASSERT_EQ(rdb_->ExecuteSql("UPDATE Photos SET clean_flag=1"), E_OK);
    // Marking changes visibility even when deletion fails.
    ExpectSuccess(task.Refresh(rdb_));
    EXPECT_EQ(ReadAlbum(10).count, 0);
}

HWTEST_F(AnalysisPreciseRefreshTest, NamedValueReadDistinguishesEmptyValuesFromFailure, TestSize.Level1)
{
    for (const auto &sql : {"SELECT 0 AS count, '' AS album_name", "SELECT '' AS album_name, 0 AS count"}) {
        auto rows = rdb_->QuerySql(sql);
        ASSERT_NE(rows, nullptr);
        ASSERT_EQ(rows->GoToFirstRow(), NativeRdb::E_OK);
        int32_t count = -1;
        std::string name = "sentinel";
        EXPECT_TRUE(ResultSetUtils::TryGetValue("count", rows, count));
        EXPECT_EQ(count, 0);
        EXPECT_TRUE(ResultSetUtils::TryGetValue("album_name", rows, name));
        EXPECT_TRUE(name.empty());
        EXPECT_FALSE(ResultSetUtils::TryGetValue("missing", rows, count));
        EXPECT_FALSE(ResultSetUtils::TryGetValue("missing", rows, name));
        rows->Close();
        EXPECT_FALSE(ResultSetUtils::TryGetValue("count", rows, count));
        EXPECT_FALSE(ResultSetUtils::TryGetValue("album_name", rows, name));
    }
    int32_t count = 0;
    EXPECT_FALSE(ResultSetUtils::TryGetValue("count", nullptr, count));
}

HWTEST_F(AnalysisPreciseRefreshTest, NamedValueReadPreservesNativeNullConversion, TestSize.Level1)
{
    auto rows = rdb_->QuerySql("SELECT NULL AS value");
    ASSERT_NE(rows, nullptr);
    ASSERT_EQ(rows->GoToFirstRow(), NativeRdb::E_OK);
    int32_t column = -1;
    ASSERT_EQ(rows->GetColumnIndex("value", column), NativeRdb::E_OK);
    int32_t directCount = -1;
    int32_t namedCount = -1;
    std::string directName = "sentinel";
    std::string namedName = "sentinel";
    const bool intRead = rows->GetInt(column, directCount) == NativeRdb::E_OK;
    const bool stringRead = rows->GetString(column, directName) == NativeRdb::E_OK;
    EXPECT_EQ(ResultSetUtils::TryGetValue("value", rows, namedCount), intRead);
    EXPECT_EQ(ResultSetUtils::TryGetValue("value", rows, namedName), stringRead);
    EXPECT_EQ(namedCount, directCount);
    EXPECT_EQ(namedName, directName);
    rows->Close();
}

} // namespace OHOS::Media

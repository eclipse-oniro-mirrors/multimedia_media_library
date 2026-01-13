/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "HighlightRestoreTest"

#include "highlight_restore_test.h"
#include <string>
#include <thread>
#define private public
#define protected public
#include "media_log.h"
#include "highlight_restore.h"
#include "gallery_source.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#undef private
#undef protected
#include "media_audio_column.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {

const std::string TEST_BACKUP_PATH = "/data/test/gallery1.db";
const int32_t HIGHLIGHT_STATUS_SUCCESS = 1;
const int32_t HIGHLIGHT_STATUS_FAIL = -2;
const int32_t HIGHLIGHT_STATUS_DUPLICATE = -1;
const int32_t TEST_FILE_ID_OLD = 10;
const int32_t TEST_FILE_ID_NEW = 100;
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
const vector<string> CLEAR_SQLS = {
    "DELETE FROM " + PhotoColumn::PHOTOS_TABLE,
    "DELETE FROM " + PhotoAlbumColumns::TABLE + " WHERE " + PhotoAlbumColumns::ALBUM_TYPE + " != " +
        to_string(PhotoAlbumType::SYSTEM),
    "DELETE FROM " + PhotoMap::TABLE,
    "DELETE FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " + PhotoAlbumColumns::ALBUM_SUBTYPE + " != " +
        to_string(PhotoAlbumSubType::SHOOTING_MODE),
    "DELETE FROM " + ANALYSIS_PHOTO_MAP_TABLE,
    "DELETE FROM " + AudioColumn::AUDIOS_TABLE,
    "DELETE FROM tab_analysis_image_face",
    "DELETE FROM tab_analysis_face_tag",
    "DELETE FROM tab_analysis_total",
    "DELETE FROM tab_highlight_album",
    "DELETE FROM tab_highlight_play_info",
};

std::shared_ptr<MediaLibraryRdbStore> g_rdbStore = nullptr;
std::shared_ptr<NativeRdb::RdbStore> g_galleryPtr = nullptr;

void InitDestinationDb()
{
    MEDIA_INFO_LOG("Start Init");
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
}

static void ExecuteSqls(shared_ptr<RdbStore> rdbStore, const vector<string> &sqls)
{
    for (const auto &sql : sqls) {
        int32_t errCode = rdbStore->ExecuteSql(sql);
        if (errCode == E_OK) {
            continue;
        }
        MEDIA_ERR_LOG("Execute %{public}s failed: %{public}d", sql.c_str(), errCode);
    }
}

static void ClearData()
{
    MEDIA_INFO_LOG("Start clear data");
    ExecuteSqls(g_rdbStore->GetRaw(), CLEAR_SQLS);
    MediaLibraryRdbUtils::UpdateAllAlbums(g_rdbStore);
    MEDIA_INFO_LOG("End clear data");
}

void HighlightRestoreTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("SetUpTestCase");
    InitDestinationDb();
    GallerySource gallerySource;
    gallerySource.Init(TEST_BACKUP_PATH);
    g_galleryPtr = gallerySource.galleryStorePtr_;
    ASSERT_NE(g_galleryPtr, nullptr);
}

void HighlightRestoreTest::TearDownTestCase(void)
{
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
    MEDIA_INFO_LOG("TearDownTestCase");
}

void HighlightRestoreTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void HighlightRestoreTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(HighlightRestoreTest, highlight_restore_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("highlight_restore_test_001 start");

    shared_ptr<HighlightRestore> highlightRestore = make_shared<HighlightRestore>();
    highlightRestore->Init(2, "1", g_rdbStore->GetRaw(), g_galleryPtr);
    EXPECT_NE(highlightRestore->mediaLibraryRdb_, nullptr);
    EXPECT_NE(highlightRestore->galleryRdb_, nullptr);
    EXPECT_EQ(highlightRestore->sceneCode_, 2);
    std::string albumOdid = "test";
    highlightRestore->RestoreAlbums(albumOdid);
    std::unordered_map<int32_t, PhotoInfo> photoInfoMap;
    PhotoInfo photoInfo;
    photoInfo.fileIdNew = 1;
    photoInfo.cloudPath = "cloud/path/test.jpg";
    photoInfoMap.insert(std::make_pair(1, photoInfo));
    highlightRestore->RestoreMaps(photoInfoMap);
    EXPECT_NE(highlightRestore->albumInfos_.size(), 0);
    ClearData();
}

HWTEST_F(HighlightRestoreTest, highlight_restore_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("highlight_restore_test_002 start");
    shared_ptr<HighlightRestore> highlightRestore = make_shared<HighlightRestore>();
    HighlightRestore::HighlightAlbumInfo info1;
    info1.maxDateAdded = 1;
    highlightRestore->TransferClusterInfo(info1);
    EXPECT_EQ(info1.clusterType, "TYPE_NULL");

    HighlightRestore::HighlightAlbumInfo info2;
    info2.clusterType = "DBSCANTIME";
    info2.albumName = "AttractionsAlbum";
    info2.maxDateAdded = 1;
    highlightRestore->TransferClusterInfo(info2);
    EXPECT_EQ(info2.clusterSubType, "Old_Attraction");

    HighlightRestore::HighlightAlbumInfo info3;
    info3.clusterType = "OTHER_TYPE";
    info3.albumName = "AttractionsAlbum";
    info3.clusterSubType = "AttractionsAlbum";
    info3.clusterCondition = "null";
    info3.maxDateAdded = 1;
    highlightRestore->TransferClusterInfo(info3);
    EXPECT_EQ(info3.clusterSubType, "Old_Attraction");

    HighlightRestore::HighlightAlbumInfo info4;
    info4.clusterType = "OTHER_TYPE";
    info4.albumName = "AttractionsAlbum";
    info4.clusterSubType = "AttractionsAlbum";
    info4.clusterCondition = "not_null";
    info4.maxDateAdded = 1;
    highlightRestore->TransferClusterInfo(info4);
    EXPECT_EQ(info4.clusterSubType, "Old_Attraction");
}

HWTEST_F(HighlightRestoreTest, highlight_restore_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("highlight_restore_test_003 start");
    shared_ptr<HighlightRestore> highlightRestore = make_shared<HighlightRestore>();
    highlightRestore->Init(2, "1", nullptr, g_galleryPtr);
    EXPECT_EQ(highlightRestore->sceneCode_, 2);
    HighlightRestore::HighlightAlbumInfo info;
    info.highlightStatus = HIGHLIGHT_STATUS_SUCCESS;
    highlightRestore->albumInfos_.emplace_back(info);
    highlightRestore->InsertIntoHighlightAlbum();
    EXPECT_EQ(highlightRestore->albumInfos_[0].highlightStatus, HIGHLIGHT_STATUS_FAIL);
}

HWTEST_F(HighlightRestoreTest, highlight_restore_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("highlight_restore_test_004 start");
    shared_ptr<HighlightRestore> highlightRestore = make_shared<HighlightRestore>();
    highlightRestore->Init(2, "1", nullptr, g_galleryPtr);
    EXPECT_EQ(highlightRestore->sceneCode_, 2);
    HighlightRestore::HighlightAlbumInfo info;
    info.highlightStatus = HIGHLIGHT_STATUS_SUCCESS;
    highlightRestore->albumInfos_.emplace_back(info);
    highlightRestore->InsertIntoHighlightCoverAndPlayInfo();
    EXPECT_EQ(highlightRestore->albumInfos_[0].highlightStatus, HIGHLIGHT_STATUS_FAIL);
}

HWTEST_F(HighlightRestoreTest, highlight_restore_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("highlight_restore_test_005 start");
    shared_ptr<HighlightRestore> highlightRestore = make_shared<HighlightRestore>();
    highlightRestore->Init(2, "1", g_rdbStore->GetRaw(), g_galleryPtr);
    EXPECT_EQ(highlightRestore->sceneCode_, 2);
    HighlightRestore::HighlightPhotoInfo highlightPhoto;
    PhotoInfo photoInfo;
    photoInfo.displayName = "test.jpg";
    photoInfo.cloudPath = "cloud/path/test.jpg";
    photoInfo.fileIdNew = 1;
    photoInfo.fileType = 1;
    highlightPhoto.photoInfo = photoInfo;
    highlightPhoto.fileIdOld = 2;
    highlightPhoto.storyIds = "100";
    highlightPhoto.portraitIds = "4,5,6";
    highlightPhoto.hashCode = "3275";
    HighlightRestore::HighlightAlbumInfo info1;
    info1.highlightStatus = HIGHLIGHT_STATUS_SUCCESS;
    info1.coverId = 2;
    info1.albumIdOld = 100;
    highlightRestore->albumInfos_.emplace_back(info1);
    HighlightRestore::HighlightAlbumInfo info2;
    info2.highlightStatus = HIGHLIGHT_STATUS_FAIL;
    info2.coverId = -1;
    highlightRestore->albumInfos_.emplace_back(info2);
    std::vector<NativeRdb::ValuesBucket> values;
    highlightRestore->UpdateMapInsertValues(values, highlightPhoto);
    highlightRestore->UpdateAlbums();
    EXPECT_EQ(highlightRestore->successCnt_, 1);
    EXPECT_EQ(highlightRestore->failCnt_, 1);
}

HWTEST_F(HighlightRestoreTest, highlight_restore_test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("highlight_restore_test_006 start");
    shared_ptr<HighlightRestore> highlightRestore = make_shared<HighlightRestore>();
    HighlightRestore::HighlightAlbumInfo info1;
    info1.clusterType = "OTHER_TYPE";
    info1.albumName = "AttractionsAlbum";
    info1.clusterSubType = "AttractionsAlbum";
    info1.clusterCondition = "not_null";
    info1.maxDateAdded = 1;
    info1.clusterCondition = R"({"startDate": "2024-01-01", "endDate": "2025-01-01"})";
    nlohmann::json jsonObject1 = nlohmann::json::parse(info1.clusterCondition, nullptr, false);
    highlightRestore->TransferClusterInfo(info1);
    EXPECT_TRUE(jsonObject1.contains("startDate"));
    EXPECT_TRUE(jsonObject1.contains("endDate"));

    HighlightRestore::HighlightAlbumInfo info2;
    info2.clusterType = "OTHER_TYPE";
    info2.albumName = "AttractionsAlbum";
    info2.clusterSubType = "AttractionsAlbum";
    info2.clusterCondition = "not_null";
    info2.maxDateAdded = 1;
    info2.clusterCondition = R"({"otherKey": "value"})";
    nlohmann::json jsonObject2 = nlohmann::json::parse(info2.clusterCondition, nullptr, false);
    highlightRestore->TransferClusterInfo(info2);
    EXPECT_FALSE(jsonObject2.contains("startDate"));
    EXPECT_FALSE(jsonObject2.contains("endDate"));
}

HWTEST_F(HighlightRestoreTest, highlight_restore_same_cover_album_udpate_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("highlight_restore_same_coverId_album_udpate_test_001 start");
    HighlightRestore highlightRestore;
    HighlightRestore::HighlightAlbumInfo albumInfoWithSameCover1;
    albumInfoWithSameCover1.coverId = TEST_FILE_ID_OLD;
    highlightRestore.albumInfos_.emplace_back(albumInfoWithSameCover1);
    HighlightRestore::HighlightAlbumInfo albumInfoWithSameCover2;
    albumInfoWithSameCover2.coverId = TEST_FILE_ID_OLD;
    highlightRestore.albumInfos_.emplace_back(albumInfoWithSameCover2);
    HighlightRestore::HighlightAlbumInfo albumInfoWithDifferentCover;
    albumInfoWithDifferentCover.coverId = TEST_FILE_ID_NEW;
    highlightRestore.albumInfos_.emplace_back(albumInfoWithDifferentCover);

    HighlightRestore::HighlightPhotoInfo highlightPhoto;
    highlightPhoto.fileIdOld = TEST_FILE_ID_OLD;
    highlightPhoto.photoInfo.fileIdNew = TEST_FILE_ID_NEW;
    highlightRestore.UpdateAlbumInfoCoverUris(highlightPhoto);

    for (const auto &albumInfo : highlightRestore.albumInfos_) {
        bool expectedResult = albumInfo.coverId == TEST_FILE_ID_OLD;
        EXPECT_EQ(!albumInfo.coverUri.empty(), expectedResult); // albumInfo with TEST_FILE_ID_OLD get coverUri updated
    }
}
}
}
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
#define MLOG_TAG "MediaLibraryRdbUtilsTest"
#include "medialibrary_rdb_utils_test.h"
#include "medialibrary_rdb_utils_album_cover_test.h"

#include "medialibrary_rdb_utils.h"
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>

#include "datashare_values_bucket.h"
#include "media_app_uri_permission_column.h"
#include "media_audio_column.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_refresh_album_column.h"
#include "media_unique_number_column.h"
#include "media_upgrade.h"
#include "medialibrary_album_fusion_utils.h"
#include "medialibrary_async_worker.h"
#include "medialibrary_business_record_column.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_db_const.h"
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_errno.h"
#include "medialibrary_formmap_operations.h"
#include "medialibrary_notify.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_unittest_utils.h"
#include "photo_album_column.h"
#include "result_set_utils.h"
#include "story_cover_info_column.h"
#include "story_play_info_column.h"
#include "power_efficiency_manager.h"
#include "rdb_sql_utils.h"
#include "medialibrary_restore.h"
#include "medialibrary_unistore_manager.h"
#include "test_data_builder.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {

static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;

namespace {
    using namespace std;
    using namespace NativeRdb;

    void PrepareUniqueNumberTableForTest()
    {
        if (g_rdbStore == nullptr) {
            MEDIA_ERR_LOG("PrepareUniqueNumberTableForTest: can not get g_rdbStore");
            return;
        }
        std::string queryRowSql = "SELECT COUNT(*) as count FROM " + ASSET_UNIQUE_NUMBER_TABLE;
        auto resultSet = g_rdbStore->QuerySql(queryRowSql);
        if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("PrepareUniqueNumberTableForTest: can not get AssetUniqueNumberTable count");
            return;
        }
        if (GetInt32Val("count", resultSet) != 0) {
            MEDIA_DEBUG_LOG("PrepareUniqueNumberTableForTest: AssetUniqueNumberTable already inited");
            return;
        }
        NativeRdb::ValuesBucket imageBucket;
        imageBucket.PutString(ASSET_MEDIA_TYPE, CONST_IMAGE_ASSET_TYPE);
        imageBucket.PutInt(UNIQUE_NUMBER, 1);
        NativeRdb::ValuesBucket videoBucket;
        videoBucket.PutString(ASSET_MEDIA_TYPE, CONST_VIDEO_ASSET_TYPE);
        videoBucket.PutInt(UNIQUE_NUMBER, 1);
        NativeRdb::ValuesBucket audioBucket;
        audioBucket.PutString(ASSET_MEDIA_TYPE, CONST_AUDIO_ASSET_TYPE);
        audioBucket.PutInt(UNIQUE_NUMBER, 1);
        std::vector<NativeRdb::ValuesBucket> buckets = { imageBucket, videoBucket, audioBucket };
        for (auto &bucket : buckets) {
            int64_t outRowId = -1;
            int32_t insertResult = g_rdbStore->Insert(outRowId, ASSET_UNIQUE_NUMBER_TABLE, bucket);
            if (insertResult != NativeRdb::E_OK || outRowId <= 0) {
                MEDIA_ERR_LOG("PrepareUniqueNumberTableForTest: insert failed, ret=%{public}d", insertResult);
            }
        }
        MEDIA_INFO_LOG("PrepareUniqueNumberTableForTest: init AssetUniqueNumberTable done");
    }

    void SetTestTables()
    {
        if (g_rdbStore == nullptr) {
            MEDIA_ERR_LOG("SetTestTables: g_rdbStore is nullptr");
            return;
        }
        std::vector<std::string> createTableSqlList = {
            PhotoUpgrade::CREATE_PHOTO_TABLE,
            AudioColumn::CREATE_AUDIO_TABLE,
            CREATE_MEDIA_TABLE,
            CREATE_ASSET_UNIQUE_NUMBER_TABLE,
            PhotoExtUpgrade::CREATE_PHOTO_EXT_TABLE,
            PhotoAlbumColumns::CREATE_TABLE,
        };
        for (auto &createTableSql : createTableSqlList) {
            int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
            if (ret != NativeRdb::E_OK) {
                MEDIA_ERR_LOG("SetTestTables execute sql failed, ret=%{public}d", ret);
            }
        }
        PrepareUniqueNumberTableForTest();
    }

    const std::vector<std::string> ALL_SYS_PHOTO_ALBUM = {
        std::to_string(PhotoAlbumSubType::FAVORITE),
        std::to_string(PhotoAlbumSubType::VIDEO),
        std::to_string(PhotoAlbumSubType::HIDDEN),
        std::to_string(PhotoAlbumSubType::TRASH),
        std::to_string(PhotoAlbumSubType::SCREENSHOT),
        std::to_string(PhotoAlbumSubType::CAMERA),
        std::to_string(PhotoAlbumSubType::IMAGE),
        std::to_string(PhotoAlbumSubType::CLOUD_ENHANCEMENT),
        std::to_string(PhotoAlbumSubType::SOURCE_GENERIC),
    };

    const std::vector<std::string> ALL_ANALYSIS_ALBUM = {
        std::to_string(PhotoAlbumSubType::CLASSIFY),
        std::to_string(PhotoAlbumSubType::GEOGRAPHY_LOCATION),
        std::to_string(PhotoAlbumSubType::GEOGRAPHY_CITY),
        std::to_string(PhotoAlbumSubType::SHOOTING_MODE),
        std::to_string(PhotoAlbumSubType::PORTRAIT),
    };
} // namespace

void MediaLibraryRdbUtilsAlbumCoverTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::SetUpTestCase");
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    SetTestTables();
}

static void ClearPhotos()
{
    auto rdbStore = MediaLibraryDataManager::GetInstance()->rdbStore_;
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    int32_t deletedRows = -1;
    auto ret = rdbStore->Delete(deletedRows, predicates);
    MEDIA_INFO_LOG("ClearPhotos Delete retVal: %{public}d, deletedRows: %{public}d", ret, deletedRows);
}

void MediaLibraryRdbUtilsAlbumCoverTest::TearDownTestCase(void)
{
    ClearPhotos();
    g_rdbStore = nullptr;
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::TearDownTestCase");
}

void MediaLibraryRdbUtilsAlbumCoverTest::SetUp()
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::SetUp");
}

void MediaLibraryRdbUtilsAlbumCoverTest::TearDown(void)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::TearDown");
}

HWTEST_F(MediaLibraryRdbUtilsAlbumCoverTest, GetAlbumCountAndCoverPredicates_SourceAlbum, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("GetAlbumCountAndCoverPredicates_SourceAlbum::Start");
    UpdateAlbumData albumInfo;
    albumInfo.albumId = 1;
    albumInfo.albumSubtype = static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC);
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    MediaLibraryRdbUtils::GetAlbumCountAndCoverPredicates(albumInfo, predicates, false);
    EXPECT_GT(predicates.GetWhereArgs().size(), 0);
    MEDIA_INFO_LOG("GetAlbumCountAndCoverPredicates_SourceAlbum End");
}

HWTEST_F(MediaLibraryRdbUtilsAlbumCoverTest, GetAlbumCountAndCoverPredicates_UserAlbum, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("GetAlbumCountAndCoverPredicates_UserAlbum::Start");
    UpdateAlbumData albumInfo;
    albumInfo.albumId = 1;
    albumInfo.albumSubtype = static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC);
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    MediaLibraryRdbUtils::GetAlbumCountAndCoverPredicates(albumInfo, predicates, false);
    EXPECT_GT(predicates.GetWhereArgs().size(), 0);
    MEDIA_INFO_LOG("GetAlbumCountAndCoverPredicates_UserAlbum End");
}

HWTEST_F(MediaLibraryRdbUtilsAlbumCoverTest, SetUpdateCoverValues_UserGeneric, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("SetUpdateCoverValues_IsNeedSetCover_UserGeneric::Start");
    UpdateAlbumData data;
    data.albumId = 1;
    data.albumSubtype = static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC);
    data.albumCoverUri = "file://media/Photo/1";
    data.coverUriSource = static_cast<int32_t>(CoverUriSource::MANUAL_LOCAL_COVER);
    NativeRdb::ValuesBucket values {};
    int32_t ret = MediaLibraryRdbUtils::SetUpdateCoverValues(data, values, false);
    EXPECT_EQ(ret, E_SUCCESS);
    MEDIA_INFO_LOG("SetUpdateCoverValues_IsNeedSetCover_UserGeneric End");
}

HWTEST_F(MediaLibraryRdbUtilsAlbumCoverTest, SetUpdateCoverValues_Favorite, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("SetUpdateCoverValues_IsNeedSetCover_Favorite::Start");
    UpdateAlbumData data;
    data.albumId = 1;
    data.albumSubtype = static_cast<int32_t>(PhotoAlbumSubType::FAVORITE);
    data.albumCoverUri = "file://media/Photo/1";
    data.coverUriSource = static_cast<int32_t>(CoverUriSource::MANUAL_LOCAL_COVER);
    NativeRdb::ValuesBucket values {};
    int32_t ret = MediaLibraryRdbUtils::SetUpdateCoverValues(data, values, false);
    EXPECT_EQ(ret, E_SUCCESS);
    MEDIA_INFO_LOG("SetUpdateCoverValues_IsNeedSetCover_Favorite End");
}

static void EnsureTrashAssetPredicates(int32_t assetId)
{
    if (g_rdbStore == nullptr) {
        return;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, 0);
    values.PutInt(PhotoColumn::PHOTO_SYNC_STATUS, 0);
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, std::to_string(assetId));
    int32_t rows = -1;
    g_rdbStore->Update(rows, values, predicates);
}

HWTEST_F(MediaLibraryRdbUtilsAlbumCoverTest, UpdateTrashedAssetOnAlbum_LakeSupport, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateTrashedAssetOnAlbum_LakeSupport::Start");
    ASSERT_NE(g_rdbStore, nullptr);
    auto &builder = TestDataBuilder::GetInstance();
    builder.Init(g_rdbStore);
    builder.ClearAllTables();
    int32_t albumId = builder.CreateAlbum(TestAlbumType::USER_ALBUM, "LakeSupportAlbum");
    ASSERT_GT(albumId, 0);
    int32_t assetId = builder.CreateAsset(albumId, "lake_support_asset");
    ASSERT_GT(assetId, 0);
    EnsureTrashAssetPredicates(assetId);

    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, std::to_string(albumId));
    int32_t ret = MediaLibraryRdbUtils::UpdateTrashedAssetOnAlbum(g_rdbStore, predicates);
    EXPECT_EQ(ret, 1);
    builder.ClearAllTables();
    MEDIA_INFO_LOG("UpdateTrashedAssetOnAlbum_LakeSupport End");
}
} // namespace Media
} // namespace OHOS
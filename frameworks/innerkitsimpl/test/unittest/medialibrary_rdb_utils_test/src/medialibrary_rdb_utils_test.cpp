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
#define MLOG_TAG "MediaLibraryRdbUtilsTest"
#include "medialibrary_rdb_utils_test.h"

#include "medialibrary_rdb_utils.h"
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>

#include "datashare_values_bucket.h"
#include "media_app_uri_permission_column.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_refresh_album_column.h"
#include "medialibrary_album_fusion_utils.h"
#include "medialibrary_async_worker.h"
#include "medialibrary_business_record_column.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_db_const.h"
#include "medialibrary_formmap_operations.h"
#include "medialibrary_notify.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_unittest_utils.h"
#include "story_cover_info_column.h"
#include "story_play_info_column.h"
#include "power_efficiency_manager.h"
#include "rdb_sql_utils.h"
#include "medialibrary_restore.h"

namespace OHOS {
namespace Media {
namespace {
    using namespace std;
    using namespace NativeRdb;

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

void MediaLibraryRdbUtilsTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::SetUpTestCase");
}

void ClearPhotos()
{
    auto rdbStore = MediaLibraryDataManager::GetInstance()->rdbStore_;
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    int32_t deletedRows = -1;
    auto ret = rdbStore->Delete(deletedRows, predicates);
    MEDIA_INFO_LOG("ClearPhotos Delete retVal: %{public}d, deletedRows: %{public}d", ret, deletedRows);
}

void MediaLibraryRdbUtilsTest::TearDownTestCase(void)
{
    ClearPhotos();
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::TearDownTestCase");
}

// SetUp:Execute before each test case
void MediaLibraryRdbUtilsTest::SetUp()
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::SetUp");
}

void MediaLibraryRdbUtilsTest::TearDown(void)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::TearDown");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_FillOneAlbumCountCoverUri_test_001, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_FillOneAlbumCountCoverUri_test_001:start");
    int32_t albumId = 0;
    std::string sql;
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    auto rdbStore = std::make_shared<MediaLibraryRdbStore>(context);
    int32_t ret = MediaLibraryRdbUtils::FillOneAlbumCountAndCoverUri(rdbStore, albumId,
        PhotoAlbumSubType::FAVORITE, sql);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_FillOneAlbumCountCoverUri_test_001:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_FillOneAlbumCountCoverUri_test_002, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_FillOneAlbumCountCoverUri_test_002:start");
    int32_t albumId = 1;
    std::string sql;
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    auto rdbStore = std::make_shared<MediaLibraryRdbStore>(context);
    int32_t ret = MediaLibraryRdbUtils::FillOneAlbumCountAndCoverUri(rdbStore, albumId,
        PhotoAlbumSubType::HIDDEN, sql);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_FillOneAlbumCountCoverUri_test_002:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_FillOneAlbumCountCoverUri_test_003, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_FillOneAlbumCountCoverUri_test_003:start");
    int32_t albumId = 1;
    std::string sql;
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    auto rdbStore = std::make_shared<MediaLibraryRdbStore>(context);
    int32_t ret = MediaLibraryRdbUtils::FillOneAlbumCountAndCoverUri(rdbStore, albumId,
        PhotoAlbumSubType::VIDEO, sql);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_FillOneAlbumCountCoverUri_test_003:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_FillOneAlbumCountCoverUri_test_004, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_FillOneAlbumCountCoverUri_test_004:start");
    int32_t albumId = 1;
    std::string sql;
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    auto rdbStore = std::make_shared<MediaLibraryRdbStore>(context);
    int32_t ret = MediaLibraryRdbUtils::FillOneAlbumCountAndCoverUri(rdbStore, albumId,
        PhotoAlbumSubType::CLOUD_ENHANCEMENT, sql);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_FillOneAlbumCountCoverUri_test_004:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_FillOneAlbumCountCoverUri_test_005, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_FillOneAlbumCountCoverUri_test_005:start");
    int32_t albumId = 1;
    std::string sql;
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    auto rdbStore = std::make_shared<MediaLibraryRdbStore>(context);
    int32_t ret = MediaLibraryRdbUtils::FillOneAlbumCountAndCoverUri(rdbStore, albumId,
        PhotoAlbumSubType::USER_GENERIC, sql);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_FillOneAlbumCountCoverUri_test_005:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_FillOneAlbumCountCoverUri_test_006, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_FillOneAlbumCountCoverUri_test_006:start");
    int32_t albumId = 1;
    std::string sql;
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    auto rdbStore = std::make_shared<MediaLibraryRdbStore>(context);
    int32_t ret = MediaLibraryRdbUtils::FillOneAlbumCountAndCoverUri(rdbStore, albumId,
        PhotoAlbumSubType::IMAGE, sql);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_FillOneAlbumCountCoverUri_test_006:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_FillOneAlbumCountCoverUri_test_007, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_FillOneAlbumCountCoverUri_test_007:start");
    int32_t albumId = 1;
    std::string sql;
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    auto rdbStore = std::make_shared<MediaLibraryRdbStore>(context);
    int32_t ret = MediaLibraryRdbUtils::FillOneAlbumCountAndCoverUri(rdbStore, albumId,
        PhotoAlbumSubType::SOURCE_GENERIC, sql);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_FillOneAlbumCountCoverUri_test_007:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_FillOneAlbumCountCoverUri_test_008, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_FillOneAlbumCountCoverUri_test_008:start");
    int32_t albumId = 1;
    std::string sql;
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    auto rdbStore = std::make_shared<MediaLibraryRdbStore>(context);
    int32_t ret = MediaLibraryRdbUtils::FillOneAlbumCountAndCoverUri(rdbStore, albumId,
        PhotoAlbumSubType::TRASH, sql);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_FillOneAlbumCountCoverUri_test_008:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_FillOneAlbumCountCoverUri_test_009, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_FillOneAlbumCountCoverUri_test_009:start");
    int32_t albumId = -1;
    std::string sql;
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    auto rdbStore = std::make_shared<MediaLibraryRdbStore>(context);
    int32_t ret = MediaLibraryRdbUtils::FillOneAlbumCountAndCoverUri(rdbStore, albumId,
        PhotoAlbumSubType::TRASH, sql);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_FillOneAlbumCountCoverUri_test_009:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_IsNeedRefreshByCheckTable_test_001, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_IsNeedRefreshByCheckTable_test_001:start");
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    auto rdbStore = std::make_shared<MediaLibraryRdbStore>(context);
    bool flag = true;
    int32_t ret = MediaLibraryRdbUtils::IsNeedRefreshByCheckTable(rdbStore, flag);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_IsNeedRefreshByCheckTable_test_001:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_IsNeedRefreshByCheckTable_test_002, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_IsNeedRefreshByCheckTable_test_002:start");
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    auto rdbStore = std::make_shared<MediaLibraryRdbStore>(context);
    bool flag = true;
    int32_t ret = MediaLibraryRdbUtils::IsNeedRefreshByCheckTable(nullptr, flag);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_IsNeedRefreshByCheckTable_test_002:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_IsNeedRefreshAlbum_test_001, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_IsNeedRefreshAlbum_test_001:start");
    bool ret = MediaLibraryRdbUtils::IsNeedRefreshAlbum();
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_IsNeedRefreshAlbum_test_001:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_IsInRefreshTask_test_001, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_IsInRefreshTask_test_001:start");
    bool ret = MediaLibraryRdbUtils::IsInRefreshTask();
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_IsInRefreshTask_test_001:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_UpdateUserAlbumInternal_test_001, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_UpdateUserAlbumInternal_test_001:start");
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    auto rdbStore = std::make_shared<MediaLibraryRdbStore>(context);
    vector<string> columns = {
        PhotoAlbumColumns::ALBUM_ID,
        PhotoAlbumColumns::ALBUM_COVER_URI,
        PhotoAlbumColumns::ALBUM_COUNT,
        PhotoAlbumColumns::ALBUM_IMAGE_COUNT,
        PhotoAlbumColumns::ALBUM_VIDEO_COUNT,
    };
    bool shouldNotify = true;
    MediaLibraryRdbUtils::UpdateUserAlbumInternal(rdbStore, columns, shouldNotify);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_UpdateUserAlbumInternal_test_001:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_UpdateUserAlbumInternal_test_002, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_UpdateUserAlbumInternal_test_002:start");
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    auto rdbStore = std::make_shared<MediaLibraryRdbStore>(context);
    vector<string> columns = {""};
    bool shouldNotify = true;
    MediaLibraryRdbUtils::UpdateUserAlbumInternal(rdbStore, columns, shouldNotify);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_UpdateUserAlbumInternal_test_002:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_UpdateTrashedAssetOnAlbum_test_001, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_UpdateTrashedAssetOnAlbum_test_001:start");
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    auto rdbStore = std::make_shared<MediaLibraryRdbStore>(context);
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    int32_t ret = MediaLibraryRdbUtils::UpdateTrashedAssetOnAlbum(rdbStore, predicates);
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_UpdateTrashedAssetOnAlbum_test_001:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_UpdateRemovedAssetToTrash_test_001, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_UpdateRemovedAssetToTrash_test_001:start");
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    auto rdbStore = std::make_shared<MediaLibraryRdbStore>(context);
    vector<string> whereIdArgs = {""};
    int32_t ret = MediaLibraryRdbUtils::UpdateRemovedAssetToTrash(rdbStore, whereIdArgs);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_UpdateRemovedAssetToTrash_test_001:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_UpdateHighlightPlayInfo_test_001, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_UpdateHighlightPlayInfo_test_001:start");
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    auto rdbStore = std::make_shared<MediaLibraryRdbStore>(context);
    string albumId = "";
    int32_t ret = MediaLibraryRdbUtils::UpdateHighlightPlayInfo(rdbStore, albumId);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_UpdateHighlightPlayInfo_test_001:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_UpdateHighlightPlayInfo_test_002, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_UpdateHighlightPlayInfo_test_002:start");
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    auto rdbStore = std::make_shared<MediaLibraryRdbStore>(context);
    string albumId = "testAlbumId";
    int32_t ret = MediaLibraryRdbUtils::UpdateHighlightPlayInfo(rdbStore, albumId);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_UpdateHighlightPlayInfo_test_002:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_UpdateOwnerAlbumId_test_001, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_UpdateOwnerAlbumId_test_001:start");
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    auto rdbStore = std::make_shared<MediaLibraryRdbStore>(context);
    DataShare::DataShareValuesBucket value1;
    value1.Put(MediaColumn::MEDIA_NAME, "newDisplayName");
    std::string newTitle = MediaFileUtils::GetTitleFromDisplayName("newDisplayName");
    value1.Put(MediaColumn::MEDIA_TITLE, newTitle);
    vector<DataShare::DataShareValuesBucket> values = {value1};
    vector<int32_t> updateIds = {0};
    bool hidden = false;
    int32_t ret = MediaLibraryRdbUtils::UpdateOwnerAlbumId(rdbStore, values, updateIds, hidden);
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_UpdateOwnerAlbumId_test_001:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_UpdateOwnerAlbumId_test_002, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_UpdateOwnerAlbumId_test_002:start");
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    auto rdbStore = std::make_shared<MediaLibraryRdbStore>(context);
    DataShare::DataShareValuesBucket value1;
    value1.Put(MediaColumn::MEDIA_NAME, "newDisplayName");
    std::string newTitle = MediaFileUtils::GetTitleFromDisplayName("newDisplayName");
    value1.Put(MediaColumn::MEDIA_TITLE, newTitle);
    DataShare::DataShareValuesBucket value2;
    value2.Put(MediaColumn::MEDIA_NAME, "newDisplayName");
    std::string newTitle2 = MediaFileUtils::GetTitleFromDisplayName("newDisplayName2");
    value2.Put(MediaColumn::MEDIA_TITLE, newTitle2);
    vector<DataShare::DataShareValuesBucket> values = {value1, value2};
    vector<int32_t> updateIds = {0};
    bool hidden = false;
    int32_t ret = MediaLibraryRdbUtils::UpdateOwnerAlbumId(rdbStore, values, updateIds, hidden);
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_UpdateOwnerAlbumId_test_002:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_UpdateAnalysisAlbumByUri_test_001, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_UpdateAnalysisAlbumByUri_test_001:start");
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    auto rdbStore = std::make_shared<MediaLibraryRdbStore>(context);
    vector<string> uris = {""};
    MediaLibraryRdbUtils::UpdateAnalysisAlbumByUri(rdbStore, uris);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_UpdateAnalysisAlbumByUri_test_001:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_UpdateAnalysisAlbumByUri_test_002, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_UpdateAnalysisAlbumByUri_test_002:start");
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    auto rdbStore = std::make_shared<MediaLibraryRdbStore>(context);
    vector<string> uris = {"testUri", "testUri2"};
    MediaLibraryRdbUtils::UpdateAnalysisAlbumByUri(rdbStore, uris);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_UpdateAnalysisAlbumByUri_test_002:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_GetAlbumIdsForPortrait_test_001, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_GetAlbumIdsForPortrait_test_001:start");
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    auto rdbStore = std::make_shared<MediaLibraryRdbStore>(context);
    vector<string> portraitAlbumIds = {""};
    int32_t ret = MediaLibraryRdbUtils::GetAlbumIdsForPortrait(rdbStore, portraitAlbumIds);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_GetAlbumIdsForPortrait_test_001:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_GetAlbumIdsForPortrait_test_002, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_GetAlbumIdsForPortrait_test_002:start");
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    auto rdbStore = std::make_shared<MediaLibraryRdbStore>(context);
    vector<string> portraitAlbumIds = {"testId1", "testId2"};
    int32_t ret = MediaLibraryRdbUtils::GetAlbumIdsForPortrait(rdbStore, portraitAlbumIds);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_GetAlbumIdsForPortrait_test_002:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_GetAlbumSubtypeArgument_test_001, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_GetAlbumSubtypeArgument_test_001:start");
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    int32_t ret = MediaLibraryRdbUtils::GetAlbumSubtypeArgument(predicates);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_GetAlbumSubtypeArgument_test_001:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_QueryAllShootingModeAlbumIds_test_001,
    testing::ext::TestSize.Level1)
{
    vector<int32_t> albumIds;
    bool ret = MediaLibraryRdbUtils::QueryAllShootingModeAlbumIds(albumIds);
    EXPECT_FALSE(ret);
    EXPECT_EQ(albumIds.size(), 0);

    albumIds.clear();
    ret = MediaLibraryRdbUtils::QueryAllShootingModeAlbumIds(albumIds);
    EXPECT_FALSE(ret);
    EXPECT_EQ(albumIds.size(), 0);
}

int32_t CreateSingleImage(string displayname, string appId)
{
    Uri createAssetUri("file://media/Photo/create");
    string relativePath = "Pictures/";
    MediaType mediaType = MEDIA_TYPE_IMAGE;
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MEDIA_DATA_DB_MEDIA_TYPE, mediaType);
    valuesBucket.Put(MEDIA_DATA_DB_NAME, displayname);
    valuesBucket.Put(MEDIA_DATA_DB_RELATIVE_PATH, relativePath);
    valuesBucket.Put(MEDIA_DATA_DB_OWNER_APPID, appId);
    MediaLibraryCommand cmd(createAssetUri);
    return MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_TransformOwnerAppIdToTokenId_test_001,
    testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_TransformOwnerAppIdToTokenId_test_001:start");
    MediaLibraryUnitTestUtils::Init();
    int32_t id1 = CreateSingleImage("TransformOwnerAppIdTest1.jpg", "");
    RdbPredicates predicates(AppUriPermissionColumn::APP_URI_PERMISSION_TABLE);
    predicates.EqualTo(AppUriPermissionColumn::FILE_ID, id1);
    vector<string> columns;
    auto resultSet = MediaLibraryDataManager::GetInstance()->rdbStore_->Query(predicates, columns);
    int32_t rowCount = 0;
    resultSet->GetRowCount(rowCount);
    resultSet->Close();
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::rowCount:%{public}d", rowCount);
    MediaLibraryRdbUtils::TransformOwnerAppIdToTokenId(MediaLibraryDataManager::GetInstance()->rdbStore_);
    auto resultSetAfter = MediaLibraryDataManager::GetInstance()->rdbStore_->Query(predicates, columns);
    int32_t rowCountAfter = 0;
    resultSetAfter->GetRowCount(rowCountAfter);
    resultSetAfter->Close();
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::rowCount:%{public}d", rowCountAfter);
    EXPECT_EQ((rowCountAfter - rowCount), 0);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_TransformOwnerAppIdToTokenId_test_001:stop");
}

HWTEST_F(MediaLibraryRdbUtilsTest, medialib_rdbutils_CleanAmbiguousColumn_test_001,
    testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_CleanAmbiguousColumn_test_001:start");
    MediaLibraryUnitTestUtils::Init();
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(AppUriPermissionColumn::FILE_ID, "1");
    vector<string> columns;
    columns.push_back(AppUriPermissionColumn::FILE_ID);
    MediaLibraryRdbUtils::CleanAmbiguousColumn(columns, predicates, PhotoColumn::PHOTOS_TABLE);
    auto operationItems = predicates.GetOperationList();
    int32_t ret = columns[0].compare(AppUriPermissionColumn::FILE_ID);
    EXPECT_NE(ret, 0);
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::medialib_rdbutils_CleanAmbiguousColumn_test_001:stop");
}
} // namespace Media
} // namespace OHOS
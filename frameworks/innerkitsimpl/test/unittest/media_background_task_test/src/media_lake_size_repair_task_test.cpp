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

#define MLOG_TAG "LakeSizeRepairTaskTest"

#include "media_lake_size_repair_task_test.h"

#include "media_lake_size_repair_dao.h"

#include "media_column.h"
#include "media_log.h"
#include "media_upgrade.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "photo_album_column.h"
#include "result_set_utils.h"

using namespace testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS::Media::Background {
namespace {
constexpr int32_t SLEEP_SECONDS = 1;
constexpr int32_t TEST_MEDIA_TYPE_IMAGE = 1;
constexpr int32_t TEST_VALUE = 100;
constexpr int32_t TEST_LIMIT = 10;
constexpr int64_t TEST_DATE = 1700000000;

std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;
const std::vector<std::string> CREATE_TABLE_SQL_LIST = {
    PhotoUpgrade::CREATE_PHOTO_TABLE,
    PhotoAlbumColumns::CREATE_TABLE,
};
const std::vector<std::string> TEST_TABLES = {
    PhotoColumn::PHOTOS_TABLE,
    PhotoAlbumColumns::TABLE,
};

struct PhotoInsertOptions {
    int32_t cleanFlag = static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN);
    int32_t timePending = 0;
    int32_t fileSourceType = static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE);
    int32_t position = static_cast<int32_t>(PhotoPositionType::LOCAL);
    int32_t isTemp = 0;
    int32_t syncStatus = static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE);
    int64_t size = -1;
};

int32_t InsertAlbum(const std::string &albumName, const std::string &lpath, int32_t albumSubtype,
    int32_t dirty = static_cast<int32_t>(DirtyType::TYPE_SYNCED),
    int32_t albumType = static_cast<int32_t>(PhotoAlbumType::SOURCE))
{
    ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, albumType);
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, albumSubtype);
    values.PutInt(PhotoAlbumColumns::ALBUM_DIRTY, dirty);
    values.PutInt(PhotoAlbumColumns::ALBUM_HIDDEN, 0);
    values.PutLong(PhotoAlbumColumns::ALBUM_DATE_ADDED, TEST_DATE);
    values.PutLong(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, TEST_DATE);
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, albumName);
    values.PutString(PhotoAlbumColumns::ALBUM_LPATH, lpath);
    int64_t albumId = -1;
    EXPECT_EQ(g_rdbStore->Insert(albumId, PhotoAlbumColumns::TABLE, values), E_OK);
    EXPECT_GT(albumId, 0);
    return static_cast<int32_t>(albumId);
}

int32_t InsertPhoto(const std::string &displayName, int32_t ownerAlbumId, const std::string &storagePath,
    const PhotoInsertOptions &options = {})
{
    ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_FILE_PATH, "/storage/media/local/files/Photo/" + displayName + ".jpg");
    values.PutLong(MediaColumn::MEDIA_SIZE, options.size);
    values.PutString(MediaColumn::MEDIA_TITLE, displayName);
    values.PutString(MediaColumn::MEDIA_NAME, displayName + ".jpg");
    values.PutInt(MediaColumn::MEDIA_TYPE, TEST_MEDIA_TYPE_IMAGE);
    values.PutString(MediaColumn::MEDIA_OWNER_PACKAGE, "com.test.camera");
    values.PutString(MediaColumn::MEDIA_PACKAGE_NAME, "camera");
    values.PutLong(MediaColumn::MEDIA_DATE_ADDED, TEST_DATE);
    values.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, TEST_DATE);
    values.PutLong(MediaColumn::MEDIA_DATE_TAKEN, TEST_DATE);
    values.PutInt(MediaColumn::MEDIA_DURATION, 0);
    values.PutInt(MediaColumn::MEDIA_IS_FAV, 0);
    values.PutLong(MediaColumn::MEDIA_DATE_TRASHED, 0);
    values.PutInt(MediaColumn::MEDIA_HIDDEN, 0);
    values.PutInt(MediaColumn::MEDIA_TIME_PENDING, options.timePending);
    values.PutInt(PhotoColumn::PHOTO_HEIGHT, TEST_VALUE);
    values.PutInt(PhotoColumn::PHOTO_WIDTH, TEST_VALUE);
    values.PutLong(PhotoColumn::PHOTO_EDIT_TIME, 0);
    values.PutString(PhotoColumn::PHOTO_SHOOTING_MODE, "1");
    values.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, ownerAlbumId);
    values.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, options.fileSourceType);
    values.PutString(PhotoColumn::PHOTO_SOURCE_PATH, storagePath);
    values.PutString(PhotoColumn::PHOTO_STORAGE_PATH, storagePath);
    values.PutInt(PhotoColumn::PHOTO_POSITION, options.position);
    values.PutInt(PhotoColumn::PHOTO_SYNC_STATUS, options.syncStatus);
    values.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, options.cleanFlag);
    values.PutInt(PhotoColumn::PHOTO_IS_TEMP, options.isTemp);
    values.PutInt(PhotoColumn::PHOTO_FILE_HIDDEN, 0);
    int64_t fileId = -1;
    EXPECT_EQ(g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, values), E_OK);
    EXPECT_GT(fileId, 0);
    MEDIA_INFO_LOG("displayName: %{public}s, fileId: %{public}" PRId64, displayName.c_str(), fileId);
    return static_cast<int32_t>(fileId);
}
}  // namespace

void MediaLakeSizeRepairTaskTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, TEST_TABLES, true);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, CREATE_TABLE_SQL_LIST);
}

void MediaLakeSizeRepairTaskTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, TEST_TABLES, false);
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void MediaLakeSizeRepairTaskTest::SetUp()
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, TEST_TABLES, false);
}

void MediaLakeSizeRepairTaskTest::TearDown() {}

HWTEST_F(MediaLakeSizeRepairTaskTest, QueryCorruptedLakePhotos_ReturnsOnlyHoLakeSizeBroken_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryCorruptedLakePhotos_ReturnsOnlyHoLakeSizeBroken_001 start");
    int32_t albumId = InsertAlbum("LakeRepair", "/LakeRepair",
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC));

    int32_t matchNegId = InsertPhoto("lake_neg", albumId,
        "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/lake_neg.jpg");
    PhotoInsertOptions zeroOpt;
    zeroOpt.size = 0;
    int32_t matchZeroId = InsertPhoto("lake_zero", albumId,
        "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/lake_zero.jpg", zeroOpt);
    PhotoInsertOptions localCloudOpt;
    localCloudOpt.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    int32_t matchLocalCloudId = InsertPhoto("lake_local_cloud", albumId,
        "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/lake_local_cloud.jpg", localCloudOpt);
    PhotoInsertOptions okOpt;
    okOpt.size = 1024;
    InsertPhoto("lake_ok", albumId,
        "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/lake_ok.jpg", okOpt);
    PhotoInsertOptions mediaOpt;
    mediaOpt.fileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);
    mediaOpt.size = 0;
    InsertPhoto("media_broken", albumId,
        "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/media_broken.jpg", mediaOpt);
    PhotoInsertOptions cloudOpt;
    cloudOpt.position = static_cast<int32_t>(PhotoPositionType::CLOUD);
    InsertPhoto("lake_cloud_only", albumId,
        "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/lake_cloud_only.jpg", cloudOpt);
    PhotoInsertOptions cleanedOpt;
    cleanedOpt.cleanFlag = static_cast<int32_t>(CleanType::TYPE_OFFLINE_CLEAN);
    InsertPhoto("lake_cleaned", albumId,
        "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/lake_cleaned.jpg", cleanedOpt);
    PhotoInsertOptions pendingOpt;
    pendingOpt.timePending = -4;
    InsertPhoto("lake_pending", albumId,
        "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/lake_pending.jpg", pendingOpt);
    PhotoInsertOptions tempOpt;
    tempOpt.isTemp = 1;
    InsertPhoto("lake_temp", albumId,
        "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/lake_temp.jpg", tempOpt);
    PhotoInsertOptions syncingOpt;
    syncingOpt.syncStatus = static_cast<int32_t>(SyncStatusType::TYPE_DIRTY);
    InsertPhoto("lake_syncing", albumId,
        "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/lake_syncing.jpg", syncingOpt);

    MediaLakeSizeRepairDao dao;
    auto photos = dao.QueryCorruptedLakePhotos(0, TEST_LIMIT);

    ASSERT_EQ(photos.size(), static_cast<size_t>(3));
    EXPECT_EQ(photos[0].fileId, matchNegId);
    EXPECT_EQ(photos[1].fileId, matchZeroId);
    EXPECT_EQ(photos[2].fileId, matchLocalCloudId);
    EXPECT_EQ(photos[0].storagePath, "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/lake_neg.jpg");
}

HWTEST_F(MediaLakeSizeRepairTaskTest, QueryCorruptedLakePhotos_RespectsCursor_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryCorruptedLakePhotos_RespectsCursor_002 start");
    int32_t albumId = InsertAlbum("LakeRepairCursor", "/LakeRepairCursor",
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC));
    int32_t firstId = InsertPhoto("lake_c1", albumId,
        "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/lake_c1.jpg");
    int32_t secondId = InsertPhoto("lake_c2", albumId,
        "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/lake_c2.jpg");
    InsertPhoto("lake_c3", albumId, "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/lake_c3.jpg");

    MediaLakeSizeRepairDao dao;
    auto firstBatch = dao.QueryCorruptedLakePhotos(0, 1);
    ASSERT_EQ(firstBatch.size(), static_cast<size_t>(1));
    EXPECT_EQ(firstBatch[0].fileId, firstId);

    auto secondBatch = dao.QueryCorruptedLakePhotos(firstId, 1);
    ASSERT_EQ(secondBatch.size(), static_cast<size_t>(1));
    EXPECT_EQ(secondBatch[0].fileId, secondId);

    auto rest = dao.QueryCorruptedLakePhotos(secondId, TEST_LIMIT);
    ASSERT_EQ(rest.size(), static_cast<size_t>(1));
    EXPECT_GT(rest[0].fileId, secondId);
}

HWTEST_F(MediaLakeSizeRepairTaskTest, QueryCorruptedLakePhotos_ReturnsEmptyWhenNoMatch_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryCorruptedLakePhotos_ReturnsEmptyWhenNoMatch_003 start");
    int32_t albumId = InsertAlbum("LakeRepairEmpty", "/LakeRepairEmpty",
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC));
    PhotoInsertOptions fineOpt;
    fineOpt.size = 2048;
    InsertPhoto("lake_fine", albumId,
        "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/lake_fine.jpg", fineOpt);

    MediaLakeSizeRepairDao dao;
    auto photos = dao.QueryCorruptedLakePhotos(0, TEST_LIMIT);
    ASSERT_TRUE(photos.empty());
}
}  // namespace OHOS::Media::Background

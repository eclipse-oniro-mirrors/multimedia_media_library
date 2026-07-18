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

#define MLOG_TAG "FileManagerOfflineCleanupTaskTest"

#include "media_file_manager_offline_cleanup_task_test.h"

#include "media_file_manager_offline_cleanup_task.h"

#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_upgrade.h"
#include "medialibrary_subscriber.h"
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
    int32_t fileSourceType = static_cast<int32_t>(FileSourceType::FILE_MANAGER);
    int32_t position = static_cast<int32_t>(PhotoPositionType::LOCAL);
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
    MEDIA_INFO_LOG("albumName: %{public}s, lpath: %{public}s, albumId: %{public}" PRId64,
        albumName.c_str(), lpath.c_str(), albumId);
    return static_cast<int32_t>(albumId);
}

int32_t InsertPhoto(const std::string &displayName, int32_t ownerAlbumId, const std::string &sourcePath,
    const PhotoInsertOptions &options = {})
{
    ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_FILE_PATH, "/storage/media/local/files/Photo/" + displayName + ".jpg");
    values.PutLong(MediaColumn::MEDIA_SIZE, 1);
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
    values.PutString(PhotoColumn::PHOTO_SOURCE_PATH, sourcePath);
    values.PutString(PhotoColumn::PHOTO_STORAGE_PATH, "/storage/media/local/files/Docs/" + displayName + ".jpg");
    values.PutInt(PhotoColumn::PHOTO_POSITION, options.position);
    values.PutInt(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE));
    values.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, options.cleanFlag);
    values.PutInt(PhotoColumn::PHOTO_IS_TEMP, 0);
    values.PutInt(PhotoColumn::PHOTO_FILE_HIDDEN, 0);
    int64_t fileId = -1;
    EXPECT_EQ(g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, values), E_OK);
    EXPECT_GT(fileId, 0);
    MEDIA_INFO_LOG("displayName: %{public}s, fileId: %{public}" PRId64, displayName.c_str(), fileId);
    return static_cast<int32_t>(fileId);
}

bool QueryAlbumDirty(int32_t albumId, int32_t &dirty)
{
    dirty = 0;
    AbsRdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    auto resultSet = g_rdbStore->Query(predicates, {PhotoAlbumColumns::ALBUM_DIRTY});
    if (resultSet == nullptr) {
        return false;
    }
    if (resultSet->GoToFirstRow() != E_OK) {
        resultSet->Close();
        return false;
    }
    dirty = GetInt32Val(PhotoAlbumColumns::ALBUM_DIRTY, resultSet);
    resultSet->Close();
    MEDIA_INFO_LOG("albumId: %{public}d, dirty: %{public}d", albumId, dirty);
    return true;
}

bool QueryPhotoCountByFileId(int32_t fileId, int32_t &count)
{
    count = 0;
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    auto resultSet = g_rdbStore->Query(predicates, {MediaColumn::MEDIA_ID});
    if (resultSet == nullptr) {
        return false;
    }
    while (resultSet->GoToNextRow() == E_OK) {
        ++count;
    }
    resultSet->Close();
    MEDIA_INFO_LOG("fileId: %{public}d, count: %{public}d", fileId, count);
    return true;
}

bool QueryPhotoOwnerAlbumId(int32_t fileId, int32_t &ownerAlbumId)
{
    ownerAlbumId = 0;
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    auto resultSet = g_rdbStore->Query(predicates, {PhotoColumn::PHOTO_OWNER_ALBUM_ID});
    if (resultSet == nullptr) {
        return false;
    }
    if (resultSet->GoToFirstRow() == E_OK) {
        ownerAlbumId = GetInt32Val(PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSet);
    }
    resultSet->Close();
    MEDIA_INFO_LOG("fileId: %{public}d, ownerAlbumId: %{public}d", fileId, ownerAlbumId);
    return true;
}
}  // namespace

void MediaFileManagerOfflineCleanupTaskTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, TEST_TABLES, true);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, CREATE_TABLE_SQL_LIST);
}

void MediaFileManagerOfflineCleanupTaskTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, TEST_TABLES, false);
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void MediaFileManagerOfflineCleanupTaskTest::SetUp()
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, TEST_TABLES, false);
}

void MediaFileManagerOfflineCleanupTaskTest::TearDown()
{
    MedialibrarySubscriber::currentStatus_ = false;
}

HWTEST_F(MediaFileManagerOfflineCleanupTaskTest, ResolveTargetAlbumName_IgnoreLegacyConflict_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ResolveTargetAlbumName_IgnoreLegacyConflict_001 start");
    InsertAlbum("Travel", "/FromDocs/Travel",
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER));

    MediaFileManagerOfflineCleanupTask task;
    OfflineCleanupAlbumRecord sourceAlbum;
    sourceAlbum.albumName = "Travel";

    EXPECT_EQ(task.ResolveTargetAlbumName(sourceAlbum), "Travel");
}

HWTEST_F(MediaFileManagerOfflineCleanupTaskTest, ResolveTargetAlbumName_AppendSequenceForNormalAlbum_002,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("ResolveTargetAlbumName_AppendSequenceForNormalAlbum_002 start");
    InsertAlbum("travel", "/Pictures/travel", static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC));
    InsertAlbum("Travel 1", "/Pictures/travel1", static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC));

    MediaFileManagerOfflineCleanupTask task;
    OfflineCleanupAlbumRecord sourceAlbum;
    sourceAlbum.albumName = "Travel";

    EXPECT_EQ(task.ResolveTargetAlbumName(sourceAlbum), "Travel 2");
}

HWTEST_F(MediaFileManagerOfflineCleanupTaskTest, QueryAlbumByLpath_ShouldIgnoreCase_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryAlbumByLpath_ShouldIgnoreCase_003 start");
    int32_t albumId = InsertAlbum("Holiday", "/Pictures/Holiday",
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC));

    MediaFileManagerOfflineCleanupDao dao;
    OfflineCleanupAlbumRecord album;
    ASSERT_TRUE(dao.QueryAlbumByLpath("/pictures/holiday", album));
    EXPECT_EQ(album.albumId, albumId);
    EXPECT_EQ(album.albumName, "Holiday");
}

HWTEST_F(MediaFileManagerOfflineCleanupTaskTest, QueryLegacyAlbumPhotos_ShouldReturnActiveLegacyPhotosOnly_004,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryLegacyAlbumPhotos_ShouldReturnActiveLegacyPhotosOnly_004 start");
    int32_t legacyAlbumId = InsertAlbum("Docs", "/FromDocs/Docs",
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER));
    int32_t normalAlbumId = InsertAlbum("Pictures", "/Pictures/Pictures",
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC));

    int32_t expectedFileId = InsertPhoto("legacy_active", legacyAlbumId, "/storage/emulated/0/FromDocs/Docs/a.jpg");
    InsertPhoto("legacy_cleaned", legacyAlbumId, "/storage/emulated/0/FromDocs/Docs/b.jpg", {
        static_cast<int32_t>(CleanType::TYPE_OFFLINE_CLEAN), TIME_PENDING_OFFLINE_CLEANUP });
    InsertPhoto("normal_active", normalAlbumId, "/storage/emulated/0/Pictures/c.jpg");

    MediaFileManagerOfflineCleanupDao dao;
    auto photos = dao.QueryLegacyAlbumPhotos(0, TEST_LIMIT);

    ASSERT_EQ(photos.size(), 1);
    EXPECT_EQ(photos[0].fileId, expectedFileId);
    EXPECT_EQ(photos[0].ownerAlbumId, legacyAlbumId);
    EXPECT_EQ(photos[0].albumName, "Docs");
    EXPECT_EQ(photos[0].albumLpath, "/FromDocs/Docs");
}

HWTEST_F(MediaFileManagerOfflineCleanupTaskTest, QueryEmptyLegacyAlbums_ShouldSkipReferencedAndDeleted_005,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryEmptyLegacyAlbums_ShouldSkipReferencedAndDeleted_005 start");
    int32_t emptyLegacyAlbumId = InsertAlbum("EmptyDocs", "/FromDocs/EmptyDocs",
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER));
    int32_t referencedLegacyAlbumId = InsertAlbum("ReferencedDocs", "/FromDocs/ReferencedDocs",
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER));
    InsertAlbum("DeletedDocs", "/FromDocs/DeletedDocs",
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER),
        static_cast<int32_t>(DirtyType::TYPE_DELETED));
    InsertPhoto("legacy_ref", referencedLegacyAlbumId, "/storage/emulated/0/FromDocs/ReferencedDocs/a.jpg");

    MediaFileManagerOfflineCleanupDao dao;
    auto albums = dao.QueryEmptyLegacyAlbums(0, TEST_LIMIT);

    ASSERT_EQ(albums.size(), 1);
    EXPECT_EQ(albums[0].albumId, emptyLegacyAlbumId);
    EXPECT_EQ(albums[0].albumName, "EmptyDocs");
}

HWTEST_F(MediaFileManagerOfflineCleanupTaskTest, EnsureTargetAlbum_ShouldReviveDeletedTargetAlbum_006,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("EnsureTargetAlbum_ShouldReviveDeletedTargetAlbum_006 start");
    int32_t deletedAlbumId = InsertAlbum("Travel", "/Travel",
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC),
        static_cast<int32_t>(DirtyType::TYPE_DELETED));

    MediaFileManagerOfflineCleanupTask task;
    OfflineCleanupAlbumRecord sourceAlbum;
    sourceAlbum.albumName = "Travel";
    sourceAlbum.lpath = "/FromDocs/Travel";
    sourceAlbum.albumSubtype = static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER);

    int32_t targetAlbumId = task.EnsureTargetAlbum(sourceAlbum);
    int32_t albumDirty = 0;
    ASSERT_TRUE(QueryAlbumDirty(deletedAlbumId, albumDirty));
    EXPECT_EQ(targetAlbumId, deletedAlbumId);
    EXPECT_EQ(albumDirty, static_cast<int32_t>(DirtyType::TYPE_MDIRTY));
}

HWTEST_F(MediaFileManagerOfflineCleanupTaskTest, EnsureTargetAlbum_ShouldCreateAlbumAndReturnId_007,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("EnsureTargetAlbum_ShouldCreateAlbumAndReturnId_007 start");
    MediaFileManagerOfflineCleanupTask task;
    OfflineCleanupAlbumRecord sourceAlbum;
    sourceAlbum.albumName = "NewTravel";
    sourceAlbum.lpath = "/FromDocs/NewTravel";
    sourceAlbum.albumSubtype = static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER);

    int32_t targetAlbumId = task.EnsureTargetAlbum(sourceAlbum);
    ASSERT_GT(targetAlbumId, 0);

    MediaFileManagerOfflineCleanupDao dao;
    OfflineCleanupAlbumRecord targetAlbum;
    ASSERT_TRUE(dao.QueryAlbumByLpath("/NewTravel", targetAlbum));
    EXPECT_EQ(targetAlbum.albumId, targetAlbumId);
    EXPECT_EQ(targetAlbum.albumName, "NewTravel");
    EXPECT_EQ(targetAlbum.albumSubtype, static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC));
}

HWTEST_F(MediaFileManagerOfflineCleanupTaskTest, ResetRunState_ShouldClearStatisticsAndCache_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("ResetRunState_ShouldClearStatisticsAndCache_008 start");
    MediaFileManagerOfflineCleanupTask task;
    task.targetAlbumIdCache_["/pictures/test"] = 1;
    task.statistics_.markedForDeletion.count = 1;
    task.statistics_.deletedPhotos.count = 2;
    task.statistics_.burstConverted.count = 3;
    task.statistics_.localCloudConverted.count = 4;
    task.statistics_.cloudOnlyConverted.count = 5;
    task.statistics_.albumRelationsMigrated.count = 6;
    task.statistics_.legacyAlbumsDeleted.count = 7;
    task.statistics_.convertedAlbumsDeleted.count = 8;

    task.ResetRunState();

    EXPECT_TRUE(task.targetAlbumIdCache_.empty());
    EXPECT_EQ(task.statistics_.markedForDeletion.count, 0);
    EXPECT_EQ(task.statistics_.deletedPhotos.count, 0);
    EXPECT_EQ(task.statistics_.burstConverted.count, 0);
    EXPECT_EQ(task.statistics_.localCloudConverted.count, 0);
    EXPECT_EQ(task.statistics_.cloudOnlyConverted.count, 0);
    EXPECT_EQ(task.statistics_.albumRelationsMigrated.count, 0);
    EXPECT_EQ(task.statistics_.legacyAlbumsDeleted.count, 0);
    EXPECT_EQ(task.statistics_.convertedAlbumsDeleted.count, 0);
}

HWTEST_F(MediaFileManagerOfflineCleanupTaskTest, QueryPendingDeletedPhotos_ShouldReturnAuditFields_009,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryPendingDeletedPhotos_ShouldReturnAuditFields_009 start");
    int32_t albumId = InsertAlbum("Audit", "/FromDocs/Audit",
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER));
    int32_t fileId = InsertPhoto("audit_photo", albumId, "/storage/emulated/0/FromDocs/Audit/audit_photo.jpg", {
        static_cast<int32_t>(CleanType::TYPE_OFFLINE_CLEAN), TIME_PENDING_OFFLINE_CLEANUP });
    InsertPhoto("audit_photo_cloud", albumId, "/storage/emulated/0/FromDocs/Audit/audit_photo_cloud.jpg", {
        static_cast<int32_t>(CleanType::TYPE_OFFLINE_CLEAN), TIME_PENDING_OFFLINE_CLEANUP,
        static_cast<int32_t>(FileSourceType::FILE_MANAGER), static_cast<int32_t>(PhotoPositionType::CLOUD) });
    InsertPhoto("audit_photo_media", albumId, "/storage/emulated/0/FromDocs/Audit/audit_photo_media.jpg", {
        static_cast<int32_t>(CleanType::TYPE_OFFLINE_CLEAN), TIME_PENDING_OFFLINE_CLEANUP,
        static_cast<int32_t>(FileSourceType::MEDIA), static_cast<int32_t>(PhotoPositionType::LOCAL) });

    MediaFileManagerOfflineCleanupDao dao;
    auto photos = dao.QueryPendingDeletedPhotos(0, TEST_LIMIT);

    ASSERT_EQ(photos.size(), 1);
    EXPECT_EQ(photos[0].fileId, fileId);
    EXPECT_EQ(photos[0].mediaType, TEST_MEDIA_TYPE_IMAGE);
    EXPECT_EQ(photos[0].size, 1);
    EXPECT_EQ(photos[0].displayName, "audit_photo.jpg");
}

HWTEST_F(MediaFileManagerOfflineCleanupTaskTest, DeleteOfflineCleanupPhotos_ShouldProtectByPendingAndCleanFlag_010,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteOfflineCleanupPhotos_ShouldProtectByPendingAndCleanFlag_010 start");
    int32_t albumId = InsertAlbum("DeleteGuard", "/FromDocs/DeleteGuard",
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER));
    int32_t deletableFileId = InsertPhoto("deletable_photo", albumId,
        "/storage/emulated/0/FromDocs/DeleteGuard/deletable_photo.jpg", {
        static_cast<int32_t>(CleanType::TYPE_OFFLINE_CLEAN), TIME_PENDING_OFFLINE_CLEANUP });
    int32_t revertedFileId = InsertPhoto("reverted_photo", albumId,
        "/storage/emulated/0/FromDocs/DeleteGuard/reverted_photo.jpg", {
        static_cast<int32_t>(CleanType::TYPE_OFFLINE_CLEAN), 0 });
    int32_t dirtyMismatchFileId = InsertPhoto("dirty_mismatch_photo", albumId,
        "/storage/emulated/0/FromDocs/DeleteGuard/dirty_mismatch_photo.jpg", {
        static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN), TIME_PENDING_OFFLINE_CLEANUP });

    MediaFileManagerOfflineCleanupDao dao;
    AccurateRefresh::AssetAccurateRefresh assetRefresh;
    int32_t deletedRows = 0;
    ASSERT_TRUE(dao.DeleteOfflineCleanupPhotos({std::to_string(deletableFileId), std::to_string(revertedFileId),
        std::to_string(dirtyMismatchFileId)}, assetRefresh, deletedRows));
    int32_t deletableCount = 0;
    int32_t revertedCount = 0;
    int32_t dirtyMismatchCount = 0;
    ASSERT_TRUE(QueryPhotoCountByFileId(deletableFileId, deletableCount));
    ASSERT_TRUE(QueryPhotoCountByFileId(revertedFileId, revertedCount));
    ASSERT_TRUE(QueryPhotoCountByFileId(dirtyMismatchFileId, dirtyMismatchCount));

    EXPECT_EQ(deletedRows, 1);
    EXPECT_EQ(deletableCount, 0);
    EXPECT_EQ(revertedCount, 1);
    EXPECT_EQ(dirtyMismatchCount, 1);
}

HWTEST_F(MediaFileManagerOfflineCleanupTaskTest, QueryLocalDeleteCandidates_ShouldReturnOwnerAlbumId_011,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryLocalDeleteCandidates_ShouldReturnOwnerAlbumId_011 start");
    int32_t albumId = InsertAlbum("OwnerAlbum", "/FromDocs/OwnerAlbum",
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER));
    int32_t fileId = InsertPhoto("owner_photo", albumId,
        "/storage/emulated/0/FromDocs/OwnerAlbum/owner_photo.jpg");

    MediaFileManagerOfflineCleanupDao dao;
    auto photos = dao.QueryLocalDeleteCandidates(0, TEST_LIMIT);

    ASSERT_EQ(photos.size(), static_cast<size_t>(1));
    EXPECT_EQ(photos[0].fileId, fileId);
    EXPECT_EQ(photos[0].ownerAlbumId, albumId);
}

HWTEST_F(MediaFileManagerOfflineCleanupTaskTest, QueryLegacyAlbumPhotos_ShouldReturnFileSourceType_012,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryLegacyAlbumPhotos_ShouldReturnFileSourceType_012 start");
    int32_t legacyAlbumId = InsertAlbum("FmAlbum", "/FromDocs/FmAlbum",
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER));
    int32_t fmFileId = InsertPhoto("fm_photo", legacyAlbumId,
        "/storage/emulated/0/FromDocs/FmAlbum/fm_photo.jpg");
    int32_t mediaFileId = InsertPhoto("media_photo", legacyAlbumId,
        "/storage/emulated/0/FromDocs/FmAlbum/media_photo.jpg", {
        static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN), 0,
        static_cast<int32_t>(FileSourceType::MEDIA), static_cast<int32_t>(PhotoPositionType::LOCAL) });

    MediaFileManagerOfflineCleanupDao dao;
    auto photos = dao.QueryLegacyAlbumPhotos(0, TEST_LIMIT);

    ASSERT_EQ(photos.size(), static_cast<size_t>(2));
    EXPECT_EQ(photos[0].fileId, fmFileId);
    EXPECT_EQ(photos[0].fileSourceType, static_cast<int32_t>(FileSourceType::FILE_MANAGER));
    EXPECT_EQ(photos[1].fileId, mediaFileId);
    EXPECT_EQ(photos[1].fileSourceType, static_cast<int32_t>(FileSourceType::MEDIA));
}

HWTEST_F(MediaFileManagerOfflineCleanupTaskTest, ShouldMarkForDeletion_PureResidueIsMarked_013, TestSize.Level1)
{
    MEDIA_INFO_LOG("ShouldMarkForDeletion_PureResidueIsMarked_013 start");
    MediaFileManagerOfflineCleanupTask task;
    OfflineCleanupPhotoRecord photo;
    photo.fileId = 1;
    photo.storagePath = "/storage/emulated/0/nonexistent_storage_path.jpg";
    photo.data = "/storage/media/local/files/Photo/nonexistent_data_path.jpg";

    EXPECT_TRUE(task.ShouldMarkForDeletion(photo));
}

HWTEST_F(MediaFileManagerOfflineCleanupTaskTest, ShouldMarkForDeletion_SkipsWhenSandboxCopyExists_014,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("ShouldMarkForDeletion_SkipsWhenSandboxCopyExists_014 start");
    const std::string dataPath = "/data/local/tmp/offline_cleanup_sandbox_copy.jpg";
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFileFS(dataPath));
    ASSERT_TRUE(MediaFileUtils::IsFileExists(dataPath));

    MediaFileManagerOfflineCleanupTask task;
    OfflineCleanupPhotoRecord photo;
    photo.fileId = 1;
    photo.storagePath = "/storage/emulated/0/nonexistent_storage_path.jpg";
    photo.data = dataPath;

    EXPECT_FALSE(task.ShouldMarkForDeletion(photo));

    MediaFileUtils::DeleteFile(dataPath);
}

HWTEST_F(MediaFileManagerOfflineCleanupTaskTest, MigratePhotoAlbumRelations_SkipsFileManagerAssets_015,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("MigratePhotoAlbumRelations_SkipsFileManagerAssets_015 start");
    MedialibrarySubscriber::currentStatus_ = true;
    int32_t legacyAlbumId = InsertAlbum("FmMigrate", "/FromDocs/FmMigrate",
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER));
    InsertAlbum("FmMigrate", "/FmMigrate",
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC));
    int32_t fileId = InsertPhoto("fm_skip_photo", legacyAlbumId,
        "/storage/emulated/0/FromDocs/FmMigrate/fm_skip_photo.jpg");

    MediaFileManagerOfflineCleanupTask task;
    task.ResetAllCursors();
    task.MigratePhotoAlbumRelations();

    int32_t ownerAlbumId = 0;
    ASSERT_TRUE(QueryPhotoOwnerAlbumId(fileId, ownerAlbumId));
    EXPECT_EQ(ownerAlbumId, legacyAlbumId);
}

HWTEST_F(MediaFileManagerOfflineCleanupTaskTest, LogicalDeleteEmptyConvertedAlbums_DeletesAnySubtype_016,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("LogicalDeleteEmptyConvertedAlbums_DeletesAnySubtype_016 start");
    int32_t sourceAlbumId = InsertAlbum("ConvertedSource", "/ConvertedSource",
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC));
    int32_t userAlbumId = InsertAlbum("ConvertedUser", "/ConvertedUser",
        static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC),
        static_cast<int32_t>(DirtyType::TYPE_SYNCED),
        static_cast<int32_t>(PhotoAlbumType::USER));

    MediaFileManagerOfflineCleanupDao dao;
    AccurateRefresh::AlbumAccurateRefresh albumRefresh;
    int32_t deletedCount = -1;
    ASSERT_TRUE(dao.LogicalDeleteEmptyConvertedAlbums({sourceAlbumId, userAlbumId}, albumRefresh, deletedCount));
    EXPECT_EQ(deletedCount, 2);

    int32_t sourceDirty = 0;
    ASSERT_TRUE(QueryAlbumDirty(sourceAlbumId, sourceDirty));
    EXPECT_EQ(sourceDirty, static_cast<int32_t>(DirtyType::TYPE_DELETED));
    int32_t userDirty = 0;
    ASSERT_TRUE(QueryAlbumDirty(userAlbumId, userDirty));
    EXPECT_EQ(userDirty, static_cast<int32_t>(DirtyType::TYPE_DELETED));
}

HWTEST_F(MediaFileManagerOfflineCleanupTaskTest, QueryEmptyConvertedAlbums_ReturnsOnlyEmptyNonDeleted_017,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryEmptyConvertedAlbums_ReturnsOnlyEmptyNonDeleted_017 start");
    int32_t emptyA = InsertAlbum("ConvA", "/ConvA",
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC));
    int32_t nonEmptyB = InsertAlbum("ConvB", "/ConvB",
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC));
    int32_t emptyC = InsertAlbum("ConvC", "/ConvC",
        static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC),
        static_cast<int32_t>(DirtyType::TYPE_SYNCED),
        static_cast<int32_t>(PhotoAlbumType::USER));
    int32_t deletedD = InsertAlbum("ConvD", "/ConvD",
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC),
        static_cast<int32_t>(DirtyType::TYPE_DELETED));
    InsertPhoto("ref_photo", nonEmptyB, "/storage/emulated/0/ConvB/ref_photo.jpg");

    MediaFileManagerOfflineCleanupDao dao;
    auto albums = dao.QueryEmptyConvertedAlbums({emptyA, nonEmptyB, emptyC, deletedD});

    ASSERT_EQ(albums.size(), static_cast<size_t>(2));
    EXPECT_EQ(albums[0].albumId, emptyA);
    EXPECT_EQ(albums[1].albumId, emptyC);
}

HWTEST_F(MediaFileManagerOfflineCleanupTaskTest, CleanupConvertedAlbums_DeletesEmptyCachedAlbums_018,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("CleanupConvertedAlbums_DeletesEmptyCachedAlbums_018 start");
    MedialibrarySubscriber::currentStatus_ = true;
    int32_t emptyAlbumId = InsertAlbum("ConvEmpty", "/ConvEmpty",
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC));
    int32_t nonEmptyAlbumId = InsertAlbum("ConvNonEmpty", "/ConvNonEmpty",
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC));
    InsertPhoto("keep_photo", nonEmptyAlbumId, "/storage/emulated/0/ConvNonEmpty/keep_photo.jpg");

    MediaFileManagerOfflineCleanupTask task;
    task.convertedAlbumIdCache_.insert(emptyAlbumId);
    task.convertedAlbumIdCache_.insert(nonEmptyAlbumId);
    task.CleanupConvertedAlbums();

    int32_t emptyDirty = 0;
    ASSERT_TRUE(QueryAlbumDirty(emptyAlbumId, emptyDirty));
    EXPECT_EQ(emptyDirty, static_cast<int32_t>(DirtyType::TYPE_DELETED));
    int32_t nonEmptyDirty = 0;
    ASSERT_TRUE(QueryAlbumDirty(nonEmptyAlbumId, nonEmptyDirty));
    EXPECT_NE(nonEmptyDirty, static_cast<int32_t>(DirtyType::TYPE_DELETED));
    EXPECT_EQ(task.statistics_.convertedAlbumsDeleted.count, 1);
}

HWTEST_F(MediaFileManagerOfflineCleanupTaskTest, ResetRunState_PreservesConvertedAlbumCache_019, TestSize.Level1)
{
    MEDIA_INFO_LOG("ResetRunState_PreservesConvertedAlbumCache_019 start");
    MediaFileManagerOfflineCleanupTask task;
    task.convertedAlbumIdCache_.insert(11);
    task.convertedAlbumIdCache_.insert(22);
    task.statistics_.convertedAlbumsDeleted.count = 5;

    task.ResetRunState();

    EXPECT_EQ(task.convertedAlbumIdCache_.size(), static_cast<size_t>(2));
    EXPECT_EQ(task.statistics_.convertedAlbumsDeleted.count, 0);
}
}

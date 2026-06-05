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

#define MLOG_TAG "MediaLakeMonitorRdbUtilsTest"

#include "media_lake_monitor_rdb_utils_test.h"

#include <string>
#include <vector>
#include <unordered_map>

#include "media_column.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "media_file_monitor_rdb_utils.h"
#include "photo_album_column.h"
#include "rdb_helper.h"
#include "rdb_predicates.h"
#include "rdb_utils.h"
#include "media_upgrade.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::Media::AccurateRefresh;

namespace OHOS {
namespace Media {
static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
constexpr int32_t TEST_FILE_ID_1 = 1001;
constexpr int32_t TEST_FILE_ID_2 = 1002;
constexpr int32_t TEST_FILE_ID_3 = 1003;
constexpr int32_t TEST_ALBUM_ID_1 = 2001;
constexpr int32_t TEST_ALBUM_ID_2 = 2002;
constexpr int32_t TEST_ALBUM_ID_3 = 2003;
constexpr int64_t TEST_DATE_TAKEN = 1704067200000;
constexpr int32_t TEST_ALBUM_TYPE = 2048;
constexpr int32_t TEST_ALBUM_SUBTYPE = 2049;
const string TEST_STORAGE_PATH = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/test.jpg";
const string TEST_LPATH = "/test/album";
const string TEST_LPATH_SUB = "/test/album/sub";
const string TEST_PHOTO_PATH = "/storage/media/local/files/test.jpg";
const string TEST_HO_DIR_PREFIX = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC";
const string TEST_HO_PATH = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/test/album";

void MediaLakeMonitorRdbUtilsTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, {PhotoColumn::PHOTOS_TABLE, PhotoAlbumColumns::TABLE});
}

void MediaLakeMonitorRdbUtilsTest::TearDownTestCase()
{
    MediaLibraryUnitTestUtils::CleanTestTables(
        g_rdbStore, {PhotoColumn::PHOTOS_TABLE, PhotoAlbumColumns::TABLE}, false);
}

void MediaLakeMonitorRdbUtilsTest::SetUp()
{
    MediaLibraryUnitTestUtils::CleanTestTables(
        g_rdbStore, {PhotoColumn::PHOTOS_TABLE, PhotoAlbumColumns::TABLE}, false);
}

void MediaLakeMonitorRdbUtilsTest::TearDown() {}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryDataByDeletedStoragePath_ValidData_ReturnSuccess, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryDataByDeletedStoragePath_ValidData_ReturnSuccess start");

    string insertSql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + ", " + MediaColumn::MEDIA_FILE_PATH + ", " +
        PhotoColumn::PHOTO_STORAGE_PATH + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
        MediaColumn::MEDIA_HIDDEN + ", " + PhotoColumn::PHOTO_POSITION + ") VALUES (" +
        to_string(TEST_FILE_ID_1) + ", " + to_string(TEST_ALBUM_ID_1) + ", " +
        to_string(TEST_DATE_TAKEN) + ", '" + TEST_PHOTO_PATH + "', '" +
        TEST_STORAGE_PATH + "', 0, 0, 0)";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    EXPECT_EQ(ret, E_OK);

    LakeMonitorQueryResultData data;
    bool result = MediaFileMonitorRdbUtils::QueryDataByDeletedStoragePath(g_rdbStore,
        TEST_STORAGE_PATH, data);

    EXPECT_TRUE(result);
    EXPECT_EQ(data.fileId, TEST_FILE_ID_1);
    EXPECT_EQ(data.albumId, TEST_ALBUM_ID_1);
    EXPECT_EQ(data.dateTaken, TEST_DATE_TAKEN);
    EXPECT_EQ(data.photoPath, TEST_PHOTO_PATH);

    MEDIA_INFO_LOG("QueryDataByDeletedStoragePath_ValidData_ReturnSuccess end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryDataByDeletedStoragePath_NullRdbStore_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryDataByDeletedStoragePath_NullRdbStore_ReturnFalse start");

    std::shared_ptr<MediaLibraryRdbStore> nullRdbStore;
    LakeMonitorQueryResultData data;
    bool result = MediaFileMonitorRdbUtils::QueryDataByDeletedStoragePath(nullRdbStore,
        TEST_STORAGE_PATH, data);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("QueryDataByDeletedStoragePath_NullRdbStore_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryDataByDeletedStoragePath_EmptyPath_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryDataByDeletedStoragePath_EmptyPath_ReturnFalse start");

    LakeMonitorQueryResultData data;
    bool result = MediaFileMonitorRdbUtils::QueryDataByDeletedStoragePath(g_rdbStore, "", data);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("QueryDataByDeletedStoragePath_EmptyPath_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryDataByDeletedStoragePath_TrashedFile_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryDataByDeletedStoragePath_TrashedFile_ReturnFalse start");

    string insertSql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + ", " + MediaColumn::MEDIA_FILE_PATH + ", " +
        PhotoColumn::PHOTO_STORAGE_PATH + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
        MediaColumn::MEDIA_HIDDEN + ", " + PhotoColumn::PHOTO_POSITION + ") VALUES (" +
        to_string(TEST_FILE_ID_1) + ", " + to_string(TEST_ALBUM_ID_1) + ", " +
        to_string(TEST_DATE_TAKEN) + ", '" + TEST_PHOTO_PATH + "', '" +
        TEST_STORAGE_PATH + "', 1234567890, 0, 0)";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    EXPECT_EQ(ret, E_OK);

    LakeMonitorQueryResultData data;
    bool result = MediaFileMonitorRdbUtils::QueryDataByDeletedStoragePath(g_rdbStore,
        TEST_STORAGE_PATH, data);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("QueryDataByDeletedStoragePath_TrashedFile_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryDataByDeletedStoragePath_HiddenFile_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryDataByDeletedStoragePath_HiddenFile_ReturnFalse start");

    string insertSql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + ", " + MediaColumn::MEDIA_FILE_PATH + ", " +
        PhotoColumn::PHOTO_STORAGE_PATH + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
        MediaColumn::MEDIA_HIDDEN + ", " + PhotoColumn::PHOTO_POSITION + ") VALUES (" +
        to_string(TEST_FILE_ID_1) + ", " + to_string(TEST_ALBUM_ID_1) + ", " +
        to_string(TEST_DATE_TAKEN) + ", '" + TEST_PHOTO_PATH + "', '" +
        TEST_STORAGE_PATH + "', 0, 1, 0)";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    EXPECT_EQ(ret, E_OK);

    LakeMonitorQueryResultData data;
    bool result = MediaFileMonitorRdbUtils::QueryDataByDeletedStoragePath(g_rdbStore,
        TEST_STORAGE_PATH, data);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("QueryDataByDeletedStoragePath_HiddenFile_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryDataByDeletedStoragePath_CloudPosition_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryDataByDeletedStoragePath_CloudPosition_ReturnFalse start");

    string insertSql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" + MediaColumn::MEDIA_ID + ", " +
        PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " + MediaColumn::MEDIA_DATE_TAKEN + ", " +
        MediaColumn::MEDIA_FILE_PATH + ", " + PhotoColumn::PHOTO_STORAGE_PATH + ", " +
        MediaColumn::MEDIA_DATE_TRASHED + ", " + MediaColumn::MEDIA_HIDDEN + ", " +
        PhotoColumn::PHOTO_POSITION + ") VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
    std::vector<NativeRdb::ValueObject> args = {TEST_FILE_ID_1, TEST_ALBUM_ID_1, TEST_DATE_TAKEN, TEST_PHOTO_PATH,
        TEST_STORAGE_PATH, 0, 0, static_cast<int32_t>(PhotoPositionType::CLOUD)};
    int32_t ret = g_rdbStore->ExecuteSql(insertSql, args);
    EXPECT_EQ(ret, E_OK);

    LakeMonitorQueryResultData data;
    bool result = MediaFileMonitorRdbUtils::QueryDataByDeletedStoragePath(g_rdbStore, TEST_STORAGE_PATH, data);
    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("QueryDataByDeletedStoragePath_CloudPosition_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryAlbumIdsByLPath_ValidData_ReturnSuccess, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryAlbumIdsByLPath_ValidData_ReturnSuccess start");

    string insertSql = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
        PhotoAlbumColumns::ALBUM_LPATH + ") VALUES (" +
        to_string(TEST_ALBUM_ID_1) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'album', '" + TEST_LPATH + "')";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    EXPECT_EQ(ret, E_OK);

    vector<int32_t> albumIds;
    bool result = MediaFileMonitorRdbUtils::QueryAlbumIdsByLPath(g_rdbStore, TEST_LPATH,
        albumIds);

    EXPECT_TRUE(result);
    EXPECT_EQ(albumIds.size(), 1);
    EXPECT_EQ(albumIds[0], TEST_ALBUM_ID_1);

    MEDIA_INFO_LOG("QueryAlbumIdsByLPath_ValidData_ReturnSuccess end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryAlbumIdsByLPath_Subdirectory_ReturnSuccess, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryAlbumIdsByLPath_Subdirectory_ReturnSuccess start");

    string insertSql1 = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
        PhotoAlbumColumns::ALBUM_LPATH + ") VALUES (" +
        to_string(TEST_ALBUM_ID_1) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'album', '" + TEST_LPATH + "')";
    string insertSql2 = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
        PhotoAlbumColumns::ALBUM_LPATH + ") VALUES (" +
        to_string(TEST_ALBUM_ID_2) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'sub', '" +
        TEST_LPATH_SUB + "')";
    int32_t ret1 = g_rdbStore->ExecuteSql(insertSql1);
    int32_t ret2 = g_rdbStore->ExecuteSql(insertSql2);
    EXPECT_EQ(ret1, E_OK);
    EXPECT_EQ(ret2, E_OK);

    vector<int32_t> albumIds;
    bool result = MediaFileMonitorRdbUtils::QueryAlbumIdsByLPath(g_rdbStore, TEST_LPATH,
        albumIds);

    EXPECT_TRUE(result);
    EXPECT_EQ(albumIds.size(), 2);

    MEDIA_INFO_LOG("QueryAlbumIdsByLPath_Subdirectory_ReturnSuccess end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryAlbumIdsByLPath_NullRdbStore_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryAlbumIdsByLPath_NullRdbStore_ReturnFalse start");

    std::shared_ptr<MediaLibraryRdbStore> nullRdbStore;
    vector<int32_t> albumIds;
    bool result = MediaFileMonitorRdbUtils::QueryAlbumIdsByLPath(nullRdbStore, TEST_LPATH, albumIds);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("QueryAlbumIdsByLPath_NullRdbStore_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryAlbumIdsByLPath_EmptyLPath_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryAlbumIdsByLPath_EmptyLPath_ReturnFalse start");

    vector<int32_t> albumIds;
    bool result = MediaFileMonitorRdbUtils::QueryAlbumIdsByLPath(g_rdbStore, "", albumIds);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("QueryAlbumIdsByLPath_EmptyLPath_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryAlbumIdsByLPath_NoMatchingAlbums_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryAlbumIdsByLPath_NoMatchingAlbums_ReturnFalse start");

    vector<int32_t> albumIds;
    bool result = MediaFileMonitorRdbUtils::QueryAlbumIdsByLPath(g_rdbStore, "/test/nonexistent", albumIds);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("QueryAlbumIdsByLPath_NoMatchingAlbums_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryAlbumByLPath_ValidData_ReturnSuccess, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryAlbumByLPath_ValidData_ReturnSuccess start");

    string insertSql = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
        PhotoAlbumColumns::ALBUM_LPATH + ", " + PhotoAlbumColumns::ALBUM_COUNT + ") VALUES (" +
        to_string(TEST_ALBUM_ID_1) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'album', '" + TEST_LPATH + "', 5)";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    EXPECT_EQ(ret, E_OK);

    vector<int32_t> albumIds;
    unordered_map<int32_t, int32_t> albumCounts;
    bool result = MediaFileMonitorRdbUtils::QueryAlbumByLPath(g_rdbStore, TEST_LPATH,
        albumIds, albumCounts);

    EXPECT_TRUE(result);
    EXPECT_EQ(albumIds.size(), 1);
    EXPECT_EQ(albumIds[0], TEST_ALBUM_ID_1);
    EXPECT_EQ(albumCounts[TEST_ALBUM_ID_1], 5);

    MEDIA_INFO_LOG("QueryAlbumByLPath_ValidData_ReturnSuccess end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryAlbumByLPath_MultipleAlbums_ReturnSuccess, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryAlbumByLPath_MultipleAlbums_ReturnSuccess start");

    string insertSql1 = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
        PhotoAlbumColumns::ALBUM_LPATH + ", " + PhotoAlbumColumns::ALBUM_COUNT + ") VALUES (" +
        to_string(TEST_ALBUM_ID_1) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'album', '" + TEST_LPATH + "', 5)";
    string insertSql2 = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
        PhotoAlbumColumns::ALBUM_LPATH + ", " + PhotoAlbumColumns::ALBUM_COUNT + ") VALUES (" +
        to_string(TEST_ALBUM_ID_2) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'sub', '" + TEST_LPATH_SUB + "', 3)";
    int32_t ret1 = g_rdbStore->ExecuteSql(insertSql1);
    int32_t ret2 = g_rdbStore->ExecuteSql(insertSql2);
    EXPECT_EQ(ret1, E_OK);
    EXPECT_EQ(ret2, E_OK);

    vector<int32_t> albumIds;
    unordered_map<int32_t, int32_t> albumCounts;
    bool result = MediaFileMonitorRdbUtils::QueryAlbumByLPath(g_rdbStore, TEST_LPATH,
        albumIds, albumCounts);

    EXPECT_TRUE(result);
    EXPECT_EQ(albumIds.size(), 2);
    EXPECT_EQ(albumCounts[TEST_ALBUM_ID_1], 5);
    EXPECT_EQ(albumCounts[TEST_ALBUM_ID_2], 3);

    MEDIA_INFO_LOG("QueryAlbumByLPath_MultipleAlbums_ReturnSuccess end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryAlbumByLPath_NullRdbStore_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryAlbumByLPath_NullRdbStore_ReturnFalse start");

    std::shared_ptr<MediaLibraryRdbStore> nullRdbStore;
    vector<int32_t> albumIds;
    unordered_map<int32_t, int32_t> albumCounts;
    bool result = MediaFileMonitorRdbUtils::QueryAlbumByLPath(nullRdbStore, TEST_LPATH, albumIds, albumCounts);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("QueryAlbumByLPath_NullRdbStore_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryDataListByAlbumIds_ValidData_ReturnSuccess, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryDataListByAlbumIds_ValidData_ReturnSuccess start");

    string insertAlbumSql = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ") VALUES (" +
        to_string(TEST_ALBUM_ID_1) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'test')";
    int32_t retAlbum = g_rdbStore->ExecuteSql(insertAlbumSql);
    EXPECT_EQ(retAlbum, E_OK);

    string insertPhotoSql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + ", " + MediaColumn::MEDIA_FILE_PATH + ", " +
        PhotoColumn::PHOTO_FILE_SOURCE_TYPE + ") VALUES (" +
        to_string(TEST_FILE_ID_1) + ", " + to_string(TEST_ALBUM_ID_1) + ", " +
        to_string(TEST_DATE_TAKEN) + ", '" + TEST_PHOTO_PATH + "', 3)";
    int32_t retPhoto = g_rdbStore->ExecuteSql(insertPhotoSql);
    EXPECT_EQ(retPhoto, E_OK);

    vector<int32_t> albumIds = {TEST_ALBUM_ID_1};
    vector<LakeMonitorQueryResultData> dataList;
    bool result = MediaFileMonitorRdbUtils::QueryDataListByAlbumIds(g_rdbStore, albumIds, dataList,
        FileSourceType::MEDIA_HO_LAKE);

    EXPECT_TRUE(result);
    EXPECT_EQ(dataList.size(), 1);
    EXPECT_EQ(dataList[0].fileId, TEST_FILE_ID_1);
    EXPECT_EQ(dataList[0].albumId, TEST_ALBUM_ID_1);

    MEDIA_INFO_LOG("QueryDataListByAlbumIds_ValidData_ReturnSuccess end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryDataListByAlbumIds_MultipleAlbums_ReturnSuccess, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryDataListByAlbumIds_MultipleAlbums_ReturnSuccess start");

    string insertAlbumSql1 = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ") VALUES (" +
        to_string(TEST_ALBUM_ID_1) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'test1')";
    string insertAlbumSql2 = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ") VALUES (" +
        to_string(TEST_ALBUM_ID_2) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'test2')";
    int32_t retAlbum1 = g_rdbStore->ExecuteSql(insertAlbumSql1);
    int32_t retAlbum2 = g_rdbStore->ExecuteSql(insertAlbumSql2);
    EXPECT_EQ(retAlbum1, E_OK);
    EXPECT_EQ(retAlbum2, E_OK);

    string insertPhotoSql1 = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + ", " + MediaColumn::MEDIA_FILE_PATH + ", " +
        PhotoColumn::PHOTO_FILE_SOURCE_TYPE + ") VALUES (" +
        to_string(TEST_FILE_ID_1) + ", " + to_string(TEST_ALBUM_ID_1) + ", " +
        to_string(TEST_DATE_TAKEN) + ", '" + TEST_PHOTO_PATH + "', 3)";
    string insertPhotoSql2 = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + ", " + MediaColumn::MEDIA_FILE_PATH + ", " +
        PhotoColumn::PHOTO_FILE_SOURCE_TYPE + ") VALUES (" +
        to_string(TEST_FILE_ID_2) + ", " + to_string(TEST_ALBUM_ID_2) + ", " +
        to_string(TEST_DATE_TAKEN) + ", '/test2.jpg', 3)";
    int32_t retPhoto1 = g_rdbStore->ExecuteSql(insertPhotoSql1);
    int32_t retPhoto2 = g_rdbStore->ExecuteSql(insertPhotoSql2);
    EXPECT_EQ(retPhoto1, E_OK);
    EXPECT_EQ(retPhoto2, E_OK);

    vector<int32_t> albumIds = {TEST_ALBUM_ID_1, TEST_ALBUM_ID_2};
    vector<LakeMonitorQueryResultData> dataList;
    bool result = MediaFileMonitorRdbUtils::QueryDataListByAlbumIds(g_rdbStore, albumIds, dataList,
        FileSourceType::MEDIA_HO_LAKE);

    EXPECT_TRUE(result);
    EXPECT_EQ(dataList.size(), 2);

    MEDIA_INFO_LOG("QueryDataListByAlbumIds_MultipleAlbums_ReturnSuccess end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryDataListByAlbumIds_NullRdbStore_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryDataListByAlbumIds_NullRdbStore_ReturnFalse start");

    std::shared_ptr<MediaLibraryRdbStore> nullRdbStore;
    vector<int32_t> albumIds = {TEST_ALBUM_ID_1};
    vector<LakeMonitorQueryResultData> dataList;
    bool result = MediaFileMonitorRdbUtils::QueryDataListByAlbumIds(nullRdbStore, albumIds, dataList,
        FileSourceType::MEDIA_HO_LAKE);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("QueryDataListByAlbumIds_NullRdbStore_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryDataListByAlbumIds_EmptyAlbumIds_ReturnSuccess, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryDataListByAlbumIds_EmptyAlbumIds_ReturnSuccess start");

    vector<int32_t> albumIds;
    vector<LakeMonitorQueryResultData> dataList;
    bool result = MediaFileMonitorRdbUtils::QueryDataListByAlbumIds(g_rdbStore, albumIds, dataList,
        FileSourceType::MEDIA_HO_LAKE);

    EXPECT_TRUE(result);
    EXPECT_EQ(dataList.size(), 0);

    MEDIA_INFO_LOG("QueryDataListByAlbumIds_EmptyAlbumIds_ReturnSuccess end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteAssetsByOwnerAlbumIds_ValidData_ReturnSuccess, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteAssetsByOwnerAlbumIds_ValidData_ReturnSuccess start");

    string insertAlbumSql = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ") VALUES (" +
        to_string(TEST_ALBUM_ID_1) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'test')";
    int32_t retAlbum = g_rdbStore->ExecuteSql(insertAlbumSql);
    EXPECT_EQ(retAlbum, E_OK);

    string insertPhotoSql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + ", " + MediaColumn::MEDIA_FILE_PATH + ") VALUES (" +
        to_string(TEST_FILE_ID_1) + ", " + to_string(TEST_ALBUM_ID_1) + ", " +
        to_string(TEST_DATE_TAKEN) + ", '" + TEST_PHOTO_PATH + "')";
    int32_t retPhoto = g_rdbStore->ExecuteSql(insertPhotoSql);
    EXPECT_EQ(retPhoto, E_OK);

    vector<int32_t> albumIds = {TEST_ALBUM_ID_1};
    bool result = MediaFileMonitorRdbUtils::DeleteAssetsByOwnerAlbumIds(g_rdbStore, albumIds,
        FileSourceType::MEDIA_HO_LAKE);

    EXPECT_TRUE(result);

    MEDIA_INFO_LOG("DeleteAssetsByOwnerAlbumIds_ValidData_ReturnSuccess end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteAssetsByOwnerAlbumIds_MultipleAlbums_ReturnSuccess, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteAssetsByOwnerAlbumIds_MultipleAlbums_ReturnSuccess start");

    string insertAlbumSql1 = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ") VALUES (" +
        to_string(TEST_ALBUM_ID_1) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'testMultipleAlbums1')";
    string insertAlbumSql2 = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ") VALUES (" +
        to_string(TEST_ALBUM_ID_2) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'testMultipleAlbums2')";
    EXPECT_EQ(g_rdbStore->ExecuteSql(insertAlbumSql1), E_OK);
    EXPECT_EQ(g_rdbStore->ExecuteSql(insertAlbumSql2), E_OK);

    string insertPhotoSql1 = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + ", " + MediaColumn::MEDIA_FILE_PATH + ") VALUES (" +
        to_string(TEST_FILE_ID_1) + ", " + to_string(TEST_ALBUM_ID_1) + ", " +
        to_string(TEST_DATE_TAKEN) + ", '" + TEST_PHOTO_PATH + "')";
    string insertPhotoSql2 = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + ", " + MediaColumn::MEDIA_FILE_PATH + ") VALUES (" +
        to_string(TEST_FILE_ID_2) + ", " + to_string(TEST_ALBUM_ID_2) + ", " +
        to_string(TEST_DATE_TAKEN) + ", '/testMultipleAlbums.jpg')";
    int32_t retPhoto1 = g_rdbStore->ExecuteSql(insertPhotoSql1);
    int32_t retPhoto2 = g_rdbStore->ExecuteSql(insertPhotoSql2);
    EXPECT_EQ(retPhoto1, E_OK);
    EXPECT_EQ(retPhoto2, E_OK);

    vector<int32_t> albumIds = {TEST_ALBUM_ID_1, TEST_ALBUM_ID_2};
    bool result = MediaFileMonitorRdbUtils::DeleteAssetsByOwnerAlbumIds(g_rdbStore, albumIds,
        FileSourceType::MEDIA_HO_LAKE);

    EXPECT_TRUE(result);

    MEDIA_INFO_LOG("DeleteAssetsByOwnerAlbumIds_MultipleAlbums_ReturnSuccess end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteAssetsByOwnerAlbumIds_EmptyAlbumIds_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteAssetsByOwnerAlbumIds_EmptyAlbumIds_ReturnFalse start");

    vector<int32_t> albumIds;
    bool result = MediaFileMonitorRdbUtils::DeleteAssetsByOwnerAlbumIds(g_rdbStore, albumIds,
        FileSourceType::MEDIA_HO_LAKE);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("DeleteAssetsByOwnerAlbumIds_EmptyAlbumIds_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteAssetsByOwnerAlbumIds_NullRdbStore_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteAssetsByOwnerAlbumIds_NullRdbStore_ReturnFalse start");

    std::shared_ptr<MediaLibraryRdbStore> nullRdbStore;
    vector<int32_t> albumIds = {TEST_ALBUM_ID_1};
    bool result = MediaFileMonitorRdbUtils::DeleteAssetsByOwnerAlbumIds(nullRdbStore, albumIds,
        FileSourceType::MEDIA_HO_LAKE);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("DeleteAssetsByOwnerAlbumIds_NullRdbStore_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteEmptyAlbumsByLPath_ValidData_ReturnSuccess, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteEmptyAlbumsByLPath_ValidData_ReturnSuccess start");

    string insertSql = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
        PhotoAlbumColumns::ALBUM_LPATH + ", " + PhotoAlbumColumns::ALBUM_COUNT + ") VALUES (" +
        to_string(TEST_ALBUM_ID_1) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'album', '" + TEST_LPATH + "', 0)";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    EXPECT_EQ(ret, E_OK);

    bool result = MediaFileMonitorRdbUtils::DeleteEmptyAlbumsByLPath(g_rdbStore, TEST_LPATH);

    EXPECT_TRUE(result);

    MEDIA_INFO_LOG("DeleteEmptyAlbumsByLPath_ValidData_ReturnSuccess end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteEmptyAlbumsByLPath_NonEmptyAlbum_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteEmptyAlbumsByLPath_NonEmptyAlbum_ReturnFalse start");

    string insertSql = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
        PhotoAlbumColumns::ALBUM_LPATH + ", " + PhotoAlbumColumns::ALBUM_COUNT + ") VALUES (" +
        to_string(TEST_ALBUM_ID_1) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'album', '" + TEST_LPATH + "', 5)";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    EXPECT_EQ(ret, E_OK);

    bool result = MediaFileMonitorRdbUtils::DeleteEmptyAlbumsByLPath(g_rdbStore, TEST_LPATH);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("DeleteEmptyAlbumsByLPath_NonEmptyAlbum_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteEmptyAlbumsByLPath_NullRdbStore_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteEmptyAlbumsByLPath_NullRdbStore_ReturnFalse start");

    std::shared_ptr<MediaLibraryRdbStore> nullRdbStore;
    bool result = MediaFileMonitorRdbUtils::DeleteEmptyAlbumsByLPath(nullRdbStore, TEST_LPATH);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("DeleteEmptyAlbumsByLPath_NullRdbStore_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, UpdateAlbumInfo_ValidAlbumId_ReturnSuccess, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateAlbumInfo_ValidAlbumId_ReturnSuccess start");

    bool result = MediaFileMonitorRdbUtils::UpdateAlbumInfo(g_rdbStore, {TEST_ALBUM_ID_1});

    EXPECT_TRUE(result);

    MEDIA_INFO_LOG("UpdateAlbumInfo_ValidAlbumId_ReturnSuccess end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, UpdateAlbumInfo_DefaultAlbumId_ReturnSuccess, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateAlbumInfo_DefaultAlbumId_ReturnSuccess start");

    bool result = MediaFileMonitorRdbUtils::UpdateAlbumInfo(g_rdbStore);

    EXPECT_TRUE(result);

    MEDIA_INFO_LOG("UpdateAlbumInfo_DefaultAlbumId_ReturnSuccess end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, UpdateAlbumInfo_NullRdbStore_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateAlbumInfo_NullRdbStore_ReturnFalse start");

    std::shared_ptr<MediaLibraryRdbStore> nullRdbStore;
    bool result = MediaFileMonitorRdbUtils::UpdateAlbumInfo(nullRdbStore, {TEST_ALBUM_ID_1});

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("UpdateAlbumInfo_NullRdbStore_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteLakeAlbums_ValidData_ReturnTrue, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteLakeAlbums_ValidData_ReturnTrue start");

    string insertAlbumSql = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" + PhotoAlbumColumns::ALBUM_ID + ", " +
        PhotoAlbumColumns::ALBUM_TYPE + ", " + PhotoAlbumColumns::ALBUM_SUBTYPE + ", " +
        PhotoAlbumColumns::ALBUM_NAME + ", " + PhotoAlbumColumns::ALBUM_LPATH + ", " +
        PhotoAlbumColumns::ALBUM_COUNT + ") VALUES (" + to_string(TEST_ALBUM_ID_1) + ", " +
        to_string(TEST_ALBUM_TYPE) + ", " + to_string(TEST_ALBUM_SUBTYPE) + ", 'album', '" +
        TEST_LPATH + "', 1)";
    int32_t retAlbum = g_rdbStore->ExecuteSql(insertAlbumSql);
    EXPECT_EQ(retAlbum, E_OK);

    string insertPhotoSql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + ", " + MediaColumn::MEDIA_FILE_PATH + ", " +
        PhotoColumn::PHOTO_FILE_SOURCE_TYPE + ", " + PhotoColumn::PHOTO_STORAGE_PATH +
        ") VALUES (?, ?, ?, ?, ?, ?)";
    std::vector<NativeRdb::ValueObject> photoArgs = {TEST_FILE_ID_1, TEST_ALBUM_ID_1,
        TEST_DATE_TAKEN, TEST_PHOTO_PATH,
        static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE), TEST_STORAGE_PATH};
    int32_t retPhoto = g_rdbStore->ExecuteSql(insertPhotoSql, photoArgs);
    EXPECT_EQ(retPhoto, E_OK);

    unordered_map<int32_t, int32_t> albumCounts;
    albumCounts[TEST_ALBUM_ID_1] = 1;
    vector<LakeMonitorQueryResultData> dataList;
    LakeMonitorQueryResultData data;
    data.fileId = TEST_FILE_ID_1;
    data.albumId = TEST_ALBUM_ID_1;
    data.dateTaken = TEST_DATE_TAKEN;
    data.photoPath = TEST_PHOTO_PATH;
    dataList.push_back(data);

    bool result = MediaFileMonitorRdbUtils::DeleteLakeAlbums(g_rdbStore, albumCounts, dataList);
    EXPECT_TRUE(result);

    MEDIA_INFO_LOG("DeleteLakeAlbums_ValidData_ReturnTrue end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteLakeAlbums_CountMismatch_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteLakeAlbums_CountMismatch_ReturnFalse start");

    string insertAlbumSql = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
        PhotoAlbumColumns::ALBUM_LPATH + ", " + PhotoAlbumColumns::ALBUM_COUNT + ") VALUES (" +
        to_string(TEST_ALBUM_ID_1) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'album', '" + TEST_LPATH + "', 5)";
    int32_t retAlbum = g_rdbStore->ExecuteSql(insertAlbumSql);
    EXPECT_EQ(retAlbum, E_OK);

    string insertPhotoSql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + ", " + MediaColumn::MEDIA_FILE_PATH + ") VALUES (" +
        to_string(TEST_FILE_ID_1) + ", " + to_string(TEST_ALBUM_ID_1) + ", " +
        to_string(TEST_DATE_TAKEN) + ", '" + TEST_PHOTO_PATH + "')";
    int32_t retPhoto = g_rdbStore->ExecuteSql(insertPhotoSql);
    EXPECT_EQ(retPhoto, E_OK);

    unordered_map<int32_t, int32_t> albumCounts;
    albumCounts[TEST_ALBUM_ID_1] = 5;
    vector<LakeMonitorQueryResultData> dataList;
    LakeMonitorQueryResultData data;
    data.fileId = TEST_FILE_ID_1;
    data.albumId = TEST_ALBUM_ID_1;
    data.dateTaken = TEST_DATE_TAKEN;
    data.photoPath = TEST_PHOTO_PATH;
    dataList.push_back(data);

    bool result = MediaFileMonitorRdbUtils::DeleteLakeAlbums(g_rdbStore, albumCounts, dataList);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("DeleteLakeAlbums_CountMismatch_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteLakeAlbums_NullRdbStore_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteLakeAlbums_NullRdbStore_ReturnFalse start");

    std::shared_ptr<MediaLibraryRdbStore> nullRdbStore;
    unordered_map<int32_t, int32_t> albumCounts;
    vector<LakeMonitorQueryResultData> dataList;
    bool result = MediaFileMonitorRdbUtils::DeleteLakeAlbums(nullRdbStore, albumCounts, dataList);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("DeleteLakeAlbums_NullRdbStore_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteLakeAlbums_EmptyAlbumCounts_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteLakeAlbums_EmptyAlbumCounts_ReturnFalse start");

    unordered_map<int32_t, int32_t> albumCounts;
    vector<LakeMonitorQueryResultData> dataList;
    bool result = MediaFileMonitorRdbUtils::DeleteLakeAlbums(g_rdbStore, albumCounts, dataList);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("DeleteLakeAlbums_EmptyAlbumCounts_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteLakeAlbums_EmptyDataList_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteLakeAlbums_EmptyDataList_ReturnFalse start");

    unordered_map<int32_t, int32_t> albumCounts;
    albumCounts[TEST_ALBUM_ID_1] = 1;
    vector<LakeMonitorQueryResultData> dataList;
    bool result = MediaFileMonitorRdbUtils::DeleteLakeAlbums(g_rdbStore, albumCounts, dataList);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("DeleteLakeAlbums_EmptyDataList_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteLakeAlbums_InvalidAlbumIdInData_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteLakeAlbums_InvalidAlbumIdInData_ReturnFalse start");

    unordered_map<int32_t, int32_t> albumCounts;
    albumCounts[TEST_ALBUM_ID_1] = 1;
    vector<LakeMonitorQueryResultData> dataList;
    LakeMonitorQueryResultData data;
    data.fileId = TEST_FILE_ID_1;
    data.albumId = -1;
    data.dateTaken = TEST_DATE_TAKEN;
    data.photoPath = TEST_PHOTO_PATH;
    dataList.push_back(data);

    bool result = MediaFileMonitorRdbUtils::DeleteLakeAlbums(g_rdbStore, albumCounts, dataList);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("DeleteLakeAlbums_InvalidAlbumIdInData_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteLakeAlbums_MultipleMatchingAlbums_ReturnTrue, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteLakeAlbums_MultipleMatchingAlbums_ReturnTrue start");

    string insertAlbumSql1 = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
        PhotoAlbumColumns::ALBUM_LPATH + ", " + PhotoAlbumColumns::ALBUM_COUNT + ") VALUES (" +
        to_string(TEST_ALBUM_ID_2) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'album', '" + TEST_LPATH + "', 1)";
    string insertAlbumSql2 = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
        PhotoAlbumColumns::ALBUM_LPATH + ", " + PhotoAlbumColumns::ALBUM_COUNT + ") VALUES (" +
        to_string(TEST_ALBUM_ID_3) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'sub', '" + TEST_LPATH_SUB + "', 1)";
    EXPECT_EQ(g_rdbStore->ExecuteSql(insertAlbumSql1), E_OK);
    EXPECT_EQ(g_rdbStore->ExecuteSql(insertAlbumSql2), E_OK);

    string insertPhotoSql1 = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + ", " + MediaColumn::MEDIA_FILE_PATH + ") VALUES (" +
        to_string(TEST_FILE_ID_1) + ", " + to_string(TEST_ALBUM_ID_2) + ", " +
        to_string(TEST_DATE_TAKEN) + ", '" + TEST_PHOTO_PATH + "')";
    string insertPhotoSql2 = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + ", " + MediaColumn::MEDIA_FILE_PATH + ") VALUES (" +
        to_string(TEST_FILE_ID_2) + ", " + to_string(TEST_ALBUM_ID_3) + ", " +
        to_string(TEST_DATE_TAKEN) + ", '/testMultipleMatchingAlbums.jpg')";
    EXPECT_EQ(g_rdbStore->ExecuteSql(insertPhotoSql1), E_OK);
    EXPECT_EQ(g_rdbStore->ExecuteSql(insertPhotoSql2), E_OK);

    unordered_map<int32_t, int32_t> albumCounts;
    albumCounts[TEST_ALBUM_ID_2] = 1;
    albumCounts[TEST_ALBUM_ID_3] = 1;
    vector<LakeMonitorQueryResultData> dataList;
    LakeMonitorQueryResultData data1 = {TEST_FILE_ID_1, TEST_ALBUM_ID_2, TEST_DATE_TAKEN, TEST_PHOTO_PATH};
    dataList.push_back(data1);
    LakeMonitorQueryResultData data2 = {TEST_FILE_ID_2, TEST_ALBUM_ID_3, TEST_DATE_TAKEN, "/test3.jpg"};
    dataList.push_back(data2);

    bool result = MediaFileMonitorRdbUtils::DeleteLakeAlbums(g_rdbStore, albumCounts, dataList);

    EXPECT_TRUE(result);

    MEDIA_INFO_LOG("DeleteLakeAlbums_MultipleMatchingAlbums_ReturnTrue end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteLakeDirByLakePath_InvalidPath_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteLakeDirByLakePath_InvalidPath_ReturnFalse start");

    string invalidPath = "/invalid/path";
    bool result = MediaFileMonitorRdbUtils::DeleteLakeDirByLakePath(invalidPath, g_rdbStore);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("DeleteLakeDirByLakePath_InvalidPath_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteLakeDirByLakePath_NullRdbStore_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteLakeDirByLakePath_NullRdbStore_ReturnFalse start");

    std::shared_ptr<MediaLibraryRdbStore> nullRdbStore;
    bool result = MediaFileMonitorRdbUtils::DeleteLakeDirByLakePath(TEST_HO_PATH, nullRdbStore);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("DeleteLakeDirByLakePath_NullRdbStore_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteDirByLakePath_ValidData_ReturnSuccess, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteDirByLakePath_ValidData_ReturnSuccess start");

    string insertAlbumSql = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
        PhotoAlbumColumns::ALBUM_LPATH + ") VALUES (" +
        to_string(TEST_ALBUM_ID_1) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'album', '" + TEST_LPATH + "')";
    int32_t retAlbum = g_rdbStore->ExecuteSql(insertAlbumSql);
    EXPECT_EQ(retAlbum, E_OK);

    string insertPhotoSql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + ", " + MediaColumn::MEDIA_FILE_PATH + ") VALUES (" +
        to_string(TEST_FILE_ID_1) + ", " + to_string(TEST_ALBUM_ID_1) + ", " +
        to_string(TEST_DATE_TAKEN) + ", '" + TEST_PHOTO_PATH + "')";
    int32_t retPhoto = g_rdbStore->ExecuteSql(insertPhotoSql);
    EXPECT_EQ(retPhoto, E_OK);

    int32_t delNum = 0;
    bool result = MediaFileMonitorRdbUtils::DeleteLakeDirByLakePath(TEST_HO_PATH, g_rdbStore, &delNum);

    EXPECT_TRUE(result);

    MEDIA_INFO_LOG("DeleteDirByLakePath_ValidData_ReturnSuccess end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteDirByLakePath_NullDelNum_ReturnSuccess, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteDirByLakePath_NullDelNum_ReturnSuccess start");

    string insertAlbumSql = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
        PhotoAlbumColumns::ALBUM_LPATH + ") VALUES (" +
        to_string(TEST_ALBUM_ID_2) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'album', '" + TEST_LPATH + "')";
    EXPECT_EQ(g_rdbStore->ExecuteSql(insertAlbumSql), E_OK);

    string insertPhotoSql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + ", " + MediaColumn::MEDIA_FILE_PATH + ") VALUES (" +
        to_string(TEST_FILE_ID_1) + ", " + to_string(TEST_ALBUM_ID_2) + ", " +
        to_string(TEST_DATE_TAKEN) + ", '" + TEST_PHOTO_PATH + "')";
    int32_t retPhoto = g_rdbStore->ExecuteSql(insertPhotoSql);
    EXPECT_EQ(retPhoto, E_OK);

    bool result = MediaFileMonitorRdbUtils::DeleteLakeDirByLakePath(TEST_HO_PATH, g_rdbStore, nullptr);

    EXPECT_TRUE(result);

    MEDIA_INFO_LOG("DeleteDirByLakePath_NullDelNum_ReturnSuccess end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteAssetByStoragePath_NullAssetRefresh_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteAssetByStoragePath_NullAssetRefresh_ReturnFalse start");

    bool result = MediaFileMonitorRdbUtils::DeleteAssetByStoragePath(nullptr, TEST_STORAGE_PATH);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("DeleteAssetByStoragePath_NullAssetRefresh_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, CheckValidData_ValidData_ReturnTrue, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckValidData_ValidData_ReturnTrue start");

    LakeMonitorQueryResultData data;
    data.fileId = TEST_FILE_ID_1;
    data.albumId = TEST_ALBUM_ID_1;
    data.dateTaken = TEST_DATE_TAKEN;
    data.photoPath = TEST_PHOTO_PATH;
    bool result = MediaFileMonitorRdbUtils::CheckValidData(data);

    EXPECT_TRUE(result);

    MEDIA_INFO_LOG("CheckValidData_ValidData_ReturnTrue end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, CheckValidData_InvalidFileId_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckValidData_InvalidFileId_ReturnFalse start");

    LakeMonitorQueryResultData data;
    data.fileId = -1;
    data.albumId = TEST_ALBUM_ID_1;
    data.dateTaken = TEST_DATE_TAKEN;
    data.photoPath = TEST_PHOTO_PATH;
    bool result = MediaFileMonitorRdbUtils::CheckValidData(data);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("CheckValidData_InvalidFileId_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, CheckValidData_InvalidAlbumId_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckValidData_InvalidAlbumId_ReturnFalse start");

    LakeMonitorQueryResultData data;
    data.fileId = TEST_FILE_ID_1;
    data.albumId = -1;
    data.dateTaken = TEST_DATE_TAKEN;
    data.photoPath = TEST_PHOTO_PATH;
    bool result = MediaFileMonitorRdbUtils::CheckValidData(data);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("CheckValidData_InvalidAlbumId_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, CheckValidData_InvalidDateTaken_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckValidData_InvalidDateTaken_ReturnFalse start");

    LakeMonitorQueryResultData data;
    data.fileId = TEST_FILE_ID_1;
    data.albumId = TEST_ALBUM_ID_1;
    data.dateTaken = -1;
    data.photoPath = TEST_PHOTO_PATH;
    bool result = MediaFileMonitorRdbUtils::CheckValidData(data);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("CheckValidData_InvalidDateTaken_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, CheckValidData_EmptyPhotoPath_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckValidData_EmptyPhotoPath_ReturnFalse start");

    LakeMonitorQueryResultData data;
    data.fileId = TEST_FILE_ID_1;
    data.albumId = TEST_ALBUM_ID_1;
    data.dateTaken = TEST_DATE_TAKEN;
    data.photoPath = "";
    bool result = MediaFileMonitorRdbUtils::CheckValidData(data);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("CheckValidData_EmptyPhotoPath_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, FillQueryResultData_ValidResultSet_ReturnTrue, TestSize.Level1)
{
    MEDIA_INFO_LOG("FillQueryResultData_ValidResultSet_ReturnTrue start");

    string insertSql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + ", " + MediaColumn::MEDIA_FILE_PATH + ") VALUES (" +
        to_string(TEST_FILE_ID_1) + ", " + to_string(TEST_ALBUM_ID_1) + ", " +
        to_string(TEST_DATE_TAKEN) + ", '" + TEST_PHOTO_PATH + "')";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    EXPECT_EQ(ret, E_OK);

    string querySql = "SELECT " + MediaColumn::MEDIA_ID + ", " +
        PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " + MediaColumn::MEDIA_DATE_TAKEN + ", " +
        MediaColumn::MEDIA_FILE_PATH + " FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " +
        MediaColumn::MEDIA_ID + " = " + to_string(TEST_FILE_ID_1);
    auto resultSet = g_rdbStore->QuerySql(querySql, vector<string>());
    ASSERT_NE(resultSet, nullptr);

    int rowCount = 0;
    resultSet->GetRowCount(rowCount);
    EXPECT_GT(rowCount, 0);

    EXPECT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);
    LakeMonitorQueryResultData data;
    bool result = MediaFileMonitorRdbUtils::FillQueryResultData(resultSet, data);
    EXPECT_TRUE(result);
    EXPECT_EQ(data.fileId, TEST_FILE_ID_1);
    EXPECT_EQ(data.albumId, TEST_ALBUM_ID_1);
    EXPECT_EQ(data.dateTaken, TEST_DATE_TAKEN);
    EXPECT_EQ(data.photoPath, TEST_PHOTO_PATH);

    MEDIA_INFO_LOG("FillQueryResultData_ValidResultSet_ReturnTrue end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, BuildDeletePredicatesByStoragePath_ValidPath_ReturnPredicates, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildDeletePredicatesByStoragePath_ValidPath_ReturnPredicates start");

    RdbPredicates predicates = MediaFileMonitorRdbUtils::BuildDeletePredicatesByStoragePath(TEST_STORAGE_PATH);
    EXPECT_FALSE(predicates.GetWhereClause().empty());

    MEDIA_INFO_LOG("BuildDeletePredicatesByStoragePath_ValidPath_ReturnPredicates end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, BuildQueryPredicatesByAlbumIds_ValidAlbumIds_ReturnPredicates, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildQueryPredicatesByAlbumIds_ValidAlbumIds_ReturnPredicates start");

    vector<int32_t> albumIds = {TEST_ALBUM_ID_1, TEST_ALBUM_ID_2};
    RdbPredicates predicates =
        MediaFileMonitorRdbUtils::BuildQueryPredicatesByAlbumIds(albumIds, FileSourceType::MEDIA_HO_LAKE);
    EXPECT_FALSE(predicates.GetWhereClause().empty());

    MEDIA_INFO_LOG("BuildQueryPredicatesByAlbumIds_ValidAlbumIds_ReturnPredicates end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryDataByDeletedStoragePath_MultipleRecords_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryDataByDeletedStoragePath_MultipleRecords_ReturnFalse start");

    string insertSql1 = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + ", " + MediaColumn::MEDIA_FILE_PATH + ", " +
        PhotoColumn::PHOTO_STORAGE_PATH + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
        MediaColumn::MEDIA_HIDDEN + ") VALUES (" +
        to_string(TEST_FILE_ID_1) + ", " + to_string(TEST_ALBUM_ID_1) + ", " +
        to_string(TEST_DATE_TAKEN) + ", '" + TEST_PHOTO_PATH + "', '" +
        TEST_STORAGE_PATH + "', 0, 0)";
    string insertSql2 = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + ", " + MediaColumn::MEDIA_FILE_PATH + ", " +
        PhotoColumn::PHOTO_STORAGE_PATH + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
        MediaColumn::MEDIA_HIDDEN + ") VALUES (" +
        to_string(TEST_FILE_ID_2) + ", " + to_string(TEST_ALBUM_ID_2) + ", " +
        to_string(TEST_DATE_TAKEN) + ", '/test2.jpg', '" +
        TEST_STORAGE_PATH + "', 0, 0)";
    int32_t ret1 = g_rdbStore->ExecuteSql(insertSql1);
    int32_t ret2 = g_rdbStore->ExecuteSql(insertSql2);
    EXPECT_EQ(ret1, E_OK);
    EXPECT_EQ(ret2, E_OK);

    LakeMonitorQueryResultData data;
    bool result = MediaFileMonitorRdbUtils::QueryDataByDeletedStoragePath(g_rdbStore, TEST_STORAGE_PATH, data);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("QueryDataByDeletedStoragePath_MultipleRecords_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryDataListByAlbumIds_NoMatchingRecords_ReturnSuccess, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryDataListByAlbumIds_NoMatchingRecords_ReturnSuccess start");

    vector<int32_t> albumIds = {TEST_ALBUM_ID_1};
    vector<LakeMonitorQueryResultData> dataList;
    bool result = MediaFileMonitorRdbUtils::QueryDataListByAlbumIds(g_rdbStore, albumIds, dataList,
        FileSourceType::MEDIA_HO_LAKE);

    EXPECT_TRUE(result);
    EXPECT_EQ(dataList.size(), 0);

    MEDIA_INFO_LOG("QueryDataListByAlbumIds_NoMatchingRecords_ReturnSuccess end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteEmptyAlbumsByLPath_MultipleEmptyAlbums_ReturnSuccess, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteEmptyAlbumsByLPath_MultipleEmptyAlbums_ReturnSuccess start");

    string insertSql1 = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
        PhotoAlbumColumns::ALBUM_LPATH + ", " + PhotoAlbumColumns::ALBUM_COUNT + ") VALUES (" +
        to_string(TEST_ALBUM_ID_1) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'album', '" + TEST_LPATH + "', 0)";
    string insertSql2 = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
        PhotoAlbumColumns::ALBUM_LPATH + ", " + PhotoAlbumColumns::ALBUM_COUNT + ") VALUES (" +
        to_string(TEST_ALBUM_ID_2) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'sub', '" + TEST_LPATH_SUB + "', 0)";
    int32_t ret1 = g_rdbStore->ExecuteSql(insertSql1);
    int32_t ret2 = g_rdbStore->ExecuteSql(insertSql2);
    EXPECT_EQ(ret1, E_OK);
    EXPECT_EQ(ret2, E_OK);

    bool result = MediaFileMonitorRdbUtils::DeleteEmptyAlbumsByLPath(g_rdbStore, TEST_LPATH);

    EXPECT_TRUE(result);

    MEDIA_INFO_LOG("DeleteEmptyAlbumsByLPath_MultipleEmptyAlbums_ReturnSuccess end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryAlbumIdsByLPath_NestedDirectories_ReturnSuccess, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryAlbumIdsByLPath_NestedDirectories_ReturnSuccess start");

    string insertSql1 = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
        PhotoAlbumColumns::ALBUM_LPATH + ") VALUES (" +
        to_string(TEST_ALBUM_ID_1) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'album', '" +
        TEST_LPATH + "')";
    string insertSql2 = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
        PhotoAlbumColumns::ALBUM_LPATH + ") VALUES (" +
        to_string(TEST_ALBUM_ID_2) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'sub', '" +
        TEST_LPATH_SUB + "')";
    string insertSql3 = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
        PhotoAlbumColumns::ALBUM_LPATH + ") VALUES (" +
        to_string(TEST_ALBUM_ID_3) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'deep', '" +
        TEST_LPATH + "/sub/deep')";
    int32_t ret1 = g_rdbStore->ExecuteSql(insertSql1);
    int32_t ret2 = g_rdbStore->ExecuteSql(insertSql2);
    int32_t ret3 = g_rdbStore->ExecuteSql(insertSql3);
    EXPECT_EQ(ret1, E_OK);
    EXPECT_EQ(ret2, E_OK);
    EXPECT_EQ(ret3, E_OK);

    vector<int32_t> albumIds;
    bool result = MediaFileMonitorRdbUtils::QueryAlbumIdsByLPath(g_rdbStore, TEST_LPATH, albumIds);

    EXPECT_TRUE(result);
    EXPECT_EQ(albumIds.size(), 3);

    MEDIA_INFO_LOG("QueryAlbumIdsByLPath_NestedDirectories_ReturnSuccess end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteAssetsByOwnerAlbumIds_NoAssets_ReturnSuccess, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteAssetsByOwnerAlbumIds_NoAssets_ReturnSuccess start");

    string insertAlbumSql = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ") VALUES (" +
        to_string(TEST_ALBUM_ID_1) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'test')";
    int32_t retAlbum = g_rdbStore->ExecuteSql(insertAlbumSql);
    EXPECT_EQ(retAlbum, E_OK);

    vector<int32_t> albumIds = {TEST_ALBUM_ID_1};
    bool result = MediaFileMonitorRdbUtils::DeleteAssetsByOwnerAlbumIds(g_rdbStore, albumIds,
        FileSourceType::MEDIA_HO_LAKE);

    EXPECT_TRUE(result);

    MEDIA_INFO_LOG("DeleteAssetsByOwnerAlbumIds_NoAssets_ReturnSuccess end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteLakeAlbums_PartialMatch_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteLakeAlbums_PartialMatch_ReturnFalse start");

    string insertAlbumSql = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
        PhotoAlbumColumns::ALBUM_LPATH + ", " + PhotoAlbumColumns::ALBUM_COUNT + ") VALUES (" +
        to_string(TEST_ALBUM_ID_1) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'album', '" + TEST_LPATH + "', 2)";
    int32_t retAlbum = g_rdbStore->ExecuteSql(insertAlbumSql);
    EXPECT_EQ(retAlbum, E_OK);

    string insertPhotoSql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + ", " + MediaColumn::MEDIA_FILE_PATH + ") VALUES (" +
        to_string(TEST_FILE_ID_1) + ", " + to_string(TEST_ALBUM_ID_1) + ", " +
        to_string(TEST_DATE_TAKEN) + ", '" + TEST_PHOTO_PATH + "')";
    EXPECT_EQ(g_rdbStore->ExecuteSql(insertPhotoSql), E_OK);

    unordered_map<int32_t, int32_t> albumCounts;
    albumCounts[TEST_ALBUM_ID_1] = 2;
    vector<LakeMonitorQueryResultData> dataList;
    LakeMonitorQueryResultData data;
    data.fileId = TEST_FILE_ID_1;
    data.albumId = TEST_ALBUM_ID_1;
    data.dateTaken = TEST_DATE_TAKEN;
    data.photoPath = TEST_PHOTO_PATH;
    dataList.push_back(data);

    bool result = MediaFileMonitorRdbUtils::DeleteLakeAlbums(g_rdbStore, albumCounts, dataList);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("DeleteLakeAlbums_PartialMatch_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryDataByDeletedStoragePath_NonExistentPath_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryDataByDeletedStoragePath_NonExistentPath_ReturnFalse start");

    LakeMonitorQueryResultData data;
    bool result = MediaFileMonitorRdbUtils::QueryDataByDeletedStoragePath(g_rdbStore, "/nonexistent/path.jpg", data);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("QueryDataByDeletedStoragePath_NonExistentPath_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryAlbumByLPath_NonExistentLPath_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryAlbumByLPath_NonExistentLPath_ReturnFalse start");

    vector<int32_t> albumIds;
    unordered_map<int32_t, int32_t> albumCounts;
    bool result = MediaFileMonitorRdbUtils::QueryAlbumByLPath(g_rdbStore, "/nonexistent/path", albumIds, albumCounts);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("QueryAlbumByLPath_NonExistentLPath_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteEmptyAlbumsByLPath_NonExistentLPath_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteEmptyAlbumsByLPath_NonExistentLPath_ReturnFalse start");

    bool result = MediaFileMonitorRdbUtils::DeleteEmptyAlbumsByLPath(g_rdbStore, "/nonexistent/path");

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("DeleteEmptyAlbumsByLPath_NonExistentLPath_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryDataListByAlbumIds_LakeFileSourceType_ReturnSuccess, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryDataListByAlbumIds_LakeFileSourceType_ReturnSuccess start");

    string insertAlbumSql = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ") VALUES (" +
        to_string(TEST_ALBUM_ID_1) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'test')";
    int32_t retAlbum = g_rdbStore->ExecuteSql(insertAlbumSql);
    EXPECT_EQ(retAlbum, E_OK);

    string insertPhotoSql1 = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + ", " + MediaColumn::MEDIA_FILE_PATH + ", " +
        PhotoColumn::PHOTO_FILE_SOURCE_TYPE + ") VALUES (" +
        to_string(TEST_FILE_ID_1) + ", " + to_string(TEST_ALBUM_ID_1) + ", " +
        to_string(TEST_DATE_TAKEN) + ", '" + TEST_PHOTO_PATH + "', 3)";
    string insertPhotoSql2 = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + ", " + MediaColumn::MEDIA_FILE_PATH + ", " +
        PhotoColumn::PHOTO_FILE_SOURCE_TYPE + ") VALUES (" +
        to_string(TEST_FILE_ID_2) + ", " + to_string(TEST_ALBUM_ID_1) + ", " +
        to_string(TEST_DATE_TAKEN) + ", '/testLakeFileSourceType.jpg', 0)";
    EXPECT_EQ(g_rdbStore->ExecuteSql(insertPhotoSql1), E_OK);
    EXPECT_EQ(g_rdbStore->ExecuteSql(insertPhotoSql2), E_OK);

    vector<int32_t> albumIds = {TEST_ALBUM_ID_1};
    vector<LakeMonitorQueryResultData> dataList;
    bool result = MediaFileMonitorRdbUtils::QueryDataListByAlbumIds(g_rdbStore, albumIds, dataList,
        FileSourceType::MEDIA_HO_LAKE);

    EXPECT_TRUE(result);
    EXPECT_EQ(dataList.size(), 1);

    MEDIA_INFO_LOG("QueryDataListByAlbumIds_LakeFileSourceType_ReturnSuccess end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteLakeAlbums_AlbumNotInCounts_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteLakeAlbums_AlbumNotInCounts_ReturnFalse start");

    string insertPhotoSql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + ", " + MediaColumn::MEDIA_FILE_PATH + ") VALUES (" +
        to_string(TEST_FILE_ID_1) + ", " + to_string(TEST_ALBUM_ID_1) + ", " +
        to_string(TEST_DATE_TAKEN) + ", '" + TEST_PHOTO_PATH + "')";
    int32_t retPhoto = g_rdbStore->ExecuteSql(insertPhotoSql);
    EXPECT_EQ(retPhoto, E_OK);

    unordered_map<int32_t, int32_t> albumCounts;
    albumCounts[TEST_ALBUM_ID_2] = 1;
    vector<LakeMonitorQueryResultData> dataList;
    LakeMonitorQueryResultData data;
    data.fileId = TEST_FILE_ID_1;
    data.albumId = TEST_ALBUM_ID_1;
    data.dateTaken = TEST_DATE_TAKEN;
    data.photoPath = TEST_PHOTO_PATH;
    dataList.push_back(data);

    bool result = MediaFileMonitorRdbUtils::DeleteLakeAlbums(g_rdbStore, albumCounts, dataList);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("DeleteLakeAlbums_AlbumNotInCounts_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteLakeAlbums_ZeroCountInData_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteLakeAlbums_ZeroCountInData_ReturnFalse start");

    string insertAlbumSql = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
        PhotoAlbumColumns::ALBUM_LPATH + ", " + PhotoAlbumColumns::ALBUM_COUNT + ") VALUES (" +
        to_string(TEST_ALBUM_ID_1) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'album', '" + TEST_LPATH + "', 0)";
    int32_t retAlbum = g_rdbStore->ExecuteSql(insertAlbumSql);
    EXPECT_EQ(retAlbum, E_OK);

    unordered_map<int32_t, int32_t> albumCounts;
    albumCounts[TEST_ALBUM_ID_1] = 0;
    vector<LakeMonitorQueryResultData> dataList;
    bool result = MediaFileMonitorRdbUtils::DeleteLakeAlbums(g_rdbStore, albumCounts,
        dataList);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("DeleteLakeAlbums_ZeroCountInData_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteDirByLakePath_ValidDataAndDelNum_ReturnSuccess, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteDirByLakePath_ValidDataAndDelNum_ReturnSuccess start");

    string insertAlbumSql = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
        PhotoAlbumColumns::ALBUM_LPATH + ") VALUES (" +
        to_string(TEST_ALBUM_ID_2) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'album', '" + TEST_LPATH + "')";
    int32_t retAlbum = g_rdbStore->ExecuteSql(insertAlbumSql);
    EXPECT_EQ(retAlbum, E_OK);

    string insertPhotoSql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + ", " + MediaColumn::MEDIA_FILE_PATH + ", " +
        PhotoColumn::PHOTO_FILE_SOURCE_TYPE + ", " + PhotoColumn::PHOTO_STORAGE_PATH +
        ") VALUES (?, ?, ?, ?, ?, ?)";
    std::vector<NativeRdb::ValueObject> photoArgs = {TEST_FILE_ID_1, TEST_ALBUM_ID_2,
        TEST_DATE_TAKEN, TEST_PHOTO_PATH,
        static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE), TEST_STORAGE_PATH};
    int32_t retPhoto = g_rdbStore->ExecuteSql(insertPhotoSql, photoArgs);
    EXPECT_EQ(retPhoto, E_OK);

    int32_t delNum = 0;
    bool result = MediaFileMonitorRdbUtils::DeleteLakeDirByLakePath(TEST_HO_PATH, g_rdbStore, &delNum);

    EXPECT_TRUE(result);
    EXPECT_GT(delNum, 0);

    MEDIA_INFO_LOG("DeleteDirByLakePath_ValidDataAndDelNum_ReturnSuccess end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, DeleteLakeDirByLakePath_EmptyAlbums_ReturnFalse, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteLakeDirByLakePath_EmptyAlbums_ReturnFalse start");

    bool result = MediaFileMonitorRdbUtils::DeleteLakeDirByLakePath(TEST_HO_PATH, g_rdbStore);

    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("DeleteLakeDirByLakePath_EmptyAlbums_ReturnFalse end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, UpdateAlbumInfo_ValidRdbStore_ReturnSuccess, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateAlbumInfo_ValidRdbStore_ReturnSuccess start");

    bool result = MediaFileMonitorRdbUtils::UpdateAlbumInfo(g_rdbStore);

    EXPECT_TRUE(result);

    MEDIA_INFO_LOG("UpdateAlbumInfo_ValidRdbStore_ReturnSuccess end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, ColumnValueParserInt32_ParseValue_ValidData_ReturnOk, TestSize.Level1)
{
    MEDIA_INFO_LOG("ColumnValueParserInt32_ParseValue_ValidData_ReturnOk start");

    string insertSql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + ") VALUES (" +
        to_string(TEST_FILE_ID_1) + ", " + to_string(TEST_ALBUM_ID_1) + ")";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    EXPECT_EQ(ret, E_OK);

    string querySql = "SELECT " + MediaColumn::MEDIA_ID + " FROM " +
        PhotoColumn::PHOTOS_TABLE + " WHERE " + MediaColumn::MEDIA_ID + " = " +
        to_string(TEST_FILE_ID_1);
    auto resultSet = g_rdbStore->QuerySql(querySql, vector<string>());
    ASSERT_NE(resultSet, nullptr);

    EXPECT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);
    int32_t value = 0;
    int result = ColumnValueParser<int32_t>::ParseValue(*resultSet, 0, value);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(value, TEST_FILE_ID_1);

    MEDIA_INFO_LOG("ColumnValueParserInt32_ParseValue_ValidData_ReturnOk end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, ColumnValueParserInt64_ParseValue_ValidData_ReturnOk, TestSize.Level1)
{
    MEDIA_INFO_LOG("ColumnValueParserInt64_ParseValue_ValidData_ReturnOk start");

    string insertSql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + MediaColumn::MEDIA_DATE_TAKEN + ") VALUES (" +
        to_string(TEST_FILE_ID_1) + ", " + to_string(TEST_DATE_TAKEN) + ")";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    EXPECT_EQ(ret, E_OK);

    string querySql = "SELECT " + MediaColumn::MEDIA_DATE_TAKEN + " FROM " +
        PhotoColumn::PHOTOS_TABLE + " WHERE " + MediaColumn::MEDIA_ID + " = " +
        to_string(TEST_FILE_ID_1);
    auto resultSet = g_rdbStore->QuerySql(querySql, vector<string>());
    ASSERT_NE(resultSet, nullptr);

    EXPECT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);
    int64_t value = 0;
    int result = ColumnValueParser<int64_t>::ParseValue(*resultSet, 0, value);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(value, TEST_DATE_TAKEN);

    MEDIA_INFO_LOG("ColumnValueParserInt64_ParseValue_ValidData_ReturnOk end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, ColumnValueParserString_ParseValue_ValidData_ReturnOk, TestSize.Level1)
{
    MEDIA_INFO_LOG("ColumnValueParserString_ParseValue_ValidData_ReturnOk start");

    string insertSql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + MediaColumn::MEDIA_FILE_PATH + ") VALUES (" +
        to_string(TEST_FILE_ID_1) + ", '" + TEST_PHOTO_PATH + "')";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    EXPECT_EQ(ret, E_OK);

    string querySql = "SELECT " + MediaColumn::MEDIA_FILE_PATH + " FROM " +
        PhotoColumn::PHOTOS_TABLE + " WHERE " + MediaColumn::MEDIA_ID + " = " +
        to_string(TEST_FILE_ID_1);
    auto resultSet = g_rdbStore->QuerySql(querySql, vector<string>());
    ASSERT_NE(resultSet, nullptr);

    EXPECT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);
    string value = "";
    int result = ColumnValueParser<string>::ParseValue(*resultSet, 0, value);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(value, TEST_PHOTO_PATH);

    MEDIA_INFO_LOG("ColumnValueParserString_ParseValue_ValidData_ReturnOk end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, GetColumnValueInt32_ValidResultSet_ReturnValue, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetColumnValueInt32_ValidResultSet_ReturnValue start");

    string insertSql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ") VALUES (" + to_string(TEST_FILE_ID_1) + ")";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    EXPECT_EQ(ret, E_OK);

    string querySql = "SELECT " + MediaColumn::MEDIA_ID + " FROM " +
        PhotoColumn::PHOTOS_TABLE + " WHERE " + MediaColumn::MEDIA_ID + " = " +
        to_string(TEST_FILE_ID_1);
    auto resultSet = g_rdbStore->QuerySql(querySql, vector<string>());
    ASSERT_NE(resultSet, nullptr);

    EXPECT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);
    int32_t value = MediaFileMonitorRdbUtils::GetColumnValue<int32_t>(resultSet, MediaColumn::MEDIA_ID, -1);
    EXPECT_EQ(value, TEST_FILE_ID_1);

    MEDIA_INFO_LOG("GetColumnValueInt32_ValidResultSet_ReturnValue end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, GetColumnValueInt32_NullResultSet_ReturnDefaultValue, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetColumnValueInt32_NullResultSet_ReturnDefaultValue start");

    shared_ptr<ResultSet> resultSet = nullptr;
    int32_t defaultValue = -1;
    int32_t value = MediaFileMonitorRdbUtils::GetColumnValue<int32_t>(resultSet, MediaColumn::MEDIA_ID, defaultValue);
    EXPECT_EQ(value, defaultValue);

    MEDIA_INFO_LOG("GetColumnValueInt32_NullResultSet_ReturnDefaultValue end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, GetColumnValueInt64_ValidResultSet_ReturnValue, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetColumnValueInt64_ValidResultSet_ReturnValue start");

    string insertSql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + MediaColumn::MEDIA_DATE_TAKEN + ") VALUES (" +
        to_string(TEST_FILE_ID_2) + ", " + to_string(TEST_DATE_TAKEN) + ")";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    EXPECT_EQ(ret, E_OK);

    string querySql = "SELECT " + MediaColumn::MEDIA_DATE_TAKEN + " FROM " +
        PhotoColumn::PHOTOS_TABLE + " WHERE " + MediaColumn::MEDIA_ID + " = " +
        to_string(TEST_FILE_ID_2);
    auto resultSet = g_rdbStore->QuerySql(querySql, vector<string>());
    ASSERT_NE(resultSet, nullptr);

    EXPECT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);
    int64_t value = MediaFileMonitorRdbUtils::GetColumnValue<int64_t>(resultSet, MediaColumn::MEDIA_DATE_TAKEN, -1);
    EXPECT_EQ(value, TEST_DATE_TAKEN);

    MEDIA_INFO_LOG("GetColumnValueInt64_ValidResultSet_ReturnValue end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, GetColumnValueString_ValidResultSet_ReturnValue, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetColumnValueString_ValidResultSet_ReturnValue start");

    string insertSql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + " (" +
        MediaColumn::MEDIA_ID + ", " + MediaColumn::MEDIA_FILE_PATH + ") VALUES (" +
        to_string(TEST_FILE_ID_1) + ", '" + TEST_PHOTO_PATH + "')";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    EXPECT_EQ(ret, E_OK);

    string querySql = "SELECT " + MediaColumn::MEDIA_FILE_PATH + " FROM " +
        PhotoColumn::PHOTOS_TABLE + " WHERE " + MediaColumn::MEDIA_ID + " = " +
        to_string(TEST_FILE_ID_1);
    auto resultSet = g_rdbStore->QuerySql(querySql, vector<string>());
    ASSERT_NE(resultSet, nullptr);

    EXPECT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);
    string value = MediaFileMonitorRdbUtils::GetColumnValue<string>(resultSet, MediaColumn::MEDIA_FILE_PATH, "");
    EXPECT_EQ(value, TEST_PHOTO_PATH);

    MEDIA_INFO_LOG("GetColumnValueString_ValidResultSet_ReturnValue end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryAlbumIdsByLPath_ExcludePaths_ReturnSuccess, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryAlbumIdsByLPath_ExcludePaths_ReturnSuccess start");

    string insertSql = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
        PhotoAlbumColumns::ALBUM_LPATH + ") VALUES (" +
        to_string(TEST_ALBUM_ID_1) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'test', '" +
        TEST_LPATH + "')";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    EXPECT_EQ(ret, E_OK);

    vector<int32_t> albumIds;
    bool result = MediaFileMonitorRdbUtils::QueryAlbumIdsByLPath(g_rdbStore, "/Pictures/Screenrecords", albumIds);
    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("QueryAlbumIdsByLPath_ExcludePaths_ReturnSuccess end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, BuildQueryPredicatesByAlbumIds_SingleAlbumId_ReturnPredicates, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildQueryPredicatesByAlbumIds_SingleAlbumId_ReturnPredicates start");

    vector<int32_t> albumIds = {TEST_ALBUM_ID_1};
    RdbPredicates predicates =
        MediaFileMonitorRdbUtils::BuildQueryPredicatesByAlbumIds(albumIds, FileSourceType::MEDIA_HO_LAKE);
    EXPECT_FALSE(predicates.GetWhereClause().empty());

    MEDIA_INFO_LOG("BuildQueryPredicatesByAlbumIds_SingleAlbumId_ReturnPredicates end");
}

HWTEST_F(MediaLakeMonitorRdbUtilsTest, QueryAlbumByLPath_ExcludePaths_ReturnSuccess, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryAlbumByLPath_ExcludePaths_ReturnSuccess start");

    string insertSql = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" +
        PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
        PhotoAlbumColumns::ALBUM_LPATH + ", " + PhotoAlbumColumns::ALBUM_COUNT + ") VALUES (" +
        to_string(TEST_ALBUM_ID_1) + ", " + to_string(TEST_ALBUM_TYPE) + ", " +
        to_string(TEST_ALBUM_SUBTYPE) + ", 'test', '" + TEST_LPATH + "', 0)";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    EXPECT_EQ(ret, E_OK);

    vector<int32_t> albumIds;
    unordered_map<int32_t, int32_t> albumCounts;
    bool result = MediaFileMonitorRdbUtils::QueryAlbumByLPath(g_rdbStore,
        "/Pictures/Screenrecords", albumIds, albumCounts);
    EXPECT_FALSE(result);

    MEDIA_INFO_LOG("QueryAlbumByLPath_ExcludePaths_ReturnSuccess end");
}

} // namespace Media
} // namespace OHOS

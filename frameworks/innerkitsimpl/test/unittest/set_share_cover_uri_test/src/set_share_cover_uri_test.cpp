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

#define MLOG_TAG "SetShareCoverUriTest"

#include "set_share_cover_uri_test.h"

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "medialibrary_album_operations.h"
#include "medialibrary_errno.h"
#include "media_file_utils.h"
#include "photo_album_column.h"
#include "media_column.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "rdb_predicates.h"
#include "userfile_manager_types.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;

static constexpr int32_t SHARE_ALBUM_ID = 2001;
static constexpr int32_t SHARE_ALBUM_ID_EXTRA = 2002;
static constexpr int32_t USER_ALBUM_ID = 2003;
static constexpr int32_t NOT_EXIST_ALBUM_ID = 99999999;

static constexpr int32_t MEDIA_ID_LOCAL = 3001;
static constexpr int32_t MEDIA_ID_CLOUD = 3002;
static constexpr int32_t MEDIA_ID_NOT_EXIST = 888888;

static constexpr int32_t SLEEP_FIVE_SECONDS = 5;

static constexpr int64_t TEST_PHOTO_SIZE = 175258;
static constexpr int64_t TEST_PHOTO_DATE_ADDED = 1501924205218;
static constexpr int64_t TEST_PHOTO_DATE_TAKEN = 1501924205;

static const string TEST_OWNER = "test_owner";
static const string OTHER_OWNER = "other_owner";
static const string TEST_ALBUM_NAME = "TestShareAlbum";
static const string TEST_CLOUD_ID = "cloud_id_001";

static vector<string> testTables = {
    PhotoAlbumColumns::TABLE,
    PhotoColumn::PHOTOS_TABLE,
};

static string BuildCoverUri(int32_t fileId)
{
    return "file://media/Photo/" + to_string(fileId);
}

static void ClearTables()
{
    int32_t deleted = 0;
    RdbPredicates albumPredicates(PhotoAlbumColumns::TABLE);
    g_rdbStore->Delete(deleted, albumPredicates);
    RdbPredicates photoPredicates(PhotoColumn::PHOTOS_TABLE);
    g_rdbStore->Delete(deleted, photoPredicates);
}

static int32_t InsertShareAlbum(int32_t albumId, const string &owner,
    int32_t dirty = static_cast<int32_t>(DirtyTypes::TYPE_SYNCED),
    int32_t coverSource = static_cast<int32_t>(CoverUriSource::DEFAULT_COVER))
{
    ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_ID, albumId);
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, static_cast<int32_t>(PhotoAlbumType::SHARE));
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, static_cast<int32_t>(PhotoAlbumSubType::SHARE_GENERIC));
    values.PutInt(PhotoAlbumColumns::ALBUM_SHARE_TYPE,
        static_cast<int32_t>(PhotoAlbumShareType::SHARE_TYPE_SHAREALBUM));
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, TEST_ALBUM_NAME);
    values.PutString(PhotoAlbumColumns::SHARE_ALBUM_OWNER, owner);
    values.PutInt(PhotoAlbumColumns::ALBUM_DIRTY, dirty);
    values.PutInt(PhotoAlbumColumns::COVER_URI_SOURCE, coverSource);
    int64_t rowId = 0;
    int32_t ret = g_rdbStore->Insert(rowId, PhotoAlbumColumns::TABLE, values);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("InsertShareAlbum failed, ret=%{public}d", ret);
        return -1;
    }
    return static_cast<int32_t>(rowId);
}

static int32_t InsertUserAlbum(int32_t albumId, const string &owner)
{
    ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_ID, albumId);
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, static_cast<int32_t>(PhotoAlbumType::USER));
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    values.PutInt(PhotoAlbumColumns::ALBUM_SHARE_TYPE,
        static_cast<int32_t>(PhotoAlbumShareType::SHARE_TYPE_NONEALBUM));
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, TEST_ALBUM_NAME);
    values.PutString(PhotoAlbumColumns::SHARE_ALBUM_OWNER, owner);
    int64_t rowId = 0;
    int32_t ret = g_rdbStore->Insert(rowId, PhotoAlbumColumns::TABLE, values);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("InsertUserAlbum failed, ret=%{public}d", ret);
        return -1;
    }
    return static_cast<int32_t>(rowId);
}

struct PhotoInsertInfo {
    int32_t mediaId = 0;
    int32_t ownerAlbumId = 0;
    int32_t mediaType = MEDIA_TYPE_IMAGE;
    int32_t isShared = static_cast<int32_t>(PhotoSharedType::SHARED);
    int32_t dirty = static_cast<int32_t>(DirtyTypes::TYPE_SYNCED);
    string cloudId;
};

static void FillPhotoBaseColumns(ValuesBucket &values, const PhotoInsertInfo &info)
{
    values.PutInt(MediaColumn::MEDIA_ID, info.mediaId);
    values.PutString(MediaColumn::MEDIA_FILE_PATH,
        "/storage/media/local/files/Photo/" + to_string(info.mediaId) + ".jpg");
    values.PutLong(MediaColumn::MEDIA_SIZE, TEST_PHOTO_SIZE);
    values.PutString(MediaColumn::MEDIA_TITLE, "test_" + to_string(info.mediaId));
    values.PutString(MediaColumn::MEDIA_NAME, "test_" + to_string(info.mediaId) + ".jpg");
    values.PutInt(MediaColumn::MEDIA_TYPE, info.mediaType);
}

static void FillPhotoMetaColumns(ValuesBucket &values)
{
    values.PutString(MediaColumn::MEDIA_OWNER_PACKAGE, "com.ohos.camera");
    values.PutString(MediaColumn::MEDIA_PACKAGE_NAME, "camera");
    values.PutLong(MediaColumn::MEDIA_DATE_ADDED, TEST_PHOTO_DATE_ADDED);
    values.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, static_cast<int64_t>(0));
    values.PutLong(MediaColumn::MEDIA_DATE_TAKEN, TEST_PHOTO_DATE_TAKEN);
    values.PutInt(MediaColumn::MEDIA_DURATION, static_cast<int32_t>(0));
    values.PutInt(MediaColumn::MEDIA_IS_FAV, static_cast<int32_t>(0));
}

static void FillPhotoStateColumns(ValuesBucket &values, const PhotoInsertInfo &info)
{
    values.PutLong(MediaColumn::MEDIA_DATE_TRASHED, static_cast<int64_t>(0));
    values.PutInt(MediaColumn::MEDIA_HIDDEN, static_cast<int32_t>(0));
    values.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, info.ownerAlbumId);
    values.PutInt(PhotoColumn::PHOTO_IS_SHARED, info.isShared);
    values.PutInt(PhotoColumn::PHOTO_DIRTY, info.dirty);
    values.PutString(PhotoColumn::PHOTO_CLOUD_ID, info.cloudId);
    values.PutInt(PhotoColumn::PHOTO_FILE_HIDDEN, static_cast<int32_t>(0));
}

static int32_t InsertPhoto(const PhotoInsertInfo &info)
{
    ValuesBucket values;
    FillPhotoBaseColumns(values, info);
    FillPhotoMetaColumns(values);
    FillPhotoStateColumns(values, info);
    int64_t rowId = 0;
    int32_t ret = g_rdbStore->Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("InsertPhoto failed, ret=%{public}d", ret);
        return -1;
    }
    return static_cast<int32_t>(rowId);
}

static int32_t QueryAlbumIntField(int32_t albumId, const string &column, int64_t &value)
{
    string sql = "SELECT " + column + " FROM " + PhotoAlbumColumns::TABLE + " WHERE " +
        PhotoAlbumColumns::ALBUM_ID + " = ?";
    auto resultSet = g_rdbStore->QuerySql(sql, std::vector<std::string>{ to_string(albumId) });
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("QueryAlbumIntField query failed, albumId=%{public}d", albumId);
        return -1;
    }
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return -1;
    }
    resultSet->GetLong(0, value);
    resultSet->Close();
    return 0;
}

static string QueryAlbumStrField(int32_t albumId, const string &column)
{
    string sql = "SELECT " + column + " FROM " + PhotoAlbumColumns::TABLE + " WHERE " +
        PhotoAlbumColumns::ALBUM_ID + " = ?";
    auto resultSet = g_rdbStore->QuerySql(sql, std::vector<std::string>{ to_string(albumId) });
    if (resultSet == nullptr) {
        return "";
    }
    string value;
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        resultSet->GetString(0, value);
    }
    resultSet->Close();
    return value;
}

void SetShareCoverUriTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::InitUnistore();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateBasicTables(g_rdbStore);
}

void SetShareCoverUriTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryUnitTestUtils::StopUnistore();
    g_rdbStore = nullptr;
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

HWTEST_F(SetShareCoverUriTest, SetShareCoverUri_InvalidAlbumId_001, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    int32_t ret = MediaLibraryAlbumOperations::SetShareCoverUri(0, TEST_OWNER, BuildCoverUri(MEDIA_ID_LOCAL));
    EXPECT_EQ(ret, -EINVAL);
}

HWTEST_F(SetShareCoverUriTest, SetShareCoverUri_EmptyOwner_002, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearTables();
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID, TEST_OWNER), 0);
    int32_t ret = MediaLibraryAlbumOperations::SetShareCoverUri(SHARE_ALBUM_ID, "", BuildCoverUri(MEDIA_ID_LOCAL));
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);
}

HWTEST_F(SetShareCoverUriTest, SetShareCoverUri_EmptyCoverUri_003, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearTables();
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID, TEST_OWNER), 0);
    int32_t ret = MediaLibraryAlbumOperations::SetShareCoverUri(SHARE_ALBUM_ID, TEST_OWNER, "");
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);
}

HWTEST_F(SetShareCoverUriTest, SetShareCoverUri_NotShareAlbum_004, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearTables();
    ASSERT_GT(InsertUserAlbum(USER_ALBUM_ID, TEST_OWNER), 0);
    int32_t ret = MediaLibraryAlbumOperations::SetShareCoverUri(USER_ALBUM_ID, TEST_OWNER,
        BuildCoverUri(MEDIA_ID_LOCAL));
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);
}

HWTEST_F(SetShareCoverUriTest, SetShareCoverUri_OwnerMismatch_005, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearTables();
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID, TEST_OWNER), 0);
    int32_t ret = MediaLibraryAlbumOperations::SetShareCoverUri(SHARE_ALBUM_ID, OTHER_OWNER,
        BuildCoverUri(MEDIA_ID_LOCAL));
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);
}

HWTEST_F(SetShareCoverUriTest, SetShareCoverUri_AlbumNotExist_006, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearTables();
    int32_t ret = MediaLibraryAlbumOperations::SetShareCoverUri(NOT_EXIST_ALBUM_ID, TEST_OWNER,
        BuildCoverUri(MEDIA_ID_LOCAL));
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);
}

HWTEST_F(SetShareCoverUriTest, SetShareCoverUri_InvalidCoverUri_007, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearTables();
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID, TEST_OWNER), 0);
    int32_t ret = MediaLibraryAlbumOperations::SetShareCoverUri(SHARE_ALBUM_ID, TEST_OWNER, "invalid_uri");
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);
}

HWTEST_F(SetShareCoverUriTest, SetShareCoverUri_AssetNotExist_008, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearTables();
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID, TEST_OWNER), 0);
    int32_t ret = MediaLibraryAlbumOperations::SetShareCoverUri(SHARE_ALBUM_ID, TEST_OWNER,
        BuildCoverUri(MEDIA_ID_NOT_EXIST));
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);
}

HWTEST_F(SetShareCoverUriTest, SetShareCoverUri_AssetNotInAlbum_009, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearTables();
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID, TEST_OWNER), 0);
    PhotoInsertInfo assetInfo = { MEDIA_ID_LOCAL, SHARE_ALBUM_ID_EXTRA, MEDIA_TYPE_IMAGE,
        static_cast<int32_t>(PhotoSharedType::SHARED), static_cast<int32_t>(DirtyTypes::TYPE_SYNCED), "" };
    ASSERT_GT(InsertPhoto(assetInfo), 0);
    int32_t ret = MediaLibraryAlbumOperations::SetShareCoverUri(SHARE_ALBUM_ID, TEST_OWNER,
        BuildCoverUri(MEDIA_ID_LOCAL));
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);
}

HWTEST_F(SetShareCoverUriTest, SetShareCoverUri_AssetNotShared_010, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearTables();
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID, TEST_OWNER), 0);
    PhotoInsertInfo assetInfo = { MEDIA_ID_LOCAL, SHARE_ALBUM_ID, MEDIA_TYPE_IMAGE,
        static_cast<int32_t>(PhotoSharedType::NOT_SHARED), static_cast<int32_t>(DirtyTypes::TYPE_SYNCED), "" };
    ASSERT_GT(InsertPhoto(assetInfo), 0);
    int32_t ret = MediaLibraryAlbumOperations::SetShareCoverUri(SHARE_ALBUM_ID, TEST_OWNER,
        BuildCoverUri(MEDIA_ID_LOCAL));
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);
}

HWTEST_F(SetShareCoverUriTest, SetShareCoverUri_AssetNotImageOrVideo_011, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearTables();
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID, TEST_OWNER), 0);
    PhotoInsertInfo assetInfo = { MEDIA_ID_LOCAL, SHARE_ALBUM_ID, MEDIA_TYPE_IMAGE,
        static_cast<int32_t>(PhotoSharedType::SHARED), static_cast<int32_t>(DirtyTypes::TYPE_SYNCED), "" };
    ASSERT_GT(InsertPhoto(assetInfo), 0);
    ValuesBucket updateValues;
    updateValues.PutInt(MediaColumn::MEDIA_TYPE, 0);
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, MEDIA_ID_LOCAL);
    int32_t changedRows = 0;
    ASSERT_EQ(g_rdbStore->Update(changedRows, updateValues, predicates), NativeRdb::E_OK);
    int32_t ret = MediaLibraryAlbumOperations::SetShareCoverUri(SHARE_ALBUM_ID, TEST_OWNER,
        BuildCoverUri(MEDIA_ID_LOCAL));
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);
}

HWTEST_F(SetShareCoverUriTest, SetShareCoverUri_AssetDeleted_012, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearTables();
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID, TEST_OWNER), 0);
    PhotoInsertInfo assetInfo = { MEDIA_ID_LOCAL, SHARE_ALBUM_ID, MEDIA_TYPE_IMAGE,
        static_cast<int32_t>(PhotoSharedType::SHARED), static_cast<int32_t>(DirtyTypes::TYPE_DELETED), "" };
    ASSERT_GT(InsertPhoto(assetInfo), 0);
    int32_t ret = MediaLibraryAlbumOperations::SetShareCoverUri(SHARE_ALBUM_ID, TEST_OWNER,
        BuildCoverUri(MEDIA_ID_LOCAL));
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);
}

HWTEST_F(SetShareCoverUriTest, SetShareCoverUri_SuccessLocalCover_013, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearTables();
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID, TEST_OWNER), 0);
    PhotoInsertInfo assetInfo = { MEDIA_ID_LOCAL, SHARE_ALBUM_ID, MEDIA_TYPE_IMAGE,
        static_cast<int32_t>(PhotoSharedType::SHARED), static_cast<int32_t>(DirtyTypes::TYPE_SYNCED), "" };
    ASSERT_GT(InsertPhoto(assetInfo), 0);
    int32_t ret = MediaLibraryAlbumOperations::SetShareCoverUri(SHARE_ALBUM_ID, TEST_OWNER,
        BuildCoverUri(MEDIA_ID_LOCAL));
    EXPECT_EQ(ret, E_OK);

    int64_t coverSource = -1;
    ASSERT_EQ(QueryAlbumIntField(SHARE_ALBUM_ID, PhotoAlbumColumns::COVER_URI_SOURCE, coverSource), 0);
    EXPECT_EQ(coverSource, static_cast<int64_t>(CoverUriSource::MANUAL_LOCAL_COVER));
    string coverCloudId = QueryAlbumStrField(SHARE_ALBUM_ID, PhotoAlbumColumns::COVER_CLOUD_ID);
    ASSERT_FALSE(coverCloudId.empty());
    EXPECT_EQ(coverCloudId.back(), ',');
}

HWTEST_F(SetShareCoverUriTest, SetShareCoverUri_SuccessCloudCover_014, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearTables();
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID, TEST_OWNER), 0);
    PhotoInsertInfo assetInfo = { MEDIA_ID_CLOUD, SHARE_ALBUM_ID, MEDIA_TYPE_IMAGE,
        static_cast<int32_t>(PhotoSharedType::SHARED), static_cast<int32_t>(DirtyTypes::TYPE_SYNCED), TEST_CLOUD_ID };
    ASSERT_GT(InsertPhoto(assetInfo), 0);
    int32_t ret = MediaLibraryAlbumOperations::SetShareCoverUri(SHARE_ALBUM_ID, TEST_OWNER,
        BuildCoverUri(MEDIA_ID_CLOUD));
    EXPECT_EQ(ret, E_OK);

    int64_t coverSource = -1;
    ASSERT_EQ(QueryAlbumIntField(SHARE_ALBUM_ID, PhotoAlbumColumns::COVER_URI_SOURCE, coverSource), 0);
    EXPECT_EQ(coverSource, static_cast<int64_t>(CoverUriSource::MANUAL_CLOUD_COVER));
    string coverCloudId = QueryAlbumStrField(SHARE_ALBUM_ID, PhotoAlbumColumns::COVER_CLOUD_ID);
    EXPECT_NE(coverCloudId.find(TEST_CLOUD_ID), string::npos);
}

HWTEST_F(SetShareCoverUriTest, SetShareCoverUri_DirtySyncedToMdir_015, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearTables();
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID, TEST_OWNER, static_cast<int32_t>(DirtyTypes::TYPE_SYNCED)), 0);
    PhotoInsertInfo assetInfo = { MEDIA_ID_LOCAL, SHARE_ALBUM_ID, MEDIA_TYPE_IMAGE,
        static_cast<int32_t>(PhotoSharedType::SHARED), static_cast<int32_t>(DirtyTypes::TYPE_SYNCED), "" };
    ASSERT_GT(InsertPhoto(assetInfo), 0);
    int32_t ret = MediaLibraryAlbumOperations::SetShareCoverUri(SHARE_ALBUM_ID, TEST_OWNER,
        BuildCoverUri(MEDIA_ID_LOCAL));
    EXPECT_EQ(ret, E_OK);

    int64_t albumDirty = -1;
    ASSERT_EQ(QueryAlbumIntField(SHARE_ALBUM_ID, PhotoAlbumColumns::ALBUM_DIRTY, albumDirty), 0);
    EXPECT_EQ(albumDirty, static_cast<int64_t>(DirtyTypes::TYPE_MDIRTY));
}

HWTEST_F(SetShareCoverUriTest, SetShareCoverUri_DirtyNotSyncedKeepDirty_016, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearTables();
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID, TEST_OWNER, static_cast<int32_t>(DirtyTypes::TYPE_NEW)), 0);
    PhotoInsertInfo assetInfo = { MEDIA_ID_LOCAL, SHARE_ALBUM_ID, MEDIA_TYPE_IMAGE,
        static_cast<int32_t>(PhotoSharedType::SHARED), static_cast<int32_t>(DirtyTypes::TYPE_SYNCED), "" };
    ASSERT_GT(InsertPhoto(assetInfo), 0);
    int32_t ret = MediaLibraryAlbumOperations::SetShareCoverUri(SHARE_ALBUM_ID, TEST_OWNER,
        BuildCoverUri(MEDIA_ID_LOCAL));
    EXPECT_EQ(ret, E_OK);

    int64_t albumDirty = -1;
    ASSERT_EQ(QueryAlbumIntField(SHARE_ALBUM_ID, PhotoAlbumColumns::ALBUM_DIRTY, albumDirty), 0);
    EXPECT_EQ(albumDirty, static_cast<int64_t>(DirtyTypes::TYPE_NEW));
}
} // namespace OHOS::Media

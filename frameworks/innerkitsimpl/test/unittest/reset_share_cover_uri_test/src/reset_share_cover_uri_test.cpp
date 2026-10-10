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

#define MLOG_TAG "ResetShareCoverUriTest"

#include "reset_share_cover_uri_test.h"

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

static constexpr int32_t SHARE_ALBUM_ID = 2101;
static constexpr int32_t USER_ALBUM_ID = 2103;
static constexpr int32_t NOT_EXIST_ALBUM_ID = 99999999;

static constexpr int32_t SLEEP_FIVE_SECONDS = 5;

static const string TEST_OWNER = "test_owner";
static const string OTHER_OWNER = "other_owner";
static const string TEST_ALBUM_NAME = "TestShareAlbum";

static vector<string> testTables = {
    PhotoAlbumColumns::TABLE,
    PhotoColumn::PHOTOS_TABLE,
};

static void ClearTables()
{
    int32_t deleted = 0;
    RdbPredicates albumPredicates(PhotoAlbumColumns::TABLE);
    g_rdbStore->Delete(deleted, albumPredicates);
}

static int32_t InsertShareAlbum(int32_t albumId, const string &owner,
    int32_t dirty = static_cast<int32_t>(DirtyTypes::TYPE_SYNCED),
    int32_t coverSource = static_cast<int32_t>(CoverUriSource::MANUAL_LOCAL_COVER))
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

void ResetShareCoverUriTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::InitUnistore();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateBasicTables(g_rdbStore);
}

void ResetShareCoverUriTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryUnitTestUtils::StopUnistore();
    g_rdbStore = nullptr;
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

HWTEST_F(ResetShareCoverUriTest, ResetShareCoverUri_InvalidAlbumId_001, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    int32_t ret = MediaLibraryAlbumOperations::ResetShareCoverUri(0, TEST_OWNER);
    EXPECT_EQ(ret, -EINVAL);
}

HWTEST_F(ResetShareCoverUriTest, ResetShareCoverUri_EmptyOwner_002, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearTables();
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID, TEST_OWNER), 0);
    int32_t ret = MediaLibraryAlbumOperations::ResetShareCoverUri(SHARE_ALBUM_ID, "");
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);
}

HWTEST_F(ResetShareCoverUriTest, ResetShareCoverUri_NotShareAlbum_003, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearTables();
    ASSERT_GT(InsertUserAlbum(USER_ALBUM_ID, TEST_OWNER), 0);
    int32_t ret = MediaLibraryAlbumOperations::ResetShareCoverUri(USER_ALBUM_ID, TEST_OWNER);
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);
}

HWTEST_F(ResetShareCoverUriTest, ResetShareCoverUri_OwnerMismatch_004, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearTables();
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID, TEST_OWNER), 0);
    int32_t ret = MediaLibraryAlbumOperations::ResetShareCoverUri(SHARE_ALBUM_ID, OTHER_OWNER);
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);
}

HWTEST_F(ResetShareCoverUriTest, ResetShareCoverUri_AlbumNotExist_005, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearTables();
    int32_t ret = MediaLibraryAlbumOperations::ResetShareCoverUri(NOT_EXIST_ALBUM_ID, TEST_OWNER);
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);
}

HWTEST_F(ResetShareCoverUriTest, ResetShareCoverUri_NoManualCoverIdempotent_006, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearTables();
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID, TEST_OWNER, static_cast<int32_t>(DirtyTypes::TYPE_SYNCED),
        static_cast<int32_t>(CoverUriSource::DEFAULT_COVER)), 0);
    int32_t ret = MediaLibraryAlbumOperations::ResetShareCoverUri(SHARE_ALBUM_ID, TEST_OWNER);
    EXPECT_EQ(ret, E_OK);

    int64_t coverSource = -1;
    ASSERT_EQ(QueryAlbumIntField(SHARE_ALBUM_ID, PhotoAlbumColumns::COVER_URI_SOURCE, coverSource), 0);
    EXPECT_EQ(coverSource, static_cast<int64_t>(CoverUriSource::DEFAULT_COVER));
}

HWTEST_F(ResetShareCoverUriTest, ResetShareCoverUri_HasManualLocalCover_007, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearTables();
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID, TEST_OWNER, static_cast<int32_t>(DirtyTypes::TYPE_SYNCED),
        static_cast<int32_t>(CoverUriSource::MANUAL_LOCAL_COVER)), 0);
    int32_t ret = MediaLibraryAlbumOperations::ResetShareCoverUri(SHARE_ALBUM_ID, TEST_OWNER);
    EXPECT_EQ(ret, E_OK);

    int64_t coverSource = -1;
    ASSERT_EQ(QueryAlbumIntField(SHARE_ALBUM_ID, PhotoAlbumColumns::COVER_URI_SOURCE, coverSource), 0);
    EXPECT_EQ(coverSource, static_cast<int64_t>(CoverUriSource::DEFAULT_COVER));
    string coverCloudId = QueryAlbumStrField(SHARE_ALBUM_ID, PhotoAlbumColumns::COVER_CLOUD_ID);
    ASSERT_FALSE(coverCloudId.empty());
    EXPECT_EQ(coverCloudId.back(), ',');
}

HWTEST_F(ResetShareCoverUriTest, ResetShareCoverUri_HasManualCloudCover_008, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearTables();
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID, TEST_OWNER, static_cast<int32_t>(DirtyTypes::TYPE_SYNCED),
        static_cast<int32_t>(CoverUriSource::MANUAL_CLOUD_COVER)), 0);
    int32_t ret = MediaLibraryAlbumOperations::ResetShareCoverUri(SHARE_ALBUM_ID, TEST_OWNER);
    EXPECT_EQ(ret, E_OK);

    int64_t coverSource = -1;
    ASSERT_EQ(QueryAlbumIntField(SHARE_ALBUM_ID, PhotoAlbumColumns::COVER_URI_SOURCE, coverSource), 0);
    EXPECT_EQ(coverSource, static_cast<int64_t>(CoverUriSource::DEFAULT_COVER));
}

HWTEST_F(ResetShareCoverUriTest, ResetShareCoverUri_DirtySyncedToMdir_009, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearTables();
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID, TEST_OWNER, static_cast<int32_t>(DirtyTypes::TYPE_SYNCED),
        static_cast<int32_t>(CoverUriSource::MANUAL_LOCAL_COVER)), 0);
    int32_t ret = MediaLibraryAlbumOperations::ResetShareCoverUri(SHARE_ALBUM_ID, TEST_OWNER);
    EXPECT_EQ(ret, E_OK);

    int64_t albumDirty = -1;
    ASSERT_EQ(QueryAlbumIntField(SHARE_ALBUM_ID, PhotoAlbumColumns::ALBUM_DIRTY, albumDirty), 0);
    EXPECT_EQ(albumDirty, static_cast<int64_t>(DirtyTypes::TYPE_MDIRTY));
}

HWTEST_F(ResetShareCoverUriTest, ResetShareCoverUri_ResetTwice_010, TestSize.Level1)
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearTables();
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID, TEST_OWNER, static_cast<int32_t>(DirtyTypes::TYPE_SYNCED),
        static_cast<int32_t>(CoverUriSource::MANUAL_CLOUD_COVER)), 0);
    EXPECT_EQ(MediaLibraryAlbumOperations::ResetShareCoverUri(SHARE_ALBUM_ID, TEST_OWNER), E_OK);
    EXPECT_EQ(MediaLibraryAlbumOperations::ResetShareCoverUri(SHARE_ALBUM_ID, TEST_OWNER), E_OK);

    int64_t coverSource = -1;
    ASSERT_EQ(QueryAlbumIntField(SHARE_ALBUM_ID, PhotoAlbumColumns::COVER_URI_SOURCE, coverSource), 0);
    EXPECT_EQ(coverSource, static_cast<int64_t>(CoverUriSource::DEFAULT_COVER));
}
} // namespace OHOS::Media

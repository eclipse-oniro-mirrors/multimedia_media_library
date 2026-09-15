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

#define MLOG_TAG "DeleteMemberShareAlbumTest"

#include "delete_member_share_album_test.h"

#include <memory>
#include <string>
#include <vector>

#include "medialibrary_album_operations.h"
#include "medialibrary_errno.h"
#include "media_file_utils.h"
#include "share_member_column.h"
#include "photo_album_column.h"
#include "media_column.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "test_data_builder.h"
#include "rdb_predicates.h"
#include "media_upgrade.h"
#include "userfile_manager_types.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;

static constexpr int32_t SHARE_ALBUM_ID = 1001;
static constexpr int32_t SHARE_ALBUM_ID_EXTRA = 1002;
static constexpr int32_t USER_ALBUM_ID = 1003;
static constexpr int32_t NOT_EXIST_ALBUM_ID = 99999999;

static const string TEST_OWNER = "test_owner";
static const string OTHER_MEMBER = "other_member";
static const string TEST_ALBUM_NAME = "TestShareAlbum";

static vector<string> testTables = {
    PhotoAlbumColumns::TABLE,
    PhotoColumn::PHOTOS_TABLE,
    ShareMemberColumn::TABLE_NAME,
};

static int32_t InsertShareAlbum(int32_t albumId, const string &albumName, const string &owner)
{
    ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_ID, albumId);
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, static_cast<int32_t>(PhotoAlbumType::SHARE));
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, static_cast<int32_t>(PhotoAlbumSubType::SHARE_GENERIC));
    values.PutInt(PhotoAlbumColumns::ALBUM_SHARE_TYPE,
        static_cast<int32_t>(PhotoAlbumShareType::SHARE_TYPE_SHAREALBUM));
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, albumName);
    values.PutString(PhotoAlbumColumns::SHARE_ALBUM_OWNER, owner);
    int64_t rowId = 0;
    int32_t ret = g_rdbStore->Insert(rowId, PhotoAlbumColumns::TABLE, values);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("InsertShareAlbum failed, ret=%{public}d", ret);
        return -1;
    }
    return static_cast<int32_t>(rowId);
}

static int32_t InsertNonShareAlbum(int32_t albumId, const string &albumName, const string &owner)
{
    ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_ID, albumId);
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, static_cast<int32_t>(PhotoAlbumType::USER));
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
    values.PutInt(PhotoAlbumColumns::ALBUM_SHARE_TYPE,
        static_cast<int32_t>(PhotoAlbumShareType::SHARE_TYPE_NONEALBUM));
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, albumName);
    values.PutString(PhotoAlbumColumns::SHARE_ALBUM_OWNER, owner);
    int64_t rowId = 0;
    int32_t ret = g_rdbStore->Insert(rowId, PhotoAlbumColumns::TABLE, values);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("InsertNonShareAlbum failed, ret=%{public}d", ret);
        return -1;
    }
    return static_cast<int32_t>(rowId);
}

static int32_t InsertShareMember(int32_t albumId, const string &member, int32_t status)
{
    ValuesBucket values;
    values.PutInt(ShareMemberColumn::COLUMN_ALBUM_ID, albumId);
    values.PutString(ShareMemberColumn::COLUMN_SHARE_MEMBER, member);
    values.PutInt(ShareMemberColumn::COLUMN_SHARE_MEMBER_STATUS, status);
    int64_t rowId = 0;
    int32_t ret = g_rdbStore->Insert(rowId, ShareMemberColumn::TABLE_NAME, values);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("InsertShareMember failed, ret=%{public}d", ret);
        return -1;
    }
    return static_cast<int32_t>(rowId);
}

static void ClearShareAlbumTable(void)
{
    int32_t deleted = 0;
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    g_rdbStore->Delete(deleted, predicates);
}

static void ClearShareMemberTable(void)
{
    int32_t deleted = 0;
    RdbPredicates predicates(ShareMemberColumn::TABLE_NAME);
    g_rdbStore->Delete(deleted, predicates);
}

static int32_t CountAlbumById(int32_t albumId)
{
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    auto resultSet = g_rdbStore->Query(predicates, { PhotoAlbumColumns::ALBUM_ID });
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("CountAlbumById query failed, albumId=%{public}d", albumId);
        return -1;
    }
    int32_t count = 0;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        count++;
    }
    resultSet->Close();
    return count;
}

static int32_t CountShareMember(int32_t albumId, const string &member)
{
    RdbPredicates predicates(ShareMemberColumn::TABLE_NAME);
    predicates.EqualTo(ShareMemberColumn::COLUMN_ALBUM_ID, albumId)
        ->And()->EqualTo(ShareMemberColumn::COLUMN_SHARE_MEMBER, member);
    auto resultSet = g_rdbStore->Query(predicates, { ShareMemberColumn::COLUMN_ID });
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("CountShareMember query failed, albumId=%{public}d", albumId);
        return -1;
    }
    int32_t count = 0;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        count++;
    }
    resultSet->Close();
    return count;
}

void DeleteMemberShareAlbumTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::InitUnistore();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateBasicTables(g_rdbStore);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, { SQL_CREATE_TAB_SHARE_ALBUM_MEMBER });
}

void DeleteMemberShareAlbumTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryUnitTestUtils::StopUnistore();
    g_rdbStore = nullptr;
}

HWTEST_F(DeleteMemberShareAlbumTest, DeleteMemberShareAlbum_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteMemberShareAlbum_001 start");

    std::vector<int32_t> emptyAlbumIds;
    int32_t ret = MediaLibraryAlbumOperations::DeleteMemberShareAlbum(TEST_OWNER, emptyAlbumIds);
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);

    MEDIA_INFO_LOG("DeleteMemberShareAlbum_001 end, ret=%{public}d", ret);
}

HWTEST_F(DeleteMemberShareAlbumTest, DeleteMemberShareAlbum_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteMemberShareAlbum_002 start");

    ASSERT_NE(g_rdbStore, nullptr);
    ClearShareAlbumTable();
    ClearShareMemberTable();

    std::vector<int32_t> albumIds = { NOT_EXIST_ALBUM_ID };
    int32_t ret = MediaLibraryAlbumOperations::DeleteMemberShareAlbum(TEST_OWNER, albumIds);
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);

    MEDIA_INFO_LOG("DeleteMemberShareAlbum_002 end, ret=%{public}d", ret);
}

HWTEST_F(DeleteMemberShareAlbumTest, DeleteMemberShareAlbum_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteMemberShareAlbum_003 start");

    ASSERT_NE(g_rdbStore, nullptr);
    ClearShareAlbumTable();
    ClearShareMemberTable();
    int32_t albumId = InsertNonShareAlbum(USER_ALBUM_ID, TEST_ALBUM_NAME, TEST_OWNER);
    ASSERT_GT(albumId, 0);

    std::vector<int32_t> albumIds = { USER_ALBUM_ID };
    int32_t ret = MediaLibraryAlbumOperations::DeleteMemberShareAlbum(TEST_OWNER, albumIds);
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);

    MEDIA_INFO_LOG("DeleteMemberShareAlbum_003 end, ret=%{public}d", ret);
}

HWTEST_F(DeleteMemberShareAlbumTest, DeleteMemberShareAlbum_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteMemberShareAlbum_004 start");

    ASSERT_NE(g_rdbStore, nullptr);
    ClearShareAlbumTable();
    ClearShareMemberTable();
    int32_t albumId = InsertShareAlbum(SHARE_ALBUM_ID, TEST_ALBUM_NAME, TEST_OWNER);
    ASSERT_GT(albumId, 0);

    std::vector<int32_t> albumIds = { SHARE_ALBUM_ID };
    int32_t ret = MediaLibraryAlbumOperations::DeleteMemberShareAlbum(TEST_OWNER, albumIds);
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);

    MEDIA_INFO_LOG("DeleteMemberShareAlbum_004 end, ret=%{public}d", ret);
}

HWTEST_F(DeleteMemberShareAlbumTest, DeleteMemberShareAlbum_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteMemberShareAlbum_005 start");

    ASSERT_NE(g_rdbStore, nullptr);
    ClearShareAlbumTable();
    ClearShareMemberTable();
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID, TEST_ALBUM_NAME, TEST_OWNER), 0);
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID_EXTRA, TEST_ALBUM_NAME, TEST_OWNER), 0);
    ASSERT_GT(InsertShareMember(SHARE_ALBUM_ID, TEST_OWNER, static_cast<int32_t>(ShareMemberStatus::ACCEPTED)), 0);

    std::vector<int32_t> albumIds = { SHARE_ALBUM_ID, SHARE_ALBUM_ID_EXTRA };
    int32_t ret = MediaLibraryAlbumOperations::DeleteMemberShareAlbum(TEST_OWNER, albumIds);
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);
    EXPECT_EQ(CountAlbumById(SHARE_ALBUM_ID), 1);
    EXPECT_EQ(CountShareMember(SHARE_ALBUM_ID, TEST_OWNER), 1);

    MEDIA_INFO_LOG("DeleteMemberShareAlbum_005 end, ret=%{public}d", ret);
}

HWTEST_F(DeleteMemberShareAlbumTest, DeleteMemberShareAlbum_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteMemberShareAlbum_006 start");

    ASSERT_NE(g_rdbStore, nullptr);
    ClearShareAlbumTable();
    ClearShareMemberTable();
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID, TEST_ALBUM_NAME, TEST_OWNER), 0);
    ASSERT_GT(InsertShareMember(SHARE_ALBUM_ID, TEST_OWNER, static_cast<int32_t>(ShareMemberStatus::ACCEPTED)), 0);
    ASSERT_GT(InsertShareMember(SHARE_ALBUM_ID, OTHER_MEMBER, static_cast<int32_t>(ShareMemberStatus::ACCEPTED)), 0);

    std::vector<int32_t> albumIds = { SHARE_ALBUM_ID };
    int32_t ret = MediaLibraryAlbumOperations::DeleteMemberShareAlbum(TEST_OWNER, albumIds);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(CountAlbumById(SHARE_ALBUM_ID), 0);
    EXPECT_EQ(CountShareMember(SHARE_ALBUM_ID, TEST_OWNER), 0);
    EXPECT_EQ(CountShareMember(SHARE_ALBUM_ID, OTHER_MEMBER), 0);

    MEDIA_INFO_LOG("DeleteMemberShareAlbum_006 end, ret=%{public}d", ret);
}

HWTEST_F(DeleteMemberShareAlbumTest, DeleteMemberShareAlbum_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteMemberShareAlbum_007 start");

    ASSERT_NE(g_rdbStore, nullptr);
    ClearShareAlbumTable();
    ClearShareMemberTable();
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID, TEST_ALBUM_NAME, TEST_OWNER), 0);
    ASSERT_GT(InsertShareAlbum(SHARE_ALBUM_ID_EXTRA, TEST_ALBUM_NAME, TEST_OWNER), 0);
    ASSERT_GT(InsertShareMember(SHARE_ALBUM_ID, TEST_OWNER, static_cast<int32_t>(ShareMemberStatus::ACCEPTED)), 0);
    ASSERT_GT(InsertShareMember(SHARE_ALBUM_ID_EXTRA, TEST_OWNER, static_cast<int32_t>(ShareMemberStatus::ACCEPTED)),
        0);

    std::vector<int32_t> albumIds = { SHARE_ALBUM_ID, SHARE_ALBUM_ID_EXTRA };
    int32_t ret = MediaLibraryAlbumOperations::DeleteMemberShareAlbum(TEST_OWNER, albumIds);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(CountAlbumById(SHARE_ALBUM_ID), 0);
    EXPECT_EQ(CountAlbumById(SHARE_ALBUM_ID_EXTRA), 0);
    EXPECT_EQ(CountShareMember(SHARE_ALBUM_ID, TEST_OWNER), 0);
    EXPECT_EQ(CountShareMember(SHARE_ALBUM_ID_EXTRA, TEST_OWNER), 0);

    MEDIA_INFO_LOG("DeleteMemberShareAlbum_007 end, ret=%{public}d", ret);
}
} // namespace OHOS::Media

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

#define MLOG_TAG "UpdateShareMemberStatusTest"

#include "update_share_member_status_test.h"

#include <cerrno>
#include <memory>
#include <string>
#include <vector>

#include "medialibrary_album_operations.h"
#include "medialibrary_errno.h"
#include "media_file_utils.h"
#include "share_member_column.h"
#include "photo_album_column.h"
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
static constexpr int32_t USER_ALBUM_ID = 1002;
static constexpr int32_t NOT_EXIST_ALBUM_ID = 99999999;
static constexpr int32_t INVALID_ALBUM_ID = 0;
static constexpr int32_t STATUS_OUT_OF_RANGE = 10;

static const string TEST_OWNER = "test_owner";
static const string OTHER_OWNER = "other_owner";
static const string TEST_MEMBER = "test_member";
static const string TEST_ALBUM_NAME = "TestShareAlbum";

static vector<string> createTableSqlLists = {
    PhotoAlbumColumns::CREATE_TABLE,
    SQL_CREATE_TAB_SHARE_ALBUM_MEMBER,
};

static vector<string> testTables = {
    PhotoAlbumColumns::TABLE,
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

static int32_t QueryShareMemberStatus(int32_t albumId, const string &member)
{
    RdbPredicates predicates(ShareMemberColumn::TABLE_NAME);
    predicates.EqualTo(ShareMemberColumn::COLUMN_ALBUM_ID, albumId)
        ->And()->EqualTo(ShareMemberColumn::COLUMN_SHARE_MEMBER, member);
    auto resultSet = g_rdbStore->Query(predicates, { ShareMemberColumn::COLUMN_SHARE_MEMBER_STATUS });
    if (resultSet == nullptr) {
        return -1;
    }
    int32_t status = 0;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t statusIdx = 0;
        if (resultSet->GetColumnIndex(ShareMemberColumn::COLUMN_SHARE_MEMBER_STATUS, statusIdx) == NativeRdb::E_OK) {
            resultSet->GetInt(statusIdx, status);
        }
    }
    resultSet->Close();
    return status;
}

void UpdateShareMemberStatusTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::InitUnistore();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
}

void UpdateShareMemberStatusTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryUnitTestUtils::StopUnistore();
    g_rdbStore = nullptr;
}

HWTEST_F(UpdateShareMemberStatusTest, UpdateShareMemberStatus_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateShareMemberStatus_001 start");

    ASSERT_NE(g_rdbStore, nullptr);
    ClearShareAlbumTable();
    ClearShareMemberTable();
    int32_t shareAlbumId = InsertShareAlbum(SHARE_ALBUM_ID, TEST_ALBUM_NAME, TEST_OWNER);
    ASSERT_GT(shareAlbumId, 0);
    int32_t memberId = InsertShareMember(SHARE_ALBUM_ID, TEST_MEMBER, ShareMemberStatus::INVITING);
    ASSERT_GT(memberId, 0);

    int32_t ret = MediaLibraryAlbumOperations::UpdateShareMemberStatus(
        SHARE_ALBUM_ID, TEST_OWNER, TEST_MEMBER, ShareMemberStatus::ACCEPTED);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(QueryShareMemberStatus(SHARE_ALBUM_ID, TEST_MEMBER), ShareMemberStatus::ACCEPTED);

    MEDIA_INFO_LOG("UpdateShareMemberStatus_001 end, ret=%{public}d", ret);
}

HWTEST_F(UpdateShareMemberStatusTest, UpdateShareMemberStatus_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateShareMemberStatus_002 start");

    int32_t ret = MediaLibraryAlbumOperations::UpdateShareMemberStatus(
        INVALID_ALBUM_ID, TEST_OWNER, TEST_MEMBER, ShareMemberStatus::ACCEPTED);
    EXPECT_EQ(ret, -EINVAL);

    MEDIA_INFO_LOG("UpdateShareMemberStatus_002 end, ret=%{public}d", ret);
}

HWTEST_F(UpdateShareMemberStatusTest, UpdateShareMemberStatus_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateShareMemberStatus_003 start");

    ASSERT_NE(g_rdbStore, nullptr);
    ClearShareAlbumTable();
    ClearShareMemberTable();

    int32_t ret = MediaLibraryAlbumOperations::UpdateShareMemberStatus(
        NOT_EXIST_ALBUM_ID, TEST_OWNER, TEST_MEMBER, ShareMemberStatus::ACCEPTED);
    EXPECT_EQ(ret, -EINVAL);

    MEDIA_INFO_LOG("UpdateShareMemberStatus_003 end, ret=%{public}d", ret);
}

HWTEST_F(UpdateShareMemberStatusTest, UpdateShareMemberStatus_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateShareMemberStatus_004 start");

    ASSERT_NE(g_rdbStore, nullptr);
    ClearShareAlbumTable();
    ClearShareMemberTable();
    int32_t albumId = InsertNonShareAlbum(USER_ALBUM_ID, TEST_ALBUM_NAME, TEST_OWNER);
    ASSERT_GT(albumId, 0);

    int32_t ret = MediaLibraryAlbumOperations::UpdateShareMemberStatus(
        USER_ALBUM_ID, TEST_OWNER, TEST_MEMBER, ShareMemberStatus::ACCEPTED);
    EXPECT_EQ(ret, -EINVAL);

    MEDIA_INFO_LOG("UpdateShareMemberStatus_004 end, ret=%{public}d", ret);
}

HWTEST_F(UpdateShareMemberStatusTest, UpdateShareMemberStatus_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateShareMemberStatus_005 start");

    ASSERT_NE(g_rdbStore, nullptr);
    ClearShareAlbumTable();
    ClearShareMemberTable();
    int32_t shareAlbumId = InsertShareAlbum(SHARE_ALBUM_ID, TEST_ALBUM_NAME, TEST_OWNER);
    ASSERT_GT(shareAlbumId, 0);

    int32_t ret = MediaLibraryAlbumOperations::UpdateShareMemberStatus(
        SHARE_ALBUM_ID, TEST_OWNER, TEST_MEMBER, STATUS_OUT_OF_RANGE);
    EXPECT_EQ(ret, -EINVAL);

    MEDIA_INFO_LOG("UpdateShareMemberStatus_005 end, ret=%{public}d", ret);
}

HWTEST_F(UpdateShareMemberStatusTest, UpdateShareMemberStatus_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateShareMemberStatus_006 start");

    ASSERT_NE(g_rdbStore, nullptr);
    ClearShareAlbumTable();
    ClearShareMemberTable();
    int32_t shareAlbumId = InsertShareAlbum(SHARE_ALBUM_ID, TEST_ALBUM_NAME, TEST_OWNER);
    ASSERT_GT(shareAlbumId, 0);

    int32_t ret = MediaLibraryAlbumOperations::UpdateShareMemberStatus(
        SHARE_ALBUM_ID, OTHER_OWNER, TEST_MEMBER, ShareMemberStatus::ACCEPTED);
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);

    MEDIA_INFO_LOG("UpdateShareMemberStatus_006 end, ret=%{public}d", ret);
}

HWTEST_F(UpdateShareMemberStatusTest, UpdateShareMemberStatus_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateShareMemberStatus_007 start");

    ASSERT_NE(g_rdbStore, nullptr);
    ClearShareAlbumTable();
    ClearShareMemberTable();
    int32_t shareAlbumId = InsertShareAlbum(SHARE_ALBUM_ID, TEST_ALBUM_NAME, TEST_OWNER);
    ASSERT_GT(shareAlbumId, 0);

    int32_t ret = MediaLibraryAlbumOperations::UpdateShareMemberStatus(
        SHARE_ALBUM_ID, TEST_OWNER, TEST_MEMBER, ShareMemberStatus::ACCEPTED);
    EXPECT_EQ(ret, -EINVAL);

    MEDIA_INFO_LOG("UpdateShareMemberStatus_007 end, ret=%{public}d", ret);
}

HWTEST_F(UpdateShareMemberStatusTest, UpdateShareMemberStatus_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateShareMemberStatus_008 start");

    ASSERT_NE(g_rdbStore, nullptr);
    ClearShareAlbumTable();
    ClearShareMemberTable();
    int32_t shareAlbumId = InsertShareAlbum(SHARE_ALBUM_ID, TEST_ALBUM_NAME, TEST_OWNER);
    ASSERT_GT(shareAlbumId, 0);
    int32_t memberId = InsertShareMember(SHARE_ALBUM_ID, TEST_MEMBER, ShareMemberStatus::ACCEPTED);
    ASSERT_GT(memberId, 0);

    int32_t ret = MediaLibraryAlbumOperations::UpdateShareMemberStatus(
        SHARE_ALBUM_ID, TEST_OWNER, TEST_MEMBER, ShareMemberStatus::DECLINED);
    EXPECT_EQ(ret, -EINVAL);

    MEDIA_INFO_LOG("UpdateShareMemberStatus_008 end, ret=%{public}d", ret);
}

HWTEST_F(UpdateShareMemberStatusTest, UpdateShareMemberStatus_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateShareMemberStatus_009 start");

    ASSERT_NE(g_rdbStore, nullptr);
    ClearShareAlbumTable();
    ClearShareMemberTable();
    int32_t shareAlbumId = InsertShareAlbum(SHARE_ALBUM_ID, TEST_ALBUM_NAME, TEST_OWNER);
    ASSERT_GT(shareAlbumId, 0);
    int32_t memberId = InsertShareMember(SHARE_ALBUM_ID, TEST_MEMBER, ShareMemberStatus::REQUESTING);
    ASSERT_GT(memberId, 0);

    int32_t ret = MediaLibraryAlbumOperations::UpdateShareMemberStatus(
        SHARE_ALBUM_ID, TEST_OWNER, TEST_MEMBER, ShareMemberStatus::ACCEPTED);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(QueryShareMemberStatus(SHARE_ALBUM_ID, TEST_MEMBER), ShareMemberStatus::ACCEPTED);

    MEDIA_INFO_LOG("UpdateShareMemberStatus_009 end, ret=%{public}d", ret);
}
} // namespace OHOS::Media

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
#define MLOG_TAG "AddShareMemberTest"

#include "add_share_member_test.h"

#include <cerrno>
#include <memory>
#include <string>
#include <vector>

#include "medialibrary_album_operations.h"
#include "medialibrary_errno.h"
#include "media_file_utils.h"
#include "share_member_column.h"
#include "add_share_member_vo.h"
#include "photo_album_column.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
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
static constexpr int32_t MEMBER_STATUS = 1;

static const string TEST_OWNER = "test_owner";
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

static int32_t CountShareMember(int32_t albumId, const string &member)
{
    RdbPredicates predicates(ShareMemberColumn::TABLE_NAME);
    predicates.EqualTo(ShareMemberColumn::COLUMN_ALBUM_ID, albumId)
        ->And()->EqualTo(ShareMemberColumn::COLUMN_SHARE_MEMBER, member);
    auto resultSet = g_rdbStore->Query(predicates, { ShareMemberColumn::COLUMN_ID });
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("CountShareMember query failed");
        return -1;
    }
    int32_t count = 0;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        count++;
    }
    resultSet->Close();
    return count;
}

static int32_t QueryShareMemberStatus(int32_t albumId, const string &member)
{
    RdbPredicates predicates(ShareMemberColumn::TABLE_NAME);
    predicates.EqualTo(ShareMemberColumn::COLUMN_ALBUM_ID, albumId)
        ->And()->EqualTo(ShareMemberColumn::COLUMN_SHARE_MEMBER, member);
    auto resultSet = g_rdbStore->Query(predicates, { ShareMemberColumn::COLUMN_SHARE_MEMBER_STATUS });
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("QueryShareMemberStatus failed, albumId=%{public}d", albumId);
        return -1;
    }
    int32_t columnIndex = -1;
    resultSet->GetColumnIndex(ShareMemberColumn::COLUMN_SHARE_MEMBER_STATUS, columnIndex);
    int32_t status = 0;
    resultSet->GetInt(columnIndex, status);
    resultSet->Close();
    return status;
}

void AddShareMemberTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::InitUnistore();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
}

void AddShareMemberTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryUnitTestUtils::StopUnistore();
    g_rdbStore = nullptr;
}

HWTEST_F(AddShareMemberTest, AddShareMember_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AddShareMember_001 start");

    ASSERT_NE(g_rdbStore, nullptr);
    ClearShareAlbumTable();
    ClearShareMemberTable();
    int32_t shareAlbumId = InsertShareAlbum(SHARE_ALBUM_ID, TEST_ALBUM_NAME, TEST_OWNER);
    ASSERT_GT(shareAlbumId, 0);

    int32_t ret = MediaLibraryAlbumOperations::AddShareMember(SHARE_ALBUM_ID, TEST_OWNER, TEST_MEMBER, MEMBER_STATUS);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(CountShareMember(SHARE_ALBUM_ID, TEST_MEMBER), 1);
    EXPECT_EQ(QueryShareMemberStatus(SHARE_ALBUM_ID, TEST_MEMBER), MEMBER_STATUS);

    MEDIA_INFO_LOG("AddShareMember_001 end, ret=%{public}d", ret);
}

HWTEST_F(AddShareMemberTest, AddShareMember_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("AddShareMember_002 start");

    int32_t ret = MediaLibraryAlbumOperations::AddShareMember(0, TEST_OWNER, TEST_MEMBER, MEMBER_STATUS);
    EXPECT_EQ(ret, -EINVAL);

    MEDIA_INFO_LOG("AddShareMember_002 end, ret=%{public}d", ret);
}

HWTEST_F(AddShareMemberTest, AddShareMember_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("AddShareMember_003 start");

    AddShareMemberReqBody reqBody;
    reqBody.albumId = SHARE_ALBUM_ID;
    reqBody.owner = TEST_OWNER;
    reqBody.member = TEST_MEMBER;
    reqBody.status = MEMBER_STATUS;

    MessageParcel data;
    ASSERT_TRUE(reqBody.Marshalling(data));

    AddShareMemberReqBody outBody;
    ASSERT_TRUE(outBody.Unmarshalling(data));
    EXPECT_EQ(outBody.albumId, SHARE_ALBUM_ID);
    EXPECT_EQ(outBody.owner, TEST_OWNER);
    EXPECT_EQ(outBody.member, TEST_MEMBER);
    EXPECT_EQ(outBody.status, MEMBER_STATUS);

    MEDIA_INFO_LOG("AddShareMember_003 end");
}
} // namespace OHOS::Media

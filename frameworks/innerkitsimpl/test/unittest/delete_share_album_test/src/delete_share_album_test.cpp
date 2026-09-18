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

#define MLOG_TAG "DeleteShareAlbumTest"

#include "delete_share_album_test.h"

#include <memory>
#include <string>
#include <vector>

#include "medialibrary_album_operations.h"
#include "medialibrary_errno.h"
#include "media_file_utils.h"
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
static constexpr int32_t USER_ALBUM_ID = 1002;
static constexpr int32_t NOT_EXIST_ALBUM_ID = 99999999;

static const string TEST_OWNER = "test_owner";
static const string OTHER_OWNER = "other_owner";
static const string TEST_ALBUM_NAME = "TestShareAlbum";

static vector<string> testTables = {
    PhotoAlbumColumns::TABLE,
    PhotoColumn::PHOTOS_TABLE,
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

static void ClearShareAlbumTable(void)
{
    int32_t deleted = 0;
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    g_rdbStore->Delete(deleted, predicates);
}

static int32_t QueryAlbumDirty(int32_t albumId)
{
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    auto resultSet = g_rdbStore->Query(predicates, { PhotoAlbumColumns::ALBUM_DIRTY });
    if (resultSet == nullptr) {
        return -1;
    }
    int32_t dirty = 0;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t dirtyIdx = 0;
        if (resultSet->GetColumnIndex(PhotoAlbumColumns::ALBUM_DIRTY, dirtyIdx) == NativeRdb::E_OK) {
            resultSet->GetInt(dirtyIdx, dirty);
        }
    }
    resultSet->Close();
    return dirty;
}

void DeleteShareAlbumTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::InitUnistore();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateBasicTables(g_rdbStore);
}

void DeleteShareAlbumTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryUnitTestUtils::StopUnistore();
    g_rdbStore = nullptr;
}

HWTEST_F(DeleteShareAlbumTest, DeleteSharePhotoAlbum_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteSharePhotoAlbum_001 start");

    std::vector<int32_t> emptyAlbumIds;
    int32_t ret = MediaLibraryAlbumOperations::DeleteSharePhotoAlbum(TEST_OWNER, emptyAlbumIds);
    EXPECT_EQ(ret, E_INVALID_ARGS);

    MEDIA_INFO_LOG("DeleteSharePhotoAlbum_001 end, ret=%{public}d", ret);
}

HWTEST_F(DeleteShareAlbumTest, DeleteSharePhotoAlbum_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteSharePhotoAlbum_002 start");

    ASSERT_NE(g_rdbStore, nullptr);
    ClearShareAlbumTable();

    std::vector<int32_t> albumIds = { NOT_EXIST_ALBUM_ID };
    int32_t ret = MediaLibraryAlbumOperations::DeleteSharePhotoAlbum(TEST_OWNER, albumIds);
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);

    MEDIA_INFO_LOG("DeleteSharePhotoAlbum_002 end, ret=%{public}d", ret);
}

HWTEST_F(DeleteShareAlbumTest, DeleteSharePhotoAlbum_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteSharePhotoAlbum_003 start");

    ASSERT_NE(g_rdbStore, nullptr);
    ClearShareAlbumTable();
    int32_t albumId = InsertNonShareAlbum(USER_ALBUM_ID, TEST_ALBUM_NAME, TEST_OWNER);
    ASSERT_GT(albumId, 0);

    std::vector<int32_t> albumIds = { USER_ALBUM_ID };
    int32_t ret = MediaLibraryAlbumOperations::DeleteSharePhotoAlbum(TEST_OWNER, albumIds);
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);

    MEDIA_INFO_LOG("DeleteSharePhotoAlbum_003 end, ret=%{public}d", ret);
}

HWTEST_F(DeleteShareAlbumTest, DeleteSharePhotoAlbum_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteSharePhotoAlbum_004 start");

    ASSERT_NE(g_rdbStore, nullptr);
    ClearShareAlbumTable();
    int32_t albumId = InsertShareAlbum(SHARE_ALBUM_ID, TEST_ALBUM_NAME, TEST_OWNER);
    ASSERT_GT(albumId, 0);

    std::vector<int32_t> albumIds = { SHARE_ALBUM_ID };
    int32_t ret = MediaLibraryAlbumOperations::DeleteSharePhotoAlbum(OTHER_OWNER, albumIds);
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);

    MEDIA_INFO_LOG("DeleteSharePhotoAlbum_004 end, ret=%{public}d", ret);
}

HWTEST_F(DeleteShareAlbumTest, DeleteSharePhotoAlbum_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("DeleteSharePhotoAlbum_005 start");

    ASSERT_NE(g_rdbStore, nullptr);
    ClearShareAlbumTable();
    int32_t albumId = InsertShareAlbum(SHARE_ALBUM_ID, TEST_ALBUM_NAME, TEST_OWNER);
    ASSERT_GT(albumId, 0);

    std::vector<int32_t> albumIds = { SHARE_ALBUM_ID };
    int32_t ret = MediaLibraryAlbumOperations::DeleteSharePhotoAlbum(TEST_OWNER, albumIds);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(QueryAlbumDirty(SHARE_ALBUM_ID), static_cast<int32_t>(DirtyTypes::TYPE_DELETED));

    MEDIA_INFO_LOG("DeleteSharePhotoAlbum_005 end, ret=%{public}d", ret);
}
} // namespace OHOS::Media

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

#define MLOG_TAG "MediaAlbumsControllerServiceTest"

#include "change_request_absorb_album_test.h"

#include <memory>
#include <string>
#include "media_albums_controller_service.h"
#include "media_albums_service.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "permission_utils.h"
#include "get_self_permissions.h"
#include "result_set_utils.h"
#include "vision_db_sqls.h"
#include "media_upgrade.h"
#include "photo_album_column.h"
#include "change_request_absorb_album_vo.h"
#include "medialibrary_client_errno.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace IPC;

static shared_ptr<MediaLibraryRdbStore> g_rdbStoreAbsorb;
static constexpr int32_t SLEEP_SECONDS = 1;

static std::vector<std::string> createTableSqlLists = {
    PhotoAlbumColumns::CREATE_TABLE,
    PhotoUpgrade::CREATE_PHOTO_TABLE,
};

static std::vector<std::string> testTables = {
    PhotoAlbumColumns::TABLE,
    PhotoColumn::PHOTOS_TABLE,
};

void ChangeRequestAbsorbAlbumTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStoreAbsorb = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStoreAbsorb, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStoreAbsorb, createTableSqlLists);
    MEDIA_INFO_LOG("ChangeRequestAbsorbAlbumTest SetUpTestCase");
}

void ChangeRequestAbsorbAlbumTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStoreAbsorb, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("ChangeRequestAbsorbAlbumTest TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void ChangeRequestAbsorbAlbumTest::SetUp()
{
    uint64_t tokenId = 0;
    PermissionUtilsUnitTest::SetAccessTokenPermission("ChangeRequestAbsorbAlbumTest",
        { PERM_READ_IMAGEVIDEO, PERM_WRITE_IMAGEVIDEO }, tokenId);
    ASSERT_NE(tokenId, 0);
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStoreAbsorb, testTables);
    MEDIA_INFO_LOG("ChangeRequestAbsorbAlbumTest SetUp");
}

void ChangeRequestAbsorbAlbumTest::TearDown(void) {}

static int32_t CreateAlbumWithHidden(int32_t albumType, int32_t albumSubType, const string &name,
    int32_t hidden, int32_t count)
{
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_TYPE, albumType);
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_SUBTYPE, albumSubType);
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_NAME, name);
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_HIDDEN, hidden);
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_COUNT, count);
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_LPATH, "/Pictures/" + name);
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, 0);

    int64_t rowId = -1;
    int32_t ret = g_rdbStoreAbsorb->Insert(rowId, PhotoAlbumColumns::TABLE, valuesBucket);
    if (ret != NativeRdb::E_OK || rowId <= 0) {
        MEDIA_ERR_LOG("Failed to insert album, ret: %{public}d, rowId: %{public}" PRId64, ret, rowId);
        return E_HAS_DB_ERROR;
    }
    return static_cast<int32_t>(rowId);
}

static int32_t ServiceAbsorbAlbum(const ChangeRequestAbsorbAlbumReqBody &reqBody)
{
    MessageParcel data;
    if (!reqBody.Marshalling(data)) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAlbumsControllerService>();
    service->ChangeRequestAbsorbAlbum(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> respVo;
    if (!respVo.Unmarshalling(reply)) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    return respVo.GetErrCode();
}

static int32_t QueryAlbumHidden(int32_t albumId)
{
    std::vector<std::string> columns = {PhotoAlbumColumns::ALBUM_HIDDEN};
    NativeRdb::RdbPredicates rdbPredicates(PhotoAlbumColumns::TABLE);
    rdbPredicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get album hidden");
        return -1;
    }
    int32_t hidden = GetInt32Val(PhotoAlbumColumns::ALBUM_HIDDEN, resultSet);
    resultSet->Close();
    return hidden;
}

static int32_t QueryAlbumCount(int32_t albumId)
{
    std::vector<std::string> columns = {PhotoAlbumColumns::ALBUM_COUNT};
    NativeRdb::RdbPredicates rdbPredicates(PhotoAlbumColumns::TABLE);
    rdbPredicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get album count");
        return -1;
    }
    int32_t count = GetInt32Val(PhotoAlbumColumns::ALBUM_COUNT, resultSet);
    resultSet->Close();
    return count;
}

static bool AlbumExists(int32_t albumId)
{
    std::vector<std::string> columns = {PhotoAlbumColumns::ALBUM_ID};
    NativeRdb::RdbPredicates rdbPredicates(PhotoAlbumColumns::TABLE);
    rdbPredicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    bool exists = (resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK);
    if (resultSet) {
        resultSet->Close();
    }
    return exists;
}

HWTEST_F(ChangeRequestAbsorbAlbumTest, AbsorbAlbum_Vo_MarshallingUnmarshalling_RoundTrip, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start AbsorbAlbum_Vo_MarshallingUnmarshalling_RoundTrip");
    ChangeRequestAbsorbAlbumReqBody reqBody;
    reqBody.albumId = 100;
    reqBody.absorbAlbumId = 200;

    MessageParcel data;
    ASSERT_TRUE(reqBody.Marshalling(data));

    ChangeRequestAbsorbAlbumReqBody out;
    ASSERT_TRUE(out.Unmarshalling(data));
    EXPECT_EQ(out.albumId, 100);
    EXPECT_EQ(out.absorbAlbumId, 200);
    MEDIA_INFO_LOG("End AbsorbAlbum_Vo_MarshallingUnmarshalling_RoundTrip");
}

HWTEST_F(ChangeRequestAbsorbAlbumTest, AbsorbAlbum_Vo_DefaultValues, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start AbsorbAlbum_Vo_DefaultValues");
    ChangeRequestAbsorbAlbumReqBody reqBody;
    MessageParcel data;
    ASSERT_TRUE(reqBody.Marshalling(data));

    ChangeRequestAbsorbAlbumReqBody out;
    ASSERT_TRUE(out.Unmarshalling(data));
    EXPECT_EQ(out.albumId, -1);
    EXPECT_EQ(out.absorbAlbumId, -1);
    MEDIA_INFO_LOG("End AbsorbAlbum_Vo_DefaultValues");
}

HWTEST_F(ChangeRequestAbsorbAlbumTest, AbsorbAlbum_InvalidAlbumId_Negative_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start AbsorbAlbum_InvalidAlbumId_Negative_001");
    ChangeRequestAbsorbAlbumReqBody reqBody;
    reqBody.albumId = -1;
    reqBody.absorbAlbumId = 100;

    EXPECT_EQ(ServiceAbsorbAlbum(reqBody), E_INVALID_VALUES);
    MEDIA_INFO_LOG("End AbsorbAlbum_InvalidAlbumId_Negative_001");
}

HWTEST_F(ChangeRequestAbsorbAlbumTest, AbsorbAlbum_InvalidAbsorbAlbumId_Negative_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start AbsorbAlbum_InvalidAbsorbAlbumId_Negative_001");
    ChangeRequestAbsorbAlbumReqBody reqBody;
    reqBody.albumId = 100;
    reqBody.absorbAlbumId = -1;

    EXPECT_EQ(ServiceAbsorbAlbum(reqBody), E_INVALID_VALUES);
    MEDIA_INFO_LOG("End AbsorbAlbum_InvalidAbsorbAlbumId_Negative_001");
}

HWTEST_F(ChangeRequestAbsorbAlbumTest, AbsorbAlbum_BothAlbumIdsNegative_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start AbsorbAlbum_BothAlbumIdsNegative_001");
    ChangeRequestAbsorbAlbumReqBody reqBody;
    reqBody.albumId = -1;
    reqBody.absorbAlbumId = -1;

    EXPECT_EQ(ServiceAbsorbAlbum(reqBody), E_INVALID_VALUES);
    MEDIA_INFO_LOG("End AbsorbAlbum_BothAlbumIdsNegative_001");
}

HWTEST_F(ChangeRequestAbsorbAlbumTest, AbsorbAlbum_NonExistentAlbumId_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start AbsorbAlbum_NonExistentAlbumId_001");
    ChangeRequestAbsorbAlbumReqBody reqBody;
    reqBody.albumId = 99999;
    reqBody.absorbAlbumId = 99998;

    EXPECT_EQ(ServiceAbsorbAlbum(reqBody), E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("End AbsorbAlbum_NonExistentAlbumId_001");
}

HWTEST_F(ChangeRequestAbsorbAlbumTest, AbsorbAlbum_UserAlbum_Normal_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start AbsorbAlbum_UserAlbum_Normal_001");
    int32_t emptyAlbumId = CreateAlbumWithHidden(
        PhotoAlbumType::USER, PhotoAlbumSubType::USER_GENERIC, "EmptyAlbum", 1, 0);
    ASSERT_GT(emptyAlbumId, 0);

    int32_t sourceAlbumId = CreateAlbumWithHidden(
        PhotoAlbumType::USER, PhotoAlbumSubType::USER_GENERIC, "SourceAlbum", 0, 5);
    ASSERT_GT(sourceAlbumId, 0);

    ChangeRequestAbsorbAlbumReqBody reqBody;
    reqBody.albumId = emptyAlbumId;
    reqBody.absorbAlbumId = sourceAlbumId;

    int32_t ret = ServiceAbsorbAlbum(reqBody);
    EXPECT_EQ(ret, E_OK);

    int32_t hidden = QueryAlbumHidden(emptyAlbumId);
    EXPECT_EQ(hidden, 0);

    int32_t count = QueryAlbumCount(emptyAlbumId);
    EXPECT_EQ(count, 5);

    EXPECT_FALSE(AlbumExists(sourceAlbumId));
    MEDIA_INFO_LOG("End AbsorbAlbum_UserAlbum_Normal_001");
}

HWTEST_F(ChangeRequestAbsorbAlbumTest, AbsorbAlbum_SameAlbumId_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start AbsorbAlbum_SameAlbumId_001");
    int32_t albumId = CreateAlbumWithHidden(
        PhotoAlbumType::USER, PhotoAlbumSubType::USER_GENERIC, "SameAlbum", 1, 0);
    ASSERT_GT(albumId, 0);

    ChangeRequestAbsorbAlbumReqBody reqBody;
    reqBody.albumId = albumId;
    reqBody.absorbAlbumId = albumId;

    EXPECT_EQ(ServiceAbsorbAlbum(reqBody), E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("End AbsorbAlbum_SameAlbumId_001");
}

HWTEST_F(ChangeRequestAbsorbAlbumTest, AbsorbAlbum_TypeMismatch_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start AbsorbAlbum_TypeMismatch_001");
    int32_t emptyUserAlbumId = CreateAlbumWithHidden(
        PhotoAlbumType::USER, PhotoAlbumSubType::USER_GENERIC, "EmptyUserAlbum", 1, 0);
    ASSERT_GT(emptyUserAlbumId, 0);

    int32_t sourceAlbumId = CreateAlbumWithHidden(
        PhotoAlbumType::SOURCE, PhotoAlbumSubType::SOURCE_GENERIC, "SourceAlbum", 0, 5);
    ASSERT_GT(sourceAlbumId, 0);

    ChangeRequestAbsorbAlbumReqBody reqBody;
    reqBody.albumId = emptyUserAlbumId;
    reqBody.absorbAlbumId = sourceAlbumId;

    EXPECT_EQ(ServiceAbsorbAlbum(reqBody), E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("End AbsorbAlbum_TypeMismatch_001");
}

HWTEST_F(ChangeRequestAbsorbAlbumTest, AbsorbAlbum_TargetAlbumNotHidden_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start AbsorbAlbum_TargetAlbumNotHidden_001");
    int32_t visibleAlbumId = CreateAlbumWithHidden(
        PhotoAlbumType::USER, PhotoAlbumSubType::USER_GENERIC, "VisibleAlbum", 0, 0);
    ASSERT_GT(visibleAlbumId, 0);

    int32_t sourceAlbumId = CreateAlbumWithHidden(
        PhotoAlbumType::USER, PhotoAlbumSubType::USER_GENERIC, "SourceAlbum", 0, 5);
    ASSERT_GT(sourceAlbumId, 0);

    ChangeRequestAbsorbAlbumReqBody reqBody;
    reqBody.albumId = visibleAlbumId;
    reqBody.absorbAlbumId = sourceAlbumId;

    EXPECT_EQ(ServiceAbsorbAlbum(reqBody), E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("End AbsorbAlbum_TargetAlbumNotHidden_001");
}

HWTEST_F(ChangeRequestAbsorbAlbumTest, AbsorbAlbum_TargetAlbumNotEmpty_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start AbsorbAlbum_TargetAlbumNotEmpty_001");
    int32_t nonEmptyAlbumId = CreateAlbumWithHidden(
        PhotoAlbumType::USER, PhotoAlbumSubType::USER_GENERIC, "NonEmptyAlbum", 1, 3);
    ASSERT_GT(nonEmptyAlbumId, 0);

    int32_t sourceAlbumId = CreateAlbumWithHidden(
        PhotoAlbumType::USER, PhotoAlbumSubType::USER_GENERIC, "SourceAlbum", 0, 5);
    ASSERT_GT(sourceAlbumId, 0);

    ChangeRequestAbsorbAlbumReqBody reqBody;
    reqBody.albumId = nonEmptyAlbumId;
    reqBody.absorbAlbumId = sourceAlbumId;

    EXPECT_EQ(ServiceAbsorbAlbum(reqBody), E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("End AbsorbAlbum_TargetAlbumNotEmpty_001");
}

HWTEST_F(ChangeRequestAbsorbAlbumTest, AbsorbAlbum_SourceAlbumAlreadyHidden_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start AbsorbAlbum_SourceAlbumAlreadyHidden_001");
    int32_t emptyAlbumId = CreateAlbumWithHidden(
        PhotoAlbumType::USER, PhotoAlbumSubType::USER_GENERIC, "EmptyAlbum", 1, 0);
    ASSERT_GT(emptyAlbumId, 0);

    int32_t hiddenAlbumId = CreateAlbumWithHidden(
        PhotoAlbumType::USER, PhotoAlbumSubType::USER_GENERIC, "HiddenAlbum", 1, 5);
    ASSERT_GT(hiddenAlbumId, 0);

    ChangeRequestAbsorbAlbumReqBody reqBody;
    reqBody.albumId = emptyAlbumId;
    reqBody.absorbAlbumId = hiddenAlbumId;

    EXPECT_EQ(ServiceAbsorbAlbum(reqBody), E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("End AbsorbAlbum_SourceAlbumAlreadyHidden_001");
}

} // namespace OHOS::Media
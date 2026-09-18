/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define MLOG_TAG "SetShareAlbumNameTest"

#include "set_share_album_name_test.h"

#include <cerrno>
#include <memory>
#include <string>
#include <vector>

#include "media_albums_controller_service.h"
#include "medialibrary_album_operations.h"
#include "medialibrary_errno.h"
#include "media_file_utils.h"

#include "set_share_album_name_vo.h"
#include "media_empty_obj_vo.h"
#include "media_resp_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "test_data_builder.h"
#include "rdb_predicates.h"
#include "media_upgrade.h"
#include "photo_album_column.h"
#include "userfile_manager_types.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace IPC;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;

static constexpr int32_t SHARE_ALBUM_ID = 1001;
static constexpr int32_t USER_ALBUM_ID = 1002;
static constexpr int32_t NOT_EXIST_ALBUM_ID = 99999999;

static const string TEST_OWNER = "test_owner";
static const string OTHER_OWNER = "other_owner";
static const string ORIGINAL_ALBUM_NAME = "TestShareAlbum";
static const string NEW_ALBUM_NAME = "TestShareAlbumRenamed";

static std::vector<std::string> createTableSqlLists = {
    PhotoAlbumColumns::CREATE_TABLE,
};

static std::vector<std::string> testTables = {
    PhotoAlbumColumns::TABLE,
};

static int32_t InsertAlbum(int32_t albumId, int32_t albumType, int32_t albumSubType,
    const string &albumName, const string &owner = "")
{
    ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_ID, albumId);
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, albumType);
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, albumSubType);
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, albumName);
    if (!owner.empty()) {
        values.PutString(PhotoAlbumColumns::SHARE_ALBUM_OWNER, owner);
    }
    int64_t rowId = 0;
    int32_t ret = g_rdbStore->Insert(rowId, PhotoAlbumColumns::TABLE, values);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("InsertAlbum failed, ret=%{public}d", ret);
        return -1;
    }
    return static_cast<int32_t>(rowId);
}

static void ClearAlbumTable(void)
{
    int32_t deleted = 0;
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    g_rdbStore->Delete(deleted, predicates);
}

static string QueryAlbumName(int32_t albumId)
{
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    auto resultSet = g_rdbStore->Query(predicates, { PhotoAlbumColumns::ALBUM_NAME });
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("QueryAlbumName failed, albumId=%{public}d", albumId);
        return "";
    }
    int32_t columnIndex = -1;
    resultSet->GetColumnIndex(PhotoAlbumColumns::ALBUM_NAME, columnIndex);
    string albumName;
    resultSet->GetString(columnIndex, albumName);
    resultSet->Close();
    return albumName;
}

void SetShareAlbumNameTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::InitUnistore();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
}

void SetShareAlbumNameTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryUnitTestUtils::StopUnistore();
    g_rdbStore = nullptr;
}

HWTEST_F(SetShareAlbumNameTest, SetShareAlbumName_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("SetShareAlbumName_001 start");

    MessageParcel data;
    MessageParcel reply;

    auto service = make_shared<MediaAlbumsControllerService>();
    service->SetShareAlbumName(data, reply);

    MediaRespVo<MediaEmptyObjVo> respVo;
    EXPECT_TRUE(respVo.Unmarshalling(reply));
    EXPECT_EQ(respVo.GetErrCode(), E_IPC_SEVICE_UNMARSHALLING_FAIL);

    MEDIA_INFO_LOG("SetShareAlbumName_001 end, ret=%{public}d", respVo.GetErrCode());
}

HWTEST_F(SetShareAlbumNameTest, SetShareAlbumName_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("SetShareAlbumName_002 start");

    SetShareAlbumNameReqBody reqBody;
    reqBody.albumId = 0;
    reqBody.owner = TEST_OWNER;
    reqBody.albumName = NEW_ALBUM_NAME;

    MessageParcel data;
    MessageParcel reply;
    ASSERT_TRUE(reqBody.Marshalling(data));

    auto service = make_shared<MediaAlbumsControllerService>();
    service->SetShareAlbumName(data, reply);

    MediaRespVo<MediaEmptyObjVo> respVo;
    EXPECT_TRUE(respVo.Unmarshalling(reply));
    EXPECT_EQ(respVo.GetErrCode(), E_INVALID_VALUES);

    MEDIA_INFO_LOG("SetShareAlbumName_002 end, ret=%{public}d", respVo.GetErrCode());
}

HWTEST_F(SetShareAlbumNameTest, SetShareAlbumName_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("SetShareAlbumName_003 start");

    SetShareAlbumNameReqBody reqBody;
    reqBody.albumId = SHARE_ALBUM_ID;
    reqBody.owner = "";
    reqBody.albumName = NEW_ALBUM_NAME;

    MessageParcel data;
    MessageParcel reply;
    ASSERT_TRUE(reqBody.Marshalling(data));

    auto service = make_shared<MediaAlbumsControllerService>();
    service->SetShareAlbumName(data, reply);

    MediaRespVo<MediaEmptyObjVo> respVo;
    EXPECT_TRUE(respVo.Unmarshalling(reply));
    EXPECT_EQ(respVo.GetErrCode(), E_INVALID_VALUES);

    MEDIA_INFO_LOG("SetShareAlbumName_003 end, ret=%{public}d", respVo.GetErrCode());
}

HWTEST_F(SetShareAlbumNameTest, SetShareAlbumName_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("SetShareAlbumName_004 start");

    SetShareAlbumNameReqBody reqBody;
    reqBody.albumId = SHARE_ALBUM_ID;
    reqBody.owner = TEST_OWNER;
    reqBody.albumName = "";

    MessageParcel data;
    MessageParcel reply;
    ASSERT_TRUE(reqBody.Marshalling(data));

    auto service = make_shared<MediaAlbumsControllerService>();
    service->SetShareAlbumName(data, reply);

    MediaRespVo<MediaEmptyObjVo> respVo;
    EXPECT_TRUE(respVo.Unmarshalling(reply));
    EXPECT_EQ(respVo.GetErrCode(), E_INVALID_VALUES);

    MEDIA_INFO_LOG("SetShareAlbumName_004 end, ret=%{public}d", respVo.GetErrCode());
}

HWTEST_F(SetShareAlbumNameTest, SetShareAlbumName_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("SetShareAlbumName_005 start");

    ASSERT_NE(g_rdbStore, nullptr);
    ClearAlbumTable();
    int32_t shareAlbumId = InsertAlbum(SHARE_ALBUM_ID,
        static_cast<int32_t>(PhotoAlbumType::SHARE),
        static_cast<int32_t>(PhotoAlbumSubType::SHARE_GENERIC),
        ORIGINAL_ALBUM_NAME, TEST_OWNER);
    ASSERT_GT(shareAlbumId, 0);

    SetShareAlbumNameReqBody reqBody;
    reqBody.albumId = SHARE_ALBUM_ID;
    reqBody.owner = TEST_OWNER;
    reqBody.albumName = NEW_ALBUM_NAME;

    MessageParcel data;
    MessageParcel reply;
    ASSERT_TRUE(reqBody.Marshalling(data));

    auto service = make_shared<MediaAlbumsControllerService>();
    service->SetShareAlbumName(data, reply);

    MediaRespVo<MediaEmptyObjVo> respVo;
    EXPECT_TRUE(respVo.Unmarshalling(reply));
    EXPECT_EQ(respVo.GetErrCode(), E_OK);

    MEDIA_INFO_LOG("SetShareAlbumName_005 end, ret=%{public}d", respVo.GetErrCode());
}

HWTEST_F(SetShareAlbumNameTest, SetShareAlbumName_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("SetShareAlbumName_006 start");

    ASSERT_NE(g_rdbStore, nullptr);
    ClearAlbumTable();
    int32_t shareAlbumId = InsertAlbum(SHARE_ALBUM_ID,
        static_cast<int32_t>(PhotoAlbumType::SHARE),
        static_cast<int32_t>(PhotoAlbumSubType::SHARE_GENERIC),
        ORIGINAL_ALBUM_NAME, TEST_OWNER);
    ASSERT_GT(shareAlbumId, 0);

    SetShareAlbumNameReqBody reqBody;
    reqBody.albumId = SHARE_ALBUM_ID;
    reqBody.owner = OTHER_OWNER;
    reqBody.albumName = NEW_ALBUM_NAME;

    MessageParcel data;
    MessageParcel reply;
    ASSERT_TRUE(reqBody.Marshalling(data));

    auto service = make_shared<MediaAlbumsControllerService>();
    service->SetShareAlbumName(data, reply);

    MediaRespVo<MediaEmptyObjVo> respVo;
    EXPECT_TRUE(respVo.Unmarshalling(reply));
    EXPECT_EQ(respVo.GetErrCode(), E_SHARE_ALBUM_INVALID_ID_ARG);

    MEDIA_INFO_LOG("SetShareAlbumName_006 end, ret=%{public}d", respVo.GetErrCode());
}

HWTEST_F(SetShareAlbumNameTest, SetShareAlbumName_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("SetShareAlbumName_007 start");

    ASSERT_NE(g_rdbStore, nullptr);
    ClearAlbumTable();
    int32_t shareAlbumId = InsertAlbum(SHARE_ALBUM_ID,
        static_cast<int32_t>(PhotoAlbumType::SHARE),
        static_cast<int32_t>(PhotoAlbumSubType::SHARE_GENERIC),
        ORIGINAL_ALBUM_NAME, TEST_OWNER);
    ASSERT_GT(shareAlbumId, 0);

    int32_t ret = MediaLibraryAlbumOperations::SetShareAlbumName(SHARE_ALBUM_ID, TEST_OWNER, NEW_ALBUM_NAME);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(QueryAlbumName(SHARE_ALBUM_ID), NEW_ALBUM_NAME);

    MEDIA_INFO_LOG("SetShareAlbumName_007 end, ret=%{public}d", ret);
}

HWTEST_F(SetShareAlbumNameTest, SetShareAlbumName_008, TestSize.Level0)
{
    MEDIA_INFO_LOG("SetShareAlbumName_008 start");

    int32_t ret = MediaLibraryAlbumOperations::SetShareAlbumName(0, TEST_OWNER, NEW_ALBUM_NAME);
    EXPECT_EQ(ret, -EINVAL);

    MEDIA_INFO_LOG("SetShareAlbumName_008 end, ret=%{public}d", ret);
}

HWTEST_F(SetShareAlbumNameTest, SetShareAlbumName_009, TestSize.Level0)
{
    MEDIA_INFO_LOG("SetShareAlbumName_009 start");

    int32_t ret = MediaLibraryAlbumOperations::SetShareAlbumName(SHARE_ALBUM_ID, "", NEW_ALBUM_NAME);
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);

    MEDIA_INFO_LOG("SetShareAlbumName_009 end, ret=%{public}d", ret);
}

HWTEST_F(SetShareAlbumNameTest, SetShareAlbumName_010, TestSize.Level0)
{
    MEDIA_INFO_LOG("SetShareAlbumName_010 start");

    const string INVALID_ALBUM_NAME = "Invalid/Name";
    int32_t ret = MediaLibraryAlbumOperations::SetShareAlbumName(SHARE_ALBUM_ID, TEST_OWNER, INVALID_ALBUM_NAME);
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);

    MEDIA_INFO_LOG("SetShareAlbumName_010 end, ret=%{public}d", ret);
}

HWTEST_F(SetShareAlbumNameTest, SetShareAlbumName_011, TestSize.Level0)
{
    MEDIA_INFO_LOG("SetShareAlbumName_011 start");

    ASSERT_NE(g_rdbStore, nullptr);
    ClearAlbumTable();
    int32_t shareAlbumId = InsertAlbum(SHARE_ALBUM_ID,
        static_cast<int32_t>(PhotoAlbumType::SHARE),
        static_cast<int32_t>(PhotoAlbumSubType::SHARE_GENERIC),
        ORIGINAL_ALBUM_NAME, TEST_OWNER);
    ASSERT_GT(shareAlbumId, 0);

    int32_t ret = MediaLibraryAlbumOperations::SetShareAlbumName(SHARE_ALBUM_ID, OTHER_OWNER, NEW_ALBUM_NAME);
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);
    EXPECT_EQ(QueryAlbumName(SHARE_ALBUM_ID), ORIGINAL_ALBUM_NAME);

    MEDIA_INFO_LOG("SetShareAlbumName_011 end, ret=%{public}d", ret);
}

HWTEST_F(SetShareAlbumNameTest, SetShareAlbumName_012, TestSize.Level0)
{
    MEDIA_INFO_LOG("SetShareAlbumName_012 start");

    ASSERT_NE(g_rdbStore, nullptr);
    ClearAlbumTable();

    int32_t ret = MediaLibraryAlbumOperations::SetShareAlbumName(NOT_EXIST_ALBUM_ID, TEST_OWNER, NEW_ALBUM_NAME);
    EXPECT_EQ(ret, E_SHARE_ALBUM_INVALID_ID_ARG);

    MEDIA_INFO_LOG("SetShareAlbumName_012 end, ret=%{public}d", ret);
}
} // namespace OHOS::Media

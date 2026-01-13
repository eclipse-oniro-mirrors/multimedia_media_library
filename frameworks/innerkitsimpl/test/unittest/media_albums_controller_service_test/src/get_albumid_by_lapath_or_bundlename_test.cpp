/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "get_albumid_by_lapath_or_bundlename_test.h"

#include <memory>
#include <string>

#include "media_albums_controller_service.h"

#include "get_albumid_by_lpath_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace IPC;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;

static std::vector<std::string> createTableSqlLists = {
    PhotoAlbumColumns::CREATE_TABLE,
};

static std::vector<std::string> testTables = {
    PhotoAlbumColumns::TABLE,
};

void GetAlbumIdByLpathOrBundleNameTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    auto ret = MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    ASSERT_EQ(ret, true);
    MEDIA_INFO_LOG("SetRelationshipTest SetUpTestCase");
}

void GetAlbumIdByLpathOrBundleNameTest::TearDownTestCase(void)
{
    auto ret = MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    ASSERT_EQ(ret, true);
    MEDIA_INFO_LOG(" SetRelationshipTest TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void GetAlbumIdByLpathOrBundleNameTest::SetUp()
{
    MEDIA_INFO_LOG("SetRelationshipTest SetUp");
    auto ret = MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
    ASSERT_EQ(ret, true);
}

void GetAlbumIdByLpathOrBundleNameTest::TearDown(void) {}

static const string SQL_CREATE_ALBUM = "INSERT INTO " + PhotoAlbumColumns::TABLE + "(" +
    PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
    PhotoAlbumColumns::ALBUM_LPATH + ")";

static int32_t InsertAlbumInfo(int32_t albumId, const std::string &lpath)
{
    int32_t ret = g_rdbStore->ExecuteSql(SQL_CREATE_ALBUM + " VALUES (" +
        to_string(albumId) + ", 0, '" + lpath + "')");
    return ret;
}

HWTEST_F(GetAlbumIdByLpathOrBundleNameTest, GetAlbumIdByLpathOrBundleName_001, TestSize.Level0) {
    MEDIA_INFO_LOG("GetAlbumIdByLpathOrBundleName_001 Start");
    MessageParcel data;
    MessageParcel reply;

    auto service = make_shared<MediaAlbumsControllerService>();
    service->GetAlbumIdByLpathOrBundleName(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);
    ASSERT_LT(respVo.GetErrCode(), 0);
    MEDIA_INFO_LOG("GetAlbumIdByLpathOrBundleName_001 end");
}

HWTEST_F(GetAlbumIdByLpathOrBundleNameTest, GetAlbumIdByLpathOrBundleName_002, TestSize.Level0) {
    MEDIA_INFO_LOG("GetAlbumIdByLpathOrBundleName_002 Start");

    MessageParcel data;
    MessageParcel reply;
    const int32_t testAlbumId = 100;
    const string testLpath = "/test/album/100";
    int32_t insertRet = InsertAlbumInfo(testAlbumId, testLpath);
    ASSERT_EQ(insertRet, E_OK);
    GetAlbumIdByLpathReqBody reqBody;
    reqBody.predicates.EqualTo(PhotoAlbumColumns::ALBUM_LPATH, testLpath);
    reqBody.columns = {PhotoAlbumColumns::ALBUM_ID};

    bool marshalRet = reqBody.Marshalling(data);
    ASSERT_EQ(marshalRet, true);

    auto service = make_shared<MediaAlbumsControllerService>();
    service->GetAlbumIdByLpathOrBundleName(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);
    EXPECT_EQ(respVo.GetErrCode(), 0);
    MEDIA_INFO_LOG("GetAlbumIdByLpathOrBundleName_002 end");
}
} // namespace OHOS::Media
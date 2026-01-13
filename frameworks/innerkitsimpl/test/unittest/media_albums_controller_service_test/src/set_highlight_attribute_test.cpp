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

#include "set_highlight_attribute_test.h"

#include <string>
#include <vector>

#include "media_albums_controller_service.h"

#include "change_request_set_highlight_attribute_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "result_set_utils.h"
#include "medialibrary_business_code.h"
#include "story_album_column.h"
#include "story_db_sqls.h"
#include "media_upgrade.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace IPC;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;

static std::vector<std::string> createTableSqlLists = {
    PhotoUpgrade::CREATE_PHOTO_TABLE,
    PhotoAlbumColumns::CREATE_TABLE,
    CREATE_HIGHLIGHT_ALBUM_TABLE,
};

static std::vector<std::string> testTables = {
    PhotoColumn::PHOTOS_TABLE,
    PhotoAlbumColumns::TABLE,
    HIGHLIGHT_ALBUM_TABLE,
};

void SetHighlightAttributeTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    MEDIA_INFO_LOG("SetHighlightAttributeTest SetUpTestCase");
}

void SetHighlightAttributeTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("SetHighlightAttributeTest TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void SetHighlightAttributeTest::SetUp()
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
    MEDIA_INFO_LOG("SetHighlightAttributeTest SetUp");
}

void SetHighlightAttributeTest::TearDown() {}

int32_t InsertValueHighlightAlbum()
{
    std::string insertSql = " \
        INSERT INTO tab_highlight_album \
        (album_id, cluster_type, cluster_sub_type, cluster_condition, highlight_version, \
        is_favorite, is_viewed, notification_time) \
        VALUES (1, 'TYPE_LIFE_STAGE', 'Graduate', ?, 0, 0, 0, 0);";
    std::string clusterCondition =
        R"([{"end": "1244081480000", "group_tag": "ser_1755585091890809000", \
        "locationType": "COLLEGE_UNIVERSITY", "start": "1117851080000"}])";
    std::vector<NativeRdb::ValueObject> params = {};
    params.push_back(NativeRdb::ValueObject(clusterCondition));
    EXPECT_NE(g_rdbStore, nullptr);
    int32_t ret = g_rdbStore->ExecuteSql(insertSql, params);
    return ret;
}

HWTEST_F(SetHighlightAttributeTest, SetHighlightAttribute_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetHighlightAttribute_Test_001");
    InsertValueHighlightAlbum();
    ChangeRequestSetHighlightAttributeReqBody reqBody;
    reqBody.albumId = 1;
    reqBody.highlightAlbumChangeAttribute = 0;
    reqBody.highlightAlbumChangeAttributeValue = "1";
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SMART);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::HIGHLIGHT);

    MessageParcel data;
    bool result = reqBody.Marshalling(data);
    ASSERT_NE(result, false);

    MessageParcel reply;
    auto service = make_shared<MediaAlbumsControllerService>();
    service->ChangeRequestSetHighlightAttribute(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> respVo;
    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    int32_t errCode = respVo.GetErrCode();
    EXPECT_EQ(errCode, E_OK);
    MEDIA_INFO_LOG("end SetHighlightAttribute_Test_001");
}

HWTEST_F(SetHighlightAttributeTest, SetHighlightAttribute_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetHighlightAttribute_Test_002");
    MessageParcel data;
    MessageParcel reply;
    auto service = make_shared<MediaAlbumsControllerService>();
    service->ChangeRequestSetHighlightAttribute(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> resp;
    ASSERT_EQ(resp.Unmarshalling(reply), true);
    ASSERT_LT(resp.GetErrCode(), E_OK);
    MEDIA_INFO_LOG("end SetHighlightAttribute_Test_002");
}
} // namespace OHOS::Media
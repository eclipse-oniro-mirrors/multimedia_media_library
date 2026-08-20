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

#include "create_analysis_album_test.h"

#include <memory>
#include <string>

#include "media_analysis_data_controller_service.h"

#include "create_analysis_album_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "vision_db_sqls_more.h"
#include "userfile_manager_types.h"
#include "photo_album_column.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;

static std::vector<std::string> createTableSqlLists = {
    CREATE_ANALYSIS_ALBUM_FOR_ONCREATE,
};

static std::vector<std::string> testTables = {
    "AnalysisAlbum",
};

void CreateAnalysisAlbumTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    MEDIA_INFO_LOG("CreateAnalysisAlbumTest SetUpTestCase");
}

void CreateAnalysisAlbumTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("CreateAnalysisAlbumTest TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void CreateAnalysisAlbumTest::SetUp()
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
    MEDIA_INFO_LOG("CreateAnalysisAlbumTest SetUp");
}

void CreateAnalysisAlbumTest::TearDown(void) {}

bool CheckAnalysisAlbum(string albumName)
{
    int32_t count = 0;
    NativeRdb::RdbPredicates rdbPredicate("AnalysisAlbum");
    rdbPredicate.EqualTo(PhotoAlbumColumns::ALBUM_NAME, albumName);
    vector<string> columns = { PhotoAlbumColumns::ALBUM_NAME };
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    if (resultSet != nullptr) {
        resultSet->GetRowCount(count);
        resultSet->Close();
    }
    return count == 1;
}

int32_t ServiceCreateAnalysisAlbum(const std::string &albumName, const int32_t &subType)
{
    CreateAnalysisAlbumReqBody reqBody;
    reqBody.albumName = albumName;
    reqBody.subType = subType;

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<AnalysisData::MediaAnalysisDataControllerService>();
    int32_t ret = service->CreateAnalysisAlbum(data, reply);

    return ret;
}

HWTEST_F(CreateAnalysisAlbumTest, CreateAnalysisAlbum_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CreateAnalysisAlbum_Test_001");
    int32_t result = ServiceCreateAnalysisAlbum("CreateAnalysisAlbum_Test_001.XXX", PORTRAIT);
    ASSERT_LT(result, 0);

    MEDIA_INFO_LOG("End CreateAnalysisAlbum_Test_001");
}

HWTEST_F(CreateAnalysisAlbumTest, CreateAnalysisAlbum_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start CreateAnalysisAlbum_Test_002");
    int32_t result = ServiceCreateAnalysisAlbum("Album_Test_002", SYSTEM_START);
    ASSERT_LT(result, 0);
    MEDIA_INFO_LOG("End CreateAnalysisAlbum_Test_002");
}

HWTEST_F(CreateAnalysisAlbumTest, CreateAnalysisAlbum_Test_003, TestSize.Level0) {
    MEDIA_INFO_LOG("Start CreateAnalysisAlbum_Test_003");
    int32_t result = ServiceCreateAnalysisAlbum("Album_Test_003", PORTRAIT);
    ASSERT_EQ(result, 0);
    bool hasAlbum = CheckAnalysisAlbum("Album_Test_003");
    ASSERT_EQ(hasAlbum, true);
    MEDIA_INFO_LOG("End CreateAnalysisAlbum_Test_003");
}
}  // namespace OHOS::Media
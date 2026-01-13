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
 
#include "get_highlight_album_info_test.h"
 
#include <string>
#include <vector>
 
#define private public
#define protected public
#include "datashare_result_set.h"
#include "media_albums_controller_service.h"
#include "media_assets_service.h"
#include "rdb_utils.h"
#include "media_analysis_data_controller_service.h"
#undef private
#undef protected
 
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"
#include "get_highlight_album_info_vo.h"
#include "query_result_vo.h"
 
namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
 
static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;
 
static int32_t ClearTable(const string &table)
{
    RdbPredicates predicates(table);
 
    int32_t rows = 0;
    int32_t err = g_rdbStore->Delete(rows, predicates);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to clear album table, err: %{public}d", err);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}
 
void GetHightlightAlbumInfoTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start GetHightlightAlbumInfoTest failed, can not get g_rdbStore");
        exit(1);
    }
    MEDIA_INFO_LOG("SetUpTestCase");
}
 
void GetHightlightAlbumInfoTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}
 
void GetHightlightAlbumInfoTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}
 
void GetHightlightAlbumInfoTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}
 
HWTEST_F(GetHightlightAlbumInfoTest, GetHightlightAlbumInfoTest_Test_001, TestSize.Level0)
{
    GetHighlightAlbumReqBody reqBody;
    reqBody.highlightAlbumInfoType = PLAY_INFO;
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);

    auto service = make_shared<AnalysisData::MediaAnalysisDataControllerService>();
    service->GetHighlightAlbumInfo(data, reply);
    IPC::MediaRespVo<QueryResultRespBody> resp;
    bool isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    EXPECT_EQ(resp.GetErrCode(), E_ERR);
    EXPECT_EQ(resp.GetBody().resultSet, nullptr);
}
 
}  // namespace OHOS::Media
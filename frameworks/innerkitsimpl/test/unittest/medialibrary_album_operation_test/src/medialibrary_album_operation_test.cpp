/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MediaLibraryAlbumOperationTest"

#include "medialibrary_album_operation_test.h"
#include "datashare_result_set.h"
#include "photo_album_column.h"
#include "get_self_permissions.h"
#include "location_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_album_operations.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "result_set_utils.h"
#include "uri.h"
#include "vision_db_sqls_more.h"
#include "vision_portrait_nickname_column.h"
#include "album_operation_uri.h"
#include "asset_accurate_refresh.h"
#include "media_upgrade.h"
#include "analysis_album_operation_data_utils.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media::AccurateRefresh;

namespace OHOS {
namespace Media {
static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
constexpr int32_t FALSE_DISPLAY_LEVEL_VALUE = -1;
constexpr int32_t FALSE_ALBUM_ID = -2;
constexpr int32_t TRUE_ALBUM_ID = 1;
constexpr int32_t TURE_ALBUM_ID_TWO = 2;
constexpr int32_t TRUE_ALBUM_ID_THREE = 3;
constexpr int32_t TRUE_ALBUM_ID_FOUR = 4;
constexpr int32_t PIC_COUNT = 20;
constexpr int32_t ALBUM_CUR_COUNT = 10;
constexpr int32_t ALBUM_TARGET_COUNT = 10;
constexpr int32_t MERGED_GROUP_COMMON_COUNT = 5;
constexpr int32_t MERGED_GROUP_LAST_COUNT = 4;
constexpr int32_t FIRST_ALBUM_START_ASSET_ID = 1;
constexpr int32_t FIRST_ALBUM_END_ASSET_ID = 5;
constexpr int32_t SECOND_ALBUM_START_ASSET_ID = 6;
constexpr int32_t SECOND_ALBUM_END_ASSET_ID = 10;
constexpr int32_t THIRD_ALBUM_START_ASSET_ID = 11;
constexpr int32_t THIRD_ALBUM_END_ASSET_ID = 15;
constexpr int32_t FOURTH_ALBUM_START_ASSET_ID = 16;
constexpr int32_t FOURTH_ALBUM_END_ASSET_ID = 19;
constexpr int32_t IS_ME_VALUE = 1;
constexpr int32_t IS_NOT_ME_VALUE = 0;
constexpr int32_t RANK_ONE = 1;
constexpr int32_t RANK_TWO = 2;
constexpr int32_t WAIT_TIME = 3;
const string ME_RELATIONSHIP_VALUE = "me";
const string FAMILY_RELATIONSHIP_VALUE = "family";
struct AlbumColumn {
    string coverUri;
    int count;
    string tagId;
    string groupTag;
    string rank;
    int userOperation;
    int displayLevel;
    int isMe;
    int renameOperation;
    int isCoverSatisfied;
    string albumName;
    string relationship;
};

enum class TestNickNameChangeOperation {
    ADD,
    REMOVE,
};

struct TestNickNameOperationData {
    vector<string> addNickNames;
    vector<string> removeNickNames;
};

void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        ANALYSIS_ALBUM_TABLE,
        ANALYSIS_PHOTO_MAP_TABLE,
        ANALYSIS_NICK_NAME_TABLE,
    };
    for (auto &dropTable : dropTableList) {
        string dropSql = "DROP TABLE " + dropTable + ";";
        int32_t ret = g_rdbStore->ExecuteSql(dropSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Drop %{public}s table failed", dropTable.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Drop %{public}s table success", dropTable.c_str());
    }
}

void SetTables()
{
    vector<string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        CREATE_ANALYSIS_ALBUM_FOR_ONCREATE,
        CREATE_ANALYSIS_ALBUM_MAP,
        CREATE_ANALYSIS_NICK_NAME_TABLE,
        CREATE_ANALYSIS_NICK_NAME_UNIQUE_INDEX,
    };
    for (auto &createTableSql : createTableSqlList) {
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
}

void ClearAndRestart()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    system("rm -rf /storage/cloud/files/.thumbs");
    system("rm -rf /storage/cloud/files/.editData");
    for (const auto &dir : TEST_ROOT_DIRS) {
        string ROOT_PATH = "/storage/cloud/100/files/";
        bool ret = MediaFileUtils::CreateDirectory(ROOT_PATH + dir + "/");
        CHECK_AND_PRINT_LOG(ret, "make %{public}s dir failed, ret=%{public}d", dir.c_str(), ret);
    }
    CleanTestTables();
    SetTables();
}

void ClearAnalysisAlbum()
{
    auto rdbStore = MediaLibraryDataManager::GetInstance()->rdbStore_;
    NativeRdb::AbsRdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    predicates.NotEqualTo(ALBUM_SUBTYPE, PhotoAlbumSubType::SHOOTING_MODE);
    int32_t deletedRows = -1;
    auto ret = rdbStore->Delete(deletedRows, predicates);
    MEDIA_INFO_LOG("ClearAnalysisAlbum Delete retVal: %{public}d, deletedRows: %{public}d", ret, deletedRows);
}

void MediaLibraryAlbumOperationTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("Vision_Test::Start");
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    vector<string> perms = { "ohos.permission.MEDIA_LOCATION" };
    uint64_t tokenId = 0;
    PermissionUtilsUnitTest::SetAccessTokenPermission("MediaLibraryAlbumOperationTest", perms, tokenId);
    ASSERT_TRUE(tokenId != 0);
    ClearAnalysisAlbum();
    ClearAndRestart();
}

void MediaLibraryAlbumOperationTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("Vision_Test::End");
    std::this_thread::sleep_for(std::chrono::seconds(WAIT_TIME));
}

void MediaLibraryAlbumOperationTest::SetUp(void)
{
    MEDIA_INFO_LOG("SetUp");
    MediaLibraryUnitTestUtils::CleanTestFiles();
    MediaLibraryUnitTestUtils::CleanBundlePermission();
    MediaLibraryUnitTestUtils::InitRootDirs();
    MediaLibraryUnitTestUtils::Init();
    ClearAnalysisAlbum();
    ClearAndRestart();
}

void MediaLibraryAlbumOperationTest::TearDown(void) {}

HWTEST_F(MediaLibraryAlbumOperationTest, portrait_set_display_level_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("portrait_set_display_level_001::Start");
    Uri uri(CONST_PAH_PORTRAIT_DISPLAY_LEVLE);
    MediaLibraryCommand queryCmd(uri);
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(USER_DISPLAY_LEVEL, UNFAVORITE_PAGE);
    int result = MediaLibraryDataManager::GetInstance()->Update(queryCmd, valuesBucket, predicates);
    EXPECT_EQ(result, E_INVALID_VALUES);
    MEDIA_INFO_LOG("portrait_set_display_level_001 End, result:%{public}d", result);
}

int HandleAnalysisPhotoAlbumTest(const OperationType &opType)
{
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    return MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates);
}

HWTEST_F(MediaLibraryAlbumOperationTest, HandleAnalysisPhotoAlbum_Displaylevel, TestSize.Level1)
{
    MEDIA_INFO_LOG("HandleAnalysisPhotoAlbum_Displaylevel::Start");
    EXPECT_NE(HandleAnalysisPhotoAlbumTest(OperationType::PORTRAIT_DISPLAY_LEVEL), E_OK);
    MEDIA_INFO_LOG("HandleAnalysisPhotoAlbum_Displaylevel End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, HandleAnalysisPhotoAlbum_MergeAlbum, TestSize.Level1)
{
    MEDIA_INFO_LOG("HandleAnalysisPhotoAlbum_MergeAlbum::Start");
    EXPECT_NE(HandleAnalysisPhotoAlbumTest(OperationType::PORTRAIT_MERGE_ALBUM), E_OK);
    MEDIA_INFO_LOG("HandleAnalysisPhotoAlbum_MergeAlbum End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, HandleAnalysisPhotoAlbum_SetIsMe, TestSize.Level1)
{
    MEDIA_INFO_LOG("HandleAnalysisPhotoAlbum_SetIsMe::Start");
    EXPECT_NE(HandleAnalysisPhotoAlbumTest(OperationType::PORTRAIT_IS_ME), E_OK);
    MEDIA_INFO_LOG("HandleAnalysisPhotoAlbum_SetIsMe End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, HandleAnalysisPhotoAlbum_SetAlbumName, TestSize.Level1)
{
    MEDIA_INFO_LOG("HandleAnalysisPhotoAlbum_SetAlbumName::Start");
    EXPECT_NE(HandleAnalysisPhotoAlbumTest(OperationType::PORTRAIT_ALBUM_NAME), E_OK);
    MEDIA_INFO_LOG("HandleAnalysisPhotoAlbum_SetAlbumName End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, HandleAnalysisPhotoAlbum_SetCoverUri, TestSize.Level1)
{
    MEDIA_INFO_LOG("HandleAnalysisPhotoAlbum_SetCoverUri::Start");
    EXPECT_NE(HandleAnalysisPhotoAlbumTest(OperationType::PORTRAIT_COVER_URI), E_OK);
    MEDIA_INFO_LOG("HandleAnalysisPhotoAlbum_SetCoverUri End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, HandleAnalysisPhotoAlbum_Others, TestSize.Level1)
{
    MEDIA_INFO_LOG("HandleAnalysisPhotoAlbum_Others::Start");
    EXPECT_NE(HandleAnalysisPhotoAlbumTest(OperationType::UNKNOWN_TYPE), E_OK);
    MEDIA_INFO_LOG("HandleAnalysisPhotoAlbum_Others End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetDisplaylevel_CheckDisplayLevel_false, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetDisplaylevel_CheckDisplayLevel_false::Start");
    NativeRdb::ValuesBucket values;
    values.Put(USER_DISPLAY_LEVEL, FALSE_DISPLAY_LEVEL_VALUE);
    DataShare::DataSharePredicates predicates;
    OperationType opType = OperationType::PORTRAIT_DISPLAY_LEVEL;
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("SetDisplaylevel_CheckDisplayLevel_false End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetDisplaylevel_WhereArgs_Size_0, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetDisplaylevel_WhereArgs_Size_0::Start");
    NativeRdb::ValuesBucket values;
    values.Put(USER_DISPLAY_LEVEL, FIRST_PAGE);
    DataShare::DataSharePredicates predicates;
    OperationType opType = OperationType::PORTRAIT_DISPLAY_LEVEL;
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("SetDisplaylevel_WhereArgs_Size_0 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetDisplaylevel_AlbumId_less_0, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetDisplaylevel_AlbumId_less_0::Start");
    NativeRdb::ValuesBucket values;
    values.Put(USER_DISPLAY_LEVEL, FIRST_PAGE);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(ALBUM_ID, FALSE_ALBUM_ID);
    OperationType opType = OperationType::PORTRAIT_DISPLAY_LEVEL;
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("SetDisplaylevel_AlbumId_less_0 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetDisplaylevel_FirstPage_RDB_ERROR, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetDisplaylevel_FirstPage::Start");
    NativeRdb::ValuesBucket values;
    values.Put(USER_DISPLAY_LEVEL, FIRST_PAGE);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    OperationType opType = OperationType::PORTRAIT_DISPLAY_LEVEL;
    CleanTestTables();
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("SetDisplaylevel_FirstPage End");
}

void InsertAlbumTestData(string coverUri, int count, string tagId)
{
    Uri analysisAlbumUri(CONST_PAH_INSERT_ANA_PHOTO_ALBUM);
    MediaLibraryCommand cmd(analysisAlbumUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(ALBUM_TYPE, PhotoAlbumType::SMART);
    valuesBucket.Put(ALBUM_SUBTYPE, PORTRAIT);
    valuesBucket.Put(COVER_URI, coverUri);
    valuesBucket.Put(COUNT, count);
    valuesBucket.Put(TAG_ID, tagId);
    valuesBucket.Put(GROUP_TAG, tagId);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetDisplaylevel_FirstPage_RDB_OK, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetDisplaylevel_FirstPage_RDB_OK::Start");
    NativeRdb::ValuesBucket values;
    values.Put(USER_DISPLAY_LEVEL, FIRST_PAGE);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    OperationType opType = OperationType::PORTRAIT_DISPLAY_LEVEL;
    InsertAlbumTestData("", 0, "");
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("SetDisplaylevel_FirstPage_RDB_OK End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetDisplaylevel_Second_Page_RDB_OK, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetDisplaylevel_Second_Page_RDB_OK::Start");
    NativeRdb::ValuesBucket values;
    values.Put(USER_DISPLAY_LEVEL, SECOND_PAGE);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    OperationType opType = OperationType::PORTRAIT_DISPLAY_LEVEL;
    InsertAlbumTestData("", 0, "");
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("SetDisplaylevel_Second_Page_RDB_OK End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetDisplaylevel_Favorite_Page_ObtainAlbumOrder_Result_null, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetDisplaylevel_Favorite_Page_ObtainAlbumOrder_Result_null::Start");
    NativeRdb::ValuesBucket values;
    values.Put(USER_DISPLAY_LEVEL, FAVORITE_PAGE);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    OperationType opType = OperationType::PORTRAIT_DISPLAY_LEVEL;
    CleanTestTables();
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("SetDisplaylevel_Favorite_Page_ObtainAlbumOrder_Result_null End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetDisplaylevel_Favorite_Page_ObtainAlbumOrder_Result_0, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetDisplaylevel_Favorite_Page_ObtainAlbumOrder_Result_0::Start");
    NativeRdb::ValuesBucket values;
    values.Put(USER_DISPLAY_LEVEL, FAVORITE_PAGE);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    OperationType opType = OperationType::PORTRAIT_DISPLAY_LEVEL;
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("SetDisplaylevel_Favorite_Page_ObtainAlbumOrder_Result_0 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetDisplaylevel_Favorite_Page_ObtainAlbumOrder_Result_OK, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetDisplaylevel_Favorite_Page_ObtainAlbumOrder_Result_OK::Start");
    NativeRdb::ValuesBucket values;
    values.Put(USER_DISPLAY_LEVEL, FAVORITE_PAGE);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    OperationType opType = OperationType::PORTRAIT_DISPLAY_LEVEL;
    InsertAlbumTestData("", 0, "");
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("SetDisplaylevel_Favorite_Page_ObtainAlbumOrder_Result_OK End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetDisplaylevel_UnFavorite_Page_ObtainAlbumOrder_Result_null, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetDisplaylevel_UnFavorite_Page_ObtainAlbumOrder_Result_null::Start");
    NativeRdb::ValuesBucket values;
    values.Put(USER_DISPLAY_LEVEL, UNFAVORITE_PAGE);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    OperationType opType = OperationType::PORTRAIT_DISPLAY_LEVEL;
    CleanTestTables();
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("SetDisplaylevel_UnFavorite_Page_ObtainAlbumOrder_Result_null End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetDisplaylevel_UnFavorite_Page_ObtainAlbumOrder_Result_0, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetDisplaylevel_UnFavorite_Page_ObtainAlbumOrder_Result_0::Start");
    NativeRdb::ValuesBucket values;
    values.Put(USER_DISPLAY_LEVEL, UNFAVORITE_PAGE);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    OperationType opType = OperationType::PORTRAIT_DISPLAY_LEVEL;
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("SetDisplaylevel_UnFavorite_Page_ObtainAlbumOrder_Result_0 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetDisplaylevel_UnFavorite_Page_ObtainAlbumOrder_Result_OK, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetDisplaylevel_UnFavorite_Page_ObtainAlbumOrder_Result_OK::Start");
    NativeRdb::ValuesBucket values;
    values.Put(USER_DISPLAY_LEVEL, UNFAVORITE_PAGE);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    OperationType opType = OperationType::PORTRAIT_DISPLAY_LEVEL;
    InsertAlbumTestData("", 0, "");
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("SetDisplaylevel_UnFavorite_Page_ObtainAlbumOrder_Result_OK End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_GetAlbumId_ERR, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_GetAlbumId_ERR::Start");
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    OperationType opType = OperationType::PORTRAIT_MERGE_ALBUM;
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_GetAlbumId_ERR End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_GetAlbumId_less_0, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_GetAlbumId_less_0::Start");
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    DataShare::DataSharePredicates predicates;
    OperationType opType = OperationType::PORTRAIT_MERGE_ALBUM;
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_GetAlbumId_less_0 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_GetTargetAlbumId_less_0, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_GetTargetAlbumId_less_0::Start");
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(TARGET_ALBUM_ID, FALSE_ALBUM_ID);
    DataShare::DataSharePredicates predicates;
    OperationType opType = OperationType::PORTRAIT_MERGE_ALBUM;
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_GetTargetAlbumId_less_0 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_Curr_Equal_Target, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_Curr_Equal_Target::Start");
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    DataShare::DataSharePredicates predicates;
    values.Put(TARGET_ALBUM_ID, TRUE_ALBUM_ID);
    OperationType opType = OperationType::PORTRAIT_MERGE_ALBUM;
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_Curr_Equal_Target End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_GetMergeAlbumInfo_result_null, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_GetMergeAlbumInfo_result_null::Start");
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    DataShare::DataSharePredicates predicates;
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    OperationType opType = OperationType::PORTRAIT_MERGE_ALBUM;
    CleanTestTables();
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_GetMergeAlbumInfo_result_null End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_GetMergeAlbumInfo_result_0, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_GetMergeAlbumInfo_result_0::Start");
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    DataShare::DataSharePredicates predicates;
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    OperationType opType = OperationType::PORTRAIT_MERGE_ALBUM;
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_GetMergeAlbumInfo_result_0 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_GetMergeAlbumCoverUri_ParseFileIdFromCoverUri_uri_size_0,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_GetMergeAlbumCoverUri_ParseFileIdFromCoverUri_uri_size_0::Start");
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    DataShare::DataSharePredicates predicates;
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    OperationType opType = OperationType::PORTRAIT_MERGE_ALBUM;
    InsertAlbumTestData("", 0, "");
    InsertAlbumTestData("", 0, "");
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_GetMergeAlbumCoverUri_ParseFileIdFromCoverUri_uri_size_0 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_GetMergeAlbumCoverUri_ParseFileIdFromCoverUri_uri_size_10,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_GetMergeAlbumCoverUri_ParseFileIdFromCoverUri_uri_size_10::Start");
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    DataShare::DataSharePredicates predicates;
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    OperationType opType = OperationType::PORTRAIT_MERGE_ALBUM;
    InsertAlbumTestData("", 0, "1");
    InsertAlbumTestData("file://media/Photo/", 0, "2");
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_GetMergeAlbumCoverUri_ParseFileIdFromCoverUri_uri_size_10 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_GetMergeAlbumCoverUri_ParseFileIdFromCoverUri_uri_size_01,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_GetMergeAlbumCoverUri_ParseFileIdFromCoverUri_uri_size_01::Start");
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    DataShare::DataSharePredicates predicates;
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    OperationType opType = OperationType::PORTRAIT_MERGE_ALBUM;
    InsertAlbumTestData("file://media/Photo/", 0, "2");
    InsertAlbumTestData("file://media/Photo/2/3/3.jpg", 0, "1");
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_GetMergeAlbumCoverUri_ParseFileIdFromCoverUri_uri_size_01 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_GetMergeAlbumCoverUri_result_null, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_GetMergeAlbumInfo_GetMergeAlbumCoverUri_result_null::Start");
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    OperationType opType = OperationType::PORTRAIT_MERGE_ALBUM;
    InsertAlbumTestData("file://media/Photo/2/3/3.jpg", 0, "2");
    InsertAlbumTestData("file://media/Photo/2/3/3.jpg", 0, "1");
    CleanTestTables();
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_GetMergeAlbumInfo_GetMergeAlbumCoverUri_result_null End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_GetMergeAlbumCoverUri_result_0, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_GetMergeAlbumCoverUri_result_0::Start");
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    OperationType opType = OperationType::PORTRAIT_MERGE_ALBUM;
    InsertAlbumTestData("file://media/Photo/2/3/3.jpg", 0, "2");
    InsertAlbumTestData("file://media/Photo/2/3/3.jpg", 0, "1");
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_GetMergeAlbumCoverUri_result_0 End");
}

void CreatTestImage()
{
    Uri createAssetUri("file://media/Photo/create");
    string relativePath = "Pictures/";
    MediaType mediaType = MEDIA_TYPE_IMAGE;
    for (int i = 1; i < PIC_COUNT; i++) {
        string displayName = "";
        displayName = displayName + to_string(i);
        displayName = displayName + ".jpg";
        MEDIA_INFO_LOG("displayName:%{public}s", displayName.c_str());
        DataShare::DataShareValuesBucket valuesBucket;
        valuesBucket.Put(CONST_MEDIA_DATA_DB_MEDIA_TYPE, mediaType);
        valuesBucket.Put(CONST_MEDIA_DATA_DB_NAME, displayName);
        valuesBucket.Put(CONST_MEDIA_DATA_DB_RELATIVE_PATH, relativePath);
        MediaLibraryCommand cmd(createAssetUri);
        MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    }
}

void InsertAlbumTestData(AlbumColumn &column, const PhotoAlbumSubType &subType = PhotoAlbumSubType::PORTRAIT)
{
    Uri analysisAlbumUri(CONST_PAH_INSERT_ANA_PHOTO_ALBUM);
    MediaLibraryCommand cmd(analysisAlbumUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(ALBUM_TYPE, PhotoAlbumType::SMART);
    valuesBucket.Put(ALBUM_SUBTYPE, subType);
    valuesBucket.Put(COVER_URI, column.coverUri);
    valuesBucket.Put(COUNT, column.count);
    valuesBucket.Put(TAG_ID, column.tagId);
    valuesBucket.Put(GROUP_TAG, column.tagId);
    valuesBucket.Put(RANK, column.rank);
    valuesBucket.Put(USER_OPERATION, column.userOperation);
    valuesBucket.Put(USER_DISPLAY_LEVEL, column.displayLevel);
    valuesBucket.Put(IS_ME, column.isMe);
    valuesBucket.Put(RENAME_OPERATION, column.renameOperation);
    valuesBucket.Put(IS_COVER_SATISFIED, column.isCoverSatisfied);
    valuesBucket.Put(ALBUM_RELATIONSHIP, column.relationship);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
}

void InsertAlbumMapTestData(int albumId, int assetId)
{
    Uri analysisAlbumMapUri(CONST_PAH_INSERT_ANA_PHOTO_MAP);
    MediaLibraryCommand cmd(analysisAlbumMapUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MAP_ALBUM, albumId);
    valuesBucket.Put(MAP_ASSET, assetId);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
}

void InsertMergeTestData(AlbumColumn &curColumn, AlbumColumn &targetColumn)
{
    InsertAlbumTestData(curColumn);
    for (int i = 1; i <= ALBUM_CUR_COUNT; i++) {
        InsertAlbumMapTestData(TRUE_ALBUM_ID, i);
    }
    InsertAlbumTestData(targetColumn);
    for (int i = ALBUM_CUR_COUNT + 1; i <= ALBUM_TARGET_COUNT; i++) {
        InsertAlbumMapTestData(TURE_ALBUM_ID_TWO, i);
    }
}

void InsertAlbumMapRangeTestData(int32_t albumId, int32_t startAssetId, int32_t endAssetId)
{
    for (int32_t assetId = startAssetId; assetId <= endAssetId; assetId++) {
        InsertAlbumMapTestData(albumId, assetId);
    }
}

void InsertPortraitNickNameTestData(int32_t albumId, const vector<string> &nickNames)
{
    string sql = string("INSERT INTO ") + ANALYSIS_NICK_NAME_TABLE + " (" + ALBUM_ID + ", " + NICK_NAME + ") VALUES ";
    vector<NativeRdb::ValueObject> bindArgs;
    for (size_t i = 0; i < nickNames.size(); i++) {
        sql += "(?, ?)";
        if (i + 1 < nickNames.size()) {
            sql += ", ";
        }
        bindArgs.emplace_back(to_string(albumId));
        bindArgs.emplace_back(nickNames[i]);
    }
    EXPECT_EQ(g_rdbStore->ExecuteSql(sql, bindArgs), NativeRdb::E_OK);
}

vector<string> QueryPortraitNickNames(int32_t albumId)
{
    string sql = string("SELECT ") + NICK_NAME + " FROM " + ANALYSIS_NICK_NAME_TABLE + " WHERE " + ALBUM_ID +
        " = ? ORDER BY " + NICK_NAME;
    vector<string> bindArgs = { to_string(albumId) };
    vector<string> nickNames;
    auto resultSet = g_rdbStore->QuerySql(sql, bindArgs);
    EXPECT_NE(resultSet, nullptr);
    while (resultSet != nullptr && resultSet->GoToNextRow() == NativeRdb::E_OK) {
        nickNames.emplace_back(GetStringVal(NICK_NAME, resultSet));
    }
    return nickNames;
}

vector<string> GeneratePortraitNickNames(int32_t startIndex, int32_t count)
{
    vector<string> nickNames;
    nickNames.reserve(count);
    for (int32_t index = 0; index < count; index++) {
        nickNames.emplace_back("nick_" + to_string(startIndex + index));
    }
    return nickNames;
}

AlbumColumn QueryAlbumRelationshipInfo(int32_t albumId)
{
    AlbumColumn column = {};
    string sql = "SELECT " + IS_ME + ", " + ALBUM_RELATIONSHIP + " FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE " + ALBUM_ID + " = ?";
    vector<string> bindArgs = { to_string(albumId) };
    auto resultSet = g_rdbStore->QuerySql(sql, bindArgs);
    EXPECT_NE(resultSet, nullptr);
    if (resultSet == nullptr) {
        return column;
    }
    int32_t ret = resultSet->GoToFirstRow();
    EXPECT_EQ(ret, NativeRdb::E_OK);
    if (ret != NativeRdb::E_OK) {
        return column;
    }
    column.isMe = GetInt32Val(IS_ME, resultSet);
    column.relationship = GetStringVal(ALBUM_RELATIONSHIP, resultSet);
    return column;
}

AlbumColumn BuildPortraitAlbumColumn(int32_t albumId, int32_t count)
{
    AlbumColumn column = {};
    column.coverUri = "file://media/Photo/" + to_string(albumId) + "/3/3.jpg";
    column.count = count;
    column.tagId = to_string(albumId);
    return column;
}

void InsertFourPortraitAlbums()
{
    AlbumColumn firstColumn = BuildPortraitAlbumColumn(TRUE_ALBUM_ID, MERGED_GROUP_COMMON_COUNT);
    AlbumColumn secondColumn = BuildPortraitAlbumColumn(TURE_ALBUM_ID_TWO, MERGED_GROUP_COMMON_COUNT);
    AlbumColumn thirdColumn = BuildPortraitAlbumColumn(TRUE_ALBUM_ID_THREE, MERGED_GROUP_COMMON_COUNT);
    AlbumColumn fourthColumn = BuildPortraitAlbumColumn(TRUE_ALBUM_ID_FOUR, MERGED_GROUP_LAST_COUNT);
    InsertAlbumTestData(firstColumn);
    InsertAlbumMapRangeTestData(TRUE_ALBUM_ID, FIRST_ALBUM_START_ASSET_ID, FIRST_ALBUM_END_ASSET_ID);
    InsertAlbumTestData(secondColumn);
    InsertAlbumMapRangeTestData(TURE_ALBUM_ID_TWO, SECOND_ALBUM_START_ASSET_ID, SECOND_ALBUM_END_ASSET_ID);
    InsertAlbumTestData(thirdColumn);
    InsertAlbumMapRangeTestData(TRUE_ALBUM_ID_THREE, THIRD_ALBUM_START_ASSET_ID, THIRD_ALBUM_END_ASSET_ID);
    InsertAlbumTestData(fourthColumn);
    InsertAlbumMapRangeTestData(TRUE_ALBUM_ID_FOUR, FOURTH_ALBUM_START_ASSET_ID, FOURTH_ALBUM_END_ASSET_ID);
}

void MergePortraitAlbumsForTest(int32_t albumId, int32_t targetAlbumId,
    DataShare::DataSharePredicates &predicates)
{
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, albumId);
    values.Put(TARGET_ALBUM_ID, targetAlbumId);
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(OperationType::PORTRAIT_MERGE_ALBUM,
        values, predicates), E_OK);
}

void ExpectMergedGroupNickNames(const vector<string> &nickNames)
{
    EXPECT_EQ(QueryPortraitNickNames(TRUE_ALBUM_ID), nickNames);
    EXPECT_TRUE(QueryPortraitNickNames(TURE_ALBUM_ID_TWO).empty());
    EXPECT_TRUE(QueryPortraitNickNames(TRUE_ALBUM_ID_THREE).empty());
    EXPECT_TRUE(QueryPortraitNickNames(TRUE_ALBUM_ID_FOUR).empty());
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_UpdateMergeAlbumsInfo_isme_00, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_isme_00::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/3/3.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.tagId = "1";
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/3/3.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "2";
    InsertMergeTestData(curColumn, targetColumn);
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    OperationType opType = OperationType::PORTRAIT_MERGE_ALBUM;
    DataShare::DataSharePredicates predicates;
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_isme_00 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_MergePortraitNickNamesByMinAlbumId_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_MergePortraitNickNamesByMinAlbumId_001::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/3/3.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.tagId = "1";
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/3/3.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "2";
    InsertMergeTestData(curColumn, targetColumn);
    InsertPortraitNickNameTestData(TURE_ALBUM_ID_TWO, { "nick_b", "nick_a" });
    InsertPortraitNickNameTestData(TRUE_ALBUM_ID, { "nick_a" });

    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TURE_ALBUM_ID_TWO);
    values.Put(TARGET_ALBUM_ID, TRUE_ALBUM_ID);
    DataShare::DataSharePredicates predicates;
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(OperationType::PORTRAIT_MERGE_ALBUM,
        values, predicates), E_OK);

    vector<string> retainedNickNames = QueryPortraitNickNames(TRUE_ALBUM_ID);
    vector<string> removedNickNames = QueryPortraitNickNames(TURE_ALBUM_ID_TWO);
    EXPECT_EQ(retainedNickNames, (vector<string>{ "nick_a", "nick_b" }));
    EXPECT_TRUE(removedNickNames.empty());
    MEDIA_INFO_LOG("MergeAlbum_MergePortraitNickNamesByMinAlbumId_001::End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, AddPortraitNickName_UseMinAlbumIdInMergedAlbums_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AddPortraitNickName_UseMinAlbumIdInMergedAlbums_001::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/3/3.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.tagId = "1";
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/3/3.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "2";
    InsertMergeTestData(curColumn, targetColumn);

    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TURE_ALBUM_ID_TWO);
    values.Put(TARGET_ALBUM_ID, TRUE_ALBUM_ID);
    DataShare::DataSharePredicates predicates;
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(OperationType::PORTRAIT_MERGE_ALBUM,
        values, predicates), E_OK);

    EXPECT_EQ(MediaLibraryAlbumOperations::OperatePortraitAlbumNickName(to_string(TURE_ALBUM_ID_TWO), "add",
        { "nick_c" }), E_OK);
    vector<string> retainedNickNames = QueryPortraitNickNames(TRUE_ALBUM_ID);
    vector<string> removedNickNames = QueryPortraitNickNames(TURE_ALBUM_ID_TWO);
    EXPECT_EQ(retainedNickNames, (vector<string>{ "nick_c" }));
    EXPECT_TRUE(removedNickNames.empty());
    MEDIA_INFO_LOG("AddPortraitNickName_UseMinAlbumIdInMergedAlbums_001::End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_MergePortraitNickNamesUseMinAlbumIdAcrossMergedGroups_001,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_MergePortraitNickNamesUseMinAlbumIdAcrossMergedGroups_001::Start");
    CreatTestImage();
    InsertFourPortraitAlbums();
    DataShare::DataSharePredicates predicates;
    MergePortraitAlbumsForTest(1, 2, predicates);
    MergePortraitAlbumsForTest(3, 4, predicates);

    InsertPortraitNickNameTestData(1, { "nick_a" });
    InsertPortraitNickNameTestData(2, { "nick_b" });
    InsertPortraitNickNameTestData(3, { "nick_c" });
    InsertPortraitNickNameTestData(4, { "nick_d" });

    MergePortraitAlbumsForTest(2, 4, predicates);

    ExpectMergedGroupNickNames({ "nick_a", "nick_b", "nick_c", "nick_d" });
    MEDIA_INFO_LOG("MergeAlbum_MergePortraitNickNamesUseMinAlbumIdAcrossMergedGroups_001::End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_MergePortraitNickNamesDedupAcrossMergedGroups_001,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_MergePortraitNickNamesDedupAcrossMergedGroups_001::Start");
    CreatTestImage();
    InsertFourPortraitAlbums();
    DataShare::DataSharePredicates predicates;
    MergePortraitAlbumsForTest(1, 2, predicates);
    MergePortraitAlbumsForTest(3, 4, predicates);

    InsertPortraitNickNameTestData(1, { "nick_a", "nick_shared" });
    InsertPortraitNickNameTestData(2, { "nick_b" });
    InsertPortraitNickNameTestData(3, { "nick_c", "nick_shared" });
    InsertPortraitNickNameTestData(4, { "nick_a", "nick_d" });

    MergePortraitAlbumsForTest(2, 4, predicates);

    ExpectMergedGroupNickNames({ "nick_a", "nick_b", "nick_c", "nick_d", "nick_shared" });
    MEDIA_INFO_LOG("MergeAlbum_MergePortraitNickNamesDedupAcrossMergedGroups_001::End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, RemovePortraitNickName_RemoveAcrossMergedAlbums_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("RemovePortraitNickName_RemoveAcrossMergedAlbums_001::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/3/3.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.tagId = "1";
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/3/3.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "2";
    InsertMergeTestData(curColumn, targetColumn);
    InsertPortraitNickNameTestData(TRUE_ALBUM_ID, { "nick_a" });
    InsertPortraitNickNameTestData(TURE_ALBUM_ID_TWO, { "nick_b" });

    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TURE_ALBUM_ID_TWO);
    values.Put(TARGET_ALBUM_ID, TRUE_ALBUM_ID);
    DataShare::DataSharePredicates predicates;
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(OperationType::PORTRAIT_MERGE_ALBUM,
        values, predicates), E_OK);

    EXPECT_EQ(MediaLibraryAlbumOperations::OperatePortraitAlbumNickName(to_string(TURE_ALBUM_ID_TWO), "remove",
        { "nick_a", "nick_b" }), E_OK);
    EXPECT_TRUE(QueryPortraitNickNames(TRUE_ALBUM_ID).empty());
    EXPECT_TRUE(QueryPortraitNickNames(TURE_ALBUM_ID_TWO).empty());
    MEDIA_INFO_LOG("RemovePortraitNickName_RemoveAcrossMergedAlbums_001::End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, AddPortraitNickName_FailWhenMergedAlbumNickNameExceedsLimit_001,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("AddPortraitNickName_FailWhenMergedAlbumNickNameExceedsLimit_001::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/3/3.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.tagId = "1";
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/3/3.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "2";
    InsertMergeTestData(curColumn, targetColumn);
    InsertPortraitNickNameTestData(TRUE_ALBUM_ID, GeneratePortraitNickNames(0, 60));
    InsertPortraitNickNameTestData(TURE_ALBUM_ID_TWO, GeneratePortraitNickNames(60, 60));

    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TURE_ALBUM_ID_TWO);
    values.Put(TARGET_ALBUM_ID, TRUE_ALBUM_ID);
    DataShare::DataSharePredicates predicates;
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(OperationType::PORTRAIT_MERGE_ALBUM,
        values, predicates), E_OK);

    vector<string> retainedNickNames = QueryPortraitNickNames(TRUE_ALBUM_ID);
    EXPECT_EQ(retainedNickNames.size(), 120);
    EXPECT_EQ(MediaLibraryAlbumOperations::OperatePortraitAlbumNickName(to_string(TURE_ALBUM_ID_TWO), "add",
        { "nick_over_limit" }), E_OPERATION_NOT_SUPPORT);
    EXPECT_EQ(QueryPortraitNickNames(TRUE_ALBUM_ID).size(), retainedNickNames.size());
    EXPECT_TRUE(QueryPortraitNickNames(TURE_ALBUM_ID_TWO).empty());
    MEDIA_INFO_LOG("AddPortraitNickName_FailWhenMergedAlbumNickNameExceedsLimit_001::End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, AddPortraitNickName_FallbackToCurrentAlbumWhenGroupTagEmpty_001,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("AddPortraitNickName_FallbackToCurrentAlbumWhenGroupTagEmpty_001::Start");
    CreatTestImage();
    InsertAlbumTestData("file://media/Photo/2/3/3.jpg", 0, "");

    EXPECT_EQ(MediaLibraryAlbumOperations::OperatePortraitAlbumNickName(to_string(TRUE_ALBUM_ID), "add",
        { "nick_a" }), E_OK);
    EXPECT_EQ(QueryPortraitNickNames(TRUE_ALBUM_ID), (vector<string>{ "nick_a" }));
    MEDIA_INFO_LOG("AddPortraitNickName_FallbackToCurrentAlbumWhenGroupTagEmpty_001::End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, NickNameOperationData_KeepIndependentNetState_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("NickNameOperationData_KeepIndependentNetState_001::Start");
    TestNickNameOperationData operationData;
    vector<TestNickNameChangeOperation> operations;

    AnalysisAlbumOperationDataUtils::UpdateAddNickNameOperationData(operationData, operations,
        TestNickNameChangeOperation::ADD, TestNickNameChangeOperation::REMOVE, { "nick_a" });
    AnalysisAlbumOperationDataUtils::UpdateRemoveNickNameOperationData(operationData, operations,
        TestNickNameChangeOperation::ADD, TestNickNameChangeOperation::REMOVE, { "nick_b" });
    AnalysisAlbumOperationDataUtils::UpdateAddNickNameOperationData(operationData, operations,
        TestNickNameChangeOperation::ADD, TestNickNameChangeOperation::REMOVE, { "nick_c" });

    EXPECT_EQ(operationData.addNickNames, (vector<string>{ "nick_a", "nick_c" }));
    EXPECT_EQ(operationData.removeNickNames, (vector<string>{ "nick_b" }));
    EXPECT_EQ(operations, (vector<TestNickNameChangeOperation> {
        TestNickNameChangeOperation::ADD, TestNickNameChangeOperation::REMOVE }));
    MEDIA_INFO_LOG("NickNameOperationData_KeepIndependentNetState_001::End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, NickNameOperationData_UseLatestStateForSameNickName_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("NickNameOperationData_UseLatestStateForSameNickName_001::Start");
    TestNickNameOperationData operationData;
    vector<TestNickNameChangeOperation> operations;

    AnalysisAlbumOperationDataUtils::UpdateRemoveNickNameOperationData(operationData, operations,
        TestNickNameChangeOperation::ADD, TestNickNameChangeOperation::REMOVE, { "nick_a" });
    AnalysisAlbumOperationDataUtils::UpdateAddNickNameOperationData(operationData, operations,
        TestNickNameChangeOperation::ADD, TestNickNameChangeOperation::REMOVE, { "nick_a" });

    EXPECT_EQ(operationData.addNickNames, (vector<string>{ "nick_a" }));
    EXPECT_TRUE(operationData.removeNickNames.empty());
    EXPECT_EQ(operations, (vector<TestNickNameChangeOperation> { TestNickNameChangeOperation::ADD }));
    MEDIA_INFO_LOG("NickNameOperationData_UseLatestStateForSameNickName_001::End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, NickNameOperationData_SetOperationDataFollowNetState_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("NickNameOperationData_SetOperationDataFollowNetState_001::Start");
    TestNickNameOperationData operationData;
    vector<TestNickNameChangeOperation> operations;

    AnalysisAlbumOperationDataUtils::SetNickNameOperationData(operationData, operations, ANALYSIS_ALBUM_OP_ADD,
        TestNickNameChangeOperation::ADD, TestNickNameChangeOperation::REMOVE, { "nick_a", "nick_b" });
    AnalysisAlbumOperationDataUtils::SetNickNameOperationData(operationData, operations, ANALYSIS_ALBUM_OP_REMOVE,
        TestNickNameChangeOperation::ADD, TestNickNameChangeOperation::REMOVE, { "nick_a", "nick_c" });
    AnalysisAlbumOperationDataUtils::SetNickNameOperationData(operationData, operations, ANALYSIS_ALBUM_OP_ADD,
        TestNickNameChangeOperation::ADD, TestNickNameChangeOperation::REMOVE, { "nick_c" });

    EXPECT_EQ(operationData.addNickNames, (vector<string>{ "nick_b", "nick_c" }));
    EXPECT_EQ(operationData.removeNickNames, (vector<string>{ "nick_a" }));
    EXPECT_EQ(operations, (vector<TestNickNameChangeOperation> {
        TestNickNameChangeOperation::ADD, TestNickNameChangeOperation::REMOVE }));
    MEDIA_INFO_LOG("NickNameOperationData_SetOperationDataFollowNetState_001::End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, NickNameOperationData_ExceedLimitShouldNotPolluteState_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("NickNameOperationData_ExceedLimitShouldNotPolluteState_001::Start");
    TestNickNameOperationData operationData;
    vector<TestNickNameChangeOperation> operations;

    for (int32_t i = 0; i < ANALYSIS_ALBUM_MAX_OPERATION_VALUES - 1; i++) {
        operationData.addNickNames.emplace_back("nick_" + to_string(i));
    }
    operationData.removeNickNames = { "nick_remove" };
    operations = { TestNickNameChangeOperation::ADD, TestNickNameChangeOperation::REMOVE };

    const auto oldAddNickNames = operationData.addNickNames;
    const auto oldRemoveNickNames = operationData.removeNickNames;
    const auto oldOperations = operations;

    bool ret = AnalysisAlbumOperationDataUtils::SetNickNameOperationData(operationData, operations,
        ANALYSIS_ALBUM_OP_ADD, TestNickNameChangeOperation::ADD, TestNickNameChangeOperation::REMOVE,
        { "nick_over_limit" });

    EXPECT_FALSE(ret);
    EXPECT_EQ(operationData.addNickNames, oldAddNickNames);
    EXPECT_EQ(operationData.removeNickNames, oldRemoveNickNames);
    EXPECT_EQ(operations, oldOperations);
    MEDIA_INFO_LOG("NickNameOperationData_ExceedLimitShouldNotPolluteState_001::End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_UpdateMergeAlbumsInfo_isme_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_isme_01::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/3/3.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.tagId = "1";
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/3/3.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "2";
    targetColumn.isMe = IS_ME_VALUE;
    targetColumn.relationship = ME_RELATIONSHIP_VALUE;
    InsertMergeTestData(curColumn, targetColumn);
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    OperationType opType = OperationType::PORTRAIT_MERGE_ALBUM;
    DataShare::DataSharePredicates predicates;
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_isme_01 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_UpdateMergeAlbumsInfo_isme_10, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_isme_10::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/3/3.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.tagId = "1";
    curColumn.isMe = IS_ME_VALUE;
    curColumn.relationship = ME_RELATIONSHIP_VALUE;
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/3/3.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "2";
    InsertMergeTestData(curColumn, targetColumn);
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    OperationType opType = OperationType::PORTRAIT_MERGE_ALBUM;
    DataShare::DataSharePredicates predicates;
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_isme_10 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_UpdateMergeAlbumsInfo_IsMeFollowRelationship_01,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_IsMeFollowRelationship_01::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/3/3.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.tagId = "1";
    curColumn.displayLevel = FAVORITE_PAGE;
    curColumn.rank = RANK_ONE;
    curColumn.relationship = FAMILY_RELATIONSHIP_VALUE;
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/3/3.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "2";
    targetColumn.displayLevel = UNFAVORITE_PAGE;
    targetColumn.isMe = IS_ME_VALUE;
    targetColumn.relationship = ME_RELATIONSHIP_VALUE;
    InsertMergeTestData(curColumn, targetColumn);

    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    DataShare::DataSharePredicates predicates;
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(OperationType::PORTRAIT_MERGE_ALBUM,
        values, predicates), E_OK);

    AlbumColumn mergedColumn = QueryAlbumRelationshipInfo(TRUE_ALBUM_ID);
    EXPECT_EQ(mergedColumn.relationship, FAMILY_RELATIONSHIP_VALUE);
    EXPECT_EQ(mergedColumn.isMe, IS_NOT_ME_VALUE);
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_IsMeFollowRelationship_01 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_UpdateMergeAlbumsInfo_IsMeFollowRelationship_02,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_IsMeFollowRelationship_02::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/3/3.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.tagId = "1";
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/3/3.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "2";
    targetColumn.isMe = IS_ME_VALUE;
    targetColumn.relationship = ME_RELATIONSHIP_VALUE;
    InsertMergeTestData(curColumn, targetColumn);

    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    DataShare::DataSharePredicates predicates;
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(OperationType::PORTRAIT_MERGE_ALBUM,
        values, predicates), E_OK);

    AlbumColumn mergedColumn = QueryAlbumRelationshipInfo(TRUE_ALBUM_ID);
    EXPECT_EQ(mergedColumn.relationship, ME_RELATIONSHIP_VALUE);
    EXPECT_EQ(mergedColumn.isMe, IS_ME_VALUE);
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_IsMeFollowRelationship_02 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_UpdateMergeAlbumsInfo_albumName_00, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_albumName_00::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/4/5.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.tagId = "3";
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/3/3.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "4";
    InsertMergeTestData(curColumn, targetColumn);
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    OperationType operationType = OperationType::PORTRAIT_MERGE_ALBUM;
    DataShare::DataSharePredicates predicates;
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, predicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_albumName_00 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_UpdateMergeAlbumsInfo_albumName_10, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_albumName_10::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/4/5.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.tagId = "3";
    curColumn.albumName = "test1";
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/6/6.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "4";
    InsertMergeTestData(curColumn, targetColumn);
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    OperationType operationType = OperationType::PORTRAIT_MERGE_ALBUM;
    DataShare::DataSharePredicates dataSharePredicates;
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataSharePredicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_albumName_10 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_UpdateMergeAlbumsInfo_albumName_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_albumName_01::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/6/6.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.tagId = "3";
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/6/6.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "4";
    targetColumn.albumName = "test1";
    InsertMergeTestData(curColumn, targetColumn);
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    OperationType operationType = OperationType::PORTRAIT_MERGE_ALBUM;
    DataShare::DataSharePredicates predicates;
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, predicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_albumName_01 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_UpdateMergeAlbumsInfo_albumName_11, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_albumName_11::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/6/6.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.albumName = "test2";
    curColumn.tagId = "3";
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/6/6.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "4";
    targetColumn.albumName = "test3";
    InsertMergeTestData(curColumn, targetColumn);
    NativeRdb::ValuesBucket values;
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    OperationType operationType = OperationType::PORTRAIT_MERGE_ALBUM;
    DataShare::DataSharePredicates predicates;
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, predicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_albumName_11 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_UpdateMergeAlbumsInfo_first_first_level, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_first_first_level::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/6/6.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.albumName = "testdd2";
    curColumn.tagId = "3";
    curColumn.displayLevel = FIRST_PAGE;
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/6/6.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "4";
    targetColumn.albumName = "testdd3";
    targetColumn.displayLevel = FIRST_PAGE;
    InsertMergeTestData(curColumn, targetColumn);
    NativeRdb::ValuesBucket values;
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    DataShare::DataSharePredicates predicates;
    OperationType operationType = OperationType::PORTRAIT_MERGE_ALBUM;
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, predicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_first_first_level End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_UpdateMergeAlbumsInfo_second_second_level, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_second_second_level::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/6/6.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.albumName = "testdd2";
    curColumn.tagId = "3";
    curColumn.displayLevel = SECOND_PAGE;
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/5/5.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "4";
    targetColumn.albumName = "testdd3";
    targetColumn.displayLevel = SECOND_PAGE;
    InsertMergeTestData(curColumn, targetColumn);
    OperationType operationType = OperationType::PORTRAIT_MERGE_ALBUM;
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    DataShare::DataSharePredicates predicates;
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, predicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_second_second_level End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_UpdateMergeAlbumsInfo_unfavorite_unfavorite_level, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_unfavorite_unfavorite_level::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/6/6.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.albumName = "testddee2";
    curColumn.tagId = "3";
    curColumn.displayLevel = UNFAVORITE_PAGE;
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/5/5.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "4";
    targetColumn.albumName = "teseetdd3";
    targetColumn.displayLevel = UNFAVORITE_PAGE;
    InsertMergeTestData(curColumn, targetColumn);
    OperationType operationType = OperationType::PORTRAIT_MERGE_ALBUM;
    DataShare::DataSharePredicates predicates;
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, predicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_unfavorite_unfavorite_level End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_UpdateMergeAlbumsInfo_favorite_favorite_level, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_favorite_favorite_level::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/6/6.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.albumName = "testddggee2";
    curColumn.tagId = "3";
    curColumn.displayLevel = FAVORITE_PAGE;
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/5/5.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "4";
    targetColumn.albumName = "teseetdred3";
    targetColumn.displayLevel = FAVORITE_PAGE;
    InsertMergeTestData(curColumn, targetColumn);
    OperationType operationType = OperationType::PORTRAIT_MERGE_ALBUM;
    DataShare::DataSharePredicates predicates;
    NativeRdb::ValuesBucket values;
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, predicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_favorite_favorite_level End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_UpdateMergeAlbumsInfo_favorite_favorite_level_rank_01,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_favorite_favorite_level_rank_01::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/7/7.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.albumName = "testddggee2";
    curColumn.tagId = "3";
    curColumn.displayLevel = FAVORITE_PAGE;
    curColumn.rank = RANK_ONE;
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/6/6.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "4";
    targetColumn.albumName = "teseetdred3";
    targetColumn.displayLevel = FAVORITE_PAGE;
    targetColumn.rank = RANK_TWO;
    InsertMergeTestData(curColumn, targetColumn);
    OperationType operationType = OperationType::PORTRAIT_MERGE_ALBUM;
    DataShare::DataSharePredicates predicates;
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, predicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_favorite_favorite_level_rank_01 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_UpdateMergeAlbumsInfo_favorite_favorite_level_rank_10,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_favorite_favorite_level_rank_10::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/7/7.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.albumName = "testddggffee2";
    curColumn.tagId = "3";
    curColumn.displayLevel = FAVORITE_PAGE;
    curColumn.rank = RANK_TWO;
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/6/6.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "4";
    targetColumn.albumName = "teseetdrerd3";
    targetColumn.displayLevel = FAVORITE_PAGE;
    targetColumn.rank = RANK_ONE;
    InsertMergeTestData(curColumn, targetColumn);
    DataShare::DataSharePredicates predicates;
    OperationType operationType = OperationType::PORTRAIT_MERGE_ALBUM;
    NativeRdb::ValuesBucket values;
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, predicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_favorite_favorite_level_rank_10 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_UpdateMergeAlbumsInfo_one_favorite_level_rank_10, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_one_favorite_level_rank_10::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/7/7.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.albumName = "testddggrrffee2";
    curColumn.tagId = "3";
    curColumn.displayLevel = FAVORITE_PAGE;
    curColumn.rank = RANK_TWO;
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/6/6.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "4";
    targetColumn.albumName = "teseetdrereed3";
    targetColumn.displayLevel = UNFAVORITE_PAGE;
    targetColumn.rank = RANK_ONE;
    InsertMergeTestData(curColumn, targetColumn);
    DataShare::DataSharePredicates dataPredicates;
    OperationType operationType = OperationType::PORTRAIT_MERGE_ALBUM;
    NativeRdb::ValuesBucket values;
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_one_favorite_level_rank_10 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_UpdateMergeAlbumsInfo_favorite_one_level_rank_10, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_favorite_one_level_rank_10::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/7/7.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.albumName = "testddggrrffee2";
    curColumn.tagId = "3";
    curColumn.displayLevel = UNFAVORITE_PAGE;
    curColumn.rank = RANK_TWO;
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/9/9.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "4eee";
    targetColumn.albumName = "teseetdrereed3";
    targetColumn.displayLevel = FAVORITE_PAGE;
    targetColumn.rank = RANK_ONE;
    InsertMergeTestData(curColumn, targetColumn);
    DataShare::DataSharePredicates dataPredicates;
    OperationType operationType = OperationType::PORTRAIT_MERGE_ALBUM;
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_favorite_one_level_rank_10 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_UpdateMergeAlbumsInfo_portrait_pet_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_portrait_pet_01::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/7/7.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.albumName = "testddggrrffee2";
    curColumn.tagId = "3";
    curColumn.displayLevel = UNFAVORITE_PAGE;
    curColumn.rank = RANK_TWO;
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/9/9.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "4eee";
    targetColumn.albumName = "teseetdrereed3";
    targetColumn.displayLevel = FAVORITE_PAGE;
    targetColumn.rank = RANK_ONE;
    InsertAlbumTestData(curColumn, PhotoAlbumSubType::PORTRAIT);
    for (int i = 1; i <= ALBUM_CUR_COUNT; i++) {
        InsertAlbumMapTestData(TRUE_ALBUM_ID, i);
    }
    InsertAlbumTestData(targetColumn, PhotoAlbumSubType::PET);
    for (int i = ALBUM_CUR_COUNT + 1; i <= ALBUM_TARGET_COUNT; i++) {
        InsertAlbumMapTestData(TURE_ALBUM_ID_TWO, i);
    }
    DataShare::DataSharePredicates dataPredicates;
    OperationType operationType = OperationType::PORTRAIT_MERGE_ALBUM;
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    auto result = MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates);
    EXPECT_EQ(result, E_INVALID_VALUES);
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_portrait_pet_01 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_UpdateMergeAlbumsInfo_pet_portrait_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_pet_portrait_01::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/7/7.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.albumName = "testddggrrffee2";
    curColumn.tagId = "3";
    curColumn.displayLevel = UNFAVORITE_PAGE;
    curColumn.rank = RANK_TWO;
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/9/9.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "4eee";
    targetColumn.albumName = "teseetdrereed3";
    targetColumn.displayLevel = FAVORITE_PAGE;
    targetColumn.rank = RANK_ONE;
    InsertAlbumTestData(curColumn, PhotoAlbumSubType::PET);
    for (int i = 1; i <= ALBUM_CUR_COUNT; i++) {
        InsertAlbumMapTestData(TRUE_ALBUM_ID, i);
    }
    InsertAlbumTestData(targetColumn, PhotoAlbumSubType::PORTRAIT);
    for (int i = ALBUM_CUR_COUNT + 1; i <= ALBUM_TARGET_COUNT; i++) {
        InsertAlbumMapTestData(TURE_ALBUM_ID_TWO, i);
    }
    DataShare::DataSharePredicates dataPredicates;
    OperationType operationType = OperationType::PORTRAIT_MERGE_ALBUM;
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    auto result = MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates);
    EXPECT_EQ(result, E_INVALID_VALUES);
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_pet_portrait_01 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_UpdateMergeAlbumsInfo_pet_pet_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_pet_pet_01::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/7/7.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.albumName = "testddggrrffee2";
    curColumn.tagId = "3";
    curColumn.displayLevel = UNFAVORITE_PAGE;
    curColumn.rank = RANK_TWO;
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/9/9.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "4eee";
    targetColumn.albumName = "teseetdrereed3";
    targetColumn.displayLevel = FAVORITE_PAGE;
    targetColumn.rank = RANK_ONE;
    InsertAlbumTestData(curColumn, PhotoAlbumSubType::PET);
    for (int i = 1; i <= ALBUM_CUR_COUNT; i++) {
        InsertAlbumMapTestData(TRUE_ALBUM_ID, i);
    }
    InsertAlbumTestData(targetColumn, PhotoAlbumSubType::PET);
    for (int i = ALBUM_CUR_COUNT + 1; i <= ALBUM_TARGET_COUNT; i++) {
        InsertAlbumMapTestData(TURE_ALBUM_ID_TWO, i);
    }
    DataShare::DataSharePredicates dataPredicates;
    OperationType operationType = OperationType::PORTRAIT_MERGE_ALBUM;
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    auto result = MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates);
    EXPECT_NE(result, 0);
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_pet_pet_01 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_UpdateMergeAlbumsInfo_first_one_level_rank_10, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_first_one_level_rank_10::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/7/7.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.albumName = "testddeeggrrffee2";
    curColumn.tagId = "3";
    curColumn.displayLevel = FIRST_PAGE;
    curColumn.rank = RANK_TWO;
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/9/9.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "4eee";
    targetColumn.albumName = "teseetdrereed3";
    targetColumn.displayLevel = SECOND_PAGE;
    targetColumn.rank = RANK_ONE;
    DataShare::DataSharePredicates dataPredicates;
    OperationType operationType = OperationType::PORTRAIT_MERGE_ALBUM;
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    InsertMergeTestData(curColumn, targetColumn);
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_first_one_level_rank_10 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_UpdateMergeAlbumsInfo__one_first_level_rank_10, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo__one_first_level_rank_10::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/7/7.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.albumName = "testddeeggrrffee2";
    curColumn.tagId = "3";
    curColumn.displayLevel = SECOND_PAGE;
    curColumn.rank = RANK_TWO;
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/9/9.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "4eee";
    targetColumn.albumName = "teseetdrereeeed3";
    targetColumn.displayLevel = FIRST_PAGE;
    targetColumn.rank = RANK_ONE;
    DataShare::DataSharePredicates dataPredicates;
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    InsertMergeTestData(curColumn, targetColumn);
    OperationType operationType = OperationType::PORTRAIT_MERGE_ALBUM;
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo__one_first_level_rank_10 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, MergeAlbum_UpdateMergeAlbumsInfo_second_second_level_rank_10, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_second_second_level_rank_10::Start");
    CreatTestImage();
    AlbumColumn curColumn;
    curColumn.coverUri = "file://media/Photo/2/7/7.jpg";
    curColumn.count = ALBUM_CUR_COUNT;
    curColumn.albumName = "testddeeggrrffee2";
    curColumn.tagId = "3";
    curColumn.displayLevel = SECOND_PAGE;
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/11/9/9.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "4eee";
    targetColumn.albumName = "teseetdrerddeeeed3";
    targetColumn.displayLevel = SECOND_PAGE;
    DataShare::DataSharePredicates dataPredicates;
    NativeRdb::ValuesBucket values;
    values.Put(TARGET_ALBUM_ID, TURE_ALBUM_ID_TWO);
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    InsertMergeTestData(curColumn, targetColumn);
    OperationType operationType = OperationType::PORTRAIT_MERGE_ALBUM;
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("MergeAlbum_UpdateMergeAlbumsInfo_second_second_level_rank_10 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetIsMe_No_albumId, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetIsMe_No_albumId::Start");
    OperationType operationType = OperationType::PORTRAIT_IS_ME;
    DataShare::DataSharePredicates dataPredicates;
    NativeRdb::ValuesBucket values;
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetIsMe_No_albumId End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetIsMe_SetMyOldAlbum_result_null, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetIsMe_SetMyOldAlbum_result_null::Start");
    OperationType operationType = OperationType::PORTRAIT_IS_ME;
    DataShare::DataSharePredicates dataPredicates;
    dataPredicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    NativeRdb::ValuesBucket values;
    CleanTestTables();
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetIsMe_SetMyOldAlbum_result_null End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetIsMe_SetMyOldAlbum_result_0, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetIsMe_SetMyOldAlbum_result_0::Start");
    OperationType operationType = OperationType::PORTRAIT_IS_ME;
    DataShare::DataSharePredicates dataPredicates;
    NativeRdb::ValuesBucket values;
    dataPredicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetIsMe_SetMyOldAlbum_result_0 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetIsMe_SetMyOldAlbum_count_more_0_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetIsMe_SetMyOldAlbum_count_more_0_001::Start");
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/1/9/9.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "4ee";
    targetColumn.albumName = "teseetd3";
    targetColumn.isMe = IS_ME_VALUE;
    targetColumn.displayLevel = FAVORITE_PAGE;
    InsertAlbumTestData(targetColumn);
    OperationType operationType = OperationType::PORTRAIT_IS_ME;
    DataShare::DataSharePredicates dataPredicates;
    NativeRdb::ValuesBucket values;
    dataPredicates.EqualTo(ALBUM_ID, TURE_ALBUM_ID_TWO);
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetIsMe_SetMyOldAlbum_count_more_0_001 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetIsMe_SetMyOldAlbum_count_more_0_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetIsMe_SetMyOldAlbum_count_more_0_002::Start");
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/1/9/9.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "4e";
    targetColumn.isMe = IS_ME_VALUE;
    targetColumn.displayLevel = SECOND_PAGE;
    InsertAlbumTestData(targetColumn);
    OperationType operationType = OperationType::PORTRAIT_IS_ME;
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates dataPredicates;
    dataPredicates.EqualTo(ALBUM_ID, TURE_ALBUM_ID_TWO);
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetIsMe_SetMyOldAlbum_count_more_0_002 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetIsMe_SetMyOldAlbum_favorite_level, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetIsMe_SetMyOldAlbum_favorite_level::Start");
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/1/9/9.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "4eeew";
    targetColumn.isMe = IS_ME_VALUE;
    targetColumn.displayLevel = FAVORITE_PAGE;
    InsertAlbumTestData(targetColumn);
    OperationType operationType = OperationType::PORTRAIT_IS_ME;
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    DataShare::DataSharePredicates dataPredicates;
    dataPredicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetIsMe_SetMyOldAlbum_favorite_level End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetIsMe_SetMyOldAlbum_not_favorite_level, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetIsMe_SetMyOldAlbum_not_favorite_level::Start");
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/1/9/9.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "4eeeeww";
    targetColumn.isMe = IS_ME_VALUE;
    targetColumn.displayLevel = UNFAVORITE_PAGE;
    InsertAlbumTestData(targetColumn);
    OperationType operationType = OperationType::PORTRAIT_IS_ME;
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    DataShare::DataSharePredicates dataPredicates;
    dataPredicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetIsMe_SetMyOldAlbum_not_favorite_level End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetAlbumName_no_album_id, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetAlbumName_no_album_id::Start");
    OperationType operationType = OperationType::PORTRAIT_ALBUM_NAME;
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates dataPredicates;
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetAlbumName_no_album_id End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetAlbumName_no_album_name, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetAlbumName_no_album_name::Start");
    OperationType operationType = OperationType::PORTRAIT_ALBUM_NAME;
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    DataShare::DataSharePredicates dataPredicates;
    dataPredicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetAlbumName_no_album_name End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetAlbumName_album_name_empty, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetAlbumName_album_name_empty::Start");
    OperationType operationType = OperationType::PORTRAIT_ALBUM_NAME;
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(ALBUM_NAME, "");
    DataShare::DataSharePredicates dataPredicates;
    dataPredicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetAlbumName_album_name_empty End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetAlbumName_update_err, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetAlbumName_update_err::Start");
    OperationType operationType = OperationType::PORTRAIT_ALBUM_NAME;
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(ALBUM_NAME, "dd");
    DataShare::DataSharePredicates dataPredicates;
    dataPredicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    CleanTestTables();
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetAlbumName_update_err End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetAlbumName_update_succ, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetAlbumName_update_succ::Start");
    OperationType operationType = OperationType::PORTRAIT_ALBUM_NAME;
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(ALBUM_NAME, "dd");
    DataShare::DataSharePredicates dataPredicates;
    dataPredicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/1/10/10.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "eww";
    targetColumn.isMe = IS_ME_VALUE;
    targetColumn.displayLevel = UNFAVORITE_PAGE;
    InsertAlbumTestData(targetColumn);
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetAlbumName_update_succ End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetCoverUri_no_coverUri_id, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetCoverUri_no_coverUri_id::Start");
    OperationType operationType = OperationType::PORTRAIT_COVER_URI;
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates dataPredicates;
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetCoverUri_no_coverUri_id End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetCoverUri_no_coverUri, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetCoverUri_no_coverUri::Start");
    OperationType operationType = OperationType::PORTRAIT_COVER_URI;
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates dataPredicates;
    dataPredicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetCoverUri_no_coverUri End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetCoverUri_coverUri_empty, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetCoverUri_coverUri_empty::Start");
    OperationType operationType = OperationType::PORTRAIT_COVER_URI;
    NativeRdb::ValuesBucket values;
    values.Put(COVER_URI, "");
    DataShare::DataSharePredicates dataPredicates;
    dataPredicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    EXPECT_NE(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetCoverUri_coverUri_empty End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetCoverUri_update_err, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetCoverUri_update_err::Start");
    OperationType operationType = OperationType::PORTRAIT_COVER_URI;
    NativeRdb::ValuesBucket values;
    values.Put(COVER_URI, "eee");
    DataShare::DataSharePredicates dataPredicates;
    dataPredicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    CleanTestTables();
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetCoverUri_update_err End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, SetCoverUri_update_succ, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetCoverUri_update_succ::Start");
    OperationType operationType = OperationType::PORTRAIT_COVER_URI;
    NativeRdb::ValuesBucket values;
    values.Put(COVER_URI, "dd");
    DataShare::DataSharePredicates dataPredicates;
    dataPredicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/1/10/10.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "eww";
    targetColumn.displayLevel = FAVORITE_PAGE;
    InsertAlbumTestData(targetColumn);
    EXPECT_EQ(MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetCoverUri_update_succ End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, Maot_RenewDeletedPhotoAlbum_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Maot_RenewDeletedPhotoAlbum_test_001::Start");
    int32_t test_id = 10;
    int32_t ret = -1;
    NativeRdb::ValuesBucket values;
    std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>("test1");
    ASSERT_NE(trans, nullptr);
    ret = MediaLibraryAlbumOperations::RenewDeletedPhotoAlbum(test_id, values, trans);
    EXPECT_NE(ret, -1);
    MEDIA_INFO_LOG("Maot_RenewDeletedPhotoAlbum_test_001 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, Maot_DeleteHighlightAlbums_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Maot_DeleteHighlightAlbums_test_001::Start");
    int32_t ret = -1;
    NativeRdb::RdbPredicates predicates("test");
    ret = MediaLibraryAlbumOperations::DeleteHighlightAlbums(predicates);
    EXPECT_NE(ret, -1);
    MEDIA_INFO_LOG("Maot_DeleteHighlightAlbums_test_001 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, Maot_QueryPhotoAlbum_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Maot_QueryPhotoAlbum_test_001::Start");
    Uri analysisAlbumUri(CONST_PAH_INSERT_ANA_PHOTO_ALBUM);
    MediaLibraryCommand cmd(analysisAlbumUri);
    vector<string> columns;
    EXPECT_NE(MediaLibraryAlbumOperations::QueryPhotoAlbum(cmd, columns), nullptr);
    MEDIA_INFO_LOG("Maot_QueryPhotoAlbum_test_001 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, Maot_GetLPathFromSourcePath_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Maot_GetLPathFromSourcePath_test_001::Start");
    int32_t ret = -1;
    std::string sourcePath = "/storage/emulated/0/Documents/File.txt";
    std::string lPath = "test";
    int32_t mediaType = 0;
    ret = MediaLibraryAlbumOperations::GetLPathFromSourcePath(sourcePath, lPath, mediaType);
    EXPECT_NE(ret, -1);
    MEDIA_INFO_LOG("Maot_GetLPathFromSourcePath_test_001 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, Maot_RecoverAlbum_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Maot_RecoverAlbum_test_001::Start");
    bool isUserAlbum = false;
    std::string assetId = "/storage/emulated/1/Documents/File.txt";
    std::string lPath = "/storage/emulated/0/Documents/File.txt";
    int64_t newAlbumId = 0;
    MediaLibraryAlbumOperations::RecoverAlbum(assetId, lPath, isUserAlbum, newAlbumId);
    EXPECT_EQ(isUserAlbum, false);
    MEDIA_INFO_LOG("Maot_RecoverAlbum_test_001 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, Maot_HandleAnalysisPhotoAlbum_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Maot_HandleAnalysisPhotoAlbum_test_001::Start");
    OperationType opType = OperationType::HIGHLIGHT_ALBUM_NAME;
    int32_t ret;
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    std::shared_ptr<int> countPtr;
    ret = MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates, countPtr);
    EXPECT_NE(ret, E_ERR);
    MEDIA_INFO_LOG("Maot_HandleAnalysisPhotoAlbum_test_001 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, Maot_HandleAnalysisPhotoAlbum_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Maot_HandleAnalysisPhotoAlbum_test_002::Start");
    OperationType opType = OperationType::HIGHLIGHT_COVER_URI;
    int32_t ret;
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    std::shared_ptr<int> countPtr;
    ret = MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates, countPtr);
    EXPECT_NE(ret, E_ERR);
    MEDIA_INFO_LOG("Maot_HandleAnalysisPhotoAlbum_test_002 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, Maot_HandleAnalysisPhotoAlbum_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Maot_HandleAnalysisPhotoAlbum_test_003::Start");
    OperationType opType = OperationType::HIGHLIGHT_SUBTITLE;
    int32_t ret;
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    std::shared_ptr<int> countPtr;
    ret = MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates, countPtr);
    EXPECT_NE(ret, E_ERR);
    MEDIA_INFO_LOG("Maot_HandleAnalysisPhotoAlbum_test_003 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, Maot_HandleAnalysisPhotoAlbum_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Maot_HandleAnalysisPhotoAlbum_test_004::Start");
    OperationType opType = OperationType::GROUP_COVER_URI;
    int32_t ret;
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    std::shared_ptr<int> countPtr;
    ret = MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates, countPtr);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("Maot_HandleAnalysisPhotoAlbum_test_004 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, Maot_HandleAnalysisPhotoAlbum_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Maot_HandleAnalysisPhotoAlbum_test_005::Start");
    OperationType opType = OperationType::SET_LOCATION;
    int32_t ret;
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    std::shared_ptr<int> countPtr;
    ret = MediaLibraryAlbumOperations::HandleAnalysisPhotoAlbum(opType, values, predicates, countPtr);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("Maot_HandleAnalysisPhotoAlbum_test_005 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, Maot_HandlePhotoAlbumOperations_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Maot_HandlePhotoAlbumOperations_test_001::Start");
    Uri analysisAlbumUri(CONST_PAH_INSERT_ANA_PHOTO_ALBUM);
    int ret;
    MediaLibraryCommand cmd(analysisAlbumUri);
    cmd.oprnType_ = OperationType::SET_LOCATION;
    ret = MediaLibraryAlbumOperations::HandlePhotoAlbumOperations(cmd);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("Maot_HandlePhotoAlbumOperations_test_001 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, Maot_RenewDeletedPhotoAlbum_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Maot_RenewDeletedPhotoAlbum_test_002::Start");
    int32_t id = 1;
    NativeRdb::ValuesBucket albumValues;
    std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>("test");
    ASSERT_NE(trans, nullptr);
    int32_t ret = MediaLibraryAlbumOperations::RenewDeletedPhotoAlbum(id, albumValues, trans);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);

    trans = nullptr;
    ret = MediaLibraryAlbumOperations::RenewDeletedPhotoAlbum(id, albumValues, trans);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("Maot_RenewDeletedPhotoAlbum_test_002 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, IsCoverInAlbum_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("IsCoverInAlbum_test_001::Start");
    string fileId = "1";
    int32_t albumSubtype = 7;
    int32_t albumId = 7;
    bool ret = MediaLibraryAlbumOperations::IsCoverInAlbum(fileId, albumSubtype, albumId);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("IsCoverInAlbum_test_001 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, IsCoverInAlbum_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("IsCoverInAlbum_test_002::Start");
    string fileId = "1";
    int32_t albumSubtype = -1;
    int32_t albumId = 7;
    bool ret = MediaLibraryAlbumOperations::IsCoverInAlbum(fileId, albumSubtype, albumId);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("IsCoverInAlbum_test_002 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, IsManunalCloudCover_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("IsManunalCloudCover_test_001::Start");
    string fileId = "0";
    string coverCloudId = "0,0";
    bool ret = MediaLibraryAlbumOperations::IsManunalCloudCover(fileId, coverCloudId);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("IsManunalCloudCover_test_001 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, UpdateCoverUriEXecute_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateCoverUriEXecute_test_001::Start");
    int32_t albumId = -1;
    SetCoverUriAlbumInfo albumInfo;
    albumInfo.albumId = albumId;
    string fileId = "0";
    string coverUri = "file://media/Photo" + fileId;
    AlbumAccurateRefresh albumRefresh;
    bool ret = MediaLibraryAlbumOperations::UpdateCoverUriExecute(albumInfo, coverUri, fileId, 0, albumRefresh);
    EXPECT_NE(ret, true);
    MEDIA_INFO_LOG("UpdateCoverUriEXecute_test_001 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, UpdateAlbumCoverUri_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateAlbumCoverUri_test_001::Start");
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    string fileId = "0";
    string coverUri = "file://media/Photo" + fileId;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, 1);
    values.Put(PhotoAlbumColumns::ALBUM_COVER_URI, coverUri);
    values.Put(PhotoAlbumColumns::ALBUM_SUBTYPE, 1);
    auto ret = MediaLibraryAlbumOperations::UpdateAlbumCoverUri(values, predicates, false);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("UpdateAlbumCoverUri_test_001 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, UpdateAlbumCoverUri_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateAlbumCoverUri_test_002::Start");
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    string fileId = "0";
    string coverUri = "file://media/Photo" + fileId;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, 1);
    values.Put(PhotoAlbumColumns::ALBUM_COVER_URI, coverUri);
    values.Put(PhotoAlbumColumns::ALBUM_SUBTYPE, 1);
    auto ret = MediaLibraryAlbumOperations::UpdateAlbumCoverUri(values, predicates, true);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("UpdateAlbumCoverUri_test_002 End");
}

HWTEST_F(MediaLibraryAlbumOperationTest, ResetCoverUri_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ResetCoverUri_test_001::Start");
    string albumId = "1";
    int32_t albumSubtype = 1;
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, albumSubtype);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    values.Put(PhotoAlbumColumns::ALBUM_SUBTYPE, 1);
    auto ret = MediaLibraryAlbumOperations::ResetCoverUri(values, predicates);
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("UpdateAlbumCoverUri_test_002 End");
}
} // namespace Media
} // namespace OHOS

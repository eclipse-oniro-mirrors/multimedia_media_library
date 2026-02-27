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

#define MLOG_TAG "AspectRatioOperationUnitTest"

#include "medialibrary_aspect_ratio_operation_test.h"


#include "abs_rdb_predicates.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"
#include "medialibrary_type_const.h"
#include "photo_album_column.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_errno.h"

#include "medialibrary_common_utils.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_aspect_ratio_operation.h"
#include "media_upgrade.h"

using namespace std;
using namespace OHOS::NativeRdb;
using namespace testing::ext;

namespace OHOS {
namespace Media {
static shared_ptr<MediaLibraryRdbStore> g_rdbStore;

static void CleanTestTables()
{
    string dropSql = "DROP TABLE " + PhotoColumn::PHOTOS_TABLE + ";";
    int32_t ret = g_rdbStore->ExecuteSql(dropSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Drop table failed");
        return;
    }
    MEDIA_ERR_LOG("Drop table success");
}

static void SetTables()
{
    int32_t ret = g_rdbStore->ExecuteSql(PhotoUpgrade::CREATE_PHOTO_TABLE);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute photo table creation SQL failed");
        return;
    }
    MEDIA_ERR_LOG("Create photo table success");
}

static void ClearAndRestart()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    CleanTestTables();
    SetTables();
}

void MediaLibraryAspectRatioOperationTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    SetTables();
}

void MediaLibraryAspectRatioOperationTest::TearDownTestCase(void)
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    ClearAndRestart();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    this_thread::sleep_for(chrono::seconds(1));
    MEDIA_INFO_LOG("Clean is finish");
}

// SetUp:Execute before each test case
void MediaLibraryAspectRatioOperationTest::SetUp()
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearAndRestart();
}

void MediaLibraryAspectRatioOperationTest::TearDown(void) {}

static void InsertTestPhotoRecord(int32_t fileId, int32_t width, int32_t height, int32_t aspectRatio)
{
    CHECK_AND_RETURN_LOG(g_rdbStore != nullptr, "g_rdbStore is null, insert failed");

    std::string insertSql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "(" +
                            PhotoColumn::MEDIA_ID + ", " +
                            PhotoColumn::PHOTO_WIDTH + ", " +
                            PhotoColumn::PHOTO_HEIGHT + ", " +
                            PhotoColumn::PHOTO_ASPECT_RATIO + ") " +
                            "VALUES( " +
                            std::to_string(fileId) + ", " +
                            std::to_string(width) + ", " +
                            std::to_string(height) + ", " +
                            std::to_string(aspectRatio) + ");";

    int32_t dbRet = g_rdbStore->ExecuteSql(insertSql);

    EXPECT_EQ(dbRet, NativeRdb::E_OK) << "Insert SQL failed: " << insertSql;

    if (dbRet == NativeRdb::E_OK) {
        MEDIA_INFO_LOG("Insert test record success, fileId=%{public}d, aspectRatio=%{public}d", fileId, aspectRatio);
    } else {
        MEDIA_ERR_LOG("Execute sql %{public}s failed, ret=%{public}d", insertSql.c_str(), dbRet);
    }
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, get_unfilled_values_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("get_unfilled_values_test_001 Start");
    InsertTestPhotoRecord(666, 1, 1, 1);
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 0);
    MEDIA_INFO_LOG("get_unfilled_values_test_001 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, get_unfilled_values_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("get_unfilled_values_test_002 Start");
    InsertTestPhotoRecord(667, 1, 2, -2);
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 1);
    EXPECT_EQ(assetInfos[0].fileId, 667);
    EXPECT_EQ(assetInfos[0].aspect_ratio, 0.5);
    MEDIA_INFO_LOG("get_unfilled_values_test_002 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, query_unfilled_values_count_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("query_unfilled_values_count_test_001 Start");
    InsertTestPhotoRecord(555, 1, 1, 1);
    InsertTestPhotoRecord(666, 1, 1, 1);
    auto count = MediaLibraryAspectRatioOperation::QueryUnfilledValueCount();
    EXPECT_EQ(count, 0);
    MEDIA_INFO_LOG("query_unfilled_values_count_test_001 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, query_unfilled_values_count_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("query_unfilled_values_count_test_002 Start");
    InsertTestPhotoRecord(555, 1, 1, -2);
    InsertTestPhotoRecord(556, 1, 1, -2);
    InsertTestPhotoRecord(666, 1, 1, 1);
    InsertTestPhotoRecord(667, 1, 1, 1);
    auto count = MediaLibraryAspectRatioOperation::QueryUnfilledValueCount();
    EXPECT_EQ(count, 2);
    MEDIA_INFO_LOG("query_unfilled_values_count_test_002 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, get_unfilled_values_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("get_unfilled_values_test_003 Start");
    InsertTestPhotoRecord(668, 0, 1, -2);
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 1);
    EXPECT_EQ(assetInfos[0].fileId, 668);
    MEDIA_INFO_LOG("get_unfilled_values_test_003 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, get_unfilled_values_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("get_unfilled_values_test_004 Start");
    InsertTestPhotoRecord(669, 1, 0, -2);
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 1);
    EXPECT_EQ(assetInfos[0].fileId, 669);
    MEDIA_INFO_LOG("get_unfilled_values_test_004 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, get_unfilled_values_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("get_unfilled_values_test_005 Start");
    InsertTestPhotoRecord(670, 1920, 1080, -2);
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 1);
    EXPECT_EQ(assetInfos[0].fileId, 670);
    EXPECT_NEAR(assetInfos[0].aspect_ratio, 1.777, 0.01);
    MEDIA_INFO_LOG("get_unfilled_values_test_005 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, get_unfilled_values_test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("get_unfilled_values_test_006 Start");
    InsertTestPhotoRecord(671, 1080, 1920, -2);
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 1);
    EXPECT_EQ(assetInfos[0].fileId, 671);
    EXPECT_NEAR(assetInfos[0].aspect_ratio, 0.562, 0.001);
    MEDIA_INFO_LOG("get_unfilled_values_test_006 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, get_unfilled_values_test_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("get_unfilled_values_test_007 Start");
    InsertTestPhotoRecord(672, 1000, 1000, -2);
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 1);
    EXPECT_EQ(assetInfos[0].fileId, 672);
    EXPECT_NEAR(assetInfos[0].aspect_ratio, 1.0, 0.001);
    MEDIA_INFO_LOG("get_unfilled_values_test_007 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, get_unfilled_values_test_008, TestSize.Level0)
{
    MEDIA_INFO_LOG("get_unfilled_values_test_008 Start");
    InsertTestPhotoRecord(673, 3840, 2160, -2);
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 1);
    EXPECT_EQ(assetInfos[0].fileId, 673);
    EXPECT_NEAR(assetInfos[0].aspect_ratio, 1.777, 0.01);
    MEDIA_INFO_LOG("get_unfilled_values_test_008 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, get_unfilled_values_test_009, TestSize.Level0)
{
    MEDIA_INFO_LOG("get_unfilled_values_test_009 Start");
    for (int i = 0; i < 10; i++) {
        InsertTestPhotoRecord(680 + i, 1920 + i, 1080 + i, -2);
    }
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 1);
    MEDIA_INFO_LOG("get_unfilled_values_test_009 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, query_unfilled_values_count_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("query_unfilled_values_count_test_003 Start");
    auto count = MediaLibraryAspectRatioOperation::QueryUnfilledValueCount();
    EXPECT_EQ(count, 0);
    MEDIA_INFO_LOG("query_unfilled_values_count_test_003 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, query_unfilled_values_count_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("query_unfilled_values_count_test_004 Start");
    InsertTestPhotoRecord(557, 1920, 1080, -2);
    InsertTestPhotoRecord(558, 1080, 1920, -2);
    InsertTestPhotoRecord(559, 1000, 1000, -2);
    auto count = MediaLibraryAspectRatioOperation::QueryUnfilledValueCount();
    EXPECT_EQ(count, 3);
    MEDIA_INFO_LOG("query_unfilled_values_count_test_004 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, query_unfilled_values_count_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("query_unfilled_values_count_test_005 Start");
    for (int i = 0; i < 20; i++) {
        InsertTestPhotoRecord(600 + i, 1920, 1080, -2);
    }
    auto count = MediaLibraryAspectRatioOperation::QueryUnfilledValueCount();
    EXPECT_EQ(count, 20);
    MEDIA_INFO_LOG("query_unfilled_values_count_test_005 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, update_aspect_ratio_value_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("update_aspect_ratio_value_test_001 Start");
    InsertTestPhotoRecord(700, 1920, 1080, -2);
    MediaLibraryAspectRatioOperation::UpdateAspectRatioValue();
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 1);
    MEDIA_INFO_LOG("update_aspect_ratio_value_test_001 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, update_aspect_ratio_value_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("update_aspect_ratio_value_test_002 Start");
    InsertTestPhotoRecord(701, 1, 1, 1);
    MediaLibraryAspectRatioOperation::UpdateAspectRatioValue();
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 0);
    MEDIA_INFO_LOG("update_aspect_ratio_value_test_002 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, update_aspect_ratio_value_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("update_aspect_ratio_value_test_003 Start");
    for (int i = 0; i < 5; i++) {
        InsertTestPhotoRecord(710 + i, 1920 + i * 10, 1080 + i * 10, -2);
    }
    MediaLibraryAspectRatioOperation::UpdateAspectRatioValue();
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 1);
    MEDIA_INFO_LOG("update_aspect_ratio_value_test_003 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, update_aspect_ratio_value_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("update_aspect_ratio_value_test_004 Start");
    InsertTestPhotoRecord(720, 0, 1080, -2);
    MediaLibraryAspectRatioOperation::UpdateAspectRatioValue();
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 1);
    MEDIA_INFO_LOG("update_aspect_ratio_value_test_004 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, update_aspect_ratio_value_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("update_aspect_ratio_value_test_005 Start");
    InsertTestPhotoRecord(721, 1920, 0, -2);
    MediaLibraryAspectRatioOperation::UpdateAspectRatioValue();
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 1);
    MEDIA_INFO_LOG("update_aspect_ratio_value_test_005 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, comprehensive_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("comprehensive_test_001 Start");
    for (int i = 0; i < 10; i++) {
        InsertTestPhotoRecord(900 + i, 1920, 1080, -2);
    }
    auto countBefore = MediaLibraryAspectRatioOperation::QueryUnfilledValueCount();
    EXPECT_EQ(countBefore, 10);
    
    MediaLibraryAspectRatioOperation::UpdateAspectRatioValue();
    
    auto countAfter = MediaLibraryAspectRatioOperation::QueryUnfilledValueCount();
    EXPECT_EQ(countAfter, 10);
    MEDIA_INFO_LOG("comprehensive_test_001 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, comprehensive_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("comprehensive_test_002 Start");
    std::vector<AssetAspectRatio> photoInfos;
    for (int i = 0; i < 5; i++) {
        InsertTestPhotoRecord(910 + i, 1920, 1080, -2);
        AssetAspectRatio info;
        info.fileId = 910 + i;
        info.aspect_ratio = 1.777;
        photoInfos.push_back(info);
    }
    
    MediaLibraryAspectRatioOperation::HandleAspectRatio(photoInfos);
    
    auto count = MediaLibraryAspectRatioOperation::QueryUnfilledValueCount();
    EXPECT_EQ(count, 5);
    MEDIA_INFO_LOG("comprehensive_test_002 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, comprehensive_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("comprehensive_test_003 Start");
    InsertTestPhotoRecord(920, 1920, 1080, -2);
    InsertTestPhotoRecord(921, 1080, 1920, -2);
    InsertTestPhotoRecord(922, 1000, 1000, -2);
    InsertTestPhotoRecord(923, 3840, 2160, -2);
    InsertTestPhotoRecord(924, 2160, 3840, -2);
    
    auto countBefore = MediaLibraryAspectRatioOperation::QueryUnfilledValueCount();
    EXPECT_EQ(countBefore, 5);
    
    MediaLibraryAspectRatioOperation::UpdateAspectRatioValue();
    
    auto countAfter = MediaLibraryAspectRatioOperation::QueryUnfilledValueCount();
    EXPECT_EQ(countAfter, 5);
    MEDIA_INFO_LOG("comprehensive_test_003 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, boundary_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("boundary_test_001 Start");
    InsertTestPhotoRecord(930, 1, 1, -2);
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 1);
    EXPECT_NEAR(assetInfos[0].aspect_ratio, 1.0, 0.001);
    MEDIA_INFO_LOG("boundary_test_001 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, boundary_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("boundary_test_002 Start");
    InsertTestPhotoRecord(931, 9999, 1, -2);
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 1);
    MEDIA_INFO_LOG("boundary_test_002 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, boundary_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("boundary_test_003 Start");
    InsertTestPhotoRecord(932, 1, 9999, -2);
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 1);
    EXPECT_NEAR(assetInfos[0].aspect_ratio, 0.0001, 0.01);
    MEDIA_INFO_LOG("boundary_test_003 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, edge_case_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("edge_case_test_001 Start");
    InsertTestPhotoRecord(940, -1, 1080, -2);
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 1);
    MEDIA_INFO_LOG("edge_case_test_001 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, edge_case_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("edge_case_test_002 Start");
    InsertTestPhotoRecord(941, 1920, -1, -2);
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 1);
    MEDIA_INFO_LOG("edge_case_test_002 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, edge_case_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("edge_case_test_003 Start");
    InsertTestPhotoRecord(942, 0, 0, -2);
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 1);
    MEDIA_INFO_LOG("edge_case_test_003 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, mixed_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("mixed_test_001 Start");
    InsertTestPhotoRecord(950, 1920, 1080, 1);
    InsertTestPhotoRecord(951, 1920, 1080, -2);
    InsertTestPhotoRecord(952, 1080, 1920, 1);
    InsertTestPhotoRecord(953, 1080, 1920, -2);
    
    auto count = MediaLibraryAspectRatioOperation::QueryUnfilledValueCount();
    EXPECT_EQ(count, 2);
    
    MediaLibraryAspectRatioOperation::UpdateAspectRatioValue();
    
    auto countAfter = MediaLibraryAspectRatioOperation::QueryUnfilledValueCount();
    EXPECT_EQ(countAfter, 2);
    MEDIA_INFO_LOG("mixed_test_001 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, performance_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("performance_test_001 Start");
    for (int i = 0; i < 100; i++) {
        InsertTestPhotoRecord(1000 + i, 1920, 1080, -2);
    }
    
    auto countBefore = MediaLibraryAspectRatioOperation::QueryUnfilledValueCount();
    EXPECT_EQ(countBefore, 100);
    
    MediaLibraryAspectRatioOperation::UpdateAspectRatioValue();
    
    auto countAfter = MediaLibraryAspectRatioOperation::QueryUnfilledValueCount();
    EXPECT_EQ(countAfter, 100);
    MEDIA_INFO_LOG("performance_test_001 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, get_unfilled_values_test_010, TestSize.Level0)
{
    MEDIA_INFO_LOG("get_unfilled_values_test_010 Start");
    InsertTestPhotoRecord(674, 2560, 1440, -2);
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 1);
    EXPECT_EQ(assetInfos[0].fileId, 674);
    EXPECT_NEAR(assetInfos[0].aspect_ratio, 1.777, 0.01);
    MEDIA_INFO_LOG("get_unfilled_values_test_010 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, get_unfilled_values_test_011, TestSize.Level0)
{
    MEDIA_INFO_LOG("get_unfilled_values_test_011 Start");
    InsertTestPhotoRecord(675, 1280, 720, -2);
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 1);
    EXPECT_EQ(assetInfos[0].fileId, 675);
    EXPECT_NEAR(assetInfos[0].aspect_ratio, 1.777, 0.01);
    MEDIA_INFO_LOG("get_unfilled_values_test_011 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, get_unfilled_values_test_012, TestSize.Level0)
{
    MEDIA_INFO_LOG("get_unfilled_values_test_012 Start");
    InsertTestPhotoRecord(676, 640, 480, -2);
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 1);
    EXPECT_EQ(assetInfos[0].fileId, 676);
    EXPECT_NEAR(assetInfos[0].aspect_ratio, 1.333, 0.001);
    MEDIA_INFO_LOG("get_unfilled_values_test_012 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, get_unfilled_values_test_013, TestSize.Level0)
{
    MEDIA_INFO_LOG("get_unfilled_values_test_013 Start");
    InsertTestPhotoRecord(677, 480, 640, -2);
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 1);
    EXPECT_EQ(assetInfos[0].fileId, 677);
    EXPECT_NEAR(assetInfos[0].aspect_ratio, 0.75, 0.001);
    MEDIA_INFO_LOG("get_unfilled_values_test_013 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, get_unfilled_values_test_014, TestSize.Level0)
{
    MEDIA_INFO_LOG("get_unfilled_values_test_014 Start");
    InsertTestPhotoRecord(678, 320, 240, -2);
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 1);
    EXPECT_EQ(assetInfos[0].fileId, 678);
    EXPECT_NEAR(assetInfos[0].aspect_ratio, 1.333, 0.001);
    MEDIA_INFO_LOG("get_unfilled_values_test_014 End End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, update_aspect_ratio_value_test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("update_aspect_ratio_value_test_006 Start");
    for (int i = 0; i < 50; i++) {
        InsertTestPhotoRecord(730 + i, 1920 + i, 1080 + i, -2);
    }
    MediaLibraryAspectRatioOperation::UpdateAspectRatioValue();
    std::vector<AssetAspectRatio> assetInfos = MediaLibraryAspectRatioOperation::GetUnfilledValues();
    EXPECT_EQ(assetInfos.size(), 1);
    MEDIA_INFO_LOG("update_aspect_ratio_value_test_006 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, query_unfilled_values_count_test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("query_unfilled_values_count_test_006 Start");
    InsertTestPhotoRecord(560, 1920, 1080, -2);
    InsertTestPhotoRecord(561, 1920, 1080, 1);
    InsertTestPhotoRecord(562, 1920, 1080, -2);
    InsertTestPhotoRecord(563, 1920, 1080, 1);
    auto count = MediaLibraryAspectRatioOperation::QueryUnfilledValueCount();
    EXPECT_EQ(count, 2);
    MEDIA_INFO_LOG("query_unfilled_values_count_test_006 End");
}

HWTEST_F(MediaLibraryAspectRatioOperationTest, query_unfilled_values_count_test_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("query_unfilled_values_count_test_007 Start");
    for (int i = 0; i < 5; i++) {
        InsertTestPhotoRecord(570 + i, 1920, 1080, -2);
    }
    for (int i = 0; i < 5; i++) {
        InsertTestPhotoRecord(575 + i, 1920, 1080, 1);
    }
    auto count = MediaLibraryAspectRatioOperation::QueryUnfilledValueCount();
    EXPECT_EQ(count, 5);
    MEDIA_INFO_LOG("query_unfilled_values_count_test_007 End");
}
}
}
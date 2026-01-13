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
}
}
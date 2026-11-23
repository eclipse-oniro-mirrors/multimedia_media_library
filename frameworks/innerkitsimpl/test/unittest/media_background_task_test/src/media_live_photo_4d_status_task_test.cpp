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

#include "media_live_photo_4d_status_task_test.h"

#include "medialibrary_errno.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_upgrade.h"
#include "medialibrary_type_const.h"
#include "medialibrary_photo_operations.h"
#include "result_set_utils.h"
#include "moving_photo_file_utils.h"

#include "media_live_photo_4d_status_task.h"

using namespace testing::ext;
using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS::Media::Background {
static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;

static void CleanTestTables()
{
    std::vector<std::string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
    };
    for (auto &dropTable : dropTableList) {
        std::string dropSql = "DROP TABLE " + dropTable + ";";
        int32_t ret = g_rdbStore->ExecuteSql(dropSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Drop %{public}s table failed", dropTable.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Drop %{public}s table success", dropTable.c_str());
    }
}

static void SetTables()
{
    std::vector<std::string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
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

void MediaLivePhoto4dStatusTaskTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLivePhoto4dStatusTaskTest failed, can not get g_rdbStore");
        exit(1);
    }
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("MediaLivePhoto4dStatusTaskTest SetUpTestCase");
}

void MediaLivePhoto4dStatusTaskTest::TearDownTestCase(void)
{
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("MediaLivePhoto4dStatusTaskTest TearDownTestCase");
}

void MediaLivePhoto4dStatusTaskTest::SetUp() {}

void MediaLivePhoto4dStatusTaskTest::TearDown(void) {}

HWTEST_F(MediaLivePhoto4dStatusTaskTest, Accept_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Accept_test_001 start");
    auto mediaLivePhoto4dStatusTask = std::make_shared<MediaLivePhoto4dStatusTask>();
    ASSERT_NE(mediaLivePhoto4dStatusTask, nullptr);
    bool res = mediaLivePhoto4dStatusTask->Accept();
    MEDIA_INFO_LOG("Accept_test_001 end, result: %{public}d", res);
}

HWTEST_F(MediaLivePhoto4dStatusTaskTest, Execute_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Execute_test_001 start");
    auto mediaLivePhoto4dStatusTask = std::make_shared<MediaLivePhoto4dStatusTask>();
    ASSERT_NE(mediaLivePhoto4dStatusTask, nullptr);
    mediaLivePhoto4dStatusTask->Execute();
    MEDIA_INFO_LOG("Execute_test_001 end");
}

HWTEST_F(MediaLivePhoto4dStatusTaskTest, HandleLivePhoto4dStatus_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("HandleLivePhoto4dStatus_test_001 start");
    auto mediaLivePhoto4dStatusTask = std::make_shared<MediaLivePhoto4dStatusTask>();
    ASSERT_NE(mediaLivePhoto4dStatusTask, nullptr);
    mediaLivePhoto4dStatusTask->SetBatchStatus(0);
    mediaLivePhoto4dStatusTask->HandleLivePhoto4dStatus();
    MEDIA_INFO_LOG("HandleLivePhoto4dStatus_test_001 end");
}

HWTEST_F(MediaLivePhoto4dStatusTaskTest, HandleLivePhoto4dStatus_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("HandleLivePhoto4dStatus_test_002 start");
    auto mediaLivePhoto4dStatusTask = std::make_shared<MediaLivePhoto4dStatusTask>();
    ASSERT_NE(mediaLivePhoto4dStatusTask, nullptr);
    mediaLivePhoto4dStatusTask->SetBatchStatus(100);
    mediaLivePhoto4dStatusTask->HandleLivePhoto4dStatus();
    MEDIA_INFO_LOG("HandleLivePhoto4dStatus_test_002 end");
}

HWTEST_F(MediaLivePhoto4dStatusTaskTest, SetBatchStatus_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("SetBatchStatus_test_001 start");
    auto mediaLivePhoto4dStatusTask = std::make_shared<MediaLivePhoto4dStatusTask>();
    ASSERT_NE(mediaLivePhoto4dStatusTask, nullptr);
    mediaLivePhoto4dStatusTask->SetBatchStatus(0);
    int32_t status = mediaLivePhoto4dStatusTask->GetBatchStatus();
    EXPECT_EQ(status, 0);
    MEDIA_INFO_LOG("SetBatchStatus_test_001 end, status: %{public}d", status);
}

HWTEST_F(MediaLivePhoto4dStatusTaskTest, SetBatchStatus_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("SetBatchStatus_test_002 start");
    auto mediaLivePhoto4dStatusTask = std::make_shared<MediaLivePhoto4dStatusTask>();
    ASSERT_NE(mediaLivePhoto4dStatusTask, nullptr);
    mediaLivePhoto4dStatusTask->SetBatchStatus(100);
    int32_t status = mediaLivePhoto4dStatusTask->GetBatchStatus();
    EXPECT_EQ(status, 100);
    MEDIA_INFO_LOG("SetBatchStatus_test_002 end, status: %{public}d", status);
}

HWTEST_F(MediaLivePhoto4dStatusTaskTest, SetBatchStatus_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("SetBatchStatus_test_003 start");
    auto mediaLivePhoto4dStatusTask = std::make_shared<MediaLivePhoto4dStatusTask>();
    ASSERT_NE(mediaLivePhoto4dStatusTask, nullptr);
    mediaLivePhoto4dStatusTask->SetBatchStatus(1000);
    mediaLivePhoto4dStatusTask->SetBatchStatus(0);
    int32_t status = mediaLivePhoto4dStatusTask->GetBatchStatus();
    EXPECT_EQ(status, 0);
    MEDIA_INFO_LOG("SetBatchStatus_test_003 end, status: %{public}d", status);
}

HWTEST_F(MediaLivePhoto4dStatusTaskTest, GetBatchStatus_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetBatchStatus_test_001 start");
    auto mediaLivePhoto4dStatusTask = std::make_shared<MediaLivePhoto4dStatusTask>();
    ASSERT_NE(mediaLivePhoto4dStatusTask, nullptr);
    mediaLivePhoto4dStatusTask->SetBatchStatus(50);
    int32_t status = mediaLivePhoto4dStatusTask->GetBatchStatus();
    EXPECT_EQ(status, 50);
    MEDIA_INFO_LOG("GetBatchStatus_test_001 end");
}

HWTEST_F(MediaLivePhoto4dStatusTaskTest, QueryLivePhoto4d_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("QueryLivePhoto4d_test_001 start");
    auto mediaLivePhoto4dStatusTask = std::make_shared<MediaLivePhoto4dStatusTask>();
    ASSERT_NE(mediaLivePhoto4dStatusTask, nullptr);
    auto resultSet = mediaLivePhoto4dStatusTask->QueryLivePhoto4d(0);
    MEDIA_INFO_LOG("QueryLivePhoto4d_test_001 end");
}

HWTEST_F(MediaLivePhoto4dStatusTaskTest, QueryLivePhoto4d_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("QueryLivePhoto4d_test_002 start");
    auto mediaLivePhoto4dStatusTask = std::make_shared<MediaLivePhoto4dStatusTask>();
    ASSERT_NE(mediaLivePhoto4dStatusTask, nullptr);
    auto resultSet = mediaLivePhoto4dStatusTask->QueryLivePhoto4d(100);
    MEDIA_INFO_LOG("QueryLivePhoto4d_test_002 end");
}

HWTEST_F(MediaLivePhoto4dStatusTaskTest, ParseLivePhoto4dData_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("ParseLivePhoto4dData_test_001 start");
    auto mediaLivePhoto4dStatusTask = std::make_shared<MediaLivePhoto4dStatusTask>();
    ASSERT_NE(mediaLivePhoto4dStatusTask, nullptr);
    std::shared_ptr<NativeRdb::ResultSet> resultSet = nullptr;
    std::vector<LivePhoto4dData> dataList;
    bool ret = mediaLivePhoto4dStatusTask->ParseLivePhoto4dData(resultSet, dataList);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("ParseLivePhoto4dData_test_001 end");
}

HWTEST_F(MediaLivePhoto4dStatusTaskTest, UpdateLivePhoto4dStatus_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("UpdateLivePhoto4dStatus_test_001 start");
    auto mediaLivePhoto4dStatusTask = std::make_shared<MediaLivePhoto4dStatusTask>();
    ASSERT_NE(mediaLivePhoto4dStatusTask, nullptr);
    int32_t ret = mediaLivePhoto4dStatusTask->UpdateLivePhoto4dStatus(1,
        static_cast<int32_t>(LivePhoto4dStatusType::TYPE_UNIDENTIFIED));
    MEDIA_INFO_LOG("UpdateLivePhoto4dStatus_test_001 end, ret: %{public}d", ret);
}

HWTEST_F(MediaLivePhoto4dStatusTaskTest, UpdateLivePhoto4dStatus_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("UpdateLivePhoto4dStatus_test_002 start");
    auto mediaLivePhoto4dStatusTask = std::make_shared<MediaLivePhoto4dStatusTask>();
    ASSERT_NE(mediaLivePhoto4dStatusTask, nullptr);
    int32_t ret = mediaLivePhoto4dStatusTask->UpdateLivePhoto4dStatus(100,
        static_cast<int32_t>(LivePhoto4dStatusType::TYPE_SUPPORTED));
    MEDIA_INFO_LOG("UpdateLivePhoto4dStatus_test_002 end, ret: %{public}d", ret);
}

HWTEST_F(MediaLivePhoto4dStatusTaskTest, UpdateLivePhoto4dStatus_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("UpdateLivePhoto4dStatus_test_003 start");
    auto mediaLivePhoto4dStatusTask = std::make_shared<MediaLivePhoto4dStatusTask>();
    ASSERT_NE(mediaLivePhoto4dStatusTask, nullptr);
    int32_t ret = mediaLivePhoto4dStatusTask->UpdateLivePhoto4dStatus(999,
        static_cast<int32_t>(LivePhoto4dStatusType::TYPE_USED));
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("UpdateLivePhoto4dStatus_test_003 end, ret: %{public}d", ret);
}

HWTEST_F(MediaLivePhoto4dStatusTaskTest, ProcessLivePhoto4d_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("ProcessLivePhoto4d_test_002 start");
    auto mediaLivePhoto4dStatusTask = std::make_shared<MediaLivePhoto4dStatusTask>();
    ASSERT_NE(mediaLivePhoto4dStatusTask, nullptr);
    std::vector<LivePhoto4dData> dataList;
    LivePhoto4dData data;
    data.fileId = 1;
    data.path = "/storage/media/local/files/Photo/test.jpg";
    data.extraDataPath = "/storage/media/local/files/Photo/test.jpg_extra";
    dataList.push_back(data);
    mediaLivePhoto4dStatusTask->ProcessLivePhoto4d(dataList);
    MEDIA_INFO_LOG("ProcessLivePhoto4d_test_002 end");
}

HWTEST_F(MediaLivePhoto4dStatusTaskTest, ProcessLivePhoto4d_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("ProcessLivePhoto4d_test_003 start");
    auto mediaLivePhoto4dStatusTask = std::make_shared<MediaLivePhoto4dStatusTask>();
    ASSERT_NE(mediaLivePhoto4dStatusTask, nullptr);
    std::vector<LivePhoto4dData> dataList;
    for (int i = 0; i < 5; i++) {
        LivePhoto4dData data;
        data.fileId = i + 1;
        data.path = "/storage/media/local/files/Photo/test" + std::to_string(i) + ".jpg";
        data.extraDataPath = "/storage/media/local/files/Photo/test" + std::to_string(i) + ".jpg_extra";
        dataList.push_back(data);
    }
    mediaLivePhoto4dStatusTask->ProcessLivePhoto4d(dataList);
    MEDIA_INFO_LOG("ProcessLivePhoto4d_test_003 end");
}

HWTEST_F(MediaLivePhoto4dStatusTaskTest, QueryLivePhoto4dWithRealData_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("QueryLivePhoto4dWithRealData_test_001 start");
    auto mediaLivePhoto4dStatusTask = std::make_shared<MediaLivePhoto4dStatusTask>();
    ASSERT_NE(mediaLivePhoto4dStatusTask, nullptr);

    ValuesBucket values;
    values.PutString(PhotoColumn::MEDIA_NAME, "moving_photo_test.jpg");
    values.PutInt(MediaColumn::MEDIA_TYPE, MediaType::MEDIA_TYPE_IMAGE);
    values.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    values.PutInt(PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_STATUS,
        static_cast<int32_t>(LivePhoto4dStatusType::TYPE_UNIDENTIFIED));

    int64_t outRowId = -1;
    int32_t insertResult = g_rdbStore->Insert(outRowId, PhotoColumn::PHOTOS_TABLE, values);
    ASSERT_EQ(insertResult, NativeRdb::E_OK);
    ASSERT_GT(outRowId, 0);

    auto resultSet = mediaLivePhoto4dStatusTask->QueryLivePhoto4d(0);
    ASSERT_NE(resultSet, nullptr);

    int32_t rowCount = 0;
    int32_t ret = resultSet->GetRowCount(rowCount);
    ASSERT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GT(rowCount, 0);

    std::string querySql = "SELECT " + PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_STATUS +
        " FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " + PhotoColumn::MEDIA_ID + " = " + to_string(outRowId);
    auto verifyResultSet = g_rdbStore->QuerySql(querySql);
    ASSERT_NE(verifyResultSet, nullptr);
    ASSERT_EQ(verifyResultSet->GoToFirstRow(), NativeRdb::E_OK);
    int32_t dbStatus = GetInt32Val(PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_STATUS, verifyResultSet);
    EXPECT_EQ(dbStatus, static_cast<int32_t>(LivePhoto4dStatusType::TYPE_UNIDENTIFIED));

    MEDIA_INFO_LOG("QueryLivePhoto4dWithRealData_test_001 end, rowCount: %{public}d", rowCount);
}
} // namespace OHOS::Media::Background
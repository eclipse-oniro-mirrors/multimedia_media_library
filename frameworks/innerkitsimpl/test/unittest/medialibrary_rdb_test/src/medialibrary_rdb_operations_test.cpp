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
#define MLOG_TAG "MediaLibraryRdbOperationsTest"

#include "medialibrary_rdb_operations_test.h"

#include <chrono>
#include <cstdint>
#include <thread>

#include "ability_context_impl.h"
#include "context.h"
#include "js_runtime.h"
#include "media_log.h"
#include "media_upgrade.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdb_operations.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "result_set_utils.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace NativeRdb;

namespace OHOS {
namespace Media {

static constexpr int32_t SLEEP_SECONDS = 1;
static shared_ptr<MediaLibraryRdbStore> rdbStorePtr = nullptr;

static std::vector<std::string> createTableSqlLists = {
    PhotoUpgrade::CREATE_PHOTO_TABLE,
};

static std::vector<std::string> testTables = {
    PhotoColumn::PHOTOS_TABLE,
};

/**
 * 查询测试结果 - 根据fileId查询照片信息
 */
static shared_ptr<NativeRdb::ResultSet> QueryForTestResult(int64_t fileId)
{
    MEDIA_INFO_LOG("QueryForTestResult, fileId: %{public}" PRId64, fileId);
    static const vector<string> COLUMNS = {
        PhotoColumn::MEDIA_ID,
        PhotoColumn::PHOTO_LAST_VISIT_TIME,
    };

    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, fileId);

    EXPECT_NE(rdbStorePtr, nullptr);
    return rdbStorePtr->Query(predicates, COLUMNS);
}

void MediaLibraryRdbOperationsTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::InitUnistore();
    rdbStorePtr = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(rdbStorePtr,  nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(rdbStorePtr, createTableSqlLists);
    MEDIA_INFO_LOG("SetUpTestCase MediaLibraryRdbOperationsTest succeed");
}

void MediaLibraryRdbOperationsTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(rdbStorePtr, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("MediaLibraryRdbOperationsTest TearDownTestCase done");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void MediaLibraryRdbOperationsTest::SetUp()
{
    MediaLibraryUnitTestUtils::CleanTestTables(rdbStorePtr, testTables, false);
}

void MediaLibraryRdbOperationsTest::TearDown() {}

/**
 * @tc.name: [正常场景: 更新最后访问时间] MediaLibraryRdbOperations_UpdateLastVisitTime_001
 * @tc.desc: 测试UpdateLastVisitTime更新照片的最后访问时间
 *           [1] 插入测试照片数据
 *           [2] 调用UpdateLastVisitTime更新最后访问时间
 *           [3] 验证返回更改的行数为1
 *           [4] 验证数据库中的时间已更新
 */
HWTEST_F(MediaLibraryRdbOperationsTest, MediaLibraryRdbOperations_UpdateLastVisitTime_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbOperations_UpdateLastVisitTime_001");

    ValuesBucket values;
    values.PutString(PhotoColumn::MEDIA_FILE_PATH, "/data/test/photo.jpg");
    values.PutString(PhotoColumn::MEDIA_NAME, "test_photo.jpg");
    values.PutString(PhotoColumn::MEDIA_MIME_TYPE, "image/jpeg");
    values.PutInt(PhotoColumn::PHOTO_LAST_VISIT_TIME, 0);

    int64_t fileId = 0;
    int32_t ret = MediaLibraryRdbStore::Insert(fileId, PhotoColumn::PHOTOS_TABLE, values);
    EXPECT_EQ(ret, E_OK);
    EXPECT_GT(fileId, 0);

    string id = to_string(fileId);
    int32_t changedRows = MediaLibraryRdbOperations::UpdateLastVisitTime(id);
    EXPECT_EQ(changedRows, 1);

    auto resultSet = QueryForTestResult(fileId);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    int64_t lastVisitTime = GetInt64Val(PhotoColumn::PHOTO_LAST_VISIT_TIME, resultSet);
    resultSet->Close();
    EXPECT_GT(lastVisitTime, 0);
}

/**
 * @tc.name: [异常场景: 更新不存在的照片ID] MediaLibraryRdbOperations_UpdateLastVisitTime_002
 * @tc.desc: 测试UpdateLastVisitTime更新不存在的照片ID
 *           [1] 调用UpdateLastVisitTime更新不存在的ID
 *           [2] 验证返回更改的行数为0
 */
HWTEST_F(MediaLibraryRdbOperationsTest, MediaLibraryRdbOperations_UpdateLastVisitTime_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbOperations_UpdateLastVisitTime_002");

    string nonExistentId = "999999";
    int32_t changedRows = MediaLibraryRdbOperations::UpdateLastVisitTime(nonExistentId);
    EXPECT_EQ(changedRows, 0);
}

/**
 * @tc.name: [正常场景: 查询PRAGMA值] MediaLibraryRdbOperations_QueryPragma_001
 * @tc.desc: 测试QueryPragma查询数据库PRAGMA值
 *           [1] 调用QueryPragma查询user_version
 *           [2] 验证返回成功
 */
HWTEST_F(MediaLibraryRdbOperationsTest, MediaLibraryRdbOperations_QueryPragma_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbOperations_QueryPragma_001");

    int64_t value = 0;
    int32_t ret = MediaLibraryRdbOperations::QueryPragma("user_version", value);
    EXPECT_EQ(ret, E_OK);
    EXPECT_GE(value, 0);
}

/**
 * @tc.name: [异常场景: 查询不存在的PRAGMA] MediaLibraryRdbOperations_QueryPragma_002
 * @tc.desc: 测试QueryPragma查询不存在的PRAGMA
 *           [1] 调用QueryPragma查询不存在的key
 *           [2] 验证返回错误
 */
HWTEST_F(MediaLibraryRdbOperationsTest, MediaLibraryRdbOperations_QueryPragma_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryRdbOperations_QueryPragma_002");

    int64_t value = 0;
    int32_t ret = MediaLibraryRdbOperations::QueryPragma("invalid_pragma_key", value);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

} // namespace Media
} // namespace OHOS
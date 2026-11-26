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

#include "media_update_local_asset_size_task_test.h"

#include <fstream>

#include "medialibrary_errno.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "media_file_utils.h"
#include "media_pure_file_utils.h"
#include "media_log.h"
#include "media_upgrade.h"
#include "media_column.h"
#include "media_update_local_asset_size_task.h"
#include "photo_album_column.h"
#include "result_set_utils.h"

using namespace testing::ext;
using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS::Media::Background {
static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;

const int64_t SIZE_DEFAULT = 0;
const int64_t SIZE_VALUE = 100;

static void CleanTestTables()
{
    std::vector<std::string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        PhotoAlbumColumns::TABLE,
    };
    for (auto &dropTable : dropTableList) {
        std::string dropSql = "DROP TABLE IF EXISTS " + dropTable + ";";
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
        PhotoAlbumColumns::CREATE_TABLE,
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

void MediaUpdateLocalAssetSizeTaskTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaUpdateLocalAssetSizeTaskTest failed, can not get g_rdbStore");
        exit(1);
    }
    system("rm -rf /data/test/*");
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("MediaUpdateLocalAssetSizeTaskTest SetUpTestCase");
}

void MediaUpdateLocalAssetSizeTaskTest::TearDownTestCase(void)
{
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("MediaUpdateLocalAssetSizeTaskTest TearDownTestCase");
}

void MediaUpdateLocalAssetSizeTaskTest::SetUp()
{
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("MediaUpdateLocalAssetSizeTaskTest SetUp");
}

void MediaUpdateLocalAssetSizeTaskTest::TearDown(void)
{
    system("rm -rf /data/test/*");
}

static int64_t InsertPhoto(int64_t size, int64_t localAssetSize, int32_t position, bool isTemp, const std::string& path,
    int32_t movingPhotoEffectMode)
{
    ValuesBucket values;
    values.Put(MediaColumn::MEDIA_SIZE, size);
    values.Put(PhotoColumn::LOCAL_ASSET_SIZE, localAssetSize);
    values.Put(PhotoColumn::PHOTO_POSITION, position);
    values.Put(PhotoColumn::PHOTO_IS_TEMP, std::to_string(static_cast<int32_t>(isTemp)));
    values.Put(MediaColumn::MEDIA_FILE_PATH, path);
    values.Put(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, movingPhotoEffectMode);

    int64_t outRowId = -1;
    int32_t insertResult = g_rdbStore->Insert(outRowId, PhotoColumn::PHOTOS_TABLE, values);
    if (insertResult != NativeRdb::E_OK) {
        return -1;
    }
    return outRowId;
}

static int64_t QueryForLocalAssetSize(int64_t fileId)
{
    static const std::vector<std::string> COLUMNS = {
        PhotoColumn::LOCAL_ASSET_SIZE
    };

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr.");
        return -1;
    }

    NativeRdb::AbsRdbPredicates predicate(PhotoColumn::PHOTOS_TABLE);
    predicate.EqualTo(MediaColumn::MEDIA_ID, fileId);

    auto resultSet = rdbStore->Query(predicate, COLUMNS);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdbStore is nullptr.");
        return -1;
    }

    return GetInt64Val(PhotoColumn::LOCAL_ASSET_SIZE, resultSet);
}

// 测试 HandleUpdateLocalAssetSizeTask 方法: 纯云文件不处理
HWTEST_F(MediaUpdateLocalAssetSizeTaskTest, HandleUpdateLocalAssetSizeTask_test01, TestSize.Level1)
{
    MEDIA_INFO_LOG("HandleUpdateLocalAssetSizeTask_test01 start");

    // 1.数据准备
    int64_t fileId = InsertPhoto(SIZE_VALUE, SIZE_DEFAULT, static_cast<int32_t>(PhotoPositionType::CLOUD), false,
        "/data/test/GetLocalAssetSize/photo.jpg", static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT));
    EXPECT_EQ(fileId > 0, true);

    // 2.执行
    auto task = std::make_shared<MediaUpdateLocalAssetSizeTask>();
    ASSERT_NE(task, nullptr);
    QueryLocalAssetSizeStatus ret = task->HandleUpdateLocalAssetSizeTask();
    EXPECT_EQ(ret, QueryLocalAssetSizeStatus::NONE_DATA);

    // 3.value符合预期
    int64_t localAssetSize = QueryForLocalAssetSize(fileId);
    EXPECT_EQ(localAssetSize, SIZE_DEFAULT);
}

// 测试 HandleUpdateLocalAssetSizeTask 方法: position异常文件, 不处理
HWTEST_F(MediaUpdateLocalAssetSizeTaskTest, HandleUpdateLocalAssetSizeTask_test02, TestSize.Level1)
{
    MEDIA_INFO_LOG("HandleUpdateLocalAssetSizeTask_test02 start");

    // 1.数据准备
    int64_t fileId = InsertPhoto(SIZE_VALUE, SIZE_DEFAULT, static_cast<int32_t>(PhotoPositionType::INVALID), false,
        "/data/test/GetLocalAssetSize/photo.jpg", static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT));
    EXPECT_EQ(fileId > 0, true);

    // 2.执行
    auto task = std::make_shared<MediaUpdateLocalAssetSizeTask>();
    ASSERT_NE(task, nullptr);
    QueryLocalAssetSizeStatus ret = task->HandleUpdateLocalAssetSizeTask();
    EXPECT_EQ(ret, QueryLocalAssetSizeStatus::NONE_DATA);

    // 3.value符合预期
    int64_t localAssetSize = QueryForLocalAssetSize(fileId);
    EXPECT_EQ(localAssetSize, SIZE_DEFAULT);
}

// 测试 HandleUpdateLocalAssetSizeTask 方法: 临时文件不处理
HWTEST_F(MediaUpdateLocalAssetSizeTaskTest, HandleUpdateLocalAssetSizeTask_test03, TestSize.Level1)
{
    MEDIA_INFO_LOG("HandleUpdateLocalAssetSizeTask_test03 start");

    // 1.数据准备
    int64_t fileId = InsertPhoto(SIZE_VALUE, SIZE_DEFAULT, static_cast<int32_t>(PhotoPositionType::LOCAL), true,
        "/data/test/GetLocalAssetSize/photo.jpg", static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT));
    EXPECT_EQ(fileId > 0, true);

    // 2.执行
    auto task = std::make_shared<MediaUpdateLocalAssetSizeTask>();
    ASSERT_NE(task, nullptr);
    QueryLocalAssetSizeStatus ret = task->HandleUpdateLocalAssetSizeTask();
    EXPECT_EQ(ret, QueryLocalAssetSizeStatus::NONE_DATA);

    // 3.value符合预期
    int64_t localAssetSize = QueryForLocalAssetSize(fileId);
    EXPECT_EQ(localAssetSize, SIZE_DEFAULT);
}

// 测试 HandleUpdateLocalAssetSizeTask 方法: local_asset_size != 0 认定为不是脏数据, 不处理
HWTEST_F(MediaUpdateLocalAssetSizeTaskTest, HandleUpdateLocalAssetSizeTask_test04, TestSize.Level1)
{
    MEDIA_INFO_LOG("HandleUpdateLocalAssetSizeTask_test04 start");

    // 1.数据准备
    int64_t fileId = InsertPhoto(SIZE_VALUE, SIZE_VALUE, static_cast<int32_t>(PhotoPositionType::LOCAL), false,
        "/data/test/GetLocalAssetSize/photo.jpg", static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT));
    EXPECT_EQ(fileId > 0, true);

    // 2.执行
    auto task = std::make_shared<MediaUpdateLocalAssetSizeTask>();
    ASSERT_NE(task, nullptr);
    QueryLocalAssetSizeStatus ret = task->HandleUpdateLocalAssetSizeTask();
    EXPECT_EQ(ret, QueryLocalAssetSizeStatus::NONE_DATA);

    // 3.value符合预期
    int64_t localAssetSize = QueryForLocalAssetSize(fileId);
    EXPECT_EQ(localAssetSize, SIZE_VALUE);
}

// 测试 HandleUpdateLocalAssetSizeTask 方法: size = 0 认定为无法处理, 不处理
HWTEST_F(MediaUpdateLocalAssetSizeTaskTest, HandleUpdateLocalAssetSizeTask_test05, TestSize.Level1)
{
    MEDIA_INFO_LOG("HandleUpdateLocalAssetSizeTask_test05 start");

    // 1.数据准备
    int64_t fileId = InsertPhoto(SIZE_DEFAULT, SIZE_DEFAULT, static_cast<int32_t>(PhotoPositionType::LOCAL), false,
        "/data/test/GetLocalAssetSize/photo.jpg", static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT));
    EXPECT_EQ(fileId > 0, true);

    // 2.执行
    auto task = std::make_shared<MediaUpdateLocalAssetSizeTask>();
    ASSERT_NE(task, nullptr);
    QueryLocalAssetSizeStatus ret = task->HandleUpdateLocalAssetSizeTask();
    EXPECT_EQ(ret, QueryLocalAssetSizeStatus::NONE_DATA);

    // 3.value符合预期
    int64_t localAssetSize = QueryForLocalAssetSize(fileId);
    EXPECT_EQ(localAssetSize, SIZE_DEFAULT);
}

// 测试 HandleUpdateLocalAssetSizeTask 方法: 非纯云资产、非临时资产、size正常、local_asset_size = 0, 认定为历史脏数据
// 如果不是动态照片关闭效果模式, 则与size保持一致
HWTEST_F(MediaUpdateLocalAssetSizeTaskTest, HandleUpdateLocalAssetSizeTask_test06, TestSize.Level1)
{
    MEDIA_INFO_LOG("HandleUpdateLocalAssetSizeTask_test06 start");

    // 1.数据准备
    int64_t fileId = InsertPhoto(SIZE_VALUE, SIZE_DEFAULT, static_cast<int32_t>(PhotoPositionType::LOCAL), false,
        "/data/test/GetLocalAssetSize/photo.jpg", static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT));
    EXPECT_EQ(fileId > 0, true);

    // 2.执行
    auto task = std::make_shared<MediaUpdateLocalAssetSizeTask>();
    ASSERT_NE(task, nullptr);
    QueryLocalAssetSizeStatus ret = task->HandleUpdateLocalAssetSizeTask();
    EXPECT_EQ(ret, QueryLocalAssetSizeStatus::E_OK);

    // 3.value符合预期
    int64_t localAssetSize = QueryForLocalAssetSize(fileId);
    EXPECT_EQ(localAssetSize, SIZE_VALUE);
}

// 测试 HandleUpdateLocalAssetSizeTask 方法: 非纯云资产、非临时资产、size正常、local_asset_size = 0, 认定为历史脏数据
// 如果是动态照片关闭效果模式, 则读取实际文件大小, 与实际图片保持一致
HWTEST_F(MediaUpdateLocalAssetSizeTaskTest, HandleUpdateLocalAssetSizeTask_test07, TestSize.Level1)
{
    MEDIA_INFO_LOG("HandleUpdateLocalAssetSizeTask_test07 start");

    std::string dirPath = "/data/test/GetLocalAssetSize";
    EXPECT_EQ(MediaPureFileUtils::CreateDirectory(dirPath), true);
    std::string filePath = dirPath + "/" + "photo.jpg";
    EXPECT_EQ(MediaFileUtils::CreateFile(filePath), true);
    std::ofstream outFile(filePath, std::ios::binary | std::ios::trunc);
    if (outFile.is_open()) {
        outFile.write("A", 200);
        outFile.close();
    }

    // 1.数据准备
    int64_t fileId = InsertPhoto(SIZE_VALUE, SIZE_DEFAULT, static_cast<int32_t>(PhotoPositionType::LOCAL), false,
        "/data/test/GetLocalAssetSize/photo.jpg", static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY));
    EXPECT_EQ(fileId > 0, true);

    // 2.执行
    auto task = std::make_shared<MediaUpdateLocalAssetSizeTask>();
    ASSERT_NE(task, nullptr);
    QueryLocalAssetSizeStatus ret = task->HandleUpdateLocalAssetSizeTask();
    EXPECT_EQ(ret, QueryLocalAssetSizeStatus::E_OK);

    // 3.value符合预期
    int64_t localAssetSize = QueryForLocalAssetSize(fileId);
    EXPECT_EQ(localAssetSize, 200);
}
} // namespace OHOS::Media::Background
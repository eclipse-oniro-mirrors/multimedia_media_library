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

#include "media_xt_style_template_task_test.h"

#include <chrono>
#include <string>
#include <thread>

#include "media_column.h"
#include "media_log.h"
#include "media_upgrade.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"

#include "media_xt_style_template_task.h"

using namespace testing::ext;
using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS::Media::Background {
static constexpr int32_t XT_NUMBER_UNPROCESSED = -2;
static constexpr int32_t XT_NUMBER_UNPARSED = -1;
static const char *const BY_DB_NAME_PROGRESS_KEY = "xt_style_template_by_db_name_progress";
static const char *const BY_FILE_EXIF_PROGRESS_KEY = "xt_style_template_by_file_exif_progress";
static const char *const PROGRESS_TEST_KEY = "xt_style_template_test_progress";
static constexpr int32_t WAIT_THREAD_EXIT_SECONDS = 1;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;

static void CleanTestTables()
{
    const string dropSql = "DROP TABLE IF EXISTS " + PhotoColumn::PHOTOS_TABLE + ";";
    int32_t ret = g_rdbStore->ExecuteSql(dropSql);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Drop photos table failed, ret=%{public}d", ret);
    }
}

static void SetTables()
{
    // CREATE_PHOTO_TABLE 已包含 xt_style_template_name 与 xt_style_template_number，无需额外补列
    int32_t ret = g_rdbStore->ExecuteSql(PhotoUpgrade::CREATE_PHOTO_TABLE);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Create photos table failed, ret=%{public}d", ret);
    }
}

void MediaXtStyleTemplateTaskTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaXtStyleTemplateTaskTest failed, can not get g_rdbStore");
        exit(1);
    }
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("MediaXtStyleTemplateTaskTest SetUpTestCase");
}

void MediaXtStyleTemplateTaskTest::TearDownTestCase(void)
{
    CleanTestTables();
    SetTables();
    g_rdbStore = nullptr;
    std::this_thread::sleep_for(std::chrono::seconds(WAIT_THREAD_EXIT_SECONDS));
    MEDIA_INFO_LOG("MediaXtStyleTemplateTaskTest TearDownTestCase");
}

void MediaXtStyleTemplateTaskTest::SetUp(void)
{
    g_rdbStore->ExecuteSql("DELETE FROM " + PhotoColumn::PHOTOS_TABLE);
    MedialibrarySubscriber::currentStatus_ = true;
}

void MediaXtStyleTemplateTaskTest::TearDown(void) {}

static int64_t InsertPhoto(const string &name, int32_t number, int32_t position = 1,
    int32_t mediaType = static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE), int32_t isTemp = 0)
{
    ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, "xt_style_template_test.jpg");
    values.PutInt(MediaColumn::MEDIA_TYPE, mediaType);
    values.PutInt(PhotoColumn::PHOTO_POSITION, position);
    values.PutInt(PhotoColumn::PHOTO_IS_TEMP, isTemp);
    values.PutInt(PhotoColumn::PHOTO_XT_STYLE_TEMPLATE_NUMBER, number);
    values.PutString(PhotoColumn::PHOTO_XT_STYLE_TEMPLATE_NAME, name);
    int64_t rowId = -1;
    EXPECT_EQ(g_rdbStore->Insert(rowId, PhotoColumn::PHOTOS_TABLE, values), E_OK);
    return rowId;
}

static int32_t GetNumber(int64_t fileId)
{
    const string querySql = "SELECT " + PhotoColumn::PHOTO_XT_STYLE_TEMPLATE_NUMBER + " FROM " +
        PhotoColumn::PHOTOS_TABLE + " WHERE " + MediaColumn::MEDIA_ID + " = " + to_string(fileId);
    auto resultSet = g_rdbStore->QuerySql(querySql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK) {
        return INT32_MAX;
    }
    return GetInt32Val(PhotoColumn::PHOTO_XT_STYLE_TEMPLATE_NUMBER, resultSet);
}

static string GetName(int64_t fileId)
{
    const string querySql = "SELECT " + PhotoColumn::PHOTO_XT_STYLE_TEMPLATE_NAME + " FROM " +
        PhotoColumn::PHOTOS_TABLE + " WHERE " + MediaColumn::MEDIA_ID + " = " + to_string(fileId);
    auto resultSet = g_rdbStore->QuerySql(querySql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK) {
        return "";
    }
    return GetStringVal(PhotoColumn::PHOTO_XT_STYLE_TEMPLATE_NAME, resultSet);
}

HWTEST_F(MediaXtStyleTemplateTaskTest, Accept_test_001, TestSize.Level0)
{
    auto task = make_shared<MediaXtStyleTemplateTask>();
    ASSERT_NE(task, nullptr);
    MedialibrarySubscriber::currentStatus_ = true;
    EXPECT_TRUE(task->Accept());
    MedialibrarySubscriber::currentStatus_ = false;
    EXPECT_FALSE(task->Accept());
    MedialibrarySubscriber::currentStatus_ = true;
}

HWTEST_F(MediaXtStyleTemplateTaskTest, QueryMaxFileId_test_001, TestSize.Level0)
{
    auto task = make_shared<MediaXtStyleTemplateTask>();
    ASSERT_NE(task, nullptr);
    int64_t firstId = InsertPhoto("3", XT_NUMBER_UNPROCESSED);
    int64_t secondId = InsertPhoto("7", XT_NUMBER_UNPROCESSED);
    ASSERT_GT(firstId, 0);
    ASSERT_GT(secondId, firstId);
    EXPECT_EQ(task->QueryMaxFileId(), static_cast<int32_t>(secondId));
}

HWTEST_F(MediaXtStyleTemplateTaskTest, HasUnprocessedAssets_test_001, TestSize.Level0)
{
    auto task = make_shared<MediaXtStyleTemplateTask>();
    ASSERT_NE(task, nullptr);
    EXPECT_FALSE(task->HasUnprocessedAssets());
    int64_t fileId = InsertPhoto("3", XT_NUMBER_UNPROCESSED);
    ASSERT_GT(fileId, 0);
    EXPECT_TRUE(task->HasUnprocessedAssets());
    const string updateSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " +
        PhotoColumn::PHOTO_XT_STYLE_TEMPLATE_NUMBER + " = 3 WHERE " +
        MediaColumn::MEDIA_ID + " = " + to_string(fileId);
    ASSERT_EQ(g_rdbStore->ExecuteSql(updateSql), E_OK);
    EXPECT_FALSE(task->HasUnprocessedAssets());
}

HWTEST_F(MediaXtStyleTemplateTaskTest, QueryUnprocessedAssetsByDbName_test_001, TestSize.Level0)
{
    auto task = make_shared<MediaXtStyleTemplateTask>();
    ASSERT_NE(task, nullptr);
    int64_t hitId = InsertPhoto("3", XT_NUMBER_UNPROCESSED);
    InsertPhoto("", XT_NUMBER_UNPROCESSED);
    InsertPhoto("-1", XT_NUMBER_UNPROCESSED);
    InsertPhoto("9", 5);
    ASSERT_GT(hitId, 0);

    auto assets = task->QueryUnprocessedAssetsByDbName(0);
    ASSERT_EQ(assets.size(), 1);
    EXPECT_EQ(assets.front().fileId, static_cast<int32_t>(hitId));
    EXPECT_EQ(assets.front().name, "3");
}

HWTEST_F(MediaXtStyleTemplateTaskTest, QueryUnprocessedAssetsByFileExif_test_001, TestSize.Level0)
{
    auto task = make_shared<MediaXtStyleTemplateTask>();
    ASSERT_NE(task, nullptr);
    int64_t hitId = InsertPhoto("", XT_NUMBER_UNPROCESSED);
    InsertPhoto("", XT_NUMBER_UNPROCESSED, 2);
    InsertPhoto("", XT_NUMBER_UNPROCESSED, 1, static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE), 1);
    InsertPhoto("", XT_NUMBER_UNPROCESSED, 1, static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO));
    InsertPhoto("", 5);
    ASSERT_GT(hitId, 0);

    auto assets = task->QueryUnprocessedAssetsByFileExif(0);
    ASSERT_EQ(assets.size(), 1);
    EXPECT_EQ(assets.front().fileId, static_cast<int32_t>(hitId));
}

HWTEST_F(MediaXtStyleTemplateTaskTest, BatchUpdateNumber_test_001, TestSize.Level0)
{
    auto task = make_shared<MediaXtStyleTemplateTask>();
    ASSERT_NE(task, nullptr);
    int64_t idThree = InsertPhoto("", XT_NUMBER_UNPROCESSED);
    int64_t idZero = InsertPhoto("", XT_NUMBER_UNPROCESSED);
    int64_t idInvalid = InsertPhoto("", XT_NUMBER_UNPROCESSED);
    int64_t idDefault = InsertPhoto("", XT_NUMBER_UNPROCESSED);
    int64_t idAbsent = InsertPhoto("", XT_NUMBER_UNPROCESSED);

    vector<XtStyleTemplateAssetInfo> assets;
    assets.push_back({ static_cast<int32_t>(idThree), XT_NUMBER_UNPROCESSED, "", "3" });
    assets.push_back({ static_cast<int32_t>(idZero), XT_NUMBER_UNPROCESSED, "", "0" });
    assets.push_back({ static_cast<int32_t>(idInvalid), XT_NUMBER_UNPROCESSED, "", "abc" });
    assets.push_back({ static_cast<int32_t>(idDefault), XT_NUMBER_UNPROCESSED, "", "default_exif_value" });
    assets.push_back({ static_cast<int32_t>(idAbsent), XT_NUMBER_UNPROCESSED, "", "-1" });
    ASSERT_EQ(task->BatchUpdateNumber(assets), E_OK);

    EXPECT_EQ(GetNumber(idThree), 3);
    EXPECT_EQ(GetNumber(idZero), 0);
    EXPECT_EQ(GetNumber(idInvalid), XT_NUMBER_UNPARSED);
    EXPECT_EQ(GetNumber(idDefault), XT_NUMBER_UNPARSED);
    EXPECT_EQ(GetNumber(idAbsent), XT_NUMBER_UNPARSED);
}

HWTEST_F(MediaXtStyleTemplateTaskTest, BatchUpdateNameAndNumber_test_001, TestSize.Level0)
{
    auto task = make_shared<MediaXtStyleTemplateTask>();
    ASSERT_NE(task, nullptr);
    int64_t fileId = InsertPhoto("", XT_NUMBER_UNPROCESSED);
    ASSERT_GT(fileId, 0);

    vector<XtStyleTemplateAssetInfo> assets;
    assets.push_back({ static_cast<int32_t>(fileId), 5, "", "5" });
    ASSERT_EQ(task->BatchUpdateNameAndNumber(assets), E_OK);

    EXPECT_EQ(GetNumber(fileId), 5);
    EXPECT_EQ(GetName(fileId), "5");
}

HWTEST_F(MediaXtStyleTemplateTaskTest, ExtractNumberFromFile_test_001, TestSize.Level0)
{
    auto task = make_shared<MediaXtStyleTemplateTask>();
    ASSERT_NE(task, nullptr);
    XtStyleTemplateAssetInfo assetInfo;
    assetInfo.fileId = 1;
    assetInfo.path = "";
    EXPECT_FALSE(task->ExtractNumberFromFile(assetInfo));
    EXPECT_EQ(assetInfo.number, XT_NUMBER_UNPARSED);
    EXPECT_EQ(assetInfo.name, "-1");
}

HWTEST_F(MediaXtStyleTemplateTaskTest, GetProgressSetProgress_test_001, TestSize.Level0)
{
    auto task = make_shared<MediaXtStyleTemplateTask>();
    ASSERT_NE(task, nullptr);
    task->SetProgress(PROGRESS_TEST_KEY, 42);
    EXPECT_EQ(task->GetProgress(PROGRESS_TEST_KEY), 42);
    task->SetProgress(PROGRESS_TEST_KEY, 0);
    EXPECT_EQ(task->GetProgress(PROGRESS_TEST_KEY), 0);
}

HWTEST_F(MediaXtStyleTemplateTaskTest, FillUnprocessedNumbersByDbName_test_001, TestSize.Level0)
{
    auto task = make_shared<MediaXtStyleTemplateTask>();
    ASSERT_NE(task, nullptr);
    int64_t fileId = InsertPhoto("4", XT_NUMBER_UNPROCESSED);
    ASSERT_GT(fileId, 0);
    task->SetProgress(BY_DB_NAME_PROGRESS_KEY, 0);

    task->FillUnprocessedNumbersByDbName();

    EXPECT_EQ(GetNumber(fileId), 4);
    EXPECT_EQ(task->GetProgress(BY_DB_NAME_PROGRESS_KEY), static_cast<int32_t>(fileId));
    task->SetProgress(BY_DB_NAME_PROGRESS_KEY, 0);
}

HWTEST_F(MediaXtStyleTemplateTaskTest, FillUnprocessedNumbersByFileExif_test_001, TestSize.Level0)
{
    auto task = make_shared<MediaXtStyleTemplateTask>();
    ASSERT_NE(task, nullptr);
    int64_t fileId = InsertPhoto("", XT_NUMBER_UNPROCESSED);
    ASSERT_GT(fileId, 0);
    task->SetProgress(BY_FILE_EXIF_PROGRESS_KEY, 0);

    task->FillUnprocessedNumbersByFileExif();

    EXPECT_EQ(GetNumber(fileId), XT_NUMBER_UNPARSED);
    EXPECT_EQ(GetName(fileId), "-1");
    EXPECT_EQ(task->GetProgress(BY_FILE_EXIF_PROGRESS_KEY), static_cast<int32_t>(fileId));
    task->SetProgress(BY_FILE_EXIF_PROGRESS_KEY, 0);
}

HWTEST_F(MediaXtStyleTemplateTaskTest, Execute_test_001, TestSize.Level0)
{
    auto task = make_shared<MediaXtStyleTemplateTask>();
    ASSERT_NE(task, nullptr);
    int64_t stageAId = InsertPhoto("7", XT_NUMBER_UNPROCESSED);
    int64_t stageBId = InsertPhoto("", XT_NUMBER_UNPROCESSED);
    ASSERT_GT(stageAId, 0);
    ASSERT_GT(stageBId, stageAId);
    task->SetProgress(BY_DB_NAME_PROGRESS_KEY, 0);
    task->SetProgress(BY_FILE_EXIF_PROGRESS_KEY, 0);

    task->Execute();

    EXPECT_EQ(GetNumber(stageAId), 7);
    EXPECT_EQ(GetNumber(stageBId), XT_NUMBER_UNPARSED);
    task->SetProgress(BY_DB_NAME_PROGRESS_KEY, 0);
    task->SetProgress(BY_FILE_EXIF_PROGRESS_KEY, 0);
}
} // namespace OHOS::Media::Background

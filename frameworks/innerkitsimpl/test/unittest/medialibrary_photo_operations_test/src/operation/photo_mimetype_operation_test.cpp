/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#define MLOG_TAG "PhotoMimetypeOperationTest"
#include "photo_mimetype_operation_test.h"

#include <string>

#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "values_bucket.h"

#include "photo_mimetype_operation.h"
#include "media_upgrade.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS::Media {
static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;

static void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        MEDIALIBRARY_TABLE
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

static void SetTables()
{
    vector<string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        CREATE_MEDIA_TABLE,
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

static void ClearAndRestart()
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

int32_t InsertCloudAssetINDb(const string &title, const string &extension)
{
    string displayName = title + extension;
    string data = "/storage/cloud/files/photo/1/" + displayName;
    ValuesBucket valuesBucket;
    valuesBucket.PutString(MediaColumn::MEDIA_FILE_PATH, data);
    valuesBucket.PutString(MediaColumn::MEDIA_TITLE, title);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, displayName);
    valuesBucket.PutInt(MediaColumn::MEDIA_TYPE, static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE));
    valuesBucket.PutString(PhotoColumn::MEDIA_MIME_TYPE, "application/octet-stream");
    int64_t fileId = -1;
    int32_t ret = g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("InsertCloudAsset fileId is %{public}s", to_string(fileId).c_str());
    return ret;
}

void PhotoMimetypeOperationTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start PhotoMimetypeOperationTest failed, can not get rdbstore");
        exit(1);
    }
    SetTables();
}

void PhotoMimetypeOperationTest::TearDownTestCase(void)
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    ClearAndRestart();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    this_thread::sleep_for(chrono::seconds(1));
    MEDIA_INFO_LOG("Clean is finish");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

void PhotoMimetypeOperationTest::SetUp()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryCloudAssetDownloadTest failed, can not get rdbstore");
        exit(1);
    }
    ClearAndRestart();
}

void PhotoMimetypeOperationTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(PhotoMimetypeOperationTest, update_invalid_mimetype_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("update_invalid_mimetype_test_001 Start");
    int32_t ret = InsertCloudAssetINDb("test1", ".jpg");
    EXPECT_EQ(ret, E_OK);
    ret = InsertCloudAssetINDb("test2", ".heic");
    EXPECT_EQ(ret, E_OK);
    ret = InsertCloudAssetINDb("test3", ".heif");
    EXPECT_EQ(ret, E_OK);
    ret = InsertCloudAssetINDb("test4", ".dng");
    EXPECT_EQ(ret, E_OK);

    ret = PhotoMimetypeOperation::UpdateInvalidMimeType();
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("update_invalid_mimetype_test_001 End");
}

HWTEST_F(PhotoMimetypeOperationTest, update_invalid_mimetype_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("update_invalid_mimetype_test_002 Start");
    int32_t ret = InsertCloudAssetINDb("test1", ".jpg");
    EXPECT_EQ(ret, E_OK);
    ret = InsertCloudAssetINDb("test2", ".heic");
    EXPECT_EQ(ret, E_OK);
    ret = InsertCloudAssetINDb("test3", ".xxx");
    EXPECT_EQ(ret, E_OK);
    ret = InsertCloudAssetINDb("test4", ".xxx");
    EXPECT_EQ(ret, E_OK);

    ret = PhotoMimetypeOperation::UpdateInvalidMimeType();
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("update_invalid_mimetype_test_002 End");
}
}  // namespace OHOS::Media
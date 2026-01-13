/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
 
#define MLOG_TAG "MediaLibraryFaCardOperationsTest"
 
#include "medialibrary_facard_operations_test.h"
 
#define private public
#include "medialibrary_facard_operations.h"
#undef private
 
#include <chrono>
#include <fcntl.h>
#include <fstream>
#include <string>
#include <thread>
#include <unistd.h>
#include <unordered_map>
 
#include "abs_shared_result_set.h"
#include "file_ex.h"
#include "media_column.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_rdbstore.h"
#include "rdb_predicates.h"
#include "result_set_utils.h"
#include "string_ex.h"
#include "thumbnail_const.h"
#include "uri.h"
#include "userfile_manager_types.h"
#include "value_object.h"
#include "values_bucket.h"
#include "image_type.h"
#include "datashare_helper.h"
#include "unique_fd.h"
#include "medialibrary_data_manager.h"
#include "media_facard_photos_column.h"
#include "medialibrary_unittest_utils.h"
#include "media_upgrade.h"
 
using namespace testing::ext;
 
namespace OHOS {
namespace Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using OHOS::DataShare::DataShareValuesBucket;
using OHOS::DataShare::DataSharePredicates;
 
static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
const string F0RMID_FOR_TEST_STORE = "123456789";
const string ASSETURI_FOR_TEST_STORE = "file://media/0/IMG_1698250306_000/IMG_20250126_001146.jpg";
const string F0RMID_FOR_TEST_REMOVE = "309";
const string ASSETURI_FOR_TEST_REMOVE = "file://media/10/IMG_232563421_001/IMG_20250126_008923.jpg";
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
 
int32_t CreateFaCardPhotoApi10(int mediaType, const string &displayName)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutInt(MediaColumn::MEDIA_TYPE, mediaType);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryPhotoOperations::Create(cmd);
    if (ret < 0) {
        MEDIA_ERR_LOG("Create Photo failed, errCode=%{public}d", ret);
        return ret;
    }
    return ret;
}
 
void CleanTestFaCardTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        TabFaCardPhotosColumn::FACARD_PHOTOS_TABLE
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
 
void SetFaCardTables()
{
    const std::string createFacardTableSql = "\
        CREATE TABLE IF NOT EXISTS tab_facard_photos \
            form_id     TEXT, \
            asset_uri   TEXT \
        );";
    vector<string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        createFacardTableSql
    };
    for (auto &createTableSql : createTableSqlList) {
        if (g_rdbStore == nullptr) {
            MEDIA_ERR_LOG("can not get g_rdbstore");
            return;
        }
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
}
 
void ClearAndRestartFaCardTables()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }
 
    system("rm -rf /storage/cloud/files/*");
    system("rm -rf /storage/cloud/files/.thumbs");
    system("rm -rf /storage/cloud/files/.editData");
    system("rm -rf /storage/cloud/files/.cache");
    for (const auto &dir : TEST_ROOT_DIRS) {
        string ROOT_PATH = "/storage/cloud/100/files/";
        bool ret = MediaFileUtils::CreateDirectory(ROOT_PATH + dir + "/");
        CHECK_AND_PRINT_LOG(ret, "make %{public}s dir failed, ret=%{public}d", dir.c_str(), ret);
    }
    CleanTestFaCardTables();
    SetFaCardTables();
}
 
void MediaLibraryFaCardOperationsTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    SetFaCardTables();
}
 
void MediaLibraryFaCardOperationsTest::TearDownTestCase(void)
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }
 
    system("rm -rf /storage/cloud/files/*");
    ClearAndRestartFaCardTables();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    this_thread::sleep_for(chrono::seconds(SLEEP_FIVE_SECONDS));
    MEDIA_INFO_LOG("Clean is finish");
}
 
// SetUp:Execute before each test case
void MediaLibraryFaCardOperationsTest::SetUp()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    ClearAndRestartFaCardTables();
}
 
void MediaLibraryFaCardOperationsTest::TearDown()
{}
 
HWTEST_F(MediaLibraryFaCardOperationsTest, FaCardOperations_test_001, TestSize.Level1)
{
    std::map<std::string, std::vector<std::string>> urisMap = MediaLibraryFaCardOperations::GetUris();
    EXPECT_EQ(urisMap.empty(), true);
}
 
HWTEST_F(MediaLibraryFaCardOperationsTest, FaCardOperations_test_002, TestSize.Level1)
{
    MediaLibraryCommand cmd(OperationObject::TAB_FACARD_PHOTO, OperationType::OPRN_STORE_FORM_ID);
    cmd.SetTableName(TabFaCardPhotosColumn::FACARD_PHOTOS_TABLE);
    NativeRdb::ValuesBucket values;
    values.PutString(TabFaCardPhotosColumn::FACARD_PHOTOS_FORM_ID, F0RMID_FOR_TEST_STORE);
    values.PutString(TabFaCardPhotosColumn::FACARD_PHOTOS_ASSET_URI, ASSETURI_FOR_TEST_STORE);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryFaCardOperations::HandleStoreGalleryFormOperation(cmd);
    EXPECT_EQ(ret > 0, false);
}
 
HWTEST_F(MediaLibraryFaCardOperationsTest, FaCardOperations_test_003, TestSize.Level1)
{
    MediaLibraryCommand cmd(OperationObject::TAB_FACARD_PHOTO, OperationType::OPRN_STORE_FORM_ID);
    cmd.SetTableName(TabFaCardPhotosColumn::FACARD_PHOTOS_TABLE);
    NativeRdb::ValuesBucket values;
    values.PutString(TabFaCardPhotosColumn::FACARD_PHOTOS_FORM_ID, F0RMID_FOR_TEST_REMOVE);
    values.PutString(TabFaCardPhotosColumn::FACARD_PHOTOS_ASSET_URI, ASSETURI_FOR_TEST_REMOVE);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryFaCardOperations::HandleStoreGalleryFormOperation(cmd);
    RdbPredicates predicates(TabFaCardPhotosColumn::FACARD_PHOTOS_TABLE);
    predicates.EqualTo(TabFaCardPhotosColumn::FACARD_PHOTOS_FORM_ID, F0RMID_FOR_TEST_REMOVE);
    ret = MediaLibraryFaCardOperations::HandleRemoveGalleryFormOperation(predicates);
    EXPECT_EQ(ret > 0, false);
}
} // namespace Media
} // namespace OHOS
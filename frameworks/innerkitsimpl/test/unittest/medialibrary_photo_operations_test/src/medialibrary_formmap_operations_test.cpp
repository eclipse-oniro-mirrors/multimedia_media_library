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

#define MLOG_TAG "FormMapOperationsTest"

#include "medialibrary_formmap_operations_test.h"

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
#include "medialibrary_formmap_operations.h"
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
#include "form_map.h"
#include "medialibrary_unittest_utils.h"
#include "media_upgrade.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using OHOS::DataShare::DataShareValuesBucket;
using OHOS::DataShare::DataSharePredicates;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
const string F0RMID_FOR_TEST = "123456789";
const string F0RMID_FOR_TEST_TWO = "12345678910";
const string F0RMID_NO_SAVE = "12345678911";
const string FALSE_URI = "file://media/Photo/0/IMG_1698250306_000/IMG_20231026_001146.jpg";
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;

int32_t CreatePhotoApi10(int mediaType, const string &displayName)
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

void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        FormMap::FORM_MAP_TABLE,
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
        FormMap::CREATE_FORM_MAP_TABLE,
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

void ClearAndRestart()
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
    CleanTestTables();
    SetTables();
}

void MediaLibraryFormOperationsTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    SetTables();
}

void MediaLibraryFormOperationsTest::TearDownTestCase()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    ClearAndRestart();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    this_thread::sleep_for(chrono::seconds(SLEEP_FIVE_SECONDS));
    MEDIA_INFO_LOG("Clean is finish");
}

void MediaLibraryFormOperationsTest::SetUp()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    ClearAndRestart();
}

void MediaLibraryFormOperationsTest::TearDown()
{}

HWTEST_F(MediaLibraryFormOperationsTest, FormMapOperations_test_001, TestSize.Level1)
{
    // test for store form operation and remove form operation
    MediaLibraryCommand cmd(OperationObject::PAH_FORM_MAP, OperationType::UPDATE);
    cmd.SetTableName(FormMap::FORM_MAP_TABLE);
    NativeRdb::ValuesBucket values;
    values.PutString(FormMap::FORMMAP_FORM_ID, F0RMID_FOR_TEST);
    values.PutString(FormMap::FORMMAP_URI, "");
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryFormMapOperations::HandleStoreFormIdOperation(cmd);
    EXPECT_EQ(ret > 0, true);

    RdbPredicates predicates(FormMap::FORM_MAP_TABLE);
    predicates.EqualTo(FormMap::FORMMAP_FORM_ID, F0RMID_FOR_TEST);
    ret = MediaLibraryFormMapOperations::RemoveFormIdOperations(predicates);
    EXPECT_EQ(ret > 0, false);
}

HWTEST_F(MediaLibraryFormOperationsTest, FormMapOperations_test_002, TestSize.Level1)
{
    MediaLibraryCommand cmd(OperationObject::PAH_FORM_MAP, OperationType::UPDATE);
    cmd.SetTableName(FormMap::FORM_MAP_TABLE);
    NativeRdb::ValuesBucket values;
    values.PutString(FormMap::FORMMAP_FORM_ID, F0RMID_FOR_TEST);
    values.PutString(FormMap::FORMMAP_URI, "");
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryFormMapOperations::HandleStoreFormIdOperation(cmd);
    EXPECT_EQ(ret > 0, true);
    bool res = MediaLibraryFormMapOperations::CheckQueryIsInDb(OperationObject::PAH_FORM_MAP, F0RMID_FOR_TEST);
    EXPECT_EQ(res, true);

    int32_t fileId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "formPhoto_1.jpg");
    res = MediaLibraryFormMapOperations::CheckQueryIsInDb(OperationObject::UFM_PHOTO, ToString(fileId));
    EXPECT_EQ(res, true);
}

HWTEST_F(MediaLibraryFormOperationsTest, FormMapOperations_test_003, TestSize.Level1)
{
    MediaLibraryCommand cmd(OperationObject::PAH_FORM_MAP, OperationType::UPDATE);
    cmd.SetTableName(FormMap::FORM_MAP_TABLE);
    NativeRdb::ValuesBucket values;
    values.PutString(FormMap::FORMMAP_FORM_ID, F0RMID_FOR_TEST);
    values.PutString(FormMap::FORMMAP_URI, "");
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryFormMapOperations::HandleStoreFormIdOperation(cmd);
    EXPECT_EQ(ret > 0, true);

    vector<int64_t> formIds;
    MediaLibraryFormMapOperations::GetFormMapFormId("", formIds);
    EXPECT_EQ(to_string(formIds.front()).c_str(), F0RMID_FOR_TEST);
}

HWTEST_F(MediaLibraryFormOperationsTest, FormMapOperations_test_004, TestSize.Level1)
{
    int32_t fileId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "formPhoto_2.jpg");
    string path = MediaLibraryFormMapOperations::GetFilePathById(ToString(fileId));
    string uri = MediaLibraryFormMapOperations::GetUriByFileId(fileId, path.c_str());

    MediaLibraryCommand cmd(OperationObject::PAH_FORM_MAP, OperationType::UPDATE);
    cmd.SetTableName(FormMap::FORM_MAP_TABLE);
    NativeRdb::ValuesBucket values;
    values.PutString(FormMap::FORMMAP_FORM_ID, F0RMID_FOR_TEST_TWO);
    values.PutString(FormMap::FORMMAP_URI, uri.c_str());
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryFormMapOperations::HandleStoreFormIdOperation(cmd);
    EXPECT_EQ(ret > 0, true);

    vector<int64_t> formIds;
    MediaLibraryFormMapOperations::GetFormMapFormId(uri.c_str(), formIds);
    EXPECT_EQ(to_string(formIds.front()).c_str(), F0RMID_FOR_TEST_TWO);
}

HWTEST_F(MediaLibraryFormOperationsTest, FormMapOperations_test_005, TestSize.Level1)
{
    MediaLibraryCommand cmd(OperationObject::PAH_FORM_MAP, OperationType::UPDATE);
    cmd.SetTableName(FormMap::FORM_MAP_TABLE);
    NativeRdb::ValuesBucket values;
    values.PutString(FormMap::FORMMAP_FORM_ID, F0RMID_FOR_TEST_TWO);
    values.PutString(FormMap::FORMMAP_URI, FALSE_URI);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryFormMapOperations::HandleStoreFormIdOperation(cmd);
    EXPECT_EQ(ret, E_GET_PRAMS_FAIL);
}

HWTEST_F(MediaLibraryFormOperationsTest, FormMapOperations_test_006, TestSize.Level1)
{
    MediaLibraryCommand cmd(OperationObject::PAH_FORM_MAP, OperationType::UPDATE);
    cmd.SetTableName(FormMap::FORM_MAP_TABLE);
    NativeRdb::ValuesBucket values;
    values.PutString(FormMap::FORMMAP_FORM_ID, "");
    values.PutString(FormMap::FORMMAP_URI, "");
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryFormMapOperations::HandleStoreFormIdOperation(cmd);
    EXPECT_EQ(ret, E_GET_PRAMS_FAIL);
}

HWTEST_F(MediaLibraryFormOperationsTest, FormMapOperations_test_007, TestSize.Level1)
{
    int32_t fileId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "formPhoto_3.jpg");
    string path = MediaLibraryFormMapOperations::GetFilePathById(ToString(fileId + 1));
    EXPECT_EQ(path, "");

    string uri = MediaLibraryFormMapOperations::GetUriByFileId((fileId + 1), path.c_str());
    EXPECT_EQ(uri, "");
}

HWTEST_F(MediaLibraryFormOperationsTest, FormMapOperations_test_008, TestSize.Level1)
{
    int32_t fileId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "formPhoto_4.jpg");
    bool res = MediaLibraryFormMapOperations::CheckQueryIsInDb(OperationObject::UFM_PHOTO, ToString(fileId + 1));
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryFormOperationsTest, FormMapOperations_test_009, TestSize.Level1)
{
    bool res = MediaLibraryFormMapOperations::CheckQueryIsInDb(OperationObject::PAH_FORM_MAP, F0RMID_NO_SAVE);
    EXPECT_EQ(res, false);
}
}
}
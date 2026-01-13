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

#define MLOG_TAG "CloudEnhancementGetPairUnitTest"


#include "medialibrary_cloud_enhancement_get_pair_test.h"

#include <chrono>
#include <thread>

#include "gmock/gmock.h"
#include "image_source.h"
#include "media_exif.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_command.h"
#include "medialibrary_common_utils.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_errno.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_rdb_transaction.h"
#include "result_set_utils.h"
#include "values_bucket.h"

#define private public
#define protected public
#include "file_utils.h"
#include "enhancement_manager.h"
#include "enhancement_service_callback.h"
#include "enhancement_task_manager.h"
#include "enhancement_service_adapter.h"
#include "enhancement_database_operations.h"
#undef private
#undef protected
#include "media_upgrade.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using OHOS::DataShare::DataShareValuesBucket;
using OHOS::DataShare::DataSharePredicates;

namespace OHOS {
namespace Media {
static const string FIRST_TESTING_PHOTO_ID = "202408261737";
static const string SECOND_TESTING_PHOTO_ID = "20240826173";
static const string FIRST_TESTING_DISPLAYNAME = "CloudEnhancementTest001.jpg";
static const string SECOND_TESTING_DISPLAYNAME = "CloudEnhancementTest002.jpg";
static const double TESTING_TIME = 1725282828560;
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;

static int32_t firstFileId;
static int32_t secondFileId;
static string uriStr = "";
static string uriStrAssociated = "";

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
 
void CleanTestTables()
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
 
void SetTables()
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

inline int32_t CreatePhotoApi10(int mediaType, const string &displayName)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutInt(MediaColumn::MEDIA_TYPE, mediaType);
    cmd.SetValueBucket(values);
    return MediaLibraryPhotoOperations::Create(cmd);
}

string GetFilePath(int fileId)
{
    if (fileId < 0) {
        MEDIA_ERR_LOG("this file id %{private}d is invalid", fileId);
        return "";
    }
 
    vector<string> columns = { PhotoColumn::MEDIA_FILE_PATH };
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("can not get rdbstore");
        return "";
    }
    auto resultSet = g_rdbStore->Query(cmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get file Path");
        return "";
    }
    string path = GetStringVal(PhotoColumn::MEDIA_FILE_PATH, resultSet);
    return path;
}

int32_t MakePhotoUnpending(int fileId)
{
    if (fileId < 0) {
        MEDIA_ERR_LOG("this file id %{private}d is invalid", fileId);
        return E_INVALID_FILEID;
    }
 
    string path = GetFilePath(fileId);
    if (path.empty()) {
        MEDIA_ERR_LOG("Get path failed");
        return E_INVALID_VALUES;
    }
    int32_t errCode = MediaFileUtils::CreateAsset(path);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("Can not create asset");
        return errCode;
    }
 
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("can not get rdbstore");
        return E_HAS_DB_ERROR;
    }
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    ValuesBucket values;
    values.PutLong(PhotoColumn::MEDIA_TIME_PENDING, 0);
    values.PutLong(PhotoColumn::PHOTO_EDIT_TIME, 0);
    values.PutLong(MediaColumn::MEDIA_DATE_TRASHED, 0);
    values.PutLong(PhotoColumn::PHOTO_HIDDEN_TIME, 0);
    cmd.SetValueBucket(values);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    int32_t changedRows = -1;
    errCode = g_rdbStore->Update(cmd, changedRows);
    if (errCode != E_OK || changedRows <= 0) {
        MEDIA_ERR_LOG("Update pending failed, errCode = %{public}d, changeRows = %{public}d",
            errCode, changedRows);
        return errCode;
    }

    return E_OK;
}

int32_t SetDefaultPhotoApi10(int mediaType, const string &displayName)
{
    int fileId = CreatePhotoApi10(mediaType, displayName);
    if (fileId < 0) {
        MEDIA_ERR_LOG("create photo failed, res=%{public}d", fileId);
        return fileId;
    }
    int32_t errCode = MakePhotoUnpending(fileId);
    if (errCode != E_OK) {
        return errCode;
    }
    return fileId;
}

int32_t PrepareHighQualityPhoto(const string &photoId, const string &displayName)
{
    auto fileId = SetDefaultPhotoApi10(MediaType::MEDIA_TYPE_IMAGE, displayName);
    EXPECT_GT(fileId, 0);
 
    // update multi-stages capture db info
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE, MediaLibraryApi::API_10);
    ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(MultiStagesPhotoQuality::FULL));
    values.Put(PhotoColumn::PHOTO_ID, photoId);
    values.Put(PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, 1);
    cmd.SetValueBucket(values);
    cmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, to_string(fileId));
    EXPECT_GT(MediaLibraryPhotoOperations::Update(cmd), E_OK);
 
    return fileId;
}

bool UpdateAssociateFileId(int32_t fileId, int32_t associateFileId)
{
    // update cloud enhancement associateFileId
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE, MediaLibraryApi::API_10);
    ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_ASSOCIATE_FILE_ID, associateFileId);
    cmd.SetValueBucket(values);
    cmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, to_string(fileId));
    int32_t res = MediaLibraryPhotoOperations::Update(cmd);

    MediaLibraryCommand cmdAssociated(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE,
        MediaLibraryApi::API_10);
    ValuesBucket valuesAssociated;
    valuesAssociated.Put(PhotoColumn::PHOTO_ASSOCIATE_FILE_ID, fileId);
    cmdAssociated.SetValueBucket(valuesAssociated);
    cmdAssociated.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, to_string(associateFileId));
    int32_t resAssociated = MediaLibraryPhotoOperations::Update(cmdAssociated);
    return res == E_OK && resAssociated == E_OK;
}

bool UpdateEditAndTrashAndHiddenTime(int32_t fileId, double editedTime, double trashedTime, double hiddenTime)
{
    // update cloud enhancement associateFileId
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE, MediaLibraryApi::API_10);
    ValuesBucket values;
    values.PutLong(PhotoColumn::PHOTO_EDIT_TIME, editedTime);
    values.PutLong(MediaColumn::MEDIA_DATE_TRASHED, trashedTime);
    values.PutLong(PhotoColumn::PHOTO_HIDDEN_TIME, hiddenTime);
    cmd.SetValueBucket(values);
    cmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, to_string(fileId));
    int32_t res = MediaLibraryPhotoOperations::Update(cmd);
    return res == E_OK;
}

void MediaLibraryCloudEnhancementGetPairTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    SetTables();
}

void MediaLibraryCloudEnhancementGetPairTest::TearDownTestCase(void)
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }
 
    system("rm -rf /storage/cloud/files/*");
    ClearAndRestart();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
    MEDIA_INFO_LOG("Clean is finish");
}

// SetUp:Execute before each test case
void MediaLibraryCloudEnhancementGetPairTest::SetUp()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    ClearAndRestart();
    firstFileId = PrepareHighQualityPhoto(FIRST_TESTING_PHOTO_ID, FIRST_TESTING_DISPLAYNAME);
    secondFileId = PrepareHighQualityPhoto(SECOND_TESTING_PHOTO_ID, SECOND_TESTING_DISPLAYNAME);
    uriStr = "file://media/Photo/" + std::to_string(firstFileId) + "/" + FIRST_TESTING_DISPLAYNAME;
    uriStrAssociated = "file://media/Photo/" + std::to_string(secondFileId) + "/" + SECOND_TESTING_DISPLAYNAME;
}

void MediaLibraryCloudEnhancementGetPairTest::TearDown(void) {}

#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_001 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStr);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_NE(resultSet, nullptr);
    int32_t count = 0;
    resultSet->GetRowCount(count);
    EXPECT_EQ(count, 1);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_001 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_002 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStrAssociated);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_NE(resultSet, nullptr);
    int32_t count = 0;
    resultSet->GetRowCount(count);
    EXPECT_EQ(count, 1);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_002 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_003 Start");
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStr);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_003 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_004 Start");
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStrAssociated);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_004 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_005 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(firstFileId, TESTING_TIME, TESTING_TIME, TESTING_TIME);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStr);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_005 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_006 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(firstFileId, TESTING_TIME, TESTING_TIME, TESTING_TIME);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStrAssociated);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_006 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_007 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(secondFileId, TESTING_TIME, TESTING_TIME, TESTING_TIME);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStr);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_007 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_008 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(secondFileId, TESTING_TIME, TESTING_TIME, TESTING_TIME);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStrAssociated);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_001 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_009 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(firstFileId, 0, 0, TESTING_TIME);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStr);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_009 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_010 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(firstFileId, 0, 0, TESTING_TIME);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStrAssociated);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_010 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_011, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_011 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(secondFileId, 0, 0, TESTING_TIME);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStr);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_011 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_012, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_012 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(secondFileId, 0, 0, TESTING_TIME);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStrAssociated);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_012 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_013, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_013 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(firstFileId, 0, TESTING_TIME, 0);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStr);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_013 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_014, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_014 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(firstFileId, 0, TESTING_TIME, 0);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStrAssociated);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_014 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_015, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_015 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(secondFileId, 0, TESTING_TIME, 0);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStr);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_015 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_016, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_016 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(secondFileId, 0, TESTING_TIME, 0);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStrAssociated);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_016 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_017, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_017 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(firstFileId, 0, TESTING_TIME, TESTING_TIME);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStr);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_017 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_018, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_018 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(firstFileId, 0, TESTING_TIME, TESTING_TIME);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStrAssociated);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_018 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_019, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_019 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(secondFileId, 0, TESTING_TIME, TESTING_TIME);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStr);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_019 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_020, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_020 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(secondFileId, 0, TESTING_TIME, TESTING_TIME);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStrAssociated);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_020 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_021, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_021 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(firstFileId, TESTING_TIME, 0, 0);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStr);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_021 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_022, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_022 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(firstFileId, TESTING_TIME, 0, 0);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStrAssociated);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_022 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_023, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_023 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(secondFileId, TESTING_TIME, 0, 0);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStr);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_023 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_024, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_024 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(secondFileId, TESTING_TIME, 0, 0);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStrAssociated);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_024 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_025, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_025 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(firstFileId, TESTING_TIME, 0, TESTING_TIME);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStr);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_025 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_026, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_026 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(firstFileId, TESTING_TIME, 0, TESTING_TIME);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStrAssociated);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_026 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_027, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_027 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(secondFileId, TESTING_TIME, 0, TESTING_TIME);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStr);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_027 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_028, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_028 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(secondFileId, TESTING_TIME, 0, TESTING_TIME);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStrAssociated);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_028 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_029, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_029 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(firstFileId, TESTING_TIME, TESTING_TIME, 0);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStr);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_029 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_030, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_030 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(firstFileId, TESTING_TIME, TESTING_TIME, 0);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStrAssociated);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_030 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_031, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_031 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(secondFileId, TESTING_TIME, TESTING_TIME, 0);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStr);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_031 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, manager_handle_get_pair_operation_032, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_032 Start");
    UpdateAssociateFileId(firstFileId, secondFileId);
    UpdateEditAndTrashAndHiddenTime(secondFileId, TESTING_TIME, TESTING_TIME, 0);
    MediaLibraryCommand cmd (OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, uriStrAssociated);
    cmd.SetDataSharePred(predicates);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleGetPairOperation(cmd);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("manager_handle_get_pair_operation_032 End");
}

HWTEST_F(MediaLibraryCloudEnhancementGetPairTest, dfx_total_time_033, TestSize.Level1)
{
    MEDIA_INFO_LOG("dfx_total_time_033 Start");
    string photoId = "1234566";
    string type = "TestType";
    CloudEnhancementGetCount::GetInstance().AddStartTime(photoId);
    EXPECT_EQ(CloudEnhancementGetCount::GetInstance().GetStartTimes().empty(), false);
    // sleep for 1234 millseconds
    this_thread::sleep_for(chrono::milliseconds(1234));
    CloudEnhancementGetCount::GetInstance().Report(type, photoId, 0, 0);
    EXPECT_EQ(CloudEnhancementGetCount::GetInstance().GetStartTimes().empty(), true);
    MEDIA_INFO_LOG("dfx_total_time_033 End");
}
#endif
}
}

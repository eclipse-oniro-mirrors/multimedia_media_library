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

#define MLOG_TAG "CloudEnhancementUnitTest"

#include "medialibrary_cloud_enhancement_test.h"

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

#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
#include "media_enhance_constants_c_api.h"
#include "media_enhance_handles.h"
#include "media_enhance_client_c_api.h"
#include "media_enhance_bundle_c_api.h"
#endif

using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using OHOS::DataShare::DataShareValuesBucket;
using OHOS::DataShare::DataSharePredicates;
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
using namespace OHOS::MediaEnhance;
#endif

namespace OHOS {
namespace Media {
static const string TESTING_PHOTO_ID = "202408261737";
static const string TESTING_DISPLAYNAME = "IMG_20240904_133901.jpg";
static const uint8_t BUFFER[] = {
    255, 216, 255, 224, 0, 16, 74, 70, 73, 70, 0, 1, 1, 1, 0, 96, 0, 96, 0, 0, 255, 219, 0, 67, 0, 8, 6, 6, 7, 6, 5, 8,
    7, 7, 7, 9, 9, 8, 10, 12, 20, 13, 12, 11, 11, 12, 25, 18, 19, 15, 20, 29, 26, 31, 30, 29, 26, 28, 28, 32, 36, 46,
    39, 32, 34, 44, 35, 28, 28, 40, 55, 41, 44, 48, 49, 52, 52, 52, 31, 39, 57, 61, 56, 50, 60, 46, 51, 52, 50, 255,
    219, 0, 67, 1, 9, 9, 9, 12, 11, 12, 24, 13, 13, 24, 50, 33, 28, 33, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
    50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
    50, 50, 50, 50, 50, 50, 50, 50, 50, 255, 192, 0, 17, 8, 0, 132, 0, 132, 3, 1, 34, 0, 2, 17, 1, 3, 17, 1, 255, 196,
    0, 31, 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 255, 196, 0, 181,
    16, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 125, 1, 2, 3, 0, 4, 17, 5, 18, 33, 49, 65, 6, 19, 81, 97, 7, 34,
    113, 20, 50, 129, 145, 161, 8, 35, 66, 177, 193, 21, 82, 209, 240, 36, 51, 98, 114, 130, 9, 10, 22, 23, 24, 25, 26,
    37, 38, 39, 40, 41, 42, 52, 53, 54, 55, 56, 57, 58, 67, 68, 69, 70, 71, 72, 73, 74, 83, 84, 85, 86, 87, 88, 89, 90,
    99, 100, 101, 102, 103, 104, 105, 106, 115, 116, 117, 118, 119, 120, 121, 122, 131, 132, 133, 134, 135, 136, 137,
    138, 146, 147, 148, 149, 150, 151, 152, 153, 154, 162, 163, 164, 165, 166, 167, 168, 169, 170, 178, 179, 180, 181,
    182, 183, 184, 185, 186, 194, 195, 196, 197, 198, 199, 200, 201, 202, 210, 211, 212, 213, 214, 215, 216, 217, 218,
    225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 255, 196, 0, 31,
    1, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 255, 196, 0, 181, 17, 0,
    2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 119, 0, 1, 2, 3, 17, 4, 5, 33, 49, 6, 18, 65, 81, 7, 97, 113, 19, 34, 50,
    129, 8, 20, 66, 145, 161, 177, 193, 9, 35, 51, 82, 240, 21, 98, 114, 209, 10, 22, 36, 52, 225, 37, 241, 23, 24, 25,
    26, 38, 39, 40, 41, 42, 53, 54, 55, 56, 57, 58, 67, 68, 69, 70, 71, 72, 73, 74, 83, 84, 85, 86, 87, 88, 89, 90, 99,
    100, 101, 102, 103, 104, 105, 106, 115, 116, 117, 118, 119, 120, 121, 122, 130, 131, 132, 133, 134, 135, 136, 137,
    138, 146, 147, 148, 149, 150, 151, 152, 153, 154, 162, 163, 164, 165, 166, 167, 168, 169, 170, 178, 179, 180, 181,
    182, 183, 184, 185, 186, 194, 195, 196, 197, 198, 199, 200, 201, 202, 210, 211, 212, 213, 214, 215, 216, 217, 218,
    226, 227, 228, 229, 230, 231, 232, 233, 234, 242, 243, 244, 245, 246, 247, 248, 249, 250, 255, 218, 0, 12, 3, 1, 0,
    2, 17, 3, 17, 0, 63, 0, 244, 74, 40, 162, 191, 35, 62, 148, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40,
    162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0,
    40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138,
    0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162,
    138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40,
    162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0,
    40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138,
    0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162,
    138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40,
    162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0,
    40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138,
    0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 40, 162,
    138, 0, 40, 162, 138, 0, 40, 162, 138, 0, 255, 217
};

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
        PhotoColumn::CREATE_PHOTO_TABLE,
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

int32_t UpdateCEAvailable(int32_t fileId, int32_t ceAvailable, bool hasCloudWaterMark = false)
{
    // update cloud enhancement ce_available
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE, MediaLibraryApi::API_10);
    ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_CE_AVAILABLE, ceAvailable);
    if (hasCloudWaterMark) {
        values.Put(PhotoColumn::PHOTO_HAS_CLOUD_WATERMARK, 1);
    }
    cmd.SetValueBucket(values);
    cmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, to_string(fileId));
    return MediaLibraryPhotoOperations::Update(cmd);
}

void TestCloudEnhancementImage(vector<string> &columns, int32_t associateFileId, int32_t fileId, int32_t hidden,
    int32_t subtype)
{
    MediaLibraryCommand cmd2(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    cmd2.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(associateFileId));
    ASSERT_NE(g_rdbStore, nullptr);
    auto resultSet = g_rdbStore->Query(cmd2, columns);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);
    int32_t newFileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    string newFilePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    string newDisplayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    int32_t newHidden = GetInt32Val(MediaColumn::MEDIA_HIDDEN, resultSet);
    int32_t newSubtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    int32_t newCEAvailable = GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet);
    int32_t newAssociation = GetInt32Val(PhotoColumn::PHOTO_STRONG_ASSOCIATION, resultSet);
    int32_t newAssociateFileId = GetInt32Val(PhotoColumn::PHOTO_ASSOCIATE_FILE_ID, resultSet);
    EXPECT_EQ(newFileId, associateFileId);
    EXPECT_EQ(newAssociateFileId, fileId);
    EXPECT_EQ(newDisplayName.find("_enhanced") != string::npos, true);
    EXPECT_EQ(hidden, newHidden);
    EXPECT_EQ(subtype, newSubtype);
    EXPECT_EQ(newCEAvailable, static_cast<int32_t>(CloudEnhancementAvailableType::FINISH));
    EXPECT_EQ(newAssociation, 1);
    EXPECT_EQ(FileUtils::IsFileExist(newFilePath), true);
}

shared_ptr<NativeRdb::ResultSet> GetQueryResultSet(string photoId)
{
    vector<string> columns;
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::PHOTO_ID, photoId);
    if (g_rdbStore == nullptr) {
        return nullptr;
    }
    
    shared_ptr<NativeRdb::ResultSet> resultSet = g_rdbStore->Query(cmd, columns);
    if (resultSet != nullptr && resultSet->GoToFirstRow() == E_OK) {
        return resultSet;
    }
    return nullptr;
}


void MediaLibraryCloudEnhancementTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    SetTables();
}

void MediaLibraryCloudEnhancementTest::TearDownTestCase(void)
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
}

// SetUp:Execute before each test case
void MediaLibraryCloudEnhancementTest::SetUp()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    ClearAndRestart();
    vector<string> taskIds;
    EnhancementTaskManager::RemoveAllEnhancementTask(taskIds);
}

void MediaLibraryCloudEnhancementTest::TearDown(void) {}

#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
HWTEST_F(MediaLibraryCloudEnhancementTest, manager_init_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_init_001 Start");
    int32_t fileId = PrepareHighQualityPhoto(TESTING_PHOTO_ID, TESTING_DISPLAYNAME);
    UpdateCEAvailable(fileId, 1);
    bool initResult = EnhancementManager::GetInstance().Init();
    EXPECT_EQ(initResult, false);
    UpdateCEAvailable(fileId, 2);
    initResult = EnhancementManager::GetInstance().Init();
    EXPECT_EQ(initResult, true);
    UpdateCEAvailable(fileId, 2, true);
    initResult = EnhancementManager::GetInstance().Init();
    EXPECT_EQ(initResult, true);
    MEDIA_INFO_LOG("manager_init_001 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, manager_handle_enhancement_update_operation_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_enhancement_update_operation_002 Start");
    DataSharePredicates predicates;
    string photoUri = "file://media/Photo/1/IMG_1722329102_000/" + TESTING_DISPLAYNAME;
    predicates.EqualTo(MediaColumn::MEDIA_ID, photoUri);
    string uriStr = PAH_CLOUD_ENHANCEMENT_ADD;
    MediaFileUtils::UriAppendKeyValue(uriStr, MEDIA_OPERN_KEYWORD, "false");
    Uri addTaskWithoutWaterMarkUri(uriStr);
    MediaLibraryCommand cmd(addTaskWithoutWaterMarkUri);
    cmd.SetDataSharePred(predicates);
    int32_t ret = EnhancementManager::GetInstance().HandleEnhancementUpdateOperation(cmd);
    EXPECT_EQ(ret, -1);
    uriStr = PAH_CLOUD_ENHANCEMENT_ADD;
    MediaFileUtils::UriAppendKeyValue(uriStr, MEDIA_OPERN_KEYWORD, "true");
    Uri addTaskUri(uriStr);
    MediaLibraryCommand cmd2(addTaskUri);
    cmd2.SetDataSharePred(predicates);
    ret = EnhancementManager::GetInstance().HandleEnhancementUpdateOperation(cmd2);
    EXPECT_EQ(ret, -1);
    uriStr = PAH_CLOUD_ENHANCEMENT_PRIORITIZE;
    Uri prioritizeTaskUri(uriStr);
    MediaLibraryCommand cmd3(prioritizeTaskUri);
    cmd3.SetDataSharePred(predicates);
    ret = EnhancementManager::GetInstance().HandleEnhancementUpdateOperation(cmd3);
    EXPECT_EQ(ret, -1);
    uriStr = PAH_CLOUD_ENHANCEMENT_CANCEL;
    Uri cancelTaskUri(uriStr);
    MediaLibraryCommand cmd4(cancelTaskUri);
    cmd4.SetDataSharePred(predicates);
    ret = EnhancementManager::GetInstance().HandleEnhancementUpdateOperation(cmd4);
    EXPECT_EQ(ret, -1);
    uriStr = PAH_CLOUD_ENHANCEMENT_CANCEL_ALL;
    Uri cancelAllTasksUri(uriStr);
    MediaLibraryCommand cmd5(cancelAllTasksUri);
    cmd5.SetDataSharePred(predicates);
    ret = EnhancementManager::GetInstance().HandleEnhancementUpdateOperation(cmd5);
    EXPECT_EQ(ret, -1);
    uriStr = PAH_CLOUD_ENHANCEMENT_SYNC;
    Uri syncTasksUri(uriStr);
    MediaLibraryCommand cmd6(syncTasksUri);
    cmd6.SetDataSharePred(predicates);
    ret = EnhancementManager::GetInstance().HandleEnhancementUpdateOperation(cmd6);
    EXPECT_EQ(ret, -1);
    MEDIA_INFO_LOG("manager_handle_enhancement_update_operation_002 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, manager_handle_enhancement_query_operation_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_enhancement_query_operation_003 Start");
    DataSharePredicates predicates;
    string photoUri = "file://media/Photo/1/IMG_1722329102_000/" + TESTING_DISPLAYNAME;
    predicates.EqualTo(MediaColumn::MEDIA_ID, photoUri);
    EnhancementManager &instance = EnhancementManager::GetInstance();
    string uriStr = PAH_CLOUD_ENHANCEMENT_QUERY;
    Uri queryTaskUri(uriStr);
    MediaLibraryCommand cmd1(queryTaskUri);
    cmd1.SetDataSharePred(predicates);
    vector<string> columns;
    shared_ptr<NativeRdb::ResultSet> resultSet1 = instance.HandleEnhancementQueryOperation(cmd1, columns);
    EXPECT_EQ(resultSet1, nullptr);
    uriStr = PAH_CLOUD_ENHANCEMENT_GET_PAIR;
    Uri getPairUri(uriStr);
    MediaLibraryCommand cmd2(getPairUri);
    cmd2.SetDataSharePred(predicates);
    shared_ptr<NativeRdb::ResultSet> resultSet2 = instance.HandleEnhancementQueryOperation(cmd2, columns);
    EXPECT_EQ(resultSet2, nullptr);
    MEDIA_INFO_LOG("manager_handle_enhancement_query_operation_003 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, manager_handle_add_operation_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_add_operation_004 Start");
    vector<string> uris;
    DataSharePredicates predicates;
    string photoUri = "file://media/Photo/0/IMG_1722329102_000/" + TESTING_DISPLAYNAME;
    uris.emplace_back(photoUri);

    int32_t fileId2 = PrepareHighQualityPhoto(TESTING_PHOTO_ID, TESTING_DISPLAYNAME);
    UpdateCEAvailable(fileId2, 1, true);
    string photoUri2 = "file://media/Photo/" + to_string(fileId2) +
        "/IMG_1722329102_001/" + TESTING_DISPLAYNAME;

    string photoId3 = "202408261738";
    string diplayName3 = "CloudEnhancementTest002.jpg";
    int32_t fileId3 = PrepareHighQualityPhoto(photoId3, diplayName3);
    UpdateCEAvailable(fileId3, 2, true);
    string photoUri3 = "file://media/Photo/" + to_string(fileId3) +
        "/IMG_1722329102_002/" + TESTING_DISPLAYNAME;
    
    string photoId4 = "202408261739";
    string diplayName4 = "CloudEnhancementTest003.jpg";
    int32_t fileId4 = PrepareHighQualityPhoto(photoId4, diplayName3);
    UpdateCEAvailable(fileId3, 3, true);
    string photoUri4 = "file://media/Photo/" + to_string(fileId4) +
        "/IMG_1722329102_003/" + TESTING_DISPLAYNAME;
    EnhancementTaskManager::AddEnhancementTask(fileId4, photoId4);
    
    uris.emplace_back(photoUri2);
    uris.emplace_back(photoUri3);
    uris.emplace_back(photoUri4);
    predicates.In(MediaColumn::MEDIA_ID, uris);

    EnhancementManager &instance = EnhancementManager::GetInstance();
    string uriStr = PAH_CLOUD_ENHANCEMENT_ADD;
    MediaFileUtils::UriAppendKeyValue(uriStr, MEDIA_OPERN_KEYWORD, "true");
    Uri addTaskUri(uriStr);
    MediaLibraryCommand cmd(addTaskUri);
    cmd.SetDataSharePred(predicates);
    int32_t result = instance.HandleAddOperation(cmd, false);
    EXPECT_EQ(result, -1);

    result = instance.HandleAddOperation(cmd, true);
    EXPECT_EQ(result, -1);
    MEDIA_INFO_LOG("manager_handle_add_operation_004 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, manager_handle_add_operation_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_add_operation_005 Start");
    EnhancementManager &instance = EnhancementManager::GetInstance();
    string uriStr = PAH_CLOUD_ENHANCEMENT_ADD;
    MediaFileUtils::UriAppendKeyValue(uriStr, MEDIA_OPERN_KEYWORD, "true");
    Uri addTaskUri(uriStr);
    MediaLibraryCommand cmd(addTaskUri);
    int32_t fileId = PrepareHighQualityPhoto(TESTING_PHOTO_ID, TESTING_DISPLAYNAME);
    UpdateCEAvailable(fileId, 1);
    DataSharePredicates predicates;
    string photoUri = "file://media/Photo/" + to_string(fileId) + "/IMG_1722329102_000/" + TESTING_DISPLAYNAME;
    vector<string> uris;
    uris.emplace_back(photoUri);
    predicates.In(MediaColumn::MEDIA_ID, uris);
    cmd.SetDataSharePred(predicates);
    int32_t result = instance.HandleAddOperation(cmd, true);
    // add service task must failed, due to premission
    EXPECT_EQ(result, -1);
    MEDIA_INFO_LOG("manager_handle_add_operation_005 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, manager_handle_prioritize_operation_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_prioritize_operation_006 Start");
    string uriStr = PAH_CLOUD_ENHANCEMENT_PRIORITIZE;
    Uri prioritizeTaskUri(uriStr);
    MediaLibraryCommand cmd(prioritizeTaskUri);
    EnhancementManager &instance = EnhancementManager::GetInstance();

    int32_t fileId = PrepareHighQualityPhoto(TESTING_PHOTO_ID, TESTING_DISPLAYNAME);
    string photoUri = "file://media/Photo/" + to_string(fileId) + "/IMG_1722329102_000/" + TESTING_DISPLAYNAME;
    DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, photoUri);
    UpdateCEAvailable(fileId, 1);
    cmd.SetDataSharePred(predicates);
    auto result = instance.HandlePrioritizeOperation(cmd);
    EXPECT_EQ(result, -1);

    EnhancementTaskManager::AddEnhancementTask(fileId, TESTING_PHOTO_ID);
    UpdateCEAvailable(fileId, 2);
    result = instance.HandlePrioritizeOperation(cmd);
    EXPECT_EQ(result, -1);
    
    EnhancementTaskManager::RemoveEnhancementTask(TESTING_PHOTO_ID);
    result = instance.HandlePrioritizeOperation(cmd);
    EXPECT_EQ(result, -1);
    MEDIA_INFO_LOG("manager_handle_prioritize_operation_006 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, manager_handle_cancel_operation_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_cancel_operation_007 Start");
    EnhancementManager &instance = EnhancementManager::GetInstance();
    string uriStr = PAH_CLOUD_ENHANCEMENT_CANCEL;
    Uri cancelTaskUri(uriStr);
    MediaLibraryCommand cmd(cancelTaskUri);
    int32_t fileId = PrepareHighQualityPhoto(TESTING_PHOTO_ID, TESTING_DISPLAYNAME);
    UpdateCEAvailable(fileId, 1);

    string photoUri = "file://media/Photo/" + to_string(fileId) + "/IMG_1722329102_000/" + TESTING_DISPLAYNAME;
    DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, photoUri);
    cmd.SetDataSharePred(predicates);
    auto result = instance.HandleCancelOperation(cmd);
    EXPECT_EQ(result, 0);

    EnhancementTaskManager::AddEnhancementTask(fileId, TESTING_PHOTO_ID);
    UpdateCEAvailable(fileId, 2);
    result = instance.HandleCancelOperation(cmd);
    EnhancementTaskManager::RemoveEnhancementTask(TESTING_PHOTO_ID);
    EXPECT_EQ(result, 0);

    result = instance.HandleCancelOperation(cmd);
    EXPECT_EQ(result, 0);

    MEDIA_INFO_LOG("manager_handle_cancel_operation_007 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, manager_handle_cancel_all_operation_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_cancel_all_operation_008 Start");
    EnhancementManager &instance = EnhancementManager::GetInstance();
    int32_t result = instance.HandleCancelAllOperation();
    EXPECT_EQ(result, -1);
    result = instance.HandleCancelAllOperation();
    EXPECT_EQ(result, -1);
    MEDIA_INFO_LOG("manager_handle_cancel_all_operation_008 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, manager_handle_sync_operation_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_sync_operation_009 Start");
    EnhancementManager &instance = EnhancementManager::GetInstance();
    int32_t result = instance.HandleSyncOperation();
    EXPECT_EQ(result, -1);
    MEDIA_INFO_LOG("manager_handle_sync_operation_009 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, manager_handle_query_operation_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_handle_query_operation_010 Start");
    EnhancementManager &instance = EnhancementManager::GetInstance();
    int32_t fileId = PrepareHighQualityPhoto(TESTING_PHOTO_ID, TESTING_DISPLAYNAME);
    UpdateCEAvailable(fileId, 1);
    string uriStr = PAH_CLOUD_ENHANCEMENT_QUERY;
    Uri queryTaskUri(uriStr);
    MediaLibraryCommand cmd(queryTaskUri);
    DataSharePredicates predicates;
    string photoUri = "file://media/Photo/" + to_string(fileId) + "/IMG_1722329102_000/" + TESTING_DISPLAYNAME;
    predicates.EqualTo(MediaColumn::MEDIA_ID, photoUri);
    cmd.SetDataSharePred(predicates);
    vector<string> columns = {
        MediaColumn::MEDIA_ID,
        PhotoColumn::PHOTO_ID,
        PhotoColumn::PHOTO_CE_AVAILABLE,
        PhotoColumn::PHOTO_CE_STATUS_CODE
    };
    shared_ptr<NativeRdb::ResultSet> resultSet = instance.HandleQueryOperation(cmd, columns);
    EXPECT_NE(resultSet, nullptr);
    int32_t count = 0;
    auto ret = resultSet->GetRowCount(count);
    EXPECT_GT(count, 0);

    EnhancementTaskManager::AddEnhancementTask(fileId, TESTING_PHOTO_ID);
    resultSet = instance.HandleQueryOperation(cmd, columns);
    EnhancementTaskManager::RemoveEnhancementTask(TESTING_PHOTO_ID);
    EXPECT_NE(resultSet, nullptr);
    count = 0;
    ret = resultSet->GetRowCount(count);
    EXPECT_GT(count, 0);

    MEDIA_INFO_LOG("manager_handle_query_operation_010 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, manager_cancel_tasks_internal_011, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_cancel_tasks_internal_011 Start");
    EnhancementManager &instance = EnhancementManager::GetInstance();
    int32_t fileId = PrepareHighQualityPhoto(TESTING_PHOTO_ID, TESTING_DISPLAYNAME);
    UpdateCEAvailable(fileId, 2);
    string photoId = "202408261739";
    string displayName = "CloudEnhancementTest002.jpg";
    int32_t fileId2 = PrepareHighQualityPhoto(photoId, displayName);
    UpdateCEAvailable(fileId2, 5);
    vector<string> fileIds;
    fileIds.emplace_back(to_string(fileId));
    fileIds.emplace_back("-1");
    fileIds.emplace_back(to_string(fileId2));
    vector<string> photoIds;
    instance.CancelTasksInternal(fileIds, photoIds, CloudEnhancementAvailableType::EDIT);
    EXPECT_EQ(photoIds.size(), 0);

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    ASSERT_NE(g_rdbStore, nullptr);

    vector<string> columns = { PhotoColumn::PHOTO_CE_AVAILABLE };
    auto resultSet = g_rdbStore->Query(cmd, columns);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);

    int32_t ceAvailable = GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet);
    EXPECT_EQ(ceAvailable, static_cast<int32_t>(CloudEnhancementAvailableType::EDIT));

    MEDIA_INFO_LOG("manager_cancel_tasks_internal_011 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, manager_remove_tasks_internal_012, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_remove_tasks_internal_012 Start");
    EnhancementManager &instance = EnhancementManager::GetInstance();
    int32_t fileId = PrepareHighQualityPhoto(TESTING_PHOTO_ID, TESTING_DISPLAYNAME);
    UpdateCEAvailable(fileId, 2);
    vector<string> fileIds;
    vector<string> photoIds;
    fileIds.emplace_back(to_string(fileId));
    instance.RemoveTasksInternal(fileIds, photoIds);
    UpdateCEAvailable(fileId, 7);
    instance.RemoveTasksInternal(fileIds, photoIds);
    EXPECT_EQ(photoIds.size(), 0);
    MEDIA_INFO_LOG("manager_remove_tasks_internal_012 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, manager_revert_and_revocer_internal_013, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_edit_and_revert_internal_013 Start");
    EnhancementManager &instance = EnhancementManager::GetInstance();
    int32_t fileId = PrepareHighQualityPhoto(TESTING_PHOTO_ID, TESTING_DISPLAYNAME);
    UpdateCEAvailable(fileId, 1);
    bool ret = instance.RevertEditUpdateInternal(fileId);
    EXPECT_EQ(ret, false);
    UpdateCEAvailable(fileId, 6);
    ret = instance.RevertEditUpdateInternal(fileId);
    EXPECT_EQ(ret, true);
    vector<string> fileIds;
    fileIds.emplace_back(to_string(fileId));
    ret = instance.RecoverTrashUpdateInternal(fileIds);
    EXPECT_EQ(ret, false);
    UpdateCEAvailable(fileId, 7);
    ret = instance.RecoverTrashUpdateInternal(fileIds);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("manager_edit_and_revert_internal_013 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, manager_add_service_task_014, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_add_service_task_014 Start");
    EnhancementManager &instance = EnhancementManager::GetInstance();
    MediaEnhanceBundleHandle* mediaEnhanceBundle = instance.enhancementService_->CreateBundle();
    int32_t fileId = PrepareHighQualityPhoto(TESTING_PHOTO_ID, TESTING_DISPLAYNAME);
    UpdateCEAvailable(fileId, 1);
    int32_t ret = instance.AddServiceTask(mediaEnhanceBundle, fileId, TESTING_PHOTO_ID, true);
    EXPECT_NE(ret, 0);
    mediaEnhanceBundle = instance.enhancementService_->CreateBundle();
    ret = instance.AddServiceTask(mediaEnhanceBundle, fileId, TESTING_PHOTO_ID, false);
    EXPECT_EQ(ret, -1);

    vector<string> columns = { PhotoColumn::PHOTO_CE_AVAILABLE, PhotoColumn::PHOTO_HAS_CLOUD_WATERMARK };
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    ASSERT_NE(g_rdbStore, nullptr);

    auto resultSet = g_rdbStore->Query(cmd, columns);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);

    int32_t ceAvailable = GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet);
    int32_t hasCloudWaterMark = GetInt32Val(PhotoColumn::PHOTO_HAS_CLOUD_WATERMARK, resultSet);
    EXPECT_EQ(ceAvailable, static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
    EXPECT_EQ(hasCloudWaterMark, 0);

    string photoId = "202408302001001";
    string displayName = "test.jpg";
    fileId = PrepareHighQualityPhoto(photoId, displayName);
    UpdateCEAvailable(fileId, 1);
    mediaEnhanceBundle = instance.enhancementService_->CreateBundle();
    ret = instance.AddServiceTask(mediaEnhanceBundle, fileId, TESTING_PHOTO_ID, true);
    EXPECT_EQ(ret, -1);
    MEDIA_INFO_LOG("manager_add_service_task_014 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, task_manager_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("task_manager_test_001 Start");
    string photoId = "202408302001001";
    int32_t fileId = PrepareHighQualityPhoto(photoId, TESTING_DISPLAYNAME);
    EnhancementTaskManager::AddEnhancementTask(fileId, photoId);
    EXPECT_EQ(EnhancementTaskManager::InProcessingTask(photoId), true);
    EnhancementTaskManager::RemoveEnhancementTask(photoId);
    EXPECT_EQ(EnhancementTaskManager::InProcessingTask(photoId), false);
    EnhancementTaskManager::RemoveEnhancementTask(photoId);
    
    string photoId2 = "202408302001002";
    int32_t fileId2 = PrepareHighQualityPhoto(photoId2, TESTING_DISPLAYNAME);
    EnhancementTaskManager::AddEnhancementTask(fileId, photoId);
    EnhancementTaskManager::AddEnhancementTask(fileId2, photoId2);
    EXPECT_EQ(EnhancementTaskManager::QueryPhotoIdByFileId(fileId), photoId);
    EXPECT_EQ(EnhancementTaskManager::QueryPhotoIdByFileId(fileId2), photoId2);

    vector<string> taskIds;
    EnhancementTaskManager::RemoveAllEnhancementTask(taskIds);
    for (string &taskId : taskIds) {
        EXPECT_EQ((taskId == photoId || taskId == photoId2), true);
    }
    MEDIA_INFO_LOG("task_manager_test_001 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, enhancement_adapter_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("task_manager_test_001 Start");
    shared_ptr<EnhancementServiceAdapter> enhancementService_ = make_shared<EnhancementServiceAdapter>();
    int32_t ret = enhancementService_->LoadEnhancementService();
    EXPECT_EQ(ret, 0);
    string photoId = "202408302001001";
    MediaEnhanceBundleHandle* bundle = enhancementService_->CreateBundle();
    ret = enhancementService_->AddTask(photoId, bundle);
    EXPECT_EQ(ret, -1);
    ret = enhancementService_->CancelTask(photoId);
    EXPECT_EQ(ret, -1);
    ret = enhancementService_->RemoveTask(photoId);
    EXPECT_EQ(ret, -1);
    ret = enhancementService_->CancelAllTasks();
    EXPECT_EQ(ret, -1);
    vector<string> taskIdList;
    ret = enhancementService_->GetPendingTasks(taskIdList);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(taskIdList.size(), 0);
    MEDIA_INFO_LOG("task_manager_test_001 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, enhancement_callback_on_success_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enhancement_callback_on_success_001 Start");
    const char* taskId = "";
    shared_ptr<EnhancementServiceAdapter> enhancementService_ = make_shared<EnhancementServiceAdapter>();
    MediaEnhanceBundleHandle* bundle = nullptr;
    EnhancementServiceCallback::OnSuccess(taskId, bundle);
    const char* taskId2 = "202408302001001";
    EnhancementServiceCallback::OnSuccess(taskId2, bundle);
    bundle = enhancementService_->CreateBundle();
    EnhancementServiceCallback::OnSuccess(taskId2, bundle);
    MEDIA_INFO_LOG("enhancement_callback_on_success_001 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, enhancement_callback_save_cloud_enhancement_photo_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("enhancement_callback_save_cloud_enhancement_photo_002 Start");
    string sourceDisplayName;
    CloudEnhancementThreadTask task("", 0, nullptr, 0, true);
    int32_t sourceFileId = -1;
    uint32_t size = static_cast<uint32_t>(sizeof(BUFFER));
    uint8_t* buffer = new uint8_t[size];
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = BUFFER[i];
    }
    string sourceFilePath;
    int32_t sourceSubtype = 0;
    shared_ptr<CloudEnhancementFileInfo> fileInfo = make_shared<CloudEnhancementFileInfo>(sourceFileId, sourceFilePath,
        sourceDisplayName, sourceSubtype, 0);

    string photoId = "202408302001001";
    sourceFileId = PrepareHighQualityPhoto(photoId, "test.jpg");
    sourceDisplayName = "a.f";
    fileInfo = make_shared<CloudEnhancementFileInfo>(sourceFileId, sourceFilePath, sourceDisplayName, sourceSubtype, 0);
    
    auto resultSet = GetQueryResultSet(photoId);
    ASSERT_NE(resultSet, nullptr);

    int32_t ret = EnhancementServiceCallback::SaveCloudEnhancementPhoto(fileInfo, task, resultSet);
    EXPECT_EQ(ret <= 0, true);
    sourceDisplayName = "test.jpg";
    fileInfo = make_shared<CloudEnhancementFileInfo>(sourceFileId, sourceFilePath, sourceDisplayName, sourceSubtype, 0);

    ret = EnhancementServiceCallback::SaveCloudEnhancementPhoto(fileInfo, task, resultSet);
    EXPECT_EQ(ret, -1);

    string photoId2 = "202408302001002";
    sourceFileId = PrepareHighQualityPhoto(photoId2, TESTING_DISPLAYNAME);
    task.addr = buffer;
    task.bytes = size;
    fileInfo = make_shared<CloudEnhancementFileInfo>(sourceFileId, sourceFilePath, sourceDisplayName, sourceSubtype, 0);
    resultSet = GetQueryResultSet(photoId2);
    ASSERT_NE(resultSet, nullptr);
    ret = EnhancementServiceCallback::SaveCloudEnhancementPhoto(fileInfo, task, resultSet);
    EXPECT_GT(ret, 0);

    string photoId3 = "202408302001003";
    sourceFileId = PrepareHighQualityPhoto(photoId3, "IMG_20240830_200151.jpg");
    sourceSubtype = 3;
    buffer = new uint8_t[size];
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = BUFFER[i];
    }
    task.addr = buffer;
    fileInfo = make_shared<CloudEnhancementFileInfo>(sourceFileId, sourceFilePath, sourceDisplayName, sourceSubtype, 0);
    resultSet = GetQueryResultSet(photoId3);
    ASSERT_NE(resultSet, nullptr);
    ret = EnhancementServiceCallback::SaveCloudEnhancementPhoto(fileInfo, task, resultSet);
    EXPECT_GT(ret, 0);
    MEDIA_INFO_LOG("enhancement_callback_save_cloud_enhancement_photo_002 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, enhancement_callback_create_cloud_enhancement_photo_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("enhancement_callback_create_cloud_enhancement_photo_003 Start");
    string filePath;
    string displayName = TESTING_DISPLAYNAME;
    string photoId = "202408302001001";
    int32_t sourceFileId = PrepareHighQualityPhoto(photoId, displayName);
    shared_ptr<CloudEnhancementFileInfo> info = make_shared<CloudEnhancementFileInfo>(sourceFileId,
        filePath, displayName, 0, 0);
    auto resultSet = GetQueryResultSet(photoId);
    ASSERT_NE(resultSet, nullptr);
    int32_t newFileId = EnhancementServiceCallback::CreateCloudEnhancementPhoto(sourceFileId, info,
        resultSet);
    EXPECT_GT(newFileId, 0);

    vector<string> columns = {
        PhotoColumn::PHOTO_CE_AVAILABLE
    };
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, to_string(newFileId));
    ASSERT_NE(g_rdbStore, nullptr);

    resultSet = g_rdbStore->Query(cmd, columns);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);

    int32_t ceAvailable = GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet);
    EXPECT_EQ(ceAvailable, static_cast<int32_t>(CloudEnhancementAvailableType::FINISH));

    MEDIA_INFO_LOG("enhancement_callback_create_cloud_enhancement_photo_003 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, enhancement_callback_on_failed_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("enhancement_callback_on_failed_004 Start");
    const char* taskId = "";
    EnhancementManager &instance = EnhancementManager::GetInstance();
    MediaEnhanceBundleHandle* bundle = instance.enhancementService_->CreateBundle();
    EnhancementServiceCallback::OnFailed(taskId, bundle);
    instance.enhancementService_->PutInt(bundle, MediaEnhance_Bundle_Key::ERROR_CODE, 0);
    const char* taskId2 = "202408302001001";
    EnhancementServiceCallback::OnFailed(taskId2, bundle);

    instance.enhancementService_->PutInt(bundle, MediaEnhance_Bundle_Key::ERROR_CODE,
        static_cast<int32_t>(CEErrorCodeType::NETWORK_WEAK));
    int32_t fileId = PrepareHighQualityPhoto(string(taskId2), TESTING_DISPLAYNAME);
    UpdateCEAvailable(fileId, 1);
    ASSERT_GT(fileId, 0);
    EnhancementServiceCallback::OnFailed(taskId2, bundle);
    this_thread::sleep_for(chrono::seconds(1));

    vector<string> columns = {
        PhotoColumn::PHOTO_CE_AVAILABLE
    };
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, to_string(fileId));
    ASSERT_NE(g_rdbStore, nullptr);

    auto resultSet = g_rdbStore->Query(cmd, columns);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);

    int32_t ceAvailable = GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet);
    EXPECT_EQ(ceAvailable, static_cast<int32_t>(CloudEnhancementAvailableType::FAILED_RETRY));

    MEDIA_INFO_LOG("enhancement_callback_on_failed_004 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, enhancement_callback_on_failed_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("enhancement_callback_on_failed_005 Start");
    const char* taskId = "202408302001001";
    EnhancementManager &instance = EnhancementManager::GetInstance();
    MediaEnhanceBundleHandle* bundle = instance.enhancementService_->CreateBundle();
    instance.enhancementService_->PutInt(bundle, MediaEnhance_Bundle_Key::ERROR_CODE,
        static_cast<int32_t>(CEErrorCodeType::NON_RECOVERABLE));
    int32_t fileId = PrepareHighQualityPhoto(string(taskId), TESTING_DISPLAYNAME);
    ASSERT_GT(fileId, 0);
    UpdateCEAvailable(fileId, 2);
    EnhancementServiceCallback::OnFailed(taskId, bundle);
    this_thread::sleep_for(chrono::seconds(1));
    vector<string> columns = {
        PhotoColumn::PHOTO_CE_AVAILABLE,
        PhotoColumn::PHOTO_CE_STATUS_CODE
    };
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, to_string(fileId));
    ASSERT_NE(g_rdbStore, nullptr);

    auto resultSet = g_rdbStore->Query(cmd, columns);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);

    int32_t ceAvailable = GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet);
    int32_t ceStatusCode = GetInt32Val(PhotoColumn::PHOTO_CE_STATUS_CODE, resultSet);
    EXPECT_EQ(ceAvailable, static_cast<int32_t>(CloudEnhancementAvailableType::FAILED));
    EXPECT_EQ(ceStatusCode, static_cast<int32_t>(CEErrorCodeType::NON_RECOVERABLE));
    MEDIA_INFO_LOG("enhancement_callback_on_failed_005 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, enhancement_callback_on_service_reconnected_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("enhancement_callback_on_service_reconnected_006 Start");
    int32_t fileId = PrepareHighQualityPhoto(TESTING_PHOTO_ID, TESTING_DISPLAYNAME);
    UpdateCEAvailable(fileId, 2);
    EnhancementServiceCallback::OnServiceReconnected();
    EXPECT_EQ(EnhancementTaskManager::InProcessingTask(TESTING_PHOTO_ID), false);
    MEDIA_INFO_LOG("enhancement_callback_on_service_reconnected_006 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, enhancement_callback_deal_with_successed_task_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("enhancement_callback_deal_with_successed_task_007 Start");
    CloudEnhancementThreadTask task("", 0, nullptr, 0, true);
    EnhancementServiceCallback::DealWithSuccessedTask(task);
    string taskId = "202408302001001";
    int32_t fileId = PrepareHighQualityPhoto(taskId, TESTING_DISPLAYNAME);
    task.taskId = taskId;
    uint32_t size = static_cast<uint32_t>(sizeof(BUFFER));
    uint8_t* buffer = new uint8_t[size];
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = BUFFER[i];
    }
    task.addr = buffer;
    task.bytes = size;
    EnhancementServiceCallback::DealWithSuccessedTask(task);
    vector<string> columns = { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH, MediaColumn::MEDIA_NAME,
        MediaColumn::MEDIA_HIDDEN, PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::PHOTO_CE_AVAILABLE,
        PhotoColumn::PHOTO_STRONG_ASSOCIATION, PhotoColumn::PHOTO_ASSOCIATE_FILE_ID};
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    ASSERT_NE(g_rdbStore, nullptr);
    auto resultSet = g_rdbStore->Query(cmd, columns);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);
    fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    string filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    int32_t hidden = GetInt32Val(MediaColumn::MEDIA_HIDDEN, resultSet);
    int32_t subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    int32_t ceAvailable = GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet);
    int32_t association = GetInt32Val(PhotoColumn::PHOTO_STRONG_ASSOCIATION, resultSet);
    int32_t associateFileId = GetInt32Val(PhotoColumn::PHOTO_ASSOCIATE_FILE_ID, resultSet);
    EXPECT_EQ(displayName.find("_enhanced") != string::npos, false);
    EXPECT_EQ(ceAvailable, static_cast<int32_t>(CloudEnhancementAvailableType::SUCCESS));
    EXPECT_EQ(association, 0);
    EXPECT_EQ(FileUtils::IsFileExist(filePath), true);
    TestCloudEnhancementImage(columns, associateFileId, fileId, hidden, subtype);
    MEDIA_INFO_LOG("enhancement_callback_deal_with_successed_task_007 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, enhancement_database_operations_query_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enhancement_database_operations_query_001 Start");
    string uri = "file";
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, uri);
    cmd.SetDataSharePred(predicates);
    RdbPredicates servicePredicates(PhotoColumn::PHOTOS_TABLE);
    vector<string> columns = {
        PhotoColumn::PHOTO_CE_AVAILABLE
    };
    shared_ptr<NativeRdb::ResultSet> resultSet = EnhancementDatabaseOperations::Query(cmd, servicePredicates, columns);
    EXPECT_EQ(resultSet, nullptr);

    int32_t fileId = PrepareHighQualityPhoto(TESTING_PHOTO_ID, TESTING_DISPLAYNAME);
    UpdateCEAvailable(fileId, 2);
    uri = "file://media/Photo/" + to_string(fileId) + "/IMG_1722329102_000/IMG_20240730_164322.jpg";
    DataSharePredicates normalPredicates;
    normalPredicates.EqualTo(MediaColumn::MEDIA_ID, uri);
    cmd.SetDataSharePred(normalPredicates);
    resultSet = EnhancementDatabaseOperations::Query(cmd, servicePredicates, columns);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToNextRow(), E_OK);
    int32_t ceAvailable = GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet);
    EXPECT_EQ(ceAvailable, 2);

    MEDIA_INFO_LOG("enhancement_database_operations_query_001 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, enhancement_database_operations_batch_query_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("enhancement_database_operations_batch_query_002 Start");
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    DataSharePredicates predicates;
    vector<string> uris;
    uris.push_back("file");
    uris.push_back("file://");
    predicates.In(MediaColumn::MEDIA_ID, uris);
    cmd.SetDataSharePred(predicates);
    vector<string> columns = {
        PhotoColumn::PHOTO_CE_AVAILABLE
    };
    unordered_map<int32_t, string> fileId2Uri;
    shared_ptr<NativeRdb::ResultSet> resultSet = EnhancementDatabaseOperations::BatchQuery(cmd, columns, fileId2Uri);
    EXPECT_EQ(resultSet, nullptr);

    int32_t fileId = PrepareHighQualityPhoto(TESTING_PHOTO_ID, TESTING_DISPLAYNAME);
    UpdateCEAvailable(fileId, 2);
    string uri = "file://media/Photo/" + to_string(fileId) + "/IMG_1722329102_000/IMG_20240730_164322.jpg";
    DataSharePredicates predicates2;
    uris.push_back(uri);
    predicates2.In(MediaColumn::MEDIA_ID, uris);
    cmd.SetDataSharePred(predicates2);
    resultSet = EnhancementDatabaseOperations::BatchQuery(cmd, columns, fileId2Uri);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToNextRow(), E_OK);
    int32_t ceAvailable = GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet);
    EXPECT_EQ(ceAvailable, 2);
    MEDIA_INFO_LOG("enhancement_database_operations_batch_query_002 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, enhancement_database_operations_update_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("enhancement_database_operations_update_003 Start");
    ValuesBucket bucket;
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    int32_t fileId = PrepareHighQualityPhoto(TESTING_PHOTO_ID, TESTING_DISPLAYNAME);
    UpdateCEAvailable(fileId, 2);
    bucket.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE, 1);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    predicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE, 2);
    int32_t ret = EnhancementDatabaseOperations::Update(bucket, predicates);
    EXPECT_EQ(ret, E_OK);
    ret = EnhancementDatabaseOperations::Update(bucket, predicates);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("enhancement_database_operations_update_003 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, enhancement_database_operations_insert_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("enhancement_database_operations_insert_004 Start");
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    FileAsset fileAsset;
    int32_t fileId = PrepareHighQualityPhoto(TESTING_PHOTO_ID, TESTING_DISPLAYNAME);
    cmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, to_string(fileId));
    ASSERT_NE(g_rdbStore, nullptr);

    vector<string> columns = {
        PhotoColumn::PHOTO_DIRTY
    };
    auto resultSet = g_rdbStore->Query(cmd, columns);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);

    int32_t dirty = GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet);
    EXPECT_EQ(dirty, 1);

    shared_ptr<CloudEnhancementFileInfo> info = make_shared<CloudEnhancementFileInfo>();
    info->hidden = 0;
    info->subtype = 3; // moving photo
    int32_t ret = EnhancementDatabaseOperations::InsertCloudEnhancementImageInDb(cmd, fileAsset,
        fileId, info, resultSet);
    EXPECT_GT(ret, 0);
    resultSet = g_rdbStore->Query(cmd, columns);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);

    int32_t dirtyAfter = GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet);
    EXPECT_EQ(dirtyAfter, 1);

    string photoId = "202408261752";
    string displayName = "CloudEnhancementTest002.jpg";
    fileId = PrepareHighQualityPhoto(photoId, displayName);
    info->subtype = 1;
    resultSet = GetQueryResultSet(photoId);
    ASSERT_NE(resultSet, nullptr);
    ret = EnhancementDatabaseOperations::InsertCloudEnhancementImageInDb(cmd, fileAsset,
        fileId, info, resultSet);
    EXPECT_GT(ret, 0);

    MEDIA_INFO_LOG("enhancement_database_operations_insert_004 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, manager_is_add_operation_enabled_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_is_add_operation_enabled_001 Start");
    EnhancementManager &instance = EnhancementManager::GetInstance();
    string uriStr = PAH_CLOUD_ENHANCEMENT_ADD;
    MediaFileUtils::UriAppendKeyValue(uriStr, MEDIA_OPERN_KEYWORD, "true");
    Uri addTaskUri(uriStr);
    MediaLibraryCommand cmd(addTaskUri);
    int32_t fileId = PrepareHighQualityPhoto(TESTING_PHOTO_ID, TESTING_DISPLAYNAME);
    UpdateCEAvailable(fileId, 1);
    DataSharePredicates predicates;
    string photoUri = "file://media/Photo/" + to_string(fileId) + "/IMG_1722329102_000/" + TESTING_DISPLAYNAME;
    vector<string> uris;
    uris.emplace_back(photoUri);
    predicates.In(MediaColumn::MEDIA_ID, uris);
    cmd.SetDataSharePred(predicates);
    instance.HandlePhotosAutoOptionChange("close");
    int32_t result = instance.HandleAddOperation(cmd, true, 1);
    EXPECT_EQ(result, -1);
    MEDIA_INFO_LOG("manager_is_add_operation_enabled_001 End");
}

HWTEST_F(MediaLibraryCloudEnhancementTest, manager_is_add_operation_enabled_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_is_add_operation_enabled_002 Start");
    EnhancementManager &instance = EnhancementManager::GetInstance();
    string uriStr = PAH_CLOUD_ENHANCEMENT_ADD;
    MediaFileUtils::UriAppendKeyValue(uriStr, MEDIA_OPERN_KEYWORD, "true");
    Uri addTaskUri(uriStr);
    MediaLibraryCommand cmd(addTaskUri);
    int32_t fileId = PrepareHighQualityPhoto(TESTING_PHOTO_ID, TESTING_DISPLAYNAME);
    UpdateCEAvailable(fileId, 1);
    DataSharePredicates predicates;
    string photoUri = "file://media/Photo/" + to_string(fileId) + "/IMG_1722329102_000/" + TESTING_DISPLAYNAME;
    vector<string> uris;
    uris.emplace_back(photoUri);
    predicates.In(MediaColumn::MEDIA_ID, uris);
    cmd.SetDataSharePred(predicates);
    instance.HandlePhotosAutoOptionChange("WLAN only");
    instance.HandleNetChange(false, true);
    int32_t result = instance.HandleAddOperation(cmd, true, 1);
    EXPECT_EQ(result, -1);
    MEDIA_INFO_LOG("manager_is_add_operation_enabled_002 End");
}
#endif
} // namespace Media
} // namespace OHOS

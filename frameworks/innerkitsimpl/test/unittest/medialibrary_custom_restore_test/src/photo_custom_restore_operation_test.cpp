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

#define MLOG_TAG "CustomRestoreCallbackUnitTest"

#include "photo_custom_restore_operation_test.h"

#define private public
#include "photo_custom_restore_operation.h"
#undef private

#include "custom_restore_const.h"
#include "custom_restore_source_test.h"
#include "directory_ex.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "photo_album_column.h"
#include "photo_file_utils.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"
#include "media_upgrade.h"

using namespace testing;
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
shared_ptr<MediaLibraryRdbStore> g_rdbStore;
const size_t DETAIL_TIME_SIZE = 20;

int32_t PhotoCustomRestoreOperationTest::ExecSqls(const vector<string> &sqls)
{
    EXPECT_NE(g_rdbStore, nullptr);
    int32_t err = E_OK;
    for (const auto &sql : sqls) {
        err = g_rdbStore->ExecuteSql(sql);
        MEDIA_INFO_LOG("exec sql: %{public}s result: %{public}d", sql.c_str(), err);
        EXPECT_EQ(err, E_OK);
    }
    return E_OK;
}

void PhotoCustomRestoreOperationTest::ClearTables()
{
    vector<string> createTableSqlList = {
        "DELETE FROM " + PhotoColumn::PHOTOS_TABLE,
        "DELETE FROM " + PhotoAlbumColumns::TABLE,
    };
    MEDIA_INFO_LOG("start clear data");
    ExecSqls(createTableSqlList);
}

void PhotoCustomRestoreOperationTest::SetTables()
{
    vector<string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        PhotoAlbumColumns::CREATE_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        ASSERT_NE(g_rdbStore, nullptr);

        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        ASSERT_EQ(ret, NativeRdb::E_OK);
        MEDIA_INFO_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
}

void PhotoCustomRestoreOperationTest::SetUpTestCase()
{
    MEDIA_INFO_LOG("PhotoCustomRestoreOperationTest SetUpTestCase");

    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    SetTables();
}

void PhotoCustomRestoreOperationTest::TearDownTestCase()
{
    MEDIA_INFO_LOG("PhotoCustomRestoreOperationTest TearDownTestCase");
    ClearTables();
}

void PhotoCustomRestoreOperationTest::SetUp()
{
    MEDIA_INFO_LOG("PhotoCustomRestoreOperationTest SetUp");
    ClearTables();
}

void PhotoCustomRestoreOperationTest::TearDown()
{
    MEDIA_INFO_LOG("PhotoCustomRestoreOperationTest TearDown");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_001 Start");
    PhotoCustomRestoreOperation::GetInstance();
    PhotoCustomRestoreOperation::GetInstance();
    EXPECT_EQ(PhotoCustomRestoreOperation::instance_ != nullptr, true);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    operatorObj.isRunning_.store(false);
    operatorObj.Start();
    EXPECT_EQ(operatorObj.isRunning_, false);
    operatorObj.isRunning_.store(true);
    operatorObj.Start();
    EXPECT_EQ(operatorObj.isRunning_, true);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_003 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    RestoreTaskInfo restoreTaskInfo2;
    restoreTaskInfo2.keyPath = "restoreTaskInfo2";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    EXPECT_EQ(PhotoCustomRestoreOperation::instance_ != nullptr, true);
    operatorObj.AddTask(restoreTaskInfo);
    operatorObj.AddTask(restoreTaskInfo2);
    operatorObj.cancelKeySet_.insert("restoreTaskInfo2");
    operatorObj.Start();
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_003 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_004 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    RestoreTaskInfo restoreTaskInfo2;
    restoreTaskInfo2.keyPath = "restoreTaskInfo2";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    operatorObj.isRunning_.store(true);
    operatorObj.AddTask(restoreTaskInfo);
    operatorObj.AddTask(restoreTaskInfo2);
    operatorObj.cancelKeySet_.insert("restoreTaskInfo2");
    operatorObj.Start();
    EXPECT_EQ(operatorObj.isRunning_, true);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_004 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_005 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    operatorObj.CancelTask(restoreTaskInfo);
    EXPECT_EQ(operatorObj.IsCancelTask(restoreTaskInfo), true);
    operatorObj.CancelTaskFinish(restoreTaskInfo);
    EXPECT_EQ(operatorObj.IsCancelTask(restoreTaskInfo), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_005 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_006 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    operatorObj.ApplyEfficiencyQuota(1);
    EXPECT_EQ(operatorObj.IsCancelTask(restoreTaskInfo), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_006 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_007 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    operatorObj.AddTask(restoreTaskInfo);
    EXPECT_EQ(operatorObj.IsCancelTask(restoreTaskInfo), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_007 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_008, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_008 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    operatorObj.AddTask(restoreTaskInfo);
    EXPECT_EQ(operatorObj.IsCancelTask(restoreTaskInfo), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_008 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_009, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_008 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    operatorObj.DoCustomRestore(restoreTaskInfo);
    EXPECT_EQ(operatorObj.IsCancelTask(restoreTaskInfo), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_008 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_010, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_010 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    operatorObj.ReleaseCustomRestoreTask(restoreTaskInfo);
    EXPECT_EQ(operatorObj.IsCancelTask(restoreTaskInfo), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_010 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_011, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_011 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    operatorObj.ReportCustomRestoreTask(restoreTaskInfo);
    EXPECT_EQ(operatorObj.IsCancelTask(restoreTaskInfo), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_011 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_012, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_012 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    operatorObj.ReportCustomRestoreTask(restoreTaskInfo);
    EXPECT_EQ(operatorObj.IsCancelTask(restoreTaskInfo), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_012 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_014, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_014 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    vector<string> files = {"test", "test2", "test3", "test4"};
    const unordered_map<string, TimeInfo> timeInfoMap = operatorObj.GetTimeInfoMap(restoreTaskInfo);
    int32_t firstRestoreIndex = 0;
    operatorObj.HandleFirstRestoreFile(timeInfoMap, restoreTaskInfo, files, 0, firstRestoreIndex);
    EXPECT_EQ(operatorObj.IsCancelTask(restoreTaskInfo), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_014 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_015, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_015 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    vector<string> files = { "test", "test2", "test3", "test4" };
    const unordered_map<string, TimeInfo> timeInfoMap = operatorObj.GetTimeInfoMap(restoreTaskInfo);
    operatorObj.HandleBatchCustomRestore(timeInfoMap, restoreTaskInfo, 2, files);
    EXPECT_EQ(operatorObj.IsCancelTask(restoreTaskInfo), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_015 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_016, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_016 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    operatorObj.InitRestoreTask(restoreTaskInfo, 2);
    EXPECT_EQ(operatorObj.IsCancelTask(restoreTaskInfo), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_016 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_017, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_017 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    vector<string> files = { "test", "test2", "test3", "test4" };
    UniqueNumber uniqueNumber;
    const unordered_map<string, TimeInfo> timeInfoMap = operatorObj.GetTimeInfoMap(restoreTaskInfo);
    auto result = operatorObj.HandleCustomRestore(timeInfoMap, restoreTaskInfo, files, 2, uniqueNumber);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_017 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_018, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_018 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    FileInfo fileInfo;
    auto result = operatorObj.UpdatePhotoAlbum(restoreTaskInfo, fileInfo);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_018 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_019, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_019 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    FileInfo fileInfo;
    auto result = operatorObj.UpdatePhotoAlbum(restoreTaskInfo, fileInfo);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_019 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_021, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_021 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    InnerRestoreResult restoreResult = operatorObj.GenerateCustomRestoreNotify(restoreTaskInfo, 2);
    EXPECT_NE(restoreResult.cancelNum, 0);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_021 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_022, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_022 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    UniqueNumber uniqueNumber;
    operatorObj.SendNotifyMessage(restoreTaskInfo, 2, 2, 2, uniqueNumber);
    EXPECT_EQ(operatorObj.IsCancelTask(restoreTaskInfo), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_022 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_023, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_023 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    vector<FileInfo> files = {};
    UniqueNumber uniqueNumber;
    vector<FileInfo> result = operatorObj.SetDestinationPath(files, uniqueNumber);
    EXPECT_EQ(result.size(), 0);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_023 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_024, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_024 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    std::string result = "";
    operatorObj.GetAssetRootDir(1, result);
    EXPECT_NE(result, "");
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_024 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_025, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_025 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    std::string result = "";
    operatorObj.GetAssetRootDir(1, result);
    EXPECT_NE(result, "");
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_025 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_026, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_026 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    vector<FileInfo> files = {};
    int32_t code = 0;
    const unordered_map<string, TimeInfo> timeInfoMap = operatorObj.GetTimeInfoMap(restoreTaskInfo);
    vector<FileInfo> result = operatorObj.BatchInsert(timeInfoMap, restoreTaskInfo, files, code, true);
    EXPECT_EQ(result.size(), 0);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_026 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_027, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_027 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    operatorObj.QueryAlbumId(restoreTaskInfo);
    EXPECT_EQ(restoreTaskInfo.isDeduplication, false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_027 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_028, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_028 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    FileInfo fileInfo;
    bool result = operatorObj.IsDuplication(restoreTaskInfo, fileInfo);
    EXPECT_EQ(result, false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_028 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_029, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_029 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    int32_t result = operatorObj.InitPhotoCache(restoreTaskInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_029 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_030, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_030 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    int32_t result = operatorObj.InitPhotoCache(restoreTaskInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_030 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_031, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_031 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    UniqueNumber uniqueNumber;
    int32_t result = operatorObj.UpdateUniqueNumber(uniqueNumber);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_031 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_033, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_033 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    UniqueNumber uniqueNumber;
    vector<string> files = { "test", "test2", "test3", "test4" };
    vector<FileInfo> result = operatorObj.GetFileInfos(files, uniqueNumber);
    EXPECT_EQ(result.size(), 0);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_033 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_034, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_034 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    FileInfo fileInfo;
    const unordered_map<string, TimeInfo> timeInfoMap = operatorObj.GetTimeInfoMap(restoreTaskInfo);
    NativeRdb::ValuesBucket result = operatorObj.GetInsertValue(timeInfoMap, restoreTaskInfo, fileInfo);
    EXPECT_EQ(result.IsEmpty(), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_034 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_036, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_036 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    FileInfo fileInfo;
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    data->SetFilePath(fileInfo.originFilePath);
    data->SetFileName(fileInfo.fileName);
    data->SetFileMediaType(fileInfo.mediaType);
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    const unordered_map<string, TimeInfo> timeInfoMap = operatorObj.GetTimeInfoMap(restoreTaskInfo);
    int32_t result = operatorObj.FillMetadata(timeInfoMap, fileInfo, data);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_036 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_037, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_037 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    FileInfo fileInfo;
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    data->SetFilePath(fileInfo.originFilePath);
    data->SetFileName(fileInfo.fileName);
    data->SetFileMediaType(fileInfo.mediaType);
    int32_t result = operatorObj.GetFileMetadata(data);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_037 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_038, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_038 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    FileInfo fileInfo;
    vector<FileInfo> fileInfos;
    operatorObj.RenameFiles(fileInfos);
    fileInfos.push_back(fileInfo);
    operatorObj.RenameFiles(fileInfos);
    EXPECT_NE(fileInfos.size(), 0);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_038 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_039, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_039 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    operatorObj.CleanTimeoutCustomRestoreTaskDir();
    EXPECT_EQ(operatorObj.IsCancelTask(restoreTaskInfo), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_039 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Operation_Test_040, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_040 Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    vector<FileInfo> files = {};
    int32_t code = 0;
    const unordered_map<string, TimeInfo> timeInfoMap = operatorObj.GetTimeInfoMap(restoreTaskInfo);
    vector<FileInfo> result = operatorObj.BatchInsert(timeInfoMap, restoreTaskInfo, files, code, false);
    EXPECT_EQ(result.size(), 0);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Operation_Test_040 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_UpdatePhotoAlbum_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_UpdatePhotoAlbum_Test Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "restoreTaskInfo";
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();

    FileInfo fileInfo;
    auto ret = operatorObj.UpdatePhotoAlbum(restoreTaskInfo, fileInfo);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("Photo_Custom_Restore_UpdatePhotoAlbum_Test End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GenerateCustomRestoreNotify_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GenerateCustomRestoreNotify_Test Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.totalNum = 0;
    int type = 0;
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    InnerRestoreResult ret = operatorObj.GenerateCustomRestoreNotify(restoreTaskInfo, type);
    EXPECT_EQ(ret.successNum, 0);

    restoreTaskInfo.totalNum = 100;
    type = NOTIFY_LAST;
    ret = operatorObj.GenerateCustomRestoreNotify(restoreTaskInfo, type);
    EXPECT_EQ(ret.cancelNum, 0);

    type = NOTIFY_FIRST;
    ret = operatorObj.GenerateCustomRestoreNotify(restoreTaskInfo, type);
    EXPECT_EQ(ret.cancelNum, 0);

    type = NOTIFY_CANCEL;
    operatorObj.GenerateCustomRestoreNotify(restoreTaskInfo, type);
    EXPECT_EQ(ret.cancelNum, 0);
    MEDIA_INFO_LOG("Photo_Custom_Restore_GenerateCustomRestoreNotify_Test End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetAssetRootDir_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetAssetRootDir_Test Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    string dirPath;
    int32_t mediaType = MEDIA_TYPE_VIDEO;
    operatorObj.GetAssetRootDir(mediaType, dirPath);
    EXPECT_EQ(dirPath, "Photo/");

    mediaType = MEDIA_TYPE_DEFAULT;
    operatorObj.GetAssetRootDir(mediaType, dirPath);
    EXPECT_EQ(dirPath, "Document/");
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetAssetRootDir_Test End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_InitPhotoCache_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_InitPhotoCache_Test Start");
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.isDeduplication = false;
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    auto ret = operatorObj.InitPhotoCache(restoreTaskInfo);
    EXPECT_EQ(ret, E_OK);

    restoreTaskInfo.isDeduplication = true;
    restoreTaskInfo.albumId = 21;
    ret = operatorObj.InitPhotoCache(restoreTaskInfo);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_InitPhotoCache_Test End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetInsertValue_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetInsertValue_Test Start");
    RestoreTaskInfo restoreTaskInfo;
    FileInfo fileInfo;
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    EXPECT_EQ(PhotoCustomRestoreOperation::instance_ != nullptr, true);
    const unordered_map<string, TimeInfo> timeInfoMap = operatorObj.GetTimeInfoMap(restoreTaskInfo);
    auto values = operatorObj.GetInsertValue(timeInfoMap, restoreTaskInfo, fileInfo);
    EXPECT_NE(values.IsEmpty(), true);
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetInsertValue_Test End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HasTlvFiles_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HasTlvFiles_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    vector<string> emptyFiles;
    bool result = operatorObj.HasTlvFiles(emptyFiles);
    EXPECT_EQ(result, false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HasTlvFiles_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HasTlvFiles_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HasTlvFiles_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    vector<string> files = {"/storage/media/local/files/test.jpg", "/storage/media/local/files/test2.jpg"};
    bool result = operatorObj.HasTlvFiles(files);
    EXPECT_EQ(result, false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HasTlvFiles_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_MoveFile_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_MoveFile_Test_001 Start");
    string srcPath = "";
    string destPath = "/storage/media/local/files/Photo/1/test.jpg";
    int32_t result = PhotoCustomRestoreOperation::MoveFile(srcPath, destPath);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_MoveFile_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_MoveFile_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_MoveFile_Test_002 Start");
    string srcPath = "/storage/media/local/files/test.jpg";
    string destPath = "";
    int32_t result = PhotoCustomRestoreOperation::MoveFile(srcPath, destPath);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_MoveFile_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_DeleteDirectoryIfExists_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_DeleteDirectoryIfExists_Test_001 Start");
    string dirPath = "/storage/media/local/files/nonexistent_dir";
    bool result = PhotoCustomRestoreOperation::DeleteDirectoryIfExists(dirPath);
    EXPECT_EQ(result, true);
    MEDIA_INFO_LOG("Photo_Custom_Restore_DeleteDirectoryIfExists_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_ProcessExtractTlvFile_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_ProcessExtractTlvFile_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    string tlvFilePath = "";
    string destDir;
    unordered_map<TlvTag, string> extractedFiles;
    int32_t result = operatorObj.ProcessExtractTlvFile(tlvFilePath, destDir, extractedFiles);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_ProcessExtractTlvFile_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_ProcessExtractTlvFile_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_ProcessExtractTlvFile_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    string tlvFilePath = "/storage/media/local/files/nonexistent.tlv";
    string destDir;
    unordered_map<TlvTag, string> extractedFiles;
    int32_t result = operatorObj.ProcessExtractTlvFile(tlvFilePath, destDir, extractedFiles);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_ProcessExtractTlvFile_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandleTlvRestore_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleTlvRestore_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "test_key";
    vector<string> filePathVector;
    unordered_map<string, TimeInfo> timeInfoMap;
    UniqueNumber uniqueNumber;
    int32_t result = operatorObj.HandleTlvRestore(timeInfoMap, restoreTaskInfo, filePathVector, true, uniqueNumber);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleTlvRestore_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetUniqueTempDir_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetUniqueTempDir_Test_001 Start");
    string tlvPath = "/storage/media/local/files/test.tlv";
    string result = PhotoCustomRestoreOperation::GetUniqueTempDir(tlvPath);
    EXPECT_EQ(result.empty(), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetUniqueTempDir_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandlePhotoSourceRestore_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandlePhotoSourceRestore_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    string sourceBackSrcPath = "/storage/media/local/files/source.jpg";
    string assetPath = "/storage/media/local/files/Photo/1/asset.jpg";
    int32_t result = operatorObj.HandlePhotoSourceRestore(sourceBackSrcPath, assetPath);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandlePhotoSourceRestore_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandlePhotoSourceBackRestore_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandlePhotoSourceBackRestore_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    string sourceBackSrcPath = "/storage/media/local/files/source_back.jpg";
    string assetPath = "/storage/media/local/files/Photo/1/asset.jpg";
    int32_t result = operatorObj.HandlePhotoSourceBackRestore(sourceBackSrcPath, assetPath);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandlePhotoSourceBackRestore_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandleEditDataRestore_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleEditDataRestore_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    string editDataSrcPath = "/storage/media/local/files/edit_data.jpg";
    string assetPath = "/storage/media/local/files/Photo/1/asset.jpg";
    int32_t result = operatorObj.HandleEditDataRestore(editDataSrcPath, assetPath);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleEditDataRestore_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandleEditDataCameraRestore_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleEditDataCameraRestore_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    string editDataCameraSrcPath = "/storage/media/local/files/edit_camera.jpg";
    string assetPath = "/storage/media/local/files/Photo/1/asset.jpg";
    int32_t result = operatorObj.HandleEditDataCameraRestore(editDataCameraSrcPath, assetPath);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleEditDataCameraRestore_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandleExtraDataRestore_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleExtraDataRestore_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    string editDataCameraSrcPath = "/storage/media/local/files/extra_data.jpg";
    string assetPath = "/storage/media/local/files/Photo/1/asset.jpg";
    int32_t result = operatorObj.HandleExtraDataRestore(editDataCameraSrcPath, assetPath);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleExtraDataRestore_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_CheckNeedProcessMovingPhotoSize_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_CheckNeedProcessMovingPhotoSize_Test_002 Start");
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::DEFAULT));
    values.PutInt(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, 0);
    bool result = PhotoCustomRestoreOperation::CheckNeedProcessMovingPhotoSize(values);
    EXPECT_EQ(result, false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_CheckNeedProcessMovingPhotoSize_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandleDbFieldsFromJsonRestore_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleDbFieldsFromJsonRestore_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    string jsonPath = "/storage/media/local/files/nonexistent.json";
    string assetPath = "/storage/media/local/files/Photo/1/asset.jpg";
    int32_t result = operatorObj.HandleDbFieldsFromJsonRestore(jsonPath, assetPath);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleDbFieldsFromJsonRestore_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandleAllEditData_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleAllEditData_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    unordered_map<TlvTag, string> decodeTlvPathMap;
    string assetPath = "/storage/media/local/files/Photo/1/asset.jpg";
    int32_t result = operatorObj.HandleAllEditData(decodeTlvPathMap, assetPath);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleAllEditData_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandleAllEditData_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleAllEditData_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    unordered_map<TlvTag, string> decodeTlvPathMap;
    decodeTlvPathMap[TlvTag::TLV_TAG_ORIGIN] = "/storage/media/local/files/origin.jpg";
    string assetPath = "/storage/media/local/local/files/Photo/1/asset.jpg";
    int32_t result = operatorObj.HandleAllEditData(decodeTlvPathMap, assetPath);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleAllEditData_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandleTlvSingleRestore_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleTlvSingleRestore_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "test_key";
    restoreTaskInfo.sourceDir = "/storage/media/local/files/test";
    unordered_map<TlvTag, string> editFileMap;
    unordered_map<string, TimeInfo> timeInfoMap;
    UniqueNumber uniqueNumber;
    int32_t result = operatorObj.HandleTlvSingleRestore(editFileMap, timeInfoMap, restoreTaskInfo, true, uniqueNumber);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleTlvSingleRestore_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandleTlvSingleRestore_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleTlvSingleRestore_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "test_key";
    restoreTaskInfo.sourceDir = "/storage/media/local/files/test";
    unordered_map<TlvTag, string> editFileMap;
    editFileMap[TlvTag::TLV_TAG_ORIGIN] = "/storage/media/local/files/origin.jpg";
    editFileMap[TlvTag::TLV_TAG_JSON] = "/storage/media/local/files/edit.json";
    unordered_map<string, TimeInfo> timeInfoMap;
    UniqueNumber uniqueNumber;
    int32_t result =
        operatorObj.HandleTlvSingleRestore(editFileMap, timeInfoMap, restoreTaskInfo, false, uniqueNumber);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleTlvSingleRestore_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_RestoreTlvRollback_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_RestoreTlvRollback_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    string assetPath = "/storage/media/local/files/Photo/1/asset.jpg";
    operatorObj.RestoreTlvRollback(assetPath);
    MEDIA_INFO_LOG("Photo_Custom_Restore_RestoreTlvRollback_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_UpdateTlvEditDataSize_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_UpdateTlvEditDataSize_Test_001 Start");
    string assetPath = "/storage/media/local/files/Photo/1/asset.jpg";
    int32_t result = PhotoCustomRestoreOperation::UpdateTlvEditDataSize(assetPath);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_UpdateTlvEditDataSize_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_MoveLivePhoto_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_MoveLivePhoto_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    string originFilePath = "/storage/media/local/files/live_photo.jpg";
    string filePath = "/storage/media/local/files/Photo/1/asset.jpg";
    int32_t result = operatorObj.MoveLivePhoto(originFilePath, filePath);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_MoveLivePhoto_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_DeleteDatabaseRecord_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_DeleteDatabaseRecord_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    string filePath = "/storage/media/local/files/Photo/1/asset.jpg";
    operatorObj.DeleteDatabaseRecord(filePath);
    MEDIA_INFO_LOG("Photo_Custom_Restore_DeleteDatabaseRecord_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetAlbumInfoBySubType_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetAlbumInfoBySubType_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    int32_t subType = PhotoAlbumSubType::IMAGE;
    string albumUri;
    int32_t albumId;
    int32_t result = operatorObj.GetAlbumInfoBySubType(subType, albumUri, albumId);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetAlbumInfoBySubType_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetAlbumInfoBySubType_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetAlbumInfoBySubType_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    int32_t subType = PhotoAlbumSubType::VIDEO;
    string albumUri;
    int32_t albumId;
    int32_t result = operatorObj.GetAlbumInfoBySubType(subType, albumUri, albumId);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetAlbumInfoBySubType_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_QueryMediaInfo_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_QueryMediaInfo_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    shared_ptr<NativeRdb::RdbStore> nullStore = nullptr;
    unordered_map<string, TimeInfo> result = operatorObj.QueryMediaInfo(nullStore);
    EXPECT_EQ(result.size(), 0);
    MEDIA_INFO_LOG("Photo_Custom_Restore_QueryMediaInfo_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetTimeInfoMap_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetTimeInfoMap_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.dbPath = "";
    unordered_map<string, TimeInfo> result = operatorObj.GetTimeInfoMap(restoreTaskInfo);
    EXPECT_EQ(result.size(), 0);
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetTimeInfoMap_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetTimeInfoMap_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetTimeInfoMap_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.dbPath = "nonexistent.db";
    restoreTaskInfo.bundleName = "test.bundle";
    unordered_map<string, TimeInfo> result = operatorObj.GetTimeInfoMap(restoreTaskInfo);
    EXPECT_EQ(result.size(), 0);
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetTimeInfoMap_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_IsDuplication_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_IsDuplication_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.isDeduplication = false;
    restoreTaskInfo.albumId = 0;
    FileInfo fileInfo;
    fileInfo.fileName = "test.jpg";
    fileInfo.size = 1024;
    fileInfo.mediaType = MediaType::MEDIA_TYPE_IMAGE;
    fileInfo.orientation = 0;
    bool result = operatorObj.IsDuplication(restoreTaskInfo, fileInfo);
    EXPECT_EQ(result, false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_IsDuplication_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_IsDuplication_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_IsDuplication_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.isDeduplication = true;
    restoreTaskInfo.albumId = 1;
    restoreTaskInfo.hasPhotoCache = true;
    FileInfo fileInfo;
    fileInfo.fileName = "test.jpg";
    fileInfo.size = 1024;
    fileInfo.mediaType = MediaType::MEDIA_TYPE_IMAGE;
    fileInfo.orientation = 0;
    operatorObj.photoCache_.insert("test.jpg_1024_1_0");
    bool result = operatorObj.IsDuplication(restoreTaskInfo, fileInfo);
    EXPECT_EQ(result, true);
    MEDIA_INFO_LOG("Photo_Custom_Restore_IsDuplication_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_IsDuplication_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_IsDuplication_Test_003 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.isDeduplication = true;
    restoreTaskInfo.albumId = 1;
    restoreTaskInfo.hasPhotoCache = true;
    FileInfo fileInfo;
    fileInfo.fileName = "test2.jpg";
    fileInfo.size = 2048;
    fileInfo.mediaType = MediaType::MEDIA_TYPE_IMAGE;
    fileInfo.orientation = 0;
    operatorObj.photoCache_.insert("test.jpg_1024_1_0");
    bool result = operatorObj.IsDuplication(restoreTaskInfo, fileInfo);
    EXPECT_EQ(result, false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_IsDuplication_Test_003 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_QueryAlbumId_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_QueryAlbumId_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.bundleName = "test.bundle";
    restoreTaskInfo.packageName = "test.package";
    restoreTaskInfo.isDeduplication = true;
    operatorObj.QueryAlbumId(restoreTaskInfo);
    EXPECT_EQ(restoreTaskInfo.isDeduplication, false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_QueryAlbumId_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetFileInfos_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetFileInfos_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    vector<string> filePathVector;
    UniqueNumber uniqueNumber;
    vector<FileInfo> result = operatorObj.GetFileInfos(filePathVector, uniqueNumber);
    EXPECT_EQ(result.size(), 0);
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetFileInfos_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetFileInfos_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetFileInfos_Test_003 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    vector<string> filePathVector = {"/storage/media/local/files/test.txt"};
    UniqueNumber uniqueNumber;
    vector<FileInfo> result = operatorObj.GetFileInfos(filePathVector, uniqueNumber);
    EXPECT_EQ(result.size(), 0);
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetFileInfos_Test_003 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_SetDestinationPath_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_SetDestinationPath_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    vector<FileInfo> restoreFiles;
    UniqueNumber uniqueNumber;
    vector<FileInfo> result = operatorObj.SetDestinationPath(restoreFiles, uniqueNumber);
    EXPECT_EQ(result.size(), 0);
    MEDIA_INFO_LOG("Photo_Custom_Restore_SetDestinationPath_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_RenameFiles_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_RenameFiles_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    vector<FileInfo> restoreFiles;
    int32_t result = operatorObj.RenameFiles(restoreFiles);
    EXPECT_EQ(result, 0);
    MEDIA_INFO_LOG("Photo_Custom_Restore_RenameFiles_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_RenameFiles_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_RenameFiles_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    vector<FileInfo> restoreFiles;
    FileInfo fileInfo;
    fileInfo.originFilePath = "/storage/media/local/files/test.jpg";
    fileInfo.filePath = "/storage/media/local/files/Photo/1/test.jpg";
    fileInfo.isLivePhoto = false;
    restoreFiles.push_back(fileInfo);
    int32_t result = operatorObj.RenameFiles(restoreFiles);
    EXPECT_EQ(result, 0);
    MEDIA_INFO_LOG("Photo_Custom_Restore_RenameFiles_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_RenameFiles_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_RenameFiles_Test_003 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    vector<FileInfo> restoreFiles;
    FileInfo fileInfo;
    fileInfo.originFilePath = "/storage/media/local/files/live_photo.jpg";
    fileInfo.filePath = "/storage/media/local/files/Photo/1/live_photo.jpg";
    fileInfo.isLivePhoto = true;
    restoreFiles.push_back(fileInfo);
    int32_t result = operatorObj.RenameFiles(restoreFiles);
    EXPECT_EQ(result, 0);
    MEDIA_INFO_LOG("Photo_Custom_Restore_RenameFiles_Test_003 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_BatchInsert_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_BatchInsert_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.bundleName = "test.bundle";
    restoreTaskInfo.packageName = "test.package";
    restoreTaskInfo.appId = "test.app";
    vector<FileInfo> restoreFiles;
    int32_t sameFileNum = 0;
    unordered_map<string, TimeInfo> timeInfoMap;
    vector<FileInfo> result = operatorObj.BatchInsert(timeInfoMap, restoreTaskInfo, restoreFiles, sameFileNum, true);
    EXPECT_EQ(result.size(), 0);
    MEDIA_INFO_LOG("Photo_Custom_Restore_BatchInsert_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_BatchInsert_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_BatchInsert_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.bundleName = "test.bundle";
    restoreTaskInfo.packageName = "test.package";
    restoreTaskInfo.appId = "test.app";
    restoreTaskInfo.isDeduplication = true;
    restoreTaskInfo.albumId = 1;
    restoreTaskInfo.hasPhotoCache = true;
    vector<FileInfo> restoreFiles;
    FileInfo fileInfo;
    fileInfo.fileName = "test.jpg";
    fileInfo.displayName = "test.jpg";
    fileInfo.originFilePath = "/storage/media/local/files/test.jpg";
    fileInfo.filePath = "/storage/media/local/files/Photo/1/test.jpg";
    fileInfo.mediaType = MediaType::MEDIA_TYPE_IMAGE;
    fileInfo.size = 1024;
    fileInfo.orientation = 0;
    restoreFiles.push_back(fileInfo);
    operatorObj.photoCache_.insert("test.jpg_1024_1_0");
    int32_t sameFileNum = 0;
    unordered_map<string, TimeInfo> timeInfoMap;
    vector<FileInfo> result = operatorObj.BatchInsert(timeInfoMap, restoreTaskInfo, restoreFiles, sameFileNum, false);
    EXPECT_EQ(result.size(), 0);
    MEDIA_INFO_LOG("Photo_Custom_Restore_BatchInsert_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandleCustomRestore_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleCustomRestore_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "test_key";
    restoreTaskInfo.bundleName = "test.bundle";
    restoreTaskInfo.packageName = "test.package";
    restoreTaskInfo.appId = "test.app";
    restoreTaskInfo.sourceDir = "/storage/media/local/files/test";
    vector<string> filePathVector;
    unordered_map<string, TimeInfo> timeInfoMap;
    UniqueNumber uniqueNumber;
    int32_t result = operatorObj.HandleCustomRestore(timeInfoMap, restoreTaskInfo, filePathVector, true, uniqueNumber);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleCustomRestore_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandleCustomRestore_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleCustomRestore_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "test_key";
    restoreTaskInfo.bundleName = "test.bundle";
    restoreTaskInfo.packageName = "test.package";
    restoreTaskInfo.appId = "test.app";
    restoreTaskInfo.sourceDir = "/storage/media/local/files/test";
    vector<string> filePathVector = {"/storage/media/local/files/test.jpg"};
    unordered_map<string, TimeInfo> timeInfoMap;
    UniqueNumber uniqueNumber;
    int32_t result =
        operatorObj.HandleCustomRestore(timeInfoMap, restoreTaskInfo, filePathVector, false, uniqueNumber);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleCustomRestore_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandleFirstRestoreFile_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleFirstRestoreFile_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "test_key";
    restoreTaskInfo.bundleName = "test.bundle";
    restoreTaskInfo.packageName = "test.package";
    restoreTaskInfo.appId = "test.app";
    restoreTaskInfo.sourceDir = "/storage/media/local/files/test";
    vector<string> files = {"/storage/media/local/files/test.jpg"};
    unordered_map<string, TimeInfo> timeInfoMap;
    int32_t firstRestoreIndex = 0;
    bool result = operatorObj.HandleFirstRestoreFile(timeInfoMap, restoreTaskInfo, files, 0, firstRestoreIndex);
    EXPECT_EQ(result, false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleFirstRestoreFile_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GenerateCustomRestoreNotify_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GenerateCustomRestoreNotify_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.totalNum = 100;
    restoreTaskInfo.uriType = RESTORE_URI_TYPE_PHOTO;
    restoreTaskInfo.uri = "file://media/photo/1";
    operatorObj.successNum_.store(80);
    operatorObj.failNum_.store(10);
    operatorObj.sameNum_.store(10);
    InnerRestoreResult result = operatorObj.GenerateCustomRestoreNotify(restoreTaskInfo, NOTIFY_PROGRESS);
    EXPECT_EQ(result.stage, "onRestore");
    EXPECT_EQ(result.successNum, 80);
    EXPECT_EQ(result.failedNum, 10);
    EXPECT_EQ(result.sameNum, 10);
    MEDIA_INFO_LOG("Photo_Custom_Restore_GenerateCustomRestoreNotify_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GenerateCustomRestoreNotify_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GenerateCustomRestoreNotify_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.totalNum = 100;
    restoreTaskInfo.uriType = RESTORE_URI_TYPE_PHOTO;
    restoreTaskInfo.uri = "file://media/photo/1";
    operatorObj.successNum_.store(80);
    operatorObj.failNum_.store(10);
    operatorObj.sameNum_.store(10);
    InnerRestoreResult result = operatorObj.GenerateCustomRestoreNotify(restoreTaskInfo, NOTIFY_LAST);
    EXPECT_EQ(result.stage, "finished");
    EXPECT_EQ(result.successNum, 80);
    EXPECT_EQ(result.failedNum, 10);
    EXPECT_EQ(result.sameNum, 10);
    MEDIA_INFO_LOG("Photo_Custom_Restore_GenerateCustomRestoreNotify_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GenerateCustomRestoreNotify_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GenerateCustomRestoreNotify_Test_003 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.totalNum = 100;
    restoreTaskInfo.uriType = RESTORE_URI_TYPE_PHOTO;
    restoreTaskInfo.uri = "file://media/photo/1";
    operatorObj.successNum_.store(50);
    operatorObj.failNum_.store(20);
    operatorObj.sameNum_.store(10);
    InnerRestoreResult result = operatorObj.GenerateCustomRestoreNotify(restoreTaskInfo, NOTIFY_CANCEL);
    EXPECT_EQ(result.stage, "finished");
    EXPECT_EQ(result.successNum, 50);
    EXPECT_EQ(result.failedNum, 20);
    EXPECT_EQ(result.sameNum, 10);
    EXPECT_EQ(result.cancelNum, 20);
    MEDIA_INFO_LOG("Photo_Custom_Restore_GenerateCustomRestoreNotify_Test_003 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_UpdatePhotoAlbum_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_UpdatePhotoAlbum_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.bundleName = "test.bundle";
    restoreTaskInfo.packageName = "test.package";
    FileInfo fileInfo;
    fileInfo.fileName = "test.jpg";
    fileInfo.displayName = "test.jpg";
    fileInfo.filePath = "/storage/media/local/files/Photo/1/test.jpg";
    fileInfo.mediaType = MediaType::MEDIA_TYPE_IMAGE;
    int32_t result = operatorObj.UpdatePhotoAlbum(restoreTaskInfo, fileInfo);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_UpdatePhotoAlbum_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_UpdateUniqueNumber_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_UpdateUniqueNumber_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    UniqueNumber uniqueNumber;
    uniqueNumber.imageTotalNumber = 10;
    uniqueNumber.videoTotalNumber = 5;
    int32_t result = operatorObj.UpdateUniqueNumber(uniqueNumber);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_UpdateUniqueNumber_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetInsertValue_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetInsertValue_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.bundleName = "test.bundle";
    restoreTaskInfo.packageName = "test.package";
    restoreTaskInfo.appId = "test.app";
    FileInfo fileInfo;
    fileInfo.fileName = "test.jpg";
    fileInfo.displayName = "test.jpg";
    fileInfo.originFilePath = "/storage/media/local/files/test.jpg";
    fileInfo.filePath = "/storage/media/local/files/Photo/1/test.jpg";
    fileInfo.mediaType = MediaType::MEDIA_TYPE_IMAGE;
    fileInfo.isLivePhoto = false;
    unordered_map<string, TimeInfo> timeInfoMap;
    NativeRdb::ValuesBucket result = operatorObj.GetInsertValue(timeInfoMap, restoreTaskInfo, fileInfo);
    EXPECT_EQ(result.IsEmpty(), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetInsertValue_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetInsertValue_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetInsertValue_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.bundleName = "test.bundle";
    restoreTaskInfo.packageName = "test.package";
    restoreTaskInfo.appId = "test.app";
    FileInfo fileInfo;
    fileInfo.fileName = "test.mp4";
    fileInfo.displayName = "test.mp4";
    fileInfo.originFilePath = "/storage/media/local/files/test.mp4";
    fileInfo.filePath = "/storage/media/local/files/Photo/1/test.mp4";
    fileInfo.mediaType = MediaType::MEDIA_TYPE_VIDEO;
    fileInfo.isLivePhoto = false;
    unordered_map<string, TimeInfo> timeInfoMap;
    NativeRdb::ValuesBucket result = operatorObj.GetInsertValue(timeInfoMap, restoreTaskInfo, fileInfo);
    EXPECT_EQ(result.IsEmpty(), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetInsertValue_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetInsertValue_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetInsertValue_Test_003 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.bundleName = "test.bundle";
    restoreTaskInfo.packageName = "test.package";
    restoreTaskInfo.appId = "test.app";
    FileInfo fileInfo;
    fileInfo.fileName = "live_photo.jpg";
    fileInfo.displayName = "live_photo.jpg";
    fileInfo.originFilePath = "/storage/media/local/files/live_photo.jpg";
    fileInfo.filePath = "/storage/media/local/files/Photo/1/live_photo.jpg";
    fileInfo.mediaType = MediaType::MEDIA_TYPE_IMAGE;
    fileInfo.isLivePhoto = true;
    unordered_map<string, TimeInfo> timeInfoMap;
    NativeRdb::ValuesBucket result = operatorObj.GetInsertValue(timeInfoMap, restoreTaskInfo, fileInfo);
    EXPECT_EQ(result.IsEmpty(), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetInsertValue_Test_003 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_FillMetadata_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_FillMetadata_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    FileInfo fileInfo;
    fileInfo.fileName = "test.jpg";
    fileInfo.originFilePath = "/storage/media/local/files/test.jpg";
    fileInfo.mediaType = MediaType::MEDIA_TYPE_IMAGE;
    unique_ptr<Metadata> data = make_unique<Metadata>();
    unordered_map<string, TimeInfo> timeInfoMap;
    int32_t result = operatorObj.FillMetadata(timeInfoMap, fileInfo, data);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_FillMetadata_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_FillMetadata_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_FillMetadata_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    FileInfo fileInfo;
    fileInfo.fileName = "test.jpg";
    fileInfo.originFilePath = "/storage/media/local/files/test/test.jpg";
    fileInfo.mediaType = MediaType::MEDIA_TYPE_IMAGE;
    unique_ptr<Metadata> data = make_unique<Metadata>();
    unordered_map<string, TimeInfo> timeInfoMap;
    timeInfoMap["test.jpg"] = {MediaFileUtils::UTCTimeMilliSeconds(),
                                MediaFileUtils::UTCTimeMilliSeconds(),
                                "2024-01-01 12:00:00"};
    int32_t result = operatorObj.FillMetadata(timeInfoMap, fileInfo, data);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_FillMetadata_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetFileMetadata_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetFileMetadata_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    unique_ptr<Metadata> data = make_unique<Metadata>();
    data->SetFilePath("/storage/media/local/files/nonexistent.jpg");
    data->SetFileName("nonexistent.jpg");
    int32_t result = operatorObj.GetFileMetadata(data);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetFileMetadata_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_InitPhotoCache_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_InitPhotoCache_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.isDeduplication = false;
    restoreTaskInfo.albumId = 0;
    int32_t result = operatorObj.InitPhotoCache(restoreTaskInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_InitPhotoCache_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_InitPhotoCache_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_InitPhotoCache_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.isDeduplication = true;
    restoreTaskInfo.albumId = 0;
    int32_t result = operatorObj.InitPhotoCache(restoreTaskInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_InitPhotoCache_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_InitPhotoCache_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_InitPhotoCache_Test_003 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.isDeduplication = true;
    restoreTaskInfo.albumId = 1;
    int32_t result = operatorObj.InitPhotoCache(restoreTaskInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_InitPhotoCache_Test_003 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetAssetRootDir_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetAssetRootDir_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    string rootDirPath;
    operatorObj.GetAssetRootDir(MEDIA_TYPE_IMAGE, rootDirPath);
    EXPECT_EQ(rootDirPath, "Photo/");
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetAssetRootDir_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetAssetRootDir_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetAssetRootDir_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    string rootDirPath;
    operatorObj.GetAssetRootDir(MEDIA_TYPE_VIDEO, rootDirPath);
    EXPECT_EQ(rootDirPath, "Photo/");
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetAssetRootDir_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetAssetRootDir_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetAssetRootDir_Test_003 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    string rootDirPath;
    operatorObj.GetAssetRootDir(MEDIA_TYPE_AUDIO, rootDirPath);
    EXPECT_EQ(rootDirPath, "Audio/");
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetAssetRootDir_Test_003 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetAssetRootDir_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetAssetRootDir_Test_004 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    string rootDirPath;
    operatorObj.GetAssetRootDir(MEDIA_TYPE_FILE, rootDirPath);
    EXPECT_EQ(rootDirPath, "Document/");
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetAssetRootDir_Test_004 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetAssetRootDir_Test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetAssetRootDir_Test_005 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    string rootDirPath;
    operatorObj.GetAssetRootDir(999, rootDirPath);
    EXPECT_EQ(rootDirPath, "Document/");
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetAssetRootDir_Test_005 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_AddTask_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_AddTask_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "test_key";
    restoreTaskInfo.sourceDir = "/storage/media/local/files/test";
    PhotoCustomRestoreOperation &result = operatorObj.AddTask(restoreTaskInfo);
    EXPECT_EQ(&result, &operatorObj);
    MEDIA_INFO_LOG("Photo_Custom_Restore_AddTask_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Start_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Start_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    operatorObj.isRunning_.store(false);
    PhotoCustomRestoreOperation &result = operatorObj.Start();
    EXPECT_EQ(&result, &operatorObj);
    EXPECT_EQ(operatorObj.isRunning_.load(), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Start_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Start_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Start_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    operatorObj.isRunning_.store(true);
    PhotoCustomRestoreOperation &result = operatorObj.Start();
    EXPECT_EQ(&result, &operatorObj);
    EXPECT_EQ(operatorObj.isRunning_.load(), true);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Start_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_CancelTask_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_CancelTask_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "test_key";
    operatorObj.CancelTask(restoreTaskInfo);
    EXPECT_EQ(operatorObj.IsCancelTask(restoreTaskInfo), true);
    MEDIA_INFO_LOG("Photo_Custom_Restore_CancelTask_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_CancelTaskFinish_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_CancelTaskFinish_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "test_key";
    operatorObj.cancelKeySet_.insert("test_key");
    operatorObj.CancelTaskFinish(restoreTaskInfo);
    EXPECT_EQ(operatorObj.IsCancelTask(restoreTaskInfo), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_CancelTaskFinish_Test_001 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandleTlvSingleRestore_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleTlvSingleRestore_Test_003 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "test_key";
    restoreTaskInfo.sourceDir = "/storage/media/local/files/test";
    restoreTaskInfo.bundleName = "test.bundle";
    restoreTaskInfo.packageName = "test.package";
    restoreTaskInfo.appId = "test.app";
    unordered_map<TlvTag, string> editFileMap;
    editFileMap[TlvTag::TLV_TAG_ORIGIN] = "/storage/media/local/files/test/origin.jpg";
    editFileMap[TlvTag::TLV_TAG_JSON] = "/storage/media/local/files/test/edit.json";
    editFileMap[TlvTag::TLV_TAG_EDITDATA] = "/storage/media/local/files/test/edit_data.jpg";
    unordered_map<string, TimeInfo> timeInfoMap;
    UniqueNumber uniqueNumber;
    int32_t result = operatorObj.HandleTlvSingleRestore(editFileMap, timeInfoMap, restoreTaskInfo, true, uniqueNumber);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleTlvSingleRestore_Test_003 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandleAllEditData_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleAllEditData_Test_003 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    unordered_map<TlvTag, string> decodeTlvPathMap;
    decodeTlvPathMap[TlvTag::TLV_TAG_ORIGIN] = "/storage/media/local/files/origin.jpg";
    decodeTlvPathMap[TlvTag::TLV_TAG_JSON] = "/storage/media/local/files/edit.json";
    decodeTlvPathMap[TlvTag::TLV_TAG_EDITDATA] = "/storage/media/local/files/edit_data.jpg";
    decodeTlvPathMap[TlvTag::TLV_TAG_CAMERA] = "/storage/media/local/files/camera.jpg";
    decodeTlvPathMap[TlvTag::TLV_TAG_SOURCE] = "/storage/media/local/files/source.jpg";
    decodeTlvPathMap[TlvTag::TLV_TAG_SOURCE_BACK] = "/storage/media/local/files/source_back.jpg";
    string assetPath = "/storage/media/local/files/Photo/1/asset.jpg";
    int32_t result = operatorObj.HandleAllEditData(decodeTlvPathMap, assetPath);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleAllEditData_Test_003 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandleAllEditData_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleAllEditData_Test_004 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    unordered_map<TlvTag, string> decodeTlvPathMap;
    decodeTlvPathMap[TlvTag::TLV_TAG_ORIGIN] = "/storage/media/local/files/origin.jpg";
    decodeTlvPathMap[TlvTag::TLV_TAG_JSON] = "/storage/media/local/files/edit.json";
    decodeTlvPathMap[TlvTag::TLV_TAG_MOVING_PHOTO_VIDEO] = "/storage/media/local/files/video.mp4";
    decodeTlvPathMap[TlvTag::TLV_TAG_MOVING_PHOTO_VIDEO_SOURCE] = "/storage/media/local/files/video_source.mp4";
    decodeTlvPathMap[TlvTag::TLV_TAG_MOVING_PHOTO_VIDEO_SOURCE_BACK] =
        "/storage/media/local/files/video_source_back.mp4";
    decodeTlvPathMap[TlvTag::TLV_TAG_EXTRA_DATA] = "/storage/media/local/files/extra_data.jpg";
    string assetPath = "/storage/media/local/files/Photo/1/asset.jpg";
    int32_t result = operatorObj.HandleAllEditData(decodeTlvPathMap, assetPath);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleAllEditData_Test_004 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandleFirstRestoreFile_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleFirstRestoreFile_Test_003 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "test_key";
    restoreTaskInfo.bundleName = "test.bundle";
    restoreTaskInfo.packageName = "test.package";
    restoreTaskInfo.appId = "test.app";
    restoreTaskInfo.sourceDir = "/storage/media/local/files/test";
    vector<string> files = {"/storage/media/local/files/test.jpg", "/storage/media/local/files/test2.jpg"};
    unordered_map<string, TimeInfo> timeInfoMap;
    int32_t firstRestoreIndex = 0;
    bool result = operatorObj.HandleFirstRestoreFile(timeInfoMap, restoreTaskInfo, files, 1, firstRestoreIndex);
    EXPECT_EQ(result, false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleFirstRestoreFile_Test_003 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_MoveFile_Testing_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_MoveFile_Test_003 Start");
    string srcPath = "/storage/media/local/files/test.jpg";
    string destPath = "/storage/media/local/files/Photo/1/test.jpg";
    int32_t result = PhotoCustomRestoreOperation::MoveFile(srcPath, destPath);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_MoveFile_Test_003 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_MoveFile_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_MoveFile_Test_004 Start");
    string srcPath = "/storage/media/local/files/test.mp4";
    string destPath = "/storage/media/local/files/Photo/1/test.mp4";
    int32_t result = PhotoCustomRestoreOperation::MoveFile(srcPath, destPath);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_MoveFile_Test_004 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_DeleteDirectoryIfExists_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_DeleteDirectoryIfExists_Test_002 Start");
    string dirPath = "/storage/media/local/files/test_dir";
    bool result = PhotoCustomRestoreOperation::DeleteDirectoryIfExists(dirPath);
    EXPECT_EQ(result, true);
    MEDIA_INFO_LOG("Photo_Custom_Restore_DeleteDirectoryIfExists_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetUniqueTempDir_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetUniqueTempDir_Test_002 Start");
    string tlvPath = "/storage/media/local/files/test2.tlv";
    string result = PhotoCustomRestoreOperation::GetUniqueTempDir(tlvPath);
    EXPECT_EQ(result.empty(), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetUniqueTempDir_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetUniqueTempDir_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetUniqueTempDir_Test_003 Start");
    string tlvPath = "/storage/media/local/files/subdir/test.tlv";
    string result = PhotoCustomRestoreOperation::GetUniqueTempDir(tlvPath);
    EXPECT_EQ(result.empty(), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetUniqueTempDir_Test_003 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_CheckNeedProcessMovingPhotoSize_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_CheckNeedProcessMovingPhotoSize_Test_004 Start");
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::DEFAULT));
    values.PutInt(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY));
    bool result = PhotoCustomRestoreOperation::CheckNeedProcessMovingPhotoSize(values);
    EXPECT_EQ(result, true);
    MEDIA_INFO_LOG("Photo_Custom_Restore_CheckNeedProcessMovingPhotoSize_Test_004 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_CheckNeedProcessMovingPhotoSize_Test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_CheckNeedProcessMovingPhotoSize_Test_005 Start");
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::SPATIAL_3DGS));
    values.PutInt(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, 0);
    bool result = PhotoCustomRestoreOperation::CheckNeedProcessMovingPhotoSize(values);
    EXPECT_EQ(result, false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_CheckNeedProcessMovingPhotoSize_Test_005 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandleDbFieldsFromJsonRestore_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleDbFieldsFromJsonRestore_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    string jsonPath = "/storage/media/local/files/test/edit.json";
    string assetPath = "/storage/media/local/files/Photo/1/asset.jpg";
    int32_t result = operatorObj.HandleDbFieldsFromJsonRestore(jsonPath, assetPath);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleDbFieldsFromJsonRestore_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_UpdateTlvEditDataSize_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_UpdateTlvEditDataSize_Test_003 Start");
    string assetPath = "/storage/media/local/files/Photo/3/asset.mp4";
    int32_t result = PhotoCustomRestoreOperation::UpdateTlvEditDataSize(assetPath);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_UpdateTlvEditDataSize_Test_003 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_MoveLivePhoto_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_MoveLivePhoto_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    string originFilePath = "/storage/media/local/files/live_photo2.jpg";
    string filePath = "/storage/media/local/files/Photo/2/asset.jpg";
    int32_t result = operatorObj.MoveLivePhoto(originFilePath, filePath);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_MoveLivePhoto_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_MoveLivePhoto_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_MoveLivePhoto_Test_003 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    string originFilePath = "/storage/media/local/files/live_photo3.jpg";
    string filePath = "/storage/media/local/files/Photo/3/asset.jpg";
    int32_t result = operatorObj.MoveLivePhoto(originFilePath, filePath);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_MoveLivePhoto_Test_003 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetAlbumInfoBySubType_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetAlbumInfoBySubType_Test_003 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    int32_t subType = PhotoAlbumSubType::FAVORITE;
    string albumUri;
    int32_t albumId;
    int32_t result = operatorObj.GetAlbumInfoBySubType(subType, albumUri, albumId);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetAlbumInfoBySubType_Test_003 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetAlbumInfoBySubType_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetAlbumInfoBySubType_Test_004 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    int32_t subType = PhotoAlbumSubType::TRASH;
    string albumUri;
    int32_t albumId;
    int32_t result = operatorObj.GetAlbumInfoBySubType(subType, albumUri, albumId);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetAlbumInfoBySubType_Test_004 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetTimeInfoMap_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetTimeInfoMap_Test_003 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.dbPath = "test.db";
    restoreTaskInfo.bundleName = "test.bundle";
    restoreTaskInfo.packageName = "test.package";
    unordered_map<string, TimeInfo> result = operatorObj.GetTimeInfoMap(restoreTaskInfo);
    EXPECT_EQ(result.size(), 0);
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetTimeInfoMap_Test_003 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetTimeInfoMap_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetTimeInfoMap_Test_004 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.dbPath = "subdir/test.db";
    restoreTaskInfo.bundleName = "test.bundle";
    restoreTaskInfo.packageName = "test.package";
    unordered_map<string, TimeInfo> result = operatorObj.GetTimeInfoMap(restoreTaskInfo);
    EXPECT_EQ(result.size(), 0);
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetTimeInfoMap_Test_004 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandleCustomRestore_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleCustomRestore_Test_003 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "test_key";
    restoreTaskInfo.bundleName = "test.bundle";
    restoreTaskInfo.packageName = "test.package";
    restoreTaskInfo.appId = "test.app";
    restoreTaskInfo.sourceDir = "/storage/media/local/files/test";
    vector<string> filePathVector = {"/storage/media/local/files/test.jpg", "/storage/media/local/files/test2.jpg"};
    unordered_map<string, TimeInfo> timeInfoMap;
    UniqueNumber uniqueNumber;
    int32_t result = operatorObj.HandleCustomRestore(timeInfoMap, restoreTaskInfo, filePathVector, true, uniqueNumber);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleCustomRestore_Test_003 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandleCustomRestore_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleCustomRestore_Test_004 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "test_key";
    restoreTaskInfo.bundleName = "test.bundle";
    restoreTaskInfo.packageName = "test.package";
    restoreTaskInfo.appId = "test.app";
    restoreTaskInfo.sourceDir = "/storage/media/local/files/test";
    vector<string> filePathVector = {"/storage/media/local/files/test.mp4"};
    unordered_map<string, TimeInfo> timeInfoMap;
    UniqueNumber uniqueNumber;
    int32_t result =
        operatorObj.HandleCustomRestore(timeInfoMap, restoreTaskInfo, filePathVector, false, uniqueNumber);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleCustomRestore_Test_004 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_RenameFiles_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_RenameFiles_Test_004 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    vector<FileInfo> restoreFiles;
    FileInfo fileInfo1;
    fileInfo1.originFilePath = "/storage/media/local/files/test.jpg";
    fileInfo1.filePath = "/storage/media/local/files/Photo/1/test.jpg";
    fileInfo1.isLivePhoto = false;
    restoreFiles.push_back(fileInfo1);
    FileInfo fileInfo2;
    fileInfo2.originFilePath = "/storage/media/local/files/test2.jpg";
    fileInfo2.filePath = "/storage/media/local/files/Photo/2/test2.jpg";
    fileInfo2.isLivePhoto = false;
    restoreFiles.push_back(fileInfo2);
    int32_t result = operatorObj.RenameFiles(restoreFiles);
    EXPECT_EQ(result, 0);
    MEDIA_INFO_LOG("Photo_Custom_Restore_RenameFiles_Test_004 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_RenameFiles_Test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_RenameFiles_Test_005 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    vector<FileInfo> restoreFiles;
    FileInfo fileInfo1;
    fileInfo1.originFilePath = "/storage/media/local/files/live_photo1.jpg";
    fileInfo1.filePath = "/storage/media/local/files/Photo/1/live_photo1.jpg";
    fileInfo1.isLivePhoto = true;
    restoreFiles.push_back(fileInfo1);
    FileInfo fileInfo2;
    fileInfo2.originFilePath = "/storage/media/local/files/live_photo2.jpg";
    fileInfo2.filePath = "/storage/media/local/files/Photo/2/live_photo2.jpg";
    fileInfo2.isLivePhoto = true;
    restoreFiles.push_back(fileInfo2);
    int32_t result = operatorObj.RenameFiles(restoreFiles);
    EXPECT_EQ(result, 0);
    MEDIA_INFO_LOG("Photo_Custom_Restore_RenameFiles_Test_005 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_RenameFiles_Test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_RenameFiles_Test_006 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    vector<FileInfo> restoreFiles;
    FileInfo fileInfo1;
    fileInfo1.originFilePath = "/storage/media/local/files/test.jpg";
    fileInfo1.filePath = "/storage/media/local/files/Photo/1/test.jpg";
    fileInfo1.isLivePhoto = false;
    restoreFiles.push_back(fileInfo1);
    FileInfo fileInfo2;
    fileInfo2.originFilePath = "/storage/media/local/files/live_photo.jpg";
    fileInfo2.filePath = "/storage/media/local/files/Photo/2/live_photo.jpg";
    fileInfo2.isLivePhoto = true;
    restoreFiles.push_back(fileInfo2);
    int32_t result = operatorObj.RenameFiles(restoreFiles);
    EXPECT_EQ(result, 0);
    MEDIA_INFO_LOG("Photo_Custom_Restore_RenameFiles_Test_006 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_FillMetadata_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_FillMetadata_Test_003 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    FileInfo fileInfo;
    fileInfo.fileName = "test.mp4";
    fileInfo.originFilePath = "/storage/media/local/files/test.mp4";
    fileInfo.mediaType = MediaType::MEDIA_TYPE_VIDEO;
    unique_ptr<Metadata> data = make_unique<Metadata>();
    unordered_map<string, TimeInfo> timeInfoMap;
    timeInfoMap["test.mp4"] = {MediaFileUtils::UTCTimeMilliSeconds(),
                                MediaFileUtils::UTCTimeMilliSeconds(),
                                "2024-01-01 12:00:00"};
    int32_t result = operatorObj.FillMetadata(timeInfoMap, fileInfo, data);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_FillMetadata_Test_003 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetFileMetadata_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetFileMetadata_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    unique_ptr<Metadata> data = make_unique<Metadata>();
    data->SetFilePath("/storage/media/local/files/test/nonexistent.jpg");
    data->SetFileName("nonexistent.jpg");
    int32_t result = operatorObj.GetFileMetadata(data);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetFileMetadata_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_GetFileMetadata_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetFileMetadata_Test_003 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    unique_ptr<Metadata> data = make_unique<Metadata>();
    data->SetFilePath("/storage/media/local/files/nonexistent.mp4");
    data->SetFileName("nonexistent.mp4");
    int32_t result = operatorObj.GetFileMetadata(data);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_GetFileMetadata_Test_003 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_InitPhotoCache_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_InitPhotoCache_Test_004 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.isDeduplication = true;
    restoreTaskInfo.albumId = 2;
    int32_t result = operatorObj.InitPhotoCache(restoreTaskInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_InitPhotoCache_Test_004 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_InitPhotoCache_Test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_InitPhotoCache_Test_005 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.isDeduplication = true;
    restoreTaskInfo.albumId = 3;
    int32_t result = operatorObj.InitPhotoCache(restoreTaskInfo);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_InitPhotoCache_Test_005 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_UpdatePhotoAlbum_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_UpdatePhotoAlbum_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.bundleName = "test.bundle";
    restoreTaskInfo.packageName = "test.package";
    FileInfo fileInfo;
    fileInfo.fileName = "test.mp4";
    fileInfo.displayName = "test.mp4";
    fileInfo.filePath = "/storage/media/local/files/Photo/1/test.mp4";
    fileInfo.mediaType = MediaType::MEDIA_TYPE_VIDEO;
    int32_t result = operatorObj.UpdatePhotoAlbum(restoreTaskInfo, fileInfo);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_UpdatePhotoAlbum_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_UpdatePhotoAlbum_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_UpdatePhotoAlbum_Test_003 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.bundleName = "test2.bundle";
    restoreTaskInfo.packageName = "test2.package";
    FileInfo fileInfo;
    fileInfo.fileName = "test2.jpg";
    fileInfo.displayName = "test2.jpg";
    fileInfo.filePath = "/storage/media/local/files/Photo/2/test2.jpg";
    fileInfo.mediaType = MediaType::MEDIA_TYPE_IMAGE;
    int32_t result = operatorObj.UpdatePhotoAlbum(restoreTaskInfo, fileInfo);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_UpdatePhotoAlbum_Test_003 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_UpdateUniqueNumber_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_UpdateUniqueNumber_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    UniqueNumber uniqueNumber;
    uniqueNumber.imageTotalNumber = 20;
    uniqueNumber.videoTotalNumber = 10;
    int32_t result = operatorObj.UpdateUniqueNumber(uniqueNumber);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_UpdateUniqueNumber_Test_002 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_UpdateUniqueNumber_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_UpdateUniqueNumber_Test_003 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    UniqueNumber uniqueNumber;
    uniqueNumber.imageTotalNumber = 0;
    uniqueNumber.videoTotalNumber = 0;
    int32_t result = operatorObj.UpdateUniqueNumber(uniqueNumber);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_UpdateUniqueNumber_Test_003 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandleTlvSingleRestore_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleTlvSingleRestore_Test_004 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "test_key2";
    restoreTaskInfo.sourceDir = "/storage/media/local/files/test2";
    restoreTaskInfo.bundleName = "test.bundle";
    restoreTaskInfo.packageName = "test.package";
    restoreTaskInfo.appId = "test.app";
    unordered_map<TlvTag, string> editFileMap;
    editFileMap[TlvTag::TLV_TAG_ORIGIN] = "/storage/media/local/files/test2/origin.jpg";
    editFileMap[TlvTag::TLV_TAG_JSON] = "/storage/media/local/files/test2/edit.json";
    unordered_map<string, TimeInfo> timeInfoMap;
    UniqueNumber uniqueNumber;
    int32_t result = operatorObj.HandleTlvSingleRestore(editFileMap,
        timeInfoMap, restoreTaskInfo, false, uniqueNumber);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleTlvSingleRestore_Test_004 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandleAllEditData_Test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleAllEditData_Test_005 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    unordered_map<TlvTag, string> decodeTlvPathMap;
    decodeTlvPathMap[TlvTag::TLV_TAG_ORIGIN] = "/storage/media/local/files/origin2.jpg";
    decodeTlvPathMap[TlvTag::TLV_TAG_JSON] = "/storage/media/local/files/edit2.json";
    string assetPath = "/storage/media/local/files/Photo/2/asset.jpg";
    int32_t result = operatorObj.HandleAllEditData(decodeTlvPathMap, assetPath);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleAllEditData_Test_005 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandleAllEditData_Test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleAllEditData_Test_006 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    unordered_map<TlvTag, string> decodeTlvPathMap;
    decodeTlvPathMap[TlvTag::TLV_TAG_ORIGIN] = "/storage/media/local/files/origin3.jpg";
    decodeTlvPathMap[TlvTag::TLV_TAG_JSON] = "/storage/media/local/files/edit3.json";
    decodeTlvPathMap[TlvTag::TLV_TAG_EDITDATA] = "/storage/media/local/files/edit_data3.jpg";
    decodeTlvPathMap[TlvTag::TLV_TAG_CAMERA] = "/storage/media/local/files/camera3.jpg";
    string assetPath = "/storage/media/local/files/Photo/3/asset.jpg";
    int32_t result = operatorObj.HandleAllEditData(decodeTlvPathMap, assetPath);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandleAllEditData_Test_006 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_ProcessExtractTlvFile_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_ProcessExtractTlvFile_Test_003 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    string tlvFilePath = "/storage/media/local/files/test2.tlv";
    string destDir;
    unordered_map<TlvTag, string> extractedFiles;
    int32_t result = operatorObj.ProcessExtractTlvFile(tlvFilePath, destDir, extractedFiles);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_ProcessExtractTlvFile_Test_003 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_ProcessExtractTlvFile_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_ProcessExtractTlvFile_Test_004 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    string tlvFilePath = "/storage/media/local/files/subdir/test.tlv";
    string destDir;
    unordered_map<TlvTag, string> extractedFiles;
    int32_t result = operatorObj.ProcessExtractTlvFile(tlvFilePath, destDir, extractedFiles);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_ProcessExtractTlvFile_Test_004 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HasTlvFiles_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HasTlvFiles_Test_003 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    vector<string> files = {"/storage/media/local/files/test.jpg"};
    bool result = operatorObj.HasTlvFiles(files);
    EXPECT_EQ(result, false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HasTlvFiles_Test_003 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HasTlvFiles_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HasTlvFiles_Test_004 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    vector<string> files = {"/storage/media/local/files/test.mp4", "/storage/media/local/files/test2.mp4"};
    bool result = operatorObj.HasTlvFiles(files);
    EXPECT_EQ(result, false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HasTlvFiles_Test_004 End");
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_HandlePhotoSourceRestore_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandlePhotoSourceRestore_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    string sourceBackSrcPath = "/storage/media/local/files/source2.jpg";
    string assetPath = "/storage/media/local/files/Photo/2/asset.jpg";
    int32_t result = operatorObj.HandlePhotoSourceRestore(sourceBackSrcPath, assetPath);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("Photo_Custom_Restore_HandlePhotoSourceRestore_Test_002 End");
}

/*
 * Async Worker Thread Tests
 */

// Test: AddTaskAsync queues task with mutex protection
HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_AddTaskAsync_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_AddTaskAsync_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation::GetInstance();
    // Clear queue
    while (!operatorObj.taskQueue_.empty()) {
        operatorObj.taskQueue_.pop();
    }
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "async_test_001";
    operatorObj.AddTaskAsync(restoreTaskInfo);
    EXPECT_EQ(operatorObj.taskQueue_.size(), 1);
    EXPECT_EQ(operatorObj.taskQueue_.front().keyPath, "async_test_001");
    // Clean up
    operatorObj.taskQueue_.pop();
    MEDIA_INFO_LOG("Photo_Custom_Restore_AddTaskAsync_Test_001 End");
}

// Test: Is a single task added?
HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_AddTaskAsync_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_AddTaskAsync_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    while (!operatorObj.taskQueue_.empty()) {
        operatorObj.taskQueue_.pop();
    }
    RestoreTaskInfo task;
    task.keyPath = "task";
    operatorObj.AddTaskAsync(task);
    EXPECT_FALSE(operatorObj.taskQueue_.empty());
    EXPECT_EQ(operatorObj.taskQueue_.front().keyPath, "task");
    EXPECT_EQ(operatorObj.taskQueue_.size(), 1);
    MEDIA_INFO_LOG("Photo_Custom_Async_Restore_Operation_Test_002 End");
}

// Test: AddTaskAsync queues multiple tasks in order
HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_AddTaskAsync_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_AddTaskAsync_Test_003 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation::GetInstance();
    while (!operatorObj.taskQueue_.empty()) {
        operatorObj.taskQueue_.pop();
    }
    RestoreTaskInfo task1;
    task1.keyPath = "async_task_1";
    RestoreTaskInfo task2;
    task2.keyPath = "async_task_2";
    RestoreTaskInfo task3;
    task3.keyPath = "async_task_3";
    operatorObj.AddTaskAsync(task1);
    operatorObj.AddTaskAsync(task2);
    operatorObj.AddTaskAsync(task3);
    EXPECT_EQ(operatorObj.taskQueue_.size(), 3);
    EXPECT_EQ(operatorObj.taskQueue_.front().keyPath, "async_task_1");
    // Clean up
    while (!operatorObj.taskQueue_.empty()) {
        operatorObj.taskQueue_.pop();
    }
    MEDIA_INFO_LOG("Photo_Custom_Restore_AddTaskAsync_Test_003 End");
}

// Test: Are multiple tasks added?
HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_AddTaskAsync_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_AddTaskAsync_Test_004 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    while (!operatorObj.taskQueue_.empty()) {
        operatorObj.taskQueue_.pop();
    }
    RestoreTaskInfo task1;
    task1.keyPath = "task1";
    RestoreTaskInfo task2;
    task2.keyPath = "task2";
    RestoreTaskInfo task3;
    task3.keyPath = "task3";
    operatorObj.AddTaskAsync(task1);
    operatorObj.AddTaskAsync(task2);
    operatorObj.AddTaskAsync(task3);
    EXPECT_FALSE(operatorObj.taskQueue_.empty());
    EXPECT_EQ(operatorObj.taskQueue_.size(), 3);
    EXPECT_EQ(operatorObj.taskQueue_.front().keyPath, "task1");
    operatorObj.taskQueue_.pop();
    EXPECT_EQ(operatorObj.taskQueue_.front().keyPath, "task2");
    operatorObj.taskQueue_.pop();
    EXPECT_EQ(operatorObj.taskQueue_.front().keyPath, "task3");
    operatorObj.taskQueue_.pop();
    MEDIA_INFO_LOG("Photo_Custom_Restore_AddTaskAsync_Test_004 End");
}

// Test: StartAsync spawns worker thread
HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_StartAsync_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_StartAsync_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation::GetInstance();
    while (!operatorObj.taskQueue_.empty()) {
        operatorObj.taskQueue_.pop();
    }
    operatorObj.workerRunning_.store(false);
    operatorObj.stopWorker_.store(false);
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "start_async_test";
    operatorObj.AddTaskAsync(restoreTaskInfo);
    operatorObj.StartAsync();
    EXPECT_EQ(operatorObj.workerRunning_.load(), true);
    // Wait briefly for worker to process
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    // Stop worker
    operatorObj.stopWorker_.store(true);
    operatorObj.queueCv_.notify_one();
    if (operatorObj.workerThread_.joinable()) {
        operatorObj.workerThread_.join();
    }
    operatorObj.workerRunning_.store(false);
    EXPECT_EQ(operatorObj.taskQueue_.empty(), true);
    MEDIA_INFO_LOG("Photo_Custom_Restore_StartAsync_Test_001 End");
}

// Test: StartAsync skips when worker already running
HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_StartAsync_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_StartAsync_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation::GetInstance();
    operatorObj.workerRunning_.store(true);
    operatorObj.StartAsync();
    // Should have returned early without spawning new thread
    EXPECT_EQ(operatorObj.workerRunning_.load(), true);
    operatorObj.workerRunning_.store(false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_StartAsync_Test_002 End");
}

// Test: Does the flag correctly change from true to false?
HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_StartAsync_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_StartAsync_Test_003 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    operatorObj.stopWorker_.store(true);
    operatorObj.queueCv_.notify_one();
    if (operatorObj.workerThread_.joinable()) {
        operatorObj.workerThread_.join();
    }
    while (!operatorObj.taskQueue_.empty()) {
        operatorObj.taskQueue_.pop();
    }
    operatorObj.workerRunning_.store(false);
    operatorObj.StartAsync();
    EXPECT_EQ(operatorObj.workerRunning_.load(), true);
    operatorObj.stopWorker_.store(true);
    operatorObj.queueCv_.notify_one();
    if (operatorObj.workerThread_.joinable()) {
        operatorObj.workerThread_.join();
    }
    EXPECT_EQ(operatorObj.workerRunning_.load(), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_StartAsync_Test_003 End");
}

//StartAsync restart
HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_StartAsync_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_StartAsync_Test_004 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    operatorObj.stopWorker_.store(true);
    operatorObj.queueCv_.notify_one();
    if (operatorObj.workerThread_.joinable()) {
        operatorObj.workerThread_.join();
    }
    while (!operatorObj.taskQueue_.empty()) {
        operatorObj.taskQueue_.pop();
    }
    operatorObj.workerRunning_.store(false);
    operatorObj.StartAsync();
    EXPECT_EQ(operatorObj.workerRunning_.load(), true);
    
    operatorObj.stopWorker_.store(true);
    operatorObj.queueCv_.notify_one();
    if (operatorObj.workerThread_.joinable()) {
        operatorObj.workerThread_.join();
    }
    EXPECT_FALSE(operatorObj.workerThread_.joinable());
    EXPECT_FALSE(operatorObj.workerRunning_.load());
    
    operatorObj.StartAsync();
    EXPECT_EQ(operatorObj.workerRunning_.load(), true);
    
    RestoreTaskInfo task;
    task.keyPath = "task";
    operatorObj.AddTaskAsync(task);
    EXPECT_EQ(operatorObj.workerRunning_.load(), true);
    int retry = 0;
    while (!operatorObj.taskQueue_.empty() && retry < 50) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        retry++;
    }
    EXPECT_TRUE(operatorObj.taskQueue_.empty());

    operatorObj.stopWorker_.store(true);
    operatorObj.queueCv_.notify_one();
    if (operatorObj.workerThread_.joinable()) {
        operatorObj.workerThread_.join();
    }
    EXPECT_EQ(operatorObj.workerRunning_.load(), false);
    MEDIA_INFO_LOG("Photo_Custom_Async_Restore_Operation_Test_004 End");
}

// Test: When called repeatedly
HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_StartAsync_Test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_StartAsync_Test_005 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    operatorObj.stopWorker_.store(true);
    operatorObj.queueCv_.notify_one();
    if (operatorObj.workerThread_.joinable()) {
        operatorObj.workerThread_.join();
    }
    while (!operatorObj.taskQueue_.empty()) {
        operatorObj.taskQueue_.pop();
    }
    operatorObj.workerRunning_.store(false);
    operatorObj.StartAsync();
    EXPECT_EQ(operatorObj.workerRunning_.load(), true);
    operatorObj.StartAsync();
    EXPECT_EQ(operatorObj.workerRunning_.load(), true);
    operatorObj.stopWorker_.store(true);
    operatorObj.queueCv_.notify_one();
    if (operatorObj.workerThread_.joinable()) {
        operatorObj.workerThread_.join();
    }
    EXPECT_EQ(operatorObj.workerRunning_.load(), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_StartAsync_Test_005 End");
}

// Test: Destructor stops worker thread gracefully
HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Destructor_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Destructor_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation::GetInstance();
    while (!operatorObj.taskQueue_.empty()) {
        operatorObj.taskQueue_.pop();
    }
    operatorObj.workerRunning_.store(false);
    operatorObj.stopWorker_.store(false);
    // Start worker with no tasks - it will wait on cv
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = "destructor_test";
    operatorObj.AddTaskAsync(restoreTaskInfo);
    operatorObj.StartAsync();
    EXPECT_EQ(operatorObj.workerRunning_.load(), true);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    // Simulate destructor behavior
    operatorObj.stopWorker_.store(true);
    operatorObj.queueCv_.notify_one();
    if (operatorObj.workerThread_.joinable()) {
        operatorObj.workerThread_.join();
    }
    EXPECT_EQ(operatorObj.workerRunning_.load(), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Destructor_Test_001 End");
}

// Test: Multiple tasks processed by worker in order
HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_WorkerLoop_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_WorkerLoop_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation::GetInstance();
    while (!operatorObj.taskQueue_.empty()) {
        operatorObj.taskQueue_.pop();
    }
    operatorObj.workerRunning_.store(false);
    operatorObj.stopWorker_.store(false);
    // Queue 3 tasks then start
    RestoreTaskInfo task1;
    task1.keyPath = "worker_task_1";
    RestoreTaskInfo task2;
    task2.keyPath = "worker_task_2";
    RestoreTaskInfo task3;
    task3.keyPath = "worker_task_3";
    operatorObj.AddTaskAsync(task1);
    operatorObj.AddTaskAsync(task2);
    operatorObj.AddTaskAsync(task3);
    EXPECT_EQ(operatorObj.taskQueue_.size(), 3);
    operatorObj.StartAsync();
    // Wait for worker to process all tasks
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    // Stop worker
    operatorObj.stopWorker_.store(true);
    operatorObj.queueCv_.notify_one();
    if (operatorObj.workerThread_.joinable()) {
        operatorObj.workerThread_.join();
    }
    EXPECT_EQ(operatorObj.taskQueue_.empty(), true);
    EXPECT_EQ(operatorObj.workerRunning_.load(), false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_WorkerLoop_Test_001 End");
}

// Test: When multiple tasks arrive, are they dequeued and removed by the worker
HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_WorkerLoop_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_WorkerLoop_Test_002 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    operatorObj.stopWorker_.store(true);
    operatorObj.queueCv_.notify_one();
    if (operatorObj.workerThread_.joinable()) {
        operatorObj.workerThread_.join();
    }
    while (!operatorObj.taskQueue_.empty()) {
        operatorObj.taskQueue_.pop();
    }
    operatorObj.workerRunning_.store(false);
    RestoreTaskInfo task1;
    task1.keyPath = "task_before_start1";
    operatorObj.AddTaskAsync(task1);
    RestoreTaskInfo task2;
    task2.keyPath = "task_before_start2";
    operatorObj.AddTaskAsync(task2);
    operatorObj.StartAsync();
    EXPECT_EQ(operatorObj.workerRunning_.load(), true);
    int retry = 0;
    while (!operatorObj.taskQueue_.empty() && retry < 50) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        retry++;
    }
    EXPECT_TRUE(operatorObj.taskQueue_.empty());
    EXPECT_EQ(operatorObj.taskQueue_.size(), 0);
    operatorObj.stopWorker_.store(true);
    operatorObj.queueCv_.notify_one();
    while (!operatorObj.taskQueue_.empty()) {
        operatorObj.taskQueue_.pop();
    }
    if (operatorObj.workerThread_.joinable()) {
        operatorObj.workerThread_.join();
    }
    MEDIA_INFO_LOG("Photo_Custom_Restore_WorkerLoop_Test_002 End");
}

// Test: Does the worker take and remove a single task from the queue?
HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_WorkerLoop_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_WorkerLoop_Test_003 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    operatorObj.stopWorker_.store(true);
    operatorObj.queueCv_.notify_one();
    if (operatorObj.workerThread_.joinable()) {
        operatorObj.workerThread_.join();
    }
    while (!operatorObj.taskQueue_.empty()) {
        operatorObj.taskQueue_.pop();
    }
    operatorObj.workerRunning_.store(false);
    RestoreTaskInfo task;
    task.keyPath = "task_before_start";
    operatorObj.AddTaskAsync(task);
    operatorObj.StartAsync();
    EXPECT_EQ(operatorObj.workerRunning_.load(), true);
    int retry = 0;
    while (!operatorObj.taskQueue_.empty() && retry < 50) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        retry++;
    }
    EXPECT_TRUE(operatorObj.taskQueue_.empty());
    EXPECT_EQ(operatorObj.taskQueue_.size(), 0);
    operatorObj.stopWorker_.store(true);
    operatorObj.queueCv_.notify_one();
    while (!operatorObj.taskQueue_.empty()) {
        operatorObj.taskQueue_.pop();
    }
    if (operatorObj.workerThread_.joinable()) {
        operatorObj.workerThread_.join();
    }
    MEDIA_INFO_LOG("Photo_Custom_Restore_WorkerLoop_Test_003 End");
}

// Test: If a task is added while the worker is running, is it picked up and removed?
HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_WorkerLoop_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_WorkerLoop_Test_004 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    operatorObj.stopWorker_.store(true);
    operatorObj.queueCv_.notify_one();
    if (operatorObj.workerThread_.joinable()) {
        operatorObj.workerThread_.join();
    }
    while (!operatorObj.taskQueue_.empty()) {
        operatorObj.taskQueue_.pop();
    }
    operatorObj.workerRunning_.store(false);
    operatorObj.StartAsync();
    EXPECT_EQ(operatorObj.workerRunning_.load(), true);
    RestoreTaskInfo task;
    task.keyPath = "task_after_start";
    operatorObj.AddTaskAsync(task);
    int retry = 0;
    while (!operatorObj.taskQueue_.empty() && retry < 50) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        retry++;
    }
    EXPECT_TRUE(operatorObj.taskQueue_.empty());
    EXPECT_EQ(operatorObj.taskQueue_.size(), 0);
    operatorObj.stopWorker_.store(true);
    operatorObj.queueCv_.notify_one();
    while (!operatorObj.taskQueue_.empty()) {
        operatorObj.taskQueue_.pop();
    }
    if (operatorObj.workerThread_.joinable()) {
        operatorObj.workerThread_.join();
    }
    MEDIA_INFO_LOG("Photo_Custom_Restore_WorkerLoop_Test_004 End");
}

// Test: If multiple tasks are added while the worker is running, are they picked up?
HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_WorkerLoop_Test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_WorkerLoop_Test_005 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation ::GetInstance();
    operatorObj.stopWorker_.store(true);
    operatorObj.queueCv_.notify_one();
    if (operatorObj.workerThread_.joinable()) {
        operatorObj.workerThread_.join();
    }
    while (!operatorObj.taskQueue_.empty()) {
        operatorObj.taskQueue_.pop();
    }
    operatorObj.workerRunning_.store(false);
    operatorObj.StartAsync();
    EXPECT_EQ(operatorObj.workerRunning_.load(), true);
    RestoreTaskInfo task1;
    task1.keyPath = "task1_after_start";
    operatorObj.AddTaskAsync(task1);
    RestoreTaskInfo task2;
    task2.keyPath = "task2_after_start";
    operatorObj.AddTaskAsync(task2);
    int retry = 0;
    while (!operatorObj.taskQueue_.empty() && retry < 50) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        retry++;
    }
    EXPECT_TRUE(operatorObj.taskQueue_.empty());
    EXPECT_EQ(operatorObj.taskQueue_.size(), 0);
    operatorObj.stopWorker_.store(true);
    operatorObj.queueCv_.notify_one();
    while (!operatorObj.taskQueue_.empty()) {
        operatorObj.taskQueue_.pop();
    }
    if (operatorObj.workerThread_.joinable()) {
        operatorObj.workerThread_.join();
    }
    MEDIA_INFO_LOG("Photo_Custom_Restore_WorkerLoop_Test_005 End");
}

// Test: AddTaskAsync with cancelled task
HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_AsyncCancel_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_AsyncCancel_Test_001 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation::GetInstance();
    while (!operatorObj.taskQueue_.empty()) {
        operatorObj.taskQueue_.pop();
    }
    operatorObj.workerRunning_.store(false);
    operatorObj.stopWorker_.store(false);
    // Queue a task and cancel it
    RestoreTaskInfo task;
    task.keyPath = "cancel_async_test";
    operatorObj.AddTaskAsync(task);
    operatorObj.cancelKeySet_.insert("cancel_async_test");
    operatorObj.StartAsync();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    // Stop worker
    operatorObj.stopWorker_.store(true);
    operatorObj.queueCv_.notify_one();
    if (operatorObj.workerThread_.joinable()) {
        operatorObj.workerThread_.join();
    }
    EXPECT_EQ(operatorObj.taskQueue_.empty(), true);
    operatorObj.workerRunning_.store(false);
    MEDIA_INFO_LOG("Photo_Custom_Restore_AsyncCancel_Test_001 End");
}

// Test: AddTaskAsync with empty sourceDir
HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_AddTaskAsync_EmptySourceDir_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_AddTaskAsync_EmptySourceDir_Test Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation::GetInstance();
    while (!operatorObj.taskQueue_.empty()) {
        operatorObj.taskQueue_.pop();
    }
    RestoreTaskInfo task;
    task.keyPath = "empty_sourcedir_test";
    task.sourceDir = "";  // empty sourceDir - should skip GetDirFiles
    operatorObj.AddTaskAsync(task);
    EXPECT_EQ(operatorObj.taskQueue_.size(), 1);
    operatorObj.taskQueue_.pop();
    MEDIA_INFO_LOG("Photo_Custom_Restore_AddTaskAsync_EmptySourceDir_Test End");
}
} // namespace Media
} // namespace OHOS
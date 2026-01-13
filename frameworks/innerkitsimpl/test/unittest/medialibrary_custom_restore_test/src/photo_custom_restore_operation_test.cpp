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

int32_t InsertShareRestoreTable(const string &folder, const string &dbPath)
{
    vector<string> files;
    GetDirFiles(folder, files);
    std::vector<NativeRdb::ValuesBucket> values;
    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    auto base_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    const auto three_days_ms = 3 * 24 * 60 * 60 * 1000;
    for (auto i = 0; i < files.size(); ++i) {
        NativeRdb::ValuesBucket value;
        auto fileName = MediaFileUtils::GetFileName(files[i]);
        value.Put(SHARE_RESTORE_MEDIA_INFO_FILE_NAME, fileName);
        auto takenOffset = base_ms - (i * three_days_ms * 2);
        std::time_t offsetTime = takenOffset / 1000;
        struct tm tmTime;
        localtime_noenv_r(&offsetTime, &tmTime);
        char detailTime[DETAIL_TIME_SIZE];
        if (strftime(detailTime, sizeof(detailTime), PhotoColumn::PHOTO_DETAIL_TIME_FORMAT.c_str(), &tmTime) == 0) {
            MEDIA_ERR_LOG("strftime error: %{public}d", errno);
            continue;
        }
        auto addedOffset = base_ms - (i * three_days_ms);
        value.Put(SHARE_RESTORE_MEDIA_INFO_DATE_ADDED, static_cast<int64_t>(addedOffset));
        value.Put(SHARE_RESTORE_MEDIA_INFO_DATE_TAKEN, static_cast<int64_t>(takenOffset));
        value.Put(SHARE_RESTORE_MEDIA_INFO_DETAIL_TIME, detailTime);
        values.push_back(value);
    }
    CustomRestoreSourceTest sourceTest;
    auto rdbStore = sourceTest.Init(CUSTOM_RESTORE_DIR + "/" + dbPath);
    EXPECT_NE(rdbStore, nullptr);
    int64_t outRowId = -1;
    auto ret = rdbStore->BatchInsert(outRowId, SHARE_RESTORE_TABLE_NAME, values);
    EXPECT_GT(outRowId, 0);
    return ret;
}

int32_t QueryPhotosCount()
{
    EXPECT_NE(g_rdbStore, nullptr);

    const string sql = "SELECT"
                       "  COUNT( * ) AS count "
                       "FROM"
                       "  Photos;";
    shared_ptr<NativeRdb::ResultSet> resultSet = g_rdbStore->QuerySql(sql);
    EXPECT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToFirstRow(), E_OK);

    int32_t count = GetInt32Val("count", resultSet);
    MEDIA_INFO_LOG("Photos Count is %{public}d", count);
    return count;
}

HWTEST_F(PhotoCustomRestoreOperationTest, Photo_Custom_Restore_Start_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Photo_Custom_Restore_Start_Test Start");
    string albumLpath = "/Share";
    string keyPath = "test";
    string bundleName = "com.test.share.instantshare";
    string appName = "华为分享";
    string appId = "com.test.share.instantshare_test_app_id";
    string dbPath = "test_db/media_info.db";
    string folder = CUSTOM_RESTORE_DIR + "/" + keyPath;

    auto ret = InsertShareRestoreTable(folder, dbPath);
    EXPECT_EQ(ret, E_OK);

    RestoreTaskInfo restoreTaskInfo = {.dbPath = dbPath,
        .albumLpath = albumLpath,
        .keyPath = keyPath,
        .isDeduplication = false,
        .bundleName = bundleName,
        .packageName = appName,
        .appId = appId,
        .sourceDir = folder};
    PhotoCustomRestoreOperation::GetInstance().AddTask(restoreTaskInfo).Start();
    int32_t count = QueryPhotosCount();
    EXPECT_GT(count, 0);
    MEDIA_INFO_LOG("Photo_Custom_Restore_Start_Test End");
}
}
}

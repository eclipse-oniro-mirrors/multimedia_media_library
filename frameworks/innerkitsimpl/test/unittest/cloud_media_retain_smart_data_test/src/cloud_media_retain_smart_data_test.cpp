/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "CloudMediaRetainSmartDataTest"

#include "cloud_media_retain_smart_data_test.h"

#include <chrono>
#include <thread>
#include <vector>

#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_mock_tocken.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unittest_utils.h"
#include "values_bucket.h"
#include "preferences.h"
#include "preferences_helper.h"

#define private public
#define protected public
#include "cloud_media_retain_smart_data.h"
#undef private
#undef protected

using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static uint64_t g_shellToken = 0;
static MediaLibraryMockHapToken* mockToken = nullptr;

void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        "PhotoAlbum",
        "PhotosAlbumBackupForSaveAnalysisData",
        "tab_highlight_album",
        "tab_highlight_cover_info",
    };
    for (auto &dropTable : dropTableList) {
        string dropSql = "DROP TABLE IF EXISTS " + dropTable + ";";
        int32_t ret = g_rdbStore->ExecuteSql(dropSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Drop %{public}s table failed", dropTable.c_str());
        }
    }
}

void SetTables()
{
    vector<string> createTableSqlList = {
        "CREATE TABLE IF NOT EXISTS Photos ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "file_path TEXT, "
        "title TEXT, "
        "name TEXT, "
        "media_type INTEGER, "
        "position INTEGER, "
        "clean_flag INTEGER DEFAULT 0, "
        "cloud_id TEXT, "
        "south_device_type INTEGER DEFAULT 0, "
        "real_lcd_visit_time INTEGER DEFAULT 0, "
        "owner_album_id INTEGER DEFAULT 0);",
        "CREATE TABLE IF NOT EXISTS PhotoAlbum ("
        "album_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "album_type INTEGER DEFAULT 0, "
        "album_subtype INTEGER DEFAULT 0, "
        "is_local INTEGER DEFAULT 0, "
        "lpath TEXT);",
        "CREATE TABLE IF NOT EXISTS PhotosAlbumBackupForSaveAnalysisData ("
        "album_id INTEGER PRIMARY KEY, "
        "lpath TEXT DEFAULT NULL);",
        "CREATE TABLE IF NOT EXISTS tab_highlight_album ("
        "highlight_status INTEGER DEFAULT 0);",
        "CREATE TABLE IF NOT EXISTS tab_highlight_cover_info ("
        "status INTEGER DEFAULT 0);",
    };
    for (auto &createTableSql : createTableSqlList) {
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql failed");
        }
    }
}

void ClearAndRestart()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }
    CleanTestTables();
    SetTables();
}

int32_t InsertPhotoForClean(int64_t &fileId)
{
    ValuesBucket valuesBucket;
    int32_t deleteState = -3;
    valuesBucket.PutInt("media_type", 1);
    valuesBucket.PutInt("position", 1);
    valuesBucket.PutInt("clean_flag", 1);
    valuesBucket.PutInt("south_device_type", 1);
    valuesBucket.PutInt("real_lcd_visit_time", deleteState);
    int32_t ret = g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    return ret;
}

int32_t InsertAlbumForBackup(int64_t &albumId)
{
    ValuesBucket valuesBucket;
    valuesBucket.PutInt("album_type", 1);
    valuesBucket.PutInt("album_subtype", 1);
    valuesBucket.PutInt("is_local", 1);
    valuesBucket.PutString("lpath", "/test/path");
    int32_t ret = g_rdbStore->Insert(albumId, "PhotoAlbum", valuesBucket);
    return ret;
}

void CloudMediaRetainSmartDataTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);

    vector<string> perms;
    perms.push_back("ohos.permission.GET_NETWORK_INFO");
    mockToken = new MediaLibraryMockHapToken("com.ohos.medialibrary.medialibrarydata", perms);
    for (auto &perm : perms) {
        MediaLibraryMockTokenUtils::GrantPermissionByTest(IPCSkeleton::GetSelfTokenID(), perm, 0);
    }

    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Failed to get rdbstore");
        exit(1);
    }
    SetTables();
}

void CloudMediaRetainSmartDataTest::TearDownTestCase(void)
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }
    ClearAndRestart();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    this_thread::sleep_for(chrono::seconds(1));

    if (mockToken != nullptr) {
        delete mockToken;
        mockToken = nullptr;
    }

    MediaLibraryMockTokenUtils::ResetToken();
    SetSelfTokenID(g_shellToken);
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void CloudMediaRetainSmartDataTest::SetUp()
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearAndRestart();
}

void CloudMediaRetainSmartDataTest::TearDown()
{
}

HWTEST_F(CloudMediaRetainSmartDataTest, SmartDataCleanState_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("SmartDataCleanState_test_001 Start");
    SetSmartDataCleanState(CleanTaskState::CLEANING);
    int64_t state = GetSmartDataCleanState();
    EXPECT_EQ(state, static_cast<int64_t>(CleanTaskState::CLEANING));
    MEDIA_INFO_LOG("SmartDataCleanState_test_001 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, SmartDataCleanState_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("SmartDataCleanState_test_002 Start");
    SetSmartDataCleanState(CleanTaskState::IDLE);
    int64_t state = GetSmartDataCleanState();
    EXPECT_EQ(state, static_cast<int64_t>(CleanTaskState::IDLE));
    MEDIA_INFO_LOG("SmartDataCleanState_test_002 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, SmartDataCleanState_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("SmartDataCleanState_test_003 Start");
    int64_t state = GetSmartDataCleanState();
    EXPECT_EQ(state, static_cast<int64_t>(CleanTaskState::IDLE));
    MEDIA_INFO_LOG("SmartDataCleanState_test_003 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, SmartDataCleanState_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("SmartDataCleanState_test_004 Start");
    SetSmartDataCleanState(CleanTaskState::CLEANING);
    int64_t state = GetSmartDataCleanState();
    EXPECT_EQ(state, static_cast<int64_t>(CleanTaskState::CLEANING));
    SetSmartDataCleanState(CleanTaskState::IDLE);
    state = GetSmartDataCleanState();
    EXPECT_EQ(state, static_cast<int64_t>(CleanTaskState::IDLE));
    MEDIA_INFO_LOG("SmartDataCleanState_test_004 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, SmartDataUpdateState_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("SmartDataUpdateState_test_005 Start");
    SetSmartDataUpdateState(UpdateSmartDataState::UPDATE_SMART_DATA);
    int64_t state = GetSmartDataUpdateState();
    EXPECT_EQ(state, static_cast<int64_t>(UpdateSmartDataState::UPDATE_SMART_DATA));
    MEDIA_INFO_LOG("SmartDataUpdateState_test_005 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, SmartDataUpdateState_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("SmartDataUpdateState_test_006 Start");
    SetSmartDataUpdateState(UpdateSmartDataState::IDLE);
    int64_t state = GetSmartDataUpdateState();
    EXPECT_EQ(state, static_cast<int64_t>(UpdateSmartDataState::IDLE));
    MEDIA_INFO_LOG("SmartDataUpdateState_test_006 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, SmartDataUpdateState_test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("SmartDataUpdateState_test_007 Start");
    int64_t state = GetSmartDataUpdateState();
    EXPECT_EQ(state, static_cast<int64_t>(UpdateSmartDataState::IDLE));
    MEDIA_INFO_LOG("SmartDataUpdateState_test_007 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, SmartDataUpdateState_test_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("SmartDataUpdateState_test_008 Start");
    SetSmartDataUpdateState(UpdateSmartDataState::UPDATE_SMART_DATA);
    int64_t state = GetSmartDataUpdateState();
    EXPECT_EQ(state, static_cast<int64_t>(UpdateSmartDataState::UPDATE_SMART_DATA));
    SetSmartDataUpdateState(UpdateSmartDataState::IDLE);
    state = GetSmartDataUpdateState();
    EXPECT_EQ(state, static_cast<int64_t>(UpdateSmartDataState::IDLE));
    MEDIA_INFO_LOG("SmartDataUpdateState_test_008 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, SmartDataRetainTime_test_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("SmartDataRetainTime_test_009 Start");
    SetSmartDataRetainTime();
    int64_t time = GetSmartDataRetainTime();
    EXPECT_GT(time, 0);
    MEDIA_INFO_LOG("SmartDataRetainTime_test_009 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, SmartDataRetainTime_test_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("SmartDataRetainTime_test_010 Start");
    int64_t time = GetSmartDataRetainTime();
    EXPECT_GT(time, 0);
    MEDIA_INFO_LOG("SmartDataRetainTime_test_010 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, SmartDataRetainTime_test_011, TestSize.Level1)
{
    MEDIA_INFO_LOG("SmartDataRetainTime_test_011 Start");
    SetSmartDataRetainTime();
    int64_t time1 = GetSmartDataRetainTime();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    SetSmartDataRetainTime();
    int64_t time2 = GetSmartDataRetainTime();
    EXPECT_GT(time2, time1);
    MEDIA_INFO_LOG("SmartDataRetainTime_test_011 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, SmartDataRetainTime_test_012, TestSize.Level1)
{
    MEDIA_INFO_LOG("SmartDataRetainTime_test_012 Start");
    SetSmartDataRetainTime();
    int64_t time = GetSmartDataRetainTime();
    EXPECT_GT(time, 0);
    SetSmartDataRetainTime();
    int64_t time2 = GetSmartDataRetainTime();
    EXPECT_GE(time2, time);
    MEDIA_INFO_LOG("SmartDataRetainTime_test_012 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, SmartDataProcessingMode_test_013, TestSize.Level1)
{
    MEDIA_INFO_LOG("SmartDataProcessingMode_test_013 Start");
    SetSmartDataProcessingMode(SmartDataProcessingMode::NONE);
    SmartDataProcessingMode mode = GetSmartDataProcessingMode();
    EXPECT_EQ(mode, SmartDataProcessingMode::NONE);
    MEDIA_INFO_LOG("SmartDataProcessingMode_test_013 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, SmartDataProcessingMode_test_014, TestSize.Level1)
{
    MEDIA_INFO_LOG("SmartDataProcessingMode_test_014 Start");
    SetSmartDataProcessingMode(SmartDataProcessingMode::RETAIN);
    SmartDataProcessingMode mode = GetSmartDataProcessingMode();
    EXPECT_EQ(mode, SmartDataProcessingMode::RETAIN);
    MEDIA_INFO_LOG("SmartDataProcessingMode_test_014 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, SmartDataProcessingMode_test_015, TestSize.Level1)
{
    MEDIA_INFO_LOG("SmartDataProcessingMode_test_015 Start");
    SetSmartDataProcessingMode(SmartDataProcessingMode::RECOVER);
    SmartDataProcessingMode mode = GetSmartDataProcessingMode();
    EXPECT_EQ(mode, SmartDataProcessingMode::RECOVER);
    MEDIA_INFO_LOG("SmartDataProcessingMode_test_015 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, SmartDataProcessingMode_test_016, TestSize.Level1)
{
    MEDIA_INFO_LOG("SmartDataProcessingMode_test_016 Start");
    SmartDataProcessingMode mode = GetSmartDataProcessingMode();
    EXPECT_EQ(mode, SmartDataProcessingMode::NONE);
    MEDIA_INFO_LOG("SmartDataProcessingMode_test_016 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, SmartDataProcessingMode_test_017, TestSize.Level1)
{
    MEDIA_INFO_LOG("SmartDataProcessingMode_test_017 Start");
    SetSmartDataProcessingMode(SmartDataProcessingMode::RETAIN);
    SmartDataProcessingMode mode = GetSmartDataProcessingMode();
    EXPECT_EQ(mode, SmartDataProcessingMode::RETAIN);
    SetSmartDataProcessingMode(SmartDataProcessingMode::RECOVER);
    mode = GetSmartDataProcessingMode();
    EXPECT_EQ(mode, SmartDataProcessingMode::RECOVER);
    MEDIA_INFO_LOG("SmartDataProcessingMode_test_017 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, IsNeedRecoverSmartData_test_018, TestSize.Level1)
{
    MEDIA_INFO_LOG("IsNeedRecoverSmartData_test_018 Start");
    SetSmartDataProcessingMode(SmartDataProcessingMode::RECOVER);
    bool needRecover = IsNeedRecoverSmartData();
    EXPECT_TRUE(needRecover);
    MEDIA_INFO_LOG("IsNeedRecoverSmartData_test_018 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, IsNeedRecoverSmartData_test_019, TestSize.Level1)
{
    MEDIA_INFO_LOG("IsNeedRecoverSmartData_test_019 Start");
    SetSmartDataProcessingMode(SmartDataProcessingMode::RETAIN);
    bool needRecover = IsNeedRecoverSmartData();
    EXPECT_FALSE(needRecover);
    SetSmartDataProcessingMode(SmartDataProcessingMode::NONE);
    needRecover = IsNeedRecoverSmartData();
    EXPECT_FALSE(needRecover);
    MEDIA_INFO_LOG("IsNeedRecoverSmartData_test_019 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, SouthDeviceNextStatus_test_020, TestSize.Level1)
{
    MEDIA_INFO_LOG("SouthDeviceNextStatus_test_020 Start");
    SetSouthDeviceNextStatus(CloudMediaRetainType::RETAIN_FORCE, SwitchStatus::HDC);
    SwitchStatus status = GetSouthDeviceNextStatus(CloudMediaRetainType::RETAIN_FORCE);
    EXPECT_EQ(status, SwitchStatus::HDC);
    MEDIA_INFO_LOG("SouthDeviceNextStatus_test_020 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, SouthDeviceNextStatus_test_021, TestSize.Level1)
{
    MEDIA_INFO_LOG("SouthDeviceNextStatus_test_021 Start");
    SetSouthDeviceNextStatus(CloudMediaRetainType::RETAIN_FORCE, SwitchStatus::CLOUD);
    SwitchStatus status = GetSouthDeviceNextStatus(CloudMediaRetainType::RETAIN_FORCE);
    EXPECT_EQ(status, SwitchStatus::CLOUD);
    MEDIA_INFO_LOG("SouthDeviceNextStatus_test_021 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, SouthDeviceNextStatus_test_022, TestSize.Level1)
{
    MEDIA_INFO_LOG("SouthDeviceNextStatus_test_022 Start");
    SetSouthDeviceNextStatus(CloudMediaRetainType::HDC_RETAIN_FORCE, SwitchStatus::CLOUD);
    SwitchStatus status = GetSouthDeviceNextStatus(CloudMediaRetainType::HDC_RETAIN_FORCE);
    EXPECT_EQ(status, SwitchStatus::CLOUD);
    MEDIA_INFO_LOG("SouthDeviceNextStatus_test_022 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, SouthDeviceNextStatus_test_023, TestSize.Level1)
{
    MEDIA_INFO_LOG("SouthDeviceNextStatus_test_023 Start");
    SetSouthDeviceNextStatus(CloudMediaRetainType::HDC_RETAIN_FORCE, SwitchStatus::HDC);
    SwitchStatus status = GetSouthDeviceNextStatus(CloudMediaRetainType::HDC_RETAIN_FORCE);
    EXPECT_EQ(status, SwitchStatus::HDC);
    MEDIA_INFO_LOG("SouthDeviceNextStatus_test_023 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, SouthDeviceNextStatus_test_024, TestSize.Level1)
{
    MEDIA_INFO_LOG("SouthDeviceNextStatus_test_024 Start");
    SetSouthDeviceNextStatus(CloudMediaRetainType::RETAIN_FORCE, SwitchStatus::HDC);
    SwitchStatus status = GetSouthDeviceNextStatus(CloudMediaRetainType::RETAIN_FORCE);
    EXPECT_EQ(status, SwitchStatus::HDC);
    SetSouthDeviceNextStatus(CloudMediaRetainType::RETAIN_FORCE, SwitchStatus::CLOUD);
    status = GetSouthDeviceNextStatus(CloudMediaRetainType::RETAIN_FORCE);
    EXPECT_EQ(status, SwitchStatus::CLOUD);
    MEDIA_INFO_LOG("SouthDeviceNextStatus_test_024 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, SouthDeviceNextStatus_test_025, TestSize.Level1)
{
    MEDIA_INFO_LOG("SouthDeviceNextStatus_test_025 Start");
    SetSouthDeviceNextStatus(CloudMediaRetainType::HDC_RETAIN_FORCE, SwitchStatus::HDC);
    SwitchStatus status = GetSouthDeviceNextStatus(CloudMediaRetainType::HDC_RETAIN_FORCE);
    EXPECT_EQ(status, SwitchStatus::HDC);
    SetSouthDeviceNextStatus(CloudMediaRetainType::HDC_RETAIN_FORCE, SwitchStatus::CLOUD);
    status = GetSouthDeviceNextStatus(CloudMediaRetainType::HDC_RETAIN_FORCE);
    EXPECT_EQ(status, SwitchStatus::CLOUD);
    MEDIA_INFO_LOG("SouthDeviceNextStatus_test_025 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, SouthDeviceNextStatus_test_026, TestSize.Level1)
{
    MEDIA_INFO_LOG("SouthDeviceNextStatus_test_026 Start");
    CloudMediaRetainType invalidType = static_cast<CloudMediaRetainType>(999);
    SwitchStatus status = GetSouthDeviceNextStatus(invalidType);
    EXPECT_EQ(status, SwitchStatus::NONE);
    MEDIA_INFO_LOG("SouthDeviceNextStatus_test_026 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, SouthDeviceNextStatus_test_027, TestSize.Level1)
{
    MEDIA_INFO_LOG("SouthDeviceNextStatus_test_027 Start");
    CloudMediaRetainType invalidType = static_cast<CloudMediaRetainType>(999);
    SetSouthDeviceNextStatus(invalidType, SwitchStatus::HDC);
    SwitchStatus status = GetSouthDeviceNextStatus(invalidType);
    EXPECT_EQ(status, SwitchStatus::NONE);
    MEDIA_INFO_LOG("SouthDeviceNextStatus_test_027 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, GetSmartDataProcessingMode_test_028, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetSmartDataProcessingMode_test_028 Start");
    SmartDataProcessingMode mode = GetSmartDataProcessingMode(
        CloudMediaRetainType::RETAIN_FORCE, SwitchStatus::HDC);
    EXPECT_EQ(mode, SmartDataProcessingMode::RETAIN);
    MEDIA_INFO_LOG("GetSmartDataProcessingMode_test_028 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, GetSmartDataProcessingMode_test_029, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetSmartDataProcessingMode_test_029 Start");
    SmartDataProcessingMode mode = GetSmartDataProcessingMode(
        CloudMediaRetainType::RETAIN_FORCE, SwitchStatus::CLOUD);
    EXPECT_EQ(mode, SmartDataProcessingMode::RECOVER);
    MEDIA_INFO_LOG("GetSmartDataProcessingMode_test_029 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, GetSmartDataProcessingMode_test_030, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetSmartDataProcessingMode_test_030 Start");
    SmartDataProcessingMode mode = GetSmartDataProcessingMode(
        CloudMediaRetainType::HDC_RETAIN_FORCE, SwitchStatus::CLOUD);
    EXPECT_EQ(mode, SmartDataProcessingMode::RECOVER);
    MEDIA_INFO_LOG("GetSmartDataProcessingMode_test_030 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, GetSmartDataProcessingMode_test_031, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetSmartDataProcessingMode_test_031 Start");
    SmartDataProcessingMode mode = GetSmartDataProcessingMode(
        CloudMediaRetainType::HDC_RETAIN_FORCE, SwitchStatus::HDC);
    EXPECT_EQ(mode, SmartDataProcessingMode::RETAIN);
    MEDIA_INFO_LOG("GetSmartDataProcessingMode_test_031 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, GetSmartDataProcessingMode_test_032, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetSmartDataProcessingMode_test_032 Start");
    SmartDataProcessingMode mode = GetSmartDataProcessingMode(
        CloudMediaRetainType::RETAIN_FORCE, SwitchStatus::NONE);
    EXPECT_EQ(mode, SmartDataProcessingMode::NONE);
    mode = GetSmartDataProcessingMode(
        CloudMediaRetainType::HDC_RETAIN_FORCE, SwitchStatus::NONE);
    EXPECT_EQ(mode, SmartDataProcessingMode::NONE);
    MEDIA_INFO_LOG("GetSmartDataProcessingMode_test_032 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, BackupPhotosAlbumTable_test_033, TestSize.Level1)
{
    MEDIA_INFO_LOG("BackupPhotosAlbumTable_test_033 Start");
    InitBackupPhotosAlbumTable();
    std::string checkSql = "SELECT count(*) FROM PhotosAlbumBackupForSaveAnalysisData";
    auto resultSet = g_rdbStore->QuerySql(checkSql);
    ASSERT_NE(resultSet, nullptr);
    int32_t rowCount = 0;
    resultSet->GetRowCount(rowCount);
    resultSet->Close();
    EXPECT_GE(rowCount, 0);
    MEDIA_INFO_LOG("BackupPhotosAlbumTable_test_033 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, BackupPhotosAlbumTable_test_034, TestSize.Level1)
{
    MEDIA_INFO_LOG("BackupPhotosAlbumTable_test_034 Start");
    InitBackupPhotosAlbumTable();
    InitBackupPhotosAlbumTable();
    std::string checkSql = "SELECT count(*) FROM PhotosAlbumBackupForSaveAnalysisData";
    auto resultSet = g_rdbStore->QuerySql(checkSql);
    ASSERT_NE(resultSet, nullptr);
    int32_t rowCount = 0;
    resultSet->GetRowCount(rowCount);
    resultSet->Close();
    EXPECT_GE(rowCount, 0);
    MEDIA_INFO_LOG("BackupPhotosAlbumTable_test_034 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, BackupPhotosAlbumTable_test_035, TestSize.Level1)
{
    MEDIA_INFO_LOG("BackupPhotosAlbumTable_test_035 Start");
    InitBackupPhotosAlbumTable();
    int64_t albumId = 0;
    InsertAlbumForBackup(albumId);
    BackupBackupPhotosAlbumTable();
    std::string checkSql = "SELECT count(*) FROM PhotosAlbumBackupForSaveAnalysisData";
    auto resultSet = g_rdbStore->QuerySql(checkSql);
    ASSERT_NE(resultSet, nullptr);
    int32_t rowCount = 0;
    resultSet->GetRowCount(rowCount);
    resultSet->Close();
    EXPECT_GT(rowCount, 0);
    MEDIA_INFO_LOG("BackupPhotosAlbumTable_test_035 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, BackupPhotosAlbumTable_test_036, TestSize.Level1)
{
    MEDIA_INFO_LOG("BackupPhotosAlbumTable_test_036 Start");
    InitBackupPhotosAlbumTable();
    int64_t albumId = 0;
    InsertAlbumForBackup(albumId);
    BackupBackupPhotosAlbumTable();
    DeleteBackupPhotosAlbumForSmartData();
    std::string checkSql = "SELECT count(*) FROM PhotosAlbumBackupForSaveAnalysisData";
    auto resultSet = g_rdbStore->QuerySql(checkSql);
    ASSERT_NE(resultSet, nullptr);
    int32_t rowCount = 0;
    resultSet->GetRowCount(rowCount);
    resultSet->Close();
    EXPECT_EQ(rowCount, 0);
    MEDIA_INFO_LOG("BackupPhotosAlbumTable_test_036 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, BackupPhotosAlbumTable_test_037, TestSize.Level1)
{
    MEDIA_INFO_LOG("BackupPhotosAlbumTable_test_037 Start");
    InitBackupPhotosAlbumTable();
    BackupBackupPhotosAlbumTable();
    std::string checkSql = "SELECT count(*) FROM PhotosAlbumBackupForSaveAnalysisData";
    auto resultSet = g_rdbStore->QuerySql(checkSql);
    ASSERT_NE(resultSet, nullptr);
    int32_t rowCount = 0;
    resultSet->GetRowCount(rowCount);
    resultSet->Close();
    EXPECT_EQ(rowCount, 0);
    MEDIA_INFO_LOG("BackupPhotosAlbumTable_test_037 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, BackupPhotosAlbumTable_test_038, TestSize.Level1)
{
    MEDIA_INFO_LOG("BackupPhotosAlbumTable_test_038 Start");
    InitBackupPhotosAlbumTable();
    DeleteBackupPhotosAlbumForSmartData();
    std::string checkSql = "SELECT count(*) FROM PhotosAlbumBackupForSaveAnalysisData";
    auto resultSet = g_rdbStore->QuerySql(checkSql);
    ASSERT_NE(resultSet, nullptr);
    int32_t rowCount = 0;
    resultSet->GetRowCount(rowCount);
    resultSet->Close();
    EXPECT_EQ(rowCount, 0);
    MEDIA_INFO_LOG("BackupPhotosAlbumTable_test_038 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, UpdateInvalidCloudHighlightInfo_test_039, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateInvalidCloudHighlightInfo_test_039 Start");
    std::string insertSql = "INSERT INTO tab_highlight_album (highlight_status) VALUES (0)";
    g_rdbStore->ExecuteSql(insertSql);
    insertSql = "INSERT INTO tab_highlight_cover_info (status) VALUES (0)";
    g_rdbStore->ExecuteSql(insertSql);
    UpdateInvalidCloudHighlightInfo();
    std::string checkSql = "SELECT highlight_status FROM tab_highlight_album";
    auto resultSet = g_rdbStore->QuerySql(checkSql);
    ASSERT_NE(resultSet, nullptr);
    int32_t rowCount = 0;
    resultSet->GetRowCount(rowCount);
    EXPECT_GT(rowCount, 0);
    resultSet->Close();
    MEDIA_INFO_LOG("UpdateInvalidCloudHighlightInfo_test_039 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, UpdateInvalidCloudHighlightInfo_test_040, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateInvalidCloudHighlightInfo_test_040 Start");
    std::string insertSql = "INSERT INTO tab_highlight_album (highlight_status) VALUES (0)";
    g_rdbStore->ExecuteSql(insertSql);
    insertSql = "INSERT INTO tab_highlight_album (highlight_status) VALUES (1)";
    g_rdbStore->ExecuteSql(insertSql);
    insertSql = "INSERT INTO tab_highlight_album (highlight_status) VALUES (-3)";
    g_rdbStore->ExecuteSql(insertSql);
    UpdateInvalidCloudHighlightInfo();
    std::string checkSql = "SELECT highlight_status FROM tab_highlight_album WHERE highlight_status = -4";
    auto resultSet = g_rdbStore->QuerySql(checkSql);
    ASSERT_NE(resultSet, nullptr);
    int32_t rowCount = 0;
    resultSet->GetRowCount(rowCount);
    EXPECT_GT(rowCount, 0);
    resultSet->Close();
    MEDIA_INFO_LOG("UpdateInvalidCloudHighlightInfo_test_040 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, UpdateInvalidCloudHighlightInfo_test_041, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateInvalidCloudHighlightInfo_test_041 Start");
    std::string insertSql = "INSERT INTO tab_highlight_cover_info (status) VALUES (0)";
    g_rdbStore->ExecuteSql(insertSql);
    UpdateInvalidCloudHighlightInfo();
    std::string checkSql = "SELECT status FROM tab_highlight_cover_info WHERE status = 1";
    auto resultSet = g_rdbStore->QuerySql(checkSql);
    ASSERT_NE(resultSet, nullptr);
    int32_t rowCount = 0;
    resultSet->GetRowCount(rowCount);
    EXPECT_GT(rowCount, 0);
    resultSet->Close();
    MEDIA_INFO_LOG("UpdateInvalidCloudHighlightInfo_test_041 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, UpdateInvalidCloudHighlightInfo_test_042, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateInvalidCloudHighlightInfo_test_042 Start");
    UpdateInvalidCloudHighlightInfo();
    MEDIA_INFO_LOG("UpdateInvalidCloudHighlightInfo_test_042 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, DoCleanPhotosTableCloudData_test_043, TestSize.Level1)
{
    MEDIA_INFO_LOG("DoCleanPhotosTableCloudData_test_043 Start");
    InitBackupPhotosAlbumTable();
    int32_t ret = DoCleanPhotosTableCloudData();
    EXPECT_EQ(ret, E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    MEDIA_INFO_LOG("DoCleanPhotosTableCloudData_test_043 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, DoCleanPhotosTableCloudData_test_044, TestSize.Level1)
{
    MEDIA_INFO_LOG("DoCleanPhotosTableCloudData_test_044 Start");
    InitBackupPhotosAlbumTable();
    int32_t ret = DoCleanPhotosTableCloudData();
    EXPECT_EQ(ret, E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    MEDIA_INFO_LOG("DoCleanPhotosTableCloudData_test_044 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, DoCleanPhotosTableCloudData_test_045, TestSize.Level1)
{
    MEDIA_INFO_LOG("DoCleanPhotosTableCloudData_test_045 Start");
    InitBackupPhotosAlbumTable();
    for (int i = 0; i < 10; i++) {
        int64_t fileId = 0;
        InsertPhotoForClean(fileId);
    }
    int32_t ret = DoCleanPhotosTableCloudData();
    EXPECT_EQ(ret, E_OK);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::string checkSql = "SELECT count(*) FROM Photos WHERE clean_flag = 1";
    auto resultSet = g_rdbStore->QuerySql(checkSql);
    ASSERT_NE(resultSet, nullptr);
    int32_t rowCount = 0;
    resultSet->GetRowCount(rowCount);
    resultSet->Close();
    EXPECT_EQ(rowCount, 0);
    MEDIA_INFO_LOG("DoCleanPhotosTableCloudData_test_045 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, DoCloudMediaRetainCleanup_test_046, TestSize.Level1)
{
    MEDIA_INFO_LOG("DoCloudMediaRetainCleanup_test_046 Start");
    int32_t ret = DoCloudMediaRetainCleanup();
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("DoCloudMediaRetainCleanup_test_046 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, DoCloudMediaRetainCleanup_test_047, TestSize.Level1)
{
    MEDIA_INFO_LOG("DoCloudMediaRetainCleanup_test_047 Start");
    SetSmartDataRetainTime();
    int32_t ret = DoCloudMediaRetainCleanup();
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("DoCloudMediaRetainCleanup_test_047 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, DoCloudMediaRetainCleanup_test_048, TestSize.Level1)
{
    MEDIA_INFO_LOG("DoCloudMediaRetainCleanup_test_048 Start");
    InitBackupPhotosAlbumTable();
    for (int i = 0; i < 10; i++) {
        int64_t fileId = 0;
        InsertPhotoForClean(fileId);
    }
    SetSmartDataRetainTime();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    int32_t ret = DoCloudMediaRetainCleanup();
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("DoCloudMediaRetainCleanup_test_048 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, UpdatePhotosLcdVisitTime_test_049, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdatePhotosLcdVisitTime_test_049 Start");
    int64_t fileId = 0;
    ValuesBucket valuesBucket;
    valuesBucket.PutInt("media_type", 1);
    valuesBucket.PutInt("real_lcd_visit_time", 0);
    g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    std::vector<std::string> fileIds = {std::to_string(fileId)};
    int32_t ret = UpdatePhotosLcdVisitTime(fileIds);
    EXPECT_EQ(ret, E_OK);
    std::string checkSql = "SELECT real_lcd_visit_time FROM Photos WHERE id = " + std::to_string(fileId);
    auto resultSet = g_rdbStore->QuerySql(checkSql);
    ASSERT_NE(resultSet, nullptr);
    resultSet->GoToFirstRow();
    int64_t lcdTime = 0;
    resultSet->GetLong(0, lcdTime);
    resultSet->Close();
    EXPECT_EQ(lcdTime, REAL_LCD_VISIT_TIME_DELETED);
    MEDIA_INFO_LOG("UpdatePhotosLcdVisitTime_test_049 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, UpdatePhotosLcdVisitTime_test_050, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdatePhotosLcdVisitTime_test_050 Start");
    std::vector<int64_t> fileIdsNum;
    std::vector<std::string> fileIds;
    for (int i = 0; i < 5; i++) {
        int64_t fileId = 0;
        ValuesBucket valuesBucket;
        valuesBucket.PutInt("media_type", 1);
        valuesBucket.PutInt("real_lcd_visit_time", 0);
        g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
        fileIdsNum.push_back(fileId);
        fileIds.push_back(std::to_string(fileId));
    }
    int32_t ret = UpdatePhotosLcdVisitTime(fileIds);
    EXPECT_EQ(ret, E_OK);
    for (auto fileId : fileIdsNum) {
        std::string checkSql = "SELECT real_lcd_visit_time FROM Photos WHERE id = " + std::to_string(fileId);
        auto resultSet = g_rdbStore->QuerySql(checkSql);
        ASSERT_NE(resultSet, nullptr);
        resultSet->GoToFirstRow();
        int64_t lcdTime = 0;
        resultSet->GetLong(0, lcdTime);
        resultSet->Close();
        EXPECT_EQ(lcdTime, REAL_LCD_VISIT_TIME_DELETED);
    }
    MEDIA_INFO_LOG("UpdatePhotosLcdVisitTime_test_050 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, UpdatePhotosLcdVisitTime_test_051, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdatePhotosLcdVisitTime_test_051 Start");
    std::vector<std::string> fileIds;
    int32_t ret = UpdatePhotosLcdVisitTime(fileIds);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("UpdatePhotosLcdVisitTime_test_051 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, UpdatePhotosLcdVisitTime_test_052, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdatePhotosLcdVisitTime_test_052 Start");
    std::vector<std::string> fileIds = {"999999", "999998"};
    int32_t ret = UpdatePhotosLcdVisitTime(fileIds);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("UpdatePhotosLcdVisitTime_test_052 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, DoUpdateSmartDataAlbum_test_053, TestSize.Level1)
{
    MEDIA_INFO_LOG("DoUpdateSmartDataAlbum_test_053 Start");
    int32_t ret = DoUpdateSmartDataAlbum();
    EXPECT_EQ(ret, E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    MEDIA_INFO_LOG("DoUpdateSmartDataAlbum_test_053 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, DoUpdateSmartDataAlbum_test_054, TestSize.Level1)
{
    MEDIA_INFO_LOG("DoUpdateSmartDataAlbum_test_054 Start");
    int32_t ret = DoUpdateSmartDataAlbum();
    EXPECT_EQ(ret, E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    MEDIA_INFO_LOG("DoUpdateSmartDataAlbum_test_054 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, DoUpdateSmartDataAlbum_test_055, TestSize.Level1)
{
    MEDIA_INFO_LOG("DoUpdateSmartDataAlbum_test_055 Start");
    int32_t ret = DoUpdateSmartDataAlbum();
    EXPECT_EQ(ret, E_OK);
    ret = DoUpdateSmartDataAlbum();
    EXPECT_EQ(ret, E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    MEDIA_INFO_LOG("DoUpdateSmartDataAlbum_test_055 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, BoundaryConditions_test_056, TestSize.Level1)
{
    MEDIA_INFO_LOG("BoundaryConditions_test_056 Start");
    CleanTaskState invalidState = static_cast<CleanTaskState>(999);
    SetSmartDataCleanState(invalidState);
    int64_t state = GetSmartDataCleanState();
    EXPECT_EQ(state, 999);
    MEDIA_INFO_LOG("BoundaryConditions_test_056 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, BoundaryConditions_test_057, TestSize.Level1)
{
    MEDIA_INFO_LOG("BoundaryConditions_test_057 Start");
    UpdateSmartDataState invalidState = static_cast<UpdateSmartDataState>(999);
    SetSmartDataUpdateState(invalidState);
    int64_t state = GetSmartDataUpdateState();
    EXPECT_EQ(state, 999);
    MEDIA_INFO_LOG("BoundaryConditions_test_057 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, BoundaryConditions_test_058, TestSize.Level1)
{
    MEDIA_INFO_LOG("BoundaryConditions_test_058 Start");
    SmartDataProcessingMode invalidMode = static_cast<SmartDataProcessingMode>(999);
    SetSmartDataProcessingMode(invalidMode);
    SmartDataProcessingMode mode = GetSmartDataProcessingMode();
    EXPECT_EQ(mode, invalidMode);
    MEDIA_INFO_LOG("BoundaryConditions_test_058 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, BoundaryConditions_test_059, TestSize.Level1)
{
    MEDIA_INFO_LOG("BoundaryConditions_test_059 Start");
    CloudMediaRetainType invalidType = static_cast<CloudMediaRetainType>(999);
    SetSouthDeviceNextStatus(invalidType, SwitchStatus::HDC);
    MEDIA_INFO_LOG("BoundaryConditions_test_059 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, BoundaryConditions_test_060, TestSize.Level1)
{
    MEDIA_INFO_LOG("BoundaryConditions_test_060 Start");
    CloudMediaRetainType invalidType = static_cast<CloudMediaRetainType>(999);
    SwitchStatus status = GetSouthDeviceNextStatus(invalidType);
    EXPECT_EQ(status, SwitchStatus::NONE);
    MEDIA_INFO_LOG("BoundaryConditions_test_060 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, BoundaryConditions_test_061, TestSize.Level1)
{
    MEDIA_INFO_LOG("BoundaryConditions_test_061 Start");
    CloudMediaRetainType invalidType = static_cast<CloudMediaRetainType>(999);
    SmartDataProcessingMode mode = GetSmartDataProcessingMode(invalidType, SwitchStatus::HDC);
    EXPECT_EQ(mode, SmartDataProcessingMode::NONE);
    MEDIA_INFO_LOG("BoundaryConditions_test_061 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, BoundaryConditions_test_062, TestSize.Level1)
{
    MEDIA_INFO_LOG("BoundaryConditions_test_062 Start");
    std::vector<std::string> fileIds = {"1"};
    int32_t ret = UpdatePhotosLcdVisitTime(fileIds);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("BoundaryConditions_test_062 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, BoundaryConditions_test_063, TestSize.Level1)
{
    MEDIA_INFO_LOG("BoundaryConditions_test_063 Start");
    int32_t ret = DoUpdateSmartDataAlbum();
    EXPECT_EQ(ret, E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    MEDIA_INFO_LOG("BoundaryConditions_test_063 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, IntegrationTest_test_064, TestSize.Level1)
{
    MEDIA_INFO_LOG("IntegrationTest_test_064 Start");
    InitBackupPhotosAlbumTable();
    SetSmartDataProcessingMode(SmartDataProcessingMode::RETAIN);
    SetSouthDeviceNextStatus(CloudMediaRetainType::RETAIN_FORCE, SwitchStatus::HDC);
    BackupBackupPhotosAlbumTable();
    SetSmartDataRetainTime();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    MEDIA_INFO_LOG("IntegrationTest_test_064 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, IntegrationTest_test_065, TestSize.Level1)
{
    MEDIA_INFO_LOG("IntegrationTest_test_065 Start");
    InitBackupPhotosAlbumTable();
    SetSmartDataProcessingMode(SmartDataProcessingMode::RECOVER);
    SetSouthDeviceNextStatus(CloudMediaRetainType::RETAIN_FORCE, SwitchStatus::CLOUD);
    BackupBackupPhotosAlbumTable();
    SetSmartDataRetainTime();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    MEDIA_INFO_LOG("IntegrationTest_test_065 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, IntegrationTest_test_066, TestSize.Level1)
{
    MEDIA_INFO_LOG("IntegrationTest_test_066 Start");
    SetSmartDataCleanState(CleanTaskState::IDLE);
    SetSmartDataCleanState(CleanTaskState::CLEANING);
    int64_t state = GetSmartDataCleanState();
    EXPECT_EQ(state, static_cast<int64_t>(CleanTaskState::CLEANING));
    SetSmartDataCleanState(CleanTaskState::IDLE);
    state = GetSmartDataCleanState();
    EXPECT_EQ(state, static_cast<int64_t>(CleanTaskState::IDLE));
    MEDIA_INFO_LOG("IntegrationTest_test_066 End");
}

HWTEST_F(CloudMediaRetainSmartDataTest, IntegrationTest_test_067, TestSize.Level1)
{
    MEDIA_INFO_LOG("IntegrationTest_test_067 Start");
    InitBackupPhotosAlbumTable();
    for (int i = 0; i < 5; i++) {
        int64_t fileId = 0;
        InsertPhotoForClean(fileId);
    }
    SetSmartDataRetainTime();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    int32_t ret = DoCloudMediaRetainCleanup();
    EXPECT_EQ(ret, E_OK);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::string checkSql = "SELECT count(*) FROM Photos WHERE clean_flag = 1";
    auto resultSet = g_rdbStore->QuerySql(checkSql);
    ASSERT_NE(resultSet, nullptr);
    int32_t rowCount = 0;
    resultSet->GetRowCount(rowCount);
    resultSet->Close();
    EXPECT_EQ(rowCount, 0);
    MEDIA_INFO_LOG("IntegrationTest_test_067 End");
}

} // namespace Media
} // namespace OHOS

/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "GlobalScannerTest"
#define private public

#include "global_scanner_test.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include "global_scanner.h"
#include "folder_scanner.h"
#include "folder_scanner_utils.h"
#include "media_lake_clone_event_manager.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
using namespace testing::ext;

static const std::string TEST_ROOT_PATH = "/data/test/media_lake_scanner";

void GlobalScannerTest::SetUpTestCase()
{
    std::error_code ec;
    if (!std::filesystem::exists(TEST_ROOT_PATH, ec)) {
        std::filesystem::create_directories(TEST_ROOT_PATH, ec);
    }
}

void GlobalScannerTest::TearDownTestCase()
{
    std::error_code ec;
    if (std::filesystem::exists(TEST_ROOT_PATH, ec)) {
        std::filesystem::remove_all(TEST_ROOT_PATH, ec);
    }
}

void GlobalScannerTest::SetUp()
{
}

void GlobalScannerTest::TearDown()
{
}

HWTEST_F(GlobalScannerTest, GetInstance_001, TestSize.Level1)
{
    GlobalScanner& instance1 = GlobalScanner::GetInstance();
    GlobalScanner& instance2 = GlobalScanner::GetInstance();
    EXPECT_EQ(&instance1, &instance2);
}

HWTEST_F(GlobalScannerTest, GetScannerStatus_001, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, GetScannerStatus_002, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    ScannerStatus initialStatus = scanner.GetScannerStatus();
    EXPECT_EQ(initialStatus, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, InterruptScanner_001, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    scanner.InterruptScanner();
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, InterruptScanner_002, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    scanner.InterruptScanner();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, InterruptScanner_003, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    ScannerStatus beforeStatus = scanner.GetScannerStatus();
    scanner.InterruptScanner();
    ScannerStatus afterStatus = scanner.GetScannerStatus();
    EXPECT_EQ(beforeStatus, ScannerStatus::IDLE);
    EXPECT_EQ(afterStatus, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, IsGlobalScanning_001, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = "/data/test/path";
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    std::vector<MediaNotifyInfo> notifyInfos = {notifyInfo};
    bool isScanning = scanner.IsGlobalScanning(notifyInfos, ScanTaskType::File);
    EXPECT_FALSE(isScanning);
}

HWTEST_F(GlobalScannerTest, IsGlobalScanning_002, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = "/data/test/path";
    notifyInfo.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo.optType = FileNotifyOperationType::MOD;
    std::vector<MediaNotifyInfo> notifyInfos = {notifyInfo};
    bool isScanning = scanner.IsGlobalScanning(notifyInfos, ScanTaskType::Folder);
    EXPECT_FALSE(isScanning);
}

HWTEST_F(GlobalScannerTest, IsGlobalScanning_003, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    MediaNotifyInfo notifyInfo1;
    notifyInfo1.afterPath = "/data/test/path1";
    notifyInfo1.objType = FileNotifyObjectType::FILE;
    notifyInfo1.optType = FileNotifyOperationType::ADD;
    
    MediaNotifyInfo notifyInfo2;
    notifyInfo2.afterPath = "/data/test/path2";
    notifyInfo2.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo2.optType = FileNotifyOperationType::DEL;
    
    std::vector<MediaNotifyInfo> notifyInfos = {notifyInfo1, notifyInfo2};
    bool isScanning = scanner.IsGlobalScanning(notifyInfos, ScanTaskType::File);
    EXPECT_FALSE(isScanning);
}

HWTEST_F(GlobalScannerTest, IsGlobalScanning_004, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = "";
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    std::vector<MediaNotifyInfo> notifyInfos = {notifyInfo};
    bool isScanning = scanner.IsGlobalScanning(notifyInfos, ScanTaskType::File);
    EXPECT_FALSE(isScanning);
}

HWTEST_F(GlobalScannerTest, IsGlobalScanning_005, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::vector<MediaNotifyInfo> notifyInfos;
    bool isScanning = scanner.IsGlobalScanning(notifyInfos, ScanTaskType::File);
    EXPECT_FALSE(isScanning);
}

HWTEST_F(GlobalScannerTest, IsGlobalScanning_006, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = "/data/test/path";
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    std::vector<MediaNotifyInfo> notifyInfos = {notifyInfo};
    bool isScanningFile = scanner.IsGlobalScanning(notifyInfos, ScanTaskType::File);
    bool isScanningFolder = scanner.IsGlobalScanning(notifyInfos, ScanTaskType::Folder);
    EXPECT_FALSE(isScanningFile);
    EXPECT_FALSE(isScanningFolder);
}

HWTEST_F(GlobalScannerTest, IsGlobalScanning_007, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = "/storage/media/local/files/Photos/test.jpg";
    notifyInfo.beforePath = "/storage/media/local/files/Photos/old.jpg";
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = FileNotifyOperationType::MOD;
    std::vector<MediaNotifyInfo> notifyInfos = {notifyInfo};
    bool isScanning = scanner.IsGlobalScanning(notifyInfos, ScanTaskType::File);
    EXPECT_FALSE(isScanning);
}

HWTEST_F(GlobalScannerTest, IsGlobalScanning_008, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = "/storage/media/local/files/Videos/test.mp4";
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    std::vector<MediaNotifyInfo> notifyInfos = {notifyInfo};
    bool isScanning = scanner.IsGlobalScanning(notifyInfos, ScanTaskType::File);
    EXPECT_FALSE(isScanning);
}

HWTEST_F(GlobalScannerTest, IsGlobalScanning_009, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = "/storage/media/local/files/Audios/test.mp3";
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = FileNotifyOperationType::DEL;
    std::vector<MediaNotifyInfo> notifyInfos = {notifyInfo};
    bool isScanning = scanner.IsGlobalScanning(notifyInfos, ScanTaskType::File);
    EXPECT_FALSE(isScanning);
}

HWTEST_F(GlobalScannerTest, IsGlobalScanning_010, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    for (int i = 0; i < 10; i++) {
        MediaNotifyInfo notifyInfo;
        notifyInfo.afterPath = "/data/test/path" + std::to_string(i);
        notifyInfo.objType = FileNotifyObjectType::FILE;
        notifyInfo.optType = FileNotifyOperationType::ADD;
        std::vector<MediaNotifyInfo> notifyInfos = {notifyInfo};
        bool isScanning = scanner.IsGlobalScanning(notifyInfos, ScanTaskType::File);
        EXPECT_FALSE(isScanning);
    }
}

HWTEST_F(GlobalScannerTest, Run_001, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/run_test_001";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, true);
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, Run_002, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/run_test_002";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, false);
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, Run_003, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::string testPath = "";
    scanner.RunLakeScan(testPath, true);
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, Run_004, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/run_test_004";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, true);
    scanner.InterruptScanner();
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, Run_005, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/run_test_005/subdir";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, true);
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, Run_006, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/run_test_006";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, true);
    scanner.RunLakeScan(testPath, false);
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, Run_007, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/run_test_007";
    std::filesystem::create_directories(testPath, ec);
    std::string subPath = testPath + "/sub1";
    std::filesystem::create_directories(subPath, ec);
    std::string subPath2 = testPath + "/sub2";
    std::filesystem::create_directories(subPath2, ec);
    scanner.RunLakeScan(testPath, true);
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, Run_008, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/run_test_008";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    scanner.InterruptScanner();
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, Run_009, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/run_test_009";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, true);
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, Run_010, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/run_test_010";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, true);
    bool isScanning = scanner.IsGlobalScanning({}, ScanTaskType::File);
    EXPECT_FALSE(isScanning);
}

HWTEST_F(GlobalScannerTest, Run_011, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/run_test_011";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, false);
    scanner.InterruptScanner();
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, Run_012, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/run_test_012";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, Run_013, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/run_test_013";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, true);
    scanner.RunLakeScan(testPath, true);
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, Run_014, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/run_test_014";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, false);
    scanner.RunLakeScan(testPath, false);
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, Run_015, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/run_test_015";
    std::filesystem::create_directories(testPath, ec);
    for (int i = 0; i < 5; i++) {
        std::string subPath = testPath + "/sub" + std::to_string(i);
        std::filesystem::create_directories(subPath, ec);
    }
    scanner.RunLakeScan(testPath, true);
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, IsForceScanning_001, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    bool isForce = scanner.IsForceScanning();
    EXPECT_FALSE(isForce);
}

HWTEST_F(GlobalScannerTest, IsForceScanning_002, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    scanner.InterruptScanner();
    bool isForce = scanner.IsForceScanning();
    EXPECT_FALSE(isForce);
}

HWTEST_F(GlobalScannerTest, IsForceScanning_003, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/force_test_003";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, true);
    bool isForce = scanner.IsForceScanning();
    EXPECT_FALSE(isForce);
}

HWTEST_F(GlobalScannerTest, IsForceScanning_004, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    bool isForce = scanner.IsForceScanning();
    EXPECT_FALSE(isForce);
}

HWTEST_F(GlobalScannerTest, IsForceScanning_005, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    for (int i = 0; i < 3; i++) {
        bool isForce = scanner.IsForceScanning();
        EXPECT_FALSE(isForce);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

HWTEST_F(GlobalScannerTest, IsForceScanning_006, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    scanner.InterruptScanner();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    bool isForce = scanner.IsForceScanning();
    EXPECT_FALSE(isForce);
}

HWTEST_F(GlobalScannerTest, IsForceScanning_007, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/force_test_007";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, false);
    bool isForce = scanner.IsForceScanning();
    EXPECT_FALSE(isForce);
}

HWTEST_F(GlobalScannerTest, IsForceScanning_008, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    scanner.InterruptScanner();
    scanner.InterruptScanner();
    bool isForce = scanner.IsForceScanning();
    EXPECT_FALSE(isForce);
}

HWTEST_F(GlobalScannerTest, IsForceScanning_009, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    ScannerStatus status = scanner.GetScannerStatus();
    bool isForce = scanner.IsForceScanning();
    EXPECT_FALSE(isForce);
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, IsForceScanning_010, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/force_test_010";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, true);
    scanner.InterruptScanner();
    bool isForce = scanner.IsForceScanning();
    EXPECT_FALSE(isForce);
}

HWTEST_F(GlobalScannerTest, ScannerStatus_001, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    ScannerStatus status1 = scanner.GetScannerStatus();
    ScannerStatus status2 = scanner.GetScannerStatus();
    EXPECT_EQ(status1, ScannerStatus::IDLE);
    EXPECT_EQ(status2, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, ScannerStatus_002, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_TRUE(status == ScannerStatus::IDLE ||
                status == ScannerStatus::GLOBAL_SCAN ||
                status == ScannerStatus::CHECK_SCAN);
}

HWTEST_F(GlobalScannerTest, ScannerStatus_003, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    scanner.InterruptScanner();
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, ScannerStatus_004, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/status_test_004";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, true);
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, ScannerStatus_005, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    for (int i = 0; i < 5; i++) {
        ScannerStatus status = scanner.GetScannerStatus();
        EXPECT_TRUE(status == ScannerStatus::IDLE);
    }
}

HWTEST_F(GlobalScannerTest, ScannerStatus_006, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/status_test_006";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, false);
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, ScannerStatus_007, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/status_test_007";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, true);
    scanner.InterruptScanner();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, ScannerStatus_008, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    scanner.InterruptScanner();
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, ScannerStatus_009, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/status_test_009";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, true);
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, ScannerStatus_010, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = "/data/test/path";
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    std::vector<MediaNotifyInfo> notifyInfos = {notifyInfo};
    scanner.IsGlobalScanning(notifyInfos, ScanTaskType::File);
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, MultipleOperations_001, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/Multi_test_001";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, true);
    scanner.InterruptScanner();
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, MultipleOperations_002, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/Multi_test_002";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, false);
    bool isForce = scanner.IsForceScanning();
    EXPECT_FALSE(isForce);
}

HWTEST_F(GlobalScannerTest, MultipleOperations_003, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/Multi_test_003";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, true);
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = testPath;
    notifyInfo.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo.optType = FileNotifyOperationType::MOD;
    std::vector<MediaNotifyInfo> notifyInfos = {notifyInfo};
    bool isScanning = scanner.IsGlobalScanning(notifyInfos, ScanTaskType::Folder);
    EXPECT_FALSE(isScanning);
}

HWTEST_F(GlobalScannerTest, MultipleOperations_004, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    scanner.InterruptScanner();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/Multi_test_004";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, true);
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, MultipleOperations_005, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/Multi_test_005";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, false);
    scanner.RunLakeScan(testPath, true);
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, MultipleOperations_006, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    for (int i = 0; i < 3; i++) {
        ScannerStatus status = scanner.GetScannerStatus();
        EXPECT_EQ(status, ScannerStatus::IDLE);
    }
}

HWTEST_F(GlobalScannerTest, MultipleOperations_007, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/Multi_test_007";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, true);
    for (int i = 0; i < 5; i++) {
        ScannerStatus status = scanner.GetScannerStatus();
        EXPECT_EQ(status, ScannerStatus::IDLE);
    }
}

HWTEST_F(GlobalScannerTest, MultipleOperations_008, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/Multi_test_008";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, false);
    scanner.InterruptScanner();
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, MultipleOperations_009, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/Multi_test_009";
    std::filesystem::create_directories(testPath, ec);
    for (int i = 0; i < 2; i++) {
        scanner.RunLakeScan(testPath, true);
        scanner.InterruptScanner();
    }
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, MultipleOperations_010, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/Multi_test_010";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, true);
    bool isForce = scanner.IsForceScanning();
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_FALSE(isForce);
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, EdgeCase_001, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::string testPath = "/";
    scanner.RunLakeScan(testPath, true);
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, EdgeCase_002, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::string testPath = "non_existent_path";
    scanner.RunLakeScan(testPath, true);
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, EdgeCase_003, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::string testPath = "   ";
    scanner.RunLakeScan(testPath, true);
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, EdgeCase_004, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = "";
    notifyInfo.objType = FileNotifyObjectType::UNDEFINED;
    notifyInfo.optType = FileNotifyOperationType::UNDEFINED;
    std::vector<MediaNotifyInfo> notifyInfos = {notifyInfo};
    bool isScanning = scanner.IsGlobalScanning(notifyInfos, ScanTaskType::File);
    EXPECT_FALSE(isScanning);
}

HWTEST_F(GlobalScannerTest, EdgeCase_005, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::string testPath = TEST_ROOT_PATH + "/edge_test_005";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, true);
    scanner.RunLakeScan(testPath, false);
    scanner.InterruptScanner();
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, EdgeCase_006, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/edge_test_006";
    std::filesystem::create_directories(testPath, ec);
    for (int i = 0; i < 10; i++) {
        scanner.InterruptScanner();
    }
    scanner.RunLakeScan(testPath, true);
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, EdgeCase_007, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/edge_test_007";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, true);
    scanner.InterruptScanner();
    bool isForce = scanner.IsForceScanning();
    EXPECT_FALSE(isForce);
}

HWTEST_F(GlobalScannerTest, EdgeCase_008, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/edge_test_008";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, false);
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = testPath;
    notifyInfo.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    std::vector<MediaNotifyInfo> notifyInfos = {notifyInfo};
    bool isScanning = scanner.IsGlobalScanning(notifyInfos, ScanTaskType::Folder);
    EXPECT_FALSE(isScanning);
}

HWTEST_F(GlobalScannerTest, EdgeCase_009, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/edge_test_009";
    std::filesystem::create_directories(testPath, ec);
    scanner.RunLakeScan(testPath, true);
    scanner.InterruptScanner();
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, EdgeCase_010, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/edge_test_010";
    std::filesystem::create_directories(testPath, ec);
    for (int i = 0; i < 3; i++) {
        scanner.RunLakeScan(testPath, true);
        ScannerStatus status = scanner.GetScannerStatus();
        EXPECT_EQ(status, ScannerStatus::IDLE);
    }
}

HWTEST_F(GlobalScannerTest, ThreadSafety_001, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/thread_test_001";
    std::filesystem::create_directories(testPath, ec);
    
    std::thread t1([&scanner, testPath]() {
        scanner.RunLakeScan(testPath, true);
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    scanner.InterruptScanner();
    t1.join();
    
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, ThreadSafety_002, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/thread_test_002";
    std::filesystem::create_directories(testPath, ec);
    
    std::thread t1([&scanner]() {
        scanner.InterruptScanner();
    });
    
    std::thread t2([&scanner]() {
        ScannerStatus status = scanner.GetScannerStatus();
        EXPECT_TRUE(status == ScannerStatus::IDLE);
    });
    
    t1.join();
    t2.join();
}

HWTEST_F(GlobalScannerTest, ThreadSafety_003, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/thread_test_003";
    std::filesystem::create_directories(testPath, ec);

    
    std::thread t2([&scanner]() {
        ScannerStatus status = scanner.GetScannerStatus();
        EXPECT_EQ(status, ScannerStatus::IDLE);
    });
    
    t2.join();
}

HWTEST_F(GlobalScannerTest, ThreadSafety_004, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/thread_test_004";
    std::filesystem::create_directories(testPath, ec);
    
    std::thread t1([&scanner, testPath]() {
        scanner.RunLakeScan(testPath, false);
    });
    
    std::thread t2([&scanner]() {
        bool isForce = scanner.IsForceScanning();
        EXPECT_FALSE(isForce);
    });
    
    t1.join();
    t2.join();
}

HWTEST_F(GlobalScannerTest, ThreadSafety_005, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/thread_test_005";
    std::filesystem::create_directories(testPath, ec);
    
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = testPath;
    notifyInfo.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    std::vector<MediaNotifyInfo> notifyInfos = {notifyInfo};
    
    std::thread t1([&scanner, notifyInfos]() {
        bool isScanning = scanner.IsGlobalScanning(notifyInfos, ScanTaskType::Folder);
        EXPECT_FALSE(isScanning);
    });
    
    std::thread t2([&scanner]() {
        scanner.InterruptScanner();
    });
    
    t1.join();
    t2.join();
}

HWTEST_F(GlobalScannerTest, ThreadSafety_006, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/thread_test_006";
    std::filesystem::create_directories(testPath, ec);

    std::thread t2([&scanner]() {
        scanner.InterruptScanner();
    });
    
    std::thread t3([&scanner]() {
        ScannerStatus status = scanner.GetScannerStatus();
        EXPECT_EQ(status, ScannerStatus::IDLE);
    });
    
    t2.join();
    t3.join();
}

HWTEST_F(GlobalScannerTest, ThreadSafety_007, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/thread_test_007";
    std::filesystem::create_directories(testPath, ec);
    
    std::thread t1([&scanner, testPath]() {
        scanner.RunLakeScan(testPath, true);
    });

    std::thread t3([&scanner]() {
        bool isForce = scanner.IsForceScanning();
        EXPECT_FALSE(isForce);
    });
    
    t1.join();
    t3.join();
}

HWTEST_F(GlobalScannerTest, ThreadSafety_008, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    
    std::thread t1([&scanner]() {
        scanner.InterruptScanner();
    });
    
    std::thread t2([&scanner]() {
        scanner.InterruptScanner();
    });
    
    std::thread t3([&scanner]() {
        scanner.InterruptScanner();
    });
    
    t1.join();
    t2.join();
    t3.join();
    
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, ThreadSafety_009, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/thread_test_009";
    std::filesystem::create_directories(testPath, ec);
    
    std::thread t1([&scanner, testPath]() {
        scanner.RunLakeScan(testPath, true);
    });
    
    std::thread t2([&scanner, testPath]() {
        scanner.RunLakeScan(testPath, false);
    });
    
    t1.join();
    t2.join();
    
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, ThreadSafety_010, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/thread_test_010";
    std::filesystem::create_directories(testPath, ec);
    
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = testPath;
    notifyInfo.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    std::vector<MediaNotifyInfo> notifyInfos = {notifyInfo};
    
    std::thread t1([&scanner, notifyInfos]() {
        scanner.IsGlobalScanning(notifyInfos, ScanTaskType::Folder);
    });
    
    std::thread t2([&scanner, notifyInfos]() {
        scanner.IsGlobalScanning(notifyInfos, ScanTaskType::File);
    });
    
    std::thread t3([&scanner]() {
        scanner.IsForceScanning();
    });
    
    t1.join();
    t2.join();
    t3.join();
}

HWTEST_F(GlobalScannerTest, Performance_001, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/perf_test_001";
    std::filesystem::create_directories(testPath, ec);
    
    auto start = std::chrono::high_resolution_clock::now();
    scanner.RunLakeScan(testPath, true);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_LT(duration, 5000);
}

HWTEST_F(GlobalScannerTest, Performance_002, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; i++) {
        scanner.GetScannerStatus();
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_LT(duration, 100);
}

HWTEST_F(GlobalScannerTest, Performance_003, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = "/data/test/path";
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    std::vector<MediaNotifyInfo> notifyInfos = {notifyInfo};
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; i++) {
        scanner.IsGlobalScanning(notifyInfos, ScanTaskType::File);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_LT(duration, 100);
}

HWTEST_F(GlobalScannerTest, Performance_005, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; i++) {
        scanner.InterruptScanner();
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_LT(duration, 100);
}

HWTEST_F(GlobalScannerTest, Performance_006, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; i++) {
        scanner.IsForceScanning();
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_LT(duration, 100);
}

HWTEST_F(GlobalScannerTest, Performance_007, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/perf_test_007";
    std::filesystem::create_directories(testPath, ec);
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10; i++) {
        scanner.RunLakeScan(testPath, true);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_LT(duration, 10000);
}

HWTEST_F(GlobalScannerTest, Performance_008, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/perf_test_008";
    std::filesystem::create_directories(testPath, ec);
    
    auto start = std::chrono::high_resolution_clock::now();
    scanner.RunLakeScan(testPath, true);
    scanner.RunLakeScan(testPath, false);
    scanner.RunLakeScan(testPath, true);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_LT(duration, 10000);
}

HWTEST_F(GlobalScannerTest, Performance_009, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = "/data/test/path";
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    std::vector<MediaNotifyInfo> notifyInfos = {notifyInfo};
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; i++) {
        scanner.IsGlobalScanning(notifyInfos, ScanTaskType::File);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_LT(duration, 1000);
}

HWTEST_F(GlobalScannerTest, Performance_010, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/perf_test_010";
    std::filesystem::create_directories(testPath, ec);
    
    for (int i = 0; i < 10; i++) {
        std::string subPath = testPath + "/sub" + std::to_string(i);
        std::filesystem::create_directories(subPath, ec);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    scanner.RunLakeScan(testPath, true);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_LT(duration, 5000);
}

HWTEST_F(GlobalScannerTest, Concurrency_001, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/concurrent_test_001";
    std::filesystem::create_directories(testPath, ec);
    
    std::thread t1([&scanner, testPath]() {
        scanner.RunLakeScan(testPath, true);
    });

    std::thread t3([&scanner]() {
        scanner.InterruptScanner();
    });

    t1.join();
    t3.join();
    
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, Concurrency_002, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/concurrent_test_002";
    std::filesystem::create_directories(testPath, ec);
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; i++) {
        threads.emplace_back([&scanner, testPath]() {
            scanner.RunLakeScan(testPath, true);
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, Concurrency_003, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&scanner]() {
            scanner.GetScannerStatus();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

HWTEST_F(GlobalScannerTest, Concurrency_004, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&scanner]() {
            scanner.InterruptScanner();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, Concurrency_006, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = "/data/test/path";
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    std::vector<MediaNotifyInfo> notifyInfos = {notifyInfo};
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&scanner, notifyInfos]() {
            scanner.IsGlobalScanning(notifyInfos, ScanTaskType::File);
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

HWTEST_F(GlobalScannerTest, Concurrency_007, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/concurrent_test_007";
    std::filesystem::create_directories(testPath, ec);
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; i++) {
        threads.emplace_back([&scanner, testPath]() {
            scanner.RunLakeScan(testPath, true);
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, Concurrency_008, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; i++) {
        threads.emplace_back([&scanner]() {
            scanner.IsForceScanning();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

HWTEST_F(GlobalScannerTest, Concurrency_010, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/concurrent_test_010";
    std::filesystem::create_directories(testPath, ec);
    
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = testPath;
    notifyInfo.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    std::vector<MediaNotifyInfo> notifyInfos = {notifyInfo};
    
    std::thread t1([&scanner, testPath]() {
        scanner.RunLakeScan(testPath, true);
    });
    
    std::thread t2([&scanner, notifyInfos]() {
        scanner.IsGlobalScanning(notifyInfos, ScanTaskType::Folder);
    });
    
    std::thread t3([&scanner]() {
        scanner.IsForceScanning();
    });
    
    t1.join();
    t2.join();
    t3.join();
}

HWTEST_F(GlobalScannerTest, StressTest_001, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/stress_test_001";
    std::filesystem::create_directories(testPath, ec);
    
    for (int i = 0; i < 100; i++) {
        scanner.RunLakeScan(testPath, true);
        ScannerStatus status = scanner.GetScannerStatus();
        EXPECT_EQ(status, ScannerStatus::IDLE);
    }
}

HWTEST_F(GlobalScannerTest, StressTest_002, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    
    for (int i = 0; i < 1000; i++) {
        ScannerStatus status = scanner.GetScannerStatus();
        EXPECT_TRUE(status == ScannerStatus::IDLE);
    }
}

HWTEST_F(GlobalScannerTest, StressTest_003, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    
    for (int i = 0; i < 1000; i++) {
        scanner.InterruptScanner();
    }
    
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, StressTest_004, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    
    for (int i = 0; i < 1000; i++) {
        bool isForce = scanner.IsForceScanning();
        EXPECT_FALSE(isForce);
    }
}

HWTEST_F(GlobalScannerTest, StressTest_005, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = "/data/test/path";
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    std::vector<MediaNotifyInfo> notifyInfos = {notifyInfo};
    
    for (int i = 0; i < 1000; i++) {
        bool isScanning = scanner.IsGlobalScanning(notifyInfos, ScanTaskType::File);
        EXPECT_FALSE(isScanning);
    }
}

HWTEST_F(GlobalScannerTest, StressTest_006, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    
    for (int i = 0; i < 500; i++) {
        ScannerStatus status = scanner.GetScannerStatus();
        EXPECT_EQ(status, ScannerStatus::IDLE);
    }
}

HWTEST_F(GlobalScannerTest, StressTest_007, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    
    for (int i = 0; i < 500; i++) {
        scanner.InterruptScanner();
        bool isForce = scanner.IsForceScanning();
        EXPECT_FALSE(isForce);
    }
}

HWTEST_F(GlobalScannerTest, MixedOperations_001, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/mixed_test_001";
    std::filesystem::create_directories(testPath, ec);
    
    scanner.RunLakeScan(testPath, true);
    scanner.InterruptScanner();
    
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, MixedOperations_002, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/mixed_test_002";
    std::filesystem::create_directories(testPath, ec);
    
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = testPath;
    notifyInfo.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    std::vector<MediaNotifyInfo> notifyInfos = {notifyInfo};
    
    scanner.IsGlobalScanning(notifyInfos, ScanTaskType::Folder);
    scanner.RunLakeScan(testPath, false);
    
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, MixedOperations_003, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/mixed_test_003";
    std::filesystem::create_directories(testPath, ec);
    
    scanner.RunLakeScan(testPath, true);
    scanner.InterruptScanner();
    scanner.RunLakeScan(testPath, false);
    
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, MixedOperations_004, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/mixed_test_004";
    std::filesystem::create_directories(testPath, ec);
    
    scanner.RunLakeScan(testPath, true);
    scanner.RunLakeScan(testPath, false);
    scanner.InterruptScanner();
    
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, MixedOperations_005, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/mixed_test_005";
    std::filesystem::create_directories(testPath, ec);
    
    for (int i = 0; i < 10; i++) {
        scanner.RunLakeScan(testPath, true);
        scanner.InterruptScanner();
    }
    
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, MixedOperations_006, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/mixed_test_007";
    std::filesystem::create_directories(testPath, ec);
    
    scanner.RunLakeScan(testPath, true);
    scanner.InterruptScanner();
    scanner.RunLakeScan(testPath, false);
    scanner.InterruptScanner();
    
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, MixedOperations_007, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/mixed_test_008";
    std::filesystem::create_directories(testPath, ec);
    
    for (int i = 0; i < 5; i++) {
        scanner.RunLakeScan(testPath, true);
        scanner.InterruptScanner();
        scanner.RunLakeScan(testPath, false);
    }
    
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, MixedOperations_008, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/mixed_test_009";
    std::filesystem::create_directories(testPath, ec);
    
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = testPath;
    notifyInfo.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    std::vector<MediaNotifyInfo> notifyInfos = {notifyInfo};
    
    scanner.RunLakeScan(testPath, true);
    scanner.IsGlobalScanning(notifyInfos, ScanTaskType::Folder);
    scanner.InterruptScanner();
    
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}

HWTEST_F(GlobalScannerTest, MixedOperations_009, TestSize.Level1)
{
    GlobalScanner& scanner = GlobalScanner::GetInstance();
    std::error_code ec;
    std::string testPath = TEST_ROOT_PATH + "/mixed_test_010";
    std::filesystem::create_directories(testPath, ec);
    
    for (int i = 0; i < 3; i++) {
        scanner.RunLakeScan(testPath, true);
        scanner.RunLakeScan(testPath, false);
        scanner.InterruptScanner();
        scanner.GetScannerStatus();
        scanner.IsForceScanning();
    }
    
    ScannerStatus status = scanner.GetScannerStatus();
    EXPECT_EQ(status, ScannerStatus::IDLE);
}
} // namespace Media
} // namespace OHOS

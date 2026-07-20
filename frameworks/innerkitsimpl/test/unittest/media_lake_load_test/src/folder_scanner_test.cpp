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

#define MLOG_TAG "FolderScannerTest"

#include "folder_scanner_test.h"

#include <filesystem>
#include <memory>
#include <thread>
#include <chrono>
#include <fstream>

#include "folder_scanner.h"
#include "lake_folder_parser.h"
#include "lake_file_parser.h"
#include "folder_scanner_utils.h"
#include "media_file_notify_info.h"
#include "file_const.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
using namespace testing::ext;

static const std::string TEST_ROOT_PATH = "/data/test/folder_scanner";

void FolderScannerTest::SetUpTestCase()
{
    std::error_code ec;
    if (!std::filesystem::exists(TEST_ROOT_PATH, ec)) {
        std::filesystem::create_directories(TEST_ROOT_PATH, ec);
    }
}

void FolderScannerTest::TearDownTestCase()
{
    std::error_code ec;
    if (std::filesystem::exists(TEST_ROOT_PATH, ec)) {
        std::filesystem::remove_all(TEST_ROOT_PATH, ec);
    }
}

void FolderScannerTest::SetUp()
{
}

void FolderScannerTest::TearDown()
{
}

HWTEST_F(FolderScannerTest, ConstructorWithPath_001, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/constructor_test_001";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    int32_t albumId = scanner.GetAlbumId();
    EXPECT_EQ(albumId, -1);
}

HWTEST_F(FolderScannerTest, ConstructorWithPath_002, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/constructor_test_002";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::INCREMENT);
    int32_t addCount = scanner.GetAddCount();
    EXPECT_EQ(addCount, 0);
}

HWTEST_F(FolderScannerTest, ConstructorWithPath_003, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/constructor_test_003";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::VALIDATION);
    int32_t updateCount = scanner.GetUpdateCount();
    EXPECT_EQ(updateCount, 0);
}

HWTEST_F(FolderScannerTest, ConstructorWithPath_004, TestSize.Level1)
{
    std::string testPath = "";
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    int32_t albumId = scanner.GetAlbumId();
    EXPECT_EQ(albumId, -1);
}

HWTEST_F(FolderScannerTest, ConstructorWithPath_005, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/constructor_test_005";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    std::vector<int32_t> fileIds;
    scanner.GetFileIds(fileIds);
    EXPECT_TRUE(fileIds.empty());
}

HWTEST_F(FolderScannerTest, ConstructorWithNotifyInfo_001, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = TEST_ROOT_PATH + "/notify_test_001";
    notifyInfo.beforePath = TEST_ROOT_PATH + "/notify_test_001_old";
    notifyInfo.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    
    std::error_code ec;
    std::filesystem::create_directories(notifyInfo.afterPath, ec);
    
    FolderScanner scanner(notifyInfo, ScanMode::FULL);
    int32_t albumId = scanner.GetAlbumId();
    EXPECT_EQ(albumId, -1);
}

HWTEST_F(FolderScannerTest, ConstructorWithNotifyInfo_002, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = TEST_ROOT_PATH + "/notify_test_002";
    notifyInfo.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo.optType = FileNotifyOperationType::MOD;
    
    std::error_code ec;
    std::filesystem::create_directories(notifyInfo.afterPath, ec);
    
    FolderScanner scanner(notifyInfo, ScanMode::INCREMENT);
    int32_t addCount = scanner.GetAddCount();
    EXPECT_EQ(addCount, 0);
}

HWTEST_F(FolderScannerTest, ConstructorWithNotifyInfo_003, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = TEST_ROOT_PATH + "/notify_test_003";
    notifyInfo.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo.optType = FileNotifyOperationType::DEL;
    
    std::error_code ec;
    std::filesystem::create_directories(notifyInfo.afterPath, ec);
    
    FolderScanner scanner(notifyInfo, ScanMode::VALIDATION);
    int32_t updateCount = scanner.GetUpdateCount();
    EXPECT_EQ(updateCount, 0);
}

HWTEST_F(FolderScannerTest, ConstructorWithNotifyInfo_004, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = "";
    notifyInfo.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    
    FolderScanner scanner(notifyInfo, ScanMode::FULL);
    int32_t albumId = scanner.GetAlbumId();
    EXPECT_EQ(albumId, -1);
}

HWTEST_F(FolderScannerTest, ConstructorWithNotifyInfo_005, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = TEST_ROOT_PATH + "/notify_test_005";
    notifyInfo.beforePath = TEST_ROOT_PATH + "/notify_test_005_old";
    notifyInfo.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    
    std::error_code ec;
    std::filesystem::create_directories(notifyInfo.afterPath, ec);
    
    FolderScanner scanner(notifyInfo, ScanMode::FULL);
    std::vector<int32_t> fileIds;
    scanner.GetFileIds(fileIds);
    EXPECT_TRUE(fileIds.empty());
}

HWTEST_F(FolderScannerTest, GetAlbumId_001, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/get_album_id_001";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    int32_t albumId = scanner.GetAlbumId();
    EXPECT_EQ(albumId, -1);
}

HWTEST_F(FolderScannerTest, GetAlbumId_002, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = TEST_ROOT_PATH + "/get_album_id_002";
    notifyInfo.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    
    std::error_code ec;
    std::filesystem::create_directories(notifyInfo.afterPath, ec);
    
    FolderScanner scanner(notifyInfo, ScanMode::INCREMENT);
    int32_t albumId = scanner.GetAlbumId();
    EXPECT_EQ(albumId, -1);
}

HWTEST_F(FolderScannerTest, GetAddCount_001, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/get_add_count_001";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    int32_t addCount = scanner.GetAddCount();
    EXPECT_EQ(addCount, 0);
}

HWTEST_F(FolderScannerTest, GetAddCount_002, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = TEST_ROOT_PATH + "/get_add_count_002";
    notifyInfo.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo.optType = FileNotifyOperationType::MOD;
    
    std::error_code ec;
    std::filesystem::create_directories(notifyInfo.afterPath, ec);
    
    FolderScanner scanner(notifyInfo, ScanMode::INCREMENT);
    int32_t addCount = scanner.GetAddCount();
    EXPECT_EQ(addCount, 0);
}

HWTEST_F(FolderScannerTest, GetUpdateCount_001, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/get_update_count_001";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    int32_t updateCount = scanner.GetUpdateCount();
    EXPECT_EQ(updateCount, 0);
}

HWTEST_F(FolderScannerTest, GetUpdateCount_002, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = TEST_ROOT_PATH + "/get_update_count_002";
    notifyInfo.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    
    std::error_code ec;
    std::filesystem::create_directories(notifyInfo.afterPath, ec);
    
    FolderScanner scanner(notifyInfo, ScanMode::VALIDATION);
    int32_t updateCount = scanner.GetUpdateCount();
    EXPECT_EQ(updateCount, 0);
}

HWTEST_F(FolderScannerTest, GetFileIds_001, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/get_file_ids_001";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    std::vector<int32_t> fileIds;
    scanner.GetFileIds(fileIds);
    EXPECT_TRUE(fileIds.empty());
}

HWTEST_F(FolderScannerTest, GetFileIds_002, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = TEST_ROOT_PATH + "/get_file_ids_002";
    notifyInfo.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo.optType = FileNotifyOperationType::MOD;
    
    std::error_code ec;
    std::filesystem::create_directories(notifyInfo.afterPath, ec);
    
    FolderScanner scanner(notifyInfo, ScanMode::INCREMENT);
    std::vector<int32_t> fileIds;
    scanner.GetFileIds(fileIds);
    EXPECT_TRUE(fileIds.empty());
}

HWTEST_F(FolderScannerTest, GetFileIds_003, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/get_file_ids_003";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    std::vector<int32_t> fileIds1;
    scanner.GetFileIds(fileIds1);
    
    std::vector<int32_t> fileIds2;
    scanner.GetFileIds(fileIds2);
    
    EXPECT_TRUE(fileIds1.empty());
    EXPECT_TRUE(fileIds2.empty());
}

HWTEST_F(FolderScannerTest, IsScanFolderFile_004, TestSize.Level1)
{
    std::string testPath = "";
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    bool isScan = scanner.IsScanFolderFile();
    EXPECT_TRUE(isScan);
}

HWTEST_F(FolderScannerTest, Run_001, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/run_test_001";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    int32_t ret = scanner.Run();
    EXPECT_EQ(ret, ERR_SUCCESS);
}

HWTEST_F(FolderScannerTest, Run_002, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/run_test_002";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::INCREMENT);
    int32_t ret = scanner.Run();
    EXPECT_EQ(ret, ERR_SUCCESS);
}

HWTEST_F(FolderScannerTest, Run_003, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/run_test_003";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::VALIDATION);
    int32_t ret = scanner.Run();
    EXPECT_EQ(ret, ERR_SUCCESS);
}

HWTEST_F(FolderScannerTest, Run_004, TestSize.Level1)
{
    std::string testPath = "";
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    int32_t ret = scanner.Run();
    EXPECT_EQ(ret, ERR_INCORRECT_PATH);
}

HWTEST_F(FolderScannerTest, Run_005, TestSize.Level1)
{
    std::string testPath = "/non_existent_path";
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    int32_t ret = scanner.Run();
    EXPECT_EQ(ret, ERR_INCORRECT_PATH);
}

HWTEST_F(FolderScannerTest, Run_006, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/run_test_006";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    std::string subPath = testPath + "/subdir";
    std::filesystem::create_directories(subPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    int32_t ret = scanner.Run();
    EXPECT_EQ(ret, ERR_SUCCESS);
}

HWTEST_F(FolderScannerTest, Run_007, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/run_test_007";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    for (int i = 0; i < 5; i++) {
        std::string subPath = testPath + "/subdir" + std::to_string(i);
        std::filesystem::create_directories(subPath, ec);
    }
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    int32_t ret = scanner.Run();
    EXPECT_EQ(ret, ERR_SUCCESS);
}

HWTEST_F(FolderScannerTest, Run_008, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/run_test_008";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    std::string subPath = testPath + "/subdir1";
    std::filesystem::create_directories(subPath, ec);
    std::string subPath2 = testPath + "/subdir2";
    std::filesystem::create_directories(subPath2, ec);
    
    FolderScanner scanner(testPath, ScanMode::INCREMENT);
    int32_t ret = scanner.Run();
    EXPECT_EQ(ret, ERR_SUCCESS);
}

HWTEST_F(FolderScannerTest, Run_009, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = TEST_ROOT_PATH + "/run_test_009";
    notifyInfo.beforePath = TEST_ROOT_PATH + "/run_test_009_old";
    notifyInfo.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    
    std::error_code ec;
    std::filesystem::create_directories(notifyInfo.afterPath, ec);
    
    FolderScanner scanner(notifyInfo, ScanMode::FULL);
    int32_t ret = scanner.Run();
    EXPECT_EQ(ret, ERR_SUCCESS);
}

HWTEST_F(FolderScannerTest, Run_010, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = TEST_ROOT_PATH + "/run_test_010";
    notifyInfo.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo.optType = FileNotifyOperationType::MOD;
    
    std::error_code ec;
    std::filesystem::create_directories(notifyInfo.afterPath, ec);
    
    FolderScanner scanner(notifyInfo, ScanMode::INCREMENT);
    int32_t ret = scanner.Run();
    EXPECT_EQ(ret, ERR_SUCCESS);
}

HWTEST_F(FolderScannerTest, ScanCurrentDirectory_008, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/scan_current_dir_008";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    std::queue<std::string> subDirQueue1;
    scanner.ScanCurrentDirectory(subDirQueue1);
    
    std::queue<std::string> subDirQueue2;
    scanner.ScanCurrentDirectory(subDirQueue2);
    
    EXPECT_EQ(subDirQueue1.size(), subDirQueue2.size());
}

HWTEST_F(FolderScannerTest, BuildSubDirFolderScanner_001, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/build_subdir_001";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    std::queue<std::string> subDirQueue;
    scanner.ScanCurrentDirectory(subDirQueue);
    
    std::string subDirPath = testPath + "/subdir";
    std::filesystem::create_directories(subDirPath, ec);
    
    FolderScanner subScanner = scanner.BuildSubDirFolderScanner(subDirPath);
    int32_t albumId = subScanner.GetAlbumId();
    EXPECT_EQ(albumId, -1);
}

HWTEST_F(FolderScannerTest, BuildSubDirFolderScanner_002, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = TEST_ROOT_PATH + "/build_subdir_002";
    notifyInfo.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    
    std::error_code ec;
    std::filesystem::create_directories(notifyInfo.afterPath, ec);
    
    FolderScanner scanner(notifyInfo, ScanMode::FULL);
    std::queue<std::string> subDirQueue;
    scanner.ScanCurrentDirectory(subDirQueue);
    
    std::string subDirPath = notifyInfo.afterPath + "/subdir";
    std::filesystem::create_directories(subDirPath, ec);
    
    FolderScanner subScanner = scanner.BuildSubDirFolderScanner(subDirPath);
    int32_t addCount = subScanner.GetAddCount();
    EXPECT_EQ(addCount, 0);
}

HWTEST_F(FolderScannerTest, BuildSubDirFolderScanner_003, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/build_subdir_003";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::INCREMENT);
    std::queue<std::string> subDirQueue;
    scanner.ScanCurrentDirectory(subDirQueue);
    
    std::string subDirPath = testPath + "/subdir";
    std::filesystem::create_directories(subDirPath, ec);
    
    FolderScanner subScanner = scanner.BuildSubDirFolderScanner(subDirPath);
    int32_t updateCount = subScanner.GetUpdateCount();
    EXPECT_EQ(updateCount, 0);
}

HWTEST_F(FolderScannerTest, BuildSubDirFolderScanner_004, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/build_subdir_004";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::VALIDATION);
    std::string subDirPath = testPath + "/subdir";
    std::filesystem::create_directories(subDirPath, ec);
    
    FolderScanner subScanner = scanner.BuildSubDirFolderScanner(subDirPath);
    std::vector<int32_t> fileIds;
    subScanner.GetFileIds(fileIds);
    EXPECT_TRUE(fileIds.empty());
}

HWTEST_F(FolderScannerTest, BuildSubDirFolderScanner_005, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/build_subdir_005";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    std::queue<std::string> subDirQueue;
    scanner.ScanCurrentDirectory(subDirQueue);
    
    std::string subDirPath = testPath + "/subdir1";
    std::filesystem::create_directories(subDirPath, ec);
    std::string subDirPath2 = testPath + "/subdir2";
    std::filesystem::create_directories(subDirPath2, ec);
    
    FolderScanner subScanner1 = scanner.BuildSubDirFolderScanner(subDirPath);
    FolderScanner subScanner2 = scanner.BuildSubDirFolderScanner(subDirPath2);
    
    int32_t albumId1 = subScanner1.GetAlbumId();
    int32_t albumId2 = subScanner2.GetAlbumId();
    EXPECT_EQ(albumId1, albumId2);
}

HWTEST_F(FolderScannerTest, EdgeCase_001, TestSize.Level1)
{
    std::string testPath = "/";
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    int32_t ret = scanner.Run();
    EXPECT_EQ(ret, ERR_SUCCESS);
}

HWTEST_F(FolderScannerTest, EdgeCase_002, TestSize.Level1)
{
    std::string testPath = "   ";
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    int32_t ret = scanner.Run();
    EXPECT_EQ(ret, ERR_INCORRECT_PATH);
}

HWTEST_F(FolderScannerTest, EdgeCase_003, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/edge_test_003";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    for (int i = 0; i < 100; i++) {
        int32_t albumId = scanner.GetAlbumId();
        int32_t addCount = scanner.GetAddCount();
        int32_t updateCount = scanner.GetUpdateCount();
        EXPECT_EQ(albumId, -1);
        EXPECT_EQ(addCount, 0);
        EXPECT_EQ(updateCount, 0);
    }
}

HWTEST_F(FolderScannerTest, EdgeCase_004, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/edge_test_004";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::INCREMENT);
    std::vector<int32_t> fileIds;
    for (int i = 0; i < 100; i++) {
        scanner.GetFileIds(fileIds);
        EXPECT_TRUE(fileIds.empty());
    }
}

HWTEST_F(FolderScannerTest, EdgeCase_005, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/edge_test_005";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::VALIDATION);
    for (int i = 0; i < 100; i++) {
        bool isScan = scanner.IsScanFolderFile();
        EXPECT_TRUE(isScan);
    }
}

HWTEST_F(FolderScannerTest, EdgeCase_007, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = "";
    notifyInfo.objType = FileNotifyObjectType::UNDEFINED;
    notifyInfo.optType = FileNotifyOperationType::UNDEFINED;
    
    FolderScanner scanner(notifyInfo, ScanMode::FULL);
    int32_t albumId = scanner.GetAlbumId();
    EXPECT_EQ(albumId, -1);
}

HWTEST_F(FolderScannerTest, EdgeCase_008, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/edge_test_008";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    for (int i = 0; i < 10; i++) {
        std::string subPath = testPath + "/subdir" + std::to_string(i);
        std::filesystem::create_directories(subPath, ec);
    }
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    int32_t ret = scanner.Run();
    EXPECT_EQ(ret, ERR_SUCCESS);
}

HWTEST_F(FolderScannerTest, EdgeCase_009, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/edge_test_009";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    scanner.Run();
    scanner.Run();
    
    int32_t albumId = scanner.GetAlbumId();
    int32_t addCount = scanner.GetAddCount();
    int32_t updateCount = scanner.GetUpdateCount();
    
    EXPECT_EQ(albumId, -1);
    EXPECT_EQ(addCount, 0);
    EXPECT_EQ(updateCount, 0);
}

HWTEST_F(FolderScannerTest, EdgeCase_010, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = TEST_ROOT_PATH + "/edge_test_010";
    notifyInfo.beforePath = "";
    notifyInfo.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    
    std::error_code ec;
    std::filesystem::create_directories(notifyInfo.afterPath, ec);
    
    FolderScanner scanner(notifyInfo, ScanMode::FULL);
    int32_t ret = scanner.Run();
    EXPECT_EQ(ret, ERR_SUCCESS);
}

HWTEST_F(FolderScannerTest, Performance_001, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/perf_test_001";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; i++) {
        scanner.GetAlbumId();
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_LT(duration, 100);
}

HWTEST_F(FolderScannerTest, Performance_002, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/perf_test_002";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; i++) {
        scanner.GetAddCount();
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_LT(duration, 100);
}

HWTEST_F(FolderScannerTest, Performance_003, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/perf_test_003";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; i++) {
        scanner.GetUpdateCount();
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_LT(duration, 100);
}

HWTEST_F(FolderScannerTest, Performance_004, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/perf_test_004";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; i++) {
        scanner.IsScanFolderFile();
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_LT(duration, 100);
}

HWTEST_F(FolderScannerTest, Performance_005, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/perf_test_005";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    std::vector<int32_t> fileIds;
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; i++) {
        scanner.GetFileIds(fileIds);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_LT(duration, 100);
}

HWTEST_F(FolderScannerTest, Performance_006, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/perf_test_006";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    std::string subPath = testPath + "/subdir";
    std::filesystem::create_directories(subPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    std::queue<std::string> subDirQueue;
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10; i++) {
        scanner.ScanCurrentDirectory(subDirQueue);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_LT(duration, 1000);
}

HWTEST_F(FolderScannerTest, Performance_007, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/perf_test_007";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    std::string subDirPath = testPath + "/subdir";
    std::filesystem::create_directories(subDirPath, ec);
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; i++) {
        scanner.BuildSubDirFolderScanner(subDirPath);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_LT(duration, 100);
}

HWTEST_F(FolderScannerTest, Performance_008, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/perf_test_008";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    std::string filePath = testPath + "/test.jpg";
    std::ofstream file(filePath);
    file.close();
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; i++) {
        std::unique_ptr<FileParser> fileParser = scanner.BuildFileParser(filePath);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_LT(duration, 100);
}

HWTEST_F(FolderScannerTest, Performance_009, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/perf_test_009";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    
    auto start = std::chrono::high_resolution_clock::now();
    int32_t ret = scanner.Run();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_EQ(ret, ERR_SUCCESS);
    EXPECT_LT(duration, 5000);
}

HWTEST_F(FolderScannerTest, Performance_010, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = TEST_ROOT_PATH + "/perf_test_010";
    notifyInfo.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    
    std::error_code ec;
    std::filesystem::create_directories(notifyInfo.afterPath, ec);
    
    FolderScanner scanner(notifyInfo, ScanMode::FULL);
    
    auto start = std::chrono::high_resolution_clock::now();
    int32_t ret = scanner.Run();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_EQ(ret, ERR_SUCCESS);
    EXPECT_LT(duration, 5000);
}

HWTEST_F(FolderScannerTest, ThreadSafety_001, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/thread_test_001";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    
    int32_t albumId = 0;
    int32_t addCount = 0;
    int32_t updateCount = 0;
    
    std::thread t1([&scanner, &albumId]() {
        albumId = scanner.GetAlbumId();
    });
    
    std::thread t2([&scanner, &addCount]() {
        addCount = scanner.GetAddCount();
    });
    
    std::thread t3([&scanner, &updateCount]() {
        updateCount = scanner.GetUpdateCount();
    });
    
    t1.join();
    t2.join();
    t3.join();
    
    EXPECT_EQ(albumId, -1);
    EXPECT_EQ(addCount, 0);
    EXPECT_EQ(updateCount, 0);
}

HWTEST_F(FolderScannerTest, ThreadSafety_002, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/thread_test_002";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    
    bool isScan = true;
    std::vector<int32_t> fileIds;
    
    std::thread t1([&scanner, &isScan]() {
        isScan = scanner.IsScanFolderFile();
    });
    
    std::thread t2([&scanner, &fileIds]() {
        scanner.GetFileIds(fileIds);
    });
    
    t1.join();
    t2.join();
    
    EXPECT_TRUE(isScan);
    EXPECT_TRUE(fileIds.empty());
}

HWTEST_F(FolderScannerTest, ThreadSafety_004, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/thread_test_004";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    
    std::vector<std::thread> threads;
    std::vector<int32_t> results(10);
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&scanner, &results, i]() {
            results[i] = scanner.GetAlbumId();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    for (int32_t result : results) {
        EXPECT_EQ(result, -1);
    }
}

HWTEST_F(FolderScannerTest, ThreadSafety_005, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/thread_test_005";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    
    std::vector<std::thread> threads;
    std::vector<int32_t> results(10);
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&scanner, &results, i]() {
            results[i] = scanner.GetAddCount();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    for (int32_t result : results) {
        EXPECT_EQ(result, 0);
    }
}

HWTEST_F(FolderScannerTest, ThreadSafety_006, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/thread_test_006";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    
    std::vector<std::thread> threads;
    std::vector<int32_t> results(10);
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&scanner, &results, i]() {
            results[i] = scanner.GetUpdateCount();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    for (int32_t result : results) {
        EXPECT_EQ(result, 0);
    }
}

HWTEST_F(FolderScannerTest, ThreadSafety_007, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/thread_test_007";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    
    std::vector<std::thread> threads;
    std::vector<bool> results(10);
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&scanner, &results, i]() {
            results[i] = scanner.IsScanFolderFile();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    for (bool result : results) {
        EXPECT_TRUE(result);
    }
}

HWTEST_F(FolderScannerTest, ThreadSafety_009, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/thread_test_009";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    std::string subDirPath = testPath + "/subdir";
    std::filesystem::create_directories(subDirPath, ec);
    
    std::vector<std::thread> threads;
    std::vector<int32_t> results(10);
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&scanner, subDirPath, &results, i]() {
            FolderScanner subScanner = scanner.BuildSubDirFolderScanner(subDirPath);
            results[i] = subScanner.GetAlbumId();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    for (int32_t result : results) {
        EXPECT_EQ(result, -1);
    }
}

HWTEST_F(FolderScannerTest, ThreadSafety_010, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/thread_test_010";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    std::string filePath = testPath + "/test.jpg";
    std::ofstream file(filePath);
    file.close();
    
    std::vector<std::thread> threads;
    std::vector<bool> results(10);
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&scanner, filePath, &results, i]() {
            std::unique_ptr<FileParser> fileParser = scanner.BuildFileParser(filePath);
            if (fileParser == nullptr) {
                return;
            }
            results[i] = fileParser->IsFileValidAsset();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    for (bool result : results) {
        EXPECT_FALSE(result);
    }
}

HWTEST_F(FolderScannerTest, MixedOperations_001, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/mixed_test_001";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    
    scanner.GetAlbumId();
    scanner.GetAddCount();
    scanner.GetUpdateCount();
    scanner.IsScanFolderFile();
    
    std::vector<int32_t> fileIds;
    scanner.GetFileIds(fileIds);
    
    EXPECT_TRUE(fileIds.empty());
}

HWTEST_F(FolderScannerTest, MixedOperations_002, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = TEST_ROOT_PATH + "/mixed_test_002";
    notifyInfo.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    
    std::error_code ec;
    std::filesystem::create_directories(notifyInfo.afterPath, ec);
    
    FolderScanner scanner(notifyInfo, ScanMode::FULL);
    
    scanner.GetAlbumId();
    scanner.GetAddCount();
    scanner.GetUpdateCount();
    scanner.IsScanFolderFile();
    
    std::vector<int32_t> fileIds;
    scanner.GetFileIds(fileIds);
    
    EXPECT_TRUE(fileIds.empty());
}

HWTEST_F(FolderScannerTest, MixedOperations_003, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/mixed_test_003";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::INCREMENT);
    
    for (int i = 0; i < 10; i++) {
        int32_t albumId = scanner.GetAlbumId();
        int32_t addCount = scanner.GetAddCount();
        int32_t updateCount = scanner.GetUpdateCount();
        EXPECT_EQ(albumId, -1);
        EXPECT_EQ(addCount, 0);
        EXPECT_EQ(updateCount, 0);
    }
}

HWTEST_F(FolderScannerTest, MixedOperations_006, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/mixed_test_006";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    std::string subPath = testPath + "/subdir";
    std::filesystem::create_directories(subPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    std::queue<std::string> subDirQueue;
    scanner.ScanCurrentDirectory(subDirQueue);
    
    FolderScanner subScanner = scanner.BuildSubDirFolderScanner(subPath);
    
    int32_t albumId1 = scanner.GetAlbumId();
    int32_t albumId2 = subScanner.GetAlbumId();
    
    EXPECT_EQ(albumId1, -1);
    EXPECT_EQ(albumId2, -1);
}

HWTEST_F(FolderScannerTest, MixedOperations_007, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/mixed_test_007";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    
    int32_t addCount1 = scanner.GetAddCount();
    int32_t updateCount1 = scanner.GetUpdateCount();
    
    scanner.Run();
    
    int32_t addCount2 = scanner.GetAddCount();
    int32_t updateCount2 = scanner.GetUpdateCount();
    
    EXPECT_EQ(addCount1, 0);
    EXPECT_EQ(updateCount1, 0);
}

HWTEST_F(FolderScannerTest, MixedOperations_008, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/mixed_test_008";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    for (int i = 0; i < 5; i++) {
        std::string subPath = testPath + "/subdir" + std::to_string(i);
        std::filesystem::create_directories(subPath, ec);
    }
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    int32_t ret = scanner.Run();
    
    EXPECT_EQ(ret, ERR_SUCCESS);
}

HWTEST_F(FolderScannerTest, MixedOperations_009, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.afterPath = TEST_ROOT_PATH + "/mixed_test_009";
    notifyInfo.objType = FileNotifyObjectType::DIRECTORY;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    
    std::error_code ec;
    std::filesystem::create_directories(notifyInfo.afterPath, ec);
    
    FolderScanner scanner(notifyInfo, ScanMode::FULL);
    
    int32_t ret = scanner.Run();
    
    EXPECT_EQ(ret, ERR_SUCCESS);
}

HWTEST_F(FolderScannerTest, MixedOperations_010, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/mixed_test_010";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    std::string subPath = testPath + "/subdir";
    std::filesystem::create_directories(subPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    std::queue<std::string> subDirQueue;
    scanner.ScanCurrentDirectory(subDirQueue);
    
    FolderScanner subScanner = scanner.BuildSubDirFolderScanner(subPath);
    
    int32_t ret1 = scanner.Run();
    int32_t ret2 = subScanner.Run();
    
    EXPECT_EQ(ret1, ERR_SUCCESS);
    EXPECT_EQ(ret2, ERR_SUCCESS);
}

HWTEST_F(FolderScannerTest, StressTest_001, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/stress_test_001";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    
    for (int i = 0; i < 1000; i++) {
        int32_t albumId = scanner.GetAlbumId();
        EXPECT_EQ(albumId, -1);
    }
}

HWTEST_F(FolderScannerTest, StressTest_002, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/stress_test_002";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    
    for (int i = 0; i < 1000; i++) {
        int32_t addCount = scanner.GetAddCount();
        EXPECT_EQ(addCount, 0);
    }
}

HWTEST_F(FolderScannerTest, StressTest_003, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/stress_test_003";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    
    for (int i = 0; i < 1000; i++) {
        int32_t updateCount = scanner.GetUpdateCount();
        EXPECT_EQ(updateCount, 0);
    }
}

HWTEST_F(FolderScannerTest, StressTest_004, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/stress_test_004";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    
    for (int i = 0; i < 1000; i++) {
        bool isScan = scanner.IsScanFolderFile();
        EXPECT_TRUE(isScan);
    }
}

HWTEST_F(FolderScannerTest, StressTest_005, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/stress_test_005";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    std::vector<int32_t> fileIds;
    
    for (int i = 0; i < 1000; i++) {
        scanner.GetFileIds(fileIds);
        EXPECT_TRUE(fileIds.empty());
    }
}

HWTEST_F(FolderScannerTest, StressTest_007, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/stress_test_007";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    std::string subDirPath = testPath + "/subdir";
    std::filesystem::create_directories(subDirPath, ec);
    
    for (int i = 0; i < 1000; i++) {
        FolderScanner subScanner = scanner.BuildSubDirFolderScanner(subDirPath);
        int32_t albumId = subScanner.GetAlbumId();
        EXPECT_EQ(albumId, -1);
    }
}

HWTEST_F(FolderScannerTest, StressTest_009, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/stress_test_009";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    
    for (int i = 0; i < 100; i++) {
        int32_t ret = scanner.Run();
        EXPECT_EQ(ret, ERR_SUCCESS);
    }
}

HWTEST_F(FolderScannerTest, StressTest_010, TestSize.Level1)
{
    std::string testPath = TEST_ROOT_PATH + "/stress_test_010";
    std::error_code ec;
    std::filesystem::create_directories(testPath, ec);
    
    FolderScanner scanner(testPath, ScanMode::FULL);
    
    for (int i = 0; i < 10; i++) {
        int32_t albumId = scanner.GetAlbumId();
        int32_t addCount = scanner.GetAddCount();
        int32_t updateCount = scanner.GetUpdateCount();
        bool isScan = scanner.IsScanFolderFile();
        std::vector<int32_t> fileIds;
        scanner.GetFileIds(fileIds);
        
        EXPECT_EQ(albumId, -1);
        EXPECT_EQ(addCount, 0);
        EXPECT_EQ(updateCount, 0);
        EXPECT_TRUE(isScan);
        EXPECT_TRUE(fileIds.empty());
    }
}

} // namespace Media
} // namespace OHOS

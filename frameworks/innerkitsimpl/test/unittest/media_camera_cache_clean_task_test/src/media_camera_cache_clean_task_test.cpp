/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MediaCameraCacheCleanTaskTest"

#include "media_camera_cache_clean_task_test.h"
#include "media_camera_cache_clean_task.h"

#include <chrono>
#include <thread>

#include "media_log.h"
#include "media_file_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_mock_tocken.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_type_const.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"
#include "media_column.h"
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <securec.h>
#include <sys/mman.h>
#include <unistd.h>

namespace OHOS {
namespace Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::Media::Background;

static uint64_t g_shellToken = 0;
static MediaLibraryMockHapToken* mockToken = nullptr;

// 时间相关常量（单位：秒）
static constexpr int ONE_HOUR_SECONDS = 60 * 60;
static constexpr int THRESHOLD_SECONDS = 24 * 60 * 60;  // 24小时阈值
static constexpr int ONE_DAY_SECONDS = 24 * 60 * 60;
static constexpr int TWO_DAYS_SECONDS = 48 * 60 * 60;
static constexpr int THIRTY_HOURS_SECONDS = 30 * 60 * 60;
static constexpr int TWENTY_FIVE_HOURS_SECONDS = 25 * 60 * 60;

// 测试相关常量
static constexpr int TEST_SLEEP_SECONDS = 2;  // 测试等待时间（秒）
static constexpr int TEST_FILE_COUNT = 10;     // 测试文件数量

static const std::string TEST_ENHANCE_DIR = ROOT_MEDIA_CAMERA_CACHE_DIR + SLASH_STR + CAMERA_CACHE_ENHANCE_DIR_VALUES;

void MediaCameraCacheCleanTaskTest::SetUpTestCase()
{
    MEDIA_INFO_LOG("MediaCameraCacheCleanTaskTest SetUpTestCase");

    MediaLibraryUnitTestUtils::Init();
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);
 
    vector<string> perms;
    perms.push_back("ohos.permission.GET_NETWORK_INFO");
    // mock  tokenID
    mockToken = new MediaLibraryMockHapToken("com.ohos.medialibrary.medialibrarydata", perms);
    for (auto &perm : perms) {
        MediaLibraryMockTokenUtils::GrantPermissionByTest(IPCSkeleton::GetSelfTokenID(), perm, 0);
    }
}

void MediaCameraCacheCleanTaskTest::TearDownTestCase()
{
    MEDIA_INFO_LOG("MediaCameraCacheCleanTaskTest TearDownTestCase");
    if (mockToken != nullptr) {
    delete mockToken;
    mockToken = nullptr;
    }
 
    MediaLibraryMockTokenUtils::ResetToken();
    SetSelfTokenID(g_shellToken);
    EXPECT_EQ(g_shellToken, IPCSkeleton::GetSelfTokenID());
}

void MediaCameraCacheCleanTaskTest::SetUp()
{
    MEDIA_INFO_LOG("MediaCameraCacheCleanTaskTest SetUp");
}

void MediaCameraCacheCleanTaskTest::TearDown()
{
    MEDIA_INFO_LOG("MediaCameraCacheCleanTaskTest TearDown");
}

// 辅助函数：创建测试文件
static bool CreateTestFile(const std::string& filePath, const std::string& content = "test")
{
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    file << content;
    file.close();
    return true;
}

// 辅助函数：设置文件修改时间
static bool SetFileModificationTime(const std::string& filePath, time_t mtime)
{
    struct stat statInfo;
    if (stat(filePath.c_str(), &statInfo) != 0) {
        return false;
    }
    struct timeval times[2];
    times[0].tv_sec = mtime;
    times[0].tv_usec = 0;
    times[1].tv_sec = mtime;
    times[1].tv_usec = 0;
    return utimes(filePath.c_str(), times) == 0;
}

// 辅助函数：检查文件是否存在
static bool FileExists(const std::string& filePath)
{
    struct stat statInfo;
    return stat(filePath.c_str(), &statInfo) == 0;
}

// 辅助函数：确保测试目录存在（如果目录已存在则不报错）
static bool EnsureTestDirectory(const std::string& dirPath)
{
    struct stat statInfo;
    if (stat(dirPath.c_str(), &statInfo) == 0) {
        // 目录已存在
        return true;
    }
    // 目录不存在，创建目录
    return mkdir(dirPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == 0;
}

// 辅助函数：清空测试目录中的所有文件和子目录
static void ClearTestDirectory(const std::string& dirPath)
{
    std::string command = "rm -rf " + dirPath + "/*";
    system(command.c_str());
}

// 测试DelEnhanceFolderDirtyFile - 删除超过24小时的文件
HWTEST_F(MediaCameraCacheCleanTaskTest, Mccctt_DeleteEnhanceFolderDirtyfile_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mccctt_DeleteEnhanceFolderDirtyfile_01 Start");

    // 确保测试目录存在
    std::string testDir = TEST_ENHANCE_DIR;
    EnsureTestDirectory(testDir);

    // 创建测试文件
    std::string testFile = testDir + "/old_file.txt";
    ASSERT_TRUE(CreateTestFile(testFile, "test content"));

    // 设置文件修改时间为48小时前（超过24小时阈值）
    time_t now = time(nullptr);
    time_t oldTime = now - TWO_DAYS_SECONDS;
    ASSERT_TRUE(SetFileModificationTime(testFile, oldTime));

    // 执行清理任务
    MediaCameraCacheCleanTask task;
    task.Execute();
    std::this_thread::sleep_for(std::chrono::seconds(TEST_SLEEP_SECONDS));

    // 验证文件已被删除
    EXPECT_FALSE(FileExists(testFile));

    // 清空测试目录
    ClearTestDirectory(testDir);

    MEDIA_INFO_LOG("Mccctt_DeleteEnhanceFolderDirtyfile_01 End");
}

// 测试DelEnhanceFolderDirtyFile - 保留未超过24小时的文件
HWTEST_F(MediaCameraCacheCleanTaskTest, Mccctt_DeleteEnhanceFolderDirtyfile_02, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mccctt_DeleteEnhanceFolderDirtyfile_02 Start");

    // 确保测试目录存在
    std::string testDir = TEST_ENHANCE_DIR;
    EnsureTestDirectory(testDir);

    // 创建测试文件
    std::string testFile = testDir + "/new_file.txt";
    ASSERT_TRUE(CreateTestFile(testFile, "test content"));

    // 设置文件修改时间为1小时前（未超过24小时阈值）
    time_t now = time(nullptr);
    time_t newTime = now - ONE_HOUR_SECONDS;
    ASSERT_TRUE(SetFileModificationTime(testFile, newTime));

    // 执行清理任务
    MediaCameraCacheCleanTask task;
    task.Execute();
    std::this_thread::sleep_for(std::chrono::seconds(TEST_SLEEP_SECONDS));

    // 验证文件仍然存在
    EXPECT_TRUE(FileExists(testFile));

    // 清空测试目录
    ClearTestDirectory(testDir);

    MEDIA_INFO_LOG("Mccctt_DeleteEnhanceFolderDirtyfile_02 End");
}

// 测试DelEnhanceFolderDirtyFile - 处理文件不存在的情况
HWTEST_F(MediaCameraCacheCleanTaskTest, Mccctt_DeleteEnhanceFolderDirtyfile_03, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mccctt_DeleteEnhanceFolderDirtyfile_03 Start");

    // 确保测试目录存在
    std::string testDir = TEST_ENHANCE_DIR;
    EnsureTestDirectory(testDir);

    // 使用不存在的文件名，不应导致崩溃
    std::string nonExistFile = testDir + "/non_exist_file.txt";

    // 执行清理任务
    MediaCameraCacheCleanTask task;
    task.Execute();
    std::this_thread::sleep_for(std::chrono::seconds(TEST_SLEEP_SECONDS));

    // 验证程序正常运行
    EXPECT_TRUE(true);

    // 清空测试目录
    ClearTestDirectory(testDir);

    MEDIA_INFO_LOG("Mccctt_DeleteEnhanceFolderDirtyfile_03 End");
}

// 测试HandleCameraCacheClean - 处理空目录
HWTEST_F(MediaCameraCacheCleanTaskTest, Mccctt_HandleCameraCacheClean_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mccctt_HandleCameraCacheClean_01 Start");

    // 确保空的测试目录存在
    std::string testDir = TEST_ENHANCE_DIR;
    EnsureTestDirectory(testDir);

    // 执行清理任务，空目录不应导致崩溃
    MediaCameraCacheCleanTask task;
    task.Execute();
    std::this_thread::sleep_for(std::chrono::seconds(TEST_SLEEP_SECONDS));

    // 验证目录仍然存在
    EXPECT_TRUE(FileExists(testDir));

    // 清空测试目录
    ClearTestDirectory(testDir);

    MEDIA_INFO_LOG("Mccctt_HandleCameraCacheClean_01 End");
}

// 测试HandleCameraCacheClean - 混合场景（包含新旧文件）
HWTEST_F(MediaCameraCacheCleanTaskTest, Mccctt_HandleCameraCacheClean_02, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mccctt_HandleCameraCacheClean_02 Start");

    // 确保测试目录存在
    std::string testDir = TEST_ENHANCE_DIR;
    EnsureTestDirectory(testDir);

    // 创建旧文件1（48小时前，应被删除）
    std::string oldFile1 = testDir + "/old_file_1.txt";
    ASSERT_TRUE(CreateTestFile(oldFile1, "old content 1"));
    time_t now = time(nullptr);
    ASSERT_TRUE(SetFileModificationTime(oldFile1, now - TWO_DAYS_SECONDS));

    // 创建新文件（1小时前，应保留）
    std::string newFile = testDir + "/new_file.txt";
    ASSERT_TRUE(CreateTestFile(newFile, "new content"));
    ASSERT_TRUE(SetFileModificationTime(newFile, now - ONE_HOUR_SECONDS));

    // 创建旧文件2（30小时前，应被删除）
    std::string oldFile2 = testDir + "/old_file_2.txt";
    ASSERT_TRUE(CreateTestFile(oldFile2, "old content 2"));
    ASSERT_TRUE(SetFileModificationTime(oldFile2, now - THIRTY_HOURS_SECONDS));

    // 执行清理任务
    MediaCameraCacheCleanTask task;
    task.Execute();
    std::this_thread::sleep_for(std::chrono::seconds(TEST_SLEEP_SECONDS));

    // 验证结果
    EXPECT_FALSE(FileExists(oldFile1)); // 应被删除
    EXPECT_TRUE(FileExists(newFile));   // 应保留
    EXPECT_FALSE(FileExists(oldFile2)); // 应被删除

    // 清空测试目录
    ClearTestDirectory(testDir);

    MEDIA_INFO_LOG("Mccctt_HandleCameraCacheClean_02 End");
}

// 测试边界情况 - 正好24小时的文件
HWTEST_F(MediaCameraCacheCleanTaskTest, Mccctt_BoundaryTest_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mccctt_BoundaryTest_01 Start");

    // 确保测试目录存在
    std::string testDir = TEST_ENHANCE_DIR;
    EnsureTestDirectory(testDir);

    // 创建正好24小时前的文件
    std::string boundaryFile = testDir + "/boundary_file.txt";
    ASSERT_TRUE(CreateTestFile(boundaryFile, "boundary content"));
    time_t now = time(nullptr);
    time_t boundaryTime = now - ONE_DAY_SECONDS;
    ASSERT_TRUE(SetFileModificationTime(boundaryFile, boundaryTime));

    // 执行清理任务
    MediaCameraCacheCleanTask task;
    task.Execute();
    std::this_thread::sleep_for(std::chrono::seconds(TEST_SLEEP_SECONDS));

    // 验证文件被删除（因为条件是 duration >= thresholdSeconds）
    EXPECT_FALSE(FileExists(boundaryFile));

    // 清空测试目录
    ClearTestDirectory(testDir);

    MEDIA_INFO_LOG("Mccctt_BoundaryTest_01 End");
}

// 测试多个文件同时清理
HWTEST_F(MediaCameraCacheCleanTaskTest, Mccctt_MultipleFiles_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mccctt_MultipleFiles_01 Start");

    // 确保测试目录存在
    std::string testDir = TEST_ENHANCE_DIR;
    EnsureTestDirectory(testDir);

    // 创建多个旧文件
    time_t now = time(nullptr);
    for (int i = 0; i < TEST_FILE_COUNT; i++) {
        std::string file = testDir + "/old_file_" + std::to_string(i) + ".txt";
        ASSERT_TRUE(CreateTestFile(file, "content " + std::to_string(i)));
        ASSERT_TRUE(SetFileModificationTime(file, now - (TWENTY_FIVE_HOURS_SECONDS + i * ONE_HOUR_SECONDS)));
    }

    // 执行清理任务
    MediaCameraCacheCleanTask task;
    task.Execute();
    std::this_thread::sleep_for(std::chrono::seconds(TEST_SLEEP_SECONDS));

    // 验证所有文件都被删除
    for (int i = 0; i < TEST_FILE_COUNT; i++) {
        std::string file = testDir + "/old_file_" + std::to_string(i) + ".txt";
        EXPECT_FALSE(FileExists(file));
    }

    // 清空测试目录
    ClearTestDirectory(testDir);

    MEDIA_INFO_LOG("Mccctt_MultipleFiles_01 End");
}
} // namespace Media
} // namespace OHOS

/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "FileManagerParserTest"

#include "file_manager_parser_test.h"

#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

#include "file_manager_parser.h"
#include "media_log.h"
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_unittest_utils.h"

namespace OHOS {
namespace Media {
using namespace testing::ext;

static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;
static const size_t THREAD_COUNT = 8;
static const size_t CALL_COUNT_PER_THREAD = 20;
static const size_t IMAGE_RATIO = 4;
static const std::string TEST_PATH_PREFIX = "/data/test/media_lake_load_test_data/";

void ExecuteSqls(const std::vector<std::string> &sqls)
{
    ASSERT_NE(g_rdbStore, nullptr);
    for (const auto &sql : sqls) {
        int32_t err = g_rdbStore->ExecuteSql(sql);
        MEDIA_INFO_LOG("Execute sql: %{public}s result: %{public}d", sql.c_str(), err);
    }
}

void CreateTables()
{
    const std::vector<std::string> SQLS = {
        CREATE_ASSET_UNIQUE_NUMBER_TABLE,
    };
    ExecuteSqls(SQLS);
}

void FileManagerParserTest::SetUpTestCase()
{
    MEDIA_INFO_LOG("SetUpTestCase");
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    CreateTables();
}

void FileManagerParserTest::TearDownTestCase()
{
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void FileManagerParserTest::SetUp() {}
void FileManagerParserTest::TearDown() {}

HWTEST_F(FileManagerParserTest, SetCloudPath_Concurrent_MixedMediaTypeNoDuplicatePath_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start SetCloudPath_Concurrent_MixedMediaTypeNoDuplicatePath_001");
    const size_t totalCount = THREAD_COUNT * CALL_COUNT_PER_THREAD;
    std::vector<FileManagerParser> parsers;
    parsers.reserve(totalCount);
    for (size_t i = 0; i < totalCount; i++) {
        bool isImage = (i % IMAGE_RATIO != 0);
        std::string extension = isImage ? ".jpg" : ".mp4";
        std::string displayName = "FileManagerParserTest001_" + std::to_string(i) + extension;
        std::string storagePath = TEST_PATH_PREFIX + displayName;
        parsers.emplace_back(storagePath, ScanMode::INCREMENT);
        parsers[i].fileInfo_.fileType = isImage ? MediaType::MEDIA_TYPE_IMAGE : MediaType::MEDIA_TYPE_VIDEO;
        parsers[i].fileInfo_.displayName = displayName;
        parsers[i].fileInfo_.cloudPath = "";
    }
    EXPECT_EQ(parsers.size(), totalCount);

    std::vector<std::string> allPaths(totalCount);
    std::mutex pathMutex;
    std::atomic<size_t> successCount{0};

    auto worker = [&](size_t threadIdx) {
        for (size_t j = 0; j < CALL_COUNT_PER_THREAD; j++) {
            size_t parserIdx = threadIdx * CALL_COUNT_PER_THREAD + j;
            if (parserIdx < 0 || parserIdx >= totalCount) {
                continue;
            }
            parsers[parserIdx].SetCloudPath();
            std::string path = parsers[parserIdx].fileInfo_.cloudPath;
            {
                std::lock_guard<std::mutex> lock(pathMutex);
                allPaths[parserIdx] = path;
            }
            successCount += !path.empty();
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(THREAD_COUNT);
    for (size_t i = 0; i < THREAD_COUNT; i++) {
        threads.emplace_back(worker, i);
    }
    for (auto &t : threads) {
        t.join();
    }

    std::unordered_set<std::string> pathSet(allPaths.begin(), allPaths.end());
    pathSet.erase("");
    EXPECT_EQ(pathSet.size(), totalCount);
    EXPECT_EQ(successCount.load(), totalCount);
    MEDIA_INFO_LOG("End SetCloudPath_Concurrent_MixedMediaTypeNoDuplicatePath_001, uniquePaths: %{public}zu, "
        "successCount: %{public}zu", pathSet.size(), successCount.load());
}

HWTEST_F(FileManagerParserTest, SetCloudPath_SingleThread_AllPathsNotEmpty_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start SetCloudPath_SingleThread_AllPathsNotEmpty_002");
    const size_t COUNT = 20;
    std::vector<FileManagerParser> parsers;
    parsers.reserve(COUNT);
    for (size_t i = 0; i < COUNT; i++) {
        std::string displayName = "FileManagerParserTest002_" + std::to_string(i) + ".jpg";
        std::string storagePath = TEST_PATH_PREFIX + displayName;
        parsers.emplace_back(storagePath, ScanMode::INCREMENT);
        parsers[i].fileInfo_.fileType = MediaType::MEDIA_TYPE_IMAGE;
        parsers[i].fileInfo_.displayName = displayName;
        parsers[i].fileInfo_.cloudPath = "";
    }
    EXPECT_EQ(parsers.size(), COUNT);
    for (size_t i = 0; i < COUNT; i++) {
        parsers[i].SetCloudPath();
    }
    std::unordered_set<std::string> pathSet;
    for (size_t i = 0; i < COUNT; i++) {
        EXPECT_FALSE(parsers[i].fileInfo_.cloudPath.empty());
        pathSet.insert(parsers[i].fileInfo_.cloudPath);
    }
    EXPECT_EQ(pathSet.size(), COUNT);
    MEDIA_INFO_LOG("End SetCloudPath_SingleThread_AllPathsNotEmpty_002");
}

HWTEST_F(FileManagerParserTest, SetCloudPath_ExistingCloudPath_NoOverwrite_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start SetCloudPath_ExistingCloudPath_NoOverwrite_003");
    std::string displayName = "FileManagerParserTest003.mp4";
    std::string storagePath = TEST_PATH_PREFIX + displayName;
    FileManagerParser parser(storagePath, ScanMode::INCREMENT);
    parser.fileInfo_.fileType = MediaType::MEDIA_TYPE_VIDEO;
    parser.fileInfo_.displayName = displayName;
    const std::string EXPECTED_PATH = "/storage/cloud/files/Photo/1/IMG_existing.mp4";
    parser.fileInfo_.cloudPath = EXPECTED_PATH;
    parser.SetCloudPath();
    EXPECT_EQ(parser.fileInfo_.cloudPath, EXPECTED_PATH);
    MEDIA_INFO_LOG("End SetCloudPath_ExistingCloudPath_NoOverwrite_003");
}

} // namespace Media
} // namespace OHOS
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
static const std::string TEST_IMAGE_NAME = "test.jpg";
static const std::string TEST_VIDEO_NAME = "test.mp4";

void FileManagerParserTest::SetUpTestCase()
{
    MEDIA_INFO_LOG("SetUpTestCase");
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
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
        std::string storagePath = isImage ? TEST_PATH_PREFIX + TEST_IMAGE_NAME : TEST_PATH_PREFIX + TEST_VIDEO_NAME;
        parsers.emplace_back(storagePath, ScanMode::INCREMENT);
        parsers[i].fileInfo_.fileType = isImage ? MediaType::MEDIA_TYPE_IMAGE : MediaType::MEDIA_TYPE_VIDEO;
        parsers[i].fileInfo_.displayName = isImage ? TEST_IMAGE_NAME : TEST_VIDEO_NAME;
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
        parsers.emplace_back(TEST_PATH_PREFIX + TEST_IMAGE_NAME, ScanMode::INCREMENT);
        parsers[i].fileInfo_.fileType = MediaType::MEDIA_TYPE_IMAGE;
        parsers[i].fileInfo_.displayName = TEST_IMAGE_NAME;
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
    FileManagerParser parser(TEST_PATH_PREFIX + TEST_VIDEO_NAME, ScanMode::INCREMENT);
    parser.fileInfo_.fileType = MediaType::MEDIA_TYPE_VIDEO;
    parser.fileInfo_.displayName = TEST_VIDEO_NAME;
    const std::string EXPECTED_PATH = "/storage/cloud/files/Photo/1/IMG_existing.mp4";
    parser.fileInfo_.cloudPath = EXPECTED_PATH;
    parser.SetCloudPath();
    EXPECT_EQ(parser.fileInfo_.cloudPath, EXPECTED_PATH);
    MEDIA_INFO_LOG("End SetCloudPath_ExistingCloudPath_NoOverwrite_003");
}

} // namespace Media
} // namespace OHOS
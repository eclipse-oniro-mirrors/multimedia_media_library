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

static const int32_t THREAD_COUNT = 8;
static const int32_t CALL_COUNT_PER_THREAD = 20;
static const int32_t IMAGE_RATIO = 4;

void FileManagerParserTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
}

void FileManagerParserTest::TearDownTestCase() {}
void FileManagerParserTest::SetUp() {}
void FileManagerParserTest::TearDown() {}

HWTEST_F(FileManagerParserTest, SetCloudPath_Concurrent_MixedMediaTypeNoDuplicatePath_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start SetCloudPath_Concurrent_MixedMediaTypeNoDuplicatePath_001");
    const int32_t totalCount = THREAD_COUNT * CALL_COUNT_PER_THREAD;
    std::vector<FileManagerParser> parsers;
    parsers.reserve(totalCount);
    for (int32_t i = 0; i < totalCount; i++) {
        bool isImage = (i % IMAGE_RATIO != (IMAGE_RATIO - 1));
        std::string ext = isImage ? ".jpg" : ".mp4";
        std::string dummyPath = "/data/test/fm_parser_test/dummy_" + std::to_string(i) + ext;
        parsers.emplace_back(dummyPath, ScanMode::INCREMENT);
        parsers[i].fileInfo_.fileType = isImage ? MediaType::MEDIA_TYPE_IMAGE : MediaType::MEDIA_TYPE_VIDEO;
        parsers[i].fileInfo_.displayName = "dummy_" + std::to_string(i) + ext;
        parsers[i].fileInfo_.cloudPath = "";
    }
    EXPECT_EQ(parsers.size(), static_cast<size_t>(totalCount));

    std::vector<std::string> allPaths(totalCount);
    std::mutex pathMutex;
    std::atomic<int32_t> successCount{0};

    auto worker = [&](int32_t threadIdx) {
        for (int32_t j = 0; j < CALL_COUNT_PER_THREAD; j++) {
            int32_t parserIdx = threadIdx * CALL_COUNT_PER_THREAD + j;
            if (parserIdx < 0 || parserIdx >= totalCount) {
                continue;
            }
            parsers[parserIdx].SetCloudPath();
            std::string path = parsers[parserIdx].fileInfo_.cloudPath;
            {
                std::lock_guard<std::mutex> lock(pathMutex);
                allPaths[parserIdx] = path;
            }
            if (!path.empty()) {
                successCount++;
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(THREAD_COUNT);
    for (int32_t i = 0; i < THREAD_COUNT; i++) {
        threads.emplace_back(worker, i);
    }
    for (auto &t : threads) {
        t.join();
    }

    std::unordered_set<std::string> pathSet(allPaths.begin(), allPaths.end());
    pathSet.erase("");
    EXPECT_EQ(pathSet.size(), static_cast<size_t>(totalCount));
    EXPECT_EQ(successCount.load(), totalCount);
    MEDIA_INFO_LOG("End SetCloudPath_Concurrent_MixedMediaTypeNoDuplicatePath_001, uniquePaths: %{public}zu, successCount: %{public}d",
        pathSet.size(), successCount.load());
}

HWTEST_F(FileManagerParserTest, SetCloudPath_SingleThread_AllPathsNotEmpty_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start SetCloudPath_SingleThread_AllPathsNotEmpty_002");
    const int32_t COUNT = 20;
    std::vector<FileManagerParser> parsers;
    parsers.reserve(COUNT);
    for (int32_t i = 0; i < COUNT; i++) {
        std::string dummyPath = "/data/test/fm_parser_test/single_" + std::to_string(i) + ".jpg";
        parsers.emplace_back(dummyPath, ScanMode::INCREMENT);
        parsers[i].fileInfo_.fileType = MediaType::MEDIA_TYPE_IMAGE;
        parsers[i].fileInfo_.displayName = "single_" + std::to_string(i) + ".jpg";
        parsers[i].fileInfo_.cloudPath = "";
    }
    EXPECT_EQ(parsers.size(), static_cast<size_t>(COUNT));
    for (int32_t i = 0; i < COUNT; i++) {
        parsers[i].SetCloudPath();
    }
    std::unordered_set<std::string> pathSet;
    for (int32_t i = 0; i < COUNT; i++) {
        EXPECT_FALSE(parsers[i].fileInfo_.cloudPath.empty());
        pathSet.insert(parsers[i].fileInfo_.cloudPath);
    }
    EXPECT_EQ(pathSet.size(), static_cast<size_t>(COUNT));
    MEDIA_INFO_LOG("End SetCloudPath_SingleThread_AllPathsNotEmpty_002");
}

HWTEST_F(FileManagerParserTest, SetCloudPath_ExistingCloudPath_NoOverwrite_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start SetCloudPath_ExistingCloudPath_NoOverwrite_003");
    std::string dummyPath = "/data/test/fm_parser_test/existing.jpg";
    FileManagerParser parser(dummyPath, ScanMode::INCREMENT);
    parser.fileInfo_.fileType = MediaType::MEDIA_TYPE_IMAGE;
    parser.fileInfo_.displayName = "existing.jpg";
    const std::string EXPECTED_PATH = "/storage/cloud/files/Photo/1/IMG_existing.jpg";
    parser.fileInfo_.cloudPath = EXPECTED_PATH;
    parser.SetCloudPath();
    EXPECT_EQ(parser.fileInfo_.cloudPath, EXPECTED_PATH);
    MEDIA_INFO_LOG("End SetCloudPath_ExistingCloudPath_NoOverwrite_003");
}

} // namespace Media
} // namespace OHOS
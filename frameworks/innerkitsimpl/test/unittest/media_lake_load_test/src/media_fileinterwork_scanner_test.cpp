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

#define MLOG_TAG "MediaFileInterworkScannerTest"

#include "media_fileinterwork_scanner_test.h"

#include "media_file_utils.h"
#include "media_fileinterwork_scanner.h"
#include "media_log.h"
#include "medialibrary_errno.h"

namespace OHOS {
namespace Media {
using namespace testing::ext;

static const std::string TEST_DIR = "/data/test/media_lake_load_test";
static const std::string TEST_EXIST_IMG = TEST_DIR + "/test.jpg";
static const std::string TEST_EXIST_VIDEO = TEST_DIR + "/test.mp4";

void MediaFileInterworkScannerTest::SetUpTestCase() {}

void MediaFileInterworkScannerTest::TearDownTestCase() {}

void MediaFileInterworkScannerTest::SetUp() {}
void MediaFileInterworkScannerTest::TearDown() {}

HWTEST_F(MediaFileInterworkScannerTest, GetFileInfos_OnlyExistingPathsInResult_001, TestSize.Level1)
{
    const int32_t EXPECTED_RESULT_COUNT = 2;
    const int32_t EXPECTED_IMAGE_COUNT = 1;
    const int32_t EXPECTED_VIDEO_COUNT = 1;

    MEDIA_INFO_LOG("Start GetFileInfos_OnlyExistingPathsInResult_001");
    auto *scanner = MediaFileInterworkScanner::GetInstance();
    ASSERT_NE(scanner, nullptr);
    std::vector<std::string> filePathVector = {
        "/data/test/interwork_scanner_test/nonexistent_file.jpg",
        TEST_EXIST_IMG,
        TEST_EXIST_VIDEO,
        "/data/test/interwork_scanner_test/ghost.mp4"
    };
    UniqueNumber uniqueNumber;
    auto result = scanner->GetFileInfos(filePathVector, uniqueNumber);
    EXPECT_EQ(result.size(), EXPECTED_RESULT_COUNT);
    EXPECT_EQ(uniqueNumber.imageTotalNumber, EXPECTED_IMAGE_COUNT);
    EXPECT_EQ(uniqueNumber.videoTotalNumber, EXPECTED_VIDEO_COUNT);
    for (const auto &info : result) {
        EXPECT_FALSE(info.originFilePath.empty());
        EXPECT_TRUE(MediaFileUtils::IsFileExists(info.originFilePath));
    }
    MEDIA_INFO_LOG("End GetFileInfos_OnlyExistingPathsInResult_001");
}

HWTEST_F(MediaFileInterworkScannerTest, GetFileInfos_AllNonExistent_ReturnEmpty_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start GetFileInfos_AllNonExistent_ReturnEmpty_002");
    auto *scanner = MediaFileInterworkScanner::GetInstance();
    ASSERT_NE(scanner, nullptr);
    std::vector<std::string> filePathVector = {
        "/data/test/interwork_scanner_test/no_such_image.jpg",
        "/data/test/interwork_scanner_test/no_such_video.mp4"
    };
    UniqueNumber uniqueNumber;
    auto result = scanner->GetFileInfos(filePathVector, uniqueNumber);
    EXPECT_TRUE(result.empty());
    EXPECT_EQ(uniqueNumber.imageTotalNumber, 0);
    EXPECT_EQ(uniqueNumber.videoTotalNumber, 0);
    MEDIA_INFO_LOG("End GetFileInfos_AllNonExistent_ReturnEmpty_002");
}

HWTEST_F(MediaFileInterworkScannerTest, GetFileInfos_EmptyInput_ReturnEmpty_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start GetFileInfos_EmptyInput_ReturnEmpty_003");
    auto *scanner = MediaFileInterworkScanner::GetInstance();
    ASSERT_NE(scanner, nullptr);
    std::vector<std::string> filePathVector;
    UniqueNumber uniqueNumber;
    auto result = scanner->GetFileInfos(filePathVector, uniqueNumber);
    EXPECT_TRUE(result.empty());
    EXPECT_EQ(uniqueNumber.imageTotalNumber, 0);
    EXPECT_EQ(uniqueNumber.videoTotalNumber, 0);
    MEDIA_INFO_LOG("End GetFileInfos_EmptyInput_ReturnEmpty_003");
}

} // namespace Media
} // namespace OHOS
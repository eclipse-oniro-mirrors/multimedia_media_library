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

#define MLOG_TAG "C2paUtilsTest"

#include "c2pa_utils_test.h"
#include "c2pa_utils.h"

#include <string>
#include <fstream>
#include <filesystem>

#include "media_log.h"
#include "medialibrary_errno.h"

using namespace testing::ext;
namespace fs = std::filesystem;

namespace OHOS {
namespace Media {
namespace {
const std::string TEST_DIR = "/data/test/c2pa_utils_test/";
const std::string TEST_IMAGE_JPEG = TEST_DIR + "test_image.jpg";
const std::string TEST_IMAGE_HEIC = TEST_DIR + "test_image.heic";
const std::string TEST_OUTPUT_PATH = TEST_DIR + "test_output.jpg";
const std::string TEST_CACHE_PATH = "/storage/media/.cache/cloud/enhancement_temp_image.jpg";
const std::string TEST_AUTHOR_ID = "test_author_id_001";
const std::string TEST_AUTHOR_NAME = "Test Author";
} // namespace

void C2paUtilsTest::SetUpTestCase()
{
    MEDIA_INFO_LOG("[C2paUtilsTest] SetUpTestCase");
    if (!fs::exists(TEST_DIR)) {
        fs::create_directories(TEST_DIR);
    }
}

void C2paUtilsTest::TearDownTestCase()
{
    MEDIA_INFO_LOG("[C2paUtilsTest] TearDownTestCase");
    if (fs::exists(TEST_DIR)) {
        fs::remove_all(TEST_DIR);
    }
}

void C2paUtilsTest::SetUp()
{
    MEDIA_INFO_LOG("[C2paUtilsTest] SetUp");
}

void C2paUtilsTest::TearDown()
{
    MEDIA_INFO_LOG("[C2paUtilsTest] TearDown");
}

HWTEST_F(C2paUtilsTest, HasImageSignature_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("[C2paUtilsTest] HasImageSignature_001 start");
    bool result = C2paUtils::HasImageSignature("");
    EXPECT_FALSE(result) << "Empty path should return false";
    MEDIA_INFO_LOG("[C2paUtilsTest] HasImageSignature_001 end");
}

HWTEST_F(C2paUtilsTest, HasImageSignature_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("[C2paUtilsTest] HasImageSignature_002 start");
    bool result = C2paUtils::HasImageSignature("/non/existent/path/image.jpg");
    EXPECT_FALSE(result) << "Non-existent path should return false";
    MEDIA_INFO_LOG("[C2paUtilsTest] HasImageSignature_002 end");
}

HWTEST_F(C2paUtilsTest, SignForCreate_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("[C2paUtilsTest] SignForCreate_001 start");
    int32_t result = C2paUtils::SignForCreate("", TEST_AUTHOR_ID, TEST_AUTHOR_NAME);
    EXPECT_NE(result, E_OK) << "Empty path should fail";
    MEDIA_INFO_LOG("[C2paUtilsTest] SignForCreate_001 end, result: %{public}d", result);
}

HWTEST_F(C2paUtilsTest, SignForCreate_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("[C2paUtilsTest] SignForCreate_002 start");
    int32_t result = C2paUtils::SignForCreate(TEST_IMAGE_JPEG, "", TEST_AUTHOR_NAME);
    EXPECT_NE(result, E_OK) << "Empty authorId should fail";
    MEDIA_INFO_LOG("[C2paUtilsTest] SignForCreate_002 end, result: %{public}d", result);
}

HWTEST_F(C2paUtilsTest, SignForCreate_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("[C2paUtilsTest] SignForCreate_003 start");
    int32_t result = C2paUtils::SignForCreate(TEST_IMAGE_JPEG, TEST_AUTHOR_ID, "");
    EXPECT_NE(result, E_OK) << "Empty authorName should fail";
    MEDIA_INFO_LOG("[C2paUtilsTest] SignForCreate_003 end, result: %{public}d", result);
}

HWTEST_F(C2paUtilsTest, SignForTranscode_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("[C2paUtilsTest] SignForTranscode_001 start");
    int32_t result = C2paUtils::SignForTranscode("", TEST_OUTPUT_PATH);
    EXPECT_NE(result, E_OK) << "Empty sourcePath should fail";
    MEDIA_INFO_LOG("[C2paUtilsTest] SignForTranscode_001 end, result: %{public}d", result);
}

HWTEST_F(C2paUtilsTest, SignForTranscode_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("[C2paUtilsTest] SignForTranscode_002 start");
    int32_t result = C2paUtils::SignForTranscode(TEST_IMAGE_JPEG, "");
    EXPECT_NE(result, E_OK) << "Empty targetPath should fail";
    MEDIA_INFO_LOG("[C2paUtilsTest] SignForTranscode_002 end, result: %{public}d", result);
}

HWTEST_F(C2paUtilsTest, SignForTranscode_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("[C2paUtilsTest] SignForTranscode_003 start");
    int32_t result = C2paUtils::SignForTranscode("/non/existent/source.jpg", TEST_OUTPUT_PATH);
    EXPECT_NE(result, E_OK) << "Non-existent source should fail";
    MEDIA_INFO_LOG("[C2paUtilsTest] SignForTranscode_003 end, result: %{public}d", result);
}

HWTEST_F(C2paUtilsTest, SignForRevert_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("[C2paUtilsTest] SignForRevert_001 start");
    int32_t result = C2paUtils::SignForRevert("", TEST_OUTPUT_PATH);
    EXPECT_NE(result, E_OK) << "Empty sourcePath should fail";
    MEDIA_INFO_LOG("[C2paUtilsTest] SignForRevert_001 end, result: %{public}d", result);
}

HWTEST_F(C2paUtilsTest, SignForRevert_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("[C2paUtilsTest] SignForRevert_002 start");
    int32_t result = C2paUtils::SignForRevert(TEST_IMAGE_JPEG, "");
    EXPECT_NE(result, E_OK) << "Empty targetPath should fail";
    MEDIA_INFO_LOG("[C2paUtilsTest] SignForRevert_002 end, result: %{public}d", result);
}

HWTEST_F(C2paUtilsTest, SignatureToLcd_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("[C2paUtilsTest] SignatureToLcd_001 start");
    int32_t result = C2paUtils::SignatureToLcd("", TEST_OUTPUT_PATH);
    EXPECT_NE(result, E_OK) << "Empty sourcePath should fail";
    MEDIA_INFO_LOG("[C2paUtilsTest] SignatureToLcd_001 end, result: %{public}d", result);
}

HWTEST_F(C2paUtilsTest, SignatureToLcd_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("[C2paUtilsTest] SignatureToLcd_002 start");
    int32_t result = C2paUtils::SignatureToLcd(TEST_IMAGE_JPEG, "");
    EXPECT_NE(result, E_OK) << "Empty targetPath should fail";
    MEDIA_INFO_LOG("[C2paUtilsTest] SignatureToLcd_002 end, result: %{public}d", result);
}

HWTEST_F(C2paUtilsTest, SignForEnhancement_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("[C2paUtilsTest] SignForEnhancement_001 start");
    int32_t result = C2paUtils::SignForEnhancement("", TEST_OUTPUT_PATH);
    EXPECT_NE(result, E_OK) << "Empty sourcePath should fail";
    MEDIA_INFO_LOG("[C2paUtilsTest] SignForEnhancement_001 end, result: %{public}d", result);
}

HWTEST_F(C2paUtilsTest, SignForEnhancement_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("[C2paUtilsTest] SignForEnhancement_002 start");
    int32_t result = C2paUtils::SignForEnhancement(TEST_IMAGE_JPEG, "");
    EXPECT_NE(result, E_OK) << "Empty targetPath should fail";
    MEDIA_INFO_LOG("[C2paUtilsTest] SignForEnhancement_002 end, result: %{public}d", result);
}

HWTEST_F(C2paUtilsTest, SignForEnhancement_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("[C2paUtilsTest] SignForEnhancement_003 start");
    int32_t result = C2paUtils::SignForEnhancement("/non/existent/source.jpg", TEST_OUTPUT_PATH);
    EXPECT_NE(result, E_OK) << "Non-existent source should fail";
    MEDIA_INFO_LOG("[C2paUtilsTest] SignForEnhancement_003 end, result: %{public}d", result);
}

} // namespace Media
} // namespace OHOS
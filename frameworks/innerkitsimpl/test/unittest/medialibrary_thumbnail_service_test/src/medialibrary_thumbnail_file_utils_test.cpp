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

#include "medialibrary_thumbnail_file_utils_test.h"

#include "medialibrary_errno.h"
#include "media_log.h"

#define private public
#define protected public
#include "thumbnail_file_utils.h"
#undef private
#undef protected

using namespace testing::ext;
using namespace std;

namespace OHOS {
namespace Media {
const int32_t TEST_PIXELMAP_WIDTH_AND_HEIGHT = 100;
const string TEST_IMAGE_PATH = "/storage/cloud/files/Photo/1/CreateImageThumbnailTest_001.jpg";

void MediaLibraryThumbnailFileUtilsTest::SetUpTestCase(void) {}

void MediaLibraryThumbnailFileUtilsTest::TearDownTestCase(void) {}

void MediaLibraryThumbnailFileUtilsTest::SetUp() {}

void MediaLibraryThumbnailFileUtilsTest::TearDown(void) {}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, DeleteMonthAndYearAstc_test_001, TestSize.Level0)
{
    ThumbnailData data;
    auto res = ThumbnailFileUtils::DeleteMonthAndYearAstc(data);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, GetThumbnailSuffix_test_001, TestSize.Level0)
{
    ThumbnailType type = ThumbnailType::MTH;
    auto res = ThumbnailFileUtils::GetThumbnailSuffix(type);
    EXPECT_EQ(res, "");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, DeleteThumbFile_test_001, TestSize.Level0)
{
    ThumbnailType type = ThumbnailType::LCD;
    ThumbnailData data;
    auto res = ThumbnailFileUtils::DeleteThumbFile(data, type);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, DeleteThumbFile_test_002, TestSize.Level0)
{
    ThumbnailType type = ThumbnailType::LCD;
    ThumbnailData data;
    data.path = "/storage/cloud/files/DeleteThumbFile_test_002/DeleteThumbFile_test_002.jpg";
    auto res = ThumbnailFileUtils::DeleteThumbFile(data, type);
    EXPECT_EQ(res, true);
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, DeleteThumbExDir_test_001, TestSize.Level0)
{
    ThumbnailData data;
    auto res = ThumbnailFileUtils::DeleteThumbExDir(data);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, DeleteBeginTimestampDir_test_001, TestSize.Level0)
{
    ThumbnailData data;
    auto res = ThumbnailFileUtils::DeleteBeginTimestampDir(data);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, CheckRemainSpaceMeetCondition_test_001, TestSize.Level0)
{
    int32_t freeSizePercentLimit = 101;
    auto res = ThumbnailFileUtils::CheckRemainSpaceMeetCondition(freeSizePercentLimit);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, DeleteAstcDataFromKvStore_test_001, TestSize.Level0)
{
    ThumbnailData data;
    const ThumbnailType type = ThumbnailType::LCD;
    auto res = ThumbnailFileUtils::DeleteAstcDataFromKvStore(data, type);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, BatchDeleteMonthAndYearAstc_test_001, TestSize.Level0)
{
    ThumbnailDataBatch dataBatch;
    bool res = ThumbnailFileUtils::BatchDeleteMonthAndYearAstc(dataBatch);
    EXPECT_EQ(res, true);
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, BatchDeleteAstcData_test_001, TestSize.Level0)
{
    ThumbnailDataBatch dataBatch;
    ThumbnailType type = ThumbnailType::YEAR_ASTC;
    bool res = ThumbnailFileUtils::BatchDeleteAstcData(dataBatch, type);
    EXPECT_EQ(res, true);

    type = ThumbnailType::MTH_ASTC;
    res = ThumbnailFileUtils::BatchDeleteAstcData(dataBatch, type);
    EXPECT_EQ(res, true);
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, ThumbnailFileUtils_GetThumbFileSize, TestSize.Level0)
{
    size_t size;
    ThumbnailData data;
    bool ret = ThumbnailFileUtils::GetThumbFileSize(data, ThumbnailType::LCD, size);
    EXPECT_EQ(ret, false);
    ret = ThumbnailFileUtils::GetThumbFileSize(data, ThumbnailType::LCD_EX, size);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, ThumbnailFileUtils_GetFileInfo_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("ThumbnailFileUtils_GetFileInfo_001");
    string ret = ThumbnailFileUtils::GetFileInfo(TEST_IMAGE_PATH);
    EXPECT_TRUE(!ret.empty());
    MEDIA_INFO_LOG("ThumbnailFileUtils_GetFileInfo_001 end");
}

} // namespace Media
} // namespace OHOS
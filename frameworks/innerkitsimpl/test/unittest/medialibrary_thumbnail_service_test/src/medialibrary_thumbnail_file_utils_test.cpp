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

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, ThumbnailFileUtils_GetThumbFileSize, TestSize.Level0)
{
    size_t size;
    ThumbnailData data;
    bool ret = ThumbnailFileUtils::GetThumbFileSize(data, ThumbnailType::LCD, size);
    EXPECT_EQ(ret, false);
    ret = ThumbnailFileUtils::GetThumbFileSize(data, ThumbnailType::LCD_EX, size);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, GetThumbnailDir_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetThumbnailDir_test_001 start");
    ThumbnailData data;
    data.path = "";
    string ret = ThumbnailFileUtils::GetThumbnailDir(data);
    EXPECT_EQ(ret, "");
    MEDIA_INFO_LOG("GetThumbnailDir_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, GetThumbExDir_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetThumbExDir_test_001 start");
    ThumbnailData data;
    data.path = "";
    string ret = ThumbnailFileUtils::GetThumbExDir(data);
    EXPECT_EQ(ret, "");
    MEDIA_INFO_LOG("GetThumbExDir_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, DeleteThumbnailDir_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("DeleteThumbnailDir_test_001 start");
    ThumbnailData data;
    data.path = "";
    bool ret = ThumbnailFileUtils::DeleteThumbnailDir(data);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("DeleteThumbnailDir_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, DeleteAllThumbFiles_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("DeleteAllThumbFiles_test_001 start");
    ThumbnailData data;
    data.path = "";
    bool ret = ThumbnailFileUtils::DeleteAllThumbFiles(data);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("DeleteAllThumbFiles_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, RemoveDirectoryAndFile_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("RemoveDirectoryAndFile_test_001 start");
    string path = "";
    bool ret = ThumbnailFileUtils::RemoveDirectoryAndFile(path);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("RemoveDirectoryAndFile_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, RemoveDirectoryAndFile_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("RemoveDirectoryAndFile_test_002 start");
    string path = "/nonexistent/path";
    bool ret = ThumbnailFileUtils::RemoveDirectoryAndFile(path);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("RemoveDirectoryAndFile_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, GetLocalThumbnailFilePath_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetLocalThumbnailFilePath_test_001 start");
    string path = "";
    string ret = ThumbnailFileUtils::GetLocalThumbnailFilePath(path, ThumbnailType::LCD);
    EXPECT_EQ(ret, "");
    MEDIA_INFO_LOG("GetLocalThumbnailFilePath_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, GetLocalThumbnailFilePath_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetLocalThumbnailFilePath_test_002 start");
    string path = "/invalid/path/test.jpg";
    string ret = ThumbnailFileUtils::GetLocalThumbnailFilePath(path, ThumbnailType::LCD);
    EXPECT_EQ(ret, "");
    MEDIA_INFO_LOG("GetLocalThumbnailFilePath_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, DeleteThumbFile_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("DeleteThumbFile_test_003 start");
    ThumbnailType type = ThumbnailType::THUMB;
    ThumbnailData data;
    data.path = "";
    auto res = ThumbnailFileUtils::DeleteThumbFile(data, type);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("DeleteThumbFile_test_003 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, CheckRemainSpaceMeetCondition_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("CheckRemainSpaceMeetCondition_test_002 start");
    int32_t freeSizePercentLimit = 0;
    auto res = ThumbnailFileUtils::CheckRemainSpaceMeetCondition(freeSizePercentLimit);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("CheckRemainSpaceMeetCondition_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, DeleteAstcDataFromKvStore_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("DeleteAstcDataFromKvStore_test_002 start");
    ThumbnailData data;
    data.id = "test_id_001";
    data.dateTaken = "1234567890";
    const ThumbnailType type = ThumbnailType::MTH_ASTC;
    auto res = ThumbnailFileUtils::DeleteAstcDataFromKvStore(data, type);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("DeleteAstcDataFromKvStore_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, DeleteAstcDataFromKvStore_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("DeleteAstcDataFromKvStore_test_0033 start");
    ThumbnailData data;
    data.id = "test_id_002";
    data.dateTaken = "1234567890";
    const ThumbnailType type = ThumbnailType::YEAR_ASTC;
    auto res = ThumbnailFileUtils::DeleteAstcDataFromKvStore(data, type);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("DeleteAstcDataFromKvStore_test_003 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, BatchDeleteAstcData_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("BatchDeleteAstcData_test_002 start");
    ThumbnailDataBatch dataBatch;
    dataBatch.ids.push_back("test_id_001");
    dataBatch.dateTakens.push_back("1234567890");
    ThumbnailType type = ThumbnailType::MTH_ASTC;
    bool res = ThumbnailFileUtils::BatchDeleteAstcData(dataBatch, type);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("BatchDeleteAstcData_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, BatchDeleteAstcData_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("BatchDeleteAstcData_test_003 start");
    ThumbnailDataBatch dataBatch;
    dataBatch.ids.push_back("test_id_001");
    dataBatch.ids.push_back("test_id_002");
    dataBatch.dateTakens.push_back("1234567890");
    dataBatch.dateTakens.push_back("1234567891");
    ThumbnailType type = ThumbnailType::YEAR_ASTC;
    bool res = ThumbnailFileUtils::BatchDeleteAstcData(dataBatch, type);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("BatchDeleteAstcData_test_003 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, BatchDeleteMonthAndYearAstc_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("BatchDeleteMonthAndYearAstc_test_002 start");
    ThumbnailDataBatch dataBatch;
    dataBatch.ids.push_back("test_id_001");
    dataBatch.dateTakens.push_back("1234567890");
    bool res = ThumbnailFileUtils::BatchDeleteMonthAndYearAstc(dataBatch);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("BatchDeleteMonthAndYearAstc_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, ThumbnailFileUtils_GetThumbFileSize_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("ThumbnailFileUtils_GetThumbFileSize_002 start");
    size_t size;
    ThumbnailData data;
    data.path = "/storage/cloud/files/Photo/1/test.jpg";
    bool ret = ThumbnailFileUtils::GetThumbFileSize(data, ThumbnailType::THUMB, size);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("ThumbnailFileUtils_GetThumbFileSize_002 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, ThumbnailFileUtils_GetThumbFileSize_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("ThumbnailFileUtils_GetThumbFileSize_003 start");
    size_t size;
    ThumbnailData data;
    data.path = "/storage/cloud/files/Photo/1/test.jpg";
    bool ret = ThumbnailFileUtils::GetThumbFileSize(data, ThumbnailType::THUMB_ASTC, size);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("ThumbnailFileUtils_GetThumbFileSize_003 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, ThumbnailFileUtils_GetThumbFileSize_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("ThumbnailFileUtils_GetThumbFileSize_004 start");
    size_t size;
    ThumbnailData data;
    data.path = "/storage/cloud/files/Photo/1/test.jpg";
    bool ret = ThumbnailFileUtils::GetThumbFileSize(data, ThumbnailType::THUMB_EX, size);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("ThumbnailFileUtils_GetThumbFileSize_004 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, ThumbnailFileUtils_GetThumbFileSize_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("ThumbnailFileUtils_GetThumbFileSize_005 start");
    size_t size;
    ThumbnailData data;
    data.path = "/storage/cloud/files/Photo/1/test.jpg";
    bool ret = ThumbnailFileUtils::GetThumbFileSize(data, ThumbnailType::LCD_EX, size);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("ThumbnailFileUtils_GetThumbFileSize_005 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, DeleteMonthAndYearAstc_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("DeleteMonthAndYearAstc_test_002 start");
    ThumbnailData data;
    data.id = "test_id_001";;
    data.dateTaken = "1234567890";
    auto res = ThumbnailFileUtils::DeleteMonthAndYearAstc(data);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("DeleteMonthAndYearAstc_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, DeleteMonthAndYearAstc_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("DeleteMonthAndYearAstc_test_003 start");
    ThumbnailData data;
    data.id = "test_id_002";;
    data.dateTaken = "9876543210";
    auto res = ThumbnailFileUtils::DeleteMonthAndYearAstc(data);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("DeleteMonthAndYearAstc_test_003 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, CheckRemainSpaceMeetCondition_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("CheckRemainSpaceMeetCondition_test_003 start");
    int32_t freeSizePercentLimit = -1;
    auto res = ThumbnailFileUtils::CheckRemainSpaceMeetCondition(freeSizePercentLimit);
    EXPECT_EQ(res, true);
    MEDIA_INFO_LOG("CheckRemainSpaceMeetCondition_test_003 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, DeleteAstcDataFromKvStore_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("DeleteAstcDataFromKvStore_test_004 start");
    ThumbnailData data;
    data.id = "";
    data.dateTaken = "1234567890";
    const ThumbnailType type = ThumbnailType::MTH_ASTC;
    auto res = ThumbnailFileUtils::DeleteAstcDataFromKvStore(data, type);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("DeleteAstcDataFromKvStore_test_004 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, DeleteAstcDataFromKvStore_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("DeleteAstcDataFromKvStore_test_005 start");
    ThumbnailData data;
    data.id = "test_id_003";;
    data.dateTaken = "0";
    const ThumbnailType type = ThumbnailType::YEAR_ASTC;
    auto res = ThumbnailFileUtils::DeleteAstcDataFromKvStore(data, type);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("DeleteAstcDataFromKvStore_test_005 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, BatchDeleteAstcData_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("BatchDeleteAstcData_test_004 start");
    ThumbnailDataBatch dataBatch;
    dataBatch.ids.push_back("test_id_001");
    dataBatch.dateTakens.push_back("1234567890");
    dataBatch.ids.push_back("test_id_002");
    dataBatch.dateTakens.push_back("1234567891");
    dataBatch.ids.push_back("test_id_003");
    dataBatch.dateTakens.push_back("1234567892");
    ThumbnailType type = ThumbnailType::MTH_ASTC;
    bool res = ThumbnailFileUtils::BatchDeleteAstcData(dataBatch, type);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("BatchDeleteAstcData_test_004 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, BatchDeleteMonthAndYearAstc_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("BatchDeleteMonthAndYearAstc_test_003 start");
    ThumbnailDataBatch dataBatch;
    dataBatch.ids.push_back("test_id_001");
    dataBatch.dateTakens.push_back("1234567890");
    dataBatch.ids.push_back("test_id_002");
    dataBatch.dateTakens.push_back("1234567891");
    bool res = ThumbnailFileUtils::BatchDeleteMonthAndYearAstc(dataBatch);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("BatchDeleteMonthAndYearAstc_test_003 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, BatchDeleteMonthAndYearAstc_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("BatchDeleteMonthAndYearAstc_test_004 start");
    ThumbnailDataBatch dataBatch;
    for (int i = 0; i < 10; i++) {
        dataBatch.ids.push_back("test_id_" + to_string(i));
        dataBatch.dateTakens.push_back(to_string(1234567890 + i));
    }
    bool res = ThumbnailFileUtils::BatchDeleteMonthAndYearAstc(dataBatch);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("BatchDeleteMonthAndYearAstc_test_004 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, RemoveDirectoryAndFile_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("RemoveDirectoryAndFile_test_003 start");
    string path = "/storage/cloud/files/Photo/1/test.jpg";
    bool ret = ThumbnailFileUtils::RemoveDirectoryAndFile(path);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("RemoveDirectoryAndFile_test_003 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, RemoveDirectoryAndFile_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("RemoveDirectoryAndFile_test_004 start");
    string path = "/storage/cloud/files/Video/1/test.mp4";
    bool ret = ThumbnailFileUtils::RemoveDirectoryAndFile(path);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("RemoveDirectoryAndFile_test_004 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, DeleteAstcDataFromKvStore_test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("DeleteAstcDataFromKvStore_test_006 start");
    ThumbnailData data;
    data.id = "test_id_004";;
    data.dateTaken = "1111111111";
    const ThumbnailType type = ThumbnailType::MTH_ASTC;
    auto res = ThumbnailFileUtils::DeleteAstcDataFromKvStore(data, type);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("DeleteAstcDataFromKvStore_test_006 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, DeleteAstcDataFromKvStore_test_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("DeleteAstcDataFromKvStore_test_007 start");
    ThumbnailData data;
    data.id = "test_id_005";;
    data.dateTaken = "2222222222";
    const ThumbnailType type = ThumbnailType::YEAR_ASTC;
    auto res = ThumbnailFileUtils::DeleteAstcDataFromKvStore(data, type);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("DeleteAstcDataFromKvStore_test_007 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, BatchDeleteAstcData_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("BatchDeleteAstcData_test_005 start");
    ThumbnailDataBatch dataBatch;
    for (int i = 0; i < 5; i++) {
        dataBatch.ids.push_back("test_id_" + to_string(i));
        dataBatch.dateTakens.push_back(to_string(1234567890 + i));
    }
    ThumbnailType type = ThumbnailType::YEAR_ASTC;
    bool res = ThumbnailFileUtils::BatchDeleteAstcData(dataBatch, type);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("BatchDeleteAstcData_test_005 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, BatchDeleteAstcData_test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("BatchDeleteAstcData_test_006 start");
    ThumbnailDataBatch dataBatch;
    for (int i = 0; i < 20; i++) {
        dataBatch.ids.push_back("test_id_" + to_string(i));
        dataBatch.dateTakens.push_back(to_string(1234567890 + i));
    }
    ThumbnailType type = ThumbnailType::MTH_ASTC;
    bool res = ThumbnailFileUtils::BatchDeleteAstcData(dataBatch, type);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("BatchDeleteAstcData_test_006 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, BatchDeleteAstcData_test_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("BatchDeleteAstcData_test_007 start");
    ThumbnailDataBatch dataBatch;
    for (int i = 0; i < 50; i++) {
        dataBatch.ids.push_back("test_id_" + to_string(1000 + i));
        dataBatch.dateTakens.push_back(to_string(1234567890 + i));
    }
    ThumbnailType type = ThumbnailType::YEAR_ASTC;
    bool res = ThumbnailFileUtils::BatchDeleteAstcData(dataBatch, type);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("BatchDeleteAstcData_test_007 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, BatchDeleteMonthAndYearAstc_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("BatchDeleteMonthAndYearAstc_test_005 start");
    ThumbnailDataBatch dataBatch;
    for (int i = 0; i < 15; i++) {
        dataBatch.ids.push_back("test_id_" + to_string(i));
        dataBatch.dateTakens.push_back(to_string(1111111111 + i));
    }
    bool res = ThumbnailFileUtils::BatchDeleteMonthAndYearAstc(dataBatch);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("BatchDeleteMonthAndYearAstc_test_005 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, BatchDeleteMonthAndYearAstc_test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("BatchDeleteMonthAndYearAstc_test_006 start");
    ThumbnailDataBatch dataBatch;
    for (int i = 0; i < 25; i++) {
        dataBatch.ids.push_back("test_id_" + to_string(i));
        dataBatch.dateTakens.push_back(to_string(2222222222 + i));
    }
    bool res = ThumbnailFileUtils::BatchDeleteMonthAndYearAstc(dataBatch);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("BatchDeleteMonthAndYearAstc_test_006 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, BatchDeleteMonthAndYearAstc_test_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("BatchDeleteMonthAndYearAstc_test_007 start");
    ThumbnailDataBatch dataBatch;
    for (int i = 0; i < 30; i++) {
        dataBatch.ids.push_back("test_id_" + to_string(i));
        dataBatch.dateTakens.push_back(to_string(3333333333 + i));
    }
    bool res = ThumbnailFileUtils::BatchDeleteMonthAndYearAstc(dataBatch);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("BatchDeleteMonthAndYearAstc_test_007 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, RemoveDirectoryAndFile_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("RemoveDirectoryAndFile_test_005 start");
    string path = "/storage/cloud/files/Photo/2023/12/25/test.jpg";
    bool ret = ThumbnailFileUtils::RemoveDirectoryAndFile(path);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("RemoveDirectoryAndFile_test_005 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, RemoveDirectoryAndFile_test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("RemoveDirectoryAndFile_test_006 start");
    string path = "/storage/cloud/files/Video/2023/12/25/test.mp4";
    bool ret = ThumbnailFileUtils::RemoveDirectoryAndFile(path);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("RemoveDirectoryAndFile_test_006 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, DeleteAstcDataFromKvStore_test_008, TestSize.Level0)
{
    MEDIA_INFO_LOG("DeleteAstcDataFromKvStore_test_008 start");
    ThumbnailData data;
    data.id = "test_id_006";;
    data.dateTaken = "3333333333";
    const ThumbnailType type = ThumbnailType::MTH_ASTC;
    auto res = ThumbnailFileUtils::DeleteAstcDataFromKvStore(data, type);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("DeleteAstcDataFromKvStore_test_008 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, DeleteAstcDataFromKvStore_test_009, TestSize.Level0)
{
    MEDIA_INFO_LOG("DeleteAstcDataFromKvStore_test_009 start");
    ThumbnailData data;
    data.id = "test_id_007";;
    data.dateTaken = "4444444444";
    const ThumbnailType type = ThumbnailType::YEAR_ASTC;
    auto res = ThumbnailFileUtils::DeleteAstcDataFromKvStore(data, type);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("DeleteAstcDataFromKvStore_test_009 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, BatchDeleteAstcData_test_008, TestSize.Level0)
{
    MEDIA_INFO_LOG("BatchDeleteAstcData_test_008 start");
    ThumbnailDataBatch dataBatch;
    for (int i = 0; i < 8; i++) {
        dataBatch.ids.push_back("test_id_" + to_string(i));
        dataBatch.dateTakens.push_back(to_string(1234567890 + i));
    }
    ThumbnailType type = ThumbnailType::MTH_ASTC;
    bool res = ThumbnailFileUtils::BatchDeleteAstcData(dataBatch, type);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("BatchDeleteAstcData_test_008 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, BatchDeleteAstcData_test_009, TestSize.Level0)
{
    MEDIA_INFO_LOG("BatchDeleteAstcData_test_009 start");
    ThumbnailDataBatch dataBatch;
    for (int i = 0; i < 12; i++) {
        dataBatch.ids.push_back("test_id_" + to_string(i));
        dataBatch.dateTakens.push_back(to_string(9876543210 + i));
    }
    ThumbnailType type = ThumbnailType::YEAR_ASTC;
    bool res = ThumbnailFileUtils::BatchDeleteAstcData(dataBatch, type);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("BatchDeleteAstcData_test_009 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, BatchDeleteMonthAndYearAstc_test_008, TestSize.Level0)
{
    MEDIA_INFO_LOG("BatchDeleteMonthAndYearAstc_test_008 start");
    ThumbnailDataBatch dataBatch;
    for (int i = 0; i < 35; i++) {
        dataBatch.ids.push_back("test_id_" + to_string(i));
        dataBatch.dateTakens.push_back(to_string(4444444444 + i));
    }
    bool res = ThumbnailFileUtils::BatchDeleteMonthAndYearAstc(dataBatch);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("BatchDeleteMonthAndYearAstc_test_008 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, BatchDeleteMonthAndYearAstc_test_009, TestSize.Level0)
{
    MEDIA_INFO_LOG("BatchDeleteMonthAndYearAstc_test_009 start");
    ThumbnailDataBatch dataBatch;
    for (int i = 0; i < 40; i++) {
        dataBatch.ids.push_back("test_id_" + to_string(i));
        dataBatch.dateTakens.push_back(to_string(5555555555 + i));
    }
    bool res = ThumbnailFileUtils::BatchDeleteMonthAndYearAstc(dataBatch);
    EXPECT_EQ(res, false);
    MEDIA_INFO_LOG("BatchDeleteMonthAndYearAstc_test_009 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, RemoveDirectoryAndFile_test_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("RemoveDirectoryAndFile_test_007 start");
    string path = "/storage/cloud/files/Photo/2024/01/01/test.jpg";
    bool ret = ThumbnailFileUtils::RemoveDirectoryAndFile(path);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("RemoveDirectoryAndFile_test_007 end");
}

HWTEST_F(MediaLibraryThumbnailFileUtilsTest, RemoveDirectoryAndFile_test_008, TestSize.Level0)
{
    MEDIA_INFO_LOG("RemoveDirectoryAndFile_test_008 start");
    string path = "/storage/cloud/files/Video/2024/01/01/test.mp4";
    bool ret = ThumbnailFileUtils::RemoveDirectoryAndFile(path);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("RemoveDirectoryAndFile_test_008 end");
}
} // namespace Media
} // namespace OHOS
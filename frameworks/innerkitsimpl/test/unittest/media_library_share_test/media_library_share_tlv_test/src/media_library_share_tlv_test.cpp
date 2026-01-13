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

#define MLOG_TAG "MediaLibraryShareTlvUnitTest"
#define private public
#include "photo_custom_restore_operation.h"
#undef private
#include "directory_ex.h"
#include "medialibrary_errno.h"
#include "media_log.h"
#include "media_library_share_tlv_test.h"
#include "medialibrary_json_operation.h"
#include "tlv_util.h"
#include <fstream>
#include <fcntl.h>
#include "file_ex.h"

using namespace testing::ext;
using namespace std;

namespace OHOS {
namespace Media {
void MediaLibraryShareTlvTest::SetUpTestCase()
{
    MEDIA_INFO_LOG("MediaLibraryShareTlvTest SetUpTestCase");
}

void MediaLibraryShareTlvTest::TearDownTestCase()
{
    MEDIA_INFO_LOG("MediaLibraryShareTlvTest TearDownTestCase");
}

void MediaLibraryShareTlvTest::SetUp()
{
    MEDIA_INFO_LOG("MediaLibraryShareTlvTest SetUp");
}

void MediaLibraryShareTlvTest::TearDown()
{
    MEDIA_INFO_LOG("MediaLibraryShareTlvTest TearDown");
}

HWTEST_F(MediaLibraryShareTlvTest, media_library_maptojson_test, TestSize.Level0) {
    std::unordered_map<std::string, std::variant<int32_t, int64_t, std::string, double>> testMap = {
        {"id", 1001},
        {"name", std::string("test_media_file")},
        {"size", int64_t(1024000)},
        {"duration", 120.5},
        {"width", 1920},
        {"height", 1080},
        {"format", std::string("mp4")}
    };
    
    std::vector<std::string> columns = {"id", "name", "size", "duration", "width", "height", "format"};
    std::string outputPath = "/data/test/media_info_01.json";
    
    int32_t result = MediaJsonOperation::MapToJsonFile(testMap, columns, outputPath);
    EXPECT_EQ(result, E_OK);
    
    std::ifstream inputFile(outputPath);
    ASSERT_TRUE(inputFile.is_open());
    
    nlohmann::json jsonData;
    inputFile >> jsonData;
    inputFile.close();
    
    EXPECT_EQ(jsonData["id"], 1001);
    EXPECT_EQ(jsonData["name"], "test_media_file");
    EXPECT_EQ(jsonData["size"], 1024000);
    EXPECT_EQ(jsonData["duration"], 120.5);
    EXPECT_EQ(jsonData["width"], 1920);
    EXPECT_EQ(jsonData["height"], 1080);
    EXPECT_EQ(jsonData["format"], "mp4");
}

HWTEST_F(MediaLibraryShareTlvTest, media_library_jsontomap_test_01, TestSize.Level0) {
    std::unordered_map<std::string, std::variant<int32_t, int64_t, std::string, double>> testMap = {
        {PhotoColumn::PHOTO_FILE_SOURCE_TYPE, 1001},
        {PhotoColumn::PHOTO_STORAGE_PATH, "test_media_file"},
        {PhotoColumn::PHOTO_HIDDEN_TIME, 1024000},
        {PhotoColumn::PHOTO_LONGITUDE, 120.5},
    };
    std::vector<std::string> columns = {PhotoColumn::PHOTO_FILE_SOURCE_TYPE, PhotoColumn::PHOTO_STORAGE_PATH,
        PhotoColumn::PHOTO_HIDDEN_TIME, PhotoColumn::PHOTO_LONGITUDE};
    std::string outputPath = "/data/test/media_info_02.json";
    int32_t result = MediaJsonOperation::MapToJsonFile(testMap, columns, outputPath);
    EXPECT_EQ(result, E_OK);
    NativeRdb::ValuesBucket values = MediaJsonOperation::ReadJsonToValuesBucket(outputPath, columns);
    EXPECT_TRUE(FileExists(outputPath));
}

HWTEST_F(MediaLibraryShareTlvTest, media_library_jsontomap_test_02, TestSize.Level0) {
    std::unordered_map<std::string, std::variant<int32_t, int64_t, std::string, double>> testMap = {
        {PhotoColumn::PHOTO_FILE_SOURCE_TYPE, "test_media_file"},
        {PhotoColumn::PHOTO_STORAGE_PATH, 789},
        {PhotoColumn::PHOTO_HIDDEN_TIME, "test_media_file"},
        {PhotoColumn::PHOTO_LONGITUDE, 1},
    };
    std::vector<std::string> columns = {PhotoColumn::PHOTO_FILE_SOURCE_TYPE, PhotoColumn::PHOTO_STORAGE_PATH,
        PhotoColumn::PHOTO_HIDDEN_TIME, PhotoColumn::PHOTO_LONGITUDE};
    std::string outputPath = "/data/test/media_info_03.json";
    int32_t result = MediaJsonOperation::MapToJsonFile(testMap, columns, outputPath);
    EXPECT_EQ(result, E_OK);
    NativeRdb::ValuesBucket values = MediaJsonOperation::ReadJsonToValuesBucket(outputPath, columns);
    EXPECT_TRUE(FileExists(outputPath));
}

void GenerateTlvFile(std::string &tlvFilePath)
{
    tlvFilePath = "/data/local/tmp/assets_share_resources/assets_share/test_share/origin_file";
    std::string editDataPath = "/data/local/tmp/assets_share_resources/assets_share/editdata";
    std::string editdataCameraPath = "/data/local/tmp/assets_share_resources/assets_share/editdata_camera";
    std::string srcFilePath = "/data/local/tmp/assets_share_resources/assets_share/CreateImageLcdTest_001.jpg";
    std::string sourceBackPath = "/data/local/tmp/assets_share_resources/assets_share/HasHdrHasRotate.jpg";
    std::string sourcePath = "/data/local/tmp/assets_share_resources/assets_share/HasHdrNoRotate.jpg";
    std::string jsonPath = "/data/test/media_info_01.json";
    std::filesystem::remove_all(tlvFilePath);
    std::filesystem::create_directories(std::filesystem::path(tlvFilePath).parent_path());
    std::string realSrcFilePath;
    PathToRealPath(srcFilePath, realSrcFilePath);
    std::string realSourceFilePath;
    PathToRealPath(sourcePath, realSourceFilePath);
    std::string sourceBackFilePath;
    PathToRealPath(sourceBackPath, sourceBackFilePath);
    auto srcFd = open(realSrcFilePath.c_str(), O_RDONLY);
    ASSERT_GT(srcFd, 0) << "Failed to open srcFile";
    auto sourceFd = open(realSourceFilePath.c_str(), O_RDONLY);
    ASSERT_GT(sourceFd, 0) << "Failed to open source file";
    auto sourcebackFd = open(sourceBackFilePath.c_str(), O_RDONLY);
    ASSERT_GT(sourcebackFd, 0) << "Failed to open source back file";
    TlvFile tlvFile = TlvUtil::CreateTlvFile(tlvFilePath);
    ASSERT_GT(tlvFile, 0);
    UniqueFd tlvFd(tlvFile);
    auto ret = TlvUtil::WriteOriginFileToTlv(tlvFile, srcFilePath, srcFd);
    EXPECT_EQ(ret, 0) << "Failed to write origin file to TLV";
    ASSERT_TRUE(std::filesystem::exists(editdataCameraPath)) << "Camera edit data file does not exist";
    ASSERT_GT(std::filesystem::file_size(editdataCameraPath), 0) << "Camera edit data file is empty";
    ret = TlvUtil::WriteCameraDataToTlv(tlvFile, editdataCameraPath);
    EXPECT_EQ(ret, 0) << "Failed to write camera data to TLV";
    ASSERT_TRUE(std::filesystem::exists(editDataPath)) << "Gallery edit data file does not exist";
    ASSERT_GT(std::filesystem::file_size(editDataPath), 0) << "Gallery edit data file is empty";
    ret = TlvUtil::WriteEditDataToTlv(tlvFile, editDataPath);
    EXPECT_EQ(ret, 0) << "Failed to write edit data to TLV";
    ret = TlvUtil::WriteSourceFileToTlv(tlvFile, sourceFd);
    EXPECT_EQ(ret, 0) << "Failed to write source file to TLV";
    ret = TlvUtil::WriteSourceBackFileToTlv(tlvFile, sourcebackFd);
    EXPECT_EQ(ret, 0) << "Failed to write source back data to TLV";
    ret = TlvUtil::WriteJsonDataToTlv(tlvFile, jsonPath);
    EXPECT_EQ(ret, 0) << "Failed to write json to TLV";
    ret = TlvUtil::UpdateTlvHeadSize(tlvFile);
    EXPECT_EQ(ret, 0) << "Failed to update TLV head size";
    close(tlvFile);
    close(srcFd);
    close(sourceFd);
}

void GenerateTlvFiles(std::string &tlvFilePath)
{
    tlvFilePath = "/data/local/tmp/assets_share_resources/assets_share/test_share/origin_file_01";
    std::string movingPhotoVideoSourceBackPath = "/data/local/tmp/assets_share_resources/assets_share/sourceBack";
    std::string movingPhotoVideoSourcePath = "/data/local/tmp/assets_share_resources/assets_share/source";
    std::filesystem::remove_all(tlvFilePath);
    std::filesystem::create_directories(std::filesystem::path(tlvFilePath).parent_path());
    TlvFile tlvFile = TlvUtil::CreateTlvFile(tlvFilePath);
    ASSERT_GT(tlvFile, 0);
    UniqueFd tlvFd(tlvFile);
    auto movingPhotoVideoSourceBackFd = open(movingPhotoVideoSourceBackPath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    ASSERT_GT(movingPhotoVideoSourceBackFd, 0) << "Failed to open movingPhotoVideoSourceBack";
    const char* data1 = "{this is movingPhotoVideoSourceBack data}";
    ssize_t writeRet = write(movingPhotoVideoSourceBackFd, data1, strlen(data1));
    EXPECT_NE(writeRet, -1) << "Failed to write to movingPhotoVideoSourceBack";
    lseek(movingPhotoVideoSourceBackFd, 0, SEEK_SET);
    auto ret = TlvUtil::WriteMovingPhotoVideoSourceBackFileToTlv(tlvFile, movingPhotoVideoSourceBackFd);
    EXPECT_EQ(ret, 0) << "Failed to write movingPhotoVideoSourceBack data to TLV";
    close(movingPhotoVideoSourceBackFd);
    auto movingPhotoVideoSourceFd = open(movingPhotoVideoSourcePath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    ASSERT_GT(movingPhotoVideoSourceFd, 0) << "Failed to open movingPhotoVideoSource";
    const char* data2 = "{this is movingPhotoVideoSource data}";
    writeRet = write(movingPhotoVideoSourceFd, data2, strlen(data2));
    EXPECT_NE(writeRet, -1) << "Failed to write to movingPhotoVideoSource";
    lseek(movingPhotoVideoSourceFd, 0, SEEK_SET);
    ret = TlvUtil::WriteMovingPhotoVideoSourceFileToTlv(tlvFile, movingPhotoVideoSourceFd);
    EXPECT_EQ(ret, 0) << "Failed to movingPhotoVideoSource data to TLV";
    close(movingPhotoVideoSourceFd);
    ret = TlvUtil::UpdateTlvHeadSize(tlvFile);
    EXPECT_EQ(ret, 0) << "Failed to update TLV head size";
}

HWTEST_F(MediaLibraryShareTlvTest, media_library_WriteFileToTlv_test_001, TestSize.Level0)
{
    std::string destDir = "/data/local/tmp/assets_share_resources/assets_share/test_share/extracted";
    std::string editdataCameraPath = "/data/local/tmp/assets_share_resources/assets_share/editdata_camera";
    auto editdataCameraFd = open(editdataCameraPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    ASSERT_GT(editdataCameraFd, 0) << "Failed to create editdata_camera file";
    const char* data1 = "{this is editdata_camera data}";
    ssize_t writeRet = write(editdataCameraFd, data1, strlen(data1));
    EXPECT_NE(writeRet, -1) << "Failed to write to editdata_camera";
    close(editdataCameraFd);
    std::string editDataPath = "/data/local/tmp/assets_share_resources/assets_share/editdata";
    auto editDataFd = open(editDataPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    ASSERT_GT(editDataFd, 0) << "Failed to create editdata file";
    const char* data2 = "{this is editdata data}";
    writeRet = write(editDataFd, data2, strlen(data2));
    EXPECT_NE(writeRet, -1) << "Failed to write to editdata";
    close(editDataFd);
    std::string tlvFilePath = "";
    GenerateTlvFile(tlvFilePath);
    int32_t ret = TlvUtil::ValidateTlvFile(tlvFilePath);
    EXPECT_EQ(ret, 0) << "TLV file validation failed";
    ASSERT_TRUE(std::filesystem::exists(tlvFilePath)) << "TLV file does not exist";
    ASSERT_GT(std::filesystem::file_size(tlvFilePath), 0) << "TLV file is empty";
    std::filesystem::remove_all(destDir);
    std::filesystem::create_directories(destDir);
    std::unordered_map<TlvTag, std::string> extractedFiles;
    ret = TlvUtil::ExtractTlv(tlvFilePath, destDir, extractedFiles);
    EXPECT_EQ(ret, 0) << "Failed to extract TLV file";
    EXPECT_FALSE(extractedFiles.empty()) << "No files were extracted";
    for (const auto& [tag, filePath] : extractedFiles) {
        bool exists = std::filesystem::exists(filePath);
        auto size = std::filesystem::file_size(filePath);
        EXPECT_TRUE(exists) << "Extracted file does not exist: " << filePath;
        if (exists) {
            EXPECT_GT(size, 0) << "Extracted file is empty: " << filePath;
        }
    }
}

HWTEST_F(MediaLibraryShareTlvTest, media_library_WriteFileToTlv_test_002, TestSize.Level0)
{
    std::string destDir = "/data/local/tmp/assets_share_resources/assets_share/test_share/extracted";
    std::string tlvFilePath = "";
    GenerateTlvFiles(tlvFilePath);
    int32_t ret = TlvUtil::ValidateTlvFile(tlvFilePath);
    EXPECT_EQ(ret, 0) << "TLV file validation failed";
    ASSERT_TRUE(std::filesystem::exists(tlvFilePath)) << "TLV file does not exist";
    ASSERT_GT(std::filesystem::file_size(tlvFilePath), 0) << "TLV file is empty";
    std::filesystem::remove_all(destDir);
    std::filesystem::create_directories(destDir);
    std::unordered_map<TlvTag, std::string> extractedFiles;
    ret = TlvUtil::ExtractTlv(tlvFilePath, destDir, extractedFiles);
    EXPECT_EQ(ret, 0) << "Failed to extract TLV file";
}

HWTEST_F(MediaLibraryShareTlvTest, media_library_WriteFileToTlv_test_003, TestSize.Level0)
{
    std::string tlvFilePath = "/data/local/tmp/assets_share_resources/assets_share/test_share/origin_file_02";
    TlvFile tlvFile = TlvUtil::CreateTlvFile(tlvFilePath);
    ASSERT_GT(tlvFile, 0);
    UniqueFd tlvFd(tlvFile);
    TlvLength dataLength = 100;
    auto ret = TlvUtil::SkipUnknownField(tlvFile, dataLength);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(MediaLibraryShareTlvTest, media_library_WriteFileToTlv_test_004, TestSize.Level0)
{
    int invalidFd = -1;
    TlvLength dataLength = 100;
    auto ret = TlvUtil::SkipUnknownField(invalidFd, dataLength);
    EXPECT_EQ(ret, E_ERR);
}
} // namespace Media
} // namespace OHOS